"""Tests for the register IR (Include/internal/pycore_ir.h) that the
assembler (Python/assemble.c) emits."""

import dis
import io
import sys
import types
import unittest
from test import support
from test.support import import_helper
from test.support.bytecode_helper import AssemblerTestCase, encode_words

_testinternalcapi = import_helper.import_module("_testinternalcapi")
get_ir = _testinternalcapi.get_ir

# The free-threaded build does not specialise the IR.
SPECIALIZES = not support.Py_GIL_DISABLED
# Include/internal/pycore_ir.h
IR_MAX_SLOTS = 4095
IR_FIELD_MAX = 4095
IR_MAX_FORWARD = 2047


def handlers(func):
    """The handlers of the executing copy of func's code"""
    return [word[0] for word in get_ir(func.__code__, True)]


def nlocalsplus(func):
    return len(_testinternalcapi.get_co_localskinds(func.__code__))


def make_function(source, name="f"):
    namespace = {}
    exec(source, namespace)
    return namespace[name]


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


# The attributes are instance attributes, whose loads specialise.  A class
# attribute holding an inline int is classified INLINE_VALUE and its load
# stays generic (Python/specialize.c).
class C:
    def __init__(self):
        self.attr = 5


class AssemblerTest(unittest.TestCase):

    def check_offsets(self, func):
        # An instruction per word, at the word's index, and line ranges
        # that cover the words
        nwords = len(func.__code__.co_code) // 8
        self.assertEqual(len(get_ir(func.__code__)), nwords)
        self.assertEqual([instr.offset for instr in dis.get_instructions(func)],
                         list(range(nwords)))
        ranges = [(start, end) for start, end, _ in func.__code__.co_lines()]
        self.assertEqual(ranges[0][0], 0)
        self.assertEqual(ranges[-1][1], nwords)

    def test_offsets(self):
        def f(a, b):
            for x in a:
                if x > b:
                    return x * 2
            return None
        self.check_offsets(f)

    def test_stack_slots(self):
        # The temporary at depth d lives in slot nlocalsplus + d
        def f(a, b):
            return a.x in b.y
        base = nlocalsplus(f)
        contains = [instr for instr in dis.get_instructions(f)
                    if instr.opname == "CONTAINS_OP"][0]
        self.assertEqual(contains.dst, base)
        self.assertEqual(contains.srcs, (base, base + 1))
        o = types.SimpleNamespace(x=1, y=[1])
        self.assertTrue(f(o, o))

    def test_borrowed_load_folds(self):
        # The load of the owner has no word: LOAD_ATTR reads the local
        def f(o):
            return o.attr
        ir = get_ir(f.__code__)
        self.assertNotIn("LOAD_FAST_BORROW", [word[0] for word in ir])
        attr = [word for word in ir if word[0] == "LOAD_ATTR"][0]
        self.assertEqual(attr[1], nlocalsplus(f))
        self.assertEqual(attr[2], 0)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 1):
            self.assertEqual(f(C()), 5)
        if SPECIALIZES:
            self.assertNotIn("LOAD_ATTR", handlers(f))
        # The words of the code object stay as the compiler emitted them
        self.assertEqual(get_ir(f.__code__), ir)

    def test_deopt_after_borrowing_a_local(self):
        # A deoptimising LOAD_ATTR must preserve the local's owner.
        def f(o):
            try:
                return o.attr
            except AttributeError:
                return None
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 1):
            self.assertEqual(f(C()), 5)
        if SPECIALIZES:
            self.assertNotIn("LOAD_ATTR", handlers(f))
        for _ in range(3):
            self.assertIsNone(f(object()))

    def test_exception_from_a_pushed_frame(self):
        # LOAD_ATTR_PROPERTY pushes the frame of the getter.  When the getter
        # raises, the unwinding of f looks up the word of LOAD_ATTR in the
        # exception table.
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

    def test_line_events(self):
        def f(o):
            x = 1
            return o.attr
        self.assertEqual(f(C()), 5)
        result, events = trace(f, C())
        self.assertEqual(result, 5)
        line = f.__code__.co_firstlineno
        self.assertEqual([e[2] for e in events if e[0] == "line"],
                         [line + 1, line + 2])
        # The next RESUME removes the instrumentation
        self.assertEqual(f(C()), 5)
        self.assertNotIn("INSTRUMENTED_LINE", handlers(f))

    def test_opcode_events(self):
        # One event per word from the first RESUME on, at its index
        def f(o):
            return o.attr
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 1):
            self.assertEqual(f(C()), 5)
        result, events = trace(f, C(), opcodes=True)
        self.assertEqual(result, 5)
        lasti = list(range(1, len(get_ir(f.__code__))))
        self.assertEqual([e[1] for e in events if e[0] == "opcode"], lasti)
        self.assertEqual(f(C()), 5)

    def test_frames_when_instrumented(self):
        # Frames that are in the code when instrumentation rewrites its
        # executing copy go on in it: a suspended generator, and a frame in
        # a call that turns on opcode events for it and raises.
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
        sys.settrace(lambda *args: None)
        try:
            self.assertEqual(f(C()), 10)
            g.gi_frame.f_trace_opcodes = True
            g.gi_frame.f_trace = lambda *args: None
            self.assertEqual(next(g), 5)
        finally:
            sys.settrace(None)
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
        self.assertIn("FOR_ITER_GEN", handlers(f))
        self.assertIn("SEND_GEN", handlers(delegate))

    @unittest.skipUnless(SPECIALIZES, "the IR is not specialised")
    def test_compare_and_branch(self):
        # A specialised compare and the jump that consumes its result are
        # one word.
        source = ("def f(xs, y):\n"
                  "    n = 0\n"
                  "    for x in xs:\n"
                  "        if x == y or y == x:\n"
                  "            n += 1\n"
                  "        if x != y:\n"
                  "            n += 10\n"
                  "    return n\n")
        cases = (([1, 2, 3], 2, "COMPARE_OP_JUMP_INT"),
                 ([1.5, 2.5], 2.5, "COMPARE_OP_JUMP_FLOAT"),
                 (["a", "b"], "b", "COMPARE_OP_JUMP_STR"))
        for xs, y, handler in cases:
            with self.subTest(handler=handler):
                f = make_function(source)
                xs = xs * _testinternalcapi.SPECIALIZATION_THRESHOLD
                expected = xs.count(y) + 10 * (len(xs) - xs.count(y))
                self.assertEqual(f(xs, y), expected)
                ir = handlers(f)
                self.assertEqual(ir.count(handler), 3)
                self.assertNotIn("POP_JUMP_IF_FALSE_BOOL", ir)
                self.assertNotIn("POP_JUMP_IF_TRUE_BOOL", ir)
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
        self.assertIn("COMPARE_OP_JUMP_INT", handlers(f))
        jump = [instr for instr in dis.get_instructions(f)
                if instr.opname.startswith("COMPARE_OP_JUMP")][0]
        if_true = "if true" in jump.argrepr
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
        self.assertIn("INSTRUMENTED_COMPARE_OP_JUMP", ir)
        self.assertEqual(f(xs), len(xs) // 3)

    def test_f_lasti(self):
        def f():
            return sys._getframe().f_lasti
        load_attr = [instr for instr in dis.get_instructions(f)
                     if instr.argval == "f_lasti"]
        self.assertEqual(f(), load_attr[0].offset)

    def test_many_constants(self):
        # Constant indices beyond 255 fit in a field: no EXTENDED_ARG
        n = 300
        source = "def f():\n" + "".join(f"    a = {i}.5\n" for i in range(n))
        f = make_function(source)
        self.assertNotIn("EXTENDED_ARG", [word[0] for word in get_ir(f.__code__)])
        _, events = trace(f)
        self.assertEqual([e[2] for e in events if e[0] == "line"],
                         list(range(2, n + 2)))
        self.check_offsets(f)

    def test_wide_operands(self):
        # Constant indices beyond 4095 take the 24-bit field CD, and a name
        # index beyond 4095 of an instruction with an entry lives in the
        # entry.
        nconsts = 70_000
        nnames = 5_000
        source = ("def f(o):\n"
                  + "".join(f"    global g{i}\n" for i in range(nnames))
                  + "".join(f"    g{i} = 0\n" for i in range(nnames))
                  + "".join(f"    v = {i}.5\n" for i in range(nconsts))
                  + "    return v, o.attr\n")
        f = make_function(source)
        self.assertGreater(len(f.__code__.co_code) // 8, 65535)
        self.assertGreater(f.__code__.co_names.index("attr"), IR_FIELD_MAX)
        ir = get_ir(f.__code__)
        # The last LOAD_CONST, which may push its constant inline, names it
        # through CD
        index = [k for k, word in enumerate(ir)
                 if word[0].startswith("LOAD_CONST")][-1]
        _, _, _, c, d = ir[index]
        arg = list(dis.get_instructions(f))[index].arg
        self.assertGreater(arg, IR_FIELD_MAX)
        self.assertEqual(d << 12 | c, arg)
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

        o = C()
        sys.settrace(tracer)
        try:
            result = f(o)
        finally:
            sys.settrace(None)
        self.assertEqual(result, (last, 5))
        self.assertEqual(lines, nnames + nconsts + 1)

    def test_slot_limit(self):
        # Locals, cells, free variables and the stack share IR_MAX_SLOTS
        # slots; code that needs more does not compile.
        def make(nlocals):
            source = ("def f():\n"
                      + "".join(f"    v{i} = {i}\n" for i in range(nlocals))
                      + "    return [v0]\n")
            return make_function(source)
        # The list's window is the one temporary
        stacksize = make(1).__code__.co_stacksize
        self.assertEqual(stacksize, 1)
        f = make(IR_MAX_SLOTS - stacksize)
        self.assertEqual(f.__code__.co_nlocals + f.__code__.co_stacksize,
                         IR_MAX_SLOTS)
        self.assertEqual(f(), [0])
        with self.assertRaisesRegex(SyntaxError, "slots"):
            make(IR_MAX_SLOTS - stacksize + 1)

    def test_shared_feedback_entry(self):
        # The sites that get no entry (fb 0) share entry 0, a header: they
        # never specialise and keep running their generic handler, which
        # reads its operand from the word.  The compare and contains sites
        # have their entries before all the others.
        class O:
            def __init__(self):
                self.attr = 5
                self.other = 7
        nsites = 1000     # attribute entries are 40 bytes: 817 fit after the compares
        source = ("def f(o, n):\n"
                  + "".join(f"    x = o.attr\n" for i in range(nsites))
                  + "    return (o.attr - o.other, o.other // n,\n"
                  + "            o.attr < o.other, 5 in [o.attr])\n")
        f = make_function(source)
        expected = (-2, 3, True, True)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD * 4):
            self.assertEqual(f(O(), 2), expected)
        instrs = list(dis.get_instructions(f))
        # The fused pairs, as far as their compares keep an entry, and the
        # compares alone
        compares = [i for i in instrs
                    if i.opname.startswith(("COMPARE_OP", "CONTAINS_OP"))]
        self.assertEqual([i.fb for i in compares], [2, 6])
        load_attrs = [i for i in instrs if i.opname == "LOAD_ATTR"]
        self.assertEqual(load_attrs[0].fb, 10)
        self.assertEqual(load_attrs[1].fb, 15)
        shared = [i for i in load_attrs if i.fb == 0]
        owned = [i for i in load_attrs if i.fb != 0]
        self.assertEqual(len(owned), 817)
        self.assertEqual(len(shared), nsites + 6 - len(owned))
        self.assertEqual({i.argval for i in shared}, {"attr", "other"})
        ir = handlers(f)
        self.assertEqual({ir[i.offset] for i in shared}, {"LOAD_ATTR"})
        if SPECIALIZES:
            self.assertNotEqual(ir[owned[0].offset], "LOAD_ATTR")
        # The binary operations, last in source order, have no entry left
        # and keep the handlers of their operations
        for name in ("BINARY_OP_SUBTRACT", "BINARY_OP_FLOOR_DIVIDE"):
            self.assertIn(name, ir)
        # Other operands take the generic paths, and many runs more change
        # nothing about the shared sites
        self.assertEqual(f(O(), 2.0), (-2, 3.0, True, True))
        with self.assertRaises(ZeroDivisionError):
            f(O(), 0)
        header = f.__code__._co_feedback[:4]       # kind, state, counter
        self.assertEqual(header[:2], bytes([0, 4]))  # COUNTER, OFF
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD * 8):
            f(O(), 2)
        after = f.__code__._co_feedback[:4]
        self.assertEqual(after[:2], header[:2])
        # The counter of entry 0 stays unreachable (backoff bits 7)
        self.assertEqual(after[2] & 7, 7)
        self.assertEqual({handlers(f)[i.offset] for i in shared}, {"LOAD_ATTR"})
        # The shared entry is the header and nothing else is written
        dis.dis(f, adaptive=True, show_caches=True, file=io.StringIO())
        fbinit = f.__code__.co_fbinit
        self.assertLessEqual(int.from_bytes(fbinit[:4], sys.byteorder) & 0xFFFF,
                             4096)
        self.assertEqual(len(f.__code__._co_feedback),
                         (int.from_bytes(fbinit[:4], sys.byteorder) & 0xFFFF) * 8)

    def test_sizeof(self):
        # The first call makes the executing copy
        code = compile("x = [i for i in range(3)]", "<test>", "exec")
        before = sys.getsizeof(code)
        exec(code, {})
        self.assertGreater(sys.getsizeof(code), before)

    def test_constructor_checks_the_words(self):
        def f(a):
            return a.attr
        code = f.__code__
        words = get_ir(code)
        index = [k for k, word in enumerate(words) if word[0] == "LOAD_ATTR"][0]
        _, a, b, c, d = words[index]

        def with_word(word):
            return code.co_code[:8 * index] + encode_words(word) \
                + code.co_code[8 * (index + 1):]

        nslots = nlocalsplus(f) + code.co_stacksize
        with self.assertRaisesRegex(ValueError, "slot"):
            code.replace(co_code=with_word(("LOAD_ATTR", nslots, b, c, d)))
        with self.assertRaisesRegex(ValueError, "feedback"):
            code.replace(co_code=with_word(("LOAD_ATTR", a, b, IR_FIELD_MAX, d)))
        with self.assertRaisesRegex(ValueError, "malformed"):
            code.replace(co_code=code.co_code + b"\0")
        self.assertEqual(code.replace(co_code=with_word(("LOAD_ATTR", a, b, c, d))),
                         code)

    def test_binary_op_handlers(self):
        # BINARY_OP runs the handler of its operation
        def f(a, b):
            a -= b
            return a - b, a * b
        ir = [word[0] for word in get_ir(f.__code__)]
        self.assertEqual(ir.count("BINARY_OP_SUBTRACT"), 1)
        self.assertIn("BINARY_OP_INPLACE_SUBTRACT", ir)
        self.assertIn("BINARY_OP_MULTIPLY", ir)
        self.assertEqual(f(7, 2), (3, 10))
        self.assertEqual(f(7.0, 2), (3.0, 10.0))
        ops = [instr.opname for instr in dis.get_instructions(f)]
        self.assertNotIn("BINARY_OP_SUBTRACT", ops)
        self.assertEqual(ops.count("BINARY_OP"), 3)

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


