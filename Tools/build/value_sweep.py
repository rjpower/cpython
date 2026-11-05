#!/usr/bin/env python3
"""The sweep tool of the value-API waves.

pyjit/design/value-api.md (R23, section 9) and
pyjit/design/proposals/value-api-v2a0.md (section 1.5) describe the wave:
mark every object function whose value form exists, let the compiler list
the core calls of marked functions, rewrite the mechanical ones here, and
hand the rest to workers. The data is the ownership table,
Tools/build/value_ownership.toml (see its header); a wave extends it.

    value_sweep.py mark [--unmark] [--family F]... [--srcdir DIR]
        Put Py_VALUE_COMPAT(<value>) before the declaration of every row's
        object function in Include/ (or take the marks away).

    value_sweep.py errors LOG... [--srcdir DIR] > sites.tsv
        Parse clang or GCC build logs (make -k, clang -ferror-limit=0) for
        "'F' is unavailable" errors and "returning 'PyObject *' from a
        function with incompatible result type 'PyValue'" errors, and
        print one TSV line per site: file, line, col, function, class,
        family, note (the columns of Tools/build/wave_files.py).

    value_sweep.py fix SITES.tsv --build DIR [--dry-run] [--jobs N]
                   [--report OUT.tsv]
        Rewrite the sites of classes M1, M3 and R by byte offset and report
        every site with its class; the sites left are the workers'.

Classes:

  M1  every argument of the call has a rule: "keep" stays, "borrow" becomes
      PyValue_FromObjectBorrow(o) (_PyValue_FromObjectKnown(o) for a typed
      pointer that cannot be an exact int or float, v for
      PyValue_AsKnownObjectBorrow(v)), "steal" becomes PyValue_PrepareOwned(o)
      (PyValue_DUP(<borrow>) for Py_NewRef(x), v for
      PyValue_AsKnownObjectSteal(v)), and an "out" argument must be NULL.
  M3  a "new" result passed straight to a "steal" argument of another row:
      both calls take their value forms, and the owner passes through.
  R   "return F(...)" of a [[constructor]] row, in a function that returns a
      PyValue, calls the value constructor.
  P   a "new" result or a non-NULL output that reaches a C local: a worker
      retypes the local or writes the explicit exit
      _PyValue_AsObjectAt(v, BOX_SITE_<family>) and PyValue_CLOSE(v).
  J   anything else (a macro body, an address taken, a type the tool cannot
      see): a worker's site, with the reason in the note.

Types come from a per-function AST dump of the unit (clang -fsyntax-only
-Xclang -ast-dump=json -Xclang -ast-dump-filter=<function>, with the
unit's flags from the build's Makefile), since a full dump of a unit that
includes Python.h runs to hundreds of MB. The dump decides whether an
argument is a typed pointer; without it the tool takes the generic entry,
which is always correct.
"""

import argparse
import collections
import concurrent.futures
import json
import os
import re
import shlex
import subprocess
import sys
import tomllib
from dataclasses import dataclass, field
from pathlib import Path

TOOL_DIR = Path(__file__).resolve().parent
SRCDIR = TOOL_DIR.parent.parent
TABLE = TOOL_DIR / "value_ownership.toml"

ARG_KINDS = {"keep", "borrow", "steal", "out", "count"}
RESULTS = {"status", "object", "new"}

BORROW_ENTRY = "PyValue_FromObjectBorrow"
KNOWN_ENTRY = "_PyValue_FromObjectKnown"
STEAL_ENTRY = "PyValue_PrepareOwned"
OBJECT_CAST = "_PyObject_CAST"

# Expressions that name an object which is never an exact int or float.
KNOWN_OBJECT_TEXT = re.compile(
    r"&?\s*_Py_(?:ID|STR|LATIN1_CHR|INTERP_CACHED_OBJECT|INTERP_STR"
    r"|INTERP_ID)\s*\(.*\)"
    r"|Py_(?:None|True|False|Ellipsis|NotImplemented)"
    r"|\(\s*PyObject\s*\*\s*\)\s*&\s*_Py_\w+Struct", re.S)
# Expressions of a typed pointer that names a type.
KNOWN_TYPE_TEXT = re.compile(r"Py_TYPE\s*\(.*\)", re.S)
# Pointer types that hold an object which is never an exact int or float.
GENERIC_TYPES = {"PyObject", "struct _object", "PyVarObject", "PyLongObject",
                 "struct _longobject", "PyFloatObject", "void"}
TYPED_POINTER = re.compile(
    r"^(?:const\s+)?(?:struct\s+)?(_?Py\w*(?:Object|Type))\s*\*$")


