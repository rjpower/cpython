"""Owning dictionary value providers through their real set consumers.

These tests need the remaining dictionary mutation/storage port. They do not
exercise or redefine the legacy borrowed object getter APIs.
"""

import gc
import sys
import unittest
import weakref

from test.support import import_helper


class DictValueProviderTests(unittest.TestCase):
    def test_set_update_mixed_keys_and_holes(self):
        marker = object()
        mapping = {1: None, -2147483648: marker, 3.25: [],
                   "text": 2147483647, marker: 2.5}
        del mapping[1]
        mapping[17] = object()
        result = set()
        result.update(mapping)
        self.assertEqual(result, {-2147483648, 3.25, "text", marker, 17})
        mapping.clear()
        self.assertIn(marker, result)
        self.assertIn(3.25, result)

    def test_set_update_unicode_and_split_order(self):
        class Instance:
            pass

        first = Instance()
        second = Instance()
        for instance in (first, second):
            instance.alpha = 1234567
            instance.beta = object()
            instance.gamma = 2.5
        del first.beta
        first.beta = 2147483647
        for mapping in (first.__dict__, {"alpha": 0, "gamma": 1,
                                         "beta": 2}):
            with self.subTest(mapping=mapping):
                result = set()
                result.update(mapping)
                self.assertEqual(result, {"alpha", "beta", "gamma"})

    def test_difference_present_missing_and_scalar_values(self):
        # Keep |set| <= 4*|dict| so set_difference uses known-hash lookup.
        for value in (2147483647, -2147483648, 2.5, object(), []):
            with self.subTest(value=value):
                mapping = {3.25: value, "present": value}
                original = {3.25, "present", 17, "missing"}
                self.assertEqual(original.difference(mapping), {17, "missing"})
                self.assertEqual(original, {3.25, "present", 17, "missing"})
                self.assertEqual(mapping["present"], value)
                if type(value) not in (int, float):
                    self.assertIs(mapping["present"], value)

    def test_difference_restarts_after_comparison_clear(self):
        mapping = {}
        calls = []

        class Key:
            def __init__(self, stored):
                self.stored = stored

            def __hash__(self):
                return 17

            def __eq__(self, other):
                if self.stored:
                    calls.append("compare")
                    mapping.clear()
                return False

            def __del__(self):
                if self.stored:
                    calls.append("retire")
                    mapping["after"] = 2147483647

        stored = Key(True)
        query = Key(False)
        mapping[stored] = object()
        del stored
        original = {query}
        result = original.difference(mapping)
        self.assertEqual(calls, ["compare", "retire"])
        self.assertEqual(result, original)
        self.assertEqual(mapping, {"after": 2147483647})

    def test_difference_comparison_error_and_recovery(self):
        class LookupError(Exception):
            pass

        class Stored:
            def __hash__(self):
                return 23

            def __eq__(self, other):
                raise LookupError("dictionary comparison")

        class Query:
            def __hash__(self):
                return 23

        mapping = {Stored(): 2147483647}
        original = {Query()}
        with self.assertRaisesRegex(LookupError, "dictionary comparison"):
            original.difference(mapping)
        self.assertEqual(len(original), 1)
        mapping.clear()
        self.assertEqual(original.difference(mapping), original)

    def test_yielded_key_owner_survives_source_clear(self):
        class Key:
            pass

        key = Key()
        reference = weakref.ref(key)
        mapping = {key: 2147483647}
        result = set()
        result.update(mapping)
        del key
        mapping.clear()
        gc.collect()
        self.assertIsNotNone(reference())
        result.clear()
        gc.collect()
        self.assertIsNone(reference())


