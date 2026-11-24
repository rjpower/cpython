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

import copy
import dataclasses
from dataclasses import dataclass, field
import re

import analyzer
from analyzer import IR_MARKER_EFFECTS, Instruction, Uop, analysis_error
from generators_common import ROOT
from lexer import Token
from parser import BlockStmt, ForStmt, IfStmt, MacroIfStmt, SimpleStmt, Stmt, WhileStmt
import ir_records
from ir_records import (ALLOCATES, EFFECTS as RECORD_EFFECTS, MAY_RAISE, MAY_RELEASE,
                        MAY_RUN_PYTHON, READS_HEAP, WRITES_HEAP, Fact, _call, _close,
                        _split, _strip, _uncast, guard_condition)


# ---------------------------------------------------------------------------
# The vocabulary (design.md section 1).  Each set is closed.

# retires_frame: drops a frame's references, its code's among them (the
# frame clears and pops, error processing): a property of a callee that
# constrains its callers (VT3), as Torque's `transitioning`.  No other
# effect covers it: a uop that calls such a row declares it
EFFECTS = ("reads", "writes", "allocates", "raises", "releases", "runs_python",
           "polls", "switches_frame", "retires_frame")
# Each declared effect in the record's terms; runs_python, polls,
# switches_frame and retires_frame may do anything
RECORD_EFFECT = {
    "reads": {READS_HEAP}, "writes": {WRITES_HEAP}, "allocates": {ALLOCATES},
    "raises": {MAY_RAISE}, "releases": {MAY_RELEASE},
    "runs_python": set(RECORD_EFFECTS), "polls": set(RECORD_EFFECTS),
    "switches_frame": set(RECORD_EFFECTS), "retires_frame": set(RECORD_EFFECTS),
}
# The one record effect that stands for a site of each declared effect
# (ir_records.guard_order reads sites)
SITE_EFFECT = {
    "reads": READS_HEAP, "writes": WRITES_HEAP, "allocates": ALLOCATES,
    "raises": MAY_RAISE, "releases": MAY_RELEASE, "runs_python": MAY_RUN_PYTHON,
    "polls": MAY_RUN_PYTHON, "switches_frame": MAY_RUN_PYTHON,
    "retires_frame": MAY_RUN_PYTHON,
}
# The effects that a declared effect covers
COVERS = {
    "runs_python": set(EFFECTS) - {"polls", "switches_frame", "retires_frame"},
    "polls": set(EFFECTS) - {"switches_frame", "retires_frame"},
    "switches_frame": set(EFFECTS) - {"polls", "retires_frame"},
    "retires_frame": set(EFFECTS) - {"polls", "switches_frame"},
}
# The effects at whose sites heap facts die (1.1)
KILLS = frozenset(("writes", "releases", "runs_python", "polls", "switches_frame",
                   "retires_frame"))

# `redirect`: a monitoring callback moved the frame to another word (a
# debugger's f_lineno jump, which retires the temporaries it does not keep);
# the word leaves for frame->instr_ptr without the rest of its handler.  On
# the uop's other exits no callback redirected the frame.
# abort: the process ends (Py_FatalError), the only exit of a handler that
# never runs: a cache, RESERVED, the stack-only opcodes, operand words
EXITS = ("next", "branch", "jump", "miss", "deopt", "raise", "enter", "return",
         "yield", "exit", "redirect", "abort")
# The exits after which no compiled code of the word's frame state runs: the
# error route and the evaluator's exit.  A redirect continues the frame at
# another word, so its effects are kept.
LEAVING_EXITS = frozenset(("raise", "exit"))

# Feedback access (design 1.6, section 6): what a row or a uop reads and
# writes of its site's feedback, transitively.  `entry` is every field of
# the site's entry, its header's counter included; `handler` is the word's
# handler, which a specializer rewrites.  A uop's cache operands are reads
# of their fields and need no clause.
FEEDBACK_MODES = ("read", "write")
FEEDBACK_FIELDS = ("counter", "entry", "handler")
FEEDBACK_COVERS = {"entry": {"counter"}}

INPUT_MODES = ("borrowed", "consumed", "taken", "closed")
# The input modes whose value the body does not own: a pass-through (""),
# a borrowed input, and a closed one, which the skeleton closes after the
# uop on next and on raise (design 1.2)
UNOWNED = ("", "borrowed", "closed")
OUTPUT_MODES = ("owned", "inline", "borrowed", "moved")
# `lvalue`: a macro argument that the macro assigns (PyValue_CLEAR(x) sets x)
ARG_MODES = ("borrowed", "consumed", "taken", "scalar", "object", "tstate", "frame",
             "out", "lvalue", "destructor")

# Facts: the vocabulary's words and the record's Fact kinds.  A fact with an
# argument is written `kind(arg)`; `exact(T1|T2)` is a union of exact types,
# `type_slot(slot,fn)` a type slot's value, `in_bounds(x)` an index within
# the length of input x, `is(K)` the identity of a CONSTANT row's value,
# `encoding(E1|E2|null)` a union of encodings (the representation table).
FACT_KIND = {
    "exact": "EXACT_TYPE",
    "type_version": "TYPE_VERSION", "has_values": "INLINE_VALUES_VALID",
    "shape": "SHAPE", "keys_version": "KEYS_VERSION", "length": "LENGTH",
    "in_bounds": "IN_BOUNDS", "nonnegative": "NON_NEGATIVE",
    "nonzero": "NON_ZERO", "is": "IDENTITY", "subtype": "SUBTYPE",
    "not_subtype": "NOT_SUBTYPE", "not_exact": "NOT_TYPE", "type_flag": "TYPE_FLAG",
    "type_slot": "TYPE_SLOT", "class_version": "CLASS_VERSION",
    "func_version": "FUNC_VERSION", "argcount": "ARGCOUNT", "vectorcall": "VECTORCALL",
    "convention": "CONVENTION", "ascii": "ASCII", "no_managed_dict": "NO_MANAGED_DICT",
    "unique": "UNIQUE", "self": "SELF", "method": "METHOD",
    "tagged_index": "TAGGED_INDEX",
    # Every representation word is one record kind, ENCODING, with the set
    # of kinds of word that it allows (VFact.record)
    **{word: "ENCODING" for word in ("nonnull", "null", "inline", "inline_int",
                                    "inline_float", "object", "encoding")},
}
VOCABULARY_OF_KIND = {v: k for k, v in FACT_KIND.items() if v != "ENCODING"}
FACTS_WITH_ARGUMENT = frozenset(("exact", "type_version", "shape", "keys_version", "length",
                                 "in_bounds", "is", "subtype", "not_subtype", "not_exact",
                                 "type_flag", "type_slot", "self", "method", "encoding"))
# The call-shape kinds take a value where the guard names one
FACTS_WITH_OPTIONAL_ARGUMENT = frozenset(("class_version", "func_version", "argcount",
                                          "vectorcall", "convention"))
# Properties of mutable heap state: they die at every KILLS site (1.1).
# not_exact(T) is one: a mutable class's membership can change
HEAP_FACTS = frozenset(("type_version", "has_values", "shape", "keys_version", "length",
                        "in_bounds", "subtype", "not_subtype", "not_exact", "type_flag",
                        "type_slot", "class_version", "func_version", "argcount",
                        "vectorcall", "convention", "no_managed_dict"))
# Facts of one site (refcount 1): they die at any count change of the
# value, any publication and any reentry, within the uop, and are never
# exported (F1)
SITE_FACTS = frozenset(("unique",))
# Facts of a handler's state (a STATE row's item, call_state): they live
# for the handler's run, and only a declared write of the item kills them;
# heap kills and reentry do not (item 2 of G0a4, V8's implicit operands).
# method(expanded): the callable operand is a bound method whose function
# and receiver the push takes in its place (CALL_EXPANDED)
STATE_FACTS = frozenset(("self", "method"))
STATE_ARGS = {"self": ("present", "absent"), "method": ("expanded", "whole")}
# The representation table (design 1.1; the table of words in
# pycore_value.h): each encoding of a value that is not NULL and not an
# object, the TEST row that decides it, and the exact type of its values.
# A tagged index (a FOR_ITER cursor, a saved lasti) is no Python value and
# has no type.  A type without an encoding here is an object whenever it is
# exact; the static constants (PyValue_None, PyValue_True) are objects.
ENCODINGS = {
    "inline_int": ("PyValue_IsInlineInt", "PyLong_Type"),
    "inline_float": ("PyValue_IsInlineFloat", "PyFloat_Type"),
    "tagged_index": ("PyValue_IsTaggedIndex", None),
}
# The members of an `encoding(...)` union: the encodings, an object, NULL
WORD_KINDS = (*ENCODINGS, "object", "null")
TYPE_ENCODINGS = {t: frozenset(e for e, (_, ty) in ENCODINGS.items() if ty == t)
                  for t in sorted({ty for _, ty in ENCODINGS.values() if ty})}
NUMBER_TYPES = tuple(TYPE_ENCODINGS)


@dataclass(frozen=True, order=True)
class VFact:
    "A fact of the vocabulary: kind, and its argument or \"\""
    kind: str
    arg: str = ""

    def __str__(self) -> str:
        return f"{self.kind}({self.arg})" if self.arg else self.kind

    def record(self, subject: str) -> Fact:
        arg = self.arg
        if FACT_KIND[self.kind] == "ENCODING":
            arg = "|".join(sorted(word_kinds({self})))
        elif self.kind in ("exact", "convention"):
            arg = " | ".join(arg.split("|"))
        elif self.kind == "type_slot":
            arg = " == ".join(arg.split(","))
        return Fact(FACT_KIND[self.kind], subject, arg)


NONNULL = VFact("nonnull")
NULL_FACT = VFact("null")       # the negation of nonnull
OBJECT = VFact("object")
INLINE = VFact("inline")


# The representation lattice.  What a value's word may be is one set of
# kinds of word (WORD_KINDS); every representation fact is a spelling of a
# set in it, and word_kinds() intersects them.  An ordinary value is never a
# tagged index: a value may be one only where a declaration or a row says
# so (a cursor), by naming tagged_index.
ALL_WORDS = frozenset(WORD_KINDS)
ORDINARY_WORDS = ALL_WORDS - {"tagged_index"}
INLINE_WORDS = frozenset(e for e, (_, ty) in ENCODINGS.items() if ty)
SPELLINGS = {"null": frozenset({"null"}), "object": frozenset({"object"}),
             "inline": INLINE_WORDS, **{e: frozenset({e}) for e in ENCODINGS}}
# The facts that only an object has
OBJECT_KINDS = frozenset(("type_version", "has_values", "shape", "keys_version", "length",
                          "subtype", "type_flag", "type_slot", "class_version",
                          "func_version", "ascii", "no_managed_dict", "unique"))
# excludes(S): the value's set, whatever it is, minus S: a TEST row's
# `rest` side (the false side of PyValue_IsObject is the value's kinds of
# word but object).  It stays with the value's facts, so that it applies
# to the set the value has; it is no vocabulary word and is not exported
EXCLUDES = "excludes"
REPRESENTATION = frozenset((*SPELLINGS, "nonnull", "encoding", EXCLUDES))


def _members(fact: VFact) -> frozenset[str]:
    return frozenset(fact.arg.split("|")) if fact.arg else frozenset()


def word_kinds(facts, universe: frozenset[str] | None = None) -> frozenset[str]:
    """The kinds of word a value with facts may be: the intersection of the
    sets that its facts spell, within all kinds if one names tagged_index
    and the ordinary ones if none does"""
    if universe is None:
        explicit = any("tagged_index" in (_members(f) if f.kind == "encoding" else {f.kind})
                       for f in facts)
        universe = ALL_WORDS if explicit else ORDINARY_WORDS
    found = set(universe)
    for f in facts:
        if f.kind in SPELLINGS:
            found &= SPELLINGS[f.kind]
        elif f.kind == "encoding":
            found &= _members(f)
        elif f.kind == EXCLUDES:
            found -= _members(f)
        elif f.kind == "exact":
            # exact(T): T's encodings (the representation table), or an object
            found &= {"object"}.union(*(TYPE_ENCODINGS.get(t, ())
                                        for t in f.arg.split("|")))
        elif f.kind in OBJECT_KINDS:
            found &= {"object"}
        else:
            # Any other fact (nonnull, is(K), in_bounds(x)...) is of a value
            found.discard("null")
    return frozenset(found)


def spelled(kinds: frozenset[str]) -> set[VFact]:
    "the plain spellings of a set of kinds of word: each one that contains it"
    found = set()
    if "null" not in kinds:
        found.add(NONNULL)
    elif kinds == {"null"}:
        found.add(NULL_FACT)
    if kinds and "null" not in kinds:
        found |= {VFact(name) for name, s in SPELLINGS.items() if kinds <= s}
    return found


def implied(facts) -> frozenset[VFact]:
    """facts and every fact that they imply (design.md 1.1), with their
    representation in its one form: the kinds of word they allow, spelled
    by the plain words that contain it and by encoding(...) where those do
    not say it exactly"""
    facts = set(facts)
    if any(f.kind == "shape" for f in facts):
        facts.add(VFact("has_values"))
    conventions = [f for f in facts if f.kind == "convention" and f.arg]
    if len(conventions) > 1:
        members = frozenset.intersection(*(frozenset(f.arg.split("|")) for f in conventions))
        facts -= set(conventions)
        facts.add(VFact("convention", "|".join(sorted(members))))
    kinds = word_kinds(facts)
    for e, (_, ty) in ENCODINGS.items():
        if kinds == {e} and ty:
            facts.add(VFact("exact", ty))
    result = {f for f in facts if f.kind not in REPRESENTATION or f.kind == EXCLUDES}
    result |= spelled(kinds)
    if word_kinds(result) != kinds or ("tagged_index" in kinds and kinds != {"tagged_index"}):
        result.add(VFact("encoding", "|".join(sorted(kinds))))
    return frozenset(result)


def satisfied(fact: VFact, facts) -> bool:
    "whether facts show fact; a representation fact, by a set within its own"
    if fact.kind not in REPRESENTATION:
        return fact in facts
    have = word_kinds(facts)
    return have <= word_kinds({fact}, ALL_WORDS if "tagged_index" in have else ORDINARY_WORDS)


def _type_encodings(facts) -> frozenset[str] | None:
    "The encodings that a value's exact types allow, if it has an exact type"
    types = _exact_types(facts)
    if types is None:
        return None
    return frozenset().union(*(TYPE_ENCODINGS.get(t, frozenset()) for t in types))


def without_heap_facts(facts: frozenset[VFact]) -> frozenset[VFact]:
    return frozenset(f for f in facts if f.kind not in HEAP_FACTS)


def without_site_facts(facts: frozenset[VFact]) -> frozenset[VFact]:
    return frozenset(f for f in facts if f.kind not in SITE_FACTS)


def parse_fact(word: str, where: Token | None) -> VFact:
    m = re.fullmatch(r"(\w+)(?:\(([\w|,]+)\))?", word)
    if m is None or m.group(1) not in FACT_KIND:
        message = (f"`{word}` is not a fact of the contract vocabulary "
                   f"({', '.join(sorted(FACT_KIND))})")
        raise analysis_error(message, where) if where else ValueError(message)
    kind, arg = m.group(1), m.group(2) or ""
    shape = {"exact": r"\w+(\|\w+)*", "encoding": r"\w+(\|\w+)*",
             "convention": r"\w+(\|\w+)*", "type_slot": r"\w+,\w+"}.get(kind, r"\w+")
    if arg and not re.fullmatch(shape, arg):
        message = f"`{word}`: the argument of `{kind}` is {shape}"
        raise analysis_error(message, where) if where else ValueError(message)
    if kind == "encoding" and not set(arg.split("|")) <= set(WORD_KINDS):
        message = (f"`{word}`: {', '.join(sorted(set(arg.split('|')) - set(WORD_KINDS)))} "
                   f"is no kind of word ({', '.join(WORD_KINDS)})")
        raise analysis_error(message, where) if where else ValueError(message)
    if kind == "convention" and arg:
        unknown = set(arg.split("|")) - call_conventions()
        if unknown:
            message = (f"`{word}`: {', '.join(sorted(unknown))} is no member of "
                       "_PyCallConvention (pycore_call.h)")
            raise analysis_error(message, where) if where else ValueError(message)
        arg = "|".join(sorted(set(arg.split("|"))))
    if kind in FACTS_WITH_OPTIONAL_ARGUMENT:
        pass
    elif (kind in FACTS_WITH_ARGUMENT) != bool(arg):
        message = (f"the fact `{kind}` "
                   f"{'needs an' if kind in FACTS_WITH_ARGUMENT else 'takes no'} argument")
        raise analysis_error(message, where) if where else ValueError(message)
    return VFact(kind, arg)


@dataclass(frozen=True, order=True)
class Assumption:
    """`assumed(fact) by producer` (F7): a fact that a value has from a named
    producer (the uop that pushes it, or an invariant of the runtime), which
    the verifier trusts and every execution form asserts"""
    fact: VFact
    by: str


# The C assertion of each fact an assumption may name, of a value {0}.  A
# fact without one cannot be assumed: an assumption is always checked in
# the assert build
ASSERTION_FORMS = {
    "nonnull": "!PyValue_IsNull({0})",
    "object": "PyValue_IsObject({0})",
    "inline": "PyValue_IsInline({0})",
    "inline_int": "PyValue_IsInlineInt({0})",
    "inline_float": "PyValue_IsInlineFloat({0})",
    "tagged_index": "PyValue_IsTaggedIndex({0})",
    "exact": "!PyValue_IsNull({0}) && ({1})",
    "encoding": "{1}",
}


def assertion(fact: VFact, subject: str) -> str:
    "The C assertion statement of an assumed fact of subject"
    form = ASSERTION_FORMS[fact.kind]
    if fact.kind == "encoding":
        # A union holds where one of its members' tests does
        tests = {"null": "PyValue_IsNull", "object": "PyValue_IsObject",
                 **{e: test for e, (test, _) in ENCODINGS.items()}}
        alternatives = " || ".join(f"{tests[m]}({subject})" for m in fact.arg.split("|"))
    else:
        alternatives = " || ".join(f"PyValue_TYPE({subject}) == &{t}"
                                   for t in fact.arg.split("|"))
    return f"assert({form.format(subject, alternatives)});"


def _assumptions(words: list[str], where: Token | None
                 ) -> tuple[list[str], tuple[Assumption, ...]]:
    "words without their `assumed(fact) by producer` triples, and the triples"
    rest, found, i = [], [], 0
    while i < len(words):
        m = re.fullmatch(r"assumed\((.+)\)", words[i])
        if m is None:
            rest.append(words[i])
            i += 1
            continue
        fact = parse_fact(m.group(1), where)
        if words[i + 1:i + 2] != ["by"] or not re.fullmatch(r"\w+", "".join(words[i + 2:i + 3])):
            message = f"`{words[i]}` names no producer: `{words[i]} by <producer>`"
            raise analysis_error(message, where) if where else ValueError(message)
        if fact.kind not in ASSERTION_FORMS:
            message = (f"`{words[i]}`: {fact} has no assertion form, so it cannot be "
                       f"assumed ({', '.join(ASSERTION_FORMS)})")
            raise analysis_error(message, where) if where else ValueError(message)
        found.append(Assumption(fact, words[i + 2]))
        i += 3
    return rest, tuple(found)


# ---------------------------------------------------------------------------
# The definition list (Include/internal/pycore_ir_components.h)

COMPONENTS_HEADER = ROOT / "Include/internal/pycore_ir_components.h"
ROW_KINDS = ("PRIMITIVE", "HELPER", "ROUTINE", "MACRO", "INTRINSIC", "DESTRUCTOR", "FIELD",
             "CONSTANT", "SLOT", "STATE")
RESULT_SHAPES = ("NONE", "SCALAR", "VIEW", "PATH", "MEASURE", "TEST", "CONTEXT",
                 "VALUE", "VALUE_OR_NULL", "STATUS", "GUARD", "HOME", "SLOT", "CASES",
                 "LITERAL")
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
    # `consumed object` of a `PyObject *` parameter: a stolen object
    # pointer; mode and representation are separate words
    object: bool = False


# The signs of an integer field of a CASES result
SIGNS = frozenset(("neg", "zero", "pos"))
SIGN_TESTS = {"<": {"neg"}, "<=": {"neg", "zero"}, "==": {"zero"}, "!=": {"neg", "pos"},
              ">": {"pos"}, ">=": {"zero", "pos"}}


@dataclass(frozen=True)
class Spec:
    """A value field of a CASES result in one case: its mode or null, its
    facts, and the kinds of word it may be (the facts' set, nonnull unless
    the case spells `nullable` or a union with null)"""
    mode: str                   # owned, borrowed, inline, or null
    facts: frozenset[VFact] = frozenset()
    kinds: frozenset[str] = frozenset({"null"})

    @property
    def nullable(self) -> bool:
        return "null" in self.kinds


@dataclass(frozen=True)
class Case:
    """One outcome of a CASES result: the signs of its integer fields, each
    value field's ownership and nullness, and whether an exception is set"""
    signs: tuple[tuple[str, frozenset[str]], ...]
    values: tuple[tuple[str, Spec], ...]
    raised: bool = False
    # Integer fields whose negative and positive signs are exactly -1 and 1
    exact: frozenset[str] = frozenset()


def _parse_case(text: str) -> Case:
    signs, values, raised, exact = [], [], False, set()
    for w in _split_words(text):
        if w == "raised":
            raised = True
            continue
        m = re.fullmatch(r"(\w+)(<|<=|==|!=|>|>=)(-?\d)", w)
        if m:
            field, op, value = m.groups()
            if value == "0":
                found = frozenset(SIGN_TESTS[op])
            elif op == "==" and value in ("-1", "1"):
                found = frozenset({"neg" if value == "-1" else "pos"})
                exact.add(field)
            else:
                raise ValueError(f"`{w}`: a case compares an integer field with 0, or "
                                 "states it == -1 or == 1")
            signs.append((field, found))
            continue
        m = re.fullmatch(r"(\w+)\((.*)\)", w)
        if m is None:
            raise ValueError(f"`{w}` is no field of a case: f<0 (a sign), f(mode facts) "
                             "or f(null), or `raised`")
        words = m.group(2).split()
        if not words or words[0] not in ("owned", "borrowed", "inline", "null"):
            raise ValueError(f"`{w}`: a value field is owned, borrowed, inline or null")
        nullable = "nullable" in words[1:]
        facts = frozenset(parse_fact(x, None) for x in words[1:] if x != "nullable")
        if words[0] == "null" and (facts or nullable):
            raise ValueError(f"`{w}`: a null field has no facts")
        # `nullable` spells the field without its default, nonnull; a
        # union with null (encoding(tagged_index|null)) is nullable as well
        union = any(f.kind == "encoding" and "null" in _members(f) for f in facts)
        kinds = (frozenset({"null"}) if words[0] == "null" else
                 word_kinds(facts if nullable or union else facts | {NONNULL}))
        values.append((m.group(1), Spec(words[0], facts, kinds)))
    return Case(tuple(signs), tuple(values), raised, frozenset(exact))


def _split_words(text: str) -> list[str]:
    "text split at spaces outside parentheses"
    words, depth, start = [], 0, 0
    for i, c in enumerate(text + " "):
        depth += {"(": 1, ")": -1}.get(c, 0)
        if c == " " and depth == 0:
            if text[start:i].strip():
                words.append(text[start:i].strip())
            start = i + 1
    return words


