"""Observed code runs in a second compilation of the evaluator.  A monitoring
change sends the frames already on a stack to the switch word, whose entry in
the thread's switch list keeps the word each goes on at
(Python/ceval_evaluator.h).  The list stays exact through exception
unwinding, threads, and generators suspended across the change.

A frame that waits in a native call that does not poll after it returns goes
on in the evaluator it ran in until its next poll; the tests below take
their events where the frame polls or handles an exception."""

import sys
import threading
import unittest

from test.support import import_helper

_testinternalcapi = import_helper.import_module("_testinternalcapi")

monitoring = sys.monitoring
E = monitoring.events
TOOL = monitoring.DEBUGGER_ID
DEPTH = 50


class MonitoringScope:
    """A tool that records LINE events of the given code objects"""

    def __init__(self, *codes):
        self.codes = set(codes)
        self.lines = []

    def __enter__(self):
        monitoring.use_tool_id(TOOL, "test_ir_eval_switch")
        monitoring.register_callback(TOOL, E.LINE, self.line)
        return self

    def __exit__(self, *exc):
        monitoring.set_events(TOOL, 0)
        monitoring.register_callback(TOOL, E.LINE, None)
        monitoring.free_tool_id(TOOL)

    def line(self, code, line):
        if code in self.codes:
            self.lines.append((code.co_name, line - code.co_firstlineno))

    def on(self):
        monitoring.set_events(TOOL, E.LINE)

    def off(self):
        monitoring.set_events(TOOL, 0)


def entries_below_caller():
    """The entries of the frames below the caller after a walk of the stack:
    restart_events() is a monitoring change, which rebuilds the list.  The
    caller's own entry goes when this function returns to it."""
    monitoring.restart_events()
    return _testinternalcapi.eval_switch_count() - 1


def handler(n, scope):
    # The frame waits on its child during the change and handles the error
    # that unwinds through the frames above it
    try:
        recurse(n, scope)
    except ValueError:
        done = True
    return done


def recurse(n, scope):
    if n == 0:
        scope.on()
        raise ValueError
    recurse(n - 1, scope)


class EvalSwitchTests(unittest.TestCase):

    def test_unwind_through_redirected_frames(self):
        with MonitoringScope(handler.__code__, recurse.__code__) as scope:
            base = entries_below_caller()
            self.assertTrue(handler(DEPTH, scope))
            after = _testinternalcapi.eval_switch_count()
            scope.off()
            # The frames that the error unwound left the list; the frames
            # below this one keep theirs
            self.assertEqual(after, base)
            self.assertEqual(entries_below_caller(), base)
        # handler went on in the observing evaluator at its except clause
        self.assertIn(("handler", 6), scope.lines)
        self.assertIn(("handler", 7), scope.lines)

    def test_unwind_on_a_thread_that_another_thread_changes(self):
        ready = threading.Event()
        changed = threading.Event()
        result = {}

        def bottom(n):
            if n == 0:
                ready.set()
                changed.wait()
                raise ValueError
            bottom(n - 1)

        def middle():
            try:
                bottom(DEPTH)
            except ValueError:
                caught = True
            return caught

        def worker():
            base = entries_below_caller()
            result["caught"] = middle()
            result["after"] = _testinternalcapi.eval_switch_count()
            result["base"] = base

        with MonitoringScope(middle.__code__, bottom.__code__) as scope:
            thread = threading.Thread(target=worker)
            thread.start()
            ready.wait()
            scope.on()
            changed.set()
            thread.join()
            scope.off()
        self.assertTrue(result["caught"])
        self.assertEqual(result["after"], result["base"])
        self.assertIn(("middle", 4), scope.lines)
        self.assertIn(("middle", 5), scope.lines)

    def test_generators_suspended_across_the_change(self):
        def gen():
            x = yield 1
            y = yield x
            return y

        def consume(g):
            total = 0
            for value in g:
                total += value
            return total

        def counter(n):
            for i in range(n):
                yield i

        with MonitoringScope(gen.__code__, counter.__code__,
                             consume.__code__) as scope:
            base = entries_below_caller()
            g = gen()
            self.assertEqual(next(g), 1)
            c = counter(4)
            self.assertEqual(next(c), 0)
            thrown = gen()
            next(thrown)
            closed = gen()
            next(closed)
            scope.on()
            self.assertEqual(g.send(2), 2)
            with self.assertRaises(StopIteration):
                g.send(3)
            self.assertEqual(consume(c), 1 + 2 + 3)
            with self.assertRaises(KeyError):
                thrown.throw(KeyError)
            closed.close()
            scope.off()
            self.assertEqual(_testinternalcapi.eval_switch_count(), base)
            lines = list(scope.lines)
            # Resumed after the change, the generators report their lines
            self.assertIn(("gen", 2), lines)
            self.assertIn(("gen", 3), lines)
            self.assertIn(("counter", 1), lines)
            self.assertIn(("consume", 3), lines)
            # And none after it is off
            g = gen()
            next(g)
            self.assertEqual(scope.lines, lines)


if __name__ == "__main__":
    unittest.main()
