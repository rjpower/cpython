"""Encoded register-image admission through public and serialized code paths."""

import dis
import marshal
import opcode
import struct
import types
import unittest

import _opcode
from test.support import import_helper
from test.support.bytecode_helper import encode_words


_testinternalcapi = import_helper.import_module("_testinternalcapi")
CONTRACT = _opcode.get_ir_contract()
HANDLERS = {**opcode.opmap, **opcode._specialized_opmap, **opcode._ir_opmap}


def construct(code, **changes):
    fields = (
        "argcount", "posonlyargcount", "kwonlyargcount", "nlocals",
        "stacksize", "flags", "code", "consts", "names", "varnames",
        "filename", "name", "qualname", "firstlineno", "linetable",
        "exceptiontable", "freevars", "cellvars", "fbinit", "constslots",
    )
    return types.CodeType(*(changes.get("co_" + field,
                                      getattr(code, "co_" + field))
                            for field in fields))


def replace_word(code, index, word):
    handler, *fields = word
    if isinstance(handler, str):
        handler = HANDLERS[handler]
    word = (handler, *fields)
    return code.co_code[:8 * index] + encode_words(word) + code.co_code[8 * (index + 1):]


def replace_field(word, index, value):
    fields = list(word)
    fields[index + 1] = value
    return fields


