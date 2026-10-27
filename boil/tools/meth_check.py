"""Check that METH_O and METH_FASTCALL functions take and return values.

A PyMethodDef entry casts its function to PyCFunction, so the compiler
does not see a function whose signature does not match its flags.  This
script finds every entry whose flags include METH_O or METH_FASTCALL, finds
the function's definition, and reports those whose signature is still the
object form: METH_O must be (PyObject *self, PyValue arg) and METH_FASTCALL
must take const PyValue *args, and both must return PyValue.

Usage: meth_check.py [-v] [PATH ...]   (default: Objects Python Modules PC)
Prints the count of entries by convention and form, and lists the entries
in object form; exit status 1 if there are any.
"""
import argparse
import collections
import os
import re
import sys

R = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# {"name", <cast>func, FLAGS, ...}: the function is the last identifier
# before the comma that precedes the flags.
ENTRY = re.compile(
    r'\{\s*"[^"]*"\s*,\s*([^,{}]*?)\s*,\s*((?:METH_\w+\s*\|?\s*)+)\s*[,}]')
IDENT = re.compile(r"[A-Za-z_]\w*")
CASTS = {"PyCFunction", "_PyCFunction_CAST", "void", "PyCFunctionWithKeywords",
         "PyCFunctionFast", "PyCFunctionFastWithKeywords", "PyCMethod",
         "PyCFunctionO", "PyNoArgsFunction", "getter"}


def function_of(expr):
    names = [n for n in IDENT.findall(expr) if n not in CASTS]
    return names[-1] if names else None


def sources(paths):
    for top in paths:
        for root, dirs, files in os.walk(os.path.join(R, top)):
            dirs[:] = [d for d in dirs if d not in ("build", "_decimal/libmpdec")]
            for f in files:
                if f.endswith((".c", ".h")):
                    yield os.path.join(root, f)


def definitions(text):
    """name -> (return type, parameter list) of each function defined."""
    out = {}
    for m in re.finditer(
            r"^(?:static\s+)?(?:inline\s+)?(PyObject\s*\*|PyValue)\s*\n?"
            r"\s*(\w+)\s*\(([^;{)]*(?:\([^)]*\)[^;{)]*)*)\)\s*(?:/\*.*?\*/\s*)?\{",
            text, re.M | re.S):
        out.setdefault(m.group(2), (m.group(1).replace(" ", ""), " ".join(m.group(3).split())))
    return out


def form(conv, ret, params):
    """'value' if the signature is the value form for conv, else 'object'."""
    if ret != "PyValue":
        return "object"
    ps = [p.strip() for p in params.split(",")]
    if conv == "O":
        return "value" if len(ps) == 2 and ps[1].startswith("PyValue ") else "object"
    return "value" if any(re.match(r"const PyValue \*", p) for p in ps) else "object"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-v", action="store_true")
    ap.add_argument("paths", nargs="*", default=["Objects", "Python", "Modules", "PC"])
    a = ap.parse_args()
    files = list(sources(a.paths))
    texts = {f: open(f, encoding="utf-8", errors="replace").read() for f in files}
    defs = {}
    for f, t in texts.items():
        for name, sig in definitions(t).items():
            defs.setdefault(name, []).append((f, sig))
    counts = collections.Counter()
    bad = []
    for f, t in texts.items():
        for m in ENTRY.finditer(t):
            flags = set(re.findall(r"METH_\w+", m.group(2)))
            if "METH_O" in flags:
                conv = "O"
            elif "METH_FASTCALL" in flags:
                conv = "FASTCALL"
            else:
                continue
            name = function_of(m.group(1))
            kind = "clinic" if "/clinic/" in f or f.endswith(".h.h") else "hand"
            cands = defs.get(name, [])
            # Prefer a definition in the same file, then any.
            same = [s for g, s in cands if g == f]
            sig = (same or [s for _, s in cands] or [None])[0]
            line = t.count("\n", 0, m.start()) + 1
            where = f"{os.path.relpath(f, R)}:{line} {name}"
            if sig is None:
                counts[(conv, kind, "unresolved")] += 1
                if a.v:
                    print(f"unresolved {conv} {where}")
                continue
            fm = form(conv, *sig)
            counts[(conv, kind, fm)] += 1
            if fm == "object":
                bad.append(f"{conv:8} {where}")
    for key in sorted(counts):
        print(f"{'/'.join(key):28} {counts[key]}")
    for b in bad:
        print(b)
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
