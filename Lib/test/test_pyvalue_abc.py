import abc
import _abc
import unittest
import weakref


class ItemsMapping:
    def __init__(self, pairs):
        self.pairs = pairs
        self.items_calls = 0

    def items(self):
        self.items_calls += 1
        return self.pairs


class Subject:
    def __init__(self, mapping, bases=()):
        self.mapping = mapping
        self.bases = bases

    @property
    def __dict__(self):
        return self.mapping

    @property
    def __bases__(self):
        return self.bases


class AbstractFlag:
    def __init__(self, callback):
        self.callback = callback

    @property
    def __isabstractmethod__(self):
        return self.callback()


class TestABCAbstractMethodValues(unittest.TestCase):
    def test_ordinary_abstract_inheritance(self):
        class Base(metaclass=abc.ABCMeta):
            @abc.abstractmethod
            def method(self):
                pass

        class StillAbstract(Base):
            @abc.abstractmethod
            def method(self):
                pass

        _abc._abc_init(StillAbstract)
        self.assertEqual(StillAbstract.__abstractmethods__, frozenset({"method"}))

        class Concrete(Base):
            def method(self):
                pass

        _abc._abc_init(Concrete)
        self.assertEqual(Concrete.__abstractmethods__, frozenset())

    def test_inline_scalar_members(self):
        subject = Subject(ItemsMapping([
            ("integer", 1),
            ("float", 1.5),
            ("none", None),
        ]))
        _abc._abc_init(subject)
        self.assertEqual(subject.__abstractmethods__, frozenset())
        self.assertEqual(subject.mapping.items_calls, 1)

    def test_mutated_pair_keeps_selected_key_and_calls_once(self):
        pair = ["selected", None]
        calls = []
        mapping = ItemsMapping([
            pair,
            ["second", AbstractFlag(lambda: calls.append("second") or True)],
        ])

        def mutate_pair():
            calls.append("abstractness")
            pair[:] = ["replacement", 0]
            return True

        subject = Subject(mapping)
        pair[1] = AbstractFlag(mutate_pair)

        _abc._abc_init(subject)

        self.assertEqual(subject.__abstractmethods__, frozenset({"selected", "second"}))
        self.assertEqual(calls, ["abstractness", "second"])
        self.assertEqual(mapping.items_calls, 1)

    def test_mutation_failure_preserves_exception_and_stops(self):
        pair = ["selected", None]
        sentinel = RuntimeError("abstractness sentinel")
        calls = []
        mapping = ItemsMapping([pair])

        def fail_after_mutation():
            calls.append("abstractness")
            pair[:] = ["replacement", 0]
            raise sentinel

        mapping.pairs.append(["later", AbstractFlag(lambda: calls.append("later"))])
        subject = Subject(mapping)
        pair[1] = AbstractFlag(fail_after_mutation)

        with self.assertRaises(RuntimeError) as caught:
            _abc._abc_init(subject)

        self.assertIs(caught.exception, sentinel)
        self.assertEqual(calls, ["abstractness"])
        self.assertFalse(hasattr(subject, "__abstractmethods__"))
        self.assertFalse(hasattr(subject, "_abc_impl"))

    def test_items_list_clear_during_callback_stops_at_current_size(self):
        calls = []
        mapping = ItemsMapping([])

        def clear_pairs():
            calls.append("first")
            mapping.pairs.clear()
            return True

        mapping.pairs.extend([
            ["first", AbstractFlag(clear_pairs)],
            ["later", AbstractFlag(lambda: calls.append("later") or True)],
        ])
        subject = Subject(mapping)
        _abc._abc_init(subject)
        self.assertEqual(calls, ["first"])
        self.assertEqual(subject.__abstractmethods__, frozenset({"first"}))
        self.assertEqual(mapping.items_calls, 1)

    def test_items_list_replacement_during_callback_uses_current_index(self):
        calls = []
        mapping = ItemsMapping([])

        def replace_second_pair():
            calls.append("first")
            mapping.pairs[1] = [
                "replacement", AbstractFlag(lambda: calls.append("replacement") or True)
            ]
            return True

        mapping.pairs.extend([
            ["first", AbstractFlag(replace_second_pair)],
            ["old", AbstractFlag(lambda: calls.append("old") or True)],
        ])
        subject = Subject(mapping)
        _abc._abc_init(subject)
        self.assertEqual(calls, ["first", "replacement"])
        self.assertEqual(subject.__abstractmethods__, frozenset({"first", "replacement"}))
        self.assertEqual(mapping.items_calls, 1)

    def test_inherited_key_hash_runs_after_transient_value_retirement(self):
        events = []
        transient_ref = None
        abstractness_started = False

        class AbstractName(str):
            def __hash__(self):
                events.append("key_hash_after" if abstractness_started else "key_hash_before")
                if abstractness_started and transient_ref() is not None:
                    raise AssertionError("transient value still alive during key hashing")
                return hash(str(self))

        class TransientValue:
            @property
            def __isabstractmethod__(self):
                nonlocal abstractness_started
                abstractness_started = True
                events.append("abstractness")
                return True

            def __del__(self):
                events.append("value_retired")

        class Base:
            __abstractmethods__ = [AbstractName("inherited")]

        class InheritedSubject(Subject):
            def __getattr__(self, name):
                if name == "inherited":
                    value = TransientValue()
                    nonlocal transient_ref
                    transient_ref = weakref.ref(value)
                    return value
                raise AttributeError(name)

        subject = InheritedSubject(ItemsMapping([]), bases=(Base(),))
        _abc._abc_init(subject)
        self.assertEqual(
            [event for event in events if event != "key_hash_before"],
            ["abstractness", "value_retired", "key_hash_after"],
        )
        self.assertIsNone(transient_ref())
        self.assertEqual(subject.__abstractmethods__, frozenset({"inherited"}))

    def test_malformed_pair_and_noniterable_diagnostics(self):
        for pairs, message in (
            ([["only one"]], r"items\(\) returned item which size is not 2"),
            ([42], r"items\(\) returned non-iterable"),
        ):
            with self.subTest(pairs=pairs):
                subject = Subject(ItemsMapping(pairs))
                with self.assertRaisesRegex(TypeError, message):
                    _abc._abc_init(subject)
                self.assertFalse(hasattr(subject, "__abstractmethods__"))
                self.assertFalse(hasattr(subject, "_abc_impl"))

    def test_inherited_abstractmethods_getter_failure(self):
        sentinel = LookupError("inherited abstract getter")
        calls = []

        class Base:
            @property
            def __abstractmethods__(self):
                calls.append("lookup")
                raise sentinel

        subject = Subject(ItemsMapping([]), bases=(Base(),))
        with self.assertRaises(LookupError) as caught:
            _abc._abc_init(subject)
        self.assertIs(caught.exception, sentinel)
        self.assertEqual(calls, ["lookup"])
        self.assertFalse(hasattr(subject, "__abstractmethods__"))
        self.assertFalse(hasattr(subject, "_abc_impl"))


if __name__ == "__main__":
    unittest.main()
