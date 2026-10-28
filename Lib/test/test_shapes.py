"""Shapes: the hidden classes in the object header, and the slots they
describe.

See boil/design/09-object-protocol.md and 04-shapes.md.
"""

import collections
import dataclasses
import dis
import gc
import itertools
import math
import sys
import types
import unittest
import weakref
from test.support import cpython_only, requires_specialization, reset_code
from test.support import script_helper
from test.support.import_helper import import_module

_testinternalcapi = import_module("_testinternalcapi")
_testcapi = import_module("_testcapi")
getattr_value = _testcapi.pyvalue.GetAttrValue
shape_of = _testinternalcapi.get_object_shape
shape_count = _testinternalcapi.get_type_shape_count
MAX_SHAPES = _testinternalcapi._Py_SHAPE_MAX_PER_TYPE
THRESHOLD = _testinternalcapi.SPECIALIZATION_THRESHOLD
COOLDOWN = _testinternalcapi.SPECIALIZATION_COOLDOWN
MIN_SLOTS = _testinternalcapi.SHAPE_MIN_SLOTS
MAX_SLOTS = _testinternalcapi.SHAPE_MAX_SLOTS
capacity = _testinternalcapi.type_slot_capacity
slots_of = _testinternalcapi.object_slots
BOXED = _testinternalcapi.SLOT_BOXED
I32 = _testinternalcapi.SLOT_I32
F64 = _testinternalcapi.SLOT_F64


Plan = collections.namedtuple(
    "Plan", "kind fallback slot_kind cacheable member type_version offset value")
StorePlan = collections.namedtuple(
    "StorePlan", "outcome cacheable member offset has_shape_to slot_kind")


def plan(o, name):
    return Plan(*_testinternalcapi.lookup_plan(o, name))


def store_plan(o, name, delete=False, *value):
    return StorePlan(*_testinternalcapi.lookup_store_plan(o, name, delete, *value))


def opnames(f):
    return {i.opname for i in dis.get_instructions(f, adaptive=True)}


def make_class():
    class C:
        def __init__(self):
            self.x = 1
            self.y = 2
    # Each class gets fresh feedback for __init__, whose code they share.
    reset_code(C.__init__)
    return C


@cpython_only
class TransitionTests(unittest.TestCase):

    def test_new_instance_has_root_shape(self):
        class C:
            pass
        self.assertEqual(shape_of(C()), 0)
        self.assertEqual(shape_count(C), 1)

    def test_same_order_gives_same_shape(self):
        C = make_class()
        a, b = C(), C()
        self.assertEqual(shape_of(a), shape_of(b))
        self.assertEqual(shape_count(C), 3)

    def test_different_order_gives_different_shape(self):
        class C:
            pass
        a, b = C(), C()
        a.x = 1
        a.y = 2
        b.y = 2
        b.x = 1
        self.assertNotEqual(shape_of(a), shape_of(b))
        self.assertEqual(shape_count(C), 5)

    def test_replace_keeps_shape(self):
        o = make_class()()
        shape = shape_of(o)
        o.x = "replaced"
        self.assertEqual(shape_of(o), shape)
        self.assertEqual(o.x, "replaced")

    def test_delete_last_returns_to_parent(self):
        class C:
            pass
        o = C()
        o.x = 1
        parent = shape_of(o)
        o.y = 2
        del o.y
        self.assertEqual(shape_of(o), parent)

    def test_delete_other_leaves_hole(self):
        class C:
            pass
        a, b = C(), C()
        a.x, a.y, a.z = 1, 2, 3
        b.x, b.y, b.z = 1, 2, 3
        del a.y
        # A hole shape: the slots keep their positions, and the fields
        # their kinds.
        hole = _testinternalcapi.shape(a)[2]
        self.assertEqual(hole, (("x", I32), None, ("z", I32)))
        self.assertEqual(slots_of(a)[:3], (1, None, 3))
        # The same delete follows the same edge.
        del b.y
        self.assertEqual(shape_of(a), shape_of(b))
        # A re-added name is appended, which keeps the dict's order.
        a.y = 2
        self.assertEqual(slots_of(a)[:4], (1, None, 3, 2))
        self.assertEqual(list(a.__dict__), ["x", "z", "y"])

    def test_dict_materialisation_drops_shape(self):
        o = make_class()()
        d = o.__dict__
        self.assertIsNone(shape_of(o))
        d["x"] = 10
        o.z = 3
        self.assertEqual((o.x, o.y, o.z), (10, 2, 3))
        self.assertIsNone(shape_of(o))

    def test_vars_drops_shape(self):
        o = make_class()()
        self.assertEqual(vars(o), {"x": 1, "y": 2})
        self.assertIsNone(shape_of(o))

    def test_dict_assignment_drops_shape(self):
        o = make_class()()
        o.__dict__ = {"z": 3}
        self.assertIsNone(shape_of(o))
        self.assertEqual(o.z, 3)
        self.assertFalse(hasattr(o, "x"))

    def test_class_assignment_drops_shape(self):
        C = make_class()
        class D:
            pass
        o = C()
        o.__class__ = D
        self.assertIsNone(shape_of(o))
        self.assertEqual((o.x, o.y), (1, 2))

    def test_slots_have_no_shape(self):
        class S:
            __slots__ = ("x", "y")
        s = S()
        s.x = 1
        self.assertIsNone(shape_of(s))

    def test_subclass_has_its_own_shapes(self):
        B = make_class()
        class D(B):
            pass
        b, d1, d2 = B(), D(), D()
        self.assertEqual(shape_of(d1), shape_of(d2))
        self.assertEqual(shape_count(B), 3)
        self.assertEqual(shape_count(D), 3)
        self.assertEqual((b.x, d1.y), (1, 2))

    def test_many_attributes_overflow_to_dict(self):
        class C:
            pass
        o = C()
        for i in range(40):
            setattr(o, f"a{i}", i)
        self.assertIsNone(shape_of(o))
        self.assertEqual([getattr(o, f"a{i}") for i in range(40)],
                         list(range(40)))

    def test_shapes_per_type_are_limited(self):
        class C:
            pass
        names = "abcdef"
        objects = []
        for order in itertools.permutations(names, 3):
            o = C()
            for name in order:
                setattr(o, name, name)
            objects.append((o, order))
        self.assertEqual(shape_count(C), MAX_SHAPES)
        self.assertIsNone(shape_of(objects[-1][0]))
        for o, order in objects:
            self.assertEqual([getattr(o, name) for name in order], list(order))
        # Orders that already have a shape still get it.
        o = C()
        o.a, o.b, o.c = "a", "b", "c"
        self.assertEqual(shape_of(o), shape_of(objects[0][0]))


