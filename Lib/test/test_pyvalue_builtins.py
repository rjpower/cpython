"""Owned object bridges for canonical globals['__builtins__'] values."""

import builtins
import gc
import types
import unittest
import weakref


def _lookup_marker():
    return marker


class BuiltinsValueBridgeTests(unittest.TestCase):
    def test_scalar_builtins_are_captured_before_lookup(self):
        constant_code = (lambda: 42).__code__
        lookup_code = _lookup_marker.__code__
        for value in (1, -7, 1.25, -0.0, False, None, 2 ** 80):
            with self.subTest(value=value):
                namespace = {'__builtins__': value}
                constant = types.FunctionType(constant_code, namespace)
                lookup = types.FunctionType(lookup_code, namespace)
                self.assertIs(type(constant.__builtins__), type(value))
                self.assertEqual(constant.__builtins__, value)
                namespace.clear()
                self.assertEqual(constant(), 42)
                with self.assertRaisesRegex(TypeError, 'not subscriptable'):
                    lookup()

    def test_dictionary_builtins_survive_globals_clear(self):
        mapping = {'marker': object()}
        marker = mapping['marker']
        namespace = {'__builtins__': mapping}
        function = types.FunctionType(_lookup_marker.__code__, namespace)
        self.assertIs(function.__builtins__, mapping)
        namespace.clear()
        del mapping
        gc.collect()
        self.assertIs(function(), marker)

    def test_module_dictionary_survives_module_release(self):
        module = types.ModuleType('canonical_builtins')
        module.marker = object()
        marker = module.marker
        module_ref = weakref.ref(module)
        namespace = {'__builtins__': module}
        function = types.FunctionType(_lookup_marker.__code__, namespace)
        self.assertIs(function.__builtins__, module.__dict__)
        namespace.clear()
        del module
        gc.collect()
        self.assertIsNone(module_ref())
        self.assertIs(function(), marker)

    def test_custom_builtins_keep_their_owner(self):
        class Mapping:
            def __getitem__(self, name):
                if name == 'marker':
                    return 37
                raise KeyError(name)

        mapping = Mapping()
        mapping_ref = weakref.ref(mapping)
        namespace = {'__builtins__': mapping}
        function = types.FunctionType(_lookup_marker.__code__, namespace)
        self.assertIs(function.__builtins__, mapping)
        namespace.clear()
        del mapping
        gc.collect()
        self.assertIsNotNone(mapping_ref())
        self.assertEqual(function(), 37)
        del function
        gc.collect()
        self.assertIsNone(mapping_ref())

    def test_missing_builtins_use_the_current_dictionary(self):
        namespace = {}
        function = types.FunctionType((lambda: len((1, 2, 3))).__code__, namespace)
        self.assertIs(function.__builtins__, builtins.__dict__)
        self.assertEqual(function(), 3)


if __name__ == '__main__':
    unittest.main()
