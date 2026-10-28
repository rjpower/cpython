"""Tests for the PyValue protocols (Include/cpython/abstract.h).

The wrappers in Modules/_testcapi/pyvalue.c pass the canonical value of
each argument: an int that fits in int32 is an inline int, a float other
than NaN an inline float, and anything else an object.  A value result
comes back as (result, kind), where kind says how the value was held; the
wrapper raises AssertionError if the result is not canonical.
"""

import collections
import fractions
import math
import operator
import sys
import unittest
import warnings

from test import support
from test.support import import_helper
from test.support.script_helper import assert_python_failure

_testcapi = import_helper.import_module('_testcapi')
_testlimitedcapi = import_helper.import_module('_testlimitedcapi')
pv = _testcapi.pyvalue

INT32_MAX = 2**31 - 1
INT32_MIN = -2**31
BIG = 2**40


def kind(x):
    """The kind of the canonical value of x."""
    if type(x) is int and INT32_MIN <= x <= INT32_MAX:
        return 'int'
    if type(x) is float and not math.isnan(x):
        return 'float'
    return 'object'


class Boom(Exception):
    pass


class Raising:
    """Every operation raises Boom."""
    def _raise(self, *args):
        raise Boom
    __add__ = __radd__ = __sub__ = __rsub__ = __mul__ = __rmul__ = _raise
    __matmul__ = __rmatmul__ = __floordiv__ = __rfloordiv__ = _raise
    __truediv__ = __rtruediv__ = __mod__ = __rmod__ = __pow__ = _raise
    __rpow__ = __lshift__ = __rlshift__ = __rshift__ = __rrshift__ = _raise
    __and__ = __rand__ = __or__ = __ror__ = __xor__ = __rxor__ = _raise
    __neg__ = __pos__ = __invert__ = __abs__ = __index__ = _raise
    __int__ = __float__ = __bool__ = __hash__ = __str__ = __repr__ = _raise
    __format__ = __iter__ = __next__ = __getitem__ = __setitem__ = _raise
    __delitem__ = __contains__ = __lt__ = __eq__ = _raise


# Operands: inline ints, inline floats and objects, and pairs of them that
# overflow int32, divide by zero, or mix types.
OPERANDS = [
    (7, 3), (-7, 3), (7, -3), (0, 5), (5, 0),
    (INT32_MAX, 1), (INT32_MIN, -1), (INT32_MAX, INT32_MAX), (1, 40),
    (7.5, 2.0), (-7.5, 2.0), (7.5, 0.0), (0.0, -0.0), (1e308, 10.0),
    (7, 2.5), (2.5, 7), (3, -2), (-8, 0.5),
    (BIG, 3), (3, BIG), (BIG, BIG), (BIG, 2.5),
    (True, 2), (fractions.Fraction(1, 3), 2),
    ('ab', 3), (3, 'ab'), ('ab', 'cd'), ('ab', 2.5),
    ([1], [2]), ((1,), 2), (None, 1),
]

BINARY = [
    ('Add', operator.add),
    ('Subtract', operator.sub),
    ('Multiply', operator.mul),
    ('MatrixMultiply', operator.matmul),
    ('FloorDivide', operator.floordiv),
    ('TrueDivide', operator.truediv),
    ('Remainder', operator.mod),
    ('Power', operator.pow),
    ('Lshift', operator.lshift),
    ('Rshift', operator.rshift),
    ('And', operator.and_),
    ('Or', operator.or_),
    ('Xor', operator.xor),
]

def huge(name, a, b):
    """Is the result of name(a, b) too large to compute?"""
    return (name.removeprefix('InPlace') in ('Power', 'Lshift')
            and isinstance(a, int) and isinstance(b, int) and b > 1000)

INPLACE = [
    ('InPlaceAdd', operator.iadd),
    ('InPlaceSubtract', operator.isub),
    ('InPlaceMultiply', operator.imul),
    ('InPlaceMatrixMultiply', operator.imatmul),
    ('InPlaceFloorDivide', operator.ifloordiv),
    ('InPlaceTrueDivide', operator.itruediv),
    ('InPlaceRemainder', operator.imod),
    ('InPlacePower', operator.ipow),
    ('InPlaceLshift', operator.ilshift),
    ('InPlaceRshift', operator.irshift),
    ('InPlaceAnd', operator.iand),
    ('InPlaceOr', operator.ior),
    ('InPlaceXor', operator.ixor),
]

UNARY_OPERANDS = [
    0, 5, -5, INT32_MAX, INT32_MIN, 2.5, -2.5, 0.0, -0.0, 1e300,
    math.inf, -math.inf, BIG, -BIG, True, fractions.Fraction(1, 3),
    '5', None, [],
]

