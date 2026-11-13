"""Selected error snapshots come from reached post-flush Storage states."""

import gc
import sys
import types
import unittest
from unittest import mock

import _testinternalcapi
import _opcode
import opcode

from test.test_ir_call_roles import routes
from test.test_ir_completion import exception_entry
from test.test_ir_transfer_admission import make_code


class ErrorCheckpointTests(unittest.TestCase):
    def image(self, prefix=()):
        return make_code([
            ("RESUME", 0), *prefix,
            ("LOAD_FAST_AND_CLEAR", 3, 0), ("LOAD_FAST", 4, 1),
            ("GET_ANEXT", 5, 4), ("RETURN_VALUE", 5),
        ], slots=5, locals=("payload", "iterator", "mutable")).replace(co_argcount=2)

    def authority(self, code, site, checkpoint=1, **kw):
        return _testinternalcapi.check_ir_frame_phase(
            code, site, 0, error_checkpoint=checkpoint, **kw)

    def test_explicit_origin_query_and_public_routes(self):
        row = _opcode.get_ir_contract()[opcode.opmap["GET_ANEXT"]]
        self.assertTrue(row.flags & _opcode.IR_ERROR_PROGRAM)
        for code in routes(self.image()):
            self.assertEqual(self.authority(code, 3), 2)
            self.assertEqual(self.authority(code, 3, 2), 2)
            self.assertEqual(self.authority(code, 3, 0), 0)
            self.assertEqual(self.authority(code, 3, 255), 0)
            self.assertEqual(self.authority(code, 2), 0)
            self.assertEqual(self.authority(code, len(code.co_code) // 8), 0)
            self.assertEqual(self.authority(code, 3, unaligned=True), 0)
            self.assertEqual(self.authority(code, 3, wrong_image=True), 0)
            # Publication state is not the error origin or its authority.
            self.assertEqual(_testinternalcapi.check_ir_frame_phase(
                code, 3, 65535, error_checkpoint=1), 2)

    def test_actual_scope_keeps_survivors_through_monitor_and_retirement(self):
        monitor = sys.monitoring
        tool = monitor.DEBUGGER_ID
        observations = []
        escaped = []
        addresses = []
        function = None

        class Payload:
            def __init__(self):
                addresses.append(id(self))

            def __del__(self):
                frame = sys._getframe(1)
                while frame is not None and frame.f_code is not function.__code__:
                    frame = frame.f_back
                if frame is None:
                    observations.append(("missing frame",))
                    return
                frame.f_locals["mutable"] = 42
                gc.collect()
                observations.append(("retire",
                    _testinternalcapi.get_frame_home_view(frame)[0],
                    id(self) in _testinternalcapi.get_frame_selected_object_ids(frame),
                    _testinternalcapi.gc_verify()))

        class Iterator:
            def __anext__(self):
                raise LookupError("selected snapshot")

        def raised(code, offset, exception):
            if code is function.__code__:
                frame = sys._getframe(1)
                escaped.append(frame)
                frame.f_locals["mutable"] = 41
                observations.append(("raise",
                    _testinternalcapi.get_frame_home_view(frame)[0],
                    addresses[-1] in _testinternalcapi.get_frame_selected_object_ids(frame),
                    _testinternalcapi.gc_verify()))

        monitor.use_tool_id(tool, "selected-error-snapshot")
        try:
            monitor.register_callback(tool, monitor.events.RAISE, raised)
            for code in routes(self.image()):
                function = types.FunctionType(code, globals())
                monitor.set_local_events(tool, code, monitor.events.RAISE)
                try:
                    try:
                        function(Payload(), Iterator())
                    except LookupError as error:
                        self.assertEqual(str(error), "selected snapshot")
                    else:
                        self.fail("GET_ANEXT must fail")
                finally:
                    monitor.set_local_events(tool, code, 0)
                self.assertEqual(observations[-2:], [
                    ("raise", 2, True, []), ("retire", 2, False, [])])
                self.assertEqual(escaped[-1].f_locals["mutable"], 42)
                escaped[-1].clear()
        finally:
            monitor.register_callback(tool, monitor.events.RAISE, None)
            monitor.free_tool_id(tool)

    def test_actual_retired_scalar_input_is_absent_from_selected_snapshot(self):
        monitor = sys.monitoring
        tool = monitor.DEBUGGER_ID
        observations = []
        current = None
        payload_id = value_id = 0

        class Value:
            def __bool__(self):
                raise LookupError("retired scalar input")

        def raised(code, offset, exception):
            if code is current:
                frame = sys._getframe(1)
                selected = _testinternalcapi.get_frame_selected_object_ids(frame)
                observations.append((
                    _testinternalcapi.get_frame_home_view(frame)[0],
                    payload_id in selected, value_id in selected,
                    _testinternalcapi.gc_verify()))

        monitor.use_tool_id(tool, "scalar-error-actions")
        try:
            monitor.register_callback(tool, monitor.events.RAISE, raised)
            # A may be an independent recipient, the consumed input itself,
            # or a dynamic named binding. Replay coalesces these aliases
            # before interpreting the actual VACATE of B.
            for recipient in (5, 4, 2):
                image = make_code([
                    ("RESUME", 0), ("LOAD_FAST_AND_CLEAR", 3, 0),
                    ("LOAD_FAST", 4, 1), ("DELETE_FAST", 1),
                    ("UNARY_NOT", recipient, 4),
                    ("RETURN_VALUE", recipient),
                ], slots=5, locals=("payload", "value", "mutable")).replace(co_argcount=2)
                for current in routes(image):
                    self.assertEqual(self.authority(current, 4), 2)
                    function = types.FunctionType(current, globals())
                    payload, value = object(), Value()
                    payload_id, value_id = id(payload), id(value)
                    monitor.set_local_events(tool, current, monitor.events.RAISE)
                    try:
                        try:
                            function(payload, value)
                        except LookupError as error:
                            self.assertEqual(str(error), "retired scalar input")
                        else:
                            self.fail("the scalar predicate must fail")
                    finally:
                        monitor.set_local_events(tool, current, 0)
                    self.assertEqual(observations[-1], (2, True, False, []))
        finally:
            monitor.register_callback(tool, monitor.events.RAISE, None)
            monitor.free_tool_id(tool)

    def test_nullable_binding_does_not_supply_required_nonnull_read(self):
        # Named bindings are canonical but may be unbound. The scalar
        # action replay does not invent the missing boundness check.
        for words in (
                [("RESUME", 0), ("UNARY_NOT", 2, 0), ("RETURN_VALUE", 2)],
                [("RESUME", 0), ("LOAD_FAST_AND_CLEAR", 1, 0),
                 ("UNARY_NOT", 2, 1), ("RETURN_VALUE", 2)]):
            for code in routes(make_code(words, slots=2, locals=("unbound",))):
                self.assertEqual(self.authority(code, len(words) - 2), 0)

    def test_published_native_result_supplies_the_following_selected_error(self):
        monitor = sys.monitoring
        tool = monitor.DEBUGGER_ID
        current = None
        observations = []
        redirects = []
        retired = []
        payload_id = 0

        class Iterator:
            def __anext__(self):
                raise LookupError("published scalar result")

        class Value:
            def __invert__(self):
                return Iterator()

            def __del__(self):
                retired.append(True)

        def at_line(code, line):
            if code is Value.__del__.__code__:
                parent = sys._getframe(1).f_back
                if parent is not None and parent.f_code is current:
                    parent.f_locals["mutable"] = 42
                    try:
                        parent.f_lineno = parent.f_lineno
                    except ValueError as error:
                        redirects.append(str(error))
                    else:
                        self.fail("a native value operation is not an active CALL recipe")
                    self.assertEqual(_testinternalcapi.gc_verify(), [])

        def raised(code, offset, exception):
            if code is current:
                frame = sys._getframe(1)
                observations.append((
                    _testinternalcapi.get_frame_home_view(frame)[0],
                    payload_id in _testinternalcapi.get_frame_selected_object_ids(frame),
                    _testinternalcapi.gc_verify(), frame.f_locals["mutable"]))

        image = make_code([
            ("RESUME", 0), ("LOAD_FAST_AND_CLEAR", 3, 0),
            ("LOAD_FAST", 4, 1), ("DELETE_FAST", 1),
            ("UNARY_INVERT", 5, 4), ("GET_ANEXT", 6, 5),
            ("RETURN_VALUE", 6),
        ], slots=5, locals=("payload", "value", "mutable")).replace(co_argcount=2)
        monitor.use_tool_id(tool, "published-result-error")
        try:
            monitor.register_callback(tool, monitor.events.RAISE, raised)
            monitor.register_callback(tool, monitor.events.LINE, at_line)
            monitor.set_local_events(tool, Value.__del__.__code__, monitor.events.LINE)
            for current in routes(image):
                self.assertEqual(self.authority(current, 4), 2)
                # This read needs the previous native result's actual
                # canonical publication and successful NULL-guard arm.
                self.assertEqual(self.authority(current, 5), 2)
                function = types.FunctionType(current, globals())
                payload = object()
                payload_id = id(payload)
                monitor.set_local_events(tool, current, monitor.events.RAISE)
                try:
                    try:
                        function(payload, Value())
                    except LookupError as error:
                        self.assertEqual(str(error), "published scalar result")
                    else:
                        self.fail("the published iterator must fail")
                finally:
                    monitor.set_local_events(tool, current, 0)
                self.assertEqual(observations[-1], (2, True, [], 42))
                self.assertTrue(retired)
                self.assertTrue(redirects)
                self.assertTrue(all("unsupported active call recipe" in error
                                    for error in redirects))
        finally:
            monitor.set_local_events(tool, Value.__del__.__code__, 0)
            monitor.register_callback(tool, monitor.events.LINE, None)
            monitor.register_callback(tool, monitor.events.RAISE, None)
            monitor.free_tool_id(tool)

    def test_unknown_and_joined_may_shape_cannot_supply_readability(self):
        # The opaque operation is valid, but its whole incoming shape is not
        # a positive checkpoint certificate.
        unknown = make_code([
            ("RESUME", 0), ("LOAD_FAST_AND_CLEAR", 3, 0),
            ("UNARY_NOT", 3, 3), ("LOAD_FAST", 4, 1),
            ("GET_ANEXT", 5, 4), ("RETURN_VALUE", 5),
        ], slots=5, locals=("payload", "iterator", "mutable"))
        # A later valid scalar graph can publish its own result, but cannot
        # recover the unrelated incoming shape lost at an uncovered read.
        unknown_then_published = make_code([
            ("RESUME", 0), ("LOAD_FAST_AND_CLEAR", 3, 0),
            ("UNARY_NOT", 3, 3), ("LOAD_FAST", 4, 1),
            ("UNARY_INVERT", 5, 4), ("GET_ANEXT", 6, 5),
            ("RETURN_VALUE", 6),
        ], slots=5, locals=("payload", "iterator", "mutable"))
        # One predecessor has a scratch owner; the other has never
        # initialized that home. MAY is insufficient even with a real read.
        joined = make_code([
            ("RESUME", 0), ("LOAD_FAST", 6, 0),
            ("POP_JUMP_IF_NONE", 6, 0, 5),
            ("LOAD_FAST_AND_CLEAR", 3, 0), ("IR_JUMP", 0, 0, 6),
            ("IR_JUMP", 0, 0, 6), ("LOAD_FAST", 4, 1),
            ("GET_ANEXT", 5, 4), ("RETURN_VALUE", 5),
        ], slots=5, locals=("payload", "iterator", "mutable"))
        for image, site in ((unknown, 4), (unknown_then_published, 5), (joined, 7)):
            for code in routes(image):
                self.assertEqual(self.authority(code, site), 0)

        # Different incoming values can share a shape. Visit both physical
        # predecessor orders; object identity is not a shape certificate.
        for first, second in (("LOAD_FAST", "LOAD_CONST"),
                              ("LOAD_CONST", "LOAD_FAST")):
            image = make_code([
                ("RESUME", 0), ("LOAD_FAST", 6, 0),
                ("POP_JUMP_IF_NONE", 6, 0, 5), (first, 3, 0),
                ("IR_JUMP", 0, 0, 6), (second, 3, 0),
                ("LOAD_FAST", 4, 1), ("GET_ANEXT", 5, 4),
                ("RETURN_VALUE", 5),
            ], slots=5, locals=("payload", "iterator", "mutable"))
            for code in routes(image):
                self.assertEqual(self.authority(code, 7), 2)

    def test_new_handler_protocol_uses_explicit_control_until_transformed(self):
        words = [
            ("RESUME", 0), ("LOAD_FAST_AND_CLEAR", 3, 0),
            ("LOAD_FAST", 4, 1), ("GET_ANEXT", 5, 4),
            ("RETURN_VALUE", 5), ("POP_TOP", 6),
            ("LOAD_CONST", 7), ("RETURN_VALUE", 7),
        ]
        code = make_code(words, slots=5,
            locals=("payload", "iterator", "mutable")).replace(
                co_argcount=2, co_exceptiontable=exception_entry(3, 1, 5, 6 << 1))
        function = types.FunctionType(code, globals())
        observations = []
        monitor = sys.monitoring
        tool = monitor.DEBUGGER_ID

        class Iterator:
            def __anext__(self):
                raise LookupError("handled selected error")

        def handled(current, offset, exception):
            if current is code:
                frame = sys._getframe(1)
                observations.append((_testinternalcapi.get_frame_home_view(frame)[0],
                    id(exception) in _testinternalcapi.get_frame_selected_object_ids(frame),
                    _testinternalcapi.gc_verify()))

        monitor.use_tool_id(tool, "selected-error-handler-entry")
        try:
            monitor.register_callback(tool, monitor.events.EXCEPTION_HANDLED, handled)
            monitor.set_local_events(tool, code, monitor.events.EXCEPTION_HANDLED)
            self.assertIsNone(function(object(), Iterator()))
        finally:
            monitor.set_local_events(tool, code, 0)
            monitor.register_callback(tool, monitor.events.EXCEPTION_HANDLED, None)
            monitor.free_tool_id(tool)
        self.assertEqual(observations, [(3, True, [])])

    def test_source_and_actual_flush_receipts_are_both_required(self):
        from pathlib import Path
        from test import test_tools
        test_tools.skip_if_missing("cases_generator")
        with test_tools.imports_under_tool("cases_generator"):
            import analyzer
            import ir_error_effects
            import ir_generator

        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        analysis = analyzer.analyze_files([str(source)])
        formats = ir_generator.compute_formats(analysis)

        def checkpoints():
            return ir_generator.handler_contracts(analysis, formats)["GET_ANEXT"].error_checkpoints

        self.assertTrue(checkpoints())
        # An absence-only callback effect is not a readability contract.
        helper = ir_error_effects.HELPERS["_PyEval_GetANext"]
        with mock.patch.dict(ir_error_effects.HELPERS, {"_PyEval_GetANext":
                ir_error_effects.HelperEffect(helper.inputs, helper.result,
                    ir_error_effects.HomeEffect.PRESERVES_SCRATCH_ABSENCE,
                    helper.may_fail, helper.callback)}):
            self.assertFalse(checkpoints())
        with mock.patch.object(ir_generator.IREmitter, "operand_error_flush", lambda *args: None):
            self.assertFalse(checkpoints())
        original = ir_generator.IRStack.flush

        def writing_flush(stack, *args, **kwargs):
            result = original(stack, *args, **kwargs)
            # A write without its matching action receipt is uncovered.
            stack.home_writes += 1
            return result

        with mock.patch.object(ir_generator.IRStack, "flush", writing_flush):
            self.assertFalse(checkpoints())
        part = analysis.instructions["GET_ANEXT"].parts[-1]
        original_nullable = part.stack.inputs[0].nullable
        part.stack.inputs[0].nullable = True
        try:
            # An annotation does not by itself prove NULL-safe source use.
            self.assertFalse(checkpoints())
        finally:
            part.stack.inputs[0].nullable = original_nullable


if __name__ == "__main__":
    unittest.main()
