"""Semantic checks for canonical compiler metadata consumers."""

import inspect
import types
import unittest


class CodegenValueTests(unittest.TestCase):
    def test_super_and_closure_names(self):
        class Base:
            def value(self):
                return 19

        class Child(Base):
            def value(self):
                def capture():
                    return self
                return super().value(), capture()

            def explicit(self):
                return super(Child, self).value()

        child = Child()
        self.assertEqual(child.value(), (19, child))
        self.assertEqual(child.explicit(), 19)

    def test_comprehension_cells_and_exception_restore(self):
        def execute():
            x = 37
            functions = [lambda: x for x in range(3)]
            try:
                [1 // (x - 1) for x in range(3)]
            except ZeroDivisionError:
                observed = x
            finally:
                final = x
            nested = [[(x, y) for y in range(2)] for x in range(2)]
            return [function() for function in functions], observed, final, nested

        self.assertEqual(execute(),
                         ([2, 2, 2], 37, 37,
                          [[(0, 0), (0, 1)], [(1, 0), (1, 1)]]))

    def test_class_comprehension_hidden_names(self):
        class Example:
            x = 41
            values = [x for x in range(3)]

        self.assertEqual(Example.x, 41)
        self.assertEqual(Example.values, [0, 1, 2])

    def test_or_pattern_capture_order_and_guard(self):
        def select(value):
            match value:
                case ['left', a, b] | ['right', b, a] if a > 0:
                    return a, b
                case _:
                    return None

        self.assertEqual(select(['left', 3, 8]), (3, 8))
        self.assertEqual(select(['right', 8, 3]), (3, 8))
        self.assertIsNone(select(['right', 8, -1]))
        self.assertIsNone(select(['other', 8, 3]))
        with self.assertRaisesRegex(SyntaxError, 'different names'):
            compile('match []:\n case [a] | [b]: pass', '<pattern>', 'exec')

    def test_deferred_annotation_parameter_and_cells(self):
        namespace = {}
        exec('''
def outer():
    captured = int
    def function(value: captured) -> str:
        return str(value)
    return function
''', namespace)
        function = namespace['outer']()
        annotate = function.__annotate__
        self.assertEqual(list(inspect.signature(annotate).parameters), ['format'])
        self.assertEqual(annotate(1), {'value': int, 'return': str})
        self.assertEqual(function.__annotations__, {'value': int, 'return': str})

    def test_conditional_module_and_class_annotations(self):
        module = types.ModuleType('canonical_annotations')
        exec('''
if True:
    present: int
if False:
    absent: float
class Example:
    if True:
        present: int
    if False:
        absent: float
    unconditional: str
''', module.__dict__)
        self.assertEqual(module.__annotations__, {'present': int})
        self.assertEqual(module.Example.__annotations__,
                         {'present': int, 'unconditional': str})


if __name__ == '__main__':
    unittest.main()
