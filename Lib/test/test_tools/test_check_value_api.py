"""The typed-borrow units of Tools/build/check_value_api.py, compiled with the
compilers and flags that built this interpreter, and its public-value rule.

Each misuse of a PyValueBorrow must fail to compile, and the readers must
accept owners and borrows alike (pyjit/design/proposals/register-ir-v2-a5b.md,
Appendix A). make check-value-api runs the same units, with the rest of the
value-API lint, which needs clang-query.
"""

import shlex
import shutil
import sysconfig
import tempfile
import unittest

from test.test_tools import imports_under_tool, skip_if_missing

skip_if_missing("build")
if not sysconfig.is_python_build():
    raise unittest.SkipTest("the units compile against a build tree")

with imports_under_tool("build"):
    import check_value_api


def compiler(name):
    """The command of the build's compiler name (CC or CXX); skip if it is
    not configured or not installed."""
    command = sysconfig.get_config_var(name)
    if not command or shutil.which(shlex.split(command)[0]) is None:
        raise unittest.SkipTest(f"{name} {command!r} is not available")
    return command


class BorrowUnitTests(unittest.TestCase):
    def setUp(self):
        self.directory = sysconfig.get_config_var("abs_builddir")
        self.flags = shlex.split(sysconfig.get_config_var("PY_CORE_CFLAGS"))
        self.tmp = self.enterContext(tempfile.TemporaryDirectory())

    def check(self, units, command, flags, suffix):
        for name, (body, pattern) in units.items():
            with self.subTest(unit=name):
                failure = check_value_api.borrow_unit_failure(
                    self.tmp, self.directory, command, flags, name, body,
                    pattern, suffix)
                self.assertIsNone(failure)

    def test_c(self):
        self.check(check_value_api.BORROW_TESTS, compiler("CC"), self.flags,
                   ".c")

    def test_cxx(self):
        self.check(check_value_api.BORROW_CXX_TESTS, compiler("CXX"),
                   check_value_api.cxx_flags(self.flags), ".cpp")


class PublicValueTests(unittest.TestCase):
    """The public-value rule: public headers carry the word opaquely, and
    the value slots are opaque storage."""

    def test_self_tests(self):
        for name, (text, count) in check_value_api.PUBLIC_VALUE_TESTS.items():
            with self.subTest(unit=name):
                findings = check_value_api.check_public_header(name, text)
                self.assertEqual(len(findings), count, findings)

    def test_public_headers(self):
        srcdir = sysconfig.get_config_var("srcdir")
        self.assertEqual(check_value_api.check_public_headers(srcdir), [])


if __name__ == "__main__":
    unittest.main()
