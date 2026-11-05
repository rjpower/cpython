"""The value vectorcall (M3a).

Covers pyjit/design/proposals/value-api-m3.md, sections 1.1-1.6 and the
core cases of section 3. The helpers are in
Modules/_testinternalcapi/value_call.c.
"""

import enum
import gc
import sys
import unittest

from test.support import import_helper

_testinternalcapi = import_helper.import_module('_testinternalcapi')

vc_args = _testinternalcapi.vc_args
vc_noargs = _testinternalcapi.vc_noargs
vc_one = _testinternalcapi.vc_one
vc_self = _testinternalcapi.vc_self
vc_method = _testinternalcapi.vc_method
make_int_method = _testinternalcapi.make_int_method
bad_flags = _testinternalcapi.bad_flags
value_vectorcall_is_set = _testinternalcapi.value_vectorcall_is_set
object_vectorcall_is_thunk = _testinternalcapi.object_vectorcall_is_thunk
call_value = _testinternalcapi.call_value
call_one_arg = _testinternalcapi.call_one_arg
call_dict = _testinternalcapi.call_dict
vc_call_first = _testinternalcapi.vc_call_first
vectorcall_offsets = _testinternalcapi.vectorcall_offsets
VCall = _testinternalcapi.VCall
VCallBase = _testinternalcapi.VCallBase

# The METH_* flags that cannot be combined with METH_VALUE.
METH_VARARGS = 0x0001
METH_KEYWORDS = 0x0002
METH_NOARGS = 0x0004
METH_O = 0x0008
METH_FASTCALL = 0x0080

Py_TPFLAGS_HAVE_VECTORCALL = 1 << 11


class Recorder:
    def m(self, *args, **kwargs):
        return (self, args, kwargs)

    def one(self, x):
        return (self, x)


class MetaclassCallTests(unittest.TestCase):
    def test_metaclass_call(self):
        constructed = []

        class M(type):
            def __call__(cls, *args, **kwargs):
                return ('M.__call__', args, kwargs)

        class C(metaclass=M):
            def __init__(self, *args, **kwargs):
                constructed.append(self)

        self.assertEqual(C(1, a=2), ('M.__call__', (1,), {'a': 2}))
        self.assertEqual(constructed, [])
        self.assertFalse(M.__flags__ & Py_TPFLAGS_HAVE_VECTORCALL)

    def test_metaclass_call_assigned_and_deleted(self):
        class M(type):
            pass

        class C(metaclass=M):
            pass

        self.assertIsInstance(C(), C)
        M.__call__ = lambda cls, *args, **kwargs: 'M.__call__'
        self.assertEqual(C(), 'M.__call__')
        self.assertFalse(M.__flags__ & Py_TPFLAGS_HAVE_VECTORCALL)
        del M.__call__
        self.assertIsInstance(C(), C)
        self.assertFalse(M.__flags__ & Py_TPFLAGS_HAVE_VECTORCALL)

    def test_enum(self):
        class Color(enum.Enum):
            RED = 1
            GREEN = 2

        self.assertIs(Color.RED, Color(1))
        self.assertIs(Color(2), Color.GREEN)
        self.assertEqual(Color.RED.value, 1)
        self.assertRaises(ValueError, Color, 3)

    def test_c_metaclass_with_tp_call(self):
        _testcapi = import_helper.import_module('_testcapi')
        Meta = _testcapi.MetaTpCall
        # PyType_Ready inherited both offsets from type and cleared the
        # flag, because the metaclass has its own tp_call.
        self.assertEqual(vectorcall_offsets(Meta), vectorcall_offsets(type))
        self.assertFalse(Meta.__flags__ & Py_TPFLAGS_HAVE_VECTORCALL)

        class C(metaclass=Meta):
            def __init__(self, x):
                self.x = x

        tag, obj = C(5)
        self.assertEqual(tag, 'MetaTpCall')
        self.assertIsInstance(obj, C)
        self.assertEqual(obj.x, 5)
        tag, obj = call_value(C, 6)
        self.assertEqual((tag, obj.x), ('MetaTpCall', 6))

        class EnumMeta(enum.EnumType, Meta):
            pass

        class Color(enum.Enum, metaclass=EnumMeta):
            RED = 1

        self.assertIs(Color(1), Color.RED)
        self.assertEqual(Color.RED.value, 1)


