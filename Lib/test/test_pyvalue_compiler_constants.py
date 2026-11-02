"""Value-native compiler constant indexing, compaction, and owner transfer."""

import ast
import marshal
import math
import opcode
import unittest
import weakref

from test.support import import_helper
from test.support.bytecode_helper import CfgOptimizationTestCase

_testinternalcapi = import_helper.import_module('_testinternalcapi')


class PyValueCompilerConstantTests(CfgOptimizationTestCase):
    def optimize_return(self, consts, index):
        seq = self.seq_from_insts([
            ('LOAD_CONST', index, 0),
            ('RETURN_VALUE', None, 0),
        ])
        return _testinternalcapi.optimize_cfg(seq, consts, 0)

    def test_compaction_preserves_scalar_type_and_signed_zero(self):
        # Small ints and common constants become non-table load opcodes.
        for value in (-257, 1 << 30, 1.0, -0.0, 0.0):
            with self.subTest(value=value, kind=type(value)):
                consts = ['doc', 'unused', value]
                self.optimize_return(consts, 2)
                self.assertEqual(len(consts), 2)
                self.assertIs(type(consts[1]), type(value))
                self.assertEqual(consts[1], value)
                if isinstance(value, float) and value == 0:
                    self.assertEqual(math.copysign(1, consts[1]),
                                     math.copysign(1, value))

    def test_compaction_retains_heap_constant_identity(self):
        for value in (float('nan'), (object(),), 'a non-interned constant ' * 20):
            with self.subTest(kind=type(value)):
                consts = ['doc', 'unused', value]
                self.optimize_return(consts, 2)
                self.assertEqual(len(consts), 2)
                self.assertIs(consts[1], value)

    def test_fold_reuses_existing_canonical_constant_index(self):
        consts = [None, 'unused', -1.5, 1.5]
        seq = self.seq_from_insts([
            ('LOAD_CONST', 3, 0),
            ('UNARY_NEGATIVE', None, 0),
            ('RETURN_VALUE', None, 0),
        ])
        result = _testinternalcapi.optimize_cfg(seq, consts, 0)
        self.assertEqual(consts, [None, -1.5])
        loads = [inst[1] for inst in result.get_instructions()
                 if inst[0] == opcode.opmap['LOAD_CONST']]
        self.assertEqual(loads, [1])

    def test_many_integral_float_keys(self):
        consts = [None] + [float(index) for index in range(2048)]
        self.optimize_return(consts, len(consts) - 1)
        self.assertEqual(consts, [None, 2047.0])
        self.assertIs(type(consts[1]), float)

    def test_compile_distinguishes_equal_values_with_different_types(self):
        source = '(True, 1, 1.0, False, 0, 0.0, -0.0)'
        expected = (True, 1, 1.0, False, 0, 0.0, -0.0)
        result = eval(compile(source, '<value constants>', 'eval'))
        self.assertEqual(tuple(map(type, result)), tuple(map(type, expected)))
        self.assertEqual(result, expected)
        self.assertEqual(math.copysign(1, result[-1]), -1)

    def test_nested_tuple_repeat_complexity(self):
        multiply = next(index for index, (name, _) in enumerate(opcode._nb_ops)
                        if name == 'NB_MULTIPLY')
        for depth, folded in ((1, True), (9, False)):
            with self.subTest(depth=depth):
                value = (1, -1.5, None)
                for _ in range(depth):
                    value = (value, value)
                consts = [None, value]
                seq = self.seq_from_insts([
                    ('LOAD_CONST', 1, 0),
                    ('LOAD_SMALL_INT', 2, 0),
                    ('BINARY_OP', multiply, 0),
                    ('RETURN_VALUE', None, 0),
                ])
                result = _testinternalcapi.optimize_cfg(seq, consts, 0)
                opcodes = [inst[0] for inst in result.get_instructions()]
                self.assertEqual(opcode.opmap['BINARY_OP'] not in opcodes,
                                 folded)
                if folded:
                    self.assertEqual(consts[-1], value * 2)

    def test_consuming_setter_owner_and_publication(self):
        _testinternalcapi.test_list_setitem_take_value()

    def constant_key(self, value, fail=False):
        inline, key, legacy = _testinternalcapi.constant_value_key(value, fail)
        self.assertEqual(key, legacy)
        return inline, key

    def test_canonical_key_scalar_types(self):
        keys = []
        for value, inline in ((True, 0), (1, 1), (1.0, 0),
                              (False, 0), (0, 1), (0.0, 0),
                              (-1, 1), (1 << 40, 0)):
            actual_inline, key = self.constant_key(value)
            self.assertEqual(actual_inline, inline)
            keys.append(key)
        self.assertNotEqual(keys[0], keys[1])
        self.assertNotEqual(keys[1], keys[2])
        self.assertNotEqual(keys[3], keys[4])
        self.assertNotEqual(keys[4], keys[5])

    def test_canonical_key_signed_float_and_complex_zeros(self):
        self.assertNotEqual(self.constant_key(0.0)[1],
                            self.constant_key(-0.0)[1])
        values = [complex(real, imag) for real in (0.0, -0.0)
                  for imag in (0.0, -0.0)]
        keys = [self.constant_key(value)[1] for value in values]
        self.assertEqual(len(set(keys)), 4)

    def test_canonical_key_nan_identity(self):
        nan = float('nan')
        first = self.constant_key(nan)[1]
        same = self.constant_key(nan)[1]
        other = self.constant_key(float('nan'))[1]
        self.assertEqual(first, same)
        self.assertNotEqual(first, other)
        self.assertIs(first[1], nan)

    def test_canonical_key_recursive_containers(self):
        values = [
            (1, -1.5, None, (True, b'bytes', 'str')),
            frozenset((1, 2.5, 'name')),
            (frozenset((1, -0.0)), slice(1, 12, 2)),
            slice(-0.0, 2.5, None),
        ]
        for value in values:
            with self.subTest(kind=type(value)):
                key = self.constant_key(value)[1]
                self.assertEqual(key, self.constant_key(value)[1])
                self.assertIs(key[1], value)

    def test_canonical_key_retains_unsupported_object(self):
        class Marker:
            pass

        marker = Marker()
        reference = weakref.ref(marker)
        key = self.constant_key(marker)[1]
        self.assertEqual(key[0], id(marker))
        del marker
        self.assertIsNotNone(reference())
        del key
        self.assertIsNone(reference())

    def test_canonical_key_allocation_failure_and_recovery(self):
        # A 32-slot key tuple exceeds the small-tuple freelists, ensuring
        # the first producer allocation reaches the one-shot OBJ hook.
        value = tuple(float(index) for index in range(32))
        key = self.constant_key(value, fail=True)[1]
        self.assertIs(key[1], value)
        self.assertEqual(key, self.constant_key(value)[1])

    def compile_shared_constant(self, value):
        tree = ast.parse('def left(): return None\ndef right(): return None\n')
        for function in tree.body:
            function.body[0].value = ast.Constant(value=value)
        ast.fix_missing_locations(tree)
        return compile(tree, '<shared value constants>', 'exec')

    def test_cache_merge_keeps_shared_hashed_tuple_meaning(self):
        name = ''.join(['a_shared_', 'internable_constant'])
        shared = (name, (True, 1, 1.0, -0.0), frozenset((name, 2.5)))
        before_hash = hash(shared)
        lookup = {shared: 'retained'}
        code = self.compile_shared_constant(shared)
        namespace = {}
        exec(code, namespace)
        left = namespace['left']()
        right = namespace['right']()
        self.assertIs(left, right)
        self.assertEqual(hash(shared), before_hash)
        self.assertEqual(lookup[shared], 'retained')
        self.assertEqual(tuple(map(type, left[1])), (bool, int, float, float))
        self.assertEqual(math.copysign(1, left[1][-1]), -1)

    def test_recursive_interning_marshal_is_deterministic(self):
        def payload():
            name = ''.join(['a_shared_', 'internable_constant'])
            return (frozenset((name, 2.5)), (name, -0.0, 1 << 40))

        first = self.compile_shared_constant(payload())
        second = self.compile_shared_constant(payload())
        self.assertEqual(marshal.dumps(first), marshal.dumps(second))

    def test_nested_code_units_share_typed_constants(self):
        source = '''
def outer():
    def inner():
        return (True, 1, 1.0, 0.0, -0.0, (2.5, None))
    return inner, (True, 1, 1.0, 0.0, -0.0, (2.5, None))
'''
        namespace = {}
        exec(compile(source, '<nested value constants>', 'exec'), namespace)
        inner, value = namespace['outer']()
        self.assertIs(inner(), value)
        self.assertEqual(tuple(map(type, value[:3])), (bool, int, float))
        self.assertEqual(math.copysign(1, value[3]), 1)
        self.assertEqual(math.copysign(1, value[4]), -1)

    def test_shared_nan_tuple_retains_identity_and_hash(self):
        nan = float('nan')
        shared = (nan, True, 1, 1.0, 0.0, -0.0)
        before_hash = hash(shared)
        lookup = {shared: 'retained'}
        namespace = {}
        exec(self.compile_shared_constant(shared), namespace)
        left = namespace['left']()
        self.assertIs(left, namespace['right']())
        self.assertIs(left[0], nan)
        self.assertEqual(hash(shared), before_hash)
        self.assertEqual(lookup[shared], 'retained')
        self.assertEqual(tuple(map(type, left[1:4])), (bool, int, float))
        self.assertEqual(math.copysign(1, left[4]), 1)
        self.assertEqual(math.copysign(1, left[5]), -1)

    def test_constant_slice_interning_is_shared_across_code_units(self):
        source = '''
def left(obj):
    return obj['start':'stop':'step']
def right(obj):
    return obj['start':'stop':'step']
'''
        namespace = {}
        exec(compile(source, '<slice value constants>', 'exec'), namespace)

        class SliceEcho:
            def __getitem__(self, key):
                return key

        left = namespace['left'](SliceEcho())
        right = namespace['right'](SliceEcho())
        self.assertIsInstance(left, slice)
        self.assertIs(left, right)
        self.assertEqual((left.start, left.stop, left.step),
                         ('start', 'stop', 'step'))


if __name__ == '__main__':
    unittest.main()
