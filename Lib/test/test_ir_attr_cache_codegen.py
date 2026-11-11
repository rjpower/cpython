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

    def test_canonical_pair_declaration(self):
        output = self.generate("""
        inst(LOAD_ATTR, (class_value/4, owner -- first, second)) {
            first = PyValue_DUP(_PyValueGuarded_Load(class_value));
            second = TAKE(owner);
            IR_CANONICAL_OUTPUTS(first, second);
        }
        """)
        self.assertNotIn("IR_CANONICAL_OUTPUTS(", output)
        self.assertIn("IR_STORE_CANONICAL_TEMP(", output)
        self.assertNotIn("IR_STORE_TEMP(", output)
        self.assertIn("IR_TAKE(owner, ir_own_B)", output)
        self.assertNotIn("_PyValue_Publish", output)

    def test_canonical_pair_false_declarations(self):
        source = """
        inst(LOAD_ATTR, (class_value/4, owner -- attr, self)) {
            attr = PyValue_DUP(_PyValueGuarded_Load(class_value));
            self = TAKE(owner);
            IR_CANONICAL_OUTPUTS(attr, self);
        }
        """
        changes = (
            ("IR_CANONICAL_OUTPUTS(attr, self)", "IR_CANONICAL_OUTPUTS(self, attr)"),
            ("IR_CANONICAL_OUTPUTS(attr, self)", "IR_CANONICAL_OUTPUTS(attr)"),
            ("attr = PyValue_DUP(_PyValueGuarded_Load(class_value));",
             "attr = PyValue_DUP(owner);"),
            ("self = TAKE(owner);", "self = owner;"),
            ("self = TAKE(owner);", "self = unknown(owner);"),
            ("self = TAKE(owner);", "self = TAKE(owner); self = unknown();"),
            ("self = TAKE(owner);", "self = TAKE(owner); mutate(&attr);"),
            ("self = TAKE(owner);", "self = TAKE(owner); assert(++attr.bits);"),
            ("self = TAKE(owner);", "self = TAKE(owner); assert(mutate(&self));"),
            ("attr = PyValue_DUP", "PyValue (*PyValue_DUP)(PyValueBorrow) = replacement; attr = PyValue_DUP"),
            ("attr = PyValue_DUP", "#define PyValue_DUP replacement\nattr = PyValue_DUP"),
            ("owner -- attr, self", "owner -- attr, owner"),
            ("IR_CANONICAL_OUTPUTS(attr, self);",
             "IR_CANONICAL_OUTPUTS(attr, self); self = unknown();"),
        )
        for old, new in changes:
            with self.subTest(new=new):
                with self.assertRaises(SyntaxError):
                    self.generate(source.replace(old, new, 1))

    def test_real_method_outputs_are_canonical_owners(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()
        output = self.generate(source, runtime=True)
        for name in ("LOAD_ATTR_METHOD_WITH_VALUES", "LOAD_ATTR_METHOD_NO_DICT",
                     "LOAD_ATTR_METHOD_LAZY_DICT"):
            with self.subTest(name=name):
                start = output.index(f"TARGET({name}) {{")
                end = output.find("TARGET(", start + 1)
                handler = output[start:end]
                self.assertIn("_PyValueGuarded class_value = fb->class_value;", handler)
                self.assertIn("attr = PyValue_DUP(_PyValueGuarded_Load(class_value));", handler)
                self.assertNotIn("PyValue_FromObjectBorrow", handler)
                self.assertNotIn("IR_STORE_TEMP(", handler)
                self.assertIn("IR_STORE_CANONICAL_TEMP(", handler)
                self.assertLess(handler.index("tp->tp_version_tag"),
                                handler.index("_PyValueGuarded_Load(class_value)"))

    def test_canonical_pair_prefix_must_preserve_inputs(self):
        source = """
        op(_GUARD, (owner -- owner)) {
            assert(PyValue_IsObject(owner));
            PyTypeObject *tp = Py_TYPE(PyValue_AsKnownObjectBorrow(owner));
            assert(tp != NULL);
        }
        op(_CAPTURE, (class_value/4, owner -- attr, self)) {
            attr = PyValue_DUP(_PyValueGuarded_Load(class_value));
            self = TAKE(owner);
            IR_CANONICAL_OUTPUTS(attr, self);
        }
        macro(LOAD_ATTR) = _GUARD + _CAPTURE;
        """
        self.assertIn("IR_STORE_CANONICAL_TEMP(", self.generate(source))
        for mutation in ("owner = unknown();", "tp->tp_version_tag += 1;",
                         "tp->tp_version_tag = 1;", "owner.bits ^= 1;",
                         "assert(mutate(&owner));",
                         "PyTypeObject *other = tp = NULL;",
                         "PyObject *other = (*callback)(owner);",
                         "PyObject *other = (callback)(owner);",
                         "PyObject *other = callback[0](owner);"):
            with self.subTest(mutation=mutation):
                with self.assertRaises(SyntaxError):
                    self.generate(source.replace("assert(tp != NULL);", mutation))

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



class TestAdjacentMethodCallCodegen(unittest.TestCase):
    setUp = TestGuardedAttributeCodegen.setUp
    generate = TestGuardedAttributeCodegen.generate

    def runtime_source(self):
        return (Path(__file__).resolve().parents[2] /
                'Python' / 'bytecodes.c').read_text()

    def test_terminal_owner_transfer_and_shared_entry(self):
        output = self.generate(self.runtime_source(), runtime=True)
        start = output.index('TARGET(LOAD_ATTR_METHOD_WITH_VALUES_CALL0)')
        end = output.index('\n        TARGET(', start + 1)
        handler = output[start:end]
        positions = [(handler.rindex(text) if text == 'frame->instr_ptr = next_instr'
                      else handler.index(text)) for text in (
            'ir_method_call0_preflight', 'tp->tp_version_tag',
            'FT_ATOMIC_LOAD_UINT8',
            'attr = PyValue_DUP', 'self = IR_TAKE',
            'frame->instr_ptr = next_instr', 'call_func->func_version',
            '_PyFrame_PushExact', 'IR_SAVE_CALL_CONTINUATION',
            'frame = tstate->current_frame = call_child')]
        self.assertEqual(positions, sorted(positions))
        self.assertIn('INSTRUCTION_STATS(LOAD_ATTR_METHOD_WITH_VALUES_CALL0)', handler)
        self.assertEqual(handler.count('IR_STORE_CANONICAL_TEMP(call_window[0], attr)'), 1)
        self.assertNotIn('IR_STORE_TEMP(', handler)
        self.assertNotIn('IR_STORE_CANONICAL_TEMP(IR_SLOT(', handler)
        self.assertNotIn('finish_displaced_local', handler)
        self.assertIn('DISPATCH();', handler[handler.index('call_code->co_argcount'):])
        start = output.index('TARGET(CALL_PY_EXACT_ARGS)')
        end = output.index('\n        TARGET(', start + 1)
        ordinary = output[start:end]
        self.assertIn('_PyFrame_PushExact', ordinary)
        self.assertIn('IR_SAVE_CALL_CONTINUATION', ordinary)
        self.assertNotIn('ir_method_call0_preflight', ordinary)
        self.assertIn('slots = _PyFrame_Slots(frame)', ordinary)

    def test_terminal_consumer_rejects_changed_guard_or_origin(self):
        source = self.runtime_source()
        changes = (
            ('op(_GUARD_TYPE_VERSION,', 'inline_ok op(_GUARD_TYPE_VERSION,'),
            ('tp->tp_version_tag) != type_version',
             'tp->tp_version_tag) == type_version'),
            ('EXIT_IF(!FT_ATOMIC_LOAD_UINT8(_PyObject_InlineValues(owner_o)->valid));',
             'assert(FT_ATOMIC_LOAD_UINT8(_PyObject_InlineValues(owner_o)->valid));'),
            ('attr = PyValue_DUP(_PyValueGuarded_Load(class_value));\n            self = TAKE(owner);',
             'self = PyValue_DUP(_PyValueGuarded_Load(class_value));\n            attr = TAKE(owner);'),
        )
        for old, new in changes:
            with self.subTest(change=new):
                self.assertIn(old, source)
                with self.assertRaises(SyntaxError):
                    self.generate(source.replace(old, new, 1), runtime=True)

    def test_terminal_consumer_effects(self):
        self.source.write_text(self.runtime_source())
        with cases.handle_stderr():
            analysis = cases.analyzer.analyze_files([str(self.source)], None)
        formats = ir_generator.compute_formats(analysis)
        contracts = ir_generator.handler_contracts(analysis, formats)
        effects = contracts['LOAD_ATTR_METHOD_WITH_VALUES_CALL0'].effects
        for effect in ('_PyIR_PUSHES_FRAME', '_PyIR_RUNS_PYTHON', '_PyIR_ALLOCATES'):
            self.assertIn(effect, effects)
        self.assertNotIn('_PyIR_PUSHES_FRAME',
                         contracts['LOAD_ATTR_METHOD_WITH_VALUES'].effects)
        self.assertNotIn('_PyIR_ESCAPES', contracts['CALL_PY_EXACT_ARGS'].effects)
        self.assertTrue(analysis.uops['_PUSH_FRAME'].is_viable())


if __name__ == "__main__":
    unittest.main()
