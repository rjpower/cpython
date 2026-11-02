"""Focused controls for the nontransaction type-reader tail.

Several test shapes are extracted from test_super.TestSuper and
test_buffer.TestBufferProtocol; candidate-only C routes remain documented in
the adjacent evidence because this file is also used as a host control.
"""

import collections.abc
import gc
import unittest
import weakref

try:
    import _testcapi
except ImportError:
    _testcapi = None


def _sequence_registration_flag_weakrefs():
    class Parent:
        def __len__(self):
            return 1

        def __getitem__(self, index):
            if index:
                raise IndexError
            return "item"

    class Child(Parent):
        pass

    parent_ref = weakref.ref(Parent)
    child_ref = weakref.ref(Child)
    collections.abc.Sequence.register(Parent)
    match Child():
        case [value]:
            matched = value
        case _:
            matched = None
    return matched, parent_ref, child_ref


class TestPyValueTypeReaderTail(unittest.TestCase):
    @unittest.skipUnless(
        _testcapi is not None and hasattr(_testcapi, "pymodule_get_token"),
        "requires _testcapi module-token support")
    def test_module_by_token(self):
        # Extracted from test_capi.test_type.TypeTests.test_get_module_by_token.
        token = _testcapi.pymodule_get_token(_testcapi)
        heap_type = _testcapi.create_type_with_token("_testcapi.H", 0)
        self.assertIs(
            _testcapi.pytype_getmodulebytoken(heap_type, token), _testcapi)

        class Child(heap_type):
            pass

        self.assertIs(_testcapi.pytype_getmodulebytoken(Child, token), _testcapi)
        with self.assertRaises(TypeError):
            _testcapi.pytype_getmodulebytoken(int, token)

    def test_zero_argument_super_instance_and_classmethod(self):
        class Base:
            def value(self):
                return "base"

            @classmethod
            def class_value(cls):
                return cls, "base"

        class Child(Base):
            def value(self):
                return super().value() + ":child"

            @classmethod
            def class_value(cls):
                return cls, super().class_value()[1] + ":child"

        self.assertEqual(Child().value(), "base:child")
        self.assertEqual(Child.class_value(), (Child, "base:child"))

    def test_zero_argument_super_scalar_first_argument(self):
        class Base:
            def value(self):
                return "base"

        class Child(Base):
            def value(number):
                return super().value()

        with self.assertRaises(TypeError):
            Child.value(7)

    def test_zero_argument_super_error_routes(self):
        def no_arguments():
            super()

        with self.assertRaisesRegex(RuntimeError, "no arguments"):
            no_arguments()

        class DeletedArgument:
            def method(argument):
                del argument
                super()

        with self.assertRaisesRegex(RuntimeError, r"arg\[0\] deleted"):
            DeletedArgument().method()

        class EmptyClassCell:
            def method(self):
                return super()

        class_cell = EmptyClassCell.method.__closure__[0]
        original = class_cell.cell_contents
        try:
            del class_cell.cell_contents
            with self.assertRaisesRegex(RuntimeError, "empty __class__ cell"):
                EmptyClassCell().method()
        finally:
            class_cell.cell_contents = original

        try:
            class_cell.cell_contents = 123
            with self.assertRaisesRegex(
                    RuntimeError, r"__class__ is not a type \(int\)"):
                EmptyClassCell().method()
        finally:
            class_cell.cell_contents = original

        namespace = {}
        exec("def missing_class_cell(argument):\n    return super()", namespace)
        with self.assertRaisesRegex(RuntimeError, "__class__ cell not found"):
            namespace["missing_class_cell"](object())

    def test_abc_registration_reaches_existing_descendants(self):
        class Parent:
            pass

        class Child(Parent):
            pass

        collections.abc.Sequence.register(Parent)
        self.assertTrue(issubclass(Parent, collections.abc.Sequence))
        self.assertTrue(issubclass(Child, collections.abc.Sequence))

        matched, parent_ref, child_ref = _sequence_registration_flag_weakrefs()
        self.assertEqual(matched, "item")
        gc.collect()
        self.assertIsNone(parent_ref())
        self.assertIsNone(child_ref())

    def test_buffer_release_super_called_once(self):
        calls = []

        class Exporter(bytearray):
            def __release_buffer__(self, view):
                calls.append(view)
                super().__release_buffer__(view)

        with memoryview(Exporter(b"bytes")) as view:
            self.assertEqual(view.tobytes(), b"bytes")
            self.assertEqual(calls, [])
        self.assertEqual(len(calls), 1)


if __name__ == "__main__":
    unittest.main()
