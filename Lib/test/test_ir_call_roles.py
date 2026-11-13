"""Canonical CALL roles and positive public-image virtual-hole admission."""

import gc
import marshal
import opcode
import sys
import types
import unittest
import weakref

import _opcode
import _testinternalcapi
from test import support
from test.test_ir_admission import construct
from test.test_ir_transfer_admission import encoded, make_code


HANDLERS = {**opcode.opmap, **opcode._specialized_opmap, **opcode._ir_opmap}
CONTRACT = _opcode.get_ir_contract()


def routes(code):
    return (code, construct(code), code.replace(),
            marshal.loads(marshal.dumps(code)))


def call_rows(code, adaptive=False):
    return [row for row in _testinternalcapi.get_ir(code, adaptive)
            if CONTRACT[HANDLERS[row[0]]].base in
            (opcode.opmap["CALL"], opcode.opmap["CALL_KW"],
             opcode.opmap["CALL_FUNCTION_EX"])]


def argument_functions(count):
    names = [f"a{i}" for i in range(count)]
    signature = ", ".join(names)
    arguments = ", ".join(f"values[{i}]" for i in range(count))
    result = "(" + ", ".join(names) + ("," if count == 1 else "") + ")"
    namespace = {}
    exec(f"def target({signature}):\n    return {result}\n"
         f"def method(self{', ' if count else ''}{signature}):\n"
         f"    return self, {result}\n"
         f"def plain(fn, values):\n    return fn({arguments})\n"
         f"def bound(fn, values):\n    return fn({arguments})\n", namespace)
    return namespace


