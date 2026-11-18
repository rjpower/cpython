"""Complete generator transfers retain ordinary specialization and ownership."""

import gc
import marshal
import opcode
import struct
import sys
import types
import unittest
import weakref

import _opcode
import _testinternalcapi
from test import support
from test.test_ir_admission import construct


HANDLERS = {**opcode.opmap, **opcode._specialized_opmap, **opcode._ir_opmap}
NAMES = {number: name for name, number in HANDLERS.items()}
CONTRACTS = _opcode.get_ir_contract()
ROUNDS = _testinternalcapi.SPECIALIZATION_THRESHOLD * 3


def sites(function, adaptive=False):
    return [(pc, word, CONTRACTS[HANDLERS[word[0]]])
            for pc, word in enumerate(
                _testinternalcapi.get_ir(function.__code__, adaptive))]


def ordinary_names(function):
    return {NAMES[row.ordinary_handler] for _, _, row in sites(function, True)}


def leaf(value):
    received = yield value
    return received


def delegated(value):
    return (yield from leaf(value))


def finish(generator, value):
    try:
        generator.send(value)
    except StopIteration as result:
        return result.value
    raise AssertionError("generator did not complete")


class GeneratorContractExportTests(unittest.TestCase):
    def test_metadata_only_operation_has_an_ordinary_identity(self):
        handler = HANDLERS["BINARY_OP"]
        row = CONTRACTS[handler]
        self.assertEqual(row.ordinary_handler, handler)
        self.assertEqual(row.generator_yielded_consumer, _opcode.GEN_CONSUMER_NONE)
        self.assertEqual(row.generator_completed_consumer, _opcode.GEN_CONSUMER_NONE)
        self.assertEqual(row.generator_resume_consumer, _opcode.GEN_CONSUMER_NONE)
        self.assertEqual(row.generator_boundary_flags, 0)


@unittest.skipIf(support.Py_GIL_DISABLED, "requires adaptive specialization")
class SpecializationPreservationTests(unittest.TestCase):
    def test_constructor_init_specialization_survives_result_return(self):
        class Point:
            def __init__(self, value):
                self.value = value

        def build(value):
            return Point(value)

        for value in range(ROUNDS):
            self.assertEqual(build(value).value, value)
        self.assertIn("CALL_NO_SELF_ALLOC_AND_ENTER_INIT", ordinary_names(build))
        for _, _, row in sites(build):
            self.assertEqual(row.generator_boundary_flags, 0)

    def test_float_provider_specializations_survive_result_return(self):
        def add(left, right):
            return left + right

        def subtract(left, right):
            return left - right

        for _ in range(ROUNDS):
            self.assertEqual(add(1.25, 2.5), 3.75)
            self.assertEqual(subtract(2.5, 1.25), 1.25)
        self.assertIn("BINARY_OP_ADD_FLOAT", ordinary_names(add))
        self.assertIn("BINARY_OP_SUBTRACT_FLOAT", ordinary_names(subtract))
        for function in (add, subtract):
            for _, _, row in sites(function):
                self.assertEqual(row.generator_boundary_flags, 0)