@dataclass
class Row:
    object: str
    value: str
    family: str
    args: list
    result: str
    form: str = "call"
    aliases: list = field(default_factory=list)


@dataclass
class Constructor:
    object: str
    value: str
    family: str = "number"


def load_table(path=TABLE):
    with open(path, "rb") as f:
        data = tomllib.load(f)
    rows = {}
    for raw in data.get("row", []):
        row = Row(**raw)
        bad = set(row.args) - ARG_KINDS
        if bad or row.result not in RESULTS or row.form not in ("call", "pack"):
            raise ValueError(f"{path}: bad row {row.object}: {raw}")
        if row.object in rows:
            raise ValueError(f"{path}: duplicate row {row.object}")
        rows[row.object] = row
    constructors = {c["object"]: Constructor(**c)
                    for c in data.get("constructor", [])}
    return rows, constructors


def names_of(rows):
    """The name -> row map, aliases included."""
    names = {}
    for row in rows.values():
        names[row.object] = row
        for alias in row.aliases:
            names[alias] = row
    return names


# --- C text ---------------------------------------------------------------

def skip_space(text, i):
    """The offset of the next token at or after i: whitespace, comments and
    line continuations skipped."""
    n = len(text)
    while i < n:
        if text[i].isspace():
            i += 1
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            i = n if j < 0 else j + 2
        elif text.startswith("//", i):
            j = text.find("\n", i)
            i = n if j < 0 else j
        elif text.startswith("\\\n", i):
            i += 2
        else:
            break
    return i


def skip_literal(text, i):
    """The offset after the string or character literal at i."""
    quote = text[i]
    i += 1
    while i < len(text) and text[i] != quote:
        i += 2 if text[i] == "\\" else 1
    return i + 1


def identifier_at(text, i):
    m = re.compile(r"[A-Za-z_]\w*").match(text, i)
    return m.group(0) if m else None


def scan_call(text, i):
    """For the identifier at offset i: (name, args, end) where args is a list
    of (start, end) spans of the argument texts, stripped, and end is the
    offset after the closing parenthesis; None if it is not called here."""
    name = identifier_at(text, i)
    if name is None:
        return None
    j = skip_space(text, i + len(name))
    if j >= len(text) or text[j] != "(":
        return None
    args, depth, start, k = [], 0, j + 1, j + 1
    closers = {"(": ")", "[": "]", "{": "}"}
    stack = []
    while k < len(text):
        c = text[k]
        if c in "\"'":
            k = skip_literal(text, k)
            continue
        if text.startswith("/*", k) or text.startswith("//", k):
            k = skip_space(text, k)
            continue
        if c in closers:
            stack.append(closers[c])
        elif stack and c == stack[-1]:
            stack.pop()
        elif not stack and c in ",)":
            args.append(strip_span(text, start, k))
            if c == ")":
                if args == [(start, start)] and not text[start:k].strip():
                    args = []
                return name, args, k + 1
            start = k + 1
        k += 1
    return None


def strip_span(text, start, end):
    while start < end and text[start].isspace():
        start += 1
    while end > start and text[end - 1].isspace():
        end -= 1
    return start, end


def offset_of(text, line, col):
    """The offset of a 1-based line and byte column (clang's columns)."""
    pos = 0
    for _ in range(line - 1):
        pos = text.index("\n", pos) + 1
    return pos + col - 1


def line_of(text, offset):
    return text.count("\n", 0, offset) + 1


def in_define(text, offset):
    """Whether offset lies in a #define, continuation lines included."""
    start = text.rfind("\n", 0, offset) + 1
    while start > 1 and text[start - 2] == "\\":
        start = text.rfind("\n", 0, start - 1) + 1
    return re.match(r"[ \t]*#[ \t]*define\b", text[start:]) is not None


FUNCTION_HEAD = re.compile(r"\(?([A-Za-z_]\w*)\)?\s*\(")


def enclosing_function(text, offset):
    """The name of the function whose body holds offset, by CPython's layout:
    the body opens with "{" in column 0 after the declarator. None if the
    offset is outside a function body."""
    brace = text.rfind("\n{", 0, offset)
    if brace < 0:
        return None
    closing = text.find("\n}", brace + 1)
    if closing != -1 and closing < offset:
        return None
    # The declarator: the lines above the brace, back to a blank line or the
    # end of the previous declaration.
    # Comments between the declarator and the brace (Argument Clinic's
    # checksum line) are skipped.
    head_start, head = brace, ""
    while head_start > 0:
        prev = text.rfind("\n", 0, head_start)
        stripped = text[prev + 1:head_start].strip()
        if not head.strip() and stripped.startswith(("/*", "//")) \
                and (stripped.endswith("*/") or stripped.startswith("//")):
            head_start = prev
            continue
        if (prev < 0 or not stripped or stripped.endswith((";", "}", "*/"))
                or stripped.startswith("#")):
            break
        head = text[prev + 1:head_start] + "\n" + head
        head_start = prev
    for m in FUNCTION_HEAD.finditer(head):
        if m.group(1) not in ("PyAPI_FUNC", "Py_LOCAL_INLINE", "Py_NO_INLINE",
                              "Py_DEPRECATED", "_Py_NO_SANITIZE_THREAD",
                              "Py_GCC_ATTRIBUTE", "__attribute__"):
            return m.group(1)
    return None


