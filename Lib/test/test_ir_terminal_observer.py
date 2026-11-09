"""Terminal observers see published results and cannot strand C owners."""

import dis
import gc
import sys
import unittest
import weakref


class TerminalObserverTests(unittest.TestCase):
    def test_jump_callbacks_keep_actual_site_and_preserve_errors(self):
        class HookError(Exception):
            pass
        def consume():
            result = 0
            try:
                count = 3
                while count:
                    if count & 1:
                        result += 1
                    else:
                        result += 2
                    count -= 1
                return result
            except HookError:
                return "caught"

        jumps = {i.offset: i.opname for i in dis.get_instructions(consume)
                 if i.opname in ('JUMP_FORWARD', 'JUMP_BACKWARD')}
        self.assertEqual(set(jumps.values()), {'JUMP_FORWARD', 'JUMP_BACKWARD'})
        mon, tool = sys.monitoring, 4
        for failure in (None, 'JUMP_FORWARD', 'JUMP_BACKWARD'):
            with self.subTest(failure=failure):
                seen = []
                def jump(code, source, destination):
                    if code is consume.__code__ and source in jumps:
                        frame = sys._getframe(1)
                        self.assertEqual(frame.f_lasti, source)
                        seen.append(jumps[source])
                        if jumps[source] == failure:
                            raise HookError
                mon.use_tool_id(tool, 'terminal-jump')
                try:
                    mon.register_callback(tool, mon.events.JUMP, jump)
                    mon.set_local_events(tool, consume.__code__, mon.events.JUMP)
                    self.assertEqual(consume(), 4 if failure is None else "caught")
                finally:
                    mon.set_local_events(tool, consume.__code__, 0)
                    mon.register_callback(tool, mon.events.JUMP, None)
                    mon.free_tool_id(tool)
                if failure is None:
                    self.assertEqual(set(seen), {'JUMP_FORWARD', 'JUMP_BACKWARD'})
                else:
                    self.assertIn(failure, seen)

    def test_pop_iter_callback_error_releases_detached_iterator(self):
        references = []
        class HookError(Exception):
            pass
        class Iterator:
            def __init__(self):
                references.append(weakref.ref(self))
            def __iter__(self):
                return self
            def __next__(self):
                return True
        def consume():
            try:
                return any(value for value in Iterator())
            except HookError:
                return "caught"

        offsets = {i.offset for i in dis.get_instructions(consume)
                   if i.opname == 'POP_ITER'}
        self.assertTrue(offsets)
        mon, tool = sys.monitoring, 4
        seen = []
        def branch(code, source, destination):
            if code is consume.__code__:
                frame = sys._getframe(1)
                if frame.f_lasti in offsets:
                    seen.append((frame.f_lasti, source))
                    raise HookError
        mon.use_tool_id(tool, 'terminal-pop-iter')
        try:
            mon.register_callback(tool, mon.events.BRANCH_RIGHT, branch)
            mon.set_local_events(tool, consume.__code__, mon.events.BRANCH_RIGHT)
            for _ in range(3):
                self.assertEqual(consume(), "caught")
        finally:
            mon.set_local_events(tool, consume.__code__, 0)
            mon.register_callback(tool, mon.events.BRANCH_RIGHT, None)
            mon.free_tool_id(tool)
        self.assertEqual(len(seen), 3)
        self.assertEqual(len(references), 3)
        gc.collect()
        self.assertTrue(all(reference() is None for reference in references))

    def test_or_pop_callback_error_releases_retained_and_dropped_values(self):
        for truth in (False, True):
            with self.subTest(truth=truth):
                references = []
                class Resource:
                    def __init__(self):
                        references.append(weakref.ref(self))
                    def __bool__(self):
                        return truth
                class HookError(Exception):
                    pass
                def consume():
                    try:
                        return Resource() or "fallback"
                    except HookError:
                        return "caught"
                consume.__code__ = consume.__code__.replace()
                offsets = {i.offset for i in dis.get_instructions(consume)
                           if i.opname == 'JUMP_IF_TRUE_OR_POP'}
                self.assertTrue(offsets)
                mon, tool = sys.monitoring, 4
                event = mon.events.BRANCH_RIGHT if truth else mon.events.BRANCH_LEFT
                seen = []
                def branch(code, source, destination):
                    if code is consume.__code__ and source in offsets:
                        seen.append(source)
                        raise HookError
                mon.use_tool_id(tool, 'terminal-or-pop-owner')
                try:
                    mon.register_callback(tool, event, branch)
                    mon.set_local_events(tool, consume.__code__, event)
                    for _ in range(3):
                        self.assertEqual(consume(), "caught")
                finally:
                    mon.set_local_events(tool, consume.__code__, 0)
                    mon.register_callback(tool, event, None)
                    mon.free_tool_id(tool)
                self.assertEqual(len(seen), 3)
                self.assertEqual(len(references), 3)
                gc.collect()
                self.assertTrue(all(reference() is None for reference in references))

    def test_for_body_callback_error_releases_produced_item(self):
        references = []
        class Resource:
            pass
        class HookError(Exception):
            pass
        class Iterator:
            def __iter__(self):
                return self
            def __next__(self):
                value = Resource()
                references.append(weakref.ref(value))
                return value
        def consume():
            try:
                for value in Iterator():
                    return "body"
            except HookError:
                return "caught"

        offsets = {i.offset for i in dis.get_instructions(consume)
                   if i.opname == 'FOR_ITER'}
        self.assertTrue(offsets)
        mon, tool = sys.monitoring, 4
        seen = []
        def branch(code, source, destination):
            if code is consume.__code__ and source in offsets:
                seen.append(source)
                raise HookError
        mon.use_tool_id(tool, 'terminal-for-owner')
        try:
            mon.register_callback(tool, mon.events.BRANCH_LEFT, branch)
            mon.set_local_events(tool, consume.__code__, mon.events.BRANCH_LEFT)
            for _ in range(3):
                self.assertEqual(consume(), "caught")
        finally:
            mon.set_local_events(tool, consume.__code__, 0)
            mon.register_callback(tool, mon.events.BRANCH_LEFT, None)
            mon.free_tool_id(tool)
        self.assertEqual(len(seen), 3)
        self.assertEqual(len(references), 3)
        gc.collect()
        self.assertTrue(all(reference() is None for reference in references))

    def test_for_body_callback_sees_folded_local(self):
        value = object()
        def consume(values):
            current = None
            for current in values:
                return current
        offsets = {i.offset for i in dis.get_instructions(consume)
                   if i.opname == 'FOR_ITER'}
        observed = []
        mon, tool = sys.monitoring, 4
        def branch(code, source, destination):
            if code is consume.__code__ and source in offsets:
                frame = sys._getframe(1)
                self.assertIs(frame.f_code, code)
                observed.append(frame.f_locals['current'])
        mon.use_tool_id(tool, 'terminal-for-local')
        try:
            mon.register_callback(tool, mon.events.BRANCH_LEFT, branch)
            mon.set_local_events(tool, consume.__code__, mon.events.BRANCH_LEFT)
            self.assertIs(consume([value]), value)
        finally:
            mon.set_local_events(tool, consume.__code__, 0)
            mon.register_callback(tool, mon.events.BRANCH_LEFT, None)
            mon.free_tool_id(tool)
        self.assertEqual(observed, [value])

    def test_for_displaced_owner_observes_committed_binding(self):
        value = object()
        retired = []
        class Displaced:
            def __init__(self):
                self.frame = sys._getframe(1)
            def __del__(self):
                retired.append(self.frame.f_locals['current'])
        def consume(values):
            current = Displaced()
            for current in values:
                return current
        offsets = {i.offset for i in dis.get_instructions(consume)
                   if i.opname == 'FOR_ITER'}
        observed = []
        mon, tool = sys.monitoring, 4
        def branch(code, source, destination):
            if code is consume.__code__ and source in offsets:
                observed.append(sys._getframe(1).f_locals['current'])
        mon.use_tool_id(tool, 'terminal-displaced-owner')
        try:
            mon.register_callback(tool, mon.events.BRANCH_LEFT, branch)
            mon.set_local_events(tool, consume.__code__, mon.events.BRANCH_LEFT)
            self.assertIs(consume([value]), value)
        finally:
            mon.set_local_events(tool, consume.__code__, 0)
            mon.register_callback(tool, mon.events.BRANCH_LEFT, None)
            mon.free_tool_id(tool)
        gc.collect()
        self.assertEqual(observed, [value])
        self.assertEqual(retired, [value])

    def test_for_body_line_landing_without_source_only_span(self):
        def consume(values):
            result = []
            for value in values:
                result.append(value)
            return result
        events = []
        def trace(frame, event, arg):
            if frame.f_code is consume.__code__:
                if event == 'line':
                    events.append(frame.f_lineno - frame.f_code.co_firstlineno)
                return trace
            return None
        previous = sys.gettrace()
        sys.settrace(trace)
        try:
            self.assertEqual(consume([1, 2]), [1, 2])
        finally:
            sys.settrace(previous)
        self.assertEqual(events, [1, 2, 3, 2, 3, 2, 4])


if __name__ == '__main__':
    unittest.main()
