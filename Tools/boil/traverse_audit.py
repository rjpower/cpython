"""Find object types that hold references a tracing collector cannot see.

The tracing collector (note 06, step 4; Python/gc_trace.c) follows
tp_traverse, so a type whose instances hold object pointers must have one,
whether or not it is a GC type, and it must visit every reference the
instance owns.  This tool parses the C sources and reports:

1. types whose instance struct has PyObject* (or other object pointer)
   members and that have no tp_traverse;
2. types with a tp_traverse that does not visit a member that their
   tp_dealloc or tp_clear releases;
3. types with a tp_traverse that names neither a member with references
   nor, in dealloc or clear, its release (upstream leaked such members
   under reference counting; under the tracing collector they are freed
   under the object: the multibyte codec stream writer's pending, found
   by 6d rung 4; triage: boil/design/work/audit-2.md).

The parse is heuristic: struct definitions that begin with PyObject_HEAD,
PyObject_VAR_HEAD, another object struct or a macro that expands to one;
static PyTypeObject initializers (positional or designated) and PyType_Spec
definitions, matched to structs by sizeof() in the basic size.  A member that
is a plain struct (not an object) contributes that struct's references as
"member.field"; a member that points to one, such as the head of a malloc'd
list of nodes, contributes them as "member->field", through at most
MAX_POINTER_DEPTH pointers.  A struct's pointers to its own type (the links
of a list) add nothing, as the nodes they reach have the same members, and
pointers to module, thread and interpreter states are taken as borrowed.
Report 2 reads member names from Py_DECREF/Py_XDECREF/Py_CLEAR/Py_XSETREF/
Py_SETREF (and a Py_buffer's obj from PyBuffer_Release) in the dealloc and
clear functions, and the members the traverse function names, each with the
functions and function-like macros that it calls, two calls deep, looked up
in the same file, then in the headers of its directory, then in Include/
(accessors such as PyInstanceMethod_GET_FUNCTION); a function of the same
file that the body names without calling it (a visit callback handed to an
enumerator) is read as a callee.  A traverse that calls another type's
tp_traverse (PyDict_Type.tp_traverse(op, ...)) visits that type's members
when its struct is the type's own or a base; that traverse is audited as its
own type's.  A traverse that delegates to a helper in another .c file is
listed and needs a look.

With --check the tool is a gate (boil/tools/gate.sh, level 3): it prints
nothing else and exits 1 when a type of the three reports, or one whose
traverse function it cannot find, has a member path that the allowlist
(Tools/boil/traverse_allow.txt) does not list for it, or when an entry
lists a type or a path that is no longer reported.  An entry is "path
variable member...: reason", where variable is the type's PyTypeObject or
PyType_Spec and the members are the paths as the report prints them.

Usage: python Tools/boil/traverse_audit.py [--root DIR] [--missing-only]
       python Tools/boil/traverse_audit.py --check [--root DIR] [--allow FILE]
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

SOURCE_DIRS = ("Objects", "Python", "Modules", "Include")

# Conditionals taken as false; every other #if/#ifdef is taken as true, and
# every #ifndef too.
FALSE_MACROS = {"Py_GIL_DISABLED", "MS_WINDOWS", "__APPLE__", "Py_TRACE_REFS",
                "Py_STATS", "_Py_TIER2", "__EMSCRIPTEN__"}

# The PyTypeObject fields in order, after PyVarObject_HEAD_INIT.
TYPE_FIELDS = [
    "tp_name", "tp_basicsize", "tp_itemsize", "tp_dealloc",
    "tp_vectorcall_offset", "tp_getattr", "tp_setattr", "tp_as_async",
    "tp_repr", "tp_as_number", "tp_as_sequence", "tp_as_mapping", "tp_hash",
    "tp_call", "tp_str", "tp_getattro", "tp_setattro", "tp_as_buffer",
    "tp_flags", "tp_doc", "tp_traverse", "tp_clear", "tp_richcompare",
    "tp_weaklistoffset", "tp_iter", "tp_iternext", "tp_methods", "tp_members",
    "tp_getset", "tp_base",
]

RELEASE_RE = re.compile(
    r"\bPy_(?:X?DECREF|CLEAR|X?SETREF)\s*\(\s*(?:\([^()]*\)\s*)?"
    r"[A-Za-z_][\w]*(?:\s*(?:->|\.)\s*[A-Za-z_]\w*)*\s*(?:->|\.)\s*([A-Za-z_]\w*)\s*[,)]")
MEMBER_RE = re.compile(r"(?:->|\.)\s*([A-Za-z_]\w*)")
VISIT_RE = re.compile(
    r"\b(?:Py_VISIT|_Py_VISIT_VALUE|visit)\s*\(\s*(?:\([^()]*\)\s*)?"
    r"[A-Za-z_][\w]*(?:\s*(?:->|\.)\s*[A-Za-z_]\w*)*\s*(?:->|\.)\s*([A-Za-z_]\w*)\s*[,)]")


# How many pointers to plain structs a reference may go through.
MAX_POINTER_DEPTH = 2

# Plain structs that an object points to without owning them: module,
# thread, interpreter and runtime states.
BORROWED_RE = re.compile(r"(?i)state$")


@dataclass
class Struct:
    name: str
    path: Path
    line: int
    base: str | None            # the struct it starts with, if any
    members: list[str]          # declarations after the base
    refs: list[str] = field(default_factory=list)


@dataclass
class TypeDef:
    name: str                   # tp_name, or the C variable
    var: str
    path: Path
    line: int
    struct: str | None
    traverse: str | None
    dealloc: str | None
    clear: str | None
    gc: bool
    base: str | None            # tp_base, from which it may inherit


def strip_comments(text: str) -> str:
    """Remove comments, keeping newlines so that line numbers hold."""
    def repl(m: re.Match) -> str:
        s = m.group(0)
        if s.startswith("/"):
            return "\n" * s.count("\n") + " "
        return s
    return re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'',
                  repl, text, flags=re.S)


def preprocess(text: str) -> str:
    """Resolve #if blocks by FALSE_MACROS; blank out directives."""
    out = []
    stack: list[tuple[bool, bool]] = []   # (active, taken)
    active = True
    lines = text.split("\n")
    i = 0
    while i < len(lines):
        line = lines[i]
        s = line.strip()
        if s.startswith("#"):
            # Join continuation lines of the directive.
            full = s
            while full.endswith("\\") and i + 1 < len(lines):
                i += 1
                out.append("")
                full = full[:-1] + " " + lines[i].strip()
            d = full[1:].strip()
            word = d.split(None, 1)[0] if d else ""
            rest = d[len(word):].strip()
            if word in ("if", "ifdef", "ifndef"):
                cond = evaluate(word, rest)
                stack.append((active, cond))
                active = active and cond
            elif word == "elif":
                parent, taken = stack[-1]
                cond = not taken and evaluate("if", rest)
                stack[-1] = (parent, taken or cond)
                active = parent and cond
            elif word == "else":
                parent, taken = stack[-1]
                stack[-1] = (parent, True)
                active = parent and not taken
            elif word == "endif":
                if stack:
                    active = stack.pop()[0]
            if word == "define" and active:
                out.append("#" + d)
            else:
                out.append("")
        else:
            out.append(line if active else "")
        i += 1
    return "\n".join(out)


