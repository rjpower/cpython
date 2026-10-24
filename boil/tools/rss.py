"""Peak resident set size of the interpreter on fixed workloads.

Measures peak RSS (KiB, ``/usr/bin/time -f %M``) of a child interpreter per workload:
``startup`` (nothing), ``imports`` (json, re, collections), ``dicts``
(200,000 small dicts kept alive) and ``loops`` (every loop in micro_loops.py
once, 1000 iterations each).

Invariants: each workload runs in its own ``python -S -E`` process, so peaks
are independent. Needs GNU time at /usr/bin/time.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path

from micro_loops import DEFAULT_INTERPRETER, LOOPS, program

# Not getrusage(RUSAGE_CHILDREN): a forked child inherits its parent's RSS
# high-water mark, which would put a floor under every small workload.
TIME = "/usr/bin/time"

WORKLOADS = {
    "startup": "pass",
    "imports": "import json, re, collections",
    "dicts": "d = [{'a': i, 'b': str(i)} for i in range(200_000)]",
}


def loops_source() -> str:
    """Return a script running each micro loop once."""
    parts = []
    for n, loop in enumerate(LOOPS):
        src = program(loop).replace("import sys\n", "").replace("f(int(sys.argv[1]))", "f(1)")
        parts.append(src.replace("def f(", f"def f{n}(").replace("\nf(1)", f"\nf{n}(1)"))
    return "\n".join(parts)


def peak_rss_kib(interpreter: Path, source: str) -> int:
    """Run source in a child interpreter and return its peak RSS in KiB."""
    cmd = [TIME, "-f", "%M", str(interpreter), "-S", "-E", "-c", source]
    proc = subprocess.run(cmd, capture_output=True, text=True, check=True)
    return int(proc.stderr.strip().splitlines()[-1])


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--interpreter", type=Path, default=DEFAULT_INTERPRETER)
    ap.add_argument("--json", type=Path, help="write JSON here (default: stdout)")
    args = ap.parse_args()
    sources = {**WORKLOADS, "loops": loops_source()}
    peaks = {name: peak_rss_kib(args.interpreter, src) for name, src in sources.items()}
    for name, kib in peaks.items():
        print(f"{name:10} {kib:9d} KiB", file=sys.stderr)
    text = json.dumps({"interpreter": str(args.interpreter), "unit": "KiB", "peak_rss": peaks}, indent=2)
    if args.json:
        args.json.write_text(text + "\n")
    else:
        print(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
