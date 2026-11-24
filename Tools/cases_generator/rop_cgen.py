"""Interpreter C of register instruction arms (F2).

`lower_arm` turns one checked arm graph into the statements of a handler
body.  The same text is the computed-goto case of ceval.c's `ir_evaluate`
and the body of the arm's function form: the transfer macros (`DISPATCH`,
`PROCESS_ERROR_AT`, `JUMP_TO_PREDICTED`) differ, the statements do not.

Operands are decoded from the family's derived encoding, a call lowers
through its extern's C convention (rop_catalog.CONVENTIONS, as data),
`publish` follows each source's local flag, and each exit becomes dispatch,
the error path with its point index, or the family's miss adapter.  The
generated C uses the value API and never `.bits`.
"""

from collections.abc import Mapping
from dataclasses import dataclass, field

from rop_catalog import (Schema, SchemaCtor, CONVENTIONS, CReturn, CSource, CTest, DebtKind, Effect, Fact,
                         FieldKind,
                         Handle, Scalar, Slot, Wrap)
from rop_check import GraphInfo, Program
from rop_model import (SchemaType, Advance, Arm, BoolLit, Branch, Call, CallTerm, Cases,
                       Each, Exit, Expr, Extern, Goto, Guard, HandleType, Let,
                       Name, Nat, Placement, Publish, ScalarType, Target, Type,
                       WrapType)


class Unsupported(Exception):
    """A construct whose lowering a later unit supplies; says which."""


SCALAR_C = {
    Scalar.Bool: "bool", Scalar.U8: "uint8_t", Scalar.U16: "uint16_t",
    Scalar.U32: "uint32_t", Scalar.U64: "uint64_t", Scalar.I32: "int32_t",
    Scalar.I64: "int64_t", Scalar.F64: "double", Scalar.I51: "int64_t",
    Scalar.Scaled51: "int64_t", Scalar.Size: "size_t",
}

# Handles that are a C variable of their own; the rest are tokens that the
# generated C does not carry (an `Err` is the pending exception itself).
HANDLE_C = {
    Handle.B: "PyValueBorrow",
    Handle.O: "PyValue",
    Handle.Obj: "PyObject *",
}

# Effects after which Python may have run: a stub reports ESCAPED
ESCAPING = frozenset({Effect.runs_python, Effect.polls})

# Names the handler's C scope owns
RESERVED = frozenset({
    "frame", "tstate", "next_instr", "ir_word", "opcode", "this_instr",
    "ir_old",
})


RETURN_C = {
    CReturn.bool: "bool", CReturn.scalar: "int64_t", CReturn.value: "PyValue",
    CReturn.borrow: "PyValueBorrow", CReturn.pointer: "void *",
    CReturn.int_status: "int", CReturn.status_value: "_PyStatusValue",
}


def test_text(test: CTest, returns: CReturn, ret: str) -> str:
    """The C condition under which an outcome's test holds of the result"""
    subject = f"{ret}.status" if returns is CReturn.status_value else ret
    null = f"PyValue_IsNull({ret})" if returns in (CReturn.value, CReturn.borrow) \
        else f"{ret} == NULL"
    return {
        CTest.true: ret, CTest.false: f"!{ret}",
        CTest.nonnull: f"!{null}", CTest.null: null,
        CTest.zero: f"{subject} == 0", CTest.nonzero: f"{subject} != 0",
        CTest.negative: f"{subject} < 0", CTest.positive: f"{subject} > 0",
        CTest.nonnegative: f"{subject} >= 0",
    }[test]


def source_text(source: CSource, ret: str, out: str | None) -> str:
    """The C expression of a payload component; the pending exception is
    not a C value, and its parameter has no variable"""
    match source:
        case CSource.result:
            return ret
        case CSource.value:
            return f"{ret}.value"
        case CSource.nonzero:
            return f"{ret} != 0"
        case CSource.out:
            assert out is not None
            return out
        case CSource.exception:
            return ""


# The C entry type of each feedback schema whose arms are lowered
SCHEMA_C = {Schema.Attr: "_PyFbAttr"}


def c_type(t: Type) -> str | None:
    """The C type of a graph value, None for one that has no C variable"""
    if isinstance(t, SchemaType) and t.ctor is SchemaCtor.Recipe:
        return {Schema.Attr: "_PyAttrRecipe"}.get(t.schema)
    if isinstance(t, ScalarType):
        return SCALAR_C.get(t.scalar)
    if isinstance(t, HandleType):
        return HANDLE_C.get(t.handle)
    if isinstance(t, WrapType) and t.wrap is Wrap.Maybe:
        return c_type(t.arg)
    return None


