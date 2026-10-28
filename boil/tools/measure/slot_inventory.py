"""The functions named in the initialisers of type slots, with counts.

Usage, from the repository root: python slot_inventory.py [--where] SLOT...
for SLOT such as tp_getattro.  Positional initialisers with the slot's
comment, designated initialisers and PyType_Slot entries are counted, over
Objects/, Modules/, Python/ and PC/ (boil/design/work/6d-2.md).
"""
import re, sys, pathlib, collections
where = "--where" in sys.argv
slots = [a for a in sys.argv[1:] if a != "--where"]
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
