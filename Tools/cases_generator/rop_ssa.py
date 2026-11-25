"""The tracer's SSA expansion of register instruction arms.

`expand(program, arm)` turns a checked arm (rop_check.Program) into an
`SsaArm`: blocks with parameters over single-assignment values that carry
the language's types.  It covers what a tracer needs from an arm:

- a guard is a side exit, with the frame state at the exit (the entry
  state, by the restart rule);
- a call to a row keeps the row's declared effects, feedback and parameter
  modes, and a row with cases is a `Switch` with one edge per case;
- `publish` is a sequence of home writes, a checkpoint and the releases in
  the declared retire order (change 21);
- every exit names its frame state and the tokens it discharges.

Nothing is inferred.  Where a declaration does not say what an arm does to
its homes, `expand` raises `SsaError` with a diagnostic, and the arm does
not expand.

The leaf IR (ir_leaf.py) is the templates' lowering target and stays as it
is; the evaluator (rop_ssa_eval.py) runs the leaf IR's primitive specs for
the primitive rows that have them.
"""

import enum
from dataclasses import dataclass, field

from rop_catalog import ENVIRONMENT, DebtKind, Scalar, ExcRole, FieldKind, FrameRole, Handle, PcRole, Wrap
from rop_cgen import Unsupported, flatten
from rop_check import (CANDIDATE, DEST_HANDLES, PRIMITIVES, UNWRAP, Program,
                       field_kind)
from rop_model import (Arm, BoolLit, Branch, Call, CallTerm, Cases, Diagnostic,
                       Each, EnumType, Exit, Expr, Goto, Guard, HandleType, Let, Name, Nat,
                       Publish, ScalarType, SpanLit, SrcSpan, Tail, Type,
                       WrapType, DISCARD)


class SsaError(Exception):
    "An arm the declarations do not let the expansion express"

    def __init__(self, diagnostic: Diagnostic) -> None:
        super().__init__(f"{diagnostic.span.filename}:{diagnostic.span.line}:"
                         f"{diagnostic.span.column}: {diagnostic.definition}: "
                         f"{diagnostic.message}")
        self.diagnostic = diagnostic


class OpKind(enum.Enum):
    read = "read"               # attr: home operand; the value it holds, borrowed
    home = "home"               # attr: home operand; the home itself (a row argument)
    operand = "operand"         # attr: immediate, name, constant, feedback or target operand
    env = "env"                 # attr: thread, activation, frame, site, next_head
    const = "const"             # attr: an int, a bool or an enum member
    span = "span"               # args: the items of a span literal
    prim = "prim"               # attr: a primitive row; pure
    call = "call"               # attr: a `-> T` helper row
    dup = "dup"                 # a new owner of a borrowed value
    acquire = "acquire"         # attr: home; the owner a dying source holds
    unwrap = "unwrap"
    candidate = "candidate"     # attr: the arm a candidate names
    guard = "guard"             # args: (cond,); attr: Guarded, a side exit
    take = "take"               # attr: home; a spent temporary's owner out of its home
    old = "old"                 # attr: home; the displaced owner of a destination
    store = "store"             # attr: home; args: (owner,)
    checkpoint = "checkpoint"   # attr: the point the frame state now matches
    release = "release"         # attr: DebtKind; args: (owner or anchors,)


@dataclass(frozen=True)
class HomeState:
    """A home at an exit: intact (as on entry), spent (a temporary emptied,
    a local left), defined(value) or held(value: an owner the checkpoint
    carries)"""
    state: str
    value: int | None = None


INTACT = HomeState("intact")
# The checkpoint after a join of paths that published different points
JOINED = "<joined>"


@dataclass(frozen=True)
class FrameState:
    "What a tracer writes back at an exit"
    frame: str                       # a FrameRole, or "value" for a computed frame
    pc: str                          # a PcRole, or "value" for a computed head
    exc: str
    homes: tuple[tuple[str, HomeState], ...]
    point: str | None = None
    discharged: tuple[int, ...] = () # the tokens the exit takes


