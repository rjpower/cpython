"""The typed-borrow units of Tools/build/check_value_api.py, compiled with the
compilers and flags that built this interpreter, and its text rules.

Each misuse of a PyValueBorrow must fail to compile, and the readers must
accept owners and borrows alike (pyjit/design/proposals/register-ir-v2-a5b.md,
Appendix A). The text rules, raw-bits and public-value, hold for this source
tree (pyjit/design/value-api.md, section 1.6). make check-value-api runs the
same units and rules, with the rest of the value-API lint, which needs
clang-query.
"""

import os
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


class TextRuleTests(unittest.TestCase):
    """raw-bits: only the value headers and the code that rule 1 lists know
    the word's encoding. public-value: public headers carry the word
    opaquely, and the value slots are opaque storage."""

    def setUp(self):
        self.srcdir = sysconfig.get_config_var("srcdir")

    def test_raw_bits_self_tests(self):
        for name, (path, text, scan_bits, count) in (
                check_value_api.RAW_BITS_TEXT_TESTS.items()):
            with self.subTest(unit=name):
                findings = check_value_api.raw_bits_text(path, text,
                                                         scan_bits)
                self.assertEqual(len(findings), count, findings)

    def test_box_text_self_tests(self):
        # test-site and boundary-site, including the non-CORE kind tokens
        # and the parenthesised callee that names no For form.
        for name, (path, text, count) in (
                check_value_api.BOX_TEXT_TESTS.items()):
            with self.subTest(unit=name):
                findings = check_value_api.box_text(path, text)
                self.assertEqual(len(findings), count, findings)

    def test_public_value_self_tests(self):
        for name, (text, count) in check_value_api.PUBLIC_VALUE_TESTS.items():
            with self.subTest(unit=name):
                findings = check_value_api.check_public_header(name, text)
                self.assertEqual(len(findings), count, findings)

    def test_encoding_names_defined(self):
        self.assertEqual(check_value_api.encoding_names_defined(self.srcdir),
                         [])

    def test_raw_bits(self):
        # Without the compile database the text reads .bits in headers
        # only; check_value_api.py decides the units with clang-query.
        self.assertEqual(check_value_api.check_raw_bits_text(self.srcdir), [])

    def test_call_text_self_tests(self):
        for name, (path, text, rule, count) in (
                check_value_api.CALL_TEXT_TESTS.items()):
            with self.subTest(unit=name):
                findings = [f for f in
                            check_value_api.window_read_text(path, text)
                            + check_value_api.vectorcall_thunk_text(path, text)
                            if f.rule == rule]
                self.assertEqual(len(findings), count, findings)

    def test_call_text(self):
        # window-read and vectorcall-thunk (value-api-m3.md, sections 1.1
        # and 1.7).
        self.assertEqual(check_value_api.check_call_text(self.srcdir), [])

    def test_public_headers(self):
        self.assertEqual(check_value_api.check_public_headers(self.srcdir), [])

    def test_deferred_units(self):
        # The wave's defer line is reported, not failing, at its line.
        with tempfile.TemporaryDirectory() as tmp:
            with open(f"{tmp}/deferred.c", "w") as f:
                f.write("/* unit */\n#define Py_VALUE_DEFER_MARKS 1\n")
            with open(f"{tmp}/plain.c", "w") as f:
                f.write("#define Py_VALUE_DEFER_MARKS_NOT 1\n")
            entries = [({"directory": tmp, "file": name}, "core")
                       for name in ("deferred.c", "plain.c")]
            findings = check_value_api.deferred_units(entries)
        self.assertEqual([(f.line, f.rule, f.fails) for f in findings],
                         [(2, "deferred", False)])
        self.assertTrue(findings[0].path.endswith("deferred.c"))

    def test_unchecked_value_units(self):
        # A value-native source outside the compile database (a shared
        # module that the database was not recorded for) is reported, not
        # failing; one in the database is not.
        with tempfile.TemporaryDirectory() as tmp:
            os.mkdir(f"{tmp}/Modules")
            with open(f"{tmp}/Modules/shared.c", "w") as f:
                f.write("/* unit */\n#  define Py_VALUE_CORE 1\n")
            with open(f"{tmp}/Modules/plain.c", "w") as f:
                f.write("#define Py_VALUE_CORE_NOT 1\n")
            findings = check_value_api.unchecked_value_units(tmp, set())
            self.assertEqual([(f.path, f.line, f.rule, f.fails)
                              for f in findings],
                             [("Modules/shared.c", 2, "unchecked", False)])
            self.assertEqual(check_value_api.unchecked_value_units(
                tmp, {"Modules/shared.c"}), [])

    def test_included_source_is_not_unchecked(self):
        # A source that a unit includes textually is checked with that unit.
        with tempfile.TemporaryDirectory() as tmp:
            os.makedirs(f"{tmp}/Modules/sub")
            with open(f"{tmp}/Modules/unit.c", "w") as f:
                f.write('#define Py_VALUE_CORE 1\n#include "sub/part.c"\n')
            with open(f"{tmp}/Modules/sub/part.c", "w") as f:
                f.write("#define Py_VALUE_CORE 1\n")
            self.assertEqual(check_value_api.unchecked_value_units(
                tmp, {"Modules/unit.c"}), [])


if __name__ == "__main__":
    unittest.main()