class HashTests(unittest.TestCase):
    def test_hash_builtin_method(self):
        lst = []
        self.assertEqual(hash(lst.append), hash(lst.append))
        self.assertEqual(hash(lst.append), hash(lst.append))

    def test_hash_does_not_run_hash_of_self(self):
        calls = []

        class C:
            def m(self):
                pass

            def __hash__(self):
                calls.append(1)
                raise RuntimeError('__hash__ must not run')

        o = C()
        self.assertEqual(hash(o.m), hash(o.m))
        self.assertEqual(calls, [])

    def test_hash_int_method_descriptor(self):
        d = make_int_method('vc_int_method')
        self.assertEqual(hash(d.__get__(5)), hash(d.__get__(5)))
        big = 2**100
        self.assertEqual(hash(d.__get__(big)), hash(d.__get__(big)))


class MethodDescriptorMessageTests(unittest.TestCase):
    def test_unbound_method_needs_an_argument(self):
        with self.assertRaisesRegex(
                TypeError, r"unbound method str\.lower\(\) needs an argument"):
            str.lower()


class ProfilerNameTests(unittest.TestCase):
    def test_static_builtin_method_name(self):
        # A METH_STATIC builtin has a self (the type): _lsprof names it as a
        # built-in method, as upstream does.
        import _lsprof
        prof = _lsprof.Profiler()
        prof.enable()
        str.maketrans('a', 'b')
        prof.disable()
        names = [str(entry.code) for entry in prof.getstats()]
        self.assertIn('<built-in method builtins.maketrans>', names)


class SlotWrapperTests(unittest.TestCase):
    def test_int_add(self):
        self.assertEqual(int.__add__(5, 2), 7)
        self.assertEqual((5).__add__(2), 7)
        self.assertEqual(int.__add__(5, 2, **{}), 7)

    def test_wrong_self(self):
        with self.assertRaisesRegex(TypeError, "requires a 'int' object"):
            int.__add__('x', 2)

    def test_keywords_rejected(self):
        with self.assertRaisesRegex(TypeError, "takes no keyword arguments"):
            int.__add__(5, 2, x=1)


class RecursionTests(unittest.TestCase):
    def test_call_is_instance_of_itself(self):
        class A:
            pass

        A.__call__ = A()
        with self.assertRaises(RecursionError):
            A()()

    def test_call_chain_through_bound_call(self):
        class A:
            pass

        class B:
            pass

        A.__call__ = A()
        B.__call__ = A().__call__
        with self.assertRaises(RecursionError):
            B()()


class WindowTests(unittest.TestCase):
    def test_dict_values_outlive_the_dict(self):
        # The callee reaches the caller's dict and clears it; the keyword
        # values bound at entry stay alive, because the window owns them.
        kwargs = {'a': [1], 'b': 2.5}

        def clear():
            kwargs.clear()
            gc.collect()

        self.assertEqual(call_dict(vc_call_first, (clear,), kwargs),
                         ([1], 2.5))
        self.assertEqual(kwargs, {})


class BoundMethodTests(unittest.TestCase):
    def test_one_arg(self):
        o = Recorder()
        before = sys.getrefcount(o)
        self.assertEqual(call_one_arg(o.one, 7), (o, 7))
        self.assertEqual(sys.getrefcount(o), before)
        self.assertEqual(o.one(7), (o, 7))
        self.assertEqual(sys.getrefcount(o), before)

    def test_self_first(self):
        o = Recorder()
        self.assertEqual(call_one_arg(o.m, 1), (o, (1,), {}))
        self.assertEqual(o.m(1), (o, (1,), {}))

    def test_call_value_sizes(self):
        o = Recorder()
        method = o.m
        before = sys.getrefcount(o)
        for n in (0, 1, 5, 6, 100):
            with self.subTest(n=n):
                t = tuple(range(n))
                self.assertEqual(call_value(method, *t), (o, t, {}))
                self.assertEqual(method(*t), (o, t, {}))
        self.assertEqual(sys.getrefcount(o), before)

    def test_prepend_under_gc(self):
        class C:
            def in_generator(self, *args):
                gen = (gc.collect() for _ in range(1))
                list(gen)
                return (self, args)

            def with_locals(self, *args):
                frame = sys._getframe()
                frame.f_locals
                gc.collect()
                return (self, args)

        o = C()
        for n in (0, 1, 6, 100):
            t = tuple(range(n))
            with self.subTest(n=n):
                self.assertEqual(call_value(o.in_generator, *t), (o, t))
                self.assertEqual(call_value(o.with_locals, *t), (o, t))
                self.assertEqual(o.in_generator(*t), (o, t))
                self.assertEqual(o.with_locals(*t), (o, t))


