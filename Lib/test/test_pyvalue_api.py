"""Value API packet V1 (pyjit/design/value-api.md): the entry primitives of
section 1.3 and the value format conversions of section 4."""

import contextvars
import math
import sys
import types
import unittest
import weakref
from test import support
from test.support import import_helper
from test.support.script_helper import assert_python_failure

_testinternalcapi = import_helper.import_module("_testinternalcapi")


class Number(int):
    pass


class Item:
    def __repr__(self):
        return "<item é>"


class BadRepr:
    def __repr__(self):
        raise ZeroDivisionError("repr")


def qualified_name(tp, sep="."):
    if tp.__module__ == "builtins":
        return tp.__qualname__
    return f"{tp.__module__}{sep}{tp.__qualname__}"


VALUES = [5, -7, 2**31 - 1, -2**31, 2**31, 2**40, 2.5, -0.0, math.inf,
          float("nan"), True, Number(3), "é", None, [1, "a"], Item()]


class BorrowedEntryTests(unittest.TestCase):
    def check(self, obj, kind):
        got_kind, back, same_bits = (
            _testinternalcapi.pyvalue_from_object_borrow_inline(obj))
        self.assertEqual(got_kind, kind)
        self.assertTrue(same_bits)
        if kind == "object":
            self.assertIs(back, obj)
        else:
            self.assertIs(type(back), type(obj))
            self.assertEqual(math.copysign(1, back), math.copysign(1, obj))
            self.assertEqual(back, obj)

    def test_inline_ints(self):
        for obj in (0, 5, -7, 2**31 - 1, -2**31, 2**50 - 1, -2**50):
            with self.subTest(obj=obj):
                self.check(obj, "int")

    def test_inline_floats(self):
        for obj in (2.5, -0.0, 0.0, math.inf, -math.inf):
            with self.subTest(obj=obj):
                self.check(obj, "float")

    def test_objects(self):
        for obj in (2**50, -2**50 - 1, 2**100, float("nan"), True, False,
                    Number(3), "é", None, [1], int, Item()):
            with self.subTest(obj=obj):
                self.check(obj, "object")

    def test_null(self):
        _testinternalcapi.pyvalue_entry_null()


class KnownEntryTests(unittest.TestCase):
    def test_round_trip(self):
        for obj in ("é", int, [1], {}, None, Item(), len, iter([]), True,
                    Number(4)):
            with self.subTest(obj=obj):
                self.assertIs(
                    _testinternalcapi.pyvalue_from_object_known(obj), obj)

    @unittest.skipUnless(support.Py_DEBUG, "the check is in debug builds")
    @support.requires_subprocess()
    def test_debug_check(self):
        for obj in ("5", "2.5", "2**40"):
            with self.subTest(obj=obj):
                code = ("import _testinternalcapi; "
                        f"_testinternalcapi.pyvalue_from_object_known({obj})")
                _, _, err = assert_python_failure("-c", code)
                self.assertIn(b"_PyValue_FromObjectKnown() needs an object",
                              err)


class NoReentryCloseTests(unittest.TestCase):
    def test_nonlast_owner(self):
        for obj in ([], {}, Item(), Number(7), 2**100, float("nan")):
            with self.subTest(type=type(obj)):
                closed, before, after = _testinternalcapi.pyvalue_try_xclose(obj)
                self.assertEqual(closed, not support.Py_GIL_DISABLED)
                self.assertEqual(before - after, closed)

    def test_inline_and_no_count(self):
        for obj in (5, -7, 2.5, -0.0, math.inf, None, True, int):
            with self.subTest(obj=obj):
                closed, before, after = _testinternalcapi.pyvalue_try_xclose(obj)
                self.assertTrue(closed)
                self.assertEqual(before, after)

    def test_last_owner_is_retained(self):
        finalized = []

        class Payload:
            def __del__(self):
                finalized.append("closed")

        _testinternalcapi.pyvalue_try_xclose_last(Payload)
        self.assertEqual(finalized, ["closed"])

    def test_success_does_not_finalize(self):
        finalized = []

        class Payload:
            def __del__(self):
                finalized.append("closed")

        obj = Payload()
        closed, before, after = _testinternalcapi.pyvalue_try_xclose(obj)
        self.assertEqual(closed, not support.Py_GIL_DISABLED)
        self.assertEqual(before - after, closed)
        self.assertEqual(finalized, [])
        del obj
        self.assertEqual(finalized, ["closed"])

    def test_internal_encodings_and_dynamic_immortal(self):
        _testinternalcapi.pyvalue_try_xclose_internal()


