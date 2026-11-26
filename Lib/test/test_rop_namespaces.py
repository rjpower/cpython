"""The namespace, annotation and import families
(Python/register_ops/namespaces.rop): NamespaceRead, NamespaceReadFrom,
NamespaceWrite, SetupAnnotations, Import and ImportFrom.
Their stack ops lower to these arms and nothing else runs them."""

import sys
import types
import unittest

from test.support import import_helper

capi = import_helper.import_module("_testinternalcapi")


def arms(code):
    return [word[0] for word in capi.get_ir(code, True)]


def run(source, namespace=None, locals_=None):
    """The code object of source, run once so that its words exist"""
    code = compile(source, "<test>", "exec")
    exec(code, namespace if namespace is not None else {}, locals_)
    return code


class Mapping(dict):
    """A class namespace of the kind a metaclass's __prepare__ makes"""
    log = None

    def __getitem__(self, key):
        self.log.append(("get", key))
        return super().__getitem__(key)

    def __setitem__(self, key, value):
        self.log.append(("set", key))
        super().__setitem__(key, value)

    def __delitem__(self, key):
        self.log.append(("del", key))
        super().__delitem__(key)


class Final:
    log = []

    def __del__(self):
        Final.log.append("del")


class WordTests(unittest.TestCase):
    def check(self, source, *expected):
        names = arms(run(source))
        for name in expected:
            self.assertIn(name, names)
        for old in ("LOAD_NAME", "STORE_NAME", "STORE_GLOBAL", "IMPORT_NAME",
                    "IMPORT_FROM", "LOAD_LOCALS", "LOAD_BUILD_CLASS",
                    "SETUP_ANNOTATIONS", "LOAD_FROM_DICT_OR_GLOBALS"):
            self.assertNotIn(old, names)
        return names

    def test_name_read_and_write(self):
        self.check("x = 1\ny = x\n", "NamespaceWrite.Name", "NamespaceRead.Name")

    def test_delete(self):
        self.check("x = 1\ndel x\n", "NamespaceWrite.Name")

    def test_global(self):
        ns = {}
        code = compile("def f():\n    global g\n    g = 1\n    del g\n"
                       "f()\n", "<test>", "exec")
        exec(code, ns)
        body = ns["f"].__code__
        names = arms(body)
        self.assertIn("NamespaceWrite.Global", names)
        self.assertIn("NamespaceWrite.Global", names)

    def test_class_body(self):
        self.check("class C:\n    x = 1\n    y = x\n",
                   "NamespaceRead.BuildClass")
        ns = {}
        exec("class C:\n    x = 1\n    y = x\n", ns)
        self.assertEqual(ns["C"].y, 1)

    def test_mapped_read(self):
        # an annotation scope reads the class namespace, then the globals
        ns = {"Other": float}
        exec("class C:\n    Local = int\n    a: Local\n    b: Other\n"
             "    c: undefined_name_xyz\n", ns)
        annotate = ns["C"].__annotate_func__
        with self.assertRaises(NameError):
            annotate(1)
        self.assertIn("NamespaceReadFrom.Mapping", arms(annotate.__code__))

    def test_import(self):
        self.check("import sys\nfrom os import path, sep\nimport os.path as p\n",
                   "Import.Policy",
                   "ImportFrom.From", "ImportFrom.FromLast")

    def test_last_from_spends_the_module(self):
        names = self.check("from os import path, sep, curdir\n")
        self.assertEqual(names.count("ImportFrom.From"), 2)
        self.assertEqual(names.count("ImportFrom.FromLast"), 1)
        self.assertNotIn("Retire.Close", names)

    def test_eager_import_in_a_handler(self):
        names = self.check(
            "try:\n    import sys\nexcept ImportError:\n    pass\n",
            "Import.Eager")

    def test_annotations(self):
        self.check("from __future__ import annotations\nx: int = 1\n",
                   "SetupAnnotations.Setup")