@dataclass(frozen=True)
class Guarded:
    "The side exit of a guard: facts its true edge grants, and its state"
    facts: tuple[str, ...]
    state: FrameState


@dataclass(frozen=True)
class Op:
    kind: OpKind
    result: int | None
    args: tuple[int, ...] = ()
    attr: object = None


@dataclass(frozen=True)
class Jump:
    block: str
    args: tuple[int, ...]


@dataclass(frozen=True)
class BranchTerm:
    cond: int
    if_true: Jump
    if_false: Jump


@dataclass(frozen=True)
class CaseEdge:
    """An outcome of a row: its payload binds the target's first
    parameters, then args"""
    case: str
    payload: tuple[int, ...]
    target: Jump
    facts: tuple[str, ...] = ()


@dataclass(frozen=True)
class Switch:
    row: str
    args: tuple[int, ...]
    edges: tuple[CaseEdge, ...]


@dataclass(frozen=True)
class ExitTerm:
    kind: str
    args: tuple[int, ...]
    state: FrameState


Term = Jump | BranchTerm | Switch | ExitTerm


@dataclass
class SsaBlock:
    label: str
    params: tuple[int, ...]
    ops: list[Op] = field(default_factory=list)
    term: Term | None = None


@dataclass
class SsaArm:
    name: str
    family: str
    region: str
    operands: dict[str, FieldKind]
    blocks: dict[str, SsaBlock]
    types: dict[int, Type]
    names: dict[int, str]           # the language name of each value, for dumps

    @property
    def entry(self) -> SsaBlock:
        return next(iter(self.blocks.values()))

    def exits(self) -> list[ExitTerm]:
        return [b.term for b in self.blocks.values() if isinstance(b.term, ExitTerm)]

    def guards(self) -> list[Op]:
        return [op for b in self.blocks.values() for op in b.ops if op.kind is OpKind.guard]


def is_dest_param(t: Type) -> bool:
    "A row parameter that names a home the row may write"
    if isinstance(t, WrapType) and t.wrap in (Wrap.Maybe, Wrap.Span):
        return is_dest_param(t.arg)
    return isinstance(t, HandleType) and t.handle in DEST_HANDLES


O_TYPE = HandleType(Handle.O)
SIZE = ScalarType(Scalar.Size)
BOOL = ScalarType(Scalar.Bool)


