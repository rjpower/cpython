"""Box audit of the generic and specialised handlers on inline operands.

Each case runs a loop body over inline ints and floats under a Py_STATS
build: once to warm (specialise) it, then N and 2N times with stats on;
the difference is the boxes of N iterations, without those of the loop's
setup.  A case passes when its iterations make no box of an inline int or
float; a case listed as residue reports its boxes without failing.

The slot cases (6d rung 2, the attribute slots) also count object
allocations per iteration, over the empty loop's, and fail over their
budget; a slot case with a residue reports both without failing.

Every case runs in a process of its own, with the stats dumped to a
directory of the run's own (PYTHON_STATS_DIR).  The run prints the
commit of the interpreter's tree.  -v resolves the boxing call sites with
addr2line.

Usage: box_audit.py PYSTATS_PYTHON [-k SUBSTRING] [-v]
Exit status 1 if a case that should not box does, or allocates over its
budget.
"""
import argparse, collections, glob, os, re, subprocess, sys, tempfile, textwrap

STATS = None    # the run's stats directory
ROOT = None     # the interpreter's source tree
WARM, N = 200, 1000
LINE = re.compile(r"^Object inline (int|float) boxes: (\d+)$")
ALLOC = re.compile(r"^Object allocations: (\d+)$")
SITE = re.compile(r"^Object box site\[([^]]*)\]: (\d+)$")
OPCODE = re.compile(r"^opcode\[(\w+)\]\.boxes : (\d+)$")

# name: (setup, body, residue).  The body runs in a loop over i in
# range(n), with these locals bound by the setup.  A residue is the reason
# the case is allowed to box.  The bodies avoid calls into C, whose
# arguments are boxed until METH_FASTCALL takes values.
COMMON = """
a, b, x, y = 7, 3, 7.5, 2.5
lst = [1, 2.5, 3, 4]
rng = range(10)
tup = (1, 2.5, 3, 4)
d = {1: 2, 2.5: 3, 3: 4}
s = {1, 2.5, 3}
text = "abcdef"
class C:
    pass
class S:
    __slots__ = ("v",)
obj, sobj = C(), S()
obj.v = 1
sobj.v = 1
def gen():
    yield from tup
    return 2.5
"""

CASES = {
    # BINARY_OP: every operator on int/int, float/float and the mixes
    "binary int": ("", "r = (a + b, a - b, a * b, a // b, a % b, a / b, a ** b,"
                       " a << b, a >> b, a & b, a | b, a ^ b, -a ** 2)", None),
    "binary float": ("", "r = (x + y, x - y, x * y, x // y, x % y, x / y,"
                         " x ** y)", None),
    "binary mixed": ("", "r = (a + y, x - b, a * y, x // b, a % y, a / y,"
                         " x ** b, b ** x)", None),
    "binary overflow": ("", "r = (2147483647 + b, a * 1000000000)", None),
    "inplace": ("", "t = a; t += b; t -= y; t *= b; t /= y; t //= b; "
                    "u = a; u <<= b; u >>= b; u &= b; u |= b; u ^= b; "
                    "u %= b; u **= 2", None),
    "subscr list/tuple/str": ("", "r = (lst[1], lst[-1], tup[2], text[a % 6])",
                              None),
    "subscr dict": ("", "r = (d[1], d[2.5], d[3])", None),
    "subscr slice": ("", "r = (lst[1:3], tup[:b], text[1:a:2], lst[::-1])",
                     None),
    "store subscr": ("", "lst[0] = a; lst[1] = y; d[a] = y; d[y] = a", None),
    "delete subscr": ("", "d[100] = a; del d[100]; lst += [y]; del lst[-1]",
                      None),
    "compare": ("", "r = (a < b, a == y, x > y, x <= b, a != 3, y >= 2.5)",
                None),
    "contains": ("", "r = (a in lst, y in lst, 3 in tup, y in d, a in s,"
                     " 2.5 in s, a in rng)", None),
    "is": ("", "r = (a is b, x is y, a is not None)", None),
    "unary": ("", "r = (-a, -x, ~a, not a, not x, +a, +x)", None),
    "to_bool": ("", "if a: pass\nif x: pass\nif not b: pass", None),
    "for iter": ("", "for v in lst: pass\nfor v in tup: pass\n"
                     "for v in rng: pass\nfor v in d: pass\n"
                     "for v in s: pass", None),
    "attributes": ("", "obj.v = a; obj.w = y; sobj.v = x; "
                       "r = obj.v + obj.w + sobj.v", None),
    "build": ("", "r = ([a, y], (a, y), {a, y}, {a: y}, {y: a}, "
                  "lst[a:b:1], [*lst], (*tup,), {**d})", None),
    "unpack": ("", "p, q = a, y; p, q, *rest = lst; p, q = [a, y]", None),
    "comprehensions": ("", "r = ([v * 2 for v in tup], {v for v in tup},"
                           " {v: v for v in tup})", None),
    "generator": ("", "for v in (w for w in tup): pass\n"
                      "for v in gen(): pass", None),
    "format": ("", "r = (f'{a}', f'{x}', f'{a!r}', f'{x:.2f}', f'{y:>8}',"
                   " f'{y!s}')", None),
    "format int spec": ("", "r = f'{a:5d}'",
                        "an inline int with a format spec is boxed: the int "
                        "formatter does not split from the object (ruling d)"),
    "names": ("", "exec(code, ns)", None),
}

