"""Synthetic native homes cannot become an ordinary code image."""

import marshal
import opcode
from pathlib import Path
import unittest
from unittest import mock

import _opcode

from test.test_ir_admission import construct
from test.test_ir_transfer_admission import encoded, make_code


class NativeOnlyTests(unittest.TestCase):
    def test_all_public_image_routes_reject_native_activation_handler(self):
        code = make_code([("LOAD_CONST", 0), ("RETURN_VALUE", 0)])
        native = encoded([("LOAD_CONST", 0), ("INTERPRETER_EXIT", 0)])
        self.assertEqual(len(native), len(code.co_code))
        for route in (construct, lambda co, **kwargs: co.replace(**kwargs)):
            with self.subTest(route=route):
                with self.assertRaisesRegex(ValueError, "native-only executable handler"):
                    route(code, co_code=native)
        payload = marshal.dumps(code)
        self.assertEqual(payload.count(code.co_code), 1)
        with self.assertRaisesRegex(ValueError, "native-only executable handler"):
            marshal.loads(payload.replace(code.co_code, native, 1))

    def test_native_only_effect_comes_from_checked_source_contract(self):
        from test import test_tools
        test_tools.skip_if_missing("cases_generator")
        with test_tools.imports_under_tool("cases_generator"):
            import analyzer
            import ir_generator

        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        analysis = analyzer.analyze_files([str(source)])
        inst = analysis.instructions["INTERPRETER_EXIT"]
        fmt = ir_generator.compute_formats(analysis)[inst.name]
        self.assertTrue(ir_generator.native_entry_input_contract(inst))
        self.assertIn("_PyIR_NATIVE_ONLY",
            ir_generator.handler_effects(inst.name, inst, fmt, False))
        with mock.patch.object(ir_generator, "native_entry_input_contract", return_value=False):
            self.assertNotIn("_PyIR_NATIVE_ONLY",
                ir_generator.handler_effects(inst.name, inst, fmt, False))
        ids = {**opcode.opmap, **opcode._ir_opmap}
        self.assertTrue(_opcode.get_ir_contract()[ids[inst.name]].effects & _opcode.IR_NATIVE_ONLY)


if __name__ == "__main__":
    unittest.main()
