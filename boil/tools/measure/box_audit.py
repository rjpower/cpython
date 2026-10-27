"""Box audit of the generic and specialised handlers on inline operands.

Each case runs a loop body over inline ints and floats under a Py_STATS
build: once to warm (specialise) it, then N and 2N times with stats on;
the difference is the boxes of N iterations, without those of the loop's
setup.  A case passes when its iterations make no box of an inline int or
float; a case listed as residue reports its boxes without failing.  Every
case runs in a process of its own, since the stats are dumped to
/tmp/py_stats.  -v resolves the boxing call sites with addr2line.

Usage: box_audit.py [--build dev-pystats] [-k SUBSTRING] [-v]
Exit status 1 if a case that should not box does.
"""
import argparse, collections, glob, os, re, subprocess, sys, tempfile, textwrap

R = "/home/power/code/cpython/.worktrees/python-boil-the-ocean"
STATS = "/tmp/py_stats"
WARM, N = 200, 1000
LINE = re.compile(r"^Object inline (int|float) boxes: (\d+)$")
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
    "generator": ("", "for v in (w * 2 for w in tup): pass",
                  "FOR_ITER_GEN leaves to FOR_ITER when the loop target is a "
                  "local, so the generator resumes from C and its yields are "
                  "boxed at INTERPRETER_EXIT (being fixed in 6b)"),
    "format": ("", "r = (f'{a}', f'{x}', f'{a!r}', f'{x:.2f}', f'{y:>8}',"
                   " f'{y!s}')", None),
    "format int spec": ("", "r = f'{a:5d}'",
                        "an inline int with a format spec is boxed: the int "
                        "formatter does not split from the object (ruling d)"),
    "names": ("", "exec(code, ns)", None),
}

# Module-level code for LOAD_NAME, STORE_NAME and DELETE_NAME.
NAMES_SETUP = """
code = compile("p = a\\nq = p + 1\\nz = 2.5\\nw = z * 2\\ndel p", "<names>", "exec")
ns = {"a": 7}
"""


def program(name):
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
                for pattern, key in ((LINE, "{}"), (OPCODE, "op {}"),
                                     (SITE, "site {}")):
                    m = pattern.match(line)
                    if m:
                        counts[key.format(m.group(1))] += int(m.group(2))
                        break
        os.remove(path)
    return counts


def handler_at(line):
    """The instruction whose generated code holds line of
    generated_ir_cases.c.h."""
    with open(f"{R}/Python/generated_ir_cases.c.h") as f:
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
    env = dict(os.environ, PYTHONPYCACHEPREFIX=pyc)
    before = set(glob.glob(f"{STATS}/*.txt"))
    out = subprocess.run([python, "-c", program(name)], env=env, check=True,
                         capture_output=True, text=True).stdout.split("\n")
    first, second = ([f"{STATS}/{n}" for n in line.split()] for line in out[:2])
    counts = read_stats(second)
    counts.subtract(read_stats(first))
    # The dump at exit.
    read_stats(new_files(before) - set(first) - set(second))
    return counts


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--build", default="dev-pystats")
    p.add_argument("-k", default="")
    p.add_argument("-v", action="store_true")
    a = p.parse_args()
    python = f"{R}/build/{a.build}/python"
    os.makedirs(STATS, exist_ok=True)
    failed = []
    with tempfile.TemporaryDirectory(prefix="box_audit-") as pyc:
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
                sites = sorted(((n, k[5:]) for k, n in counts.items()
                                if k.startswith("site ") and n > 0),
                               reverse=True)
                for n, site in sites[:6]:
                    print(f"         {n / N:g}/iter {resolve(site)}")
    if failed:
        print(f"{len(failed)} case(s) box: {', '.join(failed)}")
        sys.exit(1)


if __name__ == "__main__":
    main()