UNARY = [
    ('NegativeValue', operator.neg),
    ('PositiveValue', operator.pos),
    ('InvertValue', operator.invert),
    ('AbsoluteValue', abs),
    ('IndexValue', operator.index),
    ('LongValue', int),
    ('FloatValue', float),
]


class ValueTestCase(unittest.TestCase):

    def assertValue(self, got, expected):
        """got is (result, kind) from a wrapper; expected is the result."""
        result, got_kind = got
        self.assertIs(type(result), type(expected))
        if isinstance(expected, float) and math.isnan(expected):
            self.assertTrue(math.isnan(result))
        else:
            self.assertEqual(result, expected)
        self.assertEqual(got_kind, kind(expected))

    def assertSameOutcome(self, func, expected_func, *args):
        """func(*args) returns what expected_func(*args) does, or raises
        the same exception type."""
        try:
            expected = expected_func(*args)
        except Exception as exc:
            with self.assertRaises(type(exc)):
                func(*args)
        else:
            self.assertValue(func(*args), expected)


class ArgumentKindTests(unittest.TestCase):

    def test_value_kind(self):
        # The wrappers pass inline values for these operands.
        for x in (0, 7, -7, INT32_MAX, INT32_MIN, 2.5, -0.0, math.inf,
                  BIG, INT32_MAX + 1, math.nan, True, 'a', None):
            with self.subTest(x=x):
                self.assertEqual(pv.value_kind(x), kind(x))


class NumberTests(ValueTestCase):

    def test_binary(self):
        for name, op in BINARY:
            func = getattr(pv, name + 'Value')
            for a, b in OPERANDS:
                if huge(name, a, b):
                    continue
                with self.subTest(name=name, a=a, b=b):
                    self.assertSameOutcome(func, op, a, b)

    def test_inplace(self):
        for name, op in INPLACE:
            func = getattr(pv, name + 'Value')
            for a, b in OPERANDS:
                if isinstance(a, list) or huge(name, a, b):
                    continue
                with self.subTest(name=name, a=a, b=b):
                    self.assertSameOutcome(func, op, a, b)

    def test_inplace_mutates(self):
        lst = [1]
        result, k = pv.InPlaceAddValue(lst, [2])
        self.assertIs(result, lst)
        self.assertEqual(lst, [1, 2])
        self.assertEqual(k, 'object')

    def test_power_modulus(self):
        for func in (pv.PowerValue, pv.InPlacePowerValue):
            with self.subTest(func=func.__name__):
                self.assertValue(func(3, 4, 5), 1)
                self.assertValue(func(3, 4, None), 81)
                self.assertValue(func(2, -1), 0.5)
                self.assertValue(func(BIG, 2, 7), pow(BIG, 2, 7))
                self.assertValue(func(-8, 1/3), (-8) ** (1/3))
                with self.assertRaises(TypeError):
                    func(2.5, 2, 3)
                with self.assertRaises(ValueError):
                    func(3, 4, 0)
                with self.assertRaises(ZeroDivisionError):
                    func(0, -1)

    def test_errors_propagate(self):
        for name, _ in BINARY + INPLACE:
            func = getattr(pv, name + 'Value')
            for a, b in ((Raising(), 5), (2.5, Raising())):
                with self.subTest(name=name, a=a, b=b):
                    with self.assertRaises(Boom):
                        func(a, b)

    def test_unary(self):
        for name, op in UNARY:
            func = getattr(pv, name)
            for x in UNARY_OPERANDS:
                with self.subTest(name=name, x=x):
                    self.assertSameOutcome(func, op, x)
            with self.subTest(name=name, x='Raising'):
                with self.assertRaises(Boom):
                    func(Raising())

    def test_canonical_results(self):
        # Results that fit are inline, whatever the operands were.
        self.assertValue(pv.SubtractValue(BIG + 5, BIG), 5)
        self.assertValue(pv.NegativeValue(INT32_MIN), -INT32_MIN)
        self.assertValue(pv.AddValue(INT32_MAX, 1), INT32_MAX + 1)
        self.assertValue(pv.LongValue(2.75), 2)
        self.assertValue(pv.LongValue(1e10), 10**10)
        self.assertValue(pv.FloatValue(BIG), float(BIG))
        self.assertValue(pv.IndexValue(True), 1)