class FrameCodePinTests(unittest.TestCase):
    def test_code_pin_count(self):
        code = self.test_code_pin_count.__code__.replace()
        self.assertFalse(sys._is_immortal(code))
        _testinternalcapi.check_frame_code_pin(code)

    def test_active_code_survives_replacement(self):
        def original(function, replacement, reference):
            function.__code__ = replacement
            return reference() is not None

        def replacement(function, replacement, reference):
            return False

        code = original.__code__.replace()
        function = types.FunctionType(code, globals())
        reference = weakref.ref(code)
        del code
        self.assertTrue(function(function, replacement.__code__, reference))
        self.assertIsNone(reference())

    def test_suspended_generator_keeps_code(self):
        def original():
            yield 1

        def replacement():
            yield 2

        code = original.__code__.replace()
        function = types.FunctionType(code, globals())
        reference = weakref.ref(code)
        generator = function()
        function.__code__ = replacement.__code__
        del code
        self.assertIsNotNone(reference())
        self.assertEqual(next(generator), 1)
        generator.close()
        del generator
        self.assertIsNone(reference())

    def test_escaped_frame_keeps_independent_pins(self):
        def body(callback):
            callback()
            return sys._getframe()

        code = body.__code__.replace()
        function = types.FunctionType(code, {'sys': sys})
        code_ref = weakref.ref(code)
        function_ref = weakref.ref(function)
        functions = [function]
        del code, function

        def check_active():
            support.gc_collect()
            self.assertIsNotNone(function_ref())
            self.assertIsNotNone(code_ref())

        frame = functions.pop()(check_active)
        support.gc_collect()
        self.assertIsNotNone(function_ref())
        self.assertIs(frame.f_code, code_ref())
        frame.clear()
        support.gc_collect()
        self.assertIsNotNone(function_ref())
        self.assertIs(frame.f_code, code_ref())
        del frame
        support.gc_collect()
        self.assertIsNone(function_ref())
        self.assertIsNone(code_ref())

    def test_completed_generator_keeps_code_without_function(self):
        def body():
            yield 1

        code = body.__code__.replace()
        function = types.FunctionType(code, {})
        code_ref = weakref.ref(code)
        function_ref = weakref.ref(function)
        generator = function()
        del code, function
        self.assertEqual(list(generator), [1])
        support.gc_collect()
        self.assertIsNone(function_ref())
        self.assertIs(generator.gi_code, code_ref())
        del generator
        support.gc_collect()
        self.assertIsNone(code_ref())

    def test_context_and_init_trampoline(self):
        class Item:
            def __init__(self, fail=False):
                if fail:
                    raise ValueError("init")

        context = contextvars.copy_context()
        for _ in range(100):
            self.assertIsInstance(Item(), Item)
            self.assertIsInstance(context.run(Item), Item)
        with self.assertRaisesRegex(ValueError, "init"):
            context.run(Item, True)


