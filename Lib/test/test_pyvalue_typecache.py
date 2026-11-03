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


class TypeGraphReferenceControls(unittest.TestCase):
    def test_snapshot_retains_child_until_snapshot_release(self):
        class Base:
            pass

        class Child(Base):
            pass

        reference = weakref.ref(Child)
        snapshot = Base.__subclasses__()
        self.assertEqual(snapshot, [Child])
        del Child
        gc.collect()
        self.assertIs(reference(), snapshot[0])
        del snapshot
        gc.collect()
        self.assertIsNone(reference())
        self.assertEqual(Base.__subclasses__(), [])

    def test_returned_snapshot_is_independent_of_later_insertions(self):
        class Base:
            pass

        class First(Base):
            pass

        snapshot = Base.__subclasses__()

        class Second(Base):
            pass

        self.assertEqual(snapshot, [First])
        self.assertEqual(Base.__subclasses__(), [First, Second])
        snapshot.clear()
        self.assertEqual(Base.__subclasses__(), [First, Second])

    def test_dead_siblings_do_not_hide_surviving_children(self):
        class Base:
            pass

        children = [type(f"Child{i}", (Base,), {}) for i in range(24)]
        dead = [weakref.ref(child) for child in children[::2]]
        survivors = children[1::2]
        del children
        gc.collect()
        self.assertTrue(all(ref() is None for ref in dead))
        self.assertEqual(Base.__subclasses__(), survivors)

    def test_diamond_lists_each_immediate_child_once(self):
        class Root:
            pass

        class Left(Root):
            pass

        class Right(Root):
            pass

        class Leaf(Left, Right):
            pass

        self.assertEqual(Root.__subclasses__(), [Left, Right])
        self.assertEqual(Left.__subclasses__(), [Leaf])
        self.assertEqual(Right.__subclasses__(), [Leaf])
        self.assertEqual(Leaf.__subclasses__(), [])

    def test_reparenting_updates_both_parents_and_preserves_snapshot(self):
        class Before:
            pass

        class After:
            pass

        class Child(Before):
            pass

        snapshot = Before.__subclasses__()
        Child.__bases__ = (After,)
        self.assertEqual(snapshot, [Child])
        self.assertEqual(Before.__subclasses__(), [])
        self.assertEqual(After.__subclasses__(), [Child])
        self.assertEqual(Child.__bases__, (After,))

    def test_failed_construction_does_not_leave_a_live_graph_entry(self):
        references = []
        error = ValueError("failed subclass")

        class Base:
            def __init_subclass__(cls):
                references.append(weakref.ref(cls))
                raise error

        with self.assertRaises(ValueError) as caught:
            type("Failed", (Base,), {})
        self.assertIs(caught.exception, error)
        error.__traceback__ = None
        gc.collect()
        self.assertEqual(len(references), 1)
        self.assertIsNone(references[0]())
        self.assertEqual(Base.__subclasses__(), [])

    def test_nested_creation_during_mro_keeps_completed_snapshot(self):
        class Base:
            pass

        events = []
        snapshots = []
        siblings = []

        class Meta(type):
            def mro(cls):
                events.append(cls.__name__)
                sibling = type("Sibling", (Base,), {})
                siblings.append(sibling)
                snapshots.append(Base.__subclasses__())
                return super().mro()

        class Child(Base, metaclass=Meta):
            pass

        self.assertEqual(events, ["Child"])
        self.assertEqual(snapshots, [siblings])
        self.assertEqual(Base.__subclasses__(), [siblings[0], Child])


