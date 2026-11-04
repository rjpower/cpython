"""Native scope mechanisms and selected ordinary VM caller routes."""

import gc
import dis
import struct
import sys
import unittest

from test import support
from test.support import import_helper

_testinternalcapi = import_helper.import_module("_testinternalcapi")


def setUpModule():
    # Hot-call and backedge method admission is off by default.
    previous = _testinternalcapi.jit_method_natural(True)
    unittest.addModuleCleanup(_testinternalcapi.jit_method_natural, previous)


def executing_handlers(function):
    return [word[0] for word in
            _testinternalcapi.get_ir(function.__code__, True)]


class NativeScopeMechanismTests(unittest.TestCase):
    # These callbacks publish a C-owned destination. They do not claim to
    # exercise a VM frame or its post-cleanup reload protocol.
    def test_publish_scalar_and_object(self):
        for value in (1234567, -7654321, 3.25, None, True, object(), []):
            with self.subTest(value=value):
                result = _testinternalcapi.native_scope_publish(value)
                self.assertEqual(result, value)
                if type(value) not in (int, float):
                    self.assertIs(result, value)

    def test_abort_retains_destination(self):
        for value in (1234567, 3.25, object(), []):
            with self.subTest(value=value):
                result = _testinternalcapi.native_scope_abort(value)
                self.assertEqual(result, value)
                if type(value) not in (int, float):
                    self.assertIs(result, value)

    def test_gateway_mechanism(self):
        self.assertEqual(_testinternalcapi.native_scope_gateway_noargs(), 2.5)
        for value in (1234567, 3.25, None, object()):
            with self.subTest(value=value):
                result = _testinternalcapi.native_scope_gateway_o(value)
                self.assertEqual(result, value)
                if type(value) not in (int, float):
                    self.assertIs(result, value)

    def test_cleanup_failure(self):
        with support.catch_unraisable_exception() as caught:
            self.assertEqual(
                _testinternalcapi.native_scope_cleanup_error(1234567), 1234567)
            self.assertIs(caught.unraisable.exc_type, RuntimeError)
            self.assertEqual(str(caught.unraisable.exc_value),
                             "native scope cleanup failure")

    def test_primary_exception_survives_cleanup(self):
        with support.catch_unraisable_exception() as unraisable:
            with self.assertRaisesRegex(ValueError,
                                       "native scope primary failure") as caught:
                _testinternalcapi.native_scope_primary_error(1234567)
            # The C mechanism also checks the exact saved exception pointer.
            self.assertIsNone(caught.exception.__context__)
            self.assertIs(unraisable.unraisable.exc_type, RuntimeError)
            self.assertEqual(str(unraisable.unraisable.exc_value),
                             "native scope cleanup failure")

    @unittest.skipIf(support.Py_GIL_DISABLED or struct.calcsize("P") != 8,
                     "requires inline scalar values")
    def test_box_oom(self):
        # Two PyLong digits bypass the one-digit freelist as well as cache.
        with self.assertRaises(MemoryError):
            _testinternalcapi.native_scope_box_oom(2147483647)
        # The one-shot wrapper restores the allocator before error handling.
        self.assertEqual(_testinternalcapi.native_scope_publish(1234567),
                         1234567)


