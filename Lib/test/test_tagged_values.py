"""Tests for the inline int and float encoding of interpreter stack values."""

import gc
import math
import struct
import unittest
from test.support import import_helper

_testinternalcapi = import_helper.import_module("_testinternalcapi")
roundtrip = getattr(_testinternalcapi, "stackref_box_roundtrip", None)


@unittest.skipIf(roundtrip is None, "build has no inline stack values")
class TaggedValuesTest(unittest.TestCase):

    def check(self, value):
        boxed = roundtrip(value)
        self.assertIs(type(boxed), type(value))
        self.assertEqual(boxed, value)
        if isinstance(value, float):
            self.assertEqual(math.copysign(1.0, boxed),
                             math.copysign(1.0, value))

    def test_int32(self):
        for value in (0, 1, -1, 2, 2**31 - 1, -2**31, 1 << 16, -(1 << 16)):
            with self.subTest(value=value):
                self.check(value)

    def test_int_out_of_range(self):
        for value in (2**31, -2**31 - 1, 2**63, -2**100):
            with self.subTest(value=value):
                self.assertRaises(OverflowError, roundtrip, value)

    def test_float(self):
        values = (0.0, -0.0, 1.5, -1.5, math.inf, -math.inf, 5e-324,
                  -5e-324, 1.7976931348623157e308, -1.7976931348623157e308,
                  float(2**31))
        for value in values:
            with self.subTest(value=value):
                self.check(value)

    def test_nan_refused(self):
        # Offset by 2**49, these would alias the inline int 1, a pointer
        # and an index.
        nans = [struct.unpack("<d", struct.pack("<Q", bits))[0]
                for bits in (0xFFFC_0000_0000_0001, 0xFFFE_0000_0000_1000,
                             0xFFFF_FFFF_FFFF_FFFF)]
        for value in (math.nan, -math.nan, *nans):
            with self.subTest(value=value):
                self.assertRaises(ValueError, roundtrip, value)

    def test_box_memory_error(self):
        _testcapi = import_helper.import_module("_testcapi")
        # Values that are not cached, so boxing allocates.
        for value in (2**31 - 1, -2**31, 1.5):
            with self.subTest(value=value):
                gc.collect()  # Empty the free lists.
                _testcapi.set_nomemory(0)
                try:
                    roundtrip(value)
                except MemoryError:
                    raised = True
                else:
                    raised = False
                finally:
                    _testcapi.remove_mem_hooks()
                self.assertTrue(raised)

    def test_other_types_refused(self):
        class Int(int):
            pass

        for value in (True, Int(1), 1j, "1", None):
            with self.subTest(value=value):
                self.assertRaises(TypeError, roundtrip, value)


if __name__ == "__main__":
    unittest.main()
