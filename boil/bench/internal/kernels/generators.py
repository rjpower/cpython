# Copied from pyperformance 1.14.0, benchmarks/bm_generators/run_benchmark.py,
# under the MIT licence (see LICENSE in boil/bench/internal/kernels).
# pyperf removed: run(loops) does the benchmark's timed work loops times
# and setup() does the untimed preparation; workload sizes are
# pyperformance's.
"""
Benchmark recursive generators implemented in python
by traversing a binary tree.

Author: Kumar Aditya
"""

from __future__ import annotations

from collections.abc import Iterator


class Tree:
    def __init__(self, left: Tree | None, value: int, right: Tree | None) -> None:
        self.left = left
        self.value = value
        self.right = right

    def __iter__(self) -> Iterator[int]:
        if self.left:
            yield from self.left
        yield self.value
        if self.right:
            yield from self.right


def tree(input: range) -> Tree | None:
    n = len(input)
    if n == 0:
        return None
    i = n // 2
    return Tree(tree(input[:i]), input[i], tree(input[i + 1:]))

def bench_generators(loops: int, iterable) -> None:
    range_it = range(loops)
    for _ in range_it:
        for _ in iterable:
            pass


def setup(variant):
    global ITERABLE
    assert list(tree(range(10))) == list(range(10))
    ITERABLE = tree(range(100000))


def run(loops):
    bench_generators(loops, ITERABLE)
