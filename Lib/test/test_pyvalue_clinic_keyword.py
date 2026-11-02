import datetime
import io
import itertools
import typing
import unittest

import _tokenize

from test.test_clinic import _make_clinic
from typing import TypeVar


class PyValueClinicKeywordTests(unittest.TestCase):
    def test_typevar_names_constraints_bound_and_default(self):
        name_only = TypeVar('NameOnly')
        keyword_name = TypeVar(name='KeywordName')
        self.assertEqual(name_only.__constraints__, ())
        self.assertEqual(keyword_name.__constraints__, ())

        constraints = TypeVar('Constraints', int, str)
        self.assertEqual(constraints.__constraints__, (int, str))
        self.assertIsNone(constraints.__bound__)

        bounded = TypeVar('Bounded', bound=int)
        self.assertIs(bounded.__bound__, int)
        self.assertIs(bounded.__default__, typing.NoDefault)
        defaulted = TypeVar('Defaulted', bound=int, default=int)
        self.assertIs(defaulted.__bound__, int)
        self.assertIs(defaulted.__default__, int)

        with self.assertRaisesRegex(
                TypeError, 'Constraints cannot be combined with bound'):
            TypeVar('Invalid', int, str, bound=int)

    def test_typevar_covariant_callback_runs_once(self):
        calls = []

        class Truth:
            def __bool__(self):
                calls.append('covariant')
                return True

        variable = TypeVar('Covariant', int, str, covariant=Truth())
        self.assertTrue(variable.__covariant__)
        self.assertEqual(calls, ['covariant'])

    def test_typevar_boolean_error_stops_later_converter(self):
        calls = []
        error = LookupError('covariant truth conversion')

        class BrokenTruth:
            def __bool__(self):
                calls.append('covariant')
                raise error

        class LaterTruth:
            def __bool__(self):
                calls.append('contravariant')
                return True

        with self.assertRaises(LookupError) as caught:
            TypeVar('Broken', covariant=BrokenTruth(),
                    contravariant=LaterTruth())
        self.assertIs(caught.exception, error)
        self.assertEqual(calls, ['covariant'])

    def test_fixed_optional_constructor_uses_selected_owner_count(self):
        source = (
            '/*[clinic input]\noutput everything block\n'
            'class Owner "OwnerObject *" "&Owner_Type"\n'
            '@classmethod\nOwner.__new__\n'
            '    first: object\n    index: int = 7\n'
            '    /\n    text: str = "x"\n'
            '    *\n    flag: bool = False\n'
            '[clinic start generated code]*/\n')
        generated = _make_clinic(filename='owner.c').parse(source)
        self.assertIn('Py_ssize_t nargs = PyTuple_GET_SIZE(args);', generated)
        self.assertIn('_PyArg_UnpackKeywordsOwned(', generated)
        self.assertIn('noptargs = Py_MIN(_args_scope.nargs, 3)', generated)
        self.assertIn('_args_scope.nobjects - _args_scope.nargs', generated)
        self.assertIn('- (1 + 0);', generated)
        self.assertNotIn('PyDict_GET_SIZE(kwargs)', generated)
        self.assertEqual(generated.count('_PyArg_OwnerScope_End('), 1)

    def test_integer_and_complex_constructors(self):
        self.assertEqual(int('101', base=2), 5)
        self.assertEqual(complex(real=2, imag=-3), 2 - 3j)
        with self.assertRaises(TypeError):
            complex(real=2, unknown=3)

    def test_date_index_callbacks_and_short_circuit(self):
        events = []

        class Index:
            def __init__(self, name, value):
                self.name = name
                self.value = value

            def __index__(self):
                events.append(self.name)
                return self.value

        value = datetime.date(year=Index('year', 2024),
                              month=Index('month', 2), day=Index('day', 29))
        self.assertEqual(value.isoformat(), '2024-02-29')
        self.assertEqual(events, ['year', 'month', 'day'])

        error = LookupError('first converter')
        events.clear()

        class BrokenIndex:
            def __index__(self):
                events.append('year')
                raise error

        with self.assertRaises(LookupError) as caught:
            datetime.date(year=BrokenIndex(), month=Index('month', 2), day=1)
        self.assertIs(caught.exception, error)
        self.assertEqual(events, ['year'])

    def test_time_fold_callback(self):
        calls = []

        class Index:
            def __index__(self):
                calls.append('fold')
                return 1

        value = datetime.time(hour=3, minute=4, fold=Index())
        self.assertEqual(value.fold, 1)
        self.assertEqual(calls, ['fold'])

    def test_batched_strict_callback_and_error(self):
        calls = []

        class Truth:
            def __bool__(self):
                calls.append('strict')
                return True

        batches = itertools.batched([1, 2, 3], 2, strict=Truth())
        self.assertEqual(next(batches), (1, 2))
        self.assertEqual(calls, ['strict'])
        with self.assertRaisesRegex(ValueError, 'incomplete batch'):
            next(batches)

    def test_bytesio_explicit_init(self):
        value = io.BytesIO()
        self.assertIsNone(io.BytesIO.__init__(value, initial_bytes=b'payload'))
        self.assertEqual(value.getvalue(), b'payload')

    def test_tokenizer_keyword_only_arguments(self):
        lines = iter((b'value = 42\n', b''))
        reads = []

        def readline():
            reads.append(None)
            return next(lines)

        tokens = list(_tokenize.TokenizerIter(
            readline, extra_tokens=True, encoding='utf-8'))
        self.assertTrue(tokens)
        self.assertGreaterEqual(len(reads), 2)


if __name__ == '__main__':
    unittest.main()
