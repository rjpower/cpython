import gc
import itertools
import math
import unittest


class TupleNextTests(unittest.TestCase):
    def factories(self, values):
        return (
            (lambda: itertools.product(values, repeat=2),
             [(a, b) for a in values for b in values]),
            (lambda: itertools.combinations(values, 2),
             [(values[i], values[j]) for i in range(len(values))
              for j in range(i + 1, len(values))]),
            (lambda: itertools.combinations_with_replacement(values, 2),
             [(values[i], values[j]) for i in range(len(values))
              for j in range(i, len(values))]),
            (lambda: itertools.permutations(values, 2),
             [(values[i], values[j]) for i in range(len(values))
              for j in range(len(values)) if i != j]),
        )

    def test_retained_results_are_stable(self):
        values = [1000, -0.0, 'last']
        for factory, expected in self.factories(values):
            with self.subTest(factory=factory):
                iterator = factory()
                actual = list(iterator)
                self.assertEqual(actual, expected)
                self.assertTrue(all(type(item) is tuple for item in actual))
                self.assertEqual(next(iterator, 'done'), 'done')
                # Retained outputs force copying rather than recycling.
                for row in actual:
                    for item in row:
                        if type(item) is float:
                            self.assertEqual(math.copysign(1.0, item), -1.0)

    def test_unretained_results_recycle_after_gc_and_hash(self):
        for factory, expected in self.factories([1000, 2000, 3000]):
            with self.subTest(factory=factory):
                iterator = factory()
                for row in expected:
                    actual = next(iterator)
                    self.assertEqual(actual, row)
                    self.assertEqual(hash(actual), hash(row))
                    del actual
                    gc.collect()
                self.assertEqual(next(iterator, 'done'), 'done')

    def test_mortal_items_remain_owned(self):
        events = []
        class Token:
            def __init__(self, index):
                self.index = index
            def __del__(self):
                events.append(self.index)
        for family in range(4):
            events.clear()
            values = [Token(0), Token(1), Token(2)]
            factory, expected = self.factories(values)[family]
            iterator = factory()
            actual = list(iterator)
            for left, right in zip(actual, expected):
                for got, want in zip(left, right):
                    self.assertIs(got, want)
            del left, right, got, want
            del iterator, values, expected, factory
            gc.collect()
            self.assertEqual(events, [])
            del actual
            gc.collect()
            self.assertEqual(sorted(events), [0, 1, 2])

    def test_empty_and_zero_width(self):
        self.assertEqual(list(itertools.product()), [()])
        self.assertEqual(list(itertools.product([], [1])), [])
        self.assertEqual(list(itertools.product([1], repeat=0)), [()])
        for constructor in (itertools.combinations,
                            itertools.combinations_with_replacement,
                            itertools.permutations):
            self.assertEqual(list(constructor([], 0)), [()])
            self.assertEqual(list(constructor([], 1)), [])
            self.assertEqual(list(constructor([1], 0)), [()])
        self.assertEqual(list(itertools.combinations([1], 2)), [])
        self.assertEqual(list(itertools.permutations([1], 2)), [])


if __name__ == '__main__':
    unittest.main()
