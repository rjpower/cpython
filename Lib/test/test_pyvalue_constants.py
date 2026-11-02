"""Canonical common-constant storage and owned compiler object boundaries."""

import unittest

from test import support
from test.support import import_helper

_testinternalcapi = import_helper.import_module('_testinternalcapi')


class PyValueConstantTests(unittest.TestCase):
    def test_common_table_has_one_canonical_inline_integer(self):
        self.assertEqual(_testinternalcapi.common_constant_storage(),
                         (14, 1, 1, 1))

    def test_actual_minus_one_common_load(self):
        def constant():
            return -1

        for _ in range(20):
            self.assertEqual(constant(), -1)
        handlers = [word[0] for word in
                    _testinternalcapi.get_ir(constant.__code__, True)]
        self.assertIn('LOAD_COMMON_CONSTANT', handlers)

    def test_owned_compile_fold_and_constant_results(self):
        for source, expected in [
            ('-1 if True else None', -1),
            ('(-1, -1.0, None, True, False, "", ())',
             (-1, -1.0, None, True, False, '', ())),
            ('1 - 2', -1),
        ]:
            with self.subTest(source=source):
                code = compile(source, '<common constants>', 'eval')
                for _ in range(20):
                    self.assertEqual(eval(code), expected)

    def test_common_exception_classes(self):
        with self.assertRaises(AssertionError):
            exec(compile('assert False', '<common constants>', 'exec'))
        with self.assertRaises(NotImplementedError):
            exec(compile('raise NotImplementedError', '<common constants>',
                         'exec'))

    def test_subinterpreter_initialization_and_clear(self):
        self.assertEqual(support.run_in_subinterp('''
import _testinternalcapi
assert _testinternalcapi.common_constant_storage() == (14, 1, 1, 1)
def constant():
    return -1
assert constant() == -1
'''), 0)


if __name__ == '__main__':
    unittest.main()
