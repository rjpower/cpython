"""The instruction contract of the register IR.

_opcode.get_ir_contract() exports _PyIR_Formats (Include/internal/pycore_ir.h),
which Tools/cases_generator/ir_metadata_generator.py writes from
Python/bytecodes.c.  The compiler passes, the assembler, dis and the
instrumentation read it.  Each row must agree with the stack effects that the
compiler uses, with the generator's analysis of the instruction, with the
feedback layouts and with sys.monitoring.
"""

import dis
import io
import opcode
import os
import re
import sys
import textwrap
import types
import unittest
from test import support
from test import test_tools
from test.support import import_helper, os_helper, script_helper

import _opcode

CONTRACT = _opcode.get_ir_contract()
ROWS = {h: row for h, row in enumerate(CONTRACT) if row is not None}
BY_NAME = {opcode.opname[h]: row for h, row in ROWS.items()}

SLOT_ROLES = (_opcode.IR_ROLE_LOCAL_IN, _opcode.IR_ROLE_LOCAL_OUT,
              _opcode.IR_ROLE_LOCAL_CLEAR, _opcode.IR_ROLE_LOCAL_MOVE,
              _opcode.IR_ROLE_CELL)
FIELD_LOCS = (_opcode.IR_LOC_A, _opcode.IR_LOC_B, _opcode.IR_LOC_C,
              _opcode.IR_LOC_D)

# The feedback entry of each family (Include/internal/pycore_feedback.h):
# its kind and size in bytes
FEEDBACK = {
    "POP_JUMP_IF_NONE": ("BRANCH", 16),
    "POP_JUMP_IF_NOT_NONE": ("BRANCH", 16),
    "POP_JUMP_IF_FALSE": ("BRANCH", 24),
    "POP_JUMP_IF_TRUE": ("BRANCH", 24),
    "TO_BOOL": ("UNARY", 24),
    "UNPACK_SEQUENCE": ("UNARY", 24),
    "FOR_ITER": ("UNARY", 24),
    "GET_ITER": ("UNARY", 24),
    "SEND": ("UNARY", 24),
    "BINARY_OP": ("BINARY", 32),
    "COMPARE_OP": ("BINARY", 32),
    "CONTAINS_OP": ("BINARY", 32),
    "STORE_SUBSCR": ("BINARY", 32),
    "LOAD_ATTR": ("ATTR", 48),
    "STORE_ATTR": ("ATTR", 48),
    "LOAD_SUPER_ATTR": ("ATTR", 48),
    "LOAD_GLOBAL": ("GLOBAL", 32),
    "CALL": ("CALL", 32),
    "CALL_KW": ("CALL", 32),
    "CALL_FUNCTION_EX": ("CALL", 32),
}
# Branches whose entries keep only the taken history
NOT_SPECIALIZING = {"POP_JUMP_IF_NONE", "POP_JUMP_IF_NOT_NONE"}

OPARGS = (0, 1, 2, 3, 7, 0x102)


def extent(e, oparg):
    "The slot count of extent e for oparg (_PyIR_Extent())"
    k, form = e
    if form == _opcode.IR_EXT_FIXED:
        return k
    if form == _opcode.IR_EXT_OPARG:
        return k + oparg
    if form == _opcode.IR_EXT_OPARG2:
        return k + 2 * oparg
    if form == _opcode.IR_EXT_BIT0:
        return k + (oparg & 1)
    if form == _opcode.IR_EXT_BYTES:
        return k + (oparg & 0xFF) + (oparg >> 8)
    raise ValueError(f"no extent form {form}")


def oparg_role(row):
    "The role of the field that holds the oparg, or None"
    if row.imm in FIELD_LOCS:
        return row.roles[FIELD_LOCS.index(row.imm)]
    return None


def names_with(bit, key="effects"):
    return {name for name, row in BY_NAME.items() if getattr(row, key) & bit}