def relaxed_compile(source):
    """The namespace of source compiled with every loop relaxed"""
    old = _testinternalcapi.set_ir_max_forward(0)
    try:
        namespace = {}
        exec(source, namespace)
    finally:
        _testinternalcapi.set_ir_max_forward(old)
    return namespace


class SlotLimitTest(unittest.TestCase):
    """Running out of slots is a SyntaxError naming the limit."""

    def test_too_many_temporaries(self):
        source = "def f(a, b, c, d):\n    return a + (b + (c + (d + a)))\n"
        old = _testinternalcapi.set_ir_max_slots(2)
        try:
            with self.assertRaisesRegex(SyntaxError,
                                        r"too many temporaries.*limit 2"):
                compile(source, "<limit>", "exec")
        finally:
            _testinternalcapi.set_ir_max_slots(old)
        namespace = {}
        exec(source, namespace)
        self.assertEqual(namespace["f"](1, 2, 3, 4), 11)

    def test_knob_range(self):
        for value in (-1, 4096):
            with self.assertRaises(ValueError):
                _testinternalcapi.set_ir_max_slots(value)


def fb_entries(code):
    """The (kind, unit, arg, units) of each entry of code's feedback table,
    entry 0 first"""
    return [(kind, unit, arg, units) for kind, arg, units, unit in
            ((*dis._feedback_entry(code.co_fbinit, w), w) for w in
             [(int.from_bytes(code.co_fbinit[i:i + 4], sys.byteorder) >> 16)
              & IR_FIELD_MAX for i in range(0, len(code.co_fbinit), 4)])]


COMPARE_OPS = ("<", "<=", "==", "!=", ">", ">=", "in", "not in")