@dataclass(frozen=True)
class Facts:
    """What is known of each value on entry to a block: its facts, as
    (fact, type argument) pairs."""
    by_name: Mapping[str, frozenset[tuple[Fact, object]]]

    def has(self, name: str, fact: Fact) -> bool:
        return any(f is fact for f, _ in self.by_name.get(name, ()))


def intersect(facts: list[dict[str, frozenset]]) -> dict[str, frozenset]:
    first, *rest = facts
    out = {}
    for name, fs in first.items():
        for other in rest:
            fs = fs & other.get(name, frozenset())
        if fs:
            out[name] = fs
    return out


# Facts known on entry to each block, by a forward pass over the blocks in
# reverse postorder.  A guard grants the facts its row's `test` declares,
# a case grants its row's `ensures`, and a block parameter keeps what every
# incoming edge grants.

def arm_facts(program: Program, arm: Arm) -> dict[str, Facts]:
    info = program.graphs[arm.name]
    blocks = {b.label: b for b in arm.graph.blocks}
    facts_in: dict[str, Facts] = {}
    incoming: dict[str, list[dict[str, frozenset]]] = {b: [] for b in info.order}
    incoming["entry"] = [{}]
    for label in info.order:
        known = intersect(incoming[label]) if incoming[label] else {}
        facts_in[label] = Facts(dict(known))
        block = blocks[label]
        known = dict(known)
        for s in block.stmts:
            if isinstance(s, Guard):
                add_test_facts(program, known, s.cond)
        for target, extra in edges_of(program, blocks, block):
            params = blocks[target.label].params
            out = dict(known)
            for p, a in zip(params[len(params) - len(target.args):], target.args):
                fs = arg_facts(program, a, known)
                if fs:
                    out[p.name] = fs
            out.update(extra)
            incoming[target.label].append(out)
    return facts_in


def arg_facts(program: Program, e: Expr, known: Mapping[str, frozenset]) -> frozenset:
    if isinstance(e, Name):
        return known.get(e.name, frozenset())
    if isinstance(e, Call):
        x = program.externs.get(e.callee)
        if x is not None:
            return frozenset((ef.fact.fact, ef.fact.type_arg)
                             for ef in x.contract.ensures if ef.edge == "result")
    return frozenset()


def add_test_facts(program: Program, known: dict, cond: Expr) -> None:
    if not isinstance(cond, Call):
        return
    x = program.externs.get(cond.callee)
    if x is None:
        return
    for fu in x.test:
        if len(fu.subjects) != 1:
            continue
        (subject,) = fu.subjects
        arg = cond.args[[p.name for p in x.params].index(subject)]
        if isinstance(arg, Name):
            known[arg.name] = known.get(arg.name, frozenset()) | {(fu.fact, fu.type_arg)}


def edges_of(program: Program, blocks, block):
    """(target, facts of the case payload) for each way out of a block"""
    t = block.term
    if isinstance(t, Goto):
        yield t.target, {}
    elif isinstance(t, Branch):
        yield t.if_true, {}
        yield t.if_false, {}
    elif isinstance(t, CallTerm):
        x = program.externs.get(t.callee)
        for edge in t.edges:
            extra: dict[str, frozenset] = {}
            params = blocks[edge.target.label].params
            if x is not None and isinstance(x.result, Cases):
                case = next(c for c in x.result.cases if c.name == edge.case)
                names = [p.name for p in case.params]
                for ef in x.contract.ensures:
                    if ef.edge != case.name or len(ef.fact.subjects) != 1:
                        continue
                    (subject,) = ef.fact.subjects
                    if subject in names:
                        dest = params[names.index(subject)].name
                        extra[dest] = extra.get(dest, frozenset()) | {
                            (ef.fact.fact, ef.fact.type_arg)}
            yield edge.target, extra


