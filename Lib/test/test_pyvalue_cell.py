"""Canonical cell storage across closure, attribute and C boundaries."""

import gc
import types
import unittest
import weakref

from test.support import import_helper

_testinternalcapi = import_helper.import_module('_testinternalcapi')


class PyValueCellTests(unittest.TestCase):
    def test_empty_and_canonical_contents(self):
        class IntSubclass(int):
            pass

        cell = types.CellType()
        self.assertEqual(_testinternalcapi.cell_value_kind(cell), 'n')
        with self.assertRaises(ValueError):
            _ = cell.cell_contents
        for value, kind in ((-(2**31), 'i'), (2**31 - 1, 'i'),
                            (2**31, 'o'), (1.25, 'f'), (-0.0, 'f'),
                            (float('nan'), 'o'), (True, 'o'), (None, 'o'),
                            (IntSubclass(3), 'o')):
            cell.cell_contents = value
            self.assertEqual(_testinternalcapi.cell_value_kind(cell), kind)
            self.assertIs(cell.cell_contents, value)
        del cell.cell_contents
        del cell.cell_contents
        self.assertEqual(_testinternalcapi.cell_value_kind(cell), 'n')

    def test_closure_load_store_and_finalizer_publication(self):
        value = None

        def get():
            return value

        def set_value(new):
            nonlocal value
            value = new

        observed = []

        class Previous:
            def __del__(self):
                observed.append(get())

        set_value(Previous())
        set_value(100000)
        self.assertEqual(observed, [100000])
        cell = get.__closure__[0]
        self.assertEqual(_testinternalcapi.cell_value_kind(cell), 'i')
        self.assertEqual(get(), 100000)
        set_value(1.25)
        self.assertEqual(_testinternalcapi.cell_value_kind(cell), 'f')
        self.assertEqual(get(), 1.25)

    def test_alias_and_cycle_ownership(self):
        class Item:
            pass

        item = Item()
        reference = weakref.ref(item)
        cell = types.CellType(item)
        cell.cell_contents = cell.cell_contents
        del item
        self.assertIsNotNone(reference())
        cell.cell_contents.cell = cell
        del cell
        gc.collect()
        self.assertIsNone(reference())

    def test_empty_order_numeric_comparison_and_identity_repr(self):
        empty = types.CellType()
        integer = types.CellType(100000)
        floating = types.CellType(100000.0)
        self.assertLess(empty, integer)
        self.assertEqual(integer, floating)
        self.assertIn(hex(id(integer.cell_contents)), repr(integer))
        self.assertIn('int object at', repr(integer))
        self.assertIn('empty', repr(empty))

    def test_object_getter_boxing_failure_preserves_cell(self):
        _testinternalcapi.cell_value_boxing_errors()


if __name__ == '__main__':
    unittest.main()
