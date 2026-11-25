"""The data model of register instruction definitions.

Frozen dataclasses: a parsed `.rop` file is a tuple of declarations; an arm
or a macro holds a graph of blocks with parameters, straight-line
statements and one terminator each.  Names are single assignment within a
graph, so a value's name identifies it.  Source spans do not take part in
equality.  The model has no stack notion: operands have names and types.
"""

from dataclasses import dataclass, field
from pathlib import PurePath
from typing import Union

from rop_catalog import (AccessMode, BuiltinType, DebtKind, Effect, ExcRole, Fact,
                         FieldKind,
                         FrameRole, Handle, MonitoringEvent, PcRole, Region, RowKind, Scalar, Schema, Select,
                         SchemaCtor, Slot, Wrap)


@dataclass(frozen=True, slots=True)
class SrcSpan:
    filename: str
    line: int
    column: int
    end_line: int
    end_column: int

    def __str__(self) -> str:
        return f"{PurePath(self.filename).name}:{self.line}"


NOWHERE = SrcSpan("<builtin>", 0, 0, 0, 0)


def _span() -> SrcSpan:
    return field(default=NOWHERE, compare=False, repr=False)


@dataclass(frozen=True, slots=True)
class Diagnostic:
    """One finding: where, in which definition, and what.  The message names
    the offending use and, where it helps, the location of the other party
    (`call box_i64 at add.rop:31 has no edge for err`)."""
    span: SrcSpan
    definition: str
    message: str

    def __str__(self) -> str:
        where = f"{self.span.filename}:{self.span.line}:{self.span.column}"
        return f"{where}: {self.definition}: {self.message}"


class RopError(Exception):
    """The one exception type of the frontend: a diagnostic with a span."""

    def __init__(self, diagnostic: Diagnostic) -> None:
        super().__init__(str(diagnostic))
        self.diagnostic = diagnostic


# Types

@dataclass(frozen=True, slots=True)
class ScalarType:
    scalar: Scalar


@dataclass(frozen=True, slots=True)
class HandleType:
    handle: Handle


@dataclass(frozen=True, slots=True)
class WrapType:
    wrap: Wrap
    arg: "Type"


@dataclass(frozen=True, slots=True)
class PairType:
    first: "Type"
    second: "Type"


@dataclass(frozen=True, slots=True)
class SchemaVar:
    """A schema parameter of a generic row or macro (`tick<K>`)."""
    name: str


SchemaRef = Union[Schema, SchemaVar]


@dataclass(frozen=True, slots=True)
class SchemaType:
    ctor: SchemaCtor
    schema: SchemaRef


@dataclass(frozen=True, slots=True)
class EnumType:
    name: str


Type = Union[ScalarType, HandleType, WrapType, PairType, SchemaType, EnumType]


# Expressions

@dataclass(frozen=True, slots=True)
class Name:
    """A value, operand, environment name, enum member, point or arm,
    possibly qualified (`Add.Done`, `Numeric.NB_ADD`)."""
    name: str
    span: SrcSpan = _span()


@dataclass(frozen=True, slots=True)
class Nat:
    value: int
    span: SrcSpan = _span()


@dataclass(frozen=True, slots=True)
class BoolLit:
    value: bool
    span: SrcSpan = _span()


@dataclass(frozen=True, slots=True)
class Call:
    """A call of a total row, macro or language primitive in an expression."""
    callee: str
    args: tuple["Expr", ...]
    span: SrcSpan = _span()


@dataclass(frozen=True, slots=True)
class SpanLit:
    """An ordered list of evaluated operands: a `Span<T>`."""
    items: tuple["Expr", ...]
    span: SrcSpan = _span()


@dataclass(frozen=True, slots=True)
class Null:
    """`null`: the vacancy, which only a publish binding may write."""
    span: SrcSpan = _span()


Expr = Union[Name, Nat, BoolLit, Call, SpanLit, Null]


# Declarations' parts

@dataclass(frozen=True, slots=True)
class Param:
    name: str
    type: Type
    span: SrcSpan = _span()


@dataclass(frozen=True, slots=True)
class CaseDecl:
    """A named outcome of a row or macro, and its payload."""
    name: str
    params: tuple[Param, ...]
    span: SrcSpan = _span()


@dataclass(frozen=True, slots=True)
class Returns:
    """A single total result: `-> T`."""
    type: Type
    span: SrcSpan = _span()


@dataclass(frozen=True, slots=True)
class Cases:
    cases: tuple[CaseDecl, ...]
    span: SrcSpan = _span()


Result = Union[Returns, Cases]

# The case name of a `-> T` macro's one outcome: `exit result(v)`
RESULT_CASE = "result"


@dataclass(frozen=True, slots=True)
class EffectDecl:
    effect: Effect
    on: tuple[str, ...] = ()
    span: SrcSpan = _span()


