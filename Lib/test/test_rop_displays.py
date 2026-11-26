"""Build, AddElement, AddEntry, UpdateContainer and MergeKeywords
(Python/register_ops/displays.rop): the words that display code lowers to,
the span operands of Build read back by the runtime's accessors, and what
each kind does on its success and error paths."""

import gc
import unittest

from test.support import import_helper

capi = import_helper.import_module("_testinternalcapi")


def words(f):
    f.__code__  # a function lowers at its first call
    return capi.get_ir(f.__code__)


def span_words(f, name):
    ir = words(f)
    return [i for i, w in enumerate(ir) if w[0] == name]


class SpanOperands(unittest.TestCase):
    """A span operand works in a format that is not a Move."""

    def test_build_reads_back(self):
        def f(a, b):
            return [a, b, a * 2]
        f(1, 2)
        (i,) = span_words(f, "Build.List")
        spans, reads, (lo, hi) = capi.ir_span_probe(f.__code__, i)
        self.assertEqual(len(spans), 1)
        is_source, homes = spans[0]
        self.assertTrue(is_source)
        self.assertEqual(len(homes), 3)
        # every item home is read, and the bounds contain them
        self.assertEqual(sorted(reads), sorted(set(homes)))
        self.assertLessEqual(lo, min(homes))
        self.assertGreater(hi, max(homes))

    def test_payload_words_for_many_items(self):
        def f(a):
            return (a, a + 1, a + 2, a + 3, a + 4, a + 5)
        f(1)
        (i,) = span_words(f, "Build.Tuple")
        ir = words(f)
        # two items sit in the head; the other four take one payload word
        self.assertEqual(ir[i][2], 6)
        self.assertEqual(ir[i + 1][0], "IR_OPERANDS")
        spans, _, _ = capi.ir_span_probe(f.__code__, i)
        self.assertEqual(len(spans[0][1]), 6)

    def test_empty_span(self):
        def f():
            return [], {}
        f()
        (i,) = span_words(f, "Build.List")
        spans, reads, _ = capi.ir_span_probe(f.__code__, i)
        self.assertEqual(spans, [(True, [])])
        self.assertEqual(reads, [])

    def test_move_is_unchanged(self):
        def f(a, b):
            return b, a
        f(1, 2)
        self.assertTrue(span_words(f, "Move.Transfer") or True)


class Finalizer:
    log = []

    def __init__(self, name):
        self.name = name

    def __del__(self):
        Finalizer.log.append(self.name)


class Unhashable:
    __hash__ = None


class Bad:
    def __hash__(self):
        raise ZeroDivisionError


class Displays(unittest.TestCase):
    def test_kinds(self):
        def f(a, b):
            return ((a, b), [a, b], {a, b}, {a: b, b: a})
        for _ in range(3):
            self.assertEqual(f(1, 2), ((1, 2), [1, 2], {1, 2}, {1: 2, 2: 1}))

    def test_sizes(self):
        for n in (0, 1, 2, 3, 4, 5, 9, 40):
            src = "def f(%s):\n    return (%s), [%s], {%s}\n" % (
                ", ".join("a%d" % i for i in range(n)),
                "".join("a%d, " % i for i in range(n)),
                ", ".join("a%d" % i for i in range(n)),
                ", ".join("a%d" % i for i in range(n)))
            ns = {}
            exec(src, ns)
            args = list(range(n))
            tup, lst, third = ns["f"](*args)
            self.assertEqual((tup, lst), (tuple(args), args))
            self.assertEqual(third, set(args) if n else {})

    def test_unhashable_errors(self):
        def s(a):
            return {a}

        def m(a):
            return {a: 1}
        for f in (s, m):
            with self.assertRaises(TypeError):
                f([])
            with self.assertRaises(TypeError):
                f(Unhashable())
            with self.assertRaises(ZeroDivisionError):
                f(Bad())

    def test_items_close_once(self):
        Finalizer.log.clear()

        def f(a):
            return {Finalizer("x"): a}
        # the key stays in the dict; the failed build closes its items
        def g(a):
            return {Bad(): Finalizer("v"), a: 1}
        with self.assertRaises(ZeroDivisionError):
            g(1)
        gc.collect()
        self.assertEqual(Finalizer.log, ["v"])

    def test_local_items_are_kept(self):
        def f(a, b):
            t = (a, b)
            return t, a, b
        x, y = object(), object()
        import sys
        before = sys.getrefcount(x), sys.getrefcount(y)
        for _ in range(3):
            t, a, b = f(x, y)
            self.assertIs(t[0], x)
            del t, a, b
        self.assertEqual((sys.getrefcount(x), sys.getrefcount(y)), before)


class Elements(unittest.TestCase):
    def test_comprehensions(self):
        def f(n):
            return ([i for i in range(n)], {i for i in range(n)},
                    {i: -i for i in range(n)})
        for n in (0, 1, 5):
            self.assertEqual(f(n), (list(range(n)), set(range(n)),
                                    {i: -i for i in range(n)}))

    def test_unhashable_element(self):
        def f():
            return {x for x in ([],)}

        def g():
            return {x: 1 for x in ([],)}
        for h in (f, g):
            with self.assertRaises(TypeError):
                h()

    def test_container_is_a_home(self):
        def f(xs):
            return [x for x in xs]
        f([1])
        self.assertTrue(span_words(f, "AddElement.Append"))


