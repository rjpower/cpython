"""Interpreter C of register instruction arms (F2).

`lower_arm` turns one checked arm graph into the statements of a handler
body: the arm's function in the tail-calling interpreter
(Python/ceval_evaluator.h) and, with the transfer macros (`DISPATCH`,
`PROCESS_ERROR_AT`, `JUMP_TO_PREDICTED`) of the baseline JIT's stencils
(Python/jit_stencils.c), its stencil.

Operands are decoded from the family's derived encoding, a call lowers
through its extern's C convention (rop_catalog.CONVENTIONS, as data),
`publish` follows each source's local flag, and each exit becomes dispatch,
the error path with its point index, or the family's miss adapter.  The
generated C uses the value API and never `.bits`.
"""

import re
from collections.abc import Mapping
from dataclasses import dataclass, field, replace

from rop_catalog import (CONVENTIONS, ENVIRONMENT_C, FB_LAYOUT, RECIPE_LAYOUT, CReturn, RowKind,
                         CSource, CTest, DebtKind, Effect, Fact, FieldKind, Handle, Scalar, Schema, SchemaCtor,
                         PcRole, Slot, UNCOUNTED_FACTS, Wrap)
from rop_check import GraphInfo, Program, enum_c_name, enum_of_member, expand_debts, fixed_locals, expand_group, group_part_decls, group_parts, is_args
from rop_model import (Advance, Arm, EnumType, BoolLit, Branch, Binding, Block, Call, CallTerm,
                       Cases, DISCARD, Debt, Each, Edge, Exit, Expr, Extern, Goto, Guard,
                       HandleType, Let, Name, Nat, Null, Param, Placement, Publish,
                       ScalarType, SchemaType, SchemaVar, SpanLit, Target, Type,
                       Returns, WrapType)


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

# Rule HT-R1: the effects after which something may read the frame's pc
# (a traceback, f_lineno, tracemalloc, a finalizer, the call protocol)
PC_EFFECTS = frozenset({Effect.allocates, Effect.raises, Effect.releases,
                        Effect.runs_python, Effect.polls, Effect.switches_frame})
# The frame handles: a row that takes one may read or set the pc
FRAME_HANDLES = frozenset({Handle.Frame, Handle.NewFrame, Handle.DeadFrame})
# The exits that leave the word's frame to code that reads its pc
PC_EXITS = frozenset({"raise", "unwind", "enter", "yield", "return", "exit", "redirect"})
# A publication point of the pc, which ArmLowerer.resolve_publish keeps or drops
PUBLISH_MARK = "IR_PUBLISH_IP(); // in "


def row_sets_pc(x: Extern) -> bool:
    """Whether row x may set the frame's pc: it takes the frame (as
    set_origin does) or is a transfer.  Python that a row runs cannot: a
    running frame's pc moves under a debugger only in the observing
    evaluator, which publishes on entry."""
    if x.c is not None and CONVENTIONS[x.c.convention].transfer:
        return True
    return any(isinstance(p.type, HandleType) and p.type.handle in FRAME_HANDLES
               for p in x.params)


def row_reads_pc(x: Extern) -> bool:
    """Rule HT-R1: whether a call of row x is an escape point: it has an
    effect after which the frame's pc may be read, or it may set the pc.  A
    primitive is pure by its contract."""
    if x.kind is RowKind.primitive:
        return False
    return bool(x.contract.effect_set & PC_EFFECTS) or row_sets_pc(x)


# Names the handler's C scope owns
RESERVED = frozenset({
    "frame", "tstate", "next_instr", "ir_word", "opcode", "this_instr",
    "ir_old", "prev_instr",
})


RETURN_C = {
    CReturn.bool: "bool", CReturn.scalar: "int64_t", CReturn.value: "PyValue",
    CReturn.borrow: "PyValueBorrow", CReturn.pointer: "void *",
    CReturn.int_status: "int", CReturn.status_value: "_PyStatusValue",
}


def test_text(test: CTest, returns: CReturn, ret: str) -> str:
    """The C condition under which an outcome's test holds of the result"""
    subject = f"{ret}.status" if returns is CReturn.status_value else ret
    if returns in (CReturn.value, CReturn.borrow):
        null, nonnull = f"PyValue_IsNull({ret})", f"!PyValue_IsNull({ret})"
    else:
        null, nonnull = f"{ret} == NULL", f"{ret} != NULL"
    return {
        CTest.true: ret, CTest.false: f"!{ret}",
        CTest.nonnull: nonnull, CTest.null: null,
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
        return "_PyRopItems"
    if isinstance(t, EnumType):
        # A member that a primitive computes from another operand
        return "uint32_t"
    return None


def inline_rows(program: Program) -> list[Extern]:
    """The rows declared `inline`, in declaration order"""
    return [x for x in program.externs.values() if x.c is not None and x.c.inline]


def row_signature(x: Extern) -> tuple[str, list[str]]:
    """The C return type and parameter types of a row's entry point, from
    its declaration, the way a call passes them (c_args).  The C compiler
    checks them against the body."""
    assert x.c is not None
    conv = CONVENTIONS[x.c.convention]
    params = []
    for p in x.params:
        t = p.type
        if isinstance(t, HandleType) and t.handle in TOKENS:
            continue
        if isinstance(t, SchemaType) and t.ctor is SchemaCtor.Ticket:
            continue
        if t in (HandleType(Handle.Src), HandleType(Handle.Local),
                 WrapType(Wrap.Maybe, HandleType(Handle.Src))):
            params.append("PyValue *")
        elif isinstance(t, SchemaType) and t.ctor is SchemaCtor.Fb:
            params.append(f"const {SCHEMA_C[t.schema]} *")
        elif c_type(t) is not None:
            params.append(c_type(t))
        else:
            raise Unsupported(f"{x.name}: parameter {p.name} has no C type for an "
                              "inline row's out-of-line copy")
    if conv.returns is CReturn.void:
        ret = "void"
    elif conv.returns in (CReturn.pointer, CReturn.handle):
        if not isinstance(x.result, Returns) or c_type(x.result.type) is None:
            raise Unsupported(f"{x.name}: no C result type for an inline row")
        ret = c_type(x.result.type)
    else:
        ret = RETURN_C[conv.returns]
    return ret, params


@dataclass(frozen=True)
class Facts:
    """What is known of each value on entry to a block: its facts, as
    (fact, type argument) pairs."""
    by_name: Mapping[str, frozenset[tuple[Fact, object]]]

    def has(self, name: str, fact: Fact) -> bool:
        return any(f is fact for f, _ in self.by_name.get(name, ()))

    def uncounted(self, name: str) -> bool:
        """The value owns no count (UNCOUNTED_FACTS)"""
        return bool(self.by_name.get(name, frozenset()) & UNCOUNTED_FACTS)


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
                              Goto(Target(prefix + "entry", ())), block.span,
                              block.outline)
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


