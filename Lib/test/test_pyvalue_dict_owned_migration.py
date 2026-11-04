"""Owning dictionary API migrations, exercised through real C providers."""

import unittest

from test.support import import_helper


class DictOwnedMigrationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.capi = import_helper.import_module('_testinternalcapi')

    def operation(self, mode, mapping, key, default=None):
        return self.capi.dict_owned_migration((mode, mapping, key, default))

    def test_get_mixed_values_and_missing(self):
        marker = object()
        mapping = {1: 2147483647, 3.25: -2147483648,
                   'float': 2.5, 'object': marker, 'none': None}
        for mode in (0, 2):
            for key, expected in mapping.items():
                with self.subTest(mode=mode, key=key):
                    status, value = self.operation(mode, mapping, key)
                    self.assertEqual(status, 1)
                    self.assertEqual(value, expected)
                    if expected is marker:
                        self.assertIs(value, marker)
            self.assertEqual(self.operation(mode, mapping, 'absent'), (0, None))
        self.assertEqual(self.operation(1, mapping, 'float'), (1, 2.5))
        self.assertEqual(self.operation(1, mapping, 'absent'), (0, None))

    def test_result_survives_source_clear(self):
        for mode in (0, 1, 2, 3):
            with self.subTest(mode=mode):
                mapping = {'key': []}
                status, value = self.operation(mode, mapping, 'key', object())
                self.assertEqual(status, 1)
                mapping.clear()
                value.append('retained')
                self.assertEqual(value, ['retained'])

    def test_known_hash_is_not_recomputed(self):
        class Key:
            calls = 0

            def __hash__(self):
                self.calls += 1
                return 37

        key = Key()
        mapping = {key: 2147483647}
        key.calls = 0
        self.assertEqual(self.operation(2, mapping, key), (1, 2147483647))
        self.assertEqual(key.calls, 1)

    def test_hash_errors_propagate(self):
        error = RuntimeError('hash marker')

        class Key:
            def __hash__(self):
                raise error

        for mode in (0, 2, 3, 4):
            mapping = {}
            with self.subTest(mode=mode), self.assertRaises(RuntimeError) as caught:
                self.operation(mode, mapping, Key(), 1)
            self.assertIs(caught.exception, error)
            self.assertEqual(mapping, {})

    def test_unhashable_keys(self):
        for mode in (0, 2, 3, 4):
            mapping = {}
            with self.subTest(mode=mode), self.assertRaises(TypeError):
                self.operation(mode, mapping, [], 1)
            self.assertEqual(mapping, {})

    def test_equality_errors_propagate(self):
        error = LookupError('equality marker')

        class Key:
            def __hash__(self):
                return 37

            def __eq__(self, other):
                raise error

        stored = Key()
        for mode in (0, 2, 3, 4):
            mapping = {stored: 1}
            with self.subTest(mode=mode), self.assertRaises(LookupError) as caught:
                self.operation(mode, mapping, Key(), 2)
            self.assertIs(caught.exception, error)
            self.assertEqual(len(mapping), 1)

    def test_get_revalidates_after_equality_mutation(self):
        for mode in (0, 2):
            mapping = {}

            class Stored:
                def __hash__(self):
                    return 37

                def __eq__(self, other):
                    mapping.clear()
                    mapping['replacement'] = 2.5
                    return False

            class Query:
                def __hash__(self):
                    return 37

            mapping[Stored()] = 2147483647
            with self.subTest(mode=mode):
                self.assertEqual(self.operation(mode, mapping, Query()), (0, None))
                self.assertEqual(mapping, {'replacement': 2.5})

    def test_setdefault_insert_existing_and_no_output(self):
        for mode in (3, 4):
            for default in (2147483647, 2.5, []):
                with self.subTest(mode=mode, default=default):
                    mapping = {}
                    status, result = self.operation(mode, mapping, 'key', default)
                    self.assertEqual(status, 0)
                    self.assertEqual(mapping, {'key': default})
                    self.assertEqual(result, default if mode == 3 else None)
                    self.assertEqual(self.operation(mode, mapping, 'key', 99),
                                     (1, default if mode == 3 else None))

    def test_owned_entry_status_and_cached_hash(self):
        mapping = {1: 2147483647, 3.25: 2.5, 'object': []}
        position = 0
        retained = []
        for expected in mapping.items():
            status, position, key, value, cached_hash = self.capi.dict_owned_entry(
                (mapping, position))
            self.assertEqual(status, 1)
            self.assertEqual((key, value), expected)
            self.assertEqual(cached_hash, hash(key))
            retained.append((key, value))
        self.assertEqual(self.capi.dict_owned_entry((mapping, position)),
                         (0, position, None, None, -1))
        mapping.clear()
        self.assertEqual(retained, [(1, 2147483647), (3.25, 2.5), ('object', [])])

    def test_owned_entry_failure_contracts(self):
        self.capi.dict_owned_entry_contract()
        self.capi.dict_owned_entry_boxing_oom()


class DictPythonSemanticsTests(unittest.TestCase):
    def test_python_setdefault(self):
        for default in (2147483647, -2147483648, 2.5, []):
            with self.subTest(default=default):
                mapping = {}
                result = mapping.setdefault('key', default)
                self.assertEqual(result, default)
                self.assertEqual(mapping.setdefault('key', 99), default)
                mapping.clear()
                self.assertEqual(result, default)

    def test_keyword_key_classification(self):
        def keywords(**kwargs):
            return kwargs

        # Delete a non-string key so a general table retains only string keys.
        mapping = {1: None, 'integer': 2147483647, 'float': 2.5}
        del mapping[1]
        self.assertEqual(keywords(**mapping), mapping)
        with self.assertRaisesRegex(TypeError, 'keywords must be strings'):
            keywords(**{1: 2147483647})


if __name__ == '__main__':
    unittest.main()
