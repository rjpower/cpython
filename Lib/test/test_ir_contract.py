"""The instruction contract of the register IR.

_opcode.get_ir_contract() exports _PyIR_Formats (Include/internal/pycore_ir.h),
which Tools/cases_generator/ir_metadata_generator.py writes from
Tools/cases_generator/stack_ops.py and Python/register_ops.  The compiler passes, the assembler, dis and the
instrumentation read it.  Each row must agree with the stack effects that the
compiler uses, with the generator's analysis of the instruction, with the
feedback layouts and with sys.monitoring.
"""

from test.support import ir_dis as dis
import io
from test.support import ir_dis as opcode
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
# The rows of the register instruction arms follow the old handlers and have
# no stack effect to check (test_ir_arm_words)
OLD_CONTRACT = CONTRACT[:_opcode.IR_FIRST_ARM]
ROWS = {h: row for h, row in enumerate(OLD_CONTRACT) if row is not None}
BY_NAME = {opcode._all_opname[h]: row for h, row in ROWS.items()}

SLOT_ROLES = (_opcode.IR_ROLE_LOCAL_IN, _opcode.IR_ROLE_LOCAL_OUT,
              _opcode.IR_ROLE_LOCAL_CLEAR, _opcode.IR_ROLE_LOCAL_MOVE,
              _opcode.IR_ROLE_CELL)
FIELD_LOCS = (_opcode.IR_LOC_A, _opcode.IR_LOC_B, _opcode.IR_LOC_C,
              _opcode.IR_LOC_D)

# The feedback entry of each family (Include/internal/pycore_feedback.h):
# its kind and size in bytes
FEEDBACK = {
    "FOR_ITER": ("UNARY", 24),
    "GET_ITER": ("UNARY", 24),
    "SEND": ("UNARY", 24),
    "CALL_KW": ("CALL", 40),
    "CALL_FUNCTION_EX": ("CALL", 40),
}

OPARGS = (0, 1, 2, 3, 7, 0x102)

# Ops that no stack bytecode has as an opcode: the translator_only
# instructions of Tools/cases_generator/stack_ops.py
TRANSLATOR_ONLY = ("JUMP_IF_FALSE_OR_POP", "JUMP_IF_TRUE_OR_POP",
                "INSTRUMENTED_JUMP_IF_FALSE_OR_POP",
                "INSTRUMENTED_JUMP_IF_TRUE_OR_POP", "MATERIALIZE_KWARGS")


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


def present(*names):
    """The names that still have a row.  A ported stack op's handlers leave
    the table, so a test that names handlers follows those that remain."""
    found = [name for name in names if name in BY_NAME]
    if not found:
        raise unittest.SkipTest("no live handler left; the old generator goes at D")
    return found


def names_with(bit, key="effects"):
    return {name for name, row in BY_NAME.items() if getattr(row, key) & bit}


