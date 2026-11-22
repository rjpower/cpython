"""`for key, value in d.items()`: the loop that the interpreter runs as one
step per item, with the key and the value going straight to the two locals.

Each loop sits in a function that the tests call enough times to specialize
it before the case under test."""

import gc
import sys
import unittest
import weakref

WARMUP = 200


def warm(function, *args):
    for _ in range(WARMUP):
        function(*args)


def collect(d):
    result = []
    for k, v in d.items():
        result.append((k, v))
    return result


def last_pair(d):
    k = v = None
    for k, v in d.items():
        pass
    return k, v


def whole_items(d):
    result = []
    for t in d.items():
        result.append(t)
    return result


def three_targets(d):
    for a, b, c in d.items():
        pass


def nested_target(d):
    result = []
    for k, (a, b) in d.items():
        result.append((k, a, b))
    return result


class Holder:
    pass


def attribute_targets(d):
    h = Holder()
    result = []
    for h.k, h.v in d.items():
        result.append((h.k, h.v))
    return result


def global_targets(d):
    global GK, GV
    result = []
    for GK, GV in d.items():
        result.append((GK, GV))
    return result


class Dict(dict):
    pass


class DictItemsLoopTest(unittest.TestCase):

    def test_pairs(self):
        for size in (0, 1, 2, 5, 40):
            d = {f"k{i}": i * 1.5 for i in range(size)}
            warm(collect, d)
            self.assertEqual(collect(d), list(d.items()))

    def test_keys_and_values_of_every_kind(self):
        d = {1: "a", 2.5: None, "s": (1, 2), (1, 2): [3], None: 7, True: 8.5}
        warm(collect, d)
        self.assertEqual(collect(d), list(d.items()))

    def test_locals_keep_the_last_pair(self):
        d = {"a": 1, "b": 2, "c": 3}
        warm(last_pair, d)
        self.assertEqual(last_pair(d), ("c", 3))
        self.assertEqual(last_pair({}), (None, None))

    def test_old_value_closes_when_the_next_pair_is_stored(self):
        events = []

        class Obj:
            def __init__(self, name):
                self.name = name

            def __del__(self):
                events.append(f"del {self.name}")

        def loop(d):
            for k, v in d.items():
                events.append(f"body {k} {v.name}")
                d[k] = None

        def make():
            return {"k0": Obj("v0"), "k1": Obj("v1")}

        for _ in range(WARMUP):
            loop(make())
        events.clear()
        loop(make())
        # The local alone holds each value once the body replaces it in the
        # dict: the next pair's store releases it, and the return the last.
        self.assertEqual(events, ["body k0 v0", "del v0",
                                  "body k1 v1", "del v1"])

    def test_objects_are_released(self):
        class Obj:
            pass

        d = {Obj(): Obj() for _ in range(4)}
        refs = [weakref.ref(o) for pair in d.items() for o in pair]
        warm(collect, d)
        collect(d)
        last_pair(d)
        del d
        gc.collect()
        self.assertEqual([r() for r in refs], [None] * 8)

    def test_exception_in_the_body_releases_the_pair(self):
        class Obj:
            pass

        def loop(d, fail):
            for k, v in d.items():
                if fail:
                    raise ZeroDivisionError

        d = {Obj(): Obj()}
        refs = [weakref.ref(o) for pair in d.items() for o in pair]
        warm(loop, d, False)
        with self.assertRaises(ZeroDivisionError):
            loop(d, True)
        del d
        gc.collect()
        self.assertEqual([r() for r in refs], [None] * 2)

    def test_value_replaced_in_the_body(self):
        def loop(d):
            seen = []
            for k, v in d.items():
                d[k] = v + 100
                seen.append(v)
            return seen

        d = {"a": 1, "b": 2}
        warm(loop, {"a": 1, "b": 2})
        self.assertEqual(loop(d), [1, 2])
        self.assertEqual(d, {"a": 101, "b": 102})

    def check_mutation(self, mutate, message, d):
        def loop(d, mutating):
            for k, v in d.items():
                if mutating:
                    mutate(d, k, v)

        warm(loop, {"a": 1, "b": 2}, False)
        with self.assertRaises(RuntimeError) as cm:
            loop(d, True)
        self.assertEqual(str(cm.exception), message)

    def test_insert_during_iteration(self):
        d = {"a": 1, "b": 2}
        self.check_mutation(lambda d, k, v: d.__setitem__("new", 0),
                            "dictionary changed size during iteration", d)
        self.assertEqual(len(d), 3)

    def test_delete_during_iteration(self):
        d = {"a": 1, "b": 2, "c": 3}
        self.check_mutation(lambda d, k, v: d.__delitem__(k),
                            "dictionary changed size during iteration", d)
        self.assertEqual(len(d), 2)

    def test_clear_during_iteration(self):
        d = {"a": 1, "b": 2}
        self.check_mutation(lambda d, k, v: d.clear(),
                            "dictionary changed size during iteration", d)
        self.assertEqual(d, {})

    def test_keys_changed_during_iteration(self):
        def swap(d, k, v):
            del d[k]
            d[k + "x"] = v

        self.check_mutation(swap, "dictionary keys changed during iteration",
                            {"a": 1, "b": 2})

    def test_mutation_raises_as_the_generic_form_does(self):
        def pairs(d, mutating):
            for k, v in d.items():
                if mutating:
                    d["z" + k] = v

        def whole(d, mutating):
            for t in d.items():
                if mutating:
                    d["z" + t[0]] = t[1]

        warm(pairs, {"a": 1}, False)
        messages = []
        for loop in (pairs, whole):
            with self.assertRaises(RuntimeError) as cm:
                loop({"a": 1, "b": 2}, True)
            messages.append(str(cm.exception))
        self.assertEqual(messages[0], messages[1])

    def test_exhausted_iterator(self):
        def loop(it):
            n = 0
            for k, v in it:
                n += 1
            return n

        d = {"a": 1, "b": 2}
        warm(loop, iter(d.items()))
        it = iter(d.items())
        self.assertEqual(loop(it), 2)
        self.assertEqual(loop(it), 0)

    def test_partly_consumed_iterator(self):
        def loop(it):
            return [(k, v) for k, v in it]

        d = {"a": 1, "b": 2, "c": 3}

        def run():
            it = iter(d.items())
            next(it)
            result = []
            for k, v in it:
                result.append((k, v))
            return result

        warm(run)
        self.assertEqual(run(), [("b", 2), ("c", 3)])

    def test_stored_view_iterated_twice(self):
        def loop(view):
            first = []
            for k, v in view:
                first.append((k, v))
            second = []
            for k, v in view:
                second.append((k, v))
            return first, second

        d = {"a": 1, "b": 2}
        warm(loop, d.items())
        view = d.items()
        first, second = loop(view)
        self.assertEqual(first, [("a", 1), ("b", 2)])
        self.assertEqual(second, first)
        d["c"] = 3
        self.assertEqual(loop(view)[0], [("a", 1), ("b", 2), ("c", 3)])

    def test_whole_item_target(self):
        d = {"a": 1, "b": 2}
        warm(whole_items, d)
        self.assertEqual(whole_items(d), [("a", 1), ("b", 2)])

    def test_other_unpacks(self):
        d = {"a": (1, 2), "b": (3, 4)}
        warm(nested_target, d)
        self.assertEqual(nested_target(d), [("a", 1, 2), ("b", 3, 4)])
        warm(attribute_targets, d)
        self.assertEqual(attribute_targets(d), list(d.items()))
        warm(global_targets, d)
        self.assertEqual(global_targets(d), list(d.items()))
        self.assertEqual((GK, GV), ("b", (3, 4)))

    def test_wrong_target_count(self):
        d = {"a": 1}
        for _ in range(WARMUP):
            with self.assertRaises(ValueError):
                three_targets(d)

    def test_subclass_with_overridden_items(self):
        class Pairs(dict):
            def items(self):
                return [("x", 1), ("y", 2)]

        class Lazy(dict):
            def items(self):
                yield "p", 1
                yield "q", 2

        def loop(d):
            result = []
            for k, v in d.items():
                result.append((k, v))
            return result

        plain = {"a": 1, "b": 2}
        warm(loop, plain)
        self.assertEqual(loop(Pairs(a=0)), [("x", 1), ("y", 2)])
        self.assertEqual(loop(Lazy(a=0)), [("p", 1), ("q", 2)])
        self.assertEqual(loop(plain), [("a", 1), ("b", 2)])

    def test_plain_subclass_and_other_mappings(self):
        import collections

        class D(dict):
            pass

        def loop(d):
            result = []
            for k, v in d.items():
                result.append((k, v))
            return result

        warm(loop, {"a": 1})
        for d in (D(a=1, b=2), collections.OrderedDict(a=1, b=2),
                  collections.defaultdict(int, a=1, b=2),
                  collections.Counter("ab"),
                  {"a": 1, "b": 2}):
            self.assertEqual(loop(d), list(d.items()))

    def test_reversed_items(self):
        def loop(d):
            result = []
            for k, v in reversed(d.items()):
                result.append((k, v))
            return result

        d = {"a": 1, "b": 2, "c": 3}
        warm(loop, d)
        self.assertEqual(loop(d), [("c", 3), ("b", 2), ("a", 1)])

    def test_loop_exits(self):
        def loop(d):
            for k, v in d.items():
                if v == 2:
                    break
            else:
                return "none"
            return k

        d = {"a": 1, "b": 2, "c": 3}
        warm(loop, d)
        self.assertEqual(loop(d), "b")
        self.assertEqual(loop({"a": 1}), "none")

    def test_split_table_instance_dict(self):
        class C:
            def __init__(self):
                self.a = 1
                self.b = "two"

        def loop(o):
            result = []
            for k, v in o.__dict__.items():
                result.append((k, v))
            return result

        warm(loop, C())
        self.assertEqual(loop(C()), [("a", 1), ("b", "two")])

    def test_generator_loop(self):
        def gen(d):
            for k, v in d.items():
                yield k, v

        d = {"a": 1, "b": 2}
        for _ in range(WARMUP):
            list(gen(d))
        self.assertEqual(list(gen(d)), [("a", 1), ("b", 2)])


