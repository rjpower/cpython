"""Actual local/discarded window-result moves vacate their source homes."""

import marshal
import types
import unittest

import _testinternalcapi
from test.test_ir_admission import construct
from test.test_ir_call_roles import routes
from test.test_ir_transfer_admission import encoded, make_code


class WindowResultTransferTests(unittest.TestCase):
    def assert_rejected(self, code, words, message):
        replacement = encoded(words)
        for route in (construct, lambda co, **kw: co.replace(**kw)):
            with self.assertRaisesRegex(ValueError, message):
                route(code, co_code=replacement)
        payload = marshal.dumps(code)
        self.assertEqual(payload.count(code.co_code), 1)
        with self.assertRaisesRegex(ValueError, message):
            marshal.loads(payload.replace(code.co_code, replacement, 1))

    def test_only_actual_local_or_discard_moves_empty_the_source(self):
        for count in (0, 2):
            for destination in (0, 4095):
                with self.subTest(count=count, destination=destination):
                    words = [("POP_TOP", 4)]
                    words.extend(("LOAD_CONST", 2 + i) for i in range(count))
                    producer = len(words)
                    words.extend((("BUILD_LIST", destination, 2, count, 0),
                                  ("LOAD_CONST", 1),
                                  ("CALL_NO_SELF", 1, 1, 0, 0),
                                  ("RETURN_VALUE", 1)))
                    code = make_code(words, slots=6, locals=("local",))
                    for admitted in routes(code):
                        self.assertEqual(_testinternalcapi.get_ir(admitted, True),
                                         _testinternalcapi.get_ir(code))
                    bad = words.copy()
                    bad[producer] = ("BUILD_LIST", 2, 2, count, 0)
                    self.assert_rejected(code, bad, "virtual call input")
                    bad[producer] = ("BUILD_LIST", 3, 2, count, 0)
                    self.assert_rejected(code, bad, "window result destination")
                    bad = words.copy()
                    bad[0] = ("LOAD_CONST", 4)
                    self.assert_rejected(code, bad, "child clobber home")

    def test_parallel_empty_list_assignment_then_call(self):
        def prepare(predicate):
            first, second = [], []
            return predicate(first, second)

        rows = _testinternalcapi.get_ir(prepare.__code__)
        self.assertTrue(any(row[0] == "BUILD_LIST" and
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