class TypeMroReferenceControls(unittest.TestCase):
    def test_single_base_and_returned_mro_snapshot(self):
        class Base:
            pass

        class Child(Base):
            pass

        selected = type.mro(Child)
        self.assertEqual(selected, [Child, Base, object])
        selected.clear()
        self.assertEqual(Child.__mro__, (Child, Base, object))

    def test_c3_diamond_and_dominant_base_order(self):
        class Root:
            pass

        class Left(Root):
            pass

        class Right(Root):
            pass

        class Combined(Left, Right):
            pass

        self.assertEqual(type.mro(Combined),
                         [Combined, Left, Right, Root, object])
        self.assertIs(Combined.__base__, Left)
        self.assertTrue(issubclass(Combined, Right))
        self.assertFalse(issubclass(Right, Left))

    def test_duplicate_base_error_uses_name_once(self):
        events = []

        class Named(type):
            def __getattribute__(cls, name):
                if name == "__name__":
                    events.append("name")
                return super().__getattribute__(name)

        class Base(metaclass=Named):
            pass

        with self.assertRaises(TypeError) as caught:
            type("Duplicate", (Base, Base), {})
        self.assertEqual(str(caught.exception), "duplicate base class Base")
        self.assertEqual(events, ["name"])

    def test_duplicate_base_name_error_remains_primary(self):
        events = []
        error = LookupError("selected base name")

        class Named(type):
            def __getattribute__(cls, name):
                if name == "__name__":
                    events.append("name")
                    raise error
                return super().__getattribute__(name)

        class Base(metaclass=Named):
            pass

        with self.assertRaises(LookupError) as caught:
            type("Duplicate", (Base, Base), {})
        self.assertIs(caught.exception, error)
        self.assertEqual(events, ["name"])

    def test_conflicting_c3_formats_owned_candidates_once(self):
        events = []
        enabled = False

        class Named(type):
            def __getattribute__(cls, name):
                if name == "__name__" and enabled:
                    events.append(super().__getattribute__(name))
                    gc.collect()
                return super().__getattribute__(name)

        class X(metaclass=Named):
            pass

        class Y(metaclass=Named):
            pass

        class A(X, Y):
            pass

        class B(Y, X):
            pass

        enabled = True
        with self.assertRaisesRegex(TypeError,
                                    r"consistent method resolution order .*X, Y"):
            type("Conflict", (A, B), {})
        self.assertEqual(events, ["X", "Y"])

    def test_layout_conflict_and_unacceptable_base(self):
        with self.assertRaisesRegex(TypeError, "instance lay-out conflict"):
            type("Conflict", (int, str), {})
        with self.assertRaisesRegex(TypeError, "not an acceptable base type"):
            type("BoolBase", (bool,), {})

    def test_metaclass_winner_and_conflict(self):
        class First(type):
            pass

        class Derived(First):
            pass

        class Other(type):
            pass

        class A(metaclass=First):
            pass

        class B(metaclass=Derived):
            pass

        class C(metaclass=Other):
            pass

        made = type("Winner", (A, B), {})
        self.assertIs(type(made), Derived)
        self.assertEqual(made.__mro__, (made, A, B, object))
        with self.assertRaisesRegex(TypeError, "metaclass conflict"):
            type("Conflict", (A, C), {})

    def test_custom_mro_rejects_inline_entries_in_order(self):
        for invalid, name in ((2147483647, "int"), (3.25, "float"), (None, "NoneType")):
            events = []

            class Bad(type):
                def mro(cls):
                    events.append(cls)
                    return [cls, invalid, object]

            with self.subTest(invalid=invalid), self.assertRaises(TypeError) as caught:
                Bad("Invalid", (), {})
            self.assertIn(f"mro() returned a non-class ('{name}')", str(caught.exception))
            self.assertEqual(len(events), 1)

    def test_custom_mro_empty_and_unsuitable_layout(self):
        events = []

        class Empty(type):
            def mro(cls):
                events.append("empty")
                return []

        with self.assertRaisesRegex(TypeError, "type MRO must not be empty"):
            Empty("EmptyMro", (), {})

        class Unsuitable(type):
            def mro(cls):
                events.append("layout")
                return [cls, int, object]

        with self.assertRaisesRegex(TypeError, "unsuitable layout"):
            Unsuitable("InvalidLayout", (), {})
        self.assertEqual(events, ["empty", "layout"])

    def test_custom_mro_conversion_error_is_not_replayed(self):
        events = []
        error = ValueError("selected MRO iterator")

        class Sequence:
            def __iter__(self):
                events.append("iter")
                raise error

        class Meta(type):
            def mro(cls):
                events.append("mro")
                return Sequence()

        with self.assertRaises(ValueError) as caught:
            Meta("ConversionError", (), {})
        self.assertIs(caught.exception, error)
        self.assertEqual(events, ["mro", "iter"])

    def test_custom_mro_result_retires_after_conversion(self):
        events = []

        class Sequence:
            def __init__(self, cls):
                self.cls = cls

            def __iter__(self):
                events.append("iter")
                return iter((self.cls, object))

            def __del__(self):
                events.append("retired")

        class Meta(type):
            def mro(cls):
                events.append("mro")
                return Sequence(cls)

        made = Meta("Selected", (), {})
        self.assertEqual(made.__mro__, (made, object))
        self.assertEqual(events, ["mro", "iter", "retired"])

    def test_custom_mro_can_omit_dominant_base(self):
        class Base:
            pass

        class Meta(type):
            def mro(cls):
                return [cls, object]

        made = Meta("OmittedBase", (Base,), {})
        self.assertIs(made.__base__, Base)
        self.assertEqual(made.__mro__, (made, object))
        self.assertFalse(issubclass(made, Base))
        self.assertIs(type(made()), made)


if __name__ == "__main__":
    unittest.main()
