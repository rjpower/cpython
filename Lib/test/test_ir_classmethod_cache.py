"""Class-receiver classmethod capture through the ordinary owned call pair."""

import dis
import importlib.util
import sys
import unittest
import weakref

from test.support import (cpython_only, requires_gil_enabled,
                          requires_specialization, reset_code, gc_collect)
from test.support.import_helper import import_module
from test.test_ir_attr_cache import handlers, warm

_testinternalcapi = import_module("_testinternalcapi")


def make_invoke():
    def invoke(cls, value):
        return cls.selected(value)
    return reset_code(invoke)


@cpython_only
@requires_specialization
@requires_gil_enabled("classmethod capture uses the GIL")
class TestClassMethodCache(unittest.TestCase):
    def assert_cached(self, invoke):
        names = handlers(invoke)
        self.assertIn("LOAD_ATTR_CLASSMETHOD", names)
        self.assertIn("CALL_PY_EXACT_ARGS", names)
        self.assertNotIn("CALL_BOUND_METHOD_EXACT_ARGS", names)

    def warm_invoke(self, invoke, cls, value=7):
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            invoke(cls, value)
        self.assert_cached(invoke)

    def test_binding_and_escaping_attribute(self):
        class Base:
            @classmethod
            def selected(cls, value):
                return cls, value

        class Receiver(Base):
            pass

        invoke = make_invoke()
        self.warm_invoke(invoke, Receiver)
        self.assertEqual(invoke(Receiver, 8), (Receiver, 8))
        self.assertEqual(invoke(Base, 9), (Base, 9))
        escaped = Receiver.selected
        self.assertIs(escaped.__self__, Receiver)
        self.assertIs(escaped.__func__, Base.__dict__["selected"].__func__)
        self.assertEqual(escaped(10), (Receiver, 10))

    def test_zero_and_multiple_arguments(self):
        class Receiver:
            @classmethod
            def zero(cls):
                return cls

            @classmethod
            def many(cls, a, b):
                return cls, a, b

        def zero(cls):
            return cls.zero()

        def many(cls):
            return cls.many(1, 2)

        for invoke in (reset_code(zero), reset_code(many)):
            warm(invoke, Receiver)
            self.assert_cached(invoke)
        self.assertIs(zero(Receiver), Receiver)
        self.assertEqual(many(Receiver), (Receiver, 1, 2))

    def test_reinitialize_before_and_during_arguments(self):
        def original(cls, value):
            return "original", cls, value

        def replacement(cls, value):
            return "replacement", cls, value

        descriptor = classmethod(original)
        Receiver = type("Receiver", (), {"selected": descriptor})
        invoke = make_invoke()
        self.warm_invoke(invoke, Receiver)
        descriptor.__init__(replacement)
        self.assertEqual(invoke(Receiver, 8), ("replacement", Receiver, 8))

        def capture(cls, argument):
            return cls.selected(argument())

        reset_code(capture)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            capture(Receiver, lambda: 7)
        self.assertIn("LOAD_ATTR_CLASSMETHOD", handlers(capture))

        def mutate():
            descriptor.__init__(original)
            return 9

        self.assertEqual(capture(Receiver, mutate), ("replacement", Receiver, 9))
        self.assertEqual(invoke(Receiver, 10), ("original", Receiver, 10))

    def test_descriptor_and_bases_mutate_after_capture(self):
        class Base:
            @classmethod
            def selected(cls, value):
                return "base", cls, value

        class Other:
            @classmethod
            def selected(cls, value):
                return "other", cls, value

        class Receiver(Base):
            pass

        def capture(cls, argument):
            return cls.selected(argument())

        reset_code(capture)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            capture(Receiver, lambda: 7)
        self.assertIn("LOAD_ATTR_CLASSMETHOD", handlers(capture))

        def mutate():
            Receiver.__bases__ = (Other,)
            del Base.selected
            return 11

        self.assertEqual(capture(Receiver, mutate), ("base", Receiver, 11))
        self.assertEqual(capture(Receiver, lambda: 12), ("other", Receiver, 12))

    def test_live_function_code_and_defaults(self):
        def original(cls, value=1):
            return "original", cls, value

        def replacement(cls, value=2):
            return "replacement", cls, value

        Receiver = type("Receiver", (), {"selected": classmethod(original)})

        def capture(cls, argument):
            return cls.selected(argument())

        reset_code(capture)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            capture(Receiver, lambda: 7)
        self.assertIn("LOAD_ATTR_CLASSMETHOD", handlers(capture))

        def mutate():
            original.__code__ = replacement.__code__
            original.__defaults__ = (13,)
            return 14

        self.assertEqual(capture(Receiver, mutate), ("replacement", Receiver, 14))
        self.assertEqual(Receiver.selected(), ("replacement", Receiver, 13))

    def test_general_call_current_defaults_and_binding_error(self):
        class Receiver:
            @classmethod
            def selected(cls, value=1):
                return cls, value

        def invoke(cls):
            return cls.selected()

        reset_code(invoke)
        warm(invoke, Receiver)
        self.assertIn("LOAD_ATTR_CLASSMETHOD", handlers(invoke))
        self.assertIn("CALL_PY_GENERAL", handlers(invoke))
        function = Receiver.__dict__["selected"].__func__
        function.__defaults__ = (20,)
        self.assertEqual(invoke(Receiver), (Receiver, 20))
        function.__defaults__ = ()
        with self.assertRaises(TypeError):
            invoke(Receiver)

    def test_polymorphic_rows_keep_live_descriptors(self):
        def original(cls, value):
            return cls, value

        def replacement(cls, value):
            return "replacement", cls, value

        classes = [type(f"Receiver{i}", (), {"selected": classmethod(original)})
                   for i in range(3)]
        invoke = make_invoke()
        self.warm_invoke(invoke, classes[0])
        for cls in classes[1:]:
            for _ in range(_testinternalcapi.SPECIALIZATION_COOLDOWN):
                self.assertEqual(invoke(cls, 7), (cls, 7))
        [site] = [i for i in dis.get_instructions(invoke) if i.opname == "LOAD_ATTR"]
        kind, state, _, _, rows = _testinternalcapi.get_feedback(
            invoke.__code__, site.offset)
        self.assertEqual((kind, state, len(rows)), ("ATTR", "POLY", 3))
        self.assertEqual({row[2] for row in rows}, {111})
        for cls in classes:
            cls.__dict__["selected"].__init__(replacement)
            self.assertEqual(invoke(cls, 21), ("replacement", cls, 21))

    def test_capture_owns_function_and_class(self):
        def original(cls, value):
            return cls, value

        def replacement(cls, value):
            return "replacement"

        descriptor = classmethod(original)
        Receiver = type("Receiver", (), {"selected": descriptor})
        source = [Receiver]

        def capture(source, argument):
            return source.pop().selected(argument())

        reset_code(capture)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            source.append(Receiver)
            capture(source, lambda: 7)
        self.assertIn("LOAD_ATTR_CLASSMETHOD", handlers(capture))
        function_ref, class_ref = weakref.ref(original), weakref.ref(Receiver)
        del original, Receiver

        def mutate():
            descriptor.__init__(replacement)
            gc_collect()
            self.assertIsNotNone(function_ref())
            self.assertIsNotNone(class_ref())
            return 15

        result = capture(source, mutate)
        self.assertEqual(result, (class_ref(), 15))
        gc_collect()
        self.assertIsNone(function_ref())

    def test_wrapped_callable_and_unsupported_receivers(self):
        class Callable:
            def __call__(self, cls, value):
                return cls, value

        class Receiver:
            @classmethod
            def selected(cls, value):
                return cls, value

        invoke = make_invoke()
        self.warm_invoke(invoke, Receiver)
        Receiver.__dict__["selected"].__init__(Callable())
        self.assertEqual(invoke(Receiver, 16), (Receiver, 16))

        class Meta(type):
            pass

        class DerivedMethod(classmethod):
            pass

        for cls in (Meta("CustomMeta", (), {"selected": classmethod(
                lambda cls, value: (cls, value))}),
                    type("CustomDescriptor", (), {"selected": DerivedMethod(
                lambda cls, value: (cls, value))})):
            invoke = make_invoke()
            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
                self.assertEqual(invoke(cls, 17), (cls, 17))
            self.assertNotIn("LOAD_ATTR_CLASSMETHOD", handlers(invoke))
        owner = Receiver()
        owner.selected = lambda value: value + 1
        self.assertEqual(make_invoke()(owner, 17), 18)

    def test_error_after_capture_and_profile_observation(self):
        class Receiver:
            @classmethod
            def selected(cls, value):
                return cls, value

        def capture(cls, argument):
            return cls.selected(argument())

        reset_code(capture)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            capture(Receiver, lambda: 7)
        self.assertIn("LOAD_ATTR_CLASSMETHOD", handlers(capture))
        marker = ValueError("argument")

        def fail():
            raise marker

        with self.assertRaises(ValueError) as raised:
            capture(Receiver, fail)
        self.assertIs(raised.exception, marker)
        calls = []
        target_code = Receiver.__dict__["selected"].__func__.__code__

        def profile(frame, event, arg):
            if event == "call" and frame.f_code is target_code:
                calls.append(frame.f_locals["cls"])

        previous = sys.getprofile()
        sys.setprofile(profile)
        try:
            self.assertEqual(capture(Receiver, lambda: 19), (Receiver, 19))
        finally:
            sys.setprofile(previous)
        self.assertEqual(calls, [Receiver])

    def test_monitoring_after_specialization(self):
        class Receiver:
            @classmethod
            def selected(cls, value):
                return value

        invoke = make_invoke()
        self.warm_invoke(invoke, Receiver)
        mon = sys.monitoring
        tool = mon.DEBUGGER_ID
        mon.use_tool_id(tool, "classmethod capture")
        seen = set()

        def record(event):
            def callback(code, *args):
                seen.add(event)
            return callback

        events = (mon.events.LINE, mon.events.INSTRUCTION, mon.events.CALL)
        try:
            for event in events:
                mon.register_callback(tool, event, record(event))
            mon.set_local_events(tool, invoke.__code__, sum(events))
            self.assertEqual(invoke(Receiver, 22), 22)
            self.assertEqual(seen, set(events))
            mon.set_local_events(tool, invoke.__code__, 0)
        finally:
            mon.set_local_events(tool, invoke.__code__, 0)
            for event in events:
                mon.register_callback(tool, event, None)
            mon.free_tool_id(tool)
        for _ in range(_testinternalcapi.SPECIALIZATION_COOLDOWN):
            invoke(Receiver, 7)
        self.assert_cached(invoke)

    def test_adaptive_name_number_and_original_words(self):
        class Receiver:
            @classmethod
            def selected(cls, value):
                return value

        invoke = make_invoke()
        original = invoke.__code__.co_code
        self.warm_invoke(invoke, Receiver)
        self.assertEqual(dis._specialized_opmap["LOAD_ATTR_CLASSMETHOD"], 111)
        self.assertEqual(invoke.__code__.co_code, original)
        self.assertIn("LOAD_ATTR_CLASSMETHOD", {
            i.opname for i in dis.get_instructions(invoke, adaptive=True)})
        self.assertEqual(importlib.util.MAGIC_NUMBER,
                         (64422).to_bytes(2, "little") + b"\r\n")


if __name__ == "__main__":
    unittest.main()