class FeedbackLayoutTest(unittest.TestCase):
    """The feedback vector: 8-byte units, per-kind sizes, entry 0 a header,
    compare sites first, and the limits (Python/assemble.c)"""

    def test_layout(self):
        def f(o, n):
            pre = o.pre
            for i in range(n):
                if o.a < i and o.b in (1, 2):
                    o.c = o.a - i
            return o.a
        code = f.__code__
        entries = fb_entries(code)
        kinds = dis._FEEDBACK_KINDS
        self.assertEqual(entries[0][:2], (0, 0))
        self.assertEqual(entries[0][3], 2)
        # Entries are in order of unit, without gaps; entry 0 gives the total
        total = int.from_bytes(code.co_fbinit[:4], sys.byteorder) & 0xFFFF
        position = 2
        sizes = {"UNARY": (24,), "BINARY": (32,), "ATTR": (40,),
                 "GLOBAL": (32,), "CALL": (24,), "BRANCH": (16, 24)}
        for kind, unit, arg, units in entries[1:]:
            self.assertEqual(unit, position)
            self.assertIn(units * 8, sizes[kinds[kind]])
            position += units
        self.assertEqual(position, total)
        # The compare and contains entries come first, then the sites in the
        # loop, then the others
        by_name = {}
        for instr in dis.get_instructions(f):
            by_name.setdefault(instr.opname, []).append(instr.fb)
        self.assertEqual(by_name["COMPARE_OP_JUMP"], [2])
        self.assertEqual(by_name["CONTAINS_OP_JUMP"], [6])
        pre, a1, b, a2 = by_name["LOAD_ATTR"][:4]
        after = by_name["LOAD_ATTR"][4]
        self.assertEqual(sorted([a1, b, a2, by_name["STORE_ATTR"][0]]),
                         [a1, b, a2, by_name["STORE_ATTR"][0]])
        self.assertLess(by_name["STORE_ATTR"][0], pre)
        self.assertLess(pre, after)
        o = types.SimpleNamespace(a=1, b=2, c=0, pre=0)
        self.assertEqual(f(o, 4), 1)
        self.assertEqual(len(f.__code__._co_feedback), total * 8)

    def test_loop_sites_before_others(self):
        # In a function, the sites inside a loop get entries before the sites
        # outside; in a module body no site is ranked as in a loop
        source = ("def f(o, n):\n"
                  "    x = o.a\n"
                  "    for i in range(n):\n"
                  "        y = o.b\n"
                  "    return o.c\n")
        f = make_function(source)
        attrs = [i for i in dis.get_instructions(f) if i.opname == "LOAD_ATTR"]
        self.assertEqual([i.argval for i in attrs], ["a", "b", "c"])
        self.assertLess(attrs[1].fb, attrs[0].fb)
        self.assertLess(attrs[0].fb, attrs[2].fb)
        module = compile("x = a.p\nfor i in range(3):\n    y = a.q\nz = a.r\n",
                         "<test>", "exec")
        attrs = [i for i in dis.get_instructions(module)
                 if i.opname == "LOAD_ATTR"]
        self.assertEqual(sorted(i.fb for i in attrs), [i.fb for i in attrs])

    def test_loops_are_in_the_flow_graph(self):
        # The handler of a try in a loop is laid out after the function and
        # jumps back to the loop; the code after the loop is not in it
        def f(o, n):
            o.pre0
            for i in range(n):
                try:
                    o.inside
                except KeyError:
                    pass
            o.after
        fbs = {i.argval: i.fb for i in dis.get_instructions(f)
               if i.opname == "LOAD_ATTR"}
        self.assertLess(fbs["inside"], fbs["pre0"])
        self.assertLess(fbs["pre0"], fbs["after"])

    def compare_source(self, ncopies, extra=""):
        lines = ["def f(a, b, c=None, d=None):", "    r = []", "    n = 0"]
        for i in range(ncopies):
            for op in COMPARE_OPS:
                lines.append(f"    r.append(a {op} b)")
                lines.append(f"    if a {op} b:")
                lines.append("        n += 1")
        lines.append(extra)
        lines.append("    return r, n")
        return "\n".join(lines) + "\n"

    def test_compare_sites_past_the_entry_limit(self):
        # 1023 compare entries fit; the other sites become intrinsic calls
        # of the same result, whatever the operation and the use of the result
        ncopies = 70
        f = make_function(self.compare_source(ncopies))
        nsites = ncopies * len(COMPARE_OPS) * 2
        instrs = list(dis.get_instructions(f))
        # The fused pairs, as far as their compares keep an entry, and the
        # compares alone
        compares = [i for i in instrs
                    if i.opname.startswith(("COMPARE_OP", "CONTAINS_OP"))]
        intrinsics = [i for i in instrs if i.opname == "CALL_INTRINSIC_2"]
        self.assertEqual(len(compares), 1023)
        self.assertEqual(len(intrinsics), nsites - 1023)
        self.assertEqual([i.fb for i in compares],
                         [2 + 4 * k for k in range(1023)])
        names = {i.argrepr for i in intrinsics}
        self.assertTrue(all(n.startswith(("INTRINSIC_COMPARE_", "INTRINSIC_CONTAINS_"))
                            for n in names), names)
        total = int.from_bytes(f.__code__.co_fbinit[:4], sys.byteorder) & 0xFFFF
        self.assertEqual(total, 2 + 4 * 1023)

        import operator
        ops = {"<": operator.lt, "<=": operator.le, "==": operator.eq,
               "!=": operator.ne, ">": operator.gt, ">=": operator.ge,
               "in": lambda a, b: a in b, "not in": lambda a, b: a not in b}
        def expect(a, b):
            r, n = [], 0
            for _ in range(ncopies):
                for op in COMPARE_OPS:
                    v = ops[op](a, b)
                    r.append(v)
                    n += bool(v)
            return r, n
        for a, b in ((1, 2), (3, 3), (2.5, 1), ("a", "ab"), (1, [1, 2]),
                     (4, [1, 2]), (2, {2: 3})):
            try:
                want = expect(a, b)
            except TypeError:
                with self.assertRaises(TypeError):
                    f(a, b)
                continue
            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD * 2):
                self.assertEqual(f(a, b), want)

    def test_compare_error_at_an_intrinsic_site(self):
        same = [1, 2]
        for extra, good, bad, message in (
                ("c < d", (1, 5), (1, "x"), "not supported between"),
                ("c in d", (1, [1, 5]), (1, 5.5), "not a container|not iterable")):
            lines = self.compare_source(75).splitlines()
            lines[-2:-1] = [f"    r.append({extra})"]
            f = make_function("\n".join(lines) + "\n")
            self.assertTrue(f(same, same, *good)[0][-1])
            with self.assertRaisesRegex(TypeError, message):
                f(same, same, *bad)

    def test_big_vector(self):
        # More than 4,096 units of entries: the later sites share entry 0
        class O:
            def __init__(self):
                self.attr = 5
        lines = ["def f(o):", "    t = 0"]
        for i in range(2000):
            lines.append("    t += o.attr")
        lines.append("    return t")
        f = make_function("\n".join(lines) + "\n")
        total = int.from_bytes(f.__code__.co_fbinit[:4], sys.byteorder) & 0xFFFF
        self.assertLessEqual(total, 4096)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD * 3):
            self.assertEqual(f(O()), 10000)
        self.assertEqual(len(f.__code__._co_feedback), total * 8)
        # Round trips through marshal and the constructor
        import marshal
        g = types.FunctionType(marshal.loads(marshal.dumps(f.__code__)), {})
        self.assertEqual(g(O()), 10000)
        self.assertEqual(f.__code__.replace(co_fbinit=f.__code__.co_fbinit),
                         f.__code__)

    def test_fbinit_is_checked(self):
        import struct
        def f(a):
            return a.x
        c = f.__code__
        entries = fb_entries(c)
        self.assertEqual(entries[1][0], dis._FEEDBACK_KINDS.index("ATTR"))
        def table(*words):
            return struct.pack(f"<{len(words)}I", *words)
        def word(kind, unit, arg):
            return kind << 28 | unit << 16 | arg
        attr = dis._FEEDBACK_KINDS.index("ATTR")
        good = table(word(0, 0, 7), word(attr, 2, 0))
        self.assertEqual(c.replace(co_fbinit=good).co_fbinit, good)
        bad = {
            "entry 0 is not a counter": table(word(attr, 0, 7), word(attr, 2, 0)),
            "unit out of order": table(word(0, 0, 12), word(attr, 7, 0),
                                       word(attr, 2, 0)),
            "entry does not fit": table(word(0, 0, 6), word(attr, 2, 0)),
            "entry in the header": table(word(0, 0, 7), word(attr, 1, 0)),
            "past the limit": table(word(0, 0, 5000), word(attr, 2, 0)),
            "no entry 0 units": table(word(0, 0, 1), word(attr, 2, 0)),
        }
        for name, fbinit in bad.items():
            with self.subTest(name):
                with self.assertRaisesRegex(ValueError, "malformed"):
                    c.replace(co_fbinit=fbinit)
        # A word that names a unit that starts no entry, or whose entry is
        # too small for its handler
        short = table(word(0, 0, 5), word(dis._FEEDBACK_KINDS.index("CALL"), 2, 0))
        with self.assertRaisesRegex(ValueError, "feedback"):
            c.replace(co_fbinit=short)


class AllOps:
    """Supports every operation of COMPARE_OPS, with the operand on either side"""
    def __lt__(self, other): return True
    __gt__ = __le__ = __ge__ = __lt__
    def __contains__(self, item): return item == 1


