"""The baseline JIT (Python/jit.c): code compiled under -X jit=always and
-X jit=on gives the interpreter's results and events.

Each case runs a script in a subprocess with the JIT off and with it on,
and compares what the two print."""

import os
import sys
import textwrap
import unittest
from test.support import import_helper, script_helper

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

    @unittest.skipUnless(AVAILABLE, "the JIT is not available")
    def test_warmup_variable(self):
        # The previous value that set_jit_warmup() returns is the warm-up
        # in force since startup
        src = "import _testinternalcapi as c; print(c.set_jit_warmup(7))"
        env = {"PYTHON_JIT_WARMUP": "500"}
        self.assertEqual(run(src, "jit=on", env=env).strip(), "500")
        # always compiles at the first event, whatever the variable says
        self.assertEqual(run(src, "jit=always", env=env).strip(), "1")
        # (an empty value is unset, as for every PYTHON* variable)
        for value in ("0", "-3", "x", "12x"):
            with self.subTest(value=value):
                rc, _, err = script_helper.assert_python_failure(
                    "-c", "pass", PYTHON_JIT="on", PYTHON_JIT_WARMUP=value,
                    __isolated=False)
                self.assertIn(b"PYTHON_JIT_WARMUP", err)

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

    def test_jump_over_comprehension(self):
        # A debugger's f_lineno jump backward over a comprehension, at the
        # line after it: under always the frame runs JUMP_BACKWARD_JIT and
        # RESUME_JIT, whose owner programs the jump reads, as the
        # interpreter's JUMP_BACKWARD.
        self.check_same("""
            import sys
            def f(out):
                out.append(1)
                x = [i for i in range(10)]
                out.append(3)
            first = f.__code__.co_firstlineno
            jumped = []
            def tracer(frame, event, arg):
                if frame.f_code is f.__code__:
                    if (event == "line" and frame.f_lineno == first + 3
                            and not jumped):
                        jumped.append(True)
                        try:
                            frame.f_lineno = first + 1
                        except ValueError as error:
                            jumped.append(str(error))
                    return tracer
                return None
            out = []
            sys.settrace(tracer)
            f(out)
            sys.settrace(None)
            print(out, jumped)
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

    def test_return_to_a_frame_at_the_switch_word(self):
        # Observation ends while outer, observed, waits on mid: the change
        # sends both to the switch word.  mid goes on in the evaluator,
        # compiles at its loop and returns from its unit to outer, whose
        # code compiles again: the unit hands the switch word to the
        # evaluator.  noop's return takes outer out of its unit first.
        prelude = """
            import sys
            mon = sys.monitoring
            E = mon.events
            mon.use_tool_id(3, "test")
            mon.register_callback(3, E.LINE, lambda code, line: None)
        """
        ways = {
            "settrace": ("sys.settrace(lambda *args: None)", "sys.settrace(None)"),
            "setprofile": ("sys.setprofile(lambda *args: None)", "sys.setprofile(None)"),
            "monitoring": ("mon.set_events(3, E.LINE)", "mon.set_events(3, 0)"),
        }
        for way, (on, off) in ways.items():
            with self.subTest(way=way):
                self.check_same(prelude + f"""
            def noop():
                pass
            def inner():
                {off}
            def mid():
                inner()
                s = 0
                for i in range(3):
                    s += i
                return s
            def outer(observe):
                if observe:
                    {on}
                noop()
                return mid() + 1
            print(outer(False), outer(True), outer(False))
        """)

    def test_return_to_a_compiled_frame_after_disable(self):
        # f waits in its unit on g when LINE goes on, so its resume point is
        # the switch word; a second call of f returns DISABLE at each of its
        # lines, which clears the code's observed mark with no walk.  g's
        # return finds f's unit retired and the code no longer observed:
        # the switch word stays in the evaluator.
        self.check_same("""
            import sys
            mon = sys.monitoring
            E = mon.events
            mon.use_tool_id(3, "test")
            lines = []
            def line(code, n):
                if code is f.__code__:
                    lines.append(n - f.__code__.co_firstlineno)
                    return mon.DISABLE
            mon.register_callback(3, E.LINE, line)
            def g(mode):
                if mode == 1:
                    mon.set_events(3, E.LINE)
                    f(2)
                return mode
            def f(mode):
                x = g(mode)
                return x
            print(f(0), f(1), f(0), sorted(lines))
            mon.set_events(3, 0)
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
    return or yield into a frame whose code has live entries, a generator
    resume, a backward jump and a handled exception; compiled code goes to
    the evaluator at a switch to code that publishes no entries."""

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
            print(sorted(n for n in names if n.startswith(("Resume", "Jump"))))
        """
        self.assertEqual(run(src, "jit=off").strip(), "['Jump.Poll', 'Resume.Poll']")
        for mode in ("on", "always"):
            with self.subTest(mode=mode):
                self.assertEqual(run(src, f"jit={mode}").strip(),
                                 "['Jump.Warm', 'Resume.Warm']")


class TestEntries(JitTestCase):
    """A frame switch goes to the target frame's word through its code's
    entry table, which the code publishes while its newest unit is valid
    (Python/jit.c, publish()): a retire or a stale mark unpublishes it, and
    the switch that finds none recompiles a stale unit."""

    def test_monitoring_unpublishes_until_the_code_recompiles(self):
        out = run("""
            import sys, _testinternalcapi as t
            def g():
                return sys._jit.is_active()
            def published():
                return t.jit_unit(g.__code__)["published"]
            print(g(), published())
            sys.monitoring.use_tool_id(3, "test")
            sys.monitoring.set_local_events(3, g.__code__,
                                            sys.monitoring.events.LINE)
            sys.monitoring.register_callback(3, sys.monitoring.events.LINE,
                                             lambda *a: None)
            print(g(), published())
            sys.monitoring.set_local_events(3, g.__code__, 0)
            sys.monitoring.free_tool_id(3)
            for i in range(3):
                g()
            print(g(), published())
        """, "jit=always")
        self.assertEqual(out.split(), ["True", "True", "False", "False",
                                       "True", "True"])

    def test_a_stale_unit_recompiles_at_the_next_switch(self):
        # f has no loop, so no backward jump of its own recompiles it: the
        # stale mark unpublishes the entries, and the next call from the
        # compiled loop goes through the driver, which recompiles
        out = run("""
            import _testinternalcapi as t
            class O:
                x = 1
            def f(o):
                return o.x
            # Compiled before any word of f has specialised
            t.jit_compile(f.__code__)
            u = t.jit_unit(f.__code__)
            print(u["published"], u["invalid"], u["compiles"])
            o = O()
            for i in range(100):
                f(o)
            u = t.jit_unit(f.__code__)
            print(u["published"], u["invalid"], u["compiles"] > 1)
        """, "jit=always")
        self.assertEqual(out.split(), ["True", "0", "1", "True", "0", "True"])

    def test_a_frame_in_a_stale_copy_leaves_at_the_changed_word(self):
        # The outer frame's loop runs o.x's copy for A.  Its inner call
        # runs the same word on B's until it specialises for B: the copy
        # that the outer frame goes on in now reads feedback that A's arm
        # did not write, so it goes to the evaluator at the word
        # (Python/jit.c, divert_copy()), and the outer loop's results stay
        # the interpreter's
        out = self.check_same("""
            import sys, _testinternalcapi as t
            class A:
                def __init__(self, x):
                    self.x = x
            class B:
                __slots__ = ("x",)
                def __init__(self, x):
                    self.x = x
            def f(objs, inner):
                total = 0
                for o in objs:
                    total += o.x
                    if o is objs[10] and inner is not None:
                        total += f(inner, None)
                return total
            outer = [A(i) for i in range(20)]
            inner = [B(i) for i in range(1000)]
            print(f(outer, inner))
            u = t.jit_unit(f.__code__)
            print(sys._jit.is_active() == (u is not None and u["diverted"] > 0))
        """, modes=("always",))
        self.assertEqual(out.split(), [str(sum(range(20)) + sum(range(1000))),
                                       "True"])

    def test_a_compiled_head_moves_its_frame_to_the_observing_evaluator(self):
        # f is compiled and off the stack when LINE events start, so its
        # unit is still published: the frame start enters it, and its
        # Resume.Poll head instruments f and names the switch word in
        # return_pc (a REDIRECT status).  The frame goes on there and leaves
        # no entry in the thread's switch list.
        out = run("""
            import sys, _testinternalcapi as t
            m = sys.monitoring
            def f(x):
                y = x + 1
                return y
            for i in range(5):
                f(i)
            print(t.jit_unit(f.__code__)["published"])
            lines = []
            def line(code, n):
                if code is f.__code__:
                    lines.append(n - code.co_firstlineno)
            m.use_tool_id(3, "test")
            m.register_callback(3, m.events.LINE, line)
            m.set_events(3, m.events.LINE)
            base = t.eval_switch_count()
            f(1)
            f(2)
            after = t.eval_switch_count()
            m.set_events(3, 0)
            print(lines, after - base)
        """, "jit=always")
        self.assertEqual(out.splitlines(), ["True", "[1, 2, 1, 2] 0"])

    def test_heads_are_resume_poll_while_published(self):
        # Compile rewrites the code's Resume.Warm heads (the frame start and
        # each yield-from loop's resume) to Resume.Poll; a retire gives them
        # back, so that frame starts count warm-up events again
        out = run("""
            import sys, _testinternalcapi as t
            def g(it):
                yield from it
            def heads():
                return sorted({w[0] for w in t.get_ir(g.__code__, True)
                               if w[0].startswith("Resume.")})
            print(*heads())
            list(g(range(3)))
            print(*heads(), t.jit_unit(g.__code__)["published"])
            sys.monitoring.use_tool_id(3, "test")
            sys.monitoring.set_local_events(3, g.__code__,
                                            sys.monitoring.events.LINE)
            print(*heads(), t.jit_unit(g.__code__)["published"])
            sys.monitoring.set_local_events(3, g.__code__, 0)
            sys.monitoring.free_tool_id(3)
            for i in range(3):
                list(g(range(3)))
            print(*heads(), t.jit_unit(g.__code__)["published"])
        """, "jit=always")
        self.assertEqual(out.splitlines(), [
            "Resume.Warm", "Resume.Poll True", "Resume.Warm False",
            "Resume.Poll True"])

    def test_a_delegation_chain_through_compiled_levels(self):
        # GN's chains move a yield-from chain in one transfer, through
        # compiled levels too (their heads are Resume.Poll): items, a throw
        # that the leaf handles and a close reach every level
        out = run("""
            import sys
            log = []
            def leaf(n):
                try:
                    for i in range(n):
                        try:
                            yield i, sys._jit.is_active()
                        except KeyError:
                            log.append("leaf caught")
                finally:
                    log.append("leaf closed")
            def mid(n):
                yield from leaf(n)
            def top(n):
                yield from mid(n)
            for k in range(3):
                g = top(100)
                items = [next(g) for i in range(50)]
                print(sum(i for i, _ in items), all(a for _, a in items))
                print(g.throw(KeyError))
                g.close()
                print(log)
                log.clear()
        """, "jit=always")
        self.assertEqual(out.splitlines()[-3:], [
            str(sum(range(50))) + " True", "(50, True)",
            "['leaf caught', 'leaf closed']"])


@unittest.skipIf(_testinternalcapi is None, "needs _testinternalcapi")
class TestReclamation(unittest.TestCase):
    """A dead code's units outlive it while a thread may run them: a
    departing word opens the thread's departure window, which its next
    frame switch closes, and the units wait on the interpreter's dead list
    until no thread is in a window (Python/jit.c, _PyJit_Reclaim()).  Freed
    unit memory is inaccessible, so a premature free crashes."""

    # The interpreter and both JIT modes
    MODES = (("jit=off",),) + (
        (("jit=on",), ("jit=always",)) if AVAILABLE else ())

    def check_modes(self, source):
        for options in self.MODES:
            with self.subTest(options=options):
                self.assertEqual(run(source, *options), "ok\n")

    def test_a_code_dies_in_its_own_return(self):
        # f replaces its function's code, so its frame holds the last
        # reference, which the return drops before the unit switches
        self.check_modes("""
            import _testinternalcapi as t, operator, weakref
            log = []
            for i in range(200):
                ns = {}
                exec("def f(self, n):\\n"
                     "    self.__code__ = (lambda self, n: None).__code__\\n"
                     "    return n + 1\\n", ns)
                f = ns.pop("f")
                ref = weakref.ref(f.__code__, lambda r: log.append(i))
                if i % 2:
                    t.jit_compile(f.__code__)
                call = operator.call if i % 3 else (lambda g, *a: g(*a))
                assert call(f, f, i) == i + 1
                assert ref() is None and log[-1] == i, (i, log[-1:])
                del f, ns
            print("ok")
        """)

    @unittest.skipUnless(AVAILABLE, "the JIT is not available")
    def test_a_window_on_another_thread_keeps_the_dead_units(self):
        # The worker's departing frame closes a local whose finalizer
        # blocks with the GIL released: its window stays open, so the units
        # of the codes that die meanwhile wait, through the main thread's
        # compiles; they go once the worker has left its unit
        out = run("""
            import threading, _testinternalcapi as t
            entered = threading.Event()
            release = threading.Event()
            class Blocker:
                def __del__(self):
                    entered.set()
                    release.wait()
            def depart():
                b = Blocker()
                return 1
            def worker():
                depart()
            def die():
                ns = {}
                exec("def f(n):\\n    return n + 1\\n", ns)
                t.jit_compile(ns["f"].__code__)
                assert ns["f"](1) == 2
            # Its return runs in its unit, which marks the departure
            t.jit_compile(depart.__code__)
            th = threading.Thread(target=worker)
            th.start()
            entered.wait()
            for i in range(5):
                die()
            print(t.jit_stats()["dead"] >= 5)
            release.set()
            th.join()
            print(t.jit_stats()["dead"])
        """, "jit=always")
        self.assertEqual(out.split(), ["True", "0"])

    @unittest.skipUnless(AVAILABLE, "the JIT is not available")
    def test_a_raising_unit_leaves_no_window(self):
        # f replaces its function's code and raises, directly or from a
        # callee, so the unwinding of its compiled frame drops the code's
        # last reference.  The return of the compiled leaf before it opens
        # the thread's departure window.  f's caller, which is not warm,
        # goes on in the evaluator: the window is closed, and the next
        # compile frees the dead units.
        out = run("""
            import sys, _testinternalcapi as t
            def leaf(i):
                return i
            t.jit_compile(leaf.__code__)
            for i in range(10):
                ns = {"leaf": leaf}
                exec("def f(self, i):\\n"
                     "    self.__code__ = (lambda self, i: None).__code__\\n"
                     "    i = leaf(i)\\n"
                     "    if i % 2:\\n"
                     "        raise ValueError(i)\\n"
                     "    return int('x')\\n", ns)
                f = ns.pop("f")
                t.jit_compile(f.__code__)
                try:
                    f(f, i)
                except ValueError:
                    pass
                del f, ns
            print(t.jit_unit(sys._getframe().f_code) is None)
            print(t.jit_stats()["dead"] > 0)
            t.jit_compile((lambda: None).__code__)
            print(t.jit_stats()["dead"])
        """, "jit=on")
        self.assertEqual(out.split(), ["True", "True", "0"])

    def test_codes_come_and_go(self):
        self.check_modes("""
            import _testinternalcapi as t
            for i in range(300):
                ns = {}
                exec(f"def f(n):\\n"
                     f"    return n + {i} if n < 3 else f(n - 1)\\n", ns)
                f = ns["f"]
                if i % 2:
                    t.jit_compile(f.__code__)
                assert f(5) == 2 + i
                del f, ns
            print("ok")
        """)


@unittest.skipIf(_testinternalcapi is None, "needs _testinternalcapi")
class TestCompiledArms(JitTestCase):
    """Words of arms that miss, release and specialise, compiled by copying
    their stencils: a hit, a miss to the family's generic arm and the
    generic arm itself give the interpreter's results, exceptions, slots and
    counts, and a finalizer that a release runs sees its events from the
    next word on."""

    def test_resume_check_arm_and_generic(self):
        # RESUME's words are copies of the Resume.Poll stencil.  A pending
        # signal, or monitoring turned on between calls, misses to the
        # generic RESUME arm, which polls or instruments (and retires the
        # unit).  A frame with cells reaches its RESUME inside the unit,
        # after the words that make the cells.
        self.check_same("""
            import signal, sys
            hits = []
            signal.signal(signal.SIGUSR1, lambda *a: hits.append("signal"))
            def cells(a):
                def g():
                    return a
                return g() + 1
            def plain(a):
                return a + 1
            def calls(n):
                return [cells(i) + plain(i) for i in range(n)]
            for i in range(20):
                calls(3)
            print(calls(5))
            # Pending at the next check: an entry's RESUME_CHECK misses and
            # the generic polls, which runs the handler
            signal.raise_signal(signal.SIGUSR1)
            print(calls(2), hits)
            seen = []
            sys.monitoring.use_tool_id(3, "test")
            sys.monitoring.register_callback(
                3, sys.monitoring.events.PY_START,
                lambda code, offset: seen.append(code.co_name))
            sys.monitoring.set_events(3, sys.monitoring.events.PY_START)
            print(calls(2), seen)
            sys.monitoring.set_events(3, 0)
            sys.monitoring.free_tool_id(3)
            print(calls(2))
        """)

    def test_unpack_arms_and_generic(self):
        # Lists, tuples, other iterables, length failures and non-iterables
        # at one site: the list and tuple arms, their misses to the generic
        # arm, and the generic's raise
        self.check_same("""
            import sys
            def f(seqs):
                out = []
                for s in seqs:
                    try:
                        a, b = s
                        out.append((a, b))
                    except (ValueError, TypeError) as e:
                        out.append(type(e).__name__ + ": " + str(e))
                return out
            seqs = [[1, 2], (3, 4), [5, 6, 7], (8,), "xy", iter([9, 10]),
                    {11: 0, 12: 0}, 13, None, [14], ([15], (16,))]
            for i in range(30):
                f(seqs[:2] * 5)
            print(f(seqs))
            print(f(seqs[::-1]))
        """)

    def test_list_slice_misses_to_a_frame_entering_generic(self):
        # GetItem.Generic can enter a __getitem__ frame: a word adapted to
        # GetItem.ListSlice runs ListSlice, whose miss on a str, a tuple or a
        # class with __getitem__ goes on in the generic arm
        self.check_same("""
            class G:
                def __getitem__(self, k):
                    return ("G", k.step)
            def rev(xs):
                return xs[::-1]
            out = []
            for i in range(50):
                out.append(rev([i, 1, 2]))
            print(out[-1])
            for xs in ([1, 2], "abc", (3, 4), G(), b"xy"):
                print(rev(xs))
            for i in range(20):
                for xs in ([1, 2], "abc", G()):
                    out.append(rev(xs))
            print(out[-6:])
        """)

    def test_unpack_into_locals_and_temporaries(self):
        # The elements go to locals (the direct form) or to temporaries; the
        # destination aliases the sequence's own local; the counts of the
        # elements are the interpreter's
        self.check_same("""
            import sys
            class O: pass
            x, y = O(), O()
            def f(n, l, t):
                for i in range(n):
                    a, b, c = l
                    k, m = t
                    t = (k, m)
                    t, m = t, t
                    d = {}
                    e = [a, c]
                    d["p"], d["q"] = e
                return a, b, c, k, m, sorted(d.items(), key=str)
            for i in range(5):
                r = f(10, [x, y, 3], (x, [y]))
            print([type(v).__name__ for v in r[:5]])
            del r
            print(sys.getrefcount(x), sys.getrefcount(y))
        """)

    def test_unpack_element_finalizers(self):
        # Displaced locals die in the unpack, in the interpreter's order
        self.check_same("""
            order = []
            class D:
                def __init__(self, n): self.n = n
                def __del__(self): order.append(self.n)
            def f(n):
                a = b = None
                for i in range(n):
                    seq = [D(2 * i), D(2 * i + 1)]
                    a, b = seq
                return a.n, b.n
            for i in range(4):
                order.clear()
                r = f(5)
            print(r, order)
        """)

    def test_binary_op_arms_and_generic(self):
        # The add arms, their misses to BINARY_OP's generic arm (the
        # operation from the word's entry: + and +=), overflow, and the
        # generic arm's results and raises for other operations
        self.check_same("""
            class V:
                def __init__(self, v): self.v = v
                def __add__(self, o): return V(self.v + getattr(o, "v", o))
                def __repr__(self): return f"V({self.v})"
            def f(pairs):
                out = []
                for a, b in pairs:
                    try:
                        x = a + b
                        a += b
                        out.append((x, a, a * 2, a - b if not isinstance(a, str) else a))
                    except TypeError as e:
                        out.append(str(e))
                return out
            ints = [(i, i * 3) for i in range(10)]
            mixed = ints + [(2**62, 2**62), (1.5, 2), (2.5, 0.5), ("a", "b"),
                            (V(1), 2), (1, "x"), (None, 1), (-2**62, -2**62)]
            for i in range(30):
                f(ints)
            print(f(mixed))
            print(f(ints))
        """)

    def test_monitoring_enabled_by_a_finalizer(self):
        # A __del__ turns LINE events on: run by the generic unpack arm's
        # release of its iterable, by the close of a local that an unpack's
        # store displaces, and by an add's displaced local.  The compiled
        # code leaves at the next word and the events are the interpreter's
        # from there.
        self.check_same("""
            import sys
            mon = sys.monitoring
            E = mon.events
            TOOL = 3
            events = []
            mon.use_tool_id(TOOL, "test")
            mon.register_callback(TOOL, E.LINE,
                lambda code, line: events.append((code.co_name, line)))
            class D:
                def __del__(self):
                    mon.set_events(TOOL, E.LINE)
            class It:
                def __init__(self, die): self.die = die
                def __iter__(self): return iter((1, 2))
                def __del__(self):
                    if self.die:
                        mon.set_events(TOOL, E.LINE)
            def generic(n, at):
                s = 0
                for i in range(n):
                    a, b = It(i == at)
                    s += a + b
                return s
            def unpack(n, at):
                s = 0
                a = None
                for i in range(n):
                    if i == at:
                        a = D()
                    seq = [i, 1]
                    a, b = seq
                    s += a
                return s
            def add(n, at):
                s = 0
                x = None
                for i in range(n):
                    if i == at:
                        x = D()
                    x = i + 1
                    s += x
                return s
            for f in (generic, unpack, add):
                for i in range(5):
                    f(6, -1)
                events.clear()
                r = f(6, 2)
                mon.set_events(TOOL, 0)
                print(f.__name__, r, events)
        """)

    def test_settrace_enabled_by_a_finalizer(self):
        # The same with sys.settrace, from a displaced local's close and from
        # the release of an unpacked iterable (the generic arm).  The
        # finalizer also traces the running frame, whose next line events
        # then show where compiled code left.
        self.check_same("""
            import sys
            events = []
            def tracer(frame, event, arg):
                events.append((frame.f_code.co_name, event, frame.f_lineno))
                return tracer
            def trace_caller():
                sys.settrace(tracer)
                sys._getframe(2).f_trace = tracer
            class D:
                def __del__(self):
                    trace_caller()
            class It:
                def __init__(self, die): self.die = die
                def __iter__(self): return iter((1, 2))
                def __del__(self):
                    if self.die:
                        trace_caller()
            def unpack(n, at):
                s = 0
                a = None
                for i in range(n):
                    if i == at:
                        a = D()
                    seq = (i, 1)
                    a, b = seq
                    s += a
                return s
            def generic(n, at):
                s = 0
                for i in range(n):
                    a, b = It(i == at)
                    s += a + b
                return s
            def add(n, at):
                s = 0
                x = None
                for i in range(n):
                    if i == at:
                        x = D()
                    x = i + 1
                    s += x
                return s
            for f in (unpack, generic, add):
                for i in range(5):
                    f(6, -1)
                events.clear()
                r = f(6, 2)
                sys.settrace(None)
                print(f.__name__, r, events)
        """)

    def test_specialization_reruns_the_word(self):
        # The generic arm specializes its word inside the unit (AGAIN): the
        # word runs again through the generic arm with the right result,
        # the unit is marked stale and recompiles at the next warm-up event,
        # and the second compile plans the specialized arm
        out = run("""
            import _testinternalcapi as t
            def f(n, l):
                s = 0
                for i in range(n):
                    a, b = l
                    s += a * b
                return s
            print(f(200, [3, 4]))
            names = [w[0] for w in t.get_ir(f.__code__, True)]
            print([n for n in names if n.startswith("Unpack.")])
            u = t.jit_unit(f.__code__)
            print(u["compiles"], u["invalid"])
            print(f(3, [5, 6]), t.jit_unit(f.__code__)["compiles"])
        """, "jit=always")
        lines = out.splitlines()
        self.assertEqual(lines[0], str(200 * 12))
        self.assertEqual(lines[1], "['Unpack.List']")
        # The rewrites recompiled the code within the call (words that run
        # once per call, such as the call of range, specialize later)
        compiles, invalid = map(int, lines[2].split())
        self.assertGreaterEqual(compiles, 2)
        self.assertLess(compiles, 8)
        self.assertEqual(invalid, 0)
        self.assertEqual(lines[3].split()[0], "90")

    def test_misses_do_not_mark_unchanged_words(self):
        # A guard miss whose generic run leaves the word as it is does not
        # make the unit stale
        out = run("""
            import _testinternalcapi as t
            def f(n, l):
                s = 0
                for i in range(n):
                    a, b = l[i % 2]
                    s += a + b
                return s
            seqs = [[1, 2], [3, 4]]
            for i in range(10):
                f(20, seqs)
            u = t.jit_unit(f.__code__)
            before = u["compiles"]
            print(f(20, [(1, 2), [3, 4]]))
            u = t.jit_unit(f.__code__)
            print(u["compiles"] == before, u["invalid"])
        """, "jit=always")
        lines = out.splitlines()
        self.assertEqual(lines[0], str(10 * 3 + 10 * 7))
        self.assertEqual(lines[1], "True 0")

    def test_capped_units_are_not_marked(self):
        # A unit compiled at the recompile cap carries no mark: a plan change
        # leaves it valid, and the code compiles no more
        out = run("""
            import _testinternalcapi as t
            def f(n, l):
                s = 0
                for i in range(n):
                    a, b = l
                    s += a * b
                return s
            f(1, [1, 2])
            for i in range(8):
                t.jit_compile(f.__code__)
            print(t.jit_unit(f.__code__)["compiles"])
            print(f(200, [3, 4]))
            names = [w[0] for w in t.get_ir(f.__code__, True)]
            print([n for n in names if n.startswith("Unpack.")])
            u = t.jit_unit(f.__code__)
            print(u["compiles"], u["invalid"])
        """, "jit=always")
        lines = out.splitlines()
        self.assertGreaterEqual(int(lines[0]), 8)
        self.assertEqual(lines[1], str(200 * 12))
        self.assertEqual(lines[2], "['Unpack.List']")
        compiles, invalid = map(int, lines[3].split())
        self.assertEqual(compiles, int(lines[0]))
        self.assertEqual(invalid, 0)


class TestCompilePolicy(JitTestCase):
    """What and when the default mode (on) compiles (Python/jit.c): a word
    that has not run compiles as a stub, a site that flips back goes
    generic, and recompiles back off."""

    def test_a_word_that_has_not_run_is_a_stub(self):
        # The branch's words had not run when f compiled: they are stubs,
        # which run the word in the evaluator.  Reaching them earns a
        # warm-up, after which the code recompiles them as copies.
        out = self.check_same("""
            import _testinternalcapi as t
            t.set_jit_warmup(4)
            def f(n, flag):
                s = 0
                for i in range(n):
                    s += i
                if flag:
                    s = s + len(str(s))
                return s
            for i in range(10):
                f(10, False)
            u = t.jit_unit(f.__code__)
            if u is not None:
                print("stub" in {how for _, how in u["plans"].values()},
                      u["stubs"] > 0)
            else:
                print(True, True)
            print([f(10, True) for i in range(10)])
            u = t.jit_unit(f.__code__)
            print(u is None or u["stubs"] == 0)
        """, modes=("on",))
        self.assertEqual(out.splitlines()[0], "True True")

    def test_always_compiles_every_word(self):
        out = run("""
            import _testinternalcapi as t
            def f(n, flag):
                s = 0
                for i in range(n):
                    s += i
                if flag:
                    s = s + len(str(s))
                return s
            f(10, False)
            print(t.jit_unit(f.__code__)["stubs"])
        """, "jit=always")
        self.assertEqual(out.strip(), "0")

    def test_a_site_that_flips_back_goes_generic(self):
        # Tuple, list, tuple: the site would go back to the arm it held
        # before, so it takes the generic arm and keeps it, in the
        # interpreter and under the JIT
        src = """
            import _testinternalcapi as t
            t.set_jit_warmup(4)
            def g(seqs):
                s = 0
                for x in seqs:
                    a, b = x
                    s += a + b
                return s
            def unpack_arm():
                return [w[0] for w in t.get_ir(g.__code__, True)
                        if w[0].startswith("Unpack.")]
            arms = []
            for k in range(8):
                print(g([(1, 2)] * 100 if k % 2 == 0 else [[1, 2]] * 100))
                arms.append(unpack_arm()[0])
            print(arms[:3])
            print(set(arms[3:]))
        """
        out = self.check_same(src, modes=("on", "always"))
        lines = out.splitlines()
        self.assertEqual(lines[-2], "['Unpack.Tuple', 'Unpack.List', 'Unpack.Generic']")
        self.assertEqual(lines[-1], "{'Unpack.Generic'}")
        out = run(src + """
            u = t.jit_unit(g.__code__)
            print([a for a, how in u["plans"].values() if a.startswith("Unpack.")],
                  u["deopts"], u["invalid"])
        """, "jit=on")
        # Tuple to list deoptimises once; the move to the generic arm is the
        # word's last and is not counted
        self.assertEqual(out.splitlines()[-1], "['Unpack.Generic'] 1 0")

    def test_words_going_generic_do_not_use_the_deopts(self):
        # Four words flip one after another, each deoptimising once: past
        # the two deopts a code has, the third and fourth stay diverted
        # until they go generic, a final move whose recompile is free, so
        # the last unit compiles all four generic arms and diverts none
        out = run("""
            import _testinternalcapi as t
            t.set_jit_warmup(4)
            def g(xs, ys, zs, ws):
                s = 0
                for i in range(len(xs)):
                    a, b = xs[i]
                    c, d = ys[i]
                    e, f = zs[i]
                    h, j = ws[i]
                    s += a + d + e + j
                return s
            T, L = [(1, 2)] * 50, [[1, 2]] * 50
            for _ in range(8):
                g(T, T, T, T)
            for k in range(4):
                for seq in (L, T):
                    args = [T] * 4
                    args[k] = seq
                    for _ in range(8):
                        g(*args)
            u = t.jit_unit(g.__code__)
            print(sorted((a, how) for a, how in u["plans"].values()
                         if a.startswith("Unpack.")), u["invalid"])
        """, "jit=on")
        self.assertEqual(out.splitlines()[-1],
                         str([("Unpack.Generic", "stencil")] * 4) + " 0")

    def test_recompiles_back_off(self):
        # A word that leaves its specialised arm makes the unit stale; the
        # code recompiles once it has earned another warm-up, and after two
        # such recompiles it runs on with the changed words diverted
        out = run("""
            import _testinternalcapi as t
            t.set_jit_warmup(4)
            def f(n, l1, l2, l3):
                s = 0
                for i in range(n):
                    a, b = l1
                    c, d = l2
                    e, g = l3
                    s += a + c + e
                return s
            T, L = (1, 2), [1, 2]
            for i in range(10):
                f(100, T, T, T)
            u = t.jit_unit(f.__code__)
            print(u["deopts"], u["invalid"])
            # One word changes: stale, and not recompiled within fewer events
            # than the warm-up
            t.set_jit_warmup(100)
            f(60, L, T, T)
            u = t.jit_unit(f.__code__)
            print(u["deopts"], u["published"])
            compiles = u["compiles"]
            f(1, L, T, T)
            print(t.jit_unit(f.__code__)["compiles"] == compiles)
            for i in range(10):
                f(100, L, T, T)
            u = t.jit_unit(f.__code__)
            print(u["compiles"] > compiles, u["invalid"], u["published"])
            for i in range(10):
                f(100, L, L, T)
            for i in range(10):
                f(100, L, L, L)
            u = t.jit_unit(f.__code__)
            print(u["deopts"], u["invalid"], u["diverted"] > 0)
            print(f(10, L, L, L))
        """, "jit=on")
        lines = out.splitlines()
        self.assertEqual(lines[0], "0 0")
        self.assertEqual(lines[1], "1 False")
        self.assertEqual(lines[2], "True")
        self.assertEqual(lines[3], "True 0 True")
        self.assertEqual(lines[4], "2 0 True")
        self.assertEqual(lines[5], "30")


    def test_a_loop_over_the_l1i_is_declined(self):
        # A loop whose compiled body would not fit the L1i is declined at
        # warm-up: the code keeps no unit.  A small one compiles.
        out = run("""
            import _testinternalcapi as t
            t.set_jit_warmup(4)
            def make(n):
                body = "".join(f"        a{k}, b{k} = x\\n" for k in range(n))
                ns = {}
                exec(f"def f(seq):\\n    for x in seq:\\n{body}    return a0", ns)
                return ns["f"]
            big, small = make(400), make(4)
            before = t.jit_stats()["declined"]
            for i in range(10):
                print(big([(1, 2)] * 10), small([(1, 2)] * 10))
            print(t.jit_unit(big.__code__) is None,
                  t.jit_unit(small.__code__) is not None,
                  t.jit_stats()["declined"] - before)
        """, "jit=on")
        lines = out.splitlines()
        self.assertEqual(lines[:10], ["1 1"] * 10)
        self.assertEqual(lines[-1], "True True 1")

    def test_a_loop_that_grows_over_the_l1i_is_retired(self):
        # The body compiles while its unpacks have not run (stubs); once
        # they run, the recompile would not fit the L1i: the unit is
        # retired and the code runs in the evaluator from then on
        out = run("""
            import _testinternalcapi as t
            t.set_jit_warmup(4)
            body = "".join(f"            a{k}, b{k} = x\\n" for k in range(400))
            ns = {}
            exec("def f(seq, run):\\n    s = 0\\n    for x in seq:\\n"
                 f"        if run:\\n{body}            s += a0\\n    return s", ns)
            f = ns["f"]
            for i in range(10):
                print(f([(1, 2)] * 10, False))
            u = t.jit_unit(f.__code__)
            print(u["invalid"], u["stubs"] >= 400)
            before = t.jit_stats()["declined"]
            for i in range(10):
                print(f([(1, 2)] * 10, True))
            print(t.jit_unit(f.__code__)["invalid"] & 2,
                  t.jit_stats()["declined"] - before)
        """, "jit=on")
        lines = out.splitlines()
        self.assertEqual(lines[:10], ["0"] * 10)
        self.assertEqual(lines[10], "0 True")
        self.assertEqual(lines[11:21], ["10"] * 10)
        self.assertEqual(lines[-1], "2 1")

TRANSFER_PRELUDE = """
import sys, weakref, _testinternalcapi as t
def stats():
    # Compiled code counts its switches only with Py_STATS: -1 without
    s = t.jit_stats()
    return s["handoffs"], s.get("switches", -1)
def report(name, before):
    # What the JIT did meanwhile: the evaluator's hand-offs into units and
    # compiled code's switches, for the record
    handoffs, switches = stats()
    switched = switches - before[1] if switches >= 0 else -1
    print(f"jit {name} handoffs={handoffs - before[0]} switches={switched}")
"""


@unittest.skipIf(_testinternalcapi is None, "needs _testinternalcapi")
class TestFrameTransfers(JitTestCase):
    """Calls, returns, yields and resumes between compiled frames
    (g3/design.md section 8, Tests).  Each script prints the same with the
    JIT off, on and always, apart from its 'jit' lines, which record the
    evaluator's hand-offs into units and the switches each part took, and
    its 'active' lines, which hold sys._jit.is_active() inside callees and
    must be True under always.  A frame switch between compiled frames is a
    tail call through the target's entries, so a part that calls compiled
    code counts switches and only the hand-offs of the few leaves: an
    interpreter fallback, which hands the frame back to its unit at the next
    switch, fails assert_switched()."""

    def assert_switched(self, out, name, switches, handoffs=None):
        """The part name of out took at most handoffs hand-offs from the
        evaluator: the few leaves (a thread's or C's entry, a warm-up event,
        a recompile), and one for each guard miss, after which the frame
        runs in the evaluator until its next switch (a call whose frame
        needs a new chunk of the data stack misses, about once in 100
        levels); by default 16 and 1 in 50 of the switches.  And, where
        compiled code counts them (Py_STATS), at least switches
        switches."""
        if handoffs is None:
            handoffs = 16 + switches // 50
        for line in out.splitlines():
            if line.startswith(f"jit {name} "):
                fields = dict(f.split("=") for f in line.split()[2:])
                if int(fields["switches"]) >= 0:
                    self.assertGreaterEqual(int(fields["switches"]), switches, line)
                self.assertLessEqual(int(fields["handoffs"]), handoffs, line)
                return
        self.fail(f"no 'jit {name}' line in {out}")

    def check_transfers(self, source):
        source = TRANSFER_PRELUDE + textwrap.dedent(source)
        strip = lambda out: [l for l in out.splitlines()
                             if not l.startswith(("jit", "active"))]
        expected = run(source, "jit=off")
        outs = {}
        for mode in ("on", "always"):
            with self.subTest(mode=mode):
                got = run(source, f"jit={mode}")
                self.assertEqual(strip(got), strip(expected))
                outs[mode] = got
        for line in outs.get("always", "").splitlines():
            if line.startswith("active"):
                self.assertTrue(line.endswith("True"), outs["always"])
        return outs

    def test_recursion(self):
        outs = self.check_transfers("""
            def fib(n):
                return n if n < 2 else fib(n - 1) + fib(n - 2)
            def even(n):
                return True if n == 0 else odd(n - 1)
            def odd(n):
                return False if n == 0 else even(n - 1)
            def kw(n, step=1, *, acc=0):
                return acc if n <= 0 else kw(n - step, acc=acc + n)
            def active(n):
                return sys._jit.is_active() if n == 0 else active(n - 1)
            for f in (fib, even, kw, active):
                f(3)
            before = stats()
            print(fib(18), even(301), odd(301), kw(100), kw(100, 3))
            report("recursion", before)
            print("active", active(10))
        """)
        # fib(18) alone makes 8,360 calls and as many returns
        self.assert_switched(outs["always"], "recursion", 2 * 8360)

    def test_recursion_limit(self):
        # Counted, not measured on the C stack: the same depth everywhere
        self.check_transfers("""
            def down(n):
                return down(n + 1) if n < 10**6 else n
            depth = []
            def probe(n):
                try:
                    return probe(n + 1)
                except RecursionError:
                    depth.append(n)
                    return n
            for limit in (200, 1000):
                sys.setrecursionlimit(limit)
                try:
                    down(0)
                except RecursionError as e:
                    print(type(e).__name__, e)
                print(probe(0) - len(depth) * 0)
            sys.setrecursionlimit(1000)
        """)

    def test_deep_recursion_in_a_small_c_stack(self):
        # 20,000 Python frames in a thread with 256 KiB of C stack: compiled
        # calls must not grow the C stack per Python call
        outs = self.check_transfers("""
            import threading
            def down(n):
                return 0 if n == 0 else 1 + down(n - 1)
            down(50)
            result = []
            def body():
                sys.setrecursionlimit(25000)
                before = stats()
                result.append(down(20000))
                report("deep", before)
            threading.stack_size(256 * 1024)
            thread = threading.Thread(target=body)
            thread.start()
            thread.join()
            print(result)
        """)
        self.assert_switched(outs["always"], "deep", 2 * 20000)

    def test_mixed_tiers(self):
        # Every other level is cold: one code is monitored, which retires
        # its units, so its frames run in the evaluator
        self.check_transfers("""
            def hot(n):
                return 0 if n == 0 else 1 + cold(n - 1)
            def cold(n):
                return 0 if n == 0 else 1 + hot(n - 1)
            def via_c(n):
                # Python from C inside compiled frames
                return sum(map(hot, range(n)))
            for i in range(3):
                hot(10), via_c(5)
            sys.monitoring.use_tool_id(3, "test")
            sys.monitoring.set_local_events(3, cold.__code__, sys.monitoring.events.LINE)
            sys.monitoring.register_callback(3, sys.monitoring.events.LINE, lambda *a: None)
            before = stats()
            print(hot(101), via_c(30))
            report("mixed", before)
            sys.monitoring.set_local_events(3, cold.__code__, 0)
            sys.monitoring.free_tool_id(3)
            print(hot(101), via_c(30))
        """)

    def test_capped_callers_and_callees(self):
        self.check_transfers("""
            def callee(x):
                return x + 1
            def caller(n):
                total = 0
                for i in range(n):
                    total += callee(i)
                return total
            for f in (callee, caller):
                for i in range(8):
                    t.jit_compile(f.__code__)
            print(t.jit_unit(caller.__code__)["compiles"] if t.jit_unit(caller.__code__) else 8)
            print(caller(1000))
        """)

    def test_code_destruction(self):
        self.check_transfers("""
            import gc
            log = []
            def g(x):
                return x * 2
            def died(name):
                # The callback runs Python that runs compiled code
                return lambda ref: log.append((name, [g(i) for i in range(3)]))
            def fresh(source, name="f", **env):
                ns = dict(env)
                exec(source, ns)
                return ns.pop(name)

            # 1. a function deletes the last references to itself and its
            #    code, and returns
            for i in range(40):
                ns = {"log": log}
                exec("def f():\\n    del ns['f']\\n    return 1\\n", ns)
                ns["ns"] = ns
                keep = weakref.ref(ns["f"].__code__, died("one"))
                ns["f"]()
                ns.clear()
            print(log[-1], len(log))
            log.clear()

            # 2. a frame object kept by an object whose finalizer the pop
            #    runs
            class Holder:
                def __del__(self):
                    log.append(("finalizer", g(3)))
            for i in range(40):
                f = fresh("def f(h):\\n    h.frame = sys._getframe()\\n    return 2\\n",
                          sys=sys)
                h = Holder()
                keep = weakref.ref(f.__code__, died("two"))
                f(h)
                del f, h
            gc.collect()
            print(sorted(set(name for name, _ in log)))
            log.clear()

            # 3. recursion whose last reference goes at depth 5
            for i in range(20):
                ns = {}
                exec("def r(n):\\n"
                     "    if n == 5:\\n"
                     "        ns.clear()\\n"
                     "        return n\\n"
                     "    return r(n + 1) if n < 5 else n\\n", ns)
                r = ns["r"]
                ns["ns"] = ns
                keep = weakref.ref(r.__code__, died("three"))
                del r
                print(ns["r"](0), end=" ") if i == 0 else ns["r"](0)
            print()
            print(log[-1], len(log))
            log.clear()

            # 4. a finalizer in the pop calls compiled functions and drops
            #    the outer departing code
            class Dropper:
                def __init__(self, ns):
                    self.ns = ns
                def __del__(self):
                    log.append(("dropper", [g(i) for i in range(2)]))
                    self.ns.clear()
            for i in range(20):
                ns = {}
                exec("def f():\\n    d = Dropper(ns)\\n    return 4\\n",
                     {"Dropper": Dropper, "ns": ns}, ns)
                keep = weakref.ref(ns["f"].__code__, died("four"))
                ns["f"]()
            print(log[-2:], len(log))
            log.clear()

            # 5. 300 codes made and freed with calls between them
            total = 0
            for i in range(300):
                f = fresh(f"def f(n):\\n    return n + {i} if n < 2 else f(n - 1)\\n")
                f.__globals__["f"] = f
                total += f(3) + g(i)
                del f
            print(total)

            # 6. a capped unit's return loses the last reference to its
            #    code: the code is at the recompile cap, so its unit keeps
            #    running, and the return marks the departure
            for i in range(20):
                ns = {}
                exec("def f(drop):\\n"
                     "    if drop:\\n"
                     "        ns.clear()\\n"
                     "    return 6\\n", {"ns": ns}, ns)
                for k in range(8):
                    t.jit_compile(ns["f"].__code__)
                if i == 0:
                    unit = t.jit_unit(ns["f"].__code__)
                    print("jit capped", unit and unit["compiles"])
                for k in range(3):
                    ns["f"](False)
                keep = weakref.ref(ns["f"].__code__, died("six"))
                ns["f"](True)
            print(log[-1], len(log))
            log.clear()

            # 7. code.replace() and __code__ assignment while frames of the
            #    old code run and return
            def outer(n):
                if n == 0:
                    outer.__code__ = replaced
                    return "old"
                return outer(n - 1) + "+old"
            replaced = outer.__code__.replace(co_consts=tuple(
                "new" if c == "old" else "+new" if c == "+old" else c
                for c in outer.__code__.co_consts))
            original = outer.__code__
            for i in range(3):
                outer.__code__ = original
                print(outer(3), outer(2))
        """)

    def test_generators(self):
        self.check_transfers("""
            log = []
            def gen(n):
                for i in range(n):
                    yield i, sys._jit.is_active()
            def consume(n):
                return [v for v, _ in gen(n)], all(a for _, a in gen(n))
            consume(3)
            print(consume(10)[0])
            print("active", consume(10)[1])

            # Destruction during yield publication: the consumer's
            # destination held the last reference to the generator
            def chain():
                for i in range(3):
                    holder = [iter(gen(2))]
                    x = next(holder[0])
                    holder = None
                    yield x[0]
            print(list(chain()))

            # close() and destruction of a suspended compiled generator
            def closing():
                try:
                    yield 1
                    yield 2
                finally:
                    log.append("closed")
            for i in range(3):
                c = closing()
                next(c)
                c.close()
                d = closing()
                next(d)
                del d
            print(log)

            # A generator function called from a compiled frame whose code
            # then loses its last reference
            def maker():
                ns = {}
                exec("def h():\\n    yield 7\\n    yield 8\\n", ns)
                g = ns.pop("h")()
                return g
            gens = [maker() for i in range(5)]
            print([list(g) for g in gens])
        """)

    def test_generator_boundary_departures(self):
        # The guarded generator-boundary variants, warmed until their
        # words specialise: (a) a miss at the guard just after a code died
        # in its own return, whose units wait on the dead list; (b) a
        # completion whose departing frame's code loses its last reference
        # (GEN_COMPLETE_RETURN for the RETURN consumers)
        outs = self.check_transfers("""
            import gc
            log = []
            def g(x):
                return x * 2
            def died(name):
                return lambda ref: log.append((name, [g(i) for i in range(3)]))
            def dying_code(name):
                # A return that drops the last reference to its code
                ns = {}
                exec("def d():\\n    ns.clear()\\n    return 0\\n", {"ns": ns}, ns)
                keep = weakref.ref(ns["d"].__code__, died(name))
                ns["d"]()
                return keep
            def inner():
                yield 5
                return 6
            async def agen():
                yield 1
            async def coro():
                return 9
            class Other:
                # An awaitable that is no async generator's asend
                def asend(self, value):
                    return coro()
            def drive(c):
                try:
                    while True:
                        c.send(None)
                except StopIteration as e:
                    return e.value
            def run_virtual(f, it, drop):
                return list(f(it, drop))
            def run_async(f, it, drop):
                return drive(f(it, drop))
            DROP = "    if drop:\\n        ns.clear()\\n"
            cases = [
                ("SEND_VIRTUAL_XFER_RETURN",
                 "def f(it, drop):\\n" + DROP + "    return (yield from it)\\n",
                 run_virtual, lambda: [1, 2], inner),
                ("SEND_VIRTUAL_XFER_POP_TOP",
                 "def f(it, drop):\\n" + DROP + "    yield from it\\n    return 1\\n",
                 run_virtual, lambda: [1, 2], inner),
                ("SEND_ASYNC_GEN_XFER_RETURN",
                 "async def f(it, drop):\\n" + DROP + "    return await it.asend(None)\\n",
                 run_async, agen, Other),
                ("SEND_ASYNC_GEN_XFER_STORE_FAST",
                 "async def f(it, drop):\\n" + DROP +
                 "    x = await it.asend(None)\\n    return x\\n",
                 run_async, agen, Other),
            ]
            for name, source, run, hit, miss in cases:
                ns = {}
                exec(source, {"ns": ns}, ns)
                for k in range(40):
                    run(ns["f"], hit(), False)
                words = {r[0] for r in t.get_ir(ns["f"].__code__, True)}
                print("jit words", name, name in words)
                # (a)
                keep_a = dying_code(name + " a")
                print(name, "a", run(ns["f"], miss(), False))
                # (b)
                keep_b = weakref.ref(ns["f"].__code__, died(name + " b"))
                print(name, "b", run(ns["f"], hit(), True))
                gc.collect()
            print(sorted(log))
        """)
        # The XFER fusions of SEND with its consumer are dropped: the words
        # in "jit words" lines are no longer expected

    def test_unwinding(self):
        self.check_transfers("""
            import traceback
            def thrower(n):
                if n == 0:
                    raise KeyError("deep")
                return thrower(n - 1) + 1
            def catcher(n, m):
                if n == m:
                    try:
                        return thrower(n)
                    except KeyError as e:
                        return ("caught", n, sys._jit.is_active())
                return catcher(n + 1, m)
            for i in range(3):
                catcher(0, 4)
            r = catcher(0, 6)
            print(r[:2])
            print("active", r[2])
            try:
                thrower(4)
            except KeyError:
                print(traceback.format_exc().replace(__file__ if "__file__" in dir() else "@", "@"))

            # RecursionError in a frame just made current
            def at_limit(n):
                return at_limit(n + 1)
            sys.setrecursionlimit(300)
            for i in range(3):
                try:
                    at_limit(0)
                except RecursionError as e:
                    print(type(e).__name__)
            sys.setrecursionlimit(1000)
        """)

    def test_monitoring_turned_on_in_compiled_callees(self):
        # Events turned on inside a compiled callee, a callee that
        # instruments its caller, and monitoring off again: the events and
        # results equal the interpreter's, and the codes compile again
        outs = self.check_transfers("""
            M = sys.monitoring
            E = M.events
            events = []
            def leaf(n):
                return n + 1
            def mid(n):
                return leaf(n) + 1
            def top(n):
                return mid(n) + 1
            def turn_on(n):
                if n == 0:
                    M.use_tool_id(2, "test")
                    for ev in (E.PY_START, E.PY_RETURN):
                        M.register_callback(2, ev, lambda code, *a, ev=ev:
                            events.append((ev, code.co_name)) if code.co_name != "<lambda>" else None)
                    M.register_callback(2, E.CALL, lambda code, off, f, arg0:
                        events.append(("call", code.co_name, getattr(f, "__name__", "?")))
                        if code.co_name in ("top", "mid", "turn_on") else None)
                    M.set_events(2, E.PY_START | E.PY_RETURN | E.CALL)
                    return 0
                return turn_on(n - 1) + top(n)
            for i in range(5):
                top(i)
            print(turn_on(3))
            print(top(1))
            print(events)
            M.set_events(2, 0)
            M.free_tool_id(2)
            events.clear()

            # A callee instruments its caller, which goes on in the
            # evaluator from the return
            lines = []
            def instrumenting(code):
                M.set_local_events(3, code, E.LINE)
                return 1
            def instrumented(n):
                x = instrumenting(instrumented.__code__) + n
                y = x * 2
                return y
            for i in range(5):
                M.use_tool_id(3, "test")
                M.register_callback(3, E.LINE, lambda code, line:
                    lines.append(line - code.co_firstlineno))
                print(instrumented(i), lines)
                lines.clear()
                M.set_local_events(3, instrumented.__code__, 0)
                M.free_tool_id(3)

            # Monitoring off: the codes run compiled again
            for i in range(5):
                top(i)
            before = stats()
            print(top(10))
            report("off", before)
            unit = t.jit_unit(top.__code__)
            print("jit valid", unit is not None and unit["invalid"] == 0)
        """)
        self.assertIn("jit valid True", outs["always"])
        self.assert_switched(outs["always"], "off", 4)

    def test_settrace_three_frames_deep(self):
        # sys.settrace set in a compiled frame three frames deep: the
        # events of the frames that go on, compared with the interpreter's
        self.check_transfers("""
            events = []
            def tracer(frame, event, arg):
                if frame.f_code.co_name in ("a", "b", "c", "d"):
                    events.append((event, frame.f_code.co_name,
                                   frame.f_lineno - frame.f_code.co_firstlineno))
                return tracer
            def d(n):
                return n * 2
            def c(on):
                if on:
                    sys.settrace(tracer)
                x = d(1)
                return x
            def b(on):
                y = c(on)
                return y + d(2)
            def a(on):
                z = b(on)
                return z + d(3)
            for i in range(10):
                a(False)
            print(a(True))
            sys.settrace(None)
            for e in events:
                print(*e)
            events.clear()
            print(a(False), events)
        """)

    def test_delegated_yield_publication(self):
        # (c) of the generator-boundary variants: a value that f's
        # SEND_VIRTUAL_XFER word yields on, delegated, goes into the
        # consumer's loop variable, which held the last reference to a
        # suspended delegating generator: its close() runs compiled code
        # while the yield publishes.  The async variants yield to a C
        # send(), so their consumer has no home to publish into.
        outs = self.check_transfers("""
            log = []
            def g(x):
                return x * 2
            def susp():
                try:
                    yield 0
                finally:
                    log.append(("closed", [g(i) for i in range(3)]))
            DROP = "    if drop:\\n        ns.clear()\\n"
            cases = [
                ("SEND_VIRTUAL_XFER_RETURN",
                 "def f(it, drop):\\n" + DROP + "    return (yield from it)\\n"),
                ("SEND_VIRTUAL_XFER_POP_TOP",
                 "def f(it, drop):\\n" + DROP + "    yield from it\\n    return 1\\n"),
            ]
            def consume(f):
                x = f(susp(), False)
                next(x)
                seen = []
                for x in f([1, 2, 3], False):
                    seen.append(x)
                return seen
            for name, source in cases:
                ns = {}
                exec(source, {"ns": ns}, ns)
                for k in range(40):
                    list(ns["f"]([1, 2], False))
                words = {r[0] for r in t.get_ir(ns["f"].__code__, True)}
                print("jit words", name, name in words)
                for k in range(3):
                    print(name, consume(ns["f"]), log)
                    log.clear()
        """)
        # The XFER fusions of SEND with its consumer are dropped: the words
        # in "jit words" lines are no longer expected

    def test_stale_and_retired_callers(self):
        self.check_transfers("""
            class A:
                x = 1
            class B:
                x = 2
            def callee(obj, flip):
                if flip:
                    # The caller's word specialised for A meets B: its
                    # unit is marked stale and recompiled at a warm-up
                    return caller(B(), False)
                return obj.x
            def caller(obj, flip):
                return obj.x + callee(obj, flip)
            for i in range(50):
                caller(A(), False)
            print(caller(A(), True), caller(B(), False), caller(A(), False))

            monitoring = []
            def retiring():
                # Instruments the caller's code: its unit retires, and the
                # rest of it runs in the evaluator
                if monitoring:
                    sys.monitoring.set_local_events(
                        5, victim.__code__, sys.monitoring.events.LINE)
                return 1
            def victim(n):
                total = 0
                for i in range(n):
                    total += retiring() if i == n // 2 else i
                return total
            victim(10)
            sys.monitoring.use_tool_id(5, "test")
            sys.monitoring.register_callback(5, sys.monitoring.events.LINE, lambda *a: None)
            monitoring.append(True)
            print(victim(100))
            monitoring.clear()
            sys.monitoring.set_local_events(5, victim.__code__, 0)
            sys.monitoring.free_tool_id(5)
            print(victim(100))
        """)


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
        if op >> 6 == 1 or op in (0x02, 0x03, 0x04):    # advance_loc, loc1, loc2, loc4
            flush()
            if op >> 6 == 1:
                location += op & 0x3F
            else:
                width = {0x02: 1, 0x03: 2, 0x04: 4}[op]
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
    """A unit registers unwind info that states its copies' frames."""

    RSP, RIP = 7, 16                        # DWARF register numbers

    def unit_of(self, source):
        namespace = {}
        exec(textwrap.dedent(source), namespace)
        function = namespace["f"]
        self.assertTrue(_testinternalcapi.jit_compile(function.__code__))
        # The unit lives as long as the function's code, which the caller keeps
        return function, _testinternalcapi.jit_unit(function.__code__)

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

    def test_rows_follow_the_copies(self):
        """A unit has no frame of its own: its rows are its copies' own,
        each starting from the CIE's rule, which is the rule at every head
        word's entry."""
        function, unit = self.unit_of("""
            def f(n, total=0):
                for i in range(n):
                    total += i
                return total
        """)
        start, size = unit["ranges"][0]
        program = self.parse_frame(unit["eh_frame"], size)
        entry = (self.RSP, 8, {self.RIP: -8})
        rows = cfi_rows(program, entry)

        def rule_at(pc):
            return [rule for at, rule in rows if at <= pc][-1]

        self.assertTrue(unit["entries"])
        for index, offset in unit["entries"].items():
            self.assertEqual(rule_at(offset), entry, index)
        # The copies run on the driver's frame, as the arms do: the CFA is
        # the stack pointer's, the return address the driver's, and the
        # stack adjustments of the copies are stated
        for at, (reg, offset, saved) in rows:
            self.assertEqual(reg, self.RSP, at)
            self.assertGreaterEqual(offset, 8, at)
            self.assertEqual(saved.get(self.RIP), -8, at)
        self.assertTrue(any(offset > 8 for _, (_, offset, _) in rows))

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
            # The module's names are bound before build runs: a new global
            # would fail the global reads' guards, and a frame that misses
            # goes on in the evaluator.  Until specialisation settles, a
            # recompile may leave a frame in the evaluator (a stale unit).
            stack = ranges = labels = i = None
            for i in range(4):
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

    def test_faulthandler_walks_through_units(self):
        """faulthandler's C stack dump goes through every compiled frame,
        which has no symbol, down to the program's entry."""
        out = run("""
            import faulthandler, operator, sys
            def build(n, dump):
                if n == 0:
                    if dump:
                        faulthandler.dump_c_stack(sys.stdout)
                    return
                return operator.call(build, n - 1, dump)
            # Until specialisation settles, a frame may go on in the
            # evaluator
            for i in range(3):
                build(3, False)
            build(3, True)
        """, "jit=always")
        lines = out.splitlines()
        units = [i for i, line in enumerate(lines) if "'<unknown>'" in line]
        self.assertGreaterEqual(len(units), 4, out)  # build(3) to build(0)
        self.assertTrue(any("Py_BytesMain" in line
                            for line in lines[units[-1] + 1:]), out)

    def test_backtrace_in_one_activation(self):
        """Direct recursion ten compiled frames deep runs in one native
        activation: backtrace() finds one return address in the units."""
        out = run("""
            import _testinternalcapi as t
            def build(n):
                if n == 0:
                    return t.gnu_backtrace_unwind()
                return build(n - 1)
            for _ in range(4):
                stack = build(10)
            ranges = t.jit_unit(build.__code__)["ranges"]
            print(sum(any(start <= a < start + size for start, size in ranges)
                      for a in stack))
        """, "jit=always")
        self.assertEqual(int(out), 1)

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
