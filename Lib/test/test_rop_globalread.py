"""Tests of the GlobalRead family of the register instruction definitions
(Python/register_ops/names.rop): the lowering of LOAD_GLOBAL, NameError,
non-dict namespaces, and observation of its words."""

import dis
import sys
import types
import unittest
from pathlib import Path

from test import test_tools
from test.support import import_helper

test_tools.skip_if_missing("cases_generator")
with test_tools.imports_under_tool("cases_generator"):
    import rop_verify

_testinternalcapi = import_helper.import_module("_testinternalcapi")

ROOT = Path(test_tools.basepath)
ARMS = ["Generic", "Module", "Builtin"]


def program():
    return rop_verify.load(sorted((ROOT / "Python" / "register_ops").glob("*.rop")))


def names(fn):
    return [row[0] for row in _testinternalcapi.get_ir(fn.__code__)]


def adapted(fn, *args):
    """The words of the executing copy, after enough calls to specialise"""
    for _ in range(300):
        try:
            fn(*args)
        except NameError:
            pass
    return [row[0] for row in _testinternalcapi.get_ir(fn.__code__, True)]


def warmed(fn, *args):
    for _ in range(3):
        try:
            fn(*args)
        except NameError:
            pass
    return names(fn)


counter = 3


def read_global():
    return counter


def call_builtin():
    return len("ab")


def discard_global():
    counter


def read_dyn():
    return dyn


def missing():
    return not_defined_anywhere


class DefinitionTests(unittest.TestCase):
    def test_definitions_check_clean(self):
        self.assertEqual(list(program().diagnostics), [])

    def test_arms_are_defined(self):
        arms = program().arms
        for name in ARMS:
            with self.subTest(arm=name):
                self.assertIn(f"GlobalRead.{name}", arms)


class LoweringTests(unittest.TestCase):
    def test_words_are_global_reads(self):
        self.assertIn("GlobalRead.Generic", warmed(read_global))
        self.assertEqual(read_global(), 3)

    def test_call_null_has_no_word(self):
        words = warmed(call_builtin)
        self.assertEqual(sum(w.startswith("GlobalRead") for w in words), 1)
        self.assertNotIn("PUSH_NULL", words)
        self.assertIn("Call.Generic+NO_SELF", words)
        self.assertEqual(call_builtin(), 2)

    def test_discarded_read_has_no_retire(self):
        words = warmed(discard_global)
        self.assertEqual(words.count("GlobalRead.Generic"), 1)
        self.assertFalse([w for w in words if w.startswith("Move.Retire")], words)
        discard_global()

    def test_name_error(self):
        for _ in range(3):
            try:
                missing()
            except NameError as e:
                self.assertEqual(e.name, "not_defined_anywhere")
                tb = e.__traceback__
                while tb.tb_next:
                    tb = tb.tb_next
                self.assertEqual(tb.tb_lineno, missing.__code__.co_firstlineno + 1)
            else:
                self.fail("no NameError")


class ArmTests(unittest.TestCase):
    def test_module_and_builtin_arms(self):
        ns = {}
        exec("v = 1\ndef f():\n    return v + len('a')", ns)
        words = adapted(ns["f"])
        self.assertIn("GlobalRead.Module", words)
        self.assertIn("GlobalRead.Builtin", words)
        self.assertEqual(ns["f"](), 2)

    def test_module_misses_on_new_key_and_deletion(self):
        ns = {"v": 1}
        exec("def f():\n    return v", ns)
        f = ns["f"]
        self.assertIn("GlobalRead.Module", adapted(f))
        for i in range(5):
            ns["w%d" % i] = i
            self.assertEqual(f(), 1)
        del ns["v"]
        with self.assertRaises(NameError):
            f()
        ns["v"] = 7
        self.assertEqual(f(), 7)
        for _ in range(300):
            self.assertEqual(f(), 7)

    def test_builtin_misses_when_shadowed_or_changed(self):
        ns = {"__builtins__": {"b": 1}}
        exec("def f():\n    return b", ns)
        f = ns["f"]
        self.assertIn("GlobalRead.Builtin", adapted(f))
        ns["b"] = 2
        self.assertEqual(f(), 2)
        del ns["b"]
        self.assertEqual(f(), 1)
        ns["__builtins__"]["b"] = 3
        self.assertEqual(f(), 3)
        del ns["__builtins__"]["b"]
        with self.assertRaises(NameError):
            f()

    def test_refcounts_do_not_drift(self):
        class Box:
            pass

        ns = {"v": Box()}
        exec("def f():\n    x = v\n    return None", ns)
        f = ns["f"]
        adapted(f)
        before = sys.getrefcount(ns["v"])
        for _ in range(1000):
            f()
        self.assertEqual(sys.getrefcount(ns["v"]), before)

    def test_value_released_when_global_is_discarded(self):
        class Box:
            released = 0

            def __del__(self):
                Box.released += 1

        ns = {}
        exec("def f():\n    v\n", ns)
        for _ in range(300):
            ns["v"] = Box()
            ns["f"]()
        ns["v"] = None
        self.assertEqual(Box.released, 300)


