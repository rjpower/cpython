"""PyValue_NULL is zero: zero-filled memory holds empty values."""

import unittest

from test.support import import_helper

_testinternalcapi = import_helper.import_module('_testinternalcapi')

# Enough calls for the handlers to specialize.
WARMUP = 64


def executing_handlers(function):
    return [word[0] for word in
            _testinternalcapi.get_ir(function.__code__, True)]


class ZeroFilledTests(unittest.TestCase):
    def test_generic_alloc_tuple_subclass(self):
        class Tuple(tuple):
            pass

        for size in (1, 3, 9):
            with self.subTest(size=size):
                bits, item = _testinternalcapi.generic_alloc_value_bits(
                    Tuple, size)
                self.assertEqual(bits, [0] * size)
                self.assertIs(type(item), Tuple)
                self.assertEqual(item, tuple(range(size)))

    def test_generic_alloc_tuple_subclass_with_slots(self):
        class Tuple(tuple):
            __slots__ = ('value',)

        bits, item = _testinternalcapi.generic_alloc_value_bits(Tuple, 3)
        self.assertEqual(bits, [0, 0, 0, 0])
        self.assertEqual(item, (0, 1, 2))
        self.assertFalse(hasattr(item, 'value'))
        item.value = 1.5
        self.assertEqual(item.value, 1.5)

    def test_generic_alloc_slots(self):
        class Base:
            __slots__ = ('first',)

        class Item(Base):
            __slots__ = ('second', 'third')

        bits, item = _testinternalcapi.generic_alloc_value_bits(Item, 0)
        self.assertEqual(bits, [0, 0, 0])
        for name in ('first', 'second', 'third'):
            with self.subTest(name=name):
                self.assertFalse(hasattr(item, name))
                with self.assertRaises(AttributeError):
                    delattr(item, name)
                self.assertEqual(
                    _testinternalcapi.slot_value_storage(item, name),
                    ('empty', 0))
        item.first, item.second, item.third = 1234567, 0.5, 'text'
        self.assertEqual((item.first, item.second, item.third),
                         (1234567, 0.5, 'text'))
        del item.second
        self.assertFalse(hasattr(item, 'second'))

    def test_calloc_value_members(self):
        item = _testinternalcapi.CallocValues()
        for name in ('value', 'optional'):
            self.assertEqual(_testinternalcapi.slot_value_storage(item, name),
                             ('empty', 0))
        self.assertFalse(hasattr(item, 'value'))
        self.assertIsNone(item.optional)
        with self.assertRaises(AttributeError):
            del item.value
        del item.optional

        sentinel = object()
        item.value = 1234567
        item.optional = sentinel
        self.assertEqual(item.value, 1234567)
        self.assertIs(item.optional, sentinel)
        self.assertEqual(
            _testinternalcapi.slot_value_storage(item, 'value')[0], 'int')
        del item.value, item.optional
        for name in ('value', 'optional'):
            self.assertEqual(_testinternalcapi.slot_value_storage(item, name),
                             ('empty', 0))
        self.assertIsNone(item.optional)


class Point:
    def __init__(self, x):
        self.x = x

    def shifted(self, dx):
        return Point(self.x + dx)


class UnboundLocalTests(unittest.TestCase):
    # In each function, p is unbound on some paths and is the receiver or
    # the attribute base of instructions whose handlers read it through a
    # field where the compiler proves it bound.

    def test_comprehension_restores_unbound_local(self):
        def f(points, bind):
            if bind:
                p = Point(-1)
                first = p.shifted(1).x + p.x
            else:
                first = None
            # The comprehension saves the outer p with LOAD_FAST_AND_CLEAR
            # and restores it, unbound if it was.
            xs = [p.shifted(1).x + p.x for p in points]
            try:
                last = p.x
            except UnboundLocalError:
                last = None
            return first, xs, last

        points = [Point(i) for i in range(3)]
        for _ in range(WARMUP):
            self.assertEqual(f(points, False), (None, [1, 3, 5], None))
            self.assertEqual(f(points, True), (-1, [1, 3, 5], -1))
            self.assertEqual(f([], False), (None, [], None))
        self.assertIn('LOAD_FAST_AND_CLEAR', executing_handlers(f))

    def test_comprehension_of_numbers_restores_unbound_local(self):
        def f(numbers, bind):
            if bind:
                n = 2.5
                first = n.is_integer()
            else:
                first = None
            out = [n.bit_length() + n.real for n in numbers]
            try:
                last = n.real
            except UnboundLocalError:
                last = None
            return first, out, last

        for _ in range(WARMUP):
            self.assertEqual(f([1, 7, 1 << 40], False),
                             (None, [2, 10, 41 + (1 << 40)], None))
            self.assertEqual(f([1], True), (False, [2], 2.5))

    def test_receiver_and_attribute_base_after_del(self):
        def f(x):
            p = Point(x)
            q = p.shifted(2)
            y = p.x + q.x
            p.x = y
            z = p.shifted(y).x
            del p
            try:
                p.shifted(1)
            except UnboundLocalError:
                pass
            else:
                raise AssertionError('p is bound after del')
            try:
                p.x
            except UnboundLocalError:
                return y, z
            raise AssertionError('p is bound after del')

        for _ in range(WARMUP):
            self.assertEqual(f(1), (4, 8))
            self.assertEqual(f(0.5), (3.0, 6.0))

    def test_conditional_del_then_receiver(self):
        def f(text, drop):
            s = text
            parts = s.split(',')
            if drop:
                del s
            try:
                return parts, s.upper()
            except UnboundLocalError:
                return parts, None

        for _ in range(WARMUP):
            self.assertEqual(f('a,b', False), (['a', 'b'], 'A,B'))
            self.assertEqual(f('a,b', True), (['a', 'b'], None))

    def test_delete_global_stores_null_temporary(self):
        # `del` of a global passes NULL to STORE_GLOBAL in a temporary.
        def f(value):
            global _pyvalue_null_global
            _pyvalue_null_global = value
            seen = _pyvalue_null_global.real
            del _pyvalue_null_global
            try:
                del _pyvalue_null_global
            except NameError:
                return seen, '_pyvalue_null_global' in globals()
            raise AssertionError('deleted an unbound global')

        for _ in range(WARMUP):
            self.assertEqual(f(3), (3, False))
            self.assertEqual(f(2.5), (2.5, False))


if __name__ == '__main__':
    unittest.main()
