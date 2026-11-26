"""The lowering selects the empty return where the stack holds only the result."""

import marshal
import sys
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
                        # The selected arm, which the observing evaluator
                        # runs the observer for while the event is enabled
                        self.assertEqual(returns(code, True), plain)
                        self.assertEqual(
                            _testinternalcapi.code_is_observed(code), enabled)
                    monitor.set_local_events(tool, code, 0)
                    self.assertEqual(sum(actual is code for actual, _ in seen), previous + 2)
        finally:
            monitor.register_callback(tool, event, None)
            monitor.free_tool_id(tool)


if __name__ == "__main__":
    unittest.main()
