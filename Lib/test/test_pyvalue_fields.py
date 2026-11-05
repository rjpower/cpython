"""Value fields of C types: storage, members, GC, release and reduce."""

import copy
import gc
import math
import pickle
import sys
import unittest

from test.support import import_helper

_testinternalcapi = import_helper.import_module('_testinternalcapi')
ValueFields = _testinternalcapi.ValueFields


class Index:
    def __init__(self, value):
        self.value = value

    def __index__(self):
        return self.value


class Finalized:
    def __init__(self, log):
        self.log = log

    def __del__(self):
        self.log.append('finalized')


class ValueFieldTests(unittest.TestCase):
    def test_empty(self):
        fields = ValueFields()
        self.assertEqual(fields.kinds(), 'nnnn')
        self.assertIsNone(fields.i)
        self.assertIsNone(fields.f)
        self.assertIsNone(fields.o)
        self.assertIsNone(fields.ro)
        with self.assertRaises(AttributeError):
            fields.ex
        self.assertFalse(hasattr(fields, 'ex'))

    def test_storage_by_value(self):
        item = object()
        fields = ValueFields(5, 1.5, item)
        self.assertEqual(fields.kinds(), 'ifon')
        self.assertEqual(fields.i, 5)
        self.assertEqual(fields.f, 1.5)
        self.assertIs(fields.o, item)
        self.assertEqual(fields.ro, 5)

    def test_canonical_numbers(self):
        fields = ValueFields()
        cases = [
            (0, 'i'), (-(2**50), 'i'), (2**50 - 1, 'i'),
            (2**50, 'o'), (-(2**50) - 1, 'o'), (2**60, 'o'),
            (0.5, 'f'), (-0.0, 'f'), (math.inf, 'f'), (-math.inf, 'f'),
            (math.nan, 'o'), (True, 'o'), (None, 'o'), (Index(3), 'o'),
        ]
        for value, kind in cases:
            with self.subTest(value=value):
                fields.ex = value
                self.assertEqual(fields.kinds()[3], kind)
                result = fields.ex
                self.assertIs(type(result), type(value))
                if isinstance(value, float) and math.isnan(value):
                    self.assertIs(result, value)
                else:
                    self.assertEqual(result, value)
                if kind == 'o':
                    self.assertIs(result, value)
        fields.ex = -0.0
        self.assertEqual(math.copysign(1, fields.ex), -1)

    def test_int_subclass_stays_object(self):
        class MyInt(int):
            pass

        value = MyInt(7)
        fields = ValueFields(value)
        self.assertEqual(fields.kinds(), 'onnn')
        self.assertIs(fields.i, value)

    def test_delete_states(self):
        fields = ValueFields(1, 2.0, 'x')
        fields.ex = 4
        self.assertEqual(fields.kinds(), 'ifoi')

        del fields.i
        self.assertEqual(fields.kinds(), 'nfoi')
        self.assertIsNone(fields.i)
        self.assertIsNone(fields.ro)
        # An absent optional field deletes again without error.
        del fields.i

        del fields.ex
        self.assertEqual(fields.kinds(), 'nfon')
        with self.assertRaises(AttributeError):
            fields.ex
        with self.assertRaises(AttributeError):
            del fields.ex

        fields.ex = 2**60
        self.assertEqual(fields.ex, 2**60)

    def test_readonly(self):
        fields = ValueFields(1)
        with self.assertRaises(AttributeError):
            fields.ro = 2
        with self.assertRaises(AttributeError):
            del fields.ro
        self.assertEqual(fields.ro, 1)

    def test_store_releases_previous(self):
        log = []
        fields = ValueFields()
        fields.ex = Finalized(log)
        fields.ex = 1
        self.assertEqual(log, ['finalized'])
        self.assertEqual(fields.kinds()[3], 'i')

    def test_dealloc_releases_objects(self):
        item = object()
        big = 2**60 + 1
        before_item = sys.getrefcount(item)
        before_big = sys.getrefcount(big)
        fields = ValueFields(big, 1.5, item)
        fields.ex = item
        self.assertEqual(sys.getrefcount(item), before_item + 2)
        self.assertEqual(sys.getrefcount(big), before_big + 1)
        del fields
        self.assertEqual(sys.getrefcount(item), before_item)
        self.assertEqual(sys.getrefcount(big), before_big)

    def test_traverse_visits_objects_only(self):
        item = []
        big = 2**60
        fields = ValueFields(5, 1.5, item)
        self.assertTrue(gc.is_tracked(fields))
        self.assertEqual(gc.get_referents(fields), [item])
        fields.ex = big
        referents = gc.get_referents(fields)
        self.assertEqual(len(referents), 2)
        self.assertIs(referents[0], item)
        self.assertIs(referents[1], big)
        # Immortal objects are visited too; the count is not on them.
        fields.i = None
        self.assertIn(None, gc.get_referents(fields))

    def test_cycle_through_field_is_collected(self):
        log = []
        fields = ValueFields(1, 2.5)
        fields.o = fields
        fields.ex = Finalized(log)
        del fields
        gc.collect()
        self.assertEqual(log, ['finalized'])

    def test_clear(self):
        fields = ValueFields(1, 2.5, [])
        fields.ex = 2**60
        _testinternalcapi.slot_clear(fields)
        self.assertEqual(fields.kinds(), 'nnnn')
        self.assertIsNone(fields.i)
        with self.assertRaises(AttributeError):
            fields.ex

    def test_reduce_reads_fields(self):
        item = ('t', 1)
        fields = ValueFields(7, -0.5, item)
        cls, args, state = fields.__reduce__()
        self.assertIs(cls, ValueFields)
        self.assertEqual(args, (7, -0.5, item))
        self.assertIs(args[2], item)
        self.assertIsNone(state)
        fields.ex = 2**60
        self.assertEqual(fields.__reduce__()[2], (None, {'ex': 2**60}))

    def test_pickle_and_copy(self):
        fields = ValueFields(2**50 - 1, 0.25, 'text')
        fields.ex = 9
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.subTest(proto=proto):
                clone = pickle.loads(pickle.dumps(fields, proto))
                self.assertEqual(clone.kinds(), 'ifoi')
                self.assertEqual((clone.i, clone.f, clone.o, clone.ex),
                                 (2**50 - 1, 0.25, 'text', 9))
        for clone in (copy.copy(fields), copy.deepcopy(fields)):
            self.assertEqual(clone.kinds(), 'ifoi')
            self.assertEqual((clone.i, clone.f, clone.o, clone.ex),
                             (2**50 - 1, 0.25, 'text', 9))
        # An absent optional field reads as None, and is reduced as None.
        clone = copy.copy(ValueFields())
        self.assertEqual(clone.kinds(), 'ooon')
        self.assertIsNone(clone.i)

    def test_struct_sequence_reduce_hidden_fields(self):
        # os.stat_result keeps st_atime and others beyond its visible
        # fields; its __reduce__ reads them from value fields.
        import os
        result = os.stat(__file__)
        cls, (visible, hidden) = result.__reduce__()
        self.assertEqual(hidden['st_atime'], result.st_atime)
        self.assertEqual(pickle.loads(pickle.dumps(result)), result)
        self.assertEqual(copy.copy(result).st_mtime_ns, result.st_mtime_ns)


if __name__ == '__main__':
    unittest.main()
