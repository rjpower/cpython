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




@unittest.skipUnless(
    sys.platform == "linux" and platform.machine() == "x86_64" and
    not sysconfig.get_config_var("Py_GIL_DISABLED") and
    not sysconfig.get_config_var("Py_STATS") and
    not sysconfig.get_config_var("_Py_TAIL_CALL_INTERP"),
    "natural baseline requires Linux x86-64 GIL/default evaluator",
)
class TestNaturalMethods(unittest.TestCase):
    def info(self, function):
        return _testinternalcapi.jit_method_info(function)

    def test_call_heat_and_attribute_semantics(self):
        ledger = []

        class Value:
            def __init__(self):
                self._x = 0

            @property
            def x(self):
                return self._x

            @x.setter
            def x(self, value):
                ledger.append(value)
                self._x = value

        def update(obj, value):
            obj.x = value
            return obj.x + 1

        obj = Value()
        self.assertEqual(update(obj, 0), 1)
        self.assertFalse(self.info(update)["active"])
        for value in range(100):
            self.assertEqual(update(obj, value), value + 1)
        self.assertEqual(ledger, [0] + list(range(100)))
        info = self.info(update)
        self.assertTrue(info["active"])
        self.assertTrue(info["entry_seen"])
        self.assertEqual(info["exits"], 0)

    def test_periodic_loop_iteration_and_containers(self):
        def work(n):
            values = []
            mapping = {}
            for i in range(n):
                values.append(i + 1)
                mapping[i] = i + 2
            return len(values) + len(mapping)

        self.assertEqual(work(700), 1400)
        info = self.info(work)
        self.assertTrue(info["active"])
        self.assertTrue(info["entry_seen"])
        self.assertEqual(info["exits"], 0)
        self.assertEqual(work(0), 0)
        self.assertEqual(work(3), 6)

    def test_keyword_call_and_closure_continuation(self):
        def outer(bias):
            def callee(value, *, extra):
                return value + extra

            def work(n):
                total = bias
                i = 0
                while i < n:
                    total = total + callee(i, extra=1)
                    i = i + 1
                return total
            return work

        work = outer(7)
        self.assertEqual(work(700), 7 + 700 * 701 // 2)
        self.assertEqual(work(3), 13)
        info = self.info(work)
        self.assertTrue(info["active"])
        self.assertTrue(info["entry_seen"])
        self.assertTrue(info["return_resume_seen"])
        self.assertEqual(info["exits"], 0)

    def test_unsupported_cache_and_unrelated_method(self):
        def unsupported(value):
            try:
                return 1 // value
            except ZeroDivisionError:
                return -1

        def supported(value):
            return value + 2

        for i in range(100):
            self.assertEqual(unsupported(0), -1)
            self.assertEqual(supported(i), i + 2)
        self.assertEqual(self.info(unsupported)["coverage"], -1)
        self.assertFalse(self.info(unsupported)["active"])
        self.assertTrue(self.info(supported)["entry_seen"])

    def test_generator_creation_return_and_suspension(self):
        def values(n):
            yield n
            yield n + 1

        def drive(n):
            total = 0
            i = 0
            while i < n:
                iterator = values(i)
                total = total + next(iterator)
                total = total + next(iterator)
                i = i + 1
            return total

        self.assertEqual(drive(700), 700 * 700)
        info = self.info(drive)
        self.assertTrue(info["entry_seen"])
        self.assertTrue(info["return_resume_seen"])
        self.assertFalse(self.info(values)["active"])
        self.assertEqual(drive(2), 4)

    def test_target_replacement_and_exception_frame_reuse(self):
        def add(value):
            return value + 1

        def subtract(value):
            return value - 1

        def invoke(function, value):
            return function(value)

        for i in range(160):
            self.assertEqual(invoke(add, i), i + 1)
        for i in range(160):
            self.assertEqual(invoke(subtract, i), i - 1)
        self.assertTrue(self.info(add)["entry_seen"])
        self.assertTrue(self.info(subtract)["entry_seen"])

        def fail(value):
            return 1 // value

        def caller(value):
            return fail(value) + 3

        for i in range(160):
            self.assertEqual(caller(1), 4)
        for i in range(8):
            with self.assertRaises(ZeroDivisionError):
                caller(0)
            self.assertEqual(caller(1), 4)
        self.assertTrue(self.info(caller)["return_resume_seen"])

    def test_exec_only_words_rejected_by_constructor_and_marshal(self):
        import marshal
        import struct

        def sample(value):
            return value + 1

        original = sample.__code__.co_code
        for handler in (302, 303, 304):
            replacement = struct.pack("<H", handler) + original[2:]
            with self.assertRaisesRegex(ValueError, "execution-only"):
                sample.__code__.replace(co_code=replacement)
            blob = marshal.dumps(sample.__code__)
            self.assertEqual(blob.count(original), 1)
            offset = blob.index(original)
            corrupted = blob[:offset] + replacement + blob[offset + len(original):]
            with self.assertRaisesRegex(ValueError, "execution-only"):
                marshal.loads(corrupted)

    def test_observer_off_fresh_call_and_same_loop_rearm(self):
        def leaf(value):
            return value + 1

        for i in range(100):
            self.assertEqual(leaf(i), i + 1)
        self.assertTrue(self.info(leaf)["entry_seen"])
        sys.setprofile(lambda *args: None)
        try:
            for i in range(8):
                self.assertEqual(leaf(i), i + 1)
        finally:
            sys.setprofile(None)

        def fresh_caller(n):
            total = 0
            i = 0
            while i < n:
                total = total + leaf(i)
                i = i + 1
            return total

        self.assertEqual(fresh_caller(700), 700 * 701 // 2)
        self.assertTrue(self.info(leaf)["entry_seen"])

        def toggle(i):
            if i == 400:
                sys.setprofile(lambda *args: None)
            if i == 450:
                sys.setprofile(None)
            return 1

        def loop(n, callback):
            i = 0
            total = 0
            while i < n:
                total = total + callback(i)
                i = i + 1
            return total

        try:
            self.assertEqual(loop(1200, toggle), 1200)
        finally:
            sys.setprofile(None)
        self.assertTrue(self.info(loop)["active"])
        self.assertTrue(self.info(loop)["entry_seen"])
        self.assertGreaterEqual(self.info(loop)["compile_attempts"], 2)

    def test_pending_capacity_preserves_older_callers(self):
        def recurse(n):
            if n:
                return recurse(n - 1) + 1
            return 0

        # The second call is wholly hot and exceeds the64 pending-record bound.
        self.assertEqual(recurse(180), 180)
        self.assertEqual(recurse(180), 180)
        self.assertEqual(recurse(2), 2)
        self.assertTrue(self.info(recurse)["entry_seen"])
        self.assertTrue(self.info(recurse)["return_resume_seen"])

    def test_actual_coroutine_creation_and_interpreted_suspension(self):
        import runpy
        from pathlib import Path
        path = Path(__file__).resolve().parents[2] / "boil/bench/internal/kernels/coroutines.py"
        fibonacci = runpy.run_path(str(path))["fibonacci"]

        def create(n):
            return fibonacci(n)

        def drain(coroutine):
            try:
                coroutine.send(None)
            except StopIteration as exc:
                return exc.value
            self.fail("fibonacci unexpectedly suspended externally")

        for i in range(100):
            create(2).close()
        self.assertEqual(drain(create(5)), 5)
        self.assertTrue(self.info(create)["entry_seen"])
        # create's only CALL is generator/coroutine creation: this is the
        # RETURN_GENERATOR continuation, not an ordinary callee RETURN_VALUE.
        self.assertTrue(self.info(create)["return_resume_seen"])
        self.assertFalse(self.info(fibonacci)["active"])

        def work(n):
            return drain(fibonacci(n)) + 1

        for i in range(100):
            self.assertEqual(work(5), 6)
        self.assertTrue(self.info(work)["entry_seen"])
        self.assertTrue(self.info(work)["return_resume_seen"])

    def test_visible_store_precedes_displaced_owner_finalizer(self):
        ledger = []

        class Object:
            pass

        class Owned:
            def __del__(self):
                ledger.append(obj.x)

        def update(value, number):
            value.x = number + 1
            return value.x

        obj = Object()
        obj.x = 0
        for i in range(100):
            self.assertEqual(update(obj, i), i + 1)
        obj.x = Owned()
        self.assertEqual(update(obj, 700), 701)
        self.assertEqual(ledger, [701])
        self.assertTrue(self.info(update)["entry_seen"])

    def test_bootstrap_resource_failure_obeys_eligible_cooldown(self):
        def driver(n):
            def leaf(value):
                return value + 1
            i = 0
            total = 0
            while i < n:
                total = total + leaf(i)
                i = i + 1
            return leaf, total

        result, used, remaining, blocked = _testinternalcapi.jit_method_cold_call(
            driver, (7000,), None, 0, 1)
        leaf, total = result
        self.assertTrue(used)
        self.assertEqual(total, 7000 * 7001 // 2)
        self.assertEqual(blocked, 4096)
        self.assertEqual(remaining, 0)
        self.assertTrue(self.info(leaf)["entry_seen"])
        self.assertEqual(self.info(leaf)["compile_attempts"], 1)

    def test_cold_call_owns_original_operand_and_retries_observers(self):
        events = []

        def replacement():
            events.append("new")
            return 2

        def driver(n):
            # No wrapper argument or other external owner retains this
            # function when the allocation callback replaces its local.
            def old():
                events.append("old")
                return 1
            function = old
            del old
            i = 0
            total = 0
            while i < n:
                total = total + function()
                i = i + 1
            return total

        def callback():
            frame = sys._getframe(1)
            self.assertIs(frame.f_code, driver.__code__)
            events.append("hook")
            frame.f_locals["function"] = replacement
            sys.setprofile(lambda *args: None)

        try:
            result, used, remaining, blocked = _testinternalcapi.jit_method_cold_call(
                driver, (100,), callback, 0, 0)
        finally:
            sys.setprofile(None)
        self.assertTrue(used)
        hook = events.index("hook")
        self.assertEqual(events[hook + 1], "old")
        self.assertTrue(all(event == "new" for event in events[hook + 2:]))
        self.assertEqual(result, events.count("old") + 2 * events.count("new"))

    def test_flow_failure_and_callback_error_restore_request(self):
        def leaf(value):
            return value + 1

        def driver(n, callback):
            total = 0
            i = 0
            while i < n:
                total = total + callback(i)
                i = i + 1
            return total

        result, used, remaining, blocked = _testinternalcapi.jit_method_cold_call(
            driver, (7000, leaf), None, 1, 1)
        self.assertTrue(used)
        self.assertEqual(result, 7000 * 7001 // 2)
        self.assertEqual(blocked, 4096)
        self.assertEqual(remaining, 0)
        self.assertTrue(self.info(driver)["return_resume_seen"])

        def fresh(n):
            def target(value):
                return value + 1
            total = 0
            for i in range(n):
                total = total + target(i)
            return total

        def fail():
            raise RuntimeError("cold callback")

        with self.assertRaisesRegex(RuntimeError, "cold callback"):
            _testinternalcapi.jit_method_cold_call(fresh, (100,), fail, 0, 0)
        self.assertEqual(fresh(100), 5050)

        observations = []

        def reenter():
            def other(value):
                return value + 1
            for i in range(160):
                other(i)
            observations.append("finalized")

        class Finalizer:
            def __del__(self):
                unconsumed(True)

        def unconsumed(reentrant=False):
            if reentrant:
                reenter()
            else:
                marker = Finalizer()
            return 7

        result, used, remaining, blocked = _testinternalcapi.jit_method_cold_call(
            unconsumed, (), lambda: self.fail("unrelated frame consumed request"), 0, 0)
        self.assertEqual(result, 7)
        self.assertFalse(used)
        self.assertEqual(observations, ["finalized"])
        # This checks noninheritance/retirement. It does not assert that a
        # particular frame address was reused during finalizer execution.


if __name__ == "__main__":
    unittest.main()
