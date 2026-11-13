"""Exceptional source effects need complete typed source, even when inert."""

from pathlib import Path
import tempfile
import unittest
from unittest import mock

from test import test_tools


test_tools.skip_if_missing("cases_generator")
with test_tools.imports_under_tool("cases_generator"):
    import analyzer
    import ir_error_effects
    import ir_generator
    import ir_numbers


class SourceNumbers(ir_numbers.NumberTable):
    def __init__(self):
        table = ir_numbers.load_table()
        super().__init__(numbers=table.numbers, constants=table.constants)

    def number(self, name, low, high, what):
        if name == "SAMPLE":
            return low
        return super().number(name, low, high, what)


class ErrorEffectTests(unittest.TestCase):
    def analyze(self, body, effect="-- value"):
        with tempfile.TemporaryDirectory() as directory:
            filename = Path(directory) / "source.c"
            filename.write_text("// BEGIN BYTECODES //\n"
                                f"inst(SAMPLE, ({effect})) {{ {body} }}\n"
                                "// END BYTECODES //\n")
            instruction = analyzer.analyze_files([str(filename)], numbers=SourceNumbers()).instructions["SAMPLE"]
        return instruction, ir_error_effects.analyze_error_effects(instruction)

    def test_inert_source_is_not_unknown(self):
        for body, effect in (("", "--"), ("JUMPTO(oparg);", "--"),
                             ("value = PyValue_XDUP(PyTuple_GET_VALUE(FRAME_CO_CONSTS, oparg));", "-- value")):
            with self.subTest(body=body):
                _, summary = self.analyze(body, effect)
                self.assertIsNotNone(summary)
                self.assertFalse(summary.error_sources)
                self.assertFalse(summary.unwind_sources)
                self.assertFalse(summary.may_fail)
                self.assertEqual(summary.homes, ir_error_effects.HomeEffect.NO_ARENA_WRITE)

    def test_helper_failure_and_both_explicit_routes_remain_distinct(self):
        instruction, summary = self.analyze("""
            PyObject *result = _PyEval_GetANext(PyValue_AsKnownObjectBorrow(input));
            if (result == NULL) { ERROR_NO_POP(); }
            if (oparg) { ERROR_IF(true); }
            output = PyValue_FromObjectSteal(result);
        """, "input -- input, output")
        self.assertIsNotNone(summary)
        self.assertTrue(summary.may_fail)
        self.assertTrue(summary.callback)
        self.assertEqual(summary.homes,
                         ir_error_effects.HomeEffect.PRESERVES_SCRATCH_ABSENCE)
        expected = {id(token) for part in instruction.parts
                    for token in part.body.tokens()
                    if token.text in {"ERROR_IF", "ERROR_NO_POP"}}
        self.assertEqual(summary.error_sources, expected)
        self.assertEqual(ir_generator.operand_error_sources(instruction), expected)
        self.assertEqual(len(summary.error_sources), 2)
        # This summary still has no authority to claim the emitter flushed
        # either branch. Its consumer must compare the actual receipts.
        self.assertFalse(summary.terminal_sources)
        for statement in ("DECREF_INPUTS();", "INPUTS_DEAD();"):
            with self.subTest(stack_operation=statement):
                instruction, summary = self.analyze(
                    f"if (oparg) {{ ERROR_NO_POP(); }} {statement}", "input --")
                self.assertEqual(ir_generator.operand_error_sources(instruction),
                                 summary.error_sources)
        instruction.parts.append(instruction.parts[0])
        self.assertIsNone(ir_error_effects.analyze_error_effects(instruction))

    def test_unknown_source_cannot_hide_behind_an_empty_error_set(self):
        for body in (
            "value = opaque();",
            "value = frame->base[oparg];",
            "PyValue *alias = &(value); value = PyValue_NULL;",
            "(value) = PyValue_NULL;",
            "value.bits = 0;",
            "int ERROR_IF = 0; value = PyValue_NULL;",
            "int _PyFrame_GetCode = 0; value = PyValue_NULL;",
            "if ((oparg = 0)) { value = PyValue_NULL; }",
            "oparg = 0; value = PyValue_NULL;",
            "for (int i = 0; i < oparg; i++) { value = PyValue_NULL; }",
            "goto private_label;",
        ):
            with self.subTest(body=body):
                _, summary = self.analyze(body)
                self.assertIsNone(summary)

    def test_native_observation_is_not_a_home_pointer_capability(self):
        for argument in ("frame", "slots", "&value", "PyValue_AsKnownObjectBorrow(value)"):
            _, summary = self.analyze(f"int result = check_periodics({argument}); ERROR_IF(result);", "value --")
            self.assertIsNone(summary)
        instruction, summary = self.analyze("int result = check_periodics(tstate); ERROR_IF(result);", "--")
        self.assertIsNotNone(summary)
        self.assertTrue(summary.may_fail)
        self.assertTrue(summary.callback)
        self.assertEqual(summary.homes,
                         ir_error_effects.HomeEffect.PRESERVES_SCRATCH_ABSENCE)
        self.assertEqual(ir_generator.operand_error_sources(instruction),
                         summary.error_sources)

    def test_actual_scalar_sources_use_the_same_summary(self):
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        if not source.is_file():
            self.skipTest("requires bytecodes.c")
        analysis = analyzer.analyze_files([str(source)])
        for name in ("NOP", "IR_JUMP", "LOAD_CONST", "LOAD_CONST_INLINE_INT", "GET_ANEXT"):
            with self.subTest(handler=name):
                instruction = {**analysis.instructions, **analysis.ir_only}[name]
                summary = ir_error_effects.analyze_error_effects(instruction)
                self.assertIsNotNone(summary)
                self.assertEqual(bool(summary.error_sources), name == "GET_ANEXT")

    def test_monitor_errors_keep_the_original_instruction_origin(self):
        call = ("int err = _Py_call_instrumentation(tstate, "
                "PY_MONITORING_EVENT_PY_RESUME, frame, this_instr); RELOAD_STACK();")
        adopt = "if (frame->instr_ptr != this_instr) { next_instr = frame->instr_ptr; }"
        _, summary = self.analyze(call + "ERROR_IF(err);" + adopt, "--")
        self.assertIsNotNone(summary)
        self.assertTrue(summary.changes_origin)
        self.assertEqual(summary.homes,
                         ir_error_effects.HomeEffect.PRESERVES_SCRATCH_ABSENCE)
        for body in (call + adopt + "ERROR_IF(err);",
                     call + "next_instr = this_instr; ERROR_IF(err);",
                     call + "frame->instr_ptr = next_instr; ERROR_IF(err);",
                     call.replace("frame, this_instr", "slots, this_instr") + "ERROR_IF(err);"):
            with self.subTest(body=body):
                _, summary = self.analyze(body, "--")
                self.assertIsNone(summary)

    def test_metadata_and_step_capabilities_require_audited_interfaces(self):
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        if not source.is_file():
            self.skipTest("requires bytecodes.c")
        analysis = analyzer.analyze_files([str(source)])
        formats = ir_generator.compute_formats(analysis)
        # Native signature alone is not a callback/home-effect certificate.
        previous = ir_error_effects.HELPERS["check_periodics"]
        unknown = ir_error_effects.HelperEffect(
            previous.inputs, previous.result, ir_error_effects.HomeEffect.UNKNOWN,
            previous.may_fail, previous.callback)
        with mock.patch.dict(ir_error_effects.HELPERS, {"check_periodics": unknown}):
            contracts = ir_generator.handler_contracts(analysis, formats)
        for name in ("RESUME", "INSTRUMENTED_RESUME"):
            self.assertFalse(contracts[name].error_routes)
            self.assertFalse(contracts[name].abrupt_operands)

        classify_boxes = ir_generator.boxes_into_record

        def resume_with_bridge(text):
            return "int check_instrumentation" in text or classify_boxes(text)

        # DISPATCH can release bridges after adopting a continuation. Its
        # possible failure then needs that new origin, not this source PC.
        with mock.patch.object(ir_generator, "boxes_into_record", resume_with_bridge):
            contracts = ir_generator.handler_contracts(analysis, formats)
        for name in ("RESUME", "INSTRUMENTED_RESUME"):
            self.assertFalse(contracts[name].error_routes)
            self.assertFalse(contracts[name].abrupt_operands)

        original = ir_generator.IREmitter.complete_step

        def missing_step_error_receipt(emitter, token, *args):
            result = original(emitter, token, *args)
            emitter.abrupt_operand_sources.discard(id(token))
            return result

        with mock.patch.object(ir_generator.IREmitter, "complete_step", missing_step_error_receipt):
            contracts = ir_generator.handler_contracts(analysis, formats)
        self.assertFalse(contracts["CLEANUP_THROW"].error_routes)
        self.assertFalse(contracts["CLEANUP_THROW"].abrupt_operands)

        text = source.read_text()
        changes = (
            ("&tstate->eval_breaker", "&frame->base[0]", "RESUME"),
            ("FT_ATOMIC_LOAD_UINTPTR_ACQUIRE", "opaque_metadata_reader", "RESUME"),
            ("IR_COMPLETE_STEP(_PY_IR_SEND_THROW, completed, oparg)",
             "int changed_target = oparg; "
             "IR_COMPLETE_STEP(_PY_IR_SEND_THROW, completed, changed_target)", "CLEANUP_THROW"),
            ("monitor_reraise(tstate, frame, this_instr)",
             "opaque_reraise(tstate, frame, this_instr)", "CLEANUP_THROW"),
        )
        with tempfile.TemporaryDirectory() as directory:
            changed = Path(directory) / "bytecodes.c"
            for before, after, name in changes:
                with self.subTest(change=after):
                    self.assertIn(before, text)
                    changed.write_text(text.replace(before, after))
                    altered = analyzer.analyze_files([str(changed)])
                    self.assertIsNone(ir_generator.operand_error_sources(altered.instructions[name]))

    def test_complete_routes_require_actual_error_and_publication_receipts(self):
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        if not source.is_file():
            self.skipTest("requires bytecodes.c")
        analysis = analyzer.analyze_files([str(source)])
        formats = ir_generator.compute_formats(analysis)
        contracts = ir_generator.handler_contracts(analysis, formats)
        self.assertEqual(contracts["IR_JUMP"].error_routes, ("_PyIR_ERROR_COMPLETE",))
        self.assertIn("_PyIR_ERROR_LOCAL_DST", contracts["LOAD_CONST"].error_routes)
        self.assertIn("_PyIR_ERROR_DIRECT", contracts["GET_ANEXT"].error_routes)
        for name in ("RESUME", "INSTRUMENTED_RESUME", "CLEANUP_THROW"):
            with self.subTest(handler=name):
                self.assertIn("_PyIR_ERROR_COMPLETE", contracts[name].error_routes)
                self.assertIn("_PyIR_ERROR_DIRECT", contracts[name].error_routes)
                self.assertTrue(contracts[name].abrupt_operands)
        for name in ("GET_LEN", "UNARY_NOT", "BINARY_OP_ADD"):
            with self.subTest(uncovered=name):
                self.assertFalse(contracts[name].error_routes)
        with mock.patch.object(ir_generator.IREmitter, "operand_error_flush", lambda *args: None):
            incomplete = ir_generator.handler_contracts(analysis, formats)
        self.assertFalse(incomplete["GET_ANEXT"].error_routes)
        original = ir_generator.IREmitter.release_displaced

        def missing_publication_receipt(emitter):
            original(emitter)
            emitter.error_local_dst = False

        with mock.patch.object(ir_generator.IREmitter, "release_displaced", missing_publication_receipt):
            incomplete = ir_generator.handler_contracts(analysis, formats)
        self.assertFalse(incomplete["LOAD_CONST"].error_routes)


if __name__ == "__main__":
    unittest.main()
