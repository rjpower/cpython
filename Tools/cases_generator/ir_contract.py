"""Declared contracts of the register IR's uops, and their verifier.

A uop declares its contract in Python/bytecodes.c: a mode and facts on each
stack item, and the clauses `effects(...)` and `exits(...)` between its stack
effect and its body (pyjit regions89, rd/design.md sections 1-3):

    leaf op(_GUARD_TOS_INT, (value -- value: inline_int))
        effects() exits(next, miss)
    { EXIT_IF(!PyValue_IsInlineInt(value)); }

The components that bodies call are rows of the definition list
Include/internal/pycore_ir_components.h, each with its own contract.  The
declarations are authoritative: the generator builds a declared uop's
record from them (effects, guard facts, result facts), and this module
checks the body against them.  It never authorizes anything by inference:
a body that may do more than its uop declares is refused.

During the pilot the uops without clauses keep the records that
ir_records.py infers; the migration (G0) seeds every uop and deletes that
inference.
"""

from dataclasses import dataclass, field
import re

from analyzer import Instruction, Uop, analysis_error
from generators_common import ROOT
from lexer import Token
from parser import BlockStmt, ForStmt, IfStmt, MacroIfStmt, SimpleStmt, Stmt, WhileStmt
import ir_records
from ir_records import (ALLOCATES, EFFECTS as RECORD_EFFECTS, MAY_RAISE, MAY_RELEASE,
                        MAY_RUN_PYTHON, READS_HEAP, WRITES_HEAP, Fact, _call, _close,
                        _split, _strip, _uncast, guard_condition)


# ---------------------------------------------------------------------------
# The vocabulary (design.md section 1).  Each set is closed.

EFFECTS = ("reads", "writes", "allocates", "raises", "releases", "runs_python",
           "polls", "switches_frame")
# Each declared effect in the record's terms; runs_python, polls and
# switches_frame may do anything
RECORD_EFFECT = {
    "reads": {READS_HEAP}, "writes": {WRITES_HEAP}, "allocates": {ALLOCATES},
    "raises": {MAY_RAISE}, "releases": {MAY_RELEASE},
    "runs_python": set(RECORD_EFFECTS), "polls": set(RECORD_EFFECTS),
    "switches_frame": set(RECORD_EFFECTS),
}
# The one record effect that stands for a site of each declared effect
# (ir_records.guard_order reads sites)
SITE_EFFECT = {
    "reads": READS_HEAP, "writes": WRITES_HEAP, "allocates": ALLOCATES,
    "raises": MAY_RAISE, "releases": MAY_RELEASE, "runs_python": MAY_RUN_PYTHON,
    "polls": MAY_RUN_PYTHON, "switches_frame": MAY_RUN_PYTHON,
}
# The effects that a declared effect covers
COVERS = {
    "runs_python": set(EFFECTS) - {"polls", "switches_frame"},
    "polls": set(EFFECTS) - {"switches_frame"},
    "switches_frame": set(EFFECTS) - {"polls"},
}
# The effects at whose sites heap facts die (1.1)
KILLS = frozenset(("writes", "releases", "runs_python", "polls", "switches_frame"))

EXITS = ("next", "branch", "jump", "miss", "deopt", "raise", "enter", "return",
         "yield", "exit")

# Feedback access (design 1.6, section 6): what a row or a uop reads and
# writes of its site's feedback, transitively.  `entry` is every field of
# the site's entry, its header's counter included; `handler` is the word's
# handler, which a specializer rewrites.  A uop's cache operands are reads
# of their fields and need no clause.
FEEDBACK_MODES = ("read", "write")
FEEDBACK_FIELDS = ("counter", "entry", "handler")
FEEDBACK_COVERS = {"entry": {"counter"}}

INPUT_MODES = ("borrowed", "consumed", "taken")
OUTPUT_MODES = ("owned", "inline", "borrowed", "moved")
ARG_MODES = ("borrowed", "consumed", "taken", "scalar", "object", "tstate", "frame",
             "out", "destructor")

# Facts: the vocabulary's words and the record's Fact kinds.  A fact with an
# argument is written `kind(arg)`.
FACT_KIND = {
    "nonnull": "NOT_NULL", "inline": "NUMBER", "inline_int": "INLINE_INT",
    "inline_float": "INLINE_FLOAT", "object": "OBJECT", "exact": "EXACT_TYPE",
    "type_version": "TYPE_VERSION", "has_values": "INLINE_VALUES_VALID",
    "shape": "SHAPE", "keys_version": "KEYS_VERSION", "length": "LENGTH",
}
VOCABULARY_OF_KIND = {v: k for k, v in FACT_KIND.items()}
FACTS_WITH_ARGUMENT = frozenset(("exact", "type_version", "shape", "keys_version", "length"))
# Properties of mutable heap state: they die at every KILLS site (1.1)
HEAP_FACTS = frozenset(("type_version", "has_values", "shape", "keys_version", "length"))
NUMBER_TYPES = ("PyLong_Type", "PyFloat_Type")


@dataclass(frozen=True, order=True)
class VFact:
    "A fact of the vocabulary: kind, and its argument or \"\""
    kind: str
    arg: str = ""

    def __str__(self) -> str:
        return f"{self.kind}({self.arg})" if self.arg else self.kind

    def record(self, subject: str) -> Fact:
        return Fact(FACT_KIND[self.kind], subject, self.arg)


NONNULL = VFact("nonnull")
OBJECT = VFact("object")
INLINE = VFact("inline")


def implied(facts) -> frozenset[VFact]:
    "facts and every fact that they imply (design.md 1.1)"
    result = set(facts)
    for f in list(result):
        if f.kind == "inline_int":
            result |= {INLINE, VFact("exact", "PyLong_Type")}
        elif f.kind == "inline_float":
            result |= {INLINE, VFact("exact", "PyFloat_Type")}
        elif f.kind == "exact" and f.arg not in NUMBER_TYPES:
            result.add(OBJECT)
        elif f.kind in ("type_version", "has_values", "keys_version", "length"):
            result.add(OBJECT)
        elif f.kind == "shape":
            result |= {VFact("has_values"), OBJECT}
    if result:
        result.add(NONNULL)
    return frozenset(result)


def without_heap_facts(facts: frozenset[VFact]) -> frozenset[VFact]:
    return frozenset(f for f in facts if f.kind not in HEAP_FACTS)


def parse_fact(word: str, where: Token | None) -> VFact:
    m = re.fullmatch(r"(\w+)(?:\((\w+)\))?", word)
    if m is None or m.group(1) not in FACT_KIND:
        message = (f"`{word}` is not a fact of the contract vocabulary "
                   f"({', '.join(sorted(FACT_KIND))})")
        raise analysis_error(message, where) if where else ValueError(message)
    kind, arg = m.group(1), m.group(2) or ""
    if (kind in FACTS_WITH_ARGUMENT) != bool(arg):
        message = (f"the fact `{kind}` "
                   f"{'needs an' if kind in FACTS_WITH_ARGUMENT else 'takes no'} argument")
        raise analysis_error(message, where) if where else ValueError(message)
    return VFact(kind, arg)


# ---------------------------------------------------------------------------
# The definition list (Include/internal/pycore_ir_components.h)

COMPONENTS_HEADER = ROOT / "Include/internal/pycore_ir_components.h"
ROW_KINDS = ("PRIMITIVE", "HELPER", "ROUTINE", "MACRO", "INTRINSIC", "DESTRUCTOR", "FIELD")
RESULT_SHAPES = ("NONE", "SCALAR", "VIEW", "PATH", "MEASURE", "TEST", "CONTEXT",
                 "VALUE", "VALUE_OR_NULL", "STATUS")
# The failure value of a STATUS result: the int compares to 0 so
STATUS_FAILURES = ("ZERO", "NEGATIVE", "NONZERO")
BUILDS = ("gil", "free_threaded", "all")
# The build whose handlers the generator verifies: the register IR's leaf
# forms and the JIT exist in the build with the GIL only
BUILD = "gil"


@dataclass(frozen=True)
class Arg:
    mode: str
    facts: frozenset[VFact] = frozenset()


@dataclass(frozen=True)
class Result:
    shape: str
    mode: str = ""                          # VALUE, VALUE_OR_NULL
    facts: frozenset[VFact] = frozenset()   # VALUE, VALUE_OR_NULL
    measure: str = ""                       # MEASURE; PATH: the path
    tests: tuple[tuple[frozenset[VFact], frozenset[VFact]], ...] = ()  # TEST
    reason: str = ""                        # CONTEXT
    null_iff: int = -1                      # VALUE: NULL exactly where argument k is
    failure: str = ""                       # STATUS: ZERO, NEGATIVE or NONZERO


@dataclass(frozen=True)
class Row:
    kind: str
    name: str
    ret: str = ""                   # C; "" for MACRO, INTRINSIC and DESTRUCTOR
    params: tuple[str, ...] = ()    # C parameter types; "..." last if variadic
    args: tuple[Arg, ...] = ()
    result: Result = Result("NONE")
    effects: frozenset[str] = frozenset()
    exits: frozenset[str] = frozenset()
    builds: frozenset[str] = frozenset()    # INTRINSIC
    callback_free: bool = False             # DESTRUCTOR
    feedback: frozenset[tuple[str, str]] = frozenset()     # (read|write, field)
    struct: str = ""                        # FIELD: the struct, the field,
    field: str = ""                         # and the path of the pointer
    on: str = ""                            # it is read through
    # CONJOINS(t): a two-argument TEST that holds exactly when the
    # one-argument TEST t holds of both arguments (guard fusion)
    conjoins: str = ""
    # NONNULL_FORM(f): f with the precondition that its one argument is
    # not NULL, which a caller may use after a non-NULL test
    nonnull_form: str = ""
    # SLOW(h): a VALUE_OR_NULL result is NULL only when the slow-path
    # helper h fails; the fast path cannot fail
    slow: str = ""

    def new_owner(self) -> bool:
        "whether a call returns a value that its caller must release"
        return self.result.shape in ("VALUE", "VALUE_OR_NULL") and self.result.mode == "owned"


def _split_text(text: str) -> list[str]:
    "text split at commas outside parentheses"
    parts, depth, start = [], 0, 0
    for i, c in enumerate(text):
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
        elif c == "," and depth == 0:
            parts.append(text[start:i])
            start = i + 1
    parts.append(text[start:])
    return parts


def _inner(text: str, head: str) -> str | None:
    text = text.strip()
    if text == head:
        return ""
    if text.startswith(head + "(") and text.endswith(")"):
        return text[len(head) + 1:-1]
    return None


def _parse_args(text: str | None) -> tuple[Arg, ...]:
    if text is None:
        raise ValueError("expected ARGS(...)")
    args = []
    for entry in [e.strip() for e in _split_text(text)]:
        if not entry:
            continue
        words = entry.split()
        if words[0] not in ARG_MODES:
            raise ValueError(f"argument mode {words[0]!r} is not one of {ARG_MODES}")
        args.append(Arg(words[0], frozenset(parse_fact(w, None) for w in words[1:])))
    return tuple(args)


