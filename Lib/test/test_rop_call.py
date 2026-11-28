"""Calls and returns through the register instructions Call and Return.

Each test forces an arm on the word of a stack op
(_testinternalcapi.force_arm): the operands of the stack op's word are the
family's, so the arm runs on them as the translator will select it.  The
family's definitions are Python/register_ops/calls.rop.
"""

import dis
import functools
import gc
import sys
import unittest
import weakref

from test.support import cpython_only, import_helper

_testinternalcapi = import_helper.import_module("_testinternalcapi")


def word_names(code):
    return [row[0].split("+")[0] for row in _testinternalcapi.get_ir(code, True)]


def force(function, opname, arm, which=0):
    """Run arm on the word of the which-th stack op opname of function"""
    offsets = [i.offset for i in dis.get_instructions(function)
               if i.opname == opname]
    _testinternalcapi.force_arm(function.__code__, offsets[which], arm)


def warm(function, *args):
    for _ in range(100):
        function(*args)


class Tracker:
    deaths = []

    def __init__(self, name):
        self.name = name

    def __del__(self):
        Tracker.deaths.append(self.name)


@cpython_only
class ReturnTests(unittest.TestCase):
    def setUp(self):
        Tracker.deaths.clear()

    def test_ordinary(self):
        def f(x, y):
            z = [x, y]
            return z[0] + z[1]
        warm(f, 1, 2)
        force(f, "RETURN_VALUE", "Return.Ordinary")
        self.assertEqual(f(1, 2), 3)
        self.assertEqual(f("a", "b"), "ab")

    def test_ordinary_returns_a_local(self):
        def f(x):
            y = x
            return y
        warm(f, 1)
        force(f, "RETURN_VALUE", "Return.Ordinary")
        value = object()
        before = sys.getrefcount(value)
        self.assertIs(f(value), value)
        self.assertEqual(sys.getrefcount(value), before)

    def test_empty(self):
        def f(x):
            return x * 2
        warm(f, 1)
        self.assertIn("Return.Empty", word_names(f.__code__))
        force(f, "RETURN_VALUE", "Return.Empty")
        self.assertEqual(f(21), 42)

    def test_finalizer_of_the_dying_frame(self):
        seen = []

        class Dying:
            def __del__(self):
                seen.append(sys._getframe(1).f_code.co_name)

        def f():
            local = Dying()
            return 7
        warm(f)
        seen.clear()
        force(f, "RETURN_VALUE", "Return.Ordinary")
        self.assertEqual(f(), 7)
        self.assertEqual(len(seen), 1)

    def test_finalizer_displaced_in_the_caller(self):
        def g(n):
            return n + 1

        def f():
            x = Tracker("old")
            x = g(1)
            return x
        warm(f)
        force(g, "RETURN_VALUE", "Return.Ordinary")
        Tracker.deaths.clear()
        self.assertEqual(f(), 2)
        self.assertEqual(Tracker.deaths, ["old"])

    def test_generator(self):
        def gen(n):
            yield n
            return n * 2
        list(gen(1))
        for arm in ("Return.Generator", "Return.GeneratorEmpty"):
            force(gen, "RETURN_VALUE", arm)
            g = gen(3)
            self.assertEqual(next(g), 3)
            with self.assertRaises(StopIteration) as cm:
                next(g)
            self.assertEqual(cm.exception.value, 6)

        def outer():
            result = yield from gen(5)
            return result
        self.assertEqual(list(outer()), [5])
        o = outer()
        next(o)
        with self.assertRaises(StopIteration) as cm:
            next(o)
        self.assertEqual(cm.exception.value, 10)
        self.assertEqual([x for x in gen(4)], [4])

    def test_frame_object_after_return(self):
        frames = []

        def f(a):
            frames.append(sys._getframe())
            b = a + 1
            return b
        warm(f, 1)
        frames.clear()
        force(f, "RETURN_VALUE", "Return.Ordinary")
        self.assertEqual(f(1), 2)
        frame_locals = frames[0].f_locals
        self.assertEqual((frame_locals["a"], frame_locals["b"]), (1, 2))


