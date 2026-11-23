"""The adapter from a native call site to its callee.

A call that no specialized handler covers reaches its callee through one
sequence chosen by the callee's calling convention: a value vectorcall, an
object vectorcall (its arguments as an object vector) or a tuple convention
(its arguments as a tuple).  Each case runs cold, through the generic
handlers, and warm, through the specialized ones, and checks the same
outcome.
"""

import collections
import functools
import gc
import operator
import sys
import unittest
import weakref

WARM = 300


def cold_and_warm(function, *args):
    results = [function(*args)]
    for _ in range(WARM):
        results.append(function(*args))
    return results[0], results[-1]


class Dtor:
    """An object whose finalizer runs Python and records its outcome"""

    seen = []

    def __del__(self):
        Dtor.seen.append(len(sys._getframe().f_back.f_code.co_name))


class Mapping:
    """A mapping that update() reads through keys() and __getitem__"""

    def __init__(self, items, hook=None):
        self.items = items
        self.hook = hook

    def keys(self):
        if self.hook is not None:
            self.hook()
        return list(self.items)

    def __getitem__(self, key):
        return self.items[key]


class RaisingKey:
    def __hash__(self):
        raise ZeroDivisionError("hash")


class TupleConventionTests(unittest.TestCase):
    """dict.update is METH_VARARGS | METH_KEYWORDS: one tuple, one parse"""

    def test_positional_and_keyword(self):
        def run():
            d = {"a": 1}
            d.update({"b": 2}, c=3)
            d.update([("d", 4)])
            return sorted(d.items())

        for result in cold_and_warm(run):
            self.assertEqual(result, [("a", 1), ("b", 2), ("c", 3), ("d", 4)])

    def test_inline_scalars_are_boxed_once_per_parse(self):
        # The ints and the float are inline words in the window; the callee
        # reads them as objects.
        def run():
            d = {}
            d.update(a=1, b=2.5, c=-7)
            return d

        for result in cold_and_warm(run):
            self.assertEqual(result, {"a": 1, "b": 2.5, "c": -7})

    def test_wrong_arity_and_type_errors(self):
        def run():
            d = {}
            with self.assertRaises(TypeError):
                d.update({}, {})
            with self.assertRaises(TypeError):
                d.update(5)
            with self.assertRaises(TypeError):
                dict.update()
            with self.assertRaises(TypeError):
                dict.update([], {})
            return d

        for result in cold_and_warm(run):
            self.assertEqual(result, {})

    def test_callee_raises(self):
        def run():
            with self.assertRaises(ZeroDivisionError):
                {}.update({RaisingKey(): 1})
            with self.assertRaises(ZeroDivisionError):
                {}.update(Mapping({}, hook=lambda: 1 / 0))
            return 1

        for result in cold_and_warm(run):
            self.assertEqual(result, 1)

    def test_callee_reenters_python(self):
        calls = []

        def hook():
            calls.append(1)
            inner = {}
            inner.update(x=1, y=2)
            return inner

        def run():
            d = {}
            d.update(Mapping({"k": 1}, hook=hook))
            return d

        for result in cold_and_warm(run):
            self.assertEqual(result, {"k": 1})
        self.assertEqual(len(calls), WARM + 1)

    def test_bound_receiver_is_dropped_while_the_call_runs(self):
        # The only owner of the dict is the call's receiver temporary.
        refs = []

        def hook():
            refs[-1]().clear()

        class Dict(dict):
            pass

        def run():
            d = Dict(z=0)
            refs.append(weakref.ref(d))
            d.update(Mapping({"k": 1}, hook=hook))
            return d

        for result in cold_and_warm(run):
            self.assertEqual(result, {"k": 1})

    def test_ordered_dict_update_through_a_subclass(self):
        class Sub(collections.OrderedDict):
            pass

        def run():
            d = Sub(a=1)
            d.update(b=2, c=3)
            return list(d.items())

        for result in cold_and_warm(run):
            self.assertEqual(result, [("a", 1), ("b", 2), ("c", 3)])

    def test_finalizer_runs_python_while_the_call_releases(self):
        Dtor.seen.clear()

        def run():
            d = {}
            d.update({"k": Dtor()})
            d.clear()
            return len(d)

        for result in cold_and_warm(run):
            self.assertEqual(result, 0)
        self.assertEqual(len(Dtor.seen), WARM + 1)

    def test_plain_varargs_descriptor(self):
        # async_generator.athrow is METH_VARARGS: keywords are rejected.
        async def agen():
            yield 1

        def run():
            g = agen()
            with self.assertRaises(TypeError):
                g.athrow(TypeError, value=1)
            awaitable = g.athrow(ValueError)
            awaitable.close()
            return 1

        for result in cold_and_warm(run):
            self.assertEqual(result, 1)

    def test_recursion_limit(self):
        # Each level calls update on a mapping whose keys() calls the next.
        def nest(depth):
            if depth == 0:
                return {}
            return {}.update(Mapping({}, hook=lambda: nest(depth - 1)))

        nest(20)
        with self.assertRaises(RecursionError):
            def forever():
                {}.update(Mapping({}, hook=forever))
            forever()


