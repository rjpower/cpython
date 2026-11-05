import doctest
import sys
import unittest
from test import support


doctests = """

Unpack tuple

    >>> t = (1, 2, 3)
    >>> a, b, c = t
    >>> a == 1 and b == 2 and c == 3
    True

Unpack list

    >>> l = [4, 5, 6]
    >>> a, b, c = l
    >>> a == 4 and b == 5 and c == 6
    True

Unpack dict

    >>> d = {4: 'four', 5: 'five', 6: 'six'}
    >>> a, b, c = d
    >>> a == 4 and b == 5 and c == 6
    True

Unpack implied tuple

    >>> a, b, c = 7, 8, 9
    >>> a == 7 and b == 8 and c == 9
    True

Unpack string... fun!

    >>> a, b, c = 'one'
    >>> a == 'o' and b == 'n' and c == 'e'
    True

Unpack generic sequence

    >>> class Seq:
    ...     def __getitem__(self, i):
    ...         if i >= 0 and i < 3: return i
    ...         raise IndexError
    ...
    >>> a, b, c = Seq()
    >>> a == 0 and b == 1 and c == 2
    True

Single element unpacking, with extra syntax

    >>> st = (99,)
    >>> sl = [100]
    >>> a, = st
    >>> a
    99
    >>> b, = sl
    >>> b
    100

Now for some failures

Unpacking non-sequence

    >>> a, b, c = 7
    Traceback (most recent call last):
      ...
    TypeError: cannot unpack non-iterable int object

Unpacking tuple of wrong size

    >>> a, b = t
    Traceback (most recent call last):
      ...
    ValueError: too many values to unpack (expected 2, got 3)

Unpacking tuple of wrong size

    >>> a, b = l
    Traceback (most recent call last):
      ...
    ValueError: too many values to unpack (expected 2, got 3)

Unpacking sequence too short

    >>> a, b, c, d = Seq()
    Traceback (most recent call last):
      ...
    ValueError: not enough values to unpack (expected 4, got 3)

Unpacking sequence too long

    >>> a, b = Seq()
    Traceback (most recent call last):
      ...
    ValueError: too many values to unpack (expected 2)

Unpacking a sequence where the test for too long raises a different kind of
error

    >>> class BozoError(Exception):
    ...     pass
    ...
    >>> class BadSeq:
    ...     def __getitem__(self, i):
    ...         if i >= 0 and i < 3:
    ...             return i
    ...         elif i == 3:
    ...             raise BozoError
    ...         else:
    ...             raise IndexError
    ...

Trigger code while not expecting an IndexError (unpack sequence too long, wrong
error)

    >>> a, b, c, d, e = BadSeq()
    Traceback (most recent call last):
      ...
    test.test_unpack.BozoError

Trigger code while expecting an IndexError (unpack sequence too short, wrong
error)

    >>> a, b, c = BadSeq()
    Traceback (most recent call last):
      ...
    test.test_unpack.BozoError

Allow unpacking empty iterables

    >>> () = []
    >>> [] = ()
    >>> [] = []
    >>> () = ()

Unpacking non-iterables should raise TypeError

    >>> () = 42
    Traceback (most recent call last):
      ...
    TypeError: cannot unpack non-iterable int object

Unpacking to an empty iterable should raise ValueError

    >>> () = [42]
    Traceback (most recent call last):
      ...
    ValueError: too many values to unpack (expected 0, got 1)

Unpacking a larger iterable should raise ValuleError, but it
should not entirely consume the iterable

    >>> it = iter(range(100))
    >>> x, y, z = it
    Traceback (most recent call last):
      ...
    ValueError: too many values to unpack (expected 3)
    >>> next(it)
    4

Unpacking unbalanced dict

    >>> d = {4: 'four', 5: 'five', 6: 'six', 7: 'seven'}
    >>> a, b, c = d
    Traceback (most recent call last):
      ...
    ValueError: too many values to unpack (expected 3, got 4)

Ensure that custom `__len__()` is NOT called when showing the error message

    >>> class LengthTooLong:
    ...     def __len__(self):
    ...         return 5
    ...     def __getitem__(self, i):
    ...         return i*2
    ...
    >>> x, y, z = LengthTooLong()
    Traceback (most recent call last):
      ...
    ValueError: too many values to unpack (expected 3)

For evil cases like these as well, no actual count to be shown

    >>> class BadLength:
    ...     def __len__(self):
    ...         return 1
    ...     def __getitem__(self, i):
    ...         return i*2
    ...
    >>> x, y, z = BadLength()
    Traceback (most recent call last):
      ...
    ValueError: too many values to unpack (expected 3)
"""

__test__ = {'doctests' : doctests}

def load_tests(loader, tests, pattern):
    tests.addTest(doctest.DocTestSuite())
    return tests


