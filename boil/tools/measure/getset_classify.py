"""Classify the getset functions of getset_inventory.py's entries by what
their returns construct.  Run from the repository root."""
import re, subprocess, collections, pathlib, sys
rows = subprocess.run([sys.executable, str(pathlib.Path(__file__).with_name("getset_inventory.py")), "--entries"],
                      capture_output=True, text=True).stdout.splitlines()[3:]
NUM = re.compile(r'^(PyLong_From\w+|PyFloat_FromDouble|PyBool_FromLong|_PyLong_Get(Zero|One)|PyLong_FromSsize_t)\(')
src = {}
def body(fn, files):
    for f in files:
        t = src.setdefault(f, pathlib.Path(f).read_text(errors="replace"))
        m = re.search(r'\n(?:static\s+)?(?:inline\s+)?\w[\w\s\*]*?\b%s\s*\([^;{]*\)\s*\{' % re.escape(fn), t)
        if m:
            i = m.end(); depth = 1
            while depth and i < len(t):
                depth += {'{': 1, '}': -1}.get(t[i], 0); i += 1
            return t[m.end():i]
    return None
getters = collections.defaultdict(set); setters = collections.defaultdict(set)
for r in rows:
    parts = r.split("\t")
    if len(parts) != 6:
        continue
    f, arr, name, g, s, via = parts
    if via == "clinic":
        continue
    if g not in ('NULL', '0', '?'): getters[g].add(f)
    if s not in ('NULL', '0', '', '?'): setters[s].add(f)
kinds = collections.Counter(); examples = collections.defaultdict(list)
for g, fs in getters.items():
    b = body(g, sorted(fs))
    if b is None:
        kinds["not found (macro or other file)"] += 1; examples["nf"].append(g); continue
    rets = [x.strip() for x in re.findall(r'\breturn\s+([^;]*);', b)]
    rets += ["Py_RETURN"] * len(re.findall(r'\bPy_RETURN_(NONE|TRUE|FALSE|NOTIMPLEMENTED)\b', b))
    rets = [x for x in rets if x not in ('NULL',)]
    nums = [x for x in rets if NUM.match(x)]
    if rets and len(nums) == len(rets):
        kinds["number only"] += 1; examples["num"].append(g)
    elif nums:
        kinds["number and other"] += 1
    else:
        kinds["object"] += 1
print("hand-written getter functions:", len(getters), dict(kinds))
print("hand-written setter functions:", len(setters))
if "-v" in sys.argv:
    print("number only:", " ".join(sorted(examples["num"])))
    print("not found:", " ".join(sorted(examples["nf"])))
