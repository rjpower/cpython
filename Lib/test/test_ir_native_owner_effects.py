"""Error checkpoints cannot lose owners held only by native object locals."""

from dataclasses import replace
from pathlib import Path
import tempfile
import unittest
from unittest import mock

from test import test_tools


test_tools.skip_if_missing("cases_generator")
with test_tools.imports_under_tool("cases_generator"):
    import analyzer
    import ir_error_effects
    import ir_numbers


class SourceNumbers(ir_numbers.NumberTable):
    def __init__(self):
        table = ir_numbers.load_table()
        super().__init__(numbers=table.numbers, constants=table.constants)

    def number(self, name, low, high, what):
        if name == "SAMPLE":
            return low
        return super().number(name, low, high, what)


class NativeOwnerEffectTests(unittest.TestCase):
    def analyze_program(self, program, supported=True):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "source.c"
            source.write_text(f"// BEGIN BYTECODES //\n{program}\n// END BYTECODES //\n")
            instruction = analyzer.analyze_files(
                [str(source)], numbers=SourceNumbers()).instructions["SAMPLE"]
        summary = ir_error_effects.analyze_error_effects(instruction)
        if supported:
            self.assertIsNotNone(summary)
        else:
            self.assertIsNone(summary)
        errors = [id(token) for part in instruction.parts
                  if isinstance(part, analyzer.Uop) for token in part.body.tokens()
                  if token.text in {"ERROR_IF", "ERROR_NO_POP"}]
        return instruction, summary, errors

    def analyze(self, body, supported=True):
        return self.analyze_program(f"inst(SAMPLE, (input -- output)) {{ {body} }}", supported)

    def test_null_failure_and_stolen_success_close_the_native_owner(self):
        instruction, summary, errors = self.analyze("""
            PyObject *result = _PyEval_GetANext(PyValue_AsKnownObjectBorrow(input));
            if (result == NULL) { ERROR_NO_POP(); }
            output = PyValue_FromObjectSteal(result);
            ERROR_IF(oparg);
        """)
        self.assertEqual(summary.native_closed_errors, frozenset(errors))
        self.assertEqual(summary.native_closed_entries,
                         frozenset(id(part) for part in instruction.parts
                                   if isinstance(part, analyzer.Uop)))
        self.assertTrue(summary.native_exit_closed)

    def test_later_error_cannot_drop_a_successful_native_result(self):
        _, summary, errors = self.analyze("""
            PyObject *result = _PyEval_GetANext(PyValue_AsKnownObjectBorrow(input));
            ERROR_IF(result == NULL);
            ERROR_IF(oparg);
            output = PyValue_FromObjectSteal(result);
        """)
        self.assertEqual(summary.native_closed_errors, frozenset(errors[:1]))
        self.assertTrue(summary.native_exit_closed)
        # The ordinary error census remains intact: owner closure is an
        # additional positive-checkpoint obligation, not absence permission.
        self.assertEqual(summary.error_sources, frozenset(errors))

    def test_other_owner_prevents_null_guard_from_closing_the_route(self):
        _, summary, errors = self.analyze("""
            PyObject *first = Py_NewRef(PyValue_AsKnownObjectBorrow(input));
            PyObject *second = _PyEval_GetANext(first);
            ERROR_IF(second == NULL);
            output = PyValue_FromObjectSteal(second);
        """)
        self.assertFalse(summary.native_closed_errors)
        self.assertFalse(summary.native_exit_closed)
        self.assertEqual(summary.error_sources, frozenset(errors))

    def test_alias_overwrite_and_repeated_steal_revoke_positive_closure(self):
        for tail in (
                "PyObject *alias = result; output = PyValue_FromObjectSteal(alias);",
                "result = Py_NewRef(PyValue_AsKnownObjectBorrow(input)); output = PyValue_FromObjectSteal(result);",
                "output = PyValue_FromObjectSteal(result); output = PyValue_FromObjectSteal(result);",
                "output = PyValue_FromObjectSteal(PyValue_AsKnownObjectBorrow(input));"):
            with self.subTest(tail=tail):
                _, summary, errors = self.analyze("""
                    PyObject *result = _PyEval_GetANext(PyValue_AsKnownObjectBorrow(input));
                    ERROR_IF(result == NULL);
                """ + tail + " ERROR_IF(oparg);")
                self.assertIn(errors[0], summary.native_closed_errors)
                self.assertNotIn(errors[-1], summary.native_closed_errors)
                self.assertFalse(summary.native_exit_closed)

    def test_branch_join_and_compound_null_guard_are_conservative(self):
        for middle in (
                "if (oparg) { output = PyValue_FromObjectSteal(result); }",
                "ERROR_IF(oparg && result == NULL);",
                "if (result != NULL) { ERROR_IF(oparg); } else { ERROR_NO_POP(); }"):
            with self.subTest(middle=middle):
                _, summary, errors = self.analyze("""
                    PyObject *result = _PyEval_GetANext(PyValue_AsKnownObjectBorrow(input));
                """ + middle + " ERROR_IF(oparg);")
                self.assertNotIn(errors[-1], summary.native_closed_errors)
                self.assertFalse(summary.native_exit_closed)

    def test_nested_conversion_cannot_hide_the_transferred_canonical_owner(self):
        _, summary, errors = self.analyze("""
            PyObject *result = _PyEval_GetANext(PyValue_AsKnownObjectBorrow(input));
            ERROR_IF(result == NULL);
            ERROR_IF(PyValue_IsNull(PyValue_FromObjectSteal(result)));
        """)
        self.assertIn(errors[0], summary.native_closed_errors)
        self.assertNotIn(errors[1], summary.native_closed_errors)
        self.assertFalse(summary.native_exit_closed)

    def test_both_branches_can_discharge_the_same_native_owner(self):
        _, summary, errors = self.analyze("""
            PyObject *result = _PyEval_GetANext(PyValue_AsKnownObjectBorrow(input));
            ERROR_IF(result == NULL);
            if (oparg) { output = PyValue_FromObjectSteal(result); }
            else { output = PyValue_FromObjectSteal(result); }
            ERROR_IF(oparg);
        """)
        self.assertEqual(summary.native_closed_errors, frozenset(errors))
        self.assertTrue(summary.native_exit_closed)

    def test_object_helper_needs_an_explicit_result_ownership_contract(self):
        contract = ir_error_effects.HELPERS["_PyEval_GetANext"]
        with mock.patch.dict(ir_error_effects.HELPERS, {
                "_PyEval_GetANext": replace(contract, result_owned=None)}):
            _, summary, _ = self.analyze("""
                PyObject *result = _PyEval_GetANext(PyValue_AsKnownObjectBorrow(input));
                ERROR_IF(result == NULL);
                output = PyValue_FromObjectSteal(result);
            """)
        self.assertFalse(summary.native_closed_errors)
        self.assertFalse(summary.native_exit_closed)

    def test_uop_entry_requires_preceding_native_owners_to_be_closed(self):
        for publish, closed in (("output = PyValue_FromObjectSteal(result);", True),
                                ("output = TAKE(input);", False)):
            with self.subTest(publish=publish):
                instruction, summary, errors = self.analyze_program("""
                    op(_FIRST, (input -- output)) {
                        PyObject *result = _PyEval_GetANext(PyValue_AsKnownObjectBorrow(input));
                        ERROR_IF(result == NULL);
                        PUBLISH
                    }
                    op(_SECOND, (output -- again)) {
                        ERROR_IF(oparg);
                        again = TAKE(output);
                    }
                    macro(SAMPLE) = _FIRST + _SECOND;
                """.replace("PUBLISH", publish))
                parts = [part for part in instruction.parts if isinstance(part, analyzer.Uop)]
                self.assertIn(id(parts[0]), summary.native_closed_entries)
                self.assertEqual(id(parts[1]) in summary.native_closed_entries, closed)
                self.assertEqual(errors[-1] in summary.native_closed_errors, closed)
                self.assertEqual(summary.native_exit_closed, closed)

    def test_actual_get_anext_closes_only_its_null_error(self):
        source = Path(__file__).resolve().parents[2] / "Python/bytecodes.c"
        if not source.is_file():
            self.skipTest("requires bytecodes.c")
        analysis = analyzer.analyze_files([str(source)])
        instruction = analysis.instructions["GET_ANEXT"]
        summary = ir_error_effects.analyze_error_effects(instruction)
        self.assertIsNotNone(summary)
        self.assertEqual(summary.native_closed_errors, summary.error_sources)
        self.assertTrue(summary.native_exit_closed)

    def test_native_truth_predicate_and_canonical_literals_share_the_parser(self):
        _, summary, errors = self.analyze("""
            int truth = _PyValue_IsTrue(input);
            PyValue_CLOSE(input);
            ERROR_IF(truth < 0);
            output = truth ? PyValue_False : PyValue_True;
        """)
        self.assertIn("_PyValue_IsTrue", summary.helpers)
        self.assertTrue(summary.may_fail)
        self.assertTrue(summary.callback)
        self.assertEqual(summary.homes,
                         ir_error_effects.HomeEffect.PRESERVES_CANONICAL_OR_NULL)
        self.assertEqual(summary.native_closed_errors, frozenset(errors))
        self.assertTrue(summary.native_exit_closed)
        self.analyze("int PyValue_None = 0; output = PyValue_NULL;", supported=False)


if __name__ == "__main__":
    unittest.main()
