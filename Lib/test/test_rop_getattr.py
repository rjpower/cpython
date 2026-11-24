"""Tests of the GetAttr and GetMethod families of the register instruction
definitions (Python/register_ops/getattr.rop) and their generated C."""

import io
import sys
import unittest
from pathlib import Path

from test import test_tools
from test.support import import_helper

test_tools.skip_if_missing("cases_generator")
with test_tools.imports_under_tool("cases_generator"):
    import rop_check
    import rop_generator
    import rop_verify

ROOT = Path(test_tools.basepath)
SPECIALIZED = {
    "GetAttr": ["Instance", "Shape", "PolyShape", "Module", "Hint", "ValueSlot",
                "PointerSlot", "Class", "NonDescriptorValues", "NonDescriptorNoDict",
                "GetSet"],
    "GetMethod": ["MethodValues", "PolyMethod", "NoDict", "LazyDict", "ClassMethod"],
}


def program():
    return rop_verify.load(sorted((ROOT / "Python" / "register_ops").glob("*.rop")))


def cases_text(p) -> str:
    out = io.StringIO()
    rop_generator.write_cases(out, p, [])
    return out.getvalue()


def arm_text(text: str, arm: str) -> str:
    start = text.index(f"TARGET({arm.replace('.', '_')})")
    end = text.find("TARGET(", start + 1)
    return text[start:] if end < 0 else text[start:end]


class DefinitionTests(unittest.TestCase):
    def test_definitions_check_clean(self):
        self.assertEqual(list(program().diagnostics), [])

    def test_every_arm_is_defined(self):
        arms = program().arms
        for family, names in SPECIALIZED.items():
            for name in names + ["Generic"]:
                with self.subTest(arm=f"{family}.{name}"):
                    self.assertIn(f"{family}.{name}", arms)

    def test_generics_are_shared_and_complete(self):
        arms = program().arms
        for family in SPECIALIZED:
            generic = arms[f"{family}.Generic"]
            self.assertEqual(generic.region.name, "shared")
            self.assertLessEqual({"next", "raise", "again", "redirect"},
                                 set(generic.contract.exits))

    def test_only_the_call_boundary_is_pending_for_generics(self):
        # Pending rows belong to the shared call-boundary macros, which are
        # written once for every family
        p = program()
        for family in SPECIALIZED:
            blockers = rop_generator.pending_blockers(p, p.arms[f"{family}.Generic"])
            self.assertTrue(all(b.startswith(("adapt_before_use", "anchor_for_call",
                                              "retire_", "tick", "backoff", "install",
                                              "getattr_at_origin", "getmethod_at_origin"))
                                for b in blockers), blockers)


class GeneratedCTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        prog = program()
        numbers = rop_generator.number_arms(prog)
        cls.text = cases_text(rop_generator.lower_all(prog, numbers))

    def test_specialized_arms_are_emitted(self):
        for family, names in SPECIALIZED.items():
            for name in names:
                with self.subTest(arm=f"{family}.{name}"):
                    self.assertIn(f"TARGET({family}_{name})", self.text)

    def test_no_raw_bits(self):
        self.assertNotIn(".bits", self.text)

    def test_instance_guards_precede_the_duplicate(self):
        body = arm_text(self.text, "GetAttr.Instance")
        order = ["_PyAttr_InstanceVersion", "_PyAttr_ValidInlineValues",
                 "_PyAttr_BorrowValueSlot", "_PyValue_BorrowPresent", "PyValue_DUP"]
        positions = [body.index(name) for name in order]
        self.assertEqual(positions, sorted(positions))

    def test_method_takes_the_receiver_after_its_guards(self):
        body = arm_text(self.text, "GetMethod.MethodValues")
        self.assertLess(body.index("_PyAttr_ValidInlineValues"),
                        body.index("_PyFrame_TakeHome"))
        # The callable and the receiver are two owners and two homes
        self.assertEqual(body.count("_PyFrame_WriteHome"), 2)

    def test_feedback_sits_in_an_operand_word_of_getmethod(self):
        body = arm_text(self.text, "GetMethod.MethodValues")
        self.assertIn("next_instr += 2", body)
        self.assertIn("this_instr[1]", body)

    def test_poly_arms_find_their_row_by_the_frame(self):
        self.assertIn("_PyAttr_PolyShape(frame,", arm_text(self.text, "GetAttr.PolyShape"))
        self.assertIn("_PyAttr_PolyMethod(frame,", arm_text(self.text, "GetMethod.PolyMethod"))


class TranslatorRuleTests(unittest.TestCase):
    """LOAD_ATTR's rule (Python/regcfg.c rule_load_attr) selects the family's
    Generic arm."""

    def test_load_attr_words_are_the_family_generics(self):
        capi = import_helper.import_module("_testinternalcapi")
        namespace = {}
        exec("def attr(o):\n    return o.x\n"
             "def method(o):\n    return o.m()\n", namespace)
        names = {}
        for name in ("attr", "method"):
            code = namespace[name].__code__
            names[name] = [row[0] for row in capi.get_ir(code, False)]
        self.assertIn("GetAttr.Generic", names["attr"])
        self.assertIn("GetMethod.Generic", names["method"])
        # The callable and self land in the CALL's window: no move between
        self.assertNotIn("MOVE", names["method"])
        self.assertNotIn("SWAP", names["method"])


class _Slots:
    __slots__ = ()

    def m(self, y):
        return y


class _Values:
    def __init__(self):
        self.x = 1

    def m(self, y):
        return y


class _ClassMethod:
    @classmethod
    def m(cls, y):
        return y


class MethodOwnershipTests(unittest.TestCase):
    """A method call through each GetMethod arm leaves the count of the
    function in the type's dict where it was: the callable the arm publishes
    is owned by the call's window and retired once."""

    N = 20000

    def drift(self, site, descriptor):
        for _ in range(200):
            site()
        before = sys.getrefcount(descriptor)
        for _ in range(self.N):
            site()
        self.assertEqual(sys.getrefcount(descriptor), before)

    def test_no_dict_builtin(self):
        items = []

        def site():
            items.append(1)
            items.pop()
        self.drift(site, list.__dict__["append"])

    def test_no_dict_python_method(self):
        obj = _Slots()
        self.drift(lambda: obj.m(1), _Slots.__dict__["m"])

    def test_method_values(self):
        obj = _Values()
        self.drift(lambda: obj.m(1), _Values.__dict__["m"])

    def test_class_method(self):
        obj = _ClassMethod()
        self.drift(lambda: obj.m(1), _ClassMethod.__dict__["m"].__func__)

    def test_poly_site(self):
        objs = [_Values(), _Slots(), _Values()]

        def site():
            for o in objs:
                o.m(1)
        self.drift(site, _Values.__dict__["m"])
        self.drift(site, _Slots.__dict__["m"])

    def test_generic_miss_after_specialising(self):
        # A site that specialised for one type then sees others and
        # attributes that are not methods
        class Late:
            m = 5

        def site(o):
            return o.m

        objs = [_Values(), _Values(), Late()]
        for _ in range(100):
            site(objs[0])
        self.assertEqual(site(objs[2]), 5)
        self.assertEqual(site(objs[1])(1), 1)


if __name__ == "__main__":
    unittest.main()
