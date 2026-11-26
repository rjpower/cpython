"""Leaf graphs and baseline templates of register instruction arms.

A template is derived from the arm's SSA expansion (`rop_ssa.SsaArm`), the
graph the tracer uses and test_rop_ssa runs against C: one graph for both.
`leaf_of_arm` lowers the expansion into the leaf IR of ir_leaf.py
(`LeafBody`):

- a home read is an `arg`, an immediate operand is folded where it is used
  (`jconst`), a feedback-entry field is a `cache` read typed by the schema's
  C layout (rop_catalog.FB_LAYOUT), a recipe is a record of leaf values;
- a row is its leaf lowering: the primitive of rop_primitives.PRIMITIVES that its C
  symbol names, a lowering in ROW_LOWERINGS, or a call of a helper of
  rop_primitives.HELPERS;
- a row with cases branches on the test that its C convention gives each
  case (`outcome_test`, over rop_catalog.CONVENTIONS);
- a guard leaves for the miss; `publish` (take, old, store, checkpoint,
  release) becomes a `PublishPlan`, the same on every path that publishes.

`template_of_arm` joins the body with its publication, the home-class
conditional detach, load, store and close that jit_template_generator.Builder
emits for a stack handler, and emits it with jit_template_generator.  An arm
that uses anything else keeps the call of its form; `Unsupported` says why.
"""

from dataclasses import dataclass
from typing import Callable

import rop_primitives
import jit_template_generator as jt
from rop_catalog import (CONVENTIONS, FB_LAYOUT, RECIPE_LAYOUT, AccessMode, CLayout,
                         CSource, CTest, DebtKind, Fact, FieldKind, Handle, Scalar,
                         Schema, SchemaCtor, Wrap)
from rop_cgen import Unsupported, arm_facts, flatten, home_views
from rop_check import Program
from rop_model import (Arm, EnumType, Extern, HandleType, Publish, ScalarType, SchemaType,
                       Type, WrapType)
from rop_ssa import (INTACT, BranchTerm, ExitTerm, Jump, Op, OpKind, SsaArm, Switch,
                     expand)

LEAF_SCALAR = {
    Scalar.Bool: "bool", Scalar.U8: "u8", Scalar.U16: "u16", Scalar.U32: "u32",
    Scalar.U64: "u64", Scalar.I32: "i32", Scalar.I64: "i64", Scalar.F64: "f64",
    Scalar.I51: "i64", Scalar.Scaled51: "i64", Scalar.Size: "u64",
}
LEAF_HANDLE = {Handle.B: rop_primitives.VALUE, Handle.O: rop_primitives.VALUE,
               Handle.Obj: "PyObject*"}

EXIT = "EXIT"


def leaf_type(t: Type) -> str | None:
    """The leaf IR type of a value, None for one that has no leaf value (a
    token such as `Err`, a handle such as `Fb`, a record)"""
    if isinstance(t, WrapType) and t.wrap is Wrap.Maybe:
        return leaf_type(t.arg)
    if isinstance(t, ScalarType):
        return LEAF_SCALAR.get(t.scalar)
    if isinstance(t, HandleType):
        return LEAF_HANDLE.get(t.handle)
    return None


def ctype_leaf(ctype: str) -> str | None:
    "The leaf type of a C type of a layout field"
    if ctype == "_PyValueGuarded":
        return rop_primitives.VALUE
    if ctype.endswith("*"):
        target = ctype.rstrip("* ")
        return f"{target}*" if target in rop_primitives.POINTER_TARGETS else None
    return rop_primitives.C_TYPES.get(ctype)


# What the leaf graph holds for a value of the expansion that is no leaf value

@dataclass(frozen=True)
class Imm:
    "An immediate of the word: a C expression over ir_word, known at compile time"
    expr: str
    type: str


@dataclass(frozen=True)
class Entry:
    "The word's feedback entry"
    schema: Schema


@dataclass(frozen=True)
class Opaque:
    "A name, target, thread or frame operand: no code of its own"
    kind: str


@dataclass(frozen=True)
class Record:
    "A recipe: one leaf value per field of its C layout, in layout order"
    schema: Schema
    fields: tuple[tuple[str, int], ...]

    def __getitem__(self, name: str) -> int:
        return dict(self.fields)[name]


@dataclass(frozen=True)
class Conj:
    "A test that holds when every one of its bools does, tested in order"
    tests: tuple[int, ...]


Rep = int | Imm | Entry | Opaque | Record | Conj | None


def immediate_expr(p) -> str:
    """An immediate's C expression over the head: its fields, low field
    first; an immediate packed into a shared field is shifted and masked"""
    if len(p.slots) == 1:
        field = f"IR_{p.slots[0].name}(ir_word)"
        if p.shared:
            return f"((({field}) >> {p.shift}) & {(1 << p.bits) - 1:#x})"
        return field
    return "(" + " | ".join(f"((uint32_t)IR_{s.name}(ir_word) << {12 * i})" if i else
                            f"(uint32_t)IR_{s.name}(ir_word)"
                            for i, s in enumerate(p.slots)) + ")"


def record_fields(schema: Schema) -> list[tuple[str, str]]:
    layout = RECIPE_LAYOUT.get(schema)
    if layout is None:
        raise Unsupported(f"no recipe layout for {schema.value}")
    fields = []
    for name, f in layout.fields.items():
        t = ctype_leaf(f.ctype)
        if t is None:
            raise Unsupported(f"recipe field {name} has no leaf type")
        fields.append((name, t))
    return fields


# The publication

@dataclass(frozen=True)
class Step:
    """One op of a publish: take, old or store of a home, or a release of a
    debt (kind) of a home"""
    op: str
    home: str
    kind: DebtKind | None = None


@dataclass(frozen=True)
class PublishPlan:
    "The publish that ends every publishing path, in the expansion's order"
    steps: tuple[Step, ...]


@dataclass
class ArmLeaf:
    body: rop_primitives.LeafBody
    plan: PublishPlan | None
    # Home operands the body reads, in order of first use
    inputs: tuple[str, ...]
    # Operands whose values are known inline ints on every path to the publish
    inline: frozenset[str]
    # The source homes whose owner the arm acquires, in order
    acquired: tuple[str, ...] = ()
    # The feedback entry's C type when the body reads the entry
    entry: str | None = None


def successors(t) -> list[str]:
    if isinstance(t, Jump):
        return [t.block]
    if isinstance(t, BranchTerm):
        return [t.if_true.block, t.if_false.block]
    if isinstance(t, Switch):
        return [e.target.block for e in t.edges]
    return []


