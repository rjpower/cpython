"""The functions named in the initialisers of type slots, with counts.

Usage, from the repository root: python slot_inventory.py [--where] [--calls] SLOT...
for SLOT such as tp_getattro.  Positional initialisers with the slot's
comment, designated initialisers and PyType_Slot entries are counted, over
Objects/, Modules/, Python/ and PC/ (boil/design/values-and-object-protocol.md).
"""
import re, sys, pathlib, collections
where = "--where" in sys.argv
slots = [a for a in sys.argv[1:] if not a.startswith("--")]
pat_pos = {s: re.compile(r'^\s*(?:\((?:\w+)\)\s*)?&?(\w+)\s*,\s*/\*\s*%s\s*\*/' % s) for s in slots}
pat_des = {s: re.compile(r'\.%s\s*=\s*(?:\((?:\w+)\)\s*)?(\w+)' % s) for s in slots}
pat_spec = {s: re.compile(r'\{\s*Py_%s\s*,\s*(?:\(\w+\)\s*)?(\w+)\s*\}' % s) for s in slots}
res = {s: collections.Counter() for s in slots}
files = {s: collections.defaultdict(list) for s in slots}
for d in ("Objects", "Modules", "Python", "PC"):
    for p in pathlib.Path(d).rglob("*.c"):
        for lineno, line in enumerate(p.read_text(errors="replace").splitlines(), 1):
            for s in slots:
                for pat in (pat_pos[s], pat_des[s], pat_spec[s]):
                    m = pat.search(line)
                    if m and m.group(1) not in ("0", "NULL"):
                        res[s][m.group(1)] += 1
                        files[s][m.group(1)].append(f"{p}:{lineno}")
for s in slots:
    c = res[s]
    print(f"== {s}: {sum(c.values())} entries, {len(c)} functions")
    print("  " + "; ".join(f"{k} {v}" for k, v in c.most_common()))
    if where:
        for k, _ in c.most_common():
            if not k.startswith("PyObject_Generic"):
                print(f"    {k}: {' '.join(files[s][k])}")

# --calls: every place that dispatches through one of the slots, or calls a
# function an initialiser names, outside initialisers, with the enclosing
# function.  These are where a slot's result enters the caller (the debug
# check's placement, 6d-2.md F).
if "--calls" in sys.argv:
    typedefs = {"tp_getattro": "getattrofunc", "tp_setattro": "setattrofunc",
                "tp_descr_get": "descrgetfunc", "tp_descr_set": "descrsetfunc",
                "tp_call": "callfunc"}
    names = set()
    for s in slots:
        names |= set(res[s])
    names |= {"_PyObject_GenericGetAttrWithDict", "_PyObject_GenericSetAttrWithDict",
              "_Py_type_getattro_impl", "_Py_module_getattro_impl"}
    field = re.compile(r'\b(%s)\b(?!\s*=)' % "|".join(slots))
    local = re.compile(r'\b(%s)\s+\**\s*(\w+)' % "|".join(typedefs[s] for s in slots))
    direct = re.compile(r'(?<![\w.>])(%s)\s*\(' % "|".join(sorted(names, key=len, reverse=True)))
    func_start = re.compile(r'^(\w+)\s*\(')
    init_like = [pat_pos, pat_des, pat_spec]
    def code_of(text):
        # The text with comments and string literals blanked, line for line,
        # so that a line opening with "*" is code unless a comment holds it.
        out, i, n = [], 0, len(text)
        while i < n:
            c = text[i]
            if text.startswith("/*", i):
                j = text.find("*/", i + 2)
                j = n if j < 0 else j + 2
                out.append("".join(ch if ch == "\n" else " " for ch in text[i:j]))
                i = j
            elif text.startswith("//", i):
                j = text.find("\n", i)
                j = n if j < 0 else j
                out.append(" " * (j - i))
                i = j
            elif c in "\"'":
                j = i + 1
                while j < n and text[j] != c and text[j] != "\n":
                    j += 2 if text[j] == "\\" else 1
                j = min(j + 1, n)
                out.append(c + " " * (j - i - 2) + c if j - i >= 2 else c)
                i = j
            else:
                out.append(c)
                i += 1
        return "".join(out)

    for d in ("Objects", "Modules", "Python", "PC", "Include"):
        for p in sorted(pathlib.Path(d).rglob("*.[ch]")):
            func = None
            text = p.read_text(errors="replace")
            for lineno, (line, code) in enumerate(
                    zip(text.splitlines(), code_of(text).splitlines()), 1):
                m = func_start.match(code)
                if m:
                    func = m.group(1)
                    if direct.match(code):
                        continue  # the definition itself
                if any(pat[s].search(line) for pat in init_like for s in slots):
                    continue
                if code.lstrip().startswith("#"):
                    continue
                line = code
                kinds = []
                if field.search(line):
                    kinds.append("field")
                if local.search(line):
                    kinds.append("local")
                m = direct.search(line)
                if m:
                    kinds.append("call " + m.group(1))
                if kinds:
                    print(f"{p}:{lineno}: {func}: {', '.join(kinds)}: {line.strip()}")