def _parse_result(text: str) -> Result:
    text = text.strip()
    for shape in ("NONE", "SCALAR", "VIEW"):
        if text == shape:
            return Result(shape)
    if (inner := _inner(text, "PATH")) is not None:
        return Result("PATH", measure=inner.strip())
    if (inner := _inner(text, "STATUS")) is not None:
        if inner.strip() not in STATUS_FAILURES:
            raise ValueError(f"STATUS({' | '.join(STATUS_FAILURES)})")
        return Result("STATUS", failure=inner.strip())
    if (inner := _inner(text, "MEASURE")) is not None:
        return Result("MEASURE", measure=inner.strip())
    if (inner := _inner(text, "CONTEXT")) is not None:
        return Result("CONTEXT", reason=inner.strip().strip('"'))
    if (inner := _inner(text, "TEST")) is not None:
        tests = []
        for entry in _split_text(inner):
            true, _, false = entry.partition("|")
            tests.append((frozenset(parse_fact(w, None) for w in true.split()),
                          frozenset(parse_fact(w, None) for w in false.split())))
        return Result("TEST", tests=tuple(tests))
    for shape in ("VALUE_OR_NULL", "VALUE"):
        if (inner := _inner(text, shape)) is not None:
            words = inner.split()
            if not words or words[0] not in ("owned", "inline", "borrowed"):
                raise ValueError(f"{shape} needs a mode: {text}")
            null_iff, facts = -1, []
            for w in words[1:]:
                m = re.fullmatch(r"null_iff\((\d+)\)", w)
                if m and shape == "VALUE":
                    null_iff = int(m.group(1))
                else:
                    facts.append(parse_fact(w, None))
            return Result(shape, words[0], frozenset(facts), null_iff=null_iff)
    raise ValueError(f"{text!r} is not a result shape ({', '.join(RESULT_SHAPES)})")


def _parse_names(text: str, head: str, allowed: tuple[str, ...]) -> frozenset[str]:
    inner = _inner(text, head)
    if inner is None:
        raise ValueError(f"expected {head}(...), got {text.strip()!r}")
    names = frozenset(n.strip() for n in inner.replace("|", ",").split(",") if n.strip())
    unknown = names - set(allowed)
    if unknown:
        raise ValueError(f"{head}: {sorted(unknown)} not in {allowed}")
    return names


def parse_feedback(words, where: Token | None = None) -> frozenset[tuple[str, str]]:
    "`read(counter)`, `write(entry)`, ...: (mode, field) pairs"
    found = set()
    for w in words:
        m = re.fullmatch(r"(\w+)\((\w+)\)", w.strip())
        if m is None or m.group(1) not in FEEDBACK_MODES or m.group(2) not in FEEDBACK_FIELDS:
            message = (f"`{w.strip()}` is no feedback access: read(f) or write(f) with f "
                       f"one of {', '.join(FEEDBACK_FIELDS)}")
            raise analysis_error(message, where) if where else ValueError(message)
        found.add((m.group(1), m.group(2)))
    return frozenset(found)


def feedback_covered(access: tuple[str, str], declared) -> bool:
    mode, f = access
    return any(m == mode and (d == f or f in FEEDBACK_COVERS.get(d, ()))
               for m, d in declared)


# The optional columns after a row's fixed ones, by row kind
OPTIONAL_COLUMNS = {
    "PRIMITIVE": ("FEEDBACK", "CONJOINS", "NONNULL_FORM", "SLOW"),
    "HELPER": ("FEEDBACK", "SLOW"),
    "ROUTINE": ("FEEDBACK",),
    "MACRO": ("FEEDBACK",),
}


def _row_options(fields: list[str], fixed: int, kind: str) -> dict:
    """The optional columns after a row's fixed ones, each at most once:
    FEEDBACK(...) as (mode, field) pairs, the others as the row they name"""
    allowed = OPTIONAL_COLUMNS[kind]
    found: dict = {}
    for text in fields[fixed:]:
        head = next((h for h in allowed if _inner(text, h) is not None), None)
        if head is None or head in found:
            raise ValueError(f"{kind}: {fixed} columns, then optionally each of "
                             f"{', '.join(f'{h}(...)' for h in allowed)} once; "
                             f"got {text.strip()!r}")
        inner = _inner(text, head)
        if head == "FEEDBACK":
            found[head] = parse_feedback(_split_text(inner))
        elif not re.fullmatch(r"\w+", inner.strip()):
            raise ValueError(f"{head}(name) names one row")
        else:
            found[head] = inner.strip()
    return {"feedback": found.get("FEEDBACK", frozenset()),
            "conjoins": found.get("CONJOINS", ""),
            "nonnull_form": found.get("NONNULL_FORM", ""),
            "slow": found.get("SLOW", "")}


def _parse_row(kind: str, fields: list[str]) -> Row:
    if kind == "FIELD":
        if len(fields) != 6:
            raise ValueError("FIELD(struct, field, ctype, ON(path), result, EFFECTS(...))")
        struct, name, ctype, on, result, effects = fields
        if _inner(on, "ON") is None:
            raise ValueError("expected ON(path)")
        return Row(kind, f"{struct}.{name}", ctype, result=_parse_result(result),
                   effects=_parse_names(effects, "EFFECTS", EFFECTS),
                   struct=struct, field=name, on=_inner(on, "ON").strip())
    if kind == "DESTRUCTOR":
        if len(fields) != 2 or fields[1] not in ("callback_free",):
            raise ValueError("DESTRUCTOR(name, callback_free)")
        return Row(kind, fields[0], callback_free=True, effects=frozenset(("writes",)))
    if kind == "INTRINSIC":
        if len(fields) != 4:
            raise ValueError("INTRINSIC(name, ARGS(...), result, BUILD(...))")
        name, args, result, builds = fields
        return Row(kind, name, args=_parse_args(_inner(args, "ARGS")),
                   result=_parse_result(result), builds=_parse_names(builds, "BUILD", BUILDS))
    if kind == "MACRO":
        if len(fields) < 5:
            raise ValueError("MACRO(name, ARGS(...), result, EFFECTS(...), EXITS(...)"
                             "[, FEEDBACK(...)])")
        name, args, result, effects, exits = fields[:5]
        return Row(kind, name, args=_parse_args(_inner(args, "ARGS")),
                   result=_parse_result(result),
                   effects=_parse_names(effects, "EFFECTS", EFFECTS),
                   exits=_parse_names(exits, "EXITS", EXITS),
                   **_row_options(fields, 5, kind))
    fixed = 7 if kind == "ROUTINE" else 6
    if len(fields) < fixed:
        raise ValueError(f"{kind}(name, ret, (params), ARGS(...), result, EFFECTS(...)"
                         f"{', EXITS(...)' if kind == 'ROUTINE' else ''}"
                         f"[, {'][, '.join(f'{h}(...)' for h in OPTIONAL_COLUMNS[kind])}])")
    name, ret, params, args, result, effects = fields[:6]
    params = params.strip()
    if not (params.startswith("(") and params.endswith(")")):
        raise ValueError("the parameters are a parenthesized list of C types")
    row = Row(kind, name, ret,
              tuple(p.strip() for p in _split_text(params[1:-1]) if p.strip()),
              _parse_args(_inner(args, "ARGS")), _parse_result(result),
              _parse_names(effects, "EFFECTS", EFFECTS),
              exits=(_parse_names(fields[6], "EXITS", EXITS) if kind == "ROUTINE"
                     else frozenset()),
              **_row_options(fields, fixed, kind))
    fixed = [p for p in row.params if p != "..."]
    if len(row.args) != len(fixed):
        raise ValueError(f"{len(row.args)} ARGS for {len(fixed)} parameters")
    return row


def parse_rows(text: str, source: str = "pycore_ir_components.h") -> dict[str, Row]:
    """The rows of a definition list: every row invocation inside its
    #define lines (a list's own parameters, `(PRIMITIVE, HELPER, ...)`,
    are not rows)"""
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S).replace("\\\n", " ")
    rows: dict[str, Row] = {}
    for m in re.finditer(rf"\b({'|'.join(ROW_KINDS)})\(", text):
        start = m.end() - 1
        end, depth = start, 0
        for end in range(start, len(text)):
            depth += {"(": 1, ")": -1}.get(text[end], 0)
            if depth == 0:
                break
        fields = [f.strip() for f in _split_text(text[start + 1:end])]
        if all(f in ROW_KINDS for f in fields):
            continue
        try:
            row = _parse_row(m.group(1), fields)
        except ValueError as e:
            raise SyntaxError(f"{source}: row {fields[0]}: {e}") from None
        if row.name in rows:
            raise SyntaxError(f"{source}: row {row.name} twice")
        rows[row.name] = row
    return rows


def check_columns(table: dict[str, Row]) -> None:
    """The columns that name another row: the row exists and fits.  They
    are declarations; test_ir_contracts holds them to the C definitions on
    generated values"""
    def fail(row: Row, message: str):
        raise SyntaxError(f"pycore_ir_components.h: row {row.name}: {message}")

    def named(row: Row, column: str, name: str) -> Row:
        if name not in table:
            fail(row, f"{column}({name}) names no row")
        return table[name]

    for row in table.values():
        if row.conjoins:
            one = named(row, "CONJOINS", row.conjoins)
            if (row.result.shape != "TEST" or len(row.args) != 2
                    or one.result.shape != "TEST" or len(one.args) != 1):
                fail(row, f"CONJOINS({one.name}): a two-argument TEST of a "
                     "one-argument TEST")
            if any(a.mode != one.args[0].mode for a in row.args):
                fail(row, f"CONJOINS({one.name}): its arguments' modes differ from "
                     f"{one.name}'s")
            true = one.result.tests[0][0] if one.result.tests else frozenset()
            if any(t != (true, frozenset()) for t in row.result.tests) \
                    or len(row.result.tests) != 2:
                fail(row, f"CONJOINS({one.name}): its TEST is "
                     f"{', '.join(sorted(map(str, true))) or 'nothing'} of each argument "
                     "when true, and nothing of either when false")
        if row.nonnull_form:
            general = named(row, "NONNULL_FORM", row.nonnull_form)
            if len(row.args) != 1 or len(general.args) != 1 or row.params != general.params \
                    or row.ret != general.ret:
                fail(row, f"NONNULL_FORM({general.name}): one argument, and the C type "
                     f"of {general.name}")
            if (row.result.shape, row.result.mode) != (general.result.shape,
                                                       general.result.mode):
                fail(row, f"NONNULL_FORM({general.name}): its result differs from "
                     f"{general.name}'s")
            if NONNULL not in implied(row.args[0].facts) \
                    or NONNULL in implied(general.args[0].facts):
                fail(row, f"NONNULL_FORM({general.name}): it requires nonnull of its "
                     f"argument, which {general.name} does not")
            if not row.effects <= general.effects:
                fail(row, f"NONNULL_FORM({general.name}): effects beyond "
                     f"{general.name}'s")
        if row.slow:
            helper = named(row, "SLOW", row.slow)
            if row.result.shape != "VALUE_OR_NULL":
                fail(row, f"SLOW({helper.name}): a VALUE_OR_NULL result")
            if helper.kind != "HELPER" or helper.result.shape not in ("VALUE_OR_NULL",
                                                                       "STATUS"):
                fail(row, f"SLOW({helper.name}): a HELPER that can fail (VALUE_OR_NULL "
                     "or STATUS)")
            if not helper.effects <= row.effects:
                fail(row, f"SLOW({helper.name}): the row declares fewer effects than "
                     f"{helper.name}")


_ROWS: dict[str, Row] = {}


def rows() -> dict[str, Row]:
    if not _ROWS:
        _ROWS.update(parse_rows(COMPONENTS_HEADER.read_text()))
        check_columns(_ROWS)
    return _ROWS