def blocks_after_acquire(ssa: SsaArm) -> set[str]:
    "The blocks that a path reaches after it acquires an owner"
    work = [s for b in ssa.blocks.values()
            if any(op.kind is OpKind.acquire for op in b.ops)
            for s in successors(b.term)]
    after: set[str] = set()
    while work:
        label = work.pop()
        if label not in after:
            after.add(label)
            work.extend(successors(ssa.blocks[label].term))
    return after


def layout_of(arm: Arm, entry: Entry) -> CLayout:
    layout = FB_LAYOUT.get(entry.schema)
    if layout is None:
        raise Unsupported(f"{arm.name}: no C layout for feedback {entry.schema.value}")
    return layout


class LeafBuilder:
    def __init__(self, program: Program, arm: Arm, ssa: SsaArm) -> None:
        self.program = program
        self.arm = arm
        self.ssa = ssa
        self.types: dict[int, str] = {}
        self.ids: dict[str, int] = {}
        self.env: dict[int, Rep] = {}
        self.inputs: list[str] = []
        self.plan: PublishPlan | None = None
        self.count = 0
        self.block_list: list[rop_primitives.Block] = []
        self.effects: set[str] = set()
        self.helpers: list[str] = []
        self.entry: str | None = None
        # The row being lowered, whose feedback clause bounds its entry
        # reads, and the entry fields read so far
        self.lowering: Extern | None = None
        self.fields_read: set[str] = set()
        self.acquired: list[str] = []
        # Whether the path to the current op has acquired an owner
        self.holding = False
        self.cur: rop_primitives.Block
        # The publish ops of the current block so far, and the home whose
        # owner each take or old gives
        self.steps: list[Step] = []
        self.owners: dict[int, str] = {}

    def refuse(self, why: str):
        raise Unsupported(f"{self.arm.name}: {why}")

    # Values and ops

    def value(self, t: str) -> int:
        self.types[self.count] = t
        self.count += 1
        return self.count - 1

    def new_block(self) -> rop_primitives.Block:
        b = rop_primitives.Block(len(self.block_list))
        self.block_list.append(b)
        return b

    def emit(self, kind: str, types: tuple[str, ...], args: tuple[int, ...] = (),
             attr=None) -> tuple[int, ...]:
        results = tuple(self.value(t) for t in types)
        self.cur.ops.append(rop_primitives.Op(kind, results, args, attr))
        return results

    def emit1(self, kind: str, t: str, args: tuple[int, ...] = (), attr=None) -> int:
        return self.emit(kind, (t,), args, attr)[0]

    def leaf(self, rep: Rep) -> int:
        "The leaf value of rep: an immediate becomes a constant of the word"
        if isinstance(rep, Imm):
            return self.emit1("jconst", rep.type, (), rep.expr)
        if not isinstance(rep, int):
            self.refuse(f"{rep} is not a leaf value")
        return rep

    def const(self, t: str, x) -> int:
        return self.emit1("const", t, (), x)

    def prim(self, name: str, *args: int) -> tuple[int, ...]:
        p = rop_primitives.PRIMITIVES[name]
        return self.emit("prim", p.results, args, name)

    def prim1(self, name: str, *args: int) -> int:
        (r,) = self.prim(name, *args)
        return r

    def cmp(self, pred: str, a: int, b: int) -> int:
        return self.emit1("cmp", "bool", (a, b), pred)

    def load(self, field: str, ptr: int) -> int:
        "A field of rop_primitives.FIELDS, or a leaf type read through the pointer"
        t = rop_primitives.FIELDS[field].type if field in rop_primitives.FIELDS else field
        return self.emit1("load", t, (ptr,), field)

    def choose(self, test: int, t: str, yes: Callable[[], int], no: Callable[[], int]) -> int:
        """yes() when the test holds, else no(), each in a block of its own:
        a row whose C body branches"""
        here = self.cur
        yes_block, no_block, join = self.new_block(), self.new_block(), self.new_block()
        result = self.value(t)
        join.params.append(result)
        here.term = ("br", test, yes_block.id, no_block.id)
        for block, fn in ((yes_block, yes), (no_block, no)):
            self.cur = block
            v = fn()
            self.cur.term = ("jmp", join.id, (v,))
        self.cur = join
        return result

    def and_then(self, test: int, then: Callable[[], int]) -> int:
        "C's `test && then()`"
        return self.choose(test, "bool", then, lambda: self.const("bool", 0))

    def zext(self, t: str, v: int) -> int:
        return self.emit1("conv", t, (v,))

    def cache(self, entry: Rep, field: str) -> int:
        """A field of the word's feedback entry, read when the template runs:
        the entry can be rewritten while the word keeps its handler"""
        if not isinstance(entry, Entry):
            self.refuse(f"{entry} is not a feedback entry")
        contracts = [("it", self.arm.contract)]
        if self.lowering is not None:
            contracts.append((self.lowering.name, self.lowering.contract))
        for who, contract in contracts:
            declared = {a.field for a in contract.feedback if a.mode is AccessMode.read}
            if f"entry.{field}" not in declared and "entry" not in declared:
                self.refuse(f"reads entry.{field}, which {who} does not declare")
        layout = layout_of(self.arm, entry)
        union = layout.union_of(field)
        if union is not None and any(f in self.fields_read for f in union.fields
                                     if f != field):
            self.refuse(f"reads entry.{field} and another field of its union")
        self.fields_read.add(field)
        f = layout.fields.get(field)
        if f is None or f.member is None:
            self.refuse(f"entry.{field} of {entry.schema.value} has no C member")
        t = ctype_leaf(f.ctype)
        if t is None:
            self.refuse(f"entry.{field} has no leaf type")
        self.entry = layout.struct
        return self.emit1("cache", t, (), f.member)

    # Blocks

    def order(self) -> list[str]:
        """The expansion's blocks: those the arm writes, in its written
        order, then the blocks of the macros it expands"""
        written = [b.label for b in self.arm.graph.blocks]
        labels = list(self.ssa.blocks)
        return sorted(labels, key=lambda l: (written.index(l) if l in written
                                             else len(written) + labels.index(l)))

    def param(self, t: Type) -> Rep:
        lt = leaf_type(t)
        if lt is not None:
            v = self.value(lt)
            self.cur.params.append(v)
            return v
        if isinstance(t, SchemaType) and t.ctor is SchemaCtor.Recipe:
            fields = []
            for name, ft in record_fields(t.schema):
                v = self.value(ft)
                self.cur.params.append(v)
                fields.append((name, v))
            return Record(t.schema, tuple(fields))
        if isinstance(t, HandleType) and t.handle in (Handle.Err, Handle.Route):
            return None
        if isinstance(t, ScalarType) and t.scalar is Scalar.Unit:
            return None
        self.refuse(f"a block parameter of type {t}")

    def build(self) -> ArmLeaf:
        order = self.order()
        for label in order:
            self.ids[label] = self.new_block().id
        for label in order:
            self.cur = self.block_list[self.ids[label]]
            for p in self.ssa.blocks[label].params:
                self.env[p] = self.param(self.ssa.types[p])
        after = blocks_after_acquire(self.ssa)
        for label in order:
            b = self.ssa.blocks[label]
            self.cur = self.block_list[self.ids[label]]
            self.steps = []
            self.holding = label in after
            for op in b.ops:
                self.op(op)
            self.term(b.term)
        outputs = tuple(s.home for s in self.plan.steps if s.op == "store") if self.plan else ()
        body = rop_primitives.LeafBody(
            self.arm.name, self.block_list, self.types, tuple(self.inputs), outputs,
            frozenset(self.effects), tuple(self.helpers))
        return ArmLeaf(body, self.plan, tuple(self.inputs), self.inline(),
                       tuple(self.acquired), self.entry)

    def inline(self) -> frozenset[str]:
        """The homes whose value the arm's facts show an inline int on every
        path that publishes (rop_cgen's analysis, as the interpreter's C)"""
        flat = flatten(self.program, self.arm)
        facts = arm_facts(self.program, flat)
        views = home_views(flat)
        publishes = [label for label, b in flat.blocks.items()
                     if any(isinstance(s, Publish) for s in b.stmts)]
        return frozenset(home for home, view in views.items()
                         if all(facts[label].has(view, Fact.inline_int)
                                for label in publishes if label in facts))

    # Ops

    def op(self, op: Op) -> None:
        k = op.kind
        args = [self.env.get(a) for a in op.args]
        if self.steps and k not in (OpKind.take, OpKind.old, OpKind.store,
                                    OpKind.checkpoint, OpKind.release, OpKind.operand):
            self.refuse(f"{k.value} inside a publish")
        if k is OpKind.read:
            if self.ssa.operands[op.attr] is not FieldKind.home:
                self.refuse(f"a read of the span {op.attr}")
            if op.attr not in self.inputs:
                self.inputs.append(op.attr)
            self.env[op.result] = self.emit1("arg", rop_primitives.VALUE, (), op.attr)
        elif k is OpKind.operand:
            self.env[op.result] = self.operand(op.attr)
        elif k is OpKind.env:
            self.env[op.result] = Opaque(op.attr)
        elif k is OpKind.const:
            t = self.ssa.types[op.result]
            lt = leaf_type(t)
            if lt is None or isinstance(t, EnumType):
                self.refuse(f"a constant of type {t}")
            self.env[op.result] = self.const(lt, int(op.attr))
        elif k in (OpKind.prim, OpKind.call):
            results = self.row(op.attr, args)
            self.env[op.result] = results[0] if results else None
        elif k is OpKind.dup:
            self.env[op.result] = self.prim1("PyValue_DUP", self.leaf(args[0]))
        elif k is OpKind.acquire:
            if op.attr not in self.inputs:
                self.inputs.append(op.attr)
            self.acquired.append(op.attr)
            self.holding = True
            self.env[op.result] = self.emit1("arg", rop_primitives.VALUE, (), op.attr)
        elif k is OpKind.unwrap:
            self.env[op.result] = args[0]
        elif k is OpKind.guard:
            if self.holding:
                self.refuse("a guard after an acquire")
            bad = self.emit1("lnot", "bool", (self.leaf(args[0]),))
            self.emit("exit_if", (), (bad,), EXIT)
        elif k in (OpKind.take, OpKind.old):
            self.steps.append(Step(k.value, op.attr))
            self.owners[op.result] = op.attr
        elif k is OpKind.store:
            self.steps.append(Step("store", op.attr))
            self.emit("out", (), (self.leaf(args[0]),), op.attr)
        elif k is OpKind.checkpoint:
            pass
        elif k is OpKind.release:
            if op.args[0] not in self.owners:
                self.refuse(f"a release of {op.attr.value} that no take or old gives")
            self.steps.append(Step("release", self.owners[op.args[0]], op.attr))
        else:
            self.refuse(f"{k.value} has no leaf lowering")

    def operand(self, name: str) -> Rep:
        kind = self.ssa.operands[name]
        param = next(p for p in self.program.families[self.arm.family].params
                     if p.name == name)
        if kind in (FieldKind.imm, FieldKind.const):
            t = "u32" if isinstance(param.type, EnumType) else leaf_type(param.type)
            for p in self.program.encodings[self.arm.family].placements:
                if (p.operand == name and p.slots and t is not None
                        and all(s.word == 0 for s in p.slots)):
                    return Imm(immediate_expr(p), t)
            self.refuse(f"the immediate {name} is not in the head")
        if kind is FieldKind.feedback:
            return Entry(param.type.schema)
        if kind in (FieldKind.name, FieldKind.target):
            return Opaque(kind.value)
        self.refuse(f"operand {name} ({kind.value})")

    def row(self, name: str, args: list[Rep]) -> tuple[Rep, ...]:
        """The C result of a row as leaf values: its primitive, its
        lowering, or a call of its helper"""
        x = self.program.externs.get(name)
        if x is None or x.c is None:
            self.refuse(f"{name} has no C body")
        symbol = x.c.symbol
        if symbol in ROW_LOWERINGS:
            self.lowering = x
            try:
                return ROW_LOWERINGS[symbol](self, *args)
            finally:
                self.lowering = None
        if symbol in rop_primitives.PRIMITIVES:
            return self.prim(symbol, *(self.leaf(a) for a in args))
        if symbol in rop_primitives.HELPERS:
            h = rop_primitives.HELPERS[symbol]
            self.helpers.append(symbol)
            self.effects |= set(h.effects)
            return self.emit("call", h.results, tuple(self.leaf(a) for a in args), symbol)
        self.refuse(f"{name} has no leaf lowering")

    # Terminators

    def args_of(self, label: str, reps: list[Rep]) -> list[int]:
        "The leaf arguments of a jump to label with these values"
        out = []
        for p, rep in zip(self.ssa.blocks[label].params, reps):
            if leaf_type(self.ssa.types[p]) is not None:
                out.append(self.leaf(rep))
            elif isinstance(rep, Record):
                out.extend(v for _, v in rep.fields)
        return out

    def jump(self, target: Jump, payload: list[Rep] = ()):
        reps = list(payload) + [self.env.get(a) for a in target.args]
        return ("jmp", self.ids[target.block], tuple(self.args_of(target.block, reps)))

    def edge_block(self, target: Jump, payload: list[Rep] = ()) -> int:
        """A block that jumps to target, so that a branch edge can carry
        arguments"""
        here = self.cur
        edge = self.new_block()
        self.cur = edge
        edge.term = self.jump(target, payload)
        self.cur = here
        return edge.id

    def term(self, t) -> None:
        if self.steps and not isinstance(t, ExitTerm):
            self.refuse("a publish that does not end its block")
        if isinstance(t, Jump):
            self.cur.term = self.jump(t)
        elif isinstance(t, BranchTerm):
            cond = self.leaf(self.env[t.cond])
            self.cur.term = ("br", cond, self.edge_block(t.if_true),
                             self.edge_block(t.if_false))
        elif isinstance(t, Switch):
            self.switch(t)
        else:
            self.exit(t)

    def exit(self, t: ExitTerm) -> None:
        if t.kind in ("next", "branch"):
            plan = PublishPlan(tuple(self.steps))
            if self.plan is not None and self.plan != plan:
                self.refuse("two different publications")
            self.plan = plan
            self.cur.term = ("next", None) if t.kind == "next" else ("branch",)
        elif self.steps:
            self.refuse(f"exit {t.kind} after a publish")
        elif self.holding:
            self.refuse(f"exit {t.kind} after an acquire")
        elif t.kind == "raise":
            if any(h != INTACT for _, h in t.state.homes):
                self.refuse("a raise whose point changes a home")
            self.cur.term = ("raise",)
        elif t.kind == "jump":
            self.cur.term = ("jump",)
        elif t.kind == "miss":
            self.cur.term = ("exit", EXIT, None)
        else:
            self.refuse(f"exit {t.kind} has no leaf lowering")

    def raise_block(self, target: Jump) -> bool:
        "Whether target is a block that only raises, with every home intact"
        b = self.ssa.blocks[target.block]
        return (not b.ops and isinstance(b.term, ExitTerm) and b.term.kind == "raise"
                and all(h == INTACT for _, h in b.term.state.homes))

    def switch(self, t: Switch) -> None:
        x = self.program.externs[t.row]
        if x.c is None:
            self.refuse(f"{t.row} has no C body")
        if t.transfer:
            self.refuse(f"the transfer row {t.row}")
        args = [self.env.get(a) for a in t.args]
        edges = {e.case: e for e in t.edges}
        if x.c.symbol in SWITCH_LOWERINGS:
            SWITCH_LOWERINGS[x.c.symbol](self, args, edges)
            return
        conv = CONVENTIONS[x.c.convention]
        if len(conv.outcomes) != 2:
            self.refuse(f"{t.row}: the convention {conv.name} has no leaf lowering")
        results = self.row(t.row, args)
        cases = [c.name for c in x.result.cases]
        outs = [(oc, edges[oc.case or cases[i]]) for i, oc in enumerate(conv.outcomes)]
        payloads = [self.payload(oc, results) for oc, _ in outs]
        raised = [i for i, (oc, _) in enumerate(outs) if oc.raised]
        if raised:
            (i,) = raised
            if not self.raise_block(outs[i][1].target):
                self.refuse(f"the error edge of {t.row} is not a raise")
            test, negated = self.outcome_test(outs[i][0], results)
            test = self.leaf(test)
            if negated:
                test = self.emit1("lnot", "bool", (test,))
            self.emit("raise_if", (), (test,))
            self.cur.term = self.jump(outs[1 - i][1].target, payloads[1 - i])
            return
        test, negated = self.outcome_test(outs[0][0], results)
        yes, no = (1, 0) if negated else (0, 1)
        self.branch(test, (outs[yes][1].target, payloads[yes]),
                    (outs[no][1].target, payloads[no]))

    def branch(self, test: Rep, yes, no) -> None:
        "A branch to yes when the test holds, each edge in a block of its own"
        if not isinstance(test, Conj):
            self.cur.term = ("br", self.leaf(test), self.edge_block(*yes),
                             self.edge_block(*no))
            return
        fail = self.edge_block(*no)
        *first, last = test.tests
        for t in first:
            cont = self.new_block()
            self.cur.term = ("br", t, cont.id, fail)
            self.cur = cont
        self.cur.term = ("br", last, self.edge_block(*yes), fail)

    def outcome_test(self, oc, results) -> tuple[Rep, bool]:
        """The leaf test of one outcome of a C convention, and whether the
        outcome is the test's false side"""
        r = results[0]
        match oc.test:
            case CTest.true:
                return r, False
            case CTest.false:
                return r, True
            case CTest.null:
                return self.prim1("PyValue_IsNull", self.leaf(r)), False
            case CTest.nonnull:
                return self.prim1("PyValue_IsNull", self.leaf(r)), True
        pred = {CTest.zero: ("eq", False), CTest.nonzero: ("eq", True),
                CTest.negative: ("slt", False), CTest.nonnegative: ("slt", True),
                CTest.positive: ("sgt", False)}.get(oc.test)
        if pred is None:
            self.refuse(f"the outcome test {oc.test.value}")
        v = self.leaf(r)
        return self.cmp(pred[0], v, self.const(self.types[v], 0)), pred[1]

    def payload(self, oc, results) -> list[Rep]:
        out = []
        for src in oc.payload:
            if src is CSource.result:
                out.append(results[0])
            elif src is CSource.out:
                out.append(results[1])
            elif src in (CSource.exception, CSource.token):
                out.append(None)
            else:
                self.refuse(f"the payload source {src.value}")
        return out


