"""Delegated suspension carries an admitted explicit receiver role."""

import marshal
import sys
from pathlib import Path
import tempfile
import types
import unittest

import _testinternalcapi
from test.test_ir_admission import construct
from test.test_ir_call_roles import routes
from test.test_ir_transfer_admission import encoded, make_code


class DelegatedYieldTests(unittest.TestCase):
    def assert_admitted(self, words, *, slots=6, bindings=()):
        # These images exercise admission and cold plan construction only.
        code = make_code(words, slots=slots, locals=bindings)
        for admitted in routes(code):
            self.assertEqual(_testinternalcapi.get_ir(admitted, True),
                             _testinternalcapi.get_ir(code))
        return code

    def assert_rejected(self, code, words, message):
        replacement = encoded(words)
        for route in (construct, lambda co, **kw: co.replace(**kw)):
            with self.assertRaisesRegex(ValueError, message):
                route(code, co_code=replacement, co_linetable=b"")
        payload = marshal.dumps(code)
        self.assertEqual(payload.count(code.co_code), 1)
        self.assertEqual(len(replacement), len(code.co_code))
        with self.assertRaisesRegex(ValueError, message):
            marshal.loads(payload.replace(code.co_code, replacement, 1))

    def basic_image(self):
        return [("LOAD_CONST", 0), ("LOAD_CONST", 2),
                ("SEND", 0, 0, 0, 4), ("YIELD_DELEGATED", 2, 2, 0, 0),
                ("POP_TOP", 2), ("POP_TOP", 0), ("IR_JUMP", 0, 0, 8),
                ("POP_TOP", 0), ("LOAD_CONST", 0), ("RETURN_VALUE", 0)]

    def test_send_body_output_is_independent_of_completion_recipient(self):
        words = self.basic_image()
        code = self.assert_admitted(words)
        changed = words.copy()
        changed[2] = ("SEND", 1, 0, 0, 4)
        changed[7] = ("POP_TOP", 1)
        self.assert_admitted(changed)
        for field, value in ((1, 3), (2, 1), (3, 1), (4, 1)):
            bad = words.copy()
            row = list(bad[3])
            row[field] = value
            bad[3] = tuple(row)
            if field in (1, 3):
                # A separate empty recipient is valid; C is unused.
                self.assert_admitted(bad)
            else:
                self.assert_rejected(code, bad, "delegated")

    def test_swap_transports_receiver_without_a_contiguous_yield_window(self):
        words = [("LOAD_CONST", 0), ("LOAD_CONST", 2),
                 ("SEND", 0, 0, 0, 7), ("LOAD_CONST", 4), ("SWAP", 0, 5),
                 ("YIELD_DELEGATED", 2, 2, 0, 4), ("POP_TOP", 2),
                 ("POP_TOP", 4), ("POP_TOP", 0), ("IR_JUMP", 0, 0, 11),
                 ("POP_TOP", 0), ("LOAD_CONST", 0), ("RETURN_VALUE", 0)]
        code = self.assert_admitted(words)
        bad = words.copy()
        bad[5] = ("YIELD_DELEGATED", 2, 2, 0, 0)
        self.assert_rejected(code, bad, "delegated SEND role proof")

    def test_named_resume_recipient_and_full_scratch_extent(self):
        self.assert_admitted([
            ("LOAD_CONST", 1), ("LOAD_CONST", 3), ("SEND", 0, 1, 0, 4),
            ("YIELD_DELEGATED", 0, 3, 0, 1), ("POP_TOP", 3),
            ("POP_TOP", 1), ("IR_JUMP", 0, 0, 8), ("IR_JUMP", 0, 0, 8),
            ("LOAD_CONST", 1), ("RETURN_VALUE", 1)], bindings=("recipient",))
        self.assert_admitted([
            ("LOAD_CONST", 4090), ("LOAD_CONST", 4092),
            ("SEND", 4090, 4090, 0, 4),
            ("YIELD_DELEGATED", 4094, 4092, 0, 4090),
            ("POP_TOP", 4094), ("POP_TOP", 4090), ("IR_JUMP", 0, 0, 8),
            ("POP_TOP", 4090), ("LOAD_CONST", 4090),
            ("RETURN_VALUE", 4090)], slots=4095)

    def test_a_missing_unknown_or_unreachable_send_cannot_certify(self):
        words = self.basic_image()
        code = self.assert_admitted(words)
        for replacement in (("IR_JUMP", 0, 0, 3), ("UNARY_NOT", 2, 2)):
            bad = words.copy()
            bad[2] = replacement
            self.assert_rejected(code, bad, "delegated")
        bad = words.copy()
        bad[2] = ("IR_JUMP", 0, 0, 8)
        self.assert_rejected(code, bad, "entry proof")

    def test_join_requires_matching_roles_on_every_predecessor(self):
        words = [("LOAD_CONST", 0), ("LOAD_CONST", 2), ("LOAD_CONST", 4),
                 ("POP_JUMP_IF_NONE", 4, 0, 6),
                 ("SEND", 0, 0, 0, 6), ("IR_JUMP", 0, 0, 8),
                 ("SEND", 0, 0, 0, 4), ("IR_JUMP", 0, 0, 8),
                 ("YIELD_DELEGATED", 2, 2, 0, 0), ("POP_TOP", 2),
                 ("POP_TOP", 0), ("POP_TOP", 0),
                 ("LOAD_CONST", 0), ("RETURN_VALUE", 0)]
        code = self.assert_admitted(words)
        # Either arrival order must lose the proof if one arm did not SEND.
        for site in (4, 6):
            bad = words.copy()
            bad[site] = ("IR_JUMP", 0, 0, 8)
            self.assert_rejected(code, bad, "delegated SEND role proof")

    def test_ordinary_yield_cannot_claim_an_implicit_receiver(self):
        words = self.basic_image()
        code = self.assert_admitted(words)
        for name in ("YIELD_VALUE", "INSTRUMENTED_YIELD_VALUE"):
            words[3] = (name, 2, 2, 1)
            self.assert_rejected(code, words, "explicit delegated yield")

    def test_real_generator_and_coroutine_forms_use_explicit_receiver(self):
        def outer(receiver):
            return (yield from receiver)

        async def awaiting(receiver):
            return await receiver

        class Awaitable:
            def __await__(self):
                yield 17
                return 23

        for source, factory in ((outer, lambda: iter((17, 23))),
                                (awaiting, Awaitable)):
            rows = _testinternalcapi.get_ir(source.__code__)
            selected = [row for row in rows if row[0] == "YIELD_DELEGATED"]
            self.assertEqual(len(selected), 1)
            self.assertEqual(selected[0][3], 0)
            self.assertGreaterEqual(selected[0][4], source.__code__.co_nlocals)
            self.assertNotEqual(selected[0][4], selected[0][2])
            for admitted in routes(source.__code__):
                function = types.FunctionType(admitted, globals())
                generator = function(factory())
                iterator = generator.__await__() if source is awaiting else generator
                self.assertEqual(next(iterator), 17)
                if source is outer:
                    self.assertEqual(next(iterator), 23)
                with self.assertRaises(StopIteration) as ended:
                    next(iterator)
                self.assertEqual(ended.exception.value, 23 if source is awaiting else None)

    def test_monitoring_toggle_preserves_receiver_and_delegated_family(self):
        def outer(receiver):
            yield from receiver

        async def awaiting(receiver):
            await receiver

        class Awaitable:
            def __await__(self):
                yield from range(4)

        monitor = sys.monitoring
        tool = 2
        monitor.use_tool_id(tool, "delegated yield transport")
        event = monitor.events.PY_YIELD
        seen = []
        monitor.register_callback(tool, event,
                                  lambda code, offset, value: seen.append((code, value)))
        try:
            for source, receiver in ((outer, iter(range(4))), (awaiting, Awaitable())):
                code = source.__code__
                suspended = source(receiver)
                iterator = suspended.__await__() if source is awaiting else suspended
                iterator_receiver = None
                try:
                    for value, enabled in enumerate((False, True, False, True)):
                        monitor.set_local_events(tool, code, event if enabled else 0)
                        self.assertEqual(next(iterator), value)
                        actual = suspended.cr_await if source is awaiting else suspended.gi_yieldfrom
                        if source is awaiting and iterator_receiver is None:
                            iterator_receiver = actual
                            self.assertIsInstance(actual, types.GeneratorType)
                        self.assertIs(actual, receiver if source is outer else iterator_receiver)
                        rows = _testinternalcapi.get_ir(code, True)
                        names = {row[0] for row in rows}
                        self.assertIn("INSTRUMENTED_YIELD_DELEGATED" if enabled
                                      else "YIELD_DELEGATED", names)
                    with self.assertRaises(StopIteration):
                        next(iterator)
                finally:
                    monitor.set_local_events(tool, code, 0)
                    suspended.close()
            self.assertEqual([value for code, value in seen if code is outer.__code__], [1, 3])
            self.assertEqual([value for code, value in seen if code is awaiting.__code__], [1, 3])
        finally:
            monitor.register_callback(tool, event, None)
            monitor.free_tool_id(tool)

    def test_actual_send_body_and_cursor_transport_are_required(self):
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
        send_names = [name for name, fmt in formats.items() if fmt.group == "SEND"]
        self.assertGreaterEqual(len(send_names), 4)
        expected = ir_generator.OwnershipEvent(
            "_PyIR_OWNER_NORMAL_COMPLETION", flags=3, src=3, dst=1, extent=2)
        for name in send_names:
            self.assertEqual(contracts[name].ownership_events, (expected,))
        for name in ("YIELD_DELEGATED", "INSTRUMENTED_YIELD_DELEGATED"):
            self.assertEqual(formats[name].roles,
                             [ir_generator.ROLE_DST, f"{ir_generator.ROLE_IN0} + 2",
                              ir_generator.ROLE_NONE, f"{ir_generator.ROLE_IN0} + 0"])
            self.assertEqual(formats[name].metadata_inputs, frozenset({1}))
            self.assertEqual(ir_metadata_generator.shadow_of(name, analysis),
                             "INSTRUMENTED_YIELD_VALUE" if name.startswith("INSTRUMENTED_")
                             else "YIELD_VALUE")
            self.assertTrue(contracts[name].abrupt_prefix)
            self.assertEqual(contracts[name].ownership_events, (
                ir_generator.OwnershipEvent("_PyIR_OWNER_YIELD_DELEGATED"),))
        text = source.read_text()
        with tempfile.TemporaryDirectory() as directory:
            changed = Path(directory) / "bytecodes.c"
            marker = "null_or_index = window[1];"
            self.assertIn(marker, text)
            changed.write_text(text.replace(marker, "null_or_index = window[0];", 1))
            altered = analyzer.analyze_files([str(changed)])
            altered_formats = ir_generator.compute_formats(altered)
            altered_contracts = ir_generator.handler_contracts(altered, altered_formats)
            for name in send_names:
                self.assertNotIn(expected, altered_contracts[name].ownership_events)
            source_marker = "macro(SEND) = unused/1 + _SPECIALIZE_SEND + _SEND;"
            self.assertIn(source_marker, text)
            suffix = """op(_OPAQUE_SEND_SUFFIX,
                (receiver, cursor, value -- receiver, cursor, value)) {
                opaque(alias);
            }
            """
            changed.write_text(text.replace(source_marker, suffix +
                source_marker.replace("+ _SEND;", "+ _SEND + _OPAQUE_SEND_SUFFIX;"), 1))
            altered = analyzer.analyze_files([str(changed)])
            altered_formats = ir_generator.compute_formats(altered)
            altered_contracts = ir_generator.handler_contracts(altered, altered_formats)
            for name in send_names:
                self.assertNotIn(expected, altered_contracts[name].ownership_events)
            guard = "EXIT_IF(!PyValue_IsNone(val));"
            self.assertIn(guard, text)
            changed.write_text(text.replace(guard,
                guard + "\n_PyFrame_SetReturnValue(alias, PyValue_None);", 1))
            altered = analyzer.analyze_files([str(changed)])
            self.assertFalse(altered.uops["_GUARD_TOS_IS_NONE"].properties.escapes)
            altered_formats = ir_generator.compute_formats(altered)
            altered_contracts = ir_generator.handler_contracts(altered, altered_formats)
            for name in send_names:
                self.assertNotIn(expected, altered_contracts[name].ownership_events)
            changed.write_text(text.replace("IR_METADATA_PASSTHROUGH(1);",
                                            "IR_METADATA_PASSTHROUGH(1);\n(void)unused_cursor;", 1))
            altered = analyzer.analyze_files([str(changed)])
            with self.assertRaisesRegex(SyntaxError, "unchanged unread delegated cursor"):
                ir_generator.compute_formats(altered)


if __name__ == "__main__":
    unittest.main()