class ContractTests(unittest.TestCase):

    def test_normal_physical_transfer(self):
        testapi = import_helper.import_module("_testinternalcapi")
        named = {**opcode.opmap, **opcode._specialized_opmap,
                 **opcode._ir_opmap}
        preserve = (0, -1)
        set_null = (1, -1)
        unknown = (2, -1)

        def transfer(name, fields, arg, edge, home, nl=2, limit=10):
            return testapi.ir_normal_physical_transfer(
                named[name], *fields, arg, edge, home, nl, limit)

        copy = (4, 2, 0, 0)
        self.assertEqual(transfer("COPY", copy, 2, 0, 6), (3, 4))
        self.assertEqual(transfer("COPY", copy, 2, 0, 4), preserve)
        self.assertEqual(transfer("COPY", copy, 2, 0, 7), preserve)
        self.assertEqual(transfer("COPY", copy, 2, 1, 6), unknown)
        self.assertEqual(transfer("COPY", (8, 2, 0, 0), 2, 0, 9), unknown)
        self.assertEqual(transfer("COPY", copy, 2**31 - 1, 0, 6), unknown)

        swap = (4, 3, 0, 0)
        self.assertEqual(transfer("SWAP", swap, 3, 0, 4), (3, 6))
        self.assertEqual(transfer("SWAP", swap, 3, 0, 6), (3, 4))
        self.assertEqual(transfer("SWAP", swap, 3, 0, 5), preserve)
        self.assertEqual(transfer("SWAP", (9, 3, 0, 0), 3, 0, 9), unknown)

        store_fast = (4, 0, 0, 0)
        self.assertEqual(transfer("STORE_FAST", store_fast, 0, 0, 4), set_null)
        self.assertEqual(transfer("STORE_FAST", store_fast, 0, 0, 5), preserve)

        pop_top = (4, 0, 0, 0)
        row = BY_NAME["POP_TOP"]
        self.assertEqual(row.roles, (_opcode.IR_ROLE_IN0,
                                     _opcode.IR_ROLE_NONE,
                                     _opcode.IR_ROLE_NONE,
                                     _opcode.IR_ROLE_NONE))
        self.assertEqual((extent(row.reads, 0), extent(row.writes, 0),
                          extent(row.through, 0)), (1, 0, 0))
        self.assertIsNone(row.twin)
        self.assertEqual(transfer("POP_TOP", pop_top, 0, 0, 4), set_null)
        self.assertEqual(transfer("POP_TOP", pop_top, 0, 0, 5), preserve)
        # Successive retirements retain each other's NULL facts.
        self.assertEqual(transfer("POP_TOP", (5, 0, 0, 0), 0, 0, 4), preserve)
        self.assertEqual(transfer("POP_TOP", (5, 0, 0, 0), 0, 0, 5), set_null)
        self.assertEqual(transfer("POP_TOP", (0, 0, 0, 0), 0, 0, 4), unknown)
        self.assertEqual(transfer("POP_TOP", (10, 0, 0, 0), 0, 0, 4), unknown)
        self.assertEqual(transfer("POP_TOP", pop_top, 0, 1, 4), unknown)

        for_iter = (6, 4, 0, 1)
        self.assertEqual(transfer("FOR_ITER", for_iter, 1, 1, 4), set_null)
        self.assertEqual(transfer("FOR_ITER", for_iter, 1, 1, 5), set_null)
        self.assertEqual(transfer("FOR_ITER", for_iter, 1, 0, 5), unknown)
        self.assertEqual(transfer("FOR_ITER", for_iter, 1, 0, 7), unknown)
        self.assertEqual(transfer("FOR_ITER", for_iter, 1, 1, 7), unknown)
        self.assertEqual(transfer("FOR_ITER", (7, 4, 0, 1), 1, 1, 4), unknown)
        self.assertEqual(transfer("FOR_ITER_RANGE", for_iter, 1, 1, 4), set_null)
        self.assertEqual(transfer("FOR_ITER_GEN", for_iter, 1, 1, 4), set_null)
        self.assertEqual(transfer("FOR_ITER_GEN", for_iter, 1, 0, 7), unknown)

        load_global = (5, 0, 0, 0)
        self.assertEqual(transfer("LOAD_GLOBAL", load_global, 0, 0, 6), preserve)
        self.assertEqual(transfer("LOAD_GLOBAL", load_global, 1, 0, 6), unknown)

    def test_call_normal_physical_transfer(self):
        testapi = import_helper.import_module("_testinternalcapi")
        named = {**opcode.opmap, **opcode._specialized_opmap,
                 **opcode._ir_opmap}
        # Pin the finite family audited for success-only window retirement.
        calls = {
            "CALL", "INSTRUMENTED_CALL", "CALL_BOUND_METHOD_EXACT_ARGS",
            "CALL_PY_EXACT_ARGS", "CALL_TYPE_1", "CALL_STR_1", "CALL_TUPLE_1",
            "CALL_BUILTIN_CLASS", "CALL_BUILTIN_O", "CALL_BUILTIN_FAST",
            "CALL_BUILTIN_FAST_WITH_KEYWORDS", "CALL_LEN", "CALL_ISINSTANCE",
            "CALL_LIST_APPEND", "CALL_METHOD_DESCRIPTOR_O",
            "CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS",
            "CALL_METHOD_DESCRIPTOR_NOARGS", "CALL_METHOD_DESCRIPTOR_FAST",
            "CALL_ALLOC_AND_ENTER_INIT", "CALL_PY_GENERAL",
            "CALL_BOUND_METHOD_GENERAL", "CALL_NON_PY_GENERAL",
        }
        legacy = calls - {"INSTRUMENTED_CALL"}
        self.assertEqual(legacy, {name for name, row in BY_NAME.items()
                                 if row.base == opcode.opmap["CALL"]
                                 and row.call_input_holes == 0})
        virtual = {"CALL_NO_SELF" + name[len("CALL"):] for name in legacy}
        self.assertEqual(virtual, {name for name, row in BY_NAME.items()
                                  if row.base == opcode.opmap["CALL"]
                                  and row.call_input_holes == 2})
        instrumented = {"INSTRUMENTED_CALL", "INSTRUMENTED_CALL_NO_SELF"}
        self.assertEqual({BY_NAME[name].call_input_holes for name in instrumented},
                         {0, 2})
        self.assertEqual(legacy | virtual | instrumented,
                         {name for name, row in BY_NAME.items()
                          if row.base in (opcode.opmap["CALL"],
                                          opcode.opmap["INSTRUMENTED_CALL"])})
        unknown = (2, -1)
        set_null = (1, -1)
        preserve = (0, -1)

        def transfer(name, dst, home, *, argc=1, encoded_arg=None,
                     edge=0, base=4):
            if encoded_arg is None:
                encoded_arg = argc
            return testapi.ir_normal_physical_transfer(
                named[name], dst, base, encoded_arg, 0,
                argc, edge, home, 2, 10)

        for name in sorted(legacy | virtual):
            argc = 2 if name.endswith("_ISINSTANCE") else 1
            with self.subTest(handler=name):
                # A new result at BASE is a value, not a known NULL.
                self.assertEqual(transfer(name, 4, 4, argc=argc), unknown)
                for home in range(5, 6 + argc):
                    self.assertEqual(transfer(name, 4, home, argc=argc),
                                     preserve if name in virtual and home == 5 else set_null)
                # Folded local and sink destinations drain the whole window.
                for dst in (0, 4095):
                    for home in range(4, 6 + argc):
                        self.assertEqual(transfer(name, dst, home, argc=argc),
                                         preserve if name in virtual and home == 5 else set_null)
                for home in (3, 9):
                    self.assertEqual(transfer(name, 4, home, argc=argc),
                                     unknown)
                # Direct variants cannot publish to arbitrary scratch A.
                for dst in (5, 6, 8):
                    self.assertEqual(transfer(name, dst, 5, argc=argc), unknown)
                self.assertEqual(transfer(name, 4, 5, argc=argc, edge=1),
                                 unknown)
                self.assertEqual(transfer(name, 4, 5, argc=argc,
                                          encoded_arg=argc + 1), unknown)
                self.assertEqual(transfer(name, 9, 9, argc=argc, base=9),
                                 unknown)
        # The observer/typed-runner path has no shared physical-NULL
        # contract. Its logical drain/virtual-hole proof is separate.
        for name in sorted(instrumented):
            for dst in (0, 4, 4095):
                for home in range(3, 10):
                    with self.subTest(handler=name, dst=dst, home=home):
                        self.assertEqual(transfer(name, dst, home), unknown)
        for name in ("CALL_KW", "INSTRUMENTED_CALL_KW"):
            self.assertEqual(transfer(name, 4, 5), unknown)

    def test_generator_completion_operand_layout(self):
        families = {opcode.opmap['SEND'], opcode.opmap['FOR_ITER']}
        # Exported base uses BaseOpcode, which retains instrumented IDs.
        # Include their positively declared canonical-family twin links.
        twins = {row.twin for row in ROWS.values()
                 if row.base in families and row.twin}
        selected = set()
        for handler, row in ROWS.items():
            if row.base not in families and handler not in twins:
                continue
            name = opcode.opname[handler]
            selected.add(name)
            with self.subTest(handler=name):
                self.assertEqual(row.roles, (_opcode.IR_ROLE_DST,
                                             _opcode.IR_ROLE_BASE,
                                             _opcode.IR_ROLE_FB,
                                             _opcode.IR_ROLE_IMM))
                self.assertEqual(row.imm, _opcode.IR_LOC_D)
                self.assertTrue(row.flags & _opcode.IR_FORWARD)
        self.assertTrue({'SEND', 'SEND_GEN', 'FOR_ITER', 'FOR_ITER_GEN',
                         'INSTRUMENTED_FOR_ITER'} <= selected)

    def test_result_publication_phase(self):
        phase = _opcode.IR_RESULT_PHASE_DST_TEMP_CANONICAL_BEFORE_RETIRE
        for name in ("LOAD_ATTR_SLOT", "LOAD_ATTR_INSTANCE_VALUE",
                     "LOAD_ATTR_SHAPE", "LOAD_ATTR_MODULE",
                     "LOAD_ATTR_WITH_HINT", "LOAD_ATTR"):
            with self.subTest(handler=name):
                self.assertEqual(BY_NAME[name].result_phase, phase)
        self.assertEqual(BY_NAME["STORE_ATTR"].result_phase,
                         _opcode.IR_RESULT_PHASE_NONE)
        self.assertEqual(BY_NAME["NOP"].result_phase,
                         _opcode.IR_RESULT_PHASE_NONE)
        for name, row in BY_NAME.items():
            with self.subTest(handler=name):
                self.assertIn(row.result_phase,
                              (_opcode.IR_RESULT_PHASE_NONE, phase))
                if row.result_phase == phase:
                    self.assertTrue(row.flags & _opcode.IR_STORE_DST)
                    self.assertIn(_opcode.IR_ROLE_DST, row.roles)

    def test_rows(self):
        self.assertEqual(len(CONTRACT), _opcode.IR_NUM_HANDLERS)
        named = {**opcode.opmap, **opcode._specialized_opmap, **opcode._ir_opmap}
        for h, row in ROWS.items():
            with self.subTest(handler=opcode.opname[h]):
                self.assertTrue(row.flags & _opcode.IR_VALID)
                self.assertIn(opcode.opname[h], named)
                self.assertIn(row.base, opcode.opmap.values())
                self.assertIn(row.shadow, ROWS)
        for name in (*opcode._specialized_opmap, *opcode._ir_opmap):
            self.assertIn(name, BY_NAME)

    def test_stack_effects(self):
        # The generic handler of an instruction and the
        # handlers of its operations have its stack effect, which is what the
        # compiler counts (_PyOpcode_num_popped() and _pushed()).  A
        # specialisation has its own, for the opargs it specialises.
        for h, row in ROWS.items():
            op = row.base
            if (op >= opcode.MIN_INSTRUMENTED_OPCODE
                    or opcode.opname[row.shadow] in opcode._specialized_opmap):
                continue
            if row.flags & _opcode.IR_FUSED:
                # The compare and the jump on its result: two inputs, no output
                with self.subTest(handler=opcode.opname[h]):
                    self.assertEqual((extent(row.reads, 0),
                                      extent(row.writes, 0)), (2, 0))
                continue
            for oparg in OPARGS:
                if not _opcode.has_arg(op) and oparg:
                    continue
                try:
                    expected = _opcode.stack_effect(
                        op, oparg if _opcode.has_arg(op) else None)
                except ValueError:
                    continue
                with self.subTest(handler=opcode.opname[h], oparg=oparg):
                    reads = extent(row.reads, oparg)
                    writes = extent(row.writes, oparg)
                    through = extent(row.through, oparg)
                    self.assertEqual(writes - reads, expected)
                    self.assertLessEqual(through, min(reads, writes))

    def test_through(self):
        # The deepest inputs that keep their slot and their value: not an
        # input that the body assigns or whose address it passes on
        # (null_or_index of FOR_ITER and SEND), nor one that it replaces
        # with its result (the callable of CALL_BUILTIN_*), nor any of SWAP
        # (the middle keeps its place but lies above bottom)
        expected = {
            "SWAP": 0, "CALL_BUILTIN_O": 0, "CALL_BUILTIN_CLASS": 0,
            "CALL_BUILTIN_FAST": 0, "CALL_BUILTIN_FAST_WITH_KEYWORDS": 0,
            "CALL_METHOD_DESCRIPTOR_FAST": 0,
            "CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS": 0,
            "FOR_ITER": 1, "FOR_ITER_LIST": 1, "FOR_ITER_TUPLE": 1,
            "FOR_ITER_VIRTUAL": 1, "INSTRUMENTED_FOR_ITER": 1, "SEND": 1,
            "SEND_VIRTUAL": 1, "FOR_ITER_RANGE": 2, "FOR_ITER_GEN": 2,
            "SEND_GEN": 2, "LOAD_ATTR_METHOD_WITH_VALUES": 0,
        }
        for name, count in expected.items():
            for oparg in (2, 3, 7):
                with self.subTest(name=name, oparg=oparg):
                    self.assertEqual(extent(BY_NAME[name].through, oparg),
                                     count)
        for oparg in (1, 2, 7):
            self.assertEqual(extent(BY_NAME["COPY"].through, oparg), oparg)

    def test_window_and_fields(self):
        for h, row in ROWS.items():
            with self.subTest(handler=opcode.opname[h]):
                roles = row.roles
                inputs = [r - _opcode.IR_ROLE_IN0 for r in roles
                          if r >= _opcode.IR_ROLE_IN0]
                if opcode.opname[h] in {"YIELD_DELEGATED", "INSTRUMENTED_YIELD_DELEGATED"}:
                    self.assertEqual(row.ninputs, 3)
                    self.assertEqual(sorted(inputs), [0, 2])
                    self.assertEqual(roles[2], _opcode.IR_ROLE_NONE)
                    self.assertEqual(row.borrow_mask, 0)
                else:
                    self.assertEqual(sorted(inputs), list(range(row.ninputs)))
                if row.flags & _opcode.IR_WINDOW:
                    self.assertIn(_opcode.IR_ROLE_BASE, roles)
                    # at most the sources read through fields of their own
                    self.assertLessEqual(row.ninputs, 2)
                self.assertEqual(row.borrow_mask >> row.ninputs, 0)
                if row.flags & _opcode.IR_UNPACKS:
                    # dst names a run of locals for the elements
                    self.assertIn(_opcode.IR_ROLE_DST, roles)
                    self.assertTrue(row.flags & _opcode.IR_WINDOW)
                elif row.dst is None:
                    self.assertNotIn(_opcode.IR_ROLE_DST, roles)
                    self.assertFalse(row.flags & _opcode.IR_STORE_DST)
                self.assertEqual(_opcode.IR_ROLE_FB in roles,
                                 row.fb != _opcode.IR_LOC_NONE)

    def test_slot_roles(self):
        expected = {
            "LOAD_FAST": _opcode.IR_ROLE_LOCAL_IN,
            "LOAD_FAST_BORROW": _opcode.IR_ROLE_LOCAL_IN,
            "LOAD_FAST_CHECK": _opcode.IR_ROLE_LOCAL_IN,
            "STORE_FAST": _opcode.IR_ROLE_LOCAL_OUT,
            "MAKE_CELL": _opcode.IR_ROLE_LOCAL_OUT,
            "DELETE_FAST": _opcode.IR_ROLE_LOCAL_CLEAR,
            "LOAD_FAST_AND_CLEAR": _opcode.IR_ROLE_LOCAL_MOVE,
            "LOAD_DEREF": _opcode.IR_ROLE_CELL,
            "STORE_DEREF": _opcode.IR_ROLE_CELL,
            "DELETE_DEREF": _opcode.IR_ROLE_CELL,
        }
        for name, role in expected.items():
            with self.subTest(name=name):
                self.assertEqual(oparg_role(BY_NAME[name]), role)
        # Exactly the opcodes whose oparg names a local or a cell have slot
        # roles
        named = {opcode.opname[op] for op in (*opcode.haslocal, *opcode.hasfree)
                 if op < opcode.MIN_INSTRUMENTED_OPCODE}
        slots = {name for name, row in BY_NAME.items()
                 if row.base < opcode.MIN_INSTRUMENTED_OPCODE
                 and oparg_role(row) in SLOT_ROLES}
        bases = {opcode.opname[BY_NAME[name].base] for name in slots}
        self.assertEqual(bases, named)

    def test_targets(self):
        for name, op in opcode.opmap.items():
            row = CONTRACT[op] if op < len(CONTRACT) else None
            if row is None or op >= opcode.MIN_INSTRUMENTED_OPCODE:
                continue
            with self.subTest(name=name):
                target = row.flags & (_opcode.IR_JUMPS | _opcode.IR_FORWARD)
                self.assertEqual(bool(target), op in opcode.hasjump)
        self.assertEqual(
            {name for name in names_with(_opcode.IR_NAMES_TARGET, "flags")
             if not name.startswith("INSTRUMENTED_")},
            {"END_ASYNC_FOR"})
        always = {name for name in names_with(_opcode.IR_JUMP_ALWAYS, "flags")
                  if not name.startswith("INSTRUMENTED_")}
        self.assertLessEqual({"JUMP_FORWARD", "JUMP_BACKWARD",
                              "JUMP_BACKWARD_NO_INTERRUPT"}, always)
        for name in always:
            self.assertTrue(BY_NAME[name].flags
                            & (_opcode.IR_JUMPS | _opcode.IR_FORWARD), name)
            self.assertNotIn(name, ("FOR_ITER", "SEND", "POP_JUMP_IF_TRUE"))

    def test_feedback(self):
        kinds = dis._FEEDBACK_KINDS
        for h, row in ROWS.items():
            # A twin shares the format, and so the entry, of its family
            family = opcode.opname[row.base].removeprefix("INSTRUMENTED_")
            with self.subTest(handler=opcode.opname[h]):
                if row.fb == _opcode.IR_LOC_NONE:
                    self.assertEqual((row.fb_kind, row.fb_size), (0, 0))
                    continue
                if row.flags & _opcode.IR_FUSED:
                    # The entry of a fused compare is that of the compare;
                    # IS_OP has no entry apart from its fused form, which
                    # has the size of the entry of a branch.
                    if family == "IS_OP":
                        self.assertEqual((kinds[row.fb_kind], row.fb_size),
                                         FEEDBACK["TO_BOOL"])
                    else:
                        self.assertEqual((kinds[row.fb_kind], row.fb_size),
                                         FEEDBACK[family])
                    continue
                self.assertIn(family, FEEDBACK)
                self.assertEqual((kinds[row.fb_kind], row.fb_size),
                                 FEEDBACK[family])
        # Every family that has an entry has one in the generic form
        for family in FEEDBACK:
            self.assertNotEqual(BY_NAME[family].fb, _opcode.IR_LOC_NONE)
        # The generic handlers that specialise from their entry
        self.assertEqual(
            {opcode.opname[op] for op in range(opcode.MIN_INSTRUMENTED_OPCODE)
             if CONTRACT[op] is not None
             and CONTRACT[op].effects & _opcode.IR_SPECIALIZES},
            set(FEEDBACK) - NOT_SPECIALIZING)

    def test_feedback_tables(self):
        # The assembler writes each entry's kind into the code object's
        # feedback table from the contract.
        import email.message, json.decoder
        seen = set()
        for module in (dis, email.message, json.decoder, unittest.case):
            for co in _code_objects(module):
                fbinit = co.co_fbinit
                for _, h, _, _, _, fb in dis._unpack_opargs(co.co_code, fbinit):
                    if not fb or fbinit is None:
                        continue
                    kind, _, units = dis._feedback_entry(fbinit, fb)
                    self.assertGreaterEqual(units * 8, CONTRACT[h].fb_size)
                    self.assertEqual(kind, CONTRACT[h].fb_kind,
                                     opcode.opname[h])
                    seen.add(dis._FEEDBACK_KINDS[kind])
        # COUNTER is entry 0, which no word names
        self.assertEqual(seen, set(dis._FEEDBACK_KINDS) - {"COUNTER"})

    def test_monitoring(self):
        events = sys.monitoring.events
        for h, row in ROWS.items():
            name = opcode.opname[h]
            with self.subTest(handler=name):
                twin = row.twin
                if name.startswith("INSTRUMENTED_"):
                    self.assertEqual(twin, h)
                if twin is None:
                    if row.base == opcode.opmap["SEND"]:
                        self.assertEqual(row.events, events.STOP_ITERATION | events.BRANCH_LEFT)
                    elif row.base in {opcode.opmap[name] for name in
                                      ('CLEANUP_THROW', 'RAISE_VARARGS', 'RERAISE')}:
                        self.assertEqual(row.events, events.RERAISE)
                    else:
                        self.assertEqual(row.events, 0)
                    continue
                self.assertTrue(opcode.opname[twin].startswith("INSTRUMENTED_"))
                self.assertEqual(ROWS[twin].twin, twin)
                self.assertEqual(row.events, ROWS[twin].events)
                self.assertEqual(row.flags, ROWS[twin].flags)
                self.assertTrue(ROWS[twin].effects & _opcode.IR_RUNS_PYTHON)
        expected = {
            "INSTRUMENTED_CALL": {"CALL", "C_RETURN", "C_RAISE"},
            "INSTRUMENTED_LINE": {"LINE"},
            "INSTRUMENTED_INSTRUCTION": {"INSTRUCTION"},
            "INSTRUMENTED_RETURN_VALUE": {"PY_RETURN"},
            "INSTRUMENTED_YIELD_VALUE": {"PY_YIELD"},
            "INSTRUMENTED_YIELD_DELEGATED": {"PY_YIELD"},
            "INSTRUMENTED_JUMP_FORWARD": {"JUMP"},
            "INSTRUMENTED_JUMP_BACKWARD": {"JUMP"},
            "INSTRUMENTED_POP_JUMP_IF_TRUE": {"BRANCH_RIGHT", "BRANCH_LEFT"},
            "INSTRUMENTED_END_SEND": {"STOP_ITERATION", "BRANCH_LEFT"},
            "INSTRUMENTED_END_FOR": {"STOP_ITERATION"},
            "INSTRUMENTED_FOR_ITER": {"BRANCH_LEFT", "BRANCH_RIGHT", "STOP_ITERATION"},
            "SEND": {"STOP_ITERATION", "BRANCH_LEFT"},
            "INSTRUMENTED_RESUME": {"PY_START", "PY_RESUME"},
        }
        for name, names in expected.items():
            with self.subTest(name=name):
                bits = BY_NAME[name].events
                for event in names:
                    self.assertTrue(bits & getattr(events, event), event)
        self.assertEqual(BY_NAME["CALL"].twin, opcode.opmap["INSTRUMENTED_CALL"])

    def test_monitoring_run(self):
        # Every declared event observed in a code object belongs to one of
        # its semantic handlers or a transparent observer added to any code.
        # This is a code-object check: branch callbacks identify the semantic
        # branch site even when another handler reports the outcome.
        events = sys.monitoring.events
        twin_events = 0
        for row in ROWS.values():
            twin_events |= row.events
        added = (BY_NAME["INSTRUMENTED_LINE"].events
                 | BY_NAME["INSTRUMENTED_INSTRUCTION"].events)

        def gen():
            yield 1
            yield from range(2)

        def sample(n):
            total = 0
            for i in range(n):
                if i % 2:
                    total += i
                elif i is None:
                    total = -1
            while n > 0:
                n -= 1
            try:
                len(5)
            except TypeError:
                pass
            return total + sum(gen()) + len([i for i in range(3)])

        fired = {}
        tool = sys.monitoring.DEBUGGER_ID
        sys.monitoring.use_tool_id(tool, "test_ir_contract")
        self.addCleanup(sys.monitoring.free_tool_id, tool)
        self.addCleanup(sys.monitoring.set_events, tool, 0)
        mask = 0
        for name in dir(events):
            bit = getattr(events, name)
            if (name.isupper() and bit & twin_events and bit & (bit - 1) == 0):
                mask |= bit
                def record(code, *args, bit=bit):
                    fired.setdefault(code, 0)
                    fired[code] |= bit
                sys.monitoring.register_callback(tool, bit, record)
                self.addCleanup(sys.monitoring.register_callback, tool, bit,
                                None)
        sys.monitoring.set_events(tool, mask)
        sample(5)
        sys.monitoring.set_events(tool, 0)
        mine = {sample.__code__, gen.__code__}
        self.assertLessEqual(mine, set(fired))
        for co in mine | {c for c in sample.__code__.co_consts
                          if isinstance(c, types.CodeType)}:
            allowed = added
            for _, h, *_ in dis._unpack_opargs(co.co_code):
                if h in ROWS:
                    allowed |= ROWS[h].events
            with self.subTest(code=co.co_name):
                self.assertEqual(fired.get(co, 0) & ~allowed, 0)

    def test_effects(self):
        for h, row in ROWS.items():
            effects, flags = row.effects, row.flags
            with self.subTest(handler=opcode.opname[h]):
                # The format runs code if any of its handlers may
                if effects & _opcode.IR_RUNS_PYTHON:
                    self.assertTrue(flags & _opcode.IR_RUNS_CODE)
                if effects & _opcode.IR_PURE:
                    # POP_TOP is pure but escapes: the release may run code
                    self.assertFalse(effects & _opcode.IR_RAISES)
                if effects & (_opcode.IR_PUSHES_FRAME | _opcode.IR_SUSPENDS):
                    self.assertTrue(effects & _opcode.IR_RUNS_PYTHON)
                if effects & _opcode.IR_SPECIALIZES:
                    name = opcode.opname[h]
                    self.assertNotIn(name, opcode._specialized_opmap)
                    self.assertFalse(name.startswith("INSTRUMENTED_"))
                    self.assertNotEqual(row.fb, _opcode.IR_LOC_NONE)
        suspends = {name for name in names_with(_opcode.IR_SUSPENDS)}
        self.assertEqual(suspends, {"YIELD_VALUE", "INSTRUMENTED_YIELD_VALUE",
                                    "YIELD_DELEGATED", "INSTRUMENTED_YIELD_DELEGATED",
                                    "RETURN_GENERATOR"})
        for name in ("CALL_PY_EXACT_ARGS", "SEND", "SEND_GEN", "FOR_ITER_GEN",
                     "LOAD_ATTR_PROPERTY"):
            self.assertTrue(BY_NAME[name].effects & _opcode.IR_PUSHES_FRAME,
                            name)
        for name in ("LOAD_FAST", "LOAD_CONST", "LOAD_SMALL_INT"):
            self.assertFalse(BY_NAME[name].flags & _opcode.IR_RUNS_CODE, name)
        for name in ("CALL", "STORE_FAST", "DELETE_FAST", "LOAD_GLOBAL",
                     "BINARY_OP"):
            self.assertTrue(BY_NAME[name].flags & _opcode.IR_RUNS_CODE, name)
        self.assertTrue(BY_NAME["JUMP_BACKWARD"].effects & _opcode.IR_POLLS)
        self.assertTrue(BY_NAME["RESUME"].effects & _opcode.IR_POLLS)
        self.assertTrue(BY_NAME["BINARY_OP_ADD_INT"].effects & _opcode.IR_DEOPTS)


