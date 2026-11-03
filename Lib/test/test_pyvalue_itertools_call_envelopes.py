"""Public reference controls for the pairwise and zip_longest C routes."""
import gc
import itertools
import unittest


class TracedIterator:
    def __init__(self, name, values, events):
        self.name = name
        self.values = iter(values)
        self.events = events

    def __iter__(self):
        return self

    def __next__(self):
        self.events.append(self.name)
        return next(self.values)


class CallEnvelopeTests(unittest.TestCase):
    def test_pairwise_dispatch_and_retained_outputs(self):
        events = []
        iterator = itertools.pairwise(TracedIterator('p', [1, 2, 3], events))
        first = next(iterator)
        self.assertEqual(first, (1, 2))
        self.assertEqual(events, ['p', 'p'])
        self.assertEqual(next(iterator), (2, 3))
        self.assertEqual(first, (1, 2))
        self.assertEqual(events, ['p'] * 3)
        for _ in range(2):
            with self.assertRaises(StopIteration):
                next(iterator)
        self.assertEqual(events, ['p'] * 4)

    def test_pairwise_direct_stop_and_error_are_terminal(self):
        for error in (StopIteration('stop'), RuntimeError('sentinel')):
            for fail_at in (1, 2, 3):
                with self.subTest(error=type(error), fail_at=fail_at):
                    calls = []

                    class Source:
                        def __iter__(self):
                            return self

                        def __next__(self):
                            calls.append(None)
                            if len(calls) == fail_at:
                                raise error
                            return len(calls)

                    iterator = itertools.pairwise(Source())
                    if fail_at == 3:
                        self.assertEqual(next(iterator), (1, 2))
                    with self.assertRaises(type(error)) as caught:
                        next(iterator)
                    self.assertIs(caught.exception, error)
                    with self.assertRaises(StopIteration):
                        next(iterator)
                    self.assertEqual(len(calls), fail_at)

    def test_zip_dispatch_sentinel_and_fill(self):
        events = []
        fill = object()
        iterator = itertools.zip_longest(TracedIterator('a', [1], events), TracedIterator('b', [2, 3, 4], events), fillvalue=fill)
        first = next(iterator)
        self.assertEqual(first, (1, 2))
        self.assertEqual(next(iterator), (fill, 3))
        self.assertEqual(events, ['a', 'b', 'a', 'b'])
        self.assertEqual(next(iterator), (fill, 4))
        self.assertEqual(events, ['a', 'b', 'a', 'b', 'b'])
        self.assertEqual(first, (1, 2))
        for _ in range(2):
            with self.assertRaises(StopIteration):
                next(iterator)
        self.assertEqual(events, ['a', 'b', 'a', 'b', 'b', 'b'])
        self.assertEqual(list(itertools.zip_longest()), [])

    def test_zip_error_stops_before_later_dispatch(self):
        events = []
        sentinel = RuntimeError('sentinel')

        class Failing:
            def __iter__(self):
                return self

            def __next__(self):
                events.append('fail')
                raise sentinel

        iterator = itertools.zip_longest(TracedIterator('before', [1], events), Failing(), TracedIterator('after', [2], events))
        with self.assertRaises(RuntimeError) as caught:
            next(iterator)
        self.assertIs(caught.exception, sentinel)
        self.assertEqual(events, ['before', 'fail'])
        with self.assertRaises(StopIteration):
            next(iterator)
        self.assertEqual(events, ['before', 'fail'])

    def test_zip_exhausted_iterator_retires_before_old_output(self):
        events = []

        class Token:
            def __del__(self):
                events.append('item')

        class Short:
            def __init__(self):
                self.called = False

            def __iter__(self):
                return self

            def __next__(self):
                if self.called:
                    raise StopIteration
                self.called = True
                return Token()

            def __del__(self):
                events.append('iterator')

        iterator = itertools.zip_longest(Short(), [1, 2])
        result = next(iterator)
        del result
        self.assertEqual(events, [])
        self.assertEqual(next(iterator), (None, 2))
        self.assertEqual(events, ['iterator', 'item'])

    def test_discarded_results_hash_gc_and_values(self):
        for iterator, expected in ((itertools.pairwise([1, 2, 3, 4]), [(1, 2), (2, 3), (3, 4)]),
                                   (itertools.zip_longest([1, 2, 3], [4]), [(1, 4), (2, None), (3, None)])):
            for wanted in expected:
                result = next(iterator)
                self.assertEqual(result, wanted)
                self.assertEqual(hash(result), hash(wanted))
                del result
                gc.collect()
            with self.assertRaises(StopIteration):
                next(iterator)


if __name__ == '__main__':
    unittest.main()