class DictItemIteratorResultTest(unittest.TestCase):
    """The pair that a dict item iterator returns when something other than a
    two-target unpack takes it."""

    def test_held_pairs_are_distinct(self):
        d = {"a": 1, "b": 2, "c": 3}
        it = iter(d.items())
        first = next(it)
        second = next(it)
        third = next(it)
        self.assertEqual([first, second, third], list(d.items()))
        self.assertEqual(len({id(first), id(second), id(third)}), 3)
        with self.assertRaises(StopIteration):
            next(it)

    def test_released_pair_is_reused(self):
        d = {"a": 1, "b": 2, "c": 3}
        it = iter(d.items())
        first = id(next(it))
        self.assertEqual(id(next(it)), first)
        self.assertEqual(id(next(it)), first)

    def test_iterator_never_advanced(self):
        d = Dict(a=1)
        reference = weakref.ref(d)
        d["it"] = iter(d.items())
        del d
        gc.collect()
        self.assertIsNone(reference())
        it = iter(reversed(Dict(a=1).items()))
        self.assertEqual(next(it), ("a", 1))

    def test_pair_survives_the_iterator(self):
        class Obj:
            pass

        d = {Obj(): Obj()}
        refs = [weakref.ref(o) for pair in d.items() for o in pair]
        it = iter(d.items())
        pair = next(it)
        del it
        d.clear()
        gc.collect()
        self.assertEqual([r() for r in refs], list(pair))
        del pair
        gc.collect()
        self.assertEqual([r() for r in refs], [None, None])

    def test_iterator_is_collected_with_its_pair(self):
        d = Dict(a=1)
        reference = weakref.ref(d)
        it = iter(d.items())
        pair = next(it)
        d["it"] = it
        d["pair"] = pair
        del d, it, pair
        gc.collect()
        self.assertIsNone(reference())


