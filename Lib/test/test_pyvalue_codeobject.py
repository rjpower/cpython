"""Canonical constant interning and code-object input validation."""

import math
import sys
import types
import unittest

from test.support import import_helper

_testinternalcapi = import_helper.import_module('_testinternalcapi')


class PyValueCodeObjectTests(unittest.TestCase):
    def base_code(self):
        return (lambda x: x).__code__

    def constructor(self, code, **changes):
        fields = [
            'co_argcount', 'co_posonlyargcount', 'co_kwonlyargcount',
            'co_nlocals', 'co_stacksize', 'co_flags', 'co_code', 'co_consts',
            'co_names', 'co_varnames', 'co_filename', 'co_name', 'co_qualname',
            'co_firstlineno', 'co_linetable', 'co_exceptiontable',
            'co_freevars', 'co_cellvars',
        ]
        return types.CodeType(*(changes.get(field, getattr(code, field))
                                for field in fields))

    def test_replace_rejects_inline_name_elements(self):
        code = self.base_code()
        for field in ('co_names', 'co_varnames', 'co_cellvars', 'co_freevars'):
            for value in (1, 1.5):
                with self.subTest(field=field, value=value):
                    with self.assertRaisesRegex(SystemError,
                                                'non-string found in code slot'):
                        code.replace(**{field: (value,)})

    def test_constructor_reports_nonstring_type(self):
        for field in ('co_names', 'co_varnames', 'co_cellvars', 'co_freevars'):
            with self.subTest(field=field):
                with self.assertRaisesRegex(TypeError,
                                            'name tuples must contain only strings'):
                    self.constructor(self.base_code(), **{field: (1,)})

    def test_constructor_copies_unicode_subclasses(self):
        class Name(str):
            pass

        code = self.constructor(self.base_code(), co_names=(Name('attribute'),),
                                co_varnames=(Name('argument'),))
        self.assertIs(type(code.co_names[0]), str)
        self.assertIs(type(code.co_varnames[0]), str)
        self.assertEqual(code.co_names, ('attribute',))
        self.assertEqual(code.co_varnames, ('argument',))
        with self.assertRaisesRegex(SystemError, 'non-string found in code slot'):
            self.base_code().replace(co_varnames=(Name('argument'),))

    def test_interning_keeps_shared_nan_tuple_hash(self):
        nan = float('nan')
        interned = sys.intern(''.join(['an_internable_', 'code_constant']))
        name = ''.join(['an_internable_', 'code_constant'])
        self.assertIsNot(name, interned)
        shared = (name, nan, True, 1, 1.0, 0.0, -0.0)
        before_hash = hash(shared)
        lookup = {shared: 'retained'}
        code = (lambda: None).__code__.replace(co_consts=(shared,))
        self.assertIs(code.co_consts[0], shared)
        self.assertIs(shared[0], interned)
        self.assertIs(shared[1], nan)
        self.assertEqual(hash(shared), before_hash)
        self.assertEqual(lookup[shared], 'retained')
        self.assertEqual(tuple(map(type, shared[2:5])), (bool, int, float))
        self.assertEqual(math.copysign(1, shared[-2]), 1)
        self.assertEqual(math.copysign(1, shared[-1]), -1)

    def test_recursive_frozenset_interning(self):
        interned = sys.intern(''.join(['an_internable_', 'frozenset_constant']))
        nested_interned = sys.intern(''.join(['a_nested_', 'tuple_constant']))
        name = ''.join(['an_internable_', 'frozenset_constant'])
        nested = ''.join(['a_nested_', 'tuple_constant'])
        self.assertIsNot(name, interned)
        self.assertIsNot(nested, nested_interned)
        value = frozenset((name, (nested, 1.5, -0.0)))
        before_hash = hash(value)
        code = (lambda: None).__code__.replace(co_consts=(value,))
        result = code.co_consts[0]
        self.assertEqual(result, value)
        self.assertEqual(hash(result), before_hash)
        for item in result:
            if isinstance(item, str):
                self.assertIs(item, interned)
            else:
                self.assertIs(item[0], nested_interned)
                self.assertEqual(math.copysign(1, item[-1]), -1)

    def pool_constant_function(self, original):
        # The constant is a list item, a window input that keeps its load;
        # a return or a call operand would read a constant slot.  Const.Pool
        # reads the code's constant whatever its form.
        words = _testinternalcapi.get_ir(original.__code__)
        index = next(i for i, word in enumerate(words)
                     if word[0] == 'Const.Pool')
        return original, index

    def check_replacements(self, original, word_index, value, replacements):
        for _ in range(64):
            self.assertEqual(original(), [value])
        index = next(i for i, c in enumerate(original.__code__.co_consts)
                     if type(c) is type(value) and c == value)
        for replacement in replacements:
            with self.subTest(kind=type(replacement)):
                consts = list(original.__code__.co_consts)
                consts[index] = replacement
                code = original.__code__.replace(co_consts=tuple(consts))
                # The replaced code lowers afresh and loads the new constant
                self.assertEqual(_testinternalcapi.get_ir(code)[word_index][0],
                                 'Const.Pool')
                result = types.FunctionType(code, {})()[0]
                self.assertIs(type(result), type(replacement))
                if replacement == replacement:
                    self.assertEqual(result, replacement)

    def test_warmed_integer_replacement_falls_back(self):
        original, word_index = self.pool_constant_function(
            eval('lambda: [1073741824]'))
        self.check_replacements(original, word_index, 1073741824,
                                (1 << 50, True, 'different type', object()))

    def test_warmed_float_replacement_falls_back(self):
        original, word_index = self.pool_constant_function(
            eval('lambda: [1.5]'))
        self.check_replacements(original, word_index, 1.5,
                                (float('nan'), 1 << 40, object()))

if __name__ == '__main__':
    unittest.main()
