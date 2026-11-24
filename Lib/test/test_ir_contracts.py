"""Declared operation contracts (Tools/cases_generator/ir_contract.py).

A uop of Python/bytecodes.c may declare its contract: modes and facts on its
stack items, `effects(...)` and `exits(...)`.  Its record then comes from
the declarations, and the generator verifies its body against them and
against the rows of Include/internal/pycore_ir_components.h.  The pilot
declares BINARY_OP's add arms and generic form, UNPACK_SEQUENCE and
LOAD_ATTR_INSTANCE_VALUE."""

import dataclasses
import io
import os
from pathlib import Path
import re
import sys
import tempfile
import textwrap
import unittest
import weakref
from unittest import mock

from test import support
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


TABLE_OVER_DECLARED = {"IR_TRY_DICT_FIRST_PROBE": {"raise"}}


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
        self.assertEqual([facts(g) for g in add_int.guards], [["ENCODING(v1, inline_int)"]])
        add_float = uop_record("BINARY_OP_ADD_FLOAT", "_BINARY_OP_ADD_FLOAT")
        self.assertEqual([facts(g) for g in add_float.guards],
                         [["ENCODING(v0, inline_float|inline_int)",
                           "ENCODING(v1, inline_float|inline_int)"]])
        self.assertEqual([str(f) for f in add_float.results], ["EXACT_TYPE(v2, PyFloat_Type)"])
        result = uop_record("BINARY_OP_ADD_INT", "_BINARY_OP_ADD_INT").results
        self.assertEqual([str(f) for f in result], ["EXACT_TYPE(v2, PyLong_Type)"])
        # The implicit pointer-space guard, then the declared type test
        guard_list = uop_record("UNPACK_SEQUENCE_LIST", "_GUARD_TOS_LIST")
        self.assertEqual([facts(g) for g in guard_list.guards],
                         [["ENCODING(v0, object)"], ["EXACT_TYPE(v0, PyList_Type)"]])
        unpack = uop_record("UNPACK_SEQUENCE_LIST", "_UNPACK_SEQUENCE_LIST")
        self.assertEqual([(facts(g), g.reason) for g in unpack.guards],
                         [([], "a free-threaded build's critical section"),
                          (["LENGTH(v0, oparg)"], "")])
        # Facts read through a path from the operand: its type's version, its
        # inline values' flag; and the loaded value's, recorded on the local
        handler = "LOAD_ATTR_INSTANCE_VALUE"
        self.assertEqual([facts(g) for g in uop_record(handler, "_GUARD_TYPE_VERSION").guards],
                         [["ENCODING(v0, object)"], ["TYPE_VERSION(v0, type_version)"]])
        has_values = uop_record(handler, "_CHECK_MANAGED_OBJECT_HAS_VALUES")
        self.assertEqual([facts(g) for g in has_values.guards], [["INLINE_VALUES_VALID(v0)"]])
        load = uop_record(handler, "_LOAD_ATTR_INSTANCE_VALUE")
        self.assertEqual([facts(g) for g in load.guards], [["ENCODING(found, inline_float|inline_int|object)"]])
        self.assertEqual(load.effects, {READS_HEAP, MAY_RELEASE})

    # Where the tables (ir_records, deleted at G0d) declare an exit that the
    # macro's expansion cannot take.  IR_TRY_DICT_FIRST_PROBE: a hit stores
    # the result and dispatches, a miss falls through; the probe reads one
    # slot and calls no hash or __eq__ (G0c batch 2 review).
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
                inferred -= TABLE_OVER_DECLARED.get(name, set())
                self.assertLessEqual(inferred - {"next"}, set(table[name].exits))

    # The reviewed corrections of G0c, each listed with its evidence in the
    # batch's differences.md and in the commit that made it.  Batch 8:
    # INSTRUMENTED_JUMP takes PROCESS_ERROR_AT when the event fails, so
    # _MONITOR_END_ASYNC_FOR may raise.  G0d1: it leaves by the redirect exit
    # when a callback redirects the frame, which the record cannot say
    LISTED_DIFFERENCES = [
        # G0d1: a handler that never runs ends the process (Py_FatalError):
        # its one exit is abort, for which the record has no transfer
        'CACHE/_CACHE: declares exits(abort), the record has ',
        'IR_OPERANDS/_IR_OPERANDS: declares exits(abort), the record has ',
        'RESERVED/_RESERVED: declares exits(abort), the record has ',
        'INSTRUMENTED_END_ASYNC_FOR/_MONITOR_END_ASYNC_FOR: declares exits(next, '
        'raise, redirect), the record has next',
        # Batch 7: no word executes RETURN_GENERATOR, so its result is NULL
        # (F19); _GUARD_TOS_IS_NONE tests the identity of the word with
        # PyValue_None (F20)
        'RETURN_GENERATOR.record.uops[0].results: inferred 0 entries [], '
        'declared 1 [{"__type": "Fact", "arg": "null", "kind": "ENCODING", '
        '"subject": "v0"}]',
        *(line
          for handler in ("SEND_VIRTUAL", "SEND_VIRTUAL_XFER_DISCARD",
                          "SEND_VIRTUAL_XFER_HOME", "SEND_VIRTUAL_XFER_POP_TOP",
                          "SEND_VIRTUAL_XFER_RETURN", "SEND_VIRTUAL_XFER_STORE_FAST",
                          "SEND_VIRTUAL_XFER_STORE_LOCAL")
          for line in (
              f'{handler}.record.uops[0].guards[0].facts[0].arg: '
              'inferred "", declared "PyValue_None"',
              f'{handler}.record.uops[0].guards[0].facts[0].kind: '
              'inferred "IS_NONE", declared "IDENTITY"')),
        # G0c9: _TO_BOOL_NONE's guard states the identity of the word with
        # PyValue_None, as _GUARD_TOS_IS_NONE's does (F20)
        *(line
          for handler in ("POP_JUMP_IF_FALSE_NONE", "POP_JUMP_IF_TRUE_NONE",
                          "TO_BOOL_NONE")
          for line in (
              f'{handler}.record.uops[0].guards[0].facts[0].arg: '
              'inferred "", declared "PyValue_None"',
              f'{handler}.record.uops[0].guards[0].facts[0].kind: '
              'inferred "IS_NONE", declared "IDENTITY"')),
        # G0c9: _GET_ITER's second result is a tagged index or NULL
        # (_PyEval_GetIter's row, G0a5's representation table)
        'GET_ITER.record.uops[1].results: inferred 0 entries [], declared 1 '
        '[{"__type": "Fact", "arg": "null|tagged_index", "kind": "ENCODING", '
        '"subject": "v2"}]',
        # G0c9: _SEND_ASYNC_GEN's body completes in place (it reaches the end
        # of the body, `next`); SEND_ASYNC_GEN_XFER_DISCARD's yield is its
        # fused transfer's, which G0d takes from the declaration
        'SEND_ASYNC_GEN_XFER_DISCARD/_SEND_ASYNC_GEN: declares exits(branch, next, raise), '
        'the record has branch, raise, yield',
        # G3a: a RETURN variant's completion returns (its boundary(...) clause,
        # completes(RETURN: exits(return, raise))) where the uop branches
        'SEND_ASYNC_GEN_XFER_RETURN/_SEND_ASYNC_GEN: declares exits(branch, next, raise), '
        'the record has raise, return, yield',
        'CLEANUP_THROW_XFER_RETURN/_CLEANUP_THROW: declares exits(branch, raise), '
        'the record has raise, return',
        # G3a: SWAP's exchange keeps every home (pass-throughs); the walk
        # reads the copies as a consume of each input, and each output as
        # the other input's value, whose count moves to it (ALIAS, from
        # the declared moved(x) since G0d1).  A record from the
        # declaration reads Item.exchange.
        'SWAP/_SWAP: input `bottom` is declared a pass-through, the record has CONSUMED',
        'SWAP/_SWAP: input `top` is declared a pass-through, the record has CONSUMED',
        'SWAP/_SWAP: output `bottom` is declared a pass-through, the record has ALIAS',
        'SWAP/_SWAP: output `top` is declared a pass-through, the record has ALIAS',
        # Batch 1: _PUSH_NULL's result is NULL (F19); LOAD_FAST_CHECK's raise
        # path calls _PyEval_FormatExcCheckArg, which runs Python (F11)
        'GET_ITER_SELF.record.uops[1].results: inferred 0 entries [], declared 1 '
        '[{"__type": "Fact", "arg": "null", "kind": "ENCODING", "subject": "v1"}]',
        'LOAD_FAST_CHECK.record.format.runs_code: inferred false, declared true',
        'LOAD_FAST_CHECK.record.uops[0].effects: inferred 2 entries '
        '["MAY_RAISE", "READS_HEAP"], declared 6 ["ALLOCATES", "MAY_RAISE", '
        '"MAY_RELEASE", "MAY_RUN_PYTHON", "READS_HEAP", "WRITES_HEAP"]',
        # Batch 4: the raise paths of LOAD_LOCALS and LOAD_DEREF can run
        # Python (F11); LOAD_GLOBAL_BUILTIN's fact names the local it reads
        'LOAD_DEREF.record.format.runs_code: inferred false, declared true',
        'LOAD_DEREF.record.uops[0].effects: inferred 2 entries '
        '["MAY_RAISE", "READS_HEAP"], declared 6 ["ALLOCATES", "MAY_RAISE", '
        '"MAY_RELEASE", "MAY_RUN_PYTHON", "READS_HEAP", "WRITES_HEAP"]',
        'LOAD_LOCALS.record.format.runs_code: inferred false, declared true',
        'LOAD_LOCALS.record.uops[0].effects: inferred 2 entries '
        '["MAY_RAISE", "READS_HEAP"], declared 6 ["ALLOCATES", "MAY_RAISE", '
        '"MAY_RELEASE", "MAY_RUN_PYTHON", "READS_HEAP", "WRITES_HEAP"]',
        'LOAD_GLOBAL_BUILTIN.record.uops[1].guards[0].facts[0].subject: '
        'inferred "builtins", declared "dict"',
        # G0d2 (analyzer and effect_sites read the rows): MATERIALIZE_KWARGS,
        # BUILD_TEMPLATE and BUILD_LIST no longer differ.
        # A call escapes where its row declares a reentry effect, and the
        # inference reads an escape as every effect at once: the frame pushes
        # release the callable (call_push_exact), while the declaration says
        # which effects they have; the inference of the builders reads the
        # rows of their calls, which state fewer effects than the uop declares
        'BUILD_INTERPOLATION.record.uops[0].effects: inferred 3 entries ["ALLOCATES", "MAY_RAISE", "MAY_RELEASE"], declared 6 ["ALLOCATES", "MAY_RAISE", "MAY_RELEASE", "MAY_RUN_PYTHON", "READS_HEAP", "WRITES_HEAP"]',
        'BUILD_SLICE.record.uops[0].effects: inferred 3 entries ["ALLOCATES", "MAY_RAISE", "MAY_RELEASE"], declared 6 ["ALLOCATES", "MAY_RAISE", "MAY_RELEASE", "MAY_RUN_PYTHON", "READS_HEAP", "WRITES_HEAP"]',
        'SET_FUNCTION_ATTRIBUTE.record.uops[0].effects: inferred 1 entries ["WRITES_HEAP"], declared 2 ["READS_HEAP", "WRITES_HEAP"]',
        'CALL_BOUND_METHOD_EXACT_ARGS.record.uops[7].effects: inferred 6 entries ["ALLOCATES", "MAY_RAISE", "MAY_RELEASE", "MAY_RUN_PYTHON", "READS_HEAP", "WRITES_HEAP"], declared 4 ["ALLOCATES", "MAY_RELEASE", "READS_HEAP", "WRITES_HEAP"]',
        'CALL_PY_EXACT_ARGS.record.uops[5].effects: inferred 6 entries ["ALLOCATES", "MAY_RAISE", "MAY_RELEASE", "MAY_RUN_PYTHON", "READS_HEAP", "WRITES_HEAP"], declared 4 ["ALLOCATES", "MAY_RELEASE", "READS_HEAP", "WRITES_HEAP"]',
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
        # G0d4: the iterator family's cursor has distinct names (`index` in,
        # `next_index` out, as the F16 rule asks).  The inference reads the
        # handlers with their declarations removed, where nothing says that
        # the output replaces the input in the same stack position (ir_records'
        # replaced_input reads that from the declared modes), so it takes the
        # FOR_ITER group for a plain push and a discarded window.  The declared
        # records keep the group's format and mark the replaced cursor changed
        # (its entry holds a new value); the leaf records of the uops differ
        # only in the cursor's encoding fact, which GET_ITER and the cursor
        # helper establish.
        'FOR_ITER.record.format.discards: inferred false, declared true',
        'FOR_ITER.record.format.dst_offset: inferred 1, declared 2',
        'FOR_ITER.record.format.store_dst: inferred false, declared true',
        'FOR_ITER.record.layout.move_dst: inferred null, declared 2',
        'FOR_ITER.record.layout.move_dst_args: inferred "", declared "ir_word, A"',
        'FOR_ITER.record.operands[0].may_name: inferred 1 entries ["TEMP"], declared 2 ["LOCAL", "TEMP"]',
        'FOR_ITER.record.outputs[1][1]: inferred -1, declared 1',
        'FOR_ITER.record.outputs[1][2].name: inferred "next_index", declared "null_or_index"',
        'FOR_ITER.record.outputs[1][2].peek: inferred false, declared true',
        'FOR_ITER.record.store_mode: inferred null, declared "store"',
        'FOR_ITER.record.uops[1].results: inferred 0 entries [], declared 1 [{"__type": "Fact", "arg": "null|tagged_index", "kind": "ENCODING", "subject": "v2"}]',
        'FOR_ITER_DICT_ITEMS.record.format.discards: inferred false, declared true',
        'FOR_ITER_DICT_ITEMS.record.format.dst_offset: inferred 1, declared 2',
        'FOR_ITER_DICT_ITEMS.record.format.store_dst: inferred false, declared true',
        'FOR_ITER_DICT_ITEMS.record.layout.move_dst: inferred null, declared 2',
        'FOR_ITER_DICT_ITEMS.record.layout.move_dst_args: inferred "", declared "ir_word, A"',
        'FOR_ITER_DICT_ITEMS.record.operands[0].may_name: inferred 1 entries ["TEMP"], declared 2 ["LOCAL", "TEMP"]',
        'FOR_ITER_DICT_ITEMS.record.store_mode: inferred null, declared "store"',
        'FOR_ITER_GEN.record.format.discards: inferred false, declared true',
        'FOR_ITER_GEN.record.format.dst_offset: inferred 1, declared 2',
        'FOR_ITER_GEN.record.format.store_dst: inferred false, declared true',
        'FOR_ITER_GEN.record.operands[0].may_name: inferred 1 entries ["TEMP"], declared 2 ["LOCAL", "TEMP"]',
        'FOR_ITER_GEN.record.results: inferred 0 entries [], declared 1 [[-1, {"__type": "Operand", "detail": "callee return", "field": "A", "kind": "SLOT", "may_name": [], "position": null}]]',
        'FOR_ITER_GEN.record.store_mode: inferred null, declared "return"',
        'FOR_ITER_LIST.record.format.discards: inferred false, declared true',
        'FOR_ITER_LIST.record.format.dst_offset: inferred 1, declared 2',
        'FOR_ITER_LIST.record.format.store_dst: inferred false, declared true',
        'FOR_ITER_LIST.record.layout.move_dst: inferred null, declared 2',
        'FOR_ITER_LIST.record.layout.move_dst_args: inferred "", declared "ir_word, A"',
        'FOR_ITER_LIST.record.operands[0].may_name: inferred 1 entries ["TEMP"], declared 2 ["LOCAL", "TEMP"]',
        'FOR_ITER_LIST.record.outputs[1][1]: inferred -1, declared 1',
        'FOR_ITER_LIST.record.outputs[1][2].name: inferred "next_index", declared "null_or_index"',
        'FOR_ITER_LIST.record.outputs[1][2].peek: inferred false, declared true',
        'FOR_ITER_LIST.record.store_mode: inferred null, declared "store"',
        'FOR_ITER_RANGE.record.format.discards: inferred false, declared true',
        'FOR_ITER_RANGE.record.format.dst_offset: inferred 1, declared 2',
        'FOR_ITER_RANGE.record.format.store_dst: inferred false, declared true',
        'FOR_ITER_RANGE.record.layout.move_dst: inferred null, declared 2',
        'FOR_ITER_RANGE.record.layout.move_dst_args: inferred "", declared "ir_word, A"',
        'FOR_ITER_RANGE.record.operands[0].may_name: inferred 1 entries ["TEMP"], declared 2 ["LOCAL", "TEMP"]',
        'FOR_ITER_RANGE.record.store_mode: inferred null, declared "store"',
        'FOR_ITER_TUPLE.record.format.discards: inferred false, declared true',
        'FOR_ITER_TUPLE.record.format.dst_offset: inferred 1, declared 2',
        'FOR_ITER_TUPLE.record.format.store_dst: inferred false, declared true',
        'FOR_ITER_TUPLE.record.layout.move_dst: inferred null, declared 2',
        'FOR_ITER_TUPLE.record.layout.move_dst_args: inferred "", declared "ir_word, A"',
        'FOR_ITER_TUPLE.record.operands[0].may_name: inferred 1 entries ["TEMP"], declared 2 ["LOCAL", "TEMP"]',
        'FOR_ITER_TUPLE.record.outputs[1][1]: inferred -1, declared 1',
        'FOR_ITER_TUPLE.record.outputs[1][2].name: inferred "next_index", declared "null_or_index"',
        'FOR_ITER_TUPLE.record.outputs[1][2].peek: inferred false, declared true',
        'FOR_ITER_TUPLE.record.store_mode: inferred null, declared "store"',
        'FOR_ITER_VIRTUAL.record.format.discards: inferred false, declared true',
        'FOR_ITER_VIRTUAL.record.format.dst_offset: inferred 1, declared 2',
        'FOR_ITER_VIRTUAL.record.format.store_dst: inferred false, declared true',
        'FOR_ITER_VIRTUAL.record.layout.move_dst: inferred null, declared 2',
        'FOR_ITER_VIRTUAL.record.layout.move_dst_args: inferred "", declared "ir_word, A"',
        'FOR_ITER_VIRTUAL.record.operands[0].may_name: inferred 1 entries ["TEMP"], declared 2 ["LOCAL", "TEMP"]',
        'FOR_ITER_VIRTUAL.record.outputs[1][1]: inferred -1, declared 1',
        'FOR_ITER_VIRTUAL.record.outputs[1][2].name: inferred "next_index", declared "index"',
        'FOR_ITER_VIRTUAL.record.store_mode: inferred null, declared "store"',
        'FOR_ITER_VIRTUAL.record.uops[0].results: inferred 0 entries [], declared 1 [{"__type": "Fact", "arg": "null|tagged_index", "kind": "ENCODING", "subject": "v2"}]',
        'FOR_ITER.record.uops[1].inputs[1].changed: inferred false, declared true',
        'FOR_ITER_LIST.record.uops[2].inputs[1].changed: inferred false, declared true',
        'FOR_ITER_TUPLE.record.uops[2].inputs[1].changed: inferred false, declared true',
        'FOR_ITER_VIRTUAL.record.uops[0].inputs[1].changed: inferred false, declared true',
        'INSTRUMENTED_FOR_ITER.record.format.discards: inferred false, declared true',
        'INSTRUMENTED_FOR_ITER.record.format.dst_offset: inferred 1, declared 2',
        'INSTRUMENTED_FOR_ITER.record.format.store_dst: inferred false, declared true',
        'INSTRUMENTED_FOR_ITER.record.layout.move_dst: inferred null, declared 2',
        'INSTRUMENTED_FOR_ITER.record.layout.move_dst_args: inferred "", declared "ir_word, A"',
        'INSTRUMENTED_FOR_ITER.record.operands[0].may_name: inferred 1 entries ["TEMP"], declared 2 ["LOCAL", "TEMP"]',
        'INSTRUMENTED_FOR_ITER.record.store_mode: inferred null, declared "store"',
    ]

    # Batch 5 and G0c9: the assume op of `iter: assumed(object) by GET_ITER`
    # in the leaf IR of the three iterator guards
    LISTED_LEAF_DIFFERENCES = [
        ("FOR_ITER_LIST.record.uops[0].leaf.blocks[0].ops:", '"kind": "assume"'),
        ("FOR_ITER_TUPLE.record.uops[0].leaf.blocks[0].ops:", '"kind": "assume"'),
        ("FOR_ITER_RANGE.record.uops[0].leaf.blocks[0].ops:", '"kind": "assume"'),
        # G0d1: a yield is declared (`exits(yield)`); with the declarations
        # removed the inference no longer reads the generator's state, and
        # a frame switch that no site names is an entry.  The boundary
        # variants' lines fall under LISTED_BOUNDARY_FAMILIES (G0d2)
        ("YIELD_VALUE.record.uops[1].transfers[0].kind:", '"ENTER_FRAME", declared "YIELD"'),
        ("INSTRUMENTED_YIELD_VALUE.record.uops[2].transfers[0].kind:",
         '"ENTER_FRAME", declared "YIELD"'),
    ]

    # G0d2: the generator boundary is checked against the declarations of
    # the SEND family's uops (send_source_contract), so the inference, which
    # reads undeclared bodies, finds no boundary in the handlers that cross
    # one: their records differ as a family, not one by one
    LISTED_BOUNDARY_FAMILIES = re.compile(
        r"(CLEANUP_THROW|INSTRUMENTED_YIELD_DELEGATED|INSTRUMENTED_YIELD_VALUE|"
        r"SEND|SEND_ASYNC_GEN|SEND_GEN|SEND_VIRTUAL|YIELD_DELEGATED|YIELD_VALUE)"
        r"_XFER_\w+\.record\.")

    def test_declarations_equal_the_inference(self):
        with cases.handle_stderr():
            lines = ir_contract_diff.report(str(BYTECODES))
        # Every reported line is a reviewed difference: an exact line
        # (LISTED_DIFFERENCES) or a long leaf-IR line named by its prefix and
        # a substring it must contain (LISTED_LEAF_DIFFERENCES)
        reported = [line for line in lines[:-1]
                    if line in self.LISTED_DIFFERENCES
                    or not self.LISTED_BOUNDARY_FAMILIES.match(line)]
        exact = [line for line in reported if line in self.LISTED_DIFFERENCES]
        self.assertCountEqual(exact, self.LISTED_DIFFERENCES)
        leaf = [line for line in reported if line not in self.LISTED_DIFFERENCES]
        self.assertCountEqual([line.split(" ")[0] for line in leaf],
                              [prefix for prefix, _ in self.LISTED_LEAF_DIFFERENCES])
        needs = dict(self.LISTED_LEAF_DIFFERENCES)
        for line in leaf:
            self.assertIn(needs[line.split(" ")[0]], line)
        self.assertRegex(lines[-1], r"^\d+ handlers compared; \d+ of \d+ uops declared$")

    def test_owner_may_be_null(self):
        # OWNER(x) moves x's obligation, or is NULL for a local's view
        # whatever x is: no value transport, unlike TAKE (null_iff(0))
        table = ir_contract.rows()
        owner, take = table["OWNER"].result, table["TAKE"].result
        self.assertEqual((owner.shape, owner.mode, owner.moved), ("VALUE_OR_NULL", "moved", 0))
        self.assertEqual((take.shape, take.moved, take.null_iff), ("VALUE", 0, 0))
        self.assertIsNone(ir_leaf.intrinsic("OWNER"))
        self.assertIsNotNone(ir_leaf.intrinsic("TAKE"))

    def test_a_steal_is_declared(self):
        # PyValue_FromNonNumberSteal steals its object (the in-place add's
        # `temp`, now that the walk admits it)
        table = ir_contract.rows()
        self.assertEqual(table["PyValue_FromNonNumberSteal"].args[0].mode, "consumed")

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
    MACRO(SPAM_EVENT, ARGS(), NONE, EFFECTS(runs_python), EXITS(redirect)) \\
    HELPER(T, PyValue, (PyValue), ARGS(taken), \\
           VALUE_OR_NULL(owned), EFFECTS(allocates, raises)) \\
    MACRO(SPAM_GUARD, ARGS(scalar), NONE, EFFECTS(), EXITS(miss)) \
    MACRO(TAKE, ARGS(consumed), VALUE(moved(0)), EFFECTS(), EXITS(), TAKES_VIEW()) \
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
        # The analyzer decides which calls escape from the rows too
        patch = mock.patch.object(ir_contract, "rows", return_value=self.table)
        patch.start()
        self.addCleanup(patch.stop)

    def verify(self, source, temporaries=False):
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
                ir_contract.verify_handler(inst, self.table, temporaries=temporaries)
        return result

    def refuses(self, source, message, temporaries=False):
        with self.assertRaisesRegex(SyntaxError, message):
            self.verify(source, temporaries)

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


    # The exchange (SWAP): pass-through outputs that take each other's inputs

    EXCHANGE = """
        inst(OP, (bottom, unused[oparg-2], top --
                  bottom: moved(top), unused[oparg-2], top: moved(bottom)))
            effects() exits(next)
        {
            %s
        }
    """

    def test_exchange_accepts_swap(self):
        result = self.verify(self.EXCHANGE % "PyValue temp = bottom; bottom = top; top = temp;")
        contract = ir_contract.contract_of(result.uops["OP"])
        self.assertEqual({o.name: o.exchange for o in contract.outputs if o.exchange},
                         {"bottom": "top", "top": "bottom"})
        self.assertEqual({i.mode for i in contract.inputs}, {""})

    def test_exchange_body_exchanges(self):
        self.refuses(self.EXCHANGE % "PyValue temp = bottom; top = temp;",
                     r"OP: the body leaves `bottom` with `bottom`, and it declares moved\(top\)")
        self.refuses(self.EXCHANGE % "PyValue temp = bottom; bottom = top; top = temp; "
                                     "PyValue_CLOSE(temp);",
                     r"OP: an exchange's body only copies between bottom, top and temporaries")
        self.refuses(self.EXCHANGE % "PyValue temp = bottom; bottom = PyValue_None; top = temp;",
                     r"OP: an exchange's body only copies")
        self.refuses("""
            inst(OP, (bottom, unused[oparg-2], top --
                      bottom: moved(top), unused[oparg-2], top: moved(bottom)))
                effects(releases) exits(next)
            {
                PyValue temp = bottom; bottom = top; top = temp;
            }
        """, r"OP: an exchange has effects\(\) exits\(next\)")

    def test_exchange_is_of_pass_throughs(self):
        self.refuses("""
            inst(OP, (bottom, top -- bottom: moved(top), top: moved(bottom), copy: moved(top)))
                effects() exits(next)
            {
                PyValue temp = bottom; bottom = top; top = temp;
            }
        """, r"OP: `copy` is no pass-through, and moves from `top`, which is exchanged")
        self.refuses("""
            inst(OP, (bottom, top -- bottom: moved(bottom), top))
                effects() exits(next)
            {
            }
        """, r"OP: `bottom` is exchanged with itself")

    def test_exchange_is_a_permutation(self):
        self.refuses("""
            inst(OP, (a, b, c -- a: moved(b), b: moved(c), c))
                effects() exits(next)
            {
                a = b; b = c;
            }
        """, r"OP: the exchange of a, b takes b, c: no permutation")
        self.refuses("""
            inst(OP, (a, b, c -- a: moved(c), b: moved(c), c: moved(a)))
                effects() exits(next)
            {
                PyValue t = a; a = c; b = c; c = t;
            }
        """, r"no permutation")

    def test_exchange_keeps_homes(self):
        self.refuses("""
            inst(OP, (bottom, unused[oparg], top --
                      bottom: moved(top), unused[oparg+1], top: moved(bottom)))
                effects() exits(next)
            {
                PyValue temp = bottom; bottom = top; top = temp;
            }
        """, r"OP: `top` is exchanged, and its home changes")

    def test_swap_is_a_declared_exchange(self):
        swap = analysis.instructions["SWAP"]
        contract = ir_contract.contract_of(swap.parts[0])
        self.assertEqual({o.name: o.exchange for o in contract.outputs if o.exchange},
                         {"bottom": "top", "top": "bottom"})
        formats = ir_generator.compute_formats(analysis)
        self.assertEqual(ir_generator.normal_transfer_kind(swap, formats["SWAP"]),
                         "_PyIR_TRANSFER_SWAP")

    def test_replacements_are_declared(self):
        # F16's cursor: each uop that advances it declares the entry that
        # its next_index takes over, and only those replace an input
        replaced = {(name, j) for name, uop in analysis.uops.items()
                    for j in range(len(uop.stack.outputs))
                    if ir_records.replaced_input(uop, j) is not None}
        self.assertEqual(replaced, {(name, 1) for name in (
            "_FOR_ITER", "_FOR_ITER_VIRTUAL", "_ITER_NEXT_LIST", "_ITER_NEXT_TUPLE")})
        for name, _ in replaced:
            output = analysis.uops[name].stack.outputs[1]
            self.assertEqual((output.name, output.replaces), ("next_index", "index"))

    def test_an_undeclared_replacement_is_refused(self):
        # Renamed outputs above unused[oparg] move no entry only if declared
        self.refuses("""
            inst(OP, (callable: consumed, self_or_null: consumed, unused[oparg]
                      -- init: owned, self: owned, unused[oparg]))
                effects() exits(next)
            {
                init = callable;
                self = self_or_null;
            }
        """, r"Cannot have used input 'self_or_null' below an unused value")

    def test_a_replacement_names_a_consumed_input_at_its_position(self):
        self.refuses("""
            inst(OP, (iter, index -- iter, index: replaces(index)))
                effects() exits(next)
            {
            }
        """, r"`index` replaces itself")
        self.refuses("""
            inst(OP, (iter: consumed, index: consumed -- next_index: inline replaces(index)))
                effects() exits(next)
            {
                next_index = index;
                PyValue_CLOSE(iter);
            }
        """, r"`next_index` replaces `index`, which is at another stack position")
        self.refuses("""
            inst(OP, (iter: object, index: consumed -- iter, next_index: inline replaces(spam)))
                effects() exits(next)
            {
                next_index = index;
            }
        """, r"`next_index` replaces `spam`, which is no input")
        self.refuses("""
            inst(OP, (iter: object, index: borrowed -- iter, next_index: inline replaces(index)))
                effects() exits(next)
            {
                next_index = index;
            }
        """, r"OP: `next_index` replaces `index`, which is borrowed, not consumed")
        self.refuses("""
            inst(OP, (iter: object, index: consumed replaces(iter) -- iter, next_index: inline))
                effects() exits(next)
            {
                next_index = index;
            }
        """, r"replaces\(iter\) is an output's: `index` is an input")

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
        """, r"_BAD_ARRAY: stores `PyValue_NULL`, which may be NULL, into `values`, "
             r"whose elements are not nullable")
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

    def test_a_nested_row_call_is_checked_by_its_result(self):
        # An argument that is a row's call is what the row's result says:
        # PyTuple_GET_VALUE's SCALAR result is not shown to be nonnull, and
        # _PyValue_CodeConstant's assumed result is, once bound (F7)
        self.refuses("""
            op(_OP, (-- value: owned)) effects(reads) exits(next) {
                value = PyValue_DUP(PyTuple_GET_VALUE(FRAME_CO_CONSTS, oparg));
            }
        """, r"_OP: passes `PyTuple_GET_VALUE \( FRAME_CO_CONSTS , oparg \)` to "
             r"PyValue_DUP, which requires nonnull of it")
        self.refuses("""
            op(_OP, (-- value: owned)) effects() exits(next) {
                value = PyValue_DUP(GETLOCAL(oparg));
            }
        """, r"_OP: passes `GETLOCAL \( oparg \)` to PyValue_DUP, which requires "
             r"nonnull of it")
        self.verify("""
            op(_OP, (-- value: owned)) effects(reads) exits(next) {
                PyValueBorrow constant = _PyValue_CodeConstant(
                    PyTuple_GET_VALUE(FRAME_CO_CONSTS, oparg));
                value = PyValue_DUP(constant);
            }
        """)
        self.verify("""
            op(_OP, (-- value: owned)) effects(reads) exits(next) {
                value = PyValue_DUP(PyValue_FromObjectBorrow(Py_None));
            }
        """)

    def test_an_unnamed_argument_is_checked_by_what_it_shows(self):
        # Every argument with required facts is checked, named or not: a
        # decided CASES field, a declared array element, a NULL literal and
        # any other expression (which shows nothing).  Before, each of these
        # passed unchecked
        self.refuses("""
            op(_OP, (-- v: owned)) effects() exits(next) {
                v = PyValue_DUP(PyValue_NULL);
            }
        """, r"_OP: passes `PyValue_NULL` to PyValue_DUP, which requires nonnull of it")
        self.refuses("""
            op(_OP, (-- v: owned)) effects(reads) exits(next) {
                PyValue *target = &IR_SLOT(IR_DST);
                v = PyValue_DUP(*target);
            }
        """, r"_OP: passes `\* target` to PyValue_DUP, which requires nonnull of it")
        array = """
            op(_OP, (values[oparg]: borrowed%s -- v: owned)) effects() exits(next) {
                v = PyValue_DUP(values[0]);
            }
        """
        self.verify(array % "")
        self.refuses(array % " nullable", r"_OP: passes `values \[ 0 \]` to "
                     r"PyValue_DUP, which requires nonnull of it")
        field = """
            op(_OP, (d: borrowed object -- v: owned)) effects(runs_python) exits(next, raise) {
                PyObject *o = PyValue_AsKnownObjectBorrow(d);
                _PyStatusValue found = value_load_from_dict(tstate, o, o);
                %s
                ERROR_IF(found.status < 0);
                if (found.status == 0) {
                    v = PyValue_None;
                }
                else {
                    %s
                    v = found.value;
                }
            }
        """
        dup = "PyValue_CLOSE(PyValue_DUP(found.value));"
        self.verify(field % ("", dup))
        self.refuses(field % (dup, ""), r"_OP: passes `found\.value` to PyValue_DUP, "
                     r"which requires nonnull of it")

    def test_a_consumed_argument_names_an_owner(self):
        # A consumed or taken argument passes an obligation: a name that
        # holds one, a row's new owner as it is made, or a value that owes
        # no count.  A borrow names none
        self.refuses("""
            op(_OP, (--)) effects(releases) exits(next) {
                PyValueBorrow b = _PyValue_BoundLocal(GETLOCAL(oparg));
                PyValue_CLOSE(b);
            }
        """, r"_OP: PyValue_CLOSE consumes `b`, which names no owner")
        # An object pointer that a row returns without a count
        self.refuses("""
            op(_OP, (v: borrowed object --)) effects(reads, releases) exits(next) {
                PyObject *cls = PyExceptionInstance_Class(PyValue_AsKnownObjectBorrow(v));
                Py_XDECREF(cls);
            }
        """, r"_OP: Py_XDECREF consumes `cls`, which names no owner")
        # A home load whose home still holds it
        self.refuses("""
            op(_OP, (--)) effects(releases) exits(next) {
                PyValue tmp = GETLOCAL(oparg);
                PyValue_XCLOSE(tmp);
            }
        """, r"_OP: PyValue_XCLOSE consumes `tmp`, which names no owner")
        # An object owner is consumed once
        self.refuses("""
            op(_OP, (v: borrowed object --)) effects(reads, releases) exits(next) {
                PyObject *tb = PyException_GetTraceback(PyValue_AsKnownObjectBorrow(v));
                Py_XDECREF(tb);
                Py_XDECREF(tb);
            }
        """, r"_OP: Py_XDECREF consumes `tb`, which is already consumed or moved")

    @support.subTests("body", [
        # _DELETE_FAST and _MAKE_CELL: a home move
        ("""PyValue tmp = GETLOCAL(oparg);
           GETLOCAL(oparg) = PyValue_NULL;
           PyValue_XCLOSE(tmp);""", "next"),
        # _BINARY_OP_INPLACE_ADD_UNICODE: the same through IR_SLOT
        ("""PyValue held = IR_SLOT(IR_DST);
           IR_SLOT(IR_DST) = PyValue_NULL;
           PyValue_XCLOSE(held);""", "next"),
        # _END_ASYNC_FOR and _CLEANUP_THROW: Py_NewRef's new owner, passed
        ("""_PyErr_SetRaisedException(tstate, Py_NewRef(PyValue_AsKnownObjectBorrow(v)));""", "next"),
        # _RAISE_VARARGS: Py_XNewRef's owners, NULL where the object is
        ("""PyObject *o = PyValue_ObjectOrNull(v);
           PyObject *owner = Py_XNewRef(o);
           Py_XDECREF(owner);""", "next"),
        # _MAKE_CELL: PyCell_NewValue's new cell
        ("""PyObject *cell = PyCell_NewValue(v);
           ERROR_IF(cell == NULL);
           PyValue c = PyValue_FromObjectSteal(cell);
           PyValue_CLOSE(c);""", "next, raise"),
        # _DICT_UPDATE: the raised exception's owner
        ("""PyObject *exc = _PyErr_GetRaisedException(tstate);
           if (oparg) { Py_XDECREF(exc); }
           else { _PyErr_ChainExceptions1(exc); }""", "next"),
        # _WITH_EXCEPT_START: a new reference or NULL
        ("""PyObject *tb = PyException_GetTraceback(PyValue_AsKnownObjectBorrow(v));
           Py_XDECREF(tb);""", "next"),
        # _DELETE_DEREF and _INSERT_NULL: NULL owes nothing
        ("""PyValue_XCLOSE(PyValue_NULL);""", "next"),
    ])
    def test_consumed_arguments_that_name_owners(self, body):
        body, exits = body
        self.verify("""
            op(_OP, (v: borrowed object --)) effects(runs_python) exits(%s) {
                %s
            }
        """ % (exits, body))

    def test_an_array_sized_by_a_test_is_empty_where_it_fails(self):
        # _LOAD_ATTR: self_or_null[oparg&1] has no element where `oparg & 1`
        # is false; there it is neither written nor owed
        source = """
            op(_OP, (owner: borrowed -- attr: owned, self_or_null[oparg&1]: owned nullable))
                effects() exits(next)
            {
                if (oparg & 1) {
                    self_or_null[0] = PyValue_NULL;
                    %s
                }
                else {
                    %s
                }
                attr = PyValue_None;
            }
        """
        self.verify(source % ("", ""))
        self.refuses(source % ("", "self_or_null[0] = PyValue_NULL;"),
                     r"stores into `self_or_null`, which has no element here")
        # Written on the true side only where the test holds
        self.refuses(source.replace("self_or_null[0] = PyValue_NULL;", "") % ("", ""),
                     r"the output array `self_or_null` is unwritten")

    def test_an_identity_test_shares_facts(self):
        # PyValue_Is is declared IDENTITY(): where it holds, its arguments
        # are one word, so a copy of a home that is `left` has left's facts
        # (_BINARY_OP_INPLACE_ADD_UNICODE's target local)
        source = """
            op(_OP, (left: exact(PyUnicode_Type) -- left)) effects(reads) exits(next, miss) {
                PyValue held = IR_SLOT(IR_DST);
                %s
                PyObject *o = PyValue_AsKnownObjectBorrow(held);
            }
        """
        self.verify(source % "EXIT_IF(!PyValue_Is(held, left));")
        self.refuses(source % "",
                     r"passes `held` to PyValue_AsKnownObjectBorrow, which requires "
                     r"nonnull, object of it")
        # Where the test fails, nothing is learned
        self.refuses(source % "EXIT_IF(PyValue_Is(held, left));",
                     r"passes `held` to PyValue_AsKnownObjectBorrow")

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
        # A PRIMITIVE row without a model is refused: it is a HELPER or an INTRINSIC
        self.assertNotIn("PyValue_TYPE", ir_leaf.PRIMITIVES)
        row = dataclasses.replace(self.table["PyValue_IsNull"], name="PyValue_TYPE")
        with self.assertRaisesRegex(ValueError, r"PyValue_TYPE: a PRIMITIVE with no model"):
            ir_contract.check_primitives(ir_leaf.PRIMITIVES, ir_contract.lowerings(),
                                         {"PyValue_TYPE": row})
        ir_contract.check_primitives(ir_leaf.PRIMITIVES, ir_contract.lowerings(),
                                     {"PyValue_TYPE": self.table["PyValue_TYPE"]})

    def test_view_columns(self):
        # The generator reads which calls take a view and how a view's owner is
        # released from the rows (G0d3)
        self.assertEqual({r.name for r in self.table.values() if r.takes_view},
                         {"TAKE", "OWNER", "PyValue_AsKnownObjectSteal"})
        self.assertEqual(self.table["PyValue_CLOSE"].view_close, "PyValue_XCLOSE")
        self.assertEqual(self.table["PyValue_CLOSE_SPECIALIZED"].pointer_form,
                         "_PyValue_XCLOSE_SPECIALIZED_POINTER")
        with self.assertRaisesRegex(SyntaxError, r"TAKES_VIEW\(\) is a flag"):
            ir_contract.parse_rows(
                "#define X(PRIMITIVE, HELPER, MACRO) "
                "MACRO(spam, ARGS(consumed), NONE, EFFECTS(), EXITS(), TAKES_VIEW(x))")

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
                                                 r"SLOW\(\.\.\.\), TAKES_VIEW\(\.\.\.\), "
                                                 r"HOMES\(\.\.\.\), OBSERVER\(\.\.\.\) once; "
                                                 r"got 'EXITS\(next\)'"):
            ir_contract.parse_rows(
                "#define X(PRIMITIVE, HELPER, MACRO) "
                "HELPER(spam, int, (int), ARGS(scalar), SCALAR, EFFECTS(), EXITS(next))")

    def test_homes_and_observer_describe_a_reentrant_row(self):
        rows = ir_contract.parse_rows(
            "#define X(PRIMITIVE, HELPER, MACRO) "
            "HELPER(spam, int, (int), ARGS(scalar), SCALAR, EFFECTS(runs_python), "
            "HOMES(canonical), OBSERVER(none))")
        self.assertEqual((rows["spam"].homes, rows["spam"].observer), ("canonical", "none"))
        for columns, message in (("HOMES(canonical)", "declares no reentry effect"),
                                 ("OBSERVER(none)", "declares no reentry effect")):
            with self.assertRaisesRegex(SyntaxError, message):
                ir_contract.parse_rows(
                    "#define X(PRIMITIVE, HELPER, MACRO) "
                    f"HELPER(spam, int, (int), ARGS(scalar), SCALAR, EFFECTS(reads), {columns})")
        with self.assertRaisesRegex(SyntaxError, r"HOMES\(canonical \| unknown\)"):
            ir_contract.parse_rows(
                "#define X(PRIMITIVE, HELPER, MACRO) "
                "HELPER(spam, int, (int), ARGS(scalar), SCALAR, EFFECTS(runs_python), HOMES(any))")

    # F1: fact kinds, their classes and lifetimes

    def test_fact_kinds(self):
        for word in ("in_bounds(seq)", "nonnegative", "nonzero", "is(PyValue_None)",
                     "subtype(PyType_Type)", "not_subtype(PyType_Type)",
                     "not_exact(PyList_Type)", "exact(PyGen_Type|PyCoro_Type)",
                     "type_flag(Py_TPFLAGS_HEAPTYPE)", "type_slot(tp_iter,PyObject_SelfIter)",
                     "class_version", "func_version(K)", "argcount", "vectorcall",
                     "convention(_Py_CALL_NOARGS|_Py_CALL_O)", "ascii", "no_managed_dict", "unique", "null"):
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

    def test_a_static_immortal_value_owes_no_count(self):
        # _POP_EXCEPT: None is a CONSTANT row's value, which DEAD may drop
        source = """
            op(_OP, (exc_value: consumed object --)) effects(reads, releases) exits(next) {
                _PyErr_StackItem *exc_info = tstate->exc_info;
                if (%s) {
                    DEAD(exc_value);
                    Py_CLEAR(exc_info->exc_value);
                }
                else {
                    PyObject *exc = PyValue_AsKnownObjectSteal(exc_value);
                    Py_XSETREF(exc_info->exc_value, exc);
                }
            }
        """
        self.verify(source % "PyValue_IsNone(exc_value)")
        self.refuses(source % "!PyValue_IsNone(exc_value)",
                     r"DEAD\(`exc_value`\) drops an owned value")
        # A test of the kind of word names no value
        self.refuses(source % "PyValue_IsObject(exc_value)",
                     r"DEAD\(`exc_value`\) drops an owned value")

    def test_a_status_states_facts_of_an_argument_on_success(self):
        # CHECK_EXC_MATCH: _PyEval_CheckExceptTypeValidValue succeeds only
        # for an object, STATUS(NEGATIVE, , object), so the value is an
        # object where the status says success, as a TEST's true side
        source = """
            inst(OP, (left: object, right: consumed -- left, b: inline exact(PyBool_Type)))
                effects(runs_python) exits(next, raise)
            {
                PyObject *left_o = PyValue_AsKnownObjectBorrow(left);
                PyValue held_right = TAKE(right);
                int err = %s(tstate, held_right);
                %s
                if (err < 0) {
                    PyValue_CLOSE(held_right);
                    ERROR_NO_POP();
                }
                %s
                int res = PyErr_GivenExceptionMatches(left_o, right_o);
                PyValue_CLOSE(held_right);
                b = res ? PyValue_True : PyValue_False;
            }
        """
        borrow = "PyObject *right_o = PyValue_AsKnownObjectBorrow(held_right);"
        check = "_PyEval_CheckExceptTypeValidValue"
        self.assertEqual(self.table[check].result.success,
                         (frozenset(), frozenset({ir_contract.OBJECT})))
        self.verify(source % (check, "", borrow))
        # Not where the status may report failure
        self.refuses(source % (check, borrow, ""), r"requires object of it")
        # A status that states nothing of its arguments shows nothing
        self.table["SPAM_CHECK"] = dataclasses.replace(
            self.table[check], name="SPAM_CHECK",
            result=dataclasses.replace(self.table[check].result, success=()))
        self.refuses(source % ("SPAM_CHECK", "", borrow), r"requires object of it")

    def test_an_exact_value_of_a_closed_type_owes_no_count(self):
        # _POP_JUMP_IF_TRUE: every bool is True or False, static immortal
        # values (PyBool_Type's INSTANCES), so DEAD of an exact bool releases
        # nothing.  An exact int is an object like any other
        source = """
            op(_OP, (cond: consumed assumed(exact(%s)) by _TO_BOOL --))
                effects() exits(next, branch)
            {
                assert(PyValue_BoolCheck(cond));
                int flag = PyValue_IsTrue(cond);
                DEAD(cond);
                IR_REPORT_EDGE(flag);
                if (flag) {
                    JUMPTO(oparg);
                }
            }
        """
        self.verify(source % "PyBool_Type")
        self.verify(source % "_PyNone_Type")
        self.refuses(source % "PyLong_Type", r"DEAD\(`cond`\) drops an owned value")
        self.refuses(source % "PyBool_Type|PyLong_Type",
                     r"DEAD\(`cond`\) drops an owned value")
        # The instances are data, held to the CONSTANT rows
        self.assertEqual(self.table["PyBool_Type"].instances,
                         ("PyValue_True", "PyValue_False"))
        with self.assertRaisesRegex(SyntaxError, r"INSTANCES\(PyValue_None\): not a VALUE "
                                    r"CONSTANT of exact\(PyBool_Type\)"):
            ir_contract.check_columns(ir_contract.rows() | ir_contract.parse_rows(
                "#define SPAM_ROWS(PRIMITIVE, HELPER, MACRO) \\\n"
                "    CONSTANT(PyBool_Type, OBJECT(PyTypeObject), INSTANCES(PyValue_None))\n"))

    def test_an_object_pointer_tested_for_null(self):
        # _BUILD_MAP: a row's VALUE_OR_NULL result as a PyObject *, which
        # `x == NULL` decides as PyValue_IsNull(x) does for a value
        source = """
            op(_OP, (values[oparg*2]: consumed -- map: owned))
                effects(reads, writes, allocates, raises, releases, runs_python)
                exits(next, raise)
            {
                PyObject *map_o = IR_BUILD_MAP(values);
                DEAD(values);
                ERROR_IF(%s);
                map = PyValue_FromObjectSteal(map_o);
            }
        """
        self.verify(source % "map_o == NULL")
        self.refuses(source % "map_o != NULL", r"on raise, the new owner `map_o` is still held")
        self.refuses(source % "oparg == 0", r"on raise, the new owner `map_o` is still held")
        self.verify(source % "!map_o")
        self.refuses(source % "map_o", r"on raise, the new owner `map_o` is still held")
        # _BUILD_TUPLE: a row that takes its values only if it succeeds
        source = """
            op(_OP, (values[oparg]: taken -- tup: owned)) effects(allocates, raises)
                exits(next, raise)
            {
                PyObject *tup_o = IR_BUILD_TUPLE(values);
                if (%s) {
                    ERROR_NO_POP();
                }
                INPUTS_DEAD();
                tup = PyValue_FromObjectSteal(tup_o);
            }
        """
        self.verify(source % "tup_o == NULL")
        self.verify(source % "!tup_o")
        self.refuses(source % "tup_o", r"on raise, `values` is released, but it is "
                                        r"declared taken")

    def test_a_move_joined_with_a_new_owner(self):
        # _FORMAT_SIMPLE: res is value itself on one path and a new owner on
        # the other; their join is owned, and the move discharges value
        source = """
            op(_OP, (value: consumed object -- res: %s)) effects(runs_python)
                exits(next, raise)
            {
                if (%s) {
                    PyObject *res_o = _PyValue_Format(value, PyValue_NULL);
                    PyValue_CLOSE(value);
                    ERROR_IF(res_o == NULL);
                    res = PyValue_FromObjectSteal(res_o);
                }
                else {
                    res = TAKE(value);
                }
            }
        """
        unicode = "!PyUnicode_CheckExact(PyValue_AsKnownObjectBorrow(value))"
        self.verify(source % ("owned", unicode))
        # moved(value) only where every path moves
        self.refuses(source % ("moved(value)", unicode),
                     r"`res` is declared moved\(value\), but is assigned "
                     r"`PyValue_FromObjectSteal")
        # Where every path moves, it must say so
        self.refuses("""
            op(_OP, (value: consumed object -- res: owned)) effects() exits(next) {
                if (oparg) {
                    res = TAKE(value);
                }
                else {
                    res = TAKE(value);
                }
            }
        """, r"`res` is declared owned, but is assigned `TAKE \( value \)`, which is the "
             r"input `value`'s value: declare moved\(value\)")

    def test_a_committed_exit_publishes_the_outputs(self):
        # _INSTRUMENTED_JUMP_IF_FALSE_OR_POP: the observer is taken after the
        # generator stores the outputs, so each must be written before it;
        # it dispatches, so the walk ends there
        source = """
            op(_OP, (cond: consumed -- res: moved(cond)%s))
                effects(runs_python) exits(next, branch, raise)
            {
                PyValue held = TAKE(cond);
                int truth = _PyValue_IsTrue(held);
                if (truth < 0) {
                    PyValue_CLOSE(held);
                    ERROR_NO_POP();
                }
                if (!truth) {
                    res = held;
                    IR_OBSERVE_COMMITTED_BRANCH(this_instr, JUMP_TARGET(oparg), 0, 0);
                }
                else {
                    %s
                    IR_OBSERVE_COMMITTED_BRANCH(this_instr, next_instr, 0, 0);
                }
            }
        """
        popped = "IR_DEFER_CLOSE(held);\n res = PyValue_NULL;"
        self.verify(source % (" nullable", popped))
        # NULL on one exit: the output says so
        self.refuses(source % ("", popped),
                     r"`res` may be NULL from PyValue_NULL, and nothing raises on it")
        # An output the generator would store before it is written
        self.refuses(source % (" nullable", "IR_DEFER_CLOSE(held);"),
                     r"`res` is not written before IR_OBSERVE_COMMITTED_BRANCH, whose "
                     r"exits \(branch, raise\) are committed")
        self.refuses("""
            op(_OP, (cond: consumed -- res: moved(cond)))
                effects(runs_python) exits(branch, raise)
            {
                PyValue held = TAKE(cond);
                IR_OBSERVE_COMMITTED_BRANCH(this_instr, next_instr, 0, 0);
                res = held;
            }
        """, r"`res` is not written before IR_OBSERVE_COMMITTED_BRANCH")
        # The generator's observer replacer is selected by the declaration
        self.assertEqual(ir_generator.committed_markers(),
                         {r.name for r in self.table.values() if r.committed})
        self.assertIn("IR_OBSERVE_COMMITTED_BRANCH", ir_generator.committed_markers())
        # next publishes, and a guard exit restarts the word: neither is
        # committed, nor a successor
        for mark in ("committed", "successor"):
            for exit in ("next", "miss"):
                with self.subTest(mark=mark, exit=exit):
                    with self.assertRaisesRegex(SyntaxError, f"{exit} cannot be {mark}"):
                        ir_contract.parse_rows(
                            "#define X(PRIMITIVE, HELPER, MACRO) MACRO(SPAM, ARGS(scalar), "
                            f"NONE, EFFECTS(), EXITS({exit}({mark})))")

    def test_a_successor_is_published_at_the_end(self):
        # _POP_JUMP_IF_FALSE_NONE_OR_PLAIN + POP_TOP: JUMPTO assigns the
        # next word; the uop continues, and POP_TOP runs on both paths, so v
        # is published at the body's end as on next
        source = """
            op(_OP, (value: consumed -- v: moved(value))) effects() exits(next, branch) {
                int flag = PyValue_IsNone(value);
                IR_REPORT_EDGE(flag);
                %s
            }
            op(_POP, (value: consumed --)) effects(releases) exits(next) {
                PyValue_CLOSE(value);
            }
            macro(OP) = _OP + _POP;
        """
        self.verify(source % """
                v = value;
                DEAD(value);
                if (flag) {
                    JUMPTO(oparg);
                }""")
        # Unconditional, it is a jump
        self.refuses(source % "v = value; DEAD(value); JUMPTO(oparg);",
                     r"declares exits\(branch, next\), but the body takes jump, next")
        # Written only off the jump path
        self.refuses(source % """
                if (flag) {
                    JUMPTO(oparg);
                }
                else {
                    v = value;
                    DEAD(value);
                }""", r"`(v|value)` is owned on one path and not on another where they join")
        # A word that has assigned its successor does not raise
        self.refuses("""
            op(_OP, (value: consumed -- v: moved(value)))
                effects(raises) exits(next, branch, raise)
            {
                v = value;
                DEAD(value);
                if (oparg) {
                    JUMPTO(oparg);
                }
                ERROR_IF(oparg > 3);
            }
        """, r"on raise, JUMPTO has assigned the next word")

    def test_effects_confined_to_exits(self):
        # _ITER_JUMP_RANGE + _ITER_NEXT_RANGE: the jump runs Python only in
        # IR_COMPLETE_STEP, which leaves by branch or raise, so the facts
        # the next uop needs survive the jump on the path to next
        source = """
            op(_CHECK, (iter, null_or_index -- iter: object, null_or_index))
                effects() exits(next, miss)
            {
                EXIT_IF(!PyValue_IsObject(iter));
            }
            op(_JUMP, (iter: object, null_or_index -- iter, null_or_index))
                effects(%s) exits(next, branch, raise)
            {
                _PyRangeIterObject *r = (_PyRangeIterObject *)PyValue_AsKnownObjectBorrow(iter);
                if (r->len <= 0) {
                    PyValue completed = PyValue_None;
                    IR_COMPLETE_STEP(_PY_IR_FOR_ITER, completed, oparg);
                }
            }
            op(_NEXT, (iter: object, null_or_index -- iter, null_or_index, next: owned))
                effects(reads, writes, allocates, raises) exits(next, raise)
            {
                _PyRangeIterObject *r = (_PyRangeIterObject *)PyValue_AsKnownObjectBorrow(iter);
                long value = r->start;
                r->start = value + r->step;
                r->len--;
                next = PyValue_FromInt64(value);
                ERROR_IF(PyValue_IsNull(next));
            }
            macro(OP) = _CHECK + _JUMP + _NEXT;
        """
        self.verify(source % "reads, runs_python(branch|raise)")
        self.refuses(source % "runs_python",
                     r"_NEXT requires nonnull, object of `iter`, which nothing before it "
                     r"establishes or which an effect since killed")
        # A site on the path to next
        self.refuses(source % "reads(branch|raise), runs_python(branch|raise)",
                     r"`->` may reads \(a load through a pointer\), which the uop "
                     r"declares only on branch, raise, but it does not leave the uop")
        # A call that may leave by an exit the confinement leaves out
        self.refuses(source % "reads, runs_python(branch)",
                     r"`IR_COMPLETE_STEP` may \w+ .*declares only on branch, "
                     r"but it may leave by raise")
        self.refuses(source % "reads, runs_python(branch|enter)",
                     r"runs_python is confined to enter, which the uop does not declare")
        self.refuses(source % "reads, runs_python(next|raise)",
                     r"confines an effect to next")
        self.refuses(source % "reads, runs_python(branch|nowhere)",
                     r"confines an effect to nowhere, which is no exit")

    def test_a_confined_effect_is_exported_where_compiled_code_goes_on(self):
        # The record, the metadata and the stub plans read the effects of the
        # paths after which compiled code goes on: an effect confined to
        # raise leaves them, one confined to branch stays (the branch's
        # target runs after it)
        source = """
            op(_JUMP, (iter: object, null_or_index -- iter, null_or_index))
                effects(reads, %s) exits(next, branch, raise)
            {
                _PyRangeIterObject *r = (_PyRangeIterObject *)PyValue_AsKnownObjectBorrow(iter);
                if (r->len <= 0) {
                    PyValue completed = PyValue_None;
                    IR_COMPLETE_STEP(_PY_IR_FOR_ITER, completed, oparg);
                }
            }
        """
        contract = ir_contract.contract_of(self.verify(
            source % "runs_python(branch|raise)").uops["_JUMP"])
        self.assertNotIn("runs_python", contract.next_effects())
        self.assertIn("runs_python", contract.continuing_effects())
        # An effect confined to raise alone is not on a path that goes on
        self.assertEqual(
            ir_contract.Contract((), (), frozenset(("runs_python", "reads")),
                                 frozenset(("next", "raise")), confined=(
                ("runs_python", frozenset(("raise",))),)).continuing_effects(),
            frozenset(("reads",)))

    def test_the_representation_table(self):
        # _TO_BOOL_INT: an exact int whose one encoding tested false is an
        # object
        source = """
            op(_OP, (value: consumed exact(%s) -- res: owned exact(PyBool_Type), v: moved(value)))
                effects(reads) exits(next)
            {
                bool zero;
                if (%s(value)) {
                    zero = 0;
                }
                else {
                    zero = _PyLong_IsZero((PyLongObject *)PyValue_AsKnownObjectBorrow(value));
                }
                res = zero ? PyValue_False : PyValue_True;
                v = value;
                DEAD(value);
            }
        """
        self.verify(source % ("PyLong_Type", "PyValue_IsInlineInt"))
        # The false test of an encoding the type lacks says nothing
        self.refuses(source % ("PyLong_Type", "PyValue_IsInlineFloat"),
                     r"passes `value` to PyValue_AsKnownObjectBorrow, which requires "
                     r".*object")
        # Two types, two encodings: one tested false leaves the other
        self.refuses(source % ("PyLong_Type|PyFloat_Type", "PyValue_IsInlineInt"),
                     r"passes `value` to PyValue_AsKnownObjectBorrow, which requires "
                     r".*object")
        # exact(T) and inline: T's one encoding
        self.verify("""
            op(_OP, (value: exact(PyLong_Type) -- value)) effects() exits(next, miss) {
                EXIT_IF(!PyValue_IsInline(value));
                int64_t n = PyValue_InlineInt64(value);
            }
        """.replace("value)) effects()", "value: inline_int)) effects()"))
        # The table against the tag definitions: each encoding's TEST row
        # tests it, and pycore_value.h's table of words has one region of
        # each beside the pointer space
        for encoding, (test, _) in ir_contract.ENCODINGS.items():
            with self.subTest(encoding=encoding):
                # A partition: true, the encoding; false, the rest
                row = self.table[test]
                self.assertEqual(row.result.tests, ((
                    frozenset({ir_contract.VFact(encoding)}),
                    frozenset({ir_contract.VFact(ir_contract.EXCLUDES, encoding)})),))
        header = Path(__file__).parents[2] / "Include/internal/pycore_value.h"
        words = re.findall(r"^\s+0x[0-9A-F]{4}(?:-0x[0-9A-F]{4})?\s+\S.*?\s{2,}(\w.*)$",
                           header.read_text(), re.M)
        regions = {"index": "tagged_index", "inline float": "inline_float",
                   "inline int51": "inline_int"}
        found = {regions[w.split(" (")[0].split(",")[0]] for w in words
                 if w.split(" (")[0].split(",")[0] in regions}
        self.assertEqual(found, set(ir_contract.ENCODINGS))

    def test_is_object_is_a_partition(self):
        # PyValue_IsObject: true, an object; false, the value's kinds of
        # word but object.  _UNPACK_SEQUENCE drops a number on the false arm
        self.verify("""
            op(_OP, (seq: consumed -- unused[oparg], top[0]: owned))
                effects(runs_python) exits(next, raise)
            {
                if (!PyValue_IsObject(seq)) {
                    _PyErr_Format(tstate, PyExc_TypeError, "cannot unpack", PyValue_TYPE(seq)->tp_name);
                    DEAD(seq);
                    ERROR_IF(true);
                }
                PyObject *seq_o = PyValue_AsKnownObjectSteal(seq);
                int res = _PyEval_UnpackIterableValue(tstate, seq_o, oparg, -1, top);
                ERROR_IF(res == 0);
            }
        """)
        # A cursor is a tagged index where it is no object: not a number
        source = """
            op(_OP, (iter: %s -- iter)) effects() exits(next) {
                if (!PyValue_IsObject(iter)) {
                    int64_t n = PyValue_InlineInt64(iter);
                }
            }
        """
        self.verify(source % "exact(PyLong_Type)")
        self.refuses(source % "encoding(object|tagged_index)",
                     r"passes `iter` to PyValue_InlineInt64, which requires")

    def test_a_union_of_encodings_with_null(self):
        # _GET_ITER's index_or_null: a tagged index, or NULL
        source = """
            op(_OP, (-- index_or_null: owned %s)) effects() exits(next) {
                if (oparg) {
                    index_or_null = PyValue_TagIndex(0);
                }
                else {
                    index_or_null = %s;
                }
            }
        """
        self.verify(source % ("encoding(tagged_index|null)", "PyValue_NULL"))
        self.verify(source % ("encoding(tagged_index|null)", "PyValue_TagIndex(1)"))
        self.refuses(source % ("tagged_index", "PyValue_NULL"),
                     r"`index_or_null` may be NULL from PyValue_NULL")
        self.refuses(source % ("encoding(tagged_index|null)", "PyValue_FromDoubleNotNaN(1.0)"),
                     r"`index_or_null` is declared encoding\(null\|tagged_index\)|"
                     r"`index_or_null` is declared encoding\(tagged_index\|null\)")
        self.refuses(source % ("encoding(tagged_index|spam)", "PyValue_NULL"),
                     r"spam is no kind of word")

    def test_a_frame_switch_is_at_its_token(self):
        # _SEND_GEN_FRAME: the guards before IR_ENTER_GENERATOR restart the
        # word; the switch is where the row that names it is called
        self.verify("""
            op(_OP, (receiver: object, null: nullable, v: taken -- receiver, null, gen_frame: owned))
                effects(runs_python, switches_frame) exits(miss, enter, raise)
            {
                PyGenObject *gen = (PyGenObject *)PyValue_AsKnownObjectBorrow(receiver);
                EXIT_IF(Py_TYPE(gen) != &PyGen_Type, "not a generator");
                IR_ENTER_GENERATOR(gen, _PY_IR_SEND);
            }
        """)
        # Nothing follows the switch: a guard there is refused, not skipped
        source = """
            op(_OP, (callable: taken, self_or_null: taken, args[oparg]: taken -- new_frame: owned))
                effects(runs_python, switches_frame) exits(miss, next, raise, enter)
            {
                %s
                IR_INVOKE_CALL();
                %s
            }
        """
        guard = 'EXIT_IF(oparg > 3, "too many arguments");'
        self.verify(source % (guard, ""))
        self.refuses(source % ("", guard),
                     r"this statement follows IR_INVOKE_CALL, which never returns")

    def test_facts_of_an_inputs_field(self):
        # _CHECK_METHOD_VERSION: facts of callable.im_func, a path from an
        # input through declared FIELD rows
        source = """
            op(_OP, (func_version/2, callable: object, null, unused[oparg]
                     -- callable: exact(PyMethod_Type), null, unused[oparg]))
                effects(%s) exits(next, miss)
                %s
            {
                PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);
                EXIT_IF(Py_TYPE(callable_o) != &PyMethod_Type);
                PyObject *func = ((PyMethodObject *)callable_o)->im_func;
                EXIT_IF(!PyFunction_Check(func));
                EXIT_IF(((PyFunctionObject *)func)->func_version != func_version);
                %s
            }
        """
        path = "paths(callable.im_func: exact(PyFunction_Type) func_version(func_version))"
        self.verify(source % ("reads", path, ""))
        # Established and not declared
        self.refuses(source % ("reads", "", ""),
                     r"the guards establish .*func_version.* of the path `callable.im_func`, "
                     r"which paths\(...\) does not declare")
        # A write may change a field on the path: its facts die
        self.refuses(source % ("reads, writes", path, "spam_write(callable_o);"),
                     r"`callable.im_func` is declared .*which nothing establishes")
        # A path steps through declared fields only
        self.refuses(source % ("reads", "paths(callable.im_spam: null)", ""),
                     r"steps through `im_spam`, which no FIELD row declares")
        self.refuses(source % ("reads", "paths(spam.im_func: null)", ""),
                     r"starts at `spam`, which is no input")
        # The NULL test of a pointer field along the path
        self.verify("""
            op(_OP, (callable: object -- callable: exact(PyMethod_Type)))
                effects(reads) exits(next, miss) paths(callable.im_weakreflist: null)
            {
                PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);
                EXIT_IF(Py_TYPE(callable_o) != &PyMethod_Type);
                EXIT_IF(((PyMethodObject *)callable_o)->im_weakreflist != NULL);
            }
        """)

    def test_calling_conventions(self):
        # _GUARD_CALLABLE_METHOD_DESCRIPTOR_FAST: a guard tests a local
        # bound to the reader against _PyCallConvention's members, and the
        # members it tests are a fact of the path callable.d_method
        members = "_Py_CALL_FASTCALL|_Py_CALL_VALUE|_Py_CALL_VALUE_NOARGS|_Py_CALL_VALUE_O"
        source = """
            op(_OP, (callable: object, self_or_null, args[oparg]
                     -- callable: exact(PyMethodDescr_Type), self_or_null, args[oparg]))
                effects(reads) exits(next, miss)
                %s
            {
                PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);
                PyMethodDescrObject *method = (PyMethodDescrObject *)callable_o;
                EXIT_IF(Py_TYPE(callable_o) != &PyMethodDescr_Type);
                _PyCallConvention conv = _PyMethodDef_Convention(method->d_method);
                %s
                int n = (int)IR_C(ir_word) - IR_CALL_FIRST();
                EXIT_IF(conv == _Py_CALL_VALUE_O && n != 2,
                        "the arguments do not fit the value form's fixed arity");
            }
        """
        guard = ("EXIT_IF(conv != _Py_CALL_FASTCALL && conv != _Py_CALL_VALUE\n"
                 "        && conv != _Py_CALL_VALUE_NOARGS && conv != _Py_CALL_VALUE_O);")
        self.verify(source % (f"paths(callable.d_method: convention({members}))", guard))
        # The members are a set: their order in the declaration is free
        self.verify(source % ("paths(callable.d_method: convention(_Py_CALL_VALUE_O|"
                              "_Py_CALL_VALUE_NOARGS|_Py_CALL_VALUE|_Py_CALL_FASTCALL))",
                              guard))
        one = "EXIT_IF(conv != _Py_CALL_O);"
        self.verify(source % ("paths(callable.d_method: convention(_Py_CALL_O))", one))
        # A declared union that differs from the members tested
        self.refuses(source % ("paths(callable.d_method: convention(_Py_CALL_FASTCALL))",
                               guard),
                     r"`callable.d_method` is declared convention\(_Py_CALL_FASTCALL\), "
                     "which nothing establishes")
        self.refuses(source % ("paths(callable.d_method: convention(_Py_CALL_O|"
                               "_Py_CALL_NOARGS))", one),
                     r"`callable.d_method` is declared convention\(_Py_CALL_NOARGS\|"
                     r"_Py_CALL_O\), which nothing establishes")
        # Established and not declared
        self.refuses(source % ("", one),
                     r"the guards establish convention\(_Py_CALL_O\) of the path "
                     r"`callable.d_method`, which paths\(...\) does not declare")
        # A member outside the enum
        self.refuses(source % ("paths(callable.d_method: convention(_Py_CALL_SPAM))", one),
                     r"_Py_CALL_SPAM is no member of _PyCallConvention")
        # The forms the guard may not take: `||`, a mask, another variable or
        # value, the reader inline, and the flags themselves
        path = "paths(callable.d_method: convention(_Py_CALL_O))"
        for body, message in [
            ("EXIT_IF(conv == _Py_CALL_NOARGS || conv == _Py_CALL_VALUE);",
             r"joins convention tests with `\|\|`"),
            ("EXIT_IF((conv & METH_O) == 0);",
             r"is no comparison of a convention local with a member"),
            ("EXIT_IF(conv != METH_O);",
             r"`conv != METH_O` is no comparison of a convention local with a member"),
            ("int other = oparg;\nEXIT_IF(conv != other);",
             r"`conv != other` is no comparison of a convention local with a member"),
            ("EXIT_IF(_PyMethodDef_Convention(method->d_method) != _Py_CALL_O);",
             r"tests the call flags directly"),
            ("EXIT_IF((method->d_method->ml_flags & _Py_METH_CALL_FLAGS) != METH_O);",
             r"tests the call flags directly"),
            ("EXIT_IF(conv != _Py_CALL_O && conv == _Py_CALL_NOARGS);",
             r"mixes convention tests"),
        ]:
            with self.subTest(body=body):
                self.refuses(source % (path, body), message)

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

    def test_from_object_steal_moves_its_argument(self):
        # VALUE(moved(0)): the result is the argument's reference, so the
        # argument must hold an obligation, as _PyValue_Str's new reference
        # does; a pointer that a SCALAR row gives holds none
        source = """
            op(_OP, (x: borrowed -- r: owned)) effects(runs_python) exits(next, raise) {
                PyObject *o = %s;
                ERROR_IF(o == NULL);
                r = PyValue_FromObjectSteal(o);
            }
        """
        self.verify(source % "_PyValue_Str(x)")
        self.refuses(source % "_PyErr_Occurred(tstate)",
                     r"`_PyErr_Occurred \( tstate \)` is assigned to an output, but it is no "
                     r"row with a value result")

    def test_decref_inputs_after_a_consumption(self):
        # DECREF_INPUTS closes every input the generator has not killed, so
        # an input a row consumed is closed twice unless it is marked DEAD
        double = r"DECREF_INPUTS releases `value`, which is already consumed or moved"
        source = """
            op(_OP, (value: consumed%s -- res: moved(value))) effects(releases) exits(next) {
                PyValue r = %s;
                %sDECREF_INPUTS();
                res = r;
            }
        """
        self.refuses(source % ("", "TAKE(value)", ""), double)
        self.verify(source % ("", "TAKE(value)", "DEAD(value);\n"))
        # A borrowed view written in the argument passes the input's count
        source = source.replace("moved(value)", "owned")
        self.refuses(source % (" object", "SPAM_STEAL(PyValue_AsKnownObjectBorrow(value))",
                               ""), double)
        self.verify(source % (" object", "SPAM_STEAL(PyValue_AsKnownObjectBorrow(value))",
                              "DEAD(value);\n"))

    def test_an_unowned_input_is_not_assigned(self):
        # A pass-through, borrowed or closed input names its home, whose
        # word stays the caller's: a new value written there is owned by
        # no one (F16: `null_or_index = window[1]` in _FOR_ITER)
        source = """
            op(_OP, (x%s -- %s)) effects() exits(next) {
                %s = PyValue_None;
            }
        """
        self.refuses(source % ("", "x", "x"),
                     r"assigns `x`, an input declared a pass-through")
        self.refuses(source % (": borrowed", "", "x"),
                     r"assigns `x`, an input declared borrowed")
        # A copy in a local of its own may change
        self.verify(source % ("", "x", "PyValue y = x;\n                y"))

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

    def test_a_frame_switch_is_declared(self):
        # The switches_frame effect and the switch exits (enter, return,
        # yield) are the authority; the body's `frame =` is held to them and
        # never names which switch it is
        source = """
            op(_OP, (--)) effects(%s) exits(%s) {
                frame = tstate->current_frame;
            }
        """
        self.verify(source % ("switches_frame", "enter"))
        self.verify(source % ("switches_frame", "yield"))
        self.refuses(source % ("switches_frame", "next"),
                     r"declares switches_frame, but none of the exits enter, return, yield")
        self.refuses(source % ("reads", "next"), r"may switches_frame")
        self.refuses("""
            op(_OP, (--)) effects() exits(enter) {
                int x = 0;
            }
        """, r"declares exits\(enter\), but not the effect switches_frame")

    def test_a_redirect_is_its_own_exit(self):
        # _MONITOR_RESUME, _YIELD_VALUE_EVENT: after a callback moved the
        # frame, the word leaves for the word it chose (DISPATCH_REDIRECT),
        # which is neither next nor the jump to the word itself
        source = """
            op(_OP, (--)) effects(runs_python) exits(%s) {
                Py_ssize_t n = _PyValue_Size(PyValue_None);
                ERROR_IF(n < 0);
                if (frame->instr_ptr != this_instr) {
                    DISPATCH_REDIRECT();
                }
            }
        """
        self.verify(source % "next, raise, redirect")
        self.refuses(source % "next, raise",
                     r"declares exits\(next, raise\), but the body takes next, raise, redirect")

    def test_a_handler_that_never_runs_declares_abort(self):
        # CACHE, RESERVED, the stack-only opcodes, IR_OPERANDS: the body
        # ends the process (Py_FatalError), so abort is its one exit, and
        # its outputs are never made
        source = """
            op(_OP, (value1: consumed -- value2: owned)) effects() exits(%s) {
                Py_FatalError("Executing a stack-only instruction.");
            }
        """
        self.verify(source % "abort")
        self.refuses(source % "next",
                     r"declares exits\(next\), but the body takes abort")
        self.refuses("""
            op(_OP, (--)) effects() exits(abort) {
                int x = 0;
            }
        """, r"declares exits\(abort\), but the body takes next")

    def test_an_assumption_on_a_temporary_survives_a_reentry(self):
        # INSTRUMENTED_END_ASYNC_FOR: a temporary is replaced only by a
        # debugger's f_lineno jump, which redirects the frame: from an event
        # of this frame, in a uop that leaves by its redirect exit; from a
        # descendant, only while the word's call has left through `enter`.
        # On the next path the temporary holds what exception_unwind pushed
        source = """
            op(_EVENT, (--)) effects(runs_python) exits(next, %s) {
                %s
            }
            op(_USE, (x: taken assumed(object) by exception_unwind --))
                effects(releases) exits(next)
            {
                spam_dealloc(PyValue_AsKnownObjectBorrow(x));
                PyValue_CLOSE(x);
            }
            macro(OP) = _EVENT + _USE;
        """
        redirects = source % ("redirect", "SPAM_EVENT();")
        reenters = source % ("raise", "Py_ssize_t n = _PyValue_Size(PyValue_None);\n"
                             "                ERROR_IF(n < 0);")
        for text in (redirects, reenters):
            self.verify(text, temporaries=True)
            # An input that may be a local home: any reentry may have
            # replaced it (a finalizer writes locals through FrameLocalsProxy)
            self.refuses(text, r"`x`, which may have been replaced since the handler's "
                         r"entry, at _EVENT, which may reenter \(")
        # The real handler verifies with its window, and refuses without it
        inst = analysis.instructions["INSTRUMENTED_END_ASYNC_FOR"]
        ir_contract.verify_handler(inst, temporaries=True)
        with self.assertRaisesRegex(SyntaxError, r"_END_ASYNC_FOR assumes object by "
                                    r"exception_unwind of `exc_st`, which may have been "
                                    r"replaced since the handler's entry, at "
                                    r"_MONITOR_END_ASYNC_FOR"):
            ir_contract.verify_handler(inst)

    def test_a_fact_made_in_the_handler_survives_a_reentry(self):
        # R1, POP_JUMP_IF_FALSE_LIST: the bool _TO_BOOL_LIST makes is a
        # temporary, whose exact type no reentry (POP_TOP's close) replaces;
        # a fact of an input that may be in a local home dies there
        for name in ("POP_JUMP_IF_FALSE_LIST", "POP_JUMP_IF_TRUE_LIST"):
            with self.subTest(name):
                ir_contract.verify_handler(analysis.instructions[name])
        source = """
            op(_GUARD, (x: object -- x: exact(PyList_Type))) effects(reads) exits(next, miss) {
                EXIT_IF(!PyList_CheckExact(PyValue_AsKnownObjectBorrow(x)));
            }
            op(_MAKE, (x: consumed -- b: owned exact(PyBool_Type))) effects(releases) exits(next) {
                PyValue_CLOSE(x);
                b = PyValue_True;
            }
            op(_EVENT, (--)) effects(runs_python) exits(next, raise) {
                Py_ssize_t n = _PyValue_Size(PyValue_None);
                ERROR_IF(n < 0);
            }
            op(_NEED, (v: %s -- v)) effects() exits(next) {
            }
            macro(OP) = _GUARD + %s;
        """
        self.verify(source % ("exact(PyBool_Type)", "_MAKE + _EVENT + _NEED"))
        self.refuses(source % ("exact(PyList_Type)", "_EVENT + _NEED"),
                     r"_NEED requires exact\(PyList_Type\).* of `v`, which nothing before "
                     r"it establishes or which an effect since killed")

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

    def test_a_case_field_is_a_set_of_kinds_of_word(self):
        # Nonnull by default; `nullable` and a union with null both allow
        # NULL, and nullable is no more than that
        [case, _] = ir_contract._parse_result(
            "CASES(status<0 a(owned) b(owned nullable) c(inline encoding(tagged_index|null)) "
            "d(owned exact(PyLong_Type)) e(null), status>0 a(owned) b(owned) c(null) "
            "d(null) e(null))").cases
        kinds = {f: (spec.kinds, spec.nullable) for f, spec in case.values}
        self.assertEqual(kinds, {
            "a": ({"inline_float", "inline_int", "object"}, False),
            "b": ({"inline_float", "inline_int", "null", "object"}, True),
            "c": ({"null", "tagged_index"}, True),
            "d": ({"inline_int", "object"}, False),
            "e": ({"null"}, True)})

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
        # _PyValueIndexPair is a CASES result.  The uop's store of the next
        # index into its pass-through input is left out: an unowned input is
        # not assigned (F16; G0d1 gives the output a name of its own)
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
                                     frozenset({ir_contract.VFact("self", "absent")})),
                                    ("CALL_EXPANDED",
                                     frozenset({ir_contract.VFact("method", "expanded")}),
                                     frozenset({ir_contract.VFact("method", "whole")}))))
        self.assertIn('static_assert((CALL_SELF_PRESENT) != 0, '
                      '"call_state: CALL_SELF_PRESENT is no bit");',
                      ir_contract.component_checks(self.table))
        for bad, message in [
                ("STATE(spam, int, BITS(SPAM: null | self(absent)), INIT(SPAM_INIT))",
                 r"null is no fact of a handler's state"),
                ("STATE(spam, int, BITS(SPAM: self(maybe) | self(absent)), INIT(SPAM_INIT))",
                 r"self\(maybe\) is no fact of a handler's state"),
                ("STATE(spam, int, BITS(SPAM: method(bound) | method(whole)), INIT(SPAM_INIT))",
                 r"method\(bound\) is no fact of a handler's state"),
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
        # A declared write of call_state: self(absent) to self(present)
        source = """
            op(_OP, (x: object -- x: object)) effects(%s) exits(next)
                state(call_state: self(absent) -- call_state: %s)
            {
                assert(!(call_state & CALL_SELF_PRESENT));
                %s
            }
        """
        write = "call_state |= CALL_SELF_PRESENT;"
        self.verify(source % ("", "self(present)", write))
        self.refuses(source % ("", "self(absent)", write),
                     r"`call_state` is declared self\(absent\), which nothing establishes")
        # Each bit states its own facts, and a write kills only those of
        # the bits it writes: CALL_EXPANDED leaves self(absent)
        self.verify(source % ("", "self(absent) method(expanded)",
                              "call_state |= CALL_EXPANDED;"))
        self.verify(source % ("", "self(present) method(expanded)",
                              "call_state |= CALL_SELF_PRESENT | CALL_EXPANDED;"))
        self.refuses(source % ("", "self(absent)", "call_state |= CALL_EXPANDED;"),
                     r"the body establishes method\(expanded\) of `call_state`, which the "
                     r"uop does not declare")
        # A bit that the STATE row does not list states nothing: refused
        self.refuses(source % ("", "self(absent)", "call_state |= CALL_KWARGS_PRESENT;"),
                     r"writes `CALL_KWARGS_PRESENT`, which is no bit of `call_state` "
                     r"\(CALL_SELF_PRESENT, CALL_EXPANDED\)")
        self.refuses(source % ("", "self(absent)", "call_state &= ~CALL_KWARGS_PRESENT;"),
                     r"writes `CALL_KWARGS_PRESENT`, which is no bit of `call_state`")
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
        # The home is overwritten on one path only: there `tmp` takes its
        # obligation (a home move), and the paths disagree
        self.refuses(source % "if (oparg) { GETLOCAL(oparg) = value; } else { "
                              "GETLOCAL(oparg + 1) = value; }",
                     r"`tmp` is owned on one path and not on another where they join")
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
            "value", "owned", ir_contract.HOME_SOURCE, words=ir_contract.ORDINARY_WORDS))
        self.assertTrue(contract.outputs[0].nullable)

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

    def test_a_switch_exit_needs_a_switch(self):
        # A declared switch exit names how the body's frame switch leaves;
        # a body that does not switch completes, whatever it declares
        source = """
            op(_RET, (--)) effects(switches_frame) exits(%s) {
                int x = 0;
            }
        """
        self.refuses(source % "return",
                     r"declares exits\(return\), but the body takes next")

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
            op(_GUARD, (x: nullable -- x: %s)) effects() exits(next, deopt) {
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


class IteratorRowTests(unittest.TestCase):
    """The iterator rows (G0c batch 5) in the interpreter: the ownership,
    failure and reentry claims that _FOR_ITER, _FOR_ITER_VIRTUAL and
    _GET_ITER rely on.  Each loop runs often enough to be specialized"""

    ROUNDS = 40

    def test_a_returned_value_is_closed_once(self):
        # _PyForIter_VirtualIteratorNextInline gives {0, value} with an
        # owned value when an iterator returns one; IR_COMPLETE_STEP closes it
        finalized = []

        class Value:
            def __del__(self):
                finalized.append(1)

        def source():
            yield 1
            return Value()

        def run():
            for _ in source():
                pass

        for _ in range(self.ROUNDS):
            run()
        self.assertEqual(len(finalized), self.ROUNDS)

    def test_an_uniterable_is_closed_on_failure(self):
        # _PyEval_GetIter consumes the iterable on its failure path
        finalized = []

        class Plain:
            def __del__(self):
                finalized.append(1)

        def run():
            for _ in Plain():
                pass

        for _ in range(self.ROUNDS):
            with self.assertRaises(TypeError):
                run()
        self.assertEqual(len(finalized), self.ROUNDS)

    def test_exhaustion_reports_the_stop_iteration_to_monitoring(self):
        # _PyForIter_StopIsExhaustion runs the RAISE callbacks (runs_python)
        # and clears the exception
        class Counter:
            def __init__(self):
                self.n = 0

            def __iter__(self):
                return self

            def __next__(self):
                self.n += 1
                if self.n > 3:
                    raise StopIteration
                return self.n

        events = []
        tool = sys.monitoring.PROFILER_ID

        def run():
            return [x for x in Counter()]

        for _ in range(self.ROUNDS):
            run()
        sys.monitoring.use_tool_id(tool, "test_ir_contracts")
        self.addCleanup(sys.monitoring.free_tool_id, tool)
        sys.monitoring.register_callback(
            tool, sys.monitoring.events.RAISE,
            lambda code, offset, exc: events.append(type(exc)))
        sys.monitoring.set_events(tool, sys.monitoring.events.RAISE)
        self.addCleanup(sys.monitoring.set_events, tool, 0)
        self.assertEqual(run(), [1, 2, 3])
        self.assertIn(StopIteration, events)
        self.assertIsNone(sys.exception())

    def test_an_error_other_than_stop_iteration_propagates(self):
        class Fails:
            def __iter__(self):
                return self

            def __next__(self):
                raise ZeroDivisionError

        def run():
            for _ in Fails():
                pass

        for _ in range(self.ROUNDS):
            with self.assertRaises(ZeroDivisionError):
                run()

    def test_the_cursor_follows_the_sequence(self):
        # _ITER_NEXT_LIST and _ITER_NEXT_TUPLE advance their cursor in place;
        # the list is read at each step, so growth is seen
        def over_list(items):
            seen = []
            for x in items:
                seen.append(x)
                if len(items) < 6:
                    items.append(len(items))
            return seen

        def over_tuple(items):
            return [x for x in items]

        for _ in range(self.ROUNDS):
            self.assertEqual(over_list([0, 1]), [0, 1, 2, 3, 4, 5])
            self.assertEqual(over_tuple((7, 8, 9)), [7, 8, 9])
            self.assertEqual(over_tuple(()), [])


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


class Counted:
    """An object whose finalizer counts: a stolen owner that a failure path
    forgot to close shows as a missing finalization, and one it closed
    twice as a crash"""
    made = 0
    freed = 0

    def __init__(self):
        type(self).made += 1

    def __del__(self):
        type(self).freed += 1


class ContainerRowsRunTest(unittest.TestCase):
    """The batch 3 rows' ownership claims, run through the handlers that
    rely on them (the rows' evidence comments in pycore_ir_components.h):
    _PyDict_SetValue_Take2 and _PySet_AddValueTakeRef consume their owners
    on failure, _PyEval_StoreSlice and _PyEval_BinarySlice borrow theirs,
    and _PyEval_UnpackIterableValue consumes the sequence and closes the
    values it wrote when it fails"""

    def setUp(self):
        Counted.made = Counted.freed = 0

    def check_balanced(self):
        import gc
        gc.collect()
        self.assertEqual(Counted.made, Counted.freed)

    def test_dict_store_with_an_unhashable_key_closes_both_owners(self):
        def f():
            d = {}
            d[[]] = Counted()
        for _ in range(3):
            with self.assertRaises(TypeError):
                f()
        self.check_balanced()

    def test_map_add_with_an_unhashable_key_closes_both_owners(self):
        def f():
            return {[]: Counted() for _ in range(1)}
        for _ in range(3):
            with self.assertRaises(TypeError):
                f()
        self.check_balanced()

    def test_set_add_with_an_unhashable_element_closes_the_owner(self):
        def f():
            return {[Counted()] for _ in range(1)}
        for _ in range(3):
            with self.assertRaises(TypeError):
                f()
        self.check_balanced()

    def test_replaced_dict_value_is_finalized_by_the_store(self):
        order = []

        class Old:
            def __del__(self):
                order.append("old")

        def f():
            d = {1: Old()}
            d[1] = 2
            order.append("stored")
        f()
        self.assertEqual(order, ["old", "stored"])

    def test_failed_unpack_closes_the_sequence_and_the_values(self):
        def f():
            a, b = iter([Counted(), Counted(), Counted()])
        for _ in range(3):
            with self.assertRaises(ValueError):
                f()
        self.check_balanced()

    def test_unpack_of_a_non_iterable_closes_the_value(self):
        def f(x):
            a, b = x
        for _ in range(3):
            with self.assertRaises(TypeError):
                f(Counted())
        for _ in range(3):
            with self.assertRaises(TypeError):
                f(7)
        self.check_balanced()

    def test_slice_operations_borrow_their_operands(self):
        c = Counted()

        def f(seq, lo, hi):
            return seq[lo:hi]

        def g(seq, lo, hi, v):
            seq[lo:hi] = v

        for _ in range(3):
            self.assertEqual(f([1, 2, 3], 1, None), [2, 3])
            self.assertEqual(f("abc", None, 2), "ab")
            with self.assertRaises(TypeError):
                f([1, 2, 3], c, 2)
            with self.assertRaises(TypeError):
                g([1, 2, 3], c, 2, [])
            with self.assertRaises(TypeError):
                g([1, 2, 3], 0, 1, Counted())
        del c
        self.check_balanced()

    def test_build_slice_takes_none_for_an_absent_step(self):
        def f(a, b):
            return a[1:b], a[::b]
        self.assertEqual(f([1, 2, 3], 3), ([2, 3], [1]))
        lo = Counted()
        s = slice(lo, None)
        self.assertIs(s.start, lo)
        self.assertIsNone(s.step)
        del lo, s
        self.check_balanced()

    def test_dict_merge_and_update_errors_close_the_operand(self):
        def merge(x):
            return {**x}

        def update(x):
            return {1: 2, **x}

        for g in (merge, update):
            for _ in range(3):
                with self.assertRaises(TypeError):
                    g(Counted())
                with self.assertRaises(TypeError):
                    g(5)
        self.check_balanced()


WARM = 200      # enough executions for every site to specialize and compile


class ObjectRowTests(unittest.TestCase):
    """The names, globals, cells, imports, super and attribute rows of
    Include/internal/pycore_ir_components.h (G0c batch 4), and the claims
    that their uops rely on: what a helper closes, what a failure leaves
    behind, and what a guard's miss does to the word it restarts"""

    def test_columns_of_the_reviewed_rows(self):
        table = ir_contract.rows()
        # The helpers read these arguments and never close them: the uops
        # close their inputs (DECREF_INPUTS) after the call
        self.assertEqual(table["value_import_level"].args[0].mode, "borrowed")
        self.assertEqual(table["super_call"].args[2].mode, "borrowed")
        # A new reference, or NULL with an exception
        for name in ("_PyEval_ImportName", "_PyEval_LazyImportName",
                     "_PyEval_LazyImportFrom", "value_load_global", "_PyEval_LoadName"):
            with self.subTest(row=name):
                self.assertEqual(table[name].result.shape, "VALUE_OR_NULL")
        # An empty cell is NULL with no exception: the caller says what it means
        self.assertEqual(table["_PyCell_GetValue"].result.shape, "VALUE_OR_NULL")
        # The cache word is never NULL, and the generator asserts it
        assumed = table["_PyValueGuarded_Load"].result.assumed
        self.assertEqual([(a.fact.kind, a.by) for a in assumed], [("nonnull", "specializer")])

    def test_empty_cell_raises_name_error(self):
        def outer():
            def inner():
                return x
            return inner
            x = 1
        inner = outer()
        for _ in range(WARM):
            with self.assertRaisesRegex(NameError, "free variable 'x'"):
                inner()

    def test_delete_of_an_unbound_cell(self):
        def f():
            x = 1
            def g():
                return x
            del x
            del x
        for _ in range(WARM):
            with self.assertRaises(UnboundLocalError):
                f()

    def test_store_and_delete_global_close_their_value_once(self):
        class Value:
            pass
        namespace = {}
        value = Value()
        ref = weakref.ref(value)
        namespace["value"] = value
        code = compile("global g\ng = value\ndel g\ng = value\n", "<test>", "exec")
        for _ in range(WARM):
            exec(code, namespace)
        del namespace["g"], namespace["value"], value
        support.gc_collect()
        self.assertIsNone(ref())
        with self.assertRaises(NameError):
            exec(compile("global g\ndel g\n", "<test>", "exec"), namespace)
        self.assertNotIn("g", namespace)

    def test_global_miss_raises_name_error(self):
        namespace = {}
        code = compile("def f():\n    return no_such_name\n", "<test>", "exec")
        exec(code, namespace)
        for _ in range(WARM):
            with self.assertRaisesRegex(NameError, "no_such_name"):
                namespace["f"]()
        namespace["no_such_name"] = 1
        for _ in range(WARM):
            self.assertEqual(namespace["f"](), 1)
        del namespace["no_such_name"]
        with self.assertRaises(NameError):
            namespace["f"]()

    def test_builtin_is_shadowed_after_specialization(self):
        namespace = {}
        exec("def f():\n    return len\n", namespace)
        for _ in range(WARM):
            self.assertIs(namespace["f"](), len)
        namespace["len"] = "shadow"
        self.assertEqual(namespace["f"](), "shadow")
        del namespace["len"]
        self.assertIs(namespace["f"](), len)

    def test_class_namespace_load_in_an_annotation_scope(self):
        class C:
            x = 7
            type A = x
        for _ in range(WARM):
            self.assertEqual(C.A.__value__, 7)

    def test_import_level_and_fromlist_are_closed_once(self):
        namespace = {"__name__": "test.test_ir_contracts", "__package__": "test"}
        code = compile("from . import support\nfrom os import path\n", "<test>", "exec")
        for _ in range(WARM):
            exec(code, namespace)
        self.assertIs(namespace["support"], support)
        self.assertIs(namespace["path"], os.path)
        with self.assertRaises(ImportError):
            exec(compile("from os import no_such_name\n", "<test>", "exec"), {})

    def test_super_call_leaves_its_receiver_alone(self):
        class A:
            def f(self):
                return 1
        class B(A):
            def f(self):
                return super().f() + 1
            def g(self):
                return super().missing
        b = B()
        ref = weakref.ref(b)
        for _ in range(WARM):
            self.assertEqual(b.f(), 2)
            with self.assertRaises(AttributeError):
                b.g()
        del b
        support.gc_collect()
        self.assertIsNone(ref())

    def test_slot_load_deoptimizes_when_the_slot_empties(self):
        class S:
            __slots__ = ("x",)
        s = S()
        s.x = 1
        def get(o):
            return o.x
        def put(o, v):
            o.x = v
        for _ in range(WARM):
            put(s, 2)
            self.assertEqual(get(s), 2)
        del s.x
        with self.assertRaises(AttributeError):
            get(s)
        put(s, 3)
        self.assertEqual(get(s), 3)

    def test_dict_attribute_with_a_hint(self):
        class D:
            pass
        d = D()
        for i in range(40):
            setattr(d, f"a{i}", i)      # a dict with many keys
        def get(o):
            return o.a30
        def put(o, v):
            o.a30 = v
        for i in range(WARM):
            put(d, i)
            self.assertEqual(get(d), i)
        del d.a30
        with self.assertRaises(AttributeError):
            get(d)
        d2 = D()
        d2.a30 = "other"
        self.assertEqual(get(d2), "other")

    def test_class_attribute_follows_the_class_version(self):
        class K:
            v = 1
        def get(k):
            return k.v
        for _ in range(WARM):
            self.assertEqual(get(K), 1)
        K.v = 2
        self.assertEqual(get(K), 2)
        del K.v
        with self.assertRaises(AttributeError):
            get(K)

    def test_property_and_getattribute_frames(self):
        class P:
            @property
            def p(self):
                return 5
        class G:
            def __getattribute__(self, name):
                return name
        def get_p(o):
            return o.p
        def get_g(o):
            return o.anything
        p, g = P(), G()
        refs = weakref.ref(p), weakref.ref(g)
        for _ in range(WARM):
            self.assertEqual(get_p(p), 5)
            self.assertEqual(get_g(g), "anything")
        P.p = property(lambda self: 6)
        self.assertEqual(get_p(p), 6)
        del p, g
        support.gc_collect()
        self.assertEqual([r() for r in refs], [None, None])


SEND_CLAUSE = """        boundary(SEND) at(last) yields(delegated) completion(send) completes(
            RETURN: exits(return, raise),
            STORE_LOCAL, DISCARD, HOME, STORE_FAST, POP_TOP: exits(branch, raise));"""
SEND_GEN_CLAUSE = SEND_CLAUSE.replace("boundary(SEND) at(last)",
                                      "boundary(SEND_GEN) at(enter)")
CLEANUP_CLAUSE = """        boundary(CLEANUP_THROW) at(last) completion(throw) completes(
            RETURN: exits(return, raise),
            STORE_LOCAL, DISCARD, HOME, STORE_FAST, POP_TOP: exits(branch, raise));"""


def expand_variant(analysis, formats, name):
    "Generate the generator-boundary variant name to no output; its emitter"
    insts = ir_generator.handler_instructions(analysis)
    out = ir_generator.CWriter.null()
    emitter = ir_generator.IREmitter(out, analysis, formats)
    emitter.handler_format = formats[name]
    emitter.allowed_error_checkpoints = None
    ir_generator.generate_handler(name, insts[name], formats[name], emitter, out)
    return emitter


def expand_variants(analysis):
    "Generate each generator-boundary variant to no output"
    formats = ir_generator.compute_formats(analysis)
    for name in ir_generator.generator_boundaries(analysis):
        expand_variant(analysis, formats, name)


class BoundaryTests(unittest.TestCase):
    """The generator-boundary variants (*_XFER_*): their records come from
    the boundary(...) clauses, and their expansions are checked against
    them (g3/design.md section 7)"""

    def test_variants_come_from_the_clauses(self):
        boundaries = ir_generator.generator_boundaries(analysis)
        sources = {b.base for b in boundaries.values()}
        self.assertEqual(sources, {
            "SEND", "SEND_GEN", "SEND_VIRTUAL", "SEND_ASYNC_GEN", "CLEANUP_THROW",
            "YIELD_VALUE", "YIELD_DELEGATED", "INSTRUMENTED_YIELD_VALUE",
            "INSTRUMENTED_YIELD_DELEGATED"})
        self.assertEqual(len(boundaries), 5 * 6 + 4 * 2)
        self.assertEqual(boundaries["SEND_GEN_XFER_RETURN"].at, "enter")
        self.assertEqual(boundaries["SEND_VIRTUAL_XFER_RETURN"].exits, ("return", "raise"))
        self.assertEqual(boundaries["CLEANUP_THROW_XFER_HOME"].yielded, "NONE")
        self.assertEqual(boundaries["YIELD_VALUE_XFER_STORE_LOCAL"].resume, "STORE_LOCAL")

    def test_records_take_the_declared_exits(self):
        def kinds(name):
            return [t.kind for t in records[name].uops[-1].transfers]
        for source in ("SEND", "SEND_VIRTUAL", "SEND_ASYNC_GEN"):
            with self.subTest(source=source):
                returning, storing = kinds(f"{source}_XFER_RETURN"), kinds(f"{source}_XFER_POP_TOP")
                self.assertEqual(returning[-1], "YIELD")
                self.assertIn("RETURN", returning)
                self.assertNotIn("BRANCH", returning)
                self.assertEqual(storing[-1], "YIELD")
                self.assertIn("BRANCH", storing)
                self.assertNotIn("RETURN", storing)
        self.assertIn("RETURN", kinds("CLEANUP_THROW_XFER_RETURN"))
        self.assertNotIn("YIELD", kinds("CLEANUP_THROW_XFER_RETURN"))
        # SEND_GEN yields and completes in the frame it enters
        self.assertEqual(kinds("SEND_GEN_XFER_RETURN"), kinds("SEND_GEN"))

    def test_expansions_agree_with_the_clauses(self):
        with cases.handle_stderr():
            expand_variants(analysis)

    def refuses(self, changes, message):
        text = BYTECODES.read_text()
        for old, new in changes:
            self.assertEqual(text.count(old), 1, old)
            text = text.replace(old, new)
        fd, filename = tempfile.mkstemp(suffix=".c")
        os.close(fd)
        self.addCleanup(os.remove, filename)
        with open(filename, "w") as f:
            f.write(text)
        with self.assertRaisesRegex(SyntaxError, message), cases.handle_stderr():
            expand_variants(cases.analyzer.analyze_files([filename]))

    def test_expansion_reaches_an_omitted_return(self):
        # The RETURN arm of the completion routine returns from the frame
        self.refuses([(SEND_CLAUSE, SEND_CLAUSE.replace("RETURN: exits(return, raise)",
                                                        "RETURN: exits(branch, raise)")),
                      (SEND_GEN_CLAUSE, SEND_GEN_CLAUSE.replace(
                          "RETURN: exits(return, raise)", "RETURN: exits(branch, raise)"))],
                     r"SEND_\w*XFER_RETURN: its expansion reaches \['return'\], "
                     r"which its boundary\(\.\.\.\) clause omits")
        self.refuses([(CLEANUP_CLAUSE, CLEANUP_CLAUSE.replace(
                          "RETURN: exits(return, raise)", "RETURN: exits(raise)"))],
                     r"CLEANUP_THROW_XFER_RETURN: its expansion reaches \['return'\]")

    def test_expansion_reaches_an_omitted_yield(self):
        self.refuses([(SEND_CLAUSE, SEND_CLAUSE.replace(" yields(delegated)", "")),
                      (SEND_GEN_CLAUSE, SEND_GEN_CLAUSE.replace(" yields(delegated)", ""))],
                     r"SEND_\w*XFER_\w+: its expansion reaches `yield`, which its "
                     r"boundary\(\.\.\.\) clause omits")

    def test_clause_declares_an_unreached_exit(self):
        self.refuses([(CLEANUP_CLAUSE, CLEANUP_CLAUSE.replace(
                          "POP_TOP: exits(branch, raise)", "POP_TOP: exits(branch, return, raise)"))],
                     r"CLEANUP_THROW_XFER_\w+: its boundary\(\.\.\.\) clause declares "
                     r"\['return'\], which its expansion never reaches")
        self.refuses([(CLEANUP_CLAUSE, CLEANUP_CLAUSE.replace("at(last)",
                                                              "at(last) yields(delegated)"))],
                     r"_CLEANUP_THROW: a delegated yield needs the departing uop's fall-through")

    def test_departure_is_declared(self):
        # No default: a clause states where its variants depart
        self.refuses([(CLEANUP_CLAUSE, CLEANUP_CLAUSE.replace(" at(last)", ""))],
                     r"boundary\(CLEANUP_THROW\) needs at\(last\) or at\(enter\)")
        # SEND_GEN's last uop only enters
        self.refuses([(SEND_GEN_CLAUSE, SEND_GEN_CLAUSE.replace("at(enter)", "at(last)"))],
                     r"_PUSH_FRAME: a delegated yield needs the departing uop's fall-through")
        # A native send yields and completes in its own uops
        self.refuses([(SEND_CLAUSE, SEND_CLAUSE.replace("at(last)", "at(enter)"))],
                     r"SEND\w*_XFER_\w+: departs at\(enter\), and its expansion reaches")

    def test_completion_is_declared(self):
        # completion(...) names the completion step's kind; GEN_THROWING in
        # the completed table comes from completion(throw)
        boundaries = ir_generator.generator_boundaries(analysis)
        self.assertEqual(boundaries["CLEANUP_THROW_XFER_POP_TOP"].completion, "throw")
        self.assertEqual(boundaries["SEND_GEN_XFER_POP_TOP"].completion, "send")
        self.refuses([(CLEANUP_CLAUSE, CLEANUP_CLAUSE.replace("completion(throw)",
                                                              "completion(send)"))],
                     r"CLEANUP_THROW_XFER_\w+: completes with _PY_IR_SEND_THROW, and its "
                     r"boundary\(\.\.\.\) clause declares completion\(send\)")
        self.refuses([(CLEANUP_CLAUSE, CLEANUP_CLAUSE.replace(" completion(throw)", ""))],
                     r"boundary\(CLEANUP_THROW\): completes\(\.\.\.\) and "
                     r"completion\(send\) or completion\(throw\) go together")

    def test_completion_transfers_are_completion_sites(self):
        # The departing uop's branches (returns, in a RETURN variant) are the
        # completions that the body lowers; any other branch is refused
        formats = ir_generator.compute_formats(analysis)
        with cases.handle_stderr():
            for name in ("SEND_VIRTUAL_XFER_POP_TOP", "SEND_VIRTUAL_XFER_RETURN"):
                emitter = expand_variant(analysis, formats, name)
                record = records[name]
                boundary = record.generator_boundary
                completions = emitter.boundary_completions
                self.assertTrue(completions)
                ir_generator.check_boundary_expansion(
                    name, boundary, record, emitter.boundary_reached, completions,
                    record.instruction.where)
                with self.subTest(name=name), self.assertRaisesRegex(
                        SyntaxError, rf"{name}: _SEND_VIRTUAL's (branch|return) at line "
                                     r"\d+ is no generator completion"):
                    ir_generator.check_boundary_expansion(
                        name, boundary, record, emitter.boundary_reached, completions[1:],
                        record.instruction.where)

    def test_member_clause_differs_only_in_departure(self):
        self.refuses([(SEND_GEN_CLAUSE, SEND_GEN_CLAUSE.replace("HOME, ", ""))],
                     r"boundary\(SEND_GEN\) may differ from its family's "
                     r"boundary\(SEND\) only in at\(\.\.\.\)")
        self.refuses([(SEND_GEN_CLAUSE, SEND_GEN_CLAUSE.replace(" yields(delegated)", ""))],
                     r"boundary\(SEND_GEN\) may differ")


if __name__ == "__main__":
    unittest.main()
