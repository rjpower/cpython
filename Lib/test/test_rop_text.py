"""BuildText, Convert and Format (Python/register_ops/text.rop): the words
that f-strings and t-strings lower to, and what each arm does on its success,
error and aliasing paths."""

import sys
import unittest

from test.support import import_helper

capi = import_helper.import_module("_testinternalcapi")


def names(f):
    f.__code__  # a function lowers at its first call
    return [w[0] for w in capi.get_ir(f.__code__)]


class Finalizer(str):
    "A str whose release runs Python"
    log = []

    def __del__(self):
        Finalizer.log.append(str(self))


class Formats:
    def __init__(self, result):
        self.result = result

    def __format__(self, spec):
        return self.result


class Raises:
    def __str__(self):
        raise ZeroDivisionError("str")

    __repr__ = __str__

    def __format__(self, spec):
        raise ZeroDivisionError("format")


class BadStr:
    def __str__(self):
        return 1


class Words(unittest.TestCase):
    def test_words(self):
        def f(a, b, c):
            return f"{a}{b!r:>{c}}x{a!s}{a!a}"
        self.assertEqual(f(1, 2, 3), "1  2x11")
        found = names(f)
        for word in ("Format.Simple", "FormatSpec.Value", "Convert.Text",
                     "BuildText.Join"):
            self.assertIn(word, found)
        for old in ("FORMAT_SIMPLE", "FORMAT_WITH_SPEC", "CONVERT_VALUE",
                    "BUILD_STRING"):
            self.assertNotIn(old, found)

    def test_template_words(self):
        def f(x):
            return t"a{x}b{x!r:5}"
        t = f(7)
        self.assertEqual(t.strings, ("a", "b", ""))
        self.assertEqual([i.conversion for i in t.interpolations], [None, "r"])
        self.assertEqual([i.format_spec for i in t.interpolations], ["", "5"])
        found = names(f)
        self.assertEqual(found.count("BuildText.Interpolation"), 2)
        self.assertIn("BuildText.Template", found)


class Behavior(unittest.TestCase):
    def test_values(self):
        def f(a):
            return f"{a}|{a!r}|{a!s}|{a!a}|{a:>6}"
        for _ in range(3):
            for v in (1, 2**60, 1.5, True, "s", "\u00e9"):
                expected = "|".join((format(v), repr(v), str(v), ascii(v),
                                     format(v, ">6")))
                self.assertEqual(f(v), expected)

    def test_exact_str_is_the_value(self):
        def f(a):
            return f"{a}"
        s = "x" * 5
        for _ in range(3):
            self.assertIs(f(s), s)

    def test_str_subclass_runs_the_protocol(self):
        class S(str):
            def __format__(self, spec):
                return "formatted"

        def f(a):
            return f"{a}"
        for _ in range(3):
            self.assertEqual(f(S("s")), "formatted")

    def test_aliasing(self):
        def f(a):
            a = f"{a}"
            return a
        for _ in range(3):
            self.assertEqual(f(5), "5")
            s = "str"
            self.assertIs(f(s), s)

        def g(a, b):
            a = f"{a}{b}"
            return a
        for _ in range(3):
            self.assertEqual(g("x", "y"), "xy")

    def test_errors_and_lines(self):
        def f(a):
            x = 1
            return (x,
                    f"{a!s}")
        for _ in range(3):
            try:
                f(Raises())
            except ZeroDivisionError as exc:
                self.assertEqual(exc.__traceback__.tb_next.tb_lineno,
                                 f.__code__.co_firstlineno + 3)
            else:
                self.fail("no ZeroDivisionError")
            with self.assertRaises(TypeError):
                f(BadStr())

        def g(a):
            return f"{a}"

        def h(a):
            return f"{a:x}"
        for _ in range(3):
            with self.assertRaises(ZeroDivisionError):
                g(Raises())
            with self.assertRaises(ZeroDivisionError):
                h(Raises())
            with self.assertRaises(ValueError):
                h(1.5)

    def test_error_closes_the_pieces(self):
        Finalizer.log.clear()

        def f(a):
            return f"{Finalizer('p')}{a}"
        for _ in range(3):
            with self.assertRaises(ZeroDivisionError):
                f(Raises())
        self.assertEqual(Finalizer.log, ["p"] * 3)

    def test_finalizer_runs_python_on_release(self):
        Finalizer.log.clear()

        def f():
            return f"{Finalizer('a')}{Finalizer('b')}"
        for _ in range(3):
            self.assertEqual(f(), "ab")
        self.assertEqual(sorted(Finalizer.log), ["a", "a", "a", "b", "b", "b"])

    def test_join_sizes(self):
        for n in (0, 1, 2, 3, 8, 70):
            src = "def f(a):\n    return f'" + "{a}" * n + "'\n"
            ns = {}
            exec(src, ns)
            for _ in range(3):
                self.assertEqual(ns["f"]("ab"), "ab" * n)

    def test_template_values(self):
        def f(x, y):
            return t"{x!r:{y}}-{y}"
        for _ in range(3):
            t = f("s", 5)
            self.assertEqual(t.interpolations[0].value, "s")
            self.assertEqual(t.interpolations[0].conversion, "r")
            self.assertEqual(t.interpolations[0].format_spec, "5")
            self.assertEqual(t.interpolations[1].value, 5)

    def test_inline_number_in_interpolation(self):
        def f(x):
            return t"{x}"
        for _ in range(3):
            self.assertIs(type(f(2**40).interpolations[0].value), int)
            self.assertEqual(f(2**40).interpolations[0].value, 2**40)


class Observation(unittest.TestCase):
    def test_lines_and_instructions(self):
        def f(a):
            return (f"{a}"
                    f"{a!r}")
        lines = []

        def tracer(frame, event, arg):
            if frame.f_code is f.__code__:
                if event == "line":
                    lines.append(frame.f_lineno - f.__code__.co_firstlineno)
                return tracer
        f(1)
        sys.settrace(tracer)
        try:
            self.assertEqual(f(1), "11")
        finally:
            sys.settrace(None)
        # The last instruction of the expression is on the first line again
        self.assertEqual(lines, [1, 2, 1])


if __name__ == "__main__":
    unittest.main()
