"""Tests for the register IR of --with-ir-interp (Include/internal/pycore_ir.h)."""

import dis
import opcode
import sys
import unittest
from test import support
from test.support import import_helper

_testinternalcapi = import_helper.import_module("_testinternalcapi")
get_ir = getattr(_testinternalcapi, "get_ir", None)

IR_BORROW = 1
# The free-threaded build does not specialise the IR.
SPECIALIZES = not support.Py_GIL_DISABLED
BORROWED_LOADS = ("LOAD_FAST_BORROW", "LOAD_FAST_BORROW_LOAD_FAST_BORROW")


def slot(field):
    return field >> 1


def handlers(func):
    return [instr[2] for instr in get_ir(func.__code__)]


def make_function(source, name="f"):
    namespace = {}
    exec(source, namespace)
    return namespace[name]


def assemble(*instrs):
    """co_code for (opname, oparg) pairs, with their inline caches"""
    code = bytearray()
    for name, arg in instrs:
        code += bytes([dis.opmap[name], arg])
        code += bytes(2 * opcode._inline_cache_entries.get(name, 0))
    return bytes(code)


def one_word_per_instruction(func):
    anchors = [instr[1] for instr in get_ir(func.__code__)]
    offsets = [instr.offset for instr in dis.get_instructions(func)
               if instr.opname != "EXTENDED_ARG"]
    return anchors == offsets


def trace(func, *args, opcodes=False):
    """Run func under sys.settrace(): its result and its (event, f_lasti,
    f_lineno) for the events of func's own frame."""
    events = []

    def tracer(frame, event, arg):
        frame.f_trace_opcodes = opcodes
        if frame.f_code is func.__code__:
            events.append((event, frame.f_lasti, frame.f_lineno))
        return tracer

    sys.settrace(tracer)
    try:
        result = func(*args)
    finally:
        sys.settrace(None)
    return result, events


class C:
    attr = 5