class NativeScopeVectorMechanismTests(unittest.TestCase):
    # This explicit C gateway publishes a C-owned destination. Generic VM
    # adoption and its executed-handler assertions belong to the paired cut.
    def test_calling_conventions(self):
        invoke = _testinternalcapi.native_scope_vector
        self.assertEqual(invoke(_testinternalcapi.native_scope_flat_noargs),
                         2147483647)
        value = object()
        self.assertIs(invoke(_testinternalcapi.native_scope_echo, value), value)
        self.assertEqual(invoke(_testinternalcapi.native_scope_flat_fast, 4, 7), 11)
        self.assertEqual(invoke(_testinternalcapi.native_scope_flat,
                                value=2147483647, scale=2), 4294967294)
        method = _testinternalcapi.native_scope_vector_method()
        self.assertIs(invoke(method, 1, extra=2), object)

    def test_argument_validation(self):
        invoke = _testinternalcapi.native_scope_vector
        flat = _testinternalcapi.native_scope_flat
        calls = (
            lambda: invoke(flat),
            lambda: invoke(flat, 1, value=2),
            lambda: invoke(flat, 1, unexpected=2),
            lambda: invoke(flat, 1, scale="not an integer"),
            lambda: invoke(_testinternalcapi.native_scope_flat_noargs, 1),
            lambda: invoke(_testinternalcapi.native_scope_echo, value=1),
            lambda: invoke(_testinternalcapi.native_scope_flat_fast, 1),
        )
        for call in calls:
            with self.subTest(call=call):
                with self.assertRaises(TypeError):
                    call()
        self.assertIsNone(_testinternalcapi.native_scope_vector_counts(flat))
        self.assertEqual(invoke(flat, 6, scale=3), 18)

    def test_nested_callback_and_body_error(self):
        events = []
        value = object()

        def callback(argument):
            events.append("body")
            self.assertIs(argument, value)
            self.assertEqual(_testinternalcapi.native_scope_vector(
                _testinternalcapi.native_scope_flat_fast, 2, 3), 5)
            return argument

        result = _testinternalcapi.native_scope_vector(
            _testinternalcapi.native_scope_flat, value, callback=callback)
        self.assertIs(result, value)
        self.assertEqual(events, ["body"])

        def fail(argument):
            events.append("failure")
            raise ValueError("vector body failure")

        with self.assertRaisesRegex(ValueError, "vector body failure"):
            _testinternalcapi.native_scope_vector(
                _testinternalcapi.native_scope_flat, value, callback=fail)
        self.assertEqual(events, ["body", "failure"])
        self.assertEqual(_testinternalcapi.native_scope_vector(
            _testinternalcapi.native_scope_flat, 5), 5)

    @unittest.skipIf(support.Py_GIL_DISABLED or struct.calcsize("P") != 8,
                     "requires inline scalar values")
    def test_partial_box_oom(self):
        with self.assertRaises(MemoryError):
            _testinternalcapi.native_scope_vector_box_oom(
                _testinternalcapi.native_scope_flat_fast)
        self.assertEqual(_testinternalcapi.native_scope_vector(
            _testinternalcapi.native_scope_flat_fast, 1, 2), 3)

    def test_entry_error_is_preserved(self):
        # The C mechanism verifies the exact entry exception pointer too.
        with self.assertRaisesRegex(ValueError, "native vector entry error"):
            _testinternalcapi.native_scope_vector_prior_error(
                _testinternalcapi.native_scope_flat_noargs)

    def test_custom_vectorcall_keeps_fallback(self):
        function = _testinternalcapi.native_scope_vector_override()
        self.assertFalse(_testinternalcapi.native_scope_vector_can_call(function))
        self.assertEqual(_testinternalcapi.native_scope_vector(function, 9),
                         "custom vectorcall")
        self.assertTrue(_testinternalcapi.native_scope_vector_can_call(
            _testinternalcapi.native_scope_flat))

    @unittest.skipIf(support.Py_GIL_DISABLED or struct.calcsize("P") != 8,
                     "requires inline scalar values")
    def test_target_change_before_invocation(self):
        function = _testinternalcapi.native_scope_flat_fast
        self.assertIsNone(
            _testinternalcapi.native_scope_vector_target_change(function))
        self.assertTrue(_testinternalcapi.native_scope_vector_can_call(function))
        self.assertEqual(_testinternalcapi.native_scope_vector(function, 2, 3), 5)