class Expander:
    def __init__(self, program: Program, arm: Arm) -> None:
        self.program = program
        self.arm = arm
        self.family = program.families[arm.family]
        try:
            self.flat = flatten(program, arm)
        except Unsupported as e:
            self.fail(arm.span, str(e))
        self.types: dict[int, Type] = {}
        self.names: dict[int, str] = {}
        self.env: dict[str, int] = {}
        self.operands = {p.name: field_kind(p.type) for p in self.family.params}
        self.blocks: dict[str, SsaBlock] = {}
        self.cur: SsaBlock
        # The homes and the last checkpoint on entry to each block
        self.states: dict[str, tuple[dict[str, HomeState], str | None]] = {}
        self.homes: dict[str, HomeState] = {}
        self.point: str | None = None
        self.acquired: set[str] = set()
        self.count = 0

    def fail(self, span: SrcSpan, message: str):
        raise SsaError(Diagnostic(span, self.arm.name, message))

    def value(self, t: Type, name: str = "") -> int:
        v = self.count
        self.count += 1
        self.types[v] = t
        self.names[v] = name
        return v

    def emit(self, kind: OpKind, t: Type | None, args=(), attr=None, name="") -> int | None:
        v = self.value(t, name) if t is not None else None
        self.cur.ops.append(Op(kind, v, tuple(args), attr))
        return v

    # Expressions

    def expr(self, e: Expr, name: str = "") -> int:
        if isinstance(e, Name):
            return self.name(e)
        if isinstance(e, Nat):
            return self.emit(OpKind.const, SIZE, (), e.value)
        if isinstance(e, BoolLit):
            return self.emit(OpKind.const, BOOL, (), e.value)
        if isinstance(e, SpanLit):
            items = [self.expr(i) for i in e.items]
            return self.emit(OpKind.span, WrapType(Wrap.Span, self.types[items[0]])
                             if items else None, items)
        assert isinstance(e, Call)
        return self.call(e, name)

    def name(self, e: Name) -> int:
        n = e.name
        if n in self.env:
            return self.env[n]
        kind = self.operands.get(n)
        param = next((p for p in self.family.params if p.name == n), None)
        if param is not None:
            op = OpKind.home if kind in (FieldKind.home, FieldKind.vector) else OpKind.operand
            v = self.emit(op, param.type, (), n, n)
            self.env[n] = v
            return v
        if n in ENVIRONMENT:
            v = self.emit(OpKind.env, HandleType(ENVIRONMENT[n]), (), n, n)
            self.env[n] = v
            return v
        member = self.enum_member(n)
        if member is not None:
            return self.emit(OpKind.const, member, (), n, n)
        self.fail(e.span, f"{n} names no value the expansion knows")

    def enum_member(self, n: str) -> Type | None:
        enum_name, _, member = n.rpartition(".")
        for e in self.program.enums.values():
            if (member if enum_name else n) in e.members and enum_name in ("", e.name):
                return EnumType(e.name)
        return None

    def call(self, e: Call, name: str) -> int:
        c = e.callee
        if c == "use":
            (home,) = e.args
            return self.emit(OpKind.read, HandleType(Handle.B), (), home.name, name)
        if c == "acquire":
            (home,) = e.args
            self.acquired.add(home.name)
            return self.emit(OpKind.acquire, O_TYPE, (), home.name, name)
        if c in ("dup", "move"):
            (arg,) = e.args
            if c == "move":
                self.fail(e.span, "move of an InOut home: no pilot arm uses it")
            return self.emit(OpKind.dup, O_TYPE, (self.expr(arg),), None, name)
        if c == UNWRAP:
            (arg,) = e.args
            v = self.expr(arg)
            return self.emit(OpKind.unwrap, self.types[v].arg, (v,), None, name)
        if c == CANDIDATE:
            target = e.args[0]
            recipe = tuple(self.expr(a) for a in e.args[1:])
            return self.emit(OpKind.candidate, self.flat_type(name), recipe, target.name, name)
        x = self.program.externs.get(c)
        if x is None:
            self.fail(e.span, f"{c} is a macro used as a value: the expansion inlines "
                              "only macros called with edges")
        self.check_row_homes(x, e.span)
        if isinstance(x.result, Cases):
            self.fail(e.span, f"{c} has cases and is used as a value")
        args = tuple(self.expr(a) for a in e.args)
        kind = OpKind.prim if x.kind.value == "primitive" else OpKind.call
        v = self.emit(kind, x.result.type, args, c, name)
        self.spend(x, e.args)
        return v

    def flat_type(self, name: str) -> Type:
        return self.flat.types[name]

    def check_row_homes(self, x, span: SrcSpan) -> None:
        """A row that takes a destination home may write it; no declaration
        says on which case, so the frame state at the next exit is unknown"""
        for p in x.params:
            if is_dest_param(p.type):
                self.fail(span, f"row {x.name} takes the destination home {p.name} and "
                                "declares no definition of it: the homes at the exit are "
                                "not declared")

    def spend(self, x, args) -> None:
        "A row's spent sources are spent after it, on every case"
        for p, a in zip(x.params, args):
            if p.name in x.contract.spends:
                for home in home_names(a):
                    self.homes[home] = HomeState("spent")

    # Statements

    def stmt(self, s) -> None:
        if isinstance(s, Let):
            v = self.expr(s.value, s.name)
            if s.name != DISCARD:
                self.env[s.name] = v
        elif isinstance(s, Guard):
            cond = self.expr(s.cond)
            if any(h != INTACT for h in self.homes.values()):
                self.fail(s.span, "a guard after the homes changed")
            facts = test_facts(self.program, s.cond)
            self.emit(OpKind.guard, None, (cond,), Guarded(facts, self.entry_state()))
        elif isinstance(s, Publish):
            self.publish(s)
        else:
            self.fail(s.span, f"statement {type(s).__name__}")

    def publish(self, s: Publish) -> None:
        values = [(b.target, self.expr(b.value)) for b in s.bindings]
        taken: dict[str, int] = {}
        for d in s.debts:
            # An acquired source's owner already left its home with the acquire
            if d.kind is DebtKind.spent and d.subject not in self.acquired:
                taken[d.subject] = self.emit(OpKind.take, O_TYPE, (), d.subject, f"own_{d.subject}")
        olds: dict[str, int] = {}
        for target, _ in values:
            olds[target] = self.emit(OpKind.old, O_TYPE, (), target, f"old_{target}")
        for target, v in values:
            self.emit(OpKind.store, None, (v,), target)
        for d in s.debts:
            if d.kind is DebtKind.spent:
                self.homes[d.subject] = HomeState("spent")
        for target, v in values:
            self.homes[target] = HomeState("defined", v)
        self.emit(OpKind.checkpoint, None, (), s.point)
        self.point = s.point
        for d in s.debts:
            if d.kind is DebtKind.spent and d.subject not in taken:
                continue
            if d.kind is DebtKind.spent:
                arg = taken[d.subject]
            elif d.kind is DebtKind.old:
                arg = olds[d.subject]
            else:
                arg = self.env[d.subject]
            self.emit(OpKind.release, None, (arg,), d.kind)

    # Frame states

    def entry_state(self) -> FrameState:
        return FrameState(FrameRole.current.value, PcRole.site.value, ExcRole.clear.value,
                          tuple((h, INTACT) for h in self.home_operands()))

    def home_operands(self) -> list[str]:
        return [n for n, k in self.operands.items() if k in (FieldKind.home, FieldKind.vector)]

    def current_homes(self) -> tuple[tuple[str, HomeState], ...]:
        return tuple((h, self.homes.get(h, INTACT)) for h in self.home_operands())

    def point_state(self, point: str, span, frame=None, pc=None) -> FrameState:
        p = self.program.points[point]
        homes = dict(self.current_homes())
        for h in p.spent:
            if homes[h].state not in ("spent",):
                homes[h] = HomeState("spent")
        return FrameState(frame or p.frame.value, pc or p.pc.value, p.exc.value,
                          tuple(homes.items()), point)

    def exit_state(self, t: Exit, args: tuple[int, ...]) -> FrameState:
        kind = t.kind
        changed = any(h != INTACT for _, h in self.current_homes())
        if kind == "miss":
            return self.entry_state()
        if kind in ("next", "branch", "jump"):
            pc = PcRole.next_head.value if kind == "next" else PcRole.target.value
            if self.point is JOINED:
                self.fail(t.span, f"exit {kind} after a join of two checkpoints")
            if self.point is not None:
                return self.point_state(self.point, t.span, pc=pc)
            if changed:
                self.fail(t.span, f"exit {kind} after the homes changed, with no point")
            return FrameState(FrameRole.current.value, pc, ExcRole.clear.value,
                              self.current_homes())
        if kind == "raise":
            return _with(self.point_state(t.args[0].name, t.span), args)
        if kind == "again":
            if changed:
                self.fail(t.span, "again after the homes changed")
            return FrameState(FrameRole.current.value, PcRole.site.value, ExcRole.clear.value,
                              self.current_homes(), None, args)
        if kind == "enter":
            if t.resume is None:
                self.fail(t.span, "enter without a resume point")
            return _with(self.point_state(t.resume, t.span, frame=FrameRole.child.value),
                         args)
        if kind == "return":
            return FrameState("value", "value", ExcRole.clear.value, self.current_homes(),
                              None, ())
        if kind in ("redirect", "yield"):
            return FrameState(FrameRole.current.value, "value", ExcRole.clear.value,
                              self.current_homes(), self.point, args)
        if kind == "exit":
            return FrameState(FrameRole.external.value, "value", ExcRole.clear.value,
                              self.current_homes(), self.point, args)
        self.fail(t.span, f"exit {kind}")

    # Blocks and terminators

    def enter_block(self, label: str) -> None:
        homes, point = self.states.get(label, ({}, None))
        self.homes = dict(homes)
        self.point = point

    def flow(self, label: str, span) -> None:
        "Carry the homes and checkpoint to a successor; a join must agree"
        state = (dict(self.homes), self.point)
        if label in self.states:
            old = self.states[label]
            if {k: v.state for k, v in old[0].items()} != {k: v.state for k, v in state[0].items()}:
                self.fail(span, f"the paths into {label} disagree on the homes")
            if old[1] != state[1]:
                # Two checkpoints join: only an exit whose pc comes from
                # the point needs one, and it fails there
                self.states[label] = (old[0], JOINED)
            return
        self.states[label] = state

    def jump(self, target, payload=()) -> Jump:
        args = payload + tuple(self.expr(a) for a in target.args)
        self.flow(target.label, target.span)
        return Jump(target.label, args)

    def term(self, t) -> None:
        if isinstance(t, Goto):
            self.cur.term = self.jump(t.target)
        elif isinstance(t, Branch):
            cond = self.expr(t.cond)
            self.cur.term = BranchTerm(cond, self.jump(t.if_true), self.jump(t.if_false))
        elif isinstance(t, CallTerm):
            self.switch(t)
        elif isinstance(t, Exit):
            args = tuple(self.expr(a) for a in t.args
                         if not (t.kind == "raise" and a is t.args[0]))
            self.cur.term = ExitTerm(t.kind, args, self.exit_state(t, args))
        elif isinstance(t, Tail):
            x = self.program.externs[t.callee]
            self.fail(t.span, f"tail {t.callee}: the routine's exits ({', '.join(x.contract.exits)}) "
                              "declare no payload and no point, so the frame state at each "
                              "is not declared")
        elif isinstance(t, Each):
            self.fail(t.span, "each: no pilot arm loops")
        else:
            self.fail(t.span, f"terminator {type(t).__name__}")

    def switch(self, t: CallTerm) -> None:
        x = self.program.externs.get(t.callee)
        if x is None:
            self.fail(t.span, f"{t.callee} is not a row")
        self.check_row_homes(x, t.span)
        args = tuple(self.expr(a) for a in t.args)
        self.spend(x, t.args)
        cases = {c.name: c for c in x.result.cases}
        edges = []
        for e in t.edges:
            params = self.flat.blocks[e.target.label].params
            case = cases[e.case]
            payload = tuple(self.value(p.type, f"{t.callee}.{e.case}.{q.name}")
                            for p, q in zip(params, case.params))
            facts = tuple(fact_text(ef.fact) for ef in x.contract.ensures if ef.edge == e.case)
            target = self.jump(e.target)
            edges.append(CaseEdge(e.case, payload, target, facts))
        self.cur.term = Switch(t.callee, args, tuple(edges))

    def expand(self) -> SsaArm:
        flat = self.flat
        for label in flat.order:
            b = flat.blocks[label]
            params = []
            for p in b.params:
                params.append(self.value(p.type, p.name))
            self.blocks[label] = SsaBlock(label, tuple(params))
        self.states["entry"] = ({}, None)
        for label in flat.order:
            b = flat.blocks[label]
            self.cur = self.blocks[label]
            self.enter_block(label)
            saved_env = dict(self.env)
            for p, v in zip(b.params, self.cur.params):
                self.env[p.name] = v
            for s in b.stmts:
                self.stmt(s)
            self.term(b.term)
            # Names defined in a block are visible only where it dominates;
            # the checker has proved every use dominated, so keeping them is
            # safe, except operand reads, which are per block
            for k in [k for k in self.env if k not in saved_env
                      and (k in self.operands or k in ENVIRONMENT)]:
                del self.env[k]
        self.fix_switch_payloads()
        return SsaArm(self.arm.name, self.arm.family, self.arm.region.value,
                      dict(self.operands), self.blocks, self.types, self.names)

    def fix_switch_payloads(self) -> None:
        """A status edge's payload binds the target block's first
        parameters: name them so"""
        for b in self.blocks.values():
            if not isinstance(b.term, Switch):
                continue
            edges = []
            for e in b.term.edges:
                target = self.blocks[e.target.block]
                n = len(e.payload)
                edges.append(CaseEdge(e.case, target.params[:n], e.target, e.facts))
            b.term = Switch(b.term.row, b.term.args, tuple(edges))