# The rows' leaf lowerings, by C symbol: the leaf IR of each row's C body
# (Include/internal/pycore_rop_rows.h, pycore_attr_recipe.h).  A lowering
# returns the C result, and for an out-parameter convention the value
# written through it.

LoweringFn = Callable[..., tuple[Rep, ...]]


def _imm(b: LeafBuilder, rep: Rep) -> str:
    if not isinstance(rep, Imm):
        b.refuse(f"{rep} is not an immediate of the word")
    return rep.expr


def _branch_drops(b, mode):
    return (b.emit1("jconst", "bool", (), f"({_imm(b, mode)}) < 2"),)


def _branch_taken(b, mode, hit):
    return (b.emit1("xorb", "bool", (b.leaf(hit),), f"!(({_imm(b, mode)}) & 1)"),)


def _same_bool(b, sense, hit):
    return (b.emit1("xorb", "bool", (b.leaf(hit),), f"!({_imm(b, sense)})"),)


def _borrow_present(b, m):
    "_PyValue_BorrowPresent"
    return (b.emit1("lnot", "bool", (b.prim1("PyValue_IsNull", b.leaf(m)),)),)


def _recipe(b, schema: Schema, **values) -> Record:
    "A recipe; the fields the admission row leaves unset are zero"
    fields = []
    for name, t in record_fields(schema):
        v = values.get(name)
        if v is None:
            v = b.const(t, "PyValue_NULL" if t == rop_primitives.VALUE else 0)
        fields.append((name, v))
    return Record(schema, tuple(fields))


