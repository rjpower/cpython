"""Checked boolean edge continuations and their logical observer anchors."""

import dis
import io
import opcode
from pathlib import Path
import sys
import types
import unittest
from test import support, test_tools
from unittest import mock
from test.support import import_helper
from test.support.bytecode_helper import encode_words

_testinternalcapi = import_helper.import_module("_testinternalcapi")

VARIANTS = {
    1: "POP_JUMP_IF_FALSE_BOOL_SKIP_TAKEN",
    2: "POP_JUMP_IF_FALSE_BOOL_SKIP_FALLTHROUGH",
    3: "POP_JUMP_IF_FALSE_BOOL_SKIP_BOTH",
}
RELEASE_SPECIALIZES = not support.Py_GIL_DISABLED and not hasattr(sys, "gettotalrefcount")


def handlers(func):
    return [word[0] for word in _testinternalcapi.get_ir(func.__code__, True)]


def warm(func, *args):
    for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 10):
        func(*args)


def edge_function(mask, unsafe_successor=False):
    def template(flag):
        if flag:
            return 11
        return 22

    words = _testinternalcapi.get_ir(template.__code__)
    branch = next(i for i, w in enumerate(words) if w[0] == "POP_JUMP_IF_FALSE")
    target = words[branch][3] | (words[branch][4] << 12)
    fall = list(words[branch + 1:target])
    taken = list(words[target:])
    result = list(words[:branch])
    jump = list(words[branch])
    result.append(jump)
    if mask & 2:
        result.append(("NOP",))
    result.extend(fall)
    destination = len(result)
    if mask & 1:
        result.append(("NOP",))
    if unsafe_successor:
        result.append(("IR_JUMP", 0, 0, destination + 2, 0))
    result.extend(taken)
    jump[3], jump[4] = destination & 4095, destination >> 12
    ids = {**opcode.opmap, **opcode._specialized_opmap, **opcode._ir_opmap}
    numeric = [(ids[word[0]], *word[1:]) for word in result]
    code = template.__code__.replace(co_code=encode_words(*numeric))
    return types.FunctionType(code, globals())


class BooleanBoundaryTests(unittest.TestCase):
    @unittest.skipUnless(RELEASE_SPECIALIZES, "release GIL specialization required")
    def test_each_edge_descriptor_and_guard_decline(self):
        for mask, variant in VARIANTS.items():
            with self.subTest(mask=mask):
                func = edge_function(mask)
                pristine = func.__code__.co_code
                warm(func, False)
                self.assertIn(variant, handlers(func))
                self.assertEqual(func(False), 22)
                self.assertEqual(func(True), 11)
                # A failed bool guard must replay the original branch, with
                # neither the static fallthrough nor taken advance applied.
                self.assertEqual(func([]), 22)
                self.assertEqual(func([1]), 11)
                self.assertEqual(func.__code__.co_code, pristine)

    @unittest.skipUnless(RELEASE_SPECIALIZES, "release GIL specialization required")
    def test_no_save_ip_successor_declines_skip(self):
        func = edge_function(1, unsafe_successor=True)
        warm(func, False)
        self.assertIn("POP_JUMP_IF_FALSE_BOOL", handlers(func))
        self.assertFalse(set(handlers(func)) & set(VARIANTS.values()))
        self.assertEqual(func(False), 22)
        self.assertEqual(func(True), 11)

    def test_monitoring_enabled_in_callee_restores_anchor(self):
        def func(flag, enable):
            enable()
            if flag:
                return 1
            try:
                return 2
            except RuntimeError:
                return 3

        warm(func, False, lambda: None)
        nop = next(i.offset for i in dis.get_instructions(func) if i.opname == "NOP")
        mon = sys.monitoring
        tool = mon.DEBUGGER_ID
        for local in (False, True):
            with self.subTest(local=local):
                seen = []
                mon.use_tool_id(tool, "ir boundary")
                try:
                    mon.register_callback(tool, mon.events.INSTRUCTION,
                                          lambda code, offset: seen.append((code, offset)))
                    def enable():
                        if local:
                            mon.set_local_events(tool, func.__code__, mon.events.INSTRUCTION)
                        else:
                            mon.set_events(tool, mon.events.INSTRUCTION)
                    self.assertEqual(func(False, enable), 2)
                    self.assertIn((func.__code__, nop), seen)
                    self.assertFalse(set(handlers(func)) & set(VARIANTS.values()))
                finally:
                    mon.set_events(tool, 0)
                    mon.set_local_events(tool, func.__code__, 0)
                    mon.free_tool_id(tool)

    def test_suspended_generator_observes_anchor(self):
        def gen(flag):
            yield 0
            if flag:
                return 1
            try:
                yield 2
            except RuntimeError:
                yield 3

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 10):
            it = gen(False)
            self.assertEqual(list(it), [0, 2])
        it = gen(False)
        self.assertEqual(next(it), 0)
        nop = next(i.offset for i in dis.get_instructions(gen) if i.opname == "NOP")
        seen = []
        mon = sys.monitoring
        tool = mon.DEBUGGER_ID
        mon.use_tool_id(tool, "ir suspended boundary")
        try:
            mon.register_callback(tool, mon.events.INSTRUCTION,
                                  lambda code, offset: seen.append((code, offset)))
            mon.set_local_events(tool, gen.__code__, mon.events.INSTRUCTION)
            self.assertEqual(next(it), 2)
            self.assertIn((gen.__code__, nop), seen)
        finally:
            mon.set_local_events(tool, gen.__code__, 0)
            mon.free_tool_id(tool)
            it.close()