@dataclass(frozen=True, slots=True)
class Access:
    """`read(entry.kind)`: a feedback field, `counter`, `entry`, `handler`
    or `entry.<schema field>`."""
    mode: AccessMode
    field: str
    span: SrcSpan = _span()


@dataclass(frozen=True, slots=True)
class FactUse:
    """`fact(subjects)`, or `fact<T>(subjects)` for a typed fact."""
    fact: Fact
    subjects: tuple[str, ...]
    type_arg: BuiltinType | None = None
    span: SrcSpan = _span()


@dataclass(frozen=True, slots=True)
class EdgeFact:
    """A fact established on one exit or case: `next.exact<int>(dst)`."""
    edge: str
    fact: FactUse
    span: SrcSpan = _span()


@dataclass(frozen=True, slots=True)
class Contract:
    effects: tuple[EffectDecl, ...] = ()
    feedback: tuple[Access, ...] = ()
    exits: tuple[str, ...] = ()
    requires: tuple[FactUse, ...] = ()
    ensures: tuple[EdgeFact, ...] = ()
    # Parameter modes of a row or macro: a linear parameter is taken by the
    # callee unless `borrows` lists it; a source parameter is read and left
    # intact unless `spends` lists it, and then it is spent on every case.
    borrows: tuple[str, ...] = ()
    spends: tuple[str, ...] = ()
    span: SrcSpan = _span()

    @property
    def effect_set(self) -> frozenset[Effect]:
        return frozenset(e.effect for e in self.effects)


# Graph

@dataclass(frozen=True, slots=True)
class Guard:
    """`guard cond else miss;`"""
    cond: Expr
    span: SrcSpan = _span()


@dataclass(frozen=True, slots=True)
class Let:
    """`let name:T = value;`; the name `_` discards a Unit or scalar."""
    name: str
    type: Type
    value: Expr
    span: SrcSpan = _span()


DISCARD = "_"


@dataclass(frozen=True, slots=True)
class Binding:
    """`dst = value` in a publish: a destination operand and its owner, or
    `null`.  A vector binding `d[] = s[]` (or `d[] = dup(s[])`) gives
    element i of the span operand d the owner taken from (or a second owner
    of) element i of the span operand s.  An owners binding `d[] = v` (owners)
    gives element i of d item i of v, a let or case value of type Span<O>
    with count(d) items; old(d) then retires the owners it displaces."""
    target: str
    value: Expr
    span: SrcSpan = _span()
    vector: bool = False
    owners: bool = False


@dataclass(frozen=True, slots=True)
class Debt:
    kind: DebtKind
    subject: str
    span: SrcSpan = _span()


@dataclass(frozen=True, slots=True)
class Publish:
    """Parallel publication, then retirement of `debts` in order, at the
    checkpoint `point`."""
    bindings: tuple[Binding, ...]
    debts: tuple[Debt, ...]
    point: str
    span: SrcSpan = _span()


Stmt = Union[Guard, Let, Publish]


@dataclass(frozen=True, slots=True)
class Target:
    """A block and the arguments bound to its parameters."""
    label: str
    args: tuple[Expr, ...]
    span: SrcSpan = _span()


@dataclass(frozen=True, slots=True)
class Goto:
    target: Target
    span: SrcSpan = _span()


@dataclass(frozen=True, slots=True)
class Branch:
    cond: Expr
    if_true: Target
    if_false: Target
    span: SrcSpan = _span()


@dataclass(frozen=True, slots=True)
class Edge:
    """`case -> Target(args);`: the case's payload is prepended to args."""
    case: str
    target: Target
    span: SrcSpan = _span()


@dataclass(frozen=True, slots=True)
class CallTerm:
    """A status call of a row or macro with one edge per declared case."""
    callee: str
    args: tuple[Expr, ...]
    edges: tuple[Edge, ...]
    span: SrcSpan = _span()


@dataclass(frozen=True, slots=True)
class Each:
    """Bounded iteration: `body` receives (element, index) per element of
    `over`, whose `advance body` is the sole backedge; `exhaust` follows
    the last.  `limit` None means the checked length captured at entry."""
    over: Expr
    limit: int | None
    body: str
    exhaust: Target
    span: SrcSpan = _span()


@dataclass(frozen=True, slots=True)
class Advance:
    body: str
    span: SrcSpan = _span()


@dataclass(frozen=True, slots=True)
class Exit:
    """An arm exit constructor, or a macro's case.  `resume` names the
    continuation point of an `enter`: where and with what the caller goes on
    when the child returns."""
    kind: str
    args: tuple[Expr, ...]
    span: SrcSpan = _span()
    resume: str | None = None
    # The family's completion arm that runs when a generator child returns
    # (`exit enter(c) resume P complete A`)
    complete: str | None = None


