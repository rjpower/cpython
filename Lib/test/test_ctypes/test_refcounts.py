import ctypes
import unittest
from test.support import script_helper


class ModuleIsolationTest(unittest.TestCase):
    def test_finalize(self):
        # check if gc_decref() succeeds
        script = (
            "import ctypes;"
            "import sys;"
            "del sys.modules['_ctypes'];"
            "import _ctypes;"
            "exit()"
        )
        script_helper.assert_python_ok("-c", script)


class PyObjectRestypeTest(unittest.TestCase):
    def test_restype_py_object_with_null_return(self):
        # Test that a function which returns a NULL PyObject *
        # without setting an exception does not crash.
        @ctypes.util.wrap_dll_function(ctypes.pythonapi)
        def PyErr_Occurred() -> ctypes.py_object:
            pass

        # At this point, there's no exception set, so PyErr_Occurred
        # returns NULL. Given the restype is py_object, the
        # ctypes machinery will raise a custom error.
        with self.assertRaisesRegex(ValueError, "PyObject is NULL"):
            PyErr_Occurred()


if __name__ == '__main__':
    unittest.main()
