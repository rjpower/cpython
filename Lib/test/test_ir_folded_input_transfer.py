"""A field-mapped scalar input does not write its former window home."""

from pathlib import Path
import tempfile
import types
import unittest

import _testinternalcapi
from test.test_ir_call_roles import routes


class FoldedInputTransferTests(unittest.TestCase):
    def test_comprehension_local_input_survives_loop_joins(self):
        def lists(fn, values):
            return fn([value for value in values])

        def sets(fn, values):
            return fn({value for value in values})

        def dictionaries(fn, values):
            return fn({key: value for key, value in values})

        for source, operation, values, expected in (
                (lists, "LIST_APPEND", (1, 2, 1), [1, 2, 1]),
                (sets, "SET_ADD", (1, 2, 1), {1, 2}),
                (dictionaries, "MAP_ADD", ((1, 2), (3, 4)), {1: 2, 3: 4})):
            rows = _testinternalcapi.get_ir(source.__code__)
            updates = [row for row in rows if row[0] == operation]
            self.assertEqual(len(updates), 1)
            self.assertLess(updates[0][2], source.__code__.co_nlocals)
            if operation == "MAP_ADD":
                self.assertLess(updates[0][3], source.__code__.co_nlocals)
            for admitted in routes(source.__code__):
                function = types.FunctionType(admitted, globals())
                for incoming in ((), values):
                    result = function(lambda value: value, incoming)
                    self.assertEqual(result, expected if incoming else type(expected)())

    def test_scalar_tail_mapping_requires_actual_scalar_io_and_flush(self):
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
        covered = set()
        for name, fmt in formats.items():
            if not fmt.window or not fmt.n_inputs:
                continue
            events = contracts[name].ownership_events
            if len(events) != 1 or events[0].kind not in (
                    "_PyIR_OWNER_NORMAL_CLEAR_INPUTS",
                    "_PyIR_OWNER_UNPACK_LOCAL_DRAIN"):
                continue
            inputs, _ = ir_generator.external_io(analysis.instructions[name])
            self.assertTrue(all(not item.is_array()
                                for item in inputs[-fmt.n_inputs:]))
            layout = ir_generator.build_record(
                name, analysis.instructions[name], fmt, analysis, formats).layout
            self.assertEqual(layout.in_fields,
                             {k - fmt.n_inputs: fmt.input_field(k)
                              for k in range(fmt.n_inputs)})
            covered.add(name)
        self.assertTrue({"LIST_APPEND", "SET_ADD", "MAP_ADD"} <= covered)

        text = source.read_text()
        marker = "inst(LIST_APPEND, (list, unused[oparg-1], v -- list, unused[oparg-1]))"
        self.assertIn(marker, text)
        with tempfile.TemporaryDirectory() as directory:
            changed = Path(directory) / "bytecodes.c"
            changed.write_text(text.replace(marker,
                marker.replace(", v --", ", v[1] --"), 1))
            changed_analysis = analyzer.analyze_files([str(changed)])
            with self.assertRaisesRegex(SyntaxError, "trailing scalar inputs"):
                ir_generator.compute_formats(changed_analysis)
            changed.write_text(text.replace(marker + " {",
                                            marker + " {\n(void)frame;", 1))
            changed_analysis = analyzer.analyze_files([str(changed)])
            changed_formats = ir_generator.compute_formats(changed_analysis)
            changed_contracts = ir_generator.handler_contracts(
                changed_analysis, changed_formats)
            self.assertNotEqual(changed_contracts["LIST_APPEND"].ownership_events,
                                contracts["LIST_APPEND"].ownership_events)


if __name__ == "__main__":
    unittest.main()
