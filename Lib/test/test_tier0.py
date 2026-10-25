"""Tests for tier 0, the baseline compiler of IR blocks to machine code
(boil/design/05-tier0-jit.md)."""

import _thread
import dis
import math
import operator
import sys
import threading
import unittest
from test.support import import_helper

_testinternalcapi = import_helper.import_module("_testinternalcapi")
if not hasattr(_testinternalcapi, "tier0_compile"):
    raise unittest.SkipTest("requires a build configured --with-tier0")

tier0_compile = _testinternalcapi.tier0_compile
has_code = _testinternalcapi.tier0_has_code

TOOL = sys.monitoring.DEBUGGER_ID


def fresh(source, name, **namespace):
    """A function with a code object of its own, which nothing has run."""
    exec(source, namespace)
    return namespace[name]


def compiled(func):
    """func, after compiling its code."""
    if not tier0_compile(func.__code__):
        raise AssertionError(f"{func.__name__} did not compile")
    return func


LOOP = """
def loop(n):
    total = 0
    for i in range(n):
        total += i
    return total
"""


class ThresholdTests(unittest.TestCase):

    def test_calls(self):
        f = fresh("def f(x):\n    return x + 1\n", "f")
        for _ in range(10):
            f(1)
        self.assertFalse(has_code(f.__code__))
        for _ in range(100):
            self.assertEqual(f(1), 2)
        self.assertTrue(has_code(f.__code__))

    def test_backward_jumps(self):
        f = fresh(LOOP, "loop")
        self.assertEqual(f(100), 4950)
        self.assertFalse(has_code(f.__code__))
        # Compiles in the loop and goes on in machine code.
        self.assertEqual(f(3000), 3000 * 2999 // 2)
        self.assertTrue(has_code(f.__code__))
        self.assertEqual(f(10), 45)

    def test_compile(self):
        f = fresh(LOOP, "loop")
        self.assertTrue(tier0_compile(f.__code__))
        self.assertTrue(has_code(f.__code__))
        self.assertEqual(f(0), 0)
        self.assertEqual(f(5), 10)


class IntTests(unittest.TestCase):

    def test_arithmetic(self):
        def f(n):
            s = 0
            d = 0
            p = 1
            for i in range(n):
                s = s + i
                d = d - i
                p = p * 3 % 1000003
            return s, d, p
        f(5)
        compiled(f)
        for n in (0, 1, 10, 5000):
            with self.subTest(n=n):
                s = n * (n - 1) // 2
                self.assertEqual(f(n), (s, -s, pow(3, n, 1000003)))

    def test_overflow(self):
        def add(x, n):
            for i in range(n):
                x = x + 1
            return x
        def sub(x, n):
            for i in range(n):
                x = x - 1
            return x
        def mul(x, n):
            for i in range(n):
                x = x * 2
            return x
        for func in (add, sub, mul):
            func(1, 3)
            compiled(func)
        self.assertEqual(add(2**31 - 5, 10), 2**31 + 5)
        self.assertEqual(sub(-2**31 + 5, 10), -2**31 - 5)
        self.assertEqual(mul(1, 40), 2**40)
        self.assertEqual(mul(-3, 33), -3 * 2**33)
        self.assertEqual(add(2**100, 3), 2**100 + 3)

    def test_range(self):
        def f(start, stop, step):
            out = []
            for i in range(start, stop, step):
                out.append(i)
            return out
        f(0, 3, 1)
        compiled(f)
        cases = [(0, 0, 1), (0, 10, 1), (10, 0, -3), (2**31 - 3, 2**31 + 3, 1),
                 (-2**31 - 2, -2**31 + 2, 1), (2**40, 2**40 + 4, 2),
                 (5, -5, -1)]
        for args in cases:
            with self.subTest(args=args):
                self.assertEqual(f(*args), list(range(*args)))

    def test_types_change_mid_loop(self):
        def f(values):
            total = 0
            for v in values:
                total = total + v * v
            return total
        def reference(values):
            total = 0
            for v in values:
                total = operator.add(total, operator.mul(v, v))
            return total
        f([1, 2])
        compiled(f)
        values = list(range(50)) + [0.5] * 50 + [2**40] + list(range(50))
        self.assertEqual(f(values), reference(values))
        self.assertEqual(f([True, 3, -2**31]), 1 + 9 + 2**62)

    def test_while_compare(self):
        def f(n):
            i = 0
            count = 0
            while i < n:
                if i >= 3:
                    count += 1
                if i == 7 or i != i:
                    count += 100
                i += 1
            return count
        f(10)
        compiled(f)
        self.assertEqual(f(0), 0)
        self.assertEqual(f(5), 2)
        self.assertEqual(f(1000), 997 + 100)


COMPARES = (operator.lt, operator.le, operator.eq, operator.ne, operator.gt,
            operator.ge)


def compare_all(values, other):
    out = []
    for v in values:
        out.append((v < other, v <= other, v == other, v != other,
                    v > other, v >= other))
    return out


class CompareTests(unittest.TestCase):

    def check(self, values, others):
        for other in others:
            expected = [tuple(op(v, other) for op in COMPARES) for v in values]
            self.assertEqual(compare_all(values, other), expected)

    def test_compare(self):
        compare_all([1, 2], 1)
        compiled(compare_all)
        ints = [-2**31, -1, 0, 1, 2, 2**31 - 1]
        self.check(ints, [0, 1, -2**31, 2**31 - 1, 2**31, 0.5])
        floats = [-math.inf, -1.5, -0.0, 0.0, 1e-300, 2.5, math.inf, math.nan]
        self.check(floats, [0.0, -0.0, 2.5, math.inf, math.nan, 1])
        # The types change at the compiled sites.
        self.check(ints + floats + [2**70], [3, 3.0])


class FloatTests(unittest.TestCase):

    def test_arithmetic(self):
        def f(values, scale):
            acc = 0.0
            for v in values:
                acc = acc + v * scale - v
            return acc
        def reference(values, scale):
            acc = 0.0
            for v in values:
                acc = operator.sub(operator.add(acc, operator.mul(v, scale)), v)
            return acc
        values = [i * 0.1 for i in range(2000)]
        f(values[:3], 1.5)
        compiled(f)
        for scale in (1.5, -0.25, 1e300, 3):
            with self.subTest(scale=scale):
                self.assertEqual(f(values, scale), reference(values, scale))

    def test_special_values(self):
        def f(a, b):
            for i in range(3):
                s = a + b
                d = a - b
                p = a * b
            return s, d, p
        f(1.0, 2.0)
        compiled(f)
        s, d, p = f(math.inf, -math.inf)
        self.assertTrue(math.isnan(s))
        self.assertEqual(d, math.inf)
        self.assertEqual(p, -math.inf)
        s, d, p = f(-0.0, -0.0)
        self.assertEqual(math.copysign(1, s), -1)
        self.assertEqual(math.copysign(1, d), 1)
        self.assertEqual(math.copysign(1, p), 1)
        s, d, p = f(1e308, 1e308)
        self.assertEqual((s, d, p), (math.inf, 0.0, math.inf))
        self.assertTrue(math.isnan(f(math.nan, 1.0)[0]))
        self.assertEqual(f(1.5, 2), (3.5, -0.5, 3.0))


class BranchTests(unittest.TestCase):

    def test_bool_locals(self):
        def f(n):
            evens = odds = 0
            flag = True
            for i in range(n):
                if flag:
                    evens += 1
                else:
                    odds += 1
                flag = not flag
            return evens, odds
        f(3)
        compiled(f)
        self.assertEqual(f(0), (0, 0))
        self.assertEqual(f(7), (4, 3))
        self.assertEqual(f(2001), (1001, 1000))

    def test_truthiness_changes(self):
        def f(values):
            count = 0
            for v in values:
                if v:
                    count += 1
            return count
        f([True, False])
        compiled(f)
        self.assertEqual(f([True, False] * 100), 100)
        self.assertEqual(f([True, 0, 1, "", "x", None, [], [0], 0.0, 2.5]), 5)


class AttributeTests(unittest.TestCase):

    def test_attributes(self):
        class P:
            def __init__(self, x, y):
                self.x = x
                self.y = y
        class Q:
            __slots__ = ("x", "y")
            def __init__(self, x, y):
                self.x = x
                self.y = y
        def f(objs):
            total = 0
            for o in objs:
                total += o.x * o.y
            return total
        ps = [P(i, 2) for i in range(100)]
        f(ps)
        compiled(f)
        self.assertEqual(f(ps), 9900)
        mixed = ps + [Q(i, 3) for i in range(10)] + [P(1.5, 2)]
        self.assertEqual(f(mixed), 9900 + 135 + 3.0)
        ps[0].x = 1000
        self.assertEqual(f(ps), 9900 + 2000)


class ReferenceTests(unittest.TestCase):

    def test_counts(self):
        def f(n, obj):
            b = None
            for i in range(n):
                a = obj
                b = a
                a = None
                [obj, b]
            return b
        obj = object()
        f(3, obj)
        compiled(f)
        before = sys.getrefcount(obj)
        self.assertIs(f(1000, obj), obj)
        self.assertEqual(sys.getrefcount(obj), before)

    def test_destructor_line(self):
        lines = []
        class D:
            def __del__(self):
                lines.append(sys._getframe(1).f_lineno)
        def f(n):
            x = None
            for i in range(n):
                x = D()
            x = None
        f(2)
        compiled(f)
        lines.clear()
        f(10)
        first = f.__code__.co_firstlineno
        self.assertEqual(lines, [first + 3] * 9 + [first + 4])


class InstrumentationTests(unittest.TestCase):

    def tearDown(self):
        sys.settrace(None)
        if sys.monitoring.get_tool(TOOL) is not None:
            sys.monitoring.set_events(TOOL, 0)
            sys.monitoring.free_tool_id(TOOL)

    # The functions start instrumentation at iteration k of their loop.
    # Instrumentation discards code for good after a few compiles, so each
    # test makes a code object of its own.

    def test_monitoring_mid_loop(self):
        events = []
        def start(code):
            sys.monitoring.use_tool_id(TOOL, "test_tier0")
            sys.monitoring.register_callback(
                TOOL, sys.monitoring.events.LINE,
                lambda code, line: events.append(line))
            sys.monitoring.set_local_events(TOOL, code,
                                            sys.monitoring.events.LINE)
        f = fresh("""if 1:
            def f(n, k):
                total = 0
                for i in range(n):
                    if i == k:
                        start(f.__code__)
                    total += i
                return total
            """, "f", start=start)
        f(3, -1)
        compiled(f)
        self.assertEqual(f(100, 60), 4950)
        self.assertFalse(has_code(f.__code__))
        line = f.__code__.co_firstlineno + 5
        self.assertEqual(events.count(line), 100 - 60)

    def test_instruction_events_mid_loop(self):
        # INSTRUCTION events lower the block again, one word per
        # instruction, while the frame runs its machine code.
        events = []
        def start(code):
            sys.monitoring.use_tool_id(TOOL, "test_tier0")
            sys.monitoring.register_callback(
                TOOL, sys.monitoring.events.INSTRUCTION,
                lambda code, offset: events.append(offset))
            sys.monitoring.set_local_events(TOOL, code,
                                            sys.monitoring.events.INSTRUCTION)
        f = fresh("""if 1:
            def f(n, k):
                total = 0
                for i in range(n):
                    if i == k:
                        start(f.__code__)
                    total += i
                return total
            """, "f", start=start)
        f(3, -1)
        compiled(f)
        self.assertEqual(f(100, 60), 4950)
        self.assertFalse(has_code(f.__code__))
        [add] = [ins.offset for ins in dis.get_instructions(f)
                 if ins.opname == "BINARY_OP"]
        self.assertEqual(events.count(add), 100 - 60)

    def test_settrace_mid_loop(self):
        events = []
        def tracer(frame, event, arg):
            if event == "line" and frame.f_code is f.__code__:
                events.append(frame.f_lineno)
            return tracer
        f = fresh("""if 1:
            def f(n, k):
                total = 0
                for i in range(n):
                    if i == k:
                        sys._getframe().f_trace = tracer
                        sys.settrace(tracer)
                    total += i
                sys.settrace(None)
                return total
            """, "f", sys=sys, tracer=tracer)
        f(3, -1)
        compiled(f)
        self.assertEqual(f(50, 20), 1225)
        line = f.__code__.co_firstlineno + 6
        self.assertEqual(events.count(line), 50 - 20)
        # Without the tracer the code compiles again.
        self.assertTrue(tier0_compile(f.__code__))
        self.assertEqual(f(50, -1), 1225)

    def test_recompile_limit(self):
        f = fresh(LOOP, "loop")
        for _ in range(10):
            tier0_compile(f.__code__)
            sys.monitoring.use_tool_id(TOOL, "test_tier0")
            sys.monitoring.set_local_events(TOOL, f.__code__,
                                            sys.monitoring.events.LINE)
            sys.monitoring.set_local_events(TOOL, f.__code__, 0)
            sys.monitoring.free_tool_id(TOOL)
            self.assertEqual(f(10), 45)
        self.assertFalse(tier0_compile(f.__code__))
        self.assertEqual(f(3000), 3000 * 2999 // 2)


class ControlFlowTests(unittest.TestCase):

    def test_exceptions(self):
        def f(n):
            caught = 0
            for i in range(n):
                try:
                    1 // (i % 3)
                except ZeroDivisionError:
                    caught += 1
            return caught
        f(3)
        compiled(f)
        self.assertEqual(f(3000), 1000)

    def test_exception_leaves_frames(self):
        def inner(x):
            return 1 // x
        def outer(n):
            total = 0
            for i in range(n):
                total += inner(n - 1 - i)
            return total
        self.assertRaises(ZeroDivisionError, outer, 3)
        compiled(inner)
        compiled(outer)
        try:
            outer(100)
        except ZeroDivisionError as e:
            tb = e.__traceback__.tb_next
        else:
            self.fail("no exception")
        self.assertEqual(tb.tb_frame.f_code, outer.__code__)
        self.assertEqual(tb.tb_lineno, outer.__code__.co_firstlineno + 3)
        self.assertEqual(tb.tb_next.tb_lineno, inner.__code__.co_firstlineno + 1)

    def test_generator(self):
        def gen(n):
            total = 0
            for i in range(n):
                sent = yield i * 2
                if sent is not None:
                    total += sent
            return total
        list(gen(3))
        compiled(gen)
        self.assertEqual(list(gen(2000)), [i * 2 for i in range(2000)])
        g = gen(10)
        self.assertEqual(next(g), 0)
        self.assertEqual(g.send(5), 2)
        self.assertEqual(g.send(6), 4)
        with self.assertRaises(ValueError):
            g.throw(ValueError)
        g = gen(10)
        next(g)
        g.close()
        try:
            g = gen(3)
            next(g)
            g.send(1)
            g.send(2)
            g.send(3)
        except StopIteration as e:
            self.assertEqual(e.value, 6)
        else:
            self.fail("generator did not finish")

    def test_recursion(self):
        def fib(n):
            return n if n < 2 else fib(n - 1) + fib(n - 2)
        fib(5)
        compiled(fib)
        self.assertEqual(fib(20), 6765)
        def down(n):
            return 0 if n == 0 else 1 + down(n - 1)
        down(5)
        compiled(down)
        self.assertEqual(down(500), 500)
        with self.assertRaises(RecursionError):
            down(10**6)
        self.assertEqual(down(100), 100)

    def test_interrupt_inline_loop(self):
        # The loop compiles to inline code but for the eval breaker check.
        def spin():
            i = 0
            while i >= 0:
                i = i + 1 - 1
        timer = threading.Timer(0.1, _thread.interrupt_main)
        timer.start()
        try:
            with self.assertRaises(KeyboardInterrupt):
                spin()
        finally:
            timer.cancel()
            timer.join()
        self.assertTrue(has_code(spin.__code__))


if __name__ == "__main__":
    unittest.main()
