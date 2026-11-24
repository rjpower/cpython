"""Calls and returns through the register instructions Call and Return.

Each test forces an arm on the word of a stack op
(_testinternalcapi.force_arm): the operands of the stack op's word are the
family's, so the arm runs on them as the translator will select it.  The
family's definitions are Python/register_ops/calls.rop.
"""

import dis
import gc
import sys
import unittest
import weakref

from test.support import cpython_only, import_helper

_testinternalcapi = import_helper.import_module("_testinternalcapi")


def word_names(code):
    return [row[0].split("+")[0] for row in _testinternalcapi.get_ir(code, True)]


def force(function, opname, arm, which=0):
    """Run arm on the word of the which-th stack op opname of function"""
    offsets = [i.offset for i in dis.get_instructions(function)
               if i.opname == opname]
    _testinternalcapi.force_arm(function.__code__, offsets[which], arm)


def warm(function, *args):
    for _ in range(100):
        function(*args)


class Tracker:
    deaths = []

    def __init__(self, name):
        self.name = name

    def __del__(self):
        Tracker.deaths.append(self.name)


@cpython_only
class ReturnTests(unittest.TestCase):
    def setUp(self):
        Tracker.deaths.clear()

    def test_ordinary(self):
        def f(x, y):
            z = [x, y]
            return z[0] + z[1]
        warm(f, 1, 2)
        force(f, "RETURN_VALUE", "Return.Ordinary")
        self.assertEqual(f(1, 2), 3)
        self.assertEqual(f("a", "b"), "ab")

    def test_ordinary_returns_a_local(self):
        def f(x):
            y = x
            return y
        warm(f, 1)
        force(f, "RETURN_VALUE", "Return.Ordinary")
        value = object()
        before = sys.getrefcount(value)
        self.assertIs(f(value), value)
        self.assertEqual(sys.getrefcount(value), before)

    def test_empty(self):
        def f(x):
            return x * 2
        warm(f, 1)
        self.assertIn("RETURN_VALUE_EMPTY", word_names(f.__code__))
        force(f, "RETURN_VALUE", "Return.Empty")
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
        force(f, "RETURN_VALUE", "Return.Ordinary")
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
        force(g, "RETURN_VALUE", "Return.Ordinary")
        Tracker.deaths.clear()
        self.assertEqual(f(), 2)
        self.assertEqual(Tracker.deaths, ["old"])

    def test_generator(self):
        def gen(n):
            yield n
            return n * 2
        list(gen(1))
        force(gen, "RETURN_VALUE", "Return.Generator")
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
        force(f, "RETURN_VALUE", "Return.Ordinary")
        self.assertEqual(f(1), 2)
        frame_locals = frames[0].f_locals
        self.assertEqual((frame_locals["a"], frame_locals["b"]), (1, 2))


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
        force(f, "CALL", "Call.Exact")
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
        force(f, "CALL", "Call.Exact")
        self.assertEqual(f(c), (c, 5))

    def test_arguments_move_and_locals_stay(self):
        def g(a, b):
            return b

        def f(x):
            y = [x]
            return g(y, [x])
        warm(f, 1)
        force(f, "CALL", "Call.Exact")
        value = object()
        before = sys.getrefcount(value)
        self.assertEqual(f(value), [value])
        self.assertEqual(sys.getrefcount(value), before)

    def test_destination_was_the_function(self):
        def g(a):
            return a * 3

        def f(h, x):
            h = h(x)
            return h
        warm(f, g, 2)
        force(f, "CALL", "Call.Exact")
        self.assertEqual(f(g, 2), 6)

    def test_displaced_destination_finalizer(self):
        def g():
            return 1

        def f():
            x = Tracker("displaced")
            x = g()
            return x
        warm(f)
        force(f, "CALL", "Call.Exact", which=1)
        Tracker.deaths.clear()
        self.assertEqual(f(), 1)
        self.assertEqual(Tracker.deaths, ["displaced"])

    def test_exception_in_the_callee(self):
        def g(a):
            return 1 // a

        def f(x):
            return g(x)
        warm(f, 1)
        force(f, "CALL", "Call.Exact")
        try:
            f(0)
        except ZeroDivisionError as exc:
            tb = exc.__traceback__
        else:
            self.fail("no ZeroDivisionError")
        names = []
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
        force(f, "CALL", "Call.Exact")
        force(g, "RETURN_VALUE", "Return.Ordinary")
        self.assertEqual(f(4), 5)


