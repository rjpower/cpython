"""The call, return and raise rows of pycore_ir_components.h, as the
language shows them (G0c batch 6).

Each test holds a claim that a declared uop relies on:
- do_raise consumes a count of the exception and of the cause on every
  path, and runs Python while it builds the exception;
- a native callee borrows the callable and the operands, which the call
  publishes and releases afterwards, so a callee that rebinds a caller's
  local does not free what the call still reads;
- a returned frame is cleared with the result held in C: a finalizer runs
  Python at that point, and a frame object that outlives its frame takes
  the locals;
- the method operand of an expanded bound-method call is released at the
  push, which runs no code because the guard refuses a method that has
  weak references.
"""

import gc
import sys
import unittest
import weakref


class Exc(Exception):
    pass


class Counted(Exception):
    created = 0

    def __init__(self, *args):
        Counted.created += 1
        super().__init__(*args)


def raise_with(exc, cause=...):
    if cause is ...:
        raise exc
    raise exc from cause


class DoRaiseTests(unittest.TestCase):
    def raised(self, *args):
        try:
            raise_with(*args)
        except BaseException as caught:
            return caught
        self.fail("nothing raised")

    def test_instance_and_cause_are_consumed(self):
        exc, cause = Exc("a"), Exc("b")
        before = sys.getrefcount(exc), sys.getrefcount(cause)
        for _ in range(50):
            caught = self.raised(exc, cause)
            self.assertIs(caught, exc)
            self.assertIs(caught.__cause__, cause)
            caught.__traceback__ = None
            del caught
        exc.__cause__ = None
        exc.__traceback__ = None
        self.assertEqual((sys.getrefcount(exc), sys.getrefcount(cause)), before)

    def test_class_is_instantiated_by_running_python(self):
        Counted.created = 0
        for _ in range(50):
            caught = self.raised(Counted)
            self.assertIsInstance(caught, Counted)
        self.assertEqual(Counted.created, 50)

    def test_cause_class_is_instantiated(self):
        Counted.created = 0
        caught = self.raised(Exc("x"), Counted)
        self.assertIsInstance(caught.__cause__, Counted)
        self.assertEqual(Counted.created, 1)

    def test_not_an_exception_balances_counts(self):
        thing = object()
        before = sys.getrefcount(thing)
        for _ in range(50):
            with self.assertRaises(TypeError):
                raise_with(thing)
            with self.assertRaises(TypeError):
                raise_with(Exc("x"), thing)
        self.assertEqual(sys.getrefcount(thing), before)

    def test_number_cause(self):
        with self.assertRaises(TypeError) as ctx:
            raise_with(Exc("x"), 3)
        self.assertIn("exception causes", str(ctx.exception))

    def test_number_exception(self):
        with self.assertRaises(TypeError) as ctx:
            raise_with(3)
        self.assertIn("exceptions must derive", str(ctx.exception))

    def test_bare_raise_reraises(self):
        try:
            try:
                raise Exc("inner")
            except Exc:
                raise
        except Exc as caught:
            self.assertEqual(caught.args, ("inner",))
        with self.assertRaises(RuntimeError):
            raise


class NativeCallBorrowTests(unittest.TestCase):
    def test_callee_rebinds_a_caller_local(self):
        def run():
            data = [3, 1, 2]

            def key(x):
                # PEP 667: the write goes to the caller's local
                sys._getframe(1).f_locals["data"] = None
                return x

            return sorted(data, key=key), data

        for _ in range(100):
            result, data = run()
            self.assertEqual(result, [1, 2, 3])
            self.assertIsNone(data)

    def test_callee_drops_the_callable_local(self):
        class Callable:
            def __call__(self, x):
                return x

        def run():
            fn = Callable()

            def dropper(x):
                sys._getframe(1).f_locals["fn"] = None
                return x

            # the builtin calls dropper, then map calls nothing else: the
            # point is that the operands outlive a native callee's rebinding
            return list(map(dropper, [1, 2, 3])), fn

        for _ in range(100):
            result, fn = run()
            self.assertEqual(result, [1, 2, 3])
            self.assertIsNone(fn)

    def test_temporaries_are_released_once(self):
        class Tracked:
            alive = 0

            def __init__(self):
                Tracked.alive += 1

            def __del__(self):
                Tracked.alive -= 1

        for _ in range(200):
            self.assertEqual(len([Tracked() for _ in range(3)]), 3)
            self.assertEqual(max(Tracked(), Tracked(), key=id).__class__, Tracked)
        gc.collect()
        self.assertEqual(Tracked.alive, 0)


class FrameReturnTests(unittest.TestCase):
    def test_finalizer_runs_python_while_the_result_is_held(self):
        seen = []

        class Fin:
            def __del__(self):
                # The caller's frame is current again and consistent
                seen.append(sys._getframe(1).f_code.co_name)
                [x * 2 for x in range(10)]

        def callee(value):
            local = Fin()
            return value

        def caller():
            result = [callee(object()) for _ in range(3)]
            return callee(result)

        for _ in range(100):
            seen.clear()
            result = caller()
            self.assertEqual(len(result), 3)
            self.assertTrue(seen)
            self.assertTrue(all(name in ("callee", "<listcomp>", "caller")
                                for name in seen), seen)

    def test_frame_object_outlives_the_frame(self):
        def callee():
            local = ["kept"]
            return sys._getframe()

        def caller():
            frame = callee()
            return frame

        for _ in range(100):
            frame = caller()
            self.assertEqual(frame.f_locals["local"], ["kept"])
            self.assertEqual(frame.f_back.f_code.co_name, "caller")
            del frame

    def test_generator_return_value_with_finalizer(self):
        class Fin:
            def __del__(self):
                [x for x in range(3)]

        def gen():
            local = Fin()
            yield 1
            return "done"

        for _ in range(100):
            g = gen()
            self.assertEqual(next(g), 1)
            with self.assertRaises(StopIteration) as ctx:
                next(g)
            self.assertEqual(ctx.exception.value, "done")


class BoundMethodExpansionTests(unittest.TestCase):
    class Box:
        def __init__(self):
            self.n = 0

        def bump(self, k):
            self.n += k
            return self.n

    def test_weakref_callback_on_the_method_runs_at_the_generic_path(self):
        box = self.Box()
        calls = []

        def call(method, k):
            return method(k)

        # Warm the specialized path with methods without weak references
        for i in range(200):
            call(box.bump, 1)
        # A method that has a weak reference: its callback must run when the
        # call releases the method, after the callee's frame is current
        method = box.bump
        ref = weakref.ref(method, lambda r: calls.append(box.n))
        self.assertEqual(call(method, 5), 205)
        del method
        gc.collect()
        self.assertEqual(calls, [205])
        del ref

    def test_expanded_call_keeps_function_and_receiver_alive(self):
        class Owner:
            def go(self, k):
                return k + 1

        for _ in range(200):
            self.assertEqual(Owner().go(1), 2)
            self.assertEqual((Owner().go)(2), 3)


if __name__ == "__main__":
    unittest.main()
