"""The baseline JIT (Python/jit.c): code compiled under -X jit=always and
-X jit=on gives the interpreter's results and events.

Each case runs a script in a subprocess with the JIT off and with it on,
and compares what the two print."""

import ctypes
import os
import sys
import textwrap
import unittest
from test.support import script_helper

try:
    import _testinternalcapi
except ImportError:
    _testinternalcapi = None

AVAILABLE = sys._jit.is_available()


def run(source, *xoptions, env=None):
    args = [arg for x in xoptions for arg in ("-X", x)]
    env = dict(env or {}, __isolated=False)
    result = script_helper.assert_python_ok(
        *args, "-c", textwrap.dedent(source), **env)
    return result.out.decode()


@unittest.skipUnless(AVAILABLE, "the JIT is not available")
class JitTestCase(unittest.TestCase):

    def check_same(self, source, modes=("always", "on")):
        """The script prints the same with the JIT off and in each mode."""
        expected = run(source, "jit=off")
        for mode in modes:
            with self.subTest(mode=mode):
                self.assertEqual(run(source, f"jit={mode}"), expected)
        return expected

    def check_compiled(self, source):
        """The script prints the same compiled; it reports what ran compiled
        as lines starting with 'active', which are True under always."""
        expected = run(source, "jit=off")
        got = run(source, "jit=always")
        active = [line for line in got.splitlines() if line.startswith("active")]
        self.assertTrue(active)
        for line in active:
            self.assertTrue(line.endswith("True"), got)
        strip = lambda out: [l for l in out.splitlines() if not l.startswith("active")]
        self.assertEqual(strip(got), strip(expected))
        return got


class TestSwitches(unittest.TestCase):

    @unittest.skipUnless(AVAILABLE, "the JIT is not available")
    def test_modes(self):
        src = "import sys; print(sys._jit.is_enabled())"
        self.assertEqual(run(src).strip(), "False")
        for mode in ("on", "always"):
            self.assertEqual(run(src, f"jit={mode}").strip(), "True")
        self.assertEqual(run(src, "jit=off").strip(), "False")
        for value, out in (("always", "True"), ("1", "True"), ("0", "False"),
                           ("off", "False")):
            with self.subTest(value=value):
                self.assertEqual(run(src, env={"PYTHON_JIT": value}).strip(), out)
        # The option wins over the variable
        self.assertEqual(
            run(src, "jit=off", env={"PYTHON_JIT": "always"}).strip(), "False")

    def test_bad_values(self):
        rc, _, err = script_helper.assert_python_failure("-X", "jit=sometimes",
                                                         "-c", "pass")
        self.assertIn(b"-X jit", err)
        rc, _, err = script_helper.assert_python_failure(
            "-c", "pass", PYTHON_JIT="2", __isolated=False)
        self.assertIn(b"PYTHON_JIT", err)

    def test_isolated_ignores_variable(self):
        out = script_helper.assert_python_ok(
            "-I", "-c", "import sys; print(sys._jit.is_enabled())",
            PYTHON_JIT="always").out
        self.assertEqual(out.strip(), b"False")


