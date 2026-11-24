"""The checker of register instruction definitions.

F0's structural checks (rop_check) show that a definition is well formed;
this module shows that each arm and each macro with a graph is correct by
its own declarations.  It walks the typed blocks of F0's Program in reverse
postorder with an abstract state:

- linear owner tokens (owners, errors, anchors, routes, tickets, ...), kept
  apart from value identities and from the state of the operand homes;
- borrowed references and control values, with what invalidates them;
- facts, closed under their implications, with heap kills;
- the first action and the last published point, for restartability and
  the checkpoints.

Rows and macros are taken at their declarations, pending or not.  The
checker refuses a declaration that the graph does not satisfy; it never
infers, extends or grants an effect, fact, exit or permission.

`verify(program)` returns the diagnostics; `load(paths)` parses, runs the
structural checks and then this checker over the definitions that passed
them.  The rules and their fixtures are in R/f1/checker.md.
"""

import argparse
import enum
import sys
from collections.abc import Iterable, Mapping
from dataclasses import dataclass, field, replace
from pathlib import Path

from rop_catalog import (AccessMode, BuiltinType, DebtKind, Effect, Fact, FrameRole,
                         Handle, Region, SchemaCtor, Wrap)
from rop_check import (CANDIDATE, ROP_DIR, UNWRAP, Program, feedback_schemas,
                       is_linear, is_source, result_cases)
import rop_check
from rop_model import (RESULT_CASE, Advance, Arm, Block, BoolLit, Branch, Call,
                       CallTerm, Cases, Contract, Diagnostic, Each, Exit, Expr, Extern,
                       FactUse, Family, Goto, Guard, HandleType, Let, Macro, Name, Nat,
                       Param, Point, Publish, Returns, SchemaType, SpanLit, SrcSpan,
                       Target, Type, WrapType, DISCARD, ENTRY)
from rop_print import expr_text


class Rule(enum.Enum):
    """The rule a diagnostic enforces; its value prefixes the message."""
    ownership = "ownership"       # linear tokens, homes, moves
    publish = "publish"           # parallel publication and its debts
    borrow = "borrow"             # borrowed references and anchors
    control = "control"           # frame transfers and completion targets
    points = "points"             # checkpoint state at raise and publish
    restart = "restart"           # guard before action, miss
    again = "again"               # tickets
    facts = "facts"               # requires and ensures
    join = "join"                 # owner states at joins
    effects = "effects"           # effect coverage
    feedback = "feedback"         # feedback coverage
    family = "family"             # family conformance


# The effect lattice of ir_contract.py: what a declared effect covers, the
# effects at whose sites heap facts die, and the reentry effects, after
# which a home may hold another value (a finalizer or Python code can
# write the frame's locals).
ALL_EFFECTS = frozenset(Effect)
COVERS = {
    Effect.runs_python: ALL_EFFECTS - {Effect.polls, Effect.switches_frame},
    Effect.polls: ALL_EFFECTS - {Effect.switches_frame},
    Effect.switches_frame: ALL_EFFECTS - {Effect.polls},
}
REENTRY = frozenset({Effect.releases, Effect.runs_python, Effect.polls,
                     Effect.switches_frame})
KILLS = REENTRY | {Effect.writes}
# The effects an adaptation row or macro may have before `again`
ADAPTATION = frozenset({Effect.reads, Effect.writes, Effect.allocates})

# Facts of mutable heap state: they die at every KILLS site.  The others
# are properties of the value itself (a static type refuses __class__
# assignment), and live as long as the value is held.
HEAP_FACTS = frozenset({Fact.type_version, Fact.has_values, Fact.in_bounds})

# Types of borrowed references: a value that may be freed or replaced
REFERENCE_HANDLES = frozenset({Handle.B, Handle.Obj, Handle.Fun, Handle.Gen,
                               Handle.Code})
# Control values that a callback can make stale: a head that a row
# computed (a completion target), not a target operand or next_head
CONTROL_HANDLES = frozenset({Handle.Head})


@dataclass(frozen=True, slots=True)
class FactT:
    """A fact of value identities (canonical names)."""
    fact: Fact
    type_arg: BuiltinType | None
    subjects: tuple[str, ...]

    def text(self) -> str:
        t = f"<{self.type_arg.name}>" if self.type_arg else ""
        return f"{self.fact.value}{t}({', '.join(self.subjects)})"


def implied(f: FactT) -> set[FactT]:
    """f and the facts it implies (regions89 design 1.1)."""
    out = {f}
    v = f.subjects[0]
    one = (v,)
    if f.fact == Fact.inline_int:
        out |= {FactT(Fact.exact, BuiltinType.int, one), FactT(Fact.nonnull, None, one)}
    elif f.fact == Fact.inline_float:
        out |= {FactT(Fact.exact, BuiltinType.float, one),
                FactT(Fact.nonnull, None, one)}
    elif f.fact == Fact.exact:
        out.add(FactT(Fact.nonnull, None, one))
        if f.type_arg not in (BuiltinType.int, BuiltinType.float):
            out.add(FactT(Fact.object, None, one))
    elif f.fact in (Fact.type_version, Fact.has_values):
        out |= {FactT(Fact.object, None, one), FactT(Fact.nonnull, None, one)}
    elif f.fact in (Fact.object, Fact.tagged_index, Fact.present):
        out.add(FactT(Fact.nonnull, None, one))
    return out


class RefKind(enum.Enum):
    home = "read from home"           # use(x): dies at reentry
    interior = "heap interior"        # a row's result: dies at any heap kill
    control = "control value"         # a computed head or frame: dies at reentry


@dataclass(frozen=True, slots=True)
class Ref:
    kind: RefKind
    span: SrcSpan
    what: str


@dataclass(frozen=True, slots=True)
class Token:
    """A linear obligation: where and how it was made, the operand it was
    acquired from, and whether the caller lent it (a borrowed macro
    parameter, which the graph may not consume)."""
    span: SrcSpan
    what: str
    source: str | None = None
    borrowed: bool = False


class Home(enum.Enum):
    intact = "intact"
    acquired = "acquired"
    spent = "spent"
    untouched = "untouched"
    defined = "defined"
    full = "full"
    vacant = "vacant"


@dataclass(frozen=True, slots=True)
class HomeState:
    state: Home
    span: SrcSpan | None = None
    what: str = ""

    def text(self) -> str:
        if self.span is None:
            return self.state.value
        return f"{self.state.value} by {self.what} at {self.span}"


