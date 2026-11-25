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

from rop_catalog import (CONVENTIONS, ENVIRONMENT_C, FB_LAYOUT, RECIPE_LAYOUT, CReturn,
                         CSource, CTest, DebtKind, Effect, Fact, FieldKind, Handle, Scalar, Schema, SchemaCtor,
                         Slot, Wrap)
from rop_check import GraphInfo, Program, is_args
from rop_model import (Advance, Arm, BoolLit, Branch, Binding, Block, Call, CallTerm,
                       Cases, DISCARD, Debt, Each, Edge, Exit, Expr, Extern, Goto, Guard,
                       HandleType, Let, Name, Nat, Null, Param, Placement, Publish,
                       ScalarType, SchemaType, SchemaVar, SpanLit, Target, Type,
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
    Handle.Anchors: "_PyRopAnchors",
    Handle.Thread: "PyThreadState *",
    Handle.Frame: "_PyInterpreterFrame *",
    Handle.NewFrame: "_PyInterpreterFrame *",
    Handle.DeadFrame: "_PyInterpreterFrame *",
    Handle.Head: "_PyInstr *",
    # An argument descriptor is the instruction's head: a row reads its
    # count, self role and homes from the head and the operand words
    Handle.Args: "const _PyIRWord *",
}

# Linear tokens that the generated C does not carry as values
TOKENS = frozenset({Handle.Err, Handle.Route})

# Effects after which Python may have run: a stub reports ESCAPED
ESCAPING = frozenset({Effect.runs_python, Effect.polls})
# A call that may release an owner may run a finalizer
ESCAPING_CALL = ESCAPING | {Effect.releases}

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
        case CSource.exception | CSource.token:
            return ""


# The C entry type of each feedback schema whose arms are lowered
SCHEMA_C = {schema: layout.struct for schema, layout in FB_LAYOUT.items()}


def c_type(t: Type) -> str | None:
    """The C type of a graph value, None for one that has no C variable"""
    if isinstance(t, SchemaType) and t.ctor is SchemaCtor.Recipe:
        layout = RECIPE_LAYOUT.get(t.schema)
        return layout.struct if layout is not None else None
    if isinstance(t, ScalarType):
        return SCALAR_C.get(t.scalar)
    if isinstance(t, HandleType):
        return HANDLE_C.get(t.handle)
    if isinstance(t, WrapType) and t.wrap is Wrap.Maybe:
        return c_type(t.arg)
    if isinstance(t, SchemaType) and t.ctor is SchemaCtor.Candidate:
        return "int"
    if t == WrapType(Wrap.Span, HandleType(Handle.O)):
        return "_PyRopOwners"
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


# Macro expansion.  A call of a macro with a graph is replaced by that graph,
# its names prefixed and its parameters replaced by the call's arguments;
# each of its exits becomes a goto to the caller's edge for that case.  The
# lowering then sees only rows.

@dataclass(frozen=True)
class FlatGraph:
    blocks: Mapping[str, Block]
    types: Mapping[str, Type]
    order: tuple[str, ...]


def rename_expr(e: Expr, names: Mapping[str, Expr]) -> Expr:
    if isinstance(e, Name):
        return names.get(e.name, e)
    if isinstance(e, Call):
        return Call(e.callee, tuple(rename_expr(a, names) for a in e.args), e.span)
    if isinstance(e, SpanLit):
        return SpanLit(tuple(rename_expr(a, names) for a in e.items), e.span)
    return e


def successors(term) -> list[str]:
    if isinstance(term, Goto):
        return [term.target.label]
    if isinstance(term, Branch):
        return [term.if_true.label, term.if_false.label]
    if isinstance(term, CallTerm):
        return [e.target.label for e in term.edges]
    if isinstance(term, Each):
        return [term.body, term.exhaust.label]
    return []


