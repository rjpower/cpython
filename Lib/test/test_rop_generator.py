"""Tests of the generated products of the register instruction definitions:
Tools/cases_generator/rop_cgen.py and rop_generator.py."""

import re
import unittest
from pathlib import Path

from test import support, test_tools

test_tools.skip_if_missing("cases_generator")
with test_tools.imports_under_tool("cases_generator"):
    import rop_check
    import rop_generator

ROOT = Path(test_tools.basepath)


def products() -> "rop_generator.Products":
    program = rop_check.load(sorted((ROOT / "Python" / "register_ops").glob("*.rop")))
    assert not program.diagnostics
    numbers = rop_generator.read_numbers()
    rop_generator.check_numbers(program, numbers)
    return rop_generator.lower_all(program, numbers)


def int51_boundaries(shift: int) -> list[int]:
    top = 1 << 50
    return sorted({v for base in (-top, top - 1, 0, 1, -1, top // 2, -top // 2)
                   for v in (base - 1, base, base + 1)
                   if -top <= v < top})


class AddCheckedScaledTests(unittest.TestCase):
    """The C definition of the row against Python integers"""

    def test_c_against_python(self):
        testcapi = support.import_helper.import_module("_testinternalcapi")
        shift = testcapi.leaf_layout()[0]["Py_INT_SHIFT"]
        values = int51_boundaries(shift)
        self.assertGreater(len(values), 10)
        for x in values:
            for y in values:
                with self.subTest(x=x, y=y):
                    exact = x + y
                    overflowed, out = testcapi.rop_add_checked_scaled(x << shift, y << shift)
                    fits = -(1 << 50) <= exact < (1 << 50)
                    self.assertEqual(overflowed, not fits)
                    self.assertEqual(out, exact << shift if fits else exact)


class GeneratedAddIntTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.products = products()

    def test_every_arm_has_a_number(self):
        numbers = rop_generator.read_numbers()
        self.assertEqual(set(numbers), set(self.products.program.arms))

    def test_add_int_is_emitted(self):
        self.assertIn("Add.Int", self.products.lowered)

    def test_arms_left_out_say_what_blocks_them(self):
        for arm, why in self.products.left_out.items():
            with self.subTest(arm):
                self.assertIn("pending", why)

    def test_add_int_shape(self):
        """About 15 statements, one type test, no NULL test on the fitting path"""
        low = self.products.lowered["Add.Int"]
        hot = []
        for line in low.body:
            if line.startswith("rop_Add_Int_Overflow:"):
                break
            hot.append(line.strip())
        statements = [line for line in hot
                      if line.endswith(";") and not line.startswith("goto")]
        self.assertLessEqual(len(statements), 17, hot)
        tests = [line for line in hot if "BothInlineInt" in line]
        self.assertEqual(len(tests), 1)
        self.assertFalse(any("IsNull" in line for line in hot))

    def test_value_api_only(self):
        for low in self.products.lowered.values():
            text = "\n".join(low.decls + low.body)
            self.assertIsNone(re.search(r"\.bits\b|_PyValue_BITS", text), text)

    def test_generated_files_are_current(self):
        for name, writer in (("Python/generated_rop_cases.c.h", rop_generator.write_cases),
                             ("Python/generated_rop_forms.c.h", rop_generator.write_forms)):
            import io
            out = io.StringIO()
            sources = sorted(f"Python/register_ops/{p.name}"
                             for p in (ROOT / "Python" / "register_ops").glob("*.rop"))
            writer(out, self.products, sources)
            self.assertEqual((ROOT / name).read_text(), out.getvalue(), name)


if __name__ == "__main__":
    unittest.main()