@cpython_only
@requires_specialization
class SpecializationTests(unittest.TestCase):

    def test_load_and_store_use_shapes(self):
        C = make_class()
        @reset_code
        def get(o):
            return o.x + o.y
        @reset_code
        def put(o, v):
            o.x = v
        o = C()
        for _ in range(THRESHOLD):
            get(o)
            put(o, 1)
            C()
        self.assertIn("LOAD_ATTR_SHAPE", opnames(get))
        self.assertIn("STORE_ATTR_SHAPE", opnames(put))
        self.assertIn("STORE_ATTR_SHAPE_TRANSITION", opnames(C.__init__))

    def test_load_deopts_after_dict_materialisation(self):
        C = make_class()
        @reset_code
        def get(o):
            return o.x
        objects = [C() for _ in range(THRESHOLD)]
        for o in objects:
            get(o)
        self.assertIn("LOAD_ATTR_SHAPE", opnames(get))
        o = objects[0]
        o.__dict__["x"] = 10
        for _ in range(2 * COOLDOWN):
            self.assertEqual(get(o), 10)
        # The site reads slot 0's dict, guarded by the dictionary-mode shape.
        self.assertIn("LOAD_ATTR_WITH_HINT", opnames(get))
        self.assertEqual(get(objects[1]), 1)

    def test_load_deopts_after_delete(self):
        C = make_class()
        @reset_code
        def get(o):
            return o.y
        objects = [C() for _ in range(THRESHOLD)]
        for o in objects:
            get(o)
        self.assertIn("LOAD_ATTR_SHAPE", opnames(get))
        o = objects[0]
        del o.y
        with self.assertRaises(AttributeError):
            get(o)
        o.y = "again"
        self.assertEqual(get(o), "again")
        self.assertEqual(get(objects[1]), 2)

    def test_store_after_dict_materialisation_reaches_dict(self):
        C = make_class()
        @reset_code
        def put(o, v):
            o.x = v
        objects = [C() for _ in range(THRESHOLD)]
        for o in objects:
            put(o, 0)
        self.assertIn("STORE_ATTR_SHAPE", opnames(put))
        o = objects[0]
        d = o.__dict__
        put(o, 5)
        self.assertEqual(d["x"], 5)

    def test_store_add_on_object_with_dict(self):
        C = make_class()
        for _ in range(THRESHOLD):
            C()
        self.assertIn("STORE_ATTR_SHAPE_TRANSITION", opnames(C.__init__))
        o = C.__new__(C)
        d = o.__dict__
        C.__init__(o)
        self.assertEqual(d, {"x": 1, "y": 2})
        o = C.__new__(C)
        o.y = 0
        C.__init__(o)
        self.assertEqual(vars(o), {"y": 2, "x": 1})

    def test_descriptor_added_after_specialisation(self):
        C = make_class()
        @reset_code
        def get(o):
            return o.x
        @reset_code
        def put(o, v):
            o.x = v
        o = C()
        for _ in range(THRESHOLD):
            get(o)
            put(o, 1)
        stored = []
        C.x = property(lambda self: 42, lambda self, v: stored.append(v))
        self.assertEqual(get(o), 42)
        put(o, 7)
        self.assertEqual(stored, [7])

    def test_base_method_on_subclass_instances(self):
        B = make_class()
        class D(B):
            @reset_code
            def __init__(self):
                self.z = 0
                super().__init__()
        @reset_code
        def get(o):
            return (o.x, o.y)
        objects = [B(), D()] * THRESHOLD
        for o in objects:
            self.assertEqual(get(o), (1, 2))
        self.assertNotEqual(shape_of(objects[0]), shape_of(objects[1]))

    def test_optional_attribute_orders(self):
        class Token:
            @reset_code
            def __init__(self, kind, value=None):
                self.kind = kind
                if value is not None:
                    self.value = value
                self.pos = 0
        @reset_code
        def get(t):
            return t.pos
        @reset_code
        def mark(t):
            t.mark = True
        tokens = [Token("name", "x") if i % 2 else Token("op")
                  for i in range(4 * COOLDOWN)]
        for t in tokens:
            self.assertEqual(get(t), 0)
            mark(t)
        # Two shapes of one type: the load reads both, the store is generic.
        self.assertIn("LOAD_ATTR_SHAPE_POLY", opnames(get))
        self.assertNotIn("LOAD_ATTR_SHAPE", opnames(get))
        self.assertNotIn("STORE_ATTR_SHAPE_TRANSITION", opnames(mark))
        # The generic path follows the same edges.
        t = Token("name", "x")
        t.mark = True
        self.assertEqual(shape_of(t), shape_of(tokens[1]))
        self.assertEqual(list(vars(tokens[0])), ["kind", "pos", "mark"])
    def test_shadowed_method_on_one_instance(self):
        class M:
            def m(self):
                return 1
        @reset_code
        def call(o):
            return o.m()
        plain, shadowed = M(), M()
        shadowed.m = lambda: 2
        for _ in range(2 * THRESHOLD):
            self.assertEqual(call(plain), 1)
            self.assertEqual(call(shadowed), 2)

    def test_property_getter_replaced(self):
        class P:
            @property
            def x(self):
                return 1
        @reset_code
        def get(o):
            return o.x
        o = P()
        for _ in range(THRESHOLD):
            get(o)
        self.assertIn("LOAD_ATTR_PROPERTY", opnames(get))
        prop = P.__dict__["x"]
        prop.__init__(lambda self: 2)
        gc.collect()
        self.assertEqual(get(o), 2)
        prop.__init__(str)
        self.assertTrue(get(o).startswith("<"))
        prop.__init__(None)
        with self.assertRaises(AttributeError):
            get(o)

    def test_getter_that_collects(self):
        class P:
            @property
            def x(self):
                gc.collect()
                return [1]
        @reset_code
        def get(o):
            return o.x
        for _ in range(THRESHOLD + 2):
            self.assertEqual(get(P()), [1])

    def test_descriptor_gains_set(self):
        class D:
            def __get__(self, o, t):
                return "descr"
        class C:
            x = D()
        @reset_code
        def get(o):
            return o.x
        o = C()
        o.__dict__["x"] = "inst"
        self.assertFalse(plan(o, "x").cacheable)
        for i in range(4 * THRESHOLD):
            if i == 2 * THRESHOLD:
                D.__set__ = lambda self, o, v: None
            self.assertEqual(get(o), "inst" if i < 2 * THRESHOLD else "descr")

    def test_getattribute_without_version(self):
        class G:
            x = 0
            def __getattribute__(self, name):
                return "first"
        for i in range(1001):
            G.x = i
            _testcapi.type_assign_version(G)
        self.assertEqual(_testcapi.type_get_version(G), 0)
        @reset_code
        def get(o):
            return o.x
        o = G()
        for _ in range(THRESHOLD):
            self.assertEqual(get(o), "first")
        # A site guarded by version 0 would never see this change.
        G.__getattribute__ = lambda self, name: "second"
        self.assertEqual(get(o), "second")

    def test_transient_types(self):
        @reset_code
        def get(o):
            return o.x
        refs = []
        for i in range(10):
            C = make_class()
            for _ in range(THRESHOLD):
                self.assertEqual(get(C()), 1)
            refs.append(weakref.ref(C))
            del C
        gc.collect()
        self.assertEqual([r() for r in refs], [None] * len(refs))
        class D:
            x = "class"
        self.assertEqual(get(D()), "class")


    def test_poly_site_survives_invalidations(self):
        # Each class mutation renews the version of every shape; the rows of
        # the old versions are replaced, not kept, so they do not fill the
        # table and turn a two-shape site megamorphic.
        class C:
            pass
        a = C(); a.x = 1; a.y = 2
        b = C(); b.y = 3; b.x = 4
        @reset_code
        def get(o):
            return o.x
        def warm():
            for _ in range(4 * (THRESHOLD + COOLDOWN)):
                self.assertEqual(get(a), 1)
                self.assertEqual(get(b), 4)
        warm()
        self.assertIn("LOAD_ATTR_SHAPE_POLY", opnames(get))
        for i in range(4):
            setattr(C, f"m{i}", i)
            warm()
            self.assertIn("LOAD_ATTR_SHAPE_POLY", opnames(get))

    def test_poly_row_shape_address_reused(self):
        # A row borrows its shape, so the shape can die and a new one take
        # its address.  The row's version is not the new shape's, so the
        # site does not apply the old row's offset to it.
        @reset_code
        def get(o):
            return o.x
        def make_old():
            class A:
                pass
            p = A(); p.x = 1; p.y = 2
            q = A(); q.y = 3; q.x = 4
            for _ in range(4 * (THRESHOLD + COOLDOWN)):
                self.assertEqual(get(p), 1)
                self.assertEqual(get(q), 4)
            self.assertIn("LOAD_ATTR_SHAPE_POLY", opnames(get))
            addrs = {_testinternalcapi.shape_address(p),
                     _testinternalcapi.shape_address(q)}
            return weakref.ref(A), addrs
        ref, addrs = make_old()
        gc.collect()
        self.assertIsNone(ref())
        for attempt in range(2000):
            class B:
                pass
            # x at slot 1 or 2, where neither old row has it at slot 0
            # alone: a stale offset would read another attribute.
            o = B(); o.q = "q"; o.x = "x"
            o2 = B(); o2.y = "y"; o2.q = "q"; o2.x = "x"
            for obj in (o, o2):
                if _testinternalcapi.shape_address(obj) in addrs:
                    for _ in range(4 * (THRESHOLD + COOLDOWN)):
                        self.assertEqual(get(obj), "x")
                    return
            del B, o, o2
            if attempt % 50 == 49:
                gc.collect()
        self.skipTest("no new shape took a dead shape's address")

    def test_lookup_that_changes_the_class(self):
        # A key comparison during the MRO lookup replaces the class's bases,
        # leaving every dict on the new MRO str-keyed.  The result found
        # under the old version is not cached under the new one.
        armed = []
        class Key:
            def __hash__(self):
                return hash("attr")
            def __eq__(self, other):
                if armed:
                    armed.pop()
                    C.__bases__ = (Base2,)
                return False
        # Key first, so that a lookup of "attr" compares it.
        with self.assertWarns(RuntimeWarning):
            Base1 = type("Base1", (), {Key(): None, "attr": "old"})
        class Base2:
            attr = "new"
        class C(Base1):
            pass
        o = C()
        @reset_code
        def get(o):
            return o.attr
        # Armed before the first lookup, which the type cache would
        # otherwise answer.
        armed.append(True)
        self.assertIn(get(o), ("old", "new"))
        self.assertEqual(C.__bases__, (Base2,))
        for _ in range(4 * (THRESHOLD + COOLDOWN)):
            self.assertEqual(get(o), "new")
            self.assertEqual(o.attr, "new")

class StrName(str):
    pass


