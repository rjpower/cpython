"""Calls that bind from a per-site plan: defaults, keywords, **kwargs
dicts of CALL_FUNCTION_EX, generator callees, and the changes of a function
after its site specialized."""

import asyncio
import sys
import unittest

from test.support import cpython_only, requires_specialization, reset_code
from test.support.import_helper import import_module
from test.test_ir_attr_cache import handlers as ir_handlers

_testinternalcapi = import_module("_testinternalcapi")


def handlers(fn):
    # The call handlers carry a role suffix, as CALL_PY_GENERAL+NO_SELF.
    return [name.split("+")[0] for name in ir_handlers(fn)]


def warm(fn, *args):
    for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 2):
        result = fn(*args)
    return result


def outcome(fn, *args):
    try:
        return fn(*args)
    except TypeError as e:
        return "TypeError: " + str(e)


class PlanTestCase(unittest.TestCase):
    pass


@cpython_only
@requires_specialization
class TestDefaultsAndKeywords(PlanTestCase):
    def test_defaults_use_the_plan(self):
        def f(x, a=1, b=2):
            return (x, a, b)

        def call(x):
            return f(x)
        call = reset_code(call)
        self.assertEqual(warm(call, 0), (0, 1, 2))
        self.assertIn("CALL_PY_GENERAL", handlers(call))

    def test_keywords_use_the_plan(self):
        def f(x, a=1, b=2):
            return (x, a, b)

        def call(x):
            return f(x, b=9)
        call = reset_code(call)
        self.assertEqual(warm(call, 0), (0, 1, 9))
        self.assertIn("CALL_KW_PY", handlers(call))

    def test_defaults_changed_after_specialization(self):
        def f(x, a=1, b=2):
            return (x, a, b)

        def call(x):
            return f(x)

        def call_one(x):
            return f(x, 5)

        def call_kw(x):
            return f(x, b=9)
        call, call_one, call_kw = map(reset_code, (call, call_one, call_kw))
        for fn in (call, call_one, call_kw):
            warm(fn, 0)
        self.assertEqual(call(0), (0, 1, 2))
        f.__defaults__ = (10, 20)
        self.assertEqual(call(0), (0, 10, 20))
        self.assertEqual(call_one(0), (0, 5, 20))
        self.assertEqual(call_kw(0), (0, 10, 9))
        f.__defaults__ = None
        self.assertIn("missing 2 required positional arguments: 'a' and 'b'",
                      outcome(call, 0))
        self.assertIn("missing 1 required positional argument: 'b'",
                      outcome(call_one, 0))
        self.assertIn("missing 1 required positional argument: 'a'",
                      outcome(call_kw, 0))
        f.__defaults__ = (7,)
        self.assertIn("missing 1 required positional argument: 'a'",
                      outcome(call, 0))
        self.assertEqual(call_one(0), (0, 5, 7))
        f.__defaults__ = (1, 2, 3, 4)
        self.assertEqual(call(0), (0, 3, 4))
        f.__defaults__ = (1, 2)
        self.assertEqual(call(0), (0, 1, 2))

    def test_code_changed_after_specialization(self):
        def f(x, a=1, b=2):
            return (x, a, b)

        def g(x, y, z=3):
            return ("g", x, y, z)

        def h(x):
            return ("h", x)

        def call(x):
            return f(x)

        def call_one(x):
            return f(x, 5)
        call, call_one = map(reset_code, (call, call_one))
        warm(call, 0)
        warm(call_one, 0)
        f.__code__ = g.__code__
        self.assertEqual(call(0), ("g", 0, 1, 2))
        self.assertEqual(call_one(0), ("g", 0, 5, 2))
        f.__defaults__ = (3,)
        self.assertEqual(call_one(0), ("g", 0, 5, 3))
        f.__code__ = h.__code__
        f.__defaults__ = None
        self.assertEqual(call(0), ("h", 0))
        self.assertIn("takes 1 positional argument but 2 were given",
                      outcome(call_one, 0))

    def test_kwdefaults_changed_after_specialization(self):
        def f(x, *, a=1, b=2):
            return (x, a, b)

        def call(x):
            return f(x)

        def call_kw(x):
            return f(x, b=5)
        call, call_kw = map(reset_code, (call, call_kw))
        warm(call, 0)
        warm(call_kw, 0)
        f.__kwdefaults__ = {'a': 10, 'b': 20}
        self.assertEqual(call(0), (0, 10, 20))
        self.assertEqual(call_kw(0), (0, 10, 5))
        f.__kwdefaults__['a'] = 99
        self.assertEqual(call(0), (0, 99, 20))
        f.__kwdefaults__ = None
        self.assertIn("missing 2 required keyword-only arguments: 'a' and 'b'",
                      outcome(call, 0))
        self.assertIn("missing 1 required keyword-only argument: 'a'",
                      outcome(call_kw, 0))
        f.__kwdefaults__ = {'a': 1}
        self.assertIn("missing 1 required keyword-only argument: 'b'",
                      outcome(call, 0))
        self.assertEqual(call_kw(0), (0, 1, 5))

    def test_one_code_two_functions(self):
        def make(d):
            def inner(x, a=d):
                return (x, a)
            return inner
        functions = [make(i) for i in range(4)]

        def call(fn, x):
            return fn(x)
        call = reset_code(call)
        for i in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 2):
            call(functions[i % 4], 0)
        self.assertEqual([call(fn, 1) for fn in functions],
                         [(1, 0), (1, 1), (1, 2), (1, 3)])
        functions[2].__defaults__ = (100,)
        self.assertEqual([call(fn, 1) for fn in functions],
                         [(1, 0), (1, 1), (1, 100), (1, 3)])

    def test_one_code_different_default_counts(self):
        def two(x, a=1, b=2):
            return (x, a, b)

        def call(fn, x):
            return fn(x)
        call = reset_code(call)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 2):
            call(two, 0)
        import types
        code = two.__code__
        t2 = types.FunctionType(code, globals(), "t2", (5, 6))
        t3 = types.FunctionType(code, globals(), "t3", (6,))
        t4 = types.FunctionType(code, globals(), "t4", None)
        self.assertEqual(call(t2, 1), (1, 5, 6))
        self.assertIn("missing 1 required positional argument: 'a'",
                      outcome(call, t3, 1))
        self.assertIn("missing 2 required positional arguments: 'a' and 'b'",
                      outcome(call, t4, 1))

    def test_binding_errors(self):
        def e1(a, b, c=3, *, d, e=5):
            return (a, b, c, d, e)

        def e2(a, /, b, *, c):
            return (a, b, c)

        def e3(a, /, **kw):
            return (a, kw)

        def e4(a, b=2):
            return (a, b)

        def cases():
            return [
                outcome(lambda: e1(1, 2, d=1)),
                outcome(lambda: e1(1, d=1)),
                outcome(lambda: e1(1, 2)),
                outcome(lambda: e1(1, 2, 3, 4, d=1)),
                outcome(lambda: e1(1, 2, d=1, z=3)),
                outcome(lambda: e1(1, 2, 3, c=3, d=1)),
                outcome(lambda: e2(1, 2, c=3)),
                outcome(lambda: e2(a=1, b=2, c=3)),
                outcome(lambda: e3(1, a=2)),
                outcome(lambda: e3(a=1)),
                outcome(lambda: e4()),
                outcome(lambda: e4(1, 2, 3)),
                outcome(lambda: e4(1, a=2)),
                outcome(lambda: e4(1, c=2)),
            ]
        cold = cases()
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 2):
            hot = cases()
        self.assertEqual(hot, cold)
        self.assertEqual(hot[0], (1, 2, 3, 1, 5))
        self.assertIn("missing 1 required positional argument: 'b'", hot[1])
        self.assertIn("missing 1 required keyword-only argument: 'd'", hot[2])
        self.assertIn("got an unexpected keyword argument 'z'", hot[4])
        self.assertIn("got multiple values for argument 'c'", hot[5])
        self.assertEqual(hot[6], (1, 2, 3))
        self.assertIn("positional-only arguments passed as keyword arguments: 'a'",
                      hot[7])
        self.assertEqual(hot[8], (1, {'a': 2}))
        self.assertIn("got an unexpected keyword argument 'c'", hot[13])

    def test_defaults_with_star_and_kwargs(self):
        def f(a, b=2, *args, c, d=4, **kw):
            return (a, b, args, c, d, kw)

        def call():
            return [f(1, c=3), f(1, 2, 3, 4, c=3), f(1, c=3, z=9),
                    f(1, 5, 6, c=3, d=9, q=1)]
        call = reset_code(call)
        expected = [(1, 2, (), 3, 4, {}), (1, 2, (3, 4), 3, 4, {}),
                    (1, 2, (), 3, 4, {'z': 9}),
                    (1, 5, (6,), 3, 9, {'q': 1})]
        self.assertEqual(warm(call), expected)
        self.assertEqual(call(), expected)

    def test_method_with_default_and_self_skipped(self):
        class C:
            def m(self, x, a=1):
                return (self is not None, x, a)

            def plain(x, a=1):
                return (x, a)
            alias = plain

        c = C()

        def call():
            return c.m(5)

        def call_class_attribute():
            return c.alias(5)
        call, call_class_attribute = map(reset_code, (call, call_class_attribute))
        self.assertEqual(warm(call), (True, 5, 1))
        # c.alias is a bound method of plain: x is c
        self.assertEqual(warm(call_class_attribute)[1], 5)


