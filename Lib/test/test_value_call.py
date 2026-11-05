"""The value vectorcall (M3a).

Covers pyjit/design/proposals/value-api-m3.md, sections 1.1-1.6 and the
core cases of section 3. The helpers are in
Modules/_testinternalcapi/value_call.c.
"""

import enum
import functools
import gc
import sys
import pickle
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
meth_flags = _testinternalcapi.meth_flags
value_vectorcall_is_set = _testinternalcapi.value_vectorcall_is_set
object_vectorcall_is_thunk = _testinternalcapi.object_vectorcall_is_thunk
call_value = _testinternalcapi.call_value
call_one_arg = _testinternalcapi.call_one_arg
call_dict = _testinternalcapi.call_dict
vc_call_first = _testinternalcapi.vc_call_first
vectorcall_offsets = _testinternalcapi.vectorcall_offsets
VCall = _testinternalcapi.VCall
VCallBase = _testinternalcapi.VCallBase

METH_VALUE = 0x0400

# The METH_* flags that cannot be combined with METH_VALUE.
METH_VARARGS = 0x0001
METH_KEYWORDS = 0x0002
METH_NOARGS = 0x0004
METH_O = 0x0008
METH_FASTCALL = 0x0080
METH_METHOD = 0x0200

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

    def test_arity_message_names_the_defining_class(self):
        # A call through the method descriptor names the class that
        # defines the method.
        def call(obj):
            obj.__getstate__(1)

        for f in (call, lambda obj: object.__getstate__(obj, 1),
                  lambda obj: functools.partial(object.__getstate__, obj)(1)):
            with self.subTest(f=f):
                with self.assertRaisesRegex(
                        TypeError, r"^object\.__getstate__\(\) takes no "
                                   r"arguments \(1 given\)$"):
                    f(5)

    def test_arity_message_of_a_bound_method_names_type_of_self(self):
        # The bound builtin method names type(self).
        class IntSub(int):
            pass

        for obj in (5, IntSub(5), None, 1.5, ()):
            name = type(obj).__qualname__.replace(".", r"\.")
            with self.subTest(obj=obj):
                with self.assertRaisesRegex(
                        TypeError, rf"^{name}\.__getstate__\(\) takes no "
                                   r"arguments \(1 given\)$"):
                    obj.__getstate__.__call__(1)
        with self.assertRaisesRegex(
                TypeError, r"^int\.__format__\(\) takes exactly one "
                           r"argument \(0 given\)$"):
            getattr(5, "__format__")()
        with self.assertRaisesRegex(
                TypeError, r"^int\.__getstate__\(\) takes no "
                           r"keyword arguments$"):
            getattr(5, "__getstate__")(a=1)

    def test_typevar_arity_message(self):
        import typing
        T = typing.TypeVar("T")
        for f in (T.__typing_subst__,
                  lambda: typing.TypeVar.__typing_subst__(T)):
            with self.subTest(f=f):
                with self.assertRaisesRegex(
                        TypeError, r"^TypeVar\.__typing_subst__\(\) takes "
                                   r"exactly one argument"):
                    f()

    def test_sizeof_of_an_inline_int(self):
        # The size of the box, as int.__sizeof__() reports it.
        for n in (0, 5, -5, 2**30, 2**40):
            with self.subTest(n=n):
                self.assertEqual(object.__sizeof__(n), int.__sizeof__(n))


