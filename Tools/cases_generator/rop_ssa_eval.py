"""A test evaluator of the tracer's SSA expansion (rop_ssa.py).

The evaluator runs an `SsaArm` on the homes of a live frame: their words,
read from the objects a test placed in them, and memory read through ctypes
(ir_leaf_eval.RealMemory).  It never writes real memory and never runs
Python.  A primitive row whose C symbol has a spec in ir_leaf.PRIMITIVES runs
that spec.  A row with a model in MODELS runs the model.  Every other row is
opaque.  The opaque rows are those that may run Python, read feedback recipes
or would need a model of CPython's machinery, and the evaluator explores
each of their declared outcomes.  An opaque `-> Bool` row is explored as
true and as false, and an opaque payload is a fresh `Symbol`.

`outcomes()` returns one `Outcome` per path: the exit, the point, every slot
after it, the change in owners of each value, and the feedback the rows on
the path declare they write.  A test runs the C arm on the same frame and
asks which outcomes agree with what C did.

Ownership follows the declarations alone:
- `dup`, and a row's owned (`O`) result or payload, make an owner;
- a `release`, and a linear argument that a row does not borrow, consume
  one;
- a spent source is taken out of its home when it is a temporary, and left
  when it is a local.

A span operand's home ops act on each of its items.
"""

import ctypes
from collections import Counter
from dataclasses import dataclass, field

import ir_leaf
from ir_leaf_eval import Box, Context, HELPER_MODELS
from rop_catalog import AccessMode, FieldKind, Handle, Scalar, Slot
from rop_check import Program, is_linear
from rop_model import Cases, EnumType, HandleType, ScalarType, Type, WrapType
from rop_ssa import BranchTerm, ExitTerm, Jump, OpKind, SsaArm, Switch

EMPTY = 0
PATH_LIMIT = 4096


class Symbol:
    "The unknown payload of an opaque row: bound to C's value on comparison"

    def __init__(self, row: str, case: str, index: int) -> None:
        self.row, self.case, self.index = row, case, index

    def __repr__(self) -> str:
        return f"Symbol({self.row}.{self.case}[{self.index}])"


class Token:
    "An error, route, ticket, candidate or frame: a value with no word"

    def __init__(self, kind: str, what: object = None) -> None:
        self.kind, self.what = kind, what

    def __repr__(self) -> str:
        return f"Token({self.kind}, {self.what!r})"


@dataclass(eq=False)
class Anchors:
    "anchor_for_call's handle: the values read, and which hold an owner"
    items: list
    owned: list[bool]


@dataclass(frozen=True)
class HomeRef:
    operand: str


@dataclass(frozen=True)
class Head:
    index: int


@dataclass(frozen=True)
class EnvRef:
    name: str


@dataclass(frozen=True)
class FbRef:
    index: int


@dataclass
class Site:
    """The word under test, decoded: each operand's value (a slot, a list of
    slots, an immediate, a feedback index, a target head) and whether each
    home is a local"""
    index: int
    length: int
    operands: dict[str, object]
    local: dict[str, bool]


@dataclass
class Outcome:
    exit: str
    point: str | None
    slots: dict[int, object]
    owners: Counter
    feedback: frozenset[str]
    path: tuple[tuple[str, str], ...]   # (row, case) of each opaque choice
    args: tuple = ()
    problems: tuple[str, ...] = ()
    calls: tuple[tuple[str, tuple], ...] = ()   # every row call, with its arguments


class Opaque(Exception):
    "A row the evaluator does not compute"


# ---------------------------------------------------------------------------
# Words


def owns(cx: Context, w) -> bool:
    "Whether a value carries a count: a mortal object word, or a new value"
    if isinstance(w, (Box, Symbol)):
        return True
    return (isinstance(w, int) and w != 0 and (ir_leaf.u64(w) & cx.k("Py_OWNED_MASK")) == 0
            and ir_leaf.u64(w) < cx.k("Py_PTR_LIMIT"))


def key(cx: Context, w):
    "What an owner is an owner of"
    if isinstance(w, (Box, Symbol)):
        return w
    return ir_leaf.u64(w) & ~cx.k("Py_TAG_REFCNT")