class DictValueMutationTests(unittest.TestCase):
    def test_internal_pop_boxing_oom_keeps_item(self):
        capi = import_helper.import_module("_testinternalcapi")
        self.assertIsNone(capi.dict_value_pop_boxing_oom("target"))

    def test_pop_and_conditional_delete_compare_once(self):
        capi = import_helper.import_module("_testinternalcapi")
        for conditional in (False, True):
            with self.subTest(conditional=conditional):
                calls = []

                class Key:
                    def __hash__(self):
                        return 13

                    def __eq__(self, other):
                        calls.append("compare")
                        if len(calls) != 1:
                            raise RuntimeError("user equality was replayed")
                        return isinstance(other, Key)

                mapping = capi.dict_value_seed(Key())
                query = Key()
                if conditional:
                    self.assertEqual(capi.dict_value_delete_if((mapping, query)), 1)
                else:
                    self.assertEqual(mapping.pop(query), 2147483647)
                self.assertEqual(calls, ["compare"])
                self.assertEqual(mapping, {})

    def test_growth_replace_delete_and_order(self):
        mapping = {}
        for key in range(100):
            mapping[key] = key + 0.25
        for key in range(0, 100, 2):
            del mapping[key]
        for key in range(1, 100, 2):
            mapping[key] = 2147483647
        mapping["unicode"] = -2147483648
        self.assertEqual(list(mapping), [*range(1, 100, 2), "unicode"])
        self.assertEqual(set(mapping), {*range(1, 100, 2), "unicode"})
        self.assertTrue(all(mapping[key] == 2147483647
                            for key in range(1, 100, 2)))
        self.assertEqual(mapping["unicode"], -2147483648)

    def test_replacement_published_before_finalizer_reentry(self):
        mapping = {}
        observations = []

        class Old:
            def __del__(self):
                observations.append(mapping["target"])
                for key in range(40):
                    mapping[key] = key + 0.5

        mapping["target"] = Old()
        mapping["target"] = 2147483647
        self.assertEqual(observations, [2147483647])
        self.assertEqual(mapping["target"], 2147483647)
        self.assertEqual(mapping[39], 39.5)

    def test_deletion_published_before_finalizer_reentry(self):
        mapping = {}
        observations = []

        class Old:
            def __del__(self):
                observations.append("target" not in mapping)
                mapping["target"] = 2.5

        mapping["target"] = Old()
        del mapping["target"]
        self.assertEqual(observations, [True])
        self.assertEqual(mapping, {"target": 2.5})

    def test_owned_pop_scalar_and_object_results(self):
        class Value:
            pass

        value = Value()
        reference = weakref.ref(value)
        mapping = {"object": value, "scalar": 2147483647}
        del value
        result = mapping.pop("object")
        self.assertIs(result, reference())
        self.assertNotIn("object", mapping)
        self.assertEqual(mapping.pop("scalar"), 2147483647)
        self.assertEqual(mapping, {})
        del result
        gc.collect()
        self.assertIsNone(reference())
        with self.assertRaises(KeyError):
            mapping.pop("absent")

    def test_unraisable_hook_refills_dictionary_before_store(self):
        testcapi = import_helper.import_module("_testcapi")
        mapping = {index: index for index in range(5)}
        watcher = testcapi.add_dict_watcher(1)  # Real error-reporting watcher.
        testcapi.watch_dict(watcher, mapping)
        errors = []
        original_hook = sys.unraisablehook

        def refill(event):
            errors.append(str(event.exc_value))
            testcapi.unwatch_dict(watcher, mapping)
            mapping.clear()
            for key in range(40):
                mapping[key] = key + 0.5

        try:
            # The watcher itself only reports an error. Its unraisable hook
            # performs the legal reentry, with the failing watcher detached.
            sys.unraisablehook = refill
            mapping["target"] = 2147483647
        finally:
            sys.unraisablehook = original_hook
            testcapi.unwatch_dict(watcher, mapping)
            testcapi.clear_dict_watcher(watcher)
        self.assertEqual(errors, ["boom!"])
        self.assertEqual(mapping["target"], 2147483647)
        self.assertEqual(mapping[39], 39.5)
        self.assertEqual(len(mapping), 41)


    def test_copy_combined_holes_and_split_owners(self):
        class Value:
            pass

        value = Value()
        reference = weakref.ref(value)
        mapping = {index: index + 0.5 for index in range(100)}
        mapping["owner"] = value
        for index in range(90):
            del mapping[index]
        copied = mapping.copy()
        self.assertEqual(list(copied), list(mapping))
        mapping.clear()
        del value
        self.assertIs(copied["owner"], reference())
        self.assertEqual(copied[99], 99.5)
        copied.clear()
        gc.collect()
        self.assertIsNone(reference())

        class Instance:
            pass

        instance = Instance()
        instance.alpha = 2147483647
        instance.beta = -2147483648
        instance.gamma = object()
        split = instance.__dict__
        copied = split.copy()
        instance.__dict__ = {"replacement": 2.5}
        self.assertEqual(list(copied), ["alpha", "beta", "gamma"])
        self.assertEqual(copied["alpha"], 2147483647)
        self.assertEqual(copied["beta"], -2147483648)
        self.assertIs(copied["gamma"], split["gamma"])

    def test_merge_uses_cached_dictionary_hash_and_mapping_condition(self):
        capi = import_helper.import_module("_testinternalcapi")
        for mode in (0, 1, 2):
            with self.subTest(mode=mode):
                calls = []

                class Key:
                    def __hash__(self):
                        calls.append("hash")
                        if len(calls) > 1:
                            raise RuntimeError("cached source hash was discarded")
                        return 17

                key = Key()
                source = {key: 2147483647}
                target = {0: 0}
                capi.dict_value_merge_mode((target, source, mode))
                self.assertEqual(calls, ["hash"])
                self.assertIs(list(target)[1], key)

        for mode in (0, 2):
            with self.subTest(mapping_mode=mode):
                target = {}

                class Mapping:
                    def keys(self):
                        return ["target"]

                    def __getitem__(self, key):
                        target[key] = 2147483647
                        return -2147483648

                if mode == 2:
                    with self.assertRaises(KeyError):
                        capi.dict_value_merge_mode((target, Mapping(), mode))
                else:
                    capi.dict_value_merge_mode((target, Mapping(), mode))
                self.assertEqual(target, {"target": 2147483647})

    def test_cloned_merge_reentry_preserves_requested_mode(self):
        capi = import_helper.import_module("_testinternalcapi")
        testcapi = import_helper.import_module("_testcapi")
        for mode in (0, 2):
            with self.subTest(mode=mode):
                target = {}
                watcher = testcapi.add_dict_watcher(1)
                testcapi.watch_dict(watcher, target)
                errors = []
                original = sys.unraisablehook

                def populate(event):
                    errors.append(str(event.exc_value))
                    testcapi.unwatch_dict(watcher, target)
                    target["target"] = 2147483647

                try:
                    sys.unraisablehook = populate
                    source = {"target": -2147483648, "other": 2.5}
                    if mode == 2:
                        with self.assertRaises(KeyError):
                            capi.dict_value_merge_mode((target, source, mode))
                    else:
                        capi.dict_value_merge_mode((target, source, mode))
                finally:
                    sys.unraisablehook = original
                    testcapi.unwatch_dict(watcher, target)
                    testcapi.clear_dict_watcher(watcher)
                self.assertEqual(errors, ["boom!"])
                self.assertEqual(target["target"], 2147483647)
                if mode == 0:
                    self.assertEqual(target["other"], 2.5)

    def test_setdefault_owned_results_and_single_comparison(self):
        capi = import_helper.import_module("_testinternalcapi")
        self.assertIsNone(capi.dict_value_setdefault_boxing_oom("target"))
        marker = object()
        mapping = {}
        self.assertIs(mapping.setdefault("owner", marker), marker)
        self.assertEqual(mapping.setdefault("scalar", 2147483647), 2147483647)
        self.assertEqual(mapping.setdefault("scalar", -2147483648), 2147483647)
        self.assertIsNone(mapping.setdefault("none"))
        calls = []

        class Key:
            def __hash__(self):
                return 13

            def __eq__(self, other):
                calls.append("compare")
                if len(calls) > 1:
                    raise RuntimeError("setdefault repeated equality without mutation")
                return isinstance(other, Key)

        mapping = {Key(): 2147483647}
        self.assertEqual(mapping.setdefault(Key(), -2147483648), 2147483647)
        self.assertEqual(calls, ["compare"])

    def test_popitem_preserves_lifo_holes_and_callback_appends(self):
        mapping = {index: index + 0.5 for index in range(10)}
        del mapping[8]
        self.assertEqual(mapping.popitem(), (9, 9.5))
        self.assertEqual(mapping.popitem(), (7, 7.5))
        mapping.clear()
        with self.assertRaises(KeyError):
            mapping.popitem()

        testcapi = import_helper.import_module("_testcapi")
        mapping = {"first": 0, "tail": 2147483647}
        watcher = testcapi.add_dict_watcher(1)
        testcapi.watch_dict(watcher, mapping)
        errors = []
        original = sys.unraisablehook

        def append(event):
            errors.append(str(event.exc_value))
            testcapi.unwatch_dict(watcher, mapping)
            mapping["later"] = -2147483648

        try:
            sys.unraisablehook = append
            self.assertEqual(mapping.popitem(), ("tail", 2147483647))
        finally:
            sys.unraisablehook = original
            testcapi.unwatch_dict(watcher, mapping)
            testcapi.clear_dict_watcher(watcher)
        self.assertEqual(errors, ["boom!"])
        self.assertEqual(mapping.popitem(), ("later", -2147483648))
        self.assertEqual(mapping, {"first": 0})

    def test_managed_store_and_zero_rc_detach_preserve_owners(self):
        from test import support

        capi = import_helper.import_module("_testinternalcapi")

        class ClearInstance:
            pass

        clearing = ClearInstance()
        clearing.value = 2147483647
        with support.catch_unraisable_exception() as caught:
            self.assertIsNone(capi.dict_value_clear_preserves_exception(clearing))
            self.assertIs(caught.unraisable.exc_type, MemoryError)

        class Instance:
            pass

        instance = Instance()
        observations = []

        class Old:
            def __del__(self):
                observations.append(instance.value)
                instance.extra = -2147483648

        instance.value = Old()
        instance.value = 2147483647
        self.assertEqual(observations, [2147483647])
        self.assertEqual(instance.extra, -2147483648)
        del instance.extra
        self.assertFalse(hasattr(instance, "extra"))
        instance.value = None
        self.assertIsNone(instance.value)

        class Value:
            pass

        value = Value()
        reference = weakref.ref(value)
        instance.value = value
        saved = instance.__dict__
        instance_reference = weakref.ref(instance)
        del value, instance
        gc.collect()
        self.assertIsNone(instance_reference())
        self.assertIs(saved["value"], reference())
        saved.clear()
        gc.collect()
        self.assertIsNone(reference())


