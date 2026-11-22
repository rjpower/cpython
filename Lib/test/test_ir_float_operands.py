"""Float arithmetic and comparison handlers on inline and boxed operands.

The specialized handlers take two inline floats, or an inline float and an
inline int. Any other operand pair, and any result that is NaN, goes the
generic way. Each site is warmed up on one kind of operand and then run on
all the others, so that the handler, its deoptimization and the generic
operation must agree.
"""

import itertools
import math
import operator
import unittest

INF = math.inf
NAN = math.nan

FLOATS = [0.0, -0.0, 1.5, -2.25, 1e308, -1e308, 5e-324, INF, -INF, NAN]
INLINE_INTS = [0, 1, -7, 3, 2**49]
BOXED_INTS = [2**60, -2**70]
OPERANDS = FLOATS + INLINE_INTS + BOXED_INTS + [True]

ARITHMETIC = {
    "+": operator.add,
    "-": operator.sub,
    "*": operator.mul,
    "/": operator.truediv,
}
COMPARISONS = {
    "<": operator.lt,
    "<=": operator.le,
    "==": operator.eq,
    "!=": operator.ne,
    ">": operator.gt,
    ">=": operator.ge,
}


def make_locals(op):
    """Operands are locals, consumed at their last use."""
    ns = {}
    exec(f"def f(a, b):\n    return a {op} b\n", ns)
    return ns["f"]


def make_temporaries(op):
    """Operands are temporaries made by earlier instructions."""
    ns = {}
    exec(f"def f(a, b):\n    return (a + 0.0) {op} (b * 1.0)\n", ns)
    return ns["f"]


def make_branch(op):
    """The comparison feeds a jump."""
    ns = {}
    exec(f"def f(a, b):\n    if a {op} b:\n        return 1\n    return 0\n", ns)
    return ns["f"]


def outcome(func, a, b):
    try:
        return ("value", func(a, b))
    except ZeroDivisionError:
        return ("ZeroDivisionError",)


def same(x, y):
    if x[0] != y[0]:
        return False
    if x[0] != "value":
        return True
    u, v = x[1], y[1]
    if type(u) is not type(v):
        return False
    if isinstance(u, float):
        if math.isnan(u) or math.isnan(v):
            return math.isnan(u) and math.isnan(v)
        return u == v and math.copysign(1.0, u) == math.copysign(1.0, v)
    return u == v


def warm(func, a, b):
    for _ in range(64):
        outcome(func, a, b)


WARMUPS = [(1.5, 2.5), (1.5, 3), (3, 1.5), (3, 4), (NAN, 1.5)]


class FloatOperandTests(unittest.TestCase):
    def check(self, make, table):
        for symbol, generic in table.items():
            for warmup in WARMUPS:
                func = make(symbol)
                warm(func, *warmup)
                for a, b in itertools.product(OPERANDS, repeat=2):
                    with self.subTest(op=symbol, warmup=warmup, a=a, b=b):
                        expected = outcome(generic, a, b)
                        self.assertTrue(same(outcome(func, a, b), expected),
                                        (outcome(func, a, b), expected))

    def test_arithmetic_on_locals(self):
        self.check(make_locals, ARITHMETIC)

    def test_arithmetic_on_temporaries(self):
        self.check(make_temporaries, {
            symbol: lambda a, b, generic=generic: generic(a + 0.0, b * 1.0)
            for symbol, generic in ARITHMETIC.items()
        })

    def test_comparison_on_locals(self):
        self.check(make_locals, COMPARISONS)

    def test_comparison_feeding_a_jump(self):
        for symbol, generic in COMPARISONS.items():
            for warmup in WARMUPS:
                func = make_branch(symbol)
                warm(func, *warmup)
                for a, b in itertools.product(OPERANDS, repeat=2):
                    with self.subTest(op=symbol, warmup=warmup, a=a, b=b):
                        self.assertEqual(func(a, b), int(generic(a, b)))

    def test_operands_survive_the_operation(self):
        # A borrowed local is not consumed by float arithmetic on it.
        def f(a, b):
            c = a * b
            d = a + b
            return c, d, a, b

        for _ in range(64):
            f(1.5, 2)
        self.assertEqual(f(1.5, 2), (3.0, 3.5, 1.5, 2))
        self.assertEqual(f(2, 1.5), (3.0, 3.5, 2, 1.5))

    def test_nan_result_is_a_float(self):
        def f(a, b):
            return a - b

        for _ in range(64):
            f(1.5, 0.5)
        result = f(INF, INF)
        self.assertIs(type(result), float)
        self.assertTrue(math.isnan(result))

    def test_division_by_zero_raises(self):
        def f(a, b):
            return a / b

        for _ in range(64):
            f(1.5, 2)
        for zero in (0, 0.0, -0.0):
            with self.assertRaises(ZeroDivisionError):
                f(1.5, zero)
            with self.assertRaises(ZeroDivisionError):
                f(2, zero)


if __name__ == "__main__":
    unittest.main()
