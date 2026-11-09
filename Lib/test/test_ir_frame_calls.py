"""Direct Python binding and materialization of virtual empty kwargs."""

import sys
import types
import unittest
from test import support
from test.support import import_helper

_testinternalcapi = import_helper.import_module("_testinternalcapi")
pending = _testinternalcapi.frame_has_virtual_kwargs
ROUNDS = _testinternalcapi.SPECIALIZATION_THRESHOLD + 50
VIRTUAL_KWARGS = 0x10000000


def positional_caller(target):
    def call():
        return target(1)
    call.__code__ = call.__code__.replace()
    return call


def warm(call):
    for _ in range(ROUNDS):
        result = call()
    return result


@unittest.skipIf(support.Py_GIL_DISABLED, "direct binding requires the GIL")
class FrameCallTests(unittest.TestCase):
    def test_discarded_slot_attribute_and_method_window(self):
        events = []

        class Reader:
            __slots__ = ("value",)

            def method(self):
                events.append("method")

        def read(reader):
            reader.value
            reader.method()

        rows = _testinternalcapi.get_ir(read.__code__)
        attributes = [row for row in rows if row[0] == "LOAD_ATTR"]
        self.assertEqual(len(attributes), 2)
        self.assertEqual(attributes[0][1], 4095)
        self.assertNotEqual(attributes[1][1], 4095)
        reader = Reader()
        reader.value = object()
        for _ in range(ROUNDS):
            read(reader)
        self.assertEqual(events, ["method"] * ROUNDS)
        self.assertTrue(any(row[0] == "LOAD_ATTR_SLOT" and row[1] == 4095
                            for row in _testinternalcapi.get_ir(read.__code__, True)))

    def test_discarded_custom_reader_results(self):
        events = []

        class Reader:
            def __getattr__(self, name):
                events.append(("attr", name))
                return object()

            def __getitem__(self, key):
                events.append(("item", key))
                return object()

        def read(reader):
            reader.value
            reader[3]

        rows = _testinternalcapi.get_ir(read.__code__)
        for opcode in ("LOAD_ATTR", "BINARY_OP_SUBSCR"):
            self.assertTrue(any(row[0] == opcode and row[1] == 4095
                                for row in rows), rows)
        reader = Reader()
        for _ in range(ROUNDS):
            read(reader)
        self.assertEqual(events, [("attr", "value"), ("item", 3)] * ROUNDS)

    def test_unused_empty_binding_executes_virtual_route(self):
        def target(value, *args, **kwargs):
            return pending()

        call = positional_caller(target)
        self.assertTrue(warm(call))
        self.assertIn("CALL_PY_GENERAL", [w[0] for w in
            _testinternalcapi.get_ir(call.__code__, True)])

    def test_first_read_materializes_fresh_identity(self):
        def target(value, *args, **kwargs):
            before = pending()
            result = kwargs
            return before, pending(), result

        call = positional_caller(target)
        self.assertEqual(warm(call)[:2], (True, False))
        first = call()[2]
        second = call()[2]
        self.assertIsNot(first, second)
        first["changed"] = True
        self.assertEqual(second, {})
        self.assertIn("MATERIALIZE_KWARGS", [w[0] for w in
            _testinternalcapi.get_ir(target.__code__)])

    def test_folded_window_read(self):
        def consume(mapping):
            return mapping

        def target(value, *args, **kwargs):
            before = pending()
            result = consume(kwargs)
            return before, pending(), result

        call = positional_caller(target)
        self.assertEqual(warm(call), (True, False, {}))

    def test_frame_publication_materializes(self):
        def target(value, *args, **kwargs):
            before = pending()
            frame = sys._getframe()
            return before, pending(), frame.f_locals["kwargs"]

        self.assertEqual(warm(positional_caller(target)), (True, False, {}))

    def test_child_observes_ancestor_binding(self):
        def inspect_parent():
            return sys._getframe(1).f_locals["kwargs"]

        def target(value, *args, **kwargs):
            before = pending()
            observed = inspect_parent()
            return before, pending(), observed

        self.assertEqual(warm(positional_caller(target)), (True, False, {}))

    def test_profile_frame_observation_uses_eager_values(self):
        def target(value, *args, **kwargs):
            return pending()

        call = positional_caller(target)
        self.assertTrue(warm(call))
        seen = []

        def observe(frame, event, arg):
            if event == "call" and frame.f_code is target.__code__:
                seen.append(frame.f_locals["kwargs"])

        previous = sys.getprofile()
        try:
            sys.setprofile(observe)
            self.assertFalse(call())
        finally:
            sys.setprofile(previous)
        self.assertEqual(seen, [{}])

    def test_rebind_delete_capture_and_kwonly_decline(self):
        def rebind(value, *args, **kwargs):
            before = pending()
            kwargs = None
            return before

        def delete(value, *args, **kwargs):
            before = pending()
            del kwargs
            return before

        def capture(value, *args, **kwargs):
            def nested():
                return kwargs
            return pending()

        def kwonly(value, *args, optional=None, **kwargs):
            return pending()

        for target in (rebind, delete, capture, kwonly):
            with self.subTest(target=target.__name__):
                self.assertFalse(warm(positional_caller(target)))

    def test_nonempty_variadics_use_eager_binding(self):
        def target(value, *args, **kwargs):
            return pending(), args, kwargs

        def call_args():
            return target(1, 2)

        def call_kwargs():
            return target(1, key=2)

        self.assertEqual(warm(call_args), (False, (2,), {}))
        self.assertEqual(warm(call_kwargs), (False, (), {"key": 2}))

    def test_defaults_mutation_and_overlong_tuple(self):
        def target(value, extra=2, *args, **kwargs):
            return pending(), extra

        call = positional_caller(target)
        self.assertEqual(warm(call), (True, 2))
        target.__defaults__ = (3,)
        self.assertEqual(call()[1], 3)
        # Legal defaults can exceed both the arity and a uint16 plan field.
        target.__defaults__ = tuple(range(70000))
        self.assertEqual(warm(call)[1], 69999)

    def test_structural_code_replace_cancels_eligibility(self):
        def target(value=0, *args, **kwargs):
            return pending()

        self.assertTrue(target.__code__.co_flags & VIRTUAL_KWARGS)
        replacement = target.__code__.replace(co_argcount=0)
        self.assertFalse(replacement.co_flags & VIRTUAL_KWARGS)
        replaced = types.FunctionType(replacement, globals())
        self.assertFalse(warm(positional_caller(replaced)))

    def test_error_after_materialization_preserves_traceback_locals(self):
        def target(value, *args, **kwargs):
            kwargs["saved"] = value
            raise RuntimeError("after materialization")

        call = positional_caller(target)
        for _ in range(ROUNDS):
            try:
                call()
            except RuntimeError as exc:
                tb = exc.__traceback__
                while tb.tb_next is not None:
                    tb = tb.tb_next
                self.assertEqual(tb.tb_frame.f_locals["kwargs"], {"saved": 1})


if __name__ == "__main__":
    unittest.main()