Event = tuple[SrcSpan, str]


@dataclass
class State:
    live: dict[str, Token] = field(default_factory=dict)
    gone: dict[str, Event] = field(default_factory=dict)
    homes: dict[str, HomeState] = field(default_factory=dict)
    facts: set[FactT] = field(default_factory=set)
    refs: dict[str, Ref] = field(default_factory=dict)
    dead: dict[str, Event] = field(default_factory=dict)
    alias: dict[str, str] = field(default_factory=dict)
    action: Event | None = None        # the first action of the path
    commit: Event | None = None        # the first action that is not adaptation
    points: frozenset[str] = frozenset()
    # The homes as the last publish left them, checked against its point
    published: frozenset[tuple[str, HomeState]] | None = None

    def copy(self) -> "State":
        return State(dict(self.live), dict(self.gone), dict(self.homes), set(self.facts),
                     dict(self.refs), dict(self.dead), dict(self.alias), self.action,
                     self.commit, self.points, self.published)

    def canonical(self, name: str) -> str:
        while name in self.alias:
            name = self.alias[name]
        return name

    def add_fact(self, f: FactT) -> None:
        self.facts |= implied(f)

    def has(self, f: FactT) -> bool:
        return f in self.facts

    def signature(self) -> tuple[object, ...]:
        return (frozenset(self.live), frozenset(self.gone), frozenset(self.homes.items()),
                frozenset(self.facts), frozenset(self.dead), self.points)


Callee = Extern | Macro


@dataclass
class Walk:
    """One graph's check: its declarations and what the walk found."""
    program: Program
    definition: str
    blocks: dict[str, Block]
    order: tuple[str, ...]
    idom: Mapping[str, str | None]
    family: Family | None
    macro: Macro | None
    contract: Contract
    operands: dict[str, Type]
    defblock: dict[str, str]
    diags: dict[tuple[SrcSpan, str], Diagnostic] = field(default_factory=dict)
    effects: list[tuple[Effect, SrcSpan, str]] = field(default_factory=list)
    feedback: list[tuple[AccessMode, str, SrcSpan, str]] = field(default_factory=list)
    fresh: int = 0

    def report(self, span: SrcSpan, rule: Rule, message: str) -> None:
        key = (span, f"{span.line}:{span.column}:{rule.value}: {message}")
        if key not in self.diags:
            self.diags[key] = Diagnostic(span, self.definition, f"{rule.value}: {message}")

    def temp(self, span: SrcSpan, what: str) -> str:
        self.fresh += 1
        return f"{what}@{span.line}:{span.column}#{self.fresh}"

    @property
    def fb(self) -> set[str]:
        return {n for n, t in self.operands.items()
                if isinstance(t, SchemaType) and t.ctor == SchemaCtor.Fb}


def where(e: Event) -> str:
    return f"{e[1]} at {e[0]}"


# Types

def handle_of(t: Type) -> Handle | None:
    if isinstance(t, WrapType) and t.wrap == Wrap.Maybe:
        t = t.arg
    return t.handle if isinstance(t, HandleType) else None


def ref_kind(t: Type) -> RefKind | None:
    if isinstance(t, SchemaType) and t.ctor == SchemaCtor.Rows:
        return RefKind.interior
    h = handle_of(t)
    if h in REFERENCE_HANDLES:
        return RefKind.interior
    if h in CONTROL_HANDLES:
        return RefKind.control
    return None


def is_dest(t: Type) -> bool:
    return handle_of(t) in rop_check.DEST_HANDLES


def is_inout(t: Type) -> bool:
    return isinstance(t, WrapType) and t.wrap == Wrap.InOut


def is_feedback_write(c: Contract) -> bool:
    return any(a.mode == AccessMode.write for a in c.feedback)


def covered(declared: frozenset[Effect], e: Effect) -> bool:
    return e in declared or any(e in COVERS.get(d, ()) for d in declared)


def feedback_covered(declared: Iterable[tuple[AccessMode, str]], mode: AccessMode,
                     name: str) -> bool:
    for m, f in declared:
        if m != mode:
            continue
        if f == name or (f == "entry" and (name == "counter" or name.startswith("entry."))):
            return True
    return False


# Entry

def verify(program: Program) -> tuple[Diagnostic, ...]:
    """The checker's diagnostics for every arm and macro graph whose
    definition passed the structural checks."""
    broken = {d.definition for d in program.diagnostics}
    out: list[Diagnostic] = []
    for name, macro in program.macros.items():
        if macro.graph is not None and name not in broken and name in program.graphs:
            out += check_graph(program, name, None, macro)
    for name, arm in program.arms.items():
        if name not in broken and name in program.graphs:
            out += check_graph(program, name, arm, None)
    out += check_families(program, broken)
    return tuple(sorted(out, key=rop_check.diagnostic_order))


def load(paths: Iterable[Path]) -> Program:
    """Parse, check structure, then verify."""
    program = rop_check.load(paths)
    found = verify(program)
    return replace(program, diagnostics=tuple(sorted(
        program.diagnostics + found, key=rop_check.diagnostic_order)))


def generic_arms(program: Program, family: str) -> list[Arm]:
    """The arms a miss reaches: shared arms that do not themselves miss."""
    return [a for a in program.arms_of(family)
            if a.region == Region.shared and "miss" not in a.contract.exits]


def missing_generic(program: Program) -> tuple[str, ...]:
    """Families with a specialized arm that misses and no generic arm yet
    (its batch has not written it); the checker lists them, not refuses."""
    return tuple(sorted({a.family for a in program.arms.values()
                         if "miss" in a.contract.exits
                         and not generic_arms(program, a.family)}))


def check_families(program: Program, broken: set[str]) -> list[Diagnostic]:
    """A miss reaches the family's generic arm on unchanged state: the
    generic arm must take every exit but miss that its specialized arms
    take, and must not miss itself (the restart check of each miss is in
    the walk)."""
    out = []
    for fam in program.families.values():
        generics = generic_arms(program, fam.name)
        if len(generics) > 1:
            names = ", ".join(sorted(a.name for a in generics))
            out.append(Diagnostic(generics[0].span, fam.name, f"family: {fam.name} has "
                                  f"more than one shared arm without miss ({names}); a "
                                  "miss reaches one generic arm"))
        if len(generics) != 1:
            continue
        generic = generics[0]
        for arm in program.arms_of(fam.name):
            if arm is generic or arm.name in broken:
                continue
            extra = [e for e in arm.contract.exits
                     if e != "miss" and e not in generic.contract.exits]
            if extra:
                out.append(Diagnostic(arm.contract.span, arm.name, f"family: exits "
                                      f"{', '.join(extra)} of {arm.name} are not exits "
                                      f"of the generic arm {generic.name}"))
    return out


