"""Canonical recursion guards and legal exception-preserving cleanup."""

import unittest

from test.support import import_helper

_testinternalcapi = import_helper.import_module('_testinternalcapi')


class PyValueReprTests(unittest.TestCase):
    def test_guard_uses_identity_without_user_equality(self):
        class Equal:
            def __eq__(self, other):
                raise AssertionError('repr guard called equality')

        left, right = Equal(), Equal()
        self.assertEqual(_testinternalcapi.repr_guard_probe((left, right)),
                         (0, 1, 0, 0, 1))
        self.assertEqual(_testinternalcapi.repr_guard_probe((left, left)),
                         (0, 1, 1, 0, 1))

    def test_guard_preserves_canonical_scalar_identity(self):
        nan = float('nan')
        for left, right, same in [
            (1 << 30, 1 << 30, 1),
            (1.5, 1.5, 1),
            (0.0, -0.0, 0),
            (1, True, 0),
            (nan, nan, 1),
            (nan, float('nan'), 0),
        ]:
            with self.subTest(left=left, right=right):
                self.assertEqual(_testinternalcapi.repr_guard_probe((left, right)),
                                 (0, 1, same, 0, 1))

    def test_real_recursive_containers(self):
        sequence = []
        sequence.append(sequence)
        mapping = {}
        mapping['self'] = mapping
        self.assertEqual(repr(sequence), '[[...]]')
        self.assertEqual(repr(mapping), "{'self': {...}}")
        sequence.clear()
        mapping.clear()
        self.assertEqual(repr(sequence), '[]')
        self.assertEqual(repr(mapping), '{}')

    def test_custom_repr_after_nested_guard_cleanup(self):
        class Item:
            def __repr__(self):
                return 'Item(37)'

        item = Item()
        self.assertEqual(_testinternalcapi.repr_guard_probe((item, item)),
                         (0, 1, 1, 0, 1))
        self.assertEqual(repr([item, item]), '[Item(37), Item(37)]')


if __name__ == '__main__':
    unittest.main()
