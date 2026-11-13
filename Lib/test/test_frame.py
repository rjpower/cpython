import copy
import operator
import re
import sys
import textwrap
import threading
import types
import unittest
import weakref
try:
    import _testcapi
except ImportError:
    _testcapi = None

from collections.abc import Mapping
from test import support
from test.support import import_helper, threading_helper
from test.support.script_helper import assert_python_ok
from test import mapping_tests


class HomeBitmapTest(unittest.TestCase):
    def test_word_endpoints_and_overlapping_frame_views(self):
        internal = import_helper.import_module('_testinternalcapi')
        payload = []
        before = sys.getrefcount(payload)
        internal.check_home_bitmap(payload)
        self.assertEqual(sys.getrefcount(payload), before)

    def test_waiting_parent_excludes_child_owned_argument_homes(self):
        internal = import_helper.import_module('_testinternalcapi')
        observations = []

        class Payload:
            pass

        def child(value):
            parent_frame = sys._getframe(1)
            own_ids = internal.get_frame_selected_object_ids(sys._getframe())
            parent_ids = internal.get_frame_selected_object_ids(parent_frame)
            observations.append((id(value) in own_ids, id(value) in parent_ids))
            return value

        def parent():
            return child(Payload())

        # Cover the checked generic entry and the reached exact overlap route.
        for _ in range(150):
            self.assertIsInstance(parent(), Payload)
        self.assertEqual(observations, [(True, False)] * 150)

    def test_pending_unpack_prefix_is_external_until_commit(self):
        internal = import_helper.import_module('_testinternalcapi')
        observations = []
        payload_ids = []

        class Payload:
            pass

        class Source:
            def __init__(self):
                self.index = 0
                self.frame = None

            def __iter__(self):
                return self

            def __next__(self):
                if payload_ids:
                    support.gc_collect()
                    external = internal.get_transition_root_ids()
                    homes = internal.get_frame_selected_object_ids(self.frame)
                    observations.append((all(value in external for value in payload_ids),
                                         all(value not in homes for value in payload_ids)))
                if self.index == 2:
                    raise StopIteration
                self.index += 1
                value = Payload()
                payload_ids.append(id(value))
                return value

        def unpack(source):
            left, right = source
            yield left, right

        source = Source()
        gen = unpack(source)
        source.frame = gen.gi_frame
        result = next(gen)
        self.assertEqual(observations, [(True, True), (True, True)])
        self.assertEqual([id(value) for value in result], payload_ids)
        homes = internal.get_frame_selected_object_ids(gen.gi_frame)
        self.assertTrue(all(value in homes for value in payload_ids))
        gen.close()

    def test_presence_traversal_keeps_immortal_and_no_count_referents(self):
        import gc
        internal = import_helper.import_module('_testinternalcapi')
        marker = object()

        def suspended(value):
            scalar = 17
            immortal = None
            yield value, scalar, immortal

        gen = suspended(marker)
        next(gen)
        frame = gen.gi_frame
        ids = internal.get_frame_selected_object_ids(frame)
        self.assertIn(id(marker), ids)
        self.assertIn(id(None), ids)
        referents = gc.get_referents(gen)
        self.assertTrue(any(item is marker for item in referents))
        self.assertTrue(any(item is None for item in referents))
        gen.close()

    @threading_helper.requires_working_threading()
    def test_foreign_thread_home_query_is_explicitly_unavailable(self):
        internal = import_helper.import_module('_testinternalcapi')
        ready = threading.Event()
        finish = threading.Event()
        frames = []

        def worker():
            named = object()
            frames.append(sys._getframe())
            ready.set()
            finish.wait()
            return named

        thread = threading.Thread(target=worker)
        thread.start()
        try:
            self.assertTrue(ready.wait(support.SHORT_TIMEOUT))
            frame = frames[0]
            self.assertIs(frame.f_code, worker.__code__)
            self.assertIn('named', frame.f_locals)
            self.assertEqual(internal.get_frame_home_view(frame)[0], 0)
            with self.assertRaisesRegex(ValueError, 'protected home view'):
                internal.get_frame_selected_object_ids(frame)
        finally:
            finish.set()
            thread.join(support.SHORT_TIMEOUT)
        self.assertFalse(thread.is_alive())

    def test_array_writers_during_callbacks(self):
        internal = import_helper.import_module('_testinternalcapi')
        observations = []

        class Target:
            def __setattr__(self, name, value):
                internal.check_frame_presence(sys._getframe(1))
                observations.append(name)
                object.__setattr__(self, name, value)

        def unpack(source, target):
            target.a, target.b, target.c = source
            yield target

        target = Target()
        payloads = [[], {}, set()]
        for source in (tuple(payloads), payloads):
            for _ in range(100):
                self.assertIs(next(unpack(source, target)), target)
        self.assertEqual(observations, ['a', 'b', 'c'] * 200)
        self.assertIs(target.c, payloads[2])

        class Key:
            def __hash__(self):
                caller = sys._getframe(1)
                if caller.f_code in (build.__code__, build_map.__code__):
                    internal.check_frame_presence(caller)
                observations.append('hash')
                return id(self)

        def build(a, b, c):
            yield {a, b, c}

        def build_map(a, b, c):
            yield {a: [], b: {}, c: set()}

        keys = Key(), Key(), Key()
        self.assertEqual(next(build(*keys)), set(keys))
        self.assertEqual(observations[-6:], ['hash'] * 6)

        self.assertEqual(set(next(build_map(*keys))), set(keys))
        self.assertEqual(observations[-6:], ['hash'] * 6)

    def test_string_builder_and_inplace_local_writers(self):
        internal = import_helper.import_module('_testinternalcapi')

        def build(a, b, c):
            value = f'{a}{b}{c}'
            internal.check_frame_presence(sys._getframe())
            return value

        def append(value, part):
            value += part
            internal.check_frame_presence(sys._getframe())
            return value

        for i in range(100):
            pieces = tuple(str(i + j) * 30 for j in range(3))
            self.assertEqual(build(*pieces), ''.join(pieces))
            self.assertEqual(append(pieces[0], pieces[1]),
                             pieces[0] + pieces[1])

    def test_special_lookup_writers_during_descriptor_calls(self):
        internal = import_helper.import_module('_testinternalcapi')
        observations = []

        class Method:
            def __init__(self, name):
                self.name = name

            def __get__(self, receiver, owner):
                internal.check_frame_presence(sys._getframe(1))
                observations.append(self.name)
                if self.name == 'enter':
                    return lambda: receiver
                return lambda *args: False

        class Manager:
            __enter__ = Method('enter')
            __exit__ = Method('exit')

        def use(manager):
            with manager as value:
                yield value

        manager = Manager()
        generator = use(manager)
        self.assertIs(next(generator), manager)
        generator.close()
        self.assertEqual(sorted(observations), ['enter', 'exit'])


