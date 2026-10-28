"""Inventory of getset entries: every initialiser of a PyGetSetDef array
under Objects/, Modules/, Python/, PC/, and Argument Clinic's GETSETDEF
macros.  Run from the repository root."""
import re, pathlib, collections, sys
DIRS = ("Objects", "Modules", "Python", "PC")
arr = re.compile(r'PyGetSetDef\s+(\w+)\s*\[\s*\w*\s*\]\s*=\s*\{(.*?)\n\s*\};', re.S)
macro_def = re.compile(r'#define\s+(\w+_GETSETDEF)\s*\\?\s*\n?\s*\{\s*"([^"]+)"\s*,\s*\(getter\)\s*(\w+)\s*,\s*\(setter\)\s*(\w+)', re.S)
entries = []        # (file, array, name, getter, setter, via)
macros = {}         # macro -> (file, name, getter, setter)
for d in DIRS:
    for p in sorted(pathlib.Path(d).rglob("*")):
        if p.suffix not in (".c", ".h"):
            continue
        t = p.read_text(errors="replace")
        for m in macro_def.finditer(t):
            macros[m.group(1)] = (str(p), m.group(2), m.group(3), m.group(4))
for d in DIRS:
    for p in sorted(pathlib.Path(d).rglob("*")):
        if p.suffix not in (".c", ".h") or "/clinic/" in str(p):
            continue
        t = p.read_text(errors="replace")
        for m in arr.finditer(t):
            body = re.sub(r'/\*.*?\*/', '', m.group(2), flags=re.S)
            body = re.sub(r'//[^\n]*', '', body)
            depth = 0; cur = ''; items = []
            for ch in body:
                if ch == '{':
                    depth += 1
                    if depth == 1:
                        cur = ''; continue
                if ch == '}':
                    depth -= 1
                    if depth == 0:
                        items.append(cur.strip()); continue
                if depth >= 1:
                    cur += ch
            # macro entries (Clinic's and others) sit at depth 0
            for mm in re.finditer(r'\b(\w+_GETSETDEF)\b', body):
                f, name, g, s = macros.get(mm.group(1), (None, mm.group(1), '?', '?'))
                entries.append((str(p), m.group(1), name, g, s, "clinic"))
            for it in items:
                parts = [x.strip() for x in re.split(r',(?![^(]*\))', it)]
                if not parts or parts[0] in ('NULL', '0', ''):
                    continue
                name = parts[0].strip('"')
                g = parts[1] if len(parts) > 1 else 'NULL'
                s = parts[2] if len(parts) > 2 else 'NULL'
                if g.startswith('.get'):
                    g = g.split('=', 1)[1].strip()
                if s.startswith('.set'):
                    s = s.split('=', 1)[1].strip()
                g = re.sub(r'^\(\w+\)\s*', '', g); s = re.sub(r'^\(\w+\)\s*', '', s)
                g = re.sub(r'_Py_FUNC_CAST\(\w+,\s*(\w+)\)', r'\1', g)
                s = re.sub(r'_Py_FUNC_CAST\(\w+,\s*(\w+)\)', r'\1', s)
                entries.append((str(p), m.group(1), name, g, s, "table"))
by_via = collections.Counter(e[5] for e in entries)
getters = collections.Counter(e[3] for e in entries if e[3] not in ('NULL', '0'))
setters = collections.Counter(e[4] for e in entries if e[4] not in ('NULL', '0', ''))
files = collections.Counter(e[0] for e in entries)
print(f"entries {len(entries)} ({dict(by_via)}); getter functions {len(getters)}, "
      f"setter functions {len(setters)}; entries with a setter "
      f"{sum(1 for e in entries if e[4] not in ('NULL','0',''))}; files {len(files)}")
print("clinic GETSETDEF macros defined:", len(macros))
generic = [e for e in entries if e[3].startswith("PyObject_GenericGetDict")]
print("PyObject_GenericGetDict entries:", len(generic))
if "--files" in sys.argv:
    for f, n in sorted(files.items()):
        print(f"  {n:3} {f}")
if "--entries" in sys.argv:
    for e in entries:
        print("\t".join(e))
