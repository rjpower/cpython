"""Exceptional prefix absence needs an independent abrupt-write contract."""

import marshal
import opcode
import struct
import types
import unittest

import _testinternalcapi
import _opcode
from test.test_ir_admission import construct
from test.test_ir_call_roles import routes
from test.test_ir_completion import exception_entry
from test.test_ir_transfer_admission import encoded


class ExceptionAbsenceTests(unittest.TestCase):
    def image(self):
        # The normal arm clears self home 1 again. Only the exceptional arm
        # needs its incoming absence retained below the suspension footprint.
        words = [("LOAD_CONST", 0), ("POP_TOP", 1), ("LOAD_CONST", 4),
                 ("YIELD_VALUE", 4, 4, 0), ("POP_TOP", 4), ("POP_TOP", 1),
                 ("CALL_NO_SELF", 0, 0, 0, 0), ("RETURN_VALUE", 0),
                 ("POP_TOP", 6), ("POP_TOP", 4), ("POP_TOP", 5),
                 ("IR_JUMP", 0, 0, 6)]
        table = exception_entry(3, 1, 8, 6 << 1)
        # Raw images are constructed but never executed: payloads need not
        # actually be callable or awaitable for the ownership controls.
        code = (lambda: None).__code__.replace(
            co_code=encoded(words), co_stacksize=7, co_linetable=b"",
            co_exceptiontable=table, co_fbinit=struct.pack("<I", 2))
        return code, words

    def rejected(self, code, **change):
        for route in (construct, lambda co, **kw: co.replace(**kw)):
            with self.assertRaisesRegex(ValueError, "virtual call input"):
                route(code, **change)
        key, replacement = next(iter(change.items()))
        original = getattr(code, key)
        self.assertEqual(len(original), len(replacement))
        payload = marshal.dumps(code)
        self.assertEqual(payload.count(original), 1)
        with self.assertRaisesRegex(ValueError, "virtual call input"):
            marshal.loads(payload.replace(original, replacement, 1))

    def test_suspension_retains_only_incoming_prefix_absence(self):
        code, words = self.image()
        contract = _opcode.get_ir_contract()[opcode.opmap["YIELD_VALUE"]]
        self.assertTrue(contract.flags & _opcode.IR_ABRUPT_PREFIX)
        for admitted in routes(code):
            self.assertEqual(_testinternalcapi.get_ir(admitted, True),
                             _testinternalcapi.get_ir(code))
        live = words.copy()
        live[1] = ("LOAD_CONST", 1)
        self.rejected(code, co_code=encoded(live))
        # Normal input retirement does not certify an exceptional footprint.
        unknown = words.copy()
        unknown[3] = ("UNARY_NOT", 4, 4)
        self.rejected(code, co_code=encoded(unknown))
        # An in-bounds resume recipient can itself clobber the held hole.
        aliases = words.copy()
        aliases[3] = ("YIELD_VALUE", 1, 4, 0)
        self.rejected(code, co_code=encoded(aliases))

    def test_complete_inert_exception_predecessor_is_not_an_ingress(self):
        code, _ = self.image()
        contract = _opcode.get_ir_contract()[opcode.opmap["POP_TOP"]]
        self.assertEqual(contract.error_routes, _opcode.IR_ERROR_COMPLETE)
        # The protected POP has no emitted exceptional route. Its lack of an
        # abrupt-prefix receipt must not erase the YIELD predecessor's facts.
        extended = code.replace(
            co_exceptiontable=exception_entry(3, 2, 8, 6 << 1))
        for admitted in routes(extended):
            _testinternalcapi.get_ir(admitted, True)

    def test_uncovered_exception_predecessor_poison_is_not_skipped(self):
        code, words = self.image()
        # Keep the normal path balanced while adding a genuinely uncovered
        # protected producer. Its absence of a route census remains unknown.
        words[4] = ("UNARY_NOT", 4, 4)
        words[5] = ("POP_TOP", 4)
        unknown = code.replace(co_code=encoded(words), co_linetable=b"")
        self.assertFalse(_opcode.get_ir_contract()[
            opcode.opmap["UNARY_NOT"]].error_routes & _opcode.IR_ERROR_COMPLETE)
        self.rejected(unknown,
                      co_exceptiontable=exception_entry(3, 2, 8, 6 << 1))

    def test_unknown_source_recovery_is_an_independent_predecessor(self):
        code, _ = self.image()
        # Start with the same edge carrying no recovery handler, so the
        # serialized negative can change only fixed-size record fields.
        record = (3, 4, 0, 1, 1, 0, 1, -1, 0, 0)
        source = b"IRSM" + struct.pack("<III", 1, 0, 1)
        source += struct.pack("<10i", *record)
        code = code.replace(co_linetable=source)
        for admitted in routes(code):
            _testinternalcapi.get_ir(admitted, True)
        changed = bytearray(code.co_linetable)
        struct.pack_into("<iii", changed, len(changed) - 12, 8, 6, 0)
        self.rejected(code, co_linetable=bytes(changed))

    def test_real_keyword_call_held_across_await_and_throw_recovery(self):
        class Awaitable:
            def __await__(self):
                try:
                    yield "paused"
                except ValueError:
                    return "recovered"
                return "done"

        def ident(*, val):
            return val

        async def foo():
            return ident(val=await Awaitable())

        self.assertTrue(any(row[0] == "CALL_KW_NO_SELF"
                            for row in _testinternalcapi.get_ir(foo.__code__)))
        for code in routes(foo.__code__):
            function = types.FunctionType(code, globals(), closure=foo.__closure__)
            for throw, expected in ((False, "done"), (True, "recovered")):
                with self.subTest(throw=throw):
                    coroutine = function()
                    try:
                        self.assertEqual(coroutine.send(None), "paused")
                        with self.assertRaises(StopIteration) as result:
                            if throw:
                                coroutine.throw(ValueError())
                            else:
                                coroutine.send(None)
                        self.assertEqual(result.exception.value, expected)
                    finally:
                        coroutine.close()


if __name__ == "__main__":
    unittest.main()
