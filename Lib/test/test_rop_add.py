"""The Add family (Python/register_ops/add.rop) by its arms, run on words of
BINARY_OP's function: the form, the
template and the completion stub of each arm, from the same slots."""

import struct
import sys
import unittest

from test import test_tools
from test.support import import_helper

capi = import_helper.import_module("_testinternalcapi")
if not hasattr(capi, "jit_template_run"):
    raise unittest.SkipTest("needs the template probe")

from test.test_jit_template import slot_of, word_of

FORM, TEMPLATE, STUB = 0, 1, 2
_PyIR_MISS = 1
_PyIR_AGAIN = 16

SOURCES = {
    "locals": "def f(a, b):\n    return a + b\n",
    "temporaries": "def f(a, b):\n    return (a * 1) + (b * 1)\n",
    "alias": "def f(a, b):\n    a = a + b\n    return a\n",
}


def arms():
    """The handler numbers of the arms, from the compiled metadata"""
    return capi.arm_handlers()


def compiled(kind):
    namespace = {}
    exec(SOURCES[kind], namespace)
    f = namespace["f"]
    # Lists take no arm, so the word stays on Add.Generic
    f([1], [2])
    index, _ = word_of(f, "Add.Generic")
    return f, index, dict(zip("ABCD", capi.get_ir(f.__code__, True)[index][1:]))


def handler_of(f, index):
    """The handler of word index of the executing copy"""
    words = capi.get_ir_raw(f.__code__)[-2]
    return struct.unpack_from("<Q", words, index * 8)[0] & 0x1FF


def probe(f, index, fields, lhs, rhs, mode, handler):
    slots = {slot_of(fields, "B"): lhs, slot_of(fields, "C"): rhs}
    objects = [lhs, rhs]
    before = [sys.getrefcount(o) for o in objects]
    result = capi.jit_template_run(f, index, slots, mode, handler)
    after = [sys.getrefcount(o) for o in objects]
    return result, [b - a for a, b in zip(before, after)]


def run(kind, lhs, rhs, mode, handler):
    """Runs the word once under handler (-1: the word's own) and, if the
    word asks to run again under a handler that it now names (a
    specialisation), once more under that one.  Returns the probe's result,
    the destination, the changes in the operands' counts and whether the
    word asked to run again."""
    f, index, fields = compiled(kind)
    result, counts = probe(f, index, fields, lhs, rhs, mode, handler)
    if mode == STUB and result[0] & _PyIR_MISS:
        # The compiled site's cold path: the family's generic stub
        result, counts = probe(f, index, fields, lhs, rhs, STUB, arms()["Add.Generic"])
    again = result[0] == (_PyIR_AGAIN if mode == STUB else index)
    if mode == STUB and result[0] & _PyIR_AGAIN:
        again = True
    if again:
        result, counts = probe(f, index, fields, lhs, rhs, FORM, -1)
    return result, result[3][slot_of(fields, "D" if False else "A")], counts, again


class Bump:
    def __init__(self, n):
        self.n = n

    def __add__(self, other):
        return Bump(self.n + (other.n if isinstance(other, Bump) else other))

    def __radd__(self, other):
        return Bump(other + self.n)


class Raises:
    def __add__(self, other):
        raise ZeroDivisionError("add")


class Quiet:
    def __add__(self, other):
        return NotImplemented


PAIRS = [
    (3, 4), (2**50, 2**50), (2**60, 5), (1.5, 2), ("a", "b"), ([1], [2]),
    ((1,), (2,)), (Bump(1), 2), (2, Bump(5)), (Bump(1), Bump(2)), (b"x", b"y"),
]


def same(x, y):
    if isinstance(x, Bump) and isinstance(y, Bump):
        return x.n == y.n
    if isinstance(x, float) and isinstance(y, float) and x != x and y != y:
        return True
    return type(x) is type(y) and (x == y or x is y)