def _with(state: FrameState, tokens: tuple[int, ...]) -> FrameState:
    return FrameState(state.frame, state.pc, state.exc, state.homes, state.point, tokens)


def home_names(e: Expr) -> list[str]:
    if isinstance(e, Name):
        return [e.name]
    if isinstance(e, SpanLit):
        return [n for i in e.items for n in home_names(i)]
    return []


def fact_text(f) -> str:
    t = f"<{f.type_arg.name}>" if f.type_arg is not None else ""
    return f"{f.fact.value}{t}({', '.join(f.subjects)})"


def test_facts(program: Program, cond: Expr) -> tuple[str, ...]:
    if isinstance(cond, Call) and cond.callee in program.externs:
        return tuple(fact_text(f) for f in program.externs[cond.callee].test)
    return ()


def expand(program: Program, arm: Arm) -> SsaArm:
    return Expander(program, arm).expand()


def expand_all(program: Program) -> tuple[dict[str, SsaArm], dict[str, Diagnostic]]:
    "Every arm's expansion, and the diagnostic of each arm that has none"
    done: dict[str, SsaArm] = {}
    failed: dict[str, Diagnostic] = {}
    for name, arm in program.arms.items():
        try:
            done[name] = expand(program, arm)
        except SsaError as e:
            failed[name] = e.diagnostic
    return done, failed