def evaluate(word: str, rest: str) -> bool:
    names = re.findall(r"[A-Za-z_]\w*", rest)
    if word == "ifdef":
        return names[0] not in FALSE_MACROS if names else True
    if word == "ifndef":
        # Taken as true, so that include guards and "#ifndef Py_LIMITED_API"
        # keep their contents.
        return True
    if rest.strip() == "0":
        return False
    neg = rest.lstrip().startswith("!")
    for n in names:
        if n in FALSE_MACROS:
            return neg
    return True


def head_macros(texts: list[str]) -> dict[str, str]:
    """#define NAME[(args)] bodies that begin an object struct."""
    defines = {}
    for text in texts:
        for m in re.finditer(r"^#define\s+([A-Za-z_]\w*)(\([^)]*\))?\s+(.*)$",
                             text, flags=re.M):
            defines[m.group(1)] = m.group(3)
    macros = {}
    for name, body in defines.items():
        # Expand a leading macro that is itself a head, such as
        # _PyTZINFO_HEAD in _PyDateTime_DATETIMEHEAD.
        for _ in range(5):
            lead = re.match(r"\s*([A-Za-z_]\w*)\b", body)
            if not lead or lead.group(1) not in defines \
                    or lead.group(1).startswith("PyObject_"):
                break
            body = defines[lead.group(1)] + " " + body[lead.end():]
        if re.match(r"\s*(PyObject_HEAD|PyObject_VAR_HEAD|PyObject\s+ob_base|"
                    r"PyVarObject\s+ob_base)", body):
            macros[name] = body
    return macros