# --- the AST --------------------------------------------------------------

def compile_command(build, rel):
    """The build's compile command for the unit rel (relative to srcdir), as
    Tools/build/../pyjit-link-wave1/tools/check_unit.sh finds it."""
    makefile = Path(build, "Makefile").read_text()
    srcrel = re.search(r"^srcdir=\s*(\S+)", makefile, re.M).group(1)
    obj = rel[:-2] + ".o"
    out = subprocess.run(
        ["make", "-s", "-C", str(build), "-n", "-W", f"{srcrel}/{rel}", obj],
        capture_output=True, text=True).stdout
    lines = [l for l in out.splitlines()
             if rel in l and re.search(r"(^| )-c( |$)", l)]
    if not lines:
        return None
    argv = shlex.split(lines[-1])
    if "-o" in argv:
        i = argv.index("-o")
        del argv[i:i + 2]
    return argv


def dump_function(argv, cwd, name):
    """The top-level nodes of a JSON AST dump of the unit filtered by name,
    in print order: a stream of JSON objects, one per matching declaration
    (the filter matches names that contain name)."""
    command = [*argv, "-fsyntax-only", "-Xclang", "-ast-dump=json",
               "-Xclang", f"-ast-dump-filter={name}"]
    out = subprocess.run(command, cwd=cwd, capture_output=True,
                         text=True).stdout
    decoder, nodes, i = json.JSONDecoder(), [], 0
    while True:
        i = skip_space(out, i)
        if i >= len(out):
            break
        try:
            node, i = decoder.raw_decode(out, i)
        except json.JSONDecodeError:
            break
        nodes.append(node)
    return nodes


def is_definition(node, name):
    return (node.get("kind") == "FunctionDecl" and node.get("name") == name
            and any(c.get("kind") == "CompoundStmt"
                    for c in node.get("inner", ())))


def location_spelling(loc):
    """(offset, tokLen, file-or-None) of a location's spelling."""
    if "spellingLoc" in loc:
        loc = loc["spellingLoc"]
    return loc.get("offset"), loc.get("tokLen", 0), loc.get("file")


class SpanTypes:
    """The written types of the expressions of one function body, by their
    spelling span in the unit's main file.

    The JSON dumper names a location's file only when it differs from the
    last location it printed, so the walk keeps that state in print order."""

    def __init__(self, nodes, main_file, name, cwd):
        # The dump names files relative to the directory clang ran in.
        self.cwd = cwd
        self.main = os.path.realpath(Path(cwd, main_file))
        self.current = None
        self.spans = collections.defaultdict(list)
        self.starts = collections.defaultdict(list)
        self.found = False
        # The dumper's file state runs across the stream, so every node is
        # walked; spans are kept for the definition of name only.
        for node in nodes:
            keep = is_definition(node, name)
            self.found |= keep
            self.walk(node, keep)

    def note_file(self, value):
        if isinstance(value, dict):
            if "file" in value:
                self.current = os.path.realpath(Path(self.cwd, value["file"]))
            for key in ("spellingLoc", "expansionLoc"):
                if key in value:
                    self.note_file(value[key])

    def place(self, loc):
        """Update the file state from loc; return (offset, tokLen, in main)."""
        if "spellingLoc" in loc:
            self.note_file(loc["spellingLoc"])
            spelling_file = self.current
            if "expansionLoc" in loc:
                self.note_file(loc["expansionLoc"])
            offset, toklen = (loc["spellingLoc"].get("offset"),
                              loc["spellingLoc"].get("tokLen", 0))
            return offset, toklen, spelling_file == self.main
        self.note_file(loc)
        return loc.get("offset"), loc.get("tokLen", 0), self.current == self.main

    def walk(self, node, keep=True):
        if not isinstance(node, dict):
            return
        begin = end = None
        if "loc" in node:
            self.place(node["loc"])
        if "range" in node:
            begin = self.place(node["range"].get("begin", {}))
            end = self.place(node["range"].get("end", {}))
        kind = node.get("kind", "")
        if (keep and begin and end and begin[2] and begin[0] is not None
                and kind.endswith(("Expr", "Literal", "Operator"))):
            # An end token spelled in a macro (Py_None) has no offset here.
            stop = end[0] + end[1] if end[2] and end[0] is not None else None
            qual = node.get("type", {})
            entry = (kind, qual.get("qualType"), node)
            self.spans[(begin[0], stop)].append(entry)
            self.starts[begin[0]].append((stop, entry))
        for child in node.get("inner", ()):
            self.walk(child, keep)

    def written_type(self, span):
        """(outer type, inner type) of the expression at span: the type as
        written, and the type under any casts and parentheses."""
        found = [e for e in self.spans.get(span, ())
                 if e[0] != "ImplicitCastExpr"]
        if not found:
            # The outermost expression that starts there and does not end
            # past the span, if its end lies in a macro.
            found = [e for stop, e in self.starts.get(span[0], ())
                     if e[0] != "ImplicitCastExpr"
                     and (stop is None or stop <= span[1])]
        if not found:
            return None
        kind, outer, node = found[0]
        inner = node
        while inner.get("kind") in ("CStyleCastExpr", "ParenExpr",
                                    "ImplicitCastExpr") and inner.get("inner"):
            inner = inner["inner"][0]
        inner_type = inner.get("type", {}).get("qualType")
        return outer, inner_type


