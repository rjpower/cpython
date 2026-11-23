"""Public offsets are byte offsets into co_code, though the frame runs the
register IR that lowering builds from it."""

import dis
import sys
import traceback
import unittest
import warnings

import _testinternalcapi


def stack_offsets(code):
    return {instr.offset for instr in dis.get_instructions(code)}


def instruction_at(code, offset):
    return next(instr for instr in dis.get_instructions(code)
                if instr.offset == offset)


def probe(count):
    total = 0
    for i in range(count):
        if i % 2:
            total += sys._getframe().f_lasti
    return total


def raises(x):
    y = x + 1
    return 1 / (y - y)


def branchy(xs):
    n = 0
    for x in xs:
        if x:
            n += 1
        else:
            n -= 1
    return n


class OffsetTests(unittest.TestCase):
    def test_f_lasti_names_the_call_in_co_code(self):
        def f():
            return sys._getframe().f_lasti

        lasti = f()
        # The frame is executing the attribute load
        self.assertIn(lasti, stack_offsets(f.__code__))
        self.assertEqual(instruction_at(f.__code__, lasti).argval, "f_lasti")
        self.assertLess(lasti, len(f.__code__.co_code))

    def test_tb_lasti_names_the_failing_instruction(self):
        for _ in range(3):
            try:
                raises(1)
            except ZeroDivisionError as exc:
                tb = exc.__traceback__.tb_next
                self.assertEqual(
                    instruction_at(raises.__code__, tb.tb_lasti).opname,
                    "BINARY_OP")
                self.assertEqual(tb.tb_lineno, raises.__code__.co_firstlineno + 2)

    def test_traceback_positions_match_co_positions(self):
        try:
            raises(1)
        except ZeroDivisionError as exc:
            tb = exc.__traceback__.tb_next
            position = list(raises.__code__.co_positions())[tb.tb_lasti // 2]
            frame = traceback.extract_tb(exc.__traceback__)[-1]
            self.assertEqual(frame.lineno, tb.tb_lineno)
            self.assertEqual((frame.lineno, frame.end_lineno, frame.colno,
                              frame.end_colno), position)

    def test_co_lines_and_positions_are_per_code_unit(self):
        code = branchy.__code__
        branchy([1, 0])
        positions = list(code.co_positions())
        self.assertEqual(len(positions), len(code.co_code) // 2)
        lines = list(code.co_lines())
        self.assertEqual(lines[0][0], 0)
        self.assertEqual(lines[-1][1], len(code.co_code))
        for start, end, line in lines:
            self.assertIn(start, stack_offsets(code))
            self.assertEqual(start % 2, 0)
            if line is not None:
                self.assertEqual(positions[start // 2][0], line)

    def test_co_branches_are_stack_offsets(self):
        code = branchy.__code__
        branchy([1, 0])
        offsets = stack_offsets(code)
        branches = list(code.co_branches())
        self.assertTrue(branches)
        for source, left, right in branches:
            self.assertIn(source, offsets)
            self.assertIn(left, offsets)
            self.assertIn(right, offsets)
            self.assertTrue(instruction_at(code, source).opname.startswith(
                ("POP_JUMP_IF", "FOR_ITER", "SEND")))

    def test_f_lineno_assignment(self):
        lines = []

        def tracer(frame, event, arg):
            if frame.f_code is jumper.__code__ and event == "line":
                lines.append(frame.f_lineno)
                if len(lines) == 1:
                    frame.f_lineno += 2
            return tracer

        def jumper():
            a = 1
            a = 2
            a = 3
            return a

        base = jumper.__code__.co_firstlineno
        sys.settrace(tracer)
        try:
            with warnings.catch_warnings():
                # The jump leaves `a` unbound, which the frame reports
                warnings.simplefilter("ignore", RuntimeWarning)
                self.assertEqual(jumper(), 3)
        finally:
            sys.settrace(None)
        # The jump skips two statements; upstream reports no line event for
        # its target either
        self.assertEqual(lines, [base + 1, base + 4])

    def test_opcode_events_report_stack_offsets(self):
        events = []

        def tracer(frame, event, arg):
            if frame.f_code is probe.__code__:
                frame.f_trace_opcodes = True
                if event == "opcode":
                    events.append(frame.f_lasti)
            return tracer

        probe(4)
        sys.settrace(tracer)
        try:
            probe(4)
        finally:
            sys.settrace(None)
        self.assertTrue(events)
        self.assertLessEqual(set(events), stack_offsets(probe.__code__))

    def test_monitoring_offsets_are_stack_offsets(self):
        E = sys.monitoring.events
        tool = sys.monitoring.DEBUGGER_ID
        code = branchy.__code__
        offsets = stack_offsets(code)
        seen = {"instruction": set(), "branch": set(), "jump": set()}

        def instruction(code_arg, offset):
            seen["instruction"].add(offset)

        def branch(code_arg, source, destination):
            seen["branch"].update((source, destination))

        def jump(code_arg, source, destination):
            seen["jump"].update((source, destination))

        branchy([1, 0])
        sys.monitoring.use_tool_id(tool, "test_ir_stack_offsets")
        try:
            sys.monitoring.register_callback(tool, E.INSTRUCTION, instruction)
            sys.monitoring.register_callback(tool, E.BRANCH, branch)
            sys.monitoring.register_callback(tool, E.JUMP, jump)
            sys.monitoring.set_local_events(
                tool, code, E.INSTRUCTION | E.BRANCH | E.JUMP)
            self.assertEqual(branchy([1, 0, 1]), 1)
        finally:
            sys.monitoring.set_local_events(tool, code, 0)
            for event in (E.INSTRUCTION, E.BRANCH, E.JUMP):
                sys.monitoring.register_callback(tool, event, None)
            sys.monitoring.free_tool_id(tool)
        for kind, found in seen.items():
            with self.subTest(kind=kind):
                self.assertTrue(found)
                self.assertLessEqual(found, offsets)

    def test_frame_offsets_do_not_need_the_frame_to_have_run_ir(self):
        # A code object that has never run still answers for its offsets
        source = "def f(a):\n    if a:\n        return 1\n    return 2\n"
        namespace = {}
        exec(source, namespace)
        code = namespace["f"].__code__
        self.assertTrue(list(code.co_branches()))
        self.assertEqual(len(list(code.co_positions())), len(code.co_code) // 2)
        self.assertEqual(code.co_lines().__next__()[0], 0)
        _testinternalcapi.get_ir(code)
        self.assertEqual(len(list(code.co_positions())), len(code.co_code) // 2)


if __name__ == "__main__":
    unittest.main()