class AdmissionTests(unittest.TestCase):
    def assert_rejected(self, code, replacement, message, field="co_code"):
        original = getattr(code, field)
        for route in (construct, lambda co, **changes: co.replace(**changes)):
            with self.subTest(route=route):
                with self.assertRaisesRegex(ValueError, message):
                    route(code, **{field: replacement})
        self.assertEqual(getattr(code, field), original)

        # Preserve all marshal lengths and references, changing only the
        # encoded image/table. The malformed object never reaches execution.
        self.assertEqual(len(original), len(replacement))
        payload = marshal.dumps(code)
        self.assertEqual(payload.count(original), 1)
        payload = payload.replace(original, replacement, 1)
        with self.assertRaisesRegex(ValueError, message):
            marshal.loads(payload)

    def call_code(self):
        def f(a):
            return a.target(1000)
        return f.__code__.replace(co_linetable=b"", co_exceptiontable=b"")

    def test_encoded_domains(self):
        code = self.call_code()
        words = _testinternalcapi.get_ir(code)
        nl = len(_testinternalcapi.get_co_localskinds(code))
        limit = nl + code.co_stacksize
        reached = set()
        for index, word in enumerate(words):
            name, *fields = word
            row = CONTRACT[HANDLERS[name]]
            if name == "CALL":
                reached.add(name)
                base = row.roles.index(_opcode.IR_ROLE_BASE)
                bad = replace_field(word, base, limit - 1)
                self.assert_rejected(code, replace_word(code, index, bad), "window")
            if name == "LOAD_ATTR":
                reached.add(name)
                fb = row.roles.index(_opcode.IR_ROLE_FB)
                self.assert_rejected(code, replace_word(code, index,
                    replace_field(word, fb, 4095)), "feedback")
                self.assert_rejected(code, replace_word(code, index,
                    replace_field(word, 0, limit - 1)), "output window")
                imm = row.imm - _opcode.IR_LOC_A
                self.assert_rejected(code, replace_word(code, index,
                    replace_field(word, imm, 4094)), "name index")
            if name in {"RETURN_VALUE", "RETURN_VALUE_EMPTY"}:
                reached.add("RETURN_VALUE")
                source = next(k for k, role in enumerate(row.roles)
                              if role >= _opcode.IR_ROLE_IN0)
                self.assert_rejected(code, replace_word(code, index,
                    replace_field(word, source, 4095)), "slot")
        self.assertEqual(reached, {"CALL", "LOAD_ATTR", "RETURN_VALUE"})

    def test_local_and_constant_indices(self):
        code = self.call_code()
        words = _testinternalcapi.get_ir(code)
        nl = len(_testinternalcapi.get_co_localskinds(code))
        reached = False
        for index, word in enumerate(words):
            if word[0].startswith("LOAD_CONST"):
                reached = True
                bad = (word[0], word[1], word[2], 4095, 4095)
                self.assert_rejected(code, replace_word(code, index, bad), "constant index")
        self.assertTrue(reached)
        # The scalar local role must remain inside localsplus even when a
        # scratch home of that number exists in the same frame.
        bad = ("LOAD_FAST_CHECK", nl, nl)
        self.assert_rejected(code, replace_word(code, 0, bad), "local slot")

    def test_absolute_branch_targets(self):
        code = self.call_code()
        for name in ("IR_JUMP", "POP_JUMP_IF_FALSE"):
            handler = opcode.opmap.get(name, opcode._ir_opmap.get(name))
            row = CONTRACT[handler]
            fields = [0, 0, 0, 0]
            # Preserve an available feedback entry for the conditional form.
            if row.fb != _opcode.IR_LOC_NONE:
                fields[row.fb - _opcode.IR_LOC_A] = 0
            if row.imm == _opcode.IR_LOC_CD:
                fields[2:] = [4095, 4095]
            else:
                fields[row.imm - _opcode.IR_LOC_A] = 2047
            with self.subTest(name=name):
                self.assert_rejected(code, replace_word(code, 0,
                    (handler, *fields)), "branch target")

    def test_relative_branch_target(self):
        def f(a):
            for value in a:
                value.target()
        code = f.__code__.replace(co_linetable=b"", co_exceptiontable=b"")
        words = _testinternalcapi.get_ir(code)
        index = next(i for i, word in enumerate(words) if word[0] == "FOR_ITER")
        self.assert_rejected(code, replace_word(code, index,
            replace_field(words[index], 3, 2047)), "branch target")

    def test_feedback_kind(self):
        code = self.call_code()
        table = bytearray(code.co_fbinit)
        # The first real entry belongs to LOAD_ATTR. A CALL entry can fit in
        # the same allocation but cannot satisfy its typed feedback contract.
        word, = struct.unpack_from("<I", table, 4)
        call_kind = dis._FEEDBACK_KINDS.index("CALL")
        struct.pack_into("<I", table, 4, (word & 0x0fffffff) | call_kind << 28)
        self.assert_rejected(code, bytes(table), "feedback", "co_fbinit")

    def test_frame_limit_before_integer_addition(self):
        code = self.call_code()
        for route in (construct, lambda co, **changes: co.replace(**changes)):
            with self.subTest(route=route):
                with self.assertRaisesRegex(ValueError, "register limit"):
                    route(code, co_stacksize=2**31 - 1)

    def test_inline_constant_normalization(self):
        def f():
            return 1000
        code = f.__code__
        words = _testinternalcapi.get_ir(code)
        self.assertIn("LOAD_CONST_INLINE_INT", [word[0] for word in words])
        constants = tuple("x" * 1000 if item == 1000 else item
                          for item in code.co_consts)
        for route in (construct, lambda co, **changes: co.replace(**changes)):
            with self.subTest(route=route):
                changed = route(code, co_consts=constants)
                self.assertNotIn("LOAD_CONST_INLINE_INT",
                    [word[0] for word in _testinternalcapi.get_ir(changed)])
                self.assertEqual(types.FunctionType(changed, {})(), "x" * 1000)
                restored = marshal.loads(marshal.dumps(changed))
                self.assertEqual(types.FunctionType(restored, {})(), "x" * 1000)

        # Replace a same-sized constant record in a valid marshal stream,
        # retaining its original inline-int selector until common admission.
        replacement = "zzz"
        before = marshal.dumps(1000)
        after = marshal.dumps(replacement)
        self.assertEqual(len(before), len(after))
        payload = marshal.dumps(code)
        self.assertEqual(payload.count(before), 1)
        restored = marshal.loads(payload.replace(before, after, 1))
        self.assertNotIn("LOAD_CONST_INLINE_INT",
            [word[0] for word in _testinternalcapi.get_ir(restored)])
        self.assertEqual(types.FunctionType(restored, {})(), replacement)

    def test_valid_compiled_images_roundtrip(self):
        def f(a):
            try:
                return [value.target(1000) for value in a]
            except LookupError:
                return []
        for code in (f.__code__, compile("def g():\n yield from ()\n", "<admission>", "exec")):
            with self.subTest(name=code.co_name):
                self.assertEqual(construct(code), code)
                self.assertEqual(code.replace(), code)
                self.assertEqual(marshal.loads(marshal.dumps(code)), code)


if __name__ == "__main__":
    unittest.main()