def component_checks(table: dict[str, Row] | None = None) -> list[str]:
    """C static assertions that hold each row's C type to its definition:
    a HELPER's and a DESTRUCTOR's exact function type; a PRIMITIVE, which
    may be an expression macro, through a call with its declared parameter
    types.  Types only: a row's effects and modes are trusted (3.2)."""
    table = rows() if table is None else table
    lines = ["/* The rows of pycore_ir_components.h against their C definitions */"]

    def check(expr: str, ctype: str, name: str) -> None:
        lines.append(f"static_assert(__builtin_types_compatible_p(__typeof__({expr}), {ctype}),")
        lines.append(f"              \"{name}: its C type differs from pycore_ir_components.h\");")

    for row in table.values():
        if row.kind in ("HELPER", "ROUTINE"):
            check(f"&{row.name}", f"{row.ret} (*)({', '.join(row.params)})", row.name)
        elif row.kind == "DESTRUCTOR":
            check(f"&{row.name}", "void (*)(PyObject *)", row.name)
        elif row.kind == "FIELD":
            check(f"(({row.struct} *)0)->{row.field}", row.ret, row.name)
        elif row.kind == "PRIMITIVE":
            call = ", ".join(f"({p}){{0}}" for p in row.params)
            check(f"{row.name}({call})", row.ret, row.name)
    return lines


JIT_LOWER = ROOT / "Python" / "jit_lower.c"


def lowerings(source: str | None = None) -> frozenset[str]:
    """The primitives that Python/jit_lower.c lowers to machine code: its
    LOWERING(name) and CHECK_EXACT(name, type) definitions.  The set only
    refuses (V11); it grants nothing."""
    source = JIT_LOWER.read_text() if source is None else source
    return frozenset(re.findall(r"^(?:LOWERING|CHECK_EXACT)\((\w+)", source, re.M))


def check_primitives(models, lowered: frozenset[str],
                     table: dict[str, Row] | None = None) -> None:
    """V11: a PRIMITIVE row that has a model in ir_leaf.py has a machine
    lowering in jit_lower.c.  A row without a model is one that no leaf uop
    calls (the leaf translator refuses a call it has no model of); its uops
    run as their handler's function form."""
    table = rows() if table is None else table
    for row in table.values():
        if row.kind == "PRIMITIVE" and row.name in models and row.name not in lowered:
            raise ValueError(f"{row.name}: a PRIMITIVE with a model in ir_leaf.py "
                             "but no lowering in Python/jit_lower.c (V11)")


# ---------------------------------------------------------------------------
# A uop's declared contract

@dataclass(frozen=True)
class Item:
    "The declared contract of a stack item"
    name: str
    mode: str                   # "" for a pass-through
    source: str = ""            # moved(x): x
    facts: frozenset[VFact] = frozenset()
    nullable: bool = False
    array: bool = False
    size: str = ""              # an array's size, as declared


@dataclass(frozen=True)
class Contract:
    inputs: tuple[Item, ...]
    outputs: tuple[Item, ...]
    effects: frozenset[str]
    exits: frozenset[str]
    # The feedback the body reads and writes through its callees, beyond its
    # cache operands: (read|write, field)
    feedback: frozenset[tuple[str, str]] = frozenset()

    def input(self, name: str) -> Item | None:
        return next((i for i in self.inputs if i.name == name), None)

    def output(self, name: str) -> Item | None:
        return next((o for o in self.outputs if o.name == name), None)

    def record_effects(self) -> frozenset[str]:
        return frozenset().union(*(RECORD_EFFECT[e] for e in self.effects))


def _item(stack_item, modes: tuple[str, ...], where: Token) -> Item:
    name, through = stack_item.name, stack_item.peek
    mode, source, facts = "", "", []
    for w in stack_item.contract:
        m = re.fullmatch(r"moved\((\w+)\)", w)
        if m and "moved" in modes:
            mode, source = "moved", m.group(1)
        elif w in modes and mode and w in FACT_KIND:
            # `inline` is a mode and a fact: after the mode it is the fact
            facts.append(VFact(w))
        elif w in modes:
            if mode:
                raise analysis_error(f"`{name}` has two modes, {mode} and {w}", where)
            mode = w
        else:
            facts.append(parse_fact(w, where))
    if not mode and not through and name != "unused":
        raise analysis_error(f"`{name}` declares no mode ({', '.join(modes)})", where)
    if mode and through:
        raise analysis_error(f"`{name}` passes through, so it takes no mode", where)
    return Item(name, mode, source, frozenset(facts), stack_item.nullable,
                stack_item.is_array(), stack_item.size)


_CONTRACTS: dict[int, tuple[Uop, Contract | None]] = {}


def contract_of(uop: Uop) -> Contract | None:
    "uop's declared contract, or None if it declares none"
    if id(uop) in _CONTRACTS:
        return _CONTRACTS[id(uop)][1]
    contract = None
    if uop.clauses:
        where = uop.body.open
        for clause in ("effects", "exits"):
            if clause not in uop.clauses:
                raise analysis_error(f"{uop.name} declares a contract without {clause}()",
                                     where)
        effects = frozenset(uop.clauses["effects"])
        exits = frozenset(uop.clauses["exits"])
        for names, allowed, what in ((effects, EFFECTS, "effect"), (exits, EXITS, "exit")):
            for n in sorted(names - set(allowed)):
                raise analysis_error(
                    f"{uop.name}: `{n}` is not an {what} ({', '.join(allowed)})", where)
        inputs = tuple(_item(i, INPUT_MODES, where) for i in uop.stack.inputs)
        outputs = tuple(_item(o, OUTPUT_MODES, where) for o in uop.stack.outputs)
        for o in outputs:
            if o.mode == "moved" and not any(i.name == o.source for i in inputs):
                raise analysis_error(f"{uop.name}: `{o.name}` is moved from `{o.source}`, "
                                     "which is not an input", where)
        feedback = parse_feedback(uop.clauses.get("feedback", ()), where)
        contract = Contract(inputs, outputs, effects, exits, feedback)
    # The uop is kept with its contract, so that its id is not reused
    _CONTRACTS[id(uop)] = (uop, contract)
    return contract


# ---------------------------------------------------------------------------
# The body: its calls and the effects of its sites

# The DSL's own constructs, which are not components
DSL_CONSTRUCTS = frozenset((
    "EXIT_IF", "DEOPT_IF", "ERROR_IF", "ERROR_NO_POP", "DEAD", "INPUTS_DEAD",
    "DECREF_INPUTS", "STAT_INC", "OPCODE_DEFERRED_INC", "DISPATCH_SAME_OPARG", "assert",
    "holding",
))
CHECKED_ARITHMETIC = re.compile(r"__builtin_(add|sub|mul)_overflow")
GUARDS = {"EXIT_IF": "miss", "DEOPT_IF": "deopt"}
RAISES = ("ERROR_IF", "ERROR_NO_POP")


def _tokens(uop: Uop) -> list[Token]:
    return [t for t in uop.body.tokens() if t.kind != "COMMENT"]


def _calls_in(tokens: list[Token]) -> list[tuple[Token, list[list[str]]]]:
    """The calls among tokens, in order, with their arguments as token
    texts; the contents of assert() are not code"""
    texts = [t.text for t in tokens]
    calls = []
    i = 0
    while i < len(tokens):
        t = tokens[i]
        if t.kind == "IDENTIFIER" and i + 1 < len(tokens) and texts[i + 1] == "(":
            end = _close(texts, i + 1)
            if t.text == "assert":
                i = end + 1
                continue
            inner = texts[i + 2:end]
            calls.append((t, _split(inner, ",") if inner else []))
        i += 1
    return calls


def body_calls(uop: Uop) -> list[tuple[Token, list[list[str]]]]:
    return _calls_in(_tokens(uop))


def check_callees(uop: Uop, table: dict[str, Row]) -> None:
    "V1: every call names a row or a construct of the DSL"
    for tkn, _ in body_calls(uop):
        name = tkn.text
        row = table.get(name)
        if row is not None and row.kind == "INTRINSIC" and not row.builds & {BUILD, "all"}:
            # An intrinsic is the identity only in its builds; elsewhere it is
            # no component (design 2.2)
            raise analysis_error(f"{uop.name}: calls {name}, an INTRINSIC of the "
                                 f"{', '.join(sorted(row.builds))} build, not of the "
                                 f"{BUILD} build that the generator generates", tkn)
        if name in DSL_CONSTRUCTS or row is not None or CHECKED_ARITHMETIC.fullmatch(name):
            continue
        raise analysis_error(f"{uop.name}: calls {name}, which pycore_ir_components.h "
                             "does not declare", tkn)


@dataclass(frozen=True)
class Site:
    token: Token
    effect: str
    why: str


def _call_effects(row: Row, args: list[list[str]], table: dict[str, Row],
                  inline: set[str]) -> list[tuple[str, str]]:
    """The effects of a call of row: its declared ones; a close's are its
    destructor's: `writes` for a DESTRUCTOR row, `releases` for anything
    else, none for an inline value"""
    effects = [(e, f"{row.name} declares it") for e in sorted(row.effects)]
    for spec, arg in zip(row.args, args):
        if spec.mode != "destructor":
            continue
        name = " ".join(arg)
        value = next((" ".join(a) for s, a in zip(row.args, args) if s.mode == "consumed"), "")
        if value in inline:
            continue
        destructor = table.get(name)
        if destructor is not None and destructor.kind == "DESTRUCTOR":
            effects.append(("writes", f"{name} frees memory and releases nothing"))
        else:
            effects.append(("releases", f"{name} is no callback_free DESTRUCTOR row"))
    return effects


def effect_sites(uop: Uop, table: dict[str, Row] | None = None) -> list[Site]:
    """The sites of uop's body that have an effect, in order: calls of rows,
    the DSL's raises and releases, and the body's own heap accesses"""
    table = rows() if table is None else table
    contract = contract_of(uop)
    inline = {i.name for i in contract.inputs if INLINE in implied(i.facts)} \
        if contract else set()
    # A store into an output array publishes; it writes nothing else
    arrays = {o.name for o in contract.outputs if o.array} if contract else set()
    tokens = _tokens(uop)
    texts = [t.text for t in tokens]
    calls = {id(t): args for t, args in body_calls(uop)}
    sites: list[Site] = []
    i = 0
    while i < len(tokens):
        t, text = tokens[i], texts[i]
        if text == "assert" and i + 1 < len(tokens) and texts[i + 1] == "(":
            i = _close(texts, i + 1) + 1
            continue
        if (text in ir_records.ASSIGNS and ir_records._lvalue_is_heap(texts, i)
                and _element_base(texts, i) not in arrays):
            sites.append(Site(t, "writes", "a store through a pointer"))
        elif text == "->":
            sites.append(Site(t, "reads", "a load through a pointer"))
        if id(t) in calls:
            if text in table:
                for e, why in _call_effects(table[text], calls[id(t)], table, inline):
                    sites.append(Site(t, e, why))
            elif text in RAISES:
                sites.append(Site(t, "raises", text))
            elif text == "DECREF_INPUTS" and contract is not None:
                if any(i.mode in ("consumed", "taken") and i.name not in inline
                       for i in contract.inputs):
                    sites.append(Site(t, "releases", "DECREF_INPUTS of an owned input"))
        i += 1
    p = uop.properties
    if p.eval_breaker:
        sites.append(Site(uop.body.open, "polls", "the uop polls the eval breaker"))
    if p.switches_frame or p.returns:
        sites.append(Site(uop.body.open, "switches_frame", "the uop switches frames"))
    return sites


