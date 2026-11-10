"""Guarded canonical class values and their result-retirement transaction."""

import dis
import sys
import unittest
import weakref

from test.support import cpython_only, requires_specialization, reset_code, gc_collect
from test.support.import_helper import import_module

_testinternalcapi = import_module("_testinternalcapi")


def make_reader():
    def read(owner):
        return owner.selected
    return reset_code(read)


def warm(read, owner):
    for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
        read(owner)


def handlers(read):
    return [row[0] for row in _testinternalcapi.get_ir(read.__code__, True)]


@cpython_only
@requires_specialization
class TestDirectClassValue(unittest.TestCase):
    def assert_cached(self, read, *, metaclass=False):
        expected = ("LOAD_ATTR_CLASS_WITH_METACLASS_CHECK" if metaclass else
                    "LOAD_ATTR_CLASS")
        self.assertIn(expected, handlers(read))

    def test_canonical_and_object_values(self):
        class Meta(type):
            pass

        for meta in (type, Meta):
            for value in (1, -2147483648, 2147483647, 3.25, True, False,
                          None, [], {}, (1, 2)):
                with self.subTest(meta=meta, value=value):
                    cls = meta("Receiver", (), {"selected": value})
                    read = make_reader()
                    warm(read, cls)
                    self.assert_cached(read, metaclass=meta is Meta)
                    self.assertIs(read(cls), value)

    def test_inherited_mutation_and_descriptor_replacement(self):
        class Base:
            selected = 1

        class Other:
            selected = 2

        class Receiver(Base):
            pass

        class Descriptor:
            def __get__(self, owner, cls):
                return 23

        read = make_reader()
        warm(read, Receiver)
        self.assert_cached(read)
        Base.selected = [3]
        self.assertIs(read(Receiver), Base.selected)
        warm(read, Receiver)
        self.assert_cached(read)
        Receiver.__bases__ = (Other,)
        self.assertEqual(read(Receiver), 2)
        Other.selected = Descriptor()
        self.assertEqual(read(Receiver), 23)
        del Other.selected
        with self.assertRaises(AttributeError):
            read(Receiver)

    def test_metaclass_precedence_after_warmup(self):
        class Meta(type):
            pass

        class Receiver(metaclass=Meta):
            selected = 1

        class Descriptor:
            def __get__(self, owner, cls):
                return 24

            def __set__(self, owner, value):
                raise AssertionError("unexpected store")

        read = make_reader()
        warm(read, Receiver)
        self.assert_cached(read, metaclass=True)
        Meta.selected = Descriptor()
        self.assertEqual(read(Receiver), 24)
        del Meta.selected
        warm(read, Receiver)
        self.assert_cached(read, metaclass=True)
        Meta.__getattribute__ = lambda self, name: 25
        self.assertEqual(read(Receiver), 25)

    def test_plain_function_call_keeps_null_self(self):
        class Receiver:
            def selected(value):
                return value

        def invoke(cls, value):
            return cls.selected(value)

        reset_code(invoke)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            self.assertEqual(invoke(Receiver, 31), 31)
        self.assert_cached(invoke)
        Receiver.selected = 1
        with self.assertRaises(TypeError):
            invoke(Receiver, 31)

    def test_polymorphic_class_value_snapshots(self):
        class Meta(type):
            pass

        classes = [type("One", (), {"selected": 1}),
                   Meta("Two", (), {"selected": []}),
                   type("Three", (), {"selected": 3.25})]
        read = make_reader()
        warm(read, classes[0])
        for cls in classes[1:]:
            for _ in range(_testinternalcapi.SPECIALIZATION_COOLDOWN):
                self.assertIs(read(cls), cls.selected)
            self.assert_cached(read, metaclass=type(cls) is Meta)
        [site] = [i for i in dis.get_instructions(read) if i.opname == "LOAD_ATTR"]
        kind, state, _, _, rows = _testinternalcapi.get_feedback(
            read.__code__, site.offset)
        self.assertEqual((kind, state, len(rows)), ("ATTR", "POLY", 3))
        self.assertEqual({row[2] for row in rows}, {
            dis._specialized_opmap["LOAD_ATTR_CLASS"],
            dis._specialized_opmap["LOAD_ATTR_CLASS_WITH_METACLASS_CHECK"],
        })
        for cls in classes:
            cls.selected = [42]
            self.assertIs(read(cls), cls.selected)

    def test_result_survives_displaced_local_reentry(self):
        events = []

        class Payload:
            def __del__(self):
                events.append("payload-retired")

        class Receiver:
            selected = [Payload()]

        class Other:
            selected = [43]

        def read(cls, prior):
            prior = cls.selected
            return prior

        class Displaced:
            def __del__(self):
                del Receiver.selected
                events.append(read(Other, None)[0])

        reset_code(read)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            read(Receiver, None)
        self.assert_cached(read)
        result = read(Receiver, Displaced())
        self.assertEqual(events, [43])
        self.assertIsInstance(result[0], Payload)
        del result
        gc_collect()
        self.assertEqual(events, [43, "payload-retired"])