@dataclass(frozen=True)
class Result:
    shape: str
    mode: str = ""                          # VALUE, VALUE_OR_NULL
    facts: frozenset[VFact] = frozenset()   # VALUE, VALUE_OR_NULL
    measure: str = ""                       # MEASURE; PATH: the path
    tests: tuple[tuple[frozenset[VFact], frozenset[VFact]], ...] = ()  # TEST
    guards: tuple[frozenset[VFact], ...] = ()  # GUARD: per argument, on return
    reason: str = ""                        # CONTEXT
    null_iff: int = -1                      # VALUE: NULL exactly where argument k is
    moved: int = -1                         # VALUE(moved(k)): argument k's value and
                                            # obligation
    failure: str = ""                       # STATUS: ZERO, NEGATIVE or NONZERO
    # STATUS(failure, facts...): per argument, the facts that hold of it
    # where the status reports success, as a TEST's true side (desk C)
    success: tuple[frozenset[VFact], ...] = ()
    # VALUE: the facts it has from a producer (F7), also in facts; the
    # generator asserts them after each call
    assumed: tuple[Assumption, ...] = ()
    # CASES: the outcomes of a struct result, which its fields correlate
    cases: tuple[Case, ...] = ()


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
    callback_free: bool = False             # DESTRUCTOR, and the type it frees
    frees: str = ""
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
    # TAKES_VIEW(): the call takes the owner of the view that its argument
    # is (TAKE, a steal): the handler must have detached the input first
    takes_view: bool = False
    # VIEW_CLOSE(f), POINTER_FORM(g): the release of a view's owner is f
    # (the owner is NULL for a local), g for a value known to be in pointer
    # space
    view_close: str = ""
    pointer_form: str = ""
    # IDENTITY(): a two-argument TEST that holds exactly when its arguments
    # are one word (one object, or one inline value), so where it holds
    # each has the other's facts but those of one site (PyValue_Is)
    identity: bool = False
    # STATE: per bit, (bit, facts where it is set, facts where it is clear),
    # and the macro of the handler's word that initializes the item
    bits: tuple[tuple[str, frozenset, frozenset], ...] = ()
    init: str = ""
    # EXITS(branch(committed)): the exits a row takes after the generator
    # has stored the uop's outputs (IR_OBSERVE_COMMITTED_BRANCH), on which
    # they are published as on next
    committed: frozenset[str] = frozenset()
    # EXITS(jump(successor)): the exits a row names by assigning the next
    # word (JUMPTO is `next_instr = ...`): not an exit at the call; the body
    # continues, and that path's successor is published at the body's end
    # as next is
    successor: frozenset[str] = frozenset()
    # HOMES(canonical | unknown): what code that a reentrant row runs does to
    # the frame's homes.  Without it a reentrant row can vacate scratch homes
    # (a debugger's redirect) and leaves canonical values or NULL elsewhere;
    # `canonical` says it leaves initialized homes canonical-or-NULL, as a
    # finalizer or a dunder method does, `unknown` that nothing is promised
    homes: str = ""
    # OBSERVER(none): the code that a reentrant row runs opens no observer
    # (instrumentation) context of the current frame
    observer: str = ""
    # CALLS(r, ...): the declared rows that this row calls (a helper's
    # callees, a macro's expansion).  Its effects include each callee's
    # retires_frame, and its exits each callee's frame switch (VT1, VT3)
    calls: tuple[str, ...] = ()
    # CONSTANT: how the constant is held, VALUE (a PyValue, ret PyValue) or
    # OBJECT (a static object named by its address, ret its C type)
    representation: str = ""
    # CONSTANT OBJECT of a type: INSTANCES(K1, ...), the VALUE CONSTANT rows
    # that are every instance of the type, so an exact value of it is one
    # of them and owes no count (G0a5 desk D)
    instances: tuple[str, ...] = ()

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
    for shape in ("NONE", "SCALAR", "VIEW", "HOME"):
        if text == shape:
            return Result(shape)
    if (inner := _inner(text, "LITERAL")) is not None:
        # The constant the form is in its build (an INTRINSIC)
        parts = _split_text(inner)
        if len(parts) != 2 or not re.fullmatch(r"-?\d+", parts[0].strip()):
            raise ValueError('LITERAL(integer, "reason")')
        return Result("LITERAL", measure=parts[0].strip(), reason=parts[1].strip().strip('"'))
    if text == "HOME(base)":
        # The base of the frame's homes, fixed for the activation (F18)
        return Result("HOME", measure="base")
    if (inner := _inner(text, "PATH")) is not None:
        return Result("PATH", measure=inner.strip())
    if (inner := _inner(text, "CASES")) is not None:
        # A struct of a status and values (F6): per outcome, the signs of
        # its integer fields, each value's ownership and nullness, and
        # whether an exception is set
        cases = tuple(_parse_case(c) for c in _split_text(inner))
        fields = [{f for f, _ in c.signs} | {f for f, _ in c.values} for c in cases]
        if len(cases) < 2 or any(f != fields[0] for f in fields):
            raise ValueError("CASES(case, case, ...): two or more outcomes, each stating "
                             "every field")
        exact = {f for c in cases for f in c.exact}
        if any(f in exact and sign & {"neg", "pos"} and f not in c.exact
               for c in cases for f, sign in c.signs):
            raise ValueError("an exact status field is == -1, == 0 or == 1 in every case")
        return Result("CASES", cases=cases)
    if (inner := _inner(text, "SLOT")) is not None:
        # A FIELD that holds a pointer of the protocol that SLOT row names
        # (F6): a local assigned from it calls that row
        if not re.fullmatch(r"\w+", inner.strip()):
            raise ValueError("SLOT(protocol) names one SLOT row")
        return Result("SLOT", measure=inner.strip())
    if (inner := _inner(text, "STATUS")) is not None:
        failure, *entries = _split_text(inner)
        if failure.strip() not in STATUS_FAILURES:
            raise ValueError(f"STATUS({' | '.join(STATUS_FAILURES)}[, facts of each argument "
                             "on success])")
        success = tuple(frozenset(parse_fact(w, None) for w in e.split()) for e in entries)
        return Result("STATUS", failure=failure.strip(), success=success)
    if (inner := _inner(text, "MEASURE")) is not None:
        return Result("MEASURE", measure=inner.strip())
    if (inner := _inner(text, "CONTEXT")) is not None:
        return Result("CONTEXT", reason=inner.strip().strip('"'))
    if (inner := _inner(text, "GUARD")) is not None:
        # The facts of each argument on the row's normal return, after its
        # effects; its guard failure is a miss or deopt exit (F2)
        return Result("GUARD", guards=tuple(frozenset(parse_fact(w, None) for w in e.split())
                                            for e in _split_text(inner)))
    if (inner := _inner(text, "TEST")) is not None:
        tests = []
        for entry in _split_text(inner):
            # The sides split at the `|` outside a fact's parentheses
            depth, bar = 0, len(entry)
            for i, ch in enumerate(entry):
                depth += {"(": 1, ")": -1}.get(ch, 0)
                if ch == "|" and depth == 0:
                    bar = i
                    break
            true, false = entry[:bar], entry[bar + 1:]
            yes = frozenset(parse_fact(w, None) for w in true.split())
            if false.split() == ["rest"]:
                # A partition: where the test fails, the value is what it
                # was but for the kinds of word the true side spells
                no = frozenset({VFact(EXCLUDES, "|".join(sorted(word_kinds(yes, ALL_WORDS))))})
            else:
                no = frozenset(parse_fact(w, None) for w in false.split())
            tests.append((yes, no))
        return Result("TEST", tests=tuple(tests))
    for shape in ("VALUE_OR_NULL", "VALUE"):
        if (inner := _inner(text, shape)) is not None:
            words = inner.split()
            moved = re.fullmatch(r"moved\((\d+)\)", words[0]) if words else None
            if moved and shape == "VALUE_OR_NULL":
                # Argument k's obligation, or NULL where it holds none (a
                # local's view: OWNER), whatever argument k's value is
                if words[1:]:
                    raise ValueError(f"VALUE_OR_NULL(moved(k)) takes no facts: {text}")
                return Result(shape, "moved", moved=int(moved.group(1)))
            if moved and shape == "VALUE":
                # Argument k itself: its value, its identity and its
                # obligation (a move, as an output's moved(x))
                # null_iff(k) says it is NULL exactly where argument k is
                k = int(moved.group(1))
                if words[1:] not in ([], [f"null_iff({k})"]):
                    raise ValueError(f"VALUE(moved(k)) is argument k as it is: {text}")
                return Result(shape, "moved", moved=k, null_iff=k if words[1:] else -1)
            if not words or words[0] not in ("owned", "inline", "borrowed"):
                raise ValueError(f"{shape} needs a mode: {text}")
            rest, assumed = _assumptions(words[1:], None)
            if assumed and shape != "VALUE":
                raise ValueError(f"an assumed fact qualifies a VALUE result: {text}")
            null_iff, facts = -1, [a.fact for a in assumed]
            for w in rest:
                m = re.fullmatch(r"null_iff\((\d+)\)", w)
                if m and shape == "VALUE":
                    null_iff = int(m.group(1))
                else:
                    facts.append(parse_fact(w, None))
            return Result(shape, words[0], frozenset(facts), null_iff=null_iff,
                          assumed=assumed)
    raise ValueError(f"{text!r} is not a result shape ({', '.join(RESULT_SHAPES)})")


def _parse_exits(text: str) -> tuple[frozenset[str], frozenset[str], frozenset[str]]:
    """EXITS(...): the exit names; those marked `(committed)`, taken after
    the uop's outputs are stored; and those marked `(successor)`, which the
    row assigns as the next word without leaving"""
    inner = _inner(text, "EXITS")
    if inner is None:
        raise ValueError(f"expected EXITS(...), got {text.strip()!r}")
    names: set[str] = set()
    marked: dict[str, set[str]] = {"committed": set(), "successor": set()}
    for n in (n.strip() for n in inner.replace("|", ",").split(",") if n.strip()):
        m = re.fullmatch(r"(\w+)\((committed|successor)\)", n)
        if m:
            if m.group(1) in ("next", "miss", "deopt"):
                raise ValueError(f"EXITS: {m.group(1)} cannot be {m.group(2)}: next "
                                 "publishes the outputs, and a guard exit restarts the word")
            marked[m.group(2)].add(m.group(1))
            n = m.group(1)
        names.add(n)
    unknown = names - set(EXITS)
    if unknown:
        raise ValueError(f"EXITS: {sorted(unknown)} not in {EXITS}")
    return frozenset(names), frozenset(marked["committed"]), frozenset(marked["successor"])


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
    "PRIMITIVE": ("FEEDBACK", "CONJOINS", "NONNULL_FORM", "SLOW", "TAKES_VIEW", "VIEW_CLOSE",
                  "POINTER_FORM", "HOMES", "OBSERVER", "IDENTITY"),
    "HELPER": ("FEEDBACK", "SLOW", "TAKES_VIEW", "HOMES", "OBSERVER", "CALLS"),
    "ROUTINE": ("FEEDBACK", "CALLS"),
    "MACRO": ("FEEDBACK", "TAKES_VIEW", "HOMES", "OBSERVER", "CALLS"),
    "SLOT": ("FEEDBACK",),
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
        elif head == "CALLS":
            names = tuple(n.strip() for n in _split_text(inner))
            if not names or not all(re.fullmatch(r"\w+", n) for n in names):
                raise ValueError("CALLS(row, ...) names the rows that the row calls")
            found[head] = names
        elif head in ("TAKES_VIEW", "IDENTITY"):
            if inner.strip():
                raise ValueError(f"{head}() is a flag")
            found[head] = True
        elif not re.fullmatch(r"\w+", inner.strip()):
            raise ValueError(f"{head}(name) names one row")
        else:
            found[head] = inner.strip()
    return {"feedback": found.get("FEEDBACK", frozenset()),
            "conjoins": found.get("CONJOINS", ""),
            "nonnull_form": found.get("NONNULL_FORM", ""),
            "slow": found.get("SLOW", ""),
            "takes_view": found.get("TAKES_VIEW", False),
            "view_close": found.get("VIEW_CLOSE", ""),
            "pointer_form": found.get("POINTER_FORM", ""),
            "homes": found.get("HOMES", ""),
            "observer": found.get("OBSERVER", ""),
            "identity": found.get("IDENTITY", False),
            "calls": found.get("CALLS", ())}


def _parse_row(kind: str, fields: list[str]) -> Row:
    if kind == "FIELD":
        if len(fields) != 6:
            raise ValueError("FIELD(struct, field, ctype, ON(path), result, EFFECTS(...))")
        struct, name, ctype, on, result, effects = fields
        if _inner(on, "ON") is None:
            raise ValueError("expected ON(path)")
        # ON(globals | builtins): a field of the pointers along either path
        paths = "|".join(w.strip() for w in _inner(on, "ON").split("|"))
        return Row(kind, f"{struct}.{name}", ctype, result=_parse_result(result),
                   effects=_parse_names(effects, "EFFECTS", EFFECTS),
                   struct=struct, field=name, on=paths)
    if kind == "STATE":
        # A handler's state item, an implicit operand (V8's implicit register
        # operands): its C type, the facts its bits state, its initializer
        if len(fields) != 4 or _inner(fields[2], "BITS") is None \
                or _inner(fields[3], "INIT") is None:
            raise ValueError("STATE(name, ctype, BITS(BIT: set facts | clear facts, ...), "
                             "INIT(macro))")
        bits = []
        for part in _split_text(_inner(fields[2], "BITS")):
            bit, _, facts = part.partition(":")
            set_, _, clear = facts.partition("|")
            parsed = [frozenset(parse_fact(w, None) for w in _split_words(side))
                      for side in (set_, clear)]
            for f in parsed[0] | parsed[1]:
                if f.kind not in STATE_FACTS or f.arg not in STATE_ARGS[f.kind]:
                    raise ValueError(f"{f} is no fact of a handler's state "
                                     f"({', '.join(sorted(STATE_FACTS))})")
            if not re.fullmatch(r"\w+", bit.strip()):
                raise ValueError(f"`{bit.strip()}` is no bit macro")
            bits.append((bit.strip(), parsed[0], parsed[1]))
        init = _inner(fields[3], "INIT").strip()
        if not re.fullmatch(r"\w+", init):
            raise ValueError("INIT(macro): a macro of the handler's word")
        return Row(kind, fields[0].strip(), fields[1].strip(), bits=tuple(bits), init=init)
    if kind == "CONSTANT":
        # A static immortal value (V8's roots): its facts are the value's.
        # Or a static object, named by its address: `is(K)` compares a
        # value's object with it
        if len(fields) == 3 and fields[1].strip().startswith("OBJECT("):
            # A type every instance of which is a CONSTANT row's value
            instances = _inner(fields[2], "INSTANCES")
            if instances is None or not instances.strip():
                raise ValueError("CONSTANT(type, OBJECT(PyTypeObject), INSTANCES(K, ...))")
            return dataclasses.replace(_parse_row(kind, fields[:2]), instances=tuple(
                n.strip() for n in instances.split(",")))
        if len(fields) != 2:
            raise ValueError("CONSTANT(name, VALUE(inline facts) | OBJECT(type))")
        if fields[1].strip().startswith("OBJECT("):
            ctype = _inner(fields[1], "OBJECT").strip()
            if not re.fullmatch(r"\w+", ctype):
                raise ValueError("OBJECT(type): the C type of the static object")
            return Row(kind, fields[0], ctype, representation="OBJECT")
        result = _parse_result(fields[1])
        if result.shape != "VALUE" or result.mode != "inline":
            raise ValueError("a CONSTANT is a static immortal value: VALUE(inline facts)")
        return Row(kind, fields[0], "PyValue", result=result, representation="VALUE")
    if kind == "DESTRUCTOR":
        if (len(fields) != 3 or fields[2] not in ("callback_free",)
                or not re.fullmatch(r"\w+", fields[1])):
            raise ValueError("DESTRUCTOR(name, type, callback_free)")
        return Row(kind, fields[0], callback_free=True, effects=frozenset(("writes",)),
                   frees=fields[1])
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
        exits, committed, successor = _parse_exits(exits)
        return Row(kind, name, args=_parse_args(_inner(args, "ARGS")),
                   result=_parse_result(result),
                   effects=_parse_names(effects, "EFFECTS", EFFECTS),
                   exits=exits, committed=committed, successor=successor,
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
              **(dict(zip(("exits", "committed", "successor"), _parse_exits(fields[6])))
                 if kind == "ROUTINE" else {}),
              **_row_options(fields, fixed, kind))
    fixed = [p for p in row.params if p != "..."]
    if len(row.args) != len(fixed):
        raise ValueError(f"{len(row.args)} ARGS for {len(fixed)} parameters")
    # A value mode with `object` on a `PyObject *` parameter is the
    # representation: `consumed object` steals an object pointer.  On a
    # PyValue parameter `object` stays the fact the value must have
    args = []
    for arg, param in zip(row.args, fixed):
        if (arg.mode in ("consumed", "taken") and OBJECT in arg.facts
                and re.sub(r"\s+", "", param) == "PyObject*"):
            arg = Arg(arg.mode, arg.facts - {OBJECT}, object=True)
        args.append(arg)
    return dataclasses.replace(row, args=tuple(args + list(row.args[len(fixed):])))


def parse_rows(text: str, source: str = "pycore_ir_components.h") -> dict[str, Row]:
    """The rows of a definition list: every row invocation inside its
    #define lines (a list's own parameters, `(PRIMITIVE, HELPER, ...)`,
    are not rows)"""
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S).replace("\\\n", " ")
    rows: dict[str, Row] = {}
    inside = 0      # the end of the last row: a SLOT(...) result is no row
    for m in re.finditer(rf"\b({'|'.join(ROW_KINDS)})\(", text):
        if m.start() < inside:
            continue
        start = m.end() - 1
        end, depth = start, 0
        for end in range(start, len(text)):
            depth += {"(": 1, ")": -1}.get(text[end], 0)
            if depth == 0:
                break
        inside = end
        fields = [f.strip() for f in _split_text(text[start + 1:end])]
        if all(f in ROW_KINDS for f in fields):
            continue
        try:
            row = _parse_row(m.group(1), fields)
            k = row.result.null_iff
            if k >= 0 and (k >= len(row.args)
                           or row.args[k].mode not in ("borrowed", "consumed", "taken")):
                raise ValueError(f"null_iff({k}): argument {k} is no value argument")
            if row.homes not in ("", "canonical", "unknown") or row.observer not in ("", "none"):
                raise ValueError("HOMES(canonical | unknown), OBSERVER(none)")
            if (row.homes or row.observer) and not row.effects & REENTRY:
                raise ValueError("HOMES and OBSERVER describe the code a reentrant row runs: "
                                 "the row declares no reentry effect")
            if row.result.shape == "HOME" and row.effects & {"reads", "writes"}:
                raise ValueError("a HOME row addresses a frame home, which is not the heap: "
                                 "it declares no reads or writes")
            if row.result.shape == "GUARD" and not row.exits & {"miss", "deopt"}:
                raise ValueError("GUARD(...) is the result of a row with a miss or deopt "
                                 "exit")
            if row.result.shape == "GUARD" and len(row.result.guards) > len(row.args):
                raise ValueError(f"GUARD(...) states {len(row.result.guards)} arguments' "
                                 f"facts, but the row has {len(row.args)}")
            # A fact of argument k: in_bounds(k) names value argument k, any
            # other fact takes scalar argument k's value (F2)
            for f in _row_facts(row):
                if not f.arg.isdigit():
                    continue
                k = int(f.arg)
                if k >= len(row.args):
                    raise ValueError(f"{f}: the row has no argument {k}")
                if (row.args[k].mode == "scalar") == (f.kind == "in_bounds"):
                    raise ValueError(
                        f"{f}: argument {k} is {row.args[k].mode or 'a value'}; "
                        + ("in_bounds(k) names a value argument" if f.kind == "in_bounds"
                           else f"{f.kind}(k) takes a scalar argument's value"))
            k = row.result.moved
            if k >= 0 and (k >= len(row.args) or row.args[k].mode != "consumed"):
                raise ValueError(f"moved({k}): argument {k} is not consumed, so its "
                                 "obligation cannot move to the result")
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
        if len(row.result.success) > len(row.args):
            fail(row, "STATUS states facts of more arguments than ARGS has")
        for name in row.calls:
            # VT1 and VT3 through callees: each edge carries the callee's
            # retirement and frame switch, so a chain of rows carries them
            # to its first row
            callee = named(row, "CALLS", name)
            if "retires_frame" in callee.effects and "retires_frame" not in row.effects:
                fail(row, f"calls {name}, which retires a frame, and does not declare "
                     "retires_frame")
            switches = callee.exits & SWITCH_EXITS - row.exits
            if switches:
                fail(row, f"calls {name}, which switches frames by "
                     f"{', '.join(sorted(switches))}, and does not declare "
                     f"exits({', '.join(sorted(switches))})")
        for name in row.instances:
            # Each instance is a VALUE CONSTANT of exactly this type
            instance = named(row, "INSTANCES", name)
            if not (is_value_constant(instance)
                    and VFact("exact", row.name) in instance.result.facts):
                fail(row, f"INSTANCES({name}): not a VALUE CONSTANT of exact({row.name})")
        if row.identity and (row.result.shape != "TEST" or len(row.args) != 2
                             or any(a.mode != "borrowed" or a.facts for a in row.args)
                             or row.effects):
            fail(row, "IDENTITY(): a TEST of two borrowed values, without effects")
        if row.conjoins:
            # Fusion must not change failure or short-circuit behaviour: both
            # tests are total (no precondition beyond a borrowed value), have
            # no effect and touch no feedback
            one = named(row, "CONJOINS", row.conjoins)
            if (row.result.shape != "TEST" or len(row.args) != 2
                    or one.result.shape != "TEST" or len(one.args) != 1):
                fail(row, f"CONJOINS({one.name}): a two-argument TEST of a "
                     "one-argument TEST")
            if row.params != (one.params[0],) * 2 or row.ret != one.ret:
                fail(row, f"CONJOINS({one.name}): its C type is {one.name}'s with two "
                     "arguments")
            for r in (row, one):
                if r.effects or r.feedback:
                    fail(row, f"CONJOINS({one.name}): {r.name} has effects or feedback")
                if any(a.mode != "borrowed" or a.facts for a in r.args):
                    fail(row, f"CONJOINS({one.name}): {r.name} is not total: its "
                         "arguments are borrowed and require nothing")
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
            mine, theirs = row.args[0], general.args[0]
            if (mine.mode != theirs.mode or NONNULL in theirs.facts
                    or mine.facts != theirs.facts | {NONNULL}):
                fail(row, f"NONNULL_FORM({general.name}): its argument is "
                     f"{general.name}'s with nonnull added and nothing else")
            mres, gres = row.result, general.result
            if ((mres.shape, mres.mode, mres.tests, mres.measure)
                    != (gres.shape, gres.mode, gres.tests, gres.measure)
                    or not gres.facts <= mres.facts or mres.null_iff >= 0):
                fail(row, f"NONNULL_FORM({general.name}): its result is "
                     f"{general.name}'s, with at least its facts and no null_iff")
            if row.feedback != general.feedback or not row.effects <= general.effects:
                fail(row, f"NONNULL_FORM({general.name}): its feedback is "
                     f"{general.name}'s and its effects within them")
        if row.slow:
            # The slow path calls the helper with the row's arguments, and
            # the helper's failure is the row's NULL
            helper = named(row, "SLOW", row.slow)
            if row.result.shape != "VALUE_OR_NULL":
                fail(row, f"SLOW({helper.name}): a VALUE_OR_NULL result")
            if helper.kind != "HELPER" or helper.result.shape != "VALUE_OR_NULL":
                fail(row, f"SLOW({helper.name}): a HELPER with a VALUE_OR_NULL result")
            if helper.params != row.params or helper.result.mode != row.result.mode:
                fail(row, f"SLOW({helper.name}): {helper.name} takes the row's "
                     "parameters and gives its result's mode")
            if not helper.effects <= row.effects:
                fail(row, f"SLOW({helper.name}): the row declares fewer effects than "
                     f"{helper.name}")
        if row.result.shape == "SLOT":
            # A field that holds a protocol's pointer has the protocol's C type
            protocol = named(row, "SLOT", row.result.measure)
            if row.kind != "FIELD" or protocol.kind != "SLOT":
                fail(row, f"SLOT({protocol.name}): a FIELD that holds a pointer of a "
                     "SLOT row's protocol")
            if row.ret != protocol.name:
                fail(row, f"SLOT({protocol.name}): the field's C type is {row.ret}, "
                     f"not the protocol's, {protocol.name}")


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

    constants = []
    for row in table.values():
        if row.kind in ("HELPER", "ROUTINE"):
            check(f"&{row.name}", f"{row.ret} (*)({', '.join(row.params) or 'void'})",
                  row.name)
        elif row.kind == "SLOT":
            # The protocol's pointer type
            check(row.name, f"{row.ret} (*)({', '.join(row.params) or 'void'})", row.name)
        elif row.kind == "DESTRUCTOR":
            check(f"&{row.name}", "void (*)(PyObject *)", row.name)
        elif row.kind == "FIELD":
            accessor = next((a for a, prefix in FIELD_ACCESSORS.items()
                             if row.field.startswith(prefix + ".")), None)
            reader = next((r for r, f in FIELD_READERS.items() if f == row.field), None)
            if accessor is not None:
                field = row.field.split(".", 1)[1]
                check(f"{accessor}(({row.struct} *)0, {field})", row.ret, row.name)
            elif reader is not None:
                check(f"{reader}(({row.struct} *)0)", row.ret, row.name)
            else:
                check(f"(({row.struct} *)0)->{row.field}", row.ret, row.name)
        elif row.kind == "PRIMITIVE":
            call = ", ".join(f"({p}){{0}}" for p in row.params)
            check(f"{row.name}({call})", row.ret, row.name)
        elif row.kind == "CONSTANT" and row.representation == "OBJECT":
            check(f"&{row.name}", f"{row.ret} *", row.name)
        elif row.kind == "CONSTANT":
            constants.append(row.name)
        elif row.kind == "STATE":
            for bit, _, _ in row.bits:
                lines.append(f"static_assert(({bit}) != 0, \"{row.name}: {bit} is no bit\");")
    if constants:
        # A constant's compound literal holds an address, which is no constant
        # expression at file scope (GCC refuses it without optimization)
        lines.append("static inline void _PyIR_CheckConstants(void) {")
        for name in constants:
            check(name, "PyValue", name)
        lines.append("}")
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
    """V11: every PRIMITIVE row has a model in ir_leaf.py and a machine
    lowering in jit_lower.c, or is declared as another kind (HELPER for a
    function that no leaf uop calls, INTRINSIC for a form with no operation
    in the build).  The translator refuses a call it has no model of."""
    table = rows() if table is None else table
    for row in table.values():
        if row.kind != "PRIMITIVE":
            continue
        if row.name not in models:
            raise ValueError(f"{row.name}: a PRIMITIVE with no model in ir_leaf.py: model it "
                             "or declare it as a HELPER or an INTRINSIC (V11)")
        if row.name not in lowered:
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
    # The kinds of word its value may be (the representation lattice): its
    # facts', not NULL unless it is declared `nullable` or a union with null
    words: frozenset[str] = ORDINARY_WORDS - {"null"}
    array: bool = False
    size: str = ""              # an array's size, as declared
    # The facts its value has at the handler's entry from a producer (F7);
    # not requirements, and not in facts
    assumed: tuple[Assumption, ...] = ()
    # An exchange (SWAP): the pass-through input whose value this
    # pass-through output takes (check_exchange(), verify_exchange())
    exchange: str = ""
    # replaces(x): the consumed input whose stack entry this output takes
    # over, at its position, with a new value (check_replacements())
    replaces: str = ""

    @property
    def nullable(self) -> bool:
        return "null" in self.words


