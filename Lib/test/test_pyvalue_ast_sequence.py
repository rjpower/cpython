import ast
import unittest


class ASTSequenceTests(unittest.TestCase):
    def test_sequence_mutation_is_reported(self):
        body = []
        mutate = False

        class MutatingExpr(ast.Expr):
            def __getattribute__(self, name):
                if name == "value" and mutate:
                    body.clear()
                return super().__getattribute__(name)

        body.append(MutatingExpr(value=ast.Constant(1)))
        module = ast.Module(body=body, type_ignores=[])
        ast.fix_missing_locations(module)
        mutate = True
        with self.assertRaisesRegex(RuntimeError, 'Module field "body" changed size'):
            compile(module, "<test>", "exec")

    def test_invalid_scalar_sequence_element_fails(self):
        module = ast.Module(body=[42], type_ignores=[])
        with self.assertRaises(TypeError):
            compile(module, "<test>", "exec")

    def test_recursive_sequence_element_fails(self):
        function = ast.FunctionDef(
            name="f",
            args=ast.arguments(
                posonlyargs=[], args=[], kwonlyargs=[], kw_defaults=[], defaults=[]
            ),
            body=[],
            decorator_list=[],
        )
        function.lineno = function.col_offset = 1
        function.end_lineno = function.end_col_offset = 1
        function.body.append(function)
        module = ast.Module(body=[function], type_ignores=[])
        with self.assertRaises(RecursionError):
            compile(module, "<test>", "exec")


if __name__ == "__main__":
    unittest.main()