def typed_pointer(type_name):
    if not type_name:
        return False
    m = TYPED_POINTER.match(type_name.strip())
    return bool(m) and m.group(1) not in GENERIC_TYPES


def generic_pointer(type_name):
    return type_name is not None and type_name.replace(" ", "") in (
        "PyObject*", "structobject*", "struct_object*")


# --- rewriting ------------------------------------------------------------

class Refuse(Exception):
    """A site the tool leaves to a worker; the message is the note."""


def unwrap(text, function):
    """If text is exactly function(x), x; else None."""
    m = re.fullmatch(rf"{function}\s*\((.*)\)", text, re.S)
    if m is None:
        return None
    inner = scan_call(text, 0)
    if inner is None or inner[2] != len(text) or len(inner[1]) != 1:
        return None
    a, b = inner[1][0]
    return text[a:b]


class Rewriter:
    def __init__(self, rows, constructors=None, types=None):
        self.names = names_of(rows)
        self.constructors = constructors or {}
        self.types = types          # SpanTypes or None

    def arg_type(self, span):
        if self.types is None:
            return None
        return self.types.written_type(span)

    def object_arg(self, text, span, macro):
        """The argument text as a PyObject *: cast if its written type may
        be a typed pointer (only a function-like macro accepts one)."""
        arg = text[span[0]:span[1]]
        if not macro:
            return arg
        found = self.arg_type(span)
        if found is not None and generic_pointer(found[0]):
            return arg
        return f"{OBJECT_CAST}({arg})"

    def borrow(self, text, span, macro, owner_view=False):
        arg = text[span[0]:span[1]]
        inner = unwrap(arg, "PyValue_AsKnownObjectBorrow")
        if inner is not None:
            return f"PyValue_Borrow({inner})" if owner_view else inner
        if KNOWN_OBJECT_TEXT.fullmatch(arg):
            return f"{KNOWN_ENTRY}({arg})"
        if KNOWN_TYPE_TEXT.fullmatch(arg):
            return f"{KNOWN_ENTRY}({OBJECT_CAST}({arg}))"
        found = self.arg_type(span)
        if found is not None and typed_pointer(found[1]):
            cast = arg if generic_pointer(found[0]) else f"{OBJECT_CAST}({arg})"
            return f"{KNOWN_ENTRY}({cast})"
        return f"{BORROW_ENTRY}({self.object_arg(text, span, macro)})"

    def steal(self, text, span, macro, sites):
        arg = text[span[0]:span[1]]
        inner = unwrap(arg, "PyValue_AsKnownObjectSteal")
        if inner is not None:
            return inner, "M1"
        for new_ref, dup in (("Py_NewRef", "PyValue_DUP"),
                             ("Py_XNewRef", "PyValue_XDUP")):
            if unwrap(arg, new_ref) is not None:
                ispan = scan_call(text, span[0])[1][0]
                return f"{dup}({self.borrow(text, ispan, macro)})", "M1"
        call = scan_call(text, span[0])
        if call is not None and call[2] == span[1]:
            row = self.names.get(call[0])
            if row is not None and row.result == "new":
                # M3: the value form's owner goes straight in.
                replacement, _ = self.call(text, span[0], call, row, sites)
                sites.add(span[0])
                return replacement, "M3"
        return f"{STEAL_ENTRY}({self.object_arg(text, span, macro)})", "M1"

    def call(self, text, start, call, row, sites):
        """The value-form text of the call at start; (text, class)."""
        name, args, end = call
        # A function-like macro of the same name (PyList_SET_ITEM) or an
        # alias casts its arguments, so they may be typed pointers.
        macro = name != row.object or re.search(r"_[A-Z][A-Z_]+$", name)
        kinds = list(row.args)
        if kinds == ["count"]:
            if not args:
                raise Refuse("variadic call without a count")
            count = text[args[0][0]:args[0][1]]
            if not re.fullmatch(r"\d+", count) or int(count) != len(args) - 1:
                raise Refuse(f"count {count!r} is not the literal number of "
                             "items")
            args = args[1:]
            kinds = ["borrow"] * len(args)
            # A variadic argument may be any pointer, as a macro's may.
            macro = True
        if len(args) != len(kinds):
            raise Refuse(f"{len(args)} arguments, the table has {len(kinds)}")
        out, klass = [], "M1"
        for span, kind in zip(args, kinds):
            arg = text[span[0]:span[1]]
            if kind == "keep":
                out.append(arg)
            elif kind == "borrow":
                out.append(self.borrow(text, span, macro,
                                       owner_view=row.form == "pack"))
            elif kind == "steal":
                value, k = self.steal(text, span, macro, sites)
                out.append(value)
                if k == "M3":
                    klass = "M3"
            elif kind == "out":
                if arg != "NULL":
                    raise Refuse("P: output argument is not NULL; retype the "
                                 "local that receives it")
                out.append(arg)
        if row.form == "pack":
            items = ", ".join(out)
            return f"{row.value}({len(out)}, (PyValueBorrow[]){{{items}}})", klass
        return f"{row.value}({', '.join(out)})", klass

    def site(self, text, offset, in_macro=False):
        """The edits [(start, end, replacement)], the class and the offsets of
        inner sites it covers, for the site at offset; raises Refuse with
        the reason."""
        if not in_macro and in_define(text, offset):
            raise Refuse("in a macro body")
        name = identifier_at(text, offset)
        if name in self.constructors:
            return self.constructor(text, offset, name)
        row = self.names.get(name)
        if row is None:
            return self.macro(text, offset, name)
        call = scan_call(text, offset)
        if call is None:
            raise Refuse("not a call (an address taken?)")
        if row.result == "new":
            raise Refuse("P: new result reaches a C local")
        sites = set()
        replacement, klass = self.call(text, offset, call, row, sites)
        return [(offset, call[2], replacement)], klass, sites

    def macro(self, text, offset, name, seen=()):
        """The site is the use of a macro of this file whose body calls a
        marked function, directly or through other macros of this file:
        rewrite the body once. Its arguments are macro parameters, whose
        types the dump cannot give."""
        if name is None:
            raise Refuse("no identifier at the site")
        if name in seen:
            raise Refuse(f"{name} at the site: no marked call in its body")
        # The definition in force at the use: the last one before it.
        m = None
        for m in re.finditer(rf"^[ \t]*#[ \t]*define[ \t]+{name}\b",
                             text[:offset], re.M):
            pass
        if m is None:
            raise Refuse(f"{name} at the site: the call is spelled through a "
                         "macro of another file")
        end = m.end()
        while True:
            eol = text.find("\n", end)
            if eol < 0 or text[eol - 1] != "\\":
                break
            end = eol + 1
        body_end = len(text) if eol < 0 else eol
        saved, self.types = self.types, None
        edits, klass, covered = [], "M1", set()
        try:
            for call in re.finditer(r"\b\w+\b", text[m.end():body_end]):
                start = m.end() + call.start()
                if start in covered:
                    continue
                if call.group(0) not in self.names:
                    if re.search(rf"^[ \t]*#[ \t]*define[ \t]+{call.group(0)}\b",
                                 text, re.M) is None:
                        continue
                    try:
                        inner_edits, k, inner = self.macro(
                            text, offset, call.group(0), (*seen, name))
                    except Refuse as refusal:
                        if "no marked call" in str(refusal):
                            continue
                        raise
                elif scan_call(text, start) is None:
                    continue
                else:
                    inner_edits, k, inner = self.site(text, start,
                                                      in_macro=True)
                edits += inner_edits
                covered |= inner | {start}
                klass = "M3" if k == "M3" else klass
        except Refuse as refusal:
            raise Refuse(f"in macro {name}: {refusal}")
        finally:
            self.types = saved
        if not edits:
            raise Refuse(f"{name} at the site: no marked call in its body")
        return edits, klass, set()

    def constructor(self, text, offset, name):
        before = text[:offset].rstrip()
        if not before.endswith("return"):
            raise Refuse("constructor not returned directly")
        call = scan_call(text, offset)
        if call is None:
            raise Refuse("not a call")
        value = self.constructors[name].value
        return [(offset, offset + len(name), value)], "R", set()