class ClearTest(unittest.TestCase):
    """
    Tests for frame.clear().
    """

    def inner(self, x=5, **kwargs):
        1/0

    def outer(self, **kwargs):
        try:
            self.inner(**kwargs)
        except ZeroDivisionError as e:
            exc = e
        return exc

    def clear_traceback_frames(self, tb):
        """
        Clear all frames in a traceback.
        """
        while tb is not None:
            tb.tb_frame.clear()
            tb = tb.tb_next

    def test_clear_locals(self):
        class C:
            pass
        c = C()
        wr = weakref.ref(c)
        exc = self.outer(c=c)
        del c
        support.gc_collect()
        # A reference to c is held through the frames
        self.assertIsNot(None, wr())
        self.clear_traceback_frames(exc.__traceback__)
        support.gc_collect()
        # The reference was released by .clear()
        self.assertIs(None, wr())

    def test_clear_locals_after_f_locals_access(self):
        # see gh-113939
        class C:
            pass

        wr = None
        def inner():
            nonlocal wr
            c = C()
            wr = weakref.ref(c)
            1/0

        try:
            inner()
        except ZeroDivisionError as exc:
            support.gc_collect()
            self.assertIsNotNone(wr())
            exc.__traceback__.tb_next.tb_frame.clear()
            support.gc_collect()
            self.assertIsNone(wr())

    def test_clear_preserves_callback_local_writes(self):
        for nested in (False, True):
            with self.subTest(nested=nested):
                frames = []
                references = {}
                closed = []
                observed = []

                class Payload:
                    def __init__(self, name):
                        self.name = name

                    def __del__(self):
                        closed.append(self.name)

                def install(frame, name):
                    value = Payload(name)
                    references[name] = weakref.ref(value)
                    frame.f_locals[name] = value

                class Trigger:
                    def __del__(self):
                        frame = frames[0]
                        observed.append(frame.f_code is code)
                        if nested:
                            frame.clear()
                        install(frame, 'first')
                        install(frame, 'later')
                        support.gc_collect()

                def capture():
                    first = None
                    trigger = Trigger()
                    later = None
                    return sys._getframe()

                code = capture.__code__
                frame = capture()
                frames.append(frame)
                self.assertLess(code.co_varnames.index('first'),
                                code.co_varnames.index('trigger'))
                self.assertGreater(code.co_varnames.index('later'),
                                   code.co_varnames.index('trigger'))
                frame.clear()
                # Clearing visits each home once. A callback's write below
                # the cursor is retired; one above it remains a heap binding.
                self.assertEqual(observed, [True])
                self.assertEqual(closed, ['first'])
                self.assertIsNone(references['first']())
                self.assertIs(frame.f_locals['later'], references['later']())
                self.assertIs(frame.f_code, code)
                frame.clear()
                self.assertEqual(closed, ['first', 'later'])
                self.assertIsNone(references['later']())
                frames.clear()

    def test_clear_does_not_clear_specials(self):
        class C:
            pass
        c = C()
        exc = self.outer(c=c)
        del c
        f = exc.__traceback__.tb_frame
        f.clear()
        self.assertIsNot(f.f_code, None)
        self.assertIsNot(f.f_locals, None)
        self.assertIsNot(f.f_builtins, None)
        self.assertIsNot(f.f_globals, None)

    def test_clear_generator(self):
        endly = False
        def g():
            nonlocal endly
            try:
                yield
                self.inner()
            finally:
                endly = True
        gen = g()
        next(gen)
        self.assertFalse(endly)

        # Cannot clear a suspended frame
        with self.assertRaisesRegex(RuntimeError, r'suspended frame'):
            gen.gi_frame.clear()
        self.assertFalse(endly)

    def test_clear_executing(self):
        # Attempting to clear an executing frame is forbidden.
        try:
            1/0
        except ZeroDivisionError as e:
            f = e.__traceback__.tb_frame
        with self.assertRaises(RuntimeError):
            f.clear()
        with self.assertRaises(RuntimeError):
            f.f_back.clear()

    def test_clear_executing_generator(self):
        # Attempting to clear an executing generator frame is forbidden.
        endly = False
        def g():
            nonlocal endly
            try:
                1/0
            except ZeroDivisionError as e:
                f = e.__traceback__.tb_frame
                with self.assertRaises(RuntimeError):
                    f.clear()
                with self.assertRaises(RuntimeError):
                    f.f_back.clear()
                yield f
            finally:
                endly = True
        gen = g()
        f = next(gen)
        self.assertFalse(endly)
        # Cannot clear a suspended frame
        with self.assertRaisesRegex(RuntimeError, 'suspended frame'):
            f.clear()
        self.assertFalse(endly)

    def test_lineno_with_tracing(self):
        def record_line():
            f = sys._getframe(1)
            lines.append(f.f_lineno-f.f_code.co_firstlineno)

        def test(trace):
            record_line()
            if trace:
                sys._getframe(0).f_trace = True
            record_line()
            record_line()

        expected_lines = [1, 4, 5]
        lines = []
        test(False)
        self.assertEqual(lines, expected_lines)
        lines = []
        test(True)
        self.assertEqual(lines, expected_lines)

    @support.cpython_only
    @support.requires_refcounting
    def test_clear_refcycles(self):
        # .clear() doesn't leave any refcycle behind
        with support.disable_gc():
            class C:
                pass
            c = C()
            wr = weakref.ref(c)
            exc = self.outer(c=c)
            del c
            self.assertIsNot(None, wr())
            self.clear_traceback_frames(exc.__traceback__)
            self.assertIs(None, wr())


