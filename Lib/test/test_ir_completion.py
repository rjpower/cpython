"""Semantic iterator completion and explicit generator resume recipients."""

import dis
import gc
import struct
import sys
import unittest

from test import test_ir
from test.support import import_helper
from test.support.ir_semantics import (semantic_table, semantic_parts, position_parts,
                                       observation_tables, observation_records)

_testinternalcapi = import_helper.import_module('_testinternalcapi')


def code_names(func):
    return [instruction.opname for instruction in dis.get_instructions(func)]


def exception_item(value):
    parts = [value & 63]
    while value >> 6:
        value >>= 6
        parts.append((value & 63) | 64)
    return bytes(reversed(parts))


def exception_entry(*values):
    data = bytearray(b''.join(exception_item(value) for value in values))
    data[0] |= 128
    return semantic_table(bytes(data))


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
        words = struct.unpack(f'<{len(outer.__code__.co_code) // 8}Q', outer.__code__.co_code)
        for instruction in throws:
            self.assertIn(instruction.jump_target, offsets)
            self.assertEqual(instruction.jump_target, words[instruction.offset] >> 40)
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
        self.assertNotIn('POP_TOP', code_names(gen))
        recipients = [i.dst for i in dis.get_instructions(gen)
                      if i.opname in ('RETURN_GENERATOR', 'YIELD_VALUE')]
        self.assertEqual(recipients, [None, None])

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
                site = next(i.offset for i in dis.get_instructions(consume)
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
                site = next(i.offset for i in dis.get_instructions(consume)
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

    def test_constructor_rejects_window_and_handler_anchor_mismatch(self):
        def consume(values):
            for value in values:
                pass
        code = consume.__code__
        words = list(struct.unpack(f'<{len(code.co_code) // 8}Q', code.co_code))
        site = next(i.offset for i in dis.get_instructions(consume)
                    if i.opname == 'FOR_ITER')
        base = (words[site] >> 28) & 4095
        count = len(_testinternalcapi.get_co_localskinds(code)) + code.co_stacksize
        words[site] = (words[site] & ~(4095 << 28)) | ((count - 1) << 28)
        with self.assertRaisesRegex(ValueError, 'invalid register window'):
            code.replace(co_code=struct.pack(f'<{len(words)}Q', *words),
                         co_linetable=b'', co_exceptiontable=code.co_exceptiontable)
        table = exception_entry(site, 1, len(words) - 1, (base + 1) << 1)
        with self.assertRaisesRegex(ValueError, 'semantic completion'):
            code.replace(co_exceptiontable=table, co_linetable=b'')

        # A preserved prefix cannot grant an explicit read of the old iterator
        # after completion, even when its physical home still exists.
        words = list(struct.unpack(f'<{len(code.co_code) // 8}Q', code.co_code))
        words[-1] = dis.opmap['POP_TOP'] | (base << 16)
        with self.assertRaisesRegex(ValueError, 'semantic completion'):
            code.replace(co_code=struct.pack(f'<{len(words)}Q', *words),
                         co_exceptiontable=table, co_linetable=b'')

        def delegate():
            yield from ()

        code = delegate.__code__
        words = list(struct.unpack(f'<{len(code.co_code) // 8}Q', code.co_code))
        site = next(i.offset for i in dis.get_instructions(delegate)
                    if i.opname == 'SEND')
        base = (words[site] >> 28) & 4095
        words[site] = (words[site] & ~(4095 << 16)) | (base << 16)
        words[-1] = dis.opmap['POP_TOP'] | (base << 16)
        table = exception_entry(site, 1, len(words) - 1, (base + 1) << 1)
        # Publishing a new completed result into B does not reconstruct the
        # old receiver that this forged handler attempts to consume.
        with self.assertRaisesRegex(ValueError, 'semantic completion'):
            code.replace(co_code=struct.pack(f'<{len(words)}Q', *words),
                         co_exceptiontable=table, co_linetable=b'')

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
                site = next(i.offset for i in dis.get_instructions(code)
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

    def test_constructor_completion_proof_validates_local_roles_and_eh_structure(self):
        iterator = (value for value in ())
        self.addCleanup(iterator.close)
        code = iterator.gi_code
        code.replace()
        instructions = list(dis.get_instructions(code))
        site = next(i.offset for i in instructions if i.opname == 'FOR_ITER')
        entry = next(e for e in dis.Bytecode(code).exception_entries
                     if e.start <= site < e.end)
        words = list(struct.unpack(f'<{len(code.co_code) // 8}Q', code.co_code))
        base = (words[site] >> 28) & 4095
        words[entry.target] = dis.opmap['LOAD_FAST'] | (base << 28)
        with self.assertRaisesRegex(ValueError, 'names local slot'):
            code.replace(co_code=struct.pack(f'<{len(words)}Q', *words),
                         co_linetable=b'', co_exceptiontable=code.co_exceptiontable)
        table, _ = semantic_parts(code.co_exceptiontable)
        first, _ = semantic_parts(exception_entry(site, 1, entry.target, entry.slot << 1))
        second, _ = semantic_parts(exception_entry(0, 1, entry.target, entry.slot << 1))
        malformed = (semantic_table(bytes([table[0] & 127]) + table[1:]),
                     semantic_table(table + table), semantic_table(first + second))
        for data in malformed:
            with self.subTest(table=data):
                with self.assertRaisesRegex(ValueError, 'semantic recovery'):
                    code.replace(co_exceptiontable=data)

    def test_constructor_rejects_preserved_receiver_as_nullable_call_self(self):
        def delegate():
            for values in ((),):
                yield from values

        code = delegate.__code__
        original = list(struct.unpack(f'<{len(code.co_code) // 8}Q', code.co_code))
        site = next(i.offset for i in dis.get_instructions(delegate)
                    if i.opname == 'SEND')
        base = (original[site] >> 28) & 4095
        self.assertGreater(base, len(_testinternalcapi.get_co_localskinds(code)))
        table = exception_entry(site, 1, len(original) - 1, (base + 2) << 1)
        handlers = ('CALL', 'CALL_KW', 'CALL_PY_EXACT_ARGS', 'INSTRUMENTED_CALL',
                    'CALL_FUNCTION_EX', 'CALL_EX_PY', 'INSTRUMENTED_CALL_FUNCTION_EX')
        for handler, destination in ((h, d) for h in handlers for d in (4095, base)):
            with self.subTest(handler=handler, destination=destination):
                words = original.copy()
                words[site] = (words[site] & ~(4095 << 16)) | (destination << 16)
                words[-1] = dis._all_opmap[handler] | (4095 << 16) | ((base - 1) << 28)
                # B is preserved by PRE's handler and is neither an optional
                # NULL self nor reconstructed by POST's completed result.
                with self.assertRaisesRegex(ValueError, 'may_nonnull='):
                    code.replace(co_code=struct.pack(f'<{len(words)}Q', *words),
                                 co_exceptiontable=table, co_linetable=b'')

    def test_call_ex_callable_and_argument_tuple_remain_required(self):
        def delegate(values):
            yield from values

        code = delegate.__code__
        original = list(struct.unpack(f'<{len(code.co_code) // 8}Q', code.co_code))
        site = next(i.offset for i in dis.get_instructions(delegate)
                    if i.opname == 'CLEANUP_THROW')
        base = (original[site] >> 28) & 4095
        table = exception_entry(site, 1, len(original) - 1, base << 1)
        for handler in ('CALL_FUNCTION_EX', 'CALL_EX_PY', 'INSTRUMENTED_CALL_FUNCTION_EX'):
            for window in (base, base + 1):
                with self.subTest(handler=handler, window=window):
                    words = original.copy()
                    words[-1] = dis._all_opmap[handler] | (4095 << 16) | (window << 28)
                    with self.assertRaisesRegex(ValueError, 'may_nonnull=0'):
                        code.replace(co_code=struct.pack(f'<{len(words)}Q', *words),
                                     co_exceptiontable=table, co_linetable=b'',
                                     co_stacksize=max(code.co_stacksize, base + 5 -
                                                      len(_testinternalcapi.get_co_localskinds(code))))

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
        for change in ('local', 'global', 'restart'):
            for source_lines, trigger, expected in (((5,), 5, [1, 5, 5]),
                                                     ((3, 5), 3, [1, 3, 5])):
                with self.subTest(change=change, source_lines=source_lines):
                    def consume():
                        for value in ():
                            pass
                        pass
                        pass
                        return 'complete'

                    code = consume.__code__
                    site = next(i for i in dis.get_instructions(consume)
                                if i.opname == 'FOR_ITER')
                    raw = code.co_linetable
                    version, size, _ = struct.unpack_from('<III', raw, 4)
                    locations = raw[16:16 + size]
                    # Deliberately specify the source-only edge independently
                    # of which empty statements the compiler coalesces.
                    records = [(site.offset, site.jump_target, 1,
                                code.co_firstlineno + line, code.co_firstlineno + line,
                                0, 1, -1, 0, 0) for line in source_lines]
                    exceptions, _ = semantic_parts(code.co_exceptiontable)
                    consume.__code__ = code.replace(
                        **observation_tables(locations, records, exceptions))
                    lines = []
                    changed = []

                    def callback(observed, line):
                        if observed is not consume.__code__:
                            return
                        relative = line - observed.co_firstlineno
                        lines.append(relative)
                        if relative == trigger and not changed:
                            self.assertEqual(sys._getframe(1).f_lasti, site.offset)
                            changed.append(True)
                            if change == 'local':
                                sys.monitoring.set_local_events(tool, observed, 0)
                                sys.monitoring.set_local_events(tool, observed, line_event)
                            elif change == 'global':
                                sys.monitoring.set_events(tool, 0)
                                sys.monitoring.set_events(tool, line_event)
                            else:
                                sys.monitoring.restart_events()

                    sys.monitoring.register_callback(tool, line_event, callback)
                    try:
                        if change == 'global':
                            sys.monitoring.set_events(tool, line_event)
                        else:
                            sys.monitoring.set_local_events(tool, consume.__code__, line_event)
                        self.assertEqual(consume(), 'complete')
                        self.assertEqual(changed, [True])
                        self.assertEqual(lines, expected)
                    finally:
                        sys.monitoring.set_events(tool, 0)
                        sys.monitoring.set_local_events(tool, consume.__code__, 0)

    def test_completion_handler_skips_only_untouched_window_holes(self):
        def consume(values):
            for outer in values:
                for middle in outer:
                    for value in middle:
                        pass

        code = consume.__code__
        original = list(struct.unpack(f'<{len(code.co_code) // 8}Q', code.co_code))
        site = [i.offset for i in dis.get_instructions(consume)
                if i.opname == 'FOR_ITER'][-1]
        base = (original[site] >> 28) & 4095
        self.assertGreaterEqual(base, len(_testinternalcapi.get_co_localskinds(code)) + 4)
        exception = base - 1
        table = exception_entry(site, 1, len(original), exception << 1)
        # The retired inputs and not-yet-produced item are untouched middle
        # positions; ordinary PRE unwind may leave all three unavailable.
        # These are constructor proofs only: the synthetic handlers are not run.
        shapes = {
            'LIST_APPEND': (base - 1, 0, 4, 0),
            'SET_ADD': (base - 1, 0, 4, 0),
            'MAP_ADD': (base - 1, 0, 0, 4),
            'DICT_MERGE': (base - 4, 5, 0, 0),
            'DICT_UPDATE': (base - 1, 4, 0, 0),
            'LIST_EXTEND': (base - 1, 4, 0, 0),
            'SET_UPDATE': (base - 1, 4, 0, 0),
            'COPY': (base - 1, 4, 0, 0),
            'SWAP': (base - 1, 5, 0, 0),
        }
        for name, fields in shapes.items():
            with self.subTest(opcode=name):
                def replace(fields):
                    words = original.copy()
                    if name == 'SWAP':
                        # Unwind clears the other endpoint beyond the retired
                        # window. Give SWAP a real value there first; the
                        # required-source negative below still reads base.
                        words.append(dis.opmap['LOAD_CONST'] |
                                     ((base + 3) << 16))
                    handler = dis._all_opmap[name] | sum(
                        value << shift for value, shift in zip(fields, (16, 28, 40, 52)))
                    words.extend((handler, dis.opmap['RERAISE'] | (exception << 16)))
                    return code.replace(
                        co_code=struct.pack(f'<{len(words)}Q', *words),
                        co_exceptiontable=table, co_linetable=b'',
                        co_stacksize=max(code.co_stacksize, base + 8))
                replace(fields)
                # Moving the real collection/source onto a retired home must
                # still fail; hole suppression is neither a definition nor NULL.
                required = (base, *fields[1:])
                with self.assertRaisesRegex(ValueError, 'semantic completion'):
                    replace(required)
                if name in ('LIST_APPEND', 'SET_ADD', 'MAP_ADD'):
                    required = (fields[0], base, *fields[2:])
                    with self.assertRaisesRegex(ValueError, 'semantic completion'):
                        replace(required)

    def test_completion_handler_attribute_deletion_uses_actual_handler_capability(self):
        def consume(values):
            for outer in values:
                for value in outer:
                    pass

        code = consume.__code__
        words = list(struct.unpack(f'<{len(code.co_code) // 8}Q', code.co_code))
        site = [i.offset for i in dis.get_instructions(consume)
                if i.opname == 'FOR_ITER'][-1]
        base = (words[site] >> 28) & 4095
        self.assertGreaterEqual(base, len(_testinternalcapi.get_co_localskinds(code)) + 2)
        exception = base - 1
        table = exception_entry(site, 1, len(words), exception << 1)
        for name in ('STORE_ATTR', 'STORE_ATTR_INSTANCE_VALUE'):
            with self.subTest(handler=name):
                handler = words + [dis._all_opmap[name] | (base << 28),
                                   dis.opmap['RERAISE'] | (exception << 16)]
                def replace():
                    return code.replace(co_code=struct.pack(f'<{len(handler)}Q', *handler),
                                        co_exceptiontable=table, co_linetable=b'',
                                        co_names=('attribute',))
                if name == 'STORE_ATTR_INSTANCE_VALUE':
                    with self.assertRaisesRegex(ValueError, 'semantic completion'):
                        replace()
                else:
                    replace()

    def test_nullable_deletion_value_cannot_alias_required_receiver(self):
        def consume(values):
            for outer in values:
                for value in outer:
                    pass

        code = consume.__code__
        words = list(struct.unpack(f'<{len(code.co_code) // 8}Q', code.co_code))
        site = [i.offset for i in dis.get_instructions(consume)
                if i.opname == 'FOR_ITER'][-1]
        base = (words[site] >> 28) & 4095
        exception = base - 1
        table = exception_entry(site, 1, len(words), exception << 1)
        words.extend((dis.opmap['STORE_ATTR'] | (base << 16) | (base << 28),
                      dis.opmap['RERAISE'] | (exception << 16)))
        with self.assertRaisesRegex(ValueError, 'semantic completion'):
            code.replace(co_code=struct.pack(f'<{len(words)}Q', *words),
                         co_exceptiontable=table, co_linetable=b'',
                         co_names=('attribute',))

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

    def test_ignored_window_dst_does_not_define_a_retired_owner(self):
        def delegate(values):
            yield from values

        code = delegate.__code__
        words = list(struct.unpack(f'<{len(code.co_code) // 8}Q', code.co_code))
        site = next(i.offset for i in dis.get_instructions(delegate)
                    if i.opname == 'CLEANUP_THROW')
        base = (words[site] >> 28) & 4095
        handler = len(words)
        exception = base + 4
        words.extend((dis.opmap['PUSH_EXC_INFO'] | (base << 16) | (exception << 28),
                      dis.opmap['POP_TOP'] | (base << 16),
                      dis.opmap['RERAISE'] | ((exception + 1) << 16)))
        table = exception_entry(site, 1, handler, exception << 1)
        # PUSH_EXC_INFO publishes at its implicit B window; its ignored A
        # field cannot recreate the old receiver that POP_TOP attempts to read.
        with self.assertRaisesRegex(ValueError, f'pc={handler + 1} '):
            code.replace(co_code=struct.pack(f'<{len(words)}Q', *words),
                         co_exceptiontable=table, co_linetable=b'',
                         co_stacksize=max(code.co_stacksize, exception + 2 -
                                          len(_testinternalcapi.get_co_localskinds(code))))

    def test_constructor_completion_proof_follows_source_callback_errors(self):
        iterator = (value for value in ())
        self.addCleanup(iterator.close)
        code = iterator.gi_code
        instructions = list(dis.get_instructions(code))
        site = next(i.offset for i in instructions if i.opname == 'FOR_ITER')
        entry = next(e for e in dis.Bytecode(code).exception_entries
                     if e.start <= site < e.end)
        words = list(struct.unpack(f'<{len(code.co_code) // 8}Q', code.co_code))
        base = (words[site] >> 28) & 4095
        new_handler = len(words)
        words.append(dis.opmap['POP_TOP'] | (base << 16))
        raw = code.co_linetable
        version, locations_size, _ = struct.unpack_from('<III', raw, 4)
        locations = raw[16:16 + locations_size] + b'\xf8'
        event = (entry.target, entry.target + 1, 0,
                 code.co_firstlineno, code.co_firstlineno, 0, 1,
                 new_handler, entry.slot, int(entry.lasti))
        exceptions, _ = semantic_parts(code.co_exceptiontable)
        maps = observation_tables(locations, [event], exceptions)
        with self.assertRaisesRegex(ValueError, 'semantic completion'):
            code.replace(co_code=struct.pack(f'<{len(words)}Q', *words),
                         **maps)

    def test_constructor_terminal_observer_owners(self):
        def iterate(values):
            for value in values:
                pass
            return None

        def choose(left, right):
            return (left + 1) or right

        def source_handler(code, site_name, edge, dst, read, nullable,
                           ordinary=False):
            words = list(struct.unpack(f'<{len(code.co_code) // 8}Q',
                                       code.co_code))
            # This fixture changes control/output geometry. Re-admit general
            # returns rather than retaining the old image's empty certificate.
            general_return = {
                dis._all_opmap['RETURN_VALUE_EMPTY']: dis.opmap['RETURN_VALUE'],
                dis._all_opmap['INSTRUMENTED_RETURN_VALUE_EMPTY']:
                    dis.opmap['INSTRUMENTED_RETURN_VALUE'],
            }
            words = [(word & ~0xffff) | general_return.get(word & 0xffff, word & 0xffff)
                     for word in words]
            instruction = next(i for i in dis.get_instructions(code)
                               if i.opname == site_name)
            site = instruction.offset
            base = (words[site] >> 28) & 4095
            self.assertGreaterEqual(base, code.co_nlocals)
            if dst == 'result':
                dst = base + 1
            elif dst == 'window':
                dst = base + 2
            elif dst == 'discard':
                dst = 4095
            words[site] = (words[site] & ~(4095 << 16)) | (dst << 16)
            home = {'item': base + 2, 'cursor': base + 1,
                    'input': base, 'result': dst}[read]
            handler = len(words)
            if nullable:
                words.append(dis.opmap['STORE_GLOBAL'] | (home << 16))
                words.append(dis.opmap['RETURN_VALUE'])
            else:
                words.append(dis.opmap['RETURN_VALUE'] | (home << 16))
            locations, _ = position_parts(code.co_linetable)
            locations += bytes((0xf8 + len(words) - handler - 1,))
            records = observation_records(code)
            exceptions, _ = semantic_parts(code.co_exceptiontable)
            if ordinary:
                exceptions, _ = semantic_parts(exception_entry(
                    site, 1, handler, (base + 4) << 1))
            else:
                records.append((site,
                                instruction.jump_target if edge else site + 1,
                                edge, code.co_firstlineno, code.co_firstlineno,
                                0, 1, handler, base + 4, 0))
            maps = observation_tables(locations, records, exceptions)
            stacksize = max(code.co_stacksize, base + 5 - code.co_nlocals)
            return code.replace(co_code=struct.pack(f'<{len(words)}Q', *words),
                                co_names=code.co_names + ('terminal_sink',),
                                co_stacksize=stacksize, **maps)

        loop = iterate.__code__
        self.assertIsNotNone(source_handler(loop, 'FOR_ITER', 0,
                                            'window', 'item', False))
        # The same encoded item read is valid after an observer commit but
        # unavailable to an ordinary exception before FOR_ITER produces it.
        with self.assertRaisesRegex(ValueError, 'semantic completion'):
            source_handler(loop, 'FOR_ITER', 0, 'window', 'item', False,
                           ordinary=True)
        for dst, read, nullable in (('window', 'cursor', True),
                                    ('discard', 'item', False)):
            with self.subTest(dst=dst, read=read):
                with self.assertRaisesRegex(ValueError, 'semantic completion'):
                    source_handler(loop, 'FOR_ITER', 0, dst, read, nullable)
        self.assertIsNotNone(source_handler(loop, 'FOR_ITER', 0,
                                            'discard', 'item', True))

        branch = choose.__code__
        self.assertIsNotNone(source_handler(branch, 'JUMP_IF_TRUE_OR_POP', 0,
                                            'result', 'result', True))
        self.assertIsNotNone(source_handler(branch, 'JUMP_IF_TRUE_OR_POP', 1,
                                            'result', 'result', False))
        with self.assertRaisesRegex(ValueError, 'semantic completion'):
            source_handler(branch, 'JUMP_IF_TRUE_OR_POP', 0,
                           'result', 'input', True)

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

        site = next(i.offset for i in dis.get_instructions(consume)
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
                func = test_ir.relaxed_compile(source)[name]
                instructions = list(dis.get_instructions(func))
                words = struct.unpack(f'<{len(func.__code__.co_code) // 8}Q',
                                      func.__code__.co_code)
                helpers = {offset for offset, word in enumerate(words)
                           if word & 65535 == dis._ir_opmap['IR_JUMP']}
                self.assertTrue(helpers)
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
                self.assertLessEqual(helpers, set(observed))
                self.assertTrue(all(offset < len(instructions) for offset in observed))
                body, completed = sorted(helpers)
                self.assertEqual(observed.count(body), 2)
                self.assertEqual(observed.count(completed), 1)

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
                            real = {i.offset for i in dis.get_instructions(func)
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