@cpython_only
class CallBoundExactTests(unittest.TestCase):
    def test_bound_method(self):
        class C:
            def m(self, a):
                return (self, a)

        def f(m):
            return m(5)
        c = C()
        warm(f, c.m)
        force(f, "CALL", "Call.BoundExact")
        before = sys.getrefcount(c)
        self.assertEqual(f(c.m), (c, 5))
        self.assertEqual(sys.getrefcount(c), before)

    def test_misses_to_generic(self):
        class C:
            def m(self, a):
                return a + 1

        def g(a):
            return a * 2

        def f(m):
            return m(5)
        c = C()
        warm(f, c.m)
        force(f, "CALL", "Call.BoundExact")
        self.assertEqual(f(g), 10)
        self.assertEqual(f(lambda a: -a), -5)
        self.assertEqual(f(c.m), 6)

    def test_weakly_referenced_method_misses(self):
        class C:
            def m(self, a):
                return a

        def f(m):
            return m(1)
        c = C()
        warm(f, c.m)
        force(f, "CALL", "Call.BoundExact")
        m = c.m
        r = weakref.ref(m)
        self.assertEqual(f(m), 1)
        self.assertIs(r(), m)


class Point:
    def __init__(self, x, y):
        self.x = x
        self.y = y


@cpython_only
class CallInitTests(unittest.TestCase):
    def test_adapts_to_init(self):
        def f(a, b):
            return Point(a, b)
        warm(f, 1, 2)
        force(f, "CALL", "Call.Generic")
        warm(f, 1, 2)
        self.assertIn("Call.Init", word_names(f.__code__))
        p = f(3, 4)
        self.assertEqual((p.x, p.y), (3, 4))
        value = object()
        before = sys.getrefcount(value)
        p = f(value, 0)
        self.assertIs(p.x, value)
        del p
        self.assertEqual(sys.getrefcount(value), before)

    def test_init_returns_not_none(self):
        class Bad:
            def __init__(self):
                return 1

        def f():
            return Bad()
        force(f, "CALL", "Call.Generic")
        for _ in range(3):
            with self.assertRaisesRegex(TypeError, "should return None, not 'int'"):
                f()
        warm(lambda: self.assertRaises(TypeError, f))
        self.assertIn("Call.Init", word_names(f.__code__))
        with self.assertRaisesRegex(TypeError, "should return None, not 'int'"):
            f()

    def test_init_raises(self):
        class Raises:
            def __init__(self, fail):
                if fail:
                    raise ValueError("no")

        def f(fail):
            return Raises(fail)
        force(f, "CALL", "Call.Generic")
        warm(f, False)
        self.assertIn("Call.Init", word_names(f.__code__))
        with self.assertRaisesRegex(ValueError, "no"):
            f(True)
        self.assertIsInstance(f(False), Raises)

    def test_class_changes(self):
        class C:
            def __init__(self, a):
                self.a = a

        def f(a):
            return C(a)
        force(f, "CALL", "Call.Generic")
        warm(f, 1)
        C.__init__ = lambda self, a: setattr(self, "a", -a)
        self.assertEqual(f(2).a, -2)