@cpython_only
class LookupPlanTests(unittest.TestCase):
    """The plan of each kind, with the behaviour it describes beside it."""

    def assertLoads(self, o, name, value):
        self.assertEqual(getattr(o, name), value)
        self.assertEqual(getattr_value(o, name)[0], value)

    def assertLoadRaises(self, o, name, exc=AttributeError):
        with self.assertRaises(exc):
            getattr(o, name)
        with self.assertRaises(exc):
            getattr_value(o, name)

    def test_slot(self):
        C = make_class()
        o = C()
        p = plan(o, "x")
        self.assertEqual((p.kind, p.fallback), ("SLOT", "ABSENT"))
        self.assertTrue(p.cacheable)
        self.assertNotEqual(p.type_version, 0)
        self.assertNotEqual(plan(o, "y").offset, p.offset)
        self.assertLoads(o, "x", 1)

    def test_dictionary_mode(self):
        C = make_class()
        o = C()
        d = o.__dict__
        self.assertEqual(plan(o, "x")[:2], ("DICT", "ABSENT"))
        # The dict is slot 0, and the store goes to it.
        d["z"] = 3
        o.x = 10
        self.assertEqual(d["x"], 10)
        self.assertLoads(o, "z", 3)
        self.assertLoads(o, "x", 10)

    def test_shape_exhaustion_without_dict(self):
        class C:
            pass
        objects = []
        for order in itertools.permutations("abcdef", 3):
            o = C()
            for name in order:
                setattr(o, name, name)
            objects.append((o, order))
        o, order = objects[-1]
        self.assertIsNone(shape_of(o))
        # Dictionary mode: the values moved to the dict in slot 0.
        self.assertEqual(plan(o, order[0]).kind, "DICT")
        for name in order:
            self.assertLoads(o, name, name)
        o.g = "g"
        delattr(o, order[0])
        self.assertLoadRaises(o, order[0])
        self.assertEqual(list(vars(o)), [*order[1:], "g"])

    def test_member(self):
        class S:
            __slots__ = ("a",)
        s = S()
        p = plan(s, "a")
        self.assertEqual(p.kind, "DATA_DESCR")
        self.assertTrue(p.member)
        self.assertTrue(p.cacheable)
        self.assertLoadRaises(s, "a")
        s.a = 1
        self.assertLoads(s, "a", 1)
        sp = store_plan(s, "a")
        self.assertEqual((sp.outcome, sp.member, sp.offset),
                         ("DATA_DESCR", True, p.offset))
        del s.a
        with self.assertRaises(AttributeError):
            del s.a
        self.assertLoadRaises(s, "a")

    def test_transplanted_member(self):
        class A:
            __slots__ = ("x",)
        class B:
            __slots__ = ()
        B.x = A.x
        b = B()
        self.assertEqual(plan(b, "x")[:5], ("DATA_DESCR", "NONE", 0, True, False))
        self.assertFalse(store_plan(b, "x").member)
        self.assertLoadRaises(b, "x", TypeError)
        with self.assertRaises(TypeError):
            b.x = 1

    def test_member_on_variable_size_subclass(self):
        class T(tuple):
            __slots__ = ("a",)
        for n in (0, 1, 5, 40):
            t = T(range(n))
            # After the items: no offset from the object start.
            self.assertFalse(plan(t, "a").member)
            t.a = n
            self.assertLoads(t, "a", n)
            self.assertEqual(len(t), n)

    def test_native_members(self):
        # A double, and an object member whose reads are audited.
        self.assertEqual(plan(2j, "real")[:5],
                         ("DATA_DESCR", "NONE", 0, True, False))
        self.assertLoads(2j, "real", 0.0)
        def f():
            pass
        self.assertFalse(plan(f, "__globals__").member)
        self.assertIs(f.__globals__, globals())

    def test_property(self):
        class P:
            @property
            def x(self):
                return 1
        o = P()
        p = plan(o, "x")
        self.assertEqual(p.kind, "PROPERTY")
        self.assertTrue(p.cacheable)
        self.assertIs(p.value, P.__dict__["x"])
        self.assertLoads(o, "x", 1)
        prop = P.__dict__["x"]
        prop.__init__(lambda self: 2)
        self.assertLoads(o, "x", 2)
        prop.__init__(str)
        self.assertTrue(o.x.startswith("<"))
        prop.__init__(None)
        self.assertLoadRaises(o, "x")
        self.assertEqual(store_plan(o, "x").outcome, "DATA_DESCR")

    def test_set_only_descriptor(self):
        class SetOnly:
            def __set__(self, o, v):
                o.__dict__["seen"] = v
        d = SetOnly()
        class C:
            x = d
        o = C()
        # Not a data descriptor for loads: storage comes first.
        self.assertEqual(plan(o, "x")[:2], ("CLASS_VALUE", "CLASS_VALUE"))
        self.assertFalse(plan(o, "x").cacheable)
        self.assertLoads(o, "x", d)
        self.assertEqual(store_plan(o, "x").outcome, "DATA_DESCR")
        o.x = 5
        self.assertEqual(o.seen, 5)
        o.__dict__["x"] = 7
        self.assertLoads(o, "x", 7)

    def test_methods(self):
        class M:
            def m(self):
                return 1
            s = staticmethod(lambda: 2)
            c = classmethod(lambda cls: cls)
        o, shadowed = M(), M()
        shadowed.m = lambda: 3
        p = plan(o, "m")
        self.assertEqual(p.kind, "METHOD")
        self.assertTrue(p.cacheable)
        self.assertIs(p.value, M.__dict__["m"])
        f = o.m
        self.assertIs(f.__self__, o)
        self.assertEqual(f(), 1)
        self.assertEqual(plan(shadowed, "m")[:2], ("SLOT", "METHOD"))
        self.assertEqual(shadowed.m(), 3)
        self.assertEqual(o.m(), 1)
        self.assertEqual(plan(o, "s").kind, "DESCR")
        self.assertEqual(o.s(), 2)
        self.assertEqual(plan(o, "c").kind, "DESCR")
        self.assertIs(o.c(), M)

    def test_custom_get(self):
        class D:
            def __get__(self, o, t):
                return "got"
        class C:
            x = D()
        o = C()
        p = plan(o, "x")
        self.assertEqual(p.kind, "DESCR")
        self.assertFalse(p.cacheable)
        self.assertLoads(o, "x", "got")

    def test_class_values(self):
        class V:
            a = 1
            b = 2.5
        o = V()
        self.assertEqual(plan(o, "a")[:2], ("CLASS_VALUE", "CLASS_VALUE"))
        self.assertEqual(plan(o, "a").value, 1)
        self.assertLoads(o, "b", 2.5)
        o.a = 3
        self.assertEqual(plan(o, "a")[:2], ("SLOT", "CLASS_VALUE"))
        self.assertLoads(o, "a", 3)
        self.assertLoads(V(), "a", 1)

    def test_missing(self):
        C = make_class()
        self.assertEqual(plan(C(), "nope").kind, "ABSENT")
        self.assertLoadRaises(C(), "nope")
        class G:
            def __getattr__(self, name):
                return name.upper()
        # The plan describes the generic protocol beneath the hook.
        self.assertEqual(plan(G(), "nope").kind, "ABSENT")
        self.assertLoads(G(), "nope", "NOPE")

    def test_getattribute(self):
        class GA:
            x = 42
            def __getattribute__(self, name):
                return "ga"
        o = GA()
        self.assertLoads(o, "x", "ga")
        # plan.value is the MRO result of the name, not the hook.
        self.assertEqual(plan(o, "x").value, 42)
        self.assertEqual(object.__getattribute__(o, "x"), 42)

    def test_delegating_getattribute(self):
        class D:
            y = 1
            def __getattribute__(self, name):
                return object.__getattribute__(self, name)
        o = D()
        o.x = 2
        self.assertLoads(o, "x", 2)
        self.assertLoads(o, "y", 1)
        self.assertLoadRaises(o, "z")

    def test_independent_read_and_write_overrides(self):
        class RW:
            def __getattr__(self, name):
                return "missing"
            def __setattr__(self, name, value):
                object.__setattr__(self, name, value * 2)
        o = RW()
        o.x = 1
        self.assertLoads(o, "x", 2)
        self.assertLoads(o, "y", "missing")
        object.__setattr__(o, "z", 1)
        self.assertLoads(o, "z", 1)
        self.assertEqual(store_plan(o, "x", False, 3).outcome, "REPLACE")

    def test_str_subclass_name(self):
        class P:
            @property
            def x(self):
                return "prop"
        class Q:
            __slots__ = ()
            x = P.x
        for o in (P(), Q()):
            name = StrName("x")
            p = plan(o, name)
            self.assertEqual(p.kind, "PROPERTY")
            self.assertFalse(p.cacheable)
            self.assertLoads(o, name, "prop")
        o = P()
        setattr(o, StrName("y"), 1)
        self.assertLoads(o, "y", 1)
        self.assertEqual(plan(o, StrName("y")).kind, "DICT")

    def test_modules(self):
        a = types.ModuleType("a")
        a.x, a.y = 1, 2
        b = types.ModuleType("b")
        b.y, b.x = 3, 4
        self.assertEqual(plan(a, "x")[:2], ("DICT", "ABSENT"))
        @reset_code
        def get(m):
            return m.x
        for _ in range(2 * THRESHOLD):
            self.assertEqual(get(a), 1)
            self.assertEqual(get(b), 4)
        del a.x
        with self.assertRaises(AttributeError):
            get(a)
        a.x = 5
        self.assertEqual(get(a), 5)
        a.__getattr__ = lambda name: "hook"
        self.assertEqual(a.missing, "hook")
        a.__dict__["__class__"] = 5
        self.assertIs(a.__class__, types.ModuleType)

    def test_raw_type_lookup(self):
        def make():
            class C:
                x = 42
                def __getattribute__(self, name):
                    return "ga"
            return C
        lookup = _testinternalcapi.type_cache_lookup
        # Root entry first, then the plan.
        C = make()
        self.assertEqual(C.x, 42)
        self.assertEqual(C().x, "ga")
        self.assertEqual(plan(C(), "x").value, 42)
        hit, value, _ = lookup(C, "x")
        self.assertEqual((hit, value), (1, 42))
        # The plan first, then the root entry.
        C = make()
        self.assertEqual(plan(C(), "x").value, 42)
        self.assertEqual(C().x, "ga")
        self.assertEqual(C.x, 42)
        hit, value, _ = lookup(C, "x")
        self.assertEqual(value, 42)
        self.assertIs(plan(C(), "__getattribute__").value,
                      C.__dict__["__getattribute__"])

    def test_non_interned_name(self):
        C = make_class()
        name = "".join(["x", "x"])
        self.assertFalse(plan(C(), name).cacheable)
        self.assertTrue(plan(C(), sys.intern(name)).cacheable)

    def test_store_plans(self):
        C = make_class()
        o = C()
        sp = store_plan(o, "x", False, 3)
        self.assertEqual(sp.outcome, "REPLACE")
        self.assertEqual(sp.offset, plan(o, "x").offset)
        self.assertEqual(store_plan(o, "z").outcome, "ADD")
        self.assertFalse(store_plan(o, "z").has_shape_to)
        other = C()
        other.z = 0
        self.assertTrue(store_plan(o, "z").has_shape_to)
        self.assertEqual(store_plan(o, "x", delete=True).outcome, "REMOVE")
        self.assertEqual(store_plan(o, "z", delete=True).outcome, "MISSING")
        with self.assertRaises(AttributeError):
            del o.z
        o.__dict__
        self.assertEqual(store_plan(o, "x").outcome, "DICT")
        class N:
            __slots__ = ()
        self.assertEqual(store_plan(N(), "q").outcome, "NO_STORAGE")
        with self.assertRaises(AttributeError):
            N().q = 1

    def test_store_out_of_memory(self):
        C = make_class()
        # A new name needs a new shape, so preparing it allocates.
        failures = 0
        for start in range(20):
            o = C()
            before = shape_of(o)
            name = f"fresh_{start}"
            try:
                _testcapi.set_nomemory(start)
                try:
                    setattr(o, name, 1)
                finally:
                    _testcapi.remove_mem_hooks()
            except MemoryError:
                failures += 1
                self.assertEqual(shape_of(o), before)
                self.assertFalse(hasattr(o, name))
                self.assertEqual(list(vars(o)), ["x", "y"])
            else:
                self.assertEqual(getattr(o, name), 1)
                self.assertEqual(list(vars(o)), ["x", "y", name])
        self.assertGreater(failures, 0)