@cpython_only
class CallExactTests(unittest.TestCase):
    def setUp(self):
        Tracker.deaths.clear()

    def test_exact(self):
        def g(a, b):
            return a - b

        def f(x):
            return g(x, 1)
        warm(f, 1)
        self.assertIn("Call.Exact", word_names(f.__code__))
        force(f, "CALL", "Call.Exact")
        self.assertEqual(f(10), 9)
        self.assertEqual(f(3), 2)

    def test_exact_with_self(self):
        class C:
            def m(self, a):
                return (self, a)

        def f(c):
            return c.m(5)
        c = C()
        warm(f, c)
        force(f, "CALL", "Call.Exact")
        self.assertEqual(f(c), (c, 5))

    def test_arguments_move_and_locals_stay(self):
        def g(a, b):
            return b

        def f(x):
            y = [x]
            return g(y, [x])
        warm(f, 1)
        force(f, "CALL", "Call.Exact")
        value = object()
        before = sys.getrefcount(value)
        self.assertEqual(f(value), [value])
        self.assertEqual(sys.getrefcount(value), before)

    def test_destination_was_the_function(self):
        def g(a):
            return a * 3

        def f(h, x):
            h = h(x)
            return h
        warm(f, g, 2)
        force(f, "CALL", "Call.Exact")
        self.assertEqual(f(g, 2), 6)

    def test_displaced_destination_finalizer(self):
        def g():
            return 1

        def f():
            x = Tracker("displaced")
            x = g()
            return x
        warm(f)
        force(f, "CALL", "Call.Exact", which=1)
        Tracker.deaths.clear()
        self.assertEqual(f(), 1)
        self.assertEqual(Tracker.deaths, ["displaced"])

    def test_exception_in_the_callee(self):
        def g(a):
            return 1 // a

        def f(x):
            return g(x)
        warm(f, 1)
        force(f, "CALL", "Call.Exact")
        try:
            f(0)
        except ZeroDivisionError as exc:
            tb = exc.__traceback__
        else:
            self.fail("no ZeroDivisionError")
        names = []
        while tb is not None:
            names.append(tb.tb_frame.f_code.co_name)
            tb = tb.tb_next
        self.assertEqual(names[-2:], ["f", "g"])

    def test_exact_and_return_together(self):
        def g(a):
            b = a + 1
            return b

        def f(x):
            return g(x)
        warm(f, 1)
        force(f, "CALL", "Call.Exact")
        force(g, "RETURN_VALUE", "Return.Ordinary")
        self.assertEqual(f(4), 5)


@cpython_only
class CallBoundExactTests(unittest.TestCase):
    def test_bound_method(self):
        class C:
            def m(self, a):
                return (self, a)

        def f(m):
            return m(5)
        c = C()
        warm(f, c.m)
        force(f, "CALL", "Call.BoundExact")
        before = sys.getrefcount(c)
        self.assertEqual(f(c.m), (c, 5))
        self.assertEqual(sys.getrefcount(c), before)

    def test_misses_to_generic(self):
        class C:
            def m(self, a):
                return a + 1

        def g(a):
            return a * 2

        def f(m):
            return m(5)
        c = C()
        warm(f, c.m)
        force(f, "CALL", "Call.BoundExact")
        self.assertEqual(f(g), 10)
        self.assertEqual(f(lambda a: -a), -5)
        self.assertEqual(f(c.m), 6)

    def test_weakly_referenced_method_misses(self):
        class C:
            def m(self, a):
                return a

        def f(m):
            return m(1)
        c = C()
        warm(f, c.m)
        force(f, "CALL", "Call.BoundExact")
        m = c.m
        r = weakref.ref(m)
        self.assertEqual(f(m), 1)
        self.assertIs(r(), m)


