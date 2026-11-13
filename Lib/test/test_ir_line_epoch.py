"""LINE handoffs depend on LINE configuration, not unrelated instrumentation."""
import struct
import sys
import unittest


class LineEpochTests(unittest.TestCase):
    def assert_source_body(self, target):
        magic, version, size, records = struct.unpack_from('<4sIII', target.__code__.co_linetable)
        self.assertEqual((magic, version), (b'IRSM', 2))
        self.assertGreater(records, 0)

    def test_instruction_enable_does_not_repeat_source_line(self):
        def target(value):
            value
        self.assert_source_body(target)
        mon, tool = sys.monitoring, 4
        lines, opcodes = [], []
        def instruction(code, offset):
            opcodes.append(offset)
        def line(code, number):
            lines.append(number - code.co_firstlineno)
            mon.set_local_events(tool, code, mon.events.LINE | mon.events.INSTRUCTION)
        mon.use_tool_id(tool, 'line-epoch-test')
        try:
            mon.register_callback(tool, mon.events.LINE, line)
            mon.register_callback(tool, mon.events.INSTRUCTION, instruction)
            mon.set_local_events(tool, target.__code__, mon.events.LINE)
            target(object())
        finally:
            mon.set_local_events(tool, target.__code__, 0)
            mon.register_callback(tool, mon.events.LINE, None)
            mon.register_callback(tool, mon.events.INSTRUCTION, None)
            mon.free_tool_id(tool)
        self.assertEqual(lines, [1])
        self.assertTrue(opcodes)

    def test_reused_line_tool_resets_disabled_source_decisions(self):
        for global_events in (False, True):
            with self.subTest(global_events=global_events):
                def target(value):
                    value
                target.__code__ = target.__code__.replace()
                self.assert_source_body(target)
                mon, tool = sys.monitoring, 4
                old, new = [], []
                def disabled(code, number):
                    if code is target.__code__:
                        old.append(number - code.co_firstlineno)
                        return mon.DISABLE
                def replacement(code, number):
                    if code is target.__code__:
                        new.append(number - code.co_firstlineno)
                def enable():
                    if global_events:
                        mon.set_events(tool, mon.events.LINE)
                    else:
                        mon.set_local_events(tool, target.__code__, mon.events.LINE)
                acquired = False
                try:
                    mon.use_tool_id(tool, 'line-epoch-old')
                    acquired = True
                    mon.register_callback(tool, mon.events.LINE, disabled)
                    enable()
                    target(object())
                    target(object())
                    self.assertEqual(old, [1])
                    # The target is inactive while the same numeric ID gets a
                    # new identity. Its old code must not retain DISABLE bits.
                    mon.clear_tool_id(tool)
                    mon.free_tool_id(tool)
                    acquired = False
                    mon.use_tool_id(tool, 'line-epoch-new')
                    acquired = True
                    mon.register_callback(tool, mon.events.LINE, replacement)
                    enable()
                    target(object())
                    self.assertEqual(new, [1])
                finally:
                    if acquired:
                        mon.set_events(tool, 0)
                        mon.set_local_events(tool, target.__code__, 0)
                        mon.register_callback(tool, mon.events.LINE, None)
                        mon.free_tool_id(tool)

    def test_callback_replacement_does_not_replay_current_source_line(self):
        def target(value):
            value
        self.assert_source_body(target)
        mon, tool = sys.monitoring, 4
        seen = []
        def replacement(code, number):
            seen.append(('new', number - code.co_firstlineno))
        def original(code, number):
            seen.append(('old', number - code.co_firstlineno))
            mon.register_callback(tool, mon.events.LINE, replacement)
        mon.use_tool_id(tool, 'line-epoch-test')
        try:
            mon.register_callback(tool, mon.events.LINE, original)
            mon.set_local_events(tool, target.__code__, mon.events.LINE)
            target(object())
            self.assertEqual(seen, [('old', 1)])
            target(object())
        finally:
            mon.set_local_events(tool, target.__code__, 0)
            mon.register_callback(tool, mon.events.LINE, None)
            mon.free_tool_id(tool)
        self.assertEqual(seen, [('old', 1), ('new', 1)])

    def test_callback_replacement_keeps_disable_until_restart(self):
        def target(value):
            value
        mon, tool = sys.monitoring, 4
        old, new = [], []
        def disabled(code, number):
            old.append(number)
            return mon.DISABLE
        def replacement(code, number):
            new.append(number)
        mon.use_tool_id(tool, 'line-epoch-test')
        try:
            mon.register_callback(tool, mon.events.LINE, disabled)
            mon.set_local_events(tool, target.__code__, mon.events.LINE)
            target(object())
            mon.register_callback(tool, mon.events.LINE, replacement)
            target(object())
            self.assertEqual(new, [])
            mon.restart_events()
            target(object())
        finally:
            mon.set_local_events(tool, target.__code__, 0)
            mon.register_callback(tool, mon.events.LINE, None)
            mon.free_tool_id(tool)
        self.assertEqual(len(old), 1)
        self.assertEqual(new, [target.__code__.co_firstlineno + 1])


if __name__ == '__main__':
    unittest.main()
