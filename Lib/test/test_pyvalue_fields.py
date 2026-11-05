"""Value fields of C types: storage, members, GC, release and reduce."""

import ast
import copy
import gc
import marshal
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



def slice_kinds(s):
    return tuple(_testinternalcapi.slot_value_storage(s, name)[0]
                 for name in ('start', 'stop', 'step'))


class Key:
    def __getitem__(self, key):
        return key


def compile_slices(*bounds):
    # Compile x[start:stop:step] for each tuple of bounds, built from AST
    # constants: in source, -0.0 is a unary minus, which is folded only
    # after the compiler has made the constant slices.
    body = [ast.Expr(ast.Subscript(ast.Name('x', ast.Load()),
                                   ast.Slice(*map(ast.Constant, b)),
                                   ast.Load()))
            for b in bounds]
    module = ast.fix_missing_locations(ast.Module(body, []))
    return compile(module, '<slices>', 'exec')


def slice_constants(code):
    return [c for c in code.co_consts if isinstance(c, slice)]


class SliceFieldTests(unittest.TestCase):
    def test_storage(self):
        cases = [
            (slice(1, 2**60, None), ('int', 'object', 'object')),
            (slice(-(2**50), 2**50 - 1, -1), ('int', 'int', 'int')),
            (slice(2**50, -(2**50) - 1), ('object', 'object', 'object')),
            (slice(1.5), ('object', 'float', 'object')),
            (slice('a', None), ('object', 'object', 'object')),
            (slice(Index(2), math.nan, True), ('object', 'object', 'object')),
        ]
        # BUILD_SLICE and BINARY_SLICE pass boxes to PySlice_New(), which
        # stores their values; constant slices come from the compiler.
        a, b, c, d = 3, 4, 5, 0.5
        cases += [
            (Key()[a:b:c], ('int', 'int', 'int')),
            (Key()[d:], ('float', 'object', 'object')),
            (Key()[3:4:5], ('int', 'int', 'int')),
            (Key()[0.5:2**60], ('float', 'object', 'object')),
        ]
        for s, kinds in cases:
            with self.subTest(s=s):
                self.assertEqual(slice_kinds(s), kinds)

    def test_members_by_value(self):
        item = object()
        s = slice(7, 2.5, item)
        self.assertIs(type(s.start), int)
        self.assertEqual(s.start, 7)
        self.assertEqual(s.stop, 2.5)
        self.assertIs(s.step, item)
        self.assertIsNone(slice(3).start)
        self.assertIsNone(slice(3).step)
        for name in ('start', 'stop', 'step'):
            with self.assertRaises(AttributeError):
                setattr(s, name, 1)
            with self.assertRaises(AttributeError):
                delattr(s, name)

    def test_from_values(self):
        s = _testinternalcapi.slice_from_values(1, 2**60, 0.25)
        self.assertEqual(s, slice(1, 2**60, 0.25))
        self.assertEqual(slice_kinds(s), ('int', 'object', 'float'))
        # A missing value is PyValue_NULL, which stores None.
        for s in (_testinternalcapi.slice_from_values(),
                  _testinternalcapi.slice_from_values(None, None, None)):
            self.assertEqual(s, slice(None, None, None))
            self.assertEqual(slice_kinds(s), ('object',) * 3)
        s = _testinternalcapi.slice_from_values(5)
        self.assertEqual((s.start, s.stop, s.step), (5, None, None))

    def test_from_values_owns_its_fields(self):
        item = object()
        big = 2**60 + 3
        before = sys.getrefcount(item), sys.getrefcount(big)
        s = _testinternalcapi.slice_from_values(item, big, item)
        self.assertEqual((sys.getrefcount(item), sys.getrefcount(big)),
                         (before[0] + 2, before[1] + 1))
        del s
        self.assertEqual((sys.getrefcount(item), sys.getrefcount(big)),
                         before)

    def test_traverse_and_cycle(self):
        item = []
        s = slice(1, 2.5, item)
        self.assertEqual(gc.get_referents(s), [item])
        log = []
        holder = [Finalized(log)]
        holder.append(slice(1, holder, 3))
        del holder
        gc.collect()
        self.assertEqual(log, ['finalized'])

    def test_reduce_by_value(self):
        s = slice(1, 2**60, 0.5)
        cls, args = s.__reduce__()
        self.assertIs(cls, slice)
        self.assertEqual(args, (1, 2**60, 0.5))
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.subTest(proto=proto):
                clone = pickle.loads(pickle.dumps(s, proto))
                self.assertEqual(clone, s)
                self.assertEqual(slice_kinds(clone), slice_kinds(s))

    def test_slice_cache_reuses_cleanly(self):
        # A freed slice goes to the one-entry freelist; the next slice must
        # not see its fields.
        item = object()
        before = sys.getrefcount(item)
        slice(item, item, item)
        self.assertEqual(sys.getrefcount(item), before)
        s = slice(1, 2)
        self.assertEqual((s.start, s.stop, s.step), (1, 2, None))
        self.assertEqual(slice_kinds(s), ('int', 'int', 'object'))

    def test_constant_keys_stay_distinct(self):
        # The compiler merges constants by key; the key of a slice is built
        # from its value fields and keeps 0.0 and -0.0, 1 and True, and
        # 2 and 2.0 apart.  The last pair repeats the third and is merged.
        bounds = [(0.0, None), (-0.0, None), (1, 2), (True, 2), (1, 2.0),
                  (1, 2)]
        code = compile_slices(*bounds)
        for code in (code, marshal.loads(marshal.dumps(code))):
            slices = slice_constants(code)
            self.assertEqual(len(slices), 5)
            for s, (start, stop) in zip(slices, bounds):
                with self.subTest(s=s):
                    self.assertIs(type(s.start), type(start))
                    self.assertIs(type(s.stop), type(stop))
                    self.assertEqual((s.start, s.stop), (start, stop))
            self.assertEqual(math.copysign(1, slices[0].start), 1)
            self.assertEqual(math.copysign(1, slices[1].start), -1)
            self.assertEqual(slice_kinds(slices[1]),
                             ('float', 'object', 'object'))
            self.assertEqual(slice_kinds(slices[4]),
                             ('int', 'float', 'object'))

    def test_constant_slice_interns_its_strings(self):
        # The compiler interns the strings of a constant slice by building
        # a new slice from the fields of the old one.
        name = ''.join(['slice_', 'field'])
        interned = sys.intern('slice_field')
        self.assertIsNot(name, interned)
        [s] = slice_constants(compile_slices((name, 1.5, 2**60)))
        self.assertIs(s.start, interned)
        self.assertEqual((s.stop, s.step), (1.5, 2**60))
        self.assertEqual(slice_kinds(s), ('object', 'float', 'object'))

    def test_marshal_round_trip(self):
        # marshal writes each field as an object and reads it back into
        # the same storage.
        cases = [
            (slice(1, 1.5, 2**60), ('int', 'float', 'object')),
            (slice(-0.0), ('object', 'float', 'object')),
            (slice(math.nan), ('object', 'object', 'object')),
            (slice(True), ('object', 'object', 'object')),
        ]
        for s, kinds in cases:
            self.assertEqual(slice_kinds(s), kinds)
            for version in range(5, marshal.version + 1):
                with self.subTest(s=s, version=version):
                    clone = marshal.loads(marshal.dumps(s, version))
                    self.assertEqual(repr(clone), repr(s))
                    self.assertEqual(slice_kinds(clone), kinds)
                    for name in ('start', 'stop', 'step'):
                        self.assertIs(type(getattr(clone, name)),
                                      type(getattr(s, name)))
        clone = marshal.loads(marshal.dumps(slice(-0.0)))
        self.assertEqual(math.copysign(1, clone.stop), -1)
        clone = marshal.loads(marshal.dumps(slice(math.nan)))
        self.assertTrue(math.isnan(clone.stop))


if __name__ == '__main__':
    unittest.main()
