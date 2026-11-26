"""The inline policy of the register instruction rows holds in the binary:
the interpreter makes no call to a row declared `inline`, and the forms and
stubs share one out-of-line copy of it (Tools/cases_generator/rop_cgen.py)."""

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


def run(*argv: str) -> str:
    return subprocess.run(argv, capture_output=True, text=True, check=True).stdout


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
        cls.calls = run("objdump", "-d", "--no-show-raw-insn",
                        "--disassemble=ir_evaluate", sys.executable)

    def test_the_declared_rows_are_declared(self):
        self.assertIn("_PyRop_PushExact", self.symbols)

    def test_the_interpreter_calls_no_declared_row(self):
        for symbol in self.symbols:
            with self.subTest(symbol=symbol):
                # a call or tail jump to the row, or to a clone of it
                self.assertNotRegex(self.calls,
                                    rf"(call|jmp)\s+\w+ <{symbol}(_Outlined)?[.>]")

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
