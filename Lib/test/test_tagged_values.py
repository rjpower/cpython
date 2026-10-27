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
roundtrip = getattr(_testinternalcapi, "value_box_roundtrip", None)
inline_locals = getattr(_testinternalcapi, "inline_locals", None)
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


def fadd(a, b):
    return a + b


def make_inline_float(value):
    """Return value, a float other than NaN, as an inline float: the result
    of specialized float arithmetic."""
    return fadd(value, 0.0) if value != 0.0 else fadd(value, -0.0)


def float_bits(value):
    return struct.unpack("<Q", struct.pack("<d", value))[0]


def bits_float(bits):
    return struct.unpack("<d", struct.pack("<Q", bits))[0]


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


def float_ops(a, b):
    return a + b, a - b, a * b, a / b


def int_ops(a, b):
    return a // b, a % b, a & b, a | b, a ^ b


def shifts(a, b):
    return a << b, a >> b


def reference(op, a, b):
    """op(a, b) computed by the unspecialized C implementation."""
    return op(int(str(a)) if type(a) is int else float(repr(a)),
              int(str(b)) if type(b) is int else float(repr(b)))


class InlineFloatTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        warm(add3, 1, 2, 3)
        warm(fadd, 1.5, 2.5)
        warm(float_ops, 1.5, 2.5)

    def test_quickened(self):
        def f(n):
            total = 0.5
            for i in range(n):
                total = total * 1.5 - i / 2.0 + 0.25
            return total, total < 1e300
        warm(f, 3)
        names = opnames(f)
        for name in ("LOAD_CONST_INLINE_FLOAT", "BINARY_OP_ADD_FLOAT",
                     "BINARY_OP_SUBTRACT_FLOAT", "BINARY_OP_MULTIPLY_FLOAT",
                     "BINARY_OP_TRUEDIV_FLOAT", "COMPARE_OP_FLOAT"):
            self.assertIn(name, names)
        self.assertNotIn("BINARY_OP_EXTEND", names)
        self.assertEqual(f(3), (1.125, True))

    def test_results_are_inline(self):
        def f(a, b, i):
            s = a + b
            d = a - b
            p = a * i
            q = i / b
            n = -a
            c = 1.5
            frame = sys._getframe()
            return inline_locals(frame), (s, d, p, q, n, c)
        names, values = warm(f, 1.5, 2.5, 3)
        self.assertLessEqual({"s", "d", "p", "q", "n", "c"}, names)
        self.assertEqual(values, (4.0, -1.0, 4.5, 1.2, -1.5, 1.5))
        names, _ = f(make_inline_float(1.5), make_inline_float(2.5), make_inline(3))
        self.assertLessEqual({"a", "b", "i", "s", "d", "p", "q", "n", "c"}, names)

    def test_nan_never_inline(self):
        inf = math.inf
        def f(a, b, i):
            sub = a - a
            mul = i * a
            div = a / b
            frame = sys._getframe()
            return inline_locals(frame), sub, mul, div
        warm(f, 1.5, 2.5, 3)
        names, sub, mul, div = f(inf, inf, 0)
        self.assertEqual(names & {"sub", "mul", "div"}, set())
        for value in (sub, mul, div):
            with self.subTest(value=value):
                self.assertTrue(math.isnan(value))
                self.assertIs(type(value), float)
                # NaN keeps object identity: its id is an address.
                self.assertLess(id(value), 2**48)
        a, b = float_ops(inf, inf)[1], float_ops(inf, inf)[1]
        self.assertIsNot(a, b)
        self.assertIs(a, a)
        self.assertIn(a, [a])
        self.assertNotIn(a, [b])
        d = {a: 1}
        self.assertIn(a, d)
        self.assertNotIn(b, d)
        self.assertEqual(d[a], 1)
        self.assertEqual(len({a, b}), 2)
        self.assertTrue(math.isnan(f(0.0, inf, 0)[3] * inf))

    def test_nan_operand(self):
        nan = float("nan")
        for result in float_ops(nan, 1.5) + float_ops(1.5, nan):
            self.assertTrue(math.isnan(result))
        def mixed(a, i):
            return a + i, i * a
        warm(mixed, 1.5, 2)
        for result in mixed(nan, 2):
            self.assertTrue(math.isnan(result))

    def test_struct_nan_payloads(self):
        payloads = (0x7FF8_0000_0000_0001, 0xFFF8_0000_DEAD_BEEF,
                    0x7FFF_FFFF_FFFF_FFFF, 0xFFFC_0000_0000_0001,
                    0x7FF0_0000_0000_0001)
        def keep(x):
            y = x
            values = [y]
            return values[0], y, -y
        warm(keep, 1.5)
        for bits in payloads:
            with self.subTest(bits=hex(bits)):
                nan = bits_float(bits)
                kept, local, negated = keep(nan)
                self.assertEqual(float_bits(kept), float_bits(nan))
                self.assertEqual(float_bits(local), float_bits(nan))
                self.assertEqual(float_bits(negated), float_bits(-nan))
                self.assertIs(kept, nan)
                # Arithmetic on a NaN gives the bits the unspecialized
                # operation gives.
                for i, op in enumerate((operator.add, operator.sub,
                                        operator.mul, operator.truediv)):
                    self.assertEqual(float_bits(float_ops(nan, 1.5)[i]),
                                     float_bits(op(nan, 1.5)))
                    self.assertEqual(float_bits(float_ops(1.5, nan)[i]),
                                     float_bits(op(1.5, nan)))

    def test_negative_zero(self):
        def f(a):
            z = -0.0
            return z, a * -1.0, a - 0.0, -a, a + 0.0, z + 0.0, a / -1
        warm(f, 1.5)
        z, prod, diff, neg, total, pos, quot = f(make_inline_float(0.0))
        for value in (z, prod, neg, quot):
            self.assertEqual(math.copysign(1.0, value), -1.0)
        for value in (diff, total, pos):
            self.assertEqual(math.copysign(1.0, value), 1.0)
        self.assertEqual(math.copysign(1.0, f(-0.0)[2]), -1.0)
        self.assertIsNot(z, 0.0)
        self.assertIs(z, -0.0)
        self.assertEqual(z, 0.0)

    def test_signed_zero_compare_and_bool(self):
        def f(a, b):
            return a < b, a == b, a != b, a >= b, bool(a), not b
        warm(f, 1.5, 2.5)
        nan = float("nan")
        for a, b in ((0.0, -0.0), (-0.0, 0.0), (1.5, nan), (nan, nan),
                     (math.inf, -math.inf), (5e-324, 0.0)):
            with self.subTest(a=a, b=b):
                self.assertEqual(f(a, b), reference(f, a, b))
                self.assertEqual(f(make_inline_float(a) if a == a else a, b),
                                 reference(f, a, b))

    def test_to_bool(self):
        def f(x):
            if x:
                return True
            return False
        for value in (0.0, -0.0, 1.5, -5e-324, math.inf):
            with self.subTest(value=value):
                self.assertEqual(warm(f, make_inline_float(value)), value != 0.0)

    def test_mixed_int_float(self):
        cases = [(3, 2.0), (3.0, 2), (-2**31, 0.5), (2**31 - 1, -1.5),
                 (0.25, 2**31 - 1), (1e308, 10), (7, 3.5), (1, 3.0)]
        for a, b in cases:
            with self.subTest(a=a, b=b):
                expected = (operator.add(a, b), operator.sub(a, b),
                            operator.mul(a, b), operator.truediv(a, b))
                self.assertEqual(float_ops(a, b), expected)
                inline_a = make_inline(a) if type(a) is int else make_inline_float(a)
                inline_b = make_inline(b) if type(b) is int else make_inline_float(b)
                self.assertEqual(float_ops(inline_a, inline_b), expected)
                self.assertTrue(all(type(r) is float for r in float_ops(a, b)))
        # Ints that are neither compact nor inline take the slow path.
        self.assertEqual(float_ops(2**40, 1.5), (2**40 + 1.5, 2**40 - 1.5,
                                                 2**40 * 1.5, 2**40 / 1.5))
        self.assertEqual(float_ops(1.5, 2**62)[3], 1.5 / 2**62)
        self.assertEqual(float_ops(True, 1.5), (2.5, -0.5, 1.5, 1 / 1.5))
        self.assertRaises(OverflowError, float_ops, 10**400, 1.5)

    def test_division_by_zero(self):
        def div(a, b):
            return a / b
        warm(div, 1.5, 2.5)
        for a, b in ((1.0, 0.0), (1.0, -0.0), (1.0, 0), (1, 0.0), (0.0, 0.0),
                     (math.nan, 0.0), (1, 0)):
            with self.subTest(a=a, b=b):
                with self.assertRaisesRegex(ZeroDivisionError, "division by zero"):
                    div(a, b)
        with self.assertRaises(ZeroDivisionError):
            1 / 0
        with self.assertRaises(ZeroDivisionError):
            1.0 / 0.0
        self.assertIn("BINARY_OP_TRUEDIV_FLOAT", opnames(div))
        self.assertEqual(div(1e308, 1e-308), math.inf)

    def test_float_subclass(self):
        class Float(float):
            pass

        class Custom(float):
            def __add__(self, other):
                return "add"
            def __rsub__(self, other):
                return "rsub"
            def __lt__(self, other):
                return "lt"

        def f(a, b):
            return a + b, a - b, a < b
        warm(f, 1.5, 2.5)
        result = f(Float(1.5), 2.5)
        self.assertEqual(result, (4.0, -1.0, True))
        self.assertIs(type(result[0]), float)
        self.assertEqual(f(Custom(1.5), 2.5)[0], "add")
        self.assertEqual(f(1.5, Custom(2.5))[1], "rsub")
        self.assertEqual(f(Custom(1.5), 2.5)[2], "lt")
        x = Float(1.5)
        self.assertIsNot(x, 1.5)
        self.assertIs(type(warm(lambda: -x)), float)

    def test_errors_name_the_type(self):
        x = make_inline_float(1.5)
        with self.assertRaisesRegex(TypeError, "'float' object is not iterable"):
            for _ in x:
                pass
        with self.assertRaisesRegex(TypeError, "'float' object is not callable"):
            x()
        self.assertEqual(warm(math.sqrt, x), math.sqrt(1.5))
        self.assertEqual(warm(lambda: x ** 2), 2.25)
        self.assertEqual(warm(lambda: [x, x][1]), 1.5)
        self.assertEqual(warm(lambda: f"{x}"), "1.5")


class InlineIntOpsTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        warm(add3, 1, 2, 3)
        warm(int_ops, 7, 2)
        warm(shifts, 7, 2)

    def check_ops(self, func, ops, a, b):
        expected = tuple(op(a, b) for op in ops)
        self.assertEqual(func(a, b), expected)
        self.assertEqual(func(make_inline(a), make_inline(b)), expected)
        self.assertEqual(func(make_inline(a), b), expected)

    def test_quickened(self):
        names = opnames(int_ops) | opnames(shifts)
        for name in ("BINARY_OP_FLOOR_DIVIDE_INT", "BINARY_OP_REMAINDER_INT",
                     "BINARY_OP_AND_INT", "BINARY_OP_OR_INT", "BINARY_OP_XOR_INT",
                     "BINARY_OP_LSHIFT_INT", "BINARY_OP_RSHIFT_INT"):
            self.assertIn(name, names)
        def f(a, b):
            q = a // b
            m = a % b
            x = a ^ b
            sh = a << 3
            frame = sys._getframe()
            return inline_locals(frame)
        self.assertLessEqual({"q", "m", "x", "sh"}, warm(f, 70000, 3))

    def test_floor_divide_and_modulo(self):
        ops = (operator.floordiv, operator.mod, operator.and_,
               operator.or_, operator.xor)
        values = (0, 1, -1, 2, -2, 3, -3, 7, -7, 2**30, -2**30, 2**31 - 1,
                  -2**31, 2**31 - 2, -2**31 + 1, 46341)
        for a in values:
            for b in values:
                if b == 0:
                    continue
                with self.subTest(a=a, b=b):
                    self.check_ops(int_ops, ops, a, b)
                    q, m = int_ops(make_inline(a), make_inline(b))[:2]
                    self.assertEqual(q * b + m, a)
        self.assertEqual(int_ops(make_inline(-2**31), make_inline(-1))[:2], (2**31, 0))
        self.assertIs(type(int_ops(make_inline(-2**31), -1)[0]), int)

    def test_division_by_zero(self):
        for a in (0, 7, -7, make_inline(70000)):
            with self.subTest(a=a):
                with self.assertRaisesRegex(ZeroDivisionError, "division by zero"):
                    int_ops(a, 0)
                with self.assertRaises(ZeroDivisionError):
                    int_ops(a, make_inline(0))
        self.assertIn("BINARY_OP_FLOOR_DIVIDE_INT", opnames(int_ops))

    def test_shifts(self):
        ops = (operator.lshift, operator.rshift)
        cases = [(1, 0), (1, 30), (1, 31), (-1, 31), (1, 32), (-1, 32),
                 (3, 100), (-3, 100), (0, 100), (0, 2**31 - 1),
                 (2**31 - 1, 1), (-2**31, 1), (-2**31, 31), (2**31 - 1, 31),
                 (-2**31, 63), (5, 2**31 - 1), (-5, 2**31 - 1), (12345, 17)]
        for a, b in cases:
            if a and b > 64:
                expected = (a << b, a >> b)
                self.assertEqual(shifts(a, b), expected)
                continue
            with self.subTest(a=a, b=b):
                self.check_ops(shifts, ops, a, b)
        for a, b in ((1, -1), (make_inline(70000), make_inline(-2**31)), (0, -5)):
            with self.subTest(a=a, b=b):
                with self.assertRaisesRegex(ValueError, "negative shift count"):
                    shifts(a, b)

    def test_bool_and_subclass(self):
        class Int(int):
            def __floordiv__(self, other):
                return "floordiv"
        self.assertEqual(int_ops(True, 2), (0, 1, 0, 3, 3))
        self.assertEqual(int_ops(Int(7), 2)[0], "floordiv")
        self.assertEqual(shifts(True, 3), (8, 0))