def make_static(n, base=object, prefix="a"):
    """A class whose __init__ assigns n attributes, so that its
    __static_attributes__ has n names."""
    body = "".join(f"        self.{prefix}{i} = 0\n" for i in range(n))
    ns = {}
    exec(f"class S(base):\n    def __init__(self):\n{body or '        pass'}\n",
         {"base": base}, ns)
    return ns["S"]


@cpython_only
class LayoutTests(unittest.TestCase):
    """The slots after tp_basicsize, as many as the type's capacity."""

    def test_capacity_from_static_attributes(self):
        self.assertEqual(capacity(make_static(0)), MIN_SLOTS)
        self.assertEqual(capacity(make_static(3)), MIN_SLOTS)
        self.assertEqual(capacity(make_static(10)), 12)
        self.assertEqual(capacity(make_static(40)), MAX_SLOTS)
        # The names of every class on the MRO, each once.
        B = make_static(5)
        self.assertEqual(capacity(make_static(5, B, "b")), 12)
        self.assertEqual(capacity(make_static(5, B)), 7)
        self.assertIsNone(capacity(int))

    def test_static_attributes_set_before_instances(self):
        # A class decorator may declare the attributes until the first
        # instance exists (dataclasses does).
        names = tuple(f"n{i}" for i in range(8))
        C = make_static(0)
        self.assertEqual(capacity(C), MIN_SLOTS)
        C.__static_attributes__ = names
        self.assertEqual(capacity(C), 10)
        del C.__static_attributes__
        self.assertEqual(capacity(C), MIN_SLOTS)
        C.__static_attributes__ = names
        o = C()
        for name in names:
            setattr(o, name, name)
        self.assertIsNotNone(shape_of(o))

    def test_static_attributes_set_after_an_instance(self):
        names = tuple(f"n{i}" for i in range(8))
        C = make_static(0)
        C.__new__(C)
        C.__static_attributes__ = names
        self.assertEqual(C.__static_attributes__, names)
        self.assertEqual(capacity(C), MIN_SLOTS)

    def test_static_attributes_set_with_a_subclass(self):
        names = tuple(f"n{i}" for i in range(8))
        C = make_static(0)
        class S(C):
            pass
        C.__static_attributes__ = names
        self.assertEqual(capacity(C), MIN_SLOTS)
        self.assertEqual(capacity(S), MIN_SLOTS)

    def test_dataclass_capacity(self):
        fields = [f"f{i}" for i in range(8)]
        D = dataclasses.make_dataclass("D", fields)
        self.assertEqual(capacity(D), 10)
        o = D(*range(8))
        self.assertIsNotNone(shape_of(o))
        self.assertEqual(slots_of(o)[:8], tuple(range(8)))
        # Instantiated before the decorator: the capacity stays.
        class C:
            pass
        C.__annotations__ = dict.fromkeys(fields, int)
        C()
        D = dataclasses.dataclass(C)
        self.assertEqual(capacity(D), MIN_SLOTS)
        self.assertEqual(vars(D(*range(8))), dict(zip(fields, range(8))))

    def test_slots_are_positions(self):
        C = make_class()
        o = C()
        self.assertEqual(plan(o, "x").offset, C.__basicsize__)
        self.assertEqual(plan(o, "y").offset, C.__basicsize__ + 8)
        self.assertEqual(slots_of(o), (1, 2) + (None,) * (MIN_SLOTS - 2))

    def test_sizeof_counts_slots(self):
        C = make_static(10)
        self.assertEqual(sys.getsizeof(C()), C.__basicsize__ + 12 * 8)
        class L(list):
            pass
        self.assertEqual(sys.getsizeof(L()), sys.getsizeof([]) + MIN_SLOTS * 8)

    def test_slots_match_shape(self):
        C = make_class()
        o = C()
        o.z = 3
        del o.y
        names = _testinternalcapi.shape(o)[2]
        for i, value in enumerate(slots_of(o)):
            named = i < len(names) and names[i] is not None
            self.assertEqual(named, value is not None, i)

    def test_holes_count_against_capacity(self):
        class C:
            pass
        o = C()
        names = [f"a{i}" for i in range(MIN_SLOTS)]
        for name in names:
            setattr(o, name, name)
        del o.a1
        self.assertIsNotNone(shape_of(o))
        # The hole is not reused, and there is no slot left.
        o.z = "z"
        self.assertIsNone(shape_of(o))
        self.assertEqual(list(vars(o)), [n for n in names if n != "a1"] + ["z"])

    def test_referents_in_both_modes(self):
        o = make_class()()
        x = [1]
        o.x = x
        self.assertIn(x, gc.get_referents(o))
        d = o.__dict__
        refs = gc.get_referents(o)
        self.assertIn(d, refs)
        self.assertNotIn(x, refs)

    def test_transient_types_with_many_shapes(self):
        refs = []
        for i in range(10):
            class C:
                pass
            for order in itertools.permutations("abcd", 3):
                o = C()
                for name in order:
                    setattr(o, name, name)
            refs.append(weakref.ref(C))
            del C, o
        gc.collect()
        self.assertEqual([r() for r in refs], [None] * len(refs))


@cpython_only
class DictionaryModeTests(unittest.TestCase):
    """Slot 0 holds the dict or NULL, and the other slots are NULL."""

    def test_dict_identity_and_mutation(self):
        o = make_class()()
        d = o.__dict__
        self.assertIs(o.__dict__, d)
        self.assertEqual(slots_of(o), (d,))
        d["z"] = 3
        self.assertEqual(o.z, 3)
        o.w = 4
        self.assertEqual(d["w"], 4)
        del o.x
        self.assertNotIn("x", d)

    def test_dict_assignment_and_deletion(self):
        o = make_class()()
        d = {"a": 1}
        o.__dict__ = d
        self.assertIs(o.__dict__, d)
        self.assertEqual(slots_of(o), (d,))
        del o.__dict__
        self.assertEqual(slots_of(o), (None,))
        self.assertFalse(hasattr(o, "a"))
        self.assertEqual(o.__dict__, {})
        o.b = 2
        self.assertEqual(vars(o), {"b": 2})

    def test_dict_survives_its_owner(self):
        o = make_class()()
        d = o.__dict__
        del o
        gc.collect()
        self.assertEqual(d, {"x": 1, "y": 2})

    def test_entries_into_dictionary_mode(self):
        C = make_class()
        for enter in (lambda o: o.__dict__, vars, lambda o: o.__getstate__(),
                      _testinternalcapi.object_dict_ptr):
            o = C()
            enter(o)
            self.assertIsNone(shape_of(o))
            self.assertEqual(vars(o), {"x": 1, "y": 2})
        # An instance without attributes has no state to convert.
        o = C.__new__(C)
        self.assertIsNone(o.__getstate__())
        self.assertEqual(shape_of(o), 0)

    def test_get_dict_ptr(self):
        o = make_class()()
        self.assertEqual(_testinternalcapi.object_dict_ptr(o),
                         (True, {"x": 1, "y": 2}, False))
        self.assertIs(o.__dict__, _testinternalcapi.object_dict_ptr(o)[1])
        class S:
            __slots__ = ()
        self.assertEqual(_testinternalcapi.object_dict_ptr(S()),
                         (False, None, False))

    def test_get_dict_ptr_out_of_memory(self):
        C = make_class()
        failures = 0
        for start in range(10):
            o = C()
            _testcapi.set_nomemory(start, start + 1)
            try:
                res = _testinternalcapi.object_dict_ptr(o)
            except MemoryError:
                # The helper's own result
                continue
            finally:
                _testcapi.remove_mem_hooks()
            if not res[0]:
                failures += 1
                # NULL, no exception, and the object unchanged
                self.assertFalse(res[2])
                self.assertIsNotNone(shape_of(o))
                self.assertEqual(slots_of(o)[:2], (1, 2))
        self.assertGreater(failures, 0)

    def test_class_assignment_across_capacities(self):
        Small = make_static(0)
        Big = make_static(40, prefix="b")
        self.assertEqual((capacity(Small), capacity(Big)),
                         (MIN_SLOTS, MAX_SLOTS))
        o = Small()
        o.a, o.b = 1, 2
        # o keeps Small's six slots: only slot 0 may be used from now on.
        for cls in (Big, Small, Big, Small):
            o.__class__ = cls
            self.assertIsNone(shape_of(o))
            self.assertEqual(slots_of(o), (o.__dict__,))
            o.c = len(vars(o))
            _testcapi.clear_managed_dict(o)
            self.assertEqual(slots_of(o), (None,))
            self.assertIsNone(shape_of(o))
            for i in range(MAX_SLOTS + 2):
                setattr(o, f"n{i}", i)
            o.__dict__ = {"a": 1, "b": 2}
            gc.collect()
            self.assertEqual(gc.get_referents(o).count(o.__dict__), 1)
            self.assertEqual(vars(o), {"a": 1, "b": 2})

    def test_c_managed_dict_type(self):
        T = _testcapi.HeapCTypeWithManagedDict
        class D(T):
            pass
        for cls in (T, D):
            o = cls()
            o.a = [1]
            self.assertEqual(shape_of(o), 1)
            _testcapi.clear_managed_dict(o)
            self.assertEqual(shape_of(o), 0)
            self.assertFalse(hasattr(o, "a"))
            o.b = 2
            o.__dict__
            _testcapi.clear_managed_dict(o)
            self.assertIsNone(shape_of(o))
            self.assertEqual(vars(o), {})
            gc.collect()