def agree(test, kind, lhs, rhs, mode, handler):
    """The arm and Add.Generic leave the same state from
    the same slots: next word, exception, every slot and the counts"""
    (nxt, _, exc, slots), out, counts, _ = run(kind, lhs, rhs, mode, handler)
    (rnxt, _, rexc, rslots), rout, rcounts, _ = run(kind, lhs, rhs, FORM, -1)
    test.assertEqual(type(exc), type(rexc))
    test.assertEqual(str(exc), str(rexc))
    for i, (x, y) in enumerate(zip(slots, rslots)):
        test.assertTrue(same(x, y), f"slot {i}: {x!r} != {y!r}")
    # A traceback holds the operands of a failed add
    test.assertEqual(counts if exc is None else counts[:0], rcounts if exc is None else [])


class GenericTests(unittest.TestCase):
    """Add.Generic runs on every pair, by the form and by the stub, as the
    old generic handler does"""

    def test_pairs(self):
        generic = arms()["Add.Generic"]
        for kind in SOURCES:
            for lhs, rhs in PAIRS:
                for mode in (FORM, STUB):
                    with self.subTest(kind=kind, lhs=lhs, rhs=rhs, mode=mode):
                        agree(self, kind, lhs, rhs, mode, generic)

    def test_failures_raise_at_the_word(self):
        generic = arms()["Add.Generic"]
        for lhs, rhs, error in [(Raises(), 1, ZeroDivisionError), (1, "a", TypeError),
                                (Quiet(), Quiet(), TypeError), ([1], (2,), TypeError)]:
            for kind in SOURCES:
                for mode in (FORM, STUB):
                    with self.subTest(kind=kind, lhs=lhs, rhs=rhs, mode=mode):
                        (_, _, exc, _), *_ = run(kind, lhs, rhs, mode, generic)
                        self.assertIsInstance(exc, error)
                        agree(self, kind, lhs, rhs, mode, generic)

    def test_finalizer_runs_when_the_operand_dies(self):
        generic = arms()["Add.Generic"]
        log = []

        class Final(Bump):
            def __del__(self):
                log.append("del")

        for mode in (FORM, STUB):
            log.clear()
            f, index, fields = compiled("temporaries")
            lhs = Final(1)
            slots = {slot_of(fields, "B"): lhs, slot_of(fields, "C"): 2}
            del lhs
            (_, _, exc, _) = capi.jit_template_run(f, index, slots, mode, generic)
            del slots
            self.assertIsNone(exc)
            self.assertEqual(log, ["del"])


class IntTests(unittest.TestCase):
    """Add.Int by the form and the template, and its miss to the generic arm"""

    def test_hits_and_overflow(self):
        add_int = arms()["Add.Int"]
        pairs = [(3, 4), (-5, 2), (2**49, 2**49), (-2**50, -1), (2**50 - 1, 1), (0, 0)]
        for kind in SOURCES:
            for lhs, rhs in pairs:
                for mode in (FORM,):  # templates need the arm formats: step 4
                    with self.subTest(kind=kind, lhs=lhs, rhs=rhs, mode=mode):
                        agree(self, kind, lhs, rhs, mode, add_int)

    def test_miss_runs_the_generic_arm(self):
        add_int = arms()["Add.Int"]
        for kind in SOURCES:
            for lhs, rhs in PAIRS[2:]:
                for mode in (FORM,):  # templates need the arm formats: step 4
                    with self.subTest(kind=kind, lhs=lhs, rhs=rhs, mode=mode):
                        agree(self, kind, lhs, rhs, mode, add_int)


FLOAT_PAIRS = [(1.5, 2.5), (1.5, 2), (2, 1.5), (-0.0, 0.0), (1e308, 1e308),
               (float("inf"), float("-inf")), (3, 4), ("a", "b"), (1.5, "a"), (2**60, 1.5)]
TEXT_PAIRS = [("a", "b"), ("", "x"), ("é", "ñ"), ("x" * 1000, "y"), (3, 4), ("a", 1),
              (type("S", (str,), {})("a"), "b")]


