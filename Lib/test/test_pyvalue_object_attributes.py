"""Object-form attribute bridges over canonical class and instance values."""

import builtins
import math
import pickle
import unittest
import weakref


class AttributePicklePayload:
    def __init__(self, value):
        self.value = value


class AttributeObjectTests(unittest.TestCase):
    # These invoke the object-returning C protocol, rather than LOAD_ATTR's
    # separate value-native provider.
    readers = (builtins.getattr, object.__getattribute__)

    def test_inherited_class_scalars(self):
        for value in (2147483647, 3.25, True, False, None, 1 << 100):
            class Base:
                selected = value

            class Child(Base):
                pass

            for read in self.readers:
                with self.subTest(value=value, read=read):
                    selected = read(Child(), "selected")
                    self.assertEqual(selected, value)
                    self.assertIs(type(selected), type(value))

    def test_instance_override_and_class_replacement(self):
        class C:
            selected = 2147483647

        obj = C()
        for read in self.readers:
            self.assertEqual(read(obj, "selected"), 2147483647)
        obj.selected = 8.5
        C.selected = None
        for read in self.readers:
            self.assertEqual(read(obj, "selected"), 8.5)
        del obj.selected
        for read in self.readers:
            self.assertIsNone(read(obj, "selected"))
        del C.selected
        self.assertFalse(hasattr(obj, "selected"))

    def test_data_descriptor_precedes_instance(self):
        events = []

        class Descriptor:
            def __get__(self, obj, owner):
                events.append(obj)
                return 3.25

            def __set__(self, obj, value):
                raise AssertionError("unexpected descriptor assignment")

        class C:
            selected = Descriptor()

        obj = C()
        obj.__dict__["selected"] = 2147483647
        for read in self.readers:
            self.assertEqual(read(obj, "selected"), 3.25)
        self.assertEqual(events, [obj, obj])

    def test_instance_precedes_nondata_descriptor(self):
        class Descriptor:
            def __get__(self, obj, owner):
                return 3.25

        class C:
            selected = Descriptor()

        obj = C()
        obj.__dict__["selected"] = 2147483647
        for read in self.readers:
            self.assertEqual(read(obj, "selected"), 2147483647)
        del obj.selected
        for read in self.readers:
            self.assertEqual(read(obj, "selected"), 3.25)

    def test_result_survives_selected_descriptor_retirement(self):
        for read in self.readers:
            events, references = [], []

            class Result:
                pass

            class Descriptor:
                def __get__(self, obj, owner):
                    events.append("get")
                    del owner.selected
                    result = Result()
                    references.append(weakref.ref(result))
                    return result

                def __del__(self):
                    events.append(("descriptor-retire", references[0]() is not None))

            class C:
                selected = Descriptor()

            result = read(C(), "selected")
            self.assertIs(references[0](), result)
            self.assertEqual(events, ["get", ("descriptor-retire", True)])
            del result
            self.assertIsNone(references[0]())

    def test_optional_attribute_error_is_suppressed(self):
        marker = AttributeError("selected descriptor")

        class Descriptor:
            def __get__(self, obj, owner):
                raise marker

        class C:
            selected = Descriptor()

        obj, default = C(), object()
        self.assertIs(getattr(obj, "selected", default), default)
        self.assertFalse(hasattr(obj, "selected"))
        for read in self.readers:
            with self.assertRaises(AttributeError) as caught:
                read(obj, "selected")
            self.assertIs(caught.exception, marker)

    def test_other_descriptor_error_keeps_identity(self):
        marker = ValueError("selected descriptor")

        class Descriptor:
            def __get__(self, obj, owner):
                raise marker

        class C:
            selected = Descriptor()

        obj = C()
        for read in (*self.readers, lambda obj, name: getattr(obj, name, None), hasattr):
            with self.assertRaises(ValueError) as caught:
                read(obj, "selected")
            self.assertIs(caught.exception, marker)

    def test_unicode_subclass_and_invalid_name(self):
        class Name(str):
            pass

        class C:
            selected = 2147483647

        for read in self.readers:
            self.assertEqual(read(C(), Name("selected")), 2147483647)
            with self.assertRaises(TypeError):
                read(C(), 42)

    def test_declared_slots_and_missing_members(self):
        class C:
            __slots__ = ("selected",)

        obj = C()
        default = object()
        self.assertFalse(hasattr(obj, "selected"))
        self.assertIs(getattr(obj, "selected", default), default)
        for value in (2147483647, 3.25, None, object()):
            obj.selected = value
            for read in self.readers:
                self.assertEqual(read(obj, "selected"), value)
        del obj.selected
        for read in self.readers:
            with self.assertRaises(AttributeError):
                read(obj, "selected")

    def test_python_attribute_hooks_remain_distinct(self):
        events = []

        class C:
            selected = 2147483647

            def __getattribute__(self, name):
                if name == "selected":
                    events.append("getattribute")
                    return 3.25
                return object.__getattribute__(self, name)

            def __getattr__(self, name):
                events.append(("getattr", name))
                return None

        obj = C()
        self.assertEqual(getattr(obj, "selected"), 3.25)
        self.assertEqual(object.__getattribute__(obj, "selected"), 2147483647)
        self.assertIsNone(getattr(obj, "missing"))
        self.assertEqual(events, ["getattribute", ("getattr", "missing")])

    def test_nan_and_heap_result_views(self):
        token = object()

        class C:
            nan = float("nan")
            selected = token

        for read in self.readers:
            self.assertTrue(math.isnan(read(C(), "nan")))
            self.assertIs(read(C(), "selected"), token)

    def test_generic_set_and_delete_instance_values(self):
        class C:
            pass

        obj = C()
        for value in (2147483647, 3.25, None, 1 << 100):
            object.__setattr__(obj, "selected", value)
            self.assertEqual(object.__getattribute__(obj, "selected"), value)
            object.__delattr__(obj, "selected")
            with self.assertRaises(AttributeError):
                object.__delattr__(obj, "selected")
        with self.assertRaises(TypeError):
            object.__setattr__(obj, 42, None)

    def test_descriptor_setter_errors_keep_identity(self):
        set_error = KeyError("descriptor set")
        delete_error = KeyError("descriptor delete")
        events = []

        class Descriptor:
            def __set__(self, instance, value):
                events.append(("set", value))
                raise set_error

            def __delete__(self, instance):
                events.append("delete")
                raise delete_error

        class C:
            selected = Descriptor()

        obj = C()
        obj.__dict__["selected"] = "instance"
        with self.assertRaises(KeyError) as caught:
            object.__setattr__(obj, "selected", 2147483647)
        self.assertIs(caught.exception, set_error)
        with self.assertRaises(KeyError) as caught:
            object.__delattr__(obj, "selected")
        self.assertIs(caught.exception, delete_error)
        self.assertEqual(obj.__dict__["selected"], "instance")
        self.assertEqual(events, [("set", 2147483647), "delete"])

    def test_setter_survives_descriptor_replacement(self):
        retired = []
        events = []

        class Descriptor:
            def __set__(self, instance, value):
                C.selected = None
                self.assert_alive = True
                events.append((self.assert_alive, value))

            def __del__(self):
                retired.append("descriptor")

        class C:
            selected = Descriptor()

        object.__setattr__(C(), "selected", 3.25)
        self.assertEqual(events, [(True, 3.25)])
        self.assertEqual(retired, ["descriptor"])

    def test_object_reduce_identity_and_override(self):
        obj = AttributePicklePayload((2147483647, 3.25, None))
        for protocol in range(2, pickle.HIGHEST_PROTOCOL + 1):
            restored = pickle.loads(pickle.dumps(obj, protocol))
            self.assertIs(type(restored), AttributePicklePayload)
            self.assertEqual(restored.value, obj.value)

        class Override:
            def __reduce__(self):
                return (tuple, ((2147483647, 3.25),))

        self.assertEqual(pickle.loads(pickle.dumps(Override())), (2147483647, 3.25))


if __name__ == "__main__":
    unittest.main()
