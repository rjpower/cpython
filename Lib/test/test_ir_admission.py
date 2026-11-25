"""Code object construction, replacement and marshal over compiled functions."""

import marshal
import types
import unittest

from test.support import import_helper


_testinternalcapi = import_helper.import_module("_testinternalcapi")


def construct(code, **changes):
    fields = (
        "argcount", "posonlyargcount", "kwonlyargcount", "nlocals",
        "stacksize", "flags", "code", "consts", "names", "varnames",
        "filename", "name", "qualname", "firstlineno", "linetable",
        "exceptiontable", "freevars", "cellvars",
    )
    return types.CodeType(*(changes.get("co_" + field,
                                      getattr(code, "co_" + field))
                            for field in fields))


class AdmissionTests(unittest.TestCase):
    def test_inline_constant_normalization(self):
        def f():
            # A list item is a window input, which keeps its load
            return [1000]
        code = f.__code__
        words = _testinternalcapi.get_ir(code)
        # Const.Pool reads the code's constant when it runs: no word holds
        # the value, so a replaced constant needs no rewrite
        self.assertIn("Const.Pool", [word[0] for word in words])
        constants = tuple("x" * 1000 if item == 1000 else item
                          for item in code.co_consts)
        for route in (construct, lambda co, **changes: co.replace(**changes)):
            with self.subTest(route=route):
                changed = route(code, co_consts=constants)
                self.assertIn("Const.Pool",
                    [word[0] for word in _testinternalcapi.get_ir(changed)])
                self.assertEqual(types.FunctionType(changed, {})(), ["x" * 1000])
                restored = marshal.loads(marshal.dumps(changed))
                self.assertEqual(types.FunctionType(restored, {})(), ["x" * 1000])

    def test_valid_compiled_images_roundtrip(self):
        def f(a):
            try:
                return [value.target(1000) for value in a]
            except LookupError:
                return []
        for code in (f.__code__, compile("def g():\n yield from ()\n", "<admission>", "exec")):
            with self.subTest(name=code.co_name):
                self.assertEqual(construct(code), code)
                self.assertEqual(code.replace(), code)
                self.assertEqual(marshal.loads(marshal.dumps(code)), code)


if __name__ == "__main__":
    unittest.main()
