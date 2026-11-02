"""Compiler flags and local indices stored as canonical dictionary values."""

import types
import unittest


class CompilerMetadataTests(unittest.TestCase):
    def test_argument_cells_keep_their_offsets(self):
        def outer(pos, /, arg, *args, kw=3, **kwargs):
            local = 11

            def inner():
                return pos, arg, args, kw, kwargs, local

            return inner

        inner = outer(1, 2, 4, 5, kw=7, flag=9)
        self.assertEqual(inner(), (1, 2, (4, 5), 7, {'flag': 9}, 11))
        self.assertEqual(outer.__code__.co_varnames[:5],
                         ('pos', 'arg', 'kw', 'args', 'kwargs'))
        self.assertEqual(set(inner.__code__.co_freevars),
                         {'pos', 'arg', 'args', 'kw', 'kwargs', 'local'})

    def test_inlined_comprehension_cells_preserve_outer_local(self):
        def outer(value):
            index = 17
            functions = [lambda: (value, index) for index in range(4)]
            return index, functions

        index, functions = outer(23)
        self.assertEqual(index, 17)
        self.assertEqual([function() for function in functions], [(23, 3)] * 4)

    def test_class_scope_and_method_free_variables(self):
        def outer(value):
            class Result:
                value = 97

                def method(self):
                    return value, __class__

            return Result

        result = outer(31)
        self.assertEqual(result().method(), (31, result))
        self.assertEqual(result.value, 97)

    def test_metadata_order_is_deterministic(self):
        source = '''
def outer(zeta, alpha, /, *args, beta=5, **kwargs):
    gamma = 7
    def inner():
        return alpha, zeta, args, beta, kwargs, gamma
    return inner
'''
        first = compile(source, '<canonical metadata>', 'exec')
        second = compile(source, '<canonical metadata>', 'exec')

        def metadata(code):
            return (code.co_varnames, code.co_cellvars, code.co_freevars,
                    code.co_code,
                    tuple(metadata(value) for value in code.co_consts
                          if isinstance(value, types.CodeType)))

        self.assertEqual(metadata(first), metadata(second))
        namespace = {}
        exec(first, namespace)
        self.assertEqual(namespace['outer'](2, 3, 4, beta=11, answer=13)(),
                         (3, 2, (4,), 11, {'answer': 13}, 7))


if __name__ == '__main__':
    unittest.main()