@cpython_only
class CallPyGeneralTests(unittest.TestCase):
    """Call.PyGeneral binds a function whose arguments the exact arms do not
    take, by the site's plan"""

    def test_defaults_star_and_keyword_only(self):
        def g(a, b=2, *rest, c=3, **extra):
            return (a, b, rest, c, extra)

        def f():
            return g(1), g(1, 5), g(1, 5, 6, 7)
        warm(f)
        self.assertIn("Call.PyGeneral", word_names(f.__code__))
        expected = ((1, 2, (), 3, {}), (1, 5, (), 3, {}),
                    (1, 5, (6, 7), 3, {}))
        self.assertEqual(f(), expected)

    def test_the_plan_rebinds_after_the_defaults_change(self):
        def g(a, b=2):
            return a + b

        def f():
            return g(1)
        warm(f)
        self.assertIn("Call.PyGeneral", word_names(f.__code__))
        self.assertEqual(f(), 3)
        g.__defaults__ = (10,)
        self.assertEqual(f(), 11)
        g.__defaults__ = ()
        with self.assertRaises(TypeError):
            f()
        g.__defaults__ = (2,)
        self.assertEqual(f(), 3)

    def test_a_different_function_misses(self):
        def g(a, b=2):
            return a + b

        def h(a, b=7):
            return a * b

        def f(fn):
            return fn(3)
        warm(f, g)
        self.assertEqual(f(g), 5)
        self.assertEqual(f(h), 21)
        self.assertEqual(f(g), 5)

    def test_the_operands_are_released(self):
        Tracker.deaths.clear()

        def g(a, b=None):
            return a

        def f():
            return g(Tracker("arg"))
        warm(f)
        Tracker.deaths.clear()
        result = f()
        del result
        self.assertEqual(Tracker.deaths, ["arg"])

    def test_a_binding_error_raises(self):
        def g(a, b=2, *, c):
            return a

        def f():
            return g(1)
        for _ in range(100):
            with self.assertRaises(TypeError):
                f()

    def test_a_deep_recursion_raises(self):
        def g(n, step=1):
            return g(n + step)

        for _ in range(3):
            with self.assertRaises(RecursionError):
                g(0)

    def test_a_generator_function_takes_the_plan(self):
        def g(a, b=2, *rest):
            yield (a, b, rest)

        def f():
            return list(g(1)), list(g(1, 5, 6))
        warm(f)
        self.assertIn("Call.PlanGenerator", word_names(f.__code__))
        self.assertEqual(f(), ([(1, 2, ())], [(1, 5, (6,))]))

    def test_a_generator_plan_rebinds_after_the_defaults_change(self):
        def g(a, b=2):
            yield a + b

        def f():
            return list(g(1))
        warm(f)
        self.assertIn("Call.PlanGenerator", word_names(f.__code__))
        g.__defaults__ = (10,)
        self.assertEqual(f(), [11])
        g.__defaults__ = ()
        with self.assertRaises(TypeError):
            f()

    def test_the_generator_owns_the_operands(self):
        Tracker.deaths.clear()

        def g(a, b=None):
            yield a

        def f():
            return g(Tracker("arg"))
        warm(f)
        Tracker.deaths.clear()
        generator = f()
        self.assertEqual(Tracker.deaths, [])
        del generator
        self.assertEqual(Tracker.deaths, ["arg"])


@cpython_only
class CallBoundGeneralTests(unittest.TestCase):
    """Call.BoundGeneral takes a bound method that Call.BoundExact does not:
    one whose function needs a general binding, or creates a generator"""

    def test_a_method_with_defaults_enters(self):
        class C:
            def m(self, a, b=2, *rest):
                return (self, a, b, rest)

        c = C()
        m = c.m

        def f():
            return m(1), m(1, 5, 6)
        warm(f)
        self.assertIn("Call.BoundGeneral", word_names(f.__code__))
        self.assertEqual(f(), ((c, 1, 2, ()), (c, 1, 5, (6,))))

    def test_a_generator_method_creates_the_generator(self):
        class C:
            def m(self, a):
                yield (self, a)

        c = C()
        m = c.m

        def f():
            return list(m(4))
        warm(f)
        self.assertIn("Call.BoundGeneral", word_names(f.__code__))
        self.assertEqual(f(), [(c, 4)])

    def test_a_generator_method_with_defaults(self):
        class C:
            def m(self, a, b=3):
                yield a + b

        m = C().m

        def f():
            return list(m(4))
        warm(f)
        self.assertIn("Call.BoundGeneral", word_names(f.__code__))
        self.assertEqual(f(), [7])

    def test_a_binding_error_raises(self):
        class C:
            def m(self, a, b=2, *, c):
                return a

        m = C().m

        def f():
            return m(1)
        for _ in range(100):
            with self.assertRaises(TypeError):
                f()

    def test_another_method_misses(self):
        class C:
            def m(self, a, b=2):
                return a + b

            def n(self, a, b=5):
                return a * b

        c = C()

        def f(m):
            return m(3)
        warm(f, c.m)
        self.assertEqual(f(c.m), 5)
        self.assertEqual(f(c.n), 15)
        self.assertEqual(f(c.m), 5)

    def test_the_operands_are_released(self):
        Tracker.deaths.clear()

        class C:
            def m(self, a, b=None):
                return a

        m = C().m

        def f():
            return m(Tracker("arg"))
        warm(f)
        Tracker.deaths.clear()
        result = f()
        del result
        self.assertEqual(Tracker.deaths, ["arg"])


