"""Completed receiver destruction preserves owners across callbacks."""

import gc
import opcode
import sys
import types
import unittest
import weakref

import _opcode
import _testinternalcapi
from test import support


HANDLERS = {**opcode.opmap, **opcode._specialized_opmap, **opcode._ir_opmap}
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
        rows = [CONTRACTS[HANDLERS[word[0].partition("+")[0]]]
                for word in _testinternalcapi.get_ir(function.__code__)]
        self.assertTrue(any(
            row.generator_completed_consumer == _opcode.GEN_CONSUMER_RETURN
            and row.generator_boundary_flags & _opcode.GEN_BOUNDARY_CLOSED
            for row in rows))

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


if __name__ == "__main__":
    unittest.main()
