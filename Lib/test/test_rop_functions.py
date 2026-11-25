"""Function and Intrinsic (Python/register_ops/functions.rop)"""
import sys
import unittest
from test.support import import_helper

_testinternalcapi = import_helper.import_module("_testinternalcapi")


def words(func):
    return [w[0] for w in _testinternalcapi.get_ir(func.__code__)]


def make_all_fields():
    a, b = 1, 2

    def f(p=a, q: int = b, *, r=3, s: str = "x"):
        return a + b + p + q + r
    return f


class FunctionTests(unittest.TestCase):
    def test_words(self):
        w = words(make_all_fields)
        self.assertIn("Function.Make", w)
        self.assertIn("FunctionSet.SetField", w)
        self.assertNotIn("MAKE_FUNCTION", w)
        self.assertNotIn("SET_FUNCTION_ATTRIBUTE", w)

    def test_every_field(self):
        f = make_all_fields()
        self.assertEqual(f.__defaults__, (1, 2))
        self.assertEqual(f.__kwdefaults__, {"r": 3, "s": "x"})
        self.assertEqual(f.__annotations__, {"q": int, "s": str})
        self.assertEqual(f.__closure__[0].cell_contents, 1)
        self.assertEqual(f(), 1 + 2 + 1 + 2 + 3)

    def test_fields_from_locals(self):
        # An attribute that a local holds is counted, not taken
        d = (5,)
        def f(x=d): return x
        self.assertEqual(f.__defaults__, ((5,),))
        self.assertEqual(d, (5,))

    def test_stacked_defs(self):
        def outer(a, b=2):
            def one(x=a): return x
            def two(y=b, z=one): return y, z()
            return one, two
        one, two = outer(7)
        self.assertEqual(two(), (2, 7))


class IntrinsicTests(unittest.TestCase):
    def test_unary(self):
        ns = {}
        exec("def f(*a): return (*a, 1)\nclass C[T]: pass\ntype X = int\n"
             "def g[T, *U, **P](x: T): pass\nS = {*[1, 2]}", ns)
        w = words(ns["f"])
        self.assertIn("Intrinsic.Unary", w)
        self.assertEqual(ns["f"](2), (2, 1))
        self.assertEqual(len(ns["C"].__type_params__), 1)
        self.assertEqual(ns["X"].__value__, int)
        self.assertEqual(len(ns["g"].__type_params__), 3)

    def test_stopiteration(self):
        def gen():
            raise StopIteration
            yield
        w = words(gen)
        self.assertIn("Intrinsic.StopIteration", w)
        with self.assertRaisesRegex(RuntimeError, "generator raised StopIteration"):
            list(gen())

    def test_import_star(self):
        ns = {}
        exec("from os.path import *", ns)
        self.assertIn("join", ns)
        self.assertNotIn("_get_sep", ns)

    def test_import_star_runs_python(self):
        class Names(dict):
            def __setitem__(self, k, v):
                self.log.append(k)
                super().__setitem__(k, v)
        log = []
        Names.log = log
        exec("from os.path import *", {}, Names())
        self.assertIn("join", log)

    def test_binary(self):
        ns = {}
        exec("def f[T: int](x): pass\ndef g[T: (int, str)](x): pass\n"
             "def h[T = int](x): pass", ns)
        self.assertEqual(ns["f"].__type_params__[0].__bound__, int)
        self.assertEqual(ns["g"].__type_params__[0].__constraints__, (int, str))
        self.assertEqual(ns["h"].__type_params__[0].__default__, int)

    def test_except_star(self):
        try:
            try:
                raise ExceptionGroup("g", [ValueError(1), TypeError(2)])
            except* ValueError as e:
                self.assertEqual(len(e.exceptions), 1)
        except ExceptionGroup as eg:
            self.assertEqual(len(eg.exceptions), 1)
        else:
            self.fail("not reraised")

    def test_match_class(self):
        class P: pass
        match P():
            case P():
                pass
            case _:
                self.fail("no match")
        N = type("N", (), {"v": 5})
        with self.assertRaises(TypeError):
            match 1:
                case N.v():
                    pass

    def test_async_gen(self):
        async def ag():
            yield 1
        a = ag()
        try:
            a.asend(None).send(None)
        except StopIteration as e:
            self.assertEqual(e.value, 1)

    def test_error_closes_operand(self):
        class Bad:
            def __init__(self): self.alive = True
            def __del__(self): Bad.dead = True
        Bad.dead = False
        N = type("N", (), {"v": 5})
        with self.assertRaises(TypeError):
            match Bad():
                case N.v():
                    pass
        self.assertTrue(Bad.dead)


if __name__ == "__main__":
    unittest.main()
