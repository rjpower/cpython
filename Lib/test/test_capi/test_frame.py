import sys
import unittest
import weakref
from test import support
from test.support import import_helper


_testcapi = import_helper.import_module('_testcapi')


class FrameTest(unittest.TestCase):
    def test_capi_generator_first_input_and_escaped_frame(self):
        class Namespace(dict):
            pass

        def body():
            yield 17
            return 29

        for finish in ('return', 'close'):
            with self.subTest(finish=finish):
                namespace = Namespace()
                namespace_ref = weakref.ref(namespace)
                gen = _testcapi.frame_new_generator(body.__code__, globals(), namespace)
                frame = gen.gi_frame
                del namespace
                support.gc_collect()
                self.assertIsNotNone(namespace_ref())
                self.assertIs(_testcapi.frame_getgenerator(frame), gen)
                with self.assertRaises(TypeError):
                    gen.send(1)
                self.assertEqual(gen.send(None), 17)
                if finish == 'return':
                    with self.assertRaises(StopIteration) as stopped:
                        next(gen)
                    self.assertEqual(stopped.exception.value, 29)
                else:
                    gen.close()
                self.assertIsNone(gen.gi_frame)
                self.assertIs(frame.f_code, body.__code__)
                del gen
                support.gc_collect()
                self.assertIsNotNone(namespace_ref())
                frame.clear()
                support.gc_collect()
                self.assertIsNone(namespace_ref())

    def test_capi_coroutine_first_input_and_origin(self):
        async def body():
            return 41

        previous = sys.get_coroutine_origin_tracking_depth()
        try:
            for depth in (0, 2):
                sys.set_coroutine_origin_tracking_depth(depth)
                coro = _testcapi.frame_new_generator(body.__code__, globals(), {})
                frame = coro.cr_frame
                self.assertIs(_testcapi.frame_getgenerator(frame), coro)
                self.assertEqual(coro.cr_origin is None, depth == 0)
                if depth:
                    self.assertEqual(coro.cr_origin[0][2],
                                     'test_capi_coroutine_first_input_and_origin')
                with self.assertRaises(StopIteration) as stopped:
                    coro.send(None)
                self.assertEqual(stopped.exception.value, 41)
                self.assertIsNone(coro.cr_frame)
                frame.clear()
        finally:
            sys.set_coroutine_origin_tracking_depth(previous)

    def test_capi_async_generator_hooks_and_close(self):
        async def body():
            value = yield 53
            yield value

        previous = sys.get_asyncgen_hooks()
        first, finalized = [], []
        try:
            sys.set_asyncgen_hooks(first.append, finalized.append)
            gen = _testcapi.frame_new_generator(body.__code__, globals(), {})
            frame = gen.ag_frame
            self.assertIs(_testcapi.frame_getgenerator(frame), gen)
            with self.assertRaises(StopIteration) as stopped:
                gen.__anext__().send(None)
            self.assertEqual(stopped.exception.value, 53)
            self.assertEqual(first, [gen])
            with self.assertRaises(StopIteration) as stopped:
                gen.asend(67).send(None)
            self.assertEqual(stopped.exception.value, 67)
            with self.assertRaises(StopIteration):
                gen.aclose().send(None)
            self.assertIsNone(gen.ag_frame)
            first.clear()
            del gen
            support.gc_collect()
            self.assertEqual(finalized, [])
            frame.clear()
        finally:
            sys.set_asyncgen_hooks(*previous)

    def getframe(self):
        return sys._getframe()

    def test_frame_getters(self):
        frame = self.getframe()
        self.assertEqual(frame.f_locals, _testcapi.frame_getlocals(frame))
        self.assertIs(frame.f_globals, _testcapi.frame_getglobals(frame))
        self.assertIs(frame.f_builtins, _testcapi.frame_getbuiltins(frame))
        self.assertEqual(frame.f_lasti, _testcapi.frame_getlasti(frame))

    def test_getvar(self):
        current_frame = sys._getframe()
        x = 1
        self.assertEqual(_testcapi.frame_getvar(current_frame, "x"), 1)
        self.assertEqual(_testcapi.frame_getvarstring(current_frame, b"x"), 1)
        with self.assertRaises(NameError):
            _testcapi.frame_getvar(current_frame, "y")
        with self.assertRaises(NameError):
            _testcapi.frame_getvarstring(current_frame, b"y")

        # wrong name type
        with self.assertRaises(TypeError):
            _testcapi.frame_getvar(current_frame, b'x')
        with self.assertRaises(TypeError):
            _testcapi.frame_getvar(current_frame, 123)

    def getgenframe(self):
        yield sys._getframe()

    def test_frame_get_generator(self):
        gen = self.getgenframe()
        frame = next(gen)
        self.assertIs(gen, _testcapi.frame_getgenerator(frame))

    def test_frame_fback_api(self):
        """Test that accessing `f_back` does not cause a segmentation fault on
        a frame created with `PyFrame_New` (GH-99110)."""
        def dummy():
            pass

        frame = _testcapi.frame_new(dummy.__code__, globals(), locals())
        # The following line should not cause a segmentation fault.
        self.assertIsNone(frame.f_back)


if __name__ == "__main__":
    unittest.main()
