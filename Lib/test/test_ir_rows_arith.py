"""The failure, ownership and reentry claims of the arithmetic, compare,
contains and match rows of Include/internal/pycore_ir_components.h that
their uops rely on (regions89 G0c, batch 2).  Each test runs the operation
often enough for the word to specialize, then takes the row's failure path
at the same site."""

import gc
import sys
import textwrap
import unittest
from test.support import script_helper


WARM = 200


class Boom(Exception):
    pass


def warmed(fn, *args):
    for _ in range(WARM):
        fn(*args)


class ArithmeticRows(unittest.TestCase):
    def test_lshift_failure_leaves_the_site_usable(self):
        # _PyValue_LshiftInt64 gives NULL with MemoryError (or OverflowError)
        # for a result that cannot be allocated, which the uop raises
        def shift(a, b):
            return a << b

        warmed(shift, 3, 5)
        for _ in range(3):
            with self.assertRaises((MemoryError, OverflowError)):
                shift(1, (1 << 50) - 1)
            self.assertEqual(shift(3, 70), 3 << 70)
            self.assertEqual(shift(1, 62), 1 << 62)
            self.assertEqual(shift(-5, 3), -40)

    def test_multiply_overflow_boxes_the_product(self):
        def mul(a, b):
            return a * b

        warmed(mul, 3, 5)
        big = (1 << 50) - 1
        self.assertEqual(mul(big, big), big * big)
        self.assertEqual(mul(-big, big), -(big * big))
        self.assertEqual(mul(1 << 49, 1 << 49), 1 << 98)
        self.assertEqual(mul(1 << 30, 1 << 30), 1 << 60)

    def test_division_and_shift_guards_restart_the_word(self):
        def floordiv(a, b):
            return a // b

        def mod(a, b):
            return a % b

        def rshift(a, b):
            return a >> b

        warmed(floordiv, 7, 2)
        warmed(mod, 7, 2)
        warmed(rshift, 7, 2)
        with self.assertRaises(ZeroDivisionError):
            floordiv(1, 0)
        with self.assertRaises(ZeroDivisionError):
            mod(1, 0)
        with self.assertRaises(ValueError):
            rshift(1, -1)
        self.assertEqual(floordiv(-7, 2), -4)
        self.assertEqual(mod(-7, 2), 1)
        self.assertEqual(rshift(-7, 100), -1)

    def test_truediv_by_zero_restarts_the_word(self):
        def div(a, b):
            return a / b

        warmed(div, 1.5, 2.5)
        with self.assertRaises(ZeroDivisionError):
            div(1.0, 0.0)
        self.assertEqual(div(3.0, 2.0), 1.5)

    def test_nan_results_are_boxed_floats(self):
        def add(a, b):
            return a + b

        warmed(add, 1.5, 2.5)
        inf = float("inf")
        result = add(inf, -inf)
        self.assertIsInstance(result, float)
        self.assertNotEqual(result, result)

    def test_str_concatenation_in_place_keeps_other_references(self):
        # value_unicode_append consumes its left argument: an aliased string
        # is replaced, not extended
        def append(s, t):
            s += t
            return s

        warmed(append, "a", "b")
        base = "x" * 50 + str(id(self))
        alias = base
        grown = append(base, "tail")
        self.assertEqual(alias, base)
        self.assertEqual(grown, base + "tail")
        self.assertIsNot(grown, alias)

        def loop(n):
            s = ""
            for i in range(n):
                s += "ab"
            return s

        warmed(loop, 10)
        self.assertEqual(loop(1000), "ab" * 1000)

    def test_str_concatenation_target_holds_any_word(self):
        # _BINARY_OP_INPLACE_ADD_UNICODE's guard reads the local that the
        # next word stores to, which may hold any word: an inline int or
        # float, None, or an object.  It compares the words (it used to read
        # an object from it, a fatal error for a number).  Under the JIT off
        # and always (the word compiled)
        source = textwrap.dedent("""
            def f(s, t, k, other):
                y = s if k else other
                y = s + t
                return y
            for _ in range(500):
                f("a", "b", True, None)
            for other in (1, 1.5, None, object(), 2**70):
                print(f("a", "b", False, other))
        """)
        modes = ["off"]
        if sys._jit.is_available():
            modes.append("always")
        for mode in modes:
            with self.subTest(jit=mode):
                result = script_helper.assert_python_ok("-X", f"jit={mode}", "-c", source)
                self.assertEqual(result.out.decode().split(), ["ab"] * 5)

    def test_str_subscripts_by_inline_index(self):
        def at(s, i):
            return s[i]

        warmed(at, "abc", 1)
        self.assertEqual(at("abc", -1), "c")
        self.assertEqual(at("a€b", 1), "€")
        with self.assertRaises(IndexError):
            at("abc", 3)
        self.assertEqual(at("abc", 2), "c")


