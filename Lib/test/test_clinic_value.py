"""Runtime tests of value-emission Argument Clinic output.

_testclinic_value is compiled from Clinic output in the value form
(the value_emission directive).  Its functions are checked against the
results above all and against the error messages of the object-emission
module _testclinic, which must be the same.
"""

import math
import unittest
from test.support import import_helper

_testclinic_value = import_helper.import_module('_testclinic_value')
_testclinic = import_helper.import_module('_testclinic')

V = _testclinic_value
NAN = float('nan')
BIG = 2**70


class WithIndex:
    def __index__(self):
        return 7


class WithFloat:
    def __float__(self):
        return 2.5


class TupleSub(tuple):
    pass


def outcome(func, *args, **kwargs):
    """The result of a call, or the type and message of its exception."""
    try:
        return 'ok', func(*args, **kwargs)
    except Exception as exc:
        return type(exc), str(exc)


def twin_error(name, *args, rename=None, **kwargs):
    """The error of the same call to the object-emission function, with
    the module and (if renamed) the function name of the value module."""
    kind, message = outcome(getattr(_testclinic, name), *args, **kwargs)
    assert kind != 'ok', (name, args, kwargs)
    message = message.replace('_testclinic.', '_testclinic_value.')
    if rename is not None:
        message = message.replace(name, rename)
    return kind, message


