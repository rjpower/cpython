"""Typed cache bindings and publication for guarded class-value loads."""

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
        output = self.generate("""
        inst(LOAD_ATTR, (class_value/4, owner: consumed -- attr: owned))
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
                with self.assertRaisesRegex(SyntaxError, "must use four units"):
                    self.generate(f"""
                    inst(LOAD_ATTR, (class_value/{size}, owner: consumed -- attr: owned))
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

    def test_real_handler_acquires_stores_then_closes(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()
        output = self.generate(source, runtime=True)
        for name in ("GetAttr.NonDescriptorValues",
                     "GetAttr.NonDescriptorNoDict", "GetAttr.Class",
                     "GetAttr.Class"):
            with self.subTest(name=name):
                start = output.index(f"TARGET({name}) {{")
                end = output.find("TARGET(", start + 1)
                handler = output[start:end]
                guard = handler.index("tp_version_tag")
                load = handler.index("_PyValueGuarded_Load(class_value)")
                acquire = handler.index("attr = PyValue_DUP(found)")
                detach = handler.index("IR_DETACH(ir_word, B, owner)")
                store = handler.index("IR_STORE_DST(ir_word, A, attr)")
                close = handler.index("_PyValue_XCLOSE_POINTER(ir_own_B)")
                self.assertLess(guard, load)
                self.assertLess(load, acquire)
                self.assertLess(acquire, detach)
                self.assertLess(detach, store)
                self.assertLess(store, close)
                self.assertNotIn("retire_published_values(", handler)
                self.assertNotIn("PyObject *class_value", handler)
                if name.startswith("GetAttr.Class"):
                    self.assertIn("class_version = fb->class_version", handler)
                    self.assertNotIn("fb->descr", handler)
                if name == "GetAttr.Class":
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
            ("GetAttr.Class", 189, [(2, 2, "class_version")]),
            ("GetAttr.Class", 190,
             [(2, 2, "type_version"), (4, 2, "class_version")]),
        ):
            with self.subTest(name=name):
                self.assertEqual(analysis.opmap[name], number)
                self.assertEqual(feedback_fields(analysis.instructions[name]),
                                 versions + [(6, 4, "class_value")])
                self.assertEqual(formats[name].fb_layout, "_PyFbAttr")
                self.assertEqual(contracts[name].result_phase,
                                 ir_generator.RESULT_PHASE_NONE)

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
            "_PyValueGuarded_LoadObject(class_value)",
            "assert(PyValue_IsObject(descriptor));",
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
                        handler.index("_PyValueGuarded_LoadObject(class_value)"))
        self.assertNotIn("retire_published_values(", handler)
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
        name = "GetMethod.ClassMethod"
        # An execution-only handler: it has no number in the stack opmap
        self.assertNotIn(name, analysis.opmap)
        instruction = analysis.ir_only[name]
        self.assertEqual(feedback_fields(instruction),
                         [(2, 2, "class_version"), (6, 4, "class_value")])
        self.assertEqual(formats[name].roles, formats["LOAD_ATTR"].roles)
        self.assertEqual(formats[name].fb_layout, "_PyFbAttr")
        self.assertEqual(contracts["LOAD_ATTR"].writes, (1, ir_generator.EXT_BIT0))
        self.assertEqual(contracts[name].writes, (2, ir_generator.EXT_FIXED))

    def test_classmethod_unknown_target_reader_still_escapes(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()
        source = source.replace("_PyClassMethod_GetFunc(cm)", "unknown_getter(cm)")
        # The verifier refuses a callee that has no row, before the generator
        # asks whether the call can run code.  So the stand-in has a row that
        # declares runs_python, and the uop declares it too.  A miss may not
        # follow such a call (the word could not restart), so the stand-in
        # body drops its check of the result; the generator then refuses the
        # read of the view `owner` (the TAKE) after the call
        source = source.replace(
            'EXIT_IF(target == NULL || !Py_IS_TYPE(target, &PyFunction_Type),\n'
            '                    "the classmethod wraps no Python function");',
            "(void)target;")
        declared = ("(class_value/4, owner: consumed -- attr: owned, self: moved(owner)))\n"
                    "            effects(reads) exits(next, miss)\n"
                    "        {\n            assert(oparg & 1);")
        self.assertEqual(source.count(declared), 1)
        source = source.replace(declared, declared.replace(
            "owner: consumed", "owner: taken").replace(
            "effects(reads) exits(next, miss)",
            "effects(reads, runs_python) exits(next, miss)"))
        rows = cases.ir_contract.parse_rows("""
        #define UNKNOWN_ROWS(PRIMITIVE, HELPER, MACRO) \\
            HELPER(unknown_getter, PyObject *, (PyObject *), \\
                   ARGS(object), SCALAR, EFFECTS(runs_python))
        """)
        with mock.patch.dict(cases.ir_contract.rows(), rows):
            with self.assertRaisesRegex(SyntaxError, "view 'owner' is read after a call"):
                self.generate(source, runtime=True)

    def test_classmethod_capture_requires_audited_readers(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()
        for reader in ("_PyClassMethod_GetFunc", "_PyValueGuarded_Load"):
            with self.subTest(reader=reader):
                # A reader whose row declares a reentry effect escapes
                row = cases.ir_contract.rows()[reader]
                reentrant = dataclasses.replace(row, effects=frozenset({"runs_python"}))
                with mock.patch.dict(cases.ir_contract.rows(), {reader: reentrant}):
                    # The verifier refuses the uop's declared effects, or the
                    # generator the view that the call may leave stale
                    with self.assertRaisesRegex(
                            SyntaxError, "may runs_python|view 'owner' is read after a call"):
                        self.generate(source, runtime=True)


if __name__ == "__main__":
    unittest.main()