class ObjectTests(ValueTestCase):

    def test_getattr(self):
        cases = [(5, 'real'), (5, 'imag'), (2.5, 'real'), (2.5, 'imag'),
                 (BIG, 'numerator'), ('ab', '__len__'), (5, 'missing'),
                 (2.5, 'missing'), (None, 'missing')]
        for o, name in cases:
            with self.subTest(o=o, name=name):
                self.assertSameOutcome(pv.GetAttrValue, getattr, o, name)
        result, k = pv.GetAttrValue(5, 'bit_length')
        self.assertEqual(result(), 3)
        self.assertEqual(k, 'object')
        with self.assertRaises(Boom):
            pv.GetAttrValue(type('C', (), {'__getattr__': Raising._raise})(),
                            'x')

    def test_getoptionalattr(self):
        class C:
            x = 12345
        class Missing:
            def __getattr__(self, name):
                raise AttributeError(name)
        class Raises:
            def __getattr__(self, name):
                raise Boom
        cases = [(5, 'real', 5), (2.5, 'imag', 0.0), (C(), 'x', 12345),
                 (C, 'x', 12345), (math, 'pi', math.pi),
                 (BIG, 'numerator', BIG)]
        for o, name, expected in cases:
            with self.subTest(o=o, name=name):
                found, result, k = pv.GetOptionalAttrValue(o, name)
                self.assertEqual(found, 1)
                self.assertValue((result, k), expected)
        for o in (5, 2.5, C(), C, math, Missing()):
            with self.subTest(o=o):
                self.assertEqual(pv.GetOptionalAttrValue(o, 'missing'),
                                 (0, None, None))
        with self.assertRaises(Boom):
            pv.GetOptionalAttrValue(Raises(), 'x')
        with self.assertRaises(TypeError):
            pv.GetOptionalAttrValue(C(), 5)

    def test_setattr(self):
        class C:
            pass
        class S:
            __slots__ = ('x',)
        for cls in (C, S):
            obj = cls()
            for v in (5, INT32_MIN, 2.5, -0.0, BIG, 'a'):
                with self.subTest(cls=cls.__name__, v=v):
                    self.assertEqual(pv.SetAttrValue(obj, 'x', v), 0)
                    self.assertValue(pv.GetAttrValue(obj, 'x'), v)
                    self.assertIs(type(obj.x), type(v))
                    self.assertEqual(obj.x, v)
            # None stands for NULL here: delete.
            self.assertEqual(pv.SetAttrValue(obj, 'x', None), 0)
            self.assertFalse(hasattr(obj, 'x'))
            with self.assertRaises(AttributeError):
                pv.SetAttrValue(obj, 'x', None)
        for owner in (5, 2.5, BIG, 'a'):
            with self.subTest(owner=owner):
                with self.assertRaises(AttributeError):
                    pv.SetAttrValue(owner, 'x', 1)

    def test_richcompare(self):
        ops = [operator.lt, operator.le, operator.eq, operator.ne,
               operator.gt, operator.ge]
        operands = [(1, 2), (2, 2), (2.5, 2), (2, 2.5), (-0.0, 0.0),
                    (0, -0.0), (BIG, 5), (5, BIG), ('a', 'b'), (1, 'a'),
                    (math.nan, math.nan), (math.inf, INT32_MAX)]
        for opid, op in enumerate(ops):
            for a, b in operands:
                with self.subTest(op=op.__name__, a=a, b=b):
                    self.assertSameOutcome(
                        lambda a, b: pv.RichCompareValue(a, b, opid), op, a, b)
                    try:
                        expected = bool(op(a, b))
                    except TypeError:
                        with self.assertRaises(TypeError):
                            pv.RichCompareBoolValue(a, b, opid)
                    else:
                        if not (a is b and opid in (2, 3)):
                            self.assertEqual(
                                pv.RichCompareBoolValue(a, b, opid), expected)
        with self.assertRaises(Boom):
            pv.RichCompareValue(Raising(), 5, 0)
        with self.assertRaises(Boom):
            pv.RichCompareBoolValue(2.5, Raising(), 2)

    def test_richcomparebool_identity(self):
        # As PyObject_RichCompareBool(): identical operands are equal.
        nan = math.nan
        self.assertEqual(pv.RichCompareBoolValue(nan, nan, 2), 1)
        self.assertEqual(pv.RichCompareBoolValue(nan, nan, 3), 0)
        self.assertEqual(pv.RichCompareBoolValue(5, 5, 2), 1)

    def test_istrue(self):
        for x in (0, 5, -1, 0.0, -0.0, 2.5, BIG, '', 'a', [], [0], None):
            with self.subTest(x=x):
                self.assertEqual(pv.IsTrueValue(x), int(bool(x)))
        with self.assertRaises(Boom):
            pv.IsTrueValue(Raising())

    def test_hash(self):
        for x in (0, 5, -1, -2, INT32_MAX, INT32_MIN, 2.5, -0.0, 1e300,
                  math.inf, BIG, 'a', None, (1, 2.5)):
            with self.subTest(x=x):
                self.assertEqual(pv.HashValue(x), hash(x))
        with self.assertRaises(TypeError):
            pv.HashValue([])
        with self.assertRaises(Boom):
            pv.HashValue(Raising())


