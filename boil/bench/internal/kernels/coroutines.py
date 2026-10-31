# Copied from pyperformance 1.14.0, benchmarks/bm_coroutines/run_benchmark.py,
# under the MIT licence (see LICENSE in boil/bench/internal/kernels).
# pyperf removed: run(loops) does the benchmark's timed work loops times
# and setup() does the untimed preparation; workload sizes are
# pyperformance's.
"""
Benchmark for recursive coroutines.

Author: Kumar Aditya
"""



async def fibonacci(n: int) -> int:
    if n <= 1:
        return n
    return await fibonacci(n - 1) + await fibonacci(n - 2)


def bench_coroutines(loops: int) -> None:
    range_it = range(loops)
    for _ in range_it:
        coro = fibonacci(25)
        try:
            while True:
                coro.send(None)
        except StopIteration:
            pass


def run(loops):
    bench_coroutines(loops)