@cpython_only
@requires_specialization
class TestKwargsDict(PlanTestCase):
    def test_empty_and_filled_dicts(self):
        def f(a, b=2, *args, c=3, **kw):
            return (a, b, args, c, dict(kw))

        def call(d):
            return f(1, **d)
        call = reset_code(call)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 2):
            call({})
        self.assertEqual(call({}), (1, 2, (), 3, {}))
        self.assertEqual(call({'b': 5}), (1, 5, (), 3, {}))
        self.assertEqual(call({'c': 7, 'z': 1}), (1, 2, (), 7, {'z': 1}))
        self.assertEqual(call({'b': 1, 'c': 2, 'x': 3, 'y': 4}),
                         (1, 1, (), 2, {'x': 3, 'y': 4}))
        self.assertIn("got multiple values for argument 'a'",
                      outcome(call, {'a': 8}))

    def test_str_subclass_keys(self):
        calls = []

        class S(str):
            pass

        class SE(str):
            def __eq__(self, other):
                calls.append("eq")
                return str.__eq__(self, other)
            __hash__ = str.__hash__

        class SH(str):
            def __hash__(self):
                calls.append("hash")
                return str.__hash__(self)

        def f(a, b=2):
            return (a, b)

        def g(a, **kw):
            return (a, sorted(kw))

        def call(d):
            return f(1, **d)

        def call_g(d):
            return g(1, **d)
        call, call_g = map(reset_code, (call, call_g))
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 2):
            call({'b': 3})
            call_g({'z': 1})
        self.assertEqual(call({S('b'): 8}), (1, 8))
        self.assertEqual(call({SE('b'): 8}), (1, 8))
        self.assertEqual(call({SH('b'): 8}), (1, 8))
        self.assertEqual(call_g({SH('zz'): 8}), (1, ['zz']))
        self.assertEqual(call_g({S('zz'): 8}), (1, ['zz']))

    def test_non_str_keys_and_mappings(self):
        def f(a, b=2):
            return (a, b)

        def g(**kw):
            return dict(kw)

        class M:
            def keys(self):
                return ['b']

            def __getitem__(self, k):
                return 42

        def call(d):
            return f(1, **d)

        def call_g(d):
            return g(**d)
        call, call_g = map(reset_code, (call, call_g))
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 2):
            call({'b': 3})
            call_g({'b': 3})
        self.assertIn("keywords must be strings", outcome(call, {1: 8}))
        self.assertIn("keywords must be strings", outcome(call_g, {1: 8}))
        self.assertEqual(call(M()), (1, 42))
        self.assertIn("must be a mapping", outcome(call, None))
        self.assertIn("got multiple values for keyword argument 'b'",
                      outcome(lambda: f(1, **{'b': 2}, **{'b': 3})))

    def test_callee_kwargs_is_a_new_dict(self):
        def mutate(**kw):
            kw['new'] = 1
            kw.pop('q', None)
            return kw

        def identity(a, **kw):
            return kw

        def call(d):
            return mutate(**d), identity(1, **d)
        call = reset_code(call)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 2):
            call({'q': 1})
        caller = {'q': 1}
        mutated, same = call(caller)
        self.assertEqual(mutated, {'new': 1})
        self.assertEqual(caller, {'q': 1})
        self.assertIsNot(same, caller)
        self.assertEqual(same, caller)
        empty = {}
        self.assertIsNot(identity(1, **empty), empty)

    def test_forwarding_through_a_bound_method(self):
        class Logger:
            def _inner(self, level, msg, args, exc_info=None, extra=None,
                       stack_info=False, stacklevel=1):
                return (level, msg, args, exc_info, extra, stack_info,
                        stacklevel)

            def warning(self, msg, *args, **kwargs):
                return self._inner(30, msg, args, **kwargs)

        lg = Logger()

        def call():
            return [lg.warning('m'), lg.warning('m', 1, 2),
                    lg.warning('m', extra='x', stacklevel=3)]
        call = reset_code(call)
        expected = [(30, 'm', (), None, None, False, 1),
                    (30, 'm', (1, 2), None, None, False, 1),
                    (30, 'm', (), None, 'x', False, 3)]
        self.assertEqual(warm(call), expected)
        self.assertEqual(call(), expected)
        with self.assertRaisesRegex(TypeError, "unexpected keyword argument 'bad'"):
            lg.warning('m', bad=1)

    def test_split_table_and_deleted_entries(self):
        def f(a, b=2, c=3, **kw):
            return (a, b, c, dict(kw))

        class O:
            pass
        o = O()
        o.b = 11
        o.c = 12
        o.z = 1
        d = {'b': 1, 'c': 2, 'd': 3}
        del d['c']

        def call(m):
            return f(1, **m)
        call = reset_code(call)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 2):
            call({'b': 3})
        self.assertEqual(call(o.__dict__), (1, 11, 12, {'z': 1}))
        self.assertEqual(call(d), (1, 1, 3, {'d': 3}))
        self.assertEqual(call({str(i): i for i in range(40)}),
                         (1, 2, 3, {str(i): i for i in range(40)}))

    def test_tuple_calls(self):
        def f(a, b=2, *args, c=3):
            return (a, b, args, c)

        def call(t):
            return f(*t)
        call = reset_code(call)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 2):
            call((1,))
        self.assertEqual(call((1,)), (1, 2, (), 3))
        self.assertEqual(call((1, 5, 6, 7)), (1, 5, (6, 7), 3))
        self.assertEqual(call([1, 5]), (1, 5, (), 3))
        self.assertIn("missing 1 required positional argument: 'a'",
                      outcome(call, ()))


