import unittest

from test import test_tools

test_tools.skip_if_missing("cases_generator")
with test_tools.imports_under_tool("cases_generator"):
    import stack_ops


class TestPinnedOpcodes(unittest.TestCase):
    """Stack opcode numbers are declared in stack_ops, not inferred"""

    PINS = {**stack_ops.PINNED_OPCODES, **stack_ops.PINNED_PSEUDOS}

    def test_opmaps_are_the_pinned_table(self):
        import opcode
        for name, number in {**opcode.opmap, **opcode._specialized_opmap}.items():
            with self.subTest(name=name):
                self.assertEqual(self.PINS.get(name), number)

    def test_stack_ops_are_pinned(self):
        for op in stack_ops.STACK_OPS:
            if not op.translator_only:
                with self.subTest(name=op.name):
                    self.assertEqual(self.PINS[op.name], op.opcode)
        for pseudo in stack_ops.PSEUDOS:
            with self.subTest(name=pseudo.name):
                self.assertEqual(self.PINS[pseudo.name], pseudo.opcode)

    def test_translator_only_ops_are_no_stack_opcode(self):
        stack = {op.opcode for op in stack_ops.STACK_OPS if not op.translator_only}
        for op in stack_ops.STACK_OPS:
            if op.translator_only:
                with self.subTest(name=op.name):
                    self.assertNotIn(op.name, self.PINS)
                    self.assertNotIn(op.opcode, stack)


if __name__ == "__main__":
    unittest.main()
