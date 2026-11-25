"""A conditional branch word whose BRANCH event moves the frame.

A sys.monitoring callback that sets the caller's f_lineno takes the branch
word's redirect exit (_PyIR_REDIRECT): the word leaves for the frame's new
position, and the skipped statement never runs.
"""

import sys
import unittest
from test.support import import_helper

_testinternalcapi = import_helper.import_module('_testinternalcapi')


def truth_branch(x):
    result = 0
    if x:
        result = 1
        result = 2
    else:
        result = 3
        result = 4
    return result


def none_branch(x):
    result = 0
    if x is None:
        result = 1
        result = 2
    else:
        result = 3
        result = 4
    return result


# the first line of each arm, and the line after it
# (offsets from co_firstlineno)
TAKEN, NOT_TAKEN = (3, 4), (6, 7)


def run_jumping(func, arg, target):
    """Run func(arg) with a BRANCH callback that moves the caller's frame
    to the line at offset target; return the result and the edges seen"""
    mon = sys.monitoring
    tool = mon.DEBUGGER_ID
    first = func.__code__.co_firstlineno
    edges = []

    def callback(code, source, destination):
        edges.append((source, destination))
        sys._getframe(1).f_lineno = first + target

    mon.use_tool_id(tool, "test_ir_redirect")
    try:
        for event in (mon.events.BRANCH_LEFT, mon.events.BRANCH_RIGHT):
            mon.register_callback(tool, event, callback)
        mon.set_local_events(
            tool, func.__code__,
            mon.events.BRANCH_LEFT | mon.events.BRANCH_RIGHT)
        result = func(arg)
    finally:
        mon.set_local_events(tool, func.__code__, 0)
        for event in (mon.events.BRANCH_LEFT, mon.events.BRANCH_RIGHT):
            mon.register_callback(tool, event, None)
        mon.free_tool_id(tool)
    return result, edges


class BranchRedirectTests(unittest.TestCase):
    CASES = (
        # func, arg that takes the first arm, arg that takes the second
        (truth_branch, True, False),
        (truth_branch, 5, 0),
        (truth_branch, [1], []),
        (truth_branch, "s", ""),
        (truth_branch, object(), None),
        (none_branch, None, 0),
    )

    def warm(self, func, arg):
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 10):
            func(arg)

    def test_branch_event_redirects(self):
        for func, yes, no in self.CASES:
            for arg, (_, target), expected in (
                    (yes, TAKEN, 2), (no, NOT_TAKEN, 4)):
                with self.subTest(func=func.__name__, arg=arg):
                    # The jump skips the first statement of the arm
                    self.warm(func, arg)
                    self.assertEqual(func(arg), expected)
                    result, edges = run_jumping(func, arg, target)
                    self.assertEqual(len(edges), 1)
                    self.assertEqual(result, expected)
                    self.assertEqual(func(arg), expected)

    def test_redirect_skips_statement(self):
        def func(x):
            seen = []
            if x:
                seen.append("skipped")
                seen.append("kept")
            return seen

        self.warm(func, True)
        result, edges = run_jumping(func, True, 4)
        self.assertEqual(len(edges), 1)
        self.assertEqual(result, ["kept"])
        self.assertEqual(func(True), ["skipped", "kept"])


if __name__ == '__main__':
    unittest.main()