class NativeScopeDispatchTests(unittest.TestCase):
    def warm(self, function, value):
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            self.assertEqual(function(value), value)
        if not support.Py_GIL_DISABLED:
            self.assertIn("CALL_BUILTIN_O", executing_handlers(function))

    def test_cold_and_warmed_o(self):
        def call(value):
            return _testinternalcapi.native_scope_echo(value)

        # The initial general route still uses the existing bridge. Only the
        # specialised CALL_BUILTIN_O route adopts the new scope in Stage 1.
        self.assertEqual(call(1234567), 1234567)
        self.warm(call, 1234567)
        for value in (-7654321, 3.25, None, object(), []):
            with self.subTest(value=value):
                result = call(value)
                self.assertEqual(result, value)
                if type(value) not in (int, float):
                    self.assertIs(result, value)

    def test_noargs_and_argument_errors(self):
        # NOARGS is engine/gateway coverage only until its VM caller is
        # adopted; this ordinary call intentionally stays on the old bridge.
        self.assertEqual(_testinternalcapi.native_scope_noargs(), 2.5)
        with self.assertRaises(TypeError):
            _testinternalcapi.native_scope_noargs(1)
        with self.assertRaises(TypeError):
            _testinternalcapi.native_scope_echo()
        with self.assertRaises(TypeError):
            _testinternalcapi.native_scope_echo(1, 2)
        with self.assertRaises(TypeError):
            _testinternalcapi.native_scope_echo(value=1)

    def test_completed_call_cleanup_error(self):
        def invoke(factory):
            return _testinternalcapi.native_scope_discard(factory())

        def ordinary():
            return None

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            self.assertEqual(invoke(ordinary), 2147483647)
        if not support.Py_GIL_DISABLED:
            self.assertIn("CALL_BUILTIN_O", executing_handlers(invoke))
        ledger = []

        def make_argument():
            return _testinternalcapi.native_scope_cleanup_argument(ledger)

        # No Python parameter/local retains the capsule when native s dies.
        with support.catch_unraisable_exception() as caught:
            self.assertEqual(invoke(make_argument), 2147483647)
            self.assertIs(caught.unraisable.exc_type, RuntimeError)
            self.assertEqual(str(caught.unraisable.exc_value),
                             "completed native call cleanup")
        self.assertEqual(ledger, ["body", "cleanup"])
        self.assertEqual(invoke(ordinary), 2147483647)

    @unittest.skipIf(support.Py_GIL_DISABLED or struct.calcsize("P") != 8,
                     "requires inline scalar values")
    def test_end_result_box_cleanup_error(self):
        ledger = []
        probe = _testinternalcapi.native_scope_end_failure(ledger)

        def invoke(value):
            return probe(value)

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            self.assertEqual(invoke(None), 2147483647)
        self.assertIn("CALL_BUILTIN_O", executing_handlers(invoke))
        # The exact two-digit result box is scope.result.retired: its public
        # OBJ free occurs in End, before the original argument/callable pops.
        reentered = []
        with support.catch_unraisable_exception() as caught:
            capture = sys.unraisablehook

            def reenter(event):
                reentered.append(_testinternalcapi.native_scope_echo(3.25))
                capture(event)

            sys.unraisablehook = reenter
            try:
                self.assertEqual(invoke(True), 2147483647)
            finally:
                sys.unraisablehook = capture
            self.assertIs(caught.unraisable.exc_type, RuntimeError)
            self.assertEqual(str(caught.unraisable.exc_value),
                             "native scope End cleanup")
        self.assertEqual(reentered, [3.25])
        self.assertEqual(ledger, ["body", "end cleanup"])
        self.assertEqual(invoke(None), 2147483647)

    def test_nested_reentry_and_exception(self):
        def inner(value):
            return _testinternalcapi.native_scope_echo(value)

        self.warm(inner, 1234567)

        def outer(callback):
            return _testinternalcapi.native_scope_reenter(callback)

        def callback():
            return inner(1234567)

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            self.assertEqual(outer(callback), 1234567)
        if not support.Py_GIL_DISABLED:
            self.assertIn("CALL_BUILTIN_O", executing_handlers(outer))

        def fail(value):
            return _testinternalcapi.native_scope_raise(value)

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            with self.assertRaisesRegex(ValueError, "expected failure"):
                fail("expected failure")
        if not support.Py_GIL_DISABLED:
            self.assertIn("CALL_BUILTIN_O", executing_handlers(fail))

    def test_temporary_argument_before_local_replacement(self):
        events = []
        calls = []
        reenter = False

        class Old:
            def __del__(self):
                frame = sys._getframe(1)
                events.append(("old", frame.f_code is replace.__code__,
                               type(frame.f_locals["old"]) is object))

        class Argument:
            def __call__(self):
                calls.append("body")
                return object()

            def __del__(self):
                frame = sys._getframe(1)
                events.append(("argument", frame.f_code is replace.__code__,
                               isinstance(frame.f_locals["old"], Old)))
                # Retiring the original argument is a reentry boundary after
                # scope End, including when no scalar bridge owners exist.
                if reenter:
                    sys.setprofile(lambda *args: None)
                    sys.setprofile(None)

        class Receiver:
            def __del__(self):
                frame = sys._getframe(1)
                events.append(("receiver", frame.f_code is replace.__code__,
                               isinstance(frame.f_locals["old"], Old)))

        def replace():
            old = Old()
            # Use a separate fresh object result so the argument dies during
            # the call, before replacing the old local's owner.
            old = _testinternalcapi.native_scope_bound(Receiver())(Argument())
            return old

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            events.clear()
            calls.clear()
            try:
                replace()
            finally:
                sys.setprofile(None)
            gc.collect()
            self.assertEqual(calls, ["body"])
            self.assertEqual(events, [("argument", True, True),
                                      ("receiver", True, True),
                                      ("old", True, True)])
        if not support.Py_GIL_DISABLED:
            call_handlers = [name for name in executing_handlers(replace)
                             if name.startswith("CALL")]
            self.assertEqual(call_handlers[-1], "CALL_BUILTIN_O")
        events.clear()
        calls = []
        reenter = True
        try:
            replace()
        finally:
            sys.setprofile(None)
        self.assertEqual(calls, ["body"])
        self.assertEqual(events, [("argument", True, True),
                                  ("receiver", True, True),
                                  ("old", True, True)])

    def test_displaced_local_reports_unraisable(self):
        ledger = []

        def factory():
            ledger.append("factory")
            return _testinternalcapi.native_scope_cleanup_argument(ledger)

        def replace(make):
            old = make()
            old = 2147483647
            return old

        with support.catch_unraisable_exception() as caught:
            self.assertEqual(replace(factory), 2147483647)
            self.assertIs(caught.unraisable.exc_type, RuntimeError)
            self.assertEqual(str(caught.unraisable.exc_value),
                             "completed native call cleanup")
        self.assertEqual(ledger, ["factory", "cleanup"])
        self.assertEqual(replace(lambda: None), 2147483647)

    def test_displaced_local_finalizer_refreshes_dispatch(self):
        events = []

        class Old:
            def __del__(self):
                frame = sys._getframe(1)
                events.append((frame.f_code is replace.__code__,
                               frame.f_locals["old"] == 2147483647))
                sys.setprofile(lambda *args: None)
                sys.setprofile(None)

        def replace(make):
            old = make()
            old = 2147483647
            return _testinternalcapi.native_scope_echo(old)

        try:
            self.assertEqual(replace(Old), 2147483647)
        finally:
            sys.setprofile(None)
        self.assertEqual(events, [(True, True)])


