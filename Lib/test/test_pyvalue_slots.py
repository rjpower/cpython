"""Canonical instance fields and ownership at attribute boundaries."""

import gc
import sys
import unittest
import weakref

from test.support import import_helper

_testinternalcapi = import_helper.import_module('_testinternalcapi')


def executing_handlers(function):
    return [word[0] for word in
            _testinternalcapi.get_ir(function.__code__, True)]


class PyValueSlotTests(unittest.TestCase):
    def test_initialized_before_reference_tracer(self):
        class Item:
            __slots__ = ('value',)

        item = _testinternalcapi.slot_new_with_tracer(Item, ())
        self.assertEqual(_testinternalcapi.slot_value_storage(item, 'value'),
                         ('empty', 0))

    def test_empty_none_delete_and_canonical_numbers(self):
        class Item:
            __slots__ = ('value',)

        item = Item()
        self.assertEqual(_testinternalcapi.slot_value_storage(item, 'value'),
                         ('empty', 0))
        self.assertFalse(hasattr(item, 'value'))
        with self.assertRaises(AttributeError):
            del item.value
        for value, kind in [(None, 'object'), (True, 'object'),
                            (-(2**50), 'int'), (2**50 - 1, 'int'),
                            (0.5, 'float'), (float('inf'), 'float'),
                            (2**50, 'object'), (object(), 'object')]:
            item.value = value
            self.assertEqual(item.value, value)
            self.assertEqual(_testinternalcapi.slot_value_storage(
                item, 'value')[0], kind)
            del item.value
            self.assertEqual(_testinternalcapi.slot_value_storage(
                item, 'value'), ('empty', 0))

    def test_managed_values_initialized_before_reference_tracer(self):
        class Managed:
            def __init__(self):
                self.number = 37
                self.fraction = 1.5

        for _ in range(4):
            item = _testinternalcapi.slot_new_with_tracer(Managed, ())
            self.assertEqual((item.number, item.fraction), (37, 1.5))
            self.assertEqual(item.__dict__, {'number': 37, 'fraction': 1.5})

    def test_inherited_managed_and_declared_slots_before_reference_tracer(self):
        class Base:
            pass

        class Child(Base):
            __slots__ = ('value',)

        for _ in range(4):
            item = _testinternalcapi.slot_new_with_tracer(Child, ())
            self.assertEqual(_testinternalcapi.slot_value_storage(item, 'value'),
                             ('empty', 0))
            item.number = 37
            item.value = 1.5
            self.assertEqual((item.number, item.value), (37, 1.5))

    def test_inherited_and_variable_tuple_slots(self):
        class Base:
            __slots__ = ('first',)

        class Child(Base):
            __slots__ = ('second',)

        child = Child()
        for name in ('first', 'second'):
            self.assertEqual(_testinternalcapi.slot_value_storage(child, name),
                             ('empty', 0))
        child.first, child.second = 1234567, 0.5
        self.assertEqual((child.first, child.second), (1234567, 0.5))

        class Tuple(tuple):
            __slots__ = ('value',)

        for size in (0, 1, 9):
            item = _testinternalcapi.slot_new_with_tracer(Tuple, (range(size),))
            self.assertEqual(_testinternalcapi.slot_value_storage(item, 'value'),
                             ('empty', 0))
            item.value = 1234567
            self.assertEqual(tuple(item), tuple(range(size)))
            self.assertEqual(item.value, 1234567)

    def test_scalar_subclass_and_nan_stay_objects(self):
        class Item:
            __slots__ = ('value',)

        class Number(int):
            pass

        item = Item()
        for value in (Number(3), float('nan')):
            item.value = value
            self.assertIs(item.value, value)
            self.assertEqual(_testinternalcapi.slot_value_storage(
                item, 'value')[0], 'object')

    def test_class_property_and_custom_attribute_semantics(self):
        class Item:
            __slots__ = ('value',)

        class Other:
            __slots__ = ('value',)

        item = Item()
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            self.assertIs(item.__class__, Item)
        item.__class__ = Other
        self.assertIs(item.__class__, Other)
        item.value = None
        self.assertIsNone(item.value)

        class Custom:
            __slots__ = ('_value',)

            @property
            def value(self):
                return self._value + 1

            @value.setter
            def value(self, value):
                self._value = value * 2

        custom = Custom()
        custom.value = 4
        self.assertEqual(custom.value, 9)
        self.assertEqual(_testinternalcapi.slot_value_storage(
            custom, '_value')[0], 'int')

    def test_replacement_publishes_before_finalizer(self):
        class Item:
            __slots__ = ('value',)

        item = Item()
        events = []

        class Old:
            def __del__(self):
                events.append(item.value)
                sys.setprofile(lambda *args: None)
                sys.setprofile(None)

        def replace():
            item.value = 1234567

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            replace()
        self.assertIn('STORE_ATTR_SLOT', executing_handlers(replace))
        item.value = Old()
        try:
            replace()
        finally:
            sys.setprofile(None)
        self.assertEqual(item.value, 1234567)
        self.assertEqual(events, [1234567])

    def test_temporary_owner_retires_after_old_field(self):
        events = []

        class Old:
            def __del__(self):
                events.append('old field')

        class Item:
            __slots__ = ('value',)

            def __del__(self):
                events.append(('owner', self.value))

        def make():
            item = Item()
            item.value = Old()
            return item

        def assign():
            make().value = 1234567

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            events.clear()
            assign()
            self.assertEqual(events, ['old field', ('owner', 1234567)])
        self.assertIn('STORE_ATTR_SLOT', executing_handlers(assign))

    def test_load_owner_retires_before_folded_local_replacement(self):
        # The result is stored into the local, then the owner and the
        # displaced value are released.  Each finalizer runs once and finds
        # the result in the local; the order between them is free.
        events = []
        reenter = False

        def check(events):
            self.assertEqual(sorted(events),
                             [('old', 1234567), ('owner', 1234567)])

        class Old:
            def __del__(self):
                frame = sys._getframe(1)
                events.append(('old', frame.f_locals['old']))

        class Item:
            __slots__ = ('value',)

            def __del__(self):
                frame = sys._getframe(1)
                events.append(('owner', frame.f_locals['old']))
                if reenter:
                    sys.setprofile(lambda *args: None)
                    sys.setprofile(None)

        def make():
            item = Item()
            item.value = 1234567
            return item

        def read():
            old = Old()
            old = make().value
            return old

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            events.clear()
            self.assertEqual(read(), 1234567)
            check(events)
        self.assertIn('LOAD_ATTR_SLOT', executing_handlers(read))
        events.clear()
        reenter = True
        try:
            self.assertEqual(read(), 1234567)
        finally:
            sys.setprofile(None)
        check(events)

    def test_field_and_receiver_alias_retirement(self):
        events = []

        class Item:
            __slots__ = ('value',)

            def __del__(self):
                events.append(self.value)

        def make():
            item = Item()
            item.value = item
            return item

        def assign():
            make().value = 1234567

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            events.clear()
            assign()
            self.assertEqual(events, [1234567])
        self.assertIn('STORE_ATTR_SLOT', executing_handlers(assign))

    def test_clear_and_cycles_release_pointer_values(self):
        class Item:
            __slots__ = ('value', '__weakref__')

        item = Item()
        item.value = item
        ref = weakref.ref(item)
        del item
        gc.collect()
        self.assertIsNone(ref())

        item = Item()
        item.value = 0.5
        _testinternalcapi.slot_clear(item)
        self.assertEqual(_testinternalcapi.slot_value_storage(item, 'value'),
                         ('empty', 0))
        self.assertFalse(hasattr(item, 'value'))


