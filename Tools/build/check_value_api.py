"""Check the value API rules of value-native units.

The rules are those of pyjit/design/value-api.md, section 4. A value-native
unit is one built with Py_BUILD_CORE and neither Py_BUILD_CORE_BUILTIN nor
Py_BUILD_CORE_MODULE, or one that defines Py_VALUE_CORE (Include/pyport.h).

* variadic: a PyValue or a PyValueBorrow, or a pointer to one, passed to a
  variadic argument. Neither GCC nor Clang warns, and a value at %R, or a
  PyValue * where PyArg_ParseTuple() writes a PyObject *, is undefined
  behaviour. The one exception is a value conversion (%vR, %vS, %vA, %vT,
  %vN) of a formatting function whose format is a string literal. A value
  conversion reads its argument, so it takes a PyValueBorrow
  (va_arg(*vargs, PyValueBorrow) in Objects/unicodeobject.c): the check
  requires every value conversion to receive one, and reports an owner
  there, which the caller passes as PyValue_Borrow(v).
* slot-call: a direct call through an object slot field that has a value form
  (tp_as_number->nb_add, tp_richcompare, tp_iternext, tp_call and the rest).
  Calls in a family in MARKED_SLOT_FAMILIES fail. Mere reads, comparisons,
  copies and indirect calls are listed as non-failing slot-read findings.
  Units that define Py_VALUE_ADAPTER are skipped.
* raw-bits: knowledge of the word's encoding outside the code that may have
  it (value-api.md, section 1.6, rule 1: ENCODING_FILES and
  ENCODING_FUNCTIONS). In a value-native unit: a .bits or ->bits access of a
  PyValue, PyValueBorrow or _PyValueGuarded; a brace initializer of one
  ({x.bits}, (PyValue){...}) other than {0}, which zero-initializes; a
  cast to a word pointer from anything but void *, a byte pointer or the
  same type, or from a word pointer to anything but those; a union with a
  word, an array of words or a struct holding one. Byte pointers are
  allowed because a word's address is often computed as a byte offset (a
  member offset, a PyMem_* block); so that a second cast cannot launder the
  first, a cast to a word pointer from the address of a non-word object, or
  from a cast of a pointer to a non-byte, non-word type, fails, and so does
  a cast to a non-byte, non-word pointer from a cast of a word pointer. A
  match belongs to the files where its first and last tokens are written:
  a match in the body of a macro that one of those files defines is
  theirs, one in a macro's argument is the caller's. In the text of every
  C file of the tree: an encoding constant (ENCODING_NAMES), a (PyValue){
  or (PyValueBorrow){ literal and, in a header or a unit that clang-query
  does not check, a .bits or ->bits access; the text covers the macro
  bodies that no unit expands.
* public-value: the public headers (Include/ outside Include/internal/)
  carry the word opaquely (rule 2). A struct field may name PyValue or
  PyValueBorrow, directly or through a typedef, only if it is one of the
  five frozen word fields (PUBLIC_WORD_FIELDS): no other word field, and no
  typed value signature in a public layout. Value slots are opaque
  _Py_valuefunc storage, which the rule accepts
  (pyjit/design/proposals/value-api-v2a0.md, D2). An inline function may
  return a word but not take one, and no function or macro body names a
  value type: an inline accessor may only copy a word out of a field. No
  file-scope declaration other than a prototype or a typedef names one.
  Comments and string literals are skipped; macro bodies are read.
* window-read: in a function that returns PyValue and takes a
  `const PyValue *args` window (the value vectorcall shapes), an args[i]
  that is not an operand of PyValue_Borrow(), PyValue_DUP() or a
  _PyValue_Window*() primitive (value-api-m3.md, section 1.1). A text rule.
* vectorcall-thunk: _PyValue_VectorcallThunk named outside the creators
  and Objects/call.c, or _PyFunction_Vectorcall outside the files that
  compare against it (THUNK_FILES, FUNCTION_VECTORCALL_FILES; section 1.7).
  A text rule.
* borrow-field: a field, global, static or array of type PyValueBorrow. A
  borrow lives only in C locals (pyjit/design/proposals/register-ir-v2-a5b.md,
  L1 and R2-S1).
* home-copy: a PyValue local initialised from a home (a field, an element
  or *p). It is a move if the code stores that home afterwards; otherwise it
  should be a PyValueBorrow (register-ir-v2-a5b.md, section 5.3). Reported,
  not failing.

* slot-write: an assignment through a pointer to an object slot field of
  SLOT_FAMILIES in Objects/ or Python/, outside the type-construction
  functions (SLOT_WRITE_FUNCTIONS). After PyType_Ready the hook rewrite
  and slot_tp_descr_get change an object slot through _PyType_WriteSlotPair(),
  which writes its value field beside it, and update_one_slot() writes both
  fields through slotptr() (pyjit/design/proposals/value-api-m1b-m2.md, A7). The
  check is syntactic: writes through an offset (update_one_slot(),
  PyType_FromMetaclass()) are out of its reach, and in Modules/, where
  writes before ready are normal, the debug check of the pairs
  (value-api-v2a0.md, D6) is the only one.
* pair-table: a function body in Objects/value_pairs.c. That unit holds
  only the pair table, and is the one unit where Py_VALUE_COMPAT is off, so
  that a marked function's address may be taken there (A8).

* compat-body: a box primitive or box helper (BOX_PRIMITIVES) called
  directly in the body of a function that Py_VALUE_COMPAT marks. Such a
  function is a thunk of a value dispatcher
  (pyjit/design/proposals/value-api-v2a0.md, D10), and its boxes are of
  kind THUNK: it boxes through _PyValue_THUNK_EXIT()
  (Include/internal/pycore_value_slots.h). The rule checks a thunk once its
  declaration is marked, which the waves do (W1b, W2); until then it has
  nothing to check.

* deferred: a value-native unit that defines Py_VALUE_DEFER_MARKS, so that
  Py_VALUE_COMPAT marks nothing there (Include/pyport.h): the defer line of
  a bulk wave (pyjit/design/proposals/value-api-v2a0.md, section 1.5).
  Reported, not failing; the R27 gate needs none.

* compat-body (second pass, -DPy_STATS, ENTRY_UNITS): a core box form
  (PyValue_AsObject, _PyValueTemps_Object,
  _PyValue_BoxInlineValue) or _PyValue_AdapterBox in the body of a
  Py_VALUE_ENTRY function.
* compat-capture (second pass): __builtin_return_address, or a call of
  _PyValue_THUNK_EXIT, outside the body of a Py_VALUE_ENTRY function, and a
  call from one such function to another.
* test-site, boundary-site (text, and the box-kind matcher of the second
  pass): value-api-g.md, section 1.7; see BOX_TEXT_RULES.

The functions that Py_VALUE_COMPAT marks need no other rule here: calling
one from a value-native unit is a compile error, including the int and float
extractors (PyLong_AsLong, PyFloat_AsDouble, ...).

The script reads compile_commands.json, which intercept-build records while
the units compile, and runs clang-query over every value-native unit, then
the text rules over the tree. Before that it runs its self-tests: units
that must make each rule fire (SELF_TESTS, ENTRY_TESTS, RAW_BITS_TEXT_TESTS,
BOX_TEXT_TESTS and PUBLIC_VALUE_TESTS), a unit that calls the marked probe declaration in
Include/internal/pycore_value_api.h, which must fail to compile, and the
typed-borrow units (BORROW_TESTS): each misuse of a PyValueBorrow must fail
to compile, and the readers must accept owners and borrows alike.

Exit status: 0 if the self-tests pass and no rule fails; 1 otherwise.
"""

import argparse
import bisect
import concurrent.futures
import fnmatch
import glob
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import textwrap
from dataclasses import dataclass
from pathlib import Path

# Formatting functions and the index of their format argument.
FORMAT_FUNCTIONS = {
    "PyUnicode_FromFormat": 0,
    "PyErr_Format": 1,
    "_PyErr_Format": 2,
    "PyErr_FormatUnraisable": 0,
    "_PyErr_FormatFromCause": 1,
    "_PyErr_FormatFromCauseTstate": 2,
    "PyErr_WarnFormat": 2,
    "PyErr_ResourceWarning": 2,
    "PyErr_WarnExplicitFormat": 5,
    "PyUnicodeWriter_Format": 1,
    "_PyUnicodeWriter_Format": 1,
    "PySys_FormatStdout": 0,
    "PySys_FormatStderr": 0,
}

# Object slot fields with a value form (value-api.md, section 2.1), by the
# family whose marking makes a read of the field fail.
NUMBER_SLOTS = """
    nb_add nb_subtract nb_multiply nb_remainder nb_divmod nb_power
    nb_negative nb_positive nb_absolute nb_bool nb_invert nb_lshift nb_rshift
    nb_and nb_xor nb_or nb_int nb_float nb_inplace_add nb_inplace_subtract
    nb_inplace_multiply nb_inplace_remainder nb_inplace_power
    nb_inplace_lshift nb_inplace_rshift nb_inplace_and nb_inplace_xor
    nb_inplace_or nb_floor_divide nb_true_divide nb_inplace_floor_divide
    nb_inplace_true_divide nb_index nb_matrix_multiply
    nb_inplace_matrix_multiply
""".split()
SLOT_FAMILIES = {
    **{name: "number" for name in NUMBER_SLOTS},
    "tp_richcompare": "compare",
    "tp_hash": "hash",
    "tp_repr": "repr",
    "tp_str": "repr",
    "tp_getattro": "attribute",
    "tp_setattro": "attribute",
    "tp_descr_get": "descriptor",
    "tp_descr_set": "descriptor",
    "mp_subscript": "subscript",
    "sq_item": "subscript",
    "mp_ass_subscript": "subscript",
    "sq_ass_item": "subscript",
    "sq_contains": "subscript",
    "tp_iternext": "iteration",
    "am_send": "iteration",
    "tp_call": "call",
}

# The packet that marks a family adds it here (value-api.md, section 9).
# V2a/V2b have operational value dispatch for every family below. Keep this
# list explicit: a typed slot-table row is not enough to mark a family until
# its dispatcher is wired.
MARKED_SLOT_FAMILIES = frozenset({
    "number", "compare", "hash", "repr", "attribute", "descriptor",
    "subscript", "iteration", "call",
})

VALUE_TYPES = ("PyValue", "PyValueBorrow")
PYVALUE_TYPES = tuple(f'asString("{const}{name}")' for name in VALUE_TYPES
                      for const in ("", "const "))
VARIADIC_MATCHER = (
    "callExpr(callee(functionDecl(isVariadic())), hasAnyArgument(hasType("
    "hasCanonicalType(anyOf({value}, pointsTo(anyOf({value})))))))"
    '.bind("variadic")'.format(value=", ".join(PYVALUE_TYPES))
)
SLOT_READ_MATCHER = (
    "implicitCastExpr(hasCastKind(\"CK_LValueToRValue\"), has(memberExpr(member("
    "hasAnyName({names})))))"
    '.bind("slot")'.format(
        names=", ".join(f'"{name}"' for name in sorted(SLOT_FAMILIES)))
)
SLOT_CALL_MATCHER = (
    "callExpr(callee(expr(hasDescendant(memberExpr(member("
    "hasAnyName({names})))))))"
    '.bind("slotcall")'.format(
        names=", ".join(f'"{name}"' for name in sorted(SLOT_FAMILIES)))
)

# Who may know the word's encoding (value-api.md, section 1.6, rule 1), as
# paths relative to the source tree; one that ends in "/" or "_" is a
# prefix.
ENCODING_FILES = (
    "Include/internal/pycore_pyvalue.h",
    "Include/internal/pycore_value.h",
    "Include/internal/pycore_value_api.h",
    "Python/pyvalue.c",
    # The JIT, which emits tag tests (Python/jit_*.c, Python/jit_x86.h).
    "Python/jit_",
    # The out-of-process readers, which get the layout through
    # _Py_DebugOffsets.
    "Include/internal/pycore_debug_offsets.h",
    "Modules/_remote_debugging/",
    # The encoding's own tests.
    "Modules/_testinternalcapi.c",
    "Modules/_testinternalcapi/list.c",
    "Modules/_testinternalcapi/slots.c",
    # The type cache's guarded word (register-ir-v2-a5b.md, section 2.4).
    "Include/internal/pycore_typecache.h",
)
# Functions outside those files that may: the generated code's one
# count-free conversion of a borrow to an owner (register-ir-v2-a5b.md,
# section 4.2), and the conversions between the addresses of the data stack,
# which the public PyThreadState declares as PyObject **, and its slots
# (Include/internal/pycore_interpframe.h).
ENCODING_FUNCTIONS = ("_PyIR_AdoptTemp", "_PyDataStack_Slots",
                      "_PyDataStack_Address")
# The encoding constants of the value headers. The self-tests check that
# each is still defined there, so that a rename cannot empty the rule.
ENCODING_NAMES = (
    "Py_PTR_LIMIT", "Py_INDEX_BASE", "Py_DOUBLE_OFFSET", "Py_INT_SHIFT",
    "Py_INT_BASE", "Py_TAG_REFCNT", "Py_TAG_INVALID", "Py_TAG_BITS",
    "Py_TAGGED_SHIFT", "Py_OWNED_MASK", "PyValue_NULL_BITS", "_PyValue_BITS",
)
# Files whose text names .bits of something other than a word, which the
# text half of raw-bits cannot tell apart: no compile database covers the
# test modules, and these are the read-write lock's bits field.
NOT_WORD_BITS = ("Modules/_testinternalcapi/test_lock.c",)
ENCODING_HEADERS = ("Include/internal/pycore_pyvalue.h",
                    "Include/internal/pycore_value.h")
# The C files of the tree that the text rules read.
SOURCE_DIRS = ("Include", "Objects", "Python", "Modules", "Parser",
               "Programs", "PC")


