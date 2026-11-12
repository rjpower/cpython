"""Explicit delegation preserves protocols, owners and observation boundaries."""

from contextlib import contextmanager
import _opcode
import dis
import gc
import inspect
import marshal
import opcode
import sys
import textwrap
import unittest
import weakref

from test import support, test_ir
from test.support import import_helper
from test.support.script_helper import assert_python_ok

_testinternalcapi = import_helper.import_module('_testinternalcapi')
HANDLERS = {**opcode.opmap, **opcode._specialized_opmap, **opcode._ir_opmap}
WARM = _testinternalcapi.SPECIALIZATION_THRESHOLD + 5


def words(code):
    return [int.from_bytes(code.co_code[i:i + 8], sys.byteorder)
            for i in range(0, len(code.co_code), 8)]


def replace_words(code, changed, **kwargs):
    kwargs.setdefault('co_linetable', b'')
    return code.replace(co_code=b''.join(word.to_bytes(8, sys.byteorder)
                                        for word in changed), **kwargs)


def field(word, index, value):
    shift = 16 + 12 * index
    return (word & ~(4095 << shift)) | (value << shift)


def site_of(function, name='DELEGATE'):
    return next(i.offset for i in dis.get_instructions(function) if i.opname == name)


@contextmanager
def observing(code, callbacks):
    monitoring, tool = sys.monitoring, 4
    monitoring.use_tool_id(tool, 'explicit delegation')
    try:
        for event, callback in callbacks.items():
            monitoring.register_callback(tool, event, callback)
        monitoring.set_local_events(tool, code, sum(callbacks))
        yield tool
    finally:
        monitoring.set_local_events(tool, code, 0)
        for event in callbacks:
            monitoring.register_callback(tool, event, None)
        monitoring.free_tool_id(tool)


