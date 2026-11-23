"""Normal-edge initialization and owner transport under NULL shadow authority."""

import marshal
import types
import unittest

from test.test_ir_admission import construct


class TransferAdmissionTests(unittest.TestCase):
    def test_compiled_implicit_call_null_and_failure_edges(self):
        def f(call, value):
            try:
                result = call(value)
            except LookupError:
                return None
            return result
        code = self.assert_admitted_compiled(f.__code__)
        self.assertEqual(types.FunctionType(code, {})(lambda value: value, 7), 7)
        def fail(value):
            raise LookupError
        self.assertIsNone(types.FunctionType(code, {})(fail, 7))

    def assert_admitted_compiled(self, code):
        self.assertEqual(construct(code), code)
        self.assertEqual(code.replace(), code)
        restored = marshal.loads(marshal.dumps(code))
        self.assertEqual(restored, code)
        return restored


if __name__ == "__main__":
    unittest.main()