def type_names(*names):
    return ", ".join(f'asString("{const}{name}")' for name in names
                     for const in ("", "const "))


def points_to(matcher):
    return f"pointsTo(qualType({matcher}))"


WORD_TYPES = ("PyValue", "PyValueBorrow", "_PyValueGuarded")
WORD = f"hasCanonicalType(anyOf({type_names(*WORD_TYPES)}))"
OWNER = f"hasCanonicalType(anyOf({type_names('PyValue')}))"
BORROW = f"hasCanonicalType(anyOf({type_names('PyValueBorrow')}))"
BYTE_POINTER = "pointsTo(voidType()), pointsTo(qualType(isAnyCharacter()))"
BYTE_OR_WORD_POINTER = f"qualType(anyOf({BYTE_POINTER}, {points_to(WORD)}))"
# A word, an array of words, or a struct with one: what a union may not
# hold beside other members.
WORD_OR_ARRAY = (f"qualType(anyOf({WORD}, hasCanonicalType(arrayType("
                 f"hasElementType(qualType({WORD}))))))")
WORD_HOLDER = (f"qualType(anyOf({WORD_OR_ARRAY}, hasCanonicalType(recordType("
               f"hasDeclaration(recordDecl(hasDescendant(fieldDecl(hasType("
               f"{WORD_OR_ARRAY})))))))))")
# A pointer to anything but a byte or a word, seen through every cast.
OTHER_POINTER = (f"ignoringParenCasts(expr(hasType(hasCanonicalType("
                 f"pointerType())), unless(hasType({BYTE_OR_WORD_POINTER}))))")
NOT_ENCODING_FUNCTION = (
    "unless(hasAncestor(functionDecl(hasAnyName({}))))".format(
        ", ".join(f'"{name}"' for name in ENCODING_FUNCTIONS)))
BORROW_STORAGE = (f"qualType(anyOf({BORROW}, hasCanonicalType(arrayType("
                  f"hasElementType(qualType({BORROW}))))))")
RAW_BITS_MATCHERS = (
    f'memberExpr(member(hasName("bits")), hasObjectExpression(hasType('
    f'qualType(anyOf({WORD}, {points_to(WORD)})))), {NOT_ENCODING_FUNCTION})'
    '.bind("rawbits")',
    # {0} zero-initializes, as static storage and calloc() do: the all-zero
    # word is PyValue_NULL by contract.
    f'initListExpr(hasType(qualType({WORD})), unless(hasInit(0, '
    f'ignoringParenImpCasts(integerLiteral(equals(0))))), '
    f'{NOT_ENCODING_FUNCTION}).bind("rawinit")',
    *(f'explicitCastExpr(hasDestinationType({points_to(word)}), '
      f'unless(hasSourceExpression(hasType(qualType(anyOf({BYTE_POINTER}, '
      f'{points_to(word)}))))), {NOT_ENCODING_FUNCTION})'
      '.bind("rawcast")'
      for word in (f"hasCanonicalType(anyOf({type_names(name)}))"
                   for name in WORD_TYPES)),
    f'explicitCastExpr(hasSourceExpression(hasType({points_to(WORD)})), '
    'hasDestinationType(pointerType()), unless(hasDestinationType('
    f'{BYTE_OR_WORD_POINTER})), {NOT_ENCODING_FUNCTION}).bind("rawcast")',
    # The byte-pointer allowance does not launder: a word pointer made from
    # the address of a non-word object, or from a cast of a pointer to
    # another type, and a typed pointer made from a cast of a word pointer.
    f'explicitCastExpr(hasDestinationType({points_to(WORD)}), '
    'hasSourceExpression(ignoringParenImpCasts(anyOf('
    'ignoringParenCasts(unaryOperator(hasOperatorName("&"), hasUnaryOperand('
    f'expr(unless(hasType(qualType({WORD}))))))), '
    f'explicitCastExpr(hasSourceExpression({OTHER_POINTER}))))), '
    f'{NOT_ENCODING_FUNCTION}).bind("rawcast")',
    'explicitCastExpr(hasDestinationType(pointerType()), unless('
    f'hasDestinationType({BYTE_OR_WORD_POINTER})), hasSourceExpression('
    'ignoringParenImpCasts(explicitCastExpr(hasSourceExpression('
    f'ignoringParenCasts(expr(hasType(qualType({points_to(WORD)})))))))), '
    f'{NOT_ENCODING_FUNCTION}).bind("rawcast")',
    f'recordDecl(isUnion(), hasDescendant(fieldDecl(hasType({WORD_HOLDER}))))'
    '.bind("rawunion")',
)
BORROW_FIELD_MATCHERS = (
    f'fieldDecl(hasType({BORROW_STORAGE})).bind("borrowfield")',
    'varDecl(anyOf(hasGlobalStorage(), hasType(arrayType())), '
    f'hasType({BORROW_STORAGE})).bind("borrowvar")',
)
# A member of a C local struct (a status-and-value pair returned by value)
# is not a home: the struct is a local owner.
HOME_COPY_MATCHER = (
    f'varDecl(hasLocalStorage(), hasType(qualType({OWNER})), hasInitializer('
    'ignoringParenImpCasts(anyOf(memberExpr(unless(hasObjectExpression('
    'ignoringParenImpCasts(declRefExpr(to(varDecl(hasLocalStorage(), '
    'hasType(hasCanonicalType(recordType()))))))))), arraySubscriptExpr(), '
    'unaryOperator(hasOperatorName("*")))))).bind("homecopy")'
)
RAW_BITS_MESSAGES = {
    "rawbits": "reads or writes a word's bits",
    "rawinit": "builds a word from bits",
    "rawcast": "casts between a word pointer and another pointer type",
    "rawunion": "puns a word through a union",
}
# The functions that box an inline value, which a thunk body must not call
# directly: the primitives and the helpers built on them.
# The At forms are macros over these functions; the For forms state a
# boundary box, which a compatibility entry makes.
BOX_PRIMITIVES = ("PyValue_AsObject", "_PyValue_BoxInlineValue",
                  "_PyValue_BoxInto", "_PyValueTemps_Object",
                  "_PyBorrow_TupleItems", "_PyValue_AdapterBox")
COMPAT_BODY_MATCHER = (
    "callExpr(callee(functionDecl(hasAnyName({names}))), hasAncestor("
    'functionDecl(isDefinition(), hasAttr("attr::Unavailable"))))'
    '.bind("compat_box")'.format(
        names=", ".join(f'"{name}"' for name in BOX_PRIMITIVES))
)
# The second pass (Py_STATS), over the units that define a Py_VALUE_ENTRY
# (pyjit/design/proposals/value-api-g.md, section 1.3). Py_VALUE_ENTRY and
# _Py_BOX_CALLER() exist only under Py_STATS, so the first pass cannot see
# them. An entry is a definition that carries attr::Annotate (clang's mark;
# the attribute of a declaration reaches its definition).
ENTRY = 'functionDecl(isDefinition(), hasAttr("attr::Annotate"))'
CORE_BOX_CALLEES = ("PyValue_AsObject", "_PyValueTemps_Object",
                    "_PyValue_BoxInlineValue", "_PyValue_AdapterBox")
# The THUNK forms: _PyValue_THUNK_EXIT() is a function whose definition
# is the macro's own; the capture is __builtin_return_address, which the
# THUNK_AT macro expands in the caller (_PyValue_THUNK_NEW() has no
# function, only that capture and _PyValue_AsObjectFor).
THUNK_FUNCTIONS = ("_PyValue_THUNK_EXIT", "_PyValue_THUNK_New")
BOX_KINDS_BOUNDARY = ("BOX_KIND_THUNK", "BOX_KIND_SLOT_ADAPTER",
                      "BOX_KIND_CALL_ADAPTER")
ENTRY_MATCHERS = {
    "entry_box": (
        "callExpr(callee(functionDecl(hasAnyName({names}))), "
        "hasAncestor({entry})).bind(\"entry_box\")".format(
            names=", ".join(f'"{n}"' for n in CORE_BOX_CALLEES),
            entry=ENTRY)),
    "capture": (
        "callExpr(anyOf(callee(functionDecl(hasName("
        '"__builtin_return_address"))), callee(functionDecl(hasAnyName({t})))'
        "), unless(hasAncestor({entry})), unless(hasAncestor(functionDecl("
        "isDefinition(), hasAnyName({t})))), "
        # The debug builds' borrow-stack push site is no box owner.
        "unless(isExpandedFromMacro(\"BORROW_PUSH_SITE\")))"
        ".bind(\"capture\")".format(
            t=", ".join(f'"{n}"' for n in THUNK_FUNCTIONS), entry=ENTRY)),
    "nested": (
        'callExpr(callee(functionDecl(hasAttr("attr::Annotate"))), '
        "hasAncestor({entry})).bind(\"nested\")".format(entry=ENTRY)),
    # A _PyBoxAt with a kind other than CORE (0), outside the THUNK kind
    # (1) in an entry. The kinds are macros (BOX_KIND_*), so the matcher
    # reads the literal's third initializer, which also catches a literal 2
    # or 3. parse_output reports it when the expansion site is not a
    # boundary file; the THUNK, For and adapter macros expand in the files
    # that may use them. See BOX_TEXT_RULES for why the text rule exists too.
    "kind": (
        'compoundLiteralExpr(hasType(asString("_PyBoxAt")), has(initListExpr('
        "hasInit(2, ignoringParenImpCasts(integerLiteral(unless(equals(0)))))"
        ")), unless(allOf(hasAncestor({entry}), has(initListExpr(hasInit(2, "
        "ignoringParenImpCasts(integerLiteral(equals(1))))))))"
        ").bind(\"kind\")".format(entry=ENTRY)),
}
ENTRY_EXTRA = ("-DPy_STATS",)
# The units that define a function marked Py_VALUE_ENTRY, whose bodies the
# second pass reads (value-api-g.md, section 1.3). A wave that marks a
# function in another unit adds the unit here.
ENTRY_UNITS = tuple(f"Objects/{n}.c" for n in (
    "abstract", "object", "typeobject", "dictobject", "listobject",
    "setobject", "cellobject", "structseq", "tupleobject")) + (
    "Python/borrow.c", "Python/getargs.c")
ENTRY_QUERY = "\n".join([
    "set output diag",
    "enable output dump",
    "set traversal AsIs",
    *(f"match {m}" for m in ENTRY_MATCHERS.values()),
    "",
])
QUERY = "\n".join([
    "set output diag",
    "enable output dump",
    "set traversal IgnoreUnlessSpelledInSource",
    f"match {VARIADIC_MATCHER}",
    *(f"match {m}" for m in (*RAW_BITS_MATCHERS, *BORROW_FIELD_MATCHERS,
                             HOME_COPY_MATCHER)),
    "set traversal AsIs",
    f"match {SLOT_READ_MATCHER}",
    f"match {SLOT_CALL_MATCHER}",
    f"match {COMPAT_BODY_MATCHER}",
    "",
])

FORMAT_SPEC = re.compile(
    r"%(?P<flags>[-0#]*)(?P<width>\*|\d*)(?:\.(?P<precision>\*|\d*))?"
    r"(?P<size>ll|l|z|t|j|v)?(?P<conv>.)", re.S)
VALUE_CONVERSIONS = frozenset("RSATN")


@dataclass(frozen=True, order=True)
class Finding:
    path: str
    line: int
    column: int
    rule: str
    message: str
    fails: bool

    def __str__(self):
        status = "error" if self.fails else "note"
        return (f"{self.path}:{self.line}:{self.column}: {status}: "
                f"[{self.rule}] {self.message}")


def find_tool(name, explicit):
    """explicit, else name on PATH, else the newest /usr/lib/llvm-*/bin."""
    if explicit:
        return explicit
    found = shutil.which(name)
    if found:
        return found
    candidates = glob.glob(f"/usr/lib/llvm-*/bin/{name}")
    candidates.sort(key=lambda path: int(re.search(r"llvm-(\d+)", path)[1]))
    if candidates:
        return candidates[-1]
    sys.exit(f"check_value_api: {name} not found; pass --{name}")


def entry_arguments(entry):
    if "arguments" in entry:
        return list(entry["arguments"])
    return shlex.split(entry["command"])


def defines(arguments):
    names = set()
    for i, arg in enumerate(arguments):
        if arg == "-D" and i + 1 < len(arguments):
            arg = "-D" + arguments[i + 1]
        if arg.startswith("-D"):
            names.add(arg[2:].split("=", 1)[0])
    return names


SOURCE_DEFINE = re.compile(
    r"^\s*#\s*define\s+(Py_VALUE_CORE|Py_VALUE_ADAPTER|Py_BUILD_CORE"
    r"|Py_BUILD_CORE_BUILTIN|Py_BUILD_CORE_MODULE)\b", re.M)


DEFER_LINE = re.compile(r"^[ \t]*#[ \t]*define[ \t]+Py_VALUE_DEFER_MARKS\b", re.M)


def deferred_units(entries):
    """A note for each unit (entry, kind) that carries the defer line."""
    findings = []
    for entry, kind in entries:
        path = Path(entry["directory"], entry["file"])
        try:
            text = path.read_text(errors="replace")
        except OSError:
            continue
        m = DEFER_LINE.search(text)
        if m is not None:
            line = text.count("\n", 0, m.start()) + 1
            findings.append(Finding(
                display_path(entry["file"], entry["directory"]), line, 1,
                "deferred", "Py_VALUE_DEFER_MARKS: the compatibility marks "
                "do not apply in this unit", False))
    return findings


VALUE_CORE_LINE = re.compile(r"^[ \t]*#[ \t]*define[ \t]+Py_VALUE_CORE\b", re.M)
INCLUDED_SOURCE = re.compile(r'^[ \t]*#[ \t]*include[ \t]+"([^"]+\.c)"', re.M)


