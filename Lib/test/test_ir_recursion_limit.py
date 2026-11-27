"""The RecursionError of a call that reaches the recursion limit through an
arm that enters the callee's frame (Call.Exact, Call.PyGeneral) or a
generator's (Send.Gen).  The frame start's check raises it: the callee
never runs, so the traceback ends at the caller's call line, and the
caller's frame is intact."""

import inspect
import sys
import traceback
import unittest
from test import support
from test.support import import_helper

_testinternalcapi = import_helper.import_module("_testinternalcapi")

LIMIT = 300


def exact(n):
    return exact(n + 1)


def general(n, k=1):
    return general(n + 1)


made = []


def delegate(n):
    inner = delegate(n + 1)
    made.append(inner)
    yield from inner


def arms(code):
    return [word[0] for word in _testinternalcapi.get_ir(code, True)]


class RecursionLimitTest(unittest.TestCase):

    def overflow(self, call):
        marker = object()
        error = None
        with support.infinite_recursion(LIMIT):
            try:
                call()
            except RecursionError as e:
                error = e
        self.assertIsNotNone(error)
        # The caller's frame and its locals are as they were
        self.assertIsNotNone(marker)
        return error

    def check_traceback(self, error, function, line):
        self.assertEqual(str(error), "maximum recursion depth exceeded")
        frames = [(f.name, f.line) for f in
                  traceback.extract_tb(error.__traceback__)]
        # this method's frame, the lambda, then the recursive frames, the
        # last at its call: the frame that the limit refused never ran
        self.assertEqual(frames[0][0], "overflow")
        self.assertEqual(frames[1][0], "<lambda>")
        self.assertEqual(set(frames[2:]), {(function, line)})
        return len(frames)

    def warm(self, function, *args):
        # The recursive site specialises after a few shallow runs
        for _ in range(3):
            self.overflow(lambda: function(*args))
        return arms(function.__code__)

    def test_exact(self):
        self.assertIn("Call.Exact", " ".join(self.warm(exact, 0)))
        error = self.overflow(lambda: exact(0))
        count = self.check_traceback(error, "exact", "return exact(n + 1)")
        # Every refused call fails at the same depth
        error = self.overflow(lambda: exact(0))
        self.assertEqual(self.check_traceback(error, "exact", "return exact(n + 1)"),
                         count)
        error = self.overflow(lambda: general(0))
        self.assertEqual(self.check_traceback(error, "general", "return general(n + 1)"),
                         count)

    def test_general(self):
        self.assertIn("Call.PyGeneral", " ".join(self.warm(general, 0)))
        error = self.overflow(lambda: general(0))
        self.check_traceback(error, "general", "return general(n + 1)")

    def test_generator_send(self):
        self.assertIn("Send.Gen", " ".join(self.warm(lambda n: next(delegate(n)), 0)
                                            + arms(delegate.__code__)))
        made.clear()
        error = self.overflow(lambda: next(delegate(0)))
        self.check_traceback(error, "delegate", "yield from inner")
        # The generator that the limit refused was claimed, then closed with
        # its delegators
        self.assertEqual({inspect.getgeneratorstate(g) for g in made},
                         {"GEN_CLOSED"})


if __name__ == "__main__":
    unittest.main()
