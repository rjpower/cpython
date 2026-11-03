"""Focused C datetime reader controls; proposed Lib/test destination."""
import _datetime as datetime
import gc
import operator
import pickle
import unittest


class DatetimeReadersTests(unittest.TestCase):
    def test_iso_fields_repr_and_reduce(self):
        for value in (datetime.date(2019, 1, 1), datetime.datetime(2019, 1, 1)):
            with self.subTest(value=value):
                iso = value.isocalendar()
                self.assertEqual((iso.year, iso.week, iso.weekday), (2019, 1, 2))
                self.assertEqual(repr(iso), 'datetime.IsoCalendarDate(year=2019, week=1, weekday=2)')
                self.assertEqual(iso.__reduce__(), (tuple, ((2019, 1, 2),)))
                gc.collect()
                for protocol in range(pickle.HIGHEST_PROTOCOL + 1):
                    copied = pickle.loads(pickle.dumps(iso, protocol))
                    self.assertIs(type(copied), tuple)
                    self.assertEqual(copied, (2019, 1, 2))

    def test_numeric_chain(self):
        td = datetime.timedelta
        for numerator, quotient, remainder in ((150, 2, 30), (-150, -3, 30)):
            with self.subTest(numerator=numerator):
                self.assertEqual(divmod(td(seconds=numerator), td(minutes=1)), (quotient, td(seconds=remainder)))
        self.assertEqual(td(microseconds=3) * 0.5, td(microseconds=2))
        self.assertEqual(td(microseconds=5) / 2.0, td(microseconds=2))
        self.assertEqual(td(microseconds=-3) * 0.5, td(microseconds=-2))
        self.assertEqual(td(days=2, seconds=3, microseconds=4) / 1.0, td(days=2, seconds=3, microseconds=4))
        with self.assertRaises(ZeroDivisionError):
            divmod(td(seconds=1), td())

    def test_ratio_callback_error_identity(self):
        sentinel = RuntimeError('ratio sentinel')

        class Ratio(float):
            def as_integer_ratio(self):
                raise sentinel

        for operation in (operator.mul, operator.truediv):
            with self.subTest(operation=operation):
                with self.assertRaises(RuntimeError) as caught:
                    operation(datetime.timedelta(seconds=1), Ratio(1.0))
                self.assertIs(caught.exception, sentinel)

    def test_ratio_selected_operand_error(self):
        sentinel = RuntimeError('selected operand sentinel')

        class Operand:
            def __rmul__(self, other):
                raise sentinel

        class Ratio(float):
            def as_integer_ratio(self):
                return (Operand(), Operand())

        for operation in (operator.mul, operator.truediv):
            with self.subTest(operation=operation):
                with self.assertRaises(RuntimeError) as caught:
                    operation(datetime.timedelta(seconds=1), Ratio(1.0))
                self.assertIs(caught.exception, sentinel)

    def test_bad_ratio_validation(self):
        class Ratio(float):
            def as_integer_ratio(self):
                return ratio

        for ratio, error in ((1, TypeError), ((), ValueError), ((1,), ValueError), ((1, 2, 3), ValueError)):
            for operation in (operator.mul, operator.truediv):
                with self.subTest(ratio=ratio, operation=operation):
                    with self.assertRaises(error):
                        operation(datetime.timedelta(seconds=1), Ratio(1.0))

    def test_pickle_state_bytes_unicode_and_tzinfo(self):
        zone = datetime.timezone(datetime.timedelta(hours=3))
        values = (datetime.date(2024, 2, 29), datetime.time(12, 34, 56, 789),
                  datetime.time(12, 34, 56, 789, zone), datetime.datetime(2024, 2, 29, 12, 34, 56, 789),
                  datetime.datetime(2024, 2, 29, 12, 34, 56, 789, zone))
        for value in values:
            cls, args = value.__reduce_ex__(2)
            self.assertIs(cls, type(value))
            for state in (args[0], args[0].decode('latin1')):
                with self.subTest(value=value, state_type=type(state)):
                    result = cls(state, *args[1:])
                    self.assertEqual(result, value)
                    if cls is not datetime.date:
                        self.assertIs(result.tzinfo, value.tzinfo)

    def test_ordinary_constructor_fallback(self):
        self.assertEqual(datetime.date(2024, 2, 29).day, 29)
        self.assertEqual(datetime.time(12, 34).minute, 34)
        self.assertEqual(datetime.datetime(2024, 2, 29).year, 2024)
        for cls in (datetime.date, datetime.time, datetime.datetime):
            with self.subTest(cls=cls):
                with self.assertRaises(TypeError):
                    cls(b'x')


if __name__ == '__main__':
    unittest.main()
