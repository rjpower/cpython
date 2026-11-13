"""Native scalar result facts become home facts at actual publication."""

from dataclasses import replace
from unittest import mock
import unittest

from test import test_tools
from test import test_ir_scalar_storage

test_tools.skip_if_missing("cases_generator")
with test_tools.imports_under_tool("cases_generator"):
    import ir_error_effects
    import ir_generator


class ScalarResultTests(unittest.TestCase):
    def program(self, body, **kw):
        return test_ir_scalar_storage.ScalarStorageTests().program(
            "inst(SAMPLE, (value -- out)) { " + body + " }", **kw)

    def test_nullable_native_result_uses_distinct_selected_guard_arms(self):
        contract = self.program("""
            out = _PyValue_Invert(value);
            if (PyValue_IsNull(out)) { ERROR_NO_POP(); }
            PyValue_CLOSE(value);
        """, scratch_result=True)
        self.assertEqual(len(contract.error_checkpoints), 1)
        error = contract.error_checkpoints[0]
        self.assertEqual(error.actions[-1].kind, "_PyIR_OWNER_STORAGE_GUARD")
        self.assertEqual(error.actions[-1].flags, 0)
        normal = contract.normal_storage
        self.assertIsNotNone(normal)
        results = [a for a in normal.actions if a.kind == "_PyIR_OWNER_STORAGE_RESULT"]
        self.assertEqual([a.flags for a in results], [0])
        self.assertTrue(any(a.kind == "_PyIR_OWNER_STORAGE_GUARD" and a.flags == 1
                            for a in normal.actions))
        publication = [a for a in normal.actions if a.kind == "_PyIR_OWNER_STORAGE_PUBLISH"]
        self.assertEqual(len(publication), 1)
        # A private result is normalized by the actual store, not by its
        # source VALUE type or by the successful non-NULL test.
        self.assertFalse(publication[0].flags & 4)

    def test_exact_error_guard_closes_only_the_null_result(self):
        contract = self.program("""
            PyValue result = _PyValue_Invert(value);
            PyValue_CLOSE(value);
            ERROR_IF(PyValue_IsNull(result));
            out = result;
        """, scratch_result=True)
        self.assertEqual(len(contract.error_checkpoints), 1)
        self.assertIsNotNone(contract.normal_storage)
        for condition in ("oparg", "PyValue_IsNull(result) && oparg"):
            with self.subTest(condition=condition):
                unknown = self.program(f"""
                    PyValue result = _PyValue_Invert(value);
                    PyValue_CLOSE(value);
                    ERROR_IF({condition}); out = result;
                """, scratch_result=True)
                self.assertFalse(unknown.error_checkpoints)

    def test_value_type_alone_has_no_result_ownership_or_canonicality(self):
        helper = ir_error_effects.HELPERS["_PyValue_Invert"]
        for unknown in (replace(helper, result_owned=None), replace(helper, result_owned=False)):
            with self.subTest(owned=unknown.result_owned), mock.patch.dict(
                    ir_error_effects.HELPERS, {"_PyValue_Invert": unknown}):
                contract = self.program("""
                    out = _PyValue_Invert(value);
                    if (PyValue_IsNull(out)) { ERROR_NO_POP(); }
                    PyValue_CLOSE(value);
                """, scratch_result=True)
                self.assertFalse(contract.error_checkpoints)
                self.assertIsNone(contract.normal_storage)

    def test_inline_result_has_explicit_canonical_nonnull_contract(self):
        contract = self.program("out = PyValue_FromInlineInt(1); PyValue_CLOSE(value);",
                                scratch_result=True)
        self.assertIsNotNone(contract.normal_storage)
        results = [a for a in contract.normal_storage.actions
                   if a.kind == "_PyIR_OWNER_STORAGE_RESULT"]
        self.assertEqual([a.flags for a in results], [3])

    def test_a_second_pending_owner_cannot_disappear_on_null_error(self):
        contract = self.program("""
            PyValue held = TAKE(value);
            PyValue first = _PyValue_Invert(held);
            PyValue second = _PyValue_Invert(held);
            PyValue_CLOSE(held);
            ERROR_IF(PyValue_IsNull(second));
            PyValue_XCLOSE(first);
            out = second;
        """, scratch_result=True)
        self.assertFalse(contract.error_checkpoints)

    def test_borrowed_object_projection_cannot_become_an_owned_value(self):
        contract = self.program("""
            out = PyValue_FromObjectSteal(PyValue_AsKnownObjectBorrow(value));
            PyValue_CLOSE(value);
            ERROR_IF(oparg);
        """, scratch_result=True)
        # An input-boxing error before the bad conversion may still have
        # an incoming-frame certificate. No result producer may be granted.
        self.assertFalse(any(a.kind == "_PyIR_OWNER_STORAGE_RESULT"
                             for cp in contract.error_checkpoints for a in cp.actions))
        self.assertIsNone(contract.normal_storage)

    def test_owned_object_conversion_needs_its_actual_nonnull_guard(self):
        for guard in ("", "if (object == NULL) { ERROR_NO_POP(); }"):
            with self.subTest(guard=guard):
                contract = self.program(f"""
                    PyObject *object = _PyEval_GetANext(PyValue_AsKnownObjectBorrow(value));
                    {guard}
                    out = PyValue_FromObjectSteal(object);
                    PyValue_CLOSE(value);
                """, scratch_result=True)
                self.assertEqual(contract.normal_storage is not None, bool(guard))

    def test_object_nonnull_join_intersects_both_actual_predecessors(self):
        guard = "if (object == NULL) { ERROR_NO_POP(); }"
        for other in ("", guard):
            with self.subTest(other=other):
                contract = self.program(f"""
                    PyObject *object = _PyEval_GetANext(PyValue_AsKnownObjectBorrow(value));
                    if (oparg) {{ {guard} }} else {{ {other} }}
                    out = PyValue_FromObjectSteal(object);
                    PyValue_CLOSE(value);
                """, scratch_result=True)
                self.assertEqual(contract.normal_storage is not None, bool(other))

    def test_normal_readability_needs_the_actual_publication_receipt(self):
        original = ir_generator.IRStack.publish_field

        def uncovered(stack, *args, **kw):
            original(stack, *args, **kw)
            stack.home_write_receipts -= 1

        with mock.patch.object(ir_generator.IRStack, "publish_field", uncovered):
            self.assertIsNone(self.program(
                "out = PyValue_FromInlineInt(1); PyValue_CLOSE(value);",
                scratch_result=True).normal_storage)

    def test_null_guard_follows_the_current_alias_not_an_old_name(self):
        for replacement in ("", "alias = PyValue_None;"):
            with self.subTest(replacement=replacement):
                contract = self.program(f"""
                    PyValue result = _PyValue_Invert(value);
                    PyValue alias = result;
                    PyValue_CLOSE(value);
                    {replacement}
                    ERROR_IF(PyValue_IsNull(alias));
                    out = result;
                """, scratch_result=True)
                self.assertEqual(bool(contract.error_checkpoints), not replacement)

    def test_one_uncovered_sibling_revokes_normal_family_readability(self):
        for extra in ("", "opaque(oparg);"):
            with self.subTest(extra=extra):
                contract = test_ir_scalar_storage.ScalarStorageTests().program(f"""
                    family(UNARY_INVERT, 0) = {{ UNARY_NOT }};
                    inst(UNARY_INVERT, (value -- out)) {{
                        out = PyValue_FromInlineInt(1); PyValue_CLOSE(value);
                    }}
                    inst(UNARY_NOT, (value -- out)) {{
                        out = PyValue_FromInlineInt(1); PyValue_CLOSE(value); {extra}
                    }}
                """, name="UNARY_INVERT")
                self.assertEqual(contract.normal_storage is not None, not extra)

    def test_stack_only_format_has_no_emitted_family_certificate(self):
        source = """
            inst(SAMPLE, (value -- out)) {
                out = PyValue_FromInlineInt(1); PyValue_CLOSE(value);
            }
            pure inst(PUSH_NULL, (-- out)) { out = PyValue_NULL; }
        """
        helper = test_ir_scalar_storage.ScalarStorageTests()
        self.assertIsNotNone(helper.program(source, scratch_result=True).normal_storage)
        unexecuted = helper.program(source, name="PUSH_NULL")
        self.assertIsNone(unexecuted.normal_storage)
        self.assertFalse(unexecuted.error_checkpoints)
        self.assertFalse(unexecuted.ownership_events)


if __name__ == "__main__":
    unittest.main()