class TypeCallTests(unittest.TestCase):
    """The value vectorcall of int, bool, str, list, set, frozenset and range
    agrees with the tp_new path (type.__call__) on every shape of call."""

    SHAPES = [
        (int, (), {}), (int, (5,), {}), (int, (2**70,), {}),
        (int, (1.5,), {}), (int, ('7', 8), {}), (int, ('11',), {'base': 2}),
        (int, (), {'x': 1}), (int, (b'1', 10), {}),
        (int, ([],), {}),
        (bool, (), {}), (bool, (0,), {}), (bool, (2.5,), {}),
        (bool, ([],), {}), (bool, (1, 2), {}), (bool, (), {'x': 1}),
        (str, (), {}), (str, (5,), {}), (str, (1.5,), {}),
        (str, (b'ab', 'ascii'), {}), (str, (b'ab', 'ascii', 'strict'), {}),
        (str, (b'ab',), {'encoding': 'ascii'}), (str, (1, 'ascii'), {}),
        (str, (b'ab', 1), {}), (str, (b'ab', 'ascii', 1), {}),
        (str, (), {'object': 1}),
        (list, (), {}), (list, ((1, 2),), {}), (list, (5,), {}),
        (list, (), {'x': 1}), (list, ([], []), {}),
        (set, (), {}), (set, ([1, 2],), {}), (set, (5,), {}),
        (set, (), {'x': 1}), (set, ([[]],), {}),
        (frozenset, (), {}), (frozenset, ([1, 2],), {}), (frozenset, (5,), {}),
        (frozenset, (), {'x': 1}),
        (range, (), {}), (range, (5,), {}), (range, (1, 10, 2), {}),
        (range, (1.5,), {}), (range, (1, 2, 0), {}), (range, (1, 2, 3, 4), {}),
        (range, (2**70,), {}), (range, (), {'x': 1}), (range, ('a',), {}),
    ]

    def outcome(self, func, *args, **kwargs):
        try:
            result = func(*args, **kwargs)
        except Exception as exc:
            return type(exc), str(exc)
        return type(result), result

    def test_shapes(self):
        for cls, args, kwargs in self.SHAPES:
            with self.subTest(cls=cls.__name__, args=args, kwargs=kwargs):
                direct = self.outcome(cls, *args, **kwargs)
                slow = self.outcome(type.__call__, cls, *args, **kwargs)
                self.assertEqual(direct, slow)

    def test_too_many_arguments(self):
        # The vectorcall words these as upstream's does; tp_new differs.
        with self.assertRaisesRegex(
                TypeError, r"^int expected at most 2 arguments, got 3$"):
            int(1, 2, 3)
        with self.assertRaisesRegex(
                TypeError, r"^str expected at most 3 arguments, got 4$"):
            str(1, 2, 3, 4)

    def test_inline_arguments(self):
        self.assertIs(bool(5), True)
        self.assertIs(bool(0.0), False)
        self.assertEqual(int(), 0)
        self.assertEqual(int(7.9), 7)
        self.assertEqual(str(5), '5')
        self.assertEqual(list(range(3)), [0, 1, 2])
        self.assertEqual(range(3, 10, 2)[1], 5)

    def test_value_vectorcall_is_set(self):
        for cls in (int, bool, str, list, set, frozenset, range, float, tuple):
            with self.subTest(cls=cls.__name__):
                self.assertTrue(value_vectorcall_is_set(cls))


def function_str(func):
    """_PyObject_FunctionStr(): "math.gcd()", "list.append()"."""
    module = getattr(func, '__module__', None)
    name = func.__qualname__
    if module is not None and module != 'builtins':
        name = f'{module}.{name}'
    return f'{name}()'


