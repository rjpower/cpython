"""Ownership and value behavior of canonical home transfers."""

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
    def test_list_child_local_temporary_and_discard(self):
        def local(seq, index):
            result = seq[index]
            return result

        def temporary(seq, index):
            return seq[index] + 1

        def discard(seq, index):
            seq[index]

        for _ in range(64):
            self.assertEqual(local([2.5], 0), 2.5)
            self.assertEqual(temporary([2.5], 0), 3.5)
            self.assertIsNone(discard([2.5], 0))
        routes = {}
        for function in (local, temporary, discard):
            words = _testinternalcapi.get_ir(function.__code__, True)
            self.assertIn('BINARY_OP_SUBSCR_LIST_INT',
                          [word[0] for word in words])
            routes[function] = next(word for word in words
                                    if word[0] == 'BINARY_OP_SUBSCR_LIST_INT')
        local_count = len(_testinternalcapi.get_co_localskinds(local.__code__))
        self.assertLess(routes[local][1], local_count)
        temp_count = len(_testinternalcapi.get_co_localskinds(temporary.__code__))
        self.assertGreaterEqual(routes[temporary][1], temp_count)
        self.assertLess(routes[temporary][1], 4095)
        self.assertEqual(routes[discard][1], 4095)
        for value in (2, 2.5, 1 << 100, -0.0, float('inf'), float('nan')):
            with self.subTest(value=value):
                result = local([value], -1)
                if isinstance(value, float) and math.isnan(value):
                    self.assertTrue(math.isnan(result))
                    self.assertTrue(math.isnan(temporary([value], 0)))
                else:
                    self.assertEqual(result, value)
                    self.assertEqual(temporary([value], 0), value + 1)
                if value == 0:
                    self.assertEqual(math.copysign(1, result), -1)
                self.assertIsNone(discard([value], 0))
        with self.assertRaises(IndexError):
            local([], 0)

    def test_list_child_survives_last_list_owner_retirement(self):
        retired, references = [], []

        class Child:
            pass

        class Sibling:
            def __del__(self):
                gc.collect()
                retired.append(True)

        def factory():
            child = Child()
            references.append(weakref.ref(child))
            return [child, Sibling()]

        def read(make):
            return make()[0]

        for _ in range(64):
            self.assertEqual(read(lambda: [2.5]), 2.5)
        self.assertIn('BINARY_OP_SUBSCR_LIST_INT',
                      [word[0] for word in _testinternalcapi.get_ir(
                          read.__code__, True)])
        result = read(factory)
        gc.collect()
        self.assertIs(references[0](), result)
        self.assertEqual(retired, [True])
        del result
        gc.collect()
        self.assertIsNone(references[0]())

    def test_yielded_canonical_values_and_owned_child(self):
        def source(make):
            yield make()

        def first(make):
            for item in source(make):
                return item

        for _ in range(64):
            self.assertEqual(first(lambda: 2.5), 2.5)
        for value in (1 << 100, -0.0, float('inf'), float('nan')):
            with self.subTest(value=value):
                result = first(lambda: value)
                if math.isnan(value):
                    self.assertTrue(math.isnan(result))
                else:
                    self.assertEqual(result, value)
                if value == 0:
                    self.assertEqual(math.copysign(1, result), -1)

        class Child:
            pass

        references = []

        def make():
            value = Child()
            references.append(weakref.ref(value))
            return value

        result = first(make)
        gc.collect()
        self.assertIs(references[0](), result)
        del result
        gc.collect()
        self.assertIsNone(references[0]())

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
