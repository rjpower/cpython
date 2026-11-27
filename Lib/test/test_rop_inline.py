"""The inline policy of the register instruction rows holds in the binary:
the interpreter makes no call to a row declared `inline`, and the forms and
stubs share one out-of-line copy of it (Tools/cases_generator/rop_cgen.py).
An arm whose closes leave it by a tail call (lower_tail_closes) makes no
call and allocates no stack frame.
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
                                    rf"(call|jmp)\s+\w+ <{symbol}(_Outlined)?[.>]")

    def test_tail_closes_leave_no_call_and_no_frame(self):
        if sysconfig.get_config_var("Py_GIL_DISABLED"):
            self.skipTest("the free-threaded build closes in place")
        if "-DNDEBUG" not in sysconfig.get_config_var("PY_CFLAGS").split():
            self.skipTest("an assert() is a call")
        for arm in TAIL_CLOSE_ONLY:
            with self.subTest(arm=arm):
                match = re.search(rf"^[0-9a-f]+ <ir_tail_{arm}>:\n(.*?)(?=^[0-9a-f]+ <|\Z)",
                                  self.calls, re.M | re.S)
                self.assertIsNotNone(match)
                body = match.group(1)
                self.assertNotRegex(body, r"\scall\s")
                self.assertNotRegex(body, r"\ssub\s+\$0x[0-9a-f]+,%rsp")

    def test_each_row_has_one_outlined_copy_and_no_other(self):
        for symbol in self.symbols:
            with self.subTest(symbol=symbol):
                names = re.findall(rf" [tT] ({symbol}(?:_Outlined)?(?:\.[a-z]+\.\d+|\.\d+)*)$",
                                   self.text, re.M)
                names = [n for n in names if not n.endswith(".cold")]
                self.assertEqual([n.split(".")[0] for n in names],
                                 [f"{symbol}_Outlined"], names)


if __name__ == "__main__":
    unittest.main()