class IntrinsicSiteTest(unittest.TestCase):
    """Sites past the limits: the intrinsic calls, entry 0, and the tools"""

    def mixed(self, nattrs=6000, ncopies=70):
        # More attribute loads than 2,048 names allow, and more compares
        # than the vector holds, in one function
        lines = ["def f(o, a, b):", "    t = 0", "    r = []"]
        for i in range(nattrs):
            lines.append(f"    t += o.n{i}")
        for _ in range(ncopies):
            for op in COMPARE_OPS:
                lines.append(f"    r.append(a {op} b)")
        lines.append("    return t, r")
        return make_function("\n".join(lines) + "\n")

    def expected(self, a, b, ncopies=70):
        import operator
        ops = {"<": operator.lt, "<=": operator.le, "==": operator.eq,
               "!=": operator.ne, ">": operator.gt, ">=": operator.ge,
               "in": lambda a, b: a in b, "not in": lambda a, b: a not in b}
        return [ops[op](a, b) for _ in range(ncopies) for op in COMPARE_OPS]

    def test_mixed_function_marshal_and_tools(self):
        import marshal
        n = 6000
        o = types.SimpleNamespace(**{f"n{i}": 1 for i in range(n)})
        f = self.mixed(n)
        a, b = 1, AllOps()
        want = (n, self.expected(a, b))
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD * 2):
            self.assertEqual(f(o, a, b), want)
        g = types.FunctionType(marshal.loads(marshal.dumps(f.__code__)),
                               f.__globals__)
        self.assertEqual(g.__code__, f.__code__)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD * 2):
            self.assertEqual(g(o, a, b), want)

        # Monitoring and tracing see the same results at the sites that
        # share entry 0 or call an intrinsic
        events = []
        mon = sys.monitoring
        mon.use_tool_id(3, "test_ir")
        try:
            mon.set_events(3, mon.events.INSTRUCTION | mon.events.LINE)
            mon.register_callback(3, mon.events.INSTRUCTION,
                                  lambda code, off: events.append(off))
            mon.register_callback(3, mon.events.LINE,
                                  lambda code, line: mon.DISABLE)
            self.assertEqual(f(o, a, b), want)
        finally:
            mon.set_events(3, 0)
            mon.free_tool_id(3)
        self.assertGreater(len(events), n)
        lines = []
        def tracer(frame, event, arg):
            if frame.f_code is f.__code__:
                frame.f_trace_opcodes = True
                lines.append(event)
            return tracer
        sys.settrace(tracer)
        try:
            self.assertEqual(f(o, a, b), want)
        finally:
            sys.settrace(None)
        self.assertGreater(len(lines), n)

    def test_compares_match_plain_python(self):
        class Rich:
            def __init__(self, v):
                self.v = v
            def __lt__(self, other):
                return [self.v]
            def __eq__(self, other):
                return NotImplemented
            def __contains__(self, item):
                return item == self.v
            __hash__ = None
        class BadBool:
            def __bool__(self):
                raise ZeroDivisionError
        class Rich2(AllOps):
            def __lt__(self, other):
                return BadBool()
            __gt__ = __lt__
            def __contains__(self, item):
                raise KeyError(item)
        f = make_function(FeedbackLayoutTest.compare_source(
            FeedbackLayoutTest, 70))
        nan = float("nan")
        cases = [(1, 2), (nan, nan), (nan, 1.0), (2.5, 2.5), ("a", "abc"),
                 (1, AllOps()), (Rich(1), Rich(1)), (Rich(2), [1])]
        for a, b in cases:
            for _ in range(2):
                try:
                    want = self.expected(a, b, 70)
                except TypeError:
                    with self.assertRaises(TypeError):
                        f(a, b)
                    continue
                r, count = f(a, b)
                self.assertEqual([x is True for x in r],
                                 [x is True for x in want])
        # Errors raised by the operands propagate from intrinsic sites
        for a, b, exc in ((Rich2(), Rich2(), ZeroDivisionError),
                          (1, Rich2(), ZeroDivisionError)):
            with self.assertRaises(exc):
                f(a, b)

    @support.cpython_only
    def test_no_leak(self):
        f = make_function(FeedbackLayoutTest.compare_source(
            FeedbackLayoutTest, 70))
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD * 2):
            f(1, AllOps())
        if not hasattr(sys, "gettotalrefcount"):
            return
        before = sys.gettotalrefcount()
        for _ in range(200):
            f(1, AllOps())
            with self.assertRaises(TypeError):
                f(1, 2)
        self.assertLess(sys.gettotalrefcount() - before, 100)

    def test_delete_attr_at_a_lowered_site(self):
        n = 5000
        class O:
            def __delattr__(self, name):
                raise RuntimeError(name)
        lines = ["def f(o):"] + [f"    o.n{i}" for i in range(n)]
        lines.append(f"    del o.n{n - 1}")
        lines.append("    return 1")
        f = make_function("\n".join(lines) + "\n")
        o = types.SimpleNamespace(**{f"n{i}": i for i in range(n)})
        self.assertEqual(f(o), 1)
        self.assertFalse(hasattr(o, f"n{n - 1}"))
        class P(O):
            def __getattr__(self, name):
                return 0
        with self.assertRaisesRegex(RuntimeError, f"n{n - 1}"):
            f(P())
        # A method call by a name past 1,024 in a function that declines
        # the super optimisation
        lines = ["class B:"] + [f"    def m{i}(self): return {i}" for i in range(1100)]
        lines += ["class C(B):", "    def m1099(self):",
                  "        return super().m1099() + super().m0()"]
        ns = {}
        exec("\n".join(lines) + "\n", ns)
        self.assertEqual(ns["C"]().m1099(), 1099)


class NameLimitTest(unittest.TestCase):
    """A name index that does not fit the field of the word is lowered by
    the compiler to an intrinsic call (Python/codegen.c)"""

    NAMES = 5000

    def make(self, body_lines, extra_args=""):
        lines = [f"def f(o{extra_args}):", "    t = 0"] + body_lines + ["    return t"]
        return make_function("\n".join(lines) + "\n")

    def test_load_attr(self):
        n = self.NAMES
        ns = types.SimpleNamespace(**{f"n{i}": i for i in range(n)})
        f = self.make([f"    t += o.n{i}" for i in range(n)])
        self.assertEqual(len(f.__code__.co_names), n)
        instrs = list(dis.get_instructions(f))
        loads = [i for i in instrs if i.opname == "LOAD_ATTR"]
        gets = [i for i in instrs if i.opname == "CALL_INTRINSIC_2"
                and i.argrepr == "INTRINSIC_GETATTR"]
        # oparg = index << 1 must fit 12 bits: 2,048 names
        self.assertEqual(len(loads), 2048)
        self.assertEqual(len(gets), n - 2048)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD * 2):
            self.assertEqual(f(ns), sum(range(n)))
        with self.assertRaises(AttributeError):
            f(types.SimpleNamespace())
        ns.n4000 = None
        with self.assertRaises(TypeError):
            f(ns)

    def test_method_call_store_delete_augassign(self):
        n = 2200
        lines = []
        for i in range(n):
            lines.append(f"    t += o.m{i}()")
        for i in range(n):
            lines.append(f"    o.s{i} = {i}")
            lines.append(f"    t += o.s{i}")
            lines.append(f"    o.s{i} += 1")
            lines.append(f"    del o.s{i}")
        f = self.make(lines)
        ns = types.SimpleNamespace(**{f"m{i}": (lambda i=i: i) for i in range(n)})
        want = sum(range(n)) + sum(range(n))
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD * 2):
            self.assertEqual(f(ns), want)
            self.assertEqual(vars(ns).keys(), {f"m{i}" for i in range(n)})
        instrs = list(dis.get_instructions(f))
        self.assertIn("INTRINSIC_SETATTR",
                      {i.argrepr for i in instrs if i.opname == "CALL_INTRINSIC_2"})
        # A store to an object that refuses it raises from the lowered site
        class Frozen:
            def __getattr__(self, name):
                return lambda: 0
            def __setattr__(self, name, value):
                raise AttributeError(name)
        with self.assertRaises(AttributeError):
            f(Frozen())

    def test_super_attr(self):
        n = 1500
        ns = {}
        lines = ["class Base:"]
        lines += [f"    def m{i}(self): return {i}" for i in range(n)]
        lines += ["class D(Base):", "    def f(self):", "        t = 0"]
        lines += [f"        t += super().m{i}()" for i in range(n)]
        lines += ["        return t"]
        exec("\n".join(lines) + "\n", ns)
        d = ns["D"]()
        code = ns["D"].f.__code__
        names = [i.opname for i in dis.get_instructions(code)]
        self.assertIn("LOAD_SUPER_ATTR", names)
        self.assertGreater(names.count("LOAD_SUPER_ATTR"), 1000)
        self.assertLess(names.count("LOAD_SUPER_ATTR"), n)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD * 2):
            self.assertEqual(d.f(), sum(range(n)))

    def test_wide_names_survive_marshal(self):
        import marshal
        n = self.NAMES
        ns = types.SimpleNamespace(**{f"n{i}": i for i in range(n)})
        f = self.make([f"    t += o.n{i}" for i in range(n)])
        g = types.FunctionType(marshal.loads(marshal.dumps(f.__code__)), {})
        self.assertEqual(g(ns), sum(range(n)))


class RelaxationTest(unittest.TestCase):
    """FOR_ITER and SEND reach IR_MAX_FORWARD words; a loop whose exit is
    further away exits through IR_JUMP words after its own."""

    def relaxed(self, func, opname):
        # The index of the first opname word of func and the handlers of
        # the IR_JUMP words after it
        ir = get_ir(func.__code__)
        index = [k for k, word in enumerate(ir) if word[0] == opname][0]
        jumps = []
        for word in ir[index + 1:]:
            if word[0] != "IR_JUMP":
                break
            jumps.append(word[0])
        return index, jumps

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
        index, jumps = self.relaxed(f, "FOR_ITER")
        self.assertEqual(jumps, ["IR_JUMP"] * 3)
        # The loop exits by distance 1, to the second jump
        self.assertEqual(list(dis.get_instructions(f))[index].arg, 1)
        AssemblerTest.check_offsets(self, f)

        # A generator returns to the second jump
        f = make_function(source)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 1):
            self.assertEqual(f(gen(4)), 6 * nlines)
        if SPECIALIZES:
            self.assertEqual(handlers(f)[index], "FOR_ITER_GEN")

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
        namespace = relaxed_compile(source)
        for name in ("outer", "co"):
            _, jumps = self.relaxed(namespace[name], "SEND")
            self.assertEqual(jumps, ["IR_JUMP"] * 2)
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
        relaxed = relaxed_compile(source)["f"]
        plain = make_function(source)
        self.assertEqual(self.relaxed(relaxed, "FOR_ITER")[1], ["IR_JUMP"] * 3)
        self.assertEqual(self.relaxed(plain, "FOR_ITER")[1], [])

        def canonical(func, offset):
            # The index of the word at offset among the words that are not
            # IR_JUMP
            ir = get_ir(func.__code__)
            self.assertNotEqual(ir[offset][0], "IR_JUMP")
            return offset - sum(word[0] == "IR_JUMP" for word in ir[:offset])

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
                            events.append((event, canonical(func, src),
                                           canonical(func, dst))))
                sys.monitoring.set_local_events(
                    tool, func.__code__,
                    E.BRANCH_LEFT | E.BRANCH_RIGHT | E.JUMP)
                self.assertEqual(func([0, 1, 1]), 2)
            finally:
                sys.monitoring.set_local_events(tool, func.__code__, 0)
                sys.monitoring.free_tool_id(tool)
            return events

        def opcodes(func):
            result, events = trace(func, [0, 1, 1], opcodes=True)
            return result, [(event, canonical(func, lasti), line)
                            for event, lasti, line in events]

        self.assertEqual(branches(relaxed), branches(plain))
        self.assertEqual(opcodes(relaxed), opcodes(plain))



def words(func):
    """The handler and slot fields of each word of func, RESUME excluded:
    (handler, dst or None, srcs)"""
    return [(instr.handler, instr.dst, instr.srcs)
            for instr in dis.get_instructions(func)
            if instr.opname != "RESUME"]


