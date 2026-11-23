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
ROW_KINDS = ("PRIMITIVE", "HELPER", "MACRO", "INTRINSIC", "DESTRUCTOR")
RESULT_SHAPES = ("NONE", "SCALAR", "VIEW", "MEASURE", "TEST", "CONTEXT", "VALUE",
                 "VALUE_OR_NULL")
BUILDS = ("gil", "free_threaded", "all")


@dataclass(frozen=True)
class Arg:
    mode: str
    facts: frozenset[VFact] = frozenset()


@dataclass(frozen=True)
class Result:
    shape: str
    mode: str = ""                          # VALUE, VALUE_OR_NULL
    facts: frozenset[VFact] = frozenset()   # VALUE, VALUE_OR_NULL
    measure: str = ""                       # MEASURE
    tests: tuple[tuple[frozenset[VFact], frozenset[VFact]], ...] = ()  # TEST
    reason: str = ""                        # CONTEXT


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
            return Result(shape, words[0], frozenset(parse_fact(w, None) for w in words[1:]))
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


def _parse_row(kind: str, fields: list[str]) -> Row:
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
        if len(fields) != 5:
            raise ValueError("MACRO(name, ARGS(...), result, EFFECTS(...), EXITS(...))")
        name, args, result, effects, exits = fields
        return Row(kind, name, args=_parse_args(_inner(args, "ARGS")),
                   result=_parse_result(result),
                   effects=_parse_names(effects, "EFFECTS", EFFECTS),
                   exits=_parse_names(exits, "EXITS", EXITS))
    if len(fields) != 6:
        raise ValueError(f"{kind}(name, ret, (params), ARGS(...), result, EFFECTS(...))")
    name, ret, params, args, result, effects = fields
    params = params.strip()
    if not (params.startswith("(") and params.endswith(")")):
        raise ValueError("the parameters are a parenthesized list of C types")
    row = Row(kind, name, ret,
              tuple(p.strip() for p in _split_text(params[1:-1]) if p.strip()),
              _parse_args(_inner(args, "ARGS")), _parse_result(result),
              _parse_names(effects, "EFFECTS", EFFECTS))
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


_ROWS: dict[str, Row] = {}


def rows() -> dict[str, Row]:
    if not _ROWS:
        _ROWS.update(parse_rows(COMPONENTS_HEADER.read_text()))
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
        if row.kind == "HELPER":
            check(f"&{row.name}", f"{row.ret} (*)({', '.join(row.params)})", row.name)
        elif row.kind == "DESTRUCTOR":
            check(f"&{row.name}", "void (*)(PyObject *)", row.name)
        elif row.kind == "PRIMITIVE":
            call = ", ".join(f"({p}){{0}}" for p in row.params)
            check(f"{row.name}({call})", row.ret, row.name)
    return lines


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


@dataclass(frozen=True)
class Contract:
    inputs: tuple[Item, ...]
    outputs: tuple[Item, ...]
    effects: frozenset[str]
    exits: frozenset[str]

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
                stack_item.is_array())


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
        contract = Contract(inputs, outputs, effects, exits)
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
        if name in DSL_CONSTRUCTS or name in table or CHECKED_ARITHMETIC.fullmatch(name):
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
        if text in ir_records.ASSIGNS and ir_records._lvalue_is_heap(texts, i):
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


def record_effect_sites(uop: Uop) -> list[tuple[Token, str]]:
    "effect_sites() in ir_records' terms, for its guard-order check"
    return [(s.token, SITE_EFFECT[s.effect]) for s in effect_sites(uop)]


# ---------------------------------------------------------------------------
# Guard facts (V9)

class Views:
    """What the names of a uop body stand for: an input is itself; a local
    assigned from a VIEW row of a name is a view of that name"""

    def __init__(self, uop: Uop, table: dict[str, Row]):
        self.names = {i.name: i.name for i in uop.stack.inputs if i.name != "unused"}
        texts = [t.text for t in _tokens(uop)]
        for i, t in enumerate(texts):
            if t != "=" or i == 0 or texts[i - 1] in self.names:
                continue
            end = texts.index(";", i) if ";" in texts[i:] else len(texts)
            rhs = _uncast(texts[i + 1:end])
            if rhs and rhs[0] in table and table[rhs[0]].result.shape == "VIEW":
                args = _call(rhs, rhs[0])
                if args and len(args) == 1:
                    inner = self.resolve(args[0])
                    if inner is not None:
                        self.names.setdefault(texts[i - 1], inner)

    def resolve(self, tokens: list[str]) -> str | None:
        tokens = _uncast(tokens)
        if len(tokens) == 1:
            return self.names.get(tokens[0])
        return None


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
    call = _call_of(sides[0], table)
    if call is None or call[0].result.shape != "MEASURE":
        return None
    name = views.resolve(call[1][0])
    arg = " ".join(_strip(sides[1]))
    if name is None or not re.fullmatch(r"\w+", arg):
        return None
    if (op == "==") != holds:
        return {}, ""
    return {name: implied({VFact(call[0].result.measure, arg)})}, ""


