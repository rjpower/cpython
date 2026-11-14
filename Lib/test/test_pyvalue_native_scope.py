"""Native scope mechanisms and selected ordinary VM caller routes."""

import functools
import gc
import struct
import sys
import threading
import unittest

from test import support
from test.support import import_helper

_testinternalcapi = import_helper.import_module("_testinternalcapi")


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

    def test_end_keeps_pending_exception_over_result_finalizer(self):
        class Result:
            def __del__(self):
                raise RuntimeError("native scope result finalizer")

        reentered = []
        with support.catch_unraisable_exception() as caught:
            capture = sys.unraisablehook

            def reenter(event):
                reentered.append(_testinternalcapi.native_scope_echo(3.25))
                capture(event)

            sys.unraisablehook = reenter
            try:
                with self.assertRaisesRegex(ValueError,
                                            "native scope primary failure"):
                    _testinternalcapi.native_scope_end_pending(Result)
            finally:
                sys.unraisablehook = capture
            self.assertIs(caught.unraisable.exc_type, RuntimeError)
            self.assertEqual(str(caught.unraisable.exc_value),
                             "native scope result finalizer")
        self.assertEqual(reentered, [3.25])

    @unittest.skipIf(support.Py_GIL_DISABLED or struct.calcsize("P") != 8,
                     "requires inline scalar values")
    def test_box_oom(self):
        # Two PyLong digits bypass the one-digit freelist as well as cache.
        with self.assertRaises(MemoryError):
            _testinternalcapi.native_scope_box_oom(2147483647)
        # The one-shot wrapper restores the allocator before error handling.
        self.assertEqual(_testinternalcapi.native_scope_publish(1234567),
                         1234567)


class NativeScopeDispatchTests(unittest.TestCase):
    def warm(self, function, value):
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            self.assertEqual(function(value), value)
        if not support.Py_GIL_DISABLED:
            # These module-attribute sites retain the runtime self-or-NULL slot.
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
            self.assertEqual(call_handlers[-1], "CALL_NO_SELF_BUILTIN_O")
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
                    # The cold calls left the site backing off, because the
                    # specialiser declines an inline callable (upstream
                    # specialises it to CALL_NON_PY_GENERAL or
                    # CALL_KW_NON_PY). Warm a fresh site.
                    support.reset_code(call)
                    target = _testinternalcapi.native_scope_vector_method()
                    for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
                        call(target)
                    expected = ('CALL_KW_NO_SELF_NON_PY' if call is keyword
                                else 'CALL_NO_SELF_NON_PY_GENERAL')
                    self.assertIn(expected, executing_handlers(call))
                for target, typename in ((2147483647, 'int'), (3.25, 'float')):
                    with self.subTest(call=call.__name__, warm=warm, target=target):
                        ledger.clear()
                        with self.assertRaisesRegex(TypeError, typename + '.*not callable'):
                            call(target)
                        self.assertEqual(ledger, ['second', 'first'])


class Pending(Exception):
    pass


# Each site folds `x = call` into the call's local destination.
def positional_site(target, first, second):
    x = 'old'
    try:
        x = target(first, second)
    except Pending:
        return 'raised', x
    return 'returned', x


def keyword_site(target, first, second):
    x = 'old'
    try:
        x = target(first, num=second)
    except Pending:
        return 'raised', x
    return 'returned', x


def star_site(target, first, second):
    x = 'old'
    try:
        x = target(*(first, second))
    except Pending:
        return 'raised', x
    return 'returned', x


def single_site(target, first, second):
    x = 'old'
    try:
        x = target(first)
    except Pending:
        return 'raised', x
    return 'returned', x


def python_callee(first, second=None, num=None):
    return 0


