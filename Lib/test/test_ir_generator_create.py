"""Generators created at their call, GET_ITER over a generator __iter__,
and the plain bit."""

import gc
import sys
import unittest
import weakref

from test.support import cpython_only, requires_specialization, reset_code
from test.support.import_helper import import_module
from test.test_ir_attr_cache import handlers as ir_handlers

_testinternalcapi = import_module("_testinternalcapi")


def handlers(fn):
    # The call handlers carry a role suffix, as Call.Generic+NO_SELF.
    return [name.split("+")[0] for name in ir_handlers(fn)]


def warm(fn, *args):
    for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 2):
        result = fn(*args)
    return result


def gen(a, b):
    yield a
    yield b


def make_caller():
    def caller(a, b):
        return list(gen(a, b))
    return reset_code(caller)


class Box:
    def __init__(self, n):
        self.n = n

    def __iter__(self):
        for i in range(self.n):
            yield i


def make_looper():
    def loop(box):
        total = 0
        for i in box:
            total += i
        return total
    return reset_code(loop)


@cpython_only
@requires_specialization
class TestGeneratorCreation(unittest.TestCase):
    def test_call_creates_generator_without_frame(self):
        def call(a, b):
            return gen(a, b)
        call = reset_code(call)
        warm(call, 1, 2)
        self.assertIn("Call.Generic", handlers(call))
        g = call(3, 4)
        self.assertEqual(sys._getframe().f_code.co_name, self.id().split(".")[-1])
        self.assertIsNone(g.gi_frame.f_back)
        self.assertEqual(list(g), [3, 4])

    def test_arguments_are_owned_by_the_generator(self):
        class Tracked:
            pass

        def call(x):
            return gen(x, x)
        call = reset_code(call)
        warm(call, 0)
        obj = Tracked()
        ref = weakref.ref(obj)
        g = call(obj)
        del obj
        self.assertIsNotNone(ref())
        del g
        gc.collect()
        self.assertIsNone(ref())

    def test_unstarted_generator_closes(self):
        def call(x):
            return gen(x, x)
        call = reset_code(call)
        warm(call, 0)
        g = call(1)
        g.close()
        self.assertEqual(list(g), [])

    def test_coroutine_origin_tracking(self):
        async def co(x):
            return x

        def call(x):
            c = co(x)
            c.close()
            return c
        call = reset_code(call)
        old = sys.get_coroutine_origin_tracking_depth()
        sys.set_coroutine_origin_tracking_depth(1)
        try:
            c = warm(call, 1)
            self.assertIsNotNone(c.cr_origin)
            self.assertEqual(c.cr_origin[0][2], "call")
        finally:
            sys.set_coroutine_origin_tracking_depth(old)

    def test_type_change_rebinds_generator_call(self):
        caller = make_caller()
        self.assertEqual(warm(caller, 1, 2), [1, 2])
        self.assertIn("Call.Generic", handlers(caller))
        global gen
        saved = gen

        def plain(a, b):
            return [b, a]
        gen = plain
        try:
            self.assertEqual(caller(1, 2), [2, 1])
        finally:
            gen = saved


@cpython_only
@requires_specialization
class TestGetIterGenerator(unittest.TestCase):
    def test_get_iter_creates_generator(self):
        loop = make_looper()
        self.assertEqual(warm(loop, Box(4)), 6)
        self.assertIn("GET_ITER_PY_GENERATOR", handlers(loop))

    def test_replacing_iter_invalidates(self):
        class Local:
            def __iter__(self):
                yield 1
                yield 2

        loop = make_looper()
        self.assertEqual(warm(loop, Local()), 3)
        self.assertIn("GET_ITER_PY_GENERATOR", handlers(loop))

        def __iter__(self):
            return iter([10, 20])
        Local.__iter__ = __iter__
        self.assertEqual(loop(Local()), 30)

    def test_non_generator_iter_is_not_specialized(self):
        class Plain:
            def __iter__(self):
                return iter([1, 2, 3])

        loop = make_looper()
        self.assertEqual(warm(loop, Plain()), 6)
        self.assertNotIn("GET_ITER_PY_GENERATOR", handlers(loop))

    def test_error_in_iter_generator_creation(self):
        class Bad:
            def __iter__(self):
                yield 1
                raise ValueError("boom")

        loop = make_looper()
        warm(loop, Box(2))
        with self.assertRaises(ValueError):
            loop(Bad())


class TestPlainGenerator(unittest.TestCase):
    def test_renamed_generator_keeps_its_name(self):
        def g():
            yield 1
        it = g()
        it.__name__ = "renamed"
        it.__qualname__ = "q.renamed"
        self.assertEqual(list(it), [1])
        self.assertEqual(it.__name__, "renamed")
        self.assertEqual(it.__qualname__, "q.renamed")

    def test_finished_generator_is_freed_by_for(self):
        def g():
            yield 1
        refs = []
        for _ in range(3):
            it = g()
            refs.append(weakref.ref(it))
            for _ in it:
                pass
            del it
        self.assertEqual([r() for r in refs], [None] * 3)

    def test_finalizer_still_runs_for_started_generator(self):
        log = []

        def g():
            try:
                yield 1
            finally:
                log.append("closed")
        it = g()
        next(it)
        del it
        self.assertEqual(log, ["closed"])


class TestUnoptimizedCoroutine(unittest.TestCase):
    # Top-level await compiles to a coroutine whose code is not optimized:
    # its frame reads and writes the mapping of locals it was called with,
    # which the created coroutine must keep.
    def test_top_level_await_keeps_its_locals(self):
        import ast
        import types

        async def ready(value):
            return value

        code = compile("a = await ready(1)\nb = a + 1", "<test>", "exec",
                       flags=ast.PyCF_ALLOW_TOP_LEVEL_AWAIT)
        namespace = {"ready": ready}
        coro = types.FunctionType(code, namespace)()
        with self.assertRaises(StopIteration):
            coro.send(None)
        self.assertEqual((namespace["a"], namespace["b"]), (1, 2))


if __name__ == "__main__":
    unittest.main()
