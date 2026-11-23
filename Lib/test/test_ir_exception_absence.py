"""A keyword call held across an await survives throw recovery."""

import marshal
import types
import unittest

import _testinternalcapi
from test.test_ir_admission import construct


def routes(code):
    return (code, construct(code), code.replace(),
            marshal.loads(marshal.dumps(code)))


class ExceptionAbsenceTests(unittest.TestCase):
    def test_real_keyword_call_held_across_await_and_throw_recovery(self):
        class Awaitable:
            def __await__(self):
                try:
                    yield "paused"
                except ValueError:
                    return "recovered"
                return "done"

        def ident(*, val):
            return val

        async def foo():
            return ident(val=await Awaitable())

        self.assertTrue(any(row[0] == "CALL_KW+NO_SELF"
                            for row in _testinternalcapi.get_ir(foo.__code__)))
        for code in routes(foo.__code__):
            function = types.FunctionType(code, globals(), closure=foo.__closure__)
            for throw, expected in ((False, "done"), (True, "recovered")):
                with self.subTest(throw=throw):
                    coroutine = function()
                    try:
                        self.assertEqual(coroutine.send(None), "paused")
                        with self.assertRaises(StopIteration) as result:
                            if throw:
                                coroutine.throw(ValueError())
                            else:
                                coroutine.send(None)
                        self.assertEqual(result.exception.value, expected)
                    finally:
                        coroutine.close()


if __name__ == "__main__":
    unittest.main()
