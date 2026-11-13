"""Static ownership modes of executable register-IR handlers."""

import dis
import gc
import io
import marshal
import opcode
from pathlib import Path
import sys
import types
import unittest
import weakref
from test import support, test_tools
from test.support import import_helper
from test.support.bytecode_helper import encode_words
from unittest import mock

_testinternalcapi = import_helper.import_module("_testinternalcapi")
VARIANT = "LOAD_ATTR_SHAPE_LOCAL_TEMP"
ROUNDS = _testinternalcapi.SPECIALIZATION_THRESHOLD * 3
INTEGER_VARIANT = "BINARY_OP_ADD_INT_OVERWRITE_TEMP"


def handlers(func):
    return [word[0] for word in _testinternalcapi.get_ir(func.__code__, True)]


def overwrite_template():
    def add(x, y):
        return (x + y) + y

    code = add.__code__.replace()
    nlocalsplus = len(_testinternalcapi.get_co_localskinds(code))
    candidates = [(index, word[1:]) for index, word in enumerate(
        _testinternalcapi.get_ir(code))
        if word[0].startswith("BINARY_OP")
        and word[1] == word[2] >= nlocalsplus and word[2] != word[3]]
    assert len(candidates) == 1, candidates
    return code, *candidates[0]


def overwrite_bytes(code, index, fields):
    word = encode_words((opcode._ir_opmap[INTEGER_VARIANT], *fields))
    return code.co_code[:index * 8] + word + code.co_code[(index + 1) * 8:]


def replace_overwrite(code, index, fields, **changes):
    # A different source-map object permits the structural change to reach
    # validation; the same instruction family/extent retains its source edges.
    return code.replace(co_code=overwrite_bytes(code, index, fields),
                        co_linetable=memoryview(code.co_linetable).tobytes(),
                        **changes)


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
        self.assertNotIn(VARIANT, [word[0] for word in
                                  _testinternalcapi.get_ir(read.__code__)])
        # Both public disassembly paths must accept a physical handler > 255.
        adaptive = {instr.opname for instr in dis.get_instructions(read, adaptive=True)}
        self.assertIn("LOAD_ATTR_SHAPE", adaptive)
        self.assertNotIn("LOAD_ATTR", adaptive)
        dis.dis(read, adaptive=True, file=io.StringIO())

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
            self.assertNotIn(VARIANT, handlers(read))
            self.assertIn("LOAD_ATTR_SHAPE", handlers(read))

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
                      if instr.opname == "LOAD_ATTR")
        kind, state, _, _, rows = _testinternalcapi.get_feedback(read.__code__, offset)
        self.assertEqual(kind, "ATTR")
        self.assertTrue(rows)
        self.assertNotIn(opcode._ir_opmap[VARIANT], [row[2] for row in rows])
        self.assertIn(opcode._specialized_opmap["LOAD_ATTR_SHAPE"],
                      [row[2] for row in rows])
        self.assertNotIn(VARIANT, handlers(read))

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

    def test_metadata_domain(self):
        import _opcode
        h = opcode._ir_opmap[VARIANT]
        row = _opcode.get_ir_contract()[h]
        base = _opcode.get_ir_contract()[opcode._specialized_opmap["LOAD_ATTR_SHAPE"]]
        self.assertGreater(h, 255)
        self.assertEqual(row, base)


