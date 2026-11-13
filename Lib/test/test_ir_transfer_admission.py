"""Normal-edge initialization and owner transport under NULL shadow authority."""

import marshal
import opcode
import struct
import types
import unittest

from test.support.bytecode_helper import encode_words
from test.test_ir_admission import construct


HANDLERS = {**opcode.opmap, **opcode._specialized_opmap, **opcode._ir_opmap}


def encoded(words):
    return encode_words(*((HANDLERS[name], *fields) for name, *fields in words))


def make_code(words, *, slots=4, locals=(), consts=(None,)):
    template = (lambda: None).__code__
    return template.replace(co_code=encoded(words), co_stacksize=slots,
                            co_linetable=b"", co_exceptiontable=b"",
                            co_fbinit=struct.pack("<I", 2), co_consts=consts,
                            co_nlocals=len(locals), co_varnames=locals)


class TransferAdmissionTests(unittest.TestCase):
    def assert_rejected(self, words, bad, message, **geometry):
        code = make_code(words, **geometry)
        changed = encoded(bad)
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

    def test_use_after_consumption(self):
        words = [("LOAD_CONST", 0), ("POP_TOP", 0),
                 ("LOAD_CONST", 0), ("RETURN_VALUE", 0)]
        bad = words.copy()
        bad[2] = ("IR_JUMP", 0, 0, 3)
        self.assert_rejected(words, bad, "reads unavailable scratch")

    def test_no_implicit_language_value_at_entry(self):
        words = [("LOAD_CONST", 0), ("RETURN_VALUE", 0)]
        bad = [("IR_JUMP", 0, 0, 1), words[1]]
        self.assert_rejected(words, bad, "reads unavailable scratch")

    def test_null_retirement_is_not_a_language_value_definition(self):
        words = [("POP_TOP", 0), ("LOAD_CONST", 0), ("RETURN_VALUE", 0)]
        self.assert_admitted(words)
        bad = [words[0], ("IR_JUMP", 0, 0, 2), words[2]]
        self.assert_rejected(words, bad, "reads unavailable scratch")

    def test_copy_duplicates_and_swap_transports_owners(self):
        words = [("LOAD_CONST", 0), ("COPY", 0, 1), ("SWAP", 0, 2),
                 ("POP_TOP", 1), ("RETURN_VALUE", 0)]
        self.assert_admitted(words)
        bad = words.copy()
        bad[3] = ("POP_TOP", 0)
        self.assert_rejected(words, bad, "reads unavailable scratch")

    def test_overwrite_and_missing_retirement(self):
        words = [("LOAD_CONST", 0), ("POP_TOP", 0),
                 ("LOAD_CONST", 0), ("RETURN_VALUE", 0)]
        bad = words.copy()
        bad[1] = ("LOAD_CONST", 0)
        self.assert_rejected(words, bad, "overwrites scratch owner")
        words = [("LOAD_CONST", 1), ("POP_TOP", 1),
                 ("LOAD_CONST", 0), ("RETURN_VALUE", 0)]
        bad = words.copy()
        bad[1] = ("IR_JUMP", 0, 0, 2)
        # General frame exit owns remaining scratch. An explicit empty
        # form must still prove that no owner is left behind.
        self.assert_admitted(bad)
        bad[-1] = ("RETURN_VALUE_EMPTY", 0)
        self.assert_rejected(words, bad, "unproved empty return")

    def test_named_binding_move_store_and_checked_unbound_read(self):
        # LOAD_FAST_AND_CLEAR can produce canonical NULL from an unbound local.
        # The value is initialized; STORE_FAST restores that unbound binding.
        words = [("LOAD_FAST_AND_CLEAR", 1, 0), ("STORE_FAST", 1, 0),
                 ("LOAD_FAST_CHECK", 1, 0), ("RETURN_VALUE", 1)]
        self.assert_admitted(words, locals=("a",))
        bad = words.copy()
        bad[2] = ("IR_JUMP", 0, 0, 3)
        self.assert_rejected(words, bad, "reads unavailable scratch", locals=("a",))

    def test_branch_join_waits_for_all_predecessors(self):
        words = [("LOAD_CONST", 0), ("LOAD_CONST", 1),
                 ("POP_JUMP_IF_NONE", 1, 0, 5), ("POP_TOP", 0),
                 ("IR_JUMP", 0, 0, 6), ("POP_TOP", 0),
                 ("LOAD_CONST", 0), ("RETURN_VALUE", 0)]
        self.assert_admitted(words)
        bad = words.copy()
        bad[6] = ("IR_JUMP", 0, 0, 7)
        self.assert_rejected(words, bad, "reads unavailable scratch")
        # One normal edge has no semantic transfer proof. Its touched home
        # becomes unknown, so the other edge's NULL fact cannot reject it.
        unknown = bad.copy()
        unknown[5] = ("GET_ITER", 0, 0, 0)
        self.assert_admitted(unknown)

    def test_loop_fixed_point(self):
        words = [("LOAD_CONST", 0), ("POP_TOP", 0),
                 ("LOAD_CONST", 1), ("POP_JUMP_IF_NONE", 1, 0, 5),
                 ("IR_JUMP", 0, 0, 0), ("LOAD_CONST", 0),
                 ("RETURN_VALUE", 0)]
        self.assert_admitted(words)
        bad = words.copy()
        bad[5] = ("IR_JUMP", 0, 0, 6)
        self.assert_rejected(words, bad, "reads unavailable scratch")

    def test_owner_obligation_on_one_diamond_edge(self):
        # Reverse the branch's sense as well as the owner-producing arm. The
        # result cannot depend on which predecessor the queue visits first.
        for conditional in ("POP_JUMP_IF_NONE", "POP_JUMP_IF_NOT_NONE"):
            for owner_on_taken in (False, True):
                for overwrite in (False, True):
                    with self.subTest(conditional=conditional,
                                      owner_on_taken=owner_on_taken, overwrite=overwrite):
                        words = [("LOAD_CONST", 1), (conditional, 1, 0, 5),
                                 ("LOAD_CONST", 2), ("POP_TOP", 2),
                                 ("IR_JUMP", 0, 0, 7),
                                 ("LOAD_CONST", 2), ("POP_TOP", 2),
                                 ("LOAD_CONST", 2 if overwrite else 0),
                                 ("RETURN_VALUE", 2 if overwrite else 0)]
                        bad = words.copy()
                        release = 6 if owner_on_taken else 3
                        bad[release] = ("IR_JUMP", 0, 0, release + 1)
                        if not overwrite:
                            self.assert_admitted(bad)
                            words[-1] = ("RETURN_VALUE_EMPTY", 0)
                            bad[-1] = ("RETURN_VALUE_EMPTY", 0)
                        message = "overwrites scratch owner" if overwrite else "unproved empty return"
                        self.assert_rejected(words, bad, message)

    def test_unknown_barrier_keeps_only_certified_untouched_null(self):
        words = [("LOAD_CONST", 0), ("POP_TOP", 0),
                 ("LOAD_CONST", 1), ("GET_ITER", 1, 1, 0),
                 ("LOAD_CONST", 0), ("RETURN_VALUE", 0)]
        self.assert_admitted(words)
        bad = words.copy()
        bad[4] = ("IR_JUMP", 0, 0, 5)
        self.assert_rejected(words, bad, "reads unavailable scratch")
        touched = bad.copy()
        touched[3] = ("GET_ITER", 0, 0, 0)
        self.assert_admitted(touched)

    def test_full_scratch_extent_and_many_straight_instructions(self):
        for home in (0, 63, 64, 128, 4094):
            with self.subTest(home=home):
                words = [("LOAD_CONST", home), ("POP_TOP", home),
                         ("LOAD_CONST", home), ("RETURN_VALUE", home)]
                self.assert_admitted(words, slots=4095)
                bad = words.copy()
                bad[2] = ("IR_JUMP", 0, 0, 3)
                self.assert_rejected(words, bad, "reads unavailable scratch", slots=4095)
        words = [("LOAD_CONST", 0), ("POP_TOP", 0)] * 5000
        words += [("LOAD_CONST", 0), ("RETURN_VALUE", 0)]
        self.assert_admitted(words, slots=1)

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