def spec(name: str):
    return ir_leaf.PRIMITIVES[name].spec


def obj_of(cx: Context, w):
    "The live object of a pointer-space word"
    if isinstance(w, (Box, Symbol)) or not isinstance(w, int):
        raise Opaque(f"no object for {w!r}")
    return ctypes.cast(key(cx, w), ctypes.py_object).value


def is_object(cx, w) -> bool:
    return isinstance(w, int) and bool(spec("PyValue_IsObject")(cx, w))


def exact(cx, w, symbol: str) -> bool:
    if not is_object(cx, w):
        return False
    return cx.load(key(cx, w) + cx.off("PyObject.ob_type"), "PyTypeObject*") == cx.addr(symbol)


def value_word(cx, name: str) -> int:
    symbol, tag = ir_leaf.VALUE_CONSTANTS[name]
    return cx.addr(symbol) | tag


def fact_holds(cx: Context, fact: str, type_arg, w) -> bool | None:
    "Whether a unary fact holds of a word; None where the evaluator cannot tell"
    if not isinstance(w, int) or isinstance(w, bool):
        return None
    if fact == "inline_int":
        return bool(spec("PyValue_IsInlineInt")(cx, w))
    if fact == "object":
        return is_object(cx, w)
    if fact in ("nonnull", "present"):
        return w != EMPTY
    if fact == "exact" and type_arg is not None:
        if type_arg.name == "int" and spec("PyValue_IsInlineInt")(cx, w):
            return True
        if type_arg.value in ir_leaf.SYMBOLS:
            return exact(cx, w, type_arg.value)
    return None


def is_value_type(t: Type) -> bool:
    "A value that has a word: B or O, or a Maybe of one"
    if isinstance(t, WrapType) and t.wrap.value == "Maybe":
        t = t.arg
    return isinstance(t, HandleType) and t.handle in (Handle.O, Handle.B)


def is_owner_type(t: Type) -> bool:
    if isinstance(t, WrapType) and t.wrap.value == "Maybe":
        t = t.arg
    return t == HandleType(Handle.O)


def concrete(*args) -> bool:
    return not any(isinstance(a, (Symbol, Box)) for a in args)


# ---------------------------------------------------------------------------
# Models of the rows without a leaf spec, by C symbol.  A model takes the
# evaluation and the row's arguments and returns its logical result: a value
# for a `-> T` row, (case, payload) for a row with cases.  It raises Opaque
# where it would need what it cannot see.


def _int_nonzero(ev, v):
    cx = ev.cx
    if spec("PyValue_IsInlineInt")(cx, v):
        return spec("PyValue_InlineInt64")(cx, v) != 0
    return not spec("_PyLong_IsZero")(cx, key(cx, v))


def _float_operands(ev, a, b):
    cx = ev.cx
    return bool(spec("PyValue_FloatOperandValues")(cx, a, b)[0])


def _add_floats(ev, a, b):
    ok, x, y = spec("PyValue_FloatOperandValues")(ev.cx, a, b)
    assert ok
    return ("ok", (spec("PyValue_FromDouble")(ev.cx, x + y),))


def _concat(ev, a, b):
    return ("ok", (Box("str", obj_of(ev.cx, a) + obj_of(ev.cx, b)),))


def _box_i64(ev, v):
    return ("ok", (HELPER_MODELS["_PyValue_BoxInt64"](ev.cx, v),))


def _code_constant(ev, frame, i):
    return ev.constants[i]


def _anchor(ev, homes):
    items, owned = [], []
    for h in homes:
        w = ev.slot_of(h.operand)
        local = ev.site.local[h.operand]
        items.append(w)
        owned.append(local and owns(ev.cx, w))
        if owned[-1]:
            ev.owners[key(ev.cx, w)] += 1
    return Anchors(items, owned)


def _local_view(ev, home):
    return ev.slot_of(home.operand)


def _str_nonempty(ev, v):
    return len(obj_of(ev.cx, v)) != 0


def _home_value(ev, home):
    "A row that spends a source and returns its owner: the source's word"
    return ev.slot_of(home.operand)


