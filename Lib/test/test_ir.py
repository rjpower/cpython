"""Tests for the register IR (Include/internal/pycore_ir.h)."""

import dis
import io
import opcode
import sys
import unittest
from test import support
from test.support import import_helper

_testinternalcapi = import_helper.import_module("_testinternalcapi")
get_ir = _testinternalcapi.get_ir

# The free-threaded build does not specialise the IR.
SPECIALIZES = not support.Py_GIL_DISABLED
BORROWED_LOADS = ("LOAD_FAST_BORROW", "LOAD_FAST_BORROW_LOAD_FAST_BORROW")
# Include/internal/pycore_ir.h
IR_MAX_SLOTS = 4095
IR_FIELD_MAX = 4095
IR_MAX_FORWARD = 2047


def instructions(code):
    """The words of get_ir() less the jumps that a relaxed FOR_ITER or SEND
    adds, which share its offsets"""
    words = []
    for word in get_ir(code):
        if words and word[:2] == words[-1][:2]:
            assert word[2] == "JUMP_FORWARD", word
            continue
        words.append(word)
    return words


def handlers(func):
    return [instr[2] for instr in instructions(func.__code__)]


def names_local(func, field):
    """Whether a slot field of func names a local, which an input field does
    only for an absorbed load"""
    return field < len(_testinternalcapi.get_co_localskinds(func.__code__))


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
    anchors = [instr[1] for instr in instructions(func.__code__)]
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