class ItemTests(ValueTestCase):

    def test_getitem(self):
        lst = [10, 2.5, 'a', BIG]
        tup = tuple(lst)
        d = {1: 'one', 2.5: 'two and a half', BIG: 'big', 'k': 7}
        cases = [(lst, 0), (lst, 1), (lst, -1), (lst, 3), (lst, 4),
                 (lst, BIG), (lst, 1.0), (lst, slice(1, 3)),
                 (tup, 0), (tup, -2), (tup, 9), ('abc', 1), ('abc', -1),
                 ('abc', 5), (d, 1), (d, 1.0), (d, True), (d, 2.5), (d, BIG),
                 (d, 'k'), (d, 3), (d, []), (range(10), 3),
                 (5, 0), (2.5, 0)]
        for o, key in cases:
            with self.subTest(o=type(o).__name__, key=key):
                self.assertSameOutcome(pv.GetItemValue, operator.getitem,
                                       o, key)
        dd = collections.defaultdict(int)
        self.assertValue(pv.GetItemValue(dd, 5), 0)
        self.assertIn(5, dd)
        with self.assertRaises(Boom):
            pv.GetItemValue(Raising(), 5)

    def test_setitem(self):
        lst = [0, 0, 0]
        d = {}
        for o, key in ((lst, 0), (lst, -1), (d, 1), (d, 2.5), (d, BIG)):
            for v in (5, 2.5, BIG, 'a'):
                with self.subTest(o=type(o).__name__, key=key, v=v):
                    self.assertEqual(pv.SetItemValue(o, key, v), 0)
                    self.assertValue(pv.GetItemValue(o, key), v)
                    self.assertIs(type(o[key]), type(v))
        self.assertEqual(list(d), [1, 2.5, BIG])
        with self.assertRaises(IndexError):
            pv.SetItemValue(lst, 3, 1)
        with self.assertRaises(TypeError):
            pv.SetItemValue((1,), 0, 1)
        with self.assertRaises(TypeError):
            pv.SetItemValue(5, 0, 1)
        with self.assertRaises(TypeError):
            pv.SetItemValue(d, [], 1)
        with self.assertRaises(Boom):
            pv.SetItemValue(Raising(), 1, 2)

    def test_delitem(self):
        lst = [1, 2, 3]
        self.assertEqual(pv.DelItemValue(lst, 1), 0)
        self.assertEqual(lst, [1, 3])
        self.assertEqual(pv.DelItemValue(lst, -1), 0)
        self.assertEqual(lst, [1])
        d = {1: 'a', 2.5: 'b', BIG: 'c'}
        for key in (1.0, 2.5, BIG):
            self.assertEqual(pv.DelItemValue(d, key), 0)
        self.assertEqual(d, {})
        with self.assertRaises(IndexError):
            pv.DelItemValue(lst, 5)
        with self.assertRaises(KeyError):
            pv.DelItemValue(d, 1)
        with self.assertRaises(TypeError):
            pv.DelItemValue(5, 0)
        with self.assertRaises(Boom):
            pv.DelItemValue(Raising(), 1)

    def test_contains(self):
        containers = [[1, 2.5, BIG, 'a'], (1, 2.5, BIG, 'a'),
                      {1, 2.5, BIG, 'a'}, {1: 0, 2.5: 0, BIG: 0, 'a': 0},
                      frozenset({1, 2.5}), range(5), 'abc']
        for c in containers:
            for v in (1, 1.0, True, 2.5, 3, BIG, 'a', 'b'):
                with self.subTest(c=type(c).__name__, v=v):
                    try:
                        expected = v in c
                    except TypeError:
                        with self.assertRaises(TypeError):
                            pv.ContainsValue(c, v)
                    else:
                        self.assertEqual(pv.ContainsValue(c, v), expected)
        for c in (5, 2.5):
            with self.assertRaises(TypeError):
                pv.ContainsValue(c, 1)
        with self.assertRaises(TypeError):
            pv.ContainsValue({1}, [])
        with self.assertRaises(Boom):
            pv.ContainsValue(Raising(), 1)