# --- includes -------------------------------------------------------------

# The names a rewrite introduces besides the table's value forms.
ENTRY_NAMES = (BORROW_ENTRY, KNOWN_ENTRY, STEAL_ENTRY, "PyValue_DUP",
               "PyValue_XDUP", "PyValue_Borrow")


def declaring_headers(names, srcdir):
    """name -> the internal header ("pycore_x.h") that declares it; names
    that a public header declares are left out, since Python.h has them."""
    found = {}
    headers = sorted(Path(srcdir, "Include", "internal").glob("pycore_*.h"))
    for path in headers:
        text = path.read_text(errors="replace")
        for name in names:
            if name in found:
                continue
            if re.search(rf"^[ \t]*#[ \t]*define[ \t]+{name}\b"
                         rf"|^(?:PyAPI_FUNC\([^)]*\)|static inline\b|extern\b)"
                         rf"[^;{{]*?\b\(?{name}\)?\s*\("
                         rf"|^\(?{name}\)?\s*\(", text, re.M):
                found[name] = path.name
    return found


INCLUDE = re.compile(r'^#include "(pycore_\w+\.h)"[^\n]*\n', re.M)


def reachable_headers(text, srcdir):
    """The internal headers that text includes, directly or through other
    internal headers (Python.h includes none of them)."""
    seen, todo = set(), list(INCLUDE.findall(text))
    internal = Path(srcdir, "Include", "internal")
    while todo:
        header = todo.pop()
        if header in seen:
            continue
        seen.add(header)
        path = internal / header
        if path.exists():
            todo += INCLUDE.findall(path.read_text(errors="replace"))
    return seen