class TransitionRootTests(unittest.TestCase):
    def test_nested_retirement_roots_and_arena_growth(self):
        references = {}
        observations = []
        payload = Item()
        namespace = {}
        assignments = ''.join(f'    local_{i} = depth\n' for i in range(128))
        exec('def grow(depth):\n' + assignments +
             '    if depth:\n        grow(depth - 1)\n', namespace)

        class Result:
            def __init__(self, name):
                references[name] = weakref.ref(self)

        class Trigger:
            def __init__(self, name):
                self.name = name

            def __del__(self):
                name = self.name
                ids = set(_testinternalcapi.get_transition_root_ids())
                observations.append((name, id(payload) in ids,
                                     id(references[name]()) in ids,
                                     name != 'inner' or
                                     id(references['outer']()) in ids))
                namespace['grow'](24)
                support.gc_collect()
                if name == 'outer':
                    nested = retire('inner', payload)
                    observations.append(('nested-result',
                                         isinstance(nested, Result)))

        def retire(name, remaining):
            trigger = Trigger(name)
            return Result(name)

        result = retire('outer', payload)
        self.assertIs(result, references['outer']())
        self.assertEqual(observations, [('outer', True, True, True),
                                        ('inner', True, True, True),
                                        ('nested-result', True)])
        self.assertEqual(_testinternalcapi.get_transition_root_ids(), [])

    def test_result_published_after_finalizer_mutates_caller(self):
        caller = sys._getframe()
        observations = []
        references = []
        target = 'before'

        class Result:
            def __init__(self):
                references.append(weakref.ref(self))

        class Trigger:
            def __del__(self):
                ids = set(_testinternalcapi.get_transition_root_ids())
                observations.append((caller.f_locals['target'],
                                     id(references[-1]()) in ids))
                caller.f_locals['target'] = 'during'
                support.gc_collect()

        def retire():
            trigger = Trigger()
            return Result()

        target = retire()
        self.assertIs(target, references[-1]())
        self.assertEqual(observations, [('before', True)])
        self.assertEqual(_testinternalcapi.get_transition_root_ids(), [])

    def test_final_code_close_keeps_result_root(self):
        observations = []
        references = []

        class Result:
            def __init__(self):
                references.append(weakref.ref(self))

        class Constant:
            def __del__(self):
                ids = set(_testinternalcapi.get_transition_root_ids())
                observations.append(id(references[-1]()) in ids)
                support.gc_collect()

        def body():
            return Result()

        constant = Constant()
        code = body.__code__.replace(co_consts=body.__code__.co_consts +
                                     (constant,))
        function = types.FunctionType(code, {'Result': Result},
                                      closure=body.__closure__)
        functions = [function]
        del code, constant, function
        result = functions.pop()()
        self.assertIs(result, references[-1]())
        self.assertEqual(observations, [True])
        self.assertEqual(_testinternalcapi.get_transition_root_ids(), [])

    def test_frame_association_close_keeps_result_root(self):
        observations = []
        references = []

        class Result:
            def __init__(self):
                references.append(weakref.ref(self))

        class Trace:
            def __call__(self, *args):
                return self

            def __del__(self):
                ids = set(_testinternalcapi.get_transition_root_ids())
                observations.append(id(references[-1]()) in ids)
                support.gc_collect()

        def retire():
            sys._getframe().f_trace = Trace()
            return Result()

        result = retire()
        self.assertIs(result, references[-1]())
        self.assertEqual(observations, [True])
        self.assertEqual(_testinternalcapi.get_transition_root_ids(), [])

    def test_unstarted_generator_deallocation_roots(self):
        observations = []
        payload = Item()

        class Trigger:
            def __del__(self):
                ids = set(_testinternalcapi.get_transition_root_ids())
                observations.append((id(payload) in ids,
                                     id(retire.__code__) in ids))
                support.gc_collect()

        def retire(remaining, trigger):
            yield

        generator = retire(payload, Trigger())
        del generator
        self.assertEqual(observations, [(True, True)])
        self.assertEqual(_testinternalcapi.get_transition_root_ids(), [])

    def test_generator_completion_keeps_result_root(self):
        observations = []
        references = []
        payload = Item()

        class Result:
            def __init__(self):
                references.append(weakref.ref(self))

        class Trigger:
            def __del__(self):
                ids = set(_testinternalcapi.get_transition_root_ids())
                observations.append((id(payload) in ids,
                                     id(references[-1]()) in ids))
                support.gc_collect()

        def retire(remaining):
            trigger = Trigger()
            if False:
                yield
            return Result()

        generator = retire(payload)
        with self.assertRaises(StopIteration) as stopped:
            next(generator)
        self.assertIs(stopped.exception.value, references[-1]())
        self.assertEqual(observations, [(True, True)])
        self.assertEqual(_testinternalcapi.get_transition_root_ids(), [])

    def test_generator_deallocation_keeps_exception_root(self):
        exception_ids = []
        observations = []
        unraisable = []

        class Trigger:
            def __del__(self):
                ids = set(_testinternalcapi.get_transition_root_ids())
                observations.append(exception_ids[-1] in ids)
                support.gc_collect()

        def retire(trigger):
            try:
                yield
            except GeneratorExit as error:
                exception_ids.append(id(error))
                # Keep this on direct generator retirement: a traceback
                # would let the escaped frame own Trigger until the exception
                # itself is already being destroyed.
                error.__traceback__ = None
                yield

        generator = retire(Trigger())
        next(generator)
        # Keep only the message: retaining the unraisable's object would
        # resurrect the generator and defer the direct deallocation path.
        def hook(error):
            unraisable.append(str(error.exc_value))

        with support.swap_attr(sys, 'unraisablehook', hook):
            del generator
            support.gc_collect()
        self.assertEqual(unraisable, ['generator ignored GeneratorExit'])
        self.assertEqual(observations, [True])
        self.assertEqual(_testinternalcapi.get_transition_root_ids(), [])

    def test_exception_retirement_collects_without_leaking_roots(self):
        observations = []

        class Trigger:
            def __del__(self):
                support.gc_collect()
                observations.append('finalized')

        def retire():
            trigger = Trigger()
            raise ValueError('retire')

        with self.assertRaisesRegex(ValueError, 'retire'):
            retire()
        support.gc_collect()
        self.assertEqual(observations, ['finalized'])
        self.assertEqual(_testinternalcapi.get_transition_root_ids(), [])


