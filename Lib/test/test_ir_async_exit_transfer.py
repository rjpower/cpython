"""The EndAsyncFor family (Python/register_ops/cleanup.rop): the end of an
async loop retires its operands only for a StopAsyncIteration."""

import types
import unittest

import _testinternalcapi
from test.test_ir_call_roles import routes


class AsyncExitTransferTests(unittest.TestCase):
    def test_nested_async_loop_reuses_call_window_on_exhaustion(self):
        class Pause:
            def __await__(self):
                yield "pause"

        class Values:
            def __init__(self, values, *, error=False):
                self.values = iter(values)
                self.error = error

            def __aiter__(self):
                return self

            async def __anext__(self):
                await Pause()
                if self.error:
                    raise LookupError("async source")
                value = next(self.values, self)
                if value is self:
                    raise StopAsyncIteration
                return value

        async def outer(fn, groups):
            result = []
            for group in groups:
                async for value in group:
                    result.append(value)
                fn(17)
            return result

        names = {row[0] for row in _testinternalcapi.get_ir(outer.__code__)}
        self.assertTrue({"EndAsyncFor.Generic", "Call.Generic+NO_SELF"} <= names)
        for admitted in routes(outer.__code__):
            function = types.FunctionType(admitted, globals())
            seen = []
            coroutine = function(seen.append, (Values(()), Values((1, 2))))
            try:
                while True:
                    self.assertEqual(coroutine.send(None), "pause")
            except StopIteration as ended:
                self.assertEqual(ended.value, [1, 2])
            finally:
                coroutine.close()
            self.assertEqual(seen, [17, 17])
            coroutine = function(seen.append, (Values((), error=True),))
            try:
                self.assertEqual(coroutine.send(None), "pause")
                with self.assertRaisesRegex(LookupError, "async source"):
                    coroutine.send(None)
            finally:
                coroutine.close()
            self.assertEqual(seen, [17, 17])
