"""Inventory of getset entries under Objects/, Modules/, Python/ and PC/.
Run from the repository root.

Counted: the initialisers of PyGetSetDef arrays, of scalar PyGetSetDef
definitions and of compound literals (`(PyGetSetDef[]) {...}`); entries
written through a function-like macro, expanded from its definition in
the same file, and Argument Clinic's *_GETSETDEF macros, resolved in the
generated headers (with or without casts); and getsets filled in at run
time (`.getset.get = f`), reported as "dynamic".  An entry whose getter or
setter does not resolve to a function name is reported and makes the
script exit 1."""
import collections
import pathlib
import re
import sys

DIRS = ("Objects", "Modules", "Python", "PC")
ARRAY = re.compile(r'PyGetSetDef\s+(\w+)\s*\[\s*\w*\s*\]\s*=\s*\{(.*?)\n\s*\};',
                   re.S)
SCALAR = re.compile(r'PyGetSetDef\s+(\w+)\s*=\s*\{(.*?)\};', re.S)
LITERAL = re.compile(r'\(PyGetSetDef\s*\[\s*\]\)\s*\{')
DEFINE = re.compile(r'#\s*define\s+(\w+)\(([^)]*)\)((?:[^\n]*\\\n)*[^\n]*)')
CLINIC = re.compile(r'#\s*define\s+(\w+_GETSETDEF)\s*\\?\s*\n?\s*\{(.*?)\}', re.S)
DYNAMIC = re.compile(r'(\w+(?:\[\w+\])?(?:\.|->)getset)\.(get|set)\s*=\s*(\w+)\s*;')
IDENT = re.compile(r'^[A-Za-z_]\w*$')


def strip_comments(t):
    t = re.sub(r'/\*.*?\*/', '', t, flags=re.S)
    return re.sub(r'//[^\n]*', '', t)


def files():
    for d in DIRS:
        for p in sorted(pathlib.Path(d).rglob("*")):
            if p.suffix in (".c", ".h"):
                yield p


def macro_defs(text):
    defs = {}
    for m in DEFINE.finditer(text):
        params = [x.strip() for x in m.group(2).split(",") if x.strip()]
        defs[m.group(1)] = (params, m.group(3).replace("\\\n", " "))
    return defs


def expand(defs, name, args):
    params, body = defs[name]
    for p, a in zip(params, args):
        body = re.sub(r'(?<!#)#\s*%s\b' % p, '"%s"' % a, body)
        body = re.sub(r'\s*##\s*%s\b' % p, a, body)
        body = re.sub(r'\b%s\s*##\s*' % p, a, body)
        body = re.sub(r'\b%s\b' % p, a, body)
    return re.sub(r'\s*##\s*', '', body)


def balanced(text, start):
    """The text inside the braces opening at text[start]."""
    depth = 0
    for i in range(start, len(text)):
        if text[i] == '{':
            depth += 1
        elif text[i] == '}':
            depth -= 1
            if depth == 0:
                return text[start + 1:i]
    raise ValueError("unbalanced braces")


def split_top(body):
    """The brace groups and the top-level macro uses of a table body."""
    out = []
    i = 0
    while i < len(body):
        if body[i] == '{':
            inner = balanced(body, i)
            out.append(("entry", inner))
            i += len(inner) + 2
            continue
        m = re.match(r'(\w+)', body[i:])
        if m and (i == 0 or not (body[i - 1].isalnum() or body[i - 1] == '_')):
            word = m.group(1)
            rest = body[i + m.end():]
            call = re.match(r'\s*\(', rest)
            if call:
                start = i + m.end() + call.end() - 1
                depth = 0
                for j in range(start, len(body)):
                    if body[j] == '(':
                        depth += 1
                    elif body[j] == ')':
                        depth -= 1
                        if depth == 0:
                            break
                args = [a.strip() for a in
                        re.split(r',(?![^(]*\))', body[start + 1:j])]
                out.append(("macro", (word, args)))
                i = j + 1
                continue
            if word.endswith("_GETSETDEF"):
                out.append(("macro", (word, [])))
            i += m.end()
            continue
        i += 1
    return out


def clean(f):
    f = f.strip()
    f = re.sub(r'^\(\w+\)\s*', '', f)
    return re.sub(r'_Py_FUNC_CAST\(\w+,\s*(\w+)\)', r'\1', f)


