"""A delegation chain moves in one transfer.

A send down a chain of `yield from` or `await` levels enters every level
whose start would skip its Resume in one row (enter_generator,
Python/ceval_shared.h _PyRop_EnterChain), and a yield goes up through every
plain delegating consumer in one suspension (ROP_SUSPEND).  Nothing that
Python can see changes: values, exceptions, tracebacks, frame attributes and
the events of observed levels are those of the word-by-word transfer.
"""

import sys
import threading
import traceback
import types
import unittest

from test import support


def leaf(n):
    for i in range(n):
        x = yield i
        if x is not None:
            yield ("got", x)
    return "leaf-done"


def level(inner):
    r = yield from inner
    return ("level", r)


def chain(depth, n=3):
    g = leaf(n)
    for _ in range(depth):
        g = level(g)
    return g


def nest(depth, bottom):
    g = bottom
    for _ in range(depth):
        g = level(g)
    return g


class ValuesTests(unittest.TestCase):

    def test_values_at_each_depth(self):
        for depth in range(7):
            with self.subTest(depth=depth):
                self.assertEqual(list(chain(depth)), [0, 1, 2])

    def test_send_reaches_the_deepest(self):
        for depth in (1, 2, 4):
            with self.subTest(depth=depth):
                g = chain(depth)
                self.assertEqual(next(g), 0)
                self.assertEqual(g.send("v"), ("got", "v"))
                self.assertEqual(g.send(None), 1)
                self.assertEqual(next(g), 2)

    def test_return_values(self):
        def outer():
            r = yield from chain(3)
            yield r
        self.assertEqual(list(outer())[-1],
                         ("level", ("level", ("level", "leaf-done"))))

    def test_for_loop_and_next(self):
        total = 0
        for x in chain(4, 100):
            total += x
        self.assertEqual(total, sum(range(100)))
        g = chain(4, 5)
        self.assertEqual([next(g) for _ in range(5)], list(range(5)))
        with self.assertRaises(StopIteration) as cm:
            next(g)
        self.assertEqual(cm.exception.value[0], "level")


class NonGeneratorLevelTests(unittest.TestCase):

    def test_iterator_class_level(self):
        class It:
            def __init__(self):
                self.i = 0
            def __iter__(self):
                return self
            def __next__(self):
                self.i += 1
                if self.i > 3:
                    raise StopIteration("it-done")
                return self.i
        self.assertEqual(list(nest(3, It())), [1, 2, 3])
        g = nest(2, It())
        list(g)
        with self.assertRaises(StopIteration) as cm:
            next(nest(1, iter(())))
        self.assertEqual(cm.exception.value, ("level", None))

    def test_list_iterator_level(self):
        self.assertEqual(list(nest(3, iter([5, 6, 7]))), [5, 6, 7])

    def test_generator_below_a_non_generator(self):
        def mid():
            yield from [0]
            yield from chain(2)
        self.assertEqual(list(nest(2, mid())), [0, 0, 1, 2])


class ExceptionTests(unittest.TestCase):

    def test_raise_at_the_deepest_caught_mid_chain(self):
        def bottom():
            yield 1
            raise KeyError("deep")
        def catcher(inner):
            try:
                yield from inner
            except KeyError as e:
                yield ("caught", e.args[0])
        g = level(level(catcher(level(bottom()))))
        self.assertEqual(list(g), [1, ("caught", "deep")])

    def test_traceback_through_a_chain(self):
        def bottom():
            yield 1
            raise ValueError("deep")
        g = nest(3, bottom())
        next(g)
        try:
            next(g)
        except ValueError as e:
            tb = traceback.extract_tb(e.__traceback__)
        names = [f.name for f in tb]
        self.assertEqual(names[-4:], ["level", "level", "level", "bottom"])
        self.assertEqual(tb[-1].line, 'raise ValueError("deep")')
        for f in tb[-4:-1]:
            self.assertEqual(f.line, "r = yield from inner")

    def test_throw_into_the_outer(self):
        g = chain(3)
        next(g)
        with self.assertRaises(ZeroDivisionError):
            g.throw(ZeroDivisionError)
        self.assertIsNone(g.gi_frame)

    def test_throw_into_a_mid_chain_generator(self):
        def catcher(inner):
            try:
                yield from inner
            except KeyError:
                yield "mid-caught"
                yield from chain(1)
        bottom = chain(1)
        mid = catcher(bottom)
        g = nest(2, mid)
        self.assertEqual(next(g), 0)
        self.assertEqual(g.throw(KeyError), "mid-caught")
        self.assertEqual(list(g), [0, 1, 2])
        self.assertIsNone(bottom.gi_frame)

    def test_close_a_mid_chain_generator_from_the_outer(self):
        closed = []
        def bottom():
            try:
                yield 1
                yield 2
            finally:
                closed.append("bottom")
        b = bottom()
        g = nest(3, b)
        self.assertEqual(next(g), 1)
        g.close()
        self.assertEqual(closed, ["bottom"])
        self.assertIsNone(b.gi_frame)

    def test_close_mid_chain_directly(self):
        b = chain(1)
        mid = level(b)
        g = level(mid)
        next(g)
        self.assertEqual(g.gi_yieldfrom, mid)
        self.assertEqual(mid.gi_yieldfrom, b)
        # mid is suspended inside g's delegation: its close closes b, and g
        # then resumes at an exhausted delegate and returns
        mid.close()
        self.assertIsNone(b.gi_frame)
        with self.assertRaises(StopIteration) as cm:
            next(g)
        self.assertEqual(cm.exception.value, ("level", None))


