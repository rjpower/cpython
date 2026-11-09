"""Loop/send outcomes own their results and real continuations."""

import dis
import unittest

from test.support import import_helper

_testinternalcapi = import_helper.import_module("_testinternalcapi")
ROUNDS = _testinternalcapi.SPECIALIZATION_THRESHOLD + 10


def opnames(function):
    return [instruction.opname for instruction in dis.get_instructions(function)]


class SemanticCompletionTests(unittest.TestCase):
    def test_for_iter_preserves_last_local_on_generator_return(self):
        item = object()
        returned = object()

        def source():
            yield item
            return returned

        def consume():
            last = None
            for last in source():
                pass
            return last

        for _ in range(ROUNDS):
            self.assertIs(consume(), item)
        self.assertNotIn("END_FOR", opnames(consume))
        self.assertNotIn("POP_ITER", opnames(consume))

    def test_native_for_iter_outcomes(self):
        class Iterator:
            def __init__(self):
                self.index = 0

            def __iter__(self):
                return self

            def __next__(self):
                self.index += 1
                if self.index == 4:
                    raise StopIteration("not a loop item")
                return self.index

        def consume(values):
            total = 0
            last = None
            for last in values:
                total += last
            return total, last

        for factory in (lambda: [1, 2, 3], lambda: (1, 2, 3),
                        lambda: range(1, 4), Iterator):
            with self.subTest(factory=factory):
                for _ in range(ROUNDS):
                    self.assertEqual(consume(factory()), (6, 3))
        self.assertEqual(consume([]), (0, None))
        self.assertNotIn("END_FOR", opnames(consume))
        self.assertNotIn("POP_ITER", opnames(consume))

    def test_send_completion_separate_from_yield_recipient(self):
        marker = object()

        def inner():
            yield "ready"
            return marker

        def outer():
            completed = yield from inner()
            return completed

        for _ in range(ROUNDS):
            generator = outer()
            self.assertEqual(next(generator), "ready")
            with self.assertRaises(StopIteration) as stopped:
                generator.send(None)
            self.assertIs(stopped.exception.value, marker)
        self.assertNotIn("END_SEND", opnames(outer))

    def test_cleanup_throw_completes_same_send_result(self):
        marker = object()

        class Delegate:
            def __iter__(self):
                return self

            def __next__(self):
                return "ready"

            def throw(self, *exception):
                raise StopIteration(marker)

        def outer():
            result = yield from Delegate()
            return result

        generator = outer()
        self.assertEqual(next(generator), "ready")
        with self.assertRaises(StopIteration) as stopped:
            generator.throw(ValueError("injected"))
        self.assertIs(stopped.exception.value, marker)
        self.assertNotIn("END_SEND", opnames(outer))

    def test_ignored_resume_input_and_created_send_validation(self):
        def ignore():
            yield "first"
            yield "second"

        generator = ignore()
        self.assertEqual(generator.gi_frame.f_code, ignore.__code__)
        with self.assertRaisesRegex(TypeError, "just-started generator"):
            generator.send(object())
        self.assertEqual(next(generator), "first")
        self.assertEqual(generator.send(object()), "second")
        with self.assertRaises(StopIteration):
            generator.send(object())
        self.assertNotIn("POP_TOP", opnames(ignore))

    def test_init_cleanup_real_continuation(self):
        class Normal:
            def __init__(self, value):
                self.value = value

        class Invalid:
            def __init__(self):
                return 17

        for _ in range(ROUNDS):
            self.assertEqual(Normal(31).value, 31)
            with self.assertRaisesRegex(TypeError, "__init__.*return None"):
                Invalid()


if __name__ == "__main__":
    unittest.main()
