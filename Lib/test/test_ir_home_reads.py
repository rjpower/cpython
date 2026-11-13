"""Required execution values and explicit nullable home transports."""

import types
import unittest

import _testinternalcapi
from test.test_ir_call_roles import routes
from test.test_ir_transfer_admission import make_code


class HomeReadTests(unittest.TestCase):
    def test_required_scalar_absence_raises_before_projection(self):
        code = make_code([
            ("UNARY_NOT", 0, 0), ("POP_TOP", 0),
            ("LOAD_CONST", 0), ("RETURN_VALUE", 0)], slots=2)
        for admitted in routes(code):
            with self.subTest(route=admitted), self.assertRaisesRegex(
                    SystemError, "absent required input"):
                types.FunctionType(admitted, {})()

    def test_required_array_gap_does_not_publish_a_container(self):
        for name in ("BUILD_TUPLE", "BUILD_LIST"):
            code = make_code([
                ("LOAD_CONST", 1), (name, 0, 1, 2), ("POP_TOP", 0),
                ("LOAD_CONST", 0), ("RETURN_VALUE", 0)], slots=3)
            for admitted in routes(code):
                with self.subTest(name=name, route=admitted), self.assertRaisesRegex(
                        SystemError, "absent required input"):
                    types.FunctionType(admitted, {})()

    def test_required_named_binding_keeps_unbound_error(self):
        code = make_code([
            ("UNARY_NOT", 1, 0), ("POP_TOP", 1),
            ("LOAD_CONST", 1), ("RETURN_VALUE", 1)],
            slots=2, locals=("missing",))
        for admitted in routes(code):
            with self.subTest(route=admitted), self.assertRaisesRegex(
                    UnboundLocalError, "missing"):
                types.FunctionType(admitted, {})()

    def test_absent_nullable_copy_swap_and_retirement(self):
        code = make_code([
            ("COPY", 0, 1), ("SWAP", 0, 2),
            ("POP_TOP", 0), ("POP_TOP", 1),
            ("LOAD_CONST", 0), ("RETURN_VALUE", 0)], slots=2,
            consts=(42,))
        for admitted in routes(code):
            with self.subTest(route=admitted):
                self.assertEqual(types.FunctionType(admitted, {})(), 42)

    def test_unbound_binding_restore_stays_nullable(self):
        def complete():
            if False:
                item = None
            [item for item in (1, 2)]
            return item

        def fail(values):
            if False:
                item = None
            try:
                [item for item in values]
            except LookupError:
                pass
            return item

        def values():
            yield 1
            raise LookupError("iteration failed")

        for original, failing in ((complete.__code__, False), (fail.__code__, True)):
            for admitted in routes(original):
                function = types.FunctionType(admitted, {})
                actual = (values(),) if failing else ()
                with self.subTest(code=original.co_name), self.assertRaises(UnboundLocalError):
                    function(*actual)

    def test_absence_probe_covers_all_physical_word_offsets(self):
        def target(value):
            return value, value

        payload = []
        _testinternalcapi.check_home_bitmap(payload)
        _testinternalcapi.check_frame_selected_copy(target.__code__, payload)