class FrameStateTests(unittest.TestCase):

    def test_state_seen_from_the_deepest(self):
        seen = {}
        gens = []
        def bottom():
            for g in gens:
                seen[id(g)] = (
                    g.gi_running, g.gi_frame.f_lasti, g.gi_frame.f_lineno,
                    g.gi_yieldfrom is not None)
            yield 1
        b = bottom()
        g1 = level(b)
        g2 = level(g1)
        g3 = level(g2)
        gens.extend([g1, g2, g3])
        self.assertEqual(next(g3), 1)
        line = level.__code__.co_firstlineno + 1
        values = list(seen.values())
        self.assertEqual(len(values), 3)
        for running, lasti, lineno, yf in values:
            # A running generator delegates to nothing that it can show
            self.assertTrue(running)
            self.assertEqual(lineno, line)
            self.assertFalse(yf)
        lastis = {v[1] for v in values}
        self.assertEqual(len(lastis), 1)

        # Suspended: the same frames, not running, delegating
        for g, inner in ((g3, g2), (g2, g1), (g1, b)):
            self.assertFalse(g.gi_running)
            self.assertIs(g.gi_yieldfrom, inner)
            self.assertEqual(g.gi_frame.f_lineno, line)
        list(g3)
        for g in (g1, g2, g3):
            self.assertIsNone(g.gi_frame)
            self.assertIsNone(g.gi_yieldfrom)

    def test_coroutine_chain(self):
        @types.coroutine
        def bottom():
            x = yield "bottom"
            return x

        async def mid(d):
            if d == 0:
                return await bottom()
            return await mid(d - 1)

        c = mid(4)
        self.assertEqual(c.send(None), "bottom")
        self.assertIsNotNone(c.cr_await)
        inner = c.cr_await
        depth = 0
        while inner is not None and getattr(inner, "cr_await", None) is not None:
            inner = inner.cr_await
            depth += 1
        self.assertEqual(depth, 4)
        with self.assertRaises(StopIteration) as cm:
            c.send("done")
        self.assertEqual(cm.exception.value, "done")


# The chain that the event tests observe, at module level, so that its
# offsets are those recorded
def inner(n):
    for i in range(n):
        x = yield i
        if x is not None:
            yield ('got', x)


def middle(n):
    r = yield from inner(n)
    return r


def outer(n):
    yield from middle(n)