def facts_at(program: Program, entry: Facts, block: Block, index: int) -> Facts:
    """The facts before statement `index` of a block: those on entry, and
    those that each earlier guard's test grants"""
    known = dict(entry.by_name)
    for s in block.stmts[:index]:
        if isinstance(s, Guard):
            add_test_facts(program, known, s.cond)
    return Facts(known)


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


def acquired_on_entry(program: Program, blocks) -> dict[str, frozenset[str]]:
    """The operands whose owner an acquire() took before each block, on
    every path into it; paths that disagree cannot be lowered"""
    taken = {
        label: frozenset(
            s.value.args[0].name for s in block.stmts
            if isinstance(s, Let) and isinstance(s.value, Call) and s.value.callee == "acquire")
        for label, block in blocks.items()}
    entry = next(iter(blocks))
    result: dict[str, frozenset[str]] = {entry: frozenset()}
    work = [entry]
    while work:
        label = work.pop()
        out = result[label] | taken[label]
        for target, _ in edges_of(program, blocks, blocks[label]):
            key = target if isinstance(target, str) else target.label
            known = result.get(key)
            if known is None:
                result[key] = out
                work.append(key)
            elif known != out:
                raise Unsupported(f"acquire() on some paths into {key} only")
    return {label: result.get(label, frozenset()) for label in blocks}


# The exits that leave an arm by a tail call of a cold routine (the miss
# adapter, process_error, the redirect and again dispatches)
COLD_EXITS = frozenset({"miss", "redirect", "again", "raise", "unwind"})


def expr_calls(program: Program, e) -> bool:
    """Whether an expression calls a helper row (a C call, or an inline row
    that may make one); primitives and language forms do not"""
    if isinstance(e, Call):
        x = program.externs.get(e.callee)
        if x is not None and x.kind is not RowKind.primitive:
            return True
        return any(expr_calls(program, a) for a in e.args)
    if isinstance(e, (tuple, list)):
        return any(expr_calls(program, a) for a in e)
    if isinstance(e, SpanLit):
        return expr_calls(program, e.items)
    return False


def block_calls(program: Program, block: Block) -> bool:
    """Whether a block may make a call before its terminator leaves it: a
    helper row, or a publication, whose retirements may deallocate"""
    for st in block.stmts:
        if isinstance(st, Publish):
            return True
        if isinstance(st, Let) and expr_calls(program, st.value):
            return True
        if isinstance(st, Guard) and expr_calls(program, st.cond):
            return True
    t = block.term
    if isinstance(t, CallTerm):
        x = program.externs.get(t.callee)
        return x is None or x.kind is not RowKind.primitive or expr_calls(program, t.args)
    if isinstance(t, Branch):
        return expr_calls(program, t.cond)
    if isinstance(t, Exit):
        return expr_calls(program, t.args)
    return False


def cold_region_heads(program: Program, blocks, skip: set[str]) -> set[str]:
    """Rule J5-R1: the blocks where a cold region begins.  A block is cold
    when every way out of it ends in a cold exit (COLD_EXITS).  A region
    head is a cold block with a predecessor outside the region, other than
    the entry, and counts only if the region makes a call: such a region is
    outlined (ArmLowerer.outline_cold), and a region without one costs the
    arm no more than its own cold tail call would."""
    succs = {label: [t.label for t, _ in edges_of(program, blocks, b)]
             for label, b in blocks.items()}
    cold: set[str] = set()
    changed = True
    while changed:
        changed = False
        for label, b in blocks.items():
            if label in cold or label in skip or label == "entry":
                continue
            t = b.term
            if isinstance(t, Exit):
                is_cold = t.kind in COLD_EXITS
            elif isinstance(t, (Goto, Branch, CallTerm)):
                x = program.externs.get(t.callee) if isinstance(t, CallTerm) else None
                is_cold = (not (x is not None and x.c is not None
                                and CONVENTIONS[x.c.convention].transfer)
                           and bool(succs[label])
                           and all(s in cold for s in succs[label]))
            else:
                is_cold = False
            if is_cold:
                cold.add(label)
                changed = True
    preds: dict[str, set[str]] = {label: set() for label in blocks}
    for label, ss in succs.items():
        for s_ in ss:
            preds[s_].add(label)
    heads = set()
    for label in cold:
        if not preds[label] <= cold:
            region, todo = set(), [label]
            while todo:
                lb = todo.pop()
                if lb in region:
                    continue
                region.add(lb)
                todo.extend(s_ for s_ in succs[lb] if s_ in cold)
            if any(block_calls(program, blocks[lb]) for lb in region):
                heads.add(label)
    return heads


def cold_case_heads(program: Program, blocks, skip: set[str]) -> set[str]:
    """Rule J5-R2: the blocks that only a row's `cold` case leads to.  The
    arm runs such a block, and every block that it reaches, out of line
    (ArmLowerer.outline_cold), as for a cold region: the rare path that
    rejoins the arm, such as an overflow that boxes its result, leaves by
    the outlined copy of the arm's rest, so the hot path keeps no frame or
    register for it.  As for a cold region, a block counts only if its
    region makes a call."""
    into: dict[str, list[bool]] = {label: [] for label in blocks}
    for b in blocks.values():
        t = b.term
        x = program.externs.get(t.callee) if isinstance(t, CallTerm) else None
        if x is not None and isinstance(x.result, Cases):
            cold_cases = {c.name for c in x.result.cases if c.cold}
            for e in t.edges:
                into[e.target.label].append(e.case in cold_cases)
            continue
        for target, _ in edges_of(program, blocks, b):
            into[target.label].append(False)
    succs = {label: [t.label for t, _ in edges_of(program, blocks, b)]
             for label, b in blocks.items()}
    heads = set()
    for label, marks in into.items():
        if label in skip or label == "entry" or not marks or not all(marks):
            continue
        region, todo = set(), [label]
        while todo:
            lb = todo.pop()
            if lb not in region:
                region.add(lb)
                todo.extend(succs[lb])
        if any(block_calls(program, blocks[lb]) for lb in region):
            heads.add(label)
    return heads


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
    # A span instruction's word count, from its counts and the constant
    # geometry of its layout (span_words())
    spans: str | None = None
    decls: list[str] = field(default_factory=list)
    body: list[str] = field(default_factory=list)
    # The arm's cold regions, each a function of its own (outline_cold)
    cold: list["ColdRegion"] = field(default_factory=list)
    # Rule HT-R1: the arm publishes its pc on entry, not at its escape
    # points (ArmLowerer.resolve_publish)
    publish_entry: bool = True