def _vacate(ev, home):
    """The owner a row takes out of a local and returns.  A spent local is
    otherwise left (after_row), so the owner that leaves it is consumed
    here; the returned owner is the row's result"""
    w = ev.slot_of(home.operand)
    ev.set_slot(home.operand, EMPTY)
    ev.consume(w)
    return w


def _delete_local(ev, frame, home):
    if ev.slot_of(home.operand) == EMPTY:
        return ("err", (Token("Err", "UnboundLocalError"),))
    return ("ok", (_vacate(ev, home),))


MODELS = {
    "_PyRop_ExactInt": lambda ev, v: bool(spec("PyValue_LongCheckExact")(ev.cx, v)),
    "_PyRop_ExactList": lambda ev, v: exact(ev.cx, v, "PyList_Type"),
    "_PyRop_ExactStr": lambda ev, v: exact(ev.cx, v, "PyUnicode_Type"),
    "_PyRop_BothExactStr": lambda ev, a, b: (exact(ev.cx, a, "PyUnicode_Type")
                                            and exact(ev.cx, b, "PyUnicode_Type")),
    "_PyRop_IntNonzero": _int_nonzero,
    "_PyRop_ListNonempty": lambda ev, v: spec("PyList_GET_SIZE")(ev.cx, key(ev.cx, v)) != 0,
    "_PyRop_StrNonempty": _str_nonempty,
    "_PyRop_TruthObject": lambda ev, test, invert: value_word(
        ev.cx, "PyValue_True" if bool(test) != bool(invert) else "PyValue_False"),
    "_PyRop_BranchTaken": lambda ev, mode, test: bool(test) == bool(mode & 1),
    "_PyRop_BranchDrops": lambda ev, mode: mode < 2,
    "_PyRop_BranchKeeps": lambda ev, mode: mode >= 2,
    "_PyRop_SameBool": lambda ev, a, b: bool(a) == bool(b),
    "_PyValue_BorrowPresent": lambda ev, m: m != EMPTY,
    "_PyRop_FloatOperands": _float_operands,
    "_PyRop_AddFloats": _add_floats,
    "_PyRop_ConcatText": _concat,
    "_PyValue_BoxInt64": _box_i64,
    "_PyRop_SmallInt": lambda ev, i: spec("PyValue_FromInlineInt")(ev.cx, i),
    "_PyRop_CodeConstant": _code_constant,
    "_PyRop_AnchorForCall": _anchor,
    "_PyRop_AnchorArg": lambda ev, a, i: a.items[i],
    "_PyRop_RetireFailedCall": lambda ev, a, s, e: e,
    "_PyRop_RetireAbandoned": lambda ev, a, r: r,
    "_PyRop_LocalView": _local_view,
    "_PyRop_SrcPresent": lambda ev, home: ev.slot_of(home.operand) != EMPTY,
    "_PyRop_RestoreLocal": _home_value,
    "_PyRop_SaveLocal": _vacate,
    "_PyRop_DeleteLocal": _delete_local,
    "_PyRop_UnboundLocal": lambda ev, frame, home: ("err", (Token("Err", "UnboundLocalError"),)),
    "_PyRop_InitResultError": lambda ev, v: ("err", (Token("Err", "TypeError"),)),
    "_PyRop_DiscardResult": lambda ev, z: None,
    "_PyRop_DiscardMaybe": lambda ev, z: None,
    "_PyRop_DefaultEvalFrame": lambda ev, t: True,
}

# The rows the evaluator treats as opaque, and why (the report lists them)
OPAQUE_WHY = {
    "runs_python": "may run Python",
    "feedback": "reads or writes the feedback entry (the specialiser's state)",
    "frame": "pushes, pops or reads frames, the activation or the thread",
    "object": "reads an object's layout through a recipe (dict keys, slots, shapes)",
}


def opaque_reason(program: Program, row: str) -> str:
    x = program.externs[row]
    effects = {e.effect.value for e in x.contract.effects}
    if "runs_python" in effects or "polls" in effects:
        return OPAQUE_WHY["runs_python"]
    if x.contract.feedback:
        return OPAQUE_WHY["feedback"]
    if any(isinstance(p.type, HandleType) and p.type.handle in
           (Handle.Thread, Handle.Frame, Handle.Activation, Handle.DeadFrame,
            Handle.Site) for p in x.params):
        return OPAQUE_WHY["frame"]
    return OPAQUE_WHY["object"]