# The attribute slots (6d rung 2): name: (body, boxes and allocations per
# iteration, residue).  The bodies run with SLOT_SETUP's locals; i is an inline int,
# and the setup must not bind n, the loop's count.
SLOT_SETUP = """
import datetime, functools, hashlib, io, operator, pickle, socket, threading
import types, weakref
big = 100000
class P:
    def __init__(self): self._x = 1; self._f = 2.5
    @property
    def x(self): return self._x
    @property
    def f(self): return self._f
    @x.setter
    def x(self, v): self._x = v
class GA:
    def __getattribute__(self, name): return 5
class GH:
    def __getattr__(self, name): return 5
class SA:
    def __setattr__(self, name, v): pass
class D:
    def __get__(self, o, t): return 5
class DS:
    def __get__(self, o, t): return 5
    def __set__(self, o, v): pass
class HD:
    d = D(); ds = DS()
class S:
    __slots__ = ("a",)
class Ann:
    pass
class W:
    pass
class G:
    x = 12345
    def __class_getitem__(cls, item): return types.GenericAlias(cls, item)
class Pk(pickle.Pickler):
    pass
p, ga, gh, sa, hd, s, w = P(), GA(), GH(), SA(), HD(), S(), W()
s.a = 1
w.x = 12345
proxy = weakref.proxy(w)
local = threading.local()
local.x = 12345
mod = types.ModuleType("m")
mod.__getattr__ = lambda name: 12345
alias = G[int]
pk = Pk(io.BytesIO())
NAME = "x"
class Call:
    def __call__(self, v): return v + big
callme = Call()
def add(u, v): return u + v
part = functools.partial(add, big)
partf = functools.partial(add, 2.5)
ig, ag = operator.itemgetter(0), operator.attrgetter("x")
agf = operator.attrgetter("f")
c0 = functools.lru_cache(maxsize=0)(callme)
cn = functools.lru_cache(maxsize=None)(callme)
c128 = functools.lru_cache(maxsize=128)(callme)
aint = types.GenericAlias(int, (int,))
mv = memoryview(bytearray(big))
tm = datetime.time(microsecond=999999)
dt = datetime.date(2020, 1, 2)
nan = float("nan")
def cells():
    a = big + 1; b = 2.5 * big
    return (lambda: (a, b)).__closure__
ci, cf = cells()
sha = hashlib.sha256()
class Raw(io.RawIOBase):
    name = big + 1
    def readable(self): return True
br = io.BufferedReader(Raw())
tw = io.TextIOWrapper(br, encoding="ascii")
sk = socket.socket()
sk.settimeout(2.5 * big)
oe = OSError()
def fn(): pass
class Dict: pass
dobj = Dict()
"""