class DeallocTest(unittest.TestCase):
    def test_code_pin_and_remaining_homes_survive_finalizers(self):
        internal = import_helper.import_module('_testinternalcapi')
        for nested in (False, True):
            with self.subTest(nested=nested):
                observations = []

                def target(lower, trigger):
                    return sys._getframe()

                class Lower:
                    def __del__(self):
                        observations.append(('lower', code_ref() is not None))

                class NestedTrigger:
                    def __del__(self):
                        ids = set(internal.get_transition_root_ids())
                        observations.append(('nested', code_id in ids,
                                             id(target.__code__) in ids))
                        support.gc_collect()

                class Trigger:
                    def __del__(self):
                        ids = set(internal.get_transition_root_ids())
                        observations.append(('home', frame_ref() is None,
                                             function_ref() is None,
                                             code_ref() is not None,
                                             code_id in ids, lower_id in ids,
                                             trace_ref() is not None))
                        if nested:
                            nested_frame = target(None, NestedTrigger())
                            del nested_frame
                        support.gc_collect()

                class Trace:
                    def __call__(self, frame, event, arg):
                        return self

                    def __del__(self):
                        ids = set(internal.get_transition_root_ids())
                        observations.append(('trace', code_ref() is not None,
                                             code_id in ids,
                                             lower_ref() is None))
                        support.gc_collect()

                def frame_gone(ref):
                    ids = set(internal.get_transition_root_ids())
                    observations.append(('weakref', function_ref() is not None,
                                         code_id in ids, lower_id in ids,
                                         trace_ref() is not None))
                    support.gc_collect()

                code = target.__code__.replace()
                code_id = id(code)
                code_ref = weakref.ref(code)
                function = types.FunctionType(code, globals())
                function_ref = weakref.ref(function)
                lower = Lower()
                lower_id = id(lower)
                lower_ref = weakref.ref(lower)
                frame = function(lower, Trigger())
                trace = Trace()
                trace_ref = weakref.ref(trace)
                frame.f_trace = trace
                frame_ref = weakref.ref(frame, frame_gone)
                del lower, trace, function, code
                del frame
                expected = [('weakref', True, True, True, True),
                            ('home', True, True, True, True, True, True)]
                if nested:
                    expected.append(('nested', True, True))
                expected.extend([('lower', True), ('trace', True, True, True)])
                self.assertEqual(observations, expected)
                self.assertIsNone(code_ref())
                self.assertIsNone(function_ref())
                self.assertIsNone(lower_ref())
                self.assertIsNone(trace_ref())
                self.assertEqual(internal.get_transition_root_ids(), [])


class DelegatedPhaseTest(unittest.TestCase):
    def test_marker_tracks_real_suspension_and_invalidates_before_resume(self):
        internal = import_helper.import_module('_testinternalcapi')
        observations = []

        def delegate():
            yield 'first'
            return 'done'

        def outer(receiver):
            local = 'before'
            result = yield from receiver
            observations.append(internal.has_delegated_frame_phase(sys._getframe()))
            yield result, local

        receiver = delegate()
        gen = outer(receiver)
        frame = gen.gi_frame
        self.assertFalse(internal.has_delegated_frame_phase(frame))
        self.assertEqual(next(gen), 'first')
        self.assertTrue(internal.has_delegated_frame_phase(frame))
        self.assertIs(gen.gi_yieldfrom, receiver)
        frame.f_locals['local'] = 'changed'
        support.gc_collect()
        self.assertTrue(internal.has_delegated_frame_phase(frame))
        self.assertEqual(next(gen), ('done', 'changed'))
        self.assertEqual(observations, [False])
        self.assertFalse(internal.has_delegated_frame_phase(frame))
        gen.close()
        self.assertFalse(internal.has_delegated_frame_phase(frame))

    def test_close_keeps_marker_through_delegate_callback(self):
        internal = import_helper.import_module('_testinternalcapi')
        observations = []
        gen = frame = None

        class Receiver:
            def __iter__(self):
                return self

            def __next__(self):
                return 1

            def close(self):
                observations.append((internal.has_delegated_frame_phase(frame),
                                     gen.gi_yieldfrom))
                support.gc_collect()

        def outer(receiver):
            yield from receiver

        gen = outer(Receiver())
        frame = gen.gi_frame
        self.assertEqual(next(gen), 1)
        gen.close()
        self.assertEqual(observations, [(True, None)])
        self.assertFalse(internal.has_delegated_frame_phase(frame))

    def test_throw_claim_keeps_marker_until_actual_outer_resume(self):
        internal = import_helper.import_module('_testinternalcapi')
        observations = []
        gen = frame = None

        class Receiver:
            def __iter__(self):
                return self

            def __next__(self):
                return 1

            def throw(self, *args):
                observations.append((internal.has_delegated_frame_phase(frame),
                                     gen.gi_yieldfrom))
                support.gc_collect()
                return 2

        receiver = Receiver()

        def outer():
            yield from receiver

        gen = outer()
        frame = gen.gi_frame
        self.assertEqual(next(gen), 1)
        self.assertEqual(gen.throw(ValueError), 2)
        self.assertEqual(observations, [(True, None)])
        self.assertTrue(internal.has_delegated_frame_phase(frame))
        self.assertIs(gen.gi_yieldfrom, receiver)
        gen.close()
        self.assertFalse(internal.has_delegated_frame_phase(frame))

    def test_clear_cannot_reacquire_marker_from_saved_continuation(self):
        internal = import_helper.import_module('_testinternalcapi')

        def outer():
            yield from iter((1, 2))

        gen = outer()
        frame = gen.gi_frame
        self.assertEqual(next(gen), 1)
        self.assertTrue(internal.has_delegated_frame_phase(frame))
        with self.assertRaisesRegex(RuntimeError, 'suspended frame'):
            frame.clear()
        self.assertTrue(internal.has_delegated_frame_phase(frame))
        gen.close()
        frame.clear()
        self.assertFalse(internal.has_delegated_frame_phase(frame))
        self.assertIsNone(gen.gi_yieldfrom)
        support.gc_collect()
        self.assertFalse(internal.has_delegated_frame_phase(frame))

    def test_yield_observer_precedes_marker_and_cannot_redirect(self):
        internal = import_helper.import_module('_testinternalcapi')
        observations = []

        def outer(receiver):
            yield from receiver

        def observe(code, offset, value):
            self.assertIs(code, outer.__code__)
            frame = sys._getframe(1)
            observations.append(internal.has_delegated_frame_phase(frame))
            with self.assertRaisesRegex(ValueError, 'suspended or protected owner recipe'):
                frame.f_lineno = frame.f_lineno

        mon, tool = sys.monitoring, 4
        gen = outer(iter((1,)))
        mon.use_tool_id(tool, 'delegated-phase')
        try:
            mon.register_callback(tool, mon.events.PY_YIELD, observe)
            mon.set_local_events(tool, outer.__code__, mon.events.PY_YIELD)
            self.assertEqual(next(gen), 1)
            self.assertEqual(observations, [False])
            self.assertTrue(internal.has_delegated_frame_phase(gen.gi_frame))
        finally:
            mon.set_local_events(tool, outer.__code__, 0)
            mon.register_callback(tool, mon.events.PY_YIELD, None)
            mon.free_tool_id(tool)
            gen.close()

    def test_selected_copy_preserves_only_a_retained_receiver(self):
        internal = import_helper.import_module('_testinternalcapi')

        def outer(receiver):
            yield from receiver

        for payload in (object(), 17, 2.5):
            with self.subTest(payload=type(payload)):
                internal.check_delegated_frame_copy(outer.__code__, payload)
                internal.check_clipped_delegated_frame_copy(outer.__code__, payload)