class FoldTest(unittest.TestCase):
    """Loads folded into the next word (Python/regcfg_passes.c)"""

    def test_attribute_sum(self):
        # boil/design/06-architecture-v2.md, section 2.2
        def g(o):
            return o.x + o.y
        t = nlocalsplus(g)
        self.assertEqual(words(g), [
            ("LOAD_ATTR", t, (0,)),
            ("LOAD_ATTR", t + 1, (0,)),
            ("BINARY_OP_ADD", t, (t, t + 1)),
            ("RETURN_VALUE", None, (t,)),
        ])
        o = types.SimpleNamespace(x=2, y=3)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 1):
            self.assertEqual(g(o), 5)

    @support.requires_refcounting
    def test_return_local(self):
        # RETURN_VALUE takes a new reference to the local before the frame
        # releases its locals
        def f(x):
            return x
        def g():
            y = [1, 2]
            return y
        self.assertEqual(words(f), [("RETURN_VALUE", None, (0,))])
        self.assertEqual(words(g)[-1], ("RETURN_VALUE", None, (0,)))
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 1):
            o = object()
            before = sys.getrefcount(o)
            self.assertIs(f(o), o)
            self.assertEqual(sys.getrefcount(o), before)
            self.assertEqual(g(), [1, 2])

    def test_load_below_a_constant_folds(self):
        # Nothing between the load of x and the add writes x
        def f(x):
            return x + 1
        self.assertNotIn("LOAD_FAST_BORROW", [w[0] for w in words(f)])
        self.assertEqual(f(1), 2)

    def test_store_to_the_loaded_local(self):
        def inc(x):
            x = x + 1
            return x
        def swap(a, b):
            a, b = b, a
            return a, b
        def walrus(x):
            def f(p, q):
                return (p, q)
            def g():
                return 7
            x = f(x, x := g())
            return x
        def loop(n):
            x = 0
            out = []
            for i in range(n):
                y = x
                x = i
                out.append((y, x))
            return out
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 1):
            self.assertEqual(inc(1), 2)
            self.assertEqual(swap(1, 2), (2, 1))
            self.assertEqual(walrus(3), (3, 7))
            self.assertEqual(loop(3), [(0, 0), (0, 1), (1, 2)])

    def test_augmented_assignment(self):
        def attr(o):
            o.a += 1
            return o.a
        def item(s, i):
            s[i] += 1
            return s[i]
        o = types.SimpleNamespace(a=1)
        s = [1, 2]
        for k in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 1):
            self.assertEqual(attr(o), k + 2)
            self.assertEqual(item(s, 1), k + 3)

    def test_raise_in_the_first_word_of_a_try(self):
        # The subscript is the first word of the try block: the loads of d
        # and key before it have none.  A Python __getitem__ runs in a frame
        # of its own once BINARY_OP specialises.
        class M:
            def __getitem__(self, k):
                raise KeyError(k)
        def pop(d, key):
            try:
                return d[key]
            except KeyError:
                return None
        m = M()
        for i in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 2):
            self.assertIsNone(pop(m, i))

    def test_trace_sees_no_folded_value(self):
        # A line event before the word that a load folds into: the frame's
        # stack does not hold the value yet.
        def f(o):
            return o.x
        lines = []
        def trace(frame, event, arg):
            if frame.f_code is f.__code__ and event == "line":
                lines.append(frame.f_lineno)
            return trace
        o = types.SimpleNamespace(x=1)
        sys.settrace(trace)
        try:
            self.assertEqual(f(o), 1)
        finally:
            sys.settrace(None)
        self.assertEqual(lines, [f.__code__.co_firstlineno + 1])


class Truthy:
    pass


class Falsy:
    def __bool__(self):
        return False


class TruthTest(unittest.TestCase):
    """Conditional jumps test the truth of any value (Python/flowgraph.c)"""

    def test_no_to_bool_before_a_jump(self):
        def f(x):
            if x:
                return 1
            return 2
        def g(x):
            if not x:
                return 1
            return 2
        self.assertEqual(words(f)[0], ("POP_JUMP_IF_FALSE", None, (0,)))
        self.assertEqual(words(g)[0], ("POP_JUMP_IF_TRUE", None, (0,)))
        for func in f, g:
            names = [instr.opname for instr in dis.get_instructions(func)]
            self.assertNotIn("TO_BOOL", names)
            self.assertNotIn("UNARY_NOT", names)

    def test_copy_of_a_local_is_a_load(self):
        # The second load folds into the jump, so no COPY is left
        def f(x, y):
            return x and y
        def g(o):
            o.a += 1
        for func in f, g:
            names = [instr.opname for instr in dis.get_instructions(func)]
            self.assertNotIn("COPY", names)
        o = types.SimpleNamespace(a=1)
        for k in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 1):
            self.assertEqual(f(0, 5), 0)
            self.assertEqual(f([1], 5), 5)
            g(o)
        self.assertEqual(o.a, _testinternalcapi.SPECIALIZATION_THRESHOLD + 2)

    @unittest.skipUnless(SPECIALIZES, "requires specialisation")
    def test_specialisations(self):
        source = ("def f(x):\n"
                  "    if x:\n"
                  "        return 1\n"
                  "    return 2\n"
                  "def g(x):\n"
                  "    if not x:\n"
                  "        return 1\n"
                  "    return 2\n")
        cases = (("BOOL", True, False), ("INT", 3, 0),
                 ("INT", 2**40, 0), ("LIST", [0], []),
                 ("NONE", None, None), ("STR", "a", ""),
                 ("ALWAYS_TRUE", Truthy(), Truthy()))
        for kind, true, false in cases:
            with self.subTest(kind=kind, true=true):
                f = make_function(source)
                g = make_function(source, "g")
                for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 1):
                    self.assertEqual(f(true), 1 if true else 2)
                    self.assertEqual(f(false), 1 if false else 2)
                    self.assertEqual(g(true), 2 if true else 1)
                    self.assertEqual(g(false), 2 if false else 1)
                self.assertIn(f"POP_JUMP_IF_FALSE_{kind}", handlers(f))
                self.assertIn(f"POP_JUMP_IF_TRUE_{kind}", handlers(g))
                # Another type deoptimises
                for value in (Falsy(), 0.0, (), 1.5, (1,)):
                    self.assertEqual(f(value), 1 if value else 2)
                    self.assertEqual(g(value), 2 if value else 1)

    def test_always_true_after_the_type_changes(self):
        class T:
            pass
        def f(x):
            return 1 if x else 2
        t = T()
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 1):
            self.assertEqual(f(t), 1)
        T.__bool__ = lambda self: False
        self.assertEqual(f(t), 2)
        T.__len__ = lambda self: 1
        del T.__bool__
        self.assertEqual(f(t), 1)

    def test_errors_of_the_truth_test(self):
        class Raises:
            def __bool__(self):
                raise ZeroDivisionError
        class NotBool:
            def __bool__(self):
                return 1
        def f(x):
            if x:
                return 1
            return 2
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 1):
            self.assertEqual(f(1), 1)
        try:
            f(Raises())
        except ZeroDivisionError as exc:
            tb = exc.__traceback__.tb_next
        else:
            self.fail("no exception")
        self.assertIs(tb.tb_frame.f_code, f.__code__)
        self.assertEqual(tb.tb_lineno, f.__code__.co_firstlineno + 1)
        with self.assertRaises(TypeError):
            f(NotBool())

    def test_branch_events_of_any_value(self):
        # The instrumented jumps test truth too
        def f(xs):
            n = 0
            for x in xs:
                if x:
                    n += 1
            return n

        xs = [0, 1, None, "a", [], [0], Truthy(), Falsy()] * 10
        jump = [instr for instr in dis.get_instructions(f)
                if instr.opname.startswith("POP_JUMP_IF_")][0]
        if_true = jump.opname == "POP_JUMP_IF_TRUE"
        expected = ["right" if bool(x) == if_true else "left" for x in xs]
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
            self.assertEqual(f(xs), 40)
        finally:
            sys.monitoring.set_local_events(tool, f.__code__, 0)
            sys.monitoring.free_tool_id(tool)
        self.assertEqual(events, expected)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 1):
            self.assertEqual(f(xs), 40)