@cpython_only
@requires_specialization
class TestGeneratorCallees(PlanTestCase):
    def test_generator_with_defaults_binds_into_the_generator(self):
        def gen(x, y=5):
            yield x
            yield y

        def call(x):
            return gen(x)
        call = reset_code(call)
        g = warm(call, 1)
        self.assertEqual(list(g), [1, 5])
        self.assertIn("CALL_PY_GENERAL", handlers(call))
        self.assertEqual(list(call(2)), [2, 5])

    def test_generator_keywords_and_kwargs(self):
        def gen(a, b=2, *args, c=3, **kw):
            yield (a, b, args, c, kw)

        def call():
            return [list(gen(1)), list(gen(1, 2, 3, c=4, z=5)),
                    list(gen(1, **{'z': 5}))]
        call = reset_code(call)
        expected = [[(1, 2, (), 3, {})], [(1, 2, (3,), 4, {'z': 5})],
                    [(1, 2, (), 3, {'z': 5})]]
        self.assertEqual(warm(call), expected)
        self.assertEqual(call(), expected)

    def test_coroutine_with_defaults(self):
        async def co(a, b=2, *args, c=3, **kw):
            return (a, b, args, c, kw)

        def call():
            return asyncio.run(co(1)), asyncio.run(co(1, 2, 3, c=1, q=2))
        call = reset_code(call)
        expected = ((1, 2, (), 3, {}), (1, 2, (3,), 1, {'q': 2}))
        self.assertEqual(warm(call), expected)

    def test_generator_defaults_changed(self):
        def gen(x, y=5):
            yield x
            yield y

        def call(x):
            return list(gen(x))
        call = reset_code(call)
        warm(call, 1)
        gen.__defaults__ = (50,)
        self.assertEqual(call(1), [1, 50])
        gen.__defaults__ = None
        self.assertIn("missing 1 required positional argument: 'y'",
                      outcome(call, 1))
        self.assertEqual(list(gen(1, 2)), [1, 2])

    def test_failed_binding_creates_no_generator(self):
        def gen(x, y=5):
            yield x

        def call():
            return gen()
        call = reset_code(call)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 2):
            self.assertIn("missing 1 required positional argument: 'x'",
                          outcome(call))

    def test_generator_name_and_type(self):
        def gen(x, y=5):
            yield x

        def call(x):
            return gen(x)
        call = reset_code(call)
        g = warm(call, 1)
        self.assertEqual((type(g).__name__, g.__name__), ("generator", "gen"))
        self.assertEqual(g.gi_code.co_name, "gen")


