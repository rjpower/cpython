'Host reference coverage for the final Clinic tuple-reader routes.'

import subprocess
import sys
import unittest


class FinalConstructorReferenceTests(unittest.TestCase):

    def test_explicit_new_routes_and_arity(self):
        self.assertEqual(float.__new__(float), 0.0)
        self.assertEqual(float.__new__(float, 0), 0.0)
        self.assertEqual(float.__new__(float, 1 << 30), float(1 << 30))
        self.assertEqual(float.__new__(float, 1 << 1000), float(1 << 1000))
        self.assertEqual(tuple.__new__(tuple), ())
        self.assertEqual(tuple.__new__(tuple, [1, 2]), (1, 2))
        self.assertEqual(list(reversed.__new__(reversed, [1, 2])), [2, 1])

        for constructor, arguments in (
                (float.__new__, (float, 1, 2)),
                (tuple.__new__, (tuple, [], 1)),
                (reversed.__new__, (reversed, [], 1))):
            with self.subTest(constructor=constructor):
                with self.assertRaises(TypeError):
                    constructor(*arguments)
        with self.assertRaises(TypeError):
            reversed.__new__(reversed)

    def test_builtin_keyword_rejection_and_subtype_init_exception(self):
        for constructor, arguments in (
                (float.__new__, (float, 1)),
                (tuple.__new__, (tuple, [])),
                (reversed.__new__, (reversed, []))):
            with self.subTest(constructor=constructor):
                with self.assertRaises(TypeError):
                    constructor(*arguments, unexpected=object())

        class FloatSubclass(float):
            def __init__(self, value=0, *, marker=None):
                pass

        class TupleSubclass(tuple):
            def __init__(self, iterable=(), *, marker=None):
                pass

        class ReversedSubclass(reversed):
            def __init__(self, sequence, *, marker=None):
                pass

        marker = object()
        self.assertEqual(float.__new__(FloatSubclass, 2, marker=marker), 2.0)
        self.assertEqual(tuple.__new__(TupleSubclass, [1], marker=marker), (1,))
        self.assertEqual(list(reversed.__new__(ReversedSubclass, [1, 2],
                                               marker=marker)), [2, 1])

    def test_conversion_callbacks_run_once_and_preserve_exception_identity(self):
        calls = []

        class Number:
            def __float__(self):
                calls.append('float')
                return 3.5

        class Iterable:
            def __iter__(self):
                calls.append('iter')
                return iter((4, 5))

        class Sequence:
            def __len__(self):
                calls.append('len')
                return 2

            def __getitem__(self, index):
                calls.append(('getitem', index))
                return index

        self.assertEqual(float.__new__(float, Number()), 3.5)
        self.assertEqual(tuple.__new__(tuple, Iterable()), (4, 5))
        iterator = reversed.__new__(reversed, Sequence())
        self.assertEqual(next(iterator), 1)

        reversed_calls = []
        marker = object()

        class ReversedHook:
            def __reversed__(self):
                reversed_calls.append('__reversed__')
                return iter((marker,))

        self.assertEqual(list(reversed.__new__(reversed, ReversedHook())),
                         [marker])
        self.assertEqual(reversed_calls, ['__reversed__'])
        self.assertEqual(calls.count('float'), 1)
        self.assertEqual(calls.count('iter'), 1)
        self.assertEqual(calls.count('len'), 1)

        sentinel = RuntimeError('sentinel')

        class BadFloat:
            def __float__(self):
                raise sentinel

        class BadIterable:
            def __iter__(self):
                raise sentinel

        class BadSequence:
            def __len__(self):
                raise sentinel

            def __getitem__(self, index):
                return index

        for constructor, arguments in (
                (float.__new__, (float, BadFloat())),
                (tuple.__new__, (tuple, BadIterable())),
                (reversed.__new__, (reversed, BadSequence()))):
            with self.subTest(constructor=constructor):
                with self.assertRaises(RuntimeError) as caught:
                    constructor(*arguments)
                self.assertIs(caught.exception, sentinel)

    def test_atexit_retains_payload_and_returns_callback_once_in_subprocess(self):
        script = r'''
import atexit, gc
seen = []
payload = object()
payload_id = id(payload)
def callback(value, *, marker):
    seen.append((id(value) == payload_id, id(marker) == payload_id))
registered = atexit.register(callback, payload, marker=payload)
assert registered is callback
del payload
gc.collect()
atexit._run_exitfuncs()
atexit._run_exitfuncs()
assert seen == [(True, True)]
print("atexit reference route passed")
'''
        result = subprocess.run([sys.executable, '-c', script],
                                capture_output=True, text=True, check=True)
        self.assertIn('atexit reference route passed', result.stdout)

    def test_atexit_rejects_missing_or_noncallable_callback(self):
        import atexit

        with self.assertRaises(TypeError):
            atexit.register()
        with self.assertRaises(TypeError):
            atexit.register(42)


if __name__ == '__main__':
    unittest.main()