class IterationTests(ValueTestCase):

    def test_getiter(self):
        it, k = pv.GetIterValue([1, 2])
        self.assertEqual(k, 'object')
        self.assertEqual(list(it), [1, 2])
        for o in (5, 2.5, None):
            with self.assertRaises(TypeError):
                pv.GetIterValue(o)
        with self.assertRaises(Boom):
            pv.GetIterValue(Raising())

    def test_next(self):
        def gen():
            yield 2.5
            yield BIG
        items = [5, 2.5, BIG, 'a', INT32_MIN]
        for it in (iter(items), iter(tuple(items)), iter(range(3)), gen(),
                   iter('ab'), iter(b'ab'), iter({1: 2, 2.5: 3})):
            with self.subTest(it=type(it).__name__):
                got = []
                while True:
                    status, pair = pv.NextValue(it)
                    if status == 0:
                        self.assertIsNone(pair)
                        break
                    self.assertEqual(status, 1)
                    self.assertEqual(pair[1], kind(pair[0]))
                    got.append(pair[0])
                self.assertEqual(pv.NextValue(it), (0, None))
                self.assertTrue(got)
        it = iter(items)
        self.assertEqual([pv.NextValue(it)[1][0] for _ in items], items)

    def test_next_errors(self):
        def gen():
            yield 1
            raise Boom
        it = gen()
        self.assertEqual(pv.NextValue(it), (1, (1, 'int')))
        with self.assertRaises(Boom):
            pv.NextValue(it)
        with self.assertRaises(TypeError):
            pv.NextValue([1])
        with self.assertRaises(Boom):
            pv.NextValue(Raising())


class StringTests(ValueTestCase):

    def test_str_repr_ascii(self):
        for name, func in (('StrValue', str), ('ReprValue', repr),
                           ('ASCIIValue', ascii)):
            f = getattr(pv, name)
            for x in (0, 5, -5, INT32_MAX, INT32_MIN, 2.5, -0.0, 1e100,
                      1e-7, math.inf, -math.inf, math.nan, BIG, 'é', None):
                with self.subTest(name=name, x=x):
                    self.assertValue(f(x), func(x))
            with self.assertRaises(Boom):
                f(Raising())

    def test_format(self):
        cases = [(5, None), (5, ''), (5, '05d'), (5, 'x'), (-5, '+'),
                 (INT32_MIN, ','), (2.5, None), (2.5, ''), (2.5, '.3f'),
                 (-0.0, 'g'), (1e100, 'e'), (2.5, '>10'), (math.inf, '%'),
                 (BIG, '_'), ('x', '>3'), (5, 'z'), (2.5, 'd'),
                 (fractions.Fraction(1, 3), '.2f')]
        for x, spec in cases:
            with self.subTest(x=x, spec=spec):
                self.assertSameOutcome(
                    pv.FormatValue,
                    lambda x, spec: format(x, '' if spec is None else spec),
                    x, spec)
        with self.assertRaises(Boom):
            pv.FormatValue(Raising(), 'x')



class Index:
    def __init__(self, i):
        self.i = i

    def __index__(self):
        return self.i


class ConversionTests(unittest.TestCase):
    """The C conversions read an inline int or float without a box and
    otherwise behave as the PyObject * functions."""

    def test_integer(self):
        for name in ('AsLongValue', 'AsIntValue', 'AsLongLongValue',
                     'AsSsize_tValue'):
            func = getattr(pv, name)
            with self.subTest(name=name):
                for x in (0, 7, -7, INT32_MAX, INT32_MIN, True):
                    self.assertEqual(func(x), x)
                self.assertRaises(TypeError, func, 2.5)
                self.assertRaises(TypeError, func, math.nan)
                self.assertRaises(TypeError, func, 'a')
                if name != 'AsSsize_tValue':
                    # PyLong_AsSsize_t() does not call __index__.
                    self.assertEqual(func(Index(5)), 5)
        self.assertEqual(pv.AsLongLongValue(BIG), BIG)
        self.assertRaises(OverflowError, pv.AsIntValue, INT32_MAX + 1)
        self.assertRaises(OverflowError, pv.AsLongValue, 2**100)

    def test_number_as_ssize_t(self):
        f = pv.NumberAsSsize_tValue
        self.assertEqual(f(7, OverflowError), 7)
        self.assertEqual(f(INT32_MIN, None), INT32_MIN)
        self.assertEqual(f(Index(BIG), None), BIG)
        self.assertEqual(f(2**100, None), sys.maxsize)
        self.assertEqual(f(-2**100, None), -sys.maxsize - 1)
        self.assertRaises(IndexError, f, 2**100, IndexError)
        self.assertRaises(TypeError, f, 2.5, None)

    def test_double(self):
        f = pv.AsDoubleValue
        for x in (2.5, -0.0, math.inf, 0, 7, INT32_MIN, BIG, 2**100,
                  fractions.Fraction(1, 4)):
            with self.subTest(x=x):
                self.assertEqual(f(x), float(x))
        self.assertTrue(math.isnan(f(math.nan)))
        self.assertTrue(math.copysign(1, f(-0.0)) < 0)
        self.assertRaises(TypeError, f, 'a')
        self.assertRaises(OverflowError, f, 10**400)


