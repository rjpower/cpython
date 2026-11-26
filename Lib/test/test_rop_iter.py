"""The iteration families (Python/register_ops/iteration.rop): IterOpen
(GET_ITER), ForIter (FOR_ITER) and IterClose (the early exit's POP_ITER)."""

import sys
import types
import unittest

from test.support import cpython_only, requires_specialization


class Log:
    """Records the order of finalizers"""
    def __init__(self, log, name):
        self.log = log
        self.name = name

    def __del__(self):
        self.log.append(self.name)


class OpenTests(unittest.TestCase):
    def test_sources_of_every_protocol(self):
        sources = [
            [1, 2, 3], (1, 2, 3), range(1, 4), {1: 0, 2: 0, 3: 0}, {1, 2, 3},
            "\x01\x02\x03", b"\x01\x02\x03", iter([1, 2, 3]), (i for i in (1, 2, 3)),
            map(int, "123"), reversed([3, 2, 1]), enumerate([1, 2, 3]),
        ]
        for source in sources:
            with self.subTest(type=type(source).__name__):
                seen = []
                for x in source:
                    seen.append(x)
                self.assertEqual(len(seen), 3)

    def test_a_user_iterable_and_a_user_iterator(self):
        class Iterator:
            def __init__(self, n):
                self.n = n
            def __iter__(self):
                return self
            def __next__(self):
                if self.n == 0:
                    raise StopIteration
                self.n -= 1
                return self.n

        class Iterable:
            def __iter__(self):
                return Iterator(3)

        self.assertEqual([x for x in Iterable()], [2, 1, 0])
        self.assertEqual([x for x in Iterator(2)], [1, 0])

    def test_a_generator_method_as_iter(self):
        class Iterable:
            def __iter__(self):
                yield 1
                yield 2

        self.assertEqual([x for x in Iterable()], [1, 2])

    def test_not_iterable(self):
        for source in (1, 1.5, None, object()):
            with self.subTest(source=source):
                with self.assertRaisesRegex(TypeError, "not iterable"):
                    for _ in source:
                        pass

    def test_a_failed_iter_closes_the_source(self):
        log = []

        class Source:
            def __init__(self):
                self.guard = Log(log, "source")

            def __iter__(self):
                raise RuntimeError("no iterator")

        def f():
            for _ in Source():
                pass

        with self.assertRaisesRegex(RuntimeError, "no iterator"):
            f()
        self.assertEqual(log, ["source"])

    def test_iter_returning_a_non_iterator(self):
        class Source:
            def __iter__(self):
                return 1

        with self.assertRaisesRegex(TypeError, "iter"):
            for _ in Source():
                pass


class StepTests(unittest.TestCase):
    def test_exhaustion_runs_no_body(self):
        for source in ([], (), range(0), iter(()), (x for x in ())):
            with self.subTest(type=type(source).__name__):
                ran = False
                for _ in source:
                    ran = True
                self.assertFalse(ran)

    def test_the_last_item_stays_bound(self):
        for x in [1, 2, 3]:
            pass
        self.assertEqual(x, 3)

    def test_a_raising_step_propagates_and_closes_the_iterator(self):
        log = []

        class Iterator:
            def __init__(self):
                self.guard = Log(log, "iterator")
                self.n = 0

            def __iter__(self):
                return self

            def __next__(self):
                self.n += 1
                if self.n == 3:
                    raise ValueError("step")
                return self.n

        def f():
            seen = []
            for x in Iterator():
                seen.append(x)
            return seen

        with self.assertRaisesRegex(ValueError, "step"):
            f()
        self.assertEqual(log, ["iterator"])

    def test_a_generator_return_value_is_dropped(self):
        def gen():
            yield 1
            return "done"

        self.assertEqual([x for x in gen()], [1])

    def test_exhaustion_closes_the_iterator_before_the_exit_runs(self):
        log = []

        class Iterator:
            def __init__(self):
                self.guard = Log(log, "iterator")
                self.done = False

            def __iter__(self):
                return self

            def __next__(self):
                if self.done:
                    raise StopIteration
                self.done = True
                return 1

        def f():
            for _ in Iterator():
                pass
            log.append("after")

        f()
        self.assertEqual(log, ["iterator", "after"])

    def test_mutation_during_iteration(self):
        items = [1, 2, 3]
        seen = []
        for x in items:
            seen.append(x)
            if x == 1:
                items.append(4)
        self.assertEqual(seen, [1, 2, 3, 4])

    def test_nested_loops(self):
        total = 0
        for i in range(3):
            for j in (10, 20):
                total += i * j
        self.assertEqual(total, 90)

    def test_a_long_body_and_a_far_exit(self):
        # The exit of a long body is beyond one word's distance
        source = "def f(xs):\n    n = 0\n    for x in xs:\n"
        source += "".join(f"        n += {i % 7}\n" for i in range(3000))
        source += "    return n\n"
        namespace = {}
        exec(source, namespace)
        expected = sum(i % 7 for i in range(3000))
        self.assertEqual(namespace["f"]([1, 2]), 2 * expected)
        self.assertEqual(namespace["f"]([]), 0)