def _element_base(texts: list[str], i: int) -> str:
    "the array of an element store `a[k] = ...` at the `=` at i, or \"\""
    if i == 0 or texts[i - 1] != "]":
        return ""
    depth = 0
    for j in range(i - 1, -1, -1):
        depth += {"]": 1, "[": -1}.get(texts[j], 0)
        if depth == 0:
            return texts[j - 1] if j > 0 else ""
    return ""


def covered(effect: str, declared: frozenset[str]) -> bool:
    return effect in declared or any(effect in COVERS.get(d, ()) for d in declared)


def check_effects(uop: Uop, contract: Contract, table: dict[str, Row]) -> None:
    "V2: the body's effects lie within the declared ones"
    for site in effect_sites(uop, table):
        if not covered(site.effect, contract.effects):
            raise analysis_error(
                f"{uop.name}: `{site.token.text}` may {site.effect} ({site.why}), which "
                f"effects({', '.join(sorted(contract.effects))}) does not declare",
                site.token)
    if "pure" in uop.annotations:
        bad = contract.effects - {"reads", "allocates", "raises"}
        if bad:
            raise analysis_error(f"{uop.name} is annotated pure but declares "
                                 f"{', '.join(sorted(bad))}", uop.body.open)


def check_feedback(uop: Uop, contract: Contract, table: dict[str, Row]) -> None:
    """The feedback that the body's callees read and write lies within the
    uop's feedback(...) clause (staleness in G1 depends on it)"""
    for tkn, _ in body_calls(uop):
        row = table.get(tkn.text)
        if row is None:
            continue
        for access in sorted(row.feedback):
            if not feedback_covered(access, contract.feedback):
                declared = ", ".join(f"{m}({f})" for m, f in sorted(contract.feedback))
                raise analysis_error(
                    f"{uop.name}: `{tkn.text}` may {access[0]} the feedback {access[1]}, "
                    f"which feedback({declared}) does not declare", tkn)


def feedback_access(uop: Uop) -> frozenset[tuple[str, str]]:
    """What a declared uop reads and writes of its site's feedback: its
    clause, and a read of the field of each cache operand"""
    contract = contract_of(uop)
    caches = {("read", c.name) for c in uop.caches if c.name != "unused"}
    return frozenset(caches | (contract.feedback if contract else set()))


def arm_feedback(inst: Instruction) -> frozenset[tuple[str, str]] | None:
    """What an arm reads and writes of its site's feedback, through all of
    its uops; None if one of them declares no contract"""
    parts = [p for p in inst.parts if isinstance(p, Uop)]
    if any(contract_of(p) is None for p in parts):
        return None
    return frozenset().union(*(feedback_access(p) for p in parts))


def record_effect_sites(uop: Uop) -> list[tuple[Token, str]]:
    "effect_sites() in ir_records' terms, for its guard-order check"
    return [(s.token, SITE_EFFECT[s.effect]) for s in effect_sites(uop)]


# ---------------------------------------------------------------------------
# Guard facts (V9)

def _unwrap(tokens: list[str], table: dict[str, Row]) -> list[str]:
    "tokens without casts and the INTRINSIC identity forms around them"
    tokens = _strip(_uncast(tokens))
    while tokens and tokens[0] in table and table[tokens[0]].kind == "INTRINSIC":
        args = _call(tokens, tokens[0])
        if args is None or len(args) != 1:
            break
        tokens = _strip(_uncast(args[0]))
    return tokens


class Views:
    """What the names of a uop body stand for: an input is itself; a local
    assigned from a VIEW row of a name is a view of that name, and one
    assigned from a PATH row is a pointer along that path from it; any
    other local stands for itself"""

    def __init__(self, uop: Uop, table: dict[str, Row]):
        self.table = table
        self.names = {i.name: i.name for i in uop.stack.inputs if i.name != "unused"}
        self.paths: dict[str, tuple[str, str]] = {}
        self.locals: set[str] = set()
        texts = [t.text for t in _tokens(uop)]
        for i, t in enumerate(texts):
            if t != "=" or i == 0 or texts[i - 1] in self.names:
                continue
            self.locals.add(texts[i - 1])
            end = texts.index(";", i) if ";" in texts[i:] else len(texts)
            path = self.path(texts[i + 1:end])
            if path is not None:
                self.paths.setdefault(texts[i - 1], path)
                continue
            inner = self.resolve(texts[i + 1:end], locals_=False)
            if inner is not None:
                self.names.setdefault(texts[i - 1], inner)

    def resolve(self, tokens: list[str], locals_: bool = True) -> str | None:
        "the input (or, if locals_, the local) that tokens stand for"
        tokens = _unwrap(tokens, self.table)
        if len(tokens) == 1:
            if tokens[0] in self.names:
                return self.names[tokens[0]]
            return tokens[0] if locals_ and tokens[0] in self.locals else None
        call = _call_of(tokens, self.table)
        if call is not None and call[0].result.shape == "VIEW" and len(call[1]) == 1:
            return self.resolve(call[1][0], locals_)
        return None

    def path(self, tokens: list[str]) -> tuple[str, str] | None:
        "the (input, path) of a pointer along a PATH row from an input"
        tokens = _unwrap(tokens, self.table)
        if len(tokens) == 1:
            return self.paths.get(tokens[0])
        call = _call_of(tokens, self.table)
        if call is not None and call[0].result.shape == "PATH" and call[1]:
            name = self.resolve(call[1][0], locals_=False)
            if name is not None:
                return name, call[0].result.measure
        return None

    def field(self, tokens: list[str]) -> tuple[Row, str] | None:
        """the FIELD row of a read `p->field` through a pointer p along a
        path from an input, and that input"""
        tokens = _unwrap(tokens, self.table)
        if len(tokens) < 3 or tokens[-2] != "->":
            return None
        path = self.path(tokens[:-2])
        if path is None:
            return None
        row = next((r for r in self.table.values() if r.kind == "FIELD"
                    and r.field == tokens[-1] and r.on == path[1]), None)
        return (row, path[0]) if row is not None else None


def _call_of(tokens: list[str], table: dict[str, Row]) -> tuple[Row, list[list[str]]] | None:
    tokens = _strip(_uncast(tokens))
    if tokens and tokens[0] in table:
        args = _call(tokens, tokens[0])
        if args is not None:
            return table[tokens[0]], args
    return None


Facts = dict[str, frozenset[VFact]]


def _union(found: list[Facts]) -> Facts:
    result: dict[str, frozenset[VFact]] = {}
    for facts in found:
        for name, f in facts.items():
            result[name] = implied(result.get(name, frozenset()) | f)
    return result


def _intersection(found: list[Facts]) -> Facts:
    names = set.intersection(*(set(f) for f in found)) if found else set()
    return {n: frozenset.intersection(*(implied(f[n]) for f in found)) for n in names}


def condition_facts(cond: list[str], views: Views, table: dict[str, Row],
                    holds: bool) -> tuple[Facts, str] | None:
    """The facts that hold of the names where cond has the value holds, and
    a reason if a CONTEXT row decides it; None if cond is no combination
    of TEST rows, MEASURE comparisons and CONTEXT rows.  Joins are
    explicit: a disjunction that holds, or a conjunction that fails, gives
    only the facts that all its alternatives share."""
    cond = _strip(cond)
    for op, all_hold in (("||", False), ("&&", True)):
        parts = _split(cond, op)
        if len(parts) > 1:
            found = [condition_facts(p, views, table, holds) for p in parts]
            if any(f is None for f in found):
                return None
            reason = next((f[1] for f in found if f[1]), "")
            each = [f[0] for f in found]
            # `a || b` false and `a && b` true: every part has the value
            return (_union(each) if holds == all_hold else _intersection(each)), reason
    if cond[:1] == ["!"]:
        return condition_facts(cond[1:], views, table, not holds)
    cond = _unwrap(cond, table)
    field = views.field(cond)
    if field is not None and field[0].result.shape == "TEST":
        true, false = field[0].result.tests[0]
        found = true if holds else false
        return ({field[1]: implied(found)} if found else {}), ""
    call = _call_of(cond, table)
    if call is not None and call[0].result.shape == "TEST":
        row, args = call
        facts: dict[str, frozenset[VFact]] = {}
        for arg, (true, false) in zip(args, row.result.tests):
            found = true if holds else false
            if not found:
                continue
            name = views.resolve(arg)
            if name is None:
                return None
            facts[name] = implied(facts.get(name, frozenset()) | found)
        return facts, ""
    if call is not None and call[0].result.shape == "CONTEXT":
        return {}, call[0].result.reason
    for op in ("!=", "=="):
        sides = _split(cond, op)
        if len(sides) == 2:
            break
    else:
        return None
    field = views.field(sides[0])
    call = _call_of(_unwrap(sides[0], table), table)
    if field is not None and field[0].result.shape == "MEASURE":
        measure, name = field[0].result.measure, field[1]
    elif call is not None and call[0].result.shape == "MEASURE":
        measure, name = call[0].result.measure, views.resolve(call[1][0])
    else:
        return None
    arg = " ".join(_strip(sides[1]))
    if name is None or not re.fullmatch(r"\w+", arg):
        return None
    if (op == "==") != holds:
        return {}, ""
    return {name: implied({VFact(measure, arg)})}, ""


def site_facts(uop: Uop, site: Token, table: dict[str, Row] | None = None
               ) -> tuple[Facts, str]:
    """V9: the facts that a guard site establishes of the names of the
    uop's body, from the rows its condition calls, and its reason"""
    table = rows() if table is None else table
    cond, reason = guard_condition(uop, site)
    found = condition_facts(cond, Views(uop, table), table, holds=False)
    if found is None:
        if reason:
            return {}, reason
        raise analysis_error(
            f"{uop.name}: the guard `{' '.join(cond)}` is no combination of TEST rows, "
            "MEASURE comparisons and CONTEXT rows; give its reason at the site, "
            "EXIT_IF(cond, \"reason\")", site)
    facts, context = found
    return facts, reason or context


def check_guard_conditions(uop: Uop, table: dict[str, Row]) -> None:
    "V3: a guard's condition calls only rows whose effects lie within `reads`"
    tokens = _tokens(uop)
    texts = [t.text for t in tokens]
    for i, t in enumerate(tokens):
        if t.text not in GUARDS or i + 1 >= len(tokens) or texts[i + 1] != "(":
            continue
        for tkn, _ in _calls_in(tokens[i + 2:_close(texts, i + 1)]):
            row = table.get(tkn.text)
            if row is not None and row.effects - {"reads"}:
                raise analysis_error(
                    f"{uop.name}: the guard calls {tkn.text}, which may "
                    f"{', '.join(sorted(row.effects - {'reads'}))}; a guard's condition "
                    "may only read", tkn)


# ---------------------------------------------------------------------------
# Ownership, facts and restartability on each exit (V4, V5, V6 inside a uop)

HELD, GONE, NULL = "held", "gone", "null"
# An owned output array: not yet written, wholly written, written in part
# (element stores), or written if the status local named after PENDING
# says success
UNWRITTEN, WRITTEN, PARTIAL, PENDING = "unwritten", "written", "partly written", "pending:"
# An owner that a row takes (ARGS `taken`): gone if the row succeeds, still
# the caller's if it fails.  TAKEN alone until the row's result is bound to
# a local, then TAKEN + the local, until a test of the local decides it
TAKEN = "taken:"


