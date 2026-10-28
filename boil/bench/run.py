"""Run the internal benchmark set against one build and write one JSON file.

Usage: run.py --build BUILD [--set SET[,SET]] [--metric METRIC[,METRIC]]
              [--only NAME[,NAME]] [-o FILE]

The set is defined in boil/design/10-internal-benchmarks.md and documented in
boil/bench/README.md.  Every measurement runs in a fresh
``python -S -X pycache_prefix=DIR`` process, with one fresh prefix directory
per invocation of this script, removed at the end.  The child's environment
is this script's with every ``PYTHON*`` variable removed, as ``-E`` would,
and ``PYTHONHASHSEED=0`` set (``-E`` would ignore it), so str hashing, and
with it dict probing, is the same in every run and instruction counts
measure the build, not the seed.

Metrics:

instr
    User-mode instructions (``perf stat -e instructions:u``) per loop: the
    process runs at ``loops`` and at ``3 * loops``, and the difference is
    divided by ``2 * loops``, so start-up, import and compilation cancel.  One
    warm-up run fills the pyc cache first; the value is the median of
    ``--instr-runs`` (default 4) differenced pairs.  A micro kernel's loop
    is 1000 iterations of its statement, so its value is per iteration,
    less the median of its skeleton (the empty loop of the same form)
    measured in the same invocation; ``raw`` keeps the value before the
    subtraction.  If perf cannot count, this script exits with an error and
    writes nothing.
time
    Nanoseconds per loop: one process makes a warm-up call of ``run(loops)``
    then ``--time-runs`` (default 5) timed calls; the value is the median.
boxes
    Boxes of inline values per loop (per iteration for the micro set) on a
    ``--enable-pystats`` build of the value model: the process runs with
    ``-X pystats`` at ``loops`` and ``3 * loops`` after a warm-up, and the
    value is the difference of the int, float and array boxes, divided as
    for instr.  The entry also keeps the boxes by class, the boxes by
    instruction and the dispatches (instructions executed).  One pair: the
    counts are deterministic.  Py_STATS writes each process's stats to a new
    randomly named file in /tmp/py_stats, shared with other users of the
    machine, so the runner takes the one file that appears during its
    child's run and fails if there is not exactly one; it never clears the
    directory.  Boxing call sites: boil/tools/measure/box_audit.py -v.
rss
    Peak resident set size in KiB of a process calling ``run(loops)`` once,
    from GNU ``/usr/bin/time -f %M`` (not getrusage of children, which
    inherits the parent's high-water mark); median of ``--rss-runs``
    (default 3) processes.

The JSON holds the build, the machine, ``results`` (metric, then benchmark,
then the entry with its value, the per-run values and the loop and run
counts) and ``runs`` (per metric: the timestamp, the bench commit and the
1-minute load averages sampled before every measurement and at the end).
``--update`` merges a run's metrics into an existing file for the same build,
so instr and rss can be recorded under load and time later on an idle
machine; ``--max-load`` makes a run fail, writing nothing, if any sample
reaches the limit.

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
METRICS = ("instr", "time", "rss", "boxes")
PYSTATS_DIR = Path("/tmp/py_stats")
GNU_TIME = "/usr/bin/time"
CHILD_ENV = {k: v for k, v in os.environ.items() if not k.startswith("PYTHON")}
CHILD_ENV["PYTHONHASHSEED"] = "0"
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
    inner: int = 1  # iterations per loop; instr is reported per iteration
    skeleton: str | None = None  # benchmark whose instr is subtracted
    metrics: tuple[str, ...] | None = None  # default: the group's
    time_unit: str | None = None  # time in seconds per loop, under this name

    @property
    def path(self) -> Path:
        return INTERNAL / self.group / f"{self.module or self.name}.py"



def kernel(name: str, loops: int, module: str | None = None) -> Bench:
    """A pyperformance kernel; a module holding several takes the name as variant."""
    return Bench(name, "kernels", loops, module, name if module else None)


def micro(name: str, skeleton: str | None = None) -> Bench:
    """A micro kernel: 1000 iterations per loop, differenced at 200 and 600 loops."""
    return Bench(name, "micro", 200, inner=1000, skeleton=skeleton)


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
    # The object-protocol and calls set; each file's docstring says what an
    # iteration does.  Skeletons first: the others subtract them.
    micro("empty_loop"),
    micro("empty_loop_large_i"),
    micro("empty_list_loop"),
    micro("int_add", "empty_loop"),
    micro("int_add_small", "empty_loop"),
    micro("float_add", "empty_loop"),
    micro("int_compare", "empty_loop"),
    micro("float_compare", "empty_loop"),
    micro("int_chain", "empty_loop"),
    micro("float_chain", "empty_loop"),
    micro("compare_branch", "empty_loop"),
    micro("store_fast_two", "empty_loop"),
    micro("attr_load", "empty_loop"),
    micro("attr_load_poly", "empty_list_loop"),
    micro("attr_load_poly5", "empty_list_loop"),
    micro("attr_store", "empty_loop"),
    micro("attr_store_boxed", "empty_loop"),
    micro("attr_store_poly", "empty_list_loop"),
    micro("attr_load_after_dict", "empty_loop"),
    micro("attr_store_after_dict", "empty_loop"),
    micro("attr_load_dataclass", "empty_loop"),
    micro("attr_store_dataclass", "empty_loop"),
    micro("slots_load", "empty_loop"),
    micro("property_load", "empty_loop"),
    micro("class_attr_load", "empty_loop"),
    micro("method_call", "empty_loop"),
    micro("new_instance", "empty_loop"),
    micro("new_instance_empty", "empty_loop"),
    micro("isinstance", "empty_loop"),
    micro("global_load", "empty_loop"),
    micro("builtin_load", "empty_loop"),
    micro("call_len", "empty_loop"),
    micro("call_abs", "empty_loop"),
    micro("call_math_sin", "empty_loop"),
    micro("call_py_1", "empty_loop"),
    micro("call_py_3", "empty_loop"),
    micro("call_py_kw", "empty_loop"),
    micro("call_closure", "empty_loop"),
    micro("generator_resume", "empty_loop"),
    micro("list_subscript", "empty_loop"),
    micro("tuple_subscript", "empty_loop"),
    micro("dict_subscript", "empty_loop"),
    micro("float_list_subscript", "empty_loop"),
    micro("str_join", "empty_loop"),
    micro("str_concat", "empty_loop"),
    micro("str_concat_loop", "empty_loop"),
    micro("try_noraise", "empty_loop"),
    micro("try_raise", "empty_loop"),
    # The slot boundaries of note 12 section 10 (step 6d)
    micro("getattr_property", "empty_loop"),
    micro("getattr_property_float", "empty_loop"),
    micro("getattribute_hook", "empty_loop"),
    micro("getattr_hook", "empty_loop"),
    micro("setattr_hook", "empty_loop"),
    micro("descr_get_py", "empty_loop"),
    micro("descr_set_py", "empty_loop"),
    micro("property_store", "empty_loop"),
    micro("property_store_float", "empty_loop"),
    micro("call_instance", "empty_loop"),
    micro("namedtuple_field", "empty_loop"),
    micro("member_double", "empty_loop"),
    micro("object_setattr", "empty_loop"),
    micro("class_attr_store", "empty_loop"),
    micro("method_lazy_dict", "empty_loop"),
    # The getsets (6d rung 4)
    micro("getset_number", "empty_loop"),
    micro("cell_contents", "empty_loop"),
]

# The memory and collector group: fixed workloads, one loop each; each
# file's docstring documents its objects.
BENCHMARKS += [
    Bench("startup", "memory", 1, metrics=("rss",)),
    Bench("imports", "memory", 1, metrics=("rss",)),
    Bench("tuples_10m", "memory", 1),
    Bench("instances_1m", "memory", 1),
    Bench("dicts_200k", "memory", 1),
] + [Bench(f"collector_live{mb}", "memory", 1, "collector", str(mb),
           time_unit="s per allocated GB") for mb in (16, 128, 512)]
GROUP_METRICS = {"kernels": ("instr", "time", "rss", "boxes"), "micro": ("instr", "boxes"),
                 "memory": ("instr", "time", "rss")}


def metrics_of(bench: Bench) -> tuple[str, ...]:
    return bench.metrics or GROUP_METRICS[bench.group]


class MeasurementError(RuntimeError):
    """A measurement could not be made; nothing is guessed in its place."""


@dataclass
class Context:
    python: Path
    pycache: Path
    scratch: Path

    def command(self, bench: Bench, mode: str, loops: int, runs: int = 1,
                options: tuple[str, ...] = ()) -> list[str]:
        cmd = [str(self.python), "-S", *options, "-X", f"pycache_prefix={self.pycache}",
               str(DRIVER), mode, str(bench.path), str(loops), str(runs)]
        return cmd + ([bench.variant] if bench.variant else [])


def run_checked(cmd: list[str], what: str) -> subprocess.CompletedProcess:
    proc = subprocess.run(cmd, check=False, capture_output=True, text=True, env=CHILD_ENV)
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
    perf_instructions(ctx, [str(ctx.python), "-S", "-c", "pass"], "perf probe")


def measure_instr(ctx: Context, bench: Bench, runs: int, loops: int) -> dict:
    lo, hi = loops, HIGH * loops
    perf_instructions(ctx, ctx.command(bench, "run", lo), f"{bench.name} warm-up")
    values = []
    for _ in range(runs):
        a = perf_instructions(ctx, ctx.command(bench, "run", lo), f"{bench.name} at {lo}")
        b = perf_instructions(ctx, ctx.command(bench, "run", hi), f"{bench.name} at {hi}")
        values.append((b - a) / ((hi - lo) * bench.inner))
    unit = "instructions:u per iteration" if bench.inner > 1 else "instructions:u per loop"
    entry = {"group": bench.group, "unit": unit,
             "value": statistics.median(values), "values": values,
             "loops": [lo, hi], "runs": runs}
    if bench.inner > 1:
        entry["inner"] = bench.inner
    return entry


def subtract_skeletons(table: dict, benches: list[Bench]) -> None:
    """Subtract each micro kernel's skeleton, keeping the value before as raw."""
    for bench in benches:
        if bench.skeleton:
            entry = table[bench.name]
            entry["raw"] = entry["value"]
            entry["skeleton"] = bench.skeleton
            entry["value"] = entry["raw"] - table[bench.skeleton]["value"]