class CloseTests(unittest.TestCase):
    def test_break_closes_the_iterator(self):
        log = []

        class Iterator:
            def __init__(self):
                self.guard = Log(log, "iterator")

            def __iter__(self):
                return self

            def __next__(self):
                return 1

        def f():
            for _ in Iterator():
                break
            log.append("after")

        f()
        self.assertEqual(log, ["iterator", "after"])

    def test_return_inside_the_loop(self):
        def f(xs):
            for x in xs:
                if x > 1:
                    return x
            return None

        self.assertEqual(f([1, 2, 3]), 2)
        self.assertIsNone(f([1]))

    def test_exceptions_unwind_the_pair(self):
        log = []

        def f():
            for x in [Log(log, "item")]:
                raise KeyError

        with self.assertRaises(KeyError):
            f()
        self.assertEqual(log, ["item"])


class DelegationTests(unittest.TestCase):
    def test_yield_from_an_iterable(self):
        def inner():
            yield from [1, 2, 3]

        self.assertEqual(list(inner()), [1, 2, 3])

    def test_yield_from_a_coroutine_in_a_generator_is_refused(self):
        async def coro():
            return 1

        @types.coroutine
        def delegate():
            return (yield from coro())

        self.assertEqual(list(_drive(delegate())), [])

        def plain():
            yield from coro()

        g = plain()
        with self.assertRaisesRegex(TypeError, "cannot 'yield from' a coroutine"):
            next(g)

    def test_a_generator_expression_opens_its_argument_eagerly(self):
        def f():
            return (x for x in 1)

        with self.assertRaisesRegex(TypeError, "not iterable"):
            f()


def _drive(coroutine):
    try:
        while True:
            yield coroutine.send(None)
    except StopIteration:
        return


class MonitoringTests(unittest.TestCase):
    tool = sys.monitoring.PROFILER_ID

    def setUp(self):
        sys.monitoring.use_tool_id(self.tool, "test_rop_iter")
        self.addCleanup(sys.monitoring.free_tool_id, self.tool)
        self.addCleanup(sys.monitoring.set_events, self.tool, 0)

    def record(self, events, names):
        log = []
        monitoring = sys.monitoring
        for name in names:
            event = getattr(monitoring.events, name)
            monitoring.register_callback(
                self.tool, event,
                lambda code, *args, _name=name: log.append(
                    (_name, code.co_name)) if code.co_name == "loop" else None)
        monitoring.set_events(self.tool, events)
        return log

    def test_branch_events_of_a_loop(self):
        def loop():
            for _ in (1, 2):
                pass

        events = sys.monitoring.events
        log = self.record(events.BRANCH_LEFT | events.BRANCH_RIGHT,
                          ("BRANCH_LEFT", "BRANCH_RIGHT"))
        loop()
        sys.monitoring.set_events(self.tool, 0)
        self.assertEqual([name for name, _ in log],
                         ["BRANCH_LEFT", "BRANCH_LEFT", "BRANCH_RIGHT"])

    def test_stop_iteration_event_of_a_generator(self):
        def gen():
            yield 1
            return "value"

        def loop():
            for _ in gen():
                pass

        events = sys.monitoring.events
        seen = []

        def on_stop(code, offset, exception):
            if code.co_name == "gen":
                return
            seen.append(exception.value if isinstance(exception, StopIteration)
                        else exception)

        sys.monitoring.register_callback(self.tool, events.STOP_ITERATION, on_stop)
        sys.monitoring.set_events(self.tool, events.STOP_ITERATION)
        loop()
        sys.monitoring.set_events(self.tool, 0)
        self.assertEqual(seen, ["value"])