def check_graph(program: Program, name: str, arm: Arm | None, macro: Macro | None
                ) -> list[Diagnostic]:
    info = program.graphs[name]
    if arm is not None:
        family = program.families[arm.family]
        graph, contract, params = arm.graph, arm.contract, family.params
    else:
        assert macro is not None and macro.graph is not None
        family = None
        graph, contract, params = macro.graph, macro.contract, macro.params
    blocks = {b.label: b for b in graph.blocks}
    defblock: dict[str, str] = {}
    for b in graph.blocks:
        for p in b.params:
            defblock[p.name] = b.label
        for s in b.stmts:
            if isinstance(s, Let):
                defblock[s.name] = b.label
    w = Walk(program, name, blocks, info.order, info.idom, family, macro, contract,
             {p.name: p.type for p in params}, defblock)
    walk(w, entry_state(w, params))
    if arm is not None:
        for f in contract.requires:
            w.report(f.span, Rule.facts, f"arm requires "
                     f"{FactT(f.fact, f.type_arg, f.subjects).text()}, which nothing "
                     "before the arm's dispatch establishes")
    check_coverage(w)
    return list(w.diags.values())


def entry_state(w: Walk, params: tuple[Param, ...]) -> State:
    st = State()
    for p in params:
        t = p.type
        if is_inout(t):
            st.homes[p.name] = HomeState(Home.full)
        elif is_linear(t):
            st.live[p.name] = Token(p.span, "the caller", None, p.name in w.contract.borrows)
        elif w.family is not None and is_source(t) and handle_of(t) != Handle.Args:
            st.homes[p.name] = HomeState(Home.intact)
        elif w.family is not None and is_dest(t):
            st.homes[p.name + "/dst"] = HomeState(Home.untouched)
        elif w.family is None and is_source(t):
            st.homes[p.name] = HomeState(Home.intact)
    if w.family is None:
        for f in w.contract.requires:
            st.add_fact(FactT(f.fact, f.type_arg, f.subjects))
    return st


# The walk

def walk(w: Walk, start: State) -> None:
    """Blocks in reverse postorder; each/advance loops are iterated until
    the states entering their bodies are stable, and the diagnostics of
    the last pass are kept."""
    back: dict[str, list[tuple[str, State]]] = {}
    for _ in range(4):
        w.diags.clear()
        w.fresh = 0
        w.effects.clear()
        w.feedback.clear()
        incoming: dict[str, list[tuple[str, State]]] = {ENTRY: [("", start)]}
        new_back: dict[str, list[tuple[str, State]]] = {}
        for label in w.order:
            block = w.blocks[label]
            edges = incoming.get(label, [])
            loop = back.get(label, [])
            if not edges:
                continue
            st = merge(w, block, edges, loop)
            run_block(w, block, st, incoming, new_back, back)
        if signature(new_back) == signature(back):
            return
        back = new_back


def signature(back: Mapping[str, list[tuple[str, State]]]) -> object:
    return {k: [(p, s.signature()) for p, s in v] for k, v in back.items()}


def run_block(w: Walk, b: Block, st: State,
              incoming: dict[str, list[tuple[str, State]]],
              new_back: dict[str, list[tuple[str, State]]],
              back: Mapping[str, list[tuple[str, State]]]) -> None:
    for s in b.stmts:
        if isinstance(s, Guard):
            do_guard(w, st, s)
        elif isinstance(s, Let):
            do_let(w, st, s)
        else:
            do_publish(w, st, s)
    t = b.term
    if isinstance(t, Goto):
        flow(w, st.copy(), t.target, [], incoming, b.label)
    elif isinstance(t, Branch):
        _, facts = condition(w, st, t.cond)
        yes = st.copy()
        for f in facts:
            yes.add_fact(f)
        flow(w, yes, t.if_true, [], incoming, b.label)
        flow(w, st.copy(), t.if_false, [], incoming, b.label)
    elif isinstance(t, CallTerm):
        do_status_call(w, st, t, b.label, incoming)
    elif isinstance(t, Each):
        do_each(w, st, t, b.label, incoming, back)
    elif isinstance(t, Advance):
        check_loop_tokens(w, st, t)
        new_back.setdefault(t.body, []).append((b.label, st.copy()))
    else:
        do_exit(w, st, t)


def flow(w: Walk, st: State, target: Target, prefix: list[tuple[str, Type]],
         incoming: dict[str, list[tuple[str, State]]], pred: str) -> None:
    """Bind the target block's parameters: the payload first, then the
    evaluated arguments.  A token passes to the parameter; facts and
    borrow lifetimes follow the value."""
    block = w.blocks.get(target.label)
    if block is None:
        return
    values: list[tuple[str | None, Type]] = list(prefix)
    for arg, p in zip(target.args, block.params[len(prefix):]):
        values.append((value(w, st, arg, p.type), p.type))
    passed: set[str] = set()
    for (vid, t), p in zip(values, block.params):
        bind_param(w, st, p.name, vid, t, target.span, passed)
    incoming.setdefault(target.label, []).append((pred, st))


def bind_param(w: Walk, st: State, name: str, vid: str | None, t: Type, span: SrcSpan,
               passed: set[str]) -> None:
    if vid is None:
        return
    if is_linear(t):
        if vid in passed:
            w.report(span, Rule.ownership, f"{name}: {vid} is passed twice; two "
                     "parameters need two owners")
        passed.add(vid)
        token = take(w, st, vid, span, f"edge to {name}")
        if token is not None:
            st.live[name] = replace(token, borrowed=False)
    canon = st.canonical(vid)
    for f in list(st.facts):
        if canon in f.subjects:
            st.facts.add(FactT(f.fact, f.type_arg,
                               tuple(name if s == canon else s for s in f.subjects)))
    if vid in st.refs:
        st.refs[name] = st.refs[vid]
    if vid in st.dead:
        st.dead[name] = st.dead[vid]


