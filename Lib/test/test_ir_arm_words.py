"""Generic observation of words whose handler is a register instruction arm.

_testinternalcapi.force_arm() makes the word of a stack op run an arm before
its translation rule exists.  Line and instruction events, sys.settrace line
events and f_lineno jumps must report what they report for the ordinary
word, with PYTHON_JIT off and always.  Each scenario runs in a child process
twice, with and without the forced arm, and the two outputs must agree.
"""

import _opcode
import dis
import json
import types
import textwrap
import unittest

from pathlib import Path
from test import test_tools
from test.support import import_helper, ir_dis
from test.support.script_helper import assert_python_ok

capi = import_helper.import_module("_testinternalcapi")

SCRIPT = """
import dis
import json
import sys
import _testinternalcapi as capi

FORCE = sys.argv[1] == "1"
MON = sys.monitoring


def g(a, b):
    x = 0
    x = a + b + 1
    x = x + 10
    return x


FIRST = g.__code__.co_firstlineno


def arm_words(code):
    return sum(1 for w in capi.get_ir(code, True) if w[0] == "Add.Int")


def prepare():
    # force_arm builds the executing copy and rewrites its BINARY_OP words
    # before the first call, so compiled code is made from the arm words.
    if FORCE:
        for ins in dis.get_instructions(g):
            if ins.opname == "BINARY_OP":
                capi.force_arm(g.__code__, ins.offset, "Add.Int")
        assert arm_words(g.__code__) == 3, capi.get_ir(g.__code__, True)


def monitored(event, callback):
    MON.use_tool_id(3, "arms")
    MON.register_callback(3, event, callback)
    MON.set_events(3, event)
    try:
        return g(1, 2)
    finally:
        MON.set_events(3, 0)
        MON.free_tool_id(3)


def tracing(tracer):
    sys.settrace(tracer)
    try:
        return g(1, 2)
    finally:
        sys.settrace(None)


results = {}
prepare()
results["plain"] = g(1, 2)
# Compiled code made from the arm words: the unit stays valid for the run
results["compiled"] = [capi.jit_compile(g.__code__), g(1, 2)]

lines = []
present = []
def on_line(code, line):
    if code is g.__code__:
        lines.append(line - FIRST)
        present.append(arm_words(code))
results["line"] = [monitored(MON.events.LINE, on_line), lines]
results["line_arms_seen"] = max(present) if FORCE else 0

offsets = []
def on_instruction(code, offset):
    if code is g.__code__:
        offsets.append(offset)
results["instruction"] = [monitored(MON.events.INSTRUCTION, on_instruction), offsets]

traced = []
def tracer(frame, event, arg):
    if frame.f_code is g.__code__:
        if event == "line":
            traced.append(frame.f_lineno - FIRST)
        return tracer
    return None
results["settrace"] = [tracing(tracer), traced]


def jumper(source, target, times):
    seen = []
    def trace(frame, event, arg):
        if frame.f_code is not g.__code__:
            return None
        if event == "line" and frame.f_lineno - FIRST == source and times[0]:
            times[0] -= 1
            seen.append(target)
            frame.f_lineno = FIRST + target
        elif event == "line":
            seen.append(frame.f_lineno - FIRST)
        return trace
    return [tracing(trace), seen]

# onto the word (the second addition's line) and over the words
results["jump_onto"] = jumper(2, 3, [1])
results["jump_over"] = jumper(2, 4, [1])
# back onto the first addition, once, from the return
results["jump_back"] = jumper(4, 2, [1])
# jump with the arm words in place, then run without observation
results["after"] = g(5, 6)
print(json.dumps(results))
"""


