"""The functions named in the initialisers of type slots, with counts.

Usage, from the repository root: python slot_inventory.py [--where] [--calls] SLOT...
for SLOT such as tp_getattro.  Positional initialisers with the slot's
comment, designated initialisers and PyType_Slot entries are counted, over
Objects/, Modules/, Python/ and PC/ (boil/design/work/6d-2.md).
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
                "tp_descr_get": "descrgetfunc", "tp_descr_set": "descrsetfunc"}
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
    for d in ("Objects", "Modules", "Python", "PC", "Include"):
        for p in sorted(pathlib.Path(d).rglob("*.[ch]")):
            func = None
            for lineno, line in enumerate(p.read_text(errors="replace").splitlines(), 1):
                m = func_start.match(line)
                if m:
                    func = m.group(1)
                    if direct.match(line):
                        continue  # the definition itself
                if any(pat[s].search(line) for pat in init_like for s in slots):
                    continue
                if line.lstrip().startswith(("#", "*", "/*", "//")):
                    continue
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