def unchecked_value_units(srcdir, checked):
    """A note for each C file of Modules/ that defines Py_VALUE_CORE and is
    not a unit of the compile database (checked: their paths relative to
    srcdir).  A shared extension module is a unit only if the database was
    recorded with sharedmods; one whose library the host lacks is absent
    legitimately, so this is a note, not a failure."""
    findings = []
    # A source that a unit includes textually (_ssl.c includes _ssl/cert.c)
    # is preprocessed into that unit, so it is checked with it.
    included = set()
    for rel in checked:
        unit = Path(srcdir, rel)
        if not unit.is_file():
            continue
        for name in INCLUDED_SOURCE.findall(unit.read_text(errors="replace")):
            included.add(str(Path(rel).parent / name))
    for path in sorted(Path(srcdir, "Modules").rglob("*.c")):
        rel = str(path.relative_to(srcdir))
        if rel in checked or rel in included:
            continue
        m = VALUE_CORE_LINE.search(path.read_text(errors="replace"))
        if m is not None:
            line = path.read_text(errors="replace").count("\n", 0, m.start()) + 1
            findings.append(Finding(
                rel, line, 1, "unchecked",
                "a value-native source that is not a unit of the compile "
                "database (its extension module is not built on this host)",
                False))
    return findings


# Units that test the compatibility API, whose boxes are not debt
# (pyjit/design/proposals/value-api-g.md, section 1.8): fnmatch patterns over
# source paths relative to the source directory. W4 shares the list.
COMPAT_TEST_UNITS = (
    # These units exercise object-form APIs; _testclinic_value.c covers the
    # generated value convention separately.
    "Modules/_testclinic.c",
    "Modules/_testinternalcapi.c",
    "Modules/_testinternalcapi/interpreter.c",
    "Modules/_xxtestfuzz/fuzzer.c",
    "Modules/_testcapi*",
    "Modules/_testlimitedcapi*",
    "Modules/xx*",
    "Modules/_testbuffer.c",
    "Modules/_testmultiphase.c",
    "Modules/_testsinglephase.c",
    "Modules/_testimportmultiple.c",
    "Modules/_ctypes/_ctypes_test*",
    "Lib/test/test_cext/*",
    "Lib/test/test_cppext/*",
)


def is_compat_test_unit(relpath):
    """Whether a source path relative to the source directory is listed."""
    relpath = str(relpath).replace(os.sep, "/")
    return any(fnmatch.fnmatchcase(relpath, pattern)
               for pattern in COMPAT_TEST_UNITS)


def unit_kind(entry):
    """'core', 'adapter' (core, slot rule skipped) or None."""
    path = Path(entry["directory"], entry["file"])
    if path.suffix != ".c":
        return None
    names = defines(entry_arguments(entry))
    try:
        names |= set(SOURCE_DEFINE.findall(path.read_text(errors="replace")))
    except OSError:
        return None
    core = "Py_VALUE_CORE" in names or (
        "Py_BUILD_CORE" in names
        and not names & {"Py_BUILD_CORE_BUILTIN", "Py_BUILD_CORE_MODULE"})
    if not core:
        return None
    return "adapter" if "Py_VALUE_ADAPTER" in names else "core"


def c_string(literal):
    """The value of the C string literal text between the quotes."""
    simple = {"n": "\n", "t": "\t", "r": "\r", "0": "\0", "\\": "\\",
              '"': '"', "'": "'", "a": "\a", "b": "\b", "f": "\f",
              "v": "\v", "?": "?"}
    out = []
    i = 0
    while i < len(literal):
        c = literal[i]
        if c == "\\" and i + 1 < len(literal):
            nxt = literal[i + 1]
            if nxt == "x":
                m = re.match(r"[0-9a-fA-F]+", literal[i + 2:])
                out.append(chr(int(m[0], 16)))
                i += 2 + len(m[0])
                continue
            if nxt in "01234567":
                m = re.match(r"[0-7]{1,3}", literal[i + 1:])
                out.append(chr(int(m[0], 8)))
                i += 1 + len(m[0])
                continue
            out.append(simple.get(nxt, nxt))
            i += 2
            continue
        out.append(c)
        i += 1
    return "".join(out)


def format_arguments(fmt):
    """For each argument that fmt consumes, (conversion, takes a value)."""
    args = []
    pos = 0
    while True:
        pos = fmt.find("%", pos)
        if pos < 0:
            return args
        if fmt.startswith("%%", pos):
            pos += 2
            continue
        m = FORMAT_SPEC.match(fmt, pos)
        if m is None:
            return args
        spec = m[0]
        if m["width"] == "*":
            args.append((spec, False))
        if m["precision"] == "*":
            args.append((spec, False))
        conv = m["conv"]
        value = m["size"] == "v" and conv in VALUE_CONVERSIONS
        args.append((spec, value))
        if conv == "V":
            args.append((spec, False))
        pos = m.end()


@dataclass
class Call:
    callee: str
    argument_types: list
    literals: list


DUMP_CHILD = re.compile(r"^[|`]-(\w+) 0x[0-9a-f]+ (?:<[^>]*> )?(.*)$")
QUOTED = re.compile(r"'([^']*)'(?::'([^']*)')?")


def parse_call_dump(lines):
    """The callee, the canonical type of each argument and, per argument,
    the string literal it is (or None), from a clang-query dump."""
    children = []
    for line in lines[1:]:
        if line.startswith(("|-", "`-")):
            children.append([line])
        elif children:
            children[-1].append(line)
    callee = None
    types = []
    literals = []
    for i, child in enumerate(children):
        m = DUMP_CHILD.match(child[0])
        if m is None:
            types.append("?")
            literals.append(None)
            continue
        kind, rest = m.groups()
        q = QUOTED.search(rest)
        if i == 0:
            names = re.findall(r"Function 0x[0-9a-f]+ '([^']+)'", rest)
            callee = names[0] if names else None
            continue
        types.append((q[2] or q[1]) if q else "?")
        literal = None
        if kind == "StringLiteral":
            literal = c_string(rest[rest.index('"') + 1:rest.rindex('"')])
        literals.append(literal)
    return Call(callee, types, literals)


def value_type(type_name):
    """PyValue or PyValueBorrow if type_name is one of them, else None."""
    name = type_name.replace("const ", "").strip()
    return name if name in VALUE_TYPES else None


def is_value_pointer(type_name):
    return re.fullmatch(r"(const )?PyValue(Borrow)? \*( const)?",
                        type_name.strip())


def check_call(call):
    """Messages for one variadic call that receives a PyValue."""
    messages = []
    fmt_index = FORMAT_FUNCTIONS.get(call.callee)
    for i, type_name in enumerate(call.argument_types):
        if is_value_pointer(type_name):
            messages.append(f"argument {i + 1} of {call.callee}() is a "
                            f"{type_name} passed to a variadic argument")
    if fmt_index is None or fmt_index >= len(call.literals) \
            or call.literals[fmt_index] is None:
        for name in dict.fromkeys(filter(None, map(value_type,
                                                   call.argument_types))):
            what = ("a formatting function with no literal format"
                    if fmt_index is not None else "a variadic argument")
            messages.append(f"{name} passed to {what} of {call.callee}()")
        return messages
    expected = format_arguments(call.literals[fmt_index])
    actual = call.argument_types[fmt_index + 1:]
    if len(expected) != len(actual):
        messages.append(f"{call.callee}() format takes {len(expected)} "
                        f"arguments, got {len(actual)}")
    for (spec, wants_value), type_name in zip(expected, actual):
        name = value_type(type_name)
        if name and not wants_value:
            conv = spec[-1]
            hint = (f"; use %v{conv}" if conv in VALUE_CONVERSIONS
                    else "")
            messages.append(f"{name} passed to {spec} in "
                            f"{call.callee}(){hint}")
        elif wants_value and name == "PyValue":
            messages.append(f"PyValue passed to the value conversion {spec} "
                            f"in {call.callee}(); pass PyValue_Borrow()")
        elif wants_value and name != "PyValueBorrow":
            messages.append(f"{type_name} passed to the value conversion "
                            f"{spec} in {call.callee}()")
    return messages


LOCATION = re.compile(r"^(.*?):(\d+):(\d+): note: \"(\w+)\" binds here")
# The range of a node dump's first line: <file:line:col, ...> or
# <file:line:col>, where the end may name its own file.
DUMP_RANGE = re.compile(r" <([^<>,]+?):(\d+):(\d+)(?:, (?:line:\d+:\d+|col:\d+|"
                        r"([^<>,]+?):\d+:\d+))?>")
SRCDIR = Path(__file__).resolve().parents[2]


def display_path(path, directory):
    """path, which clang reports relative to directory, made relative to the
    source tree if it is inside it, so that each location has one name."""
    full = Path(os.path.normpath(Path(directory, path)))
    if full.is_relative_to(SRCDIR):
        return str(full.relative_to(SRCDIR))
    return str(full)


def spelled_range(dump, path, line, column):
    """Where the first and last tokens of a match are written: ((file, line,
    column), file). clang reports a match at its file location: a token of
    a macro's body at the macro's expansion site, a token of a macro's
    argument where the argument is written. The node dump that follows
    gives the range's spelling locations; a location without a file is in
    the previous one's file."""
    m = DUMP_RANGE.search(dump[0]) if dump else None
    if m is None or m[1] is None:
        return (path, line, column), path
    begin = (m[1], int(m[2]), int(m[3]))
    return begin, m[4] or begin[0]


def encoding_allowed(path):
    """Whether path, relative to the source tree, may know the encoding."""
    return any(path == allowed or allowed.endswith(("/", "_"))
               and path.startswith(allowed) for allowed in ENCODING_FILES)


def parse_output(text, kind, directory):
    """Findings from the output of clang-query for one unit, run in
    directory."""
    findings = []
    lines = text.splitlines()
    i = 0
    while i < len(lines):
        m = LOCATION.match(lines[i])
        if m is None or m[4] == "root":
            i += 1
            continue
        path, line, column, binding = m[1], int(m[2]), int(m[3]), m[4]
        i += 1
        while i < len(lines) and not lines[i].startswith(
                f'Binding for "{binding}":'):
            i += 1
        i += 1
        dump = []
        while i < len(lines) and lines[i].strip() and not LOCATION.match(
                lines[i]) and not lines[i].startswith("Match #"):
            dump.append(lines[i])
            i += 1
        # A match is written where its tokens are spelled: in a macro's body
        # it is the macro's file's, in a macro's argument the unit's.
        begin, end = spelled_range(dump, path, line, column)
        path = display_path(path, directory)
        begin_path = display_path(begin[0], directory)
        in_macro_body = (begin_path, *begin[1:]) != (path, line, column)
        spelled = {begin_path, display_path(end, directory)}
        if binding in RAW_BITS_MESSAGES:
            if not all(map(encoding_allowed, spelled)):
                findings.append(Finding(path, line, column, "raw-bits",
                                        RAW_BITS_MESSAGES[binding], True))
        elif binding in ("borrowfield", "borrowvar"):
            what = "field" if binding == "borrowfield" else "variable"
            findings.append(Finding(
                path, line, column, "borrow-field",
                f"a PyValueBorrow {what} outside a C local", True))
        elif binding == "homecopy" and not in_macro_body:
            # A copy that a macro declares is the macro's: the IR
            # publication macros displace a slot's word and close it.
            findings.append(Finding(
                path, line, column, "home-copy",
                "a PyValue local copied from a home: a PyValueBorrow unless "
                "the home is stored afterwards", False))
        elif binding == "variadic":
            for message in check_call(parse_call_dump(dump)):
                findings.append(
                    Finding(path, line, column, "variadic", message, True))
        elif binding in ("slot", "slotcall") and kind == "core":
            names = re.findall(r"->(\w+)|\.(\w+)", " ".join(dump))
            field = next((a or b for a, b in names
                          if (a or b) in SLOT_FAMILIES), None)
            family = SLOT_FAMILIES.get(field, "?")
            if binding == "slotcall":
                findings.append(Finding(
                    path, line, column, "slot-call",
                    f"direct call of {field} ({family} family)",
                    family in MARKED_SLOT_FAMILIES))
            else:
                findings.append(Finding(
                    path, line, column, "slot-read",
                    f"read of {field} ({family} family)", False))
        elif binding == "entry_box":
            findings.append(Finding(
                path, line, column, "compat-body",
                "core box form in a Py_VALUE_ENTRY body; box through "
                "_PyValue_THUNK_EXIT() or _PyValue_THUNK_NEW()", True))
        elif binding == "capture":
            findings.append(Finding(
                path, line, column, "compat-capture",
                "a caller capture outside a Py_VALUE_ENTRY body", True))
        elif binding == "nested":
            findings.append(Finding(
                path, line, column, "compat-capture",
                "a Py_VALUE_ENTRY function calls another: the outer entry "
                "would become the owner", True))
        elif binding == "kind":
            if not boundary_allowed(path):
                findings.append(Finding(
                    path, line, column, "boundary-site",
                    "a boundary box kind written outside the boundary "
                    "files", True))
        elif binding == "compat_box":
            findings.append(Finding(
                path, line, column, "compat-body",
                "box primitive in a Py_VALUE_COMPAT body; box through "
                "_PyValue_THUNK_EXIT()", True))
    return findings


def compile_flags(entry):
    """The options of entry's command: no compiler, -c, output or source."""
    flags = []
    arguments = entry_arguments(entry)[1:]
    i = 0
    while i < len(arguments):
        arg = arguments[i]
        if arg == "-o":
            i += 2
            continue
        if arg != "-c" and not arg.endswith(".c"):
            flags.append(arg)
        i += 1
    return flags


