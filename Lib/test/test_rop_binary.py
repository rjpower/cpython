"""The Binary and AppendText families (Python/register_ops/numeric.rop): each
operator on operand pairs that take the Int, Float, Text and Extended arms
and on pairs that fall to Generic, compared with the operator module after
the site has adapted."""

import operator
import sys
import unittest

from test import support
from test.support import import_helper

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


INF, NAN = float("inf"), float("nan")
# Float power: (base, exponent, whether Binary.Float_NB_POWER admits the pair).
# It admits a positive finite base with a finite exponent whose result is zero
# or a normal float, where float_pow returns pow(base, exponent) itself;
# every other pair misses to Binary.Generic, which raises or special-cases.
POWER_CASES = [
    (2.0, 0.5, True), (3, 2.0, True), (2.0, -3, True), (1e-5, 1e-300, True),
    (2.0, -0.0, True),                          # a negative zero exponent
    (2.0, -1080.0, True),                       # underflows to zero
    (2.0, -1070.0, False),                      # a subnormal result
    (10.0, 400.0, False), (10, 400.0, False),   # overflow: OverflowError
    (-8.0, 1 / 3, False), (-8, 0.5, False),     # a complex result
    (-2.0, 3.0, False), (-2.0, -2.0, False),    # a negative base
    (-0.0, 3.0, False), (-0.0, -1.0, False),    # a negative zero base
    (0.0, 2.0, False), (0.0, -1.0, False),      # a zero base
    (INF, 2.0, False), (INF, -1.0, False), (-INF, 3.0, False),
    (NAN, 0.0, False), (NAN, 2.0, False),
    (2.0, INF, False), (0.5, INF, False), (2.0, -INF, False),
    (1.0, NAN, False), (2.0, NAN, False),
]


def binary_arm(function):
    """The arm that function's one Binary word runs"""
    from test.support import ir_dis
    (arm,) = [w.arm for w in ir_dis.get_instructions(function, adaptive=True)
              if w.arm.split(".")[0] == "Binary"]
    return arm


@support.cpython_only
class FloatPowerTests(unittest.TestCase):
    """Power is one member arm of Binary.Float: on every edge it either admits
    the pair and gives float_pow's result, or misses to the generic arm, which
    gives the language's; the word stays on the member arm either way."""

    def setUp(self):
        self.capi = import_helper.import_module("_testinternalcapi")
        if support.Py_GIL_DISABLED:
            self.skipTest("the free-threaded build does not specialise the IR")

    def trained(self, op):
        function = site(op)
        for _ in range(WARMUP):
            function(2.0, 0.5)
        member = "NB_POWER" if op == "**" else "NB_INPLACE_POWER"
        self.assertEqual(binary_arm(function), f"Binary.Float_{member}")
        return function

    def test_values_and_the_arm_the_word_ends_on(self):
        for op in ("**", "**="):
            for a, b, _ in POWER_CASES:
                with self.subTest(op=op, a=a, b=b):
                    function = self.trained(op)
                    want = outcome(operator.pow, a, b)
                    for _ in range(3):
                        self.assertTrue(same(want, outcome(function, a, b)), (want,))
                    self.assertEqual(binary_arm(function), binary_arm(self.trained(op)))

    def test_the_member_arm_admits_or_misses(self):
        if not hasattr(self.capi, "jit_template_run"):
            self.skipTest("needs the template probe")
        from test.test_jit_template import slot_of, word_of
        stub, missed = 2, 1
        function = self.trained("**")
        handler = self.capi.arm_handlers()["Binary.Float_NB_POWER"]
        index, _ = word_of(function, "Binary.Float_NB_POWER")
        fields = dict(zip("ABCD", self.capi.get_ir(function.__code__, True)[index][1:]))
        for a, b, admits in POWER_CASES:
            with self.subTest(a=a, b=b):
                slots = {slot_of(fields, "B"): a, slot_of(fields, "C"): b}
                result = self.capi.jit_template_run(function, index, slots, stub, handler)
                # (the probe shows an inline result as its raw word: the values
                # are the test above's)
                self.assertEqual(not result[0] & missed, admits, result)
                self.assertIsNone(result[2])


@support.cpython_only
class IntMemberTests(unittest.TestCase):
    """Binary.Int has an arm per operator of its hot set; the other operators
    it admits share its rest arm, which reads the operator from the word"""

    def test_each_operator_takes_its_arm(self):
        if support.Py_GIL_DISABLED:
            self.skipTest("the free-threaded build does not specialise the IR")
        cases = {"-": "NB_SUBTRACT", "-=": "NB_INPLACE_SUBTRACT", "+=": "NB_INPLACE_ADD",
                 "*": "NB_MULTIPLY", "*=": "NB_INPLACE_MULTIPLY", "//": "NB_FLOOR_DIVIDE",
                 "%": "NB_REMAINDER", "&": "NB_AND", "|": "NB_OR", "^": "NB_XOR",
                 "<<": "NB_LSHIFT", ">>": "NB_RSHIFT", "//=": None, "%=": None,
                 "&=": None, "|=": None, "^=": None, "<<=": None, ">>=": None}
        for op, member in cases.items():
            with self.subTest(op=op):
                function = site(op)
                for _ in range(WARMUP):
                    function(13, 3)
                self.assertEqual(function(13, 3), eval(f"13 {op.rstrip('=')} 3"))
                self.assertEqual(binary_arm(function),
                                 f"Binary.Int_{member}" if member else "Binary.Int")


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