class TestCompiled(JitTestCase):

    def test_straight_line(self):
        self.check_compiled("""
            import sys
            def f(a, b):
                c = a + b
                d = c * a - b
                print("active", sys._jit.is_active())
                return (c, d, str(d), [a, b], {a: b})
            for i in range(3):
                print(f(i, 2.5))
            print(f("x", "y") if False else f(3, 4))
        """)

    def test_branches_and_loops(self):
        self.check_compiled("""
            import sys
            def f(n):
                s = 0
                for i in range(n):
                    if i % 3 == 0:
                        s += i
                    elif i % 3 == 1:
                        continue
                    else:
                        s -= 1
                    while s > 100:
                        s //= 2
                print("active", sys._jit.is_active())
                return s
            def g(xs):
                return [x for x in xs if x and not x % 2], any(x > 5 for x in xs)
            print(f(1000), f(0), g(range(10)))
        """)

    def test_calls_and_recursion(self):
        self.check_compiled("""
            import sys
            def fib(n):
                return n if n < 2 else fib(n - 1) + fib(n - 2)
            def deep(n):
                if n == 0:
                    print("active", sys._jit.is_active())
                    return 0
                return 1 + deep(n - 1)
            class A:
                def __init__(self, x):
                    self.x = x
                def m(self, *args, k=1, **kw):
                    return self.x + sum(args) + k + len(kw)
            def kw(**kw):
                return sorted(kw.items())
            print(fib(20), deep(200), A(3).m(1, 2, k=5, z=1), kw(a=1, b=2))
            print(list(map(lambda x: x * 2, range(5))), sorted([3, 1, 2], key=lambda x: -x))
        """)

    def test_exceptions(self):
        self.check_compiled("""
            import sys
            def thrower(n):
                if n == 0:
                    raise ValueError("bottom")
                return thrower(n - 1)
            def f():
                out = []
                for exc in (ZeroDivisionError, KeyError, None):
                    try:
                        try:
                            if exc is ZeroDivisionError:
                                1 / 0
                            elif exc is KeyError:
                                {}[1]
                        finally:
                            out.append("finally")
                    except ArithmeticError as e:
                        out.append(type(e).__name__)
                    except LookupError as e:
                        out.append(repr(e))
                    else:
                        out.append("else")
                try:
                    thrower(50)
                except ValueError as e:
                    out.append(str(e))
                try:
                    try:
                        raise TypeError("a")
                    except TypeError as e:
                        raise RuntimeError("b") from e
                except RuntimeError as e:
                    out.append((str(e), str(e.__cause__)))
                try:
                    raise ExceptionGroup("g", [ValueError(1), KeyError(2)])
                except* ValueError as eg:
                    out.append("star-value")
                except* KeyError as eg:
                    out.append("star-key")
                print("active", sys._jit.is_active())
                return out
            class CM:
                def __enter__(self): return self
                def __exit__(self, *a): return True
            def g():
                with CM():
                    raise OSError
                return "suppressed"
            print(f(), g())
            try:
                thrower(3)
            except ValueError as e:
                import traceback
                print([fs.name for fs in traceback.extract_tb(e.__traceback__)])
        """)

    def test_generators_and_coroutines(self):
        self.check_compiled("""
            import sys, asyncio
            def gen(n):
                for i in range(n):
                    x = yield i * i
                    if x:
                        yield -x
                print("active", sys._jit.is_active())
                return "done"
            def delegate():
                r = yield from gen(3)
                yield r
            g = gen(5)
            out = [next(g), g.send(7), next(g)]
            out += list(gen(4)), list(delegate())
            def closer():
                try:
                    yield 1
                finally:
                    out.append("closed")
            c = closer(); next(c); c.close()
            async def leaf(x):
                await asyncio.sleep(0)
                return x + 1
            async def main():
                print("active", sys._jit.is_active())
                vals = await asyncio.gather(*(leaf(i) for i in range(5)))
                async def agen():
                    for i in range(3):
                        yield i
                return vals, [v async for v in agen()]
            print(out, asyncio.run(main()))
            def thrown():
                try:
                    yield 1
                except KeyError:
                    yield "caught"
            t = thrown(); next(t); print(t.throw(KeyError))
        """)

    def test_settrace_while_compiled(self):
        # Tracing switched on inside a compiled function: the events of its
        # callees and of the frames it starts are the interpreter's.
        self.check_same("""
            import sys
            events = []
            def tracer(frame, event, arg):
                events.append((frame.f_code.co_name, event, frame.f_lineno))
                return tracer
            def callee(n):
                s = 0
                for i in range(n):
                    s += i
                return s
            def f():
                callee(3)
                sys.settrace(tracer)
                x = callee(3)
                y = callee(2)
                sys.settrace(None)
                return x + y
            for i in range(3):
                f()
            print(f())
            print(events)
        """)

    def test_monitoring_while_compiled(self):
        # LINE and branch events switched on in the middle of a compiled
        # function start in that function at once, as in the interpreter.
        self.check_same("""
            import sys
            mon = sys.monitoring
            E = mon.events
            TOOL = 3
            events = []
            def f(n, on):
                s = 0
                for i in range(n):
                    if i == 2 and on:
                        mon.set_events(TOOL, E.LINE | E.BRANCH_LEFT | E.BRANCH_RIGHT
                                       | E.PY_RETURN | E.CALL)
                    if i % 2:
                        s += i
                return s
            mon.use_tool_id(TOOL, "test")
            mon.register_callback(TOOL, E.LINE,
                lambda code, line: events.append(("line", code.co_name, line)))
            mon.register_callback(TOOL, E.BRANCH_LEFT,
                lambda code, a, b: events.append(("left", code.co_name, a, b)))
            mon.register_callback(TOOL, E.BRANCH_RIGHT,
                lambda code, a, b: events.append(("right", code.co_name, a, b)))
            mon.register_callback(TOOL, E.PY_RETURN,
                lambda code, off, v: events.append(("return", code.co_name, v)))
            for i in range(5):
                f(6, False)
            r = f(6, True)
            mon.set_events(TOOL, 0)
            print(r, f(6, False))
            print(events)
        """)

    @unittest.skipIf(_testinternalcapi is None, "needs _testinternalcapi")
    def test_recompiled_after_specialization(self):
        out = run("""
            import sys, _testinternalcapi as t
            def f(a, b):
                return a.x + b
            class A:
                x = 1
            first = None
            for i in range(20):
                f(A(), i)
                if first is None:
                    first = dict(t.jit_unit(f.__code__))
            last = t.jit_unit(f.__code__)
            print(first["compiles"], last["compiles"], last["invalid"], last["units"])
        """, "jit=always")
        first, last, invalid, units = map(int, out.split())
        self.assertEqual(first, 1)
        self.assertGreater(last, 1)
        self.assertLessEqual(last, 8)
        self.assertEqual(invalid, 0)
        self.assertEqual(units, last)

    @unittest.skipIf(_testinternalcapi is None, "needs _testinternalcapi")
    def test_specialize_deopt_respecialize_terminates(self):
        # A site that specializes, deoptimizes and specializes again in one
        # long-running loop: it finishes, the code compiles at most the cap
        # times, and the results are the interpreter's.
        src = """
            import _testinternalcapi as t
            class A:
                def __init__(self): self.v = 1
            class B:
                __slots__ = ("v",)
                def __init__(self): self.v = 2
            def loop(n):
                total = 0
                objs = [A(), B(), 3.5, 2]
                for phase in range(12):
                    o = objs[phase % 4]
                    for i in range(n):
                        if isinstance(o, (A, B)):
                            total += o.v
                        else:
                            total = total + o
                return total
            print(loop(500))
            u = t.jit_unit(loop.__code__)
            print(u and u["compiles"] <= 8)
        """
        expected = run(src, "jit=off")
        for mode in ("always", "on"):
            with self.subTest(mode=mode):
                got = run(src, f"jit={mode}")
                self.assertEqual(got.split()[0], expected.split()[0])
                self.assertEqual(got.split()[1], "True")

    @unittest.skipIf(_testinternalcapi is None, "needs _testinternalcapi")
    def test_warm_up(self):
        out = run("""
            import _testinternalcapi as t
            def once(): return 1
            def hot(): return 2
            def loop():
                s = 0
                for i in range(100):
                    s += i
                return s
            once()
            for i in range(100):
                hot()
            loop()
            print(t.jit_unit(once.__code__) is None,
                  t.jit_unit(hot.__code__) is not None,
                  t.jit_unit(loop.__code__) is not None)
        """, "jit=on")
        self.assertEqual(out.split(), ["True", "True", "True"])

    @unittest.skipIf(_testinternalcapi is None, "needs _testinternalcapi")
    def test_set_warm_up(self):
        out = run("""
            import _testinternalcapi as t
            print(t.set_jit_warmup(1000))
            def hot(): return 2
            before = t.jit_stats()
            for i in range(100):
                hot()
            print(t.jit_unit(hot.__code__) is None)
            t.set_jit_warmup(32)
            for i in range(100):
                hot()
            after = t.jit_stats()
            print(t.jit_unit(hot.__code__) is not None,
                  after["compiles"] > before["compiles"],
                  after["words"] > before["words"])
        """, "jit=on")
        self.assertEqual(out.split(), ["32", "True", "True", "True", "True"])

    def test_perf_map(self):
        if not sys.platform.startswith("linux"):
            self.skipTest("perf maps are Linux files")
        out = run("""
            import os
            def jitted_function_name(): return 1
            jitted_function_name()
            path = f"/tmp/perf-{os.getpid()}.map"
            with open(path) as f:
                names = [line.split(" ", 2)[2] for line in f]
            os.unlink(path)
            print(any(n.startswith("py::jit:jitted_function_name:") for n in names))
        """, "jit=always", "perf")
        self.assertEqual(out.strip(), "True")


