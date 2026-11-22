# Tests for rich comparisons

import unittest
from test import support

import operator

class Number:

    def __init__(self, x):
        self.x = x

    def __lt__(self, other):
        return self.x < other

    def __le__(self, other):
        return self.x <= other

    def __eq__(self, other):
        return self.x == other

    def __ne__(self, other):
        return self.x != other

    def __gt__(self, other):
        return self.x > other

    def __ge__(self, other):
        return self.x >= other

    def __repr__(self):
        return "Number(%r)" % (self.x, )

class Vector:

    def __init__(self, data):
        self.data = data

    def __len__(self):
        return len(self.data)

    def __getitem__(self, i):
        return self.data[i]

    def __setitem__(self, i, v):
        self.data[i] = v

    __hash__ = None # Vectors cannot be hashed

    def __bool__(self):
        raise TypeError("Vectors cannot be used in Boolean contexts")

    def __repr__(self):
        return "Vector(%r)" % (self.data, )

    def __lt__(self, other):
        return Vector([a < b for a, b in zip(self.data, self.__cast(other))])

    def __le__(self, other):
        return Vector([a <= b for a, b in zip(self.data, self.__cast(other))])

    def __eq__(self, other):
        return Vector([a == b for a, b in zip(self.data, self.__cast(other))])

    def __ne__(self, other):
        return Vector([a != b for a, b in zip(self.data, self.__cast(other))])

    def __gt__(self, other):
        return Vector([a > b for a, b in zip(self.data, self.__cast(other))])

    def __ge__(self, other):
        return Vector([a >= b for a, b in zip(self.data, self.__cast(other))])

    def __cast(self, other):
        if isinstance(other, Vector):
            other = other.data
        if len(self.data) != len(other):
            raise ValueError("Cannot compare vectors of different length")
        return other

opmap = {
    "lt": (lambda a,b: a< b, operator.lt, operator.__lt__),
    "le": (lambda a,b: a<=b, operator.le, operator.__le__),
    "eq": (lambda a,b: a==b, operator.eq, operator.__eq__),
    "ne": (lambda a,b: a!=b, operator.ne, operator.__ne__),
    "gt": (lambda a,b: a> b, operator.gt, operator.__gt__),
    "ge": (lambda a,b: a>=b, operator.ge, operator.__ge__)
}

class VectorTest(unittest.TestCase):

    def checkfail(self, error, opname, *args):
        for op in opmap[opname]:
            self.assertRaises(error, op, *args)

    def checkequal(self, opname, a, b, expres):
        for op in opmap[opname]:
            realres = op(a, b)
            # can't use assertEqual(realres, expres) here
            self.assertEqual(len(realres), len(expres))
            for i in range(len(realres)):
                # results are bool, so we can use "is" here
                self.assertTrue(realres[i] is expres[i])

    def test_mixed(self):
        # check that comparisons involving Vector objects
        # which return rich results (i.e. Vectors with itemwise
        # comparison results) work
        a = Vector(range(2))
        b = Vector(range(3))
        # all comparisons should fail for different length
        for opname in opmap:
            self.checkfail(ValueError, opname, a, b)

        a = list(range(5))
        b = 5 * [2]
        # try mixed arguments (but not (a, b) as that won't return a bool vector)
        args = [(a, Vector(b)), (Vector(a), b), (Vector(a), Vector(b))]
        for (a, b) in args:
            self.checkequal("lt", a, b, [True,  True,  False, False, False])
            self.checkequal("le", a, b, [True,  True,  True,  False, False])
            self.checkequal("eq", a, b, [False, False, True,  False, False])
            self.checkequal("ne", a, b, [True,  True,  False, True,  True ])
            self.checkequal("gt", a, b, [False, False, False, True,  True ])
            self.checkequal("ge", a, b, [False, False, True,  True,  True ])

            for ops in opmap.values():
                for op in ops:
                    # calls __bool__, which should fail
                    self.assertRaises(TypeError, bool, op(a, b))

