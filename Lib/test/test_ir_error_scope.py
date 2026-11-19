"""Reached error selection stays live through cleanup and monitoring."""

import sys
import threading
import unittest
import weakref

import _testinternalcapi
from test import support



class ErrorScopeTests(unittest.TestCase):
    @support.requires_gil_enabled("foreign native scopes require the GIL or STW")
    def test_foreign_error_selection_during_head_locked_gc_walk(self):
        entered = threading.Event()
        release = threading.Event()
        escaped = []
        failures = []

        class Payload:
            def __del__(self):
                frame = sys._getframe(1)
                while frame is not None and frame.f_code is not target.__code__:
                    frame = frame.f_back
                escaped.append(frame)
                entered.set()
                if not release.wait(support.SHORT_TIMEOUT):
                    failures.append("timed out waiting for the verifier")

        def fail():
            raise LookupError("worker error")

        def consume(*args):
            self.fail("unreachable call")

        def target():
            return consume(Payload(), fail())

        def worker():
            try:
                target()
            except LookupError:
                pass

        thread = threading.Thread(target=worker)
        thread.start()
        try:
            self.assertTrue(entered.wait(support.SHORT_TIMEOUT))
            self.assertIsNotNone(escaped[0])
            # The verifier holds HEAD_LOCK through recursive frame traversal.
            # Foreign metadata lookup must use that actual held capability.
            self.assertEqual(_testinternalcapi.gc_verify(), [])
        finally:
            release.set()
            thread.join(support.SHORT_TIMEOUT)
        self.assertFalse(thread.is_alive())
        self.assertEqual(failures, [])

    def test_remaining_scratch_is_selected(self):
        observations = []
        references = []
        escaped = []
        bindings = []

        def consume(*args):
            self.fail("the failed argument must prevent the call")

        def fail():
            raise LookupError("argument failed")

        class Payload:
            def __init__(self, index):
                self.index = index
                references.append(weakref.ref(self))

            def __del__(self):
                frame = sys._getframe(1)
                while frame is not None and frame.f_code is not target.__code__:
                    frame = frame.f_back
                if frame is None:
                    observations.append((self.index, "missing active target"))
                    return
                if not escaped:
                    escaped.append(frame)
                frame.f_locals["mutable"] = 42
                bindings.append((self.index, frame.f_locals["mutable"]))
                observations.append((self.index, _testinternalcapi.gc_verify()))

        def target():
            mutable = 17
            return consume(Payload(0), Payload(1), fail())

        # assertRaises clears traceback frames on exit. Keep this escaped
        # frame intact until its captured bindings have been inspected.
        try:
            target()
        except LookupError as error:
            self.assertEqual(str(error), "argument failed")
        else:
            self.fail("the argument must fail")
        self.assertEqual([row[0] for row in observations], [1, 0])
        self.assertEqual(bindings, [(1, 42), (0, 42)])
        for index, holes in observations:
            self.assertEqual(holes, [])
        self.assertEqual(escaped[0].f_locals["mutable"], 42)
        self.assertTrue(all(reference() is None for reference in references))
        escaped[0].clear()

    def test_nested_error_monitor_restores_outer_scope(self):
        observations = []
        monitor = sys.monitoring
        tool = monitor.DEBUGGER_ID

        def inner():
            raise ValueError("inner error")

        def callback(code, offset, exception):
            if code is inner.__code__:
                observations.append(_testinternalcapi.gc_verify())

        class Payload:
            def __del__(self):
                try:
                    inner()
                except ValueError:
                    pass
                observations.append("handled")

        def fail():
            raise LookupError("outer error")

        def consume(*args):
            self.fail("unreachable call")

        def target():
            return consume(Payload(), fail())

        monitor.use_tool_id(tool, "nested-error-selection")
        try:
            monitor.register_callback(tool, monitor.events.RAISE, callback)
            monitor.set_local_events(tool, inner.__code__, monitor.events.RAISE)
            with self.assertRaisesRegex(LookupError, "outer error"):
                target()
        finally:
            monitor.set_local_events(tool, inner.__code__, 0)
            monitor.register_callback(tool, monitor.events.RAISE, None)
            monitor.free_tool_id(tool)
        self.assertEqual(observations,
                         [[], "handled"])

    def test_generator_throw_callback_sees_active_selection(self):
        observations = []
        monitor = sys.monitoring
        tool = monitor.DEBUGGER_ID

        def target():
            try:
                yield "ready"
            except LookupError:
                return "handled"

        def callback(code, offset, exception):
            observations.append((code is target.__code__,
                                 _testinternalcapi.gc_verify()))

        generator = target()
        self.assertEqual(next(generator), "ready")
        monitor.use_tool_id(tool, "throw-error-selection")
        try:
            monitor.register_callback(tool, monitor.events.PY_THROW, callback)
            monitor.set_local_events(tool, target.__code__, monitor.events.PY_THROW)
            with self.assertRaises(StopIteration) as ended:
                generator.throw(LookupError("injected"))
            self.assertEqual(ended.exception.value, "handled")
        finally:
            monitor.set_local_events(tool, target.__code__, 0)
            monitor.register_callback(tool, monitor.events.PY_THROW, None)
            monitor.free_tool_id(tool)
            generator.close()
        self.assertEqual(observations,
                         [(True, [])])
        self.assertIsNone(generator.gi_frame)

    def test_handled_callback_failure_starts_fresh_error_ingress(self):
        observations = []
        monitor = sys.monitoring
        tool = monitor.DEBUGGER_ID

        def target():
            try:
                raise ValueError("original")
            except ValueError:
                self.fail("the failed monitor must prevent handler execution")

        def callback(code, offset, exception):
            if code is target.__code__ and isinstance(exception, ValueError):
                observations.append(_testinternalcapi.gc_verify())
                raise RuntimeError("handled monitor failed")

        monitor.use_tool_id(tool, "handled-error-selection")
        try:
            monitor.register_callback(tool, monitor.events.EXCEPTION_HANDLED, callback)
            monitor.set_local_events(tool, target.__code__, monitor.events.EXCEPTION_HANDLED)
            with self.assertRaisesRegex(RuntimeError, "handled monitor failed"):
                target()
        finally:
            monitor.set_local_events(tool, target.__code__, 0)
            monitor.register_callback(tool, monitor.events.EXCEPTION_HANDLED, None)
            monitor.free_tool_id(tool)
        self.assertEqual(observations,
                         [[]])


if __name__ == "__main__":
    unittest.main()
