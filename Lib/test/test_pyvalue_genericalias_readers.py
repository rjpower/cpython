import gc
import math
import sys
import types
import typing
import unittest
import weakref


class GenericAliasReaderTests(unittest.TestCase):
    def test_construction_forms_and_argument_lifetimes(self):
        sentinel = object()
        for value in (0, 2**40, 2**200, 0.0, -0.0, Ellipsis, sentinel):
            alias = types.GenericAlias(list, value)
            self.assertEqual(len(alias.__args__), 1)
            result = alias.__args__[0]
            if value is sentinel:
                self.assertIs(result, sentinel)
            else:
                self.assertEqual(result, value)
                self.assertIs(type(result), type(value))
        self.assertEqual(math.copysign(1.0, types.GenericAlias(list, -0.0).__args__[0]), -1.0)
        self.assertEqual(types.GenericAlias(list, ()).__args__, ())
        self.assertEqual(types.GenericAlias(list, (1, 2)).__args__, (1, 2))
        nested = [sentinel]
        alias = types.GenericAlias(list, nested)
        del nested
        self.assertIs(alias.__args__[0][0], sentinel)
        class Alias(types.GenericAlias):
            pass
        self.assertIs(type(Alias(list, int)), Alias)
        with self.assertRaises(TypeError):
            types.GenericAlias()
        with self.assertRaises(TypeError):
            types.GenericAlias(list, int, int)
        with self.assertRaises(TypeError):
            types.GenericAlias(origin=list, args=int)

    def test_repr_callbacks_and_nested_arguments(self):
        calls = []
        class Origin:
            def __repr__(self):
                calls.append('origin')
                return 'Origin'
        class Arg:
            def __repr__(self):
                calls.append('arg')
                return 'Arg'
        alias = types.GenericAlias(Origin(), ([Arg()],))
        self.assertEqual(repr(alias), 'Origin[[Arg]]')
        self.assertEqual(calls, ['origin', 'arg'])
        empty = types.GenericAlias(list, ())
        self.assertEqual(repr(empty), 'list[()]')
        class Broken:
            def __repr__(self):
                raise marker
        marker = LookupError('repr marker')
        with self.assertRaises(LookupError) as caught:
            repr(types.GenericAlias(list, Broken()))
        self.assertIs(caught.exception, marker)

    def test_parameter_discovery_and_protocol_metadata(self):
        t = typing.TypeVar('t')
        u = typing.TypeVar('u')
        ts = typing.TypeVarTuple('ts')
        self.assertEqual(types.GenericAlias(list, (t, t, u)).__parameters__, (t, u))
        self.assertEqual(types.GenericAlias(list, (int,)).__parameters__, ())
        self.assertEqual(types.GenericAlias(list, ([t, (u, t)],)).__parameters__, (t, u))
        self.assertEqual(types.GenericAlias(list, (typing.Unpack[ts],)).__parameters__, (ts,))
        class Protocol:
            def __init__(self, params):
                self.params = params
                self.reads = 0
            @property
            def __parameters__(self):
                self.reads += 1
                return self.params
        p = Protocol((t, 7))
        # Metadata is accepted as returned by the protocol, including scalars.
        self.assertEqual(types.GenericAlias(list, (p,)).__parameters__, (t, 7))
        self.assertEqual(p.reads, 1)
        marker = LookupError('parameters marker')
        class BrokenMetadata:
            reads = 0
            @property
            def __parameters__(self):
                self.reads += 1
                raise marker
        broken = BrokenMetadata()
        with self.assertRaises(LookupError) as caught:
            types.GenericAlias(list, (broken,)).__parameters__
        self.assertIs(caught.exception, marker)
        self.assertEqual(broken.reads, 1)
        recursive = Protocol(())
        recursive.params = (recursive,)
        self.assertEqual(types.GenericAlias(list, (recursive,)).__parameters__, (recursive,))

    def test_substitution_shapes_diagnostics_and_callbacks(self):
        t = typing.TypeVar('t')
        alias = types.GenericAlias(list, (t, [t]))
        big = 2**200
        result = alias[big]
        self.assertEqual(result.__args__, (big, [big]))
        self.assertEqual(types.GenericAlias(list, (t,))[(-0.0)].__args__, (-0.0,))
        obj = object()
        self.assertIs(types.GenericAlias(list, (t,))[obj].__args__[0], obj)
        with self.assertRaisesRegex(TypeError, 'Too few arguments'):
            types.GenericAlias(list, (t,))[()]
        with self.assertRaisesRegex(TypeError, 'Too many arguments'):
            types.GenericAlias(list, (t,))[(1, 2)]
        class Substitutable:
            def __init__(self, result):
                self.result = result
                self.calls = []
            def __typing_subst__(self, value):
                self.calls.append(value)
                return self.result
        class ParameterProtocol(Substitutable):
            @property
            def __parameters__(self):
                return (self,)
        replacement = ParameterProtocol(42)
        self.assertEqual(types.GenericAlias(list, (replacement,))[t].__args__, (42,))
        self.assertEqual(replacement.calls, [t])
        marker = RuntimeError('subst marker')
        def fail(value):
            failing.calls.append(value)
            raise marker
        class FailingParameter(Substitutable):
            @property
            def __parameters__(self):
                return (self,)
        failing = FailingParameter(None)
        failing.__typing_subst__ = fail
        with self.assertRaises(RuntimeError) as caught:
            types.GenericAlias(list, (failing,))[t]
        self.assertIs(caught.exception, marker)

    def test_actual_parameter_prepare_chain(self):
        events = []
        aliases = []
        class Prepared(tuple):
            def __new__(cls, values, label):
                result = super().__new__(cls, values)
                result.label = label
                return result
            def __del__(self):
                events.append(('retired', self.label))
        class Parameter:
            def __init__(self, label):
                self.label = label
            def __typing_prepare_subst__(self, alias, args):
                aliases.append(alias is outer)
                events.append(('prepare', self.label, tuple(args)))
                if self.label == 'first':
                    return Prepared((int, str), 'first')
                return Prepared((float, bytes), 'second')
            def __typing_subst__(self, value):
                events.append(('subst', self.label, value))
                return value
        first, second = Parameter('first'), Parameter('second')
        outer = types.GenericAlias(tuple, (first, second))
        result = outer[(0, 1)]
        self.assertEqual(result.__args__, (float, bytes))
        self.assertEqual(aliases, [True, True])
        self.assertEqual(events, [
            ('prepare', 'first', (0, 1)),
            ('prepare', 'second', (int, str)),
            ('retired', 'first'),
            ('subst', 'first', float),
            ('subst', 'second', bytes),
            ('retired', 'second'),
        ])

    def test_actual_parameter_prepare_nontuple(self):
        events = []
        class Parameter:
            def __typing_prepare_subst__(self, alias, args):
                events.append(('prepare', args))
                return str
            def __typing_subst__(self, value):
                events.append(('subst', value))
                return value
        parameter = Parameter()
        alias = types.GenericAlias(list, (parameter,))
        result = alias[int]
        self.assertEqual(result.__args__, (str,))
        self.assertEqual(events, [('prepare', (int,)), ('subst', str)])

    def test_variadic_unpack_substitution(self):
        ts = typing.TypeVarTuple('ts')
        unpacked = types.GenericAlias(list, (typing.Unpack[ts],))
        for values, expected in (((), ()), ((int,), (int,)), ((int, str), (int, str))):
            self.assertEqual(unpacked[values].__args__, expected)
        self.assertEqual(types.GenericAlias(list, (typing.Unpack[ts], Ellipsis))[()].__args__, (Ellipsis,))
        self.assertEqual(types.GenericAlias(list, (int, Ellipsis)).__args__, (int, Ellipsis))
        with self.assertRaisesRegex(TypeError, 'not a generic class'):
            types.GenericAlias(list, (int,))[str]
        self.assertEqual(types.GenericAlias(list, (typing.Unpack[ts],))[()].__args__, ())

    def test_repr_drops_external_roots(self):
            events = []
            holders = {}
            class Origin:
                def __repr__(self):
                    events.append('origin')
                    holders.clear()
                    return 'Origin'
            class Argument:
                def __repr__(self):
                    events.append('argument')
                    return 'Argument'
            origin, argument = Origin(), Argument()
            origin_ref, argument_ref = weakref.ref(origin), weakref.ref(argument)
            args = (argument,)
            holders.update(alias=types.GenericAlias(origin, args), origin=origin, args=args)
            del origin, argument, args
            self.assertEqual(repr(holders['alias']), 'Origin[Argument]')
            self.assertEqual(events, ['origin', 'argument'])
            self.assertEqual(holders, {})
            gc.collect()
            self.assertIsNone(origin_ref())
            self.assertIsNone(argument_ref())

    def test_nested_getitem_once_and_unmatched_parameter(self):
            t = typing.TypeVar('T')
            calls = []
            result_token = object()
            class Nested:
                __parameters__ = (t,)
                def __getitem__(self, args):
                    calls.append(args)
                    return result_token
            nested = Nested()
            alias = types.GenericAlias(list, (nested,))
            result = alias[2**200]
            self.assertIs(result.__args__[0], result_token)
            self.assertEqual(calls, [(2**200,)])
            calls.clear()
            cached = alias.__parameters__
            self.assertEqual(cached, (t,))
            unknown = object()
            nested.__parameters__ = (unknown,)
            result = alias[str]
            self.assertIs(result.__args__[0], result_token)
            self.assertEqual(calls, [(unknown,)])

    def test_unpacked_custom_parameter_invalid_output(self):
            calls = []
            class Bad:
                __typing_is_unpacked_typevartuple__ = True
                def __typing_subst__(self, value):
                    calls.append(value)
                    return 1000
            parameter = Bad()
            alias = types.GenericAlias(list, (parameter,))
            with self.assertRaises(TypeError) as caught:
                alias[str]
            self.assertRegex(str(caught.exception),
                             r'expected __typing_subst__ of .*Bad objects to return a tuple, not int')
            self.assertEqual(calls, [str])

    @unittest.skipIf(sys.version_info < (3, 16), "current 3.16 stale-parameter guard; 3.14 reference probe crashes")
    def test_stale_parameter_exact_diagnostic(self):
            t = typing.TypeVar('T')
            calls = []
            class Late:
                def __repr__(self):
                    return 'Late'
            late = Late()
            alias = types.GenericAlias(list, (t, late))
            self.assertEqual(alias.__parameters__, (t,))
            late.__typing_subst__ = lambda value: calls.append(value)
            with self.assertRaises(TypeError) as caught:
                alias[int]
            self.assertEqual(str(caught.exception),
                             'argument Late with __typing_subst__ was not found in __parameters__')
            self.assertEqual(calls, [])
if __name__ == '__main__':
    unittest.main()