def add_includes(text, headers, srcdir=SRCDIR):
    """text with an #include of each header that it does not reach, among
    its pycore_ includes in sorted place; headers maps a header to the names
    it is included for. Returns (text, added)."""
    present = reachable_headers(text, srcdir)
    added = []
    for header in sorted(set(headers) - present):
        line = f'#include "{header}"'
        line = f"{line:<34}// {sorted(headers[header])[0]}()"
        matches = list(INCLUDE.finditer(text))
        if matches:
            after = [m for m in matches if m.group(1) > header]
            pos = after[0].start() if after else matches[-1].end()
        else:
            m = None
            for m in re.finditer(r'^#include "Python\.h"[^\n]*\n', text,
                                 re.M):
                pass
            if m is None:
                continue
            pos = m.end()
        text = text[:pos] + line + "\n" + text[pos:]
        added.append(header)
    return text, added


def needed_headers(text, index):
    """header -> the names of index that text calls and it declares."""
    used = {m.group(1) for m in re.finditer(r"\b(\w+)\s*\(", text)}
    headers = collections.defaultdict(set)
    for name in used & set(index):
        headers[index[name]].add(name)
    return headers


def cmd_includes(args):
    rows, _ = load_table(args.table)
    names = {r.value for r in rows.values()} | set(ENTRY_NAMES)
    index = declaring_headers(names, args.srcdir)
    for rel in args.files:
        path = Path(args.srcdir, rel)
        text = path.read_text()
        new, added = add_includes(text, needed_headers(text, index),
                                  args.srcdir)
        if added:
            path.write_text(new)
            print(f"{rel}: {', '.join(added)}")
    return 0


# --- commands -------------------------------------------------------------

def declaration_pattern(name):
    # A declaration may carry Py_VALUE_ENTRY (an entry whose callers a wave
    # has yet to port) or an in-line Py_VALUE_COMPAT before PyAPI_FUNC.
    return re.compile(
        rf"^([ \t]*)((?:(?:Py_VALUE_ENTRY|Py_VALUE_COMPAT\(\w+\))[ \t]+)?"
        rf"(?:Py_DEPRECATED\([^)]*\)\s*)?(?:PyAPI_FUNC\([^)]*\)|extern\b[^;(]*?)"
        rf"\s*\(?{re.escape(name)}\)?\s*\()", re.M)


MARK_LINE = re.compile(r"^[ \t]*Py_VALUE_COMPAT\((\w+)\)[ \t]*\n", re.M)
INLINE_MARK = re.compile(r"Py_VALUE_(ENTRY|COMPAT\(\w+\))")


