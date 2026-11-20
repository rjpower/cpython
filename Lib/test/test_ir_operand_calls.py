"""Calls read their inputs from the operand words that follow the head.

Each shape runs cold, through the generic handlers, and warm, through the
specialized ones; the results are those of the stack interpreter.
"""

import sys
import unittest
import weakref

import _opcode
from test.support import import_helper

_testinternalcapi = import_helper.import_module("_testinternalcapi")

WARM = 300


def head_rows(code):
    "(row, operand rows) of each call in the code"
    rows = _testinternalcapi.get_ir(code)
    result = []
    for i, row in enumerate(rows):
        if row[0].startswith(("CALL", "INSTRUMENTED_CALL")):
            result.append((row, rows[i + 1:i + 1 + (row[3] + 3) // 4]))
    return result


def run(function, *args, **kwargs):
    """The results of a cold call and of the calls after the function has
    been specialized"""
    cold = function(*args, **kwargs)
    for _ in range(WARM):
        warm = function(*args, **kwargs)
    return cold, warm


class Point:
    def __init__(self, x, y=0):
        self.x, self.y = x, y

    def method(self, a, b=2):
        return self.x + a + b

    @staticmethod
    def static(a):
        return a + 1


def add(a, b):
    return a + b


def many(a, b, c, d, e, f, g, h, i):
    return a + b + c + d + e + f + g + h + i


class OperandCallShapeTests(unittest.TestCase):
    def check(self, function, expected, *args, **kwargs):
        for result in run(function, *args, **kwargs):
            self.assertEqual(result, expected)

    def test_positional_call(self):
        def f(x, y):
            return add(x, y)
        self.check(f, 5, 2, 3)
        (head, operands), = head_rows(f.__code__)
        self.assertEqual(head[3], 2)
        self.assertEqual([row[0] for row in operands], ["IR_OPERANDS"])

    def test_method_call(self):
        def f(o, x):
            return o.method(x)
        self.check(f, 10 + 3 + 2, Point(10), 3)
        (head, operands), = head_rows(f.__code__)
        # The self operand is the first of two
        self.assertEqual(head[3], 2)
        self.assertNotIn("NO_SELF", head[0])

    def test_call_in_an_argument_keeps_the_snapshot(self):
        def g():
            return 1

        def f(x):
            return add(x, g())
        self.check(f, 8, 7)

    def test_star_call(self):
        def f(a, k):
            return add(*a, **k)
        self.check(f, 3, (1,), {"b": 2})
        (head, operands), = head_rows(f.__code__)
        self.assertEqual(head[3], 2)

    def test_star_call_without_keywords(self):
        def f(a):
            return add(*a)
        self.check(f, 3, (1, 2))
        (head, operands), = head_rows(f.__code__)
        self.assertEqual(head[3], 1)
        self.assertIn("NO_KW", head[0])

    def test_keyword_call(self):
        def f(a):
            return add(a, b=1)
        self.check(f, 6, 5)
        (head, operands), = head_rows(f.__code__)
        # a, the value 1 and the index of the names in co_consts
        self.assertEqual(head[3], 3)
        names = _opcode.IR_OPERANDS and operands[0][3]
        self.assertEqual(f.__code__.co_consts[names], ("b",))

    def test_builtins(self):
        def f(x, t):
            return len(x), isinstance(x, t), type(x), str(len(x)), tuple(x)
        self.check(f, (3, True, list, "3", (1, 2, 3)), [1, 2, 3], list)

    def test_constructor_with_init(self):
        def f(x):
            return Point(x)
        for point in run(f, 4):
            self.assertEqual((point.x, point.y), (4, 0))

    def test_bound_method_value(self):
        def f(m, x):
            return m(x)
        self.check(f, 10 + 4 + 2, Point(10).method, 4)

    def test_static_and_class_attribute_calls(self):
        # LOAD_ATTR leaves a plain function, and the call has no self
        def f(o, x):
            return o.static(x)
        self.check(f, 6, Point(0), 5)
        self.check(f, 6, Point, 5)

    def test_method_descriptor(self):
        def f(items, text):
            items.append(text.upper())
            return items.index("A"), "a,b".split(","), text.count("a")
        self.check(f, (1, ["a", "b"], 1), ["x"], "a")

    def test_descriptor_called_through_its_class(self):
        # The self operand is a slot that is NULL at run time
        def f(items, i):
            return list.__getitem__(items, i)
        self.check(f, 7, [5, 6, 7], 2)

    def test_subclass_calls_the_base_method(self):
        class Base(list):
            def __getitem__(self, i):
                if isinstance(i, slice):
                    return i.start, i.stop
                return list.__getitem__(self, i) + 100
        items = Base([0, 1, 2])
        for _ in range(WARM):
            self.assertEqual(items[1], 101)
            self.assertEqual(items[3:4], (3, 4))

    def test_class_of_self_called_with_self(self):
        # The callable is a class attribute that is not a method, so the
        # self operand is NULL and the arguments follow it
        class Copy:
            def __init__(self, data=None):
                self.data = data

            def copy(self):
                return self.__class__(self)

        copy = Copy()
        for _ in range(WARM):
            self.assertIs(copy.copy().data, copy)

    def test_more_than_four_operands(self):
        def f(a):
            return many(a, a, a, a, a, 1, 2, 3, 4)
        (head, operands), = head_rows(f.__code__)
        self.assertEqual(head[3], 9)
        self.assertEqual(len(operands), 3)
        self.check(f, 5 * 3 + 10, 3)

    def test_more_than_seven_arguments(self):
        def f(a):
            return max(a, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10)
        self.check(f, 10, 0)
        self.check(f, 20, 20)

    def test_generic_call_to_a_python_function(self):
        def f(function, args):
            return function(*args)
        self.check(f, 3, add, (1, 2))
        self.check(f, 36, many, tuple(range(9)))

    def test_call_with_a_weakly_referenced_bound_method(self):
        events = []

        def f(m):
            return m()

        class A:
            def method(self):
                return "a"

        a = A()
        for _ in range(WARM):
            method = a.method
            reference = weakref.ref(method, events.append)
            self.assertEqual(f(method), "a")
            del method
            self.assertEqual(len(events), 1)
            events.clear()
            del reference


class EvaluationOrderTests(unittest.TestCase):
    def test_call_sees_the_old_local(self):
        seen = []

        def g():
            frame = sys._getframe(1)
            frame.f_locals["x"] = "new"
            return "g"

        def record(a, b):
            seen.append((a, b))

        def f():
            x = "old"
            record(x, g())
            return x

        for _ in range(WARM):
            seen.clear()
            self.assertEqual(f(), "new")
            self.assertEqual(seen, [("old", "g")])

    def test_method_call_sees_the_old_receiver_and_argument(self):
        seen = []

        class Recorder:
            def method(self, a):
                seen.append((self.name, a))

        first, second = Recorder(), Recorder()
        first.name, second.name = "first", "second"

        def g():
            frame = sys._getframe(1)
            frame.f_locals["o"] = second
            frame.f_locals["y"] = "new"
            return "g"

        def f():
            o = first
            y = "old"
            o.method(y)
            o.method(g())
            return y

        for _ in range(WARM):
            seen.clear()
            f()
            self.assertEqual(seen, [("first", "old"), ("first", "g")])

    def test_keyword_call_sees_the_old_local(self):
        seen = []

        def record(a, b=None):
            seen.append((a, b))

        def g():
            sys._getframe(1).f_locals["x"] = "new"
            return "g"

        def f():
            x = "old"
            record(x, b=g())

        for _ in range(WARM):
            seen.clear()
            f()
            self.assertEqual(seen, [("old", "g")])


class ObservationTests(unittest.TestCase):
    """Operand words are not instructions: no instruction, line or opcode
    event is delivered for them."""

    def test_instruction_events_are_per_head(self):
        events = []
        monitoring = sys.monitoring
        tool = monitoring.PROFILER_ID

        def f(x):
            return add(x, 1)

        monitoring.use_tool_id(tool, "operand-calls")
        self.addCleanup(monitoring.free_tool_id, tool)
        monitoring.register_callback(
            tool, monitoring.events.INSTRUCTION,
            lambda code, offset: events.append(offset)
            if code is f.__code__ else None)
        self.addCleanup(monitoring.register_callback, tool,
                        monitoring.events.INSTRUCTION, None)
        monitoring.set_local_events(tool, f.__code__,
                                    monitoring.events.INSTRUCTION)
        self.addCleanup(monitoring.set_local_events, tool, f.__code__, 0)
        self.assertEqual(f(1), 2)
        rows = _testinternalcapi.get_ir(f.__code__)
        # RESUME, at offset 0, has no event of its own
        heads = [i for i, row in enumerate(rows)
                 if row[0] not in ("IR_OPERANDS", "RESUME")]
        operand_words = [i for i, row in enumerate(rows)
                         if row[0] == "IR_OPERANDS"]
        self.assertTrue(operand_words)
        self.assertEqual(events, heads)

    def test_call_event_sees_callable_and_first_argument(self):
        events = []
        monitoring = sys.monitoring
        tool = monitoring.PROFILER_ID

        def f(x):
            return add(x, 1)

        monitoring.use_tool_id(tool, "operand-calls")
        self.addCleanup(monitoring.free_tool_id, tool)
        monitoring.register_callback(
            tool, monitoring.events.CALL,
            lambda code, offset, callable, arg0: events.append((callable, arg0)))
        self.addCleanup(monitoring.register_callback, tool,
                        monitoring.events.CALL, None)
        monitoring.set_local_events(tool, f.__code__, monitoring.events.CALL)
        self.addCleanup(monitoring.set_local_events, tool, f.__code__, 0)
        self.assertEqual(f(41), 42)
        self.assertEqual(events, [(add, 41)])

    def test_settrace_opcode_events_skip_operand_words(self):
        offsets = []

        def f(x):
            return add(x, 1)

        def tracer(frame, event, arg):
            if frame.f_code is f.__code__:
                frame.f_trace_opcodes = True
                if event == "opcode":
                    offsets.append(frame.f_lasti)
            return tracer

        sys.settrace(tracer)
        try:
            f(1)
        finally:
            sys.settrace(None)
        rows = _testinternalcapi.get_ir(f.__code__)
        operand_words = {i for i, row in enumerate(rows)
                         if row[0] == "IR_OPERANDS"}
        self.assertTrue(operand_words)
        self.assertTrue(offsets)
        self.assertFalse(operand_words & set(offsets))


if __name__ == "__main__":
    unittest.main()