class ValueConverterTests(unittest.TestCase):

    def assert_same_error(self, name, args=(), kwargs=None, *,
                          twin_args=None, rename=None):
        kwargs = kwargs or {}
        expected = twin_error(name, *(args if twin_args is None else twin_args),
                              rename=rename, **kwargs)
        self.assertEqual(outcome(getattr(V, name), *args, **kwargs), expected)

    def assert_error(self, func, kind, message, *args, **kwargs):
        self.assertEqual(outcome(func, *args, **kwargs), (kind, message))

    def test_object(self):
        obj = object()
        for x in (5, -1, BIG, 1.5, True, False, None, 'str', obj):
            with self.subTest(x=x):
                result = V.object_converter(x)
                self.assertEqual(result, x)
                self.assertIs(type(result), type(x))
        self.assertIs(V.object_converter(obj), obj)
        self.assertTrue(math.isnan(V.object_converter(NAN)))
        self.assertEqual(V.objects_converter(5), (5, None))
        self.assertEqual(V.objects_converter(1.5, -1), (1.5, -1))

    def test_int(self):
        self.assertEqual(V.int_converter(), (12, 34))
        for x, expected in [(5, 5), (-1, -1), (True, 1), (WithIndex(), 7)]:
            with self.subTest(x=x):
                self.assertEqual(V.int_converter(x, x), (expected, expected))
        for x in (BIG, 1.5, NAN, 'x', None):
            with self.subTest(x=x):
                self.assert_same_error('int_converter', (x,))
                self.assert_same_error('int_converter', (0, x))

    def test_long(self):
        for x, expected in [(5, 5), (-1, -1), (False, 0), (WithIndex(), 7)]:
            with self.subTest(x=x):
                self.assertEqual(V.long_converter(x), expected)
        for x in (BIG, 1.5, 'x', None):
            with self.subTest(x=x):
                self.assert_same_error('long_converter', (x,))

    def test_py_ssize_t(self):
        self.assertEqual(V.py_ssize_t_converter(), (12, 56, 78, 90))
        self.assertEqual(V.py_ssize_t_converter(-1, 5, 0), (-1, 5, 0, 90))
        self.assertEqual(V.py_ssize_t_converter(WithIndex(), WithIndex()),
                         (7, 7, 78, 90))
        for x in (BIG, 1.5, 'x', None):
            with self.subTest(x=x):
                self.assert_same_error('py_ssize_t_converter', (x,))

    def test_py_ssize_t_accept_none(self):
        self.assertEqual(V.py_ssize_t_converter(1, None), (1, 56, 78, 90))
        self.assertEqual(V.py_ssize_t_converter(1, -1), (1, -1, 78, 90))
        # The twin's c is the second parameter here.
        for x in (BIG, 'x', 1.5):
            with self.subTest(x=x):
                self.assert_same_error('py_ssize_t_converter', (0, x),
                                       twin_args=(0, 0, x))
        self.assert_error(V.py_ssize_t_converter, TypeError,
                          "argument should be integer or None, not 'str'",
                          0, 'x')

    def test_py_ssize_t_not_negative(self):
        self.assertEqual(V.py_ssize_t_converter(0, 0, 0, None),
                         (0, 0, 0, 90))
        # The object form names the parameter for 'n', and says
        # "argument" in the accept-None converter function.
        self.assert_error(V.py_ssize_t_converter, ValueError,
                          'c cannot be negative', 0, 0, -1)
        self.assert_error(V.py_ssize_t_converter, ValueError,
                          'argument cannot be negative', 0, 0, 0, -1)
        # The twin's d and e are the third and fourth parameters here.
        for x in (BIG, 'x'):
            with self.subTest(x=x):
                self.assert_same_error('py_ssize_t_converter', (0, 0, x),
                                       twin_args=(0, 0, 0, x))
                self.assert_same_error('py_ssize_t_converter', (0, 0, 0, x),
                                       twin_args=(0, 0, 0, 0, x))

    def test_slice_index(self):
        top = 2**63 - 1
        self.assertEqual(V.slice_index_converter(), (12, 34, 56))
        self.assertEqual(V.slice_index_converter(-1, WithIndex(), None),
                         (-1, 7, 56))
        self.assertEqual(V.slice_index_converter(BIG, -BIG, BIG),
                         (top, -top - 1, top))
        for args in [('x',), (1.5,), (0, None), (0, 0, 'x')]:
            with self.subTest(args=args):
                self.assert_same_error('slice_index_converter', args)

    def test_bool(self):
        self.assertEqual(V.bool_converter(), (True, True))
        for x in (0, 1, 5, BIG, 0.0, 1.5, NAN, '', 'x', None, [], [0]):
            with self.subTest(x=x):
                self.assertEqual(V.bool_converter(x), (bool(x), True))
        for x in (0, 5, True, False, WithIndex()):
            with self.subTest(x=x):
                self.assertEqual(V.bool_converter(True, x)[1], bool(x))
        # accept={int} reads a C int, as PyLong_AsInt() does.
        for x in (BIG, 1.5, 'x', None):
            with self.subTest(x=x):
                self.assert_same_error('bool_converter', (True, x),
                                       twin_args=(True, True, x))

    def test_double(self):
        self.assertEqual(V.double_converter(), 12.5)
        for x, expected in [(1.5, 1.5), (5, 5.0), (-1, -1.0), (True, 1.0),
                            (BIG, float(BIG)), (WithFloat(), 2.5)]:
            with self.subTest(x=x):
                result = V.double_converter(x)
                self.assertEqual(result, expected)
                self.assertIs(type(result), float)
        self.assertTrue(math.isnan(V.double_converter(NAN)))
        for x in ('x', None, 2**1100):
            with self.subTest(x=x):
                self.assert_same_error('double_converter', (x,))

    def test_unicode(self):
        self.assertEqual(V.unicode_converter('str'), 'str')
        for x in (5, 1.5, None, b'x'):
            with self.subTest(x=x):
                self.assert_same_error('unicode_converter', (x,))

    def test_str(self):
        self.assertEqual(V.str_converter(), 'a')
        self.assertEqual(V.str_converter('h\xe9'), 'h\xe9')
        for x in (5, None):
            with self.subTest(x=x):
                self.assert_same_error('str_converter', (x,))
        self.assert_same_error('str_converter', ('a\0',))

    def test_str_accept_none(self):
        self.assertEqual(V.str_none_converter('s'), 's')
        self.assertIsNone(V.str_none_converter(None))
        self.assert_error(V.str_none_converter, TypeError,
                          'str_none_converter() argument must be str or None, '
                          'not int', 5)

    def test_py_buffer(self):
        for x in (b'abc', bytearray(b'ab'), memoryview(b'a')):
            with self.subTest(x=x):
                self.assertEqual(V.py_buffer_converter(x), len(x))
        for x, name in [(5, 'int'), ('str', 'str'), (None, 'NoneType')]:
            with self.subTest(x=x):
                self.assert_error(V.py_buffer_converter, TypeError,
                                  f"a bytes-like object is required, "
                                  f"not '{name}'", x)

    def test_subclass_of(self):
        sub = TupleSub((1, 2))
        self.assertEqual(V.subclass_tuple((1,)), (1,))
        self.assertIs(V.subclass_tuple(sub), sub)
        self.assert_error(V.subclass_tuple, TypeError,
                          'subclass_tuple() argument must be tuple, not list',
                          [1])
        for x in (5, -1, BIG, True):
            with self.subTest(x=x):
                self.assertEqual(V.subclass_long(x), x)
        # _PyArg_BadArgument() names None, not its type.
        for x, name in [(1.5, 'float'), ('1', 'str'), (None, 'None')]:
            with self.subTest(x=x):
                self.assert_error(V.subclass_long, TypeError,
                                  f'subclass_long() argument must be int, '
                                  f'not {name}', x)

    def test_return_converters(self):
        for x in (0, 1, 5, '', 'x', None):
            with self.subTest(x=x):
                self.assertIs(V.ret_bool(x), bool(x))
        for x in (5, -1, 0, 2**31 - 1):
            with self.subTest(x=x):
                result = V.ret_int(x)
                self.assertEqual(result, x)
                self.assertIs(type(result), int)
        self.assertEqual(V.ret_ssize_t(-1), -1)
        self.assertEqual(V.ret_ssize_t(2**40), 2**40)
        self.assertRaises(OverflowError, V.ret_ssize_t, BIG)
        self.assertRaises(OverflowError, V.ret_int, 2**31)
        for x, expected in [(1.5, 1.5), (5, 5.0), (-1, -1.0)]:
            with self.subTest(x=x):
                result = V.ret_double(x)
                self.assertEqual(result, expected)
                self.assertIs(type(result), float)
        self.assertTrue(math.isnan(V.ret_double(NAN)))