class CopyHomeViewTest(unittest.TestCase):
    def test_created_home_view_requires_marker_and_tracks_current_locals(self):
        internal = import_helper.import_module('_testinternalcapi')

        def body(value):
            yield value

        payload = object()
        gen = body(None)
        frame = gen.gi_frame
        authority, bindings, limit, nranges = internal.get_frame_home_view(frame)
        self.assertEqual((authority, limit, nranges), (2, 0, 0))
        self.assertGreaterEqual(bindings, 1)
        internal.check_created_home_view_invalidation(frame)
        frame.f_locals['value'] = payload
        support.gc_collect()
        self.assertEqual(internal.get_frame_home_view(frame)[0], 2)
        self.assertIs(next(gen), payload)
        # The created certificate was invalidated; dynamic presence remains a
        # separate provider for the actually suspended frame's current homes.
        self.assertEqual(internal.get_frame_home_view(frame)[0], 2)
        self.assertIn(id(payload), internal.get_frame_selected_object_ids(frame))
        gen.close()
        frame.clear()

    def test_empty_created_copy_uses_certified_kind_and_skips_scratch(self):
        internal = import_helper.import_module('_testinternalcapi')

        def gen(value):
            yield value

        async def coro(value):
            return value

        async def asyncgen(value):
            yield value

        for function in (gen, coro, asyncgen):
            with self.subTest(function=function.__name__):
                self.assertTrue(internal.has_empty_created_copy_plan(function.__code__))
                for payload in (object(), 17, 2.5):
                    internal.check_created_frame_copy(function.__code__, payload)

    def test_empty_created_copy_preserves_cells_and_free_variables(self):
        internal = import_helper.import_module('_testinternalcapi')
        captured = object()

        def body(value):
            def read():
                return value, captured
            yield read

        self.assertTrue(internal.has_empty_created_copy_plan(body.__code__))
        payload = object()
        gen = body(payload)
        try:
            read = next(gen)
            self.assertEqual(read(), (payload, captured))
        finally:
            gen.close()

    def test_genexpr_iterator_is_not_empty_created_scratch(self):
        internal = import_helper.import_module('_testinternalcapi')
        payload = object()
        gen = (value for value in (payload,))
        self.assertFalse(internal.has_empty_created_copy_plan(gen.gi_code))
        self.assertIs(next(gen), payload)
        with self.assertRaises(StopIteration):
            next(gen)

    def test_zero_binding_escape_keeps_empty_presence(self):
        internal = import_helper.import_module('_testinternalcapi')

        def body():
            return sys._getframe()

        frame = body()
        self.assertEqual(internal.get_frame_home_view(frame), (2, 0, 0, 0))
        frame.clear()
        self.assertEqual(internal.get_frame_home_view(frame)[0], 2)
        self.assertEqual(internal.get_frame_selected_object_ids(frame), [])

    def test_selected_copy_skips_inactive_words(self):
        internal = import_helper.import_module('_testinternalcapi')

        def body(value):
            return value, object(), object()

        for payload in (object(), 17, 2.5):
            with self.subTest(payload=type(payload)):
                internal.check_frame_selected_copy(body.__code__, payload)

    def test_escaped_frame_keeps_explicit_presence(self):
        internal = import_helper.import_module('_testinternalcapi')

        def body(value):
            scalar = 17
            return sys._getframe()

        def replacement(value):
            return value

        payload = object()
        code = body.__code__
        frame = body(payload)
        authority, bindings, limit, nranges = internal.get_frame_home_view(frame)
        self.assertEqual((authority, limit, nranges), (2, 0, 0))
        self.assertGreaterEqual(bindings, code.co_nlocals)
        body.__code__ = replacement.__code__
        self.assertIs(frame.f_code, code)
        self.assertIs(frame.f_locals['value'], payload)
        self.assertEqual(frame.f_locals['scalar'], 17)
        frame.clear()
        self.assertEqual(internal.get_frame_home_view(frame)[0], 2)
        self.assertEqual(internal.get_frame_selected_object_ids(frame), [])

    def test_created_generator_and_suspended_escape_values(self):
        internal = import_helper.import_module('_testinternalcapi')

        def body(value):
            scalar = 17
            yield value, scalar

        def replacement(value):
            yield None

        payload = object()
        code = body.__code__
        gen = body(payload)
        frame = gen.gi_frame
        self.assertIs(frame.f_locals['value'], payload)
        body.__code__ = replacement.__code__
        self.assertEqual(next(gen), (payload, 17))
        gen.close()
        self.assertIs(frame.f_code, code)
        self.assertIs(frame.f_locals['value'], payload)
        self.assertEqual(frame.f_locals['scalar'], 17)
        self.assertEqual(internal.get_frame_home_view(frame)[0], 2)
        frame.clear()

    @unittest.skipIf(_testcapi is None, '_testcapi required')
    def test_public_frame_generator_keeps_capture_until_resume(self):
        internal = import_helper.import_module('_testinternalcapi')

        def body():
            yield 17

        gen = _testcapi.frame_new_generator(body.__code__, globals(), {})
        frame = gen.gi_frame
        self.assertEqual(internal.get_frame_home_view(frame)[0], 2)
        self.assertEqual(next(gen), 17)
        gen.close()
        self.assertEqual(internal.get_frame_home_view(frame)[0], 2)
        frame.clear()