def cfi_rows(program, initial):
    """Run a CFI program (x86-64: code alignment 1, data alignment -8) from
    the CIE's initial rule.  Returns [(location, rule)], one per location
    that the program advances to, from 0; a rule is (CFA register, CFA
    offset, {register: its offset from the CFA})."""
    reg, offset, saved = initial
    saved = dict(saved)
    stack = []
    rows = []
    location = 0
    i = 0

    def uleb():
        nonlocal i
        value = shift = 0
        while True:
            byte = program[i]
            i += 1
            value |= (byte & 0x7F) << shift
            shift += 7
            if not byte & 0x80:
                return value

    def flush():
        rows.append((location, (reg, offset, dict(saved))))

    while i < len(program):
        op = program[i]
        i += 1
        if op >> 6 == 1 or op in (0x02, 0x03):          # advance_loc, loc1, loc2
            flush()
            if op >> 6 == 1:
                location += op & 0x3F
            else:
                width = op - 1
                location += int.from_bytes(program[i:i + width], "little")
                i += width
        elif op >> 6 == 2:                              # offset
            saved[op & 0x3F] = -8 * uleb()
        elif op >> 6 == 3:                              # restore: no rule
            saved.pop(op & 0x3F, None)
        elif op == 0x00:                                # nop
            pass
        elif op == 0x0A:                                # remember_state
            stack.append((reg, offset, dict(saved)))
        elif op == 0x0B:                                # restore_state
            reg, offset, saved = stack.pop()
        elif op == 0x0C:                                # def_cfa
            reg, offset = uleb(), uleb()
        elif op == 0x0D:                                # def_cfa_register
            reg = uleb()
        elif op == 0x0E:                                # def_cfa_offset
            offset = uleb()
        else:
            raise AssertionError(f"unexpected CFA opcode {op:#x}")
    flush()
    return rows


