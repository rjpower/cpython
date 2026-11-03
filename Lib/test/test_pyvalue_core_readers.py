import gc
import math
import operator
import sys
import unittest
import weakref


class CoreReaderTests(unittest.TestCase):
    def test_enumerate_snapshots(self):
        for start in (0, sys.maxsize - 1, sys.maxsize, sys.maxsize + 1):
            with self.subTest(start=start):
                items = [object(), -0.0, 2**80]
                result = list(enumerate(items, start))
                self.assertEqual([x[0] for x in result], [start, start+1, start+2])
                self.assertIs(result[0][1], items[0])
                self.assertEqual(math.copysign(1.0, result[1][1]), -1.0)
                self.assertEqual(result[2][1], 2**80)

    def test_enumerate_recycle_hash_gc_and_retirement(self):
        class Item:
            pass
        for start in (0, sys.maxsize):
            with self.subTest(start=start):
                item = Item()
                ref = weakref.ref(item)
                def source(first):
                    yield first
                    del first
                    yield 'second'
                    yield 'third'
                en = enumerate(source(item), start)
                del item
                first = next(en)
                self.assertIs(first[1], ref())
                del first
                second = next(en)
                self.assertEqual(second, (start+1, 'second'))
                self.assertEqual(hash(second), hash((start+1, 'second')))
                del second
                gc.collect()
                third = next(en)
                self.assertEqual(third, (start+2, 'third'))
                self.assertEqual(hash(third), hash((start+2, 'third')))
                self.assertIsNone(ref())
                with self.assertRaises(StopIteration):
                    next(en)

    def test_enumerate_callback_error(self):
        class Broken:
            def __iter__(self):
                return self
            def __next__(self):
                raise LookupError('source')
        for start in (0, sys.maxsize):
            with self.subTest(start=start):
                with self.assertRaisesRegex(LookupError, 'source'):
                    next(enumerate(Broken(), start))

    def test_map_reduce_public_routes(self):
        for strict in (False, True):
            with self.subTest(strict=strict):
                a, b = iter([1, 2, 3]), iter([10, 20, 30])
                original = map(operator.add, a, b, strict=strict)
                self.assertEqual(next(original), 11)
                reduction = original.__reduce__()
                self.assertIs(reduction[0], map)
                self.assertIs(reduction[1][0], operator.add)
                self.assertIs(reduction[1][1], a)
                self.assertIs(reduction[1][2], b)
                self.assertEqual(len(reduction), 3 if strict else 2)
                restored = reduction[0](*reduction[1])
                if strict:
                    self.assertIs(reduction[2], True)
                    restored.__setstate__(reduction[2])
                self.assertEqual(list(restored), [22, 33])


if __name__ == '__main__':
    unittest.main()