def _code_objects(module):
    "The code objects of the functions and methods of module, nested ones too"
    todo = []
    for value in vars(module).values():
        if isinstance(value, types.FunctionType):
            todo.append(value.__code__)
        elif isinstance(value, type):
            for attr in vars(value).values():
                if isinstance(attr, types.FunctionType):
                    todo.append(attr.__code__)
    while todo:
        co = todo.pop()
        yield co
        todo.extend(c for c in co.co_consts if isinstance(c, types.CodeType))


@unittest.skipIf(test_tools.basepath is None, "needs the source tree")
class AnalyzerTests(unittest.TestCase):
    """Rows against the cases generator's analysis of Python/bytecodes.c,
    from which _opcode_metadata's HAS_ERROR, HAS_ESCAPES and HAS_PURE flags
    also come"""

    @classmethod
    def setUpClass(cls):
        test_tools.skip_if_missing("cases_generator")
        with test_tools.imports_under_tool("cases_generator"):
            import analyzer
            import ir_generator
        source = os.path.join(test_tools.basepath, "Python", "bytecodes.c")
        if not os.path.exists(source):
            raise unittest.SkipTest("needs Python/bytecodes.c")
        cls.ir_generator = ir_generator
        cls.analysis = analyzer.analyze_files([source])
        cls.insts = ir_generator.handler_instructions(cls.analysis)
        cls.ids = ir_generator.handler_ids(cls.analysis)
        cls._generated = None

    def test_ids(self):
        for name, h in self.ids.items():
            self.assertEqual(opcode.opname[h], name)
            self.assertIsNotNone(CONTRACT[h], name)

    def test_properties(self):
        for name, inst in self.insts.items():
            p = inst.properties
            effects = CONTRACT[self.ids[name]].effects
            with self.subTest(handler=name):
                # The twin of a fused jump raises when a callback does,
                # which the analysis does not see in INSTRUMENTED_JUMP()
                fused_twin = (name.startswith("INSTRUMENTED_")
                              and name.endswith("_JUMP"))
                self.assertEqual(bool(effects & _opcode.IR_RAISES),
                                 not p.infallible or fused_twin)
                self.assertEqual(bool(effects & _opcode.IR_ESCAPES), p.escapes)
                self.assertEqual(bool(effects & _opcode.IR_PURE), p.pure)
                self.assertEqual(bool(effects & _opcode.IR_DEOPTS),
                                 p.deopts or p.side_exit)
                self.assertEqual(bool(effects & _opcode.IR_RUNS_PYTHON),
                                 self.ir_generator.runs_code(inst))

    def test_runs_code(self):
        # The format's RUNS_CODE is the union of RUNS_PYTHON over the
        # handlers that share the format
        formats = self.ir_generator.compute_formats(self.analysis)
        groups = {}
        for name in self.insts:
            groups.setdefault(formats[name].group, []).append(name)
        for group, names in groups.items():
            with self.subTest(group=group):
                rows = [CONTRACT[self.ids[name]] for name in names]
                union = any(row.effects & _opcode.IR_RUNS_PYTHON for row in rows)
                for row in rows:
                    self.assertEqual(bool(row.flags & _opcode.IR_RUNS_CODE),
                                     union)

    def test_boxing(self):
        # No macro boxes into value_temps without the generated code naming
        # it: the handlers box through _PyValueTemps_Box(value_temps, x),
        # which boxes_into_record() sees
        path = os.path.join(test_tools.basepath, "Python", "ceval_macros.h")
        with open(path) as f:
            text = f.read().replace("\\\n", " ")
        writers = {m.group(1) for m in re.finditer(
            r"#\s*define\s+(\w+)[^\n]*\bvalue_temps->"
            r"(?:count\s*(?:\+\+|[-+]?=(?!=))|objects\s*\[)", text)}
        self.assertEqual(writers, set())
        boxes = self.ir_generator.boxes_into_record
        decl = "_PyValueTemps *value_temps = &value_owners;\n"
        for code, expected in [
            ("", False),
            ("assert(value_temps->count == 0);", False),
            ("finish_displaced_local(tstate, value_temps, ir_displaced);", False),
            ("ir_obj_left = _PyValueTemps_Box(value_temps, left);", True),
            ("value_reader_complete(&s, next_instr, value_temps, &r);", False),
            ("_PyValueTemps_Object(value_temps, l);", True),
            ("spam(value_temps);", True),
            ("value_temps->count++;", True),
            ("_PyValueTemps *t = value_temps;", True),
        ]:
            with self.subTest(code=code):
                self.assertEqual(boxes(decl + code), expected)

    def generated(self):
        "The generated handlers: the text and the bodies by name"
        if type(self)._generated is None:
            source = os.path.join(test_tools.basepath, "Python", "bytecodes.c")
            gen = self.ir_generator
            out = io.StringIO()
            gen.generate_ir([source], self.analysis, out, False)
            cases = out.getvalue()
            text = cases.split(gen.INSTRUCTION_START_MARKER)[1]
            text = text.split(gen.INSTRUCTION_END_MARKER)[0]
            handlers = dict(re.findall(
                r"\bTARGET\((\w+)\) \{(.*?)(?=\bTARGET\(\w+\) \{|\Z)", text, re.S))
            type(self)._generated = cases, handlers
        return type(self)._generated

    def test_value_record(self):
        # A handler sets up a value record, value_temps, only if the
        # contract says it boxes (BOXES, IR v2 step A1).  Elsewhere
        # value_temps is a NULL constant, so its dispatch finishes no
        # bridges.
        _, handlers = self.generated()
        self.assertEqual(handlers.keys(), self.insts.keys())
        record = "_PyValueTemps *value_temps = &value_owners;"
        null = "_PyValueTemps *const value_temps = NULL;"
        for name, body in handlers.items():
            boxes = bool(CONTRACT[self.ids[name]].effects & _opcode.IR_BOXES)
            with self.subTest(handler=name):
                self.assertEqual(record in body, boxes)
                if not boxes:
                    self.assertNotIn("value_owners", body)
                    self.assertNotIn("value_boxes", body)
                    uses = {line.strip() for line in body.splitlines()
                            if re.search(r"\bvalue_temps\b", line)}
                    self.assertIn(null, uses)
                    self.assertLessEqual(uses, {null, "(void)value_temps;"})

    def test_next_instr_by_value(self):
        # No handler or dispatch macro takes the address of
        # next_instr (IR v2 step A2): the executing copy is fixed for the
        # code object's life, so no helper rebases the PC.
        cases, _ = self.generated()
        path = os.path.join(test_tools.basepath, "Python", "ceval_macros.h")
        with open(path) as f:
            macros = f.read()
        for name, text in (("generated_ir_cases.c.h", cases),
                           ("ceval_macros.h", macros)):
            with self.subTest(name):
                lines = [line.strip() for line in text.splitlines()
                         if re.search(r"&\s*next_instr\b", line)]
                self.assertEqual(lines, [])

    def test_stack_items(self):
        # The extents count the instruction's own stack items, for every
        # handler including those of the instrumented twins
        for name, inst in self.insts.items():
            row = CONTRACT[self.ids[name]]
            inputs, outputs = self.ir_generator.external_io(inst)
            with self.subTest(handler=name):
                fixed = [i for i in inputs if not i.is_array()]
                self.assertGreaterEqual(row.reads[0], 0)
                if all(not i.is_array() for i in inputs):
                    self.assertEqual(row.reads,
                                     (len(fixed), _opcode.IR_EXT_FIXED))
                if all(not e.item.is_array() for e in outputs):
                    self.assertEqual(row.writes,
                                     (len(outputs), _opcode.IR_EXT_FIXED))