@unittest.skipIf(test_tools.basepath is None, "needs the source tree")
class BoundaryGeneratorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        test_tools.skip_if_missing("cases_generator")
        with test_tools.imports_under_tool("cases_generator"):
            import analyzer
            import ir_generator
            import ir_metadata_generator
        cls.generator = ir_generator
        cls.metadata = ir_metadata_generator
        cls.analysis = analyzer.analyze_files([
            str(Path(test_tools.basepath) / "Python" / "bytecodes.c")])

    def test_generated_continuations_and_save_ip_facts(self):
        gen = self.generator
        formats = gen.compute_formats(self.analysis)
        _, _, flags = self.metadata.handler_facts(self.analysis, formats)
        for name in ("END_FOR", "INSTRUMENTED_END_FOR", "IR_JUMP"):
            self.assertNotIn("_PyIR_H_SAVES_IP", flags[name])
        self.assertIn("_PyIR_H_SAVES_IP", flags["LOAD_ATTR_SHAPE"])
        output = io.StringIO()
        gen.generate_ir([], self.analysis, output, False)
        for variant in VARIANTS.values():
            body = output.getvalue().split(f"TARGET({variant}) {{", 1)[1]
            body = body.split("TARGET(", 1)[0]
            self.assertNotIn("IR_EXECUTE_ORDINARY_NOP", body)
            self.assertEqual(body.count("frame->instr_ptr ="), 1)
            self.assertIn("int opcode = POP_JUMP_IF_FALSE_BOOL;", body)

    def test_invalid_boundary_descriptors(self):
        gen = self.generator
        for base, mask in (("POP_JUMP_IF_FALSE_BOOL", 4),
                           ("LOAD_ATTR_SHAPE", 1)):
            with self.subTest(base=base, mask=mask):
                variant = gen.ExecutionVariant(base, boundary_mask=mask)
                with mock.patch.dict(gen.EXECUTION_VARIANTS,
                                     {VARIANTS[1]: variant}):
                    with self.assertRaisesRegex(ValueError, "invalid boundary continuation"):
                        gen.generate_ir([], self.analysis, io.StringIO(), False)


if __name__ == "__main__":
    unittest.main()