class DictValueReaderTests(unittest.TestCase):
    def test_get_preserves_values_defaults_and_subclass_semantics(self):
        class Mapping(dict):
            def __missing__(self, key):
                raise AssertionError("get must not call __missing__")

        marker = object()
        values = (None, 2147483647, -2147483648, 2.5, marker, [])
        for mapping_type in (dict, Mapping):
            for key in (17, 3.25, "present", marker):
                for value in values:
                    with self.subTest(mapping=mapping_type, key=key, value=value):
                        mapping = mapping_type({key: value})
                        self.assertEqual(mapping.get(key), value)
                        self.assertEqual(dict.get(mapping, key), value)
                        if value is marker or type(value) is list:
                            self.assertIs(mapping.get(key), value)
                            self.assertIs(dict.get(mapping, key), value)
                        self.assertIs(mapping.get("missing", marker), marker)
                        self.assertIsNone(mapping.get("missing"))

    def test_get_hash_mutation_keeps_dictionary_and_query_alive(self):
        events = []
        owners = []

        class Mapping(dict):
            def __del__(self):
                events.append("mapping retired")
                self.clear()

        class Query:
            def __hash__(self):
                mapping = owners.pop()
                mapping.clear()
                mapping[23] = "after hash"
                gc.collect()
                return 23

            def __eq__(self, other):
                return other == 23

            def __del__(self):
                events.append("query retired")

        owners.append(Mapping({"before": object()}))
        self.assertEqual(owners[0].get(Query()), "after hash")
        self.assertEqual(owners, [])
        gc.collect()
        self.assertCountEqual(events, ["mapping retired", "query retired"])

    def test_get_equality_mutation_restarts_and_allows_reentry(self):
        mapping = {}
        events = []

        class Stored:
            def __hash__(self):
                return 23

            def __eq__(self, other):
                mapping.clear()
                mapping[23] = "replacement"
                events.append("compare")
                gc.collect()
                return False

            def __del__(self):
                events.append(mapping.get(23))

        mapping[Stored()] = object()
        self.assertEqual(mapping.get(23), "replacement")
        self.assertCountEqual(events, ["compare", "replacement"])
        self.assertEqual(mapping, {23: "replacement"})

    def test_get_result_survives_input_owner_cleanup(self):
        events = []
        owners = []

        class Value:
            pass

        class Mapping(dict):
            def __del__(self):
                self.clear()
                events.append("mapping retired")
                gc.collect()

        class Query:
            def __hash__(self):
                owners.clear()
                return 17

            def __eq__(self, other):
                return other == 17

            def __del__(self):
                events.append("query retired")

        value = Value()
        reference = weakref.ref(value)
        owners.append(Mapping({17: value}))
        del value
        result = owners[0].get(Query())
        self.assertIs(result, reference())
        self.assertIsNotNone(result)
        gc.collect()
        self.assertCountEqual(events, ["mapping retired", "query retired"])
        del result
        gc.collect()
        self.assertIsNone(reference())

    def test_get_default_survives_hash_callback(self):
        class Default:
            pass

        defaults = [Default()]
        reference = weakref.ref(defaults[0])

        class Query:
            def __hash__(self):
                defaults.clear()
                gc.collect()
                return 17

        result = {}.get(Query(), defaults[0])
        self.assertIs(result, reference())
        self.assertIsNotNone(result)
        del result
        gc.collect()
        self.assertIsNone(reference())

    def test_get_preserves_hash_and_equality_errors(self):
        events = []

        class Query:
            def __hash__(self):
                raise LookupError("hash marker")

            def __del__(self):
                events.append({17: "reentered"}.get(17))

        with self.assertRaisesRegex(LookupError, "hash marker"):
            {}.get(Query())
        gc.collect()
        self.assertEqual(events, ["reentered"])

        class Stored:
            def __hash__(self):
                return 17

            def __eq__(self, other):
                raise LookupError("equality marker")

        mapping = {Stored(): object()}
        with self.assertRaisesRegex(LookupError, "equality marker"):
            mapping.get(17)
        self.assertEqual(len(mapping), 1)
        with self.assertRaises(TypeError):
            mapping.get([])

    def test_owned_reader_keeps_entry_error(self):
        capi = import_helper.import_module("_testinternalcapi")
        marker = object()
        for mapping, key, status, expected in (
            ({"present": marker}, "present", 1, marker),
            ({"present": 2147483647}, "present", 1, 2147483647),
            ({"present": marker}, "missing", 0, None),
        ):
            with self.subTest(status=status, expected=expected):
                actual, error, result = capi.dict_value_getref_entry((mapping, key))
                self.assertEqual(actual, status)
                self.assertIs(type(error), ValueError)
                self.assertEqual(str(error), "entry marker")
                self.assertEqual(result, expected)

        self.assertIsNone(capi.dict_value_getref_boxing_oom("present"))

    def test_forward_reverse_owned_iteration_and_split_snapshots(self):
        class Instance:
            pass

        instance = Instance()
        instance.first = 2147483647
        instance.second = object()
        instance.third = 3.25
        mixed = {2147483647: -2147483648, "second": instance.second,
                 3.25: None, "hole": 0}
        del mixed["hole"]
        for mapping in (mixed, instance.__dict__):
            with self.subTest(mapping=mapping):
                keys, values, items = list(mapping), list(mapping.values()), list(mapping.items())
                self.assertEqual(mapping.keys() | set(), set(keys))
                self.assertEqual(list(reversed(mapping)), keys[::-1])
                self.assertEqual(list(reversed(mapping.values())), values[::-1])
                self.assertEqual(list(reversed(mapping.items())), items[::-1])
                self.assertEqual(mapping.items() ^ {}.items(), set(items))
                self.assertTrue(all(item in mapping.items() for item in items))
                self.assertEqual(dict(items), mapping)

    def test_iterator_boxing_oom_consumes_entry(self):
        # As when upstream's item iterator fails to allocate its tuple, the
        # failed step consumes the entry.
        capi = import_helper.import_module("_testinternalcapi")
        for reverse in (False, True):
            mapping = {"first": 2147483647, "second": -2147483648}
            iterator = reversed(mapping.values()) if reverse else iter(mapping.values())
            expected = 2147483647 if reverse else -2147483648
            self.assertEqual(capi.dict_iterator_boxing_oom(iterator), expected)
            self.assertEqual(list(iterator), [])
            key_mapping = {2147483647: 0, -2147483648: 1}
            iterator = reversed(key_mapping) if reverse else iter(key_mapping)
            self.assertEqual(capi.dict_iterator_boxing_oom(iterator), expected)
            self.assertEqual(list(iterator), [])

    def test_reused_item_pair_publishes_hash_and_fields_before_release(self):
        capi = import_helper.import_module("_testinternalcapi")
        events = []

        class Old:
            def __hash__(self):
                return 19

            def __del__(self):
                events.append(next(iterator))

        mapping = {"first": Old(), "second": 2147483647, "third": -2147483648}
        iterator = iter(mapping.items())
        capi.dict_iterator_prime_pair(iterator)
        mapping["first"] = None
        pair = next(iterator)
        self.assertEqual(pair, ("second", 2147483647))
        self.assertEqual(hash(pair), hash(("second", 2147483647)))
        self.assertEqual(events, [("third", -2147483648)])
        with self.assertRaises(StopIteration):
            next(iterator)

    def test_fromkeys_keeps_cached_hash_and_subclass_setitem(self):
        class Key:
            armed = False

            def __hash__(self):
                if self.armed:
                    raise AssertionError("cached hash recomputed")
                return 29

        key = Key()
        source = {key: 0}
        key.armed = True
        self.assertEqual(list(dict.fromkeys(source, 2147483647)), [key])
        key.armed = False
        source_set = {key}
        key.armed = True
        self.assertEqual(list(dict.fromkeys(source_set, -2147483648)), [key])

        calls = []

        class Subclass(dict):
            def __setitem__(self, key, value):
                calls.append((key, value))
                super().__setitem__(key, value)

        self.assertEqual(Subclass.fromkeys([1, 3.25], None), {1: None, 3.25: None})
        self.assertEqual(calls, [(1, None), (3.25, None)])
        frozen = frozendict.fromkeys({1: None, 3.25: None}, 2147483647)
        self.assertEqual(frozen, {1: 2147483647, 3.25: 2147483647})
        self.assertEqual(hash(frozen), hash(frozenset(frozen.items())))
        self.assertEqual(hash(frozen), hash(frozenset(frozen.items())))
        with self.assertRaises(TypeError):
            hash(frozendict({1: []}))

    def test_repr_and_equality_retain_selected_values_across_callbacks(self):
        mapping = {}

        class Value:
            def __repr__(self):
                mapping.clear()
                return "selected"

        mapping["key"] = Value()
        self.assertEqual(repr(mapping), "{'key': selected}")
        self.assertEqual(mapping, {})
        left, right = {}, {}

        class Compare:
            def __eq__(self, other):
                left.clear()
                right.clear()
                return True

        left["key"] = Compare()
        right["key"] = object()
        self.assertTrue(left == right)
        self.assertEqual(left, {})
        self.assertEqual(right, {})