Term = Union[Goto, Branch, CallTerm, Each, Advance, Exit]


@dataclass(frozen=True, slots=True)
class Block:
    label: str
    params: tuple[Param, ...]
    stmts: tuple[Stmt, ...]
    term: Term
    span: SrcSpan = _span()


@dataclass(frozen=True, slots=True)
class Graph:
    """The first block is `entry()`."""
    blocks: tuple[Block, ...]
    span: SrcSpan = _span()


ENTRY = "entry"


# Declarations

@dataclass(frozen=True, slots=True)
class EnumDecl:
    name: str
    members: tuple[str, ...]
    span: SrcSpan = _span()


@dataclass(frozen=True, slots=True)
class Family:
    """A register instruction family: its operands and the exits its arms
    may take.  Its encoding is derived from the operand types (Encoding)."""
    name: str
    params: tuple[Param, ...]
    exits: tuple[str, ...]
    # The public stack ops that lower to the family; the first is the base
    # and event opcode of its arms' words
    ops: tuple[str, ...] = ()
    # The events of the family's observed wrapper (its arms' twins)
    events: tuple[MonitoringEvent, ...] = ()
    span: SrcSpan = _span()
    # The arm a miss restarts in (`generic(Arm)`), by its short name
    generic: str | None = None


@dataclass(frozen=True, slots=True)
class Placement:
    """Where one operand lives: its field kind, the bits it needs and the
    fields it takes (none for a vector, which follows in the payload).
    Consecutive small immediates share one field: `shift` is where an
    immediate starts in it, and `shared` says that a reader must mask."""
    operand: str
    kind: FieldKind
    bits: int
    slots: tuple[Slot, ...]
    shift: int = 0          # low bit of an immediate in its (first) field
    shared: bool = False    # an immediate that shares its field with another


@dataclass(frozen=True, slots=True)
class Encoding:
    """A family's derived encoding: placements in operand order, the
    number of operand words after the head (vectors aside), and the head
    fields that a family with spans fills with their first elements."""
    family: str
    placements: tuple[Placement, ...]
    words: int
    elements: tuple[Slot, ...] = ()


@dataclass(frozen=True, slots=True)
class Point:
    """A checkpoint: frame, pc and exception roles, and the family operands
    kept, spent and defined there."""
    name: str
    frame: FrameRole
    pc: PcRole
    exc: ExcRole
    keep: tuple[str, ...]
    spent: tuple[str, ...]
    defs: tuple[str, ...]
    span: SrcSpan = _span()

    @property
    def family(self) -> str:
        return self.name.split(".")[0]


@dataclass(frozen=True, slots=True)
class Arm:
    name: str
    region: Region
    contract: Contract
    graph: Graph
    span: SrcSpan = _span()
    # The static evidence that selects the arm (`select(...)`), for a family
    # whose arms never miss
    select: tuple[Select, ...] = ()
    # The classifier case whose recipe the arm's entry holds (`entry(k)`)
    entry: str | None = None
    # The family's observed wrapper, which runs under instrumented twins
    observes: bool = False
    # A completion arm (`completion(P)`): no word of its own; it runs in the
    # consumer's frame, from P's homes, when a generator child returns
    completion: str | None = None

    @property
    def family(self) -> str:
        return self.name.split(".")[0]


@dataclass(frozen=True, slots=True)
class Macro:
    """A typed macro: expanded in every consumer, no dispatch of its own.
    `graph` None is a pending macro, declared ahead of its body."""
    name: str
    schema_params: tuple[str, ...]
    params: tuple[Param, ...]
    result: Result
    contract: Contract
    graph: Graph | None
    span: SrcSpan = _span()


@dataclass(frozen=True, slots=True)
class EvidenceItem:
    key: str
    text: str
    span: SrcSpan = _span()


@dataclass(frozen=True, slots=True)
class CBinding:
    """The C entry point of a row and its return convention."""
    symbol: str
    convention: str
    evidence: tuple[EvidenceItem, ...]
    span: SrcSpan = _span()
    # The interpreter product always inlines the body; the function forms and
    # stubs call one out-of-line copy
    inline: bool = False


@dataclass(frozen=True, slots=True)
class Extern:
    """A component row: the one declaration of its logical signature.
    `test` lists the facts a `-> Bool` row establishes when true; `c` None
    is a pending row whose C body F2 supplies."""
    kind: RowKind
    name: str
    schema_params: tuple[str, ...]
    params: tuple[Param, ...]
    result: Result
    contract: Contract
    test: tuple[FactUse, ...]
    c: CBinding | None
    span: SrcSpan = _span()


Decl = Union[EnumDecl, Family, Point, Arm, Macro, Extern]


@dataclass(frozen=True, slots=True)
class SourceFile:
    filename: str
    decls: tuple[Decl, ...]
