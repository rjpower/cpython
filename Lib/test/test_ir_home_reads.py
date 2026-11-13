"""Required execution values and explicit nullable home transports."""

import opcode
import types
import unittest

import _testinternalcapi
from test.test_ir_call_roles import routes
from test.test_ir_transfer_admission import make_code


class HomeReadTests(unittest.TestCase):
    def test_delete_name_accepts_null_in_module_and_class_namespace(self):
        sources = ("item = 42\ndel item\n", "del item\n")
        for source in sources:
            module = compile(source, "<delete-name>", "exec")
            wrapper = compile("class Target:\n" + "".join(
                "    " + line + "\n" for line in source.splitlines()),
                "<delete-name>", "exec")
            class_body = next(value for value in wrapper.co_consts
                              if isinstance(value, types.CodeType))
            for scope, code in (("module", module), ("class", class_body)):
                for admitted in routes(code):
                    with self.subTest(scope=scope, source=source):
                        namespace = {}
                        if source.startswith("item ="):
                            exec(admitted, {"__name__": __name__}, namespace)
                            self.assertNotIn("item", namespace)
                        else:
                            with self.assertRaisesRegex(NameError, "item"):
                                exec(admitted, {"__name__": __name__}, namespace)

    def test_other_compiler_delete_protocols_keep_their_roles(self):
        code = compile("global item\ndel item\n", "<delete-global>", "exec")
        for admitted in routes(code):
            namespace = {"item": 42}
            exec(admitted, namespace)
            self.assertNotIn("item", namespace)
            with self.assertRaisesRegex(NameError, "item"):
                exec(admitted, namespace)

        def attribute(owner):
            del owner.item

        def subscript(owner):
            del owner["item"]

        def slice_(owner):
            del owner[1:3]

        for original, factory, error in (
                (attribute, lambda: types.SimpleNamespace(item=42), AttributeError),
                (subscript, lambda: {"item": 42}, KeyError)):
            for admitted in routes(original.__code__):
                function = types.FunctionType(admitted, {})
                owner = factory()
                function(owner)
                with self.subTest(protocol=original.__name__), self.assertRaises(error):
                    function(owner)
        for admitted in routes(slice_.__code__):
            owner = [0, 1, 2, 3]
            types.FunctionType(admitted, {})(owner)
            self.assertEqual(owner, [0, 3])

    def test_large_attribute_delete_uses_nullable_setattr_intrinsic(self):
        namespace = {}
        source = ("def target(owner, visit):\n    if visit:\n" + "".join(
            f"        owner.attr_{i}\n" for i in range(4096)) +
            "    del owner.attr_4096\n")
        exec(source, namespace)
        code = namespace["target"].__code__
        self.assertGreaterEqual(code.co_names.index("attr_4096"), 4096)
        self.assertTrue(any(row[0] == "CALL_INTRINSIC_2"
                            for row in _testinternalcapi.get_ir(code)))
        for admitted in routes(code):
            function = types.FunctionType(admitted, {})
            owner = types.SimpleNamespace(attr_4096=42)
            function(owner, False)
            self.assertEqual(vars(owner), {})
            with self.assertRaises(AttributeError):
                function(owner, False)

    def test_other_binary_intrinsics_reject_absent_second_value(self):
        for name in ("INTRINSIC_GETATTR", "INTRINSIC_TYPEVAR_WITH_BOUND"):
            intrinsic = opcode._intrinsic_2_descs.index(name)
            code = make_code([
                ("LOAD_CONST", 2), ("CALL_INTRINSIC_2", 0, 1, 2, intrinsic),
                ("RETURN_VALUE", 0)], slots=3, consts=("name",))
            for admitted in routes(code):
                with self.subTest(intrinsic=name), self.assertRaisesRegex(
                        SystemError, "absent required input"):
                    types.FunctionType(admitted, {})()

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