class KeywordMessageTests(unittest.TestCase):
    """METH_VALUE takes positional arguments only, as METH_FASTCALL does: the
    vectorcall rejects keywords with the message of the callable it was
    given, and METH_VALUE | METH_KEYWORDS callees receive them."""

    def check_rejects(self, func, *args):
        expected = f'{function_str(func)} takes no keyword arguments'
        with self.assertRaises(TypeError) as cm:
            func(*args, bogus=1)
        self.assertEqual(str(cm.exception), expected)

    def test_flags(self):
        self.assertEqual(meth_flags(vc_noargs), METH_VALUE)
        self.assertEqual(meth_flags(vc_args), METH_VALUE | METH_KEYWORDS)
        self.assertEqual(meth_flags(vc_call_first), METH_VALUE | METH_KEYWORDS)
        self.assertEqual(meth_flags([].append), METH_VALUE | METH_O)
        self.assertEqual(meth_flags(list.sort), METH_VALUE | METH_KEYWORDS)

    def test_function(self):
        self.assertEqual(vc_args(1, a=2), (1, (1,), {'a': 2}))
        for func in (vc_noargs, vc_one, vc_self):
            with self.subTest(func=func):
                self.check_rejects(func)
        self.check_rejects(vc_one, 1)

    def test_reviewer_reproducers(self):
        import _io
        import _struct
        import math
        import struct
        funcs = [math.gcd, math.lcm, math.comb, math.perm,
                 struct.pack, struct.pack_into, struct.unpack,
                 struct.iter_unpack, _struct.pack]
        for func in funcs:
            with self.subTest(func=func):
                self.check_rejects(func)
        self.assertEqual(function_str(math.gcd), 'math.integer.gcd()')
        self.assertEqual(function_str(struct.pack), '_struct.pack()')
        # A bound method names type(self), a descriptor its defining class.
        for cls in (_io.StringIO, _io.BytesIO, _io._RawIOBase,
                    _io._BufferedIOBase, _io._TextIOBase):
            for name in ('readlines', '__exit__', 'readline'):
                if cls is _io._TextIOBase and name == 'readline':
                    continue  # defined by _TextIOBase itself, not METH_VALUE
                with self.subTest(cls=cls, name=name):
                    self.check_rejects(getattr(cls(), name))
                    self.check_rejects(getattr(_io._IOBase, name), cls())
        with self.assertRaisesRegex(
                TypeError, r"^StringIO\.readlines\(\) takes no keyword "
                           r"arguments$"):
            _io.StringIO().readlines.__call__(bogus=1)
        with self.assertRaisesRegex(
                TypeError, r"^_IOBase\.readlines\(\) takes no keyword "
                           r"arguments$"):
            _io.StringIO().readlines(bogus=1)

    def positional_only(self, func):
        flags = meth_flags(func)
        return flags & ~METH_O & ~METH_NOARGS == METH_VALUE

    def test_stdlib_sweep(self):
        # Every positional-only METH_VALUE callable of these modules and
        # types names itself in the message.
        import _io
        import _struct
        import array
        import collections
        import itertools
        import math
        import operator
        import cmath
        import _bisect
        import _heapq
        import _functools
        modules = [math, cmath, _struct, operator, itertools, array, _bisect,
                   _heapq, _functools, _io]
        instances = {
            str: '', bytes: b'', bytearray: bytearray(), list: [], dict: {},
            set: set(), frozenset: frozenset(), tuple: (), int: 0,
            float: 0.5, complex: 1j, range: range(3), memoryview: memoryview(b''),
            _io.StringIO: _io.StringIO(), _io.BytesIO: _io.BytesIO(),
            _io._IOBase: _io._IOBase(), _io._RawIOBase: _io._RawIOBase(),
            _io._BufferedIOBase: _io._BufferedIOBase(),
            _io._TextIOBase: _io._TextIOBase(),
            _struct.Struct: _struct.Struct('i'),
            array.array: array.array('i'),
            collections.deque: collections.deque(),
        }
        checked = 0
        for module in modules:
            for name, func in vars(module).items():
                if not callable(func) or isinstance(func, type):
                    continue
                if type(func) is not type(len) or not self.positional_only(func):
                    continue
                with self.subTest(func=func):
                    self.check_rejects(func)
                checked += 1
        for cls, obj in instances.items():
            for name, descr in vars(cls).items():
                if type(descr) is not type(str.lower):
                    continue
                if not self.positional_only(descr):
                    continue
                with self.subTest(descr=descr):
                    self.check_rejects(descr, obj)
                    self.check_rejects(getattr(obj, name))
                checked += 1
        self.assertGreater(checked, 300)


