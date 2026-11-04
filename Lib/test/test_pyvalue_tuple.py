"""Storage, ownership and failure boundaries of value-based tuples."""

import gc
import math
import unittest
import weakref

from test.support import import_helper

_testinternalcapi = import_helper.import_module('_testinternalcapi')


class PyValueTupleTests(unittest.TestCase):
    def test_canonical_storage_boundaries(self):
        class IntSubclass(int):
            pass

        nan = float('nan')
        values = (-(2**50), 2**50 - 1, -(2**50) - 1, 2**50,
                  0.0, -0.0, math.inf, nan, True, None, IntSubclass(3))
        self.assertEqual(_testinternalcapi.tuple_value_layout(values),
                         'iioofffoooo')
        copied = values[::2] + values[1::2]
        self.assertEqual(_testinternalcapi.tuple_value_layout(copied),
                         'ioffooiofoo')
        self.assertIs(values[7], nan)
        self.assertEqual(math.copysign(1, values[5]), -1)

    def test_owned_object_bridge_and_cycles(self):
        class Item:
            pass

        item = Item()
        reference = weakref.ref(item)
        values = (item, 100000, 1.25)
        owned = _testinternalcapi.tuple_value_object(values, 0)
        del item, values
        gc.collect()
        self.assertIs(reference(), owned)
        owned.cycle = (owned, 42, 2.5)
        del owned
        gc.collect()
        self.assertIsNone(reference())

    def test_hash_equality_and_subclass_comparison(self):
        for value in (-1, -(2**31), 2**31 - 1, -(2**50), 2**50 - 1, 0, 100000):
            self.assertEqual(hash((value,)), hash((float(value),)))
            self.assertEqual((value,), (float(value),))
        self.assertEqual(hash((0.0,)), hash((-0.0,)))
        nan = float('nan')
        self.assertEqual((nan,), (nan,))

        calls = []

        class Special(int):
            def __eq__(self, other):
                calls.append(other)
                return True

        self.assertEqual((Special(3),), (999,))
        self.assertEqual(calls, [999])

    def test_repetition_owns_every_object_edge(self):
        class Item:
            pass

        item = Item()
        reference = weakref.ref(item)
        values = (item, 100000, 1.25) * 9
        del item
        sliced = values[::3]
        del values
        gc.collect()
        self.assertIsNotNone(reference())
        self.assertTrue(all(value is reference() for value in sliced))
        del sliced
        gc.collect()
        self.assertIsNone(reference())

    def test_iterator_boxing_errors(self):
        _testinternalcapi.tuple_value_boxing_errors()

    def test_virtual_iteration_uses_language_values(self):
        cases = ([100000, 1.25, -0.0, None],
                 (100000, 1.25, -0.0, None), b'\x00\x7f\xff', 'aλ𐀀')
        for sequence in cases:
            for _ in range(40):
                result = [value for value in sequence]
            self.assertEqual(result, list(sequence))
            if isinstance(sequence, (list, tuple)):
                self.assertEqual(_testinternalcapi.tuple_value_layout(tuple(result)),
                                 'iffo')
                self.assertEqual(math.copysign(1, result[2]), -1)
            elif isinstance(sequence, bytes):
                self.assertEqual(_testinternalcapi.tuple_value_layout(tuple(result)),
                                 'iii')

    def test_value_member_storage_and_absence(self):
        member = _testinternalcapi.ValueMember()
        self.assertEqual(member.kind(), 'n')
        self.assertIsNone(member.optional)
        with self.assertRaises(AttributeError):
            _ = member.value
        for value, kind in ((100000, 'i'), (1.25, 'f'), (None, 'o')):
            member.value = value
            self.assertEqual(member.kind(), kind)
            self.assertEqual(member.value, value)
            self.assertEqual(member.readonly, value)
        with self.assertRaises(AttributeError):
            member.readonly = 4
        del member.value
        with self.assertRaises(AttributeError):
            del member.value
        del member.optional
        self.assertIsNone(member.optional)

    def test_value_member_publish_before_finalizer_and_alias(self):
        member = _testinternalcapi.ValueMember()
        observed = []

        class Previous:
            def __del__(self):
                observed.append(member.value)

        member.value = Previous()
        member.value = 100000
        self.assertEqual(observed, [100000])

        class Item:
            pass

        item = Item()
        reference = weakref.ref(item)
        member.value = item
        member.value = member.value
        del item
        self.assertIsNotNone(reference())
        del member.value
        self.assertIsNone(reference())


if __name__ == '__main__':
    unittest.main()