class FrameAttrsTest(unittest.TestCase):

    def make_frames(self):
        def outer():
            x = 5
            y = 6
            def inner():
                z = x + 2
                1/0
                t = 9
            return inner()
        try:
            outer()
        except ZeroDivisionError as e:
            tb = e.__traceback__
            frames = []
            while tb:
                frames.append(tb.tb_frame)
                tb = tb.tb_next
        return frames

    def test_clear_locals(self):
        # Test f_locals after clear() (issue #21897)
        f, outer, inner = self.make_frames()
        outer.clear()
        inner.clear()
        self.assertEqual(outer.f_locals, {})
        self.assertEqual(inner.f_locals, {})

    def test_locals_clear_locals(self):
        # Test f_locals before and after clear() (to exercise caching)
        f, outer, inner = self.make_frames()
        self.assertNotEqual(outer.f_locals, {})
        self.assertNotEqual(inner.f_locals, {})
        outer.clear()
        inner.clear()
        self.assertEqual(outer.f_locals, {})
        self.assertEqual(inner.f_locals, {})

    def test_f_lineno_del_segfault(self):
        f, _, _ = self.make_frames()
        with self.assertRaises(AttributeError):
            del f.f_lineno

    def test_f_trace(self):
        f, _, _ = self.make_frames()
        def tracer(*args):
            pass
        for value in tracer, 42, None:
            f.f_trace = value
            self.assertEqual(f.f_trace, value)
        f.f_trace = tracer
        del f.f_trace
        self.assertIsNone(f.f_trace)

    def test_f_trace_lines_and_opcodes(self):
        f, _, _ = self.make_frames()
        for name in 'f_trace_lines', 'f_trace_opcodes':
            with self.subTest(name=name):
                for value in False, True:
                    setattr(f, name, value)
                    self.assertEqual(getattr(f, name), value)
                with self.assertRaisesRegex(TypeError,
                                            'attribute value type must be bool'):
                    setattr(f, name, 1)
        with self.assertRaisesRegex(TypeError,
                                    "can't delete numeric/char attribute"):
            del f.f_trace_lines
        with self.assertRaisesRegex(AttributeError, 'cannot be deleted'):
            del f.f_trace_opcodes

    def test_f_generator(self):
        # Test f_generator in different contexts.

        def t0():
            def nested():
                frame = sys._getframe()
                return frame.f_generator

            def gen():
                yield nested()

            g = gen()
            try:
                return next(g)
            finally:
                g.close()

        def t1():
            frame = sys._getframe()
            return frame.f_generator

        def t2():
            frame = sys._getframe()
            yield frame.f_generator

        async def t3():
            frame = sys._getframe()
            return frame.f_generator

        # For regular functions f_generator is None
        self.assertIsNone(t0())
        self.assertIsNone(t1())

        # For generators f_generator is equal to self
        g = t2()
        try:
            frame_g = next(g)
            self.assertIs(g, frame_g)
        finally:
            g.close()

        # Ditto for coroutines
        c = t3()
        try:
            c.send(None)
        except StopIteration as ex:
            self.assertIs(ex.value, c)
        else:
            raise AssertionError('coroutine did not exit')


class WeakRefTest(unittest.TestCase):
    """
    Frames support weak references (gh-102960).
    """

    def make_frame(self):
        # Return the frame object of a finished function call.  Unlike
        # frames extracted from a traceback, it isn't part of a reference
        # cycle, so it dies as soon as the last reference is dropped.
        def func():
            return sys._getframe()
        return func()

    def make_traceback_frames(self):
        def outer():
            def inner():
                1/0
            return inner()
        try:
            outer()
        except ZeroDivisionError as e:
            tb = e.__traceback__
            frames = []
            while tb:
                frames.append(tb.tb_frame)
                tb = tb.tb_next
        return frames

    def test_weakref_basic(self):
        called = []
        f = self.make_frame()
        ref = weakref.ref(f)
        cb_ref = weakref.ref(f, called.append)
        self.assertIs(ref(), f)
        self.assertIs(cb_ref(), f)
        del f
        support.gc_collect()
        self.assertIsNone(ref())
        self.assertIsNone(cb_ref())
        self.assertEqual(called, [cb_ref])

    @support.thread_unsafe("relies on gc.collect() reclaiming its cycles")
    def test_weakref_live_frame(self):
        refs = []
        def func():
            frame = sys._getframe()
            refs.append(weakref.ref(frame))
            self.assertIs(refs[0](), frame)
        func()
        support.gc_collect()
        self.assertIsNone(refs[0]())

    @support.thread_unsafe("relies on gc.collect() reclaiming its cycles")
    def test_weak_key_dictionary(self):
        wkd = weakref.WeakKeyDictionary()
        def _fill():
            for i, frame in enumerate(self.make_traceback_frames()):
                wkd[frame] = i
            self.assertEqual(len(wkd), 3)
        _fill()
        support.gc_collect()
        self.assertEqual(len(wkd), 0)

    @support.thread_unsafe("relies on gc.collect() reclaiming its cycles")
    def test_weakref_traceback_frames(self):
        # Frames that participate in reference cycles are cleaned up
        # by the cyclic garbage collector.
        refs = []
        def _make():
            for frame in self.make_traceback_frames():
                refs.append(weakref.ref(frame))
            for ref in refs:
                self.assertIsNotNone(ref())
        _make()
        support.gc_collect()
        for ref in refs:
            self.assertIsNone(ref())

    def test_weakref_generator_frame(self):
        def gen():
            yield sys._getframe()
        g = gen()
        frame = next(g)
        ref = weakref.ref(frame)
        del frame
        support.gc_collect()
        # The generator keeps its frame alive while suspended.
        self.assertIsNotNone(ref())
        g.close()
        del g
        support.gc_collect()
        self.assertIsNone(ref())

    def test_weakref_after_frame_clear(self):
        f = self.make_frame()
        ref = weakref.ref(f)
        # Clearing the frame's contents must not affect weak references
        # to the frame object itself.
        f.clear()
        self.assertIs(ref(), f)
        del f
        support.gc_collect()
        self.assertIsNone(ref())

    @threading_helper.requires_working_threading()
    def test_weakref_concurrent(self):
        # Exercise concurrent creation and destruction of weak references
        # to the same frame, mainly for the free-threaded build.
        def gen():
            yield sys._getframe()
        g = gen()
        frame = next(g)
        barrier = threading.Barrier(4)
        # Collect failures instead of asserting in the workers: exceptions
        # raised in threads don't propagate to the unittest result.
        failures = []
        def work():
            barrier.wait()
            for _ in range(1000):
                ref = weakref.ref(frame)
                if ref() is not frame:
                    failures.append('shared ref dead while frame alive')
                # Callback refs are not shared, so this concurrently adds
                # to and removes from the frame's weakref list.
                cb_ref = weakref.ref(frame, lambda r: None)
                if cb_ref() is not frame:
                    failures.append('callback ref dead while frame alive')
                del ref, cb_ref
        threads = [threading.Thread(target=work) for _ in range(4)]
        with threading_helper.start_threads(threads):
            pass
        self.assertEqual(failures, [])
        ref = weakref.ref(frame)
        del frame
        g.close()
        del g
        support.gc_collect()
        self.assertIsNone(ref())


