"""The cleanup families (Python/register_ops/cleanup.rop): CleanupThrow
(CLEANUP_THROW), EndAsyncFor (END_ASYNC_FOR) and WithExit
(WITH_EXCEPT_START)."""

import dis
import sys
import unittest


def run(coro):
    """Drive a coroutine that never suspends"""
    try:
        coro.send(None)
    except StopIteration as stop:
        return stop.value
    raise AssertionError("the coroutine suspended")


class Log:
    """Records the order of finalizers"""
    def __init__(self, log, name):
        self.log = log
        self.name = name

    def __del__(self):
        self.log.append(self.name)


class CleanupThrowTests(unittest.TestCase):
    def test_a_throw_that_the_delegate_returns_from(self):
        def inner():
            try:
                yield 1
            except ValueError:
                return "caught"

        def outer():
            result = yield from inner()
            return result

        g = outer()
        self.assertEqual(next(g), 1)
        with self.assertRaises(StopIteration) as cm:
            g.throw(ValueError)
        self.assertEqual(cm.exception.value, "caught")

    def test_the_value_is_none_when_the_delegate_returns_nothing(self):
        def inner():
            try:
                yield 1
            except ValueError:
                pass

        def outer():
            result = yield from inner()
            return ("done", result)

        g = outer()
        next(g)
        with self.assertRaises(StopIteration) as cm:
            g.throw(ValueError)
        self.assertEqual(cm.exception.value, ("done", None))

    def test_another_exception_is_raised_again(self):
        def inner():
            yield 1

        def outer():
            yield from inner()

        g = outer()
        next(g)
        with self.assertRaises(KeyError):
            g.throw(KeyError)

    def test_the_delegate_raises_another_exception(self):
        def inner():
            try:
                yield 1
            except ValueError:
                raise OSError("replaced")

        def outer():
            yield from inner()

        g = outer()
        next(g)
        with self.assertRaises(OSError) as cm:
            g.throw(ValueError)
        self.assertIsInstance(cm.exception.__context__, ValueError)

    def test_a_delegate_that_yields_again_after_the_throw(self):
        def inner():
            try:
                yield 1
            except ValueError:
                yield 2
            return "end"

        def outer():
            return (yield from inner())

        g = outer()
        next(g)
        self.assertEqual(g.throw(ValueError), 2)
        with self.assertRaises(StopIteration) as cm:
            next(g)
        self.assertEqual(cm.exception.value, "end")

    def test_the_operands_are_closed(self):
        log = []

        def inner(guard):
            try:
                yield 1
            except ValueError:
                return "caught"

        def outer():
            guard = Log(log, "guard")
            return (yield from inner(guard))

        g = outer()
        next(g)
        with self.assertRaises(StopIteration):
            g.throw(ValueError)
        del g
        self.assertEqual(log, ["guard"])

    def test_close_of_a_delegation(self):
        closed = []

        def inner():
            try:
                yield 1
            except GeneratorExit:
                closed.append("inner")
                raise

        def outer():
            yield from inner()

        g = outer()
        next(g)
        g.close()
        self.assertEqual(closed, ["inner"])

    def test_await_of_a_coroutine_that_handles_a_throw(self):
        class Suspend:
            def __await__(self):
                yield "suspended"

        async def inner():
            try:
                await Suspend()
            except ValueError:
                return "handled"

        async def outer():
            return await inner()

        c = outer()
        self.assertEqual(c.send(None), "suspended")
        with self.assertRaises(StopIteration) as cm:
            c.throw(ValueError)
        self.assertEqual(cm.exception.value, "handled")

    def test_a_delegation_in_a_generator_expression_of_try_blocks(self):
        def inner():
            while True:
                try:
                    yield "x"
                except KeyError:
                    return "k"

        def outer():
            r = yield from inner()
            yield r

        g = outer()
        next(g)
        self.assertEqual(g.throw(KeyError), "k")