def obligated(status: str | None) -> bool:
    "whether an owner's status still owes a release: held, or held if a row fails"
    return status is not None and (status == HELD or status.startswith(TAKEN))


# Statement heads whose control flow the walk does not follow
UNMODELLED_HEADS = ("return", "goto", "break", "continue", "switch", "do")
# The effects whose sites may run Python.  Python may write the frame's
# homes (a finalizer through FrameLocalsProxy), so a value that stays in a
# home, a pass-through or a borrowed input, may be replaced there: its
# facts no longer describe the home (design 1.3, 3.2)
REENTRY = frozenset(("releases", "runs_python", "polls", "switches_frame"))


@dataclass
class _State:
    # An input or a local that holds an obligation: held, gone or null
    owners: dict[str, str]
    # The live facts of each name
    facts: dict[str, frozenset[VFact]]
    # Facts that hold of a name once it is shown not NULL: a VALUE_OR_NULL
    # result's declared facts
    pending: dict[str, frozenset[VFact]] = field(default_factory=dict)
    # The row or input that a local was last assigned from, for messages
    sources: dict[str, str] = field(default_factory=dict)
    # Inputs left in homes that a reentry site may have rewritten: the site
    rehomed: dict[str, str] = field(default_factory=dict)
    # The owned output arrays and how much of each is written
    arrays: dict[str, str] = field(default_factory=dict)
    # Locals holding a STATUS result: its failure value
    status: dict[str, str] = field(default_factory=dict)
    restartable: bool = True
    why: str = ""                   # what ended restartability

    def copy(self) -> "_State":
        return _State(dict(self.owners), dict(self.facts), dict(self.pending),
                      dict(self.sources), dict(self.rehomed), dict(self.arrays),
                      dict(self.status), self.restartable, self.why)

    def end_restart(self, why: str) -> None:
        if self.restartable:
            self.restartable, self.why = False, why

    def kill_heap_facts(self) -> None:
        self.facts = {n: without_heap_facts(f) for n, f in self.facts.items()}
        self.pending = {n: without_heap_facts(f) for n, f in self.pending.items()}

    def add_facts(self, name: str, facts) -> None:
        found = implied(self.facts.get(name, frozenset()) | facts)
        if NONNULL in found and name in self.pending:
            found = implied(found | self.pending.pop(name))
        self.facts[name] = found

    def assign(self, name: str, facts, pending, source: str) -> None:
        "name holds a new value: facts now, pending once it is not NULL"
        self.facts[name] = implied(facts)
        self.pending.pop(name, None)
        if pending:
            self.pending[name] = implied(pending)
        self.sources[name] = source