SLOT_CASES = {
    "property (getattr)": ("r = getattr(p, NAME)", 0, 0, None),
    "property float (getattr)": ("r = getattr(p, 'f')", 0, 0, None),
    "property store": ("p.x = i", 0, 0, None),
    "__getattribute__ hook": ("r = ga.y", 0, 0, None),
    "__getattr__ hook": ("r = gh.y", 0, 0, None),
    "__setattr__ hook": ("sa.y = i", 0, 0, None),
    "__get__ descr": ("r = hd.d", 0, 0, None),
    "__set__ descr": ("hd.ds = i", 0, 0, None),
    "getattr default": ("r = getattr(p, 'missing', i)", 0, 0, None),
    "hasattr": ("r = hasattr(p, 'x')", 0, 0, None),
    # A suppressed miss on an inline owner makes no error and no box.
    "hasattr inline miss": ("r = hasattr(i, 'missing')", 0, 0, None),
    "getattr inline miss": ("r = getattr(i, 'missing', None)", 0, 0, None),
    "slots member store": ("s.a = i", 0, 0, None),
    "class attr store": ("Ann.v = i", 0, 0, None),
    "proxy read": ("r = proxy.x", 0, 0, None),
    "proxy store": ("proxy.y = i", 0, 0, None),
    "local read": ("r = local.x", 0, 0, None),
    # The generic lookup raises its AttributeError and clears it before
    # the module __getattr__ runs, as upstream: three objects, no box.
    "module __getattr__": ("r = mod.missing", 0, 3, None),
    "alias read": ("r = alias.x", 0, 0, None),
    "pickler store": ("pk.attr = i", 0, 0, None),
    # The binding box of the inline owner, and the bound method.
    "getattr inline method": ("r = getattr(big, 'bit_length')", 1, 2, None),
    # persistent_id is a PyObject * field, which keeps a box.
    "pickler persistent_id": ("pk.persistent_id = big", 1, 1, None),
    "getset int.real": (
        "r = getattr(big, 'real')", None, None,
        "a getset of an inline owner boxes the owner, since self is an object "
        "(note 12 open question 1)"),
    # The argument tuple and the sliced tuple of the wrapper call.
    "object.__getattribute__": ("r = object.__getattribute__(p, NAME)",
                                0, 2, None),
    "super inline receiver": (
        "r = super(int, big).__repr__", None, None,
        "the method-wrapper keeps the receiver in its self field, so its box "
        "is the minimum; no step owns it"),
    # The call results (6d rung 3): tp_call returns a value.  A Python
    # class has no vectorcall, so _PyObject_MakeTpCall() builds the
    # argument tuple: one object.
    "call instance": ("r = callme(i)", 0, 1, None),
    "call instance float": ("r = callme(i + 0.5)", 0, 1, None),
    "starred call": ("r = callme(*(i,))", 0, 1, None),
    # Two boxes: the receiver, since a wrapper's self is an object (the
    # minimum), and the operand, since nb_add takes objects (the number
    # slots' step).  The tuples of the wrapper call are the rest.
    "int.__add__": ("r = int.__add__(big, i)", 2, 5, None),
    # The wrapper's two tuples and partial's merged arguments.
    "partial (tp_call)": ("r = type(part).__call__(part, i)", 0, 3, None),
    "itemgetter (tp_call)": ("r = type(ig).__call__(ig, (i, 2.5))", 0, 3,
                             None),
    "attrgetter (tp_call)": ("r = type(ag).__call__(ag, p)", 0, 2, None),
    # The float results through the same paths: a float made and then
    # canonicalised would count an allocation.
    "partial float (tp_call)": ("r = type(partf).__call__(partf, i)", 0, 3,
                                None),
    "itemgetter float (tp_call)": (
        "r = type(ig).__call__(ig, (i + 0.5, 2.5))", 0, 3, None),
    "attrgetter float (tp_call)": ("r = type(agf).__call__(agf, p)", 0, 2,
                                   None),
    # lru_cache has no vectorcall: the argument tuple, and a dict entry on
    # an unbounded miss.  The bounded cache's link keeps the key and the
    # result as objects: one box of each on a miss, and the link.
    "lru_cache 0": ("r = c0(i)", 0, 1, None),
    "lru_cache None hit": ("r = cn(1)", 0, 1, None),
    "lru_cache None miss": ("cn.cache_clear(); r = cn(i)", 0, 2, None),
    "lru_cache 128 hit": ("r = c128(1)", 0, 1, None),
    "lru_cache 128 miss": ("c128.cache_clear(); r = c128(i)", 2, 5, None),
    "lru_cache 0 float": ("r = c0(i + 0.5)", 0, 1, None),
    "lru_cache None hit float": ("r = cn(1.5)", 0, 1, None),
    "lru_cache None miss float": ("cn.cache_clear(); r = cn(i + 0.5)", 0, 2,
                                  None),
    "lru_cache 128 hit float": ("r = c128(1.5)", 0, 1, None),
    # The key of a float argument is the argument tuple itself
    # (lru_cache_make_key), an object, so the link boxes the result only.
    "lru_cache 128 miss float": ("c128.cache_clear(); r = c128(i + 0.5)", 1,
                                 4, None),
    "generic alias of int": ("r = aint(i)", 0, 0, None),
    # The getters (6d rung 4) return values.  Discriminating: the number is
    # not a cached small int, or the owner is a heap float.
    "getset memoryview.nbytes": ("r = mv.nbytes", 0, 0, None),
    "getset time.microsecond": ("r = tm.microsecond", 0, 0, None),
    "getset NaN.imag": ("r = nan.imag", 0, 0, None),
    "cell_contents int": ("r = ci.cell_contents", 0, 0, None),
    "cell_contents float": ("r = cf.cell_contents", 0, 0, None),
    # The forwarding getters: an attribute of the raw stream or buffer,
    # and the socket's timeout.
    "BufferedReader.name": ("r = br.name", 0, 0, None),
    "TextIOWrapper.name": ("r = tw.name", 0, 0, None),
    "socket.timeout": ("r = sk.timeout", 0, 0, None),
    # Controls: the number is a cached small int.  The hash object's
    # module (_hashlib or _sha2) is printed with the run.
    "getset memoryview.ndim": ("r = mv.ndim", 0, 0, None),
    "getset date.day": ("r = dt.day", 0, 0, None),
    "getset sha256.digest_size": ("r = sha.digest_size", 0, 0, None),
    # The setters (6d rung 4) take values.  A cell stores the value; a
    # number setter reads it with a value form.
    "cell_contents store int": ("ci.cell_contents = i", 0, 0, None),
    "cell_contents store float": ("cf.cell_contents = i + 0.5", 0, 0, None),
    "OSError.characters_written store": ("oe.characters_written = i", 0, 0,
                                         None),
    "TextIOWrapper._CHUNK_SIZE store": ("tw._CHUNK_SIZE = i + 1", 0, 0,
                                        None),
    # A setter that stores into a PyObject * field keeps a box.
    "function.__doc__ store": ("fn.__doc__ = i", 1, 1, None),
    # __dict__ refuses an inline value by its type, without a box; the
    # four allocations are the error's.
    "__dict__ store refused": (
        "try: dobj.__dict__ = i\nexcept TypeError: pass", 0, 4, None),
    "starred inline callee": (
        "try: big(*(i,))\nexcept TypeError: pass", None, None,
        "the generator boxes the callee at the entry of _MAKE_CALLARGS_A_TUPLE "
        "(_Py_Check_ArgsIterable's unused parameter, the ceval.c hand-off) "
        "and of _DO_CALL_FUNCTION_EX, which convert it on other paths; the "
        "callee always raises (6d-3.md, as built)"),
}

