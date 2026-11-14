"""List iteration and append retain canonical values and explicit owners."""

import gc
import struct
import sys
import types
import unittest
import weakref

import _testinternalcapi

from test import support
from test.test_ir_call_roles import routes
from test.test_ir_transfer_admission import HANDLERS


ROUNDS = _testinternalcapi.SPECIALIZATION_THRESHOLD + 10


def adaptive_handlers(function):
    return {row[0] for row in _testinternalcapi.get_ir(function.__code__, True)}


class ListIterationTests(unittest.TestCase):
    def test_canonical_values_and_local_recipients(self):
        def consume(values):
            result = []
            for item in values:
                result.append(item)
            return result

        values = [None, False, 37, 1 << 80, 1.5, float("nan"), object()]
        for _ in range(ROUNDS):
            result = consume(values)
        if not support.Py_GIL_DISABLED:
            self.assertIn("FOR_ITER_LIST", adaptive_handlers(consume))
        for value, actual in zip(values, result):
            if isinstance(value, float) and value != value:
                self.assertNotEqual(actual, actual)
            elif isinstance(value, (int, float)):
                self.assertEqual(actual, value)
            else:
                self.assertIs(actual, value)
        self.assertEqual(consume([]), [])

        class Override(list):
            def __iter__(self):
                return iter((17, 23))

        self.assertEqual(consume(Override(values)), [17, 23])

    def test_list_mutation_is_checked_again_on_each_iteration(self):
        def consume(values, change):
            result = []
            for item in values:
                result.append(item)
                if item == 1:
                    change(values)
            return result

        for _ in range(ROUNDS):
            self.assertEqual(consume([1, 2], lambda values: None), [1, 2])
        if not support.Py_GIL_DISABLED:
            self.assertIn("FOR_ITER_LIST", adaptive_handlers(consume))
        self.assertEqual(consume([1, 2], lambda values: values.append(3)), [1, 2, 3])
        self.assertEqual(consume([1, 2], lambda values: values.clear()), [1])
        self.assertEqual(consume([1, 2, 3], lambda values: values.pop(0)), [1, 3])

    def test_discard_recipient_releases_temporary_elements(self):
        def consume(values):
            for unused in values:
                pass
            return 7

        code = consume.__code__
        rows = _testinternalcapi.get_ir(code)
        site = next(pc for pc, row in enumerate(rows) if row[0] == "FOR_ITER")
        words = list(struct.unpack(f"<{len(code.co_code) // 8}Q", code.co_code))
        words[site] = (words[site] & ~(4095 << 16)) | (4095 << 16)
        for pc, row in enumerate(rows):
            if row[0] == "RETURN_VALUE_EMPTY":
                words[pc] = (words[pc] & ~65535) | HANDLERS["RETURN_VALUE"]
        code = code.replace(co_code=struct.pack(f"<{len(words)}Q", *words),
                            co_linetable=b"", co_exceptiontable=code.co_exceptiontable)
        class Payload:
            pass
        for admitted in routes(code):
            function = types.FunctionType(admitted, globals())
            for _ in range(ROUNDS):
                self.assertEqual(function([None, 17, 3.5]), 7)
            if not support.Py_GIL_DISABLED:
                self.assertIn("FOR_ITER_LIST", adaptive_handlers(function))
            payload = Payload()
            reference = weakref.ref(payload)
            self.assertEqual(function([payload]), 7)
            del payload
            gc.collect()
            self.assertIsNone(reference())

    def test_replaced_local_finalizer_sees_the_published_element(self):
        events = []

        class Payload:
            pass

        class Retired:
            def __del__(self):
                frame = sys._getframe(1)
                while frame is not None and frame.f_code is not consume.__code__:
                    frame = frame.f_back
                if frame is None:
                    events.append("missing frame")
                    return
                events.append(isinstance(frame.f_locals["item"], Payload))
                frame.f_locals["mutable"] = 42
                gc.collect()
                events.append(tuple(_testinternalcapi.gc_verify()))

        def consume(factory):
            mutable = 0
            item = Retired()
            for item in factory():
                pass
            return item, mutable

        def factory():
            return [Payload()]

        for _ in range(ROUNDS):
            result, mutable = consume(factory)
        if not support.Py_GIL_DISABLED:
            self.assertIn("FOR_ITER_LIST", adaptive_handlers(consume))
        events.clear()
        result, mutable = consume(factory)
        self.assertEqual(events, [True, ()])
        self.assertEqual(mutable, 42)
        reference = weakref.ref(result)
        del result
        gc.collect()
        self.assertIsNone(reference())


if __name__ == "__main__":
    unittest.main()
