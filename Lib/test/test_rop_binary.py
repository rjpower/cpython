"""The Binary and AppendText families (Python/register_ops/numeric.rop): each
operator on operand pairs that take the Int, Float, Text and Extended arms
and on pairs that fall to Generic, compared with the operator module after
the site has adapted."""

import operator
import sys
import unittest

from test import support

WARMUP = 4000  # past the specialization threshold and its backoff

OPS = {
    "+=": operator.iadd, "-": operator.sub, "-=": operator.isub,
    "*": operator.mul, "*=": operator.imul, "/": operator.truediv,
    "//": operator.floordiv, "%": operator.mod, "&": operator.and_,
    "|": operator.or_, "^": operator.xor, "<<": operator.lshift,
    ">>": operator.rshift, "**": operator.pow,
}

INT51 = 1 << 50
INTS = [0, 1, -1, 2, -2, 7, -7, 62, 63, 64, INT51 - 1, -INT51, INT51, -INT51 - 1,
        1 << 62, 1 << 70]
FLOATS = [0.0, -0.0, 1.5, -2.25, float("inf"), float("nan"), 1e300]
SMALL = [0, 1, -1, 2, 7, 62, 63, 64, 100]


def site(op):
    """A function of one BINARY_OP site for the operator"""
    namespace = {}
    if op.endswith("="):
        source = f"def f(a, b):\n    a {op} b\n    return a\n"
    else:
        source = f"def f(a, b):\n    return a {op} b\n"
    exec(source, namespace)
    return namespace["f"]


def outcome(function, a, b):
    try:
        return ("ok", function(a, b))
    except (ArithmeticError, ValueError, TypeError) as error:
        return (type(error), None)


def same(x, y):
    if x[0] != y[0]:
        return False
    a, b = x[1], y[1]
    if isinstance(a, float) and a != a:
        return isinstance(b, float) and b != b
    return type(a) is type(b) and a == b


class BinaryArmTests(unittest.TestCase):
    def check(self, op, pairs):
        function = site(op)
        reference = OPS[op]
        # Train the site on the first pair, then run them all
        a, b = pairs[0]
        for _ in range(WARMUP):
            outcome(function, a, b)
        for a, b in pairs:
            if op in ("<<", "**") and (abs(b) > 100 or isinstance(b, float)):
                continue
            want = outcome(reference, a, b)
            got = outcome(function, a, b)
            self.assertTrue(same(want, got), (op, a, b, want, got))

    def test_int_operators(self):
        pairs = [(a, b) for a in INTS for b in INTS]
        small = [(a, b) for a in INTS for b in SMALL]
        for op in OPS:
            with self.subTest(op=op):
                shift = op in ("<<", "<<=", "**")
                self.check(op, small if shift else pairs)

    def test_float_operators(self):
        pairs = [(a, b) for a in FLOATS for b in FLOATS + [3, -4, 0]]
        pairs += [(a, 2.5) for a in INTS]
        for op in ("+=", "-", "-=", "*", "*=", "/"):
            with self.subTest(op=op):
                self.check(op, pairs)

    def test_site_survives_changing_operands(self):
        function = site("-")
        for _ in range(WARMUP):
            function(5, 3)
        self.assertEqual(function(5, 3), 2)
        self.assertEqual(function(5.5, 3), 2.5)
        self.assertEqual(function({1, 2}, {2}), {1})
        self.assertEqual(function(7, 1 << 80), 7 - (1 << 80))
        with self.assertRaises(TypeError):
            function("a", 1)

    def test_extended_operands(self):
        for op, a, b in (("*", (1, 2), 3), ("*", 3, (1, 2)),
                         ("|", {1: 1}, {2: 2}), ("*=", "ab", 3),
                         ("+=", b"a", b"b"), ("+=", (1,), (2,))):
            with self.subTest(op=op, a=a, b=b):
                function = site(op)
                for _ in range(WARMUP):
                    function(a, b)
                self.assertEqual(function(a, b), OPS[op](a, b))

    def test_unary(self):
        def negate(a):
            return -a

        def invert(a):
            return ~a

        def plus(a):
            return +a

        for function, reference in ((negate, operator.neg), (invert, operator.inv),
                                    (plus, operator.pos)):
            for _ in range(WARMUP):
                function(5)
            for value in INTS + FLOATS[:4] + [1 + 2j]:
                if function is invert and isinstance(value, (float, complex)):
                    continue
                self.assertTrue(same(outcome(lambda a, b: function(a), value, 0),
                                     outcome(lambda a, b: reference(a), value, 0)),
                                (function.__name__, value))
        with self.assertRaises(TypeError):
            invert(1.5)


class AppendTextTests(unittest.TestCase):
    def test_append_to_a_local(self):
        def append(s, t):
            s += t
            return s

        for _ in range(WARMUP):
            append("ab", "cd")
        self.assertEqual(append("ab", "cd"), "abcd")
        # Another name for the string keeps its value
        def shared(s, t):
            u = s
            s += t
            return u, s

        for _ in range(WARMUP):
            shared("ab", "cd")
        self.assertEqual(shared("ab" * 3, "cd"), ("ababab", "abababcd"))

    def test_append_to_itself(self):
        def double(s):
            for _ in range(5):
                s += s
            return s

        for _ in range(WARMUP):
            double("ab")
        self.assertEqual(double("ab"), "ab" * 32)

    def test_site_changes_type(self):
        def add(s, t):
            s += t
            return s

        for _ in range(WARMUP):
            add("a", "b")
        self.assertEqual(add(1, 2), 3)
        self.assertEqual(add([1], [2]), [1, 2])
        self.assertEqual(add(b"a", b"b"), b"ab")
        self.assertEqual(add("a", "b"), "ab")
        with self.assertRaises(TypeError):
            add("a", 1)

    def test_failure_keeps_the_local_defined_by_the_operands(self):
        def add(s, t):
            s += t
            return s

        for _ in range(WARMUP):
            add("a", "b")
        before = sys.getrefcount("a")
        with self.assertRaises(TypeError):
            add("a", 1)
        self.assertEqual(sys.getrefcount("a"), before)

    @support.requires_resource("cpu")
    def test_loop_is_linear(self):
        def build(n):
            s = ""
            for _ in range(n):
                s += "ab"
            return len(s)

        import time
        timings = []
        for n in (100_000, 400_000):
            start = time.perf_counter()
            self.assertEqual(build(n), 2 * n)
            timings.append(time.perf_counter() - start)
        # Quadratic copying would cost 16 times as much for 4 times the work
        self.assertLess(timings[1], timings[0] * 10 + 0.05)


if __name__ == "__main__":
    unittest.main()
