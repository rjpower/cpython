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
        code = unpack.__code__
        rows = _testinternalcapi.get_ir(code)
        [found] = [i for i, row in enumerate(rows) if row[0] == "Unpack.Generic"]
        [(is_source, homes)] = _testinternalcapi.ir_span_probe(code, found)[0]
        self.assertFalse(is_source)
        self.assertTrue(all(h < code.co_nlocals for h in homes), homes)
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