class BuiltinValueCallTests(unittest.TestCase):
    """min, max and sum read and return inline ints and floats as values."""

    def test_min_max(self):
        self.assertEqual(min(3, 1, 2), 1)
        self.assertEqual(max(3, 1, 2), 3)
        self.assertEqual(min([4, 2, 9]), 2)
        self.assertEqual(max(1.5, 2), 2)
        self.assertEqual(max([], default=7), 7)
        self.assertEqual(min((), default=None), None)
        self.assertEqual(max(3, -5, key=abs), -5)
        self.assertEqual(min([3, -5], key=None), -5)
        self.assertEqual(min(2**70, 5), 5)
        self.assertIs(max(1, 1.0), 1)
        self.assertIs(min(1.0, 1), 1.0)

    def test_min_max_errors(self):
        for func in (min, max):
            name = func.__name__
            with self.subTest(func=name):
                with self.assertRaisesRegex(
                        TypeError, rf"^{name} expected at least 1 argument, "
                                   r"got 0$"):
                    func()
                with self.assertRaisesRegex(
                        TypeError, rf"^{name}\(\) got an unexpected keyword "
                                   r"argument 'foo'$"):
                    func(1, 2, foo=3)
                with self.assertRaisesRegex(
                        TypeError, rf"^Cannot specify a default for {name}"
                                   r"\(\) with multiple positional arguments$"):
                    func(1, 2, default=3)
                with self.assertRaisesRegex(
                        ValueError, rf"^{name}\(\) iterable argument is empty$"):
                    func([])
                with self.assertRaisesRegex(
                        TypeError, r"^'int' object is not iterable$"):
                    func(5)
                with self.assertRaisesRegex(
                        TypeError, r"^'int' object is not callable$"):
                    func(1, 2, key=1)

    def test_sum(self):
        self.assertEqual(sum([]), 0)
        self.assertEqual(sum([1, 2, 3]), 6)
        self.assertEqual(sum((1.5, 2, 3)), 6.5)
        self.assertEqual(sum([0.1] * 10), 1.0)
        self.assertEqual(sum([1, 2.5]), 3.5)
        self.assertEqual(sum([1j, 2, 3.5]), (5.5 + 1j))
        self.assertEqual(sum([2**62, 2**62, 2**62]), 3 * 2**62)
        self.assertEqual(sum([2**70, 1, 2]), 2**70 + 3)
        self.assertEqual(sum([1, 2], 10), 13)
        self.assertEqual(sum([1, 2], 0.5), 3.5)
        self.assertEqual(sum([[1], [2]], []), [1, 2])
        self.assertEqual(sum([True, True, 1]), 3)
        self.assertEqual(sum([1, 2], True), 4)
        self.assertEqual(sum(range(10), start=5), 50)
        with self.assertRaisesRegex(TypeError, "can't sum strings"):
            sum(['a'], '')
        with self.assertRaisesRegex(TypeError, "unsupported operand"):
            sum([1, 'a'])
        with self.assertRaises(OverflowError):
            sum([1.5, 10**400])


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
        for name, flag in (('METH_FASTCALL', METH_FASTCALL),
                           ('METH_VARARGS', METH_VARARGS),
                           ('METH_NOARGS | METH_O', METH_NOARGS | METH_O),
                           ('METH_NOARGS | METH_METHOD',
                            METH_NOARGS | METH_METHOD),
                           ('METH_O | METH_METHOD', METH_O | METH_METHOD),
                           ('METH_METHOD without METH_KEYWORDS', METH_METHOD),
                           ('METH_NOARGS | METH_KEYWORDS',
                            METH_NOARGS | METH_KEYWORDS),
                           ('METH_O | METH_KEYWORDS',
                            METH_O | METH_KEYWORDS)):
            with self.subTest(flag=name):
                for exc in bad_flags(flag):
                    self.assertIsInstance(exc, SystemError)

    def test_keyword_flags(self):
        self.assertEqual(bad_flags(METH_KEYWORDS), (None, None))

    def test_fixed_arity_flags(self):
        for name, flag in (('METH_O', METH_O), ('METH_NOARGS', METH_NOARGS)):
            with self.subTest(flag=name):
                self.assertEqual(bad_flags(flag), (None, None))

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
        # A function of a compatibility test module stays object-form.
        _testcapi = import_helper.import_module('_testcapi')
        func = _testcapi.get_args
        self.assertFalse(value_vectorcall_is_set(func))
        self.assertFalse(object_vectorcall_is_thunk(func))


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