# Module-level code for LOAD_NAME, STORE_NAME and DELETE_NAME.
NAMES_SETUP = """
code = compile("p = a\\nq = p + 1\\nz = 2.5\\nw = z * 2\\ndel p", "<names>", "exec")
ns = {"a": 7}
"""


def program(name):
    if name in SLOT_CASES or name == "empty":
        setup = SLOT_SETUP
        body = SLOT_CASES[name][0] if name != "empty" else "pass"
    else:
        setup, body, _ = CASES[name]
    if name == "names":
        setup = NAMES_SETUP
    return ("import os, sys\n"
            "def dump():\n"
            f"    before = set(os.listdir({STATS!r}))\n"
            "    sys._stats_dump()\n"
            f"    print(*(set(os.listdir({STATS!r})) - before), flush=True)\n"
            "def run(n):\n"
            + textwrap.indent(COMMON + setup, "    ")
            + "    for i in range(n):\n"
            + textwrap.indent(body, "        ") + "\n"
            f"run({WARM})\n"
            "sys._stats_on()\n"
            "sys._stats_clear()\n"
            f"run({N})\n"
            "dump()\n"
            f"run({2 * N})\n"
            "dump()\n"
            "sys._stats_off()\n")


def read_stats(paths):
    counts = collections.Counter()
    for path in paths:
        with open(path) as f:
            for line in f:
                line = line.strip()
                for pattern, key in ((LINE, "{}"), (ALLOC, "alloc"),
                                     (OPCODE, "op {}"), (SITE, "site {}")):
                    m = pattern.match(line)
                    if m:
                        counts[key.format(*m.groups()[:-1])] += int(
                            m.group(m.lastindex))
                        break
        os.remove(path)
    return counts