class SubscriptRows(unittest.TestCase):
    def test_dict_subscript_hit_and_miss(self):
        # IR_TRY_DICT_FIRST_PROBE declines without effects on a miss
        def get(d, k):
            return d[k]

        d = {i: str(i) for i in range(50)}
        warmed(get, d, 7)
        self.assertEqual(get(d, 49), "49")
        with self.assertRaises(KeyError) as cm:
            get(d, 1000)
        self.assertEqual(cm.exception.args, (1000,))
        self.assertEqual(get(d, 3), "3")

    def test_dict_subscript_runs_python_in_a_key(self):
        class Key:
            armed = False

            def __hash__(self):
                return 1

            def __eq__(self, other):
                if self.armed:
                    raise Boom
                return other is self

        def get(d, k):
            return d[k]

        d = {1: 2}
        warmed(get, d, 1)
        d[Key()] = 1
        Key.armed = True
        try:
            with self.assertRaises(Boom):
                get(d, Key())
        finally:
            Key.armed = False

    def test_list_slice_runs_index(self):
        class Index:
            def __index__(self):
                raise Boom

        def cut(x, i):
            return x[i:]

        data = list(range(10))
        warmed(cut, data, 2)
        with self.assertRaises(Boom):
            cut(data, Index())
        self.assertEqual(cut(data, 8), [8, 9])


class ContainsAndCompareRows(unittest.TestCase):
    def test_contains_runs_eq_and_propagates(self):
        class Eq:
            def __hash__(self):
                return 7

            def __eq__(self, other):
                raise Boom

        def contains(c, x):
            return x in c

        for container in ({1, 2, 3}, {1: 1, 2: 2}, [1, 2, 3], (1, 2, 3), "abc"):
            probe = 2 if not isinstance(container, str) else "b"
            warmed(contains, container, probe)
            self.assertTrue(contains(container, probe))
        with self.assertRaises(Boom):
            contains({Eq(), 1}, Eq())
        with self.assertRaises(TypeError):
            contains("abc", 1)

    def test_compare_raises_from_the_protocol(self):
        class Lt:
            def __lt__(self, other):
                raise Boom

        def less(a, b):
            return a < b

        warmed(less, 1, 2)
        with self.assertRaises(Boom):
            less(Lt(), 1)
        self.assertTrue(less(1, 2))
        self.assertFalse(less(float("nan"), 1.0))


class MatchRows(unittest.TestCase):
    def test_match_class_with_numbers_and_errors(self):
        class P:
            __match_args__ = ("x", "y")

            def __init__(self, x, y):
                self.x, self.y = x, y

        def match_class(v):
            match v:
                case P(x, y):
                    return (x, y)
                case int(n):
                    return n
                case float():
                    return "float"
            return None

        warmed(match_class, P(1, 2))
        self.assertEqual(match_class(P(1, 2)), (1, 2))
        self.assertEqual(match_class(5), 5)
        self.assertEqual(match_class(1.5), "float")
        self.assertIsNone(match_class("s"))

    def test_match_class_rejects_a_non_class(self):
        number = 5

        def bad(v):
            match v:
                case number():
                    return 1

        with self.assertRaises(TypeError):
            bad(1)

    def test_match_class_duplicate_attribute_is_an_error(self):
        class Q:
            __match_args__ = ("x",)
            x = 1

        def dup(v):
            match v:
                case Q(x, x=y):
                    return y

        with self.assertRaises(TypeError):
            dup(Q())

    def test_match_keys_on_numbers_and_mappings(self):
        def keys(v):
            match v:
                case {"a": a, "b": b}:
                    return (a, b)
            return None

        warmed(keys, {"a": 1, "b": 2})
        self.assertEqual(keys({"a": 1, "b": 2, "c": 3}), (1, 2))
        self.assertIsNone(keys({"a": 1}))
        self.assertIsNone(keys(5))
        self.assertIsNone(keys(1.5))

    def test_match_keys_runs_get(self):
        class M(dict):
            def get(self, key, default=None):
                raise Boom

        def keys(v):
            match v:
                case {"a": a}:
                    return a

        with self.assertRaises(Boom):
            keys(M(a=1))

    def test_except_validates_its_types(self):
        def catch(exc, typ):
            try:
                raise exc
            except typ:
                return "caught"
            except BaseException:
                return "other"

        warmed(catch, ValueError(), ValueError)
        self.assertEqual(catch(ValueError(), ValueError), "caught")
        self.assertEqual(catch(KeyError(), (ValueError, KeyError)), "caught")
        with self.assertRaises(TypeError):
            catch(ValueError(), 5)
        with self.assertRaises(TypeError):
            catch(ValueError(), (ValueError, 5))

    def test_except_star_validates_its_types(self):
        def catch(exc, typ):
            caught = False
            try:
                raise exc
            except* typ:
                caught = True
            return caught

        eg = ExceptionGroup("g", [ValueError(1)])
        self.assertTrue(catch(eg, ValueError))
        with self.assertRaises(TypeError):
            catch(eg, 5)
        with self.assertRaises(TypeError):
            catch(eg, ExceptionGroup)


if __name__ == "__main__":
    unittest.main()
