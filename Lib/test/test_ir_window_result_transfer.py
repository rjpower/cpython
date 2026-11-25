"""A window result may be built directly into a local."""

import types
import unittest

import _testinternalcapi
from test.test_ir_call_roles import routes


class WindowResultTransferTests(unittest.TestCase):
    def test_parallel_empty_list_assignment_then_call(self):
        def prepare(predicate):
            first, second = [], []
            return predicate(first, second)

        rows = _testinternalcapi.get_ir(prepare.__code__)
        self.assertTrue(any(row[0] == "Build.List" and
                            row[1] < prepare.__code__.co_nlocals for row in rows))
        for admitted in routes(prepare.__code__):
            function = types.FunctionType(admitted, globals())
            for _ in range(20):
                first, second = function(lambda a, b: (a, b))
                self.assertEqual(first, [])
                self.assertEqual(second, [])
                self.assertIsNot(first, second)


if __name__ == "__main__":
    unittest.main()
