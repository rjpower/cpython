"""Canonical element ownership: the item arms guard, then duplicate the element."""

import gc
import sys
import types
import unittest
import weakref

from test import support


class CanonicalElementRuntimeTests(unittest.TestCase):
    def test_specialized_bounds_values_and_generic_misses(self):
        import _testinternalcapi

        def template(container, index):
            return container[index]

        values = (None, True, 37, 1 << 80, 1.5, float("nan"), object())
        for kind in (list, tuple):
            read = types.FunctionType(template.__code__.replace(), globals())
            container = kind(values)
            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 10):
                self.assertIs(read(container, 0), values[0])
            expected = f"GetItem.{kind.__name__.capitalize()}Index"
            if kind is tuple or not support.Py_GIL_DISABLED:
                self.assertIn(expected, {word[0] for word in
                                        _testinternalcapi.get_ir(read.__code__, True)})
            for index, value in enumerate(values):
                result = read(container, index)
                if isinstance(value, float) and value != value:
                    self.assertNotEqual(result, result)
                elif isinstance(value, (int, float)):
                    self.assertEqual(result, value)
                else:
                    self.assertIs(result, value)
            self.assertIs(read(container, -1), values[-1])
            for index in (len(values), -len(values) - 1, 1 << 80):
                with self.subTest(kind=kind, index=index), self.assertRaises(IndexError):
                    read(container, index)

            calls = []

            class Override(kind):
                def __getitem__(self, index):
                    calls.append(index)
                    return values[-1]

            self.assertIs(read(Override(values), 0), values[-1])
            self.assertEqual(calls, [0])

    def test_temporary_anchor_finalizer_observes_published_result(self):
        import _testinternalcapi

        def read(factory):
            mutable = 0
            result = factory()[0]
            return result, mutable

        class Payload:
            pass

        events = []

        class Retired:
            def __del__(self):
                frame = sys._getframe(1)
                while frame is not None and frame.f_code is not read.__code__:
                    frame = frame.f_back
                if frame is None:
                    events.append(("missing frame",))
                    return
                frame.f_locals["mutable"] = 42
                gc.collect()
                events.append(tuple(_testinternalcapi.gc_verify()))

        for kind in (list, tuple):
            def factory():
                return kind((Payload(), Retired()))

            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 10):
                result, mutable = read(factory)
                self.assertEqual(mutable, 42)
            events.clear()
            result, mutable = read(factory)
            self.assertEqual(events, [()])
            self.assertEqual(mutable, 42)
            reference = weakref.ref(result)
            self.assertIs(reference(), result)
            del result
            gc.collect()
            self.assertIsNone(reference())


if __name__ == "__main__":
    unittest.main()