def run_query(clang_query, query_file, entry, kind, extra=()):
    path = Path(entry["directory"], entry["file"])
    # Clang parses a unit that GCC compiles: silence its notes on GCC-only
    # options.
    proc = subprocess.run(
        [clang_query, "-f", query_file, str(path), "--", *compile_flags(entry), *extra,
         "-Wno-unknown-warning-option", "-Wno-ignored-optimization-argument"],
        cwd=entry["directory"], capture_output=True, text=True)
    errors = [line for line in proc.stdout.splitlines()
              + proc.stderr.splitlines() if ": error:" in line]
    return path, parse_output(proc.stdout, kind, entry["directory"]), errors


def check_units(clang_query, entries, jobs, query=QUERY, extra=()):
    with tempfile.TemporaryDirectory() as tmp:
        query_file = os.path.join(tmp, "query.txt")
        Path(query_file).write_text(query)
        findings = set()
        errors = []
        with concurrent.futures.ThreadPoolExecutor(jobs) as pool:
            futures = [pool.submit(run_query, clang_query, query_file,
                                   entry, kind, extra)
                       for entry, kind in entries]
            for future in futures:
                path, unit_findings, unit_errors = future.result()
                findings.update(unit_findings)
                errors.extend(f"{path}: {e}" for e in unit_errors)
        return sorted(findings), errors


# The text rules. C_NOISE matches what they skip: comments, and string and
# character literals.
C_NOISE = re.compile(r"/\*.*?\*/|//[^\n]*|\"(?:\\.|[^\"\\\n])*\""
                     r"|'(?:\\.|[^'\\\n])*'", re.S)


def blank(m):
    """The text of match m as spaces, with its newlines kept."""
    return re.sub(r"[^\n]", " ", m[0])


def strip_c(text):
    """text with comments and literals blanked, keeping each remaining
    character at its offset."""
    return C_NOISE.sub(blank, text)


def lines_of(text):
    """The function from an offset in text to its line number."""
    starts = [0] + [m.end() for m in re.finditer(r"\n", text)]
    return lambda offset: bisect.bisect_right(starts, offset)


ENCODING_NAME = re.compile(r"\b(?:{})\b".format("|".join(ENCODING_NAMES)))
WORD_LITERAL = re.compile(r"\(\s*(?:const\s+)?(?:PyValue|PyValueBorrow)\s*\)"
                          r"\s*\{")
BITS_ACCESS = re.compile(r"(?:\.|->)\s*bits\b")


def encoding_function_lines(text):
    """The lines of the bodies of ENCODING_FUNCTIONS in text, in the
    CPython layout: the name at the start of a line, the body up to the next
    line that is a lone closing brace."""
    lines = set()
    for name in ENCODING_FUNCTIONS:
        for m in re.finditer(rf"^{re.escape(name)}\s*\(", text, re.M):
            end = re.compile(r"^\}", re.M).search(text, m.end())
            first = text.count("\n", 0, m.start()) + 1
            last = (text.count("\n", 0, end.start()) + 1 if end
                    else text.count("\n") + 1)
            lines.update(range(first, last + 1))
    return lines


def raw_bits_text(path, text, scan_bits):
    """The raw-bits findings of the text of one C file, path relative to the
    source tree. scan_bits: whether to look for .bits and ->bits, which
    clang-query decides in the units it checks."""
    if encoding_allowed(path):
        return []
    code = strip_c(text)
    line_of = lines_of(code)
    allowed_lines = encoding_function_lines(code)
    patterns = [(ENCODING_NAME, "names the encoding constant {}"),
                (WORD_LITERAL, "builds a word from bits")]
    if scan_bits:
        patterns.append((BITS_ACCESS, "reads or writes a word's bits"))
    findings = []
    for pattern, message in patterns:
        for m in pattern.finditer(code):
            line = line_of(m.start())
            if line not in allowed_lines:
                column = m.start() - code.rfind("\n", 0, m.start())
                findings.append(Finding(path, line, column, "raw-bits",
                                        message.format(m[0]), True))
    return findings


def source_files(srcdir):
    """The C files of the tree that the text rules read, relative to it."""
    found = []
    for directory in SOURCE_DIRS:
        for suffix in ("*.c", "*.h"):
            found.extend(Path(srcdir, directory).rglob(suffix))
    return sorted(str(p.relative_to(srcdir)) for p in found)


def check_raw_bits_text(srcdir, checked_units=frozenset()):
    """The raw-bits findings of the tree's text. checked_units: the paths of
    the units that clang-query checks, whose .bits accesses it decides; a
    caller with no database passes none and the text reads .bits in the
    headers only."""
    findings = []
    for path in source_files(srcdir):
        scan_bits = path not in NOT_WORD_BITS and (path.endswith(".h") or (
            checked_units and path not in checked_units))
        findings.extend(raw_bits_text(
            path, Path(srcdir, path).read_text(errors="replace"), scan_bits))
    return findings



# window-read (value-api-m3.md, section 1.1): in a function that takes a
# call window, `const PyValue *args`, and returns PyValue (the shape of
# _Py_vectorcallvaluefunc, _PyCFunctionValue, _PyCMethodValue and
# _Py_wrappervaluefunc), every args[i] is the operand of one of these.
WINDOW_READERS = frozenset({"PyValue_Borrow", "PyValue_DUP", "PyValue_XDUP"})
WINDOW_PRIMITIVE = re.compile(r"_PyValue_Window\w*")
WINDOW_FUNCTION = re.compile(
    r"\bPyValue\s+\(?\s*\*?\s*\w+\s*\)?\s*\(((?:[^(){};]|\([^()]*\))*)\)\s*\{")
WINDOW_PARAMETER = re.compile(r"\bconst\s+PyValue\s*\*\s*args\b")
WINDOW_READ = re.compile(r"(?<![\w&])args\s*\[")


def function_body_end(code, start):
    """The offset after the brace that closes the one that opens at start."""
    depth = 0
    for i in range(start, len(code)):
        if code[i] == "{":
            depth += 1
        elif code[i] == "}":
            depth -= 1
            if depth == 0:
                return i + 1
    return len(code)


def enclosing_call(code, offset, floor):
    """The name of the innermost call that contains offset (searching back
    to floor), or None."""
    depth = 0
    for i in range(offset - 1, floor - 1, -1):
        if code[i] in ")]":
            depth += 1
        elif code[i] in "([":
            if depth:
                depth -= 1
            elif code[i] == "(":
                m = re.search(r"(\w+)\s*$", code[floor:i])
                return m[1] if m else None
            else:
                return None
        elif code[i] in ";{}":
            return None
    return None


def window_read_text(path, text):
    """The window-read findings of the text of one C file."""
    code = strip_c(text)
    line_of = lines_of(code)
    findings = []
    for fn in WINDOW_FUNCTION.finditer(code):
        if not WINDOW_PARAMETER.search(fn[1]):
            continue
        body = fn.end() - 1
        end = function_body_end(code, body)
        for read in WINDOW_READ.finditer(code, body, end):
            name = enclosing_call(code, read.start(), body)
            if name in WINDOW_READERS or (
                    name and WINDOW_PRIMITIVE.fullmatch(name)):
                continue
            column = read.start() - code.rfind("\n", 0, read.start())
            findings.append(Finding(
                path, line_of(read.start()), column, "window-read",
                "args[i] is read outside PyValue_Borrow(), PyValue_DUP() "
                "and the _PyValue_Window*() primitives", True))
    return findings


# vectorcall-thunk (value-api-m3.md, section 1.7): only the creators store
# the object thunk of a value vectorcall, and only the files that compare
# against the function's own vectorcall name it. Paths as in ENCODING_FILES.
THUNK_FILES = (
    "Objects/call.c", "Objects/funcobject.c", "Objects/methodobject.c",
    "Objects/classobject.c", "Objects/descrobject.c", "Objects/typeobject.c",
    "Include/internal/pycore_value_call.h",
    "Include/internal/pycore_methodobject.h",
    "Include/internal/pycore_classobject.h",
    # Static types whose Clinic @vectorcall constructor is a value
    # vectorcall, stored beside the object thunk.
    "Objects/tupleobject.c", "Objects/bytesobject.c",
    "Objects/bytearrayobject.c", "Objects/floatobject.c",
    "Objects/enumobject.c", "Objects/boolobject.c", "Objects/longobject.c",
    "Objects/unicodeobject.c", "Objects/listobject.c", "Objects/setobject.c",
    "Objects/rangeobject.c", "Objects/dictobject.c",
    # Decimal pairs its selective Clinic constructor with the object thunk.
    "Modules/_decimal/_decimal.c",
    # BaseException stores its value vectorcall beside the object thunk.
    "Objects/exceptions.c",
    # The test helpers: a spec type that stores the thunk, and an accessor
    # that compares against both.
    "Modules/_testinternalcapi/value_call.c",
    # The legacy trace handlers and the branch handler of sys.monitoring
    # are value-vectorcall types of their own.
    "Python/instrumentation.c", "Python/legacy_tracing.c",
    # functools.partial is a value-vectorcall type: its instances store the
    # value call beside the object thunk.
    "Modules/_functoolsmodule.c",
    # Operator getter/caller instances store paired value vectorcalls.
    "Modules/_operator.c",
)
FUNCTION_VECTORCALL_FILES = (
    "Objects/call.c", "Objects/funcobject.c", "Python/bytecodes.c",
    "Python/generated_", "Python/specialize.c", "Python/ceval.c",
    "Python/perf_trampoline.c", "Include/internal/pycore_function.h",
    "Include/cpython/funcobject.h", "Modules/_testinternalcapi/value_call.c",
)
THUNK_NAMES = (
    (re.compile(r"\b_PyValue_VectorcallThunk\b"), THUNK_FILES),
    (re.compile(r"\b_PyFunction_Vectorcall\b"), FUNCTION_VECTORCALL_FILES),
)


def path_listed(path, listed):
    """Whether path is in listed; an entry ending in "_" is a prefix."""
    return any(path == entry or entry.endswith("_") and path.startswith(entry)
               for entry in listed)


def vectorcall_thunk_text(path, text):
    """The vectorcall-thunk findings of the text of one C file."""
    code = strip_c(text)
    line_of = lines_of(code)
    findings = []
    for pattern, listed in THUNK_NAMES:
        if path_listed(path, listed):
            continue
        for m in pattern.finditer(code):
            column = m.start() - code.rfind("\n", 0, m.start())
            findings.append(Finding(
                path, line_of(m.start()), column, "vectorcall-thunk",
                f"{m[0]} is named outside the files that create or compare "
                "against it; call _PyValue_VectorcallTstate()", True))
    return findings


def check_call_text(srcdir):
    """The window-read and vectorcall-thunk findings of the tree's text."""
    findings = []
    for path in source_files(srcdir):
        text = Path(srcdir, path).read_text(errors="replace")
        findings.extend(window_read_text(path, text))
        findings.extend(vectorcall_thunk_text(path, text))
    return findings


# The box text rules (value-api-g.md, section 1.7). Each rule names tokens
# and the files, as fnmatch patterns over paths relative to the source tree,
# that may spell them. Adding a file takes a line in the design.
#
# boundary-site is keyed on what makes a box a boundary box, the non-CORE
# kind tokens, as well as on the names of the For forms: a callee in
# parentheses, (PyValue_AsObject)(v, _PyBox_AT(s, BOX_KIND_CALL_ADAPTER, x)),
# names no For form but still states the kind. The tokens are read after
# comments and literals are blanked, with no regard to the callee's spelling,
# so that form fails. The ENTRY_MATCHERS["kind"] AST matcher repeats the
# check on the expanded units of the second pass (a kind reached through a
# macro that is spelled elsewhere), which is why both exist.
BOX_SCAN_DIRS = ("Include", "Objects", "Python", "Parser", "Modules")
BOUNDARY_FILES = (
    "Include/internal/pycore_value.h", "Include/internal/pycore_borrow.h",
    "Include/internal/pycore_value_slots.h",
    "Include/internal/pycore_value_api.h", "Python/borrow.c",
    "Python/getargs.c", "Python/native_scope.c", "Objects/call.c",
    "Objects/abstract.c", "Objects/object.c", "Modules/_ctypes/*",
    "Modules/_testinternalcapi/value_boxes.c",
    # The container getters (PyList_GetItemRef, the PyDict_GetItemRef family)
    # pass the THUNK box argument of their entry to a static helper that
    # core callers share (value-api-g.md, section 1.3); added to the table.
    "Objects/dictobject.c", "Objects/listobject.c",
    # The call adapters name the callee that receives the boxed self.
    "Objects/methodobject.c", "Objects/descrobject.c",
    # The adapter of PyEval_SetTrace()/SetProfile(): the C-API entry that
    # boxes the argument word for the extension's Py_tracefunc.
    "Python/legacy_tracing.c")
STATS_FILES = ("Include/cpython/pystats.h", "Python/pystats.c")
BOX_TEXT_RULES = (
    ("test-site", ("BOX_SITE_TEST",),
     (*STATS_FILES, "Modules/_testinternalcapi*")),
    ("boundary-site", ("_Py_BOX_OWNER_FOREIGN",),
     ("Include/internal/pycore_value.h", "Modules/_ctypes/*",
      "Modules/_testinternalcapi/value_boxes.c")),
    ("boundary-site",
     ("_PyValue_AsObjectFor", "_PyBorrow_TupleItemsFor",
      "_PyBorrow_ValueVectorFor", "_PyValue_AdapterBox"),
     BOUNDARY_FILES),
    ("boundary-site", BOX_KINDS_BOUNDARY, (*BOUNDARY_FILES, *STATS_FILES)),
)


def boundary_allowed(path):
    """Whether path (relative to the source tree) may state a boundary
    box kind."""
    return any(fnmatch.fnmatchcase(path, p)
               for p in (*BOUNDARY_FILES, *STATS_FILES))


