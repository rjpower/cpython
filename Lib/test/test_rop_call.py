"""Calls and returns through the register instructions Call and Return.

Each test forces an arm on a word that the assembler made
(_testinternalcapi.rop_set_arm): the operands of the stack op's word are the
family's, so the arm runs on them as the translator will select it.  The
family's definitions are Python/register_ops/calls.rop.
"""

import gc
import sys
import unittest
import weakref

from test.support import cpython_only, import_helper

_testinternalcapi = import_helper.import_module("_testinternalcapi")


def word_names(code):
    return [row[0].split("+")[0] for row in _testinternalcapi.get_ir(code, True)]


def force(function, prefixes, arm, which=0):
    """Run arm on the which-th word of function whose stack op starts with
    one of prefixes; the function must have run"""
    code = function.__code__
    found = [i for i, name in enumerate(word_names(code))
             if name.startswith(prefixes)]
    _testinternalcapi.rop_set_arm(code, found[which], arm)
    return found[which]


def warm(function, *args):
    for _ in range(100):
        function(*args)


class Tracker:
    deaths = []

    def __init__(self, name):
        self.name = name

    def __del__(self):
        Tracker.deaths.append(self.name)


RETURNS = ("RETURN_VALUE",)


@cpython_only
class ReturnTests(unittest.TestCase):
    def setUp(self):
        Tracker.deaths.clear()

    def test_ordinary(self):
        def f(x, y):
            z = [x, y]
            return z[0] + z[1]
        warm(f, 1, 2)
        force(f, RETURNS, "Return.Ordinary")
        self.assertEqual(f(1, 2), 3)
        self.assertEqual(f("a", "b"), "ab")

    def test_ordinary_returns_a_local(self):
        def f(x):
            y = x
            return y
        warm(f, 1)
        force(f, RETURNS, "Return.Ordinary")
        value = object()
        self.assertIs(f(value), value)
        self.assertEqual(sys.getrefcount(value), 2)

    def test_empty(self):
        def f(x):
            return x * 2
        warm(f, 1)
        self.assertIn("RETURN_VALUE_EMPTY", word_names(f.__code__))
        force(f, ("RETURN_VALUE_EMPTY",), "Return.Empty")
        self.assertEqual(f(21), 42)

    def test_finalizer_of_the_dying_frame(self):
        seen = []

        class Dying:
            def __del__(self):
                seen.append(sys._getframe(1).f_code.co_name)

        def f():
            local = Dying()
            return 7
        warm(f)
        seen.clear()
        force(f, RETURNS, "Return.Ordinary")
        self.assertEqual(f(), 7)
        self.assertEqual(len(seen), 1)

    def test_finalizer_displaced_in_the_caller(self):
        def g(n):
            return n + 1

        def f():
            x = Tracker("old")
            x = g(1)
            return x
        warm(f)
        force(g, RETURNS, "Return.Ordinary")
        Tracker.deaths.clear()
        self.assertEqual(f(), 2)
        self.assertEqual(Tracker.deaths, ["old"])

    def test_generator(self):
        def gen(n):
            yield n
            return n * 2
        list(gen(1))
        force(gen, RETURNS, "Return.Generator")
        g = gen(3)
        self.assertEqual(next(g), 3)
        with self.assertRaises(StopIteration) as cm:
            next(g)
        self.assertEqual(cm.exception.value, 6)

        def outer():
            result = yield from gen(5)
            return result
        self.assertEqual(list(outer()), [5])
        o = outer()
        next(o)
        with self.assertRaises(StopIteration) as cm:
            next(o)
        self.assertEqual(cm.exception.value, 10)
        self.assertEqual([x for x in gen(4)], [4])

    def test_frame_object_after_return(self):
        frames = []

        def f(a):
            frames.append(sys._getframe())
            b = a + 1
            return b
        warm(f, 1)
        frames.clear()
        force(f, RETURNS, "Return.Ordinary")
        self.assertEqual(f(1), 2)
        self.assertEqual(frames[0].f_locals, {"a": 1, "b": 2})


@cpython_only
class CallExactTests(unittest.TestCase):
    def setUp(self):
        Tracker.deaths.clear()

    def test_exact(self):
        def g(a, b):
            return a - b

        def f(x):
            return g(x, 1)
        warm(f, 1)
        self.assertIn("CALL_PY_EXACT_ARGS", word_names(f.__code__))
        force(f, ("CALL",), "Call.Exact")
        self.assertEqual(f(10), 9)
        self.assertEqual(f(3), 2)

    def test_exact_with_self(self):
        class C:
            def m(self, a):
                return (self, a)

        def f(c):
            return c.m(5)
        c = C()
        warm(f, c)
        force(f, ("CALL",), "Call.Exact")
        self.assertEqual(f(c), (c, 5))

    def test_arguments_move_and_locals_stay(self):
        def g(a, b):
            return b

        def f(x):
            y = [x]
            return g(y, [x])
        warm(f, 1)
        force(f, ("CALL",), "Call.Exact")
        value = object()
        self.assertEqual(f(value), [value])
        self.assertEqual(sys.getrefcount(value), 2)

    def test_destination_was_the_function(self):
        def g(a):
            return a * 3

        def f(h, x):
            h = h(x)
            return h
        warm(f, g, 2)
        force(f, ("CALL",), "Call.Exact")
        self.assertEqual(f(g, 2), 6)

    def test_displaced_destination_finalizer(self):
        def g():
            return 1

        def f():
            x = Tracker("displaced")
            x = g()
            return x
        warm(f)
        force(f, ("CALL",), "Call.Exact")
        Tracker.deaths.clear()
        self.assertEqual(f(), 1)
        self.assertEqual(Tracker.deaths, ["displaced"])

    def test_exception_in_the_callee(self):
        def g(a):
            return 1 // a

        def f(x):
            return g(x)
        warm(f, 1)
        force(f, ("CALL",), "Call.Exact")
        with self.assertRaises(ZeroDivisionError) as cm:
            f(0)
        names = []
        tb = cm.exception.__traceback__
        while tb is not None:
            names.append(tb.tb_frame.f_code.co_name)
            tb = tb.tb_next
        self.assertEqual(names[-2:], ["f", "g"])

    def test_exact_and_return_together(self):
        def g(a):
            b = a + 1
            return b

        def f(x):
            return g(x)
        warm(f, 1)
        force(f, ("CALL",), "Call.Exact")
        force(g, RETURNS, "Return.Ordinary")
        self.assertEqual(f(4), 5)


if __name__ == "__main__":
    unittest.main()
