"""Successful folded unpack transport retains no scratch outputs."""

import gc
from pathlib import Path
import tempfile
import types
import unittest

import _testinternalcapi
from test.test_ir_call_roles import routes


class UnpackTransferTests(unittest.TestCase):
    def test_generic_and_specialized_local_transport_with_callbacks(self):
        observations = []

        class Old:
            def __del__(self):
                gc.collect()
                observations.append(None)

        def unpack(source):
            a, b = Old(), Old()
            a, b = source
            return a, b

        self.assertTrue(any(row[0] == "UNPACK_SEQUENCE" and
                            row[4] < unpack.__code__.co_nlocals for row in
                            _testinternalcapi.get_ir(unpack.__code__)))
        a, b = [], {}
        for admitted in routes(unpack.__code__):
            function = types.FunctionType(admitted, globals(),
                                          closure=unpack.__closure__)
            for factory in (tuple, list, iter):
                for _ in range(20):
                    result = function(factory((a, b)))
                    self.assertIs(result[0], a)
                    self.assertIs(result[1], b)
        self.assertEqual(len(observations), 4 * 3 * 20 * 2)

    def test_drain_recipe_requires_each_actual_family_adapter(self):
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
        names = [name for name, fmt in formats.items()
                 if fmt.group == "UNPACK_SEQUENCE"]
        expected = (ir_generator.OwnershipEvent("_PyIR_OWNER_UNPACK_LOCAL_DRAIN"),)
        self.assertGreaterEqual(len(names), 3)
        for name in names:
            self.assertEqual(contracts[name].ownership_events, expected)
        text = source.read_text()
        marker = "STAT_INC(UNPACK_SEQUENCE, hit);"
        self.assertIn(marker, text)
        with tempfile.TemporaryDirectory() as directory:
            changed = Path(directory) / "bytecodes.c"
            changed.write_text(text.replace(marker, "(void)frame;\n" + marker, 1))
            analysis = analyzer.analyze_files([str(changed)])
            formats = ir_generator.compute_formats(analysis)
            contracts = ir_generator.handler_contracts(analysis, formats)
        for name in names:
            self.assertNotEqual(contracts[name].ownership_events, expected)


if __name__ == "__main__":
    unittest.main()
