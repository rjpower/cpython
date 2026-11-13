"""Normal SEND and throw completion retire their input windows."""

import marshal
from pathlib import Path
import tempfile
import types
import unittest

import _testinternalcapi
from test.test_ir_admission import construct
from test.test_ir_call_roles import routes
from test.test_ir_transfer_admission import encoded, make_code


class SendTransferTests(unittest.TestCase):
    def assert_rejected(self, code, words, message):
        replacement = encoded(words)
        for route in (construct, lambda co, **kw: co.replace(**kw)):
            with self.assertRaisesRegex(ValueError, message):
                route(code, co_code=replacement)
        payload = marshal.dumps(code)
        self.assertEqual(payload.count(code.co_code), 1)
        with self.assertRaisesRegex(ValueError, message):
            marshal.loads(payload.replace(code.co_code, replacement, 1))

    def test_send_completion_cursor_and_result_are_distinct(self):
        words = [("LOAD_CONST", 0), ("LOAD_CONST", 2),
                 ("SEND", 0, 0, 0, 4), ("POP_TOP", 2), ("POP_TOP", 1),
                 ("CALL_NO_SELF", 0, 0, 0, 0), ("RETURN_VALUE", 0),
                 ("CALL_NO_SELF", 0, 0, 0, 0), ("RETURN_VALUE", 0)]
        code = make_code(words, slots=6)
        for admitted in routes(code):
            self.assertEqual(_testinternalcapi.get_ir(admitted, True),
                             _testinternalcapi.get_ir(code))
        bad = words.copy()
        bad[4] = ("IR_JUMP", 0, 0, 5)
        self.assert_rejected(code, bad, "virtual call input")
        bad = words.copy()
        bad[2] = ("SEND", 1, 0, 0, 4)
        self.assert_rejected(code, bad, "virtual call input")

    def test_throw_completion_preserves_only_prior_empty_outside_tail(self):
        words = [("LOAD_CONST", 0), ("LOAD_CONST", 2), ("LOAD_CONST", 3),
                 ("POP_TOP", 4), ("CLEANUP_THROW", 0, 0, 5, 0),
                 ("CALL_NO_SELF", 0, 0, 0, 0), ("RETURN_VALUE", 0)]
        code = make_code(words, slots=6)
        for admitted in routes(code):
            self.assertEqual(_testinternalcapi.get_ir(admitted, True),
                             _testinternalcapi.get_ir(code))
        bad = words.copy()
        bad[3] = ("LOAD_CONST", 4)
        self.assert_rejected(code, bad, "child clobber home")
        bad = words.copy()
        bad[4] = ("CLEANUP_THROW", 1, 0, 5, 0)
        self.assert_rejected(code, bad, "virtual call input")

    def test_delegation_and_recovered_throw_reenter_the_outer_loop(self):
        def outer(fn, groups):
            for group in groups:
                yield from group
                fn(17)

        class Recover:
            def __iter__(self):
                return self

            def __next__(self):
                return 23

            def throw(self, error):
                raise StopIteration("recovered")

        names = {row[0] for row in _testinternalcapi.get_ir(outer.__code__)}
        self.assertTrue({"SEND", "CLEANUP_THROW"} <= names)
        for admitted in routes(outer.__code__):
            function = types.FunctionType(admitted, globals())
            seen = []
            groups = ((1, 2), (value for value in (3, 4)))
            self.assertEqual(list(function(seen.append, groups)), [1, 2, 3, 4])
            self.assertEqual(seen, [17, 17])
            seen.clear()
            generator = function(seen.append, (Recover(), (42,)))
            self.assertEqual(next(generator), 23)
            self.assertEqual(generator.throw(ValueError()), 42)
            self.assertEqual(list(generator), [])
            self.assertEqual(seen, [17, 17])

    def test_completion_recipe_requires_actual_matching_helper_target(self):
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
        contracts = ir_generator.handler_contracts(analysis, formats)
        names = [name for name, fmt in formats.items() if fmt.group == "SEND"]
        expected = ir_generator.OwnershipEvent(
            "_PyIR_OWNER_NORMAL_COMPLETION", flags=3, src=3, dst=1, extent=2)
        self.assertGreaterEqual(len(names), 4)
        for name in names:
            self.assertEqual(contracts[name].ownership_events, (expected,))
        self.assertEqual(contracts["CLEANUP_THROW"].ownership_events, (
            ir_generator.OwnershipEvent(
                "_PyIR_OWNER_NORMAL_COMPLETION", flags=1, src=4, dst=1),))
        text = source.read_text()
        marker = "IR_COMPLETE_STEP(_PY_IR_SEND, completed, oparg);"
        self.assertIn(marker, text)
        with tempfile.TemporaryDirectory() as directory:
            changed = Path(directory) / "bytecodes.c"
            changed_route = "int changed_target = oparg + 1;\n" + marker.replace("oparg", "changed_target")
            changed.write_text(text.replace(marker, changed_route, 1))
            analysis = analyzer.analyze_files([str(changed)])
            formats = ir_generator.compute_formats(analysis)
            contracts = ir_generator.handler_contracts(analysis, formats)
        for name in names:
            self.assertNotIn(expected, contracts[name].ownership_events)


if __name__ == "__main__":
    unittest.main()
