import datetime
import io
import itertools
import typing
import unittest

import _tokenize

from test.test_clinic import _make_clinic
from typing import TypeVar


class PyValueClinicKeywordTests(unittest.TestCase):
    def test_constructor_tuple_slot_host_controls(self):
        class BytesSubclass(bytes):
            pass

        source = (65, 66)
        self.assertEqual(bytes.__new__(bytes, source), b'AB')
        self.assertEqual(bytes(source), b'AB')
        self.assertIsInstance(bytes.__new__(BytesSubclass, source), BytesSubclass)
        self.assertEqual(bytes.__new__(bytes, 'café', 'utf-8'), 'café'.encode())
        self.assertEqual(bytes('café', 'utf-8'), 'café'.encode())
        self.assertEqual(bytes.__new__(bytes, source='café', encoding='utf-8',
                                      errors='strict'), 'café'.encode())
        self.assertEqual(bytes(source='café', encoding='utf-8', errors='strict'),
                         'café'.encode())
        self.assertEqual(bytes.__new__(bytes, 3), b'\0\0\0')
        self.assertEqual(bytes(3), b'\0\0\0')
        self.assertEqual(bytes.__new__(bytes), b'')
        target = bytearray()
        self.assertIsNone(bytearray.__init__(target, 'café', 'utf-8'))
        self.assertEqual(target, bytearray('café', 'utf-8'))
        self.assertIsNone(bytearray.__init__(target, source='café',
                                             encoding='ascii', errors='ignore'))
        self.assertEqual(target, b'caf')
        self.assertEqual(bytearray(source='café', encoding='ascii',
                                   errors='ignore'), target)
        self.assertIsNone(bytearray.__init__(target, source))
        self.assertEqual(target, bytearray(source))
        self.assertIsNone(bytearray.__init__(target, 3))
        self.assertEqual(target, b'\0\0\0')
        self.assertEqual(bytearray(3), b'\0\0\0')
        class BytearraySubclass(bytearray):
            pass
        subclass = BytearraySubclass()
        self.assertIsNone(bytearray.__init__(subclass, source))
        self.assertEqual(subclass, bytearray(source))
        class Index:
            def __init__(self, calls):
                self.calls = calls

            def __index__(self):
                self.calls.append('start')
                return 4

        calls = []
        explicit = enumerate.__new__(enumerate, ['x', 'y'], Index(calls))
        self.assertEqual(list(explicit), [(4, 'x'), (5, 'y')])
        self.assertEqual(calls, ['start'])
        self.assertEqual(list(enumerate(['x', 'y'], start=4)),
                         [(4, 'x'), (5, 'y')])
        class EnumerateSubclass(enumerate):
            pass
        sub_explicit = enumerate.__new__(EnumerateSubclass, ['x'], start=9)
        self.assertEqual(list(sub_explicit), [(9, 'x')])
        self.assertEqual(list(EnumerateSubclass(['x'], start=9)), [(9, 'x')])
        with self.assertRaises(TypeError):
            enumerate.__new__(enumerate)
        with self.assertRaises(TypeError):
            enumerate.__new__(enumerate, ['x'], 0, 1)
        with self.assertRaises(TypeError):
            enumerate.__new__(enumerate, ['x'], iterable=['y'])

    def test_constructor_tuple_slot_host_controls_errors(self):
        sentinel = LookupError('source callback')
        calls = []

        class BrokenIterable:
            def __iter__(self):
                calls.append('iter')
                raise sentinel

        for make in (lambda: bytes.__new__(bytes, BrokenIterable()),
                     lambda: bytes(BrokenIterable()),
                     lambda: bytearray.__init__(bytearray(), BrokenIterable()),
                     lambda: bytearray(BrokenIterable())):
            with self.assertRaises(LookupError) as caught:
                make()
            self.assertIs(caught.exception, sentinel)
        self.assertEqual(calls, ['iter', 'iter', 'iter', 'iter'])

        # Invalid arity, unknown names and duplicate source arguments must be
        # rejected before the tuple-slot path invokes user iteration hooks.
        calls.clear()
        rejected = (
            lambda: bytes.__new__(bytes, BrokenIterable(), 'utf-8', 'strict', 'extra'),
            lambda: bytes(BrokenIterable(), 'utf-8', 'strict', 'extra'),
            lambda: bytearray.__init__(bytearray(), BrokenIterable(), 'utf-8', 'strict', 'extra'),
            lambda: bytearray(BrokenIterable(), 'utf-8', 'strict', 'extra'),
            lambda: bytes.__new__(bytes, source=BrokenIterable(), unknown=True),
            lambda: bytes(source=BrokenIterable(), unknown=True),
            lambda: bytearray.__init__(bytearray(), source=BrokenIterable(), unknown=True),
            lambda: bytearray(source=BrokenIterable(), unknown=True),
            lambda: bytes.__new__(bytes, BrokenIterable(), source=BrokenIterable()),
            lambda: bytes(BrokenIterable(), source=BrokenIterable()),
            lambda: bytearray.__init__(bytearray(), BrokenIterable(), source=BrokenIterable()),
            lambda: bytearray(BrokenIterable(), source=BrokenIterable()),
        )
        for call in rejected:
            with self.assertRaises(TypeError):
                call()
        self.assertEqual(calls, [])

        class Index:
            def __init__(self, value):
                self.value = value

            def __index__(self):
                calls.append(self.value)
                return self.value

        calls.clear()
        self.assertEqual(bytes.__new__(bytes, [Index(65), Index(66)]), b'AB')
        self.assertEqual(calls, [65, 66])

        calls.clear()
        for make in (lambda: enumerate.__new__(enumerate, BrokenIterable(), 0, 1),
                     lambda: enumerate(BrokenIterable(), 0, 1)):
            with self.assertRaises(TypeError):
                make()
        self.assertEqual(calls, [])

        calls.clear()
        invalid_enumerate = (
            lambda: enumerate.__new__(enumerate, [1], Index(2), 3),
            lambda: enumerate([1], Index(2), 3),
            lambda: enumerate.__new__(enumerate, iterable=[1], start=Index(2), unknown=True),
            lambda: enumerate(iterable=[1], start=Index(2), unknown=True),
            lambda: enumerate.__new__(enumerate, [1], Index(2), start=Index(3)),
            lambda: enumerate([1], Index(2), start=Index(3)),
        )
        for call in invalid_enumerate:
            with self.assertRaises(TypeError):
                call()
        self.assertEqual(calls, [])

        index_error = ArithmeticError('start conversion')

        class BrokenIndex:
            def __index__(self):
                raise index_error

        with self.assertRaises(ArithmeticError) as caught:
            enumerate.__new__(enumerate, [1], BrokenIndex())
        self.assertIs(caught.exception, index_error)

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
