"""Run the internal benchmark set against one build and write one JSON file.

Usage: run.py --build BUILD [--set SET[,SET]] [--metric METRIC[,METRIC]]
              [--only NAME[,NAME]] [-o FILE]

The set is defined in boil/design/10-internal-benchmarks.md and documented in
boil/bench/README.md.  Every measurement runs in a fresh
``python -S -E -X pycache_prefix=DIR`` process (``-E`` ignores
``PYTHONPYCACHEPREFIX``, so the prefix goes through ``-X``), with one fresh
prefix directory per invocation of this script, removed at the end.

Metrics:

instr
    User-mode instructions (``perf stat -e instructions:u``) per loop: the
    process runs at ``loops`` and at ``3 * loops``, and the difference is
    divided by ``2 * loops``, so start-up, import and compilation cancel.  One
    warm-up run fills the pyc cache first; the value is the median of
    ``--instr-runs`` (default 4) differenced pairs.  If perf cannot count,
    this script exits with an error and writes nothing.
time
    Nanoseconds per loop: one process makes a warm-up call of ``run(loops)``
    then ``--time-runs`` (default 5) timed calls; the value is the median.
rss
    Peak resident set size in KiB of a process calling ``run(loops)`` once,
    from GNU ``/usr/bin/time -f %M`` (not getrusage of children, which
    inherits the parent's high-water mark); median of ``--rss-runs``
    (default 3) processes.

This script needs only the standard library and any Python 3.10 or later; the
interpreter under test is ``--build``.
"""

from __future__ import annotations

import argparse
import datetime
import json
import os
import platform
import shutil
import statistics
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path

HERE = Path(__file__).resolve().parent
INTERNAL = HERE / "internal"
DRIVER = INTERNAL / "driver.py"
REPO = HERE.parents[1]
GROUPS = ("kernels", "micro", "memory")
METRICS = ("instr", "time", "rss")
GNU_TIME = "/usr/bin/time"
# The high repeat count of the instruction difference, as a multiple of the low.
HIGH = 3


@dataclass(frozen=True)
class Bench:
    """One benchmark: a module under internal/ exposing run(loops).

    ``loops`` is the fixed loop count of one timed call and the low count of
    the instruction difference.  Recorded with every value; never calibrated.
    """

    name: str
    group: str
    loops: int
    module: str | None = None  # file under the group's directory; default: name
    variant: str | None = None  # passed to the module's setup()

    @property
    def path(self) -> Path:
        return INTERNAL / self.group / f"{self.module or self.name}.py"



def kernel(name: str, loops: int, module: str | None = None) -> Bench:
    """A pyperformance kernel; a module holding several takes the name as variant."""
    return Bench(name, "kernels", loops, module, name if module else None)


# Loop counts: the smallest giving one call of about 0.1 s or more on the
# baseline build (measured 2026-10-27 under load, so 0.1 to 0.4 s idle).
# Names are pyperformance's, so the 29 kernels match its results files.
BENCHMARKS = [
    kernel("richards", 2),
    kernel("go", 1),
    kernel("nbody", 1),
    kernel("float", 1),
    kernel("spectral_norm", 1),
    kernel("raytrace", 1),
    kernel("deltablue", 30),
    kernel("chaos", 2),
    kernel("nqueens", 1),
    kernel("fannkuch", 1),
    kernel("generators", 2),
    kernel("comprehensions", 6000),
    kernel("coroutines", 4),
    kernel("unpack_sequence", 4000),
    kernel("deepcopy", 300, "deepcopy"),
    kernel("deepcopy_reduce", 20000, "deepcopy"),
    kernel("deepcopy_memo", 4000, "deepcopy"),
    kernel("json_dumps", 10),
    kernel("json_loads", 200),
    kernel("pickle", 400, "pickle"),
    kernel("unpickle", 300, "pickle"),
    kernel("regex_compile", 1),
    kernel("logging_format", 1000, "logging"),
    kernel("logging_silent", 60000, "logging"),
    kernel("logging_simple", 1000, "logging"),
    kernel("xml_etree_generate", 1, "xml_etree"),
    kernel("xml_etree_iterparse", 1, "xml_etree"),
    kernel("xml_etree_parse", 1, "xml_etree"),
    kernel("xml_etree_process", 1, "xml_etree"),
]