def measure_time(ctx: Context, bench: Bench, runs: int, loops: int) -> dict:
    proc = run_checked(ctx.command(bench, "time", loops, runs), f"{bench.name} timing")
    durations = json.loads(proc.stdout.strip().splitlines()[-1])
    if bench.time_unit:
        values, unit = [d / loops / 1e9 for d in durations], bench.time_unit
    else:
        values, unit = [d / loops for d in durations], "ns per loop"
    return {"group": bench.group, "unit": unit,
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


BOX_TOTALS = ("Object inline int boxes", "Object inline float boxes",
              "Object inline array boxes")


def pystats_counts(ctx: Context, bench: Bench, loops: int) -> dict[str, int]:
    """Run the benchmark under -X pystats and return its counters by name."""
    PYSTATS_DIR.mkdir(exist_ok=True)
    before = set(PYSTATS_DIR.iterdir())
    run_checked(ctx.command(bench, "run", loops, options=("-X", "pystats")),
                f"{bench.name} under pystats at {loops}")
    new = set(PYSTATS_DIR.iterdir()) - before
    if len(new) != 1:
        raise MeasurementError(f"{len(new)} new files in {PYSTATS_DIR} during {bench.name}; "
                               "another process is writing stats, so none is attributable")
    path = new.pop()
    counts = {}
    for line in path.read_text().splitlines():
        key, sep, value = line.strip().rpartition(":")
        if sep and value.strip().isdigit():
            counts[key.strip()] = int(value)
    path.unlink()
    return counts


def check_pystats(ctx: Context) -> None:
    """Exit before measuring anything if the build was not configured with --enable-pystats."""
    proc = run_checked([str(ctx.python), "-S", "-c",
                        "import sys; print(int('_stats_on' in dir(sys)))"], "pystats probe")
    if proc.stdout.strip() != "1":
        raise MeasurementError(f"{ctx.python} is not a --enable-pystats build; "
                               "the boxes metric needs one")


def measure_boxes(ctx: Context, bench: Bench, runs: int, loops: int) -> dict:
    lo, hi = loops, HIGH * loops
    pystats_counts(ctx, bench, lo)  # warm-up: fills the pyc cache
    a, b = pystats_counts(ctx, bench, lo), pystats_counts(ctx, bench, hi)
    missing = [k for k in BOX_TOTALS if k not in b]
    if missing:
        raise MeasurementError(f"{ctx.python} reports no {missing[0]}: not a Py_STATS "
                               "build of the value model")
    scale = (hi - lo) * bench.inner

    def per(key: str) -> float:
        return (b.get(key, 0) - a.get(key, 0)) / scale

    by_class = {k[len("Object boxes ("):-1]: per(k) for k in b if k.startswith("Object boxes (")}
    by_op = {k[len("opcode["):-len("].boxes")]: per(k) for k in b
             if k.startswith("opcode[") and k.endswith("].boxes") and per(k)}
    dispatches = sum(per(k) for k in b if k.endswith("].execution_count"))
    unit = "boxes per iteration" if bench.inner > 1 else "boxes per loop"
    entry = {"group": bench.group, "unit": unit, "value": sum(per(k) for k in BOX_TOTALS),
             "int_boxes": per(BOX_TOTALS[0]), "float_boxes": per(BOX_TOTALS[1]),
             "array_boxes": per(BOX_TOTALS[2]), "by_class": by_class,
             "by_instruction": dict(sorted(by_op.items(), key=lambda kv: -kv[1])),
             "dispatches": dispatches, "allocations": per("Object allocations"),
             "dict_mode": {k[len("Object materialize dict ("):-1]: per(k) for k in b
                           if k.startswith("Object materialize dict (")},
             "values": [sum(per(k) for k in BOX_TOTALS)],
             "loops": [lo, hi], "runs": 1}
    if bench.inner > 1:
        entry["inner"] = bench.inner
    return entry


def git(*args: str) -> str:
    return subprocess.run(["git", "-C", str(REPO), *args], capture_output=True,
                          text=True, check=True).stdout.strip()


def machine() -> dict:
    info = {"hostname": platform.node(), "kernel": platform.release(),
            "cpus": os.cpu_count()}
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
    probe = ("import json, sys; "
             "print(json.dumps([sys.version, list(sys._git), sys.flags.hash_randomization]))")
    proc = run_checked([str(python), "-S", "-c", probe], "build version probe")
    version, git_info, randomized = json.loads(proc.stdout)
    if randomized:
        raise MeasurementError("PYTHONHASHSEED=0 did not reach the child interpreter")
    return {"label": label, "path": str(python), "version": version, "git": git_info,
            "hash_seed": 0}


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
        needed = set(names) | {b.skeleton for b in chosen if b.name in names and b.skeleton}
        chosen = [b for b in chosen if b.name in needed]
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
    ap.add_argument("--max-load", type=float,
                    help="fail if the 1-minute load average reaches this before any measurement")
    ap.add_argument("--update", action="store_true",
                    help="merge into the existing -o file for the same build")
    ap.add_argument("-o", "--output", type=Path, help="JSON file (default: stdout)")
    args = ap.parse_args(argv)
    if args.update and not (args.output and args.output.exists()):
        raise SystemExit("run.py: --update needs an existing -o file")

    python = resolve_build(args.build)
    metrics = split(args.metric, METRICS, "metric")
    benches = select(split(args.set, GROUPS, "set"), args.only)
    label = args.label or python.parent.name
    started = datetime.datetime.now().astimezone()
    loads: list[float] = []

    def sample_load() -> None:
        load = os.getloadavg()[0]
        loads.append(load)
        if args.max_load is not None and load >= args.max_load:
            raise MeasurementError(f"1-minute load average {load:.2f} reached --max-load "
                                   f"{args.max_load}; nothing written")
    with tempfile.TemporaryDirectory(prefix="boil-bench-") as tmp:
        ctx = Context(python, Path(tmp) / "pycache", Path(tmp))
        results: dict[str, dict] = {}
        try:
            build = build_info(python, label)
            if "instr" in metrics:
                check_perf(ctx)
            if "boxes" in metrics:
                check_pystats(ctx)
            if "rss" in metrics and not os.access(GNU_TIME, os.X_OK):
                raise MeasurementError(f"the rss metric needs GNU time at {GNU_TIME}")
            for metric in metrics:
                measure = {"instr": measure_instr, "time": measure_time,
                           "rss": measure_rss, "boxes": measure_boxes}[metric]
                runs = {"instr": args.instr_runs, "time": args.time_runs,
                        "rss": args.rss_runs, "boxes": 1}[metric]
                table = results.setdefault(metric, {})
                chosen = [b for b in benches if metric in metrics_of(b)]
                for bench in chosen:
                    sample_load()
                    entry = measure(ctx, bench, runs, args.loops or bench.loops)
                    table[bench.name] = entry
                    print(f"{label} {metric} {bench.group}/{bench.name} "
                          f"{entry['value']:,.1f} {entry['unit']}", file=sys.stderr, flush=True)
                if metric == "instr":
                    subtract_skeletons(table, chosen)
            sample_load()
        except MeasurementError as exc:
            print(f"run.py: {exc}", file=sys.stderr)
            return 1
    run_info = {"timestamp": started.isoformat(timespec="seconds"),
                "commit": {"sha": git("rev-parse", "HEAD"),
                           "dirty": bool(git("status", "--porcelain", "--", "boil/bench"))},
                "load_1min": {"max": max(loads), "samples": loads}}
    report = {"schema": "boil-bench/1", "build": build, "machine": machine(),
              "results": {}, "runs": {}}
    if args.update:
        report = json.loads(args.output.read_text())
        if (report["build"]["path"], report["build"]["label"]) != (build["path"], label):
            raise SystemExit(f"run.py: {args.output} holds another build")
        if report["machine"]["hostname"] != platform.node():
            raise SystemExit(f"run.py: {args.output} was recorded on another machine")
    for metric, table in results.items():
        report["results"][metric] = table
        report["runs"][metric] = run_info
    text = json.dumps(report, indent=1) + "\n"
    if args.output:
        args.output.write_text(text)
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
