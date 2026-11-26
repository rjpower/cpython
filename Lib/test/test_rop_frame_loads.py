"""GetAttr.Property, GetAttr.GetattributeOverridden and GetItem.Getitem run
the Python function of a property, of __getattribute__ and of __getitem__ in a
frame of the loading activation, as does the generic arm of each family when
a site has not adapted."""

import dis
import sys
import unittest
from test.support import cpython_only

_testinternalcapi = __import__("_testinternalcapi")


def word_names(code):
    return [row[0].split("+")[0] for row in _testinternalcapi.get_ir(code, True)]


def force(function, opname, arm):
    offsets = [i.offset for i in dis.get_instructions(function)
               if i.opname == opname]
    _testinternalcapi.force_arm(function.__code__, offsets[0], arm)


def warm(function, *args):
    for _ in range(100):
        function(*args)


class Tracker:
    deaths = []

    def __init__(self, name):
        self.name = name

    def __del__(self):
        Tracker.deaths.append(self.name)


class WithProperty:
    def __init__(self, value=5):
        self._value = value

    @property
    def value(self):
        return self._value

    @property
    def fails(self):
        raise ValueError("getter")

    @property
    def recurses(self):
        return self.recurses


@cpython_only
class PropertyTests(unittest.TestCase):
    def test_adapts_and_reads(self):
        def f(o):
            return o.value
        warm(f, WithProperty())
        self.assertIn("GetAttr.Property", word_names(f.__code__))
        self.assertEqual(f(WithProperty(7)), 7)

    def test_a_temporary_owner_is_released(self):
        class Owned(WithProperty):
            def __del__(self):
                Tracker.deaths.append("owner")

        def f():
            return Owned().value
        warm(f)
        Tracker.deaths.clear()
        self.assertEqual(f(), 5)
        self.assertEqual(Tracker.deaths, ["owner"])

    def test_a_local_owner_stays_alive(self):
        def f(o):
            x = o.value
            return o, x
        o = WithProperty(3)
        warm(f, o)
        self.assertIs(f(o)[0], o)
        self.assertEqual(f(o)[1], 3)

    def test_the_getter_raises_to_the_loader(self):
        def f(o):
            return o.fails
        for _ in range(100):
            try:
                f(WithProperty())
            except ValueError as error:
                names = []
                tb = error.__traceback__
                while tb:
                    names.append(tb.tb_frame.f_code.co_name)
                    tb = tb.tb_next
                self.assertEqual(names[-2:], ["f", "fails"])
            else:
                self.fail("no exception")

    def test_the_property_is_replaced(self):
        class C:
            @property
            def x(self):
                return 1

        def f(o):
            return o.x
        c = C()
        warm(f, c)
        self.assertIn("GetAttr.Property", word_names(f.__code__))
        C.x = property(lambda self: 2)
        self.assertEqual(f(c), 2)
        C.x = 3
        self.assertEqual(f(c), 3)

    def test_the_getter_is_redefined(self):
        class C:
            @property
            def x(self):
                return 1

        def other(self):
            return 4

        def f(o):
            return o.x
        c = C()
        warm(f, c)
        C.x.fget.__code__ = other.__code__
        self.assertEqual(f(c), 4)

    def test_recursion_raises(self):
        def f(o):
            return o.recurses
        with self.assertRaises(RecursionError):
            f(WithProperty())

    def test_the_generic_arm_enters_the_getter(self):
        def f(o):
            return o.value
        force(f, "LOAD_ATTR", "GetAttr.Generic")
        self.assertEqual(f(WithProperty(9)), 9)
        with self.assertRaises(ValueError):
            force_fails = lambda o: o.fails
            force(force_fails, "LOAD_ATTR", "GetAttr.Generic")
            force_fails(WithProperty())

    def test_a_property_with_a_getattr_hook_is_not_entered(self):
        class Hooked(WithProperty):
            def __getattr__(self, name):
                return "hook"

        def f(o):
            return o.value
        warm(f, Hooked())
        self.assertNotIn("GetAttr.Property", word_names(f.__code__))
        self.assertEqual(f(Hooked(2)), 2)


