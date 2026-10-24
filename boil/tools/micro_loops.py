"""Instructions per iteration of small interpreter loops, via ``perf stat``.

Measures user-mode instructions (``instructions:u``) per iteration of the
inner ``for i in range(1000)`` loop bodies listed in
boil/design/02-tagged-values.md section 1.

Invariants: each benchmark is run twice in a fresh ``python -S -E`` process
with different outer repeat counts; the per-iteration cost is the difference
in instruction counts divided by the difference in iterations, so start-up and
compilation cancel exactly. Rows other than the two skeleton rows subtract the
empty-loop skeleton, as the design note does. If ``perf`` cannot count
instructions, the tool exits with an error rather than guessing.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
DEFAULT_INTERPRETER = REPO / "build" / "baseline-default" / "python"
INNER = 1000
REPEATS = (200, 600)


@dataclass(frozen=True)
class Loop:
    name: str
    setup: str
    body: str
    start: int = 0
    subtract_skeleton: bool = True
    design_note: int | None = None  # value quoted in 02-tagged-values.md


LOOPS = [
    Loop("empty loop skeleton", "", "pass", subtract_skeleton=False, design_note=80),
    Loop("empty loop, i >= 1025 (whole loop)", "", "pass", start=1025,
         subtract_skeleton=False, design_note=143),
    Loop("int add, result not cached", "x = 2000; y = 3000", "z = x + y", design_note=164),
    Loop("int add, cached small result", "x = 1; y = 2", "z = x + y", design_note=100),
    Loop("float add", "a = 1.5; b = 2.5", "z = a + b", design_note=130),
    Loop("int compare", "x = 1; y = 2", "z = x < y", design_note=93),
    Loop("float compare", "a = 1.5; b = 2.5", "z = a < b", design_note=79),
    Loop("list-of-floats subscript", "l = [1.5] * 8; j = 3", "z = l[j]", design_note=88),
    Loop("two STORE_FAST", "x = 1; y = 2", "z = x; z = y", design_note=57),
]


def program(loop: Loop) -> str:
    """Return the source of a script running the loop; argv[1] is the outer repeat count."""
    return (
        "import sys\n"
        "def f(reps):\n"
        f"    {loop.setup or 'pass'}\n"
        "    for _ in range(reps):\n"
        f"        for i in range({loop.start}, {loop.start + INNER}):\n"
        f"            {loop.body}\n"
        "f(int(sys.argv[1]))\n"
    )


def count_instructions(interpreter: Path, script: Path, reps: int) -> int:
    """Run the script under perf stat and return user-mode instructions."""
    cmd = ["perf", "stat", "-e", "instructions:u", "-x,", str(interpreter), "-S", "-E",
           str(script), str(reps)]
    proc = subprocess.run(cmd, capture_output=True, text=True, check=True)
    for line in proc.stderr.splitlines():
        fields = line.split(",")
        if len(fields) > 2 and fields[2].startswith("instructions"):
            return int(fields[0])
    raise RuntimeError(f"no instructions counter in perf output:\n{proc.stderr}")


def per_iteration(interpreter: Path, script: Path) -> float:
    """Return instructions per inner iteration, cancelling start-up cost."""
    lo, hi = (count_instructions(interpreter, script, r) for r in REPEATS)
    return (hi - lo) / ((REPEATS[1] - REPEATS[0]) * INNER)


def measure(interpreter: Path) -> list[dict]:
    """Measure every loop and apply skeleton subtraction."""
    raw: dict[str, float] = {}
    with tempfile.TemporaryDirectory(prefix="micro_loops_") as tmp:
        for i, loop in enumerate(LOOPS):
            script = Path(tmp) / f"loop{i}.py"
            script.write_text(program(loop))
            raw[loop.name] = per_iteration(interpreter, script)
    skeleton = raw[LOOPS[0].name]
    rows = []
    for loop in LOOPS:
        value = raw[loop.name] - (skeleton if loop.subtract_skeleton else 0)
        rows.append({
            "name": loop.name,
            "raw_instructions_per_iteration": raw[loop.name],
            "instructions_per_iteration": value,
            "design_note": loop.design_note,
            "ratio_to_design_note": value / loop.design_note if loop.design_note else None,
        })
    return rows


def table(rows: list[dict]) -> str:
    """Format rows as a fixed-width table."""
    lines = [f"{'loop':38} {'measured':>9} {'note':>6} {'ratio':>6}"]
    for r in rows:
        ratio = r["ratio_to_design_note"]
        flag = "" if ratio is None or abs(ratio - 1) <= 0.05 else "  >5% off"
        lines.append(f"{r['name']:38} {r['instructions_per_iteration']:9.1f} "
                     f"{r['design_note']:6} {ratio:6.3f}{flag}")
    return "\n".join(lines)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--interpreter", type=Path, default=DEFAULT_INTERPRETER)
    ap.add_argument("--json", type=Path, help="write JSON here (default: stdout)")
    args = ap.parse_args()
    rows = measure(args.interpreter)
    print(table(rows), file=sys.stderr)
    text = json.dumps({"interpreter": str(args.interpreter), "unit": "instructions:u per iteration",
                       "loops": rows}, indent=2)
    if args.json:
        args.json.write_text(text + "\n")
    else:
        print(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