# A scan of Python/bytecodes.c by regular expressions, which shares no code
# with the cases generator, for the columns that it can check
_DEFINITION = re.compile(r"\b(?:op|inst)\((\w+),\s*\(([^)]*)\)\)\s*\{")
_MACRO = re.compile(r"\bmacro\((\w+)\)\s*=\s*([^;]*);")


def _stack_items(text):
    "The (name, size) of each item of one side of a stack effect"
    result = []
    for item in text.split(","):
        item = item.strip()
        if item and "/" not in item:    # not a cache entry
            m = re.fullmatch(r"(\w+)\s*(?:\[(.*)\])?", item)
            result.append((m.group(1), m.group(2)))
    return result


def _body(text, start):
    depth = 0
    for i in range(start, len(text)):
        depth += {"{": 1, "}": -1}.get(text[i], 0)
        if depth == 0:
            return text[start + 1:i]
    raise ValueError("unbalanced braces")


def _scan_definitions(path):
    """The ops and instructions of path, as (inputs, outputs, body), and the
    parts of each macro"""
    with open(path) as f:
        text = re.sub(r"//[^\n]*|/\*.*?\*/", " ", f.read(), flags=re.S)
    ops = {}
    for m in _DEFINITION.finditer(text):
        inputs, outputs = m.group(2).split("--")
        ops[m.group(1)] = (_stack_items(inputs), _stack_items(outputs),
                           _body(text, m.end() - 1))
    macros = {}
    for m in _MACRO.finditer(text):
        parts = (p.strip() for p in m.group(2).split("+"))
        macros[m.group(1)] = [p for p in parts if "/" not in p and p != "flush"]
    return ops, macros


