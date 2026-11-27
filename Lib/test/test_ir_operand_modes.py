"""Static ownership modes of executable register-IR handlers."""

from test.support import ir_dis as dis
import gc
import io
from pathlib import Path
import sys
import unittest
import weakref
from test import support, test_tools
from test.support import import_helper
from unittest import mock

_testinternalcapi = import_helper.import_module("_testinternalcapi")
VARIANT = "GetAttr.Shape"
ROUNDS = _testinternalcapi.SPECIALIZATION_THRESHOLD * 3


def handlers(func):
    return [dis.declared_arm(word[0])
            for word in _testinternalcapi.get_ir(func.__code__, True)]


@unittest.skipUnless(support.Py_SHAPES and not support.Py_GIL_DISABLED,
                     "requires shape specialization")
class OperandModeTests(unittest.TestCase):
    def test_local_receiver_temporary_result(self):
        class Owner:
            pass

        def read(owner):
            return owner.value

        owner = Owner()
        owner.value = object()
        for _ in range(ROUNDS):
            self.assertIs(read(owner), owner.value)
        self.assertIn(VARIANT, handlers(read))
        # The words of the code object stay generic
        self.assertNotIn(VARIANT, [word[0] for word in
                                  _testinternalcapi.get_ir(read.__code__)])

    def test_other_ownership_modes(self):
        class Owner:
            pass

        owner = Owner()
        owner.value = object()

        def read_temporary(factory):
            return factory().value

        def read_to_local(owner):
            result = owner.value
            return result

        for _ in range(ROUNDS):
            self.assertIs(read_temporary(lambda: owner), owner.value)
            self.assertIs(read_to_local(owner), owner.value)
        for read in (read_temporary, read_to_local):
            self.assertIn(VARIANT, handlers(read))

    def test_object_alias_and_result_lifetime(self):
        class Owner:
            pass

        class Payload:
            pass

        def read(owner):
            return owner.value

        owner = Owner()
        owner.value = owner
        for _ in range(ROUNDS):
            self.assertIs(read(owner), owner)
        self.assertIn(VARIANT, handlers(read))
        owner.value = Payload()
        result = read(owner)
        reference = weakref.ref(result)
        del owner.value
        gc.collect()
        self.assertIs(reference(), result)
        del result
        gc.collect()
        self.assertIsNone(reference())

    def test_guard_failure_and_descriptor(self):
        class Owner:
            pass

        def read(owner):
            return owner.value

        owner = Owner()
        owner.value = object()
        for _ in range(ROUNDS):
            read(owner)
        self.assertIn(VARIANT, handlers(read))
        del owner.value
        with self.assertRaises(AttributeError):
            read(owner)
        owner.value = object()
        self.assertIs(read(owner), owner.value)
        calls = []

        def getter(instance):
            calls.append(instance)
            raise RuntimeError("descriptor")

        Owner.value = property(getter)
        with self.assertRaisesRegex(RuntimeError, "descriptor"):
            read(owner)
        self.assertEqual(calls, [owner])

    def test_shape_history_uses_canonical_handlers(self):
        class Owner:
            pass

        def read(owner):
            return owner.value

        owner = Owner()
        owner.value = object()
        other = Owner()
        other.value = object()
        other.extra = 1
        # Introducing a new shared key invalidates the type version. Establish
        # both shapes first so the feedback policy compares one stable type.
        self.assertNotEqual(_testinternalcapi.get_object_shape(owner),
                            _testinternalcapi.get_object_shape(other))
        for _ in range(ROUNDS):
            read(owner)
        self.assertIn(VARIANT, handlers(read))
        for _ in range(_testinternalcapi.SPECIALIZATION_COOLDOWN * 3):
            self.assertIs(read(other), other.value)
        offset = next(instr.offset for instr in dis.get_instructions(read)
                      if instr.family in ("GetAttr", "GetMethod"))
        kind, state, _, _, rows = _testinternalcapi.get_feedback(read.__code__, offset)
        self.assertEqual(kind, "ATTR")
        self.assertTrue(rows)
        self.assertIn(_testinternalcapi.arm_handlers()["GetAttr.Shape"],
                      [row[2] for row in rows])

    def test_tracing_after_warmup(self):
        class Owner:
            pass

        def read(owner):
            return owner.value

        owner = Owner()
        owner.value = object()
        for _ in range(ROUNDS):
            read(owner)
        events = []

        def trace(frame, event, arg):
            if frame.f_code is read.__code__:
                frame.f_trace_opcodes = True
                events.append(event)
            return trace

        previous = sys.gettrace()
        try:
            sys.settrace(trace)
            self.assertIs(read(owner), owner.value)
        finally:
            sys.settrace(previous)
        self.assertIn("line", events)
        self.assertIn("opcode", events)
        for _ in range(ROUNDS):
            self.assertIs(read(owner), owner.value)
        self.assertIn(VARIANT, handlers(read))

    def test_monitoring_error_after_warmup(self):
        class Owner:
            pass

        def read(owner):
            return owner.value

        owner = Owner()
        owner.value = object()
        for _ in range(ROUNDS):
            read(owner)
        self.assertIn(VARIANT, handlers(read))
        mon = sys.monitoring
        tool = mon.DEBUGGER_ID

        def callback(code, offset):
            if code is read.__code__:
                raise RuntimeError("instruction observer")

        mon.use_tool_id(tool, "IR operand modes")
        try:
            mon.register_callback(tool, mon.events.INSTRUCTION, callback)
            mon.set_local_events(tool, read.__code__, mon.events.INSTRUCTION)
            with self.assertRaisesRegex(RuntimeError, "instruction observer"):
                read(owner)
        finally:
            mon.set_local_events(tool, read.__code__, 0)
            mon.free_tool_id(tool)
        for _ in range(ROUNDS):
            self.assertIs(read(owner), owner.value)
        self.assertIn(VARIANT, handlers(read))