class OwnershipWalk:
    """Walks a declared uop's statements, tracking each obligation (the
    inputs, and every local that a new owner or a move puts one in), the
    live facts of the names, and whether the word can still restart; checks
    every exit site.  After run(), `exit` is the state at the `next` exit
    (the join of the paths that reach it), from which the uop's exported
    facts come (V9).  With kills false the walk ignores the effects' kills
    and follows control flow only: the facts a guard site gives on every
    path from it to the exit (the record's guard facts)."""

    def __init__(self, uop: Uop, contract: Contract, table: dict[str, Row],
                 kills: bool = True):
        self.uop, self.contract, self.table = uop, contract, table
        self.kills = kills
        self.views = Views(uop, table)
        self.declared = {i.name: i for i in contract.inputs if i.name != "unused"}
        self.new: set[str] = set()
        self.exit: _State | None = None
        self.site_effects: dict[int, list[str]] = {}
        for site in effect_sites(uop, table):
            self.site_effects.setdefault(id(site.token), []).append(site.effect)

    def run(self) -> "OwnershipWalk":
        facts = {n: implied(i.facts | (set() if i.nullable else {NONNULL}))
                 for n, i in self.declared.items()}
        state = _State({n: HELD for n in self.declared}, facts)
        state.arrays = {o.name: UNWRITTEN for o in self.contract.outputs
                        if o.array and o.mode == "owned"}
        end = self.block(self.uop.body.body, state)
        if end is not None and not self.uop.properties.always_exits:
            self.check_exit("next", end, self.uop.body.close)
            self.exit = end
        return self

    def exported(self, name: str) -> frozenset[VFact]:
        """The facts of name that hold at the `next` exit: none for an input
        left in a home that a reentry site may have rewritten"""
        if self.exit is None or name in self.exit.rehomed:
            return frozenset()
        return self.exit.facts.get(name, frozenset())

    # -- exits

    def fail(self, where: Token, message: str):
        raise analysis_error(f"{self.uop.name}: {message}", where)

    def check_exit(self, exit: str, state: _State, where: Token,
                   holding: frozenset[str] = frozenset()) -> None:
        if exit in ("miss", "deopt"):
            if not state.restartable:
                self.fail(where, f"the {exit} follows {state.why}, so the word cannot "
                          "restart (design.md 3.1)")
        for name, status in state.owners.items():
            if status.startswith(TAKEN):
                local = status[len(TAKEN):]
                self.fail(where, f"on {exit}, `{name}` is taken by a row only if "
                          + (f"`{local}` reports success, which nothing tests here"
                             if local else "the row succeeds, and its result is not kept"))
            item = self.declared.get(name)
            if item is None:
                # A new owner: published on next if it is an output; released,
                # or NULL, on every other exit
                if status == HELD and not (exit == "next" and self.contract.output(name)):
                    self.fail(where, f"on {exit}, the new owner `{name}` is still held")
                continue
            gone = status != HELD
            mode = item.mode or "a pass-through"
            if exit in ("miss", "deopt"):
                if gone:
                    self.fail(where, f"on {exit}, `{name}` is released or moved, but a "
                              "guard exit must leave it untouched")
            elif item.mode in ("borrowed", "") and gone:
                self.fail(where, f"on {exit}, `{name}` is released or moved, but it is "
                          f"declared {mode}")
            elif exit == "next" and item.mode in ("consumed", "taken") and not gone:
                self.fail(where, f"on next, `{name}` is still owned, but it is declared "
                          f"{item.mode}")
            elif exit == "raise" and item.mode == "consumed":
                if gone == (name in holding):
                    self.fail(where, f"on raise, `{name}` is "
                              f"{'released' if gone else 'still owned'}, but the site "
                              f"declares {'holding' if name in holding else 'consumed'}")
            elif exit == "raise" and item.mode == "taken" and gone:
                self.fail(where, f"on raise, `{name}` is released, but it is declared "
                          "taken (kept on raise)")
        for name, written in state.arrays.items():
            if exit == "next" and written != WRITTEN:
                self.fail(where, f"on next, the output array `{name}` is "
                          f"{self.array_state(written)}, but it is published whole")
            if exit != "next" and written != UNWRITTEN:
                self.fail(where, f"on {exit}, the output array `{name}` is "
                          f"{self.array_state(written)}: its values would leak or be "
                          "published by a word that did not complete")
        if exit == "next":
            self.check_outputs(state, where)

    @staticmethod
    def array_state(written: str) -> str:
        if written.startswith(PENDING):
            return (f"written only if `{written[len(PENDING):]}` reports success, "
                    "which nothing tests here")
        return written

    def check_outputs(self, state: _State, where: Token) -> None:
        """V5 on the paths to next: each value output is not NULL unless it
        is nullable, and has its declared facts, whatever path reached it"""
        for out in self.contract.outputs:
            if out.array or out.mode not in ("owned", "inline", "moved"):
                continue
            live = state.facts.get(out.name, frozenset())
            source = state.sources.get(out.name)
            if not out.nullable and NONNULL not in live:
                if source is not None:
                    self.fail(where, f"`{out.name}` may be NULL from {source}, and nothing "
                              "raises on it on every path to next")
                self.fail(where, f"on next, `{out.name}` is not assigned on every path")
            missing = implied(out.facts) - live
            if missing:
                self.fail(where, f"on next, `{out.name}` is declared "
                          f"{', '.join(sorted(map(str, missing)))}, which does not hold on "
                          "every path (a join, or a kill since the assignment)")

    # -- statements

    def block(self, stmts: list[Stmt], state: _State) -> _State | None:
        "the state after stmts, or None if they always leave"
        for stmt in stmts:
            state = self.stmt(stmt, state)
            if state is None:
                return None
        return state

    def stmt(self, stmt: Stmt, state: _State) -> _State | None:
        if isinstance(stmt, BlockStmt):
            return self.block(stmt.body, state)
        if isinstance(stmt, MacroIfStmt):
            return self.join([self.block(stmt.body, state.copy()),
                              self.block(stmt.else_body or [], state.copy())])
        if isinstance(stmt, IfStmt):
            toks = [t for t in stmt.condition if t.kind != "COMMENT"]
            self.calls(toks, state)
            self.effects(toks, state)
            cond = [t.text for t in toks]
            then, other = state.copy(), state.copy()
            for st, holds in ((then, True), (other, False)):
                self.refine(st, cond, holds)
            ends = [self.stmt(stmt.body, then)]
            ends.append(self.stmt(stmt.else_body, other) if stmt.else_body else other)
            return self.join(ends)
        if isinstance(stmt, (ForStmt, WhileStmt)):
            # The body may run any number of times: walk it once and join
            return self.join([self.stmt(stmt.body, state.copy()), state])
        assert isinstance(stmt, SimpleStmt)
        return self.simple(stmt.contents, state)

    def join(self, ends: list[_State | None]) -> _State | None:
        """where paths meet: the same obligations; the facts that all of them
        have; the homes that any of them may have rewritten"""
        live = [e for e in ends if e is not None]
        if not live:
            return None
        first = live[0].copy()
        for other in live[1:]:
            for name in set(first.owners) | set(other.owners):
                a, b = first.owners.get(name, GONE), other.owners.get(name, GONE)
                if obligated(a) != obligated(b) or (
                        a != b and (a.startswith(TAKEN) or b.startswith(TAKEN))):
                    self.fail(self.uop.body.open, f"`{name}` is owned on one path and "
                              "not on another where they join")
                first.owners[name] = a
            # What holds of a name once it is not NULL, on each path: its
            # pending facts, or the facts it already has
            pending = {}
            for name in set(first.pending) | set(other.pending):
                a, b = (implied(st.facts.get(name, frozenset()) | st.pending.get(name, frozenset()))
                        for st in (first, other))
                if a & b:
                    pending[name] = a & b
            first.facts = _intersection([first.facts, other.facts])
            first.pending = pending
            first.sources.update(other.sources)
            for name, why in other.rehomed.items():
                first.rehomed.setdefault(name, why)
            for name in first.arrays:
                if first.arrays[name] != other.arrays.get(name):
                    self.fail(self.uop.body.open, f"the output array `{name}` is "
                              f"{self.array_state(first.arrays[name])} on one path and "
                              f"{self.array_state(other.arrays.get(name, UNWRITTEN))} on "
                              "another where they join")
            first.status.update(other.status)
            if not other.restartable:
                first.end_restart(other.why)
        return first

    def refine(self, state: _State, cond: list[str], holds: bool) -> None:
        self.refine_status(state, cond, holds)
        found = condition_facts(cond, self.views, self.table, holds)
        if found:
            for name, facts in found[0].items():
                state.add_facts(name, facts)
        # `PyValue_IsNull(x)`: a new owner that is NULL owes nothing; the
        # owners that the row which made x takes are the caller's again
        cond = _strip(cond)
        while cond[:1] == ["!"]:
            cond, holds = _strip(cond[1:]), not holds
        null = _call(cond, "PyValue_IsNull")
        if null is not None and len(null) == 1:
            name = " ".join(null[0])
            for owner, status in state.owners.items():
                if status == TAKEN + name:
                    state.owners[owner] = HELD if holds else GONE
            if state.owners.get(name) == HELD and name in self.new:
                if holds:
                    state.owners[name] = NULL
                else:
                    state.add_facts(name, {NONNULL})

    # A STATUS result's test: the predicate that a condition states of it
    STATUS_TESTS = {("==", "0"): "zero", ("!=", "0"): "nonzero", ("<", "0"): "negative",
                    (">=", "0"): "nonnegative"}
    FAILS = {"ZERO": ("zero", "nonzero"), "NONZERO": ("nonzero", "zero"),
             "NEGATIVE": ("negative", "nonnegative")}

    def refine_status(self, state: _State, cond: list[str], holds: bool) -> None:
        """`res == 0`, `!res`, `res`, `res < 0`...: on the path where the
        status local reports failure, the arrays pending on it are
        unwritten; where it reports success, written"""
        cond = _strip(cond)
        if cond[:1] == ["!"]:
            cond, holds = _strip(cond[1:]), not holds
        if not cond or cond[0] not in state.status:
            return
        name = cond[0]
        predicate = ("nonzero" if len(cond) == 1
                     else self.STATUS_TESTS.get(tuple(cond[1:])) if len(cond) == 3 else None)
        failure, success = self.FAILS[state.status[name]]
        if predicate not in (failure, success):
            return
        failed = (predicate == failure) == holds
        for array, written in state.arrays.items():
            if written == f"{PENDING}{name}":
                state.arrays[array] = UNWRITTEN if failed else WRITTEN
        for owner, status in state.owners.items():
            if status == TAKEN + name:
                state.owners[owner] = HELD if failed else GONE

    def effects(self, tokens: list[Token], state: _State) -> None:
        """the effects of the sites among tokens: heap facts die, homes may be
        rewritten, restart ends"""
        for t in tokens:
            for effect in self.site_effects.get(id(t), ()):
                if effect in KILLS and self.kills:
                    state.kill_heap_facts()
                if effect in REENTRY and self.kills:
                    for name, item in self.declared.items():
                        if item.mode in ("", "borrowed"):
                            state.rehomed.setdefault(
                                name, f"`{t.text}` at line {t.line}, which may {effect},")
                if effect != "reads" and effect != "raises":
                    state.end_restart(f"`{t.text}`, which may {effect}")

    def owner_of(self, arg: list[str], state: _State) -> str | None:
        """The obligation that an argument passes: a name that holds one, or
        an input through a borrowed view of it; None for anything else (a
        nested call passes what its own row says)"""
        tokens = _unwrap(arg, self.table)
        if len(tokens) != 1:
            return None
        if tokens[0] in state.owners:
            return tokens[0]
        name = self.views.resolve(tokens, locals_=False)
        return name if name in self.declared else None

    def take(self, name: str, state: _State, where: Token, what: str) -> None:
        "an owner's obligation leaves it (a consumption or a move)"
        item = self.declared.get(name)
        if item is not None and item.mode in ("borrowed", ""):
            self.fail(where, f"{what} `{name}`, declared {item.mode or 'a pass-through'}, "
                      "which it does not own")
        status = state.owners.get(name)
        if status != HELD:
            self.fail(where, f"{what} `{name}`, which is "
                      f"{'NULL' if status == NULL else 'already consumed or moved'}")
        state.owners[name] = GONE

    def calls(self, tokens: list[Token], state: _State) -> None:
        """the calls among tokens: their argument requirements (V8 inside the
        body) and the obligations they consume"""
        for tkn, args in _calls_in(tokens):
            row = self.table.get(tkn.text)
            if row is None:
                continue
            for exit in sorted(row.exits & {"miss", "deopt"}):
                # A row's guard exit, before the row takes anything
                self.check_exit(exit, state, tkn)
            taken = []
            for arg, spec in zip(args, row.args):
                name = self.views.resolve(arg)
                if name is not None and spec.facts:
                    missing = implied(spec.facts) - state.facts.get(name, frozenset())
                    if missing:
                        self.fail(tkn, f"passes `{' '.join(arg)}` to {row.name}, which "
                                  f"requires {', '.join(sorted(map(str, missing)))} of it")
                if spec.mode in ("consumed", "taken"):
                    owner = self.owner_of(arg, state)
                    if owner is not None:
                        self.take(owner, state, tkn, f"{row.name} "
                                  f"{'consumes' if spec.mode == 'consumed' else 'takes'}")
                        state.end_restart(f"{row.name} consuming `{owner}`")
                        if spec.mode == "taken":
                            taken.append(owner)
                if spec.mode == "out":
                    array = " ".join(_unwrap(arg, self.table))
                    if array in state.arrays:
                        # Publication: wholly written, or written if a STATUS
                        # result says so (the assignment names the local)
                        state.arrays[array] = (PENDING if row.result.shape == "STATUS"
                                               else WRITTEN)
                        state.end_restart(f"the publication of `{array}` by {row.name}")
            for exit in sorted(row.exits - {"next", "miss", "deopt"}):
                # A row's other exits leave the word from inside it, after
                # it has taken what its arguments' modes say: on its raise,
                # a `taken` argument is still the caller's (design 1.2)
                at = state
                if exit == "raise" and taken:
                    at = state.copy()
                    for owner in taken:
                        at.owners[owner] = HELD
                self.check_exit(exit, at, tkn)
            if taken and row.result.shape in ("VALUE_OR_NULL", "STATUS"):
                # Gone if the result reports success: the assignment binds
                # the result's local, whose test decides (refine)
                for owner in taken:
                    state.owners[owner] = TAKEN

    def discharge(self, name: str, state: _State, where: Token) -> None:
        "DEAD(name): only an inline value, or one already moved, owes nothing"
        if state.owners.get(name) == HELD and INLINE not in state.facts.get(name, frozenset()):
            self.fail(where, f"DEAD(`{name}`) drops an owned value: release it, move it "
                      "first, or declare it inline")
        if name in state.owners:
            state.owners[name] = GONE
            state.end_restart(f"DEAD(`{name}`)")

    def simple(self, tokens: list[Token], state: _State) -> _State | None:
        toks = [t for t in tokens if t.kind != "COMMENT"]
        texts = [t.text for t in toks]
        if not toks:
            return state
        head = texts[0]
        if head in GUARDS:
            args = _split(texts[2:_close(texts, 1)], ",")
            self.calls(toks, state)
            self.check_exit(GUARDS[head], state, toks[0])
            if args[0] == ["true"]:
                return None
            self.refine(state, args[0], False)
            return state
        if head in RAISES:
            args = _split(texts[2:_close(texts, 1)], ",") if texts[2:3] != [")"] else [[]]
            for a in args[1:]:
                if (head != "ERROR_IF" or len(a) != 4 or a[:2] != ["holding", "("]
                        or a[3] != ")" or not self.declared.get(a[2])):
                    self.fail(toks[0], f"{head}: `{' '.join(a)}` is not holding(input), "
                              "the one argument a raise site takes after its condition")
            holding = frozenset(a[2] for a in args[1:])
            self.calls(toks, state)
            self.effects(toks, state)
            raised = state.copy()
            if args[0]:
                self.refine(raised, args[0], True)
            self.check_exit("raise", raised, toks[0], holding)
            if head == "ERROR_NO_POP" or args[0] == ["true"]:
                return None
            self.refine(state, args[0], False)
            return state
        if head == "DISPATCH_SAME_OPARG":
            return None
        if head == "DEAD":
            name = self.views.resolve(texts[2:_close(texts, 1)])
            if name is not None:
                self.discharge(name, state, toks[0])
            return state
        if head == "INPUTS_DEAD":
            for name in self.declared:
                self.discharge(name, state, toks[0])
            return state
        if head == "DECREF_INPUTS":
            # The emitters close the inputs that are still held
            for name, item in self.declared.items():
                if state.owners[name] == HELD and item.mode in ("borrowed", ""):
                    self.fail(toks[0], f"DECREF_INPUTS releases `{name}`, declared "
                              f"{item.mode or 'a pass-through'}")
                state.owners[name] = GONE
            self.effects(toks, state)
            state.end_restart("DECREF_INPUTS")
            return state
        self.check_forms(toks, texts, state)
        self.calls(toks, state)
        self.effects(toks, state)
        if "=" in texts:
            self.assignment(toks, texts, texts.index("="), state)
        return state

    def check_forms(self, toks: list[Token], texts: list[str], state: _State,
                    statement: bool = True) -> None:
        """Fail closed: the forms the walk models are calls, assignments of a
        name, a move, a row call or an expression, and element stores into
        an output array.  A name that carries an obligation or a declared
        value anywhere else, a row that makes an owner whose result is not
        bound, and a row that consumes in a nested expression are refused"""
        what = " ".join(t for t in texts if t != ";")
        if statement and texts[0] in UNMODELLED_HEADS:
            self.fail(toks[0], f"`{texts[0]}` is a form the walk does not model")
        tracked = (set(state.owners) | set(state.arrays)
                   | {o.name for o in self.contract.outputs if o.name != "unused"})
        covered = [False] * len(texts)
        expr = texts[:-1] if texts[-1:] == [";"] else texts
        rhs: list[str] | None = None
        if statement and "=" in expr:
            e = expr.index("=")
            rhs = _uncast(expr[e + 1:])
            lhs = expr[:e]
            for j in range(e + 1):
                covered[j] = True
            named = (re.fullmatch(r"\w+", lhs[-1]) is not None
                     and (len(lhs) < 2 or lhs[-2] not in ("->", ".", "*")))
            element = lhs[-1] == "]" and _element_base(texts, e) in state.arrays
            if len(rhs) == 1 and rhs[0] in tracked and (named or element):
                covered[len(texts) - 1 - texts[::-1].index(rhs[0])] = True
        for j in range(len(texts) - 3):
            if texts[j:j + 3] == ["(", "void", ")"] and re.fullmatch(r"\w+", texts[j + 3]):
                covered[j:j + 4] = [True] * 4
        spans = [(i, _close(texts, i + 1)) for i, t in enumerate(texts)
                 if re.fullmatch(r"[A-Za-z_]\w*", t) and texts[i + 1:i + 2] == ["("]]
        for i, end in spans:
            covered[i:end + 1] = [True] * (end + 1 - i)
            row = self.table.get(texts[i])
            if row is None:
                continue
            call = texts[i:end + 1]
            owning = row.new_owner()
            consuming = any(a.mode in ("consumed", "taken") for a in row.args)
            if not (owning or consuming):
                continue
            as_rhs = rhs is not None and _strip(rhs) == call
            whole = statement and rhs is None and _uncast(expr) == call
            as_arg = False
            for p, pend in spans:
                outer = self.table.get(texts[p])
                if outer is None or not p < i <= pend:
                    continue
                inner = texts[p + 2:pend]
                for arg, spec in zip(_split(inner, ",") if inner else [], outer.args):
                    if spec.mode in ("consumed", "taken") and _uncast(arg) == call:
                        as_arg = True
            if owning and whole:
                self.fail(toks[0], f"`{what}` discards the owned result of {row.name}")
            if not (as_rhs or as_arg or (whole and not owning)):
                self.fail(toks[0], f"`{what}` calls {row.name}, which "
                          f"{'makes an owner' if owning else 'consumes an argument'}, in a "
                          "form the walk does not model; bind it: `x = "
                          f"{row.name}(...);`")
        for j, t in enumerate(texts):
            if not covered[j] and t in tracked:
                self.fail(toks[0], f"`{what}` uses `{t}` in a form the walk does not model")

    def assignment(self, toks: list[Token], texts: list[str], i: int, state: _State) -> None:
        lhs = texts[i - 1]
        rhs = _strip(texts[i + 1:-1] if texts[-1] == ";" else texts[i + 1:])
        if lhs == "]":
            self.element_store(toks, texts, i, rhs, state)
            return
        if i >= 2 and texts[i - 2] in ("->", ".") or not re.fullmatch(r"\w+", lhs):
            return      # a store through a pointer: V2's `writes`
        item = self.declared.get(lhs)
        if obligated(state.owners.get(lhs)) and (item is None or item.mode
                                                  in ("consumed", "taken")):
            self.fail(toks[0], f"`{lhs}` is assigned while it holds an owner, which "
                      "would leak")
        out = self.contract.output(lhs)
        if len(rhs) == 1 and (rhs[0] in self.declared or rhs[0] in state.owners):
            # A move, `out = in;` or `PyValue tmp = in;`: the obligation goes
            # with the value, and the new holder must discharge it
            source = self.declared.get(rhs[0])
            if source is not None and source.mode in ("borrowed", ""):
                if out is not None and out.mode == "moved":
                    self.fail(toks[0], f"on next, `{rhs[0]}` is moved, but it is declared "
                              f"{source.mode or 'a pass-through'}")
                return          # a copy of a borrowed view: no obligation
            self.take(rhs[0], state, toks[0], "moves")
            state.owners[lhs] = HELD
            self.new.add(lhs)
            state.assign(lhs, state.facts.get(rhs[0], frozenset()),
                         state.pending.get(rhs[0]), rhs[0])
            state.end_restart(f"the move of `{rhs[0]}`")
            return
        call = _call_of(_uncast(rhs), self.table)
        if call is None:
            if out is not None:
                state.end_restart(f"the assignment of the output `{lhs}`")
            return
        row, args = call
        result = row.result
        if result.shape in ("VALUE_OR_NULL", "STATUS"):
            # The owners this row takes are decided by its result's local
            for owner, status in state.owners.items():
                if status == TAKEN:
                    state.owners[owner] = TAKEN + lhs
        if result.shape == "VIEW" and any(a.mode in ("consumed", "taken") for a in row.args):
            # A stealing view, `o = PyValue_AsKnownObjectSteal(x)`: the row
            # consumed x, and its obligation is now the local's
            source = self.views.resolve(args[0]) if args else None
            state.owners[lhs] = HELD
            self.new.add(lhs)
            state.assign(lhs, state.facts.get(source, frozenset()), None, row.name)
            return
        if result.shape == "STATUS":
            # Its `out` arguments are written on success only: the status
            # local decides which (refine)
            state.status[lhs] = result.failure
            for name, st in state.arrays.items():
                if st == PENDING:
                    state.arrays[name] = f"{PENDING}{lhs}"
            return
        if result.shape not in ("VALUE", "VALUE_OR_NULL"):
            if out is not None:
                state.end_restart(f"the assignment of the output `{lhs}`")
            return
        facts = set(result.facts)
        if result.shape == "VALUE_OR_NULL":
            state.assign(lhs, frozenset(), facts, row.name)
        elif result.null_iff >= 0:
            # NULL exactly where its argument k is
            k = result.null_iff
            source = self.views.resolve(args[k]) if len(args) > k else None
            nonnull = NONNULL in state.facts.get(source, frozenset())
            if out is not None and not out.nullable and not nonnull:
                self.fail(toks[0], f"`{lhs}` may be NULL: {row.name} of "
                          f"`{' '.join(args[k]) if len(args) > k else ''}`, which nothing "
                          "shows is not NULL")
            state.assign(lhs, facts | {NONNULL} if nonnull else frozenset(),
                         None if nonnull else facts | {NONNULL}, row.name)
        else:
            state.assign(lhs, facts | {NONNULL}, None, row.name)
        # An `owned` output is an obligation whether or not its value needs a
        # physical count (1.2), so an inline value assigned to it is one too
        if row.new_owner() or (out is not None and out.mode == "owned"):
            if state.owners.get(lhs) == HELD:
                self.fail(toks[0], f"`{lhs}` is assigned a new owner while it holds one")
            state.owners[lhs] = HELD
            self.new.add(lhs)
            state.end_restart(f"the new owner `{lhs}` from {row.name}")
        elif out is not None:
            state.end_restart(f"the assignment of the output `{lhs}`")


    def element_store(self, toks: list[Token], texts: list[str], i: int, rhs: list[str],
                      state: _State) -> None:
        """`arr[k] = v`: into an owned output array, a publication of v; it
        writes the whole array only if the array has one element"""
        array = _element_base(texts, i)
        if array not in state.arrays:
            return
        if len(rhs) == 1 and (rhs[0] in state.owners):
            item = self.declared.get(rhs[0])
            if item is None or item.mode not in ("borrowed", ""):
                self.take(rhs[0], state, toks[0], "moves")
        size = self.contract.output(array).size
        whole = re.fullmatch(r"1|\(?oparg\s*&\s*1\)?", size.strip()) is not None
        state.arrays[array] = WRITTEN if whole else PARTIAL
        state.end_restart(f"the publication of `{array}`")


