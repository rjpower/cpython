"""Canonical value ownership and actual interpreter publication boundaries."""

import math
import sys
import types
import unittest

from test.support import import_helper

capi = import_helper.import_module("_testinternalcapi")


class PyValueTests(unittest.TestCase):
    def test_scalar_boundary_and_identity(self):
        for value in (-2**50, 2**50 - 1, -2**31, 2**31 - 1, 0, -1, 1.25, -0.0,
                      math.inf, -math.inf):
            with self.subTest(value=value):
                kind, result = capi.pyvalue_contract(value)
                self.assertEqual(kind, "int51" if type(value) is int else "float64")
                self.assertIs(type(result), type(value))
                self.assertEqual(result, value)
                if type(value) is float:
                    self.assertEqual(math.copysign(1, result), math.copysign(1, value))

        class Integer(int):
            pass

        class Float(float):
            pass

        for value in (2**50, -2**50 - 1, float("nan"), True, None,
                      Integer(3), Float(2.5), object()):
            with self.subTest(type=type(value)):
                kind, result = capi.pyvalue_contract(value)
                self.assertEqual(kind, "object")
                self.assertIs(result, value)

    def test_internal_values_do_not_cross_object_bridge(self):
        capi.pyvalue_internal_contract()

    def test_interpreter_return_boxing_memory_error(self):
        testcapi = import_helper.import_module("_testcapi")

        def result():
            return 2**31 - 1

        for _ in range(100):
            self.assertEqual(result(), 2**31 - 1)
        # The C fixture arms failure immediately before the ordinary call
        # and restores allocators before returning to the test runner.
        with self.assertRaises(MemoryError):
            testcapi.call_with_nomemory(0, 1, result)
        self.assertEqual(result(), 2**31 - 1)

    def test_constant_arguments_defaults_and_c_result_publish_inline(self):
        def work(a=2**31 - 1, *, b=1.5):
            c = len([None] * 257)
            names = capi.inline_locals(sys._getframe())
            self.assertTrue({"a", "b", "c"} <= names)
            return a, b, c

        self.assertEqual(work(), (2**31 - 1, 1.5, 257))
        self.assertEqual(work(32768, b=-0.0), (32768, -0.0, 257))

    def test_closure_and_generator_inputs_are_canonical(self):
        def outer(value):
            def inner():
                return value
            return inner

        self.assertEqual(outer(2**31 - 1)(), 2**31 - 1)

        def receiver():
            value = yield
            self.assertIn("value", capi.inline_locals(sys._getframe()))
            yield value

        gen = receiver()
        next(gen)
        self.assertEqual(gen.send(65537), 65537)

    def test_numeric_extension_decline_preserves_operands(self):
        def add(a, b):
            return a + b

        for _ in range(100):
            self.assertEqual(add(1, 2.5), 3.5)
        ledger = []

        class Number(int):
            def __radd__(self, left):
                ledger.append(left)
                return 99

        self.assertEqual(add(1, Number(2)), 99)
        self.assertEqual(ledger, [1])
        self.assertEqual(add(2**31 - 1, 1), 2**31)

    def test_specialized_object_guards_decline_canonical_scalars(self):
        class Indexed:
            def __getitem__(self, key):
                return key + 1

        def subscript(obj):
            return obj[0]

        module = types.ModuleType("value_guard")
        module.x = 65537

        def attribute(obj):
            return obj.x

        def invoke(function, args):
            return function(*args)

        def function(value):
            return value

        def super_doc(cls, value):
            return super(cls, value).__doc__

        for _ in range(100):
            self.assertEqual(subscript(Indexed()), 1)
            self.assertEqual(attribute(module), 65537)
            self.assertEqual(invoke(function, (65537,)), 65537)
            self.assertIsInstance(super_doc(int, 65537), str)
        with self.assertRaises(TypeError):
            subscript(5)
        with self.assertRaises(AttributeError):
            attribute(5)
        ledger = []

        class Arguments:
            def __iter__(self):
                ledger.append("iter")
                return iter((1,))

        with self.assertRaises(TypeError):
            invoke(5, Arguments())
        self.assertEqual(ledger, ["iter"])
        with self.assertRaises(TypeError):
            super_doc(5, 65537)

    def test_instance_fields_keep_canonical_numeric_values(self):
        class Instance:
            pass

        obj = Instance()
        obj.x = 1
        mapping = obj.__dict__

        def store(value):
            obj.x = value
            return obj.x

        for value in range(100):
            self.assertEqual(store(value), value)
        self.assertEqual(store(2**31 - 1), 2**31 - 1)
        self.assertEqual(mapping["x"], 2**31 - 1)
        self.assertEqual(store(1.5), 1.5)

    def test_result_visible_before_displaced_owner_finalizer(self):
        ledger = []

        class Owner:
            def __del__(self):
                frame = sys._getframe(1)
                ledger.append(frame.f_locals["value"])

        def work():
            value = Owner()
            value = len([None] * 257)
            self.assertIn("value", capi.inline_locals(sys._getframe()))
            return value

        self.assertEqual(work(), 257)
        self.assertEqual(ledger, [257])

    def test_real_calls_exceptions_and_operand_lifetime(self):
        ledger = []

        class Value:
            def __abs__(self):
                ledger.append("abs")
                return 65537

        def work(callback, value):
            result = abs(callback(value))
            self.assertIn("result", capi.inline_locals(sys._getframe()))
            return result

        def callback(value):
            ledger.append("callback")
            return value

        self.assertEqual(work(callback, Value()), 65537)
        self.assertEqual(ledger, ["callback", "abs"])
        with self.assertRaisesRegex(ValueError, "once"):
            work(lambda value: (_ for _ in ()).throw(ValueError("once")), Value())


if __name__ == "__main__":
    unittest.main()
