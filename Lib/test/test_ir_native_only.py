"""Native-only handlers are marked by the checked source contract."""

from test.support import ir_dis as opcode
from pathlib import Path
import unittest
from unittest import mock

import _opcode


class NativeOnlyTests(unittest.TestCase):
    def test_native_only_effect_comes_from_checked_source_contract(self):
        from test import test_tools
        test_tools.skip_if_missing("cases_generator")
        with test_tools.imports_under_tool("cases_generator"):
            import analyzer
            import ir_generator

        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        analysis = analyzer.analyze_files([str(source)])
        inst = analysis.instructions["INTERPRETER_EXIT"]
        formats = ir_generator.compute_formats(analysis)

        def effects():
            return ir_generator.handler_effects(ir_generator.build_record(
                inst.name, inst, formats[inst.name], analysis, formats))

        self.assertTrue(ir_generator.native_entry_input_contract(inst))
        self.assertIn("_PyIR_NATIVE_ONLY", effects())
        with mock.patch.object(ir_generator, "native_entry_input_contract", return_value=False):
            self.assertNotIn("_PyIR_NATIVE_ONLY", effects())
        ids = {**opcode.opmap, **opcode._ir_opmap}
        self.assertTrue(_opcode.get_ir_contract()[ids[inst.name]].effects & _opcode.IR_NATIVE_ONLY)


if __name__ == "__main__":
    unittest.main()
