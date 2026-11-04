"""Canonical set keys and ownership at callback/object boundaries."""

import gc
import math
import sys
import unittest
import weakref

from test.support import import_helper, swap_attr

_testinternalcapi = import_helper.import_module('_testinternalcapi')


class PyValueSetTests(unittest.TestCase):
    def test_canonical_key_storage_and_numeric_equality(self):
        values = {-(2**50), 2**50 - 1, -(2**50) - 1, 2**50,
                  0.5, math.inf, None, True}
        self.assertEqual(_testinternalcapi.set_value_storage(values), (2, 2, 4))
        self.assertEqual(_testinternalcapi.set_value_storage(frozenset(values)),
                         (2, 2, 4))
        self.assertEqual({100000, 100000.0}, {100000})
        self.assertEqual({0.0, -0.0}, {0})
        self.assertEqual(hash(frozenset({100000})),
                         hash(frozenset({100000.0})))

    def test_nan_identity_and_subclass_dispatch(self):
        nan = float('nan')
        self.assertIn(nan, {nan})
        self.assertEqual(len({nan, float('nan')}), 2)
        calls = []

        class Number(int):
            def __hash__(self):
                return hash(3)

            def __eq__(self, other):
                calls.append(other)
                return True

        values = {Number(3)}
        self.assertIn(3, values)
        self.assertTrue(calls)
        self.assertEqual(_testinternalcapi.set_value_storage(values), (0, 0, 1))

    def test_algebra_resize_delete_and_dictionary_keys(self):
        values = set(range(1000))
        values.difference_update(range(100, 900))
        values.update({i: None for i in range(300, 700)})
        self.assertEqual(_testinternalcapi.set_value_storage(values),
                         (len(values), 0, 0))
        other = set(range(0, 1000, 3))
        self.assertEqual(values & other, set(sorted(values)).intersection(other))
        self.assertEqual(values.difference(dict.fromkeys(other)), values - other)
        copy = values.copy()
        copy.symmetric_difference_update(dict.fromkeys(other))
        self.assertEqual(copy, values ^ other)
        self.assertTrue((values & other).issubset(values))
        self.assertFalse(values.isdisjoint(values))

    def test_collision_callback_can_replace_table(self):
        values = set()
        mutate = False

        class Key:
            def __hash__(self):
                return 7

            def __eq__(self, other):
                nonlocal mutate
                if mutate:
                    mutate = False
                    values.clear()
                    values.update(range(1000))
                return False

        values.add(Key())
        mutate = True
        query = Key()
        self.assertNotIn(query, values)
        self.assertEqual(values, set(range(1000)))

    def test_strong_owners_clear_publication_and_cycles(self):
        values = set()
        seen = []

        class Item:
            def __del__(self):
                seen.append(len(values))

        item = Item()
        reference = weakref.ref(item)
        values.add(item)
        del item
        values.clear()
        self.assertIsNone(reference())
        self.assertEqual(seen, [0])
        item = Item()
        reference = weakref.ref(item)
        cycle = {item, 100000, 0.5}
        item.cycle = cycle
        del item, cycle
        gc.collect()
        self.assertIsNone(reference())

    def test_object_boxing_failure_consumes_iteration_and_preserves_pop(self):
        _testinternalcapi.set_value_boxing_errors()

    def test_clear_reports_unraisable_and_keeps_pending_exception(self):
        class Key:
            def __hash__(self):
                return 13

            def __del__(self):
                raise RuntimeError("set key finalizer")

        reports = []

        def report(event):
            reports.append((event.exc_type, str(event.exc_value)))

        for large in (False, True):
            for pending in (False, True):
                with self.subTest(large=large, pending=pending):
                    values = {Key(), Key()}
                    if large:
                        values.update(range(100))
                    with swap_attr(sys, 'unraisablehook', report):
                        if pending:
                            _testinternalcapi.set_clear_pending(values)
                        else:
                            try:
                                raise KeyError('handled')
                            except KeyError as handled:
                                values.clear()
                                self.assertIs(sys.exception(), handled)
                    self.assertEqual(len(values), 0)
        self.assertEqual(reports, [(RuntimeError, 'set key finalizer')] * 8)

    def test_frozenset_value_insertion_tracks_new_cycle(self):
        class Item:
            pass

        item = Item()
        reference = weakref.ref(item)
        frozen = _testinternalcapi.set_value_frozenset_tracking(item)
        self.assertTrue(gc.is_tracked(frozen))
        item.frozen = frozen
        del item, frozen
        gc.collect()
        self.assertIsNone(reference())


if __name__ == '__main__':
    unittest.main()
