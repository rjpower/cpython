"""Normal-edge NULL publication comes from the emitted final store."""

import marshal
from pathlib import Path
import tempfile
import types
import unittest

import _testinternalcapi

from test.test_ir_admission import construct


def routes(code):
    return (code, construct(code), code.replace(),
            marshal.loads(marshal.dumps(code)))


class EdgeNullTests(unittest.TestCase):
    def test_actual_language_paths(self):
        def left(function, value):
            return function(value and 23)

        def right(function, value):
            return function(value or 23)

        for template, expected in ((left, (0, 23)), (right, (23, 1))):
            # A branch that keeps its operand where it jumps is BranchTruth
            # in a keep mode (BRANCH_KEEP_IF_FALSE, BRANCH_KEEP_IF_TRUE)
            rows = _testinternalcapi.get_ir(template.__code__)
            self.assertTrue([row for row in rows
                             if row[0] == "BranchTruth.Generic" and row[3] in (2, 3)])
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
            # The ordinary twin declares `res: moved(cond) nullable` too
            # (G0c9), so its declaration refuses a changed store
            changed.write_text(text[:begin] + body.replace("res = PyValue_NULL;", "res = PyValue_None;") + text[end:])
            with self.assertRaisesRegex(SyntaxError, r"`res` is declared moved\(cond\), "
                                        r"but is assigned `PyValue_None`"):
                altered = analyzer.analyze_files([str(changed)])
                ir_generator.handler_contracts(altered, ir_generator.compute_formats(altered))

            prefix = "// BEGIN BYTECODES //\n"
            suffix = "\n// END BYTECODES //\n"
            for arms in (("PyValue_NULL", "PyValue_NULL"),
                         ("PyValue_NULL", "PyValue_None"),
                         ("PyValue_None", "PyValue_NULL")):
                changed.write_text(prefix + """
inst(JUMP_IF_FALSE_OR_POP, (cond: consumed -- res: owned nullable))
    effects(releases) exits(next)
{
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
            # The observed twin declares `res: moved(cond) nullable`, so its
            # declaration refuses a changed store before any record is made
            changed.write_text(text[:observed_begin] + observed.replace(
                "res = PyValue_NULL;", "res = PyValue_None;") + text[observed_end:])
            with self.assertRaisesRegex(SyntaxError, r"`res` is declared moved\(cond\), "
                                        r"but is assigned `PyValue_None`"):
                altered = analyzer.analyze_files([str(changed)])
                ir_generator.handler_contracts(altered, ir_generator.compute_formats(altered))

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