class LoweringTest(unittest.TestCase):

    def check_offsets(self, func):
        # One IR instruction per bytecode instruction but EXTENDED_ARG and
        # the loads that their consumer absorbs, anchored at the bytecode
        # offset of the instruction.  Its offsets start at the first of those
        # before it.
        instrs = list(dis.get_instructions(func))
        i = 0
        for start, anchor, *_ in instructions(func.__code__):
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
        self.assertEqual(b, 0)
        self.check_offsets(f)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 1):
            self.assertEqual(f(C()), 5)
        if SPECIALIZES:
            self.assertNotIn("LOAD_ATTR", handlers(f))
        self.assertNotIn("LOAD_FAST_BORROW", handlers(f))

    def test_deopt_after_borrowing_a_local(self):
        # The specialised LOAD_ATTR borrows its absorbed owner before its
        # guards, and a deopt drops that borrow; a Py_STACKREF_DEBUG build
        # aborts when the frame closes the local with the borrow still out.
        def f(o):
            try:
                return o.attr
            except AttributeError:
                return None
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 1):
            self.assertEqual(f(C()), 5)
        if SPECIALIZES:
            self.assertNotIn("LOAD_ATTR", handlers(f))
        self.assertTrue(names_local(f, self.load_attr_owner_field(f)))
        for _ in range(3):
            self.assertIsNone(f(object()))

    def load_attr_owner_field(self, func):
        # Field B of the word that runs the LOAD_ATTR of func
        anchor = [instr.offset for instr in dis.get_instructions(func)
                  if instr.opname == "LOAD_ATTR"][0]
        return [instr for instr in get_ir(func.__code__)
                if instr[1] == anchor][0][4]

    def test_absorbed_pair(self):
        def f(a, b):
            return a in b
        self.assertNotIn("LOAD_FAST_BORROW_LOAD_FAST_BORROW", handlers(f))
        ir = {instr[2]: instr for instr in get_ir(f.__code__)}
        # Field A is the destination, and B and C name the locals
        _, _, _, _, b, c, _ = ir["CONTAINS_OP"]
        self.assertEqual((b, c), (0, 1))
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
        self.assertFalse(names_local(f, self.load_attr_owner(f)))
        self.assertEqual(f(C(), None), 5)
        self.assertEqual(f(None, C()), 5)

    def test_no_absorption_across_a_line(self):
        def f(o):
            return (o
                    .attr)
        self.assertIn("LOAD_FAST_BORROW", handlers(f))
        self.assertFalse(names_local(f, self.load_attr_owner(f)))
        self.assertEqual(f(C()), 5)

    def test_no_absorption_under_tracing(self):
        def f(o):
            return o.attr
        result, events = trace(f, C(), opcodes=True)
        self.assertEqual(result, 5)
        # A word per instruction, and one event per instruction from the
        # first RESUME on
        offsets = [instr.offset for instr in dis.get_instructions(f)]
        self.assertEqual([instr[1] for instr in instructions(f.__code__)],
                         offsets)
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
    def test_yield_to_each_kind_of_resumer(self):
        # A yield returns past the instruction that resumed the generator:
        # FOR_ITER_GEN, SEND_GEN or the trampoline of a C-level next().  One
        # generator alternates between all three.
        def counter():
            n = 0
            while True:
                n += 1
                yield n

        def delegate(g):
            yield from g

        def f(g, d, rounds):
            seen = []
            for n in g:
                seen += [n, next(g), next(d)]
                if len(seen) >= 3 * rounds:
                    return seen

        rounds = 2 * _testinternalcapi.SPECIALIZATION_THRESHOLD
        g = counter()
        self.assertEqual(f(g, delegate(g), rounds),
                         list(range(1, 3 * rounds + 1)))
        if SPECIALIZES:
            self.assertIn("FOR_ITER_GEN", handlers(f))
            self.assertIn("SEND_GEN", handlers(delegate))

    @unittest.skipUnless(SPECIALIZES, "the IR is not specialised")
    def test_compare_and_branch(self):
        # A specialised compare and the jump that consumes its result keep
        # a word each.
        source = ("def f(xs, y):\n"
                  "    n = 0\n"
                  "    for x in xs:\n"
                  "        if x == y or y == x:\n"
                  "            n += 1\n"
                  "        if x != y:\n"
                  "            n += 10\n"
                  "    return n\n")
        cases = (([1, 2, 3], 2, "COMPARE_OP_INT"),
                 ([1.5, 2.5], 2.5, "COMPARE_OP_FLOAT"),
                 (["a", "b"], "b", "COMPARE_OP_STR"))
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
    def test_branch_events_of_a_specialised_compare(self):
        def f(xs):
            n = 0
            for x in xs:
                if x < 2:
                    n += 1
            return n

        xs = [1, 2, 3] * _testinternalcapi.SPECIALIZATION_THRESHOLD
        self.assertEqual(f(xs), len(xs) // 3)
        self.assertIn("COMPARE_OP_INT", handlers(f))
        jump = [instr for instr in dis.get_instructions(f)
                if instr.opname.startswith("POP_JUMP_IF_")][0]
        if_true = jump.opname == "POP_JUMP_IF_TRUE"
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

    def test_wide_operands(self):
        # Constant indices beyond 4095 take the 24-bit field CD, a name index
        # beyond 4095 of an instruction with an entry lives in the entry, and
        # more than 65535 code units need a pcmap with 32-bit offsets.
        nconsts = 70_000
        nnames = 5_000
        source = ("def f(o):\n"
                  + "".join(f"    global g{i}\n" for i in range(nnames))
                  + "".join(f"    g{i} = 0\n" for i in range(nnames))
                  + "".join(f"    v = {i}.5\n" for i in range(nconsts))
                  + "    return v, o.attr\n")
        f = make_function(source)
        self.assertGreater(len(f.__code__.co_code) // 2, 65535)
        self.assertGreater(f.__code__.co_names.index("attr"), IR_FIELD_MAX)
        ir = get_ir(f.__code__)
        self.assertGreater(ir[-1][1], 2 * 65535)
        # The last LOAD_CONST, which may push its constant inline, names it
        # through CD
        load_const = [instr for instr in ir
                      if instr[2].startswith("LOAD_CONST")][-1]
        _, anchor, _, _, _, c, d = load_const
        index = [instr for instr in dis.get_instructions(f)
                 if instr.offset == anchor][0].arg
        self.assertGreater(index, IR_FIELD_MAX)
        self.assertEqual(d << 12 | c, index)
        self.check_offsets(f)

        # The handlers that run the LOAD_ATTR: specialised, deoptimised and
        # instrumented
        class D:
            attr = 6
        last = nconsts - 1 + 0.5
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 1):
            self.assertEqual(f(C()), (last, 5))
        if SPECIALIZES:
            self.assertNotIn("LOAD_ATTR", handlers(f))
        self.assertEqual(f(D()), (last, 6))
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
        self.assertEqual(result, (last, 5))
        self.assertEqual(lines, nnames + nconsts + 1)

    def test_slot_limit(self):
        # Locals, cells, free variables and the stack share IR_MAX_SLOTS
        # slots.
        def make(nlocals):
            source = ("def f():\n"
                      + "".join(f"    v{i} = {i}\n" for i in range(nlocals))
                      + "    return v0\n")
            return make_function(source)
        stacksize = make(1).__code__.co_stacksize
        f = make(IR_MAX_SLOTS - stacksize)
        self.assertEqual(f.__code__.co_nlocals + f.__code__.co_stacksize,
                         IR_MAX_SLOTS)
        self.assertEqual(f(), 0)
        f = make(IR_MAX_SLOTS - stacksize + 1)
        with self.assertRaisesRegex(SystemError, "slots"):
            f()
        with self.assertRaisesRegex(SystemError, "slots"):
            get_ir(f.__code__)

    def test_shared_feedback_entry(self):
        # The sites past the IR_FIELD_MAX-th share entry 0: they never
        # specialise and read their static operands from the bytecode.
        class O:
            attr = 5
            other = 7
        nsites = IR_FIELD_MAX + 10
        source = ("def f(o, n):\n"
                  + "".join(f"    x = o.attr\n" for i in range(nsites))
                  + "    return (o.attr - o.other, o.other // n,\n"
                  + "            o.attr < o.other, 5 in [o.attr])\n")
        f = make_function(source)
        expected = (-2, 3, True, True)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 1):
            self.assertEqual(f(O(), 2), expected)
        ir = instructions(f.__code__)
        load_attrs = [instr for instr in ir if instr[2].startswith("LOAD_ATTR")]
        # Field C is the feedback index
        self.assertEqual([instr[5] for instr in load_attrs[:3]], [1, 2, 3])
        self.assertEqual({instr[5] for instr in load_attrs[IR_FIELD_MAX:]},
                         {0})
        names = [instr[2] for instr in ir]
        if SPECIALIZES:
            self.assertNotEqual(load_attrs[0][2], "LOAD_ATTR")
        self.assertEqual({instr[2] for instr in load_attrs[IR_FIELD_MAX:]},
                         {"LOAD_ATTR"})
        self.assertIn("BINARY_OP_SUBTRACT", names)
        self.assertIn("BINARY_OP_FLOOR_DIVIDE", names)
        self.assertIn("COMPARE_OP", names)
        self.assertIn("CONTAINS_OP", names)
        # Other operands take the generic paths
        self.assertEqual(f(O(), 2.0), (-2, 3.0, True, True))
        with self.assertRaises(ZeroDivisionError):
            f(O(), 0)
        # The shared entry holds nothing that dis could show
        dis.dis(f, adaptive=True, file=io.StringIO())

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

    def test_binary_op_handlers(self):
        # BINARY_OP runs the handler of its operation
        def f(a, b):
            a -= b
            return a - b, a * b
        self.assertEqual(handlers(f).count("BINARY_OP_SUBTRACT"), 1)
        self.assertIn("BINARY_OP_INPLACE_SUBTRACT", handlers(f))
        self.assertIn("BINARY_OP_MULTIPLY", handlers(f))
        self.assertEqual(f(7, 2), (3, 10))
        self.assertEqual(f(7.0, 2), (3.0, 10.0))
        ops = [instr.opname for instr in dis.get_instructions(f, adaptive=True)]
        self.assertNotIn("BINARY_OP_SUBTRACT", ops)

    @unittest.skipUnless(SPECIALIZES, "the IR is not specialised")
    def test_binary_op_deopt(self):
        # A specialisation of BINARY_OP deoptimises to the handler of its own
        # operation, which the specialiser installs when it gives up.
        class Sub:
            def __sub__(self, other):
                return "sub"
            def __rsub__(self, other):
                return "rsub"
            def __add__(self, other):
                return "add"

        def f(a, b):
            return a - b
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 1):
            self.assertEqual(f(3, 1), 2)
        self.assertIn("BINARY_OP_SUBTRACT_INT", handlers(f))
        self.assertEqual(f(Sub(), 1), "sub")
        self.assertEqual(f(1, Sub()), "rsub")
        # Each deoptimisation advances the counter of the specialisation
        for _ in range(_testinternalcapi.SPECIALIZATION_COOLDOWN + 1):
            self.assertEqual(f(Sub(), 1), "sub")
        self.assertIn("BINARY_OP_SUBTRACT", handlers(f))
        self.assertNotIn("BINARY_OP_SUBTRACT_INT", handlers(f))
        self.assertEqual(f(3, 1), 2)