def _attr_recipe(b, **values) -> Record:
    return _recipe(b, Schema.Attr, **values)


def _null(b) -> int:
    return b.const(rop_primitives.VALUE, "PyValue_NULL")


def _admitted_recipe(b, fb, name):
    """_PyAttr_AdmittedRecipe: an entry that holds a type version or a class
    or keys version (keys_version's storage, which C reads as class_version)"""
    tv = b.cache(fb, "type_version")
    kv = b.cache(fb, "keys_version")
    hit = b.cmp("ne", b.emit1("or", "u32", (tv, kv)), b.const("u32", 0))
    return hit, _attr_recipe(
        b, type_version=tv, aux=kv, offset=b.cache(fb, "offset"),
        read_kind=b.cache(fb, "kind"), class_value=b.cache(fb, "class_value"))


def _admitted_method(b, fb, name):
    "_PyAttr_AdmittedMethod: a live type version and the method it caches"
    tv = b.cache(fb, "type_version")
    hit = b.cmp("ne", tv, b.const("u32", 0))
    return hit, _attr_recipe(b, type_version=tv, values_layout=b.const("bool", 1),
                             class_value=b.cache(fb, "class_value"))


def _borrow_method(b, r):
    "_PyAttr_BorrowMethod"
    return (b.prim1("_PyValueGuarded_Load", r["class_value"]),)


