"""The instruction contract of the register IR.

_opcode.get_ir_contract() exports _PyIR_Formats (Include/internal/pycore_ir.h),
which Tools/cases_generator/ir_metadata_generator.py writes from
Tools/cases_generator/stack_ops.py and Python/register_ops.  The compiler passes, the assembler, dis and the
instrumentation read it.  Each row must agree with the word's encoding and
homes, with the feedback layouts and with sys.monitoring.
"""

from test.support import ir_dis as dis
import os
import sys
import textwrap
import types
import unittest
from test import support
from test import test_tools
from test.support import import_helper, os_helper, script_helper

import _opcode
import _testinternalcapi
from _opcode_metadata import _ir_handlers

CONTRACT = _opcode.get_ir_contract()
ROWS = {h: row for h, row in enumerate(CONTRACT) if row is not None}
BY_NAME = {dis._all_opname[h]: row for h, row in ROWS.items()}
ALL_BY_NAME = BY_NAME

# The feedback entry of some families (Include/internal/pycore_feedback.h):
# its kind and size in bytes
FEEDBACK = {
    "ForIter": ("UNARY", 24),
    "IterOpen": ("UNARY", 24),
    "Send": ("UNARY", 24),
    "CallKw": ("CALL", 40),
    "CallEx": ("CALL", 40),
}



def names_with(bit, key="effects"):
    return {name for name, row in BY_NAME.items() if getattr(row, key) & bit}