def _changed_in(body, name, array):
    "Whether body assigns, steps or takes the address of item name"
    n = re.escape(name)
    assign = r"\s*(?:[-+*/%&|^]?=(?!=)|\+\+|--)"
    if array:
        # An element store, or the array passed on, which lets the callee
        # write the elements
        return bool(re.search(rf"(?<![\w.>]){n}\s*\[[^\]]*\]{assign}", body)
                    or re.search(rf"(?<![\w.>]){n}\b(?!\s*\[)", body)
                    or re.search(rf"&\s*{n}\s*\[", body))
    if (re.search(rf"(?<![\w.>]){n}{assign}", body)
            or re.search(rf"(?:\+\+|--)\s*{n}\b", body)):
        return True
    for m in re.finditer(rf"&\s*{n}\b", body):
        # Address-of, not a binary and
        if not re.search(r"[\w)\]]$", body[:m.start()].rstrip()):
            return True
    return False


def _scan_stack(ops, macros, name):
    """The inputs of handler name, from the deepest, and the stack entries
    that it leaves, as dicts of size (None for one item) and whether the
    entry is an input that keeps its value.  Both call-invoke markers publish
    the result themselves, so trailing callee-frame operations are not part
    of the register IR word."""
    stack = []
    inputs = []
    for part in macros.get(name, [name]):
        ins, outs, body = ops[part]
        if len(stack) < len(ins):
            fresh = [{"size": None, "kept": True}
                     for _ in range(len(ins) - len(stack))]
            stack[:0] = fresh
            inputs[:0] = fresh
        popped = stack[len(stack) - len(ins):]
        del stack[len(stack) - len(ins):]
        for entry, (_, size) in zip(popped, ins):
            if any(entry is e for e in inputs) and entry["size"] is None:
                entry["size"] = size
        for k, (out, size) in enumerate(outs):
            if (k < len(ins) and ins[k] == (out, size)
                    and (out == "unused"
                         or not _changed_in(body, out, size is not None))):
                stack.append(popped[k])
            else:
                stack.append({"size": size, "kept": False})
        if re.search(r"\bIR_INVOKE_(?:CALL|GUARDED_PYTHON_CALL)\b", body):
            break
    return inputs, stack