def mark_text(text, rows, unmark=False):
    """text with the marks of rows added (or removed); (text, count).

    A declaration marked Py_VALUE_ENTRY takes the mark in place of the
    entry token (the entry mark is the weaker of the two, value-api-g.md
    section 1.3); any other takes a Py_VALUE_COMPAT line before it.
    """
    count = 0
    for row in rows:
        pattern = declaration_pattern(row.object)
        pos = 0
        while (m := pattern.search(text, pos)) is not None:
            line_start = m.start()
            inline = INLINE_MARK.match(m.group(2))
            if inline is not None:
                entry = inline.group(1) == "ENTRY"
                if entry != unmark:
                    start = m.start(2)
                    new = ("Py_VALUE_ENTRY" if unmark
                           else f"Py_VALUE_COMPAT({row.value})")
                    text = text[:start] + new + text[start + inline.end():]
                    count += 1
                pos = m.start(2) + 1
                continue
            prev_start = text.rfind("\n", 0, max(line_start - 1, 0)) + 1
            prev = MARK_LINE.match(text, prev_start)
            marked = prev is not None and prev.end() == line_start
            if unmark and marked:
                text = text[:prev_start] + text[line_start:]
                count += 1
                pos = prev_start + 1
            elif not unmark and not marked:
                mark = f"{m.group(1)}Py_VALUE_COMPAT({row.value})\n"
                text = text[:line_start] + mark + text[line_start:]
                count += 1
                pos = line_start + len(mark) + 1
            else:
                pos = m.end()
    return text, count


def cmd_mark(args):
    rows, _ = load_table(args.table)
    chosen = [r for r in rows.values()
              if not args.family or r.family in args.family]
    total, found = 0, set()
    for path in sorted(Path(args.srcdir, "Include").rglob("*.h")):
        text = path.read_text()
        new, count = mark_text(text, chosen, args.unmark)
        for row in chosen:
            if declaration_pattern(row.object).search(text):
                found.add(row.object)
        if count:
            path.write_text(new)
            total += count
            print(f"{path.relative_to(args.srcdir)}: {count}")
    missing = sorted({r.object for r in chosen} - found)
    for name in missing:
        print(f"no declaration found: {name}", file=sys.stderr)
    print(f"{'unmarked' if args.unmark else 'marked'} {total}")
    return 1 if missing else 0


UNAVAILABLE = re.compile(
    r"^(?P<file>[^:\s]+):(?P<line>\d+):(?P<col>\d+): error: "
    r"['‘](?P<name>\w+)['’] is unavailable")
INCOMPATIBLE_RETURN = re.compile(
    r"^(?P<file>[^:\s]+):(?P<line>\d+):(?P<col>\d+): error: returning "
    r"'PyObject \*'.*incompatible result type 'PyValue'")


def relative(path, srcdir, build):
    full = Path(os.path.normpath(Path(build or ".", path)))
    if not full.is_absolute():
        full = Path(os.path.normpath(Path.cwd() / full))
    try:
        return str(full.relative_to(srcdir))
    except ValueError:
        return str(full)


def parse_errors(lines, srcdir, build=None, names=None):
    """Sites as dicts, in order, each once."""
    seen, sites = set(), []
    for line in lines:
        m = UNAVAILABLE.match(line) or INCOMPATIBLE_RETURN.match(line)
        if m is None:
            continue
        name = m.groupdict().get("name") or ""
        key = (relative(m["file"], srcdir, build), int(m["line"]),
               int(m["col"]))
        if key in seen:
            continue
        seen.add(key)
        row = (names or {}).get(name)
        sites.append({"file": key[0], "line": key[1], "col": key[2],
                      "function": name, "class": "",
                      "family": row.family if row else "",
                      "note": ""})
    return sites


COLUMNS = ("file", "line", "col", "function", "class", "family", "note")


def write_sites(sites, out):
    print("\t".join(COLUMNS), file=out)
    for s in sites:
        print("\t".join(str(s.get(c, "")) for c in COLUMNS), file=out)


def read_sites(path):
    with open(path) as f:
        lines = [l.rstrip("\n") for l in f if l.strip() and not l.startswith("#")]
    header = lines[0].split("\t")
    sites = []
    for line in lines[1:]:
        s = dict(zip(header, line.split("\t")))
        s["line"], s["col"] = int(s["line"]), int(s["col"])
        sites.append(s)
    return sites


def cmd_errors(args):
    rows, _ = load_table(args.table)
    lines = []
    for log in args.logs:
        with open(log, errors="replace") as f:
            lines.extend(f)
    sites = parse_errors(lines, Path(args.srcdir).resolve(), args.build,
                         names_of(rows))
    write_sites(sites, sys.stdout)
    return 0