class MeasurementError(RuntimeError):
    """A measurement could not be made; nothing is guessed in its place."""


@dataclass
class Context:
    python: Path
    pycache: Path
    scratch: Path

    def command(self, bench: Bench, mode: str, loops: int, runs: int = 1) -> list[str]:
        cmd = [str(self.python), "-S", "-E", "-X", f"pycache_prefix={self.pycache}",
               str(DRIVER), mode, str(bench.path), str(loops), str(runs)]
        return cmd + ([bench.variant] if bench.variant else [])


def run_checked(cmd: list[str], what: str) -> subprocess.CompletedProcess:
    proc = subprocess.run(cmd, check=False, capture_output=True, text=True)
    if proc.returncode != 0:
        raise MeasurementError(f"{what} failed with exit status {proc.returncode}:\n"
                               f"$ {' '.join(cmd)}\n{proc.stdout[-2000:]}{proc.stderr[-4000:]}")
    return proc


def perf_instructions(ctx: Context, cmd: list[str], what: str) -> int:
    """Run cmd under perf stat and return its user-mode instruction count."""
    out = ctx.scratch / "perf.txt"
    run_checked(["perf", "stat", "-x,", "-o", str(out), "-e", "instructions:u", "--", *cmd], what)
    for line in out.read_text().splitlines():
        fields = line.split(",")
        if len(fields) > 2 and fields[2].startswith("instructions"):
            if not fields[0].isdigit():
                raise MeasurementError(f"perf could not count instructions:u for {what}: {line}")
            return int(fields[0])
    raise MeasurementError(f"no instructions:u line in perf output for {what}:\n{out.read_text()}")


def check_perf(ctx: Context) -> None:
    """Exit before measuring anything if perf cannot count user-mode instructions."""
    if shutil.which("perf") is None:
        raise MeasurementError("perf is not on PATH; the instr metric needs it")
    perf_instructions(ctx, [str(ctx.python), "-S", "-E", "-c", "pass"], "perf probe")


def measure_instr(ctx: Context, bench: Bench, runs: int, loops: int) -> dict:
    lo, hi = loops, HIGH * loops
    perf_instructions(ctx, ctx.command(bench, "run", lo), f"{bench.name} warm-up")
    values = []
    for _ in range(runs):
        a = perf_instructions(ctx, ctx.command(bench, "run", lo), f"{bench.name} at {lo}")
        b = perf_instructions(ctx, ctx.command(bench, "run", hi), f"{bench.name} at {hi}")
        values.append((b - a) / (hi - lo))
    return {"group": bench.group, "unit": "instructions:u per loop",
            "value": statistics.median(values), "values": values,
            "loops": [lo, hi], "runs": runs}


def measure_time(ctx: Context, bench: Bench, runs: int, loops: int) -> dict:
    proc = run_checked(ctx.command(bench, "time", loops, runs), f"{bench.name} timing")
    durations = json.loads(proc.stdout.strip().splitlines()[-1])
    values = [d / loops for d in durations]
    return {"group": bench.group, "unit": "ns per loop",
            "value": statistics.median(values), "values": values,
            "loops": loops, "runs": runs, "warmup_calls": 1}


def measure_rss(ctx: Context, bench: Bench, runs: int, loops: int) -> dict:
    out = ctx.scratch / "time.txt"
    values = []
    for _ in range(runs):
        run_checked([GNU_TIME, "-f", "%M", "-o", str(out), *ctx.command(bench, "run", loops)],
                    f"{bench.name} peak RSS")
        values.append(int(out.read_text().split()[-1]))
    return {"group": bench.group, "unit": "KiB peak RSS",
            "value": statistics.median(values), "values": values,
            "loops": loops, "runs": runs}


def git(*args: str) -> str:
    return subprocess.run(["git", "-C", str(REPO), *args], capture_output=True,
                          text=True, check=True).stdout.strip()


def machine() -> dict:
    info = {"hostname": platform.node(), "kernel": platform.release(),
            "cpus": os.cpu_count(), "load_average": list(os.getloadavg())}
    for path, key, field in (("/proc/cpuinfo", "cpu", "model name"),
                             ("/proc/meminfo", "memory", "MemTotal")):
        try:
            for line in Path(path).read_text().splitlines():
                if line.startswith(field):
                    info[key] = line.split(":", 1)[1].strip()
                    break
        except OSError:
            pass
    return info


