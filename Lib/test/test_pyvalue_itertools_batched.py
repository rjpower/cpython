import gc
import itertools
import math
import unittest


class BatchedValueTests(unittest.TestCase):
    def test_order_dispatch_and_partial_batch(self):
        calls = []

        class Source:
            def __init__(self):
                self.values = iter(range(5))

            def __iter__(self):
                return self

            def __next__(self):
                calls.append(None)
                return next(self.values)

        iterator = itertools.batched(Source(), 2)
        self.assertEqual(next(iterator), (0, 1))
        self.assertEqual(len(calls), 2)
        self.assertEqual(next(iterator), (2, 3))
        self.assertEqual(len(calls), 4)
        self.assertEqual(next(iterator), (4,))
        self.assertEqual(len(calls), 6)
        with self.assertRaises(StopIteration):
            next(iterator)
        self.assertEqual(len(calls), 7)
        with self.assertRaises(StopIteration):
            next(iterator)
        self.assertEqual(len(calls), 7)

    def test_strict_and_error_are_terminal(self):
        iterator = itertools.batched(range(3), 2, strict=True)
        self.assertEqual(next(iterator), (0, 1))
        with self.assertRaisesRegex(ValueError, r'batched\(\): incomplete batch'):
            next(iterator)
        with self.assertRaises(StopIteration):
            next(iterator)
        marker = LookupError('batched iterator marker')
        calls = []

        class Source:
            def __iter__(self):
                return self

            def __next__(self):
                calls.append(None)
                if len(calls) == 2:
                    raise marker
                return 1000

        iterator = itertools.batched(Source(), 3)
        with self.assertRaises(LookupError) as caught:
            next(iterator)
        self.assertIs(caught.exception, marker)
        with self.assertRaises(StopIteration):
            next(iterator)
        self.assertEqual(len(calls), 2)
        self.assertEqual(list(itertools.batched([], 2)), [])
        self.assertEqual(list(itertools.batched([1, 2], 2, strict=True)), [(1, 2)])

    def test_values_and_retained_owners(self):
        events = []

        class Token:
            def __del__(self):
                events.append('item')

        item = Token()
        values = [1000, -0.0, 2**200, None, item]
        iterator = itertools.batched(values, 2)
        batches = list(iterator)
        self.assertEqual(batches, [(1000, -0.0), (2**200, None), (item,)])
        self.assertIs(batches[-1][0], item)
        self.assertEqual(math.copysign(1.0, batches[0][1]), -1.0)
        del iterator, values, item
        gc.collect()
        self.assertEqual(events, [])
        del batches
        gc.collect()
        self.assertEqual(events, ['item'])

    def test_iterator_retirement_precedes_partial_item(self):
        events = []

        class Token:
            def __del__(self):
                events.append('item')

        class Source:
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

        iterator = itertools.batched(Source(), 2, strict=True)
        with self.assertRaisesRegex(ValueError, 'incomplete batch'):
            next(iterator)
        self.assertEqual(events, ['iterator', 'item'])


if __name__ == '__main__':
    unittest.main()