@unittest.skipIf(get_ir is None, "build without the register IR")
class LoweringTest(unittest.TestCase):

    def check_offsets(self, func):
        # One IR instruction per bytecode instruction but EXTENDED_ARG and
        # the loads that their consumer absorbs, anchored at the bytecode
        # offset of the instruction.  Its offsets start at the first of those
        # before it.
        instrs = list(dis.get_instructions(func))
        i = 0
        for start, anchor, *_ in get_ir(func.__code__):
            self.assertEqual(start, instrs[i].offset)
            while instrs[i].offset != anchor:
                self.assertIn(instrs[i].opname,
                              ("EXTENDED_ARG",) + BORROWED_LOADS)
                i += 1
            i += 1
        self.assertEqual(i, len(instrs))

    def test_offsets(self):
        def f(a, b):
            for x in a:
                if x > b:
                    return x * 2
            return None
        self.check_offsets(f)

    def load_attr_owner(self, func):
        # Field B of the LOAD_ATTR of func
        ir = get_ir(func.__code__)
        return [instr for instr in ir if instr[2] == "LOAD_ATTR"][0][4]

    def test_absorbed_load(self):
        def f(o):
            return o.attr
        load = [instr for instr in dis.get_instructions(f)
                if instr.opname == "LOAD_FAST_BORROW"][0]
        ir = {instr[2]: instr for instr in get_ir(f.__code__)}
        # The load has no word.  LOAD_ATTR covers its offset and reads the
        # owner through field B, from local 0.
        self.assertNotIn("LOAD_FAST_BORROW", ir)
        start, anchor, _, _, b, *_ = ir["LOAD_ATTR"]
        self.assertEqual(start, load.offset)
        self.assertGreater(anchor, start)
        self.assertTrue(b & IR_BORROW)
        self.assertEqual(slot(b), 0)
        self.check_offsets(f)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 1):
            self.assertEqual(f(C()), 5)
        if SPECIALIZES:
            self.assertNotIn("LOAD_ATTR", handlers(f))
        self.assertNotIn("LOAD_FAST_BORROW", handlers(f))

    def test_absorbed_pair(self):
        def f(a, b):
            return a in b
        self.assertNotIn("LOAD_FAST_BORROW_LOAD_FAST_BORROW", handlers(f))
        ir = {instr[2]: instr for instr in get_ir(f.__code__)}
        # Field A is the destination
        _, _, _, _, b, c, _ = ir["CONTAINS_OP"]
        self.assertEqual((b, c), (0 << 1 | IR_BORROW, 1 << 1 | IR_BORROW))
        self.check_offsets(f)
        self.assertTrue(f(1, [1]))
        self.assertFalse(f(2, {1}))

    def test_exception_from_a_pushed_frame(self):
        # LOAD_ATTR_PROPERTY pushes the frame of the getter.  When the getter
        # raises, the unwinding of f looks up the code unit before LOAD_ATTR,
        # the load that starts the try block, which LOAD_ATTR absorbs.
        class P:
            @property
            def attr(self):
                raise AttributeError

        def f(o):
            try:
                return o.attr
            except AttributeError:
                return "caught"
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 2):
            self.assertEqual(f(P()), "caught")
        if SPECIALIZES:
            self.assertIn("LOAD_ATTR_PROPERTY", handlers(f))
        self.assertNotIn("LOAD_FAST_BORROW", handlers(f))

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
        self.assertFalse(self.load_attr_owner(f) & IR_BORROW)
        self.assertEqual(f(C(), None), 5)
        self.assertEqual(f(None, C()), 5)

    def test_no_absorption_across_a_line(self):
        def f(o):
            return (o
                    .attr)
        self.assertIn("LOAD_FAST_BORROW", handlers(f))
        self.assertFalse(self.load_attr_owner(f) & IR_BORROW)
        self.assertEqual(f(C()), 5)

    def test_no_absorption_under_tracing(self):
        def f(o):
            return o.attr
        result, events = trace(f, C(), opcodes=True)
        self.assertEqual(result, 5)
        # A word per instruction, and one event per instruction from the
        # first RESUME on
        offsets = [instr.offset for instr in dis.get_instructions(f)]
        self.assertEqual([instr[1] for instr in get_ir(f.__code__)], offsets)
        lasti = offsets[1:]
        self.assertEqual([e[1] for e in events if e[0] == "opcode"], lasti)

    def test_line_starting_at_absorbed_load(self):
        def f(o):
            x = 1
            return o.attr
        self.assertEqual(f(C()), 5)
        self.assertNotIn("LOAD_FAST_BORROW", handlers(f))
        result, events = trace(f, C())
        self.assertEqual(result, 5)
        line = f.__code__.co_firstlineno
        self.assertEqual([e[2] for e in events if e[0] == "line"],
                         [line + 1, line + 2])
        # The next RESUME removes the instrumentation
        self.assertEqual(f(C()), 5)
        self.assertNotIn("LOAD_FAST_BORROW", handlers(f))

    def test_opcode_events_of_absorbed_loads(self):
        # Enabling INSTRUCTION events lowers fused code again, one word per
        # instruction.
        def f(o):
            return o.attr
        self.assertEqual(f(C()), 5)
        self.assertNotIn("LOAD_FAST_BORROW", handlers(f))
        result, events = trace(f, C(), opcodes=True)
        self.assertEqual(result, 5)
        lasti = [instr.offset for instr in dis.get_instructions(f)][1:]
        self.assertEqual([e[1] for e in events if e[0] == "opcode"], lasti)
        self.assertTrue(one_word_per_instruction(f))
        # The new block stays once the events are off
        self.assertEqual(f(C()), 5)
        self.assertTrue(one_word_per_instruction(f))

    def test_frames_in_the_fused_block(self):
        # Frames that are in the code when it is lowered again go on in the
        # new block: a suspended generator, and a frame in a call that turns
        # on opcode events for it and raises.
        def gen(o):
            yield o.attr
            yield o.attr

        def enable_and_raise():
            frame = sys._getframe(1)
            frame.f_trace = lambda *args: None
            frame.f_trace_opcodes = True
            raise ValueError

        def f(o):
            x = o.attr
            try:
                enable_and_raise()
            except ValueError:
                pass
            return x + o.attr

        g = gen(C())
        self.assertEqual(next(g), 5)
        self.assertFalse(one_word_per_instruction(gen))
        self.assertFalse(one_word_per_instruction(f))
        sys.settrace(lambda *args: None)
        try:
            self.assertEqual(f(C()), 10)
            g.gi_frame.f_trace_opcodes = True
            g.gi_frame.f_trace = lambda *args: None
            self.assertEqual(next(g), 5)
        finally:
            sys.settrace(None)
        self.assertTrue(one_word_per_instruction(f))
        self.assertTrue(one_word_per_instruction(gen))
        with self.assertRaises(StopIteration):
            next(g)

    @unittest.skipUnless(SPECIALIZES, "the IR is not specialised")
    def test_compare_and_branch(self):
        # A specialised compare whose result a POP_JUMP_IF_FALSE or
        # POP_JUMP_IF_TRUE consumes runs the jump too; the jump keeps its
        # word.
        source = ("def f(xs, y):\n"
                  "    n = 0\n"
                  "    for x in xs:\n"
                  "        if x == y or y == x:\n"
                  "            n += 1\n"
                  "        if x != y:\n"
                  "            n += 10\n"
                  "    return n\n")
        cases = (([1, 2, 3], 2, "IR_SUPER_COMPARE_OP_INT"),
                 ([1.5, 2.5], 2.5, "IR_SUPER_COMPARE_OP_FLOAT"),
                 (["a", "b"], "b", "IR_SUPER_COMPARE_OP_STR"))
        for xs, y, handler in cases:
            with self.subTest(handler=handler):
                f = make_function(source)
                xs = xs * _testinternalcapi.SPECIALIZATION_THRESHOLD
                expected = xs.count(y) + 10 * (len(xs) - xs.count(y))
                self.assertEqual(f(xs, y), expected)
                ir = handlers(f)
                self.assertEqual(ir.count(handler), 3)
                self.assertIn("POP_JUMP_IF_FALSE", ir)
                self.assertIn("POP_JUMP_IF_TRUE", ir)
                self.assertEqual(f(xs, y), expected)
                # Operands of another type deoptimise the compare
                self.assertEqual(f([None, y], y), 11)
                self.assertEqual(f(xs, y), expected)

    @unittest.skipUnless(SPECIALIZES, "the IR is not specialised")
    def test_branch_events_of_a_compare_and_branch(self):
        # While BRANCH events instrument the jump, the compare deoptimises
        # and specialises again without it.
        def f(xs):
            n = 0
            for x in xs:
                if x < 2:
                    n += 1
            return n

        xs = [1, 2, 3] * _testinternalcapi.SPECIALIZATION_THRESHOLD
        self.assertEqual(f(xs), len(xs) // 3)
        self.assertIn("IR_SUPER_COMPARE_OP_INT", handlers(f))
        jump = [instr for instr in dis.get_instructions(f)
                if instr.opname.startswith("POP_JUMP_IF_")][0]
        if_true = jump.opname == "POP_JUMP_IF_TRUE"
        # More runs than the cooldown of a specialisation
        xs = [1, 2, 3] * 100
        expected = ["right" if (x < 2) == if_true else "left" for x in xs]
        events = []
        E = sys.monitoring.events
        tool = sys.monitoring.DEBUGGER_ID
        sys.monitoring.use_tool_id(tool, "test_ir")
        try:
            for event, name in ((E.BRANCH_LEFT, "left"),
                                (E.BRANCH_RIGHT, "right")):
                sys.monitoring.register_callback(
                    tool, event,
                    lambda code, src, dst, name=name:
                        events.append(name) if src == jump.offset else None)
            sys.monitoring.set_local_events(tool, f.__code__,
                                            E.BRANCH_LEFT | E.BRANCH_RIGHT)
            self.assertEqual(f(xs), len(xs) // 3)
            ir = handlers(f)
        finally:
            sys.monitoring.set_local_events(tool, f.__code__, 0)
            sys.monitoring.free_tool_id(tool)
        self.assertEqual(events, expected)
        self.assertIn("COMPARE_OP_INT", ir)
        self.assertNotIn("IR_SUPER_COMPARE_OP_INT", ir)
        self.assertEqual(f(xs), len(xs) // 3)

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
        _, events = trace(f)
        self.assertEqual([e[2] for e in events if e[0] == "line"],
                         list(range(2, n + 2)))
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
        if SPECIALIZES:
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

    def test_backward_jump_into_unreachable_code(self):
        # The forward pass finds the LOAD_SMALL_INT unreachable, and only the
        # JUMP_BACKWARD after it gives it a depth.
        def f():
            pass
        f.__code__ = f.__code__.replace(co_code=assemble(
            ("RESUME", 0),
            ("JUMP_FORWARD", 2),
            ("LOAD_SMALL_INT", 7),
            ("RETURN_VALUE", 0),
            ("JUMP_BACKWARD", 4)))
        self.assertEqual(f(), 7)

    def test_inconsistent_stack_depth(self):
        def f():
            pass
        code = f.__code__.replace(co_code=assemble(
            ("RESUME", 0),
            ("LOAD_SMALL_INT", 0),
            ("POP_JUMP_IF_FALSE", 1),
            ("LOAD_SMALL_INT", 1),
            ("RETURN_VALUE", 0)))
        with self.assertRaisesRegex(SystemError, "inconsistent stack depth"):
            get_ir(code)

    def test_jump_into_an_instruction(self):
        def f():
            pass
        code = f.__code__.replace(co_code=assemble(
            ("RESUME", 0),
            ("JUMP_FORWARD", 1),
            ("POP_JUMP_IF_FALSE", 0),
            ("RETURN_VALUE", 0)))
        with self.assertRaisesRegex(SystemError, "jump into an instruction"):
            get_ir(code)


if __name__ == "__main__":
    unittest.main()