@cpython_only
class DictionaryModeReasonTests(unittest.TestCase):
    """Each entry into dictionary mode is counted once, for one reason,
    after it succeeds (Py_STATS builds)."""

    def setUp(self):
        state = _testinternalcapi.dict_mode_counts()
        if state is None:
            self.skipTest("requires --enable-pystats")
        if not state[0]:
            sys._stats_on()
            # Leave nothing for this process to dump at exit.
            self.addCleanup(sys._stats_clear)
            self.addCleanup(sys._stats_off)

    def entries(self, operation, *args):
        """The counters that OPERATION(*ARGS) changes, by how much."""
        before = _testinternalcapi.dict_mode_counts()[1]
        operation(*args)
        after = _testinternalcapi.dict_mode_counts()[1]
        return {k: after[k] - before[k] for k in after if after[k] != before[k]}

    def assertEntersOnce(self, reason, o, operation, *args, again=None):
        self.assertIsNotNone(shape_of(o))
        self.assertEqual(self.entries(operation, o, *args), {reason: 1})
        self.assertIsNone(shape_of(o))
        # An object already in dictionary mode is not counted again.
        self.assertEqual(self.entries(operation, o, *(again or args)), {})

    def test_requests(self):
        C = make_class()
        for request in (lambda o: o.__dict__, vars,
                        lambda o: o.__getstate__(),
                        _testinternalcapi.object_dict_ptr):
            with self.subTest(request=request):
                self.assertEntersOnce("on_request", C(), request)

    def test_str_name_store_and_load(self):
        C = make_class()
        self.assertEntersOnce("str_name", C(), setattr, StrName("z"), 3)
        self.assertEntersOnce("str_name", C(), getattr, StrName("x"))

    def test_capacity(self):
        class C:
            pass
        o = C()
        for i in range(MIN_SLOTS):
            setattr(o, f"a{i}", i)
        self.assertEntersOnce("capacity", o, setattr, "z", 0)

    def exhaust_shapes(self, C):
        for i in range(MAX_SHAPES + 1):
            p = C()
            setattr(p, f"n{i}", i)
            if shape_of(p) is None:
                break
        self.assertEqual(shape_count(C), MAX_SHAPES)

    def test_shape_limit(self):
        class C:
            pass
        o = C()
        o.a = 1
        self.exhaust_shapes(C)
        self.assertEntersOnce("shape_limit", o, setattr, "b", 2)

    def test_full_object_and_shape_limit(self):
        class C:
            pass
        full = []
        for _ in range(2):
            o = C()
            for i in range(MIN_SLOTS):
                setattr(o, f"a{i}", i)
            full.append(o)
        self.exhaust_shapes(C)
        # An ADD refused on both counts is for capacity; a REMOVE needs
        # no slot, so it is refused for the shape limit.
        self.assertEntersOnce("capacity", full[0], setattr, "z", 0)
        self.assertEntersOnce("shape_limit", full[1], delattr, "a1",
                              again=("a2",))

    def test_class_assignment(self):
        C = make_class()
        class D(C):
            pass
        def assign(o):
            o.__class__ = D if type(o) is C else C
        self.assertEntersOnce("class_assigned", C(), assign)

    def test_dict_assignment_and_deletion(self):
        C = make_class()
        def assign(o):
            o.__dict__ = {"a": 1}
        def delete(o):
            del o.__dict__
        self.assertEntersOnce("dict_assigned", C(), assign)
        self.assertEntersOnce("dict_deleted", C(), delete)
        o = C()
        self.assertEqual(self.entries(assign, o), {"dict_assigned": 1})
        self.assertEqual(self.entries(delete, o), {})

    def test_failed_conversions_are_not_counted(self):
        C = make_class()
        for request in (lambda o: o.__dict__, _testinternalcapi.object_dict_ptr):
            failures = 0
            for start in range(10):
                o = C()
                before = _testinternalcapi.dict_mode_counts()[1]
                _testcapi.set_nomemory(start, start + 1)
                try:
                    res = request(o)
                except MemoryError:
                    res = None
                finally:
                    _testcapi.remove_mem_hooks()
                after = _testinternalcapi.dict_mode_counts()[1]
                if shape_of(o) is None:
                    self.assertEqual(after["on_request"],
                                     before["on_request"] + 1)
                    continue
                failures += 1
                # _PyObject_GetDictPtr() fails with no exception.
                if res is not None:
                    self.assertEqual(res, (False, None, False))
                self.assertEqual(after, before)
            self.assertGreater(failures, 0)

@cpython_only
class VariableSizeTests(unittest.TestCase):
    """Subclasses of variable-size types keep a dict word of their own."""

    def check(self, o, expected):
        self.assertFalse(type(o).__flags__ & (1 << 4))  # no MANAGED_DICT
        self.assertIsNone(shape_of(o))
        o.attr = [expected]
        o.other = "other"
        gc.collect()
        self.assertEqual(o.attr, [expected])
        self.assertEqual(vars(o), {"attr": [expected], "other": "other"})
        self.assertGreater(sys.getsizeof(o), 0)
        del o.attr
        self.assertFalse(hasattr(o, "attr"))
        gc.collect()
        self.assertEqual(o.other, "other")

    def test_int_subclass(self):
        class I(int):
            pass
        # Zero, one and many digits: the dict word follows the digits.
        for v in (0, 1, -5, 2**30, 2**200, -(2**1000)):
            i = I(v)
            self.check(i, v)
            self.assertEqual(i, v)

    def test_tuple_subclass(self):
        class T(tuple):
            pass
        for n in (0, 1, 5, 40):
            t = T(range(n))
            self.check(t, n)
            self.assertEqual(t, tuple(range(n)))

    def test_bytes_subclass(self):
        class B(bytes):
            pass
        for n in (0, 1, 100):
            b = B(b"x" * n)
            self.check(b, n)
            self.assertEqual(b, b"x" * n)

    def test_sibling_layouts(self):
        # Subclasses that each add the dict word share one layout: they
        # exchange __class__ and __bases__, and combine as bases.
        cases = ((int, (5,)), (int, (2**100,)), (tuple, ((1, 2),)),
                 (bytes, (b"ab",)), (_testcapi.HeapCCollection, ([1], [2])))
        for base, args in cases:
            with self.subTest(base=base.__name__):
                class A(base):
                    pass
                class B(base):
                    pass
                o = A(*args)
                o.attr = [1]
                o.__class__ = B
                self.assertIs(type(o), B)
                gc.collect()
                self.assertEqual(vars(o), {"attr": [1]})
                if base is _testcapi.HeapCCollection:
                    self.assertEqual(tuple(o), args)
                else:
                    self.assertEqual(o, base(*args))
                class C(A, B):
                    pass
                c = C(*args)
                c.attr = "c"
                gc.collect()
                self.assertEqual(vars(c), {"attr": "c"})
                class D(A):
                    pass
                D.__bases__ = (B,)
                self.assertIs(D.__base__, B)

    def test_c_type_dict_word_starts_empty(self):
        # A variable-size C type that asks for a managed dict gets a dict
        # word after its items; PyObject_GC_NewVar clears it.
        T = _testcapi.HeapCVarWithManagedDict
        self.assertFalse(T.__flags__ & (1 << 4))  # no MANAGED_DICT
        self.assertLess(T.__dictoffset__, 0)
        for n in (0, 1, 7):
            # Reused memory is not zeroed.
            garbage = [T(*[object()] * n) for _ in range(100)]
            del garbage
            gc.collect()
            objects = [T(*([i] for i in range(n))) for _ in range(100)]
            for o in objects:
                self.assertEqual(vars(o), {})
                o.a = n
            gc.collect()
            for o in objects:
                self.assertEqual(o.a, n)
                self.assertEqual([o[i] for i in range(n)], [[i] for i in range(n)])
            _testcapi.clear_managed_dict(objects[0])
            self.assertFalse(hasattr(objects[0], "a"))

    def test_c_type_resize_moves_dict_word(self):
        # The dict word follows the items, so a resize moves it.
        T = _testcapi.HeapCVarWithManagedDict
        for before, after in ((0, 5), (5, 0), (3, 3), (2, 40), (40, 1)):
            with self.subTest(before=before, after=after):
                value = [object()]
                o = _testcapi.heapvarmanaged_resize(T, before, after, value)
                gc.collect()
                self.assertEqual(len(o), after)
                self.assertEqual(vars(o), {"a": value})
                self.assertIs(o.a, value)

    def test_items_at_end_subclass(self):
        # The items follow tp_basicsize, so the dict word is before them.
        class H(_testcapi.HeapCCollection):
            pass
        for items in ((), ([1],), tuple([i] for i in range(7))):
            h = H(*items)
            self.check(h, len(items))
            self.assertEqual(tuple(h), items)
            for i in range(10):
                setattr(h, f"a{i}", [i])
            gc.collect()
            self.assertEqual(tuple(h), items)
            del h
            gc.collect()


class StrNameWithEq(str):
    """A name whose comparisons run Python code."""
    eq_calls = 0
    def __hash__(self):
        return str.__hash__(self)
    def __eq__(self, other):
        StrNameWithEq.eq_calls += 1
        return str.__eq__(self, other)