def box_text(path, text):
    """The test-site and boundary-site findings of one file's text."""
    if path.split("/")[0] not in BOX_SCAN_DIRS:
        return []
    code = strip_c(text)
    line_of = lines_of(code)
    findings = []
    for rule, tokens, allowed in BOX_TEXT_RULES:
        if any(fnmatch.fnmatchcase(path, p) for p in allowed):
            continue
        for m in re.finditer(r"\b(?:{})\b".format("|".join(tokens)), code):
            column = m.start() - code.rfind("\n", 0, m.start())
            findings.append(Finding(
                path, line_of(m.start()), column, rule,
                f"{m[0]} outside the files that may spell it", True))
    return findings


def check_box_text(srcdir):
    findings = []
    for path in source_files(srcdir):
        findings.extend(box_text(
            path, Path(srcdir, path).read_text(errors="replace")))
    return findings


def encoding_names_defined(srcdir):
    """Failures for the ENCODING_NAMES that no value header defines."""
    text = "".join(Path(srcdir, h).read_text() for h in ENCODING_HEADERS)
    return [f"raw-bits: ENCODING_NAMES lists {name}, which "
            f"{' and '.join(ENCODING_HEADERS)} do not define"
            for name in ENCODING_NAMES
            if not re.search(rf"^\s*#\s*define\s+{name}\b", text, re.M)]


# The public layouts that may hold a word (value-api.md, section 1.6, rule
# 2), as (struct, field).
PUBLIC_WORD_FIELDS = frozenset({
    ("PyListObject", "ob_item"),
    ("PyTupleObject", "ob_item"),
    ("PyCellObject", "ob_ref"),
    ("setentry", "key"),
    ("_PyValueIndexPair", "value"),
})
PUBLIC_TOKEN = re.compile(r"->|[A-Za-z_]\w*|[{}();\[\].,*=]")
# A preprocessor line, with its continuations.
DIRECTIVE = re.compile(r"^[ \t]*#(?:\\\n|[^\n])*", re.M)


def field_name(tokens):
    """The name that a member declaration declares: the identifier after
    the first '(*' of a function pointer, else the last identifier before a
    '[' or the end."""
    for i in range(len(tokens) - 2):
        if tokens[i][1] == "(" and tokens[i + 1][1] == "*":
            return tokens[i + 2][1]
    names = [t for _, t in tokens if t == "[" or re.match(r"[A-Za-z_]", t)]
    if "[" in names:
        names = names[:names.index("[")]
    return names[-1] if names else "?"


def tokens_of(code):
    """The tokens of stripped code outside preprocessor lines, as (offset,
    text)."""
    code = DIRECTIVE.sub(blank, code)
    return [(m.start(), m[0]) for m in PUBLIC_TOKEN.finditer(code)]


def public_aliases(code, known=frozenset()):
    """The names that file-scope typedefs in stripped code give to a type
    that names a value type or an alias in known: `typedef PyValue word;`,
    or a typed value signature, `typedef PyValue (*f)(PyValueBorrow);`."""
    aliases = set()
    names = set(VALUE_TYPES) | set(known)
    for m in re.finditer(r"\btypedef\b([^;{}]*);", DIRECTIVE.sub("", code)):
        words = re.findall(r"[A-Za-z_]\w*", m[1])
        if not words:
            continue
        declarator = [(0, t) for t in
                      re.findall(r"->|[A-Za-z_]\w*|[()\[\]*,]", m[1])]
        name = field_name(declarator)
        if name in VALUE_TYPES:
            continue
        if names & set(words) - {name}:
            aliases.add(name)
            names.add(name)
    return aliases


def check_public_header(path, text, aliases=frozenset()):
    """The public-value findings of one public header's text. aliases: the
    value-type aliases that other public headers declare."""
    code = strip_c(text)
    line_of = lines_of(code)
    value_names = set(VALUE_TYPES) | set(aliases)
    value_names |= public_aliases(code, value_names)
    findings = []

    def fail(offset, message):
        findings.append(Finding(path, line_of(offset), 1, "public-value",
                                message, True))

    for m in DIRECTIVE.finditer(code):
        directive = re.match(r"\s*#\s*define\s+\w+(?:\([^)]*\))?", m[0])
        if directive is None:
            continue
        for word in re.finditer(r"[A-Za-z_]\w*", m[0][directive.end():]):
            if word[0] in value_names:
                fail(m.start(), f"a public macro names {word[0]}: it may "
                     "only call exported functions")
                break

    tokens = tokens_of(code)
    # Each frame: [kind ("extern", "record", "function" or "block"), tag,
    # member tokens, fields]. An extern "C" block is file scope.
    frames = []
    statement_start = 0
    for i, (offset, token) in enumerate(tokens):
        scope = [f for f in frames if f[0] != "extern"]
        if token == "{":
            words = [t for _, t in tokens[max(0, i - 3):i]]
            if i and tokens[i - 1][1] == "extern":
                frames.append(["extern", None, [], []])
                statement_start = i + 1
                continue
            if any(w in ("struct", "union") for w in words):
                tag = words[-1] if words[-1] not in ("struct", "union") \
                    else None
                frames.append(["record", tag, [], []])
                continue
            kind = "block"
            if not scope and i and tokens[i - 1][1] == ")":
                kind = "function"
                # The parameters: the parenthesized group before the brace.
                depth = 0
                for k in range(i - 1, statement_start - 1, -1):
                    depth += {")": 1, "(": -1}.get(tokens[k][1], 0)
                    if depth == 0:
                        break
                taken = next((t for _, t in tokens[k:i]
                              if t in value_names), None)
                if taken:
                    fail(offset, f"an inline function takes a {taken}: only "
                         "an exported function may")
            frames.append([kind, None, [], []])
        elif token == "}":
            if not frames:
                continue
            kind, tag, _, fields = frames.pop()
            if kind == "record":
                # The typedef name, if any, follows the brace.
                after = [t for _, t in tokens[i + 1:i + 3]]
                name = after[0] if after and re.match(r"[A-Za-z_]",
                                                     after[0]) else tag
                for field, line in fields:
                    if (name, field) not in PUBLIC_WORD_FIELDS:
                        findings.append(Finding(
                            path, line, 1, "public-value",
                            f"{name}.{field} holds a word or a value "
                            "signature in a public layout", True))
                if frames and frames[-1][0] == "record":
                    frames[-1][2].clear()
            # A record's declaration goes on after its brace.
            if kind != "record" and not [f for f in frames
                                         if f[0] != "extern"]:
                statement_start = i + 1
        elif frames and frames[-1][0] == "record":
            members = frames[-1][2]
            if token == ";":
                where = [o for o, t in members if t in value_names]
                if where:
                    frames[-1][3].append((field_name(members),
                                          line_of(where[0])))
                members.clear()
            else:
                members.append((offset, token))
        elif scope:
            if token in value_names:
                fail(offset, f"a public function body names {token}: an "
                     "inline accessor may only copy a word out of a field")
        elif token == ";":
            # The declaration's tokens outside a record's braces, which the
            # field rule checks.
            statement, depth = [], 0
            for _, t in tokens[statement_start:i]:
                depth += {"{": 1, "}": -1}.get(t, 0)
                if depth == 0 and t != "}":
                    statement.append(t)
            taken = next((t for t in statement if t in value_names), None)
            if taken and declares_object(statement):
                fail(offset, f"a public file-scope declaration of a {taken} "
                     "object: only prototypes and typedefs may name one")
            statement_start = i + 1
    return findings


def declares_object(statement):
    """Whether the tokens of a file-scope declaration, without its ';',
    declare an object rather than a function or a type."""
    if not statement or statement[0] == "typedef":
        return False
    if statement[0] == "PyAPI_DATA" or statement[-1] != ")":
        return True
    # A function's declarator ends with its parameters, after its name;
    # (*name)(...) declares a pointer.
    depth = 0
    for k in range(len(statement) - 1, -1, -1):
        depth += {")": 1, "(": -1}.get(statement[k], 0)
        if depth == 0:
            break
    return not (k > 0 and re.fullmatch(r"[A-Za-z_]\w*", statement[k - 1]))


def public_headers(srcdir):
    include = Path(srcdir, "Include")
    return sorted([*include.glob("*.h"), *include.glob("cpython/*.h")])


def check_public_headers(srcdir):
    texts = {str(path.relative_to(srcdir)): path.read_text(errors="replace")
             for path in public_headers(srcdir)}
    codes = [strip_c(text) for text in texts.values()]
    aliases = set()
    while True:
        found = set().union(*(public_aliases(code, aliases)
                              for code in codes))
        if found <= aliases:
            break
        aliases |= found
    findings = []
    for path, text in texts.items():
        findings.extend(check_public_header(path, text, aliases))
    return findings


