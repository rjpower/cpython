import ast
import sys
import unittest


class ASTConstructorTests(unittest.TestCase):
    def test_positional_keyword_and_defaults(self):
        name = ast.Name("item", ast.Load())
        self.assertEqual(name.id, "item")
        self.assertIsInstance(name.ctx, ast.Load)

        constant = ast.Constant(value=42)
        self.assertEqual(constant.value, 42)

        arguments = ast.arguments(
            posonlyargs=[], args=[], vararg=None,
            kwonlyargs=[], kw_defaults=[], kwarg=None, defaults=[],
        )
        self.assertEqual(arguments.kwonlyargs, [])

    def test_missing_fields_receive_owned_defaults(self):
        first = ast.arguments()
        second = ast.arguments()
        for field in ("posonlyargs", "args", "kwonlyargs", "kw_defaults", "defaults"):
            self.assertEqual(getattr(first, field), [])
            self.assertIsNot(getattr(first, field), getattr(second, field))
        self.assertIsNone(first.vararg)
        self.assertIsNone(first.kwarg)
        self.assertIsInstance(ast.Name(id="item").ctx, ast.Load)

    def test_constructor_error_order_and_messages(self):
        with self.assertRaisesRegex(
            TypeError,
            r"Name constructor takes at most 2 positional arguments",
        ):
            ast.Name("item", ast.Load(), "extra")

        with self.assertRaisesRegex(
            TypeError,
            r"Constant got multiple values for argument 'value'",
        ):
            ast.Constant(1, value=2)


    @unittest.skipIf(
        sys.version_info < (3, 15),
        "older host AST constructors still accept arbitrary keywords with warnings",
    )
    def test_unknown_keyword_error(self):
        with self.assertRaisesRegex(
            TypeError,
            r"Name.__init__ got an unexpected keyword argument 'unknown'",
        ):
            ast.Name(id="item", unknown=1)

    @unittest.skipIf(
        sys.version_info < (3, 15),
        "older host AST constructors warn instead of rejecting missing fields",
    )
    def test_required_field_error(self):
        with self.assertRaisesRegex(
            TypeError,
            r"Name\.__init__ missing 1 required positional argument: 'id'",
        ):
            ast.Name()

    @unittest.skipIf(
        sys.version_info < (3, 15),
        "older host AST constructors warn instead of rejecting missing _field_types entries",
    )
    def test_field_type_absence_and_missing_entry(self):
        class FieldsWithoutTypes(ast.AST):
            _fields = ("value",)

        obj = FieldsWithoutTypes()
        self.assertNotIn("value", obj.__dict__)
        self.assertEqual(FieldsWithoutTypes(value=5).value, 5)

        class IncompleteTypes(ast.AST):
            _fields = ("optional", "required")
            _field_types = {"optional": int | None}
            optional: int | None = None
            required: int | None = None

        with self.assertRaisesRegex(
            TypeError,
            r"Field 'required' is missing from .*\.IncompleteTypes\._field_types",
        ):
            IncompleteTypes()

    def test_setattr_exception_instance_survives_cleanup(self):
        sentinel = RuntimeError("setter sentinel")
        visited = []

        class RaisingName(ast.Name):
            def __setattr__(self, name, value):
                visited.append(name)
                if name == "id":
                    raise sentinel
                super().__setattr__(name, value)

        with self.assertRaises(RuntimeError) as caught:
            RaisingName(id="item", ctx=ast.Load())
        self.assertIs(caught.exception, sentinel)
        self.assertEqual(visited, ["id"])


if __name__ == "__main__":
    unittest.main()