class DelegationTests(unittest.TestCase):
    def test_real_triple_and_independent_resume_fields(self):
        def outer(receiver):
            return (yield from receiver)

        code = outer.__code__
        instructions = list(dis.get_instructions(code))
        site = site_of(outer)
        triple = instructions[site:site + 3]
        self.assertEqual([i.opname for i in triple],
                         ['DELEGATE', 'SUSPEND', 'RESUME_DELEGATE'])
        encoded = words(code)
        base = (encoded[site] >> 28) & 4095
        self.assertEqual(triple[1].dst, base + 2)
        self.assertEqual(triple[1].srcs, (base + 2,))
        self.assertEqual(triple[2].arg & 7, 2)
        self.assertEqual(triple[2].jump_target, site)
        self.assertEqual(triple[2].argval, site)
        self.assertIn(site, dis.findlabels(code.co_code))
        self.assertEqual(len(code.co_code), len(instructions) * 8)
        self.assertEqual(len(list(code.co_positions())), len(instructions))
        self.assertEqual(marshal.loads(marshal.dumps(code)).co_code, code.co_code)
        code.replace()
        for name in ('DELEGATE', 'DELEGATE_GEN', 'SUSPEND', 'INSTRUMENTED_SUSPEND',
                     'RESUME_DELEGATE', 'INSTRUMENTED_RESUME_DELEGATE'):
            self.assertTrue(_opcode.is_valid(HANDLERS[name]))
            self.assertTrue(_opcode.has_arg(HANDLERS[name]))
            self.assertIn(HANDLERS[name], opcode.hasarg)
        for name in ('DELEGATE', 'DELEGATE_GEN', 'RESUME_DELEGATE',
                     'INSTRUMENTED_RESUME_DELEGATE'):
            self.assertTrue(_opcode.has_jump(HANDLERS[name]))
            self.assertIn(HANDLERS[name], opcode.hasjump)

    def test_deep_send_completion_and_discard(self):
        marker = object()

        def leaf():
            value = yield 'ready'
            yield value
            return marker

        def delegate(child):
            return (yield from child)

        def discard(child):
            yield from child
            return 'discarded'

        for depth in (1, 40):
            for discarded in (False, True):
                with self.subTest(depth=depth, discarded=discarded):
                    for _ in range(WARM):
                        child = leaf()
                        for _ in range(depth):
                            child = delegate(child)
                        if discarded:
                            child = discard(child)
                        self.assertEqual(next(child), 'ready')
                        self.assertIs(child.send(marker), marker)
                        with self.assertRaises(StopIteration) as stopped:
                            next(child)
                        if discarded:
                            self.assertEqual(stopped.exception.value, 'discarded')
                        else:
                            self.assertIs(stopped.exception.value, marker)

    def test_generic_protocol_send_throw_and_close(self):
        log, marker = [], object()

        class Provider:
            def __iter__(self):
                return self

            def __next__(self):
                log.append('next')
                return 'ready'

            def send(self, value):
                log.append(('send', value))
                return value

            def throw(self, error):
                log.append(('throw', error))
                raise StopIteration(marker)

            def close(self):
                log.append('close')

        def outer(receiver):
            return (yield from receiver)

        receiver = Provider()
        iterator = outer(receiver)
        self.assertEqual(next(iterator), 'ready')
        self.assertIs(iterator.gi_yieldfrom, receiver)
        self.assertIs(iterator.send(marker), marker)
        error = LookupError('injected')
        with self.assertRaises(StopIteration) as stopped:
            iterator.throw(error)
        self.assertIs(stopped.exception.value, marker)
        self.assertEqual(log, ['next', ('send', marker), ('throw', error)])
        iterator = outer(receiver)
        next(iterator)
        iterator.close()
        self.assertEqual(log[-1], 'close')

    def test_missing_and_raising_protocol_methods(self):
        def outer(receiver):
            yield from receiver

        class Missing:
            def __iter__(self):
                return self

            def __next__(self):
                return 1

        class Raising(Missing):
            def send(self, value):
                raise ArithmeticError('send failed')

            def throw(self, error):
                raise ArithmeticError('throw failed')

            def close(self):
                raise ArithmeticError('close failed')

        for receiver, action, error, message in (
            (Missing(), lambda g: g.send(7), AttributeError, 'send'),
            (Missing(), lambda g: g.throw(LookupError('missing')), LookupError, 'missing'),
            (Raising(), lambda g: g.send(7), ArithmeticError, 'send failed'),
            (Raising(), lambda g: g.throw(ValueError()), ArithmeticError, 'throw failed'),
            (Raising(), lambda g: g.close(), ArithmeticError, 'close failed'),
        ):
            with self.subTest(receiver=type(receiver), message=message):
                iterator = outer(receiver)
                next(iterator)
                with self.assertRaisesRegex(error, message):
                    action(iterator)
                self.assertIsNone(iterator.gi_frame)

    def test_provider_error_and_injected_exception_recipes(self):
        def outer(receiver):
            try:
                return (yield from receiver)
            except ArithmeticError as error:
                return str(error)

        class Provider:
            def __iter__(self):
                return self

            def __next__(self):
                return 'ready'

            def send(self, value):
                raise ArithmeticError('provider')

            def throw(self, error):
                raise StopIteration('cleanup')

        for injected, expected in ((False, 'provider'), (True, 'cleanup')):
            iterator = outer(Provider())
            next(iterator)
            with self.assertRaises(StopIteration) as stopped:
                iterator.throw(ValueError()) if injected else iterator.send(1)
            self.assertEqual(stopped.exception.value, expected)

    def test_generator_exit_and_reentry(self):
        def inner():
            try:
                yield 1
            except GeneratorExit:
                yield 2

        def outer(child):
            yield from child

        iterator = outer(inner())
        next(iterator)
        with self.assertRaisesRegex(RuntimeError, 'ignored GeneratorExit'):
            iterator.close()
        iterator.close()

        active = None

        class Reentrant:
            def __iter__(self):
                return self

            def __next__(self):
                return next(active)

        active = outer(Reentrant())
        with self.assertRaisesRegex(ValueError, 'already executing'):
            next(active)

    def test_await_and_async_generator_provider(self):
        class Awaitable:
            def __await__(self):
                incoming = yield 'awaiting'
                return incoming

        async def coroutine():
            return await Awaitable()

        async def agen():
            yield 13

        async def consume():
            return await agen().__anext__()

        for _ in range(WARM):
            iterator = coroutine()
            self.assertEqual(iterator.send(None), 'awaiting')
            self.assertIsNotNone(iterator.cr_await)
            with self.assertRaises(StopIteration) as stopped:
                iterator.send(17)
            self.assertEqual(stopped.exception.value, 17)
            self.assertIsNone(iterator.cr_await)
            with self.assertRaises(StopIteration) as stopped:
                consume().send(None)
            self.assertEqual(stopped.exception.value, 13)
        resume = list(dis.get_instructions(coroutine))[site_of(coroutine) + 2]
        self.assertEqual(resume.arg & 7, 3)

    def test_finally_and_exception_state(self):
        log = []

        def leaf():
            try:
                raise LookupError('handled')
            except LookupError as error:
                yield error
                self.assertIs(sys.exception(), error)

        def outer(child):
            try:
                yield from child
            finally:
                log.append('finally')

        iterator = outer(outer(leaf()))
        self.assertIsInstance(next(iterator), LookupError)
        self.assertIsNone(sys.exception())
        iterator.close()
        self.assertEqual(log, ['finally', 'finally'])
        self.assertIsNone(sys.exception())

    def test_retained_delegate_owner_and_collection(self):
        def leaf():
            yield 'ready'

        def outer(child):
            yield from child

        child = leaf()
        reference = weakref.ref(child)
        iterator = outer(child)
        del child
        next(iterator)
        self.assertIs(iterator.gi_yieldfrom, reference())
        gc.collect()
        self.assertIsNotNone(reference())
        iterator.close()
        gc.collect()
        self.assertIsNone(reference())

    def test_yield_observer_sees_coherent_frames_during_gc(self):
        def leaf():
            yield 'ready'

        def outer(child):
            yield from child

        child = leaf()
        iterator = outer(child)
        escaped = []

        def on_yield(code, offset, value):
            frame = sys._getframe(1)
            self.assertIs(frame.f_code, code)
            self.assertIs(frame.f_locals['child'], child)
            self.assertTrue(iterator.gi_running)
            self.assertFalse(child.gi_running)
            escaped.append(frame)
            gc.collect()
            self.assertEqual(value, 'ready')

        with observing(outer.__code__, {sys.monitoring.events.PY_YIELD: on_yield}):
            self.assertEqual(next(iterator), 'ready')
        self.assertIs(iterator.gi_frame, escaped[0])
        self.assertIs(iterator.gi_yieldfrom, child)
        iterator.close()
        self.assertIs(escaped[0].f_code, outer.__code__)

    def test_resume_input_finalizer_observes_coherent_chain(self):
        snapshots = []
        entered = []
        chain = []

        class Input:
            def __del__(self):
                snapshots.append((tuple(generator.gi_running for generator in chain),
                                  tuple(generator.gi_frame.f_code for generator in chain)))

        def leaf():
            running = tuple(generator.gi_running for generator in chain)
            frame = sys._getframe()
            callers = []
            while frame is not None:
                callers.append(frame.f_code)
                frame = frame.f_back
            entered.append((running, callers.count(outer.__code__)))
            yield 1
            yield 2

        def outer(child):
            yield from child

        chain.append(leaf())
        for _ in range(3):
            chain.append(outer(chain[-1]))
        self.assertEqual(next(chain[-1]), 1)
        self.assertEqual(entered, [((True,) * len(chain), len(chain) - 1)])
        self.assertEqual(chain[-1].send(Input()), 2)
        gc.collect()
        self.assertEqual(len(snapshots), 1)
        running, codes = snapshots[0]
        # The caller can retain the send argument until the yielded result
        # returns. Either retirement phase must expose a coherent whole chain.
        self.assertEqual(len(set(running)), 1)
        self.assertEqual(codes, (leaf.__code__,) + (outer.__code__,) * (len(chain) - 1))
        chain[-1].close()

    def test_pep523_after_specialization(self):
        def leaf():
            yield 1
            return 2

        def outer():
            return (yield from leaf())

        for _ in range(WARM):
            self.assertEqual(list(outer()), [1])
        calls = []
        _testinternalcapi.set_eval_frame_record(calls)
        try:
            iterator = outer()
            self.assertEqual(next(iterator), 1)
            with self.assertRaises(StopIteration) as stopped:
                next(iterator)
            self.assertEqual(stopped.exception.value, 2)
        finally:
            _testinternalcapi.set_eval_frame_default()
        self.assertTrue(calls)

    def test_monitoring_enabled_by_child_observes_parent_landing(self):
        events, seen = sys.monitoring.events, []
        tool = None

        def leaf():
            sys.monitoring.set_local_events(tool, outer.__code__,
                events.LINE | events.INSTRUCTION | events.PY_YIELD | events.PY_RESUME)
            yield 'ready'

        def outer():
            yield from leaf()
            return 'complete'

        callbacks = {
            event: (lambda code, offset, *args, event=event: seen.append((event, offset)))
            for event in (events.LINE, events.INSTRUCTION, events.PY_YIELD, events.PY_RESUME)
        }
        with observing(outer.__code__, callbacks) as tool:
            sys.monitoring.set_local_events(tool, outer.__code__, 0)
            iterator = outer()
            self.assertEqual(next(iterator), 'ready')
            suspend = site_of(outer, 'SUSPEND')
            self.assertIn((events.INSTRUCTION, suspend), seen)
            self.assertIn((events.PY_YIELD, suspend), seen)
            with self.assertRaises(StopIteration) as stopped:
                next(iterator)
            self.assertEqual(stopped.exception.value, 'complete')
            self.assertIn((events.PY_RESUME, suspend + 1), seen)
            self.assertIn((events.LINE, outer.__code__.co_firstlineno + 2), seen)

    def test_resume_and_yield_callback_errors_keep_error_recipe(self):
        events = sys.monitoring.events
        for event in (events.PY_YIELD, events.PY_RESUME):
            with self.subTest(event=event):
                def leaf():
                    yield 1
                    yield 2

                def outer():
                    try:
                        yield from leaf()
                    except ArithmeticError:
                        yield 'caught'

                def fail(code, offset, *args):
                    sys.monitoring.set_local_events(4, code, 0)
                    raise ArithmeticError('observer')

                iterator = outer()
                if event == events.PY_RESUME:
                    self.assertEqual(next(iterator), 1)
                with observing(outer.__code__, {event: fail}):
                    self.assertEqual(next(iterator), 'caught')
                iterator.close()

    def test_near_far_events_and_real_adapter(self):
        source = ('def inner():\n    yield 1\n    return 7\n'
                  'def outer():\n    return (yield from inner())\n')
        events = sys.monitoring.events
        for far in (False, True):
            namespace = test_ir.relaxed_compile(source) if far else {}
            if not far:
                exec(source, namespace)
            outer = namespace['outer']
            site = site_of(outer)
            instructions = list(dis.get_instructions(outer))
            physical = instructions[site].jump_target
            done = words(outer.__code__)[physical] >> 40 if far else physical
            if far:
                self.assertEqual(instructions[site + 3].handler, 'IR_JUMP')
                self.assertEqual(physical, site + 3)
            seen, branches, stopped = [], [], []
            callbacks = {
                events.INSTRUCTION: lambda code, offset: seen.append(offset),
                events.LINE: lambda code, line: None,
                events.BRANCH_LEFT: lambda code, src, dst: branches.append((src, dst)),
                events.STOP_ITERATION: lambda code, offset, error: stopped.append(error.value),
            }
            with observing(outer.__code__, callbacks):
                iterator = outer()
                self.assertEqual(next(iterator), 1)
                with self.assertRaises(StopIteration) as result:
                    next(iterator)
            self.assertEqual(result.exception.value, 7)
            self.assertEqual(stopped, [7])
            self.assertIn((site, done), branches)
            self.assertIn(site + 1, seen)
            self.assertIn(site + 2, seen)
            if far:
                self.assertIn(physical, seen)

    def test_constructor_rejects_malformed_local_contract(self):
        def outer(receiver):
            return (yield from receiver)

        code = outer.__code__
        original = words(code)
        site = site_of(outer)
        mutations = ((site, 1, 4095), (site + 1, 0, 4095),
                     (site + 1, 1, 4095), (site + 1, 2, 0),
                     (site + 2, 0, 0), (site + 2, 0, 16),
                     (site + 2, 1, 1), (site + 2, 2, site + 1))
        for pc, operand, value in mutations:
            with self.subTest(pc=pc, operand=operand, value=value):
                changed = original.copy()
                changed[pc] = field(changed[pc], operand, value)
                with self.assertRaisesRegex(ValueError, 'delegation|suspension'):
                    replace_words(code, changed)
        with self.assertRaisesRegex(ValueError, 'delegation|suspension'):
            code.replace(co_flags=code.co_flags & ~inspect.CO_GENERATOR)
        entry = site_of(outer, 'RETURN_GENERATOR')
        changed = original.copy()
        changed[entry] = (changed[entry] & ~65535) | HANDLERS['RESUME']
        with self.assertRaisesRegex(ValueError, 'delegation|suspension'):
            replace_words(code, changed)

    def test_constructor_rejects_middle_ingress_and_forged_far_adapter(self):
        def outer(receiver):
            yield from receiver
            yield 2

        code, site = outer.__code__, site_of(outer)
        for target in (site + 1, site + 2):
            changed = words(code)
            changed[-1] = HANDLERS['IR_JUMP'] | (target << 40)
            with self.subTest(target=target):
                with self.assertRaisesRegex(ValueError, 'delegation|suspension'):
                    replace_words(code, changed, co_linetable=b'')

        source = ('def outer(receiver):\n    return (yield from receiver)\n')
        outer = test_ir.relaxed_compile(source)['outer']
        code, site = outer.__code__, site_of(outer)
        original = words(code)
        self.assertEqual(original[site + 3] & 65535, HANDLERS['IR_JUMP'])
        for operand, value in ((0, 1), (1, 1), (2, site + 1)):
            changed = original.copy()
            changed[site + 3] = field(changed[site + 3], operand, value)
            with self.subTest(operand=operand):
                with self.assertRaisesRegex(ValueError, 'delegation|suspension'):
                    replace_words(code, changed)
        cleanup = next(entry.target for entry in dis.Bytecode(code).exception_entries
                       if entry.start <= site + 1 < entry.end)
        changed = original.copy()
        for pc in (site + 3, cleanup):
            changed[pc] = (changed[pc] & ((1 << 40) - 1)) | (site << 40)
        # Both completion routes agree, but neither may resume the retired
        # receiver window without a new receiver producer.
        with self.assertRaisesRegex(ValueError, 'delegation|suspension'):
            replace_words(code, changed)

    @unittest.skipUnless(support.Py_DEBUG, 'lltrace requires Py_DEBUG')
    def test_lltrace_new_handler_domain(self):
        script = textwrap.dedent('''
            def leaf():
                yield 3
                return 7
            def middle():
                return (yield from leaf())
            def outer():
                return (yield from middle())
            for _ in range(100):
                list(outer())
            __lltrace__ = 1
            iterator = outer()
            assert next(iterator) == 3
            try:
                next(iterator)
            except StopIteration as stopped:
                assert stopped.value == 7
            else:
                raise AssertionError('missing completion')
            del __lltrace__
            print('delegation complete')
        ''')
        _, stdout, stderr = assert_python_ok('-c', script)
        self.assertEqual(stderr, b'')
        for name in (b'DELEGATE', b'SUSPEND', b'RESUME_DELEGATE',
                     b'delegation complete'):
            self.assertIn(name, stdout)
        if test_ir.SPECIALIZES and _opcode.ENABLE_SPECIALIZATION:
            self.assertIn(b'DELEGATE_GEN', stdout)

    def test_native_consumers_of_plain_and_delegated_generators(self):
        script = textwrap.dedent('''
            def plain():
                incoming = yield 1
                yield incoming
                return 7
            def delegated():
                return (yield from plain())
            def values():
                yield 1
                yield 2
                yield 3
            def delegated_values():
                yield from values()
            marker = object()
            for factory in (plain, delegated):
                for _ in range(100):
                    iterator = factory()
                    assert next(iterator) == 1
                    assert iterator.send(9) == 9
                    assert next(iterator, marker) is marker
                    iterator = factory()
                    assert next(iterator) == 1
                    iterator.close()
                    assert next(iterator, marker) is marker
            for factory in (values, delegated_values):
                assert list(factory()) == [1, 2, 3]
                assert tuple(factory()) == (1, 2, 3)
                assert min(factory()) == 1
                assert sum(factory()) == 6
            assert min(i for i in range(3)) == 0
            print('native consumers complete')
        ''')
        _, stdout, stderr = assert_python_ok('-c', script)
        self.assertEqual(stderr, b'')
        self.assertEqual(stdout.strip(), b'native consumers complete')


if __name__ == '__main__':
    unittest.main()