def loop(source):
    seen = []
    for x in source:
        seen.append(x)
    return seen


class Counting:
    def __init__(self, n):
        self.n = n

    def __iter__(self):
        for i in range(self.n):
            yield i


class User:
    def __init__(self, n):
        self.n = n

    def __iter__(self):
        return self

    def __next__(self):
        if self.n == 0:
            raise StopIteration
        self.n -= 1
        return self.n


# (a source, the arms of the open and of the step once the site is warm)
ARMS = {
    "list": (lambda: [1, 2, 3], "IterOpen.Indexed", "ForIter.List"),
    "tuple": (lambda: (1, 2, 3), "IterOpen.Indexed", "ForIter.Tuple"),
    "range": (lambda: range(3), "IterOpen.Generic", "ForIter.Range"),
    "str": (lambda: "abc", "IterOpen.Indexed", "ForIter.Indexed"),
    "iterator": (lambda: iter([1, 2, 3]), "IterOpen.Self", "ForIter.Slot"),
    "user": (lambda: User(3), "IterOpen.Generic", "ForIter.Slot"),
    "generator method": (lambda: Counting(3), "IterOpen.GenMethod", "ForIter.Slot"),
}


def fresh_loop():
    code = loop.__code__.replace()
    return types.FunctionType(code, globals())


class SpecializedTests(unittest.TestCase):
    @cpython_only
    @requires_specialization
    def test_each_source_reaches_its_arms(self):
        import _testinternalcapi
        for name, (make, opener, stepper) in ARMS.items():
            with self.subTest(source=name):
                f = fresh_loop()
                expected = f(make())
                for _ in range(100):
                    self.assertEqual(f(make()), expected)
                words = [w[0] for w in _testinternalcapi.get_ir(f.__code__, True)]
                self.assertIn(opener, words)
                self.assertIn(stepper, words)

    def test_a_site_that_changes_its_source_type(self):
        # A warmed arm misses and the generic arm takes over, and the site
        # adapts again, with every item seen once and in order
        f = fresh_loop()
        order = [name for name in ARMS for _ in range(3)] * 20
        for name in order:
            make = ARMS[name][0]
            self.assertEqual(f(make()), loop(make()), name)

    def test_early_exit_and_exhaustion_under_each_arm(self):
        def stop_early(source):
            for x in source:
                if x == 1:
                    break
            return x

        for name, (make, _, _) in ARMS.items():
            with self.subTest(source=name):
                expected = stop_early(make())
                for _ in range(50):
                    self.assertEqual(stop_early(make()), expected)

    def test_range_of_big_ints(self):
        n = 2**62
        self.assertEqual(loop(range(n, n + 3)), [n, n + 1, n + 2])
        for _ in range(50):
            loop(range(n, n + 3))
        self.assertEqual(loop(range(n, n + 3)), [n, n + 1, n + 2])
        self.assertEqual(loop(range(0)), [])

    def test_mutation_during_a_warmed_list_loop(self):
        f = fresh_loop()
        for _ in range(100):
            f([1, 2, 3])
        items = [1, 2, 3]
        seen = []
        for x in items:
            seen.append(x)
            if x == 1:
                items.append(4)
        self.assertEqual(seen, [1, 2, 3, 4])
        self.assertEqual(f(items), [1, 2, 3, 4])


if __name__ == "__main__":
    unittest.main()
