"""Shapes: the hidden classes in the object header.

See boil/design/09-object-protocol.md and 04-shapes.md.
"""

import collections
import dis
import gc
import itertools
import sys
import types
import unittest
import weakref
from test.support import cpython_only, requires_specialization, reset_code
from test.support.import_helper import import_module

_testinternalcapi = import_module("_testinternalcapi")
_testcapi = import_module("_testcapi")
getattr_value = _testcapi.pyvalue.GetAttrValue
shape_of = _testinternalcapi.get_object_shape
shape_count = _testinternalcapi.get_type_shape_count
MAX_SHAPES = _testinternalcapi._Py_SHAPE_MAX_PER_TYPE
THRESHOLD = _testinternalcapi.SPECIALIZATION_THRESHOLD
COOLDOWN = _testinternalcapi.SPECIALIZATION_COOLDOWN


Plan = collections.namedtuple(
    "Plan", "kind fallback slot_kind cacheable member type_version offset value")
StorePlan = collections.namedtuple(
    "StorePlan", "outcome cacheable member offset has_shape_to")


def plan(o, name):
    return Plan(*_testinternalcapi.lookup_plan(o, name))


def store_plan(o, name, delete=False):
    return StorePlan(*_testinternalcapi.lookup_store_plan(o, name, delete))


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

    def test_delete_other_follows_remaining_order(self):
        class C:
            pass
        a, b = C(), C()
        a.x, a.y, a.z = 1, 2, 3
        b.x, b.z = 1, 3
        del a.y
        self.assertEqual(shape_of(a), shape_of(b))
        a.y = 2
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
        self.assertIn("STORE_ATTR_SHAPE_ADD", opnames(C.__init__))
        self.assertNotIn("LOAD_ATTR_INSTANCE_VALUE", opnames(get))
        self.assertNotIn("STORE_ATTR_INSTANCE_VALUE", opnames(put))

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
        # The site falls back to the inline values, which the dict shares.
        self.assertIn("LOAD_ATTR_INSTANCE_VALUE", opnames(get))
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
        self.assertIn("STORE_ATTR_SHAPE_ADD", opnames(C.__init__))
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
        # Two shapes of one type: the sites check presence instead.
        self.assertIn("LOAD_ATTR_INSTANCE_VALUE", opnames(get))
        self.assertIn("STORE_ATTR_INSTANCE_VALUE", opnames(mark))
        self.assertIn("STORE_ATTR_INSTANCE_VALUE", opnames(Token.__init__))
        # Adding through the instance value handler keeps the shape.
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
        # The dict aliases the inline values.
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
        # Dictionary mode, with the values still inline and no dict.
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
        self.assertEqual(store_plan(o, "x").outcome, "REPLACE")

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
        sp = store_plan(o, "x")
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
        # Fill the shared keys, so that a new name needs a dict.
        filler = C()
        for i in range(40):
            setattr(filler, f"f{i}", i)
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


if __name__ == "__main__":
    unittest.main()