class ReprTest(unittest.TestCase):
    """
    Tests for repr(frame).
    """

    def test_repr(self):
        def outer():
            x = 5
            y = 6
            def inner():
                z = x + 2
                1/0
                t = 9
            return inner()

        offset = outer.__code__.co_firstlineno
        try:
            outer()
        except ZeroDivisionError as e:
            tb = e.__traceback__
            frames = []
            while tb:
                frames.append(tb.tb_frame)
                tb = tb.tb_next
        else:
            self.fail("should have raised")

        f_this, f_outer, f_inner = frames
        file_repr = re.escape(repr(__file__))
        self.assertRegex(repr(f_this),
                         r"^<frame at 0x[0-9a-fA-F]+, file %s, line %d, code test_repr>$"
                         % (file_repr, offset + 23))
        self.assertRegex(repr(f_outer),
                         r"^<frame at 0x[0-9a-fA-F]+, file %s, line %d, code outer>$"
                         % (file_repr, offset + 7))
        self.assertRegex(repr(f_inner),
                         r"^<frame at 0x[0-9a-fA-F]+, file %s, line %d, code inner>$"
                         % (file_repr, offset + 5))

class TestFrameLocals(unittest.TestCase):
    def test_scope(self):
        class A:
            x = 1
            sys._getframe().f_locals['x'] = 2
            sys._getframe().f_locals['y'] = 2

        self.assertEqual(A.x, 2)
        self.assertEqual(A.y, 2)

        def f():
            x = 1
            sys._getframe().f_locals['x'] = 2
            sys._getframe().f_locals['y'] = 2
            self.assertEqual(x, 2)
            self.assertEqual(locals()['y'], 2)
        f()

    def test_closure(self):
        x = 1
        y = 2

        def f():
            z = x + y
            d = sys._getframe().f_locals
            self.assertEqual(d['x'], 1)
            self.assertEqual(d['y'], 2)
            d['x'] = 2
            d['y'] = 3

        f()
        self.assertEqual(x, 2)
        self.assertEqual(y, 3)

    def test_closure_with_inline_comprehension(self):
        lambda: k
        k = 1
        lst = [locals() for k in [0]]
        self.assertEqual(lst[0]['k'], 0)

    def test_as_dict(self):
        x = 1
        y = 2
        d = sys._getframe().f_locals
        # self, x, y, d
        self.assertEqual(len(d), 4)
        self.assertIs(d['d'], d)
        self.assertEqual(set(d.keys()), set(['x', 'y', 'd', 'self']))
        self.assertEqual(len(d.values()), 4)
        self.assertIn(1, d.values())
        self.assertEqual(len(d.items()), 4)
        self.assertIn(('x', 1), d.items())
        self.assertEqual(d.__getitem__('x'), 1)
        d.__setitem__('x', 2)
        self.assertEqual(d['x'], 2)
        self.assertEqual(d.get('x'), 2)
        self.assertIs(d.get('non_exist', None), None)
        self.assertEqual(d.__len__(), 4)
        self.assertEqual(set([key for key in d]), set(['x', 'y', 'd', 'self']))
        self.assertIn('x', d)
        self.assertTrue(d.__contains__('x'))

        self.assertEqual(reversed(d), list(reversed(d.keys())))

        d.update({'x': 3, 'z': 4})
        self.assertEqual(d['x'], 3)
        self.assertEqual(d['z'], 4)

        with self.assertRaises(TypeError):
            d.update([1, 2])

        self.assertEqual(d.setdefault('x', 5), 3)
        self.assertEqual(d.setdefault('new', 5), 5)
        self.assertEqual(d['new'], 5)

        with self.assertRaises(KeyError):
            d['non_exist']

    def test_as_number(self):
        x = 1
        y = 2
        d = sys._getframe().f_locals
        self.assertIn('z', d | {'z': 3})
        d |= {'z': 3}
        self.assertEqual(d['z'], 3)
        d |= {'y': 3}
        self.assertEqual(d['y'], 3)
        with self.assertRaises(TypeError):
            d |= 3
        with self.assertRaises(TypeError):
            _ = d | [3]

    def test_non_string_key(self):
        d = sys._getframe().f_locals
        d[1] = 2
        self.assertEqual(d[1], 2)

    def test_write_with_hidden(self):
        def f():
            f_locals = [sys._getframe().f_locals for b in [0]][0]
            f_locals['b'] = 2
            f_locals['c'] = 3
            self.assertEqual(b, 2)
            self.assertEqual(c, 3)
            b = 0
            c = 0
        f()

    def test_local_objects(self):
        o = object()
        k = '.'.join(['a', 'b', 'c'])
        f_locals = sys._getframe().f_locals
        f_locals['o'] = f_locals['k']
        self.assertEqual(o, 'a.b.c')

    def test_copy(self):
        x = 0
        d = sys._getframe().f_locals
        d_copy = d.copy()
        self.assertIsInstance(d_copy, dict)
        self.assertEqual(d_copy['x'], 0)
        d_copy['x'] = 1
        self.assertEqual(x, 0)

    def test_update_with_self(self):
        def f():
            f_locals = sys._getframe().f_locals
            f_locals.update(f_locals)
            f_locals.update(f_locals)
            f_locals.update(f_locals)
        f()

    def test_repr(self):
        x = 1
        # Introduce a reference cycle
        frame = sys._getframe()
        self.assertEqual(repr(frame.f_locals), repr(dict(frame.f_locals)))

    def test_delete(self):
        x = 1
        d = sys._getframe().f_locals

        # This needs to be tested before f_extra_locals is created
        with self.assertRaisesRegex(KeyError, 'non_exist'):
            del d['non_exist']

        with self.assertRaises(KeyError):
            d.pop('non_exist')

        with self.assertRaisesRegex(ValueError, 'local variables'):
            del d['x']

        with self.assertRaises(AttributeError):
            d.clear()

        with self.assertRaises(ValueError):
            d.pop('x')

        with self.assertRaises(ValueError):
            d.pop('x', None)

        # 'm', 'n' is stored in f_extra_locals
        d['m'] = 1
        d['n'] = 1

        with self.assertRaises(KeyError):
            d.pop('non_exist')

        del d['m']
        self.assertEqual(d.pop('n'), 1)

        self.assertNotIn('m', d)
        self.assertNotIn('n', d)

        self.assertEqual(d.pop('n', 2), 2)

    @support.cpython_only
    def test_sizeof(self):
        proxy = sys._getframe().f_locals
        support.check_sizeof(self, proxy, support.calcobjsize("P"))

    def test_unsupport(self):
        x = 1
        d = sys._getframe().f_locals
        with self.assertRaises(TypeError):
            copy.copy(d)

        with self.assertRaises(TypeError):
            copy.deepcopy(d)

    def test_is_mapping(self):
        x = 1
        d = sys._getframe().f_locals
        self.assertIsInstance(d, Mapping)
        match d:
            case {"x": value}:
                self.assertEqual(value, 1)
                kind = "mapping"
            case _:
                kind = "other"
        self.assertEqual(kind, "mapping")

    def _x_stringlikes(self):
        class StringSubclass(str):
            pass

        class ImpostorX:
            def __hash__(self):
                return hash('x')

            def __eq__(self, other):
                return other == 'x'

        return StringSubclass('x'), ImpostorX(), 'x'

    def test_proxy_key_stringlikes_overwrite(self):
        def f(obj):
            x = 1
            proxy = sys._getframe().f_locals
            proxy[obj] = 2
            return (
                list(proxy.keys()),
                dict(proxy),
                proxy
            )

        for obj in self._x_stringlikes():
            with self.subTest(cls=type(obj).__name__):

                keys_snapshot, proxy_snapshot, proxy = f(obj)
                expected_keys = ['obj', 'x', 'proxy']
                expected_dict = {'obj': 'x', 'x': 2, 'proxy': proxy}
                self.assertEqual(proxy.keys(),  expected_keys)
                self.assertEqual(proxy, expected_dict)
                self.assertEqual(keys_snapshot,  expected_keys)
                self.assertEqual(proxy_snapshot, expected_dict)

    def test_proxy_key_stringlikes_ftrst_write(self):
        def f(obj):
            proxy = sys._getframe().f_locals
            proxy[obj] = 2
            self.assertEqual(x, 2)
            x = 1

        for obj in self._x_stringlikes():
            with self.subTest(cls=type(obj).__name__):
                f(obj)

    def test_proxy_key_unhashables(self):
        class StringSubclass(str):
            __hash__ = None

        class ObjectSubclass:
            __hash__ = None

        proxy = sys._getframe().f_locals

        for obj in StringSubclass('x'), ObjectSubclass():
            with self.subTest(cls=type(obj).__name__):
                with self.assertRaises(TypeError):
                    proxy[obj]
                with self.assertRaises(TypeError):
                    proxy[obj] = 0

    def test_constructor(self):
        FrameLocalsProxy = type([sys._getframe().f_locals
                                 for x in range(1)][0])
        self.assertEqual(FrameLocalsProxy.__name__, 'FrameLocalsProxy')

        def make_frame():
            x = 1
            y = 2
            return sys._getframe()

        proxy = FrameLocalsProxy(make_frame())
        self.assertEqual(proxy, {'x': 1, 'y': 2})

        # constructor expects 1 frame argument
        with self.assertRaises(TypeError):
            FrameLocalsProxy()     # no arguments
        with self.assertRaises(TypeError):
            FrameLocalsProxy(123)  # wrong type
        with self.assertRaises(TypeError):
            FrameLocalsProxy(frame=sys._getframe())  # no keyword arguments

    def test_overwrite_locals(self):
        # Verify we do not crash if we overwrite a local passed as an argument
        # from an ancestor in the call stack.
        def f():
            xs = [1, 2, 3]
            ys = [4, 5, 6]
            return g(xs)

        def g(xs):
            f = sys._getframe()
            f.f_back.f_locals["xs"] = None
            f.f_back.f_locals["ys"] = None
            return xs[1]

        self.assertEqual(f(), 2)


