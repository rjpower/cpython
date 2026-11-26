"""Resolution and structural checks of register instruction definitions.

`load(paths)` parses `.rop` files and checks them; `check(files)` checks
parsed files.  Both return a `Program`: the declarations by name, a
`GraphInfo` per arm and macro (the types of its names, block order,
predecessors and dominators) and the diagnostics.  F0 checks names, types,
graph shape, edges, exits and encodings; ownership, facts and effect
coverage are F1's, on the same Program.

Run as a script: `rop_check.py [FILE.rop ...]` (default: every file in
Python/register_ops) prints the diagnostics and the pending rows and
macros, and exits 1 on any diagnostic.
"""

import argparse
import itertools
import math
import sys
from collections.abc import Iterable, Mapping
from dataclasses import dataclass, field, replace
from pathlib import Path
from typing import Any

from rop_catalog import (HEAD_FLAG_ENUMS, CONVENTIONS, ENVIRONMENT, CSource, Convention, EVIDENCE_KEYS, EVIDENCE_PRIMITIVE,
                         EVIDENCE_REQUIRED, FEEDBACK_ROOTS, HOME_GROUPS, INDEX_BITS, LINEAR_HANDLES,
                         TARGET_BITS, TRANSFER_EXITS, AccessMode,
                         LINEAR_SCHEMA_CTORS, READ_ONLY_EFFECTS, SCHEMA_FIELDS,
                         OPTIONAL, SELECT_DIMENSIONS, SELECT_FACTS, SLOT_ORDER,
                         WIDEST_IMMEDIATE, DebtKind,
                         Effect, ExcRole, ExitKind, Fact, FieldKind, FrameRole, Handle, PcRole,
                         Region, RowKind, Scalar, Schema, SchemaCtor, Select, Slot, Wrap)
from rop_model import (RESULT_CASE, Advance, Arm, Binding, Block, BoolLit, Branch, Call,
                       CallTerm, Cases, Contract, Debt, Decl, Diagnostic, Each, Edge, Encoding,
                       EnumDecl, Placement,
                       EnumType, Exit, Expr, Extern, FactUse, Family, Goto, Graph,
                       Guard, HandleType, Let, Macro, Name, Nat, Null, PairType, Param,
                       Point, Publish, Result, Returns, RopError, ScalarType,
                       SchemaRef, SchemaType, SchemaVar, SourceFile, SpanLit, SrcSpan,
                       Target, Term, Type, WrapType, DISCARD, ENTRY, NOWHERE)
from rop_print import expr_text, type_text
import rop_header
import rop_syntax
from ir_contract import Row

ROOT = Path(__file__).resolve().parents[2]
ROP_DIR = ROOT / "Python" / "register_ops"


def T(h: Handle) -> HandleType:
    return HandleType(h)


def S(s: Scalar) -> ScalarType:
    return ScalarType(s)


BOOL, UNIT, SIZE = S(Scalar.Bool), S(Scalar.Unit), S(Scalar.Size)
SRC, DST, B, O = T(Handle.Src), T(Handle.Dst), T(Handle.B), T(Handle.O)

# The payload types of each arm exit.  None stands for the family's own
# feedback schema (`again(Ticket<K>)`).
EXIT_PAYLOAD: dict[ExitKind, tuple[Type | None, ...]] = {
    ExitKind.next: (),
    ExitKind.branch: (T(Handle.Head),),
    ExitKind.jump: (T(Handle.Head),),
    ExitKind.miss: (),
    ExitKind.again: (None,),
    ExitKind.raise_: (T(Handle.Point), T(Handle.Err)),
    ExitKind.unwind: (T(Handle.Point), T(Handle.Err)),
    # The new frame; `resume P` names the caller's point at the return
    ExitKind.enter: (T(Handle.NewFrame),),
    ExitKind.return_: (T(Handle.Frame), T(Handle.Head)),
    ExitKind.yield_: (O,),
    ExitKind.exit: (O,),
    ExitKind.redirect: (T(Handle.Route),),
    ExitKind.dispatch: (T(Handle.Handler),),
}
EXIT_NAMES = {e.value: e for e in ExitKind}

# Operand roles: the types a point may list as kept or spent sources, and
# as definitions
SOURCE_HANDLES = frozenset({Handle.Src, Handle.Local, Handle.Cell, Handle.Free,
                            *HOME_GROUPS, Handle.Part, Handle.Args})
DEST_HANDLES = frozenset({Handle.Dst, Handle.Local, *HOME_GROUPS, Handle.Part})
PART = T(Handle.Part)


def group_parts(t: Type | None) -> tuple[str, ...]:
    """The projections of a home group type; () for any other type"""
    return HOME_GROUPS.get(t.handle, ()) if isinstance(t, HandleType) else ()


def home_params(params: Iterable[Param]) -> tuple[Param, ...]:
    """The operands as homes: each home group stands for its projections
    `g.p`, which are temporaries the instruction reads and writes"""
    out: list[Param] = []
    for p in params:
        parts = group_parts(p.type)
        if parts:
            out.extend(Param(f"{p.name}.{q}", PART, p.span) for q in parts)
        else:
            out.append(p)
    return tuple(out)


def expand_group(name: str, operands: Mapping[str, Type]) -> tuple[str, ...]:
    """A name in a point list or a spent debt: a group is its projections"""
    parts = group_parts(operands.get(name))
    return tuple(f"{name}.{q}" for q in parts) if parts else (name,)


def expand_debts(debts: Iterable[Debt], operands: Mapping[str, Type]) -> tuple[Debt, ...]:
    """spent(g) of a home group is spent(g.p) of each projection, in order"""
    return tuple(replace(d, subject=n) for d in debts
                 for n in (expand_group(d.subject, operands)
                           if d.kind == DebtKind.spent else (d.subject,)))


# Language primitives (operation dictionary, P).  Actions may stand only as
# the outermost expression of a `let`.
@dataclass(frozen=True, slots=True)
class Primitive:
    name: str
    params: tuple[Type, ...]
    result: Type
    action: bool


PRIMITIVES = {p.name: p for p in (
    Primitive("use", (SRC,), B, False),
    Primitive("dup", (B,), O, True),
    Primitive("move", (WrapType(Wrap.InOut, O),), O, True),
    Primitive("acquire", (SRC,), O, True),
)}
# unwrap(Maybe<T>) -> T, given `present` (F1); candidate(Family.Arm[, recipe]);
# count(Span<T>) -> Size, the number of items, which neither reads nor
# spends them
UNWRAP = "unwrap"
CANDIDATE = "candidate"
COUNT = "count"
SPECIAL_PRIMITIVES = frozenset((UNWRAP, CANDIDATE, COUNT))


@dataclass(frozen=True, slots=True)
class GraphInfo:
    """What the checks computed of one arm or macro graph.  `types` maps
    every name the graph can read (operands, environment, block parameters
    and lets) to its type; `order` lists reachable blocks in reverse
    postorder; `idom` gives each reachable block's immediate dominator
    (None for entry), computed without `advance` backedges."""
    definition: str
    types: Mapping[str, Type]
    order: tuple[str, ...]
    preds: Mapping[str, tuple[str, ...]]
    idom: Mapping[str, str | None]
    exits_used: frozenset[str]


@dataclass(frozen=True)
class Program:
    files: tuple[SourceFile, ...]
    enums: Mapping[str, EnumDecl]
    families: Mapping[str, Family]
    points: Mapping[str, Point]
    arms: Mapping[str, Arm]
    macros: Mapping[str, Macro]
    externs: Mapping[str, Extern]
    graphs: Mapping[str, GraphInfo]
    encodings: Mapping[str, Encoding]
    diagnostics: tuple[Diagnostic, ...]
    # Completion arms (`completion(P)`), which have no word and no handler
    completions: Mapping[str, Arm] = field(default_factory=dict)

    @property
    def pending(self) -> tuple[str, ...]:
        """Rows without a C body and macros without a graph."""
        rows = [n for n, e in self.externs.items() if e.c is None]
        macros = [n for n, m in self.macros.items() if m.graph is None]
        return tuple(sorted(rows) + sorted(macros))

    def arms_of(self, family: str) -> tuple[Arm, ...]:
        return tuple(a for a in self.arms.values() if a.family == family)


# Helpers

def is_linear(t: Type) -> bool:
    """Whether a value of type t carries an obligation: a linear handle or
    ticket, or a Maybe, Span or Pair of one.  An InOut is a home, not an
    owner: `move` takes the owner out of it."""
    if isinstance(t, HandleType):
        return t.handle in LINEAR_HANDLES
    if isinstance(t, WrapType):
        return t.wrap != Wrap.InOut and is_linear(t.arg)
    if isinstance(t, PairType):
        return is_linear(t.first) or is_linear(t.second)
    return isinstance(t, SchemaType) and t.ctor in LINEAR_SCHEMA_CTORS


def is_source(t: Type) -> bool:
    """Whether t names source homes: a source operand type, or a Maybe or
    Span of one."""
    if isinstance(t, WrapType) and t.wrap in (Wrap.Maybe, Wrap.Span):
        return is_source(t.arg)
    return isinstance(t, HandleType) and t.handle in SOURCE_HANDLES


def optional_sources(fam: Family) -> list[str]:
    """The family's optional source operands (Maybe<Src>)"""
    return [p.name for p in fam.params
            if isinstance(p.type, WrapType) and p.type.wrap is Wrap.Maybe
            and is_source(p.type)]


def select_facts(fam: Family, a: Arm) -> list[tuple[Fact, str]]:
    """The facts an arm's select items state at its entry (SELECT_FACTS),
    with OPTIONAL expanded to each of the family's optional operands"""
    out = []
    for m in a.select:
        for fact, subject in SELECT_FACTS.get(m, ()):
            subjects = optional_sources(fam) if subject == OPTIONAL else [subject]
            out.extend((fact, s) for s in subjects)
    return out


