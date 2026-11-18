"""Value-form operations borrow their arguments for the whole call.

The caller keeps the receiver, key, name and argument vector alive; the
callee pins only what it reads from mutable storage and uses across code
that can run Python.  These tests run Python code (__hash__, __eq__,
__getattr__, descriptors) that drops every other reference to the objects
and containers involved, mid-operation.  A missing pin shows as a crash or
a debug-build refcount assertion.
"""

import gc
import sys
import unittest


class DropOnHash:
    """A key whose __hash__ runs a callback first."""

    def __init__(self, hook, value=7):
        self.hook = hook
        self.value = value

    def __hash__(self):
        hook, self.hook = self.hook, None
        if hook is not None:
            hook()
        return hash(self.value)

    def __eq__(self, other):
        return isinstance(other, DropOnHash) and other.value == self.value


class EqHook:
    """A stored key whose __eq__ runs a callback, then compares values."""

    def __init__(self, value, hook=None):
        self.value = value
        self.hook = hook

    def __hash__(self):
        return hash(self.value)

    def __eq__(self, other):
        hook, self.hook = self.hook, None
        if hook is not None:
            hook()
        return getattr(other, "value", other) == self.value


class Collide:
    """Keys that share one hash; the first stored one runs a hook on __eq__."""

    def __init__(self, value, hook=None):
        self.value = value
        self.hook = hook

    def __hash__(self):
        return 1

    def __eq__(self, other):
        hook, self.hook = self.hook, None
        if hook is not None:
            hook()
        return isinstance(other, Collide) and other.value == self.value


class DictLookupTests(unittest.TestCase):

    def test_get_key_drops_dict_during_hash(self):
        holder = [{DropOnHash(None): "v"}]

        def drop():
            holder.clear()
            gc.collect()

        d = holder[0]
        key = DropOnHash(drop)
        self.assertEqual(d.get(key), "v")
        self.assertEqual(holder, [])

    def test_get_stored_eq_clears_dict_and_drops_query(self):
        box = []

        def clear():
            d.clear()
            box.clear()
            gc.collect()

        d = {EqHook(5, clear): "five", "s": 1}
        box.append(EqHook(5))
        self.assertIsNone(d.get(box[0]))
        self.assertEqual(d, {})

    def test_subscript_stored_eq_replaces_entry(self):
        def replace():
            d.clear()
            d[EqHook(5)] = "new"
            gc.collect()

        d = {EqHook(5, replace): "old", 1: 2}
        query = EqHook(5)
        # The table changed under the lookup: the result is either value,
        # never a freed one.
        self.assertIn(d[query], ("old", "new"))

    def test_getattr_instance_dict_replaced_by_key_eq(self):
        class C:
            pass

        obj = C()

        def swap():
            obj.__dict__ = {"attr": "swapped"}
            gc.collect()

        stored = EqHook("attr", swap)
        obj.__dict__[stored] = "original"
        # The stored non-str key is consulted for the str query and replaces
        # the dict being read.
        self.assertIn(getattr(obj, "attr"), ("original", "swapped"))

    def test_getattr_name_owner_dropped_by_key_eq(self):
        class C:
            pass

        names = ["na" + "me"]
        obj = C()

        def drop():
            names.clear()
            gc.collect()

        obj.__dict__[EqHook("name", drop)] = "x"
        self.assertEqual(getattr(obj, names[0], "absent"), "x")


class DictMutationTests(unittest.TestCase):
    """Dictionary operations whose callbacks drop every other owner."""

    def holder_dict(self, *items):
        holder = [dict(items)]

        def drop():
            holder.clear()
            gc.collect()

        return holder, drop

    def test_setdefault_hash_drops_dict_and_default(self):
        holder, drop = self.holder_dict()
        default = [object()]
        d = holder[0]
        key = DropOnHash(lambda: (drop(), default.clear()))
        value = default[0]
        self.assertIs(d.setdefault(key, value), value)
        self.assertEqual(default, [])
        self.assertIs(d[DropOnHash(None)], value)

    def test_setdefault_stored_eq_clears_dict(self):
        def clear():
            d.clear()
            gc.collect()

        d = {Collide(0, clear): "a"}
        self.assertEqual(d.setdefault(Collide(1), "b"), "b")
        self.assertEqual(list(d.values()), ["b"])

    def test_pop_and_del_stored_eq_clears_dict(self):
        for op in ("pop", "del", "contains", "get"):
            with self.subTest(op=op):
                def clear():
                    d.clear()
                    gc.collect()

                first = Collide(0)
                d = {first: "a", Collide(1): "b"}
                first.hook = clear
                del first
                query = Collide(1)
                if op == "pop":
                    self.assertIn(d.pop(query, "gone"), ("b", "gone"))
                elif op == "del":
                    try:
                        del d[query]
                    except KeyError:
                        pass
                elif op == "contains":
                    self.assertIn(query in d, (True, False))
                else:
                    self.assertIn(d.get(query), ("b", None))
                self.assertNotIn(query, d)

    def test_fromkeys_value_owned_by_caller(self):
        value = [object()]
        d = dict.fromkeys({1: 0, 2: 0}, value.pop())
        self.assertIs(d[1], d[2])
        d = dict.fromkeys({1, 2}, object())
        self.assertIs(d[1], d[2])

    def test_update_key_eq_clears_target(self):
        def clear():
            target.clear()
            gc.collect()

        target = {Collide(0, clear): "a"}
        target.update({Collide(1): "b"})
        self.assertEqual(list(target.values()), ["b"])

    def test_items_contains_value_eq_drops_view(self):
        holder = [{1: EqHook(5, lambda: (holder.clear(), gc.collect()))}]
        items = holder[0].items()
        self.assertIn((1, 5), items)
        self.assertEqual(holder, [])

    def test_items_xor_value_eq_mutates(self):
        def clear():
            left.clear()
            gc.collect()

        left = {1: EqHook(5, clear), 2: 3}
        right = {1: 5, 2: 3}
        result = left.items() ^ right.items()
        self.assertIsInstance(result, set)

    def test_repr_drops_dict(self):
        holder = []

        class R:
            def __repr__(self):
                holder.clear()
                gc.collect()
                return "r"

        holder.append({1: R(), 2: R()})
        self.assertEqual(repr(holder[0]), "{1: r, 2: r}")

    def test_instance_store_old_value_del_drops_object(self):
        holder = []

        class Dropper:
            def __del__(self):
                holder.clear()
                gc.collect()

        class C:
            pass

        obj = C()
        obj.attr = Dropper()
        holder.append(obj)
        del obj
        holder[0].attr = 1
        self.assertEqual(holder, [])

    def test_instance_dict_old_value_del_drops_object(self):
        holder = []

        class Dropper:
            def __del__(self):
                holder.clear()
                gc.collect()

        class C:
            pass

        obj = C()
        obj.__dict__[EqHook("x")] = 0
        obj.attr = Dropper()
        holder.append(obj)
        del obj
        holder[0].attr = 1
        self.assertEqual(holder, [])


