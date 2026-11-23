"""Observation readers join optional positions with immutable recovery IDs."""

from contextlib import contextmanager
import sys
import types
import unittest

import _opcode
import _testinternalcapi

class Marker(Exception):
    pass


@contextmanager
def monitoring(code, callbacks):
    monitor = sys.monitoring
    tool = monitor.DEBUGGER_ID
    monitor.use_tool_id(tool, "semantic-reader")
    try:
        events = 0
        for event, callback in callbacks.items():
            monitor.register_callback(tool, event, callback)
            events |= event
        monitor.set_local_events(tool, code, events)
        yield
    finally:
        monitor.set_local_events(tool, code, 0)
        for event in callbacks:
            monitor.register_callback(tool, event, None)
        monitor.free_tool_id(tool)


class SemanticReaderTests(unittest.TestCase):
    def test_stripped_positions_keep_branch_error_and_deliver_no_line(self):
        def target(flag):
            try:
                if flag:
                    return "taken"
                return "not-taken"
            except Marker:
                return "handled"

        original = target.__code__
        code = original.replace(co_linetable=b"")
        # The IR, and with it the recovery entries, exists once lowered
        _testinternalcapi.get_ir(original)
        _testinternalcapi.get_ir(code)
        self.assertEqual(code.co_exceptiontable, original.co_exceptiontable)
        self.assertTrue(_opcode.get_exception_entries(code))
        self.assertEqual(_opcode.get_exception_entries(code),
                         _opcode.get_exception_entries(original))
        function = types.FunctionType(code, globals())
        for flag in (False, True):
            with self.subTest(flag=flag):
                branches = []
                lines = []

                def on_branch(observed, source, destination):
                    if observed is code and not branches:
                        branches.append((source, destination))
                        raise Marker("branch")

                def on_line(observed, line):
                    if observed is code:
                        lines.append(line)

                events = sys.monitoring.events
                with monitoring(code, {events.LINE: on_line,
                                       events.BRANCH_LEFT: on_branch,
                                       events.BRANCH_RIGHT: on_branch}):
                    self.assertEqual(function(flag), "handled")
                self.assertTrue(branches)
                self.assertEqual(lines, [])


if __name__ == "__main__":
    unittest.main()