# ---------------------------------------------------------------------------
# The evaluation of one path


class Path:
    def __init__(self, program: Program, arm: SsaArm, cx: Context, site: Site,
                 slots: dict[int, object], constants, choices: tuple[int, ...]) -> None:
        self.program = program
        self.arm = arm
        self.cx = cx
        self.site = site
        self.slots = dict(slots)
        self.constants = constants
        self.choices = choices
        self.taken: list[int] = []
        self.arity: list[int] = []
        self.picked: list[tuple[str, str]] = []
        self.values: dict[int, object] = {}
        self.owners: Counter = Counter()
        self.feedback: set[str] = set()
        self.point: str | None = None
        self.problems: list[str] = []
        self.calls: list[tuple[str, tuple]] = []
        self.symbols = 0

    # Choices at opaque rows

    def choose(self, n: int) -> int:
        i = len(self.taken)
        c = self.choices[i] if i < len(self.choices) else 0
        self.taken.append(c)
        self.arity.append(n)
        return c

    def fresh(self, t: Type, row: str, case: str):
        self.symbols += 1
        if is_value_type(t):
            return Symbol(row, case, self.symbols)
        return Token(str(t), f"{row}.{case}")

    # Homes

    def slot_of(self, operand: str):
        where = self.site.operands[operand]
        if isinstance(where, list):
            return tuple(self.slots.get(x, EMPTY) for x in where)
        return self.slots.get(where, EMPTY)

    def set_slot(self, operand: str, w) -> None:
        where = self.site.operands[operand]
        if isinstance(where, list):
            # Item i of the value to item i of the span
            for x, item in zip(where, w):
                self.slots[x] = item
            return
        self.slots[where] = w

    def take(self, operand: str):
        "A spent source: a temporary's owner leaves its home; a local stays"
        where = self.site.operands[operand]
        if isinstance(where, list):
            out = []
            for x, local in zip(where, self.site.local[operand]):
                out.append(EMPTY if local else self.slots.get(x, EMPTY))
                if not local:
                    self.slots[x] = EMPTY
            return tuple(out)
        if self.site.local[operand]:
            return EMPTY
        w = self.slot_of(operand)
        self.set_slot(operand, EMPTY)
        return w

    def homes_of(self, v) -> list[str]:
        if isinstance(v, HomeRef):
            return [v.operand]
        if isinstance(v, tuple):
            return [h for item in v for h in self.homes_of(item)]
        return []

    # Ownership

    def make_owner(self, w) -> None:
        if isinstance(w, tuple):
            for item in w:
                self.make_owner(item)
        elif owns(self.cx, w):
            self.owners[key(self.cx, w)] += 1

    def consume(self, w) -> None:
        if isinstance(w, Anchors):
            for item, owned in zip(w.items, w.owned):
                if owned:
                    self.owners[key(self.cx, item)] -= 1
            w.owned = [False] * len(w.owned)
        elif isinstance(w, tuple):
            for item in w:
                self.consume(item)
        elif owns(self.cx, w):
            self.owners[key(self.cx, w)] -= 1

    # Rows

    def row(self, name: str, args: list, result_type: Type | None):
        """Run a row: (case, payload) for a row with cases, else its value"""
        x = self.program.externs[name]
        self.calls.append((name, tuple(args)))
        for access in x.contract.feedback:
            if access.mode is AccessMode.write and any(isinstance(a, FbRef) for a in args):
                self.feedback.add(access.field)
        symbol = x.c.symbol
        result = None
        model = MODELS.get(symbol)
        unproven = self.unproven(x, args)
        try:
            if unproven:
                # A model or spec would read memory the row may not touch
                self.problems.append(f"{name} requires {unproven}, which does not hold")
                raise Opaque(name)
            if model is not None and concrete(*args):
                result = model(self, *args)
            elif symbol in ir_leaf.PRIMITIVES and concrete(*args) and all(
                    isinstance(a, (int, float, bool)) for a in args):
                result = self.run_spec(symbol, args, x)
            else:
                raise Opaque(name)
        except Opaque:
            result = self.opaque(name, x)
        self.after_row(x, args, result)
        return result

    def unproven(self, x, args) -> str | None:
        "The first of the row's `requires` facts that the arguments refute"
        names = [p.name for p in x.params]
        for f in x.contract.requires:
            values = [args[names.index(s)] for s in f.subjects if s in names]
            if len(values) == 1 and fact_holds(self.cx, f.fact.value, f.type_arg,
                                               values[0]) is False:
                return f"{f.fact.value}({', '.join(f.subjects)})"
        return None

    def run_spec(self, symbol: str, args, x):
        r = spec(symbol)(self.cx, *args)
        if isinstance(x.result, Cases):
            flag, out = r
            case = x.result.cases[1 if flag else 0]
            return (case.name, (out,))
        return r

    def opaque(self, name: str, x):
        if isinstance(x.result, Cases):
            c = self.choose(len(x.result.cases))
            case = x.result.cases[c]
            self.picked.append((name, case.name))
            return (case.name, tuple(self.fresh(p.type, name, case.name) for p in case.params))
        t = x.result.type
        if t == ScalarType(Scalar.Bool):
            c = self.choose(2)
            self.picked.append((name, str(c == 0)))
            return c == 0
        if t == ScalarType(Scalar.Unit):
            self.picked.append((name, "unit"))
            return None
        return self.fresh(t, name, "result")

    def after_row(self, x, args, result) -> None:
        "The ownership the row's declaration states"
        for p, a in zip(x.params, args):
            if p.name in x.contract.spends:
                for h in self.homes_of(a):
                    self.consume(self.take(h))
            elif is_linear(p.type) and p.name not in x.contract.borrows:
                self.consume(a)
        if isinstance(x.result, Cases):
            case, payload = result
            decl = next(c for c in x.result.cases if c.name == case)
            for p, v in zip(decl.params, payload):
                if is_owner_type(p.type):
                    self.make_owner(v)
        elif is_owner_type(x.result.type):
            self.make_owner(result)

    # Ops

    def operand(self, name: str):
        kind = self.arm.operands[name]
        v = self.site.operands[name]
        if kind is FieldKind.feedback:
            return FbRef(v)
        if kind is FieldKind.target:
            return Head(v)
        return v

    def op(self, op) -> Outcome | None:
        a = [self.values[x] for x in op.args]
        k = op.kind
        r = None
        if k is OpKind.read:
            r = self.slot_of(op.attr)
        elif k is OpKind.home:
            r = HomeRef(op.attr)
        elif k is OpKind.operand:
            r = self.operand(op.attr)
        elif k is OpKind.env:
            r = {"site": Head(self.site.index),
                 "next_head": Head(self.site.index + self.site.length)}.get(
                     op.attr, EnvRef(op.attr))
        elif k is OpKind.const:
            r = op.attr
            if isinstance(r, str):
                t = self.arm.types[op.result]
                assert isinstance(t, EnumType)
                r = self.program.enums[t.name].members.index(r.rpartition(".")[2])
        elif k is OpKind.span:
            r = tuple(a)
        elif k in (OpKind.prim, OpKind.call):
            r = self.row(op.attr, a, self.arm.types.get(op.result))
        elif k is OpKind.dup:
            r = a[0]
            self.make_owner(r)
        elif k is OpKind.acquire:
            if self.site.local[op.attr]:
                r = self.slot_of(op.attr)
                self.make_owner(r)
            else:
                r = self.take(op.attr)
        elif k is OpKind.unwrap:
            if a[0] == EMPTY:
                self.problems.append("unwrap of an absent value")
            r = a[0]
        elif k is OpKind.candidate:
            r = Token("Candidate", op.attr)
        elif k is OpKind.guard:
            if not a[0]:
                return self.finish("miss", op.attr.state, ())
        elif k is OpKind.take:
            r = self.take(op.attr)
        elif k is OpKind.old:
            r = self.slot_of(op.attr)
        elif k is OpKind.store:
            self.set_slot(op.attr, a[0])
        elif k is OpKind.null:
            r = EMPTY
        elif k is OpKind.count:
            r = len(self.site.operands[op.attr]) if op.attr else len(a[0])
        elif k is OpKind.rest:
            r = a[0][len(self.site.operands[op.attr]):]
        elif k is OpKind.checkpoint:
            self.point = op.attr
        elif k is OpKind.release:
            self.consume(a[0])
        else:
            raise NotImplementedError(k)
        if op.result is not None:
            self.values[op.result] = r
        return None

    def bind(self, block: str, args: tuple) -> str:
        target = self.arm.blocks[block]
        for p, v in zip(target.params[len(target.params) - len(args):], args):
            self.values[p] = v
        return block

    def finish(self, kind: str, state, args) -> Outcome:
        if kind == "exit":
            self.consume(args[0])
        return Outcome(kind, state.point, self.slots, self.owners, frozenset(self.feedback),
                       tuple(self.picked), args, tuple(self.problems), tuple(self.calls))

    def run(self) -> Outcome:
        label = self.arm.entry.label
        while True:
            block = self.arm.blocks[label]
            for op in block.ops:
                out = self.op(op)
                if out is not None:
                    return out
            t = block.term
            if isinstance(t, Jump):
                label = self.bind(t.block, tuple(self.values[a] for a in t.args))
            elif isinstance(t, BranchTerm):
                edge = t.if_true if self.values[t.cond] else t.if_false
                label = self.bind(edge.block, tuple(self.values[a] for a in edge.args))
            elif isinstance(t, Switch):
                case, payload = self.row(t.row, [self.values[a] for a in t.args], None)
                edge = next(e for e in t.edges if e.case == case)
                for p, v in zip(edge.payload, payload):
                    self.values[p] = v
                label = self.bind(edge.target.block,
                                  tuple(self.values[a] for a in edge.target.args))
            elif isinstance(t, ExitTerm):
                return self.finish(t.kind, t.state, tuple(self.values[a] for a in t.args))
            else:
                raise NotImplementedError(t)


