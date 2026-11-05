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
* slot-read: a read of an object slot field that has a value form
  (tp_as_number->nb_add, tp_richcompare, tp_iternext, tp_call and the rest).
  A read of a slot whose family is in MARKED_SLOT_FAMILIES fails; other reads
  are listed. Units that define Py_VALUE_ADAPTER are skipped.

The functions that Py_VALUE_COMPAT marks need no rule here: calling one from
a value-native unit is a compile error.

The script reads compile_commands.json, which intercept-build records while
the units compile, and runs clang-query over every value-native unit. Before
that it runs its self-tests: one-line units that must make each rule fire,
a unit that calls the marked probe declaration in
Include/internal/pycore_value_api.h, which must fail to compile, and the
typed-borrow units (BORROW_TESTS): each misuse of a PyValueBorrow must fail
to compile, and the readers must accept owners and borrows alike.

Exit status: 0 if the self-tests pass and no rule fails; 1 otherwise.
"""

import argparse
import concurrent.futures
import glob
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
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
MARKED_SLOT_FAMILIES = frozenset()

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
QUERY = f"""\
set output diag
enable output dump
set traversal IgnoreUnlessSpelledInSource
match {VARIADIC_MATCHER}
set traversal AsIs
match {SLOT_READ_MATCHER}
"""

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
SRCDIR = Path(__file__).resolve().parents[2]


def display_path(path, directory):
    """path, which clang reports relative to directory, made relative to the
    source tree if it is inside it, so that each location has one name."""
    full = Path(os.path.normpath(Path(directory, path)))
    if full.is_relative_to(SRCDIR):
        return str(full.relative_to(SRCDIR))
    return str(full)


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
        path = display_path(path, directory)
        if binding == "variadic":
            for message in check_call(parse_call_dump(dump)):
                findings.append(
                    Finding(path, line, column, "variadic", message, True))
        elif binding == "slot" and kind == "core":
            names = re.findall(r"->(\w+)|\.(\w+)", " ".join(dump))
            field = next((a or b for a, b in names
                          if (a or b) in SLOT_FAMILIES), None)
            family = SLOT_FAMILIES.get(field, "?")
            findings.append(Finding(
                path, line, column, "slot-read",
                f"read of {field} ({family} family)",
                family in MARKED_SLOT_FAMILIES))
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


def run_query(clang_query, query_file, entry, kind):
    path = Path(entry["directory"], entry["file"])
    # Clang parses a unit that GCC compiles: silence its notes on GCC-only
    # options.
    proc = subprocess.run(
        [clang_query, "-f", query_file, str(path), "--", *compile_flags(entry),
         "-Wno-unknown-warning-option", "-Wno-ignored-optimization-argument"],
        cwd=entry["directory"], capture_output=True, text=True)
    errors = [line for line in proc.stdout.splitlines()
              + proc.stderr.splitlines() if ": error:" in line]
    return path, parse_output(proc.stdout, kind, entry["directory"]), errors


def check_units(clang_query, entries, jobs):
    with tempfile.TemporaryDirectory() as tmp:
        query_file = os.path.join(tmp, "query.txt")
        Path(query_file).write_text(QUERY)
        findings = set()
        errors = []
        with concurrent.futures.ThreadPoolExecutor(jobs) as pool:
            futures = [pool.submit(run_query, clang_query, query_file,
                                   entry, kind)
                       for entry, kind in entries]
            for future in futures:
                path, unit_findings, unit_errors = future.result()
                findings.update(unit_findings)
                errors.extend(f"{path}: {e}" for e in unit_errors)
        return sorted(findings), errors


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
            PyValue v = PyValue_FromObjectBorrow(o);
            return PyUnicode_FromFormat("%vR %vS", PyValue_Borrow(v), o);
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
}
SELF_TEST_HEADER = '#include "Python.h"\n#include "pycore_value_api.h"\n'

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
                    f.fails or rule == "slot-read" for f in got)):
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
        for name, prefix, should_fail in (
                ("unavailable_core", "", True),
                ("unavailable_module", "#define Py_BUILD_CORE_MODULE 1\n",
                 False)):
            path = Path(tmp, f"{name}.c")
            path.write_text(prefix + SELF_TEST_HEADER + call)
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

    failures = self_test(clang_query, template, compiler, args.cxx,
                         args.jobs)
    if failures or args.self_test_only:
        for failure in failures:
            print(failure)
        return 1 if failures else 0

    if args.files:
        wanted = {os.path.normpath(os.path.abspath(f)) for f in args.files}
        entries = {p: e for p, e in entries.items() if p in wanted}
    findings, errors = check_units(clang_query, entries.values(), args.jobs)
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
