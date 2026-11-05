"""Value fields of C types: storage, members, GC, release and reduce."""

import ast
import copy
import gc
import io
import marshal
import math
import pickle
import sys
import unittest
import weakref

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


class PickleHomeTests(unittest.TestCase):
    """The C unpickler's stack (Pdata) and memo hold values."""

    PROTOCOLS = range(pickle.HIGHEST_PROTOCOL + 1)

    def roundtrip(self, obj, proto):
        data = pickle.dumps(obj, proto)
        return pickle.Unpickler(io.BytesIO(data)).load()

    def test_int_boundaries(self):
        ints = [0, 1, -1, 255, 256, 65535, 65536, 2**31 - 1, -2**31,
                2**31, -2**31 - 1,
                2**50 - 1, 2**50, 2**50 + 1, -2**50, -2**50 - 1,
                2**62, 2**63 - 1, 2**63, -2**63, -2**63 - 1,
                2**64 - 1, 2**64, -2**64, 2**100, -2**100]
        for proto in self.PROTOCOLS:
            with self.subTest(proto=proto):
                result = self.roundtrip(ints, proto)
                self.assertEqual(result, ints)
                self.assertEqual([type(x) for x in result], [int] * len(ints))
                result = self.roundtrip(tuple(ints), proto)
                self.assertEqual(result, tuple(ints))

    def test_floats(self):
        for proto in self.PROTOCOLS:
            with self.subTest(proto=proto):
                result = self.roundtrip(
                    [0.0, -0.0, 1.5, -2.25, 1e300, 5e-324, math.inf,
                     -math.inf, math.nan], proto)
                self.assertEqual(result[:-1],
                                 [0.0, -0.0, 1.5, -2.25, 1e300, 5e-324,
                                  math.inf, -math.inf])
                self.assertEqual(math.copysign(1.0, result[1]), -1.0)
                self.assertEqual(math.copysign(1.0, result[0]), 1.0)
                self.assertTrue(math.isnan(result[-1]))

    def test_bool_and_none(self):
        for proto in self.PROTOCOLS:
            with self.subTest(proto=proto):
                result = self.roundtrip([True, False, None, 1, 0], proto)
                self.assertIs(result[0], True)
                self.assertIs(result[1], False)
                self.assertIsNone(result[2])
                self.assertIsNot(result[3], True)

    def test_memo_reuse(self):
        big = 2**70
        shared = [1.5, big, 'text']
        obj = [shared, shared, (shared, shared)]
        for proto in self.PROTOCOLS:
            with self.subTest(proto=proto):
                result = self.roundtrip(obj, proto)
                self.assertEqual(result, obj)
                self.assertIs(result[0], result[1])
                self.assertIs(result[2][0], result[0])

    def test_memo_proxy_copy_and_reduce(self):
        obj = [1.5, 2**70, 'a', ('b', 2**40)]
        data = pickle.dumps(obj, 2)
        unpickler = pickle.Unpickler(io.BytesIO(data))
        self.assertEqual(unpickler.load(), obj)
        memo = unpickler.memo.copy()
        self.assertIsInstance(memo, dict)
        self.assertTrue(memo)
        self.assertEqual(list(memo), sorted(memo))
        reduced = unpickler.memo.__reduce__()
        self.assertEqual(reduced[1], (memo,))
        self.assertIn(obj, memo.values())
        # Assigning the copy to another unpickler keeps the values.
        other = pickle.Unpickler(io.BytesIO(b''))
        other.memo = unpickler.memo
        self.assertEqual(other.memo.copy(), memo)
        other.memo = memo
        self.assertEqual(other.memo.copy(), memo)
        unpickler.memo.clear()
        self.assertEqual(unpickler.memo.copy(), {})
        self.assertEqual(other.memo.copy(), memo)

    def test_memo_assign_dict(self):
        memo = {0: 1, 1: 2.5, 2: -0.0, 3: 2**50, 4: -2**63, 5: 2**64,
                6: 'x', 7: None}
        unpickler = pickle.Unpickler(io.BytesIO(b''))
        unpickler.memo = memo
        copy = unpickler.memo.copy()
        self.assertEqual(copy, memo)
        self.assertEqual(math.copysign(1.0, copy[2]), -1.0)
        unpickler.memo = {}
        self.assertEqual(unpickler.memo.copy(), {})
        with self.assertRaises(TypeError):
            unpickler.memo = {'a': 1}
        with self.assertRaises(ValueError):
            unpickler.memo = {-1: 1}
        with self.assertRaises(TypeError):
            unpickler.memo = 1

    def test_memo_get_after_assign(self):
        # BINGET reads the assigned values: 1.5 and 2**70, then a tuple.
        memo = {0: 1.5, 1: 2**70, 2: 7}
        data = b'\x80\x02(h\x00h\x01h\x02t.'
        unpickler = pickle.Unpickler(io.BytesIO(data))
        unpickler.memo = memo
        self.assertEqual(unpickler.load(), (1.5, 2**70, 7))

    def test_sparse_memo(self):
        # BINPUT at index 200 is sparse: it goes to the backup dict, and the
        # BINGET migrates it to the array.
        data = b'\x80\x02G@\x04\x00\x00\x00\x00\x00\x00q\xc8h\xc8h\xc8\x86.'
        unpickler = pickle.Unpickler(io.BytesIO(data))
        self.assertEqual(unpickler.load(), (2.5, 2.5))

    def test_stack_growth(self):
        for proto in self.PROTOCOLS:
            with self.subTest(proto=proto):
                values = list(range(-500, 500)) + [i / 4 for i in range(500)]
                for obj in (tuple(values), values,
                            dict(zip(range(2000), values + values)),
                            frozenset(values), set(values)):
                    self.assertEqual(self.roundtrip(obj, proto), obj)

    def test_deep_nesting(self):
        obj = 0
        for i in range(200):
            obj = [obj, 1.5, i]
        for proto in self.PROTOCOLS:
            self.assertEqual(self.roundtrip(obj, proto), obj)

    def test_build_and_reduce_with_numbers(self):
        class Point:
            def __init__(self, x, y):
                self.x = x
                self.y = y
        Point.__module__ = __name__
        globals()['_PickleHomePoint'] = Point
        Point.__qualname__ = '_PickleHomePoint'
        try:
            for proto in self.PROTOCOLS:
                with self.subTest(proto=proto):
                    result = self.roundtrip(
                        [Point(1, 2.5), Point(2**70, -0.0)], proto)
                    self.assertEqual((result[0].x, result[0].y), (1, 2.5))
                    self.assertEqual(result[1].x, 2**70)
                    self.assertEqual(math.copysign(1.0, result[1].y), -1.0)
        finally:
            del globals()['_PickleHomePoint']

    def test_appends_to_non_list(self):
        # APPENDS/SETITEMS/ADDITEMS on objects that only have the Python
        # level methods consume stack items through a call.
        class Appender:
            def __new__(cls):
                self = super().__new__(cls)
                self.items = []
                return self
            def append(self, item):
                self.items.append(item)

        class Adder:
            def __new__(cls):
                self = super().__new__(cls)
                self.items = []
                return self
            def add(self, item):
                self.items.append(item)

        class Mapping:
            def __new__(cls):
                self = super().__new__(cls)
                self.items = {}
                return self
            def __setitem__(self, key, value):
                self.items[key] = value

        values = [1, 2.5, 2**70, 'x']
        data = (b'\x80\x02c' + __name__.encode() + b'\n_PHAppender\n)\x81(' +
                b'K\x01G\x40\x04\x00\x00\x00\x00\x00\x00' +
                b'\x8a\x09\x00\x00\x00\x00\x00\x00\x00\x00\x40' +
                b'X\x01\x00\x00\x00xe.')
        globals().update(_PHAppender=Appender)
        try:
            appender = pickle.Unpickler(io.BytesIO(data)).load()
        finally:
            del globals()['_PHAppender']
        self.assertEqual(appender.items, values)

        data = (b'\x80\x02c' + __name__.encode() + b'\n_PHAdder\n)\x81(' +
                b'K\x01G\x40\x04\x00\x00\x00\x00\x00\x00' +
                b'\x8a\x09\x00\x00\x00\x00\x00\x00\x00\x00\x40' +
                b'X\x01\x00\x00\x00x\x90.')
        globals().update(_PHAdder=Adder)
        try:
            adder = pickle.Unpickler(io.BytesIO(data)).load()
        finally:
            del globals()['_PHAdder']
        self.assertEqual(adder.items, values)

        data = (b'\x80\x02c' + __name__.encode() + b'\n_PHMapping\n)\x81(' +
                b'K\x01G\x40\x04\x00\x00\x00\x00\x00\x00' +
                b'\x8a\x09\x00\x00\x00\x00\x00\x00\x00\x00\x40' +
                b'X\x01\x00\x00\x00xu.')
        globals().update(_PHMapping=Mapping)
        try:
            mapping = pickle.Unpickler(io.BytesIO(data)).load()
        finally:
            del globals()['_PHMapping']
        self.assertEqual(mapping.items, {1: 2.5, 2**70: 'x'})

    def test_stack_underflow_and_bad_input(self):
        for data in (b'.', b'0.', b'1.', b'\x85.', b'a.', b's.', b'b.',
                     b'N\x94b.', b'K\x01\x94h\x05.', b'(0.'):
            with self.subTest(data=data):
                with self.assertRaises(pickle.UnpicklingError):
                    pickle.Unpickler(io.BytesIO(data)).load()

    def test_gc_cycle_through_stack_and_memo(self):
        class Node:
            pass

        unpickler = pickle.Unpickler(io.BytesIO(b''))
        node = Node()
        node.unpickler = unpickler
        unpickler.memo = {0: node, 1: 1.5}
        ref = weakref.ref(node)
        del node, unpickler
        gc.collect()
        self.assertIsNone(ref())

    def test_gc_with_live_stack(self):
        # persistent_load runs while earlier items sit on the stack.
        seen = []

        class Pickler(pickle.Pickler):
            def persistent_id(self, obj):
                return 'pid' if obj == 'marker' else None

        class Node:
            pass

        refs = []

        class Unpickler(pickle.Unpickler):
            def persistent_load(self, pid):
                node = Node()
                node.cycle = [node, self]
                refs.append(weakref.ref(node))
                gc.collect()
                seen.append(refs[0]() is not None)
                return node

        buf = io.BytesIO()
        Pickler(buf, 2).dump([1, 2.5, 2**70, 'marker', 'marker', (3,)])
        buf.seek(0)
        result = Unpickler(buf).load()
        self.assertEqual(seen, [True, True])
        self.assertEqual(result[:3], [1, 2.5, 2**70])
        self.assertIsNot(result[3], result[4])
        self.assertEqual(result[5], (3,))
        del result
        gc.collect()
        self.assertTrue(all(ref() is None for ref in refs))

    def test_memo_refcount(self):
        class Thing:
            pass

        thing = Thing()
        base = sys.getrefcount(thing)
        unpickler = pickle.Unpickler(io.BytesIO(b''))
        unpickler.memo = {0: thing, 1: thing}
        self.assertEqual(sys.getrefcount(thing), base + 2)
        unpickler.memo = {0: thing}
        self.assertEqual(sys.getrefcount(thing), base + 1)
        copy = unpickler.memo.copy()
        self.assertEqual(sys.getrefcount(thing), base + 2)
        del copy
        del unpickler
        gc.collect()
        self.assertEqual(sys.getrefcount(thing), base)

    def test_memo_refcount_after_load(self):
        class Thing:
            pass

        thing = Thing()
        base = sys.getrefcount(thing)
        unpickler = pickle.Unpickler(io.BytesIO(b'\x80\x02h\x00.'))
        unpickler.memo = {0: thing}
        self.assertIs(unpickler.load(), thing)
        self.assertEqual(sys.getrefcount(thing), base + 1)
        del unpickler
        self.assertEqual(sys.getrefcount(thing), base)

    def test_sizeof(self):
        unpickler = pickle.Unpickler(io.BytesIO(b''))
        self.assertGreater(unpickler.__sizeof__(), 32 * 8)


