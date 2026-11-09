"""Pending source exception recipes survive cleanup that reenters Python."""

import sys
import unittest

from test.support import import_helper

_testinternalcapi = import_helper.import_module('_testinternalcapi')

SOURCE = '''
def nested():
    try:
        pass  # nested event
    except InnerError:
        events.append("inner caught")

class Returned:
    def __del__(self):
        events.append("retire")
        nested()
        events.append("retired")

def values():
    yield 1
    return Returned()

def outer():
    for item in values():
        pass
    else:
        try:
            pass  # outer event
        except OuterError:
            events.append("outer caught")
'''


class OuterError(Exception):
    pass


class InnerError(Exception):
    pass


class SourceReentryTests(unittest.TestCase):
    def test_same_pc_source_redirect_replays_selected_instruction(self):
        for warmup in (0, _testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            for action in ('none', 'getter', 'redirect'):
                with self.subTest(warmup=warmup, action=action):
                    def target():
                        x = 1
                        pass
                        return x
                    target.__code__ = target.__code__.replace()
                    first = target.__code__.co_firstlineno
                    for _ in range(warmup):
                        target()
                    commits = []
                    applied = False

                    def trace(frame, event, arg):
                        nonlocal applied
                        if (frame.f_code is target.__code__ and event == 'line'
                                and frame.f_lineno == first + 2 and not applied):
                            applied = True
                            frame.f_locals['x'] = 99
                            if action == 'redirect':
                                before = frame.f_lasti
                                frame.f_lineno = first + 1
                                commits.append((before, frame.f_lasti))
                            elif action == 'getter':
                                frame.f_lineno
                        return trace

                    old_trace = sys.gettrace()
                    try:
                        sys.settrace(trace)
                        result = target()
                    finally:
                        sys.settrace(old_trace)
                    self.assertTrue(applied)
                    self.assertEqual(result, 1 if action == 'redirect' else 99)
                    if action == 'redirect':
                        self.assertEqual(len(commits), 1)
                        self.assertEqual(commits[0][0], commits[0][1])
                    else:
                        self.assertEqual(commits, [])

    def test_same_pc_source_redirect_error_uses_committed_position(self):
        for warmup in (0, _testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            for redirect in (False, True):
                with self.subTest(warmup=warmup, redirect=redirect):
                    def target():
                        x = 1
                        pass
                        return x
                    target.__code__ = target.__code__.replace()
                    first = target.__code__.co_firstlineno
                    for _ in range(warmup):
                        target()
                    commits = []

                    def trace(frame, event, arg):
                        if (frame.f_code is target.__code__ and event == 'line'
                                and frame.f_lineno == first + 2):
                            if redirect:
                                before = frame.f_lasti
                                frame.f_lineno = first + 1
                                commits.append((before, frame.f_lasti))
                            raise OuterError('source callback')
                        return trace

                    old_trace = sys.gettrace()
                    try:
                        sys.settrace(trace)
                        try:
                            target()
                        except OuterError as error:
                            traceback = error.__traceback__
                        else:
                            self.fail('source callback did not raise')
                    finally:
                        sys.settrace(old_trace)
                    while traceback.tb_frame.f_code is not target.__code__:
                        traceback = traceback.tb_next
                    self.assertEqual(traceback.tb_lineno, first + (1 if redirect else 2))
                    if redirect:
                        self.assertEqual(len(commits), 1)
                        self.assertEqual(commits[0][0], commits[0][1])
                    else:
                        self.assertEqual(commits, [])

    def test_fused_branch_preserves_source_only_lines(self):
        for condition, handler, yes, no in (
            ('value == marker', 'COMPARE_OP_JUMP', (1, 1), (0, 1)),
            ('value in marker', 'CONTAINS_OP_JUMP', (1, (1,)), (0, (1,))),
            ('value is marker', 'IS_OP_JUMP', (None, None), (0, None)),
        ):
            with self.subTest(condition=condition):
                source = (f'def target(value, marker):\n'
                          f'    if {condition}:\n'
                          '        pass\n'
                          '        if False:\n'
                          '            return 10\n'
                          '    else:\n'
                          '        pass\n'
                          '        if False:\n'
                          '            return 20\n')
                namespace = {}
                exec(compile(source, '<fused-source-lines>', 'exec'), namespace)
                target = namespace['target']
                self.assertIn(handler, [word[0] for word in
                                        _testinternalcapi.get_ir(target.__code__)])
                for arguments, expected in ((yes, [1, 2, 3]), (no, [1, 6, 7])):
                    events = []
                    def trace(frame, event, arg):
                        if frame.f_code is target.__code__ and event == 'line':
                            events.append(frame.f_lineno - target.__code__.co_firstlineno)
                        return trace
                    old_trace = sys.gettrace()
                    try:
                        sys.settrace(trace)
                        self.assertIsNone(target(*arguments))
                    finally:
                        sys.settrace(old_trace)
                    self.assertEqual(events, expected)

    def test_fused_branch_source_error_uses_source_handler(self):
        for condition, handler, yes, no in (
            ('value == marker', 'COMPARE_OP_JUMP', (1, 1), (0, 1)),
            ('value in marker', 'CONTAINS_OP_JUMP', (1, (1,)), (0, (1,))),
            ('value is marker', 'IS_OP_JUMP', (None, None), (0, None)),
        ):
            with self.subTest(condition=condition):
                source = (f'def target(value, marker):\n'
                          '    try:\n'
                          f'        if {condition}:\n'
                          '            pass\n'
                          '            if False:\n'
                          '                return 10\n'
                          '        else:\n'
                          '            pass\n'
                          '            if False:\n'
                          '                return 20\n'
                          '    except OuterError:\n'
                          '        return "caught"\n')
                namespace = {'OuterError': OuterError}
                exec(compile(source, '<fused-source-errors>', 'exec'), namespace)
                target = namespace['target']
                self.assertIn(handler, [word[0] for word in
                                        _testinternalcapi.get_ir(target.__code__)])
                for arguments, source_line in ((yes, 3), (no, 7)):
                    events = []
                    def trace(frame, event, arg):
                        if (frame.f_code is target.__code__ and event == 'line'
                                and frame.f_lineno == target.__code__.co_firstlineno + source_line):
                            events.append(source_line)
                            raise OuterError('fused source callback')
                        return trace
                    old_trace = sys.gettrace()
                    try:
                        sys.settrace(trace)
                        self.assertEqual(target(*arguments), 'caught')
                    finally:
                        sys.settrace(old_trace)
                    self.assertEqual(events, [source_line])

    def test_nested_callback_error_preserves_outer_recipe(self):
        outer_line = next(i for i, line in enumerate(SOURCE.splitlines(), 1)
                          if '# outer event' in line)
        inner_line = next(i for i, line in enumerate(SOURCE.splitlines(), 1)
                          if '# nested event' in line)
        for warmup in (0, _testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            for nested_failure in (False, True):
                with self.subTest(warmup=warmup, nested_failure=nested_failure):
                    events = []
                    namespace = dict(events=events, OuterError=OuterError,
                                     InnerError=InnerError)
                    exec(compile(SOURCE, '<source-reentry>', 'exec'), namespace)
                    outer, nested = namespace['outer'], namespace['nested']
                    for _ in range(warmup):
                        outer()
                    events.clear()

                    def callback(code, line):
                        if code is outer.__code__ and line == outer_line:
                            events.append('outer event')
                            raise OuterError('outer callback')
                        if nested_failure and code is nested.__code__ and line == inner_line:
                            events.append('inner event')
                            raise InnerError('inner callback')

                    tool = 2
                    sys.monitoring.use_tool_id(tool, 'source-reentry')
                    try:
                        sys.monitoring.register_callback(
                            tool, sys.monitoring.events.LINE, callback)
                        for function in (outer, nested):
                            sys.monitoring.set_local_events(
                                tool, function.__code__, sys.monitoring.events.LINE)
                        outer()
                    finally:
                        for function in (outer, nested):
                            sys.monitoring.set_local_events(tool, function.__code__, 0)
                        sys.monitoring.register_callback(tool, sys.monitoring.events.LINE, None)
                        sys.monitoring.free_tool_id(tool)
                    expected = ['outer event', 'retire']
                    if nested_failure:
                        expected += ['inner event', 'inner caught']
                    expected += ['retired', 'outer caught']
                    self.assertEqual(events, expected)


if __name__ == '__main__':
    unittest.main()
