"""The list append take path consumes its input on growth and failure."""

import unittest

from test.support.script_helper import assert_python_ok
from test.test_ir_list_paths import ROUNDS, adaptive_handlers


class ListAppendTests(unittest.TestCase):
    def test_growth_and_spare_capacity_keep_value_identity(self):
        def build(values):
            return [item for item in values]

        payload = object()
        values = [None, False, 37, 1 << 80, 1.5, float("nan"), payload] * 3
        self.assertIn("LIST_APPEND", adaptive_handlers(build))
        for _ in range(ROUNDS):
            actual = build(values)
        for before, after in zip(values, actual):
            if isinstance(before, float) and before != before:
                self.assertNotEqual(after, after)
            elif isinstance(before, (int, float)):
                self.assertEqual(after, before)
            else:
                self.assertIs(after, before)

    def test_resize_failure_consumes_the_input_owner_and_can_retry(self):
        assert_python_ok("-c", """if 1:
            import gc
            import weakref
            import _testcapi
            import _testinternalcapi

            references = []
            armed = False
            class Payload:
                pass
            def factory():
                value = Payload()
                references.append(weakref.ref(value))
                if armed:
                    _testcapi.set_nomemory(0, 1)
                return value
            def build(factory):
                return [factory() for unused in (0,)]

            for unused in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 10):
                result = build(factory)
            del result
            references.clear()
            gc.collect()
            gc.disable()
            armed = True
            failure = None
            try:
                build(factory)
            except MemoryError as error:
                failure = error
            finally:
                _testcapi.remove_mem_hooks()
                gc.enable()
            assert failure is not None
            trace = failure.__traceback__
            while trace.tb_next is not None:
                trace = trace.tb_next
            assert trace.tb_frame.f_code is build.__code__
            assert _testinternalcapi.get_ir(build.__code__)[trace.tb_lasti][0] == 'LIST_APPEND'
            failure = trace = None
            gc.collect()
            assert len(references) == 1
            assert references[0]() is None
            armed = False
            result = build(factory)
            assert len(result) == 1 and isinstance(result[0], Payload)
            assert not _testinternalcapi.gc_verify()
        """)


if __name__ == "__main__":
    unittest.main()