class NativeGenericVMTests(unittest.TestCase):
    def test_preparation_target_change_and_observer_redispatch(self):
        target = _testinternalcapi.native_scope_flat

        def run(driver, target, callback):
            return _testinternalcapi.native_scope_vm_prepare(driver, target, callback=callback)

        def noop():
            return None

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            run(noop, target, None)
        self.assertIn('CALL_KW_NON_PY', executing_handlers(run))
        self.assertFalse(_testinternalcapi.jit_method_info(run)['active'])
        for observe in (False, True):
            with self.subTest(observe=observe):
                ledger = []

                def body(value):
                    ledger.append('body')
                    return value

                def driver():
                    return target(2147483647, callback=body)

                for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
                    driver()
                self.assertIn('CALL_KW_NON_PY', executing_handlers(driver))
                self.assertFalse(_testinternalcapi.jit_method_info(driver)['active'])
                ledger.clear()
                events = []

                def profile(frame, event, arg):
                    if event == 'c_call' and arg is target:
                        events.append('target')
                        ledger.append('c_call')

                def prepare():
                    ledger.append('prepare')
                    sys.setprofile(profile)

                try:
                    result = run(driver, target, prepare if observe else None)
                finally:
                    sys.setprofile(None)
                value, fired, handler, target_calls, callback_calls = result
                self.assertEqual(value, 2147483647)
                self.assertEqual(fired, 1)
                self.assertEqual(handler, dis._specialized_opmap['CALL_KW_NON_PY'])
                self.assertEqual(target_calls, 1)
                self.assertEqual(callback_calls, int(observe))
                self.assertEqual(ledger, ['prepare', 'c_call', 'body'] if observe else ['body'])
                self.assertEqual(events, ['target'] if observe else [])
                self.assertTrue(_testinternalcapi.native_scope_vector_can_call(target))

    def test_preparation_callback_error_restores_request(self):
        target = _testinternalcapi.native_scope_flat
        ledger = []
        failure = ValueError('native preparation once')

        def run(driver, target, callback):
            return _testinternalcapi.native_scope_vm_prepare(driver, target, callback=callback)

        def noop():
            return None

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            run(noop, target, None)
        self.assertIn('CALL_KW_NON_PY', executing_handlers(run))
        self.assertFalse(_testinternalcapi.jit_method_info(run)['active'])

        def body(value):
            ledger.append('body')
            return value

        def driver():
            return target(2147483647, callback=body)

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            driver()
        self.assertIn('CALL_KW_NON_PY', executing_handlers(driver))
        self.assertFalse(_testinternalcapi.jit_method_info(driver)['active'])
        ledger.clear()

        def prepare():
            ledger.append('prepare')
            raise failure

        with self.assertRaises(ValueError) as caught:
            run(driver, target, prepare)
        self.assertIs(caught.exception, failure)
        self.assertEqual(ledger, ['prepare'])
        self.assertTrue(_testinternalcapi.native_scope_vector_can_call(target))
        self.assertEqual(driver(), 2147483647)
        self.assertEqual(ledger, ['prepare', 'body'])

    def test_noargs_and_keyword_routes(self):
        def noargs():
            return _testinternalcapi.native_scope_flat_noargs()

        def keyword(value):
            return _testinternalcapi.native_scope_flat(value, scale=2)

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            self.assertEqual(noargs(), 2147483647)
            self.assertEqual(keyword(4), 8)
        self.assertIn('CALL_NON_PY_GENERAL', executing_handlers(noargs))
        self.assertIn('CALL_KW_NON_PY', executing_handlers(keyword))
        with self.assertRaises(TypeError):
            _testinternalcapi.native_scope_flat_noargs(1)
        with self.assertRaises(TypeError):
            _testinternalcapi.native_scope_flat(1, missing=2)

    def test_keyword_argument_retirement_precedes_local_assignment(self):
        ledger = []

        class Old:
            def __del__(self):
                ledger.append(('old', sys._getframe(1).f_locals['result']))

        class Value:
            def __mul__(self, scale):
                ledger.append(('body', scale))
                return 2147483647

            def __del__(self):
                ledger.append(('argument', isinstance(sys._getframe(1).f_locals['result'], Old)))
                sys.setprofile(lambda *args: None)
                sys.setprofile(None)

        def invoke(factory):
            result = Old()
            result = _testinternalcapi.native_scope_flat(factory(), scale=2)
            ledger.append('next')
            return result

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            invoke(lambda: 4)
        self.assertIn('CALL_KW_NON_PY', executing_handlers(invoke))
        ledger.clear()
        try:
            self.assertEqual(invoke(Value), 2147483647)
        finally:
            sys.setprofile(None)
        self.assertEqual(ledger, [('body', 2), ('argument', True),
                                  ('old', 2147483647), 'next'])

    def test_inline_noncallable_consumes_arguments_once(self):
        ledger = []

        class Argument:
            def __init__(self, name):
                self.name = name

            def __del__(self):
                ledger.append(self.name)

        def ordinary(target):
            return target(Argument('first'), Argument('second'))

        def keyword(target):
            return target(Argument('first'), second=Argument('second'))

        for call in (ordinary, keyword):
            for warm in (False, True):
                if warm:
                    target = _testinternalcapi.native_scope_vector_method()
                    for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
                        call(target)
                    expected = 'CALL_KW_NON_PY' if call is keyword else 'CALL_NON_PY_GENERAL'
                    self.assertIn(expected, executing_handlers(call))
                for target, typename in ((2147483647, 'int'), (3.25, 'float')):
                    with self.subTest(call=call.__name__, warm=warm, target=target):
                        ledger.clear()
                        with self.assertRaisesRegex(TypeError, typename + '.*not callable'):
                            call(target)
                        self.assertEqual(ledger, ['second', 'first'])


if __name__ == "__main__":
    unittest.main()
