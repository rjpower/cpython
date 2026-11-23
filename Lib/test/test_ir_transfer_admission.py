"""Normal-edge initialization and owner transport under NULL shadow authority."""

from test.support import ir_dis as dis
import marshal
from test.support import ir_dis as opcode
import struct
import types
import unittest

import _testinternalcapi

from test.support.ir_dis import encode_words
from test.test_ir_admission import construct


HANDLERS = {**opcode.opmap, **opcode._specialized_opmap, **opcode._ir_opmap}


def raw_encoded(words):
    "co_code for words, which name the slots of the frame as it is"
    return encode_words(*((dis._handler_field(name), *fields)
                          for name, *fields in words))


def encoded(words, nlocals=0):
    """co_code for words written by hand, which name temporaries as
    nlocals + k, as they did before the discard slot"""
    return encode_words(*shift_for_discard_slot(words, nlocals))


# The words of these tests name the slots of a frame with the discard slot
# only, so the template reads no constant: `return None` would take a
# constant slot.
template_code = (lambda: undefined).__code__


def make_code(words, *, slots=4, locals=(), consts=(None,)):
    code = template_code.replace(co_code=encoded(words, len(locals)), co_stacksize=slots,
                            co_linetable=b"", co_exceptiontable=b"",
                            co_fbinit=struct.pack("<I", 2), co_consts=consts,
                            co_nlocals=len(locals), co_varnames=locals)
    # The compiler selects the empty returns; constructors keep the words
    # they are given.
    return code.replace(co_code=_testinternalcapi.select_empty_returns(code))


class TransferAdmissionTests(unittest.TestCase):
    def assert_rejected(self, words, bad, message, **geometry):
        code = make_code(words, **geometry)
        changed = encoded(bad, len(geometry.get("locals", ())))
        self.assertEqual(len(changed), len(code.co_code))
        for route in (construct, lambda co, **kw: co.replace(**kw)):
            with self.subTest(route=route):
                with self.assertRaisesRegex(ValueError, message):
                    route(code, co_code=changed)
        payload = marshal.dumps(code)
        self.assertEqual(payload.count(code.co_code), 1)
        with self.assertRaisesRegex(ValueError, message):
            marshal.loads(payload.replace(code.co_code, changed, 1))

    def assert_admitted(self, words, **geometry):
        code = make_code(words, **geometry)
        self.assertEqual(construct(code), code)
        self.assertEqual(code.replace(), code)
        self.assertEqual(marshal.loads(marshal.dumps(code)), code)
        return code

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