@dataclass
class ColdRegion:
    """A region of an arm, outlined: the function NAME runs it from its
    head, given the values that the arm computed before the head and the
    region reads, which travel in the activation (IR_OUTLINE_IN).  A cold
    region (J5) is rare; a declared one (`outline`) is hot."""
    name: str                               # the function, Cold_ or Out_<arm>_<head>
    head: str                               # the head's C label
    live: list[tuple[str, str]]             # (C type, local) of each value
    body: list[str]
    hot: bool = False


class ArmLowerer:
    def __init__(self, program: Program, arm: Arm, numbers: Mapping[str, int],
                 points: Mapping[str, int], generic: Mapping[str, str | None],
                 outline_cold: bool = True) -> None:
        self.program = program
        # An arm of the evaluator outlines its cold regions; a completion,
        # which returns to its caller, keeps them
        self.outline_cold_regions = outline_cold
        self.arm = arm
        self.info: FlatGraph = flatten(program, arm)
        self.family = program.families[arm.family]
        self.enc = program.encodings[arm.family]
        self.placements: dict[str, Placement] = {
            p.operand: p for p in self.enc.placements}
        # A home group's projections: g.p is the home at g's field plus i
        self.parts: dict[str, tuple[str, int]] = {
            f"{p.name}.{q}": (p.name, i)
            for p in self.family.params for i, q in enumerate(group_parts(p.type))}
        # The projections whose declared type admits a tagged index
        self.index_parts: set[str] = {
            f"{p.name}.{q.name}" for p in self.family.params
            for q in group_part_decls(p.type) if q.index}
        self.points = points
        self.generic = generic
        self.prefix = "rop_" + arm.name.replace(".", "_")
        self.blocks = dict(self.info.blocks)
        self.lines: list[str] = []
        self.decls: list[str] = []
        self.decl_list: list[Decl] = []
        self.declared: set[str] = set()
        self.homes: dict[str, str] = {}      # operand -> its home pointer
        self.views: dict[str, str] = {}      # operand -> value bound by use()
        self.temp = 0
        self.facts_in: dict[str, Facts] = {}
        self.targeted: set[str] = set()      # blocks reached by a goto
        self.next_label: str | None = None
        self.acquired: set[str] = set()      # operands whose owner acquire() took
        self.acquired_in: dict[str, frozenset[str]] = {}  # ... on every path into a block
        # Anchors made from a literal span, as scalars: name -> source count
        self.anchors: dict[str, int] = {}
        # The closes emitted last, which a dispatching exit takes as its
        # tail closes (lower_tail_closes); any other line emits them first
        self.pending_closes: list[str] = []
        self.tails = 0
        self.cold: list[ColdRegion] = []
        # A slot-class member arm (`over(locals: ...)`): each locals
        # operand's class, a constant, True for a local
        self.fixed_local: dict[str, bool] = fixed_locals(program, arm)
        # Rule HT-R1: the block being lowered, the blocks with an escape
        # point, and whether the arm publishes its pc on entry instead
        self.cur_label: str | None = None
        self.escaped: set[str] = set()
        self.sets_pc: set[str] = set()
        self.publish_entry = True

    # Names

    def local(self, name: str) -> str:
        if name in RESERVED:
            raise Unsupported(f"{self.arm.name}: the name {name} is reserved by the handler")
        return name

    def declare(self, name: str, ctype: str, init: str | None = None) -> None:
        if name in self.declared:
            return
        self.declared.add(name)
        self.decl_list.append(Decl(name, ctype))
        self.decls.append(f"{ctype} {name}" + (f" = {init}" if init else "") + ";")

    def new_temp(self, ctype: str, stem: str = "t") -> str:
        self.temp += 1
        name = f"{stem}{self.temp}"
        self.declare(name, ctype)
        return name

    def escape(self, sets_pc: bool = False) -> None:
        """Rule HT-R1: an escape point follows, one that may set the pc if
        sets_pc.  The first of a block is preceded by a publication of the
        pc, which resolve_publish keeps where some path into the block
        passed no escape point."""
        if self.cur_label is None:
            return
        if sets_pc:
            self.sets_pc.add(self.cur_label)
        if self.cur_label in self.escaped:
            return
        self.escaped.add(self.cur_label)
        self.lines.append(PUBLISH_MARK + self.cur_label)

    def emit(self, line: str) -> None:
        if self.pending_closes:
            # A close may deallocate, and a finalizer may read the pc
            self.escape()
        self.lines += [f"PyValue_XCLOSE({v});" for v in self.pending_closes]
        self.pending_closes = []
        self.lines.append(line)

    def close(self, value: str) -> None:
        """Close an owned value: at once, or as a tail close if a
        dispatching exit follows"""
        self.pending_closes.append(value)

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

    def is_home(self, name: str) -> bool:
        """An operand field or a projection of a home group"""
        return name in self.placements or name in self.parts

    def home_index(self, operand: str) -> str:
        """The frame index of an operand's home"""
        if operand in self.parts:
            group, i = self.parts[operand]
            return f"({self.field(group)} + {i})" if i else self.field(group)
        return self.field(operand)

    def home(self, operand: str) -> str:
        """The address of an operand's home, declared on first use"""
        if operand not in self.homes:
            name = "home_" + cname(operand)
            self.homes[operand] = name
            self.declare(name, "PyValue *const", f"&IR_SLOT({self.home_index(operand)})")
        return self.homes[operand]

    def is_vector(self, operand: str) -> bool:
        """An argument descriptor (Args): its homes follow in the payload
        words.  Other vector operands are spans (is_span_operand)."""
        p = self.placements.get(operand)
        return (p is not None and p.kind is FieldKind.vector
                and not self.is_span_operand(operand))

    def local_flag(self, operand: str) -> str | None:
        """The word's flag that marks the home a local's; None for a
        projection, which is always a temporary, and for an operand whose
        class the member arm fixes (is_local)"""
        if operand in self.parts or operand in self.fixed_local:
            return None
        return f"IR_F_LOCAL_{self.slot(operand).name}"

    def is_local(self, operand: str) -> bool | None:
        """Whether the operand's home is a local: a constant for a
        projection (never) and for an operand the member arm fixes, None
        where the word's flag says"""
        if operand in self.parts:
            return False
        return self.fixed_local.get(operand)

    def is_vacant(self, operand: str) -> bool:
        """Whether the member arm fixes the operand's home as a temporary:
        a destination temporary is vacant by construction
        (rop_check.check_contract), so it has no old value to close.  A
        projection is a temporary too, but not a vacant one."""
        return self.fixed_local.get(operand) is False

    def flag_test(self, operand: str) -> str:
        """The C condition that the operand's home is a local"""
        fixed = self.is_local(operand)
        if fixed is not None:
            return "1" if fixed else "0"
        return f"(ir_word & {self.local_flag(operand)})"

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
            if not isinstance(item, Name) or not self.is_home(item.name):
                raise Unsupported(f"{self.arm.name}: a span item {item} is not an operand")
            names.append(item.name)
        homes = ", ".join(self.home(n) for n in names)
        # Built where a row needs it, which is a cold exit: a named local
        # rather than an address-taken compound literal
        srcs = self.new_temp("_PyRopSrcs", "srcs")
        self.emit(f"{srcs} = (_PyRopSrcs){{frame, {len(names)}, "
                  f"{{{self.local_flags(names)}}}, {{{homes}}}}};")
        return f"&{srcs}"

    def local_flags(self, names: list[str]) -> str:
        return ", ".join(t if (t := self.flag_test(n)) in ("0", "1") else f"!!{t}"
                         for n in names)

    def anchor_scalars(self, name: str, span: SpanLit) -> None:
        """anchor_for_call over a literal span, as a view and an owner per
        source (_PyRop_AnchorForSrcs): a local's view is held by an owner of
        its own, a temporary's by its home"""
        for i, item in enumerate(span.items):
            if not isinstance(item, Name) or not self.is_home(item.name):
                raise Unsupported(f"{self.arm.name}: a span item {item} is not an operand")
            view, owned = f"{name}_view{i}", f"{name}_owned{i}"
            self.declare(view, "PyValueBorrow")
            self.declare(owned, "PyValue")
            self.emit(f"{view} = PyValue_Borrow(*{self.home(item.name)});")
            fixed = self.is_local(item.name)
            if fixed is not None:
                self.emit(f"{owned} = {f'PyValue_DUP({view})' if fixed else 'PyValue_NULL'};")
            else:
                self.emit(f"{owned} = {self.flag_test(item.name)} ? PyValue_DUP({view}) "
                          ": PyValue_NULL;")

    def anchor_aggregate(self, name: str) -> str:
        """A scalar anchor as the aggregate a row that consumes it takes"""
        n = self.anchors[name]
        whole = self.new_temp("_PyRopAnchors", f"{name}_all")
        views = ", ".join(f"{name}_view{i}" for i in range(n))
        owned = ", ".join(f"{name}_owned{i}" for i in range(n))
        self.emit(f"{whole} = (_PyRopAnchors){{{n}, {{{views}}}, {{{owned}}}}};")
        return f"&{whole}"

    def close_anchors(self, name: str) -> None:
        for i in range(self.anchors[name]):
            self.close(f"{name}_owned{i}")

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
        """A span operand: its count, the index of its first element among
        the elements of the word's spans, which follow in declaration order,
        and the layout's constant geometry: the elements in the head's
        trailing fields and the fixed operand words before the rest
        (pycore_ir.h, _PyIR_SpanElementAt())"""
        before = []
        for name, p in self.placements.items():
            if name == operand:
                break
            if self.is_span_operand(name):
                before.append(self.field(name))
        first = " + ".join(before) or "0"
        span = self.new_temp("_PyRopSpan", "span")
        self.emit(f"{span} = (_PyRopSpan){{frame, this_instr, {first}, "
                  f"{self.field(operand)}, {len(self.enc.elements)}, {self.enc.words}}};")
        return f"&{span}"

    def row_symbol(self, x: Extern) -> str:
        assert x.c is not None
        return x.c.symbol

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
            elif (isinstance(t, HandleType) and t.handle is Handle.Anchors
                  and isinstance(a, Name) and a.name in self.anchors):
                out.append(self.anchor_aggregate(a.name))
            elif isinstance(t, HandleType) and t.handle is Handle.Anchors:
                out.append("&" + self.expr_text(a))
            elif (t in (HandleType(Handle.Src), HandleType(Handle.Local),
                        HandleType(Handle.Cell),
                        WrapType(Wrap.Maybe, HandleType(Handle.Src)))
                    and isinstance(a, Name) and self.is_home(a.name)):
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
            if e.name in self.parts:
                return self.home(e.name)
            if e.name in self.placements:
                return self.operand_value(e.name)
            if e.name in ENVIRONMENT_C:
                return ENVIRONMENT_C[e.name]
            if e.name not in self.info.types:
                member = enum_of_member(self.program.enums, e.name)
                if member is not None:
                    return enum_c_name(*member)
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
        if p.kind is FieldKind.flag:
            return f"(((uint32_t)(ir_word) >> {p.shift}) & 1u)"
        if not p.slots or any(s.word != 0 for s in p.slots):
            raise Unsupported(f"{self.arm.name}: immediate {operand} is not in the head")
        if p.shared:
            field = f"((uint32_t)IR_{p.slots[0].name}(ir_word) >> {p.shift})"
            return f"({field} & {(1 << p.bits) - 1:#x})"
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
        if p.kind in (FieldKind.imm, FieldKind.flag):
            return self.immediate(operand)
        if p.kind is FieldKind.name:
            return f"_PyRop_NameAt(frame, {self.field(operand)})"
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
        head, whether or not the family has operand words"""
        p = self.placements[operand]
        first = p.slots[0]
        assert [s.offset for s in p.slots] == [first.offset + 12 * i for i in range(len(p.slots))]
        # Shift the field's top bit to the word's, then shift arithmetically
        # down: two instructions decode and sign-extend it
        top = first.offset + p.bits
        assert top <= 64
        field = (f"Py_ARITHMETIC_RIGHT_SHIFT(int64_t, (int64_t)(ir_word << {64 - top}), "
                 f"{64 - p.bits})")
        return f"(this_instr + 1 + {field})"

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
            fixed = self.is_local(arg.name)
            if fixed is not None:
                return f"PyValue_DUP(*{home})" if fixed else f"_PyFrame_TakeHome(frame, {home})"
            return (f"({self.flag_test(arg.name)} ? PyValue_DUP(*{home}) "
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
            members = self.program.member_sets.get(arm.name)
            if members is None:
                return "ROP_HANDLER_" + arm.name.replace(".", "_")
            # The member arm for this word's operand, which a word of another
            # family that installs the arm (AppendText's Binary arms) holds too
            mine = next((p.type for p in self.family.params if p.name == members.operand),
                        None)
            if mine != EnumType(members.enum):
                raise Unsupported(f"{self.arm.name}: candidate({arm.name}) needs the "
                                  f"operand {members.operand}:Enum<{members.enum}>")
            table = "_PyRop_Members_" + arm.name.replace(".", "_")
            return f"{table}[{self.operand_value(members.operand)}]"
        x = self.program.externs.get(e.callee)
        if x is None or x.c is None:
            raise Unsupported(f"{self.arm.name}: {e.callee} has no C body (pending)")
        if (e.callee == "arg" and isinstance(e.args[0], Name)
                and e.args[0].name in self.anchors):
            # The view the scalar anchor recorded
            index = e.args[1]
            if not isinstance(index, Nat):
                raise Unsupported(f"{self.arm.name}: arg() of a computed index")
            return f"{e.args[0].name}_view{index.value}"
        conv = CONVENTIONS[x.c.convention]
        if not conv.single:
            raise Unsupported(f"{self.arm.name}: {e.callee} has cases; use a call")
        if row_reads_pc(x):
            self.escape(row_sets_pc(x))
        return f"{self.row_symbol(x)}({', '.join(self.c_args(e.args, x.params))})"

    # Statements

    def lower(self) -> Lowering:
        self.facts_in = arm_facts(self.program, self.info)
        self.views = home_views(self.info)
        # A transfer row's callee performs the exits of its edges' blocks
        self.transfer_blocks = {
            e.target.label for b in self.blocks.values()
            if isinstance(b.term, CallTerm) and is_transfer(self.program, b.term.callee)
            for e in b.term.edges}
        self.acquired_in = acquired_on_entry(self.program, self.blocks)
        self.anchors = {
            st.name: len(st.value.args[0].items)
            for b in self.blocks.values() for st in b.stmts if is_span_anchor(st)}
        order = [label for label in self.layout() if label not in self.transfer_blocks]
        # The declared regions (`outline`) and the cold ones
        self.outline_heads = (
            {label for label, b in self.blocks.items() if b.outline}
            if self.outline_cold_regions else set())
        self.cold_heads = self.outline_heads | (
            cold_region_heads(self.program, self.blocks, self.transfer_blocks)
            | cold_case_heads(self.program, self.blocks, self.transfer_blocks)
            if self.outline_cold_regions else set())
        spans: dict[str, tuple[int, int]] = {}
        if self.fixed_local:
            # The member's word: an install that bypassed the generated row
            # (_PyIR_LocalsArm) fails here in a build with assert() on
            flags = [f"IR_F_LOCAL_{self.slot(n).name}" for n in self.fixed_local]
            bits = [f"IR_F_LOCAL_{self.slot(n).name}" for n, l in self.fixed_local.items() if l]
            self.emit(f"assert((ir_word & ({' | '.join(flags)})) == "
                      f"({' | '.join(bits) or '0'}));")
        for i, label in enumerate(order):
            self.next_label = order[i + 1] if i + 1 < len(order) else None
            start = len(self.lines)
            self.lower_block(self.blocks[label], label)
            spans[label] = (start, len(self.lines))
        assert not self.pending_closes, self.arm.name
        for b in self.blocks.values():
            for p in b.params:
                t = c_type(p.type)
                if t is not None:
                    self.declare(self.local(p.name), t)
        self.resolve_publish()
        if self.cold_heads:
            self.lines = self.outline_cold(order, spans)
        for c in self.cold:
            c.body = [line for line in c.body if line != ""]
        body = []
        for line in self.lines:
            if line != "":
                body.append(line)
        return Lowering(self.arm.name, self.enc.words, self.payload_words(),
                        self.span_words(),
                        self.decls,
                        self.drop_unused_labels(body),
                        self.cold, self.publish_entry)

    def resolve_publish(self) -> None:
        """Rule HT-R1: the frame's pc (instr_ptr) is published at the arm's
        escape points, not on entry.  A block that every path reaches after
        an escape point does not publish; one that some path reaches without
        one publishes before its first, unless another path into it passed
        a row that may have set the pc, which a second store would
        overwrite: then the arm publishes on entry, as before.  A
        completion runs with the pc at its site already.  The marks become
        IR_PUBLISH_IP() or empty lines, which keeps the block spans of
        outline_cold."""
        succs = {label: [t.label for t, _ in edges_of(self.program, self.blocks, b)]
                 for label, b in self.blocks.items()}
        preds: dict[str, list[str]] = {label: [] for label in self.blocks}
        for label, ss in succs.items():
            for s_ in ss:
                preds[s_].append(label)
        reach, todo = set(), ["entry"]
        while todo:
            label = todo.pop()
            if label not in reach:
                reach.add(label)
                todo.extend(succs[label])
        # sets: some path into the block passed a row that may set the pc;
        # must: every path passed an escape point
        sets = {label: False for label in self.blocks}
        must = {label: label != "entry" for label in self.blocks}
        changed = True
        while changed:
            changed = False
            for label in reach - {"entry"}:
                ps = [p for p in preds[label] if p in reach]
                m = any(sets[p] or p in self.sets_pc for p in ps)
                n = bool(ps) and all(must[p] or p in self.escaped for p in ps)
                if (m, n) != (sets[label], must[label]):
                    sets[label], must[label] = m, n
                    changed = True
        mixed = any(sets[b] and not must[b] for b in self.escaped if b in reach)
        self.publish_entry = mixed or not self.outline_cold_regions
        for i, line in enumerate(self.lines):
            if line.startswith(PUBLISH_MARK):
                keep = not self.publish_entry and not must[line[len(PUBLISH_MARK):]]
                self.lines[i] = "IR_PUBLISH_IP();" if keep else ""

    def outline_cold(self, order: list[str], spans: Mapping[str, tuple[int, int]]) -> list[str]:
        """Rule J5-R1: each cold region (cold_region_heads), and each
        declared one (`outline`), becomes a function of the evaluator's
        type, which the arm tail-calls at the region's head
        (IR_OUTLINE_GOTO, IR_OUTLINE_HOT_GOTO): the arm's hot path keeps no
        copy of it, and a stencil of the baseline JIT reaches the one in the
        binary.  The function is the arm's own text from the head: the
        arm's declarations, the region's blocks, and the locals that the
        code before the head set and the region reads, which it leaves in
        the activation.  It decodes the word again, from this_instr.  A
        region that reaches another region's head tail-calls that one's
        function there, as the arm does."""
        succs = {label: [t.label for t, _ in edges_of(self.program, self.blocks, b)]
                 for label, b in self.blocks.items()}
        heads = self.cold_heads

        def reach(start: str, stop: set[str]) -> set[str]:
            seen: set[str] = set()
            todo = [start]
            while todo:
                label = todo.pop()
                if label in seen:
                    continue
                seen.add(label)
                if label not in stop:
                    todo.extend(succs[label])
            return seen

        def text(label: str) -> list[str]:
            start, end = spans[label]
            return self.lines[start:end]

        first = spans[order[0]][0]
        prelude = self.lines[:first]
        types = {d.name: d.ctype for d in self.decl_list}
        functions: dict[str, ColdRegion] = {}

        def name_of(label: str) -> str:
            kind = "Out" if label in self.outline_heads else "Cold"
            return f"{kind}_{self.prefix.removeprefix('rop_')}_{label}"

        def emit(start: str | None) -> list[str]:
            """The text that runs from start (the arm's entry, None, or a
            head), with a tail call at every other head it reaches"""
            stop = heads - {start} if start is not None else heads
            region = reach(start or "entry", stop)
            lines: list[str] = []
            for label in order:
                if label not in region:
                    continue
                if label not in stop:
                    lines += text(label)
                    continue
                enter(label)
                c = functions[label]
                # The lines that a fallthrough into the head ran stay
                head_lines = text(label)
                marker = f"{self.label(label)}:"
                at = head_lines.index(marker) if marker in head_lines else -1
                lines += head_lines[:at + 1]
                if c.live:
                    lines.append(f"{{ rop_in_{c.name} *ir_in = IR_OUTLINE_IN(rop_in_{c.name});")
                    lines += [f"  ir_in->{n} = {n};" for _, n in c.live]
                    lines.append("}")
                goto = "IR_OUTLINE_HOT_GOTO" if c.hot else "IR_OUTLINE_GOTO"
                lines.append(f"{goto}({c.name});")
            return lines

        def enter(label: str) -> None:
            if label in functions:
                return
            whole = reach(label, set())
            needed = live_in(label, whole, succs, {lb: text(lb) for lb in whole},
                             self.label)
            # Set before the head: in the code of the blocks outside its region
            outside = set(c_names(prelude + [
                line for lb in order if lb not in whole for line in text(lb)]))
            live = [(types[n], n) for n in types
                    if n in needed and n in outside and not types[n].endswith("const")]
            c = ColdRegion(name_of(label), self.label(label), live, [],
                           hot=label in self.outline_heads)
            functions[label] = c
            body = ([] if label == "entry" else [f"goto {self.label(label)};"]) + emit(label)
            c.body = self.drop_unused_labels(body)

        arm_lines = prelude + emit(None)
        self.cold = [functions[label] for label in order if label in functions]
        return arm_lines

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

    def span_words(self) -> str | None:
        """The words of a span instruction: the head, its fixed operand
        words, and the elements past the head's free fields, four to a word.
        The head holds at most three elements, so the dividend is never
        negative."""
        counts = [self.field(n) for n in self.placements if self.is_span_operand(n)]
        if not counts:
            return None
        nhead = len(self.enc.elements)
        assert nhead <= 3, self.arm.name
        total = " + ".join(counts)
        return (f"{1 + self.enc.words} + ({total} + {3 - nhead}) / "
                "IR_OPERANDS_PER_WORD")

    def drop_unused_labels(self, lines: list[str]) -> list[str]:
        text = "\n".join(lines)
        out = []
        for line in lines:
            if line.endswith(":") and not line.startswith(" "):
                if (f"goto {line[:-1]};" not in text
                        and f", {line[:-1]});" not in text):
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
        self.cur_label = label
        if label != "entry":
            self.emit(f"{self.label(label)}:")
        # An acquire() takes the owner on its own path only
        self.acquired = set(self.acquired_in[label])
        for i, s in enumerate(block.stmts):
            self.current_facts = facts_at(self.program, self.facts_in[label], block, i)
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
        if is_span_anchor(s):
            self.anchor_scalars(self.local(s.name), s.value.args[0])
            return
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
        operands = {p.name: p.type for p in self.family.params}
        s = replace(s, debts=expand_debts(s.debts, operands))
        order = [h for name in self.placements for h in expand_group(name, operands)]
        spent = sorted((d for d in s.debts if d.kind is DebtKind.spent),
                       key=lambda d: order.index(d.subject))
        olds = [d for d in s.debts if d.kind is DebtKind.old]
        # d[] = v: the items of v are written straight into d's homes with
        # the arm's constant span geometry, and no displaced value is
        # deallocated before the last write (_PyRop_PublishItems), which
        # discharges old(d) too; the debts after it close what it read from
        stored = {b.target: b.value.name for b in s.bindings if b.owners}
        vectors = [b for b in s.bindings if b.vector and not b.owners]
        moved = {b.value.name for b in vectors if isinstance(b.value, Name)}
        owners: dict[str, str | None] = {}
        for b in vectors:
            # A vector binding's runtime moves the pairs in order: the
            # lowering ordered them so that each destination is vacant
            # (_PyRop_MoveInOrder)
            if isinstance(b.value, Call):
                (source,) = b.value.args
                self.emit(f"_PyRop_CopyHomes({self.span(b.target)}, {self.span(source.name)});")
            else:
                self.emit(f"_PyRop_MoveInOrder({self.span(b.target)}, "
                          f"{self.span(b.value.name)});")
        for d in spent:
            if d.subject in moved:
                continue
            if self.is_span_operand(d.subject):
                # A span's homes close together, after every home is cleared
                # (_PyRop_CloseSpan)
                owners[d.subject] = None
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
            if self.is_local(d.subject):
                # A local keeps its value: nothing to clear or close
                owners[d.subject] = None
                continue
            home, flag = self.home(d.subject), self.local_flag(d.subject)
            view = self.views.get(d.subject)
            if view is not None and facts.uncounted(view):
                # A value that owns no count: clearing the home retires it
                owners[d.subject] = None
                self.emit_temporary(flag, f"_PyFrame_WriteHome(frame, {home}, PyValue_NULL);")
                continue
            owner = "own_" + cname(d.subject)
            self.declare(owner, "PyValue", "PyValue_NULL")
            owners[d.subject] = owner
            self.emit_temporary(flag, f"{owner} = _PyFrame_TakeHome(frame, {home});")
        for d in olds:
            if d.subject in stored:
                continue
            if self.is_vacant(d.subject):
                self.emit(f"assert(PyValue_IsNull(*{self.home(d.subject)}));")
                continue
            self.declare("old_" + cname(d.subject), "PyValue")
            self.emit(f"old_{cname(d.subject)} = *{self.home(d.subject)};")
        for target, value in stored.items():
            # The closes may run finalizers, which a stub reports
            self.escape()
            self.emit("IR_STUB_ESCAPE();")
            self.emit(f"_PyRop_PublishItems({self.span(target)}, &{value});")
        for b in s.bindings:
            if b.vector:
                continue
            home = self.home(b.target)
            if isinstance(b.value, Null):
                self.emit(f"_PyFrame_WriteHome(frame, {home}, PyValue_NULL);")
                continue
            self.emit(f"_PyFrame_WriteHome(frame, {home}, {self.expr(b.value)});")
            check = "Temp" if b.target in self.index_parts else "Home"
            self.emit(f"_PyValue_Check{check}(*{home});")
        scalar = [b for b in s.bindings if not b.vector]
        kept = self.home(scalar[0].target) if len(s.bindings) == 1 and scalar else "NULL"
        for d in s.debts:
            if d.kind is DebtKind.spent and d.subject in moved:
                raise Unsupported(f"{self.arm.name}: {d.subject} is moved and spent")
            elif d.kind is DebtKind.owned:
                self.close(self.expr_text(Name(d.subject)))
            elif d.kind is DebtKind.spent and self.is_span_operand(d.subject):
                # The closes may run finalizers, which a stub reports
                self.escape()
                self.emit("IR_STUB_ESCAPE();")
                self.emit(f"_PyRop_CloseSpan({self.span(d.subject)});")
            elif d.kind is DebtKind.spent and self.is_vector(d.subject):
                self.escape()
                self.emit(f"_PyRop_SpendArgs(frame, this_instr, {kept});")
            elif d.kind is DebtKind.spent and owners[d.subject] is not None:
                self.close(owners[d.subject])
            elif d.kind is DebtKind.old and d.subject in stored:
                pass
            elif d.kind is DebtKind.old and self.is_vacant(d.subject):
                pass
            elif d.kind is DebtKind.old:
                self.close(f"old_{cname(d.subject)}")
            elif d.kind is DebtKind.roots and d.subject in self.anchors:
                self.close_anchors(d.subject)
            elif d.kind is DebtKind.roots:
                self.escape()
                self.emit(f"_PyRop_CloseAnchors(&{d.subject});")

    def emit_temporary(self, flag: str | None, line: str) -> None:
        """A statement for a home that is a temporary: a projection always
        is; an operand is unless the word marks it a local's"""
        if flag is None:
            self.emit(line)
            return
        self.emit(f"if (!(ir_word & {flag})) {{")
        self.emit(f"    {line}")
        self.emit("}")

    # Terminators

    def lower_term(self, block, label: str) -> None:
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
        if any(isinstance(a, Name) and a.name in self.anchors for a in target.args):
            raise Unsupported(f"{self.arm.name}: a scalar anchor passed to a block")
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
        if row_reads_pc(x):
            self.escape(row_sets_pc(x))
        if conv.transfer:
            # The callee leaves the word by the exit of the case it takes
            self.emit(f"{self.row_symbol(x)}({', '.join(args)});")
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
        ret = f"{self.row_symbol(x)}({', '.join(args)})"
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
        if row_reads_pc(x):
            self.escape(row_sets_pc(x))
        if x.contract.effect_set & ESCAPING:
            self.emit("IR_STUB_ESCAPE();")
        ret = self.new_temp("int", "ret")
        self.emit(f"{ret} = {self.row_symbol(x)}({', '.join(args)});")
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
        if t.kind in PC_EXITS:
            self.escape(True)
        if t.kind in ("next", "branch", "jump") and self.pending_closes:
            self.lower_tail_closes(t)
        elif t.kind == "next":
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
        elif t.kind in ("raise", "unwind"):
            point = t.args[0]
            assert isinstance(point, Name)
            ingress = t.kind.upper()
            origin = ("frame->instr_ptr"
                      if self.program.points[point.name].pc is PcRole.left
                      else "this_instr")
            self.emit(f"PROCESS_ERROR_AT({ingress}, {self.points[point.name]}, {origin});")
        elif t.kind == "enter":
            self.emit(f"ROP_ENTER({self.expr(t.args[0])}, {self.resume_slot(t)});")
        elif t.kind == "yield":
            macro = "ROP_YIELD_FROM" if self.arm.family in DELEGATING_FAMILIES else "ROP_YIELD"
            self.emit(f"{macro}({self.expr(t.args[0])}, {self.resume_slot(t)});")
        elif t.kind == "return":
            caller, head = (self.expr(a) for a in t.args)
            self.emit(f"ROP_RETURN({caller}, {head});")
        elif t.kind == "exit":
            self.emit(f"IR_EXIT({self.expr(t.args[0])});")
        else:
            raise Unsupported(f"{self.arm.name}: exit {t.kind} is not lowered yet")

    def lower_tail_closes(self, t: Exit) -> None:
        """Rule LR 1: the closes before a dispatching exit leave the arm
        by a tail call.  Each close drops its count inline
        (ROP_TAIL_CLOSE); a value that would lose its last count keeps it
        and goes to a cold block, which parks it and the last count of every
        later close in the activation and leaves through the VM routine
        close_dispatch, which closes them in order and dispatches.  So the
        arm makes no call on any path to its exit, and finalizers run after
        its last decrement instead of between two.  The bindings
        (Python/ceval_macros.h) say what the macros do."""
        closes, self.pending_closes = self.pending_closes, []
        if len(closes) > TAIL_CLOSES:
            raise ValueError(f"{self.arm.name}: {len(closes)} tail closes, more "
                             f"than the activation parks ({TAIL_CLOSES})")
        tail = f"{self.prefix}_Tail{self.tails}"
        self.tails += 1
        for k, v in enumerate(closes):
            self.emit(f"ROP_TAIL_CLOSE({v}, {tail}_{k});")
        if t.kind == "next":
            self.emit("DISPATCH();")
            leave = "ROP_TAIL_DISPATCH();"
        else:
            (target,) = t.args
            assert isinstance(target, Name)
            value = self.operand_value(target.name)
            self.emit(f"IR_BRANCH({value});")
            leave = f"ROP_TAIL_BRANCH({value});"
        # Cold block k parks close k's value, then runs the later closes
        # in parking mode (Rest k + 1 onwards) and leaves
        n = len(closes)
        # A park leaves for close_dispatch, whose deallocations may read the
        # pc: each publishes it unless an escape point came before
        first = self.cur_label is not None and self.cur_label not in self.escaped
        for k, v in enumerate(closes):
            self.emit(f"{tail}_{k}:")
            if first:
                self.lines.append(PUBLISH_MARK + self.cur_label)
            self.emit(f"ROP_TAIL_PARK_FIRST({v});")
            if n > 1:
                self.emit(f"goto {tail}_Rest{k + 1};")
        for j in range(1, n + 1):
            if n > 1:
                self.emit(f"{tail}_Rest{j}:")
            self.emit(f"ROP_TAIL_PARK({closes[j]});" if j < n else leave)
        if first:
            self.escaped.add(self.cur_label)

    def resume_slot(self, t: Exit) -> str:
        """The home that the child's return value, or the value sent to a
        yielding generator, goes to: the resume point's definition"""
        assert t.resume is not None
        defs = self.program.points[t.resume].defs
        if not defs:
            raise Unsupported(f"{self.arm.name}: a resume point with no definition")
        (dst,) = defs
        return self.home_index(dst)


