"""Successful folded unpack transport retains no scratch outputs."""

import gc
import marshal
from pathlib import Path
import tempfile
import types
import unittest

import _testinternalcapi
from test.test_ir_call_roles import routes
from test.test_ir_transfer_admission import encoded, make_code
from test.test_ir_admission import construct


class UnpackTransferTests(unittest.TestCase):
    def test_nonfolded_unpack_preserves_only_untouched_tail_absence(self):
        for count in (0, 2, 8):
            with self.subTest(count=count):
                base = 1
                tail = base + max(count, 2)
                words = [("LOAD_CONST", base), ("POP_TOP", tail),
                         ("UNPACK_SEQUENCE", base, count, 0, 4095)]
                words.extend(("POP_TOP", base + i)
                             for i in range(max(count, 1)))
                words.extend((("LOAD_CONST", base),
                              ("CALL_NO_SELF", base, base, 0, 0),
                              ("RETURN_VALUE", base)))
                code = make_code(words, slots=count + 4, locals=("local",))
                for admitted in routes(code):
                    self.assertEqual(_testinternalcapi.get_ir(admitted, True),
                                     _testinternalcapi.get_ir(code))
                # Being outside the output footprint preserves an existing
                # absence fact; it cannot manufacture one for a live value.
                bad = words.copy()
                bad[1] = ("LOAD_CONST", tail)
                replacement = encoded(bad)
                for route in (construct, lambda co, **kw: co.replace(**kw)):
                    with self.assertRaisesRegex(ValueError, "child clobber home"):
                        route(code, co_code=replacement)
                payload = marshal.dumps(code)
                self.assertEqual(payload.count(code.co_code), 1)
                with self.assertRaisesRegex(ValueError, "child clobber home"):
                    marshal.loads(payload.replace(code.co_code, replacement, 1))

    def test_folded_outputs_are_empty_but_scratch_outputs_remain_live(self):
        for count in (0, 2, 8):
            with self.subTest(count=count):
                nl = max(count, 1)
                words = [("LOAD_CONST", nl),
                         ("UNPACK_SEQUENCE", nl, count, 0, 0),
                         ("LOAD_CONST", nl),
                         ("CALL_NO_SELF", nl, nl, 0, 0),
                         ("RETURN_VALUE", nl)]
                code = make_code(words, slots=count + 4,
                                 locals=tuple(f"local{i}" for i in range(nl)))
                for admitted in routes(code):
                    self.assertEqual(_testinternalcapi.get_ir(admitted, True),
                                     _testinternalcapi.get_ir(code))
                if count < 2:
                    continue
                bad = words.copy()
                bad[1] = ("UNPACK_SEQUENCE", nl, count, 0, 4095)
                replacement = encoded(bad)
                for route in (construct, lambda co, **kw: co.replace(**kw)):
                    with self.assertRaisesRegex(ValueError, "virtual call input"):
                        route(code, co_code=replacement)
                payload = marshal.dumps(code)
                self.assertEqual(payload.count(code.co_code), 1)
                with self.assertRaisesRegex(ValueError, "virtual call input"):
                    marshal.loads(payload.replace(code.co_code, replacement, 1))

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
        self.assertGreaterEqual(len(names), 4)
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