class FrameLocalsProxyMappingTests(mapping_tests.TestHashMappingProtocol):
    """Test that FrameLocalsProxy behaves like a Mapping (with exceptions)"""

    def _f(*args, **kwargs):
        def _f():
            return sys._getframe().f_locals
        return _f()
    type2test = _f

    @unittest.skipIf(True, 'Locals proxies for different frames never compare as equal')
    def test_constructor(self):
        pass

    @unittest.skipIf(True, 'Unlike a mapping: del proxy[key] fails')
    def test_write(self):
        pass

    @unittest.skipIf(True, 'Unlike a mapping: no proxy.popitem')
    def test_popitem(self):
        pass

    @unittest.skipIf(True, 'Unlike a mapping: no proxy.pop')
    def test_pop(self):
        pass

    @unittest.skipIf(True, 'Unlike a mapping: no proxy.clear')
    def test_clear(self):
        pass

    @unittest.skipIf(True, 'Unlike a mapping: no proxy.fromkeys')
    def test_fromkeys(self):
        pass

    # no del
    def test_getitem(self):
        mapping_tests.BasicTestMappingProtocol.test_getitem(self)
        d = self._full_mapping({'a': 1, 'b': 2})
        self.assertEqual(d['a'], 1)
        self.assertEqual(d['b'], 2)
        d['c'] = 3
        d['a'] = 4
        self.assertEqual(d['c'], 3)
        self.assertEqual(d['a'], 4)

    @unittest.skipIf(True, 'Unlike a mapping: no proxy.update')
    def test_update(self):
        pass

    # proxy.copy returns a regular dict
    def test_copy(self):
        d = self._full_mapping({1:1, 2:2, 3:3})
        self.assertEqual(d.copy(), {1:1, 2:2, 3:3})
        d = self._empty_mapping()
        self.assertEqual(d.copy(), d)
        self.assertRaises(TypeError, d.copy, None)

        self.assertIsInstance(d.copy(), dict)

    @unittest.skipIf(True, 'Locals proxies for different frames never compare as equal')
    def test_eq(self):
        pass


