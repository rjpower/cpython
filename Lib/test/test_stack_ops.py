"""The stack op table (Tools/cases_generator/stack_ops.py): the ops that
co_code, .pyc files and the translator hold"""

import unittest

from test import test_tools

test_tools.skip_if_missing("cases_generator")
with test_tools.imports_under_tool("cases_generator"):
    import handler_table
    import stack_ops


class StackOpTests(unittest.TestCase):
    def test_names_and_numbers_are_unique(self):
        names = [op.name for op in stack_ops.STACK_OPS]
        self.assertEqual(len(names), len(set(names)))
        numbers = [op.opcode for op in stack_ops.STACK_OPS]
        self.assertEqual(len(numbers), len(set(numbers)))
        self.assertTrue(set(names).isdisjoint(p.name for p in stack_ops.PSEUDOS))

    def test_every_op_has_the_translators_row(self):
        for op in stack_ops.STACK_OPS:
            with self.subTest(op.name):
                self.assertIsNotNone(op.format)
                self.assertLess(op.opcode, 256)

    def test_pseudos_target_known_ops(self):
        known = {op.name for op in stack_ops.STACK_OPS} | {
            p.name for p in stack_ops.PSEUDOS}
        for pseudo in stack_ops.PSEUDOS:
            with self.subTest(pseudo.name):
                self.assertTrue(set(pseudo.targets) <= known)

    def test_the_only_word_kind_is_operands(self):
        self.assertEqual(handler_table.WORD_KINDS, ("IR_OPERANDS",))
        self.assertEqual(handler_table.handler_ids(),
                         {"IR_OPERANDS": handler_table.id_base()})
        self.assertEqual(handler_table.first_arm(), handler_table.id_base() + 1)


class PinTests(unittest.TestCase):
    def test_numbers_are_the_pins(self):
        for op in stack_ops.STACK_OPS:
            if not op.translator_only:
                self.assertEqual(op.opcode, stack_ops.PINNED_OPCODES[op.name], op.name)
        for pseudo in stack_ops.PSEUDOS:
            self.assertEqual(pseudo.opcode, stack_ops.PINNED_PSEUDOS[pseudo.name])


if __name__ == "__main__":
    unittest.main()
