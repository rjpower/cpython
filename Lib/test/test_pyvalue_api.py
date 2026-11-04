"""Value API packet V1 (pyjit/design/value-api.md): the entry primitives of
section 1.3 and the value format conversions of section 4."""

import math
import unittest
from test import support
from test.support import import_helper
from test.support.script_helper import assert_python_failure

_testinternalcapi = import_helper.import_module("_testinternalcapi")


class Number(int):
    pass


class Item:
    def __repr__(self):
        return "<item é>"


class BadRepr:
    def __repr__(self):
        raise ZeroDivisionError("repr")


def qualified_name(tp, sep="."):
    if tp.__module__ == "builtins":
        return tp.__qualname__
    return f"{tp.__module__}{sep}{tp.__qualname__}"


VALUES = [5, -7, 2**31 - 1, -2**31, 2**31, 2**40, 2.5, -0.0, math.inf,
          float("nan"), True, Number(3), "é", None, [1, "a"], Item()]


class BorrowedEntryTests(unittest.TestCase):
    def check(self, obj, kind):
        got_kind, back, same_bits = (
            _testinternalcapi.pyvalue_from_object_borrow_inline(obj))
        self.assertEqual(got_kind, kind)
        self.assertTrue(same_bits)
        if kind == "object":
            self.assertIs(back, obj)
        else:
            self.assertIs(type(back), type(obj))
            self.assertEqual(math.copysign(1, back), math.copysign(1, obj))
            self.assertEqual(back, obj)

    def test_inline_ints(self):
        for obj in (0, 5, -7, 2**31 - 1, -2**31):
            with self.subTest(obj=obj):
                self.check(obj, "int")

    def test_inline_floats(self):
        for obj in (2.5, -0.0, 0.0, math.inf, -math.inf):
            with self.subTest(obj=obj):
                self.check(obj, "float")

    def test_objects(self):
        for obj in (2**31, -2**31 - 1, 2**100, float("nan"), True, False,
                    Number(3), "é", None, [1], int, Item()):
            with self.subTest(obj=obj):
                self.check(obj, "object")

    def test_null(self):
        _testinternalcapi.pyvalue_entry_null()


class KnownEntryTests(unittest.TestCase):
    def test_round_trip(self):
        for obj in ("é", int, [1], {}, None, Item(), len, iter([]), True,
                    Number(4)):
            with self.subTest(obj=obj):
                self.assertIs(
                    _testinternalcapi.pyvalue_from_object_known(obj), obj)

    @unittest.skipUnless(support.Py_DEBUG, "the check is in debug builds")
    @support.requires_subprocess()
    def test_debug_check(self):
        for obj in ("5", "2.5", "2**40"):
            with self.subTest(obj=obj):
                code = ("import _testinternalcapi; "
                        f"_testinternalcapi.pyvalue_from_object_known({obj})")
                _, _, err = assert_python_failure("-c", code)
                self.assertIn(b"_PyValue_FromObjectKnown() needs an object",
                              err)


class ValueFormatTests(unittest.TestCase):
    def format(self, fmt, obj, *other):
        return _testinternalcapi.pyvalue_format(fmt, obj, *other)

    def test_conversions(self):
        for obj in VALUES:
            with self.subTest(obj=obj):
                self.assertEqual(self.format("%vR", obj), repr(obj))
                self.assertEqual(self.format("%vS", obj), str(obj))
                self.assertEqual(self.format("%vA", obj), ascii(obj))
                self.assertEqual(self.format("%vT", obj),
                                 qualified_name(type(obj)))
                self.assertEqual(self.format("%#vT", obj),
                                 qualified_name(type(obj), ":"))

    def test_type_names(self):
        for tp in (int, float, Number, Item):
            with self.subTest(tp=tp):
                self.assertEqual(self.format("%vN", tp), qualified_name(tp))
                self.assertEqual(self.format("%#vN", tp),
                                 qualified_name(tp, ":"))
        for obj in (5, 2.5, 2**40, "int"):
            with self.subTest(obj=obj):
                with self.assertRaisesRegex(TypeError,
                                            "%N argument must be a type"):
                    self.format("%vN", obj)

    def test_width_and_precision(self):
        self.assertEqual(self.format("%6vR|", 1000), "  1000|")
        self.assertEqual(self.format("%-6vS|", 2.5), "2.5   |")
        self.assertEqual(self.format("%.2vS", 12345), "12")
        self.assertEqual(self.format("%.3vA", "é"), "'\\x")
        self.assertEqual(self.format("%5.1vT|", 2.5), "    f|")

    def test_mixed_with_object_conversions(self):
        self.assertEqual(self.format("%vR and %R", 1000, [2.5]),
                         "1000 and [2.5]")
        self.assertEqual(self.format("%vS%%%S", -0.0, 7), "-0.0%7")

    def test_error_in_repr(self):
        with self.assertRaisesRegex(ZeroDivisionError, "repr"):
            self.format("%vR", BadRepr())

    def test_invalid(self):
        for fmt in ("%vd", "%vs", "%vU", "%vc", "%lvR", "%vlR", "%vV"):
            with self.subTest(fmt=fmt):
                with self.assertRaisesRegex(SystemError,
                                            "invalid format string"):
                    self.format(fmt, 5)

    def test_err_format(self):
        with self.assertRaisesRegex(ValueError, r"^bad 1000$"):
            _testinternalcapi.pyvalue_err_format(ValueError, "bad %vR", 1000)
        with self.assertRaisesRegex(TypeError, r"^not float$"):
            _testinternalcapi.pyvalue_err_format(TypeError, "not %vT", 2.5)


if __name__ == "__main__":
    unittest.main()
