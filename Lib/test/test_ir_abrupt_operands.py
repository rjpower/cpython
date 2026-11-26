"""Ordinary error transport confines writes to actual encoded operands."""

import marshal
from pathlib import Path
import tempfile
import types
import unittest
from unittest import mock

import _opcode
import _testinternalcapi

from test.test_ir_call_roles import routes


class AbruptOperandTests(unittest.TestCase):
    def test_nonwindow_peek_retains_its_input(self):
        def match(subject):
            match subject:
                case [first, *rest]:
                    return first, rest
                case _:
                    return None

        self.assertEqual(match([1, 2, 3]), (1, [2, 3]))
        self.assertIsNone(match(5))
        names = {row[0] for row in _testinternalcapi.get_ir(match.__code__, True)}
        self.assertIn("Length.Size", names)
        # The subject outlives the peek of its length
        subject = [1, 2]
        for code in routes(match.__code__):
            function = types.FunctionType(code, globals())
            self.assertEqual(function(subject), (1, [2]))
            self.assertEqual(subject, [1, 2])

    def test_window_scalar_fields_are_trailing_inputs_not_through_indices(self):
        # The comprehensions run with items already pending below them,
        # so the appended-to container sits at several distances from the top.
        def lists(values):
            return (0, 1, 2, 3, 4, 5, 6, [v for v in values]), 7

        def sets(values):
            return (0, 1, 2, 3, 4, 5, 6, {v for v in values}), 7

        for function, container in ((lists, list), (sets, set)):
            with self.subTest(function=function.__name__):
                for code in routes(function.__code__):
                    names = {row[0] for row in _testinternalcapi.get_ir(code, True)}
                    self.assertTrue(names & {"AddElement.Append", "AddElement.Add"}, names)
                    result = types.FunctionType(code, globals())([3, 4, 3])
                    self.assertEqual(result, ((0, 1, 2, 3, 4, 5, 6, container([3, 4, 3])), 7))

    def test_actual_anext_exception_keeps_a_held_keyword_call_hole(self):
        class Pause:
            def __await__(self):
                yield "pause"

        class Values:
            def __init__(self, values, *, fail=False):
                self.values = iter(values)
                self.fail = fail

            def __aiter__(self):
                return self

            def __anext__(self):
                if self.fail:
                    raise LookupError("anext source")
                value = next(self.values, self)
                if value is self:
                    # Raised by GET_ANEXT itself, not a later SEND/YIELD.
                    raise StopAsyncIteration

                async def ready():
                    await Pause()
                    return value
                return ready()

        async def invoke(function, source):
            return function(value=[item async for item in source])

        names = {row[0] for row in _testinternalcapi.get_ir(invoke.__code__)}
        self.assertIn("CallKw.Generic+NO_SELF", names)
        for code in routes(invoke.__code__):
            function = types.FunctionType(code, globals())
            for values in ((), (17, 23)):
                coro = function(lambda *, value: value, Values(values))
                try:
                    while True:
                        self.assertEqual(coro.send(None), "pause")
                except StopIteration as ended:
                    self.assertEqual(ended.value, list(values))
                finally:
                    coro.close()
            coro = function(lambda *, value: value, Values((), fail=True))
            try:
                with self.assertRaisesRegex(LookupError, "anext source"):
                    coro.send(None)
            finally:
                coro.close()


if __name__ == "__main__":
    unittest.main()
