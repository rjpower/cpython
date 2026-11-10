"""Whole-body guarded leaf calls and their nonfinal retirement boundary."""

import opcode
import sys
import types
import unittest
import weakref

from test.support import import_helper, requires_subprocess, reset_code, script_helper
from test.support.bytecode_helper import encode_words

_testinternalcapi = import_helper.import_module("_testinternalcapi")
ROUNDS = _testinternalcapi.SPECIALIZATION_THRESHOLD + 100


class Limit:
    selected = 1


class Record:
    def __init__(self):
        self.direction = 1
        self.first = object()
        self.second = object()

    def select(self):
        if self.direction == Limit.selected:
            return self.first
        return self.second


def info(function):
    """Return certified, hits, probe/owner/entry misses, and build support."""
    return _testinternalcapi.ir_leaf_info(function.__code__)


def fresh_function(code, namespace):
    """Create a versioned target through MAKE_FUNCTION, preserving its code."""
    assert not code.co_freevars

    def make():
        def target():
            pass
        return target

    constants = tuple(code if isinstance(value, types.CodeType) else value
                      for value in make.__code__.co_consts)
    factory = types.FunctionType(make.__code__.replace(co_consts=constants), namespace)
    return factory()


def fresh_record():
    record = Record()
    target = fresh_function(Record.select.__code__.replace(), globals())
    return record, target


def make_caller(target):
    def call(record):
        return target(record)
    return reset_code(call)


def warm(call, record):
    for _ in range(ROUNDS):
        result = call(record)
    return result


