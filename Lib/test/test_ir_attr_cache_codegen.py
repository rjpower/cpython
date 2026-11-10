"""Typed cache bindings and publication for guarded class-value loads."""

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
        output = self.generate("""
        inst(LOAD_ATTR, (class_value/4, owner -- attr)) {
            PyValueBorrow found = _PyValueGuarded_Load(class_value);
            attr = IR_CANONICAL_HOME_DUP(found);
            PyValue receiver = OWNER(owner);
            IR_RETIRE_PUBLISHED_RESULT(attr, receiver);
        }
        """)
        self.assertIn("_PyValueGuarded class_value = fb->class_value;", output)
        self.assertNotIn("PyObject *class_value", output)
        self.assertNotIn("PyValueBorrow class_value", output)
        acquire = output.index("attr = IR_CANONICAL_HOME_DUP(found);")
        publish = output.index("IR_STORE_CANONICAL_TEMP_DST(")
        retire = output.index("retire_published_values(")
        self.assertLess(acquire, publish)
        self.assertLess(publish, retire)

    def test_guarded_operand_rejects_wrong_width(self):
        for size in (1, 2):
            with self.subTest(size=size):
                with self.assertRaisesRegex(SyntaxError, "must use four units"):
                    self.generate(f"""
                    inst(LOAD_ATTR, (class_value/{size}, owner -- attr)) {{
                        attr = PyValue_DUP(owner);
                        PyValue_CLOSE(owner);
                    }}
                    """)

    def test_family_head_result_retirement_has_reloadable_instruction(self):
        source = """
        inst(OP, (class_value/4, owner -- attr)) {
            PyValueBorrow found = _PyValueGuarded_Load(class_value);
            attr = IR_CANONICAL_HOME_DUP(found);
            PyValue receiver = OWNER(owner);
            IR_RETIRE_PUBLISHED_RESULT(attr, receiver);
        }
        inst(OP_SPECIALIZED, (class_value/4, owner -- attr)) {
            PyValueBorrow found = _PyValueGuarded_Load(class_value);
            attr = IR_CANONICAL_HOME_DUP(found);
            PyValue receiver = OWNER(owner);
            IR_RETIRE_PUBLISHED_RESULT(attr, receiver);
        }
        family(OP, 4) = { OP_SPECIALIZED };
        """
        with mock.patch.dict(ir_generator.FEEDBACK_LAYOUTS, {"OP": "_PyFbAttr"}):
            output = self.generate(source)
        start = output.index("TARGET(OP) {")
        end = output.find("TARGET(", start + 1)
        handler = output[start:end]
        self.assertIn("_PyIRWord* this_instr = next_instr - 1;", handler)
        self.assertNotIn("_PyIRWord* const this_instr", handler)
        retire = handler.index("retire_published_values(")
        self.assertIn("this_instr = frame->instr_ptr;", handler[retire:])

    def test_guarded_operand_rejects_wrong_feedback_type(self):
        with self.assertRaisesRegex(SyntaxError, "requires attribute feedback"):
            self.generate("""
            inst(TO_BOOL, (class_value/4, owner -- attr)) {
                attr = PyValue_DUP(owner);
                PyValue_CLOSE(owner);
            }
            """)

    def test_real_handler_acquires_and_publishes_before_retirement(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()
        output = self.generate(source, runtime=True)
        for name in ("LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES",
                     "LOAD_ATTR_NONDESCRIPTOR_NO_DICT"):
            with self.subTest(name=name):
                start = output.index(f"TARGET({name}) {{")
                end = output.find("TARGET(", start + 1)
                handler = output[start:end]
                guard = handler.index("tp->tp_version_tag")
                load = handler.index("_PyValueGuarded_Load(class_value)")
                acquire = handler.index("attr = IR_CANONICAL_HOME_DUP(found)")
                publish = handler.index("IR_STORE_CANONICAL_TEMP_DST(")
                retire = handler.index("retire_published_values(")
                self.assertLess(guard, load)
                self.assertLess(load, acquire)
                self.assertLess(acquire, publish)
                self.assertLess(publish, retire)
                self.assertIn("IR_STORE_DST(IR_A(ir_word), attr)", handler[retire:])
                self.assertIn("if (retire_failed < 0)", handler[retire:])
                self.assertIn("_PyIRWord* this_instr = next_instr;", handler)
                self.assertNotIn("_PyIRWord* const this_instr", handler)
                self.assertIn("this_instr = frame->instr_ptr;", handler[retire:])
                self.assertNotIn("PyObject *class_value", handler)


if __name__ == "__main__":
    unittest.main()