def outcomes(program: Program, arm: SsaArm, cx: Context, site: Site,
             slots: dict[int, object], constants=()) -> list[Outcome]:
    """Every path of the arm from the frame: one Outcome per combination of
    outcomes of its opaque rows"""
    found: list[Outcome] = []
    work: list[tuple[int, ...]] = [()]
    while work:
        if len(found) >= PATH_LIMIT:
            raise RuntimeError(f"{arm.name}: more than {PATH_LIMIT} paths")
        choices = work.pop()
        path = Path(program, arm, cx, site, slots, constants, choices)
        found.append(path.run())
        for i in range(len(choices), len(path.taken)):
            for c in range(1, path.arity[i]):
                work.append(tuple(path.taken[:i]) + (c,))
    return found


# ---------------------------------------------------------------------------
# The word under test, and what C did


IR_F_LOCAL_SHIFT = 9    # IR_F_LOCAL_A; B, C and D follow (pycore_ir.h)
HEAD_FIELDS = ("A", "B", "C", "D")


def field(word: int, payload: list[int], slot) -> int:
    offset, width = slot.value
    if offset < 64:
        return (word >> offset) & ((1 << width) - 1)
    offset -= 64
    return (payload[offset // 64] >> (offset % 64)) & ((1 << width) - 1)


def decode_site(program: Program, family: str, words: list[int], index: int,
                length: int) -> Site:
    """The operands of word index: homes as slots, immediates as numbers, a
    target as the absolute index of its head"""
    word = words[index]
    payload = words[index + 1:index + length]
    operands: dict[str, object] = {}
    local: dict[str, bool] = {}
    enc = program.encodings[family]
    # The elements of the word's spans, in declaration order: first the head
    # fields the encoding gives them, then the words after the fixed ones,
    # four to a word (pycore_ir.h, _PyIR_SpanElement())
    in_head = sorted(enc.elements, key=lambda s: HEAD_FIELDS.index(s.name))
    taken = 0

    def element(j: int) -> tuple[int, bool]:
        if j < len(in_head):
            f = HEAD_FIELDS.index(in_head[j].name)
            return (field(word, [], in_head[j]),
                    bool(word >> (IR_F_LOCAL_SHIFT + f) & 1))
        j -= len(in_head)
        w = payload[enc.words + j // 4]
        name = HEAD_FIELDS[j % 4]
        return field(w, [], Slot[name]), bool(w >> (IR_F_LOCAL_SHIFT + j % 4) & 1)

    for p in enc.placements:
        value = 0
        for i, slot in enumerate(p.slots):
            value |= field(word, payload, slot) << (12 * i)
        if p.kind is FieldKind.target:
            if value >> (p.bits - 1):
                value -= 1 << p.bits
            value += index + length
        if p.kind is FieldKind.vector:
            # The count; the span's elements follow the earlier spans'
            slots, flags = [], []
            for i in range(value):
                slot, flag = element(taken + i)
                slots.append(slot)
                flags.append(flag)
            taken += value
            operands[p.operand] = slots
            local[p.operand] = flags
            continue
        operands[p.operand] = value
        if p.kind is FieldKind.home:
            name = p.slots[0].name
            local[p.operand] = bool(word >> (IR_F_LOCAL_SHIFT + HEAD_FIELDS.index(name)) & 1)
    return Site(index, length, operands, local)


@dataclass
class Observation:
    """A C run of the word: the exit (next, branch, jump, raise, again,
    miss, redirect), the next head for a branch, every slot as
    jit_template_run reports it, and whether the feedback entry or the
    word's handler changed"""
    exit: str
    next: int | None
    slots: tuple
    feedback_written: bool
    handler_written: bool
    delivered: tuple | None = None      # (value,) for a return to the caller


def same_value(cx: Context, pred, c, binding: dict) -> bool:
    if isinstance(pred, Symbol):
        if pred in binding:
            b = binding[pred]
            return b is c or (type(b) is type(c) and b == c)
        binding[pred] = c
        return True
    if isinstance(pred, Box):
        if type(c).__name__ != pred.type:
            return False
        return c == pred.value or (c != c and pred.value != pred.value)
    if isinstance(c, int) and not isinstance(c, bool) and pred == c:
        return True
    if pred == EMPTY:
        return c == 0
    return isinstance(pred, int) and ir_leaf.u64(pred) < cx.k("Py_PTR_LIMIT") and key(cx, pred) == id(c)


def disagreement(cx: Context, out: Outcome, obs: Observation, homes: dict[int, str]) -> str | None:
    """Why an outcome does not agree with C, or None.  homes maps each slot
    that is an operand home to its operand"""
    if out.exit != obs.exit:
        return f"exit {out.exit}, C took {obs.exit}"
    if (out.exit in ("branch", "jump") and obs.next is not None and out.args
            and isinstance(out.args[0], Head) and out.args[0].index != obs.next):
        return f"{out.exit} to {out.args[0].index}, C went to {obs.next}"
    binding: dict = {}
    if obs.delivered is not None:
        # A return: the frame is gone; the value delivered to the caller is
        # the argument of the path's deliver row
        values = [args[1] for row, args in out.calls if row == "deliver"]
        if len(values) != 1 or not same_value(cx, values[0], obs.delivered[0], binding):
            return f"delivered {values!r}, C delivered {obs.delivered[0]!r}"
        return None
    for slot, operand in homes.items():
        pred = out.slots.get(slot, EMPTY)
        if not same_value(cx, pred, obs.slots[slot], binding):
            return f"home {operand} (slot {slot}): {pred!r}, C left {obs.slots[slot]!r}"
    if obs.feedback_written and not (out.feedback & {"counter", "entry"} or any(
            f.startswith("entry.") for f in out.feedback)):
        return "C wrote the feedback entry; no row on the path declares a write"
    if obs.handler_written and "handler" not in out.feedback:
        return "C changed the word's handler; no row on the path declares write(handler)"
    return None


def balance(cx: Context, out: Outcome, before: dict[int, object]) -> str | None:
    """The expansion's own ownership: the owners it made and consumed of
    each value equal the change in the homes that hold it"""
    def counts(slots):
        c = Counter()
        for w in slots.values():
            if owns(cx, w):
                c[key(cx, w)] += 1
        return c
    after, start = counts(out.slots), counts(before)
    for k in set(after) | set(start) | set(out.owners):
        if out.owners[k] != after[k] - start[k]:
            return (f"owners of {k!r}: the path made {out.owners[k]:+d}, "
                    f"the homes changed by {after[k] - start[k]:+d}")
    return None