class Point:
    def __init__(self, x, y):
        self.x = x
        self.y = y


@cpython_only
class CallInitTests(unittest.TestCase):
    def test_adapts_to_init(self):
        def f(a, b):
            return Point(a, b)
        warm(f, 1, 2)
        force(f, "CALL", "Call.Generic")
        warm(f, 1, 2)
        self.assertIn("Call.Init", word_names(f.__code__))
        p = f(3, 4)
        self.assertEqual((p.x, p.y), (3, 4))
        value = object()
        before = sys.getrefcount(value)
        p = f(value, 0)
        self.assertIs(p.x, value)
        del p
        self.assertEqual(sys.getrefcount(value), before)

    def test_init_returns_not_none(self):
        class Bad:
            def __init__(self):
                return 1

        def f():
            return Bad()
        force(f, "CALL", "Call.Generic")
        for _ in range(3):
            with self.assertRaisesRegex(TypeError, "should return None, not 'int'"):
                f()
        warm(lambda: self.assertRaises(TypeError, f))
        self.assertIn("Call.Init", word_names(f.__code__))
        with self.assertRaisesRegex(TypeError, "should return None, not 'int'"):
            f()

    def test_init_raises(self):
        class Raises:
            def __init__(self, fail):
                if fail:
                    raise ValueError("no")

        def f(fail):
            return Raises(fail)
        force(f, "CALL", "Call.Generic")
        warm(f, False)
        self.assertIn("Call.Init", word_names(f.__code__))
        with self.assertRaisesRegex(ValueError, "no"):
            f(True)
        self.assertIsInstance(f(False), Raises)

    def test_class_changes(self):
        class C:
            def __init__(self, a):
                self.a = a

        def f(a):
            return C(a)
        force(f, "CALL", "Call.Generic")
        warm(f, 1)
        C.__init__ = lambda self, a: setattr(self, "a", -a)
        self.assertEqual(f(2).a, -2)