@cpython_only
@requires_specialization
class TestEvents(PlanTestCase):
    def test_monitoring_and_tracing_over_planned_calls(self):
        def d1(x, a=1):
            return x

        def d3(**kw):
            return kw

        def d4(x, y=3):
            yield x

        def run():
            d1(1)
            d1(1, a=2)
            d3(a=1)
            d3(**{'z': 1})
            list(d4(1))
            d1(*(1,), **{})
        run = reset_code(run)
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 2):
            run()
        events = []
        tool = 3
        monitoring = sys.monitoring
        monitoring.use_tool_id(tool, "bind-plan")
        self.addCleanup(monitoring.free_tool_id, tool)
        monitoring.register_callback(
            tool, monitoring.events.CALL,
            lambda code, offset, callable, arg0: events.append(
                ("CALL", getattr(callable, "__name__", "?"))))
        monitoring.register_callback(
            tool, monitoring.events.PY_START,
            lambda code, offset: events.append(("PY_START", code.co_name)))
        monitoring.set_events(
            tool, monitoring.events.CALL | monitoring.events.PY_START)
        try:
            run()
        finally:
            monitoring.set_events(tool, 0)
        names = [(kind, name) for kind, name in events
                 if name in ("d1", "d3", "d4")]
        self.assertEqual(names, [
            ("CALL", "d1"), ("PY_START", "d1"),
            ("CALL", "d1"), ("PY_START", "d1"),
            ("CALL", "d3"), ("PY_START", "d3"),
            ("CALL", "d3"), ("PY_START", "d3"),
            ("CALL", "d4"), ("PY_START", "d4"),
            ("CALL", "d1"), ("PY_START", "d1"),
        ])

        traced = []

        def tracer(frame, event, arg):
            if frame.f_code.co_name in ("d1", "d3", "d4"):
                traced.append((event, frame.f_code.co_name))
            return tracer
        sys.settrace(tracer)
        try:
            run()
        finally:
            sys.settrace(None)
        # a generator reports a call at each resume
        self.assertEqual([e for e in traced if e[0] == "call"],
                         [("call", "d1"), ("call", "d1"), ("call", "d3"),
                          ("call", "d3"), ("call", "d4"), ("call", "d4"),
                          ("call", "d1")])


@cpython_only
@requires_specialization
class TestRecursion(PlanTestCase):
    def test_recursion_through_defaults_and_kwargs(self):
        def rec(n, a=1):
            return rec(n + 1)

        def rec_kw(n, a=0):
            return rec_kw(n + 1, a=a)

        def rec_star(n, **kw):
            return rec_star(n + 1, **{'q': 1})

        for fn in (rec, rec_kw, rec_star):
            with self.subTest(fn=fn.__name__):
                with self.assertRaises(RecursionError):
                    fn(0)
                self.assertEqual((lambda: 1)(), 1)


if __name__ == "__main__":
    unittest.main()
