"""The register IR's end condition, checked by tests.

During the migration the IR handler table holds three kinds of member: arms
of the families written in Python/register_ops/*.rop (`Family.Arm`),
inherited register instructions that Python/bytecodes.c still defines, and
stack-only stubs.  The inherited instructions and the stubs only shrink: the
generated classification (Lib/_opcode_metadata.py) is checked against
ir_handler_ceiling.txt, which each port edits, and which is empty at the end.
"""

import re
import unittest
from pathlib import Path

import opcode
import _opcode_metadata as metadata

from test import test_tools

CEILING = Path(__file__).with_name("ir_handler_ceiling.txt")
FIRST_PSEUDO = 256


def read_ceiling() -> dict[str, set[str]]:
    classes: dict[str, set[str]] = {"inherited": set(), "stub": set()}
    for line in CEILING.read_text().splitlines():
        if not line or line.startswith("#"):
            continue
        kind, name = line.split()
        classes[kind].add(name)
    return classes


def arm_names() -> set[str]:
    return {name for name in metadata._ir_opmap if "." in name}


def stack_op_names() -> set[str]:
    return set(opcode.opmap)


def handler_table() -> set[str]:
    """Every name that can run: the opcodes, their specialized forms and
    the IR-only handlers.  The compiler's pseudo-instructions never run."""
    names = set(metadata._specialized_opmap) | set(metadata._ir_opmap)
    names.update(name for name, number in metadata.opmap.items()
                 if number < FIRST_PSEUDO)
    return names


class CeilingTests(unittest.TestCase):
    def test_every_handler_has_one_class(self):
        arms = arm_names()
        inherited = set(metadata._ir_inherited_handlers)
        stubs = set(metadata._ir_stack_stubs)
        self.assertFalse(arms & inherited)
        self.assertFalse(arms & stubs)
        self.assertFalse(inherited & stubs)
        unclassified = handler_table() - arms - inherited - stubs
        self.assertEqual(unclassified, set())
        self.assertEqual((inherited | stubs) - handler_table(), set())

    def test_no_new_inherited_instruction(self):
        ceiling = read_ceiling()
        new = set(metadata._ir_inherited_handlers) - ceiling["inherited"]
        self.assertEqual(new, set(),
                         "new register instructions are written in "
                         "Python/register_ops/*.rop, not in Python/bytecodes.c")

    def test_no_new_stack_only_stub(self):
        new = set(metadata._ir_stack_stubs) - read_ceiling()["stub"]
        self.assertEqual(new, set(),
                         "a stack op's behavior is a family in "
                         "Python/register_ops/*.rop; a stub is not added")

    def test_ceiling_holds_no_stale_name(self):
        ceiling = read_ceiling()
        self.assertEqual(
            ceiling["inherited"] - set(metadata._ir_inherited_handlers),
            set(), "a port deletes its names from ir_handler_ceiling.txt")
        self.assertEqual(
            ceiling["stub"] - set(metadata._ir_stack_stubs), set(),
            "a port deletes its names from ir_handler_ceiling.txt")

    def test_ceiling_is_sorted_and_unique(self):
        lines = [line for line in CEILING.read_text().splitlines()
                 if line and not line.startswith("#")]
        self.assertEqual(lines, sorted(set(lines)))


class ArmNameTests(unittest.TestCase):
    def test_python_view_has_no_stack_op_name(self):
        stack_ops = stack_op_names()
        for dotted in arm_names():
            for part in dotted.split("."):
                with self.subTest(arm=dotted):
                    self.assertNotIn(part, stack_ops)

    def test_arm_tables_have_no_stack_op_name(self):
        test_tools.skip_if_missing("cases_generator")
        header = (Path(test_tools.basepath) / "Include" / "internal"
                  / "pycore_rop_metadata.h")
        names = re.findall(r"\bROP_(?:FAMILY|HANDLER)_(\w+)",
                           header.read_text())
        self.assertTrue(names)
        stack_ops = stack_op_names()
        for name in names:
            with self.subTest(name=name):
                self.assertFalse(stack_ops & set(name.split("_")),
                                 "an arm table names a stack op")

    def test_families_have_no_stack_op_name(self):
        families = {name.split(".")[0] for name in arm_names()}
        self.assertTrue(families)
        self.assertFalse(families & stack_op_names())

    def test_definitions_declare_no_stack_op_name(self):
        # Every name the register definitions declare: enums and their
        # members, families and operands, points, arms, rows and macros and
        # their parameters.  Only the ops(...) clause names stack ops.
        test_tools.skip_if_missing("cases_generator")
        with test_tools.imports_under_tool("cases_generator"):
            import rop_check
        program = rop_check.load(sorted(rop_check.ROP_DIR.glob("*.rop")))
        declared = set()
        for e in program.enums.values():
            declared |= {e.name, *e.members}
        for f in program.families.values():
            declared |= {f.name, *(p.name for p in f.params)}
        declared |= {n.split(".")[1] for n in [*program.points, *program.arms]}
        for x in [*program.externs.values(), *program.macros.values()]:
            declared |= {x.name, *(p.name for p in x.params)}
        self.assertTrue(declared)
        self.assertEqual(declared & stack_op_names(), set())


class BannerTests(unittest.TestCase):
    def test_bytecodes_banner(self):
        test_tools.skip_if_missing("cases_generator")
        source = Path(test_tools.basepath) / "Python" / "bytecodes.c"
        head = source.read_text()[:2000]
        self.assertIn("inherited register instructions", head)
        self.assertIn("Python/register_ops/", head)


if __name__ == "__main__":
    unittest.main()