class NamespaceTests(unittest.TestCase):
    def test_mapping_globals_and_builtins(self):
        class Namespace(dict):
            def __missing__(self, name):
                return "from-missing:" + name

        class Builtins:
            def __getitem__(self, name):
                if name == "dyn":
                    return 42
                raise KeyError(name)

        fn = types.FunctionType(read_dyn.__code__, {"__builtins__": Builtins()})
        for _ in range(3):
            self.assertEqual(fn(), 42)

    def test_exec_namespace(self):
        ns = {"__builtins__": {"x": 1}}
        exec("def f():\n    return x\nr = [f(), f(), f()]", ns)
        self.assertEqual(ns["r"], [1, 1, 1])

    def test_rebinding_is_seen(self):
        ns = {"v": 1}
        exec("def f():\n    return v", ns)
        for i in range(10):
            ns["v"] = i
            self.assertEqual(ns["f"](), i)
        del ns["v"]
        with self.assertRaises(NameError):
            ns["f"]()

    def test_aliasing_with_local(self):
        counter_alias = 7

        def f():
            counter = counter_alias
            counter = counter_read()
            return counter

        def counter_read():
            return counter

        for _ in range(3):
            self.assertEqual(f(), 3)


class ObservationTests(unittest.TestCase):
    def test_line_events(self):
        def target():
            a = counter
            b = counter
            return a + b

        words = adapted(target)
        self.assertIn("GlobalRead.Module", words)
        lines = []

        def tracer(frame, event, arg):
            if frame.f_code is target.__code__ and event == "line":
                lines.append(frame.f_lineno - target.__code__.co_firstlineno)
            return tracer

        sys.settrace(tracer)
        try:
            target()
        finally:
            sys.settrace(None)
        self.assertEqual(lines, [1, 2, 3])

    def test_monitoring_line_and_instruction(self):
        mon = sys.monitoring
        tool = mon.DEBUGGER_ID
        mon.use_tool_id(tool, "test_rop_globalread")
        seen = {"line": 0, "instruction": 0}

        def target():
            return counter

        words = adapted(target)
        self.assertIn("GlobalRead.Module", words)
        code = target.__code__

        def on_line(c, line):
            if c is code:
                seen["line"] += 1

        offsets = []

        def on_instruction(c, offset):
            if c is code:
                seen["instruction"] += 1
                offsets.append(offset)

        try:
            mon.register_callback(tool, mon.events.LINE, on_line)
            mon.register_callback(tool, mon.events.INSTRUCTION, on_instruction)
            mon.set_events(tool, mon.events.LINE | mon.events.INSTRUCTION)
            self.assertEqual(target(), 3)
            mon.set_events(tool, 0)
        finally:
            mon.register_callback(tool, mon.events.LINE, None)
            mon.register_callback(tool, mon.events.INSTRUCTION, None)
            mon.free_tool_id(tool)
        self.assertEqual(seen["line"], 1)
        load = [i.offset for i in dis.get_instructions(code)
                if i.opname == "LOAD_GLOBAL"]
        self.assertEqual(len(load), 1)
        self.assertIn(load[0], offsets)


if __name__ == "__main__":
    unittest.main()