@unittest.skipIf(support.Py_GIL_DISABLED, "generator placement requires the GIL")
class GeneratorBoundaryTests(unittest.TestCase):
    def assert_completed_shape(self, function, consumer):
        selected = [(pc, word, row) for pc, word, row in sites(function)
                    if row.generator_completed_consumer == consumer]
        self.assertTrue(selected, function.__qualname__)
        for _, _, row in selected:
            self.assertTrue(row.generator_boundary_flags & _opcode.GEN_BOUNDARY_CLOSED)
        return selected

    def assert_rejected_image(self, code, words, message="incomplete generator boundary"):
        original = code.co_code
        replacement = struct.pack(f"<{len(words)}Q", *words)
        for route in (construct, lambda source, **kw: source.replace(**kw)):
            with self.assertRaisesRegex(ValueError, message):
                route(code, co_code=replacement,
                      co_exceptiontable=code.co_exceptiontable)
        payload = marshal.dumps(code)
        self.assertEqual(payload.count(original), 1)
        with self.assertRaisesRegex(ValueError, message):
            marshal.loads(payload.replace(original, replacement, 1))
        self.assertEqual(code.co_code, original)

    def test_complete_yield_resume_and_return_shape(self):
        self.assert_completed_shape(delegated, _opcode.GEN_CONSUMER_RETURN)
        self.assertTrue(any(row.generator_resume_consumer == _opcode.GEN_CONSUMER_SEND
                            for _, _, row in sites(delegated)))
        self.assertTrue(any(row.generator_resume_consumer ==
                            _opcode.GEN_CONSUMER_STORE_LOCAL
                            for _, _, row in sites(leaf)))
        value, reply = object(), object()
        for _ in range(ROUNDS):
            iterator = delegated(value)
            self.assertIs(next(iterator), value)
            self.assertIs(finish(iterator, reply), reply)
        self.assertTrue(any(row.generator_yielded_consumer ==
                            _opcode.GEN_CONSUMER_DELEGATED
                            for _, _, row in sites(delegated, True)))

    def test_explicit_local_and_discard_completion(self):
        def stored(value):
            result = yield from leaf(value)
            return result

        def discarded(value):
            yield from leaf(value)
            return None

        value, reply = object(), object()
        for function, expected, consumer in (
                (stored, reply, _opcode.GEN_CONSUMER_STORE_FAST),
                (discarded, None, _opcode.GEN_CONSUMER_POP_TOP)):
            with self.subTest(function=function.__name__):
                selected = self.assert_completed_shape(function, consumer)
                words = _testinternalcapi.get_ir(function.__code__)
                for pc, word, row in selected:
                    target_pc = (word[3] | word[4] << 12
                                 if NAMES[row.ordinary_handler] == "CLEANUP_THROW"
                                 else pc + 1 + ((word[4] ^ 2048) - 2048))
                    target = words[target_pc]
                    self.assertGreaterEqual(word[1], function.__code__.co_nlocals)
                    self.assertEqual(target[0], "STORE_FAST" if consumer ==
                                     _opcode.GEN_CONSUMER_STORE_FAST else "POP_TOP")
                    self.assertEqual(target[1], word[1])
                    if consumer == _opcode.GEN_CONSUMER_STORE_FAST:
                        self.assertLess(target[2], function.__code__.co_nlocals)
                iterator = function(value)
                self.assertIs(next(iterator), value)
                self.assertIs(finish(iterator, reply), expected)

    def test_home_result_survives_another_await_and_feeds_arithmetic(self):
        async def identity(value):
            return value

        async def add(left, right):
            return await identity(left) + await identity(right)

        selected = self.assert_completed_shape(add, _opcode.GEN_CONSUMER_HOME)
        sends = [(pc, word) for pc, word, row in selected
                 if row.generator_yielded_consumer == _opcode.GEN_CONSUMER_DELEGATED]
        self.assertEqual(len(sends), 2)
        words = _testinternalcapi.get_ir(add.__code__)
        exits = [words[pc + 1 + ((word[4] ^ 2048) - 2048)][0]
                 for pc, word in sends]
        self.assertNotIn("RETURN_VALUE", exits)
        self.assertNotIn("RETURN_VALUE_EMPTY", exits)
        for _ in range(ROUNDS):
            self.assertEqual(finish(add(17, 29), None), 46)

    def test_mortal_home_survives_a_suspending_await_and_provider_error(self):
        references, alive = [], []
        marker = object()

        class Result:
            def __init__(self):
                self.cycle = self
                references.append(weakref.ref(self))

            def __add__(self, other):
                gc.collect()
                alive.append(references[0]() is self)
                raise LookupError("arithmetic provider")

        class Pause:
            def __await__(self):
                gc.collect()
                alive.append(references[0]() is not None)
                yield marker
                gc.collect()
                alive.append(references[0]() is not None)
                return 1

        async def first():
            return Result()

        async def outer():
            return await first() + await Pause()

        self.assert_completed_shape(outer, _opcode.GEN_CONSUMER_HOME)
        coroutine = outer()
        self.assertIs(coroutine.send(None), marker)
        self.assertIsNotNone(references[0]())
        with self.assertRaisesRegex(LookupError, "arithmetic provider"):
            coroutine.send(None)
        self.assertEqual(alive, [True, True, True])
        self.assertIsNone(coroutine.cr_frame)
        gc.collect()
        self.assertIsNone(references[0]())

    def test_public_partial_yield_cycle_is_rejected(self):
        words = list(struct.unpack(f"<{len(delegated.__code__.co_code) // 8}Q",
                                   delegated.__code__.co_code))
        selected = [(pc, row) for pc, _, row in sites(delegated)
                    if row.generator_resume_consumer == _opcode.GEN_CONSUMER_SEND]
        self.assertEqual(len(selected), 1)
        pc, row = selected[0]
        words[pc] = (words[pc] & ~_opcode.IR_HANDLER_MASK) | row.ordinary_handler
        self.assert_rejected_image(delegated.__code__, words)

    def test_public_partial_send_cycle_is_rejected(self):
        code = delegated.__code__
        words = list(struct.unpack(f"<{len(code.co_code) // 8}Q", code.co_code))
        selected = [(pc, row) for pc, _, row in sites(delegated)
                    if row.generator_yielded_consumer == _opcode.GEN_CONSUMER_DELEGATED]
        self.assertEqual(len(selected), 1)
        pc, row = selected[0]
        words[pc] = (words[pc] & ~_opcode.IR_HANDLER_MASK) | row.ordinary_handler
        self.assert_rejected_image(code, words)

    def test_public_partial_throw_completion_is_rejected(self):
        code = delegated.__code__
        words = list(struct.unpack(f"<{len(code.co_code) // 8}Q", code.co_code))
        selected = [(pc, row) for pc, _, row in sites(delegated)
                    if NAMES[row.ordinary_handler] == "CLEANUP_THROW"]
        self.assertEqual(len(selected), 1)
        pc, row = selected[0]
        self.assertEqual(row.generator_completed_consumer, _opcode.GEN_CONSUMER_RETURN)
        words[pc] = (words[pc] & ~_opcode.IR_HANDLER_MASK) | row.ordinary_handler
        self.assert_rejected_image(code, words)

    def test_public_completion_join_requires_the_same_recipient_and_window(self):
        # Extra NULL scratch makes the alternate B a valid window. Neither
        # mutation changes a handler or fails an unrelated index bound.
        code = delegated.__code__.replace(
            co_stacksize=delegated.__code__.co_stacksize + 1,
            co_exceptiontable=delegated.__code__.co_exceptiontable)
        original = list(struct.unpack(f"<{len(code.co_code) // 8}Q", code.co_code))
        pc, row = next((pc, row) for pc, word, row in sites(delegated)
                       if NAMES[row.ordinary_handler] == "CLEANUP_THROW")
        self.assertEqual(row.generator_completed_consumer, _opcode.GEN_CONSUMER_RETURN)
        for shift in (16, 28):
            words = original.copy()
            value = (words[pc] >> shift) & 4095
            words[pc] = (words[pc] & ~(4095 << shift)) | ((value + 1) << shift)
            # The existing semantic owner proof can reject the inconsistent
            # branch before the new connected-shape check. Both are retained.
            self.assert_rejected_image(
                code, words,
                "incomplete generator boundary|semantic completion|"
                "reads unavailable scratch|unproved delegated")

    def test_custom_iterator_and_native_provider_error(self):
        reply = object()

        class Receiver:
            def __iter__(self):
                return self

            def __next__(self):
                return reply

            def send(self, value):
                raise LookupError("native send failure")

        def outer(receiver):
            return (yield from receiver)

        self.assert_completed_shape(outer, _opcode.GEN_CONSUMER_RETURN)
        iterator = outer(Receiver())
        self.assertIs(next(iterator), reply)
        with self.assertRaisesRegex(LookupError, "native send failure"):
            iterator.send(object())
        self.assertIsNone(iterator.gi_frame)

    def test_placed_virtual_send_advances_its_cursor(self):
        def outer(values):
            return (yield from values)

        self.assert_completed_shape(outer, _opcode.GEN_CONSUMER_RETURN)
        values = (17, 23, 31, 47)
        for _ in range(ROUNDS):
            iterator = outer(values)
            try:
                for value in values:
                    self.assertEqual(next(iterator), value)
                with self.assertRaises(StopIteration) as ended:
                    next(iterator)
                self.assertIsNone(ended.exception.value)
            finally:
                iterator.close()
        selected = [row for _, _, row in sites(outer, True)
                    if NAMES[row.ordinary_handler] == "SEND_VIRTUAL"]
        self.assertEqual(len(selected), 1)
        row = selected[0]
        self.assertTrue(row.generator_boundary_flags & _opcode.GEN_BOUNDARY_CLOSED)
        self.assertEqual(row.generator_yielded_consumer,
                         _opcode.GEN_CONSUMER_DELEGATED)
        self.assertEqual(row.generator_completed_consumer,
                         _opcode.GEN_CONSUMER_RETURN)

    def test_throw_completion_and_close_keep_real_receiver(self):
        value, result = object(), object()
        closed = []

        def child():
            try:
                yield value
            except LookupError:
                return result
            finally:
                closed.append(True)

        def outer():
            return (yield from child())

        self.assert_completed_shape(outer, _opcode.GEN_CONSUMER_RETURN)
        for action in ("throw", "close"):
            iterator = outer()
            self.assertIs(next(iterator), value)
            receiver = iterator.gi_yieldfrom
            self.assertIsInstance(receiver, types.GeneratorType)
            if action == "throw":
                with self.assertRaises(StopIteration) as ended:
                    iterator.throw(LookupError())
                self.assertIs(ended.exception.value, result)
            else:
                iterator.close()
            self.assertIsNone(receiver.gi_frame)
            self.assertIsNone(iterator.gi_frame)
        self.assertEqual(closed, [True, True])

    def test_child_frame_takeover_retains_code_and_bindings(self):
        value, reply = object(), object()
        iterator = delegated(value)
        self.assertIs(next(iterator), value)
        receiver = iterator.gi_yieldfrom
        frame = receiver.gi_frame
        self.assertIs(frame.f_code, leaf.__code__)
        self.assertIs(finish(iterator, reply), reply)
        self.assertIsNone(receiver.gi_frame)
        self.assertIs(frame.f_code, leaf.__code__)
        self.assertIs(frame.f_locals["value"], value)
        self.assertIs(frame.f_locals["received"], reply)

    def test_delegated_close_callback_error_finishes_both_frames(self):
        def child():
            try:
                yield None
            finally:
                raise LookupError("close callback")

        def outer():
            return (yield from child())

        self.assert_completed_shape(outer, _opcode.GEN_CONSUMER_RETURN)
        iterator = outer()
        next(iterator)
        receiver = iterator.gi_yieldfrom
        with self.assertRaisesRegex(LookupError, "close callback"):
            iterator.close()
        self.assertIsNone(receiver.gi_frame)
        self.assertIsNone(iterator.gi_frame)

    def test_monitoring_enabled_while_suspended_preserves_shape(self):
        def child(value):
            received = yield value
            received = yield received
            return received

        def outer(value):
            return (yield from child(value))

        monitor, tool = sys.monitoring, 2
        monitor.use_tool_id(tool, "generator boundary suspension")
        event = monitor.events.PY_YIELD
        seen = []
        monitor.register_callback(tool, event,
                                  lambda code, offset, value: seen.append(value))
        iterator = outer(object())
        try:
            next(iterator)
            receiver = iterator.gi_yieldfrom
            before = [(pc, row) for pc, _, row in sites(outer)
                      if row.generator_resume_consumer == _opcode.GEN_CONSUMER_SEND]
            self.assertEqual(len(before), 1)
            monitor.set_local_events(tool, outer.__code__, event)
            reply = object()
            self.assertIs(iterator.send(reply), reply)
            self.assertEqual(seen, [reply])
            self.assertIs(iterator.gi_yieldfrom, receiver)
            pc, original = before[0]
            actual = sites(outer, True)[pc][2]
            self.assertEqual(actual.ordinary_handler,
                             CONTRACTS[original.ordinary_handler].twin)
            self.assertEqual(actual.generator_resume_consumer,
                             original.generator_resume_consumer)
            monitor.set_local_events(tool, outer.__code__, 0)
            self.assertIs(finish(iterator, reply), reply)
            self.assertIsNone(receiver.gi_frame)
        finally:
            monitor.set_local_events(tool, outer.__code__, 0)
            monitor.register_callback(tool, event, None)
            monitor.free_tool_id(tool)
            iterator.close()

    def test_line_and_instruction_hooks_preserve_deopt_and_respecialization(self):
        def outer(value):
            result = yield from leaf(value)
            return result

        value, reply = object(), object()
        for _ in range(ROUNDS):
            iterator = outer(value)
            self.assertIs(next(iterator), value)
            self.assertIs(finish(iterator, reply), reply)
        self.assertIn("SEND_GEN", ordinary_names(outer))
        original = [(row.generator_yielded_consumer,
                     row.generator_completed_consumer, row.generator_resume_consumer)
                    for _, _, row in sites(outer)]
        monitor, tool = sys.monitoring, 2
        monitor.use_tool_id(tool, "generator boundary instruction hooks")
        try:
            for event in (monitor.events.LINE, monitor.events.INSTRUCTION):
                seen = []
                monitor.register_callback(tool, event,
                                          lambda code, offset: seen.append(offset))
                iterator = outer(value)
                self.assertIs(next(iterator), value)
                try:
                    # The child has already armed its parent continuation.
                    monitor.set_local_events(tool, outer.__code__, event)
                    self.assertIs(finish(iterator, reply), reply)
                    self.assertTrue(seen)
                finally:
                    monitor.set_local_events(tool, outer.__code__, 0)
                    monitor.register_callback(tool, event, None)
                    iterator.close()
            for _ in range(ROUNDS):
                iterator = outer(value)
                self.assertIs(next(iterator), value)
                self.assertIs(finish(iterator, reply), reply)
            self.assertIn("SEND_GEN", ordinary_names(outer))
            self.assertEqual(
                [(row.generator_yielded_consumer, row.generator_completed_consumer,
                  row.generator_resume_consumer) for _, _, row in sites(outer)], original)
        finally:
            monitor.set_local_events(tool, outer.__code__, 0)
            monitor.register_callback(tool, monitor.events.LINE, None)
            monitor.register_callback(tool, monitor.events.INSTRUCTION, None)
            monitor.free_tool_id(tool)

    def test_post_completion_redirect_uses_the_committed_landing(self):
        ordinary = []

        def outer(value):
            result = (
                yield from leaf(value)
            )
            ordinary.append("ordinary continuation")
            return "redirected"

        self.assert_completed_shape(outer, _opcode.GEN_CONSUMER_STORE_FAST)
        monitor, tool = sys.monitoring, 2
        event = monitor.events.LINE
        monitor.use_tool_id(tool, "generator completion redirect")
        commits = []

        def callback(code, line):
            self.assertIs(code, outer.__code__)
            if line != code.co_firstlineno + 1:
                return
            frame = sys._getframe(1)
            self.assertIs(frame.f_code, code)
            with self.assertWarnsRegex(
                    RuntimeWarning, "assigning None to 1 unbound local"):
                frame.f_lineno = code.co_firstlineno + 5
            commits.append(frame.f_lineno)

        iterator = outer(object())
        next(iterator)
        monitor.register_callback(tool, event, callback)
        try:
            monitor.set_local_events(tool, outer.__code__, event)
            reply = object()
            self.assertEqual(finish(iterator, reply), "redirected")
            self.assertEqual(commits, [outer.__code__.co_firstlineno + 5])
            self.assertEqual(ordinary, [])
        finally:
            monitor.set_local_events(tool, outer.__code__, 0)
            monitor.register_callback(tool, event, None)
            monitor.free_tool_id(tool)
            iterator.close()

    def test_pre_and_post_completion_errors_keep_the_observed_location(self):
        def outer(value):
            return (
                yield from leaf(value)
            )

        self.assert_completed_shape(outer, _opcode.GEN_CONSUMER_RETURN)
        source, word = next((pc, word) for pc, word, row in sites(outer)
                            if row.generator_yielded_consumer ==
                            _opcode.GEN_CONSUMER_DELEGATED)
        completed = source + 1 + ((word[4] ^ 2048) - 2048)
        monitor, tool = sys.monitoring, 2
        monitor.use_tool_id(tool, "generator completion observer errors")
        try:
            for event in (monitor.events.STOP_ITERATION, monitor.events.LINE):
                iterator = outer(object())
                next(iterator)
                seen = []

                def callback(code, position, *args):
                    self.assertIs(code, outer.__code__)
                    if event == monitor.events.LINE:
                        if position != code.co_firstlineno + 1:
                            return
                        self.assertEqual(sys._getframe(1).f_lasti, completed)
                    else:
                        self.assertEqual(position, source)
                        self.assertEqual(sys._getframe(1).f_lasti, source)
                    seen.append(position)
                    raise LookupError("completion observer")

                monitor.register_callback(tool, event, callback)
                try:
                    monitor.set_local_events(tool, outer.__code__, event)
                    with self.assertRaisesRegex(LookupError, "completion observer"):
                        iterator.send(object())
                    self.assertEqual(seen, [outer.__code__.co_firstlineno + 1
                                           if event == monitor.events.LINE else source])
                    self.assertIsNone(iterator.gi_frame)
                finally:
                    monitor.set_local_events(tool, outer.__code__, 0)
                    monitor.register_callback(tool, event, None)
                    iterator.close()
        finally:
            monitor.set_local_events(tool, outer.__code__, 0)
            monitor.free_tool_id(tool)

    def test_same_pc_line_redirect_after_completion(self):
        def outer(value):
            result = yield from leaf(value)
            copied = result
            return copied

        self.assert_completed_shape(outer, _opcode.GEN_CONSUMER_STORE_FAST)
        target = outer.__code__.co_firstlineno + 2
        replacement, commits = object(), []

        def trace(frame, event, arg):
            if (frame.f_code is outer.__code__ and event == "line" and
                frame.f_lineno == target and not commits):
                frame.f_locals["result"] = replacement
                before = frame.f_lasti
                frame.f_lineno = target
                commits.append((before, frame.f_lasti))
            return trace

        iterator = outer(object())
        next(iterator)
        previous = sys.gettrace()
        try:
            sys.settrace(trace)
            self.assertIs(finish(iterator, object()), replacement)
        finally:
            sys.settrace(previous)
            iterator.close()
        self.assertEqual(len(commits), 1)
        self.assertEqual(commits[0][0], commits[0][1])

    def test_fresh_result_survives_retirement_gc(self):
        observations, references = [], []

        class Result:
            def __init__(self):
                self.cycle = self
                references.append(weakref.ref(self))

        class Cleanup:
            def __del__(self):
                gc.collect()
                observations.append(references[0]() is not None)

        def child():
            cleanup = Cleanup()
            yield None
            # No child local retains the fresh returned owner at cleanup.
            return Result()

        def outer():
            return (yield from child())

        self.assert_completed_shape(outer, _opcode.GEN_CONSUMER_RETURN)
        iterator = outer()
        self.assertIsNone(next(iterator))
        result = finish(iterator, None)
        self.assertEqual(observations, [True])
        self.assertIs(references[0](), result)
        del result
        gc.collect()
        self.assertIsNone(references[0]())


if __name__ == "__main__":
    unittest.main()
