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
                         ('empty', 1))

    def test_empty_none_delete_and_canonical_numbers(self):
        class Item:
            __slots__ = ('value',)

        item = Item()
        self.assertEqual(_testinternalcapi.slot_value_storage(item, 'value'),
                         ('empty', 1))
        self.assertFalse(hasattr(item, 'value'))
        with self.assertRaises(AttributeError):
            del item.value
        for value, kind in [(None, 'object'), (True, 'object'),
                            (-(2**31), 'int'), (2**31 - 1, 'int'),
                            (0.5, 'float'), (float('inf'), 'float'),
                            (2**31, 'object'), (object(), 'object')]:
            item.value = value
            self.assertEqual(item.value, value)
            self.assertEqual(_testinternalcapi.slot_value_storage(
                item, 'value')[0], kind)
            del item.value
            self.assertEqual(_testinternalcapi.slot_value_storage(
                item, 'value'), ('empty', 1))

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
                             ('empty', 1))
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
                             ('empty', 1))
        child.first, child.second = 1234567, 0.5
        self.assertEqual((child.first, child.second), (1234567, 0.5))

        class Tuple(tuple):
            __slots__ = ('value',)

        for size in (0, 1, 9):
            item = _testinternalcapi.slot_new_with_tracer(Tuple, (range(size),))
            self.assertEqual(_testinternalcapi.slot_value_storage(item, 'value'),
                             ('empty', 1))
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
        events = []
        reenter = False

        class Old:
            def __del__(self):
                frame = sys._getframe(1)
                events.append(('old', frame.f_locals['old'] == 1234567))

        class Item:
            __slots__ = ('value',)

            def __del__(self):
                frame = sys._getframe(1)
                events.append(('owner', isinstance(frame.f_locals['old'], Old)))
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
            self.assertEqual(events, [('owner', True), ('old', True)])
        self.assertIn('LOAD_ATTR_SLOT', executing_handlers(read))
        events.clear()
        reenter = True
        try:
            self.assertEqual(read(), 1234567)
        finally:
            sys.setprofile(None)
        self.assertEqual(events, [('owner', True), ('old', True)])

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
                         ('empty', 1))
        self.assertFalse(hasattr(item, 'value'))


if __name__ == '__main__':
    unittest.main()