class DictItemsLoopMonitoringTest(unittest.TestCase):

    def setUp(self):
        self.tool = sys.monitoring.PROFILER_ID
        sys.monitoring.use_tool_id(self.tool, "test_dict_items_loop")
        self.addCleanup(sys.monitoring.free_tool_id, self.tool)

    def test_branch_events(self):
        events = []

        def on_branch(code, offset, destination):
            if code is collect.__code__:
                events.append(destination > offset)

        d = {"a": 1, "b": 2, "c": 3}
        warm(collect, d)
        sys.monitoring.register_callback(
            self.tool, sys.monitoring.events.BRANCH, on_branch)
        sys.monitoring.set_events(self.tool, sys.monitoring.events.BRANCH)
        try:
            self.assertEqual(collect(d), list(d.items()))
        finally:
            sys.monitoring.set_events(self.tool, 0)
            sys.monitoring.register_callback(
                self.tool, sys.monitoring.events.BRANCH, None)
        # One branch per item and one at the exhaustion, as for any loop.
        generic = []

        def whole(d):
            for t in d.items():
                pass

        def on_generic(code, offset, destination):
            if code is whole.__code__:
                generic.append(destination > offset)

        warm(whole, d)
        sys.monitoring.register_callback(
            self.tool, sys.monitoring.events.BRANCH, on_generic)
        sys.monitoring.set_events(self.tool, sys.monitoring.events.BRANCH)
        try:
            whole(d)
        finally:
            sys.monitoring.set_events(self.tool, 0)
            sys.monitoring.register_callback(
                self.tool, sys.monitoring.events.BRANCH, None)
        self.assertEqual(events, generic)
        self.assertEqual(len(events), 4)

    def test_events_turned_on_inside_the_loop(self):
        counts = []

        def on_branch(code, offset, destination):
            if code is loop.__code__:
                counts.append(offset)

        def loop(d, start):
            n = 0
            for k, v in d.items():
                if k == start:
                    sys.monitoring.register_callback(
                        self.tool, sys.monitoring.events.BRANCH, on_branch)
                    sys.monitoring.set_events(
                        self.tool, sys.monitoring.events.BRANCH)
                n += v
            return n

        d = {"a": 1, "b": 2, "c": 3}
        warm(loop, d, "none")
        try:
            self.assertEqual(loop(d, "a"), 6)
        finally:
            sys.monitoring.set_events(self.tool, 0)
            sys.monitoring.register_callback(
                self.tool, sys.monitoring.events.BRANCH, None)
        # The tests of `k == start` for the two later items, and the
        # branches of the loop at those items and at its exhaustion.
        self.assertEqual(len(counts), 5)

    def test_line_events(self):
        def lines_of(function, *args):
            lines = []
            first = function.__code__.co_firstlineno

            def tracer(frame, event, arg):
                if frame.f_code is function.__code__ and event == "line":
                    lines.append(frame.f_lineno - first)
                return tracer

            old = sys.gettrace()
            sys.settrace(tracer)
            try:
                function(*args)
            finally:
                sys.settrace(old)
            return lines

        def split_targets(d):
            n = 0
            for (k,
                 v) in d.items():
                n += v
            return n

        def split_targets_reference(d):
            n = 0
            for (k,
                 v) in list(d.items()):
                n += v
            return n

        d = {"a": 1, "b": 2}
        warm(split_targets, d)
        self.assertEqual(lines_of(split_targets, d),
                         lines_of(split_targets_reference, d))
        self.assertEqual(split_targets(d), 3)


if __name__ == "__main__":
    unittest.main()