class ObservedTests(unittest.TestCase):
    """The events are those that the word-by-word transfer reports
    (recorded before the chain transfer landed)."""

    def test_observed_middle_level(self):
        for _ in range(3):
            list(outer(5))
        mon = sys.monitoring
        E = mon.events
        TOOL = 3
        codes = {inner.__code__, middle.__code__, outer.__code__}
        events = []
        names = ("PY_RESUME", "PY_YIELD", "PY_START", "PY_RETURN",
                 "STOP_ITERATION", "JUMP", "BRANCH_LEFT", "BRANCH_RIGHT")
        mon.use_tool_id(TOOL, "gn")
        try:
            def cb(name):
                def f(code, off, *rest):
                    if code in codes:
                        events.append((name, code.co_name, off)
                                      + tuple(r for r in rest if isinstance(r, int)))
                return f
            mask = 0
            for name in names:
                mon.register_callback(TOOL, getattr(E, name), cb(name))
                mask |= getattr(E, name)
            mon.set_local_events(TOOL, middle.__code__, mask)
            g = outer(3)
            out = [next(g), g.send(None), g.send('v')]
            out += list(g)
        finally:
            mon.set_local_events(TOOL, middle.__code__, 0)
            for name in names:
                mon.register_callback(TOOL, getattr(E, name), None)
            mon.free_tool_id(TOOL)
        self.assertEqual(out, [0, 1, ('got', 'v'), 2])
        y, r, j = ('PY_YIELD', 'middle', 38), ('PY_RESUME', 'middle', 40), \
            ('JUMP', 'middle', 44, 34)
        self.assertEqual(events, [
            ('PY_START', 'middle', 4),
            y + (0,), r, j, y + (1,), r, j, y, r, j, y + (2,), r, j,
            ('STOP_ITERATION', 'middle', 34), ('PY_RETURN', 'middle', 52)])

    def test_settrace_on_a_chain(self):
        for _ in range(3):
            list(outer(5))
        codes = {inner.__code__, middle.__code__, outer.__code__}
        events = []

        def tr(frame, ev, arg):
            if frame.f_code in codes:
                events.append((ev, frame.f_code.co_name,
                               frame.f_lineno - frame.f_code.co_firstlineno))
            return tr
        old = sys.gettrace()
        sys.settrace(tr)
        try:
            out = list(outer(3))
        finally:
            sys.settrace(old)
        self.assertEqual(out, [0, 1, 2])
        first = [('call', 'outer', 0), ('line', 'outer', 1),
                 ('call', 'middle', 0), ('line', 'middle', 1),
                 ('call', 'inner', 0), ('line', 'inner', 1), ('line', 'inner', 2),
                 ('return', 'inner', 2), ('return', 'middle', 1),
                 ('return', 'outer', 1)]
        again = [('call', 'outer', 1), ('line', 'outer', 1),
                 ('call', 'middle', 1), ('line', 'middle', 1),
                 ('call', 'inner', 2), ('line', 'inner', 3), ('line', 'inner', 1)]
        step = again + [('line', 'inner', 2), ('return', 'inner', 2),
                        ('return', 'middle', 1), ('return', 'outer', 1)]
        last = again + [('return', 'inner', 1), ('exception', 'middle', 1),
                        ('line', 'middle', 2), ('return', 'middle', 2),
                        ('exception', 'outer', 1), ('return', 'outer', 1)]
        self.assertEqual(events, first + step + step + last)


class RecursionTests(unittest.TestCase):

    def deep_send(self, limit, observed):
        """Send into a suspended chain deeper than the budget, so that each
        level is entered by the chain's send.  observed: every
        level's code is observed, so each level runs its own words and the
        chain transfer passes none."""
        mon = sys.monitoring
        TOOL = 4
        old = sys.getrecursionlimit()
        # Suspended at full depth first, under a budget that holds it
        g = chain(limit + 50, 4)
        self.assertEqual(next(g), 0)
        sys.setrecursionlimit(limit)
        if observed:
            mon.use_tool_id(TOOL, "gn-recursion")
            mon.register_callback(TOOL, mon.events.PY_RESUME, lambda *a: None)
            mon.set_local_events(TOOL, level.__code__, mon.events.PY_RESUME)
        try:
            # The levels that the error leaves suspended are closed as their
            # delegators die, near the limit: their RecursionErrors are
            # unraisable, in either transfer
            with support.catch_unraisable_exception():
                try:
                    next(g)
                except RecursionError as e:
                    tb = traceback.extract_tb(e.__traceback__)
                    return len(tb), tb[-1].name, tb[-1].line
                finally:
                    del g
            return None
        finally:
            sys.setrecursionlimit(old)
            if observed:
                mon.set_local_events(TOOL, level.__code__, 0)
                mon.register_callback(TOOL, mon.events.PY_RESUME, None)
                mon.free_tool_id(TOOL)

    def test_chain_deeper_than_the_budget(self):
        # The RecursionError comes from the same level, at the same word, as
        # in the word-by-word transfer
        walked = self.deep_send(200, observed=False)
        self.assertIsNotNone(walked)
        self.assertEqual(walked, self.deep_send(200, observed=True))
        self.assertEqual(walked[1], "level")

    def test_chain_within_the_budget_after_a_partial_walk(self):
        old = sys.getrecursionlimit()
        sys.setrecursionlimit(300)
        try:
            g = chain(150, 2)
            self.assertEqual(list(g), [0, 1])
        finally:
            sys.setrecursionlimit(old)

    @support.requires_resource('cpu')
    def test_deep_chain_on_a_small_c_stack(self):
        result = []

        def body():
            old = sys.getrecursionlimit()
            sys.setrecursionlimit(20_000)
            try:
                g = chain(5_000, 3)
                result.extend(g)
                g = chain(5_000, 3)
                next(g)
                result.append(g.send("x"))
                # Exhausted, not closed: closing a suspended chain recurses
                # in C (gen_close), with or without the chain transfer
                result.extend(g)
            finally:
                sys.setrecursionlimit(old)

        old = threading.stack_size()
        threading.stack_size(256 * 1024)
        try:
            t = threading.Thread(target=body)
            t.start()
            t.join()
        finally:
            threading.stack_size(old)
        self.assertEqual(result, [0, 1, 2, ("got", "x"), 1, 2])


if __name__ == "__main__":
    unittest.main()