# The values that an arm's tail closes may park in the activation
# (_PyEvalActivation.tail_close, Include/internal/pycore_ceval.h)
TAIL_CLOSES = 8

# The families whose yield suspends in the delegating state
DELEGATING_FAMILIES = frozenset({"YieldFrom"})


def is_span_anchor(s) -> bool:
    """A let of anchors made from a literal span of sources, which lowers
    to scalars (ArmLowerer.anchor_scalars)"""
    return (isinstance(s, Let) and s.type == HandleType(Handle.Anchors)
            and isinstance(s.value, Call) and len(s.value.args) == 1
            and isinstance(s.value.args[0], SpanLit))


@dataclass
class Decl:
    name: str
    ctype: str


def c_names(lines: list[str]) -> list[str]:
    """The identifiers in some lines of C"""
    return re.findall(r"\b[A-Za-z_]\w*\b", "\n".join(lines))


def block_uses_defs(lines: list[str]) -> tuple[set[str], set[str], dict[str, set[str]]]:
    """For a block's C: the names it reads before it sets them; the names
    it sets on every path through it (an assignment at its top level,
    before any label of its own); and for each label that a goto in a
    braced group leaves for, the names the group set before the goto (a
    jump's payload)"""
    uses: set[str] = set()
    defs: set[str] = set()
    edge: dict[str, set[str]] = {}
    group: list[str] = []
    depth = 0
    straight = True
    for line in lines[1:] if lines and lines[0].endswith(":") else lines:
        text = line.strip()
        if depth == 0 and text.endswith(":"):
            straight = False
        m = re.match(r"([A-Za-z_]\w*) = (.*);$", text)
        g = re.match(r"goto ([A-Za-z_]\w*);$", text)
        if m:
            uses |= {n for n in c_names([m.group(2)]) if n not in defs and n not in group}
            if depth == 0 and straight:
                defs.add(m.group(1))
            else:
                group.append(m.group(1))
        elif g:
            edge.setdefault(g.group(1), set()).update(group if depth else ())
        else:
            uses |= {n for n in c_names([text]) if n not in defs and n not in group}
        if text.endswith("{") or text == "}":
            group = []
        depth += text.count("{") - text.count("}")
    return uses, defs, edge


def live_in(head: str, region: set[str], succs: Mapping[str, list[str]],
            texts: Mapping[str, list[str]], label) -> set[str]:
    """The names that some path from head reads before it sets them"""
    ud = {lb: block_uses_defs(texts[lb]) for lb in region}
    live = {lb: set(ud[lb][0]) for lb in region}
    changed = True
    while changed:
        changed = False
        for lb in region:
            uses, defs, edge = ud[lb]
            out: set[str] = set()
            for s_ in succs[lb]:
                if s_ in region:
                    out |= live[s_] - edge.get(label(s_), set())
            new = uses | (out - defs)
            if new != live[lb]:
                live[lb] = new
                changed = True
    return live[head]


def cname(operand: str) -> str:
    """A C identifier part for an operand or a projection g.p"""
    return operand.replace(".", "_")


def lower_arm(program: Program, arm: Arm, numbers: Mapping[str, int],
              points: Mapping[str, int], generic: Mapping[str, str | None],
              outline_cold: bool = True) -> Lowering:
    return ArmLowerer(program, arm, numbers, points, generic, outline_cold).lower()
