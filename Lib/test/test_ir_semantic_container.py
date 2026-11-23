"""Optional positions cannot supply or erase semantic recovery authority."""

from test.support import ir_dis as dis
import marshal
import sys
import types
import unittest

import _opcode
import _testinternalcapi


def routes(code):
    "The same code reached by the public routes that rebuild a code object"
    return (code, code.replace(), marshal.loads(marshal.dumps(code)))


class SemanticContainerTests(unittest.TestCase):
    def test_line_redirect_failure_uses_the_target_recovery(self):
        class Marker(Exception):
            pass

        def target(make_value, cleanup, never):
            start = destination = 0
            try:
                start = (
                    make_value()
                    + never()
                )
            except Marker:
                return "source"
            try:
                destination = 2
            except Marker:
                cleanup()
                return "target"
            return "ordinary"

        original = target.__code__
        code = original
        visited, released, completed = [], [], []

        class Owner:
            def __del__(self):
                released.append("scratch")

        def never():
            self.fail("redirect did not cancel the expression")

        function = types.FunctionType(code, globals(), closure=target.__closure__)
        monitor = sys.monitoring
        tool = monitor.DEBUGGER_ID

        def line(observed, lineno):
            if observed is code and lineno == code.co_firstlineno + 5 and not visited:
                frame = sys._getframe(1)
                self.assertIs(frame.f_code, code)
                frame.f_locals["destination"] = 9
                frame.f_lineno = code.co_firstlineno + 10
                self.assertEqual(released, ["scratch"])
                visited.append(True)
                raise Marker("redirected failure")

        monitor.use_tool_id(tool, "redirected-observer")
        try:
            monitor.register_callback(tool, monitor.events.LINE, line)
            monitor.set_local_events(tool, code, monitor.events.LINE)
            self.assertEqual(function(Owner, lambda: completed.append("target"), never),
                             "target")
        finally:
            monitor.set_local_events(tool, code, 0)
            monitor.register_callback(tool, monitor.events.LINE, None)
            monitor.free_tool_id(tool)
        self.assertEqual(visited, [True])
        self.assertEqual(released, ["scratch"])
        self.assertEqual(completed, ["target"])

    def test_inert_protected_body_keeps_observer_error_ingress(self):
        def target(value, cleanup):
            try:
                return value
            finally:
                cleanup()

        original = target.__code__
        for code in (original, original.replace(co_linetable=b"")):
            with self.subTest(positions=code is original):
                # The IR exists once lowered; its words and the instruction
                # events' byte offsets differ, so map the first protected
                # word to the offset of the unit it was lowered from
                _testinternalcapi.get_ir(code)
                word = _opcode.get_exception_entries(code)[0][0]
                protected = 2 * dis.IRCode(code)._pcmap[word]
                completed = []
                function = types.FunctionType(code, globals())
                monitor = sys.monitoring
                tool = monitor.DEBUGGER_ID

                def instruction(observed, offset):
                    if observed is code and offset == protected:
                        raise ValueError("observed inert word")

                monitor.use_tool_id(tool, "inert-protected")
                try:
                    monitor.register_callback(tool, monitor.events.INSTRUCTION, instruction)
                    monitor.set_local_events(tool, code, monitor.events.INSTRUCTION)
                    with self.assertRaisesRegex(ValueError, "observed inert word"):
                        function(object(), lambda: completed.append("finally"))
                finally:
                    monitor.set_local_events(tool, code, 0)
                    monitor.register_callback(tool, monitor.events.INSTRUCTION, None)
                    monitor.free_tool_id(tool)
                self.assertEqual(completed, ["finally"])

    def template(self):
        def target(flag):
            try:
                if flag:
                    pass
                raise ValueError("expected")
            except ValueError:
                return 7
        code = target.__code__
        _testinternalcapi.get_ir(code)
        self.assertTrue(_opcode.get_exception_entries(code))
        self.assertTrue(_opcode.get_recovery_entries(code))
        return code

    def test_stripping_positions_preserves_semantics_on_all_routes(self):
        code = self.template()
        stripped = code.replace(co_linetable=b"")
        self.assertEqual(stripped.co_exceptiontable, code.co_exceptiontable)
        self.assertTrue(all(position == (None,) * 4
                            for position in stripped.co_positions()))
        for route in routes(stripped):
            _testinternalcapi.get_ir(route)
            self.assertEqual(_opcode.get_exception_entries(route),
                             _opcode.get_exception_entries(code))
            target = types.FunctionType(route, globals())
            self.assertEqual(target(False), 7)
            self.assertEqual(target(True), 7)

    def test_rename_keeps_exception_table(self):
        code = self.template()
        renamed = code.replace(co_varnames=tuple("renamed_" + name
                                                for name in code.co_varnames))
        self.assertEqual(renamed.co_exceptiontable, code.co_exceptiontable)
        self.assertEqual(types.FunctionType(renamed, globals())(True), 7)


if __name__ == "__main__":
    unittest.main()
