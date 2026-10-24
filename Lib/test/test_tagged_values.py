"""Tests for the inline int and float encoding of interpreter stack values."""

import dis
import gc
import math
import operator
import struct
import sys
import traceback
import unittest
from test.support import import_helper, script_helper

_testinternalcapi = import_helper.import_module("_testinternalcapi")
roundtrip = getattr(_testinternalcapi, "stackref_box_roundtrip", None)
# Absent from builds without inline values, false under PYTHON_TAGGED_VALUES=0.
TAGGED_VALUES = getattr(_testinternalcapi, "TAGGED_VALUES", None)
THRESHOLD = _testinternalcapi.SPECIALIZATION_THRESHOLD


@unittest.skipIf(roundtrip is None, "build has no inline stack values")
class TaggedValuesTest(unittest.TestCase):

    def check(self, value):
        boxed = roundtrip(value)
        self.assertIs(type(boxed), type(value))
        self.assertEqual(boxed, value)
        if isinstance(value, float):
            self.assertEqual(math.copysign(1.0, boxed),
                             math.copysign(1.0, value))

    def test_int32(self):
        for value in (0, 1, -1, 2, 2**31 - 1, -2**31, 1 << 16, -(1 << 16)):
            with self.subTest(value=value):
                self.check(value)

    def test_int_out_of_range(self):
        for value in (2**31, -2**31 - 1, 2**63, -2**100):
            with self.subTest(value=value):
                self.assertRaises(OverflowError, roundtrip, value)

    def test_float(self):
        values = (0.0, -0.0, 1.5, -1.5, math.inf, -math.inf, 5e-324,
                  -5e-324, 1.7976931348623157e308, -1.7976931348623157e308,
                  float(2**31))
        for value in values:
            with self.subTest(value=value):
                self.check(value)

    def test_nan_refused(self):
        # Offset by 2**49, these would alias the inline int 1, a pointer
        # and an index.
        nans = [struct.unpack("<d", struct.pack("<Q", bits))[0]
                for bits in (0xFFFC_0000_0000_0001, 0xFFFE_0000_0000_1000,
                             0xFFFF_FFFF_FFFF_FFFF)]
        for value in (math.nan, -math.nan, *nans):
            with self.subTest(value=value):
                self.assertRaises(ValueError, roundtrip, value)

    def test_box_memory_error(self):
        _testcapi = import_helper.import_module("_testcapi")
        # Values that are not cached, so boxing allocates.
        for value in (2**31 - 1, -2**31, 1.5):
            with self.subTest(value=value):
                gc.collect()  # Empty the free lists.
                _testcapi.set_nomemory(0)
                try:
                    roundtrip(value)
                except MemoryError:
                    raised = True
                else:
                    raised = False
                finally:
                    _testcapi.remove_mem_hooks()
                self.assertTrue(raised)

    def test_other_types_refused(self):
        class Int(int):
            pass

        for value in (True, Int(1), 1j, "1", None):
            with self.subTest(value=value):
                self.assertRaises(TypeError, roundtrip, value)


def distinct(value):
    """An object equal to value but not created by the same operation."""
    return type(value)(str(value))


@unittest.skipIf(TAGGED_VALUES is None, "build has no inline stack values")
class IdentityTest(unittest.TestCase):
    """An int that fits in int32 and a float other than NaN have value
    identity, whether held inline or boxed, in any number of boxes."""

    def check_same(self, a, b):
        self.assertIs(a, b)
        self.assertTrue(a is b)
        self.assertFalse(a is not b)
        self.assertEqual(id(a), id(b))
        self.assertTrue(operator.is_(a, b))
        self.assertFalse(operator.is_not(a, b))

    def check_different(self, a, b):
        self.assertIsNot(a, b)
        self.assertFalse(a is b)
        self.assertTrue(a is not b)
        self.assertNotEqual(id(a), id(b))
        self.assertFalse(operator.is_(a, b))
        self.assertTrue(operator.is_not(a, b))

    def test_int32(self):
        for value in (0, 1, -1, 256, 257, 70000, 2**30, 2**31 - 1, -2**31):
            with self.subTest(value=value):
                self.check_same(value, distinct(value))
                self.check_same(distinct(value), distinct(value))

    def test_int_outside_int32(self):
        for value in (2**31, -2**31 - 1, 2**63, -2**100):
            with self.subTest(value=value):
                self.check_different(distinct(value), distinct(value))
                x = distinct(value)
                self.check_same(x, x)

    def test_float(self):
        for value in (0.0, -0.0, 1.5, -1e300, math.inf, -math.inf, 5e-324):
            with self.subTest(value=value):
                self.check_same(value, distinct(value))

    def test_zero_signs_differ(self):
        self.check_different(distinct(0.0), distinct(-0.0))
        self.check_same(-0, 0)

    def test_nan(self):
        nan = float("nan")
        self.check_same(nan, nan)
        self.check_different(nan, float("nan"))

    def test_bool_is_not_int(self):
        self.check_different(True, distinct(1))
        self.check_different(False, distinct(0))
        self.check_same(True, bool(distinct(1)))

    def test_subclass(self):
        class Int(int):
            pass

        class Float(float):
            pass

        self.check_different(Int(5), 5)
        self.check_different(Int(5), Int(5))
        self.check_different(Float(1.5), 1.5)
        x = Int(5)
        self.check_same(x, x)

    def test_list_element(self):
        def f(x):
            return [x * 1]
        for _ in range(THRESHOLD):
            x = 70000
            lst = f(x)
            self.assertIs(lst[0], x)
            self.assertEqual(id(lst[0]), id(x))

    def test_id_stable(self):
        def f(a, b):
            total = a + b
            first = id(total)
            boxes = [total, (total,), {total: total}]
            return total, first, boxes
        for _ in range(THRESHOLD):
            total, first, boxes = f(70000, 1)
            self.assertEqual(id(total), first)
            self.assertEqual(id(boxes[0]), first)
            self.assertEqual(id(boxes[1][0]), first)
            [(key, value)] = boxes[2].items()
            self.assertEqual(id(key), first)
            self.assertEqual(id(value), first)

    def test_id_differs_from_objects(self):
        objects = [object() for _ in range(100)]
        ids = {id(o) for o in objects}
        for value in range(-50, 50):
            self.assertNotIn(id(distinct(value * 1000)), ids)
        self.assertEqual(len({id(distinct(v)) for v in range(-500, 500)}), 1000)