@cpython_only
class CallGenericTests(unittest.TestCase):
    def setUp(self):
        Tracker.deaths.clear()

    def test_native(self):
        def f(x):
            return len(x)
        force(f, "CALL", "Call.Generic")
        value = [1, 2, 3]
        before = sys.getrefcount(value)
        self.assertEqual(f(value), 3)
        self.assertEqual(sys.getrefcount(value), before)

    def test_native_temporaries_are_released(self):
        def f(x):
            return isinstance([x], list)
        force(f, "CALL", "Call.Generic")
        value = object()
        before = sys.getrefcount(value)
        self.assertTrue(f(value))
        self.assertEqual(sys.getrefcount(value), before)

    def test_native_raises(self):
        def f(x):
            return int([x][0])
        force(f, "CALL", "Call.Generic", which=0)
        with self.assertRaises(ValueError):
            f("x")
        value = "not a number"
        before = sys.getrefcount(value)
        with self.assertRaises(ValueError):
            f(value)
        self.assertEqual(sys.getrefcount(value), before)

    def test_destination_is_an_operand(self):
        def f(x):
            x = str(x)
            return x
        force(f, "CALL", "Call.Generic")
        self.assertEqual(f(12), "12")

    def test_general_binding(self):
        def g(a, b=2, *rest, c=3, **kw):
            return (a, b, rest, c, kw)

        def f(x):
            return g(x)

        def h(x):
            return g(x, 1, 2, 3)
        force(f, "CALL", "Call.Generic")
        force(h, "CALL", "Call.Generic")
        self.assertEqual(f(1), (1, 2, (), 3, {}))
        self.assertEqual(h(1), (1, 1, (2, 3), 3, {}))

    def test_binding_error(self):
        def g(a):
            return a

        def f():
            return g()
        force(f, "CALL", "Call.Generic")
        with self.assertRaises(TypeError):
            f()

    def test_generator_function(self):
        def gen(n):
            yield n
            yield n + 1

        def f(n):
            return gen(n)
        force(f, "CALL", "Call.Generic")
        self.assertEqual(list(f(3)), [3, 4])

    def test_bound_method_with_self(self):
        class C:
            def m(self, a, b=1):
                return a + b

        def f(c):
            return c.m(5)
        c = C()
        force(f, "CALL", "Call.Generic")
        self.assertEqual(f(c), 6)

    def test_displaced_destination(self):
        def f():
            x = Tracker("displaced")
            x = abs(-1)
            return x
        force(f, "CALL", "Call.Generic", which=1)
        self.assertEqual(f(), 1)
        self.assertEqual(Tracker.deaths, ["displaced"])

    def test_adapts_to_exact(self):
        def g(a, b):
            return a + b

        def f(x):
            return g(x, 1)
        force(f, "CALL", "Call.Generic")
        warm(f, 1)
        self.assertIn("Call.Exact", word_names(f.__code__))
        self.assertEqual(f(2), 3)

    def test_adapts_to_bound_exact(self):
        class C:
            def m(self, a):
                return a

        def f(m):
            return m(4)
        force(f, "CALL", "Call.Generic")
        warm(f, C().m)
        self.assertIn("Call.BoundExact", word_names(f.__code__))
        self.assertEqual(f(C().m), 4)

    def test_native_adapts_to_its_arm(self):
        def f(x):
            return len(x)
        force(f, "CALL", "Call.Generic")
        warm(f, "ab")
        self.assertIn("Call.Len", word_names(f.__code__))

    def test_exact_closure(self):
        # The site's entry says the callee has free variables
        k = 3

        def g(a):
            return a + k

        def f(x):
            return g(x)
        force(f, "CALL", "Call.Generic")
        warm(f, 1)
        self.assertIn("Call.Exact", word_names(f.__code__))
        self.assertEqual(f(1), 4)

    def test_exact_misses_when_the_self_is_absent(self):
        # Classified with the receiver present: the same function as an
        # instance attribute takes one argument fewer at the same site
        def g(a, b):
            return (a, b)

        class C:
            m = g

        def f(o):
            return o.m(1)
        c = C()
        force(f, "CALL", "Call.Generic")
        warm(f, c)
        self.assertIn("Call.Exact", word_names(f.__code__))
        self.assertEqual(f(c), (c, 1))
        d = C()
        d.m = g
        with self.assertRaises(TypeError):
            f(d)
        self.assertEqual(f(c), (c, 1))

    def test_exact_misses_after_redefinition(self):
        def g(a):
            return a

        def f(x):
            return g(x)
        force(f, "CALL", "Call.Generic")
        warm(f, 1)
        self.assertIn("Call.Exact", word_names(f.__code__))
        g.__code__ = (lambda a: -a).__code__
        self.assertEqual(f(1), -1)

    def test_recursion_limit(self):
        def r(n):
            return r(n - 1) if n else 0
        force(r, "CALL", "Call.Generic")
        self.assertEqual(r(50), 0)
        with self.assertRaises(RecursionError):
            r(10 ** 6)
        force(r, "CALL", "Call.Exact")
        self.assertEqual(r(50), 0)
        with self.assertRaises(RecursionError):
            r(10 ** 6)


def target(a, b):
    return a + b


def generator(n):
    yield n
    return n + 1


