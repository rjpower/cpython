"""Operand words: a call head followed by the words that name its inputs.

Lowering gives every call operand words, and the call handlers read
them.  The code under test never runs past a call that the test does not
mean to run: g is a function that fails the test.
"""

from test.support import ir_dis as dis
import marshal
import sys
import types
import unittest

import _opcode
from _opcode_metadata import _ir_handlers
from test.support import import_helper
from test.support.ir_dis import IRCode, encode_words

_testinternalcapi = import_helper.import_module('_testinternalcapi')

CALL_FAMILIES = ("Call", "CallKw", "CallEx")
CONTRACT = _opcode.get_ir_contract()
EMPTY = 0xFFF

# Line 3 holds a call with seven operands, two operand words, which the
# tests step over: it runs only if x is true.
SOURCE = """\
def f(x):
    if x:
        g(1, x, 3, 4, 5, 6, 7)
    return x

def one(x):
    return g(x)

def four(x):
    return g(x, x, x, x)

def five(x):
    return g(x, x, x, x, x)

def none():
    return g()

def guarded(x):
    try:
        g(1, x)
    except ValueError:
        pass
    return x

def cell():
    a = 1
    def inner():
        return a
    if a:
        g(1, 2)
    return inner
"""


def compile_functions():
    module = compile(SOURCE, "<operands>", "exec")
    return {c.co_name: c for c in module.co_consts if hasattr(c, "co_code")}


FUNCTIONS = compile_functions()


def rows(code, adaptive=False):
    return _testinternalcapi.get_ir(code, adaptive)


