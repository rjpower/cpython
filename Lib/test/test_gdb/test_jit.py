"""gdb walks through the baseline JIT's compiled units: each registers its
symbol and unwind info through gdb's JIT interface (Python/jit_unwind.c)."""

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

            build(3)
        """)
        out, err = run_gdb(
            '--eval-command=set breakpoint pending yes',
            '--eval-command=break gnu_backtrace_unwind',
            '--eval-command=run',
            '--eval-command=bt',
            '--args', sys.executable, '-X', 'jit=always', '-S', '-c', source)
        units = [line for line in out.splitlines()
                 if "py::jit:build:" in line and line.startswith("#")]
        # build(3) to build(0) each run in a unit
        self.assertGreaterEqual(len(units), 4, out)
        # and the walk ends in main, not at the first unit
        outermost = [line for line in out.splitlines() if line.startswith("#")][-1]
        self.assertRegex(outermost, r" in (main|Py_BytesMain|pymain_main) ", out)


if __name__ == "__main__":
    unittest.main()
