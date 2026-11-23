"""Declared operation contracts (Tools/cases_generator/ir_contract.py).

A uop of Python/bytecodes.c may declare its contract: modes and facts on its
stack items, `effects(...)` and `exits(...)`.  Its record then comes from
the declarations, and the generator verifies its body against them and
against the rows of Include/internal/pycore_ir_components.h.  The pilot
declares BINARY_OP's add arms and generic form and UNPACK_SEQUENCE."""

import os
from pathlib import Path
import re
import tempfile
import textwrap
import unittest

from test import test_generated_cases as cases
from test import test_tools

ir_generator = cases.ir_generator
with test_tools.imports_under_tool("cases_generator"):
    import ir_contract
    from ir_records import ALLOCATES, EFFECTS, MAY_RAISE, MAY_RELEASE, READS_HEAP, WRITES_HEAP

BYTECODES = Path(test_tools.basepath) / "Python" / "bytecodes.c"

SLICE = ("_GUARD_TOS_INT", "_GUARD_NOS_INT", "_BINARY_OP_ADD_INT", "_POP_TOP_INT",
         "_BINARY_OP_ADD_FLOAT", "_POP_TOP_NUMBER", "_SPECIALIZE_BINARY_OP", "_BINARY_OP",
         "_GUARD_TOS_TUPLE", "_GUARD_TOS_LIST", "_UNPACK_SEQUENCE_TUPLE",
         "_UNPACK_SEQUENCE_LIST", "_SPECIALIZE_UNPACK_SEQUENCE", "_UNPACK_SEQUENCE")


def setUpModule():
    global analysis, records
    with cases.handle_stderr():
        analysis = cases.analyzer.analyze_files([str(BYTECODES)])
    records = ir_generator.build_records(analysis, ir_generator.compute_formats(analysis))


def uop_record(handler, name):
    return next(u for u in records[handler].uops if u.name == name)


class SliceTests(unittest.TestCase):
    "The pilot slice: records from declarations, equal to what was inferred"

    def test_slice_declares_contracts(self):
        for name in SLICE:
            with self.subTest(uop=name):
                self.assertIsNotNone(ir_contract.contract_of(analysis.uops[name]))
        for name in ("BINARY_OP", "BINARY_OP_ADD_INT", "BINARY_OP_ADD_FLOAT",
                     "UNPACK_SEQUENCE", "UNPACK_SEQUENCE_TUPLE", "UNPACK_SEQUENCE_LIST"):
            with self.subTest(macro=name):
                self.assertIsNotNone(analysis.instructions[name].region)

    def test_effects_come_from_declarations(self):
        expected = {
            ("BINARY_OP_ADD_INT", "_GUARD_TOS_INT"): set(),
            ("BINARY_OP_ADD_INT", "_BINARY_OP_ADD_INT"): {ALLOCATES, MAY_RAISE},
            ("BINARY_OP_ADD_INT", "_POP_TOP_INT"): {WRITES_HEAP},
            ("BINARY_OP_ADD_FLOAT", "_POP_TOP_NUMBER"): set(),
            ("UNPACK_SEQUENCE_LIST", "_GUARD_TOS_LIST"): {READS_HEAP},
            ("UNPACK_SEQUENCE_LIST", "_UNPACK_SEQUENCE_LIST"): {READS_HEAP, MAY_RELEASE},
            ("UNPACK_SEQUENCE", "_SPECIALIZE_UNPACK_SEQUENCE"): {READS_HEAP, WRITES_HEAP},
            ("UNPACK_SEQUENCE", "_UNPACK_SEQUENCE"): set(EFFECTS),
        }
        for (handler, uop), effects in expected.items():
            with self.subTest(handler=handler, uop=uop):
                self.assertEqual(uop_record(handler, uop).effects, effects)

    def test_guard_and_result_facts(self):
        def facts(guard):
            return [str(f) for f in guard.facts]
        add_int = uop_record("BINARY_OP_ADD_INT", "_GUARD_TOS_INT")
        self.assertEqual([facts(g) for g in add_int.guards], [["INLINE_INT(v1)"]])
        add_float = uop_record("BINARY_OP_ADD_FLOAT", "_BINARY_OP_ADD_FLOAT")
        self.assertEqual([facts(g) for g in add_float.guards],
                         [["NUMBER(v0)", "NUMBER(v1)"]])
        self.assertEqual([str(f) for f in add_float.results], ["EXACT_TYPE(v2, PyFloat_Type)"])
        result = uop_record("BINARY_OP_ADD_INT", "_BINARY_OP_ADD_INT").results
        self.assertEqual([str(f) for f in result], ["EXACT_TYPE(v2, PyLong_Type)"])
        # The implicit pointer-space guard, then the declared type test
        guard_list = uop_record("UNPACK_SEQUENCE_LIST", "_GUARD_TOS_LIST")
        self.assertEqual([facts(g) for g in guard_list.guards],
                         [["OBJECT(v0)"], ["EXACT_TYPE(v0, PyList_Type)"]])
        unpack = uop_record("UNPACK_SEQUENCE_LIST", "_UNPACK_SEQUENCE_LIST")
        self.assertEqual([(facts(g), g.reason) for g in unpack.guards],
                         [([], "a free-threaded build's critical section"),
                          (["LENGTH(v0, oparg)"], "")])

    def test_component_checks_are_generated(self):
        checks = "\n".join(ir_contract.component_checks())
        table = ir_contract.rows()
        for row in table.values():
            if row.kind in ("HELPER", "PRIMITIVE", "DESTRUCTOR"):
                self.assertIn(f'"{row.name}: its C type differs', checks)
        self.assertIn("__typeof__(&_PyValue_BoxInt64), PyValue (*)(int64_t)", checks)
        self.assertIn("__typeof__(&_PyLong_ExactDealloc), void (*)(PyObject *)", checks)
        generated = (BYTECODES.parent / "generated_ir_cases.c.h").read_text()
        self.assertIn(checks, generated)


