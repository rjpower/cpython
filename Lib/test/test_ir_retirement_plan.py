"""Cold semantic retirement certificates while NULL homes stay authoritative."""

import marshal
import unittest

import _testinternalcapi
from test.test_ir_admission import construct
from test.test_ir_transfer_admission import encoded, make_code


NORMAL_RETURN, ERROR_UNWIND, HEAP_CLEAR, STAGED_FAILURE = range(1, 5)
UNCOVERED, EXACT, MISMATCH = range(3)
RETIRING = 1


def return_sites(code):
    return [i for i, row in enumerate(_testinternalcapi.get_ir(code))
            if row[0] == "RETURN_VALUE"]


def retiring_states():
    # Provider tests assert the complete chain, including disarmed STAGED
    # nodes. These fixtures select actual retirement records explicitly.
    return [state for state in _testinternalcapi.get_transition_shadow_status()
            if state[0] == RETIRING]


class RetirementPlanTests(unittest.TestCase):
    def routes(self, code):
        return (code, construct(code), code.replace(),
                marshal.loads(marshal.dumps(code)))

    def test_full_extent_and_canonical_null_comparison(self):
        for home in (0, 63, 64, 128, 4094):
            code = make_code([("LOAD_CONST", home), ("POP_TOP", home),
                              ("LOAD_CONST", 0), ("RETURN_VALUE", 0)],
                             slots=home + 1)
            for admitted in self.routes(code):
                with self.subTest(home=home, code=admitted):
                    self.assertEqual(return_sites(admitted), [3])
                    check = _testinternalcapi.check_ir_retirement
                    self.assertEqual(check(admitted, 3, NORMAL_RETURN), EXACT)
                    # An inline scalar owns no mortal count, but cannot occupy
                    # an inactive scratch home in this exact NULL-control shape.
                    self.assertEqual(check(admitted, 3, NORMAL_RETURN,
                                           dirty_home=home), MISMATCH)

    def test_zero_stack_request_keeps_the_physical_scratch_home(self):
        code = make_code([("LOAD_CONST", 0), ("RETURN_VALUE", 0)], slots=0)
        for admitted in self.routes(code):
            self.assertEqual(admitted.co_stacksize, 1)
            self.assertEqual(return_sites(admitted), [1])
            check = _testinternalcapi.check_ir_retirement
            self.assertEqual(check(admitted, 1, NORMAL_RETURN), EXACT)
            self.assertEqual(check(admitted, 1, NORMAL_RETURN, dirty_home=0),
                             MISMATCH)
        # Home0 is a real language input, not an absent zero-sized domain.
        bad = encoded([("IR_JUMP", 0, 0, 1), ("RETURN_VALUE", 0)])
        for route in (construct, lambda co, **kw: co.replace(**kw)):
            with self.assertRaisesRegex(ValueError, "reads unavailable scratch"):
                route(code, co_code=bad, co_stacksize=0)
        payload = marshal.dumps(code)
        self.assertEqual(payload.count(code.co_code), 1)
        with self.assertRaisesRegex(ValueError, "reads unavailable scratch"):
            marshal.loads(payload.replace(code.co_code, bad, 1))

    def test_default_still_checks_site_and_reason(self):
        def f(value):
            return value

        code = f.__code__
        sites = return_sites(code)
        self.assertEqual(len(sites), 1)
        check = _testinternalcapi.check_ir_retirement
        self.assertEqual(check(code, sites[0], NORMAL_RETURN), EXACT)
        self.assertEqual(check(code, sites[0], NORMAL_RETURN, unaligned=True),
                         UNCOVERED)
        self.assertEqual(check(code, 0, NORMAL_RETURN), UNCOVERED)
        self.assertEqual(check(code, len(code.co_code) // 8, NORMAL_RETURN),
                         UNCOVERED)
        for reason in (0, ERROR_UNWIND, HEAP_CLEAR, STAGED_FAILURE):
            self.assertEqual(check(code, sites[0], reason), UNCOVERED)

    def test_sparse_sites_and_unknown_path(self):
        def f(value):
            if value is None:
                return value
            return iter(value)

        self.assertIn("POP_JUMP_IF_NOT_NONE",
                      [row[0] for row in _testinternalcapi.get_ir(f.__code__)])
        for code in self.routes(f.__code__):
            sites = return_sites(code)
            self.assertEqual(len(sites), 2)
            self.assertEqual([_testinternalcapi.check_ir_retirement(
                code, site, NORMAL_RETURN) for site in sites],
                [EXACT, UNCOVERED])

    def test_join_cannot_recover_coverage_after_unknown(self):
        def f(value):
            if value is not None:
                iter(value)
            return value

        rows = _testinternalcapi.get_ir(f.__code__)
        self.assertIn("CALL", [row[0] for row in rows])
        for code in self.routes(f.__code__):
            sites = return_sites(code)
            self.assertEqual(len(sites), 1)
            self.assertEqual(_testinternalcapi.check_ir_retirement(
                code, sites[0], NORMAL_RETURN), UNCOVERED)

    def test_all_covered_loop_fixed_point(self):
        code = make_code([("LOAD_CONST", 0), ("POP_TOP", 0),
                          ("LOAD_CONST", 1),
                          ("POP_JUMP_IF_NONE", 1, 0, 5),
                          ("IR_JUMP", 0, 0, 0),
                          ("LOAD_CONST", 0), ("RETURN_VALUE", 0)])
        for admitted in self.routes(code):
            self.assertEqual(_testinternalcapi.check_ir_retirement(
                admitted, 6, NORMAL_RETURN), EXACT)

    def test_actual_nested_retirement_records(self):
        observations = []

        def f(value, trigger):
            return value

        class Trigger:
            def __init__(self, nested):
                self.nested = nested

            def __del__(self):
                observations.append(retiring_states())
                if self.nested:
                    f(None, Trigger(False))

        result = object()
        self.assertIs(f(result, Trigger(True)), result)
        self.assertEqual(observations,
                         [[(RETIRING, NORMAL_RETURN, EXACT, True)],
                          [(RETIRING, NORMAL_RETURN, EXACT, True),
                           (RETIRING, NORMAL_RETURN, EXACT, True)]])
        self.assertEqual(retiring_states(), [])

    def test_actual_unknown_retirement_and_nullable_binding(self):
        observations = []

        class Trigger:
            def __del__(self):
                observations.append(retiring_states())

        def unknown(value, trigger):
            iter(value)
            return value

        value = []
        self.assertIs(unknown(value, Trigger()), value)
        self.assertEqual(observations,
                         [[(RETIRING, NORMAL_RETURN, UNCOVERED, True)]])
        observations.clear()

        def nullable(trigger):
            if False:
                unbound = 1
            return None

        self.assertIn("unbound", nullable.__code__.co_varnames)
        self.assertIsNone(nullable(Trigger()))
        self.assertEqual(observations, [[(RETIRING, NORMAL_RETURN, EXACT, True)]])
        self.assertEqual(retiring_states(), [])


if __name__ == "__main__":
    unittest.main()
