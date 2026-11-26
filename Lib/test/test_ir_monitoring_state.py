"""Monitoring displaces the handlers of a word with the LINE and INSTRUCTION
wrappers, and observed code runs in the observing evaluator.  The code
object's own words are what comes back, whatever order the events are
removed in."""

import sys
import unittest

monitoring = sys.monitoring
TOOL = monitoring.DEBUGGER_ID
E = monitoring.events


def work(n):
    total = 0
    for i in range(n):
        total = total + i * 2
        if total % 3:
            total += 1
    return total


def run_counting(events):
    counts = {}

    def record(name):
        def callback(*args):
            counts[name] = counts.get(name, 0) + 1
        return callback

    names = {E.LINE: "line", E.INSTRUCTION: "instruction", E.CALL: "call",
             E.BRANCH_RIGHT: "branch"}
    for event, name in names.items():
        monitoring.register_callback(TOOL, event, record(name))
    monitoring.use_tool_id(TOOL, "test_ir_monitoring_state")
    try:
        monitoring.set_events(TOOL, events)
        result = work(20)
    finally:
        monitoring.set_events(TOOL, 0)
        for event in names:
            monitoring.register_callback(TOOL, event, None)
        monitoring.free_tool_id(TOOL)
    return result, counts


class MonitoringStateTests(unittest.TestCase):
    def setUp(self):
        self.expected = work(20)

    def specialize(self):
        # Warm the function so that its arithmetic words specialise
        for _ in range(200):
            work(20)

    def test_specialised_words_come_back(self):
        self.specialize()
        for events in (E.CALL | E.BRANCH_RIGHT, E.LINE, E.INSTRUCTION,
                       E.LINE | E.INSTRUCTION | E.BRANCH_RIGHT, 0):
            with self.subTest(events=events):
                result, _ = run_counting(events)
                self.assertEqual(result, self.expected)
                self.assertEqual(work(20), self.expected)

    def test_line_over_instruction_removed_in_both_orders(self):
        monitoring.use_tool_id(TOOL, "test_ir_monitoring_state")
        counts = {"line": 0, "instruction": 0}

        def on_line(*args):
            counts["line"] += 1

        def on_instruction(*args):
            counts["instruction"] += 1

        monitoring.register_callback(TOOL, E.LINE, on_line)
        monitoring.register_callback(TOOL, E.INSTRUCTION, on_instruction)
        try:
            for first, second in ((E.LINE, E.INSTRUCTION), (E.INSTRUCTION, E.LINE)):
                with self.subTest(first=first):
                    monitoring.set_events(TOOL, E.LINE | E.INSTRUCTION)
                    counts.update(line=0, instruction=0)
                    work(5)
                    self.assertTrue(counts["line"] and counts["instruction"])
                    # Remove one, the other stays
                    monitoring.set_events(TOOL, second)
                    counts.update(line=0, instruction=0)
                    work(5)
                    kept = "line" if second == E.LINE else "instruction"
                    gone = "instruction" if kept == "line" else "line"
                    self.assertTrue(counts[kept])
                    self.assertEqual(counts[gone], 0)
                    # Remove the last
                    monitoring.set_events(TOOL, 0)
                    counts.update(line=0, instruction=0)
                    self.assertEqual(work(20), self.expected)
                    self.assertEqual(counts, {"line": 0, "instruction": 0})
        finally:
            monitoring.set_events(TOOL, 0)
            monitoring.register_callback(TOOL, E.LINE, None)
            monitoring.register_callback(TOOL, E.INSTRUCTION, None)
            monitoring.free_tool_id(TOOL)


if __name__ == "__main__":
    unittest.main()