def build_info(python: Path, label: str) -> dict:
    proc = run_checked([str(python), "-S", "-E", "-c",
                        "import json, sys; print(json.dumps([sys.version, list(sys._git)]))"],
                       "build version probe")
    version, git_info = json.loads(proc.stdout)
    return {"label": label, "path": str(python), "version": version, "git": git_info}


def resolve_build(build: str) -> Path:
    path = Path(build).resolve()
    if path.is_dir():
        path = path / "python"
    if not os.access(path, os.X_OK):
        raise SystemExit(f"run.py: {path} is not an executable interpreter")
    return path


def split(value: str, allowed: tuple[str, ...], what: str) -> list[str]:
    items = [v for v in value.split(",") if v]
    if items == ["all"]:
        return list(allowed)
    unknown = [v for v in items if v not in allowed]
    if unknown:
        raise SystemExit(f"run.py: unknown {what} {', '.join(unknown)}; choose from "
                         f"{', '.join(allowed)} or all")
    return items


def select(sets: list[str], only: str | None) -> list[Bench]:
    chosen = [b for b in BENCHMARKS if b.group in sets]
    if only:
        names = only.split(",")
        missing = set(names) - {b.name for b in chosen}
        if missing:
            raise SystemExit(f"run.py: no benchmark {', '.join(sorted(missing))} in {sets}")
        chosen = [b for b in chosen if b.name in names]
    return chosen


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", required=True,
                    help="interpreter, or a build directory holding ./python")
    ap.add_argument("--label", help="name of the build in the results (default: its directory)")
    ap.add_argument("--set", default="kernels", help=f"{','.join(GROUPS)} or all")
    ap.add_argument("--metric", default="instr", help=f"{','.join(METRICS)} or all")
    ap.add_argument("--only", help="comma-separated benchmark names within the set")
    ap.add_argument("--loops", type=int,
                    help="override every benchmark's loop count (tests and smoke runs only)")
    ap.add_argument("--instr-runs", type=int, default=4)
    ap.add_argument("--time-runs", type=int, default=5)
    ap.add_argument("--rss-runs", type=int, default=3)
    ap.add_argument("-o", "--output", type=Path, help="JSON file (default: stdout)")
    args = ap.parse_args(argv)

    python = resolve_build(args.build)
    metrics = split(args.metric, METRICS, "metric")
    benches = select(split(args.set, GROUPS, "set"), args.only)
    label = args.label or python.parent.name
    started = datetime.datetime.now().astimezone()
    machine_info = machine()
    with tempfile.TemporaryDirectory(prefix="boil-bench-") as tmp:
        ctx = Context(python, Path(tmp) / "pycache", Path(tmp))
        results: dict[str, dict] = {}
        try:
            if "instr" in metrics:
                check_perf(ctx)
            if "rss" in metrics and not os.access(GNU_TIME, os.X_OK):
                raise MeasurementError(f"the rss metric needs GNU time at {GNU_TIME}")
            for metric in metrics:
                measure = {"instr": measure_instr, "time": measure_time,
                           "rss": measure_rss}[metric]
                runs = {"instr": args.instr_runs, "time": args.time_runs,
                        "rss": args.rss_runs}[metric]
                table = results.setdefault(metric, {})
                for bench in benches:
                    entry = measure(ctx, bench, runs, args.loops or bench.loops)
                    table[bench.name] = entry
                    print(f"{label} {metric} {bench.group}/{bench.name} "
                          f"{entry['value']:,.1f} {entry['unit']}", file=sys.stderr, flush=True)
        except MeasurementError as exc:
            print(f"run.py: {exc}", file=sys.stderr)
            return 1
    machine_info["load_average_end"] = list(os.getloadavg())
    report = {
        "schema": "boil-bench/1",
        "timestamp": started.isoformat(timespec="seconds"),
        "commit": {"sha": git("rev-parse", "HEAD"),
                   "dirty": bool(git("status", "--porcelain", "--", "boil/bench"))},
        "machine": machine_info,
        "build": build_info(python, label),
        "results": results,
    }
    text = json.dumps(report, indent=1) + "\n"
    if args.output:
        args.output.write_text(text)
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
