"""Real semantic-helper execution through the ordinary evaluator (not JIT code)."""
import sys
import sysconfig
import unittest

from test.support import import_helper

_testinternalcapi = import_helper.import_module("_testinternalcapi")


@unittest.skipIf(
    sysconfig.get_config_var("Py_GIL_DISABLED") or
    sysconfig.get_config_var("Py_STATS") or
    sysconfig.get_config_var("_Py_TAIL_CALL_INTERP"),
    "semantic-step fixture requires the GIL non-stats default evaluator",
)
class TestMethodHelpers(unittest.TestCase):
    def step(self, function, args=(), limit=10000):
        return _testinternalcapi.jit_method_step_call(function, args, limit)

    def test_loop_branch_and_python_call(self):
        ledger = []

        # Callee stays interpreted; its completed return must resume this
        # requested caller's helpers after storing the result exactly once.
        def add(i):
            ledger.append(i)
            return i + 2

        def work(n, cb):
            total = 0
            i = 0
            while i < n:
                if i & 1:
                    total = total + cb(i)
                else:
                    total = total - 1
                i = i + 1
            return total

        for n in (7, 0, 1, 12):
            with self.subTest(n=n):
                ledger.clear()
                result, actions, phases = self.step(work, (n, add))
                actual_ledger = ledger[:]
                ledger.clear()
                self.assertEqual(result, work(n, add))
                self.assertEqual(result, sum(i + 2 if i & 1 else -1 for i in range(n)))
                self.assertEqual(actual_ledger, ledger)
                self.assertEqual(actual_ledger, list(range(1, n, 2)))
                self.assertEqual(actions[2], n // 2)
                self.assertEqual(phases[1], n // 2)
                self.assertEqual(phases[2], 0)
                self.assertEqual(actions[3], 1)  # completed return
                if n > 1:
                    self.assertGreater(actions[1], 0)  # branch
                    self.assertGreater(actions[2], 0)  # real Python frame transfer
                    self.assertGreater(phases[1], 0)  # start pending, not done twice
                    self.assertGreater(phases[3], 0)  # result committed before return

    def test_c_call_dunder_and_effects_once(self):
        ledger = []

        class Number:
            def __abs__(self):
                ledger.append("abs")
                return 17

        def work(value):
            return abs(value)

        result, actions, phases = self.step(work, (Number(),))
        self.assertEqual(result, 17)
        self.assertEqual(ledger, ["abs"])
        self.assertGreater(actions[0], 0)
        self.assertEqual(actions[3], 1)

    def test_error_action_caught_without_replay(self):
        ledger = []

        class Bad:
            def __add__(self, other):
                ledger.append("add")
                raise ValueError("semantic helper")

        def work(value):
            try:
                return value + 1
            except ValueError:
                return 23

        result, actions, phases = self.step(work, (Bad(),))
        self.assertEqual(result, 23)
        self.assertEqual(ledger, ["add"])
        self.assertEqual(actions[4], 1)
        self.assertEqual(phases[4], 1)

    def test_request_restored_on_raise_and_nested_request(self):
        def fail(value):
            return 1 // value

        with self.assertRaises(ZeroDivisionError):
            self.step(fail, (0,))
        self.assertEqual(self.step(fail, (1,))[0], 1)

        def nested():
            return _testinternalcapi.jit_method_step_call(fail, (1,))[0]

        self.assertEqual(self.step(nested)[0], 1)

    def test_bounded_before_effect_tier_down(self):
        events = []

        def work(value):
            events.append(value)
            return value + 1

        result, actions, phases = self.step(work, (9,), limit=1)
        self.assertEqual(result, 10)
        self.assertEqual(events, [9])
        self.assertLessEqual(sum(actions), 1)

    def test_profile_enabled_from_callee(self):
        events = []

        def profile(frame, event, arg):
            if frame.f_code is work.__code__:
                events.append(event)

        def enable():
            sys.setprofile(profile)

        def work(callback, value):
            callback()
            return value + 1

        try:
            result, actions, phases = self.step(work, (enable, 8))
        finally:
            sys.setprofile(None)
        self.assertEqual(result, 9)
        self.assertIn("return", events)
        self.assertEqual(actions[2], 1)
        self.assertEqual(actions[3], 0)  # observer-safe tier-down, not a helper return

        def recovered(value):
            return value + 2

        result, actions, phases = self.step(recovered, (10,))
        self.assertEqual(result, 12)
        self.assertEqual(actions[3], 1)  # fresh code still works after the global epoch changed

    def test_owned_local_published_before_destructor_reentry(self):
        ledger = []

        class Owned:
            def __del__(self):
                ledger.append((sys._getframe(1).f_locals["value"], sum((2, 3))))

        def factory():
            return Owned()

        def work(make):
            value = make()
            value = 19
            return value

        result, actions, phases = self.step(work, (factory,))
        self.assertEqual(result, 19)
        self.assertEqual(ledger, [(19, 5)])
        self.assertEqual(actions[2], 1)
        self.assertEqual(actions[3], 1)

    def test_generator_callee_fallback_and_unexpected_exception(self):
        def values():
            yield 3
            yield 8

        def work(source):
            return tuple(source)

        result, actions, phases = self.step(work, (values(),))
        self.assertEqual(result, (3, 8))
        self.assertEqual(actions[3], 1)

        def unexpected():
            raise NotImplementedError("requested program error")

        with self.assertRaisesRegex(NotImplementedError, "requested program error"):
            self.step(unexpected)


if __name__ == "__main__":
    unittest.main()
