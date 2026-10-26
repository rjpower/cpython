"""Find object types that hold references a tracing collector cannot see.

The tracing collector (note 06, step 4; Python/gc_trace.c) follows
tp_traverse, so a type whose instances hold object pointers must have one,
whether or not it is a GC type, and it must visit every reference the
instance owns.  This tool parses the C sources and reports:

1. types whose instance struct has PyObject* (or other object pointer)
   members and that have no tp_traverse;
2. types with a tp_traverse that does not visit a member that their
   tp_dealloc or tp_clear releases.

The parse is heuristic: struct definitions that begin with PyObject_HEAD,
PyObject_VAR_HEAD, another object struct or a macro that expands to one;
static PyTypeObject initializers (positional or designated) and PyType_Spec
definitions, matched to structs by sizeof() in the basic size.  Report 2
reads member names from Py_DECREF/Py_XDECREF/Py_CLEAR/Py_XSETREF/Py_SETREF in
the dealloc and clear functions and from Py_VISIT in the traverse function,
so a traverse that delegates to a helper is listed and needs a look.

Usage: python Tools/boil/traverse_audit.py [--root DIR] [--missing-only]
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
                "Py_STATS", "_Py_TIER2", "Py_SHAPES", "__EMSCRIPTEN__"}

# The PyTypeObject fields in order, after PyVarObject_HEAD_INIT.
TYPE_FIELDS = [
    "tp_name", "tp_basicsize", "tp_itemsize", "tp_dealloc",
    "tp_vectorcall_offset", "tp_getattr", "tp_setattr", "tp_as_async",
    "tp_repr", "tp_as_number", "tp_as_sequence", "tp_as_mapping", "tp_hash",
    "tp_call", "tp_str", "tp_getattro", "tp_setattro", "tp_as_buffer",
    "tp_flags", "tp_doc", "tp_traverse", "tp_clear",
]

RELEASE_RE = re.compile(
    r"\bPy_(?:X?DECREF|CLEAR|X?SETREF)\s*\(\s*(?:\([^()]*\)\s*)?"
    r"[A-Za-z_][\w]*(?:\s*(?:->|\.)\s*[A-Za-z_]\w*)*\s*(?:->|\.)\s*([A-Za-z_]\w*)\s*[,)]")
MEMBER_RE = re.compile(r"(?:->|\.)\s*([A-Za-z_]\w*)")
VISIT_RE = re.compile(
    r"\b(?:Py_VISIT|_Py_VISIT_STACKREF|visit)\s*\(\s*(?:\([^()]*\)\s*)?"
    r"[A-Za-z_][\w]*(?:\s*(?:->|\.)\s*[A-Za-z_]\w*)*\s*(?:->|\.)\s*([A-Za-z_]\w*)\s*[,)]")


@dataclass
class Struct:
    name: str
    path: Path
    line: int
    base: str | None            # the struct it starts with, if any
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


OBJ_PTR_RE = re.compile(r"^(?:const\s+)?(?:struct\s+)?([A-Za-z_]\w*)\s*(\*+)\s*([A-Za-z_]\w*)\s*(\[[^\]]*\])?$")


def parse_structs(path: Path, text: str, macros: dict[str, str],
                  structs: dict[str, Struct]) -> None:
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
        if not head:
            continue
        h = head.group(1)
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
                continue
        else:
            continue
        refs = []
        for mem in members[1:] if first is not None else members:
            mem = re.sub(r"\s+", " ", mem.replace("\n", " ")).strip()
            if "(" in mem:      # function pointers, macros
                continue
            decl = mem.split(",")
            mm = OBJ_PTR_RE.match(decl[0].strip())
            if not mm:
                continue
            typ, stars = mm.group(1), mm.group(2)
            if not is_object_type(typ, structs):
                continue
            refs.append(mm.group(3) + ("[]" if mm.group(4) or len(stars) > 1 else ""))
            for d in decl[1:]:
                dm = re.match(r"\s*(\*+)\s*([A-Za-z_]\w*)\s*(\[[^\]]*\])?\s*$", d)
                if dm:
                    refs.append(dm.group(2) + ("[]" if dm.group(3) or len(dm.group(1)) > 1 else ""))
        line = text.count("\n", 0, m.start()) + 1
        for n in names:
            structs[n] = Struct(n, path, line, base, refs)


def is_object_type(typ: str, structs: dict[str, Struct]) -> bool:
    if typ in ("PyObject", "PyTypeObject", "PyVarObject"):
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
            d[s.group(1)] = re.sub(r"^\(\w+\s*\)\s*", "", s.group(2).strip())
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
    expr = re.sub(r"\(\s*\w+\s*\)", "", expr).strip()
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
        base=func_name(fields.get("tp_base")),
    ))


def functions(text: str) -> dict[str, str]:
    out = {}
    for m in re.finditer(r"^([A-Za-z_]\w*)\s*\([^;{]*\)\s*\{", text, flags=re.M):
        out[m.group(1)] = brace_body(text, m.end())
    return out


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


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--root", default=Path(__file__).resolve().parents[2], type=Path)
    ap.add_argument("--missing-only", action="store_true",
                    help="only report types with references and no tp_traverse")
    args = ap.parse_args()

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
    # Two passes, so that a struct can start with one defined later.
    for _ in range(2):
        for p, t in texts.items():
            parse_structs(p, t, macros, structs)
    types: list[TypeDef] = []
    funcs: dict[Path, dict[str, str]] = {}
    for p, t in texts.items():
        if p.suffix == ".c":
            parse_types(p, t, types)
            funcs[p] = functions(t)

    rel = lambda p: p.relative_to(args.root)
    missing = []
    inherited = []
    partial = []
    # A type without tp_traverse that has a base, or whose struct another
    # type with a tp_traverse uses, most likely inherits one.
    traversed_structs = {t.struct for t in types if t.traverse}
    for t in types:
        if t.struct is None:
            continue
        refs = all_refs(t.struct, structs)
        if not refs:
            continue
        if t.traverse is None:
            if t.struct in traversed_structs or t.base:
                inherited.append((t, refs))
            else:
                missing.append((t, refs))
            continue
        if args.missing_only:
            continue
        fs = funcs.get(t.path, {})
        body = fs.get(t.traverse)
        if body is None:
            continue
        # Any member the traverse function names counts as visited, which
        # also covers casts and helper functions taking the member.
        visited = set(MEMBER_RE.findall(body))
        released = set()
        for f in (t.dealloc, t.clear):
            if f and f in fs:
                released |= set(RELEASE_RE.findall(fs[f]))
        names = {r.rstrip("[]") for r in refs}
        miss = sorted((released & names) - visited)
        if miss:
            partial.append((t, miss))

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
    return 0


if __name__ == "__main__":
    sys.exit(main())