def dominates(w: Walk, a: str, b: str) -> bool:
    node: str | None = b
    while node is not None:
        if node == a:
            return True
        node = w.idom.get(node)
    return False


def visible(w: Walk, name: str, label: str) -> bool:
    """Whether name is defined in a block dominating label, or is an
    operand, or one of label's parameters."""
    d = w.defblock.get(name)
    if d is None:
        return name in w.operands
    return dominates(w, d, label)


def merge(w: Walk, b: Block, edges: list[tuple[str, State]],
          loop: list[tuple[str, State]]) -> State:
    """The state entering b: facts intersect; owner and home states must
    agree on every incoming edge."""
    label = b.label
    params = {p.name for p in b.params}
    states = [s for _, s in edges] + [s for _, s in loop]
    names = set().union(*(s.live.keys() | s.gone.keys() for s in states))
    for n in sorted(names):
        d = w.defblock.get(n)
        if n in params or (d is not None and dominates(w, label, d)):
            # A parameter, or a value of the loop the back edge closes: the
            # advance reports it
            continue
        if not visible(w, n, label):
            for src, s in edges + loop:
                if n in s.live:
                    tok = s.live[n]
                    w.report(b.span, Rule.ownership, f"{n} (made by {tok.what} at "
                             f"{tok.span}) is live on the edge from {src} to {label}, "
                             f"which cannot see it: it is never discharged")
            continue
        live = [(src, s) for src, s in edges + loop if n in s.live]
        if live and len(live) != len(states):
            dead = next((src, s) for src, s in edges + loop if n not in s.live)
            how = dead[1].gone.get(n)
            w.report(b.span, Rule.join, f"at {label}, {n} is live from {live[0][0]} but "
                     f"{'consumed by ' + where(how) if how else 'absent'} from {dead[0]}: "
                     "incompatible owners")
    homes = set().union(*(s.homes.keys() for s in states))
    for h in sorted(homes):
        seen = {s.homes.get(h, HomeState(Home.untouched)).state for s in states}
        if len(seen) > 1:
            texts = "; ".join(f"{s.homes[h].text()} from {src}"
                              for src, s in edges + loop if h in s.homes)
            w.report(b.span, Rule.join, f"at {label}, home {h.removesuffix('/dst')} "
                     f"differs: {texts}")
    first = states[0]
    out = first.copy()
    for s in states[1:]:
        out.facts &= s.facts
        out.live = {k: v for k, v in out.live.items() if k in s.live}
        out.gone.update(s.gone)
        out.refs.update(s.refs)
        out.dead.update(s.dead)
        out.alias.update(s.alias)
        out.action = out.action or s.action
        out.commit = out.commit or s.commit
        out.points = out.points | s.points if out.points and s.points else frozenset()
        if out.published != s.published:
            out.published = None
    for k in list(out.live):
        if k not in params and not visible(w, k, label):
            del out.live[k]
    return out


def check_loop_tokens(w: Walk, st: State, t: Advance) -> None:
    for n, tok in st.live.items():
        d = w.defblock.get(n)
        if d is not None and dominates(w, t.body, d):
            w.report(t.span, Rule.ownership, f"advance {t.body}: {n} (made by "
                     f"{tok.what} at {tok.span}) is live at the end of the iteration")


# Values

def take(w: Walk, st: State, vid: str, span: SrcSpan, by: str) -> Token | None:
    """Consume a token."""
    token = st.live.get(vid)
    if token is None:
        how = st.gone.get(vid)
        if how is not None:
            w.report(span, Rule.ownership, f"{by}: {display(vid)} was already consumed "
                     f"by {where(how)}")
        return None
    if token.borrowed:
        w.report(span, Rule.ownership, f"{by}: {vid} is borrowed from the caller "
                 "(borrows); the macro cannot consume it")
        return None
    del st.live[vid]
    st.gone[vid] = (span, by)
    return token


def display(vid: str) -> str:
    return vid.split("@")[0] if "@" in vid else vid


def check_alive(w: Walk, st: State, name: str, span: SrcSpan) -> None:
    if name in st.dead:
        ref = st.refs.get(name)
        made = f" ({ref.kind.value} by {ref.what} at {ref.span})" if ref else ""
        kill = st.dead[name]
        rule = Rule.control if ref and ref.kind == RefKind.control else Rule.borrow
        w.report(span, rule, f"{name}{made} is used after {where(kill)}, which can "
                 "invalidate it")
    h = st.homes.get(name)
    if h is not None and h.state == Home.spent:
        w.report(span, Rule.ownership, f"{name} is used after it was spent by "
                 f"{h.what} at {h.span}")
    if h is not None and h.state == Home.vacant:
        w.report(span, Rule.ownership, f"{name} is used after the move at {h.span} "
                 "vacated it")


def value(w: Walk, st: State, e: Expr, want: Type | None = None,
          name: str | None = None) -> str | None:
    """Evaluate e: its value identity, or None for a literal."""
    if isinstance(e, Name):
        check_alive(w, st, e.name, e.span)
        return e.name
    if isinstance(e, (Nat, BoolLit)):
        return None
    if isinstance(e, SpanLit):
        vid = name or w.temp(e.span, "span")
        for item in e.items:
            iv = value(w, st, item)
            if iv is not None and iv in st.live:
                take(w, st, iv, item.span, f"[...] at {e.span}")
                st.live.setdefault(vid, Token(e.span, "a [...] of owners"))
        return vid
    return call_value(w, st, e, name)


