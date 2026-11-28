"""gdb walks through the baseline JIT's compiled units: each registers its
symbol and unwind info through gdb's JIT interface (Python/jit_unwind.c)."""

import re
import sys
import textwrap
import unittest

from test import support

from .util import setup_module, run_gdb

try:
    import _testinternalcapi
except ImportError:
    _testinternalcapi = None


def setUpModule():
    setup_module()


@unittest.skipUnless(sys._jit.is_available(), "the JIT is not available")
@unittest.skipIf(_testinternalcapi is None, "requires _testinternalcapi")
class JitBacktraceTests(unittest.TestCase):

    def test_bt_through_units(self):
        source = textwrap.dedent("""
            import operator
            import _testinternalcapi

            def build(n):
                if n == 0:
                    return _testinternalcapi.gnu_backtrace_unwind()
                return operator.call(build, n - 1)

            # Under always a code compiles before its words specialise, and
            # a word specialises at its second run: a word that specialises
            # in a unit goes on in the evaluator until the frame's next
            # switch.  The third walk is the one checked.
            build(3)
            build(3)
            build(3)
        """)
        out, err = run_gdb(
            '--eval-command=set breakpoint pending yes',
            '--eval-command=break gnu_backtrace_unwind',
            '--eval-command=ignore 1 2',
            '--eval-command=run',
            '--eval-command=bt',
            '--args', sys.executable, '-X', 'jit=always', '-S', '-c', source)
        frames = [line for line in out.splitlines() if line.startswith("#")]
        units = [i for i, line in enumerate(frames) if "py::jit:build:" in line]
        # build(3) to build(0) each run in a unit
        self.assertGreaterEqual(len(units), 4, out)
        # and the walk goes on past the last unit to the program's entry
        # (main may be a tail call, so gdb can also walk on into libc)
        self.assertTrue(
            any(re.search(r"\b(main|Py_BytesMain|pymain_main) \(", line)
                for line in frames[units[-1] + 1:]),
            out)

    def test_bt_from_a_call_in_a_unit(self):
        # A native call from compiled code: the third call of the loop runs
        # in its unit (a site that specialises leaves the unit by its cold
        # sample, and the code compiles again at the loop's next event).  The
        # walk goes through the unit, which runs on the driver's frame, to
        # ir_evaluate() and the evaluation loop
        source = textwrap.dedent("""
            import _testinternalcapi

            def loop(f):
                for i in range(3):
                    f()
            # The marker, the breakpoint's first hit
            _testinternalcapi.gnu_backtrace_unwind()
            loop(_testinternalcapi.gnu_backtrace_unwind)
        """)
        out, err = run_gdb(
            '--eval-command=set breakpoint pending yes',
            '--eval-command=break gnu_backtrace_unwind',
            '--eval-command=run',
            '--eval-command=ignore 1 2',
            '--eval-command=continue',
            '--eval-command=bt',
            '--args', sys.executable, '-X', 'jit=always', '-S', '-c', source)
        frames = [line for line in out.splitlines() if line.startswith("#")]
        self.assertIn("gnu_backtrace_unwind", frames[0], out)
        names = [re.sub(r"^#\d+\s+(0x\w+ in )?", "", line).split(" ")[0]
                 for line in frames]
        unit = next(i for i, name in enumerate(names)
                    if name.startswith("py::jit:loop:"))
        self.assertEqual(names[unit + 1], "ir_evaluate", out)
        self.assertTrue(
            any(name.startswith("_PyEval_EvalFrame") for name in names[unit + 2:]),
            out)


if __name__ == "__main__":
    unittest.main()