def substitute(t: Type, binding: Mapping[str, Schema]) -> Type:
    if isinstance(t, SchemaType) and isinstance(t.schema, SchemaVar):
        bound = binding.get(t.schema.name)
        return SchemaType(t.ctor, bound) if bound else t
    if isinstance(t, WrapType):
        return WrapType(t.wrap, substitute(t.arg, binding))
    if isinstance(t, PairType):
        return PairType(substitute(t.first, binding), substitute(t.second, binding))
    return t


def unify(param: Type, arg: Type, binding: dict[str, Schema]) -> bool:
    """Whether `arg` has type `param`, binding the schema variables."""
    if isinstance(param, SchemaType) and isinstance(param.schema, SchemaVar):
        if not isinstance(arg, SchemaType) or arg.ctor != param.ctor:
            return False
        if isinstance(arg.schema, SchemaVar):
            return False
        bound = binding.setdefault(param.schema.name, arg.schema)
        return bound == arg.schema
    if isinstance(param, WrapType):
        return (isinstance(arg, WrapType) and arg.wrap == param.wrap
                and unify(param.arg, arg.arg, binding))
    if isinstance(param, PairType):
        return (isinstance(arg, PairType) and unify(param.first, arg.first, binding)
                and unify(param.second, arg.second, binding))
    return param == arg


def feedback_schemas(params: Iterable[Param]) -> list[SchemaRef]:
    return [p.type.schema for p in params
            if isinstance(p.type, SchemaType) and p.type.ctor == SchemaCtor.Fb]


def result_cases(result: Result) -> dict[str, tuple[Param, ...]]:
    if isinstance(result, Returns):
        return {RESULT_CASE: (Param(RESULT_CASE, result.type, result.span),)}
    return {c.name: c.params for c in result.cases}


def imm_bits(t: Type, enums: Mapping[str, EnumDecl]) -> int | None:
    """The bits an immediate of type t needs, or None if t is no immediate."""
    if isinstance(t, ScalarType):
        return t.scalar.bits if t.scalar != Scalar.Unit else None
    if isinstance(t, EnumType):
        members = len(enums[t.name].members) if t.name in enums else 1
        return max(1, math.ceil(math.log2(members)))
    if isinstance(t, WrapType) and t.wrap == Wrap.Maybe:
        inner = imm_bits(t.arg, enums)
        return None if inner is None else inner + 1
    return None


HOME_HANDLES = frozenset({Handle.Src, Handle.Dst, Handle.Local, Handle.Cell,
                          Handle.Free, *HOME_GROUPS})


def field_kind(t: Type) -> FieldKind | None:
    """The kind of field an operand of type t takes, or None if t is no
    operand type."""
    inner = t.arg if isinstance(t, WrapType) and t.wrap == Wrap.Maybe else t
    if isinstance(inner, HandleType):
        h = inner.handle
        if h in HOME_HANDLES:
            return FieldKind.home
        if h == Handle.Name:
            return FieldKind.name
        if h == Handle.Const:
            return FieldKind.const
        if h in TARGET_BITS and inner is t:
            return FieldKind.target
        if h == Handle.Args and inner is t:
            return FieldKind.vector
        return None
    if isinstance(t, SchemaType) and t.ctor == SchemaCtor.Fb:
        return FieldKind.feedback
    if (isinstance(t, WrapType) and t.wrap == Wrap.Span and isinstance(t.arg, HandleType)
            and t.arg.handle in HOME_HANDLES):
        return FieldKind.vector
    if isinstance(t, EnumType) and t.name in HEAD_FLAG_ENUMS:
        return FieldKind.flag
    if isinstance(inner, (ScalarType, EnumType)) and imm_bits(inner, {}) is not None:
        return FieldKind.imm
    return None


def is_args(t: Type) -> bool:
    return isinstance(t, HandleType) and t.handle == Handle.Args


def operand_bits(t: Type, kind: FieldKind, enums: Mapping[str, EnumDecl]) -> int:
    if kind == FieldKind.imm:
        bits = imm_bits(t, enums)
        assert bits is not None
        return bits
    if kind == FieldKind.target:
        assert isinstance(t, HandleType)
        return TARGET_BITS[t.handle]
    return INDEX_BITS


def pack(family: str, params: Iterable[Param], enums: Mapping[str, EnumDecl]
         ) -> tuple[Encoding, list[tuple[Param, str]]]:
    """The packing rule.  Operands take the fixed fields A, B, C, D, then
    the operand words' P0..P7, in declaration order; an immediate wider
    than the next field takes it and the one after, if both are in the same
    word.  An immediate that fits in the unused high bits of the field the
    immediate before it took shares that field, low bits first.  A span and an argument descriptor (Args) take one field for
    their count.  A span's elements, the spans in declaration order, fill
    the head's fields that no operand takes, then follow in operand words
    of four fields each; Args's homes follow in the payload.  Returns the
    encoding and the operands that cannot be packed, with the reason."""
    placements: list[Placement] = []
    problems: list[tuple[Param, str]] = []
    i = 0
    used = 0        # bits taken by immediates in field i - 1
    spans = False
    for p in params:
        kind = field_kind(p.type)
        spans |= kind == FieldKind.vector and not is_args(p.type)
        if kind is None:
            problems.append((p, f"operand {p.name}:{type_text(p.type)} has no field "
                             "kind"))
            continue
        if kind == FieldKind.flag:
            # A flag of the handler field takes no field of the word
            if len(enums[p.type.name].members) != 2:
                problems.append((p, f"operand {p.name}: a flag enum has two members"))
                continue
            placements.append(Placement(p.name, kind, 1, (), shift=HEAD_FLAG_ENUMS[p.type.name]))
            continue
        bits = operand_bits(p.type, kind, enums)
        if bits > WIDEST_IMMEDIATE:
            problems.append((p, f"operand {p.name}:{type_text(p.type)} needs {bits} "
                             f"bits; an immediate has at most {WIDEST_IMMEDIATE}"))
            continue
        if i >= len(SLOT_ORDER):
            problems.append((p, f"operand {p.name}: every field is taken"))
            continue
        if (kind == FieldKind.imm and used and placements
                and placements[-1].kind == FieldKind.imm
                and len(placements[-1].slots) == 1
                and used + bits <= placements[-1].slots[0].width):
            before = placements[-1]
            placements[-1] = replace(before, shared=True)
            placements.append(Placement(p.name, kind, bits, before.slots,
                                        shift=used, shared=True))
            used += bits
            continue
        first = SLOT_ORDER[i]
        used = 0
        if bits <= first.width:
            placements.append(Placement(p.name, kind, bits, (first,)))
            used = bits if kind == FieldKind.imm else 0
            i += 1
            continue
        second = SLOT_ORDER[i + 1] if i + 1 < len(SLOT_ORDER) else None
        if second is None or second.word != first.word or (
                first.width + second.width < bits):
            problems.append((p, f"operand {p.name}:{type_text(p.type)} needs {bits} "
                             f"bits; field {first.name} and the next field of its word "
                             "cannot hold it"))
            continue
        placements.append(Placement(p.name, kind, bits, (first, second)))
        i += 2
    words = max((s.word for pl in placements for s in pl.slots), default=0)
    elements = tuple(s for s in SLOT_ORDER[i:] if s.word == 0) if spans else ()
    return Encoding(family, tuple(placements), words, elements), problems


# The checker

@dataclass
class Checker:
    """Accumulates diagnostics while the checks walk the declarations."""
    files: tuple[SourceFile, ...]
    diagnostics: list[Diagnostic] = field(default_factory=list)
    enums: dict[str, EnumDecl] = field(default_factory=dict)
    members: dict[str, list[str]] = field(default_factory=dict)
    families: dict[str, Family] = field(default_factory=dict)
    points: dict[str, Point] = field(default_factory=dict)
    arms: dict[str, Arm] = field(default_factory=dict)
    macros: dict[str, Macro] = field(default_factory=dict)
    externs: dict[str, Extern] = field(default_factory=dict)
    graphs: dict[str, GraphInfo] = field(default_factory=dict)
    callees: dict[str, frozenset[str]] = field(default_factory=dict)
    encodings: dict[str, Encoding] = field(default_factory=dict)
    completions: dict[str, Arm] = field(default_factory=dict)

    def arm(self, name: str) -> Arm:
        """A word arm or a completion arm"""
        return self.arms[name] if name in self.arms else self.completions[name]

    def report(self, span: SrcSpan, definition: str, message: str) -> None:
        self.diagnostics.append(Diagnostic(span, definition, message))


def check(files: Iterable[SourceFile]) -> Program:
    c = Checker(tuple(files))
    collect(c)
    for e in c.enums.values():
        check_enum(c, e)
    for fam in c.families.values():
        check_family(c, fam)
    for p in c.points.values():
        check_point(c, p)
    for x in c.externs.values():
        check_extern(c, x)
    for m in c.macros.values():
        check_macro(c, m)
    for a in c.arms.values():
        check_arm(c, a)
    for a in c.completions.values():
        check_completion_arm(c, a)
    for fam in c.families.values():
        check_selection(c, fam)
    check_macro_recursion(c)
    return Program(c.files, dict(c.enums), dict(c.families), dict(c.points),
                   dict(c.arms), dict(c.macros), dict(c.externs), dict(c.graphs),
                   dict(c.encodings), tuple(sorted(c.diagnostics, key=diagnostic_order)),
                   dict(c.completions))


def diagnostic_order(d: Diagnostic) -> tuple[str, int, int]:
    return (d.span.filename, d.span.line, d.span.column)