def _scan_count(entries, oparg):
    "The slots that entries cover for oparg, or None if an array is negative"
    sizes = [1 if e["size"] is None else eval(e["size"], {"oparg": oparg})
             for e in entries]
    return None if min(sizes, default=0) < 0 else sum(sizes)


def _scan_through(inputs, outputs):
    "The deepest inputs that keep their slot and value, up to the first not"
    through = []
    for entry, item in zip(inputs, outputs):
        if entry is not item or not entry["kept"]:
            break
        through.append(entry)
    return through


def _stack_counts(path, function):
    "The return expression of each case of function in path, by opcode name"
    with open(path) as f:
        text = f.read()
    start = text.index(f"int {function}(int opcode, int oparg)")
    body = _body(text, text.index("{", start))
    return dict(re.findall(r"case (\w+):\s*return ([^;]*);", body))


@unittest.skipIf(test_tools.basepath is None, "needs the source tree")
class SourceScanTests(unittest.TestCase):
    """Rows against sources that do not come from ir_generator.py"""

    def source(self, *parts):
        path = os.path.join(test_tools.basepath, *parts)
        if not os.path.exists(path):
            self.skipTest(f"needs {os.path.join(*parts)}")
        return path

    def scans(self):
        "The scan of each handler that is an instruction of bytecodes.c"
        ops, macros = _scan_definitions(self.source("Python", "bytecodes.c"))
        result = {}
        for h, row in ROWS.items():
            name = opcode.opname[h]
            if name in ops or name in macros:
                result[h] = _scan_stack(ops, macros, name)
        names = {opcode.opname[h] for h in result}
        self.assertLessEqual({"SWAP", "CALL_BUILTIN_O", "CALL_BUILTIN_FAST",
                              "FOR_ITER", "FOR_ITER_RANGE", "SEND"}, names)
        self.assertGreater(len(result), 200)
        return result

    def test_through(self):
        for h, (inputs, outputs) in self.scans().items():
            through = _scan_through(inputs, outputs)
            for oparg in (2, 3, 7, 0x102):
                count = _scan_count(through, oparg)
                if count is None:
                    continue
                with self.subTest(handler=opcode.opname[h], oparg=oparg):
                    self.assertEqual(extent(ROWS[h].through, oparg), count)

    def test_stack_counts(self):
        # reads and writes, each against the scan, and against
        # _PyOpcode_num_popped() and _pushed(), which
        # opcode_metadata_generator.py writes, for every handler: the
        # specialisations and twins too.  Those disagree only for a call
        # that publishes its result itself.  An IR-only handler has the
        # counts of the instruction that it is named after.
        path = self.source("Include", "internal", "pycore_opcode_metadata.h")
        popped = _stack_counts(path, "_PyOpcode_num_popped")
        pushed = _stack_counts(path, "_PyOpcode_num_pushed")
        scans = self.scans()
        invokes = set()
        for h, row in ROWS.items():
            name = opcode.opname[h]
            if name not in popped:
                shadow = opcode.opname[row.shadow]
                name = shadow if shadow in popped else opcode.opname[row.base]
            for oparg in OPARGS:
                counts = (extent(row.reads, oparg),
                          extent(row.writes, oparg))
                expected = (eval(popped[name], {"oparg": oparg}),
                            eval(pushed[name], {"oparg": oparg}))
                if opcode.opname[h] in {"YIELD_DELEGATED", "INSTRUMENTED_YIELD_DELEGATED"}:
                    # Frontend YIELD_VALUE1 is normalized to three logical
                    # inputs/outputs before allocation. Its cursor dependency
                    # remains in SSA without a physical input field.
                    self.assertEqual(expected, (1, 1))
                    self.assertEqual(extent(row.through, oparg), 2)
                    expected = (3, 3)
                if min(expected) < 0:
                    continue
                if opcode.opname[h].endswith(("OP_JUMP", "OP_JUMP_FLOAT", "OP_JUMP_INT", "OP_JUMP_STR", "OP_JUMP_SET", "OP_JUMP_DICT")):
                    # A fused compare-and-jump consumes the operands
                    # and the jump consumes the result: no write.
                    expected = (expected[0], 0)
                with self.subTest(handler=opcode.opname[h], oparg=oparg):
                    scanned = tuple(_scan_count(e, oparg)
                                    for e in scans.get(h, ()))
                    if scanned and None not in scanned:
                        self.assertEqual(counts, scanned)
                    if counts != expected:
                        invokes.add(name)
                        self.assertEqual(counts, (expected[0], expected[1] + 1))
        self.assertEqual(invokes, {
            "CALL_PY_GENERAL", "CALL_BOUND_METHOD_GENERAL", "CALL_KW_PY",
            "CALL_KW_BOUND_METHOD", "CALL_EX_PY"})