class BehaviorTests(unittest.TestCase):
    def test_name_lookup_order(self):
        ns = {"__builtins__": {"b": "builtin"}, "g": "global"}
        loc = {"l": "local"}
        exec("r = (l, g, b)", ns, loc)
        self.assertEqual(loc["r"], ("local", "global", "builtin"))

    def test_mapped_read_order(self):
        ns = {"Other": float, "__name__": "m",
              "__builtins__": {"Builtin": str, "int": int,
                               "__build_class__": __build_class__}}
        exec("class C:\n    Local = int\n    a: Local\n    b: Other\n"
             "    c: Builtin\n", ns)
        self.assertEqual(ns["C"].__annotations__,
                         {"a": int, "b": float, "c": str})

    def test_missing_build_class(self):
        with self.assertRaises(NameError):
            exec("class C: pass", {"__builtins__": {}})

    def test_name_error(self):
        with self.assertRaises(NameError):
            exec("missing", {}, {})
        with self.assertRaises(NameError):
            exec("del missing", {}, {})

    def test_delete_in_mapping(self):
        m = Mapping()
        m.log = []
        exec("x = 1\ndel x\n", {}, m)
        self.assertEqual(m.log, [("set", "x"), ("del", "x")])
        with self.assertRaises(NameError):
            exec("del y", {}, m)

    def test_global_delete(self):
        ns = {}
        exec("def f():\n    global g\n    g = 1\n    del g\n", ns)
        ns["f"]()
        self.assertNotIn("g", ns)
        with self.assertRaises(NameError):
            ns["f"].__globals__.pop("g", None)
            exec("def h():\n    global g\n    del g\nh()", ns)

    def test_class_namespace_mapping(self):
        log = []

        class Meta(type):
            @classmethod
            def __prepare__(mcls, name, bases):
                m = Mapping()
                m.log = log
                return m

        class C(metaclass=Meta):
            x = 1
            y = x
        self.assertEqual(C.y, 1)
        self.assertIn(("get", "x"), log)

    def test_store_releases_the_value(self):
        Final.log.clear()
        ns = {}
        exec("a = F()\na = 1\n", {"F": Final}, ns)
        self.assertEqual(Final.log, ["del"])

    def test_failed_store_releases_the_value(self):
        class Bad(dict):
            def __setitem__(self, key, value):
                raise KeyError(key)
        Final.log.clear()
        with self.assertRaises(KeyError):
            exec("a = F()", {"F": Final}, Bad())
        self.assertEqual(Final.log, ["del"])

    def test_no_locals(self):
        # a frame without locals cannot store a name
        code = compile("x = 1", "<t>", "exec")
        with self.assertRaises(TypeError):
            exec(code, {}, 5)

    def test_annotations(self):
        ns = {}
        exec("from __future__ import annotations\nx: int = 1\ny: str\n", ns)
        self.assertEqual(ns["__annotations__"], {"x": "int", "y": "str"})
        # a namespace that has __annotations__ keeps it
        m = {"__annotations__": {"seen": 1}}
        exec("from __future__ import annotations\nx: int = 1", {}, m)
        self.assertEqual(m["__annotations__"], {"seen": 1, "x": "int"})

    def test_import_forms(self):
        ns = {}
        exec("import os.path as p\nfrom os import path, sep as s\n"
             "import sys\nfrom sys import (argv, path as sp)\n", ns)
        import os
        self.assertIs(ns["p"], os.path)
        self.assertIs(ns["path"], os.path)
        self.assertEqual(ns["s"], os.sep)
        self.assertIs(ns["sp"], sys.path)

    def test_import_from_errors(self):
        with self.assertRaises(ImportError):
            exec("from os import nonexistent_name", {})
        with self.assertRaises(ImportError):
            exec("from os import path, nonexistent_name", {})
        with self.assertRaises(ModuleNotFoundError):
            exec("import nonexistent_module_x", {})

    def test_relative_import(self):
        ns = {"__name__": "test.test_rop_namespaces", "__package__": "test"}
        exec("from . import support", ns)
        self.assertIs(ns["support"], sys.modules["test.support"])

    def test_lazy_import_keeps_its_behavior(self):
        ns = {}
        exec("lazy import json\nlazy from os import path\n", ns)
        self.assertEqual(type(ns["json"]).__name__, "lazy_import")
        # the first read of the name reifies it
        exec("r = json.dumps([1])", ns)
        self.assertEqual(ns["r"], "[1]")
        self.assertIs(type(ns["json"]), types.ModuleType)

    def test_import_in_a_function_is_eager(self):
        def f():
            import json
            return json
        self.assertIs(f(), sys.modules["json"])

    def test_import_level_and_fromlist_are_released(self):
        # a __import__ that raises: nothing leaks and the error propagates
        class Boom(Exception):
            pass

        def bad(*args, **kwargs):
            raise Boom
        with self.assertRaises(Boom):
            exec("import anything", {"__builtins__": {"__import__": bad}})


if __name__ == "__main__":
    unittest.main()