class LeafCallTests(unittest.TestCase):
    def require_hits(self, target):
        if not info(target)[5]:
            self.skipTest("leaf elision disabled in this build")
        if not _testinternalcapi.is_specialization_enabled():
            self.skipTest("requires specialization")
        if sys.gettrace() or sys.getprofile():
            self.skipTest("leaf elision declines active tracing/profiling")
        if any(sys.monitoring.get_events(tool) for tool in range(6)):
            self.skipTest("leaf elision declines active global monitoring")

    def test_complete_conditional_both_arms_hit(self):
        record, target = fresh_record()
        self.require_hits(target)
        self.assertTrue(info(target)[0])
        call = make_caller(target)
        self.assertIs(warm(call, record), record.first)
        before = info(target)[1]
        self.assertGreater(before, 0)
        record.direction = 2
        self.assertIs(warm(call, record), record.second)
        self.assertGreater(info(target)[1], before)

    def test_both_immutable_fused_branch_senses_hit(self):
        record, original = fresh_record()
        self.require_hits(original)
        code = original.__code__
        words = _testinternalcapi.get_ir(code)
        compare = next(row for row in words if row[0] == "COMPARE_OP_JUMP")
        unit = compare[3]
        for flip in (0, 0x100):
            with self.subTest(flip=flip):
                initializers = bytearray(code.co_fbinit)
                for offset in range(4, len(initializers), 4):
                    entry = int.from_bytes(initializers[offset:offset + 4], sys.byteorder)
                    if ((entry >> 16) & 4095) == unit:
                        initializers[offset:offset + 4] = (entry ^ flip).to_bytes(4, sys.byteorder)
                        break
                else:
                    self.fail("missing comparison initializer")
                target = fresh_function(code.replace(co_fbinit=bytes(initializers)), globals())
                call = make_caller(target)
                for direction in (1, 2):
                    record.direction = direction
                    expected_first = (direction == 1) ^ bool(flip)
                    before = info(target)[1]
                    self.assertIs(warm(call, record),
                                  record.first if expected_first else record.second)
                    self.assertGreater(info(target)[1], before)

    def test_ordered_inline_floats_hit_and_nan_declines(self):
        class FloatLimit:
            selected = 2.5
        namespace = {"Limit": FloatLimit}
        exec("def select(record):\n"
             "    if record.direction < Limit.selected:\n"
             "        return record.first\n"
             "    return record.second\n", namespace)
        target = namespace["select"]
        self.require_hits(target)
        record = Record()
        call = make_caller(target)
        for direction, first in ((1.5, True), (3.5, False)):
            record.direction = direction
            before = info(target)[1]
            self.assertIs(warm(call, record), record.first if first else record.second)
            self.assertGreater(info(target)[1], before)
        before = info(target)[1:4]
        record.direction = float("nan")
        self.assertIs(call(record), record.second)
        after = info(target)[1:4]
        self.assertEqual(after[0], before[0])
        self.assertGreater(after[1], before[1])

    def test_argument_constant_and_mortal_replaced_constant(self):
        def identity(value):
            return value

        def constant(value):
            return "leaf constant"

        value = object()
        identity = fresh_function(identity.__code__.replace(), globals())
        constant = fresh_function(constant.__code__.replace(), globals())
        for target in (identity, constant):
            self.require_hits(target)
            call = make_caller(target)
            expected = value if target is identity else "leaf constant"
            self.assertIs(warm(call, value), expected)
            self.assertTrue(info(target)[0])
            self.assertGreater(info(target)[1], 0)
        replacement = object()
        constant.__code__ = constant.__code__.replace(co_consts=tuple(
            replacement if item == "leaf constant" else item
            for item in constant.__code__.co_consts))
        self.assertEqual(info(constant)[1], 0)
        self.assertIs(warm(make_caller(constant), value), replacement)

    def test_extra_effect_exception_and_loop_reject(self):
        calls = []

        def effect(value):
            calls.append(value)
            return value

        def unused_read(value):
            value.missing
            return value

        def protected(value):
            try:
                return value.first
            except AttributeError:
                return value

        def loop(value):
            while value:
                value -= 1
            return value

        for target in (effect, unused_read, protected, loop):
            with self.subTest(target=target.__name__):
                self.assertFalse(info(target)[0])
        self.assertEqual(effect(3), 3)
        self.assertEqual(calls, [3])

    def test_common_constant_roots_and_bounds(self):
        def constant(value):
            return None

        code = constant.__code__
        rows = _testinternalcapi.get_ir(code)
        pc = next(i for i, row in enumerate(rows)
                  if row[0] == "LOAD_COMMON_CONSTANT")
        _, destination, _, c, d = rows[pc]

        def replace_index(index):
            word = encode_words((opcode.opmap["LOAD_COMMON_CONSTANT"],
                                 destination, index, c, d))
            return code.replace(co_code=(code.co_code[:8 * pc] + word +
                                         code.co_code[8 * (pc + 1):]))

        argument = object()
        for index, expected in enumerate(opcode._common_constants):
            with self.subTest(index=index):
                target = fresh_function(replace_index(index), globals())
                self.require_hits(target)
                self.assertTrue(info(target)[0])
                self.assertIs(warm(make_caller(target), argument), expected)
                self.assertGreater(info(target)[1], 0)
        # Malformed immutable IR is never executed.
        try:
            invalid = replace_index(len(opcode._common_constants))
        except ValueError:
            pass
        else:
            self.assertFalse(_testinternalcapi.ir_leaf_info(invalid)[0])

    def test_same_code_distinct_globals_and_existing_key_rebind(self):
        record, first = fresh_record()
        self.require_hits(first)
        class OtherLimit:
            selected = 2
        second = fresh_function(first.__code__, {**globals(), "Limit": OtherLimit})
        call = make_caller(first)
        before = info(first)[1]
        self.assertIs(warm(call, record), record.first)
        self.assertGreater(info(first)[1], before)
        before = info(second)[1]
        self.assertIs(warm(make_caller(second), record), record.second)
        self.assertGreater(info(second)[1], before)
        second.__globals__["Limit"] = Limit
        before = info(second)[1]
        self.assertIs(warm(make_caller(second), record), record.first)
        self.assertGreater(info(second)[1], before)

    def test_comparison_callback_and_descriptor_fallback_once(self):
        record, target = fresh_record()
        call = make_caller(target)
        warm(call, record)
        events = []
        class Comparison:
            def __eq__(self, other):
                events.append(other)
                return False
        record.direction = Comparison()
        self.assertIs(call(record), record.second)
        self.assertEqual(events, [1])
        record.direction = 1

        class Missing:
            def __get__(self, receiver, owner):
                events.append("selected")
                raise LookupError("selected descriptor")
        class Changed(Record):
            pass
        changed = Changed()
        changed.direction = 2
        del changed.first
        Changed.first = Missing()
        # The unselected descriptor must never execute, including on fallback.
        self.assertIs(call(changed), changed.second)
        changed.direction = 1
        with self.assertRaisesRegex(LookupError, "selected descriptor"):
            call(changed)
        self.assertEqual(events, [1, "selected"])

    def test_custom_getattribute_invalidates_shape_probe(self):
        class Changed(Record):
            pass
        record = Changed()
        target = fresh_function(Record.select.__code__.replace(), globals())
        call = make_caller(target)
        warm(call, record)
        events = []
        def getattribute(self, name):
            events.append(name)
            return object.__getattribute__(self, name)
        Changed.__getattribute__ = getattribute
        self.assertIs(call(record), object.__getattribute__(record, "first"))
        self.assertEqual(events, ["direction", "first"])

    def test_scalar_substitution_preserves_normal_comparisons(self):
        record, target = fresh_record()
        call = make_caller(target)
        for value, expected in ((True, True), (1 << 100, False),
                                (1.0, True), (float("nan"), False)):
            with self.subTest(value=value):
                record.direction = value
                self.assertIs(warm(call, record),
                              record.first if expected else record.second)

    @requires_subprocess()
    def test_local_monitoring_and_trace_decline(self):
        script_helper.assert_python_ok(
            "-m", "unittest",
            f"{__name__}.LeafCallTests._check_local_monitoring_and_trace_decline")

    def _check_local_monitoring_and_trace_decline(self):
        record, target = fresh_record()
        call = make_caller(target)
        warm(call, record)
        events = []
        tool = 3
        sys.monitoring.use_tool_id(tool, "leaf-test")
        self.addCleanup(sys.monitoring.free_tool_id, tool)
        for event in (sys.monitoring.events.PY_RETURN,
                      sys.monitoring.events.LINE,
                      sys.monitoring.events.BRANCH_LEFT):
            with self.subTest(event=event):
                sys.monitoring.register_callback(tool, event,
                                                lambda *args: events.append(args))
                sys.monitoring.set_local_events(tool, target.__code__, event)
                before = info(target)[1]
                self.assertIs(call(record), record.first)
                self.assertEqual(info(target)[1], before)
                sys.monitoring.set_local_events(tool, target.__code__, 0)
                sys.monitoring.register_callback(tool, event, None)
        self.assertTrue(events)
        before = info(target)[1]
        def trace(frame, event, arg):
            return trace
        old = sys.gettrace()
        sys.settrace(trace)
        try:
            self.assertIs(call(record), record.first)
        finally:
            sys.settrace(old)
        self.assertEqual(info(target)[1], before)

    @requires_subprocess()
    def test_profile_observes_real_callee(self):
        script_helper.assert_python_ok(
            "-m", "unittest",
            f"{__name__}.LeafCallTests._check_profile_observes_real_callee")

    def _check_profile_observes_real_callee(self):
        record, target = fresh_record()
        call = make_caller(target)
        warm(call, record)
        before = info(target)[1]
        events = []
        def profile(frame, event, arg):
            if frame.f_code is target.__code__:
                events.append(event)
        old = sys.getprofile()
        sys.setprofile(profile)
        try:
            self.assertIs(call(record), record.first)
        finally:
            sys.setprofile(old)
        self.assertEqual(events, ["call", "return"])
        self.assertEqual(info(target)[1], before)

    def test_code_provider_arity_and_defaults_mutation(self):
        record, target = fresh_record()
        call = make_caller(target)
        warm(call, record)
        def replacement(record):
            return record.second
        target.__code__ = replacement.__code__
        self.assertIs(call(record), record.second)
        _testinternalcapi.set_vectorcall_nop(target)
        self.assertIsNone(call(record))
        with self.assertRaises(TypeError):
            replacement()
        def default(record=record):
            return record
        self.assertIs(default(), record)
        self.assertIs(default(record=record), record)

    def test_alias_multiplicity_and_discard_nonfinal_guard(self):
        def ignore(first, second):
            return None
        ignore = fresh_function(ignore.__code__.replace(), globals())
        self.require_hits(ignore)
        events = []
        class Owned:
            def __del__(self):
                events.append("retired")
        def consume_pair(items):
            ignore(items.pop(), items.pop())
        def pair():
            value = Owned()
            return [value, value]
        anchor = Owned()
        for _ in range(ROUNDS):
            consume_pair([anchor, anchor])
        self.assertGreater(info(ignore)[1], 0)
        before = info(ignore)[1:4]
        consume_pair(pair())
        after = info(ignore)[1:4]
        self.assertEqual(after[0], before[0])
        self.assertGreater(after[2], before[2])
        self.assertEqual(events, ["retired"])

        def identity(value):
            return value
        identity = fresh_function(identity.__code__.replace(), globals())
        def discard_last(items):
            identity(items.pop())
        for _ in range(ROUNDS):
            discard_last([anchor])
        self.assertGreater(info(identity)[1], 0)
        before = info(identity)[1:4]
        discard_last([Owned()])
        after = info(identity)[1:4]
        self.assertEqual(after[0], before[0])
        self.assertGreater(after[2], before[2])
        self.assertEqual(events, ["retired", "retired"])

    def test_old_local_and_window_alias_guard(self):
        def constant(value):
            return None
        constant = fresh_function(constant.__code__.replace(), globals())
        self.require_hits(constant)
        events = []
        class Owned:
            def __del__(self):
                events.append("retired")
        def replace(items):
            value = items.pop()
            value = constant(value)
            return value
        anchor = Owned()
        for _ in range(ROUNDS):
            self.assertIsNone(replace([anchor]))
        self.assertGreater(info(constant)[1], 0)
        before = info(constant)[1:4]
        self.assertIsNone(replace([Owned()]))
        after = info(constant)[1:4]
        self.assertEqual(after[0], before[0])
        self.assertGreater(after[2], before[2])
        self.assertEqual(events, ["retired"])

    def test_function_argument_alias_and_retained_caller(self):
        def identity(value):
            return value
        identity = fresh_function(identity.__code__.replace(), globals())
        self.require_hits(identity)
        frames = []
        def call():
            frames.append(sys._getframe())
            return identity(identity)
        self.addCleanup(frames.clear)
        for _ in range(ROUNDS):
            self.assertIs(call(), identity)
        self.assertGreater(info(identity)[1], 0)
        self.assertIs(frames[-1].f_locals["identity"], identity)

    def test_sole_function_owner_falls_back(self):
        events, references = [], []
        def make():
            def identity(value):
                return value
            references.append(weakref.ref(identity, lambda ref: events.append("function")))
            return identity
        def call(value):
            return make()(value)
        sample = make()
        self.require_hits(sample)
        code = sample.__code__
        del sample
        events.clear()
        value = object()
        for _ in range(ROUNDS):
            self.assertIs(call(value), value)
        counts = _testinternalcapi.ir_leaf_info(code)
        self.assertEqual(counts[1], 0)
        self.assertGreater(counts[3], 0)
        self.assertEqual(len(events), ROUNDS)

    def test_invalid_feedback_rejected_by_code_construction(self):
        _, target = fresh_record()
        words = _testinternalcapi.get_ir(target.__code__)
        index = next(i for i, row in enumerate(words) if row[0] == "LOAD_ATTR")
        _, a, b, _, d = words[index]
        data = (target.__code__.co_code[:8 * index] +
                encode_words((opcode.opmap["LOAD_ATTR"], a, b, 4095, d)) +
                target.__code__.co_code[8 * (index + 1):])
        with self.assertRaisesRegex(ValueError, "feedback"):
            target.__code__.replace(co_code=data)

    def test_consumed_latest_temporary_cannot_resurrect_older_definition(self):
        _, target = fresh_record()
        code = target.__code__
        rows = _testinternalcapi.get_ir(code)
        compare_index = next(i for i, row in enumerate(rows)
                             if row[0] == "COMPARE_OP_JUMP")
        global_index = next(i for i, row in enumerate(rows)
                            if row[0] == "LOAD_GLOBAL")
        slot = rows[global_index][1]
        class_index = next(i for i in range(global_index + 1, compare_index)
                           if rows[i][0] == "LOAD_ATTR" and rows[i][2] == slot)
        # The global and class-value definitions share this temporary. The
        # comparison kills its newest definition; neither one may be revived.
        self.assertEqual(rows[class_index][1], slot)
        self.assertIn(slot, rows[compare_index][1:3])
        index = next(i for i in range(compare_index + 1, len(rows))
                     if rows[i][0] == "LOAD_ATTR")
        _, a, _, c, d = rows[index]
        data = (code.co_code[:8 * index] +
                encode_words((opcode.opmap["LOAD_ATTR"], a, slot, c, d)) +
                code.co_code[8 * (index + 1):])
        try:
            changed = code.replace(co_code=data)
        except ValueError:
            return
        # Inspect the fresh certificate only. Never execute malformed IR.
        self.assertFalse(_testinternalcapi.ir_leaf_info(changed)[0])


if __name__ == "__main__":
    unittest.main()