class Sliceable:
    def __getitem__(self, key):
        return key

    def __setitem__(self, key, value):
        self.stored = key, value


class InlineSliceTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        warm(add3, 1, 2, 3)

    def test_binary_slice(self):
        def f(seq, i, j):
            return seq[i:j], seq[i:], seq[:j]
        warm(f, [1, 2, 3], 1, 2)
        seqs = (list(range(10)), tuple(range(10)), "abcdefghij", "\u0101" * 10,
                b"abcdefghij", bytearray(b"abcdefghij"))
        bounds = (0, 1, 3, -1, -3, 10, 11, -11, 2**31 - 1, -2**31)
        for seq in seqs:
            for i in bounds:
                for j in bounds:
                    with self.subTest(seq=seq, i=i, j=j):
                        expected = seq[slice(i, j)], seq[slice(i, None)], seq[slice(None, j)]
                        self.assertEqual(f(seq, make_inline(i), make_inline(j)), expected)
                        self.assertEqual(f(seq, i, j), expected)
        t = tuple(range(5))
        self.assertIs(f(t, make_inline(0), make_inline(5))[0], t)
        self.assertEqual(f(list(range(5)), make_inline(1), None)[0], [1, 2, 3, 4])

    def test_getitem_gets_slice(self):
        def f(obj, i, j):
            return obj[i:j], obj[i:], obj[:j]
        warm(f, [1, 2, 3], 1, 2)
        a, b, c = f(Sliceable(), make_inline(70000), make_inline(-3))
        self.assertEqual((a, b, c), (slice(70000, -3), slice(70000, None),
                                     slice(None, -3)))
        self.assertIs(type(a.start), int)
        self.assertEqual(a.start, 70000)
        self.assertEqual(f(Sliceable(), 1.5, make_inline(3))[0], slice(1.5, 3))
        with self.assertRaisesRegex(TypeError, "slice indices must be integers"):
            f([1, 2, 3], 1.5, 2)
        with self.assertRaisesRegex(TypeError, "slice indices must be integers"):
            f([1, 2, 3], make_inline(1), 2.5)

    def test_store_slice(self):
        def store(seq, i, j, value):
            seq[i:j] = value
            return seq
        warm(store, [1, 2, 3], 1, 2, [5])
        bounds = (0, 1, 3, -1, -4, 4, 2**31 - 1, -2**31)
        for i in bounds:
            for j in bounds:
                for value in ([], [7, 8], (9,), "ab", iter([1, 2, 3])):
                    with self.subTest(i=i, j=j, value=value):
                        expected = list(range(4))
                        expected[slice(i, j)] = list(value) if not isinstance(value, list) else value
                        if isinstance(value, type(iter([]))):
                            continue
                        self.assertEqual(store(list(range(4)), make_inline(i),
                                               make_inline(j), value), expected)
        lst = [1, 2, 3]
        self.assertEqual(store(lst, make_inline(1), make_inline(2), lst), [1, 1, 2, 3, 3])
        self.assertEqual(store(list(range(5)), make_inline(4), make_inline(1), [9]),
                         [0, 1, 2, 3, 9, 4])
        with self.assertRaisesRegex(TypeError, "must assign iterable"):
            store([1, 2, 3], make_inline(1), make_inline(2), 5)
        with self.assertRaisesRegex(TypeError, "must assign iterable"):
            store([1, 2, 3], make_inline(1), make_inline(2), make_inline(70000))
        b = bytearray(b"abcd")
        self.assertEqual(store(b, make_inline(1), make_inline(3), b"XYZ"), bytearray(b"aXYZd"))
        obj = store(Sliceable(), make_inline(70000), None, make_inline(70001))
        self.assertEqual(obj.stored, (slice(70000, None), 70001))
        with self.assertRaises(TypeError):
            store((1, 2, 3), make_inline(1), make_inline(2), [5])


class InlineCallTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        warm(add3, 1, 2, 3)

    def test_len(self):
        def f(seq):
            n = len(seq)
            frame = sys._getframe()
            return n, inline_locals(frame)
        n, names = warm(f, [1, 2, 3])
        self.assertEqual(n, 3)
        self.assertIn("n", names)
        with self.assertRaisesRegex(TypeError, "object of type 'int' has no len"):
            f(make_inline(70000))

    def test_isinstance(self):
        import numbers
        def f(x, cls):
            return isinstance(x, cls)
        warm(f, 1, int)
        self.assertIn("CALL_ISINSTANCE", opnames(f))
        for value in (make_inline(70000), make_inline_float(1.5)):
            for cls in (int, float, object, bool, str, (int, float), (str, bytes),
                        (str, numbers.Number), numbers.Number, numbers.Integral,
                        numbers.Real, ((int,), float), ()):
                with self.subTest(value=value, cls=cls):
                    self.assertEqual(f(value, cls), isinstance(int(value) if type(value) is int
                                                               else float(value), cls))
        with self.assertRaises(TypeError):
            f(make_inline(1), make_inline(2))
        with self.assertRaises(TypeError):
            f(make_inline(1), (str, 5))
        self.assertTrue(f(make_inline(1), (int, 5)))


