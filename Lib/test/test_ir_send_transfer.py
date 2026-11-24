"""Normal SEND and throw completion retire their input windows."""

from pathlib import Path
import tempfile
import types
import unittest

import _testinternalcapi
from test.test_ir_call_roles import routes


class SendTransferTests(unittest.TestCase):
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

        # The words name the family's variants, such as SEND_XFER_POP_TOP
        names = {row[0] for row in _testinternalcapi.get_ir(outer.__code__)}
        for family in ("SEND", "CLEANUP_THROW"):
            self.assertTrue(any(name.startswith(family) for name in names), family)
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
        boundaries = ir_generator.generator_boundaries(analysis)
        names = [name for name, fmt in formats.items()
                 if fmt.group == "SEND" and name not in boundaries]
        expected = ir_generator.OwnershipEvent(
            "_PyIR_OWNER_NORMAL_COMPLETION", flags=3, src=3, dst=1, extent=2)
        self.assertGreaterEqual(len(names), 4)
        for name in names:
            self.assertEqual(contracts[name].ownership_events, (expected,))
        for name, boundary in boundaries.items():
            completed = ir_generator.OwnershipEvent(
                "_PyIR_OWNER_NATIVE_WINDOW_OUT", flags=1, src=4 if
                boundary.base == "CLEANUP_THROW" else 3, dst=1)
            if boundary.yielded == "DELEGATED":
                self.assertEqual(contracts[name].ownership_events, (
                    ir_generator.OwnershipEvent(
                        "_PyIR_OWNER_NATIVE_WINDOW_OUT", flags=1, src=3, dst=0, extent=2),
                    completed))
            elif boundary.base == "CLEANUP_THROW":
                self.assertEqual(contracts[name].ownership_events, (completed,))
        self.assertEqual(contracts["CLEANUP_THROW"].ownership_events, (
            ir_generator.OwnershipEvent(
                "_PyIR_OWNER_NORMAL_COMPLETION", flags=1, src=4, dst=1),))
        text = source.read_text()
        marker = "IR_COMPLETE_STEP(_PY_IR_SEND, completed, oparg);"
        self.assertIn(marker, text)
        with tempfile.TemporaryDirectory() as directory:
            changed = Path(directory) / "bytecodes.c"
            changed.write_text(text.replace(marker, marker.replace("oparg", "oparg + 1"), 1))
            analysis = analyzer.analyze_files([str(changed)])
            formats = ir_generator.compute_formats(analysis)
            with self.assertRaisesRegex(SyntaxError, "IR_COMPLETE_STEP takes 3 variables"):
                ir_generator.handler_contracts(analysis, formats)


if __name__ == "__main__":
    unittest.main()