class NumberTest(unittest.TestCase):

    def test_basic(self):
        # Check that comparisons involving Number objects
        # give the same results give as comparing the
        # corresponding ints
        for a in range(3):
            for b in range(3):
                for typea in (int, Number):
                    for typeb in (int, Number):
                        if typea==typeb==int:
                            continue # the combination int, int is useless
                        ta = typea(a)
                        tb = typeb(b)
                        for ops in opmap.values():
                            for op in ops:
                                realoutcome = op(a, b)
                                testoutcome = op(ta, tb)
                                self.assertEqual(realoutcome, testoutcome)

    def checkvalue(self, opname, a, b, expres):
        for typea in (int, Number):
            for typeb in (int, Number):
                ta = typea(a)
                tb = typeb(b)
                for op in opmap[opname]:
                    realres = op(ta, tb)
                    realres = getattr(realres, "x", realres)
                    self.assertTrue(realres is expres)

    def test_values(self):
        # check all operators and all comparison results
        self.checkvalue("lt", 0, 0, False)
        self.checkvalue("le", 0, 0, True )
        self.checkvalue("eq", 0, 0, True )
        self.checkvalue("ne", 0, 0, False)
        self.checkvalue("gt", 0, 0, False)
        self.checkvalue("ge", 0, 0, True )

        self.checkvalue("lt", 0, 1, True )
        self.checkvalue("le", 0, 1, True )
        self.checkvalue("eq", 0, 1, False)
        self.checkvalue("ne", 0, 1, True )
        self.checkvalue("gt", 0, 1, False)
        self.checkvalue("ge", 0, 1, False)

        self.checkvalue("lt", 1, 0, False)
        self.checkvalue("le", 1, 0, False)
        self.checkvalue("eq", 1, 0, False)
        self.checkvalue("ne", 1, 0, True )
        self.checkvalue("gt", 1, 0, True )
        self.checkvalue("ge", 1, 0, True )

class MiscTest(unittest.TestCase):

    def test_misbehavin(self):
        class Misb:
            def __lt__(self_, other): return 0
            def __gt__(self_, other): return 0
            def __eq__(self_, other): return 0
            def __le__(self_, other): self.fail("This shouldn't happen")
            def __ge__(self_, other): self.fail("This shouldn't happen")
            def __ne__(self_, other): self.fail("This shouldn't happen")
        a = Misb()
        b = Misb()
        self.assertEqual(a<b, 0)
        self.assertEqual(a==b, 0)
        self.assertEqual(a>b, 0)

    def test_not(self):
        # Check that exceptions in __bool__ are properly
        # propagated by the not operator
        import operator
        class Exc(Exception):
            pass
        class Bad:
            def __bool__(self):
                raise Exc

        def do(bad):
            not bad

        for func in (do, operator.not_):
            self.assertRaises(Exc, func, Bad())

    @support.no_tracing
    @support.infinite_recursion(25)
    def test_recursion(self):
        # Check that comparison for recursive objects fails gracefully
        from collections import UserList
        a = UserList()
        b = UserList()
        a.append(b)
        b.append(a)
        self.assertRaises(RecursionError, operator.eq, a, b)
        self.assertRaises(RecursionError, operator.ne, a, b)
        self.assertRaises(RecursionError, operator.lt, a, b)
        self.assertRaises(RecursionError, operator.le, a, b)
        self.assertRaises(RecursionError, operator.gt, a, b)
        self.assertRaises(RecursionError, operator.ge, a, b)

        b.append(17)
        # Even recursive lists of different lengths are different,
        # but they cannot be ordered
        self.assertTrue(not (a == b))
        self.assertTrue(a != b)
        self.assertRaises(RecursionError, operator.lt, a, b)
        self.assertRaises(RecursionError, operator.le, a, b)
        self.assertRaises(RecursionError, operator.gt, a, b)
        self.assertRaises(RecursionError, operator.ge, a, b)
        a.append(17)
        self.assertRaises(RecursionError, operator.eq, a, b)
        self.assertRaises(RecursionError, operator.ne, a, b)
        a.insert(0, 11)
        b.insert(0, 12)
        self.assertTrue(not (a == b))
        self.assertTrue(a != b)
        self.assertTrue(a < b)

    def test_exception_message(self):
        class Spam:
            pass

        tests = [
            (lambda: 42 < None, r"'<' .* of 'int' and 'NoneType'"),
            (lambda: None < 42, r"'<' .* of 'NoneType' and 'int'"),
            (lambda: 42 > None, r"'>' .* of 'int' and 'NoneType'"),
            (lambda: "foo" < None, r"'<' .* of 'str' and 'NoneType'"),
            (lambda: "foo" >= 666, r"'>=' .* of 'str' and 'int'"),
            (lambda: 42 <= None, r"'<=' .* of 'int' and 'NoneType'"),
            (lambda: 42 >= None, r"'>=' .* of 'int' and 'NoneType'"),
            (lambda: 42 < [], r"'<' .* of 'int' and 'list'"),
            (lambda: () > [], r"'>' .* of 'tuple' and 'list'"),
            (lambda: None >= None, r"'>=' .* of 'NoneType' and 'NoneType'"),
            (lambda: Spam() < 42, r"'<' .* of 'Spam' and 'int'"),
            (lambda: 42 < Spam(), r"'<' .* of 'int' and 'Spam'"),
            (lambda: Spam() <= Spam(), r"'<=' .* of 'Spam' and 'Spam'"),
        ]
        for i, test in enumerate(tests):
            with self.subTest(test=i):
                with self.assertRaisesRegex(TypeError, test[1]):
                    test[0]()