SELF_TESTS = {
    # unit text -> (rule, number of failing findings)
    "variadic_format": ("""
        PyObject *f(PyValue v) {
            return PyErr_Format(PyExc_TypeError, "%R", v);
        }""", "variadic", 1),
    "variadic_pointer": ("""
        int f(PyObject *args) {
            PyValue v;
            return PyArg_ParseTuple(args, "O", &v);
        }""", "variadic", 1),
    "variadic_object_at_value_conversion": ("""
        PyObject *f(PyObject *o) {
            PyValueBorrow v = PyValue_FromObjectBorrow(o);
            return PyUnicode_FromFormat("%vR %vS", v, o);
        }""", "variadic", 1),
    "variadic_owner_at_value_conversion": ("""
        PyObject *f(PyValue v) {
            return PyUnicode_FromFormat("%vR", v);
        }""", "variadic", 1),
    "variadic_borrow_format": ("""
        PyObject *f(PyValueBorrow b) {
            return PyErr_Format(PyExc_TypeError, "%R", b);
        }""", "variadic", 1),
    "variadic_borrow_pointer": ("""
        int f(PyObject *args) {
            PyValueBorrow b;
            return PyArg_ParseTuple(args, "O", &b);
        }""", "variadic", 1),
    "value_conversions": ("""
        PyObject *f(PyValue v, PyValueBorrow b, PyObject *o) {
            return PyUnicode_FromFormat("%vR %R %#vT %.*vS",
                                        PyValue_Borrow(v), o, b, 3, b);
        }""", "variadic", 0),
    "slot_read": ("""
        PyObject *f(PyObject *o) {
            return Py_TYPE(o)->tp_as_number->nb_add(o, o);
        }""", "slot-read", 1),
    "slot_call_direct": ("""
        PyObject *f(PyObject *o) {
            return Py_TYPE(o)->tp_as_number->nb_add(o, o);
        }""", "slot-call", 1),
    "slot_call_parenthesized": ("""
        PyObject *f(PyObject *o) {
            return ((Py_TYPE(o)->tp_richcompare))(o, o, Py_EQ);
        }""", "slot-call", 1),
    "slot_call_cast": ("""
        PyObject *f(PyObject *o) {
            return ((binaryfunc)Py_TYPE(o)->tp_as_number->nb_add)(o, o);
        }""", "slot-call", 1),
    "slot_read_comparison": ("""
        int f(PyObject *o) {
            return Py_TYPE(o)->tp_hash == NULL;
        }""", "slot-call", 0),
    "slot_read_argument": ("""
        static void consume(iternextfunc);
        void f(PyObject *o) {
            consume(Py_TYPE(o)->tp_iternext);
        }""", "slot-call", 0),
    "slot_read_copy": ("""
        PyObject *f(PyObject *o) {
            unaryfunc next = Py_TYPE(o)->tp_iternext;
            return NULL;
        }""", "slot-call", 0),
    "raw_bits_access": ("""
        uintptr_t f(PyValue v, PyValueBorrow *p) {
            return v.bits + p->bits;
        }""", "raw-bits", 2),
    "raw_bits_built": ("""
        PyValue f(uintptr_t w, int c) {
            PyValue v = {w};
            PyValue zeros[2] = {0};
            PyValue zero = {0};
            (void)zeros; (void)zero;
            return c ? v : (PyValue){1};
        }""", "raw-bits", 2),
    "raw_bits_casts": ("""
        uintptr_t f(PyObject **objects, PyValue *p, PyValueBorrow *b,
                    void *raw, char *bytes) {
            PyValue *a = (PyValue *)objects;
            PyValueBorrow *c = (PyValueBorrow *)p;
            PyValue *d = (PyValue *)b;
            uintptr_t w = *(uintptr_t *)p;
            PyValue *e = (PyValue *)raw;
            PyValue *g = (PyValue *)(bytes + 8);
            const PyValue *h = (const PyValue *)p;
            void *i = (void *)p;
            unsigned char *j = (unsigned char *)&p[1];
            (void)a; (void)c; (void)d; (void)e; (void)g; (void)h; (void)i;
            (void)j;
            return w;
        }""", "raw-bits", 4),
    "raw_bits_allowed_function": ("""
        static inline PyValue *
        _PyDataStack_Slots(PyObject **address)
        {
            return (PyValue *)address;
        }
        static inline PyValue *
        other_slots(PyObject **address)
        {
            return (PyValue *)address;
        }""", "raw-bits", 1),
    "raw_bits_union": ("""
        union pun { PyValue v; uintptr_t w; };
        """, "raw-bits", 1),
    "raw_bits_union_array": ("""
        union pun { PyValue v[2]; uintptr_t w[2]; };
        """, "raw-bits", 1),
    "raw_bits_union_wrapped": ("""
        struct wrapped { PyValueBorrow v; };
        union pun { struct wrapped s; uintptr_t w; };
        union inline_pun { struct { int n; PyValue v; } s; uintptr_t w; };
        """, "raw-bits", 2),
    "raw_bits_accessors": ("""
        static PyObject dummy_struct;
        int f(PyValue v, PyValueBorrow b) {
            return PyValue_IsNull(v) + PyValue_IsInlineInt(b)
                   + (_PyValue_Word(v) != 0)
                   + _PyValue_IsSameWord(b, _PyValue_SENTINEL(&dummy_struct));
        }""", "raw-bits", 0),
    # A match written in a value header's macro argument is the file's;
    # one written in the macro's body is the header's.
    "macro_arg_cast": ("""
        int f(uintptr_t u) {
            return PyValue_IsNull(*(PyValue *)&u);
        }""", "raw-bits", 1),
    "macro_arg_bits": ("""
        int f(PyValue v) {
            return PyValue_IsNull(PyValue_Wrap((void *)v.bits));
        }""", "raw-bits", 1),
    "macro_body": ("""
        int f(PyValue v) {
            return PyValue_Is(v, PyValue_True) + PyValue_IsNull(PyValue_None);
        }""", "raw-bits", 0),
    "macro_body_nested": ("""
        static PyObject dummy_struct;
        #define DUMMY _PyValue_SENTINEL(&dummy_struct)
        int f(PyValue v) {
            return _PyValue_IsSameWord(v, DUMMY);
        }""", "raw-bits", 0),
    "macro_body_member": ("""
        int f(void) {
            return PyValue_True.bits != 0;
        }""", "raw-bits", 1),
    "local_macro_arg_cast": ("""
        #define IS_NULL_BITS(x) PyValue_IsNull(*(PyValue *)&(x))
        int f(uintptr_t u) {
            return IS_NULL_BITS(u);
        }""", "raw-bits", 1),
    # A byte pointer between two casts does not launder a word.
    "launder_word_from_bits": ("""
        int f(uintptr_t b) {
            return PyValue_IsNull(*(PyValue *)(void *)&b);
        }""", "raw-bits", 1),
    "launder_word_from_byte_object": ("""
        int f(void) {
            char c = 0;
            return PyValue_IsNull(*(PyValue *)&c);
        }""", "raw-bits", 1),
    "launder_bits_through_void": ("""
        uintptr_t f(PyValue v) {
            return *(uintptr_t *)(void *)&v;
        }""", "raw-bits", 1),
    "launder_bits_through_char": ("""
        uintptr_t f(PyValue v) {
            return *(uintptr_t *)(char *)&v;
        }""", "raw-bits", 1),
    "byte_offsets": ("""
        PyValue *f(PyObject *o, Py_ssize_t offset, void *block) {
            PyValue *p = (PyValue *)((char *)o + offset);
            PyValue *q = (PyValue *)block;
            char *r = (char *)&p[1];
            return r == NULL ? q : p;
        }""", "raw-bits", 0),
    "raw_bits_guarded_cast": ("""
        #include "pycore_typecache.h"
        int f(PyObject **objects) {
            return (_PyValueGuarded *)objects != NULL;
        }""", "raw-bits", 1),
    "borrow_field": ("""
        struct holder { PyValueBorrow one; PyValueBorrow many[2]; };
        static PyValueBorrow cached;
        int f(PyValueBorrow p) {
            PyValueBorrow local[2];
            PyValueBorrow q = p;
            (void)local;
            return PyValue_IsNull(q) + PyValue_IsNull(cached);
        }""", "borrow-field", 4),
    "home_copy": ("""
        typedef struct { int status; PyValue value; } pair;
        pair g(void);
        int f(PyValue *p, PyListObject *l, pair *q) {
            PyValue a = p[0];
            PyValue b = *p;
            PyValue c = l->ob_item[1];
            PyValue d = PyValue_DUP(p[0]);
            pair r = g();
            PyValue e = r.value;
            PyValue h = q->value;
            int n = PyValue_IsNull(a) + PyValue_IsNull(b) + PyValue_IsNull(c)
                    + PyValue_IsNull(e) + PyValue_IsNull(h);
            PyValue_CLOSE(d);
            return n;
        }""", "home-copy", 4),
    "compat_body_box": ("""
        Py_VALUE_COMPAT(g) PyObject *f(PyValue v);
        PyObject *f(PyValue v) {
            return PyValue_AsObject(v);
        }""", "compat-body", 1),
    "compat_body_helper_box": ("""
        Py_VALUE_COMPAT(g) PyObject *f(PyValue v);
        PyObject *f(PyValue v) {
            return PyValue_AsKnownObjectSteal(_PyValue_BoxInlineValue(v));
        }""", "compat-body", 1),
    "compat_body_thunk_exit": ("""
        #include "pycore_value_slots.h"
        Py_VALUE_COMPAT(g) PyObject *f(PyValue v);
        PyObject *f(PyValue v) {
            return _PyValue_THUNK_EXIT(v, BOX_SITE_NUMBER);
        }""", "compat-body", 0),
    "compat_body_unmarked": ("""
        PyObject *f(PyValue v) {
            return PyValue_AsObject(v);
        }""", "compat-body", 0),
}
# The second pass (Py_STATS): name -> (body, rule, number of findings, path).
# The path, relative to the source tree, is where the unit is taken to be
# written; it defaults to a file outside the boundary files.
ENTRY_TESTS = {
    "entry_body_core_box": ("""
        Py_VALUE_ENTRY PyObject *f(PyValue v);
        PyObject *f(PyValue v) {
            return PyValue_AsObject(v);
        }""", "compat-body", 1),
    "entry_body_adapter_box": ("""
        #include "pycore_value_slots.h"
        Py_VALUE_ENTRY PyObject *f(PyValue v);
        PyObject *f(PyValue v) {
            return _PyValue_AdapterBox(v, BOX_SITE_NUMBER, f);
        }""", "compat-body", 1),
    "entry_body_thunk": ("""
        #include "pycore_value_slots.h"
        Py_VALUE_ENTRY PyObject *f(PyValue v);
        PyObject *f(PyValue v) {
            return _PyValue_THUNK_NEW(v, BOX_SITE_NUMBER);
        }""", "compat-body", 0),
    "entry_body_unmarked_core_box": ("""
        PyObject *f(PyValue v) {
            return PyValue_AsObject(v);
        }""", "compat-body", 0),
    "capture_in_unmarked": ("""
        #include "pycore_value_slots.h"
        PyObject *f(PyValue v) {
            return _PyValue_THUNK_EXIT(v, BOX_SITE_NUMBER);
        }""", "compat-capture", 1),
    "capture_new_in_unmarked": ("""
        #include "pycore_value_slots.h"
        PyObject *f(PyValue v) {
            return _PyValue_THUNK_NEW(v, BOX_SITE_NUMBER);
        }""", "compat-capture", 1),
    "capture_return_address_in_unmarked": ("""
        const void *f(void) { return __builtin_return_address(0); }
        """, "compat-capture", 1),
    "capture_in_entry": ("""
        #include "pycore_value_slots.h"
        Py_VALUE_ENTRY PyObject *f(PyValue v);
        PyObject *f(PyValue v) {
            return _PyValue_THUNK_EXIT(v, BOX_SITE_NUMBER);
        }
        Py_VALUE_ENTRY const void *g(void);
        const void *g(void) { return __builtin_return_address(0); }
        """, "compat-capture", 0),
    "entry_calls_entry": ("""
        Py_VALUE_ENTRY int g(void);
        int g(void) { return 1; }
        Py_VALUE_ENTRY int f(void);
        int f(void) { return g(); }""", "compat-capture", 1),
    "unmarked_calls_entry": ("""
        Py_VALUE_ENTRY int g(void);
        int g(void) { return 1; }
        int f(void) { return g(); }""", "compat-capture", 0),
    "kind_literal_outside": ("""
        PyObject *f(PyValue v) {
            return (PyValue_AsObject)(PyValue_Borrow(v), _PyBox_AT(
                BOX_SITE_NUMBER, BOX_KIND_CALL_ADAPTER, NULL));
        }""", "boundary-site", 1),
    "kind_core_outside": ("""
        PyObject *f(PyValue v) {
            return (PyValue_AsObject)(PyValue_Borrow(v),
                                      _PyBox_CORE(BOX_SITE_NUMBER));
        }""", "boundary-site", 0),
    "kind_thunk_macro_outside": ("""
        #include "pycore_value_slots.h"
        Py_VALUE_ENTRY PyObject *f(PyValue v);
        PyObject *f(PyValue v) {
            return _PyValue_THUNK_NEW(v, BOX_SITE_NUMBER);
        }""", "boundary-site", 0),
}
# The rules whose findings report and do not fail.
REPORT_RULES = frozenset({"slot-read", "home-copy"})
SELF_TEST_HEADER = '#include "Python.h"\n#include "pycore_value_api.h"\n'

# The text of raw-bits: name -> (path, text, scan_bits, number of findings).
RAW_BITS_TEXT_TESTS = {
    "test_module_bits": ("Modules/_testinternalcapi/spam.c",
                         "uintptr_t f(PyValue v) { return v.bits; }\n",
                         True, 1),
    "encoding_test_bits": ("Modules/_testinternalcapi/list.c",
                           "uintptr_t f(PyValue v) { return v.bits; }\n",
                           True, 0),
    "macro_bits": ("Include/internal/pycore_spam.h",
                   "#define SPAM_BITS(v) ((v).bits)\n", True, 1),
    "public_macro_bits": ("Include/cpython/spam.h",
                          "#define PyValue_BITS(v) \\\n    ((v)->bits)\n",
                          True, 1),
    "literal": ("Objects/spam.c",
                "static PyValueBorrow f(void) { return (PyValueBorrow){0}; }\n",
                False, 1),
    "constants": ("Objects/spam.c", """
        int f(PyValue v, uintptr_t w) {
            return (w & Py_TAG_REFCNT) + (_PyValue_BITS(v) > Py_INT_BASE);
        }
        """, False, 3),
    "comments_and_strings": ("Include/spam.h", """
        /* v.bits, (PyValue){0} and Py_TAG_BITS */
        static const char *s = "p->bits Py_INT_SHIFT";
        // (PyValueBorrow){x}
        """, True, 0),
    "unit_bits_left_to_clang_query": (
        "Objects/spam.c", "int f(PyValue v) { return v.bits != 0; }\n",
        False, 0),
    "unchecked_unit_bits": (
        "Modules/spam.c", "int f(PyValue v) { return v.bits != 0; }\n",
        True, 1),
    "value_header": ("Include/internal/pycore_value.h",
                     "#define X(v) ((v).bits | Py_TAG_REFCNT)\n", True, 0),
    "jit": ("Python/jit_spam.c", "int f(PyValue v) { return v.bits & 3; }\n",
            True, 0),
    "encoding_function": ("Include/internal/pycore_interpframe.h", """\
static inline PyValue
_PyIR_AdoptTemp(PyValueBorrow view)
{
    PyValue owner = {view.bits};
    return owner;
}

static inline uintptr_t
spam(PyValueBorrow v)
{
    return v.bits;
}
""", True, 1),
}

# The text of window-read and vectorcall-thunk: name -> (path, text, rule,
# number of findings).
CALL_TEXT_TESTS = {
    "window_read_raw": ("Objects/spam.c", """
        static PyValue
        spam(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs,
             PyObject *kwnames)
        {
            PyValue x = args[0];
            return x;
        }""", "window-read", 1),
    "window_read_borrow": ("Objects/spam.c", """
        static PyValue
        spam(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs,
             PyObject *kwnames)
        {
            PyValueBorrow x = PyValue_Borrow(args[0]);
            PyValue y = PyValue_DUP(args[nargs - 1]);
            _PyValue_WindowPut(&y, args[1]);
            const PyValue *rest = &args[1];
            return PyValue_DUP(x);
        }""", "window-read", 0),
    "window_read_nested": ("Objects/spam.c", """
        static PyValue
        spam(PyThreadState *tstate, PyObject *callable, const PyValue *args,
             size_t nargsf, PyObject *kwnames)
        {
            return f(PyValue_Borrow(args[0]), g(args[1]));
        }""", "window-read", 1),
    "window_read_other_function": ("Objects/spam.c", """
        static PyValue
        spam(PyValue *args)
        {
            return args[0];
        }
        static int
        eggs(const PyValue *args)
        {
            return PyValue_IsNull(args[0]);
        }""", "window-read", 0),
    "thunk_in_core": ("Objects/spam.c", """
        void f(PyObject *o) { _PyValue_VectorcallThunk(o, NULL, 0, NULL); }
        """, "vectorcall-thunk", 1),
    "thunk_in_creator": ("Objects/methodobject.c", """
        vectorcall = _PyValue_VectorcallThunk;
        """, "vectorcall-thunk", 0),
    "thunk_in_comment": ("Objects/spam.c", """
        /* _PyValue_VectorcallThunk */ // _PyFunction_Vectorcall
        """, "vectorcall-thunk", 0),
    "function_vectorcall_in_core": ("Objects/spam.c", """
        void f(void) { vectorcall = _PyFunction_Vectorcall; }
        """, "vectorcall-thunk", 1),
    "function_vectorcall_allowed": ("Python/generated_cases.c.h", """
        if (func->vectorcall == _PyFunction_Vectorcall) { }
        """, "vectorcall-thunk", 0),
}

