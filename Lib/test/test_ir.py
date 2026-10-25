"""Tests for the register IR of --with-ir-interp (Include/internal/pycore_ir.h)."""

import dis
import sys
import unittest
from test.support import import_helper

_testinternalcapi = import_helper.import_module("_testinternalcapi")
get_ir = getattr(_testinternalcapi, "get_ir", None)

IR_BORROW = 1


def slot(field):
    return field >> 1


def handlers(func):
    return [instr[2] for instr in get_ir(func.__code__)]


def make_function(source, name="f"):
    namespace = {}
    exec(source, namespace)
    return namespace[name]


class C:
    attr = 5


@unittest.skipIf(get_ir is None, "build without the register IR")
class LoweringTest(unittest.TestCase):

    def check_offsets(self, func):
        # One IR instruction per bytecode instruction but EXTENDED_ARG,
        # anchored at the bytecode offset of the instruction.
        anchors = [instr[1] for instr in get_ir(func.__code__)]
        offsets = [instr.offset for instr in dis.get_instructions(func)
                   if instr.opname != "EXTENDED_ARG"]
        self.assertEqual(anchors, offsets)

    def test_offsets(self):
        def f(a, b):
            for x in a:
                if x > b:
                    return x * 2
            return None
        self.check_offsets(f)

    def test_absorbed_load(self):
        def f(o):
            return o.attr
        ir = {instr[2]: instr for instr in get_ir(f.__code__)}
        self.assertIn("IR_ABSORBED_LOAD_FAST_BORROW", ir)
        # LOAD_ATTR reads its owner through field B, from local 0
        b = ir["LOAD_ATTR"][4]
        self.assertTrue(b & IR_BORROW)
        self.assertEqual(slot(b), 0)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 1):
            self.assertEqual(f(C()), 5)
        self.assertIn("IR_ABSORBED_LOAD_FAST_BORROW", handlers(f))

    def test_no_absorption_into_a_join(self):
        # LOAD_ATTR starts a basic block, so its owner comes from the stack
        # on both paths.  The compiler borrows no load whose value crosses a
        # block boundary; patch one in to check that the lowering holds back.
        def f(a, b):
            return (a or b).attr
        code = bytearray(f.__code__.co_code)
        load_b = [instr for instr in dis.get_instructions(f)
                  if instr.opname == "LOAD_FAST" and instr.argval == "b"]
        code[load_b[0].offset] = dis.opmap["LOAD_FAST_BORROW"]
        f.__code__ = f.__code__.replace(co_code=bytes(code))
        self.assertIn("LOAD_FAST_BORROW", handlers(f))
        self.assertNotIn("IR_ABSORBED_LOAD_FAST_BORROW", handlers(f))
        self.assertEqual(f(C(), None), 5)
        self.assertEqual(f(None, C()), 5)

    def test_no_absorption_across_a_line(self):
        def f(o):
            return (o
                    .attr)
        self.assertNotIn("IR_ABSORBED_LOAD_FAST_BORROW", handlers(f))
        self.assertEqual(f(C()), 5)

    def test_tracing_keeps_absorbed_loads(self):
        def f(o):
            return o.attr
        f(C())
        opcodes = []

        def tracer(frame, event, arg):
            frame.f_trace_opcodes = True
            if event == "opcode" and frame.f_code is f.__code__:
                opcodes.append(frame.f_lasti)
            return tracer

        sys.settrace(tracer)
        try:
            result = f(C())
        finally:
            sys.settrace(None)
        self.assertEqual(result, 5)
        # One event per instruction from the first RESUME on
        lasti = [instr.offset for instr in dis.get_instructions(f)][1:]
        self.assertEqual(opcodes, lasti)
        # The next RESUME removes the instrumentation
        self.assertEqual(f(C()), 5)
        self.assertIn("IR_ABSORBED_LOAD_FAST_BORROW", handlers(f))

    def test_f_lasti(self):
        def f():
            return sys._getframe().f_lasti
        load_attr = [instr for instr in dis.get_instructions(f)
                     if instr.argval == "f_lasti"]
        self.assertEqual(f(), load_attr[0].offset)

    def test_line_starting_at_extended_arg(self):
        # From the 257th constant on, each line starts with an EXTENDED_ARG,
        # which has no word of its own.
        n = 300
        source = "def f():\n" + "".join(f"    a = {i}.5\n" for i in range(n))
        f = make_function(source)
        lines = []

        def tracer(frame, event, arg):
            if event == "line" and frame.f_code is f.__code__:
                lines.append(frame.f_lineno)
            return tracer

        sys.settrace(tracer)
        try:
            f()
        finally:
            sys.settrace(None)
        self.assertEqual(lines, list(range(2, n + 2)))
        self.check_offsets(f)

    def test_wide_fields(self):
        # Slots beyond 32767 and constants beyond 65535 need IR_EXT, and
        # more than 65535 code units a pcmap with 32-bit offsets.
        nlocals = 40_000
        nconsts = 70_000
        last = f"v{nlocals - 1}"
        source = ("def f(o):\n"
                  + "".join(f"    v{i % nlocals} = {i}.5\n"
                            for i in range(nconsts))
                  + f"    {last} = o\n"
                  + f"    return v0, {last}.attr\n")
        f = make_function(source)
        self.assertGreater(len(f.__code__.co_code) // 2, 65535)
        ir = get_ir(f.__code__)
        # get_ir folds IR_EXT words into the fields they widen
        self.assertGreater(max(max(instr[3:6]) for instr in ir), 0xFFFF)
        self.assertGreater(ir[-1][1], 2 * 65535)
        # LOAD_ATTR reads the last local through a wide field
        load_attr = [instr for instr in ir if instr[2] == "LOAD_ATTR"]
        self.assertEqual(slot(load_attr[0][4]),
                         f.__code__.co_varnames.index(last))
        self.check_offsets(f)

        # The handlers that run the wide LOAD_ATTR: specialised, deoptimised
        # and instrumented
        class D:
            attr = 6
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 1):
            self.assertEqual(f(C()), (40_000.5, 5))
        self.assertNotIn("LOAD_ATTR", handlers(f))
        self.assertEqual(f(D()), (40_000.5, 6))
        lines = 0

        def tracer(frame, event, arg):
            nonlocal lines
            lines += event == "line"
            return tracer

        sys.settrace(tracer)
        try:
            result = f(C())
        finally:
            sys.settrace(None)
        self.assertEqual(result, (40_000.5, 5))
        self.assertEqual(lines, nconsts + 2)

    def test_sizeof(self):
        code = compile("x = [i for i in range(3)]", "<test>", "exec")
        before = sys.getsizeof(code)
        get_ir(code)
        self.assertGreater(sys.getsizeof(code), before)

    def test_stack_underflow(self):
        def f():
            pass
        code = f.__code__.replace(co_code=bytes([
            dis.opmap["RESUME"], 0, 0, 0,
            dis.opmap["POP_TOP"], 0,
            dis.opmap["RETURN_VALUE"], 0]))
        with self.assertRaisesRegex(SystemError, "stack underflow"):
            get_ir(code)


if __name__ == "__main__":
    unittest.main()