def split_top(body: str, sep: str) -> list[str]:
    parts, depth, cur = [], 0, []
    for ch in body:
        if ch in "({[":
            depth += 1
        elif ch in ")}]":
            depth -= 1
        if ch == sep and depth == 0:
            parts.append("".join(cur))
            cur = []
        else:
            cur.append(ch)
    if "".join(cur).strip():
        parts.append("".join(cur))
    return parts


DECL_RE = re.compile(r"^(?:(?:const|volatile|struct|union|enum)\s+)*([A-Za-z_]\w*)\s*(.*)$")
DECLARATOR_RE = re.compile(
    r"^((?:\*\s*(?:const\b\s*)?)*)([A-Za-z_]\w*)\s*((?:\[[^\]]*\]\s*)*)(?::\s*\w+)?$")


@dataclass
class Plain:
    """The structs that are not objects, by struct tag and typedef name,
    and the typedefs that rename them."""
    members: dict[str, list[str]] = field(default_factory=dict)
    aliases: dict[str, str] = field(default_factory=dict)

    def resolve(self, typ: str) -> str:
        for _ in range(5):
            if typ in self.members or typ not in self.aliases:
                break
            typ = self.aliases[typ]
        return typ


def member_refs(members: list[str], structs: dict[str, Struct], plain: Plain,
                depth: int = MAX_POINTER_DEPTH,
                within: frozenset[int] = frozenset()) -> list[str]:
    """The object references among a struct's members.  A member that is a
    plain struct contributes that struct's references as "member.field", and
    a member that points to one contributes them as "member->field", to
    depth pointers.  within holds the plain structs that the path to here
    went through (by the identity of their member lists, which a struct tag
    and its typedef share), so that a list's links are not followed."""
    refs = []
    for mem in members:
        mem = re.sub(r"\s+", " ", mem.replace("\n", " ")).strip()
        if "(" in mem or "{" in mem:    # function pointers, macros, unions
            continue
        dm = DECL_RE.match(mem)
        if not dm:
            continue
        typ = dm.group(1)
        obj = is_object_type(typ, structs)
        target = None if obj else plain.resolve(typ)
        if target is not None and target not in plain.members:
            continue
        borrowed = target is not None and (BORROWED_RE.search(typ)
                                           or BORROWED_RE.search(target))
        for d in dm.group(2).split(","):
            m = DECLARATOR_RE.match(d.strip())
            if not m:
                continue
            stars = m.group(1).count("*")
            name, array = m.group(2), bool(m.group(3))
            if typ == "PyValue":
                # A value holds a reference unless it is an inline number.
                refs.append(name + ("[]" if array or stars else ""))
                continue
            if obj:
                if stars:
                    refs.append(name + ("[]" if array or stars > 1 else ""))
                continue
            key = id(plain.members[target])
            if key in within:
                continue
            if stars == 0:
                prefix = name + ("[]" if array else "") + "."
                sub = member_refs(plain.members[target], structs, plain,
                                  depth, within | {key})
            elif depth > 0 and not borrowed:
                prefix = name + ("[]" if array or stars > 1 else "") + "->"
                sub = member_refs(plain.members[target], structs, plain,
                                  depth - 1, within | {key})
            else:
                continue
            refs.extend(prefix + r for r in sub)
    return refs