@cpython_only
class NonExactNameTests(unittest.TestCase):
    """A name that is not an exact str never takes a slot."""

    def test_load_converts(self):
        o = make_class()()
        before = StrNameWithEq.eq_calls
        self.assertEqual(getattr(o, StrNameWithEq("x")), 1)
        self.assertIsNone(shape_of(o))
        self.assertGreater(StrNameWithEq.eq_calls, before)
        self.assertEqual(getattr_value(o, StrNameWithEq("y"))[0], 2)
        with self.assertRaises(AttributeError):
            getattr(o, StrNameWithEq("z"))

    def test_data_descriptor_wins_before_storage(self):
        class P:
            @property
            def p(self):
                return "prop"
        o = P()
        o.x = 1
        self.assertEqual(getattr(o, StrNameWithEq("p")), "prop")
        self.assertIsNotNone(shape_of(o))

    def test_store_and_delete(self):
        o = make_class()()
        setattr(o, StrNameWithEq("z"), 3)
        self.assertIsNone(shape_of(o))
        self.assertEqual(vars(o), {"x": 1, "y": 2, "z": 3})
        delattr(o, StrNameWithEq("x"))
        self.assertEqual(list(vars(o)), ["y", "z"])
        with self.assertRaises(AttributeError):
            delattr(o, StrNameWithEq("x"))
        p = make_class()()
        with self.assertRaises(AttributeError):
            delattr(p, StrName("missing"))
        self.assertEqual(vars(p), {"x": 1, "y": 2})

    def test_load_out_of_memory(self):
        # The load converts first; a failed conversion leaves the object.
        C = make_class()
        failures = 0
        for start in range(10):
            o = C()
            name = StrName("x")
            value = None
            _testcapi.set_nomemory(start, start + 1)
            try:
                value = getattr(o, name)
            except MemoryError:
                failed = True
            else:
                failed = False
            finally:
                _testcapi.remove_mem_hooks()
            if failed:
                failures += 1
                self.assertIsNotNone(shape_of(o))
                self.assertEqual(slots_of(o)[:2], (1, 2))
            else:
                self.assertEqual(value, 1)
        self.assertGreater(failures, 0)


@cpython_only
class ConversionTests(unittest.TestCase):
    """Conversion to dictionary mode, with a collection just before the
    dict is published: the one point where a missing root loses a value."""

    VALUES = (0, -1, 2**40, -0.0, 0.0, math.inf, -math.inf, math.nan, 1.5)

    def setUp(self):
        old = _testinternalcapi.set_conversion_collect(True)
        self.addCleanup(_testinternalcapi.set_conversion_collect, old)

    def make(self, extra=0):
        # Values that nothing else holds: inline numbers, and new objects.
        C = make_static(len(self.VALUES) + 2 + extra)
        o = C.__new__(C)
        for i, v in enumerate(self.VALUES):
            setattr(o, f"v{i}", v)
        o.list = [object()]
        o.text = "".join(["fr", "esh"])
        return o

    def check(self, o):
        for i, want in enumerate(self.VALUES):
            got = getattr(o, f"v{i}")
            if math.isnan(want):
                self.assertTrue(math.isnan(got))
                continue
            self.assertEqual(got, want)
            self.assertIs(type(got), type(want))
            if isinstance(want, float):
                self.assertEqual(math.copysign(1, got), math.copysign(1, want))
        self.assertEqual(len(o.list), 1)
        self.assertEqual(o.text, "fresh")

    def names(self):
        return [f"v{i}" for i in range(len(self.VALUES))] + ["list", "text"]

    def test_entering_dictionary_mode(self):
        o = self.make()
        o.__dict__
        self.assertIsNone(shape_of(o))
        self.check(o)
        self.assertEqual(list(vars(o)), self.names())

    def test_limit_with_a_new_value(self):
        o = self.make()
        # make_static(n) has n + 2 slots: fill them, then one more converts.
        o.f1 = o.f2 = 0
        self.assertIsNotNone(shape_of(o))
        o.new = [object(), "".join(["n", "ew"])]
        self.assertIsNone(shape_of(o))
        self.check(o)
        self.assertEqual(o.new[1], "new")
        self.assertEqual(list(vars(o)), self.names() + ["f1", "f2", "new"])

    def test_non_exact_name_with_a_new_value(self):
        o = self.make()
        setattr(o, StrName("s"), [object()])
        self.check(o)
        self.assertEqual(len(o.s), 1)


@cpython_only
@requires_specialization
class InstanceLayoutSpecializationTests(unittest.TestCase):
    """The handlers of the slot layout, and the sites that stay generic."""

    def test_speculative_prepare_out_of_memory(self):
        failures = successes = 0
        for start in range(20):
            # A fresh type: no shape has a z edge yet.
            C = make_class()
            @reset_code
            def put(o):
                o.z = None
            other = C.__new__(C)
            other.z = 0
            target = C()
            # A generic REPLACE on another shape, then the site specialises
            # on target, whose add has no edge to follow yet.
            put(other)
            count = shape_count(C)
            _testcapi.set_nomemory(start)
            try:
                put(target)
            except MemoryError:
                failed = True
            else:
                failed = False
            finally:
                _testcapi.remove_mem_hooks()
            if failed:
                failures += 1
                # Neither the specialiser nor the generic store added a shape.
                self.assertEqual(shape_count(C), count)
                self.assertEqual(slots_of(target)[:3], (1, 2, None))
                self.assertNotIn("STORE_ATTR_SHAPE_TRANSITION", opnames(put))
            else:
                successes += 1
                self.assertIsNone(target.z)
        self.assertGreater(failures, 0)
        self.assertGreater(successes, 0)

    def test_method_and_nondescriptor_shape(self):
        class M:
            f = len
            def __init__(self):
                self.a = 0
            def m(self):
                return 1
        @reset_code
        def call(o):
            return o.m()
        @reset_code
        def get(o):
            return o.f
        o = M()
        for _ in range(THRESHOLD):
            self.assertEqual(call(o), 1)
            self.assertIs(get(o), len)
        self.assertIn("LOAD_ATTR_METHOD_SHAPE", opnames(call))
        self.assertIn("LOAD_ATTR_NONDESCRIPTOR_SHAPE", opnames(get))
        # A second shape that shadows both names.
        s = M()
        s.m = lambda: 2
        s.f = "inst"
        for _ in range(2 * COOLDOWN):
            self.assertEqual(call(o), 1)
            self.assertEqual(call(s), 2)
            self.assertIs(get(o), len)
            self.assertEqual(get(s), "inst")
        # Shadowing through the dict, after the site was specialised.
        o.__dict__["m"] = lambda: 3
        self.assertEqual(call(o), 3)

    def warmed_sites(self):
        class K:
            __slots__ = ("v", "__dict__")
            f = len
            def __init__(self):
                self.v = "slot"
            def m(self):
                return "method"
        @reset_code
        def call(o):
            return o.m()
        @reset_code
        def get_f(o):
            return o.f
        @reset_code
        def get_v(o):
            return o.v
        @reset_code
        def get_d(o):
            return o.d
        d = K()
        d.d = "dict"
        d.__dict__
        o = K()
        for _ in range(THRESHOLD):
            call(o), get_f(o), get_v(o), get_d(d)
        self.assertIn("LOAD_ATTR_METHOD_SHAPE", opnames(call))
        self.assertIn("LOAD_ATTR_NONDESCRIPTOR_SHAPE", opnames(get_f))
        self.assertIn("LOAD_ATTR_SLOT", opnames(get_v))
        self.assertIn("LOAD_ATTR_WITH_HINT", opnames(get_d))
        def check(m, f, v, dv):
            self.assertEqual(call(o), m)
            self.assertEqual(get_f(o), f)
            self.assertEqual(get_v(o), v)
            self.assertEqual(get_d(d), dv)
        return K, o, d, check

    def test_class_mutation_after_warming(self):
        K, o, d, check = self.warmed_sites()
        check("method", len, "slot", "dict")
        K.m = lambda self: "new"
        K.f = abs
        check("new", abs, "slot", "dict")
        K.v = "class"   # replaces the member descriptor
        K.d = property(lambda self: "prop")
        check("new", abs, "class", "prop")

    def test_version_exhaustion_after_warming(self):
        K, o, d, check = self.warmed_sites()
        for i in range(1001):
            K.f = i
            _testcapi.type_assign_version(K)
            if i % 100 == 0:
                check("method", i, "slot", "dict")
        self.assertEqual(_testcapi.type_get_version(K), 0)
        K.m = lambda self: "late"
        check("late", 1000, "slot", "dict")

    def test_with_hint_receivers(self):
        class I(int):
            pass
        class E(Exception):
            pass
        class C:
            pass
        def dict_mode():
            o = C()
            o.__dict__
            return o
        for make in (lambda: I(5), lambda: I(2**100), lambda: E("e"), dict_mode):
            with self.subTest(make=make):
                @reset_code
                def get(o):
                    return o.v
                @reset_code
                def put(o, v):
                    o.v = v
                objects = [make() for _ in range(THRESHOLD + 1)]
                for i, o in enumerate(objects):
                    o.v = None
                    put(o, i)
                    self.assertEqual(get(o), i)
                self.assertIn("LOAD_ATTR_WITH_HINT", opnames(get))
                self.assertIn("STORE_ATTR_WITH_HINT", opnames(put))
                gc.collect()
                self.assertEqual([get(o) for o in objects],
                                 list(range(len(objects))))

    def test_store_over_class_attribute_stays_generic(self):
        class K:
            v = "class"
            def __init__(self):
                self.a = 0
        @reset_code
        def put(o, x):
            o.v = x
        objects = [K() for _ in range(2 * THRESHOLD)]
        for i, o in enumerate(objects):
            put(o, i)
        names = opnames(put)
        self.assertIn("STORE_ATTR", names)
        self.assertFalse({"STORE_ATTR_SHAPE", "STORE_ATTR_SHAPE_TRANSITION"} & names)
        self.assertEqual([o.v for o in objects], list(range(len(objects))))
        self.assertEqual(K.v, "class")


