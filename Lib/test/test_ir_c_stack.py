"""The interpreter runs Python control flow at a bounded C stack depth.

Dispatch, a call of a Python function, its return, an exception that
unwinds through Python frames and a generator delegation all run in the
same activation of the interpreter, without growing the C stack: in a
thread with a small C stack, a long loop and deep Python recursion
complete.  An interpreter that grew the C stack by a few bytes per
transfer would overflow it (Python/ceval_tail.h, Python/ceval.c)."""

import unittest
from test.support import script_helper

# 256 KiB of C stack, and thousands of Python frames on it
SCRIPT = """
import sys, threading
sys.setrecursionlimit(30_000)

def loop(n):
    total = 0
    for i in range(n):
        total += i
    return total

def recurse(n):
    return 0 if n == 0 else 1 + recurse(n - 1)

def raise_at(n):
    if n == 0:
        raise KeyError(n)
    return raise_at(n - 1)

def delegate(n):
    if n == 0:
        yield 1
        return 2
    return (yield from delegate(n - 1))

def run():
    global result
    caught = None
    try:
        raise_at(DEPTH)
    except KeyError as e:
        caught = e.args
    gen = delegate(DEPTH // 4)
    first = next(gen)
    try:
        next(gen)
    except StopIteration as e:
        done = e.value
    result = (loop(1_000_000), recurse(DEPTH), caught, first, done)

DEPTH = 20_000
threading.stack_size(256 * 1024)
t = threading.Thread(target=run)
t.start()
t.join()
print(result)
"""


class BoundedCStackTests(unittest.TestCase):

    def test_python_control_flow_in_a_small_c_stack(self):
        rc, out, err = script_helper.assert_python_ok("-c", SCRIPT)
        self.assertEqual(out.strip().decode(),
                         str((499999500000, 20000, (0,), 1, 2)))


if __name__ == "__main__":
    unittest.main()
