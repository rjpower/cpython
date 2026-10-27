#!/usr/bin/env python3
"""List the core C functions that hold an object in a C local across a call.

Usage: python3 boil/tools/measure/c_roots.py [--list] [--top N] [--self-test] [ROOT]

ROOT is the source tree to read, by default this checkout.

The token-level probe of note 11 section 12.  For every function defined
in a .c file under Objects/, Python/, Modules/ and Parser/ (without
vendored libraries and clinic output), it asks whether an object-typed
local or parameter (``PyObject *``, ``Py*Object *``, or a ``PyValue`` by
value) occurs both before and after a call that is not on the short
non-collecting list below.  Such a function keeps an object only in its C
frame across a call that may collect, and is opaque to the precise-root
collector until it is migrated (10b's work list).

The probe is flow-insensitive: it compares token positions, so a loop or
a dead path overcounts, and it misses objects held in struct locals and
in arrays reached through pointers.  Comments, strings and preprocessor
lines are dropped before tokenizing, so code under #if is counted in
every branch.

Output: per tree, the functions and those that hold an object across a
call, and the files with the most such functions.  --list prints each
function as file:line name.
"""

import argparse
import re
import sys
from collections import Counter
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
TREES = ("Objects", "Python", "Modules", "Parser")
# Vendored libraries and generated argument parsers.
EXCLUDE_DIRS = {"clinic", "mimalloc", "_hacl", "expat", "libmpdec",
                "_xxtestfuzz"}

TOKEN = re.compile(r"""
    (?P<ident>[A-Za-z_]\w*)
  | (?P<num>\.?\d(?:[\w.]|[eEpP][+-])*)
  | (?P<op>->|\+\+|--|<<=|>>=|&&|\|\||[-+*/%&|^!=<>]=|<<|>>|\.\.\.|[{}()\[\];,.?:~!%^&*+\-=<>/|])
""", re.VERBOSE)

KEYWORDS = {
    "if", "for", "while", "switch", "return", "sizeof", "do", "else",
    "case", "default", "goto", "break", "continue", "typedef", "struct",
    "union", "enum", "_Alignof", "alignof", "__typeof__", "typeof",
    "_Static_assert", "static_assert", "__attribute__", "__asm__", "asm",
    "_Generic", "defined",
}
QUALIFIERS = {"const", "volatile", "static", "register", "restrict",
              "_Py_ALIGNED_DEF", "Py_ALIGNED"}

# Calls that neither allocate an object nor run Python code: type
# predicates, sizes, raw data accessors, stores (a store canonicalises and
# never allocates), reference-count macros (no-ops after 5c), assertions
# and libc memory and string helpers.  An item or cell read is not here:
# PyTuple_GET_ITEM, PyList_GET_ITEM and PyCell_GET box an inline value, and
# so may collect.
NON_COLLECTING = {
    "Py_INCREF", "Py_DECREF", "Py_XINCREF", "Py_XDECREF", "Py_CLEAR",
    "Py_SETREF", "Py_XSETREF", "Py_NewRef", "Py_XNewRef", "Py_TYPE",
    "Py_SIZE", "Py_IS_TYPE", "Py_SET_TYPE", "Py_SET_SIZE", "Py_Is",
    "Py_IsNone", "Py_IsTrue", "Py_IsFalse", "Py_UNUSED", "Py_ARRAY_LENGTH",
    "Py_MIN", "Py_MAX", "Py_ABS", "Py_UNREACHABLE", "Py_FatalError",
    "_Py_ID", "_Py_STR", "_Py_LATIN1_CHR", "assert", "memcpy", "memmove",
    "memset", "memcmp", "strlen", "strcmp", "strncmp", "strchr",
    "PyErr_Occurred", "_PyErr_Occurred", "_PyThreadState_GET",
    "PyThreadState_Get", "_PyInterpreterState_GET", "PyInterpreterState_Get",
    "PyType_HasFeature", "_PyType_HasFeature", "PyType_IsSubtype",
    "PyObject_TypeCheck", "PyUnicode_READ", "PyUnicode_READ_CHAR",
    "PyUnicode_DATA", "PyUnicode_KIND", "PyUnicode_IS_ASCII",
    "PyUnicode_IS_COMPACT_ASCII", "PyUnicode_WRITE", "PyFunction_GET_CODE",
    "PyFunction_GET_GLOBALS", "PyMethod_GET_FUNCTION", "PyMethod_GET_SELF",
    "PyCFunction_GET_FUNCTION", "PyCFunction_GET_SELF",
    "PyCFunction_GET_FLAGS", "Py_BEGIN_CRITICAL_SECTION",
    "Py_END_CRITICAL_SECTION", "Py_BEGIN_CRITICAL_SECTION2",
    "Py_END_CRITICAL_SECTION2", "PyValue_IsObject", "PyValue_IsNull",
    "PyValue_Is", "PyValue_FromObject", "PyValue_Bits",
}
NON_COLLECTING_PATTERNS = (
    re.compile(r"^_?Py\w*_Check(Exact)?$"),
    re.compile(r"^_?Py\w*_CHECK\w*$"),
    re.compile(r"^_?Py\w*_GET_(SIZE|LENGTH)$"),
    re.compile(r"^_?Py\w*_SET_ITEM$"),
    re.compile(r"^_?Py\w*_AS_\w+$"),
    re.compile(r"^_?Py_atomic_\w+$"),
    re.compile(r"^FT_ATOMIC_\w+$"),
    re.compile(r"^_?Py\w*_CAST$"),
    re.compile(r"^_Py_hashtable_\w+$"),
)