def call_value(w: Walk, st: State, e: Call, name: str | None) -> str | None:
    vid = name or w.temp(e.span, e.callee)
    if e.callee == "use":
        (arg,) = e.args
        src = value(w, st, arg)
        if src is not None:
            st.alias[vid] = src
            st.refs[vid] = Ref(RefKind.home, e.span, f"use({src})")
        return vid
    if e.callee == "dup":
        (arg,) = e.args
        src = value(w, st, arg)
        set_action(st, e.span, f"dup({expr_text(arg)})", adaptation=False)
        st.live[vid] = Token(e.span, f"dup({expr_text(arg)})")
        copy_facts(st, src, vid)
        return vid
    if e.callee == "acquire":
        (arg,) = e.args
        src = value(w, st, arg)
        set_action(st, e.span, f"acquire({expr_text(arg)})", adaptation=False)
        if src is not None and src in st.homes:
            h = st.homes[src]
            if h.state == Home.acquired:
                w.report(e.span, Rule.ownership, f"acquire({src}): {src} was already "
                         f"acquired at {h.span}; a dying source has one owner")
            elif h.state == Home.intact:
                st.homes[src] = HomeState(Home.acquired, e.span, f"acquire({src})")
        st.live[vid] = Token(e.span, f"acquire({expr_text(arg)})", src)
        return vid
    if e.callee == "move":
        (arg,) = e.args
        src = value(w, st, arg)
        set_action(st, e.span, f"move({expr_text(arg)})", adaptation=False)
        use_effect(w, st, Effect.writes, e.span, f"move({expr_text(arg)})")
        if src is not None and src in st.homes and st.homes[src].state == Home.full:
            st.homes[src] = HomeState(Home.vacant, e.span, f"move({src})")
        st.live[vid] = Token(e.span, f"move({expr_text(arg)})")
        return vid
    if e.callee == UNWRAP:
        (arg,) = e.args
        src = value(w, st, arg)
        if src is not None:
            need = FactT(Fact.present, None, (st.canonical(src),))
            if not st.has(need):
                w.report(e.span, Rule.facts, f"unwrap({expr_text(arg)}) requires "
                         f"present({expr_text(arg)}), which no dominating fact proves")
            if src in st.refs:
                st.refs[vid] = st.refs[src]
            if src in st.dead:
                st.dead[vid] = st.dead[src]
        st.add_fact(FactT(Fact.nonnull, None, (vid,)))
        return vid
    if e.callee == CANDIDATE:
        for arg in e.args[1:]:
            value(w, st, arg)
        return vid
    callee = w.program.externs.get(e.callee) or w.program.macros.get(e.callee)
    assert callee is not None and isinstance(callee.result, Returns)
    args = [value(w, st, a) for a in e.args]
    apply_call(w, st, callee, e.args, args, e.span)
    t = callee.result.type
    if is_linear(t):
        st.live[vid] = Token(e.span, f"{e.callee}(...)")
    kind = ref_kind(t)
    if kind is not None:
        st.refs[vid] = Ref(kind, e.span, f"{e.callee}(...)")
    ensure(w, st, callee, RESULT_CASE, args, [vid])
    return vid


def copy_facts(st: State, src: str | None, dst: str, heap: bool = True) -> None:
    if src is None:
        return
    canon = st.canonical(src)
    for f in list(st.facts):
        if f.subjects == (canon,) and (heap or f.fact not in HEAP_FACTS):
            st.facts.add(FactT(f.fact, f.type_arg, (dst,)))


def set_action(st: State, span: SrcSpan, what: str, adaptation: bool) -> None:
    if st.action is None:
        st.action = (span, what)
    if not adaptation and st.commit is None:
        st.commit = (span, what)


def use_effect(w: Walk, st: State, e: Effect, span: SrcSpan, what: str) -> None:
    w.effects.append((e, span, what))


def kill(st: State, effects: frozenset[Effect], span: SrcSpan, what: str) -> None:
    """Heap facts and interior borrows die at a heap kill; borrows of a
    home, computed control values and the facts of values left in homes
    die at reentry."""
    killing = effects & KILLS
    if not killing:
        return
    why = (span, f"{what} ({', '.join(sorted(e.value for e in killing))})")
    reentry = bool(effects & REENTRY)
    homes = {h.removesuffix("/dst") for h in st.homes}
    st.facts = {f for f in st.facts if f.fact not in HEAP_FACTS
                and not (reentry and any(s in homes for s in f.subjects))}
    for n, ref in st.refs.items():
        if n in st.dead:
            continue
        if ref.kind == RefKind.interior or reentry:
            st.dead[n] = why


def apply_call(w: Walk, st: State, callee: Callee, exprs: tuple[Expr, ...],
               args: list[str | None], span: SrcSpan) -> None:
    """A call of a row or macro at its declaration: its requires, the
    borrows it crosses, its parameter modes, its effects, feedback and
    kills."""
    c = callee.contract
    what = f"call {callee.name}"
    params = {p.name: (p, a) for p, a in zip(callee.params, args)}
    for f in c.requires:
        subjects = tuple(st.canonical(params[s][1] or "?") if s in params else s
                         for s in f.subjects)
        if not st.has(FactT(f.fact, f.type_arg, subjects)):
            text = f"{f.fact.value}{'<' + f.type_arg.name + '>' if f.type_arg else ''}"
            shown = ", ".join(expr_text(exprs[list(params).index(s)]) if s in params
                              else s for s in f.subjects)
            w.report(span, Rule.facts, f"{what} requires {text}({shown}), which no "
                     "dominating fact proves")
    effects = c.effect_set
    for p, a in zip(callee.params, args):
        if a is None or is_linear(p.type):
            continue
        ref = st.refs.get(a)
        if ref is None or a in st.dead:
            continue
        if (ref.kind == RefKind.home and effects & REENTRY) or (
                ref.kind == RefKind.interior and effects & KILLS):
            killing = sorted(e.value for e in effects & (
                REENTRY if ref.kind == RefKind.home else KILLS))
            w.report(span, Rule.borrow, f"{display(a)} ({ref.kind.value} by {ref.what} "
                     f"at {ref.span}) crosses {what} ({', '.join(killing)}), which can "
                     "invalidate it; pass an anchor the call cannot invalidate")
    for p, a in zip(callee.params, args):
        if a is None or not is_linear(p.type):
            continue
        if p.name in c.borrows:
            if a not in st.live:
                how = st.gone.get(a)
                w.report(span, Rule.ownership, f"{what} borrows {display(a)}, which "
                         f"was consumed by {where(how) if how else 'nothing live'}")
            continue
        take(w, st, a, span, what)
    for p, e in zip(callee.params, exprs):
        if p.name in c.spends:
            items = e.items if isinstance(e, SpanLit) else (e,)
            for item in items:
                if isinstance(item, Name):
                    spend(w, st, item.name, item.span, what)
    for e in sorted(effects, key=lambda x: x.value):
        use_effect(w, st, e, span, what)
    if effects - {Effect.reads}:
        adapt = is_feedback_write(c) and effects <= ADAPTATION
        set_action(st, span, what, adaptation=adapt)
    fbs = w.fb
    if any(a in fbs for a in args if a is not None):
        for acc in c.feedback:
            w.feedback.append((acc.mode, acc.field, span, what))
    kill(st, effects, span, what)