class DictOwnedEntryBridgeTests(unittest.TestCase):
    def test_owned_entry_snapshot_status_hash_and_lifetime(self):
        capi = import_helper.import_module("_testinternalcapi")
        marker = object()
        key_object = object()
        mapping = {1: 2147483647, 3.25: marker, "text": None, key_object: []}
        expected = list(mapping.items())
        for source in (mapping, frozendict(mapping)):
            position = 0
            result = []
            while True:
                status, position, key, value, cached_hash = capi.dict_owned_entry((source, position))
                if status == 0:
                    self.assertIsNone(key)
                    self.assertIsNone(value)
                    self.assertEqual(cached_hash, -1)
                    break
                self.assertEqual(status, 1)
                self.assertEqual(cached_hash, hash(key))
                result.append((key, value))
            self.assertEqual(result, expected)
        snapshot = capi.dict_owned_entry((mapping, 1))
        mapping.clear()
        self.assertEqual(snapshot[0], 1)
        self.assertEqual(snapshot[2], 3.25)
        self.assertIs(snapshot[3], marker)

    def test_owned_entry_partial_box_failure(self):
        capi = import_helper.import_module("_testinternalcapi")
        self.assertIsNone(capi.dict_owned_entry_contract())
        self.assertIsNone(capi.dict_owned_entry_boxing_oom())


if __name__ == "__main__":
    unittest.main()
