"""Direct Python binding and materialization of virtual empty kwargs."""

import gc
import itertools
import sys
import types
import unittest
import weakref
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


def keyword_caller(target, arguments):
    namespace = {"target": target}
    exec(f"def call():\n    return target({arguments})", namespace)
    return namespace["call"]


def warm(call):
    for _ in range(ROUNDS):
        result = call()
    return result


@unittest.skipIf(support.Py_GIL_DISABLED, "direct binding requires the GIL")
class FrameCallTests(unittest.TestCase):
    def test_keyword_permutations_and_positional_prefix(self):
        def target(a, b, c):
            return a, b, c

        for names in itertools.permutations("abc"):
            arguments = ", ".join(f"{name}={i}" for i, name in enumerate(names))
            call = keyword_caller(target, arguments)
            with self.subTest(names=names):
                expected = tuple(names.index(name) for name in "abc")
                self.assertEqual(warm(call), expected)
                self.assertIn("CALL_KW_NO_SELF_PY", [w[0] for w in
                    _testinternalcapi.get_ir(call.__code__, True)])

        for arguments in ("0, 1, c=2", "0, c=2, b=1"):
            with self.subTest(arguments=arguments):
                self.assertEqual(warm(keyword_caller(target, arguments)), (0, 1, 2))

    def test_keyword_defaults_holes_and_explicit_kwonly(self):
        default = object()

        def target(a, b=default, c=default, d=default, *, e):
            return a, b, c, d, e

        for arguments in ("0, c=2, e=4", "a=0, e=4, c=2"):
            with self.subTest(arguments=arguments):
                result = warm(keyword_caller(target, arguments))
                self.assertEqual((result[0], result[2], result[4]), (0, 2, 4))
                self.assertIs(result[1], default)
                self.assertIs(result[3], default)

        def positional_only(a, /, b=default, *, c):
            return a, b, c

        result = warm(keyword_caller(positional_only, "0, c=2"))
        self.assertEqual((result[0], result[2]), (0, 2))
        self.assertIs(result[1], default)

    def test_keyword_defaults_and_code_mutation(self):
        first = object()
        second = object()

        def target(a, b=first, c=first):
            return a, b, c

        call = keyword_caller(target, "a=0, c=2")
        self.assertIs(warm(call)[1], first)
        target.__defaults__ = (second, second)
        self.assertIs(call()[1], second)
        target.__defaults__ = tuple(range(70000))
        self.assertEqual(warm(call), (0, 69998, 2))
        target.__defaults__ = None
        with self.assertRaisesRegex(TypeError, "missing.*'b'"):
            call()

        def replacement(a, *, c):
            return c, a

        target.__code__ = replacement.__code__
        self.assertEqual(warm(call), (2, 0))
        call.__globals__["target"] = lambda a, c: (a + c,)
        self.assertEqual(warm(call), (2,))

    def test_keyword_kwdefaults_mutation_with_explicit_value(self):
        def target(a, *, b=1):
            return a, b

        call = keyword_caller(target, "b=2, a=0")
        self.assertEqual(warm(call), (0, 2))
        target.__kwdefaults__["b"] = object()
        self.assertEqual(call(), (0, 2))
        target.__kwdefaults__ = None
        self.assertEqual(call(), (0, 2))

    def test_keyword_same_code_different_default_count(self):
        def factory():
            def target(a, b=10, c=20):
                return a, b, c
            return target

        first = factory()
        call = keyword_caller(first, "a=0, c=2")
        self.assertEqual(warm(call), (0, 10, 2))
        constants = tuple(() if value == (10, 20) else value
                          for value in factory.__code__.co_consts)
        factory.__code__ = factory.__code__.replace(co_consts=constants)
        second = factory()
        self.assertIs(first.__code__, second.__code__)
        self.assertEqual(second.__defaults__, ())
        call.__globals__["target"] = second
        with self.assertRaisesRegex(TypeError, "missing.*'b'"):
            call()

    def test_positional_same_code_different_default_count(self):
        def factory():
            def target(a, b=10, c=20):
                return a, b, c
            return target

        first = factory()
        factory_code = factory.__code__
        cases = (
            ((30, 40), (1, 30, 40)),
            ((30, 40, 50), (1, 40, 50)),
            ((30,), None),
            ((), None),
            (tuple(range(70000)), (1, 69998, 69999)),
        )
        for defaults, expected in cases:
            with self.subTest(default_count=len(defaults)):
                target = first

                def call():
                    return target(1)

                call.__code__ = call.__code__.replace()
                self.assertEqual(warm(call), (1, 10, 20))
                self.assertIn("CALL_NO_SELF_PY_GENERAL", [w[0] for w in
                    _testinternalcapi.get_ir(call.__code__, True)])
                constants = tuple(defaults if value == (10, 20) else value
                                  for value in factory_code.co_consts)
                factory.__code__ = factory_code.replace(co_consts=constants)
                target = factory()
                self.assertIs(target.__code__, first.__code__)
                self.assertEqual(target.__defaults__, defaults)
                if expected is None:
                    with self.assertRaisesRegex(TypeError, "missing.*'b'"):
                        call()
                else:
                    self.assertEqual(call(), expected)

    def test_keyword_method_and_generator_caller(self):
        class Target:
            def method(self, a, b=1, *, c):
                return self, a, b, c

        target = Target()

        def call():
            return target.method(0, c=2)

        self.assertEqual(warm(call), (target, 0, 1, 2))
        self.assertEqual(warm(keyword_caller(target.method, "0, c=2")),
                         (target, 0, 1, 2))

        def generator():
            for _ in range(ROUNDS):
                yield target.method(0, c=2)

        self.assertEqual(list(generator()), [(target, 0, 1, 2)] * ROUNDS)

    def test_keyword_profile_observes_complete_child(self):
        def target(a, b=1, *, c):
            return a, b, c

        call = keyword_caller(target, "c=2, a=0")
        self.assertEqual(warm(call), (0, 1, 2))
        seen = []

        def observe(frame, event, arg):
            if event == "call" and frame.f_code is target.__code__:
                seen.append(tuple(frame.f_locals[name] for name in "abc"))

        previous = sys.getprofile()
        try:
            sys.setprofile(observe)
            self.assertEqual(call(), (0, 1, 2))
        finally:
            sys.setprofile(previous)
        self.assertEqual(seen, [(0, 1, 2)])

    def test_keyword_retirement_reenters_after_child_publication(self):
        events = []

        def target(a, b, c):
            return a

        call = keyword_caller(target, "a=0, b=1, c=2")
        self.assertEqual(warm(call), 0)

        class Value:
            def __del__(self):
                events.append(call())

        def retire():
            return target(c=Value(), b=Value(), a=0)

        for _ in range(ROUNDS):
            self.assertEqual(retire(), 0)
        gc.collect()
        self.assertEqual(events, [0] * (2 * ROUNDS))

    def test_keyword_owners_and_observed_locals(self):
        class Value:
            pass

        def target(a, b, c):
            frame = sys._getframe()
            return frame.f_locals, (a, b, c)

        def call(a, b, c):
            return target(c=c, a=a, b=b)

        values = [Value() for _ in range(3)]
        for _ in range(ROUNDS):
            observed, result = call(*values)
        for name, value, returned in zip("abc", values, result):
            self.assertIs(observed[name], value)
            self.assertIs(returned, value)
        refs = [weakref.ref(value) for value in values]
        del observed, result, values, value, returned
        gc.collect()
        self.assertEqual([ref() for ref in refs], [None] * 3)

        def failing(a, b, c):
            raise RuntimeError("keyword child")

        target = failing
        for _ in range(ROUNDS):
            try:
                call(0, 1, 2)
            except RuntimeError as exc:
                tb = exc.__traceback__
                while tb.tb_next is not None:
                    tb = tb.tb_next
                self.assertEqual(tuple(tb.tb_frame.f_locals[n] for n in "abc"),
                                 (0, 1, 2))

    def test_keyword_unsupported_shapes_and_errors(self):
        def target(a, b, c, d):
            return a, b, c, d

        self.assertEqual(warm(keyword_caller(target, "d=3, c=2, b=1, a=0")),
                         (0, 1, 2, 3))

        def variadic(a, *args, **kwargs):
            return a, args, kwargs

        self.assertEqual(warm(keyword_caller(variadic, "0, extra=2")),
                         (0, (), {"extra": 2}))

        def kwonly(a, *, b=1):
            return a, b

        self.assertEqual(warm(keyword_caller(kwonly, "a=0")), (0, 1))

        def positional_only(a, /, b):
            return a, b

        cases = (
            (target, "a=0, b=1, c=2", "missing.*'d'"),
            (target, "0, a=1, b=2, c=3, d=4", "multiple values.*'a'"),
            (target, "a=0, b=1, c=2, extra=3", "unexpected keyword.*'extra'"),
            (target, "0, 1, 2, 3, 4, a=5", "multiple values.*'a'"),
            (positional_only, "a=0, b=1", "positional-only"),
        )
        for function, arguments, message in cases:
            call = keyword_caller(function, arguments)
            with self.subTest(arguments=arguments):
                for _ in range(ROUNDS):
                    with self.assertRaisesRegex(TypeError, message):
                        call()

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
        self.assertIn("CALL_NO_SELF_PY_GENERAL", [w[0] for w in
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
