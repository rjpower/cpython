"""The return of a frame clears the slots that can own and the constant block.

A frame's constant slots (CO_FAST_CONST) hold a copy of the values that the
code's image lists, and never own them: co_consts keeps them alive and the
frame holds the code.  The return NULLs the block without a test, closes
everything else, and leaves every slot of the popped frame NULL.  A constant
that is mortal, or a block of more than eight, takes the same path.
"""

import asyncio
import gc
import sys
import threading
import types
import unittest
import weakref

from test.support import cpython_only, gc_collect
from test.support.ir_dis import IRCode


class Tracker:
    """An owner that records its death."""
    deaths = []

    def __init__(self, name):
        self.name = name

    def __del__(self):
        Tracker.deaths.append(self.name)


def const_slot_indexes(code):
    """The co_consts index of each constant slot of code."""
    consts = list(code.co_consts)
    return tuple(consts.index(value) for value in IRCode(code).const_values)


def with_mortal_constant(function, mortal):
    """function with the constant of its first constant slot replaced."""
    code = function.__code__
    slots = const_slot_indexes(code)
    assert slots, "the function has no constant slot"
    consts = list(code.co_consts)
    consts[slots[0]] = mortal
    return types.FunctionType(code.replace(co_consts=tuple(consts)),
                              function.__globals__)