class KeywordTests(unittest.TestCase):
    def test_python_function(self):
        def f(a, b):
            return (a, b)

        self.assertEqual(f(1, b=2), (1, 2))
        self.assertEqual(call_value(f, 1, **{'b': 2}), (1, 2))

    def test_bound_method(self):
        o = Recorder()
        self.assertEqual(o.m(1, b=2), (o, (1,), {'b': 2}))
        self.assertEqual(call_value(o.m, 1, **{'b': 2}), (o, (1,), {'b': 2}))

    def test_value_builtin(self):
        self.assertEqual(vc_args(1, b=2), (1, (1,), {'b': 2}))
        self.assertEqual(vc_args(), (0, (), None))
        self.assertEqual(vc_args(1, 2.5, 'x'), (3, (1, 2.5, 'x'), None))
        self.assertEqual(call_value(vc_args, 1, **{'b': 2}),
                         (1, (1,), {'b': 2}))


class ValueBuiltinTests(unittest.TestCase):
    def test_self(self):
        self.assertIs(vc_self(), vc_self.__self__)
        self.assertIs(vc_self(1, 2), vc_self.__self__)

    def test_method_defining_class(self):
        self.assertIs(vc_method(), VCallBase)
        self.assertIs(vc_method(1, a=2), VCallBase)

    def test_int_method_descriptor(self):
        d = make_int_method('vc_int_method')
        self.assertEqual(d(5, 2.5), ((5, 2.5), True))
        self.assertEqual(d.__get__(5)(2.5), ((5, 2.5), True))
        self.assertEqual(d(2**100), ((2**100,), False))

    def test_bad_flags(self):
        for name, flag in (('METH_O', METH_O),
                           ('METH_FASTCALL', METH_FASTCALL),
                           ('METH_VARARGS', METH_VARARGS),
                           ('METH_NOARGS', METH_NOARGS)):
            with self.subTest(flag=name):
                for exc in bad_flags(flag):
                    self.assertIsInstance(exc, SystemError)

    def test_arity_noargs(self):
        with self.assertRaises(TypeError) as ours:
            vc_noargs(1)
        with self.assertRaises(TypeError) as upstream:
            [].copy(1)
        self.assertEqual(vc_noargs(), None)
        self.assertEqual(self.tail(ours.exception),
                         self.tail(upstream.exception))

    def test_arity_one(self):
        with self.assertRaises(TypeError) as ours:
            vc_one()
        with self.assertRaises(TypeError) as upstream:
            len()
        self.assertEqual(vc_one(5), 5)
        self.assertEqual(self.tail(ours.exception),
                         self.tail(upstream.exception))
        with self.assertRaises(TypeError) as ours:
            vc_one(1, 2)
        with self.assertRaises(TypeError) as upstream:
            len(1, 2)
        self.assertEqual(self.tail(ours.exception),
                         self.tail(upstream.exception))

    @staticmethod
    def tail(exc):
        # The text after "name()".
        return str(exc).partition('() ')[2]


class NotCallableTests(unittest.TestCase):
    def test_int(self):
        x = 5
        with self.assertRaisesRegex(TypeError,
                                    "'int' object is not callable"):
            x()

    def test_float(self):
        x = 1.5
        with self.assertRaisesRegex(TypeError,
                                    "'float' object is not callable"):
            x()

    def test_from_c(self):
        with self.assertRaisesRegex(TypeError,
                                    "'int' object is not callable"):
            call_value(5)
        with self.assertRaisesRegex(TypeError,
                                    "'float' object is not callable"):
            call_one_arg(1.5, 1)


class InstanceInvariantTests(unittest.TestCase):
    def test_value_callables(self):
        def f():
            pass

        for name, obj in (('function', f),
                          ('METH_VALUE builtin', vc_args),
                          ('bound method', Recorder().m),
                          ('type', type)):
            with self.subTest(obj=name):
                self.assertTrue(value_vectorcall_is_set(obj))
                self.assertTrue(object_vectorcall_is_thunk(obj))

    def test_object_form_builtin(self):
        self.assertFalse(value_vectorcall_is_set(len))
        self.assertFalse(object_vectorcall_is_thunk(len))


class SpecTypeTests(unittest.TestCase):
    def test_value_field_is_called(self):
        o = VCall()
        self.assertTrue(value_vectorcall_is_set(o))
        self.assertTrue(object_vectorcall_is_thunk(o))
        self.assertEqual(o(), ('value-marker', 0))
        self.assertEqual(o(1, 2), ('value-marker', 2))
        self.assertEqual(call_value(o, 1), ('value-marker', 1))
        self.assertEqual(call_one_arg(o, 1), ('value-marker', 1))

    def test_subclass_inherits_offset(self):
        class Sub(VCallBase):
            pass

        o = Sub()
        self.assertTrue(value_vectorcall_is_set(o))
        self.assertEqual(o(1), ('value-marker', 1))
        self.assertEqual(call_value(o, 1, 2), ('value-marker', 2))

    def test_final_type_not_subclassable(self):
        with self.assertRaises(TypeError):
            class Sub(VCall):
                pass


if __name__ == '__main__':
    unittest.main()
