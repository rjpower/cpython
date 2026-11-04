"""Canonical constant interning and code-object input validation."""

import math
import opcode
import sys
import types
import unittest

from test.support import import_helper
from test.support.bytecode_helper import encode_words

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

    def inline_constant_function(self, original, handler):
        code = original.__code__
        words = _testinternalcapi.get_ir(code)
        index = next(i for i, word in enumerate(words)
                     if word[0] in ('LOAD_CONST', 'LOAD_CONST_INLINE_INT',
                                    'LOAD_CONST_INLINE_FLOAT'))
        word = (opcode._specialized_opmap[handler], *words[index][1:])
        encoded = (code.co_code[:8 * index] + encode_words(word)
                   + code.co_code[8 * (index + 1):])
        code = code.replace(co_code=encoded)
        self.assertEqual(_testinternalcapi.get_ir(code)[index][0], handler)
        return types.FunctionType(code, {}), index

    def test_warmed_integer_replacement_falls_back(self):
        original, word_index = self.inline_constant_function(
            eval('lambda: 1073741824'), 'LOAD_CONST_INLINE_INT')
        for _ in range(64):
            self.assertEqual(original(), 1073741824)
        index = next(i for i, value in enumerate(original.__code__.co_consts)
                     if type(value) is int and value == 1073741824)
        for replacement in (1 << 50, True, 'different type', object()):
            with self.subTest(kind=type(replacement)):
                consts = list(original.__code__.co_consts)
                consts[index] = replacement
                code = original.__code__.replace(co_consts=tuple(consts))
                self.assertEqual(_testinternalcapi.get_ir(code)[word_index][0],
                                 'LOAD_CONST')
                result = types.FunctionType(code, {})()
                self.assertIs(result, replacement)

    def test_warmed_float_replacement_falls_back(self):
        original, word_index = self.inline_constant_function(
            eval('lambda: 1.5'), 'LOAD_CONST_INLINE_FLOAT')
        for _ in range(64):
            self.assertEqual(original(), 1.5)
        index = next(i for i, value in enumerate(original.__code__.co_consts)
                     if type(value) is float and value == 1.5)
        for replacement in (float('nan'), 1 << 40, object()):
            with self.subTest(kind=type(replacement)):
                consts = list(original.__code__.co_consts)
                consts[index] = replacement
                code = original.__code__.replace(co_consts=tuple(consts))
                self.assertEqual(_testinternalcapi.get_ir(code)[word_index][0],
                                 'LOAD_CONST')
                self.assertIs(types.FunctionType(code, {})(), replacement)


if __name__ == '__main__':
    unittest.main()
