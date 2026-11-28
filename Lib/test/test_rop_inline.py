"""The inline policy of the register instruction rows holds in the binary:
the interpreter makes no call to a row declared `inline`, and the binary has
no out-of-line copy of it (Tools/cases_generator/rop_cgen.py).
An arm whose closes leave it by a tail call (lower_tail_closes) and whose
name operands are read unchecked makes no call and allocates no stack frame.
An arm that publishes a span lets no address of its frame escape, so it needs
no stack protector.
The interpreter is ir_evaluate() and, when it tail-calls, the function of
each handler, label and routine (ir_tail_*, Python/ceval_tail.h)."""

import re
import shutil
import subprocess
import sys
import sysconfig
import unittest
from pathlib import Path

from test import test_tools

test_tools.skip_if_missing("cases_generator")
with test_tools.imports_under_tool("cases_generator"):
    import rop_cgen
    import rop_check

ROOT = Path(test_tools.basepath)


def inline_symbols() -> list[str]:
    program = rop_check.load(sorted((ROOT / "Python" / "register_ops").glob("*.rop")))
    assert not program.diagnostics
    return [x.c.symbol for x in rop_cgen.inline_rows(program)]


# Arms whose only calls were the deallocations of their closes before a
# dispatching exit
TAIL_CLOSE_ONLY = [
    "LocalRead_Bound", "LocalWrite_Store", "GlobalRead_Module",
    "GlobalRead_Builtin", "GetItem_ListIndex", "SetItem_ListIndex",
    "GetAttr_PolyShape", "BranchNone_Test", "BranchTruth_Plain_BRANCH_IF_FALSE",
    "ForIter_List",
]

# Arms whose other call was the release-build check of a name operand, which
# the generator reads through _PyRop_NameAt() unchecked
NAME_READ_ONLY = [
    "GetAttr_Shape", "GetAttr_Shape_TL", "GetAttr_ValueSlot",
    "GetAttr_ValueSlot_TL", "GetAttr_Instance", "GetAttr_Module",
    "GetMethod_MethodValues", "GetMethod_MethodValues_TTL",
    "GetMethod_NoDict", "GetMethod_NoDict_TTL", "GetMethod_Module",
    "AttrWrite_Shape", "AttrWrite_Shape_LL", "AttrWrite_ShapeAdd",
    "AttrWrite_ShapeAdd_LL",
]

# Arms that publish a span (`d[] = v`); _PyRop_PublishItemsRest() takes the
# span and the items by value
SPAN_PUBLISH = ["Unpack_Tuple", "Unpack_List"]


def run(*argv: str) -> str:
    return subprocess.run(argv, capture_output=True, text=True, check=True).stdout


def interpreter_code(text: str) -> str:
    """The disassembly of the interpreter's functions, from one objdump run
    over the range that holds them"""
    funcs = {}
    for address, size, name in re.findall(
            r"^([0-9a-f]+) ([0-9a-f]+) [tT] (ir_evaluate|ir_tail_\w+)$", text, re.M):
        funcs[name] = (int(address, 16), int(size, 16))
    start = min(a for a, _ in funcs.values())
    stop = max(a + n for a, n in funcs.values())
    dump = run("objdump", "-d", "--no-show-raw-insn", f"--start-address={start:#x}",
               f"--stop-address={stop:#x}", sys.executable)
    # keep the blocks of the interpreter's functions only
    blocks = re.split(r"^(?=[0-9a-f]+ <)", dump, flags=re.M)
    return "".join(b for b in blocks
                   if re.match(r"[0-9a-f]+ <(ir_evaluate|ir_tail_\w+)>:", b))


def arm_body(code: str, arm: str) -> str | None:
    """The disassembly of the function of the arm, from interpreter_code()"""
    match = re.search(rf"^[0-9a-f]+ <ir_tail_{arm}>:\n(.*?)(?=^[0-9a-f]+ <|\Z)",
                      code, re.M | re.S)
    return match and match.group(1)


@unittest.skipUnless(shutil.which("nm") and shutil.which("objdump"),
                     "needs nm and objdump")
class InlineRowBinaryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if sysconfig.get_config_var("Py_DEBUG"):
            raise unittest.SkipTest(
                "a Py_DEBUG build does not optimize, so it inlines no row")
        cls.symbols = inline_symbols()
        # (address, size, name) of each text symbol
        cls.text = run("nm", "-S", sys.executable)
        if not re.search(r" [tT] ir_evaluate$", cls.text, re.M):
            raise unittest.SkipTest("ir_evaluate is not in this executable's symbols")
        cls.calls = interpreter_code(cls.text)

    def test_the_declared_rows_are_declared(self):
        self.assertIn("_PyRop_PushExact", self.symbols)

    def test_the_interpreter_calls_no_declared_row(self):
        for symbol in self.symbols:
            with self.subTest(symbol=symbol):
                # a call or tail jump to the row, or to a clone of it
                self.assertNotRegex(self.calls,
                                    rf"(call|jmp)\s+\w+ <{symbol}[.>]")

    def test_tail_closes_leave_no_call_and_no_frame(self):
        if sysconfig.get_config_var("Py_GIL_DISABLED"):
            self.skipTest("the free-threaded build closes in place")
        if "-DNDEBUG" not in sysconfig.get_config_var("PY_CFLAGS").split():
            self.skipTest("an assert() is a call")
        for arm in TAIL_CLOSE_ONLY + NAME_READ_ONLY:
            with self.subTest(arm=arm):
                body = arm_body(self.calls, arm)
                self.assertIsNotNone(body)
                self.assertNotRegex(body, r"\scall\s")
                self.assertNotRegex(body, r"\ssub\s+\$0x[0-9a-f]+,%rsp")

    def test_span_publishes_need_no_stack_protector(self):
        if "-DNDEBUG" not in sysconfig.get_config_var("PY_CFLAGS").split():
            self.skipTest("a row that asserts is not inlined, so its "
                          "out-parameter (Unpack.List's items) escapes")
        for arm in SPAN_PUBLISH:
            with self.subTest(arm=arm):
                body = arm_body(self.calls, arm)
                self.assertIsNotNone(body)
                # the canary read of -fstack-protector
                self.assertNotIn("%fs:0x28", body)

    def test_no_row_has_an_out_of_line_copy(self):
        for symbol in self.symbols:
            with self.subTest(symbol=symbol):
                names = re.findall(rf" [tT] ({symbol}(?:\.[a-z]+\.\d+|\.\d+)*)$",
                                   self.text, re.M)
                names = [n for n in names if not n.endswith(".cold")]
                self.assertEqual(names, [])


@unittest.skipUnless(shutil.which("nm"), "needs nm")
class EvaluatorLayoutTests(unittest.TestCase):
    """The normal evaluator's functions are one contiguous block, in the order
    IR_TARGET_LAYOUT states: the arms of the families declared `hot` in the
    .rop sources, less their generic arms, before every other arm
    (Python/ceval_tail.h)"""

    @classmethod
    def setUpClass(cls):
        text = run("nm", "-n", sys.executable)
        cls.addr = {}
        cls.order = []          # every text symbol, by address
        for m in re.finditer(r"^([0-9a-f]+) [tT] (\S+)$", text, re.M):
            cls.order.append(m.group(2))
            cls.addr.setdefault(m.group(2), int(m.group(1), 16))
        if "ir_tail_Call_Exact" not in cls.addr:
            raise unittest.SkipTest("the interpreter's functions are not in this executable's symbols")
        program = rop_check.load(sorted((ROOT / "Python" / "register_ops").glob("*.rop")))
        assert not program.diagnostics
        cls.hot, cls.rest = [], []
        for name, arm in program.arms.items():
            fam = program.families[arm.family]
            symbol = "ir_tail_" + name.replace(".", "_")
            if symbol not in cls.addr:
                continue
            is_hot = fam.hot and name != f"{fam.name}.{fam.generic}"
            (cls.hot if is_hot else cls.rest).append(symbol)

    def test_hot_arms_come_before_the_others(self):
        self.assertGreater(len(self.hot), 20)
        last_hot = max(self.addr[s] for s in self.hot)
        first_rest = min(self.addr[s] for s in self.rest)
        self.assertLess(last_hot, first_rest)

    def test_the_arms_are_one_block_with_no_other_function_in_it(self):
        arms = self.hot + self.rest
        first = min(self.addr[s] for s in arms)
        last = max(self.addr[s] for s in arms)
        # A declared region's entry for compiled code is an alias of its
        # function (IR_OUTLINED_HOT_JIT_ENTRY): another name, no other code
        own = {self.addr[s] for s in self.order if s.startswith("ir_tail_")}
        inside = [s for s in self.order
                  if first <= self.addr[s] <= last
                  and not s.startswith("ir_tail_")
                  and not s.startswith("ir_observe_")
                  and not (s.startswith("_PyEval_JitOut_Out_")
                           and self.addr[s] in own)]
        self.assertEqual(inside, [])


if __name__ == "__main__":
    unittest.main()
