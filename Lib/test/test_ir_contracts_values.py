"""Native tests for the value and number rows that batch 1's uops rely on
(G0c): the identity tests against the C predicates on generated words, and
the ownership, failure and reentry claims of the helpers and of POP_TOP,
as the handlers show them."""

import gc
import sys
import unittest
from collections.abc import Sequence

from test.support import import_helper
from test import test_tools

with test_tools.imports_under_tool("cases_generator"):
    import ir_contract


class Raises:
    "an object whose protocol slots fail, and say that they ran"

    def __init__(self):
        self.calls = []

    def __bool__(self):
        self.calls.append("bool")
        raise ZeroDivisionError("bool")

    def __len__(self):
        self.calls.append("len")
        raise ZeroDivisionError("len")

    def __neg__(self):
        self.calls.append("neg")
        raise ZeroDivisionError("neg")

    def __invert__(self):
        self.calls.append("invert")
        raise ZeroDivisionError("invert")


Sequence.register(Raises)


class IdentityRowTests(unittest.TestCase):
    """PyValue_IsNone, IsTrue and IsFalse declare TEST(is(K)): a true result
    names the constant K.  The C predicate holds on exactly K's word"""

    def test_is_tests_against_c(self):
        capi = import_helper.import_module("_testinternalcapi")
        if not hasattr(capi, "leaf_primitive"):
            self.skipTest("no leaf primitives")
        from test import test_ir_leaf as leaf
        constants = {"PyValue_None": None, "PyValue_True": True, "PyValue_False": False}
        rows = ir_contract.rows()
        checked = 0
        for name, row in rows.items():
            facts = {f for true, false in row.result.tests for f in true | false}
            named = [f.arg for f in facts if f.kind == "is"]
            if not named:
                continue
            with self.subTest(row=name):
                self.assertEqual(len(named), 1)
                bits = leaf.word(constants[named[0]])
                for w in leaf.ANY_WORDS + [bits]:
                    self.assertEqual(bool(capi.leaf_primitive(name, w)), w == bits,
                                     f"{name}({w:#x})")
                checked += 1
        self.assertEqual(checked, 3)


class ProtocolFailureTests(unittest.TestCase):
    """_PyValue_IsTrue, _PyValue_Size, _PyValue_Negative and _PyValue_Invert
    run the type's slot (runs_python), fail only with an exception set, and
    borrow their argument on every path: a failure leaves its count alone"""

    def check(self, operation, slot):
        obj = Raises()
        for _ in range(3):
            before = sys.getrefcount(obj)
            with self.assertRaises(ZeroDivisionError):
                operation(obj)
            self.assertEqual(sys.getrefcount(obj), before)
        self.assertEqual(obj.calls, [slot] * 3)

    def test_truth(self):
        def to_bool(x):
            if x:
                return 1
        def unary_not(x):
            return not x
        self.check(to_bool, "bool")
        self.check(unary_not, "bool")

    def test_len(self):
        def match_len(x):
            match x:
                case [_, *_]:
                    return 1
        self.check(match_len, "len")

    def test_unary(self):
        self.check(lambda x: -x, "neg")
        self.check(lambda x: ~x, "invert")

    def test_unary_failure_without_slot(self):
        for operation in (lambda x: -x, lambda x: ~x):
            with self.assertRaises(TypeError):
                operation(object())
            with self.assertRaises(TypeError):
                operation("text")


class PopTopTests(unittest.TestCase):
    """POP_TOP declares releases, not pure: closing the last count runs a
    finalizer.  Where the release happens is not part of the contract (the
    register IR may close a temporary later), so the test holds only that
    every discarded value is finalized once and that no frame is lost"""

    def test_discarded_values_are_finalized(self):
        seen = []

        class Finalizer:
            def __del__(self):
                seen.append(sys._getframe(1).f_code)

        def run():
            for _ in range(3):
                Finalizer()      # POP_TOP closes the only count
            return 1

        self.assertEqual(run(), 1)
        gc.collect()
        self.assertEqual(len(seen), 3)

    def test_unbound_local_check(self):
        def run(flag):
            if flag:
                x = 1
            return x

        self.assertEqual(run(True), 1)
        with self.assertRaises(UnboundLocalError):
            run(False)


if __name__ == "__main__":
    unittest.main()