def _is_module(b, o):
    "_PyAttr_IsModule"
    return (b.prim1("PyModule_CheckExact", b.leaf(o)),)


def _keys_entry(b, keys: int, index: int) -> int:
    "The value of a unicode keys object's entry at index (u32), NULL past the entries"
    index = b.zext("i64", index)

    def entry():
        entries = b.prim1("DK_UNICODE_ENTRIES", keys)
        e = b.emit1("ptradd", "PyDictUnicodeEntry*", (entries, index), "PyDictUnicodeEntry")
        return b.load("PyDictUnicodeEntry.me_value", e)
    in_range = b.cmp("ult", index, b.load("PyDictKeysObject.dk_nentries", keys))
    return b.choose(in_range, rop_primitives.VALUE, entry, lambda: _null(b))


def _dict_keys(b, d: int) -> int:
    return b.load("PyDictObject.ma_keys", d)


def _keys_current(b, keys: int, version: int) -> int:
    return b.cmp("eq", b.load("PyDictKeysObject.dk_version", keys), version)


def _module_entry(b, o, r):
    """_PyAttr_ModuleEntry: the module's entry at the recipe's index while
    its keys have the recipe's version, else NULL"""
    keys = _dict_keys(b, b.load("PyModuleObject.md_dict", b.leaf(o)))
    return (b.choose(_keys_current(b, keys, r["aux"]), rop_primitives.VALUE,
                     lambda: _keys_entry(b, keys, r["offset"]), lambda: _null(b)),)


def _admitted_global_module(b, fb):
    "_PyGlobal_AdmittedModule: a globals keys version"
    gv = b.cache(fb, "globals_version")
    hit = b.cmp("ne", gv, b.const("u32", 0))
    return hit, _recipe(b, Schema.Global, globals_version=gv, index=b.cache(fb, "index"))


def _admitted_global_builtin(b, fb):
    "_PyGlobal_AdmittedBuiltin: both keys versions"
    gv = b.cache(fb, "globals_version")
    bv = b.cache(fb, "builtins_version")
    hit = Conj((b.cmp("ne", gv, b.const("u32", 0)), b.cmp("ne", bv, b.const("u32", 0))))
    return hit, _recipe(b, Schema.Global, globals_version=gv, builtins_version=bv,
                        index=b.cache(fb, "index"))


def _globals_current(b, frame, r):
    "_PyGlobal_GlobalsCurrent"
    return (_keys_current(b, _dict_keys(b, b.prim1("GLOBALS")), r["globals_version"]),)


def _builtins_current(b, frame, r):
    "_PyGlobal_BuiltinsCurrent: an exact dict whose keys have the recipe's version"
    builtins = b.prim1("BUILTINS")
    return (b.and_then(b.prim1("PyDict_CheckExact", builtins), lambda: _keys_current(
        b, _dict_keys(b, builtins), r["builtins_version"])),)


def _globals_slot(b, frame, r):
    "_PyGlobal_GlobalsSlot"
    return (_keys_entry(b, _dict_keys(b, b.prim1("GLOBALS")), r["index"]),)


def _builtins_slot(b, frame, r):
    "_PyGlobal_BuiltinsSlot"
    return (_keys_entry(b, _dict_keys(b, b.prim1("BUILTINS")), r["index"]),)


def _has_type_flag(b, tp: int, flag: str) -> int:
    "PyType_HasFeature(tp, flag)"
    flags = b.load("PyTypeObject.tp_flags", tp)
    return b.cmp("ne", b.emit1("and", "u64", (flags, b.emit1("jconst", "u64", (), flag))),
                 b.const("u64", 0))


def _class_current(b, o, r):
    """_PyAttr_ClassCurrent: a class with the recipe's version, whose
    metaclass has its type version when the recipe holds one"""
    o = b.leaf(o)

    def metaclass_current():
        tv = r["type_version"]
        return b.choose(
            b.cmp("eq", tv, b.const("u32", 0)), "bool", lambda: b.const("bool", 1),
            lambda: b.cmp("eq", b.load("PyTypeObject.tp_version_tag", b.prim1("Py_TYPE", o)), tv))

    def version_current():
        return b.and_then(b.cmp("eq", b.load("PyTypeObject.tp_version_tag", o), r["aux"]),
                          metaclass_current)
    is_type = _has_type_flag(b, b.prim1("Py_TYPE", o), "Py_TPFLAGS_TYPE_SUBCLASS")
    return (b.and_then(is_type, version_current),)


