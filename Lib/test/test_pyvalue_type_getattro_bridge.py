import math
import unittest


class TypeGetattroBridgeTests(unittest.TestCase):
    def test_direct_type_getattribute_materializes_scalar_attributes(self):
        class IntType(int):
            inline_zero = 0
            inline_small = 32
            large = 10**100

        class FloatType(float):
            positive_zero = 0.0
            negative_zero = -0.0

        # Calling type.__getattribute__ directly exercises the type slot's C
        # wrapper and its object-result boundary; dotted lookup can use a
        # different interpreter route.
        self.assertEqual(type.__getattribute__(IntType, "inline_zero"), 0)
        self.assertIs(type(type.__getattribute__(IntType, "inline_zero")), int)
        self.assertEqual(type.__getattribute__(IntType, "inline_small"), 32)
        self.assertIs(type(type.__getattribute__(IntType, "inline_small")), int)
        self.assertEqual(type.__getattribute__(IntType, "large"), 10**100)
        self.assertIs(type(type.__getattribute__(IntType, "large")), int)

        positive_zero = type.__getattribute__(FloatType, "positive_zero")
        self.assertEqual(positive_zero, 0.0)
        self.assertIs(type(positive_zero), float)
        negative_zero = type.__getattribute__(FloatType, "negative_zero")
        self.assertEqual(negative_zero, 0.0)
        self.assertIs(type(negative_zero), float)
        self.assertEqual(math.copysign(1.0, negative_zero), -1.0)

    def test_direct_type_getattribute_preserves_object_identity(self):
        marker = object()

        class MarkerType:
            value = marker

        self.assertIs(type.__getattribute__(MarkerType, "value"), marker)

    def test_missing_attribute_error_and_hasattr_suppression(self):
        class EmptyType:
            pass

        with self.assertRaises(AttributeError):
            type.__getattribute__(EmptyType, "missing")
        self.assertFalse(hasattr(EmptyType, "missing"))

    def test_descriptor_exception_identity_is_preserved(self):
        error = RuntimeError("descriptor failure")

        class RaisingDescriptor:
            def __get__(self, instance, owner):
                raise error

        class DescriptorType:
            value = RaisingDescriptor()

        with self.assertRaises(RuntimeError) as caught:
            type.__getattribute__(DescriptorType, "value")
        self.assertIs(caught.exception, error)


if __name__ == "__main__":
    unittest.main()
