"""First-word generator construction and complete-call ownership."""

import dis
import gc
import inspect
import sys
import unittest
import warnings
import weakref


class GeneratorConstructionTests(unittest.TestCase):
    def test_exact_and_positional_plan_bind_final_homes(self):
        namespace = {}
        exec('def exact(value):\n yield value\n'
             'def planned(value, extra=17, *args, **kwargs):\n'
             ' yield value, extra, args, kwargs\n', namespace)
        exact, planned = namespace['exact'], namespace['planned']
        self.assertEqual(next(dis.get_instructions(exact)).opname, 'RETURN_GENERATOR')
        self.assertEqual(next(dis.get_instructions(planned)).opname, 'RETURN_GENERATOR')
        value = object()
        for _ in range(32):
            self.assertIs(next(exact(value)), value)
            self.assertEqual(next(planned(value)), (value, 17, (), {}))
            self.assertEqual(next(planned(value, extra=23)), (value, 23, (), {}))
        left, right = planned(value), planned(value)
        left_kwargs = next(left)[3]
        right_kwargs = next(right)[3]
        self.assertIsNot(left_kwargs, right_kwargs)

    def test_native_owned_vector_construction_and_frame_escape(self):
        namespace = {}
        exec('def make(value):\n yield value\n', namespace)
        make = namespace['make']
        make(None).close()  # Establish the executable before the native call.

        class Payload:
            pass

        value = Payload()
        ref = weakref.ref(value)
        # map invokes its function from the native iterator boundary.
        generators = list(map(make, [value]))
        frame = generators[0].gi_frame
        del value
        gc.collect()
        self.assertIsNotNone(ref())
        self.assertEqual(inspect.getgeneratorstate(generators[0]), inspect.GEN_CREATED)
        generators[0].close()
        self.assertIs(frame.f_locals['value'], ref())
        del generators, frame
        gc.collect()
        self.assertIsNone(ref())

    def test_immediate_coroutine_and_origin_fallback(self):
        namespace = {}
        exec('async def make(value):\n return value\n', namespace)
        make = namespace['make']
        for i in range(32):
            coroutine = make(i)
            self.assertEqual(inspect.getcoroutinestate(coroutine), inspect.CORO_CREATED)
            with self.assertRaises(StopIteration) as stopped:
                coroutine.send(None)
            self.assertEqual(stopped.exception.value, i)
        depth = sys.get_coroutine_origin_tracking_depth()
        self.addCleanup(sys.set_coroutine_origin_tracking_depth, depth)
        sys.set_coroutine_origin_tracking_depth(2)
        coroutine = make(1)
        try:
            self.assertIsNotNone(coroutine.cr_origin)
        finally:
            coroutine.close()

    def test_creation_prefixes_run_before_return(self):
        events = []

        class Iterable:
            def __iter__(self):
                events.append('iter')
                return iter((11,))

        generator = (value for value in Iterable())
        self.assertEqual(events, ['iter'])
        self.assertEqual(next(generator), 11)

        def closure(value):
            def gen():
                yield value
            return gen

        closed = closure(23)
        self.assertNotEqual(next(dis.get_instructions(closed)).opname, 'RETURN_GENERATOR')
        self.assertEqual(next(closed()), 23)

    def test_changed_flags_do_not_authorize_bypassing_word_zero(self):
        namespace = {}
        exec('def plain(optional=17):\n return optional\n'
             'def call():\n return plain()\n', namespace)
        plain = namespace['plain']
        plain.__code__ = plain.__code__.replace(
            co_flags=plain.__code__.co_flags | inspect.CO_GENERATOR)
        self.assertNotEqual(next(dis.get_instructions(plain)).opname, 'RETURN_GENERATOR')
        for _ in range(32):
            self.assertEqual(namespace['call'](), 17)

    def test_exact_displaced_owner_redirects_continuation(self):
        self.check_displaced_owner_redirect('')

    def test_planned_displaced_owner_redirects_continuation(self):
        self.check_displaced_owner_redirect('optional=17')

    def check_displaced_owner_redirect(self, signature):
        namespace = {'sys': sys}
        exec(f'def make({signature}):\n yield 1\n'
             'enabled = False\n'
             'events = []\n'
             'retired_at = []\n'
             'def trace(frame, event, arg):\n'
             ' if frame.f_code is target.__code__ and event == "line":\n'
             '  if frame.f_lineno == target.__code__.co_firstlineno + 3:\n'
             '   frame.f_lineno = target.__code__.co_firstlineno + 4\n'
             ' return trace\n'
             'class Retire:\n'
             ' def __del__(self):\n'
             '  if enabled:\n'
             '   frame = sys._getframe(1)\n'
             '   retired_at.append(frame.f_lasti)\n'
             '   sys.settrace(trace)\n'
             '   frame.f_trace = trace\n'
             'def target():\n'
             ' value = Retire()\n'
             ' value = make()\n'
             ' events.append("skipped")\n'
             ' events.append("selected")\n'
             ' value.close()\n', namespace)
        target = namespace['target']
        for _ in range(32):
            target()
        namespace['events'].clear()
        namespace['enabled'] = True
        try:
            target()
        finally:
            sys.settrace(None)
        call_sites = [instruction.offset for instruction in dis.get_instructions(target)
                      if instruction.opname == 'CALL']
        self.assertEqual(namespace['retired_at'], [call_sites[1]])
        self.assertEqual(namespace['events'], ['selected'])

    def test_discarded_coroutine_warns_once_after_call(self):
        namespace = {}
        exec('async def make(optional=17):\n return optional\n'
             'def discard():\n make()\n', namespace)
        with warnings.catch_warnings(record=True) as caught:
            warnings.simplefilter('always', RuntimeWarning)
            for _ in range(32):
                namespace['discard']()
            gc.collect()
        self.assertEqual(len(caught), 32)
        self.assertTrue(all('was never awaited' in str(item.message) for item in caught))


if __name__ == '__main__':
    unittest.main()