def shaped(n):
    """n objects of one type, each of its own shape, all with x."""
    class P:
        def m(self):
            return "method"
    objects = []
    for i in range(n):
        o = P()
        setattr(o, f"pad{i}", i)
        o.x = i
        objects.append(o)
    return P, objects


@cpython_only
@requires_specialization
class ShapePolyTests(unittest.TestCase):
    """LOAD_ATTR_SHAPE_POLY: a site's two to four shapes, of one type or
    several, each with a slot or a method."""

    def warm(self, f, objects, expected, rounds=2 * COOLDOWN):
        for _ in range(rounds):
            for o, want in zip(objects, expected):
                self.assertEqual(f(o), want)

    def test_slot_rows(self):
        for n in (2, 4):
            with self.subTest(shapes=n):
                @reset_code
                def get(o):
                    return o.x
                P, objects = shaped(n)
                self.warm(get, objects, range(n))
                self.assertIn("LOAD_ATTR_SHAPE_POLY", opnames(get))

    def test_five_shapes_are_megamorphic(self):
        @reset_code
        def get(o):
            return o.x
        P, objects = shaped(5)
        # Only the shape without a row misses, so the fifth row is seen late.
        self.warm(get, objects, range(5), rounds=8 * COOLDOWN)
        self.assertNotIn("LOAD_ATTR_SHAPE_POLY", opnames(get))
        self.assertNotIn("LOAD_ATTR_SHAPE", opnames(get))

    def test_method_rows(self):
        @reset_code
        def call(o):
            return o.m()
        P, objects = shaped(3)
        class Q:
            def m(self):
                return "other"
        objects.append(Q())
        self.warm(call, objects, ["method"] * 3 + ["other"])
        self.assertIn("LOAD_ATTR_SHAPE_POLY", opnames(call))

    def test_slot_and_method_rows(self):
        # At a call-pair site a slot row pushes NULL, a method row the receiver.
        @reset_code
        def call(o):
            return o.m()
        P, objects = shaped(2)
        shadowed = P()
        shadowed.m = lambda: "instance"
        objects.append(shadowed)
        self.warm(call, objects, ["method", "method", "instance"])
        self.assertIn("LOAD_ATTR_SHAPE_POLY", opnames(call))

    def test_types_with_slot_and_class_attribute_rows(self):
        # A class attribute that is not a method has its own handler, so the
        # site does not take the polymorphic one.
        class C:
            x = "class"
        @reset_code
        def get(o):
            return o.x
        P, objects = shaped(2)
        objects.append(C())
        self.warm(get, objects, [0, 1, "class"])
        self.assertNotIn("LOAD_ATTR_SHAPE_POLY", opnames(get))

    def test_new_shape_after_warming(self):
        @reset_code
        def get(o):
            return o.x
        P, objects = shaped(2)
        self.warm(get, objects, range(2))
        late = P()
        late.x = "late"
        self.assertEqual(get(late), "late")
        self.assertEqual([get(o) for o in objects], [0, 1])

    def test_class_mutation_after_warming(self):
        @reset_code
        def get(o):
            return o.x
        @reset_code
        def call(o):
            return o.m()
        P, objects = shaped(3)
        self.warm(get, objects, range(3))
        self.warm(call, objects, ["method"] * 3)
        self.assertIn("LOAD_ATTR_SHAPE_POLY", opnames(get))
        self.assertIn("LOAD_ATTR_SHAPE_POLY", opnames(call))
        P.m = lambda self: "new"
        P.x = property(lambda self: "prop")
        self.assertEqual([get(o) for o in objects], ["prop"] * 3)
        self.assertEqual([call(o) for o in objects], ["new"] * 3)

    def test_version_exhaustion_after_warming(self):
        @reset_code
        def call(o):
            return o.m()
        P, objects = shaped(2)
        self.warm(call, objects, ["method"] * 2)
        self.assertIn("LOAD_ATTR_SHAPE_POLY", opnames(call))
        for i in range(1001):
            P.m = lambda self, i=i: i
            _testcapi.type_assign_version(P)
            if i % 100 == 0:
                self.assertEqual([call(o) for o in objects], [i, i])
        self.assertEqual(_testcapi.type_get_version(P), 0)
        self.assertEqual([call(o) for o in objects], [1000, 1000])

    def test_rows_of_dead_types(self):
        # Rows borrow shapes; a dead type's shapes may be reused at the same
        # addresses by new types, whose versions differ.
        @reset_code
        def get(o):
            return o.x
        refs = []
        for round in range(10):
            P, objects = shaped(2)
            for _ in range(THRESHOLD + 1):
                self.assertEqual([get(o) for o in objects], [0, 1])
            refs.append(weakref.ref(P))
            del P, objects
            gc.collect()
        self.assertEqual([r() for r in refs], [None] * len(refs))


@cpython_only
class DebugAllocatorTests(unittest.TestCase):

    def test_under_mimalloc_debug(self):
        # The address map with the debug hooks' offset: conservative roots,
        # sweep and conversion.
        script_helper.assert_python_ok(
            "-m", "unittest",
            "test.test_shapes.LayoutTests",
            "test.test_shapes.DictionaryModeTests",
            "test.test_shapes.VariableSizeTests",
            "test.test_shapes.ConversionTests",
            "test.test_shapes.NonExactNameTests",
            "test.test_shapes.InstanceLayoutSpecializationTests",
            "test.test_shapes.ShapePolyTests",
            PYTHONMALLOC="mimalloc_debug")


shape_version = _testinternalcapi.shape_version
generalisations = _testinternalcapi.shape_generalisations


def kinds(o):
    """The kind of each slot of o's shape, None for a hole."""
    return tuple(None if s is None else s[1]
                 for s in _testinternalcapi.shape(o)[2])


class IntSub(int):
    pass


class FloatSub(float):
    pass


# Values of each kind, and values of no inline kind (note 09 section 5).
INT_VALUES = [0, 1, -1, 2**31 - 1, -2**31]
FLOAT_VALUES = [1.5, 0.0, -0.0, math.inf, -math.inf]
BOXED_VALUES = [2**31, -2**31 - 1, 2**100, math.nan, True, False,
                IntSub(1), FloatSub(1.0), "s", None, [1]]