# Box text rules: name -> (path, text, number of findings).
BOX_TEXT_TESTS = {
    "test_site_outside": ("Objects/spam.c",
                          "int x = BOX_SITE_TEST;\n", 1),
    "test_site_in_test_module": ("Modules/_testinternalcapi/list.c",
                                 "int x = BOX_SITE_TEST;\n", 0),
    "test_site_in_pystats": ("Python/pystats.c",
                             "int x = BOX_SITE_TEST;\n", 0),
    "test_site_in_comment": ("Objects/spam.c",
                             '/* BOX_SITE_TEST */ char *s = "BOX_SITE_TEST";\n',
                             0),
    "test_site_in_lib_test": ("Lib/test/test_cext/spam.c",
                              "int x = BOX_SITE_TEST;\n", 0),
    "foreign_outside": ("Python/spam.c",
                        "const void *x = _Py_BOX_OWNER_FOREIGN;\n", 1),
    "foreign_in_ctypes": ("Modules/_ctypes/cfield.c",
                          "const void *x = _Py_BOX_OWNER_FOREIGN;\n", 0),
    "foreign_in_value_boxes": ("Modules/_testinternalcapi/value_boxes.c",
                               "const void *x = _Py_BOX_OWNER_FOREIGN;\n",
                               0),
    "foreign_in_other_test_file": ("Modules/_testinternalcapi/list.c",
                                   "const void *x = _Py_BOX_OWNER_FOREIGN;\n",
                                   1),
    "for_form_outside": ("Objects/setobject.c",
                         "x = _PyValue_AsObjectFor(v, at);\n", 1),
    "borrow_for_outside": ("Objects/tupleobject.c",
                           "x = _PyBorrow_TupleItemsFor(v, at);\n"
                           "y = _PyBorrow_ValueVectorFor(v, at);\n", 2),
    "adapter_box_outside": ("Objects/typeobject.c",
                            "x = _PyValue_AdapterBox(v);\n", 1),
    "for_form_in_call": ("Objects/call.c",
                         "x = _PyValue_AsObjectFor(v, at);\n", 0),
    "for_form_in_value_api": ("Include/internal/pycore_value_api.h",
                              "x = _PyValue_AdapterBox(v);\n", 0),
    "kind_literal_outside": (
        "Objects/typeobject.c",
        "x = (PyValue_AsObject)(v, _PyBox_AT(s, BOX_KIND_CALL_ADAPTER, o));\n",
        1),
    "kind_thunk_and_slot_outside": (
        "Python/ceval.c",
        "a = BOX_KIND_THUNK;\nb = BOX_KIND_SLOT_ADAPTER;\n", 2),
    "kind_core_outside": ("Objects/typeobject.c",
                          "x = _PyBox_AT(s, BOX_KIND_CORE, NULL);\n", 0),
    "kind_in_borrow": ("Python/borrow.c",
                       "x = _PyBox_AT(s, BOX_KIND_CALL_ADAPTER, o);\n", 0),
    "kind_in_comment": ("Objects/typeobject.c",
                        "/* BOX_KIND_THUNK */ // BOX_KIND_CALL_ADAPTER\n", 0),
    "kind_longer_name": ("Objects/typeobject.c",
                         "int BOX_KIND_THUNKS = 0;\n", 0),
}

# Public header texts -> number of public-value findings.
PUBLIC_VALUE_TESTS = {
    "opaque_value_slots": ("""
        typedef void (*_Py_valuefunc)(void);
        typedef struct _PyValueSlots {
            _Py_valuefunc nb_add;
            _Py_valuefunc hash;
        } _PyValueSlots;
        struct _typeobject {
            void *_tp_cache;
            _PyValueSlots tp_value;
            Py_ssize_t tp_value_vectorcall_offset;
            _Py_valuefunc tp_value_vectorcall;
        };""", 0),
    "frozen_word_fields": ("""
        typedef struct {
            PyObject_VAR_HEAD
            PyValue *ob_item;
            Py_ssize_t allocated;
        } PyListObject;
        typedef struct {
            PyObject_HEAD
            PyValue ob_ref;
        } PyCellObject;""", 0),
    "exported_value_functions": ("""
        PyAPI_FUNC(PyValue) PyList_GetValue(PyObject *op, Py_ssize_t i);
        PyAPI_FUNC(int) PyList_SetValue(PyObject *op, Py_ssize_t i,
                                        PyValue value);
        static inline PyValue
        PyList_GET_VALUE(PyObject *op, Py_ssize_t i) {
            return _PyList_CAST(op)->ob_item[i];
        }""", 0),
    "typed_value_slot": ("""
        typedef struct _PyValueSlots {
            PyValue (*nb_add)(PyValueBorrow, PyValueBorrow);
        } _PyValueSlots;""", 1),
    "word_field": ("""
        struct _typeobject {
            void *_tp_cache;
            PyValue tp_cached_value;
        };""", 1),
    "inline_decoding": ("""
        static inline int
        PyValue_IsTagged(PyValue value) {
            return (value.bits & 3) != 0;
        }""", 1),
    "inline_building": ("""
        #define PyValue_FROM_BITS(B) ((PyValue){ .bits = (B) })
        """, 1),
    "alias_field": ("""
        typedef PyValue word;
        typedef struct { PyObject_HEAD word w; } PySpamObject;""", 1),
    "alias_signature_field": ("""
        typedef PyValue (*valuefunc)(PyValueBorrow);
        struct spam { valuefunc f; };""", 1),
    "alias_of_alias_field": ("""
        typedef PyValue word;
        typedef word words[2];
        struct spam { words w; };""", 1),
    "integer_pun": ("""
        static inline uintptr_t
        PyValue_WORD(PyValue word)
        {
            return *(uintptr_t *)&word;
        }""", 1),
    "body_names_value": ("""
        static inline PyValue
        PySpam_GET(PyObject *op)
        {
            PyValue v = ((PySpamObject *)op)->ob_item[0];
            return v;
        }""", 1),
    "comments_skipped": ("""
        /* struct { PyValue v; } and (PyValue){0} */
        // a PyValue in a comment
        struct spam { int n; };""", 0),
    "extern_c_block": ("""
        #ifdef __cplusplus
        extern "C" {
        #endif
        PyAPI_FUNC(PyValue) PyList_GetValue(PyObject *, Py_ssize_t);
        static inline PyValue
        PySpam_GET(PyObject *op)
        {
            return ((PySpamObject *)op)->ob_item[0];
        }
        #ifdef __cplusplus
        }
        #endif""", 0),
    "callback_parameter": ("""
        typedef int (*PySpam_Callback)(PyObject *, PyValueBorrow key);
        PyAPI_FUNC(int) PySpam_AddWatcher(PySpam_Callback callback);""", 0),
    "file_scope_value": ("""
        PyAPI_DATA(PyValue) _PySpam_Word;
        extern PyValueBorrow _PySpam_View;
        extern PyValue (*_PySpam_Hook)(PyValueBorrow);
        PyAPI_FUNC(PyValue) PySpam_GetValue(PyObject *);
        typedef PyValue PySpamWord;
        typedef struct { uintptr_t bits; } PyValue;
        struct spam { int n; };
        static struct { int n; } spam_counts;""", 3),
}


# The functions that build a type before PyType_Ready finishes, and the
# writer after it (slot-write).
SLOT_WRITE_FUNCTIONS = frozenset({
    "type_new_set_slots", "inherit_slots", "type_ready_set_hash",
    "initialize_static_fields", "_PyType_WriteSlotPair",
})
SLOT_WRITE = re.compile(r"->\s*({})\s*=(?!=)".format(
    "|".join(sorted(SLOT_FAMILIES, key=len, reverse=True))))
FUNCTION_HEAD = re.compile(r"^([A-Za-z_]\w*)\s*\(", re.M)
SLOT_WRITE_DIRS = ("Objects", "Python")
PAIR_TABLE_UNIT = "Objects/value_pairs.c"


def slot_write_text(path, text):
    """The slot-write findings of the text of one C file."""
    code = strip_c(text)
    line_of = lines_of(code)
    heads = [(m.start(), m[1]) for m in FUNCTION_HEAD.finditer(code)]
    findings = []
    for m in SLOT_WRITE.finditer(code):
        i = bisect.bisect_right(heads, (m.start(), "")) - 1
        function = heads[i][1] if i >= 0 else None
        if function in SLOT_WRITE_FUNCTIONS:
            continue
        column = m.start() - code.rfind("\n", 0, m.start())
        findings.append(Finding(
            path, line_of(m.start()), column, "slot-write",
            f"writes the object slot {m[1]} in {function or 'file scope'}; "
            "use _PyType_WriteSlotPair()", True))
    return findings


def pair_table_text(path, text):
    """The pair-table findings of the text of the pair table's unit: a
    brace at file scope that opens a function body."""
    code = re.sub(r"^[ \t]*#(?:\\\n|[^\n])*", blank, strip_c(text),
                  flags=re.M)
    line_of = lines_of(code)
    findings = []
    depth = 0
    for i, c in enumerate(code):
        if c == "{":
            if depth == 0 and code[:i].rstrip().endswith(")"):
                column = i - code.rfind("\n", 0, i)
                findings.append(Finding(
                    path, line_of(i), column, "pair-table",
                    "a function body in the pair table's unit", True))
            depth += 1
        elif c == "}":
            depth -= 1
    return findings


def check_slot_writes(srcdir):
    findings = []
    for directory in SLOT_WRITE_DIRS:
        for path in sorted(Path(srcdir, directory).rglob("*.c")):
            rel = str(path.relative_to(srcdir))
            findings.extend(slot_write_text(
                rel, path.read_text(errors="replace")))
    pairs = Path(srcdir, PAIR_TABLE_UNIT)
    findings.extend(pair_table_text(PAIR_TABLE_UNIT, pairs.read_text()))
    return findings


SLOT_WRITE_TESTS = {
    "slot_write_after_ready": ("""
        static PyObject *
        f(PyObject *self)
        {
            Py_TYPE(self)->tp_getattro = NULL;
            Py_TYPE(self)->tp_as_number->nb_add = NULL;
            if (Py_TYPE(self)->tp_hash == NULL) { return NULL; }
            return self;
        }""", 2),
    "slot_write_construction": ("""
        static void
        inherit_slots(PyTypeObject *type, PyTypeObject *base)
        {
            type->tp_getattro = base->tp_getattro;  /* x->tp_hash = 0 */
        }
        static PyTypeObject T = {.tp_hash = NULL};""", 0),
}

PAIR_TABLE_TESTS = {
    "pair_table_data": ("""
        #define Py_VALUE_PAIR_TABLE 1
        #include "Python.h"
        const _PyValueSlotPair _PyValueSlot_Pairs[] = {
            {_Py_VALUE_SLOT_INDEX_hash, (void *)PyObject_GenericHash,
             _Py_VALUE_SLOT_INIT(hash, _PyValue_GenericHash)},
        };
        const size_t _PyValueSlot_PairCount = 1;""", 0),
    "pair_table_function": ("""
        static int
        f(void)
        {
            return 0;
        }""", 1),
}


def text_self_test():
    """Run the self-tests of the text rules. Return a list of failures."""
    failures = encoding_names_defined(SRCDIR)
    cases = [(name, "raw-bits", raw_bits_text(path, text, scan_bits), count)
             for name, (path, text, scan_bits, count)
             in RAW_BITS_TEXT_TESTS.items()]
    cases += [(name, "box-text", box_text(path, text), count)
              for name, (path, text, count) in BOX_TEXT_TESTS.items()]
    cases += [(name, "public-value", check_public_header(name, text), count)
              for name, (text, count) in PUBLIC_VALUE_TESTS.items()]
    cases += [(name, rule, window_read_text(path, text)
               + vectorcall_thunk_text(path, text), count)
              for name, (path, text, rule, count) in CALL_TEXT_TESTS.items()]
    cases += [(name, "slot-write",
               slot_write_text(name, textwrap.dedent(text)), count)
              for name, (text, count) in SLOT_WRITE_TESTS.items()]
    cases += [(name, "pair-table",
               pair_table_text(name, textwrap.dedent(text)), count)
              for name, (text, count) in PAIR_TABLE_TESTS.items()]
    for name, rule, got, count in cases:
        got = ([f for f in got if f.rule == rule]
               if rule not in ("raw-bits", "box-text") else got)
        if len(got) != count:
            failures.append(f"self-test {name}: expected {count} {rule} "
                            f"findings, got {len(got)}: "
                            + "; ".join(str(f) for f in got))
        else:
            print(f"self-test {name}: {len(got)} {rule} finding(s), "
                  "as expected")
    return failures