class Overridden:
    def __getattribute__(self, name):
        return (type(self).__name__, name)


class OverriddenFails:
    def __getattribute__(self, name):
        raise KeyError(name)


@cpython_only
class GetattributeTests(unittest.TestCase):
    def test_adapts_and_reads(self):
        def f(o):
            return o.anything
        warm(f, Overridden())
        self.assertIn("GetAttr.GetattributeOverridden", word_names(f.__code__))
        self.assertEqual(f(Overridden()), ("Overridden", "anything"))

    def test_the_hook_raises(self):
        def f(o):
            return o.k
        for _ in range(100):
            with self.assertRaises(KeyError):
                f(OverriddenFails())

    def test_the_owner_is_released(self):
        class Owned(Overridden):
            def __del__(self):
                Tracker.deaths.append("owner")

        def f():
            return Owned().name
        warm(f)
        Tracker.deaths.clear()
        self.assertEqual(f(), ("Owned", "name"))
        self.assertEqual(Tracker.deaths, ["owner"])

    def test_the_generic_arm_enters_the_hook(self):
        def f(o):
            return o.n
        force(f, "LOAD_ATTR", "GetAttr.Generic")
        self.assertEqual(f(Overridden()), ("Overridden", "n"))

    def test_the_hook_is_replaced(self):
        class C:
            def __getattribute__(self, name):
                return 1

        def f(o):
            return o.x
        c = C()
        warm(f, c)
        C.__getattribute__ = lambda self, name: 2
        self.assertEqual(f(c), 2)
        del C.__getattribute__
        with self.assertRaises(AttributeError):
            f(c)


class WithGetitem:
    def __getitem__(self, key):
        return (self, key)


class GetitemFails:
    def __getitem__(self, key):
        raise IndexError(key)


@cpython_only
class GetitemTests(unittest.TestCase):
    def test_adapts_and_reads(self):
        def f(o, k):
            return o[k]
        c = WithGetitem()
        warm(f, c, 1)
        self.assertIn("GetItem.Getitem", word_names(f.__code__))
        self.assertEqual(f(c, 4), (c, 4))

    def test_the_getter_raises(self):
        def f(o, k):
            return o[k]
        for _ in range(100):
            with self.assertRaises(IndexError):
                f(GetitemFails(), 1)

    def test_the_operands_are_released(self):
        class Owned(WithGetitem):
            def __del__(self):
                Tracker.deaths.append("owner")

        def f():
            return Owned()[Tracker("key")]
        warm(f)
        Tracker.deaths.clear()
        result = f()
        self.assertEqual(Tracker.deaths, [])
        del result
        self.assertCountEqual(Tracker.deaths, ["owner", "key"])

    def test_local_operands_stay_alive(self):
        def f(o, k):
            r = o[k]
            return o, k, r
        c = WithGetitem()
        warm(f, c, "k")
        self.assertEqual(f(c, "k"), (c, "k", (c, "k")))

    def test_the_generic_arm_enters_getitem(self):
        def f(o, k):
            return o[k]
        force(f, "BINARY_OP", "GetItem.Generic")
        c = WithGetitem()
        self.assertEqual(f(c, 2), (c, 2))

    def test_getitem_is_replaced(self):
        class C:
            def __getitem__(self, k):
                return 1

        def f(o):
            return o[0]
        c = C()
        warm(f, c)
        C.__getitem__ = lambda self, k: 2
        self.assertEqual(f(c), 2)

    def test_recursion_raises(self):
        class Deep:
            def __getitem__(self, k):
                return self[k]

        def f(o):
            return o[0]
        with self.assertRaises(RecursionError):
            f(Deep())


if __name__ == "__main__":
    unittest.main()