class PointerFieldTests(unittest.TestCase):
    """LOAD_ATTR_SLOT_POINTER: a PyObject * member or the type of the owner."""

    HANDLER = 'LOAD_ATTR_SLOT_POINTER'

    def warm(self, function, *args):
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            result = function(*args)
        return result

    def test_code_members_and_class(self):
        def target():
            pass

        def filename(code):
            return code.co_filename

        def name(code):
            return code.co_name

        def cls(owner):
            return owner.__class__

        code = target.__code__
        self.assertEqual(self.warm(filename, code), __file__)
        self.assertEqual(self.warm(name, code), 'target')
        self.assertIs(self.warm(cls, code), type(code))
        for function in (filename, name, cls):
            self.assertIn(self.HANDLER, executing_handlers(function))
        self.assertEqual(filename(code), __file__)
        self.assertEqual(name(code), 'target')

    def test_members_do_not_leak_references(self):
        def args(partial):
            return partial.args

        def func(partial):
            return partial.func

        import functools
        payload = (object(), [])
        partial = functools.partial(print, *payload)
        self.warm(args, partial)
        self.warm(func, partial)
        before = (sys.getrefcount(partial.args), sys.getrefcount(partial.func))
        for _ in range(1000):
            args(partial)
            func(partial)
        self.assertEqual(
            (sys.getrefcount(partial.args), sys.getrefcount(partial.func)),
            before)
        self.assertIn(self.HANDLER, executing_handlers(args))
        self.assertIs(args(partial), partial.args)

    def test_null_member_reads_none(self):
        def name(error):
            return error.name

        error = ImportError('message')
        self.assertIsNone(self.warm(name, error))
        self.assertIn(self.HANDLER, executing_handlers(name))
        # A NULL field leaves the specialised form and the generic read
        # answers None, as it does before the field is set.
        self.assertIsNone(name(error))
        for value in ('module', 5, 2**70, 1.5, None, (1, 2), error):
            with self.subTest(value=value):
                error.name = value
                self.assertEqual(name(error), value)
                self.assertIs(type(name(error)), type(value))
                del error.name
                self.assertIsNone(name(error))

    def test_inline_values_in_members(self):
        def path(error):
            return error.path

        error = ImportError('message')
        for value in (0, -(2**50), 2**50 - 1, 0.0, -0.0, 0.5, float('inf'),
                      True):
            with self.subTest(value=value):
                error.path = value
                for _ in range(3):
                    result = self.warm(path, error)
                self.assertEqual(result, value)
                self.assertIs(type(result), type(value))
        self.assertIn(self.HANDLER, executing_handlers(path))

    def test_subclasses_and_layouts(self):
        def name(error):
            return error.name

        class Sub(ImportError):
            pass

        class Slotted(ImportError):
            __slots__ = ('extra',)

        class Other(Exception):
            name = 'class attribute'

        errors = [ImportError(name='a'), Sub(name='b'), Slotted(name='c'),
                  ModuleNotFoundError(name='d'), Other()]
        expected = ['a', 'b', 'c', 'd', 'class attribute']
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            self.assertEqual([name(e) for e in errors], expected)
        self.assertEqual([name(e) for e in reversed(errors)],
                         expected[::-1])

    def test_type_changes_leave_the_specialisation(self):
        def name(error):
            return error.name

        class Sub(ImportError):
            pass

        error = Sub(name='field')
        self.assertEqual(self.warm(name, error), 'field')
        self.assertIn(self.HANDLER, executing_handlers(name))
        Sub.name = property(lambda self: 'property')
        self.assertEqual(name(error), 'property')
        self.assertEqual(self.warm(name, error), 'property')
        del Sub.name
        self.assertEqual(self.warm(name, error), 'field')

    def test_dunder_class(self):
        def cls(owner):
            return owner.__class__

        class A:
            pass

        class B:
            pass

        class Property:
            @property
            def __class__(self):
                return 'property'

        a = A()
        self.assertIs(self.warm(cls, a), A)
        self.assertIn(self.HANDLER, executing_handlers(cls))
        a.__class__ = B
        self.assertIs(cls(a), B)
        self.assertEqual(cls(Property()), 'property')
        self.assertEqual(self.warm(cls, Property()), 'property')
        for value, expected in [(1, int), (2**70, int), (1.5, float),
                                (None, type(None)), ('s', str), (A, type),
                                (sys, type(sys)), (a, B)]:
            self.assertIs(cls(value), expected)


if __name__ == '__main__':
    unittest.main()
