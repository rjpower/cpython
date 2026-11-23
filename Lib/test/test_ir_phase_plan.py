"""Collector walks reached from a generic BINARY_OP callback."""

import marshal
import types
import unittest

import _testinternalcapi
from test.test_ir_admission import construct


def binary_code():
    # A real language value survives below the two operands, in a scratch
    # home rather than a hidden constant binding or native snapshot.
    def template(left, right):
        return ["surviving scratch owner", left + right]

    rows = _testinternalcapi.get_ir(template.__code__)
    assert [row[0] for row in rows if row[0].startswith("BINARY_OP")] == ["BINARY_OP_ADD"]
    return template.__code__


def routes(code):
    return (code, construct(code), code.replace(),
            marshal.loads(marshal.dumps(code)))


class CollectorWalkTests(unittest.TestCase):
    def setUp(self):
        if _testinternalcapi.gc_verify() is None:
            self.skipTest("the tracing frame-root verifier cannot run in this build")

    def test_provider_and_operand_finalizers_use_nonempty_live_shapes(self):
        code = binary_code()
        for admitted in routes(code):
            observations = []

            def record(label):
                # This invokes the actual collector frame-root walk. The
                # observation API below only reads the status it stored.
                observations.append((label, _testinternalcapi.gc_verify()))

            class Operand:
                def __add__(self, other):
                    record("provider")
                    return ["result"]

                def __del__(self):
                    record("operand")

            f = types.FunctionType(admitted, {})
            self.assertEqual(f(Operand(), Operand()),
                             ["surviving scratch owner", ["result"]])
            self.assertEqual([label for label, _ in observations],
                             ["provider", "operand", "operand"])
            for label, holes in observations:
                self.assertEqual(holes, [])

    def test_folded_bindings_remain_dynamic_during_error_restore(self):
        import sys
        code = binary_code()
        observations = []
        bindings = []

        def record(label):
            observations.append((label, _testinternalcapi.gc_verify()))

        class Operand:
            def __add__(self, other):
                record("provider")
                target = sys._getframe(1)
                target.f_locals["left"] = None
                target.f_locals["right"] = None
                bindings.append((target.f_locals["left"], target.f_locals["right"]))
                record("mutated")
                # Let the native numeric provider raise after this Python
                # frame has returned, so its traceback cannot retain owners.
                return NotImplemented

            def __del__(self):
                record("operand")

        f = types.FunctionType(code, {})
        with self.assertRaisesRegex(TypeError, "unsupported operand"):
            f(Operand(), Operand())
        self.assertEqual(bindings, [(None, None)])
        self.assertEqual([label for label, _ in observations],
                         ["provider", "mutated", "operand", "operand"])
        for label, holes in observations:
            self.assertEqual(holes, [])

    def test_error_restored_scratch_owners_survive_until_unwind(self):
        code = binary_code()
        observations = []

        def record(label):
            observations.append((label, _testinternalcapi.gc_verify()))

        class Operand:
            def __add__(self, other):
                record("provider")
                return NotImplemented

            def __del__(self):
                record("operand")

        f = types.FunctionType(code, {})
        with self.assertRaisesRegex(TypeError, "unsupported operand"):
            f(Operand(), Operand())
        self.assertEqual([label for label, _ in observations],
                         ["provider", "operand", "operand"])
        for label, holes in observations:
            self.assertEqual(holes, [])


if __name__ == "__main__":
    unittest.main()
