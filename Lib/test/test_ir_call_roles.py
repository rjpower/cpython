"""Canonical CALL roles of lowered calls and their virtual holes."""

import gc
import marshal
from test.support import ir_dis as opcode
import sys
import types
import unittest
import weakref

from test.support import ir_dis as dis
import _opcode
import _testinternalcapi
from test import support
from test.support import import_helper


CONTRACT = _opcode.get_ir_contract()


def routes(code):
    return (code, code.replace(), marshal.loads(marshal.dumps(code)))


def contract_of(name):
    "The contract row of the handler of a word named name"
    return CONTRACT[dis._handler_field(name) & _opcode.IR_HANDLER_MASK]


def role_flags(name):
    "The call-role flags of a word named name"
    return dis._handler_field(name) & _opcode.IR_FLAGS_MASK


def call_rows(code, adaptive=False):
    # An OPERAND word (an arm's operand) has no handler
    return [row for row in _testinternalcapi.get_ir(code, adaptive)
            if row[0] != "OPERAND" and contract_of(row[0]).base in
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
        no_self, no_kw = _opcode.IR_F_NO_SELF, _opcode.IR_F_NO_KW
        cases = ((vector, "Call.Generic+NO_SELF", no_self),
                 (keywords, "CALL_KW+NO_SELF", no_self),
                 (ex, "CALL_FUNCTION_EX+NO_KW", no_kw),
                 (ex_kw, "CALL_FUNCTION_EX", 0),
                 (method, "Call.Generic", 0))
        for function, name, flags in cases:
            for admitted in routes(function.__code__):
                with self.subTest(function=function.__name__):
                    rows = call_rows(admitted)
                    self.assertEqual(len(rows), 1)
                    self.assertEqual(rows[0][0], name)
                    self.assertEqual(role_flags(name), flags)
                    row = contract_of(name)
                    self.assertEqual(row.word_flags & flags, flags)
                    # The callable is an input; the operands follow the head
                    self.assertEqual(row.roles[0:2],
                                     (_opcode.IR_ROLE_DST, _opcode.IR_ROLE_IN0))

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
        self.assertTrue(any(contract_of(row[0]).imm == _opcode.IR_LOC_FBARG
                            for row in _testinternalcapi.get_ir(select.__code__)))
        payload = object()
        for admitted in routes(select.__code__):
            function = types.FunctionType(admitted, globals())
            self.assertIs(function(lambda value: value, (payload,)), payload)
        for admitted in routes(comprehension.__code__):
            function = types.FunctionType(admitted, globals())
            self.assertEqual(function(((payload,),)), [payload])

    def test_through_prefix_is_retained_and_consumed_tail_becomes_empty(self):
        def invoke(fn, values, names):
            return fn(*values, **names)
        self.assertTrue(any(row[0] == "MergeKeywords.Merge" for row in
                            _testinternalcapi.get_ir(invoke.__code__)))
        for admitted in routes(invoke.__code__):
            function = types.FunctionType(admitted, globals())
            self.assertEqual(function(lambda *a, **kw: (a, kw), (17,), {"x": 18}),
                             ((17,), {"x": 18}))

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
        self.assertTrue(all(role_flags(row[0]) == _opcode.IR_F_NO_SELF
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
        self.assertEqual(call_rows(invoke.__code__)[0][0], "Call.Generic+NO_SELF")

    def test_exact_and_bound_specializations_keep_zero_two_and_many_arguments(self):
        for count in (0, 2, 16):
            namespace = argument_functions(count)
            values = tuple(object() for _ in range(count))
            receiver = object()
            bound = types.MethodType(namespace["method"], receiver)
            for name in ("plain", "bound"):
                self.assertEqual(call_rows(namespace[name].__code__)[0][0], "Call.Generic+NO_SELF")
            for _ in range(200):
                self.assertEqual(namespace["plain"](namespace["target"], values), values)
                actual = namespace["bound"](bound, values)
                self.assertIs(actual[0], receiver)
                self.assertEqual(actual[1], values)
            if not support.Py_GIL_DISABLED:
                self.assertEqual(call_rows(namespace["plain"].__code__, True)[0][0],
                                 "Call.Exact+NO_SELF")
                self.assertEqual(call_rows(namespace["bound"].__code__, True)[0][0],
                                 "Call.BoundExact+NO_SELF")

    def test_large_explicit_calls_use_ex_roles(self):
        # The frontend's stack-use limit routes this source through EX,
        # independently of the exact argument counts of its Python targets.
        namespace = argument_functions(33)
        values = tuple(object() for _ in range(33))
        receiver = object()
        bound = types.MethodType(namespace["method"], receiver)
        for name in ("plain", "bound"):
            self.assertEqual(call_rows(namespace[name].__code__)[0][0],
                             "CALL_FUNCTION_EX+NO_KW")
        for _ in range(200):
            self.assertEqual(namespace["plain"](namespace["target"], values), values)
            actual = namespace["bound"](bound, values)
            self.assertIs(actual[0], receiver)
            self.assertEqual(actual[1], values)
        if not support.Py_GIL_DISABLED:
            self.assertEqual(call_rows(namespace["plain"].__code__, True)[0][0],
                             "CALL_EX_PY+NO_KW")
            self.assertEqual(call_rows(namespace["bound"].__code__, True)[0][0],
                             "CALL_EX_NON_PY_GENERAL+NO_KW")

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
            self.assertEqual(call_rows(invoke.__code__)[0][0], "CALL_KW+NO_SELF")
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
        # The method operand is released after the call bound the function
        # and the receiver, with the callee's frame current and not started:
        # the callback that replaces the code runs too late to change it
        self.assertIs(result, payload)

    def specialized(self, function):
        "the specialized name of function's one call word"
        return call_rows(function.__code__, True)[0][0]

    def test_bound_method_operands_keep_their_values(self):
        # A local callable (m(x), local B, no self slot) and a temporary
        # one (h.cb(x)): the push takes the method's function and receiver,
        # and the operands keep the method
        class Receiver:
            def method(self, value):
                return self, value
        receiver = Receiver()
        holder = types.SimpleNamespace(cb=receiver.method)
        def local(m, value):
            result = m(value)
            return result, m
        def temporary(h, value):
            return h.cb(value)
        m = receiver.method
        for _ in range(200):
            result, after = local(m, 1)
            self.assertEqual(result, (receiver, 1))
            self.assertIs(after, m)
            self.assertEqual(temporary(holder, 2), (receiver, 2))
        self.assertIs(holder.cb.__self__, receiver)
        if not support.Py_GIL_DISABLED:
            self.assertEqual(self.specialized(local), "Call.BoundExact+NO_SELF")
            self.assertEqual(self.specialized(temporary), "Call.BoundExact")

    def check_released(self, make, invoke, misses):
        """invoke(m) after specializing on make(): every call in misses
        gives the generic call's result, and the method and its receiver
        are released once"""
        def run():
            receiver, m = make()
            for _ in range(200):
                invoke(m, receiver)
            for miss in misses:
                miss(m, receiver)
            gone = weakref.ref(receiver)
            del receiver, m
            gc.collect()
            self.assertIsNone(gone())
        run()

    def test_bound_method_guards_miss_to_the_generic_call(self):
        class Receiver:
            def method(self, value):
                return self, value
        def two(self, a, b):
            return self, a, b
        def call(m, value):
            return m(value)
        def make():
            receiver = Receiver()
            return receiver, receiver.method
        def invoke(m, receiver):
            self.assertEqual(call(m, 1), (receiver, 1))
        def other_function(m, receiver):
            # Call.BoundExact's version guard: another function
            other = types.MethodType(lambda self, v: ("other", self, v), receiver)
            self.assertEqual(call(other, 2), ("other", receiver, 2))
        def wrong_arity(m, receiver):
            # The version guard misses, then the generic call raises: the
            # function takes another number of arguments
            with self.assertRaises(TypeError):
                call(types.MethodType(two, receiver), 3)
        def weakly_referenced(m, receiver):
            # Call.BoundExact: a method with a weakref
            method = types.MethodType(Receiver.method, receiver)
            ref = weakref.ref(method)
            self.assertEqual(call(method, 4), (receiver, 4))
            self.assertIs(ref(), method)
        def replaced_code(m, receiver):
            # The function's version changes with its code
            function = m.__func__
            saved = function.__code__
            function.__code__ = (lambda self, v: ("new", v)).__code__
            try:
                self.assertEqual(call(m, 5), ("new", 5))
            finally:
                function.__code__ = saved
            self.assertEqual(call(m, 6), (receiver, 6))
        self.check_released(make, invoke, [other_function, wrong_arity,
                                           weakly_referenced, replaced_code])
        if not support.Py_GIL_DISABLED:
            self.assertIn(self.specialized(call), ("Call.BoundExact+NO_SELF",
                                                   "Call.Generic+NO_SELF"))

    def test_bound_method_stack_space_and_recursion_limit(self):
        # Call.BoundExact's stack-space guard: deep recursion through a bound method
        # with a large frame crosses data stack chunks;
        # _CHECK_RECURSION_REMAINING: the limit raises RecursionError
        names = ", ".join(f"v{i}" for i in range(60))
        namespace = {}
        exec(f"def method(self, n):\n"
             f"    {names} = (None,) * 60\n"
             f"    if n == 0:\n        return self\n"
             f"    return self.m(n - 1)\n", namespace)
        class Holder:
            pass
        def run(depth, refs):
            receiver = Holder()
            receiver.m = types.MethodType(namespace["method"], receiver)
            refs.append(weakref.ref(receiver))
            try:
                return receiver.m(depth) is receiver
            finally:
                receiver.m = None
        refs = []
        for _ in range(3):
            self.assertTrue(run(400, refs))
        with support.infinite_recursion(200):
            with self.assertRaises(RecursionError):
                run(10_000, refs)
        self.assertTrue(run(400, refs))
        gc.collect()
        self.assertEqual([ref() for ref in refs], [None] * len(refs))

    def test_class_call_counts_init_and_releases_the_class(self):
        # Call.Init: the helper counts the cached __init__,
        # allocates the instance and releases the class operand after the
        # push; the arguments and the class keep their counts
        class Point:
            def __init__(self, x, y):
                self.x, self.y = x, y
        def make(cls, x, y):
            return cls(x, y)
        x, y = object(), object()
        counts = sys.getrefcount(Point), sys.getrefcount(x), sys.getrefcount(y)
        for _ in range(200):
            p = make(Point, x, y)
            self.assertIs(p.x, x)
            self.assertIs(p.y, y)
            self.assertIs(type(p), Point)
        del p
        gc.collect()
        self.assertEqual((sys.getrefcount(Point), sys.getrefcount(x), sys.getrefcount(y)),
                         counts)
        if not support.Py_GIL_DISABLED:
            self.assertEqual(self.specialized(make), "Call.Init+NO_SELF")

    def test_class_call_guards_miss_before_any_count(self):
        # A miss at Call.Init's class version guard or its __init__ guard
        # (a deep __init__ whose frame needs another
        # data stack chunk), or at the recursion check, leaves the class
        # and the arguments as they were: the generic call gives the result
        names = ", ".join(f"v{i}" for i in range(60))
        namespace = {}
        exec(f"def __init__(self, n, arg):\n"
             f"    {names} = (None,) * 60\n"
             f"    self.arg = arg\n"
             f"    self.child = type(self)(n - 1, arg) if n else None\n", namespace)
        class Arg:
            pass
        def make(cls, n, value):
            return cls(n, value)
        def calls(Deep, arg):
            for _ in range(200):
                self.assertIs(make(Deep, 0, arg).arg, arg)
            # The class's version changes
            Deep.extra = 1
            self.assertIs(make(Deep, 0, arg).arg, arg)
            for _ in range(200):
                self.assertIs(make(Deep, 0, arg).arg, arg)
            # The cached __init__ is called another way: the generic call
            # runs the replacement, whose result is not None
            _testcapi = import_helper.import_module("_testcapi")
            _testcapi.set_vectorcall_replacement(Deep.__init__)
            try:
                with self.assertRaises(TypeError):
                    make(Deep, 0, arg)
            finally:
                _testcapi.restore_vectorcall(Deep.__init__)
            self.assertIs(make(Deep, 0, arg).arg, arg)
            # Recursion 300 deep through the class call crosses data stack
            # chunks
            node = make(Deep, 300, arg)
            depth = 0
            while node is not None:
                self.assertIs(node.arg, arg)
                node, depth = node.child, depth + 1
            self.assertEqual(depth, 301)
            with support.infinite_recursion(100):
                with self.assertRaises(RecursionError):
                    make(Deep, 10_000, arg)
        Deep = type("Deep", (), {"__init__": namespace["__init__"]})
        arg = Arg()
        gc.collect()
        counts = sys.getrefcount(Deep), sys.getrefcount(arg)
        calls(Deep, arg)
        gc.collect()
        self.assertEqual((sys.getrefcount(Deep), sys.getrefcount(arg)), counts)
        if not support.Py_GIL_DISABLED:
            self.assertIn(self.specialized(make), ("Call.Init+NO_SELF",
                                                   "Call.Generic+NO_SELF"))
        gone = weakref.ref(Deep), weakref.ref(arg)
        del Deep, arg
        gc.collect()
        self.assertEqual([ref() for ref in gone], [None, None])

    def test_class_call_failure_leaves_the_operands_to_the_unwinder(self):
        # A binding error in the helper (create_init_frame_window: the
        # cached __init__ takes another number of arguments) raises at the
        # word, which stays specialized; the class, the argument and each
        # instance are released once
        freed = []
        class Missing:
            def __init__(self, a, b):
                self.a = a
            def __del__(self):
                freed.append(1)
        def make(cls, value):
            return cls(value)
        def calls(arg):
            for _ in range(300):
                with self.assertRaises(TypeError):
                    make(Missing, arg)
        arg = object()
        counts = sys.getrefcount(Missing), sys.getrefcount(arg)
        calls(arg)
        gc.collect()
        self.assertEqual((sys.getrefcount(Missing), sys.getrefcount(arg)), counts)
        self.assertEqual(len(freed), 300)
        if not support.Py_GIL_DISABLED:
            self.assertEqual(self.specialized(make), "Call.Init+NO_SELF")

if __name__ == "__main__":
    unittest.main()
