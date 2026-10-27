"""Boxes of inline values per loop of one pyperformance worker under Py_STATS:
runs at 2 and 10 loops, differenced.  Prints the int and float boxes, the
boxes by class and, with --sites N, the top N boxing call sites resolved with
addr2line.  Usage: boxes.py [--sites N] BUILD BENCH..."""
import argparse, collections, glob, os, re, shutil, subprocess

R = "/home/power/code/cpython/.worktrees/python-boil-the-ocean"
SP = glob.glob(f"{R}/build/pyperf-venvs/default/venv/*/lib/python3.16/site-packages")[0]
STATS = "/tmp/py_stats"
LINE = re.compile(r"^Object (inline int boxes|inline float boxes|boxes \(([^)]*)\)|box site\[([^]]*)\]): (\d+)$")


def run(build, bench, loops, pyc):
    shutil.rmtree(STATS, ignore_errors=True)
    os.makedirs(STATS)
    env = dict(os.environ, PYTHONPYCACHEPREFIX=pyc, PYTHONPATH=SP)
    subprocess.run([f"{R}/build/{build}/python", "-X", "pystats",
                    f"{SP}/benchmarks/bm_{bench}/run_benchmark.py", "--worker",
                    "-l", str(loops), "-w", "0", "-n", "1", "-p", "1"],
                   env=env, check=True, capture_output=True)
    counts = collections.Counter()
    for path in glob.glob(f"{STATS}/*.txt"):
        for line in open(path):
            m = LINE.match(line.strip())
            if not m:
                continue
            what, kind, site, n = m.groups()
            key = f"site {site}" if site else (f"class {kind}" if kind else what)
            counts[key] += int(n)
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
    path, _, off = site.rpartition("+")
    out = subprocess.run(["addr2line", "-f", "-i", "-C", "-e", path, off],
                         capture_output=True, text=True).stdout.split("\n")
    # Pairs of function and file:line, innermost first: show the inlined
    # chain up to the function that holds the code.
    frames = []
    for i in range(0, len(out) - 1, 2):
        where = os.path.basename(out[i + 1]).split(" ")[0]
        m = re.match(r"generated_ir_cases.c.h:(\d+)", where)
        if m:
            where += f" {handler_at(int(m.group(1)))}"
        frames.append(f"{out[i]} ({where})")
    return " <- ".join(frames[:3])


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--sites", type=int, default=0)
    p.add_argument("build")
    p.add_argument("benches", nargs="+")
    a = p.parse_args()
    pyc = f"/tmp/boil6a/pyc-boxes-{a.build}"
    shutil.rmtree(pyc, ignore_errors=True)
    for bench in a.benches:
        run(a.build, bench, 2, pyc)  # warm the pyc cache
        lo = run(a.build, bench, 2, pyc)
        hi = run(a.build, bench, 10, pyc)
        per = {k: (hi[k] - lo[k]) / 8 for k in hi}
        ints, floats = per.get("inline int boxes", 0), per.get("inline float boxes", 0)
        classes = {k[6:]: v for k, v in per.items() if k.startswith("class ")}
        print(f"{a.build} {bench} boxes/loop: int {ints:,.0f} float {floats:,.0f} | "
              + " | ".join(f"{k} {v:,.0f}" for k, v in classes.items()), flush=True)
        sites = sorted(((v, k[5:]) for k, v in per.items() if k.startswith("site ")),
                       reverse=True)
        for v, site in sites[:a.sites]:
            if v > 0:
                print(f"    {v:12,.0f}  {resolve(site)}", flush=True)


main()
