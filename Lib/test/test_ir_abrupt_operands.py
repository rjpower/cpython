"""Ordinary error transport confines writes to actual encoded operands."""

import marshal
from pathlib import Path
import tempfile
import types
import unittest
from unittest import mock

import _opcode
import _testinternalcapi
from test.support import ir_dis as opcode

from test.test_ir_call_roles import routes


class AbruptOperandTests(unittest.TestCase):
    def test_nonwindow_peek_retains_its_input(self):
        def match(subject):
            match subject:
                case [first, *rest]:
                    return first, rest
                case _:
                    return None

        self.assertEqual(match([1, 2, 3]), (1, [2, 3]))
        self.assertIsNone(match(5))
        names = {row[0] for row in _testinternalcapi.get_ir(match.__code__, True)}
        self.assertIn("GET_LEN", names)
        # The subject outlives the peek of its length
        subject = [1, 2]
        for code in routes(match.__code__):
            function = types.FunctionType(code, globals())
            self.assertEqual(function(subject), (1, [2]))
            self.assertEqual(subject, [1, 2])

    def test_window_scalar_fields_are_trailing_inputs_not_through_indices(self):
        # The comprehensions run with items already pending below them,
        # so the appended-to container sits at several distances from the top.
        def lists(values):
            return (0, 1, 2, 3, 4, 5, 6, [v for v in values]), 7

        def sets(values):
            return (0, 1, 2, 3, 4, 5, 6, {v for v in values}), 7

        for function, container in ((lists, list), (sets, set)):
            with self.subTest(function=function.__name__):
                for code in routes(function.__code__):
                    names = {row[0] for row in _testinternalcapi.get_ir(code, True)}
                    self.assertTrue(names & {"LIST_APPEND", "SET_ADD"}, names)
                    result = types.FunctionType(code, globals())([3, 4, 3])
                    self.assertEqual(result, ((0, 1, 2, 3, 4, 5, 6, container([3, 4, 3])), 7))

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
        self.assertIn("CALL_KW+NO_SELF", names)
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

            def suppress_one(emitter, stack):
                if emitter.abrupt_operand_origin != missing:
                    original_flush(emitter, stack)

            with mock.patch.object(ir_generator.IREmitter, "operand_error_flush", suppress_one):
                self.assertFalse(ir_generator.handler_contracts(multiple, multiple_formats)["GET_ANEXT"].abrupt_operands)
            for change in changes:
                self.assertNotEqual(change, body)
                changed.write_text(text[:begin] + change + text[end:])
                altered = analyzer.analyze_files([str(changed)])
                self.assertIsNone(ir_generator.operand_error_sources(altered.instructions["GET_ANEXT"]))


if __name__ == "__main__":
    unittest.main()