def load(paths: Iterable[Path], header: Mapping[str, Row] | None = None) -> Program:
    """Parse and check, and hold the externs to the header's rows (`header`,
    by default pycore_ir_components.h's).  A syntax error is the one
    diagnostic of an empty program."""
    files = []
    for path in paths:
        try:
            files.append(rop_syntax.parse_file(path))
        except RopError as e:
            return Program((), {}, {}, {}, {}, {}, {}, {}, {}, (e.diagnostic,))
    program = check(files)
    rows = rop_header.header_rows() if header is None else header
    extra = rop_header.disagreements(program.externs, rows)
    return replace(program, diagnostics=tuple(sorted(
        program.diagnostics + tuple(extra), key=diagnostic_order)))


def collect(c: Checker) -> None:
    callables: dict[str, Decl] = {}
    for f in c.files:
        for d in f.decls:
            if isinstance(d, EnumDecl):
                table: dict[str, Any] = c.enums
            elif isinstance(d, Family):
                table = c.families
            elif isinstance(d, Point):
                table = c.points
            elif isinstance(d, Arm):
                table = c.arms if d.completion is None else c.completions
                if d.name in c.arms or d.name in c.completions:
                    first = c.arm(d.name)
                    c.report(d.span, d.name, f"{d.name} is declared twice; first at "
                             f"{first.span}")
                    continue
            elif isinstance(d, Macro):
                table = c.macros
            else:
                table = c.externs
            if d.name in table:
                c.report(d.span, d.name, f"{d.name} is declared twice; first at "
                         f"{table[d.name].span}")
                continue
            if isinstance(d, (Macro, Extern)):
                if d.name in PRIMITIVES or d.name in SPECIAL_PRIMITIVES:
                    c.report(d.span, d.name, f"{d.name} is a language primitive")
                    continue
                if d.name in callables:
                    c.report(d.span, d.name, f"{d.name} is declared twice; first at "
                             f"{callables[d.name].span}")
                    continue
                callables[d.name] = d
            table[d.name] = d
    for e in c.enums.values():
        for m in e.members:
            c.members.setdefault(m, []).append(e.name)


def check_enum(c: Checker, e: EnumDecl) -> None:
    if len(set(e.members)) != len(e.members):
        c.report(e.span, e.name, "an enum member is listed twice")


def check_type(c: Checker, t: Type, span: SrcSpan, definition: str) -> None:
    if isinstance(t, EnumType) and t.name not in c.enums:
        c.report(span, definition, f"Enum<{t.name}>: no enum {t.name} is declared")
    elif isinstance(t, WrapType):
        check_type(c, t.arg, span, definition)
    elif isinstance(t, PairType):
        check_type(c, t.first, span, definition)
        check_type(c, t.second, span, definition)


def check_params(c: Checker, params: Iterable[Param], definition: str,
                 taken: Iterable[str] = ()) -> None:
    seen = set(taken)
    for p in params:
        check_type(c, p.type, p.span, definition)
        if p.name in seen or p.name in ENVIRONMENT:
            c.report(p.span, definition, f"parameter {p.name} is defined twice")
        seen.add(p.name)


def check_family(c: Checker, fam: Family) -> None:
    check_params(c, fam.params, fam.name)
    if "." in fam.name:
        c.report(fam.span, fam.name, "a family name is not qualified")
    for name in fam.exits:
        if name not in EXIT_NAMES:
            c.report(fam.span, fam.name, f"exit {name} is not an arm exit (one of: "
                     f"{', '.join(EXIT_NAMES)})")
    if len(set(fam.exits)) != len(fam.exits):
        c.report(fam.span, fam.name, "an exit is listed twice")
    if not fam.ops and any(a.family == fam.name for a in c.arms.values()):
        c.report(fam.span, fam.name, "a family with arms names the stack ops that "
                 "lower to it: ops(NAME, ...)")
    if len(set(fam.ops)) != len(fam.ops):
        c.report(fam.span, fam.name, "a stack op is listed twice")
    if len(set(fam.events)) != len(fam.events):
        c.report(fam.span, fam.name, "an event is listed twice")
    check_generic(c, fam)
    schemas = feedback_schemas(fam.params)
    if len(schemas) > 1:
        c.report(fam.span, fam.name, "a family has at most one feedback operand")
    if "again" in fam.exits and not schemas:
        c.report(fam.span, fam.name, "exit again needs a feedback operand: its ticket "
                 "is over the family's schema")
    encoding, problems = pack(fam.name, fam.params, c.enums)
    for p, message in problems:
        c.report(p.span, fam.name, message)
    if not problems:
        c.encodings[fam.name] = encoding


def check_generic(c: Checker, fam: Family) -> None:
    """`generic(Arm)`: the shared arm a miss restarts in, which itself does
    not miss.  A family whose arms miss declares one."""
    if fam.generic is None:
        return
    name = f"{fam.name}.{fam.generic}"
    arm = c.arms.get(name)
    if arm is None:
        c.report(fam.span, fam.name, f"generic({fam.generic}): no arm {name}")
        return
    if arm.region is Region.inline:
        c.report(fam.span, fam.name, f"generic({fam.generic}): {name} is inline; "
                 "the generic arm is shared, or split when it leaves the frame")
    if "miss" in arm.contract.exits:
        c.report(fam.span, fam.name, f"generic({fam.generic}): {name} misses; the "
                 "generic arm is where a miss restarts")
    if arm.observes or arm.select:
        c.report(fam.span, fam.name, f"generic({fam.generic}): {name} is selected "
                 "or observes; the generic arm is reached by a miss")


def check_point(c: Checker, p: Point) -> None:
    parts = p.name.split(".")
    fam = c.families.get(parts[0])
    if len(parts) != 2 or fam is None:
        c.report(p.span, p.name, f"point {p.name} does not name a family's point "
                 "(Family.Point)")
        return
    # A point may list a whole group or any of its projections
    operands = {q.name: q.type for q in fam.params + home_params(fam.params)}

    def role(names: tuple[str, ...], what: str, handles: frozenset[Handle]) -> None:
        for n in names:
            t = operands.get(n)
            if t is None:
                c.report(p.span, p.name, f"{what}({n}): {fam.name} has no operand {n}")
                continue
            inner = t.arg if isinstance(t, WrapType) else t
            if handles and not (isinstance(inner, HandleType) and inner.handle in handles):
                c.report(p.span, p.name, f"{what}({n}): operand {n} is "
                         f"{type_text(t)}, not a {what} role")

    role(p.keep, "keep", SOURCE_HANDLES)
    role(p.spent, "spent", SOURCE_HANDLES)
    role(p.defs, "defs", DEST_HANDLES)


def check_contract(c: Checker, contract: Contract, definition: str,
                   params: tuple[Param, ...], edges: Mapping[str, tuple[Param, ...]],
                   exits_allowed: bool) -> None:
    effects = [e.effect for e in contract.effects]
    if len(set(effects)) != len(effects):
        c.report(contract.span, definition, "an effect is listed twice")
    # `releases on (d)` of a destination is the release of d's displaced
    # owner when d names a local's home, which may run Python; when d is a
    # temporary its old owner is vacant by construction.  So the arm's own
    # effect list states the temporary class, and the `on` the local one.
    # A release on any other operand closes a binding on every word.
    types = {p.name: p.type for p in params}
    on_other = [n for e in contract.effects if e.effect is Effect.releases
                for n in e.on if n in types and not is_dst(types[n])]
    if on_other and Effect.runs_python not in effects:
        c.report(contract.span, definition, f"releases on ({', '.join(on_other)}) "
                 "closes an operand's binding, which may run Python: declare "
                 "runs_python")
    names = {p.name for p in params}
    for e in contract.effects:
        for n in e.on:
            if n not in names:
                c.report(e.span, definition, f"{e.effect.value} on ({n}): no operand {n}")
    schemas = feedback_schemas(params)
    for a in contract.feedback:
        root, _, rest = a.field.partition(".")
        if not schemas:
            c.report(a.span, definition, f"{a.mode.value}({a.field}): no feedback "
                     "operand")
        elif root not in FEEDBACK_ROOTS or (rest and root != "entry"):
            c.report(a.span, definition, f"{a.mode.value}({a.field}): not counter, "
                     "entry, handler or entry.<field>")
        elif rest and not any(isinstance(s, Schema) and rest in SCHEMA_FIELDS[s]
                              for s in schemas):
            known = sorted({f for s in schemas if isinstance(s, Schema)
                            for f in SCHEMA_FIELDS[s]})
            c.report(a.span, definition, f"{a.mode.value}({a.field}): the schema has no "
                     f"field {rest} (fields: {', '.join(known)})")
    for n in contract.borrows:
        if n not in types or not is_linear(types[n]):
            c.report(contract.span, definition, f"borrows({n}): {n} is not a linear "
                     "parameter")
    for n in contract.spends:
        if n not in types or not is_source(types[n]):
            c.report(contract.span, definition, f"spends({n}): {n} is not a source "
                     "parameter")
    if contract.spends and Effect.writes not in contract.effect_set:
        c.report(contract.span, definition, f"spends({', '.join(contract.spends)}): "
                 "spending a source takes it out of its home; declare writes")
    if (any(a.mode is AccessMode.write for a in contract.feedback)
            and not contract.effect_set & {Effect.writes, Effect.runs_python,
                                            Effect.polls, Effect.switches_frame}):
        c.report(contract.span, definition, "a feedback write writes memory: "
                 "declare writes")
    if contract.exits and not exits_allowed:
        c.report(contract.span, definition, "only an arm declares exits")
    for n in contract.exits:
        if n not in EXIT_NAMES:
            c.report(contract.span, definition, f"exit {n} is not an arm exit")
    # A row runs in the word's environment, so its requires may name it
    for f in contract.requires:
        check_fact(c, f, definition, names | set(ENVIRONMENT))
    for ef in contract.ensures:
        if ef.edge not in edges:
            c.report(ef.span, definition, f"ensures {ef.edge}.{ef.fact.fact.value}: "
                     f"{ef.edge} is not an exit or case of {definition}")
            continue
        check_fact(c, ef.fact, definition, names | {p.name for p in edges[ef.edge]})


