"""Exception matching and default-hook reads from canonical containers."""

import contextlib
import gc
import io
import sys
import unittest
import weakref


def hook_args_type():
    captured = []

    class Broken:
        def __del__(self):
            raise ValueError('hook-class witness')

    previous = sys.unraisablehook
    sys.unraisablehook = captured.append
    try:
        obj = Broken()
        del obj
    finally:
        sys.unraisablehook = previous
    args = captured.pop()
    cls = type(args)
    args.exc_value.__traceback__ = None
    return cls


class ErrorValueTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hook_args = hook_args_type()

    def test_nested_exception_alternatives(self):
        import _testcapi

        match = _testcapi.err_givenexceptionmatches
        alternatives = (1, 1.25, (None, TypeError, (ValueError,)))
        self.assertTrue(match(ValueError, alternatives))
        self.assertTrue(match(ValueError('witness'), alternatives))
        self.assertFalse(match(KeyError, alternatives))
        self.assertFalse(match(ValueError, (1, 1.25, (None, TypeError))))
        self.assertFalse(match(ValueError, ()))

    def test_non_exception_identity_fallback(self):
        import _testcapi

        match = _testcapi.err_givenexceptionmatches
        for value in (1, -7, 1.25, -0.0, True, None, 2 ** 80):
            with self.subTest(value=value):
                self.assertTrue(match(value, (value,)))
                self.assertFalse(match(value, (object(),)))

    def test_inline_and_heap_hook_objects(self):
        for value in (1, -7, 1.25, -0.0, float('nan'), False, None, 2 ** 80):
            with self.subTest(value=value):
                args = self.hook_args((ValueError, ValueError('native witness'),
                                       None, None, value))
                stream = io.StringIO()
                with contextlib.redirect_stderr(stream):
                    self.assertIsNone(sys.__unraisablehook__(args))
                output = stream.getvalue()
                self.assertIn('ValueError: native witness', output)
                if value is not None:
                    self.assertIn('Exception ignored in: ' + repr(value), output)

    def test_heap_object_survives_repr_and_stream_callbacks(self):
        events = []

        class Subject:
            def __repr__(self):
                events.append('repr')
                gc.collect()
                return 'heap subject'

            def __del__(self):
                events.append('retired')

        subject = Subject()
        ref = weakref.ref(subject)
        args = self.hook_args((ValueError, ValueError('native witness'),
                               None, None, subject))
        del subject

        class Stream(io.StringIO):
            def write(stream, text):
                self.assertIsNotNone(ref())
                events.append('write')
                gc.collect()
                return super().write(text)

        stream = Stream()
        with contextlib.redirect_stderr(stream):
            self.assertIsNone(sys.__unraisablehook__(args))
        self.assertIn('heap subject', stream.getvalue())
        self.assertIn('repr', events)
        self.assertIn('write', events)
        self.assertNotIn('retired', events)
        del args
        gc.collect()
        self.assertIsNone(ref())
        self.assertEqual(events.count('retired'), 1)

    def test_repr_failure_uses_the_existing_placeholder(self):
        class BrokenRepr:
            def __repr__(self):
                raise LookupError('repr witness')

        args = self.hook_args((ValueError, ValueError('native witness'),
                               None, None, BrokenRepr()))
        stream = io.StringIO()
        with contextlib.redirect_stderr(stream):
            self.assertIsNone(sys.__unraisablehook__(args))
        self.assertIn('<object repr() failed>', stream.getvalue())
        self.assertIn('ValueError: native witness', stream.getvalue())

    def test_stream_failure_preserves_the_error(self):
        marker = OSError('stream witness')

        class BrokenStream:
            def write(self, text):
                raise marker

        args = self.hook_args((ValueError, ValueError('native witness'),
                               None, None, 1.25))
        with contextlib.redirect_stderr(BrokenStream()):
            with self.assertRaises(OSError) as caught:
                sys.__unraisablehook__(args)
        self.assertIs(caught.exception, marker)

    def test_wrong_hook_argument_type(self):
        with self.assertRaisesRegex(TypeError, 'UnraisableHookArgs'):
            sys.__unraisablehook__((ValueError, ValueError(), None, None, 1))


if __name__ == '__main__':
    unittest.main()