@unittest.skipIf(test_tools.basepath is None, "needs the source tree")
class OperandModeGeneratorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        test_tools.skip_if_missing("cases_generator")
        with test_tools.imports_under_tool("cases_generator"):
            import analyzer
            import ir_generator
            import ir_metadata_generator
        cls.generator = ir_generator
        cls.metadata = ir_metadata_generator
        cls.analysis = analyzer.analyze_files([
            str(Path(test_tools.basepath) / "Python" / "bytecodes.c")])

    def test_inherited_contract_and_generated_modes(self):
        gen = self.generator
        formats = gen.compute_formats(self.analysis)
        contracts = gen.handler_contracts(self.analysis, formats)
        self.assertIs(formats[VARIANT], formats["LOAD_ATTR_SHAPE"])
        self.assertEqual(contracts[VARIANT], contracts["LOAD_ATTR_SHAPE"])
        self.assertEqual(self.metadata.shadow_of(VARIANT, self.analysis),
                         "LOAD_ATTR_SHAPE")
        _, bases, flags = self.metadata.handler_facts(self.analysis, formats)
        self.assertEqual(bases[VARIANT], "LOAD_ATTR")
        self.assertIn("_PyIR_H_SPECIALIZED", flags[VARIANT])
        self.assertIn("_PyIR_H_IR_ONLY", flags[VARIANT])
        self.assertNotIn("_PyIR_H_PRISTINE", flags[VARIANT])
        output = io.StringIO()
        gen.generate_ir([], self.analysis, output, False)
        body = output.getvalue().split(f"TARGET({VARIANT}) {{", 1)[1]
        body = body.split("TARGET(", 1)[0]
        self.assertNotIn("IR_IS_LOCAL", body)
        self.assertNotIn("IR_CONSUME_INPUT", body)
        self.assertIn("IR_CANONICAL_HOME_DUP(found)", body)
        self.assertIn("int opcode = LOAD_ATTR_SHAPE;", body)

    def test_synthetic_instruction_without_specialization_family(self):
        inst = self.analysis.instructions["LOAD_ATTR_SHAPE"]
        with mock.patch.object(inst, "family", None):
            self.assertNotIn(VARIANT, self.generator.execution_variants(self.analysis))
            self.assertNotIn(VARIANT, self.generator.handler_ids(self.analysis))

    def test_invalid_mode_descriptors(self):
        gen = self.generator
        for local, temp in ((("A",), ()), ((), ("B",)), (("D",), ()),
                            (("B",), ("B",))):
            with self.subTest(local=local, temp=temp):
                variant = gen.ExecutionVariant("LOAD_ATTR_SHAPE", local, temp)
                with mock.patch.dict(gen.EXECUTION_VARIANTS, {VARIANT: variant}):
                    with self.assertRaisesRegex(ValueError, "invalid operand modes"):
                        gen.compute_formats(self.analysis)


@unittest.skipIf(support.Py_GIL_DISABLED, "requires integer specialization")
class SameHomeIntegerTests(unittest.TestCase):
    def test_two_scratch_inputs_retire_before_result_observation(self):
        def add(x, y):
            return (x + 1) + (y + 2)

        for _ in range(ROUNDS):
            self.assertEqual(add(10, 20), 33)
        self.assertIn("BINARY_OP_ADD_INT_OVERWRITE_TEMP", handlers(add))
        self.assertEqual(add((1 << 50) - 2, 2), (1 << 50) + 3)
        self.assertEqual(add(1.5, 2.5), 7.0)

    def test_inline_overflow_guard_miss_and_source_observation(self):
        def add(x, y):
            return (x + 1) + y

        for _ in range(ROUNDS):
            self.assertEqual(add(10, 20), 31)
        self.assertIn("BINARY_OP_ADD_INT_OVERWRITE_TEMP", handlers(add))
        self.assertNotIn("BINARY_OP_ADD_INT_OVERWRITE_TEMP",
                         [word[0] for word in _testinternalcapi.get_ir(add.__code__)])
        # Both inputs of the second add fit int51; its result needs boxing.
        self.assertEqual(add((1 << 50) - 2, 3), (1 << 50) + 2)
        self.assertEqual(add(2.5, 3.5), 7.0)
        for _ in range(ROUNDS):
            self.assertEqual(add(10, 20), 31)
        events = []

        def tracer(frame, event, arg):
            if frame.f_code is add.__code__:
                frame.f_trace_opcodes = True
                if event == "opcode":
                    events.append(frame.f_lasti)
            return tracer

        previous = sys.gettrace()
        try:
            sys.settrace(tracer)
            self.assertEqual(add(10, 20), 31)
        finally:
            sys.settrace(previous)
        additions = [i.offset for i in dis.get_instructions(add)
                     if i.opname == "BINARY_OP"]
        self.assertEqual(len(additions), 2)
        self.assertTrue(all(offset in events for offset in additions))

    def test_local_and_discard_destinations_keep_generic_owner_paths(self):
        def local(x, y):
            result = (x + 1) + y
            return result

        def discarded(x, y):
            (x + 1) + y

        for _ in range(ROUNDS):
            self.assertEqual(local(10, 20), 31)
            self.assertIsNone(discarded(10, 20))
        self.assertNotIn("BINARY_OP_ADD_INT_OVERWRITE_TEMP", handlers(local))
        self.assertNotIn("BINARY_OP_ADD_INT_OVERWRITE_TEMP", handlers(discarded))