def _guard_condition_text(uop: Uop, site: Token) -> tuple[list[str], str]:
    return guard_condition(uop, site)


def site_facts(uop: Uop, site: Token, table: dict[str, Row] | None = None
               ) -> tuple[Facts, str]:
    """V9: the facts that a guard site establishes of the names of the
    uop's body, from the rows its condition calls, and its reason"""
    table = rows() if table is None else table
    cond, reason = _guard_condition_text(uop, site)
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
# Ownership and restartability on each exit (V4, V6 inside a uop)

HELD, GONE, NULL = "held", "gone", "null"


@dataclass
class _State:
    owners: dict[str, str]          # an input or a new owner: held, gone or null
    facts: dict[str, frozenset[VFact]]
    restartable: bool = True
    why: str = ""                   # what ended restartability

    def copy(self) -> "_State":
        return _State(dict(self.owners), dict(self.facts), self.restartable, self.why)

    def end_restart(self, why: str) -> None:
        if self.restartable:
            self.restartable, self.why = False, why

    def kill_heap_facts(self) -> None:
        self.facts = {n: without_heap_facts(f) for n, f in self.facts.items()}


class OwnershipWalk:
    """Walks a declared uop's statements, tracking each obligation (the
    inputs, and every new owner the body makes), the facts of the names,
    and whether the word can still restart; checks every exit site"""

    def __init__(self, uop: Uop, contract: Contract, table: dict[str, Row]):
        self.uop, self.contract, self.table = uop, contract, table
        self.views = Views(uop, table)
        self.declared = {i.name: i for i in contract.inputs if i.name != "unused"}
        self.new: set[str] = set()
        self.site_effects: dict[int, list[str]] = {}
        for site in effect_sites(uop, table):
            self.site_effects.setdefault(id(site.token), []).append(site.effect)

    def run(self) -> None:
        facts = {n: implied(i.facts | (set() if i.nullable else {NONNULL}))
                 for n, i in self.declared.items()}
        state = _State({n: HELD for n in self.declared}, facts)
        end = self.block(self.uop.body.body, state)
        if end is not None and not self.uop.properties.always_exits:
            self.check_exit("next", end, self.uop.body.close)

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
        "where paths meet: the same obligations, the facts all of them have"
        live = [e for e in ends if e is not None]
        if not live:
            return None
        first = live[0].copy()
        for other in live[1:]:
            for name in set(first.owners) | set(other.owners):
                a, b = first.owners.get(name, GONE), other.owners.get(name, GONE)
                if (a == HELD) != (b == HELD):
                    self.fail(self.uop.body.open, f"`{name}` is owned on one path and "
                              "not on another where they join")
                first.owners[name] = a
            first.facts = _intersection([first.facts, other.facts])
            if not other.restartable:
                first.end_restart(other.why)
        return first

    def refine(self, state: _State, cond: list[str], holds: bool) -> None:
        found = condition_facts(cond, self.views, self.table, holds)
        if found:
            for name, facts in found[0].items():
                state.facts[name] = implied(state.facts.get(name, frozenset()) | facts)
        # `PyValue_IsNull(x)`: a new owner that is NULL owes nothing
        null = _call(cond, "PyValue_IsNull")
        if null is not None and len(null) == 1:
            name = " ".join(null[0])
            if state.owners.get(name) == HELD and name in self.new:
                if holds:
                    state.owners[name] = NULL
                else:
                    state.facts[name] = implied(state.facts.get(name, frozenset()) | {NONNULL})

    def effects(self, tokens: list[Token], state: _State) -> None:
        "the effects of the sites among tokens: heap facts die, restart ends"
        for t in tokens:
            for effect in self.site_effects.get(id(t), ()):
                if effect in KILLS:
                    state.kill_heap_facts()
                if effect != "reads" and effect != "raises":
                    state.end_restart(f"`{t.text}`, which may {effect}")

    def calls(self, tokens: list[Token], state: _State) -> None:
        """the calls among tokens: their argument requirements (V8 inside the
        body) and the inputs they consume"""
        for tkn, args in _calls_in(tokens):
            row = self.table.get(tkn.text)
            if row is None:
                continue
            for arg, spec in zip(args, row.args):
                name = self.views.resolve(arg)
                if name is None:
                    continue
                missing = implied(spec.facts) - state.facts.get(name, frozenset())
                if spec.facts and missing:
                    self.fail(tkn, f"passes `{' '.join(arg)}` to {row.name}, which requires "
                              f"{', '.join(sorted(map(str, missing)))} of it")
                if spec.mode in ("consumed", "taken") and name in self.declared:
                    if self.declared[name].mode in ("borrowed", ""):
                        self.fail(tkn, f"passes `{name}`, declared "
                                  f"{self.declared[name].mode or 'a pass-through'}, to "
                                  f"{row.name}, which consumes it")
                    if state.owners.get(name) == HELD:
                        state.owners[name] = GONE
                        state.end_restart(f"{row.name} consuming `{name}`")

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
            holding = frozenset(" ".join(a[2:-1]) for a in args[1:]
                                if a[:2] == ["holding", "("])
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
            for name, item in self.declared.items():
                if state.owners[name] == HELD and item.mode in ("borrowed", ""):
                    self.fail(toks[0], f"DECREF_INPUTS releases `{name}`, declared "
                              f"{item.mode or 'a pass-through'}")
                state.owners[name] = GONE
            self.effects(toks, state)
            state.end_restart("DECREF_INPUTS")
            return state
        self.calls(toks, state)
        self.effects(toks, state)
        if "=" in texts:
            self.assignment(toks, texts, texts.index("="), state)
        return state

    def assignment(self, toks: list[Token], texts: list[str], i: int, state: _State) -> None:
        lhs = texts[i - 1]
        rhs = _strip(texts[i + 1:-1] if texts[-1] == ";" else texts[i + 1:])
        out = self.contract.output(lhs)
        if len(rhs) == 1 and rhs[0] in self.declared:
            # A move: `out = in;` or `PyValue tmp = in;`
            source = self.declared[rhs[0]]
            if source.mode in ("borrowed", ""):
                if out is not None and out.mode == "moved":
                    self.fail(toks[0], f"on next, `{rhs[0]}` is moved, but it is declared "
                              f"{source.mode or 'a pass-through'}")
                return
            if state.owners.get(rhs[0]) == HELD:
                state.owners[rhs[0]] = GONE
                state.end_restart(f"the move of `{rhs[0]}`")
            if out is not None:
                state.facts[lhs] = state.facts.get(rhs[0], frozenset())
            return
        call = _call_of(_uncast(rhs), self.table)
        # An `owned` output is an obligation whether or not its value needs a
        # physical count (1.2), so an inline value assigned to it is one too
        value = call is not None and call[0].result.shape in ("VALUE", "VALUE_OR_NULL")
        if value and (call[0].new_owner() or (out is not None and out.mode == "owned")):
            if state.owners.get(lhs) == HELD:
                self.fail(toks[0], f"`{lhs}` is assigned a new owner while it holds one")
            state.owners[lhs] = HELD
            self.new.add(lhs)
            state.end_restart(f"the new owner `{lhs}` from {call[0].name}")
        elif out is not None:
            state.end_restart(f"the assignment of the output `{lhs}`")


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
    """V5: every assignment to an output gives its declared mode and facts,
    so that their join does"""
    inputs = {i.name: i for i in contract.inputs}
    texts = [t.text for t in _tokens(uop)]
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
            if (call is not None and call[0].result.shape == "VALUE_OR_NULL"
                    and not out.nullable):
                text = " ".join(texts)
                if not re.search(rf"\b(ERROR_IF|if) \( PyValue_IsNull \( {out.name} \) [),]",
                                 text):
                    raise analysis_error(f"{uop.name}: `{out.name}` may be NULL from "
                                         f"{source}, and nothing raises on it", where)


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


