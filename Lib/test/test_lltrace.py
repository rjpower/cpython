from test.support import ir_dis as dis
import re
import textwrap
import unittest

from test import support
from test.support import os_helper
from test.support.script_helper import assert_python_ok

def example():
    x = []
    for i in range(0):
        x.append(i)
    x = "this is"
    y = "an example"
    print(x, y)


@unittest.skipUnless(support.Py_DEBUG, "lltrace requires Py_DEBUG")
class TestLLTrace(unittest.TestCase):

    def run_code(self, code):
        code = textwrap.dedent(code).strip()
        with open(os_helper.TESTFN, 'w', encoding='utf-8') as fd:
            self.addCleanup(os_helper.unlink, os_helper.TESTFN)
            fd.write(code)
        status, stdout, stderr = assert_python_ok(os_helper.TESTFN)
        self.assertEqual(stderr, b"")
        self.assertEqual(status, 0)
        result = stdout.decode('utf-8')
        if support.verbose:
            print("\n\n--- code ---")
            print(code)
            print("\n--- stdout ---")
            print(result)
            print()
        return result

    def test_lltrace(self):
        stdout = self.run_code("""
            def dont_trace_1():
                a = "a"
                a = 10 * a
            def trace_me():
                for i in range(3):
                    +i
            def dont_trace_2():
                x = 42
                y = -x
            dont_trace_1()
            __lltrace__ = 1
            trace_me()
            del __lltrace__
            dont_trace_2()
        """)
        self.assertIn("IterOpen.", stdout)
        self.assertIn("ForIter.", stdout)
        self.assertIn("Intrinsic.", stdout)
        # Register IR discards the result into the discard slot, the last
        # local of the frame (before any free variables, which trace_me has
        # none of).  The first trace shows the frame's locals.
        match = re.search(r"Resuming frame for 'trace_me'.*\n    locals=\[(.*)\]",
                          stdout)
        self.assertIsNotNone(match)
        discard = match.group(1).count(",")
        self.assertRegex(
            stdout,
            rf"(?m)^    ir Intrinsic\.\w+ {discard} [0-9]+ [0-9]+ [0-9]+$")
        self.assertNotIn("Binary.", stdout)
        self.assertNotIn("Unary.", stdout)

        self.assertIn("'trace_me' in module '__main__'", stdout)
        self.assertNotIn("dont_trace_1", stdout)
        self.assertNotIn("'dont_trace_2' in module", stdout)

    def test_lltrace_different_module(self):
        stdout = self.run_code("""
            from test import test_lltrace
            test_lltrace.__lltrace__ = 1
            test_lltrace.example()
        """)
        self.assertIn("'example' in module 'test.test_lltrace'", stdout)
        self.assertIn('Const.', stdout)
        self.assertIn('ForIter.', stdout)
        self.assertIn('this is an example', stdout)

        # check that the arms at the word indices match the disassembly of
        # the register IR
        instr_map = {i.offset: i for i in dis.get_instructions(example, adaptive=True)}
        for line in stdout.splitlines():
            offset, colon, arm = line.partition(":")
            if not colon:
                continue
            self.assertEqual(instr_map[int(offset)].arm, arm.strip())

    def test_lltrace_does_not_crash_on_subscript_operator(self):
        # If this test fails, it will reproduce a crash reported as
        # bpo-34113. The crash happened at the command line console of
        # debug Python builds with __lltrace__ enabled (only possible in console),
        # when the internal Python stack was negatively adjusted
        stdout = self.run_code("""
            import code

            console = code.InteractiveConsole()
            console.push('__lltrace__ = 1')
            console.push('a = [1, 2, 3]')
            console.push('a[0] = 1')
            print('unreachable if bug exists')
        """)
        self.assertIn("unreachable if bug exists", stdout)

if __name__ == "__main__":
    unittest.main()