class ConstructedIntegerOverwriteTests(unittest.TestCase):
    def test_constructor_rejects_each_invalid_geometry(self):
        code, index, fields = overwrite_template()
        a, b, c, d = fields
        nlocalsplus = len(_testinternalcapi.get_co_localskinds(code))
        nslots = nlocalsplus + code.co_stacksize
        invalid = {
            "different destination": (0, b, c, d),
            "same inputs": (a, b, b, d),
            "local input": (0, 0, c, d),
            "discard": (4095, 4095, c, d),
            "B outside frame": (nslots, nslots, c, d),
            "C outside frame": (a, b, nslots, d),
            "C read sentinel": (a, b, 4095, d),
        }
        for name, changed in invalid.items():
            with self.subTest(geometry=name):
                with self.assertRaisesRegex(ValueError, "inline overwrite geometry"):
                    replace_overwrite(code, index, changed)

    def test_changed_frame_metadata_rechecks_geometry(self):
        code, index, fields = overwrite_template()
        valid = replace_overwrite(code, index, fields)
        for changes in ({"co_stacksize": 0},
                        {"co_nlocals": valid.co_nlocals + 1,
                         "co_varnames": (*valid.co_varnames, "extra")}):
            with self.subTest(changes=changes):
                with self.assertRaisesRegex(ValueError, "inline overwrite geometry"):
                    replace_overwrite(valid, index, fields, **changes)

    def test_valid_constructed_variant_and_monitoring_restore(self):
        code, index, fields = overwrite_template()
        valid = replace_overwrite(code, index, fields)
        add = types.FunctionType(valid, globals())
        self.assertEqual(add(10, 20), 50)
        self.assertEqual(add((1 << 50) - 4, 3), (1 << 50) + 2)
        self.assertEqual(add(2.5, 3.5), 9.5)
        events = []
        mon = sys.monitoring
        tool = mon.PROFILER_ID
        mon.use_tool_id(tool, "constructed overwrite")
        try:
            mon.register_callback(tool, mon.events.INSTRUCTION,
                                  lambda code, offset: events.append(offset))
            mon.set_local_events(tool, valid, mon.events.INSTRUCTION)
            self.assertEqual(add(10, 20), 50)
        finally:
            mon.set_local_events(tool, valid, 0)
            mon.free_tool_id(tool)
        self.assertIn(index, events)
        self.assertEqual(add(10, 20), 50)
        self.assertIn(INTEGER_VARIANT, handlers(add))

    def test_marshal_valid_and_invalid_refinement(self):
        code, index, fields = overwrite_template()
        valid = replace_overwrite(code, index, fields)
        payload = marshal.dumps(valid)
        loaded = marshal.loads(payload)
        self.assertEqual(types.FunctionType(loaded, globals())(10, 20), 50)
        self.assertEqual(loaded.co_code, valid.co_code)
        self.assertEqual(payload.count(valid.co_code), 1)
        a, b, c, d = fields
        for changed in ((0, b, c, d), (a, b, b, d),
                        (a, b, 4095, d), (4095, 4095, c, d)):
            with self.subTest(fields=changed):
                malformed = payload.replace(
                    valid.co_code, overwrite_bytes(valid, index, changed), 1)
                with self.assertRaisesRegex(ValueError, "inline overwrite geometry"):
                    marshal.loads(malformed)


if __name__ == "__main__":
    unittest.main()
