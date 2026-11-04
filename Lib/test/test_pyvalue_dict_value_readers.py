"""Real public canonical dictionary read APIs, without tuple parser fixtures."""

import re
import unittest
import weakref

from test.support import import_helper


class DictCanonicalReadTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.capi = import_helper.import_module('_testinternalcapi')

    def read(self, mapping, key, mode=0):
        return self.capi.dict_public_value_read(mapping, key, mode)

    def test_mixed_values_and_tags(self):
        marker = object()
        values = [(2147483647, 'int'), (-2147483648, 'int'),
                  (2.5, 'float'), (None, 'none'), (marker, 'object'),
                  (2**100, 'object'), (False, 'object')]
        for factory in (dict, frozendict):
            mapping = factory({str(i): value for i, (value, _) in enumerate(values)})
            for i, (expected, kind) in enumerate(values):
                for mode in (0, 1):
                    with self.subTest(factory=factory, mode=mode, value=expected):
                        status, value, actual_kind = self.read(mapping, str(i), mode)
                        self.assertEqual((status, value, actual_kind), (1, expected, kind))
                        if expected is marker:
                            self.assertIs(value, marker)
                self.assertEqual(self.read(mapping, str(i), 2), (1, None, 'absent'))
            for mode in (0, 1, 2):
                self.assertEqual(self.read(mapping, 'absent', mode), (0, None, 'absent'))

    def test_mixed_keys_and_numeric_equivalence(self):
        marker = object()
        for factory in (dict, frozendict):
            mapping = factory({1: 'integer', 3.25: 'float', None: 'none', marker: 'object'})
            for key, expected in mapping.items():
                with self.subTest(factory=factory, key=key):
                    self.assertEqual(self.read(mapping, key), (1, expected, 'object'))
            self.assertEqual(self.read(mapping, 1.0), (1, 'integer', 'object'))

    def test_owned_result_survives_clear(self):
        for mode in (0, 1):
            mapping = {'key': []}
            status, value, kind = self.read(mapping, 'key', mode)
            self.assertEqual((status, kind), (1, 'object'))
            mapping.clear()
            value.append(mode)
            self.assertEqual(value, [mode])

    def test_utf8_string_and_conversion_error(self):
        self.assertEqual(self.read({'é漢': 2.5}, 'é漢', 1), (1, 2.5, 'float'))
        with self.assertRaises(TypeError):
            self.read({}, object(), 1)

    def test_single_hash_and_equality(self):
        for mode in (0, 2):
            events = []

            class Stored:
                def __hash__(self):
                    return 37

                def __eq__(self, other):
                    events.append('equal')
                    return True

            class Query:
                def __hash__(self):
                    events.append('hash')
                    return 37

            mapping = {Stored(): 2147483647}
            status, value, kind = self.read(mapping, Query(), mode)
            self.assertEqual(status, 1)
            self.assertEqual(events, ['hash', 'equal'])
            if mode == 0:
                self.assertEqual((value, kind), (2147483647, 'int'))

    def test_hash_and_equality_error_identity(self):
        for operation in ('hash', 'equal'):
            for mode in (0, 2):
                error = LookupError('canonical marker')

                class Key:
                    def __hash__(self):
                        if operation == 'hash':
                            raise error
                        return 37

                    def __eq__(self, other):
                        raise error

                mapping = {} if operation == 'hash' else {Key(): 1}
                with self.subTest(operation=operation, mode=mode):
                    with self.assertRaises(LookupError) as caught:
                        self.read(mapping, Key(), mode)
                    self.assertIs(caught.exception, error)

    def test_unhashable_diagnostics(self):
        for factory in (dict, frozendict):
            mapping = factory({'key': 1})
            message = f"cannot use 'list' as a {factory.__name__} key (unhashable type: 'list')"
            for mode in (0, 2):
                with self.subTest(factory=factory, mode=mode):
                    with self.assertRaisesRegex(TypeError, re.escape(message)):
                        self.read(mapping, [], mode)

    def test_invalid_map(self):
        for mode in (0, 1, 2):
            with self.subTest(mode=mode), self.assertRaises(SystemError):
                self.read(None, 'key', mode)
        with self.assertRaises(SystemError):
            self.capi.dict_public_value_next(None, 0, 3)

    def test_pending_exception(self):
        # An unrelated pending exception changes nothing: each API takes its
        # status from the operation and leaves the exception pending.
        pending = self.capi.dict_public_value_pending
        # Inline int keys and values take the same paths as objects.
        for value in (object(), 7, 2.5):
            for factory in (dict, frozendict):
                mapping = factory({'key': value, 3: value})
                with self.subTest(factory=factory, value=value):
                    self.assertEqual(pending(mapping, 'key', 0), (1, value, None))
                    self.assertEqual(pending(mapping, 'missing', 0), (0, None, None))
                    self.assertEqual(pending(mapping, 3, 0), (1, value, None))
                    self.assertEqual(pending(mapping, 4, 0), (0, None, None))
                    self.assertEqual(pending(mapping, 'key', 1), (1, value, None))
                    self.assertEqual(pending(mapping, 'missing', 1), (0, None, None))
                    self.assertEqual(pending(mapping, 'key', 2), (1, None, None))
                    self.assertEqual(pending(mapping, 'missing', 2), (0, None, None))
                    self.assertEqual(pending(mapping, 3, 2), (1, None, None))
                    self.assertEqual(pending(mapping, 4, 2), (0, None, None))
                    self.assertEqual(pending(mapping, None, 3), (1, 'key', value))
                    self.assertEqual(pending(factory(), None, 3), (0, None, None))
            mapping = {'key': value, 3: value}
            self.assertEqual(pending(mapping, 'key', 4), (0, None, None))
            self.assertEqual(pending(mapping, 3, 4), (0, None, None))
            self.assertEqual(mapping, {})

    def test_next_mixed_holes_optional_outputs_and_end(self):
        source = {17: 'hole', 1: 2147483647, 3.25: 2.5, None: None, 'text': []}
        del source[17]
        expected = list(source.items())
        expected_kinds = [('int', 'int'), ('float', 'float'),
                          ('none', 'none'), ('object', 'object')]
        for factory in (dict, frozendict):
            for mask in range(4):
                mapping = factory(source)
                position = 0
                retained = []
                for (expected_key, expected_value), (expected_key_kind, expected_value_kind) in zip(
                        expected, expected_kinds):
                    status, position, key, value, key_kind, value_kind = (
                        self.capi.dict_public_value_next(mapping, position, mask))
                    self.assertEqual(status, 1)
                    self.assertEqual(key, expected_key if mask & 1 else None)
                    self.assertEqual(value, expected_value if mask & 2 else None)
                    self.assertEqual(key_kind, expected_key_kind if mask & 1 else 'absent')
                    self.assertEqual(value_kind, expected_value_kind if mask & 2 else 'absent')
                    retained.append((key, value))
                self.assertEqual(self.capi.dict_public_value_next(mapping, position, mask),
                                 (0, position, None, None, 'absent', 'absent'))
                if factory is dict:
                    mapping.clear()
                if mask == 3:
                    self.assertEqual(retained, expected)
        self.assertEqual(self.capi.dict_public_value_next({}, 0, 3),
                         (0, 0, None, None, 'absent', 'absent'))

    def test_next_rejects_alias_and_reuses_stress_controls(self):
        with self.assertRaisesRegex(ValueError, 'distinct'):
            self.capi.dict_public_value_next({1: 2}, 0, 4)
        self.capi.dict_owned_entry_contract()
        self.capi.dict_owned_entry_boxing_oom()

    def test_hash_drops_last_input_owners(self):
        for mode in (0, 2):
            events = []
            holder = []

            class Result:
                def __del__(self):
                    events.append('result')

            class Mapping(dict):
                def __del__(self):
                    events.append('mapping')

            class Stored:
                def __hash__(self):
                    return 37

                def __eq__(self, other):
                    return True

            class Query:
                def __hash__(self):
                    holder.clear()
                    return 37

                def __del__(self):
                    events.append('query')

            result = Result()
            reference = weakref.ref(result)
            holder.extend((Mapping({Stored(): result}), Query()))
            del result
            status, result = self.capi.dict_public_value_retirement(holder, mode)
            self.assertEqual(status, 1)
            self.assertEqual(holder, [])
            self.assertEqual(events[:2], ['query', 'mapping'])
            if mode == 0:
                self.assertIs(result, reference())
                self.assertEqual(events, ['query', 'mapping'])
                del result
            self.assertEqual(events, ['query', 'mapping', 'result'])
            self.assertIsNone(reference())

    def test_string_query_allocation_failure(self):
        self.capi.dict_public_value_string_oom()


class DictReadReferenceSemanticsTests(unittest.TestCase):
    def test_python_mixed_reads_and_mutation(self):
        mapping = {1: 2147483647, 3.25: 2.5, 'none': None, 'object': []}
        for key, expected in mapping.items():
            self.assertIn(key, mapping)
            self.assertEqual(mapping[key], expected)
        self.assertNotIn('absent', mapping)
        result = mapping['object']
        mapping.clear()
        result.append(1)
        self.assertEqual(result, [1])

    def test_python_hash_error_identity(self):
        marker = LookupError('hash marker')

        class Key:
            def __hash__(self):
                raise marker

        for operation in (lambda: {}[Key()], lambda: Key() in {}):
            with self.assertRaises(LookupError) as caught:
                operation()
            self.assertIs(caught.exception, marker)


if __name__ == '__main__':
    unittest.main()