EXTRA_ROWS = """
#define TEST_ROWS(PRIMITIVE, HELPER, MACRO) \\
    MACRO(SPAM_TEST, ARGS(borrowed), TEST(inline_int), EFFECTS(writes), EXITS()) \\
    HELPER(spam_dealloc, void, (PyObject *), ARGS(object), NONE, EFFECTS())
"""


class VerifierTests(unittest.TestCase):
    "Each rule refuses what it must, and accepts the forms the slice uses"

    def setUp(self):
        fd, self.filename = tempfile.mkstemp(suffix=".c")
        os.close(fd)
        self.addCleanup(os.remove, self.filename)
        self.table = ir_contract.rows() | ir_contract.parse_rows(EXTRA_ROWS)

    def verify(self, source):
        with open(self.filename, "w") as f:
            f.write(cases.parser.BEGIN_MARKER)
            f.write(textwrap.dedent(source))
            f.write(cases.parser.END_MARKER)
        with cases.handle_stderr():
            result = cases.analyzer.analyze_files([self.filename], cases.SequentialNumbers())
        for uop in result.uops.values():
            ir_contract.verify_uop(uop, 0, self.table)
        # The handler rules for macros; an inst's inputs come from outside
        for inst in result.instructions.values():
            if len([p for p in inst.parts if isinstance(p, cases.analyzer.Uop)]) > 1:
                ir_contract.verify_handler(inst, self.table)
        return result

    def refuses(self, source, message):
        with self.assertRaisesRegex(SyntaxError, message):
            self.verify(source)

    # The notation

    def test_contract_needs_its_clauses(self):
        self.refuses("""
            inst(OP, (value: consumed --)) {
                DECREF_INPUTS();
            }
        """, r"OP: `value: consumed` declares a contract, but the uop has no effects\(\)")

    def test_words_are_the_vocabulary(self):
        self.refuses("""
            inst(OP, (value: consumed shiny --)) effects(releases) exits(next) {
                DECREF_INPUTS();
            }
        """, r"`shiny` is not a fact of the contract vocabulary")
        self.refuses("""
            inst(OP, (value: consumed --)) effects(flies) exits(next) {
                DECREF_INPUTS();
            }
        """, r"OP: `flies` is not an effect")

    # V1, V2

    def test_undeclared_callee(self):
        self.refuses("""
            inst(OP, (a: consumed -- res: owned)) effects(runs_python) exits(next, raise) {
                res = PyNumber_Add(a, a);
                DECREF_INPUTS();
                ERROR_IF(PyValue_IsNull(res));
            }
        """, r"OP: calls PyNumber_Add, which pycore_ir_components.h does not declare")

    def test_undeclared_effect(self):
        self.refuses("""
            inst(OP, (value: consumed exact(PyLong_Type) --)) effects() exits(next) {
                PyValue_CLOSE_SPECIALIZED(value, _PyLong_ExactDealloc);
            }
        """, r"OP: `PyValue_CLOSE_SPECIALIZED` may writes .*which effects\(\) does not declare")

    def test_only_destructor_rows_are_exempt_from_releases(self):
        source = """
            inst(OP, (value: consumed exact(PyList_Type) --)) effects(writes) exits(next) {
                PyValue_CLOSE_SPECIALIZED(value, %s);
            }
        """
        self.verify(source % "_PyLong_ExactDealloc")
        # An exact type's deallocator is no exemption: a list's releases its items
        self.refuses(source % "spam_dealloc",
                     r"may releases \(spam_dealloc is no callback_free DESTRUCTOR row\)")

    def test_pure_declares_no_change(self):
        self.refuses("""
            pure inst(OP, (value: consumed exact(PyLong_Type) --)) effects(writes) exits(next) {
                PyValue_CLOSE_SPECIALIZED(value, _PyLong_ExactDealloc);
            }
        """, r"OP is annotated pure but declares writes")

    # V3, V6: guards read only, and the word stays restartable

    def test_impure_guard(self):
        self.refuses("""
            inst(OP, (value -- value: inline_int)) effects(writes) exits(next, miss) {
                EXIT_IF(!SPAM_TEST(value));
            }
        """, r"the guard calls SPAM_TEST, which may writes")

    def test_guard_after_a_new_owner(self):
        self.refuses("""
            inst(OP, (value -- value, res: owned)) effects(allocates, raises)
                exits(next, miss, raise)
            {
                res = _PyValue_BoxInt64(1);
                ERROR_IF(PyValue_IsNull(res));
                EXIT_IF(!PyValue_IsInlineInt(value), "a test");
            }
        """, r"the miss follows `_PyValue_BoxInt64`, which may allocates, so the "
             r"word cannot restart")

    def test_guard_after_a_change_in_an_earlier_uop(self):
        source = """
            leaf op(_GUARD, (value -- value: inline_int)) effects() exits(next, miss) {
                EXIT_IF(!PyValue_IsInlineInt(value));
            }
            op(_POP, (value: consumed exact(PyLong_Type) --)) effects(writes) exits(next) {
                PyValue_CLOSE_SPECIALIZED(value, _PyLong_ExactDealloc);
            }
            macro(OP) = _GUARD + %s;
        """
        self.verify(source % "_GUARD + _POP")
        self.refuses(source % "_POP + _GUARD",
                     r"OP: the guard in _GUARD follows _POP, which may writes, so the "
                     r"word cannot restart")

    # V4: ownership on every exit

    def test_consumed_input_must_be_released(self):
        self.refuses("""
            inst(OP, (value: consumed --)) effects() exits(next) {
            }
        """, r"on next, `value` is still owned, but it is declared consumed")

    def test_borrowed_input_must_not_be_released(self):
        self.refuses("""
            inst(OP, (value: borrowed --)) effects(releases) exits(next) {
                DECREF_INPUTS();
            }
        """, r"DECREF_INPUTS releases `value`, declared borrowed")

    def test_dead_drops_only_inline_values(self):
        self.refuses("""
            inst(OP, (value: consumed --)) effects() exits(next) {
                DEAD(value);
            }
        """, r"DEAD\(`value`\) drops an owned value")
        self.verify("""
            inst(OP, (value: consumed inline --)) effects() exits(next) {
                DEAD(value);
            }
        """)

    def test_new_owner_must_be_discharged(self):
        self.refuses("""
            inst(OP, ( -- )) effects(allocates, raises) exits(next, raise) {
                PyValue t = _PyValue_BoxInt64(1);
                ERROR_IF(PyValue_IsNull(t));
            }
        """, r"on next, the new owner `t` is still held")

    def test_raise_checkpoint_per_site(self):
        source = """
            inst(OP, (value: consumed -- res: owned)) effects(allocates, raises, releases)
                exits(next, raise)
            {
                res = _PyValue_BoxInt64(1);
                ERROR_IF(PyValue_IsNull(res)%s);
                DECREF_INPUTS();
            }
        """
        self.refuses(source % "", r"on raise, `value` is still owned, but the site "
                                  r"declares consumed")
        self.verify(source % ", holding(value)")

    def test_taken_input_is_kept_on_raise(self):
        self.refuses("""
            inst(OP, (value: taken -- res: owned)) effects(allocates, raises, releases)
                exits(next, raise)
            {
                DECREF_INPUTS();
                res = _PyValue_BoxInt64(1);
                ERROR_IF(PyValue_IsNull(res));
            }
        """, r"on raise, `value` is released, but it is declared taken")

    # V5: result facts, and their join

    def test_boxed_result_join(self):
        source = """
            inst(OP, (left: taken inline_int, right: taken inline_int
                      -- res: owned %s, l: moved(left), r: moved(right)))
                effects(allocates, raises) exits(next, raise)
            {
                int64_t sum;
                if (__builtin_add_overflow(_PyValue_IntScaled(left), _PyValue_IntScaled(right), &sum)) {
                    res = _PyValue_BoxInt64(PyValue_InlineInt64(left) + PyValue_InlineInt64(right));
                }
                else {
                    res = _PyValue_FromIntScaled(sum);
                }
                if (PyValue_IsNull(res)) {
                    ERROR_NO_POP();
                }
                l = left;
                r = right;
                INPUTS_DEAD();
            }
        """
        self.verify(source % "exact(PyLong_Type)")
        self.refuses(source % "inline_int",
                     r"`res` is declared inline, inline_int, but _PyValue_BoxInt64 gives")

    def test_value_or_null_needs_a_raise(self):
        self.refuses("""
            inst(OP, ( -- res: owned)) effects(allocates, raises) exits(next) {
                res = _PyValue_BoxInt64(1);
            }
        """, r"`res` may be NULL from _PyValue_BoxInt64, and nothing raises on it")

    # V7: exits

    def test_exits_must_match(self):
        self.refuses("""
            inst(OP, (value -- value: inline_int)) effects() exits(next) {
                EXIT_IF(!PyValue_IsInlineInt(value));
            }
        """, r"declares exits\(next\), but the body takes miss, next")

    # V8: requirements, heap-fact lifetime

    STALE = """
        op(_GUARD_LENGTH, (seq: object -- seq: exact(PyTuple_Type) length(oparg)))
            effects(reads) exits(next, miss)
        {
            PyObject *o = PyValue_AsKnownObjectBorrow(seq);
            EXIT_IF(!PyTuple_CheckExact(o) || PyTuple_GET_SIZE(o) != oparg);
        }
        op(_CALLBACK, (seq -- seq)) effects(runs_python) exits(next) {
        }
        op(_USE_LENGTH, (seq: consumed exact(PyTuple_Type) length(oparg) --))
            effects(releases) exits(next)
        {
            DECREF_INPUTS();
        }
        macro(OP) = _GUARD_LENGTH + %s_USE_LENGTH;
    """

    def test_requirement_established_by_a_guard(self):
        self.verify(self.STALE % "")

    def test_stale_heap_fact(self):
        self.refuses(self.STALE % "_CALLBACK + ",
                     r"OP: _USE_LENGTH requires length\(oparg\) of `seq`, which nothing "
                     r"before it establishes or which an effect since killed")

    def test_requirement_of_a_call(self):
        self.refuses("""
            inst(OP, (value -- value, res: owned)) effects(allocates, raises) exits(next, raise) {
                res = _PyValue_BoxInt64(_PyValue_IntScaled(value));
                ERROR_IF(PyValue_IsNull(res));
            }
        """, r"passes `value` to _PyValue_IntScaled, which requires .*inline_int of it")

    # V9: guard facts and their joins

    def test_guard_join(self):
        # A guard that passes when either test holds establishes neither
        source = """
            inst(OP, (a, b -- a: inline_int, b: inline_int)) effects() exits(next, miss) {
                EXIT_IF(!PyValue_IsInlineInt(a) %s !PyValue_IsInlineInt(b));
            }
        """
        self.verify(source % "||")
        self.refuses(source % "&&", r"`a` is declared .*inline_int.*, which nothing establishes")

    def test_guard_fact_must_be_declared(self):
        self.refuses("""
            inst(OP, (value -- value)) effects() exits(next, miss) {
                EXIT_IF(!PyValue_IsInlineInt(value));
            }
        """, r"the guards establish .*inline_int.* of `value`, which `value` does not declare")

    def test_in_body_guard_is_a_refinement_site(self):
        source = """
            inst(OP, (left: taken, right: taken
                      -- res: owned exact(PyFloat_Type), l: moved(left)%s, r: moved(right) inline))
                effects(allocates, raises) exits(next, miss, raise)
            {
                double a, b;
                EXIT_IF(!PyValue_FloatOperandValues(left, right, &a, &b));
                res = PyValue_FromDouble(a + b);
                if (PyValue_IsNull(res)) {
                    ERROR_NO_POP();
                }
                l = left;
                r = right;
                INPUTS_DEAD();
            }
        """
        self.verify(source % " inline")
        self.refuses(source % "",
                     r"the guards establish inline.* of `left`, which `l` does not declare")

    def test_guard_without_facts_needs_a_reason(self):
        self.refuses("""
            inst(OP, (value -- value)) effects(reads) exits(next, miss) {
                EXIT_IF(value.bits == 0);
            }
        """, r"is no combination of TEST rows, MEASURE comparisons and CONTEXT rows")

    # V10, V11: regions and rows

    def test_region_inline_needs_leaf_uops(self):
        result = self.verify("""
            op(_GUARD, (value -- value: inline_int)) effects() exits(next, miss) {
                EXIT_IF(!PyValue_IsInlineInt(value));
            }
            op(_POP, (value: consumed inline --)) effects() exits(next) {
                DEAD(value);
            }
            macro(OP) region(inline) = _GUARD + _POP;
        """)
        inst = result.instructions["OP"]
        ir_contract.verify_handler(inst, self.table, leaf={"_GUARD": True, "_POP": True})
        with self.assertRaisesRegex(SyntaxError, r"region\(inline\), but _POP is not leaf"):
            ir_contract.verify_handler(inst, self.table, leaf={"_GUARD": True, "_POP": False})

    def test_rows_are_checked(self):
        with self.assertRaisesRegex(SyntaxError, r"row spam: 1 ARGS for 2 parameters"):
            ir_contract.parse_rows(
                "#define X(PRIMITIVE, HELPER, MACRO) "
                "HELPER(spam, int, (int, int), ARGS(scalar), SCALAR, EFFECTS())")
        with self.assertRaisesRegex(SyntaxError, r"row spam: EFFECTS: \['flies'\]"):
            ir_contract.parse_rows(
                "#define X(PRIMITIVE, HELPER, MACRO) "
                "HELPER(spam, int, (int), ARGS(scalar), SCALAR, EFFECTS(flies))")


if __name__ == "__main__":
    unittest.main()
