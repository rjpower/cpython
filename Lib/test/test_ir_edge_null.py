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

        # The keeping jumps of "x and y" are BranchTruth words now; the
        # generator's rule for a nullable result is held to a sample instruction
        with tempfile.TemporaryDirectory() as directory:
            changed = Path(directory) / "bytecodes.c"
            prefix = "// BEGIN BYTECODES //\n"
            suffix = "\n// END BYTECODES //\n"
            for arms in (("PyValue_NULL", "PyValue_NULL"),
                         ("PyValue_NULL", "PyValue_None"),
                         ("PyValue_None", "PyValue_NULL")):
                changed.write_text(prefix + """
inst(KEEP_OR_POP, (cond: consumed -- res: owned nullable))
    effects(releases) exits(next)
{
    PyValue held = TAKE(cond);
    if (oparg) { res = FIRST; } else { res = SECOND; }
    PyValue_CLOSE(held);
}
""".replace("FIRST", arms[0]).replace("SECOND", arms[1]) + suffix)
                sample = analyzer.analyze_files([str(changed)])
                row = ir_generator.handler_contracts(sample, ir_generator.compute_formats(sample))["KEEP_OR_POP"]
                self.assertEqual(bool(row.ownership_events[0].flags & 4), arms[0] == arms[1] == "PyValue_NULL")


if __name__ == "__main__":
    unittest.main()
