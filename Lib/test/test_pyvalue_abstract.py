"""Canonical tuple and special-method boundaries in abstract predicates."""

import sys
import unittest
import weakref


class AbstractPredicateTests(unittest.TestCase):
    def test_plain_classes_tuples_and_unions(self):
        self.assertTrue(isinstance(1, (str, int)))
        self.assertTrue(isinstance(1, str | int))
        self.assertFalse(isinstance([], str | int))
        self.assertTrue(issubclass(bool, (str, int)))
        self.assertTrue(issubclass(bool, str | int))
        self.assertFalse(issubclass(list, str | int))
        self.assertFalse(isinstance(1, ()))
        self.assertFalse(issubclass(int, ()))

    def test_scalar_metaclass_methods_are_not_callable(self):
        for name, operation, argument in (
                ("__instancecheck__", isinstance, object()),
                ("__subclasscheck__", issubclass, object)):
            for value in (None, True, 2147483647, 3.5):
                with self.subTest(name=name, value=value):
                    meta = type("Meta", (type,), {name: value})
                    checked = meta("Checked", (), {})
                    with self.assertRaisesRegex(TypeError, "not callable"):
                        operation(argument, checked)

    def test_tuple_predicates_preserve_order_and_short_circuit(self):
        for name, operation, argument in (
                ("__instancecheck__", isinstance, object()),
                ("__subclasscheck__", issubclass, str)):
            events = []
            marker = ValueError("third predicate")

            def checked(label, answer):
                def predicate(cls, value):
                    events.append((label, value))
                    if answer is marker:
                        raise marker
                    return answer
                meta = type("Meta", (type,), {name: predicate})
                return meta(label, (), {})

            first = checked("first", False)
            second = checked("second", True)
            third = checked("third", marker)
            self.assertTrue(operation(argument, (first, (second, third))))
            self.assertEqual(events, [("first", argument), ("second", argument)])
            events.clear()
            with self.assertRaises(ValueError) as caught:
                operation(argument, (third, second))
            self.assertIs(caught.exception, marker)
            self.assertEqual(events, [("third", argument)])
            self.assertTrue(operation(argument, (object, 2147483647)))
            with self.assertRaises(TypeError):
                operation(argument, (2147483647, object))

    def test_tuple_subclass_hooks_are_bypassed(self):
        class Targets(tuple):
            def __getitem__(self, index):
                raise AssertionError("Python tuple indexing")

            def __iter__(self):
                raise AssertionError("Python tuple iteration")

            def __len__(self):
                raise AssertionError("Python tuple length")

        self.assertTrue(isinstance(1, Targets((str, int))))
        self.assertTrue(issubclass(bool, Targets((str, int))))

        class Pseudo:
            __bases__ = Targets((int,))

        self.assertTrue(issubclass(Pseudo(), int))

    def test_custom_descriptor_binds_exactly_one_argument(self):
        for name, operation, argument in (
                ("__instancecheck__", isinstance, object()),
                ("__subclasscheck__", issubclass, object)):
            events = []

            class Descriptor:
                def __get__(self, owner, owner_type):
                    events.append(("bind", owner, owner_type))

                    def predicate(value):
                        events.append(("call", value))
                        return True
                    return predicate

            meta = type("Meta", (type,), {name: Descriptor()})
            checked = meta("Checked", (), {})
            self.assertTrue(operation(argument, checked))
            self.assertEqual(events, [("bind", checked, meta), ("call", argument)])

    def test_descriptor_error_identity(self):
        marker = ValueError("descriptor lookup")

        class Descriptor:
            def __get__(self, owner, owner_type):
                raise marker

        for name, operation, argument in (
                ("__instancecheck__", isinstance, object()),
                ("__subclasscheck__", issubclass, object)):
            meta = type("Meta", (type,), {name: Descriptor()})
            checked = meta("Checked", (), {})
            with self.assertRaises(ValueError) as caught:
                operation(argument, checked)
            self.assertIs(caught.exception, marker)

    def test_result_survives_method_retirement(self):
        for name, operation, argument in (
                ("__instancecheck__", isinstance, object()),
                ("__subclasscheck__", issubclass, object)):
            events, selected = [], []

            class Result:
                def __bool__(self):
                    events.append("truth")
                    return True

                def __del__(self):
                    events.append("result-retire")

            class Predicate:
                def __call__(self, value):
                    self_test.assertIs(selected[0](), self)
                    events.append("call")
                    setattr(meta, name, None)
                    return Result()

                def __del__(self):
                    events.append("method-retire")

            class Descriptor:
                def __get__(self, owner, owner_type):
                    predicate = Predicate()
                    selected.append(weakref.ref(predicate))
                    return predicate

            self_test = self
            meta = type("Meta", (type,), {name: Descriptor()})
            checked = meta("Checked", (), {})
            self.assertTrue(operation(argument, checked))
            self.assertIsNone(selected[0]())
            self.assertEqual(events, ["call", "method-retire", "truth", "result-retire"])

    def test_truth_error_identity_and_result_retirement(self):
        for name, operation, argument in (
                ("__instancecheck__", isinstance, object()),
                ("__subclasscheck__", issubclass, object)):
            marker = ValueError("predicate truth")
            events = []

            class Result:
                def __bool__(self):
                    events.append("truth")
                    raise marker

                def __del__(self):
                    events.append("retire")

            def predicate(cls, value):
                return Result()

            meta = type("Meta", (type,), {name: predicate})
            checked = meta("Checked", (), {})
            with self.assertRaises(ValueError) as caught:
                operation(argument, checked)
            self.assertIs(caught.exception, marker)
            # The exception traceback owns the truth frame and its self.
            marker.__traceback__ = None
            self.assertEqual(events, ["truth", "retire"])

    def test_fresh_single_base_and_multiple_base_views(self):
        events = []

        class Intermediate:
            @property
            def __bases__(self):
                events.append("intermediate")
                return (int,)

            def __del__(self):
                events.append("retire")

        class Derived:
            @property
            def __bases__(self):
                return (Intermediate(),)

        self.assertTrue(issubclass(Derived(), int))
        self.assertIn("intermediate", events)
        self.assertEqual(events[-1], "retire")

        class Multiple:
            __bases__ = (str, Derived())

        self.assertTrue(issubclass(Multiple(), int))
        self.assertFalse(issubclass(Multiple(), list))

    def test_bases_attribute_policy_and_scalar_children(self):
        class Invalid:
            __bases__ = 2147483647

        with self.assertRaises(TypeError):
            issubclass(Invalid(), int)
        with self.assertRaises(TypeError):
            isinstance(object(), Invalid())

        class ScalarChild:
            __bases__ = (2147483647,)

        self.assertFalse(issubclass(ScalarChild(), int))

        marker = ValueError("bases lookup")

        class Throwing:
            @property
            def __bases__(self):
                raise marker

        with self.assertRaises(ValueError) as caught:
            issubclass(Throwing(), int)
        self.assertIs(caught.exception, marker)

        class Missing:
            @property
            def __bases__(self):
                raise AttributeError("missing bases")

        with self.assertRaises(TypeError):
            issubclass(Missing(), int)

    def test_single_base_walk_uses_bounded_python_stack(self):
        class Node:
            def __init__(self, parent):
                self.parent = parent

            @property
            def __bases__(self):
                return (self.parent,)

        old_limit = sys.getrecursionlimit()
        try:
            sys.setrecursionlimit(200)
            derived = int
            for _ in range(400):
                derived = Node(derived)
            self.assertTrue(issubclass(derived, int))
        finally:
            sys.setrecursionlimit(old_limit)

    def test_recursive_predicate_error_balances_recursion(self):
        for name, operation, argument in (
                ("__instancecheck__", isinstance, object()),
                ("__subclasscheck__", issubclass, object)):
            def predicate(cls, value):
                return operation(value, cls)

            meta = type("Meta", (type,), {name: predicate})
            checked = meta("Checked", (), {})
            with self.assertRaises(RecursionError):
                operation(argument, checked)
            setattr(meta, name, lambda cls, value: True)
            for _ in range(20):
                self.assertTrue(operation(argument, checked))


if __name__ == "__main__":
    unittest.main()
