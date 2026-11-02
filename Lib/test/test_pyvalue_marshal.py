"""Marshal owning container reads, recursive sharing and reference insertion."""

import marshal
import math
import struct
import types
import unittest


def tuple_stream(*items):
    return b'(' + struct.pack('<i', len(items)) + b''.join(items)


class MarshalReferenceTests(unittest.TestCase):
    def test_mixed_roundtrips(self):
        value = ([-2147483648, 2147483647, 2**100, 2.5, -0.0, None, False],
                 {1: 'integer', 3.25: ['float'], 'none': None})
        for version in range(2, marshal.version + 1):
            with self.subTest(version=version):
                result = marshal.loads(marshal.dumps(value, version))
                self.assertEqual(result, value)
                self.assertEqual(struct.pack('d', result[0][4]), struct.pack('d', -0.0))

    def test_cycles_and_shared_heap_values(self):
        for version in range(3, marshal.version + 1):
            shared = []
            mapping = {'left': shared, 'right': shared}
            shared.append(mapping)
            result = marshal.loads(marshal.dumps(mapping, version))
            self.assertIs(result['left'], result['right'])
            self.assertIs(result['left'][0], result)
            sequence = []
            sequence.append(sequence)
            result = marshal.loads(marshal.dumps(sequence, version))
            self.assertIs(result[0], result)

    def test_shared_tuple(self):
        shared = ([], 'tuple owner')
        result = marshal.loads(marshal.dumps((shared, shared)))
        self.assertIs(result[0], result[1])

    def test_explicit_scalar_reference_entries(self):
        for encoded, expected in (
                (bytes([ord('i') | 0x80]) + struct.pack('<i', 2147483647), 2147483647),
                (bytes([ord('g') | 0x80]) + struct.pack('<d', 2.5), 2.5)):
            result = marshal.loads(tuple_stream(encoded, b'r' + struct.pack('<i', 0)))
            self.assertEqual(result, (expected, expected))
            self.assertIs(result[0], result[1])

    def test_flagged_slice_reference_insertion(self):
        encoded = (bytes([ord(':') | 0x80]) +
                   b'i' + struct.pack('<i', 1) +
                   b'i' + struct.pack('<i', 9) +
                   b'i' + struct.pack('<i', 2))
        result = marshal.loads(tuple_stream(encoded, b'r' + struct.pack('<i', 0)))
        self.assertEqual(result, (slice(1, 9, 2), slice(1, 9, 2)))
        self.assertIs(result[0], result[1])

    def test_invalid_and_reserved_references(self):
        for encoded in (
                b'r' + struct.pack('<i', -1),
                b'r' + struct.pack('<i', 0),
                bytes([ord(':') | 0x80]) + b'r' +
                struct.pack('<i', 0) + b'NN'):
            with self.subTest(encoded=encoded):
                with self.assertRaisesRegex(ValueError, 'invalid reference'):
                    marshal.loads(encoded)

    def test_sorted_sets_with_distinct_nans(self):
        value = {float('nan'), float('nan'), 2147483647, 'text'}
        for version in range(3, marshal.version + 1):
            first = marshal.dumps(value, version)
            self.assertEqual(first, marshal.dumps(value, version))
            result = marshal.loads(first)
            self.assertEqual(len(result), 4)
            self.assertEqual(sum(isinstance(x, float) and math.isnan(x) for x in result), 2)
            self.assertIn('text', result)
            self.assertIn(2147483647, result)

    def test_code_roundtrip(self):
        def function(value):
            return value + 2147483647
        result = marshal.loads(marshal.dumps(function.__code__))
        rebuilt = types.FunctionType(result, {})
        self.assertEqual(rebuilt(1), 2147483648)

    def test_unsupported_object(self):
        with self.assertRaises(ValueError):
            marshal.dumps(object())


class MarshalOwnerMutationTests(unittest.TestCase):
    # These exercise the restored providers. Do not run them as host controls:
    # the old borrowed loops can dereference released entries after callbacks.
    def test_dictionary_value_survives_key_callback(self):
        events = []
        mapping = {}

        class Key:
            def __hash__(self):
                return 37

            def __buffer__(self, flags):
                events.append('key')
                mapping.clear()
                return memoryview(b'key')

        class Value:
            def __buffer__(self, flags):
                events.append('value')
                return memoryview(b'retained value')

            def __del__(self):
                events.append('retired')

        mapping[Key()] = Value()
        encoded = marshal.dumps(mapping)
        self.assertEqual(marshal.loads(encoded), {b'key': b'retained value'})
        self.assertEqual(events, ['key', 'value', 'retired'])
        self.assertEqual(mapping, {})

    def test_list_shrink_stops_at_checked_read(self):
        events = []
        values = []

        class Buffer:
            def __buffer__(self, flags):
                events.append('buffer')
                values.clear()
                return memoryview(b'first')

        values.extend((Buffer(), 'removed tail'))
        with self.assertRaises(IndexError):
            marshal.dumps(values)
        self.assertEqual(events, ['buffer'])
        self.assertEqual(values, [])

    def test_primary_callback_error_survives_snapshot_cleanup(self):
        events = []
        mapping = {}
        marker = LookupError('marshal callback marker')

        class Key:
            def __hash__(self):
                return 37

            def __buffer__(self, flags):
                mapping.clear()
                raise marker

        class Value:
            def __del__(self):
                events.append('retired')

        mapping[Key()] = Value()
        with self.assertRaises(LookupError) as caught:
            marshal.dumps(mapping)
        self.assertIs(caught.exception, marker)
        self.assertEqual(events, ['retired'])


class MarshalFrozenDictTests(unittest.TestCase):
    def test_frozendict_roundtrip_and_version(self):
        value = frozendict({1: 2147483647, 'float': 2.5, 'none': None})
        result = marshal.loads(marshal.dumps(value, 6))
        self.assertIs(type(result), frozendict)
        self.assertEqual(result, value)
        with self.assertRaisesRegex(ValueError, 'requires version 6'):
            marshal.dumps(value, 5)


if __name__ == '__main__':
    unittest.main()
