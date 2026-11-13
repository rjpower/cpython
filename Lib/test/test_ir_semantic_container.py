"""Optional positions cannot supply or erase semantic recovery authority."""

import dis
import marshal
import struct
import types
import unittest

import _opcode
import _testinternalcapi

from test.support.ir_semantics import position_parts, position_table, semantic_parts
from test.test_ir_admission import construct
from test.test_ir_call_roles import routes


class SemanticContainerTests(unittest.TestCase):
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
