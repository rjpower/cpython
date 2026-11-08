"""Deferred SIGPROF sampling using fresh qualification timing medians.

Run only after elapsed/RSS measurement is complete. This script does not build
or modify interpreters. Its --sampler must be a previously built shared object.
"""
import argparse
import importlib.util
import json
import math
import os
from pathlib import Path
import shutil
import signal
import statistics
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
BENCH = ROOT / "bench" / "run.py"
SYMBOLIZER = ROOT / "tools" / "sampler" / "perf-triage-symbolize.py"


def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def child(path, variant, warmup, loops):
    bench = load_module("_boil_bench_module", path)
    setup = getattr(bench, "setup", None)
    if setup is not None:
        setup(None if variant == "-" else variant)
    bench.run(warmup)
    signal.setitimer(signal.ITIMER_PROF, 0.001, 0.001)
    try:
        bench.run(loops)
    finally:
        signal.setitimer(signal.ITIMER_PROF, 0)


def checked(command, **kwargs):
    return subprocess.run(command, check=True, text=True, **kwargs)


def offset_equals_vaddr(path):
    result = checked(["readelf", "-lW", str(path)], capture_output=True)
    loads = [line.split() for line in result.stdout.splitlines()
             if line.lstrip().startswith("LOAD ") and
             "E" in line.split()[6:-1]]
    return bool(loads) and all(int(row[1], 16) == int(row[2], 16) for row in loads)


def main():
    if len(sys.argv) > 1 and sys.argv[1] == "--child":
        child(Path(sys.argv[2]), sys.argv[3], int(sys.argv[4]), int(sys.argv[5]))
        return
    parser = argparse.ArgumentParser()
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--upstream", type=Path, required=True)
    parser.add_argument("--medians", type=Path, required=True,
                        help="fresh measure.py round-medians.json (ns/loop)")
    parser.add_argument("--sampler", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True,
                        help="new directory; never overwrites prior evidence")
    parser.add_argument("kernels", nargs="+")
    args = parser.parse_args()
    roster = load_module("_sample_roster", BENCH)
    kernels = {b.name: b for b in roster.BENCHMARKS if b.group == "kernels"}
    if len(kernels) != 29 or len(set(args.kernels)) != len(args.kernels):
        raise ValueError("unexpected roster or duplicate requested kernels")
    timing = json.loads(args.medians.read_text())
    args.output.mkdir(parents=True, exist_ok=False)
    shutil.copy2(args.medians, args.output / "round-medians.json")
    shutil.copy2(args.sampler, args.output / "sampler.so")
    for label, executable in (("upstream", args.upstream), ("candidate", args.candidate)):
        executable = executable.resolve(strict=True)
        shutil.copy2(executable, args.output / (label + "-python"))
        identity = checked(["readelf", "-n", str(executable)], capture_output=True).stdout
        (args.output / (label + "-elf-notes.txt")).write_text(identity)
        for name in args.kernels:
            bench = kernels[name]
            values = timing[label][name]
            median = statistics.median(values)
            if not math.isfinite(median) or median <= 0:
                raise ValueError(f"bad timing for {label}/{name}")
            loops = max(1, math.ceil(2_000_000_000 / median))
            stem = f"{label}-{name}"
            env = {key: value for key, value in os.environ.items()
                   if not key.startswith("PYTHON")}
            env.update(LD_PRELOAD=str(args.sampler.resolve()), SAMPLER_US="0",
                       SAMPLER_DEPTH="1", SAMPLER_OUT=str((args.output / stem).resolve()),
                       PYTHONHASHSEED="0")
            cache = (args.output / (stem + "-pycache")).resolve()
            if cache.exists():
                raise RuntimeError(f"cache path already exists: {cache}")
            command = [str(executable), "-S", "-B", "-X", f"pycache_prefix={cache}",
                       str(Path(__file__).resolve()), "--child",
                       str(bench.path), bench.variant or "-", str(bench.loops), str(loops)]
            (args.output / (stem + "-command.json")).write_text(json.dumps({
                "command": command, "environment_overrides": {k: env[k] for k in
                ("LD_PRELOAD", "SAMPLER_US", "SAMPLER_DEPTH", "SAMPLER_OUT", "PYTHONHASHSEED")},
                "median_ns_per_loop": median, "loops": loops}, indent=2))
            result = subprocess.run(command, env=env, capture_output=True, text=True)
            (args.output / (stem + "-stdout.txt")).write_text(result.stdout)
            (args.output / (stem + "-stderr.txt")).write_text(result.stderr)
            (args.output / (stem + "-returncode.txt")).write_text(str(result.returncode) + "\n")
            if result.returncode != 0:
                raise RuntimeError(f"{stem}: child exited {result.returncode}")
            raw = list(args.output.glob(stem + ".[0-9]*"))
            if len(raw) != 1:
                raise RuntimeError(f"expected exactly one sample file for {stem}")
            lines = raw[0].read_text().splitlines()
            if not lines or lines[0] != "depth 1" or "maps" not in lines:
                raise RuntimeError(f"{stem}: missing depth-one header or maps")
            maps_index = lines.index("maps")
            sample_count = maps_index - 1
            if sample_count <= 0:
                raise RuntimeError(f"{stem}: no samples")
            mappings = lines[maps_index + 1:]
            if not mappings:
                raise RuntimeError(f"{stem}: empty maps")
            paths = {line.split(None, 5)[5] for line in mappings
                     if len(line.split(None, 5)) == 6 and "x" in line.split()[1]
                     and line.split(None, 5)[5].startswith("/")}
            compatible = all(offset_equals_vaddr(p) for p in paths)
            (args.output / (stem + "-mapping-check.json")).write_text(json.dumps({
                "executable_load_offsets_equal_vaddrs": compatible,
                "sample_count": sample_count, "paths": sorted(paths)}, indent=2))
            if not compatible:
                raise RuntimeError(f"{stem}: symbolizer load-bias assumption failed; retain raw data")
            report = checked([sys.executable, str(SYMBOLIZER), str(raw[0]), "30"], capture_output=True)
            (args.output / (stem + "-symbols.txt")).write_text(report.stdout)
    (args.output / "limitations.txt").write_text(
        "SIGPROF statistical samples, depth one; no caller walk or hardware instruction counts. "
        "Symbol shares include unresolved and external samples. PyValue symbols include value "
        "operations and are not inherently boxing. Retain raw samples and maps.\n")


if __name__ == "__main__":
    main()