@cpython_only
class FrameReturnTests(unittest.TestCase):
    def setUp(self):
        Tracker.deaths.clear()

    def test_frame_object_after_return(self):
        holder = []

        def f(a, b):
            c = a + 1
            d = b * 2 - 3
            holder.append(sys._getframe())
            return c + d + 100 + 200 + 300

        self.assertEqual(f(1, 2), 603)
        frame = holder[0]
        self.assertIs(frame.f_code, f.__code__)
        for name, value in (("a", 1), ("b", 2), ("c", 2), ("d", 1)):
            self.assertEqual(frame.f_locals[name], value)

    def test_finished_frame_locals_exclude_constants(self):
        def f(x):
            y = (x, "kept")
            t = Tracker("local")
            frame = sys._getframe()
            return frame, 5 + 6 + 7 + 8

        frame, value = f(3)
        self.assertEqual(value, 26)
        self.assertEqual(set(frame.f_locals), {"x", "y", "t", "frame"})
        self.assertEqual(frame.f_locals["y"], (3, "kept"))
        del frame
        gc_collect()
        self.assertIn("local", Tracker.deaths)

    def test_traceback_frames(self):
        def inner(a):
            b = a * 11 + 22 + 33 + 44 + 55
            raise ValueError(b)

        try:
            inner(2)
        except ValueError as exc:
            tb = exc.__traceback__
        frame = tb.tb_next.tb_frame
        del tb
        self.assertEqual(frame.f_locals["a"], 2)
        self.assertEqual(frame.f_locals["b"], 2 * 11 + 22 + 33 + 44 + 55)

    def test_generators(self):
        def g(n):
            t = Tracker("gen%d" % n)
            yield 1 + 2 + 3
            yield 4 + 5 + 6
            if n:
                raise KeyError(n)
            return 7 + 8 + 9

        self.assertEqual(list(g(0)), [6, 15])
        it = g(1)
        next(it)
        next(it)
        with self.assertRaises(KeyError):
            next(it)
        self.assertIsNone(it.gi_frame)
        it = g(2)
        next(it)
        it.close()
        it = g(0)
        next(it)
        next(it)
        with self.assertRaises(StopIteration) as cm:
            next(it)
        self.assertEqual(cm.exception.value, 24)
        gc_collect()
        self.assertTrue({"gen0", "gen1", "gen2"} <= set(Tracker.deaths))

    def test_coroutines_and_async_generators(self):
        async def co(n):
            t = Tracker("co%d" % n)
            await asyncio.sleep(0)
            if n:
                raise OSError(n)
            return 1 + 2 + 3 + 4

        async def ag(n):
            t = Tracker("ag%d" % n)
            yield 10 + 20 + 30
            await asyncio.sleep(0)
            if n:
                raise OSError(n)
            yield 40 + 50 + 60

        async def main():
            self.assertEqual(await co(0), 10)
            with self.assertRaises(OSError):
                await co(1)
            self.assertEqual([x async for x in ag(0)], [60, 150])
            with self.assertRaises(OSError):
                async for _ in ag(1):
                    pass
            agen = ag(2)
            await agen.__anext__()
            await agen.aclose()

        asyncio.run(main())
        gc_collect()
        self.assertTrue({"co0", "co1", "ag0", "ag1", "ag2"}
                        <= set(Tracker.deaths))

    def test_settrace_return(self):
        events = []

        def tracer(frame, event, arg):
            if event == "return" and frame.f_code.co_name == "traced":
                events.append((arg, dict(frame.f_locals)))
            return tracer

        def traced(a):
            b = a + 1000 + 2000 + 3000
            return b

        sys.settrace(tracer)
        try:
            traced(1)
        finally:
            sys.settrace(None)
        self.assertEqual(events, [(6001, {"a": 1, "b": 6001})])

    def test_monitoring_return(self):
        mon = sys.monitoring
        tool = mon.PROFILER_ID
        seen = []

        def on_return(code, offset, retval):
            if code.co_name == "target":
                seen.append(retval)

        def target(a):
            return a + 1 + 2 + 3

        mon.use_tool_id(tool, "test_ir_frame_return")
        try:
            mon.register_callback(tool, mon.events.PY_RETURN, on_return)
            mon.set_events(tool, mon.events.PY_RETURN)
            for i in range(5):
                target(i)
        finally:
            mon.set_events(tool, 0)
            mon.register_callback(tool, mon.events.PY_RETURN, None)
            mon.free_tool_id(tool)
        self.assertEqual(seen, [6, 7, 8, 9, 10])

    def test_recursion_across_stack_chunks(self):
        def rec(n):
            if n == 0:
                return 1
            k = 7
            j = k * 3 + 11
            return rec(n - 1) + 1 + (j - j) + (k - k)

        def runaway(n):
            return runaway(n + 1) + 1 + 2 + 3

        limit = sys.getrecursionlimit()
        sys.setrecursionlimit(30000)
        try:
            self.assertEqual(rec(500), 501)
            self.assertEqual(rec(20000), 20001)
            with self.assertRaises(RecursionError):
                runaway(0)
            self.assertEqual(rec(20000), 20001)
        finally:
            sys.setrecursionlimit(limit)

    def test_gc_in_recursion(self):
        def rec(n):
            t = [n, "x" * 3]
            t.append(t)
            if n % 50 == 0:
                gc.collect()
            if n == 0:
                return 0
            return rec(n - 1) + 1 + 2 + 3 - 5

        limit = sys.getrecursionlimit()
        sys.setrecursionlimit(5000)
        try:
            self.assertEqual(rec(1000), 1000)
        finally:
            sys.setrecursionlimit(limit)

    def test_cycle_collected_during_a_call(self):
        def inner():
            t = Tracker("cycle")
            t.me = t
            del t
            gc.collect()
            return 1 + 2 + 3 + 4

        def outer(x):
            cell = [Tracker("outer"), x]
            cell.append(cell)
            return inner() + len(cell) + 5 + 6

        for _ in range(3):
            Tracker.deaths.clear()
            self.assertEqual(outer(0), 10 + 3 + 11)
            gc_collect()
            self.assertEqual(sorted(Tracker.deaths), ["cycle", "outer"])

    def test_frame_taken_mid_call_is_read_after_return(self):
        def leaf(a, b):
            return sys._getframe(1), a + b + 1000 + 2000

        def middle(x, y):
            z = x * y
            caller, value = leaf(z, 5)
            # The caller frame is read while this call is running
            seen = dict(caller.f_locals)
            return caller, seen, value

        caller, seen, value = middle(3, 4)
        self.assertEqual(value, 3017)
        self.assertEqual(seen["x"], 3)
        self.assertEqual(seen["z"], 12)
        self.assertEqual(caller.f_locals["y"], 4)
        self.assertEqual(caller.f_lineno, caller.f_code.co_firstlineno + 5)

    def test_errors_at_each_stage_of_a_call(self):
        def two(a, b):
            return a + b + 100 + 200

        def raises(a, b):
            raise KeyError(a)

        def deep(n, f):
            return f(n, n) + 7 + 8 + 9

        stack = sys._getframe()
        for _ in range(3):
            # A callee that raises after its frame is built
            with self.assertRaises(KeyError):
                deep(1, raises)
            # Wrong argument counts and keywords do not reach a frame
            with self.assertRaises(TypeError):
                two(1)
            with self.assertRaises(TypeError):
                two(1, 2, 3)
            with self.assertRaises(TypeError):
                two(1, b=2, a=3)
            with self.assertRaises(TypeError):
                two(1, c=2)
            with self.assertRaises(TypeError):
                two(*(1,))
            with self.assertRaises(TypeError):
                two(1, **{"a": 2})
            self.assertEqual(deep(1, two), 1 + 1 + 300 + 24)
        self.assertIs(sys._getframe(), stack)

    def test_constant_counts(self):
        # From none to more than the eight constants that an image holds,
        # with an odd and an even count of image slots.
        for n in range(0, 13):
            src = "def f(a):\n    r = a\n"
            for i in range(n):
                src += "    r = r + %d.5 * 0 + %d\n" % (i, i + 1000)
            src += "    return r\n"
            ns = {}
            exec(src, ns)
            f = ns["f"]
            self.assertEqual(f(0), sum(range(1000, 1000 + n)), n)
            self.assertEqual(f(1), 1 + sum(range(1000, 1000 + n)), n)

    def test_mortal_constant_is_not_counted(self):
        def g(a):
            return a + 1000     # beyond LOAD_SMALL_INT: a real constant

        mortal = 10 ** 30 + 12345       # a heap int, never immortal
        h = with_mortal_constant(g, mortal)
        base = sys.getrefcount(mortal)
        for i in range(100):
            self.assertEqual(h(i), i + mortal)
        self.assertEqual(sys.getrefcount(mortal), base)

    def test_mortal_constants_in_collected_generators(self):
        # Constants that the collector could track (a tuple of lists) sit in
        # the frames of several live generators of the same code: a visit of
        # those slots would subtract counts that they do not hold.
        def gen(n):
            yield 7000007
            yield n

        shared = ([1], [2])
        code = gen.__code__
        consts = list(code.co_consts)
        consts[consts.index(7000007)] = shared
        replaced = types.FunctionType(code.replace(co_consts=tuple(consts)),
                                      gen.__globals__)
        generators = [replaced(i) for i in range(4)]
        for g in generators:
            self.assertIs(next(g), shared)
        for _ in range(3):
            gc_collect()
        self.assertEqual([next(g) for g in generators], [0, 1, 2, 3])
        del generators
        gc_collect()
        self.assertEqual(shared, ([1], [2]))

    def test_mortal_constant_with_frame_object(self):
        def k(a):
            frame = sys._getframe()
            return frame, a + 1000

        mortal = 10 ** 30 + 54321
        kh = with_mortal_constant(k, mortal)
        base = sys.getrefcount(mortal)
        frame, result = kh(1)
        self.assertEqual(result, 1 + mortal)
        self.assertEqual(frame.f_locals["a"], 1)
        del frame
        gc_collect()
        self.assertEqual(sys.getrefcount(mortal), base)

    def test_discard_slot_and_free_variables_own(self):
        def ret():
            return Tracker("discard")

        def make():
            cell = Tracker("cell")

            def inner(x):
                cell.name
                ret()               # the result lands in the discard slot
                return x + 1 + 2 + 3 + 4 + 5 + 6 + 7
            return inner

        f = make()
        self.assertEqual(f(1), 29)
        self.assertIn("discard", Tracker.deaths)
        self.assertNotIn("cell", Tracker.deaths)
        del f
        gc_collect()
        self.assertIn("cell", Tracker.deaths)

    def test_finalizer_runs_while_the_frame_retires(self):
        seen = []

        class Spy:
            def __del__(self):
                seen.append(sys._getframe(1).f_code.co_name)

        def victim():
            a = Spy()
            b = 1 + 2 + 3
            c = Spy()
            return b + 4 + 5 + 6

        self.assertEqual(victim(), 21)
        self.assertEqual(len(seen), 2)

    def test_exceptions_through_frames_in_threads(self):
        out = []

        def work(n):
            def r(k):
                if k == 0:
                    raise RuntimeError("bottom")
                try:
                    return r(k - 1) + 10 + 20
                finally:
                    out.append(k)
            with self.assertRaises(RuntimeError):
                r(n)

        threads = [threading.Thread(target=work, args=(20,))
                   for _ in range(4)]
        for t in threads:
            t.start()
        for t in threads:
            t.join()
        self.assertEqual(len(out), 80)


if __name__ == "__main__":
    unittest.main()