class ValueShapeTests(unittest.TestCase):

    def assert_same_error(self, name, args=(), kwargs=None, *,
                          twin_args=None, rename=None):
        kwargs = kwargs or {}
        expected = twin_error(name, *(args if twin_args is None else twin_args),
                              rename=rename, **kwargs)
        self.assertEqual(outcome(getattr(V, name), *args, **kwargs), expected)

    def test_no_parameters(self):
        self.assertIsNone(V.test_empty_function())
        for args, kwargs in [((1,), {}), ((), {'a': 1}), ((1, 2), {'a': 1})]:
            with self.subTest(args=args, kwargs=kwargs):
                self.assert_same_error('test_empty_function', args, kwargs)

    def test_one_object(self):
        for args, kwargs in [((), {}), ((1, 2), {}), ((), {'a': 'x'})]:
            with self.subTest(args=args, kwargs=kwargs):
                self.assert_same_error('unicode_converter', args, kwargs)

    def test_positional_only_optional(self):
        for args, kwargs in [((), {}), ((1, 2, 3), {}), ((1,), {'a': 1}),
                             ((), {'a': 1, 'b': 2})]:
            with self.subTest(args=args, kwargs=kwargs):
                self.assert_same_error('objects_converter', args, kwargs)

    def test_keywords(self):
        self.assertEqual(V.keywords(1, 2), (1, 2))
        self.assertEqual(V.keywords(b=2.5, a=BIG), (BIG, 2.5))
        self.assertEqual(V.keywords_opt(1), (1, None, None))
        self.assertEqual(V.keywords_opt(1, c=3), (1, None, 3))
        self.assertEqual(V.keywords_opt(c=3, b=2, a=1), (1, 2, 3))
        cases = {
            'keywords': [
                ((), {}), ((1,), {}), ((1, 2, 3), {}), ((1,), {'a': 2}),
                ((1, 2), {'a': 2}), ((1,), {'c': 2}), ((), {'b': 1, 'c': 2}),
            ],
            'keywords_opt': [
                ((), {}), ((1, 2, 3, 4), {}), ((1,), {'a': 2}),
                ((1,), {'z': 2}), ((), {'b': 1, 'c': 2}),
            ],
        }
        for name, calls in cases.items():
            for args, kwargs in calls:
                with self.subTest(name=name, args=args, kwargs=kwargs):
                    self.assert_same_error(name, args, kwargs)

    def test_keyword_only(self):
        self.assertEqual(V.keywords_kwonly(1, b=2), (1, 2))
        for args, kwargs in [((1, 2), {}), ((1,), {}), ((), {'b': 1}),
                             ((1,), {'b': 2, 'c': 3})]:
            with self.subTest(args=args, kwargs=kwargs):
                self.assert_same_error('keywords_kwonly', args, kwargs)

    def test_positional_only_then_keyword(self):
        self.assertEqual(V.posonly_keywords(1, b=2), (1, 2))
        self.assertEqual(V.posonly_keywords(1, 2), (1, 2))
        for args, kwargs in [((), {'a': 1, 'b': 2}), ((1,), {}),
                             ((1,), {'a': 2})]:
            with self.subTest(args=args, kwargs=kwargs):
                self.assert_same_error('posonly_keywords', args, kwargs)

    def test_varargs(self):
        self.assertEqual(V.varpos(), ())
        self.assertEqual(V.varpos(1, 2.5, None, BIG), (1, 2.5, None, BIG))
        self.assertEqual(V.varpos_array(), ())
        self.assertEqual(V.varpos_array(-1, 1.5, 'x', BIG), (-1, 1.5, 'x', BIG))
        for name in ('varpos', 'varpos_array'):
            with self.subTest(name=name):
                self.assert_same_error(name, (1,), {'a': 1})

    def test_kwargs_dict(self):
        self.assertEqual(V.kwds_with_pos_only(1, 2), (1, 2, {}))
        self.assertEqual(V.kwds_with_pos_only(1, 2, x=3, a=4),
                         (1, 2, {'x': 3, 'a': 4}))
        for args in [(), (1,), (1, 2, 3)]:
            with self.subTest(args=args):
                self.assert_same_error('kwds_with_pos_only', args)


