"""Shapes: the hidden classes in the object header.

See boil/design/09-object-protocol.md and 04-shapes.md.
"""

import dis
import itertools
import unittest
from test.support import cpython_only, requires_specialization, reset_code
from test.support.import_helper import import_module

_testinternalcapi = import_module("_testinternalcapi")
shape_of = _testinternalcapi.get_object_shape
shape_count = _testinternalcapi.get_type_shape_count
MAX_SHAPES = _testinternalcapi._Py_SHAPE_MAX_PER_TYPE
THRESHOLD = _testinternalcapi.SPECIALIZATION_THRESHOLD
COOLDOWN = _testinternalcapi.SPECIALIZATION_COOLDOWN


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

if __name__ == "__main__":
    unittest.main()
