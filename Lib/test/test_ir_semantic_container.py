"""Optional positions cannot supply or erase semantic recovery authority."""

import dis
import marshal
import struct
import sys
import types
import unittest

import _opcode
import _testinternalcapi

from test.support.ir_semantics import position_parts, position_table, semantic_parts
from test.test_ir_admission import construct
from test.test_ir_call_roles import routes


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
        locations, _ = position_parts(original.co_linetable)
        for code in (original, original.replace(co_linetable=position_table(locations))):
            with self.subTest(edge_positions=code is original):
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
        protected = _opcode.get_exception_entries(original)[0][0]
        for code in (original, original.replace(co_linetable=b"")):
            with self.subTest(positions=code is original):
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
        self.assertTrue(_opcode.get_exception_entries(code))
        self.assertTrue(_opcode.get_recovery_entries(code))
        return code

    def test_stripping_positions_preserves_semantics_on_all_admission_routes(self):
        code = self.template()
        recovery = _opcode.get_recovery_entries(code)
        exceptions = dis.Bytecode(code).exception_entries
        normalized = code.replace(co_linetable=b"").co_linetable
        for supplied in (b"", normalized):
            stripped = code.replace(co_linetable=supplied)
            self.assertEqual(stripped.co_exceptiontable, code.co_exceptiontable)
            self.assertFalse(position_parts(stripped.co_linetable)[1])
            self.assertEqual(list(stripped.co_positions()),
                             [(None, None, None, None)] * (len(code.co_code) // 8))
            for admitted in routes(stripped):
                self.assertEqual(_opcode.get_recovery_entries(admitted), recovery)
                self.assertEqual(dis.Bytecode(admitted).exception_entries, exceptions)
                _testinternalcapi.get_ir(admitted, True)
                target = types.FunctionType(admitted, globals())
                self.assertEqual(target(False), 7)
                self.assertEqual(target(True), 7)

    def test_position_edits_keep_stable_semantic_row_ids(self):
        code = self.template()
        locations, positions = position_parts(code.co_linetable)
        self.assertEqual(len(positions), len(_opcode.get_recovery_entries(code)))
        moved = [(line + 100, end_line + 100, column, end_column)
                 if line > 0 and end_line > 0 else (line, end_line, column, end_column)
                 for line, end_line, column, end_column in positions]
        changed = code.replace(co_linetable=position_table(locations, moved))
        self.assertEqual(changed.co_exceptiontable, code.co_exceptiontable)
        self.assertEqual(_opcode.get_recovery_entries(changed),
                         _opcode.get_recovery_entries(code))
        self.assertEqual(types.FunctionType(changed, globals())(False), 7)
        with self.assertRaisesRegex(ValueError, "position IDs"):
            code.replace(co_linetable=position_table(locations, positions + positions[:1]))

    def test_malformed_recovery_is_rejected_even_without_positions(self):
        code = self.template()
        semantic = code.co_exceptiontable
        exceptions, recovery = semantic_parts(semantic)
        self.assertTrue(recovery)
        malformed = [b"IRSC", exceptions, semantic[:-1], semantic + b"\0",
                     b"IRSC" + struct.pack("<III", 1, 0, 0),
                     semantic[:4] + struct.pack("<I", 2) + semantic[8:]]
        changed = bytearray(semantic)
        struct.pack_into("<i", changed, 16 + len(exceptions) + 12,
                         len(code.co_code) // 8)
        malformed.append(bytes(changed))
        for positions in (code.co_linetable, b""):
            for bad in malformed:
                with self.subTest(positions=bool(positions), bad=bad[:16]):
                    for route in (construct, lambda co, **kw: co.replace(**kw)):
                        with self.assertRaisesRegex(ValueError, "semantic recovery"):
                            route(code, co_exceptiontable=bad, co_linetable=positions)
        payload = marshal.dumps(code)
        self.assertEqual(payload.count(semantic), 1)
        with self.assertRaisesRegex(ValueError, "semantic recovery"):
            marshal.loads(payload.replace(semantic, bytes(changed), 1))

    def test_old_position_envelopes_cannot_restore_semantic_rows(self):
        code = self.template()
        locations, _ = position_parts(code.co_linetable)
        old = b"IRSM" + struct.pack("<III", 1, len(locations), 0) + locations
        with self.assertRaisesRegex(ValueError, "source map"):
            code.replace(co_linetable=old)

    def test_rename_keeps_recovery_but_changed_binding_count_requires_explicit_data(self):
        code = self.template()
        renamed = code.replace(co_varnames=tuple("renamed_" + name
                                                for name in code.co_varnames))
        self.assertEqual(renamed.co_exceptiontable, code.co_exceptiontable)
        self.assertEqual(types.FunctionType(renamed, globals())(True), 7)
        with self.assertRaisesRegex(ValueError, "matching semantic recovery"):
            code.replace(co_nlocals=code.co_nlocals + 1,
                         co_varnames=code.co_varnames + ("extra",))


if __name__ == "__main__":
    unittest.main()