if __name__ == '__main__':
    unittest.main()


class Node:
    """A heap number that can hold a reference to an iterator."""

    def __float__(self):
        return 0.0

    def __add__(self, other):
        return self


class ItertoolsHomeTests(unittest.TestCase):
    # Values that are inline (ints, floats, -0.0), a NaN (boxed), a big int
    # and a heap object.
    SAMPLES = [0, -7, 2**50 - 1, 2**50, 1.5, -0.0, math.inf, 'text', 2**70]

    def assertSame(self, a, b):
        # Equal values of the same type; -0.0 and NaN included.
        self.assertIs(type(a), type(b))
        if isinstance(a, float):
            self.assertEqual(math.copysign(1.0, a), math.copysign(1.0, b))
            self.assertTrue(a == b or (a != a and b != b))
        else:
            self.assertEqual(a, b)

    def test_count_repr(self):
        from itertools import count
        for it, expected in [
            (count(5), 'count(5)'),
            (count(5, 2), 'count(5, 2)'),
            (count(1.5, 0.5), 'count(1.5, 0.5)'),
            (count(2**62), 'count(4611686018427387904)'),
            (count(True), 'count(1)'),
            (count(True, 2), 'count(True, 2)'),
            (count(5, True), 'count(5)'),
            (count(5, 1.0), 'count(5, 1.0)'),
            (count(-3, -2), 'count(-3, -2)'),
            (count(0), 'count(0)'),
        ]:
            with self.subTest(expected=expected):
                self.assertEqual(repr(it), expected)

    def test_count_steps(self):
        from itertools import count
        c = count(2**63 - 3)
        self.assertEqual([next(c) for _ in range(6)],
                         [2**63 - 3 + i for i in range(6)])
        self.assertEqual(repr(c), 'count(9223372036854775811)')
        c = count(2**63 - 3, 2)
        self.assertEqual([next(c) for _ in range(5)],
                         [2**63 - 3 + 2 * i for i in range(5)])
        self.assertEqual(repr(c), 'count(9223372036854775815, 2)')
        # across the inline limit
        c = count(2**50 - 2)
        self.assertEqual([next(c) for _ in range(5)],
                         [2**50 - 2 + i for i in range(5)])
        self.assertEqual(repr(c), 'count(1125899906842627)')
        c = count(5, -7)
        self.assertEqual([next(c) for _ in range(4)], [5, -2, -9, -16])
        self.assertEqual(repr(c), 'count(-23, -7)')
        c = count(0.1, 0.1)
        self.assertEqual([next(c) for _ in range(4)],
                         [0.1, 0.2, 0.30000000000000004, 0.4])
        c = count(-0.0, 0.0)
        self.assertSame(next(c), -0.0)
        self.assertSame(next(c), 0.0)
        c = count(float('nan'), 1)
        self.assertTrue(math.isnan(next(c)))
        self.assertTrue(math.isnan(next(c)))

    def test_count_start_type(self):
        from itertools import count

        class I(int):
            pass

        # An integer start with step 1 is normalized to an exact int.
        self.assertIs(type(next(count(True))), int)
        self.assertIs(type(next(count(I(3)))), int)
        # Otherwise the start is returned as given.
        self.assertIs(type(next(count(I(3), 2))), I)
        self.assertIs(type(next(count(True, 2))), bool)

    def test_repeat(self):
        from itertools import repeat
        for value in self.SAMPLES:
            with self.subTest(value=value):
                it = repeat(value, 3)
                self.assertEqual(repr(it), f'repeat({value!r}, 3)')
                items = list(it)
                self.assertEqual(len(items), 3)
                for item in items:
                    self.assertSame(item, value)
        nan = float('nan')
        self.assertTrue(all(x != x for x in repeat(nan, 2)))
        self.assertEqual(repr(repeat(1.5)), 'repeat(1.5)')

    def test_accumulate(self):
        from itertools import accumulate
        for value in self.SAMPLES[:6]:
            with self.subTest(value=value):
                items = list(accumulate([value, value], lambda a, b: a))
                self.assertSame(items[0], value)
                self.assertSame(items[1], value)
                items = list(accumulate([1, 2], lambda a, b: b, initial=value))
                self.assertSame(items[0], value)
                self.assertEqual(items[1:], [1, 2])
        self.assertEqual(list(accumulate([2**50 - 1, 1, 1])),
                         [2**50 - 1, 2**50, 2**50 + 1])
        self.assertEqual(list(accumulate([1.5, 2, 0.5])), [1.5, 3.5, 4.0])
        self.assertEqual(list(accumulate(['a', 'b'], initial='z')),
                         ['z', 'za', 'zab'])
        self.assertEqual(list(accumulate([1, 2], initial=None)), [1, 3])

    def test_pairwise(self):
        from itertools import pairwise
        items = [0, -0.0, 1.5, 2**50, 2**70, 'x', float('inf')]
        pairs = list(pairwise(items))
        self.assertEqual(len(pairs), len(items) - 1)
        for (a, b), x, y in zip(pairs, items, items[1:]):
            self.assertSame(a, x)
            self.assertSame(b, y)
        it = pairwise(iter([1, 2, 3]))
        first = next(it)
        second = next(it)
        self.assertEqual((first, second), ((1, 2), (2, 3)))
        with self.assertRaises(StopIteration):
            next(it)

    def test_groupby(self):
        from itertools import groupby
        items = [1, 1, 1.5, 1.5, -0.0, 2**60, 2**60, 'a', 'a']
        groups = [(k, list(g)) for k, g in groupby(items)]
        self.assertEqual([k for k, _ in groups], [1, 1.5, -0.0, 2**60, 'a'])
        self.assertEqual([len(g) for _, g in groups], [2, 2, 1, 2, 2])
        for (k, g) in groups:
            self.assertSame(k, g[0])
        nan = float('nan')
        got = [(k, list(g)) for k, g in groupby([nan, nan])]
        self.assertEqual(len(got), 1)
        self.assertEqual(len(got[0][1]), 2)
        keyed = [(k, list(g)) for k, g in groupby(range(6), lambda x: x // 2)]
        self.assertEqual(keyed, [(0, [0, 1]), (1, [2, 3]), (2, [4, 5])])
        self.assertEqual(list(next(groupby([1.5, 2.5]))[1]), [1.5])

    def test_tee(self):
        from itertools import tee
        for value in self.SAMPLES:
            with self.subTest(value=value):
                a, b = tee(iter([value] * 3))
                self.assertSame(next(a), value)
                self.assertSame(next(b), value)
                self.assertEqual(len(list(a)), 2)
                self.assertEqual(len(list(b)), 2)
        # more items than one data link (57 cells) holds
        a, b = tee(iter(range(200)))
        self.assertEqual(list(a), list(range(200)))
        self.assertEqual(list(b), list(range(200)))
        a, b = tee(iter([2**50, 1.5, -0.0] * 40))
        self.assertEqual(len(list(a)), 120)
        self.assertEqual(len(list(b)), 120)

    def test_tee_dataobject_values(self):
        from itertools import _tee_dataobject
        marker = Node()
        values = [1, 1.5, None, 'text', marker]
        data = _tee_dataobject(iter(()), values, None)
        refs = gc.get_referents(data)
        # Heap objects are referents; inline numbers are not.
        self.assertTrue(any(ref is marker for ref in refs))
        self.assertTrue(any(ref is None for ref in refs))
        self.assertTrue(any(ref == 'text' for ref in refs))
        held = sys.getrefcount(marker)
        del data
        self.assertEqual(sys.getrefcount(marker), held - 1)

    def test_zip_longest(self):
        from itertools import zip_longest
        for value in self.SAMPLES:
            with self.subTest(value=value):
                result = list(zip_longest([1, 2], [3], fillvalue=value))
                self.assertEqual(result[0], (1, 3))
                self.assertEqual(len(result), 2)
                self.assertEqual(result[1][0], 2)
                self.assertSame(result[1][1], value)
        nan = float('nan')
        result = list(zip_longest([1], [], fillvalue=nan))
        self.assertTrue(result[0][1] != result[0][1])
        # Result tuple reuse when not retained.
        self.assertEqual([tuple(t) for t in zip_longest('ab', 'c', fillvalue=0)],
                         [('a', 'c'), ('b', 0)])

    def test_heap_object_release(self):
        from itertools import (accumulate, count, groupby, pairwise, repeat,
                               tee, zip_longest)
        marker = Node()
        base = sys.getrefcount(marker)
        builders = {
            'count': lambda: count(marker, 0),
            'repeat': lambda: repeat(marker, 2),
            'accumulate': lambda: accumulate([1], initial=marker),
            'zip_longest': lambda: zip_longest([1], [], fillvalue=marker),
            'pairwise': lambda: pairwise([marker, marker]),
            'groupby': lambda: groupby([marker, marker]),
            'tee': lambda: tee(iter([marker, marker])),
        }
        for name, build in builders.items():
            with self.subTest(name=name):
                it = build()
                holder = it if isinstance(it, tuple) else (it,)
                for x in holder:
                    next(x)
                self.assertGreater(sys.getrefcount(marker), base)
                del it, holder, x
                gc.collect()
                self.assertEqual(sys.getrefcount(marker), base)

    def test_heap_object_returned(self):
        from itertools import count, repeat
        marker = Node()
        base = sys.getrefcount(marker)
        it = repeat(marker, 3)
        items = list(it)
        self.assertEqual(sys.getrefcount(marker), base + 4)
        del items
        self.assertEqual(sys.getrefcount(marker), base + 1)
        del it
        self.assertEqual(sys.getrefcount(marker), base)
        it = count(marker, 0)
        self.assertIs(next(it), marker)
        del it
        self.assertEqual(sys.getrefcount(marker), base)

    def test_gc_cycles(self):
        import weakref
        from itertools import (accumulate, count, groupby, pairwise, repeat,
                               tee, zip_longest)
        builders = {
            'count': lambda n: count(n, 0),
            'repeat': lambda n: repeat(n),
            'accumulate': lambda n: accumulate([1], initial=n),
            'zip_longest': lambda n: zip_longest([1], [], fillvalue=n),
            'pairwise': lambda n: pairwise([n, n]),
            'groupby': lambda n: groupby([n, n]),
            'tee': lambda n: tee(iter([n, n]))[0],
        }
        gc.collect()
        for name, build in builders.items():
            with self.subTest(name=name):
                node = Node()
                ref = weakref.ref(node)
                node.it = it = build(node)
                next(it)
                if name != 'tee':
                    self.assertTrue(
                        any(r is node for r in gc.get_referents(it)))
                del node, it
                self.assertIsNotNone(ref())
                gc.collect()
                self.assertIsNone(ref())

    def test_gc_referents_homes(self):
        from itertools import accumulate, count, repeat, zip_longest
        node = Node()
        for it in (count(node, 0), repeat(node), zip_longest([1], [], fillvalue=node),
                   accumulate([1], initial=node)):
            with self.subTest(type=type(it).__name__):
                if type(it).__name__ in ('accumulate', 'zip_longest'):
                    next(it)
                self.assertTrue(any(r is node for r in gc.get_referents(it)))
        for it in (count(1.5, 2), repeat(2**40), repeat(0.5)):
            with self.subTest(inline=type(it).__name__):
                self.assertFalse(any(isinstance(r, (int, float))
                                     for r in gc.get_referents(it)
                                     if not isinstance(r, type)))


class Tracked:
    """A heap object whose lifetime the tests observe."""


def _values():
    return [0, 7, -(2**50), 2**50 - 1, 2**50, 2**70, 1.5, -0.0,
            float('inf'), float('nan'), None, True, 'text', (1, 2)]


def _same(a, b):
    if isinstance(a, float) and a != a:
        return isinstance(b, float) and b != b
    return type(a) is type(b) and a == b and (
        not isinstance(a, float) or math.copysign(1, a) == math.copysign(1, b))


class ModuleHomeTests(unittest.TestCase):
    """deque, functools, operator, queue, asyncio and ElementTree homes."""

    def check_values(self, got, expected):
        self.assertEqual(len(got), len(expected))
        for g, e in zip(got, expected):
            self.assertTrue(_same(g, e), (g, e))

    def test_deque_values_across_blocks(self):
        from collections import deque
        vals = _values() * 12      # more than one 64-slot block
        d = deque(vals)
        self.check_values(list(d), vals)
        self.check_values(list(reversed(d)), vals[::-1])
        d.rotate(37)
        d.rotate(-37)
        d.reverse()
        d.reverse()
        self.check_values(list(d), vals)
        self.assertTrue(_same(d[5], vals[5]))
        self.assertTrue(_same(d[-1], vals[-1]))
        for end in (d.pop, d.popleft):
            self.assertTrue(_same(end(), vals.pop() if end == d.pop else vals.pop(0)))
        d.appendleft(2**50)
        d.append(-0.0)
        d[3] = 1.5
        self.assertTrue(_same(d[3], 1.5))
        self.assertEqual(d.count(2**50), vals.count(2**50) + 1)
        self.assertIn(2**70, d)
        self.assertEqual(d.index(1.5), 0 + d.index(1.5))
        c = d.copy()
        self.assertEqual(c, d)
        self.check_values(list(c * 2), list(d) * 2)
        d2 = pickle.loads(pickle.dumps(d, 2))
        self.assertEqual(len(d2), len(d))

    def test_deque_maxlen_and_extend(self):
        from collections import deque
        d = deque(maxlen=3)
        d.extend([1, 2.5, 2**60, 4])
        self.check_values(list(d), [2.5, 2**60, 4])
        d.extendleft([7, 8.5])
        self.check_values(list(d), [8.5, 7, 2.5])

    def test_deque_refcounts_and_gc(self):
        from collections import deque
        obj = Tracked()
        base = sys.getrefcount(obj)
        d = deque([obj] * 200 + [1, 2.5])
        self.assertEqual(sys.getrefcount(obj), base + 200)
        d.rotate(5)
        d.reverse()
        d.popleft()
        d.pop()
        self.assertEqual(sys.getrefcount(obj), base + d.count(obj))
        del d
        self.assertEqual(sys.getrefcount(obj), base)
        d = deque([1, 2.5, obj])
        self.assertEqual(gc.get_referents(d).count(obj), 1)
        self.assertFalse([r for r in gc.get_referents(d) if r in (1, 2.5)
                          and type(r) in (int, float)])
        d.append(d)
        ref = weakref.ref(d)
        del d
        gc.collect()
        self.assertIsNone(ref())

    def test_deque_iterator_pickle(self):
        from collections import deque
        d = deque([1, 2.5, 3, 4])
        it = iter(d)
        next(it)
        it2 = pickle.loads(pickle.dumps(it))
        self.check_values(list(it2), [2.5, 3, 4])
        self.check_values(list(reversed(d)), [4, 3, 2.5, 1])

    def test_lru_cache_values(self):
        import functools
        calls = []

        @functools.lru_cache(maxsize=2)
        def f(x):
            calls.append(x)
            return x

        for x in (1, 2.5, 2.5, 2**70, 2**70, 1):
            self.assertTrue(_same(f(x), x))
        self.assertEqual(calls, [1, 2.5, 2**70, 1])
        info = f.cache_info()
        self.assertEqual((info.hits, info.misses), (2, 4))
        obj = Tracked()
        base = sys.getrefcount(obj)

        @functools.lru_cache(maxsize=2)
        def g(x):
            return obj

        g(1)
        g(2)
        g(3)            # evicts 1
        self.assertEqual(sys.getrefcount(obj), base + 2)
        g.cache_clear()
        self.assertEqual(sys.getrefcount(obj), base)

    def test_cmp_to_key_obj(self):
        import functools
        K = functools.cmp_to_key(lambda a, b: (a > b) - (a < b))
        for v in (3, 2.5, 2**70, -0.0, 'x'):
            self.assertTrue(_same(K(v).obj, v))
        self.assertTrue(K(1) < K(2.5))
        self.assertEqual(sorted([3, 1.5, 2**60, -1], key=K), [-1, 1.5, 3, 2**60])
        self.assertIsNone(K.obj)
        k = K(1)
        k.obj = 2.5          # the member is writable, as it was
        self.assertTrue(_same(k.obj, 2.5))
        del k.obj
        self.assertIsNone(k.obj)

    def test_itemgetter_values(self):
        import operator
        data = [10, 20.5, 2**70]
        self.assertTrue(_same(operator.itemgetter(1)(data), 20.5))
        self.assertEqual(operator.itemgetter(0, 2)(data), (10, 2**70))
        self.assertEqual(operator.itemgetter(2**70)({2**70: 'big'}), 'big')
        self.assertEqual(operator.itemgetter(1.5)({1.5: 'f'}), 'f')
        self.assertEqual(operator.itemgetter(-1)(data), 2**70)
        self.assertEqual(repr(operator.itemgetter(1)), 'operator.itemgetter(1)')
        self.assertEqual(repr(operator.itemgetter(1, 2.5)),
                         'operator.itemgetter(1, 2.5)')
        for ig in (operator.itemgetter(1), operator.itemgetter(1, 2.5)):
            self.assertEqual(repr(pickle.loads(pickle.dumps(ig))), repr(ig))
        key = Tracked()
        base = sys.getrefcount(key)
        ig = operator.itemgetter(key)
        self.assertEqual(sys.getrefcount(key), base + 1)
        ref = [ig]
        ref.append(ref)
        del ig
        del ref
        gc.collect()
        self.assertEqual(sys.getrefcount(key), base)

    def test_simplequeue_values(self):
        _queue = import_helper.import_module('_queue')
        q = _queue.SimpleQueue()
        vals = _values() * 3
        for v in vals:
            q.put(v)
        self.assertEqual(q.qsize(), len(vals))
        got = [q.get() for _ in vals]
        self.check_values(got, vals)
        obj = Tracked()
        base = sys.getrefcount(obj)
        for _ in range(20):
            q.put(obj)
        self.assertEqual(sys.getrefcount(obj), base + 20)
        self.assertEqual(gc.get_referents(q).count(obj), 20)
        del q
        self.assertEqual(sys.getrefcount(obj), base)

    def test_future_result_values(self):
        asyncio = import_helper.import_module('asyncio')
        loop = asyncio.new_event_loop()
        self.addCleanup(loop.close)
        for v in _values():
            fut = loop.create_future()
            fut.set_result(v)
            self.assertTrue(_same(fut.result(), v))
            self.assertTrue(_same(loop.run_until_complete(fut), v))
        obj = Tracked()
        base = sys.getrefcount(obj)
        fut = loop.create_future()
        fut.set_result(obj)
        self.assertEqual(sys.getrefcount(obj), base + 1)
        del fut
        self.assertEqual(sys.getrefcount(obj), base)

    def test_element_text_tail(self):
        ET = import_helper.import_fresh_module('xml.etree.ElementTree',
                                               fresh=['_elementtree'])
        if ET.Element is ET._Element_Py:
            self.skipTest('no C accelerator')
        e = ET.Element('a')
        self.assertIsNone(e.text)
        self.assertIsNone(e.tail)
        for v in (1, 2.5, 2**70, -0.0, 'text', None):
            e.text = v
            e.tail = v
            self.assertTrue(_same(e.text, v))
            self.assertTrue(_same(e.tail, v))
            c = copy.copy(e)
            self.assertTrue(_same(c.text, v))
            d = copy.deepcopy(e)
            self.assertTrue(_same(d.tail, v))
            r = ET.Element('x')
            r.__setstate__(e.__getstate__())
            self.assertTrue(_same(r.text, v))
        # Text from the parser is joined lazily.
        root = ET.fromstring('<a>one<b/>two<![CDATA[three]]>four</a>')
        self.assertEqual(root.text, 'one')
        self.assertEqual(root[0].tail, 'twothreefour')
        # A list assigned by the application is left intact.
        lst = ['x', 'y']
        e.text = lst
        self.assertIs(e.text, lst)
        obj = Tracked()
        base = sys.getrefcount(obj)
        e.text = obj
        e.tail = obj
        self.assertEqual(sys.getrefcount(obj), base + 2)
        e.clear()
        self.assertEqual(sys.getrefcount(obj), base)