OBJECT_POINTER = re.compile(r"^Py\w*Object$")


def non_collecting(name):
    return name in NON_COLLECTING or any(
        p.match(name) for p in NON_COLLECTING_PATTERNS)


def strip(text):
    """Drop comments, string and character literals and preprocessor
    lines, keeping newlines so that line numbers stay right."""
    out = []
    i, n = 0, len(text)
    line_start = True
    while i < n:
        c = text[i]
        if line_start and c in " \t":
            out.append(c)
            i += 1
            continue
        if line_start and c == "#":
            # A preprocessor line, with its continuations.
            while i < n and text[i] != "\n":
                if text[i] == "\\" and i + 1 < n and text[i + 1] == "\n":
                    out.append("\n")
                    i += 2
                    continue
                if text.startswith("/*", i):
                    j = text.find("*/", i + 2)
                    j = n if j < 0 else j + 2
                    out.append("\n" * text.count("\n", i, j))
                    i = j
                    continue
                i += 1
            continue
        line_start = False
        if text.startswith("//", i):
            while i < n and text[i] != "\n":
                i += 1
            continue
        if text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            out.append("\n" * text.count("\n", i, j) or " ")
            i = j
            continue
        if c in "\"'":
            j = i + 1
            while j < n and text[j] != c:
                j += 2 if text[j] == "\\" else 1
            out.append(" 0 " + "\n" * text.count("\n", i, j))
            i = j + 1
            continue
        if c == "\n":
            line_start = True
        out.append(c)
        i += 1
    return "".join(out)


def tokenize(text):
    toks = []
    line = 1
    pos = 0
    for m in TOKEN.finditer(text):
        line += text.count("\n", pos, m.start())
        pos = m.start()
        toks.append((m.group(), line))
    return toks


def matching(toks, i, open_, close):
    depth = 0
    for j in range(i, len(toks)):
        t = toks[j][0]
        if t == open_:
            depth += 1
        elif t == close:
            depth -= 1
            if depth == 0:
                return j
    return len(toks) - 1


def matching_back(toks, i, open_, close):
    depth = 0
    for j in range(i, -1, -1):
        t = toks[j][0]
        if t == close:
            depth += 1
        elif t == open_:
            depth -= 1
            if depth == 0:
                return j
    return -1