class ContainerCanonicalTest(unittest.TestCase):
    """A tuple or a list holds an int that fits in int32 or a float other
    than NaN as its value: a box stored through the C API is not kept, and
    every read gives a value identical to what was stored."""

    def setUp(self):
        self.capi = import_helper.import_module("_testcapi")
        self.limited = import_helper.import_module("_testlimitedcapi")

    VALUES = (70000, -2**31, 2**31 - 1, 1.5, -0.0, math.inf, 5e-324)

    def check_read(self, value, read):
        self.assertIs(read, value)
        self.assertEqual(id(read), id(value))
        self.assertIs(type(read), type(value))

    def test_list_set_item(self):
        for value in self.VALUES:
            with self.subTest(value=value):
                box = distinct(value)
                lst = [None, None]
                self.limited.list_setitem(lst, 0, box)
                self.capi.list_set_item(lst, 1, distinct(value))
                self.check_read(value, lst[0])
                self.check_read(lst[0], lst[1])
                self.check_read(value, self.limited.list_getitem(lst, 0))
                self.check_read(value, self.capi.list_get_item(lst, 1))
                self.assertEqual(lst.index(value), 0)
                self.assertEqual(lst.count(box), 2)
                self.assertIn(value, lst)

    def test_tuple_set_item(self):
        for value in self.VALUES:
            with self.subTest(value=value):
                t = self.limited.tuple_pack(2, distinct(value), distinct(value))
                self.check_read(value, t[0])
                self.check_read(t[0], t[1])
                self.check_read(value, self.limited.tuple_getitem(t, 1))
                self.check_read(value, self.capi.tuple_get_item(t, 0))
                self.assertEqual(hash(t), hash((value, value)))
                self.assertEqual(t, (value, value))

    def test_containers_from_c(self):
        for value in self.VALUES:
            with self.subTest(value=value):
                src = [distinct(value), distinct(value)]
                for got in (tuple(src), list(src), self.limited.list_astuple(src),
                            src[:], src * 2, sorted(src)):
                    for item in got:
                        self.check_read(value, item)

    def test_cell(self):
        import types
        for value in self.VALUES:
            with self.subTest(value=value):
                cell = types.CellType(distinct(value))
                self.check_read(value, cell.cell_contents)
                cell.cell_contents = distinct(value)
                self.check_read(value, cell.cell_contents)
                self.assertEqual(cell, types.CellType(value))
                self.assertEqual(gc.get_referents(cell), [])

                def outer(x):
                    def inner():
                        return x
                    return inner
                f = outer(distinct(value))
                self.check_read(value, f())
                self.check_read(value, f.__closure__[0].cell_contents)
        self.assertLess(types.CellType(), types.CellType(0))
        self.assertEqual(types.CellType(), types.CellType())

    def test_dict(self):
        for value in self.VALUES:
            with self.subTest(value=value):
                d = {}
                self.limited.dict_setitem(d, distinct(value), distinct(value))
                self.check_read(value, d[value])
                self.check_read(value, self.limited.dict_getitem(d, value))
                rc, pos, key, item = self.limited.dict_next(d, 0)
                self.check_read(value, key)
                self.check_read(value, item)
                self.check_read(value, next(iter(d)))
                self.check_read(value, next(reversed(d)))
                for pair in (next(iter(d.items())), next(reversed(d.items()))):
                    self.check_read(value, pair[0])
                    self.check_read(value, pair[1])
                self.check_read(value, list(d.values())[0])
                self.check_read(value, d.get(distinct(value)))
                self.check_read(value, d.setdefault(distinct(value), None))
                self.assertEqual(d, {value: value})
                self.assertEqual(gc.get_referents(d), [])
                key, item = d.copy().popitem()
                self.check_read(value, key)
                self.check_read(value, item)
                self.check_read(value, d.pop(value))
                self.assertEqual(d, {})

    def test_dict_mixed_keys(self):
        # 1, 1.0 and True are one key; the first key stored is kept.
        d = {1: 'a'}
        d[1.0] = 'b'
        d[True] = 'c'
        self.assertEqual(d, {1: 'c'})
        self.assertIs(type(next(iter(d))), int)
        keys = [70000, 2.5, 2**40, 'x', -0.0, (1, 2.5)]
        d = dict.fromkeys(keys, 1.5)
        for key in keys:
            self.assertIn(key, d)
            self.check_read(1.5, d[key])
        self.assertIn(0, d)  # -0.0 == 0
        self.assertEqual(list(d), keys)

    def test_instance_attributes(self):
        class C:
            pass
        for value in self.VALUES:
            with self.subTest(value=value):
                c = C()
                c.a = distinct(value)
                setattr(c, 'b', distinct(value))
                for _ in range(3):  # specialised and unspecialised reads
                    self.check_read(value, c.a)
                    self.check_read(value, getattr(c, 'b'))
                self.assertEqual(gc.get_referents(c), [C])
                d = c.__dict__
                self.check_read(value, d['a'])
                c.a = distinct(value)
                self.check_read(value, d['a'])
                d['b'] = distinct(value)
                self.check_read(value, c.b)
                del c.a
                self.assertEqual(d, {'b': value})

    def test_slots(self):
        class C:
            __slots__ = ('a', 'b')
        for value in self.VALUES:
            with self.subTest(value=value):
                c = C()
                c.a = distinct(value)
                setattr(c, 'b', distinct(value))
                for _ in range(3):  # specialised and unspecialised reads
                    self.check_read(value, c.a)
                    self.check_read(value, getattr(c, 'b'))
                    self.check_read(value, C.a.__get__(c))
                self.assertEqual(gc.get_referents(c), [C])
                del c.a
                with self.assertRaises(AttributeError):
                    c.a
                with self.assertRaises(AttributeError):
                    del c.a
                self.assertFalse(hasattr(c, 'a'))
                C.b.__set__(c, distinct(value))
                self.check_read(value, c.b)

    def test_globals(self):
        for value in self.VALUES:
            with self.subTest(value=value):
                ns = {'v': distinct(value)}
                exec("def f():\n    return v\n", ns)
                for _ in range(3):
                    self.check_read(value, ns['f']())
                ns['v'] = distinct(value)
                self.check_read(value, ns['f']())

    def test_outside_int32_is_an_object(self):
        x = distinct(2**40)
        lst = [None]
        self.limited.list_setitem(lst, 0, x)
        self.assertIs(lst[0], x)
        self.assertIsNot(lst[0], distinct(2**40))
        self.assertEqual(gc.get_referents(lst), [x])

    def test_inline_items_are_not_referents(self):
        self.assertEqual(gc.get_referents([1, 2.5, -3]), [])
        self.assertEqual(gc.get_referents((70000, 1e10)), [])

    def test_sort_mixed(self):
        values = [3, 2.5, -1, 2**40, 0.0, -2**31, 1e300, 7]
        self.assertEqual(sorted(values), sorted(values, key=float))
        self.assertEqual(sorted(values, reverse=True),
                         sorted(values, key=float, reverse=True))
        pairs = [(v, i) for i, v in enumerate(values)]
        self.assertEqual([v for v, _ in sorted(pairs)], sorted(values))