def parse_structs(path: Path, text: str, macros: dict[str, str],
                  structs: dict[str, Struct], plain: Plain) -> None:
    """Record object structs in structs, and the members of plain structs,
    which objects can embed or point to, and the typedefs that name them,
    in plain."""
    for m in re.finditer(r"\btypedef\s+(?:struct\s+)?([A-Za-z_]\w*)\s+([A-Za-z_]\w*)\s*;",
                         text):
        if m.group(1) != m.group(2):
            plain.aliases[m.group(2)] = m.group(1)
    pat = re.compile(r"\b(typedef\s+)?struct\s*([A-Za-z_]\w*)?\s*\{")
    for m in pat.finditer(text):
        start = m.end()
        depth, i = 1, start
        while i < len(text) and depth:
            if text[i] == "{":
                depth += 1
            elif text[i] == "}":
                depth -= 1
            i += 1
        body = text[start:i - 1]
        after = text[i:i + 200]
        names = []
        if m.group(2):
            names.append(m.group(2))
        if m.group(1):
            t = re.match(r"\s*([A-Za-z_]\w*)\s*;", after)
            if t:
                names.append(t.group(1))
        if not names:
            continue
        members = [p.strip() for p in split_top(body, ";") if p.strip()]
        if not members:
            continue
        first = members[0]
        base = None
        head = re.match(r"([A-Za-z_]\w*)(\s*\([^)]*\))?", first)
        h = head.group(1) if head else ""
        if h in ("PyObject_HEAD", "PyObject_VAR_HEAD"):
            # The macro has no trailing semicolon.
            base = h
            members = [first[head.end():].strip()] + members[1:]
            first = None
        elif h in macros:
            base = h
            # The macro's own members come first.
            members = [p.strip() for p in split_top(
                re.sub(r"^\s*PyObject(_VAR)?_HEAD", "", macros[h]), ";")
                if p.strip()] + [first[head.end():].strip()] + members[1:]
            first = None
        elif re.match(r"(?:struct\s+)?[A-Za-z_]\w*\s+[A-Za-z_]\w*$", first):
            typ = first.split()[-2]
            if typ in ("PyObject", "PyVarObject") or typ in structs:
                base = typ
            else:
                base = None
        if base is None:
            for n in names:
                if n not in structs:
                    plain.members[n] = members
            continue
        line = text.count("\n", 0, m.start()) + 1
        for n in names:
            structs[n] = Struct(n, path, line, base,
                                members[1:] if first is not None else members)


def is_object_type(typ: str, structs: dict[str, Struct]) -> bool:
    if typ in ("PyObject", "PyTypeObject", "PyVarObject", "PyValue"):
        return True
    if typ in structs:
        return True
    return bool(re.match(r"Py\w*Object$", typ))


def parse_types(path: Path, text: str, types: list[TypeDef]) -> None:
    # Static types
    for m in re.finditer(r"\bPyTypeObject\s+([A-Za-z_]\w*)\s*=\s*\{", text):
        body = brace_body(text, m.end())
        items = [p.strip() for p in split_top(body, ",")]
        fields = {}
        positional = 0
        for it in items:
            d = re.match(r"\.(\w+)\s*=\s*(.*)$", it, flags=re.S)
            if d:
                fields[d.group(1)] = d.group(2).strip()
            elif positional == 0 and "HEAD_INIT" in it:
                # The macro supplies its own trailing comma.
                positional = 1
                rest = re.sub(r"^.*?HEAD_INIT\s*\([^()]*(?:\([^()]*\)[^()]*)*\)", "",
                              it, flags=re.S).strip()
                if rest:
                    fields[TYPE_FIELDS[0]] = rest
                    positional = 2
            else:
                if 0 < positional <= len(TYPE_FIELDS):
                    fields[TYPE_FIELDS[positional - 1]] = it
                positional += 1
        add_type(types, path, text, m, m.group(1), fields)
    # Spec types
    slots = {}
    for m in re.finditer(r"\bPyType_Slot\s+([A-Za-z_]\w*)\s*\[\s*\]\s*=\s*\{", text):
        body = brace_body(text, m.end())
        d = {}
        for s in re.finditer(r"\{\s*(Py_tp_\w+)\s*,\s*([^}]*)\}", body):
            d[s.group(1)] = re.sub(r"^\(\s*\w+[\s*]*\)\s*", "", s.group(2).strip())
        slots[m.group(1)] = d
    for m in re.finditer(r"\bPyType_Spec\s+([A-Za-z_]\w*)\s*=\s*\{", text):
        body = brace_body(text, m.end())
        items = [p.strip() for p in split_top(body, ",")]
        spec = {}
        order = ["name", "basicsize", "itemsize", "flags", "slots"]
        pos = 0
        for it in items:
            d = re.match(r"\.(\w+)\s*=\s*(.*)$", it, flags=re.S)
            if d:
                spec[d.group(1)] = d.group(2).strip()
            elif pos < len(order):
                spec[order[pos]] = it
                pos += 1
        sl = slots.get(spec.get("slots", "").strip(), {})
        fields = {
            "tp_name": spec.get("name", m.group(1)),
            "tp_basicsize": spec.get("basicsize", ""),
            "tp_flags": spec.get("flags", ""),
            "tp_traverse": sl.get("Py_tp_traverse"),
            "tp_dealloc": sl.get("Py_tp_dealloc"),
            "tp_clear": sl.get("Py_tp_clear"),
            "tp_base": sl.get("Py_tp_base"),
        }
        add_type(types, path, text, m, m.group(1), fields)


def brace_body(text: str, start: int) -> str:
    depth, i = 1, start
    while i < len(text) and depth:
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
        i += 1
    return text[start:i - 1]


