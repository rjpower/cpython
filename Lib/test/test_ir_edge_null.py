"""Normal-edge NULL publication comes from the emitted final store."""

from pathlib import Path
import tempfile
import types
import unittest

import _opcode
import _testinternalcapi
import opcode

from test.test_ir_call_roles import routes
from test.test_ir_transfer_admission import TransferAdmissionTests, make_code


class EdgeNullTests(unittest.TestCase):
    assert_rejected = TransferAdmissionTests.assert_rejected

    def image(self, name, *, source=1):
        return [("LOAD_CONST", 0), ("LOAD_CONST", source),
                (name, 1, source, 5), ("IR_JUMP", 0, 0, 6),
                ("IR_JUMP", 0, 0, 6), ("POP_TOP", 1),
                ("CALL_NO_SELF", 0, 0, 0, 0), ("RETURN_VALUE", 0)]

    def test_only_the_null_successor_releases_the_result_home(self):
        for name in ("JUMP_IF_FALSE_OR_POP", "JUMP_IF_TRUE_OR_POP"):
            for source in (1, 2):
                with self.subTest(handler=name, source=source):
                    words = self.image(name, source=source)
                    code = make_code(words)
                    for admitted in routes(code):
                        _testinternalcapi.get_ir(admitted, True)
                    bad = words.copy()
                    bad[5] = ("IR_JUMP", 0, 0, 6)
                    self.assert_rejected(words, bad, "virtual call input")
                    if source == 2:
                        # A NULL publication cannot erase an unrelated prior
                        # owner merely because another input was consumed.
                        prior = [("POP_TOP", 1), ("LOAD_CONST", 0), ("LOAD_CONST", 2),
                                 (name, 1, 2, 6), ("IR_JUMP", 0, 0, 7),
                                 ("IR_JUMP", 0, 0, 7), ("POP_TOP", 1),
                                 ("CALL_NO_SELF", 0, 0, 0, 0), ("RETURN_VALUE", 0)]
                        bad = prior.copy()
                        bad[0] = ("LOAD_CONST", 1)
                        self.assert_rejected(prior, bad, "virtual call input|scratch owner")

    def test_actual_language_paths(self):
        def left(function, value):
            return function(value and 23)

        def right(function, value):
            return function(value or 23)

        for template, expected in ((left, (0, 23)), (right, (23, 1))):
            names = {row[0] for row in _testinternalcapi.get_ir(template.__code__)}
            self.assertTrue(names & {"JUMP_IF_FALSE_OR_POP", "JUMP_IF_TRUE_OR_POP"})
            for code in routes(template.__code__):
                function = types.FunctionType(code, globals())
                for value, result in zip((0, 1), expected):
                    self.assertEqual(function(lambda arg: arg, value), result)

    def test_null_receipts_require_actual_assignments_joins_and_final_stores(self):
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
        names = ("JUMP_IF_FALSE_OR_POP", "JUMP_IF_TRUE_OR_POP",
                 "INSTRUMENTED_JUMP_IF_FALSE_OR_POP", "INSTRUMENTED_JUMP_IF_TRUE_OR_POP")
        for name in names:
            with self.subTest(handler=name):
                events = contracts[name].ownership_events
                self.assertEqual(len(events), 1)
                self.assertEqual(events[0].kind, "_PyIR_OWNER_NORMAL_CLEAR_INPUTS")
                self.assertEqual(events[0].flags & 12, 4)

        text = source.read_text()
        begin = text.index("inst(JUMP_IF_FALSE_OR_POP,")
        end = text.index("inst(JUMP_IF_TRUE_OR_POP,", begin)
        body = text[begin:end]
        with tempfile.TemporaryDirectory() as directory:
            changed = Path(directory) / "bytecodes.c"
            # The observed twin cannot certify a changed ordinary store.
            changed.write_text(text[:begin] + body.replace("res = PyValue_NULL;", "res = PyValue_None;") + text[end:])
            altered = analyzer.analyze_files([str(changed)])
            rows = ir_generator.handler_contracts(altered, ir_generator.compute_formats(altered))
            for name in (names[0], names[2]):
                self.assertFalse(rows[name].ownership_events[0].flags & 4)

            prefix = "// BEGIN BYTECODES //\n"
            suffix = "\n// END BYTECODES //\n"
            for arms in (("PyValue_NULL", "PyValue_NULL"),
                         ("PyValue_NULL", "PyValue_None"),
                         ("PyValue_None", "PyValue_NULL")):
                changed.write_text(prefix + """
inst(JUMP_IF_FALSE_OR_POP, (cond -- res)) {
    PyValue held = TAKE(cond);
    if (oparg) { res = FIRST; } else { res = SECOND; }
    PyValue_CLOSE(held);
}
""".replace("FIRST", arms[0]).replace("SECOND", arms[1]) + suffix)
                sample = analyzer.analyze_files([str(changed)])
                row = ir_generator.handler_contracts(sample, ir_generator.compute_formats(sample))["JUMP_IF_FALSE_OR_POP"]
                self.assertEqual(bool(row.ownership_events[0].flags & 4), arms[0] == arms[1] == "PyValue_NULL")

            observed_begin = text.index("inst(INSTRUMENTED_JUMP_IF_FALSE_OR_POP,")
            observed_end = text.index("inst(INSTRUMENTED_JUMP_IF_TRUE_OR_POP,", observed_begin)
            observed = text[observed_begin:observed_end]
            changed.write_text(text[:observed_begin] + observed.replace(
                "res = PyValue_NULL;", "res = PyValue_None;") + text[observed_end:])
            altered = analyzer.analyze_files([str(changed)])
            rows = ir_generator.handler_contracts(altered, ir_generator.compute_formats(altered))
            for name in (names[0], names[2]):
                self.assertFalse(rows[name].ownership_events[0].flags & 4)

            for mutation in (
                "PyValue *alias = &res;",
                "PyValue *alias = &(res);",
                "PyValue res = PyValue_NULL;",
            ):
                changed.write_text(text[:begin] + body.replace("PyValue held", mutation + " PyValue held", 1) + text[end:])
                altered = analyzer.analyze_files([str(changed)])
                fmt = ir_generator.compute_formats(altered)[names[0]]
                self.assertIsNone(ir_generator.null_output_names(altered.instructions[names[0]], fmt))


if __name__ == "__main__":
    unittest.main()