@unittest.skipUnless(AVAILABLE, "the JIT is not available")
@unittest.skipIf(_testinternalcapi is None, "requires _testinternalcapi")
class TestUnwind(unittest.TestCase):
    """A unit registers unwind info that states its frame exactly."""

    RBX, RSP, RBP, RIP = 3, 7, 6, 16        # DWARF register numbers
    PUSHED = (3, 12, 13, 14, 15)            # rbx, r12 to r15, in push order

    def unit_of(self, source):
        namespace = {}
        exec(textwrap.dedent(source), namespace)
        code = namespace["f"].__code__
        self.assertTrue(_testinternalcapi.jit_compile(code))
        return _testinternalcapi.jit_unit(code)

    def parse_frame(self, frame, frame_offset):
        """Check the CIE and the FDE's header; returns the FDE's CFI program"""
        def u32(offset):
            return int.from_bytes(frame[offset:offset + 4], "little")
        cie_size = u32(0)
        cie = frame[4:4 + cie_size]
        self.assertEqual(cie[:4], bytes(4))                  # a CIE
        # version 1, "zR"; code alignment 1, data alignment -8, return
        # address column 16, one byte of augmentation data (pc-relative,
        # signed 32 bits); at entry CFA = rsp + 8 and the return address is
        # at CFA - 8
        self.assertEqual(cie[4:17], b"\x01zR\x00" + bytes(
            [1, 0x78, self.RIP, 1, 0x1B, 0x0C, self.RSP, 8, 0x80 | self.RIP]))
        self.assertEqual(cie[17], 1)
        fde_start = 4 + cie_size
        fde_size = u32(fde_start)
        # The CIE pointer counts back from its own field to the CIE
        self.assertEqual(u32(fde_start + 4), fde_start + 4)
        # The pc is relative to its own field: the code is frame_offset
        # bytes before the frame's start
        pc_field = fde_start + 8
        pc = int.from_bytes(frame[pc_field:pc_field + 4], "little", signed=True)
        self.assertEqual(pc_field + pc, -frame_offset)
        self.assertEqual(u32(pc_field + 4), frame_offset)    # the code's range
        self.assertEqual(frame[pc_field + 8], 0)             # no augmentation data
        self.assertEqual(frame[fde_start + 4 + fde_size:], bytes(4))   # the end
        return frame[pc_field + 9:fde_start + 4 + fde_size]

    def test_frame_states_the_code_exactly(self):
        unit = self.unit_of("""
            def f(n, total=0):
                for i in range(n):
                    total += i
                return total
        """)
        start, size = unit["ranges"][0]
        code = ctypes.string_at(start, size)
        program = self.parse_frame(unit["eh_frame"], size)
        entry = (self.RSP, 8, {self.RIP: -8})
        rows = cfi_rows(program, entry)

        def rule_at(pc):
            return [rule for at, rule in rows if at <= pc][-1]

        # The prologue, instruction by instruction
        self.assertEqual(code[0], 0x55)                      # push rbp
        self.assertEqual(rule_at(0), entry)
        pushed_rbp = (self.RSP, 16, {self.RIP: -8, self.RBP: -16})
        self.assertEqual(rule_at(1), pushed_rbp)
        self.assertEqual(code[1:4], bytes([0x48, 0x89, 0xE5]))   # mov rbp, rsp
        self.assertEqual(rule_at(3), pushed_rbp)
        saved = {self.RIP: -8, self.RBP: -16}
        self.assertEqual(rule_at(4), (self.RBP, 16, saved))
        pc = 4
        for n, reg in enumerate(self.PUSHED):                # push rbx, r12..r15
            self.assertEqual(code[pc:pc + (1 if reg < 8 else 2)],
                             bytes([0x50 + reg]) if reg < 8
                             else bytes([0x41, 0x50 + reg - 8]))
            self.assertEqual(rule_at(pc), (self.RBP, 16, saved))
            pc += 1 if reg < 8 else 2
            saved = {**saved, reg: -24 - 8 * n}
            self.assertEqual(rule_at(pc), (self.RBP, 16, saved))
        body = (self.RBP, 16, saved)
        # The body's rule holds to the end of the code except in the shared
        # exit, where each pop restores a register and pop rbp, at the ret,
        # leaves the frame
        leaving = [at for at, (reg, _, _) in rows if reg == self.RSP and at > 4]
        self.assertEqual(len(leaving), 1)
        ret = leaving[0]
        self.assertEqual(code[ret - 1:ret + 1], bytes([0x5D, 0xC3]))   # pop rbp; ret
        self.assertEqual(rule_at(ret), (self.RSP, 8, {self.RIP: -8}))
        popped = dict(saved)
        for reg in reversed(self.PUSHED):
            del popped[reg]
            at = [at for at, rule in rows
                  if pc < at < ret and rule == (self.RBP, 16, popped)]
            self.assertEqual(len(at), 1, popped)
        self.assertEqual(rule_at(ret + 1), body)
        self.assertEqual(rule_at(size - 1), body)

    def test_backtrace_walks_through_units(self):
        """backtrace(), which has no frame pointers to follow, finds the
        return addresses of every compiled frame."""
        out = run("""
            import _testinternalcapi as t, operator
            DEPTH = 10
            def build(n):
                if n == 0:
                    return t.gnu_backtrace_unwind()
                return operator.call(build, n - 1)
            stack = build(DEPTH)
            ranges = t.jit_unit(build.__code__)["ranges"]
            print(sum(any(start <= a < start + size for start, size in ranges)
                      for a in stack))
            labels = t.classify_stack_addresses(stack)
            print(labels.count("python"))
        """, "jit=always")
        in_unit, python = map(int, out.split())
        self.assertGreaterEqual(in_unit, 11)    # build(10) down to build(0)
        self.assertGreaterEqual(python, 10)

    def test_units_come_and_go(self):
        """Every unit's frame is deregistered when its code is freed
        (libgcc aborts on a frame it does not know), including the units a
        recompile replaced."""
        out = run("""
            import _testinternalcapi as t
            for i in range(300):
                namespace = {}
                exec(f"def f(n):\\n    return n + {i}\\n", namespace)
                f = namespace["f"]
                t.jit_compile(f.__code__)
                f(1)
                del f, namespace
            print("ok")
        """, "jit=always")
        self.assertEqual(out.strip(), "ok")


if __name__ == "__main__":
    unittest.main()
