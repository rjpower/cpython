# Copied from pyperformance 1.14.0, benchmarks/bm_regex_compile/run_benchmark.py,
# under the MIT licence (see LICENSE in boil/bench/internal/kernels).
# pyperf removed: run(loops) does the benchmark's timed work loops times
# and setup() does the untimed preparation; workload sizes are
# pyperformance's.

"""Benchmark how quickly Python's regex implementation can compile regexes.

We bring in all the regexes used by the other regex benchmarks, capture them by
stubbing out the re module, then compile those regexes repeatedly. We muck with
the re module's caching to force it to recompile every regex we give it.
"""

# Python imports
import re
import sys
from pathlib import Path

# Local imports


def capture_regexes():
    regexes = []

    real_compile = re.compile
    real_search = re.search
    real_sub = re.sub

    def capture_compile(regex, flags=0):
        regexes.append((regex, flags))
        return real_compile(regex, flags)

    def capture_search(regex, target, flags=0):
        regexes.append((regex, flags))
        return real_search(regex, target, flags)

    def capture_sub(regex, *args):
        regexes.append((regex, 0))
        return real_sub(regex, *args)

    re.compile = capture_compile
    re.search = capture_search
    re.sub = capture_sub
    try:
        import bm_regex_effbot
        bm_regex_effbot.bench_regex_effbot(1)

        import bm_regex_v8
        bm_regex_v8.bench_regex_v8(1)
    finally:
        re.compile = real_compile
        re.search = real_search
        re.sub = real_sub
    return regexes


def bench_regex_compile(loops, regexes):
    range_it = range(loops)

    for _ in range_it:
        for regex, flags in regexes:
            re.purge()
            # ignore result (compiled regex)
            re.compile(regex, flags)



def setup(variant):
    # The corpus: the regex_effbot and regex_v8 benchmarks, in data/.
    global REGEXES
    sys.path.insert(0, str(Path(__file__).resolve().parent / "data"))
    REGEXES = capture_regexes()


def run(loops):
    bench_regex_compile(loops, REGEXES)