def func_name(expr: str | None) -> str | None:
    if not expr:
        return None
    expr = re.sub(r"\(\s*\w+[\s*]*\)", "", expr).strip()
    if expr in ("0", "NULL", ""):
        return None
    m = re.match(r"([A-Za-z_]\w*)$", expr)
    return m.group(1) if m else expr


def add_type(types, path, text, m, var, fields) -> None:
    size = fields.get("tp_basicsize", "")
    sm = re.search(r"sizeof\s*\(\s*(?:struct\s+)?([A-Za-z_]\w*)\s*\)", size)
    name = re.sub(r"^\.tp_name\s*=\s*", "", fields.get("tp_name", var))
    types.append(TypeDef(
        name=name.strip('" \n'), var=var, path=path,
        line=text.count("\n", 0, m.start()) + 1,
        struct=sm.group(1) if sm else None,
        traverse=func_name(fields.get("tp_traverse")),
        dealloc=func_name(fields.get("tp_dealloc")),
        clear=func_name(fields.get("tp_clear")),
        gc="HAVE_GC" in fields.get("tp_flags", ""),
        base=func_name(fields.get("tp_base")) or runtime_base(text, var),
    ))


FROM_SPEC_RE = re.compile(r"\bPyType_From(?:SpecWithBases|ModuleAndSpec|Metaclass)\s*\(")


def runtime_base(text: str, var: str) -> str | None:
    """The base a module's init gives a type: "var.tp_base = base", or the
    bases argument of PyType_FromSpecWithBases() and its relatives."""
    m = re.search(rf"\b{var}\s*\.\s*tp_base\s*=\s*([^;]+);", text)
    if m:
        return func_name(m.group(1))
    for m in FROM_SPEC_RE.finditer(text):
        args = [a.strip() for a in split_top(brace_body_paren(text, m.end()), ",")]
        for i, a in enumerate(args[:-1]):
            if re.fullmatch(rf"&\s*{var}", a):
                return func_name(args[i + 1])
    return None


def brace_body_paren(text: str, start: int) -> str:
    depth, i = 1, start
    while i < len(text) and depth:
        if text[i] == "(":
            depth += 1
        elif text[i] == ")":
            depth -= 1
        i += 1
    return text[start:i - 1]


# The start of a function definition at column 0: an optional macro such as
# Py_LOCAL_INLINE(int), an optional return type on the same line, and the
# name with its opening parenthesis.  Whether it is a definition is decided
# by the brace after the balanced parameter list, so that a return-type
# macro alone on the line before (LOCAL(void)) is not taken as the function.
DEF_RE = re.compile(r"^(?:[A-Za-z_]\w*\([^()\n]*\)[ \t]+)?"
                    r"(?:[A-Za-z_][\w \t*]*?[ \t*])?([A-Za-z_]\w*)\s*\(", re.M)
KEYWORDS = {"if", "for", "while", "switch", "return", "sizeof", "do", "else"}


def functions(text: str) -> dict[str, str]:
    """The bodies of a file's functions and function-like macros."""
    return {name: body for name, _, body in definitions(text)}


def definitions(text: str):
    """(name, parameter list, body) of a file's functions and function-like
    macros."""
    for m in re.finditer(r"^#define\s+([A-Za-z_]\w*)\(([^)]*)\)(.*)$", text, flags=re.M):
        yield m.group(1), m.group(2), m.group(3)
    for m in DEF_RE.finditer(text):
        if m.group(1) in KEYWORDS:
            continue
        end = paren_end(text, m.end())
        b = re.compile(r"\s*\{").match(text, end)
        if b:
            yield m.group(1), text[m.end():end - 1], brace_body(text, b.end())


def receivers(params: str, body: str) -> set[str]:
    """The names a traverse function has for the object it visits: its
    first parameter and the locals initialised from it."""
    first = split_top(params, ",")[:1]
    m = re.search(r"([A-Za-z_]\w*)\s*$", first[0]) if first else None
    if not m:
        return set()
    names = {m.group(1)}
    for d in re.finditer(r"\b([A-Za-z_]\w*)\s*=\s*([^;=]*);", body):
        if re.search(rf"\b{m.group(1)}\b", d.group(2)):
            names.add(d.group(1))
    return names


def paren_end(text: str, start: int) -> int:
    """The index after the parenthesis that closes the one before start."""
    depth, i = 1, start
    while i < len(text) and depth:
        if text[i] == "(":
            depth += 1
        elif text[i] == ")":
            depth -= 1
        i += 1
    return i