class ValueFormatTests(unittest.TestCase):
    def format(self, fmt, obj, *other):
        return _testinternalcapi.pyvalue_format(fmt, obj, *other)

    def test_conversions(self):
        for obj in VALUES:
            with self.subTest(obj=obj):
                self.assertEqual(self.format("%vR", obj), repr(obj))
                self.assertEqual(self.format("%vS", obj), str(obj))
                self.assertEqual(self.format("%vA", obj), ascii(obj))
                self.assertEqual(self.format("%vT", obj),
                                 qualified_name(type(obj)))
                self.assertEqual(self.format("%#vT", obj),
                                 qualified_name(type(obj), ":"))

    def test_type_names(self):
        for tp in (int, float, Number, Item):
            with self.subTest(tp=tp):
                self.assertEqual(self.format("%vN", tp), qualified_name(tp))
                self.assertEqual(self.format("%#vN", tp),
                                 qualified_name(tp, ":"))
        for obj in (5, 2.5, 2**40, "int"):
            with self.subTest(obj=obj):
                with self.assertRaisesRegex(TypeError,
                                            "%N argument must be a type"):
                    self.format("%vN", obj)

    def test_width_and_precision(self):
        self.assertEqual(self.format("%6vR|", 1000), "  1000|")
        self.assertEqual(self.format("%-6vS|", 2.5), "2.5   |")
        self.assertEqual(self.format("%.2vS", 12345), "12")
        self.assertEqual(self.format("%.3vA", "é"), "'\\x")
        self.assertEqual(self.format("%5.1vT|", 2.5), "    f|")

    def test_mixed_with_object_conversions(self):
        self.assertEqual(self.format("%vR and %R", 1000, [2.5]),
                         "1000 and [2.5]")
        self.assertEqual(self.format("%vS%%%S", -0.0, 7), "-0.0%7")

    def test_error_in_repr(self):
        with self.assertRaisesRegex(ZeroDivisionError, "repr"):
            self.format("%vR", BadRepr())

    def test_inline_format_without_object_projection(self):
        self.assertEqual(_testinternalcapi.pyvalue_format_inline(),
                         "1000|-0.0|1000")

    def test_repr_can_replace_the_borrowed_field(self):
        items = []

        class ReplacesItem:
            def __repr__(self):
                items.clear()
                return "replaced"

        items.append(ReplacesItem())
        self.assertEqual(_testinternalcapi.pyvalue_format_list_item(items),
                         "replaced")
        self.assertEqual(items, [])

    def test_invalid(self):
        for fmt in ("%vd", "%vs", "%vU", "%vc", "%lvR", "%vlR", "%vV"):
            with self.subTest(fmt=fmt):
                with self.assertRaisesRegex(SystemError,
                                            "invalid format string"):
                    self.format(fmt, 5)

    def test_err_format(self):
        with self.assertRaisesRegex(ValueError, r"^bad 1000$"):
            _testinternalcapi.pyvalue_err_format(ValueError, "bad %vR", 1000)
        with self.assertRaisesRegex(TypeError, r"^not float$"):
            _testinternalcapi.pyvalue_err_format(TypeError, "not %vT", 2.5)


class KnownObjectCheckTests(unittest.TestCase):
    @support.requires_subprocess()
    def test_inline_value_is_fatal(self):
        # The check is in release builds too (value-api.md, section 1.3).
        for steal in (False, True):
            with self.subTest(steal=steal):
                code = ("import _testinternalcapi; _testinternalcapi."
                        f"pyvalue_as_known_object_inline({steal})")
                _, _, err = assert_python_failure("-c", code)
                self.assertIn(b"_PyValue_FatalNotObject: PyValue_AsKnown"
                              b"Object*() of a value that holds no object",
                              err)


class ObjectExitTests(unittest.TestCase):
    @unittest.skipUnless(support.Py_DEBUG, "the check is in debug builds")
    @support.requires_subprocess()
    def test_internal_value_is_fatal(self):
        code = "import _testinternalcapi; _testinternalcapi.pyvalue_as_object_index()"
        _, _, err = assert_python_failure("-c", code)
        self.assertIn(b"PyValue_IsLanguageValue", err)


if __name__ == "__main__":
    unittest.main()
