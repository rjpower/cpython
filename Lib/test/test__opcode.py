import dis
from test.support.import_helper import import_module
import unittest
import opcode

_opcode = import_module("_opcode")
from _opcode import stack_effect


class OpListTests(unittest.TestCase):
    def check_bool_function_result(self, func, ops, expected):
        for op in ops:
            if isinstance(op, str):
                op = dis.opmap[op]
            with self.subTest(opcode=op, func=func):
                self.assertIsInstance(func(op), bool)
                self.assertEqual(func(op), expected)

    def test_invalid_opcodes(self):
        invalid = [-100, -1, 512, 513, 1000]
        self.check_bool_function_result(_opcode.is_valid, invalid, False)
        self.check_bool_function_result(_opcode.has_arg, invalid, False)
        self.check_bool_function_result(_opcode.has_const, invalid, False)
        self.check_bool_function_result(_opcode.has_name, invalid, False)
        self.check_bool_function_result(_opcode.has_jump, invalid, False)
        self.check_bool_function_result(_opcode.has_free, invalid, False)
        self.check_bool_function_result(_opcode.has_local, invalid, False)
        self.check_bool_function_result(_opcode.has_exc, invalid, False)

    def test_is_valid(self):
        names = [
            'CACHE',
            'POP_TOP',
            'IMPORT_NAME',
            'JUMP',
            'INSTRUMENTED_RETURN_VALUE',
        ]
        opcodes = [dis.opmap[opname] for opname in names]
        self.check_bool_function_result(_opcode.is_valid, opcodes, True)

    def test_opmaps(self):
        def check_roundtrip(name, map):
            return self.assertEqual(opcode.opname[map[name]], name)

        check_roundtrip('BINARY_OP', opcode.opmap)
        for name in opcode._specialized_opmap:
            check_roundtrip(name, opcode._specialized_opmap)

    def test_oplists(self):
        def check_function(self, func, expected):
            for op in [-10, 520]:
                with self.subTest(opcode=op, func=func):
                    res = func(op)
                    self.assertIsInstance(res, bool)
                    self.assertEqual(res, op in expected)

        check_function(self, _opcode.has_arg, dis.hasarg)
        check_function(self, _opcode.has_const, dis.hasconst)
        check_function(self, _opcode.has_name, dis.hasname)
        check_function(self, _opcode.has_jump, dis.hasjump)
        check_function(self, _opcode.has_free, dis.hasfree)
        check_function(self, _opcode.has_local, dis.haslocal)
        check_function(self, _opcode.has_exc, dis.hasexc)


class StackEffectTests(unittest.TestCase):
    def test_stack_effect(self):
        self.assertEqual(stack_effect(dis.opmap['POP_TOP']), -1)
        self.assertEqual(stack_effect(dis.opmap['BUILD_SLICE'], 2), -1)
        self.assertEqual(stack_effect(dis.opmap['BUILD_SLICE'], 3), -2)
        self.assertRaises(ValueError, stack_effect, 30000)
        # All defined opcodes
        has_arg = dis.hasarg
        for name, code in filter(lambda item: item[0] not in dis.deoptmap, dis.opmap.items()):
            if code >= opcode.MIN_INSTRUMENTED_OPCODE:
                continue
            with self.subTest(opname=name):
                stack_effect(code)
                stack_effect(code, 0)
        # All not defined opcodes
        for code in set(range(256)) - set(dis.opmap.values()):
            with self.subTest(opcode=code):
                self.assertRaises(ValueError, stack_effect, code)
                self.assertRaises(ValueError, stack_effect, code, 0)

    def test_stack_effect_jump(self):
        FOR_ITER = dis.opmap['FOR_ITER']
        self.assertEqual(stack_effect(FOR_ITER, 0), 1)
        self.assertEqual(stack_effect(FOR_ITER, 0, jump=True), 1)
        self.assertEqual(stack_effect(FOR_ITER, 0, jump=False), 1)
        JUMP_FORWARD = dis.opmap['JUMP_FORWARD']
        self.assertEqual(stack_effect(JUMP_FORWARD, 0), 0)
        self.assertEqual(stack_effect(JUMP_FORWARD, 0, jump=True), 0)
        self.assertEqual(stack_effect(JUMP_FORWARD, 0, jump=False), 0)
        # All defined opcodes
        has_arg = dis.hasarg
        has_exc = dis.hasexc
        has_jump = dis.hasjabs + dis.hasjrel
        for name, code in filter(lambda item: item[0] not in dis.deoptmap, dis.opmap.items()):
            if code >= opcode.MIN_INSTRUMENTED_OPCODE:
                continue
            with self.subTest(opname=name):
                if code not in has_arg:
                    common = stack_effect(code)
                    jump = stack_effect(code, jump=True)
                    nojump = stack_effect(code, jump=False)
                else:
                    common = stack_effect(code, 0)
                    jump = stack_effect(code, 0, jump=True)
                    nojump = stack_effect(code, 0, jump=False)
                if code in has_jump or code in has_exc:
                    self.assertEqual(common, max(jump, nojump))
                else:
                    self.assertEqual(jump, common)
                    self.assertEqual(nojump, common)


