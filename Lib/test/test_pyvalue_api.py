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


class FrameCodePinTests(unittest.TestCase):
    def test_code_pin_count(self):
        code = self.test_code_pin_count.__code__.replace()
        self.assertFalse(sys._is_immortal(code))
        _testinternalcapi.check_frame_code_pin(code)

    def test_function_owner_transfer_count(self):
        def function():
            pass

        self.assertFalse(sys._is_immortal(function))
        _testinternalcapi.check_frame_function_owner(function)

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


class BinderStagingTests(unittest.TestCase):
    def test_partial_arguments_visible_during_keyword_comparison(self):
        for native_provider in (False, True):
            with self.subTest(native_provider=native_provider):
                observations = []
                payload = Item()
                keyword_value = Item()
                caller = sys._getframe()
                changed = 'before'

                def target(remaining, *, keyword):
                    return remaining, keyword

                class Keyword(str):
                    __hash__ = str.__hash__

                    def __eq__(self, other):
                        ids = set(_testinternalcapi.get_transition_root_ids())
                        observations.append((id(target) in ids,
                                             id(payload) in ids,
                                             id(target.__code__) in ids))
                        caller.f_locals['changed'] = 'during'
                        support.gc_collect()
                        return str.__eq__(self, other)

                arguments = {Keyword('keyword'): keyword_value}
                provider = target.__call__ if native_provider else target
                self.assertEqual(provider(payload, **arguments),
                                 (payload, keyword_value))
                self.assertTrue(observations)
                self.assertTrue(all(all(entry) for entry in observations))
                self.assertEqual(changed, 'during')
                self.assertEqual(_testinternalcapi.get_transition_root_ids(), [])

    def test_prior_keyword_and_defaults(self):
        observations = []
        payload = Item()
        installed = Item()
        extra = Item()
        positional_default = Item()
        keyword_default = Item()

        def target(remaining, default=positional_default, *, keyword,
                   optional=keyword_default, **extras):
            return remaining, default, keyword, optional, extras

        class Keyword(str):
            __hash__ = str.__hash__

            def __eq__(self, other):
                if str.__eq__(self, 'extra'):
                    ids = set(_testinternalcapi.get_transition_root_ids())
                    observations.append((id(payload) in ids,
                                         id(installed) in ids,
                                         id(target) in ids))
                    support.gc_collect()
                return str.__eq__(self, other)

        result = target(payload, **{Keyword('keyword'): installed,
                                    Keyword('extra'): extra})
        self.assertEqual(result[:4], (payload, positional_default, installed,
                                      keyword_default))
        self.assertEqual(len(result[4]), 1)
        self.assertIs(next(iter(result[4].values())), extra)
        self.assertTrue(observations)
        self.assertTrue(all(all(entry) for entry in observations))
        self.assertEqual(_testinternalcapi.get_transition_root_ids(), [])

    def test_nested_binding_keeps_outer_partial_frame(self):
        observations = []
        outer_payload = Item()
        inner_payload = Item()

        def inner(remaining, *, keyword):
            return remaining

        def outer(remaining, *, keyword):
            return remaining

        class InnerKeyword(str):
            __hash__ = str.__hash__

            def __eq__(self, other):
                ids = set(_testinternalcapi.get_transition_root_ids())
                observations.append(all(id(value) in ids for value in
                                        (outer, inner, outer_payload,
                                         inner_payload)))
                support.gc_collect()
                return str.__eq__(self, other)

        class OuterKeyword(str):
            __hash__ = str.__hash__

            def __eq__(self, other):
                result = inner(inner_payload,
                               **{InnerKeyword('keyword'): None})
                observations.append(result is inner_payload)
                return str.__eq__(self, other)

        self.assertIs(outer(outer_payload, **{OuterKeyword('keyword'): None}),
                      outer_payload)
        self.assertTrue(observations)
        self.assertTrue(all(observations))
        self.assertEqual(_testinternalcapi.get_transition_root_ids(), [])

    def test_keyword_failure_retires_partial_frame(self):
        observations = []
        payload = Item()
        installed = Item()

        def target(remaining, *, first, second):
            self.fail('binding should fail before entry')

        class Keyword(str):
            __hash__ = str.__hash__

            def __eq__(self, other):
                if str.__eq__(self, 'second'):
                    ids = set(_testinternalcapi.get_transition_root_ids())
                    observations.append((id(payload) in ids,
                                         id(installed) in ids,
                                         id(target) in ids))
                    support.gc_collect()
                    raise ValueError('keyword equality')
                return str.__eq__(self, other)

        with self.assertRaisesRegex(ValueError, 'keyword equality'):
            target(payload, **{Keyword('first'): installed,
                               Keyword('second'): None})
        self.assertTrue(observations)
        self.assertTrue(all(all(entry) for entry in observations))
        self.assertEqual(_testinternalcapi.get_transition_root_ids(), [])

    def test_code_mutation_keeps_selected_binding_geometry(self):
        observations = []
        payload = Item()

        def target(remaining, *, keyword):
            return remaining, keyword

        def replacement(remaining, *, keyword):
            return 'replacement'

        selected_code = target.__code__

        class Keyword(str):
            __hash__ = str.__hash__

            def __eq__(self, other):
                target.__code__ = replacement.__code__
                ids = set(_testinternalcapi.get_transition_root_ids())
                observations.append((id(selected_code) in ids,
                                     id(payload) in ids, id(target) in ids))
                support.gc_collect()
                return str.__eq__(self, other)

        self.assertEqual(target(payload, **{Keyword('keyword'): None}),
                         (payload, None))
        self.assertEqual(target(payload, keyword=None), 'replacement')
        self.assertTrue(observations)
        self.assertTrue(all(all(entry) for entry in observations))
        self.assertEqual(_testinternalcapi.get_transition_root_ids(), [])