class CallRoleTests(unittest.TestCase):
    def assert_admitted(self, words, **geometry):
        code = make_code(words, **geometry)
        for admitted in routes(code):
            self.assertEqual(admitted.co_code, code.co_code)
            # ExecImage uses the same admitted image and builds its own proof.
            self.assertEqual(_testinternalcapi.get_ir(admitted, True),
                             _testinternalcapi.get_ir(code))
        return code

    def assert_rejected(self, words, bad, message, **geometry):
        code = make_code(words, **geometry)
        replacement = encoded(bad)
        self.assertEqual(len(replacement), len(code.co_code))
        for route in (construct, lambda co, **kw: co.replace(**kw)):
            with self.assertRaisesRegex(ValueError, message):
                route(code, co_code=replacement)
        payload = marshal.dumps(code)
        self.assertEqual(payload.count(code.co_code), 1)
        with self.assertRaisesRegex(ValueError, message):
            marshal.loads(payload.replace(code.co_code, replacement, 1))

    def test_compiler_forms_retain_geometry_and_distinct_roles(self):
        def vector(fn, value):
            return fn(value)
        def keywords(fn, value):
            return fn(value=value)
        def ex(fn, values):
            return fn(*values)
        def ex_kw(fn, values, names):
            return fn(*values, **names)
        def method(owner, value):
            return owner.method(value)
        cases = ((vector, "CALL_NO_SELF", 2),
                 (keywords, "CALL_KW_NO_SELF", 2),
                 (ex, "CALL_EX_NO_SELF_NO_KW", 10),
                 (ex_kw, "CALL_EX_NO_SELF_KW", 2),
                 (method, "CALL", 0))
        for function, name, holes in cases:
            for admitted in routes(function.__code__):
                with self.subTest(function=function.__name__):
                    rows = call_rows(admitted)
                    self.assertEqual(len(rows), 1)
                    self.assertEqual(rows[0][0], name)
                    row = CONTRACT[HANDLERS[name]]
                    self.assertEqual(row.call_input_holes, holes)
                    self.assertEqual(row.roles[0:2],
                                     (_opcode.IR_ROLE_DST, _opcode.IR_ROLE_BASE))

    def test_ex_optional_kwargs_requires_its_own_absence_proof(self):
        words = [("LOAD_CONST", 0), ("LOAD_CONST", 2),
                 ("POP_TOP", 3), ("CALL_EX_NO_SELF_NO_KW", 0, 0, 0),
                 ("RETURN_VALUE", 0)]
        self.assert_admitted(words, slots=5)
        bad = words.copy()
        bad[2] = ("LOAD_CONST", 3)
        self.assert_rejected(words, bad, "virtual call input", slots=5)
        # A present kwargs role has a real value home and keeps that owner.
        bad[3] = ("CALL_EX_NO_SELF_KW", 0, 0, 0)
        self.assert_admitted(bad, slots=5)

    def test_fused_branches_retire_holes_on_both_successors(self):
        def choose(fn, first, second, sentinel):
            if first is sentinel:
                return fn(second)
            if second is sentinel:
                return fn(first)
            return fn(first == second)
        rows = _testinternalcapi.get_ir(choose.__code__)
        self.assertTrue(any(row[0] == "IS_OP_JUMP" for row in rows))
        for admitted in routes(choose.__code__):
            function = types.FunctionType(admitted, globals())
            for first, second, expected in ((None, 17, 17), (17, None, 17),
                                            (17, 17, True), (17, 18, False)):
                self.assertEqual(function(lambda value: value, first, second, None),
                                 expected)

    def test_feedback_encoded_normal_cleanup_uses_the_admitted_operand(self):
        def select(fn, values):
            return fn(values[0])
        def comprehension(values):
            return [value[0] for value in values]
        self.assertTrue(any(CONTRACT[HANDLERS[row[0]]].imm == _opcode.IR_LOC_FBARG
                            for row in _testinternalcapi.get_ir(select.__code__)))
        payload = object()
        for admitted in routes(select.__code__):
            function = types.FunctionType(admitted, globals())
            self.assertIs(function(lambda value: value, (payload,)), payload)
        for admitted in routes(comprehension.__code__):
            function = types.FunctionType(admitted, globals())
            self.assertEqual(function(((payload,),)), [payload])

    def test_absence_is_required_after_unknown_and_at_joins(self):
        words = [("LOAD_CONST", 0), ("POP_TOP", 1),
                 ("CALL_NO_SELF", 0, 0, 0, 0), ("RETURN_VALUE", 0)]
        self.assert_admitted(words)
        bad = words.copy()
        bad[1] = ("LOAD_CONST", 1)
        self.assert_rejected(words, bad, "virtual call input")
        # The producer's owner fact disappears at this unsupported output,
        # but that loss of proof must not certify the hole empty.
        good = [("LOAD_CONST", 0), ("POP_TOP", 1),
                ("POP_TOP", 1), words[2], words[3]]
        bad = [good[0], ("LOAD_CONST", 1), ("UNARY_NOT", 1, 1),
               good[3], good[4]]
        self.assert_rejected(good, bad, "virtual call input")
        joined = [("LOAD_CONST", 0), ("LOAD_CONST", 2),
                  ("POP_JUMP_IF_NONE", 2, 0, 5), ("POP_TOP", 1),
                  ("IR_JUMP", 0, 0, 6), ("POP_TOP", 1),
                  words[2], words[3]]
        self.assert_admitted(joined)
        for arm in (3, 5):
            bad = joined.copy()
            bad[arm] = ("LOAD_CONST", 1)
            self.assert_rejected(joined, bad, "virtual call input")

    def test_real_normal_retirement_makes_reused_holes_available(self):
        self.assert_admitted([
            ("LOAD_CONST", 2), ("BUILD_TUPLE", 0, 2, 1),
            ("LOAD_CONST", 1), ("CALL_NO_SELF", 1, 1, 0, 0),
            ("RETURN_VALUE", 1)], locals=("built",))
        # The first child/native call consumes argument home2. It becomes
        # the next call's virtual self hole, independent of physical residue.
        self.assert_admitted([
            ("LOAD_CONST", 0), ("LOAD_CONST", 2),
            ("CALL_NO_SELF", 0, 0, 1, 0), ("POP_TOP", 0),
            ("LOAD_CONST", 1), ("CALL_NO_SELF", 1, 1, 0, 0),
            ("RETURN_VALUE", 1)])

    def test_through_prefix_is_retained_and_consumed_tail_becomes_empty(self):
        # DICT_MERGE preserves callable/self/args/dict and consumes only the
        # trailing source. That source may become another call's self hole.
        words = [("LOAD_CONST", 0), ("LOAD_CONST", 2),
                 ("LOAD_CONST", 3), ("LOAD_CONST", 4),
                 ("DICT_MERGE", 0, 1), ("POP_TOP", 0),
                 ("POP_TOP", 2), ("CALL_NO_SELF", 3, 3, 0, 0),
                 ("RETURN_VALUE", 3)]
        self.assert_admitted(words, slots=6)
        # The dictionary in the unchanged prefix is not an empty self hole.
        bad = words.copy()
        bad[6] = ("LOAD_CONST", 2)
        bad[7] = ("CALL_NO_SELF", 2, 2, 0, 0)
        bad[8] = ("RETURN_VALUE", 2)
        self.assert_rejected(words, bad, "virtual call input", slots=6)

        def invoke(fn, values, names):
            return fn(*values, **names)
        self.assertTrue(any(row[0] == "DICT_MERGE" for row in
                            _testinternalcapi.get_ir(invoke.__code__)))
        for admitted in routes(invoke.__code__):
            function = types.FunctionType(admitted, globals())
            self.assertEqual(function(lambda *a, **kw: (a, kw), (17,), {"x": 18}),
                             ((17,), {"x": 18}))

    def test_full_extent_and_resumed_recipient_are_not_implicit_holes(self):
        for home in (1, 63, 64, 4094):
            self.assert_admitted([
                ("LOAD_CONST", home - 1),
                ("CALL_NO_SELF", home - 1, home - 1, 0, 0),
                ("RETURN_VALUE", home - 1)], slots=home + 1)
        words = [("LOAD_CONST", 0), ("YIELD_VALUE", 3, 0, 0),
                 ("POP_TOP", 3), ("LOAD_CONST", 0),
                 ("CALL_NO_SELF", 0, 0, 0, 0), ("RETURN_VALUE", 0)]
        bad = words.copy()
        bad[1] = ("YIELD_VALUE", 1, 0, 0)
        self.assert_rejected(words, bad, "virtual call input")

    def test_live_parent_suffix_and_recipient_are_not_child_workspace(self):
        words = [("LOAD_CONST", 0), ("POP_TOP", 3),
                 ("CALL_NO_SELF", 0, 0, 0, 0), ("RETURN_VALUE", 0)]
        bad = words.copy()
        bad[1] = ("LOAD_CONST", 3)
        self.assert_rejected(words, bad, "child clobber home")
        bad[2] = ("CALL_NO_SELF", 3, 0, 0, 0)
        bad[3] = ("RETURN_VALUE", 3)
        self.assert_rejected(words, bad, "call recipient")

    def test_real_loops_exception_handlers_and_generators_are_admitted(self):
        def sequence(fn, values):
            result = []
            for value in values:
                try:
                    result.append(fn(tuple((value + 0,))))
                except LookupError:
                    result.append(fn(()))
            return result
        def generator(fn, values):
            for value in values:
                yield fn(value)
        for admitted in routes(sequence.__code__):
            self.assertTrue(any("NO_SELF" in row[0] for row in call_rows(admitted)))
            function = types.FunctionType(admitted, globals())
            self.assertEqual(function(lambda value: value, (1, 2)), [(1,), (2,)])
        for admitted in routes(generator.__code__):
            function = types.FunctionType(admitted, globals())
            self.assertEqual(list(function(lambda value: value + 1, (1, 2))), [2, 3])

    def test_loop_completion_releases_cursor_only_on_the_exit_edge(self):
        words = [("LOAD_CONST", 0), ("FOR_ITER", 2, 0, 0, 4),
                 ("POP_TOP", 1), ("POP_TOP", 2),
                 ("CALL_NO_SELF", 0, 0, 0, 0), ("RETURN_VALUE", 0),
                 ("LOAD_CONST", 0), ("CALL_NO_SELF", 0, 0, 0, 0),
                 ("RETURN_VALUE", 0)]
        self.assert_admitted(words)
        bad = words.copy()
        bad[2] = ("IR_JUMP", 0, 0, 3)
        self.assert_rejected(words, bad, "virtual call input")

        def invoke(fn, values):
            for value in values:
                fn(value)
            return fn(17)
        rows = _testinternalcapi.get_ir(invoke.__code__)
        self.assertTrue(any(row[0] == "FOR_ITER" for row in rows))
        self.assertGreaterEqual(sum("NO_SELF" in row[0] for row in rows), 2)
        for admitted in routes(invoke.__code__):
            function = types.FunctionType(admitted, globals())
            for values, expected in (((), [17]), ((1, 2), [1, 2, 17]),
                                     ((item for item in (1, 2)), [1, 2, 17])):
                seen = []
                self.assertEqual(function(lambda value: seen.append(value) or value,
                                          values), 17)
                self.assertEqual(seen, expected)

    def test_early_loop_retirement_covers_its_single_observed_successor(self):
        # Exercise the actual pair retirement regardless of whether the
        # frontend folds an early loop exit into scalar POP_TOP operations.
        words = [("LOAD_CONST", 0), ("LOAD_CONST", 1),
                 ("POP_ITER", 0, 1), ("LOAD_CONST", 0),
                 ("CALL_NO_SELF", 0, 0, 0, 0), ("RETURN_VALUE", 0)]
        self.assert_admitted(words)
        bad = words.copy()
        bad[2] = ("POP_TOP", 0)
        self.assert_rejected(words, bad, "virtual call input")
        def invoke(fn, values):
            for value in values:
                if value:
                    break
            return fn(17)
        for admitted in routes(invoke.__code__):
            function = types.FunctionType(admitted, globals())
            for values in ((), (0,), (0, 1, 2)):
                self.assertEqual(function(lambda value: value, values), 17)

    def test_local_loop_result_does_not_invent_a_scratch_output(self):
        words = [("LOAD_CONST", 1), ("POP_TOP", 3),
                 ("FOR_ITER", 0, 1, 0, 1), ("IR_JUMP", 0, 0, 2),
                 ("LOAD_CONST", 1), ("CALL_NO_SELF", 1, 1, 0, 0),
                 ("RETURN_VALUE", 1)]
        self.assert_admitted(words, locals=("value",))
        bad = words.copy()
        bad[1] = ("LOAD_CONST", 3)
        self.assert_rejected(words, bad, "child clobber home", locals=("value",))

        def invoke(fn, values):
            for value in values:
                if value:
                    continue
            return fn(17)
        rows = _testinternalcapi.get_ir(invoke.__code__)
        self.assertTrue(any(row[0] == "FOR_ITER" and
                            row[1] < invoke.__code__.co_nlocals for row in rows))
        for admitted in routes(invoke.__code__):
            function = types.FunctionType(admitted, globals())
            for values in ((), (0, 1), (item for item in (0, 1))):
                self.assertEqual(function(lambda value: value, values), 17)

    def test_specialization_deopt_trace_and_profile_preserve_roles(self):
        def invoke(fn, value):
            return fn(value)
        def first(value):
            return value
        class Other:
            def __call__(self, value):
                return value
        payload = object()
        for i in range(200):
            self.assertIs(invoke(first, payload), payload)
        self.assertTrue(all(CONTRACT[HANDLERS[row[0]]].call_input_holes == 2
                            for row in call_rows(invoke.__code__, True)))
        for target in (Other(), first):
            self.assertIs(invoke(target, payload), payload)
        def hook(*args):
            return hook
        for setter in (sys.settrace, sys.setprofile):
            old = sys.gettrace() if setter is sys.settrace else sys.getprofile()
            try:
                setter(hook)
                self.assertIs(invoke(first, payload), payload)
            finally:
                setter(old)
        self.assertEqual(call_rows(invoke.__code__)[0][0], "CALL_NO_SELF")

    def test_exact_and_bound_specializations_keep_zero_two_and_many_arguments(self):
        for count in (0, 2, 16):
            namespace = argument_functions(count)
            values = tuple(object() for _ in range(count))
            receiver = object()
            bound = types.MethodType(namespace["method"], receiver)
            for name in ("plain", "bound"):
                self.assertEqual(call_rows(namespace[name].__code__)[0][0], "CALL_NO_SELF")
            for _ in range(200):
                self.assertEqual(namespace["plain"](namespace["target"], values), values)
                actual = namespace["bound"](bound, values)
                self.assertIs(actual[0], receiver)
                self.assertEqual(actual[1], values)
            if not support.Py_GIL_DISABLED:
                self.assertEqual(call_rows(namespace["plain"].__code__, True)[0][0],
                                 "CALL_NO_SELF_PY_EXACT_ARGS")
                self.assertEqual(call_rows(namespace["bound"].__code__, True)[0][0],
                                 "CALL_NO_SELF_BOUND_METHOD_EXACT_ARGS")

    def test_large_explicit_calls_use_ex_roles(self):
        # The frontend's stack-use limit routes this source through EX,
        # independently of the exact argument counts of its Python targets.
        namespace = argument_functions(33)
        values = tuple(object() for _ in range(33))
        receiver = object()
        bound = types.MethodType(namespace["method"], receiver)
        for name in ("plain", "bound"):
            self.assertEqual(call_rows(namespace[name].__code__)[0][0],
                             "CALL_EX_NO_SELF_NO_KW")
        for _ in range(200):
            self.assertEqual(namespace["plain"](namespace["target"], values), values)
            actual = namespace["bound"](bound, values)
            self.assertIs(actual[0], receiver)
            self.assertEqual(actual[1], values)
        if not support.Py_GIL_DISABLED:
            self.assertEqual(call_rows(namespace["plain"].__code__, True)[0][0],
                             "CALL_EX_NO_SELF_NO_KW_PY")
            self.assertEqual(call_rows(namespace["bound"].__code__, True)[0][0],
                             "CALL_EX_NO_SELF_NO_KW_NON_PY_GENERAL")

    def test_native_prepend_uses_owned_copy_without_arena_offset(self):
        receiver = object()
        payloads = tuple(object() for _ in range(33))
        observed = []
        def target(self, *values, **names):
            self_frame = sys._getframe(1)
            if "values" in self_frame.f_locals:
                self_frame.f_locals["values"] = None
            gc.collect()
            observed.append((self, values, names))
            return observed[-1]
        provider = _testinternalcapi.VCallPrepend(target, receiver)
        def wrapper(count):
            # Explicit CALL_KW keeps the actual argument window in the arena.
            # CALL_EX with **names legitimately builds its own native array
            # and grants OFFSET, which is a different capability.
            arguments = ", ".join(f"values[{i}]" for i in range(count))
            if arguments:
                arguments += ", "
            namespace = {}
            exec("def invoke(fn, values):\n"
                 f"    result = fn({arguments}named=values[-1])\n"
                 "    return result, values\n", namespace)
            return namespace["invoke"]
        for count in (0, 1, 16):
            invoke = wrapper(count)
            self.assertEqual(call_rows(invoke.__code__)[0][0], "CALL_KW_NO_SELF")
            result, replaced = invoke(provider, payloads)
            self.assertIsNone(replaced)
            self.assertIs(result[0], receiver)
            self.assertEqual(result[1], payloads[:count])
            self.assertIs(result[2]["named"], payloads[-1])
        def failure(self, *values, **names):
            gc.collect()
            raise LookupError("prepend target failure")
        with self.assertRaisesRegex(LookupError, "prepend target failure"):
            wrapper(16)(_testinternalcapi.VCallPrepend(failure, receiver), payloads)

    def test_method_promotion_survives_last_wrapper_callback(self):
        class Owner:
            def method(self, value):
                return value
        def replacement(self, value):
            return self, value
        owner = Owner()
        active = False
        references = []
        callbacks = []
        def changed(reference):
            if active:
                callbacks.append(reference)
                Owner.method.__code__ = replacement.__code__
        def factory():
            method = owner.method
            references.append(weakref.ref(method, changed))
            return method
        def invoke(value):
            return factory()(value)
        payload = object()
        for _ in range(200):
            self.assertIs(invoke(payload), payload)
        active = True
        result = invoke(payload)
        self.assertEqual(len(callbacks), 1)
        self.assertIs(result[0], owner)
        self.assertIs(result[1], payload)


if __name__ == "__main__":
    unittest.main()