# An arm with a fixed operand word, planted in a branch that never runs, over
# the word of the second load of a chain of three on one line, which no
# source edge names.  argv[1] is the word's low 16 bits: "decoy" is
# the handler of Return.Ordinary, which a walker that reads the word as an
# instruction would misread or rewrite, and "marker" is the inert IR_OPERANDS
# handler that the operand words of calls carry.
OPERAND_SCRIPT = """
import dis
import json
import struct
import sys
import _testinternalcapi as capi

MON = sys.monitoring
from _opcode_metadata import _ir_handlers

LOW = {"decoy": _ir_handlers["Return.Ordinary"], "marker": _ir_handlers["IR_OPERANDS"]}


def gen(flag, o):
    x = 0
    if flag:
        o.real.real.real
    yield x
    yield x + 1


code = gen.__code__
offset = [i.offset for i in dis.get_instructions(gen) if i.opname == "LOAD_ATTR"][0]
# The operand word's low 16 bits are the site's feedback unit, which the
# GetAttr word that the plant replaces names in D: a unit the code has no
# entry for would send warm-up writes outside the feedback block.  The
# decoy or marker value sits above them.
site = next(w for w in capi.get_ir(code) if w[0] == "GetAttr.Generic")
PAYLOAD = 0xDEAD_BEEF_0000_0000 | (LOW[sys.argv[1]] << 16) | site[4]
capi.plant_arm_words(code, offset, "GetMethod.Generic", (PAYLOAD,))
head = next(i for i, w in enumerate(capi.get_ir(code, True))
            if w[0] == "GetMethod.Generic")


def operand_word():
    exec_words = capi.get_ir_raw(code)[11]
    return struct.unpack_from("<Q", exec_words, 8 * (head + 1))[0]


def run():
    return list(gen(False, 1))


PHASES = ["plain", "compiled", "line", "instruction", "settrace", "after"]
results = {}
checked = []


def check(name, value):
    results[name] = value
    checked.append(operand_word() == PAYLOAD)


check("plain", run())
check("compiled", [capi.jit_compile(code), run()])

lines = []
MON.use_tool_id(3, "arms")


def monitored(event, callback):
    MON.register_callback(3, event, callback)
    MON.set_events(3, event)
    try:
        return run()
    finally:
        MON.set_events(3, 0)


check("line", monitored(MON.events.LINE, lambda c, n: lines.append(n)))
check("instruction", monitored(MON.events.INSTRUCTION, lambda c, n: lines.append(n)))
MON.free_tool_id(3)


def tracer(frame, event, arg):
    return tracer


sys.settrace(tracer)
try:
    check("settrace", run())
finally:
    sys.settrace(None)

check("after", run())
results["unchanged"] = dict(zip(PHASES, checked))
print(json.dumps(results))
"""


def run(mode, force):
    _, out, _ = assert_python_ok("-c", textwrap.dedent(SCRIPT), str(int(force)),
                                 PYTHON_JIT=mode)
    return json.loads(out)


class ArmWordObservationTests(unittest.TestCase):
    def check(self, mode):
        plain = run(mode, False)
        forced = run(mode, True)
        arms_seen = forced.pop("line_arms_seen")
        plain.pop("line_arms_seen")
        self.assertEqual(forced, plain)
        # The words that no event rewrites keep their arm under monitoring
        self.assertGreaterEqual(arms_seen, 1)
        self.assertEqual(plain["plain"], 14)
        self.assertEqual(plain["compiled"][1], 14)
        self.assertEqual(plain["line"][1], [1, 2, 3, 4])
        self.assertEqual(plain["jump_over"][0], 0)

    def test_jit_off(self):
        self.check("off")

    def test_jit_always(self):
        self.check("always")