class ContractTests(unittest.TestCase):

    def test_normal_physical_transfer(self):
        testapi = import_helper.import_module("_testinternalcapi")
        named = _ir_handlers
        preserve = (0, -1)
        set_null = (1, -1)
        unknown = (2, -1)

        def transfer(name, fields, edge, home, nl=2, limit=10):
            return testapi.ir_normal_physical_transfer(
                named[name], *fields, edge, home, nl, limit)

        # POP_TOP lowers to a retirement: Retire.Close([src]) is
        # A = 1 source, the source in B
        retire = (1, 4, 4095, 4095)
        self.assertEqual(transfer("Retire.Close", retire, 0, 4), set_null)
        self.assertEqual(transfer("Retire.Close", retire, 0, 5), preserve)
        # Successive retirements retain each other's NULL facts.
        self.assertEqual(transfer("Retire.Close", (1, 5, 4095, 4095), 0, 4), preserve)
        self.assertEqual(transfer("Retire.Close", (1, 5, 4095, 4095), 0, 5), set_null)
        self.assertEqual(transfer("Retire.Close", retire, 1, 4), unknown)

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
        calls = {name: row for name, row in ALL_BY_NAME.items()
                 if name.split(".")[0] == "CallKw"}
        self.assertTrue(calls)
        for name, row in calls.items():
            self.assertTrue(row.word_flags & _opcode.IR_F_NO_SELF, name)

    def test_rows(self):
        self.assertEqual(len(CONTRACT), _opcode.IR_NUM_HANDLERS)
        self.assertEqual(len(dis._all_opname), _opcode.IR_NUM_HANDLERS)
        # Every named id has a row; a guard build leaves the ids below its
        # base without either
        named = {h for h, name in enumerate(dis._all_opname) if name is not None}
        self.assertEqual(set(ROWS), named)
        for h, row in ROWS.items():
            with self.subTest(handler=dis._all_opname[h]):
                self.assertTrue(row.flags & _opcode.IR_VALID)
                self.assertEqual(_ir_handlers[dis._all_opname[h]], h)

    def test_fields(self):
        # Sources are numbered from 0 in field order; a GROUP field names
        # the home group; a feedback field is where fb says
        homes = (_opcode.IR_HOME_PRESERVE, _opcode.IR_HOME_VACATE,
                 _opcode.IR_HOME_DEFINE, _opcode.IR_HOME_UNKNOWN)
        for h, row in enumerate(CONTRACT[_opcode.IR_FIRST_ARM:], _opcode.IR_FIRST_ARM):
            with self.subTest(handler=dis._all_opname[h]):
                roles = row.roles
                inputs = [r - _opcode.IR_ROLE_SRC for r in roles
                          if r >= _opcode.IR_ROLE_SRC]
                self.assertEqual(inputs, list(range(len(inputs))))
                # A group takes one GROUP field, which only a group has
                self.assertEqual(roles.count(_opcode.IR_ROLE_GROUP),
                                 1 if row.group else 0)
                if row.flags & _opcode.IR_GROUP_OUT:
                    self.assertTrue(row.group)
                self.assertEqual(_opcode.IR_ROLE_FB in roles,
                                 row.fb != _opcode.IR_LOC_NONE)
                self.assertEqual(len(row.edges), 2)
                for states in row.edges:
                    self.assertEqual(len(states), 6)
                    self.assertTrue(set(states) <= set(homes))

    def test_edge_homes(self):
        # ForIter(item, pair): the step defines the item and keeps the
        # receiver; the exhaustion retires the receiver and the cursor (home
        # 4, the pair's part 1).  Both edges publish before their callbacks.
        row = CONTRACT[_ir_handlers["ForIter.Generic"]]
        self.assertEqual(row.group, 2)
        step, done = row.edges
        self.assertEqual(step[0], _opcode.IR_HOME_DEFINE)
        self.assertEqual(step[1], _opcode.IR_HOME_PRESERVE)
        self.assertEqual((done[1], done[4]),
                         (_opcode.IR_HOME_VACATE, _opcode.IR_HOME_VACATE))
        self.assertEqual(row.committed, 0b11)

    def test_slot_roles(self):
        # A cell is named by a CELL field; the arms of the other families
        # name their locals as homes
        for name in ("CellRead.Get", "CellStore.Set", "CellDelete.Unbind"):
            with self.subTest(name=name):
                self.assertIn(_opcode.IR_ROLE_CELL, BY_NAME[name].roles)

    def test_targets(self):
        # A word names a target word if and only if its family declares a
        # target operand
        tables = _testinternalcapi.arm_operands()
        for name, row in BY_NAME.items():
            family = name.split(".")[0]
            if family not in tables:
                continue
            with self.subTest(name=name):
                declared = any(operand[1] == "target" for operand in tables[family])
                target = row.flags & (_opcode.IR_JUMPS | _opcode.IR_FORWARD)
                self.assertEqual(bool(target), declared)
        self.assertLessEqual(
            {name.split(".")[0] for name in names_with(_opcode.IR_NAMES_TARGET, "flags")},
            {"EndAsyncFor"})
        for name in names_with(_opcode.IR_JUMP_ALWAYS, "flags"):
            self.assertTrue(BY_NAME[name].flags
                            & (_opcode.IR_JUMPS | _opcode.IR_FORWARD), name)
            # ForIter.Gen has no next or branch exit of its own: the generator
            # it enters returns to the body through the frame's return_pc,
            # as Send.Gen's does
            if not name.startswith("ForIter.Gen"):
                self.assertNotEqual(name.split(".")[0], "ForIter")

    def test_feedback(self):
        kinds = dis._FEEDBACK_KINDS
        tables = _testinternalcapi.arm_operands()
        for h, row in ROWS.items():
            # An observer shares the format, and so the entry, of its family
            family = dis._all_opname[h].split(".")[0]
            with self.subTest(handler=dis._all_opname[h]):
                # A family has an entry if and only if it declares a
                # feedback operand
                declared = any(operand[1] == "feedback"
                               for operand in tables.get(family, ()))
                self.assertEqual(declared, row.fb_size != 0)
                if family in FEEDBACK:
                    self.assertEqual((kinds[row.fb_kind], row.fb_size),
                                     FEEDBACK[family])

    def test_feedback_tables(self):
        # The assembler writes each entry's kind into the code object's
        # feedback table from the contract.
        import email.message, json.decoder
        seen = set()
        for module in (dis, email.message, json.decoder, unittest.case):
            for co in _code_objects(module):
                ir = dis.IRCode(co)
                fbinit = ir.co_fbinit
                for word in dis.get_instructions(co):
                    fb, h = word.fb, _ir_handlers[word.arm]
                    if not fb or fbinit is None:
                        continue
                    kind, _, units = dis._feedback_entry(fbinit, fb)
                    self.assertGreaterEqual(units * 8, CONTRACT[h].fb_size)
                    self.assertEqual(kind, CONTRACT[h].fb_kind, word.arm)
                    seen.add(dis._FEEDBACK_KINDS[kind])
        # COUNTER is entry 0, which no word names
        # (No word names BRANCH: a conditional jump keeps its operand's type
        # in a unary entry, BranchTruth's Fb<Truth>.)
        self.assertEqual(seen, set(dis._FEEDBACK_KINDS) - {"COUNTER", "BRANCH"})

    def test_monitoring(self):
        # The observing evaluator's table (Python/ceval_observing.c) sends
        # every arm of a family that declares events to the family's
        # observer, which reads the word with the same format.  The
        # exceptions run themselves: the observers, the trailing jumps, the
        # LINE and INSTRUCTION wrappers and the entry frame's Return.
        events = sys.monitoring.events
        selves = {"Return.External"}
        for h, row in ROWS.items():
            name = dis._all_opname[h]
            with self.subTest(handler=name):
                observer = row.observer
                if observer == h:
                    self.assertTrue(row.events == 0
                                    or name.endswith(".Observed")
                                    or name.endswith(".Trailing")
                                    or name in selves, name)
                    continue
                self.assertTrue(row.events, name)
                observer_name = dis._all_opname[observer]
                self.assertEqual(observer_name,
                                 name.split(".")[0] + ".Observed")
                if row.effects & (_opcode.IR_PUSHES_FRAME | _opcode.IR_SUSPENDS):
                    self.assertTrue(ROWS[observer].effects & _opcode.IR_RUNS_PYTHON)
                self.assertEqual(ROWS[observer].observer, observer)
                self.assertEqual(row.events, ROWS[observer].events)
                # The observer reads the word with its own view of the
                # operands (rop_generator.check_observer_layouts)
                for field in ("roles", "imm", "fb", "group", "vacant",
                              "operands", "fixed_words"):
                    self.assertEqual(getattr(row, field),
                                     getattr(ROWS[observer], field), field)
        expected = {
            "CallKw.Generic": {"CALL", "C_RETURN", "C_RAISE"},
            "Yield.Ordinary": {"PY_YIELD"},
            "Send.Generic": {"STOP_ITERATION", "BRANCH_LEFT"},
        }
        for name, names in expected.items():
            with self.subTest(name=name):
                bits = BY_NAME[name].events
                for event in names:
                    self.assertTrue(bits & getattr(events, event), event)
        self.assertEqual(BY_NAME["CallKw.Generic"].observer,
                         _ir_handlers["CallKw.Observed"])

    def test_monitoring_run(self):
        # Every declared event observed in a code object belongs to one of
        # its semantic handlers or a transparent observer added to any code.
        # This is a code-object check: branch callbacks identify the semantic
        # branch site even when another handler reports the outcome.
        events = sys.monitoring.events
        family_events = 0
        for row in CONTRACT:
            if row is not None:
                family_events |= row.events
        # LINE and INSTRUCTION are every word's: the observing evaluator
        # reports them before a word where monitoring observes them
        added = events.LINE | events.INSTRUCTION
        family_events |= added

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
            if (name.isupper() and bit & family_events and bit & (bit - 1) == 0):
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
            for word in dis.get_instructions(co):
                allowed |= CONTRACT[_ir_handlers[word.arm]].events
            with self.subTest(code=co.co_name):
                self.assertEqual(fired.get(co, 0) & ~allowed, 0)

    def test_effects(self):
        for h, row in ROWS.items():
            effects, flags = row.effects, row.flags
            with self.subTest(handler=dis._all_opname[h]):
                # The format runs code if any of its handlers may
                if effects & _opcode.IR_RUNS_PYTHON:
                    self.assertTrue(flags & _opcode.IR_RUNS_CODE)
                if effects & _opcode.IR_PURE:
                    # A release may run code, but a pure word does not raise
                    self.assertFalse(effects & _opcode.IR_RAISES)
                if effects & (_opcode.IR_PUSHES_FRAME | _opcode.IR_SUSPENDS):
                    # Running the frame is an escape
                    self.assertTrue(effects & _opcode.IR_RUNS_PYTHON)
                if effects & _opcode.IR_SPECIALIZES:
                    self.assertNotEqual(row.fb_size, 0)
        suspends = {dis._all_opname[h] for h, row in ROWS.items()
                    if row.effects & _opcode.IR_SUSPENDS}
        # A send to a native iterator that suspends yields its value from
        # the outer frame, which the record states as a YIELD transfer
        self.assertLessEqual({name.split(".")[0] for name in suspends},
                             {"Yield", "YieldFrom", "Send"})
        for name in ("CallKw.Plan", "Send.Gen"):
            self.assertTrue(BY_NAME[name].effects & _opcode.IR_PUSHES_FRAME,
                            name)
        self.assertFalse(BY_NAME["Const.Common"].flags & _opcode.IR_RUNS_CODE)
        self.assertTrue(BY_NAME["CallKw.Generic"].flags & _opcode.IR_RUNS_CODE)
        self.assertTrue(BY_NAME["Binary.Int"].effects & _opcode.IR_DEOPTS)


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
