"""Symbol flags and directive locations in canonical compiler metadata."""

import ast
import symtable
import unittest


class SymtableValueTests(unittest.TestCase):
    def test_flags_and_nonlocal_cells(self):
        source = '''
def outer(parameter):
    value = 3
    def inner():
        nonlocal value
        global result
        value += parameter
        result = value
        return value
    return inner
'''
        table = symtable.symtable(source, '<canonical symtable>', 'exec')
        outer = table.lookup('outer').get_namespace()
        inner = outer.lookup('inner').get_namespace()
        self.assertTrue(outer.lookup('parameter').is_parameter())
        self.assertTrue(outer.lookup('value').is_local())
        self.assertTrue(inner.lookup('value').is_nonlocal())
        self.assertTrue(inner.lookup('value').is_free())
        self.assertTrue(inner.lookup('parameter').is_free())
        self.assertTrue(inner.lookup('result').is_global())
        namespace = {}
        exec(compile(source, '<canonical symtable>', 'exec'), namespace)
        function = namespace['outer'](7)
        self.assertEqual(function(), 10)
        self.assertEqual(function(), 17)
        self.assertEqual(namespace['result'], 17)
        self.assertEqual(function.__code__.co_freevars, ('parameter', 'value'))

    def test_nested_comprehensions_and_sibling_closure(self):
        source = '''
def outer(value):
    index = 71
    functions = [[lambda: (value, index, inner) for inner in range(2)]
                 for index in range(3)]
    def sibling():
        return value, index
    return index, functions, sibling
'''
        namespace = {}
        exec(compile(source, '<canonical symtable>', 'exec'), namespace)
        index, functions, sibling = namespace['outer'](23)
        self.assertEqual(index, 71)
        self.assertEqual(sibling(), (23, 71))
        self.assertEqual([[function() for function in group]
                          for group in functions], [[(23, 2, 1)] * 2] * 3)
        self.assertIn('index', namespace['outer'].__code__.co_cellvars)
        self.assertEqual(sibling.__code__.co_freevars, ('index', 'value'))

    def test_class_comprehension_closure(self):
        source = '''
def outer(value):
    class Result:
        value = 99
        functions = [lambda: (value, index) for index in range(2)]
        def method(self):
            return __class__
    return Result
'''
        namespace = {}
        exec(compile(source, '<canonical symtable>', 'exec'), namespace)
        result = namespace['outer'](31)
        self.assertEqual(result.value, 99)
        self.assertIs(result().method(), result)
        self.assertEqual([function() for function in result.functions],
                         [(31, 1)] * 2)
        self.assertEqual(result.functions[0].__code__.co_freevars,
                         ('index', 'value'))

    def test_global_assignment_in_comprehension(self):
        source = '''
value = -1
def update():
    global value
    return [(value := item) for item in range(4)]
'''
        namespace = {}
        exec(compile(source, '<canonical symtable>', 'exec'), namespace)
        self.assertEqual(namespace['update'](), [0, 1, 2, 3])
        self.assertEqual(namespace['value'], 3)
        table = symtable.symtable(source, '<canonical symtable>', 'exec')
        self.assertTrue(table.lookup('update').get_namespace().lookup('value').is_global())

    def test_directive_errors_preserve_source_ranges(self):
        cases = [
            ('def f():\n    nonlocal missing\n',
             "no binding for nonlocal 'missing' found", (2, 5, 2, 21)),
            ('def f():\n    global item\n    nonlocal item\n',
             "name 'item' is nonlocal and global", (2, 5, 2, 16)),
            ('def f():\n    item = 1\n    global item\n',
             "name 'item' is assigned to before global declaration",
             (3, 5, 3, 16)),
            ('def f():\n    return [(item := 1) for item in range(2)]\n',
             "assignment expression cannot rebind comprehension iteration "
             "variable 'item'", (2, 14, 2, 18)),
            ('def f[T, T]():\n    pass\n', "duplicate type parameter 'T'",
             (1, 10, 1, 11)),
        ]
        for source, message, location in cases:
            for compiler in (compile, symtable.symtable):
                with self.subTest(source=source, compiler=compiler):
                    with self.assertRaises(SyntaxError) as caught:
                        compiler(source, '<canonical symtable>', 'exec')
                    error = caught.exception
                    self.assertEqual(error.msg, message)
                    self.assertEqual(error.filename, '<canonical symtable>')
                    self.assertEqual((error.lineno, error.offset,
                                      error.end_lineno, error.end_offset),
                                     location)

    def test_directive_ast_missing_and_multiline_end(self):
        for end, location in [(None, (2, 5, 2, 5)), ((3, 1), (2, 5, 3, 2))]:
            with self.subTest(end=end):
                tree = ast.parse('def f():\n    nonlocal missing\n')
                directive = tree.body[0].body[0]
                directive.end_lineno, directive.end_col_offset = end or (None, None)
                with self.assertRaises(SyntaxError) as caught:
                    compile(tree, '<canonical symtable>', 'exec')
                error = caught.exception
                self.assertEqual(error.msg,
                                 "no binding for nonlocal 'missing' found")
                self.assertEqual((error.lineno, error.offset,
                                  error.end_lineno, error.end_offset), location)

    def test_repeated_errors_do_not_poison_success(self):
        for _ in range(20):
            with self.assertRaisesRegex(SyntaxError, 'no binding for nonlocal'):
                compile('def f():\n    nonlocal missing\n',
                        '<canonical symtable>', 'exec')
            table = symtable.symtable('def f(argument):\n    return argument\n',
                                      '<canonical symtable>', 'exec')
            function = table.lookup('f').get_namespace()
            self.assertTrue(function.lookup('argument').is_parameter())
            namespace = {}
            exec(compile('answer = 42', '<canonical symtable>', 'exec'), namespace)
            self.assertEqual(namespace['answer'], 42)


if __name__ == '__main__':
    unittest.main()
