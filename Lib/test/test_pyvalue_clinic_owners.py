"""Owning tuple parsers through registered constructors and real call routes."""

import math
import os
import struct
import sys
import types
import typing
import unittest
import weakref

from test.support import import_helper


class ClinicOwnerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.clinic = import_helper.import_module('_testclinic')

    def test_scalar_and_heap_ordinary_and_ex(self):
        marker = object()
        values = (2147483647, -2147483648, True, False, None, 3.25,
                  -0.0, float('nan'), 2**100, marker)
        echo = self.clinic.OwnerEcho
        for value in values:
            for _ in range(40):
                results = (echo(value), echo(value=value),
                           echo(*(value,)), echo(**{'value': value}))
                for result in results:
                    if isinstance(value, float):
                        if math.isnan(value):
                            self.assertTrue(math.isnan(result))
                        else:
                            self.assertEqual(struct.pack('d', result),
                                             struct.pack('d', value))
                    elif value is marker or isinstance(value, bool) or value is None:
                        self.assertIs(result, value)
                    else:
                        self.assertEqual(result, value)

    def test_keyword_output_has_independent_owner(self):
        class Value:
            pass

        mapping = {'value': Value()}
        reference = weakref.ref(mapping['value'])
        result = self.clinic.OwnerAlias((self.clinic.OwnerEcho, mapping))
        mapping.clear()
        self.assertIs(result, reference())
        del result
        self.assertIsNone(reference())

    def test_keyword_owner_survives_alias_mutation_and_reentry(self):
        for ex in (False, True):
            events = []
            mapping = {}

            class Value:
                def visit(self):
                    events.append('visit')
                    mapping.clear()
                    # Inspect a real Python frame while the native wrapper is live.
                    self.assert_frame = sys._getframe().f_code.co_name
                    self_echo = self_outer.clinic.OwnerEcho(self)
                    self_outer.assertIs(self_echo, self)

                def __repr__(self):
                    events.append('repr')
                    return 'retained parser owner'

                def __del__(self):
                    events.append('del')

            self_outer = self
            mapping['value'] = Value()
            context = (self.clinic.OwnerProbe, mapping)
            if ex:
                result = self.clinic.OwnerAlias(*(context,))
            else:
                result = self.clinic.OwnerAlias(context)
            self.assertEqual(result, 'retained parser owner')
            self.assertEqual(events, ['visit', 'repr', 'del'])
            self.assertEqual(mapping, {})

    def test_primary_callback_error_survives_owner_cleanup(self):
        events = []
        mapping = {}
        error = LookupError('parser callback marker')

        class Value:
            def visit(self):
                mapping.clear()
                events.append('visit')
                raise error

            def __repr__(self):
                events.append('unexpected repr')
                return 'unexpected'

            def __del__(self):
                events.append('del')

        mapping['value'] = Value()
        with self.assertRaises(LookupError) as caught:
            self.clinic.OwnerAlias(*((self.clinic.OwnerProbe, mapping),))
        self.assertIs(caught.exception, error)
        # assertRaises clears the traceback, including visit's self owner.
        self.assertEqual(events, ['visit', 'del'])

    def test_repr_error_survives_owner_cleanup(self):
        mapping = {}
        error = RuntimeError('repr marker')

        class Value:
            def visit(self):
                mapping.clear()

            def __repr__(self):
                raise error

        mapping['value'] = Value()
        with self.assertRaises(RuntimeError) as caught:
            self.clinic.OwnerAlias((self.clinic.OwnerProbe, mapping))
        self.assertIs(caught.exception, error)

    def test_invalid_shapes_do_not_invoke_implementation(self):
        echo = self.clinic.OwnerEcho
        for args, kwargs in (((), {}), ((1, 2), {}), ((), {'wrong': 1}),
                             ((1,), {'value': 2}), ((), {1: 'invalid'})):
            with self.subTest(args=args, kwargs=kwargs):
                with self.assertRaises(TypeError):
                    echo(*args, **kwargs)

    def test_missing_argument_keeps_precedence_over_unknown_keyword(self):
        with self.assertRaisesRegex(TypeError, "missing required argument 'value'"):
            self.clinic.OwnerEcho(vaule=1)


class ProductionConstructorTests(unittest.TestCase):
    def test_memoryview_and_mappingproxy(self):
        for ex in (False, True):
            mapping = {'key': 2147483647}
            view = (memoryview(*(b'abc',)) if ex else memoryview(object=b'abc'))
            self.assertEqual(view.tobytes(), b'abc')
            view.release()
            proxy = (types.MappingProxyType(*(mapping,)) if ex else
                     types.MappingProxyType(mapping=mapping))
            self.assertEqual(proxy['key'], 2147483647)

    def test_paramspec_wrappers(self):
        parameter = typing.ParamSpec('P')
        for factory in (typing.ParamSpecArgs, typing.ParamSpecKwargs):
            self.assertIs(factory(parameter).__origin__, parameter)
            self.assertIs(factory(origin=parameter).__origin__, parameter)
            self.assertIs(factory(*(parameter,)).__origin__, parameter)

    @unittest.skipUnless(os.name == 'posix', 'POSIX constructor')
    def test_sched_param(self):
        self.assertEqual(os.sched_param(sched_priority=7).sched_priority, 7)
        self.assertEqual(os.sched_param(*(9,)).sched_priority, 9)

    def test_struct_new_init_and_explicit_subclass_new(self):
        self.assertEqual(struct.Struct(format='>i').pack(17), b'\0\0\0\x11')
        self.assertEqual(struct.Struct(*('>i',)).size, 4)

        class Derived(struct.Struct):
            def __new__(cls, format):
                return struct.Struct.__new__(cls, format=format)

        value = Derived('>h')
        self.assertEqual(value.pack(17), b'\0\x11')
        value.__init__(format='>i')
        self.assertEqual(value.size, 4)

    def test_struct_invalid_format(self):
        with self.assertRaises(TypeError):
            struct.Struct(2147483647)
        with self.assertRaises(struct.error):
            struct.Struct(format='!invalid')

    def test_zoneinfo_constructor(self):
        zoneinfo = import_helper.import_module('zoneinfo')
        try:
            value = zoneinfo.ZoneInfo(key='UTC')
        except zoneinfo.ZoneInfoNotFoundError:
            self.skipTest('UTC zone data unavailable')
        self.assertEqual(value.key, 'UTC')
        self.assertIs(zoneinfo.ZoneInfo(*('UTC',)), value)

    def test_binary_reader_init_error(self):
        debugging = import_helper.import_module('_remote_debugging')
        with self.assertRaises(TypeError):
            debugging.BinaryReader(filename=object())


if __name__ == '__main__':
    unittest.main()