def functions(toks):
    """Yield (name, line, params, body) for each function definition at
    file scope; params and body are token lists."""
    i, n = 0, len(toks)
    stmt_start = 0
    while i < n:
        t = toks[i][0]
        if t == ";":
            stmt_start = i + 1
        elif t == "{":
            end = matching(toks, i, "{", "}")
            head = [x[0] for x in toks[stmt_start:i]]
            if (i > 0 and toks[i - 1][0] == ")" and "=" not in head
                    and not {"struct", "union", "enum"} & set(head[:1])):
                popen = matching_back(toks, i - 1, "(", ")")
                if popen > stmt_start and re.match(r"[A-Za-z_]", toks[popen - 1][0]) \
                        and toks[popen - 1][0] not in KEYWORDS:
                    yield (toks[popen - 1][0], toks[popen - 1][1],
                           toks[popen + 1:i - 1], toks[i + 1:end])
            i = end
            stmt_start = end + 1
        i += 1


def split_commas(toks):
    parts, cur, depth = [], [], 0
    for t in toks:
        if t in ("(", "[", "{"):
            depth += 1
        elif t in (")", "]", "}"):
            depth -= 1
        if t == "," and depth == 0:
            parts.append(cur)
            cur = []
        else:
            cur.append(t)
    if cur:
        parts.append(cur)
    return parts


def is_object_type(type_name, stars):
    if type_name == "PyValue":
        return stars == 0
    return OBJECT_POINTER.match(type_name) is not None and stars == 1


def declarator_name(decl):
    """The name and pointer depth of a declarator, ignoring initializers
    and array bounds; None for a function pointer or abstract one."""
    stars = 0
    for k, t in enumerate(decl):
        if t == "*":
            stars += 1
        elif t in QUALIFIERS:
            continue
        elif re.match(r"[A-Za-z_]\w*$", t):
            return t, stars
        else:
            return None
    return None


def object_params(params):
    names = []
    for p in split_commas([t for t, _ in params]):
        p = [t for t in p if t not in QUALIFIERS]
        if len(p) < 2:
            continue
        type_name = p[0]
        d = declarator_name(p[1:])
        if d and is_object_type(type_name, d[1]):
            names.append(d[0])
    return names


def object_locals(body):
    """The names of object-typed locals declared in body, at a statement
    start: after {, ; or }."""
    names = []
    toks = [t for t, _ in body]
    n = len(toks)
    i = 0
    while i < n:
        prev = toks[i - 1] if i else "{"
        if prev in ("{", ";", "}"):
            j = i
            while j < n and toks[j] in QUALIFIERS:
                j += 1
            if j < n and (toks[j] == "PyValue" or OBJECT_POINTER.match(toks[j])):
                k = j + 1
                depth = 0
                while k < n and (toks[k] != ";" or depth > 0):
                    if toks[k] in ("(", "{", "["):
                        depth += 1
                    elif toks[k] in (")", "}", "]"):
                        depth -= 1
                    k += 1
                if j + 1 < n and toks[j + 1] not in ("(", ")", "->", ".", ",", "="):
                    for decl in split_commas(toks[j + 1:k]):
                        d = declarator_name(decl)
                        if d and is_object_type(toks[j], d[1]):
                            names.append(d[0])
        i += 1
    return names


def calls(body):
    """(start, end) token positions of the calls in body that may collect:
    the callee's name and the closing parenthesis."""
    out = []
    for k in range(len(body) - 1):
        t = body[k][0]
        if body[k + 1][0] != "(" or not re.match(r"[A-Za-z_]", t):
            continue
        if t in KEYWORDS or non_collecting(t):
            continue
        out.append((k, matching(body, k + 1, "(", ")")))
    return out


def statement_end(body, k):
    depth = 0
    for j in range(k, len(body)):
        t = body[j][0]
        if t in ("(", "[", "{"):
            depth += 1
        elif t in (")", "]", "}"):
            depth -= 1
            if depth < 0:
                return j
        elif t == ";" and depth == 0:
            return j
    return len(body)


