"""Initialized frame storage survives reuse without changing ownership."""

import gc
import sys
import unittest
import weakref

from test.support import import_helper
from test.support.script_helper import assert_python_ok

_testinternalcapi = import_helper.import_module("_testinternalcapi")


def make_leaf(count):
    namespace = {}
    body = "\n".join(f"    local_{i} = token" for i in range(count))
    exec(f"def leaf(token):\n{body}\n    return local_{count - 1}\n", namespace)
    return namespace["leaf"]


class FrameArenaTests(unittest.TestCase):
    def test_dirty_allocator_and_former_header_reuse(self):
        assert_python_ok("-c", "import _testinternalcapi; _testinternalcapi.test_frame_arena()")

    def test_varying_overlap_and_provider_entry(self):
        marker = object()
        leaves = [make_leaf(size) for size in (1, 17, 400, 3, 700, 2)]
        for _ in range(30):
            for leaf in leaves:
                self.assertIs(leaf(marker), marker)

        class Accessor:
            def __getitem__(self, key):
                return leaves[key](marker)

            @property
            def value(self):
                return leaves[2](marker)

        accessor = Accessor()
        for _ in range(30):
            self.assertIs(accessor[4], marker)
            self.assertIs(accessor.value, marker)

    def test_binding_and_recursion_failure_retirement(self):
        leaf = make_leaf(300)
        marker = object()

        def required(a, b, *, c):
            return a, b, c

        for _ in range(30):
            with self.assertRaises(TypeError):
                required(marker)
            with self.assertRaises(TypeError):
                required(marker, marker, c=marker, extra=marker)
            self.assertIs(leaf(marker), marker)

        def recurse():
            leaf(marker)
            return recurse()

        with self.assertRaises(RecursionError):
            recurse()
        self.assertIs(leaf(marker), marker)

    def test_heap_frame_generator_and_escape(self):
        marker = object()
        leaf = make_leaf(450)

        def escape(token):
            return sys._getframe()

        def suspended(token):
            yield token
            return token

        frame = escape(marker)
        generator = suspended(marker)
        for _ in range(30):
            self.assertIs(leaf(marker), marker)
        self.assertIs(frame.f_locals["token"], marker)

        self.assertIs(generator.gi_frame.f_locals["token"], marker)
        self.assertIs(next(generator), marker)
        for _ in range(30):
            self.assertIs(leaf(marker), marker)
        with self.assertRaises(StopIteration) as stopped:
            next(generator)
        self.assertIs(stopped.exception.value, marker)
        self.assertIs(frame.f_locals["token"], marker)

    def test_direct_heap_frame_initializer(self):
        _testcapi = import_helper.import_module("_testcapi")
        leaf = make_leaf(400)
        frame = _testcapi.frame_new(leaf.__code__, globals(), {})
        self.assertIs(frame.f_code, leaf.__code__)
        self.assertEqual(frame.f_locals, {})
        gc.collect()
        self.assertEqual(frame.f_locals, {})

    def test_retirement_reentry_and_eventual_exactly_once(self):
        leaf = make_leaf(500)
        marker = object()
        events = []
        references = []

        class Reenter:
            def __init__(self, identity):
                self.identity = identity
                references.append(weakref.ref(self))

            def __del__(self):
                events.append((self.identity, leaf(marker) is marker))

        def retire(identity, fail):
            owner = Reenter(identity)
            if fail:
                raise ValueError("retire on unwind")
            return None

        for identity in range(60):
            if identity % 2:
                with self.assertRaises(ValueError):
                    retire(identity, True)
            else:
                retire(identity, False)
        gc.collect()
        self.assertEqual(sorted(events), [(i, True) for i in range(60)])
        self.assertTrue(all(reference() is None for reference in references))


if __name__ == "__main__":
    unittest.main()
