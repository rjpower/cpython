"""Real call homes across preparation, native cleanup and Python binding."""

import sys
import unittest

from test.support import import_helper

_testinternalcapi = import_helper.import_module("_testinternalcapi")
_testclinic = import_helper.import_module("_testclinic")


def call_state(code):
    state = _testinternalcapi.native_scope_call_state(code)
    handler, word, nlocals, base, count, destination, values, present, strong = state
    if present & ~strong:
        raise AssertionError("frame contains a non-owning live home")
    if not (base >= nlocals and base + count <= len(values)):
        raise AssertionError("invalid actual call window")
    return state


class NativeCallTransactionTests(unittest.TestCase):
    def check_alias(self, expanded):
        events, states = [], []
        kwargs = {}

        class Value:
            def visit(self):
                kwargs.clear()
                states.append(call_state(driver.__code__))
                events.append("visit")
                self.nested = _testclinic.OwnerEcho(2147483647)

            def __repr__(self):
                if self.nested != 2147483647:
                    raise AssertionError("selected parser input lost nested result")
                events.append("repr")
                return "alive"

            def __del__(self):
                events.append("del")

        kwargs["value"] = Value()
        context = (_testclinic.OwnerProbe, kwargs)
        if expanded:
            def driver():
                return _testclinic.OwnerAlias(*(context,))
        else:
            def driver():
                return _testclinic.OwnerAlias(context)

        self.assertEqual(driver(), "alive")
        self.assertEqual(events, ["visit", "repr", "del"])
        self.assertEqual(kwargs, {})
        state, = states
        _, _, _, base, count, _, values, present, _ = state
        self.assertTrue(present & (1 << base))
        self.assertIs(values[base], _testclinic.OwnerAlias)
        if expanded:
            self.assertEqual(count, 4)
            self.assertIs(values[base + 2][0], context)
        else:
            self.assertEqual(count, 3)
            self.assertIs(values[base + 2], context)

    def test_real_constructor_alias_ordinary(self):
        self.check_alias(False)

    def test_real_constructor_alias_expanded(self):
        self.check_alias(True)

    def test_warm_ordinary_and_keywords(self):
        states = []

        class Value:
            def visit(self):
                states.append(call_state(driver.__code__))

            def __repr__(self):
                return "value"

        value = Value()
        for keyword in (False, True):
            if keyword:
                def driver():
                    return _testclinic.OwnerProbe(value=value)
            else:
                def driver():
                    return _testclinic.OwnerProbe(value)
            for _ in range(200):
                self.assertEqual(driver(), "value")
            self.assertEqual(len(states), 200)
            state = states[-1]
            self.assertIs(state[6][state[3]], _testclinic.OwnerProbe)
            self.assertIs(state[6][state[3] + 2], value)
            names = [word[0] for word in
                     _testinternalcapi.get_ir(driver.__code__, True)]
            self.assertIn("CALL_KW_NON_PY" if keyword else "CALL_NON_PY_GENERAL", names)
            states.clear()

    def test_iterable_retirement_sees_final_tuple(self):
        states, events = [], []
        context = (_testclinic.OwnerEcho, {"value": 2147483647})

        class Arguments:
            def __iter__(self):
                events.append("iter")
                return iter((context,))

            def __del__(self):
                states.append(call_state(driver.__code__))
                events.append("retire")

        pending = [Arguments()]

        def driver():
            return _testclinic.OwnerAlias(*pending.pop())

        self.assertEqual(driver(), 2147483647)
        self.assertEqual(events, ["iter", "retire"])
        state, = states
        self.assertEqual(state[6][state[3] + 2], (context,))

    def test_folded_destination_published_before_retirement(self):
        states = []

        class Displaced:
            def __del__(self):
                states.append(call_state(driver.__code__))

        class Value:
            def visit(self):
                # Writes the callback-current local, rather than the value
                # evaluated by the caller before this native body began.
                sys._getframe(1).f_locals["result"] = Displaced()

            def __repr__(self):
                return "published"

        def driver(value):
            result = None
            result = _testclinic.OwnerProbe(value)
            return result

        self.assertEqual(driver(Value()), "published")
        state, = states
        _, _, nlocals, base, count, destination, values, present, _ = state
        self.assertLess(destination, nlocals)
        self.assertEqual(values[destination], "published")
        self.assertEqual(present & (((1 << count) - 1) << base), 0)

    def test_monitor_change_does_not_replay_call(self):
        states = []
        tool = 2

        def driver():
            return _testclinic.OwnerEcho(2147483647)

        def monitor(code, offset, function, first):
            if code is driver.__code__ and function is _testclinic.OwnerEcho:
                states.append(call_state(code))
                sys.monitoring.set_local_events(tool, code, 0)

        sys.monitoring.use_tool_id(tool, "native-call-transaction")
        try:
            sys.monitoring.register_callback(tool, sys.monitoring.events.CALL, monitor)
            sys.monitoring.set_local_events(tool, driver.__code__, sys.monitoring.events.CALL)
            self.assertEqual(driver(), 2147483647)
            self.assertEqual(len(states), 1)
            state, = states
            self.assertEqual(state[6][state[3] + 2], 2147483647)
        finally:
            sys.monitoring.set_local_events(tool, driver.__code__, 0)
            sys.monitoring.register_callback(tool, sys.monitoring.events.CALL, None)
            sys.monitoring.free_tool_id(tool)

    def test_python_binding_uses_one_code_snapshot(self):
        states = []

        def target(value):
            return "selected", value

        def replacement(*args, **kwargs):
            return "replacement", args, kwargs

        selected = target.__code__

        class Name(str):
            __hash__ = str.__hash__

            def __eq__(self, other):
                states.append(call_state(driver.__code__))
                target.__code__ = replacement.__code__
                return str.__eq__(self, other)

        value = object()
        kwargs = {Name("value"): value}

        def driver():
            return target(**kwargs)

        try:
            self.assertEqual(driver(), ("selected", value))
            self.assertTrue(states)
            for state in states:
                self.assertIs(state[6][state[3]], target)
                observed = state[6][state[3] + 3]
                self.assertIs(next(iter(observed)), next(iter(kwargs)))
                self.assertIs(next(iter(observed.values())), value)
        finally:
            target.__code__ = selected

    def test_python_binding_preserves_primary_exception(self):
        marker = ValueError("binding callback")
        states, bodies = [], []

        def target(value):
            bodies.append(value)

        class Name(str):
            __hash__ = str.__hash__

            def __eq__(self, other):
                states.append(call_state(driver.__code__))
                raise marker

        kwargs = {Name("value"): object()}

        def driver():
            return target(**kwargs)

        with self.assertRaises(ValueError) as caught:
            driver()
        self.assertIs(caught.exception, marker)
        self.assertTrue(states)
        self.assertEqual(bodies, [])


if __name__ == "__main__":
    unittest.main()
