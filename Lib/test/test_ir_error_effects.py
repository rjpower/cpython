"""Exceptional source effects need complete typed source, even when inert."""

import dataclasses
from pathlib import Path
import tempfile
import unittest
from unittest import mock

from test import test_generated_cases as cases
from test import test_tools


test_tools.skip_if_missing("cases_generator")
with test_tools.imports_under_tool("cases_generator"):
    import analyzer
    import ir_contract
    import ir_error_effects
    import ir_generator



def need(*names):
    """Skip the test unless bytecodes.c still defines every one of names with
    a body: these tests read the real handlers of the old generator."""
    if cases.live_names(cases.BYTECODES.read_text(), names) != list(names):
        raise unittest.SkipTest("no live handler left; the old generator goes at D")


class ErrorEffectTests(unittest.TestCase):
    def test_pre_observer_wrappers_require_actual_error_and_call_receipts(self):
        need("INSTRUMENTED_INSTRUCTION", "INSTRUMENTED_LINE")
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        if not source.is_file():
            self.skipTest("requires bytecodes.c")
        analysis = analyzer.analyze_files([str(source)])
        formats = ir_generator.compute_formats(analysis)
        names = ("INSTRUMENTED_LINE", "INSTRUMENTED_INSTRUCTION")
        contracts = ir_generator.handler_contracts(analysis, formats)
        line = analysis.instructions["INSTRUMENTED_LINE"]
        self.assertFalse(any(call.call.text == "_PyCode_GetOriginalOpcode"
                             for call in line.properties.escaping_calls.values()))
        for name in names:
            self.assertTrue(contracts[name].observer_entry)
            # Redirecting observer wrappers do not become ordinary-body
            # error contracts or full readable-home shapes.
            self.assertFalse(contracts[name].error_routes)
        with mock.patch.object(ir_generator.IREmitter, "observer_call_receipt", lambda *args: None):
            missing = ir_generator.handler_contracts(analysis, formats)
        self.assertFalse(any(missing[name].observer_entry for name in names))
        original = ir_generator.IREmitter.goto_label

        def missing_line_error(emitter, token, label, storage):
            original(emitter, token, label, storage)
            emitter.observer_errors.discard(id(token))

        with mock.patch.object(ir_generator.IREmitter, "goto_label", missing_line_error):
            missing = ir_generator.handler_contracts(analysis, formats)
        self.assertFalse(missing[names[0]].observer_entry)
        self.assertTrue(missing[names[1]].observer_entry)
        original_flush = ir_generator.IREmitter.operand_error_flush

        def missing_instruction_error(emitter, *args):
            original_flush(emitter, *args)
            emitter.observer_errors.discard(emitter.abrupt_operand_origin)

        with mock.patch.object(ir_generator.IREmitter, "operand_error_flush", missing_instruction_error):
            missing = ir_generator.handler_contracts(analysis, formats)
        self.assertFalse(missing[names[1]].observer_entry)

    def test_pre_observer_source_uses_typed_metadata_and_exact_callback_roles(self):
        need("INSTRUMENTED_LINE")
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        if not source.is_file():
            self.skipTest("requires bytecodes.c")
        text = source.read_text()
        changes = (
            ("int original_opcode = 0;", "int original_opcode = 0; window[0] = PyValue_NULL;"),
            ("int original_opcode = 0;", "int original_opcode = 0; opaque(frame);"),
            ("tstate, frame, this_instr, prev_instr", "tstate, frame, next_instr, prev_instr"),
            ("code->_co_monitoring->lines, index", "code->_co_monitoring->lines, 0"),
            ("original_opcode = _PyCode_GetOriginalOpcode(code->_co_monitoring->lines, index);",
             "original_opcode = 42;"),
            ("int original_opcode = 0;", "int _Py_call_instrumentation_line = 0; int original_opcode = 0;"),
            # Feedback is the running handler's: a plain scalar or the
            # observed opcode does not name it
            ("_PyFb_HasEntry(opcode)", "_PyFb_HasEntry(oparg)"),
            ("FB_HEADER_OF(next_instr, opcode)", "FB_HEADER_OF(next_instr, oparg)"),
            ("_PyFb_HasEntry(opcode)", "_PyFb_HasEntry(original_opcode)"),
            ("FB_HEADER_OF(next_instr, opcode)", "FB_HEADER_OF(next_instr, original_opcode)"),
        )
        with tempfile.TemporaryDirectory() as directory:
            changed = Path(directory) / "bytecodes.c"
            for before, after in changes:
                with self.subTest(change=after):
                    self.assertIn(before, text)
                    changed.write_text(text.replace(before, after, 1))
                    analysis = analyzer.analyze_files([str(changed)])
                    formats = ir_generator.compute_formats(analysis)
                    inst = analysis.instructions["INSTRUMENTED_LINE"]
                    self.assertIsNone(ir_generator.observer_entry_source(inst, formats[inst.name]))

    def analyze(self, body, effect="-- value"):
        with tempfile.TemporaryDirectory() as directory:
            filename = Path(directory) / "source.c"
            filename.write_text("// BEGIN BYTECODES //\n"
                                f"inst(SAMPLE, ({effect})) {{ {body} }}\n"
                                "// END BYTECODES //\n")
            instruction = analyzer.analyze_files([str(filename)]).instructions["SAMPLE"]
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

    def test_helper_effects_are_the_rows(self):
        # A call's reentry, homes and observer claims are its row's: the
        # same signature has other effects when the row declares others
        effect = ir_error_effects.HomeEffect
        rows = ir_contract.rows()
        signature = ir_error_effects.SIGNATURES["check_periodics"]
        for changes, homes, observer in (
                ({}, effect.PRESERVES_SCRATCH_ABSENCE, True),
                ({"homes": "canonical", "observer": "none"},
                 effect.PRESERVES_CANONICAL_OR_NULL, False),
                ({"homes": "unknown"}, effect.UNKNOWN, True),
                ({"effects": frozenset({"reads"})}, effect.NO_ARENA_WRITE, True)):
            with self.subTest(changes=changes):
                row = dataclasses.replace(rows["check_periodics"], **changes)
                with mock.patch.dict(rows, {"check_periodics": row}):
                    helper = ir_error_effects.helper_effect("check_periodics", signature)
                self.assertEqual(helper.homes, homes)
                self.assertEqual(helper.opens_current_observer, observer)
                self.assertEqual(helper.callback, homes != effect.NO_ARENA_WRITE)
        self.assertEqual(ir_error_effects.HELPERS["PyValue_XCLOSE"].homes,
                         effect.PRESERVES_CANONICAL_OR_NULL)
        self.assertEqual(ir_error_effects.HELPERS["_PyEval_GetAwaitable"].homes,
                         effect.UNKNOWN)

    def test_a_failure_is_the_rows_raises(self):
        # A helper that runs Python and reports failure (NULL, a negative
        # status, a `raised` outcome) with an exception set raises; the error
        # walk reads that effect from the row, and no signature carries it
        rows = ir_contract.rows()
        failing = [row.name for row in rows.values()
                   if row.kind == "HELPER" and "runs_python" in row.effects
                   and (row.result.shape in ("VALUE_OR_NULL", "STATUS")
                        or any(case.raised for case in row.result.cases))]
        self.assertGreater(len(failing), 40)
        for name in failing:
            with self.subTest(row=name):
                self.assertIn("raises", rows[name].effects)
        # A scalar status (-1, or NULL, with an exception set) is no shape
        for name in ("_PyValue_Contains", "_PyValue_DelItem", "_PyValue_IsInstance",
                     "_PyValue_SetAttr", "_PyValue_SetItem", "_PyValue_Size",
                     "PyCell_NewValue", "PyObject_HasAttrWithError", "PyType_GenericAlloc",
                     "value_dict_merge_uniq", "PyDict_PopValue", "PyDict_SetValue",
                     "PyDict_Update", "PySet_New", "_PyDict_ContainsValue",
                     "_PyDict_SetValue", "_PyDict_SetValue_Take2", "_PyList_AppendTakeValue",
                     "_PyList_Extend", "_PyList_SliceSubscript", "_PySet_AddValueTakeRef",
                     "_PySet_Contains", "_PySet_Update", "_Py_call_instrumentation_2args",
                     "_Py_call_instrumentation_jump", "monitor_stop_iteration",
                     "_PyEval_StoreName", "_PyEval_StoreSlice"):
            with self.subTest(status=name):
                self.assertIn("raises", rows[name].effects)
        for name in ("_PyEval_GetAwaitable", "_PyValue_IsTrue", "_PyValue_Negative",
                     "_PyEval_GetANext", "check_periodics", "_Py_Instrument",
                     "_Py_call_instrumentation", "_Py_call_instrumentation_line",
                     "_Py_call_instrumentation_instruction", "IR_COMPLETE_STEP"):
            with self.subTest(carried=name):
                self.assertTrue(ir_error_effects.HELPERS[name].may_fail)

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
                         ir_error_effects.HomeEffect.PRESERVES_CANONICAL_OR_NULL)
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
        need("NOP", "IR_JUMP", "LOAD_COMMON_CONSTANT", "GET_ANEXT")
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        if not source.is_file():
            self.skipTest("requires bytecodes.c")
        analysis = analyzer.analyze_files([str(source)])
        for name in ("NOP", "IR_JUMP", "LOAD_COMMON_CONSTANT", "UNARY_INVERT"):
            with self.subTest(handler=name):
                instruction = {**analysis.instructions, **analysis.ir_only}[name]
                summary = ir_error_effects.analyze_error_effects(instruction)
                self.assertIsNotNone(summary)
                self.assertEqual(bool(summary.error_sources), name == "UNARY_INVERT")

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
        need("CLEANUP_THROW", "INSTRUMENTED_RESUME", "LOAD_COMMON_CONSTANT", "RESUME")
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
            ("IR_COMPLETE_STEP(_PY_IR_SEND_THROW, completed, IR_EXIT)",
             "int changed_target = IR_EXIT; "
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

            before = "value = PyValue_DUP(constant);"
            # The changed body leaves the leaf subset (ir_leaf.py)
            changed.write_text(text.replace(
                before, "if (oparg) { ERROR_NO_POP(); } " + before +
                " next_instr = this_instr;", 1).replace(
                "inst(LOAD_COMMON_CONSTANT, ( -- value: owned))\n            effects(reads) exits(next)",
                "inst(LOAD_COMMON_CONSTANT, ( -- value: owned))\n"
                # The moved next_instr is where the word continues (V12)
                "            effects(reads, raises) exits(jump, raise)", 1))
            altered = analyzer.analyze_files([str(changed)])
            instruction = altered.instructions["LOAD_COMMON_CONSTANT"]
            self.assertIsNotNone(ir_generator.operand_error_sources(instruction))
            altered_formats = ir_generator.compute_formats(altered)
            self.assertTrue(altered_formats["LOAD_COMMON_CONSTANT"].store_dst)
            # The final local publication/refresh can fail after PC adoption,
            # even without a scalar box or a later explicit ERROR statement.
            contracts = ir_generator.handler_contracts(altered, altered_formats)
            self.assertFalse(contracts["LOAD_COMMON_CONSTANT"].error_routes)
            self.assertFalse(contracts["LOAD_COMMON_CONSTANT"].abrupt_operands)

    def test_complete_routes_require_actual_error_and_publication_receipts(self):
        need("IR_JUMP", "LOAD_COMMON_CONSTANT", "GET_ANEXT", "RESUME", "INSTRUMENTED_RESUME", "CLEANUP_THROW", "UNARY_INVERT", "GET_LEN", "UNARY_NEGATIVE", "BINARY_OP_ADD")
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        if not source.is_file():
            self.skipTest("requires bytecodes.c")
        analysis = analyzer.analyze_files([str(source)])
        formats = ir_generator.compute_formats(analysis)
        contracts = ir_generator.handler_contracts(analysis, formats)
        self.assertEqual(contracts["IR_JUMP"].error_routes, ("_PyIR_ERROR_COMPLETE",))
        self.assertIn("_PyIR_ERROR_LOCAL_DST", contracts["LOAD_COMMON_CONSTANT"].error_routes)
        self.assertIn("_PyIR_ERROR_DIRECT", contracts["GET_ANEXT"].error_routes)
        for name in ("CLEANUP_THROW",):
            with self.subTest(handler=name):
                self.assertIn("_PyIR_ERROR_COMPLETE", contracts[name].error_routes)
                self.assertIn("_PyIR_ERROR_DIRECT", contracts[name].error_routes)
                self.assertTrue(contracts[name].abrupt_operands)
        with mock.patch.object(ir_generator.IREmitter, "operand_error_flush", lambda *args: None):
            incomplete = ir_generator.handler_contracts(analysis, formats)
        self.assertFalse(incomplete["UNARY_INVERT"].error_routes)
        original = ir_generator.IREmitter.finish_stores

        def missing_publication_receipt(emitter, stack):
            original(emitter, stack)
            emitter.error_local_dst = False

        with mock.patch.object(ir_generator.IREmitter, "finish_stores", missing_publication_receipt):
            incomplete = ir_generator.handler_contracts(analysis, formats)
        self.assertFalse(incomplete["LOAD_COMMON_CONSTANT"].error_routes)

    def test_read_only_projections_keep_actual_late_error_routes(self):
        need("IR_JUMP", "LOAD_COMMON_CONSTANT")
        constant = ("assert(oparg < NUM_COMMON_CONSTANTS); "
                    "value = PyValue_DUP(tstate->interp->common_consts[oparg]);")
        _, summary = self.analyze(constant)
        self.assertIsNotNone(summary)
        self.assertEqual(summary.homes, ir_error_effects.HomeEffect.NO_ARENA_WRITE)
        self.assertFalse(summary.error_sources)
        _, summary = self.analyze(
            "assert(JUMP_TARGET(oparg) < next_instr); JUMPTO(oparg);", "--")
        self.assertIsNotNone(summary)
        self.assertFalse(summary.may_fail)
        for before, after in (
            ("common_consts", "opaque_consts"),
            ("[oparg]", "[frame]"),
            ("[oparg]", "[oparg++]"),
            ("tstate->interp->common_consts[oparg]", "&tstate->interp->common_consts[oparg]"),
        ):
            with self.subTest(projection=after):
                self.assertIsNone(self.analyze(constant.replace(before, after))[1])
        for body in (
            "tstate->interp->common_consts[oparg] = PyValue_NULL;",
            "int NUM_COMMON_CONSTANTS = 1; " + constant,
            "oparg = 0; " + constant,
            "value = PyValue_DUP(tstate->interp->common_consts[oparg].bits);",
        ):
            with self.subTest(mutation=body):
                self.assertIsNone(self.analyze(body)[1])
        for target in ("opaque_target(oparg)", "JUMP_TARGET(frame)",
                       "JUMP_TARGET(oparg++)", "&JUMP_TARGET(oparg)"):
            with self.subTest(target=target):
                self.assertIsNone(self.analyze(
                    f"assert({target} < next_instr); JUMPTO(oparg);", "--")[1])

        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        if not source.is_file():
            self.skipTest("requires bytecodes.c")
        analysis = analyzer.analyze_files([str(source)])
        formats = ir_generator.compute_formats(analysis)
        contracts = ir_generator.handler_contracts(analysis, formats)
        # A scratch recipient has no ordinary failure; a binding recipient
        # can still fail during the actual displaced-local refresh. Keep the
        # conditional route in metadata rather than treating PURE as inert.
        self.assertEqual(contracts["LOAD_COMMON_CONSTANT"].error_routes,
                         ("_PyIR_ERROR_COMPLETE", "_PyIR_ERROR_LOCAL_DST"))
        self.assertEqual(contracts["IR_JUMP"].error_routes,
                         ("_PyIR_ERROR_COMPLETE",))
        self.assertFalse(contracts["LOAD_COMMON_CONSTANT"].abrupt_operands)
        original = ir_generator.IREmitter.finish_stores

        def missing_binding_receipt(emitter, stack):
            original(emitter, stack)
            emitter.error_local_dst = False

        with mock.patch.object(ir_generator.IREmitter, "finish_stores", missing_binding_receipt):
            incomplete = ir_generator.handler_contracts(analysis, formats)
        self.assertFalse(incomplete["LOAD_COMMON_CONSTANT"].error_routes)


if __name__ == "__main__":
    unittest.main()
