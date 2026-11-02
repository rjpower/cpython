import gc
import unittest
import weakref

from _testinternalcapi import (
    format_engine_leases,
    format_engine_resource_failure,
    format_engine_resources,
)


HINTS = (0, 8, 17)
ALL_EVENTS = tuple(range(17))
ALL_COUNTS = (1,) * 17 + (0,)
SUCCESSFUL_FAILURE_STATUSES = (0, 0, 0, 0, None)
FAILED_EXPLICIT_STATUSES = (0, 0, -1, -1, None)
FAILED_IMPLICIT_STATUSES = (None, None, -1, -1, None)


class FormatEngineFixturesTests(unittest.TestCase):
    def assert_failure_report(self, report, error, counts, events, statuses):
        observed, actual_counts, actual_events, actual_statuses = report
        self.assertIs(actual_counts.__class__, tuple)
        self.assertIs(actual_events.__class__, tuple)
        self.assertIs(actual_statuses.__class__, tuple)
        self.assertEqual(len(actual_counts), 18)
        self.assertEqual(len(actual_statuses), 5)
        self.assertIs(observed, error)
        self.assertEqual(actual_counts, counts)
        self.assertEqual(actual_events, events)
        self.assertEqual(actual_statuses, statuses)

    def test_lease_vector_and_callback(self):
        calls = []
        nested_results = []

        class Item:
            pass

        for integer in (32, 2**40, 2**200):
            with self.subTest(integer_bits=integer.bit_length()):
                obj = Item()
                ref = weakref.ref(obj)
                values = (0, integer, 0.0, obj)

                def callback(received):
                    calls.append(received)
                    nested_results.append(format_engine_resources(0))

                result = format_engine_leases((values, callback))
                self.assertIs(type(result), tuple)
                self.assertEqual(len(result), 3)
                copied, lease_count, truth = result
                self.assertIs(type(copied), tuple)
                self.assertEqual(len(copied), 4)
                self.assertIs(type(copied[0]), int)
                self.assertIs(type(copied[1]), int)
                self.assertIs(type(copied[2]), float)
                self.assertIs(copied[3], obj)
                self.assertEqual(copied, values)
                self.assertIs(type(lease_count), int)
                self.assertEqual(lease_count, 17)
                self.assertIs(type(truth), bool)
                self.assertIs(truth, True)
                self.assertEqual(len(calls), 1)
                self.assertIs(calls[0], values)
                self.assertEqual(
                    nested_results[-1],
                    ((1,) * 17, ALL_EVENTS, (1, 1, 0, 0, None)),
                )

                del values, obj, callback
                calls.clear()
                del result, copied
                gc.collect()
                self.assertIsNone(ref())

    def test_successful_resource_transfer(self):
        for hint in HINTS:
            with self.subTest(hint=hint):
                counts, events, statuses = format_engine_resources(hint)
                self.assertIs(type(counts), tuple)
                self.assertIs(type(events), tuple)
                self.assertIs(type(statuses), tuple)
                self.assertEqual(counts, (1,) * 17)
                self.assertEqual(events, ALL_EVENTS)
                self.assertEqual(statuses, (1, 1, 0, 0, None))

    def test_explicit_and_implicit_cleanup(self):
        for mode, statuses in (
            ("explicit", SUCCESSFUL_FAILURE_STATUSES),
            ("implicit", (None, None, 0, 0, None)),
        ):
            for hint in HINTS:
                with self.subTest(mode=mode, hint=hint):
                    report = format_engine_resource_failure(
                        (mode, hint, None, None, None)
                    )
                    self.assert_failure_report(
                        report, None, ALL_COUNTS, ALL_EVENTS, statuses
                    )

    def test_register_allocation_faults(self):
        cases = (
            ("first_alloc", 0, (0,)),
            ("first_alloc", 8, (0,)),
            ("overflow_alloc", 0, (16, *range(16))),
            ("overflow_alloc", 8, (16, *range(16))),
            ("overflow_alloc", 17, (17, *range(17))),
        )
        for mode, hint, events in cases:
            with self.subTest(mode=mode, hint=hint):
                later = LookupError("cleanup after allocation fault")
                report = format_engine_resource_failure(
                    (mode, hint, None, None, later)
                )
                counts = tuple(1 if index in events else 0 for index in range(18))
                self.assertIsInstance(report[0], MemoryError)
                self.assertIsNot(report[0], later)
                self.assert_failure_report(
                    report,
                    report[0],
                    counts,
                    events,
                    FAILED_EXPLICIT_STATUSES,
                )

    def test_explicit_and_implicit_error_precedence(self):
        for mode, statuses in (
            ("explicit", FAILED_EXPLICIT_STATUSES),
            ("implicit", FAILED_IMPLICIT_STATUSES),
        ):
            for hint in HINTS:
                with self.subTest(mode=mode, hint=hint, precedence="entry"):
                    entry = RuntimeError("entry")
                    first = ValueError("first")
                    later = LookupError("later")
                    report = format_engine_resource_failure(
                        (mode, hint, entry, first, later)
                    )
                    self.assert_failure_report(
                        report, entry, ALL_COUNTS, ALL_EVENTS, statuses
                    )

                with self.subTest(mode=mode, hint=hint, precedence="first"):
                    first = ValueError("first")
                    later = LookupError("later")
                    report = format_engine_resource_failure(
                        (mode, hint, None, first, later)
                    )
                    self.assert_failure_report(
                        report, first, ALL_COUNTS, ALL_EVENTS, statuses
                    )

                with self.subTest(mode=mode, hint=hint, precedence="later"):
                    later = LookupError("later")
                    report = format_engine_resource_failure(
                        (mode, hint, None, None, later)
                    )
                    self.assert_failure_report(
                        report, later, ALL_COUNTS, ALL_EVENTS, statuses
                    )

    def test_negative_status_and_later_error(self):
        later = LookupError("later")
        report = format_engine_resource_failure(
            ("negative", 8, None, None, later)
        )
        error = report[0]
        self.assertIs(type(error), SystemError)
        self.assertEqual(
            str(error), "argument cleanup failed without an exception"
        )
        self.assertIsNot(error, later)
        self.assert_failure_report(
            report,
            error,
            ALL_COUNTS,
            ALL_EVENTS,
            FAILED_EXPLICIT_STATUSES,
        )

    def test_clean_and_error_end_reentry(self):
        clean = format_engine_resource_failure(
            ("end_reentry", 8, None, None, None)
        )
        self.assert_failure_report(
            clean,
            None,
            ALL_COUNTS,
            ALL_EVENTS,
            (None, None, 0, 0, 0),
        )

        first = ValueError("first")
        later = LookupError("later")
        failed = format_engine_resource_failure(
            ("end_reentry", 8, None, first, later)
        )
        self.assert_failure_report(
            failed,
            first,
            ALL_COUNTS,
            ALL_EVENTS,
            (None, None, -1, -1, 0),
        )

    def test_unused_scope_is_clean(self):
        report = format_engine_resource_failure(
            ("unused", 0, None, None, None)
        )
        self.assert_failure_report(
            report,
            None,
            (0,) * 18,
            (),
            (0, 0, 0, 0, None),
        )

    def test_unused_mode_rejects_resource_protocols(self):
        invalid = (
            ("unused", 8, None, None, None),
            ("unused", 0, RuntimeError("entry"), None, None),
            ("unused", 0, None, ValueError("first"), None),
            ("unused", 0, None, None, LookupError("later")),
        )
        for arguments in invalid:
            with self.subTest(arguments=arguments[:2]):
                with self.assertRaises(Exception):
                    format_engine_resource_failure(arguments)


if __name__ == "__main__":
    unittest.main()