class RangeFieldTests(unittest.TestCase):
    # start, stop, step and length are value fields: inline when they fit.
    BIG = 1 << 80

    def test_fields_and_repr(self):
        for args in [(5,), (2, 9), (2, 9, 3), (-3, -20, -4),
                     (self.BIG, self.BIG + 10, 3), (-self.BIG, 0, self.BIG),
                     (1 << 51, (1 << 51) + 5), ((1 << 50) - 2, (1 << 50) + 2)]:
            with self.subTest(args=args):
                r = range(*args)
                self.assertEqual((r.start, r.stop, r.step),
                                 (args[0] if len(args) > 1 else 0, args[-1] if len(args) < 3 else args[1],
                                  args[2] if len(args) == 3 else 1))
                self.assertEqual(eval(repr(r)), r)
                self.assertEqual(list(r), [r[i] for i in range(len(r))])
                self.assertEqual(list(reversed(r)), list(r)[::-1])
                self.assertEqual(len(r), len(list(r)))
                for proto in range(pickle.HIGHEST_PROTOCOL + 1):
                    self.assertEqual(pickle.loads(pickle.dumps(r, proto)), r)
                    it = iter(r)
                    next(it, None)
                    self.assertEqual(list(pickle.loads(pickle.dumps(it, proto))),
                                     list(r)[1:])

    def test_index_slice_contains(self):
        r = range(self.BIG, self.BIG + 30, 3)
        self.assertEqual(r[2], self.BIG + 6)
        self.assertEqual(r[-1], self.BIG + 27)
        self.assertEqual(r[1:4], range(self.BIG + 3, self.BIG + 12, 3))
        self.assertEqual(r[::-2], range(self.BIG + 27, self.BIG - 3, -6))
        self.assertIn(self.BIG + 9, r)
        self.assertNotIn(self.BIG + 10, r)
        self.assertEqual(r.index(self.BIG + 9), 3)
        self.assertEqual(r.count(self.BIG + 9), 1)
        s = range(10, 0, -3)
        self.assertEqual(s[1:], range(7, -2, -3))
        self.assertIn(4, s)
        self.assertNotIn(5, s)
        self.assertRaises(IndexError, s.__getitem__, 4)
        self.assertRaises(TypeError, s.__getitem__, 1.5)

    def test_hash_eq(self):
        self.assertEqual(hash(range(0, 10, 2)), hash(range(0, 9, 2)))
        self.assertEqual(range(self.BIG, self.BIG), range(0))
        self.assertNotEqual(range(self.BIG), range(self.BIG + 1))
        self.assertEqual(hash(range(self.BIG)), hash(range(0, self.BIG, 1)))


class UnpackIteratorTests(unittest.TestCase):
    # UNPACK_SEQUENCE and UNPACK_EX over an iterator take its items as words.
    def test_unpack(self):
        a, b, c = range(3)
        self.assertEqual((a, b, c), (0, 1, 2))
        a, *b, c = iter(range(1 << 60, (1 << 60) + 5))
        self.assertEqual((a, b, c), (1 << 60, list(range((1 << 60) + 1, (1 << 60) + 4)), (1 << 60) + 4))

    def test_errors(self):
        with self.assertRaisesRegex(ValueError, r"not enough values to unpack \(expected 3, got 2\)"):
            a, b, c = range(2)
        with self.assertRaisesRegex(ValueError, r"too many values to unpack \(expected 3\)"):
            a, b, c = range(4)

        def gen():
            yield 1
            raise KeyError('x')
        with self.assertRaises(KeyError):
            a, b = gen()


if __name__ == '__main__':
    unittest.main()