def handler_at(line):
    """The instruction whose generated code holds line of
    generated_ir_cases.c.h."""
    with open(f"{ROOT}/Python/generated_ir_cases.c.h") as f:
        lines = f.readlines()[:line]
    for text in reversed(lines):
        m = re.search(r"TARGET\((\w+)\)", text)
        if m:
            return m.group(1)
    return "?"


def resolve(site):
    """The inlined call chain at site, "path+offset", innermost first."""
    path, _, off = site.rpartition("+")
    out = subprocess.run(["addr2line", "-f", "-i", "-C", "-e", path, off],
                         capture_output=True, text=True).stdout.split("\n")
    frames = []
    for i in range(0, len(out) - 1, 2):
        where = os.path.basename(out[i + 1]).split(" ")[0]
        m = re.match(r"generated_ir_cases.c.h:(\d+)", where)
        if m:
            where += f" {handler_at(int(m.group(1)))}"
        frames.append(f"{out[i]} ({where})")
    return " <- ".join(frames[:3])


def new_files(before):
    return set(glob.glob(f"{STATS}/*.txt")) - before


def measure(python, name, pyc):
    """The counts of N iterations of the case: those of 2N less those of N."""
    env = dict(os.environ, PYTHONPYCACHEPREFIX=pyc, PYTHON_STATS_DIR=STATS)
    before = set(glob.glob(f"{STATS}/*.txt"))
    out = subprocess.run([python, "-c", program(name)], env=env, check=True,
                         capture_output=True, text=True).stdout.split("\n")
    first, second = ([f"{STATS}/{n}" for n in line.split()] for line in out[:2])
    counts = read_stats(second)
    counts.subtract(read_stats(first))
    # The dump at exit.
    read_stats(new_files(before) - set(first) - set(second))
    return counts


def report_sites(counts):
    sites = sorted(((n, k[5:]) for k, n in counts.items()
                    if k.startswith("site ") and n > 0), reverse=True)
    for n, site in sites[:6]:
        print(f"         {n / N:g}/iter {resolve(site)}")


