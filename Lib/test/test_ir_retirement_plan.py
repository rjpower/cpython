"""Cold semantic retirement certificates while NULL homes stay authoritative."""

import marshal
import unittest

import _testinternalcapi
from test.test_ir_admission import construct


NORMAL_RETURN, ERROR_UNWIND, HEAP_CLEAR, STAGED_FAILURE = range(1, 5)
UNCOVERED, EXACT, MISMATCH = range(3)


def return_sites(code):
    return [i for i, row in enumerate(_testinternalcapi.get_ir(code))
            if row[0] in {"RETURN_VALUE", "RETURN_VALUE_EMPTY",
                          "INSTRUMENTED_RETURN_VALUE", "INSTRUMENTED_RETURN_VALUE_EMPTY"}]


class RetirementPlanTests(unittest.TestCase):
    def routes(self, code):
        return (code, construct(code), code.replace(),
                marshal.loads(marshal.dumps(code)))

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
        self.assertEqual(check(code, len(_testinternalcapi.get_ir(code)), NORMAL_RETURN),
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
        self.assertIn("CALL+NO_SELF", [row[0] for row in rows])
        for code in self.routes(f.__code__):
            sites = return_sites(code)
            self.assertEqual(len(sites), 1)
            self.assertEqual(_testinternalcapi.check_ir_retirement(
                code, sites[0], NORMAL_RETURN), UNCOVERED)


if __name__ == "__main__":
    unittest.main()
