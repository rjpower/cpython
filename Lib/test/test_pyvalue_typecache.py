"""Owning canonical results from the private type lookup cache."""

import gc
import sys
import unittest
import weakref

from test.support import import_helper

_testinternalcapi = import_helper.import_module("_testinternalcapi")


class TypeCacheReferenceTests(unittest.TestCase):
    def test_base_replacement_keeps_selected_object_alive(self):
        class Value:
            pass

        class Base:
            value = Value()

        class Child(Base):
            pass

        selected = Child.value
        reference = weakref.ref(selected)
        Base.value = Value()
        gc.collect()
        self.assertIs(reference(), selected)
        del selected
        gc.collect()
        self.assertIsNone(reference())

    def test_many_attribute_reads_and_replacement(self):
        values = {f"entry_{i}": i + 1000 for i in range(128)}
        cls = type("ManyEntries", (), values)
        for name, value in values.items():
            self.assertEqual(getattr(cls, name), value)
        cls.entry_3 = 3.25
        self.assertEqual(cls.entry_3, 3.25)
        del cls.entry_4
        with self.assertRaises(AttributeError):
            cls.entry_4


class CanonicalTypeCacheTests(unittest.TestCase):
    lookup = staticmethod(_testinternalcapi.type_cache_lookup)

    def assert_cached(self, cls, name, expected):
        name = sys.intern(name)
        self.assertEqual(getattr(cls, name), expected)
        hit, selected, version = self.lookup(cls, name)
        self.assertEqual(hit, 1)
        self.assertNotEqual(version, 0)
        self.assertEqual(selected, expected)
        return selected

    def test_inline_integer(self):
        class C:
            value = 123456789

        self.assert_cached(C, "value", 123456789)

    def test_inline_float(self):
        class C:
            value = 3.25

        self.assert_cached(C, "value", 3.25)

    def test_boolean(self):
        class C:
            value = True

        self.assertIs(self.assert_cached(C, "value", True), True)

    def test_owned_heap_result_survives_replacement(self):
        class Value:
            pass

        class C:
            value = Value()

        selected = self.assert_cached(C, "value", C.value)
        reference = weakref.ref(selected)
        C.value = Value()
        gc.collect()
        self.assertIs(reference(), selected)
        del selected
        gc.collect()
        self.assertIsNone(reference())

    def test_negative_hit(self):
        class C:
            pass

        name = sys.intern("missing_typecache_entry")
        with self.assertRaises(AttributeError):
            getattr(C, name)
        hit, value, version = self.lookup(C, name)
        self.assertEqual(hit, 1)
        self.assertIsNone(value)
        self.assertNotEqual(version, 0)

    def test_growth_keeps_scalar_and_heap_entries(self):
        token = object()
        values = {sys.intern(f"entry_{i}"): i + 1000 for i in range(128)}
        values["floating"] = 8.5
        values["token"] = token
        cls = type("ManyEntries", (), values)
        for name, value in values.items():
            self.assert_cached(cls, name, value)
        for name, value in values.items():
            hit, selected, version = self.lookup(cls, name)
            self.assertEqual(hit, 1)
            self.assertEqual(selected, value)
            self.assertNotEqual(version, 0)
        self.assertIs(self.lookup(cls, "token")[1], token)

    def test_replacement_invalidates_old_scalar(self):
        class C:
            value = 123456789

        self.assert_cached(C, "value", C.value)
        C.value = 8.5
        self.assertEqual(self.lookup(C, "value")[0], 0)
        self.assert_cached(C, "value", 8.5)

    def test_deletion_invalidates_old_hit(self):
        class C:
            value = 123456789

        self.assert_cached(C, "value", C.value)
        del C.value
        self.assertEqual(self.lookup(C, "value")[0], 0)
        with self.assertRaises(AttributeError):
            C.value
        self.assertEqual(self.lookup(C, "value")[0], 1)
        self.assertIsNone(self.lookup(C, "value")[1])

    def test_base_mutation_invalidates_descendant(self):
        class Base:
            value = 123456789

        class Child(Base):
            pass

        self.assert_cached(Child, "value", Child.value)
        Base.value = 6.25
        self.assertEqual(self.lookup(Child, "value")[0], 0)
        self.assert_cached(Child, "value", 6.25)


if __name__ == "__main__":
    unittest.main()
