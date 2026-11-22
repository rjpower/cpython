"""A method load that saw several receiver types reads the site's poly rows."""

import types
import unittest

from test import support
from test.support import import_helper

_testinternalcapi = import_helper.import_module("_testinternalcapi")
POLY = "LOAD_ATTR_METHOD_POLY"
ROUNDS = _testinternalcapi.SPECIALIZATION_COOLDOWN * 4


def handlers(function):
    return {word[0] for word in _testinternalcapi.get_ir(function.__code__, True)}


def template(owner):
    return owner.name()


def make_classes():
    class Base:
        def name(self):
            return "base"

    class Own(Base):
        def name(self):
            return "own"

    class Inherited(Base):
        pass

    class Slotted:
        __slots__ = ()

        def name(self):
            return "slotted"

    class Deep(Inherited):
        def name(self):
            return "deep:" + super().name()

    return Base, Own, Inherited, Slotted, Deep


EXPECTED = {"Base": "base", "Own": "own", "Inherited": "base",
            "Slotted": "slotted", "Deep": "deep:base"}


@unittest.skipIf(support.Py_GIL_DISABLED, "requires GIL specialization")
class MethodPolyTests(unittest.TestCase):
    def owners(self, count):
        return [cls() for cls in make_classes()[:count]]

    def train(self, function, owners, rounds=ROUNDS):
        for _ in range(rounds):
            for owner in owners:
                self.assertEqual(function(owner), EXPECTED[type(owner).__name__])

    def fresh(self):
        # A code object of its own: sites keep their feedback per code object
        return types.FunctionType(template.__code__.replace(), globals())

    def test_four_types_use_the_poly_handler(self):
        read = self.fresh()
        owners = self.owners(4)
        self.train(read, owners)
        self.assertIn(POLY, handlers(read))
        self.train(read, owners, 8)
        self.assertIn(POLY, handlers(read))

    def test_fifth_type_and_inline_numbers_are_correct(self):
        read = self.fresh()
        owners = self.owners(5)
        self.train(read, owners[:4])
        self.assertIn(POLY, handlers(read))
        self.train(read, owners, ROUNDS)
        self.assertEqual(read(owners[0]), "base")

        class Number(int):
            def name(self):
                return "number"

        read = self.fresh()
        self.train(read, self.owners(3))
        for _ in range(ROUNDS):
            self.assertEqual(read(Number(3)), "number")
            with self.assertRaises(AttributeError):
                read(3)
            self.assertEqual(read(self.owners(1)[0]), "base")

    def test_class_mutation_after_specialization(self):
        Base, Own, Inherited, Slotted, Deep = make_classes()
        read = self.fresh()
        owners = [Base(), Own(), Inherited(), Slotted()]
        self.train(read, owners)
        self.assertIn(POLY, handlers(read))
        Own.name = lambda self: "changed"
        for _ in range(ROUNDS):
            self.assertEqual([read(owner) for owner in owners],
                             ["base", "changed", "base", "slotted"])
        Base.name = lambda self: "rebased"
        for _ in range(ROUNDS):
            self.assertEqual([read(owner) for owner in owners],
                             ["rebased", "changed", "rebased", "slotted"])
        del Own.name
        for _ in range(ROUNDS):
            self.assertEqual([read(owner) for owner in owners],
                             ["rebased", "rebased", "rebased", "slotted"])

    def test_instance_attribute_shadows_the_method(self):
        Base, Own, Inherited, Slotted, Deep = make_classes()
        read = self.fresh()
        owners = [Base(), Own(), Inherited(), Slotted()]
        self.train(read, owners)
        self.assertIn(POLY, handlers(read))
        owners[2].name = lambda: "instance"
        for _ in range(ROUNDS):
            self.assertEqual([read(owner) for owner in owners],
                             ["base", "own", "instance", "slotted"])
        del owners[2].name
        for _ in range(ROUNDS):
            self.assertEqual(read(owners[2]), "base")

    def test_getattribute_and_property_added_to_a_class(self):
        Base, Own, Inherited, Slotted, Deep = make_classes()
        read = self.fresh()
        owners = [Base(), Own(), Inherited(), Slotted()]
        self.train(read, owners)
        Inherited.__getattribute__ = lambda self, name: lambda: "hooked " + name
        for _ in range(ROUNDS):
            self.assertEqual(read(owners[2]), "hooked name")
            self.assertEqual(read(owners[1]), "own")
        del Inherited.__getattribute__
        Own.name = property(lambda self: lambda: "property")
        for _ in range(ROUNDS):
            self.assertEqual(read(owners[1]), "property")
            self.assertEqual(read(owners[0]), "base")

    def test_method_sites_with_arguments_and_bound_use(self):
        def template(owner, n):
            total = 0
            for _ in range(n):
                total += owner.add(1)
            return total

        run = types.FunctionType(template.__code__.replace(), globals())

        class A:
            def add(self, x):
                return x

        class B:
            def add(self, x):
                return x + 1

        class C(B):
            pass

        owners = [A(), B(), C()]
        for _ in range(ROUNDS):
            self.assertEqual([run(owner, 3) for owner in owners], [3, 6, 6])
        self.assertIn(POLY, handlers(run))


if __name__ == "__main__":
    unittest.main()
