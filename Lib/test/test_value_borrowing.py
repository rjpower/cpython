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


class TypeSlotTests(unittest.TestCase):
    """Slot wrappers, slot functions and type attributes borrow their operands."""

    def test_slot_wrapper_operand_eq_drops_receiver(self):
        holder = [[EqHook(5, lambda: (holder.clear(), gc.collect()))]]
        self.assertTrue(list.__contains__(holder[0], 5))
        self.assertEqual(holder, [])

    def test_slot_wrapper_richcompare_drops_receiver(self):
        holder = []

        class Other:
            def __eq__(self, other):
                holder.clear()
                gc.collect()
                return True

        holder.append([1])
        self.assertTrue(list.__eq__(holder[0], [Other()]))
        self.assertEqual(holder, [])

    def test_call_slot_drops_receiver(self):
        holder = []

        class C:
            def __call__(self, *args, **kwargs):
                holder.clear()
                gc.collect()
                return (args, kwargs)

        holder.append(C())
        self.assertEqual(holder[0](1, k=2), ((1,), {"k": 2}))
        self.assertEqual(holder, [])

    def test_init_slot_drops_instance_holder(self):
        holder = []

        class C:
            def __init__(self, value):
                holder.clear()
                gc.collect()
                self.value = value

        holder.append(C)
        self.assertEqual(holder[0](3).value, 3)

    def test_len_and_getitem_slots_drop_receiver(self):
        holder = []

        class C:
            def __len__(self):
                holder.clear()
                gc.collect()
                return 2

            def __getitem__(self, i):
                holder.clear()
                gc.collect()
                return i

        holder.append(C())
        self.assertEqual(len(holder[0]), 2)
        holder.append(C())
        self.assertEqual(holder[0][7], 7)

    def test_type_setattr_old_value_del_drops_class(self):
        holder = []

        class Dropper:
            def __del__(self):
                holder.clear()
                gc.collect()

        def make():
            class C:
                attr = Dropper()
            return C

        holder.append(make())
        holder[0].attr = 1
        self.assertEqual(holder, [])

    def test_type_setattr_doc_and_module(self):
        def make():
            class C:
                pass
            return C

        holder = [make()]
        holder[0].__doc__ = "doc"
        holder[0].__module__ = "mod"
        holder[0].__type_params__ = ()
        self.assertEqual(holder[0].__doc__, "doc")
        self.assertEqual(holder[0].__module__, "mod")

    def test_type_getattr_metaclass_descriptor_reassigns_class(self):
        class OtherMeta(type):
            pass

        def make():
            class Desc:
                def __get__(self, obj, owner):
                    if obj is not None:
                        obj.__class__ = OtherMeta
                        gc.collect()
                    return "meta"

                def __set__(self, obj, value):
                    raise AttributeError

            class Meta(type):
                attr = Desc()

            return Meta("C", (), {})

        cls = make()
        self.assertEqual(cls.attr, "meta")
        self.assertIs(type(cls), OtherMeta)

    def test_set_name_mutates_namespace(self):
        events = []

        class Named:
            def __set_name__(self, owner, name):
                for key in ("a", "b"):
                    if hasattr(owner, key):
                        delattr(owner, key)
                gc.collect()
                events.append(name)

        class C:
            a = Named()
            b = Named()

        self.assertEqual(len(events), 2)

    def test_bases_assignment_with_mro_override(self):
        class A:
            pass

        class B:
            pass

        class Meta(type):
            def mro(cls):
                gc.collect()
                return type.mro(cls)

        class C(A, metaclass=Meta):
            pass

        C.__bases__ = (B,)
        self.assertEqual(C.__mro__, (C, B, object))

    def test_getstate_default_slots(self):
        import copy

        class S:
            __slots__ = ("x", "y")

        s = S()
        s.x = 1
        c = copy.copy(s)
        self.assertEqual(c.x, 1)
        self.assertFalse(hasattr(c, "y"))


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