def parse_entry(inner):
    parts = [x.strip() for x in re.split(r',(?![^(]*\))', inner)]
    if not parts or parts[0] in ('NULL', '0', ''):
        return None
    named = {}
    for x in parts:
        m = re.match(r'\.(\w+)\s*=\s*(.*)', x, re.S)
        if m:
            named[m.group(1)] = m.group(2)
    if named:
        name = named.get("name", "?")
        g, s = named.get("get", "NULL"), named.get("set", "NULL")
    else:
        name = parts[0]
        g = parts[1] if len(parts) > 1 else 'NULL'
        s = parts[2] if len(parts) > 2 else 'NULL'
    return name.strip().strip('"'), clean(g), clean(s)


def main():
    clinic = {}
    for p in files():
        if "/clinic/" in str(p):
            for m in CLINIC.finditer(p.read_text(errors="replace")):
                clinic[m.group(1)] = m.group(2).replace("\\\n", " ")
    entries = []        # (file, table, name, getter, setter, via)
    unresolved = []
    for p in files():
        if "/clinic/" in str(p):
            continue
        t = strip_comments(p.read_text(errors="replace"))
        defs = macro_defs(t)
        tables = [(m.group(1), m.group(2), "array") for m in ARRAY.finditer(t)]
        tables += [(m.group(1), "{" + m.group(2) + "}", "scalar")
                   for m in SCALAR.finditer(t)]
        tables += [("(literal)", balanced(t, m.end() - 1), "literal")
                   for m in LITERAL.finditer(t)]
        for table, body, kind in tables:
            # Entries under #if count once each; the directives go.
            body = re.sub(r'^\s*#[^\n]*', '', body, flags=re.M)
            for what, item in split_top(body):
                via = kind
                if what == "macro":
                    name, args = item
                    if name in clinic:
                        inner, via = clinic[name], "clinic"
                    elif name in defs:
                        exp = expand(defs, name, args)
                        sub = split_top(exp)
                        if len(sub) == 1 and sub[0][0] == "macro" \
                                and sub[0][1][0] in clinic:
                            inner, via = clinic[sub[0][1][0]], "clinic"
                        else:
                            groups = [x for w, x in sub if w == "entry"]
                            if len(groups) != 1:
                                unresolved.append((str(p), table, name))
                                continue
                            inner, via = groups[0], "macro"
                    elif name == "PyDoc_STR":
                        continue
                    else:
                        unresolved.append((str(p), table, name))
                        continue
                else:
                    inner = item
                e = parse_entry(inner)
                if e is None:
                    continue
                for f in e[1:]:
                    if f not in ('NULL', '0') and not IDENT.match(f):
                        unresolved.append((str(p), table, f))
                entries.append((str(p), table) + e + (via,))
        for m in DYNAMIC.finditer(t):
            g = m.group(3) if m.group(2) == "get" else "NULL"
            s = m.group(3) if m.group(2) == "set" else "NULL"
            entries.append((str(p), m.group(1), "(run time)", g, s, "dynamic"))
    report(entries, clinic)
    if unresolved:
        for u in unresolved:
            print("unresolved:", "\t".join(u))
        sys.exit(1)


def report(entries, clinic):
    by_via = collections.Counter(e[5] for e in entries)
    getters = {e[3] for e in entries if e[3] not in ('NULL', '0')}
    setters = {e[4] for e in entries if e[4] not in ('NULL', '0')}
    hand = [e for e in entries if e[5] != "clinic"]
    files_ = collections.Counter(e[0] for e in entries)
    print(f"entries {len(entries)} ({dict(by_via)}); getter functions "
          f"{len(getters)} ({len({e[3] for e in hand} - {'NULL', '0'})} "
          f"hand-written), setter functions {len(setters)} "
          f"({len({e[4] for e in hand} - {'NULL', '0'})} hand-written); "
          f"entries with a setter "
          f"{sum(1 for e in entries if e[4] not in ('NULL', '0'))}; "
          f"files {len(files_)}")
    print("clinic GETSETDEF macros defined:", len(clinic))
    print("PyObject_GenericGetDict entries:",
          sum(1 for e in entries if e[3] == "PyObject_GenericGetDict"))
    if "--files" in sys.argv:
        for f, n in sorted(files_.items()):
            print(f"  {n:3} {f}")
    if "--entries" in sys.argv:
        for e in entries:
            print("\t".join(e))


main()