class UninterruptedTests(unittest.TestCase):
    """The call after a special-method lookup does not poll: the Call family's
    declared interrupt operand (IR flag bit 15) is set on that word only"""

    @cpython_only
    def test_flag_is_set_after_a_special_lookup_only(self):
        import _testinternalcapi as capi

        class Manager:
            def __enter__(self):
                return self
            def __exit__(self, *args):
                return False

        def with_call(m):
            with m:
                pass

        def plain_call(f):
            return f()

        def calls(fn):
            return [row[0] for row in capi.get_ir(fn.__code__, True)
                    if row[0].startswith("Call.")]

        with_call(Manager())
        plain_call(dict)
        self.assertTrue(any("+UNINTERRUPTED" in n for n in calls(with_call)),
                        calls(with_call))
        self.assertFalse(any("+UNINTERRUPTED" in n for n in calls(plain_call)),
                         calls(plain_call))

    def test_with_still_runs(self):
        events = []

        class Manager:
            def __enter__(self):
                events.append("enter")
                return self
            def __exit__(self, *args):
                events.append("exit")
                return False

        for _ in range(3):
            with Manager():
                events.append("body")
        self.assertEqual(events, ["enter", "body", "exit"] * 3)


class Spot:
    def __init__(self, x):
        self.x = x

    def get(self):
        return self.x


def sample():
    total = target(1, 2)
    total += len([1])
    total += Spot(3).get()
    bound = Spot(4).get
    total += bound()
    try:
        len(5)
    except TypeError:
        pass
    return total + sum(generator(4))


SAMPLE_EVENTS = [
    ("PY_START", "sample"),
    ("CALL", "sample", "target", 1),
    ("PY_START", "target"),
    ("PY_RETURN", "target", 3),
    ("CALL", "sample", "len", [1]),
    ("C_RETURN", "sample", "len", [1]),
    ("CALL", "sample", "Spot", 3),
    ("PY_START", "__init__"),
    ("PY_RETURN", "__init__", None),
    ("C_RETURN", "sample", "Spot", 3),
    ("CALL", "sample", "get", "Spot"),
    ("PY_START", "get"),
    ("PY_RETURN", "get", 3),
    ("CALL", "sample", "Spot", 4),
    ("PY_START", "__init__"),
    ("PY_RETURN", "__init__", None),
    ("C_RETURN", "sample", "Spot", 4),
    ("CALL", "sample", "get", "Spot"),
    ("PY_START", "get"),
    ("PY_RETURN", "get", 4),
    ("CALL", "sample", "len", 5),
    ("C_RAISE", "sample", "len", 5),
    ("CALL", "sample", "generator", 4),
    ("CALL", "sample", "sum", "generator"),
    ("PY_START", "generator"),
    ("PY_RETURN", "generator", 5),
    ("C_RETURN", "sample", "sum", "generator"),
    ("PY_RETURN", "sample", 15),
]


def shown(value):
    if isinstance(value, (int, list)) or value is None:
        return value
    return getattr(value, "__name__", type(value).__name__)


class Monitor:
    """Records the call and return events of the code of the given
    functions, by event name"""
    EVENTS = ("PY_START", "PY_RETURN", "CALL", "C_RETURN", "C_RAISE")

    def __init__(self, test, raising=None, callee=None, snapshot=None):
        self.log = []
        # Raise in the callbacks of raising (for a callee, its calls only)
        self.raising = raising
        self.callee = callee
        # A function whose words to list when target starts
        self.snapshot = snapshot
        self.words = None
        self.observed = None
        self.tool = sys.monitoring.DEBUGGER_ID
        sys.monitoring.use_tool_id(self.tool, "test_rop_call")
        test.addCleanup(sys.monitoring.free_tool_id, self.tool)
        test.addCleanup(sys.monitoring.set_events, self.tool, 0)
        mask = 0
        for name in self.EVENTS:
            bit = getattr(sys.monitoring.events, name)
            mask |= bit
            sys.monitoring.register_callback(
                self.tool, bit, functools.partial(self.record, name))
            test.addCleanup(sys.monitoring.register_callback, self.tool, bit, None)
        self.mask = mask

    def record(self, name, code, offset, *args):
        if code.co_filename != __file__ or code in self.OWN:
            return
        if name in ("CALL", "C_RETURN", "C_RAISE"):
            event = (name, code.co_name, shown(args[0]), shown(args[1]))
        elif name == "PY_RETURN":
            event = (name, code.co_name, shown(args[0]))
        else:
            event = (name, code.co_name)
        self.log.append(event)
        if name == "PY_START" and code is target.__code__ and self.snapshot:
            self.words = word_names(self.snapshot.__code__)
            self.observed = _testinternalcapi.code_is_observed(
                self.snapshot.__code__)
        if self.raising == name and self.callee in (None, event[2]):
            raise KeyError(name)

    def run(self, function, *args):
        # The calls of run and of the callbacks are the monitor's own
        sys.monitoring.set_events(self.tool, self.mask)
        try:
            return function(*args)
        finally:
            sys.monitoring.set_events(self.tool, 0)


