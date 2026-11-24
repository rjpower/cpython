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

    def test_declarations_equal_the_inference(self):
        parts = {(name, part.name): part
                 for name, inst in ir_generator.handler_instructions(analysis).items()
                 for part in inst.parts if isinstance(part, cases.analyzer.Uop)}
        lines = ir_contract_diff.differences(records, parts)
        self.assertEqual(lines[:-1], [])
        self.assertRegex(lines[-1], r"^\d+ declared uops compared$")

    def test_component_checks_are_generated(self):
        checks = "\n".join(ir_contract.component_checks())
        table = ir_contract.rows()
        for row in table.values():
            if row.kind in ("HELPER", "PRIMITIVE", "DESTRUCTOR", "FIELD"):
                self.assertIn(f'"{row.name}: its C type differs', checks)
        self.assertIn("__typeof__(&_PyValue_BoxInt64), PyValue (*)(int64_t)", checks)
        self.assertIn("__typeof__(&_PyLong_ExactDealloc), void (*)(PyObject *)", checks)
        self.assertIn("__typeof__(((PyTypeObject *)0)->tp_version_tag), unsigned int", checks)
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
    MACRO(SPAM_GUARD, ARGS(scalar), NONE, EFFECTS(), EXITS(miss))
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
            inst(OP, (value: consumed exact(PyLong_Type) -- res: owned)) effects(writes)
                exits(next)
            {
                res = value;
                PyValue_CLOSE_SPECIALIZED(value, _PyLong_ExactDealloc);
            }
        """, r"PyValue_CLOSE_SPECIALIZED consumes `value`, which is already consumed or moved")
        # Moved twice
        self.refuses("""
            inst(OP, (value: consumed -- a: owned, b: owned)) effects() exits(next) {
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
        records = ir_generator.build_records(analysis, ir_generator.compute_formats(analysis))
        parts = {("OP", u.name): u for u in analysis.uops.values()}
        self.assertEqual(ir_contract_diff.differences(records, parts),
                         ["OP/_OP: effects: declared ['READS_HEAP'], inferred []",
                          "1 declared uops compared"])

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
        self.refuses(source % "PyValue t = oparg ? a : b;",
                     r"`PyValue t = oparg \? a : b` uses `a` in a form the walk does not "
                     r"model")
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


if __name__ == "__main__":
    unittest.main()


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
