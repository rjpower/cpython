"""Delegated suspension carries an explicit receiver role."""

from test.support import ir_dis as opcode
import sys
from pathlib import Path
import tempfile
import textwrap
import types
import unittest

import _opcode
import _testinternalcapi
from test.support import script_helper
from test.test_ir_call_roles import routes


CONTRACTS = _opcode.get_ir_contract()


def ordinary_name(name):
    # The word flags after the handler's name stay with the ordinary handler
    handler, plus, flags = name.partition("+")
    return (opcode.opname[CONTRACTS[opcode.opname.index(handler)].ordinary_handler]
            + plus + flags)


class DelegatedYieldTests(unittest.TestCase):
    def test_throw_completed_by_a_stop_iteration_without_a_value(self):
        # A subiterator's throw() ends the delegation with a StopIteration
        # whose value field is NULL (made by __new__ without __init__, or
        # after `del e.value`): the delegation's result is None, as the
        # member reads it (it used to be a NULL count, a crash).  Under the
        # JIT off and always (the word compiled)
        source = textwrap.dedent("""
            def stop_new():
                return StopIteration.__new__(StopIteration)
            def stop_deleted():
                e = StopIteration(5)
                del e.value
                return e
            def stop_five():
                return StopIteration(5)
            def stop_empty():
                return StopIteration()
            class Sub:
                def __init__(self, make):
                    self.make = make
                def __iter__(self):
                    return self
                def __next__(self):
                    return 1
                def throw(self, *args):
                    raise self.make()
            def outer(make):
                return (yield from Sub(make))
            for _ in range(3):
                for make in (stop_new, stop_deleted, stop_five, stop_empty):
                    g = outer(make)
                    next(g)
                    try:
                        g.throw(ValueError)
                    except StopIteration as e:
                        print(e.value)
        """)
        modes = ["off"]
        if sys._jit.is_available():
            modes.append("always")
        for mode in modes:
            with self.subTest(jit=mode):
                result = script_helper.assert_python_ok("-X", f"jit={mode}", "-c", source)
                self.assertEqual(result.out.decode().split(), ["None", "None", "5", "None"] * 3)

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
            selected = [row for row in rows if ordinary_name(row[0]) == "YIELD_DELEGATED"]
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
                original = next(CONTRACTS[opcode.opname.index(row[0].partition("+")[0])]
                                for row in _testinternalcapi.get_ir(code)
                                if ordinary_name(row[0]) == "YIELD_DELEGATED")
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
                        names = {ordinary_name(row[0]) for row in rows}
                        self.assertIn("INSTRUMENTED_YIELD_DELEGATED" if enabled
                                      else "YIELD_DELEGATED", names)
                        actual = next(CONTRACTS[opcode.opname.index(row[0].partition("+")[0])]
                                      for row in rows if ordinary_name(row[0]) ==
                                      ("INSTRUMENTED_YIELD_DELEGATED" if enabled
                                       else "YIELD_DELEGATED"))
                        self.assertEqual(
                            (actual.generator_yielded_consumer,
                             actual.generator_completed_consumer,
                             actual.generator_resume_consumer),
                            (original.generator_yielded_consumer,
                             original.generator_completed_consumer,
                             original.generator_resume_consumer))
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
        boundaries = ir_generator.generator_boundaries(analysis)
        send_names = [name for name, fmt in formats.items()
                      if fmt.group == "SEND" and name not in boundaries]
        self.assertGreaterEqual(len(send_names), 4)
        expected = ir_generator.OwnershipEvent(
            "_PyIR_OWNER_NORMAL_COMPLETION", flags=3, src=3, dst=1, extent=2)
        for name in send_names:
            self.assertEqual(contracts[name].ownership_events, (expected,))
        for name, boundary in boundaries.items():
            if boundary.yielded == "DELEGATED":
                self.assertEqual(contracts[name].ownership_events, (
                    ir_generator.OwnershipEvent(
                        "_PyIR_OWNER_NATIVE_WINDOW_OUT", flags=1, src=3, dst=0, extent=2),
                    ir_generator.OwnershipEvent(
                        "_PyIR_OWNER_NATIVE_WINDOW_OUT", flags=1, src=3, dst=1)))
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
            # The cursor goes by value; a send that drops it is refused where
            # _SEND's declaration is checked
            marker = "PyValue cursor = TAKE(index);"
            self.assertIn(marker, text)
            changed.write_text(text.replace(marker, "PyValue cursor = PyValue_NULL;", 1))
            altered = analyzer.analyze_files([str(changed)])
            altered_formats = ir_generator.compute_formats(altered)
            with self.assertRaisesRegex(SyntaxError, "_SEND: on next, `index` is still owned"):
                ir_generator.build_records(altered, altered_formats)
            # A uop that declares more than a SEND may is no SEND source
            marker = "effects(runs_python, switches_frame) exits(next, branch, enter, raise)"
            self.assertIn(marker, text)
            changed.write_text(text.replace(
                marker, "effects(runs_python, switches_frame, polls) exits(next, branch, enter, raise)", 1))
            altered = analyzer.analyze_files([str(changed)])
            altered_formats = ir_generator.compute_formats(altered)
            with self.assertRaisesRegex(SyntaxError,
                    "generator boundary needs the complete SEND source contract"):
                ir_generator.handler_contracts(altered, altered_formats)
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
            with self.assertRaisesRegex(SyntaxError,
                    "generator boundary needs the complete SEND source contract"):
                ir_generator.handler_contracts(altered, altered_formats)
            guard = "EXIT_IF(!PyValue_IsNone(val));"
            self.assertIn(guard, text)
            changed.write_text(text.replace(guard,
                guard + "\n_PyFrame_SetReturnValue(alias, PyValue_None);", 1))
            altered = analyzer.analyze_files([str(changed)])
            # A call that no row declares may run anything
            self.assertTrue(altered.uops["_GUARD_TOS_IS_NONE"].properties.escapes)
            altered_formats = ir_generator.compute_formats(altered)
            # The write precedes the guard of the next uop.  The uop declares
            # its contract, so the verifier refuses the call (it has no row)
            # before the guard order is checked
            with self.assertRaisesRegex(
                    SyntaxError,
                    "guards that follow a change|which pycore_ir_components.h does not declare"):
                ir_generator.handler_contracts(altered, altered_formats)
            changed.write_text(text.replace("IR_METADATA_PASSTHROUGH(1);",
                                            "IR_METADATA_PASSTHROUGH(1);\n(void)unused_cursor;", 1))
            altered = analyzer.analyze_files([str(changed)])
            with self.assertRaisesRegex(SyntaxError, "unchanged unread delegated cursor"):
                ir_generator.compute_formats(altered)


if __name__ == "__main__":
    unittest.main()
