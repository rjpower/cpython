"""The collector's pause distribution over one benchmark run.

Usage (with the build under test):
  PYTHONHASHSEED=0 BUILD/python -S boil/tools/measure/gc_pauses.py \
      boil/bench/internal/driver.py run boil/bench/internal/memory/collector.py 1 1 16

Runs the driver as __main__ with the given arguments, records each
collection's duration from gc.callbacks, and prints one JSON line: the
number of collections that reported, the median, p99 and maximum in
milliseconds, and the marks the collector counted
(_testinternalcapi.gc_root_stats()), so that the collections that ran
inline, where no callback runs, are their difference.
"""

import atexit
import gc
import json
import runpy
import sys

durations = []


def callback(phase, info):
    if phase == "stop":
        durations.append(info["duration"])


def report():
    import _testinternalcapi
    marks = _testinternalcapi.gc_root_stats()["total"]["collections"]
    d = sorted(durations)
    n = len(d)

    def pct(p):
        return d[min(n - 1, int(p * n))] * 1e3 if n else 0.0

    print(json.dumps({"reported": n, "marks": marks,
                      "median_ms": round(pct(0.5), 3),
                      "p99_ms": round(pct(0.99), 3),
                      "max_ms": round(d[-1] * 1e3 if n else 0.0, 3)}))


gc.callbacks.append(callback)
atexit.register(report)
sys.argv = sys.argv[1:]
runpy.run_path(sys.argv[0], run_name="__main__")
