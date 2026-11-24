"""Tests of the generated products of the register instruction definitions:
Tools/cases_generator/rop_cgen.py and rop_generator.py."""

import re
import unittest
from pathlib import Path

from test import support, test_tools

test_tools.skip_if_missing("cases_generator")
with test_tools.imports_under_tool("cases_generator"):
    import ir_leaf
    import ir_leaf_eval
    import rop_check
    import rop_generator
    import rop_leaf
    import rop_syntax

ROOT = Path(test_tools.basepath)


def products() -> "rop_generator.Products":
    program = rop_check.load(sorted((ROOT / "Python" / "register_ops").glob("*.rop")))
    assert not program.diagnostics
    numbers = rop_generator.number_arms(program)
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


class AddIntLeafTests(unittest.TestCase):
    """The leaf graph of Add.Int, run by the reference evaluator, against
    Python integers: the lowering of add_checked_scaled and the boxing"""

    @classmethod
    def setUpClass(cls):
        capi = support.import_helper.import_module("_testinternalcapi")
        cls.cx = ir_leaf_eval.Context(
            ir_leaf_eval.Layout.from_c(capi), ir_leaf_eval.RealMemory())
        prog = products().program
        cls.leaf = rop_leaf.leaf_of_arm(prog, prog.arms["Add.Int"])

    def word(self, i):
        return ir_leaf.PRIMITIVES["PyValue_FromInlineInt"].spec(self.cx, i)

    def run_add(self, x, y):
        return ir_leaf_eval.evaluate(self.leaf.body, self.cx, {"x": x, "y": y}, {})

    def test_sums_at_the_int51_boundaries(self):
        top = 1 << 50
        lo, hi = self.cx.k("Py_INT_MIN"), self.cx.k("Py_INT_MAX")
        values = int51_boundaries(self.cx.k("Py_INT_SHIFT"))
        for x in values:
            for y in values:
                with self.subTest(x=x, y=y):
                    result = self.run_add(self.word(x), self.word(y))
                    self.assertEqual(result.outcome, "NEXT")
                    z = result.outputs["dst"]
                    if lo <= x + y <= hi:
                        self.assertEqual(z, self.word(x + y))
                    else:
                        self.assertIsInstance(z, ir_leaf_eval.Box)
                        self.assertEqual((z.type, z.value), ("int", x + y))

    def test_a_word_that_is_no_inline_int_misses(self):
        one = self.word(1)
        for other in (0, self.cx.addr("_Py_NoneStruct")):
            for args in ((one, other), (other, one)):
                with self.subTest(args=args):
                    self.assertEqual(self.run_add(*args).outcome, "EXIT")


def hot_sequence(lines):
    """The calls of a template's hot path, by name: up to the cold section,
    without labels, register choices or arguments"""
    names = []
    for line in lines:
        line = line.strip()
        if line.startswith("_PyJitLower_ColdBegin"):
            break
        if m := re.match(r"(?:int c\d+ = )?(_PyJit\w+)\(", line):
            if "NewLabel" not in m[1] and "Bind" not in m[1]:
                names.append(m[1])
        elif line.startswith("if ("):
            names.append(line.rstrip(" {"))
    return names


def old_template(name):
    text = (ROOT / "Python" / "jit_templates.c.h").read_text()
    start = text.index(f"jit_template_{name}(_PyJitWordCx *cx)")
    return text[start:text.index("\n}\n", start)].splitlines()


class AddIntTemplateTests(unittest.TestCase):
    def test_hot_path_matches_the_old_handler(self):
        prog = products().program
        arm = prog.arms["Add.Int"]
        new = rop_leaf.template_of_arm(prog, arm, "Add_Int")
        self.assertEqual(hot_sequence(new), hot_sequence(old_template("BINARY_OP_ADD_INT")))


SHARED = """\
family F(dst:Dst, x:Src) exits(next, miss);
point F.Done(current, next_head, clear) keep() spent(x) defs(dst) held();
extern primitive is_int(v:B) -> Bool
  effects()
  test(inline_int(v))
  c PyValue_IsInlineInt as bool
  evidence(review: "r", contract: "c", lowering: "l", test: "t");
arm F.Copy shared effects(writes, releases) feedback() exits(next, miss)
{
  entry():
    let v:B = use(x);
    guard is_int(v) else miss;
    let z:O = dup(v);
    publish [dst = z] retire [spent(x), old(dst)] at F.Done;
    exit next();
}
"""


class StubTests(unittest.TestCase):
    def test_a_shared_arm_gets_a_stub_with_status_returns(self):
        import io
        program = rop_check.check([rop_syntax.parse(SHARED, "t.rop")])
        self.assertEqual(program.diagnostics, ())
        p = rop_generator.lower_all(program, rop_generator.number_arms(program))
        out = io.StringIO()
        rop_generator.write_stubs(out, p, [])
        text = out.getvalue()
        self.assertEqual(p.left_out, {})
        self.assertIn("_PyIR_Rop_Stub_F_Copy", text)
        self.assertIn("{_PyIR_PLAN_SHARED, 0,", text)
        self.assertIn("return _PyIR_MISS", text)


class GeneratedAddIntTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.products = products()

    def test_metadata_lists_every_arm_in_definition_order(self):
        import io
        out = io.StringIO()
        rop_generator.write_metadata(out, self.products, [])
        text = out.getvalue()
        for i, name in enumerate(self.products.program.arms):
            handler = rop_generator.handler(name)
            self.assertIn(f"#define {handler} (ROP_FIRST_HANDLER + {i})", text)

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
