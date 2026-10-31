"""Compare run.py results: one markdown table per metric and group.

Usage: compare.py REFERENCE.json OTHER.json [OTHER.json ...]

Each OTHER column is the ratio OTHER / REFERENCE (below 1 is fewer
instructions, less time or less memory).  The last row of each group is the
geometric mean of the ratios.  A ratio is left out (shown as n/a) when either
value is not positive, which happens only for micro rows whose value after
the skeleton subtraction is at or below zero; the geomean row says how many
rows it covers.
"""

from __future__ import annotations

import json
import math
import sys
from pathlib import Path

GROUPS = ("kernels", "micro", "memory")


def load(path: str) -> dict:
    return json.loads(Path(path).read_text())


def label(report: dict) -> str:
    return report["build"]["label"]


def fmt(value: float) -> str:
    if abs(value) >= 1e6:
        return f"{value / 1e6:,.1f}M"
    if abs(value) >= 100:
        return f"{value:,.0f}"
    return f"{value:.3g}"


def tables(reports: list[dict]) -> list[str]:
    ref, others = reports[0], reports[1:]
    out = []
    for metric, ref_table in ref["results"].items():
        if not any(metric in o["results"] for o in others):
            continue
        for group in GROUPS:
            names = [n for n, e in ref_table.items() if e["group"] == group]
            if not names:
                continue
            units = {ref_table[n]["unit"] for n in names}
            unit = units.pop() if len(units) == 1 else None
            out.append(f"### {metric}, {group}" + (f" ({unit})" if unit else "") + "\n")
            head = ["benchmark", label(ref)] + [f"{label(o)} / {label(ref)}" for o in others]
            out.append("| " + " | ".join(head) + " |")
            out.append("|---" + "|---:" * (len(head) - 1) + "|")
            logs: list[list[float]] = [[] for _ in others]
            for name in names:
                base = ref_table[name]["value"]
                row = [name if unit else f"{name} ({ref_table[name]['unit']})", fmt(base)]
                for i, other in enumerate(others):
                    entry = other["results"].get(metric, {}).get(name)
                    if entry is None or base <= 0 or entry["value"] <= 0:
                        row.append("n/a")
                        continue
                    ratio = entry["value"] / base
                    logs[i].append(math.log(ratio))
                    row.append(f"{ratio:.3f}")
                out.append("| " + " | ".join(row) + " |")
            geo = ["geomean", ""]
            for values in logs:
                geo.append(f"{math.exp(sum(values) / len(values)):.3f} ({len(values)}/{len(names)})"
                           if values else "n/a")
            out.append("| " + " | ".join(geo) + " |")
            out.append("")
    return out


def main(argv: list[str]) -> int:
    if len(argv) < 3:
        print(__doc__, file=sys.stderr)
        return 2
    reports = [load(p) for p in argv[1:]]
    print("\n".join(tables(reports)))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
