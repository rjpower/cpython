"""Per-loop user-mode instructions of one pyperformance worker: runs at 2
and 10 loops, differenced, median of four.  Usage: perloop.py BUILD BENCH..."""
import os, statistics, subprocess, sys, glob, shutil
R = "/home/power/code/cpython/.worktrees/python-boil-the-ocean"
SP = glob.glob(f"{R}/build/pyperf-venvs/default/venv/*/lib/python3.16/site-packages")[0]
build = sys.argv[1]
def count(bench, loops, env):
    out = subprocess.run(
        ["perf", "stat", "-x,", "-e", "instructions:u", f"{R}/build/{build}/python",
         f"{SP}/benchmarks/bm_{bench}/run_benchmark.py", "--worker", "-l", str(loops),
         "-w", "0", "-n", "1", "-p", "1"],
        env=env, capture_output=True, text=True, check=True)
    for line in out.stderr.splitlines():
        if "instructions" in line:
            return int(line.split(",")[0])
    raise RuntimeError(out.stderr)
pyc = f"/tmp/boil3b/pyc-perloop-{build}"
shutil.rmtree(pyc, ignore_errors=True)
env = dict(os.environ, PYTHONPYCACHEPREFIX=pyc, PYTHONPATH=SP)
for bench in sys.argv[2:]:
    count(bench, 2, env)  # warm the pyc cache
    vals = [(count(bench, 10, env) - count(bench, 2, env)) / 8 for _ in range(4)]
    print(f"{build} {bench} {statistics.median(vals)/1e6:.1f}M  runs " +
          " ".join(f"{v/1e6:.1f}" for v in vals), flush=True)