# Slot-class members (`over(locals: ...)`): the operands of a family's
# implicit locals operand, from the operand tables: its Src and Dst homes in
# the head word, in operand order, with each field's flag bit
_HEAD_FIELDS = {16: 9, 28: 10, 40: 11, 52: 12}


def locals_operands(family):
    return [(name, _HEAD_FIELDS[offset])
            for name, kind, role, _, _, offset in _testinternalcapi.arm_operands()[family]
            if kind == "home" and role in ("dst", "src") and offset in _HEAD_FIELDS]


def letters(family, word):
    return "".join("L" if word >> bit & 1 else "T" for _, bit in locals_operands(family))


@support.requires_specialization
class LocalsMemberTests(unittest.TestCase):
    """Every executing word runs the slot-class member that its flags select,
    or its declared arm when no member has its flags, whether the
    executing-copy builder chose the handler or adaptation installed it
    (set_opcode())"""

    def check_words(self, func):
        names = [word[0].partition("+")[0]
                 for word in _testinternalcapi.get_ir(func.__code__, True)]
        raw = _testinternalcapi.get_ir_raw(func.__code__)[-2]
        words = [int.from_bytes(raw[i:i + 8], "little") for i in range(0, len(raw), 8)]
        members = {}
        for member, declared in dis._declared_arms.items():
            members.setdefault(declared, set()).add(member)
        checked = 0
        for name, word in zip(names, words):
            declared = dis.declared_arm(name)
            if declared not in members:
                continue
            family = declared.partition(".")[0]
            mine = f"{declared}_{letters(family, word)}"
            want = mine if mine in members[declared] else declared
            self.assertEqual(name, want, f"word {word:#x} of {func.__name__}")
            checked += 1
        return checked

    def test_members_follow_the_word_flags(self):
        class Owner:
            pass

        def local_receiver(owner):
            return owner.value

        def temporary_receiver(owner):
            return owner.inner.value

        def stored_result(owner):
            result = owner.value
            return result

        def chained(a, b, c):
            return a + b + c

        def into_local(a, b):
            c = a + b
            return c

        def subscripts(items, i):
            items[i] = items[i] + items[0]
            return items[i]

        owner = Owner()
        owner.value = 1
        owner.inner = Owner()
        owner.inner.value = 2
        functions = [(local_receiver, (owner,)), (temporary_receiver, (owner,)),
                     (stored_result, (owner,)), (chained, (1, 2, 3)),
                     (into_local, (1, 2)), (subscripts, ([1, 2, 3], 1))]
        checked = 0
        for func, args in functions:
            for _ in range(ROUNDS):
                func(*args)
            checked += self.check_words(func)
        self.assertTrue(checked)


if __name__ == "__main__":
    unittest.main()
