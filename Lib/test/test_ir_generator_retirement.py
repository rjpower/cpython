"""Completed receiver destruction preserves owners across callbacks."""

import gc
import sys
import types
import unittest
import weakref

import _opcode
import _testinternalcapi
from test import support


CONTRACTS = _opcode.get_ir_contract()
ROUNDS = _testinternalcapi.SPECIALIZATION_THRESHOLD * 3


class Result:
    def __init__(self):
        self.cycle = self


@unittest.skipIf(support.Py_GIL_DISABLED, "generator placement requires the GIL")
class ClearedReceiverTests(unittest.TestCase):
    def setUp(self):
        self.addCleanup(gc.collect)

    def assert_return_boundary(self, function):
        # The generator's completion metadata went with the old return rows:
        # Return.Generator completes the generator itself
        pass

    def finish(self, iterator):
        self.assertIsNone(next(iterator))
        with self.assertRaises(StopIteration) as stopped:
            next(iterator)
        return stopped.exception.value

    def test_plain_unique_receiver_preserves_fresh_result(self):
        def leaf():
            yield None
            return Result()

        def outer():
            return (yield from leaf())

        self.assert_return_boundary(outer)
        for _ in range(ROUNDS):
            value = self.finish(outer())
            ref = weakref.ref(value)
            gc.collect()
            self.assertIs(ref(), value)
            del value
            gc.collect()
            self.assertIsNone(ref())

    def test_receiver_weakref_callback_sees_live_result(self):
        result_refs, receiver_refs, observed = [], [], []

        def destroyed(ref):
            gc.collect()
            observed.append(result_refs[-1]() is not None)

        def leaf():
            yield None
            value = Result()
            result_refs.append(weakref.ref(value))
            return value

        def make_leaf():
            iterator = leaf()
            receiver_refs.append(weakref.ref(iterator, destroyed))
            return iterator

        def outer():
            return (yield from make_leaf())

        self.assert_return_boundary(outer)
        for _ in range(ROUNDS):
            value = self.finish(outer())
            self.assertIs(result_refs[-1](), value)
            self.assertIsNone(receiver_refs[-1]())
            del value
        self.assertEqual(observed, [True] * ROUNDS)

    def test_aliased_renamed_headers_finalize_with_live_result(self):
        result_refs, observed = [], []

        class Name(str):
            def __del__(self):
                gc.collect()
                observed.append(result_refs[-1]() is not None)

        def leaf():
            yield None
            value = Result()
            result_refs.append(weakref.ref(value))
            return value

        def make_leaf():
            iterator = leaf()
            name = Name("renamed")
            iterator.__name__ = iterator.__qualname__ = name
            return iterator

        def outer():
            return (yield from make_leaf())

        self.assert_return_boundary(outer)
        for _ in range(ROUNDS):
            value = self.finish(outer())
            self.assertIs(result_refs[-1](), value)
            del value
        self.assertEqual(observed, [True] * ROUNDS)

    def test_last_code_owner_finalizes_constants_with_live_result(self):
        result_refs, observed = [], []

        class Constant:
            def __del__(self):
                gc.collect()
                observed.append(result_refs[-1]() is not None)

        def leaf():
            yield None
            value = Result()
            result_refs.append(weakref.ref(value))
            return value

        def make_leaf():
            code = leaf.__code__.replace(
                co_consts=leaf.__code__.co_consts + (Constant(),))
            function = types.FunctionType(code, leaf.__globals__,
                                          closure=leaf.__closure__)
            return function()

        def outer():
            return (yield from make_leaf())

        self.assert_return_boundary(outer)
        for _ in range(ROUNDS):
            value = self.finish(outer())
            self.assertIs(result_refs[-1](), value)
            del value
        self.assertEqual(observed, [True] * ROUNDS)

    def test_coroutine_origin_owner_is_released(self):
        origins, before = [], []

        def origin_count(origin):
            return sys.getrefcount(origin)

        async def leaf():
            return Result()

        def make_leaf():
            coroutine = leaf()
            origin = coroutine.cr_origin
            origins.append(origin)
            before.append(origin_count(origin))
            return coroutine

        async def outer():
            return await make_leaf()

        self.assert_return_boundary(outer)
        depth = sys.get_coroutine_origin_tracking_depth()
        self.addCleanup(sys.set_coroutine_origin_tracking_depth, depth)
        sys.set_coroutine_origin_tracking_depth(1)
        for _ in range(ROUNDS):
            coroutine = outer()
            with self.assertRaises(StopIteration) as stopped:
                coroutine.send(None)
            self.assertIsInstance(stopped.exception.value, Result)
            origin = origins[-1]
            self.assertIsNotNone(origin)
            # Both snapshots hold the origin in a caller local and use the
            # same helper; only the child coroutine's owner has disappeared.
            self.assertEqual(origin_count(origin), before[-1] - 1)

    def test_nested_completion_during_child_retirement(self):
        result_refs, observed = [], []

        def nested():
            yield None
            return Result()

        def nested_outer():
            return (yield from nested())

        class Cleanup:
            def __del__(self):
                iterator = nested_outer()
                next(iterator)
                try:
                    next(iterator)
                except StopIteration as stopped:
                    gc.collect()
                    observed.append(isinstance(stopped.value, Result)
                                    and result_refs[-1]() is not None)

        def leaf():
            cleanup = Cleanup()
            yield None
            value = Result()
            result_refs.append(weakref.ref(value))
            return value

        def outer():
            return (yield from leaf())

        self.assert_return_boundary(outer)
        self.assert_return_boundary(nested_outer)
        for _ in range(ROUNDS):
            value = self.finish(outer())
            self.assertIs(result_refs[-1](), value)
            del value
        self.assertEqual(observed, [True] * ROUNDS)