def pending_blockers(program: Program, arm: Arm) -> list[str]:
    """Every row without a C body and every macro without a graph that the
    arm reaches, directly or through the macros it calls, in name order"""
    found: set[str] = set()
    seen: set[str] = set()

    def walk(graph) -> None:
        for b in graph.blocks:
            exprs: list[Expr] = []
            for s in b.stmts:
                exprs += [s.cond] if isinstance(s, Guard) else \
                    [s.value] if isinstance(s, Let) else \
                    [x.value for x in s.bindings] if isinstance(s, Publish) else []
            t = b.term
            if isinstance(t, CallTerm):
                visit(t.callee)
                exprs += list(t.args)
            elif isinstance(t, Goto):
                exprs += list(t.target.args)
            elif isinstance(t, Branch):
                exprs.append(t.cond)
            elif isinstance(t, Exit):
                exprs += list(t.args)
            while exprs:
                e = exprs.pop()
                if isinstance(e, Call):
                    visit(e.callee)
                    exprs += list(e.args)

    def visit(name: str) -> None:
        if name in seen:
            return
        seen.add(name)
        x = program.externs.get(name)
        if x is not None and x.c is None:
            found.add(name)
        m = program.macros.get(name)
        if m is not None:
            if m.graph is None:
                found.add(name)
            else:
                walk(m.graph)

    walk(arm.graph)
    return sorted(found)


def home_views(arm: Arm) -> dict[str, str]:
    """The value each `let v = use(x)` binds, by home operand"""
    views: dict[str, str] = {}
    for b in arm.graph.blocks:
        for s in b.stmts:
            if (isinstance(s, Let) and isinstance(s.value, Call)
                    and s.value.callee == "use"):
                (arg,) = s.value.args
                assert isinstance(arg, Name)
                views.setdefault(arg.name, s.name)
    return views


@dataclass
class Lowering:
    """The statements of one arm, with its decoding and labels"""
    arm: str
    words: int = 0           # operand words after the head, which dispatch skips
    decls: list[str] = field(default_factory=list)
    body: list[str] = field(default_factory=list)