def flatten(program: Program, arm: Arm) -> FlatGraph:
    blocks: dict[str, Block] = {b.label: b for b in arm.graph.blocks}
    types: dict[str, Type] = dict(program.graphs[arm.name].types)
    work = list(blocks)
    instances = 0
    while work:
        label = work.pop()
        block = blocks[label]
        term = block.term
        macro = program.macros.get(term.callee) if isinstance(term, CallTerm) else None
        if macro is None or macro.graph is None:
            continue
        instances += 1
        prefix = f"{macro.name}{instances}_"
        info = program.graphs[macro.name]
        own = {b.label for b in macro.graph.blocks}
        local = {p.name for b in macro.graph.blocks for p in b.params}
        local |= {st.name for b in macro.graph.blocks for st in b.stmts
                  if isinstance(st, Let) and st.name != DISCARD}
        names: dict[str, Expr] = {n: Name(prefix + n) for n in local}
        binds: list[Let] = []
        for p, a in zip(macro.params, term.args):
            if isinstance(a, (Name, SpanLit)):
                names[p.name] = a
            else:
                names[p.name] = Name(prefix + p.name)
                binds.append(Let(prefix + p.name, p.type, a))
                types[prefix + p.name] = p.type
        for n in local:
            types[prefix + n] = info.types[n]
        edges = {e.case: e.target for e in term.edges}

        def target(t: Target, extra: tuple[Expr, ...] = ()) -> Target:
            return Target(prefix + t.label,
                          extra + tuple(rename_expr(a, names) for a in t.args))

        for mb in macro.graph.blocks:
            stmts = []
            for st in mb.stmts:
                if isinstance(st, Let):
                    stmts.append(Let(st.name if st.name == DISCARD else prefix + st.name,
                                     st.type,
                                     rename_expr(st.value, names)))
                elif isinstance(st, Guard):
                    stmts.append(Guard(rename_expr(st.cond, names)))
                else:
                    raise Unsupported(f"{macro.name}: a publish in a macro")
            t = mb.term
            if isinstance(t, Exit):
                payload = tuple(rename_expr(a, names) for a in t.args)
                out = edges[t.kind]
                new = Goto(Target(out.label, payload + out.args))
            elif isinstance(t, Goto):
                new = Goto(target(t.target))
            elif isinstance(t, Branch):
                new = Branch(rename_expr(t.cond, names), target(t.if_true),
                             target(t.if_false))
            elif isinstance(t, CallTerm):
                new = CallTerm(t.callee,
                               tuple(rename_expr(a, names) for a in t.args),
                               tuple(Edge(e.case, target(e.target)) for e in t.edges))
            else:
                raise Unsupported(f"{macro.name}: {type(t).__name__} in a macro")
            params = tuple(Param(prefix + p.name, p.type) for p in mb.params)
            first = binds if mb.label == "entry" else []
            blocks[prefix + mb.label] = Block(prefix + mb.label, params,
                                              tuple(first + stmts), new)
            work.append(prefix + mb.label)
        blocks[label] = Block(label, block.params, block.stmts,
                              Goto(Target(prefix + "entry", ())))
    seen: set[str] = set()
    post: list[str] = []

    def visit(label: str) -> None:
        if label in seen:
            return
        seen.add(label)
        for nxt in successors(blocks[label].term):
            visit(nxt)
        post.append(label)

    visit("entry")
    return FlatGraph(blocks, types, tuple(reversed(post)))


# Facts known on entry to each block, by a forward pass over the blocks in
# reverse postorder.  A guard grants the facts its row's `test` declares,
# a case grants its row's `ensures`, and a block parameter keeps what every
# incoming edge grants.

def arm_facts(program: Program, flat: FlatGraph) -> dict[str, Facts]:
    info = flat
    blocks = dict(flat.blocks)
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


def is_transfer(program: Program, callee: str) -> bool:
    x = program.externs.get(callee)
    return (x is not None and x.c is not None
            and CONVENTIONS[x.c.convention].transfer)


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


def home_views(flat: FlatGraph) -> dict[str, str]:
    """The value each `let v = use(x)` binds, by home operand"""
    views: dict[str, str] = {}
    for b in flat.blocks.values():
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
    # The C expression of the payload words that follow them (an argument
    # descriptor's homes, four to a word), if the family has a payload
    payload: str | None = None
    spans: bool = False      # span operands: the counts give the length
    decls: list[str] = field(default_factory=list)
    body: list[str] = field(default_factory=list)