class CallPeriodicCheckStoreOrderTests(unittest.TestCase):
    # Upstream closes a call's inputs, runs its periodic check
    # (_CHECK_PERIODIC_AT_END) and only then stores the result with
    # STORE_FAST. So a pending call run by the check sees the local's old
    # value, and an exception from the check leaves the old value in place.

    @classmethod
    def setUpClass(cls):
        cls.pending = import_helper.import_module('_testcapi')._pending_threadfunc
        cls.set_async_exc = import_helper.import_module(
            '_testlimitedcapi').threadstate_set_async_exc

    def observe(self):
        frame = sys._getframe(1)
        self.seen.append((frame.f_code.co_name, frame.f_locals.get('x')))

    def raise_pending(self):
        raise Pending

    def trips(self, site):
        """Yield (trip, target, first, second, result) for calls that leave
        work for their own periodic check."""
        if site is single_site:
            simple = _testinternalcapi.simple_pending_call
            yield 'pending call', simple, self.observe, None, ('returned', None)
            yield ('pending call raises', simple, self.raise_pending, None,
                   ('raised', 'old'))
            return
        yield 'pending call', self.pending, self.observe, 1, ('returned', 1)
        yield ('pending call raises', self.pending, self.raise_pending, 1,
               ('raised', 'old'))
        if site is not keyword_site:
            yield ('async exception', self.set_async_exc,
                   threading.get_ident(), Pending, ('raised', 'old'))

    def check_site(self, site, handler, cold=False):
        for trip, target, first, second, result in self.trips(site):
            with self.subTest(handler=handler, trip=trip):
                if cold:
                    support.reset_code(site)
                self.seen = []
                self.assertEqual(site(target, first, second), result)
                expected = []
                if trip == 'pending call':
                    expected = [(site.__name__, 'old')]
                self.assertEqual(self.seen, expected)
                # The site still holds the handler that ran the call.
                self.assertIn(handler, executing_handlers(site))

    def warm(self, site, target, first=None):
        support.reset_code(site)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            site(target, first, None)

    def test_generic_handlers(self):
        self.check_site(positional_site, 'CALL_NO_SELF', cold=True)
        self.check_site(star_site, 'CALL_EX_NO_SELF_NO_KW', cold=True)

    def test_specialized_handlers(self):
        native = functools.partial(python_callee)
        for site, target, first, handler in (
                (positional_site, native, None, 'CALL_NO_SELF_NON_PY_GENERAL'),
                (keyword_site, native, None, 'CALL_KW_NO_SELF_NON_PY'),
                (star_site, native, None, 'CALL_EX_NO_SELF_NO_KW_NON_PY_GENERAL'),
                # The warm-up's pending calls to int() do nothing visible.
                (single_site, _testinternalcapi.simple_pending_call, int,
                 'CALL_NO_SELF_BUILTIN_O')):
            self.warm(site, target, first)
            self.assertIn(handler, executing_handlers(site))
            self.check_site(site, handler)

    def test_failed_check_releases_result_after_raise_event(self):
        # Upstream's unwinder pops the result after the frame's RAISE event.
        ledger = []
        pending = self.pending
        raise_pending = self.raise_pending

        class Result:
            def __del__(self):
                ledger.append('result')

        def callee(first, second):
            result = Result()
            # A cold CALL_KW has no periodic check, so the pending call waits
            # for the check of the call into callee.
            pending(raise_pending, num=1)
            return result

        monitoring = sys.monitoring
        tool = monitoring.PROFILER_ID
        monitoring.use_tool_id(tool, 'test_pyvalue_native_scope')
        self.addCleanup(monitoring.free_tool_id, tool)
        monitoring.register_callback(
            tool, monitoring.events.RAISE,
            lambda code, offset, exc: ledger.append(('raise', code.co_name)))
        native = functools.partial(callee)
        for handler in ('CALL_NO_SELF', 'CALL_NO_SELF_NON_PY_GENERAL'):
            with self.subTest(handler=handler):
                if handler == 'CALL_NO_SELF':
                    support.reset_code(positional_site)
                else:
                    self.warm(positional_site, functools.partial(python_callee))
                code = positional_site.__code__
                monitoring.set_local_events(tool, code, monitoring.events.RAISE)
                try:
                    support.reset_code(callee)
                    ledger.clear()
                    self.assertEqual(positional_site(native, None, None),
                                     ('raised', 'old'))
                    self.assertEqual(ledger,
                                     [('raise', 'positional_site'), 'result'])
                    self.assertIn(handler, executing_handlers(positional_site))
                finally:
                    monitoring.set_local_events(tool, code, 0)

    def test_instrumented_handlers(self):
        monitoring = sys.monitoring
        tool = monitoring.PROFILER_ID
        monitoring.use_tool_id(tool, 'test_pyvalue_native_scope')
        self.addCleanup(monitoring.free_tool_id, tool)
        monitoring.register_callback(tool, monitoring.events.CALL,
                                     lambda *args: None)
        for site, handler in ((positional_site, 'INSTRUMENTED_CALL_NO_SELF'),
                              (star_site, 'INSTRUMENTED_CALL_EX_NO_SELF_NO_KW')):
            support.reset_code(site)
            monitoring.set_local_events(tool, site.__code__,
                                        monitoring.events.CALL)
            try:
                self.check_site(site, handler)
            finally:
                monitoring.set_local_events(tool, site.__code__, 0)


if __name__ == "__main__":
    unittest.main()
