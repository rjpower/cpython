"""Declared operation contracts (Tools/cases_generator/ir_contract.py).

A uop of Python/bytecodes.c may declare its contract: modes and facts on its
stack items, `effects(...)` and `exits(...)`.  Its record then comes from
the declarations, and the generator verifies its body against them and
against the rows of Include/internal/pycore_ir_components.h.  The pilot
declares BINARY_OP's add arms and generic form, UNPACK_SEQUENCE and
LOAD_ATTR_INSTANCE_VALUE."""

import io
import os
from pathlib import Path
import re
import tempfile
import textwrap
import unittest

from test.support import import_helper

from test import test_generated_cases as cases
from test import test_tools

ir_generator = cases.ir_generator
with test_tools.imports_under_tool("cases_generator"):
    import ir_contract
    import ir_contract_diff
    import ir_error_effects
    import ir_leaf
    import ir_records
    from ir_records import ALLOCATES, EFFECTS, MAY_RAISE, MAY_RELEASE, READS_HEAP, WRITES_HEAP

BYTECODES = Path(test_tools.basepath) / "Python" / "bytecodes.c"

SLICE = ("_GUARD_TOS_INT", "_GUARD_NOS_INT", "_BINARY_OP_ADD_INT", "_POP_TOP_INT",
         "_BINARY_OP_ADD_FLOAT", "_POP_TOP_NUMBER", "_SPECIALIZE_BINARY_OP", "_BINARY_OP",
         "_GUARD_TOS_TUPLE", "_GUARD_TOS_LIST", "_UNPACK_SEQUENCE_TUPLE",
         "_UNPACK_SEQUENCE_LIST", "_SPECIALIZE_UNPACK_SEQUENCE", "_UNPACK_SEQUENCE",
         "_GUARD_TYPE_VERSION", "_CHECK_MANAGED_OBJECT_HAS_VALUES",
         "_LOAD_ATTR_INSTANCE_VALUE", "_NULL_SLOT_CONDITIONAL")


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
                     "UNPACK_SEQUENCE", "UNPACK_SEQUENCE_TUPLE", "UNPACK_SEQUENCE_LIST",
                     "LOAD_ATTR_INSTANCE_VALUE"):
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
            # Not reviewed: today's inference (G0c reviews the specializers)
            ("UNPACK_SEQUENCE", "_SPECIALIZE_UNPACK_SEQUENCE"): set(EFFECTS),
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
        # Facts read through a path from the operand: its type's version, its
        # inline values' flag; and the loaded value's, recorded on the local
        handler = "LOAD_ATTR_INSTANCE_VALUE"
        self.assertEqual([facts(g) for g in uop_record(handler, "_GUARD_TYPE_VERSION").guards],
                         [["OBJECT(v0)"], ["TYPE_VERSION(v0, type_version)"]])
        has_values = uop_record(handler, "_CHECK_MANAGED_OBJECT_HAS_VALUES")
        self.assertEqual([facts(g) for g in has_values.guards], [["INLINE_VALUES_VALID(v0)"]])
        load = uop_record(handler, "_LOAD_ATTR_INSTANCE_VALUE")
        self.assertEqual([facts(g) for g in load.guards], [["NOT_NULL(found)"]])
        self.assertEqual(load.effects, {READS_HEAP, MAY_RELEASE})

    def test_transaction_macros_have_rows(self):
        # Every IR_* macro a body calls is a MACRO row whose exits cover the
        # transfers and guard exits that today's tables give it
        body = BYTECODES.read_text()
        body = body[body.index("BEGIN BYTECODES"):body.index("END BYTECODES")]
        called = set(re.findall(r"\b(IR_[A-Z0-9_]+)\s*\(", body))
        table = ir_contract.rows()
        exits = {"NEXT": "next", "BRANCH": "branch", "JUMP": "jump", "RAISE": "raise",
                 "ENTER_FRAME": "enter", "RETURN": "return", "YIELD": "yield",
                 "EXIT": "miss", "DEOPT": "deopt"}
        for name in sorted(called):
            with self.subTest(macro=name):
                self.assertEqual(table[name].kind, "MACRO")
                inferred = {exits[t] for t in ir_records.TRANSFER_TOKENS.get(name, ())}
                if name in ir_records.GUARD_TOKENS:
                    inferred.add(exits[ir_records.GUARD_TOKENS[name]])
                self.assertLessEqual(inferred - {"next"}, set(table[name].exits))

    # Batch 6: PyFunction_Check compares the exact type (funcobject.h);
    # _INIT_CALL_PY_EXACT_ARGS pushes a frame, which writes the heap and
    # releases the callable; SET_FUNCTION_ATTRIBUTE runs no code; the call
    # uops complete in place (exits next, G0d removes the inference)
    LISTED_DIFFERENCES_B6 = [
        'CALL_BOUND_METHOD_EXACT_ARGS.record.uops[3].guards[1].facts[0].kind: inferred "SUBTYPE", declared "EXACT_TYPE"',
        'CALL_BOUND_METHOD_EXACT_ARGS.record.uops[7].effects: inferred 1 entries ["ALLOCATES"], declared 4 ["ALLOCATES", "MAY_RELEASE", "READS_HEAP", "WRITES_HEAP"]',
        'CALL_EX_PY.record.uops[1].guards[1].facts[0].kind: inferred "SUBTYPE", declared "EXACT_TYPE"',
        'CALL_KW_PY.record.uops[1].guards[1].facts[0].kind: inferred "SUBTYPE", declared "EXACT_TYPE"',
        'CALL_PY_EXACT_ARGS.record.uops[1].guards[1].facts[0].kind: inferred "SUBTYPE", declared "EXACT_TYPE"',
        'CALL_PY_EXACT_ARGS.record.uops[5].effects: inferred 1 entries ["ALLOCATES"], declared 4 ["ALLOCATES", "MAY_RELEASE", "READS_HEAP", "WRITES_HEAP"]',
        'CALL_PY_GENERAL.record.uops[1].guards[1].facts[0].kind: inferred "SUBTYPE", declared "EXACT_TYPE"',
        'CALL_PY_GENERATOR.record.uops[1].guards[1].facts[0].kind: inferred "SUBTYPE", declared "EXACT_TYPE"',
        'SET_FUNCTION_ATTRIBUTE.record.format.runs_code: inferred true, declared false',
        'SET_FUNCTION_ATTRIBUTE.record.uops[0].effects: inferred 6 entries ["ALLOCATES", "MAY_RAISE", "MAY_RELEASE", "MAY_RUN_PYTHON", "READS_HEAP", "WRITES_HEAP"], declared 2 ["READS_HEAP", "WRITES_HEAP"]',
        'CALL/_DO_CALL: declares exits(enter, next, raise), the record has enter, raise',
        'CALL_BOUND_METHOD_GENERAL/_PY_FRAME_GENERAL: declares exits(enter, next, raise), the record has enter, raise',
        'CALL_EX_NON_PY_GENERAL/_CALL_FUNCTION_EX_NON_PY_GENERAL: declares exits(enter, next, raise), the record has enter, raise',
        'CALL_EX_PY/_PY_FRAME_EX: declares exits(enter, next, raise), the record has enter, raise',
        'CALL_FUNCTION_EX/_DO_CALL_FUNCTION_EX: declares exits(enter, next, raise), the record has enter, raise',
        'CALL_KW/_DO_CALL_KW: declares exits(enter, next, raise), the record has enter, raise',
        'CALL_KW_BOUND_METHOD/_PY_FRAME_KW: declares exits(enter, next, raise), the record has enter, raise',
        'CALL_KW_NON_PY/_CALL_KW_NON_PY: declares exits(enter, next, raise), the record has enter, raise',
        'CALL_KW_PY/_PY_FRAME_KW_GUARDED: declares exits(enter, next, raise), the record has enter, raise',
        'CALL_PY_GENERAL/_PY_FRAME_GENERAL_GUARDED: declares exits(enter, next, raise), the record has enter, raise',
    ]

    def test_declarations_equal_the_inference(self):
        with cases.handle_stderr():
            lines = ir_contract_diff.report(str(BYTECODES))
        self.assertCountEqual(lines[:-1], self.LISTED_DIFFERENCES_B6)
        self.assertRegex(lines[-1], r"^\d+ handlers compared; \d+ of \d+ uops declared$")

    def test_component_checks_are_generated(self):
        checks = "\n".join(ir_contract.component_checks())
        table = ir_contract.rows()
        for row in table.values():
            if row.kind in ("HELPER", "PRIMITIVE", "DESTRUCTOR", "FIELD", "CONSTANT"):
                self.assertIn(f'"{row.name}: its C type differs', checks)
        self.assertIn("__typeof__(&_PyValue_BoxInt64), PyValue (*)(int64_t)", checks)
        self.assertIn("__typeof__(&_PyLong_ExactDealloc), void (*)(PyObject *)", checks)
        self.assertIn("__typeof__(((PyTypeObject *)0)->tp_version_tag), unsigned int", checks)
        # at block scope: an address in a file-scope literal is no constant
        self.assertIn("static inline void _PyIR_CheckConstants(void) {\n"
                      "static_assert(__builtin_types_compatible_p("
                      "__typeof__(PyValue_None), PyValue),", checks)
        generated = (BYTECODES.parent / "generated_ir_cases.c.h").read_text()
        self.assertIn(checks, generated)


