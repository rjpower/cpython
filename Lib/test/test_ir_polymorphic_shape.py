"""Learned shape cases share the guarded canonical attribute-read action."""

import dis
import gc
import marshal
import sys
import types
import unittest
import weakref

from test import support
from test.support import import_helper
from test.test_ir_admission import construct
from test.test_ir_transfer_admission import encoded, make_code

_testinternalcapi = import_helper.import_module("_testinternalcapi")
POLY = {"LOAD_ATTR_SHAPE_POLY", "LOAD_ATTR_SHAPE_POLY_LOCAL_TEMP"}
ROUNDS = _testinternalcapi.SPECIALIZATION_COOLDOWN * 4


def routes(code):
    return (code, construct(code), code.replace(), marshal.loads(marshal.dumps(code)))


def handlers(code):
    return {word[0] for word in _testinternalcapi.get_ir(code, True)}


def instances(count):
    result = []
    for index in range(count):
        cls = type(f"ShapeCase{index}", (), {})
        owner = cls()
        for padding in range(index):
            setattr(owner, f"padding{padding}", None)
        owner.value = object()
        result.append(owner)
    return result


class LearnedAdmissionTests(unittest.TestCase):
    def test_public_images_cannot_supply_learned_cases(self):
        code = make_code([("LOAD_CONST", 0), ("RETURN_VALUE", 0)])
        for name in sorted(POLY):
            changed = encoded([(name, 0), ("RETURN_VALUE", 0)])
            for route in (construct, lambda co, **kw: co.replace(**kw)):
                with self.subTest(handler=name, route=route):
                    with self.assertRaisesRegex(ValueError, "learned-only executable handler"):
                        route(code, co_code=changed)
            payload = marshal.dumps(code)
            self.assertEqual(payload.count(code.co_code), 1)
            with self.assertRaisesRegex(ValueError, "learned-only executable handler"):
                marshal.loads(payload.replace(code.co_code, changed, 1))


@unittest.skipUnless(support.Py_SHAPES and not support.Py_GIL_DISABLED,
                     "requires GIL shape specialization")
class PolymorphicShapeTests(unittest.TestCase):
    def train(self, read, owners):
        for owner in owners:
            for _ in range(ROUNDS):
                self.assertIs(read(owner), owner.value)
        self.assertTrue(handlers(read.__code__) & POLY)

    def test_distinct_types_offsets_and_four_case_bound(self):
        def template(owner):
            return owner.value

        owners = instances(5)
        for code in routes(template.__code__):
            with self.subTest(route=code):
                read = types.FunctionType(code, globals())
                self.train(read, owners[:4])
                self.assertIn("LOAD_ATTR_SHAPE_POLY_LOCAL_TEMP", handlers(code))
                for _ in range(8):
                    for owner in owners[:4]:
                        self.assertIs(read(owner), owner.value)
                self.assertIn("LOAD_ATTR_SHAPE_POLY_LOCAL_TEMP", handlers(code))
                for _ in range(ROUNDS):
                    self.assertIs(read(owners[4]), owners[4].value)
                self.assertFalse(handlers(code) & POLY)

    def test_descriptor_miss_executes_generic_lookup_once(self):
        def template(owner):
            return owner.value

        for code in routes(template.__code__):
            owners = instances(2)
            read = types.FunctionType(code, globals())
            self.train(read, owners)
            calls = []
            marker = object()

            def getter(owner):
                calls.append(owner)
                gc.collect()
                return marker

            type(owners[0]).value = property(getter)
            self.assertIs(read(owners[0]), marker)
            self.assertEqual(calls, [owners[0]])
            self.assertIs(read(owners[1]), owners[1].value)

    def test_deleted_field_and_materialized_dictionary(self):
        def template(owner):
            return owner.value

        for code in routes(template.__code__):
            owners = instances(2)
            read = types.FunctionType(code, globals())
            self.train(read, owners)
            del owners[0].value
            with self.assertRaises(AttributeError):
                read(owners[0])
            replacement = object()
            owners[0].__dict__["value"] = replacement
            self.assertIs(read(owners[0]), replacement)
            self.assertIs(read(owners[1]), owners[1].value)

    def test_pre_observer_rechecks_current_type_before_field_read(self):
        def template(owner):
            return owner.value

        owners = instances(2)
        read = types.FunctionType(template.__code__.replace(), globals())
        self.train(read, owners)
        offset = next(instruction.offset for instruction in dis.get_instructions(read)
                      if instruction.opname == "LOAD_ATTR")
        marker = object()
        calls = []

        def getter(owner):
            calls.append(owner)
            return marker

        monitor = sys.monitoring
        tool = monitor.DEBUGGER_ID

        def before(code, word):
            if code is read.__code__ and word == offset:
                monitor.set_local_events(tool, code, 0)
                type(owners[0]).value = property(getter)
                gc.collect()

        monitor.use_tool_id(tool, "polymorphic-shape")
        try:
            monitor.register_callback(tool, monitor.events.INSTRUCTION, before)
            monitor.set_local_events(tool, read.__code__, monitor.events.INSTRUCTION)
            self.assertIs(read(owners[0]), marker)
            self.assertEqual(calls, [owners[0]])
        finally:
            monitor.set_local_events(tool, read.__code__, 0)
            monitor.register_callback(tool, monitor.events.INSTRUCTION, None)
            monitor.free_tool_id(tool)

    def test_temporary_receiver_retirement_keeps_result(self):
        events = []

        class Payload:
            pass

        class First:
            def __del__(self):
                events.append("first")
                gc.collect()

        class Second:
            def __del__(self):
                events.append("second")
                gc.collect()

        def factory(cls):
            owner = cls()
            owner.value = Payload()
            return owner

        def read(factory):
            return factory().value

        for cls in (First, Second):
            for _ in range(ROUNDS):
                read(lambda: factory(cls))
        self.assertIn("LOAD_ATTR_SHAPE_POLY", handlers(read.__code__))
        events.clear()
        result = read(lambda: factory(First))
        reference = weakref.ref(result)
        self.assertEqual(events, ["first"])
        self.assertIs(reference(), result)
        self.assertEqual(_testinternalcapi.gc_verify(), [])
        del result
        gc.collect()
        self.assertIsNone(reference())


if __name__ == "__main__":
    unittest.main()
