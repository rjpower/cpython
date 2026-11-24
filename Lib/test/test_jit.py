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


@unittest.skipIf(_testinternalcapi is None, "needs _testinternalcapi")
class TestPins(unittest.TestCase):
    """The execution pin (g3/design.md section 2): a departure from a frame
    keeps the frame's code alive in the evaluation loop's activation until
    the next pin, jit_run()'s release after the native call, or the loop's
    exit, whichever comes first; nothing in one activation releases
    another's pin.  _testinternalcapi.jit_pin_departing() pins as compiled
    code does."""

    # The interpreter, the function forms and both JIT modes
    MODES = (("jit=off",), ("ir_forms",)) + (
        (("jit=on",), ("jit=always",)) if AVAILABLE else ())

    PRELUDE = """
        import _testinternalcapi as t, operator, weakref
        log = []
        def g(x):
            return x * 2
        def callback(ref):
            # Python runs inside the release: compiled code is called
            log.append(("callback", [g(i) for i in range(3)]))
        def fresh_code():
            # A code that only what the caller keeps refers to
            ns = {}
            exec("def f(): pass", ns)
            return ns.pop("f").__code__
    """

    def check_modes(self, source, expected=None):
        """The script prints the same in every mode, or what expected
        (a function of the mode's options) says."""
        source = textwrap.dedent(self.PRELUDE) + textwrap.dedent(source)
        for options in self.MODES:
            with self.subTest(options=options):
                want = expected(options) if expected else "ok\n"
                self.assertEqual(run(source, *options), want)

    def test_the_next_pin_releases_the_old_one(self):
        # One call of the hook pins each item in turn, so that no release by
        # jit_run() comes between the pins under the JIT
        self.check_modes("""
            def run():
                code = fresh_code()
                ref = weakref.ref(code, callback)
                items = [code, None, run.__code__]
                del code
                # The code; the pinned code again by the pin's own and last
                # reference (stored before the old pin is released, so it
                # lives); another code, whose pin releases it: the callback
                # runs inside that pin, which reports ESCAPED
                log.append(t.jit_pin_departing(items))
                log.append(ref() is None)
            for i in range(40):
                run()
                assert log == [("callback", [0, 2, 4]), [0, 0, 2], True], (i, log)
                log.clear()
            print("ok")
        """)

    def test_a_nested_loop_releases_only_its_own_pin(self):
        self.check_modes("""
            def inner():
                # From C: an evaluation loop of its own, whose pin this is
                t.jit_pin_departing()
                log.append("inner")
            def run():
                code = fresh_code()
                ref = weakref.ref(code, callback)
                items = [code, inner, run.__code__]
                del code
                # The outer pin outlives the nested loop's exit: the last
                # pin releases it
                log.append(t.jit_pin_departing(items))
            for i in range(40):
                run()
                assert log == ["inner", ("callback", [0, 2, 4]), [0, None, 2]], (i, log)
                log.clear()
            print("ok")
        """)

    def test_the_departed_code_lives_until_a_release(self):
        # f drops its function's reference to its code and pins it: once
        # its frame is gone the pin holds the code.  From C, f runs in a
        # loop of its own, whose exit (or jit_run()) releases the pin before
        # the call returns.  Called directly, the code lives on until the
        # next pin, unless jit_run() ran f (under always) and released it.
        def expected(options):
            if options == ("jit=always",):
                return "returning dead\nreturning dead \n\n"
            return "returning dead\nreturning alive\ndead\n"
        self.check_modes("""
            def make():
                ns = {"t": t, "log": log}
                exec("def f(self):\\n"
                     "    self.__code__ = (lambda self: None).__code__\\n"
                     "    t.jit_pin_departing()\\n"
                     "    log.append('returning')\\n", ns)
                f = ns.pop("f")
                return f, weakref.ref(f.__code__, lambda r: log.append("dead"))
            f, ref = make()
            operator.call(f, f)
            print(*log)
            log.clear()
            f, ref = make()
            f(f)
            print(*log, "alive" if ref() is not None else "")
            log.clear()
            t.jit_pin_departing()
            print(*log)
        """, expected)

    def test_pinned_codes_come_and_go(self):
        # Assert builds check at every free that no activation is running a
        # unit of the code
        self.check_modes("""
            for i in range(300):
                ns = {"t": t}
                exec(f"def f(n):\\n"
                     f"    t.jit_pin_departing()\\n"
                     f"    return n + {i} if n < 3 else f(n - 1)\\n", ns)
                f = ns["f"]
                if i % 2:
                    t.jit_compile(f.__code__)
                assert f(5) == 2 + i
                del f, ns
            print("ok")
        """)


@unittest.skipIf(_testinternalcapi is None, "needs _testinternalcapi")
class TestCompletionStubs(JitTestCase):
    """Whole-word completion stubs (g1a/abi.md): UNPACK_SEQUENCE's arms and
    BINARY_OP's generic are calls of generated C stubs, BINARY_OP's add arms
    are templates whose misses run the family's generic stub.  The inline,
    stub and generic paths give the interpreter's results, exceptions,
    slots and counts, and a finalizer that a release runs sees its events
    from the next word on."""

    def test_unpack_arms_and_generic(self):
        # Lists, tuples, other iterables, length failures and non-iterables
        # at one site: the list and tuple arms, their misses to the generic
        # stub, and the generic's raise
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
        # The add templates, their misses to BINARY_OP's generic stub (the
        # operation from the word's entry: + and +=), overflow, and the
        # generic stub's results and raises for other operations
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
        # A __del__ turns LINE events on: run by the generic unpack stub's
        # release of its iterable (the stub reports ESCAPED and the site's
        # cold path finds the unit retired), by the close of a local that an
        # unpack's store displaces, and by an add's displaced local (the
        # template's release slow path).  The compiled code leaves at the
        # next word and the events are the interpreter's from there.
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
        # the release of an unpacked iterable (the generic stub).  The
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
        # word runs again through the generic stub with the right result,
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
            print([n for n in names if n.startswith("UNPACK_SEQUENCE")])
            u = t.jit_unit(f.__code__)
            print(u["compiles"], u["invalid"])
            print(f(3, [5, 6]), t.jit_unit(f.__code__)["compiles"])
        """, "jit=always")
        lines = out.splitlines()
        self.assertEqual(lines[0], str(200 * 12))
        self.assertEqual(lines[1], "['UNPACK_SEQUENCE_LIST']")
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
            print([n for n in names if n.startswith("UNPACK_SEQUENCE")])
            u = t.jit_unit(f.__code__)
            print(u["compiles"], u["invalid"])
        """, "jit=always")
        lines = out.splitlines()
        self.assertGreaterEqual(int(lines[0]), 8)
        self.assertEqual(lines[1], str(200 * 12))
        self.assertEqual(lines[2], "['UNPACK_SEQUENCE_LIST']")
        compiles, invalid = map(int, lines[3].split())
        self.assertEqual(compiles, int(lines[0]))
        self.assertEqual(invalid, 0)


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

    def test_frame_states_the_code_exactly(self):
        function, unit = self.unit_of("""
            def f(n, total=0):
                for i in range(n):
                    total += i
                return total
        """)
        ctypes = import_helper.import_module("ctypes")
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