def spend(w: Walk, st: State, name: str, span: SrcSpan, what: str) -> None:
    h = st.homes.get(name)
    if h is None:
        return
    if h.state == Home.spent:
        w.report(span, Rule.ownership, f"{what} spends {name}, which {h.what} at "
                 f"{h.span} already spent")
        return
    st.homes[name] = HomeState(Home.spent, span, what)


def ensure(w: Walk, st: State, callee: Callee, case: str, args: list[str | None],
           payload: list[str]) -> None:
    """Add the facts the callee ensures on one case, of its payload and
    arguments."""
    names = {p.name: a for p, a in zip(callee.params, args)}
    cases = result_cases(callee.result)
    for p, vid in zip(cases.get(case, ()), payload):
        names[p.name] = vid
    for ef in callee.contract.ensures:
        if ef.edge != case:
            continue
        subjects = tuple(names.get(s) for s in ef.fact.subjects)
        if any(s is None for s in subjects):
            continue
        st.add_fact(FactT(ef.fact.fact, ef.fact.type_arg,
                          tuple(st.canonical(s) for s in subjects if s is not None)))


def condition(w: Walk, st: State, e: Expr) -> tuple[str | None, list[FactT]]:
    """Evaluate a guard or branch condition: the facts its true edge gains,
    exactly those of its row's test(...)."""
    vid = value(w, st, e)
    if not isinstance(e, Call):
        return vid, []
    row = w.program.externs.get(e.callee)
    if row is None or not row.test:
        return vid, []
    names = {p.name: arg for p, arg in zip(row.params, e.args)}
    facts = []
    for f in row.test:
        subjects = []
        for s in f.subjects:
            arg = names.get(s)
            sub = subject_of(st, arg)
            if sub is None:
                break
            subjects.append(sub)
        else:
            facts.append(FactT(f.fact, f.type_arg, tuple(subjects)))
    return vid, facts


def subject_of(st: State, arg: Expr | None) -> str | None:
    """The value identity a test argument names: a name, or use(x) of a
    home, whose value is the home's."""
    if isinstance(arg, Name):
        return st.canonical(arg.name)
    if isinstance(arg, Call) and arg.callee == "use" and isinstance(arg.args[0], Name):
        return st.canonical(arg.args[0].name)
    return None


# Statements

def do_guard(w: Walk, st: State, s: Guard) -> None:
    _, facts = condition(w, st, s.cond)
    check_restart(w, st, s.span, f"guard {expr_text(s.cond)} else miss")
    for f in facts:
        st.add_fact(f)


def do_let(w: Walk, st: State, s: Let) -> None:
    if s.name == DISCARD:
        value(w, st, s.value, s.type)
        return
    if isinstance(s.value, Name):
        src = value(w, st, s.value)
        if src is None:
            return
        if src in st.live or src in st.gone:
            tok = take(w, st, src, s.span, f"let {s.name}")
            if tok is not None:
                st.live[s.name] = tok
        st.alias[s.name] = src
        if src in st.refs:
            st.refs[s.name] = st.refs[src]
        if src in st.dead:
            st.dead[s.name] = st.dead[src]
        return
    value(w, st, s.value, s.type, s.name)


def check_restart(w: Walk, st: State, span: SrcSpan, what: str) -> None:
    """A miss restarts the word in the generic arm: the original operands
    untouched, no obligation outstanding, nothing language-visible done."""
    if st.action is not None:
        w.report(span, Rule.restart, f"{what} follows {where(st.action)}: the word "
                 "cannot restart")
        return
    for n, tok in st.live.items():
        if not tok.borrowed and tok.what != "the caller":
            w.report(span, Rule.restart, f"{what} with {n} outstanding (made by "
                     f"{tok.what} at {tok.span}): the word cannot restart")
            return
    for h, hs in st.homes.items():
        if hs.state not in (Home.intact, Home.untouched, Home.full):
            w.report(span, Rule.restart, f"{what} after {h.removesuffix('/dst')} was "
                     f"{hs.text()}: the word cannot restart")
            return


def do_publish(w: Walk, st: State, s: Publish) -> None:
    """Evaluate every input, save displaced owners once per home, clear
    spent temporaries, install all results, then discharge the debts in
    order; the point describes the state after."""
    what = f"publish at {s.point}"
    bound: dict[str, str] = {}
    owners: dict[str, str] = {}
    for b in s.bindings:
        vid = value(w, st, b.value)
        if vid is None:
            continue
        bound[b.target] = vid
        if vid in owners:
            w.report(b.span, Rule.publish, f"{owners[vid]} and {b.target} both take "
                     f"{display(vid)}: two outputs need two owners")
            continue
        owners[vid] = b.target
    olds = [d.subject for d in s.debts if d.kind == DebtKind.old]
    spents = [d.subject for d in s.debts if d.kind == DebtKind.spent]
    debts = []
    for d in s.debts:
        if d.kind == DebtKind.old and d.subject not in bound:
            w.report(d.span, Rule.publish, f"old({d.subject}): {d.subject} is not bound "
                     "by this publish")
        if (d.kind, d.subject) in debts and d.kind in (DebtKind.old, DebtKind.spent):
            why = ("one home has one displaced owner" if d.kind == DebtKind.old
                   else "a repeated operand makes one claim on its temporary")
            w.report(d.span, Rule.publish, f"{d.kind.value}({d.subject}) is retired "
                     f"twice: {why}")
            continue
        debts.append((d.kind, d.subject))
    for target in bound:
        if target not in olds:
            w.report(s.span, Rule.publish, f"{target} is bound but its displaced owner "
                     f"is not retired: add old({target})")
    published = {}
    taken: set[str] = set()
    for target, vid in bound.items():
        published[target] = vid
        dst = st.homes.get(target + "/dst")
        if dst is not None and dst.state == Home.defined:
            w.report(s.span, Rule.publish, f"{target} is published twice; first by "
                     f"{dst.what} at {dst.span}")
        if vid not in st.live and vid not in st.gone:
            w.report(s.span, Rule.publish, f"{target} = {display(vid)}: not an owner")
        if vid not in taken:
            taken.add(vid)
            take(w, st, vid, s.span, what)
    for kind, subject in debts:
        span = next(d.span for d in s.debts if (d.kind, d.subject) == (kind, subject))
        if kind == DebtKind.spent:
            spend(w, st, subject, span, what)
        elif kind in (DebtKind.roots, DebtKind.owned):
            check_alive(w, st, subject, span)
            take(w, st, subject, span, f"{kind.value}({subject}) in {what}")
    set_action(st, s.span, what, adaptation=False)
    effects: frozenset[Effect] = frozenset()
    if bound or spents:
        effects |= {Effect.writes}
        use_effect(w, st, Effect.writes, s.span, what)
    if s.debts:
        effects |= {Effect.releases}
        use_effect(w, st, Effect.releases, s.span, what)
    values = {}
    for target, vid in published.items():
        tmp = State(facts=set(st.facts), alias=st.alias)
        copy_facts(tmp, vid, target, heap=False)
        values[target] = {f for f in tmp.facts if f.subjects == (target,)}
    kill(st, effects, s.span, what)
    for target in published:
        st.facts = {f for f in st.facts if target not in f.subjects}
        st.facts |= values[target]
        st.homes[target + "/dst"] = HomeState(Home.defined, s.span, what)
    st.points = frozenset({s.point})
    st.published = frozenset(st.homes.items())
    point = w.program.points.get(s.point)
    if point is not None and w.family is not None:
        check_point(w, st, point, s.span, what, held_tokens=False)


