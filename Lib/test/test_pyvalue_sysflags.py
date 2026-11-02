"""Canonical sys.flags copies retain visible and hidden field owners."""

import sys
import types
import unittest

from test.support import import_helper
from test.support.script_helper import assert_python_ok


def flag_values(flags):
    return {name: getattr(flags, name)
            for name, descriptor in vars(type(flags)).items()
            if isinstance(descriptor, types.MemberDescriptorType)}


class SysFlagsValueTests(unittest.TestCase):
    def test_limit_update_copies_all_fields(self):
        original_limit = sys.get_int_max_str_digits()
        self.addCleanup(sys.set_int_max_str_digits, original_limit)
        for limit in (0, 2048):
            with self.subTest(limit=limit):
                old_flags = sys.flags
                old_values = flag_values(old_flags)
                self.assertGreater(len(old_values), len(old_flags))
                sys.set_int_max_str_digits(limit)
                self.assertIsNot(sys.flags, old_flags)
                self.assertEqual(sys.get_int_max_str_digits(), limit)
                self.assertEqual(flag_values(old_flags), old_values)
                expected = old_values | {'int_max_str_digits': limit}
                self.assertEqual(flag_values(sys.flags), expected)

    def test_config_update_copies_all_fields(self):
        _testcapi = import_helper.import_module('_testcapi')
        original = _testcapi.config_get('inspect')
        self.addCleanup(_testcapi.config_set, 'inspect', original)
        old_flags = sys.flags
        old_values = flag_values(old_flags)
        value = int(not original)
        _testcapi.config_set('inspect', value)
        self.assertIsNot(sys.flags, old_flags)
        self.assertEqual(_testcapi.config_get('inspect'), value)
        self.assertEqual(flag_values(old_flags), old_values)
        self.assertEqual(flag_values(sys.flags), old_values | {'inspect': value})

    def test_invalid_limit_retains_the_current_flags(self):
        old_flags = sys.flags
        old_values = flag_values(old_flags)
        for value, error in [(-1, ValueError), (1, ValueError),
                             (2048.0, TypeError), ('2048', TypeError)]:
            with self.subTest(value=value):
                with self.assertRaises(error):
                    sys.set_int_max_str_digits(value)
                self.assertIs(sys.flags, old_flags)
                self.assertEqual(flag_values(sys.flags), old_values)

    def test_index_reentry_retains_each_snapshot_and_error(self):
        original_limit = sys.get_int_max_str_digits()
        self.addCleanup(sys.set_int_max_str_digits, original_limit)
        old_flags = sys.flags
        old_values = flag_values(old_flags)
        nested = []

        class Index:
            def __index__(self):
                sys.set_int_max_str_digits(0)
                nested.append(sys.flags)
                return 2048

        sys.set_int_max_str_digits(Index())
        self.assertEqual(flag_values(old_flags), old_values)
        self.assertEqual(flag_values(nested[0]),
                         old_values | {'int_max_str_digits': 0})
        self.assertIsNot(sys.flags, nested[0])
        self.assertEqual(flag_values(sys.flags),
                         old_values | {'int_max_str_digits': 2048})
        failure = RuntimeError('index conversion failed')

        class FailedIndex:
            def __index__(self):
                sys.set_int_max_str_digits(0)
                nested.append(sys.flags)
                raise failure

        with self.assertRaises(RuntimeError) as caught:
            sys.set_int_max_str_digits(FailedIndex())
        self.assertIs(caught.exception, failure)
        self.assertIs(sys.flags, nested[-1])
        self.assertEqual(sys.get_int_max_str_digits(), 0)
        self.assertEqual(flag_values(sys.flags),
                         old_values | {'int_max_str_digits': 0})

    def test_startup_config_constructs_all_fields(self):
        script = '''
import sys, types
fields = {name: getattr(sys.flags, name)
          for name, descriptor in vars(type(sys.flags)).items()
          if isinstance(descriptor, types.MemberDescriptorType)}
assert len(fields) == sys.flags.n_fields
assert len(fields) > len(sys.flags)
assert fields['isolated'] == 1
assert fields['no_site'] == 1
assert fields['dont_write_bytecode'] == 1
assert fields['dev_mode'] is True
assert fields['int_max_str_digits'] == 2048
assert sys.get_int_max_str_digits() == 2048
assert all(type(value) in (int, bool, type(None)) for value in fields.values())
'''
        assert_python_ok('-I', '-S', '-B', '-X', 'dev',
                         '-X', 'int_max_str_digits=2048', '-c', script)


if __name__ == '__main__':
    unittest.main()