VERIFY_SCRIPT = textwrap.dedent("""
    import sys
    failures = 0
    for path in sys.argv[1:]:
        with open(path, "rb") as f:
            source = f.read()
        try:
            compile(source, path, "exec")
        except SyntaxError:
            pass
        except SystemError as e:
            failures += 1
            print(path, e)
    print("compiled", len(sys.argv) - 1)
    sys.exit(1 if failures else 0)
""")


class VerifierTests(unittest.TestCase):
    """The verifier (_PyRegCfg_Verify) checks every stage of the register CFG,
    including that no folded load's reader comes after a word that may run
    Python.  A debug build always runs it; PYTHONREGCFGVERIFY runs it in a
    release build too."""

    def run_verifier(self, paths):
        rc, out, err = script_helper.assert_python_ok(
            "-c", VERIFY_SCRIPT, *paths, PYTHONREGCFGVERIFY="1")
        self.assertIn(b"compiled", out)

    def stdlib(self):
        lib = os.path.dirname(os.__file__)
        for root, dirs, files in os.walk(lib):
            dirs[:] = sorted(d for d in dirs if d not in ("site-packages",
                                                          "__pycache__"))
            for name in sorted(files):
                if name.endswith(".py"):
                    yield os.path.join(root, name)

    def test_folded_loads(self):
        source = textwrap.dedent("""
            import sys
            def g():
                sys._getframe(1).f_locals['x'] = 100
                return 0
            def outer():
                x = 1
                return x + g()
            def stores(a, b):
                x = a
                y = b
                z = x + y
                del y
                return z, x
            assert outer() == 1
        """)
        with open(os_helper.TESTFN, "w") as f:
            f.write(source)
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        self.run_verifier([os_helper.TESTFN])

    def test_folded_load_rejected(self):
        # With copy propagation's kill at code that may run Python turned
        # off, the load of x folds into the add past the call, which may
        # rebind x; the check of folded loads must reject that, and so
        # runs.  Under -E a release build does not verify.
        import_helper.import_module("_testinternalcapi")
        script = textwrap.dedent("""
            import _testinternalcapi
            source = "def outer():\\n    x = 1\\n    return x + g()\\n"
            try:
                _testinternalcapi.compile_without_copyprop_kill(source, "g1")
            except SystemError as e:
                print(e)
            else:
                print("accepted")
            compile(source, "g1", "exec")
        """)
        rc, out, err = script_helper.assert_python_ok(
            "-c", script, PYTHONREGCFGVERIFY="1")
        self.assertIn(b"code that may rebind the local of a folded load "
                      b"before its reader", out)
        if not support.Py_DEBUG:
            rc, out, err = script_helper.assert_python_ok(
                "-E", "-c", script, PYTHONREGCFGVERIFY="1")
            self.assertEqual(out.strip(), b"accepted")

    def test_stdlib_sample(self):
        paths = list(self.stdlib())
        if not support.is_resource_enabled("cpu"):
            paths = paths[::40]
        for start in range(0, len(paths), 500):
            self.run_verifier(paths[start:start + 500])


if __name__ == "__main__":
    unittest.main()
