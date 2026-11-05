"""Tests of the value vectorcall from the extension side.

The callables are in Modules/_testcapi/value_call.c and use only the object
API; the core runs them through its value-call adapters.
"""

import sys
import unittest
from test.support import import_helper
from test.support.script_helper import assert_python_failure

_testcapi = import_helper.import_module('_testcapi')


def add(a, b=10):
    return a + b


class Obj:
    def m(self, x):
        return (self, x)


class ExtensionCallableTests(unittest.TestCase):
    def test_object_vectorcall_and_tp_call_agree(self):
        vc = _testcapi.ObjVectorcall()(1, 2.5, x=3)
        tp = _testcapi.ObjTpCall()(1, 2.5, x=3)
        self.assertEqual(vc, tp)
        args, kwargs = vc
        self.assertEqual(args, (1, 2.5))
        self.assertIs(type(args[0]), int)
        self.assertIs(type(args[1]), float)
        self.assertEqual(kwargs, {'x': 3})

    def test_no_arguments(self):
        self.assertEqual(_testcapi.ObjVectorcall()(), ((), {}))
        self.assertEqual(_testcapi.ObjTpCall()(), ((), {}))


class VectorcallReplacementTests(unittest.TestCase):
    def test_replacement(self):
        def f(a, b=10):
            return a + b

        self.assertEqual(f(1), 11)
        _testcapi.set_vectorcall_replacement(f)
        try:
            self.assertEqual(f(1, 2), ("replaced", (1, 2)))
            self.assertEqual(_testcapi.vectorcall_function_call(f, 3),
                             ("replaced", (3,)))
            self.assertEqual(list(map(f, [1, 2])),
                             [("replaced", (1,)), ("replaced", (2,))])
        finally:
            _testcapi.restore_vectorcall(f)
        self.assertEqual(f(1), 11)
        self.assertEqual(f(1, 2), 3)
        self.assertEqual(list(map(f, [1, 2])), [11, 12])


class VectorcallFunctionCallTests(unittest.TestCase):
    def test_python_function(self):
        call = _testcapi.vectorcall_function_call
        self.assertEqual(call(add, 1, 2), 3)
        self.assertEqual(call(add, 1), 11)
        self.assertEqual(call(add, 1.5, 2.5), 4.0)

    def test_bound_method(self):
        o = Obj()
        self.assertEqual(_testcapi.vectorcall_function_call(o.m, 7), (o, 7))

    def test_type(self):
        self.assertIs(_testcapi.vectorcall_function_call(type, 5), int)

    def test_builtin(self):
        self.assertEqual(_testcapi.vectorcall_function_call(len, [1, 2]), 2)
        self.assertEqual(_testcapi.vectorcall_function_call(len, "abc"), 3)


class GetSelfRefTests(unittest.TestCase):
    def test_builtin_self(self):
        d = int.__dict__['bit_length']
        self.assertEqual(_testcapi.get_self_ref(d.__get__(5)), 5)
        lst = [1]
        self.assertIs(_testcapi.get_self_ref(lst.append), lst)

    def test_bound_method_self(self):
        o = Obj()
        self.assertIs(_testcapi.get_self_ref(o.m), o)


NULL_RESULT_CODE = """
import _testcapi
_testcapi.{name}()
"""


class NullResultTests(unittest.TestCase):
    def check_null_result(self, name):
        if hasattr(sys, 'gettotalrefcount'):
            # The adapter's check is fatal in a debug build.
            assert_python_failure('-c', NULL_RESULT_CODE.format(name=name))
            return
        with self.assertRaises(SystemError):
            getattr(_testcapi, name)()

    def test_null_result_vectorcall(self):
        self.check_null_result('null_result_vectorcall')

    def test_null_result_tpcall(self):
        self.check_null_result('null_result_tpcall')


if __name__ == "__main__":
    unittest.main()