@dataclass(frozen=True)
class Contract:
    inputs: tuple[Item, ...]
    outputs: tuple[Item, ...]
    effects: frozenset[str]
    exits: frozenset[str]
    # The feedback the body reads and writes through its callees, beyond its
    # cache operands: (read|write, field)
    feedback: frozenset[tuple[str, str]] = frozenset()
    # state(item: required -- item: declared): the STATE items the body
    # reads, the facts it requires of each at entry and those it declares
    # at its next exit (item 2 of G0a4)
    state: tuple[tuple[str, frozenset, frozenset], ...] = ()
    # paths(path: facts): the facts the guards establish of a frame path,
    # GLOBALS() or BUILTINS(), whose subject is the path's name
    paths: tuple[tuple[str, frozenset], ...] = ()
    # effects(runs_python(branch|raise)): the effects that occur only on the
    # exits named, at calls that leave by them; they are on no path to next
    confined: tuple[tuple[str, frozenset[str]], ...] = ()

    def next_effects(self) -> frozenset[str]:
        "the effects that may occur on a path to next, which kill its facts"
        return self.effects - {e for e, _ in self.confined}

    def continuing_effects(self) -> frozenset[str]:
        """the effects that may occur on a path after which compiled code
        goes on: every effect except one confined to exits that leave the
        word's frame state (raise, exit).  An effect confined to branch
        stays: the branch's target runs after it, and the baseline JIT tests
        RETIRED after a word that may run Python"""
        return self.effects - {e for e, on in self.confined if on <= LEAVING_EXITS}

    def state_item(self, name: str) -> tuple[str, frozenset, frozenset] | None:
        return next((i for i in self.state if i[0] == name), None)

    def input(self, name: str) -> Item | None:
        return next((i for i in self.inputs if i.name == name), None)

    def output(self, name: str) -> Item | None:
        return next((o for o in self.outputs if o.name == name), None)

    def record_effects(self) -> frozenset[str]:
        return frozenset().union(*(RECORD_EFFECT[e] for e in self.continuing_effects()))


# moved(home): an output that takes the value and the obligation of the
# operand home it is loaded from, which the uop overwrites on every path to
# next (_SWAP_FAST, _LOAD_FAST_AND_CLEAR).  To the stack it is a new owner,
# mode owned with source home; a home may hold NULL, so it may be NULL
HOME_SOURCE = "home"


def _home_origin(key: str) -> str:
    "the origin of a value loaded from home `key` that the uop has not yet overwritten"
    return f"{HOME_SOURCE}({key})"


def _item(stack_item, modes: tuple[str, ...], where: Token) -> Item:
    name, through = stack_item.name, stack_item.peek
    mode, source, facts = "", "", []
    words, assumed = _assumptions(list(stack_item.contract), where)
    nullable = stack_item.nullable
    for w in words:
        m = re.fullmatch(r"moved\((\w+)\)", w)
        if m and "moved" in modes and m.group(1) == HOME_SOURCE:
            # A new owner on the stack, whose obligation was the home's (the
            # record's OWNED); a home may hold NULL, an unbound local
            mode, source = "owned", HOME_SOURCE
            nullable = True
        elif m and "moved" in modes:
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
    if mode == "moved" and through and "moved" in modes and source != HOME_SOURCE:
        # An exchange: the output keeps its home and takes the value of
        # another pass-through input (check_exchange())
        if facts or nullable:
            raise analysis_error(f"`{name}` is exchanged, which takes no facts", where)
        return Item(name, "", "", frozenset(), ORDINARY_WORDS - {"null"},
                    stack_item.is_array(), stack_item.size, assumed, exchange=source)
    if mode and through:
        raise analysis_error(f"`{name}` passes through, so it takes no mode", where)
    # `nullable` spells the item without its default, nonnull
    kinds = word_kinds(set(facts) | (set() if nullable else {NONNULL}))
    if any(f.kind == "encoding" and "null" in f.arg.split("|") for f in facts):
        kinds = word_kinds(facts)       # encoding(tagged_index|null)
    return Item(name, mode, source, frozenset(facts), kinds,
                stack_item.is_array(), stack_item.size, assumed,
                replaces=stack_item.replaces)


_CONTRACTS: dict[int, tuple[Uop, Contract | None]] = {}


def _clause_text(texts: list[str]) -> str:
    # A space after `)` separates two facts: `exact(T) func_version(v)`
    text = re.sub(r"\s*([(:|])\s*", r"\1", " ".join(texts))
    return re.sub(r"\s*\)", ")", text).replace(":", ": ")


def _named_facts(uop: Uop, part: str, where: Token) -> tuple[str, frozenset]:
    name, _, words = part.partition(":")
    name = re.sub(r"\s*\.\s*", ".", name.strip())
    if not re.fullmatch(r"\w+(\.\w+)*", name):
        raise analysis_error(f"{uop.name}: `{part.strip()}` is no `item: facts`", where)
    return name, frozenset(parse_fact(w, where) for w in _split_words(words))


def parse_state(uop: Uop, texts: list[str] | None, where: Token
                ) -> tuple[tuple[str, frozenset, frozenset], ...]:
    """`state(call_state: self(absent) -- call_state: self(present))`: each
    item names a STATE row and appears on both sides; its facts are the
    row's (a handler's state)"""
    if texts is None:
        return ()
    sides = _clause_text(texts).split("--")
    if len(sides) != 2:
        raise analysis_error(f"{uop.name}: state(required -- declared) has one `--`", where)
    required, declared = ({n: f for n, f in (_named_facts(uop, p, where)
                                             for p in _split_text(side) if p.strip())}
                          for side in sides)
    if set(required) != set(declared):
        raise analysis_error(f"{uop.name}: a state item appears on both sides of `--`", where)
    table = rows()
    items = []
    for name in required:
        row = table.get(name)
        if row is None or row.kind != "STATE":
            raise analysis_error(f"{uop.name}: state({name}) names no STATE row", where)
        for f in required[name] | declared[name]:
            if f.kind not in STATE_FACTS:
                raise analysis_error(f"{uop.name}: {f} is no fact of a handler's state "
                                     f"({', '.join(sorted(STATE_FACTS))})", where)
        items.append((name, required[name], declared[name]))
    return tuple(items)


# The kinds of path from an input that name their fields: an input's
# object, a pointer field of it, and a PyMethodDef reached from one
INPUT_PATHS = frozenset(("object", "pointer", "method_def"))

# The frame paths whose facts a uop may declare: the PATH rows without
# arguments (from the frame, which the activation does not replace)
def parse_paths(uop: Uop, texts: list[str] | None, where: Token
                ) -> tuple[tuple[str, frozenset], ...]:
    """`paths(globals: keys_version(v))`: facts of a frame path, with heap
    lifetime, whose subject in the record is the path's name"""
    if texts is None:
        return ()
    frame_paths = {r.result.measure for r in rows().values()
                   if r.result.shape == "PATH" and not r.args}
    items = []
    inputs = {i.name for i in uop.stack.inputs if i.name != "unused"}
    fields = {r.field: r for r in rows().values()
              if r.kind == "FIELD" and r.result.shape == "PATH"}
    for part in _split_text(_clause_text(texts)):
        name, facts = _named_facts(uop, part, where)
        root, *steps = name.split(".")
        if steps:
            # A path from an input through declared FIELD rows that hold
            # pointers: callable.im_func, owner.md_dict.ma_keys
            if root not in inputs:
                raise analysis_error(f"{uop.name}: paths({name}) starts at `{root}`, "
                                     "which is no input", where)
            for step in steps:
                if step not in fields:
                    raise analysis_error(f"{uop.name}: paths({name}) steps through "
                                         f"`{step}`, which no FIELD row declares as a "
                                         "pointer (PATH)", where)
        elif name not in frame_paths:
            raise analysis_error(f"{uop.name}: paths({name}) names no frame path "
                                 f"({', '.join(sorted(frame_paths))})", where)
        else:
            for f in facts:
                if f.kind not in HEAP_FACTS:
                    raise analysis_error(f"{uop.name}: {f} of the path {name} is no heap "
                                         "fact", where)
        items.append((name, facts))
    return tuple(items)


def _parse_effects(name: str, words: list[str], where: Token
                   ) -> tuple[frozenset[str], tuple[tuple[str, frozenset[str]], ...]]:
    """effects(...): the effect names, and those confined to exits, as in
    `runs_python(branch|raise)`: the effect occurs only at calls that leave
    by those exits, never on a path to next"""
    effects, confined = set(), []
    for w in words:
        m = re.fullmatch(r"(\w+)\(([\w|]+)\)", w)
        if m is None:
            effects.add(w)
            continue
        on = frozenset(m.group(2).split("|"))
        unknown = on - set(EXITS)
        if unknown or "next" in on:
            raise analysis_error(
                f"{name}: {m.group(1)}({m.group(2)}) confines an effect to "
                + (f"{', '.join(sorted(unknown))}, which is no exit" if unknown else
                   "next, where every unconfined effect already lies"), where)
        effects.add(m.group(1))
        confined.append((m.group(1), on))
    return frozenset(effects), tuple(sorted(confined, key=lambda c: c[0]))


def _homes(items, stack_items) -> dict[str, tuple[str, ...]]:
    "each item's home: the sizes of the stack items below it"
    return {item.name: tuple(s.size or "1" for s in stack_items[:k])
            for k, item in enumerate(items)}


def check_replacements(uop: Uop, inputs: tuple[Item, ...], outputs: tuple[Item, ...],
                       where: Token) -> None:
    """A replacement (F16's cursor): the input whose entry an output takes
    over is consumed, so that its value is the body's to retire.  The
    analyzer checked the position (analyzer.analyze_stack)"""
    for o in outputs:
        if not o.replaces:
            continue
        item = next(i for i in inputs if i.name == o.replaces)
        if item.mode != "consumed":
            raise analysis_error(f"{uop.name}: `{o.name}` replaces `{item.name}`, which is "
                                 f"{item.mode or 'a pass-through'}, not consumed", where)


def check_exchange(uop: Uop, inputs: tuple[Item, ...], outputs: tuple[Item, ...],
                   where: Token) -> None:
    """An exchange (SWAP): pass-through outputs that take `moved(x)` of
    pass-through inputs.  Each item in it keeps its home, and the outputs
    are a permutation of the inputs, with no fixed point; no other output
    moves from an exchanged input"""
    exchanged = [o for o in outputs if o.exchange]
    if not exchanged:
        return
    through = {i.name for i in inputs if not i.mode and i.name != "unused"}
    for o in outputs:
        if o.mode == "moved" and o.source in {e.exchange for e in exchanged}:
            raise analysis_error(f"{uop.name}: `{o.name}` is no pass-through, and moves "
                                 f"from `{o.source}`, which is exchanged", where)
    for o in exchanged:
        if o.exchange not in through:
            raise analysis_error(f"{uop.name}: `{o.name}` takes `{o.exchange}`, which is "
                                 "no pass-through input", where)
        if o.exchange == o.name:
            raise analysis_error(f"{uop.name}: `{o.name}` is exchanged with itself", where)
    names, sources = {o.name for o in exchanged}, [o.exchange for o in exchanged]
    if set(sources) != names or len(set(sources)) != len(sources):
        raise analysis_error(f"{uop.name}: the exchange of {', '.join(sorted(names))} "
                             f"takes {', '.join(sorted(sources))}: no permutation", where)
    homes_in = _homes(inputs, uop.stack.inputs)
    homes_out = _homes(outputs, uop.stack.outputs)
    for name in sorted(names):
        if homes_in[name] != homes_out[name]:
            raise analysis_error(f"{uop.name}: `{name}` is exchanged, and its home "
                                 "changes", where)


def verify_exchange(uop: Uop, contract: Contract) -> None:
    """The body of an exchange does exactly its permutation: copies between
    the exchanged names and PyValue temporaries, nothing else, and no call;
    at its end each exchanged output holds the input it declares"""
    exchanged = {o.name: o.exchange for o in contract.outputs if o.exchange}
    if contract.effects or contract.exits != {"next"}:
        raise analysis_error(f"{uop.name}: an exchange has effects() exits(next)",
                             uop.body.open)
    values = {name: name for name in exchanged}
    for stmt in uop.body.body:
        text = [t.text for t in stmt.tokens() if t.kind != "COMMENT"]
        if text[:1] == ["PyValue"]:
            text = text[1:]
            if len(text) == 4 and text[0] not in values:
                values[text[0]] = None
        if (len(text) != 4 or text[1] != "=" or text[3] != ";"
                or text[0] not in values or text[2] not in values
                or values[text[2]] is None):
            raise analysis_error(f"{uop.name}: an exchange's body only copies between "
                                 f"{', '.join(sorted(exchanged))} and temporaries: "
                                 f"`{' '.join(t.text for t in stmt.tokens())}`",
                                 next(iter(stmt.tokens()), uop.body.open))
        values[text[0]] = values[text[2]]
    for name, source in sorted(exchanged.items()):
        if values[name] != source:
            raise analysis_error(f"{uop.name}: the body leaves `{name}` with "
                                 f"`{values[name]}`, and it declares moved({source})",
                                 uop.body.close)


def declared_contract(uop: Uop) -> Contract:
    """uop's contract.  Every uop of the IR declares one: an undeclared uop
    is an error, not a body to infer from"""
    contract = contract_of(uop)
    if contract is None:
        raise analysis_error(f"{uop.name} declares no contract: give it effects(...) "
                             "and exits(...)", uop.body.open)
    return contract


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
        effects, confined = _parse_effects(uop.name, uop.clauses["effects"], where)
        exits = frozenset(uop.clauses["exits"])
        for e, on in confined:
            if not on <= exits:
                raise analysis_error(f"{uop.name}: {e} is confined to "
                                     f"{', '.join(sorted(on - exits))}, which the uop does "
                                     "not declare as exits", where)
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
            if o.assumed:
                raise analysis_error(f"{uop.name}: the output `{o.name}` is assumed; an "
                                     "assumption is of a value that reaches the handler "
                                     "(F7), and an output's facts are shown", where)
        check_exchange(uop, inputs, outputs, where)
        check_replacements(uop, inputs, outputs, where)
        feedback = parse_feedback(uop.clauses.get("feedback", ()), where)
        state = parse_state(uop, uop.clauses.get("state"), where)
        paths = parse_paths(uop, uop.clauses.get("paths"), where)
        contract = Contract(inputs, outputs, effects, exits, feedback, state, paths,
                            confined)
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
# The constructs among them that the leaf IR drops: the stack effects they
# state are the declaration's, and the statistics and assertions are not
# part of a release build's operation
ERASED_CONSTRUCTS = frozenset(("DEAD", "INPUTS_DEAD", "STAT_INC", "assert"))
CHECKED_ARITHMETIC = re.compile(r"__builtin_(add|sub|mul)_overflow")
GUARDS = {"EXIT_IF": "miss", "DEOPT_IF": "deopt"}
RAISES = ("ERROR_IF", "ERROR_NO_POP")
# The labels a body may goto: its raise exit at that site, as ERROR_NO_POP.
# What the error routine does after (its jump or exit) is the boundary of
# its ROUTINE row, not the uop's.  The pop_N_error labels close stack
# items the walk does not model, and are refused with every other goto.
ERROR_LABELS = ("error", "exception_unwind")


# The macros of the scoped GIL product that select preprocessor arms (F15):
# Py_GIL_DISABLED is not defined in a build with the GIL; pycore_code.h
# defines ENABLE_SPECIALIZATION as 1 in every build; every build of the
# product is configured --with-shapes, which defines Py_SHAPES as 1.
BUILD_MACROS: dict[str, str | None] = {"Py_GIL_DISABLED": None, "ENABLE_SPECIALIZATION": "1",
                                       "Py_SHAPES": "1"}
# The macros that differ between the product's supported builds: the walk
# takes both arms as alternative paths and joins them.  Any other macro is
# decided by no build, and its arms may hold no code.
ALTERNATIVE_MACROS = frozenset(("Py_DEBUG",))


def _directive(directive: str) -> tuple[str, str, bool] | None:
    """`#ifdef M`, `#ifndef M`, `#if [!]defined(M)` or `#if [!]M`: (M, how
    it is tested, negated), or None"""
    text = directive.strip()[1:].strip()
    m = re.fullmatch(r"(ifdef|ifndef)\s+(\w+)(\s*(/\*.*\*/|//.*))?", text)
    if m:
        return m.group(2), "defined", m.group(1) == "ifndef"
    m = re.fullmatch(r"if\s+(!?)\s*(?:defined\s*\(\s*(\w+)\s*\)|(\w+))(\s*(/\*.*\*/|//.*))?",
                     text)
    if m:
        return m.group(2) or m.group(3), "defined" if m.group(2) else "value", bool(m.group(1))
    return None


def macro_condition(directive: str) -> bool | None:
    """Whether the arm after `#if`, `#ifdef` or `#ifndef` is the GIL
    product's, or None if its macros do not decide it"""
    found = _directive(directive)
    if found is None or found[0] not in BUILD_MACROS:
        return None
    name, test, negated = found
    value = BUILD_MACROS[name]
    holds = value is not None if test == "defined" else value not in (None, "0")
    return holds != negated


def alternative_arms(directive: str) -> bool:
    "Whether both arms of the directive are paths of some supported build"
    found = _directive(directive)
    return found is not None and found[0] in ALTERNATIVE_MACROS


def _macro_ifs(uop: Uop) -> list[MacroIfStmt]:
    found: list[MacroIfStmt] = []
    uop.body.accept(lambda stmt: found.append(stmt) if isinstance(stmt, MacroIfStmt) else None)
    return found


def correlated_types(table: dict[str, Row] | None = None) -> frozenset[str]:
    "The C struct types that rows return as CASES results"
    table = rows() if table is None else table
    return frozenset(r.ret for r in table.values() if r.result.shape == "CASES")


def struct_locals(tokens: list[Token], types: frozenset[str]) -> frozenset[str]:
    "The locals declared `T x` with T a CASES result's struct type"
    texts = [t.text for t in tokens]
    return frozenset(texts[i + 1] for i, t in enumerate(texts[:-2])
                     if t in types and re.fullmatch(r"[A-Za-z_]\w*", texts[i + 1]))


# Typed accessors of untyped struct fields (F6): `_PyType_VALUE_SLOT(t, f)`
# reads `t->tp_value.f`, stored as an untyped _Py_valuefunc, as the type
# that the slot table gives f.  The walk reads the accessor as the field,
# and component_checks holds a FIELD row of `tp_value.f` to the accessor's
# type rather than the storage's.
FIELD_ACCESSORS = {"_PyType_VALUE_SLOT": "tp_value"}
# Readers of a field that the struct does not store (item 2 of G0a5):
# `_PyMethodDef_Convention(m)` reads `m->convention`, the masked flags.
# component_checks holds the FIELD row to the reader's type
FIELD_READERS = {"_PyMethodDef_Convention": "convention"}


def call_conventions() -> frozenset[str]:
    "The members of _PyCallConvention (Include/internal/pycore_call.h)"
    if not _CONVENTIONS:
        text = (ROOT / "Include/internal/pycore_call.h").read_text()
        body = re.search(r"typedef enum \{(.*?)\} _PyCallConvention;", text, re.S)
        _CONVENTIONS.update(re.findall(r"^\s*(_Py_CALL_\w+)\s*=", body.group(1), re.M))
    return frozenset(_CONVENTIONS)


_CONVENTIONS: set[str] = set()