def do_status_call(w: Walk, st: State, t: CallTerm, label: str,
                   incoming: dict[str, list[tuple[str, State]]]) -> None:
    callee = w.program.externs.get(t.callee) or w.program.macros.get(t.callee)
    assert callee is not None and isinstance(callee.result, Cases)
    args = [value(w, st, a) for a in t.args]
    apply_call(w, st, callee, t.args, args, t.span)
    cases = {c.name: c for c in callee.result.cases}
    for edge in t.edges:
        case = cases[edge.case]
        out = st.copy()
        payload: list[tuple[str, Type]] = []
        for p in case.params:
            vid = w.temp(edge.span, f"{edge.case}.{p.name}")
            if is_linear(p.type):
                out.live[vid] = Token(edge.span, f"case {edge.case} of {t.callee}")
            kind = ref_kind(p.type)
            if kind is not None:
                out.refs[vid] = Ref(kind, edge.span, f"case {edge.case} of {t.callee}")
            payload.append((vid, p.type))
        ensure(w, out, callee, edge.case, args, [v for v, _ in payload])
        flow(w, out, edge.target, payload, incoming, label)


def do_each(w: Walk, st: State, t: Each, label: str,
            incoming: dict[str, list[tuple[str, State]]],
            back: Mapping[str, list[tuple[str, State]]]) -> None:
    value(w, st, t.over)
    body = w.blocks.get(t.body)
    if body is not None:
        into = st.copy()
        payload = [(w.temp(t.span, p.name), p.type) for p in body.params]
        for vid, typ in payload:
            kind = ref_kind(typ)
            if kind is not None:
                into.refs[vid] = Ref(kind, t.span, f"each over {expr_text(t.over)}")
        flow(w, into, Target(t.body, (), t.span), payload, incoming, label)
    # The exhaust edge follows the last iteration as well as an empty span
    out = st.copy()
    for _, s in back.get(t.body, []):
        out.facts &= s.facts
        out.dead.update(s.dead)
        out.action = out.action or s.action
        out.commit = out.commit or s.commit
    flow(w, out, t.exhaust, [], incoming, label)


# Exits

def do_exit(w: Walk, st: State, t: Exit) -> None:
    args = [value(w, st, a) for a in t.args]
    if w.macro is not None:
        macro_exit(w, st, t, args)
        return
    assert w.family is not None
    kind = t.kind
    what = f"exit {kind}"
    accounted: set[str] = set()
    if kind == "miss":
        check_restart(w, st, t.span, "exit miss")
        return
    if kind == "again":
        ticket = args[0]
        if ticket is not None and ticket not in st.live:
            how = st.gone.get(ticket)
            w.report(t.span, Rule.again, f"exit again({display(ticket)}) has no fresh "
                     f"ticket: {display(ticket)} was consumed by "
                     f"{where(how) if how else 'nothing'}")
        elif ticket is not None:
            take(w, st, ticket, t.span, what)
        if st.commit is not None:
            w.report(t.span, Rule.again, f"exit again follows {where(st.commit)}: the "
                     "ticket is not fresh")
        else:
            check_restart_homes(w, st, t.span, "exit again")
    elif kind == "raise":
        use_effect(w, st, Effect.raises, t.span, what)
        if args[1] is not None:
            take(w, st, args[1], t.span, what)
        point_name = t.args[0].name if isinstance(t.args[0], Name) else ""
        point = w.program.points.get(point_name)
        if point is not None:
            accounted = check_point(w, st, point, t.span, f"raise at {point_name}",
                                    held_tokens=True)
    elif kind in ("enter", "return"):
        use_effect(w, st, Effect.switches_frame, t.span, what)
        check_transfer(w, st, t, kind)
    elif kind in ("yield", "redirect", "exit"):
        if kind == "yield":
            use_effect(w, st, Effect.switches_frame, t.span, what)
        if args[0] is not None:
            take(w, st, args[0], t.span, what)
    if kind in ("next", "branch", "jump"):
        check_completion(w, st, t, what)
    for n, tok in sorted(st.live.items()):
        if n in accounted:
            continue
        w.report(t.span, Rule.ownership, f"{what} leaves {display(n)} (made by "
                 f"{tok.what} at {tok.span}) unaccounted")
    for ef in w.contract.ensures:
        if ef.edge == kind:
            check_fact(w, st, ef.fact, {}, t.span, f"{what} ensures")


def check_restart_homes(w: Walk, st: State, span: SrcSpan, what: str) -> None:
    for h, hs in st.homes.items():
        if hs.state not in (Home.intact, Home.untouched, Home.full):
            w.report(span, Rule.again, f"{what} after {h.removesuffix('/dst')} was "
                     f"{hs.text()}: the word cannot restart")
            return


