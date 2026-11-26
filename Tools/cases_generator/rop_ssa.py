"""The tracer's SSA expansion of register instruction arms.

`expand(program, arm)` turns a checked arm (rop_check.Program) into an
`SsaArm`: blocks with parameters over single-assignment values that carry
the language's types.  It covers what a tracer needs from an arm:

- a guard is a side exit, with the frame state at the exit (the entry
  state, by the restart rule);
- a call to a row keeps the row's declared effects, feedback and parameter
  modes, and a row with cases is a `Switch` with one edge per case; a
  transfer row's Switch performs the exit each of its edges leads to;
- `publish` is a sequence of home writes, a checkpoint and the releases in
  the declared retire order (change 21); a vector binding moves or copies a
  span of homes, as one op over the span;
- every exit names its frame state and the tokens it discharges.

Nothing is inferred.  Where a declaration does not say what an arm does to
its homes, `expand` raises `SsaError` with a diagnostic, and the arm does
not expand.

The leaf IR (rop_leaf.py) is the templates' lowering target and stays as it
is; the evaluator (rop_ssa_eval.py) runs the leaf IR's primitive specs for
the primitive rows that have them.
"""

import enum
from dataclasses import dataclass, field, replace

from rop_catalog import (ENVIRONMENT, DebtKind, Fact, Scalar, ExcRole,
                         FieldKind, FrameRole, Handle, PcRole, Wrap)
from rop_cgen import Unsupported, flatten, is_transfer
from rop_check import (CANDIDATE, COUNT, PART, PRIMITIVES, UNWRAP, Program, expand_debts,
                       expand_group, field_kind, home_params, select_facts)