def holds_across_call(params, body):
    """Does an object-typed variable hold a value before a collecting call
    that is used after it?  A value holds from a use, or from the end of
    the statement that assigns it, so that `x = f(y)` holds x from after
    the call; a declarator without an initializer holds nothing."""
    objs = object_params(params)
    names = set(objs) | set(object_locals(body))
    if not names:
        return False
    sites = calls(body)
    if not sites:
        return False
    held = {name: [-1] if name in objs else [] for name in names}
    used = {name: [] for name in names}
    for k, (t, _) in enumerate(body):
        if t not in names:
            continue
        prev = body[k - 1][0] if k else ""
        nxt = body[k + 1][0] if k + 1 < len(body) else ""
        if prev in (".", "->"):
            continue  # a field of the same name
        if nxt == "=":
            held[t].append(statement_end(body, k))
        elif nxt in (";", ",", "[") and prev in ("*", ",") and not used[t] \
                and not held[t]:
            continue  # its declaration
        else:
            held[t].append(k)
            used[t].append(k)
    for name in names:
        if not used[name]:
            continue
        for c0, c1 in sites:
            if any(h < c0 for h in held[name]) and used[name][-1] > c1:
                return True
    return False


def source_files(root, tree):
    for path in sorted((root / tree).rglob("*.c")):
        rel = path.relative_to(root)
        if EXCLUDE_DIRS & set(rel.parts[:-1]):
            continue
        yield path, rel


# (source, expected): does the one function in source hold an object
# across a collecting call?
FIXTURES = [
    ("PyObject *f(PyObject *a) { PyObject *x = PyLong_FromLong(1);"
     " g(a); return x; }", True),
    ("PyObject *f(void) { PyObject *x = PyLong_FromLong(1); return x; }",
     False),
    ("Py_ssize_t f(PyObject *t) { return PyTuple_GET_SIZE(t) + g(); }",
     False),
    ("PyObject *f(PyObject *t) { PyObject *a = PyTuple_GET_ITEM(t, 0);"
     " PyObject *b = PyTuple_GET_ITEM(t, 1); return a; }", True),
    ("int f(PyObject *o) { if (PyLong_Check(o)) { return 1; }"
     " return Py_TYPE(o) == NULL; }", False),
    ("int f(PyObject *o) { int r = g(o); return r; }", False),
    ("int f(PyObject *o) { g(1); return h(o); }", True),
    ("int f(PyValue v) { g(); return PyValue_IsNull(v); }", True),
    ("int f(PyValue *v) { g(); return v != NULL; }", False),
    ("int f(void) { PyObject *x; x = g(); return h(x); }", False),
    ("int f(void) { PyObject *x; x = g(); i(); return h(x); }", True),
    ("int f(S *s) { PyObject *x = g(); i(); return s->x != NULL; }", False),
]


def self_test():
    failed = 0
    for source, expected in FIXTURES:
        found = [holds_across_call(params, body)
                 for _, _, params, body in functions(tokenize(strip(source)))]
        if found != [expected]:
            print(f"FAIL: {source!r}: {found}, expected [{expected}]")
            failed += 1
    print(f"{len(FIXTURES) - failed} of {len(FIXTURES)} fixtures pass")
    return 1 if failed else 0


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--list", action="store_true",
                    help="print every function that holds an object")
    ap.add_argument("--top", type=int, default=10,
                    help="the number of files to list by count")
    ap.add_argument("--self-test", action="store_true",
                    help="check the classification on small fixtures")
    ap.add_argument("root", nargs="?", type=Path, default=REPO,
                    help="the source tree (default: this checkout)")
    args = ap.parse_args(argv)
    if args.self_test:
        return self_test()
    per_file = Counter()
    rows = []
    for tree in TREES:
        nfuncs = nhold = 0
        for path, rel in source_files(args.root, tree):
            toks = tokenize(strip(path.read_text(errors="replace")))
            for name, line, params, body in functions(toks):
                nfuncs += 1
                if holds_across_call(params, body):
                    nhold += 1
                    per_file[str(rel)] += 1
                    if args.list:
                        print(f"{rel}:{line} {name}")
        rows.append((tree, nfuncs, nhold))
    print("| Tree | Functions | Hold an object across a call |")
    print("|---|---:|---:|")
    for tree, nf, nh in rows:
        print(f"| {tree}/ | {nf:,} | {nh:,} |")
    print(f"| total | {sum(r[1] for r in rows):,} | {sum(r[2] for r in rows):,} |")
    print()
    print("Largest files: " + ", ".join(
        f"{Path(f).name} ({c})" for f, c in per_file.most_common(args.top)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