def check_fact(c: Checker, f: FactUse, definition: str, names: set[str]) -> None:
    if f.fact.typed and f.type_arg is None:
        c.report(f.span, definition, f"fact {f.fact.value} names a builtin type: "
                 f"{f.fact.value}<T>(...)")
    elif not f.fact.typed and f.type_arg is not None:
        c.report(f.span, definition, f"fact {f.fact.value} takes no type argument")
    if len(f.subjects) != f.fact.arity:
        c.report(f.span, definition, f"fact {f.fact.value} takes {f.fact.arity} "
                 f"subject{'s' if f.fact.arity > 1 else ''}, not {len(f.subjects)}")
    for s in f.subjects:
        if s not in names:
            c.report(f.span, definition, f"fact {f.fact.value}({', '.join(f.subjects)}): "
                     f"no operand {s}")


def check_result(c: Checker, result: Result, definition: str,
                 params: tuple[Param, ...]) -> None:
    if isinstance(result, Returns):
        check_type(c, result.type, result.span, definition)
        return
    seen: set[str] = set()
    for case in result.cases:
        if case.name in seen:
            c.report(case.span, definition, f"case {case.name} is declared twice")
        seen.add(case.name)
        check_params(c, case.params, definition, (p.name for p in params))


def check_extern(c: Checker, x: Extern) -> None:
    check_params(c, x.params, x.name)
    check_result(c, x.result, x.name, x.params)
    check_contract(c, x.contract, x.name, x.params, result_cases(x.result), False)
    check_no_dst(c, x.params, x.name)
    if isinstance(x.result, Returns) and Effect.raises in x.contract.effect_set:
        c.report(x.span, x.name, "a row with one outcome cannot raise: declare its "
                 "error as a case")
    if x.test:
        if not (isinstance(x.result, Returns) and x.result.type == BOOL):
            c.report(x.span, x.name, "test(...) is the true edge of a -> Bool row")
        for f in x.test:
            check_fact(c, f, x.name, {p.name for p in x.params})
    if x.c is None:
        return
    conv = CONVENTIONS.get(x.c.convention)
    if conv is None:
        c.report(x.c.span, x.name, f"convention {x.c.convention} is not in the "
                 f"catalog (one of: {', '.join(CONVENTIONS)})")
    elif conv.single != isinstance(x.result, Returns):
        shape = "-> T" if conv.single else "cases(...)"
        c.report(x.c.span, x.name, f"convention {conv.name} maps onto a {shape} row")
    elif isinstance(x.result, Cases) and conv.transfer:
        check_transfer_row(c, x)
    elif isinstance(x.result, Cases) and conv.indexed:
        for case in x.result.cases:
            if len(case.params) > 1:
                c.report(case.span, x.name, f"case {case.name} has {len(case.params)} "
                         f"payload values; {conv.name} gives at most one")
    elif isinstance(x.result, Cases):
        if len(x.result.cases) != len(conv.outcomes):
            c.report(x.c.span, x.name, f"convention {conv.name} has "
                     f"{len(conv.outcomes)} outcomes; the row declares "
                     f"{len(x.result.cases)} cases")
        for case, outcome in zip(x.result.cases, conv.outcomes):
            if len(case.params) != len(outcome.payload):
                c.report(case.span, x.name, f"case {case.name} has {len(case.params)} "
                         f"payload values; outcome {outcome.test.value} of "
                         f"{conv.name} gives {len(outcome.payload)}")
            elif outcome.raised and case.params[-1].type != T(Handle.Err):
                c.report(case.span, x.name, f"case {case.name} is the raised outcome "
                         f"of {conv.name}: its payload is Err")
    if x.c.inline and conv is not None:
        why = inline_refusal(x, conv)
        if why:
            c.report(x.c.span, x.name, f"c {x.c.symbol} cannot be inline: {why}")
    keys = [e.key for e in x.c.evidence]
    for e in x.c.evidence:
        if e.key not in EVIDENCE_KEYS:
            c.report(e.span, x.name, f"evidence {e.key} is not one of: "
                     f"{', '.join(EVIDENCE_KEYS)}")
    if len(set(keys)) != len(keys):
        c.report(x.c.span, x.name, "an evidence item is given twice")
    required = EVIDENCE_REQUIRED + (EVIDENCE_PRIMITIVE if x.kind == RowKind.primitive
                                    else ())
    missing = [k for k in required if k not in keys]
    if missing:
        c.report(x.c.span, x.name, f"c {x.c.symbol} needs evidence: "
                 f"{', '.join(missing)}")


def inline_refusal(x: Extern, conv: Convention) -> str | None:
    """Why a row cannot be declared inline: the generator writes its
    out-of-line copy from the declared signature, so the call must be a
    plain C call whose arguments are the declared parameters"""
    if x.kind == RowKind.primitive:
        return "a primitive row lowers to a leaf operation"
    if conv.transfer or conv.indexed:
        return f"convention {conv.name} leaves the word or indexes its result"
    if any(CSource.out in o.payload for o in conv.outcomes):
        return f"convention {conv.name} passes a C out parameter"
    return None


def check_no_dst(c: Checker, params: tuple[Param, ...], definition: str) -> None:
    """Only `publish` writes homes: no row or macro takes a destination.
    A row takes a projection of a home group as a Src, never the group."""
    for p in params:
        t = p.type
        while isinstance(t, WrapType):
            t = t.arg
        if group_parts(t) or t == PART:
            c.report(p.span, definition, f"parameter {p.name}:{type_text(p.type)}: a "
                     "row takes one home of a group, a projection g.p, as a Src")
            continue
        if t == DST:
            c.report(p.span, definition, f"parameter {p.name}:{type_text(p.type)}: a "
                     "row cannot write a home; return the value and publish it")


def check_transfer_row(c: Checker, x: Extern) -> None:
    """A transfer row's cases are the exits its callee performs, each with
    that exit's payload; a raise's point is the arm's."""
    assert isinstance(x.result, Cases)
    for case in x.result.cases:
        kind = TRANSFER_EXITS.get(case.name)
        if kind is None:
            c.report(case.span, x.name, f"case {case.name} of a transfer row names "
                     f"the exit its callee performs (one of: "
                     f"{', '.join(TRANSFER_EXITS)})")
            continue
        want = tuple(w for w in EXIT_PAYLOAD[EXIT_NAMES[kind]] if w != T(Handle.Point))
        have = tuple(p.type for p in case.params)
        if have != want:
            c.report(case.span, x.name, f"case {case.name}: exit {kind} carries "
                     f"({', '.join(type_text(w) for w in want if w is not None)})")


def check_macro(c: Checker, m: Macro) -> None:
    check_params(c, m.params, m.name)
    check_no_dst(c, m.params, m.name)
    check_result(c, m.result, m.name, m.params)
    check_contract(c, m.contract, m.name, m.params, result_cases(m.result), False)
    if m.graph is not None:
        check_graph(c, m.name, m.graph, m.params, None, m)


def check_arm(c: Checker, a: Arm) -> None:
    parts = a.name.split(".")
    fam = c.families.get(parts[0])
    if len(parts) != 2 or fam is None:
        c.report(a.span, a.name, f"arm {a.name} does not name a family's arm "
                 "(Family.Arm)")
        return
    if a.displacing and (a.observes or a.select or a.trailing or a.entry is not None
                         or a.region is not Region.split):
        c.report(a.span, a.name, "a displacing arm is split, and neither selected, "
                 "observed, trailing nor adaptive")
    if "dispatch" in a.contract.exits and not a.displacing:
        c.report(a.contract.span, a.name, "only a displacing arm has the exit dispatch")
    if a.displacing and "dispatch" not in a.contract.exits:
        c.report(a.contract.span, a.name, "a displacing arm exits by dispatch")
    if a.trailing and (a.observes or a.select or a.entry is not None):
        c.report(a.span, a.name, "a trailing arm belongs to the instruction before "
                 "it: it is not selected, observed or adaptive, and fires no event")
    edges = {n: () for n in a.contract.exits}
    check_contract(c, a.contract, a.name, fam.params, edges, True)
    for n in a.contract.exits:
        if n not in fam.exits:
            c.report(a.contract.span, a.name, f"exit {n} is not an exit of family "
                     f"{fam.name} ({', '.join(fam.exits)})")
    frame_exits = sorted(set(a.contract.exits) & FRAME_EXITS)
    if a.region is Region.shared and frame_exits:
        c.report(a.contract.span, a.name, f"a shared arm completes the word in a stub, "
                 f"which cannot leave the frame ({', '.join(frame_exits)}): make it "
                 "split")
    dimensions = [select_dimension(m) for m in a.select]
    for d in sorted({d for d in dimensions if dimensions.count(d) > 1}):
        c.report(a.span, a.name, f"select names two members of dimension {d}; one "
                 "holds")
    if {Select.present, Select.absent} & set(a.select) and not optional_sources(fam):
        c.report(a.span, a.name, f"select names an optional operand's presence, but "
                 f"{fam.name} has no Maybe<Src> operand")
    if a.observes:
        check_observer(c, a, fam)
    check_graph(c, a.name, a.graph, fam.params, fam, None)


def observer_of(arms: Iterable[Arm], family: str) -> Arm | None:
    """The family's observed wrapper (`observes`): the twin of each of its
    other arms runs it when the word is instrumented"""
    return next((a for a in arms if a.family == family and a.observes), None)


