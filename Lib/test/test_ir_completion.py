"""Semantic iterator completion and explicit generator resume recipients."""

import dis as stack_dis
from test.support import ir_dis as dis
import gc
import struct
import sys
import unittest

from test.support import import_helper

_testinternalcapi = import_helper.import_module('_testinternalcapi')


def code_names(func):
    return [instruction.opname for instruction in dis.get_instructions(func)]


class SemanticCompletionTests(unittest.TestCase):
    def test_cleanup_throw_survives_optimization_as_named_semantic_jump(self):
        first, second = object(), object()

        class Delegate:
            def __init__(self, result):
                self.result = result

            def __iter__(self):
                return self

            def __next__(self):
                return 'ready'

            def throw(self, *args):
                raise StopIteration(self.result)

        def outer():
            left = yield from Delegate(first)
            right = yield from Delegate(second)
            return left, right

        instructions = list(dis.get_instructions(outer))
        throws = [i for i in instructions if i.opname == 'CLEANUP_THROW']
        self.assertEqual(len(throws), 2)
        offsets = {i.offset for i in instructions}
        ir_code = dis.IRCode(outer.__code__).co_code
        words = struct.unpack(f'<{len(ir_code) // 8}Q', ir_code)
        for instruction in throws:
            # The word names its target, a word of the code: the signed
            # distance of field D, from the word after it
            distance = words[instruction.offset] >> 52
            if distance >= 1 << 11:
                distance -= 1 << 12
            self.assertIn(instruction.offset + 1 + distance, offsets)
        self.assertNotIn('END_SEND', [i.opname for i in instructions])
        generator = outer()
        self.assertEqual(next(generator), 'ready')
        self.assertEqual(generator.throw(ValueError()), 'ready')
        with self.assertRaises(StopIteration) as stopped:
            generator.throw(ValueError())
        self.assertEqual(stopped.exception.value, (first, second))

    def test_loop_completion_preserves_last_local(self):
        def consume(iterator):
            item = 'empty'
            for item in iterator:
                pass
            return item

        def gen():
            yield 11
            yield 23
            return 91

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 2):
            self.assertEqual(consume(gen()), 23)
            self.assertEqual(consume([11, 23]), 23)
            self.assertEqual(consume(()), 'empty')
        self.assertNotIn('END_FOR', code_names(consume))
        self.assertNotIn('POP_ITER', code_names(consume))

    def test_send_and_throw_share_completed_recipient(self):
        def inner():
            try:
                yield 'ready'
            except LookupError:
                return 'caught'
            return 'normal'

        def outer():
            result = yield from inner()
            yield result

        for thrown, expected in ((False, 'normal'), (True, 'caught')):
            with self.subTest(thrown=thrown):
                iterator = outer()
                self.assertEqual(next(iterator), 'ready')
                actual = iterator.throw(LookupError()) if thrown else next(iterator)
                self.assertEqual(actual, expected)
                with self.assertRaises(StopIteration):
                    next(iterator)
        self.assertNotIn('END_SEND', code_names(outer))
        self.assertIn('CLEANUP_THROW', code_names(outer))

    def test_unused_resume_values_have_no_release_word(self):
        def gen():
            yield 'ready'
            return 'done'

        released = []
        class Resource:
            def __del__(self):
                released.append('released')

        iterator = gen()
        with self.assertRaisesRegex(TypeError, 'non-None'):
            iterator.send(Resource())
        self.assertEqual(next(iterator), 'ready')
        with self.assertRaises(StopIteration) as caught:
            iterator.send(Resource())
        self.assertEqual(caught.exception.value, 'done')
        iterator.close()
        gc.collect()
        self.assertEqual(released, ['released', 'released'])
        self.assertNotIn('Move.Retire', code_names(gen))
        recipients = [i.dst for i in dis.get_instructions(gen)
                      if i.opname == 'YIELD_VALUE']
        self.assertEqual(recipients, [None])

    def test_completion_events_disable_independently(self):
        events = sys.monitoring.events
        tool = sys.monitoring.PROFILER_ID
        sys.monitoring.use_tool_id(tool, 'completion')
        self.addCleanup(sys.monitoring.free_tool_id, tool)
        self.addCleanup(sys.monitoring.set_events, tool, 0)
        kinds = (events.BRANCH_LEFT, events.BRANCH_RIGHT, events.STOP_ITERATION)
        for disabled in kinds:
            with self.subTest(disabled=disabled):
                namespace = {}
                exec('def gen():\n yield 1\n yield 2\n return 7\n'
                     'def consume():\n for value in gen():\n  pass\n', namespace)
                consume = namespace['consume']
                # Event offsets are byte offsets into the stack bytecode
                site = next(i.offset for i in stack_dis.get_instructions(consume)
                            if i.opname == 'FOR_ITER')
                counts = dict.fromkeys(kinds, 0)
                for kind in kinds:
                    def callback(code, offset, *args, kind=kind):
                        if code is consume.__code__:
                            self.assertEqual(offset, site)
                            counts[kind] += 1
                            if kind == disabled:
                                return sys.monitoring.DISABLE
                    sys.monitoring.register_callback(tool, kind, callback)
                    self.addCleanup(sys.monitoring.register_callback, tool, kind, None)
                sys.monitoring.set_local_events(tool, consume.__code__, sum(kinds))
                try:
                    for _ in range(3):
                        consume()
                    expected = {events.BRANCH_LEFT: 6,
                                events.BRANCH_RIGHT: 3, events.STOP_ITERATION: 3}
                    expected[disabled] = 1
                    self.assertEqual(counts, expected)
                    sys.monitoring.restart_events()
                    consume()
                    self.assertEqual(counts[disabled], 2)
                    for kind in kinds:
                        if kind != disabled:
                            self.assertEqual(counts[kind], expected[kind] +
                                             (2 if kind == events.BRANCH_LEFT else 1))
                finally:
                    sys.monitoring.set_local_events(tool, consume.__code__, 0)

    def test_pre_and_post_callback_errors_use_semantic_handler(self):
        events = sys.monitoring.events
        tool = sys.monitoring.PROFILER_ID
        sys.monitoring.use_tool_id(tool, 'completion-errors')
        self.addCleanup(sys.monitoring.free_tool_id, tool)
        self.addCleanup(sys.monitoring.set_events, tool, 0)
        for event in (events.STOP_ITERATION, events.BRANCH_RIGHT):
            with self.subTest(event=event):
                def gen():
                    yield 1
                    return 9
                def consume():
                    try:
                        for value in gen():
                            pass
                    except LookupError as error:
                        return error.__traceback__
                # Event offsets are byte offsets into the stack bytecode
                site = next(i.offset for i in stack_dis.get_instructions(consume)
                            if i.opname == 'FOR_ITER')
                def callback(code, offset, *args):
                    if code is consume.__code__:
                        self.assertEqual(offset, site)
                        raise LookupError('observer')
                sys.monitoring.register_callback(tool, event, callback)
                self.addCleanup(sys.monitoring.register_callback, tool, event, None)
                sys.monitoring.set_local_events(tool, consume.__code__, event)
                try:
                    traceback = consume()
                    self.assertIsNotNone(traceback)
                    self.assertIs(traceback.tb_frame.f_code, consume.__code__)
                    self.assertEqual(traceback.tb_lasti, site)
                finally:
                    sys.monitoring.set_local_events(tool, consume.__code__, 0)

    def test_post_completion_preserves_generator_stopiteration_wrapper(self):
        tool = sys.monitoring.PROFILER_ID
        event = sys.monitoring.events.BRANCH_RIGHT
        sys.monitoring.use_tool_id(tool, 'completion-pep479')
        self.addCleanup(sys.monitoring.free_tool_id, tool)
        self.addCleanup(sys.monitoring.register_callback, tool, event, None)
        for error_type in (StopIteration, ValueError):
            with self.subTest(error_type=error_type):
                iterator = (value for value in ())
                code = iterator.gi_code
                site = next(i.offset for i in stack_dis.get_instructions(code)
                            if i.opname == 'FOR_ITER')

                def callback(observed, offset, *args):
                    if observed is code:
                        self.assertEqual(offset, site)
                        raise error_type('post completion')

                sys.monitoring.register_callback(tool, event, callback)
                sys.monitoring.set_local_events(tool, code, event)
                try:
                    expected = RuntimeError if error_type is StopIteration else ValueError
                    with self.assertRaises(expected) as caught:
                        next(iterator)
                    if error_type is StopIteration:
                        self.assertIsInstance(caught.exception.__cause__, StopIteration)
                finally:
                    sys.monitoring.set_local_events(tool, code, 0)
                    iterator.close()

    def test_completion_line_landing_survives_finalizer_observation_changes(self):
        # Exercise this VM's completion retirement path; Python does not
        # require every implementation to destroy this iterator immediately.
        tool = sys.monitoring.DEBUGGER_ID
        line_event = sys.monitoring.events.LINE
        sys.monitoring.use_tool_id(tool, 'completion-line-landing')
        self.addCleanup(sys.monitoring.free_tool_id, tool)
        self.addCleanup(sys.monitoring.register_callback, tool, line_event, None)
        for change in ('read', 'local', 'global', 'restart'):
            with self.subTest(change=change):
                lines = []
                finalized = []

                class Source:
                    def __iter__(self):
                        return self

                    def __next__(self):
                        raise StopIteration

                    def __del__(self):
                        frame = sys._getframe(1)
                        finalized.append((frame.f_code, frame.f_lineno))
                        if change == 'local':
                            sys.monitoring.set_local_events(tool, consume.__code__, 0)
                            sys.monitoring.set_local_events(tool, consume.__code__, line_event)
                        elif change == 'global':
                            sys.monitoring.set_events(tool, 0)
                            sys.monitoring.set_events(tool, line_event)
                        elif change == 'restart':
                            sys.monitoring.restart_events()
                        # This public lazy cache must not carry the handoff.
                        frame.f_lineno

                def consume():
                    for value in Source():
                        pass
                    return 'complete'

                def callback(code, line):
                    if code is consume.__code__:
                        lines.append(line - code.co_firstlineno)

                sys.monitoring.register_callback(tool, line_event, callback)
                try:
                    if change == 'global':
                        sys.monitoring.set_events(tool, line_event)
                    else:
                        sys.monitoring.set_local_events(tool, consume.__code__, line_event)
                    self.assertEqual(consume(), 'complete')
                    self.assertEqual([code for code, _ in finalized], [consume.__code__])
                    self.assertEqual(lines, [1, 3])
                finally:
                    sys.monitoring.set_events(tool, 0)
                    sys.monitoring.set_local_events(tool, consume.__code__, 0)

    def test_source_callback_configuration_epoch_precedes_observation(self):
        tool = sys.monitoring.DEBUGGER_ID
        line_event = sys.monitoring.events.LINE
        sys.monitoring.use_tool_id(tool, 'source-line-epoch')
        self.addCleanup(sys.monitoring.free_tool_id, tool)
        self.addCleanup(sys.monitoring.register_callback, tool, line_event, None)

        def make():
            def consume():
                for value in ():
                    pass
                pass
                pass
                return 'complete'
            return consume

        def run(consume, change, trigger):
            code = consume.__code__
            lines = []
            changed = []

            def callback(observed, line):
                if observed is not code:
                    return
                relative = line - observed.co_firstlineno
                lines.append(relative)
                if relative == trigger and not changed:
                    changed.append(True)
                    if change == 'local':
                        sys.monitoring.set_local_events(tool, observed, 0)
                        sys.monitoring.set_local_events(tool, observed, line_event)
                    elif change == 'global':
                        sys.monitoring.set_events(tool, 0)
                        sys.monitoring.set_events(tool, line_event)
                    elif change == 'restart':
                        sys.monitoring.restart_events()

            sys.monitoring.register_callback(tool, line_event, callback)
            try:
                if change == 'global':
                    sys.monitoring.set_events(tool, line_event)
                else:
                    sys.monitoring.set_local_events(tool, code, line_event)
                self.assertEqual(consume(), 'complete')
            finally:
                sys.monitoring.set_events(tool, 0)
                sys.monitoring.set_local_events(tool, code, 0)
            return lines, changed

        # Reconfiguring the observation from a callback, at any line, neither
        # repeats nor drops a line the unchanged run delivers
        expected, _ = run(make(), None, None)
        self.assertGreaterEqual(len(expected), 3)
        for change in ('local', 'global', 'restart'):
            for trigger in expected[1:]:
                with self.subTest(change=change, trigger=trigger):
                    lines, changed = run(make(), change, trigger)
                    self.assertEqual(changed, [True])
                    self.assertEqual(lines, expected)

    def test_handler_reuses_unwind_cleared_null_call_self(self):
        def delegate(factory):
            try:
                yield from ()
            except LookupError:
                pass
            return factory()

        marker = object()
        iterator = delegate(lambda: marker)
        with self.assertRaises(StopIteration) as stopped:
            next(iterator)
        self.assertIs(stopped.exception.value, marker)

    def test_handler_reuses_unwind_cleared_null_call_ex_kwargs(self):
        async def delegate(factory, args):
            try:
                await factory(*args)
            except LookupError:
                pass
            return factory(*args)

        marker = object()
        calls = []

        def factory(*args):
            calls.append(args)
            if len(calls) == 1:
                raise LookupError('first')
            return marker

        coroutine = delegate(factory, (1, 2))
        try:
            with self.assertRaises(StopIteration) as stopped:
                coroutine.send(None)
            self.assertIs(stopped.exception.value, marker)
            self.assertEqual(calls, [(1, 2), (1, 2)])
        finally:
            coroutine.close()

    def test_handler_reenters_send_with_unwind_cleared_cursor(self):
        marker = object()

        class Awaitable:
            def __await__(self):
                yield marker

        async def delegate():
            try:
                await Awaitable()
            except LookupError:
                pass
            await Awaitable()

        coroutine = delegate()
        try:
            self.assertIs(coroutine.send(None), marker)
            self.assertIs(coroutine.throw(LookupError()), marker)
            with self.assertRaises(StopIteration):
                coroutine.send(None)
        finally:
            coroutine.close()

    def test_post_completion_restores_inlined_comprehension_local(self):
        tool = sys.monitoring.PROFILER_ID
        event = sys.monitoring.events.BRANCH_RIGHT
        marker = object()
        observed = []

        def consume():
            value = marker
            try:
                [value for value in ()]
            except LookupError:
                observed.append(('except', value))
            finally:
                observed.append(('finally', value))
            return value

        site = next(i.offset for i in stack_dis.get_instructions(consume)
                    if i.opname == 'FOR_ITER')

        def callback(code, offset, *args):
            if code is consume.__code__:
                self.assertEqual(offset, site)
                raise LookupError('post completion')

        sys.monitoring.use_tool_id(tool, 'completion-restore')
        self.addCleanup(sys.monitoring.free_tool_id, tool)
        self.addCleanup(sys.monitoring.register_callback, tool, event, None)
        sys.monitoring.register_callback(tool, event, callback)
        sys.monitoring.set_local_events(tool, consume.__code__, event)
        try:
            self.assertIs(consume(), marker)
            self.assertEqual(observed, [('except', marker), ('finally', marker)])
        finally:
            sys.monitoring.set_local_events(tool, consume.__code__, 0)

    def test_relaxed_jumps_have_actual_instruction_events(self):
        for source, name, run in (
            ('def f(xs):\n n=0\n for x in xs:\n  n+=x\n return n\n',
             'f', lambda f: f([1, 2])),
            ('def f():\n yield from (1, 2)\n', 'f', lambda f: list(f())),
        ):
            with self.subTest(source=source):
                namespace = {}
                exec(source, namespace)
                func = namespace[name]
                # Lowering relaxes every loop's jumps through helper words
                # while the limit is zero
                old = _testinternalcapi.set_ir_max_forward(0)
                try:
                    rows = _testinternalcapi.get_ir(func.__code__)
                finally:
                    _testinternalcapi.set_ir_max_forward(old)
                ir = dis.IRCode(func.__code__)
                helpers = {word for word, row in enumerate(rows)
                           if row[0] == 'IR_JUMP'}
                self.assertTrue(helpers)
                valid = {i.offset for i in stack_dis.get_instructions(func)}
                observed = []
                def trace(frame, event, arg):
                    if frame.f_code is func.__code__:
                        frame.f_trace_opcodes = True
                        if event == 'opcode':
                            observed.append(frame.f_lasti)
                    return trace
                sys.settrace(trace)
                try:
                    run(func)
                finally:
                    sys.settrace(None)
                # Opcode events are per stack instruction, so a helper is
                # observed as the instruction it was lowered from
                self.assertLessEqual(set(observed), valid)
                self.assertLessEqual({2 * ir._pcmap[word] for word in helpers},
                                     set(observed))

    def test_mixed_line_instruction_ingress_and_suspension(self):
        events = sys.monitoring.events
        line_tool, instruction_tool = 2, 3
        sys.monitoring.use_tool_id(line_tool, 'completion-lines')
        self.addCleanup(sys.monitoring.free_tool_id, line_tool)
        sys.monitoring.use_tool_id(instruction_tool, 'completion-instructions')
        self.addCleanup(sys.monitoring.free_tool_id, instruction_tool)
        for tool in (line_tool, instruction_tool):
            self.addCleanup(sys.monitoring.set_events, tool, 0)
        for scope in ('local', 'global'):
            for order in ('line', 'instruction', 'both'):
                for delegated in (False, True):
                    with self.subTest(scope=scope, order=order, delegated=delegated):
                        namespace = {}
                        source = ('def inner():\n yield 1\n return 7\n'
                                  'def f():\n yield from inner()\n pass\n'
                                  if delegated else
                                  'def f():\n if True:\n  pass\n return 7\n')
                        exec(source, namespace)
                        func = namespace['f']
                        code = func.__code__
                        first = code.co_firstlineno
                        expected_lines = [first + 1, first + 2] if delegated else [first + 1, first + 2, first + 3]
                        lines, offsets = [], []
                        disabled_line = disabled_offset = None
                        def on_line(observed, line):
                            if observed is code:
                                lines.append(line)
                                if line == disabled_line:
                                    return sys.monitoring.DISABLE
                        def on_instruction(observed, offset):
                            if observed is code:
                                offsets.append(offset)
                                if offset == disabled_offset:
                                    return sys.monitoring.DISABLE
                        sys.monitoring.register_callback(line_tool, events.LINE, on_line)
                        sys.monitoring.register_callback(instruction_tool, events.INSTRUCTION, on_instruction)
                        def enable(tool, mask):
                            if scope == 'local':
                                sys.monitoring.set_local_events(tool, code, mask)
                            else:
                                sys.monitoring.set_events(tool, mask)
                        def run(observe_instructions=True):
                            if delegated:
                                iterator = func()
                                self.assertEqual(next(iterator), 1)
                                # Change observation while the SEND child and
                                # parent are suspended, then finish normally.
                                if observe_instructions:
                                    enable(instruction_tool, 0)
                                    enable(instruction_tool, events.INSTRUCTION)
                                with self.assertRaises(StopIteration):
                                    next(iterator)
                            else:
                                self.assertEqual(func(), 7)
                        try:
                            if order != 'instruction':
                                enable(line_tool, events.LINE)
                            if order != 'line':
                                enable(instruction_tool, events.INSTRUCTION)
                            if order != 'both':
                                run(order != 'line')
                                if order == 'line':
                                    self.assertEqual(offsets, [])
                                    self.assertEqual(lines, expected_lines)
                                else:
                                    self.assertEqual(lines, [])
                                    self.assertTrue(offsets)
                            lines.clear()
                            offsets.clear()
                            enable(line_tool, events.LINE)
                            enable(instruction_tool, events.INSTRUCTION)
                            run()
                            self.assertEqual(lines, expected_lines)
                            self.assertTrue(offsets)
                            real = {i.offset for i in stack_dis.get_instructions(func)
                                    if i.opname != 'RESUME'}
                            self.assertLessEqual(set(offsets), real)
                            disabled_line, disabled_offset = expected_lines[1], offsets[0]
                            lines.clear()
                            offsets.clear()
                            # Keep these masks stable across revisits: disabling
                            # one tool's site must not tear down the other's
                            # private ingress wrapper.
                            run(False)
                            run(False)
                            self.assertEqual(lines, expected_lines +
                                             [line for line in expected_lines
                                              if line != disabled_line])
                            self.assertEqual(offsets.count(disabled_offset), 1)
                            disabled_line = disabled_offset = None
                            sys.monitoring.restart_events()
                            lines.clear()
                            offsets.clear()
                            run()
                            self.assertEqual(lines, expected_lines)
                            self.assertTrue(offsets)
                        finally:
                            enable(line_tool, 0)
                            enable(instruction_tool, 0)
                            sys.monitoring.register_callback(line_tool, events.LINE, None)
                            sys.monitoring.register_callback(instruction_tool, events.INSTRUCTION, None)

    def test_instruction_tool_reuse_preserves_line_ingress(self):
        def func(flag):
            if flag:
                pass
            return 7
        events = sys.monitoring.events
        line_tool, instruction_tool = 2, 3
        sys.monitoring.use_tool_id(line_tool, 'persistent-lines')
        self.addCleanup(sys.monitoring.free_tool_id, line_tool)
        self.addCleanup(sys.monitoring.set_events, line_tool, 0)
        lines = []
        def on_line(code, line):
            if code is func.__code__:
                lines.append(line)
        sys.monitoring.register_callback(line_tool, events.LINE, on_line)
        self.addCleanup(sys.monitoring.register_callback, line_tool, events.LINE, None)
        sys.monitoring.set_local_events(line_tool, func.__code__, events.LINE)
        runs = []
        for _ in range(2):
            offsets = []
            sys.monitoring.use_tool_id(instruction_tool, 'reused-instructions')
            try:
                def on_instruction(code, offset):
                    if code is func.__code__:
                        offsets.append(offset)
                        return sys.monitoring.DISABLE
                sys.monitoring.register_callback(instruction_tool, events.INSTRUCTION, on_instruction)
                sys.monitoring.set_local_events(instruction_tool, func.__code__, events.INSTRUCTION)
                self.assertEqual(func(True), 7)
                self.assertEqual(func(True), 7)
                runs.append(offsets)
            finally:
                sys.monitoring.set_local_events(instruction_tool, func.__code__, 0)
                sys.monitoring.register_callback(instruction_tool, events.INSTRUCTION, None)
                sys.monitoring.free_tool_id(instruction_tool)
        self.assertTrue(runs[0])
        self.assertEqual(runs[0], runs[1])
        first = func.__code__.co_firstlineno
        self.assertEqual(lines, [first + 1, first + 2, first + 3] * 4)


if __name__ == '__main__':
    unittest.main()
