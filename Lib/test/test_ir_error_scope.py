"""Reached error selection stays live through cleanup and monitoring."""

import sys
import threading
import unittest
import weakref

import _testinternalcapi
from test import support


ERROR_SCOPE = 1 << 14
NULL_CONTROL_READABLE = 3


class ErrorScopeTests(unittest.TestCase):
    @support.requires_gil_enabled("foreign native scopes require the GIL or STW")
    def test_foreign_error_selection_during_head_locked_gc_walk(self):
        baseline = _testinternalcapi.get_transition_shadow_status()
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
            self.assertEqual(_testinternalcapi.get_frame_home_view(escaped[0])[0],
                             NULL_CONTROL_READABLE)
            # The verifier holds HEAD_LOCK through recursive frame traversal.
            # Foreign metadata lookup must use that actual held capability.
            self.assertEqual(_testinternalcapi.gc_verify(), [])
        finally:
            release.set()
            thread.join(support.SHORT_TIMEOUT)
        self.assertFalse(thread.is_alive())
        self.assertEqual(failures, [])
        self.assertEqual(_testinternalcapi.get_transition_shadow_status(), baseline)

    def test_remaining_scratch_is_selected_without_external_ownership(self):
        baseline = _testinternalcapi.get_transition_shadow_status()
        observations = []
        addresses = []
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
                addresses.append(id(self))
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
                selected = set(_testinternalcapi.get_frame_selected_object_ids(frame))
                external = set(_testinternalcapi.get_transition_root_ids())
                holes = _testinternalcapi.gc_verify()
                phases = [state for code, state in
                          _testinternalcapi.get_ir_frame_phases()
                          if code is target.__code__]
                scopes = [row for row in
                          _testinternalcapi.get_transition_shadow_status()
                          if row[0] == 3]
                observations.append((self.index, selected, external, holes,
                                     phases, scopes,
                                     _testinternalcapi.get_frame_home_view(frame)[0]))

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
        self.assertTrue(all(len(row) == 7 for row in observations), observations)
        for index, selected, external, holes, phases, scopes, authority in observations:
            self.assertNotIn(addresses[index], selected)
            self.assertTrue(set(addresses).isdisjoint(external))
            self.assertEqual(holes, [])
            self.assertEqual(phases, [ERROR_SCOPE])
            self.assertEqual(scopes, [(3, 0, 0, True)])
            self.assertEqual(authority, NULL_CONTROL_READABLE)
            if index == 1:
                self.assertIn(addresses[0], selected)
            else:
                self.assertTrue(set(addresses).isdisjoint(selected))
        self.assertEqual(escaped[0].f_locals["mutable"], 42)
        self.assertTrue(all(reference() is None for reference in references))
        self.assertEqual(_testinternalcapi.get_transition_root_ids(), [])
        self.assertEqual(_testinternalcapi.get_transition_shadow_status(), baseline)
        # M2 may capture the escaped frame, but only its bindings remain.
        self.assertEqual(_testinternalcapi.get_frame_home_view(escaped[0])[2], 0)
        self.assertTrue(set(addresses).isdisjoint(
            _testinternalcapi.get_frame_selected_object_ids(escaped[0])))
        escaped[0].clear()

    def test_nested_error_monitor_restores_outer_scope(self):
        baseline = _testinternalcapi.get_transition_shadow_status()
        observations = []
        monitor = sys.monitoring
        tool = monitor.DEBUGGER_ID

        def inner():
            raise ValueError("inner error")

        def callback(code, offset, exception):
            if code is inner.__code__:
                rows = [row for row in
                        _testinternalcapi.get_transition_shadow_status()
                        if row[0] == 3]
                phases = [state for current, state in
                          _testinternalcapi.get_ir_frame_phases()
                          if current in (target.__code__, inner.__code__)]
                observations.append((rows, phases,
                                     _testinternalcapi.gc_verify()))

        class Payload:
            def __del__(self):
                try:
                    inner()
                except ValueError:
                    pass
                observations.append([row for row in
                                     _testinternalcapi.get_transition_shadow_status()
                                     if row[0] == 3])

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
                         [([(3, 0, 0, True), (3, 0, 0, True)],
                           [ERROR_SCOPE, ERROR_SCOPE], []),
                          [(3, 0, 0, True)]])
        self.assertEqual(_testinternalcapi.get_transition_shadow_status(), baseline)

    def test_generator_throw_callback_sees_active_selection(self):
        baseline = _testinternalcapi.get_transition_shadow_status()
        observations = []
        monitor = sys.monitoring
        tool = monitor.DEBUGGER_ID

        def target():
            try:
                yield "ready"
            except LookupError:
                return "handled"

        def callback(code, offset, exception):
            frame = generator.gi_frame
            observations.append((code is target.__code__,
                                 _testinternalcapi.get_frame_home_view(frame)[0],
                                 [row for row in _testinternalcapi.get_transition_shadow_status()
                                  if row[0] == 3],
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
                         [(True, NULL_CONTROL_READABLE,
                           [(3, 0, 0, True)], [])])
        self.assertIsNone(generator.gi_frame)
        self.assertEqual(_testinternalcapi.get_transition_shadow_status(), baseline)

    def test_handled_callback_failure_starts_fresh_error_ingress(self):
        baseline = _testinternalcapi.get_transition_shadow_status()
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
                frame = sys._getframe(1)
                selected = _testinternalcapi.get_frame_selected_object_ids(frame)
                observations.append((id(exception) in selected,
                                     _testinternalcapi.get_frame_home_view(frame)[0],
                                     [row for row in _testinternalcapi.get_transition_shadow_status()
                                      if row[0] == 3],
                                     _testinternalcapi.gc_verify()))
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
                         [(True, NULL_CONTROL_READABLE,
                           [(3, 0, 0, True)], [])])
        self.assertEqual(_testinternalcapi.get_transition_shadow_status(), baseline)


if __name__ == "__main__":
    unittest.main()