def warm(func, *args, **kwargs):
    """Call func until its instructions are specialized; return the result."""
    for _ in range(THRESHOLD):
        result = func(*args, **kwargs)
    return result


def opnames(func):
    return {i.opname for i in dis.get_instructions(func, adaptive=True)}


def add3(a, b, c):
    return a + b + c


def make_inline(value):
    """Return value, an int in the int32 range, as an inline int: the result
    of specialized arithmetic on compact ints, which a Python caller receives
    as it is."""
    third = value // 3
    return add3(third, third, value - 2 * third)


@unittest.skipUnless(TAGGED_VALUES, "inline values are off")
class InlineIntTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        warm(add3, 1, 2, 3)

    def test_quickened(self):
        def f(n):
            total = 0
            for i in range(n):
                total = total + i * 3 - 70000
            return total
        self.assertEqual(warm(f, 10), sum(i * 3 - 70000 for i in range(10)))
        names = opnames(f)
        for name in ("LOAD_SMALL_INT_INLINE", "LOAD_CONST_INLINE_INT",
                     "BINARY_OP_ADD_INT", "BINARY_OP_SUBTRACT_INT",
                     "BINARY_OP_MULTIPLY_INT", "FOR_ITER_RANGE"):
            self.assertIn(name, names)
        self.assertIn("BINARY_OP_ADD_INT", opnames(add3))

    def test_overflow(self):
        def add(a, b):
            return a + b
        def sub(a, b):
            return a - b
        def mul(a, b):
            return a * b
        cases = [
            (add, 2**31 - 1, 1, 2**31),
            (add, 2**31 - 2, 1, 2**31 - 1),
            (add, -2**31, -1, -2**31 - 1),
            (sub, -2**31, 1, -2**31 - 1),
            (sub, -2**31 + 1, 1, -2**31),
            (sub, 2**31 - 1, -1, 2**31),
            (mul, 65536, 32768, 2**31),
            (mul, -65536, 32768, -2**31),
            (mul, -2**31, -1, 2**31),
            (mul, 46341, 46341, 46341**2),
            (mul, 46340, 46340, 46340**2),
            (mul, 2**31 - 1, 2**31 - 1, (2**31 - 1)**2),
        ]
        for func in (add, sub, mul):
            warm(func, 1, 2)
        for func, a, b, expected in cases:
            with self.subTest(func=func.__name__, a=a, b=b):
                result = func(make_inline(a), make_inline(b))
                self.assertEqual(result, expected)
                self.assertIs(type(result), int)
                self.assertEqual(hash(result), hash(expected))
                self.assertEqual(str(result), str(expected))
                # A boxed operand of the same value gives the same result.
                self.assertEqual(func(make_inline(a), b), expected)

    def test_negative(self):
        def neg(a):
            return -a
        warm(neg, 1)
        self.assertIs(neg(make_inline(0)), 0)
        self.assertEqual(neg(make_inline(70000)), -70000)
        self.assertEqual(neg(make_inline(-2**31)), 2**31)
        self.assertEqual(neg(make_inline(2**31 - 1)), -2**31 + 1)

    def test_compare_and_bool(self):
        def f(a, b):
            return a < b, a == b, a >= b, bool(a), not b
        warm(f, 1, 2)
        for a, b in ((70000, 70001), (0, -5), (2**31 - 1, -2**31)):
            with self.subTest(a=a, b=b):
                self.assertEqual(f(make_inline(a), make_inline(b)), f(a, b))
                self.assertEqual(f(make_inline(a), b), f(a, b))

    def test_subscript(self):
        lst = list(range(100))
        tup = tuple(lst)
        s = "abcdefghij"
        u = "\u0101" * 10
        def get(seq, i):
            return seq[i]
        def store(seq, i, v):
            seq[i] = v
        for seq in (lst, tup, s, u):
            with self.subTest(type=type(seq)):
                warm(get, seq, 1)
                self.assertEqual(get(seq, make_inline(5)), seq[5])
                self.assertEqual(get(seq, make_inline(-1)), seq[-1])
                self.assertRaises(IndexError, get, seq, make_inline(2**31 - 1))
                self.assertRaises(IndexError, get, seq, make_inline(-2**31))
        warm(store, lst, 1, 1)
        store(lst, make_inline(7), make_inline(70000))
        self.assertEqual(lst[7], 70000)
        self.assertRaises(IndexError, store, lst, make_inline(2**31 - 1), 1)

    def test_hash(self):
        d = {}
        for value in (0, 1, -1, 70000, -70000, 2**31 - 1, -2**31):
            with self.subTest(value=value):
                x = make_inline(value)
                self.assertEqual(hash(x), hash(distinct(value)))
                d[x] = value
                self.assertEqual(d[distinct(value)], value)

    def test_getrefcount(self):
        def f():
            x = 70000
            return sys.getrefcount(x)
        self.assertGreater(warm(f), 0)

    def test_frame_locals(self):
        def f():
            x = 70000
            y = x + 1
            frame = sys._getframe()
            seen = dict(frame.f_locals), locals()
            frame.f_locals["x"] = 5
            return seen, x, frame.f_locals["y"]
        (proxy, snapshot), x, y = warm(f)
        self.assertEqual(proxy["x"], 70000)
        self.assertEqual(snapshot["y"], 70001)
        self.assertEqual(x, 5)
        self.assertEqual(y, 70001)

    def test_generator(self):
        def gen():
            x = 70000
            yield x
            x += 1
            yield x
        g = gen()
        self.assertEqual(next(g), 70000)
        self.assertEqual(g.gi_frame.f_locals["x"], 70000)
        self.assertEqual(list(g), [70001])
        self.assertEqual(warm(lambda: list(gen())), [70000, 70001])

    def test_traceback_locals(self):
        def f():
            x = 70000
            1 / 0
        try:
            f()
        except ZeroDivisionError as e:
            tb = traceback.TracebackException.from_exception(
                e, capture_locals=True)
        self.assertEqual(tb.stack[-1].locals["x"], "70000")

    def test_calls(self):
        def args(*a, **k):
            return a, k
        def outer(x):
            def inner():
                return x
            return inner()
        class A:
            def method(self):
                return super().__repr__()
        self.assertEqual(warm(args, 1, 70000, k=70001), ((1, 70000), {"k": 70001}))
        self.assertEqual(warm(outer, 70000), 70000)
        self.assertEqual(warm(lambda: int.bit_length(70000)), 17)
        self.assertEqual(warm(lambda: (70000).bit_length()), 17)
        self.assertEqual(warm(lambda: abs(-70000)), 70000)
        self.assertEqual(warm(lambda: type(70000)), int)
        with self.assertRaisesRegex(TypeError, "obj .*instance of int"):
            A.method(70000)
        with self.assertRaisesRegex(TypeError, "'int' object is not callable"):
            x = 70000
            x()

    def test_containers(self):
        def f(a, b):
            return {a, b}, [a, b][a - 70000 : b - 70000], (a, b), {a: b}
        self.assertEqual(warm(f, 70000, 70001),
                         ({70000, 70001}, [70000], (70000, 70001), {70000: 70001}))

    def test_errors_name_the_type(self):
        x = 70000
        with self.assertRaisesRegex(TypeError, "must derive from BaseException"):
            raise x
        with self.assertRaisesRegex(TypeError, "'int' object is not iterable"):
            for _ in x:
                pass
        with self.assertRaisesRegex(TypeError, "context manager"):
            with x:
                pass

    def test_box_memory_error(self):
        _testcapi = import_helper.import_module("_testcapi")
        def f(x):
            return id(x)
        warm(f, 70000)
        x = 70000 + 1
        gc.collect()
        _testcapi.set_nomemory(0)
        try:
            f(x)
        except MemoryError:
            raised = True
        else:
            raised = False
        finally:
            _testcapi.remove_mem_hooks()
        self.assertTrue(raised)
        self.assertEqual(f(x), id(70001))


@unittest.skipIf(TAGGED_VALUES is None, "build has no inline stack values")
class SwitchTest(unittest.TestCase):

    def test_tagged_values_off(self):
        code = """if 1:
            import dis, _testinternalcapi
            assert not _testinternalcapi.TAGGED_VALUES
            def f(n):
                total = 0
                for i in range(n):
                    total += i * 3
                return total
            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
                assert f(10) == 135
            names = {i.opname for i in dis.get_instructions(f, adaptive=True)}
            assert "LOAD_SMALL_INT" in names, names
            assert "LOAD_SMALL_INT_INLINE" not in names, names
            assert "FOR_ITER_RANGE" not in names, names
            assert 70000 is int("70000")
        """
        script_helper.assert_python_ok("-c", code, PYTHON_TAGGED_VALUES="0")


if __name__ == "__main__":
    unittest.main()
