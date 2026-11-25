"""Ownership of a generic call's operands across preparation, a native callee,
Python binding and monitoring.

A call reads its operands from their homes.  A native callee borrows them, so
they stay in their homes (visible to the collector and to the callee) until
the result is published.  A Python callee takes them from their homes before
the binding that can fail consumes them.  What preparation produces (the
tuple of a CALL_FUNCTION_EX) is held by the call, not by a home.
"""

import gc
import sys
import types
import unittest

from test.support import import_helper
from test.test_ir_call_roles import call_rows, routes

_testinternalcapi = import_helper.import_module("_testinternalcapi")
_testclinic = import_helper.import_module("_testclinic")


def call_state(code):
    state = _testinternalcapi.native_scope_call_state(code)
    handler, word, nlocals, base, count, destination, values, present, strong = state
    if present & ~strong:
        raise AssertionError("frame contains a non-owning live home")
    # A local operand has no home in the window: only the callable's is known
    if not (nlocals <= base < len(values)):
        raise AssertionError("invalid actual call window")
    return state


class CallOperandOwnershipTests(unittest.TestCase):
    def test_exact_tuple_preparation_all_routes(self):
        def plain(target, arguments):
            return target(*arguments)

        def keywords(target, arguments):
            return target(*arguments, label="kept")

        def target(*arguments, **keywords):
            return arguments, keywords

        payload = object()
        cases = ((plain, {}), (keywords, {"label": "kept"}))
        for template, expected_keywords in cases:
            for code in routes(template.__code__):
                self.assertTrue(call_rows(code)[0][0].startswith("CALL_FUNCTION_EX"))
                invoke = types.FunctionType(code, {})
                for arguments in ((), (payload, 1, -0.0)):
                    with self.subTest(keywords=expected_keywords,
                                      arguments=arguments):
                        values, names = invoke(target, arguments)
                        self.assertEqual(values, arguments)
                        self.assertEqual(names, expected_keywords)
                        if arguments:
                            self.assertIs(values[0], payload)

    def test_nonexact_tuple_preparation_keeps_evaluated_inputs(self):
        def template(target, arguments):
            return target(*arguments)

        def target(*arguments):
            return "original", arguments

        def replacement(*arguments):
            return "replacement", arguments

        payload = object()
        for code in routes(template.__code__):
            events = []

            def convert():
                frame = sys._getframe(1).f_back
                self.assertIs(frame.f_code, code)
                frame.f_locals["target"] = replacement
                frame.f_locals["arguments"] = ()
                gc.collect()
                events.append("iter")
                return iter((payload,))

            class TupleArguments(tuple):
                def __iter__(self):
                    return convert()

            class Arguments:
                def __iter__(self):
                    return convert()

            invoke = types.FunctionType(code, {})
            for arguments in (TupleArguments(), Arguments()):
                with self.subTest(kind=type(arguments)):
                    self.assertEqual(invoke(target, arguments),
                                     ("original", (payload,)))
            self.assertEqual(events, ["iter", "iter"])

    def test_nonexact_tuple_preparation_errors(self):
        def template(target, arguments):
            return target(*arguments)

        marker = LookupError("tuple subclass iteration")
        events = []

        class Arguments(tuple):
            def __iter__(self):
                events.append("iter")
                gc.collect()
                raise marker

        def target(*arguments):
            self.fail("a failed preparation must not invoke its target")

        all_routes = routes(template.__code__)
        for code in all_routes:
            invoke = types.FunctionType(code, {})
            for arguments in (None, 17, object()):
                with self.subTest(arguments=arguments):
                    with self.assertRaisesRegex(
                            TypeError, r"after \* must be an iterable"):
                        invoke(target, arguments)
            with self.assertRaises(LookupError) as caught:
                invoke(target, Arguments())
            self.assertIs(caught.exception, marker)
            marker.__traceback__ = None
        self.assertEqual(events, ["iter"] * len(all_routes))

    def test_exact_tuple_monitor_observes_evaluated_inputs_once(self):
        def template(target, arguments):
            return target(*arguments)

        payload = object()
        arguments = (payload,)
        tool = sys.monitoring.DEBUGGER_ID
        for code in routes(template.__code__):
            events = []

            def target(value):
                events.append("body")
                return value

            def monitor(current, offset, function, first):
                self.assertIs(current, code)
                self.assertIs(function, target)
                self.assertIs(first, payload)
                frame = sys._getframe(1)
                self.assertIs(frame.f_code, code)
                frame.f_locals["target"] = None
                frame.f_locals["arguments"] = ()
                sys.monitoring.set_local_events(tool, code, 0)
                gc.collect()
                events.append("call")

            sys.monitoring.use_tool_id(tool, "exact-tuple-preparation")
            try:
                sys.monitoring.register_callback(
                    tool, sys.monitoring.events.CALL, monitor)
                sys.monitoring.set_local_events(tool, code, sys.monitoring.events.CALL)
                invoke = types.FunctionType(code, {})
                self.assertIs(invoke(target, arguments), payload)
                self.assertEqual(events, ["call", "body"])
            finally:
                sys.monitoring.set_local_events(tool, code, 0)
                sys.monitoring.register_callback(tool, sys.monitoring.events.CALL, None)
                sys.monitoring.free_tool_id(tool)

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
            self.assertIn("CALL_KW_NON_PY" if keyword else "Call.Generic", names)
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
        # The call holds the final tuple itself, and took the iterable from
        # its home before it closed it
        self.assertNotIsInstance(state[6][state[3] + 2], Arguments)

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

        sys.monitoring.use_tool_id(tool, "native-call-operands")
        try:
            sys.monitoring.register_callback(tool, sys.monitoring.events.CALL, monitor)
            sys.monitoring.set_local_events(tool, driver.__code__, sys.monitoring.events.CALL)
            self.assertEqual(driver(), 2147483647)
            self.assertEqual(len(states), 1)
            state, = states
            # The operands of the instruction: callable, no self, the argument
            self.assertEqual(state[4], 3)
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
            # The operands stay in their homes while the binding runs (the
            # callee holds its own counts), and the call kept one code for
            # the whole binding
            for state in states:
                self.assertEqual(state[4], 4)
                _, _, _, base, _, _, values, present, _ = state
                # The empty positional tuple is a constant in a fixed home
                # below the window; MergeKeywords retires its source, so
                # the merged dict is the window's last operand
                for home in (base, base + 2):
                    self.assertTrue(present & (1 << home))
                self.assertIs(values[base], target)
                self.assertEqual(values[base + 2], kwargs)
                self.assertTrue(any(v == () and present & (1 << i)
                                    for i, v in enumerate(values) if v is not None))
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


    def test_native_callee_sees_operands_in_their_homes(self):
        # The callable and the arguments are borrowed from their homes for
        # the whole native call: the callee and the collector see them there
        states = []
        payload = object()

        class Value:
            def visit(self):
                states.append(call_state(driver.__code__))

            def __repr__(self):
                return "value"

        def make():
            return Value()

        def driver():
            return _testclinic.OwnerProbe(make())

        self.assertEqual(driver(), "value")
        state, = states
        _, _, _, base, count, _, values, present, _ = state
        self.assertEqual(count, 3)
        self.assertIs(values[base], _testclinic.OwnerProbe)
        self.assertIsInstance(values[base + 2], Value)
        self.assertTrue(present & (1 << base))
        self.assertTrue(present & (1 << (base + 2)))

    def test_expanded_method_operand_stays_until_published(self):
        # A bound method without a self operand is its function and its
        # receiver, which the method operand keeps alive: the home holds the
        # method itself during a native callee, and releases it once
        states = []

        class Value:
            def visit(self):
                states.append(call_state(driver.__code__))

            def __repr__(self):
                return "value"

        def method():
            return types.MethodType(_testclinic.OwnerProbe, Value())

        def driver():
            return method()()

        before = sys.getrefcount(Value)
        for _ in range(3):
            self.assertEqual(driver(), "value")
        self.assertEqual(len(states), 3)
        for state in states:
            _, _, _, base, _, _, values, present, _ = state
            self.assertTrue(present & (1 << base))
            self.assertIsInstance(values[base], types.MethodType)
        del state, values
        states.clear()
        gc.collect()
        self.assertEqual(sys.getrefcount(Value), before)

    def test_errors_release_operands_once(self):
        # A call that fails after reading its operands, in a Python callee's
        # binding or in a native callee, leaves each owner released exactly
        # once, for every kind of call and for a bound method
        class Payload:
            pass

        def exact(a):
            return a

        class Holder:
            def method(self, a):
                return a

        def plain(fn, p):
            return fn(p, p)

        def keywords(fn, p):
            return fn(p, extra=p)

        def star(fn, p):
            return fn(*(p, p))

        def star_keywords(fn, p):
            return fn(*(p,), extra=p)

        payload = Payload()
        callables = (exact, Holder().method, len, Holder)
        for template in (plain, keywords, star, star_keywords):
            for code in routes(template.__code__):
                invoke = types.FunctionType(code, {})
                for fn in callables:
                    with self.subTest(route=template.__name__, callee=fn):
                        for _ in range(2):  # the second run is warm
                            before = (sys.getrefcount(payload),
                                      sys.getrefcount(fn))
                            with self.assertRaises(TypeError):
                                invoke(fn, payload)
                            gc.collect()
                            self.assertEqual((sys.getrefcount(payload),
                                              sys.getrefcount(fn)), before)

    def test_raising_event_callback_releases_operands_once(self):
        # A CALL event callback that raises, or that disables itself, leaves
        # the operands owned by their homes: the call neither ran nor leaked
        def template(fn, p):
            return fn(p, key=p)

        def target(first, key):
            return first, key

        payload = object()
        tool = sys.monitoring.DEBUGGER_ID
        for code in routes(template.__code__):
            for raising in (True, False):
                def monitor(current, offset, function, first):
                    if raising:
                        raise KeyError("event")
                    return sys.monitoring.DISABLE

                sys.monitoring.use_tool_id(tool, "call-operand-events")
                try:
                    sys.monitoring.register_callback(
                        tool, sys.monitoring.events.CALL, monitor)
                    sys.monitoring.set_local_events(
                        tool, code, sys.monitoring.events.CALL)
                    invoke = types.FunctionType(code, {})
                    before = sys.getrefcount(payload)
                    if raising:
                        with self.assertRaises(KeyError):
                            invoke(target, payload)
                    else:
                        self.assertEqual(invoke(target, payload),
                                         (payload, payload))
                    gc.collect()
                    self.assertEqual(sys.getrefcount(payload), before)
                finally:
                    sys.monitoring.set_local_events(tool, code, 0)
                    sys.monitoring.register_callback(
                        tool, sys.monitoring.events.CALL, None)
                    sys.monitoring.free_tool_id(tool)


