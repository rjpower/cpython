"""The cell families (Python/register_ops/cells.rop): CellMake, CellRead,
CellMapRead, CellStore and CellDelete, and the frame entry that gives a
frame its free variables (_PyFrame_CopyFreeVars)."""

import asyncio
import gc
import sys
import unittest

from test import support
from test.support import import_helper


class Probe:
    """A value whose finalizer reads what the cell holds when it runs"""
    def __init__(self, read, log):
        self.read = read
        self.log = log

    def __del__(self):
        self.log.append(self.read())


class ReadTests(unittest.TestCase):
    def test_free_and_cell_variables(self):
        def outer(a):
            b = a + 1
            def inner():
                return a, b
            return inner
        self.assertEqual(outer(1)(), (1, 2))

    def test_unbound_free_variable_is_a_name_error(self):
        def outer():
            def inner():
                return x
            if False:
                x = 1
            return inner
        with self.assertRaisesRegex(NameError, "free variable 'x'"):
            outer()()

    def test_unbound_cell_variable_is_an_unbound_local_error(self):
        def f():
            def g():
                return x
            try:
                return x
            finally:
                x = 1
        with self.assertRaisesRegex(UnboundLocalError, "cannot access local variable 'x'"):
            f()

    def test_read_into_a_local_releases_its_old_binding(self):
        log = []
        def f():
            x = 1
            def g():
                return x
            y = Probe(lambda: y, log)
            y = x
            return y
        self.assertEqual(f(), 1)
        self.assertEqual(log, [1])

    def test_nested_closures(self):
        def a(x):
            def b(y):
                def c(z):
                    return x, y, z
                return c
            return b
        self.assertEqual(a(1)(2)(3), (1, 2, 3))


class WriteTests(unittest.TestCase):
    def test_store_closes_the_old_value_after_the_new_is_in_place(self):
        log = []
        def f():
            x = None
            def read():
                return x
            def write(v):
                nonlocal x
                x = v
            return read, write
        read, write = f()
        write(Probe(lambda: read(), log))
        new = object()
        write(new)
        self.assertEqual(log, [new])

    def test_delete_empties_the_cell_before_closing_the_old_value(self):
        log = []
        def f():
            x = None
            def read():
                try:
                    return x
                except NameError:
                    return "empty"
            def delete():
                nonlocal x
                del x
            def write(v):
                nonlocal x
                x = v
            return read, delete, write
        read, delete, write = f()
        write(Probe(lambda: read(), log))
        delete()
        self.assertEqual(log, ["empty"])

    def test_delete_of_an_empty_cell_raises_and_changes_nothing(self):
        def f():
            x = 1
            def delete():
                nonlocal x
                del x
            def read():
                return x
            return delete, read
        delete, read = f()
        delete()
        with self.assertRaises(NameError):
            delete()
        with self.assertRaises(NameError):
            read()

    def test_delete_of_a_cell_variable_before_assignment(self):
        def f():
            def g():
                return x
            del x
        with self.assertRaises(UnboundLocalError):
            f()


class MakeCellTests(unittest.TestCase):
    def test_argument_moves_into_its_cell(self):
        def f(a, b):
            def g():
                return a
            a = 5
            return g, b
        g, b = f(1, 2)
        self.assertEqual((g(), b), (5, 2))

    def test_argument_with_a_finalizer_survives_the_wrap(self):
        log = []
        class Box:
            def __del__(self):
                log.append("closed")
        def f(a):
            return lambda: a
        g = f(Box())
        gc.collect()
        self.assertEqual(log, [])
        del g
        self.assertEqual(log, ["closed"])

    def test_comprehension_over_a_captured_variable(self):
        def f(items):
            fns = [lambda: x for x in items]
            x = "outer"
            return [fn() for fn in fns], x
        self.assertEqual(f([1, 2, 3]), ([3, 3, 3], "outer"))

    def test_inlined_comprehension_restores_a_cell_variable(self):
        def f():
            x = "before"
            def g():
                return x
            r = [x for x in range(3)]
            return r, g()
        self.assertEqual(f(), ([0, 1, 2], "before"))

    @support.cpython_only
    def test_allocation_failure_closes_the_argument(self):
        _testcapi = import_helper.import_module("_testcapi")
        log = []
        class Box:
            def __del__(self):
                log.append("closed")
        def f(a):
            return lambda: a
        for start in range(1, 12):
            log.clear()
            box = Box()
            _testcapi.set_nomemory(start, start + 1)
            try:
                try:
                    f(box)
                except MemoryError:
                    pass
            finally:
                _testcapi.remove_mem_hooks()
            del box
            gc.collect()
            self.assertEqual(log, ["closed"], start)


