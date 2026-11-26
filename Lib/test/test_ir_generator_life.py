"""The life of a generator: creation, resume and yield, and finish.

A generator, coroutine or async generator is created in one routine
(_PyGen_NewCall), resumed by a ForIter or Send arm or the C entry, yields to
whichever resumed it, and is cleared and freed without the finalizer protocol
once its frame is finished.
"""

import gc
import inspect
import sys
import unittest
import weakref

from test.support import cpython_only


class Noisy:
    """Records its finalization in the list it is given."""
    def __init__(self, log, name):
        self.log = log
        self.name = name

    def __del__(self):
        self.log.append(self.name)


class CreationTests(unittest.TestCase):

    def test_closure_defaults_keywords_star_arguments(self):
        k = 7

        def closure():
            yield k

        def args(a, b=2, *rest, c=3, d, **extra):
            yield (a, b, rest, c, d, sorted(extra.items()))

        def kwonly(*, p, q=1):
            yield p + q

        def star(*a, **kw):
            yield a, kw

        self.assertEqual(list(closure()), [7])
        self.assertEqual(list(args(1, d=4)), [(1, 2, (), 3, 4, [])])
        self.assertEqual(list(args(1, 5, 6, 7, c=8, d=9, z=1)),
                         [(1, 5, (6, 7), 8, 9, [('z', 1)])])
        self.assertEqual(list(kwonly(p=1)), [2])
        self.assertEqual(list(kwonly(q=5, p=2)), [7])
        self.assertEqual(list(star(1, 2, x=3)), [((1, 2), {'x': 3})])

    def test_argument_errors_create_nothing(self):
        def args(a, b=2, *, d):
            yield a

        with self.assertRaises(TypeError):
            args()
        with self.assertRaises(TypeError):
            args(1, 2, 3, d=1)
        with self.assertRaises(TypeError):
            args(1, d=1, a=2)

    def test_cell_argument(self):
        def gen(a):
            def inner():
                return a
            yield inner()
            a += 1
            yield inner()

        self.assertEqual(list(gen(5)), [5, 6])

    def test_genexpr_free_variables(self):
        free = 5
        pool = (10, 20, 30)
        self.assertEqual(list(x + free for x in range(3)), [5, 6, 7])
        self.assertEqual(tuple(pool[i] for i in range(3)), pool)
        self.assertEqual(list((x, y) for x in range(2) for y in range(2)),
                         [(0, 0), (0, 1), (1, 0), (1, 1)])

    def test_genexpr_over_a_failing_iterable_raises_at_creation(self):
        class Failing:
            def __iter__(self):
                raise RuntimeError("iter fails")

        with self.assertRaisesRegex(RuntimeError, "iter fails"):
            (x for x in Failing())
        with self.assertRaises(TypeError):
            (x for x in 5)
        with self.assertRaises(TypeError):
            (x for x in None)

    def test_names_and_plainness(self):
        def named():
            yield

        g = named()
        self.assertEqual(g.__name__, "named")
        self.assertEqual(g.__qualname__, named.__qualname__)
        g.__name__ = "renamed"
        g.__qualname__ = "q.renamed"
        self.assertEqual((g.__name__, g.__qualname__), ("renamed", "q.renamed"))
        del g
        gc.collect()

    def test_coroutine_origin(self):
        async def coro():
            return 1

        self.assertIsNone(coro().cr_origin)
        sys.set_coroutine_origin_tracking_depth(2)
        try:
            c = coro()
            self.assertIsNotNone(c.cr_origin)
            c.close()
        finally:
            sys.set_coroutine_origin_tracking_depth(0)
        c = coro()
        self.assertIsNone(c.cr_origin)
        c.close()

    def test_async_generator(self):
        async def agen():
            yield 1
            yield 2

        ag = agen()
        steps = []
        try:
            ag.__anext__().send(None)
        except StopIteration as e:
            steps.append(e.value)
        try:
            ag.asend(None).send(None)
        except StopIteration as e:
            steps.append(e.value)
        self.assertEqual(steps, [1, 2])
        with self.assertRaises(StopAsyncIteration):
            ag.__anext__().send(None)


