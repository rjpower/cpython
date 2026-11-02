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


class TypeMutationWriterTests(unittest.TestCase):
    def test_class_namespace_delete_reports_attribute_error(self):
        class C:
            pass

        C.selected = 123456789
        self.assertEqual(C.selected, 123456789)
        del C.selected
        with self.assertRaises(AttributeError):
            del C.selected

    def test_abstract_truth_error_is_not_replayed_or_published(self):
        class C:
            pass

        error = ValueError("selected truth error")
        events = []

        class Abstract:
            def __bool__(self):
                events.append("truth")
                raise error

        with self.assertRaises(ValueError) as caught:
            type.__dict__["__abstractmethods__"].__set__(C, Abstract())
        self.assertIs(caught.exception, error)
        self.assertEqual(events, ["truth"])
        self.assertNotIn("__abstractmethods__", vars(C))

    def test_direct_module_doc_and_type_params_setters(self):
        class C:
            pass

        C.__firstlineno__ = 42
        type.__dict__["__module__"].__set__(C, "selected_module")
        self.assertEqual(C.__module__, "selected_module")
        self.assertNotIn("__firstlineno__", vars(C))
        type.__dict__["__doc__"].__set__(C, "selected doc")
        self.assertEqual(C.__doc__, "selected doc")
        type.__dict__["__type_params__"].__set__(C, ())
        self.assertEqual(C.__type_params__, ())

    def test_direct_abstract_truth_callback_runs_once(self):
        class C:
            pass

        events = []

        class Abstract:
            def __bool__(self):
                events.append("truth")
                C.marker = "nested write"
                return True

        selected = Abstract()
        descriptor = type.__dict__["__abstractmethods__"]
        descriptor.__set__(C, selected)
        self.assertEqual(events, ["truth"])
        self.assertIs(C.__abstractmethods__, selected)
        self.assertEqual(C.marker, "nested write")
        descriptor.__delete__(C)
        self.assertIsInstance(C(), C)

    def test_direct_annotations_retains_selected_dictionary(self):
        class C:
            pass

        descriptor = type.__dict__["__annotations__"]
        selected = {"selected": int}
        descriptor.__set__(C, selected)
        self.assertIs(descriptor.__get__(C, type), selected)
        descriptor.__set__(C, {"replacement": float})
        self.assertEqual(selected, {"selected": int})
        self.assertEqual(C.__annotations__, {"replacement": float})
        descriptor.__delete__(C)
        self.assertEqual(descriptor.__get__(C, type), {})

    def test_lazy_annotations_calls_selected_callable_once(self):
        class C:
            pass

        events = []
        selected = {"selected": int}

        def annotate(format):
            events.append(format)
            C.marker = "annotation callback"
            return selected

        type.__dict__["__annotate__"].__set__(C, annotate)
        descriptor = type.__dict__["__annotations__"]
        self.assertIs(descriptor.__get__(C, type), selected)
        self.assertIs(descriptor.__get__(C, type), selected)
        self.assertEqual(events, [1])
        self.assertEqual(C.marker, "annotation callback")

    def test_lazy_annotation_error_keeps_identity_and_recovers(self):
        class C:
            pass

        error = ValueError("selected annotation error")
        events = []

        def annotate(format):
            events.append(format)
            raise error

        setter = type.__dict__["__annotate__"]
        getter = type.__dict__["__annotations__"]
        setter.__set__(C, annotate)
        with self.assertRaises(ValueError) as caught:
            getter.__get__(C, type)
        self.assertIs(caught.exception, error)
        self.assertEqual(events, [1])
        setter.__set__(C, lambda format: {"recovered": int})
        self.assertEqual(getter.__get__(C, type), {"recovered": int})


class ImmutableTypeMutationTests(unittest.TestCase):
    def test_direct_abstract_setter_rejects_immutable_type_before_write(self):
        original = dict(vars(int))
        with self.assertRaises(TypeError):
            type.__dict__["__abstractmethods__"].__set__(int, frozenset())
        self.assertEqual(dict(vars(int)), original)


class ScopedMutationCallbackTests(unittest.TestCase):
    def test_real_scope_callback_expanded_route_and_nested_writers(self):
        class C:
            value = 123456789

        events = []

        def callback():
            events.append("callback")
            C.value = 3.25
            type.__dict__["__annotations__"].__set__(C, {"selected": int})
            return C.value, C.__annotations__

        result = _testinternalcapi.typecache_run_mutation(*(C, callback))
        self.assertEqual(result, (3.25, {"selected": int}))
        self.assertEqual(events, ["callback"])

    def test_scope_callback_error_is_not_replayed(self):
        class C:
            pass

        error = ValueError("selected mutation error")
        events = []

        def callback():
            events.append("callback")
            C.marker = "committed before error"
            raise error

        with self.assertRaises(ValueError) as caught:
            _testinternalcapi.typecache_run_mutation(*(C, callback))
        self.assertIs(caught.exception, error)
        self.assertEqual(events, ["callback"])
        self.assertEqual(C.marker, "committed before error")
        self.assertEqual(_testinternalcapi.typecache_run_mutation(*(C, lambda: C.marker)),
                         "committed before error")

    def test_scope_retains_selected_result_through_receiver_mutation(self):
        class Value:
            pass

        class C:
            selected = Value()

        reference = weakref.ref(C.selected)

        def callback():
            selected = C.selected
            C.selected = None
            gc.collect()
            return selected

        result = _testinternalcapi.typecache_run_mutation(*(C, callback))
        self.assertIs(reference(), result)
        del result
        gc.collect()
        self.assertIsNone(reference())


if __name__ == "__main__":
    unittest.main()