class StoreFoldTest(unittest.TestCase):
    """Stores folded into the word before them (Python/regcfg_passes.c)"""

    def run_specialized(self, check):
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 2):
            check()

    def test_sum_to_local(self):
        def f(a, b):
            x = a + b
            return x
        t = nlocalsplus(f)
        self.assertEqual(words(f), [
            ("BINARY_OP_ADD", 2, (0, 1)),
            ("RETURN_VALUE", None, (2,)),
        ])
        self.assertEqual(t, 3)
        self.run_specialized(lambda: self.assertEqual(f(1, 2), 3))
        self.run_specialized(lambda: self.assertEqual(f("a", "b"), "ab"))

    def test_copy_before_the_store_to_its_source(self):
        # The store to y is folded into the load of x; the later store to x
        # must not change y.
        def f(x):
            y = x
            x = x + 1
            return y
        self.assertNotIn("STORE_FAST", [w[0] for w in words(f)])
        self.run_specialized(lambda: self.assertEqual(f(1), 1))

    def test_loop_index(self):
        def f(a, b):
            total = 0.0
            for i in range(len(a)):
                total += a[i] * b[i]
            return total
        handlers = [w[0] for w in words(f)]
        self.assertNotIn("STORE_FAST", handlers)
        self.run_specialized(
            lambda: self.assertEqual(f([1.0, 2.0], [3.0, 4.0]), 11.0))

    def test_raise_keeps_the_old_value(self):
        def g(z):
            raise ValueError(z)
        def f(z):
            try:
                z = g(z)
            except ValueError:
                pass
            return z
        def h(z):
            try:
                z = z + "x"
            except TypeError:
                pass
            return z
        self.run_specialized(lambda: self.assertEqual(f(5), 5))
        self.run_specialized(lambda: self.assertEqual(h(5), 5))

    def test_old_value_released_once(self):
        class C:
            freed = 0
            def __del__(self):
                C.freed += 1
            def __add__(self, other):
                return C()
        def f(x):
            x = x + 1
            x = x + 1
            return None
        self.run_specialized(lambda: f(C()))
        n = _testinternalcapi.SPECIALIZATION_THRESHOLD + 2
        support.gc_collect()  # For other GCs.
        self.assertEqual(C.freed, 3 * n)

    @support.requires_refcounting
    def test_release_runs_with_the_frame_consistent(self):
        # __del__ of the old value runs as the store releases it, and sees
        # the new value in the local.
        seen = []
        class D:
            def __del__(self):
                seen.append(sys._getframe(1).f_locals.get("x"))
        def f():
            x = D()
            x = 2 + 3
            return x
        self.assertEqual(f(), 5)
        self.assertEqual(seen, [5])

    def test_generator(self):
        def gen(n):
            for i in range(n):
                j = i * 2
                yield j
                del j
                j = i
                yield j
        self.run_specialized(
            lambda: self.assertEqual(list(gen(2)), [0, 0, 2, 1]))

    def test_delete_and_store(self):
        def f(a):
            x = a + 1
            del x
            try:
                return x
            except UnboundLocalError:
                x = a + 2
            return x
        self.run_specialized(lambda: self.assertEqual(f(1), 3))

    def test_inplace_unicode_add(self):
        def f(n):
            s = ""
            for i in range(n):
                s += "ab"
            return s
        self.run_specialized(lambda: self.assertEqual(f(3), "ababab"))

    def test_set_lineno_after_folded_store(self):
        # The frame-state simulation (frameobject.c) pops the result of a
        # word that stores it.
        def f():
            x = 1 + len("a")
            y = x + 1
            return y
        lines = []
        def trace(frame, event, arg):
            if frame.f_code is f.__code__ and event == "line":
                lines.append(frame.f_lineno)
                if frame.f_lineno == f.__code__.co_firstlineno + 2 and len(lines) == 2:
                    frame.f_lineno = f.__code__.co_firstlineno + 1
            return trace
        sys.settrace(trace)
        try:
            self.assertEqual(f(), 3)
        finally:
            sys.settrace(None)


K = 3


def one():
    return 1


class RegCfgTest(unittest.TestCase):
    """The passes over the register CFG (Python/regcfg_passes.c) and its
    compaction (Python/regcfg.c)"""

    def run_specialized(self, check):
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 2):
            check()

    def names(self, func):
        return [w[0] for w in words(func)]

    # a. Copy propagation

    def test_load_does_not_fold_under_a_global_or_a_call(self):
        # A generic LOAD_GLOBAL and a call may run Python code, which may
        # rebind x through frame.f_locals, so the load of x keeps its word
        # and the reader takes its value from a temporary
        # (CopyPropagationAcrossCodeTests in test_peepholer).
        def mul(x):
            return x * K
        def add(x):
            return x + one()
        for f in mul, add:
            self.assertIn("LOAD_FAST_BORROW", self.names(f))
            op = [w for w in words(f) if w[0].startswith("BINARY_OP")][0]
            self.assertNotEqual(op[2][0], 0)
        self.run_specialized(lambda: self.assertEqual(mul(2), 6))
        self.run_specialized(lambda: self.assertEqual(add(2), 3))

    def test_redefinition_kills_the_copy(self):
        def copy(x):
            y = x
            x = x + 1
            return y
        def walrus(x):
            return x + (x := x + 1)
        # The first x of walrus is read before the store: it stays a load
        self.assertIn("LOAD_FAST", self.names(walrus))
        self.run_specialized(lambda: self.assertEqual(copy(1), 1))
        self.run_specialized(lambda: self.assertEqual(walrus(1), 3))

    # b. Store elimination

    def test_store_elimination(self):
        def f(a, b):
            x = a + b
            return x
        self.assertEqual(words(f), [
            ("BINARY_OP_ADD", 2, (0, 1)),
            ("RETURN_VALUE", None, (2,)),
        ])
        self.run_specialized(lambda: self.assertEqual(f(1, 2), 3))

    def test_raising_producer_keeps_the_old_value(self):
        def fails(z):
            raise ValueError
        def call(z):
            try:
                z = fails(z)
            except ValueError:
                pass
            return z
        def add(z):
            try:
                z = z + 1
            except TypeError:
                pass
            return z
        self.run_specialized(lambda: self.assertEqual(call(5), 5))
        self.run_specialized(lambda: self.assertEqual(add("a"), "a"))

    def test_call_result_to_a_local(self):
        # The call's dst names the local: a Python callee's return, or the
        # C call itself, releases the local's old value
        class Counted:
            freed = 0
            def __del__(self):
                Counted.freed += 1
        def py(v):
            return v
        def f():
            x = Counted()
            x = py(1)
            y = Counted()
            y = len("ab")
            return x, y
        def fails(z):
            raise ValueError
        def keeps(z):
            try:
                z = fails(z)
            except ValueError:
                pass
            return z
        self.assertNotIn("STORE_FAST", self.names(f))
        call = [w for w in words(f) if w[0].startswith("CALL")][1]
        self.assertEqual(call[1], 0)
        n = _testinternalcapi.SPECIALIZATION_THRESHOLD + 2
        for _ in range(n):
            self.assertEqual(f(), (1, 2))
            self.assertEqual(keeps(5), 5)
        support.gc_collect()  # For other GCs.
        self.assertEqual(Counted.freed, 2 * n)

    def test_pushed_frame_result_to_a_local(self):
        # LOAD_ATTR_PROPERTY and BINARY_OP_SUBSCR_GETITEM push the frame of
        # the getter, which returns to the local
        class C:
            @property
            def p(self):
                return 7
            def __getitem__(self, i):
                return i + 1
        def f(o):
            x = o.p
            y = o[1]
            return x, y
        self.run_specialized(lambda: self.assertEqual(f(C()), (7, 2)))

    def test_generator_keeps_the_loop_variable(self):
        # A generator's frame hands the slot of the loop variable each value
        # it yields, and at the end its return value; the loop variable
        # keeps the last value yielded
        def gen():
            yield 1
            yield 2
            return 3
        def f():
            for x in gen():
                pass
            return x
        self.run_specialized(lambda: self.assertEqual(f(), 2))

    # c. COPY, SWAP and POP_TOP

    def test_swap_renaming(self):
        def f(a, b):
            a, b = b, a
            return a - b
        self.assertNotIn("SWAP", self.names(f))
        self.run_specialized(lambda: self.assertEqual(f(1, 3), 2))

    def test_pop_top_of_a_pure_value(self):
        def f(x):
            x
            1
            return x
        self.assertNotIn("POP_TOP", self.names(f))
        # Each line keeps its event
        first = f.__code__.co_firstlineno
        _, events = trace(f, 0)
        self.assertEqual([e[2] - first for e in events if e[0] == "line"],
                         [1, 2, 3])

    def test_pop_top_of_a_call_stays(self):
        def f(g):
            g()
        self.assertIn("POP_TOP", self.names(f))

    # d. Truth folding in value context

    def test_truth_folding(self):
        def conj(o, b):
            return o.x and b
        def disj(o, b):
            return o.x or b
        self.assertIn("JUMP_IF_FALSE_OR_POP", self.names(conj))
        self.assertIn("JUMP_IF_TRUE_OR_POP", self.names(disj))
        for f in conj, disj:
            self.assertNotIn("COPY", self.names(f))
            self.assertNotIn("POP_TOP", self.names(f))
        empty, full = [], [1]
        for x, b in ((empty, "b"), (full, "b"), (0, None), (2.5, 0)):
            o = types.SimpleNamespace(x=x)
            self.assertIs(conj(o, b), x and b)
            self.assertIs(disj(o, b), x or b)

        class Raises:
            def __bool__(self):
                raise ZeroDivisionError
        o = types.SimpleNamespace(x=Raises())
        for f in conj, disj:
            with self.assertRaises(ZeroDivisionError):
                f(o, 1)

    def test_truth_folding_of_a_local(self):
        # The flowgraph turns the COPY of a local into a second load, which
        # folds into the jump
        def disj(a, b):
            return a or b
        def window(a, b):
            return list((a or b(), 1))
        self.assertIn("JUMP_IF_TRUE_OR_POP", self.names(disj))
        self.assertNotIn("POP_TOP", self.names(disj))
        empty, full = [], [1]
        for a in empty, full, 0, 2.5:
            self.assertIs(disj(a, full), a or full)
            self.assertEqual(window(a, lambda: full), [a or full, 1])

    # f. Compaction

    def test_compaction(self):
        # The stack is three deep at c + d, but a and b fold into the first
        # add and c and d into the second, so two temporaries are live
        def f(a, b, c, d):
            return (a + b) * (c + d)
        t = nlocalsplus(f)
        self.assertEqual(f.__code__.co_stacksize, 2)
        self.assertEqual(words(f), [
            ("BINARY_OP_ADD", t, (0, 1)),
            ("BINARY_OP_ADD", t + 1, (2, 3)),
            ("BINARY_OP_MULTIPLY", t, (t, t + 1)),
            ("RETURN_VALUE", None, (t,)),
        ])
        self.run_specialized(lambda: self.assertEqual(f(1, 2, 3, 4), 21))

    def test_frame_holds_the_handler_depth(self):
        # The unwinder puts the exception above the loop's iterator, at the
        # handler's depth, where no word of the try body writes
        def f(xs):
            for x in xs:
                try:
                    return 1 / x
                except ZeroDivisionError as e:
                    err = e
            return err
        self.assertEqual(f([0, 2]), 0.5)
        self.assertIsInstance(f([0]), ZeroDivisionError)