class ForceArmTests(unittest.TestCase):
    def setUp(self):
        def f(a, b):
            return a + b
        f(1, 2)
        self.code = f.__code__
        self.offset = next(i.offset for i in dis.get_instructions(f)
                           if i.opname == "BINARY_OP")

    def test_names_every_arm(self):
        handlers = capi.arm_handlers()
        self.assertEqual(len(set(handlers.values())), len(handlers))
        self.assertGreaterEqual(min(handlers.values()), _opcode.IR_FIRST_ARM)
        for name in ("Add.Int", "Add.Generic", "GetAttr.Instance"):
            self.assertIn(name, handlers)

    def test_the_contract_export_has_a_row_for_each_arm(self):
        contract = _opcode.get_ir_contract()
        handlers = capi.arm_handlers()
        # An arm of a family with events has an observed twin after the arms
        twins = {contract[h].twin for h in handlers.values()} - {None}
        self.assertEqual(len(contract), max(twins.union(handlers.values())) + 1)
        row = contract[handlers["Add.Int"]]
        self.assertEqual(row.roles[1:3],
                         (_opcode.IR_ROLE_SRC, _opcode.IR_ROLE_SRC + 1))
        self.assertIsNone(row.twin)
        self.assertTrue(contract[handlers["Add.Generic"]].effects & 0x1)

    def test_word_length_follows_the_arms_derived_operand_words(self):
        handlers = capi.arm_handlers()
        contract = _opcode.get_ir_contract()
        for name, handler in handlers.items():
            with self.subTest(arm=name):
                # The head's handler field is the low 9 bits of its word
                length = _opcode.get_ir_word_length(handler)
                # A feedback entry named by an operand word makes the head two words
                two = name.startswith(("GetMethod.", "SuperMethod.", "SuperMethodExplicit.",
                                       "Compare.", "Contains.", "TestBranch.",
                                       "Binary.", "AppendText."))
                self.assertEqual(length, 2 if two else 1)
                self.assertIsNotNone(contract[handler])

    def test_the_old_specializer_leaves_a_forced_arm_to_its_miss_route(self):
        # Adaptation of an arm word goes through the family's adaptation
        # macro, not through specialize.c, whose tables the arm's handler id
        # would index out of bounds.  Warming the function hits the generic
        # word's counter and must leave the forced arm in place.
        capi.force_arm(self.code, self.offset, "Add.Int")
        f = types.FunctionType(self.code, {})
        for _ in range(200):
            self.assertEqual(f(1, 2), 3)
        self.assertIn("Add.Int", [w[0] for w in capi.get_ir(self.code, True)])

    def test_the_disassembler_names_every_arm_word(self):
        for name, handler in capi.arm_handlers().items():
            self.assertEqual(ir_dis._all_opname[handler], name)
            self.assertEqual(ir_dis._all_opmap[name], handler)

    def test_ir_dis_output_names_a_forced_arm_word(self):
        capi.force_arm(self.code, self.offset, "Add.Int")
        handlers = [i.handler for i in
                    ir_dis.get_instructions(self.code, adaptive=True)]
        self.assertIn("Add.Int", handlers)

    def test_dis_reads_the_stack_ops_of_a_function_with_an_arm_word(self):
        before = [(i.opname, i.offset) for i in dis.get_instructions(self.code)]
        capi.force_arm(self.code, self.offset, "Add.Int")
        self.assertEqual([(i.opname, i.offset) for i in dis.get_instructions(self.code)],
                         before)
        self.assertIn("BINARY_OP", dis.Bytecode(self.code).dis())

    def test_forces_the_arm_onto_the_word(self):
        capi.force_arm(self.code, self.offset, "Add.Int")
        names = [w[0] for w in capi.get_ir(self.code, True)]
        self.assertIn("Add.Int", names)
        self.assertNotIn("Add.Int", [w[0] for w in capi.get_ir(self.code)])

    def test_refuses_a_word_of_another_family(self):
        with self.assertRaisesRegex(ValueError, "no word at offset 0 stands for"):
            capi.force_arm(self.code, 0, "Add.Int")

    def test_refuses_an_unknown_arm(self):
        with self.assertRaisesRegex(ValueError, "no register instruction arm Add.Nope"):
            capi.force_arm(self.code, self.offset, "Add.Nope")


class OperandWordTests(unittest.TestCase):
    def run_script(self, mode, low):
        _, out, _ = assert_python_ok("-c", textwrap.dedent(OPERAND_SCRIPT), low,
                                     PYTHON_JIT=mode)
        results = json.loads(out)
        for name in ("plain", "line", "instruction", "settrace", "after"):
            self.assertEqual(results[name], [0, 1], name)
        self.assertEqual(results["compiled"][1], [0, 1])
        return results["unchanged"]

    def check(self, mode):
        for low in ("decoy", "marker"):
            with self.subTest(low=low):
                unchanged = self.run_script(mode, low)
                self.assertEqual(unchanged, dict.fromkeys(unchanged, True))
                self.assertEqual(len(unchanged), 6)

    def test_jit_off(self):
        self.check("off")

    def test_jit_always(self):
        self.check("always")

    def test_planting_checks_the_word_count_and_the_family(self):
        def f(o):
            return o.real
        with self.assertRaisesRegex(ValueError, "has 1 operand words"):
            capi.plant_arm_words(f.__code__, 0, "GetMethod.Generic", ())
        with self.assertRaisesRegex(ValueError, "not a stack op of family"):
            capi.plant_arm_words(f.__code__, 0, "GetMethod.Generic", (0,))


if __name__ == "__main__":
    unittest.main()