class SpecializedNativeCallTests(unittest.TestCase):
    """The specialized builtin and method-descriptor handlers read their
    operands in place, copy them once to an array that the callee borrows, and
    call the callee's value entry or its object vectorcall."""

    WARM = 40

    def run_site(self, site, *args):
        # Warm the site so that it specializes, then call it once more
        for _ in range(self.WARM):
            site(*args)
        return site(*args)

    def test_callee_rebinding_a_local_operand(self):
        # The callee runs Python that rebinds the caller's local through its
        # frame proxy: the call still holds the value that it read
        seen = []
        armed = []

        class Rebinder:
            def __getattr__(self, name):
                sys._getframe(1).f_locals["target"] = "replaced"
                raise AttributeError(name)

            def __hash__(self):
                if armed:
                    sys._getframe(1).f_locals["key"] = "replaced"
                return 1

            def __eq__(self, other):
                return True

        def has(n):
            target = Rebinder()
            result = hasattr(target, "missing")
            seen.append(target)
            return result

        def get(n):
            key = Rebinder()
            table = {key: 7}
            armed.append(True)
            try:
                result = table.get(key)
            finally:
                armed.clear()
            seen.append(key)
            return result

        self.assertIs(self.run_site(has, 0), False)
        self.assertEqual(self.run_site(get, 0), 7)
        self.assertEqual(seen, ["replaced"] * len(seen))
        self.assertTrue(seen)

    def test_native_forms_with_object_and_value_entries(self):
        # METH_VALUE builtins and methods, an object METH_FASTCALL builtin
        # and method, and a keyword form, from one site shape each
        def builtins_(x, text):
            return (abs(x), len(text), hasattr(text, "upper"), max(x, 3),
                    isinstance(x, int), divmod(x, 3), pow(x, 2))

        def methods(text, table):
            return (text.upper(), text.startswith("a"), table.get("k"),
                    table.get("z", 5), text.find("b"), text.split("b"),
                    text.replace("a", "z", 1), table.keys())

        self.assertEqual(self.run_site(builtins_, -5, "abc"),
                         (5, 3, True, 3, True, (-2, 1), 25))
        self.assertEqual(self.run_site(methods, "abc", {"k": 1}),
                         ("ABC", True, 1, 5, 1, ["a", "c"], "zbc",
                          {"k": 1}.keys()))

    def test_wide_calls(self):
        # More operands than one operand word holds
        def site(text, items):
            return (text.replace("a", "z", 1), max(1, 2, 3, 4, 5, 6),
                    items.index(5, 1, 6), "{} {} {} {}".format(1, 2, 3, 4),
                    text.split(" ", 2))

        self.assertEqual(
            self.run_site(site, "a a a", (1, 2, 3, 4, 5, 6)),
            ("z a a", 6, 4, "1 2 3 4", ["a", "a", "a"]))

    def test_wrong_arity_raises_the_callee_error(self):
        # A site that specialized on one builtin meets a fixed-arity value
        # builtin called with the wrong number of arguments: the handler
        # leaves for the generic call, which raises the vectorcall's error
        def site(target, x, y):
            return target(x, y)

        self.assertEqual(self.run_site(site, divmod, 7, 2), (3, 1))
        with self.assertRaisesRegex(TypeError, "abs.*exactly one argument"):
            site(abs, 1, 2)
        with self.assertRaisesRegex(TypeError, "len.*exactly one argument"):
            site(len, 1, 2)
        self.assertEqual(site(divmod, 9, 4), (2, 1))

    def test_method_arity_and_receiver_type_misses(self):
        def site(receiver, argument):
            return receiver.count(argument)

        self.assertEqual(self.run_site(site, "abca", "a"), 2)
        self.assertEqual(site([1, 1, 2], 1), 2)
        self.assertEqual(site((1, 2, 1), 1), 2)
        with self.assertRaises(TypeError):
            site("abc", 1)

    def test_unbound_method_descriptor_and_null_self(self):
        # The self slot of a call to a class attribute is NULL at run time
        # and the arguments follow it
        def site(table, text):
            return (dict.get(table, "k"), str.upper(text), str.startswith(text, "a"))

        self.assertEqual(self.run_site(site, {"k": 3}, "abc"), (3, "ABC", True))

    def test_native_errors_release_operands(self):
        class Tracked:
            pass

        for name, call in (
            ("builtin", lambda value: hasattr(value)),
            ("method", lambda value: {}.get()),
            ("value method", lambda value: [].index(value, "x")),
        ):
            with self.subTest(name):
                value = Tracked()
                before = sys.getrefcount(value)
                for _ in range(self.WARM):
                    with self.assertRaises(TypeError):
                        call(value)
                gc.collect()
                self.assertEqual(sys.getrefcount(value), before)


if __name__ == "__main__":
    unittest.main()