_WALKS: dict[tuple[int, bool], tuple[Uop, OwnershipWalk]] = {}


def walk_of(uop: Uop, table: dict[str, Row] | None = None,
            kills: bool = True) -> OwnershipWalk:
    "The walk of a declared uop's body, run"
    table = rows() if table is None else table
    key = (id(uop), kills)
    if table is _ROWS and key in _WALKS:
        return _WALKS[key][1]
    walk = OwnershipWalk(uop, contract_of(uop), table, kills).run()
    if table is _ROWS:
        _WALKS[key] = (uop, walk)
    return walk


# ---------------------------------------------------------------------------
# Result facts (V5), exits (V7) and exported guard facts (V9)

def _assignments(uop: Uop, name: str) -> list[tuple[Token, list[str]]]:
    tokens = _tokens(uop)
    texts = [t.text for t in tokens]
    found = []
    for i, t in enumerate(texts):
        if t == "=" and i and texts[i - 1] == name and (i < 2 or texts[i - 2] not in ("->", ".")):
            end = texts.index(";", i)
            found.append((tokens[i - 1], texts[i + 1:end]))
    return found


def check_results(uop: Uop, contract: Contract, table: dict[str, Row]) -> None:
    """V5: every assignment to an output gives its declared mode and facts.
    That they hold on every path to next, joins and kills included, and that
    a non-nullable output is not NULL there, is the walk's check
    (OwnershipWalk.check_outputs)"""
    inputs = {i.name: i for i in contract.inputs}
    for out in contract.outputs:
        if out.mode in ("", "borrowed"):
            continue
        for where, rhs in _assignments(uop, out.name):
            rhs = _uncast(rhs)
            if out.mode == "moved":
                if rhs != [out.source]:
                    raise analysis_error(f"{uop.name}: `{out.name}` is declared "
                                         f"moved({out.source}), but is assigned "
                                         f"`{' '.join(rhs)}`", where)
                continue
            call = _call_of(rhs, table)
            if call is not None and call[0].result.shape in ("VALUE", "VALUE_OR_NULL"):
                result = call[0].result
                gives, mode, source = implied(result.facts), result.mode, call[0].name
            elif len(rhs) == 1 and rhs[0] in inputs:
                gives, mode, source = implied(inputs[rhs[0]].facts), "moved", rhs[0]
            else:
                raise analysis_error(f"{uop.name}: `{out.name}` is assigned "
                                     f"`{' '.join(rhs)}`, which is no row with a value "
                                     "result", where)
            missing = sorted(map(str, implied(out.facts) - gives - {NONNULL}))
            if missing:
                raise analysis_error(
                    f"{uop.name}: `{out.name}` is declared {', '.join(missing)}, but "
                    f"{source} gives {', '.join(sorted(map(str, gives))) or 'no fact'}",
                    where)
            if out.mode == "inline" and mode != "inline":
                raise analysis_error(f"{uop.name}: `{out.name}` is declared inline, but "
                                     f"{source} gives an {mode} value", where)


def body_exits(uop: Uop, table: dict[str, Row], implicit_guards: int) -> set[str]:
    "The exits that uop's body takes"
    exits = set()
    for tkn, _ in body_calls(uop):
        if tkn.text in GUARDS:
            exits.add(GUARDS[tkn.text])
        elif tkn.text in RAISES:
            exits.add("raise")
        elif tkn.text == "DISPATCH_SAME_OPARG":
            exits.add("jump")
        elif tkn.text in table:
            exits |= table[tkn.text].exits
    if implicit_guards:
        exits.add("miss")
    if not uop.properties.always_exits:
        exits.add("next")
    return exits


def check_exits(uop: Uop, contract: Contract, table: dict[str, Row],
                implicit_guards: int) -> None:
    "V7: the declared exits are the body's"
    actual = body_exits(uop, table, implicit_guards)
    if actual != set(contract.exits):
        raise analysis_error(f"{uop.name}: declares exits({', '.join(sorted(contract.exits))}), "
                             f"but the body takes {', '.join(sorted(actual))}", uop.body.open)


def check_guard_facts(uop: Uop, contract: Contract, walk: OwnershipWalk) -> None:
    """V9: the facts of a value that leaves the uop (a pass-through, or an
    input that an output is moved from) that hold at its `next` exit, after
    the body's kills and joins, are what the uop declares there: a guard
    uop's on its pass-through, an in-body guard's (a refinement site) on the
    outputs moved from the guarded inputs.  A conditional guard's facts do
    not reach the exit; neither does a killed heap fact, nor any fact of a
    value left in a home that a reentry site may have rewritten."""
    for out in contract.outputs:
        if out.mode == "moved":
            source, live = out.source, walk.exported(out.name)
        elif not out.mode:
            source = out.name
            live = walk.exported(source)
        else:
            continue
        item = contract.input(source)
        entry = (implied(item.facts | (set() if item.nullable else {NONNULL}))
                 if item is not None else frozenset())
        declared = implied(out.facts)
        extra = declared - live - {NONNULL}
        if extra:
            rehomed = walk.exit.rehomed.get(source) if walk.exit is not None else None
            if rehomed is not None and not out.mode:
                raise analysis_error(
                    f"{uop.name}: `{out.name}` is declared "
                    f"{', '.join(sorted(map(str, extra)))}, but {rehomed} may replace the "
                    "value in its home (a finalizer can write it)", uop.body.open)
            raise analysis_error(
                f"{uop.name}: `{out.name}` is declared {', '.join(sorted(map(str, extra)))}, "
                f"which nothing establishes on every path to next", uop.body.open)
        missing = live - declared - entry
        if missing:
            raise analysis_error(
                f"{uop.name}: the guards establish {', '.join(sorted(map(str, missing)))} "
                f"of `{source}`, which `{out.name}` does not declare", uop.body.open)


_VERIFIED: set[int] = set()


