"""Canonical dictionary VM construction, assignment and callback boundaries."""

import sys
import types
import unittest
import weakref

from test import support
from test.support import import_helper

_testinternalcapi = import_helper.import_module('_testinternalcapi')


def handlers(function):
    return [word[0] for word in
            _testinternalcapi.get_ir(function.__code__, True)]


class PyValueDictVMTests(unittest.TestCase):
    def test_literal_owns_canonical_keys_and_values(self):
        def build(key, value):
            return {key: value, 8: 3.25, 'none': None}

        for _ in range(20):
            result = build(2147483647, -2147483648)
            self.assertEqual(_testinternalcapi.dict_vm_storage(result), (3, 2, 2))
            self.assertEqual(result[2147483647], -2147483648)
        self.assertIn('BUILD_MAP', handlers(build))
        self.assertEqual(_testinternalcapi.dict_vm_storage({}), (0, 0, 0))

    def test_cold_and_specialized_assignment(self):
        def assign(mapping, key, value):
            mapping[key] = value

        mapping = {}
        assign(mapping, 2147483647, 2.25)
        self.assertEqual(_testinternalcapi.dict_vm_storage(mapping), (1, 1, 1))
        for value in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            assign(mapping, 2147483647, value)
        self.assertIn('STORE_SUBSCR_DICT', handlers(assign))
        marker = object()
        assign(mapping, 2147483647, marker)
        self.assertIs(mapping[2147483647], marker)
        self.assertEqual(_testinternalcapi.dict_vm_storage(mapping), (1, 1, 0))

    def test_duplicate_numeric_keys_and_large_integer(self):
        def build(first, second, large):
            return {True: first, 1: second, large: large}

        large = 1 << 80
        result = build(2.5, 7, large)
        self.assertEqual(result, {True: 7, large: large})
        self.assertIs(next(iter(result)), True)
        self.assertEqual(_testinternalcapi.dict_vm_storage(result), (2, 0, 1))

    def test_hash_callbacks_keep_evaluated_window(self):
        ledger = []

        class Value:
            def __del__(self):
                ledger.append('value destroyed')

        source = [Value()]
        expected = weakref.ref(source[0])

        class Key:
            def __hash__(self):
                ledger.append('hash')
                source[0] = None
                return 42

        def build(key):
            return {key: source[0]}

        key = Key()
        result = build(key)
        self.assertEqual(ledger, ['hash'])
        self.assertIs(result[key], expected())
        self.assertEqual(source, [None])

    def test_hash_error_is_not_replayed(self):
        ledger = []
        failure = ValueError('hash once')

        class Key:
            def __hash__(self):
                ledger.append('hash')
                raise failure

        def assign(mapping, key):
            mapping[key] = 2147483647

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            assign({}, 1)
        self.assertIn('STORE_SUBSCR_DICT', handlers(assign))
        mapping = {}
        with self.assertRaises(ValueError) as caught:
            assign(mapping, Key())
        self.assertIs(caught.exception, failure)
        self.assertEqual(ledger, ['hash'])
        self.assertEqual(mapping, {})

    def test_generic_mapping_retires_sub_container_value(self):
        ledger = []

        class Operand:
            def __init__(self, name):
                self.name = name

            def __del__(self):
                ledger.append(self.name)

        class Mapping(Operand):
            def __setitem__(self, key, value):
                ledger.append('store')

        def assign():
            Mapping('container')[Operand('sub')] = Operand('value')

        assign()
        self.assertEqual(ledger, ['store', 'sub', 'container', 'value'])

    def test_replaced_value_refreshes_following_dispatch(self):
        ledger = []

        class Old:
            def __del__(self):
                ledger.append(('old', mapping['x']))
                sys.setprofile(lambda *args: None)
                sys.setprofile(None)
                raise RuntimeError('dict old value')

        def assign(mapping, value):
            mapping['x'] = value
            ledger.append('next')

        mapping = {'x': None}
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            assign(mapping, None)
        self.assertIn('STORE_SUBSCR_DICT', handlers(assign))
        ledger.clear()
        mapping['x'] = Old()
        try:
            with support.catch_unraisable_exception() as caught:
                assign(mapping, 2147483647)
                self.assertIs(caught.unraisable.exc_type, RuntimeError)
                self.assertEqual(str(caught.unraisable.exc_value), 'dict old value')
        finally:
            sys.setprofile(None)
        self.assertEqual(ledger, [('old', 2147483647), 'next'])

    def test_hint_retirement_and_watcher_reentry(self):
        _testcapi = import_helper.import_module('_testcapi')
        ledger = []

        class Owner:
            pass

        owner = Owner()
        owner.x = None
        mapping = owner.__dict__

        def assign(owner, value):
            owner.x = value
            ledger.append('next')

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            assign(owner, None)
        self.assertIn('STORE_ATTR_WITH_HINT', handlers(assign))
        ledger.clear()

        class Old:
            def __del__(self):
                ledger.append(('old', mapping['x']))
                sys.setprofile(lambda *args: None)
                sys.setprofile(None)

        mapping['x'] = Old()
        try:
            assign(owner, 2147483647)
        finally:
            sys.setprofile(None)
        self.assertEqual(ledger, [('old', 2147483647), 'next'])

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            assign(owner, 2147483647)
        self.assertIn('STORE_ATTR_WITH_HINT', handlers(assign))

        # The watcher error is reported before mutation. Its unraisable
        # hook may reenter; dictionary callbacks themselves must not mutate.
        watcher = _testcapi.add_dict_watcher(1)
        previous = sys.unraisablehook

        def hook(event):
            ledger.append(('watch', mapping['x']))
            sys.setprofile(lambda *args: None)
            sys.setprofile(None)

        sys.unraisablehook = hook
        try:
            _testcapi.watch_dict(watcher, mapping)
            ledger.clear()
            assign(owner, 3.25)
            self.assertEqual(ledger, [('watch', 2147483647), 'next'])
            self.assertEqual(mapping['x'], 3.25)
        finally:
            sys.unraisablehook = previous
            sys.setprofile(None)
            _testcapi.unwatch_dict(watcher, mapping)
            _testcapi.clear_dict_watcher(watcher)

    def test_hint_aliased_old_field_and_receiver(self):
        ledger = []

        class Owner:
            def __del__(self):
                ledger.append(self.x)

        def create(alias=False):
            owner = Owner()
            owner.x = owner if alias else None
            owner.__dict__
            return owner

        def assign(factory):
            factory().x = 2147483647

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            assign(create)
        self.assertIn('STORE_ATTR_WITH_HINT', handlers(assign))
        ledger.clear()
        assign(lambda: create(True))
        self.assertEqual(ledger, [2147483647])

    def test_cold_and_warm_dictionary_reads_keep_canonical_values(self):
        def read(mapping, key):
            return mapping[key]

        mapping = {2147483647: -2147483648, 2.5: 3.25, 'none': None}
        self.assertEqual(read(mapping, 2147483647), -2147483648)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            self.assertEqual(read(mapping, 2.5), 3.25)
        self.assertIn('BINARY_OP_SUBSCR_DICT', handlers(read))
        self.assertIsNone(read(mapping, 'none'))
        large = 1 << 80
        mapping['large'] = large
        self.assertIs(read(mapping, 'large'), large)
        self.assertEqual(_testinternalcapi.dict_vm_storage(mapping), (4, 2, 2))

    def test_dictionary_read_hash_and_missing_are_once_only(self):
        ledger = []

        class Key:
            def __hash__(self):
                ledger.append('hash')
                return 75

        def read(mapping, key):
            return mapping[key]

        key = Key()
        for cold in (True, False):
            if not cold:
                for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
                    read({1: None}, 1)
                self.assertIn('BINARY_OP_SUBSCR_DICT', handlers(read))
            ledger.clear()
            with self.assertRaises(KeyError) as caught:
                read({}, key)
            self.assertIs(caught.exception.args[0], key)
            self.assertEqual(ledger, ['hash'])

        class Missing(dict):
            def __missing__(self, missing):
                ledger.append(('missing', missing))
                return 2147483647

        ledger.clear()
        self.assertEqual(read(Missing(), key), 2147483647)
        self.assertEqual(ledger, ['hash', ('missing', key)])

    def test_failed_read_preserves_deeper_unwind_order(self):
        ledger = []
        failure = ValueError('read once')

        class Item:
            def __init__(self, name):
                self.name = name

            def __del__(self):
                ledger.append(self.name)

        class Key(Item):
            def __hash__(self):
                ledger.append('read')
                raise failure

        class Mapping(dict):
            # Inherited native __getitem__ adds no traceback frame owning
            # this receiver. An eager wrong close is therefore observable.
            def __del__(self):
                ledger.append('container')

        def read():
            return (Item('deeper'), Mapping()[Key('key')])

        try:
            read()
        except ValueError as error:
            self.assertIs(error, failure)
            # Drop the traceback's frame root before checking final owners.
            error.__traceback__ = None
        else:
            self.fail('read did not raise')
        self.assertEqual(ledger, ['read', 'deeper', 'container', 'key'])

    def test_binary_result_publishes_after_operand_retirement(self):
        ledger = []

        class Operand:
            def __init__(self, name):
                self.name = name

            def __add__(self, other):
                ledger.append('add')
                return 2147483647

            def __del__(self):
                ledger.append(self.name)
                sys.setprofile(lambda *args: None)
                sys.setprofile(None)

        def add():
            value = Operand('left') + Operand('right')
            ledger.append(('next', value))
            return value

        try:
            self.assertEqual(add(), 2147483647)
        finally:
            sys.setprofile(None)
        self.assertEqual(ledger, ['add', 'right', 'left', ('next', 2147483647)])

    def test_global_module_and_builtin_scalar_feedback(self):
        namespace = {'__builtins__': {'selected': 2147483647}}
        exec('def read(): return selected', namespace)
        read = namespace['read']
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            self.assertEqual(read(), 2147483647)
        self.assertIn('LOAD_GLOBAL_BUILTIN', handlers(read))
        namespace['selected'] = None
        self.assertIsNone(read())
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            read()
        self.assertIn('LOAD_GLOBAL_MODULE', handlers(read))
        namespace['selected'] = 3.25
        self.assertEqual(read(), 3.25)
        del namespace['selected']
        self.assertEqual(read(), 2147483647)

    def test_module_and_instance_scalar_read_guards(self):
        def read(owner):
            return owner.x

        module = types.ModuleType('value_reader')
        module.x = 2147483647
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            self.assertEqual(read(module), 2147483647)
        self.assertIn('LOAD_ATTR_MODULE', handlers(read))
        module.x = None
        self.assertIsNone(read(module))
        del module.x
        with self.assertRaises(AttributeError):
            read(module)

        class Owner:
            pass

        owner = Owner()
        owner.x = 3.25
        owner.__dict__
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            self.assertEqual(read(owner), 3.25)
        self.assertTrue(set(handlers(read)) & {
            'LOAD_ATTR_WITH_HINT', 'LOAD_ATTR_INSTANCE_VALUE', 'LOAD_ATTR_SHAPE'})
        Owner.x = property(lambda self: 2147483647)
        self.assertEqual(read(owner), 2147483647)

    def test_cached_receiver_retirement_precedes_folded_local(self):
        ledger = []
        observe = False

        class Old:
            def __del__(self):
                if observe:
                    ledger.append(('old', sys._getframe(1).f_locals['result']))

        class Owner:
            def __init__(self):
                self.x = 2147483647

            def __del__(self):
                if observe:
                    ledger.append(('receiver', isinstance(sys._getframe(1).f_locals['result'], Old)))
                    sys.setprofile(lambda *args: None)
                    sys.setprofile(None)

        def read(factory):
            result = Old()
            result = factory().x
            return result

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            read(Owner)
        self.assertTrue(set(handlers(read)) & {'LOAD_ATTR_INSTANCE_VALUE', 'LOAD_ATTR_SHAPE'})
        observe = True
        try:
            self.assertEqual(read(Owner), 2147483647)
        finally:
            sys.setprofile(None)
        self.assertEqual(ledger, [('receiver', True), ('old', 2147483647)])

    def test_attribute_body_error_retires_receiver_before_deeper(self):
        ledger = []

        class Deeper:
            def __del__(self):
                ledger.append('deeper')

        class Receiver:
            # Native len raises without a Python getter traceback that
            # would keep this temporary receiver alive and mask the order.
            attr = property(len)

            def __del__(self):
                ledger.append('receiver')

        def classic():
            return (Deeper(), Receiver().attr)

        def method():
            return (Deeper(), Receiver().attr())

        for read in (classic, method):
            with self.subTest(read=read.__name__):
                ledger.clear()
                with self.assertRaises(TypeError):
                    read()
                self.assertEqual(ledger, ['receiver', 'deeper'])

    def test_scalar_class_metaclass_and_method_attributes(self):
        class Meta(type):
            scalar = 3.25

        class Owner(metaclass=Meta):
            scalar = 2147483647

        def read(owner):
            return owner.scalar

        self.assertEqual(read(Owner), 2147483647)
        self.assertEqual(read(Owner()), 2147483647)
        del Owner.scalar
        self.assertEqual(read(Owner), 3.25)
        Meta.scalar = property(lambda cls: -2147483648)
        self.assertEqual(read(Owner), -2147483648)

        def numeric(value):
            return value.bit_length()

        self.assertEqual(numeric(2147483647), 31)
        self.assertEqual(numeric(-2147483648), 32)

        class Constant:
            method = 2147483647

        def not_callable(owner):
            return owner.method()

        with self.assertRaisesRegex(TypeError, 'not callable'):
            not_callable(Constant())

    def test_generic_setter_body_error_keeps_owner_value_order(self):
        ledger = []
        failure = ValueError('setter once')

        class Value:
            def __del__(self):
                ledger.append('value')

        class NativeOwner:
            x = property(None, len)

            def __del__(self):
                ledger.append('owner')

        def native_assign():
            NativeOwner().x = Value()

        # The builtin setter's arity error has no Python frame retaining
        # either operand, so this witnesses actual VM retirement order.
        with self.assertRaises(TypeError):
            native_assign()
        self.assertEqual(ledger, ['owner', 'value'])
        ledger.clear()

        class Owner:
            def __setattr__(self, name, value):
                ledger.append('set')
                raise failure

            def __del__(self):
                ledger.append('owner')

        def assign():
            Owner().x = Value()

        try:
            assign()
        except ValueError as error:
            self.assertIs(error, failure)
            error.__traceback__ = None
        else:
            self.fail('setter did not raise')
        self.assertEqual(ledger, ['set', 'owner', 'value'])

    def test_managed_store_handlers_publish_before_retirement(self):
        ledger = []
        observe = False
        toggle = False

        class Owner:
            def __del__(self):
                if observe:
                    ledger.append(('receiver', self.x))

        class Old:
            def __init__(self, owner):
                self.owner = weakref.ref(owner)

            def __del__(self):
                if observe:
                    ledger.append(('field', self.owner().x))
                    if toggle:
                        sys.setprofile(lambda *args: None)
                        sys.setprofile(None)

        def factory(instance=False, old=False, alias=False):
            owner = Owner()
            owner.x = owner if alias else Old(owner) if old else None
            if instance:
                owner.spare = None
                del owner.spare
            return owner

        def shaped(factory, value):
            factory().x = value
            if observe:
                ledger.append('next')

        def unshaped(factory, value):
            factory().x = value
            if observe:
                ledger.append('next')

        threshold = _testinternalcapi.SPECIALIZATION_THRESHOLD + 3
        for put, instance in ((shaped, False), (unshaped, True)):
            make = lambda: factory(instance=instance)
            for _ in range(threshold):
                put(make, None)
            expected = ('STORE_ATTR_INSTANCE_VALUE' if instance or not support.Py_SHAPES
                        else 'STORE_ATTR_SHAPE')
            self.assertIn(expected, handlers(put))
            observe = True
            for value in (2147483647, 3.25, None):
                ledger.clear()
                put(make, value)
                self.assertEqual(ledger, [('receiver', value), 'next'])
            ledger.clear()
            put(lambda: factory(instance=instance, alias=True), 2147483647)
            self.assertEqual(ledger, [('receiver', 2147483647), 'next'])
            ledger.clear()
            toggle = True
            try:
                put(lambda: factory(instance=instance, old=True), 3.25)
            finally:
                sys.setprofile(None)
                toggle = False
                observe = False
            self.assertEqual(ledger, [('field', 3.25), ('receiver', 3.25), 'next'])

        if support.Py_SHAPES:
            def added(factory, value):
                factory().x = value
                if observe:
                    ledger.append('next')

            for _ in range(threshold):
                added(Owner, None)
            self.assertIn('STORE_ATTR_SHAPE_ADD', handlers(added))
            observe = True
            try:
                for value in (2147483647, 3.25, None):
                    ledger.clear()
                    added(Owner, value)
                    self.assertEqual(ledger, [('receiver', value), 'next'])
            finally:
                observe = False

    def test_property_and_method_success_publish_canonical_outputs(self):
        class Owner:
            @property
            def scalar(self):
                return 2147483647

            @property
            def callable(self):
                return (2147483647).bit_length

            def method(self):
                return self.scalar

        def read(owner):
            return owner.scalar

        def property_call(owner):
            return owner.callable()

        def method_call(owner):
            return owner.method()

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            self.assertEqual(read(Owner()), 2147483647)
            self.assertEqual(property_call(Owner()), 31)
            self.assertEqual(method_call(Owner()), 2147483647)
        self.assertTrue(any(name.startswith('LOAD_ATTR_METHOD')
                            for name in handlers(method_call)))


if __name__ == '__main__':
    unittest.main()