class CallSiteRetentionTests(unittest.TestCase):
    """Dictionaries a caller reads from mutable storage stay pinned."""

    def test_kwdefaults_replaced_during_lookup(self):
        def f(*, a=1):
            return a

        def replace():
            f.__kwdefaults__ = None
            gc.collect()

        # A non-str key with the hash of "a" makes the lookup call __eq__.
        stored = EqHook("a")
        f.__kwdefaults__ = {stored: 5}
        stored.hook = replace
        del stored
        self.assertEqual(f(), 5)
        self.assertIsNone(f.__kwdefaults__)

    def _finished_frame(self):
        return sys._getframe()

    def test_frame_extra_locals_cleared_during_lookup(self):
        frame = self._finished_frame()
        proxy = frame.f_locals

        def clear():
            frame.clear()
            gc.collect()

        first = Collide(0)
        proxy[first] = "a"
        proxy[Collide(1)] = "b"
        first.hook = clear
        del first
        self.assertEqual(proxy[Collide(1)], "b")

    def test_frame_extra_locals_cleared_during_contains(self):
        frame = self._finished_frame()
        proxy = frame.f_locals

        def clear():
            frame.clear()
            gc.collect()

        first = Collide(0)
        proxy[first] = "a"
        proxy[Collide(1)] = "b"
        first.hook = clear
        del first
        self.assertIn(Collide(1), proxy)


class DescriptorTests(unittest.TestCase):

    def test_class_reassigned_by_data_descriptor_get(self):
        class Other:
            pass

        def make():
            class Desc:
                def __get__(self, obj, owner):
                    obj.__class__ = Other
                    gc.collect()
                    return "got"

                def __set__(self, obj, value):
                    raise AttributeError

            class Temp:
                d = Desc()

            return Temp()

        obj = make()
        self.assertEqual(obj.d, "got")
        self.assertIs(type(obj), Other)

    def test_method_lookup_reassigns_class(self):
        class Other:
            def m(self):
                return "other"

        def make():
            class Desc:
                def __get__(self, obj, owner):
                    obj.__class__ = Other
                    gc.collect()
                    return lambda: "bound"

            class Temp:
                m = Desc()

            return Temp()

        obj = make()
        self.assertEqual(obj.m(), "bound")
        self.assertIs(type(obj), Other)

    def test_special_method_descriptor_drops_type(self):
        class Plain:
            def __exit__(self, *args):
                return False

        def make():
            class Enter:
                def __get__(self, obj, owner):
                    obj.__class__ = Plain
                    gc.collect()
                    return lambda: "entered"

            class CM:
                __enter__ = Enter()

                def __exit__(self, *args):
                    return False

            return CM()

        cm = make()
        with cm as value:
            self.assertEqual(value, "entered")
        self.assertIs(type(cm), Plain)

    def test_getattr_hook_drops_receiver(self):
        holder = []

        class C:
            def __getattr__(self, name):
                holder.clear()
                gc.collect()
                return name

        holder.append(C())
        self.assertEqual(getattr(holder[0], "missing"), "missing")
        self.assertEqual(holder, [])


class BinaryOpTests(unittest.TestCase):

    def test_sub_descriptor_drops_operands(self):
        holder = []

        class Sub:
            def __get__(self, obj, owner):
                holder.clear()
                gc.collect()
                return lambda other: ("sub", other)

        class A:
            __sub__ = Sub()

        holder.append(A())
        self.assertEqual(holder[0] - 3, ("sub", 3))
        self.assertEqual(holder, [])

    def test_rsub_drops_both_operands(self):
        holder = []

        class B:
            def __rsub__(self, other):
                holder.clear()
                gc.collect()
                return "rsub"

        class A:
            def __sub__(self, other):
                holder.pop()
                gc.collect()
                return NotImplemented

        holder.extend([A(), B()])
        self.assertEqual(holder[0] - holder[1], "rsub")
        self.assertEqual(holder, [])


class VectorcallMethodTests(unittest.TestCase):

    def test_write_method_drops_receiver(self):
        # print() calls file.write through PyObject_VectorcallMethod.
        holder = []
        out = []

        class W:
            def write(self, s):
                holder.clear()
                gc.collect()
                out.append(s)

        holder.append(W())
        print("x", file=holder[0], end="")
        self.assertEqual(out[0], "x")


if __name__ == "__main__":
    unittest.main()
