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
    def test_pre_observer_wrappers_require_actual_error_and_call_receipts(self):
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

            before = "value = PyValue_XDUP(PyTuple_GET_VALUE(FRAME_CO_CONSTS, oparg));"
            changed.write_text(text.replace(
                before, "if (oparg) { ERROR_NO_POP(); } " + before +
                " next_instr = this_instr;", 1))
            altered = analyzer.analyze_files([str(changed)])
            instruction = altered.instructions["LOAD_CONST"]
            self.assertIsNotNone(ir_generator.operand_error_sources(instruction))
            altered_formats = ir_generator.compute_formats(altered)
            self.assertTrue(altered_formats["LOAD_CONST"].store_dst)
            # The final local publication/refresh can fail after PC adoption,
            # even without a scalar box or a later explicit ERROR statement.
            contracts = ir_generator.handler_contracts(altered, altered_formats)
            self.assertFalse(contracts["LOAD_CONST"].error_routes)
            self.assertFalse(contracts["LOAD_CONST"].abrupt_operands)

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

    def test_send_routes_require_each_reached_adapter_receipt(self):
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        if not source.is_file():
            self.skipTest("requires bytecodes.c")
        analysis = analyzer.analyze_files([str(source)])
        formats = ir_generator.compute_formats(analysis)
        names = ("SEND", "SEND_GEN", "SEND_VIRTUAL", "SEND_ASYNC_GEN")
        contracts = ir_generator.handler_contracts(analysis, formats)
        for name in names:
            with self.subTest(handler=name):
                self.assertTrue(contracts[name].abrupt_operands)
                self.assertIn("_PyIR_ERROR_COMPLETE", contracts[name].error_routes)
                self.assertIn("_PyIR_ERROR_DIRECT", contracts[name].error_routes)
                # Normal yielded/completion transport remains independent.
                self.assertEqual(contracts[name].ownership_events[0].flags, 3)
        original_jump = ir_generator.IREmitter.emit_jump_to_predicted

        def wrong_target_geometry(emitter, instruction):
            if instruction.name == "SEND_GEN":
                target = ir_generator.replace(formats["SEND"], roles=["NONE"] * 4)
                with mock.patch.dict(emitter.formats, {"SEND": target}):
                    return original_jump(emitter, instruction)
            return original_jump(emitter, instruction)

        def lose_receipt(method, attribute):
            original = getattr(ir_generator.IREmitter, method)

            def incomplete(emitter, token, *args):
                result = original(emitter, token, *args)
                getattr(emitter, attribute).discard(id(token))
                return result

            return incomplete

        def missing_guard_receipt(emitter, instruction):
            original_jump(emitter, instruction)
            if instruction.name == "SEND_ASYNC_GEN":
                emitter.error_terminal_sources.discard(emitter.error_redispatch_origin)

        original_flush = ir_generator.IREmitter.operand_error_flush

        def missing_error_flush(emitter, *args):
            original_flush(emitter, *args)
            emitter.abrupt_operand_sources.discard(emitter.abrupt_operand_origin)

        changes = (
            ("operand_error_flush", missing_error_flush),
            ("complete_step", lose_receipt("complete_step", "abrupt_operand_sources")),
            ("enter_generator", lose_receipt("enter_generator", "abrupt_operand_sources")),
            ("dispatch_same_oparg", lose_receipt("dispatch_same_oparg", "error_terminal_sources")),
            ("emit_jump_to_predicted", missing_guard_receipt),
            ("emit_jump_to_predicted", wrong_target_geometry),
        )
        for method, replacement in changes:
            with self.subTest(route=method, replacement=replacement.__name__):
                with mock.patch.object(ir_generator.IREmitter, method, replacement):
                    incomplete = ir_generator.handler_contracts(analysis, formats)
                for name in names:
                    self.assertFalse(incomplete[name].error_routes, name)
                    self.assertFalse(incomplete[name].abrupt_operands, name)

    def test_send_source_changes_revoke_the_whole_family(self):
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        if not source.is_file():
            self.skipTest("requires bytecodes.c")
        text = source.read_text()
        names = ("SEND", "SEND_GEN", "SEND_VIRTUAL", "SEND_ASYNC_GEN")
        changes = (
            ("EXIT_IF(!PyValue_IsNone(val));",
             "window[0] = PyValue_NULL; EXIT_IF(!PyValue_IsNone(val));"),
            ("_PyValue_Send(receiver_o, v)", "opaque_send(receiver_o, v)"),
            ("IR_ENTER_GENERATOR(gen, _PY_IR_SEND);",
             "next_instr = this_instr; IR_ENTER_GENERATOR(gen, _PY_IR_SEND);"),
            ("IR_COMPLETE_STEP(_PY_IR_SEND, completed, oparg);",
             "int changed_target = oparg; "
             "IR_COMPLETE_STEP(_PY_IR_SEND, completed, changed_target);"),
            ("EXIT_IF(!PyValue_IsNone(val));",
             "PyValue *alias = &val; EXIT_IF(!PyValue_IsNone(val));"),
        )
        with tempfile.TemporaryDirectory() as directory:
            changed = Path(directory) / "bytecodes.c"
            for before, after in changes:
                with self.subTest(change=after):
                    self.assertIn(before, text)
                    changed.write_text(text.replace(before, after, 1))
                    altered = analyzer.analyze_files([str(changed)])
                    formats = ir_generator.compute_formats(altered)
                    contracts = ir_generator.handler_contracts(altered, formats)
                    for name in names:
                        self.assertFalse(contracts[name].error_routes, name)
                        self.assertFalse(contracts[name].abrupt_operands, name)

    def test_read_only_projections_keep_actual_late_error_routes(self):
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
        self.assertEqual(contracts["JUMP_BACKWARD_NO_INTERRUPT"].error_routes,
                         ("_PyIR_ERROR_COMPLETE",))
        self.assertFalse(contracts["LOAD_COMMON_CONSTANT"].abrupt_operands)
        original = ir_generator.IREmitter.release_displaced

        def missing_binding_receipt(emitter):
            original(emitter)
            emitter.error_local_dst = False

        with mock.patch.object(ir_generator.IREmitter, "release_displaced", missing_binding_receipt):
            incomplete = ir_generator.handler_contracts(analysis, formats)
        self.assertFalse(incomplete["LOAD_COMMON_CONSTANT"].error_routes)
        original_boxes = ir_generator.boxes_into_record

        def has_bridge(text):
            return "common_consts" in text or original_boxes(text)

        with mock.patch.object(ir_generator, "boxes_into_record", has_bridge):
            boxed = ir_generator.handler_contracts(analysis, formats)
        self.assertIn("_PyIR_ERROR_BRIDGE", boxed["LOAD_COMMON_CONSTANT"].error_routes)


if __name__ == "__main__":
    unittest.main()