class MemberTests(ValueTestCase):
    """PyMember_GetOneValue() and PyMember_SetOneValue(): numbers read
    inline where they fit and store without a box, with the checks and
    warnings of PyMember_GetOne() and PyMember_SetOne()."""

    def setUp(self):
        self.ts = _testcapi._test_structmembersType_NewAPI()

    def roundtrip(self, o, name, value, expected=None):
        expected = value if expected is None else expected
        self.assertEqual(pv.MemberSetOneValue(o, name, value), 0)
        self.assertValue(pv.MemberGetOneValue(o, name), expected)
        self.assertEqual(getattr(o, name), expected)

    def test_integers(self):
        for name in ('T_BYTE', 'T_UBYTE', 'T_SHORT', 'T_USHORT', 'T_INT',
                     'T_UINT', 'T_LONG', 'T_ULONG', 'T_PYSSIZET',
                     'T_LONGLONG', 'T_ULONGLONG'):
            with self.subTest(name=name):
                self.roundtrip(self.ts, name, 0)
                self.roundtrip(self.ts, name, 100)
                self.roundtrip(self.ts, name, True, 1)
        for name in ('T_INT', 'T_LONG', 'T_PYSSIZET', 'T_LONGLONG'):
            with self.subTest(name=name):
                self.roundtrip(self.ts, name, INT32_MAX)
                self.roundtrip(self.ts, name, INT32_MIN)
        for name in ('T_UINT', 'T_ULONG', 'T_ULONGLONG'):
            with self.subTest(name=name):
                self.roundtrip(self.ts, name, INT32_MAX + 1)
                self.roundtrip(self.ts, name, Index(7), 7)
        for name in ('T_LONG', 'T_PYSSIZET', 'T_LONGLONG', 'T_ULONGLONG'):
            if name == 'T_LONG' and _testcapi.LONG_MAX < BIG:
                continue
            with self.subTest(name=name):
                self.roundtrip(self.ts, name, BIG)
        self.roundtrip(self.ts, 'T_LONGLONG', INT32_MIN - 1)

    def test_unsigned_negative(self):
        for name, mask in (('T_UINT', _testcapi.UINT_MAX),
                           ('T_ULONG', _testcapi.ULONG_MAX),
                           ('T_ULONGLONG', _testcapi.ULLONG_MAX)):
            for value in (-1, INT32_MIN, INT32_MIN - 1):
                with self.subTest(name=name, value=value):
                    with self.assertWarns(RuntimeWarning):
                        pv.MemberSetOneValue(self.ts, name, value)
                    self.assertEqual(getattr(self.ts, name), value & mask)
                    with warnings.catch_warnings():
                        warnings.simplefilter('error')
                        with self.assertRaises(RuntimeWarning):
                            pv.MemberSetOneValue(self.ts, name, value)

    def test_truncation(self):
        with self.assertWarns(RuntimeWarning):
            pv.MemberSetOneValue(self.ts, 'T_BYTE', 300)
        with warnings.catch_warnings():
            warnings.simplefilter('error')
            with self.assertRaises(RuntimeWarning):
                pv.MemberSetOneValue(self.ts, 'T_SHORT', INT32_MAX)
        with self.assertWarns(RuntimeWarning):
            pv.MemberSetOneValue(self.ts, 'T_INT', BIG)
        with self.assertRaises(OverflowError):
            pv.MemberSetOneValue(self.ts, 'T_LONGLONG', 2**70)

    def test_floats(self):
        for value in (2.5, -0.0, math.inf, -math.inf):
            with self.subTest(value=value):
                self.roundtrip(self.ts, 'T_DOUBLE', value)
                self.roundtrip(self.ts, 'T_FLOAT', value)
        self.roundtrip(self.ts, 'T_DOUBLE', 7, 7.0)
        self.roundtrip(self.ts, 'T_DOUBLE', BIG, float(BIG))
        pv.MemberSetOneValue(self.ts, 'T_DOUBLE', math.nan)
        self.assertTrue(math.isnan(self.ts.T_DOUBLE))
        self.assertEqual(pv.MemberGetOneValue(self.ts, 'T_DOUBLE')[1],
                         'object')

    def test_type_errors(self):
        for name in ('T_INT', 'T_UINT', 'T_LONGLONG', 'T_ULONGLONG'):
            with self.subTest(name=name):
                with self.assertRaises(TypeError):
                    pv.MemberSetOneValue(self.ts, name, 2.5)
                with self.assertRaises(TypeError):
                    pv.MemberSetOneValue(self.ts, name, 'a')
        with self.assertRaises(TypeError):
            pv.MemberSetOneValue(self.ts, 'T_DOUBLE', 'a')
        with self.assertRaises(Boom):
            pv.MemberSetOneValue(self.ts, 'T_UINT', Raising())
        for value in (1, 2.5):
            with self.subTest(value=value):
                with self.assertRaises(TypeError):
                    pv.MemberSetOneValue(self.ts, 'T_BOOL', value)
                with self.assertRaises(TypeError):
                    pv.MemberSetOneValue(self.ts, 'T_CHAR', value)
        self.roundtrip(self.ts, 'T_BOOL', True)
        self.roundtrip(self.ts, 'T_CHAR', 'z')
        with self.assertRaises(TypeError):
            pv.MemberSetOneValue(self.ts, 'T_STRING_INPLACE', 'a')
        # None stands for NULL: a number cannot be deleted.
        with self.assertRaises(TypeError):
            pv.MemberSetOneValue(self.ts, 'T_INT', None)

    def test_object_fields(self):
        # AttributeError.obj is _Py_T_OBJECT: the field holds a box of an
        # inline value, which reads back as the inline value.
        exc = AttributeError()
        for value in (12345, 2.5, BIG, 'a'):
            with self.subTest(value=value):
                self.roundtrip(exc, 'obj', value)
        self.assertEqual(pv.MemberSetOneValue(exc, 'obj', None), 0)
        self.assertValue(pv.MemberGetOneValue(exc, 'obj'), None)
        # A sentinel's __module__ is Py_T_OBJECT_EX.
        s = sentinel('S')
        self.roundtrip(s, '__module__', 12345)
        self.assertEqual(pv.MemberSetOneValue(s, '__module__', None), 0)
        with self.assertRaises(AttributeError):
            pv.MemberGetOneValue(s, '__module__')
        with self.assertRaises(AttributeError):
            pv.MemberSetOneValue(s, '__module__', None)

    def test_value_fields(self):
        # A __slots__ field is read and stored as it is.
        class S:
            __slots__ = ('x',)
        o = S()
        for value in (12345, INT32_MIN, 2.5, -0.0, BIG, 'a'):
            with self.subTest(value=value):
                self.roundtrip(o, 'x', value)
        nan = math.nan
        pv.MemberSetOneValue(o, 'x', nan)
        self.assertIs(pv.MemberGetOneValue(o, 'x')[0], nan)
        self.assertIs(o.x, nan)
        self.assertEqual(pv.MemberSetOneValue(o, 'x', None), 0)
        with self.assertRaises(AttributeError):
            pv.MemberGetOneValue(o, 'x')
        with self.assertRaises(AttributeError):
            pv.MemberSetOneValue(o, 'x', None)
        # A slice's start is _Py_T_VALUE and read-only.
        self.assertValue(pv.MemberGetOneValue(slice(12345, None), 'start'),
                         12345)
        with self.assertRaises(AttributeError):
            pv.MemberSetOneValue(slice(1), 'start', 2)