class RemainingOperationTests(unittest.TestCase):

    def test_oserror_str_field_repr_reassigns_fields(self):
        # OSError.__str__ formats its fields with %R; a field's __repr__ can
        # reassign them, so the formatter retains them at the call site.
        err = OSError(2, "msg")

        class F:
            def __repr__(self):
                err.filename = None
                err.strerror = None
                gc.collect()
                return "f"

        err.filename = F()
        self.assertEqual(str(err), "[Errno 2] msg: f")
        self.assertIsNone(err.filename)

    def test_set_contains_and_discard_drop_set(self):
        for op in ("contains", "discard"):
            holder = [{EqHook(5)}]

            def drop():
                holder.clear()
                gc.collect()

            s = holder[0]
            next(iter(s)).hook = drop
            del s
            key = EqHook(5)
            if op == "contains":
                self.assertTrue(key in holder[0] if holder else True)
            else:
                target = holder[0]
                target.discard(key)
                self.assertEqual(target, set())
            self.assertEqual(holder, [])

    def test_issubclass_bases_getter_drops_child(self):
        class Node:
            def __init__(self, parent):
                self.parent = parent
                self.hook = None

            @property
            def __bases__(self):
                hook, self.hook = self.hook, None
                if hook is not None:
                    hook()
                return (self.parent,) if self.parent is not None else ()

        root = Node(None)
        holder = [Node(Node(root))]

        def drop():
            holder.clear()
            gc.collect()

        holder[0].parent.hook = drop
        self.assertTrue(issubclass(holder[0], root))
        self.assertEqual(holder, [])

    def test_repr_guard_element_drops_container(self):
        holder = []

        class X:
            def __repr__(self):
                holder.clear()
                gc.collect()
                return "x"

        holder.append([X()])
        holder[0].append(holder[0])
        self.assertEqual(repr(holder[0]), "[x, [...]]")
        self.assertEqual(holder, [])

    def test_percent_format_mapping_item_owned(self):
        class S:
            def __str__(self):
                gc.collect()
                return "s"

        class M:
            def __getitem__(self, key):
                return S()

        self.assertEqual("%(a)s-%(b)s" % M(), "s-s")
        self.assertEqual("%s" % (S(),), "s")
        self.assertEqual("%d" % 5, "5")

    def test_match_mapping_get_drops_subject(self):
        holder = []

        class M(dict):
            def get(self, key, default=None):
                holder.clear()
                gc.collect()
                return dict.get(self, key, default)

        holder.append(M(a=1, b=2))
        match holder[0]:
            case {"a": a, "b": b}:
                self.assertEqual((a, b), (1, 2))
            case _:
                self.fail("no match")
        self.assertEqual(holder, [])

    def test_match_class_pattern(self):
        class P:
            __match_args__ = ("x",)

            def __init__(self, x):
                self.x = x

        match P(3):
            case P(x):
                self.assertEqual(x, 3)
            case _:
                self.fail("no match")
        match 5:
            case int(v):
                self.assertEqual(v, 5)

    def test_enumerate_next_drops_iterator_reference(self):
        holder = []

        class It:
            def __iter__(self):
                return self

            def __next__(self):
                holder.clear()
                gc.collect()
                return 1

        holder.append(enumerate(It(), 10**20))
        e = holder[0]
        self.assertEqual(next(e), (10**20, 1))
        e = enumerate(It())
        self.assertEqual(next(e), (0, 1))

    def test_property_setter_reinitialises_property(self):
        calls = []

        def other(obj, value):
            calls.append(("other", value))

        def setter(obj, value):
            prop.__init__(None, other)
            gc.collect()
            calls.append(("setter", value))

        prop = property(None, setter)

        class C:
            p = prop

        C().p = 1
        C().p = 2
        self.assertEqual(calls, [("setter", 1), ("other", 2)])

    def test_long_range_iterator_setstate(self):
        it = iter(range(10**20, 10**20 + 5))
        it.__setstate__(-1)
        self.assertEqual(next(it), 10**20)
        it.__setstate__(10**30)
        self.assertEqual(list(it), [])

    def test_type_call_with_keywords(self):
        class C:
            def __init__(self, **kw):
                self.kw = kw

        self.assertEqual(C(a=1, b=2).kw, {"a": 1, "b": 2})
        self.assertEqual(dict(**{"a": 1}, b=2), {"a": 1, "b": 2})

    def test_generic_alias_unpacks_tuple(self):
        from typing import TypeVarTuple
        Ts = TypeVarTuple("Ts")
        alias = tuple[int, *Ts][*tuple[str, bytes]]
        self.assertEqual(alias.__args__, (int, str, bytes))
        self.assertEqual(list[*Ts][str, bytes].__args__, (str, bytes))

    def test_keyerror_inline_key(self):
        with self.assertRaises(KeyError) as cm:
            {}[5]
        self.assertEqual(cm.exception.args, (5,))

    def test_ast_node_keywords(self):
        import ast
        node = ast.Name(id="x", ctx=ast.Load())
        self.assertEqual(node.id, "x")
        self.assertIsInstance(node.ctx, ast.Load)

    def test_compile_merges_constants(self):
        code = compile("a = (1, 2.5, None, ..., 'x', (1, 2.5))", "<s>", "exec")
        self.assertIn((1, 2.5, None, ..., "x", (1, 2.5)), code.co_consts)


class ProvenContainerTests(unittest.TestCase):
    """Core callers that have proven the container use the internal dict
    entries; the public entries keep their checks."""

    def test_dict_iterators_of_each_kind(self):
        d = {1: "a", 2.5: "b", "c": 3}
        self.assertEqual(list(iter(d)), [1, 2.5, "c"])
        self.assertEqual(list(d.values()), ["a", "b", 3])
        self.assertEqual(list(d.items()), [(1, "a"), (2.5, "b"), ("c", 3)])
        self.assertEqual(list(reversed(d)), ["c", 2.5, 1])
        self.assertEqual(list(reversed(d.values())), [3, "b", "a"])
        self.assertEqual(list(reversed(d.items())),
                         [("c", 3), (2.5, "b"), (1, "a")])

    def test_store_global_into_frozendict_globals_raises(self):
        code = compile("global x\nx = 1", "<s>", "exec")
        with self.assertRaises(TypeError):
            exec(code, frozendict({"__builtins__": __builtins__}))

    def test_exact_dict_store_delete_contains(self):
        d = {}
        d[5] = 1
        d["k"] = 2
        self.assertIn(5, d)
        self.assertNotIn(6, d)
        del d[5]
        with self.assertRaises(KeyError):
            del d[5]
        with self.assertRaises(TypeError):
            d[[]] = 1
        with self.assertRaises(TypeError):
            [] in d
        self.assertEqual(d, {"k": 2})

    def test_mappingproxy_contains(self):
        proxy = type("T", (), {"a": 1}).__dict__
        self.assertIn("a", proxy)
        self.assertNotIn("b", proxy)

    def test_build_map(self):
        a, b = 1, 2.5
        self.assertEqual({a: b, "x": a, b: "y"}, {1: 2.5, "x": 1, 2.5: "y"})


if __name__ == "__main__":
    unittest.main()