def guard_facts_of(uop: Uop, table: dict[str, Row]) -> Facts:
    "The facts that all guard sites of uop establish, by name"
    derived: list[Facts] = []
    for tkn, _ in body_calls(uop):
        if tkn.text in GUARDS:
            derived.append(site_facts(uop, tkn, table)[0])
    return _union(derived)


def check_guard_facts(uop: Uop, contract: Contract, derived: Facts) -> None:
    """V9: what the guards of a uop establish of a value that leaves it (a
    pass-through, or an input that an output is moved from) is what the
    uop declares there: a guard uop on its pass-through, an in-body guard
    (a refinement site) on the outputs moved from the guarded inputs"""
    for out in contract.outputs:
        source = out.name if not out.mode else out.source if out.mode == "moved" else None
        if source is None:
            continue
        item = contract.input(source)
        required = implied(item.facts) if item is not None else frozenset()
        established = implied(required | derived.get(source, frozenset()))
        declared = implied(out.facts)
        extra = declared - established
        if extra:
            raise analysis_error(
                f"{uop.name}: `{out.name}` is declared {', '.join(sorted(map(str, extra)))}, "
                f"which nothing establishes", uop.body.open)
        missing = implied(derived.get(source, frozenset())) - declared - required
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
    check_guard_conditions(uop, table)
    OwnershipWalk(uop, contract, table).run()
    check_results(uop, contract, table)
    check_exits(uop, contract, table, implicit_guards)
    check_guard_facts(uop, contract, guard_facts_of(uop, table))
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
    # A stack of value numbers; facts by value
    stack: list[int | None] = []
    facts: dict[int, frozenset[VFact]] = {}
    fresh = iter(range(1, 1 << 20))
    restartable, why = True, ""
    for part, contract in zip(parts, contracts):
        if any(i.array for i in contract.inputs):
            return      # arrays have no static depth: the pilot stops here
        n = len(contract.inputs)
        while len(stack) < n:
            v = next(fresh)
            stack.insert(0, v)
            facts[v] = frozenset({NONNULL})
        bound = dict(zip((i.name for i in contract.inputs), stack[len(stack) - n:]))
        del stack[len(stack) - n:]
        derived = guard_facts_of(part, table)
        guards = bool(derived) or any(t.text in GUARDS for t, _ in body_calls(part))
        implicit = []
        for item in contract.inputs:
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
        if (guards or implicit) and not restartable:
            raise analysis_error(f"{inst.name}: the guard in {part.name} follows {why}, "
                                 "so the word cannot restart (design.md 3.1)", inst.where)
        verify_uop(part, len(implicit), table)
        # After the uop: its guards' facts, then its effects' kills
        for name, f in derived.items():
            if name in bound:
                facts[bound[name]] = implied(facts.get(bound[name], frozenset()) | f)
        if contract.effects & KILLS:
            facts = {v: without_heap_facts(f) for v, f in facts.items()}
        changes = contract.effects - {"reads", "raises"}
        if restartable and (changes or any(i.mode for i in contract.inputs)
                            or any(o.mode in ("owned", "inline") for o in contract.outputs)):
            restartable = False
            why = (f"{part.name}, which may {', '.join(sorted(changes))}" if changes
                   else f"{part.name}, which takes or makes an owner")
        for item in contract.outputs:
            if item.array:
                stack.append(None)
                continue
            if not item.mode:
                value = bound[item.name]
            elif item.mode == "moved":
                value = bound[item.source]
            else:
                value = next(fresh)
                facts[value] = frozenset()
            if value is not None:
                facts[value] = implied(facts.get(value, frozenset()) | item.facts)
            stack.append(value)


# ---------------------------------------------------------------------------
# The record of a declared uop

def record_guard_facts(uop: Uop, site: Token, inputs) -> tuple[tuple[Fact, ...], str]:
    """The record's facts of a guard site of a declared uop, as the site's
    condition states them (not their implications), and its reason"""
    table = rows()
    cond, reason = _guard_condition_text(uop, site)
    views = Views(uop, table)
    found = condition_facts(cond, views, table, holds=False)
    if found is None:
        site_facts(uop, site, table)        # refuses a condition without a reason
        return (), reason
    stated = _stated_facts(cond, views, table, False)
    values = {i.name: f"v{i.value}" for i in inputs}
    result = []
    for name, facts in stated.items():
        if name in values:
            result += [f.record(values[name]) for f in facts]
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