class ObjectVectorTests(unittest.TestCase):
    """Callees with an object vectorcall get one vector, boxes included"""

    def test_inline_and_object_arguments(self):
        def run():
            return (max(1, 2.5, 3, "z" and 4, 5),
                    functools.reduce(operator.add, [1, 2, 3], 10),
                    sorted([3, 1, 2], key=None, reverse=True))

        for result in cold_and_warm(run):
            self.assertEqual(result, (5, 16, [3, 2, 1]))

    def test_more_arguments_than_the_small_vector(self):
        args = list(range(40))

        def run():
            return (max(*args), min(*args), len(set(args)))

        for result in cold_and_warm(run):
            self.assertEqual(result, (39, 0, 40))

    def test_callable_freed_during_its_own_call(self):
        class Holder:
            pass

        holder = Holder()
        refs = []

        def body(*args):
            del holder.f
            gc.collect()
            return len(args)

        def run():
            holder.f = functools.partial(body, 1)
            refs.append(weakref.ref(holder.f))
            return holder.f(2, 3.5, 4)

        for result in cold_and_warm(run):
            self.assertEqual(result, 4)
        self.assertTrue(all(ref() is None for ref in refs))

    def test_keyword_only_and_star_callees(self):
        def kwonly(a, *, b, c=3):
            return (a, b, c)

        def star(*args, **kwargs):
            return (args, sorted(kwargs.items()))

        def run():
            p = functools.partial(kwonly, 1)
            q = functools.partial(star, 1, 2)
            return p(b=2), q(x=1, y=2.5)

        for result in cold_and_warm(run):
            self.assertEqual(result, ((1, 2, 3), ((1, 2), [("x", 1), ("y", 2.5)])))

    def test_callee_raises(self):
        def run():
            with self.assertRaises(ValueError):
                max([])
            with self.assertRaises(TypeError):
                max(1, "a", key=len, bogus=1)
            with self.assertRaises(KeyError):
                functools.reduce(lambda a, b: {}[1], [1, 2], 0)
            return 1

        for result in cold_and_warm(run):
            self.assertEqual(result, 1)


class MonitoringTests(unittest.TestCase):
    TOOL = 4

    def setUp(self):
        self.events = []
        mon = sys.monitoring
        mon.use_tool_id(self.TOOL, "test_ir_native_call_adapter")
        self.addCleanup(mon.free_tool_id, self.TOOL)
        self.addCleanup(mon.set_events, self.TOOL, 0)
        for name in ("CALL", "C_RETURN", "C_RAISE"):
            event = getattr(mon.events, name)
            mon.register_callback(
                self.TOOL, event,
                lambda code, offset, callable, arg0, _n=name:
                    self.events.append((_n, callable)))
            self.addCleanup(mon.register_callback, self.TOOL, event, None)

    def test_events_fire_once_for_tuple_and_vector_callees(self):
        mon = sys.monitoring
        ev = mon.events
        update = dict.update

        def tuple_call():
            d = {}
            d.update(a=1)

        def raising_call():
            try:
                {}.update(5)
            except TypeError:
                pass

        def vector_call():
            return max(1, 2, 3)

        for function, callee, expected in (
                (tuple_call, update, ["CALL", "C_RETURN"]),
                (raising_call, update, ["CALL", "C_RAISE"]),
                (vector_call, max, ["CALL", "C_RETURN"])):
            with self.subTest(function.__name__):
                for _ in range(WARM):
                    function()
                self.events.clear()
                mon.set_events(self.TOOL, ev.CALL | ev.C_RETURN | ev.C_RAISE)
                function()
                mon.set_events(self.TOOL, 0)
                names = [name for name, callable in self.events
                         if callable == callee]
                self.assertEqual(names, expected)


if __name__ == "__main__":
    unittest.main()
