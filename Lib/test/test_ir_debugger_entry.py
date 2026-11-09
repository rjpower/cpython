"""Explicit debugger entry preserves call-site display and active owners."""
import asyncio
import bdb
import pdb
import sys
import unittest


class EntryError(Exception):
    pass


class DebuggerEntryTests(unittest.TestCase):
    def test_immediate_entry_and_commands(self):
        for backend in ('settrace', 'monitoring'):
            for command in ('continue', 'step', 'next'):
                with self.subTest(backend=backend, command=command):
                    stops = []

                    class Debugger(bdb.Bdb):
                        def user_opcode(self, frame):
                            stops.append(('entry', frame.f_lineno - first,
                                          frame.f_locals['x']))
                            if command == 'continue':
                                self.set_continue()
                            elif command == 'step':
                                self.set_step()
                            else:
                                self.set_next(frame)

                        def user_line(self, frame):
                            stops.append(('line', frame.f_lineno - first,
                                          frame.f_locals['x']))
                            self.set_continue()

                    debugger = Debugger(backend=backend)

                    def target():
                        x = 1
                        debugger.set_trace()
                        x = 2
                        return x

                    first = target.__code__.co_firstlineno
                    previous = sys.gettrace()
                    try:
                        self.assertEqual(target(), 2)
                    finally:
                        debugger.stop_trace()
                        sys.settrace(previous)
                    expected = [('entry', 2, 1)]
                    if command != 'continue':
                        expected.append(('line', 3, 1))
                    self.assertEqual(stops, expected)
                    self.assertIsNone(debugger.enterframe)
                    self.assertFalse(debugger.trace_opcodes)

    def test_entry_callback_failure_restores_context(self):
        for backend in ('settrace', 'monitoring'):
            for quit in (False, True):
                with self.subTest(backend=backend, quit=quit):
                    class Debugger(bdb.Bdb):
                        def user_opcode(self, frame):
                            if quit:
                                self.set_quit()
                            else:
                                raise EntryError('entry callback')

                    debugger = Debugger(backend=backend)
                    def target():
                        debugger.set_trace()
                    previous = sys.gettrace()
                    try:
                        with self.assertRaises(bdb.BdbQuit if quit else EntryError):
                            target()
                        self.assertIsNone(debugger.enterframe)
                        self.assertIsNone(sys.gettrace())
                    finally:
                        debugger.stop_trace()
                        sys.settrace(previous)

    def test_context_manager_restores_nested_state_on_error(self):
        debugger = bdb.Bdb()
        original, outer, inner = object(), object(), object()
        debugger.enterframe = original
        with debugger.set_enterframe(outer):
            with self.assertRaises(EntryError):
                with debugger.set_enterframe(inner):
                    raise EntryError
            self.assertIs(debugger.enterframe, outer)
        self.assertIs(debugger.enterframe, original)

    def test_private_entry_rejects_current_and_inactive_frames(self):
        called = []
        def callback(frame):
            called.append(frame)
        def same_frame():
            return sys._debugger_entry(sys._getframe(), callback)
        def inactive_frame():
            return sys._getframe()
        with self.assertRaisesRegex(ValueError, 'strict active ancestor'):
            same_frame()
        with self.assertRaisesRegex(ValueError, 'strict active ancestor'):
            sys._debugger_entry(inactive_frame(), callback)
        with self.assertRaises(TypeError):
            sys._debugger_entry(None, callback)
        self.assertEqual(called, [])

    def test_pending_ignored_call_jump(self):
        for backend in ('settrace', 'monitoring'):
            with self.subTest(backend=backend):
                class Debugger(bdb.Bdb):
                    def user_opcode(self, frame):
                        frame.f_lineno = first + 4
                        self.set_continue()
                debugger = Debugger(backend=backend)
                def target():
                    x = 1
                    debugger.set_trace()
                    x = 99
                    return x
                first = target.__code__.co_firstlineno
                previous = sys.gettrace()
                try:
                    self.assertEqual(target(), 1)
                finally:
                    debugger.stop_trace()
                    sys.settrace(previous)

    def test_used_result_inspection_does_not_authorize_jump(self):
        for backend in ('settrace', 'monitoring'):
            with self.subTest(backend=backend):
                refused = []
                class Debugger(bdb.Bdb):
                    def user_opcode(inner, frame):
                        self.assertEqual(frame.f_locals['x'], 1)
                        with self.assertRaisesRegex(ValueError, 'active call recipe'):
                            frame.f_lineno = first + 4
                        refused.append(True)
                        inner.set_continue()
                debugger = Debugger(backend=backend)
                def target():
                    x = 1
                    result = debugger.set_trace()
                    x = 99
                    return x, result
                first = target.__code__.co_firstlineno
                previous = sys.gettrace()
                try:
                    self.assertEqual(target(), (99, None))
                finally:
                    debugger.stop_trace()
                    sys.settrace(previous)
                self.assertEqual(refused, [True])

    def test_async_entry_and_await_reentry(self):
        for backend in ('settrace', 'monitoring'):
            with self.subTest(backend=backend):
                entries, awaited = [], []
                async def command():
                    awaited.append(True)
                class Debugger(pdb.Pdb):
                    def user_entry(inner, frame):
                        self.assertIsNotNone(inner.async_shim_frame)
                        entries.append(frame.f_locals['x'])
                        inner.set_continue()
                        if len(entries) == 1:
                            inner.async_awaitable = command()
                debugger = Debugger(backend=backend, nosigint=True, readrc=False)
                async def target():
                    x = 1
                    await debugger.set_trace_async()
                    x = 2
                    return x
                previous = sys.gettrace()
                try:
                    self.assertEqual(asyncio.run(target()), 2)
                finally:
                    debugger.stop_trace()
                    sys.settrace(previous)
                self.assertEqual(entries, [1, 1])
                self.assertEqual(awaited, [True])
                self.assertIsNone(debugger.async_shim_frame)
                self.assertIsNone(debugger.async_awaitable)
                self.assertIsNone(debugger.enterframe)

    def test_entry_preserves_target_and_callback_exception(self):
        for backend in ('settrace', 'monitoring'):
            with self.subTest(backend=backend):
                observed = []
                class Debugger(bdb.Bdb):
                    def user_opcode(inner, frame):
                        inherited = sys.exception()
                        self.assertIsInstance(inherited, ZeroDivisionError)
                        try:
                            raise LookupError('callback owned')
                        except LookupError as error:
                            self.assertIs(sys.exception(), error)
                        self.assertIs(sys.exception(), inherited)
                        observed.append(inherited)
                        inner.set_continue()
                debugger = Debugger(backend=backend)
                def target():
                    try:
                        1 / 0
                    except ZeroDivisionError as error:
                        debugger.set_trace()
                        return error, sys.exception()
                previous = sys.gettrace()
                try:
                    error, active = target()
                finally:
                    debugger.stop_trace()
                    sys.settrace(previous)
                self.assertEqual(len(observed), 1)
                self.assertIs(error, active)
                self.assertIs(error, observed[0])


if __name__ == '__main__':
    unittest.main()
