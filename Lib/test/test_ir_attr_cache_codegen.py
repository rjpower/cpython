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
                     "LOAD_ATTR_NONDESCRIPTOR_NO_DICT", "LOAD_ATTR_CLASS",
                     "LOAD_ATTR_CLASS_WITH_METACLASS_CHECK"):
            with self.subTest(name=name):
                start = output.index(f"TARGET({name}) {{")
                end = output.find("TARGET(", start + 1)
                handler = output[start:end]
                guard = handler.index("tp_version_tag")
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
                if name.startswith("LOAD_ATTR_CLASS"):
                    self.assertIn("class_version = fb->class_version", handler)
                    self.assertNotIn("fb->descr", handler)
                    self.assertIn("if (oparg & 1)", handler)
                if name == "LOAD_ATTR_CLASS_WITH_METACLASS_CHECK":
                    self.assertLess(handler.index("type_version = fb->type_version"),
                                    handler.index("class_version = fb->class_version"))

    def test_direct_class_selectors_and_result_contract(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c")
        with cases.handle_stderr():
            analysis = cases.analyzer.analyze_files([str(source)])
        with cases.test_tools.imports_under_tool("cases_generator"):
            from opcode_metadata_generator import feedback_fields
        formats = ir_generator.compute_formats(analysis)
        contracts = ir_generator.handler_contracts(analysis, formats)
        for name, number, versions in (
            ("LOAD_ATTR_CLASS", 189, [(2, 2, "class_version")]),
            ("LOAD_ATTR_CLASS_WITH_METACLASS_CHECK", 190,
             [(2, 2, "type_version"), (4, 2, "class_version")]),
        ):
            with self.subTest(name=name):
                self.assertEqual(analysis.opmap[name], number)
                self.assertEqual(feedback_fields(analysis.instructions[name]),
                                 versions + [(6, 4, "class_value")])
                self.assertEqual(formats[name].fb_layout, "_PyFbAttr")
                self.assertEqual(contracts[name].result_phase,
                                 ir_generator.RESULT_PHASE_DST_TEMP_CANONICAL_BEFORE_RETIRE)

    def test_classmethod_guards_capture_before_pair_acquisition(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()
        output = self.generate(source, runtime=True)
        start = output.index("TARGET(LOAD_ATTR_CLASSMETHOD) {")
        end = output.find("TARGET(", start + 1)
        handler = output[start:end]
        positions = [handler.index(text) for text in (
            "#ifdef Py_GIL_DISABLED", "Py_TYPE(owner_o) != &PyType_Type",
            "class_version = fb->class_version", "tp_version_tag",
            "_PyValueGuarded_Load(class_value)",
            "Py_IS_TYPE(cm, &PyClassMethod_Type)",
            "_PyClassMethod_GetFunc(cm)",
            "target == NULL || !Py_IS_TYPE(target, &PyFunction_Type)",
            "attr = PyValue_DUP(PyValue_FromObjectBorrow(target))",
            "self =", "STAT_INC(LOAD_ATTR, hit)",
        )]
        self.assertEqual(positions, sorted(positions))
        self.assertIn("_PyValueGuarded class_value = fb->class_value;", handler)
        self.assertIn("const bool gil_disabled = true;", handler)
        self.assertIn("const bool gil_disabled = false;", handler)
        self.assertLess(handler.index("if (gil_disabled)"),
                        handler.index("_PyValueGuarded_Load(class_value)"))
        self.assertNotIn("retire_published_values(", handler)
        self.assertNotIn("_PUSH_NULL_CONDITIONAL", handler)
        self.assertNotIn("PyObject *class_value", handler)

    def test_classmethod_family_extent_and_number(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c")
        with cases.handle_stderr():
            analysis = cases.analyzer.analyze_files([str(source)])
        with cases.test_tools.imports_under_tool("cases_generator"):
            from opcode_metadata_generator import feedback_fields
        formats = ir_generator.compute_formats(analysis)
        contracts = ir_generator.handler_contracts(analysis, formats)
        name = "LOAD_ATTR_CLASSMETHOD"
        self.assertEqual(analysis.opmap[name], 111)
        self.assertEqual(analysis.instructions[name].family.name, "LOAD_ATTR")
        self.assertEqual(feedback_fields(analysis.instructions[name]),
                         [(2, 2, "class_version"), (6, 4, "class_value")])
        self.assertEqual(formats[name].roles, formats["LOAD_ATTR"].roles)
        self.assertEqual(formats[name].fb_layout, "_PyFbAttr")
        self.assertEqual(contracts["LOAD_ATTR"].writes, (1, ir_generator.EXT_BIT0))
        self.assertEqual(contracts[name].writes, (2, ir_generator.EXT_FIXED))

    def test_classmethod_unknown_target_reader_still_escapes(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()
        source = source.replace("_PyClassMethod_GetFunc(cm)", "unknown_getter(cm)")
        with self.assertRaisesRegex(SyntaxError, "view 'owner' is read after a call"):
            self.generate(source, runtime=True)

    def test_classmethod_capture_requires_audited_readers(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()
        for reader in ("_PyClassMethod_GetFunc", "_PyValueGuarded_Load"):
            with self.subTest(reader=reader):
                remaining = tuple(name for name in cases.analyzer.NON_ESCAPING_FUNCTIONS
                                  if name != reader)
                with mock.patch.object(cases.analyzer, "NON_ESCAPING_FUNCTIONS", remaining):
                    with self.assertRaisesRegex(SyntaxError, "view 'owner' is read after a call"):
                        self.generate(source, runtime=True)


if __name__ == "__main__":
    unittest.main()