class IntSub(int):
    pass

class IntSubEq(int):
    def __eq__(self, other):
        return True
    __hash__ = int.__hash__

class IntSubHash(int):
    def __hash__(self):
        return 42

class IntSubEqOnly(int):
    def __eq__(self, other):
        return int.__eq__(self, other)

class Plain:
    pass

class Reflected(Plain):
    def __eq__(self, other):
        return "reflected"
    def __ne__(self, other):
        return "reflected ne"
    __hash__ = object.__hash__

class Declines:
    def __eq__(self, other):
        return NotImplemented

class BytesSub(bytes):
    pass

class BytesSubEq(bytes):
    def __eq__(self, other):
        return True
    __hash__ = bytes.__hash__


class SlotExitTest(unittest.TestCase):
    # Operands that share a comparison slot are decided without the
    # protocol; an operand with another slot, or an override, is not.

    def test_int_subclass(self):
        self.assertTrue(IntSub(5) == IntSub(5))
        self.assertTrue(IntSub(5) != IntSub(6))
        self.assertTrue(IntSub(5) == 5)
        self.assertTrue(5 == IntSub(5))
        self.assertTrue(IntSub(2**70) == 2**70)
        self.assertTrue(IntSub(2**70) < 2**71)
        self.assertTrue(IntSub(-3) < IntSub(2))
        self.assertTrue(IntSub(1) == True)
        self.assertTrue(IntSub(5) == 5.0)

    def test_int_subclass_overrides(self):
        self.assertTrue(IntSubEq(1) == IntSubEq(2))
        self.assertTrue(IntSubEq(1) == 3)
        self.assertTrue(3 == IntSubEq(1))
        self.assertTrue(IntSubEqOnly(1) == IntSubEqOnly(1))
        self.assertFalse(IntSubEqOnly(1) == IntSubEqOnly(2))
        self.assertRaises(TypeError, hash, IntSubEqOnly(1))
        self.assertTrue(IntSubEq(7) in {IntSubEq(7)})
        self.assertTrue(IntSubEq(7) in {7})

    def test_int_subclass_hash(self):
        for n in (0, 1, -1, 5, -2**30, 2**30, 2**70):
            self.assertEqual(hash(IntSub(n)), hash(n))
        self.assertEqual(hash(IntSubHash(3)), 42)
        self.assertIn(IntSubHash(3), {IntSubHash(3)})
        self.assertNotIn(3, {IntSubHash(3)})
        members = {IntSub(1), IntSub(2**70)}
        self.assertIn(IntSub(1), members)
        self.assertIn(1, members)
        self.assertIn(2**70, members)
        self.assertNotIn(IntSub(3), members)
        keys = {IntSub(1): "a", IntSub(2**70): "b"}
        self.assertIn(1, keys)
        self.assertIn(IntSub(2**70), keys)
        self.assertNotIn(IntSub(3), keys)
        self.assertIn(IntSub(3), (IntSub(1), IntSub(2), 3))

    def test_nan_identity(self):
        nan = float("nan")
        self.assertFalse(nan == nan)
        self.assertTrue(nan != nan)
        self.assertIn(nan, [nan])
        self.assertIn(nan, {nan})
        self.assertNotIn(float("nan"), [float("nan")])

    def test_default_equality(self):
        a, b = Plain(), Plain()
        self.assertTrue(a == a)
        self.assertFalse(a == b)
        self.assertFalse(a != a)
        self.assertTrue(a != b)
        self.assertFalse(a == None)
        self.assertTrue(a != None)
        self.assertFalse(1 == a)
        self.assertTrue(None != a)
        for op in opmap["lt"] + opmap["le"] + opmap["gt"] + opmap["ge"]:
            self.assertRaises(TypeError, op, a, b)

    def test_default_equality_then_override(self):
        class Late:
            pass
        a, b = Late(), Late()
        self.assertFalse(a == b)
        Late.__eq__ = lambda self, other: True
        self.assertTrue(a == b)
        del Late.__eq__
        self.assertFalse(a == b)

    def test_default_equality_class_assignment(self):
        class A: pass
        class B:
            def __eq__(self, other):
                return True
        a = A()
        a.__class__ = B
        self.assertTrue(a == B())

    def test_reflected_and_declined(self):
        a = Plain()
        self.assertEqual(a == Reflected(), "reflected")
        self.assertEqual(a != Reflected(), "reflected ne")
        self.assertEqual(Reflected() == a, "reflected")
        d = Declines()
        self.assertTrue(d == d)
        self.assertFalse(d == Declines())
        self.assertFalse(1 == d)
        self.assertFalse(IntSub(1) == d)
        self.assertFalse(d == IntSub(1))
        self.assertTrue(d != Declines())

    def test_enum(self):
        import enum
        class Kind(enum.Enum):
            A = 1
            B = 2
        class Other(enum.Enum):
            A = 1
        class Loose(enum.Enum):
            A = 1
            B = 2
            def __eq__(self, other):
                return True
            __hash__ = enum.Enum.__hash__
        class Number(enum.IntEnum):
            X = 1
            Y = 2
        self.assertTrue(Kind.A == Kind.A)
        self.assertFalse(Kind.A == Kind.B)
        self.assertTrue(Kind.A != Kind.B)
        self.assertFalse(Kind.A != Kind.A)
        self.assertFalse(Kind.A == Other.A)
        self.assertFalse(Kind.A == 1)
        self.assertTrue(Loose.A == Loose.B)
        self.assertFalse(Loose.A != Loose.B)
        self.assertTrue(Number.X == 1)
        self.assertTrue(1 == Number.X)
        self.assertTrue(Number.X < Number.Y)
        self.assertIn(Number.X, {1})
        self.assertIn(1, {Number.X})
        self.assertIn(Kind.A, {Kind.A: 1})
        self.assertNotIn(Kind.B, {Kind.A})

    def test_bytes(self):
        a = bytes(bytearray(b"hello"))
        b = bytes(bytearray(b"hello"))
        self.assertTrue(a == b)
        self.assertFalse(a != b)
        self.assertFalse(a == b"hellp")
        self.assertFalse(a == b"hell")
        self.assertTrue(a < b"hellp")
        self.assertTrue(a >= b)
        self.assertTrue(a == bytearray(b"hello"))
        self.assertTrue(bytearray(b"hello") == a)
        self.assertTrue(a == memoryview(b"hello"))
        self.assertTrue(memoryview(b"hello") == a)
        self.assertFalse(a == "hello")
        self.assertFalse(a == 5)
        self.assertTrue(BytesSub(b"x") == b"x")
        self.assertTrue(b"x" == BytesSub(b"x"))
        self.assertFalse(BytesSub(b"x") == BytesSub(b"y"))
        self.assertTrue(BytesSubEq(b"x") == b"y")
        self.assertTrue(b"y" == BytesSubEq(b"x"))
        self.assertIn(b, {a: 1})
        self.assertIn(BytesSub(b"hello"), {a: 1})
        self.assertIn(b, (b"a", a))

    def test_str(self):
        class StrSub(str):
            pass
        self.assertTrue("ab" == "ab")
        self.assertFalse("ab" != "ab")
        self.assertTrue("ab" < "b")
        self.assertFalse("ab" == b"ab")
        self.assertTrue(StrSub("a") == "a")
        self.assertTrue("a" == StrSub("a"))
        self.assertIn(StrSub("a"), {"a"})

    def test_mutating_eq(self):
        class Key:
            def __init__(self, v, container=None):
                self.v = v
                self.container = container
            def __hash__(self):
                return self.v
            def __eq__(self, other):
                if self.container is not None:
                    self.container.add(Key(99))
                    self.container.discard(self)
                return isinstance(other, Key) and other.v == self.v
        members = set()
        members.add(Key(1, members))
        self.assertNotIn(Key(1), members)