class ArmTests(unittest.TestCase):
    """Add.Float and Add.Text, by the form and the stub: a pair that the arm
    does not admit misses to the generic arm"""

    def check(self, arm, pairs):
        handler = arms()[arm]
        for kind in SOURCES:
            for lhs, rhs in pairs:
                for mode in (FORM, STUB):
                    with self.subTest(arm=arm, kind=kind, lhs=lhs, rhs=rhs, mode=mode):
                        agree(self, kind, lhs, rhs, mode, handler)

    def test_float(self):
        self.check("Add.Float", FLOAT_PAIRS)

    def test_text(self):
        self.check("Add.Text", TEXT_PAIRS)


def adapted(kind, lhs, rhs, arm):
    """A function whose word has been adapted to arm by running the generic
    arm on lhs and rhs, and its word's index and fields"""
    f, index, fields = compiled(kind)
    generic = arms()["Add.Generic"]
    for _ in range(4000):
        probe(f, index, fields, lhs, rhs, FORM, generic)
        if handler_of(f, index) == arms()[arm]:
            return f, index, fields
        generic = -1 if handler_of(f, index) != arms()["Add.Generic"] else generic
    raise AssertionError(f"the word never selected {arm}")


class ExtendedTests(unittest.TestCase):
    CASES = [([1], [2]), ((1,), (2,)), (b"x", b"y"), ([], []), ((), ())]

    def test_adaptation_picks_each_arm(self):
        for arm, lhs, rhs in [("Add.Float", 1.5, 2.5), ("Add.Float", 1.5, 2),
                              ("Add.Text", "a", "b"), ("Add.Extended", [1], [2]),
                              ("Add.Extended", (1,), (2,)), ("Add.Extended", b"x", b"y"),
                              ("Add.Int", 3, 4)]:
            with self.subTest(arm=arm, lhs=lhs):
                adapted("locals", lhs, rhs, arm)

    def test_extension_arm_and_its_miss(self):
        for lhs, rhs in self.CASES:
            for kind in SOURCES:
                for mode in (FORM, STUB):
                    with self.subTest(kind=kind, lhs=lhs, mode=mode):
                        f, index, fields = adapted(kind, lhs, rhs, "Add.Extended")
                        (_, _, exc, slots), counts = probe(f, index, fields, lhs, rhs, mode, -1)
                        self.assertIsNone(exc)
                        out = slots[slot_of(fields, "A")]
                        self.assertEqual(out, lhs + rhs)
                        self.assertIs(type(out), type(lhs))
                        # Other operand types leave the arm for the generic one
                        f, index, fields = adapted(kind, lhs, rhs, "Add.Extended")
                        (_, _, exc, _), counts = probe(f, index, fields, lhs, 5, mode, -1)
                        if mode == STUB:
                            (_, _, exc, _), counts = probe(
                                f, index, fields, lhs, 5, mode, arms()["Add.Generic"])
                        self.assertIsInstance(exc, TypeError)


class AdaptationTests(unittest.TestCase):
    def test_generic_installs_int_through_a_ticket(self):
        generic = arms()["Add.Generic"]
        add_int = arms()["Add.Int"]
        f, index, fields = compiled("locals")
        for i in range(2000):
            slots = {slot_of(fields, "B"): 3, slot_of(fields, "C"): 4}
            nxt, *_ = capi.jit_template_run(f, index, slots, FORM, generic)
            if handler_of(f, index) == add_int:
                break
            self.assertEqual(nxt, index + 1)
        else:
            self.fail("the word never selected Add.Int")
        # The ticket was the redispatch: the word that installed it ran again
        self.assertEqual(nxt, index)

    def test_non_int_pairs_back_off_and_stay_generic(self):
        generic = arms()["Add.Generic"]
        f, index, fields = compiled("locals")
        for i in range(2000):
            slots = {slot_of(fields, "B"): Bump(1), slot_of(fields, "C"): 2}
            nxt, _, exc, out = capi.jit_template_run(f, index, slots, FORM, generic)
            self.assertIsNone(exc)
            self.assertEqual(nxt, index + 1)
            self.assertEqual(out[slot_of(fields, "A")].n, 3)


if __name__ == "__main__":
    unittest.main()