class BoxFailureTest(unittest.TestCase):
    """A box that cannot be made at first is made again after a collection;
    an accessor that cannot report an error aborts if that fails too."""

    CODE = (
        "import _testcapi\n"
        "t = (int('70000'),)\n"
        "_testcapi.set_nomemory({start}, {stop})\n"
        "x = _testcapi.tuple_get_item(t, 0)\n"
        "_testcapi.remove_mem_hooks()\n"
        "print(x)\n")

    def setUp(self):
        import_helper.import_module("_testcapi")

    def run_failing(self, start, stop):
        code = self.CODE.format(start=start, stop=stop)
        return script_helper.run_python_until_end("-c", code)[0]

    def test_accessor_aborts_and_one_failure_is_recovered(self):
        # Find the allocation that is PyTuple_GET_ITEM()'s box: from it on,
        # every allocation fails, and the accessor aborts naming itself.
        for start in range(30):
            res = self.run_failing(start, 0)
            if b"PyTuple_GET_ITEM: out of memory" in res.err:
                break
        else:
            self.fail("PyTuple_GET_ITEM() never aborted")
        self.assertNotEqual(res.rc, 0)
        # When that allocation alone fails, the box made after the
        # collection succeeds.
        res = self.run_failing(start, start + 1)
        self.assertEqual(res.rc, 0, res.err)
        self.assertEqual(res.out.strip(), b"70000")


if __name__ == "__main__":
    unittest.main()
