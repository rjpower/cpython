"""The stack op table (Tools/cases_generator/stack_ops.py) against the
definitions that it was frozen from, until Python/bytecodes.c goes"""

import unittest

from test import test_tools

test_tools.skip_if_missing("cases_generator")
with test_tools.imports_under_tool("cases_generator"):
    import analyzer
    import handler_table
    import ir_generator
    import stack_opcode_pins
    import stack_ops
    from generators_common import DEFAULT_INPUT, cflags
    from stack import get_stack_effect


def effect(inst):
    stack = get_stack_effect(inst)
    return ((-stack.base_offset).to_c(),
            (stack.logical_sp - stack.base_offset).to_c())


class StackOpTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.analysis = analyzer.analyze_files([str(DEFAULT_INPUT)])

    def test_the_ops_are_the_definitions(self):
        ops = analyzer.stack_ops_of(self.analysis)
        self.assertEqual([op.name for op in stack_ops.STACK_OPS], list(ops))
        for op in stack_ops.STACK_OPS:
            with self.subTest(op.name):
                inst = ops[op.name]
                self.assertEqual(op.opcode, inst.opcode)
                self.assertEqual(op.size, inst.size)
                self.assertEqual(op.family,
                                 inst.family.name if inst.family else None)
                flags = tuple(f[len("HAS_"):-len("_FLAG")]
                              for f in cflags(inst.properties).split(" | ")
                              if f != "0")
                self.assertEqual(op.flags, flags)
                self.assertEqual(op.oparg, bool(inst.properties.oparg))
                self.assertEqual((op.popped, op.pushed), effect(inst))
                self.assertEqual(op.translator_only,
                                 op.name in self.analysis.translator_only)
                self.assertEqual(op.abort_stub,
                                 ir_generator.is_abort_body(self.analysis, op.name))

    def test_the_pseudos_are_the_definitions(self):
        self.assertEqual([p.name for p in stack_ops.PSEUDOS],
                         list(self.analysis.pseudos))
        for pseudo in stack_ops.PSEUDOS:
            with self.subTest(pseudo.name):
                inst = self.analysis.pseudos[pseudo.name]
                self.assertEqual(pseudo.opcode, inst.opcode)
                self.assertEqual(pseudo.targets, tuple(t.name for t in inst.targets))
                self.assertEqual(pseudo.as_sequence, bool(inst.as_sequence))
                self.assertEqual((pseudo.popped, pseudo.pushed), effect(inst))

    def test_the_handlers_are_the_definitions(self):
        self.assertEqual(handler_table.handler_ids(),
                         ir_generator.handler_ids(self.analysis))
        stack_only = ir_generator.stack_only_names(self.analysis)
        self.assertEqual({op.name for op in stack_ops.STACK_OPS if op.stack_only},
                         {name for name in stack_only
                          if name in analyzer.stack_ops_of(self.analysis)})
        self.assertEqual(stack_ops.OLD_ONLY_HANDLERS,
                         tuple(ir_generator.ir_only_handler_names(self.analysis)))


class PinTests(unittest.TestCase):
    def test_numbers_are_the_pins(self):
        pins = stack_opcode_pins.PINNED_OPCODES
        for op in stack_ops.STACK_OPS:
            if not op.translator_only:
                self.assertEqual(op.opcode, pins[op.name], op.name)
        for pseudo in stack_ops.PSEUDOS:
            self.assertEqual(pseudo.opcode, stack_opcode_pins.PINNED_PSEUDOS[pseudo.name])
        numbers = [op.opcode for op in stack_ops.STACK_OPS] + [p.opcode for p in stack_ops.PSEUDOS]
        self.assertEqual(len(numbers), len(set(numbers)))

    def test_a_stack_op_without_a_handler_has_a_format(self):
        for op in stack_ops.STACK_OPS:
            if op.stack_only:
                self.assertIsNotNone(op.format, op.name)


if __name__ == "__main__":
    unittest.main()
