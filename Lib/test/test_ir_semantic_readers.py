"""Observation readers join optional positions with immutable recovery IDs."""

from contextlib import contextmanager
import dis
import sys
import types
import unittest

import _opcode
import _testinternalcapi

from test.support.ir_semantics import (
    observation_records, observation_tables, position_parts, position_table,
    semantic_parts, semantic_table,
)


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
    def test_missing_edge_positions_keep_real_backedge_trace(self):
        def target(values):
            total = 0
            for item in values: total += item
            return total

        original = target.__code__
        locations, _ = position_parts(original.co_linetable)
        exceptions, records = semantic_parts(original.co_exceptiontable)
        self.assertFalse(exceptions)
        physical = list(original.co_positions())
        backedge = next(row for row in dis.get_instructions(original)
                        if row.jump_target is not None and row.jump_target <= row.offset)
        self.assertEqual(physical[backedge.offset][0], physical[backedge.jump_target][0])
        # The compiler need not elide a source origin on this real backedge.
        # Add its semantic row explicitly, with no optional position row.
        records.append((backedge.offset, backedge.jump_target, 1, -1, 0, 0))
        code = original.replace(co_exceptiontable=semantic_table(records=records),
                                co_linetable=position_table(locations))
        control = code.replace(co_exceptiontable=semantic_table())

        def trace_lines(selected):
            seen = []

            def trace(frame, event, arg):
                if frame.f_code is selected and event == "line":
                    seen.append(frame.f_lineno)
                return trace

            previous = sys.gettrace()
            sys.settrace(trace)
            try:
                self.assertEqual(types.FunctionType(selected, globals())((1, 2, 3)), 6)
            finally:
                sys.settrace(previous)
            return seen

        expected = trace_lines(control)
        loop_line = original.co_firstlineno + 2
        self.assertGreaterEqual(expected.count(loop_line), 3)
        self.assertEqual(trace_lines(code), expected)
        self.assertEqual(_opcode.get_recovery_entries(code), tuple(records))

    def test_active_child_redirect_with_recovery_but_no_eh(self):
        def parent(child):
            child()
            pass
            return "complete"

        def child():
            value = 1
            return value

        code = parent.__code__
        exceptions, _ = semantic_parts(code.co_exceptiontable)
        self.assertFalse(exceptions)
        call = next(i for i in dis.get_instructions(code)
                    if i.handler == "CALL_NO_SELF")
        locations, _ = position_parts(code.co_linetable)
        records = observation_records(code)
        line = code.co_firstlineno + 3
        records.append((call.offset, call.offset + 1, 0,
                        line, line, -1, -1, -1, 0, 0))
        code = code.replace(**observation_tables(locations, records))
        self.assertTrue(code.co_exceptiontable)
        self.assertFalse(semantic_parts(code.co_exceptiontable)[0])
        target = types.FunctionType(code, globals())
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 2):
            self.assertEqual(target(child), "complete")
        changed = []

        def on_line(observed, lineno):
            if observed is child.__code__ and not changed:
                active = sys._getframe(1).f_back
                self.assertIs(active.f_code, code)
                active.f_lineno = line
                changed.append(True)

        with monitoring(child.__code__, {sys.monitoring.events.LINE: on_line}):
            self.assertEqual(target(child), "complete")
        self.assertEqual(changed, [True])

    def test_source_only_line_error_keeps_recovery_without_columns(self):
        def target(values):
            try:
                for item in values:
                    pass
            except Marker:
                return "handled"
            return "ordinary"

        code = target.__code__
        site = next(i for i in dis.get_instructions(code) if i.opname == "FOR_ITER")
        entry = next(row for row in _opcode.get_exception_entries(code)
                     if row[0] <= site.offset < row[1])
        exceptions, _ = semantic_parts(code.co_exceptiontable)
        locations, _ = position_parts(code.co_linetable)
        records = observation_records(code)
        source_line = code.co_firstlineno + 300
        records.append((site.offset, site.jump_target, 1,
                        source_line, source_line, -1, -1, *entry[2:]))
        supplied = observation_tables(locations, records, exceptions)
        code = code.replace(**supplied)
        locations, positions = position_parts(code.co_linetable)
        no_columns = [(line, end, -1, -1) for line, end, _, _ in positions]
        stripped = code.replace(co_linetable=position_table(locations, no_columns))
        self.assertEqual(stripped.co_exceptiontable, code.co_exceptiontable)
        self.assertEqual(_opcode.get_recovery_entries(stripped),
                         _opcode.get_recovery_entries(code))
        function = types.FunctionType(stripped, globals())
        seen = []

        def on_line(observed, line):
            if observed is stripped and line == source_line:
                seen.append(line)
                raise Marker("source-only line")

        with monitoring(stripped, {sys.monitoring.events.LINE: on_line}):
            self.assertEqual(function(()), "handled")
        self.assertEqual(seen, [source_line])

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
        self.assertEqual(code.co_exceptiontable, original.co_exceptiontable)
        self.assertTrue(_opcode.get_exception_entries(code))
        self.assertEqual(_opcode.get_recovery_entries(code),
                         _opcode.get_recovery_entries(original))
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