class DictTest(unittest.TestCase):

    def test_dicts(self):
        # Verify that __eq__ and __ne__ work for dicts even if the keys and
        # values don't support anything other than __eq__ and __ne__ (and
        # __hash__).  Complex numbers are a fine example of that.
        import random
        imag1a = {}
        for i in range(50):
            imag1a[random.randrange(100)*1j] = random.randrange(100)*1j
        items = list(imag1a.items())
        random.shuffle(items)
        imag1b = {}
        for k, v in items:
            imag1b[k] = v
        imag2 = imag1b.copy()
        imag2[k] = v + 1.0
        self.assertEqual(imag1a, imag1a)
        self.assertEqual(imag1a, imag1b)
        self.assertEqual(imag2, imag2)
        self.assertTrue(imag1a != imag2)
        for opname in ("lt", "le", "gt", "ge"):
            for op in opmap[opname]:
                self.assertRaises(TypeError, op, imag1a, imag2)

class ListTest(unittest.TestCase):

    def test_coverage(self):
        # exercise all comparisons for lists
        x = [42]
        self.assertIs(x<x, False)
        self.assertIs(x<=x, True)
        self.assertIs(x==x, True)
        self.assertIs(x!=x, False)
        self.assertIs(x>x, False)
        self.assertIs(x>=x, True)
        y = [42, 42]
        self.assertIs(x<y, True)
        self.assertIs(x<=y, True)
        self.assertIs(x==y, False)
        self.assertIs(x!=y, True)
        self.assertIs(x>y, False)
        self.assertIs(x>=y, False)

    def test_badentry(self):
        # make sure that exceptions for item comparison are properly
        # propagated in list comparisons
        class Exc(Exception):
            pass
        class Bad:
            def __eq__(self, other):
                raise Exc

        x = [Bad()]
        y = [Bad()]

        for op in opmap["eq"]:
            self.assertRaises(Exc, op, x, y)

    def test_goodentry(self):
        # This test exercises the final call to PyObject_RichCompare()
        # in Objects/listobject.c::list_richcompare()
        class Good:
            def __lt__(self, other):
                return True

        x = [Good()]
        y = [Good()]

        for op in opmap["lt"]:
            self.assertIs(op(x, y), True)


if __name__ == "__main__":
    unittest.main()