def check_observer(c: Checker, a: Arm, fam: Family) -> None:
    """An observed wrapper runs for the twins of a family that has events;
    nothing selects or installs it, and it adapts nothing.  A family has
    one."""
    others = [b.name for b in c.arms.values()
              if b.family == fam.name and b.observes and b.name != a.name]
    if others and others[0] < a.name:
        c.report(a.span, a.name, f"{fam.name} has two observed wrappers "
                 f"({others[0]}, {a.name})")
    if not fam.events:
        c.report(a.span, a.name, f"an observed wrapper needs its family's events: "
                 f"{fam.name} declares no events(...)")
    if a.select:
        c.report(a.span, a.name, "an observed wrapper is not selected statically")
    adapting = sorted(set(a.contract.exits) & {"miss", "again"})
    if adapting:
        c.report(a.contract.span, a.name, f"an observed wrapper adapts nothing "
                 f"({', '.join(adapting)})")


# Exits that leave the word's frame, which a completion stub's status
# (g1a/abi.md) cannot report
FRAME_EXITS = frozenset({"enter", "return", "yield", "exit"})
# A completion arm runs after its child finished, from inside a transfer: it
# neither leaves the frame nor restarts the word
COMPLETION_EXITS = frozenset({"next", "branch", "jump", "raise", "unwind", "redirect"})


def check_completion_arm(c: Checker, a: Arm) -> None:
    """`arm F.A completion(P)`: runs in the consumer's frame when a
    generator that F's word entered returns, with the child's value, from
    the homes of P, the resume point of that enter.  A family has at most
    one plain completion and one observed one, which runs under the
    instrumented twins."""
    assert a.completion is not None
    parts = a.name.split(".")
    fam = c.families.get(parts[0])
    if len(parts) != 2 or fam is None:
        c.report(a.span, a.name, f"arm {a.name} does not name a family's arm "
                 "(Family.Arm)")
        return
    edges = {n: () for n in a.contract.exits}
    check_contract(c, a.contract, a.name, fam.params, edges, True)
    for n in a.contract.exits:
        if n not in fam.exits:
            c.report(a.contract.span, a.name, f"exit {n} is not an exit of family "
                     f"{fam.name} ({', '.join(fam.exits)})")
    refused = sorted(set(a.contract.exits) - COMPLETION_EXITS)
    if refused:
        c.report(a.contract.span, a.name, f"a completion runs after its child "
                 f"returned; it cannot leave the frame or restart the word "
                 f"({', '.join(refused)})")
    if a.select or a.entry is not None:
        c.report(a.span, a.name, "a completion is not selected and holds no recipe")
    p = c.points.get(a.completion)
    if p is None or p.family != fam.name:
        c.report(a.span, a.name, f"completion({a.completion}): no point "
                 f"{a.completion} of {fam.name}")
    elif p.frame != FrameRole.caller or p.pc != PcRole.next_head or len(p.defs) > 1:
        c.report(a.span, a.name, f"completion({p.name}): the point is "
                 f"({p.frame.value}, {p.pc.value}) with {len(p.defs)} definitions; a "
                 "resume point is (caller, next_head) with at most one")
    same = [b for b in c.completions.values()
            if b.family == fam.name and b.observes == a.observes]
    if same[0] is not a:
        kind = "observed" if a.observes else "plain"
        c.report(a.span, a.name, f"{fam.name} has two {kind} completions "
                 f"({same[0].name}, {a.name})")
    if a.observes:
        if not fam.events:
            c.report(a.span, a.name, f"an observed completion needs its family's "
                     f"events: {fam.name} declares no events(...)")
        if not any(b.family == fam.name and not b.observes
                   for b in c.completions.values()):
            c.report(a.span, a.name, f"an observed completion needs a plain one: "
                     f"{fam.name} has none")
    check_graph(c, a.name, a.graph, fam.params, fam, None)


def completion_of(program: "Program", family: str, observes: bool = False) -> Arm | None:
    """The family's plain completion, or its observed one"""
    return next((a for a in program.completions.values()
                 if a.family == family and a.observes == observes), None)


def select_dimension(m: Select) -> str:
    return next(d for d, members in SELECT_DIMENSIONS.items() if m in members)


def check_selection(c: Checker, fam: Family) -> None:
    """Arms chosen by static evidence: every combination of the dimensions
    the family's arms name is admitted by exactly one arm.  A trailing arm
    is the assembler's, and no lowering selects it."""
    arms = [a for a in c.arms.values()
            if a.name.split(".")[0] == fam.name and not a.observes
            and not a.trailing]
    if not any(a.select for a in arms):
        return
    if "miss" in fam.exits:
        c.report(fam.span, fam.name, "a family whose arms are selected statically "
                 "has no miss")
    named = sorted({select_dimension(m) for a in arms for m in a.select})
    for combo in itertools.product(*(SELECT_DIMENSIONS[d] for d in named)):
        admitting = [a.name for a in arms
                     if all(m in combo for m in a.select)]
        if len(admitting) != 1:
            which = ", ".join(m.value for m in combo)
            c.report(fam.span, fam.name, f"selection ({which}) is admitted by "
                     f"{len(admitting)} arms" + (f" ({', '.join(admitting)})"
                                                  if admitting else "") +
                     "; exactly one admits it")


# Graphs

@dataclass
class Scope:
    """One graph's names and what the walk has found."""
    definition: str
    family: Family | None
    macro: Macro | None
    exits: frozenset[str]
    types: dict[str, Type]
    defs: dict[str, tuple[str, int]]      # name -> (block, statement index)
    blocks: dict[str, Block]
    used_exits: set[str] = field(default_factory=set)
    idom: dict[str, str | None] = field(default_factory=dict)
    callees: set[str] = field(default_factory=set)


OPERAND = -1        # the definition index of operands, environment and params


def successors(b: Block) -> list[tuple[str, bool]]:
    """(label, is_backedge) for each edge out of b."""
    t = b.term
    if isinstance(t, Goto):
        return [(t.target.label, False)]
    if isinstance(t, Branch):
        return [(t.if_true.label, False), (t.if_false.label, False)]
    if isinstance(t, CallTerm):
        return [(e.target.label, False) for e in t.edges]
    if isinstance(t, Each):
        return [(t.body, False), (t.exhaust.label, False)]
    if isinstance(t, Advance):
        return [(t.body, True)]
    return []


def check_graph(c: Checker, definition: str, graph: Graph, operands: tuple[Param, ...],
                family: Family | None, macro: Macro | None) -> None:
    if family is not None:
        arm = c.arm(definition)
        exits = frozenset(arm.contract.exits)
        declared = arm.contract.span
    else:
        assert macro is not None
        exits = frozenset(result_cases(macro.result))
        declared = macro.result.span
    blocks: dict[str, Block] = {}
    for b in graph.blocks:
        if b.label in blocks:
            c.report(b.span, definition, f"block {b.label} is defined twice; first at "
                     f"{blocks[b.label].span}")
        else:
            blocks[b.label] = b
    first = graph.blocks[0]
    if family is not None and c.arm(definition).completion is not None:
        # A completion starts with the child's final value
        if first.label != ENTRY or [p.type for p in first.params] != [O]:
            c.report(first.span, definition, "a completion's first block is "
                     "entry(v:O), the child's final value")
    elif first.label != ENTRY or first.params:
        c.report(first.span, definition, "the first block is entry()")
    # The instruction pointer before the word is the displacing arm's alone
    names = [n for n in ENVIRONMENT
             if n != "prev_head" or (family is not None and c.arm(definition).displacing)]
    types: dict[str, Type] = {n: ENVIRONMENT_TYPES[n] for n in names}
    defs: dict[str, tuple[str, int]] = {n: ("", OPERAND) for n in names}
    for p in operands + home_params(operands):
        types[p.name] = operand_value_type(p.type)
        defs[p.name] = ("", OPERAND)
    scope = Scope(definition, family, macro, exits, types, defs, blocks)
    order, preds = graph_shape(c, scope, graph)
    scope.idom = dominators(order, preds)
    for b in graph.blocks:
        define_block(c, scope, b)
    for label in order:
        check_block(c, scope, blocks[label])
    for n in sorted(exits - scope.used_exits):
        c.report(declared, definition, f"declared {'exit' if family else 'case'} {n} "
                 "is not reached")
    c.graphs[definition] = GraphInfo(definition, dict(types), tuple(order),
                                     {k: tuple(v) for k, v in preds.items()},
                                     dict(scope.idom), frozenset(scope.used_exits))
    if macro is not None:
        c.callees[definition] = frozenset(scope.callees)


ENVIRONMENT_TYPES = {n: HandleType(h) for n, h in ENVIRONMENT.items()}


def operand_value_type(t: Type) -> Type:
    """The type a graph reads an operand as: a target operand, near or
    far, names a Head; every other operand reads as declared."""
    if isinstance(t, HandleType) and t.handle in TARGET_BITS:
        return T(Handle.Head)
    return t


def graph_shape(c: Checker, scope: Scope, graph: Graph
                ) -> tuple[list[str], dict[str, list[str]]]:
    """Reverse postorder of the reachable blocks without backedges, and
    their predecessors.  Reports unknown labels, cycles other than
    each/advance and unreachable blocks."""
    blocks = scope.blocks
    bodies: dict[str, Each] = {}
    for b in graph.blocks:
        if isinstance(b.term, Each):
            if b.term.body in bodies:
                c.report(b.term.span, scope.definition, f"block {b.term.body} is the "
                         "body of two each loops")
            bodies[b.term.body] = b.term
    for b in graph.blocks:
        for label, back in successors(b):
            if label not in blocks:
                c.report(b.term.span, scope.definition, f"no block {label}")
            elif label in bodies and not back and not (
                    isinstance(b.term, Each) and b.term.body == label):
                c.report(b.term.span, scope.definition, f"block {label} is the body of "
                         f"the each at {bodies[label].span}; only each and advance "
                         "enter it")
        if isinstance(b.term, Advance) and b.term.body not in bodies:
            c.report(b.term.span, scope.definition, f"advance {b.term.body}: "
                     f"{b.term.body} is not the body of an each")
    order: list[str] = []
    preds: dict[str, list[str]] = {}
    state: dict[str, int] = {}          # 1 on the DFS stack, 2 done

    def visit(label: str) -> None:
        state[label] = 1
        for succ, back in successors(blocks[label]):
            if succ not in blocks or back:
                continue
            preds.setdefault(succ, []).append(label)
            if state.get(succ) == 1:
                c.report(blocks[label].term.span, scope.definition,
                         f"edge {label} -> {succ} closes a cycle; only each/advance "
                         "may loop")
            elif succ not in state:
                visit(succ)
        state[label] = 2
        order.append(label)

    first = graph.blocks[0].label
    preds[first] = []
    visit(first)
    order.reverse()
    for b in graph.blocks:
        if b.label not in state and blocks.get(b.label) is b:
            c.report(b.span, scope.definition, f"block {b.label} is unreachable")
    return order, preds


