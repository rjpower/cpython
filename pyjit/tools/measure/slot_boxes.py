"""Boxes and allocations per iteration of the slot-boundary cases (note 12).

Usage: python slot_boxes.py PYSTATS_PYTHON [--sites]

Runs each case in a fresh interpreter of a Py_STATS build, warms it, then
dumps the stats after 1,000 and after 2,000 more iterations and prints the
difference per iteration: inline int and float boxes and object
allocations (over the empty loop's), with the three hottest box sites
resolved by addr2line when --sites is given.  The numbers of note 12
section 10 come from this tool at 6086cccc9c2.
"""
import collections, glob, os, re, subprocess, sys, tempfile, textwrap

PY = sys.argv[1]
WARM, N = 200, 1000
PATS = [(re.compile(r"^Object inline (int|float) boxes: (\d+)$"), "{}"),
        (re.compile(r"^Object allocations: (\d+)$"), "alloc"),
        (re.compile(r"^Object box site\[([^]]*)\]: (\d+)$"), "site {}")]

COMMON = '''
import math, functools, types
a, x = 7, 7.5
class P:
    def __init__(self): self._x = 1; self._f = 2.5
    @property
    def x(self): return self._x
    @property
    def f(self): return self._f
    @x.setter
    def x(self, v): self._x = v
class GA:
    def __getattribute__(self, n): return 5
class GH:
    def __getattr__(self, n): return 5
class SA:
    def __setattr__(self, n, v): pass
class D:
    def __get__(self, o, t): return 5
class DS:
    def __get__(self, o, t): return 5
    def __set__(self, o, v): pass
class HD:
    d = D(); ds = DS()
class CI:
    def __call__(self, v): return v
class S:
    __slots__ = ("a",)
class K:
    @classmethod
    def cm(cls, v): return v
    @staticmethod
    def sm(v): return v
    def m(self, v): return v
class Ann: pass
p, ga, gh, sa, hd, ci, s, k = P(), GA(), GH(), SA(), HD(), CI(), S(), K()
s.a = 1
z = complex(1.5, 2.5)
sl = slice(1, 2, 3)
rg = range(3, 10)
part = functools.partial(lambda u, v: v, 1)
mod = types.ModuleType("m")
NAME = "x"
import collections, datetime, os, re as _re, dataclasses
NT = collections.namedtuple("NT", "x y")
nt = NT(1, 2.5)
st = os.stat(".")
dt = datetime.date(2020, 1, 2)
mv = memoryview(b"abc")
mt = _re.match("a", "abc")
@dataclasses.dataclass
class DC:
    a: int = 1
dc = DC()
def mk(n):
    return type("Q%d" % n, (), {"x": property(lambda self: 3000)})()
objs = [mk(n) for n in range(5)]
pc = functools.partial(ci)
'''

CASES = {
    "empty": "pass",
    "property generic (getattr)": "r = getattr(p, NAME)",
    "property float (getattr)": "r = getattr(p, 'f')",
    "property site": "r = p.x",
    "property store": "p.x = i",
    "__getattribute__ site": "r = ga.y",
    "__getattribute__ getattr": "r = getattr(ga, 'y')",
    "__getattr__ site": "r = gh.y",
    "__setattr__ store": "sa.y = i",
    "__get__ descr": "r = hd.d",
    "__get__ data descr": "r = hd.ds",
    "__set__ descr": "hd.ds = i",
    "__call__ instance": "r = ci(i)",
    "member complex.real": "r = z.real",
    "member slice.start": "r = sl.start",
    "getset range.start": "r = rg.start",
    "getset int.real": "r = a.real",
    "slots member getattr": "r = getattr(s, 'a')",
    "slots member site": "r = s.a",
    "slots member store": "s.a = i",
    "method descr inline self": "r = a.bit_length()",
    "classmethod call": "r = K.cm(i)",
    "staticmethod call": "r = k.sm(i)",
    "bound method value": "m = k.m",
    "bound method call": "r = k.m(i)",
    "partial call": "r = part(i)",
    "class attr store": "Ann.v = i",
    "module attr store": "mod.v = i",
    "object.__setattr__": "object.__setattr__(p, '_x', i)",
    "setattr builtin": "setattr(p, '_x', i)",
    "namedtuple field": "r = nt.x",
    "namedtuple float field": "r = nt.y",
    "structseq field": "r = st.st_mode",
    "datetime.year": "r = dt.year",
    "memoryview.nbytes": "r = mv.nbytes",
    "match.pos": "r = mt.pos",
    "float.real": "r = x.real",
    "dataclass field": "r = dc.a",
    "property via megamorphic": "r = objs[i % 5].x",
    "__call__ via partial": "r = pc(i)",
}

def program(body):
    return ("import os, sys\n"
            "def dump():\n"
            f"    before = set(os.listdir({STATS!r}))\n"
            "    sys._stats_dump()\n"
            f"    print(*(set(os.listdir({STATS!r})) - before), flush=True)\n"
            "def run(n):\n"
            + textwrap.indent(COMMON, "    ")
            + "    for i in range(n):\n"
            + textwrap.indent(body, "        ") + "\n"
            f"run({WARM})\nsys._stats_on()\nsys._stats_clear()\n"
            f"run({N})\ndump()\nrun({2*N})\ndump()\nsys._stats_off()\n")

def read(paths):
    c = collections.Counter()
    for path in paths:
        for line in open(path):
            line = line.strip()
            for pat, key in PATS:
                m = pat.match(line)
                if m:
                    c[key.format(*m.groups()[:-1])] += int(m.groups()[-1])
                    break
        os.remove(path)
    return c

def resolve(site):
    path, _, off = site.rpartition("+")
    out = subprocess.run(["addr2line", "-f", "-i", "-e", path, off], capture_output=True, text=True).stdout.split("\n")
    fr = [out[i] for i in range(0, len(out) - 1, 2)]
    return "<-".join(fr[:4])

def measure(body, pyc):
    env = dict(os.environ, PYTHONPYCACHEPREFIX=pyc, PYTHON_STATS_DIR=STATS)
    before = set(glob.glob(f"{STATS}/*"))
    out = subprocess.run([PY, "-c", program(body)], env=env, check=True,
                         capture_output=True, text=True).stdout.split("\n")
    first, second = ([f"{STATS}/{n}" for n in l.split()] for l in out[:2])
    c = read(second); c.subtract(read(first))
    for f in set(glob.glob(f"{STATS}/*")) - before - set(first) - set(second):
        os.remove(f)
    return c

with tempfile.TemporaryDirectory() as pyc, tempfile.TemporaryDirectory() as STATS:
    base = measure("pass", pyc)["alloc"]
    for name, body in CASES.items():
        c = measure(body, pyc)
        sites = sorted(((n, k[5:]) for k, n in c.items() if k.startswith("site ") and n > 0), reverse=True)[:3]
        sites = [(n, resolve(s)) for n, s in sites]
        print(f"{name:30} int {c['int']/N:5.2f} float {c['float']/N:5.2f} alloc {(c['alloc']-base)/N:6.2f}"
              + ("  " + "; ".join(f"{n/N:g} {s}" for n, s in sites) if "--sites" in sys.argv[2:] else ""))
