"""Ownership and value behavior of canonical local duplicates."""

import gc
import math
import sys
import unittest
import weakref
from test.support import import_helper


_testinternalcapi = import_helper.import_module("_testinternalcapi")
K = 3


def constant_multiply(x):
    return x * K


def call_between_load_and_use(x, call):
    return x + call()


def local_copy(x, call):
    y = x
    call()
    return y


def loads(function, name):
    return [(a, b) for handler, a, b, _, _ in
            _testinternalcapi.get_ir(function.__code__) if handler == name]


class CanonicalHomeTests(unittest.TestCase):
    def test_real_temporary_and_folded_local_routes(self):
        temporary = loads(constant_multiply, "LOAD_FAST_BORROW")
        nl = len(_testinternalcapi.get_co_localskinds(
            constant_multiply.__code__))
        self.assertTrue(any(a >= nl and b == 0 for a, b in temporary))

        local = loads(local_copy, "LOAD_FAST")
        self.assertIn((local_copy.__code__.co_varnames.index("y"), 0), local)

    def test_numeric_values_keep_their_value(self):
        class IntSubclass(int):
            pass

        class FloatSubclass(float):
            pass

        for value, expected in ((2, 6), (2.5, 7.5),
                                (1 << 100, 3 << 100), (IntSubclass(7), 21),
                                (FloatSubclass(1.25), 3.75)):
            with self.subTest(value=value):
                self.assertEqual(constant_multiply(value), expected)
        self.assertTrue(math.isnan(constant_multiply(float("nan"))))

    def test_temporary_survives_callback_local_replacement(self):
        def replace():
            sys._getframe(1).f_locals["x"] = 100
            return 2

        self.assertEqual(call_between_load_and_use(7, replace), 9)
        large = 1 << 100
        self.assertEqual(call_between_load_and_use(large, replace), large + 2)

        class Payload:
            def __add__(self, other):
                return self

        payload = Payload()
        replacement = Payload()

        def replace_payload():
            sys._getframe(1).f_locals["x"] = replacement
            return 2

        result = call_between_load_and_use(payload, replace_payload)
        self.assertIs(result, payload)

    def test_folded_local_keeps_owned_object_after_replacement(self):
        class Payload:
            pass

        payload = Payload()
        replacement = Payload()
        reference = weakref.ref(payload)

        def replace():
            sys._getframe(1).f_locals["x"] = replacement

        result = local_copy(payload, replace)
        self.assertIs(result, payload)
        del payload
        gc.collect()
        self.assertIs(reference(), result)
        del result
        gc.collect()
        self.assertIsNone(reference())


if __name__ == "__main__":
    unittest.main()