class SlotTests(ValueTestCase):
    """Types whose four attribute slots return and take values, static
    and heap, used from Python and through both forms of each function."""

    TYPES = [pv.ValueSlots, pv.HeapValueSlots]

    def test_getattro(self):
        for tp in self.TYPES:
            with self.subTest(tp=tp):
                o = tp()
                self.assertIs(o.i, 12345)
                self.assertEqual(o.f, 2.5)
                self.assertIs(getattr(o, 'i', None), 12345)
                self.assertTrue(hasattr(o, 'f'))
                self.assertFalse(hasattr(o, 'missing'))
                self.assertIsNone(getattr(o, 'missing', None))
                self.assertValue(pv.GetAttrValue(o, 'i'), 12345)
                self.assertValue(pv.GetAttrValue(o, 'f'), 2.5)
                self.assertEqual(_testlimitedcapi.object_getattr(o, 'i'), 12345)
                self.assertEqual(pv.GetOptionalAttrValue(o, 'f'),
                                 (1, 2.5, 'float'))
                self.assertEqual(pv.GetOptionalAttrValue(o, 'missing'),
                                 (0, None, None))
                self.assertEqual(_testcapi.object_getoptionalattr(o, 'i'),
                                 12345)
                self.assertIs(_testcapi.object_getoptionalattr(o, 'missing'),
                              AttributeError)

    def test_setattro(self):
        values = [12345, 2.5, INT32_MAX, INT32_MIN, BIG, -0.0, math.inf,
                  True, 'str']
        for tp in self.TYPES:
            for v in values:
                with self.subTest(tp=tp, v=v):
                    o = tp()
                    o.x = v
                    self.assertValue(pv.GetAttrValue(o, 'x'), v)
                    del o.x
                    self.assertFalse(hasattr(o, 'x'))
                    self.assertEqual(pv.SetAttrValue(o, 'y', v), 0)
                    self.assertValue(pv.GetAttrValue(o, 'y'), v)
                    _testlimitedcapi.object_setattr(o, 'z', v)
                    self.assertEqual(o.z, v)
                    self.assertEqual(pv.SetAttrValue(o, 'z', NULL), 0)
                    self.assertFalse(hasattr(o, 'z'))

    def test_descriptor(self):
        for tp in self.TYPES:
            with self.subTest(tp=tp):
                d = tp()
                C = type('C', (), {'d': d})
                c = C()
                self.assertIs(c.d, 12345)
                self.assertIs(C.d, d)
                self.assertValue(pv.GetAttrValue(c, 'd'), 12345)
                c.d = 2.5
                self.assertEqual(d.set, 2.5)
                self.assertEqual(pv.SetAttrValue(c, 'd', 100), 0)
                self.assertIs(d.set, 100)
                del c.d
                self.assertFalse(hasattr(d, 'set'))
                self.assertIs(type(d).__get__(d, c, C), 12345)
                type(d).__set__(d, c, 7)
                self.assertIs(d.set, 7)

    def test_generic_spec(self):
        tp = pv.HeapGeneric
        C = type('C', (tp,), {})
        c = C()
        c.x = 12345
        self.assertIs(c.x, 12345)
        self.assertValue(pv.GetAttrValue(c, 'x'), 12345)
        tp.__setattr__(c, 'y', 2.5)
        self.assertEqual(tp.__getattribute__(c, 'y'), 2.5)
        self.assertValue(pv.GetAttrValue(c, 'y'), 2.5)