def calls(code):
    "(index, row, operand rows) of each call head that has operand words"
    result = []
    words = rows(code)
    for i, row in enumerate(words):
        if is_call_head(row[0]):
            n = row[3]
            result.append((i, row, words[i + 1:i + 1 + (n + 3) // 4]))
    return result


def is_call_head(name):
    handler, _, _ = name.partition("+")
    return handler.split(".")[0] in CALL_FAMILIES


class WordLengthTests(unittest.TestCase):
    def word(self, name, *fields):
        return int.from_bytes(encode_words((name, *fields)), "little")

    def test_length_of_a_call_head_follows_its_c_field(self):
        length = _opcode.get_ir_word_length
        for n, expected in [(0, 1), (1, 2), (4, 2), (5, 3), (8, 3), (9, 4),
                            (4095, 1 + 1024)]:
            with self.subTest(n=n):
                self.assertEqual(length(self.word("Call.Generic", 0, 0, n, 0)), expected)
                self.assertEqual(
                    length(self.word("Call.Generic+NO_SELF", 0, 0, n, 0)), expected)

    def test_every_other_word_has_length_one(self):
        length = _opcode.get_ir_word_length
        self.assertEqual(length(self.word("IR_OPERANDS", 1, 2, 3, 4)), 1)
        self.assertEqual(length(self.word("Return.Ordinary", 0, 0, 7, 0)), 1)

    def test_only_the_call_families_have_operand_words(self):
        length = _opcode.get_ir_word_length
        call_shaped = (_opcode.IR_OPERANDS_CALL, _opcode.IR_OPERANDS_KW,
                       _opcode.IR_OPERANDS_EX)
        for handler, row in enumerate(CONTRACT):
            # A span's length follows its count fields, not field C
            if row.operands == _opcode.IR_OPERANDS_SPANS:
                continue
            name = dis._all_opname[handler]
            with self.subTest(handler=name):
                is_call = row.operands in call_shaped
                self.assertEqual(is_call, name.split(".")[0] in CALL_FAMILIES)
                word = self.word(name, 0, 0, 7, 0)
                self.assertEqual(length(word), 3 if is_call else 1 + row.fixed_words)

    def test_operands_handler_is_never_dispatched(self):
        row = CONTRACT[_ir_handlers["IR_OPERANDS"]]
        self.assertEqual(row.roles, (_opcode.IR_ROLE_NONE,) * 4)
        self.assertEqual(row.word_flags, 0)


class EmissionTests(unittest.TestCase):
    def test_operand_words_follow_the_head(self):
        for name, nwords in [("one", 1), ("four", 1), ("five", 2),
                             ("none", 0)]:
            code = FUNCTIONS[name]
            with self.subTest(function=name):
                found = calls(code)
                self.assertEqual(len(found), 1)
                i, head, operands = found[0]
                self.assertEqual(len(operands), nwords)
                self.assertTrue(all(row[0] == "IR_OPERANDS" for row in operands))
                self.assertEqual(head[3], {"one": 1, "four": 4, "five": 5,
                                           "none": 0}[name])

    def test_operand_fields_hold_slots_and_the_rest_are_empty(self):
        code = FUNCTIONS["five"]
        (i, head, operands), = calls(code)
        slots = [s for row in operands for s in row[1:]]
        used, unused = slots[:5], slots[5:]
        self.assertEqual(unused, [EMPTY] * 3)
        # The arguments are the local x, named in place
        self.assertEqual(used, [code.co_varnames.index("x")] * 5)

    def test_seven_operands_take_two_words(self):
        (i, head, operands), = calls(FUNCTIONS["f"])
        self.assertEqual(head[3], 7)
        self.assertEqual(len(operands), 2)
        self.assertEqual(operands[1][4], EMPTY)

    def test_operand_words_belong_to_the_calls_instruction(self):
        for name in ("f", "five", "guarded"):
            code = FUNCTIONS[name]
            ir = IRCode(code)
            positions = list(ir.co_positions())
            self.assertEqual(len(positions), len(rows(code)))
            for i, head, operands in calls(code):
                for k in range(len(operands)):
                    self.assertEqual(positions[i + 1 + k], positions[i])
            lines = {}
            for start, end, line in ir.co_lines():
                for w in range(start, end):
                    lines[w] = line
            for i, head, operands in calls(code):
                for k in range(len(operands)):
                    self.assertEqual(lines[i + 1 + k], lines[i])

    def test_exception_ranges_hold_a_call_whole(self):
        code = FUNCTIONS["guarded"]
        operand_words = {i + 1 + k for i, _, ops in calls(code)
                         for k in range(len(ops))}
        entries = _opcode.get_exception_entries(code)
        self.assertTrue(entries)
        for start, end, target, slot, lasti, recipe in entries:
            self.assertNotIn(start, operand_words)
            self.assertNotIn(end, operand_words)
            self.assertNotIn(target, operand_words)
        (i, head, operands), = calls(code)
        self.assertTrue(any(start <= i and i + len(operands) < end
                            for start, end, *_ in entries))


class DisTests(unittest.TestCase):
    def test_one_instruction_per_call_and_its_operands(self):
        code = FUNCTIONS["f"]
        words = rows(code)
        instructions = list(dis.get_instructions(code))
        offset = 0
        for instruction in instructions:
            self.assertEqual(instruction.offset, offset)
            self.assertEqual(instruction.end_offset, offset + instruction.length)
            offset += instruction.length
        self.assertEqual(offset, len(words))
        self.assertEqual(len(instructions), sum(
            1 for row in words if row[0] != "IR_OPERANDS"))
        self.assertFalse(any(i.arm == "IR_OPERANDS" for i in instructions))

    def test_the_operands_are_listed_with_the_head(self):
        code = FUNCTIONS["f"]
        (i, head, operands), = calls(code)
        call = next(ins for ins in dis.get_instructions(code) if ins.offset == i)
        self.assertEqual(call.family, "Call")
        self.assertEqual(call.handler, "Call.Generic+NO_SELF")
        self.assertEqual(call.length, 1 + len(operands))
        slots = [s for row in operands for s in row[1:]][:head[3]]
        self.assertEqual(list(call.srcs[-head[3]:]), slots)
        text = self.dis(code)
        self.assertNotIn("IR_OPERANDS", text)
        self.assertIn(", ".join(f"r{s}" for s in slots), text)

    def dis(self, code):
        import io
        out = io.StringIO()
        dis.dis(code, file=out, show_offsets=True)
        return out.getvalue()

    def test_offsets_skip_the_operand_words(self):
        code = FUNCTIONS["f"]
        (i, head, operands), = calls(code)
        offsets = [ins.offset for ins in dis.get_instructions(code)]
        self.assertIn(i, offsets)
        for k in range(len(operands)):
            self.assertNotIn(i + 1 + k, offsets)
        self.assertIn(i + 1 + len(operands), offsets)

    def test_labels_never_land_on_operands(self):
        for code in FUNCTIONS.values():
            starts = {ins.offset for ins in dis.get_instructions(code)}
            for target in dis.findlabels(code):
                self.assertIn(target, starts)

    def test_adaptive_view_shows_the_same_instructions(self):
        code = FUNCTIONS["five"]
        plain = [(i.offset, i.length) for i in dis.get_instructions(code)]
        adaptive = [(i.offset, i.length)
                    for i in dis.get_instructions(code, adaptive=True)]
        self.assertEqual(plain, adaptive)


class LoweringTests(unittest.TestCase):
    def test_compiled_calls_lower_the_same_on_every_route(self):
        for name, code in FUNCTIONS.items():
            with self.subTest(function=name):
                for copy in (code.replace(),
                             marshal.loads(marshal.dumps(code))):
                    self.assertEqual(rows(copy), rows(code))


class WalkerTests(unittest.TestCase):
    def function(self, name="f"):
        code = FUNCTIONS[name].replace()
        return types.FunctionType(code, {"g": lambda *a: self.fail("ran g")})

    def test_line_events_skip_the_operand_words(self):
        function = self.function()
        code = function.__code__
        lines = []
        monitoring = sys.monitoring
        tool = monitoring.PROFILER_ID
        monitoring.use_tool_id(tool, "operand-words")
        self.addCleanup(monitoring.free_tool_id, tool)
        monitoring.register_callback(
            tool, monitoring.events.LINE,
            lambda code_, line: lines.append(line))
        self.addCleanup(monitoring.register_callback, tool,
                        monitoring.events.LINE, None)
        before = rows(code)
        monitoring.set_local_events(tool, code, monitoring.events.LINE)
        self.assertEqual(function(0), 0)
        monitoring.set_local_events(tool, code, 0)
        first = code.co_firstlineno
        self.assertEqual(lines, [first + 1, first + 3])
        self.assertEqual(rows(code), before)

    def test_instruction_instrumentation_leaves_operand_words_alone(self):
        code = FUNCTIONS["five"].replace()
        monitoring = sys.monitoring
        tool = monitoring.PROFILER_ID
        monitoring.use_tool_id(tool, "operand-words")
        self.addCleanup(monitoring.free_tool_id, tool)
        before = rows(code)
        events = (monitoring.events.INSTRUCTION | monitoring.events.LINE
                  | monitoring.events.CALL | monitoring.events.BRANCH)
        monitoring.set_local_events(tool, code, events)
        instrumented = rows(code, True)
        for k, (old, new) in enumerate(zip(before, instrumented)):
            if old[0] == "IR_OPERANDS":
                self.assertEqual(new, old)
        heads = [new for old, new in zip(before, instrumented)
                 if old[0] != "IR_OPERANDS"]
        # The words that monitoring put in place of heads are the
        # displacing arms
        self.assertTrue(any(row[0] in ("Line.Hook", "Instruction.Hook")
                            for row in heads))
        (i, head, operands), = calls(code)
        # A wrapper replaces the handler of the head and keeps its fields
        self.assertEqual(instrumented[i][1:], before[i][1:])
        monitoring.set_local_events(tool, code, 0)
        self.assertEqual(rows(code), before)

    def test_settrace_lines(self):
        function = self.function()
        first = function.__code__.co_firstlineno
        lines = []

        def tracer(frame, event, arg):
            if frame.f_code is function.__code__:
                if event == "line":
                    lines.append(frame.f_lineno)
                return tracer
        sys.settrace(tracer)
        try:
            function(0)
        finally:
            sys.settrace(None)
        self.assertEqual(lines, [first + 1, first + 3])

    def test_a_jump_passes_over_a_call_and_its_operands(self):
        function = self.function()
        code = function.__code__
        first = code.co_firstlineno

        def tracer(frame, event, arg):
            if frame.f_code is code and event == "line" \
                    and frame.f_lineno == first + 1:
                # Skip the call: its operand words are not instructions
                frame.f_lineno = first + 3
            return tracer
        sys.settrace(tracer)
        try:
            self.assertEqual(function(1), 1)
        finally:
            sys.settrace(None)

    def test_a_jump_to_a_line_after_a_call(self):
        function = self.function("guarded")
        code = function.__code__
        first = code.co_firstlineno
        seen = []

        def tracer(frame, event, arg):
            if frame.f_code is code and event == "line":
                seen.append(frame.f_lineno)
                if frame.f_lineno == first + 2:
                    # g(...) -> return x, over the call and its handler
                    frame.f_lineno = first + 5
            return tracer
        sys.settrace(tracer)
        try:
            self.assertEqual(function(5), 5)
        finally:
            sys.settrace(None)
        # g would have failed the test had its call run
        self.assertEqual(seen, [first + 1, first + 2])

    def test_instruction_walk_reaches_every_head_once(self):
        # dis and the line table agree on where each instruction starts
        for name, code in FUNCTIONS.items():
            with self.subTest(function=name):
                starts = {ins.offset for ins in dis.get_instructions(code)}
                for start, end, line in IRCode(code).co_lines():
                    self.assertIn(start, starts)
                    self.assertTrue(end in starts or end == len(rows(code)))


if __name__ == "__main__":
    unittest.main()