class ProviderHandoffTests(unittest.TestCase):
    STAGED = 2

    def test_delegated_hook_lifetime_and_reentry(self):
        namespace = {}
        assignments = ''.join(f'    local_{i} = depth\n' for i in range(128))
        exec('def grow(depth):\n' + assignments +
             '    if depth:\n        grow(depth - 1)\n', namespace)
        for object_provider in (False, True):
            with self.subTest(object_provider=object_provider):
                payload = Item()
                observations = []

                def observe(phase):
                    states = _testinternalcapi.get_transition_shadow_status()
                    ids = set(_testinternalcapi.get_transition_root_ids())
                    observations.append((phase, states, id(target) in ids,
                                         id(payload) in ids,
                                         id(selected_code) in ids))
                    namespace['grow'](24)
                    support.gc_collect()
                    # Before default, an outer live stage protects the child.
                    # Afterwards a fresh child may reuse its arena address.
                    if phase in ('before', 'after'):
                        self.assertIs(target(payload), payload)

                def target(remaining):
                    observe('body')
                    return remaining

                selected_code = target.__code__
                result = _testinternalcapi.run_staged_provider(
                    target, payload, observe, 'default', object_provider)
                self.assertIs(result, payload)
                self.assertEqual([entry[0] for entry in observations],
                                 ['before', 'body', 'body', 'after', 'body'])
                for entry, expected_live in zip(observations,
                                               (True, True, False, False, False)):
                    phase, states, function_root, payload_root, code_root = entry
                    self.assertTrue(all(len(state) == 4 for state in states))
                    stages = [state for state in states if state[0] == self.STAGED]
                    self.assertTrue(stages, phase)
                    self.assertEqual(sum(state[3] for state in stages),
                                     int(expected_live), phase)
                    self.assertEqual((function_root, payload_root, code_root),
                                     (expected_live,) * 3, phase)
                self.assertFalse(any(kind == self.STAGED and has_frame
                                     for kind, reason, status, has_frame in
                                     _testinternalcapi.get_transition_shadow_status()))
                self.assertEqual(_testinternalcapi.get_transition_root_ids(), [])

    def test_hook_abandonment_and_explicit_consumption(self):
        for object_provider in (False, True):
            for mode in ('return', 'error', 'retire', 'activate', 'entry_error'):
                with self.subTest(object_provider=object_provider, mode=mode):
                    payload = Item()
                    observations = []

                    def target(remaining):
                        self.fail('this route must not execute the body')

                    def observe(phase):
                        states = _testinternalcapi.get_transition_shadow_status()
                        ids = set(_testinternalcapi.get_transition_root_ids())
                        observations.append((phase, states,
                                             id(target) in ids,
                                             id(payload) in ids))
                        support.gc_collect()

                    arguments = target, payload, observe, mode, object_provider
                    if mode == 'error':
                        with self.assertRaisesRegex(ValueError, 'staged hook error'):
                            _testinternalcapi.run_staged_provider(*arguments)
                    elif mode == 'entry_error':
                        with self.assertRaises(RecursionError):
                            _testinternalcapi.run_staged_provider(*arguments)
                    else:
                        self.assertIs(_testinternalcapi.run_staged_provider(*arguments),
                                      payload)
                    expected_phases = ['before', 'after']
                    if mode == 'activate':
                        expected_phases.insert(1, 'active')
                    self.assertEqual([entry[0] for entry in observations],
                                     expected_phases)
                    for phase, states, function_root, payload_root in observations:
                        expected_live = phase == 'before' or mode in ('return', 'error')
                        stages = [state for state in states if state[0] == self.STAGED]
                        self.assertTrue(stages, phase)
                        self.assertEqual(sum(state[3] for state in stages),
                                         int(expected_live), phase)
                        self.assertEqual((function_root, payload_root),
                                         (expected_live,) * 2, phase)
                    self.assertFalse(any(kind == self.STAGED and has_frame
                                         for kind, reason, status, has_frame in
                                         _testinternalcapi.get_transition_shadow_status()))
                    self.assertEqual(_testinternalcapi.get_transition_root_ids(), [])

    def test_selected_code_survives_hook_mutation(self):
        for object_provider in (False, True):
            with self.subTest(object_provider=object_provider):
                payload = Item()
                observations = []

                def observe(phase):
                    if phase == 'before':
                        target.__code__ = replacement.__code__
                    ids = set(_testinternalcapi.get_transition_root_ids())
                    observations.append((phase, id(selected_code) in ids))
                    support.gc_collect()

                def target(remaining):
                    observe('body')
                    return remaining

                def replacement(remaining):
                    observe('replacement')
                    return remaining

                selected_code = target.__code__
                self.assertIs(_testinternalcapi.run_staged_provider(
                    target, payload, observe, 'default', object_provider), payload)
                self.assertEqual(observations,
                                 [('before', True), ('body', False), ('after', False)])
                self.assertIs(target(payload), payload)
                self.assertEqual(observations[-1], ('replacement', False))
                self.assertEqual(_testinternalcapi.get_transition_root_ids(), [])

    def test_hook_installed_mapping_has_staged_header_authority(self):
        for object_provider in (False, True):
            with self.subTest(object_provider=object_provider):
                payload = {}
                observations = []

                def observe(phase):
                    ids = set(_testinternalcapi.get_transition_root_ids())
                    observations.append((phase, id(payload['mapping']) in ids))
                    support.gc_collect()

                def target(remaining):
                    observe('body')
                    return remaining

                self.assertIs(_testinternalcapi.run_staged_provider(
                    target, payload, observe, 'headers', object_provider), payload)
                self.assertEqual(observations,
                                 [('before', True), ('body', False), ('after', False)])
                self.assertEqual(_testinternalcapi.get_transition_root_ids(), [])

    def test_probe_cleanup_rejects_reentry_until_all_state_closes(self):
        for object_provider in (False, True):
            with self.subTest(object_provider=object_provider):
                payload = Item()
                observations = []

                def nested_target(remaining):
                    return remaining

                def observe(phase):
                    if phase == 'before':
                        target.__code__ = replacement.__code__

                class Constant:
                    def __del__(self):
                        try:
                            _testinternalcapi.run_staged_provider(
                                nested_target, payload, observations.append,
                                'return', object_provider)
                        except RuntimeError as error:
                            observations.append(str(error))
                        else:
                            observations.append('accepted during cleanup')

                def target(remaining):
                    observe('body')
                    return remaining

                def replacement(remaining):
                    observe('replacement')
                    return remaining

                # After code replacement and default retirement, only the
                # helper's selected-code pin keeps this constant alive.
                target.__code__ = target.__code__.replace(
                    co_consts=target.__code__.co_consts + (Constant(),))
                self.assertIs(_testinternalcapi.run_staged_provider(
                    target, payload, observe, 'default', object_provider), payload)
                self.assertEqual(observations, ['staged provider probe is active'])
                self.assertIs(_testinternalcapi.run_staged_provider(
                    nested_target, payload, lambda phase: None,
                    'return', object_provider), payload)
                self.assertEqual(_testinternalcapi.get_transition_root_ids(), [])

    def test_object_hook_result_keeps_scalar_identity(self):
        payload = float('2.5')
        observations = []

        def target(remaining):
            self.fail('the hook replaces this call')

        def observe(phase):
            observations.append(phase)
            support.gc_collect()

        result = _testinternalcapi.run_staged_provider(
            target, payload, observe, 'return', True)
        self.assertIs(result, payload)
        self.assertEqual(observations, ['before', 'after'])
        self.assertEqual(_testinternalcapi.get_transition_root_ids(), [])

    def test_constructor_failure_has_no_linked_stage(self):
        for object_provider in (False, True):
            with self.subTest(object_provider=object_provider):
                observations = []

                def target(first, missing):
                    self.fail('binding must fail before hook entry')

                with self.assertRaisesRegex(TypeError, 'missing'):
                    _testinternalcapi.run_staged_provider(
                        target, Item(), observations.append, 'default', object_provider)
                self.assertEqual(observations, [])
                self.assertFalse(any(kind == self.STAGED and has_frame
                                     for kind, reason, status, has_frame in
                                     _testinternalcapi.get_transition_shadow_status()))
                self.assertEqual(_testinternalcapi.get_transition_root_ids(), [])


