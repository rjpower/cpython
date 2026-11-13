"""Ordinary error transport confines writes to actual encoded operands."""

import marshal
from pathlib import Path
import tempfile
import types
import unittest
from unittest import mock

import _opcode
import _testinternalcapi
import opcode

from test.test_ir_admission import construct
from test.test_ir_call_roles import routes
from test.test_ir_completion import exception_entry
from test.test_ir_transfer_admission import encoded, make_code


class AbruptOperandTests(unittest.TestCase):
    def image(self):
        words = [("LOAD_CONST", 0), ("POP_TOP", 1), ("LOAD_CONST", 4),
                 ("GET_ANEXT", 5, 4), ("POP_TOP", 5), ("POP_TOP", 1),
                 ("CALL_NO_SELF", 0, 0, 0, 0), ("RETURN_VALUE", 0),
                 ("POP_TOP", 6), ("POP_TOP", 4), ("POP_TOP", 5),
                 ("IR_JUMP", 0, 0, 6)]
        table = exception_entry(3, 1, 8, 6 << 1)
        code = make_code(words, slots=7).replace(co_exceptiontable=table)
        return code, words

    def reject(self, code, **change):
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

    def test_error_prefix_requires_prior_absence_below_every_operand(self):
        code, words = self.image()
        row = _opcode.get_ir_contract()[opcode.opmap["GET_ANEXT"]]
        self.assertTrue(row.flags & _opcode.IR_ABRUPT_OPERANDS)
        self.assertFalse(row.flags & _opcode.IR_ABRUPT_PREFIX)
        for admitted in routes(code):
            self.assertEqual(_testinternalcapi.get_ir(admitted, True),
                             _testinternalcapi.get_ir(code))
        live = words.copy()
        live[1] = ("LOAD_CONST", 1)
        self.reject(code, co_code=encoded(live))
        # A scratch result recipient can be touched by publication/restore;
        # it is not protected merely because it lies below the input.
        lower = words.copy()
        lower[3] = ("GET_ANEXT", 1, 4)
        self.reject(code, co_code=encoded(lower))
        self.reject(code, co_exceptiontable=exception_entry(3, 2, 8, 6 << 1))

    def test_nonwindow_peek_retains_its_input(self):
        for name in ("GET_LEN", "GET_ANEXT"):
            with self.subTest(handler=name):
                code = make_code([("LOAD_CONST", 1), (name, 2, 1),
                                  ("POP_TOP", 2), ("RETURN_VALUE", 1)])
                for admitted in routes(code):
                    _testinternalcapi.get_ir(admitted, True)
                # Reading a kept input is allowed; treating that same live
                # home as a virtual hole is not.
                valid = [("LOAD_CONST", 0), ("LOAD_CONST", 1), (name, 2, 1),
                         ("POP_TOP", 2), ("POP_TOP", 1),
                         ("CALL_NO_SELF", 0, 0, 0, 0), ("RETURN_VALUE", 0)]
                base = make_code(valid)
                for admitted in routes(base):
                    _testinternalcapi.get_ir(admitted, True)
                bad = valid.copy()
                bad[4] = ("IR_JUMP", 0, 0, 5)
                self.reject(base, co_code=encoded(bad))
                # An output that aliases the input is independently a write.
                bad[2] = (name, 1, 1)
                self.reject(base, co_code=encoded(bad))

    def test_actual_anext_exception_keeps_a_held_keyword_call_hole(self):
        class Pause:
            def __await__(self):
                yield "pause"

        class Values:
            def __init__(self, values, *, fail=False):
                self.values = iter(values)
                self.fail = fail

            def __aiter__(self):
                return self

            def __anext__(self):
                if self.fail:
                    raise LookupError("anext source")
                value = next(self.values, self)
                if value is self:
                    # Raised by GET_ANEXT itself, not a later SEND/YIELD.
                    raise StopAsyncIteration

                async def ready():
                    await Pause()
                    return value
                return ready()

        async def invoke(function, source):
            return function(value=[item async for item in source])

        names = {row[0] for row in _testinternalcapi.get_ir(invoke.__code__)}
        self.assertIn("CALL_KW_NO_SELF", names)
        for code in routes(invoke.__code__):
            function = types.FunctionType(code, globals())
            for values in ((), (17, 23)):
                coro = function(lambda *, value: value, Values(values))
                try:
                    while True:
                        self.assertEqual(coro.send(None), "pause")
                except StopIteration as ended:
                    self.assertEqual(ended.value, list(values))
                finally:
                    coro.close()
            coro = function(lambda *, value: value, Values((), fail=True))
            try:
                with self.assertRaisesRegex(LookupError, "anext source"):
                    coro.send(None)
            finally:
                coro.close()

    def test_helper_provenance_and_each_actual_error_flush_are_required(self):
        from test import test_tools
        test_tools.skip_if_missing("cases_generator")
        with test_tools.imports_under_tool("cases_generator"):
            import analyzer
            import ir_generator

        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        if not source.is_file():
            self.skipTest("requires bytecodes.c")
        analysis = analyzer.analyze_files([str(source)])
        formats = ir_generator.compute_formats(analysis)
        inst = analysis.instructions["GET_ANEXT"]
        expected = ir_generator.operand_error_sources(inst)
        self.assertEqual(len(expected), 1)
        name = inst.name
        inst.name = "RENAMED_VALUE_HELPER"
        try:
            self.assertEqual(ir_generator.operand_error_sources(inst), expected)
        finally:
            inst.name = name
        self.assertTrue(ir_generator.handler_contracts(analysis, formats)["GET_ANEXT"].abrupt_operands)
        # A matching source grammar without its emitted error stores does
        # not certify an exceptional route.
        with mock.patch.object(ir_generator.IREmitter, "operand_error_flush", lambda *args: None):
            self.assertFalse(ir_generator.handler_contracts(analysis, formats)["GET_ANEXT"].abrupt_operands)
        text = source.read_text()
        begin = text.index("inst(GET_ANEXT,")
        end = text.index("inst(GET_AWAITABLE,", begin)
        body = text[begin:end]
        changes = (
            body.replace("_PyEval_GetANext", "unknown_native_helper", 1),
            body.replace("PyValue_AsKnownObjectBorrow(aiter)", "alias", 1),
            body.replace("PyValue_AsKnownObjectBorrow(aiter)", "&aiter", 1),
            body.replace("ERROR_NO_POP();", "opaque_error(alias);", 1),
            body.replace("awaitable =", "_PyFrame_SetReturnValue(alias, PyValue_None); awaitable =", 1),
            body.replace("awaitable =", "PyValue *alias = &aiter; awaitable =", 1),
        )
        with tempfile.TemporaryDirectory() as directory:
            changed = Path(directory) / "bytecodes.c"
            two = body.replace("awaitable =", "if (awaitable_o == NULL) { ERROR_NO_POP(); } awaitable =", 1)
            changed.write_text(text[:begin] + two + text[end:])
            multiple = analyzer.analyze_files([str(changed)])
            multiple_formats = ir_generator.compute_formats(multiple)
            sources = ir_generator.operand_error_sources(multiple.instructions["GET_ANEXT"])
            self.assertEqual(len(sources), 2)
            self.assertTrue(ir_generator.handler_contracts(multiple, multiple_formats)["GET_ANEXT"].abrupt_operands)
            missing = next(iter(sources))
            original_flush = ir_generator.IREmitter.operand_error_flush

            def suppress_one(emitter, stack, bridge=None):
                if emitter.abrupt_operand_origin != missing:
                    original_flush(emitter, stack, bridge)

            with mock.patch.object(ir_generator.IREmitter, "operand_error_flush", suppress_one):
                self.assertFalse(ir_generator.handler_contracts(multiple, multiple_formats)["GET_ANEXT"].abrupt_operands)
            for change in changes:
                self.assertNotEqual(change, body)
                changed.write_text(text[:begin] + change + text[end:])
                altered = analyzer.analyze_files([str(changed)])
                self.assertIsNone(ir_generator.operand_error_sources(altered.instructions["GET_ANEXT"]))


if __name__ == "__main__":
    unittest.main()