class ValueClassTests(unittest.TestCase):

    def test_new_with_keyword(self):
        pair = V.Pair(1, b=BIG)
        self.assertEqual(pair.get(), (1, BIG))
        self.assertEqual(V.Pair(1.5).get(), (1.5, None))
        # tp_new receives a tuple and a dict: the dict is unpacked.
        self.assertEqual(V.Pair(*(2,), **{'b': 3}).get(), (2, 3))
        self.assertEqual(V.Pair(a=4).get(), (4, None))

    def test_new_errors(self):
        for args, kwargs in [((), {}), ((1, 2), {}), ((), {'b': 1})]:
            with self.subTest(args=args, kwargs=kwargs):
                kind, message = outcome(V.Pair, *args, **kwargs)
                expected = twin_error('keywords_kwonly_opt', *args,
                                      rename='Pair', **kwargs)
                self.assertEqual((kind, message), expected)

    def test_new_object_paths(self):
        # type.__call__ and __new__ reach tp_new with a tuple and a dict,
        # not the value vectorcall.
        self.assertEqual(type.__call__(V.Pair, 1, b=2).get(), (1, 2))
        self.assertEqual(V.Pair.__new__(V.Pair, 3, b=4.5).get(), (3, 4.5))
        self.assertEqual(V.Pair.__new__(V.Pair, BIG).get(), (BIG, None))

    def test_init(self):
        # A call from Python, type.__call__ and __init__ reach tp_init with
        # a tuple and a dict.
        self.assertEqual(V.Cell(1, b=2.5).get(), (1, 2.5))
        self.assertEqual(V.Cell(BIG).get(), (BIG, None))
        self.assertEqual(type.__call__(V.Cell, 3, b=4).get(), (3, 4))
        cell = V.Cell(1)
        cell.__init__(5, b=NAN)
        a, b = cell.get()
        self.assertEqual(a, 5)
        self.assertTrue(math.isnan(b))
        cell.__init__(6)
        self.assertEqual(cell.get(), (6, None))

    def test_value_vectorcall(self):
        # A value call (_PyValue_Call) reaches the generated value
        # vectorcalls of Pair.__new__ and Cell.__init__.
        call = import_helper.import_module('_testinternalcapi').call_value
        self.assertEqual(call(V.Pair, 1, b=2).get(), (1, 2))
        self.assertEqual(call(V.Pair, BIG).get(), (BIG, None))
        self.assertEqual(call(V.Cell, 1.5, b=-1).get(), (1.5, -1))
        self.assertEqual(call(V.Cell, 7).get(), (7, None))
        for args, kwargs in [((), {}), ((1, 2), {}), ((), {'b': 1}),
                             ((1,), {'z': 1})]:
            for cls in (V.Pair, V.Cell):
                with self.subTest(cls=cls, args=args, kwargs=kwargs):
                    expected = twin_error('keywords_kwonly_opt', *args,
                                          rename=cls.__name__, **kwargs)
                    self.assertEqual(outcome(call, cls, *args, **kwargs),
                                     expected)

    def test_init_errors(self):
        cell = V.Cell(1)
        for args, kwargs in [((), {}), ((1, 2), {}), ((), {'b': 1}),
                             ((1,), {'z': 1})]:
            with self.subTest(args=args, kwargs=kwargs):
                expected = twin_error('keywords_kwonly_opt', *args,
                                      rename='Cell', **kwargs)
                self.assertEqual(outcome(V.Cell, *args, **kwargs), expected)
                self.assertEqual(outcome(cell.__init__, *args, **kwargs),
                                 expected)

    def test_method_arity(self):
        pair = V.Pair(1)
        kind, message = outcome(pair.get, 1)
        self.assertIs(kind, TypeError)
        self.assertIn('get() takes no arguments (1 given)', message)

    def test_defining_class(self):
        probe = V.Probe()
        self.assertIs(probe.where(), V.Probe)
        self.assertIs(V.Probe.where(probe), V.Probe)


if __name__ == '__main__':
    unittest.main()
