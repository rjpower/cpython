"""Run the checkpoint's three interleaved timing rounds after qualification.

Run through uv with --candidate BUILD --output NEW_DIRECTORY. The controls,
roster, sample counts and load guard follow plan.md; failed attempts are kept.
"""

import argparse
import importlib.util
import json
import math
import os
from pathlib import Path
import statistics
import subprocess
import sys
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidate", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    registry = Path(__file__).resolve().parents[2] / "bench/run.py"
    spec = importlib.util.spec_from_file_location("checkpoint_bench", registry)
    bench = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = bench
    spec.loader.exec_module(bench)
    kernels = [b.name for b in bench.BENCHMARKS if b.group == "kernels"]
    if len(kernels) != 29 or len(set(kernels)) != 29:
        raise RuntimeError("checkpoint roster is no longer exactly 29 kernels")
    build_root = Path("/home/power/code/cpython/.worktrees/python-boil-the-ocean/build")
    builds = {
        "upstream": build_root / "upstream-release-devlibs-3a440b6",
        "interim": build_root / "interim-release-devlibs-eba351",
        "candidate": args.candidate.resolve(),
    }
    labels = list(builds)
    results = {label: {name: [] for name in kernels} for label in labels}
    deadline = time.monotonic() + 45 * 60
    conditions = []
    for round_index in range(3):
        for kernel_index, name in enumerate(kernels):
            rotation = (round_index + kernel_index) % len(labels)
            order = labels[rotation:] + labels[:rotation]
            for label in order:
                if time.monotonic() >= deadline:
                    raise RuntimeError("declared 45-minute timing window expired")
                path = args.output / f"r{round_index + 1}-{name}-{label}.json"
                command = [sys.executable, "-B", str(registry), "--build",
                           str(builds[label]), "--label", label, "--set", "kernels",
                           "--only", name, "--metric", "time", "--max-load", "4",
                           "-o", str(path)]
                conditions.append({"round": round_index + 1, "kernel": name,
                                   "label": label, "load": os.getloadavg(),
                                   "command": command})
                (args.output / "invocations.json").write_text(json.dumps(conditions, indent=2))
                subprocess.run(command, check=True)
                data = json.loads(path.read_text())
                results[label][name].append(data["results"]["time"][name]["value"])
                print(f"round {round_index + 1}: {name} {label}", flush=True)
    (args.output / "round-medians.json").write_text(json.dumps(results, indent=2))
    rows = ["Candidate/control elapsed time; below 1 favors candidate.\n",
            "| Kernel | Candidate ns/loop | C/U | C/I | Candidate spread |",
            "| --- | ---: | ---: | ---: | ---: |"]
    ratios = {label: [] for label in ("upstream", "interim")}
    for name in kernels:
        medians = {label: statistics.median(results[label][name]) for label in labels}
        c = medians["candidate"]
        for label in ratios:
            ratios[label].append(c / medians[label])
        samples = results["candidate"][name]
        spread = 100 * (max(samples) / min(samples) - 1)
        rows.append(f"| {name} | {c:,.0f} | {ratios['upstream'][-1]:.3f} | "
                    f"{ratios['interim'][-1]:.3f} | {spread:.1f}% |")
    rows.append("| Geomean | | " + " | ".join(
        f"{math.exp(statistics.mean(map(math.log, ratios[label]))):.3f}"
        for label in ratios) + " | |")
    (args.output / "summary.md").write_text("\n".join(rows) + "\n")


if __name__ == "__main__":
    main()