def dominators(order: list[str], preds: Mapping[str, list[str]]) -> dict[str, str | None]:
    """Immediate dominators of an acyclic graph given in reverse postorder."""
    index = {label: i for i, label in enumerate(order)}
    idom: dict[str, str | None] = {}
    for label in order:
        ps = [p for p in preds.get(label, []) if p in idom or p == label]
        if not ps:
            idom[label] = None
            continue
        common = ps[0]
        for p in ps[1:]:
            common = meet(idom, index, common, p)
        idom[label] = common
    return idom


def meet(idom: Mapping[str, str | None], index: Mapping[str, int], a: str, b: str
         ) -> str:
    """The nearest common dominator of a and b."""
    while a != b:
        while index[a] > index[b]:
            up = idom[a]
            assert up is not None
            a = up
        while index[b] > index[a]:
            up = idom[b]
            assert up is not None
            b = up
    return a


def dominates(idom: Mapping[str, str | None], a: str, b: str) -> bool:
    node: str | None = b
    while node is not None:
        if node == a:
            return True
        node = idom.get(node)
    return False


def define(c: Checker, scope: Scope, name: str, t: Type, block: str, index: int,
           span: SrcSpan) -> None:
    if name == DISCARD:
        return
    if name in scope.defs:
        where, _ = scope.defs[name]
        first = f"block {where}" if where else "the operands"
        c.report(span, scope.definition, f"{name} is assigned twice; first in {first}")
        return
    scope.defs[name] = (block, index)
    scope.types[name] = t


def define_block(c: Checker, scope: Scope, b: Block) -> None:
    for p in b.params:
        check_type(c, p.type, p.span, scope.definition)
        if field_kind(p.type) in (FieldKind.home, FieldKind.vector):
            # A home has one name, the operand's: facts about it and the
            # reentry that kills them follow that name only
            c.report(p.span, scope.definition, f"block parameter {p.name}: "
                     f"{type_text(p.type)} is a home; a block takes values")
        define(c, scope, p.name, p.type, b.label, OPERAND, p.span)
    for i, s in enumerate(b.stmts):
        if isinstance(s, Let):
            check_type(c, s.type, s.span, scope.definition)
            define(c, scope, s.name, s.type, b.label, i, s.span)


def check_block(c: Checker, scope: Scope, b: Block) -> None:
    for i, s in enumerate(b.stmts):
        if isinstance(s, Guard):
            if scope.family is None:
                c.report(s.span, scope.definition, "guard ... else miss in a macro, "
                         "whose exits are its cases")
            elif "miss" not in scope.exits:
                c.report(s.span, scope.definition, "guard ... else miss where miss is "
                         "not a declared exit")
            scope.used_exits.add("miss")
            expect(c, scope, s.cond, BOOL, b.label, i, outer=False)
        elif isinstance(s, Let):
            if s.name == DISCARD and not (isinstance(s.type, ScalarType)):
                c.report(s.span, scope.definition, f"_ discards only Unit or a scalar, "
                         f"not {type_text(s.type)}")
            expect(c, scope, s.value, s.type, b.label, i, outer=True)
        else:
            check_publish(c, scope, s, b.label, i)
    check_term(c, scope, b, len(b.stmts))


def check_publish(c: Checker, scope: Scope, s: Publish, block: str, i: int) -> None:
    if scope.macro is not None:
        # A point belongs to a family: arms publish, macros return cases
        c.report(s.span, scope.definition, "a macro does not publish; it returns a "
                 "case and its arm publishes")
        return
    seen: set[str] = set()
    vectors: set[str] = set()
    for bind in s.bindings:
        t = operand_type(scope, bind.target)
        if bind.target in seen:
            c.report(bind.span, scope.definition, f"publish binds {bind.target} twice")
        seen.add(bind.target)
        if bind.owners:
            check_owners_binding(c, scope, bind, t, block, i)
            continue
        if bind.vector:
            vectors.add(bind.target)
            check_vector_binding(c, scope, bind, t)
            continue
        if t is None or not is_dst(t):
            c.report(bind.span, scope.definition, f"publish [{bind.target} = ...]: "
                     f"{bind.target} is not a destination operand")
        if isinstance(bind.value, Null):
            continue
        # The publish reads every input before it writes a home, so an input
        # is a value already made: a let name, never an expression
        if not isinstance(bind.value, Name) or operand_type(scope, bind.value.name):
            c.report(bind.value.span, scope.definition,
                     f"publish [{bind.target} = {expr_text(bind.value)}]: a binding's "
                     "value is a let name or null")
            continue
        # A destination takes an owner, or an optional owner whose absence
        # leaves the home vacant
        have = type_of(c, scope, bind.value, O, block, i, False)
        if have is not None and have not in (O, WrapType(Wrap.Maybe, O)):
            c.report(bind.value.span, scope.definition,
                     f"{expr_text(bind.value)} is {type_text(have)}, "
                     f"expected {type_text(O)}")
    for d in s.debts:
        if d.kind == DebtKind.old and d.subject in vectors:
            c.report(d.span, scope.definition, f"old({d.subject}): a vector binding "
                     "writes vacant destinations; it displaces no owner")
        elif d.kind == DebtKind.old and is_span_of(operand_type(scope, d.subject), DST):
            pass
        elif d.kind == DebtKind.spent:
            t = operand_type(scope, d.subject)
            if not (t is not None and (is_source(t) or is_args(t) or is_span_of(t, SRC))):
                c.report(d.span, scope.definition, f"spent({d.subject}): {d.subject} "
                         "is not a source operand")
        elif d.kind == DebtKind.old:
            t = operand_type(scope, d.subject)
            if t is None or not is_dst(t):
                c.report(d.span, scope.definition, f"old({d.subject}): {d.subject} is "
                         "not a destination operand")
        else:
            want = T(Handle.Anchors) if d.kind == DebtKind.roots else O
            expect(c, scope, Name(d.subject, d.span), want, block, i, outer=False)
    check_point_ref(c, scope, s.point, s.span, block, i)


def is_dst(t: Type) -> bool:
    """A scalar destination home: Dst, Local or a group's projection, or
    their Maybe"""
    if isinstance(t, WrapType) and t.wrap == Wrap.Maybe:
        t = t.arg
    return t in (DST, T(Handle.Local), PART)


def is_span_of(t: Type | None, element: Type) -> bool:
    return t == WrapType(Wrap.Span, element)


def check_vector_binding(c: Checker, scope: Scope, bind: Binding, t: Type | None) -> None:
    """`d[] = s[]` or `d[] = dup(s[])`: span operands, element by element"""
    if not is_span_of(t, DST):
        c.report(bind.span, scope.definition, f"publish [{bind.target}[] = ...]: "
                 f"{bind.target} is not a Span<Dst> operand")
    value = bind.value
    if isinstance(value, Call):
        value = value.args[0]
    assert isinstance(value, Name)
    if not is_span_of(operand_type(scope, value.name), SRC):
        c.report(value.span, scope.definition, f"publish [{bind.target}[] = "
                 f"{value.name}[]]: {value.name} is not a Span<Src> operand")


def check_owners_binding(c: Checker, scope: Scope, bind: Binding, t: Type | None,
                         block: str, i: int) -> None:
    """`d[] = v`: v is a value the arm made, a Span<O> of count(d) owners"""
    if not is_span_of(t, DST):
        c.report(bind.span, scope.definition, f"publish [{bind.target}[] = ...]: "
                 f"{bind.target} is not a Span<Dst> operand")
    value = bind.value
    assert isinstance(value, Name)
    if operand_type(scope, value.name) is not None:
        c.report(value.span, scope.definition, f"publish [{bind.target}[] = "
                 f"{value.name}]: an operand span binds as {value.name}[]")
        return
    have = type_of(c, scope, value, None, block, i, False)
    if have is not None and not is_span_of(have, O):
        c.report(value.span, scope.definition, f"publish [{bind.target}[] = "
                 f"{value.name}]: {value.name} is {type_text(have)}, expected Span<O>")


def operand_type(scope: Scope, name: str) -> Type | None:
    where = scope.defs.get(name)
    if where is None or where[1] != OPERAND or where[0]:
        return None
    return scope.types.get(name)


def check_point_ref(c: Checker, scope: Scope, name: str, span: SrcSpan, block: str,
                    i: int) -> None:
    if scope.family is None:
        expect(c, scope, Name(name, span), T(Handle.Point), block, i, outer=False)
        return
    p = c.points.get(name)
    if p is None:
        c.report(span, scope.definition, f"no point {name}")
    elif p.family != scope.family.name:
        c.report(span, scope.definition, f"point {name} is not a point of "
                 f"{scope.family.name}")