def _lazy_dict_absent(b, o, r):
    "_PyAttr_LazyDictAbsent: no dictionary yet in the slot at the recipe's offset"
    off = b.emit1("add", "i64", (b.emit1("jconst", "i64", (), "MANAGED_DICT_OFFSET"),
                                 b.zext("i64", r["offset"])))
    slot = b.emit1("ptradd", "PyObject**", (b.leaf(o), off), 1)
    return (b.cmp("eq", b.load("PyObject*", slot), b.const("PyObject*", 0)),)


def _admitted_instance_slot(b, fb, name):
    "_PyAttr_AdmittedInstanceSlot: a live type version"
    tv = b.cache(fb, "type_version")
    hit = b.cmp("ne", tv, b.const("u32", 0))
    return hit, _attr_recipe(b, type_version=tv, offset=b.cache(fb, "offset"))


def _admitted_shape(b, fb, name):
    "_PyAttr_AdmittedShape: a live type version and a shape read (Py_SHAPES)"
    tv = b.cache(fb, "type_version")
    live = b.cmp("ne", tv, b.const("u32", 0))
    shape_read = b.emit1("jconst", "u32", (), "_Py_FB_READ_SHAPE")
    is_shape = b.cmp("eq", b.cache(fb, "kind"), shape_read)
    return Conj((live, is_shape)), _attr_recipe(
        b, type_version=tv, aux=b.cache(fb, "shape"), offset=b.cache(fb, "offset"),
        read_kind=shape_read)


def _instance_version(b, o, r):
    "_PyAttr_InstanceVersion"
    tp = b.prim1("Py_TYPE", b.leaf(o))
    return (b.cmp("eq", b.load("PyTypeObject.tp_version_tag", tp), r["type_version"]),)


def _valid_inline_values(b, o):
    "_PyAttr_ValidInlineValues"
    values = b.prim1("_PyObject_InlineValues", b.leaf(o))
    return (b.cmp("ne", b.load("PyDictValues.valid", values), b.const("u8", 0)),)


def _shape_current(b, o, r):
    "_PyAttr_ShapeCurrent (Py_SHAPES)"
    return (b.cmp("eq", b.prim1("_PyObject_Shape", b.leaf(o)), r["aux"]),)


def _borrow_value_slot(b, o, r):
    "_PyAttr_BorrowValueSlot, _PyAttr_BorrowValueSlotFilled: the word at the offset"
    slot = b.prim1("_PyObject_ValueSlot", b.leaf(o), b.zext("u64", r["offset"]))
    return (b.load(rop_primitives.VALUE, slot),)


def _small_int(b, i):
    "_PyRop_SmallInt"
    return (b.prim1("PyValue_FromInlineInt", b.emit1("jconst", "i64", (), _imm(b, i))),)


def _code_constant(b, frame, i):
    "_PyRop_CodeConstant: an item of the code's constants, with a count"
    consts = b.prim1("FRAME_CO_CONSTS")
    item = b.prim1("PyTuple_GET_VALUE", consts, b.emit1("jconst", "i64", (), _imm(b, i)))
    return (b.prim1("PyValue_DUP", b.prim1("_PyValue_CodeConstant", item)),)


def _exact_int(b, v):
    "_PyRop_ExactInt"
    return (b.prim1("PyValue_LongCheckExact", b.leaf(v)),)


def _int_nonzero(b, v):
    "_PyRop_IntNonzero: an inline int's value, else the boxed int's"
    w = b.leaf(v)
    return (b.choose(
        b.prim1("PyValue_IsInlineInt", w), "bool",
        lambda: b.cmp("ne", b.prim1("PyValue_InlineInt64", w), b.const("i64", 0)),
        lambda: b.emit1("lnot", "bool", (b.prim1(
            "_PyLong_IsZero", b.prim1("PyValue_AsKnownObjectBorrow", w)),))),)


def _exact_list(b, v):
    "_PyRop_ExactList"
    w = b.leaf(v)
    return (b.and_then(b.prim1("PyValue_IsObject", w), lambda: b.prim1(
        "PyList_CheckExact", b.prim1("PyValue_AsKnownObjectBorrow", w))),)


def _exact_str(b, v):
    "_PyRop_ExactStr"
    w = b.leaf(v)
    return (b.and_then(b.prim1("PyValue_IsObject", w), lambda: b.prim1(
        "PyUnicode_CheckExact", b.prim1("PyValue_AsKnownObjectBorrow", w))),)


def _str_nonempty(b, v):
    "_PyRop_StrNonempty"
    o = b.prim1("PyValue_AsKnownObjectBorrow", b.leaf(v))
    return (b.cmp("ne", b.load("PyASCIIObject.length", o), b.const("i64", 0)),)


def _not_none(b, v):
    "_PyRop_NotNone"
    return (b.emit1("lnot", "bool", (b.prim1("PyValue_IsNone", b.leaf(v)),)),)


def _none_or_plain(b, fb, v):
    """_PyRop_NoneOrPlain: None, or an object whose type has the entry's
    version and is a heap type"""
    w = b.leaf(v)

    def plain():
        def versioned():
            tv = b.cache(fb, "type_version")

            def matches():
                tp = b.prim1("Py_TYPE", b.prim1("PyValue_AsKnownObjectBorrow", w))
                return b.and_then(b.cmp("eq", b.load("PyTypeObject.tp_version_tag", tp), tv),
                                  lambda: _has_type_flag(b, tp, "Py_TPFLAGS_HEAPTYPE"))
            return b.and_then(b.cmp("ne", tv, b.const("u32", 0)), matches)
        return b.and_then(b.prim1("PyValue_IsObject", w), versioned)
    return (b.choose(b.prim1("PyValue_IsNone", w), "bool", lambda: b.const("bool", 1), plain),)


def _list_nonempty(b, v):
    "_PyRop_ListNonempty"
    size = b.prim1("PyList_GET_SIZE", b.prim1("PyValue_AsKnownObjectBorrow", b.leaf(v)))
    return (b.cmp("ne", size, b.const("i64", 0)),)


def _truth_object(b, test, invert):
    "_PyRop_TruthObject: True when the test differs from invert"
    if isinstance(invert, Imm):
        differs = b.emit1("xorb", "bool", (b.leaf(test),), invert.expr)
    else:
        differs = b.emit1("xor", "bool", (b.leaf(test), b.leaf(invert)))
    return (b.emit1("select", rop_primitives.VALUE, (differs, b.const(rop_primitives.VALUE, "PyValue_True"),
                                             b.const(rop_primitives.VALUE, "PyValue_False"))),)