class ResumeAndYieldTests(unittest.TestCase):

    @staticmethod
    def two_step():
        x = yield 1
        try:
            y = yield (2, x)
        except ValueError as e:
            y = yield ("caught", str(e))
        finally:
            pass
        return y

    def test_consumers_alternate(self):
        def gen():
            for i in range(8):
                r = yield i
                if r is not None:
                    yield ("sent", r)

        g = gen()
        self.assertEqual(next(g), 0)
        self.assertEqual(g.send("a"), ("sent", "a"))
        self.assertEqual(next(g), 1)
        seen = []
        for x in g:
            seen.append(x)
            if x == 3:
                break
        self.assertEqual(seen, [2, 3])
        self.assertEqual(g.send(None), 4)
        self.assertEqual(list(g), [5, 6, 7])
        self.assertEqual(inspect.getgeneratorstate(g), inspect.GEN_CLOSED)

    def test_send_and_throw(self):
        g = self.two_step()
        self.assertEqual(next(g), 1)
        self.assertEqual(g.send("a"), (2, "a"))
        self.assertEqual(g.throw(ValueError("v")), ("caught", "v"))
        with self.assertRaises(StopIteration) as cm:
            g.send("z")
        self.assertEqual(cm.exception.value, "z")

    def test_close_and_throw_at_each_suspension(self):
        for steps in range(3):
            with self.subTest(steps=steps, op="close"):
                g = self.two_step()
                for _ in range(steps):
                    next(g)
                g.close()
                self.assertEqual(inspect.getgeneratorstate(g), inspect.GEN_CLOSED)
            with self.subTest(steps=steps, op="throw"):
                g = self.two_step()
                for _ in range(steps):
                    next(g)
                if steps == 2:
                    self.assertEqual(g.throw(ValueError("x")), ("caught", "x"))
                else:
                    with self.assertRaises(KeyError):
                        g.throw(KeyError("k"))
                    self.assertEqual(inspect.getgeneratorstate(g),
                                     inspect.GEN_CLOSED)

    def test_state_while_suspended_and_running(self):
        def gen():
            me = yield
            self.assertTrue(me.gi_running)
            self.assertFalse(me.gi_suspended)
            yield

        g = gen()
        self.assertFalse(g.gi_suspended)
        next(g)
        self.assertTrue(g.gi_suspended)
        self.assertGreater(g.gi_frame.f_lineno, 0)
        g.send(g)
        g.close()
        self.assertIsNone(g.gi_frame)

    def test_delegation_chains(self):
        def inner():
            x = yield 1
            yield x
            return "done"

        def middle():
            r = yield from inner()
            yield r

        def outer():
            yield from middle()

        for make in (inner, middle, outer):
            with self.subTest(make=make.__name__):
                self.assertEqual(list(make())[0], 1)
        g = outer()
        self.assertEqual(next(g), 1)
        self.assertEqual(g.send("s"), "s")
        self.assertEqual(next(g), "done")
        with self.assertRaises(StopIteration):
            next(g)

        # the inner generator is resumed directly while it is delegated to
        i = inner()
        m = (lambda: (yield from i))()
        self.assertEqual(next(m), 1)
        self.assertEqual(next(i), None)

    def test_delegated_throw_and_close(self):
        log = []

        def inner():
            try:
                yield 1
            finally:
                log.append("inner")

        def outer():
            try:
                yield from inner()
            finally:
                log.append("outer")

        g = outer()
        next(g)
        g.close()
        self.assertEqual(log, ["inner", "outer"])
        g = outer()
        next(g)
        with self.assertRaises(KeyError):
            g.throw(KeyError)
        self.assertEqual(log, ["inner", "outer", "inner", "outer"])

    def test_recursion_limit(self):
        def rec(n):
            if n:
                yield from rec(n - 1)
            yield n

        self.assertEqual(sum(rec(50)), sum(range(51)))
        old = sys.getrecursionlimit()
        sys.setrecursionlimit(max(old // 5, 120))
        self.addCleanup(sys.setrecursionlimit, old)
        with self.assertRaises(RecursionError):
            sum(rec(5000))

    def test_monitoring_events(self):
        mon = sys.monitoring
        tool = mon.PROFILER_ID
        events = []
        E = mon.events

        def record(name):
            def callback(code, *args):
                if code.co_name in ("leaf", "delegating"):
                    events.append((name, code.co_name))
            return callback

        def leaf():
            yield 1
            yield 2

        def delegating():
            yield from leaf()
            yield 3

        mon.use_tool_id(tool, "test_ir_generator_life")
        self.addCleanup(mon.free_tool_id, tool)
        wanted = E.PY_START | E.PY_RESUME | E.PY_YIELD | E.PY_RETURN
        for name in ("PY_START", "PY_RESUME", "PY_YIELD", "PY_RETURN"):
            mon.register_callback(tool, getattr(E, name), record(name))
        mon.set_events(tool, wanted)
        try:
            for _ in leaf():
                pass
            for _ in delegating():
                pass
        finally:
            mon.set_events(tool, 0)
            for name in ("PY_START", "PY_RESUME", "PY_YIELD", "PY_RETURN"):
                mon.register_callback(tool, getattr(E, name), None)
        self.assertEqual(events[:7], [
            ("PY_START", "leaf"), ("PY_YIELD", "leaf"),
            ("PY_RESUME", "leaf"), ("PY_YIELD", "leaf"),
            ("PY_RESUME", "leaf"), ("PY_RETURN", "leaf"),
            ("PY_START", "delegating"),
        ])
        # delegating: a start, a leaf start, then a yield and a resume in
        # each of the two generators for each item, in order
        names = [e[0] for e in events[6:]]
        self.assertEqual(names.count("PY_START"), 2)
        self.assertEqual(names.count("PY_RETURN"), 2)
        self.assertEqual(names.count("PY_YIELD"), 5)
        self.assertEqual(names.count("PY_RESUME"), 5)


class FinishTests(unittest.TestCase):

    def test_locals_are_finalized_when_the_generator_finishes(self):
        log = []

        def gen():
            a = Noisy(log, "a")
            yield 1
            b = Noisy(log, "b")
            yield 2

        self.assertEqual(list(gen()), [1, 2])
        self.assertEqual(sorted(log), ["a", "b"])

    def test_abandoned_generator_finalizes_locals_and_runs_finally(self):
        log = []

        def gen():
            n = Noisy(log, "pending")
            try:
                yield 1
                yield 2
            finally:
                log.append("finally")

        g = gen()
        next(g)
        del g
        gc.collect()
        self.assertEqual(sorted(log), ["finally", "pending"])

    def test_finally_in_a_finished_generator(self):
        log = []

        def gen():
            try:
                yield 1
            finally:
                log.append("finally")

        self.assertEqual(list(gen()), [1])
        self.assertEqual(log, ["finally"])

    def test_yield_in_finally_on_close(self):
        def gen():
            try:
                yield 1
            finally:
                yield "again"

        g = gen()
        next(g)
        with self.assertRaises(RuntimeError):
            g.close()

    def test_weakref_callback_of_a_finished_generator(self):
        calls = []

        def gen():
            yield 1

        g = gen()
        ref = weakref.ref(g, lambda r: calls.append("callback"))
        list(g)
        del g
        self.assertIsNone(ref())
        self.assertEqual(calls, ["callback"])

    def test_frame_object_outlives_a_finished_generator(self):
        def gen():
            local = Noisy([], "x")
            yield 1

        g = gen()
        frame = g.gi_frame
        list(g)
        del g
        # The frame object took the locals when the generator was cleared
        self.assertEqual(list(frame.f_locals), ["local"])

    def test_finished_generator_behaves(self):
        def gen():
            yield 1

        g = gen()
        list(g)
        self.assertEqual(next(g, "done"), "done")
        self.assertIsNone(g.gi_frame)
        with self.assertRaises(StopIteration):
            g.send(None)
        with self.assertRaises(ValueError):
            g.throw(ValueError("late"))

    def test_generators_in_cycles_are_collected(self):
        def gen():
            me = yield
            yield me

        g = gen()
        next(g)
        g.send(g)
        ref = weakref.ref(g)
        del g
        gc.collect()
        gc.collect()
        self.assertIsNone(ref())

        class Holder:
            pass

        def holder_gen(h):
            h.g = yield
            yield

        h = Holder()
        g = holder_gen(h)
        next(g)
        g.send(g)
        ref = weakref.ref(h)
        del g, h
        gc.collect()
        gc.collect()
        self.assertIsNone(ref())

    def test_genexpr_over_finalizable_items(self):
        log = []

        class Items:
            def __init__(self):
                self.i = 0

            def __iter__(self):
                return self

            def __next__(self):
                self.i += 1
                if self.i > 2:
                    raise StopIteration
                return Noisy(log, "item%d" % self.i)

        self.assertEqual(len(list(x for x in Items())), 2)
        self.assertEqual(sorted(log), ["item1", "item2"])
        g = (x for x in Items())
        next(g)
        del g
        gc.collect()

    def test_exception_in_a_generator(self):
        def gen():
            local = Noisy([], "boom")
            yield 1
            raise KeyError("k")

        try:
            list(gen())
        except KeyError as e:
            names = []
            tb = e.__traceback__
            while tb is not None:
                names.append(tb.tb_frame.f_code.co_name)
                tb = tb.tb_next
        self.assertEqual(names[-1], "gen")

    def test_builtins_consuming_genexprs(self):
        self.assertEqual(sum(x for x in range(10)), 45)
        self.assertTrue(any(x > 5 for x in range(10)))
        self.assertFalse(all(x < 5 for x in range(10)))
        self.assertEqual(tuple(x for x in "abc"), ("a", "b", "c"))
        self.assertEqual(sorted(x for x in (3, 1, 2)), [1, 2, 3])
        self.assertEqual(dict((x, x) for x in range(3)), {0: 0, 1: 1, 2: 2})


if __name__ == "__main__":
    unittest.main()
