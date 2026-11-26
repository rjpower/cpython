"""Length and the pattern ops: GET_LEN, MATCH_MAPPING, MATCH_SEQUENCE,
MATCH_KEYS and MATCH_CLASS (Python/register_ops/patterns.rop)."""

import collections.abc
import sys
import textwrap
import unittest

from test.support import import_helper, script_helper


class Point:
    __match_args__ = ("x", "y")

    def __init__(self, x, y):
        self.x, self.y = x, y


class BadLen:
    __match_args__ = ()

    def __len__(self):
        raise ZeroDivisionError


collections.abc.Sequence.register(BadLen)


class BadGet(dict):
    def get(self, key, default=None):
        raise ZeroDivisionError


def classify(x):
    match x:
        case [a, *rest]:
            return "seq", a, rest
        case {"k": v, **others}:
            return "map", v, others
        case Point(x=1, y=y):
            return "point1", y
        case Point(a, b):
            return "point", a, b
        case int(n):
            return "int", n
        case _:
            return None


class Behavior(unittest.TestCase):
    def test_each_kind(self):
        cases = [
            ([1, 2, 3], ("seq", 1, [2, 3])),
            ({"k": 1, "z": 2}, ("map", 1, {"z": 2})),
            (Point(1, 2), ("point1", 2)),
            (Point(3, 4), ("point", 3, 4)),
            (5, ("int", 5)),
            ("text", None),
            ((), None),
            ({}, None),
        ]
        for _ in range(3):
            for subject, expected in cases:
                with self.subTest(subject=subject):
                    self.assertEqual(classify(subject), expected)

    def test_subject_survives_length(self):
        def f(x):
            match x:
                case [a, b]:
                    return a, b, x
                case _:
                    return x
        subject = [1, 2]
        for _ in range(3):
            self.assertEqual(f(subject), (1, 2, subject))
            self.assertIs(f(subject)[2], subject)

    def test_lengths(self):
        def f(x):
            match x:
                case [*_]:
                    return len(x)
        for n in (0, 1, 1000):
            self.assertEqual(f(list(range(n))), n)

    def test_errors(self):
        def seq(x):
            match x:
                case [a, *b]:
                    return a
        def keys(x):
            match x:
                case {"k": v}:
                    return v
        for _ in range(3):
            with self.assertRaises(ZeroDivisionError):
                seq(BadLen())
            with self.assertRaises(ZeroDivisionError):
                keys(BadGet(k=1))

    def test_class_errors_close_sources(self):
        class Bad:
            @property
            def x(self):
                raise ZeroDivisionError
        def f(x):
            match x:
                case Bad(x=1):
                    return 1
        for _ in range(3):
            with self.assertRaises(ZeroDivisionError):
                f(Bad())

    def test_positional_count(self):
        def f(x):
            match x:
                case Point(a, b, c):
                    return a
        for _ in range(3):
            with self.assertRaises(TypeError):
                f(Point(1, 2))


class Lowering(unittest.TestCase):
    # A word that only reads a stack position leaves its parameter held, so
    # that a later edge does not rename another identity into its home.  A
    # mapping pattern whose sequence value is bound under SWAPs lowered to
    # a register that an edge move had already taken.
    def test_mapping_with_nested_sequence(self):
        import_helper.import_module("_testinternalcapi")
        script = textwrap.dedent("""
            import _testinternalcapi

            def one(x):
                match x:
                    case {'a': [p], 'b': 1}:
                        return p

            def two(x):
                match x:
                    case {'a': [p], 'b': [q]}:
                        return p, q

            def classes(x):
                match x:
                    case {'a': str(p), 'b': str(q)}:
                        return p, q

            for f in (one, two, classes):
                _testinternalcapi.get_ir(f.__code__, True)
            print(one({'a': [7], 'b': 1}), two({'a': [1], 'b': [2]}),
                  classes({'a': 'x', 'b': 'y'}), one({'a': [7], 'b': 2}))
        """)
        rc, out, err = script_helper.assert_python_ok(
            "-c", script, PYTHONREGCFGVERIFY="1")
        self.assertEqual(out.strip(), b"7 (1, 2) ('x', 'y') None")


class Observation(unittest.TestCase):
    def test_branch_events(self):
        events = []
        mon = sys.monitoring
        tool = mon.PROFILER_ID
        mon.use_tool_id(tool, "test_rop_patterns")
        try:
            mon.register_callback(
                tool, mon.events.BRANCH_LEFT, lambda code, src, dst: events.append("L"))
            mon.register_callback(
                tool, mon.events.BRANCH_RIGHT, lambda code, src, dst: events.append("R"))
            mon.set_events(tool, mon.events.BRANCH_LEFT | mon.events.BRANCH_RIGHT)
            classify(5)
            classify("text")
        finally:
            mon.set_events(tool, 0)
            mon.free_tool_id(tool)
        self.assertTrue(events)


if __name__ == "__main__":
    unittest.main()
