import sys
import unittest
import weakref
from test import support
from test.support import import_helper


_testcapi = import_helper.import_module('_testcapi')


class FrameTest(unittest.TestCase):
    def test_frame_new_locals_mapping(self):
        namespace = {'value': 23}
        code = compile('pass', '<frame-new-mapping>', 'exec')
        frame = _testcapi.frame_new(code, globals(), namespace)
        self.assertIs(frame.f_locals, namespace)
        self.assertIs(_testcapi.frame_getlocals(frame), namespace)
        frame.clear()
        self.assertEqual(frame.f_locals, {})
        self.assertEqual(namespace, {'value': 23})

    def test_frame_new_generator_mapping(self):
        class Namespace(dict):
            pass

        def body():
            yield 17

        namespace = Namespace()
        namespace_ref = weakref.ref(namespace)
        generator = _testcapi.frame_new_generator(body.__code__, globals(), namespace)
        frame = generator.gi_frame
        del namespace
        support.gc_collect()
        self.assertIsNotNone(namespace_ref())
        self.assertEqual(next(generator), 17)
        generator.close()
        self.assertIsNotNone(namespace_ref())
        frame.clear()
        support.gc_collect()
        self.assertIsNone(namespace_ref())

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