class ContractTests(unittest.TestCase):

    def test_normal_physical_transfer(self):
        testapi = import_helper.import_module("_testinternalcapi")
        named = {**opcode.opmap, **opcode._specialized_opmap,
                 **opcode._ir_handlers}
        preserve = (0, -1)
        set_null = (1, -1)
        unknown = (2, -1)

        def transfer(name, fields, edge, home, nl=2, limit=10):
            return testapi.ir_normal_physical_transfer(
                named[name], *fields, edge, home, nl, limit)

        # POP_TOP lowers to a retirement: Move.Retire([], [src]) is
        # A = 0 destinations, B = 1 source, the source in C
        retire = (0, 1, 4, 4095)
        self.assertEqual(transfer("Move.Retire", retire, 0, 4), set_null)
        self.assertEqual(transfer("Move.Retire", retire, 0, 5), preserve)
        # Successive retirements retain each other's NULL facts.
        self.assertEqual(transfer("Move.Retire", (0, 1, 5, 4095), 0, 4), preserve)
        self.assertEqual(transfer("Move.Retire", (0, 1, 5, 4095), 0, 5), set_null)
        self.assertEqual(transfer("Move.Retire", retire, 1, 4), unknown)

        # ForIter(item, pair): the next edge keeps the receiver and
        # rewrites the cursor and the item; the taken edge retires the pair.
        # A home the word does not name is preserved on both.
        for_iter = (6, 4, 0, 1)
        self.assertEqual(transfer("ForIter.Generic", for_iter, 1, 4), set_null)
        self.assertEqual(transfer("ForIter.Generic", for_iter, 1, 5), set_null)
        self.assertEqual(transfer("ForIter.Generic", for_iter, 1, 6), preserve)
        self.assertEqual(transfer("ForIter.Generic", for_iter, 0, 4), preserve)
        self.assertEqual(transfer("ForIter.Generic", for_iter, 0, 5), unknown)
        self.assertEqual(transfer("ForIter.Generic", for_iter, 0, 6), unknown)
        self.assertEqual(transfer("ForIter.Generic", for_iter, 0, 7), preserve)
        self.assertEqual(transfer("ForIter.Generic", for_iter, 1, 7), preserve)
        # Every normal edge: the meet
        self.assertEqual(transfer("ForIter.Generic", for_iter, -1, 4), unknown)
        self.assertEqual(transfer("ForIter.Generic", for_iter, -1, 7), preserve)
        # The item's home is an operand of its own, anywhere in the frame
        self.assertEqual(transfer("ForIter.Generic", (8, 4, 0, 1), 1, 4), set_null)

    def test_call_family(self):
        # The words that name their operands carry the transfers of their
        # homes, not the head: every handler of the keyword call admits the
        # virtual self role, a flag of the word
        bases = {opcode.opmap["CALL_KW"], opcode.opmap["INSTRUMENTED_CALL_KW"]}
        calls = present(*(name for name, row in BY_NAME.items()
                          if row.base in bases))
        self.assertEqual({BY_NAME[name].word_flags for name in calls},
                         {_opcode.IR_F_NO_SELF})

    def test_generator_completion_operand_layout(self):
        families = {opcode.opmap['FOR_ITER']}
        # Exported base uses BaseOpcode, which retains instrumented IDs.
        # Include their positively declared canonical-family twin links.
        twins = {row.twin for row in ROWS.values()
                 if row.base in families and row.twin}
        selected = set()
        for handler, row in ROWS.items():
            if row.base not in families and handler not in twins:
                continue
            name = opcode._all_opname[handler]
            selected.add(name)
            with self.subTest(handler=name):
                self.assertEqual(row.roles, (_opcode.IR_ROLE_DST,
                                             _opcode.IR_ROLE_BASE,
                                             _opcode.IR_ROLE_FB,
                                             _opcode.IR_ROLE_IMM))
                self.assertEqual(row.imm, _opcode.IR_LOC_D)
                self.assertTrue(row.flags & _opcode.IR_FORWARD)

    def test_no_result_publication_phase(self):
        # A handler stores its result in its place in the upstream order,
        # not in a phase before a retirement: no handler names one
        for name, row in BY_NAME.items():
            with self.subTest(handler=name):
                self.assertEqual(row.result_phase,
                                 _opcode.IR_RESULT_PHASE_NONE)

    def test_rows(self):
        self.assertEqual(len(CONTRACT), _opcode.IR_NUM_HANDLERS)
        named = {**opcode.opmap, **opcode._specialized_opmap, **opcode._ir_handlers}
        for h, row in ROWS.items():
            with self.subTest(handler=opcode._all_opname[h]):
                self.assertTrue(row.flags & _opcode.IR_VALID)
                self.assertIn(opcode._all_opname[h], named)
                # A base is the stack opcode that the handler stands for, or
                # 512 + the handler for one that has no stack op
                self.assertIn(row.base, {*opcode._stack_opmap.values(),
                                         *(opcode._HANDLER_OP + h
                                           for h in opcode._ir_only_handlers)})
                self.assertIn(row.shadow, {None, *opcode._stack_opmap.values()})
        for name in (*opcode._specialized_opmap, *opcode._ir_handlers):
            if (opcode._ir_handlers.get(name, 0) >= _opcode.IR_FIRST_ARM
                    or name in TRANSLATOR_ONLY):
                continue
            self.assertIn(name, BY_NAME)

    def test_stack_effects(self):
        # The generic handler of an instruction and the
        # handlers of its operations have its stack effect, which is what the
        # compiler counts (_PyOpcode_num_popped() and _pushed()).  A
        # specialisation has its own, for the opargs it specialises.
        for h, row in ROWS.items():
            op = row.base
            if (op >= opcode.MIN_INSTRUMENTED_OPCODE
                    or (row.shadow is not None
                        and opcode.opname[row.shadow] in opcode._specialized_opmap)):
                continue
            for oparg in OPARGS:
                if not _opcode.has_arg(op) and oparg:
                    continue
                try:
                    expected = _opcode.stack_effect(
                        op, oparg if _opcode.has_arg(op) else None)
                except ValueError:
                    continue
                with self.subTest(handler=opcode._all_opname[h], oparg=oparg):
                    reads = extent(row.reads, oparg)
                    writes = extent(row.writes, oparg)
                    through = extent(row.through, oparg)
                    self.assertEqual(writes - reads, expected)
                    self.assertLessEqual(through, min(reads, writes))

    def test_window_and_fields(self):
        for h, row in ROWS.items():
            with self.subTest(handler=opcode._all_opname[h]):
                roles = row.roles
                inputs = [r - _opcode.IR_ROLE_IN0 for r in roles
                          if r >= _opcode.IR_ROLE_IN0]
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
            "STORE_FAST": _opcode.IR_ROLE_LOCAL_OUT,
            "MAKE_CELL": _opcode.IR_ROLE_LOCAL_OUT,
            "LOAD_DEREF": _opcode.IR_ROLE_CELL,
            "STORE_DEREF": _opcode.IR_ROLE_CELL,
            "DELETE_DEREF": _opcode.IR_ROLE_CELL,
        }
        for name in present(*expected):
            role = expected[name]
            with self.subTest(name=name):
                self.assertEqual(oparg_role(BY_NAME[name]), role)
        # Exactly the opcodes whose oparg names a local or a cell have slot
        # roles, except the stack ops that regcfg.c lowers by their own rules
        stack_only = {"LOAD_FAST", "LOAD_FAST_BORROW", "LOAD_FAST_CHECK",
                      "LOAD_FAST_AND_CLEAR", "DELETE_FAST"}
        named = {opcode.opname[op] for op in (*opcode.haslocal, *opcode.hasfree)
                 if op < opcode.MIN_INSTRUMENTED_OPCODE} - stack_only
        slots = {name for name, row in BY_NAME.items()
                 if row.base < opcode.MIN_INSTRUMENTED_OPCODE
                 and oparg_role(row) in SLOT_ROLES}
        bases = {opcode.opname[BY_NAME[name].base] for name in slots}
        self.assertLessEqual(bases, named)

    def test_targets(self):
        for name, op in opcode.opmap.items():
            handler = opcode._ir_handlers.get(name)
            row = None if handler is None else CONTRACT[handler]
            if row is None or op >= opcode.MIN_INSTRUMENTED_OPCODE:
                continue
            with self.subTest(name=name):
                target = row.flags & (_opcode.IR_JUMPS | _opcode.IR_FORWARD)
                # CLEANUP_THROW is a jump only in the IR: it names the word
                # that its handler continues at, which co_code does not
                self.assertEqual(bool(target), op in opcode.hasjump
                                 or name == "CLEANUP_THROW")
        self.assertLessEqual(
            {name for name in names_with(_opcode.IR_NAMES_TARGET, "flags")
             if not name.startswith("INSTRUMENTED_")},
            {"END_ASYNC_FOR"})
        always = {name for name in names_with(_opcode.IR_JUMP_ALWAYS, "flags")
                  if not name.startswith("INSTRUMENTED_")}
        for name in always:
            self.assertTrue(BY_NAME[name].flags
                            & (_opcode.IR_JUMPS | _opcode.IR_FORWARD), name)
            self.assertNotIn(name, ("FOR_ITER", "SEND", "POP_JUMP_IF_TRUE"))

    def test_feedback(self):
        kinds = dis._FEEDBACK_KINDS
        for h, row in ROWS.items():
            # A twin shares the format, and so the entry, of its family
            family = opcode.opname[row.base].removeprefix("INSTRUMENTED_")
            with self.subTest(handler=opcode._all_opname[h]):
                if row.fb == _opcode.IR_LOC_NONE:
                    self.assertEqual((row.fb_kind, row.fb_size), (0, 0))
                    continue
                self.assertIn(family, FEEDBACK)
                self.assertEqual((kinds[row.fb_kind], row.fb_size),
                                 FEEDBACK[family])
        # Every family that has an entry has one in the generic form
        for family in FEEDBACK:
            if family in BY_NAME:
                self.assertNotEqual(BY_NAME[family].fb, _opcode.IR_LOC_NONE)
        # The generic handlers that specialise from their entry
        self.assertLessEqual(
            {opcode._all_opname[op] for op in range(_opcode.IR_FIRST_ARM)
             if CONTRACT[op] is not None
             and not opcode._all_opname[op].startswith("INSTRUMENTED_")
             and op not in opcode._ir_only_handlers
             and CONTRACT[op].effects & _opcode.IR_SPECIALIZES},
            # GET_ITER and FOR_ITER specialise again at step 3
            set(FEEDBACK) - {"GET_ITER", "FOR_ITER"})

    def test_feedback_tables(self):
        # The assembler writes each entry's kind into the code object's
        # feedback table from the contract.
        import email.message, json.decoder
        seen = set()
        for module in (dis, email.message, json.decoder, unittest.case):
            for co in _code_objects(module):
                ir = dis.IRCode(co)
                fbinit = ir.co_fbinit
                for _, h, _, _, _, fb in dis._unpack_opargs(ir.co_code, fbinit):
                    if not fb or fbinit is None:
                        continue
                    kind, _, units = dis._feedback_entry(fbinit, fb)
                    self.assertGreaterEqual(units * 8, CONTRACT[h].fb_size)
                    self.assertEqual(kind, CONTRACT[h].fb_kind,
                                     opcode._all_opname[h])
                    seen.add(dis._FEEDBACK_KINDS[kind])
        # COUNTER is entry 0, which no word names
        # (No word names BRANCH: a conditional jump keeps its operand's type
        # in a unary entry, BranchTruth's Fb<Truth>.)
        self.assertEqual(seen, set(dis._FEEDBACK_KINDS) - {"COUNTER", "BRANCH"})

    def test_monitoring(self):
        events = sys.monitoring.events
        for h, row in ROWS.items():
            name = opcode._all_opname[h]
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
                self.assertTrue(opcode._all_opname[twin].startswith("INSTRUMENTED_"))
                self.assertEqual(ROWS[twin].twin, twin)
                self.assertEqual(row.events, ROWS[twin].events)
                self.assertEqual(row.flags, ROWS[twin].flags)
                self.assertTrue(ROWS[twin].effects & _opcode.IR_RUNS_PYTHON)
        expected = {
            "INSTRUMENTED_CALL_KW": {"CALL", "C_RETURN", "C_RAISE"},
            "INSTRUMENTED_LINE": {"LINE"},
            "INSTRUMENTED_INSTRUCTION": {"INSTRUCTION"},
            "INSTRUMENTED_YIELD_VALUE": {"PY_YIELD"},
            "INSTRUMENTED_YIELD_DELEGATED": {"PY_YIELD"},
            "INSTRUMENTED_END_SEND": {"STOP_ITERATION", "BRANCH_LEFT"},
            "SEND": {"STOP_ITERATION", "BRANCH_LEFT"},
        }
        for name in present(*expected):
            names = expected[name]
            with self.subTest(name=name):
                bits = BY_NAME[name].events
                for event in names:
                    self.assertTrue(bits & getattr(events, event), event)
        if "CALL_KW" in BY_NAME:
            self.assertEqual(BY_NAME["CALL_KW"].twin, opcode.opmap["INSTRUMENTED_CALL_KW"])

    def test_monitoring_run(self):
        # Every declared event observed in a code object belongs to one of
        # its semantic handlers or a transparent observer added to any code.
        # This is a code-object check: branch callbacks identify the semantic
        # branch site even when another handler reports the outcome.
        events = sys.monitoring.events
        twin_events = 0
        for row in CONTRACT:
            if row is not None:
                twin_events |= row.events
        # LINE and INSTRUCTION are every word's: the displacing arms of the
        # Line and Instruction families report them
        added = events.LINE | events.INSTRUCTION
        twin_events |= added

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
            # A register instruction arm declares its family's events
            for _, h, *_ in dis._unpack_opargs(dis.IRCode(co).co_code):
                if h < len(CONTRACT) and CONTRACT[h] is not None:
                    allowed |= CONTRACT[h].events
            with self.subTest(code=co.co_name):
                self.assertEqual(fired.get(co, 0) & ~allowed, 0)

    def test_effects(self):
        for h, row in ROWS.items():
            effects, flags = row.effects, row.flags
            with self.subTest(handler=opcode._all_opname[h]):
                # The format runs code if any of its handlers may
                if effects & _opcode.IR_RUNS_PYTHON:
                    self.assertTrue(flags & _opcode.IR_RUNS_CODE)
                if effects & _opcode.IR_PURE:
                    # POP_TOP is pure but escapes: the release may run code
                    self.assertFalse(effects & _opcode.IR_RAISES)
                if effects & (_opcode.IR_PUSHES_FRAME | _opcode.IR_SUSPENDS):
                    self.assertTrue(effects & _opcode.IR_RUNS_PYTHON)
                if effects & _opcode.IR_SPECIALIZES:
                    name = opcode._all_opname[h]
                    self.assertNotIn(name, opcode._specialized_opmap)
                    self.assertFalse(name.startswith("INSTRUMENTED_"))
                    self.assertNotEqual(row.fb, _opcode.IR_LOC_NONE)
        suspends = {opcode._all_opname[h] for h, row in ROWS.items()
                    if row.effects & _opcode.IR_SUSPENDS}
        # A send to a native iterator that suspends yields its value from
        # the outer frame, which the record states as a YIELD transfer
        self.assertLessEqual(suspends, {"YIELD_VALUE", "INSTRUMENTED_YIELD_VALUE",
                                        "YIELD_DELEGATED", "INSTRUMENTED_YIELD_DELEGATED",
                                        "SEND", "SEND_VIRTUAL", "SEND_ASYNC_GEN"})
        for name in ("CALL_KW_PY", "SEND", "SEND_GEN", "FOR_ITER_GEN"):
            if name in BY_NAME:
                self.assertTrue(BY_NAME[name].effects & _opcode.IR_PUSHES_FRAME,
                                name)
        for name in ("LOAD_COMMON_CONSTANT",):
            if name in BY_NAME:
                self.assertFalse(BY_NAME[name].flags & _opcode.IR_RUNS_CODE, name)
        for name in ("CALL_KW", "STORE_FAST"):
            if name in BY_NAME:
                self.assertTrue(BY_NAME[name].flags & _opcode.IR_RUNS_CODE, name)
        for name in ("COMPARE_OP_INT", "Binary.Int"):
            if name in BY_NAME:
                self.assertTrue(BY_NAME[name].effects & _opcode.IR_DEOPTS)


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