@cpython_only
class CallGenericTests(unittest.TestCase):
    def setUp(self):
        Tracker.deaths.clear()

    def test_native(self):
        def f(x):
            return len(x)
        force(f, "CALL", "Call.Generic")
        value = [1, 2, 3]
        before = sys.getrefcount(value)
        self.assertEqual(f(value), 3)
        self.assertEqual(sys.getrefcount(value), before)

    def test_native_temporaries_are_released(self):
        def f(x):
            return isinstance([x], list)
        force(f, "CALL", "Call.Generic")
        value = object()
        before = sys.getrefcount(value)
        self.assertTrue(f(value))
        self.assertEqual(sys.getrefcount(value), before)

    def test_native_raises(self):
        def f(x):
            return int([x][0])
        force(f, "CALL", "Call.Generic", which=0)
        with self.assertRaises(ValueError):
            f("x")
        value = "not a number"
        before = sys.getrefcount(value)
        with self.assertRaises(ValueError):
            f(value)
        self.assertEqual(sys.getrefcount(value), before)

    def test_destination_is_an_operand(self):
        def f(x):
            x = str(x)
            return x
        force(f, "CALL", "Call.Generic")
        self.assertEqual(f(12), "12")

    def test_general_binding(self):
        def g(a, b=2, *rest, c=3, **kw):
            return (a, b, rest, c, kw)

        def f(x):
            return g(x)

        def h(x):
            return g(x, 1, 2, 3)
        force(f, "CALL", "Call.Generic")
        force(h, "CALL", "Call.Generic")
        self.assertEqual(f(1), (1, 2, (), 3, {}))
        self.assertEqual(h(1), (1, 1, (2, 3), 3, {}))

    def test_binding_error(self):
        def g(a):
            return a

        def f():
            return g()
        force(f, "CALL", "Call.Generic")
        with self.assertRaises(TypeError):
            f()

    def test_generator_function(self):
        def gen(n):
            yield n
            yield n + 1

        def f(n):
            return gen(n)
        force(f, "CALL", "Call.Generic")
        self.assertEqual(list(f(3)), [3, 4])

    def test_bound_method_with_self(self):
        class C:
            def m(self, a, b=1):
                return a + b

        def f(c):
            return c.m(5)
        c = C()
        force(f, "CALL", "Call.Generic")
        self.assertEqual(f(c), 6)

    def test_displaced_destination(self):
        def f():
            x = Tracker("displaced")
            x = abs(-1)
            return x
        force(f, "CALL", "Call.Generic", which=1)
        self.assertEqual(f(), 1)
        self.assertEqual(Tracker.deaths, ["displaced"])

    def test_adapts_to_exact(self):
        def g(a, b):
            return a + b

        def f(x):
            return g(x, 1)
        force(f, "CALL", "Call.Generic")
        warm(f, 1)
        self.assertIn("Call.Exact", word_names(f.__code__))
        self.assertEqual(f(2), 3)

    def test_adapts_to_bound_exact(self):
        class C:
            def m(self, a):
                return a

        def f(m):
            return m(4)
        force(f, "CALL", "Call.Generic")
        warm(f, C().m)
        self.assertIn("Call.BoundExact", word_names(f.__code__))
        self.assertEqual(f(C().m), 4)

    def test_native_stays_generic(self):
        def f(x):
            return len(x)
        force(f, "CALL", "Call.Generic")
        warm(f, "ab")
        self.assertIn("Call.Generic", word_names(f.__code__))

    def test_exact_misses_after_redefinition(self):
        def g(a):
            return a

        def f(x):
            return g(x)
        force(f, "CALL", "Call.Generic")
        warm(f, 1)
        self.assertIn("Call.Exact", word_names(f.__code__))
        g.__code__ = (lambda a: -a).__code__
        self.assertEqual(f(1), -1)

    def test_recursion_limit(self):
        def r(n):
            return r(n - 1) if n else 0
        force(r, "CALL", "Call.Generic")
        self.assertEqual(r(50), 0)
        with self.assertRaises(RecursionError):
            r(10 ** 6)
        force(r, "CALL", "Call.Exact")
        self.assertEqual(r(50), 0)
        with self.assertRaises(RecursionError):
            r(10 ** 6)


if __name__ == "__main__":
    unittest.main()
