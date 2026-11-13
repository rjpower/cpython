"""Whole-image created-copy certificates, queried without executing raw images."""

import marshal
import unittest
from test.support.ir_semantics import observation_tables, position_parts

import _testinternalcapi
from test.test_ir_admission import construct
from test.test_ir_transfer_admission import encoded, make_code


def routes(code):
    return (code, construct(code), code.replace(),
            marshal.loads(marshal.dumps(code)))


class CreatedPlanTests(unittest.TestCase):
    def assert_plan(self, code, expected):
        for admitted in routes(code):
            with self.subTest(code=admitted):
                self.assertIs(_testinternalcapi.has_empty_created_copy_plan(admitted),
                              expected)

    def test_empty_creation_covers_the_actual_physical_extent(self):
        for slots in (0, 1, 64, 65, 4094):
            self.assert_plan(make_code([("RETURN_GENERATOR", 4095, 1)],
                                       slots=slots, locals=("value",)), True)
        # A live inline/control-capable language home is not an empty shape.
        self.assert_plan(make_code([("LOAD_CONST", 1),
                                    ("RETURN_GENERATOR", 4095, 1)],
                                   locals=("value",)), False)

    def test_every_created_site_must_be_reached_and_empty(self):
        # Both branches consume their condition before the actual copy marker.
        words = [("LOAD_CONST", 1), ("POP_JUMP_IF_NONE", 1, 0, 4),
                 ("RETURN_GENERATOR", 4095, 1), ("IR_JUMP", 0, 0, 5),
                 ("RETURN_GENERATOR", 4095, 1),
                 ("LOAD_CONST", 1), ("RETURN_VALUE", 1)]
        self.assert_plan(make_code(words, locals=("value",)), True)
        # The first marker still qualifies, but the other retains a value.
        # Retire that value on its own branch before joining the return path.
        changed = [("LOAD_CONST", 1), ("POP_JUMP_IF_NONE", 1, 0, 4),
                   ("RETURN_GENERATOR", 4095, 1), ("IR_JUMP", 0, 0, 7),
                   ("LOAD_CONST", 2), ("RETURN_GENERATOR", 4095, 1),
                   ("POP_TOP", 2), ("LOAD_CONST", 1), ("RETURN_VALUE", 1)]
        self.assert_plan(make_code(changed, locals=("value",)), False)
        # An unreachable physical marker must not disappear from the census.
        self.assert_plan(make_code([
            ("IR_JUMP", 0, 0, 2), ("RETURN_GENERATOR", 4095, 1),
            ("RETURN_GENERATOR", 4095, 1)], locals=("value",)), False)

    def test_observer_recovery_entry_cannot_inherit_normal_entry_proof(self):
        code = make_code([("RETURN_GENERATOR", 4095, 1)], locals=("value",))
        locations, _ = position_parts(code.co_linetable)
        event = (0, 0, 0, code.co_firstlineno, code.co_firstlineno, 0, 1, 0, 1, 0)
        self.assert_plan(code.replace(**observation_tables(locations, [event])), False)

    def test_free_variable_count_is_admitted_before_local_only_transfer(self):
        code = make_code([("COPY_FREE_VARS", 0),
                          ("RETURN_GENERATOR", 4095, 1)], locals=("value",))
        self.assert_plan(code, True)
        changed = encoded([("COPY_FREE_VARS", 1),
                           ("RETURN_GENERATOR", 4095, 1)])
        for route in (construct, lambda co, **kw: co.replace(**kw)):
            with self.assertRaisesRegex(ValueError, "free-variable count"):
                route(code, co_code=changed)
        payload = marshal.dumps(code)
        self.assertEqual(payload.count(code.co_code), 1)
        with self.assertRaisesRegex(ValueError, "free-variable count"):
            marshal.loads(payload.replace(code.co_code, changed, 1))


if __name__ == "__main__":
    unittest.main()