EXTRA_ROWS = """
#define TEST_ROWS(PRIMITIVE, HELPER, MACRO) \\
    MACRO(SPAM_TEST, ARGS(borrowed), TEST(inline_int), EFFECTS(writes), EXITS()) \\
    HELPER(spam_dealloc, void, (PyObject *), ARGS(object), NONE, EFFECTS()) \\
    HELPER(spam_write, void, (PyObject *), ARGS(object), NONE, EFFECTS(writes)) \\
    INTRINSIC(SPAM_FT_ONLY, ARGS(scalar), SCALAR, BUILD(free_threaded)) \\
    MACRO(SPAM_RAISE, ARGS(scalar), NONE, EFFECTS(raises), EXITS(raise)) \\
    HELPER(T, PyValue, (PyValue), ARGS(taken), \\
           VALUE_OR_NULL(owned), EFFECTS(allocates, raises)) \\
    MACRO(SPAM_GUARD, ARGS(scalar), NONE, EFFECTS(), EXITS(miss)) \
    MACRO(TAKE, ARGS(consumed), VALUE(moved(0)), EFFECTS(), EXITS()) \
    HELPER(_PyValue_Size, Py_ssize_t, (PyValueBorrow), ARGS(borrowed), SCALAR, \
           EFFECTS(runs_python)) \
    HELPER(PyValue_FromInt64, PyValue, (int64_t), ARGS(scalar), VALUE_OR_NULL(owned), \
           EFFECTS(allocates, raises)) \
    MACRO(SPAM_STATUS, ARGS(), SCALAR, EFFECTS(raises), EXITS()) \
    MACRO(SPAM_NEW, ARGS(), VALUE_OR_NULL(owned), EFFECTS(allocates, raises), EXITS()) \
    MACRO(SPAM_BORROW, ARGS(), VALUE(borrowed), EFFECTS(reads), EXITS()) \
    MACRO(SPAM_IN_BOUNDS, ARGS(borrowed, borrowed), TEST(in_bounds(1) |, |), \
          EFFECTS(reads), EXITS()) \
    MACRO(SPAM_UNIQUE, ARGS(borrowed), TEST(unique), EFFECTS(), EXITS()) \
    MACRO(SPAM_NEEDS_UNIQUE, ARGS(borrowed unique), NONE, EFFECTS(writes), EXITS()) \
    MACRO(SPAM_GEN_OR_CORO, ARGS(borrowed), TEST(exact(PyGen_Type|PyCoro_Type)), \
          EFFECTS(), EXITS()) \
    MACRO(SPAM_NEEDS_INT, ARGS(borrowed inline_int), NONE, EFFECTS(), EXITS()) \
    HELPER(SPAM_STEAL, PyValue, (PyObject *), ARGS(consumed object), VALUE(owned), \
           EFFECTS()) \
    MACRO(SPAM_GUARD_INT, ARGS(borrowed), GUARD(inline_int), EFFECTS(), EXITS(miss)) \
    MACRO(SPAM_GUARD_INDEX, ARGS(borrowed, borrowed, scalar), GUARD(in_bounds(1)), \
          EFFECTS(reads), EXITS(deopt)) \
    MACRO(SPAM_GUARD_TYPE, ARGS(borrowed, scalar), GUARD(exact(1)), EFFECTS(reads), \
          EXITS(miss)) \
    MACRO(SPAM_GUARD_SHAPE, ARGS(object, scalar, scalar), GUARD(type_version(1) shape(2)), \
          EFFECTS(reads), EXITS(miss)) \
    MACRO(SPAM_DESCR, ARGS(scalar), VALUE(borrowed assumed(nonnull) by spam_cache), \
          EFFECTS(reads), EXITS()) \
    MACRO(SPAM_DESCR_OR_NULL, ARGS(scalar), VALUE_OR_NULL(borrowed), EFFECTS(reads), EXITS()) \
    SLOT(hashfunc, Py_hash_t, (PyObject *), ARGS(object), SCALAR, EFFECTS(runs_python)) \
    FIELD(PyTypeObject, tp_hash, hashfunc, ON(type), SLOT(hashfunc), EFFECTS(reads))
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
        # A destructor frees only its type
        self.refuses(source % "_PyLong_ExactDealloc",
                     r"closes `value` through _PyLong_ExactDealloc, which frees only "
                     r"exact\(PyLong_Type\), which it is not shown to be")
        self.verify(source.replace("PyList_Type", "PyLong_Type") % "_PyLong_ExactDealloc")
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
        self.refuses(source % ", value", r"`value` is not holding\(input\)")
        self.refuses(source % ", holding(other)", r"`holding \( other \)` is not "
                                                  r"holding\(input\)")
        self.refuses(source.replace("ERROR_IF(PyValue_IsNull(res)%s);\n",
                                    "").replace("DECREF_INPUTS();",
                                                "DECREF_INPUTS();\n"
                                                "ERROR_IF(PyValue_IsNull(res)%s);")
                     % ", holding(value)",
                     r"on raise, `value` is released, but the site declares holding")
        analysis = self.verify(source % ", holding(value)")
        uop = analysis.uops["OP"]
        # The emitter keeps the held input in its slot (no clear, as at
        # ERROR_NO_POP) and raises at the site
        out = io.StringIO()
        with cases.handle_stderr():
            ir_generator.generate_ir([self.filename], analysis, out, False)
        body = out.getvalue()
        body = body[body.index("TARGET(OP)"):]
        raise_at = body.index("PROCESS_ERROR_AT(RAISE, 0, this_instr)")
        self.assertLess(raise_at, body.index("_PyFrame_WriteHome(frame, &IR_SLOT"))
        with self.assertRaisesRegex(ir_leaf.LeafError, r"holding\(\.\.\.\)"):
            ir_leaf.translate(uop)
        # The error effects record the site's checkpoint
        analysis = self.verify("""
            inst(OP, (value: consumed --)) effects(raises, releases) exits(next, raise) {
                ERROR_IF(oparg, holding(value));
                DECREF_INPUTS();
            }
        """)
        effects = ir_error_effects.analyze_error_effects(analysis.instructions["OP"])
        self.assertEqual([held for _, held in effects.raise_checkpoints],
                         [frozenset({"value"})])

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

    def test_double_consumption(self):
        self.refuses("""
            inst(OP, (value: consumed exact(PyLong_Type) --)) effects(writes) exits(next) {
                PyValue_CLOSE_SPECIALIZED(value, _PyLong_ExactDealloc);
                PyValue_CLOSE_SPECIALIZED(value, _PyLong_ExactDealloc);
            }
        """, r"PyValue_CLOSE_SPECIALIZED consumes `value`, which is already consumed or moved")
        # Moved to an output, then released
        self.refuses("""
            inst(OP, (value: consumed exact(PyLong_Type) -- res: moved(value))) effects(writes)
                exits(next)
            {
                res = value;
                PyValue_CLOSE_SPECIALIZED(value, _PyLong_ExactDealloc);
            }
        """, r"PyValue_CLOSE_SPECIALIZED consumes `value`, which is already consumed or moved")
        # Moved twice
        self.refuses("""
            inst(OP, (value: consumed -- a: moved(value), b: moved(value))) effects()
                exits(next)
            {
                a = value;
                b = value;
            }
        """, r"moves `value`, which is already consumed or moved")

    def test_move_carries_its_obligation(self):
        source = """
            inst(OP, (value: consumed exact(PyLong_Type) --)) effects(writes) exits(next) {
                PyValue tmp = value;
                %s
            }
        """
        self.refuses(source % "", r"on next, the new owner `tmp` is still held")
        self.verify(source % "PyValue_CLOSE_SPECIALIZED(tmp, _PyLong_ExactDealloc);")
        # A stealing view carries the obligation too
        source = """
            inst(OP, (seq: consumed object --)) effects(runs_python) exits(next) {
                PyObject *seq_o = PyValue_AsKnownObjectSteal(seq);
                %s
            }
        """
        self.refuses(source % "", r"on next, the new owner `seq_o` is still held")
        self.verify(source % "int res = _PyEval_UnpackIterableValue(tstate, seq_o, oparg, "
                             "-1, NULL);")

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

    def test_branch_confined_null_test(self):
        source = """
            inst(OP, (flag/1 -- res: owned)) effects(allocates, raises) exits(next, raise) {
                res = _PyValue_BoxInt64(1);
                %s
            }
        """
        self.verify(source % "ERROR_IF(PyValue_IsNull(res));")
        self.refuses(source % "if (flag) { ERROR_IF(PyValue_IsNull(res)); }",
                     r"`res` may be NULL from _PyValue_BoxInt64, and nothing raises on it")

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
        op(_WRITE, (seq -- seq)) effects(writes) exits(next) {
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
        self.refuses(self.STALE % "_WRITE + ",
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

    def test_conditional_guard_exports_no_fact(self):
        source = """
            inst(OP, (flag/1, value -- value: inline_int)) effects() exits(next, miss) {
                %s
            }
        """
        self.verify(source % "EXIT_IF(!PyValue_IsInlineInt(value));")
        self.refuses(source % "if (flag) { EXIT_IF(!PyValue_IsInlineInt(value)); }",
                     r"`value` is declared .*inline_int.*, which nothing establishes on "
                     r"every path to next")

    def test_killed_heap_fact_does_not_return(self):
        source = """
            op(_GUARD_LENGTH, (seq: object -- seq: exact(PyTuple_Type) length(oparg)))
                effects(reads%s) exits(next, miss)
            {
                PyObject *o = PyValue_AsKnownObjectBorrow(seq);
                EXIT_IF(!PyTuple_CheckExact(o) || PyTuple_GET_SIZE(o) != oparg);
                %s
            }
            op(_USE_LENGTH, (seq: consumed exact(PyTuple_Type) length(oparg) --))
                effects(releases) exits(next)
            {
                DECREF_INPUTS();
            }
            macro(OP) = _GUARD_LENGTH + _USE_LENGTH;
        """
        self.verify(source % ("", ""))
        self.refuses(source % (", writes", "spam_write(o);"),
                     r"_GUARD_LENGTH: `seq` is declared length\(oparg\), which nothing "
                     r"establishes on every path to next")

    def test_release_leaves_a_stale_home_fact(self):
        # A finalizer may write a frame's homes (FrameLocalsProxy): a value
        # left in its home after a release has no facts there
        source = """
            inst(OP, (value -- value: inline_int))
                effects(allocates, raises, %s) exits(next, miss, raise)
            {
                EXIT_IF(!PyValue_IsInlineInt(value));
                PyValue t = _PyValue_BoxInt64(1);
                ERROR_IF(PyValue_IsNull(t));
                PyValue_CLOSE_SPECIALIZED(t, %s);
            }
        """
        self.verify(source % ("writes", "_PyLong_ExactDealloc"))
        self.refuses(source % ("releases", "spam_dealloc"),
                     r"`value` is declared .*inline_int.*, but `PyValue_CLOSE_SPECIALIZED` at "
                     r"line \d+, which may releases, may replace the value in its home")
        # Across uops: the release kills the facts of every value in a home
        source = """
            op(_GUARD, (value -- value: inline_int)) effects() exits(next, miss) {
                EXIT_IF(!PyValue_IsInlineInt(value));
            }
            op(_RELEASE, (value -- value)) effects(allocates, raises, %s)
                exits(next, raise)
            {
                PyValue t = _PyValue_BoxInt64(1);
                ERROR_IF(PyValue_IsNull(t));
                PyValue_CLOSE_SPECIALIZED(t, %s);
            }
            op(_USE, (value: consumed inline_int --)) effects() exits(next) {
                DEAD(value);
            }
            macro(OP) = _GUARD + _RELEASE + _USE;
        """
        self.verify(source % ("writes", "_PyLong_ExactDealloc"))
        self.refuses(source % ("releases", "spam_dealloc"),
                     r"OP: _USE requires .*inline_int.* of `value`, which nothing before "
                     r"it establishes or which an effect since killed")

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

    # Facts through paths, and a stale one

    ATTR = """
        op(_GUARD_VERSION, (type_version/2, owner: object -- owner: type_version(type_version)))
            effects(reads) exits(next, miss)
        {
            PyTypeObject *tp = Py_TYPE(PyValue_AsKnownObjectBorrow(owner));
            EXIT_IF(FT_ATOMIC_LOAD_UINT_RELAXED(tp->tp_version_tag) != type_version);
        }
        op(_HAS_VALUES, (owner: object -- owner: has_values)) effects(reads) exits(next, miss) {
            PyObject *owner_o = PyValue_AsKnownObjectBorrow(owner);
            EXIT_IF(!FT_ATOMIC_LOAD_UINT8(_PyObject_InlineValues(owner_o)->valid));
        }
        op(_WRITE, (owner -- owner)) effects(writes) exits(next) {
        }
        op(_LOAD, (offset/1, owner: consumed has_values -- attr: owned))
            effects(reads, releases) exits(next, deopt)
        {
            PyObject *owner_o = PyValue_AsKnownObjectBorrow(owner);
            PyValueBorrow found = PyValue_Borrow(*_PyObject_ValueSlot(owner_o, offset));
            %s
            attr = PyValue_XDUP(found);
            DECREF_INPUTS();
        }
        macro(OP) = _GUARD_VERSION + _HAS_VALUES + %s_LOAD;
    """

    def test_facts_through_paths(self):
        self.verify(self.ATTR % ("DEOPT_IF(PyValue_IsNull(found));", ""))

    def test_heap_fact_dies_at_a_write(self):
        self.refuses(self.ATTR % ("DEOPT_IF(PyValue_IsNull(found));", "_WRITE + "),
                     r"_LOAD requires has_values of `owner`, which nothing before it "
                     r"establishes or which an effect since killed")

    def test_dup_of_a_value_that_may_be_null(self):
        self.refuses(self.ATTR % ("", ""),
                     r"`attr` may be NULL: PyValue_XDUP of `found`")

    def test_undeclared_feedback_access(self):
        source = """
            inst(OP, (unused/1 --)) effects(reads, writes) exits(next) %s {
                _PyFbHeader *feedback = FB_COUNTER();
                if (_PyFb_CounterTriggers(feedback)) {
                    STAT_INC(OP, hit);
                }
                _PyFb_AdvanceCounter(feedback);
            }
        """
        self.refuses(source % "",
                     r"OP: `_PyFb_CounterTriggers` may read the feedback counter, which "
                     r"feedback\(\) does not declare")
        self.refuses(source % "feedback(read(counter))",
                     r"`_PyFb_AdvanceCounter` may write the feedback counter, which "
                     r"feedback\(read\(counter\)\) does not declare")
        self.verify(source % "feedback(read(counter), write(counter))")
        # The whole entry covers its counter
        result = self.verify(source % "feedback(read(entry), write(entry))")
        self.assertEqual(ir_contract.feedback_access(result.uops["OP"]),
                         {("read", "entry"), ("write", "entry")})
        self.refuses(source % "feedback(read(shape))", r"`read\(shape\)` is no feedback access")

    # Output arrays: wholly published on next, never on raise or a guard exit

    UNPACK = """
        inst(OP, (seq: consumed object -- values[oparg]: owned)) effects(runs_python)
            exits(next, raise)
        {
            PyObject *seq_o = PyValue_AsKnownObjectSteal(seq);
            int res = _PyEval_UnpackIterableValue(tstate, seq_o, oparg, -1, values);
            %s
        }
    """

    def test_status_decides_an_out_array(self):
        self.verify(self.UNPACK % "ERROR_IF(res == 0);")
        self.verify(self.UNPACK % "ERROR_IF(!res);")
        self.refuses(self.UNPACK % "",
                     r"on next, the output array `values` is written only if `res` reports "
                     r"success, which nothing tests here")
        self.refuses(self.UNPACK % "ERROR_IF(res != 0);",
                     r"on raise, the output array `values` is written: its values would leak")

    def test_guard_after_a_publication(self):
        source = """
            inst(OP, (seq: consumed exact(PyTuple_Type) -- values[oparg]: owned))
                effects(reads, releases) exits(next, miss)
            {
                PyObject *seq_o = PyValue_AsKnownObjectBorrow(seq);
                %s
                DECREF_INPUTS();
            }
        """
        guard = "EXIT_IF(PyTuple_GET_SIZE(seq_o) != oparg);"
        unpack = "IR_UNPACK_ITEMS(values, _PyTuple_ITEMS(seq_o), oparg);"
        self.verify(source % (guard + unpack))
        self.refuses(source % (unpack + guard),
                     r"the miss follows the publication of `values` by IR_UNPACK_ITEMS, so "
                     r"the word cannot restart")

    def test_partly_written_array(self):
        source = """
            inst(OP, (a: consumed -- values[%s]: owned)) effects() exits(next) {
                values[0] = a;
            }
        """
        self.verify(source % "1")
        self.refuses(source % "oparg",
                     r"on next, the output array `values` is partly written, but it is "
                     r"published whole")

    def test_guard_after_a_skeleton_detachment(self):
        # The skeleton detaches a taken input from its home at the end of the
        # uop that takes it; a guard in a later uop cannot restart the word
        source = """
            leaf op(_GUARD, (value -- value: inline_int)) effects() exits(next, miss) {
                EXIT_IF(!PyValue_IsInlineInt(value));
            }
            op(_KEEP, (value: taken -- kept: moved(value))) effects() exits(next) {
                kept = value;
            }
            macro(OP) = %s;
        """
        self.verify(source % "_GUARD + _KEEP")
        self.refuses(source % "_KEEP + _GUARD",
                     r"OP: the guard in _GUARD follows the skeleton's detachment of "
                     r"`value` after _KEEP, so the word cannot restart")

    def test_array_facts_across_uops(self):
        # An array output is one value on the handler's stack, which a later
        # uop's array input of the same size binds
        source = """
            op(_MAKE, (seq: consumed exact(PyTuple_Type) -- values[oparg]: owned))
                effects(reads, releases) exits(next, miss)
            {
                PyObject *seq_o = PyValue_AsKnownObjectBorrow(seq);
                EXIT_IF(PyTuple_GET_SIZE(seq_o) != oparg);
                IR_UNPACK_ITEMS(values, _PyTuple_ITEMS(seq_o), oparg);
                DECREF_INPUTS();
            }
            op(_USE, (values[oparg]%s -- values[oparg])) effects() exits(next) {
            }
            op(_GUARD_SEQ, (seq: object -- seq: exact(PyTuple_Type)))
                effects(reads) exits(next, miss)
            {
                PyObject *o = PyValue_AsKnownObjectBorrow(seq);
                EXIT_IF(!PyTuple_CheckExact(o));
            }
            macro(OP) = _GUARD_SEQ + _MAKE + _USE;
        """
        self.verify(source % "")
        self.refuses(source % ": inline_int",
                     r"OP: _USE requires .*inline_int.* of `values`, which nothing before "
                     r"it establishes")

    def test_row_exits_are_sites(self):
        # A row's exits leave the word from inside it: its raise needs the
        # obligations met, its miss a restartable word
        source = """
            inst(OP, (value: consumed exact(PyLong_Type) --)) effects(writes, raises)
                exits(next, raise)
            {
                %s
            }
        """
        close = "PyValue_CLOSE_SPECIALIZED(value, _PyLong_ExactDealloc);"
        self.verify(source % (close + "SPAM_RAISE(oparg);"))
        self.refuses(source % ("SPAM_RAISE(oparg);" + close),
                     r"OP: on raise, `value` is still owned, but the site declares consumed")
        self.refuses("""
            inst(OP, (value: consumed exact(PyLong_Type) --)) effects(writes)
                exits(next, miss)
            {
                PyValue_CLOSE_SPECIALIZED(value, _PyLong_ExactDealloc);
                SPAM_GUARD(oparg);
            }
        """, r"OP: the miss follows .*, so the word cannot restart")

    def test_declared_and_inferred_records_differ(self):
        # The review script (outside the generator) reports a declaration
        # wider than today's inference; the tree's declarations equal it
        analysis = self.verify("""
            inst(OP, (value -- value)) effects(reads) exits(next) {
            }
        """)
        with cases.handle_stderr():
            lines = ir_contract_diff.report(self.filename,
                                            numbers=cases.SequentialNumbers())
        self.assertEqual(lines, ['OP.record.uops[0].effects: inferred 0 entries [], declared 1 ["READS_HEAP"]',
                                 "1 handlers compared; 1 of 1 uops declared"])

    # G0a2: Astra's witnesses against 38a1500dbba (astra/g0a-review.md)

    def test_overwritten_obligation(self):
        self.refuses("""
            op(_LEAK, (a: consumed, b: consumed --)) effects(releases) exits(next) {
                PyValue t = a;
                t = b;
                PyValue_CLOSE_SPECIALIZED(t, _Py_Dealloc);
            }
        """, r"_LEAK: `t` is assigned while it holds an owner, which would leak")

    def test_discarded_owned_result(self):
        self.refuses("""
            op(_LEAK_AND_MISS, (v -- v: inline_int)) effects() exits(next, miss) {
                PyValue_XDUP(v);
                EXIT_IF(!PyValue_IsInlineInt(v));
            }
        """, r"_LEAK_AND_MISS: `PyValue_XDUP \( v \)` discards the owned result of "
             r"PyValue_XDUP")

    def test_unmodelled_forms_are_refused(self):
        source = """
            op(_FORM, (a: consumed, b: consumed -- r: owned)) effects(releases) exits(next) {
                %s
            }
        """
        # A conditional's arms are paths: a move on one only is refused at
        # the join (F3)
        self.refuses(source % "PyValue t = oparg ? a : b;",
                     r"`[ab]` is owned on one path and not on another where they join")
        self.refuses(source % "PyValue t = oparg ? a : (b ? a : b);",
                     r"uses `[ab]` in a form the walk does not model")
        self.refuses(source % "goto done;", r"`goto` is a form the walk does not model")
        self.refuses(source % "r = PyValue_XDUP(PyValue_XDUP(a));",
                     r"calls PyValue_XDUP, which makes an owner, in a form the walk does "
                     r"not model")

    def test_taken_argument_is_kept_on_failure(self):
        # T takes x only if it succeeds: on its NULL, x is still the uop's
        self.refuses("""
            op(_BAD, (x: consumed -- r: owned))
                effects(allocates, raises) exits(next, raise) {
                r = T(x);
                ERROR_IF(PyValue_IsNull(r));
            }
        """, r"_BAD: on raise, `x` is still owned, but the site declares consumed")
        # Steal on success: a taken input is kept on raise
        self.verify("""
            op(_GOOD, (x: taken -- r: owned))
                effects(allocates, raises) exits(next, raise) {
                r = T(x);
                ERROR_IF(PyValue_IsNull(r));
            }
        """)
        # A consumed input released where T fails
        self.verify("""
            op(_GOOD, (x: consumed exact(PyLong_Type) -- r: owned))
                effects(allocates, raises, writes) exits(next, raise) {
                r = T(x);
                if (PyValue_IsNull(r)) {
                    PyValue_CLOSE_SPECIALIZED(x, _PyLong_ExactDealloc);
                    ERROR_IF(true);
                }
            }
        """)
        self.refuses("""
            op(_UNTESTED, (x: taken -- r: owned))
                effects(allocates, raises) exits(next) {
                r = T(x);
            }
        """, r"on next, `x` is taken by a row only if `r` reports success, which nothing "
             r"tests here")

    def test_row_guard_after_a_detachment(self):
        self.refuses("""
            op(_MOVE, (x: taken -- y: moved(x))) effects() exits(next) { y = x; }
            op(_GUARD, (y -- y)) effects() exits(next, miss) { SPAM_GUARD(oparg); }
            macro(BAD) = _MOVE + _GUARD;
        """, r"BAD: the guard in _GUARD follows the skeleton's detachment of `x` after "
             r"_MOVE, so the word cannot restart")

    def test_unknown_depth_keeps_restartability(self):
        self.refuses("""
            op(_WRITE, (--)) effects(writes) exits(next) feedback(write(counter)) {
                _PyFb_AdvanceCounter(FB_COUNTER());
            }
            op(_GUARD_ARRAY, (xs[oparg] -- xs[oparg])) effects() exits(next, miss) {
                EXIT_IF(oparg == 0, "test");
            }
            macro(BAD_ARRAY) = _WRITE + _GUARD_ARRAY;
        """, r"BAD_ARRAY: the guard in _GUARD_ARRAY follows _WRITE, which may writes, so "
             r"the word cannot restart")

    def test_element_stores_are_checked(self):
        self.refuses("""
            op(_BAD_ARRAY, (-- values[1]: owned inline_int)) effects() exits(next) {
                values[0] = PyValue_NULL;
            }
        """, r"_BAD_ARRAY: stores `PyValue_NULL` into the owned array `values`")
        source = """
            op(_ARRAY, (a: consumed %s -- values[1]: owned inline_int)) effects() exits(next) {
                values[0] = a;
            }
        """
        self.verify(source % "inline_int")
        self.refuses(source % "", r"stores `a` into `values`, declared .*inline_int, which it "
                                  r"is not shown to be")
        self.refuses("""
            op(_BORROWED, (a -- a, values[1]: owned)) effects() exits(next) {
                values[0] = a;
            }
        """, r"stores `a`, declared a pass-through, into the owned array `values`: no "
             r"owner moves")

    def test_borrowed_outputs_are_checked(self):
        self.refuses("""
            op(_BAD_BORROW, (x: borrowed nullable -- y: borrowed inline_int))
                effects() exits(next) {
                y = x;
            }
        """, r"_BAD_BORROW: `y` may be NULL from x")
        self.refuses("""
            op(_BAD_BORROW, (x: borrowed -- y: borrowed inline_int)) effects() exits(next) {
                y = x;
            }
        """, r"on next, `y` is declared .*inline_int, which does not hold")

    def test_a_rebound_view_certifies_the_value_tested(self):
        source = """
            op(_WRONG_FACT, (a, b -- %s)) effects() exits(next, miss) {
                PyValueBorrow v = PyValue_Borrow(a);
                v = PyValue_Borrow(b);
                EXIT_IF(!PyValue_IsInlineInt(v));
            }
        """
        self.refuses(source % "a: inline_int, b",
                     r"_WRONG_FACT: `a` is declared .*inline_int.*, which nothing "
                     r"establishes")
        self.verify(source % "a, b: inline_int")

    def test_reentry_kills_home_facts_in_the_body(self):
        self.refuses("""
            op(_STALE, (v: inline_int, victim: consumed -- v))
                effects(releases) exits(next) {
                PyValue_CLOSE_SPECIALIZED(victim, _Py_Dealloc);
                int64_t n = _PyValue_IntScaled(v);
            }
        """, r"_STALE: passes `v` to _PyValue_IntScaled, which requires .*inline_int")

    def test_is_object_needs_a_value(self):
        # PyValue_IsObject(NULL) is false: its false branch says inline only
        # of a value that is not NULL.  (Astra's `v: inline` output parses
        # as a mode on a pass-through; `nonnull` is the fact it implied)
        self.refuses("""
            op(_BAD_NULL, (v: nullable -- v: nonnull)) effects() exits(next, miss) {
                EXIT_IF(PyValue_IsObject(v));
            }
        """, r"_BAD_NULL: passes `v` to PyValue_IsObject, which requires nonnull of it")

    def test_null_iff_names_a_value_argument(self):
        for k in ("1", "0"):
            with self.subTest(k=k):
                text = ("#define X(PRIMITIVE, HELPER, MACRO) "
                        "PRIMITIVE(spam, PyValue, (int, PyValueBorrow), "
                        f"ARGS(scalar, borrowed), VALUE(owned null_iff({k})), EFFECTS())")
                if k == "1":
                    ir_contract.parse_rows(text)
                else:
                    with self.assertRaisesRegex(SyntaxError, r"null_iff\(0\): argument 0 "
                                                             r"is no value argument"):
                        ir_contract.parse_rows(text)
        with self.assertRaisesRegex(SyntaxError, r"null_iff\(2\)"):
            ir_contract.parse_rows("#define X(PRIMITIVE, HELPER, MACRO) "
                                   "PRIMITIVE(spam, PyValue, (PyValueBorrow), ARGS(borrowed), "
                                   "VALUE(owned null_iff(2)), EFFECTS())")

    def test_poly_lookup_reads_feedback(self):
        self.assertEqual(ir_contract.rows()["IR_METHOD_POLY_LOOKUP"].feedback,
                         {("read", "entry")})

    def test_feedback_only_through_rows(self):
        self.refuses("""
            op(_READ_COUNTER, (--)) effects(reads) exits(next) {
                _PyFbHeader *f = FB_COUNTER();
                _Py_BackoffCounter c = f->counter;
            }
        """, r"_READ_COUNTER: dereferences the feedback pointer `f`; a body reaches "
             r"feedback only through rows")
        self.refuses("""
            op(_READ_COUNTER, (--)) effects(reads) exits(next) {
                _Py_BackoffCounter c = FB_COUNTER()->counter;
            }
        """, r"dereferences the feedback pointer `FB_COUNTER`")
        self.refuses("""
            op(_READ_COUNTER, (--)) effects(reads) exits(next) {
                _PyFbHeader *f = FB_COUNTER();
                int x = PyValue_IsNull(f);
            }
        """, r"passes a feedback pointer to PyValue_IsNull, which declares no FEEDBACK")
        self.verify("""
            op(_READ_COUNTER, (--)) effects(reads) exits(next) feedback(read(counter)) {
                _PyFbHeader *f = FB_COUNTER();
                int x = _PyFb_CounterTriggers(f);
            }
        """)

    def test_intrinsic_outside_its_build(self):
        self.refuses("""
            inst(OP, (value -- value)) effects() exits(next) {
                int x = SPAM_FT_ONLY(1);
            }
        """, r"OP: calls SPAM_FT_ONLY, an INTRINSIC of the free_threaded build, not of the "
             r"gil build")

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

    def test_modelled_primitive_needs_a_lowering(self):
        # V11 against the tree: every modelled PRIMITIVE row is lowered
        ir_contract.check_primitives(ir_leaf.PRIMITIVES, ir_contract.lowerings())
        lowered = ir_contract.lowerings("LOWERING(PyValue_IsNull)\n"
                                        "CHECK_EXACT(PyList_CheckExact, PyList_Type)\n")
        self.assertEqual(lowered, {"PyValue_IsNull", "PyList_CheckExact"})
        with self.assertRaisesRegex(ValueError, r"PyValue_IsInlineInt: a PRIMITIVE with a "
                                                r"model in ir_leaf.py but no lowering"):
            ir_contract.check_primitives(ir_leaf.PRIMITIVES, lowered)
        # A row without a model needs none: no leaf uop can call it
        self.assertNotIn("PyValue_TYPE", ir_leaf.PRIMITIVES)
        ir_contract.check_primitives(ir_leaf.PRIMITIVES, ir_contract.lowerings(),
                                     {"PyValue_TYPE": self.table["PyValue_TYPE"]})

    def test_rows_are_checked(self):
        with self.assertRaisesRegex(SyntaxError, r"row spam: 1 ARGS for 2 parameters"):
            ir_contract.parse_rows(
                "#define X(PRIMITIVE, HELPER, MACRO) "
                "HELPER(spam, int, (int, int), ARGS(scalar), SCALAR, EFFECTS())")
        with self.assertRaisesRegex(SyntaxError, r"row spam: EFFECTS: \['flies'\]"):
            ir_contract.parse_rows(
                "#define X(PRIMITIVE, HELPER, MACRO) "
                "HELPER(spam, int, (int), ARGS(scalar), SCALAR, EFFECTS(flies))")
        # Nullness follows an argument: VALUE(... null_iff(k)); DUP is gone
        rows = ir_contract.parse_rows(
            "#define X(PRIMITIVE, HELPER, MACRO) "
            "PRIMITIVE(spam, PyValue, (PyValueBorrow), ARGS(borrowed), "
            "VALUE(owned null_iff(0)), EFFECTS())")
        self.assertEqual(rows["spam"].result.null_iff, 0)
        with self.assertRaisesRegex(SyntaxError, r"row spam: 'DUP\(owned\)' is not a result"):
            ir_contract.parse_rows(
                "#define X(PRIMITIVE, HELPER, MACRO) "
                "PRIMITIVE(spam, PyValue, (PyValueBorrow), ARGS(borrowed), DUP(owned), "
                "EFFECTS())")
        # A routine has exits; any row may say what feedback it reads and writes
        rows = ir_contract.parse_rows(
            "#define X(PRIMITIVE, HELPER, ROUTINE, MACRO) "
            "ROUTINE(spam, int, (PyThreadState *), ARGS(tstate), STATUS(NEGATIVE), "
            "EFFECTS(runs_python), EXITS(raise, exit), FEEDBACK(write(handler)))")
        self.assertEqual(rows["spam"].exits, {"raise", "exit"})
        self.assertEqual(rows["spam"].feedback, {("write", "handler")})
        self.assertEqual(rows["spam"].result.failure, "NEGATIVE")
        with self.assertRaisesRegex(SyntaxError, r"row spam: HELPER: 6 columns, then "
                                                 r"optionally each of FEEDBACK\(\.\.\.\), "
                                                 r"SLOW\(\.\.\.\) once; got 'EXITS\(next\)'"):
            ir_contract.parse_rows(
                "#define X(PRIMITIVE, HELPER, MACRO) "
                "HELPER(spam, int, (int), ARGS(scalar), SCALAR, EFFECTS(), EXITS(next))")

    # F1: fact kinds, their classes and lifetimes

    def test_fact_kinds(self):
        for word in ("in_bounds(seq)", "nonnegative", "nonzero", "is(PyValue_None)",
                     "subtype(PyType_Type)", "not_subtype(PyType_Type)",
                     "not_exact(PyList_Type)", "exact(PyGen_Type|PyCoro_Type)",
                     "type_flag(Py_TPFLAGS_HEAPTYPE)", "type_slot(tp_iter,PyObject_SelfIter)",
                     "class_version", "func_version(K)", "argcount", "vectorcall",
                     "call_flags", "ascii", "no_managed_dict", "unique", "null"):
            self.assertEqual(str(ir_contract.parse_fact(word, None)), word)
        for word, message in (("in_bounds", "needs an argument"),
                              ("exact(PyGen_Type|)", "the argument of `exact`"),
                              ("type_slot(tp_iter)", "the argument of `type_slot`"),
                              ("ascii(x)", "takes no argument")):
            with self.assertRaisesRegex(ValueError, message):
                ir_contract.parse_fact(word, None)
        # Mutable state's facts are heap facts; a value's are not
        for kind in ("in_bounds", "subtype", "not_exact", "type_flag", "class_version"):
            self.assertIn(kind, ir_contract.HEAP_FACTS)
        for kind in ("nonnegative", "nonzero", "is", "ascii", "exact", "null"):
            self.assertNotIn(kind, ir_contract.HEAP_FACTS)
        # A union of exact types is a fact of the record as a union
        union = ir_contract.parse_fact("exact(PyGen_Type|PyCoro_Type)", None)
        self.assertEqual(union.record("v0").arg, "PyGen_Type | PyCoro_Type")

    def test_in_bounds_names_an_input_and_dies_with_the_heap(self):
        source = """
            op(_GUARD, (seq, i -- seq, i: in_bounds(%s))) effects(%s) exits(next, miss) {
                EXIT_IF(!SPAM_IN_BOUNDS(i, seq));
                %s
            }
        """
        self.verify(source % ("seq", "reads", ""))
        self.refuses(source % ("other", "reads", ""),
                     r"`i` is declared in_bounds\(other\), but `other` is not an input")
        self.refuses(source % ("seq", "reads, writes", "int z = SPAM_TEST(seq);"),
                     r"`i` is declared .*in_bounds\(seq\).*, which nothing establishes")

    def test_unique_is_a_fact_of_one_site(self):
        source = """
            op(_OP, (x: borrowed --)) effects(writes, runs_python) exits(next) {
                if (SPAM_UNIQUE(x)) {
                    %s
                    SPAM_NEEDS_UNIQUE(x);
                }
            }
        """
        self.verify(source % "")
        # It dies at a reentry within the uop
        self.refuses(source % "Py_ssize_t n = _PyValue_Size(x);",
                     r"passes `x` to SPAM_NEEDS_UNIQUE, which requires .*unique of it")
        # and is never exported
        self.refuses("""
            op(_OP, (x -- x: unique)) effects() exits(next) {
                int z = 0;
            }
        """, r"`x` is declared unique, a fact of one site, which is never exported")

    def test_identities_name_constants(self):
        self.verify("""
            op(_OP, (-- res: inline is(PyValue_None))) effects() exits(next) {
                res = PyValue_None;
            }
        """)
        self.refuses("""
            op(_OP, (-- res: inline is(PyValue_True))) effects() exits(next) {
                res = oparg ? PyValue_True : PyValue_False;
            }
        """, r"`res` is declared is\(PyValue_True\), but PyValue_False gives")
        self.refuses("""
            op(_OP, (x -- x: is(spam))) effects() exits(next) {
                int z = 0;
            }
        """, r"is\(spam\) names `spam`, which is no CONSTANT row")
        self.refuses("""
            op(_OP, (x -- x: is(PyValue_NULL))) effects() exits(next) {
                int z = 0;
            }
        """, r"is\(PyValue_NULL\): NULL is no identity; it is `null`")

    def test_identities_of_static_objects(self):
        # F21, _GUARD_LOAD_SUPER_ATTR_METHOD: a CONSTANT row names a static
        # object by address; a comparison with it establishes is(K)
        row = self.table["PySuper_Type"]
        self.assertEqual((row.kind, row.ret, row.representation),
                         ("CONSTANT", "PyTypeObject", "OBJECT"))
        self.assertIn("__typeof__(&PySuper_Type), PyTypeObject *)",
                      "\n".join(ir_contract.component_checks()))
        source = """
            op(_OP, (x: object -- x: is(PySuper_Type))) effects() exits(next, miss) {
                PyObject *o = PyValue_AsKnownObjectBorrow(x);
                EXIT_IF(%s);
            }
        """
        self.verify(source % "o != (PyObject *)&PySuper_Type")
        self.verify(source % "!(o == (PyObject *)&PySuper_Type)")
        self.refuses(source % "o == (PyObject *)&PySuper_Type",
                     r"`x` is declared is\(PySuper_Type\), which nothing establishes")
        # A value that is a static object still owes its count, unlike
        # PyValue_None's
        self.refuses("""
            op(_OP, (x: consumed object --)) effects() exits(next, miss) {
                PyObject *o = PyValue_AsKnownObjectBorrow(x);
                EXIT_IF(o != (PyObject *)&PySuper_Type);
            }
        """, r"on next, `x` is still owned, but it is declared consumed")
        with self.assertRaisesRegex(SyntaxError, r"row SPAM: OBJECT\(type\)"):
            ir_contract.parse_rows("#define X(PRIMITIVE, HELPER, MACRO) "
                                   "CONSTANT(SPAM, OBJECT(PyTypeObject *))")

    def test_exact_types_join_by_union(self):
        # A union TEST, and the join of two exact types
        source = """
            op(_GUARD, (x -- x: %s)) effects() exits(next, miss) {
                EXIT_IF(!SPAM_GEN_OR_CORO(x));
            }
        """
        self.verify(source % "exact(PyGen_Type|PyCoro_Type)")
        self.refuses(source % "exact(PyGen_Type)", r"`x` is declared exact\(PyGen_Type\), "
                                                   r"which nothing establishes")
        facts = ir_contract._intersection([
            {"x": frozenset({ir_contract.parse_fact("exact(PyList_Type)", None)})},
            {"x": frozenset({ir_contract.parse_fact("exact(PyTuple_Type)", None)})}])
        self.assertIn(ir_contract.parse_fact("exact(PyList_Type|PyTuple_Type)", None),
                      facts["x"])

    # A stolen object pointer

    def test_consumed_object_steals_a_pointer(self):
        steal = self.table["SPAM_STEAL"].args[0]
        self.assertEqual((steal.mode, steal.object, steal.facts), ("consumed", True, frozenset()))
        # On a PyValue parameter `object` stays the value's fact
        view = self.table["PyValue_AsKnownObjectSteal"].args[0]
        self.assertEqual((view.mode, view.object), ("consumed", False))
        self.assertIn(ir_contract.OBJECT, view.facts)
        source = """
            op(_OP, (x: %s object -- r: owned)) effects() exits(next) {
                PyObject *o = %s(x);
                r = SPAM_STEAL(o);
            }
        """
        self.verify(source % ("consumed", "PyValue_AsKnownObjectSteal"))
        self.refuses(source % ("borrowed", "PyValue_AsKnownObjectBorrow"),
                     r"SPAM_STEAL consumes `x`, declared borrowed, which it does not own")

    # F7: assumed(fact) by <producer>

    def test_assumed_inputs(self):
        # _POP_TOP_UNICODE: the input form; an assert in the body is no
        # declaration
        source = """
            op(_OP, (value: consumed %s --)) effects(writes) exits(next) {
                assert(PyUnicode_CheckExact(PyValue_AsKnownObjectBorrow(value)));
                PyValue_CLOSE_SPECIALIZED(value, _PyUnicode_ExactDealloc);
            }
        """
        analysis = self.verify(source % "assumed(exact(PyUnicode_Type)) by _BINARY_OP_ADD_UNICODE")
        item = ir_contract.contract_of(analysis.uops["_OP"]).inputs[0]
        self.assertEqual(item.assumed, (ir_contract.Assumption(
            ir_contract.VFact("exact", "PyUnicode_Type"), "_BINARY_OP_ADD_UNICODE"),))
        self.assertEqual(item.facts, frozenset())       # no requirement
        self.assertEqual(ir_contract.assumed_assertions(analysis.uops["_OP"], None), [
            "assert(!PyValue_IsNull(value) && "
            "(PyValue_TYPE(value) == &PyUnicode_Type));"])
        self.refuses(source % "", r"frees only exact\(PyUnicode_Type\)")
        self.refuses(source % "assumed(exact(PyUnicode_Type))",
                     r"names no producer: `assumed\(exact\(PyUnicode_Type\)\) by <producer>`")
        self.refuses(source % "assumed(length(n)) by _P", r"length\(n\) has no assertion form")
        self.refuses("""
            op(_OP, (-- r: owned assumed(nonnull) by _P)) effects() exits(next) {
                r = PyValue_None;
            }
        """, r"the output `r` is assumed")

    def test_an_assumption_binds_at_the_handlers_entry(self):
        source = """
            op(_FIRST, (%s)) effects(%s) exits(next) {
                %s
            }
            op(_SECOND, (x: assumed(object) by _P -- x)) effects() exits(next) {
                spam_dealloc(PyValue_AsKnownObjectBorrow(x));
            }
            macro(OP) = _FIRST + _SECOND;
        """
        # A value that reaches the handler
        self.verify(source % ("x -- x", "", ""))
        # Made in this handler: its producer's facts must show it
        self.refuses(source % ("-- x: borrowed", "reads", "x = SPAM_BORROW();"),
                     r"_SECOND assumes object by _P of `x`, a value made in this handler")
        # The assumption does not revive a fact after a reentry
        self.refuses(source % ("x -- x", "runs_python",
                               "Py_ssize_t n = _PyValue_Size(x); (void)n;"),
                     r"`x`, which may have been replaced since the handler's entry, at "
                     r"_FIRST, which may reenter")
        # Within the uop it lives as any fact: a reentry kills it
        self.refuses("""
            op(_OP, (x: assumed(object) by _P -- x)) effects(runs_python) exits(next) {
                Py_ssize_t n = _PyValue_Size(x); (void)n;
                spam_dealloc(PyValue_AsKnownObjectBorrow(x));
            }
        """, r"passes `x` to PyValue_AsKnownObjectBorrow, which requires .*object of it")

    def test_assumed_row_results(self):
        # _LOAD_ATTR_CLASS: the cached descriptor is not NULL by the cache's
        # invariant, declared on the row that reads it
        row = self.table["SPAM_DESCR"]
        self.assertEqual(row.result.assumed, (ir_contract.Assumption(
            ir_contract.NONNULL, "spam_cache"),))
        source = """
            op(_OP, (-- attr: owned)) effects(reads) exits(next) {
                %s
            }
        """
        analysis = self.verify(source % "PyValueBorrow found = SPAM_DESCR(oparg);\n"
                               "attr = PyValue_DUP(found);")
        uop = analysis.uops["_OP"]
        texts = ["PyValueBorrow", "found", "=", "SPAM_DESCR", "(", "oparg", ")", ";"]
        self.assertEqual(ir_contract.assumed_assertions(uop, texts, self.table),
                         ["assert(!PyValue_IsNull(found));"])
        self.assertEqual(ir_contract.assumed_assertions(uop, texts[:3] + ["oparg", ";"],
                                                        self.table), [])
        # The call is bound to a local, where every form asserts it
        self.refuses(source % "attr = PyValue_DUP(SPAM_DESCR(oparg));",
                     r"SPAM_DESCR's result is assumed nonnull by spam_cache; bind it to a local")
        self.refuses(source % "if (SPAM_DESCR(oparg).bits) { attr = PyValue_None; }"
                     " else { attr = PyValue_None; }",
                     r"SPAM_DESCR's result is assumed")
        # An assert in the body is no declaration
        self.refuses(source % "PyValueBorrow found = SPAM_DESCR_OR_NULL(oparg);\n"
                     "assert(!PyValue_IsNull(found));\nattr = PyValue_DUP(found);",
                     r"passes `found` to PyValue_DUP, which requires nonnull of it")
        for text, message in [
                ("VALUE(borrowed assumed(nonnull))", "names no producer"),
                ("VALUE_OR_NULL(borrowed assumed(nonnull) by p)", "qualifies a VALUE result"),
                ("VALUE(borrowed assumed(length(x)) by p)", "has no assertion form")]:
            with self.subTest(text), self.assertRaisesRegex(ValueError, message):
                ir_contract._parse_result(text)

    def test_assumptions_are_asserted_in_every_form(self):
        analysis = self.verify("""
            inst(OP, (value: consumed assumed(exact(PyUnicode_Type)) by _P --))
                effects(writes) exits(next)
            {
                PyValue_CLOSE_SPECIALIZED(value, _PyUnicode_ExactDealloc);
            }
        """)
        out = io.StringIO()
        with cases.handle_stderr():
            ir_generator.generate_ir([self.filename], analysis, out, False)
        body = cases.expand_outlined(out.getvalue())
        body = body[body.index("TARGET(OP)"):]
        self.assertLess(body.index("assert(!PyValue_IsNull(value) && "
                                   "(PyValue_TYPE(value) == &PyUnicode_Type));"),
                        body.index("_PyUnicode_ExactDealloc"))
        # The leaf IR states it as an assume op, which the templates check
        # (test_ir_leaf, test_jit_template)
        analysis = self.verify("""
            inst(OP, (value: borrowed assumed(exact(PyUnicode_Type)) by _P --))
                effects() exits(next)
            {
            }
        """)
        self.assertIn("  assume exact PyUnicode_Type %0",
                      ir_leaf.translate(analysis.uops["OP"]).dump())

    def test_frame_paths(self):
        # GLOBALS() and BUILTINS() are pointers along paths from the frame,
        # fixed for the activation (F1): PATH rows without arguments
        for name, path in (("GLOBALS", "globals"), ("BUILTINS", "builtins")):
            row = self.table[name]
            self.assertEqual((row.args, row.result), ((), ir_contract.Result("PATH", measure=path)))

    # F6: calls through a protocol's pointer

    def test_slot_rows(self):
        row = self.table["hashfunc"]
        self.assertEqual((row.kind, row.ret, row.params, row.effects),
                         ("SLOT", "Py_hash_t", ("PyObject *",), frozenset({"runs_python"})))
        self.assertEqual(self.table["PyTypeObject.tp_hash"].result,
                         ir_contract.Result("SLOT", measure="hashfunc"))
        # The static check holds the protocol's pointer type
        self.assertIn("__typeof__(hashfunc), Py_hash_t (*)(PyObject *)",
                      "\n".join(ir_contract.component_checks(self.table)))
        # A field that holds a protocol's pointer has its C type
        for rows, message in [
                ("FIELD(PyTypeObject, tp_hash, lenfunc, ON(type), SLOT(hashfunc), "
                 "EFFECTS(reads))", r"the field's C type is lenfunc, not the protocol's"),
                ("MACRO(SPAM_H, ARGS(), SLOT(hashfunc), EFFECTS(), EXITS())",
                 r"a FIELD that holds a pointer of a SLOT row's protocol"),
                ("FIELD(PyTypeObject, tp_hash, hashfunc, ON(type), SLOT(nothing), "
                 "EFFECTS(reads))", r"SLOT\(nothing\) names no row")]:
            with self.subTest(rows), self.assertRaisesRegex(SyntaxError, message):
                ir_contract.check_columns(self.table | ir_contract.parse_rows(rows))

    def test_a_local_calls_the_protocol_its_field_holds(self):
        source = """
            op(_OP, (x: object -- x)) effects(%s) exits(next) {
                PyObject *o = PyValue_AsKnownObjectBorrow(x);
                hashfunc h = %s;
                Py_hash_t v = h(o);
                (void)v;
            }
        """
        self.verify(source % ("runs_python", "Py_TYPE(o)->tp_hash"))
        # The call is the protocol's: its effects are the uop's
        self.refuses(source % ("reads", "Py_TYPE(o)->tp_hash"),
                     r"`h` may runs_python \(hashfunc declares it\), which effects\(reads\) "
                     r"does not declare")
        self.refuses(source % ("runs_python", "(hashfunc)Py_TYPE(o)->tp_hash"),
                     r"`h` is called through a cast")
        self.refuses(source % ("runs_python", "Py_TYPE(o)->tp_version_tag"),
                     r"`h` is called, but `Py_TYPE \( o \) -> tp_version_tag` is no FIELD "
                     r"that declares the SLOT it holds")
        self.refuses(source % ("runs_python", "spam_lookup"),
                     r"`spam_lookup` is no FIELD that declares the SLOT it holds")

    # F6: correlated status-and-value results (CASES)

    def test_cases_results(self):
        row = self.table["_PyValue_GetOptionalItem"]
        self.assertEqual(row.result.shape, "CASES")
        error, missing, found = row.result.cases
        self.assertEqual((error.raised, dict(error.values)["value"].mode), (True, "null"))
        self.assertEqual((missing.raised, dict(missing.values)["value"].mode), (False, "null"))
        self.assertEqual(dict(found.values)["value"].mode, "owned")
        for text, message in [
                ("CASES(status<0 value(null) raised)", "two or more outcomes"),
                ("CASES(status<0 value(null), status>0)", "each stating every field"),
                ("CASES(status<5 value(null), status>0 value(owned))", "compares an integer"),
                ("CASES(status<0 value(shiny), status>0 value(owned))", "owned, borrowed"),
                ("CASES(status==-1 value(null), status>0 value(owned))",
                 "an exact status field")]:
            with self.subTest(text), self.assertRaisesRegex(ValueError, message):
                ir_contract._parse_result(text)

    def test_the_walk_follows_each_outcome(self):
        # _LOAD_FROM_DICT_OR_GLOBALS: an error, a missing key, or an owned value
        source = """
            op(_OP, (d: borrowed object -- v: owned)) effects(runs_python) exits(next, raise) {
                PyObject *o = PyValue_AsKnownObjectBorrow(d);
                _PyStatusValue found = value_load_from_dict(tstate, o, o);
                %s
            }
        """
        self.verify(source % """
                ERROR_IF(found.status < 0);
                if (found.status == 0) {
                    v = PyValue_None;
                }
                else {
                    v = found.value;
                }""")
        # Without the error test, the error's NULL reaches the output
        self.refuses(source % """
                if (found.status == 0) {
                    v = PyValue_None;
                }
                else {
                    v = found.value;
                }""", r"`v` may be NULL")
        # The found value is an owner
        self.refuses(source % """
                ERROR_IF(found.status < 0);
                v = PyValue_None;""", r"on next, the new owner `found.value` is still held")
        # A missing key sets no exception
        self.refuses(source % """
                ERROR_IF(found.status <= 0);
                v = found.value;""",
                     r"raises where `found` reports an outcome that sets no exception")
        # V5: the field gives the owned value
        self.refuses(source.replace("v: owned", "v: owned inline_int") % """
                ERROR_IF(found.status <= 0, holding(d));
                v = found.value;""", r"declared .*inline_int")

    def test_copies_of_fields_keep_their_outcome(self):
        # _INSTRUMENTED_FOR_ITER and _SEND: the status and value copied to
        # locals, PySendResult's exact statuses
        source = """
            op(_OP, (it: borrowed object -- v: owned)) effects(runs_python) exits(next, raise) {
                _PyStatusValue sent = _PyValue_Send(PyValue_AsKnownObjectBorrow(it), it);
                PyValue item = sent.value;
                int what = (PySendResult)sent.status;
                if (what == PYGEN_ERROR) {
                    ERROR_NO_POP();
                }
                %s
            }
        """
        self.verify(source % "v = item;")
        self.refuses(source % "if (what == PYGEN_RETURN) { PyValue_CLOSE(item); }\nv = item;",
                     r"moves `item`, which is already consumed or moved")
        self.verify(source % "if (what == PYGEN_RETURN) { PyValue_CLOSE(item); "
                    "v = PyValue_None; }\nelse { v = item; }")

    def test_value_slots_and_index_steps(self):
        # _FOR_ITER_VIRTUAL: an iternext value slot read through
        # _PyType_VALUE_SLOT, and an index step through _tp_iteritem, whose
        # _PyValueIndexPair is a CASES result
        source = """
            op(_OP, (iter: object, null_or_index -- iter, null_or_index, next: owned))
                effects(reads, runs_python) exits(next, miss, raise, branch)
            {
                PyObject *iter_o = PyValue_AsKnownObjectBorrow(iter);
                if (PyValue_IsNull(null_or_index)) {
                    _Py_iternextvaluefunc next_value = _PyType_VALUE_SLOT(Py_TYPE(iter_o), iternext);
                    EXIT_IF(next_value == NULL, "the type has no iternext value slot");
                    _PyStatusValue step = next_value(iter_o);
                    if (step.status != PYGEN_NEXT) {
                        if (step.status == PYGEN_ERROR && !_PyForIter_StopIsExhaustion(tstate, frame)) {
                            ERROR_NO_POP();
                        }
                        PyValue completed = PyValue_IsNull(step.value) ? PyValue_None : step.value;
                        IR_COMPLETE_STEP(_PY_IR_FOR_ITER, completed, oparg);
                    }
                    next = step.value;
                }
                else {
                    Py_ssize_t index = PyValue_UntagIndex(null_or_index);
                    _Py_iteritemfunc iteritem = Py_TYPE(iter_o)->_tp_iteritem;
                    _PyValueIndexPair next_index = iteritem(iter_o, index);
                    index = next_index.index;
                    if (PyValue_IsNull(next_index.value)) {
                        %s
                        PyValue completed = PyValue_None;
                        IR_COMPLETE_STEP(_PY_IR_FOR_ITER, completed, oparg);
                    }
                    null_or_index = PyValue_TagIndex(index);
                    next = next_index.value;
                }
            }
        """
        self.verify(source % "if (index < 0) { ERROR_NO_POP(); }")
        # The end of the sequence sets no exception
        self.refuses(source % "ERROR_NO_POP();",
                     r"raises where `next_index` reports an outcome that sets no exception")
        # The accessor reads the field it names: a contains slot's call
        # gives no CASES result
        self.refuses((source % "if (index < 0) { ERROR_NO_POP(); }").replace(
                         "_PyType_VALUE_SLOT(Py_TYPE(iter_o), iternext)",
                         "_PyType_VALUE_SLOT(Py_TYPE(iter_o), contains)"),
                     r"`step.value` is assigned to an output, but it is no row")
        # The completion value is consumed by IR_COMPLETE_STEP
        self.refuses((source % "if (index < 0) { ERROR_NO_POP(); }").replace(
                         "IR_COMPLETE_STEP(_PY_IR_FOR_ITER, completed, oparg);\n"
                         "                    }\n                    next = step.value;",
                         "IR_COMPLETE_STEP(_PY_IR_FOR_ITER, completed, oparg);\n"
                         "                    }\n                    next = step.value;\n"
                         "                    PyValue_CLOSE(completed);"),
                     r"completed")

    def test_a_raise_on_a_field_follows_its_outcome(self):
        # _GET_ITER: the raise tests the field that the outcome decides
        source = """
            op(_OP, (iterable: consumed -- iter: owned)) effects(runs_python) exits(next, raise) {
                _PyValuePair got = _PyEval_GetIter(iterable, oparg);
                DEAD(iterable);
                ERROR_IF(%s);
                iter = got.first;
            }
        """
        self.verify(source % "PyValue_IsNull(got.first)")
        self.refuses(source % "!PyValue_IsNull(got.first)",
                     r"raises where `got` reports an outcome that sets no exception")

    def test_a_descriptor_field_holds_the_protocol(self):
        # _BINARY_OP_EXTEND: the action of the feedback's descriptor, a FIELD
        # along a path from the cache operand
        source = """
            op(_OP, (descr/4, left: borrowed object, right: borrowed object -- res: owned))
                effects(reads, runs_python) exits(next, raise)
            {
                PyObject *left_o = PyValue_AsKnownObjectBorrow(left);
                PyObject *right_o = PyValue_AsKnownObjectBorrow(right);
                _PyBinaryOpSpecializationDescr *d = %s;
                binaryopactionfunc action = d->action;
                PyObject *res_o = action(left_o, right_o);
                if (res_o == NULL) {
                    ERROR_NO_POP();
                }
                res = PyValue_FromObjectSteal(res_o);
            }
        """
        self.verify(source % "_PyBinaryOp_ExtendDescr(descr)")
        self.refuses(source % "(_PyBinaryOpSpecializationDescr *)descr",
                     r"`action` is called, but `d -> action` is no FIELD that declares "
                     r"the SLOT it holds")

    # A handler's state (call_state) and frame paths (G0a4 item 2)

    def test_state_rows(self):
        row = self.table["call_state"]
        self.assertEqual((row.kind, row.ret, row.init), ("STATE", "int", "IR_CALL_SELF_STATE"))
        self.assertEqual(row.bits, (("CALL_SELF_PRESENT",
                                     frozenset({ir_contract.VFact("self", "present")}),
                                     frozenset({ir_contract.VFact("self", "absent")})),))
        self.assertIn('static_assert((CALL_SELF_PRESENT) != 0, '
                      '"call_state: CALL_SELF_PRESENT is no bit");',
                      ir_contract.component_checks(self.table))
        for bad, message in [
                ("STATE(spam, int, BITS(SPAM: null | self(absent)), INIT(SPAM_INIT))",
                 r"null is no fact of a handler's state"),
                ("STATE(spam, int, BITS(SPAM: self(maybe) | self(absent)), INIT(SPAM_INIT))",
                 r"self\(maybe\) is no fact of a handler's state"),
                ("STATE(spam, int, BITS(SPAM: self(present) | self(absent)))",
                 r"STATE\(name, ctype, BITS")]:
            with self.subTest(bad=bad):
                with self.assertRaisesRegex(SyntaxError, message):
                    ir_contract.parse_rows(bad)

    def test_the_generator_initializes_the_state_as_its_row_declares(self):
        source = (Path(__file__).resolve().parents[2] / "Python"
                  / "generated_ir_cases.c.h").read_text()
        self.assertIn("int call_state = IR_CALL_SELF_STATE(ir_word);", source)

    def test_guards_state_facts_of_call_state(self):
        # _GUARD_NOS_NULL and _GUARD_CALL_SELF_PRESENT
        source = """
            op(_OP, (null, unused -- null, unused)) effects() exits(next, miss)
                %s
            {
                EXIT_IF(%s);
            }
        """
        absent = "state(call_state -- call_state: self(absent))"
        present = "state(call_state -- call_state: self(present))"
        bit = "call_state & CALL_SELF_PRESENT"
        self.verify(source % (absent, bit))
        self.verify(source % (present, f"!({bit})"))
        self.refuses(source % (present, bit),
                     r"`call_state` is declared self\(present\), which nothing establishes")
        self.refuses(source % ("state(call_state -- call_state)", bit),
                     r"the body establishes self\(absent\) of `call_state`, which the uop "
                     r"does not declare")
        # An undeclared read, and an address
        self.refuses(source % ("", bit),
                     r"reads `call_state`, a handler's state, which no state\(...\) clause "
                     r"declares")
        self.refuses(source.replace("EXIT_IF(%s);", "int *p = &call_state;\n(void)p;\nEXIT_IF(%s);")
                     % (absent, bit), r"`&call_state` addresses a handler's state")
        self.refuses(source % ("state(spam -- spam)", bit), r"state\(spam\) names no STATE row")
        self.refuses(source % ("state(call_state -- call_state: exact(PyLong_Type))", bit),
                     r"exact\(PyLong_Type\) is no fact of a handler's state")

    def test_a_declared_write_of_the_state(self):
        # _INIT_CALL_BOUND_METHOD_EXACT_ARGS: self(absent) to self(present)
        source = """
            op(_OP, (x: object -- x: object)) effects(%s) exits(next)
                state(call_state: self(absent) -- call_state: %s)
            {
                assert(!(call_state & CALL_SELF_PRESENT));
                %s
            }
        """
        write = "call_state |= CALL_SELF_PRESENT | CALL_EXPANDED;"
        self.verify(source % ("", "self(present)", write))
        self.refuses(source % ("", "self(absent)", write),
                     r"`call_state` is declared self\(absent\), which nothing establishes")
        # Heap kills leave the state's facts
        self.verify(source % ("writes", "self(absent)",
                              "spam_write(PyValue_AsKnownObjectBorrow(x));"))

    def test_a_handler_requires_its_state_facts(self):
        source = """
            op(_GUARD, (x: object -- x: object)) effects() exits(next, miss)
                state(call_state -- call_state: self(absent))
            {
                EXIT_IF(call_state & CALL_SELF_PRESENT);
            }
            op(_KILL, (x: object -- x: object)) effects(writes) exits(next) {
                spam_write(PyValue_AsKnownObjectBorrow(x));
            }
            op(_NOTHING, (x -- x)) effects() exits(next) {
            }
            op(_INIT, (x -- x)) effects() exits(next)
                state(call_state: self(absent) -- call_state: self(present))
            {
                call_state |= CALL_SELF_PRESENT;
            }
            macro(OP) = %s;
        """
        self.verify(source % "_GUARD + _KILL + _INIT")
        self.refuses(source % "_NOTHING + _INIT",
                     r"OP: _INIT requires self\(absent\) of the handler's `call_state`, which "
                     r"nothing before it establishes")
        self.refuses(source % "_GUARD + _INIT + _INIT",
                     r"OP: _INIT requires self\(absent\)")

    def test_facts_of_a_frame_path(self):
        # _GUARD_GLOBALS_VERSION: the subject is the path
        source = """
            op(_OP, (globals_version/1 --)) effects(reads) exits(next, deopt) %s {
                PyDictObject *dict = (PyDictObject *)%s();
                PyDictKeysObject *keys = FT_ATOMIC_LOAD_PTR_ACQUIRE(dict->ma_keys);
                DEOPT_IF(FT_ATOMIC_LOAD_UINT32_RELAXED(keys->dk_version) != globals_version);
            }
        """
        self.verify(source % ("paths(globals: keys_version(globals_version))", "GLOBALS"))
        self.verify(source % ("paths(builtins: keys_version(globals_version))", "BUILTINS"))
        self.refuses(source % ("", "GLOBALS"),
                     r"the guards establish keys_version\(globals_version\).* of the frame "
                     r"path `globals`, which paths\(...\) does not declare")
        self.refuses(source % ("paths(builtins: keys_version(globals_version))", "GLOBALS"),
                     r"`builtins` is declared keys_version\(globals_version\)")
        self.refuses(source % ("paths(spam: keys_version(globals_version))", "GLOBALS"),
                     r"paths\(spam\) names no frame path")
        self.refuses(source % ("paths(globals: exact(PyDict_Type))", "GLOBALS"),
                     r"exact\(PyDict_Type\) of the path globals is no heap fact")
        # A heap fact: an effect that kills heap facts kills it
        self.refuses(source.replace("(globals_version/1 --)",
                                    "(globals_version/1, x: object -- x: object)").replace(
                         "effects(reads)", "effects(reads, writes)").replace(
                         "!= globals_version);", "!= globals_version);\n"
                         "                spam_write(PyValue_AsKnownObjectBorrow(x));")
                     % ("paths(globals: keys_version(globals_version))", "GLOBALS"),
                     r"`globals` is declared keys_version\(globals_version\)")

    # F15: the GIL product's preprocessor path

    def test_build_arms(self):
        # _ITER_CHECK_LIST: a free-threaded intrinsic in the free-threaded arm
        source = """
            op(_OP, (x -- x)) effects() exits(next) {
            %s
                int y = SPAM_FT_ONLY(1);
            %s
            }
        """
        self.verify(source % ("#ifdef Py_GIL_DISABLED", "#endif"))
        self.verify(source % ("#if !defined(Py_GIL_DISABLED)\n#else", "#endif"))
        self.refuses(source % ("#ifndef Py_GIL_DISABLED", "#endif"),
                     r"calls SPAM_FT_ONLY, an INTRINSIC of the free_threaded build")
        # Nested, with #else: only the selected arm is code
        source = """
            op(_OP, (x: borrowed object --)) effects(%s) exits(next) {
            #ifdef Py_GIL_DISABLED
                int y = SPAM_FT_ONLY(1);
            #else
            #if ENABLE_SPECIALIZATION
                spam_write(PyValue_AsKnownObjectBorrow(x));
            #else
                int z = SPAM_FT_ONLY(2);
            #endif
            #endif
            }
        """
        self.verify(source % "writes")
        self.refuses(source % "", r"`spam_write` may writes")
        # An arm that no macro of the product decides holds no code
        source = """
            op(_OP, (x -- x)) effects() exits(next) {
            #ifdef Py_TRACE_REFS
                %s
            #endif
            }
        """
        self.verify(source % "assert(oparg >= 0);")
        self.refuses(source % "int y = oparg;",
                     r"`#ifdef Py_TRACE_REFS` is decided by no macro of the GIL product")
        # Every build is configured --with-shapes: Py_SHAPES selects its arm
        # (_STORE_ATTR_INSTANCE_VALUE)
        source = """
            op(_OP, (x: borrowed object --)) effects(%s) exits(next) {
            #ifdef Py_SHAPES
                spam_write(PyValue_AsKnownObjectBorrow(x));
            #else
                int z = SPAM_FT_ONLY(2);
            #endif
            }
        """
        self.verify(source % "writes")
        self.refuses(source % "", r"`spam_write` may writes")
        # Py_DEBUG differs between the supported builds: both arms are
        # paths, and they join (_YIELD_VALUE)
        source = """
            op(_OP, (x: consumed --)) effects(releases) exits(next) {
            #ifdef Py_DEBUG
                int y = oparg;
                %s
            #else
                PyValue_CLOSE(x);
            #endif
            }
        """
        self.verify(source % "PyValue_CLOSE(x);")
        self.refuses(source % "", r"`x` is owned on one path and not on another where they join")
        self.refuses("""
            op(_OP, (x -- x)) effects() exits(next) {
            #if ENABLE_SPECIALIZATION
                int y = 0;
            #elif Py_DEBUG
                int y = 1;
            #endif
            }
        """, r"`#elif` is a preprocessor form the walk does not model")

    # Item 4 of G0a4: a move out of an operand home

    def test_a_move_out_of_a_home(self):
        # _LOAD_FAST_AND_CLEAR and _SWAP_FAST: the output takes the home's
        # value and obligation once the uop overwrites the home
        self.verify("""
            op(_OP, (-- value: moved(home))) effects() exits(next) {
                value = GETLOCAL(oparg);
                GETLOCAL(oparg) = PyValue_NULL;
            }
        """)
        source = """
            op(_OP, (value: consumed -- trash: moved(home))) effects() exits(next) {
                PyValue tmp = GETLOCAL(oparg);
                %s
                trash = tmp;
            }
        """
        self.verify(source % "GETLOCAL(oparg) = value;")
        # The home still holds it: two owners
        self.refuses(source % "GETLOCAL(oparg + 1) = value;",
                     r"on next, `trash` is declared moved\(home\), but the home it is "
                     r"loaded from still holds it")
        self.refuses(source % "if (oparg) { GETLOCAL(oparg) = value; } else { "
                              "GETLOCAL(oparg + 1) = value; }",
                     r"`trash` is declared moved\(home\), but is assigned `tmp`, which "
                     r"differs by path")
        # Only a home's value
        self.refuses("""
            op(_OP, (-- value: moved(home))) effects() exits(next) {
                value = PyValue_None;
            }
        """, r"`value` is declared moved\(home\), but is assigned `PyValue_None`, which "
             r"is no value loaded from a home")
        # It may be NULL, an unbound local; the stack sees a new owner
        contract = ir_contract.contract_of(self.verify("""
            op(_OP, (-- value: moved(home))) effects() exits(next) {
                value = GETLOCAL(oparg);
                GETLOCAL(oparg) = PyValue_NULL;
            }
        """).uops["_OP"])
        self.assertEqual(contract.outputs[0], ir_contract.Item(
            "value", "owned", ir_contract.HOME_SOURCE, nullable=True))

    # F18: a frame home is not the heap

    def test_homes_are_their_own_location(self):
        # _LOAD_FAST: a home load reads no heap
        source = """
            op(_OP, (x: %s -- )) effects() exits(next) {
                PyValue v = GETLOCAL(oparg);
                if (PyValue_IsInlineInt(v)) {
                    %s
                    SPAM_NEEDS_INT(v);
                }
                %s
            }
        """
        self.verify(source % ("consumed inline", "", "DEAD(x);"))
        # A home write kills what was loaded from a home
        self.refuses(source % ("consumed", "GETLOCAL(oparg + 1) = x;", ""),
                     r"passes `v` to SPAM_NEEDS_INT, which requires .*inline_int.* of it")
        # A store into a home moves an owner there
        self.refuses(source % ("borrowed", "GETLOCAL(oparg + 1) = x;", ""),
                     r"moves into a home `x`, declared borrowed, which it does not own")
        with self.assertRaisesRegex(SyntaxError, r"a HOME row addresses a frame home, which "
                                                 r"is not the heap"):
            ir_contract.parse_rows("#define X(PRIMITIVE, HELPER, MACRO) "
                                   "MACRO(SPAM, ARGS(scalar), HOME, EFFECTS(reads), EXITS())")

    # F2: a row's guard and the facts on its return

    def test_guard_rows_establish_facts(self):
        # _GUARD_ELEMENT_INT: IR_GUARD_INLINE_INT's facts hold on its return
        source = """
            op(_GUARD_ELEMENT_INT, (x -- x: %s)) effects() exits(next, miss) {
                %s
            }
        """
        self.verify(source % ("inline_int", "SPAM_GUARD_INT(x);"))
        # Only on the path that calls it
        self.refuses(source % ("inline_int", "if (oparg) { SPAM_GUARD_INT(x); }"),
                     r"`x` is declared .*inline_int, which nothing establishes on every path")
        # A row's in_bounds(k) names its argument k
        self.verify("""
            op(_GUARD_INDEX, (seq, i -- seq, i: in_bounds(seq))) effects(reads)
                exits(next, deopt)
            {
                SPAM_GUARD_INDEX(i, seq, 0);
            }
        """)
        for result, message in (
                ("EXITS(next)", "GUARD\\(\\.\\.\\.\\) is the result of a row with a miss or deopt exit"),
                ("EXITS(miss)", None)):
            text = ("#define X(PRIMITIVE, HELPER, MACRO) "
                    f"MACRO(SPAM, ARGS(borrowed), GUARD(inline_int), EFFECTS(), {result})")
            if message is None:
                ir_contract.parse_rows(text)
            else:
                with self.assertRaisesRegex(SyntaxError, message):
                    ir_contract.parse_rows(text)
        with self.assertRaisesRegex(SyntaxError, r"GUARD\(\.\.\.\) states 2 arguments' facts, "
                                                 r"but the row has 1"):
            ir_contract.parse_rows("#define X(PRIMITIVE, HELPER, MACRO) "
                                   "MACRO(SPAM, ARGS(borrowed), GUARD(inline_int, inline), "
                                   "EFFECTS(), EXITS(miss))")

    def test_guard_facts_take_scalar_arguments(self):
        # _GUARD_ELEMENT_LIST and _GUARD_SHAPE_READ: a fact's argument k is
        # the value of the row's scalar argument k
        self.verify("""
            op(_GUARD_ELEMENT_LIST, (container, unused -- container: exact(PyList_Type), unused))
                effects(reads) exits(next, miss)
            {
                SPAM_GUARD_TYPE(container, PyList_Type);
            }
        """)
        source = """
            op(_GUARD_SHAPE_READ, (type_version/2, shape/0, owner: object -- owner: %s))
                effects(reads) exits(next, miss)
            {
                SPAM_GUARD_SHAPE(PyValue_AsKnownObjectBorrow(owner), type_version, %s);
            }
        """
        self.verify(source % ("shape(shape) type_version(type_version)", "shape"))
        self.refuses(source % ("shape(shape) type_version(type_version)", "oparg + 1"),
                     r"`owner` is declared .*shape\(shape\).*, which nothing establishes")
        self.refuses(source % ("shape(type_version) type_version(type_version)", "shape"),
                     r"`owner` is declared .*shape\(type_version\).*, which nothing establishes")
        # A value fact takes a scalar argument; in_bounds names a value
        for row, message in (
                ("ARGS(borrowed, borrowed), GUARD(exact(1))",
                 r"exact\(1\): argument 1 is borrowed; exact\(k\) takes a scalar argument"),
                ("ARGS(borrowed, scalar), GUARD(in_bounds(1))",
                 r"in_bounds\(1\): argument 1 is scalar; in_bounds\(k\) names a value"),
                ("ARGS(borrowed), GUARD(shape(1))", r"shape\(1\): the row has no argument 1")):
            with self.subTest(row=row), self.assertRaisesRegex(SyntaxError, message):
                ir_contract.parse_rows(f"#define X(PRIMITIVE, HELPER, MACRO) MACRO(SPAM, {row}, "
                                       "EFFECTS(reads), EXITS(miss))")

    # The seed's vocabulary (g0b/decisions.md): F9, exits the body takes

    def test_error_labels_are_raise_exits(self):
        # _INSTRUMENTED_LINE: `goto error` is its raise exit at that site
        source = """
            op(_LINE, (--)) effects(raises) exits(%s) {
                int original = SPAM_STATUS();
                if (original < 0) {
                    goto error;
                }
            }
        """
        self.verify(source % "next, raise")
        self.refuses(source % "next", r"_LINE: declares exits\(next\), but the body takes "
                                      r"next, raise")
        # The checkpoint is the site's: what is held there is checked there
        self.refuses("""
            op(_OP, (x: consumed --)) effects(raises) exits(raise) {
                goto exception_unwind;
            }
        """, r"on raise, `x` is still owned, but the site declares consumed")
        # Labels that pop stack items, and any other, stay refused
        self.refuses(source.replace("goto error", "goto pop_1_error") % "next, raise",
                     r"`goto` is a form the walk does not model")

    def test_return_leaves_the_evaluator(self):
        # _INTERPRETER_EXIT: the value goes to the evaluator's caller
        source = """
            op(_EXIT, (retval: %s --)) effects() exits(%s) {
                PyValue result = TAKE(retval);
                return result;
            }
        """
        self.verify(source % ("consumed", "exit"))
        self.refuses(source % ("consumed", "next"), r"declares exits\(next\), but the body "
                                                    r"takes exit")
        self.refuses(source % ("borrowed", "exit"),
                     r"TAKE consumes `retval`, declared borrowed, which it does not own")
        self.refuses("""
            op(_EXIT, (retval: consumed --)) effects() exits(exit) {
                PyValue result = TAKE(retval);
                return;
            }
        """, r"`return` is a form the walk does not model")

    def test_frame_properties_give_exits(self):
        source = """
            returns op(_RET, (--)) effects(switches_frame) exits(%s) {
                int x = 0;
            }
        """
        self.verify(source % "return")
        self.refuses(source % "next, return",
                     r"declares exits\(next, return\), but the body takes return")

    # F4: inputs the skeleton closes

    def test_closed_inputs_are_closed_by_the_skeleton(self):
        # _CALL_LEN: the inputs stay in their homes; the skeleton closes
        # them after the uop, on next and on raise
        source = """
            op(_CALL_LEN, (callable: closed, null: closed, arg: closed -- res: owned))
                effects(runs_python) exits(next, raise)
            {
                Py_ssize_t len_i = _PyValue_Size(arg);
                if (len_i < 0) {
                    ERROR_NO_POP();
                }
                res = PyValue_FromInt64(len_i);
                if (PyValue_IsNull(res)) {
                    ERROR_NO_POP();
                }
                %s
            }
        """
        self.verify(source % "INPUTS_DEAD();")
        self.verify(source % "DEAD(arg);")
        # The body does not release a closed input
        self.refuses(source.replace("arg: closed", "arg: closed exact(PyLong_Type)")
                     % "PyValue_CLOSE_SPECIALIZED(arg, _PyLong_ExactDealloc);",
                     r"PyValue_CLOSE_SPECIALIZED consumes `arg`, declared closed, which it "
                     r"does not own")
        # DEAD surrenders a held owner only if it is closed
        self.refuses("""
            op(_OP, (x: consumed --)) effects() exits(next) {
                DEAD(x);
            }
        """, r"DEAD\(`x`\) drops an owned value")
        # and still marks an input already discharged (_CREATE_INIT_FRAME)
        self.verify("""
            op(_OP, (x: consumed -- r: moved(x))) effects() exits(next) {
                r = x;
                DEAD(x);
            }
        """)

    def test_the_skeleton_close_is_a_release(self):
        # After the uop: a finalizer may replace a pass-through's home
        self.refuses("""
            op(_OP, (b, a: closed -- b: inline_int)) effects() exits(next, miss) {
                EXIT_IF(!PyValue_IsInlineInt(b));
                DEAD(a);
            }
        """, r"`b` is declared .*inline_int, but the skeleton's close of `a` after the uop, "
             r"which may release, may replace the value in its home")
        # and the word cannot restart after it
        self.refuses("""
            op(_CLOSE, (a: closed --)) effects() exits(next) {
                DEAD(a);
            }
            op(_GUARD, (b -- b: inline_int)) effects() exits(next, miss) {
                EXIT_IF(!PyValue_IsInlineInt(b));
            }
            macro(OP) = _CLOSE + _GUARD;
        """, r"the guard in _GUARD follows the skeleton's close of `a` after _CLOSE")

    # F5: moves through rows and locals

    def test_moved_results(self):
        # _MAKE_HEAP_SAFE, and a move through a local (_JUMP_IF_FALSE_OR_POP)
        self.verify("""
            op(_MAKE_HEAP_SAFE, (value: consumed -- taken: moved(value))) effects()
                exits(next)
            {
                taken = TAKE(value);
            }
            op(_THROUGH, (cond: consumed -- res: moved(cond))) effects() exits(next) {
                PyValue held = TAKE(cond);
                res = held;
            }
        """)
        # Only an owner made in the body is declared owned
        self.refuses("""
            op(_OP, (value: consumed -- res: owned)) effects() exits(next) {
                PyValue held = TAKE(value);
                res = held;
            }
        """, r"`res` is declared owned, but is assigned `held`, which is the input `value`'s "
             r"value: declare moved\(value\)")
        # and a moved output holds its input's value
        self.refuses("""
            op(_OP, (value: consumed, other: consumed -- res: moved(value)))
                effects() exits(next)
            {
                res = TAKE(other);
                PyValue gone = TAKE(value);
                res = gone;
            }
        """, r"`res` is declared moved\(value\), but is assigned `TAKE \( other \)`, which is "
             r"`other`'s value")
        self.refuses("""
            op(_OP, (value: consumed -- res: moved(value))) effects(allocates, raises)
                exits(next, raise)
            {
                res = SPAM_NEW();
                ERROR_IF(PyValue_IsNull(res));
            }
        """, r"`res` is declared moved\(value\), but is assigned `SPAM_NEW \( \)`, which is "
             r"no input's value")
        # A row's moved(k) takes argument k's obligation
        with self.assertRaisesRegex(SyntaxError, r"row SPAM: moved\(0\): argument 0 is not "
                                                 r"consumed"):
            ir_contract.parse_rows("#define X(PRIMITIVE, HELPER, MACRO) "
                                   "MACRO(SPAM, ARGS(borrowed), VALUE(moved(0)), EFFECTS(), "
                                   "EXITS())")

    # F3: constants, locals and conditionals

    def test_constants_and_conditionals(self):
        # _CALL_ISINSTANCE: a conditional between two constants
        source = """
            op(_OP, (-- b: inline exact(PyBool_Type))) effects() exits(next) {
                b = oparg ? PyValue_True : %s;
            }
        """
        self.verify(source % "PyValue_False")
        self.refuses(source % "PyValue_None",
                     r"`b` is declared exact\(PyBool_Type\), but PyValue_None gives "
                     r"exact\(_PyNone_Type\)")
        # _PUSH_NULL: an output that is NULL on next declares it
        source = """
            op(_PUSH_NULL, (-- res: %s)) effects() exits(next) {
                res = PyValue_NULL;
            }
        """
        self.verify(source % "inline null")
        self.refuses(source % "inline", r"`res` may be NULL from PyValue_NULL")
        with self.assertRaisesRegex(SyntaxError, r"row SPAM: a CONSTANT is a static immortal "
                                                 r"value"):
            ir_contract.parse_rows("#define X(PRIMITIVE, HELPER, MACRO) "
                                   "CONSTANT(SPAM, VALUE(owned))")

    def test_null_is_the_negation_of_nonnull(self):
        # PyValue_IsNull declares both sides; `null` implies nothing more
        null = ir_contract.parse_fact("null", None)
        self.assertEqual(self.table["PyValue_IsNull"].result.tests,
                         ((frozenset({null}), frozenset({ir_contract.NONNULL})),))
        self.assertEqual(ir_contract.implied({null}), {null})
        # A word that is NULL where a guard passes declares it
        source = """
            op(_GUARD, (x -- x: %s)) effects() exits(next, deopt) {
                DEOPT_IF(!PyValue_IsNull(x));
            }
        """
        self.verify(source % "null")
        self.refuses(source % "nonnull", r"the guards establish null of `x`")

    def test_a_null_arm_follows_its_condition(self):
        # _CALL_INTRINSIC_2: NULL exactly where the raise's condition holds
        source = """
            op(_OP, (-- res: owned)) effects(raises) exits(next, raise) {
                int err = SPAM_STATUS();
                res = err < 0 ? PyValue_NULL : PyValue_None;
                ERROR_IF(%s);
            }
        """
        self.verify(source % "err < 0")
        self.refuses(source % "err > 0", r"on raise, the new owner `res` is still held")

    def test_values_through_locals(self):
        # _CALL_BUILTIN_O: a row's result reaches the output through a local
        source = """
            op(_OP, (-- res: owned%s)) effects(allocates, raises, reads) exits(next, raise) {
                PyValue result = %s;
                if (PyValue_IsNull(result)) {
                    ERROR_NO_POP();
                }
                res = result;
            }
        """
        self.verify(source % ("", "SPAM_NEW()"))
        self.refuses(source % (" exact(PyLong_Type)", "SPAM_NEW()"),
                     r"`res` is declared exact\(PyLong_Type\), but SPAM_NEW gives no fact")
        self.refuses(source % ("", "SPAM_BORROW()"),
                     r"`res` is declared owned, but `result` holds no owner \(from "
                     r"SPAM_BORROW\): no owner moves")
        self.refuses("""
            op(_OP, (x: borrowed -- res: owned)) effects() exits(next) {
                res = x.bits;
            }
        """, r"`x \. bits` is assigned to an output, but it is no row with a value result")


# Rows that use the columns that name another row; TA declares the tree's
COLUMN_ROWS = """
#define TEST_COLUMNS(PRIMITIVE, HELPER, MACRO) \\
    PRIMITIVE(PyValue_BothInlineInt, bool, (PyValueBorrow, PyValueBorrow), \\
              ARGS(borrowed, borrowed), TEST(inline_int, inline_int), EFFECTS(), \\
              CONJOINS(PyValue_IsInlineInt)) \\
    PRIMITIVE(PyValue_DUP, PyValue, (PyValueBorrow), ARGS(borrowed nonnull), \\
              VALUE(owned), EFFECTS(), NONNULL_FORM(PyValue_XDUP)) \\
    PRIMITIVE(PyValue_FromInt64, PyValue, (int64_t), ARGS(scalar), \\
              VALUE_OR_NULL(owned exact(PyLong_Type)), EFFECTS(allocates, raises), \\
              SLOW(_PyValue_BoxInt64))
