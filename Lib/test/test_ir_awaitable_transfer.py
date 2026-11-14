"""Canonical owner transfer for the exact-coroutine awaitable identity arm."""

import gc
import marshal
import sys
import types
import unittest

from test.support import import_helper
from test.test_ir_admission import construct
from test.test_ir_transfer_admission import make_code

_testinternalcapi = import_helper.import_module("_testinternalcapi")


def routes(code):
    return code, construct(code), code.replace(), marshal.loads(marshal.dumps(code))


def finish(coro):
    try:
        while True:
            coro.send(None)
    except StopIteration as stopped:
        return stopped.value


class AwaitableTransferTests(unittest.TestCase):
    def test_folded_recipient_alias_discard_and_displacement(self):
        async def result():
            return 7

        for scratch in (False, True):
            for destination in ((0, 1, 3, 4095) if scratch else (0, 1, 4095)):
                words = [("RESUME", 0)]
                source = 0
                if scratch:
                    words.append(("LOAD_FAST_AND_CLEAR", 3, 0))
                    source = 3
                words.append(("GET_AWAITABLE", destination, source, 0))
                if destination == 4095:
                    # Discarding a scratch result leaves no owner to return.
                    words.extend([("LOAD_CONST", 3), ("RETURN_VALUE", 3)])
                else:
                    words.append(("RETURN_VALUE", destination))
                code = make_code(words, slots=2,
                    locals=("value", "target", "mutable")).replace(co_argcount=3)
                for admitted in routes(code):
                    with self.subTest(scratch=scratch, destination=destination,
                                      code=admitted):
                        events = []
                        child = result()

                        class Previous:
                            def __del__(self):
                                frame = sys._getframe(1)
                                events.append((frame.f_code is admitted,
                                               frame.f_locals["target"] is child))
                                frame.f_locals["mutable"] = 42
                                gc.collect()
                                events.append(frame.f_locals["mutable"])

                        function = types.FunctionType(admitted, {})
                        try:
                            value = function(child, Previous() if destination == 1 else None, 17)
                            if destination == 4095:
                                self.assertIsNone(value)
                            else:
                                self.assertIs(value, child)
                            if destination == 1:
                                self.assertEqual(events, [(True, True), 42])
                            else:
                                self.assertEqual(events, [])
                        finally:
                            child.close()

    def test_exact_coroutine_from_scratch_and_binding(self):
        async def result(value):
            return value

        async def from_scratch(factory):
            return await factory()

        async def from_binding(value):
            return await value

        for template, scratch in ((from_scratch, True), (from_binding, False)):
            for code in routes(template.__code__):
                with self.subTest(scratch=scratch, code=code):
                    call = types.FunctionType(code, globals())
                    row = next(row for row in _testinternalcapi.get_ir(code)
                               if row[0] == "GET_AWAITABLE")
                    nbindings = len(_testinternalcapi.get_co_localskinds(code))
                    if scratch:
                        self.assertGreaterEqual(row[2], nbindings)
                    else:
                        self.assertEqual(row[2], 0)
                    for value in (object(), 17, 3.25, None):
                        child = result(value)
                        argument = (lambda: child) if scratch else child
                        try:
                            self.assertIs(finish(call(argument)), value)
                        finally:
                            child.close()

    def test_rejected_and_later_error_states(self):
        async def await_value(value):
            return await value

        class Pause:
            def __await__(self):
                yield "paused"

        async def suspended():
            await Pause()

        async def completed():
            return 7

        for code in routes(await_value.__code__):
            call = types.FunctionType(code, globals())
            child = suspended()
            self.assertEqual(child.send(None), "paused")
            outer = call(child)
            try:
                with self.assertRaisesRegex(RuntimeError, "being awaited already"):
                    finish(outer)
            finally:
                outer.close()
                child.close()
            child = completed()
            self.assertEqual(finish(child), 7)
            outer = call(child)
            try:
                with self.assertRaisesRegex(RuntimeError, "cannot reuse already awaited coroutine"):
                    finish(outer)
            finally:
                outer.close()

        active = None

        async def executing():
            return await active

        active = executing()
        try:
            with self.assertRaisesRegex(ValueError, "already executing"):
                finish(active)
        finally:
            active.close()

    def test_custom_await_keeps_callback_and_error_behavior(self):
        async def await_value(value):
            return await value

        marker = object()
        events = []

        def iterator():
            if False:
                yield
            return marker

        class Custom:
            def __await__(self):
                parent = sys._getframe(1)
                events.append(parent.f_code)
                parent.f_locals["value"] = marker
                gc.collect()
                return iterator()

        class Invalid:
            def __await__(self):
                events.append("invalid")
                return 17

        class Raises:
            def __await__(self):
                events.append("raises")
                gc.collect()
                raise LookupError("await callback")

        for code in routes(await_value.__code__):
            call = types.FunctionType(code, globals())
            custom = Custom()
            events.clear()
            self.assertIs(finish(call(custom)), marker)
            self.assertEqual(events, [code])
            for value, error, message, event in (
                    (Invalid(), TypeError, "must return an iterator", "invalid"),
                    (Raises(), LookupError, "await callback", "raises")):
                events.clear()
                with self.assertRaisesRegex(error, message):
                    finish(call(value))
                self.assertEqual(events, [event])
            for value in (None, 17, 3.25, object()):
                with self.assertRaisesRegex(TypeError, "can't be awaited"):
                    finish(call(value))


if __name__ == "__main__":
    unittest.main()
