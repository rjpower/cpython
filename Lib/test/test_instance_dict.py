"""Instance attribute storage, the materialised __dict__ and dict.update."""

import gc
import unittest
import weakref
from test import support


def make_probe():
    """A class whose instance destruction is counted in its own counter."""
    class Probe:
        count = 0

        def __del__(self):
            type(self).count += 1
    return Probe


def materialise(obj):
    """Make obj's __dict__ and drop it again."""
    d = obj.__dict__
    del d


class Point:
    def __init__(self, a=1, b=2):
        self.a = a
        self.b = b


class Mapping:
    """A mapping that is not a dict: update() goes through keys() and []."""
    def __init__(self, items, fail_at=None):
        self.items = items
        self.fail_at = fail_at

    def keys(self):
        return [k for k, _ in self.items]

    def __getitem__(self, key):
        if key == self.fail_at:
            raise KeyError(key)
        return dict(self.items)[key]


class StoreTests(unittest.TestCase):

    def test_first_stores_keep_order(self):
        class C:
            def __init__(self):
                self.z = 1
                self.a = 2
                self.m = 3
        for _ in range(3):
            o = C()
            self.assertEqual(list(vars(o)), ["z", "a", "m"])

    def test_instances_with_different_attribute_sets(self):
        class C:
            def __init__(self, extra):
                self.a = 1
                if extra:
                    self.b = 2
                self.c = 3
        for extra in [True, False, True, False] * 3:
            o = C(extra)
            self.assertEqual(list(vars(o)),
                             ["a", "b", "c"] if extra else ["a", "c"])

    def test_delete_and_re_add(self):
        o = Point()
        for _ in range(3):
            del o.a
            self.assertEqual(list(vars(o)), ["b"])
            o.a = 5
            self.assertEqual(list(vars(o)), ["b", "a"])
            self.assertEqual((o.a, o.b), (5, 2))

    def test_dict_assigned_after_stores(self):
        o = Point()
        o.__dict__ = {"x": 1}
        self.assertEqual(vars(o), {"x": 1})
        o.y = 2
        self.assertEqual(vars(o), {"x": 1, "y": 2})
        self.assertFalse(hasattr(o, "a"))

    def test_subclass_adds_attributes(self):
        class Sub(Point):
            def __init__(self):
                super().__init__()
                self.c = 3
                self.d = 4
        for _ in range(3):
            o = Sub()
            self.assertEqual(list(vars(o)), ["a", "b", "c", "d"])
            self.assertEqual(list(vars(Point())), ["a", "b"])

    def test_slots(self):
        class S:
            __slots__ = ("a", "b")
            def __init__(self):
                self.a = 1
                self.b = 2
        class Mixed:
            __slots__ = ("a", "__dict__")
            def __init__(self):
                self.a = 1
                self.b = 2
        for _ in range(3):
            s = S()
            self.assertEqual((s.a, s.b), (1, 2))
            self.assertFalse(hasattr(s, "__dict__"))
            m = Mixed()
            self.assertEqual(m.a, 1)
            self.assertEqual(vars(m), {"b": 2})
            m.__dict__.update({"b": 3, "c": 4})
            self.assertEqual(vars(m), {"b": 3, "c": 4})


class DictTests(unittest.TestCase):

    def test_dict_is_stable_and_live(self):
        o = Point()
        d = o.__dict__
        self.assertIs(o.__dict__, d)
        o.a = 10
        self.assertEqual(d, {"a": 10, "b": 2})
        d["c"] = 3
        self.assertEqual(o.c, 3)
        self.assertEqual(list(vars(o)), ["a", "b", "c"])

    def test_dict_of_empty_instance(self):
        class C:
            pass
        o = C()
        self.assertEqual(o.__dict__, {})
        o.x = 1
        self.assertEqual(vars(o), {"x": 1})

    def test_update_replaces_and_adds(self):
        a, b = Point(), Point()
        a.__dict__.update({"b": 20, "a": 10, "n": 1})
        self.assertEqual(vars(a), {"a": 10, "b": 20, "n": 1})
        self.assertEqual(list(vars(a)), ["a", "b", "n"])
        self.assertEqual(vars(b), {"a": 1, "b": 2})
        b.n = 7
        self.assertEqual(list(vars(b)), ["a", "b", "n"])

    def test_update_fills_deleted_slot(self):
        o = Point()
        del o.a
        o.__dict__.update({"a": 5})
        self.assertEqual(list(vars(o)), ["b", "a"])
        self.assertEqual(o.a, 5)

    def test_update_from_other_instance_dict(self):
        src, dst = Point(3, 4), Point()
        dst.__dict__.update(src.__dict__)
        self.assertEqual(vars(dst), {"a": 3, "b": 4})
        src.a = 9
        self.assertEqual(dst.a, 3)

    def test_update_from_mapping_and_pairs(self):
        o = Point()
        o.__dict__.update(Mapping([("a", 7), ("q", 8)]))
        self.assertEqual(vars(o), {"a": 7, "b": 2, "q": 8})
        o.__dict__.update([("b", 0), ("r", 1)])
        self.assertEqual(vars(o), {"a": 7, "b": 0, "q": 8, "r": 1})

    def test_update_with_keys_that_are_not_str(self):
        o = Point()
        o.__dict__.update({1: "one", "a": 5})
        self.assertEqual(vars(o), {"a": 5, "b": 2, 1: "one"})
        self.assertEqual(o.a, 5)
        o.__dict__[2.5] = None
        self.assertEqual(o.b, 2)

    def test_update_with_str_subclass_keys(self):
        class K(str):
            pass
        o = Point()
        o.__dict__.update({K("a"): 6, K("n"): 1})
        self.assertEqual(o.a, 6)
        self.assertEqual(o.n, 1)

    def test_update_raising_midway_keeps_earlier_items(self):
        o = Point()
        with self.assertRaises(KeyError):
            o.__dict__.update(Mapping([("a", 5), ("z", 6), ("b", 7)],
                                      fail_at="z"))
        self.assertEqual(vars(o), {"a": 5, "b": 2})

    def test_update_replaced_value_finalizer_sees_new_value(self):
        seen = []
        class V:
            def __init__(self, owner):
                self.owner = owner
            def __del__(self):
                seen.append(self.owner.a)
        o = Point()
        o.a = V(o)
        o.__dict__.update({"a": "new"})
        self.assertEqual(seen, ["new"])

    def test_update_does_not_grow_a_frozen_shape_family(self):
        # Many different new keys on one type must keep working.
        o = Point()
        for i in range(60):
            o.__dict__.update({f"k{i}": i})
        self.assertEqual(len(vars(o)), 62)
        self.assertEqual(o.k59, 59)
        self.assertEqual(Point().a, 1)