# Members that hold no reference: weak reference lists.
NOT_OWNED_RE = re.compile(r"weakreflist|weaklist$")


def all_refs(name: str, structs: dict[str, Struct]) -> list[str]:
    refs = []
    seen = set()
    while name in structs and name not in seen:
        seen.add(name)
        s = structs[name]
        refs = [r for r in s.refs if not NOT_OWNED_RE.search(r)] + refs
        name = s.base or ""
    return refs


CALL_RE = re.compile(r"\b([A-Za-z_]\w*)\s*\(")
IDENT_RE = re.compile(r"\b([A-Za-z_]\w*)\b")


def with_callees(name: str, fs: dict[str, str],
                 headers: tuple[dict[str, str], ...] = (),
                 depth: int = 2) -> str:
    """The body of the function name and of the functions it calls, to
    depth calls.  A callee is looked up in the same file, then in headers
    (the file's directory, then Include/), which hold the accessors such as
    PyInstanceMethod_GET_FUNCTION() and helpers such as sre_lib.h's.  A
    function of the same file that the body names without calling it, such
    as a visit callback handed to an enumerator, is read as a callee."""
    scopes = (fs, *headers)
    bodies, seen, todo = [], set(), [(name, 0)]
    while todo:
        f, d = todo.pop()
        if f in seen:
            continue
        body = next((s[f] for s in scopes if f in s), None)
        if body is None:
            continue
        seen.add(f)
        bodies.append(body)
        if d < depth:
            callees = set(CALL_RE.findall(body))
            callees |= {w for w in IDENT_RE.findall(body) if w in fs}
            todo.extend((c, d + 1) for c in callees)
    return "\n".join(bodies)


# A traverse that hands the object to another type's traverse:
# "PyDict_Type.tp_traverse(op, ...)", "((PyTypeObject *)PyExc_BaseException)->
# tp_traverse(self, ...)", "Py_TYPE(op)->tp_base->tp_traverse(op, ...)".
DELEGATE_RE = re.compile(
    r"\b([A-Za-z_]\w*)\s*\)?\s*(?:\.|->)\s*tp_traverse\s*\(\s*([^,]*),")


@dataclass
class Sources:
    """The functions of the parsed files, by file and by header directory."""
    funcs: dict[Path, dict[str, str]]
    params: dict[Path, dict[str, str]]
    headers: dict[Path, dict[str, str]]
    include: Path

    def scopes(self, path: Path) -> tuple[dict[str, str], ...]:
        return (self.headers.get(path.parent, {}),
                self.headers.get(self.include, {}))


def traverse_names(t: TypeDef, src: Sources, by_var: dict[str, TypeDef],
                   structs: dict[str, Struct],
                   seen: frozenset[str] = frozenset()) -> tuple[str, set[str]] | None:
    """The code of t's traverse with its callees, and the exact member paths
    that it visits by delegating to another type's traverse; None when the
    traverse is not found."""
    fs = src.funcs.get(t.path, {})
    if t.traverse not in fs:
        return None
    code = with_callees(t.traverse, fs, src.scopes(t.path))
    return code, delegated_paths(t, code, src, by_var, structs,
                                 seen | {t.var})


def delegated_paths(t: TypeDef, code: str, src: Sources,
                    by_var: dict[str, TypeDef], structs: dict[str, Struct],
                    seen: frozenset[str]) -> set[str]:
    """The member paths visited by the traverse of another type that code
    calls on t's own object, when that type's struct is t's struct or one of
    its bases: the paths of the delegate's struct that the delegate's
    traverse names (and those it delegates in turn).  A delegate that cannot
    be resolved, or a call on another object, credits nothing."""
    chain, name = set(), t.struct
    while name in structs and name not in chain:
        chain.add(name)
        name = structs[name].base or ""
    own = receivers(src.params.get(t.path, {}).get(t.traverse, ""),
                    src.funcs.get(t.path, {}).get(t.traverse, ""))
    out = set()
    for x, arg in DELEGATE_RE.findall(code):
        arg = re.sub(r"\(\s*\w+[\s*]*\)", "", arg).strip("() \t\n")
        if arg not in own:
            continue
        if x == "tp_base":
            x = t.base or ""
        d = by_var.get(x) or by_var.get("_" + x)
        if d is None or d.var in seen or d.struct not in chain:
            continue
        found = traverse_names(d, src, by_var, structs, seen)
        if found is None:
            continue
        dcode, dpaths = found
        names = set(MEMBER_RE.findall(dcode))
        out |= dpaths | {r.rstrip("[]") for r in all_refs(d.struct, structs)
                         if last_member(r) in names}
    return out