class UnpackPrefixTests(unittest.TestCase):
    def test_prefix_survives_iterator_and_sequence_cleanup(self):
        references = []
        observations = []

        def observe(name):
            ids = set(_testinternalcapi.get_transition_root_ids())
            observations.append((name, all(id(ref()) in ids for ref in references)))
            support.gc_collect()

        class Iterator:
            position = 0

            def __iter__(self):
                return self

            def __next__(self):
                if self.position:
                    observe('next')
                if self.position == 2:
                    raise StopIteration
                self.position += 1
                item = Item()
                references.append(weakref.ref(item))
                return item

            def __del__(self):
                observe('iterator')

        class Sequence:
            def __iter__(self):
                return Iterator()

            def __del__(self):
                observe('sequence')

        first, second = Sequence()
        self.assertIs(first, references[0]())
        self.assertIs(second, references[1]())
        self.assertEqual(observations, [('next', True), ('next', True),
                                        ('iterator', True), ('sequence', True)])
        self.assertEqual(_testinternalcapi.get_transition_root_ids(), [])

    def test_partial_prefix_errors(self):
        cases = ((False, 0, False), (False, 1, False), (False, 3, False),
                 (False, 1, True), (True, 0, False), (True, 1, False),
                 (True, 1, True))
        for starred, limit, fail in cases:
            with self.subTest(starred=starred, limit=limit, fail=fail):
                references = []
                observations = []

                class Iterator:
                    position = 0

                    def __iter__(self):
                        return self

                    def __next__(self):
                        if self.position:
                            ids = set(_testinternalcapi.get_transition_root_ids())
                            observations.append(all(id(ref()) in ids
                                                    for ref in references[:2]))
                            support.gc_collect()
                        if self.position == limit:
                            if fail:
                                raise RuntimeError('iteration')
                            raise StopIteration
                        self.position += 1
                        item = Item()
                        references.append(weakref.ref(item))
                        return item

                class Sequence:
                    def __iter__(self):
                        return Iterator()

                with self.assertRaises(RuntimeError if fail else ValueError):
                    if starred:
                        first, *middle, second = Sequence()
                    else:
                        first, second = Sequence()
                support.gc_collect()
                self.assertTrue(all(observations))
                self.assertTrue(all(ref() is None for ref in references))
                self.assertEqual(_testinternalcapi.get_transition_root_ids(), [])

    def test_starred_prefix_cleanup(self):
        references = []
        observations = []

        class Iterator:
            position = 0

            def __iter__(self):
                return self

            def __next__(self):
                if self.position:
                    ids = set(_testinternalcapi.get_transition_root_ids())
                    observations.append(id(references[0]()) in ids)
                    support.gc_collect()
                if self.position == 4:
                    raise StopIteration
                self.position += 1
                item = Item()
                references.append(weakref.ref(item))
                return item

        class Sequence:
            def __iter__(self):
                return Iterator()

            def __del__(self):
                ids = set(_testinternalcapi.get_transition_root_ids())
                observations.append(id(references[0]()) in ids and
                                    id(references[-1]()) in ids)
                support.gc_collect()

        first, *middle, last = Sequence()
        self.assertEqual([first, *middle, last], [ref() for ref in references])
        self.assertTrue(all(observations))
        self.assertEqual(_testinternalcapi.get_transition_root_ids(), [])

    def test_nested_unpack_prefixes(self):
        references = {'outer': [], 'inner': []}
        observations = []

        class Iterator:
            def __init__(self, name):
                self.name = name
                self.position = 0

            def __iter__(self):
                return self

            def __next__(self):
                if self.name == 'outer' and self.position == 1:
                    first, second = Sequence('inner')
                if self.position == 2:
                    raise StopIteration
                self.position += 1
                item = Item()
                references[self.name].append(weakref.ref(item))
                return item

        class Sequence:
            def __init__(self, name):
                self.name = name

            def __iter__(self):
                return Iterator(self.name)

            def __del__(self):
                if self.name == 'inner':
                    ids = set(_testinternalcapi.get_transition_root_ids())
                    observations.append(id(references['outer'][0]()) in ids and
                                        all(id(ref()) in ids
                                            for ref in references['inner']))
                    support.gc_collect()

        first, second = Sequence('outer')
        self.assertEqual(observations, [True])
        self.assertIs(first, references['outer'][0]())
        self.assertIs(second, references['outer'][1]())
        self.assertEqual(_testinternalcapi.get_transition_root_ids(), [])

    def test_unpack_callback_cannot_redirect_ancestor(self):
        rejections = []
        observations = []
        references = []

        class Iterator:
            position = 0

            def __iter__(self):
                return self

            def __next__(self):
                if self.position:
                    ids = set(_testinternalcapi.get_transition_root_ids())
                    observations.append(id(references[0]()) in ids)
                    support.gc_collect()
                if self.position == 2:
                    raise StopIteration
                self.position += 1
                item = Item()
                references.append(weakref.ref(item))
                return item

        class Sequence:
            def __iter__(self):
                return Iterator()

        def unpack():
            changed = False
            first, second = Sequence()
            return first, second, changed

        def trace(frame, event, arg):
            if frame.f_code is Iterator.__next__.__code__ and event == 'line':
                ancestor = frame.f_back
                if ancestor is not None and ancestor.f_code is unpack.__code__:
                    try:
                        ancestor.f_lineno = unpack.__code__.co_firstlineno + 3
                    except ValueError:
                        rejections.append(True)
                    else:
                        rejections.append(False)
                    ancestor.f_locals['changed'] = True
            return trace

        previous = sys.gettrace()
        sys.settrace(trace)
        try:
            first, second, changed = unpack()
        finally:
            sys.settrace(previous)
        self.assertIs(first, references[0]())
        self.assertIs(second, references[1]())
        self.assertTrue(changed)
        self.assertTrue(rejections)
        self.assertTrue(all(rejections))
        self.assertTrue(all(observations))
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