from rop_model import (Arm, BoolLit, Branch, Call, CallTerm, Cases, Diagnostic,
                       Each, EnumType, Exit, Expr, Family, Goto, Guard, HandleType, Let, Name, Nat,
                       Null, Publish, ScalarType, SpanLit, SrcSpan, Type,
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
    null = "null"               # the vacancy a publish may write
    rest = "rest"               # attr: span home; args: (owners,): the items past its count
    count = "count"             # attr: span home, or args: (span value,): its number of items
    # On a span operand, read, take, old, store and dup act on every item:
    # read gives the items borrowed, take and old their owners, store writes
    # item i of its argument to item i of the span, and dup is a new owner
    # of each item
    checkpoint = "checkpoint"   # attr: the point the frame state now matches
    release = "release"         # attr: DebtKind; args: (owner or anchors,)


@dataclass(frozen=True)
class HomeState:
    """A home at an exit: intact (as on entry), spent (a temporary emptied,
    a local left) or defined(value)"""
    state: str
    value: int | None = None


INTACT = HomeState("intact")
# A temporary whose owner an acquire() took: spent, with nothing left to take
ACQUIRED = HomeState("spent", -1)
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
    """A row with cases.  For a transfer row each edge leads to the exit its
    case names, and the row has performed it: the frame state the exit
    names is the one the row left"""
    row: str
    args: tuple[int, ...]
    edges: tuple[CaseEdge, ...]
    transfer: bool = False


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
    # The facts the arm's declaration grants on entry: its select items'
    # (SELECT_FACTS) and, for an observer, instrumented(site)
    facts: tuple[str, ...] = ()

    @property
    def entry(self) -> SsaBlock:
        return next(iter(self.blocks.values()))

    def exits(self) -> list[ExitTerm]:
        return [b.term for b in self.blocks.values() if isinstance(b.term, ExitTerm)]

    def guards(self) -> list[Op]:
        return [op for b in self.blocks.values() for op in b.ops if op.kind is OpKind.guard]


def is_dest_param(t: Type) -> bool:
    """A row parameter that names a home the row may write: the checker
    refuses Dst parameters on rows; a Local one a row may only spend, and
    a Cell is a read-only home"""
    if isinstance(t, WrapType) and t.wrap in (Wrap.Maybe, Wrap.Span):
        return is_dest_param(t.arg)
    return isinstance(t, HandleType) and t.handle in (Handle.Dst, Handle.IterPair)


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
        # A home group is its projections, each a home of its own
        self.params = home_params(self.family.params)
        self.groups = {p.name: p.type for p in self.family.params}
        self.operands = {p.name: FieldKind.home if p.type == PART else field_kind(p.type)
                         for p in self.params}
        self.blocks: dict[str, SsaBlock] = {}
        self.cur: SsaBlock
        # The homes and the last checkpoint on entry to each block
        self.states: dict[str, tuple[dict[str, HomeState], str | None]] = {}
        self.homes: dict[str, HomeState] = {}
        self.point: str | None = None
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
        if isinstance(e, Null):
            return self.emit(OpKind.null, WrapType(Wrap.Maybe, O_TYPE), (), None, "null")
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
        param = next((p for p in self.params if p.name == n), None)
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
            # The owner leaves the home on this path only: a branch that
            # does not acquire still finds it there
            self.homes[home.name] = ACQUIRED
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
        if c == COUNT:
            (arg,) = e.args
            if (isinstance(arg, Name) and arg.name not in self.env
                    and self.operands.get(arg.name) is FieldKind.vector):
                return self.emit(OpKind.count, SIZE, (), arg.name, name)
            return self.emit(OpKind.count, SIZE, (self.expr(arg),), None, name)
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
        """Change 21's four steps: take the spent inputs (a vector move's
        sources among them), read the olds, write, then close"""
        s = replace(s, debts=expand_debts(s.debts, self.groups))
        acquired_here = {h for h, state in self.homes.items() if state == ACQUIRED}
        owners = WrapType(Wrap.Span, O_TYPE)
        values: list[tuple[str, int]] = []
        taken: dict[str, int] = {}
        moved: set[str] = set()
        for b in s.bindings:
            if not b.vector or b.owners:
                # d = v, d = null, or d[] = v: item i of v goes to item i of d
                values.append((b.target, self.expr(b.value)))
                continue
            if isinstance(b.value, Call):
                # d[] = dup(s[]): a second owner of each source item
                (src,) = b.value.args
                items = self.emit(OpKind.read, WrapType(Wrap.Span, HandleType(Handle.B)),
                                  (), src.name, src.name)
                values.append((b.target, self.emit(OpKind.dup, owners, (items,), None,
                                                   f"dup_{src.name}")))
                continue
            # d[] = s[]: the sources' owners move
            src = b.value.name
            taken[src] = self.emit(OpKind.take, owners, (), src, f"own_{src}")
            moved.add(src)
            values.append((b.target, taken[src]))
        for d in s.debts:
            # An acquired source's owner already left its home with the acquire
            if (d.kind is DebtKind.spent and d.subject not in taken
                    and not (d.subject in acquired_here)):
                t = owners if self.operands.get(d.subject) is FieldKind.vector else O_TYPE
                taken[d.subject] = self.emit(OpKind.take, t, (), d.subject, f"own_{d.subject}")
        olds: dict[str, int] = {}
        for target, _ in values:
            t = owners if self.operands.get(target) is FieldKind.vector else O_TYPE
            olds[target] = self.emit(OpKind.old, t, (), target, f"old_{target}")
        for target, v in values:
            self.emit(OpKind.store, None, (v,), target)
        for name in moved:
            self.homes[name] = HomeState("spent")
        for d in s.debts:
            if d.kind is DebtKind.spent:
                self.homes[d.subject] = HomeState("spent")
        for target, v in values:
            self.homes[target] = HomeState("defined", v)
        self.emit(OpKind.checkpoint, None, (), s.point)
        self.point = s.point
        targets = {b.value.name: b.target for b in s.bindings
                   if b.vector and not b.owners and isinstance(b.value, Name)}
        for d in s.debts:
            if d.kind is DebtKind.spent and d.subject not in taken:
                continue
            if d.kind is DebtKind.spent and d.subject in moved:
                # A moved span's items past its destination's count
                arg = self.emit(OpKind.rest, owners, (taken[d.subject],),
                                targets[d.subject], f"rest_{d.subject}")
            elif d.kind is DebtKind.spent:
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
        defs = {h for n in p.defs for h in expand_group(n, self.groups)}
        for h in (h for n in p.spent for h in expand_group(n, self.groups)):
            # A home both spent and defined (a local deleted) is defined
            if homes[h].state not in ("spent",) and h not in defs:
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
        if kind in ("raise", "unwind"):
            return _with(self.point_state(t.args[0].name, t.span), args)
        if kind == "dispatch":
            if changed:
                self.fail(t.span, "dispatch after the homes changed")
            return FrameState(FrameRole.current.value, PcRole.site.value, ExcRole.clear.value,
                              self.current_homes(), None, args)
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
        if kind == "yield":
            return _with(self.point_state(t.resume, t.span), args)
        if kind == "redirect":
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
                         if not (t.kind in ("raise", "unwind") and a is t.args[0]))
            self.cur.term = ExitTerm(t.kind, args, self.exit_state(t, args))
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
        self.cur.term = Switch(t.callee, args, tuple(edges),
                               is_transfer(self.program, t.callee))

    def expand(self) -> SsaArm:
        flat = self.flat
        for label in flat.order:
            b = flat.blocks[label]
            params = []
            for p in b.params:
                params.append(self.value(p.type, p.name))
            self.blocks[label] = SsaBlock(label, tuple(params))
        self.states["entry"] = ({}, None)
        if self.arm.completion is not None:
            # A completion starts from its resume point's homes, with the
            # child's value as the entry's parameter
            p = self.program.points[self.arm.completion]
            spent = [h for n in p.spent for h in expand_group(n, self.groups)]
            self.states["entry"] = ({h: HomeState("spent") for h in spent}, p.name)
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
                      dict(self.operands), self.blocks, self.types, self.names,
                      entry_facts(self.family, self.arm))

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
            b.term = Switch(b.term.row, b.term.args, tuple(edges), b.term.transfer)


def entry_facts(fam: Family, arm: Arm) -> tuple[str, ...]:
    facts = [f"{fact.value}({subject})" for fact, subject in select_facts(fam, arm)]
    if arm.observes:
        facts.append(f"{Fact.instrumented.value}(site)")
    return tuple(facts)


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
    for name, arm in [*program.arms.items(), *program.completions.items()]:
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
            lines.append(f"    {'transfer' if t.transfer else 'switch'} {t.row}"
                         f"({', '.join(v(a) for a in t.args)})")
            for e in t.edges:
                lines.append(f"      {e.case} -> {j(e.target)} [{', '.join(e.facts)}]")
        elif isinstance(t, ExitTerm):
            s = t.state
            homes = ", ".join(f"{h}:{st.state}" + (f"={v(st.value)}" if st.value is not None else "")
                              for h, st in s.homes)
            lines.append(f"    exit {t.kind}({', '.join(v(a) for a in t.args)}) "
                         f"frame={s.frame} pc={s.pc} exc={s.exc} point={s.point} [{homes}]")
    return "\n".join(lines)