class TestFrameCApi(unittest.TestCase):
    def test_basic(self):
        x = 1
        ctypes = import_helper.import_module('ctypes')
        import ctypes.util  # noqa: F811

        @ctypes.util.wrap_dll_function(ctypes.pythonapi)
        def PyEval_GetFrameLocals() -> ctypes.py_object:
            pass

        @ctypes.util.wrap_dll_function(ctypes.pythonapi)
        def PyEval_GetFrameGlobals() -> ctypes.py_object:
            pass

        @ctypes.util.wrap_dll_function(ctypes.pythonapi)
        def PyEval_GetFrameBuiltins() -> ctypes.py_object:
            pass

        @ctypes.util.wrap_dll_function(ctypes.pythonapi)
        def PyFrame_GetLocals(frame: ctypes.py_object) -> ctypes.py_object:
            pass

        frame_locals = PyEval_GetFrameLocals()
        self.assertTrue(type(frame_locals), dict)
        self.assertEqual(frame_locals['x'], 1)
        frame_locals['x'] = 2
        self.assertEqual(x, 1)

        frame_globals = PyEval_GetFrameGlobals()
        self.assertTrue(type(frame_globals), dict)
        self.assertIs(frame_globals, globals())

        frame_builtins = PyEval_GetFrameBuiltins()
        self.assertEqual(frame_builtins, __builtins__)

        frame = sys._getframe()
        f_locals = PyFrame_GetLocals(frame)
        self.assertTrue(f_locals['x'], 1)
        f_locals['x'] = 2
        self.assertEqual(x, 2)


class TestIncompleteFrameAreInvisible(unittest.TestCase):

    def test_issue95818(self):
        # See GH-95818 for details
        code = textwrap.dedent(f"""
            import gc

            gc.set_threshold(1,1,1)
            class GCHello:
                def __del__(self):
                    print("Destroyed from gc")

            def gen():
                yield

            fd = open({__file__!r})
            l = [fd, GCHello()]
            l.append(l)
            del fd
            del l
            gen()
        """)
        assert_python_ok("-c", code)

    @support.cpython_only
    @threading_helper.requires_working_threading()
    @support.requires_refcounting
    def test_sneaky_frame_object_teardown(self):

        class SneakyDel:
            def __del__(self):
                """
                Stash a reference to the entire stack for walking later.

                It may look crazy, but you'd be surprised how common this is
                when using a test runner (like pytest). The typical recipe is:
                ResourceWarning + -Werror + a custom sys.unraisablehook.
                """
                nonlocal sneaky_frame_object
                sneaky_frame_object = sys._getframe()

        class SneakyThread(threading.Thread):
            """
            A separate thread isn't needed to make this code crash, but it does
            make crashes more consistent, since it means sneaky_frame_object is
            backed by freed memory after the thread completes!
            """

            def run(self):
                """Run SneakyDel.__del__ as this frame is popped."""
                ref = SneakyDel()

        sneaky_frame_object = None
        t = SneakyThread()
        t.start()
        t.join()
        # sneaky_frame_object can be anything, really, but it's crucial that
        # SneakyThread.run's frame isn't anywhere on the stack while it's being
        # torn down:
        self.assertIsNotNone(sneaky_frame_object)
        while sneaky_frame_object is not None:
            self.assertIsNot(
                sneaky_frame_object.f_code, SneakyThread.run.__code__
            )
            sneaky_frame_object = sneaky_frame_object.f_back

    @support.requires_refcounting
    def test_entry_frames_are_invisible_during_teardown(self):
        class C:
            """A weakref'able class."""

        def f():
            """Try to find globals and locals as this frame is being cleared."""
            ref = C()
            # Ignore the fact that exec(C()) is a nonsense callback. We're only
            # using exec here because it tries to access the current frame's
            # globals and locals. If it's trying to get those from a shim frame,
            # we'll crash before raising:
            return weakref.ref(ref, exec)

        with support.catch_unraisable_exception() as catcher:
            # Call from C, so there is a shim frame directly above f:
            weak = operator.call(f)  # BOOM!
            # Cool, we didn't crash. Check that the callback actually happened:
            self.assertIs(catcher.unraisable.exc_type, TypeError)
        self.assertIsNone(weak())


if __name__ == "__main__":
    unittest.main()
