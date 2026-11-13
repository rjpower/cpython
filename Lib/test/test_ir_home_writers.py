"""Checked home publications and consuming helper callback boundaries."""

import gc
import sys
import types
import unittest

import _testinternalcapi
from test.test_ir_call_roles import routes


class HomeWriterTests(unittest.TestCase):
    def test_global_publication_preserves_callback_binding_writes(self):
        payload = object()

        class Builtins:
            def __getitem__(self, name):
                if name != "dynamic":
                    raise KeyError(name)
                frame = sys._getframe(1)
                frame.f_locals["current"] = 42
                gc.collect()
                return payload

        def read():
            current = 17
            value = dynamic
            return value, current

        self.assertTrue(any(row[0] == "LOAD_GLOBAL"
                            for row in _testinternalcapi.get_ir(read.__code__)))
        for code in routes(read.__code__):
            function = types.FunctionType(code, {"__builtins__": Builtins()})
            value, current = function()
            self.assertIs(value, payload)
            self.assertEqual(current, 42)

    def test_map_builder_keeps_owners_during_hash_callback(self):
        class Key:
            def __hash__(self):
                frame = sys._getframe(1)
                frame.f_locals["current"] = 42
                gc.collect()
                return 17

        def build(key, value):
            current = 17
            result = {key: value}
            return result, current

        self.assertTrue(any(row[0] == "BUILD_MAP"
                            for row in _testinternalcapi.get_ir(build.__code__)))
        payload = object()
        for code in routes(build.__code__):
            function = types.FunctionType(code, globals())
            result, current = function(Key(), payload)
            self.assertEqual(current, 42)
            self.assertEqual(list(result.values()), [payload])

    def test_string_builder_consumes_formatted_pieces(self):
        def build(first, second):
            return f"before:{first}:between:{second}:after"

        self.assertTrue(any(row[0] == "BUILD_STRING"
                            for row in _testinternalcapi.get_ir(build.__code__)))
        for code in routes(build.__code__):
            function = types.FunctionType(code, globals())
            self.assertEqual(function("one", "two"), "before:one:between:two:after")

    def test_special_pair_survives_descriptor_callback(self):
        class Enter:
            def __get__(self, instance, owner):
                frame = sys._getframe(1)
                frame.f_locals["current"] = 42
                gc.collect()
                return lambda: None

        class Owner:
            __enter__ = Enter()

            def __exit__(self, *exc):
                return False

        def invoke(factory):
            current = 17
            with factory():
                pass
            return current

        self.assertTrue(any(row[0] == "LOAD_SPECIAL"
                            for row in _testinternalcapi.get_ir(invoke.__code__)))
        for code in routes(invoke.__code__):
            function = types.FunctionType(code, globals())
            self.assertEqual(function(Owner), 42)


if __name__ == "__main__":
    unittest.main()