def last_member(ref: str) -> str:
    """The member a path such as "head->base.obj" ends in."""
    return re.split(r"->|\.", ref.rstrip("[]"))[-1].rstrip("[]")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--root", default=Path(__file__).resolve().parents[2], type=Path)
    ap.add_argument("--missing-only", action="store_true",
                    help="only report types with references and no tp_traverse")
    ap.add_argument("--check", action="store_true",
                    help="exit 1 on a missing, partial or silent traverse "
                         "that the allowlist does not name")
    ap.add_argument("--allow", type=Path,
                    default=Path(__file__).with_name("traverse_allow.txt"),
                    help="the allowlist for --check")
    args = ap.parse_args()
    if args.check:
        args.missing_only = False

    files = []
    for d in SOURCE_DIRS:
        for p in sorted((args.root / d).rglob("*")):
            if p.suffix in (".c", ".h") and "mimalloc" not in p.parts \
                    and "_decimal/libmpdec" not in str(p) and "expat" not in p.parts:
                files.append(p)
    texts = {}
    for p in files:
        try:
            raw = p.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        texts[p] = preprocess(strip_comments(raw))

    macros = head_macros(list(texts.values()))
    structs: dict[str, Struct] = {}
    plain = Plain()
    # Two passes, so that a struct can start with one defined later.
    for _ in range(2):
        for p, t in texts.items():
            parse_structs(p, t, macros, structs, plain)
    for s in structs.values():
        s.refs = member_refs(s.members, structs, plain)
    types: list[TypeDef] = []
    # The functions of the .c files, and of the headers by directory, with
    # Include/'s as one.
    src = Sources({}, {}, {}, args.root / "Include")
    for p, t in texts.items():
        defs = list(definitions(t))
        if p.suffix == ".c":
            parse_types(p, t, types)
            src.funcs[p] = {name: body for name, _, body in defs}
            src.params[p] = {name: params for name, params, _ in defs}
        else:
            key = src.include if src.include in p.parents else p.parent
            src.headers.setdefault(key, {}).update(
                (name, body) for name, _, body in defs)
    by_var = {t.var: t for t in types}

    rel = lambda p: p.relative_to(args.root)
    missing = []
    inherited = []
    partial = []
    unvisited = []
    unresolved = []
    # A type without tp_traverse that has a base, or whose struct another
    # type with a tp_traverse uses, most likely inherits one.  A struct
    # defined in a .c file is shared only within it: several example modules
    # define their own XxoObject.
    traversed_here = {(t.struct, t.path) for t in types if t.traverse}
    traversed_structs = {t.struct for t in types if t.traverse
                         and t.struct in structs
                         and structs[t.struct].path.suffix == ".h"}
    for t in types:
        if t.struct is None:
            continue
        refs = all_refs(t.struct, structs)
        if not refs:
            continue
        if t.traverse is None:
            if t.struct in traversed_structs or t.base \
                    or (t.struct, t.path) in traversed_here:
                inherited.append((t, refs))
            else:
                missing.append((t, refs))
            continue
        if args.missing_only:
            continue
        if t.traverse == "_PyObject_VisitType":
            # The generic traverse function visits only the type.
            partial.append((t, sorted(r.rstrip("[]") for r in refs)))
            continue
        found = traverse_names(t, src, by_var, structs)
        if found is None:
            unresolved.append((t, sorted(r.rstrip("[]") for r in refs)))
            continue
        code, delegated = found
        # Any member the traverse function, or a function that it calls,
        # names counts as visited, which also covers casts and helper
        # functions taking the member.  Releases are read the same way, as
        # a list's nodes are often freed by a helper.
        visited = set(MEMBER_RE.findall(code))
        released = set()
        for f in (t.dealloc, t.clear):
            if f:
                code = with_callees(f, src.funcs.get(t.path, {}),
                                    src.scopes(t.path))
                released |= set(RELEASE_RE.findall(code))
                # Releasing a Py_buffer releases its obj.
                if re.search(r"\bPyBuffer_Release\s*\(", code):
                    released.add("obj")
        refs = [r for r in refs if r.rstrip("[]") not in delegated]
        miss = sorted(r.rstrip("[]") for r in refs
                      if last_member(r) in released - visited)
        if miss:
            partial.append((t, miss))
        # Report 3: a member that neither the traverse nor any release names.
        # Under a tracing collector a reference the traverse does not visit
        # is freed under the object whether or not the dealloc released it
        # (upstream leaked such members; mbstreamwriter's pending, 6d rung 4).
        silent = sorted(r.rstrip("[]") for r in refs
                        if last_member(r) not in visited | released)
        if silent:
            unvisited.append((t, silent))

    if args.check:
        return check(args.allow, rel, missing, partial, unvisited, unresolved)
    print(f"{len(missing)} types with object references and no tp_traverse:")
    for t, refs in sorted(missing, key=lambda x: (str(x[0].path), x[0].line)):
        print(f"  {rel(t.path)}:{t.line}: {t.name} ({t.struct}): {', '.join(refs)}")
    print(f"\n{len(inherited)} types without tp_traverse that have a base or "
          f"share a traversed struct (probably inherited):")
    for t, refs in sorted(inherited, key=lambda x: (str(x[0].path), x[0].line)):
        print(f"  {rel(t.path)}:{t.line}: {t.name} ({t.struct})")
    if not args.missing_only:
        print(f"\n{len(partial)} types whose tp_traverse misses members that "
              f"dealloc or clear releases:")
        for t, miss in sorted(partial, key=lambda x: (str(x[0].path), x[0].line)):
            print(f"  {rel(t.path)}:{t.line}: {t.name} ({t.traverse}): {', '.join(miss)}")
        print(f"\n{len(unvisited)} types whose tp_traverse does not name a member "
              f"that holds references and that no release names either:")
        for t, miss in sorted(unvisited, key=lambda x: (str(x[0].path), x[0].line)):
            print(f"  {rel(t.path)}:{t.line}: {t.name} ({t.traverse}): {', '.join(miss)}")
        print(f"\n{len(unresolved)} types whose tp_traverse the tool cannot find:")
        for t, refs in sorted(unresolved, key=lambda x: (str(x[0].path), x[0].line)):
            print(f"  {rel(t.path)}:{t.line}: {t.name} ({t.traverse}): {', '.join(refs)}")
    return 0


