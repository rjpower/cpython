"""Host-only numeric controls for the handler-entry report contract."""

import collections
import importlib
import io
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

# Use the host support package without importing this fork's newer stdlib.
import test
import test.support
import test.support.import_helper


ROOT = Path(__file__).resolve().parents[5]
test.__path__.insert(0, str(ROOT / "Lib/test"))
tests = importlib.import_module("test.test_tools.test_summarize_stats")
stats = tests.stats
assert Path(stats.__file__).resolve() == ROOT / "Tools/scripts/summarize_stats.py"


def old_overlapping_family_total(self):
    names = {entry["handler_id"]: name for name, entry in self._metadata.items()}
    result = collections.Counter()
    for name, entry in self._data.items():
        base = names[self._metadata[name]["base_id"]]
        result[base] += sum(entry.get(key, 0) for key in
                            ("execution_count", "specialization.hit", "specialization.miss"))
    return dict(result)


original_kinds = stats.OpcodeStats.get_entry_kind_counts
original_events = stats.OpcodeStats.get_specialization_counts


def subtract_specialized_misses(self):
    adaptive, specialized, other = original_kinds(self)
    misses = sum(entry.get("specialization.miss", 0)
                 for name, entry in self._data.items()
                 if self._metadata[name]["handler_flags"] & stats.HANDLER_SPECIALIZED)
    return adaptive, specialized - misses, other


def infer_truth_attribution(self, opcode):
    result = original_events(self, opcode)
    if opcode == "POP_JUMP_IF_FALSE":
        result["hit"] += original_events(self, "TO_BOOL")["hit"]
    elif opcode == "TO_BOOL":
        result["hit"] = 0
    return result


def unfiltered_events(self, opcode):
    return {key.removeprefix("specialization."): value
            for key, value in self._get_stats_for_opcode(opcode).items()}


def main():
    positive = unittest.TextTestRunner(stream=io.StringIO()).run(
        unittest.defaultTestLoader.loadTestsFromModule(tests))
    assert positive.wasSuccessful(), (positive.failures, positive.errors)
    controls = (
        ("family_entries_100_not_190", "get_family_execution_counts", old_overlapping_family_total,
         "test_family_entries_100_not_190"),
        ("guard_entry_remains_specialized", "get_entry_kind_counts", subtract_specialized_misses,
         "test_guard_entry_remains_specialized"),
        ("truth_hits_stay_at_recorded_key", "get_specialization_counts", infer_truth_attribution,
         "test_truth_hits_stay_at_recorded_key"),
        ("event_fields_exclude_boxes", "get_specialization_counts", unfiltered_events,
         "test_events_whitelist_and_zero_entry_event_rendering"),
    )
    rejected = []
    for name, method, replacement, test_name in controls:
        with patch.object(stats.OpcodeStats, method, replacement):
            result = unittest.TestResult()
            tests.HandlerEntryReportTests(test_name).run(result)
        assert result.testsRun == 1 and len(result.failures) == 1 and not result.errors, name
        rejected.append(name)
    with patch.object(stats, "_unique_dict", dict):
        result = unittest.TestResult()
        tests.HandlerEntryReportTests("test_json_roundtrip_and_duplicate_rejection").run(result)
    assert result.testsRun == 1 and len(result.failures) == 1 and not result.errors
    rejected.append("duplicate_json_rejected_before_dict")
    print(json.dumps({"host_positive_tests": positive.testsRun,
                      "copied_defects_rejected_by_assertion": rejected,
                      "native_executed": False}, indent=2))


if __name__ == "__main__":
    main()