class RelaxationTest(unittest.TestCase):
    """FOR_ITER and SEND reach IR_MAX_FORWARD words; a loop whose exit is
    further away exits through jumps after its word."""

    def relaxed(self, func, opname):
        # The word of the first opname instruction of func, which may be
        # specialised, and the words after it that share its offsets
        ir = get_ir(func.__code__)
        i = [k for k, instr in enumerate(ir)
             if instr[2].startswith(opname)][0]
        jumps = []
        for instr in ir[i + 1:]:
            if instr[:2] != ir[i][:2]:
                break
            jumps.append(instr[2])
        return ir[i], jumps

    def test_far_for_iter(self):
        nlines = IR_MAX_FORWARD
        source = ("def f(it):\n"
                  "    n = 0\n"
                  "    for x in it:\n"
                  "        if x < 0:\n"
                  "            continue\n"
                  "        if x > 100:\n"
                  "            break\n"
                  + "        n = n + x\n" * nlines
                  + "    return n\n")
        f = make_function(source)

        def gen(n):
            yield from range(n)

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 1):
            self.assertEqual(f(range(10)), 45 * nlines)
            self.assertEqual(f([1, -1, 2, 200, 3]), 3 * nlines)
            self.assertEqual(f(gen(4)), 6 * nlines)
            self.assertEqual(f(()), 0)
        word, jumps = self.relaxed(f, "FOR_ITER")
        self.assertEqual(jumps, ["JUMP_FORWARD"] * 3)
        # Field D: the exit is the third jump
        self.assertEqual(word[6], 1)
        LoweringTest.check_offsets(self, f)

        # A generator returns to the second jump
        f = make_function(source)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 1):
            self.assertEqual(f(gen(4)), 6 * nlines)
        if SPECIALIZES:
            self.assertEqual(self.relaxed(f, "FOR_ITER")[0][2], "FOR_ITER_GEN")

    def run_relaxed(self, source, name):
        # A function of source lowered with every loop relaxed, and one
        # lowered as usual
        old = _testinternalcapi.set_ir_max_forward(0)
        try:
            relaxed = make_function(source, name)
            get_ir(relaxed.__code__)
        finally:
            _testinternalcapi.set_ir_max_forward(old)
        return relaxed, make_function(source, name)

    def test_relaxed_send(self):
        source = """if 1:
            def inner():
                x = yield 1
                yield x
                return "done"

            def outer():
                r = yield from inner()
                yield r

            class Awaitable:
                def __await__(self):
                    yield "a"
                    return "b"

            async def co():
                return await Awaitable()

            def run():
                g = outer()
                values = [next(g), g.send(5), next(g)]
                try:
                    next(g)
                except StopIteration:
                    values.append("stop")
                c = co()
                values.append(c.send(None))
                try:
                    c.send(None)
                except StopIteration as e:
                    values.append(e.value)
                return values
        """
        namespace = {}
        old = _testinternalcapi.set_ir_max_forward(0)
        try:
            exec(source, namespace)
            for func in ("outer", "co"):
                get_ir(namespace[func].__code__)
        finally:
            _testinternalcapi.set_ir_max_forward(old)
        for name in ("outer", "co"):
            _, jumps = self.relaxed(namespace[name], "SEND")
            self.assertEqual(jumps, ["JUMP_FORWARD"] * 2)
        expected = [1, 5, "done", "stop", "a", "b"]
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 1):
            self.assertEqual(namespace["run"](), expected)
        if SPECIALIZES:
            self.assertIn("SEND_GEN", handlers(namespace["outer"]))

    def test_events_of_a_relaxed_loop(self):
        # The jumps that relaxation adds have no events, and the branches of
        # the loop go where they would without them.
        source = """if 1:
            def f(xs):
                n = 0
                for x in xs:
                    if x:
                        n += 1
                return n
        """
        relaxed, plain = self.run_relaxed(source, "f")
        self.assertEqual(self.relaxed(relaxed, "FOR_ITER")[1],
                         ["JUMP_FORWARD"] * 3)

        def branches(func):
            events = []
            E = sys.monitoring.events
            tool = sys.monitoring.DEBUGGER_ID
            sys.monitoring.use_tool_id(tool, "test_ir")
            try:
                for event in (E.BRANCH_LEFT, E.BRANCH_RIGHT, E.JUMP):
                    sys.monitoring.register_callback(
                        tool, event,
                        lambda code, src, dst, event=event:
                            events.append((event, src, dst)))
                sys.monitoring.set_local_events(
                    tool, func.__code__,
                    E.BRANCH_LEFT | E.BRANCH_RIGHT | E.JUMP)
                self.assertEqual(func([0, 1, 1]), 2)
            finally:
                sys.monitoring.set_local_events(tool, func.__code__, 0)
                sys.monitoring.free_tool_id(tool)
            return events

        self.assertEqual(branches(relaxed), branches(plain))
        self.assertEqual(trace(relaxed, [0, 1, 1], opcodes=True),
                         trace(plain, [0, 1, 1], opcodes=True))


if __name__ == "__main__":
    unittest.main()