class ClassBodyTests(unittest.TestCase):
    def test_free_variable_and_the_class_namespace(self):
        def f(shadow):
            x = "outer"
            class C:
                if shadow:
                    locals()["x"] = "class"
                y = x
            return C
        self.assertEqual(f(False).y, "outer")
        self.assertEqual(f(True).y, "class")

    def test_class_namespace_that_runs_python(self):
        calls = []
        class Namespace(dict):
            def __getitem__(self, key):
                calls.append(key)
                return super().__getitem__(key)
        class Meta(type):
            @classmethod
            def __prepare__(mcls, name, bases, **kw):
                return Namespace()
        def f():
            x = "cell"
            class C(metaclass=Meta):
                y = x
            return C
        self.assertEqual(f().y, "cell")
        self.assertIn("x", calls)

    def test_class_namespace_that_raises(self):
        class Boom(dict):
            def __getitem__(self, key):
                if key == "x":
                    raise RuntimeError("boom")
                return super().__getitem__(key)
        class Meta(type):
            @classmethod
            def __prepare__(mcls, name, bases, **kw):
                return Boom()
        def f():
            x = 1
            class C(metaclass=Meta):
                y = x
        with self.assertRaisesRegex(RuntimeError, "boom"):
            f()

    def test_class_cell_and_zero_argument_super(self):
        class A:
            def hello(self):
                return "A"
        class B(A):
            def hello(self):
                return "B" + super().hello()
            def cls(self):
                return __class__
        self.assertEqual(B().hello(), "BA")
        self.assertIs(B().cls(), B)

    def test_unbound_name_in_the_class_namespace_falls_to_the_cell(self):
        def f():
            def make():
                class C:
                    y = x
                return C
            if False:
                x = 1
            return make
        with self.assertRaises(NameError):
            f()()


class EntryTests(unittest.TestCase):
    """Every way a frame is entered gives it its free variables"""

    def make(self):
        a, b = 1, 2
        def f(c=3):
            return a, b, c
        return f

    def test_python_call(self):
        self.assertEqual(self.make()(), (1, 2, 3))

    def test_call_from_c(self):
        f = self.make()
        self.assertEqual(list(map(f, [4, 5])), [(1, 2, 4), (1, 2, 5)])
        self.assertEqual(sorted([2, 1], key=lambda v: (f(v), v)), [1, 2])

    def test_bound_method_and_init(self):
        n = 7
        class C:
            def __init__(self):
                self.v = n
            def m(self):
                return n
        self.assertEqual(C().v, 7)
        self.assertEqual(C().m(), 7)
        self.assertEqual(C.m(C()), 7)

    def test_generator_coroutine_and_async_generator(self):
        a = 10
        def gen():
            yield a
        async def coro():
            return a
        async def agen():
            yield a
        async def main():
            return [v async for v in agen()]
        self.assertEqual(list(gen()), [10])
        self.assertEqual(asyncio.run(coro()), 10)
        self.assertEqual(asyncio.run(main()), [10])

    def test_generator_expression_with_a_free_variable(self):
        k = 3
        self.assertEqual(list(x * k for x in range(3)), [0, 3, 6])

    def test_exec_with_a_closure(self):
        def outer():
            x = 42
            def inner():
                return x
            return inner
        inner = outer()
        code = inner.__code__
        out = []
        def outer2():
            x = 42
            def inner2():
                out.append(x)
            return inner2
        inner2 = outer2()
        exec(inner2.__code__, {"out": out}, closure=inner2.__closure__)
        self.assertEqual(out, [42])

    def test_function_type_with_a_closure(self):
        import types
        def outer():
            x = 5
            def inner():
                return x
            return inner
        inner = outer()
        g = types.FunctionType(inner.__code__, {}, closure=inner.__closure__)
        self.assertEqual(g(), 5)

    def test_f_locals_of_a_frame_with_free_variables(self):
        def f():
            a = 1
            def g():
                return a, sys._getframe().f_locals
            return g
        value, local = f()()
        self.assertEqual(value, 1)
        self.assertEqual(local, {"a": 1})

    def test_trace_call_event_sees_the_free_variables(self):
        seen = []
        def tracer(frame, event, arg):
            if event == "call" and frame.f_code.co_name == "g":
                seen.append(dict(frame.f_locals))
            return None
        def f():
            a = 1
            def g():
                return a
            return g
        g = f()
        sys.settrace(tracer)
        try:
            g()
        finally:
            sys.settrace(None)
        self.assertEqual(seen, [{"a": 1}])

    @support.cpython_only
    def test_frame_new_of_code_with_free_variables(self):
        _testcapi = import_helper.import_module("_testcapi")
        def outer():
            a = 1
            def inner():
                return a
            return inner
        code = outer().__code__
        frame = _testcapi.frame_new(code, {}, {})
        self.assertEqual(frame.f_code, code)


class ObservationTests(unittest.TestCase):
    def test_line_events_on_cell_words(self):
        def f():
            x = 1
            def g():
                nonlocal x
                x = x + 1
                return x
            return g()
        lines = []
        def tracer(frame, event, arg):
            if frame.f_code is f.__code__ and event == "line":
                lines.append(frame.f_lineno - f.__code__.co_firstlineno)
            return tracer
        sys.settrace(tracer)
        try:
            self.assertEqual(f(), 2)
        finally:
            sys.settrace(None)
        self.assertEqual(lines, [1, 2, 6])


if __name__ == "__main__":
    unittest.main()
