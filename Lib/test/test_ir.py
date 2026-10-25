"""Tests for the register IR (Include/internal/pycore_ir.h) that the
assembler (Python/assemble.c) emits."""

import dis
import io
import sys
import types
import unittest
from test import support
from test.support import import_helper
from test.support.bytecode_helper import encode_words

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


class C:
    attr = 5


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
        # A Py_STACKREF_DEBUG build aborts when the frame closes a local
        # whose borrow a deoptimising LOAD_ATTR left out.
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

        sys.settrace(tracer)
        try:
            result = f(C())
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
                      + "    return v0\n")
            return make_function(source)
        stacksize = make(1).__code__.co_stacksize
        f = make(IR_MAX_SLOTS - stacksize)
        self.assertEqual(f.__code__.co_nlocals + f.__code__.co_stacksize,
                         IR_MAX_SLOTS)
        self.assertEqual(f(), 0)
        with self.assertRaisesRegex(SystemError, "slots"):
            make(IR_MAX_SLOTS - stacksize + 1)

    def test_shared_feedback_entry(self):
        # The sites past the IR_FIELD_MAX-th share entry 0: they never
        # specialise, and those with a static operand run a NOFB form that
        # carries it in the word.
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
        load_attrs = [instr for instr in dis.get_instructions(f)
                      if instr.opname == "LOAD_ATTR"]
        self.assertEqual([instr.fb for instr in load_attrs[:3]], [1, 2, 3])
        shared = load_attrs[IR_FIELD_MAX:]
        self.assertEqual({instr.fb for instr in shared}, {None})
        self.assertEqual({instr.handler for instr in shared},
                         {"LOAD_ATTR_NOFB"})
        self.assertEqual({instr.argval for instr in shared},
                         {"attr", "other"})
        ir = handlers(f)
        if SPECIALIZES:
            self.assertNotEqual(ir[load_attrs[0].offset], "LOAD_ATTR")
        self.assertEqual({ir[instr.offset] for instr in shared},
                         {"LOAD_ATTR_NOFB"})
        for name in ("BINARY_OP_SUBTRACT", "BINARY_OP_FLOOR_DIVIDE",
                     "COMPARE_OP_NOFB", "CONTAINS_OP_NOFB"):
            self.assertIn(name, ir)
        # Other operands take the generic paths
        self.assertEqual(f(O(), 2.0), (-2, 3.0, True, True))
        with self.assertRaises(ZeroDivisionError):
            f(O(), 0)
        # The shared entry holds nothing that dis could show
        dis.dis(f, adaptive=True, show_caches=True, file=io.StringIO())

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
    """Loads folded into the next word (Python/regalloc.c)"""

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

    def test_load_below_another_word_keeps_its_word(self):
        # The constant's word runs between the load of x and the add
        def f(x):
            return x + 1
        self.assertIn("LOAD_FAST_BORROW", [w[0] for w in words(f)])
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

if __name__ == "__main__":
    unittest.main()