class TestCornerCases(unittest.TestCase):
    def test_extended_oparg_not_ignored(self):
        # https://github.com/python/cpython/issues/91625
        target = "(" + "y,"*400 + ")"
        code = f"""def unpack_400(x):
            {target} = x
            return y
        """
        ns = {}
        exec(code, ns)
        unpack_400 = ns["unpack_400"]
        # Warm up the function for quickening (PEP 659)
        for _ in range(30):
            y = unpack_400(range(400))
            self.assertEqual(y, 399)


class Logged:
    "A value whose release records the locals of the frame that holds it"
    def __init__(self, name, log, frame_locals):
        self.name = name
        self.log = log
        self.frame_locals = frame_locals

    def __del__(self):
        self.log.append((self.name, self.frame_locals()))


class FailingIterator:
    def __init__(self, fail_at, size):
        self.fail_at = fail_at
        self.size = size
        self.i = 0

    def __iter__(self):
        return self

    def __next__(self):
        self.i += 1
        if self.i == self.fail_at:
            raise RuntimeError("boom")
        if self.i > self.size:
            raise StopIteration
        return self.i


class TestUnpackToLocals(unittest.TestCase):
    # Targets that are consecutive locals receive the elements directly; the
    # observable behaviour must be that of the stores they replace.

    def test_failing_next_leaves_locals(self):
        def three(it):
            a = b = c = "old"
            try:
                a, b, c = it
            except RuntimeError:
                pass
            return a, b, c

        def twelve(it):
            a = b = c = d = e = f = g = h = i = j = k = l = "old"
            try:
                a, b, c, d, e, f, g, h, i, j, k, l = it
            except RuntimeError:
                pass
            return a, b, c, d, e, f, g, h, i, j, k, l

        for fail_at in (1, 2, 3):
            self.assertEqual(three(FailingIterator(fail_at, 3)), ("old",) * 3)
        for fail_at in (1, 6, 12):
            self.assertEqual(twelve(FailingIterator(fail_at, 12)), ("old",) * 12)

    def test_failing_next_releases_values(self):
        import weakref

        class Obj:
            pass

        class Iterator(FailingIterator):
            def __next__(self):
                value = super().__next__()
                obj = Obj()
                refs.append(weakref.ref(obj))
                return obj

        def f(it):
            a = b = c = None
            try:
                a, b, c = it
            except RuntimeError:
                pass

        refs = []
        f(Iterator(3, 3))
        support.gc_collect()
        self.assertEqual([r() for r in refs], [None] * len(refs))
        self.assertEqual(len(refs), 2)

    def test_wrong_length_leaves_locals(self):
        def f(seq):
            a = b = "old"
            try:
                a, b = seq
            except ValueError:
                pass
            return a, b

        for seq in ((1, 2, 3), (1,), [1, 2, 3], iter([1]), iter([1, 2, 3])):
            for _ in range(3):
                self.assertEqual(f(seq), ("old", "old"))
        self.assertEqual(f((1, 2)), (1, 2))
        self.assertEqual(f([3, 4]), (3, 4))

    def test_release_order_of_old_values(self):
        # The old value of each target is released before the next target
        # is stored, as a sequence of STORE_FASTs does.
        log = []
        def frame_locals():
            frame = sys._getframe(2)
            return {k: getattr(v, "name", v)
                    for k, v in frame.f_locals.items() if k in "abc"}

        def f(seq):
            a = Logged("a", log, frame_locals)
            b = Logged("b", log, frame_locals)
            c = Logged("c", log, frame_locals)
            a, b, c = seq
            return a, b, c

        for seq in ((1, 2, 3), [1, 2, 3], iter((1, 2, 3))):
            del log[:]
            f(seq)
            self.assertEqual(log, [
                ("a", {"a": 1, "b": "b", "c": "c"}),
                ("b", {"a": 1, "b": 2, "c": "c"}),
                ("c", {"a": 1, "b": 2, "c": 3}),
            ])

    def test_source_is_a_target(self):
        def f(a):
            a, b = a
            return a, b

        def g(a):
            b, a = a
            return a, b

        for _ in range(3):
            self.assertEqual(f((1, 2)), (1, 2))
            self.assertEqual(f([3, 4]), (3, 4))
            self.assertEqual(g((1, 2)), (2, 1))

    def test_star_to_locals(self):
        def f(seq):
            a, *b, c = seq
            return a, b, c

        for _ in range(3):
            self.assertEqual(f(range(5)), (0, [1, 2, 3], 4))
            self.assertEqual(f((1, 2)), (1, [], 2))
        with self.assertRaises(ValueError):
            f((1,))

    def test_many_elements(self):
        names = [f"v{i}" for i in range(30)]
        ns = {}
        exec(f"def f(x):\n    {', '.join(names)} = x\n"
             f"    return ({', '.join(names)})\n", ns)
        for _ in range(3):
            self.assertEqual(ns["f"](range(30)), tuple(range(30)))
            self.assertEqual(ns["f"](tuple(range(30))), tuple(range(30)))
            self.assertEqual(ns["f"](list(range(30))), tuple(range(30)))


if __name__ == "__main__":
    unittest.main()