def check_completion(w: Walk, st: State, t: Exit, what: str) -> None:
    """At next, branch and jump the homes are those of the point last
    published, or untouched if none was."""
    if st.points:
        if st.published == frozenset(st.homes.items()):
            return
        for name in sorted(st.points):
            point = w.program.points.get(name)
            if point is not None:
                check_point(w, st, point, t.span, f"{what} after publish at {name}",
                            held_tokens=False)
        return
    for h, hs in st.homes.items():
        if hs.state not in (Home.intact, Home.untouched, Home.full):
            w.report(t.span, Rule.points, f"{what}: {h.removesuffix('/dst')} is "
                     f"{hs.text()}, but no publish names a point for it")


def check_transfer(w: Walk, st: State, t: Exit, kind: str) -> None:
    """A direct frame transfer names its frame and its owner map: a point
    with the matching frame role published on every path, and a frame that
    is not the current one."""
    role = FrameRole.child if kind == "enter" else FrameRole.caller
    frame = t.args[0] if t.args else None
    if kind == "enter" and isinstance(frame, Name) and frame.name == "frame":
        w.report(t.span, Rule.control, "exit enter(frame, ...): the current frame is "
                 "not a child frame")
    points = [w.program.points.get(p) for p in sorted(st.points)]
    if not points or any(p is None or p.frame != role for p in points):
        w.report(t.span, Rule.control, f"exit {kind}: no publish names the owner map "
                 f"at the transfer (a point of frame role {role.value})")
        return
    for p in points:
        assert p is not None
        check_point(w, st, p, t.span, f"exit {kind} after publish at {p.name}",
                    held_tokens=False)


def check_point(w: Walk, st: State, p: Point, span: SrcSpan, what: str,
                held_tokens: bool) -> set[str]:
    """The point's keep, spent, defs and held lists against the homes and
    owners: a source is kept if intact, spent if spent, held if an owner
    acquired from it is outstanding; a destination is a def if published.
    Returns the tokens the point accounts for (those held)."""
    roles: dict[str, str] = {}
    for role, names in (("keep", p.keep), ("spent", p.spent), ("held", p.held)):
        for n in names:
            roles[n] = role
    accounted: set[str] = set()
    for h, hs in sorted(st.homes.items()):
        if h.endswith("/dst"):
            name = h.removesuffix("/dst")
            declared = name in p.defs
            actual = hs.state == Home.defined
            if declared != actual:
                w.report(span, Rule.points, f"{what}: {p.name} "
                         f"{'lists' if declared else 'does not list'} {name} in defs, "
                         f"but {name} is {hs.text()}")
            continue
        holder = [n for n, tok in st.live.items() if tok.source == h]
        if hs.state == Home.intact:
            actual = "keep"
        elif hs.state == Home.spent:
            actual = "spent"
        elif hs.state == Home.acquired and holder:
            actual = "held"
        else:
            actual = "spent"
        declared = roles.get(h, "")
        if declared != actual:
            listed = f"lists {h} in {declared}" if declared else f"does not list {h}"
            w.report(span, Rule.points, f"{what}: {p.name} {listed}, but {h} is "
                     f"{hs.text()}" + (f" with {holder[0]} outstanding" if holder else ""))
        if actual == "held" and declared == "held" and held_tokens:
            accounted.update(holder)
    return accounted


def check_fact(w: Walk, st: State, f: FactUse, names: Mapping[str, str | None],
               span: SrcSpan, what: str) -> None:
    subjects = []
    for s in f.subjects:
        v = names.get(s, s)
        if v is None:
            return
        subjects.append(st.canonical(v))
    fact = FactT(f.fact, f.type_arg, tuple(subjects))
    if not st.has(fact):
        shown = FactT(f.fact, f.type_arg, f.subjects).text()
        w.report(span, Rule.facts, f"{what} {shown}, which no dominating fact proves")


def macro_exit(w: Walk, st: State, t: Exit, args: list[str | None]) -> None:
    assert w.macro is not None
    case = result_cases(w.macro.result).get(t.kind, ())
    what = f"exit {t.kind}"
    for p, a in zip(case, args):
        if a is not None and is_linear(p.type):
            take(w, st, a, t.span, what)
    for n, tok in sorted(st.live.items()):
        if tok.borrowed:
            continue
        w.report(t.span, Rule.ownership, f"{what} leaves {display(n)} (made by "
                 f"{tok.what} at {tok.span}) unaccounted")
    for n in w.contract.borrows:
        if n not in st.live and n in st.gone:
            w.report(t.span, Rule.ownership, f"{what}: borrowed {n} was consumed by "
                     f"{where(st.gone[n])}")
    for h, hs in sorted(st.homes.items()):
        if h in w.contract.spends:
            if hs.state != Home.spent:
                w.report(t.span, Rule.ownership, f"{what}: spends({h}) is declared, "
                         f"but {h} is {hs.text()}")
        elif hs.state == Home.spent:
            w.report(t.span, Rule.ownership, f"{what}: {h} is {hs.text()}, which "
                     "spends() does not declare")
    names = {p.name: a for p, a in zip(case, args)}
    for ef in w.contract.ensures:
        if ef.edge == t.kind:
            check_fact(w, st, ef.fact, names, t.span, f"{what} ensures")


# Coverage

def check_coverage(w: Walk) -> None:
    declared = w.contract.effect_set
    shown = ", ".join(e.effect.value for e in w.contract.effects)
    for e, span, what in w.effects:
        if not covered(declared, e):
            w.report(span, Rule.effects, f"{what} may {e.value}, which effects({shown}) "
                     "does not declare")
    accesses = [(a.mode, a.field) for a in w.contract.feedback]
    fshown = ", ".join(f"{a.mode.value}({a.field})" for a in w.contract.feedback)
    for mode, name, span, what in w.feedback:
        if not feedback_covered(accesses, mode, name):
            w.report(span, Rule.feedback, f"{what} may {mode.value} {name}, which "
                     f"feedback({fshown}) does not declare")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("files", nargs="*", type=Path)
    args = parser.parse_args(argv)
    files = args.files or sorted(ROP_DIR.glob("*.rop"))
    program = load(files)
    for d in program.diagnostics:
        print(d, file=sys.stderr)
    if not program.diagnostics:
        print(f"{len(program.arms)} arms and "
              f"{sum(m.graph is not None for m in program.macros.values())} macros "
              "verified")
        missing = missing_generic(program)
        if missing:
            print("no generic arm yet: " + ", ".join(missing))
    return 1 if program.diagnostics else 0


if __name__ == "__main__":
    sys.exit(main())
