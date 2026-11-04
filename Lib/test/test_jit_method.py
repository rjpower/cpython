"""Semantic-helper and whole-method native execution contracts."""
import platform
import sys
import sysconfig
import types
import unittest

from test.support import import_helper

_testinternalcapi = import_helper.import_module("_testinternalcapi")


def setUpModule():
    # Hot-call and backedge method admission is off by default.
    previous = _testinternalcapi.jit_method_natural(True)
    unittest.addModuleCleanup(_testinternalcapi.jit_method_natural, previous)


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
        calls = []

        def other(value):
            calls.append(value)
            return value + 2

        def unsupported(callback, value):
            args = (value,)
            return callback(*args)

        self.compile(other)
        self.compile(unsupported, complete=False)
        self.assertEqual(unsupported(other, 7), 9)
        self.assertEqual(calls, [7])
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


@unittest.skipUnless(
    sys.platform == "linux" and platform.machine() == "x86_64" and
    not sysconfig.get_config_var("Py_GIL_DISABLED") and
    not sysconfig.get_config_var("Py_STATS") and
    not sysconfig.get_config_var("_Py_TAIL_CALL_INTERP"),
    "observed loops require Linux x86-64 GIL/default evaluator",
)
class TestObservedLoops(unittest.TestCase):
    def info(self, function):
        return _testinternalcapi.jit_method_info(function)

    def optimized(self, function):
        info = self.info(function)
        self.assertTrue(info["active"], info)
        trace = info["trace"]
        self.assertTrue(trace["optimized"], info)
        self.assertTrue(trace["entry_seen"], info)
        self.assertTrue(trace["scalar_seen"], info)
        self.assertTrue(trace["backedge_seen"], info)
        self.assertGreater(trace["events"], 0)
        self.assertGreater(trace["nodes"], 0)
        self.assertLessEqual(trace["scratch"], 64)
        self.assertEqual(len(trace["path"]), trace["events"])
        self.assertEqual(len(info["offsets"]), info["words"])
        self.assertGreater(len(info["code"]), 0)
        return info

    def test_natural_recording_and_native_scalar_loop(self):
        def work(n, seed):
            i = 0
            total = seed
            while i < n:
                total = total + i * 3 + (i + 7)
                i = i + 1
            return total

        for n, seed in ((2400, 3), (31, -7), (1200, 19)):
            self.assertEqual(work(n, seed), seed + 2 * n * (n - 1) + 7 * n)
        info = self.optimized(work)
        # Every home holds an exact int in int32 range, whose canonical form
        # is inline (pyvalue.md section 1.5), so no import reads a box.
        self.assertEqual(info["trace"]["boxed_imports"], 0)
        self.assertTrue(_testinternalcapi.jit_method_record_index())
        print("JIT_OBSERVED_LOOP_WITNESS", info)

    def test_python_and_c_call_continuations(self):
        ledger = []

        def callback(i):
            ledger.append(i)
            return i + 2

        def work(n, cb):
            i = 0
            total = 0
            while i < n:
                total = total + cb(i)
                total = total + abs(i - 3)
                i = i + 1
            return total

        self.assertEqual(work(2400, callback), sum(i + 2 + abs(i - 3) for i in range(2400)))
        self.assertEqual(ledger, list(range(2400)))
        info = self.optimized(work)
        self.assertGreater(info["trace"]["python_calls"], 0)
        self.assertTrue(info["trace"]["return_seen"], info)
        self.assertTrue(info["return_resume_seen"])
        self.assertTrue(info["c_call_seen"])

    def test_overflow_branch_type_and_zero_iteration_exits(self):
        def work(n, seed, cut):
            i = 0
            total = seed
            while i < n:
                if i < cut:
                    total = total + 3
                else:
                    total = total - 2
                i = i + 1
            return total

        self.assertEqual(work(2400, 1, 3000), 7201)
        self.optimized(work)
        for n, seed, cut in ((0, 7, 0), (30, 2**31 - 2, 7), (33, 1.5, 12), (49, -2**31, 3)):
            self.assertEqual(work(n, seed, cut), seed + 3 * min(n, cut) - 2 * max(0, n - cut))

        # Returning before the nominated loop, or running a different loop,
        # must settle temporary recorder hooks without a new selection policy.
        def alternate(n, first):
            i = 0
            while i < first:
                i = i + 1
            j = 0
            while j < n:
                j = j + 1
            return j

        for _ in range(80):
            self.assertEqual(alternate(0, 0), 0)
        terminal_info = self.info(alternate)
        self.assertFalse(terminal_info.get("trace", {}).get("recording", False))
        fresh = types.FunctionType(alternate.__code__.replace(), globals())
        self.assertEqual(fresh(3000, 0), 3000)
        alternate_info = self.info(fresh)
        self.assertFalse(alternate_info.get("trace", {}).get("optimized", False))
        self.assertFalse(alternate_info.get("trace", {}).get("recording", False))

        ledger = []

        def callback(value):
            ledger.append(value)
            return value + 2

        def short(n, cb):
            i = 0
            total = 0
            while i < n:
                total = total + cb(i)
                i = i + 1
            return total

        self.assertTrue(_testinternalcapi.jit_method_compile(short))
        initial = self.info(short)
        self.assertTrue(initial["trace"]["recording"])
        self.assertEqual(short(0, callback), 0)
        self.assertEqual(short(1, callback), 2)
        settled = self.info(short)
        self.assertTrue(settled["active"])
        self.assertTrue(settled["entry_seen"])
        self.assertTrue(settled["return_resume_seen"])
        self.assertFalse(settled["trace"]["recording"])
        self.assertNotEqual(settled["address"], initial["address"])
        self.assertEqual(settled["compile_attempts"], initial["compile_attempts"] + 1)
        for n in (1, 3, 7):
            self.assertEqual(short(n, callback), n * (n - 1) // 2 + 2 * n)
        final = self.info(short)
        self.assertEqual(final["address"], settled["address"])
        self.assertEqual(final["code"], settled["code"])
        self.assertEqual(final["compile_attempts"], settled["compile_attempts"])
        self.assertEqual(ledger, [0] + [0] + list(range(3)) + list(range(7)))

    def test_boxed_identity_and_owner_release_reentry(self):
        imported = int("1000000")
        ledger = []

        def observe(value):
            ledger.append(value is imported)
            return 1

        def work(n, original, cb):
            i = 0
            total = 0
            while i < n:
                total = total + cb(original)
                i = i + 1
            return total

        self.assertEqual(work(2400, imported, observe), 2400)
        self.assertEqual(ledger, [True] * 2400)
        self.optimized(work)
        released = []

        class Owner:
            def __add__(self, other):
                return 11

            def __del__(self):
                released.append(sys._getframe(1).f_locals.get("value"))

        def replace(n, factory):
            value = factory()
            i = 0
            while i < n:
                value = value + 1
                i = i + 1
            return value

        self.assertEqual(replace(2400, lambda: 1000), 3400)
        self.optimized(replace)
        # Owner declines the exact-int import and executes this same Unit's
        # native baseline helper. It does not enter scalar TraceCommit.
        self.assertEqual(replace(1, Owner), 11)
        self.assertEqual(released, [11])
        self.assertTrue(self.info(replace)["active"])

    def test_nested_recording_and_exception_retirement(self):
        ledger = []

        def inner(n):
            i = 0
            total = 0
            while i < n:
                total = total + i
                i = i + 1
            return total

        def cb(i):
            if i == 1100:
                raise ValueError("recorded callback")
            ledger.append(i)
            return inner(2)

        def outer(n, callback):
            i = 0
            total = 0
            while i < n:
                total = total + callback(i)
                i = i + 1
            return total

        with self.assertRaisesRegex(ValueError, "recorded callback"):
            outer(2400, cb)
        self.assertEqual(ledger, list(range(1100)))
        self.assertEqual(outer(2400, lambda i: 2), 4800)
        self.assertEqual(inner(2400), 2400 * 2399 // 2)
        self.optimized(inner)

    def test_common_none_return_loop(self):
        ledger = []

        def work(n, out):
            i = 0
            total = 0
            while i < n:
                total = total + i
                i = i + 1
            out.append(total)

        self.assertIsNone(work(2400, ledger))
        self.assertEqual(ledger, [2400 * 2399 // 2])
        self.optimized(work)

    def test_raise_forms_and_invalid_exception(self):
        def reraiser():
            raise

        def cause():
            raise RuntimeError("outer") from ValueError("inner")

        def invalid():
            raise 17

        def raised():
            raise ValueError("single")

        for function, exception, message in ((reraiser, ValueError, "first"),
                                              (cause, RuntimeError, "outer"),
                                              (invalid, TypeError, "exceptions must derive"),
                                              (raised, ValueError, "single")):
            self.assertTrue(_testinternalcapi.jit_method_compile(function))
            for _ in range(80):
                if function is reraiser:
                    try:
                        raise ValueError("first")
                    except ValueError as original:
                        with self.assertRaisesRegex(ValueError, "first") as caught:
                            function()
                        self.assertIs(caught.exception, original)
                else:
                    with self.assertRaisesRegex(exception, message) as caught:
                        function()
            if function is cause:
                self.assertIsInstance(caught.exception.__cause__, ValueError)
            self.assertTrue(self.info(function)["entry_seen"])

    def test_raise_constructor_finalizer_and_observers(self):
        ledger = []

        class Error(Exception):
            def __init__(self):
                ledger.append("construct")

        def raised():
            raise Error

        self.assertTrue(_testinternalcapi.jit_method_compile(raised))
        for _ in range(80):
            with self.assertRaises(Error):
                raised()
        self.assertEqual(ledger, ["construct"] * 80)

        class Operand:
            def __del__(self):
                ledger.append("release")

        def invalid():
            raise Operand()

        self.assertTrue(_testinternalcapi.jit_method_compile(invalid))
        with self.assertRaises(TypeError):
            invalid()
        self.assertEqual(ledger[-1], "release")

        def profile(frame, event, arg):
            return profile

        sys.setprofile(profile)
        try:
            with self.assertRaises(Error):
                raised()
        finally:
            sys.setprofile(None)
        self.assertEqual(ledger[-1], "construct")

        observations = []

        class Number:
            arm = False

            def __abs__(self):
                observations.append(1)
                if self.arm:
                    self.arm = False
                    sys.setprofile(profile)
                return 1

        def loop(n, value):
            i = 0
            total = 0
            while i < n:
                total = total + abs(value)
                i = i + 1
            return total

        value = Number()
        self.assertEqual(loop(2400, value), 2400)
        self.optimized(loop)
        value.arm = True
        try:
            self.assertEqual(loop(31, value), 31)
        finally:
            sys.setprofile(None)
        self.assertEqual(len(observations), 2431)


if __name__ == "__main__":
    unittest.main()
