"""The lowering selects the empty return where the stack holds only the result."""

import marshal
import sys
from pathlib import Path
import types
import unittest

import _testinternalcapi


def returns(code, adaptive=False):
    return [row[0] for row in _testinternalcapi.get_ir(code, adaptive)
            if row[0].startswith("Return.")]


def constant():
    return 42


class Manager:
    def __enter__(self):
        return self

    def __exit__(self, *args):
        return False


def with_return(manager):
    with manager:
        return 5


def routes(function):
    code = function.__code__
    return (code, code.replace(), marshal.loads(marshal.dumps(code)))


class EmptyReturnTests(unittest.TestCase):
    def test_compiler_selection_survives_every_route(self):
        for code in routes(constant):
            self.assertEqual(returns(code), ["Return.Empty"])
            self.assertEqual(types.FunctionType(code, {})(), 42)

    def test_monitoring_keeps_the_selected_return_form(self):
        monitor = sys.monitoring
        tool = 2
        event = monitor.events.PY_RETURN
        seen = []
        monitor.use_tool_id(tool, "general and empty return forms")
        monitor.register_callback(tool, event,
                                  lambda code, offset, value: seen.append((code, value)))
        try:
            for function, argument in ((constant, ()), (with_return, (Manager(),))):
                for code in routes(function):
                    function = types.FunctionType(code, {})
                    function(*argument)
                    plain = returns(code)
                    previous = sum(actual is code for actual, _ in seen)
                    for enabled in (False, True, False, True):
                        monitor.set_local_events(tool, code, event if enabled else 0)
                        function(*argument)
                        # The observed wrapper of the selected arm
                        expected = [name + (".Observed" if enabled else "")
                                    for name in plain]
                        self.assertEqual(returns(code, True), expected)
                    monitor.set_local_events(tool, code, 0)
                    self.assertEqual(sum(actual is code for actual, _ in seen), previous + 2)
        finally:
            monitor.register_callback(tool, event, None)
            monitor.free_tool_id(tool)

    def test_checked_source_contract_covers_general_empty_and_instrumented_forms(self):
        from test import test_tools
        test_tools.skip_if_missing("cases_generator")
        with test_tools.imports_under_tool("cases_generator"):
            import analyzer
            import ir_generator
            import ir_metadata_generator

        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        if not source.is_file():
            self.skipTest("requires bytecodes.c")
        analysis = analyzer.analyze_files([str(source)])
        formats = ir_generator.compute_formats(analysis)
        contracts = ir_generator.handler_contracts(analysis, formats)
        for name in ("RETURN_VALUE", "INSTRUMENTED_RETURN_VALUE",
                     "RETURN_VALUE_EMPTY", "INSTRUMENTED_RETURN_VALUE_EMPTY"):
            with self.subTest(handler=name):
                self.assertEqual(contracts[name].transfer_kind, "_PyIR_TRANSFER_EXIT")
                self.assertTrue(ir_generator.canonical_frame_transfer_input(
                    ir_generator.handler_instructions(analysis)[name]))
        self.assertEqual(ir_metadata_generator.shadow_of("RETURN_VALUE_EMPTY", analysis),
                         "RETURN_VALUE")
        self.assertEqual(ir_metadata_generator.shadow_of("INSTRUMENTED_RETURN_VALUE_EMPTY", analysis),
                         "INSTRUMENTED_RETURN_VALUE")


if __name__ == "__main__":
    unittest.main()
