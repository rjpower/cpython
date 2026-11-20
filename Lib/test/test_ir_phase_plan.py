"""Collector walks reached from a generic BINARY_OP callback."""

import marshal
import types
import unittest

import _testinternalcapi
from test.test_ir_admission import construct
from test.test_ir_transfer_admission import encoded, raw_encoded


def binary_code(*, slots=4, move=True):
    def template(left, right):
        return left + right

    code = template.__code__
    rows = _testinternalcapi.get_ir(code)
    binary = [row for row in rows if row[0].startswith("BINARY_OP")]
    assert len(binary) == 1
    assert binary[0][0] == "BINARY_OP_ADD"
    resume = [row for row in rows if row[0] == "RESUME"]
    assert len(resume) == 1
    # A real language value survives below the two operands. Keep it in a
    # scratch home rather than a hidden constant binding or native snapshot.
    words = [tuple(resume[0]), ("LOAD_CONST", 2)]
    left, right = 0, 1
    if move:
        words += [("LOAD_FAST_AND_CLEAR", 3, 0),
                  ("LOAD_FAST_AND_CLEAR", 4, 1)]
        left, right = 3, 4
    destination = slots + 1
    site = len(words)
    words += [(binary[0][0], destination, left, right, binary[0][4]),
              ("POP_TOP", 2), ("RETURN_VALUE", destination)]
    changed = code.replace(co_code=encoded(words, code.co_nlocals), co_stacksize=slots,
                           co_consts=(["surviving scratch owner"],),
                           co_linetable=b"", co_exceptiontable=b"")
    return changed, site, destination


def routes(code):
    return (code, construct(code), code.replace(),
            marshal.loads(marshal.dumps(code)))


def setUpModule():
    # These tests construct malformed images and expect the constructor to
    # reject them; release builds only do so on request.
    from test.support.ir_semantics import code_validation
    unittest.enterModuleContext(code_validation())


class AdmissionTests(unittest.TestCase):
    def test_event_admission_rejects_duplicate_scratch_take(self):
        code, site, _ = binary_code()
        words = _testinternalcapi.get_ir(code)
        bad = list(words)
        handler, dst, left, right, fb = bad[site]
        self.assertNotEqual(left, right)
        bad[site] = (handler, dst, left, left, fb)
        replacement = raw_encoded(bad)
        for route in (construct, lambda co, **kw: co.replace(**kw)):
            with self.assertRaisesRegex(ValueError, "reads unavailable scratch"):
                route(code, co_code=replacement)
        payload = marshal.dumps(code)
        self.assertEqual(payload.count(code.co_code), 1)
        with self.assertRaisesRegex(ValueError, "reads unavailable scratch"):
            marshal.loads(payload.replace(code.co_code, replacement, 1))


class CollectorWalkTests(unittest.TestCase):
    def setUp(self):
        if _testinternalcapi.gc_verify() is None:
            self.skipTest("the tracing frame-root verifier cannot run in this build")

    def test_provider_and_operand_finalizers_use_nonempty_live_shapes(self):
        code, site, _ = binary_code()
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
            self.assertEqual(f(Operand(), Operand()), ["result"])
            self.assertEqual([label for label, _ in observations],
                             ["provider", "operand", "operand"])
            for label, holes in observations:
                self.assertEqual(holes, [])

    def test_folded_bindings_remain_dynamic_during_error_restore(self):
        import sys
        code, site, _ = binary_code(move=False)
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
        code, site, _ = binary_code()
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
