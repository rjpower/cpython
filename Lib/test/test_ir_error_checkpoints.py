"""Selected error snapshots come from reached post-flush Storage states."""

import gc
import marshal
import sys
import types
import unittest
from unittest import mock

import _testinternalcapi
from test.test_ir_admission import construct


def anext_code():
    # The payload is a stack temporary that retires while the error unwinds
    def f(make, iterator):
        mutable = None
        return (make(), iterator.__anext__())
    return f.__code__


def routes(code):
    return (code, construct(code), code.replace(),
            marshal.loads(marshal.dumps(code)))


class ErrorCheckpointTests(unittest.TestCase):
    def test_actual_scope_keeps_survivors_through_monitor_and_retirement(self):
        monitor = sys.monitoring
        tool = monitor.DEBUGGER_ID
        observations = []
        escaped = []
        function = None

        class Payload:
            def __del__(self):
                frame = sys._getframe(1)
                while frame is not None and frame.f_code is not function.__code__:
                    frame = frame.f_back
                if frame is None:
                    observations.append(("missing frame",))
                    return
                frame.f_locals["mutable"] = 42
                gc.collect()
                observations.append(("retire", _testinternalcapi.gc_verify()))

        class Iterator:
            def __anext__(self):
                raise LookupError("selected snapshot")

        def raised(code, offset, exception):
            if code is function.__code__:
                frame = sys._getframe(1)
                escaped.append(frame)
                frame.f_locals["mutable"] = 41
                observations.append(("raise", _testinternalcapi.gc_verify()))

        monitor.use_tool_id(tool, "selected-error-snapshot")
        try:
            monitor.register_callback(tool, monitor.events.RAISE, raised)
            for code in routes(anext_code()):
                function = types.FunctionType(code, globals())
                monitor.set_local_events(tool, code, monitor.events.RAISE)
                try:
                    try:
                        function(Payload, Iterator())
                    except LookupError as error:
                        self.assertEqual(str(error), "selected snapshot")
                    else:
                        self.fail("GET_ANEXT must fail")
                finally:
                    monitor.set_local_events(tool, code, 0)
                self.assertEqual(observations[-2:], [
                    ("raise", []), ("retire", [])])
                self.assertEqual(escaped[-1].f_locals["mutable"], 42)
                escaped[-1].clear()
        finally:
            monitor.register_callback(tool, monitor.events.RAISE, None)
            monitor.free_tool_id(tool)

    def test_actual_retired_scalar_input_is_absent_from_selected_snapshot(self):
        monitor = sys.monitoring
        tool = monitor.DEBUGGER_ID
        observations = []
        current = None

        class Value:
            def __bool__(self):
                raise LookupError("retired scalar input")

        def raised(code, offset, exception):
            if code is current:
                observations.append(_testinternalcapi.gc_verify())

        monitor.use_tool_id(tool, "scalar-error-actions")
        try:
            monitor.register_callback(tool, monitor.events.RAISE, raised)
            def f(payload, value):
                mutable = None
                return (payload, not value)

            for current in routes(f.__code__):
                function = types.FunctionType(current, globals())
                payload, value = object(), Value()
                monitor.set_local_events(tool, current, monitor.events.RAISE)
                try:
                    try:
                        function(payload, value)
                    except LookupError as error:
                        self.assertEqual(str(error), "retired scalar input")
                    else:
                        self.fail("the scalar predicate must fail")
                finally:
                    monitor.set_local_events(tool, current, 0)
                self.assertEqual(observations[-1], [])
        finally:
            monitor.register_callback(tool, monitor.events.RAISE, None)
            monitor.free_tool_id(tool)

    def test_published_native_result_supplies_the_following_selected_error(self):
        monitor = sys.monitoring
        tool = monitor.DEBUGGER_ID
        current = None
        observations = []
        redirects = []
        retired = []

        class Iterator:
            def __anext__(self):
                raise LookupError("published scalar result")

        class Value:
            def __invert__(self):
                return Iterator()

            def __del__(self):
                retired.append(True)

        def at_line(code, line):
            if code is Value.__del__.__code__:
                parent = sys._getframe(1).f_back
                if parent is not None and parent.f_code is current:
                    parent.f_locals["mutable"] = 42
                    try:
                        parent.f_lineno = parent.f_lineno
                    except ValueError as error:
                        redirects.append(str(error))
                    else:
                        self.fail("an active operation cannot be redirected")
                    self.assertEqual(_testinternalcapi.gc_verify(), [])

        def raised(code, offset, exception):
            if code is current:
                frame = sys._getframe(1)
                observations.append((_testinternalcapi.gc_verify(),
                                     frame.f_locals["mutable"]))

        # The Value is a stack temporary that retires inside ~
        def f(payload, make):
            mutable = None
            return (~make()).__anext__()

        image = f.__code__
        monitor.use_tool_id(tool, "published-result-error")
        try:
            monitor.register_callback(tool, monitor.events.RAISE, raised)
            monitor.register_callback(tool, monitor.events.LINE, at_line)
            monitor.set_local_events(tool, Value.__del__.__code__, monitor.events.LINE)
            for current in routes(image):
                function = types.FunctionType(current, globals())
                payload = object()
                monitor.set_local_events(tool, current, monitor.events.RAISE)
                try:
                    try:
                        function(payload, Value)
                    except LookupError as error:
                        self.assertEqual(str(error), "published scalar result")
                    else:
                        self.fail("the published iterator must fail")
                finally:
                    monitor.set_local_events(tool, current, 0)
                self.assertEqual(observations[-1], ([], 42))
                self.assertTrue(retired)
                self.assertTrue(redirects)
        finally:
            monitor.set_local_events(tool, Value.__del__.__code__, 0)
            monitor.register_callback(tool, monitor.events.LINE, None)
            monitor.register_callback(tool, monitor.events.RAISE, None)
            monitor.free_tool_id(tool)

    def test_new_handler_protocol_uses_explicit_control_until_transformed(self):
        def f(payload, iterator):
            mutable = None
            try:
                return iterator.__anext__()
            except LookupError:
                return None

        code = f.__code__
        function = types.FunctionType(code, globals())
        observations = []
        monitor = sys.monitoring
        tool = monitor.DEBUGGER_ID

        class Iterator:
            def __anext__(self):
                raise LookupError("handled selected error")

        def handled(current, offset, exception):
            if current is code:
                observations.append(_testinternalcapi.gc_verify())

        monitor.use_tool_id(tool, "selected-error-handler-entry")
        try:
            monitor.register_callback(tool, monitor.events.EXCEPTION_HANDLED, handled)
            monitor.set_local_events(tool, code, monitor.events.EXCEPTION_HANDLED)
            self.assertIsNone(function(object(), Iterator()))
        finally:
            monitor.set_local_events(tool, code, 0)
            monitor.register_callback(tool, monitor.events.EXCEPTION_HANDLED, None)
            monitor.free_tool_id(tool)
        self.assertEqual(observations, [[]])


if __name__ == "__main__":
    unittest.main()