# A text form, for diagnostics and the report

def dump(arm: SsaArm) -> str:
    def v(x: int) -> str:
        n = arm.names.get(x)
        return f"%{x}" + (f".{n}" if n else "")

    def j(t: Jump) -> str:
        return f"{t.block}({', '.join(v(a) for a in t.args)})"

    lines = [f"arm {arm.name} ({arm.region})"]
    for b in arm.blocks.values():
        lines.append(f"  {b.label}({', '.join(v(p) for p in b.params)}):")
        for op in b.ops:
            lhs = f"{v(op.result)} = " if op.result is not None else ""
            attr = op.attr
            if isinstance(attr, Guarded):
                attr = f"else exit miss [{', '.join(attr.facts)}]"
            elif isinstance(attr, enum.Enum):
                attr = attr.value
            args = ", ".join(v(a) for a in op.args)
            lines.append(f"    {lhs}{op.kind.value} {attr if attr is not None else ''}"
                         f"({args})")
        t = b.term
        if isinstance(t, Jump):
            lines.append(f"    jump {j(t)}")
        elif isinstance(t, BranchTerm):
            lines.append(f"    branch {v(t.cond)} -> {j(t.if_true)}, {j(t.if_false)}")
        elif isinstance(t, Switch):
            lines.append(f"    switch {t.row}({', '.join(v(a) for a in t.args)})")
            for e in t.edges:
                lines.append(f"      {e.case} -> {j(e.target)} [{', '.join(e.facts)}]")
        elif isinstance(t, ExitTerm):
            s = t.state
            homes = ", ".join(f"{h}:{st.state}" + (f"={v(st.value)}" if st.value is not None else "")
                              for h, st in s.homes)
            lines.append(f"    exit {t.kind}({', '.join(v(a) for a in t.args)}) "
                         f"frame={s.frame} pc={s.pc} exc={s.exc} point={s.point} [{homes}]")
    return "\n".join(lines)