FUSED = {"COMPARE_OP_JUMP", "CONTAINS_OP_JUMP", "IS_OP_JUMP"}


def fused_names(func):
    """The fused handlers of the executing copy of func's code, with the
    specialisations folded to their family"""
    return [h for h in handlers(func)
            if h.startswith(("COMPARE_OP_JUMP", "CONTAINS_OP_JUMP",
                             "IS_OP_JUMP"))]


class FusedJumpTest(AssemblerTestCase):
    """A compare, contains or is test and the forward jump that consumes its
    bool are one word (register-ir-v2 section 4.3, step B6)."""

    def test_pairs_fuse(self):
        sources = {
            "COMPARE_OP_JUMP": "a < b",
            "CONTAINS_OP_JUMP": "a in b",
            "IS_OP_JUMP": "a is b",
        }
        for name, test in sources.items():
            for negated in ("", "not "):
                with self.subTest(test=test, negated=negated):
                    f = make_function(
                        f"def f(a, b):\n    if {negated}({test}):\n"
                        f"        return 1\n    return 2\n")
                    ir = handlers(f)
                    self.assertEqual(ir.count(name), 1)
                    self.assertFalse([h for h in ir
                                      if h.startswith("POP_JUMP")])
                    # The family's instruction is a compare then a jump
                    self.assertEqual(
                        [i.opname for i in dis.get_instructions(f)
                         if i.opname.startswith("POP_JUMP")], [])

    def test_instrumentation_sees_a_jump(self):
        f = make_function("def f(a, b):\n    if a < b:\n        return 1\n"
                          "    return 2\n")
        self.assertEqual(fused_names(f), ["COMPARE_OP_JUMP"])
        E = sys.monitoring.events
        tool = sys.monitoring.DEBUGGER_ID
        events = []
        sys.monitoring.use_tool_id(tool, "test_ir")
        try:
            for event, name in ((E.BRANCH_LEFT, "left"),
                                (E.BRANCH_RIGHT, "right")):
                sys.monitoring.register_callback(
                    tool, event,
                    lambda code, src, dst, name=name: events.append(name))
            sys.monitoring.set_local_events(tool, f.__code__,
                                            E.BRANCH_LEFT | E.BRANCH_RIGHT)
            self.assertEqual(f(1, 2), 1)
            self.assertEqual(f(2, 1), 2)
            self.assertEqual(f(1, 2), 1)
            self.assertIn("INSTRUMENTED_COMPARE_OP_JUMP", handlers(f))
        finally:
            sys.monitoring.set_local_events(tool, f.__code__, 0)
            sys.monitoring.free_tool_id(tool)
        # "if a < b" jumps over the body when the test is false
        self.assertEqual(events, ["left", "right", "left"])
        self.assertEqual(f(2, 1), 2)

    def test_pair_on_two_lines_stays_unfused(self):
        # The jump keeps its own line, so that a line event for it, and
        # the position of an error, stay what the source says
        for lines, fused in (((1, 1), True), ((1, 2), False)):
            with self.subTest(lines=lines):
                end = self.Label()
                insts = [
                    ("RESUME", 0),
                    ("LOAD_FAST", 0, 1),
                    ("LOAD_FAST", 1, 1),
                    ("COMPARE_OP", 18, lines[0]),
                    ("POP_JUMP_IF_FALSE", end, lines[1]),
                    ("LOAD_SMALL_INT", 1, 3),
                    ("RETURN_VALUE", None, 3),
                    end,
                    ("LOAD_SMALL_INT", 2, 4),
                    ("RETURN_VALUE", None, 4),
                ]
                metadata = {"argcount": 2, "varnames": {"a": 0, "b": 1}}
                co = self.insts_to_code_object(insts, metadata)
                ir = [w[0] for w in get_ir(co, True)]
                self.assertEqual("COMPARE_OP_JUMP" in ir, fused)
                self.assertEqual("POP_JUMP_IF_FALSE" in ir, not fused)
                f = types.FunctionType(co, {})
                self.assertEqual(f(1, 2), 1)
                self.assertEqual(f(2, 1), 2)

    def complete_metadata(self, metadata):
        for key in ("name", "qualname"):
            metadata.setdefault(key, key)
        metadata.setdefault("consts", {})
        for key in ("names", "varnames", "cellvars", "freevars"):
            metadata.setdefault(key, {})
        metadata.setdefault("fasthidden", None)
        for key in ("argcount", "posonlyargcount", "kwonlyargcount"):
            metadata.setdefault(key, 0)
        metadata.setdefault("firstlineno", 1)
        metadata.setdefault("filename", "fused.py")
        return metadata

    def insts_to_code_object(self, insts, metadata):
        metadata = self.complete_metadata(metadata)
        seq = self.seq_from_insts(insts)
        return self.get_code_object(metadata["filename"], seq, metadata)

    def test_far_target_stays_unfused(self):
        # The distance of a fused word has 11 bits and a sign; a jump that
        # does not fit is a compare and a jump of its own
        source = ("def f(a, b):\n    if a < b:\n        a = a + b\n"
                  "        b = b + a\n    return a, b\n")
        old = _testinternalcapi.set_ir_max_forward(0)
        try:
            far = make_function(source)
        finally:
            _testinternalcapi.set_ir_max_forward(old)
        near = make_function(source)
        self.assertEqual(fused_names(far), [])
        self.assertEqual(fused_names(near), ["COMPARE_OP_JUMP"])
        for a, b in ((1, 2), (2, 1), (2, 2)):
            self.assertEqual(far(a, b), near(a, b))

    def run_cases(self, test, cases):
        """test is the source of an if on a and b; its result, with and
        without the jump, is the compare itself"""
        f = make_function(
            f"def f(a, b):\n    if {test}:\n        return 1\n    return 0\n")
        g = make_function(f"def g(a, b):\n    return bool({test})\n", "g")
        self.assertEqual(len(fused_names(f)), 1, test)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            for a, b in cases:
                self.assertEqual(f(a, b), int(g(a, b)), (test, a, b))
        return f

    def test_comparisons(self):
        nan = float("nan")
        families = {
            "COMPARE_OP_JUMP_INT": [(1, 2), (2, 2), (3, 2)],
            "COMPARE_OP_JUMP_FLOAT": [(1.5, 2.5), (2.5, 2.5), (nan, 1.0),
                                      (1.0, nan), (nan, nan)],
            "COMPARE_OP_JUMP_STR": [("a", "b"), ("b", "b"), ("c", "b")],
        }
        for op in ("<", "<=", ">", ">=", "==", "!="):
            for negated in ("", "not "):
                for family, cases in families.items():
                    if family.endswith("STR") and op not in ("==", "!="):
                        continue
                    test = f"{negated}(a {op} b)"
                    with self.subTest(test=test, family=family):
                        f = self.run_cases(test, cases)
                        if SPECIALIZES:
                            self.assertEqual(fused_names(f), [family])
                        # Operands of another type deoptimise the site
                        for a, b in ((None, None), (1, 2.5), (1.5, 2),
                                     ("a", "b")):
                            try:
                                expected = int(eval(test, {"a": a, "b": b}))
                            except TypeError:
                                with self.assertRaises(TypeError):
                                    f(a, b)
                            else:
                                self.assertEqual(f(a, b), expected)
                        for a, b in cases:
                            self.assertEqual(
                                f(a, b), int(eval(test, {"a": a, "b": b})))

    def test_contains_and_is(self):
        pool = [1, 2, 3]
        cases = {
            "a in b": [(1, {1, 2}), (5, {1, 2}), (1, {1: 2}), (5, {1: 2}),
                       (1, pool), (5, pool), ("a", "abc"), ("z", "abc")],
            "a not in b": [(1, {1, 2}), (5, {1, 2}), (1, {1: 2}), (5, {1: 2}),
                           (1, pool), (5, pool), ("a", "abc"), ("z", "abc")],
            "a is b": [(None, None), (1, None), (pool, pool), (pool, [1])],
            "a is not b": [(None, None), (1, None), (pool, pool), (pool, [1])],
        }
        for test, args in cases.items():
            with self.subTest(test=test):
                for sense in ("", "not "):
                    f = self.run_cases(f"{sense}({test})", args)
                    for a, b in args:
                        self.assertEqual(
                            f(a, b), int(eval(f"{sense}({test})",
                                              {"a": a, "b": b})))

    def test_specialised_sites_of_sets_and_dicts(self):
        f = make_function("def f(a, b):\n    if a in b:\n        return 1\n"
                          "    return 0\n")
        for container in ({1, 2}, {1: 2}):
            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
                self.assertEqual(f(1, container), 1)
                self.assertEqual(f(3, container), 0)
            if SPECIALIZES:
                self.assertEqual(len(fused_names(f)), 1)
        # An unhashable operand is the TypeError of the generic form
        with self.assertRaises(TypeError):
            f([], {1})
        self.assertEqual(f(1, [1]), 1)

    def test_errors_keep_their_line(self):
        f = make_function("def f(a, b):\n    x = 1\n    if a < b:\n"
                          "        return 1\n    return 0\n")
        try:
            f(1, None)
        except TypeError as exc:
            # assertRaises would drop the traceback
            self.assertEqual(exc.__traceback__.tb_next.tb_lineno, 3)
        else:
            self.fail("no TypeError")

    def test_line_events(self):
        f = make_function("def f(a, b):\n    if a < b:\n        return 1\n"
                          "    return 0\n")
        result, events = trace(f, 1, 2)
        self.assertEqual(result, 1)
        first = f.__code__.co_firstlineno
        self.assertEqual([lineno - first for event, _, lineno in events
                          if event == "line"], [1, 2])
        result, events = trace(f, 2, 1)
        self.assertEqual(result, 0)
        self.assertEqual([lineno - first for event, _, lineno in events
                          if event == "line"], [1, 3])

    def test_set_lineno_through_a_fused_word(self):
        f = make_function("def f(a, b):\n    if a < b:\n        a = 10\n"
                          "    return a\n")
        first = f.__code__.co_firstlineno
        seen = []

        def tracer(frame, event, arg):
            if frame.f_code is f.__code__ and event == "line":
                seen.append(frame.f_lineno - first)
                if frame.f_lineno - first == 1:
                    # Skip the if, and the body with it
                    frame.f_lineno = first + 3
            return tracer

        sys.settrace(tracer)
        try:
            self.assertEqual(f(1, 2), 1)
        finally:
            sys.settrace(None)
        # The body, which would make it 10, did not run
        self.assertEqual(seen[0], 1)

    def test_compare_without_the_bool_bit_stays_unfused(self):
        # Oparg 2 is "<" without the bit that says the result is a bool:
        # the jump tests the truth of an arbitrary result, so it is a jump
        # of its own.  Oparg 18 is the same test with the bit.
        class Odd:
            def __lt__(self, other):
                return []

        for oparg, fused in ((2, False), (18, True)):
            with self.subTest(oparg=oparg):
                end = self.Label()
                insts = [
                    ("RESUME", 0),
                    ("LOAD_FAST", 0, 1),
                    ("LOAD_FAST", 1, 1),
                    ("COMPARE_OP", oparg, 1),
                    ("POP_JUMP_IF_FALSE", end, 1),
                    ("LOAD_SMALL_INT", 1, 3),
                    ("RETURN_VALUE", None, 3),
                    end,
                    ("LOAD_SMALL_INT", 2, 4),
                    ("RETURN_VALUE", None, 4),
                ]
                co = self.insts_to_code_object(
                    insts, {"argcount": 2, "varnames": {"a": 0, "b": 1}})
                ir = [w[0] for w in get_ir(co, True)]
                self.assertEqual("COMPARE_OP_JUMP" in ir, fused)
                if not fused:
                    f = types.FunctionType(co, {})
                    # [] is false: the jump is taken
                    self.assertEqual(f(Odd(), 1), 2)

    def test_result_read_again_stays_unfused(self):
        f = make_function("def f(a, b):\n    x = a < b\n    if x:\n"
                          "        return x\n    return x\n")
        self.assertEqual(fused_names(f), [])
        self.assertIs(f(1, 2), True)
        self.assertIs(f(2, 1), False)
        # The jump uses the value that is also returned
        g = make_function("def g(a, b):\n    if (x := a in b):\n"
                          "        return x\n    return not x\n", "g")
        self.assertEqual(fused_names(g), [])
        self.assertIs(g(1, [1]), True)
        self.assertIs(g(2, [1]), True)

    def test_far_targets_of_every_kind(self):
        # Pairs of all three kinds, in a loop whose exit is also out of
        # reach, run the same as pairs that fuse
        source = ("def f(xs, y):\n    n = 0\n    for x in xs:\n"
                  "        if x < y:\n            n += 1\n"
                  "        if x in (1, 2):\n            n += 10\n"
                  "        if x is y:\n            n += 100\n"
                  "        if x != y:\n            n += 1000\n"
                  "    return n\n")
        old = _testinternalcapi.set_ir_max_forward(0)
        try:
            far = make_function(source)
        finally:
            _testinternalcapi.set_ir_max_forward(old)
        near = make_function(source)
        self.assertEqual(fused_names(far), [])
        self.assertEqual(len(fused_names(near)), 4)
        xs = [0, 1, 2, 3] * 3
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 2):
            self.assertEqual(far(xs, 2), near(xs, 2))
        # Some reach: the jumps that fit fuse, the others do not
        for reach in (1, 2, 3, 5):
            with self.subTest(reach=reach):
                old = _testinternalcapi.set_ir_max_forward(reach)
                try:
                    some = make_function(source)
                finally:
                    _testinternalcapi.set_ir_max_forward(old)
                self.assertLessEqual(len(fused_names(some)), 4)
                self.assertEqual(some(xs, 2), near(xs, 2))

    def test_results_of_other_types(self):
        class Rich:
            def __init__(self, result):
                self.result = result

            def __lt__(self, other):
                return self.result

            def __contains__(self, other):
                return self.result

        class Raises:
            def __bool__(self):
                raise ZeroDivisionError

        def lt(a, b):
            if a < b:
                return 1
            return 0

        def chain(a, b):
            if a < b <= 3:
                return 1
            return 0

        def contains(a, b):
            if a in b:
                return 1
            return 0

        for result in (True, False, 0, 3, "", "x", [], [0], None):
            for func, args in ((lt, (Rich(result), 1)),
                               (contains, (1, Rich(result)))):
                with self.subTest(func=func.__name__, result=result):
                    self.assertEqual(func(*args), int(bool(result)))
                    for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 2):
                        func(*args)
                    self.assertEqual(func(*args), int(bool(result)))
        for func, args in ((lt, (Rich(Raises()), 1)),
                           (contains, (1, Rich(Raises())))):
            with self.subTest(func=func.__name__):
                for _ in range(3):
                    with self.assertRaises(ZeroDivisionError):
                        func(*args)
        for a, b, expected in ((1, 2, 1), (2, 2, 0), (1, 4, 0), (3, 3, 0)):
            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 2):
                self.assertEqual(chain(a, b), expected)

    def test_set_and_dict_sites_deoptimise(self):
        source = ("def f(a, b):\n    if a in b:\n        return 1\n"
                  "    return 0\n")
        threshold = _testinternalcapi.SPECIALIZATION_THRESHOLD + 3
        for container, name in (({1, 2}, "SET"), (frozenset({1, 2}), "SET"),
                                ({1: 0, 2: 0}, "DICT")):
            with self.subTest(container=type(container)):
                f = make_function(source)
                for _ in range(threshold):
                    self.assertEqual(f(1, container), 1)
                    self.assertEqual(f(3, container), 0)
                if SPECIALIZES:
                    self.assertEqual(handlers(f).count(f"CONTAINS_OP_JUMP_{name}"), 1)
                # Another type deoptimises the site; it still answers right
                for other in ([1], (1,), range(2), {3: 0}, {3}):
                    self.assertEqual(f(1, other), int(1 in other))
                for _ in range(4 * threshold):
                    self.assertEqual(f(1, [1]), 1)
                    self.assertEqual(f(2, [1]), 0)

    def test_branch_events_with_disable(self):
        # DISABLE on one side of a fused site turns off that side only,
        # whether the site had specialised or not
        E = sys.monitoring.events
        tool = sys.monitoring.DEBUGGER_ID
        for warm in (False, True):
            f = make_function("def f(a, b):\n    if a < b:\n        return 1\n"
                              "    return 0\n")
            if warm:
                for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 2):
                    f(1, 2)
                    f(2, 1)
            with self.subTest(warm=warm):
                events = []

                def left(code, src, dst):
                    events.append("left")
                    return sys.monitoring.DISABLE

                def right(code, src, dst):
                    events.append("right")

                sys.monitoring.use_tool_id(tool, "test_ir")
                try:
                    sys.monitoring.register_callback(tool, E.BRANCH_LEFT, left)
                    sys.monitoring.register_callback(tool, E.BRANCH_RIGHT, right)
                    sys.monitoring.set_local_events(
                        tool, f.__code__, E.BRANCH_LEFT | E.BRANCH_RIGHT)
                    for _ in range(4):
                        self.assertEqual(f(1, 2), 1)
                        self.assertEqual(f(2, 1), 0)
                    sys.monitoring.restart_events()
                    self.assertEqual(f(1, 2), 1)
                    if warm:
                        self.assertIn("INSTRUMENTED_COMPARE_OP_JUMP",
                                      handlers(f))
                finally:
                    sys.monitoring.set_local_events(tool, f.__code__, 0)
                    sys.monitoring.register_callback(tool, E.BRANCH_LEFT, None)
                    sys.monitoring.register_callback(tool, E.BRANCH_RIGHT, None)
                    sys.monitoring.free_tool_id(tool)
                # "left" is disabled after its first event until restart
                self.assertEqual(events.count("right"), 4)
                self.assertEqual(events.count("left"), 2)
                self.assertEqual(f(2, 1), 0)

    def test_set_lineno_around_a_fused_word(self):
        # Lines, counted from the def: 1 r = 0, 2 the fused if, 3 its body,
        # 4 the tail.  Jump from one to another, once.
        source = ("def f(a, b):\n    r = 0\n    if a < b:\n"
                  "        r = 1\n    r += 10\n    return r\n")
        cases = ((2, 4, 1, 2, 10), (2, 3, 2, 1, 11), (3, 2, 1, 2, 11),
                 (3, 4, 1, 2, 10), (4, 2, 1, 2, 11), (4, 2, 2, 1, 10))
        for src, dst, a, b, expected in cases:
            with self.subTest(src=src, dst=dst, a=a, b=b):
                g = make_function(source)
                first = g.__code__.co_firstlineno
                done = []

                def tracer(frame, event, arg):
                    if frame.f_code is g.__code__ and event == "line":
                        if frame.f_lineno - first == src and not done:
                            done.append(True)
                            frame.f_lineno = first + dst
                    return tracer

                sys.settrace(tracer)
                try:
                    result = g(a, b)
                finally:
                    sys.settrace(None)
                self.assertEqual(result, expected)


if __name__ == "__main__":
    unittest.main()
