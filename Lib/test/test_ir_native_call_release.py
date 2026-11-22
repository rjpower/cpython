"""What a native call owes when it finishes.

A native call stores its result, then releases the temporaries that it
consumed: its callable and the operands that are not locals.  Each case runs
cold, through the generic handlers, and warm, through the specialized ones,
and checks the same outcome.
"""

import functools
import gc
import sys
import unittest
import weakref

WARM = 300


class Tracked:
    """An object that records its finalization"""

    deleted = []

    def __init__(self, name="t"):
        self.name = name

    def __del__(self):
        Tracked.deleted.append(self.name)


def cold_and_warm(function, *args):
    results = [function(*args)]
    for _ in range(WARM):
        results.append(function(*args))
    return results[0], results[-1]


class NativeCallReleaseTests(unittest.TestCase):
    def setUp(self):
        Tracked.deleted.clear()

    def test_callable_freed_during_its_own_call(self):
        # The temporary is the only owner of the partial once the callee
        # removes the attribute that held it.
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
            return holder.f(2)

        for result in cold_and_warm(run):
            self.assertEqual(result, 2)
        self.assertTrue(all(ref() is None for ref in refs))

    def test_operand_finalizer_runs_python_after_result(self):
        # The list temporary is the last owner of the Tracked object; its
        # finalizer runs Python while the call releases it, and the result
        # is already in its home.
        seen = []

        class Watcher(Tracked):
            def __del__(self):
                seen.append(sys._getframe(1).f_locals.get("n"))
                super().__del__()

        def run():
            n = len([Watcher()])
            return n

        for result in cold_and_warm(run):
            self.assertEqual(result, 1)
        self.assertEqual(len(seen), WARM + 1)
        self.assertEqual(set(seen), {1})

    def test_result_aliases_an_operand(self):
        def run():
            first = min([1], [2])
            second = max(Tracked("a"), Tracked("b"), key=id)
            return first, second.name in ("a", "b")

        for first, second in cold_and_warm(run):
            self.assertEqual(first, [1])
            self.assertTrue(second)
        # Both objects are gone: the operand that was not the result when the
        # call released it, the other when the function returned.
        self.assertEqual(len(Tracked.deleted), 2 * (WARM + 1))

    def test_result_is_the_callable_home(self):
        # dst is the callable's own home: nothing is released twice.
        def run(x):
            f = len
            f = f(x)
            return f

        for result in cold_and_warm(run, [1, 2, 3]):
            self.assertEqual(result, 3)

    def test_callee_raises(self):
        def run():
            try:
                max(Tracked("x"), 1)
            except TypeError:
                return "raised"

        for result in cold_and_warm(run):
            self.assertEqual(result, "raised")
        gc.collect()
        self.assertEqual(len(Tracked.deleted), WARM + 1)

    def test_global_eq_rebinds_the_local(self):
        # The lookup of h compares keys, which runs Python and rebinds the
        # local x after x was loaded as an operand.  The call sees the value
        # that x had when it was loaded, as in a stack interpreter.
        class Key:
            def __hash__(self):
                return hash("h")

            def __eq__(self, other):
                sys._getframe(1).f_locals["x"] = 5
                return other == "h"

        namespace = {Key(): list}
        exec(
            "def run():\n"
            "    x = [1]\n"
            "    return isinstance(x, h)\n",
            namespace)
        for result in cold_and_warm(namespace["run"]):
            self.assertIs(result, True)


class MonitoringTests(unittest.TestCase):
    TOOL = sys.monitoring.PROFILER_ID

    def setUp(self):
        sys.monitoring.use_tool_id(self.TOOL, "native-call-release")
        self.addCleanup(sys.monitoring.free_tool_id, self.TOOL)
        self.addCleanup(sys.monitoring.set_events, self.TOOL, 0)
        self.events = []
        events = sys.monitoring.events
        for name in ("CALL", "C_RETURN", "C_RAISE"):
            event = getattr(events, name)
            sys.monitoring.register_callback(
                self.TOOL, event, self.recorder(name))
            self.addCleanup(
                sys.monitoring.register_callback, self.TOOL, event, None)

    def recorder(self, name):
        def record(code, offset, callable_, *args):
            if code.co_name == "run":
                self.events.append((name, callable_))
        return record

    def observe(self, run, expected):
        # Warm up so the specialized handlers have run, then observe
        for _ in range(WARM):
            run()
        events = sys.monitoring.events
        sys.monitoring.set_events(
            self.TOOL, events.CALL | events.C_RETURN | events.C_RAISE)
        self.events.clear()
        try:
            run()
        finally:
            sys.monitoring.set_events(self.TOOL, 0)
        self.assertEqual(self.events, expected)

    def test_len(self):
        data = [1, 2]

        def run():
            return len(data)

        self.observe(run, [("CALL", len), ("C_RETURN", len)])

    def test_isinstance_and_type(self):
        data = [1, 2]

        def run():
            isinstance(data, list)
            return type(data)

        self.observe(run, [
            ("CALL", isinstance), ("C_RETURN", isinstance),
            ("CALL", type), ("C_RETURN", type),
        ])

    def test_method_descriptor(self):
        data = {"a": 1}
        get = dict.get

        def run():
            return data.get("a")

        self.observe(run, [("CALL", get), ("C_RETURN", get)])

    def test_builtin_raises(self):
        def run():
            try:
                max(1, "a")
            except TypeError:
                pass

        self.observe(run, [("CALL", max), ("C_RAISE", max)])

    def test_class_call(self):
        def run():
            return int("12")

        self.observe(run, [("CALL", int), ("C_RETURN", int)])


if __name__ == "__main__":
    unittest.main()
