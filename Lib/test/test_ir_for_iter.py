"""FOR_ITER and its specializations step the cursor by value.

_FOR_ITER, _FOR_ITER_VIRTUAL, _ITER_NEXT_LIST and _ITER_NEXT_TUPLE take the
cursor as `index` and give the next one as `next_index`.  The cursor is a
tagged index for an index-protocol iterator and NULL for an object-protocol
one.  Each case warms a loop until its handler runs, then checks that every
item the loop yields is released exactly once and that a break, an iterator's
exception and an exhausted iterator leave nothing behind.  The script runs
with PYTHON_JIT off and always.
"""

import textwrap
import unittest

from test.support import import_helper
from test.support.script_helper import assert_python_ok

import_helper.import_module("_testinternalcapi")

SCRIPT = """
import _testinternalcapi as capi
import gc
import sys
import weakref

WARM = 400
COUNT = 5


class Value:
    "An object whose release a weak reference sees"
    released = []

    def __init__(self, n):
        self.n = n
        weakref.finalize(self, Value.released.append, n)


class UserIter:
    "An iterator with a Python __next__, which FOR_ITER steps generically"
    def __init__(self, items):
        self.items = list(items)
        self.i = 0

    def __iter__(self):
        return self

    def __next__(self):
        if self.i >= len(self.items):
            raise StopIteration
        self.i += 1
        return self.items[self.i - 1]


class Failing:
    def __init__(self, items):
        self.items = list(items)

    def __iter__(self):
        return self

    def __next__(self):
        if not self.items:
            raise ZeroDivisionError("the iterator failed")
        return self.items.pop()


def handlers(func):
    return {w[0] for w in capi.get_ir(func.__code__, True)}


def make_loops():
    "Fresh loops, so that each source specializes its own"
    def total(it):
        seen = 0
        for x in it:
            seen += x.n
        return seen

    def broken(it):
        for x in it:
            if x.n % 2:
                break
        return x.n
    return total, broken


def failing(it):
    try:
        for x in it:
            pass
    except ZeroDivisionError:
        return True
    return False


# Each source builds a fresh iterable of COUNT new Values; the case names the
# handler that the warmed loop must reach
SOURCES = {
    "list": (lambda vs: list(vs), "FOR_ITER_LIST"),
    "tuple": (lambda vs: tuple(vs), "FOR_ITER_TUPLE"),
    "map": (lambda vs: map(lambda v: v, vs), "FOR_ITER_VIRTUAL"),
    "iter_list": (lambda vs: iter(list(vs)), "FOR_ITER_VIRTUAL"),
    "zip": (lambda vs: (v for v, in zip(vs)), "FOR_ITER_VIRTUAL"),
    "user": (lambda vs: UserIter(vs), "FOR_ITER_VIRTUAL"),
}


def fresh(base):
    return [Value(base + n) for n in range(COUNT)]


def check(name, make, handler):
    Value.released.clear()
    total, broken = make_loops()
    for round_ in range(WARM):
        values = fresh(round_ * 10)
        assert total(make(values)) == sum(v.n for v in values)
        # break on the first odd value: the iterator and its cursor are dropped
        values = fresh(round_ * 10 + 1_000_000)
        broken(make(values))
        # the loop's own name keeps the last item; the others are released with
        # the iterable
        del values
    gc.collect()
    assert handler in handlers(total) | handlers(broken), (name, handler, sorted(handlers(total)))
    # every Value built was released once and only once
    assert len(Value.released) == len(set(Value.released)) == 2 * COUNT * WARM, (
        name, len(Value.released), len(set(Value.released)))


for name, (make, handler) in SOURCES.items():
    check(name, make, handler)


# Items an iterable still holds keep their counts: the loop adds one while the
# name holds it, and takes it back at the next step
keep = fresh(0)
before = [sys.getrefcount(v) for v in keep]
for _ in range(WARM):
    for x in keep:
        pass
    for x in tuple(keep):
        pass
    for x in map(lambda v: v, keep):
        pass
del x
assert [sys.getrefcount(v) for v in keep] == before, (before, [sys.getrefcount(v) for v in keep])

# An iterator that raises: the cursor and the iterator are released, the error
# is the iterator's
Value.released.clear()
for round_ in range(WARM):
    assert failing(Failing(fresh(round_ * 10 + 2_000_000)))
gc.collect()
assert len(Value.released) == len(set(Value.released)) == COUNT * WARM, len(Value.released)

# Dict and str iterate by index: the index protocol, a tagged cursor
d = {n: Value(n) for n in range(COUNT)}
for _ in range(WARM):
    assert sum(d) == sum(range(COUNT))
    assert "".join(c for c in "abcde") == "abcde"
    assert [v.n for v in d.values()] == list(range(COUNT))
print("ok")
"""


class ForIterTests(unittest.TestCase):
    def check(self, mode):
        _, out, _ = assert_python_ok("-c", textwrap.dedent(SCRIPT), PYTHON_JIT=mode)
        self.assertEqual(out.strip(), b"ok")

    def test_jit_off(self):
        self.check("off")

    def test_jit_always(self):
        self.check("always")


if __name__ == "__main__":
    unittest.main()