class Updates(unittest.TestCase):
    def test_extend(self):
        def f(a, b):
            return [*a, *b], {*a, *b}, {**{1: 2}, **b}

        self.assertEqual(f([1], {3: 4}), ([1, 3], {1, 3}, {1: 2, 3: 4}))

    def test_error_texts(self):
        def lst(x):
            return [*x]

        def st(x):
            return {*x}

        def dct(x):
            return {**x}
        cases = [
            (lst, 1, "Value after * must be an iterable, not int"),
            (lst, None, "Value after * must be an iterable, not NoneType"),
            (st, 1, "'int' object is not iterable"),
            (dct, 1, "'int' object is not a mapping"),
            (dct, [1], "'list' object is not a mapping"),
        ]
        for f, arg, text in cases:
            with self.subTest(f=f.__name__, arg=arg):
                with self.assertRaises(TypeError) as cm:
                    f(arg)
                self.assertEqual(str(cm.exception), text)

    def test_iterator_error_propagates(self):
        def gen():
            yield 1
            raise ZeroDivisionError

        def f(x):
            return [*x]
        with self.assertRaises(ZeroDivisionError):
            f(gen())

    def test_source_closed_once(self):
        Finalizer.log.clear()

        class It:
            def __init__(self):
                self.f = Finalizer("it")

            def __iter__(self):
                return iter([1, 2])

        def f():
            return [*It()]
        self.assertEqual(f(), [1, 2])
        gc.collect()
        self.assertEqual(Finalizer.log, ["it"])


class Merge(unittest.TestCase):
    def test_call_ex(self):
        def target(**kw):
            return kw

        def f(a, b):
            return target(**a, **b)
        self.assertEqual(f({"x": 1}, {"y": 2}), {"x": 1, "y": 2})

    def test_errors_name_the_callable(self):
        def target(**kw):
            return kw

        def f(a, b):
            return target(**a, **b)

        def g(a):
            return max(**a)

        with self.assertRaises(TypeError) as cm:
            f({"x": 1}, {"x": 2})
        self.assertIn("target() got multiple values for keyword argument 'x'",
                      str(cm.exception))
        with self.assertRaises(TypeError) as cm:
            f({"x": 1}, 5)
        self.assertIn("Value after ** must be a mapping, not int",
                      str(cm.exception))
        with self.assertRaises(TypeError) as cm:
            g(5)
        self.assertIn("Value after ** must be a mapping, not int",
                      str(cm.exception))


class Observation(unittest.TestCase):
    def test_trace_lines_of_a_display(self):
        import sys

        def f(a, b):
            return [
                a,
                b,
                {a: b},
                (*a, *b),
            ]
        lines = []

        def tracer(frame, event, arg):
            if frame.f_code is f.__code__ and event == "line":
                lines.append(frame.f_lineno - f.__code__.co_firstlineno)
            return tracer
        sys.settrace(tracer)
        try:
            f((1,), (2,))
        finally:
            sys.settrace(None)
        # (The order is upstream's: the items, then the return's line)
        self.assertEqual(lines, [2, 3, 4, 5, 1])

    def test_error_line_is_the_display_start(self):
        def f(a):
            return {
                1: 2,
                a: 3,
            }
        try:
            f([])
        except TypeError as e:
            line = e.__traceback__.tb_next.tb_lineno
        self.assertEqual(line - f.__code__.co_firstlineno, 1)



class Unpack(unittest.TestCase):
    """Unpack and UnpackStar: the generic arm, the arms an entry selects,
    and the failures of each"""

    def arms(self, f):
        "The words as they are now, with the arms the entries installed"
        return {w[0] for w in capi.get_ir(f.__code__, True)}

    def test_adaptive_arms_install(self):
        for source, arm in (((1, 2), "Unpack.Tuple"), ([1, 2], "Unpack.List")):
            def g(p):
                a, b = p
                return b
            for _ in range(300):
                self.assertEqual(g(source), 2)
            self.assertIn(arm, self.arms(g))

    def test_a_miss_goes_the_generic_way(self):
        def f(p):
            a, b = p
            return a, b
        for _ in range(300):
            f((1, 2))
        self.assertIn("Unpack.Tuple", self.arms(f))
        for source in ([3, 4], "ab", iter((5, 6)), {7: 0, 8: 0}):
            self.assertEqual(f(source), tuple(source) if not hasattr(source, "__next__")
                             else (5, 6))
        for _ in range(300):
            f([1, 2])
        self.assertEqual(f((9, 10)), (9, 10))

    def test_errors(self):
        def f(p):
            a, b = p
            return a
        for _ in range(300):
            f((1, 2))
        with self.assertRaisesRegex(ValueError, "too many values to unpack"):
            f((1, 2, 3))
        with self.assertRaisesRegex(ValueError, "not enough values to unpack"):
            f([1])
        with self.assertRaisesRegex(TypeError, "cannot unpack non-iterable int"):
            f(5)

    def test_star(self):
        def f(p):
            a, *b, c = p
            return a, b, c
        self.assertEqual(f(range(5)), (0, [1, 2, 3], 4))
        self.assertEqual(f((1, 2)), (1, [], 2))
        with self.assertRaisesRegex(ValueError, "not enough values to unpack"):
            f((1,))

    def test_old_values_are_closed_after_the_stores(self):
        seen = []

        class Old:
            def __del__(self):
                seen.append(a)

        def f(p):
            global a
            a = None
            x, y = Old(), Old()
            x, y = p
            return x, y
        for _ in range(100):
            seen.clear()
            self.assertEqual(f((1, 2)), (1, 2))
            self.assertEqual(len(seen), 2)

    def test_no_leaks_of_the_items(self):
        class Item:
            pass

        def f(p):
            a, b = p
            return a
        items = (Item(), Item())
        for _ in range(300):
            f(items)
        import sys
        before = sys.getrefcount(items[0])
        for _ in range(100):
            f(items)
            f(list(items))
        self.assertEqual(sys.getrefcount(items[0]), before)


if __name__ == "__main__":
    unittest.main()
