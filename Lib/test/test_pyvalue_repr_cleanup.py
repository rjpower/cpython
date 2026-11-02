"""Public repr guard recursion, error recovery and owner lifetime."""

import ctypes
import gc
import unittest
import weakref


_repr_enter = ctypes.pythonapi.Py_ReprEnter
_repr_enter.argtypes = [ctypes.py_object]
_repr_enter.restype = ctypes.c_int
_repr_leave = ctypes.pythonapi.Py_ReprLeave
_repr_leave.argtypes = [ctypes.py_object]
_repr_leave.restype = None


class ReprCleanupTests(unittest.TestCase):
    def test_direct_entry_uses_identity_and_balances_leave(self):
        class Item:
            def __eq__(self, other):
                raise AssertionError("repr guard called equality")

            def __hash__(self):
                raise AssertionError("repr guard called hash")

        left, right = Item(), Item()
        try:
            self.assertEqual(_repr_enter(left), 0)
            self.assertEqual(_repr_enter(left), 1)
            self.assertEqual(_repr_enter(right), 0)
        finally:
            _repr_leave(right)
            _repr_leave(left)
        try:
            self.assertEqual(_repr_enter(left), 0)
        finally:
            _repr_leave(left)

    def test_guard_owns_object_until_leave(self):
        class Item:
            pass

        item = Item()
        reference = weakref.ref(item)
        self.assertEqual(_repr_enter(item), 0)
        try:
            del item
            gc.collect()
            self.assertIsNotNone(reference())
        finally:
            owner = reference()
            if owner is not None:
                _repr_leave(owner)
            del owner
        gc.collect()
        self.assertIsNone(reference())

    def test_recursive_containers_leave_guard(self):
        sequence = []
        sequence.append(sequence)
        mapping = {}
        mapping["self"] = mapping
        self.assertEqual(repr(sequence), "[[...]]")
        self.assertEqual(repr(mapping), "{'self': {...}}")
        sequence.clear()
        mapping.clear()
        self.assertEqual(repr(sequence), "[]")
        self.assertEqual(repr(mapping), "{}")

    def test_repr_error_keeps_identity_and_next_call_can_enter(self):
        error = ValueError("selected repr error")

        class Item:
            fail = True

            def __repr__(self):
                if self.fail:
                    raise error
                return "Item()"

        item = Item()
        sequence = [item]
        with self.assertRaises(ValueError) as caught:
            repr(sequence)
        self.assertIs(caught.exception, error)
        item.fail = False
        self.assertEqual(repr(sequence), "[Item()]")

    def test_container_mutation_keeps_active_repr_receiver_alive(self):
        events = []
        sequence = []

        class Item:
            def __repr__(self):
                sequence.clear()
                gc.collect()
                events.append("repr")
                return "Item()"

            def __del__(self):
                events.append("del")

        item = Item()
        reference = weakref.ref(item)
        sequence.append(item)
        del item
        self.assertEqual(repr(sequence), "[Item()]")
        self.assertEqual(events, ["repr", "del"])
        self.assertIsNone(reference())
        self.assertEqual(repr(sequence), "[]")


if __name__ == "__main__":
    unittest.main()