@unittest.skipIf(support.Py_GIL_DISABLED, "generator placement requires the GIL")
class CompletedLoopReceiverTests(unittest.TestCase):
    """ForIter.Complete closes the generator its loop ran to the end by
    close_completed_receiver: a generator only the loop holds is freed at
    the loop's exit, before the next statement; one held elsewhere stays,
    exhausted."""

    def test_unique_generator_is_freed_at_the_loop_exit(self):
        log = []

        def gen():
            yield 1
            return Result()

        def make():
            iterator = gen()
            refs.append(weakref.ref(iterator, lambda ref: log.append("freed")))
            return iterator

        for _ in range(ROUNDS):
            refs = []
            log.clear()
            for x in make():
                log.append(x)
            log.append("after")
            self.assertEqual(log, [1, "freed", "after"])

    def test_unique_generator_and_its_result_are_freed(self):
        results = []

        def gen():
            yield 1
            value = Result()
            results.append(weakref.ref(value))
            return value

        for _ in range(ROUNDS):
            results.clear()
            for x in gen():
                pass
            gc.collect()
            self.assertIsNone(results[0]())

    def test_shared_generator_stays_exhausted(self):
        def gen():
            yield 1
            return 2

        for _ in range(ROUNDS):
            g = gen()
            for x in g:
                pass
            self.assertIsNone(g.gi_frame)
            self.assertEqual(list(g), [])


class Noted:
    def __init__(self, log, name):
        self.log = log
        self.name = name

    def __del__(self):
        self.log.append(self.name)


class EmptyReturnTests(unittest.TestCase):
    """A generator's return with a vacant scratch is Return.GeneratorEmpty,
    whose retirement clears only the fixed slots: the locals and the value
    a yield statement discarded."""

    def arms(self, function):
        from test.support import ir_dis
        return [w.arm for w in ir_dis.get_instructions(function)
                if w.family == "Return"]

    def test_arm_by_scratch(self):
        # The compiler empties the stack before a return, so a generator's
        # return lowers to Return.GeneratorEmpty; test_rop_call forces
        # Return.Generator.
        def vacant():
            yield 1
            return 2

        def in_loop(it):
            for x in it:
                return x
            yield

        self.assertEqual(self.arms(vacant), ["Return.GeneratorEmpty"])
        self.assertEqual(set(self.arms(in_loop)), {"Return.GeneratorEmpty"})

    def test_locals_and_discarded_value_retire_at_the_return(self):
        log = []

        def gen():
            a = Noted(log, "local")
            yield 1
            log.append("returning")
            return a.name

        for _ in range(ROUNDS):
            del log[:]
            g = gen()
            next(g)
            with self.assertRaises(StopIteration) as cm:
                g.send(Noted(log, "discarded"))
            self.assertEqual(cm.exception.value, "local")
            self.assertEqual(sorted(log[1:]), ["discarded", "local"])
            self.assertEqual(log[0], "returning")


if __name__ == "__main__":
    unittest.main()