def read_allowlist(path: Path) -> tuple[dict[str, tuple[set[str], str]], list[str]]:
    """The allowlist's entries, by "path variable", each with the member
    paths it allows and its reason, and its malformed lines."""
    allowed, bad = {}, []
    for n, line in enumerate(path.read_text().splitlines(), 1):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        m = re.match(r"(\S+)\s+([A-Za-z_]\w*)((?:\s+[^\s:]+)+)\s*:\s*(\S.*)$", line)
        if m:
            allowed[f"{m.group(1)} {m.group(2)}"] = (
                {r.rstrip("[]") for r in m.group(3).split()}, m.group(4))
        else:
            bad.append(f"{path.name}:{n}: not \"path variable member...: "
                       f"reason\": {line}")
    return allowed, bad


def check(allow: Path, rel, missing, partial, unvisited, unresolved) -> int:
    allowed, errors = read_allowlist(allow) if allow.exists() else ({}, [])
    reported: dict[str, set[str]] = {}
    for what, found in (("no tp_traverse", missing),
                        ("a partial tp_traverse", partial),
                        ("a tp_traverse that names neither these members "
                         "nor their release", unvisited),
                        ("a tp_traverse the tool cannot find", unresolved)):
        for t, refs in sorted(found, key=lambda x: (str(x[0].path), x[0].line)):
            key = f"{rel(t.path)} {t.var}"
            paths = [r.rstrip("[]") for r in refs]
            reported.setdefault(key, set()).update(paths)
            # An entry allows the member paths it lists, so that a member
            # added to the type later is reported again.
            listed = allowed[key][0] if key in allowed else set()
            unlisted = [r for r in paths if r not in listed]
            if unlisted:
                errors.append(f"{rel(t.path)}:{t.line}: {t.name} has {what} "
                              f"and {allow.name} does not list: "
                              f"{', '.join(unlisted)}")
    for key in sorted(allowed):
        stale = sorted(allowed[key][0] - reported.get(key, set()))
        if key not in reported:
            errors.append(f"{allow.name}: {key} is not reported any more; "
                          f"remove the entry")
        elif stale:
            errors.append(f"{allow.name}: {key}: {', '.join(stale)} not "
                          f"reported any more; remove them from the entry")
    for e in errors:
        print(e)
    if errors:
        print(f"traverse_audit --check: {len(errors)} problems")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