class ReleaseTests(unittest.TestCase):

    def setUp(self):
        self.Probe = make_probe()

    def test_values_are_released_with_a_sole_dict(self):
        for _ in range(3):
            o = Point(self.Probe(), self.Probe())
            materialise(o)
            del o
            support.gc_collect()
        self.assertEqual(self.Probe.count, 6)

    def test_values_are_released_without_a_dict(self):
        o = Point(self.Probe(), self.Probe())
        del o
        support.gc_collect()
        self.assertEqual(self.Probe.count, 2)

    def test_dict_that_outlives_the_object(self):
        o = Point(self.Probe(), "b")
        d = o.__dict__
        ref = weakref.ref(o)
        del o
        support.gc_collect()
        self.assertIsNone(ref())
        self.assertEqual(self.Probe.count, 0)
        self.assertEqual(list(d), ["a", "b"])
        self.assertEqual(d["b"], "b")
        d["c"] = 3
        self.assertEqual(len(d), 3)
        del d
        support.gc_collect()
        self.assertEqual(self.Probe.count, 1)

    def test_dict_of_collected_cycle_without_other_holders(self):
        o = Point(self.Probe(), self.Probe())
        o.me = o
        materialise(o)
        del o
        support.gc_collect()
        self.assertEqual(self.Probe.count, 2)

    def test_value_finalizer_that_touches_dicts(self):
        other = Point()
        log = []
        class V:
            def __del__(self):
                log.append(sorted(vars(other)))
                other.__dict__.update({"seen": 1})
        o = Point(V(), 2)
        materialise(o)
        del o
        support.gc_collect()
        self.assertEqual(log, [["a", "b"]])
        self.assertEqual(other.seen, 1)

    def test_value_finalizer_that_makes_a_dict_during_clear(self):
        keep = []
        class V:
            def __init__(self, owner_ref):
                self.owner_ref = owner_ref
            def __del__(self):
                keep.append(self.owner_ref)
        o = Point()
        o.a = V(weakref.ref(o))
        materialise(o)
        del o
        support.gc_collect()
        self.assertEqual(len(keep), 1)
        self.assertIsNone(keep[0]())

    def test_exception_state_survives_deallocation(self):
        o = Point()
        materialise(o)
        try:
            raise ValueError("kept")
        except ValueError as exc:
            del o
            self.assertIs(type(exc), ValueError)
            import sys
            self.assertIs(type(sys.exception()), ValueError)

    def test_reassigned_dict_releases_values(self):
        o = Point(self.Probe(), self.Probe())
        materialise(o)
        o.__dict__ = {}
        support.gc_collect()
        self.assertEqual(self.Probe.count, 2)
        o.q = 1
        self.assertEqual(vars(o), {"q": 1})

    def test_gc_clear_of_instances_with_dicts(self):
        objs = []
        for _ in range(10):
            a, b = Point(self.Probe(), None), Point(self.Probe(), None)
            a.b, b.b = b, a
            a.__dict__
            objs.append((weakref.ref(a), weakref.ref(b)))
        del a, b
        support.gc_collect()
        self.assertTrue(all(x() is None and y() is None for x, y in objs))
        self.assertEqual(self.Probe.count, 20)


if __name__ == "__main__":
    unittest.main()
