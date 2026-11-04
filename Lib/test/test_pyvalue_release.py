"""Releasing a value can run Python (__del__), which the interpreter must
allow for: the release keeps the handled exception, and a change to tracing
or monitoring that it makes takes effect at the next instruction.

The register IR keeps a local that an instruction's result displaces
(``x = a + b`` with ``x`` bound) until the instruction has finished, and a
native call releases its arguments after the call returns.  Setting events
re-instruments every executing code object, as upstream does, so the
instruction after the release dispatches through the new instrumentation;
a debug build checks that the code is current (Python/ceval.c,
instrumentation_is_current)."""

import sys
import textwrap
import unittest
from test import support
from test.support import import_helper, script_helper


TOOL = 2  # As in test_monitoring
LINE = sys.monitoring.events.LINE


def relative_lines(code, lines):
    return [line - code.co_firstlineno for line in lines]


class RaisesInDel:
    def __del__(self):
        raise RuntimeError("from __del__")


class ReleaseKeepsHandledException(unittest.TestCase):
    def check(self, function):
        with support.catch_unraisable_exception() as cm:
            handled = function()
            self.assertIsInstance(cm.unraisable.exc_value, RuntimeError)
        self.assertIsInstance(handled, KeyError)
        self.assertEqual(handled.args, ("handled",))
        self.assertIsNone(sys.exception())

    def test_del_local(self):
        def function():
            try:
                raise KeyError("handled")
            except KeyError:
                x = RaisesInDel()
                del x
                return sys.exception()
        self.check(function)

    def test_rebind_local(self):
        def function(a, b):
            try:
                raise KeyError("handled")
            except KeyError:
                x = RaisesInDel()
                x = a + b
                return sys.exception()
        for a, b in ((1, 2), (1.5, 2.5), ("a", "b")):
            with self.subTest(a=a, b=b):
                # Several calls, so that specialised handlers run too
                for _ in range(20):
                    self.check(lambda: function(a, b))


class ReleaseWithPendingError(unittest.TestCase):
    def test_free_interned_name_with_error_pending(self):
        # setattr() interns the fresh name, which only the call holds.
        # type_setattro raises a TypeError that keeps no reference to it, so
        # the call's error path frees an interned string while the TypeError
        # is pending. Removing it from the interned dict must not fail.
        for i in range(3):
            with self.assertRaises(TypeError):
                setattr(int, ''.join(['probe_', str(i)]), 1)


class ReleaseChangesInstrumentation(unittest.TestCase):
    def setUp(self):
        sys.monitoring.use_tool_id(TOOL, "test_pyvalue_release")
        self.addCleanup(sys.monitoring.free_tool_id, TOOL)
        self.addCleanup(sys.monitoring.register_callback, TOOL, LINE, None)
        self.addCleanup(sys.monitoring.set_events, TOOL, 0)

    def monitored_lines(self, function, *args):
        """The lines of function that LINE events report for one call, when
        the call itself turns the events on"""
        seen = []

        def on_line(code, line):
            if code is function.__code__:
                seen.append(line)

        sys.monitoring.register_callback(TOOL, LINE, on_line)
        sys.monitoring.set_events(TOOL, 0)
        function(*args)
        sys.monitoring.set_events(TOOL, 0)
        return relative_lines(function.__code__, seen)

    def test_monitoring_from_displaced_local(self):
        class Enables:
            def __del__(self):
                sys.monitoring.set_events(TOOL, LINE)

        def function(a, b):
            x = Enables()
            x = a + b
            y = x * 2
            return y

        for a, b in ((1, 2), (1.5, 2.5)):
            for _ in range(20):
                with self.subTest(a=a, b=b):
                    self.assertEqual(self.monitored_lines(function, a, b), [3, 4])

    def test_monitoring_from_native_call_argument(self):
        class Enables:
            def __del__(self):
                sys.monitoring.set_events(TOOL, LINE)

        def function():
            x = id(Enables())
            y = x + 1
            return y

        for _ in range(20):
            self.assertEqual(self.monitored_lines(function), [2, 3])

    def test_settrace_from_displaced_local(self):
        seen = []

        def tracer(frame, event, arg):
            if frame.f_code is function.__code__ and event == "line":
                seen.append(frame.f_lineno)
            return tracer

        class Enables:
            def __del__(self):
                # The frame whose handler released this object
                sys._getframe(1).f_trace = tracer
                sys.settrace(tracer)

        def function(a, b):
            x = Enables()
            x = a + b
            y = x * 2
            return y

        for a, b in ((1, 2), (1.5, 2.5)):
            for _ in range(20):
                with self.subTest(a=a, b=b):
                    seen.clear()
                    try:
                        function(a, b)
                    finally:
                        sys.settrace(None)
                    self.assertEqual(relative_lines(function.__code__, seen), [3, 4])


class FailedInstrumentation(unittest.TestCase):
    def test_monitoring_failure_handled_in_frame(self):
        # set_events fails while it re-instruments the executing code
        # (instrument_all_executing_code_objects) and f handles the
        # MemoryError.  As upstream, f goes on with its old instrumentation,
        # so it reports no LINE event, and its next operation (len) does not
        # fail the debug check of its instrumentation.  The subprocess keeps
        # a debug abort from ending the run.
        import_helper.import_module("_testcapi")
        code = textwrap.dedent("""
            import sys
            import _testcapi

            E = sys.monitoring.events
            TOOL = 4
            seen = []
            sys.monitoring.use_tool_id(TOOL, "test")
            sys.monitoring.register_callback(
                TOOL, E.LINE,
                lambda code, line: seen.append(line - code.co_firstlineno)
                if code.co_name == "f" else None)

            def f():
                try:
                    _testcapi.set_nomemory(0, 1)
                    sys.monitoring.set_events(TOOL, E.LINE)
                except MemoryError:
                    seen.append("MemoryError")
                finally:
                    _testcapi.remove_mem_hooks()
                n = len([1, 2])
                return n + 1

            print(f(), seen)
            sys.monitoring.set_events(TOOL, 0)
        """)
        result = script_helper.assert_python_ok("-c", code)
        self.assertEqual(result.out.decode().strip(), "3 ['MemoryError']")


if __name__ == "__main__":
    unittest.main()