class EndAsyncForTests(unittest.TestCase):
    def test_the_loop_ends(self):
        async def agen():
            yield 1
            yield 2

        async def main():
            seen = []
            async for x in agen():
                seen.append(x)
            return seen

        self.assertEqual(run(main()), [1, 2])

    def test_an_empty_loop(self):
        async def agen():
            return
            yield

        async def main():
            async for _ in agen():
                raise AssertionError
            return "empty"

        self.assertEqual(run(main()), "empty")

    def test_the_else_clause_runs_after_the_end(self):
        async def agen():
            yield 1

        async def main():
            order = []
            async for x in agen():
                order.append(x)
            else:
                order.append("else")
            return order

        self.assertEqual(run(main()), [1, "else"])

    def test_another_exception_is_raised_again(self):
        async def agen():
            yield 1
            raise ValueError("from the iterator")

        async def main():
            async for _ in agen():
                pass

        with self.assertRaisesRegex(ValueError, "from the iterator"):
            run(main())

    def test_an_exception_of_a_user_iterator(self):
        class Aiter:
            def __aiter__(self):
                return self

            async def __anext__(self):
                raise StopAsyncIteration("the end")

        async def main():
            n = 0
            async for _ in Aiter():
                n += 1
            return n

        self.assertEqual(run(main()), 0)

    def test_one_loop_after_another(self):
        async def agen(n):
            for i in range(n):
                yield i

        async def main():
            total = 0
            async for i in agen(3):
                total += i
            async for j in agen(4):
                total += 10 * j
            return total

        self.assertEqual(run(main()), 3 + 60)

    def test_the_operands_are_closed(self):
        log = []

        async def agen():
            guard = Log(log, "agen")
            yield 1

        async def main():
            async for _ in agen():
                pass
            log.append("after")

        run(main())
        self.assertEqual(log, ["agen", "after"])


class EndAsyncForMonitoringTests(unittest.TestCase):
    tool = sys.monitoring.PROFILER_ID

    def setUp(self):
        sys.monitoring.use_tool_id(self.tool, "test_rop_cleanup")
        self.addCleanup(sys.monitoring.free_tool_id, self.tool)
        self.addCleanup(sys.monitoring.set_events, self.tool, 0)

    def test_the_branch_event_is_reported_at_the_send(self):
        async def agen():
            yield 1

        async def main():
            async for _ in agen():
                pass

        events = sys.monitoring.events
        seen = []

        def on_branch(code, offset, destination):
            if code is main.__code__:
                seen.append((offset, destination))

        sys.monitoring.register_callback(self.tool, events.BRANCH_RIGHT, on_branch)
        sys.monitoring.set_events(self.tool, events.BRANCH_RIGHT)
        run(main())
        sys.monitoring.set_events(self.tool, 0)
        by_offset = {i.offset: i.opname for i in dis.get_instructions(main)}
        sources = [by_offset[offset] for offset, _ in seen]
        self.assertEqual(sources, ["SEND"])

    def test_a_callback_that_raises(self):
        async def agen():
            yield 1

        async def main():
            async for _ in agen():
                pass

        events = sys.monitoring.events

        def on_branch(code, offset, destination):
            if code is main.__code__:
                raise RuntimeError("from the callback")

        sys.monitoring.register_callback(self.tool, events.BRANCH_RIGHT, on_branch)
        sys.monitoring.set_events(self.tool, events.BRANCH_RIGHT)
        try:
            with self.assertRaisesRegex(RuntimeError, "from the callback"):
                run(main())
        finally:
            sys.monitoring.set_events(self.tool, 0)

    def test_a_reraise_event_of_another_exception(self):
        async def agen():
            yield 1
            raise ValueError

        async def main():
            async for _ in agen():
                pass

        events = sys.monitoring.events
        seen = []

        def on_reraise(code, offset, exception):
            if code is main.__code__:
                seen.append(type(exception))

        sys.monitoring.register_callback(self.tool, events.RERAISE, on_reraise)
        sys.monitoring.set_events(self.tool, events.RERAISE)
        try:
            with self.assertRaises(ValueError):
                run(main())
        finally:
            sys.monitoring.set_events(self.tool, 0)
        self.assertIn(ValueError, seen)