class ArmLowerer:
    def __init__(self, program: Program, arm: Arm, numbers: Mapping[str, int],
                 points: Mapping[str, int], generic: Mapping[str, str | None]
                 ) -> None:
        self.program = program
        self.arm = arm
        self.info: FlatGraph = flatten(program, arm)
        self.family = program.families[arm.family]
        self.enc = program.encodings[arm.family]
        self.placements: dict[str, Placement] = {
            p.operand: p for p in self.enc.placements}
        self.points = points
        self.generic = generic
        self.prefix = "rop_" + arm.name.replace(".", "_")
        self.blocks = dict(self.info.blocks)
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
        if p.kind not in (FieldKind.home, FieldKind.feedback, FieldKind.name,
                          FieldKind.imm, FieldKind.target, FieldKind.vector) or not p.slots:
            raise Unsupported(f"{self.arm.name}: operand {operand} is a {p.kind.name} field")
        slot = p.slots[0]
        if len(p.slots) > 1 and p.kind is not FieldKind.target:
            raise Unsupported(f"{self.arm.name}: operand {operand} takes {len(p.slots)} fields")
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

    def is_vector(self, operand: str) -> bool:
        """An argument descriptor (Args): its homes follow in the payload
        words.  Other vector operands are spans (is_span_operand)."""
        p = self.placements.get(operand)
        return (p is not None and p.kind is FieldKind.vector
                and not self.is_span_operand(operand))

    def local_flag(self, operand: str) -> str:
        return f"IR_F_LOCAL_{self.slot(operand).name}"

    # Expressions

    def type_of(self, e: Expr) -> Type:
        if isinstance(e, Name):
            # An enum member has no entry: it is the C constant of its name
            return self.info.types.get(e.name)
        if isinstance(e, Nat):
            return ScalarType(Scalar.U64)
        if isinstance(e, BoolLit):
            return ScalarType(Scalar.Bool)
        if isinstance(e, Call):
            if e.callee == "use":
                return HandleType(Handle.B)
            if e.callee in ("dup", "acquire"):
                return HandleType(Handle.O)
            if e.callee == "candidate":
                return None
            if e.callee == "count":
                return ScalarType(Scalar.Size)
            x = self.program.externs.get(e.callee)
            if x is not None and not isinstance(x.result, Cases):
                return x.result.type
        raise Unsupported(f"{self.arm.name}: the type of {e}")

    def srcs(self, e: SpanLit) -> str:
        """The homes and local flags of a span of source operands"""
        names = []
        for item in e.items:
            if not isinstance(item, Name) or item.name not in self.placements:
                raise Unsupported(f"{self.arm.name}: a span item {item} is not an operand")
            names.append(item.name)
        homes = ", ".join(self.home(n) for n in names)
        flags = ", ".join(f"!!(ir_word & {self.local_flag(n)})" for n in names)
        return (f"&(_PyRopSrcs){{frame, {len(names)}, {{{flags}}}, {{{homes}}}}}")

    def is_span_operand(self, operand: str) -> bool:
        """A vector operand other than an argument descriptor (Args), whose
        homes follow in the payload words"""
        p = self.placements.get(operand)
        return (p is not None and p.kind is FieldKind.vector
                and not is_args(next(x for x in self.family.params
                                     if x.name == operand).type))

    def is_span(self, e: Expr) -> bool:
        return isinstance(e, Name) and self.is_span_operand(e.name)

    def span(self, operand: str) -> str:
        """A span operand: its count, and the index of its first element
        among the elements of the word's spans, which follow in
        declaration order (pycore_ir.h, _PyIR_SpanElement())"""
        before = []
        for name, p in self.placements.items():
            if name == operand:
                break
            if self.is_span_operand(name):
                before.append(self.field(name))
        first = " + ".join(before) or "0"
        return (f"&(_PyRopSpan){{frame, this_instr, opcode, {first}, "
                f"{self.field(operand)}}}")

    def c_args(self, args, params) -> list[str]:
        """The C arguments of a row call: a token has no value, an anchor
        and a span of sources are passed by address"""
        out = []
        for a, p in zip(args, params):
            t = p.type
            if isinstance(t, HandleType) and t.handle in TOKENS:
                continue
            if isinstance(t, SchemaType) and t.ctor is SchemaCtor.Ticket:
                continue
            if isinstance(a, SpanLit):
                out.append(self.srcs(a))
            elif self.is_span(a):
                out.append(self.span(a.name))
            elif isinstance(t, HandleType) and t.handle is Handle.Anchors:
                out.append("&" + self.expr_text(a))
            elif (t in (HandleType(Handle.Src), HandleType(Handle.Local),
                        WrapType(Wrap.Maybe, HandleType(Handle.Src)))
                    and isinstance(a, Name) and a.name in self.placements):
                # A row that reads or spends a home takes its address
                out.append(self.home(a.name))
            elif (isinstance(t, SchemaType) and t.ctor is SchemaCtor.Fb
                  and isinstance(t.schema, SchemaVar)):
                # A row over every schema reads the entry's common header
                out.append(f"&{self.expr_text(a)}->h")
            else:
                out.append(self.expr(a, t))
        return out

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
            if e.name in ENVIRONMENT_C:
                return ENVIRONMENT_C[e.name]
            if e.name not in self.info.types:
                return e.name.rsplit(".", 1)[-1]
            return self.local(e.name)
        if isinstance(e, Nat):
            return str(e.value)
        if isinstance(e, BoolLit):
            return "true" if e.value else "false"
        if isinstance(e, Call):
            return self.call_expr(e)
        raise Unsupported(f"{self.arm.name}: expression {e}")

    def immediate(self, operand: str) -> str:
        """An immediate operand's value: its head fields, low field first"""
        p = self.placements[operand]
        if not p.slots or any(s.word != 0 for s in p.slots):
            raise Unsupported(f"{self.arm.name}: immediate {operand} is not in the head")
        parts = [f"((uint32_t)IR_{s.name}(ir_word) << {12 * i})" if i else
                 f"(uint32_t)IR_{s.name}(ir_word)" for i, s in enumerate(p.slots)]
        return "(" + " | ".join(parts) + ")" if len(parts) > 1 else parts[0]

    def operand_value(self, operand: str) -> str:
        """A name, feedback, home or immediate operand read as a value: the
        interned name, the site's entry (the entry is declared by its own
        field), the address of the home, which a row reads or writes
        itself, or the immediate"""
        p = self.placements[operand]
        if p.kind is FieldKind.home:
            return self.home(operand)
        if p.kind is FieldKind.imm:
            return self.immediate(operand)
        if p.kind is FieldKind.name:
            return f"GETITEM(FRAME_CO_NAMES, {self.field(operand)})"
        if p.kind is FieldKind.feedback:
            param = next(x for x in self.family.params if x.name == operand)
            schema = param.type.schema
            if schema not in SCHEMA_C:
                raise Unsupported(f"{self.arm.name}: no C entry type for feedback {schema}")
            return f"(({SCHEMA_C[schema]} *)IR_FB({self.field(operand)}))"
        if p.kind is FieldKind.imm:
            param = next(x for x in self.family.params if x.name == operand)
            if param.type == ScalarType(Scalar.Bool):
                return f"({self.field(operand)} != 0)"
            return self.field(operand)
        if p.kind is FieldKind.target:
            return self.target_value(operand)
        if p.kind is FieldKind.vector and p.slots:
            return "this_instr"
        raise Unsupported(f"{self.arm.name}: operand {operand} used as a value")

    def target_value(self, operand: str) -> str:
        """A branch or jump target: the word the signed distance in the
        operand's field or fields leads to, counted from the word after the
        head (the jump operands of a family have no operand words)"""
        p = self.placements[operand]
        if self.enc.words:
            raise Unsupported(f"{self.arm.name}: a target in a family with operand words")
        first = p.slots[0]
        assert [s.offset for s in p.slots] == [first.offset + 12 * i for i in range(len(p.slots))]
        sign = 1 << (p.bits - 1)
        raw = f"((uint32_t)(ir_word >> {first.offset}) & {(1 << p.bits) - 1:#x})"
        return f"(this_instr + 1 + ((int)({raw} ^ {sign:#x}) - {sign:#x}))"

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
        if e.callee == "count":
            (arg,) = e.args
            assert isinstance(arg, Name)
            if self.is_span(arg):
                return f"(Py_ssize_t){self.field(arg.name)}"
            return f"{arg.name}.count"
        if e.callee == "candidate":
            arm = e.args[0]
            assert isinstance(arm, Name)
            return "ROP_HANDLER_" + arm.name.replace(".", "_")
        x = self.program.externs.get(e.callee)
        if x is None or x.c is None:
            raise Unsupported(f"{self.arm.name}: {e.callee} has no C body (pending)")
        conv = CONVENTIONS[x.c.convention]
        if not conv.single:
            raise Unsupported(f"{self.arm.name}: {e.callee} has cases; use a call")
        return f"{x.c.symbol}({', '.join(self.c_args(e.args, x.params))})"

    # Statements

    def lower(self) -> Lowering:
        self.facts_in = arm_facts(self.program, self.info)
        self.views = home_views(self.info)
        # A transfer row's callee performs the exits of its edges' blocks
        self.transfer_blocks = {
            e.target.label for b in self.blocks.values()
            if isinstance(b.term, CallTerm) and is_transfer(self.program, b.term.callee)
            for e in b.term.edges}
        order = [label for label in self.layout() if label not in self.transfer_blocks]
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
        return Lowering(self.arm.name, self.enc.words, self.payload_words(),
                        any(self.is_span_operand(n) for n in self.placements),
                        self.decls,
                        self.drop_unused_labels(body))

    def payload_words(self) -> str | None:
        """An argument descriptor's homes follow the operand words, four to
        a word, as many as its count field says"""
        args = [p for p in self.enc.placements
                if self.is_vector(p.operand) and p.slots]
        if not args:
            return None
        (p,) = args
        return (f"(({self.field(p.operand)} + IR_OPERANDS_PER_WORD - 1) / "
                "IR_OPERANDS_PER_WORD)")

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
        if isinstance(s.value, Call):
            x = self.program.externs.get(s.value.callee)
            if x is not None and x.contract.effect_set & ESCAPING_CALL:
                self.emit("IR_STUB_ESCAPE();")
        text = self.expr(s.value, s.type)
        if t is None:
            # A Unit or a token: the row runs for its effect
            if not isinstance(s.value, Call):
                raise Unsupported(f"{self.arm.name}: let {s.name} has no C type")
            self.emit(f"{text};")
            return
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
        # d[] = v: StoreOwners exchanges each item of v with its home's
        # displaced owner, which old(d) closes with the storage
        stored = {b.target: b.value.name for b in s.bindings if b.owners}
        vectors = [b for b in s.bindings if b.vector and not b.owners]
        moved = {b.value.name for b in vectors if isinstance(b.value, Name)}
        owners: dict[str, str | None] = {}
        for b in vectors:
            # A vector binding's runtime takes every source before it writes
            # a destination, which are vacant (_PyFrame_ParallelMove)
            if isinstance(b.value, Call):
                (source,) = b.value.args
                self.emit(f"_PyRop_CopyHomes({self.span(b.target)}, {self.span(source.name)});")
            else:
                self.emit(f"_PyRop_ParallelMove({self.span(b.target)}, "
                          f"{self.span(b.value.name)});")
        for d in spent:
            if d.subject in moved:
                continue
            if self.is_vector(d.subject):
                # An argument vector's temporaries close after the bindings
                # (_PyRop_SpendArgs), all but a home that a binding now holds
                owners[d.subject] = None
                continue
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
            if d.subject in stored:
                continue
            self.declare("old_" + d.subject, "PyValue")
            self.emit(f"old_{d.subject} = *{self.home(d.subject)};")
        for target, value in stored.items():
            self.emit(f"_PyRop_StoreOwners({self.span(target)}, &{value});")
        for b in s.bindings:
            if b.vector:
                continue
            home = self.home(b.target)
            if isinstance(b.value, Null):
                self.emit(f"_PyFrame_WriteHome(frame, {home}, PyValue_NULL);")
                continue
            self.emit(f"_PyFrame_WriteHome(frame, {home}, {self.expr(b.value)});")
            self.emit(f"_PyValue_CheckHome(*{home});")
        scalar = [b for b in s.bindings if not b.vector]
        kept = self.home(scalar[0].target) if len(s.bindings) == 1 and scalar else "NULL"
        for d in s.debts:
            if d.kind is DebtKind.spent and d.subject in moved:
                # The sources past the pairs, closed after every home is written
                target = next(b.target for b in vectors if b.value.name == d.subject)
                # The closes may run finalizers, which a stub reports
                self.emit("IR_STUB_ESCAPE();")
                self.emit(f"_PyRop_CloseRest({self.span(target)}, {self.span(d.subject)});")
            elif d.kind is DebtKind.owned:
                self.emit(f"PyValue_XCLOSE({self.expr_text(Name(d.subject))});")
            elif d.kind is DebtKind.spent and self.is_vector(d.subject):
                self.emit(f"_PyRop_SpendArgs(frame, this_instr, {kept});")
            elif d.kind is DebtKind.spent and owners[d.subject] is not None:
                self.emit(f"PyValue_XCLOSE({owners[d.subject]});")
            elif d.kind is DebtKind.old and d.subject in stored:
                # The closes may run finalizers, which a stub reports
                self.emit("IR_STUB_ESCAPE();")
                self.emit(f"_PyRop_CloseOwners(&{stored[d.subject]});")
            elif d.kind is DebtKind.old:
                self.emit(f"PyValue_XCLOSE(old_{d.subject});")
            elif d.kind is DebtKind.roots:
                self.emit(f"_PyRop_CloseAnchors(&{d.subject});")

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
        args = self.c_args(t.args, x.params)
        cases = x.result.cases
        edges = {e.case: e for e in t.edges}
        if conv.transfer:
            # The callee leaves the word by the exit of the case it takes
            self.emit(f"{x.c.symbol}({', '.join(args)});")
            return
        if conv.indexed:
            self.lower_indexed_call(t, x, args)
            return
        out = None
        for i, o in enumerate(conv.outcomes):
            if CSource.out in o.payload:
                out_type = cases[i].params[o.payload.index(CSource.out)].type
                out = self.new_temp(c_type(out_type), "out")
                break
        if out is not None:
            args.append(f"&{out}")
        # A bool result is tested in place; any other is kept for the tests
        # and the payloads that read it
        inline = conv.returns is CReturn.bool and len(conv.outcomes) == 2
        ret = f"{x.c.symbol}({', '.join(args)})"
        if x.contract.effect_set & ESCAPING_CALL:
            self.emit("IR_STUB_ESCAPE();")
        if conv.returns is CReturn.void:
            # The row's one outcome always holds (raised)
            self.emit(f"{ret};")
            ret = ""
        elif not inline:
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

    def lower_indexed_call(self, t: CallTerm, x: Extern, args: list[str]) -> None:
        """A row whose C result is the position of its case; a payload value
        is written through the last parameter"""
        cases = x.result.cases
        payload_case = next((c for c in cases if c.params), None)
        out = None
        if payload_case is not None:
            out = self.new_temp(c_type(payload_case.params[0].type), "out")
            args.append(f"&{out}")
        if x.contract.effect_set & ESCAPING:
            self.emit("IR_STUB_ESCAPE();")
        ret = self.new_temp("int", "ret")
        self.emit(f"{ret} = {x.c.symbol}({', '.join(args)});")
        edges = {e.case: e.target for e in t.edges}
        ways = [(i, edges[c.name], (out,) if c.params else ()) for i, c in enumerate(cases)]
        ways.sort(key=lambda w: w[1].label == self.next_label)
        for i, target, payload in ways[:-1]:
            self.emit(f"if ({ret} == {i}) {{")
            self.jump(target, payload, "    ", force=True)
            self.emit("}")
        _, target, payload = ways[-1]
        self.jump(target, payload)

    def lower_exit(self, t: Exit) -> None:
        if t.kind == "next":
            self.emit("DISPATCH();")
        elif t.kind == "miss":
            self.emit(self.miss())
        elif t.kind == "redirect":
            self.emit("IR_REDIRECT();")
        elif t.kind in ("branch", "jump"):
            # The target is the family's operand: the word's static target.
            # A stub reports BRANCH and leaves the target to its caller.
            (target,) = t.args
            assert isinstance(target, Name)
            self.emit(f"IR_BRANCH({self.operand_value(target.name)});")
        elif t.kind == "again":
            # The word runs again, under the handler it now names
            self.emit("next_instr = this_instr;")
            self.emit("DISPATCH_SAME_OPARG();")
        elif t.kind == "raise":
            point = t.args[0]
            assert isinstance(point, Name)
            self.emit(f"PROCESS_ERROR_AT(RAISE, {self.points[point.name]}, this_instr);")
        elif t.kind == "enter":
            self.emit(f"ROP_ENTER({self.expr(t.args[0])}, {self.resume_slot(t)});")
        elif t.kind == "return":
            caller, head = (self.expr(a) for a in t.args)
            self.emit(f"ROP_RETURN({caller}, {head});")
        elif t.kind == "exit":
            self.emit(f"IR_EXIT({self.expr(t.args[0])});")
        else:
            raise Unsupported(f"{self.arm.name}: exit {t.kind} is not lowered yet")

    def resume_slot(self, t: Exit) -> str:
        """The home that the child's return value goes to: the resume
        point's definition"""
        assert t.resume is not None
        defs = self.program.points[t.resume].defs
        if not defs:
            raise Unsupported(f"{self.arm.name}: a resume point with no definition")
        (dst,) = defs
        return self.field(dst)


def lower_arm(program: Program, arm: Arm, numbers: Mapping[str, int],
              points: Mapping[str, int], generic: Mapping[str, str | None]) -> Lowering:
    return ArmLowerer(program, arm, numbers, points, generic).lower()
