"""Successful folded unpack transport retains no scratch outputs."""

import gc
import types
import unittest

import _testinternalcapi
from test.test_ir_call_roles import routes


class UnpackTransferTests(unittest.TestCase):
    def test_generic_and_specialized_local_transport_with_callbacks(self):
        observations = []

        class Old:
            def __del__(self):
                gc.collect()
                observations.append(None)

        def unpack(source):
            a, b = Old(), Old()
            a, b = source
            return a, b

        # Both targets are locals: the word's elements are their homes
        self.assertTrue(any(row[0] == "Unpack.Generic" and
                            max(row[3:5]) < unpack.__code__.co_nlocals for row in
                            _testinternalcapi.get_ir(unpack.__code__)))
        a, b = [], {}
        codes = routes(unpack.__code__)
        for admitted in codes:
            function = types.FunctionType(admitted, globals(),
                                          closure=unpack.__closure__)
            for factory in (tuple, list, iter):
                for _ in range(20):
                    result = function(factory((a, b)))
                    self.assertIs(result[0], a)
                    self.assertIs(result[1], b)
        self.assertEqual(len(observations), len(codes) * 3 * 20 * 2)


if __name__ == "__main__":
    unittest.main()
