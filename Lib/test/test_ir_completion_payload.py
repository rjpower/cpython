"""Semantic completion reports the actual returned value as one payload."""
from contextlib import contextmanager
import sys
import unittest


@contextmanager
def observers(code, callbacks, trace=None):
    acquired = []
    previous = sys.gettrace()
    try:
        for tool, callback, global_events in callbacks:
            sys.monitoring.use_tool_id(tool, 'completion-observers')
            acquired.append(tool)
            sys.monitoring.register_callback(tool, sys.monitoring.events.STOP_ITERATION, callback)
            if global_events:
                sys.monitoring.set_events(tool, sys.monitoring.events.STOP_ITERATION)
            else:
                sys.monitoring.set_local_events(tool, code, sys.monitoring.events.STOP_ITERATION)
        if trace is not None:
            sys.settrace(trace)
        yield
    finally:
        sys.settrace(previous)
        for tool in acquired:
            sys.monitoring.set_events(tool, 0)
            sys.monitoring.set_local_events(tool, code, 0)
            sys.monitoring.register_callback(tool, sys.monitoring.events.STOP_ITERATION, None)
            sys.monitoring.free_tool_id(tool)


class CompletionPayloadTests(unittest.TestCase):
    def test_returned_payload_identity(self):
        for kind in ('FOR', 'SEND'):
            for warm in (0, 100):
                for payload in (None, 7, (1, 2), StopIteration(None)):
                    with self.subTest(kind=kind, warm=warm, payload=payload):
                        self.check_payload(kind, warm, payload)

    def check_payload(self, kind, warm, payload):
        def gen():
            yield 1
            return payload
        returned = []
        if kind == 'FOR':
            def consume():
                for item in gen():
                    pass
            drive = consume
        else:
            def consume():
                returned.append((yield from gen()))
            def drive():
                list(consume())
        for _ in range(warm):
            drive()
        returned.clear()
        seen = []
        def stop(code, offset, error):
            self.assertIs(type(error), StopIteration)
            self.assertIs(error.value, payload)
            self.assertIsNot(error, payload)
            if payload is None:
                self.assertEqual(error.args, ())
            seen.append(error)
        tool = 4
        sys.monitoring.use_tool_id(tool, 'completion-payload-test')
        try:
            sys.monitoring.register_callback(tool, sys.monitoring.events.STOP_ITERATION, stop)
            sys.monitoring.set_local_events(tool, consume.__code__, sys.monitoring.events.STOP_ITERATION)
            drive()
        finally:
            sys.monitoring.set_local_events(tool, consume.__code__, 0)
            sys.monitoring.register_callback(tool, sys.monitoring.events.STOP_ITERATION, None)
            sys.monitoring.free_tool_id(tool)
        self.assertEqual(len(seen), 1)
        if kind == 'SEND':
            self.assertEqual(len(returned), 1)
            self.assertIs(returned[0], payload)

    def test_valueless_for_preserves_modern_tools_and_restart(self):
        def gen():
            yield 1
        def consume():
            for item in gen():
                pass
        counts = [0, 0]
        legacy = []
        def trace(frame, event, arg):
            if frame.f_code is consume.__code__ and event == 'exception':
                legacy.append(arg)
            return trace
        def once(code, offset, error):
            if code is consume.__code__:
                self.assertIsNone(error.value)
                counts[0] += 1
                return sys.monitoring.DISABLE
        def persistent(code, offset, error):
            if code is consume.__code__:
                self.assertIsNone(error.value)
                counts[1] += 1
        with observers(consume.__code__, [(4, once, False), (5, persistent, True)], trace):
            consume()
            self.assertEqual(counts, [1, 1])
            consume()
            self.assertEqual(counts, [1, 2])
            sys.monitoring.restart_events()
            consume()
            self.assertEqual(counts, [2, 3])
        self.assertEqual(legacy, [])

    def test_send_none_keeps_legacy_and_modern_completion(self):
        def gen():
            yield 1
        def consume():
            result = yield from gen()
            self.assertIsNone(result)
        legacy, modern = [], []
        def trace(frame, event, arg):
            if frame.f_code is consume.__code__ and event == 'exception':
                legacy.append(arg[1].value)
            return trace
        def stop(code, offset, error):
            modern.append(error.value)
        with observers(consume.__code__, [(4, stop, False)], trace):
            list(consume())
        self.assertEqual(legacy, [None])
        self.assertEqual(modern, [None])

    def test_delivered_payload_mutation_keeps_observer_order(self):
        def gen():
            yield 1
            return 7
        def consume():
            for item in gen():
                pass
        seen = []
        def trace(frame, event, arg):
            if frame.f_code is consume.__code__ and event == 'exception':
                seen.append(('legacy', arg[1].value))
                arg[1].value = 99
            return trace
        def first(code, offset, error):
            if code is consume.__code__:
                seen.append(('modern5', error.value))
                error.value = 123
        def second(code, offset, error):
            if code is consume.__code__:
                seen.append(('modern4', error.value))
        with observers(consume.__code__, [(4, second, False), (5, first, False)], trace):
            consume()
        self.assertEqual(seen, [('legacy', 7), ('modern5', 99), ('modern4', 123)])

    def test_real_iterator_raise_none_remains_legacy_exception(self):
        class Iterator:
            def __iter__(self):
                return self
            def __next__(self):
                raise StopIteration(None)
        def consume():
            for item in Iterator():
                pass
        seen = []
        def trace(frame, event, arg):
            if frame.f_code is consume.__code__ and event == 'exception':
                seen.append(arg[1])
            return trace
        previous = sys.gettrace()
        try:
            sys.settrace(trace)
            consume()
        finally:
            sys.settrace(previous)
        self.assertEqual(len(seen), 1)
        self.assertIs(type(seen[0]), StopIteration)
        self.assertIsNone(seen[0].value)

    def test_callback_error_uses_completion_pre_state(self):
        class HookError(Exception):
            pass
        for delegate in (False, True):
            with self.subTest(delegate=delegate):
                payload = (1, 2)
                caught = []
                def gen():
                    yield 1
                    return payload
                if delegate:
                    def consume():
                        try:
                            yield from gen()
                        except HookError:
                            caught.append(True)
                    def drive():
                        list(consume())
                else:
                    def consume():
                        try:
                            for item in gen():
                                pass
                        except HookError:
                            caught.append(True)
                    drive = consume
                def stop(code, offset, error):
                    self.assertIs(error.value, payload)
                    self.assertEqual(sum(range(3)), 3)
                    raise HookError
                with observers(consume.__code__, [(4, stop, False)]):
                    drive()
                self.assertEqual(caught, [True])


if __name__ == '__main__':
    unittest.main()