# The typed borrow (pyjit/design/proposals/register-ir-v2-a5b.md, section 3.1
# and Appendix A). Each unit with a pattern must fail to compile with an
# error that the pattern matches (the GCC or the Clang text, in the C
# locale), and a diagnostic must name its line marked "rejected". A unit
# without a pattern must compile without a diagnostic. BORROW_TESTS are C
# units; BORROW_CXX_TESTS are C++ units, which pin the overloads that are the
# view in C++. Lib/test/test_tools/test_check_value_api.py compiles both
# with the compilers and flags of the build.
BORROW_TESTS = {
    "borrow_stored_to_owner": ("""
        void f(PyValueBorrow b, PyValue *slot) {
            slot[1] = b;  /* rejected */
        }""", r"incompatible types when assigning to type 'PyValue' from "
              r"type 'PyValueBorrow'|assigning to 'PyValue' from "
              r"incompatible type 'PyValueBorrow'"),
    "borrow_closed": ("""
        void f(PyValueBorrow b) {
            PyValue_CLOSE(b);  /* rejected */
        }""", r"incompatible type for argument 1 of 'PyValue_CLOSE'"
              r"|passing 'PyValueBorrow' to parameter of incompatible type "
              r"'PyValue'"),
    "bits_read": ("""
        int f(uintptr_t bits) {
            return PyValue_IsInlineInt(bits);  /* rejected */
        }""", r"'_Generic' selector of type .* is not compatible with any "
              r"association|controlling expression type .* not compatible "
              r"with any generic association type"),
    "closer_as_reader": ("""
        void (*f)(PyValueBorrow) = PyValue_CLOSE;  /* rejected */
        """, r"initialization of 'void \(\*\)\(PyValueBorrow\)' from "
             r"incompatible pointer type|incompatible function pointer "
             r"types initializing 'void \(\*\)\(PyValueBorrow\)'"),
    "borrow_address_as_owner": ("""
        void f(PyValueBorrow b) {
            PyValue *p = &b;  /* rejected */
            (void)p;
        }""", r"initialization of 'PyValue \*' from incompatible pointer "
              r"type 'PyValueBorrow \*'|incompatible pointer types "
              r"initializing 'PyValue \*' with an expression of type "
              r"'PyValueBorrow \*'"),
    "borrow_cleared": ("""
        void f(PyValueBorrow b) {
            PyValue_CLEAR(b);  /* rejected */
        }""", r"initialization of 'PyValue \*' from incompatible pointer "
              r"type 'PyValueBorrow \*'|incompatible pointer types "
              r"initializing 'PyValue \*' with an expression of type "
              r"'PyValueBorrow \*'"),
    # Unit 7: a handler's store of a view to dst, against the real macros
    "borrow_stored_to_dst": (f"""
        #include "{SRCDIR}/Python/ceval.h"
        #include "{SRCDIR}/Python/ceval_macros.h"
        void f(PyThreadState *tstate, _PyInterpreterFrame *frame,
               PyValue *slots, PyValueBorrow b) {{
            PyValue ir_displaced = PyValue_NULL;
            IR_STORE_DST(0, b);  /* rejected */
            (void)ir_displaced;
        }}""", r"passing argument 1 of '_PyValue_Publish' from incompatible "
              r"pointer type|incompatible pointer types passing "
              r"'PyValueBorrow \*' to parameter of type 'PyValue \*'"),
    # Section 5.6: a producer's borrow passed to a stealing setter, against
    # the real headers.
    "borrow_element_stored": ("""
        #include "pycore_tuple.h"
        void f(PyObject *t, PyObject *u) {
            PyTuple_SET_VALUE(t, 0, PyTuple_GET_VALUE(u, 0));  /* rejected */
        }""", r"incompatible type for argument 3 of 'PyTuple_SET_VALUE'"
              r"|passing 'PyValueBorrow' to parameter of incompatible type "
              r"'PyValue'"),
    "borrow_taken_by_dict": ("""
        #include "pycore_dict.h"
        int f(PyDictObject *d, PyValue k, PyObject *o) {
            return _PyDict_SetValue_Take2(
                d, k, PyValue_FromObjectBorrow(o));  /* rejected */
        }""", r"incompatible type for argument 3 of '_PyDict_SetValue_Take2'"
              r"|passing 'PyValueBorrow' to parameter of incompatible type "
              r"'PyValue'"),
    "borrow_returned_as_owner": ("""
        PyValue f(PyValueBorrow b) {
            return b;  /* rejected */
        }""", r"incompatible types when returning type 'PyValueBorrow' but "
              r"'PyValue' was expected|returning 'PyValueBorrow' from a "
              r"function with incompatible result type 'PyValue'"),
    "readers": ("""
        typedef bool (*reader)(PyValueBorrow);
        int f(PyValue o, PyValueBorrow b, const PyValue *p, Py_ssize_t i) {
            reader test = &PyValue_IsInlineInt;
            PyValueBorrow c = PyValue_Borrow(o);
            PyValue owner = PyValue_XDUP(b);
            int n = PyValue_IsInlineInt(o) + PyValue_IsInlineInt(b)
                    + PyValue_IsInlineInt(*p) + PyValue_IsNull(p[i])
                    + PyValue_Is(o, c) + PyValue_IsNone(PyValue_Borrow(c))
                    + test(b) + PyValue_IsNull(owner);
            PyValue_XCLOSE(owner);
            return n;
        }""", None),
}
BORROW_CXX_TESTS = {
    "cxx_borrow_stored_to_owner": ("""
        void f(PyValueBorrow b, PyValue *slot) {
            slot[1] = b;  /* rejected */
        }""", r"no match for 'operator=' \(operand types are 'PyValue' and "
              r"'PyValueBorrow'\)|no viable overloaded '='"),
    "cxx_bits_read": ("""
        int f(uintptr_t bits) {
            return PyValue_IsInlineInt(bits);  /* rejected */
        }""", r"no matching function for call to 'PyValue_Borrow"),
    "cxx_readers": ("""
        int f(PyValue o, PyValueBorrow b, const PyValue *p, Py_ssize_t i) {
            bool (*test)(PyValueBorrow) = &PyValue_IsInlineInt;
            PyValueBorrow c = PyValue_Borrow(o);
            PyValue owner = PyValue_XDUP(b);
            int n = PyValue_IsInlineInt(o) + PyValue_IsInlineInt(b)
                    + PyValue_IsInlineInt(*p) + PyValue_IsNull(p[i])
                    + PyValue_Is(o, c) + PyValue_IsNone(PyValue_Borrow(c))
                    + test(b) + PyValue_IsNull(owner);
            PyValue_XCLOSE(owner);
            return n;
        }""", None),
}


def self_test(clang_query, template, compiler, cxx, jobs):
    """Run the self-tests with the flags of template, a core unit's entry.
    Return a list of failures."""
    failures = []
    with tempfile.TemporaryDirectory() as tmp:
        entries = []
        expected = {}
        for name, (body, rule, count) in SELF_TESTS.items():
            path = Path(tmp, f"{name}.c")
            path.write_text(SELF_TEST_HEADER + body + "\n")
            entries.append(({**template, "file": str(path)}, "core"))
            expected[str(path)] = (name, rule, count)
        findings, errors = check_units(clang_query, entries, jobs)
        failures.extend(errors)
        for path, (name, rule, count) in expected.items():
            got = [f for f in findings
                   if f.path == display_path(path, template["directory"])
                   and f.rule == rule]
            if len(got) != max(count, 0) or (count and not all(
                    f.fails or rule in REPORT_RULES for f in got)):
                failures.append(f"self-test {name}: expected {count} "
                                f"{rule} findings, got {len(got)}: "
                                + "; ".join(str(f) for f in got))
            else:
                print(f"self-test {name}: {len(got)} {rule} finding(s), "
                      "as expected")

        entries = []
        expected = {}
        for name, (body, rule, count) in ENTRY_TESTS.items():
            path = Path(tmp, f"{name}.c")
            path.write_text(SELF_TEST_HEADER + body + "\n")
            entries.append(({**template, "file": str(path)}, "core"))
            expected[str(path)] = (name, rule, count)
        findings, errors = check_units(clang_query, entries, jobs,
                                       ENTRY_QUERY, ENTRY_EXTRA)
        failures.extend(errors)
        for path, (name, rule, count) in expected.items():
            got = [f for f in findings
                   if f.path == display_path(path, template["directory"])
                   and f.rule == rule]
            if len(got) != count:
                failures.append(f"self-test {name}: expected {count} "
                                f"{rule} findings, got {len(got)}: "
                                + "; ".join(str(f) for f in got))
            else:
                print(f"self-test {name}: {len(got)} {rule} finding(s), "
                      "as expected")

        # Py_VALUE_COMPAT: a call of a marked function from a value-native
        # unit does not compile; from a module unit it does.
        flags = compile_flags(template)
        call = "void f(void) { _PyValue_CompatProbe(); }\n"
        # Numeric compatibility calls obey the same core/module boundary.
        numbers_call = "long f(void) { return PyLong_AsLong(0); }\n"
        for name, prefix, body, should_fail in (
                ("unavailable_core", "", call, True),
                ("unavailable_module", "#define Py_BUILD_CORE_MODULE 1\n",
                 call, False),
                ("unavailable_numbers", "", numbers_call, True),
                ("available_numbers_module", "#define Py_BUILD_CORE_MODULE 1\n",
                 numbers_call, False)):
            path = Path(tmp, f"{name}.c")
            path.write_text(prefix + SELF_TEST_HEADER + body)
            proc = subprocess.run(
                [*shlex.split(compiler), *flags, "-fsyntax-only", str(path)],
                cwd=template["directory"], capture_output=True, text=True)
            failed = proc.returncode != 0 and "unavailable" in proc.stderr
            if failed != should_fail or (not should_fail
                                         and proc.returncode != 0):
                failures.append(f"self-test {name}: compiler exit "
                                f"{proc.returncode}: {proc.stderr.strip()}")
            else:
                print(f"self-test {name}: "
                      f"{'rejected' if should_fail else 'compiled'}, "
                      "as expected")
        borrow_units = [(BORROW_TESTS, compiler, flags, ".c")]
        if cxx:
            borrow_units.append(
                (BORROW_CXX_TESTS, cxx, cxx_flags(flags), ".cpp"))
        for units, command, unit_flags, suffix in borrow_units:
            for name, (body, pattern) in units.items():
                failure = borrow_unit_failure(
                    tmp, template["directory"], command, unit_flags, name,
                    body, pattern, suffix)
                if failure:
                    failures.append(failure)
                else:
                    print(f"self-test {name}: "
                          f"{'compiled' if pattern is None else 'rejected'}"
                          ", as expected")
    return failures


def cxx_flags(flags):
    """The options of a C unit's flags that a C++ unit of the core takes:
    the include paths and macros, with C++11 and -Wall."""
    return [*(f for f in flags if f.startswith(("-I", "-D"))),
            "-std=c++11", "-Wall"]


def borrow_unit_failure(tmp, directory, compiler, flags, name, body,
                        pattern, suffix):
    """Compile one typed-borrow unit in directory, with compiler and flags.
    Return the failure, or None if the unit behaves as its pattern says."""
    path = Path(tmp, f"{name}{suffix}")
    text = SELF_TEST_HEADER + body + "\n"
    path.write_text(text)
    proc = subprocess.run(
        [*shlex.split(compiler), *flags, "-fsyntax-only", str(path)],
        cwd=directory, capture_output=True, text=True,
        env={**os.environ, "LC_ALL": "C"})
    if pattern is None:
        ok = proc.returncode == 0 and str(path) not in proc.stderr
    else:
        line = next(i for i, s in enumerate(text.splitlines(), 1)
                    if "rejected" in s)
        ok = (proc.returncode != 0
              and re.search(rf"error: .*(?:{pattern})", proc.stderr)
              and f"{path}:{line}:" in proc.stderr)
    if ok:
        return None
    return (f"self-test {name}: compiler exit {proc.returncode}: "
            f"{proc.stderr.strip()}")


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--cdb", default="compile_commands.json",
                        help="compile database (default: %(default)s)")
    parser.add_argument("--clang-query", help="clang-query to run")
    parser.add_argument("--cc", help="compiler for the Py_VALUE_COMPAT "
                        "and typed-borrow self-tests (default: the "
                        "database's)")
    parser.add_argument("--cxx", help="C++ compiler for the typed-borrow "
                        "self-tests (default: none, and the C++ units are "
                        "not compiled)")
    parser.add_argument("-j", "--jobs", type=int, default=os.cpu_count())
    parser.add_argument("--self-test-only", action="store_true")
    parser.add_argument("--intercept", nargs=argparse.REMAINDER,
                        metavar="BUILD_COMMAND",
                        help="record the database with intercept-build "
                        "while BUILD_COMMAND runs, and check nothing")
    parser.add_argument("--intercept-build", help="intercept-build to run")
    parser.add_argument("files", nargs="*",
                        help="check only these units of the database")
    args = parser.parse_args()

    if args.intercept:
        intercept = find_tool("intercept-build", args.intercept_build)
        return subprocess.run(
            [intercept, "--cdb", args.cdb, *args.intercept]).returncode
    clang_query = find_tool("clang-query", args.clang_query)
    with open(args.cdb) as f:
        database = json.load(f)
    entries = {}
    for entry in database:
        kind = unit_kind(entry)
        if kind is not None:
            path = os.path.normpath(Path(entry["directory"], entry["file"]))
            entries[path] = (entry, kind)
    if not entries:
        sys.exit(f"check_value_api: no value-native unit in {args.cdb}")
    template = next(entry for entry, kind in entries.values()
                    if entry["file"].endswith("Objects/object.c"))
    compiler = args.cc or entry_arguments(template)[0]

    failures = text_self_test()
    failures += self_test(clang_query, template, compiler, args.cxx,
                          args.jobs)
    if failures or args.self_test_only:
        for failure in failures:
            print(failure)
        return 1 if failures else 0

    checked = {display_path(entry["file"], entry["directory"])
               for entry, kind in entries.values()}
    text_findings = (check_raw_bits_text(SRCDIR, checked)
                     + check_public_headers(SRCDIR)
                     + check_call_text(SRCDIR)
                     + check_box_text(SRCDIR)
                     + check_slot_writes(SRCDIR))
    entry_entries = []
    for entry in database:
        path = display_path(entry["file"], entry["directory"])
        if path in ENTRY_UNITS:
            entry_entries.append((entry, "core"))
    if args.files:
        wanted = {os.path.normpath(os.path.abspath(f)) for f in args.files}
        entries = {p: e for p, e in entries.items() if p in wanted}
        entry_entries = [(e, k) for e, k in entry_entries
                         if os.path.normpath(Path(e["directory"], e["file"]))
                         in wanted]
        text_findings = [f for f in text_findings
                         if str(SRCDIR / f.path) in wanted]
    findings, errors = check_units(clang_query, entries.values(), args.jobs)
    entry_findings, entry_errors = check_units(
        clang_query, entry_entries, args.jobs, ENTRY_QUERY, ENTRY_EXTRA)
    errors += entry_errors
    unchecked = [] if args.files else unchecked_value_units(SRCDIR, checked)
    findings = sorted({*findings, *entry_findings, *text_findings,
                       *deferred_units(entries.values()), *unchecked})
    for finding in findings:
        print(finding)
    for error in errors:
        print(error)
    failing = [f for f in findings if f.fails]
    by_rule = {}
    for finding in findings:
        by_rule[finding.rule] = by_rule.get(finding.rule, 0) + 1
    summary = ", ".join(f"{rule} {n}" for rule, n in sorted(by_rule.items()))
    print(f"check_value_api: {len(entries)} value-native units; "
          f"{len(failing)} failing findings; {summary or 'no findings'}; "
          f"{len(errors)} parse errors")
    return 1 if failing or errors else 0


if __name__ == "__main__":
    sys.exit(main())
