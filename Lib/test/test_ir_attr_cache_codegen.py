"""Typed cache bindings and publication for guarded operands of a handler."""

import dataclasses
import io
from pathlib import Path
import tempfile
import unittest
from unittest import mock

from test import test_generated_cases as cases

ir_generator = cases.ir_generator


class TestGuardedAttributeCodegen(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.source = Path(directory.name) / "guarded-attribute.c"

    def generate(self, source, *, runtime=False):
        if not runtime:
            source = (cases.parser.BEGIN_MARKER + "\n" + source + "\n" +
                      cases.parser.END_MARKER)
        self.source.write_text(source)
        with cases.handle_stderr():
            analysis = cases.analyzer.analyze_files(
                [str(self.source)], None if runtime else cases.SequentialNumbers())
        output = io.StringIO()
        ir_generator.generate_ir([str(self.source)], analysis, output, False)
        return output.getvalue()

    def test_guarded_operand_has_metadata_type(self):
        with mock.patch.dict(ir_generator.FEEDBACK_LAYOUTS, {"OP": "_PyFbAttr"}):
            output = self.generate("""
            inst(OP, (class_value/4, owner: consumed -- attr: owned))
                effects(releases) exits(next)
            {
                PyValueBorrow found = _PyValueGuarded_Load(class_value);
                attr = PyValue_XDUP(found);
                DECREF_INPUTS();
            }
            """)
        self.assertIn("_PyValueGuarded class_value = fb->class_value;", output)
        self.assertNotIn("PyObject *class_value", output)
        self.assertNotIn("PyValueBorrow class_value", output)
        acquire = output.index("attr = PyValue_XDUP(found);")
        detach = output.index("IR_DETACH(")
        store = output.index("IR_STORE_DST(")
        close = output.index("PyValue_XCLOSE(ir_own_")
        self.assertLess(acquire, detach)
        self.assertLess(detach, store)
        self.assertLess(store, close)

    def test_guarded_operand_rejects_wrong_width(self):
        for size in (1, 2):
            with self.subTest(size=size):
                with self.assertRaisesRegex(SyntaxError, "must use four units"), \
                     mock.patch.dict(ir_generator.FEEDBACK_LAYOUTS, {"OP": "_PyFbAttr"}):
                    self.generate(f"""
                    inst(OP, (class_value/{size}, owner: consumed -- attr: owned))
                        effects(releases) exits(next)
                    {{
                        attr = PyValue_DUP(owner);
                        PyValue_CLOSE(owner);
                    }}
                    """)

    def test_family_head_has_no_retirement_reload(self):
        source = """
        inst(OP, (class_value/4, owner: consumed -- attr: owned))
            effects(releases) exits(next)
        {
            PyValueBorrow found = _PyValueGuarded_Load(class_value);
            attr = PyValue_XDUP(found);
            DECREF_INPUTS();
        }
        inst(OP_SPECIALIZED, (class_value/4, owner: consumed -- attr: owned))
            effects(releases) exits(next)
        {
            PyValueBorrow found = _PyValueGuarded_Load(class_value);
            attr = PyValue_XDUP(found);
            DECREF_INPUTS();
        }
        family(OP, 4) = { OP_SPECIALIZED };
        """
        with mock.patch.dict(ir_generator.FEEDBACK_LAYOUTS, {"OP": "_PyFbAttr"}):
            output = self.generate(source)
        start = output.index("TARGET(OP) {")
        end = output.find("TARGET(", start + 1)
        handler = output[start:end]
        self.assertIn("_PyIRWord* const this_instr = next_instr - 1;", handler)
        self.assertNotIn("this_instr = frame->instr_ptr;", handler)
        self.assertNotIn("retire_published_values(", handler)

    def test_guarded_operand_rejects_wrong_feedback_type(self):
        with self.assertRaisesRegex(SyntaxError, "names no field of _PyFbUnary"):
            self.generate("""
            inst(TO_BOOL, (class_value/4, owner: consumed -- attr: owned))
                effects(releases) exits(next)
            {
                attr = PyValue_DUP(owner);
                PyValue_CLOSE(owner);
            }
            """)


if __name__ == "__main__":
    unittest.main()