ROW_LOWERINGS: dict[str, LoweringFn] = {
    # A branch's tests of its mode or sense: functions of the word, or a
    # test result negated or not by the word
    "_PyRop_BranchDrops": _branch_drops,
    "_PyRop_BranchTaken": _branch_taken,
    "_PyRop_SameBool": _same_bool,
    "_PyValue_BorrowPresent": _borrow_present,
    # Truth
    "_PyRop_ExactInt": _exact_int,
    "_PyRop_IntNonzero": _int_nonzero,
    "_PyRop_ExactList": _exact_list,
    "_PyRop_ListNonempty": _list_nonempty,
    "_PyRop_ExactStr": _exact_str,
    "_PyRop_NoneOrPlain": _none_or_plain,
    "_PyRop_NotNone": _not_none,
    "_PyRop_StrNonempty": _str_nonempty,
    "_PyRop_TruthObject": _truth_object,
    # Constants
    "_PyRop_SmallInt": _small_int,
    "_PyRop_CodeConstant": _code_constant,
    # Attribute recipes
    "_PyAttr_AdmittedInstanceSlot": _admitted_instance_slot,
    "_PyAttr_AdmittedShape": _admitted_shape,
    "_PyAttr_AdmittedRecipe": _admitted_recipe,
    "_PyAttr_AdmittedMethod": _admitted_method,
    "_PyAttr_BorrowMethod": _borrow_method,
    "_PyAttr_IsModule": _is_module,
    "_PyAttr_ClassCurrent": _class_current,
    # Global recipes
    "_PyGlobal_AdmittedModule": _admitted_global_module,
    "_PyGlobal_AdmittedBuiltin": _admitted_global_builtin,
    "_PyGlobal_GlobalsCurrent": _globals_current,
    "_PyGlobal_BuiltinsCurrent": _builtins_current,
    "_PyGlobal_GlobalsSlot": _globals_slot,
    "_PyGlobal_BuiltinsSlot": _builtins_slot,
    "_PyAttr_LazyDictAbsent": _lazy_dict_absent,
    "_PyAttr_ModuleEntry": _module_entry,
    "_PyAttr_InstanceVersion": _instance_version,
    "_PyAttr_ValidInlineValues": _valid_inline_values,
    "_PyAttr_ShapeCurrent": _shape_current,
    "_PyAttr_BorrowValueSlot": _borrow_value_slot,
    "_PyAttr_BorrowValueSlotFilled": _borrow_value_slot,
}


def _poll(b: LeafBuilder, args, edges) -> None:
    """_PyRop_Poll: a load and a test of the eval breaker, and only when an
    event is pending the call that handles it"""
    if not b.raise_block(edges["err"].target):
        b.refuse("the error edge of poll is not a raise")
    b.helpers.append("_Py_HandlePending")
    b.effects |= set(rop_primitives.HELPERS["_Py_HandlePending"].effects)
    events = b.prim1("_Py_EvalBreakerHasEvents")
    here = b.cur
    handle = b.new_block()
    quiet = b.edge_block(edges["ok"].target)
    here.term = ("br", events, handle.id, quiet)
    b.cur = handle
    status = b.emit1("call", "i32", (), "_Py_HandlePending")
    failed = b.cmp("ne", status, b.const("i32", 0))
    b.emit("raise_if", (), (failed,))
    b.cur.term = b.jump(edges["ok"].target)


# Rows whose lowering is control flow of its own: it ends the block
SWITCH_LOWERINGS: dict[str, Callable[[LeafBuilder, list, dict], None]] = {
    "_PyRop_Poll": _poll,
}


def leaf_of_arm(program: Program, arm: Arm, ssa: SsaArm | None = None) -> ArmLeaf:
    return LeafBuilder(program, arm, ssa or expand(program, arm)).build()


# The template: the leaf body joined with its publication, as
# jit_template_generator.Builder joins the uops of a stack handler

@dataclass(frozen=True)
class Home:
    """What Builder reads of an operand: its field letter and what it may
    name (any of a temporary, a local)"""
    field: str
    may_name: frozenset[str] = frozenset({"TEMP", "LOCAL"})


