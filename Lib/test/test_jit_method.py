"""Semantic-helper and whole-method native execution contracts."""
import platform
import sys
import sysconfig
import types
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


@unittest.skipUnless(
    sys.platform == "linux" and platform.machine() == "x86_64" and
    not sysconfig.get_config_var("Py_GIL_DISABLED") and
    not sysconfig.get_config_var("Py_STATS") and
    not sysconfig.get_config_var("_Py_TAIL_CALL_INTERP"),
    "whole-method baseline requires Linux x86-64 GIL/default evaluator",
)
class TestNativeMethods(unittest.TestCase):
    def compile(self, function, *, complete=True):
        self.assertTrue(_testinternalcapi.jit_method_compile(function))
        info = _testinternalcapi.jit_method_info(function)
        self.assertTrue(info["active"])
        self.assertFalse(info["entry_seen"])
        if complete:
            self.assertEqual(info["exits"], 0)
        self.assertEqual(len(info["code"]), info["code_size"])
        self.assertGreaterEqual(info["mapping_size"], info["code_size"])
        self.assertEqual(info["code"][:4], b"\xf3\x0f\x1e\xfa")
        for offset in info["offsets"]:
            self.assertEqual(info["code"][offset:offset + 4], b"\xf3\x0f\x1e\xfa")
        return info

    def test_loop_python_call_c_call_and_dunder(self):
        ledger = []

        class Number:
            def __init__(self, value):
                self.value = value

            def __abs__(self):
                ledger.append(self.value)
                return self.value + 3

        def callback(i):
            return Number(i) if i & 1 else -i

        def bridge(cb, i):
            return abs(cb(i))

        def work(n, cb, call):
            total = 0
            i = 0
            while i < n:
                if i & 1:
                    total = total + call(cb, i)
                else:
                    total = total - call(cb, i)
                i = i + 1
            return total

        self.compile(bridge)
        self.compile(work)
        for n in (0, 1, 7, 12):
            ledger.clear()
            self.assertEqual(work(n, callback, bridge),
                sum(i + 3 if i & 1 else -i for i in range(n)))
            self.assertEqual(ledger, list(range(1, n, 2)))
        work_info = _testinternalcapi.jit_method_info(work)
        bridge_info = _testinternalcapi.jit_method_info(bridge)
        self.assertTrue(work_info["entry_seen"])
        self.assertTrue(work_info["return_resume_seen"])
        self.assertTrue(bridge_info["entry_seen"])
        self.assertTrue(bridge_info["c_call_seen"])
        self.assertTrue(bridge_info["return_resume_seen"])
        print("JIT_METHOD_NATIVE_WITNESS", work_info["address"],
              work_info["mapping_size"], work_info["offsets"], work_info["code"].hex(), flush=True)
        with open("/proc/self/maps", encoding="ascii") as maps:
            address = work_info["address"]
            for line in maps:
                bounds, permissions, *_ = line.split()
                low, high = (int(value, 16) for value in bounds.split("-"))
                if low <= address and address + work_info["mapping_size"] <= high:
                    self.assertIn("x", permissions)
                    self.assertNotIn("w", permissions)
                    break
            else:
                self.fail("native mapping absent from process maps")

    def test_raise_effect_and_traceback(self):
        events = []

        class Bad:
            def __add__(self, other):
                events.append("add")
                raise ValueError("method helper")

        def work(value):
            return value + 1

        self.compile(work)
        try:
            work(Bad())
        except ValueError as error:
            names = []
            traceback = error.__traceback__
            while traceback is not None:
                names.append(traceback.tb_frame.f_code.co_name)
                traceback = traceback.tb_next
            self.assertEqual(names[-2:], ["work", "__add__"])
        else:
            self.fail("missing operation exception")
        self.assertEqual(events, ["add"])
        self.assertTrue(_testinternalcapi.jit_method_info(work)["entry_seen"])
        self.assertEqual(work(8), 9)

    def test_observer_after_effect_and_recompile(self):
        events = []

        def work(callback, value):
            callback()
            return value + 1

        def profile(frame, event, arg):
            if frame.f_code is work.__code__:
                events.append(event)

        def enable():
            events.append("enable")
            sys.setprofile(profile)

        self.compile(work)
        try:
            self.assertEqual(work(enable, 9), 10)
        finally:
            sys.setprofile(None)
        self.assertEqual(events.count("enable"), 1)
        self.assertIn("return", events)
        info = _testinternalcapi.jit_method_info(work)
        self.assertFalse(info["active"])
        self.assertFalse(info["disabled"])
        self.compile(work)
        self.assertEqual(work(lambda: None, 13), 14)
        self.assertTrue(_testinternalcapi.jit_method_info(work)["return_resume_seen"])

    def test_unsupported_disables_only_affected_code(self):
        def other(value):
            return value + 2

        def unsupported(callback, value):
            items = [value]
            return callback(items[0])

        self.compile(other)
        self.compile(unsupported, complete=False)
        self.assertEqual(unsupported(other, 7), 9)
        info = _testinternalcapi.jit_method_info(unsupported)
        self.assertFalse(info["active"])
        self.assertTrue(info["disabled"])
        self.assertFalse(_testinternalcapi.jit_method_compile(unsupported))
        self.assertTrue(_testinternalcapi.jit_method_info(other)["entry_seen"])
        self.assertEqual(other(19), 21)

    def test_ephemeral_code_constants_die_before_result_store(self):
        events = []

        class Owned:
            def __del__(self):
                events.append(sys._getframe(1).f_locals["result"])

        def template(value):
            return value + 1

        def make():
            code = template.__code__.replace(co_consts=template.__code__.co_consts + (Owned(),))
            function = types.FunctionType(code, globals())
            self.assertTrue(_testinternalcapi.jit_method_compile(function))
            return function

        def caller(factory):
            result = "before"
            result = factory()(19)
            return result

        self.assertEqual(caller(make), 20)
        self.assertEqual(events, ["before"])

    def test_callback_invalidation_keeps_live_native_return(self):
        events = []

        def work(value):
            return abs(value) + 1

        class Invalidating:
            def __abs__(self):
                events.append(8)
                # abs keeps its C helper and native return address live while
                # this Python callback detaches the current code unit.
                sys.setprofile(lambda *args: None)
                sys.setprofile(None)
                return 11

        self.compile(work)
        self.assertEqual(work(Invalidating()), 12)
        self.assertEqual(events, [8])
        self.assertFalse(_testinternalcapi.jit_method_info(work)["active"])
        self.compile(work)
        self.assertEqual(work(-5), 6)
        self.assertTrue(_testinternalcapi.jit_method_info(work)["entry_seen"])


if __name__ == "__main__":
    unittest.main()