def apply_edits(text, edits):
    """text with the (start, end, replacement) edits applied; edits that
    overlap an earlier one are returned as rejected."""
    edits = sorted(set(edits))
    kept, rejected, last_end = [], [], -1
    for edit in edits:
        if edit[0] < last_end:
            rejected.append(edit)
            continue
        kept.append(edit)
        last_end = edit[1]
    for start, end, replacement in reversed(kept):
        text = text[:start] + replacement + text[end:]
    return text, rejected


# The first line of a generated file (Python-ast.c, the cases generator's
# output, Argument Clinic's headers).
GENERATED = re.compile(r"(?://|/\*)[^\n]*\b(?:generated|Generated)\b")


def fix_file(rel, file_sites, rows, constructors, srcdir, build, dry_run):
    """Rewrite one file's sites; return the sites with class and note."""
    path = Path(srcdir, rel)
    text = path.read_text()
    if GENERATED.match(text):
        for s in file_sites:
            s["class"], s["note"] = "J", "generated file: fix its generator"
        return file_sites
    by_function = collections.defaultdict(list)
    for s in file_sites:
        s["offset"] = offset_of(text, s["line"], s["col"])
        by_function[enclosing_function(text, s["offset"])].append(s)
    argv = compile_command(build, rel) if build and rel.endswith(".c") else None
    edits, covered = [], set()
    for function, group in by_function.items():
        types = None
        if argv is not None and function is not None:
            nodes = dump_function(argv, build, function)
            source = [a for a in argv if a.endswith(rel)]
            if nodes and source:
                types = SpanTypes(nodes, source[-1], function, build)
                if not types.found:
                    types = None
        rewriter = Rewriter(rows, constructors, types)
        for s in group:
            try:
                site_edits, klass, inner = rewriter.site(text, s["offset"])
            except Refuse as refusal:
                note = str(refusal)
                s["class"] = "P" if note.startswith("P") else "J"
                s["note"] = note
                continue
            s["class"] = klass
            s["note"] = " ; ".join(e[2] for e in site_edits)
            s["edits"] = site_edits
            covered |= inner
            edits += site_edits
    new_text, rejected = apply_edits(text, edits)
    for s in file_sites:
        if s["offset"] in covered and s["class"] in ("P", "J"):
            s["class"], s["note"] = "M3", "inner call of a chained M3 site"
        elif any(e in rejected for e in s.get("edits", ())):
            s["class"], s["note"] = "J", "overlaps another rewritten call"
    if not dry_run and new_text != text:
        path.write_text(new_text)
    for s in file_sites:
        s.pop("offset", None)
        s.pop("edits", None)
    return file_sites


def cmd_fix(args):
    rows, constructors = load_table(args.table)
    sites = read_sites(args.sites)
    by_file = collections.defaultdict(list)
    for s in sites:
        if s["file"].startswith("/"):
            s["class"], s["note"] = "J", "outside the source tree"
            continue
        by_file[s["file"]].append(s)
    srcdir = Path(args.srcdir).resolve()
    build = Path(args.build).resolve() if args.build else None
    with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
        futures = [pool.submit(fix_file, rel, group, rows, constructors,
                               srcdir, build, args.dry_run)
                   for rel, group in sorted(by_file.items())]
        for future in futures:
            future.result()
    out = open(args.report, "w") if args.report else sys.stdout
    write_sites(sites, out)
    counts = collections.Counter(s["class"] for s in sites)
    print("value_sweep: " + ", ".join(f"{k} {v}" for k, v in
                                      sorted(counts.items())),
          file=sys.stderr)
    return 0


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--table", default=TABLE)
    parser.add_argument("--srcdir", default=SRCDIR)
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("mark")
    p.add_argument("--unmark", action="store_true")
    p.add_argument("--family", action="append")
    p.set_defaults(func=cmd_mark)
    p = sub.add_parser("errors")
    p.add_argument("logs", nargs="+")
    p.add_argument("--build", help="directory the compiler ran in")
    p.set_defaults(func=cmd_errors)
    p = sub.add_parser("includes")
    p.add_argument("files", nargs="+")
    p.set_defaults(func=cmd_includes)
    p = sub.add_parser("fix")
    p.add_argument("sites")
    p.add_argument("--build", help="build directory, for the unit flags")
    p.add_argument("--dry-run", action="store_true")
    p.add_argument("--jobs", type=int, default=os.cpu_count())
    p.add_argument("--report")
    p.set_defaults(func=cmd_fix)
    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