NULL = None


@unittest.skipUnless(support.Py_DEBUG, "the checks are on debug builds")
class DebugCheckTests(unittest.TestCase):
    """A slot result that is a non-canonical box, or a value's bits in a
    PyObject * field, stops a debug build with the slot named."""

    PRELUDE = ("import gc, _testcapi\n"
               "pv = _testcapi.pyvalue\n")

    def assertStops(self, code, message):
        rc, out, err = assert_python_failure("-c", self.PRELUDE + code)
        self.assertIn(message.encode(), err)

    def test_getattro(self):
        message = ("Fatal Python error: tp_getattro of type "
                   "'_testcapi.pyvalue.BadGetattro' returned a non-canonical "
                   "int box")
        for code in ["pv.BadGetattro().x",
                     "getattr(pv.BadGetattro(), 'x', None)",
                     "hasattr(pv.BadGetattro(), 'x')",
                     "_testcapi.object_getoptionalattr(pv.BadGetattro(), 'x')",
                     "pv.GetOptionalAttrValue(pv.BadGetattro(), 'x')"]:
            with self.subTest(code=code):
                self.assertStops(code, message)

    def test_descr_get(self):
        message = ("Fatal Python error: tp_descr_get of type "
                   "'_testcapi.pyvalue.BadDescrGet' returned a non-canonical "
                   "int box")
        for code in [
                # directly
                "class C: d = pv.BadDescrGet()\nC().d",
                # a nested getset: type_get_doc
                "class C: __doc__ = pv.BadDescrGet()\nC.__doc__",
                # the explicit wrapper
                "d = pv.BadDescrGet()\ntype(d).__get__(d, 1, int)",
                # a special-method lookup
                "class C:\n __enter__ = pv.BadDescrGet()\n"
                " __exit__ = pv.BadDescrGet()\nwith C(): pass"]:
            with self.subTest(code=code):
                self.assertStops(code, message)

    def test_tracer(self):
        message = ("Fatal Python error: an inline value in a PyObject * "
                   "field of a '_testcapi.pyvalue.RawKeep' object")
        for v in ["5", "2.5"]:
            with self.subTest(v=v):
                self.assertStops(f"r = pv.RawKeep()\nr.a = {v}\ngc.collect()",
                                 message)


if __name__ == '__main__':
    unittest.main()
