"""The baseline JIT (Python/jit.c): code compiled under -X jit=always and
-X jit=on gives the interpreter's results and events.

Each case runs a script in a subprocess with the JIT off and with it on,
and compares what the two print."""

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


# The scripts of TestHandOff start with this: a function compiles at its
# first warm-up event until settle() raises the warm-up out of reach, so that
# the functions made after it (fresh() copies a code object) stay in the
# interpreter.
HAND_OFF_PRELUDE = """
import sys, types, _testinternalcapi as t
t.set_jit_warmup(1)
def settle():
    t.set_jit_warmup(1 << 30)
def fresh(f):
    return types.FunctionType(f.__code__.replace(), f.__globals__, f.__name__)
def compiled(f):
    return t.jit_unit(f.__code__) is not None
"""


@unittest.skipIf(_testinternalcapi is None, "needs _testinternalcapi")
class TestHandOff(JitTestCase):
    """The evaluator hands a frame to its code's unit at a frame entry, a
    return or yield into a frame that ran in its unit, a generator resume, a
    backward jump and a handled exception; compiled code hands back at
    every frame switch."""

    def check_hand_off(self, source):
        """The script prints the same in both modes and with the JIT off,
        apart from the lines that start with 'jit', which hold what the JIT
        did: under the JIT they must be as the script's expected line
        ('jit-expect ...') says."""
        source = HAND_OFF_PRELUDE + textwrap.dedent(source)
        strip = lambda out: [l for l in out.splitlines() if not l.startswith("jit")]
        expected = run(source, "jit=off")
        for mode in ("always", "on"):
            with self.subTest(mode=mode):
                got = run(source, f"jit={mode}")
                self.assertEqual(strip(got), strip(expected))
                lines = [l.split() for l in got.splitlines() if l.startswith("jit")]
                self.assertTrue(lines, got)
                for name, *values in lines:
                    want = name.split(":", 1)[1].split(",")
                    self.assertEqual(values, want, f"{name}\n{got}")

    def test_call_into_compiled_code(self):
        # An interpreted caller enters its callee's unit at the callee's
        # RESUME
        self.check_hand_off("""
            def callee(x):
                return x + 1, sys._jit.is_active()
            callee(0)
            settle()
            def caller():
                return [callee(i) for i in range(3)], sys._jit.is_active()
            caller = fresh(caller)
            values, active = caller()
            print([v for v, _ in values])
            print("jit:True,True,True,False", *[a for _, a in values], active)
        """)

    def test_return_into_compiled_caller(self):
        # A compiled caller goes back to its unit when an interpreted callee
        # returns into it, and after a callee raised into it
        self.check_hand_off("""
            def callee():
                return sys._jit.is_active()
            def thrower():
                raise KeyError(sys._jit.is_active())
            def caller(f, g):
                inner = f()
                after = sys._jit.is_active()
                try:
                    g()
                except KeyError as e:
                    caught = e.args[0], sys._jit.is_active()
                return inner, after, *caught
            caller(callee, thrower)
            settle()
            print("jit:False,True,False,True", *caller(fresh(callee), fresh(thrower)))
            print("jit:True", compiled(caller))
        """)

    def test_generator_resumed_into_compiled_code(self):
        # An interpreted consumer, by next() and by a for loop, resumes a
        # compiled generator in its unit, and the generator yields back
        self.check_hand_off("""
            def gen(n):
                for i in range(n):
                    yield i, sys._jit.is_active()
            def delegate(n):
                yield from gen(n)
            list(gen(2)); list(delegate(1))
            settle()
            def consume():
                g = gen(3)
                first = next(g)
                rest = [x for x in g]
                return [first] + rest + list(delegate(2)), sys._jit.is_active()
            items, active = fresh(consume)()
            print([i for i, _ in items])
            print("jit:True,True,True,True,True,False", *[a for _, a in items], active)
        """)

    def test_backward_jump_of_a_one_off_call(self):
        # A one-off call warms up in its own loop and goes on compiled from
        # a backward jump
        self.check_hand_off("""
            t.set_jit_warmup(50)
            def once(n):
                s = 0
                seen = []
                for i in range(n):
                    s += i
                    if i in (0, n - 1):
                        seen.append(sys._jit.is_active())
                return s, seen
            s, seen = once(10000)
            print(s)
            print("jit:True", seen[-1])
            print("jit:True", compiled(once))
        """)

    def test_exception_caught_in_compiled_frame(self):
        self.check_hand_off("""
            def f(n):
                out = []
                for i in range(n):
                    try:
                        if i % 2:
                            raise ValueError(i)
                        out.append({}[i])
                    except KeyError:
                        out.append(("key", sys._jit.is_active()))
                    except ValueError as e:
                        out.append(("value", sys._jit.is_active()))
                return out
            f(2)
            settle()
            out = f(4)
            print([k for k, _ in out])
            print("jit:True,True,True,True", *[a for _, a in out])
        """)

    def test_monitoring_switched_on_in_compiled_frame(self):
        # An interpreted callee switches monitoring on and returns into a
        # compiled caller, whose unit is retired: the caller goes on in the
        # interpreter, with the interpreter's events.  So does a generator
        # suspended in its unit, resumed after its unit was retired.
        self.check_hand_off("""
            mon = sys.monitoring
            E = mon.events
            TOOL = 3
            events = []
            mon.use_tool_id(TOOL, "test")
            mon.register_callback(TOOL, E.LINE,
                lambda code, line: events.append((code.co_name, line)))
            def switch():
                mon.set_events(TOOL, E.LINE)
            def gen():
                yield sys._jit.is_active()
                yield sys._jit.is_active()
            def f(on):
                s = 0
                for i in range(4):
                    if on and i == 2:
                        switch()
                    s += i
                return s, sys._jit.is_active()
            f(False)
            g = gen()
            g_before = next(g)
            settle()
            s, before = f(False)
            switch = fresh(switch)
            s2, after = f(True)
            g_after = next(g)
            mon.set_events(TOOL, 0)
            print(s, s2, events)
            print("jit:True,False,True,False", before, after, g_before, g_after)
        """)

    def test_jit_handlers_only_with_the_jit(self):
        # A code lowered with the JIT off holds none of the JIT's handlers
        src = """
            import _testinternalcapi as t
            def f(n):
                for i in range(n):
                    pass
            f(1)
            names = {w[0] for w in t.get_ir(f.__code__, True)}
            print(sorted(n for n in names if n.startswith(("RESUME", "JUMP_BACKWARD"))))
        """
        self.assertEqual(run(src, "jit=off").strip(), "['JUMP_BACKWARD', 'RESUME']")
        for mode in ("on", "always"):
            with self.subTest(mode=mode):
                self.assertEqual(run(src, f"jit={mode}").strip(),
                                 "['JUMP_BACKWARD_JIT', 'RESUME_JIT']")

    def test_ir_forms_turns_the_jit_off(self):
        # The forms executor runs no unit, so a JIT mode under -X ir_forms
        # is off
        src = """
            import sys, _testinternalcapi as t
            def f(): return 1
            for i in range(100): f()
            print(sys._jit.is_enabled(), t.jit_stats()["compiles"])
        """
        for mode in ("on", "always"):
            with self.subTest(mode=mode):
                self.assertEqual(run(src, "ir_forms", f"jit={mode}").split(),
                                 ["False", "0"])


if __name__ == "__main__":
    unittest.main()
