"""A field-mapped scalar input does not write its former window home."""

import types
import unittest

import _testinternalcapi
from test.test_ir_call_roles import routes


class FoldedInputTransferTests(unittest.TestCase):
    def test_comprehension_local_input_survives_loop_joins(self):
        def lists(fn, values):
            return fn([value for value in values])

        def sets(fn, values):
            return fn({value for value in values})

        def dictionaries(fn, values):
            return fn({key: value for key, value in values})

        for source, operation, values, expected in (
                (lists, "AddElement.Append", (1, 2, 1), [1, 2, 1]),
                (sets, "AddElement.Add", (1, 2, 1), {1, 2}),
                (dictionaries, "AddEntry.Put", ((1, 2), (3, 4)), {1: 2, 3: 4})):
            rows = _testinternalcapi.get_ir(source.__code__)
            updates = [row for row in rows if row[0] == operation]
            self.assertEqual(len(updates), 1)
            self.assertLess(updates[0][2], source.__code__.co_nlocals)
            if operation == "AddEntry.Put":
                self.assertLess(updates[0][3], source.__code__.co_nlocals)
            for admitted in routes(source.__code__):
                function = types.FunctionType(admitted, globals())
                for incoming in ((), values):
                    result = function(lambda value: value, incoming)
                    self.assertEqual(result, expected if incoming else type(expected)())


if __name__ == "__main__":
    unittest.main()