Monitor.OWN = frozenset({Monitor.run.__code__, Monitor.record.__code__,
                         Monitor.__init__.__code__})


@cpython_only
class ObservationTests(unittest.TestCase):
    """sys.monitoring and sys.settrace on the words of Call and Return: the
    observed wrappers Call.Observed and Return.Observed fire the events of
    the old CALL and RETURN_VALUE"""

    def test_monitoring_events_of_every_arm(self):
        warm(sample)
        names = word_names(sample.__code__)
        for arm in ("Call.Exact", "Call.BoundExact", "Call.Init",
                    "Call.Len", "Return.Empty"):
            self.assertIn(arm, names)
        self.assertIn("Return.GeneratorEmpty", word_names(generator.__code__))
        monitor = Monitor(self)
        self.assertEqual(monitor.run(sample), 15)
        self.assertEqual(monitor.log, SAMPLE_EVENTS)
        # Instrumentation starts each word again from the arm the translator
        # chose, as it deoptimized the old handlers; the words adapt again
        # once the events are off
        warm(sample)
        self.assertEqual(word_names(sample.__code__), names)

    def test_observed_wrappers_run_while_monitoring(self):
        # The code runs in the observing evaluator, whose table runs the
        # observers Call.Observed and Return.Observed for the words; the
        # words keep the arms that they adapted to
        warm(sample)
        names = word_names(sample.__code__)
        monitor = Monitor(self, snapshot=sample)
        monitor.run(sample)
        self.assertTrue(monitor.observed)
        # Under the JIT the head is Resume.Poll while the code's unit
        # publishes its entries, and Resume.Warm once monitoring retires it
        self.assertEqual(monitor.words[1:], names[1:])
        calls = [n for n in monitor.words if n.startswith("Call.")]
        self.assertEqual(len(calls), 9)

    def test_forced_arms_under_monitoring(self):
        def g(a):
            return a
        class K:
            def __init__(self, v):
                self.v = v
            def m(self):
                return self.v
        def f(k):
            b = k.m
            return g(1) + K(2).v + b()
        k = K(3)
        warm(f, k)
        self.assertEqual(
            [n for n in word_names(f.__code__) if n.startswith("Call.")],
            ["Call.Exact", "Call.Init", "Call.BoundExact"])
        monitor = Monitor(self)
        for which, arm in enumerate(("Call.Exact", "Call.Init",
                                     "Call.BoundExact")):
            force(f, "CALL", arm, which)
        self.assertEqual(monitor.run(f, k), 6)
        self.assertEqual(monitor.log, [
            ("PY_START", "f"),
            ("CALL", "f", "g", 1), ("PY_START", "g"), ("PY_RETURN", "g", 1),
            ("CALL", "f", "K", 2), ("PY_START", "__init__"),
            ("PY_RETURN", "__init__", None), ("C_RETURN", "f", "K", 2),
            ("CALL", "f", "m", "K"), ("PY_START", "m"), ("PY_RETURN", "m", 3),
            ("PY_RETURN", "f", 6),
        ])

    def test_call_callback_raises(self):
        warm(sample)
        monitor = Monitor(self, raising="CALL")
        with self.assertRaises(KeyError):
            monitor.run(sample)
        self.assertEqual(sample(), 15)

    def test_return_callback_raises_in_the_returning_frame(self):
        def f(x):
            try:
                return [x]
            except KeyError:
                return "caught"
        warm(f, 1)
        monitor = Monitor(self, raising="PY_RETURN")
        # The event is the frame's last act: the frame's handlers do not
        # see its error, as with the old RETURN_VALUE
        with self.assertRaises(KeyError):
            monitor.run(f, 1)
        self.assertEqual(f(1), [1])

    def test_c_return_callback_raises(self):
        def f(x):
            try:
                return len(x)
            except KeyError:
                return -1
        warm(f, [1])
        monitor = Monitor(self, raising="C_RETURN")
        self.assertEqual(monitor.run(f, [1]), -1)
        self.assertEqual(f([1, 2]), 2)

    def test_operands_released_when_an_event_raises(self):
        def g(a):
            return a
        def f():
            return g(Tracker("arg"))
        warm(f)
        Tracker.deaths.clear()
        monitor = Monitor(self, raising="CALL", callee="g")
        with self.assertRaises(KeyError):
            monitor.run(f)
        gc.collect()
        self.assertEqual(Tracker.deaths, ["arg"])

    def test_settrace(self):
        warm(sample)
        events = []
        def tracer(frame, event, arg):
            if frame.f_code.co_filename == __file__:
                events.append((event, frame.f_code.co_name,
                               frame.f_lineno - frame.f_code.co_firstlineno))
            return tracer
        sys.settrace(tracer)
        try:
            sample()
        finally:
            sys.settrace(None)
        calls = [e for e in events if e[0] in ("call", "return")]
        self.assertEqual([e[:2] for e in calls], [
            ("call", "sample"),
            ("call", "target"), ("return", "target"),
            ("call", "__init__"), ("return", "__init__"),
            ("call", "get"), ("return", "get"),
            ("call", "__init__"), ("return", "__init__"),
            ("call", "get"), ("return", "get"),
            ("call", "generator"), ("return", "generator"),
            ("call", "generator"), ("return", "generator"),
            ("return", "sample"),
        ])
        lines = [e[2] for e in events if e[:2] == ("line", "sample")]
        self.assertEqual(lines, [1, 2, 3, 4, 5, 6, 7, 8, 9, 10])

    def test_line_event_survives_adaptation(self):
        # The call is the first word of its line: the line event's word runs
        # Call.Generic, whose adaptation must not replace the event's word
        def f(g):
            x = g(1, 2)
            return x

        def lines():
            seen = []
            def tracer(frame, event, arg):
                if frame.f_code is f.__code__ and event == "line":
                    seen.append(frame.f_lineno - f.__code__.co_firstlineno)
                return tracer
            sys.settrace(tracer)
            try:
                f(target)
            finally:
                sys.settrace(None)
            return seen

        for _ in range(4):
            self.assertEqual(lines(), [1, 2])

    def test_jump_over_a_call(self):
        seen = []
        def g():
            seen.append("g")
            return 1
        def f():
            x = 0
            x = g()
            return x
        warm(f)
        first = f.__code__.co_firstlineno
        def tracer(frame, event, arg):
            if frame.f_code is f.__code__ and event == "line" and \
                    frame.f_lineno == first + 2:
                frame.f_lineno = first + 3
            return tracer
        seen.clear()
        sys.settrace(tracer)
        try:
            result = f()
        finally:
            sys.settrace(None)
        self.assertEqual((result, seen), (0, []))
        self.assertEqual(f(), 1)

    def test_traceback_through_calls(self):
        def g(x):
            return 1 / x
        def f(x):
            return g(x)
        warm(f, 1)
        try:
            f(0)
        except ZeroDivisionError as e:
            names = []
            tb = e.__traceback__
            while tb is not None:
                names.append((tb.tb_frame.f_code.co_name,
                              tb.tb_lineno - tb.tb_frame.f_code.co_firstlineno))
                tb = tb.tb_next
        self.assertEqual(names[-2:], [("f", 1), ("g", 1)])


if __name__ == "__main__":
    unittest.main()
