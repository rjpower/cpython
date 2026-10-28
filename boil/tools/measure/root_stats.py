#!/usr/bin/env python3
"""Sum the live collector's root-phase counters over many processes.

Usage:
  python3 boil/tools/measure/root_stats.py run OUT.jsonl -- CMD [ARGS...]
  python3 boil/tools/measure/root_stats.py sum OUT.jsonl [...]
  python3 boil/tools/measure/root_stats.py check OUT.jsonl

A build since 10a's rung 0 appends its main interpreter's counters
(_testinternalcapi.gc_root_stats()["total"]) to $PYTHON_GCROOTSTATS at
exit, one JSON line per process.  ``run`` truncates OUT, runs CMD with the
variable set to it, and prints the sum; ``sum`` prints the sum of existing
files.  Processes that clear their environment are not counted; the number
of lines is printed beside the sum.

The sum is printed as a markdown table: per class, the words scanned and
the words that hit an object; then the other counters, and the times in
milliseconds.  ``check`` prints one line, the stack walk's fallbacks
and the unpinned detach records visited, and exits 1 on a fallback, which
blocks a landing (boil/design/11-precise-roots.md, section 10); gate.sh
runs it after the suite at levels 2 and 3.
"""

import json
import os
import subprocess
import sys
from collections import Counter

CLASSES = ("stack", "regs", "mem", "shallow", "data", "structs")


def load(paths):
    total = Counter()
    lines = 0
    for path in paths:
        with open(path) as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                rec = json.loads(line)
                rec.pop("pid", None)
                total.update(rec)
                lines += 1
    return total, lines


def report(total, lines):
    n = total["collections"]
    print(f"processes: {lines}, collections: {n:,}")
    print()
    print("| class | words | hits | words per collection |")
    print("|---|---:|---:|---:|")
    for cls in CLASSES:
        w, h = total[f"{cls}_words"], total[f"{cls}_hits"]
        per = w / n if n else 0
        print(f"| {cls} | {w:,} | {h:,} | {per:,.0f} |")
    print()
    print("| counter | total | per collection |")
    print("|---|---:|---:|")
    for key in ("threads", "activations_skipped", "eval_words_skipped",
                "frames_walked", "cache_hits", "cache_misses",
                "cache_flushes", "fallbacks", "unpinned"):
        v = total[key]
        print(f"| {key} | {v:,} | {v / n if n else 0:,.1f} |")
    for key in ("precise_ns", "stack_ns", "data_ns", "structs_ns", "roots_ns"):
        v = total[key]
        name = key[:-3] + " ms"
        print(f"| {name} | {v / 1e6:,.1f} | {v / n / 1e6 if n else 0:,.3f} |")


def main(argv):
    if len(argv) >= 4 and argv[1] == "run" and argv[3] == "--":
        out = argv[2]
        open(out, "w").close()
        env = dict(os.environ, PYTHON_GCROOTSTATS=os.path.abspath(out))
        rc = subprocess.call(argv[4:], env=env)
        report(*load([out]))
        return rc
    if len(argv) == 3 and argv[1] == "check":
        total, lines = load(argv[2:]) if os.path.exists(argv[2]) else (Counter(), 0)
        n = total["collections"]
        print(f"roots: {lines} processes, {n:,} collections, "
              f"fallbacks {total['fallbacks']:,}, unpinned {total['unpinned']:,}"
              f" ({total['unpinned'] / n if n else 0:.2f} per collection)")
        if total["fallbacks"]:
            print("roots: a stack-walk fallback blocks a landing", file=sys.stderr)
            return 1
        return 0
    if len(argv) >= 3 and argv[1] == "sum":
        report(*load(argv[2:]))
        return 0
    print(__doc__, file=sys.stderr)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