def fold_fields(tokens: list[Token], structs: frozenset[str]) -> list[Token]:
    """tokens with each field of a struct local, `x . f`, as one name
    `x.f`: the walk tracks a CASES result's fields as names (F6).  A typed
    accessor of a field, `_PyType_VALUE_SLOT(t, f)`, becomes the read
    `(t)->tp_value.f` (FIELD_ACCESSORS)"""
    if not structs and not any(t.text in FIELD_ACCESSORS or t.text in FIELD_READERS
                               for t in tokens):
        return tokens
    out: list[Token] = []
    i = 0
    while i < len(tokens):
        t = tokens[i]
        if t.text in FIELD_READERS and i + 1 < len(tokens) and tokens[i + 1].text == "(":
            def synth(text: str, at: Token) -> Token:
                return Token(at.filename, at.kind, text, at.begin, at.end)
            end = _close([x.text for x in tokens], i + 1)
            out += [synth("(", tokens[i + 1]), *fold_fields(tokens[i + 2:end], structs),
                    synth(")", tokens[end]), synth("->", t),
                    synth(FIELD_READERS[t.text], t)]
            i = end + 1
            continue
        if t.text in FIELD_ACCESSORS and i + 1 < len(tokens) and tokens[i + 1].text == "(":
            texts = [x.text for x in tokens]
            end = _close(texts, i + 1)
            comma = len(_split(texts[i + 2:end], ",")[0]) + i + 2
            if end == comma + 2:
                def synth(text: str, at: Token) -> Token:
                    return Token(at.filename, at.kind, text, at.begin, at.end)
                field = tokens[comma + 1]
                out += [synth("(", tokens[i + 1]),
                        *fold_fields(tokens[i + 2:comma], structs),
                        synth(")", tokens[end]), synth("->", t),
                        synth(f"{FIELD_ACCESSORS[t.text]}.{field.text}", field)]
                i = end + 1
                continue
        if (t.text in structs and i + 2 < len(tokens) and tokens[i + 1].text == "."
                and (not out or out[-1].text not in (".", "->"))):
            out.append(Token(t.filename, t.kind, f"{t.text}.{tokens[i + 2].text}",
                             t.begin, tokens[i + 2].end))
            i += 3
            continue
        out.append(t)
        i += 1
    return out


def _tokens(uop: Uop) -> list[Token]:
    """The body's tokens on the GIL product's preprocessor path: an arm
    that the build's macros exclude is not code (F15).  An arm they do not
    decide stays, and verify_uop refuses it where it is code.  A CASES
    result's fields are single names (fold_fields)"""
    excluded: set[int] = set()
    for stmt in _macro_ifs(uop):
        excluded.add(id(stmt.condition))
        holds = macro_condition(stmt.condition.text)
        if holds is not None:
            for other in (stmt.else_body or []) if holds else stmt.body:
                excluded |= {id(t) for t in other.tokens()}
    tokens = [t for t in uop.body.tokens() if t.kind != "COMMENT" and id(t) not in excluded]
    return fold_fields(tokens, struct_locals(tokens, correlated_types()))


def check_preprocessor(uop: Uop) -> None:
    """F15: an arm whose condition the GIL product's macros do not decide,
    nor a macro of alternative builds (Py_DEBUG), may hold no code beyond
    assertions; `#elif`, which the parser does not
    model, is refused"""
    for t in uop.body.tokens():
        if t.kind == "CMACRO_OTHER" and t.text.strip()[1:].strip().startswith("elif"):
            raise analysis_error(f"{uop.name}: `#elif` is a preprocessor form the walk does "
                                 "not model; nest #if and #else", t)
    for stmt in _macro_ifs(uop):
        if (macro_condition(stmt.condition.text) is not None
                or alternative_arms(stmt.condition.text)):
            continue
        tokens = [t for arm in (stmt.body, stmt.else_body or []) for s_ in arm
                  for t in s_.tokens() if t.kind != "COMMENT"]
        texts = [t.text for t in tokens]
        i = 0
        while i < len(tokens):
            if texts[i] == "assert" and texts[i + 1:i + 2] == ["("]:
                i = _close(texts, i + 1) + 1
                continue
            if texts[i] != ";":
                raise analysis_error(
                    f"{uop.name}: `{stmt.condition.text.strip()}` is decided by no macro of "
                    f"the GIL product ({', '.join(sorted(BUILD_MACROS))}), and its arm "
                    "holds code", tokens[i])
            i += 1


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
    table = with_slots(uop, rows() if table is None else table)
    contract = declared_contract(uop)
    inline = {i.name for i in contract.inputs if INLINE in implied(i.facts)}
    # A store into an output array publishes; it writes nothing else
    arrays = {o.name for o in contract.outputs if o.array}
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
            elif text == "DECREF_INPUTS":
                if any(i.mode in ("consumed", "taken") and i.name not in inline
                       for i in contract.inputs):
                    sites.append(Site(t, "releases", "DECREF_INPUTS of an owned input"))
        i += 1
    p = uop.properties
    if p.eval_breaker:
        sites.append(Site(uop.body.open, "polls", "the uop polls the eval breaker"))
    if analyzer.assigns_frame(list(uop.body.tokens())) and not any(
            s.effect == "switches_frame" for s in sites):
        # A uop whose frame switch no row names switches at its start; one
        # that a row names (IR_ENTER_GENERATOR) switches there, after the
        # guards before it
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


def ends_path(row: Row) -> bool:
    """whether a call of row never returns to the body: a terminal marker,
    or a row whose exits are committed and none is next"""
    return ("terminal" in IR_MARKER_EFFECTS.get(row.name, ())
            or bool(row.committed) and "next" not in row.exits
            or row.exits == {"abort"})


