"""The common immutable-image proof selects and validates empty returns."""

import marshal
import sys
from pathlib import Path
import types
import unittest

import _testinternalcapi

from test.test_ir_admission import construct
from test.test_ir_call_roles import routes
from test.test_ir_transfer_admission import encoded, make_code


class EmptyReturnTests(unittest.TestCase):
    def test_all_admission_routes_select_the_proven_empty_form(self):
        for slots in (1, 65, 4095):
            with self.subTest(slots=slots):
                words = [("LOAD_CONST", 0), ("RETURN_VALUE", 0)]
                supplied = encoded(words)
                saved = supplied[:]
                code = make_code(words, slots=slots)
                self.assertEqual(supplied, saved)
                self.assertNotEqual(code.co_code, supplied)
                for admitted in routes(code):
                    self.assertEqual(_testinternalcapi.get_ir(admitted)[-1][0],
                                     "RETURN_VALUE_EMPTY")
                    self.assertIsNone(types.FunctionType(admitted, {})())
                # Replacing an explicitly general word must use the same
                # proof, without modifying the caller's shared bytes object.
                self.assertEqual(code.replace(co_code=supplied).co_code, code.co_code)
                self.assertEqual(construct(code, co_code=supplied).co_code, code.co_code)
                self.assertEqual(supplied, saved)

    def test_remaining_owner_keeps_general_return_and_forbids_empty_claim(self):
        words = [("LOAD_CONST", 1), ("LOAD_CONST", 0), ("RETURN_VALUE", 0)]
        code = make_code(words)
        for admitted in routes(code):
            self.assertEqual(_testinternalcapi.get_ir(admitted)[-1][0], "RETURN_VALUE")
        bad = encoded(words[:-1] + [("RETURN_VALUE_EMPTY", 0)])
        for route in (construct, lambda co, **kw: co.replace(**kw)):
            with self.assertRaisesRegex(ValueError, "unproved empty return"):
                route(code, co_code=bad)
        payload = marshal.dumps(code)
        self.assertEqual(payload.count(code.co_code), 1)
        with self.assertRaisesRegex(ValueError, "unproved empty return"):
            marshal.loads(payload.replace(code.co_code, bad, 1))

    def test_unknown_and_unreachable_sites_never_certify_empty(self):
        # UNARY_NOT has no complete owner-transfer recipe. A non-result
        # scratch home touched by it is not a positive vacancy fact.
        words = [("LOAD_CONST", 1), ("UNARY_NOT", 1, 1),
                 ("LOAD_CONST", 0), ("RETURN_VALUE", 0)]
        code = make_code(words)
        self.assertEqual(_testinternalcapi.get_ir(code)[-1][0], "RETURN_VALUE")
        bad = encoded(words[:-1] + [("RETURN_VALUE_EMPTY", 0)])
        with self.assertRaisesRegex(ValueError, "unproved empty return"):
            code.replace(co_code=bad)
        unreachable = [("LOAD_CONST", 0), ("RETURN_VALUE", 0),
                       ("RETURN_VALUE", 0)]
        code = make_code(unreachable)
        rows = _testinternalcapi.get_ir(code)
        self.assertEqual(rows[1][0], "RETURN_VALUE_EMPTY")
        self.assertEqual(rows[2][0], "RETURN_VALUE")
        bad = bytearray(code.co_code)
        bad[-8:] = encoded([("RETURN_VALUE_EMPTY", 0)])
        with self.assertRaisesRegex(ValueError, "unproved empty return"):
            code.replace(co_code=bytes(bad))

    def test_monitoring_keeps_the_selected_return_form(self):
        monitor = sys.monitoring
        tool = 2
        event = monitor.events.PY_RETURN
        seen = []
        monitor.use_tool_id(tool, "general and empty return forms")
        monitor.register_callback(tool, event,
                                  lambda code, offset, value: seen.append((code, value)))
        try:
            for general in (False, True):
                # A real entry prologue makes this a complete instrumentable
                # image; the raw fixture otherwise has no RESUME entry.
                words = [("RESUME", 0)]
                if general:
                    words.append(("LOAD_CONST", 1))
                words += [("LOAD_CONST", 0), ("RETURN_VALUE", 0)]
                for code in routes(make_code(words)):
                    function = types.FunctionType(code, {})
                    previous = sum(actual is code for actual, _ in seen)
                    for enabled in (False, True, False, True):
                        monitor.set_local_events(tool, code, event if enabled else 0)
                        self.assertIsNone(function())
                        expected = ("INSTRUMENTED_" if enabled else "") + "RETURN_VALUE"
                        if not general:
                            expected += "_EMPTY"
                        self.assertEqual(_testinternalcapi.get_ir(code, True)[-1][0], expected)
                    monitor.set_local_events(tool, code, 0)
                    self.assertEqual(sum(actual is code for actual, _ in seen), previous + 2)
        finally:
            monitor.register_callback(tool, event, None)
            monitor.free_tool_id(tool)

    def test_checked_source_contract_covers_general_empty_and_instrumented_forms(self):
        from test import test_tools
        test_tools.skip_if_missing("cases_generator")
        with test_tools.imports_under_tool("cases_generator"):
            import analyzer
            import ir_generator
            import ir_metadata_generator

        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        if not source.is_file():
            self.skipTest("requires bytecodes.c")
        analysis = analyzer.analyze_files([str(source)])
        formats = ir_generator.compute_formats(analysis)
        contracts = ir_generator.handler_contracts(analysis, formats)
        for name in ("RETURN_VALUE", "INSTRUMENTED_RETURN_VALUE",
                     "RETURN_VALUE_EMPTY", "INSTRUMENTED_RETURN_VALUE_EMPTY"):
            with self.subTest(handler=name):
                self.assertEqual(contracts[name].transfer_kind, "_PyIR_TRANSFER_EXIT")
                self.assertTrue(ir_generator.canonical_frame_transfer_input(
                    ir_generator.handler_instructions(analysis)[name]))
        self.assertEqual(ir_metadata_generator.shadow_of("RETURN_VALUE_EMPTY", analysis),
                         "RETURN_VALUE")
        self.assertEqual(ir_metadata_generator.shadow_of("INSTRUMENTED_RETURN_VALUE_EMPTY", analysis),
                         "INSTRUMENTED_RETURN_VALUE")


if __name__ == "__main__":
    unittest.main()