class SpecializationStatsTests(unittest.TestCase):
    def test_stack_bytecode_has_no_specialized_forms(self):
        # Specialized forms are arms of the register families; stack
        # bytecode is transport only.
        self.assertEqual(dict(opcode._specializations), {})

    def test_specialization_stats(self):
        stat_names = ["success", "failure", "hit", "deferred", "miss", "deopt"]
        stats = _opcode.get_specialization_stats()
        if stats is None:
            self.skipTest("needs a build with Py_STATS")
        self.assertIsInstance(stats, dict)
        self.assertTrue(stats)
        # Keyed by the stack op whose family specializes
        for name, family in stats.items():
            self.assertIn(name.upper(), opcode.opmap)
            self.assertCountEqual(family.keys(), stat_names + ['failure_kinds'])
            for sn in stat_names:
                self.assertIsInstance(family[sn], int)
            self.assertIsInstance(family['failure_kinds'], tuple)
            for v in family['failure_kinds']:
                self.assertIsInstance(v, int)


class IRMetadataTests(unittest.TestCase):
    """The declared facts that the disassembler of the register IR reads"""

    def test_handler_names(self):
        names = _opcode.get_ir_handler_names()
        self.assertEqual(len(names), _opcode.IR_NUM_HANDLERS)
        self.assertEqual(len(set(names)), len(names))
        self.assertEqual(names[_opcode.IR_OPERANDS], "IR_OPERANDS")
        for name in names[_opcode.IR_FIRST_ARM:]:
            self.assertIn(".", name)

    def test_contract_operands(self):
        names = _opcode.get_ir_handler_names()
        contract = _opcode.get_ir_contract()
        shapes = {name: contract[names.index(name)]
                  for name in ("Add.Int", "Call.Generic", "CallKw.Generic",
                               "CallEx.Generic", "Build.List", "Move.Copy")}
        self.assertEqual(shapes["Add.Int"].operands, _opcode.IR_OPERANDS_NONE)
        self.assertEqual(shapes["Call.Generic"].operands, _opcode.IR_OPERANDS_CALL)
        self.assertEqual(shapes["CallKw.Generic"].operands, _opcode.IR_OPERANDS_KW)
        self.assertEqual(shapes["CallEx.Generic"].operands, _opcode.IR_OPERANDS_EX)
        for name in ("Build.List", "Move.Copy"):
            self.assertEqual(shapes[name].operands, _opcode.IR_OPERANDS_SPANS)
            self.assertIn(_opcode.IR_ROLE_COUNT, shapes[name].roles)
        self.assertIn(_opcode.IR_ROLE_ELEMENT, shapes["Build.List"].roles)

    def test_arm_operands(self):
        testinternalcapi = import_module("_testinternalcapi")
        operands = testinternalcapi.arm_operands()
        names = {name for name, *_ in operands["GetAttr"]}
        self.assertEqual(names, {"dst", "obj", "name", "fb"})
        # (name, kind, role, optional, bits, offset): the head's fields sit
        # at bit 16 and every 12 bits after it
        self.assertEqual(operands["GetAttr"][0], ("dst", "home", "dst", 0, 12, 16))
        self.assertEqual(operands["Jump"], (("target", "target", "none", 0, 24, 16),))
        kinds = {kind for family in operands.values()
                 for _, kind, *_ in family}
        self.assertEqual(kinds, {"home", "imm", "name", "feedback", "target",
                                 "vector", "flag"})

    def test_feedback_names(self):
        testinternalcapi = import_module("_testinternalcapi")
        kinds, states = testinternalcapi.get_feedback_names()
        self.assertEqual(kinds[0], "COUNTER")
        self.assertEqual(states[:2], ("COLD", "MONO"))


if __name__ == "__main__":
    unittest.main()