def check_effects(uop: Uop, contract: Contract, table: dict[str, Row]) -> None:
    """V2: the body's effects lie within the declared ones; one declared
    only on some exits occurs at a call that leaves by them"""
    confined = dict(contract.confined)
    free = contract.next_effects()
    for site in effect_sites(uop, table):
        if not covered(site.effect, contract.effects):
            raise analysis_error(
                f"{uop.name}: `{site.token.text}` may {site.effect} ({site.why}), which "
                f"effects({', '.join(sorted(contract.effects))}) does not declare",
                site.token)
        if covered(site.effect, free):
            continue
        on = frozenset().union(*(x for e, x in confined.items()
                                 if covered(site.effect, frozenset((e,)))))
        row = table.get(site.token.text)
        leaves = (frozenset({"raise"}) if site.token.text in RAISES
                  else row.exits if row is not None and ends_path(row) else None)
        if leaves is None or not leaves <= on:
            raise analysis_error(
                f"{uop.name}: `{site.token.text}` may {site.effect} ({site.why}), which "
                f"the uop declares only on {', '.join(sorted(on))}, but "
                + ("it does not leave the uop" if leaves is None else
                   f"it may leave by {', '.join(sorted(leaves - on))}"),
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


# The rows that give the address of the site's feedback
FEEDBACK_POINTERS = ("FB_COUNTER",)


def check_feedback_pointers(uop: Uop, table: dict[str, Row]) -> None:
    """A body reaches feedback only through rows, so that the clause's union
    over its callees is complete: a feedback pointer (FB_COUNTER(), or a
    local holding one) is never dereferenced in the body, and is passed only
    to rows that declare FEEDBACK"""
    tokens = _tokens(uop)
    texts = [t.text for t in tokens]
    pointers: set[str] = set()
    for i, t in enumerate(texts):
        if t == "=" and i > 0:
            end = texts.index(";", i) if ";" in texts[i:] else len(texts)
            # The value that a FEEDBACK row reads from the feedback is no
            # pointer into it
            call = _call_of(_uncast(texts[i + 1:end]), table)
            if (call is not None and call[0].feedback
                    and call[0].result.shape in ("VALUE", "VALUE_OR_NULL")):
                continue
            if any(x in FEEDBACK_POINTERS or x in pointers for x in texts[i + 1:end]):
                pointers.add(texts[i - 1])

    def fail(tkn: Token, what: str):
        raise analysis_error(f"{uop.name}: {what}; a body reaches feedback only through "
                             "rows that declare FEEDBACK", tkn)

    for i, t in enumerate(texts):
        after = i + 1
        if t in FEEDBACK_POINTERS and texts[i + 1:i + 2] == ["("]:
            after = _close(texts, i + 1) + 1
        elif t not in pointers:
            continue
        if texts[after:after + 1] in (["->"], ["["], ["."]):
            fail(tokens[i], f"dereferences the feedback pointer `{t}`")
        if (i > 0 and texts[i - 1] == "*"
                and (i < 2 or not re.fullmatch(r"\w+", texts[i - 2]))):
            fail(tokens[i], f"dereferences the feedback pointer `{t}`")
    for tkn, args in body_calls(uop):
        row = table.get(tkn.text)
        if tkn.text in FEEDBACK_POINTERS or row is None or row.feedback:
            continue
        for arg in args:
            if any(x in FEEDBACK_POINTERS or x in pointers for x in arg):
                fail(tkn, f"passes a feedback pointer to {tkn.text}, which declares no "
                     "FEEDBACK")


def feedback_access(uop: Uop) -> frozenset[tuple[str, str]]:
    """What a declared uop reads and writes of its site's feedback: its
    clause, and a read of the field of each cache operand"""
    contract = declared_contract(uop)
    caches = {("read", c.name) for c in uop.caches if c.name != "unused"}
    return frozenset(caches | contract.feedback)


def record_effect_sites(uop: Uop) -> list[tuple[Token, str]]:
    "effect_sites() in ir_records' terms, for its guard-order check"
    return [(s.token, SITE_EFFECT[s.effect]) for s in effect_sites(uop)]


# ---------------------------------------------------------------------------
# Guard facts (V9)

def _unwrap(tokens: list[str], table: dict[str, Row]) -> list[str]:
    "tokens without casts and the INTRINSIC identity forms around them"
    tokens = _strip(_uncast(tokens))
    while (tokens and tokens[0] in table and table[tokens[0]].kind == "INTRINSIC"
           and table[tokens[0]].result.shape in ("SCALAR", "VIEW")):
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
        # A cache operand roots paths too (a feedback descriptor's fields)
        self.caches = {c.name for c in uop.caches if c.name != "unused"}
        self.paths: dict[str, tuple[str, str]] = {}
        # Locals bound once to a MEASURE field read, `conv = m->convention`:
        # the read, which a comparison of the local states facts of
        self.measures: dict[str, list[str]] = {}
        self.locals: set[str] = set()
        # Locals whose binding at a use the walk could not follow (they were
        # bound differently on paths that join): they stand for nothing
        self.unknown: set[str] = set()
        texts = [t.text for t in _tokens(uop)]
        assigned = [texts[i - 1] for i, t in enumerate(texts) if t == "=" and i > 0]
        # A local bound more than once is a view only where the walk follows
        # its binding (OwnershipWalk.views()); statically it stands for itself
        rebound = {n for n in assigned if assigned.count(n) > 1}
        for i, t in enumerate(texts):
            if t != "=" or i == 0 or texts[i - 1] in self.names:
                continue
            self.locals.add(texts[i - 1])
            if texts[i - 1] in rebound:
                continue
            end = texts.index(";", i) if ";" in texts[i:] else len(texts)
            path = self.path(texts[i + 1:end])
            if path is not None:
                self.paths.setdefault(texts[i - 1], path)
                continue
            read = self.field(texts[i + 1:end])
            if read is not None and read[0].result.shape == "MEASURE":
                self.measures[texts[i - 1]] = texts[i + 1:end]
                continue
            inner = self.resolve(texts[i + 1:end], locals_=False)
            if inner is not None:
                self.names.setdefault(texts[i - 1], inner)

    def resolve(self, tokens: list[str], locals_: bool = True) -> str | None:
        "the input (or, if locals_, the local) that tokens stand for"
        tokens = _unwrap(tokens, self.table)
        if len(tokens) == 1:
            if tokens[0] in self.unknown:
                return None
            if tokens[0] in self.names:
                return self.names[tokens[0]]
            path = self.paths.get(tokens[0])
            if path is not None and path[1] in INPUT_PATHS:
                # A pointer along a path from an input: the path itself
                # (callable.im_func), whose facts paths(...) declares
                return path[0]
            return tokens[0] if locals_ and tokens[0] in self.locals else None
        call = _call_of(tokens, self.table)
        if call is not None and call[0].result.shape == "VIEW" and len(call[1]) == 1:
            return self.resolve(call[1][0], locals_)
        return None

    def path(self, tokens: list[str]) -> tuple[str, str] | None:
        """the (input, path) of a pointer along a PATH row from an input or
        a cache operand"""
        tokens = _unwrap(tokens, self.table)
        if len(tokens) == 1:
            return None if tokens[0] in self.unknown else self.paths.get(tokens[0])
        if len(tokens) >= 3 and tokens[-2] == "->":
            # A field that holds a pointer along a further path
            # (globals->ma_keys); from an input, the path names each field
            # it steps through (callable.im_func, owner.md_dict.ma_keys)
            found = self.field(tokens)
            if found is not None and found[0].result.shape == "PATH":
                if found[2]:
                    return f"{found[1]}.{tokens[-1]}", found[0].result.measure
                return found[1], found[0].result.measure
            return None
        call = _call_of(tokens, self.table)
        if call is not None and call[0].result.shape == "PATH" and not call[0].args:
            # A frame path (GLOBALS()): its subject is the path's name
            return call[0].result.measure, call[0].result.measure
        if call is not None and call[0].result.shape == "PATH" and call[1]:
            arg = _unwrap(call[1][0], self.table)
            name = (arg[0] if len(arg) == 1 and arg[0] in self.caches
                    else self.resolve(call[1][0], locals_=False))
            if name is not None:
                return name, call[0].result.measure
        return None

    def field(self, tokens: list[str]) -> tuple[Row, str, bool] | None:
        """the FIELD row of a read `p->field` through a pointer p along a
        path from an input, that input (or the path from it), and whether
        the read steps along an input path (item 1 of G0a5): p is the
        object of an input, or a pointer along such a path"""
        tokens = _unwrap(tokens, self.table)
        if len(tokens) == 1 and tokens[0] in self.measures and tokens[0] not in self.unknown:
            return self.field(self.measures[tokens[0]])
        if len(tokens) < 3 or tokens[-2] != "->":
            return None
        path = self.path(tokens[:-2])
        if path is None:
            base = self.resolve(tokens[:-2], locals_=False)
            if base is None:
                return None
            path = (base, "object")
        row = next((r for r in self.table.values() if r.kind == "FIELD"
                    and r.field == tokens[-1] and path[1] in r.on.split("|")), None)
        return (row, path[0], path[1] in INPUT_PATHS) if row is not None else None


_SLOTS: dict[tuple[int, int], tuple[Uop, dict, dict[str, Row], list]] = {}


def _slot_bindings(uop: Uop, table: dict[str, Row]) -> tuple[dict[str, Row], list]:
    """table, with each local of uop that it calls bound to the SLOT row of
    the protocol it holds (F6): every assignment to the local reads a FIELD
    that declares SLOT(protocol), without a cast.  Also the refusals, as
    (token, message), of the locals it calls that bind to no protocol"""
    key = (id(uop), id(table))
    if key in _SLOTS and _SLOTS[key][0] is uop and _SLOTS[key][1] is table:
        return _SLOTS[key][2], _SLOTS[key][3]
    views = Views(uop, table)
    bound: dict[str, Row] = {}
    refused: list[tuple[Token, str]] = []
    for tkn, _ in body_calls(uop):
        name = tkn.text
        if name in table or name in DSL_CONSTRUCTS or name in bound:
            continue
        assignments = _assignments(uop, name)
        if not assignments:
            continue        # no local: V1 refuses the call
        protocols: set[str] = set()
        for where, rhs in assignments:
            found = views.field(rhs)
            if _uncast(rhs) != _strip(rhs):
                refused.append((where, f"`{name}` is called through a cast, "
                                f"`{' '.join(rhs)}`, which binds it to no protocol"))
            elif found is None or found[0].result.shape != "SLOT":
                refused.append((where, f"`{name}` is called, but `{' '.join(rhs)}` is no "
                                "FIELD that declares the SLOT it holds"))
            else:
                protocols.add(found[0].result.measure)
                continue
            break
        else:
            if len(protocols) == 1:
                bound[name] = table[protocols.pop()]
            else:
                refused.append((tkn, f"`{name}` holds pointers of "
                                f"{', '.join(sorted(protocols))}"))
    result = table | bound if bound else table
    _SLOTS[key] = (uop, table, result, refused)
    return result, refused


def with_slots(uop: Uop, table: dict[str, Row]) -> dict[str, Row]:
    "table with uop's locals bound to the protocols they hold (F6)"
    return _slot_bindings(uop, table)[0]


def check_slots(uop: Uop, table: dict[str, Row]) -> None:
    "F6: a local that the body calls holds a pointer of one declared protocol"
    for where, message in _slot_bindings(uop, table)[1]:
        raise analysis_error(f"{uop.name}: {message}", where)


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


def _exact_types(facts: frozenset[VFact]) -> frozenset[str] | None:
    "The exact types a name may have, if its facts say"
    found = [frozenset(f.arg.split("|")) for f in facts if f.kind == "exact"]
    return frozenset.intersection(*found) if found else None


def _intersection(found: list[Facts]) -> Facts:
    """The facts that hold on every alternative.  A conjunction of facts
    joins by intersection; the exact type joins by the union of the
    alternatives' types (F1)"""
    names = set.intersection(*(set(f) for f in found)) if found else set()
    result = {}
    for n in names:
        common = frozenset.intersection(*(implied(f[n]) for f in found))
        types = [_exact_types(implied(f[n])) for f in found]
        if all(t is not None for t in types) and not any(f.kind == "exact" for f in common):
            union = frozenset.union(*types)
            common = implied(common | {VFact("exact", "|".join(sorted(union)))})
        # A calling convention joins by the union of the members, as the
        # exact type does: `conv != A && conv != B` failing is A or B
        members = [[f.arg for f in implied(f[n]) if f.kind == "convention" and f.arg]
                   for f in found]
        if all(members) and not any(f.kind == "convention" for f in common):
            union = {m for ms in members for a in ms for m in a.split("|")}
            common = implied(common | {VFact("convention", "|".join(sorted(union)))})
        # The kinds of word join by their union: NULL on one path and a
        # tagged index on another is encoding(null|tagged_index)
        union = frozenset.union(*(word_kinds(f[n]) for f in found))
        if word_kinds(common) != union:
            common = implied(common | {VFact("encoding", "|".join(sorted(union)))})
        result[n] = common
    return result


def _bind_facts(facts: frozenset[VFact], args: list[list[str]],
                views: Views) -> frozenset[VFact] | None:
    """A row's facts with an argument k bound: in_bounds(k) to the name of
    value argument k; any other fact's k to the scalar argument k, a name
    (type_version(1), shape(2), exact(1) of a type).  None if that
    argument names nothing"""
    bound = set()
    for f in facts:
        if f.arg.isdigit():
            k = int(f.arg)
            if f.kind == "in_bounds":
                other = views.resolve(args[k]) if k < len(args) else None
            else:
                arg = _uncast(_strip(args[k])) if k < len(args) else []
                other = arg[0] if len(arg) == 1 and re.fullmatch(r"\w+", arg[0]) else None
            if other is None:
                return None
            f = VFact(f.kind, other)
        bound.add(f)
    return frozenset(bound)


def _row_facts(row: Row):
    "the facts that a row's GUARD and TEST results state of its arguments"
    for facts in row.result.guards:
        yield from facts
    for sides in row.result.tests:
        for side in sides:
            yield from side


def state_bit(cond: list[str], table: dict[str, Row]
              ) -> tuple[str, frozenset, frozenset] | None:
    """`call_state & CALL_SELF_PRESENT`: a STATE item's bit, with the facts
    where it is set and where it is clear"""
    if len(cond) != 3 or cond[1] != "&":
        return None
    row = table.get(cond[0])
    if row is None or row.kind != "STATE":
        return None
    found = next((b for b in row.bits if b[0] == cond[2]), None)
    return (row.name, found[1], found[2]) if found else None


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
    bit = state_bit(cond, table)
    if bit is not None:
        name, set_, clear = bit
        return {name: set_ if holds else clear}, ""
    field = views.field(cond)
    if field is not None and field[0].result.shape == "TEST":
        true, false = field[0].result.tests[0]
        found = true if holds else false
        return ({field[1]: implied(found)} if found else {}), ""
    call = _call_of(cond, table)
    if call is not None and call[0].result.shape == "TEST":
        row, args = call
        facts: dict[str, frozenset[VFact]] = {}
        for spec, arg, (true, false) in zip(row.args, args, row.result.tests):
            found = true if holds else false
            if not found:
                continue
            if any(f.kind == EXCLUDES for f in found):
                # The rest of what the argument may be, which the row
                # requires of it
                found = found | spec.facts
            name = views.resolve(arg)
            if name is None:
                return None
            # A row's in_bounds(k) names its argument k: here, its name
            found = _bind_facts(found, args, views)
            if found is None:
                return None
            facts[name] = implied(facts.get(name, frozenset()) | found)
        return facts, ""
    if call is not None and call[0].result.shape in ("CONTEXT", "LITERAL"):
        return {}, call[0].result.reason
    for op in ("!=", "=="):
        sides = _split(cond, op)
        if len(sides) == 2:
            break
    else:
        return None
    other = _unwrap(sides[1], table)
    typed = _call_of(_unwrap(sides[0], table), table)
    if other[:1] == ["&"] and len(other) == 2 and re.fullmatch(r"\w+_Type", other[1]) \
            and typed is not None and typed[0].result.shape == "PATH" \
            and typed[0].result.measure == "type" and len(typed[1]) == 1:
        # `Py_TYPE(x) != &T`: x's type is exactly T where it is equal
        name = views.resolve(typed[1][0])
        if name is None:
            return None
        return ({name: implied({VFact("exact", other[1])})} if (op == "==") == holds
                else {}), ""
    if other[:1] == ["&"] and len(other) == 2 and table.get(other[1]) is not None \
            and table[other[1]].representation == "OBJECT":
        # `x == &K`: x's object is the static object K, is(K)
        name = views.resolve(_unwrap(sides[0], table))
        if name is None:
            return None
        return ({name: implied({VFact("is", other[1])})} if (op == "==") == holds
                else {}), ""
    if _strip(sides[1]) == ["NULL"]:
        # A pointer along a path from an input, `m->im_weakreflist != NULL`
        path = views.path(sides[0])
        if path is not None and path[1] in INPUT_PATHS:
            null = (op == "==") == holds
            return {path[0]: frozenset({NULL_FACT if null else NONNULL})}, ""
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
    table = with_slots(uop, rows() if table is None else table)
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


# The spellings of a calling convention that a guard may not test: the
# flags and their mask, which the reader and its members replace
RAW_CALL_FLAGS = frozenset(("ml_flags", "_Py_METH_CALL_FLAGS", "PyCFunction_GET_FLAGS"))


def check_convention_guards(uop: Uop, table: dict[str, Row]) -> None:
    """Item 2 of G0a5: a guard tests a calling convention in one of two
    fixed forms, on a local bound to the reader (`conv =
    _PyMethodDef_Convention(m)`): `conv != M1 && ... && conv != Mk`, whose
    failure is convention(M1|...|Mk), or an arity check `conv == M && ...`
    whose other terms do not name it.  No `||`, no mask, no other
    variable, and no member outside _PyCallConvention"""
    views = Views(uop, table)
    convs = {n for n, read in views.measures.items()
             if (f := views.field(read)) is not None and f[0].result.measure == "convention"}
    members = call_conventions()
    for tkn, _ in body_calls(uop):
        if tkn.text not in GUARDS:
            continue
        cond, _ = guard_condition(uop, tkn)
        named = convs & set(cond)
        if RAW_CALL_FLAGS & set(cond) or FIELD_READERS.keys() & set(cond):
            raise analysis_error(
                f"{uop.name}: the guard tests the call flags directly; bind "
                "`_PyMethodDef_Convention(...)` to a local and compare it with "
                "_PyCallConvention's members", tkn)
        if not named:
            continue
        cond = _strip(cond)
        if len(_split(cond, "||")) > 1:
            raise analysis_error(
                f"{uop.name}: the guard joins convention tests with `||`; write "
                "`conv != M1 && ... && conv != Mk`", tkn)
        terms = [_strip(t) for t in _split(cond, "&&")]
        tests = []
        for term in terms:
            if not convs & set(term):
                tests.append(None)
                continue
            if (len(term) != 3 or term[0] not in convs or term[1] not in ("!=", "==")
                    or term[2] not in members):
                raise analysis_error(
                    f"{uop.name}: `{' '.join(term)}` is no comparison of a convention "
                    "local with a member of _PyCallConvention (no mask, no other "
                    "variable)", tkn)
            tests.append(term)
        if len(named) > 1:
            raise analysis_error(
                f"{uop.name}: the guard tests more than one convention local", tkn)
        if all(t is not None and t[1] == "!=" for t in tests):
            continue        # the convention guard
        if sum(t is not None for t in tests) == 1 and next(
                t for t in tests if t is not None)[1] == "==":
            continue        # an arity check of one member
        raise analysis_error(
            f"{uop.name}: the guard mixes convention tests; write the convention "
            "guard `conv != M1 && ... && conv != Mk`, or an arity check "
            "`conv == M && ...` of one member", tkn)


# ---------------------------------------------------------------------------
# Ownership, facts and restartability on each exit (V4, V5, V6 inside a uop)

HELD, GONE, NULL = "held", "gone", "null"
# An owned output array: not yet written, wholly written, written in part
# (element stores), or written if the status local named after PENDING
# says success
UNWRITTEN, WRITTEN, PARTIAL, PENDING = "unwritten", "written", "partly written", "pending:"
# An array whose declared size is 0 on this path (`oparg & 1` tested false):
# nothing to write, publish or release
EMPTY = "empty"
# An owner that a row takes (ARGS `taken`): gone if the row succeeds, still
# the caller's if it fails.  TAKEN alone until the row's result is bound to
# a local, then TAKEN + the local, until a test of the local decides it
TAKEN = "taken:"


# The calls on an input that kill it in the generator's storage
# (generators_common.Emitter.value_kill)
SKELETON_KILLS = frozenset({"PyValue_CLOSE", "PyValue_XCLOSE", "PyValue_CLOSE_SPECIALIZED",
                            "PyValue_AsKnownObjectSteal"})


def obligated(status: str | None) -> bool:
    "whether an owner's status still owes a release: held, or held if a row fails"
    return status is not None and (status == HELD or status.startswith(TAKEN))


# Statement heads whose control flow the walk does not follow, beyond a
# `goto` of an ERROR_LABELS label and the `return` of a name
UNMODELLED_HEADS = ("return", "goto", "break", "continue", "switch", "do")
# The effects whose sites may run Python.  Python may write the frame's
# homes (a finalizer through FrameLocalsProxy), so a value that stays in a
# home, a pass-through or a borrowed input, may be replaced there: its
# facts no longer describe the home (design 1.3, 3.2)
REENTRY = frozenset(("releases", "runs_python", "polls", "switches_frame",
                     "retires_frame"))


def _input_path(name: str, inputs) -> bool:
    "whether name is a path from an input, `callable.im_func`"
    return "." in name and name.split(".", 1)[0] in inputs


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
    # What each local assigned so far stands for: an input it views, a
    # (input, path) pointer, "" for itself, None where paths disagree
    bindings: dict[str, object] = field(default_factory=dict)
    # The input whose value (and obligation) a local or an output holds,
    # through moves; "" for a value made in the body, None where paths
    # disagree (F5)
    origins: dict[str, str | None] = field(default_factory=dict)
    # A local assigned `c ? NULL : v` or `c ? v : NULL`: the condition, as
    # tokens, and whether the local is NULL where it holds (F3)
    null_when: dict[str, tuple[tuple[str, ...], bool]] = field(default_factory=dict)
    # The integer fields of CASES results, and the locals copied from them:
    # the signs each may have on this path (F6)
    signs: dict[str, frozenset[str]] = field(default_factory=dict)
    # The fields that compare exactly with -1 and 1
    exact: set[str] = field(default_factory=set)
    # The value fields of CASES results and the locals moved from them,
    # whose NULL tests decide the path
    decided: set[str] = field(default_factory=set)
    # A CASES result just bound, whose outcomes the rest of the block walks
    # one by one: (local, result)
    split: tuple[str, "Result"] | None = None
    # The CASES result whose outcome on this path sets no exception, until
    # a call that may raise
    no_exception: str = ""
    # The row that assigned the next word on this path (JUMPTO): the word
    # may then only continue to its end
    successor: str = ""
    # Locals holding a STATUS result whose row states facts of its
    # arguments on success: the names and the facts (desk C)
    status_facts: dict[str, tuple[tuple[str, frozenset], ...]] = field(default_factory=dict)
    # The inputs that the generator's storage has killed (DEAD, INPUTS_DEAD,
    # the close and steal forms): DECREF_INPUTS closes every other input
    dead: set[str] = field(default_factory=set)

    def copy(self) -> "_State":
        return _State(dict(self.owners), dict(self.facts), dict(self.pending),
                      dict(self.sources), dict(self.rehomed), dict(self.arrays),
                      dict(self.status), self.restartable, self.why, dict(self.bindings),
                      dict(self.origins), dict(self.null_when), dict(self.signs),
                      set(self.exact), set(self.decided), self.split, self.no_exception,
                      self.successor, dead=set(self.dead),
                      status_facts=dict(self.status_facts))

    def end_restart(self, why: str) -> None:
        if self.restartable:
            self.restartable, self.why = False, why

    def kill_heap_facts(self, inputs) -> None:
        # Every fact of a path from an input is of the heap: a write may
        # change the field it steps through (item 1 of G0a5).  A CASES
        # result's fields (`found.value`) are locals, not paths
        self.facts = {n: without_heap_facts(f) for n, f in self.facts.items()
                      if not _input_path(n, inputs)}
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


# The values an exact status field compares with: PySendResult's
EXACT_VALUES = {"-1": "neg", "1": "pos", "0": "zero", "PYGEN_ERROR": "neg",
                "PYGEN_RETURN": "zero", "PYGEN_NEXT": "pos"}


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
        # An owned output's last assignment of an input's value: the site and
        # the refusal if every path to next moves that input (check_outputs)
        self.moves: dict[str, tuple[Token, str]] = {}
        self.exit: _State | None = None
        self.site_effects: dict[int, list[str]] = {}
        for site in effect_sites(uop, table):
            self.site_effects.setdefault(id(site.token), []).append(site.effect)
        self.structs = struct_locals(_tokens(uop), correlated_types(table))

    def run(self) -> "OwnershipWalk":
        # An assumed fact holds at entry, and then lives as any fact (F7)
        facts = {n: implied(i.facts | {a.fact for a in i.assumed}
                            | (set() if i.nullable else {NONNULL}))
                 for n, i in self.declared.items()}
        for name, required, _ in self.contract.state:
            facts[name] = implied(required) - {NONNULL}
        state = _State({n: HELD for n in self.declared}, facts)
        state.arrays = {o.name: UNWRITTEN for o in self.contract.outputs
                        if o.array and o.mode == "owned"}
        end = self.block(self.uop.body.body, state)
        if end is not None and not self.uop.properties.always_exits:
            self.check_exit("next", end, self.uop.body.close)
            # After the uop, the skeleton closes its closed inputs: a
            # release, which may run a finalizer (design 3.2)
            for name, item in self.declared.items():
                if (item.mode == "closed" and self.kills
                        and INLINE not in end.facts.get(name, frozenset())):
                    end.kill_heap_facts(self.declared)
                    self.reenter(end, f"the skeleton's close of `{name}` after the uop, "
                                 "which may release,")
            self.exit = end
        return self

    def exported(self, name: str) -> frozenset[VFact]:
        """The facts of name that hold at the `next` exit: none for an input
        left in a home that a reentry site may have rewritten"""
        if self.exit is None or name in self.exit.rehomed:
            return frozenset()
        return frozenset(f for f in without_site_facts(self.exit.facts.get(name, frozenset()))
                         if f.kind != EXCLUDES)

    # -- exits

    def fail(self, where: Token, message: str):
        raise analysis_error(f"{self.uop.name}: {message}", where)

    def check_committed(self, row: Row, state: _State, where: Token) -> None:
        """A row whose exits are committed runs after the generator stores
        the uop's outputs: every output is written before it"""
        for out in self.contract.outputs:
            if not out.mode or out.name == "unused":
                continue
            written = (state.arrays.get(out.name) in (WRITTEN, EMPTY) if out.array
                       else out.name in state.facts or out.name in state.owners)
            if not written:
                self.fail(where, f"`{out.name}` is not written before {row.name}, whose "
                          f"exits ({', '.join(sorted(row.committed))}) are committed: "
                          "the outputs are stored before it")

    def check_exit(self, exit: str, state: _State, where: Token,
                   holding: frozenset[str] = frozenset(), committed: bool = False) -> None:
        if state.successor and exit != "next":
            self.fail(where, f"on {exit}, {state.successor} has assigned the next word: "
                      "after it the word may only continue to its end, where the "
                      "outputs are published as on next")
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
                # A new owner: published on next, or on a committed exit, if
                # it is an output; released, or NULL, on every other exit
                if status == HELD and not ((exit == "next" or committed)
                                           and self.contract.output(name)):
                    self.fail(where, f"on {exit}, the new owner `{name}` is still held")
                continue
            gone = status != HELD
            mode = item.mode or "a pass-through"
            if exit in ("miss", "deopt"):
                if gone:
                    self.fail(where, f"on {exit}, `{name}` is released or moved, but a "
                              "guard exit must leave it untouched")
            elif item.mode in UNOWNED and gone:
                self.fail(where, f"on {exit}, `{name}` is released or moved, but it is "
                          f"declared {mode}")
            elif exit == "next" and item.mode in ("consumed", "taken") and not gone:
                self.fail(where, f"on next, `{name}` is still owned, but it is declared "
                          f"{item.mode}")
            elif exit == "raise" and item.mode == "consumed":
                # A value that is never an object (a cursor) has no
                # release to wait for: held or gone, the site agrees
                # (the declared and assumed facts of the input's value, which
                # no reentry changes)
                kinds = word_kinds(item.facts | {a.fact for a in item.assumed})
                if "object" not in kinds and kinds - {"null"}:
                    continue
                if gone == (name in holding):
                    self.fail(where, f"on raise, `{name}` is "
                              f"{'released' if gone else 'still owned'}, but the site "
                              f"declares {'holding' if name in holding else 'consumed'}")
            elif (exit == "raise" and item.mode == "taken" and gone
                  and not self.owes_nothing(state.facts.get(name, frozenset()))):
                # (a value that owes nothing, None, is kept or not alike)
                self.fail(where, f"on raise, `{name}` is released, but it is declared "
                          "taken (kept on raise)")
        for name, written in state.arrays.items():
            if committed:
                continue    # check_committed: written, and stored
            if written == EMPTY:
                continue
            if exit == "next" and written != WRITTEN:
                self.fail(where, f"on next, the output array `{name}` is "
                          f"{self.array_state(written)}, but it is published whole")
            if exit != "next" and written != UNWRITTEN:
                self.fail(where, f"on {exit}, the output array `{name}` is "
                          f"{self.array_state(written)}: its values would leak or be "
                          "published by a word that did not complete")
        if exit == "next" or committed:
            # One publication: next, a committed exit (the generator stores
            # the outputs before it), and a successor's path at the body's end
            self.check_outputs(state, where)

    @staticmethod
    def array_state(written: str) -> str:
        if written.startswith(PENDING):
            return (f"written only if `{written[len(PENDING):]}` reports success, "
                    "which nothing tests here")
        return written

    def holds_an_object(self, name: str) -> bool:
        """Whether the declared input may be an object: a value that never
        is (a tagged index) has no owner, so passing it on names no move"""
        item = self.declared[name]
        return "object" in word_kinds(item.facts | {a.fact for a in item.assumed})

    def check_outputs(self, state: _State, where: Token) -> None:
        """V5 on the paths to next: each value output is not NULL unless it
        is nullable, and has its declared facts, whatever path reached it"""
        for out in self.contract.outputs:
            if out.array or out.mode not in ("owned", "inline", "moved", "borrowed"):
                continue
            live = state.facts.get(out.name, frozenset())
            source = state.sources.get(out.name)
            origin = state.origins.get(out.name)
            if (out.mode in ("owned", "inline") and origin and origin in self.declared
                    and out.name in self.moves and self.holds_an_object(origin)):
                self.fail(*self.moves[out.name])
            if out.source == HOME_SOURCE and state.origins.get(out.name) != HOME_SOURCE:
                self.fail(where, f"on next, `{out.name}` is declared moved(home), but the "
                          "home it is loaded from still holds it on some path: overwrite "
                          "the home")
            # An output declared `null` is NULL on next (_PUSH_NULL)
            if not out.nullable and NULL_FACT not in out.facts and NONNULL not in live:
                if source is not None:
                    self.fail(where, f"`{out.name}` may be NULL from {source}, and nothing "
                              "raises on it on every path to next")
                self.fail(where, f"on next, `{out.name}` is not assigned on every path")
            missing = {f for f in implied(out.facts) - live
                       if not satisfied(f, live)}
            if missing:
                self.fail(where, f"on next, `{out.name}` is declared "
                          f"{', '.join(sorted(map(str, missing)))}, which does not hold on "
                          "every path (a join, or a kill since the assignment)")

    # -- statements

    def block(self, stmts: list[Stmt], state: _State) -> _State | None:
        "the state after stmts, or None if they always leave"
        for i, stmt in enumerate(stmts):
            state = self.stmt(stmt, state)
            if state is None:
                ender = next((t.text for t in stmt.tokens() if t.text in self.table
                              and ends_path(self.table[t.text])), None) \
                    if isinstance(stmt, SimpleStmt) else None
                if ender is not None and i + 1 < len(stmts):
                    # Nothing after a row that never returns runs: a guard
                    # there (after IR_ENTER_GENERATOR's frame switch) would
                    # never be checked
                    self.fail(next(iter(stmts[i + 1].tokens())), f"this statement follows "
                              f"{ender}, which never returns, so it never runs")
                return None
            if state.split is not None:
                # A CASES result: the rest of the block once per outcome,
                # with its fields as that outcome states them (F6)
                name, result = state.split
                state.split = None
                exact = frozenset().union(*(c.exact for c in result.cases))
                return self.join([self.block(stmts[i + 1:],
                                             self.outcome(state, name, c, exact))
                                  for c in result.cases])
        return state

    def outcome(self, state: _State, name: str, case: Case,
                exact: frozenset[str]) -> _State:
        """state in one outcome of the CASES result that name holds; exact
        are its fields that are -1, 0 or 1"""
        st = state.copy()
        for f, signs in case.signs:
            st.signs[f"{name}.{f}"] = signs
            if f in exact:
                st.exact.add(f"{name}.{f}")
        for f, spec in case.values:
            field_ = f"{name}.{f}"
            st.decided.add(field_)
            st.origins[field_] = ""
            if spec.mode == "null":
                st.owners[field_] = NULL
                st.assign(field_, {NULL_FACT}, None, name)
                continue
            if spec.mode == "owned":
                st.owners[field_] = HELD
                self.new.add(field_)
            # An inline value owes nothing: it is no object
            facts = spec.facts | ({VFact(EXCLUDES, "object")} if spec.mode == "inline"
                                  else set())
            if not spec.nullable:
                st.assign(field_, facts | {NONNULL}, None, name)
            else:
                # The facts of a value that may be NULL hold once it is not;
                # a union with null holds on either path
                st.assign(field_, facts if "null" in word_kinds(spec.facts) else frozenset(),
                          facts | {NONNULL}, name)
        st.no_exception = "" if case.raised else name
        return st

    def infeasible(self, state: _State, cond: list[str], holds: bool) -> bool:
        """Whether a condition on a CASES result's fields cannot be `holds`
        on this path; else the path learns it (F6)"""
        cond = _strip(cond)
        # A conjunction that holds, or a disjunction that fails, holds each
        # of its terms so: infeasible if one of them is
        for op, whole in (("&&", True), ("||", False)):
            terms = _split(cond, op)
            if len(terms) > 1:
                if holds != whole:
                    return False
                return any([self.infeasible(state, t, holds) for t in terms])
        while cond[:1] == ["!"]:
            cond, holds = _strip(cond[1:]), not holds
        null = _call(cond, "PyValue_IsNull")
        if null is not None and len(null) == 1 and len(null[0]) == 1 \
                and null[0][0] in state.decided:
            facts = state.facts.get(null[0][0], frozenset())
            return (NULL_FACT in facts and not holds) or (NONNULL in facts and holds)
        # A pointer field compared with NULL, `pair.match == NULL`
        if len(cond) > 2 and cond[-2] in ("==", "!=") and cond[-1] == "NULL" \
                and "".join(cond[:-2]) in state.decided:
            facts = state.facts.get("".join(cond[:-2]), frozenset())
            is_null = holds if cond[-2] == "==" else not holds
            return (NULL_FACT in facts and not is_null) or (NONNULL in facts and is_null)
        name = cond[0] if cond else ""
        if name not in state.signs:
            return False
        rest = cond[1:]
        if not rest:
            found, holds = SIGN_TESTS["!="], holds
        elif len(rest) == 2 and rest[0] in SIGN_TESTS and rest[1] == "0":
            found = SIGN_TESTS[rest[0]]
        elif (len(rest) in (2, 3) and rest[0] in ("==", "!=") and name in state.exact
              and "".join(rest[1:]) in EXACT_VALUES):
            found = {EXACT_VALUES["".join(rest[1:])]}
            if rest[0] == "!=":
                holds = not holds
        else:
            return False
        signs = state.signs[name] & (found if holds else SIGNS - found)
        if not signs:
            return True
        state.signs[name] = frozenset(signs)
        return False

    def stmt(self, stmt: Stmt, state: _State) -> _State | None:
        if isinstance(stmt, BlockStmt):
            return self.block(stmt.body, state)
        if isinstance(stmt, MacroIfStmt):
            # The GIL product's arm (F15); a Py_DEBUG one is a path of one
            # supported build, and an undecided one holds no code
            # (check_preprocessor), so both are walked and joined
            holds = macro_condition(stmt.condition.text)
            if holds is not None:
                return self.block(stmt.body if holds else stmt.else_body or [], state)
            return self.join([self.block(stmt.body, state.copy()),
                              self.block(stmt.else_body or [], state.copy())])
        if isinstance(stmt, IfStmt):
            toks = fold_fields([t for t in stmt.condition if t.kind != "COMMENT"],
                               self.structs)
            self.calls(toks, state)
            self.effects(toks, state)
            cond = [t.text for t in toks]
            then, other = state.copy(), state.copy()
            # Feasibility first: refine states what the branch tests
            dead = [self.infeasible(then, cond, True), self.infeasible(other, cond, False)]
            for st, holds in ((then, True), (other, False)):
                self.refine(st, cond, holds)
            ends = [None if dead[0] else self.stmt(stmt.body, then)]
            if dead[1]:
                ends.append(None)
            else:
                ends.append(self.stmt(stmt.else_body, other) if stmt.else_body else other)
            return self.join(ends)
        if isinstance(stmt, (ForStmt, WhileStmt)):
            # The body may run any number of times: walk it once and join
            return self.join([self.stmt(stmt.body, state.copy()), state])
        assert isinstance(stmt, SimpleStmt)
        return self.simple(fold_fields(stmt.contents, self.structs), state)

    def write_state(self, toks: list[Token], texts: list[str], state: _State) -> None:
        """A declared write of a STATE item: `|= BIT | ...` states each bit
        set, `&= ~BIT` it clear, and kills the facts of the bits it writes;
        any other assignment kills all its facts.  A term that is no bit of
        the item's row is refused: an undeclared bit states nothing"""
        name, op = texts[0], texts[1]
        row = self.table[name]
        rhs = _strip(texts[2:-1] if texts[-1] == ";" else texts[2:])
        bits = {b[0]: b for b in row.bits}

        def bit(term: list[str]) -> tuple[str, frozenset, frozenset]:
            if len(term) != 1 or term[0] not in bits:
                self.fail(toks[0], f"`{name} {op} {' '.join(rhs)}` writes "
                          f"`{' '.join(term)}`, which is no bit of `{name}` "
                          f"({', '.join(bits)})")
            return bits[term[0]]

        if op == "|=":
            written = [bit(_strip(term)) for term in _split(rhs, "|")]
            facts = frozenset().union(*(b[1] for b in written))
        elif op == "&=" and len(rhs) == 2 and rhs[0] == "~":
            written = [bit(rhs[1:])]
            facts = written[0][2]
        else:
            written, facts = list(row.bits), frozenset()
        kinds = {f.kind for b in written for f in b[1] | b[2]}
        state.facts[name] = frozenset(
            f for f in state.facts.get(name, frozenset())
            if f.kind not in kinds) | facts

    def owes_nothing(self, facts: frozenset) -> bool:
        "whether a value with facts is inline or a CONSTANT row's (V8's roots)"
        # A word that is neither an object nor NULL: an inline number, a
        # tagged index; or one that is that or NULL (a cursor, a tagged
        # index or NULL), whose close releases nothing either
        kinds = word_kinds(facts)
        return not kinds & {"object", "null"} or (
            "object" not in kinds and bool(kinds - {"null"})) or any(f.kind == "is" and is_value_constant(self.table.get(f.arg)) for f in facts) \
            or owes_nothing_by_type(facts, self.table)

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
                if {a, b} == {HELD, NULL} or (HELD in (a, b) and NULL_FACT in (
                        other if a == HELD else first).facts.get(name, frozenset())):
                    # A new owner, NULL on one path (a NULL local, or a
                    # PyValue_NULL assigned): held, and maybe NULL
                    a = HELD
                elif HELD in (a, b) and self.owes_nothing(
                        (other if a == HELD else first).facts.get(name, frozenset())):
                    # A new owner, an inline or static immortal value on one
                    # path (None for a NULL completion): held, since closing
                    # such a value releases nothing
                    a = HELD
                elif obligated(a) != obligated(b) or (
                        a != b and (a.startswith(TAKEN) or b.startswith(TAKEN))):
                    self.fail(self.uop.body.open, f"`{name}` is owned on one path and "
                              "not on another where they join")
                first.owners[name] = a
            # What holds of a name once it is not NULL, on each path: its
            # pending facts, or the facts it already has; a path where it is
            # NULL adds nothing
            pending = {}
            nulls = {n for st in (first, other) for n, f in st.facts.items() if NULL_FACT in f}
            for name in set(first.pending) | set(other.pending) | nulls:
                a, b = (implied(st.facts.get(name, frozenset()) | st.pending.get(name, frozenset()))
                        for st in (first, other))
                found = (b if NULL_FACT in a else a if NULL_FACT in b else a & b) - {NULL_FACT}
                if found and not (NULL_FACT in a and NULL_FACT in b):
                    pending[name] = found
            for name in set(first.origins) | set(other.origins):
                a, b = first.origins.get(name, ""), other.origins.get(name, "")
                if a != b and NULL_FACT not in other.facts.get(name, frozenset()):
                    # A NULL on one path carries no identity
                    first.origins[name] = (b if NULL_FACT in first.facts.get(name, frozenset())
                                           else None)
            first.facts = _intersection([first.facts, other.facts])
            first.pending = pending
            first.dead &= other.dead
            first.sources.update(other.sources)
            for name, why in other.rehomed.items():
                first.rehomed.setdefault(name, why)
            for name in first.arrays:
                # An empty array joins as the other path's: the size test
                # decided each path's array
                if first.arrays[name] == EMPTY:
                    first.arrays[name] = other.arrays.get(name, UNWRITTEN)
                elif other.arrays.get(name) == EMPTY:
                    continue
                if first.arrays[name] != other.arrays.get(name):
                    self.fail(self.uop.body.open, f"the output array `{name}` is "
                              f"{self.array_state(first.arrays[name])} on one path and "
                              f"{self.array_state(other.arrays.get(name, UNWRITTEN))} on "
                              "another where they join")
            first.status.update(other.status)
            for name in set(first.signs) | set(other.signs):
                first.signs[name] = (first.signs.get(name, SIGNS)
                                     | other.signs.get(name, SIGNS))
            first.exact &= other.exact
            first.decided &= other.decided
            if first.no_exception != other.no_exception:
                first.no_exception = ""
            # A successor assigned on any path may be the next word
            first.successor = first.successor or other.successor
            for name in set(first.bindings) | set(other.bindings):
                if first.bindings.get(name, "") != other.bindings.get(name, ""):
                    first.bindings[name] = None
            for name in set(first.null_when) | set(other.null_when):
                if first.null_when.get(name) != other.null_when.get(name):
                    first.null_when.pop(name, None)
            if not other.restartable:
                first.end_restart(other.why)
        return first

    def views_at(self, state: _State) -> Views:
        "the views where the walk is: each local as last bound on this path"
        views = copy.copy(self.views)
        views.names, views.paths = dict(views.names), dict(views.paths)
        views.unknown = set(views.unknown)
        for name, bound in state.bindings.items():
            views.names.pop(name, None)
            views.paths.pop(name, None)
            if bound is None:
                views.unknown.add(name)
            elif isinstance(bound, tuple):
                views.paths[name] = bound
            elif bound:
                views.names[name] = bound
        return views

    def refine_identity(self, state: _State, cond: list[str], holds: bool) -> None:
        """Where an IDENTITY() test holds, its two named arguments are one
        word: each has the other's facts, but those of one site"""
        cond = _strip(cond)
        while cond[:1] == ["!"]:
            cond, holds = _strip(cond[1:]), not holds
        call = _call_of(_unwrap(cond, self.table), self.table)
        if not holds or call is None or not call[0].identity:
            return
        views = self.views_at(state)
        names = [views.resolve(a) for a in call[1]]
        if None in names:
            return
        a, b = names
        facts_a = without_site_facts(state.facts.get(a, frozenset()))
        facts_b = without_site_facts(state.facts.get(b, frozenset()))
        state.add_facts(a, facts_b)
        state.add_facts(b, facts_a)

    def refine(self, state: _State, cond: list[str], holds: bool) -> None:
        self.refine_status(state, cond, holds)
        found = condition_facts(cond, self.views_at(state), self.table, holds)
        if found:
            for name, facts in found[0].items():
                state.add_facts(name, facts)
        self.refine_identity(state, cond, holds)
        # `PyValue_IsNull(x)`: a new owner that is NULL owes nothing; the
        # owners that the row which made x takes are the caller's again
        cond = _strip(cond)
        while cond[:1] == ["!"]:
            cond, holds = _strip(cond[1:]), not holds
        if not holds:
            # The test of an output array's declared size (`oparg & 1`)
            # fails: the array has no element, so it is wholly written
            for out in self.contract.outputs:
                if (out.array and out.name in state.arrays
                        and "".join(cond) == re.sub(r"[\s()]", "", out.size)):
                    state.arrays[out.name] = EMPTY
        for name, (when, null_if) in list(state.null_when.items()):
            if tuple(cond) == when:
                self.set_null(state, name, holds == null_if)
        null = _call(cond, "PyValue_IsNull")
        # The same test of an object pointer, `x == NULL` or `x != NULL`
        # (BUILD_MAP's map_o): a row's VALUE_OR_NULL result as a PyObject *
        if null is None and len(cond) == 3 and cond[1] in ("==", "!=") and \
                cond[2] == "NULL" and re.fullmatch(r"[A-Za-z_]\w*", cond[0]):
            null, holds = [[cond[0]]], holds == (cond[1] == "==")
        # `x` or `!x` as a condition, of such a pointer: not NULL where it
        # holds.  Only a new owner or a row's result that decides what the
        # row took; a status local is refine_status's
        if null is None and len(cond) == 1 and re.fullmatch(r"[A-Za-z_]\w*", cond[0]) \
                and cond[0] not in state.status and (
                    (state.owners.get(cond[0]) == HELD and cond[0] in self.new)
                    or TAKEN + cond[0] in state.owners.values()):
            null, holds = [[cond[0]]], not holds
        if null is not None and len(null) == 1:
            name = " ".join(null[0])
            for owner, status in state.owners.items():
                if status == TAKEN + name:
                    state.owners[owner] = HELD if holds else GONE
            if state.owners.get(name) == HELD and name in self.new:
                self.set_null(state, name, holds)

    def set_null(self, state: _State, name: str, null: bool) -> None:
        "name is NULL, and owes nothing, or it is not, and has its pending facts"
        if null:
            if state.owners.get(name) == HELD and name in self.new:
                state.owners[name] = NULL
            state.facts[name] = frozenset({NULL_FACT})
            state.pending.pop(name, None)
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
        if not failed:
            for arg, facts in state.status_facts.get(name, ()):
                state.add_facts(arg, facts)

    def effects(self, tokens: list[Token], state: _State) -> None:
        """the effects of the sites among tokens: heap facts die, homes may be
        rewritten, restart ends"""
        for t in tokens:
            for effect in self.site_effects.get(id(t), ()):
                if effect in KILLS and self.kills:
                    state.kill_heap_facts(self.declared)
                if effect in REENTRY and self.kills:
                    self.reenter(state, f"`{t.text}` at line {t.line}, which may {effect},")
                if effect != "reads" and effect != "raises":
                    state.end_restart(f"`{t.text}`, which may {effect}")
                if effect in ("raises", "runs_python") and t.text not in DSL_CONSTRUCTS:
                    # A call that may set an exception (a raise site sets none)
                    state.no_exception = ""

    def guard_results(self, tokens: list[Token], state: _State) -> None:
        """The GUARD rows called among tokens: on their normal return, after
        their effects, their facts hold of their arguments"""
        views = self.views_at(state)
        for tkn, args in _calls_in(tokens):
            row = self.table.get(tkn.text)
            if row is None or row.result.shape != "GUARD":
                continue
            for arg, facts in zip(args, row.result.guards):
                name = views.resolve(arg)
                facts = _bind_facts(facts, args, views)
                if name is not None and facts:
                    state.add_facts(name, facts)

    def home_store(self, texts: list[str], i: int) -> bool:
        "Whether the assignment at texts[i] stores through a HOME row"
        lhs = texts[:i]
        start = next((j for j in range(len(lhs) - 1, -1, -1) if lhs[j] in (";", "{", "}")), -1)
        call = _call_of(lhs[start + 1:], self.table)
        return call is not None and call[0].result.shape == "HOME"

    def home_key(self, tokens: list[str]) -> str | None:
        "the home that a HOME row call at the end of tokens addresses, as text"
        start = next((j for j in range(len(tokens) - 1, -1, -1)
                      if tokens[j] in (";", "{", "}", "=")), -1)
        call = _call_of(_uncast(_strip(tokens[start + 1:])), self.table)
        if call is None or call[0].result.shape != "HOME":
            return None
        return f"{call[0].name}({', '.join(' '.join(a) for a in call[1])})"

    def kill_home_loads(self, state: _State) -> None:
        "The values loaded from homes may be gone: their facts die (F18)"
        for name, source in state.sources.items():
            row = self.table.get(source)
            if row is not None and row.result.shape == "HOME" and name not in state.owners:
                state.facts[name] = frozenset()
                state.pending.pop(name, None)

    def reenter(self, state: _State, why: str) -> None:
        """A reentry site: a value left in a home may be replaced there (and
        the old one freed), so its facts die now, and those of the locals
        that copy it"""
        # A site fact (unique) dies at any reentry, and a home load
        state.facts = {n: without_site_facts(f) for n, f in state.facts.items()
                       if not _input_path(n, self.declared)}
        self.kill_home_loads(state)
        for name, item in self.declared.items():
            if item.mode in UNOWNED:
                state.rehomed.setdefault(name, why)
                state.facts[name] = frozenset()
                state.pending.pop(name, None)
        for name, source in state.sources.items():
            if source in state.rehomed and name not in state.owners:
                state.facts[name] = frozenset()

    def owner_of(self, arg: list[str], state: _State) -> str | None:
        """The obligation that an argument passes: a name that holds one, or
        an input through a borrowed view of it; None for anything else (a
        nested call passes what its own row says)"""
        tokens = _unwrap(arg, self.table)
        if len(tokens) != 1:
            # A borrowed view written in the argument passes what a local
            # assigned from it would; a consuming view takes on its own
            call = _call_of(tokens, self.table)
            if (call is not None and call[0].result.shape == "VIEW" and len(call[1]) == 1
                    and call[0].args[0].mode != "consumed"):
                return self.owner_of(call[1][0], state)
            return None
        if tokens[0] in state.owners:
            return tokens[0]
        name = self.views_at(state).resolve(tokens, locals_=False)
        return name if name in self.declared else None

    def expression_facts(self, arg: list[str], state: _State) -> frozenset:
        """The facts of an argument that names no value: a field of a CASES
        result this path decided, an element of a declared array (its
        declaration), a literal NULL, a row's call (its result); nothing
        for any other expression"""
        tokens = _unwrap(_uncast(arg), self.table)
        if len(tokens) == 1 and tokens[0] in state.decided:
            return state.facts.get(tokens[0], frozenset())
        if tokens in (["NULL"], ["PyValue_NULL"]):
            return frozenset({NULL_FACT})
        if len(tokens) == 4 and tokens[1] == "[" and tokens[3] == "]":
            item = self.contract.input(tokens[0]) or self.contract.output(tokens[0])
            if item is not None and item.array:
                return implied(item.facts | {a.fact for a in item.assumed}
                               | (frozenset() if item.nullable else {NONNULL}))
        return argument_facts(arg, self.table)

    def passes_no_owner(self, arg: list[str], name: str | None, state: _State) -> bool:
        """Whether an argument that names no owner may still be consumed: a
        row's new owner, passed as it is made, NULL, a CONSTANT row's value,
        or a value that owes no count (owes_nothing)"""
        call = _call_of(_uncast(_unwrap(arg, self.table)), self.table)
        if call is not None and call[0].new_owner():
            return True
        shown = implied(state.facts.get(name, frozenset()) if name is not None
                        else self.expression_facts(arg, state))
        tokens = _unwrap(_uncast(arg), self.table)
        constant = self.table.get(tokens[0]) if len(tokens) == 1 else None
        if tokens[0] not in state.facts and is_value_constant(constant):
            # A static immortal value (V8's roots), passed as itself
            shown = implied(constant.result.facts)
        return NULL_FACT in shown or bool(shown) and self.owes_nothing(shown)

    def take(self, name: str, state: _State, where: Token, what: str) -> None:
        "an owner's obligation leaves it (a consumption or a move)"
        item = self.declared.get(name)
        if item is not None and item.mode in UNOWNED:
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
            if (tkn.text in SKELETON_KILLS and len(args) == 1
                    and len(args[0]) == 1 and args[0][0] in self.declared):
                state.dead.add(args[0][0])
            row = self.table.get(tkn.text)
            if row is None:
                continue
            for exit in sorted(row.exits & {"miss", "deopt"}):
                # A row's guard exit, before the row takes anything
                self.check_exit(exit, state, tkn)
            taken = []
            views = self.views_at(state)
            for arg, spec in zip(args, row.args):
                destructor = (self.table.get(" ".join(_unwrap(arg, self.table)))
                              if spec.mode == "destructor" else None)
                if destructor is not None and destructor.kind == "DESTRUCTOR":
                    # It frees only objects of exactly its type
                    for value, vspec in zip(args, row.args):
                        if vspec.mode != "consumed":
                            continue
                        facts = state.facts.get(views.resolve(value), frozenset())
                        if INLINE not in facts and VFact("exact", destructor.frees) not in facts:
                            self.fail(tkn, f"closes `{' '.join(value)}` through "
                                      f"{destructor.name}, which frees only "
                                      f"exact({destructor.frees}), which it is not shown "
                                      "to be")
                name = views.resolve(arg)
                if spec.facts:
                    # What the argument is shown to be: a named value's
                    # facts, or what the expression shows
                    missing = implied(spec.facts) - (
                        state.facts.get(name, frozenset()) if name is not None
                        else self.expression_facts(arg, state))
                    if missing:
                        self.fail(tkn, f"passes `{' '.join(arg)}` to {row.name}, which "
                                  f"requires {', '.join(sorted(map(str, missing)))} of it")
                if name in state.facts and (row.new_owner() or spec.mode in (
                        "consumed", "taken", "out")):
                    # A count change of the value: unique dies
                    state.facts[name] = without_site_facts(state.facts[name])
                if spec.mode in ("consumed", "taken"):
                    owner = self.owner_of(arg, state)
                    if owner is None and not self.passes_no_owner(arg, name, state):
                        self.fail(tkn, f"{row.name} "
                                  f"{'consumes' if spec.mode == 'consumed' else 'takes'} "
                                  f"`{' '.join(arg)}`, which names no owner")
                    if owner is not None:
                        self.take(owner, state, tkn, f"{row.name} "
                                  f"{'consumes' if spec.mode == 'consumed' else 'takes'}")
                        state.end_restart(f"{row.name} consuming `{owner}`")
                        if spec.mode == "taken":
                            taken.append(owner)
                if spec.mode == "out":
                    array = " ".join(_unwrap(arg, self.table))
                    declared = self.contract.output(array)
                    if array in state.arrays and implied(declared.facts) - {NONNULL}:
                        # A row's `out` writes owned, non-NULL values and
                        # says nothing more of them
                        self.fail(tkn, f"{row.name} writes `{array}`, declared "
                                  f"{', '.join(sorted(map(str, declared.facts)))}, which "
                                  "its row does not show of the elements")
                    if array in state.arrays:
                        # Publication: wholly written, or written if a STATUS
                        # result says so (the assignment names the local)
                        state.arrays[array] = (PENDING if row.result.shape == "STATUS"
                                               else WRITTEN)
                        state.end_restart(f"the publication of `{array}` by {row.name}")
            if row.committed:
                self.check_committed(row, state, tkn)
            for exit in sorted(row.exits - {"next", "miss", "deopt"} - row.successor):
                # A row's other exits leave the word from inside it, after
                # it has taken what its arguments' modes say: on its raise,
                # a `taken` argument is still the caller's (design 1.2)
                at = state
                if exit == "raise" and taken:
                    at = state.copy()
                    for owner in taken:
                        at.owners[owner] = HELD
                self.check_exit(exit, at, tkn, committed=exit in row.committed)
            if row.successor:
                state.successor = row.name
            if taken and row.result.shape in ("VALUE_OR_NULL", "STATUS"):
                # Gone if the result reports success: the assignment binds
                # the result's local, whose test decides (refine)
                for owner in taken:
                    state.owners[owner] = TAKEN

    def discharge(self, name: str, state: _State, where: Token) -> None:
        """DEAD(name): an input already moved or released, or an inline
        value, owes nothing; a closed input still held is handed to the
        skeleton, which closes it after the uop; any other held owner would
        leak"""
        item = self.declared.get(name)
        if item is not None and item.mode == "closed":
            return
        # A static immortal value, is(PyValue_None), owes no count either
        if state.owners.get(name) == HELD and not self.owes_nothing(
                state.facts.get(name, frozenset())):
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
            if args[0] == ["true"] or self.infeasible(state, args[0], False):
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
            dead = bool(args[0]) and self.infeasible(raised, args[0], True)
            if args[0]:
                self.refine(raised, args[0], True)
            if not dead:
                if raised.no_exception:
                    self.fail(toks[0], f"raises where `{raised.no_exception}` reports an "
                              "outcome that sets no exception, and nothing since may raise")
                self.check_exit("raise", raised, toks[0], holding)
            if head == "ERROR_NO_POP" or args[0] == ["true"]:
                return None
            if self.infeasible(state, args[0], False):
                return None
            self.refine(state, args[0], False)
            return state
        if head == "DISPATCH_SAME_OPARG":
            return None
        if self.contract.state_item(head) is not None and texts[1:2] in (["|="], ["&="], ["="]):
            self.write_state(toks, texts, state)
            return state
        if head in self.table and self.table[head].exits == {"redirect"}:
            # DISPATCH_REDIRECT: the word leaves for the word an observer
            # chose; what the word holds is released first, as on raise
            self.calls(toks, state)
            self.effects(toks, state)
            self.check_exit("redirect", state, toks[0])
            return None
        if head in self.table and ends_path(self.table[head]):
            # A terminal marker leaves on its row's exits, after its
            # arguments' modes have taken what they say; it never returns.
            # So does a row whose exits are committed and none is next
            # (IR_OBSERVE_COMMITTED_BRANCH dispatches the word it observes)
            self.calls(toks, state)
            self.effects(toks, state)
            return None
        if head == "goto" and texts[1:2] and texts[1] in ERROR_LABELS:
            # The raise exit at this site, with its checkpoint here
            if state.no_exception:
                self.fail(toks[0], f"raises where `{state.no_exception}` reports an "
                          "outcome that sets no exception, and nothing since may raise")
            self.check_exit("raise", state, toks[0])
            return None
        if head == "return" and len(texts) == 3 and re.fullmatch(r"\w+", texts[1]):
            # Leaves the evaluator with the value: its obligation goes to the
            # evaluator's caller
            if obligated(state.owners.get(texts[1])):
                self.take(texts[1], state, toks[0], "returns")
            self.check_exit("exit", state, toks[0])
            return None
        if head == "DEAD":
            name = self.views_at(state).resolve(texts[2:_close(texts, 1)])
            if name is not None:
                self.discharge(name, state, toks[0])
                state.dead.add(name)
            return state
        if head == "INPUTS_DEAD":
            for name in self.declared:
                self.discharge(name, state, toks[0])
                state.dead.add(name)
            return state
        if head == "DECREF_INPUTS":
            # The emitters close the inputs that are still held
            for name, item in self.declared.items():
                if state.owners[name] == HELD and item.mode in UNOWNED:
                    self.fail(toks[0], f"DECREF_INPUTS releases `{name}`, declared "
                              f"{item.mode or 'a pass-through'}")
                if (state.owners[name] == GONE and name not in state.dead
                        and item.mode not in UNOWNED):
                    # The generator closes it again: a second release
                    self.fail(toks[0], f"DECREF_INPUTS releases `{name}`, which is "
                              f"already consumed or moved: mark it DEAD(`{name}`) first")
                state.owners[name] = GONE
            self.effects(toks, state)
            state.end_restart("DECREF_INPUTS")
            return state
        self.check_forms(toks, texts, state)
        e = texts.index("=") if "=" in texts else -1
        end = len(texts) - 1 if texts[-1] == ";" else len(texts)
        arms = _conditional(texts[e + 1:end]) if e > 0 else None
        if arms is None or not re.fullmatch(r"\w+", texts[e - 1]) or texts[e - 2:e - 1] in (
                ["->"], ["."]):
            self.calls(toks, state)
            self.effects(toks, state)
            self.guard_results(toks, state)
            if e > 0:
                self.assignment(toks, texts, e, state)
            return state
        return self.conditional(toks, texts, e, e + 1 + arms[0], e + 1 + arms[1], end, state)

    def conditional(self, toks: list[Token], texts: list[str], e: int, q: int, c: int,
                    end: int, state: _State) -> _State:
        """`x = cond ? a : b;`: each arm on its own path, refined by the
        condition, then joined (F3): its facts are those of both arms, its
        obligation the same on both, and where one arm is NULL the local
        is NULL exactly where the condition says so (refine).  An arm that
        a CASES outcome rules out is not walked (F6)"""
        self.calls(toks[:q], state)
        self.effects(toks[:q], state)
        cond = _strip(texts[e + 1:q])
        ends, nulls = [], []
        arms = (((q + 1, c), True), ((c + 1, end), False))
        feasible = [not self.infeasible(state.copy(), cond, holds) for _, holds in arms]
        for ((lo, hi), holds), keep in zip(arms, feasible):
            if not keep and any(feasible):
                continue
            path = state.copy()
            self.infeasible(path, cond, holds)
            self.refine(path, cond, holds)
            self.calls(toks[lo:hi], path)
            self.effects(toks[lo:hi], path)
            self.assignment(toks, texts, e, path, rhs=_strip(texts[lo:hi]))
            nulls.append(NULL_FACT in path.facts.get(texts[e - 1], frozenset()))
            ends.append(path)
        joined = self.join(ends)
        if len(nulls) == 2 and nulls[0] != nulls[1]:
            # As refine reads a condition: without its negations
            null_if = nulls[0]
            while cond[:1] == ["!"]:
                cond, null_if = _strip(cond[1:]), not null_if
            joined.null_when[texts[e - 1]] = (tuple(cond), null_if)
        return joined

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
        arms: list[list[str]] = []
        if statement and "=" in expr:
            e = expr.index("=")
            rhs = _uncast(expr[e + 1:])
            lhs = expr[:e]
            for j in range(e + 1):
                covered[j] = True
            named = (re.fullmatch(r"\w+", lhs[-1]) is not None
                     and (len(lhs) < 2 or lhs[-2] not in ("->", ".", "*")))
            element = ((lhs[-1] == "]" and _element_base(texts, e) in state.arrays)
                       or self.home_store(texts, e))
            # The values assigned: the right side, or the arms of `c ? a : b`
            # into a name, at their positions in texts
            spots = [(e + 1, len(expr))]
            marks = _conditional(expr[e + 1:]) if named else None
            if marks is not None:
                q, c = e + 1 + marks[0], e + 1 + marks[1]
                spots = [(q + 1, c), (c + 1, len(expr))]
            for lo, hi in spots:
                arm = _uncast(texts[lo:hi])
                arms.append(arm)
                if len(arm) == 1 and arm[0] in tracked and (named or element):
                    covered[hi - 1 - texts[lo:hi][::-1].index(arm[0])] = True
        for j in range(len(texts) - 3):
            if texts[j:j + 3] == ["(", "void", ")"] and re.fullmatch(r"\w+", texts[j + 3]):
                covered[j:j + 4] = [True] * 4
        for j in range(1, len(texts)):
            # A member's name (`ht->_spec_cache.iter`) is no use of a value
            if texts[j - 1] in (".", "->"):
                covered[j] = True
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
            as_rhs = any(_strip(arm) == call for arm in arms)
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

    def assignment(self, toks: list[Token], texts: list[str], i: int, state: _State,
                   rhs: list[str] | None = None) -> None:
        lhs = texts[i - 1]
        if rhs is None:
            rhs = _strip(texts[i + 1:-1] if texts[-1] == ";" else texts[i + 1:])
        if lhs == "]":
            self.element_store(toks, texts, i, rhs, state)
            return
        if self.home_store(texts, i):
            # A home write (F18): the value's obligation moves into the
            # home, and what was loaded from a home dies.  The home's old
            # value is no longer the home's: a moved(home) output loaded
            # from it takes it
            if len(rhs) == 1 and rhs[0] in state.owners:
                self.take(rhs[0], state, toks[0], "moves into a home")
                state.end_restart(f"the store of `{rhs[0]}` into a home")
            key = self.home_key(texts[:i])
            for name, origin in list(state.origins.items()):
                if key is not None and origin == _home_origin(key):
                    state.origins[name] = HOME_SOURCE
                    if name not in state.owners and name not in self.declared:
                        # A home move (`tmp = GETLOCAL(i); GETLOCAL(i) = v;`):
                        # the home's old obligation is the local's now
                        state.owners[name] = HELD
                        self.new.add(name)
            self.kill_home_loads(state)
            return
        if i >= 2 and texts[i - 2] in ("->", ".") or not re.fullmatch(r"\w+", lhs):
            return      # a store through a pointer: V2's `writes`
        item = self.declared.get(lhs)
        if item is not None and item.mode in UNOWNED:
            # The home keeps the input's word: a new value there is owned
            # by no one, and a pass-through output would publish it as the
            # caller's value
            self.fail(toks[0], f"assigns `{lhs}`, an input declared "
                      f"{item.mode or 'a pass-through'}")
        if obligated(state.owners.get(lhs)) and (item is None or item.mode
                                                  in ("consumed", "taken")):
            self.fail(toks[0], f"`{lhs}` is assigned while it holds an owner, which "
                      "would leak")
        if item is None:
            # What the local stands for from here on (Views)
            views = self.views_at(state)
            path = views.path(rhs)
            state.bindings[lhs] = (path if path is not None
                                   else views.resolve(rhs, locals_=False) or "")
        self.bind(toks, lhs, rhs, state)

    def bind(self, toks: list[Token], lhs: str, rhs: list[str], state: _State) -> None:
        """`lhs = rhs` for a name: a move, a copy, a constant, a row's value
        or anything else; the value's obligation, facts and identity go to
        lhs, and an output's identity is checked against its mode"""
        out = self.contract.output(lhs)
        state.null_when.pop(lhs, None)
        state.origins[lhs] = ""
        state.signs.pop(lhs, None)
        state.exact.discard(lhs)
        state.decided.discard(lhs)
        value = _uncast(rhs)
        if len(value) == 1 and value[0] in state.signs:
            # A copy of an integer field of a CASES result: its signs here
            state.signs[lhs] = state.signs[value[0]]
            if value[0] in state.exact:
                state.exact.add(lhs)
        if len(value) == 1 and value[0] in state.decided:
            state.decided.add(lhs)
        if len(value) == 1 and state.owners.get(value[0]) == NULL \
                and value[0] not in self.declared:
            # A NULL moves no obligation
            state.owners[lhs] = NULL
            state.assign(lhs, state.facts.get(value[0], frozenset()), None, value[0])
            if out is not None:
                state.end_restart(f"the assignment of the output `{lhs}`")
            return
        self.bind_value(toks, lhs, rhs, state, out)
        if out is not None and not out.array:
            self.check_identity(toks, out, rhs, state)
        if out is not None and lhs in state.facts:
            # A publication: unique dies
            state.facts[lhs] = without_site_facts(state.facts[lhs])

    def check_identity(self, toks: list[Token], out: Item, rhs: list[str],
                       state: _State) -> None:
        """F5: a moved output holds its input's value, through moves and
        locals; an owned or inline one a value made in the body.  NULL is no
        input's value and needs no identity"""
        origin = state.origins.get(out.name)
        if NULL_FACT in state.facts.get(out.name, frozenset()):
            return
        what = f"`{out.name}` is declared {out.mode or 'a pass-through'}"
        if out.source == HOME_SOURCE:
            if not (origin or "").startswith(HOME_SOURCE):
                self.fail(toks[0], f"`{out.name}` is declared moved(home), but is assigned "
                          f"`{' '.join(rhs)}`, which "
                          + ("differs by path" if origin is None
                             else "is no value loaded from a home"))
            return
        if out.mode == "moved" and origin != out.source:
            self.fail(toks[0], f"`{out.name}` is declared moved({out.source}), but is "
                      f"assigned `{' '.join(rhs)}`, which "
                      + (f"is `{origin}`'s value" if origin else
                         "is no input's value" if origin == "" else
                         "differs by path"))
        if out.mode in ("owned", "inline") and origin:
            # Decided on next (check_outputs): a move joined with a new owner
            # is owned; only a move on every path must say moved(x)
            self.moves[out.name] = (toks[0], f"{what}, but is assigned `{' '.join(rhs)}`, "
                                    f"which is the input `{origin}`'s value: declare "
                                    f"moved({origin})")

    def bind_value(self, toks: list[Token], lhs: str, rhs: list[str], state: _State,
                   out: Item | None) -> None:
        if len(rhs) == 1 and (rhs[0] in self.declared or rhs[0] in state.owners):
            # A move, `out = in;` or `PyValue tmp = in;`: the obligation goes
            # with the value, and the new holder must discharge it
            source = self.declared.get(rhs[0])
            if source is not None and source.mode in UNOWNED:
                if out is not None and out.mode == "moved":
                    self.fail(toks[0], f"on next, `{rhs[0]}` is moved, but it is declared "
                              f"{source.mode or 'a pass-through'}")
                if out is not None and out.mode == "owned":
                    self.fail(toks[0], f"`{lhs}` is declared owned, but `{rhs[0]}` is "
                              f"declared {source.mode or 'a pass-through'}: no owner moves")
                # A copy of a borrowed view: no obligation, the same facts
                state.assign(lhs, state.facts.get(rhs[0], frozenset()),
                             state.pending.get(rhs[0]), rhs[0])
                return
            self.take(rhs[0], state, toks[0], "moves")
            state.owners[lhs] = HELD
            self.new.add(lhs)
            state.assign(lhs, state.facts.get(rhs[0], frozenset()),
                         state.pending.get(rhs[0]), rhs[0])
            state.origins[lhs] = (rhs[0] if rhs[0] in self.declared
                                  else state.origins.get(rhs[0], ""))
            state.end_restart(f"the move of `{rhs[0]}`")
            return
        constant = self.table.get(rhs[0]) if len(rhs) == 1 else None
        if is_value_constant(constant):
            # A static immortal value: its declared facts, no count
            facts = implied(constant.result.facts | (
                set() if NULL_FACT in constant.result.facts else {NONNULL}))
            if NULL_FACT not in facts:
                facts = facts | {VFact("is", constant.name)}
            state.assign(lhs, facts, None, constant.name)
            if out is not None and out.mode == "owned":
                # An owned output's obligation, as for any inline value
                state.owners[lhs] = NULL if NULL_FACT in facts else HELD
                self.new.add(lhs)
            if out is not None:
                state.end_restart(f"the assignment of the output `{lhs}`")
            return
        if len(rhs) == 1 and rhs[0] in state.facts:
            # A local that holds no obligation: a borrowed or inline value,
            # or NULL.  Its facts and identity go with it
            facts = state.facts[rhs[0]]
            if (out is not None and out.mode == "owned" and INLINE not in facts
                    and NULL_FACT not in facts and out.source != HOME_SOURCE):
                self.fail(toks[0], f"`{lhs}` is declared owned, but `{rhs[0]}` holds no "
                          f"owner (from {state.sources.get(rhs[0], 'its assignment')}): "
                          "no owner moves")
            state.assign(lhs, facts, state.pending.get(rhs[0]),
                         state.sources.get(rhs[0], rhs[0]))
            state.origins[lhs] = state.origins.get(rhs[0], "")
            if out is not None and (out.mode == "owned" or out.source == HOME_SOURCE):
                state.owners[lhs] = NULL if NULL_FACT in facts else HELD
                self.new.add(lhs)
            if out is not None:
                state.end_restart(f"the assignment of the output `{lhs}`")
            return
        call = _call_of(_uncast(rhs), self.table)
        if call is None:
            if out is not None:
                state.end_restart(f"the assignment of the output `{lhs}`")
            return
        row, args = call
        result = row.result
        if result.shape == "HOME":
            # A home load: a borrowed copy of the home's value, whose facts
            # die at a home write and a reentry
            state.assign(lhs, frozenset(), None, row.name)
            state.origins[lhs] = _home_origin(self.home_key(_uncast(rhs)) or "")
            if out is not None and out.source == HOME_SOURCE:
                # The home's obligation, once the uop overwrites the home
                state.owners[lhs] = HELD
                self.new.add(lhs)
            if out is not None:
                state.end_restart(f"the assignment of the output `{lhs}`")
            return
        if result.mode == "moved":
            # VALUE(moved(k)): argument k itself, whose obligation the row
            # took (calls); it is the local's now
            arg = args[result.moved] if len(args) > result.moved else []
            # A view, or a CASES field (`pair.rest`), whose facts the
            # outcome states
            source = self.views_at(state).resolve(arg) or "".join(arg)
            owner = self.owner_of(arg, state)
            if owner is None:
                self.fail(toks[0], f"{row.name} moves `{' '.join(arg)}`, which holds no "
                          "obligation")
            state.owners[lhs] = HELD
            self.new.add(lhs)
            if result.shape == "VALUE_OR_NULL":
                # NULL or the argument's value: its facts once a test
                # shows it is not NULL
                state.assign(lhs, frozenset(),
                             state.facts.get(source, frozenset()) | {NONNULL}, owner)
            else:
                state.assign(lhs, state.facts.get(source, frozenset()),
                             state.pending.get(source), owner)
            state.origins[lhs] = (owner if owner in self.declared
                                  else state.origins.get(owner, ""))
            state.end_restart(f"the move of `{owner}` by {row.name}")
            return
        if result.shape == "CASES":
            # Its outcomes are walked one by one from the next statement
            state.split = (lhs, result)
            state.assign(lhs, frozenset(), None, row.name)
            return
        if result.shape in ("VALUE_OR_NULL", "STATUS"):
            # The owners this row takes are decided by its result's local
            for owner, status in state.owners.items():
                if status == TAKEN:
                    state.owners[owner] = TAKEN + lhs
        if result.shape == "VIEW" and any(a.mode in ("consumed", "taken") for a in row.args):
            # A stealing view, `o = PyValue_AsKnownObjectSteal(x)`: the row
            # consumed x, and its obligation is now the local's
            source = self.views_at(state).resolve(args[0]) if args else None
            state.owners[lhs] = HELD
            self.new.add(lhs)
            state.assign(lhs, state.facts.get(source, frozenset()), None, row.name)
            return
        if result.shape == "STATUS":
            # Its `out` arguments are written on success only: the status
            # local decides which (refine)
            state.status[lhs] = result.failure
            views = self.views_at(state)
            state.status_facts[lhs] = tuple(
                (name, facts) for arg, facts in zip(args, result.success)
                if facts and (name := views.resolve(arg)) is not None)
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
            source = self.views_at(state).resolve(args[k]) if len(args) > k else None
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
        if state.arrays[array] == EMPTY:
            self.fail(toks[0], f"stores into `{array}`, which has no element here: its "
                      "size test failed")
        declared = self.contract.output(array)
        value = _uncast(rhs)
        if len(value) == 1 and value[0] in state.owners:
            facts = state.facts.get(value[0], frozenset())
            item = self.declared.get(value[0])
            if item is not None and item.mode in UNOWNED:
                if INLINE not in facts:
                    self.fail(toks[0], f"stores `{value[0]}`, declared "
                              f"{item.mode or 'a pass-through'}, into the owned array "
                              f"`{array}`: no owner moves")
            else:
                self.take(value[0], state, toks[0], "moves")
        elif value == ["PyValue_NULL"]:
            # No value and no obligation: only a nullable array takes it
            facts = frozenset({NULL_FACT})
        else:
            call = _call_of(value, self.table)
            row = call[0] if call is not None else None
            moved = (row is not None and row.result.shape == "VALUE"
                     and row.result.mode == "moved")
            if row is None or row.result.shape not in ("VALUE", "VALUE_OR_NULL") \
                    or not (row.new_owner() or moved):
                self.fail(toks[0], f"stores `{' '.join(value)}` into the owned array "
                          f"`{array}`: an element is an owner moved in or a row's new "
                          "owner")
            if moved:
                # VALUE(moved(k)), TAKE(x): x's obligation, which the row
                # took (calls), moves into the element, with x's facts
                arg = call[1][row.result.moved] if len(call[1]) > row.result.moved else []
                if self.owner_of(arg, state) is None:
                    self.fail(toks[0], f"{row.name} moves `{' '.join(arg)}`, which holds "
                              "no obligation")
                facts = state.facts.get(self.views_at(state).resolve(arg) or "".join(arg),
                                        frozenset())
            else:
                facts = (implied(row.result.facts | {NONNULL})
                         if row.result.shape == "VALUE" and row.result.null_iff < 0
                         else implied(row.result.facts) - {NONNULL})
        if not declared.nullable and NONNULL not in implied(facts):
            self.fail(toks[0], f"stores `{' '.join(value)}`, which may be NULL, into "
                      f"`{array}`, whose elements are not nullable")
        missing = implied(declared.facts) - implied(facts)
        if missing:
            self.fail(toks[0], f"stores `{' '.join(value)}` into `{array}`, declared "
                      f"{', '.join(sorted(map(str, missing)))}, which it is not shown to be")
        size = declared.size
        whole = re.fullmatch(r"1|\(?oparg\s*&\s*1\)?", size.strip()) is not None
        state.arrays[array] = WRITTEN if whole else PARTIAL
        state.end_restart(f"the publication of `{array}`")


_WALKS: dict[tuple[int, bool], tuple[Uop, OwnershipWalk]] = {}


def walk_of(uop: Uop, table: dict[str, Row] | None = None,
            kills: bool = True) -> OwnershipWalk:
    "The walk of a declared uop's body, run"
    table = with_slots(uop, rows() if table is None else table)
    key = (id(uop), kills)
    if table is _ROWS and key in _WALKS:
        return _WALKS[key][1]
    walk = OwnershipWalk(uop, declared_contract(uop), table, kills).run()
    if table is _ROWS:
        _WALKS[key] = (uop, walk)
    return walk


# ---------------------------------------------------------------------------
# Result facts (V5), exits (V7) and exported guard facts (V9)

def _conditional(texts: list[str]) -> tuple[int, int] | None:
    """The indices of `?` and `:` if texts are `c ? a : b` outside any
    parentheses, with no other conditional among them"""
    depth, marks = 0, []
    for i, t in enumerate(texts):
        depth += {"(": 1, ")": -1, "[": 1, "]": -1}.get(t, 0)
        if depth == 0 and t in ("?", ":"):
            marks.append(i)
    if len(marks) == 2 and texts[marks[0]] == "?" and texts[marks[1]] == ":":
        return marks[0], marks[1]
    return None


def _assignments(uop: Uop, name: str) -> list[tuple[Token, list[str]]]:
    tokens = _tokens(uop)
    texts = [t.text for t in tokens]
    found = []
    for i, t in enumerate(texts):
        if t == "=" and i and texts[i - 1] == name and (i < 2 or texts[i - 2] not in ("->", ".")):
            end = texts.index(";", i)
            found.append((tokens[i - 1], texts[i + 1:end]))
    return found


def _value_results(uop: Uop, where: Token, rhs: list[str], table: dict[str, Row],
                   inputs: dict[str, Item], seen: frozenset[str] = frozenset()
                   ) -> list[tuple[frozenset[VFact], str, str]]:
    """What a value assigned to an output can be, as (facts, mode, source)
    for each alternative: a row's value, a constant, an input, the arms of
    a conditional, and through a local each value assigned to it (F3)"""
    rhs = _uncast(rhs)
    marks = _conditional(rhs)
    if marks is not None:
        q, c = marks
        return (_value_results(uop, where, rhs[q + 1:c], table, inputs, seen)
                + _value_results(uop, where, rhs[c + 1:], table, inputs, seen))
    call = _call_of(rhs, table)
    if call is not None and call[0].result.mode == "moved":
        row, args = call
        return _value_results(uop, where, args[row.result.moved], table, inputs, seen)
    if call is not None and call[0].result.shape in ("VALUE", "VALUE_OR_NULL"):
        result = call[0].result
        return [(implied(result.facts), result.mode, call[0].name)]
    if len(rhs) == 1 and rhs[0] in inputs:
        item = inputs[rhs[0]]
        # An assumed fact holds at entry (F7), as in the walk; a value that
        # is never an object owns nothing, so it moves as an inline one
        facts = implied(item.facts | {a.fact for a in item.assumed})
        entry = [(facts, "inline" if "object" not in word_kinds(facts) else "moved", rhs[0])]
        if rhs[0] in seen:
            return entry
        # The declaration describes the entry value only: a value the body
        # assigns to the input later is an alternative of its own
        return entry + [r for at, value in _assignments(uop, rhs[0])
                        for r in _value_results(uop, at, value, table, inputs,
                                                seen | {rhs[0]})]
    if len(rhs) == 1 and rhs[0].count(".") == 1:
        # A value field of a CASES result: what each outcome that is not
        # NULL gives (F6)
        local, f = rhs[0].split(".")
        for at, value in _assignments(uop, local):
            call = _call_of(value, table)
            if call is not None and call[0].result.shape == "CASES":
                specs = [spec for c in call[0].result.cases for name, spec in c.values
                         if name == f and spec.mode != "null"]
                if specs:
                    return [(implied(spec.facts | {NONNULL}), spec.mode, call[0].name)
                            for spec in specs]
    row = table.get(rhs[0]) if len(rhs) == 1 else None
    if is_value_constant(row):
        identity = set() if NULL_FACT in row.result.facts else {VFact("is", row.name)}
        return [(implied(row.result.facts | identity), "inline", row.name)]
    if len(rhs) == 1 and re.fullmatch(r"[A-Za-z_]\w*", rhs[0]) and rhs[0] not in seen:
        found = _assignments(uop, rhs[0])
        if found:
            return [r for at, value in found
                    for r in _value_results(uop, at, value, table, inputs, seen | {rhs[0]})]
    raise analysis_error(f"{uop.name}: `{' '.join(rhs)}` is assigned to an output, but it is "
                         "no row with a value result, constant, input, conditional of them "
                         "or local assigned them", where)


def check_results(uop: Uop, contract: Contract, table: dict[str, Row]) -> None:
    """V5: every assignment to an output gives its declared mode and facts,
    through locals and the arms of conditionals.  That they hold on every
    path to next, joins and kills included, and that a non-nullable output
    is not NULL there, is the walk's check (OwnershipWalk.check_outputs);
    so is a moved output's identity (check_identity)"""
    inputs = {i.name: i for i in contract.inputs}
    for out in contract.outputs:
        # A moved(home) output's value is a home's, which the walk checks
        if out.mode in ("", "borrowed", "moved") or out.source == HOME_SOURCE:
            continue
        for where, rhs in _assignments(uop, out.name):
            for gives, mode, source in _value_results(uop, where, rhs, table, inputs):
                if NULL_FACT in gives:
                    continue        # NULL: the walk holds it to nullable
                missing = sorted(str(f) for f in implied(out.facts) - gives - {NONNULL}
                                 if not satisfied(f, gives))
                if missing:
                    raise analysis_error(
                        f"{uop.name}: `{out.name}` is declared {', '.join(missing)}, but "
                        f"{source} gives {', '.join(sorted(map(str, gives))) or 'no fact'}",
                        where)
                if out.mode == "inline" and mode != "inline":
                    raise analysis_error(f"{uop.name}: `{out.name}` is declared inline, but "
                                         f"{source} gives an {mode} value", where)


SWITCH_EXITS = frozenset(("enter", "return", "yield"))


def body_exits(uop: Uop, table: dict[str, Row], implicit_guards: int,
               declared: frozenset[str] = frozenset()) -> set[str]:
    """The exits that uop's body takes: its sites' (the DSL's, its rows',
    `goto` an error label, `return` from the evaluator).  A body that
    switches frames with no site that names the switch (`frame = ...`)
    continues in the other frame in place of `next`, by the switch exits the
    uop declares: the body cannot say which, and nothing infers it"""
    exits = set()
    tokens = _tokens(uop)
    for i, tkn in enumerate(tokens):
        if tkn.kind == "GOTO" and tokens[i + 1].text in ERROR_LABELS:
            exits.add("raise")
        elif tkn.kind == "RETURN":
            exits.add("exit")
    for tkn, _ in body_calls(uop):
        if tkn.text in GUARDS:
            exits.add(GUARDS[tkn.text])
        elif tkn.text in RAISES:
            exits.add("raise")
        elif tkn.text == "DISPATCH_SAME_OPARG":
            exits.add("jump")
        elif tkn.text in table:
            row = table[tkn.text]
            exits |= row.exits - row.successor
            for exit in row.successor:
                # A successor assigned on some paths only: the word has two
                # successors, a branch (the record's BRANCH)
                exits.add("branch" if exit == "jump" and
                          ir_records.conditional(uop.body.body, tkn) else exit)
    if implicit_guards:
        exits.add("miss")
    switches = analyzer.frame_switch_sites(list(uop.body.tokens()))
    if switches:
        if not exits & SWITCH_EXITS:
            exits |= declared & SWITCH_EXITS
        # A switch on some paths only (`if (...) { IR_ENTER_GENERATOR(...); }`)
        # leaves the others to run to the end of the body
        if (not uop.properties.always_exits
                and any(ir_records.conditional(uop.body.body, site) for site in switches)
                and not any(ir_records.conditional(uop.body.body, site) is False
                            for site in switches)):
            exits.add("next")
    elif not uop.properties.always_exits:
        # A body that moves next_instr and runs to its end continues where
        # it moved it (_FOR_ITER_DICT_ITEMS steps over the unpack word): a
        # jump, on the paths that move it
        moves = [conditional for tkn, written in position_writes(tokens)
                 if written == "next_instr"
                 for conditional in [ir_records.conditional(uop.body.body, tkn)]]
        if moves:
            exits.add("jump")
        if not any(c is False for c in moves):
            exits.add("next")
    return exits


def check_exits(uop: Uop, contract: Contract, table: dict[str, Row],
                implicit_guards: int) -> None:
    """V7: the declared exits are the body's; VT1: a uop declares
    switches_frame exactly when it declares enter, return or yield"""
    switches = "switches_frame" in contract.effects
    if switches != bool(contract.exits & SWITCH_EXITS):
        raise analysis_error(
            f"{uop.name}: " + (f"declares switches_frame, but none of the exits "
                               f"{', '.join(sorted(SWITCH_EXITS))}" if switches else
                               f"declares exits({', '.join(sorted(contract.exits & SWITCH_EXITS))}), "
                               "but not the effect switches_frame"), uop.body.open)
    actual = body_exits(uop, table, implicit_guards, contract.exits)
    if actual != set(contract.exits):
        raise analysis_error(f"{uop.name}: declares exits({', '.join(sorted(contract.exits))}), "
                             f"but the body takes {', '.join(sorted(actual))}", uop.body.open)
    check_transfer_exits(uop, contract, table)


# The exits after which the word goes on in its own frame
COMPLETING_EXITS = frozenset(("next", "branch", "jump"))
GUARD_EXITS = frozenset(("miss", "deopt"))
# The exits that leave the frame for good: after one, its code may die
DEPARTING_EXITS = frozenset(("return", "yield"))


def check_transfer_exits(uop: Uop, contract: Contract, table: dict[str, Row]) -> None:
    """VT3: a uop that retires a frame (it declares retires_frame, as each
    row it calls that retires one makes it) declares return.  VT4: a uop
    does not both guard and depart; its guards go in their own uop, before
    the pin.  VT5: a uop that may also complete in its frame switches only
    through a row that declares the switch; there `DISPATCH()` is the
    completion, and an assignment of `frame` says nothing"""
    retires = "retires_frame" in contract.effects
    if retires and "return" not in contract.exits:
        raise analysis_error(f"{uop.name}: retires a frame (retires_frame), and does not "
                             "declare exits(return)", uop.body.open)
    guards = contract.exits & GUARD_EXITS
    departs = contract.exits & DEPARTING_EXITS | ({"retires_frame"} if retires else set())
    if guards and departs:
        raise analysis_error(f"{uop.name}: declares exits({', '.join(sorted(guards))}) and "
                             f"departs ({', '.join(sorted(departs))}): split the guards into "
                             "their own uop, which runs before the pin", uop.body.open)
    switches = contract.exits & SWITCH_EXITS
    if switches and contract.exits & COMPLETING_EXITS:
        by_rows = set()
        for tkn, _ in body_calls(uop):
            if tkn.text in table:
                by_rows |= table[tkn.text].exits & SWITCH_EXITS
        if switches - by_rows:
            raise analysis_error(
                f"{uop.name}: completes in its frame and switches by "
                f"{', '.join(sorted(switches - by_rows))}, which no row it calls declares: "
                "take the switch through a row that declares it (DISPATCH() here is the "
                "completion)", uop.body.open)


# The exits that leave a word at the position its body wrote
POSITION_EXITS = frozenset(("jump", "redirect"))
# The exits whose continuation reads frame->instr_ptr: the unwinder looks up
# the handler at it (exception_unwind), and a suspended frame resumes at it
INSTR_PTR_EXITS = frozenset(("raise", "yield"))
POSITION_WRITES = frozenset(("=", "+=", "-=", "++", "--"))


def position_writes(tokens: list[Token]) -> list[tuple[Token, str]]:
    "The sites where tokens write next_instr or frame->instr_ptr, with which"
    sites = []
    for i, tkn in enumerate(tokens):
        after = tokens[i + 1].text if i + 1 < len(tokens) else ""
        before = tokens[i - 1].text if i > 0 else ""
        written = after in POSITION_WRITES or before in ("++", "--")
        if tkn.text == "next_instr" and written:
            sites.append((tkn, "next_instr"))
        elif (tkn.text == "instr_ptr" and before == "->" and i > 1
                and tokens[i - 2].text == "frame" and written):
            sites.append((tokens[i - 2], "frame->instr_ptr"))
    return sites


def check_position_writes(uop: Uop, contract: Contract) -> None:
    """V12: a body that writes the frame's position (next_instr or
    frame->instr_ptr) declares an exit that leaves there, jump or redirect.
    Two writes are allowed because a declared exit fixes their destination:
    - next_instr after a frame switch is where the switch exit (yield,
      enter, return) continues, in the frame switched to: _YIELD_VALUE's
      `next_instr = frame->return_pc`, the caller's resume point;
    - frame->instr_ptr in a uop whose every exit is raise or yield is where
      that exit continues: the unwinder looks up the handler at it
      (exception_unwind; RERAISE), and the suspended frame resumes at it
      (_YIELD_VALUE's `frame->instr_ptr++`)"""
    switches = [s.begin for s in analyzer.frame_switch_sites(list(uop.body.tokens()))]
    exits = set(contract.exits)
    for tkn, written in position_writes(_tokens(uop)):
        if exits & POSITION_EXITS:
            continue
        if written == "next_instr" and any(at < tkn.begin for at in switches):
            continue
        if written == "frame->instr_ptr" and exits <= INSTR_PTR_EXITS:
            continue
        raise analysis_error(
            f"{uop.name}: writes {written}, but declares exits({', '.join(sorted(exits))}), "
            "none of which leaves there: declare jump or redirect", tkn)


def check_completion_targets(uop: Uop) -> None:
    """V13: IR_COMPLETE_STEP's target is an operand of the word, oparg or a
    declared cache operand (IR_EXIT names the word's exit, which is oparg).
    The generator lowers it as the word's normal completion; a local could
    hold any position"""
    operands = {"oparg", "IR_EXIT"} | {c.name for c in uop.caches if c.name != "unused"}
    for tkn, args in body_calls(uop):
        if tkn.text != "IR_COMPLETE_STEP":
            continue
        target = args[2] if len(args) == 3 else []
        if len(target) != 1 or target[0] not in operands:
            raise analysis_error(
                f"{uop.name}: IR_COMPLETE_STEP's target is `{' '.join(target)}`, which is "
                "no operand: pass oparg or a declared cache operand", tkn)


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
        entry = (implied(item.facts | {a.fact for a in item.assumed}
                         | (set() if item.nullable else {NONNULL}))
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


def check_state_facts(uop: Uop, contract: Contract, walk: OwnershipWalk) -> None:
    """V9 for a handler's state and the frame paths: what holds of each at
    the next exit is what the uop declares there"""
    items = [(n, req, decl) for n, req, decl in contract.state] + \
            [(n, frozenset(), decl) for n, decl in contract.paths]
    states = {n for n, _, _ in contract.state}
    for name, required, declared in items:
        live = walk.exported(name) - {NONNULL}
        if name in states:
            live = frozenset(f for f in live if f.kind in STATE_FACTS)
        declared, required = implied(declared) - {NONNULL}, implied(required) - {NONNULL}
        extra = declared - live
        if extra and walk.exit is not None:
            raise analysis_error(
                f"{uop.name}: `{name}` is declared {', '.join(sorted(map(str, extra)))}, "
                "which nothing establishes on every path to next", uop.body.open)
        missing = live - declared - required
        if missing:
            raise analysis_error(
                f"{uop.name}: the body establishes {', '.join(sorted(map(str, missing)))} "
                f"of `{name}`, which the uop does not declare", uop.body.open)
    # A frame path's facts that the body establishes are declared
    if walk.exit is not None:
        frame_paths = {r.result.measure for r in walk.table.values()
                       if r.result.shape == "PATH" and not r.args}
        inputs = {i.name for i in uop.stack.inputs}
        input_paths = {n for n in walk.exit.facts
                       if "." in n and n.split(".", 1)[0] in inputs}
        for name in sorted((frame_paths | input_paths) - {n for n, _ in contract.paths}):
            live = walk.exported(name) - {NONNULL}
            if live:
                kind = "path" if name in input_paths else "frame path"
                raise analysis_error(
                    f"{uop.name}: the guards establish {', '.join(sorted(map(str, live)))} "
                    f"of the {kind} `{name}`, which paths(...) does not declare",
                    uop.body.open)


def check_state_reads(uop: Uop, contract: Contract, table: dict[str, Row]) -> None:
    """A STATE item is read only by a uop whose state(...) clause declares
    it, and never addressed: its writes are the declared ones"""
    tokens = _tokens(uop)
    for i, t in enumerate(tokens):
        row = table.get(t.text)
        if row is None or row.kind != "STATE":
            continue
        if i and tokens[i - 1].text == "&" and (
                i < 2 or not re.fullmatch(r"\w+|\)|\]", tokens[i - 2].text)):
            raise analysis_error(f"{uop.name}: `&{t.text}` addresses a handler's state, "
                                 "whose writes must be the declared ones", t)
        if contract.state_item(t.text) is None:
            raise analysis_error(f"{uop.name}: reads `{t.text}`, a handler's state, which "
                                 "no state(...) clause declares", t)


def owes_nothing_by_type(facts, table: dict[str, Row]) -> bool:
    """whether facts show an exact type whose every instance is a static
    immortal CONSTANT (its row's INSTANCES): bool and NoneType"""
    types = _exact_types(implied(facts))
    return bool(types) and all(
        table.get(t) is not None and table[t].instances for t in types)


def is_value_constant(row: Row | None) -> bool:
    "whether row is a CONSTANT held as a PyValue (V8's roots)"
    return row is not None and row.kind == "CONSTANT" and row.representation == "VALUE"


def check_identities(uop: Uop, contract: Contract, table: dict[str, Row]) -> None:
    """is(K) names a CONSTANT row's value, which is not NULL (that is
    `null`), in the uop's declaration and in the rows it calls"""
    facts = [(f, uop.body.open) for i in contract.inputs + contract.outputs for f in i.facts]
    for tkn, _ in body_calls(uop):
        row = table.get(tkn.text)
        if row is not None:
            facts += [(f, tkn) for a in row.args for f in a.facts]
            facts += [(f, tkn) for f in row.result.facts]
            facts += [(f, tkn) for t in row.result.tests for side in t for f in side]
    for f, where in facts:
        if f.kind != "is":
            continue
        constant = table.get(f.arg)
        if constant is None or constant.kind != "CONSTANT":
            raise analysis_error(f"{uop.name}: {f} names `{f.arg}`, which is no CONSTANT row",
                                 where)
        if constant.representation == "VALUE" and NULL_FACT in constant.result.facts:
            raise analysis_error(f"{uop.name}: {f}: NULL is no identity; it is `null`", where)


_VERIFIED: set[int] = set()


def argument_facts(arg: list[str], table: dict[str, Row]) -> frozenset:
    """The facts of an argument that is a row's call: its result's facts,
    with nonnull for a VALUE that is never NULL; none for anything else"""
    call = _call_of(_uncast(arg), table)
    if call is None:
        return frozenset()
    result = call[0].result
    if result.shape == "VALUE" and result.null_iff < 0:
        return implied(result.facts | {NONNULL})
    if result.shape in ("VALUE", "VALUE_OR_NULL"):
        return implied(result.facts) - {NONNULL}
    return frozenset()


def assumed_result_of(texts: list[str], table: dict[str, Row]) -> tuple[str, Row] | None:
    """`[T] x = R(...);`, whose value is the call of a row R with assumed
    result facts: (x, R).  The generator asserts them of x after the
    statement (F7)"""
    texts = [t for t in texts if t != ";"]
    if texts.count("=") != 1:
        return None
    eq = texts.index("=")
    call = _call_of(texts[eq + 1:], table)
    if eq == 0 or call is None or not call[0].result.assumed:
        return None
    return texts[eq - 1], call[0]


def assumed_assertions(uop: Uop, texts: list[str], table: dict[str, Row] | None = None
                       ) -> list[str]:
    """The assertions the generator emits for a declared uop: of its
    assumed inputs at entry (texts None), or after a statement that binds
    an assumed row result (F7)"""
    contract = declared_contract(uop)
    if texts is None:
        return [assertion(a.fact, i.name) for i in contract.inputs for a in i.assumed]
    found = assumed_result_of(texts, rows() if table is None else table)
    if found is None:
        return []
    name, row = found
    return [assertion(a.fact, name) for a in row.result.assumed]


def check_assumed_results(uop: Uop, table: dict[str, Row]) -> None:
    """F7: a row's assumed result is bound to a local by the statement that
    calls it, `[T] x = R(...);`, so that every form asserts it there; an
    `assert` in the body never declares a fact"""
    bound: set[int] = set()

    def visit(stmt) -> None:
        if not isinstance(stmt, SimpleStmt):
            return
        tokens = [t for t in stmt.contents if t.kind != "COMMENT"]
        found = assumed_result_of([t.text for t in tokens], table)
        if found is not None:
            # The call is the whole right side: its name follows `=` and casts
            call = next(t for t in tokens[[t.text for t in tokens].index("=") + 1:]
                        if t.text == found[1].name)
            bound.add(id(call))

    uop.body.accept(visit)
    for t, _ in body_calls(uop):
        row = table.get(t.text)
        if row is not None and row.result.assumed and id(t) not in bound:
            raise analysis_error(
                f"{uop.name}: {row.name}'s result is assumed "
                f"{', '.join(f'{a.fact} by {a.by}' for a in row.result.assumed)}; "
                f"bind it to a local, `x = {row.name}(...);`, where the generator "
                "asserts it", t)


def check_fact_subjects(uop: Uop, contract: Contract) -> None:
    """A declared fact's subjects: in_bounds(x) names an input; a fact of
    one site (unique) is never declared on an output, which exports it"""
    where = uop.body.open
    for o in contract.outputs:
        for f in sorted(o.facts):
            if f.kind in SITE_FACTS:
                raise analysis_error(f"{uop.name}: `{o.name}` is declared {f}, a fact of "
                                     "one site, which is never exported", where)
    for item in contract.inputs + contract.outputs:
        for f in sorted(item.facts):
            if f.kind == "in_bounds" and contract.input(f.arg) is None:
                raise analysis_error(f"{uop.name}: `{item.name}` is declared {f}, but "
                                     f"`{f.arg}` is not an input", where)


def verify_uop(uop: Uop, implicit_guards: int = 0, table: dict[str, Row] | None = None) -> None:
    "Check a declared uop's body against its contract (V1-V5, V7, V9; V6 inside it)"
    table = with_slots(uop, rows() if table is None else table)
    contract = declared_contract(uop)
    if (id(uop), implicit_guards) in _VERIFIED:
        return
    if any(o.exchange for o in contract.outputs):
        verify_exchange(uop, contract)
        return
    check_slots(uop, table)
    check_state_reads(uop, contract, table)
    check_fact_subjects(uop, contract)
    check_assumed_results(uop, table)
    check_preprocessor(uop)
    check_callees(uop, table)
    check_effects(uop, contract, table)
    check_feedback(uop, contract, table)
    check_feedback_pointers(uop, table)
    check_guard_conditions(uop, table)
    check_convention_guards(uop, table)
    check_identities(uop, contract, table)
    for tkn, _ in body_calls(uop):
        if tkn.text in GUARDS:
            site_facts(uop, tkn, table)     # a condition without facts needs a reason
    check_results(uop, contract, table)
    walk = walk_of(uop, table)
    check_exits(uop, contract, table, implicit_guards)
    check_position_writes(uop, contract)
    check_completion_targets(uop)
    check_guard_facts(uop, contract, walk)
    check_state_facts(uop, contract, walk)
    if table is _ROWS:
        _VERIFIED.add((id(uop), implicit_guards))


# ---------------------------------------------------------------------------
# A handler (V6 across uops, V8, V10)

REGION_KINDS = ("inline", "split", "shared")


def check_transfer_order(inst: Instruction, parts: list[Uop],
                         contracts: list[Contract]) -> None:
    """VT2: a uop that switches frames and is followed by another uop of the
    arm declares a completing exit (next, branch, jump), on which alone the
    rest runs; its switch exits bind to SWITCH or to another frame.  VT4:
    no guard (miss, deopt) follows a uop that switches or departs, nor the
    pin, which follows the last guard"""
    departed = None
    for index, (part, contract) in enumerate(zip(parts, contracts)):
        if departed is not None and contract.exits & GUARD_EXITS:
            raise analysis_error(
                f"{inst.name}: {part.name} guards "
                f"({', '.join(sorted(contract.exits & GUARD_EXITS))}) after {departed}, "
                "which switches or departs: a guard after a pin or a switch cannot "
                "restart the word", inst.where)
        switches = "switches_frame" in contract.effects
        if (switches and index + 1 < len(parts)
                and not contract.exits & COMPLETING_EXITS):
            raise analysis_error(
                f"{inst.name}: {part.name} switches frames, and {parts[index + 1].name} "
                "follows it, but it declares no completing exit (next, branch, jump) "
                "on which the rest runs", inst.where)
        if (switches or contract.exits & DEPARTING_EXITS
                or "retires_frame" in contract.effects) and departed is None:
            departed = part.name


def verify_handler(inst: Instruction, table: dict[str, Row] | None = None,
                   leaf: dict[str, bool] | None = None,
                   temporaries: bool = False) -> None:
    """In a handler whose uops all declare contracts: each fact that a uop
    requires of an input is live where it runs (V8), with heap facts dying
    at every KILLS effect and an unestablished `object` met by the
    generator's implicit guard; no guard follows a uop after which the word
    cannot restart (V6).  A declared region kind fits the uops (V10).
    leaf maps uop names to whether the generator made them leaf.
    temporaries: the word's format holds every input in a temporary of its
    window, never in a local home."""
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
    contracts = [declared_contract(p) for p in parts]
    if not parts:
        return
    check_transfer_order(inst, parts, contracts)
    # A stack of value numbers; facts by value.  An array is one value, its
    # elements' common facts, with its declared size
    stack: list[int] = []
    facts: dict[int, frozenset[VFact]] = {}
    sizes: dict[int, str] = {}
    fresh = iter(range(1, 1 << 20))
    restartable, why = True, ""
    # The values that reach the handler, and the first site since its entry
    # that may have replaced any of them in its home (F7's binding).  A local
    # home may be written by any reentry (a finalizer through
    # FrameLocalsProxy).  A temporary only by a debugger's f_lineno jump
    # (frame_lineno_set_impl), and every successful jump redirects:
    # - from a callback of this frame's PY_RESUME, JUMP, BRANCH*, LINE or
    #   PY_YIELD event, whose uops leave by their redirect exit (or by
    #   branch, committed), so no uop of the word runs after it;
    # - from a descendant (sys._debugger_entry, an observer's ancestor
    #   frame), only while this frame's word is a CALL or CALL_KW whose
    #   Python child is not external: the word left its handler through
    #   `enter`, and the return goes to the redirect's target
    #   (pending_redirect, IR_STORE_RETURN), not to the rest of the word.
    # So on the next path no reentry or close replaces a temporary: the
    # values made in the handler, and with `temporaries` its inputs, keep
    # their facts, but for heap facts, which effects kill.
    entry: set[int] = set()
    replaced = ""

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
        entry.add(v)
        return v

    # Whether the stack's depth is known: past an array of unknown size the
    # fact walk (V8) stops, and restartability and the uops' own checks go on
    known = True
    # The facts of each STATE item (call_state): at entry the word's, which
    # no uop has shown; a uop that declares the item requires its facts and
    # leaves those it declares.  Neither heap kills nor reentry touch them
    state_facts: dict[str, frozenset[VFact]] = {}
    for index, (part, contract) in enumerate(zip(parts, contracts)):
        for name, required, declared in contract.state:
            missing = implied(required) - {NONNULL} - state_facts.get(name, frozenset())
            if missing:
                raise analysis_error(
                    f"{inst.name}: {part.name} requires "
                    f"{', '.join(sorted(map(str, missing)))} of the handler's `{name}`, "
                    "which nothing before it establishes", inst.where)
            texts = [t.text for t in _tokens(part)]
            writes = any(t == name and texts[i + 1:i + 2] in (["="], ["|="], ["&="])
                         for i, t in enumerate(texts))
            kept = frozenset() if writes else state_facts.get(name, frozenset())
            state_facts[name] = kept | (implied(declared) - {NONNULL})
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
            if item.assumed:
                # F7: a fact of the value that reaches the handler, from its
                # producer; one this handler makes is shown, not assumed
                said = ", ".join(f"{a.fact} by {a.by}" for a in item.assumed)
                if value not in entry:
                    raise analysis_error(
                        f"{inst.name}: {part.name} assumes {said} of `{item.name}`, a value "
                        "made in this handler, whose facts must show it", inst.where)
                since = "" if temporaries else replaced
                if since:
                    raise analysis_error(
                        f"{inst.name}: {part.name} assumes {said} of `{item.name}`, which "
                        f"may have been replaced since the handler's entry, at {since}",
                        inst.where)
                facts[value] = implied(facts[value] | {a.fact for a in item.assumed})
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
        # reentry every value that reached the handler and may be left in a
        # local home (the stack below the uop and its pass-throughs) may have
        # been replaced there, so all of its facts die; a temporary keeps
        # them (above).  Then the facts the uop declares on its outputs, which
        # V9 holds to the facts live at its exit.
        # Effects confined to other exits are on no path to next
        effects = contract.next_effects()
        if effects & KILLS:
            facts = {v: without_heap_facts(f) for v, f in facts.items()}
        if effects & REENTRY and known and not temporaries:
            homes = set(stack) | {bound[i.name] for i in contract.inputs if not i.mode
                                  and contract.output(i.name) is not None}
            for v in homes & entry:
                facts[v] = frozenset()
        if effects & REENTRY and not replaced:
            replaced = f"{part.name}, which may reenter"
        changes = effects - {"reads", "raises"}
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
                # A close may run a finalizer, which can replace any home's
                # value (design.md 3.2)
                facts = {v: frozenset() if v in entry and not temporaries
                         else without_heap_facts(f) for v, f in facts.items()}
                replaced = replaced or f"the skeleton's close of `{name}` after {part.name}"
                if restartable and contract.input(name) is not None:
                    restartable = False
                    why = f"the skeleton's close of `{name}` after {part.name}"
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
    each closed input is closed after its uop; after the word's last uop
    the outputs are published to their homes (IR_STORE_DST) and each
    displaced old value is closed (ir_old)"""
    steps = [("detach", i.name) for i in contract.inputs
             if i.mode in ("consumed", "taken") and i.name != "unused"]
    # A closed input is closed after its uop, on next as on raise
    steps += [("close", i.name) for i in contract.inputs
              if i.mode == "closed" and i.name != "unused"]
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
    table = with_slots(uop, rows())
    walk = walk_of(uop, table, kills=False)
    views = Views(uop, table)
    values = {i.name: f"v{i.value}" for i in inputs}

    def recorded(name: str, facts) -> list[Fact]:
        # A local's fact is recorded on the local, as the inference did
        live = walk.exit.facts.get(name, frozenset()) if walk.exit else frozenset()
        root, dot, rest = name.partition(".")
        subject = values.get(root, root) + dot + rest
        return [Fact(FACT_KIND[f.kind], subject, values.get(f.arg, f.arg)) if f.kind == "in_bounds"
                else f.record(subject) for f in facts if f in live]

    row = table.get(site.text)
    if row is not None and row.result.shape == "GUARD":
        # A GUARD row's facts of its arguments, on its return (F2)
        texts = [t.text for t in _tokens(uop)]
        i = _tokens(uop).index(site)
        args = _call(texts[i:_close(texts, i + 1) + 1], site.text) or []
        result = []
        for arg, facts in zip(args, row.result.guards):
            name = views.resolve(arg)
            facts = _bind_facts(facts, args, views)
            if name is not None and facts:
                result += recorded(name, sorted(facts))
        return ir_records.join_encodings(result), ""
    cond, reason = guard_condition(uop, site)
    found = condition_facts(cond, views, table, holds=False)
    if found is None:
        site_facts(uop, site, table)        # refuses a condition without a reason
        return (), reason
    stated = _stated_facts(cond, views, table, False)
    result = []
    for name, facts in stated.items():
        result += recorded(name, facts)
    return ir_records.join_encodings(result), reason or found[1]


def _stated_facts(cond: list[str], views: Views, table: dict[str, Row],
                  holds: bool) -> dict[str, list[VFact]]:
    """The facts as the TEST and MEASURE rows of cond state them, in the
    order of the condition and their arguments, without implications"""
    found = condition_facts(cond, views, table, holds) or ({}, "")
    result: dict[str, list[VFact]] = {}
    # The bits of STATE items, `call_state & CALL_SELF_PRESENT`
    for name, facts in found[0].items():
        if name in table and table[name].kind == "STATE":
            result[name] = sorted(f for f in facts if f.kind in STATE_FACTS)
        # A comparison with an object CONSTANT's address, `x == &K`
        for f in sorted(facts):
            row = table.get(f.arg) if f.kind == "is" else None
            if row is not None and row.representation == "OBJECT" and "&" in cond:
                result.setdefault(name, []).append(f)
            # The NULL test of a pointer along an input path
            if "." in name and f in (NULL_FACT, NONNULL) and "NULL" in cond \
                    and f not in result.get(name, []):
                result.setdefault(name, []).append(f)
            # A comparison of a type with a static type's address,
            # `Py_TYPE(x) != &T`
            if f.kind == "exact" and f"{f.arg}" in cond and "&" in cond and \
                    {"Py_TYPE", "PyValue_TYPE"} & set(cond) and f not in result.get(name, []):
                result.setdefault(name, []).append(f)
    for tkn_text, args in _calls_in_texts(cond):
        row = table.get(tkn_text)
        if row is None:
            continue
        for arg, (true, false) in zip(args, row.result.tests):
            name = views.resolve(arg)
            if name is None:
                continue
            sides = set(true | false)
            have = found[0].get(name, frozenset())
            if any(f.kind == EXCLUDES for f in false):
                # A `rest` side, where the test failed, states what remains:
                # the value's plain words, which the record joins in one
                # ENCODING
                sides = set(true) if true <= have else {
                    f for f in have if f.kind in SPELLINGS}
            for f in sorted(sides):
                if f in found[0].get(name, ()) and f not in result.get(name, []):
                    result.setdefault(name, []).append(f)
        if row.result.shape == "MEASURE":
            name = views.resolve(args[0])
            for f in sorted(found[0].get(name, ())):
                if f.kind == row.result.measure and f not in result.get(name, []):
                    result.setdefault(name, []).append(f)
    # Comparisons of a local bound to a MEASURE read, `conv != M`
    for local in sorted(set(cond) & set(views.measures)):
        read = views.field([local])
        if read is None:
            continue
        for f in sorted(found[0].get(read[1], ())):
            if f.kind == read[0].result.measure and f not in result.get(read[1], []):
                result.setdefault(read[1], []).append(f)
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
    contract = declared_contract(uop)
    by_name = {o.name: o for o in outputs}
    result = []
    for item in contract.outputs:
        out = by_name.get(item.name)
        if out is not None and item.mode in ("owned", "inline"):
            result += [f.record(f"v{out.value}") for f in sorted(item.facts)]
    return ir_records.join_encodings(result)