@cpython_only
class SlotKindTests(unittest.TestCase):
    """Slot kinds as promises (note 09 section 5)."""

    def test_first_store_picks_the_kind(self):
        for values, kind in ((INT_VALUES, I32), (FLOAT_VALUES, F64),
                             (BOXED_VALUES, BOXED)):
            for v in values:
                with self.subTest(v=v):
                    class C:
                        pass
                    o = C()
                    o.a = v
                    self.assertEqual(kinds(o), (kind,))
                    self.assertIs(type(o.a), type(v))
                    if v == v:
                        self.assertEqual(o.a, v)

    def test_values_of_the_kind_keep_it(self):
        class C:
            pass
        o = C()
        o.i = 1
        o.f = 1.5
        for v in INT_VALUES:
            o.i = v
        for v in FLOAT_VALUES:
            o.f = v
        self.assertEqual(kinds(o), (I32, F64))
        self.assertEqual(o.f, -math.inf)

    def test_generalise_in_place(self):
        class C:
            pass
        a = C(); a.x = 1; a.y = 2.5          # root -> x -> xy
        b = C(); b.x = 3; b.y = 4.5          # the same shapes
        c = C(); c.x = 5                     # x, the owner of x
        d = C(); d.y = 6                     # root -> y, another subtree
        e = C()                              # root
        before = {n: shape_version(o) for n, o in zip("acde", (a, c, d, e))}
        count, reversioned = generalisations()
        a.x = "s"
        self.assertEqual(kinds(a), (BOXED, F64))
        self.assertEqual(kinds(b), (BOXED, F64))
        self.assertEqual(kinds(c), (BOXED,))
        self.assertEqual(kinds(d), (I32,))
        # The owner and its subtree share one fresh version; nothing else
        # changes.
        self.assertNotEqual(shape_version(a), before["a"])
        self.assertEqual(shape_version(a), shape_version(c))
        self.assertGreater(shape_version(a), before["a"])
        self.assertEqual(shape_version(d), before["d"])
        self.assertEqual(shape_version(e), before["e"])
        self.assertEqual(generalisations(), (count + 1, reversioned + 2))
        # No value moved.
        self.assertEqual((a.x, a.y, b.x, b.y, c.x, d.y), ("s", 2.5, 3, 4.5, 5, 6))
        # A new object along the same edges finds the field BOXED.
        f = C(); f.x = 7; f.y = 8.5
        self.assertEqual(kinds(f), (BOXED, F64))
        self.assertEqual(shape_version(f), shape_version(a))
        # BOXED takes every value without generalising again.
        f.x = 1.5
        self.assertEqual(generalisations()[0], count + 1)

    def test_int_and_float_alternate(self):
        class C:
            pass
        objects = []
        for i in range(20):
            o = C()
            o.v = i if i % 2 == 0 else i + 0.5
            o.w = 1
            objects.append(o)
        self.assertEqual(kinds(objects[0]), (BOXED, I32))
        for i, o in enumerate(objects):
            self.assertEqual(o.v, i if i % 2 == 0 else i + 0.5)
            self.assertIs(type(o.v), int if i % 2 == 0 else float)

    def test_generalise_then_add_and_delete(self):
        class C:
            pass
        # Note 09's example: the delete returns to a shape whose x is BOXED.
        o = C(); o.x = 1; o.y = 2; o.x = "s"
        del o.y
        self.assertEqual(kinds(o), (BOXED,))
        self.assertEqual(o.x, "s")
        o.y = 3                                  # the existing edge
        self.assertEqual(kinds(o), (BOXED, I32))
        o.z = 4.5                                # a new edge below
        self.assertEqual(kinds(o), (BOXED, I32, F64))
        # Delete-last back to the parent, then re-add.
        del o.z
        o.z = 5.5
        self.assertEqual(kinds(o), (BOXED, I32, F64))
        # A hole: the field generalised, then another deleted.
        p = C(); p.x = 1; p.y = 2; p.z = 3.5
        p.y = None
        self.assertEqual(kinds(p), (BOXED, BOXED, F64))
        del p.x
        self.assertEqual(kinds(p), (None, BOXED, F64))
        self.assertEqual((p.y, p.z), (None, 3.5))
        p.w = 7
        self.assertEqual(kinds(p), (None, BOXED, F64, I32))
        q = C(); q.x = 1; q.y = 2
        self.assertEqual(kinds(q), (BOXED, BOXED))

    def test_store_plans(self):
        class C:
            pass
        o = C(); o.x = 1
        self.assertEqual(store_plan(o, "x", False, 2).outcome, "REPLACE")
        self.assertEqual(store_plan(o, "x", False, 2).slot_kind, I32)
        self.assertEqual(store_plan(o, "x", False, 2.5).outcome, "GENERALISE")
        self.assertEqual(store_plan(o, "x", False, "s").outcome, "GENERALISE")
        self.assertEqual(store_plan(o, "y", False, 2.5).slot_kind, F64)
        p = C(); p.x = 1; p.y = 2.5
        self.assertEqual(store_plan(o, "y", False, "s").slot_kind, F64)
        self.assertEqual(plan(p, "y").slot_kind, F64)
        self.assertEqual(plan(p, "x").slot_kind, I32)

    def test_identity_after_generalising(self):
        class C:
            pass
        big = 2**40
        s = "".join(["a", "b"])
        o = C(); o.x = 1
        o.x = big
        self.assertIs(o.x, big)
        o.x = s
        self.assertIs(o.x, s)
        o.x = 1.5
        self.assertIs(o.x, 1.5)

    def test_dictionary_mode_copies_values(self):
        class C:
            pass
        o = C(); o.i = 1; o.f = 2.5; o.n = math.nan; o.b = True
        d = o.__dict__
        self.assertEqual(list(d), ["i", "f", "n", "b"])
        self.assertIs(type(d["i"]), int)
        self.assertIs(type(d["f"]), float)
        self.assertTrue(math.isnan(d["n"]))
        self.assertIs(d["b"], True)

    def test_members_are_not_kinded(self):
        class S:
            __slots__ = ("a",)
        class T(S):
            pass
        t = T()
        t.a = 1
        t.b = 1
        self.assertEqual(kinds(t), (I32,))
        self.assertEqual(plan(t, "a").kind, "DATA_DESCR")
        self.assertEqual(plan(t, "a").slot_kind, BOXED)
        t.a = "s"
        self.assertEqual(kinds(t), (I32,))

    def test_stores_through_every_api(self):
        setattr_value = _testcapi.pyvalue.SetAttrValue
        object_setattr = import_module("_testlimitedcapi").object_setattr
        stores = [
            lambda o, v: setattr(o, "x", v),
            lambda o, v: object.__setattr__(o, "x", v),
            lambda o, v: setattr_value(o, "x", v),
            lambda o, v: object_setattr(o, "x", v),
        ]
        for store in stores:
            with self.subTest(store=store):
                class C:
                    pass
                o = C()
                store(o, 1)
                self.assertEqual(kinds(o), (I32,))
                store(o, [1, 2])
                self.assertEqual(kinds(o), (BOXED,))
                # The list is held by the slot alone.
                gc.collect()
                self.assertEqual(o.x, [1, 2])

    def test_type_without_version(self):
        type_get_version = _testcapi.type_get_version
        type_assign_version = _testcapi.type_assign_version
        class C:
            pass
        o = C(); o.x = 1
        # Too many versions: the class gets no more.
        for i in range(1001):
            C.v = i
            type_assign_version(C)
        self.assertEqual(type_get_version(C), 0)
        o.x = "s"
        self.assertEqual(kinds(o), (BOXED,))
        self.assertEqual(shape_version(o), 0)
        self.assertEqual(o.x, "s")

    def test_tags_exhausted(self):
        class C:
            pass
        o = C(); o.x = 1; o.x                    # C has a version
        self.assertNotEqual(shape_version(o), 0)
        old = _testinternalcapi.set_next_type_version(0)
        try:
            o.x = "s"
        finally:
            _testinternalcapi.set_next_type_version(old)
        self.assertEqual(shape_version(o), 0)
        self.assertEqual(kinds(o), (BOXED,))
        self.assertEqual(o.x, "s")

    def test_class_change_after_generalising(self):
        type_get_version = _testcapi.type_get_version
        class C:
            pass
        o = C(); o.x = 1; o.x = "s"
        C.y = 5
        self.assertEqual(o.y, 5)
        # The type's new version reaches every shape again.
        self.assertEqual(shape_version(o), type_get_version(C))
        self.assertEqual(kinds(o), (BOXED,))


@cpython_only
@requires_specialization
class SlotKindSpecializationTests(unittest.TestCase):

    def warm(self, f, *args):
        for _ in range(4 * (THRESHOLD + COOLDOWN)):
            f(*args)

    def check_replace_site(self, first, bad, kind):
        class C:
            pass
        @reset_code
        def put(o, v):
            o.x = v
        o = C()
        o.x = first
        self.warm(put, o, first)
        self.assertIn("STORE_ATTR_SHAPE", opnames(put))
        self.assertEqual(kinds(o), (kind,))
        put(o, bad)
        self.assertIs(o.x, bad)
        self.assertEqual(kinds(o), (BOXED,))
        # The site specialises again, over the BOXED field.
        self.warm(put, o, bad)
        self.assertIn("STORE_ATTR_SHAPE", opnames(put))
        put(o, first)
        self.assertEqual(o.x, first)

    def check_transition_site(self, first, bad, kind):
        class C:
            pass
        @reset_code
        def make(v):
            o = C()
            o.x = v
            return o
        self.warm(make, first)
        self.assertIn("STORE_ATTR_SHAPE_TRANSITION", opnames(make))
        self.assertEqual(kinds(make(first)), (kind,))
        o = make(bad)
        self.assertIs(o.x, bad)
        self.assertEqual(kinds(o), (BOXED,))
        self.warm(make, bad)
        self.assertIn("STORE_ATTR_SHAPE_TRANSITION", opnames(make))
        self.assertEqual(make(first).x, first)

    def test_warmed_int_sites(self):
        for bad in (2**31, -2**31 - 1, 1.5, math.nan, True, IntSub(1), "s"):
            with self.subTest(bad=bad):
                self.check_replace_site(7, bad, I32)
                self.check_transition_site(7, bad, I32)

    def test_warmed_float_sites(self):
        for bad in (math.nan, 1, True, FloatSub(1.5), 2**40, None):
            with self.subTest(bad=bad):
                self.check_replace_site(2.5, bad, F64)
                self.check_transition_site(2.5, bad, F64)

    def test_float_site_takes_infinities_and_zeros(self):
        class C:
            pass
        @reset_code
        def put(o, v):
            o.x = v
        o = C(); o.x = 1.0
        self.warm(put, o, 2.0)
        count = generalisations()[0]
        for v in (math.inf, -math.inf, 0.0, -0.0):
            put(o, v)
            self.assertIs(o.x, v)
        self.assertEqual(generalisations()[0], count)
        self.assertEqual(kinds(o), (F64,))

    def test_specialised_edge_takes_the_value_kind(self):
        # The specialiser prepares a new edge for the first store through
        # it, with the value's kind.
        class C:
            pass
        @reset_code
        def make(v):
            o = C()
            o.f = v
            return o
        self.warm(make, 2.5)
        self.assertEqual(kinds(make(1.5)), (F64,))
        class D:
            pass
        @reset_code
        def make_d():
            o = D()
            o.i = 3
            return o
        self.warm(make_d)
        self.assertEqual(kinds(make_d()), (I32,))

    def test_property_load_on_generalised_shape(self):
        class C:
            @property
            def p(self):
                return self.x
        o = C(); o.x = 1; o.x = "s"
        self.assertNotEqual(shape_version(o),
                            _testcapi.type_get_version(C))
        @reset_code
        def get(o):
            return o.p
        self.warm(get, o)
        self.assertIn("LOAD_ATTR_PROPERTY", opnames(get))
        self.assertEqual(get(o), "s")

    def test_slot_load_on_generalised_shape(self):
        class C:
            pass
        a = C(); a.x = 1; a.y = 2
        b = C(); b.y = 3; b.x = 4
        a.x = "s"
        @reset_code
        def get(o):
            return o.x
        for _ in range(4 * (THRESHOLD + COOLDOWN)):
            self.assertEqual(get(a), "s")
            self.assertEqual(get(b), 4)
        self.assertIn("LOAD_ATTR_SHAPE_POLY", opnames(get))
        @reset_code
        def get_one(o):
            return o.x
        self.warm(get_one, a)
        self.assertIn("LOAD_ATTR_SHAPE", opnames(get_one))

    def test_poly_site_across_generalisations(self):
        # Each generalisation renews the shapes' version; the rows of the
        # old versions are replaced, so the site stays polymorphic.
        class C:
            pass
        a = C(); a.x = 1; a.f0 = 0; a.f1 = 0; a.f2 = 0; a.f3 = 0; a.f4 = 0
        b = C(); b.y = 1; b.x = 2
        @reset_code
        def get(o):
            return o.x
        def warm():
            for _ in range(4 * (THRESHOLD + COOLDOWN)):
                self.assertEqual(get(a), 1)
                self.assertEqual(get(b), 2)
        warm()
        self.assertIn("LOAD_ATTR_SHAPE_POLY", opnames(get))
        for i in range(5):
            setattr(a, f"f{i}", "s")
            warm()
            self.assertIn("LOAD_ATTR_SHAPE_POLY", opnames(get))


if __name__ == "__main__":
    unittest.main()
