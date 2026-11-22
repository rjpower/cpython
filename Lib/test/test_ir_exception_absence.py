"""Exceptional prefix absence needs an independent abrupt-write contract."""

import marshal
from test.support import ir_dis as opcode
import struct
import types
import unittest

import _testinternalcapi
import _opcode
from test.support.ir_semantics import observation_tables, position_parts, semantic_parts
from test.test_ir_admission import construct
from test.test_ir_call_roles import routes
from test.test_ir_completion import exception_entry
from test.test_ir_transfer_admission import encoded, template_code


class ExceptionAbsenceTests(unittest.TestCase):
    def image(self):
        # The normal arm clears self home 1 again. Only the exceptional arm
        # needs its incoming absence retained below the suspension footprint.
        words = [("LOAD_CONST", 0), ("POP_TOP", 1), ("LOAD_CONST", 4),
                 ("YIELD_VALUE", 4, 4, 0), ("POP_TOP", 4), ("POP_TOP", 1),
                 ("CALL+NO_SELF", 0, 0, 0, 0), ("RETURN_VALUE", 0),
                 ("POP_TOP", 6), ("POP_TOP", 4), ("POP_TOP", 5),
                 ("IR_JUMP", 0, 0, 6)]
        table = exception_entry(3, 1, 8, 6 << 1)
        # Raw images are constructed but never executed: payloads need not
        # actually be callable or awaitable for the ownership controls.
        code = template_code.replace(
            co_code=encoded(words), co_stacksize=7, co_linetable=b"",
            co_exceptiontable=table, co_fbinit=struct.pack("<I", 2))
        return code, words

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

        self.assertTrue(any(row[0] == "CALL_KW+NO_SELF"
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
