"""Selected errors replay the scalar homes the emitter actually changed."""

from dataclasses import replace
from pathlib import Path
import io
import tempfile
import unittest
from unittest import mock

from test import test_tools


test_tools.skip_if_missing("cases_generator")
with test_tools.imports_under_tool("cases_generator"):
    import analyzer
    import cwriter
    import ir_generator
    import ir_numbers
    import ir_read_effects
    import lexer


class SourceNumbers(ir_numbers.NumberTable):
    def __init__(self):
        table = ir_numbers.load_table()
        super().__init__(numbers=table.numbers, constants=table.constants)

    def number(self, name, low, high, what):
        if name == "SAMPLE":
            return low
        return super().number(name, low, high, what)


class ScalarStorageTests(unittest.TestCase):
    def contract(self, body):
        return self.program(f"inst(SAMPLE, (value --)) {{ {body} }}")

    def program(self, program, *, scratch_result=False, name="SAMPLE"):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "source.c"
            source.write_text("// BEGIN BYTECODES //\n"
                f"{program}\n"
                "// END BYTECODES //\n")
            analysis = analyzer.analyze_files([str(source)], numbers=SourceNumbers())
        formats = ir_generator.compute_formats(analysis)
        if scratch_result:
            formats[name] = replace(formats[name], store_dst=False)
        return ir_generator.handler_contracts(analysis, formats)[name]

    def test_scalar_release_uses_actual_save_vacate_and_callback(self):
        for body in (
                "PyValue held = TAKE(value); PyValue_CLOSE(held); ERROR_IF(oparg);",
                "DECREF_INPUTS(); ERROR_IF(oparg);"):
            with self.subTest(body=body):
                checkpoints = self.contract(body).error_checkpoints
                self.assertEqual(len(checkpoints), 1)
                actions = checkpoints[0].actions
                self.assertEqual([event.kind for event in actions], [
                    "_PyIR_OWNER_STORAGE_SAVE", "_PyIR_OWNER_STORAGE_VACATE",
                    "_PyIR_OWNER_STORAGE_CALLBACK"])
                self.assertEqual(checkpoints[0].node, len(actions))
                self.assertEqual(actions[1].flags, 1)

    def test_owner_left_only_in_native_local_denies_checkpoint(self):
        contract = self.contract("""
            PyValue held = TAKE(value);
            ERROR_IF(oparg);
            PyValue_CLOSE(held);
        """)
        self.assertFalse(contract.error_checkpoints)

    def test_error_flush_rehomes_the_actual_native_token(self):
        program = """
            op(_FIRST, (value -- saved)) {
                saved = PyValue_XDUP(value);
                PyValue_CLOSE(value);
            }
            op(_SECOND, (pending -- pending)) { ERROR_IF(oparg); }
            macro(SAMPLE) = _FIRST + _SECOND;
        """
        # A fixed scratch recipient is an actual home on every route. The
        # conditional TEMP_DST adapter cannot rehome an owner when A is L.
        self.assertFalse(self.program(program).error_checkpoints)
        contract = self.program(program, scratch_result=True)
        self.assertEqual(len(contract.error_checkpoints), 1)
        actions = contract.error_checkpoints[0].actions
        self.assertTrue(any(event.kind == "_PyIR_OWNER_STORAGE_PUBLISH" for event in actions))
        self.assertTrue(any(event.kind == "_PyIR_OWNER_STORAGE_COPY" for event in actions))

    def test_join_of_distinct_retirement_paths_keeps_no_native_owner(self):
        simple = "PyValue_CLOSE(held);"
        duplicate = "PyValue duplicate = PyValue_XDUP(held); PyValue_CLOSE(duplicate); PyValue_CLOSE(held);"
        for left, right in ((simple, duplicate), (duplicate, simple)):
            with self.subTest(left=left):
                contract = self.contract(f"""
                    PyValue held = TAKE(value);
                    if (oparg) {{ {left} }} else {{ {right} }}
                    ERROR_IF(oparg);
                """)
                self.assertEqual(len(contract.error_checkpoints), 1)
                actions = contract.error_checkpoints[0].actions
                self.assertEqual(actions[-1].kind, "_PyIR_OWNER_STORAGE_JOIN")
                self.assertTrue(any(event.kind == "_PyIR_OWNER_STORAGE_COPY" for event in actions))

    def test_conditional_retirement_does_not_close_other_path(self):
        contract = self.contract("""
            PyValue held = TAKE(value);
            if (oparg) { PyValue_CLOSE(held); }
            ERROR_IF(oparg);
        """)
        self.assertFalse(contract.error_checkpoints)

    def test_missing_release_receipt_cannot_hide_native_owner(self):
        with mock.patch.object(ir_generator.IRStack, "retire_value", lambda *args: None):
            self.assertFalse(self.contract(
                "DECREF_INPUTS(); ERROR_IF(oparg);").error_checkpoints)

    def test_retired_native_alias_cannot_be_duplicated_or_closed_again(self):
        for use in ("PyValue duplicate = PyValue_XDUP(alias); PyValue_CLOSE(duplicate);",
                    "PyValue_CLOSE(alias);"):
            with self.subTest(use=use):
                contract = self.contract(f"""
                    PyValue held = PyValue_XDUP(value);
                    PyValue_CLOSE(value);
                    PyValue alias = held;
                    PyValue_CLOSE(held);
                    {use}
                    ERROR_IF(oparg);
                """)
                self.assertFalse(contract.error_checkpoints)

    def test_shared_transport_summary_rejects_aliases_and_unknown_calls(self):
        for expression, expected in (
                ("((value))", ("copy", "value")),
                ("TAKE(value)", ("take", "value")),
                ("PyValue_XDUP(value)", ("dup", "value")),
                ("PyValue_NULL", ("literal", "PyValue_NULL")),
                ("OWNER(value)", None), ("opaque(value)", None),
                ("*alias", None), ("value.member", None)):
            with self.subTest(expression=expression):
                self.assertEqual(ir_read_effects.value_transport_expression(
                    list(lexer.tokenize(expression))), expected)

    def test_actual_retained_and_retired_input_producers(self):
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        analysis = analyzer.analyze_files([str(source)])
        formats = ir_generator.compute_formats(analysis)

        def emitted(name):
            out = cwriter.CWriter(io.StringIO(), 0, False)
            emitter = ir_generator.IREmitter(out, analysis, formats)
            emitter.handler_format = formats[name]
            emitter.allowed_error_checkpoints = None
            ir_generator.generate_handler(name, analysis.instructions[name],
                                          formats[name], emitter, out, True)
            return tuple(cp for cp in emitter.error_checkpoint_sites.values() if cp.id)

        retained = emitted("GET_ANEXT")
        self.assertEqual([checkpoint.id for checkpoint in retained], [1, 2])
        self.assertFalse(any(event.kind == "_PyIR_OWNER_STORAGE_VACATE"
                             for checkpoint in retained for event in checkpoint.actions))
        retired = emitted("UNARY_NOT")
        self.assertEqual([checkpoint.id for checkpoint in retired], [1])
        self.assertTrue(any(event.kind == "_PyIR_OWNER_STORAGE_VACATE"
                            for event in retired[0].actions))
        original = ir_generator.IRStack.take_field

        def missing_receipt(stack, field):
            original(stack, field)
            stack.home_write_receipts -= 1

        with mock.patch.object(ir_generator.IRStack, "take_field", missing_receipt):
            self.assertFalse(emitted("UNARY_NOT"))

    def test_instrumented_yield_error_copies_only_the_live_stack(self):
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        analysis = analyzer.analyze_files([str(source)])
        formats = ir_generator.compute_formats(analysis)
        for record in (False, True):
            with self.subTest(record=record):
                stream = io.StringIO()
                out = cwriter.CWriter(stream, 0, False)
                emitter = ir_generator.IREmitter(out, analysis, formats)
                name = "INSTRUMENTED_YIELD_VALUE"
                emitter.handler_format = formats[name]
                ir_generator.generate_handler(name, analysis.instructions[name],
                                              formats[name], emitter, out, record)
                self.assertIn("PROCESS_ERROR_AT(RAISE, 0, this_instr)", stream.getvalue())


if __name__ == "__main__":
    unittest.main()