class Exit:
    """A context manager whose __exit__ is looked up on the type"""
    def __init__(self, result=None, calls=None):
        self.result = result
        self.calls = calls

    def __enter__(self):
        return self

    def __exit__(self, *args):
        if self.calls is not None:
            self.calls.append(args)
        return self.result


class WithExitTests(unittest.TestCase):
    def test_the_arguments_of_the_exit(self):
        calls = []
        error = ValueError("v")
        with self.assertRaises(ValueError):
            with Exit(calls=calls):
                raise error
        [(type_, value, tb)] = calls
        self.assertIs(type_, ValueError)
        self.assertIs(value, error)
        # The traceback is the one of the frame that raised; the exception
        # then grew it on its way out
        self.assertIsNotNone(tb)
        self.assertEqual(tb.tb_frame.f_code.co_name,
                         "test_the_arguments_of_the_exit")

    def test_a_true_result_suppresses(self):
        with Exit(result=True):
            raise ValueError
        for result in (True, 1, "yes", [0]):
            with self.subTest(result=result):
                with Exit(result=result):
                    raise ValueError

    def test_a_false_result_propagates(self):
        for result in (None, False, 0, "", []):
            with self.subTest(result=result):
                with self.assertRaises(ValueError):
                    with Exit(result=result):
                        raise ValueError

    def test_a_raising_exit_replaces_the_exception(self):
        class Raising:
            def __enter__(self):
                return self

            def __exit__(self, *args):
                raise KeyError("from exit")

        with self.assertRaises(KeyError) as cm:
            with Raising():
                raise ValueError
        self.assertIsInstance(cm.exception.__context__, ValueError)

    def test_an_exit_that_is_an_instance_attribute_function(self):
        # The special lookup is on the type: a bound method comes back for a
        # function, and a plain callable attribute takes no self
        calls = []

        class Plain:
            __exit__ = staticmethod(lambda *args: calls.append(args) or True)

            def __enter__(self):
                return self

        with Plain():
            raise ValueError
        self.assertEqual(len(calls), 1)
        self.assertEqual(len(calls[0]), 3)

    def test_an_exit_that_is_a_builtin(self):
        class Holder:
            __exit__ = staticmethod(lambda *a: False)

            def __enter__(self):
                return None

        with self.assertRaises(OSError):
            with Holder():
                raise OSError

    def test_a_scalar_exit_is_not_callable(self):
        class Scalar:
            def __enter__(self):
                return self

            __exit__ = 3

        with self.assertRaises(TypeError):
            with Scalar():
                pass

    def test_nested_contexts_exit_in_order(self):
        order = []

        class Named:
            def __init__(self, name):
                self.name = name

            def __enter__(self):
                return self

            def __exit__(self, *args):
                order.append((self.name, args[0]))
                return False

        with self.assertRaises(ValueError):
            with Named("a"), Named("b"):
                raise ValueError
        self.assertEqual(order, [("b", ValueError), ("a", ValueError)])

    def test_the_exception_is_still_the_handled_one_in_exit(self):
        seen = []

        class Look:
            def __enter__(self):
                return self

            def __exit__(self, *args):
                seen.append(sys.exception())
                return True

        error = ValueError("handled")
        with Look():
            raise error
        self.assertEqual(seen, [error])

    def test_an_async_with_exit(self):
        calls = []

        class Async:
            async def __aenter__(self):
                return self

            async def __aexit__(self, *args):
                calls.append(args[0])
                return True

        async def main():
            async with Async():
                raise ValueError
            return "after"

        self.assertEqual(run(main()), "after")
        self.assertEqual(calls, [ValueError])

    def test_the_operands_are_kept_for_the_handler(self):
        log = []

        class Guarded:
            def __init__(self):
                self.guard = Log(log, "context")

            def __enter__(self):
                return self

            def __exit__(self, *args):
                log.append("exit")
                return False

        def f():
            with Guarded():
                raise ValueError

        with self.assertRaises(ValueError):
            f()
        self.assertEqual(log, ["exit", "context"])


if __name__ == "__main__":
    unittest.main()
