"""Static ownership modes of executable register-IR handlers."""

import dis
import gc
import io
import opcode
from pathlib import Path
import sys
import unittest
import weakref
from test import support, test_tools
from test.support import import_helper
from unittest import mock

_testinternalcapi = import_helper.import_module("_testinternalcapi")
VARIANT = "LOAD_ATTR_SHAPE_LOCAL_TEMP"
ROUNDS = _testinternalcapi.SPECIALIZATION_THRESHOLD * 3


def handlers(func):
    return [word[0] for word in _testinternalcapi.get_ir(func.__code__, True)]


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
        self.assertIn("attr = shape_read_dup(PyValue_AsKnownObjectBorrow(owner), offset);", body)
        self.assertIn("ir_own_B = PyValue_NULL;", body)
        publication = "IR_SLOT(IR_A(ir_word)) = attr;"
        self.assertLess(body.index("attr = shape_read_dup("), body.index(publication))
        self.assertLess(body.index(publication), body.index("retire_published_values("))
        helpers = (Path(test_tools.basepath) / "Python" / "ceval_macros.h").read_text()
        read = helpers.split("shape_read_dup(PyObject *owner, uint32_t offset)", 1)[1]
        read = read.split("\n}", 1)[0]
        self.assertIn("PyValue_Borrow(*(PyValue *)((char *)owner + offset))", read)
        self.assertIn("assert(!PyValue_IsNull(found));", read)
        self.assertIn("return IR_CANONICAL_HOME_DUP(found);", read)
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


if __name__ == "__main__":
    unittest.main()
