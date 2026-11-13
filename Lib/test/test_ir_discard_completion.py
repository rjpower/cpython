"""Python child completion owns ignored results without a temporary POP."""

import dis
import gc
import io
from pathlib import Path
import sys
import types
import unittest
from unittest import mock
import weakref
from test import support, test_tools
from test.support import import_helper

_testinternalcapi = import_helper.import_module("_testinternalcapi")
VARIANTS = ("CALL_NO_SELF_PY_EXACT_ARGS", "CALL_NO_SELF_PY_GENERAL")
ROUNDS = _testinternalcapi.SPECIALIZATION_THRESHOLD + 10


def handlers(func):
    return [word[0] for word in _testinternalcapi.get_ir(func.__code__, True)]


def make_caller():
    def caller(make):
        make()
        return 7
    # Separate functions share a code object unless it is copied explicitly.
    # Keep warmed call feedback independent across makers and test cases.
    return types.FunctionType(caller.__code__.replace(), globals())


@unittest.skipIf(support.Py_GIL_DISABLED, "requires GIL specialization")
class DiscardCompletionTests(unittest.TestCase):
    def test_exact_and_general_complete_without_result_home(self):
        for general, variant in enumerate(VARIANTS):
            with self.subTest(general=general):
                events = []
                caller = make_caller()
                class Local:
                    def __del__(self):
                        events.append("local")
                class Result:
                    def __del__(self):
                        frame = sys._getframe(1)
                        events.append(("result", frame.f_code, frame.f_lasti))
                def exact():
                    local = Local()
                    return Result()
                def general_call(*args):
                    local = Local()
                    return Result()
                make = general_call if general else exact
                for _ in range(ROUNDS):
                    self.assertEqual(caller(make), 7)
                self.assertIn(variant, handlers(caller))
                call = next(i.offset for i in dis.get_instructions(caller)
                            if i.opname == "CALL")
                events.clear()
                self.assertEqual(caller(make), 7)
                self.assertEqual(events, ["local", ("result", caller.__code__, call)])
                self.assertNotIn("POP_TOP", [i.opname for i in dis.get_instructions(caller)])

    def test_warmed_inline_results_on_both_routes(self):
        for general, variant in enumerate(VARIANTS):
            for value in (None, False, 17, 3.25):
                with self.subTest(general=general, value=value):
                    caller = make_caller()
                    def exact():
                        return value
                    def general_call(*args):
                        return value
                    make = general_call if general else exact
                    for _ in range(ROUNDS):
                        self.assertEqual(caller(make), 7)
                    self.assertIn(variant, handlers(caller))
                    self.assertEqual(caller(make), 7)

    def test_child_cleanup_enables_opcode_observation_before_disposal(self):
        for general in (False, True):
            with self.subTest(general=general):
                caller = make_caller()
                seen = []
                armed = False
                def tracer(frame, event, arg):
                    if frame.f_code is caller.__code__:
                        frame.f_trace_opcodes = True
                        seen.append((event, frame.f_lasti))
                    return tracer
                class Local:
                    def __del__(self):
                        if armed:
                            frame = sys._getframe(1)
                            frame.f_trace = tracer
                            frame.f_trace_opcodes = True
                            sys.settrace(tracer)
                class Result:
                    def __del__(self):
                        if armed:
                            seen.append(("result", sys._getframe(1).f_lasti))
                def exact():
                    local = Local()
                    return Result()
                def general_call(*args):
                    local = Local()
                    return Result()
                make = general_call if general else exact
                for _ in range(ROUNDS):
                    caller(make)
                self.assertIn(VARIANTS[int(general)], handlers(caller))
                call = next(i.offset for i in dis.get_instructions(caller)
                            if i.opname == "CALL")
                successor = next(i.offset for i in dis.get_instructions(caller)
                                 if i.offset > call)
                armed = True
                try:
                    self.assertEqual(caller(make), 7)
                finally:
                    sys.settrace(None)
                self.assertIn(("result", call), seen)
                self.assertIn(("opcode", successor), seen)
                self.assertLess(seen.index(("result", call)),
                                seen.index(("opcode", successor)))

    def test_result_finalizer_enables_observation_after_discard_boundary(self):
        for mode in ("trace", "monitor"):
            with self.subTest(mode=mode):
                caller = make_caller()
                armed = False
                seen = []
                positions = []
                mon = sys.monitoring
                tool = mon.DEBUGGER_ID
                def tracer(frame, event, arg):
                    if frame.f_code is caller.__code__:
                        frame.f_trace_opcodes = True
                        if event == "opcode":
                            seen.append(frame.f_lasti)
                    return tracer
                class Result:
                    def __del__(self):
                        if not armed:
                            return
                        parent = sys._getframe(1)
                        positions.append(parent.f_lasti)
                        if mode == "trace":
                            parent.f_trace = tracer
                            parent.f_trace_opcodes = True
                            sys.settrace(tracer)
                        else:
                            mon.set_local_events(tool, caller.__code__, mon.events.INSTRUCTION)
                def make():
                    return Result()
                for _ in range(ROUNDS):
                    caller(make)
                self.assertIn(VARIANTS[0], handlers(caller))
                call = next(i.offset for i in dis.get_instructions(caller)
                            if i.opname == "CALL")
                successor = next(i.offset for i in dis.get_instructions(caller)
                                 if i.offset > call)
                if mode == "monitor":
                    mon.use_tool_id(tool, "discard finalizer")
                    mon.register_callback(tool, mon.events.INSTRUCTION,
                        lambda code, offset: seen.append(offset) if code is caller.__code__ else None)
                armed = True
                try:
                    self.assertEqual(caller(make), 7)
                finally:
                    if mode == "trace":
                        sys.settrace(None)
                    else:
                        mon.set_local_events(tool, caller.__code__, 0)
                        mon.free_tool_id(tool)
                self.assertEqual(positions, [call])
                self.assertTrue(seen)
                self.assertNotIn(call, seen)
                self.assertTrue(all(offset >= successor for offset in seen))

    def test_local_monitoring_from_callee_and_callback_error_cleanup(self):
        caller = make_caller()
        armed = False
        references = []
        mon = sys.monitoring
        tool = mon.DEBUGGER_ID
        class Result:
            pass
        def make(*args):
            if armed:
                mon.set_local_events(tool, caller.__code__, mon.events.INSTRUCTION)
            result = Result()
            references.append(weakref.ref(result))
            return result
        for _ in range(ROUNDS):
            caller(make)
        self.assertIn(VARIANTS[1], handlers(caller))
        call = next(i.offset for i in dis.get_instructions(caller)
                    if i.opname == "CALL")
        successor = next(i.offset for i in dis.get_instructions(caller)
                         if i.offset > call)
        def observer(code, offset):
            if code is caller.__code__ and offset == successor:
                raise LookupError("discard observer")
        mon.use_tool_id(tool, "discard completion")
        try:
            mon.register_callback(tool, mon.events.INSTRUCTION, observer)
            armed = True
            with self.assertRaisesRegex(LookupError, "discard observer"):
                caller(make)
        finally:
            mon.set_local_events(tool, caller.__code__, 0)
            mon.free_tool_id(tool)
        gc.collect()
        self.assertTrue(all(ref() is None for ref in references))

    def test_cross_frame_lineno_change_preserves_semantic_discard(self):
        caller = make_caller()
        armed = False
        jumped = False
        class Result:
            pass
        def tracer(frame, event, arg):
            nonlocal jumped
            if armed and not jumped and frame.f_code is make.__code__ and event == "line":
                parent = frame.f_back
                parent.f_lineno = caller.__code__.co_firstlineno + 2
                jumped = True
            return tracer
        def make():
            if armed:
                parent = sys._getframe(1)
                parent.f_trace = tracer
                sys._getframe().f_trace = tracer
                sys.settrace(tracer)
            result = Result()
            return result
        for _ in range(ROUNDS):
            caller(make)
        self.assertIn(VARIANTS[0], handlers(caller))
        armed = True
        try:
            result = caller(make)
        finally:
            sys.settrace(None)
        self.assertTrue(jumped)
        # The child result is ignored by the semantic CALL. Redirecting its
        # continuation does not reconstruct a removed evaluation-stack value.
        self.assertEqual(result, 7)

    def test_result_reachability_and_guard_failure(self):
        caller = make_caller()
        references = []
        class Result:
            pass
        def make():
            value = Result()
            references.append(weakref.ref(value))
            return value
        for _ in range(ROUNDS):
            caller(make)
        self.assertIn(VARIANTS[0], handlers(caller))
        self.assertTrue(all(ref() is None for ref in references))
        self.assertEqual(caller(lambda: 17), 7)
        def raising():
            raise LookupError("call completion")
        with self.assertRaisesRegex(LookupError, "call completion"):
            caller(raising)
        self.assertEqual(caller(make), 7)
        gc.collect()
        self.assertTrue(all(ref() is None for ref in references))

    def test_discard_recipient_with_generator_caller_and_callee(self):
        def make():
            return object()
        def caller():
            make()
            sent = yield 1
            return sent
        ordinary = make_caller()
        events = []
        def generator():
            events.append("entered")
            yield 1
        for _ in range(ROUNDS):
            active = caller()
            self.assertEqual(next(active), 1)
            with self.assertRaises(StopIteration) as caught:
                active.send(23)
            self.assertEqual(caught.exception.value, 23)
            self.assertEqual(ordinary(generator), 7)
        self.assertEqual(events, [])
        # This contract concerns unused CALL results. Generator entry and
        # iteration completion have separate ownership lowering; a whole-
        # program POP inventory would conflate those unfinished contracts.
        for function in (caller, ordinary):
            with self.subTest(function=function):
                calls = [word for word in
                         _testinternalcapi.get_ir(function.__code__)
                         if word[0] == "CALL_NO_SELF"]
                self.assertEqual(len(calls), 1)
                self.assertEqual(calls[0][1], 4095)


@unittest.skipIf(test_tools.basepath is None, "needs the source tree")
class DiscardCompletionGeneratorTests(unittest.TestCase):
    def test_one_protocol_for_both_python_entry_routes(self):
        test_tools.skip_if_missing("cases_generator")
        with test_tools.imports_under_tool("cases_generator"):
            import analyzer
            import ir_generator as gen
        analysis = analyzer.analyze_files([
            str(Path(test_tools.basepath) / "Python" / "bytecodes.c")])
        output = io.StringIO()
        gen.generate_ir([], analysis, output, False)
        generated = output.getvalue()
        for name in (*VARIANTS, "CALL_PY_EXACT_ARGS", "CALL_PY_GENERAL"):
            self.assertIn(f"TARGET({name}) {{", generated)
        self.assertNotIn("IR_POSTCALL_DISCARD", generated)
        self.assertNotIn("CALL_POSTCALL_DISCARD", generated)
        self.assertNotIn("TARGET(CALL_PY_EXACT_ARGS_DISCARD)", generated)
        self.assertNotIn("TARGET(CALL_PY_GENERAL_DISCARD)", generated)
        self.assertIn("frame->return_slot = (uint16_t)IR_A(ir_word)", generated)


if __name__ == "__main__":
    unittest.main()
