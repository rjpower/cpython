"""Consumed super-lookup inputs are not implicit CALL_EX self owners."""

import marshal
import sys
import types
import unittest

import _testinternalcapi
from test.test_ir_admission import construct
from test.test_ir_transfer_admission import encoded


def routes(code):
    return code, construct(code), code.replace(), marshal.loads(marshal.dumps(code))


class SuperCallTransportTests(unittest.TestCase):
    def make_method(self):
        class Base:
            def invoke(self, *args, **kwargs):
                return args, kwargs

        class Child(Base):
            def invoke(self, *args, **kwargs):
                return super().invoke(*args, **kwargs)

        return Child(), Child.invoke

    def test_super_attribute_consumes_the_optional_self_home(self):
        instance, method = self.make_method()
        rows = _testinternalcapi.get_ir(method.__code__)
        self.assertTrue(any(row[0] == "LOAD_SUPER_ATTR" for row in rows))
        self.assertTrue(any(row[0] == "CALL_EX_NO_SELF_KW" for row in rows))
        for code in routes(method.__code__):
            function = types.FunctionType(code, method.__globals__, closure=method.__closure__)
            with self.subTest(code=code):
                self.assertEqual(function(instance, 17, 18, answer=42),
                                 ((17, 18), {"answer": 42}))

    def test_present_optional_self_cannot_inherit_the_absent_self_proof(self):
        _, method = self.make_method()
        code = method.__code__
        words = [tuple(row) for row in _testinternalcapi.get_ir(code)]
        site = next(i for i, row in enumerate(words) if row[0] == "LOAD_SUPER_ATTR")
        name, a, b, feedback, arg = words[site]
        self.assertFalse(arg & 1)
        words[site] = name, a, b, feedback, arg | 1
        changed = encoded(words)
        for route in (construct, lambda co, **kw: co.replace(**kw)):
            with self.subTest(route=route):
                with self.assertRaisesRegex(ValueError, "virtual call input"):
                    route(code, co_code=changed)
        payload = marshal.dumps(code)
        self.assertEqual(payload.count(code.co_code), 1)
        with self.assertRaisesRegex(ValueError, "virtual call input"):
            marshal.loads(payload.replace(code.co_code, changed, 1))

    def test_lookup_error_and_observer_local_mutation_keep_existing_routes(self):
        instance, method = self.make_method()
        events = []
        mon, tool = sys.monitoring, 4

        def observe(code, offset, callable, arg):
            if code is method.__code__:
                frame = sys._getframe(1)
                frame.f_locals['kwargs'] = {'changed': 42}
                with self.assertRaises(ValueError):
                    frame.f_lineno = frame.f_lineno
                events.append(callable)

        mon.use_tool_id(tool, 'super-call-transport')
        try:
            mon.register_callback(tool, mon.events.CALL, observe)
            mon.set_local_events(tool, method.__code__, mon.events.CALL)
            self.assertEqual(method(instance, 17, answer=18),
                             ((17,), {'changed': 42}))
            self.assertTrue(events)
        finally:
            mon.set_local_events(tool, method.__code__, 0)
            mon.register_callback(tool, mon.events.CALL, None)
            mon.free_tool_id(tool)

        class Missing:
            def invoke(self, *args, **kwargs):
                return super().unknown(*args, **kwargs)

        for code in routes(Missing.invoke.__code__):
            function = types.FunctionType(code, Missing.invoke.__globals__,
                                          closure=Missing.invoke.__closure__)
            with self.assertRaises(AttributeError):
                function(Missing())


if __name__ == '__main__':
    unittest.main()
