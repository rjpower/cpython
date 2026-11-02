"""Reference controls for C sequence readers in the universal-value port."""

import ast
import codecs
import __future__
import _imp
import types
import unittest


class SequenceReaderTests(unittest.TestCase):
    def test_codec_result_can_be_scalar(self):
        name = "test_pyvalue_scalar_codec"
        info = codecs.CodecInfo(
            name=name,
            encode=lambda value, errors="strict": (17, len(value)),
            decode=lambda value, errors="strict": (23, len(value)),
        )

        def search(encoding):
            return info if encoding == name else None

        codecs.register(search)
        try:
            self.assertEqual(codecs.encode("x", name), 17)
            self.assertEqual(codecs.decode(b"x", name), 23)
        finally:
            codecs.unregister(search)

    def test_exec_closure_validation(self):
        def outer():
            value = 1
            return lambda: value

        code = outer().__code__
        cell = outer().__closure__[0]
        with self.assertRaises(TypeError):
            exec(code, {}, {}, closure=(1,))
        exec(code, {}, {}, closure=(cell,))

    def test_function_type_closure_validation(self):
        def outer():
            value = 1
            return lambda: value

        function = outer()
        with self.assertRaises(TypeError):
            types.FunctionType(function.__code__, {}, closure=(1,))
        valid = types.FunctionType(function.__code__, {}, closure=function.__closure__)
        self.assertEqual(valid(), 1)

    def test_exception_single_argument_callback_can_replace_args(self):
        cases = ((ValueError, str, "original"),
                 (ValueError, repr, "ValueError(original)"),
                 (KeyError, str, "original"))
        for exception_type, formatter, expected in cases:
            with self.subTest(exception_type=exception_type, formatter=formatter):
                error = exception_type(None)
                calls = []

                class Replacing:
                    def __str__(self):
                        calls.append("format")
                        error.args = ("replacement",)
                        return "original"

                    __repr__ = __str__

                error.args = (Replacing(),)
                self.assertEqual(formatter(error), expected)
                self.assertEqual(error.args, ("replacement",))
                self.assertEqual(calls, ["format"])

                sentinel = RuntimeError("sentinel")
                calls.clear()

                class Raising:
                    def __str__(self):
                        calls.append("format")
                        error.args = ()
                        raise sentinel

                    __repr__ = __str__

                error.args = (Raising(),)
                with self.assertRaises(RuntimeError) as caught:
                    formatter(error)
                self.assertIs(caught.exception, sentinel)
                self.assertEqual(error.args, ())
                self.assertEqual(calls, ["format"])

    def test_ast_scalar_tuple_constant_unparse(self):
        node = ast.Expression(ast.Constant((1, None, 2.5)))
        self.assertEqual(ast.unparse(node), "(1, None, 2.5)")

    def test_c_annotation_string_route_reference(self):
        tree = ast.parse("def f() -> marker:\n    pass\n")
        tree.body[0].returns = ast.Constant((1, None, 2.5))
        ast.fix_missing_locations(tree)
        namespace = {}
        flags = __future__.annotations.compiler_flag
        exec(compile(tree, "<test>", "exec", flags=flags), namespace)
        self.assertEqual(namespace["f"].__annotations__["return"], "(1, None, 2.5)")

    def test_nested_code_filename_reference(self):
        namespace = {}
        exec("def outer():\n    def inner():\n        return 1\n    return inner", namespace)
        code = namespace["outer"].__code__
        constants = code.co_consts + (1, None, 2.5)
        code = code.replace(co_consts=constants)
        _imp._fix_co_filename(code, "new.py")
        nested = code
        self.assertTrue(any(
            isinstance(item, types.CodeType) and item.co_filename == "new.py"
            for item in nested.co_consts
        ))
        self.assertEqual(nested.co_consts[-3:], (1, None, 2.5))

    def test_function_docstring_constant_reference(self):
        def sample():
            "doc"
            return 1

        code = sample.__code__
        for value in (1, 1.5, None):
            constants = (value,) + code.co_consts[1:]
            replaced = code.replace(co_consts=constants)
            function = types.FunctionType(replaced, {})
            self.assertIsNone(function.__doc__)

        function = types.FunctionType(code, {})
        self.assertEqual(function.__doc__, "doc")


if __name__ == "__main__":
    unittest.main()