"""


def column_rows(text=COLUMN_ROWS):
    table = ir_contract.rows() | ir_contract.parse_rows(text)
    ir_contract.check_columns(table)
    return table


class ColumnTests(unittest.TestCase):
    "CONJOINS, NONNULL_FORM and SLOW: declared, checked, then tested in C"

    def test_columns_parse(self):
        table = column_rows()
        self.assertEqual(table["PyValue_BothInlineInt"].conjoins, "PyValue_IsInlineInt")
        self.assertEqual(table["PyValue_DUP"].nonnull_form, "PyValue_XDUP")
        self.assertEqual(table["PyValue_FromInt64"].slow, "_PyValue_BoxInt64")
        with self.assertRaisesRegex(SyntaxError, r"optionally each of .* once"):
            ir_contract.parse_rows(COLUMN_ROWS.replace(
                "CONJOINS(PyValue_IsInlineInt)",
                "CONJOINS(PyValue_IsInlineInt), CONJOINS(PyValue_IsInlineInt)"))
        with self.assertRaisesRegex(SyntaxError, r"optionally each of FEEDBACK"):
            ir_contract.parse_rows("""
                #define X(PRIMITIVE, HELPER, MACRO) \\
                    MACRO(SPAM, ARGS(), NONE, EFFECTS(), EXITS(), SLOW(spam))
            """)
        with self.assertRaisesRegex(SyntaxError, r"names no row"):
            column_rows(COLUMN_ROWS.replace("SLOW(_PyValue_BoxInt64)", "SLOW(spam)"))

    def refuses(self, changes, message):
        text = COLUMN_ROWS
        for old, new in changes:
            self.assertIn(old, text)
            text = text.replace(old, new)
        with self.assertRaisesRegex(SyntaxError, message):
            column_rows(text)

    def test_conjoins_is_checked(self):
        conjoins = "CONJOINS(PyValue_IsInlineInt)"
        cases = [
            ([(conjoins, "CONJOINS(PyValue_IsNull)")],
             r"PyValue_BothInlineInt: CONJOINS\(PyValue_IsNull\): its TEST is"),
            ([(conjoins, "CONJOINS(_PyValue_IntScaled)")],
             r"a two-argument TEST of a one-argument"),
            # The C signature
            ([("(PyValueBorrow, PyValueBorrow)", "(PyValueBorrow, PyValue)")],
             r"its C type is PyValue_IsInlineInt's with two arguments"),
            # Effects or feedback change what fusion may skip
            ([("TEST(inline_int, inline_int), EFFECTS()", "TEST(inline_int, inline_int), "
               "EFFECTS(reads)")], r"PyValue_BothInlineInt has effects or feedback"),
            ([(conjoins, conjoins + ", FEEDBACK(read(counter))")],
             r"PyValue_BothInlineInt has effects or feedback"),
            # A precondition: fusion would evaluate the second test unguarded
            ([("ARGS(borrowed, borrowed)", "ARGS(borrowed nonnull, borrowed nonnull)")],
             r"PyValue_BothInlineInt is not total"),
            ([("ARGS(borrowed, borrowed)", "ARGS(consumed, consumed)")],
             r"is not total|modes differ"),
        ]
        for changes, message in cases:
            with self.subTest(changes=changes):
                self.refuses(changes, message)

    def test_nonnull_form_is_checked(self):
        cases = [
            ([("ARGS(borrowed nonnull)", "ARGS(borrowed)")],
             r"PyValue_DUP: NONNULL_FORM\(PyValue_XDUP\): its argument is PyValue_XDUP's "
             r"with nonnull added and nothing else"),
            # Another precondition besides nonnull
            ([("ARGS(borrowed nonnull)", "ARGS(borrowed nonnull object)")],
             r"with nonnull added and nothing else"),
            ([("ARGS(borrowed nonnull)", "ARGS(consumed nonnull)")],
             r"with nonnull added and nothing else"),
            ([("VALUE(owned), EFFECTS(), NONNULL", "VALUE(borrowed), EFFECTS(), NONNULL")],
             r"its result is PyValue_XDUP's, with at least its facts and no null_iff"),
            ([("VALUE(owned), EFFECTS(), NONNULL", "VALUE(owned null_iff(0)), EFFECTS(), "
               "NONNULL")], r"no null_iff"),
            ([("NONNULL_FORM(PyValue_XDUP)", "NONNULL_FORM(PyValue_XDUP), "
               "FEEDBACK(read(counter))")], r"its feedback is PyValue_XDUP's"),
            ([("VALUE(owned), EFFECTS(), NONNULL", "VALUE(owned), EFFECTS(reads), NONNULL")],
             r"its effects within them"),
        ]
        for changes, message in cases:
            with self.subTest(changes=changes):
                self.refuses(changes, message)

    def test_slow_is_checked(self):
        cases = [
            ([("EFFECTS(allocates, raises), \\\n", "EFFECTS(allocates), \\\n")],
             r"PyValue_FromInt64: SLOW\(_PyValue_BoxInt64\): the row declares fewer "
             r"effects"),
            ([("SLOW(_PyValue_BoxInt64)", "SLOW(PyValue_IsNull)")],
             r"a HELPER with a VALUE_OR_NULL result"),
            # The slow path calls the helper with the row's arguments
            ([("(int64_t), ARGS(scalar)", "(int32_t), ARGS(scalar)")],
             r"_PyValue_BoxInt64 takes the row's parameters and gives its result's mode"),
            ([("VALUE_OR_NULL(owned exact", "VALUE_OR_NULL(borrowed exact")],
             r"gives its result's mode"),
            ([("SLOW(_PyValue_BoxInt64)", "SLOW(value_binary_op)")],
             r"takes the row's parameters"),
        ]
        for changes, message in cases:
            with self.subTest(changes=changes):
                self.refuses(changes, message)

    def test_columns_against_c(self):
        """Each declared column against the C definitions on generated
        values; test_jit_template holds each lowering to the same
        definitions, so the lowerings follow"""
        capi = import_helper.import_module("_testinternalcapi")
        if not hasattr(capi, "leaf_primitive"):
            self.skipTest("no leaf primitives")
        from test import test_ir_leaf as leaf
        table = column_rows()
        for row in table.values():
            if row.conjoins:
                one = ir_leaf.PRIMITIVES[row.conjoins]
                words = [w for (w,) in leaf.cases(one)]
                for a in words:
                    for b in words:
                        with self.subTest(row=row.name, a=a, b=b):
                            self.assertEqual(bool(capi.leaf_primitive(row.name, a, b)),
                                             bool(capi.leaf_primitive(one.name, a))
                                             and bool(capi.leaf_primitive(one.name, b)))
            if row.nonnull_form:
                general = ir_leaf.PRIMITIVES[row.nonnull_form]
                for (w,) in leaf.cases(general):
                    if w == 0:
                        continue
                    with self.subTest(row=row.name, w=w):
                        mine = capi.leaf_primitive(row.name, w)
                        theirs = capi.leaf_primitive(general.name, w)
                        capi.leaf_primitive("PyValue_CLOSE", mine)
                        capi.leaf_primitive("PyValue_CLOSE", theirs)
                        self.assertEqual(mine, theirs)
            if row.slow:
                # The model's slow path is the declared helper, and on values
                # whose allocation succeeds the result is not NULL
                self.assertEqual(ir_leaf.PRIMITIVES[row.name].slow, row.slow)
                for i in leaf.WIDE_INTS:
                    with self.subTest(row=row.name, i=i):
                        w = capi.leaf_primitive(row.name, i)
                        self.assertNotEqual(w, 0)
                        capi.leaf_primitive("PyValue_CLOSE", w)


if __name__ == "__main__":
    unittest.main()