@cpython_only
@requires_specialization
class TestGuardedClassValue(unittest.TestCase):
    def assert_cached(self, read, *, slots=False):
        expected = ("LOAD_ATTR_NONDESCRIPTOR_NO_DICT" if slots else
                    "LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES")
        self.assertIn(expected, handlers(read))

    def test_canonical_values_and_object_identity(self):
        for slots in (False, True):
            for value in (1, 2147483647, 3.25, True, False, None, [], {}, (1, 2)):
                with self.subTest(slots=slots, value=value):
                    namespace = {"selected": value}
                    if slots:
                        namespace["__slots__"] = ()
                    cls = type("Receiver", (), namespace)
                    owner = cls()
                    read = make_reader()
                    warm(read, owner)
                    self.assert_cached(read, slots=slots)
                    self.assertIs(read(owner), value)

    def test_class_replacement_and_deletion(self):
        class Receiver:
            selected = 1

        owner, read = Receiver(), make_reader()
        warm(read, owner)
        self.assert_cached(read)
        Receiver.selected = 3.25
        self.assertEqual(read(owner), 3.25)
        del Receiver.selected
        with self.assertRaises(AttributeError):
            read(owner)

    def test_instance_shadow_and_shared_key_on_another_instance(self):
        for other_instance in (False, True):
            with self.subTest(other_instance=other_instance):
                class Receiver:
                    selected = 1

                owner, other, read = Receiver(), Receiver(), make_reader()
                warm(read, owner)
                self.assert_cached(read)
                target = other if other_instance else owner
                target.selected = 42
                self.assertEqual(read(owner), 1 if other_instance else 42)
                del target.selected
                self.assertEqual(read(owner), 1)
                # The name now exists in shared keys; this conservative class
                # cache must continue to honor this receiver's own shadow.
                owner.selected = 43
                self.assertEqual(read(owner), 43)

    def test_materialized_and_replaced_instance_dict(self):
        for materialize_before_warm in (False, True):
            with self.subTest(materialize_before_warm=materialize_before_warm):
                class Receiver:
                    selected = 1

                owner, read = Receiver(), make_reader()
                if materialize_before_warm:
                    vars(owner)
                warm(read, owner)
                self.assert_cached(read)
                dictionary = vars(owner)
                self.assertEqual(read(owner), 1)
                dictionary["selected"] = 2
                self.assertEqual(read(owner), 2)
                del dictionary["selected"]
                self.assertEqual(read(owner), 1)
                owner.__dict__ = {"selected": 3}
                self.assertEqual(read(owner), 3)
                owner.__dict__ = {}
                self.assertEqual(read(owner), 1)

    def test_inheritance_and_mro_mutation(self):
        class Base:
            selected = 1

        class Other:
            selected = 2

        class Receiver(Base):
            pass

        owner, read = Receiver(), make_reader()
        warm(read, owner)
        self.assert_cached(read)
        Base.selected = 3.25
        self.assertEqual(read(owner), 3.25)
        warm(read, owner)
        Receiver.__bases__ = (Other,)
        self.assertEqual(read(owner), 2)
        del Other.selected
        with self.assertRaises(AttributeError):
            read(owner)

    def test_descriptors_and_custom_hooks_replace_cached_scalar(self):
        class DataDescriptor:
            def __get__(self, owner, cls):
                return 21

            def __set__(self, owner, value):
                raise AssertionError("unexpected store")

        class NonDataDescriptor:
            def __get__(self, owner, cls):
                return 22

        for replacement in (DataDescriptor(), NonDataDescriptor()):
            with self.subTest(replacement=replacement):
                class Receiver:
                    selected = 1

                owner, read = Receiver(), make_reader()
                warm(read, owner)
                self.assert_cached(read)
                Receiver.selected = replacement
                self.assertEqual(read(owner), 21 if isinstance(
                    replacement, DataDescriptor) else 22)

        class Receiver:
            selected = 1

        owner, read = Receiver(), make_reader()
        warm(read, owner)
        self.assert_cached(read)
        Receiver.__getattribute__ = lambda self, name: 23
        self.assertEqual(read(owner), 23)
        del Receiver.__getattribute__
        Receiver.__getattr__ = lambda self, name: 24
        del Receiver.selected
        self.assertEqual(read(owner), 24)

    def test_descriptor_exception_after_invalidation(self):
        marker = ValueError("selected descriptor")

        class Descriptor:
            def __get__(self, owner, cls):
                raise marker

        class Receiver:
            selected = 1

        owner, read = Receiver(), make_reader()
        warm(read, owner)
        self.assert_cached(read)
        Receiver.selected = Descriptor()
        with self.assertRaises(ValueError) as raised:
            read(owner)
        self.assertIs(raised.exception, marker)

    def test_mutable_descriptor_type_remains_generic(self):
        class Value:
            pass

        class Receiver:
            selected = Value()

        owner, read = Receiver(), make_reader()
        warm(read, owner)
        self.assertIs(read(owner), Receiver.selected)
        self.assertFalse(any(name.startswith("LOAD_ATTR_NONDESCRIPTOR")
                             for name in handlers(read)))
        Value.__get__ = lambda self, instance, cls: 31
        self.assertEqual(read(owner), 31)

    def test_polymorphic_snapshots_keep_current_values(self):
        read = make_reader()
        classes = [type(f"Receiver{i}", (), {"selected": value})
                   for i, value in enumerate((1, [], 3.25))]
        owners = [cls() for cls in classes]
        warm(read, owners[0])
        self.assert_cached(read)
        for owner in owners[1:]:
            for _ in range(_testinternalcapi.SPECIALIZATION_COOLDOWN):
                self.assertIs(read(owner), type(owner).selected)
            self.assert_cached(read)
        [site] = [instruction for instruction in dis.get_instructions(read)
                  if instruction.opname == "LOAD_ATTR"]
        kind, state, _, _, rows = _testinternalcapi.get_feedback(
            read.__code__, site.offset)
        self.assertEqual((kind, state, len(rows)), ("ATTR", "POLY", 3))
        self.assertTrue(all(row[2] == dis._specialized_opmap[
            "LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES"] for row in rows))
        for owner in owners:
            self.assertIs(read(owner), type(owner).selected)

    def test_guarded_attribute_feedback_capacity(self):
        class Receiver:
            selected = 1

        namespace = {}
        exec("def read(owner):\n" +
             "    selected = owner.selected\n" * 700 +
             "    return selected\n", namespace)
        read = namespace["read"]
        sites = [instruction for instruction in dis.get_instructions(read)
                 if instruction.opname == "LOAD_ATTR"]
        private = [site for site in sites if site.fb != 0]
        shared = [site for site in sites if site.fb == 0]
        # Two header units, then 682 six-unit attr entries. The remaining
        # two units cannot hold another attr entry in the 4096-unit vector.
        self.assertEqual([site.fb for site in private], list(range(2, 4094, 6)))
        self.assertEqual(len(shared), 18)
        total = int.from_bytes(read.__code__.co_fbinit[:4], sys.byteorder) & 0xFFFF
        self.assertEqual(total, 4094)
        owner = Receiver()
        warm(read, owner)
        words = handlers(read)
        self.assertEqual({words[site.offset] for site in private},
                         {"LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES"})
        self.assertEqual({words[site.offset] for site in shared}, {"LOAD_ATTR"})
        self.assertEqual(len(read.__code__._co_feedback), total * 8)
        Receiver.selected = 2
        self.assertEqual(read(owner), 2)

    def test_reentrant_retirement_rewrites_feedback(self):
        armed = False
        events = []
        marker, replacement = object(), object()

        class Receiver:
            selected = [marker]

            def __del__(self):
                nonlocal armed
                if armed:
                    armed = False
                    Receiver.selected = [replacement]
                    events.append(read()[0])

        def read():
            return Receiver().selected

        reset_code(read)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            read()
        self.assert_cached(read)
        armed = True
        result = read()
        self.assertIs(result[0], marker)
        self.assertEqual(events, [replacement])
        self.assertIs(read()[0], replacement)

    def test_selected_object_survives_receiver_retirement(self):
        for slots in (False, True):
            for destination in ("return", "local", "discard", "alias"):
                for mutation in ("delete", "replace"):
                    with self.subTest(slots=slots, destination=destination,
                                      mutation=mutation):
                        self.check_retirement(slots, destination, mutation)

    def check_retirement(self, slots, destination, mutation):
        events = []
        armed = False

        class Payload:
            def __del__(self):
                events.append("payload-retired")

        def retire(owner):
            nonlocal armed
            if not armed:
                return
            armed = False
            if mutation == "delete":
                del cls.selected
            else:
                cls.selected = ["replacement"]
            events.append(("receiver-retired", reference() is not None))

        namespace = {"selected": [Payload()], "__del__": retire}
        if slots:
            namespace["__slots__"] = ()
        cls = type("Receiver", (), namespace)
        del namespace  # The class must be the selected list's only owner.
        reference = weakref.ref(cls.selected[0])
        if destination == "return":
            def read():
                return cls().selected
        elif destination == "local":
            def read():
                selected = cls().selected
                return selected
        elif destination == "discard":
            def read():
                cls().selected
        else:
            def read():
                owner = cls()
                owner = owner.selected
                return owner
        reset_code(read)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            read()
        self.assert_cached(read, slots=slots)
        armed = True
        result = read()
        gc_collect()
        self.assertEqual(events[0], ("receiver-retired", True))
        if destination != "discard":
            self.assertIs(reference(), result[0])
            self.assertEqual(events, [("receiver-retired", True)])
        del result
        gc_collect()
        self.assertIsNone(reference())
        self.assertEqual(events, [("receiver-retired", True), "payload-retired"])


if __name__ == "__main__":
    unittest.main()