def verify_uop(uop: Uop, implicit_guards: int = 0, table: dict[str, Row] | None = None) -> None:
    "Check a declared uop's body against its contract (V1-V5, V7, V9; V6 inside it)"
    table = rows() if table is None else table
    contract = contract_of(uop)
    if contract is None or (id(uop), implicit_guards) in _VERIFIED:
        return
    check_callees(uop, table)
    check_effects(uop, contract, table)
    check_feedback(uop, contract, table)
    check_guard_conditions(uop, table)
    for tkn, _ in body_calls(uop):
        if tkn.text in GUARDS:
            site_facts(uop, tkn, table)     # a condition without facts needs a reason
    check_results(uop, contract, table)
    walk = walk_of(uop, table)
    check_exits(uop, contract, table, implicit_guards)
    check_guard_facts(uop, contract, walk)
    if table is _ROWS:
        _VERIFIED.add((id(uop), implicit_guards))


# ---------------------------------------------------------------------------
# A handler (V6 across uops, V8, V10)

REGION_KINDS = ("inline", "split", "shared")


def verify_handler(inst: Instruction, table: dict[str, Row] | None = None,
                   leaf: dict[str, bool] | None = None) -> None:
    """In a handler whose uops all declare contracts: each fact that a uop
    requires of an input is live where it runs (V8), with heap facts dying
    at every KILLS effect and an unestablished `object` met by the
    generator's implicit guard; no guard follows a uop after which the word
    cannot restart (V6).  A declared region kind fits the uops (V10).
    leaf maps uop names to whether the generator made them leaf."""
    table = rows() if table is None else table
    parts = [p for p in inst.parts if isinstance(p, Uop)]
    if inst.region is not None:
        if inst.region not in REGION_KINDS:
            raise analysis_error(f"{inst.name}: `{inst.region}` is not a region kind "
                                 f"({', '.join(REGION_KINDS)})", inst.where)
        if inst.region == "inline" and leaf is not None:
            for part in parts:
                if not leaf.get(part.name, False):
                    raise analysis_error(f"{inst.name}: region(inline), but {part.name} "
                                         "is not leaf", inst.where)
    contracts = [contract_of(p) for p in parts]
    if not parts or any(c is None for c in contracts):
        return
    # A stack of value numbers; facts by value.  An array is one value, its
    # elements' common facts, with its declared size
    stack: list[int] = []
    facts: dict[int, frozenset[VFact]] = {}
    sizes: dict[int, str] = {}
    fresh = iter(range(1, 1 << 20))
    restartable, why = True, ""

    def pop(item: Item) -> int | None:
        "The value of an input, or None if the stack's depth is unknown"
        if item.array:
            if stack and sizes.get(stack[-1]) == item.size:
                return stack.pop()
            if not item.size.isdigit():
                # An array of another size, or of values not pushed as one:
                # the depth below it is unknown
                return None
            elements = [pop(Item(item.name, item.mode)) for _ in range(int(item.size))]
            if None in elements:
                return None
            v = next(fresh)
            facts[v] = frozenset.intersection(frozenset({NONNULL}),
                                              *(facts[e] for e in elements))
            sizes[v] = item.size
            return v
        if stack:
            if stack[-1] in sizes:
                return None         # an array whose elements are not known apart
            return stack.pop()
        v = next(fresh)
        facts[v] = frozenset({NONNULL})
        return v

    # Whether the stack's depth is known: past an array of unknown size the
    # fact walk (V8) stops, and restartability and the uops' own checks go on
    known = True
    for index, (part, contract) in enumerate(zip(parts, contracts)):
        bound: dict[str, int] = {}
        for item in reversed(contract.inputs):
            value = pop(item) if known else None
            if value is None:
                known = False
                break
            bound[item.name] = value
        # A guard: EXIT_IF and DEOPT_IF, or a row with a miss or deopt exit
        guards = any(t.text in GUARDS or (t.text in table and table[t.text].exits
                                          & {"miss", "deopt"})
                     for t, _ in body_calls(part))
        implicit = []
        for item in contract.inputs if known else ():
            value = bound[item.name]
            missing = implied(item.facts) - facts.get(value, frozenset())
            if missing <= {OBJECT, NONNULL} and OBJECT in missing and part.is_guard() \
                    and not part.properties.inline_ok:
                # The generator's implicit pointer-space guard (V8)
                implicit.append(item.name)
                facts[value] = implied(facts.get(value, frozenset()) | {OBJECT})
                missing = set()
            if missing and item.name != "unused":
                raise analysis_error(
                    f"{inst.name}: {part.name} requires "
                    f"{', '.join(sorted(map(str, missing)))} of `{item.name}`, which "
                    "nothing before it establishes or which an effect since killed",
                    inst.where)
        if not known and part.is_guard() and not part.properties.inline_ok:
            # Unknown facts: the generator may guard any input that needs
            # an object
            implicit = [i.name for i in contract.inputs if OBJECT in implied(i.facts)]
        if (guards or implicit) and not restartable:
            raise analysis_error(f"{inst.name}: the guard in {part.name} follows {why}, "
                                 "so the word cannot restart (design.md 3.1)", inst.where)
        if known:
            # Past an unknown depth the record's builder verifies the uop with
            # the implicit guards it actually emits
            verify_uop(part, len(implicit), table)
        # After the uop: its kills.  Heap facts die at a KILLS effect; at a
        # reentry every value left in a home (the stack below the uop and
        # its pass-throughs) may have been replaced there, so all of its
        # facts die.  Then the facts the uop declares on its outputs, which
        # V9 holds to the facts live at its exit.
        if contract.effects & KILLS:
            facts = {v: without_heap_facts(f) for v, f in facts.items()}
        if contract.effects & REENTRY and known:
            homes = set(stack) | {bound[i.name] for i in contract.inputs if not i.mode
                                  and contract.output(i.name) is not None}
            for v in homes:
                facts[v] = frozenset()
        changes = contract.effects - {"reads", "raises"}
        if restartable and changes:
            restartable, why = False, f"{part.name}, which may {', '.join(sorted(changes))}"
        if restartable and any(o.mode in ("owned", "inline") for o in contract.outputs):
            restartable, why = False, f"{part.name}, which makes an owner"
        last = index == len(parts) - 1
        for step, name in skeleton_steps(contract, last):
            # The generator's skeleton around the bodies (design.md 3.1),
            # placed where it emits them
            if restartable and step in ("detach", "publish"):
                restartable = False
                why = (f"the skeleton's detachment of `{name}` after {part.name}"
                       if step == "detach" else
                       f"the skeleton's publication of `{name}`")
            if step == "close":
                # The displaced value's close may run a finalizer, which can
                # replace any home's value (design.md 3.2)
                facts = {v: frozenset() for v in facts}
        if not known:
            continue
        for item in contract.outputs:
            if not item.mode and item.name in bound:
                value = bound[item.name]
            elif not item.mode:
                # A window's slots the uop leaves as they are, which no
                # input names: unknown values
                value = next(fresh)
                facts[value] = frozenset()
            elif item.mode == "moved":
                value = bound[item.source]
            else:
                value = next(fresh)
                facts[value] = frozenset()
            facts[value] = implied(facts.get(value, frozenset()) | item.facts)
            if item.array:
                sizes[value] = item.size
            stack.append(value)


def skeleton_steps(contract: Contract, last: bool) -> list[tuple[str, str]]:
    """The skeleton's steps after a uop, in the order the generator emits
    them: each input taken from its home is detached at the end of the uop
    that takes it (IR_DETACH; a consumed input's home is cleared no later);
    after the word's last uop the outputs are published to their homes
    (IR_STORE_DST) and each displaced old value is closed (ir_old)"""
    steps = [("detach", i.name) for i in contract.inputs
             if i.mode in ("consumed", "taken") and i.name != "unused"]
    if last:
        steps += [("publish", o.name) for o in contract.outputs if o.name != "unused"]
        steps += [("close", o.name) for o in contract.outputs if o.name != "unused"]
    return steps


# ---------------------------------------------------------------------------
# The record of a declared uop

def record_guard_facts(uop: Uop, site: Token, inputs) -> tuple[tuple[Fact, ...], str]:
    """The record's facts of a guard site of a declared uop, as the site's
    condition states them (not their implications), and its reason.  Only
    the facts that hold on every path from the site to the uop's `next`
    exit are recorded: the record's readers (the templates) take a guard's
    facts as the uop's, so a conditional guard's are not.  Their lifetime
    against effects is the reader's: the record keeps the effects' order."""
    table = rows()
    walk = walk_of(uop, table, kills=False)
    cond, reason = guard_condition(uop, site)
    views = Views(uop, table)
    found = condition_facts(cond, views, table, holds=False)
    if found is None:
        site_facts(uop, site, table)        # refuses a condition without a reason
        return (), reason
    stated = _stated_facts(cond, views, table, False)
    values = {i.name: f"v{i.value}" for i in inputs}
    result = []
    for name, facts in stated.items():
        # A local's fact is recorded on the local, as the inference did
        live = walk.exit.facts.get(name, frozenset()) if walk.exit else frozenset()
        result += [f.record(values.get(name, name)) for f in facts if f in live]
    return tuple(result), reason or found[1]


def _stated_facts(cond: list[str], views: Views, table: dict[str, Row],
                  holds: bool) -> dict[str, list[VFact]]:
    """The facts as the TEST and MEASURE rows of cond state them, in the
    order of the condition and their arguments, without implications"""
    found = condition_facts(cond, views, table, holds) or ({}, "")
    result: dict[str, list[VFact]] = {}
    for tkn_text, args in _calls_in_texts(cond):
        row = table.get(tkn_text)
        if row is None:
            continue
        for arg, (true, false) in zip(args, row.result.tests):
            name = views.resolve(arg)
            if name is None:
                continue
            for f in sorted(true | false):
                if f in found[0].get(name, ()) and f not in result.get(name, []):
                    result.setdefault(name, []).append(f)
        if row.result.shape == "MEASURE":
            name = views.resolve(args[0])
            for f in sorted(found[0].get(name, ())):
                if f.kind == row.result.measure and f not in result.get(name, []):
                    result.setdefault(name, []).append(f)
    # Reads of FIELD rows, `p->field`
    for i, t in enumerate(cond):
        if t != "->" or i + 1 >= len(cond):
            continue
        rows_ = [r for r in table.values() if r.kind == "FIELD" and r.field == cond[i + 1]]
        for row in rows_:
            stated = (row.result.tests[0][0] | row.result.tests[0][1]
                      if row.result.shape == "TEST" else frozenset())
            for name, facts in found[0].items():
                for f in sorted(facts):
                    if (f in stated or f.kind == row.result.measure) \
                            and f not in result.get(name, []):
                        result.setdefault(name, []).append(f)
    return result


def _calls_in_texts(texts: list[str]) -> list[tuple[str, list[list[str]]]]:
    calls = []
    for i, t in enumerate(texts):
        if re.fullmatch(r"\w+", t) and i + 1 < len(texts) and texts[i + 1] == "(":
            inner = texts[i + 2:_close(texts, i + 1)]
            calls.append((t, _split(inner, ",") if inner else []))
    return calls


def record_results(uop: Uop, outputs) -> tuple[Fact, ...]:
    """The facts of a declared uop's new values: the declared facts of its
    owned and inline outputs, as declared"""
    contract = contract_of(uop)
    by_name = {o.name: o for o in outputs}
    result = []
    for item in contract.outputs:
        out = by_name.get(item.name)
        if out is not None and item.mode in ("owned", "inline"):
            result += [f.record(f"v{out.value}") for f in sorted(item.facts)]
    return tuple(result)
