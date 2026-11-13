"""Only a successfully matched async-loop exit retires its input window."""

import marshal
from pathlib import Path
import tempfile
import types
import unittest

import _testinternalcapi
from test.test_ir_admission import construct
from test.test_ir_call_roles import routes
from test.test_ir_transfer_admission import encoded, make_code


class AsyncExitTransferTests(unittest.TestCase):
    def test_normal_exit_drains_inputs_and_preserves_only_prior_empty_tail(self):
        # The unreachable SEND is a real metadata target, not a normal edge
        # from END_ASYNC_FOR. These raw images are never executed.
        words = [("LOAD_CONST", 0), ("LOAD_CONST", 1), ("POP_TOP", 2),
                 ("END_ASYNC_FOR", 0, 0, 8, 0), ("LOAD_CONST", 0),
                 ("CALL_NO_SELF", 0, 0, 0, 0), ("RETURN_VALUE", 0),
                 ("IR_JUMP", 0, 0, 7), ("SEND", 0, 0, 0, 0),
                 ("RETURN_VALUE", 0)]
        code = make_code(words, slots=4)
        for admitted in routes(code):
            self.assertEqual(_testinternalcapi.get_ir(admitted, True),
                             _testinternalcapi.get_ir(code))
        words[2] = ("LOAD_CONST", 2)
        changed = encoded(words)
        for route in (construct, lambda co, **kw: co.replace(**kw)):
            with self.assertRaisesRegex(ValueError, "child clobber home"):
                route(code, co_code=changed)
        payload = marshal.dumps(code)
        self.assertEqual(payload.count(code.co_code), 1)
        with self.assertRaisesRegex(ValueError, "child clobber home"):
            marshal.loads(payload.replace(code.co_code, changed, 1))

    def test_nested_async_loop_reuses_call_window_on_exhaustion(self):
        class Pause:
            def __await__(self):
                yield "pause"

        class Values:
            def __init__(self, values, *, error=False):
                self.values = iter(values)
                self.error = error

            def __aiter__(self):
                return self

            async def __anext__(self):
                await Pause()
                if self.error:
                    raise LookupError("async source")
                value = next(self.values, self)
                if value is self:
                    raise StopAsyncIteration
                return value

        async def outer(fn, groups):
            result = []
            for group in groups:
                async for value in group:
                    result.append(value)
                fn(17)
            return result

        names = {row[0] for row in _testinternalcapi.get_ir(outer.__code__)}
        self.assertTrue({"END_ASYNC_FOR", "CALL_NO_SELF"} <= names)
        for admitted in routes(outer.__code__):
            function = types.FunctionType(admitted, globals())
            seen = []
            coroutine = function(seen.append, (Values(()), Values((1, 2))))
            try:
                while True:
                    self.assertEqual(coroutine.send(None), "pause")
            except StopIteration as ended:
                self.assertEqual(ended.value, [1, 2])
            finally:
                coroutine.close()
            self.assertEqual(seen, [17, 17])
            coroutine = function(seen.append, (Values((), error=True),))
            try:
                self.assertEqual(coroutine.send(None), "pause")
                with self.assertRaisesRegex(LookupError, "async source"):
                    coroutine.send(None)
            finally:
                coroutine.close()
            self.assertEqual(seen, [17, 17])

    def test_actual_matched_retirement_and_monitor_source_are_required(self):
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
        names = ("END_ASYNC_FOR", "INSTRUMENTED_END_ASYNC_FOR")
        expected = (ir_generator.OwnershipEvent("_PyIR_OWNER_NORMAL_CLEAR_INPUTS"),)
        contracts = ir_generator.handler_contracts(analysis, formats)
        for name in names:
            self.assertTrue(ir_generator.async_exit_transport(analysis.instructions[name], formats[name]))
            self.assertEqual(contracts[name].ownership_events, expected)
        # Neither the instruction's name nor a matching IO declaration is a
        # permission to ignore an extra write, callback or altered target.
        text = source.read_text()
        marker = "tier1 window op(_END_ASYNC_FOR,"
        start = text.index(marker)
        end = text.index("macro(END_ASYNC_FOR) = _END_ASYNC_FOR;", start)
        original = text[start:end]
        changes = (
            original.replace("DECREF_INPUTS();", "opaque(alias);", 1),
            original.replace("DECREF_INPUTS();", "DECREF_INPUTS(); frame->base[0] = PyValue_NULL;", 1),
            original.replace("PyExc_StopAsyncIteration", "PyExc_StopIteration", 1),
            original.replace("Py_INCREF(exc);", "Py_XINCREF(exc);", 1),
            original.replace("JUMP_TARGET(oparg), this_instr+1", "JUMP_TARGET(oparg), this_instr+2", 1),
            original.replace("_PyIR_RealHandler(frame,", "_PyIR_RealHandler(other,", 1),
        )
        with tempfile.TemporaryDirectory() as directory:
            altered = Path(directory) / "bytecodes.c"
            for change in changes:
                self.assertNotEqual(change, original)
                altered.write_text(text[:start] + change + text[end:])
                changed = analyzer.analyze_files([str(altered)])
                changed_formats = ir_generator.compute_formats(changed)
                changed_contracts = ir_generator.handler_contracts(changed, changed_formats)
                for name in names:
                    self.assertNotEqual(changed_contracts[name].ownership_events, expected)


if __name__ == "__main__":
    unittest.main()