class TemplateBuilder:
    def __init__(self, program: Program, arm: Arm, leaf: ArmLeaf) -> None:
        self.program = program
        self.arm = arm
        self.leaf = leaf
        self.g = jt.Graph()
        self.raises: jt.TBlock | None = None
        self.deopt: jt.TBlock | None = None
        self.homes: dict[str, Home] = {}
        enc = program.encodings[arm.family]
        for p in enc.placements:
            if p.kind is FieldKind.home and p.slots[0].word == 0:
                self.homes[p.operand] = Home(p.slots[0].name)

    def exit_block(self, kind: str) -> int:
        if kind == "raise":
            if self.raises is None:
                self.raises = self.g.new_block()
                self.raises.term = ("raise",)
            return self.raises.id
        if self.deopt is None:
            self.deopt = self.g.new_block()
            self.deopt.term = ("deopt",)
        return self.deopt.id

    def build(self) -> jt.Graph:
        g, body = self.g, self.leaf.body
        entry = g.blocks[g.entry]
        views: dict[str, int] = {}
        for name in self.leaf.inputs:
            if name not in self.homes:
                raise jt.Refused(f"{name} is not a home of the head")
            views[name] = g.value(rop_primitives.VALUE)
            entry.ops.append(jt.TOp("view", [views[name]], [], self.homes[name].field))
        lmap: dict[int, int] = {}
        bmap = {}
        for b in body.blocks:
            gb = g.new_block([body.types[p] for p in b.params])
            bmap[b.id] = gb
            for p, gp in zip(b.params, gb.params):
                lmap[p] = gp
        entry.term = ("jmp", bmap[0].id, [])
        outs = self.leaf.body.outputs
        # A word that can branch ends on the flag that says whether it did
        branches = any(b.term[0] == "branch" for b in body.blocks)
        if any(b.term[0] == "jump" for b in body.blocks) and self.leaf.plan is not None:
            raise jt.Refused("a jump after a publication")
        merge = g.new_block([rop_primitives.VALUE for _ in outs]
                            + (["bool"] if branches else []))
        for b in body.blocks:
            gb = bmap[b.id]
            published: dict[str, int] = {}
            for op in b.ops:
                gb = self.op(op, gb, lmap, views, published)
            term = b.term
            if term[0] == "br" and self.leaves_by_exit(gb, lmap[term[1]]):
                # The primitive branches to the overflow case itself
                gb.ops[-1].exit = bmap[term[2]].id
                gb.ops[-1].results = gb.ops[-1].results[1:]
                gb.term = ("jmp", bmap[term[3]].id, [])
            elif term[0] == "br":
                gb.term = ("br", lmap[term[1]], bmap[term[2]].id, bmap[term[3]].id)
            elif term[0] == "jmp":
                gb.term = ("jmp", bmap[term[1]].id, [lmap[a] for a in term[2]])
            elif term[0] in ("next", "branch"):
                args = [published[o] for o in outs]
                if branches:
                    flag = g.value("bool")
                    gb.ops.append(jt.TOp("const", [flag], [], int(term[0] == "branch")))
                    args.append(flag)
                gb.term = ("jmp", merge.id, args)
            elif term[0] == "jump":
                gb.term = ("end", None, True)
            elif term[0] == "raise":
                gb.term = ("jmp", self.exit_block("raise"), [])
            else:
                gb.term = ("jmp", self.exit_block("deopt"), [])
        if self.leaf.plan is None and not branches and all(
                b.term[0] != "next" for b in body.blocks):
            del g.blocks[merge.id]
        else:
            self.epilogue(merge, views, branches)
        return g

    @staticmethod
    def leaves_by_exit(gb, cond: int) -> bool:
        """Whether the block's last op is the checked add whose bool is cond:
        its lowering leaves for the overflow case by a branch of its own"""
        last = gb.ops[-1] if gb.ops else None
        return (last is not None and last.kind == "prim"
                and last.attr == "_PyValue_AddCheckedScaled" and last.results[0] == cond)

    def op(self, op, gb, lmap, views, published):
        g = self.g
        k = op.kind
        args = [lmap[a] for a in op.args]
        if k == "arg":
            lmap[op.results[0]] = views[op.attr]
            return gb
        results = []
        for res in op.results:
            results.append(g.value(self.leaf.body.types[res]))
            lmap[res] = results[-1]
        if k == "out":
            published[op.attr] = args[0]
            return gb
        if k in ("exit_if", "raise_if"):
            cont = g.new_block()
            gb.term = ("br", args[0], self.exit_block("raise" if k == "raise_if" else "deopt"),
                       cont.id)
            return cont
        if k == "prim" and op.attr not in jt.LOWERINGS:
            raise jt.Refused(f"no lowering of {op.attr}")
        gb.ops.append(jt.TOp(k, results, args, op.attr))
        return gb

    def epilogue(self, merge: jt.TBlock, views: dict[str, int],
                 branches: bool = False) -> None:
        """The publication by home class: move the acquired owners, detach
        the spent temporaries (in operand order: a detach is a store of NULL,
        which nothing observes), load the displaced owners of locals, store,
        then close in the declared retire order"""
        plan = self.leaf.plan
        if plan is None:
            raise jt.Refused("an arm that publishes nothing")
        g = self.g
        order = list(self.homes)
        for name in self.leaf.acquired:
            # The owner moves out of a temporary; a local keeps its own, and
            # the arm takes a count of its own
            home = self.homes[name]
            merge.ops.append(jt.TOp("detach", [], [], home.field, jt.temp_test(home)))
            merge.ops.append(jt.TOp("take", [], [views[name]], None, jt.local_test(home)))
        owners: dict[str, int] = {}
        taken = sorted((s.home for s in plan.steps if s.op == "take"), key=order.index)
        for name in taken:
            home = self.homes[name]
            if name not in self.leaf.inline:
                # A temporary's owner closes after the stores; one the body
                # never read is loaded before its home is detached
                v = views.get(name)
                if v is None:
                    v = g.value(rop_primitives.VALUE)
                    merge.ops.append(jt.TOp("slot_load", [v], [], home.field,
                                            jt.temp_test(home)))
                owners[name] = v
            merge.ops.append(jt.TOp("detach", [], [], home.field, jt.temp_test(home)))
        olds: dict[str, int] = {}
        stores = [s.home for s in plan.steps if s.op == "store"]
        for s in plan.steps:
            if s.op == "old":
                dst = self.homes[s.home]
                olds[s.home] = g.value(rop_primitives.VALUE)
                merge.ops.append(jt.TOp("slot_load", [olds[s.home]], [], dst.field,
                                        jt.local_test(dst)))
            elif s.op == "store":
                merge.ops.append(jt.TOp("slot_store", [], [merge.params[stores.index(s.home)]],
                                        self.homes[s.home].field))
        for s in plan.steps:
            if s.op != "release":
                continue
            home = self.homes[s.home]
            if s.kind is DebtKind.spent and s.home in owners:
                merge.ops.append(jt.TOp("prim", [], [owners[s.home]], "PyValue_XCLOSE",
                                        jt.temp_test(home)))
            elif s.kind is DebtKind.old:
                merge.ops.append(jt.TOp("prim", [], [olds[s.home]], "PyValue_XCLOSE",
                                        jt.local_test(home)))
        merge.term = ("end", merge.params[-1] if branches else None)


def entry_decl(program: Program, arm: Arm, leaf: ArmLeaf) -> "jt.FbEntry | None":
    "The feedback entry that the template reads, from the family's fb operand"
    if leaf.entry is None:
        return None
    for p in program.encodings[arm.family].placements:
        if p.kind is not FieldKind.feedback or len(p.slots) != 1:
            continue
        slot = p.slots[0]
        if slot.word == 0:
            return jt.FbEntry(leaf.entry, f"IR_{slot.name}(ir_word)")
        # A field of an operand word, read when the template is emitted
        return jt.FbEntry(leaf.entry,
                          f"((uint32_t)(cx->words[cx->index + {slot.word}] >> "
                          f"{slot.offset - 64 * slot.word}) & {(1 << slot.width) - 1:#x})")
    raise Unsupported(f"{arm.name}: the feedback operand is not one field")


def template_graph(program: Program, arm: Arm, leaf: ArmLeaf) -> jt.Graph:
    g = TemplateBuilder(program, arm, leaf).build()
    jt.simplify(g)
    return g


def template_of_arm(program: Program, arm: Arm, name: str,
                    leaf: ArmLeaf | None = None) -> list[str]:
    """The lines of the arm's `jit_template_NAME`"""
    leaf = leaf or leaf_of_arm(program, arm)
    g = template_graph(program, arm, leaf)
    return jt.Emitter(name, g, entry_decl(program, arm, leaf)).emit()