class ArmLowerer:
    def __init__(self, program: Program, arm: Arm, numbers: Mapping[str, int],
                 points: Mapping[str, int], generic: Mapping[str, str | None]
                 ) -> None:
        self.program = program
        self.arm = arm
        self.info: GraphInfo = program.graphs[arm.name]
        self.family = program.families[arm.family]
        self.enc = program.encodings[arm.family]
        self.placements: dict[str, Placement] = {
            p.operand: p for p in self.enc.placements}
        self.points = points
        self.generic = generic
        self.prefix = "rop_" + arm.name.replace(".", "_")
        self.blocks = {b.label: b for b in arm.graph.blocks}
        self.lines: list[str] = []
        self.decls: list[str] = []
        self.declared: set[str] = set()
        self.homes: dict[str, str] = {}      # operand -> its home pointer
        self.views: dict[str, str] = {}      # operand -> value bound by use()
        self.temp = 0
        self.facts_in: dict[str, Facts] = {}
        self.targeted: set[str] = set()      # blocks reached by a goto
        self.next_label: str | None = None
        self.acquired: set[str] = set()      # operands whose owner acquire() took

    # Names

    def local(self, name: str) -> str:
        if name in RESERVED:
            raise Unsupported(f"{self.arm.name}: the name {name} is reserved by the handler")
        return name

    def declare(self, name: str, ctype: str, init: str | None = None) -> None:
        if name in self.declared:
            return
        self.declared.add(name)
        self.decls.append(f"{ctype} {name}" + (f" = {init}" if init else "") + ";")

    def new_temp(self, ctype: str, stem: str = "t") -> str:
        self.temp += 1
        name = f"{stem}{self.temp}"
        self.declare(name, ctype)
        return name

    def emit(self, line: str) -> None:
        self.lines.append(line)

    # Operands

    def slot(self, operand: str) -> Slot:
        p = self.placements[operand]
        if p.kind not in (FieldKind.home, FieldKind.feedback, FieldKind.name):
            raise Unsupported(f"{self.arm.name}: operand {operand} is a {p.kind.name} field")
        (slot,) = p.slots
        if slot.word != 0 and p.kind is not FieldKind.feedback:
            raise Unsupported(f"{self.arm.name}: operand {operand} is in an operand word")
        return slot

    def field(self, operand: str) -> str:
        """The C expression of an operand's field.  The head's fields are
        IR_A to IR_D; a field of an operand word is read from the word at its
        place after the head, which the handler has not yet passed over."""
        slot = self.slot(operand)
        if slot.word == 0:
            return f"IR_{slot.name}(ir_word)"
        return (f"((uint32_t)(this_instr[{slot.word}] >> {slot.offset - 64 * slot.word})"
                f" & {(1 << slot.width) - 1:#x})")

    def home(self, operand: str) -> str:
        """The address of an operand's home, declared on first use"""
        if operand not in self.homes:
            name = "home_" + operand
            self.homes[operand] = name
            self.declare(name, "PyValue *const",
                         f"&IR_SLOT(IR_{self.slot(operand).name}(ir_word))")
        return self.homes[operand]

    def local_flag(self, operand: str) -> str:
        return f"IR_F_LOCAL_{self.slot(operand).name}"

    # Expressions

    def type_of(self, e: Expr) -> Type:
        if isinstance(e, Name):
            return self.info.types[e.name]
        if isinstance(e, Nat):
            return ScalarType(Scalar.U64)
        if isinstance(e, BoolLit):
            return ScalarType(Scalar.Bool)
        if isinstance(e, Call):
            if e.callee == "use":
                return HandleType(Handle.B)
            if e.callee in ("dup", "acquire"):
                return HandleType(Handle.O)
            x = self.program.externs.get(e.callee)
            if x is not None and not isinstance(x.result, Cases):
                return x.result.type
        raise Unsupported(f"{self.arm.name}: the type of {e}")

    def expr(self, e: Expr, want: Type | None = None) -> str:
        text = self.expr_text(e)
        if want is None:
            return text
        have = self.type_of(e)
        if have == HandleType(Handle.O) and want == HandleType(Handle.B):
            return f"PyValue_Borrow({text})"
        return text

    def expr_text(self, e: Expr) -> str:
        if isinstance(e, Name):
            if e.name in self.placements:
                return self.operand_value(e.name)
            return self.local(e.name)
        if isinstance(e, Nat):
            return str(e.value)
        if isinstance(e, BoolLit):
            return "true" if e.value else "false"
        if isinstance(e, Call):
            return self.call_expr(e)
        raise Unsupported(f"{self.arm.name}: expression {e}")

    def operand_value(self, operand: str) -> str:
        """A name or feedback operand read as a value: the interned name, or
        the site's entry (the entry is declared by its own field)"""
        p = self.placements[operand]
        if p.kind is FieldKind.name:
            return f"GETITEM(FRAME_CO_NAMES, {self.field(operand)})"
        if p.kind is FieldKind.feedback:
            param = next(x for x in self.family.params if x.name == operand)
            schema = param.type.schema
            if schema not in SCHEMA_C:
                raise Unsupported(f"{self.arm.name}: no C entry type for feedback {schema}")
            return f"(const {SCHEMA_C[schema]} *)IR_FB({self.field(operand)})"
        raise Unsupported(f"{self.arm.name}: operand {operand} used as a value")

    def call_expr(self, e: Call) -> str:
        if e.callee == "use":
            (arg,) = e.args
            assert isinstance(arg, Name)
            return f"PyValue_Borrow(*{self.home(arg.name)})"
        if e.callee == "dup":
            (arg,) = e.args
            return f"PyValue_DUP({self.expr_text(arg)})"
        if e.callee == "acquire":
            # The owner of a dying source: a local keeps its binding, so the
            # instruction takes a count of its own; a temporary gives its own
            (arg,) = e.args
            assert isinstance(arg, Name)
            home = self.home(arg.name)
            self.acquired.add(arg.name)
            return (f"((ir_word & {self.local_flag(arg.name)}) ? PyValue_DUP(*{home}) "
                    f": _PyFrame_TakeHome(frame, {home}))")
        if e.callee == "unwrap":
            (arg,) = e.args
            return self.expr_text(arg)
        x = self.program.externs.get(e.callee)
        if x is None or x.c is None:
            raise Unsupported(f"{self.arm.name}: {e.callee} has no C body (pending)")
        conv = CONVENTIONS[x.c.convention]
        if not conv.single:
            raise Unsupported(f"{self.arm.name}: {e.callee} has cases; use a call")
        args = ", ".join(self.expr(a, p.type) for a, p in zip(e.args, x.params))
        return f"{x.c.symbol}({args})"

    # Statements

    def lower(self) -> Lowering:
        self.facts_in = arm_facts(self.program, self.arm)
        self.views = home_views(self.arm)
        order = self.layout()
        for i, label in enumerate(order):
            self.next_label = order[i + 1] if i + 1 < len(order) else None
            self.lower_block(self.blocks[label], label)
        for b in self.blocks.values():
            for p in b.params:
                t = c_type(p.type)
                if t is not None:
                    self.declare(self.local(p.name), t)
        body = []
        for line in self.lines:
            body.append(line)
        return Lowering(self.arm.name, self.enc.words, self.decls,
                        self.drop_unused_labels(body))

    def drop_unused_labels(self, lines: list[str]) -> list[str]:
        text = "\n".join(lines)
        out = []
        for line in lines:
            if line.endswith(":") and not line.startswith(" "):
                if f"goto {line[:-1]};" not in text:
                    continue
            out.append(line)
        return out

    def label(self, block: str) -> str:
        return f"{self.prefix}_{block}"

    def layout(self) -> list[str]:
        """Hot blocks first: follow each terminator's first-declared way
        out, then the rest in reverse postorder"""
        done: list[str] = []
        seen: set[str] = set()

        def successors(label: str) -> list[str]:
            return [t.label for t, _ in edges_of(self.program, self.blocks, self.blocks[label])]

        def trace(label: str) -> None:
            while label not in seen:
                seen.add(label)
                done.append(label)
                nxt = [s for s in successors(label) if s not in seen]
                if not nxt:
                    return
                label = nxt[0]
        trace("entry")
        for label in self.info.order:
            trace(label)
        return done

    def lower_block(self, block, label: str) -> None:
        if label != "entry":
            self.emit(f"{self.label(label)}:")
        self.current_facts = self.facts_in[label]
        for s in block.stmts:
            self.lower_stmt(s)
        self.lower_term(block, label)

    def lower_stmt(self, s) -> None:
        if isinstance(s, Let):
            self.lower_let(s)
        elif isinstance(s, Guard):
            cond = self.expr(s.cond)
            self.emit(f"if (!{cond}) {{" if isinstance(s.cond, Call) else f"if (!({cond})) {{")
            self.emit(f"    {self.miss()}")
            self.emit("}")
        elif isinstance(s, Publish):
            self.lower_publish(s)
        else:
            raise Unsupported(f"{self.arm.name}: statement {s}")

    def lower_let(self, s: Let) -> None:
        t = c_type(s.type)
        text = self.expr(s.value, s.type)
        if t is None:
            raise Unsupported(f"{self.arm.name}: let {s.name} has no C type")
        self.declare(self.local(s.name), t)
        self.emit(f"{s.name} = {text};")

    def miss(self) -> str:
        generic = self.generic.get(self.arm.family)
        if generic is None:
            return f"ROP_NO_GENERIC({self.arm.family});"
        return f"JUMP_TO_PREDICTED({generic});"

    # Publication

    def lower_publish(self, s: Publish) -> None:
        facts = self.current_facts
        # The homes are cleared in operand order; the closes that follow keep
        # the order of the debts
        order = list(self.placements)
        spent = sorted((d for d in s.debts if d.kind is DebtKind.spent),
                       key=lambda d: order.index(d.subject))
        olds = [d for d in s.debts if d.kind is DebtKind.old]
        for d in s.debts:
            if d.kind not in (DebtKind.spent, DebtKind.old):
                raise Unsupported(f"{self.arm.name}: debt {d.kind.name} is not lowered yet")
        owners: dict[str, str | None] = {}
        for d in spent:
            if d.subject in self.acquired:
                # acquire() already took the owner or a count of its own
                owners[d.subject] = None
                continue
            home, flag = self.home(d.subject), self.local_flag(d.subject)
            view = self.views.get(d.subject)
            if view is not None and facts.has(view, Fact.inline_int):
                # An inline int owns no count: clearing the home retires it
                owners[d.subject] = None
                self.emit(f"if (!(ir_word & {flag})) {{")
                self.emit(f"    _PyFrame_WriteHome(frame, {home}, PyValue_NULL);")
                self.emit("}")
                continue
            owner = "own_" + d.subject
            self.declare(owner, "PyValue", "PyValue_NULL")
            owners[d.subject] = owner
            self.emit(f"if (!(ir_word & {flag})) {{")
            self.emit(f"    {owner} = _PyFrame_TakeHome(frame, {home});")
            self.emit("}")
        for d in olds:
            self.declare("old_" + d.subject, "PyValue")
            self.emit(f"old_{d.subject} = *{self.home(d.subject)};")
        for b in s.bindings:
            home = self.home(b.target)
            self.emit(f"_PyFrame_WriteHome(frame, {home}, {self.expr(b.value)});")
            self.emit(f"_PyValue_CheckHome(*{home});")
        for d in s.debts:
            if d.kind is DebtKind.spent and owners[d.subject] is not None:
                self.emit(f"PyValue_XCLOSE({owners[d.subject]});")
            elif d.kind is DebtKind.old:
                self.emit(f"PyValue_XCLOSE(old_{d.subject});")

    # Terminators

    def lower_term(self, block, label: str) -> None:
        self.current_facts = self.facts_in[label]
        t = block.term
        if isinstance(t, Goto):
            self.jump(t.target)
        elif isinstance(t, Branch):
            self.emit(f"if ({self.expr(t.cond)}) {{")
            self.jump(t.if_true, indent="    ", force=True)
            self.emit("}")
            self.jump(t.if_false)
        elif isinstance(t, CallTerm):
            self.lower_call(t)
        elif isinstance(t, Exit):
            self.lower_exit(t)
        elif isinstance(t, (Each, Advance)):
            raise Unsupported(f"{self.arm.name}: each/advance is not lowered yet")

    def jump(self, target: Target, payload: tuple[str, ...] = (), indent: str = "",
             force: bool = False) -> None:
        params = self.blocks[target.label].params
        values = list(payload) + [self.expr(a) for a in target.args]
        pairs = [(p, v) for p, v in zip(params, values) if c_type(p.type) is not None]
        if len(pairs) > 1:
            temps = []
            for p, v in pairs:
                tmp = self.new_temp(c_type(p.type), "arg")
                self.emit(f"{indent}{tmp} = {v};")
                temps.append(tmp)
            pairs = [(p, tmp) for (p, _), tmp in zip(pairs, temps)]
        for p, v in pairs:
            if p.name != v:
                self.emit(f"{indent}{self.local(p.name)} = {v};")
        if force or target.label != self.next_label:
            self.emit(f"{indent}goto {self.label(target.label)};")

    def lower_call(self, t: CallTerm) -> None:
        x = self.program.externs.get(t.callee)
        if x is None or x.c is None:
            raise Unsupported(f"{self.arm.name}: {t.callee} has no C body (pending)")
        conv = CONVENTIONS[x.c.convention]
        args = [self.expr(a, p.type) for a, p in zip(t.args, x.params)]
        cases = x.result.cases
        edges = {e.case: e for e in t.edges}
        out = None
        if any(CSource.out in o.payload for o in conv.outcomes):
            out = self.new_temp(c_type(cases[0].params[0].type), "out")
            args.append(f"&{out}")
        # A bool result is tested in place; any other is kept for the tests
        # and the payloads that read it
        inline = conv.returns is CReturn.bool and len(conv.outcomes) == 2
        ret = f"{x.c.symbol}({', '.join(args)})"
        if x.contract.effect_set & ESCAPING:
            self.emit("IR_STUB_ESCAPE();")
        if not inline:
            call = ret
            ret = self.new_temp(RETURN_C[conv.returns], "ret")
            self.emit(f"{ret} = {call};")
        ways = []
        for i, outcome in enumerate(conv.outcomes):
            target = edges[outcome.case or cases[i].name].target
            payload = tuple(source_text(src, ret, out) for src in outcome.payload)
            ways.append((outcome, target, payload))
        # The way into the next block is the one that falls through
        ways.sort(key=lambda w: w[1].label == self.next_label)
        for outcome, target, payload in ways[:-1]:
            self.emit(f"if ({test_text(outcome.test, conv.returns, ret)}) {{")
            self.jump(target, payload, "    ", force=True)
            self.emit("}")
        _, target, payload = ways[-1]
        self.jump(target, payload)

    def lower_exit(self, t: Exit) -> None:
        if t.kind == "next":
            self.emit("DISPATCH();")
        elif t.kind == "miss":
            self.emit(self.miss())
        elif t.kind == "raise":
            point = t.args[0]
            assert isinstance(point, Name)
            self.emit(f"PROCESS_ERROR_AT(RAISE, {self.points[point.name]}, this_instr);")
        else:
            raise Unsupported(f"{self.arm.name}: exit {t.kind} is not lowered yet")


def lower_arm(program: Program, arm: Arm, numbers: Mapping[str, int],
              points: Mapping[str, int], generic: Mapping[str, str | None]) -> Lowering:
    return ArmLowerer(program, arm, numbers, points, generic).lower()