def check_target(c: Checker, scope: Scope, t: Target, prefix: tuple[Type, ...],
                 block: str, i: int, what: str) -> None:
    target = scope.blocks.get(t.label)
    if target is None:
        return
    want = [p.type for p in target.params]
    if len(prefix) + len(t.args) != len(want):
        payload = f" (payload {len(prefix)} + {len(t.args)} given)" if prefix else ""
        c.report(t.span, scope.definition, f"{what} {t.label} takes {len(want)} "
                 f"arguments, not {len(prefix) + len(t.args)}{payload}")
        return
    for k, (have, need) in enumerate(zip(prefix, want)):
        if have != need:
            c.report(t.span, scope.definition, f"{what} {t.label}: payload value {k} "
                     f"is {type_text(have)}; parameter {target.params[k].name} is "
                     f"{type_text(need)}")
    for arg, need in zip(t.args, want[len(prefix):]):
        expect(c, scope, arg, need, block, i, outer=False)


def check_term(c: Checker, scope: Scope, b: Block, i: int) -> None:
    t = b.term
    if isinstance(t, Goto):
        check_target(c, scope, t.target, (), b.label, i, "goto")
    elif isinstance(t, Branch):
        expect(c, scope, t.cond, BOOL, b.label, i, outer=False)
        check_target(c, scope, t.if_true, (), b.label, i, "branch to")
        check_target(c, scope, t.if_false, (), b.label, i, "branch to")
    elif isinstance(t, CallTerm):
        check_status_call(c, scope, t, b.label, i)
    elif isinstance(t, Each):
        check_each(c, scope, t, b.label, i)
    elif isinstance(t, Advance):
        if t.body in scope.blocks and not dominates(scope.idom, t.body, b.label):
            c.report(t.span, scope.definition, f"advance {t.body} from block {b.label}, "
                     f"which is outside the body of {t.body}")
    else:
        check_exit(c, scope, t, b.label, i)


def check_status_call(c: Checker, scope: Scope, t: CallTerm, block: str, i: int) -> None:
    where = f"call {t.callee} at {t.span}"
    callee = c.externs.get(t.callee) or c.macros.get(t.callee)
    if callee is None:
        known = t.callee in PRIMITIVES or t.callee in SPECIAL_PRIMITIVES
        c.report(t.span, scope.definition, f"{where}: " + (
            f"{t.callee} is a language primitive with one result: use let" if known
            else f"no row or macro {t.callee}"))
        return
    if isinstance(callee, Macro):
        scope.callees.add(callee.name)
    if isinstance(callee.result, Returns):
        c.report(t.span, scope.definition, f"{where}: {t.callee} has one result: use "
                 "let")
        return
    binding = check_args(c, scope, t.callee, callee.params, t.args, t.span, block, i)
    cases = {case.name: case for case in callee.result.cases}
    seen: set[str] = set()
    for e in t.edges:
        case = cases.get(e.case)
        if case is None:
            c.report(e.span, scope.definition, f"{where}: {t.callee} has no case "
                     f"{e.case} (cases: {', '.join(cases)})")
            continue
        if e.case in seen:
            c.report(e.span, scope.definition, f"{where} has two edges for {e.case}")
        seen.add(e.case)
        payload = tuple(substitute(p.type, binding) for p in case.params)
        check_target(c, scope, e.target, payload, block, i, f"edge {e.case} ->")
        if isinstance(callee, Extern) and callee.c is not None and \
                CONVENTIONS.get(callee.c.convention, None) is not None and \
                CONVENTIONS[callee.c.convention].transfer:
            check_transfer_edge(c, scope, t, e, block)
    for name in cases:
        if name not in seen:
            c.report(t.span, scope.definition, f"{where} has no edge for {name}")


def check_transfer_edge(c: Checker, scope: Scope, t: CallTerm, e: Edge,
                        block: str) -> None:
    """The callee of a transfer row performs the exit its case names, so the
    edge's block is entered by that edge only and is exactly that exit, of
    the case's payload in order."""
    where = f"call {t.callee} at {t.span}: edge {e.case}"
    kind = TRANSFER_EXITS.get(e.case)
    target = scope.blocks.get(e.target.label)
    if kind is None or target is None:
        return
    if scope.family is None:
        c.report(e.span, scope.definition, f"{where}: only an arm calls a transfer row")
        return
    entering = [b.label for b in scope.blocks.values()
                for label, _ in successors(b) if label == target.label]
    names = [p.name for p in target.params]
    term = target.term
    args = list(term.args) if isinstance(term, Exit) else []
    if kind == ExitKind.raise_.value and args:
        args = args[1:]
    if (e.target.args or target.stmts or not isinstance(term, Exit)
            or term.kind != kind or len(entering) != 1
            or [a.name if isinstance(a, Name) else None for a in args] != names):
        c.report(e.span, scope.definition, f"{where}: the callee performs exit {kind}, "
                 f"so {target.label} is entered by this edge only and is exactly "
                 f"`exit {kind}(...)` of the case's payload")


def element_type(t: Type) -> Type | None:
    if isinstance(t, WrapType) and t.wrap == Wrap.Span:
        return t.arg
    if isinstance(t, SchemaType) and t.ctor == SchemaCtor.Rows:
        return SchemaType(SchemaCtor.Recipe, t.schema)
    return None


def check_each(c: Checker, scope: Scope, t: Each, block: str, i: int) -> None:
    over = type_of(c, scope, t.over, None, block, i, outer=False)
    element = None if over is None else element_type(over)
    if over is not None and element is None:
        c.report(t.span, scope.definition, f"each over {expr_text(t.over)}: "
                 f"{type_text(over)} is not a Span or Rows")
    if element is not None and field_kind(element) is FieldKind.home:
        if is_source(element):
            # The body gets each source's value, borrowed: a block takes
            # values, never homes
            element = B
        else:
            c.report(t.span, scope.definition, f"each over {expr_text(t.over)}: "
                     f"its items are {type_text(element)} homes; only publish "
                     "writes homes")
            element = None
    if element is not None and is_linear(element):
        # The body would take each item's obligation while the span keeps
        # them all; no declaration says which discharges it
        c.report(t.span, scope.definition, f"each over {expr_text(t.over)}: its items "
                 f"({type_text(element)}) carry obligations; each walks values only")
    if t.limit is not None and t.limit < 1:
        c.report(t.span, scope.definition, "each limit is at least 1")
    body = scope.blocks.get(t.body)
    if body is not None and element is not None:
        want = (element, SIZE)
        have = tuple(p.type for p in body.params)
        if have != want:
            c.report(body.span, scope.definition, f"each body {t.body} takes "
                     f"({type_text(element)}, Size), not "
                     f"({', '.join(type_text(x) for x in have)})")
    check_target(c, scope, t.exhaust, (), block, i, "exhaust")


def check_exit(c: Checker, scope: Scope, t: Exit, block: str, i: int) -> None:
    if t.kind not in scope.exits:
        what = "exit" if scope.family else "case"
        c.report(t.span, scope.definition, f"exit {t.kind}: {t.kind} is not a declared "
                 f"{what} of {scope.definition} ({', '.join(sorted(scope.exits))})")
        return
    scope.used_exits.add(t.kind)
    resumes = (ExitKind.enter.value, ExitKind.yield_.value)
    if (t.resume is not None) != (t.kind in resumes and scope.family is not None):
        c.report(t.span, scope.definition, "an arm's exit enter or yield names its "
                 "resume point (exit enter(c) resume P, exit yield(v) resume P), and "
                 "no other exit does")
    if t.complete is not None and t.kind != ExitKind.enter.value:
        c.report(t.span, scope.definition, "only an enter names a completion")
    if t.resume is not None and scope.family is not None:
        check_resume(c, scope, t)
    if t.complete is not None and scope.family is not None:
        check_complete(c, scope, t)
    if scope.family is None:
        assert scope.macro is not None
        want = tuple(p.type for p in result_cases(scope.macro.result)[t.kind])
    else:
        schemas = feedback_schemas(scope.family.params)
        family_ticket = SchemaType(SchemaCtor.Ticket, schemas[0]) if schemas else None
        want = tuple(family_ticket if w is None else w
                     for w in EXIT_PAYLOAD[EXIT_NAMES[t.kind]])
    if len(t.args) != len(want):
        c.report(t.span, scope.definition, f"exit {t.kind} takes {len(want)} "
                 f"arguments, not {len(t.args)}")
        return
    for k, (arg, need) in enumerate(zip(t.args, want)):
        if need == T(Handle.Point) and scope.family is not None:
            if isinstance(arg, Name):
                check_point_ref(c, scope, arg.name, arg.span, block, i)
                p = c.points.get(arg.name)
                if (t.kind == ExitKind.unwind.value and p is not None
                        and p.exc is not ExcRole.pending):
                    c.report(t.span, scope.definition, f"exit unwind({arg.name}, ...): "
                             f"the exception is pending as the frame unwinds, but "
                             f"{arg.name}'s exception role is {p.exc.value}")
            else:
                c.report(t.span, scope.definition, f"exit {t.kind}: argument {k} names "
                         "a point")
            continue
        if need is None:
            continue
        expect(c, scope, arg, need, block, i, outer=False)


def check_resume(c: Checker, scope: Scope, t: Exit) -> None:
    """The point where the caller resumes when the child returns: the child's
    value becomes its one definition."""
    assert t.resume is not None and scope.family is not None
    check_point_ref(c, scope, t.resume, t.span, "", 0)
    p = c.points.get(t.resume)
    if p is None or p.family != scope.family.name:
        return
    if t.kind == ExitKind.yield_.value:
        # The generator itself resumes there, the sent value its definition
        if p.frame != FrameRole.current or p.pc != PcRole.next_head or len(p.defs) != 1:
            c.report(t.span, scope.definition, f"yield resume {p.name}: the point is "
                     f"({p.frame.value}, {p.pc.value}) with {len(p.defs)} definitions; "
                     "a generator resumes at (current, next_head) with one, the sent "
                     "value")
        return
    if p.frame != FrameRole.caller or p.pc != PcRole.next_head:
        c.report(t.span, scope.definition, f"resume {p.name}: the point is "
                 f"({p.frame.value}, {p.pc.value}); a resume point is (caller, "
                 "next_head)")
    if len(p.defs) > 1:
        c.report(t.span, scope.definition, f"resume {p.name}: the child's value is "
                 f"one definition, not {len(p.defs)}")