def audit_slots(python, a, pyc, failed):
    base = measure(python, "empty", pyc)["alloc"]
    for name, (body, box_budget, budget, residue) in SLOT_CASES.items():
        if a.k not in name:
            continue
        counts = measure(python, name, pyc)
        boxes = counts["int"] + counts["float"]
        allocs = (counts["alloc"] - base) / N
        if residue:
            state = "residue"
        elif (abs(boxes / N - box_budget) >= 0.01
              or allocs > budget + 0.01):
            state = "BOXES"
            failed.append(name)
        else:
            state = "ok"
        print(f"{state:8} {name:24} boxes {boxes / N:g}/iter, "
              f"allocations {allocs:g}/iter"
              + (f" (budget {box_budget} and {budget})"
                 if budget is not None else ""))
        if residue and a.v:
            print(f"         residue: {residue}")
        if a.v and boxes:
            report_sites(counts)


def main():
    global STATS, ROOT
    p = argparse.ArgumentParser()
    p.add_argument("python", help="a Py_STATS build's interpreter")
    p.add_argument("-k", default="")
    p.add_argument("-v", action="store_true")
    a = p.parse_args()
    python = os.path.abspath(a.python)
    ROOT = subprocess.run(["git", "-C", os.path.dirname(python), "rev-parse",
                           "--show-toplevel"], check=True, capture_output=True,
                          text=True).stdout.strip()
    sha = subprocess.run(["git", "-C", ROOT, "rev-parse", "--short=11",
                          "HEAD"], check=True, capture_output=True,
                         text=True).stdout.strip()
    # The binary's own build line names the commit it was built from (and
    # "-dirty" if the source was); the checkout's HEAD alone does not.
    built = subprocess.run([python, "-c", "import sys; print(sys.version)"],
                           check=True, capture_output=True,
                           text=True).stdout.strip()
    dirty = subprocess.run(["git", "-C", ROOT, "status", "--porcelain",
                            "--untracked-files=no"], check=True,
                           capture_output=True, text=True).stdout.strip()
    print(f"box audit of {python} at {sha}{' (dirty)' if dirty else ''}")
    print(f"built: {built}")
    hashmod = subprocess.run(
        [python, "-c", "import hashlib; "
         "print(type(hashlib.sha256()).__module__)"],
        check=True, capture_output=True, text=True).stdout.strip()
    print(f"sha256 from {hashmod}")
    failed = []
    with tempfile.TemporaryDirectory(prefix="box_audit-") as pyc, \
         tempfile.TemporaryDirectory(prefix="box_audit-stats-") as stats:
        STATS = stats
        for name, (_, _, residue) in CASES.items():
            if a.k not in name:
                continue
            counts = measure(python, name, pyc)
            boxes = {k: counts[k] for k in ("int", "float")}
            by_op = {k[3:]: n for k, n in counts.items()
                     if k.startswith("op ") and n}
            total = boxes["int"] + boxes["float"]
            # A box that a handler makes each iteration counts at least 1;
            # less is a one-off, such as a specialisation.
            if abs(total) < N / 100:
                state = "ok"
            elif residue:
                state = "residue"
            else:
                state = "BOXES"
                failed.append(name)
            ops = ", ".join(f"{op} {n / N:g}" for op, n in
                            sorted(by_op.items(), key=lambda kv: -kv[1]))
            print(f"{state:8} {name:24} int {boxes['int'] / N:g}/iter, "
                  f"float {boxes['float'] / N:g}/iter"
                  + (f"  [{ops}]" if ops else ""))
            if residue and a.v:
                print(f"         residue: {residue}")
            if a.v and total:
                report_sites(counts)
        audit_slots(python, a, pyc, failed)
    if failed:
        print(f"{len(failed)} case(s) box: {', '.join(failed)}")
        sys.exit(1)


if __name__ == "__main__":
    main()
