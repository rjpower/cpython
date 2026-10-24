"""Compare two pyperformance JSON result files.

Measures the ratio new/old of the mean run value (seconds) per benchmark and
the geometric mean over benchmarks present in both files. Ratio < 1 means the
new file is faster.

Invariants: dependency-free; parses pyperf's BenchmarkSuite JSON directly
(benchmarks -> runs -> values), ignoring calibration runs, which carry no
``values``. Benchmarks present in only one file are listed, not compared.
"""

from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path


def load(path: Path) -> dict[str, float]:
    """Return benchmark name -> mean value in seconds."""
    suite = json.loads(path.read_text())
    result: dict[str, float] = {}
    for bench in suite["benchmarks"]:
        values = [v for run in bench["runs"] for v in run.get("values", [])]
        if not values:
            continue
        name = bench.get("metadata", {}).get("name") or suite["metadata"]["name"]
        result[name] = sum(values) / len(values)
    return result


def compare(old: dict[str, float], new: dict[str, float]) -> tuple[list[tuple[str, float, float, float]], float]:
    """Return rows (name, old, new, ratio) sorted by ratio, and the geomean ratio."""
    rows = sorted(((n, old[n], new[n], new[n] / old[n]) for n in old.keys() & new.keys()),
                  key=lambda r: r[3])
    if not rows:
        raise ValueError("no benchmarks in common")
    geomean = math.exp(sum(math.log(r[3]) for r in rows) / len(rows))
    return rows, geomean


def render(rows: list[tuple[str, float, float, float]], geomean: float) -> str:
    """Format the comparison table."""
    lines = [f"{'benchmark':40} {'old (ms)':>11} {'new (ms)':>11} {'new/old':>8}"]
    lines += [f"{n:40} {o * 1e3:11.3f} {w * 1e3:11.3f} {r:8.3f}" for n, o, w, r in rows]
    lines.append(f"{'geometric mean':40} {'':>11} {'':>11} {geomean:8.3f}")
    return "\n".join(lines)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("old", type=Path)
    ap.add_argument("new", type=Path)
    ap.add_argument("--json", type=Path, help="also write JSON here")
    args = ap.parse_args()
    old, new = load(args.old), load(args.new)
    rows, geomean = compare(old, new)
    print(render(rows, geomean))
    for label, names in (("only in old", old.keys() - new.keys()), ("only in new", new.keys() - old.keys())):
        if names:
            print(f"{label}: {', '.join(sorted(names))}", file=sys.stderr)
    if args.json:
        args.json.write_text(json.dumps({
            "geomean_ratio": geomean,
            "benchmarks": [{"name": n, "old": o, "new": w, "ratio": r} for n, o, w, r in rows],
        }, indent=2) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