def check_complete(c: Checker, scope: Scope, t: Exit) -> None:
    """`complete A`: A is the family's plain completion, and it completes
    the point the enter resumes at"""
    assert t.complete is not None and scope.family is not None
    a = c.completions.get(t.complete)
    if a is None or a.family != scope.family.name or a.observes:
        c.report(t.span, scope.definition, f"complete {t.complete}: not a plain "
                 f"completion arm of {scope.family.name}")
    elif a.completion != t.resume:
        c.report(t.span, scope.definition, f"complete {t.complete}: it completes "
                 f"{a.completion}, but the enter resumes at {t.resume}")


# Expressions

def is_optional_source(scope: Scope, e: Expr, want: Type, have: Type | None) -> bool:
    """An optional source operand (Maybe<Src>) stands for its source where
    the fact present(x) holds, which rop_verify checks"""
    return (want == SRC and have == WrapType(Wrap.Maybe, SRC) and isinstance(e, Name)
            and operand_type(scope, e.name) is not None)


def expect(c: Checker, scope: Scope, e: Expr, want: Type, block: str, i: int,
           outer: bool) -> None:
    have = type_of(c, scope, e, want, block, i, outer)
    if is_optional_source(scope, e, want, have):
        return
    if want == SRC and have == PART:
        # A projection is a source home the instruction also writes
        return
    if have is not None and have != want:
        c.report(e.span, scope.definition, f"{expr_text(e)} is {type_text(have)}, "
                 f"expected {type_text(want)}")


def resolve_name(c: Checker, scope: Scope, e: Name, block: str, i: int) -> Type | None:
    if e.name in scope.defs:
        where, at = scope.defs[e.name]
        if where:
            if where == block and at >= i and at != OPERAND:
                c.report(e.span, scope.definition, f"{e.name} is used before its let "
                         f"in block {block}")
            elif where != block and not dominates(scope.idom, where, block):
                c.report(e.span, scope.definition, f"{e.name} is used in block {block}, "
                         f"which block {where} (its definition) does not dominate")
        return scope.types[e.name]
    enum, _, member = e.name.rpartition(".")
    if enum and enum in c.enums and member in c.enums[enum].members:
        return EnumType(enum)
    owners = c.members.get(e.name, [])
    if len(owners) == 1:
        return EnumType(owners[0])
    if len(owners) > 1:
        c.report(e.span, scope.definition, f"{e.name} is a member of "
                 f"{', '.join(owners)}: qualify it")
        return None
    if e.name in c.points:
        return T(Handle.Point)
    c.report(e.span, scope.definition, f"unknown name {e.name}")
    return None


def type_of(c: Checker, scope: Scope, e: Expr, want: Type | None, block: str, i: int,
            outer: bool) -> Type | None:
    if isinstance(e, Name):
        return resolve_name(c, scope, e, block, i)
    if isinstance(e, BoolLit):
        return BOOL
    if isinstance(e, Null):
        c.report(e.span, scope.definition, "null is the vacancy a publish binding "
                 "writes; it is not a value")
        return None
    if isinstance(e, Nat):
        if not isinstance(want, ScalarType) or want.scalar in (Scalar.Unit, Scalar.Bool):
            c.report(e.span, scope.definition, f"number {e.value} needs a numeric "
                     "context")
            return None
        bits = want.scalar.bits
        if e.value >= 2 ** bits:
            c.report(e.span, scope.definition, f"{e.value} does not fit "
                     f"{type_text(want)}")
        return want
    if isinstance(e, SpanLit):
        element = want.arg if isinstance(want, WrapType) and want.wrap == Wrap.Span else None
        items = e.items
        if element is None and want is None and items:
            element = type_of(c, scope, items[0], None, block, i, outer=False)
            items = items[1:]
            if element is None:
                return None
        if element is None:
            c.report(e.span, scope.definition, "a [...] list needs a Span context")
            return None
        for item in items:
            expect(c, scope, item, element, block, i, outer=False)
        return WrapType(Wrap.Span, element)
    return type_of_call(c, scope, e, block, i, outer)


def type_of_call(c: Checker, scope: Scope, e: Call, block: str, i: int, outer: bool
                 ) -> Type | None:
    if e.callee == UNWRAP:
        if len(e.args) != 1:
            c.report(e.span, scope.definition, "unwrap takes one argument")
            return None
        t = type_of(c, scope, e.args[0], None, block, i, outer=False)
        if t is None:
            return None
        if not (isinstance(t, WrapType) and t.wrap == Wrap.Maybe):
            c.report(e.span, scope.definition, f"unwrap({expr_text(e.args[0])}): "
                     f"{type_text(t)} is not a Maybe")
            return None
        return t.arg
    if e.callee == CANDIDATE:
        return type_of_candidate(c, scope, e, block, i)
    if e.callee == COUNT:
        if len(e.args) != 1:
            c.report(e.span, scope.definition, "count takes one argument")
            return None
        t = type_of(c, scope, e.args[0], None, block, i, outer=False)
        if t is not None and not (isinstance(t, WrapType) and t.wrap == Wrap.Span):
            c.report(e.span, scope.definition, f"count({expr_text(e.args[0])}): "
                     f"{type_text(t)} is not a Span")
        return SIZE
    prim = PRIMITIVES.get(e.callee)
    if prim is not None:
        if prim.action and not outer:
            c.report(e.span, scope.definition, f"{e.callee}(...) is an action: only "
                     "the outermost expression of a let")
        if len(e.args) != len(prim.params):
            c.report(e.span, scope.definition, f"{e.callee} takes {len(prim.params)} "
                     f"arguments, not {len(e.args)}")
            return prim.result
        for arg, need in zip(e.args, prim.params):
            expect(c, scope, arg, need, block, i, outer=False)
        return prim.result
    callee = c.externs.get(e.callee) or c.macros.get(e.callee)
    if callee is None:
        c.report(e.span, scope.definition, f"no row, macro or primitive {e.callee}")
        return None
    if isinstance(callee, Macro):
        scope.callees.add(callee.name)
    if isinstance(callee.result, Cases):
        c.report(e.span, scope.definition, f"{e.callee} has cases "
                 f"({', '.join(k.name for k in callee.result.cases)}): use a call "
                 "terminator")
        return None
    if not outer and not callee.contract.effect_set <= READ_ONLY_EFFECTS:
        c.report(e.span, scope.definition, f"{e.callee}(...) is an action (effects "
                 f"{', '.join(sorted(x.value for x in callee.contract.effect_set))}): "
                 "only the outermost expression of a let")
    binding = check_args(c, scope, e.callee, callee.params, e.args, e.span, block, i)
    return substitute(callee.result.type, binding)


def check_args(c: Checker, scope: Scope, callee: str, params: tuple[Param, ...],
               args: tuple[Expr, ...], span: SrcSpan, block: str, i: int
               ) -> dict[str, Schema]:
    binding: dict[str, Schema] = {}
    if len(args) != len(params):
        c.report(span, scope.definition, f"{callee} takes {len(params)} arguments, "
                 f"not {len(args)}")
        return binding
    for arg, p in zip(args, params):
        want = substitute(p.type, binding)
        have = type_of(c, scope, arg, want, block, i, outer=False)
        if have is None:
            continue
        if is_optional_source(scope, arg, want, have):
            continue
        if not unify(want, have, binding):
            c.report(arg.span, scope.definition, f"{callee}: argument {p.name} is "
                     f"{type_text(want)}; {expr_text(arg)} is {type_text(have)}")
    return binding


def type_of_candidate(c: Checker, scope: Scope, e: Call, block: str, i: int
                      ) -> Type | None:
    if not e.args or not isinstance(e.args[0], Name):
        c.report(e.span, scope.definition, "candidate(Family.Arm[, recipe]) names an arm")
        return None
    arm = c.arms.get(e.args[0].name)
    if arm is None:
        c.report(e.args[0].span, scope.definition, f"candidate: no arm {e.args[0].name}")
        return None
    fam = c.families.get(arm.family)
    schemas = feedback_schemas(fam.params) if fam else []
    if not schemas:
        c.report(e.span, scope.definition, f"candidate: family {arm.family} has no "
                 "feedback schema")
        return None
    schema = schemas[0]
    if len(e.args) > 2:
        c.report(e.span, scope.definition, "candidate takes an arm and at most a recipe")
    elif len(e.args) == 2:
        expect(c, scope, e.args[1], SchemaType(SchemaCtor.Recipe, schema), block, i,
               outer=False)
    return SchemaType(SchemaCtor.Candidate, schema)


def check_macro_recursion(c: Checker) -> None:
    state: dict[str, int] = {}

    def visit(name: str, path: list[str]) -> None:
        state[name] = 1
        for callee in sorted(c.callees.get(name, ())):
            if state.get(callee) == 1:
                cycle = path[path.index(callee):] + [callee] if callee in path else [
                    name, callee]
                c.report(c.macros[name].span, name, "macro expansion recurses: "
                         + " -> ".join(cycle))
            elif callee not in state:
                visit(callee, path + [callee])
        state[name] = 2

    for name in sorted(c.macros):
        if name not in state:
            visit(name, [name])


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("files", nargs="*", type=Path)
    args = parser.parse_args(argv)
    files = args.files or sorted(ROP_DIR.glob("*.rop"))
    program = load(files)
    for d in program.diagnostics:
        print(d, file=sys.stderr)
    if not program.diagnostics:
        print(f"{len(program.families)} families, {len(program.arms)} arms, "
              f"{len(program.macros)} macros, {len(program.externs)} rows")
        print("pending: " + ", ".join(program.pending))
    return 1 if program.diagnostics else 0


if __name__ == "__main__":
    sys.exit(main())
