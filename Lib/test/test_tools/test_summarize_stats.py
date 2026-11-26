"""Handler-entry reports use sampled metadata, never the host opcode table."""

import copy
import io
import json
from pathlib import Path
import tempfile
import unittest

from test.test_tools import import_tool

stats = import_tool("summarize_stats")


def sample(counters=None):
    metadata = {}
    # Deliberately different from the reporting interpreter's opcode IDs.
    entries = (
        ("IR_OPERANDS", 0, 64),
        ("LoadAttr", 267, 64 | 128 | 512),
        ("LoadAttr.Int", 268, 64 | 128 | 1),
        ("LoadAttr.NoFb", 269, 64 | 128 | 16),
        ("ToBool", 37, 64 | 128 | 512),
        ("PopJumpIfFalse", 98, 64 | 128 | 512),
        ("PopJumpIfFalse.Int", 147, 64 | 128 | 1),
        ("BinaryOp", 10, 64),
        ("BinaryOp.Add", 300, 64 | 128 | 512),
        ("Jump", 110, 64 | 128),
        ("Jump.Trailing", 301, 64 | 128 | 8),
    )
    for name, identity, flags in entries:
        metadata[name] = dict(handler_id=identity, handler_flags=flags)
    return {
        "_handler_entry_coverage_version": 2,
        "_handler_entry_dispatch_mode": 0,
        "_handler_count": 302,
        "_handler_metadata_entries": len(metadata),
        "_handler_metadata": metadata,
        "_defines": {}, "_stats_defines": {},
        **(counters or {}),
    }


def raw_text(data):
    lines = [f"{source} : {data[target]}" for source, target in stats.HANDLER_FACTS.items()]
    for name, metadata in data["_handler_metadata"].items():
        lines.extend(f"opcode[{name}].{key} : {value}" for key, value in metadata.items())
    lines.extend(f"{key} : {value}" for key, value in data.items() if not key.startswith("_"))
    return "\n".join(lines) + "\n"


class HandlerEntryReportTests(unittest.TestCase):
    def opcode_stats(self, counters):
        return stats.Stats(sample(counters)).get_opcode_stats("opcode")

    def test_family_entries_100_not_190(self):
        opcodes = self.opcode_stats({
            "opcode[LoadAttr].execution_count": 10,
            "opcode[LoadAttr.Int].execution_count": 90,
            "opcode[LoadAttr].specialization.hit": 90,
        })
        self.assertEqual(opcodes.get_total_execution_count(), 100)
        self.assertEqual(opcodes.get_family_execution_counts(), {"LoadAttr": 100},
                         "family_entries_100_not_190")
        self.assertEqual(opcodes.get_entry_kind_counts(), (10, 90, 0))

    def test_family_statistics_are_read_under_the_family_prefix(self):
        report = stats.Stats(sample({
            "family[GetAttr].specialization.success": 7,
            "family[GetAttr].specialization.failure": 3,
            "opcode[LoadAttr].specialization.success": 100,
        })).get_opcode_stats("family")
        self.assertEqual(list(report.get_opcode_names()), ["GetAttr"])
        self.assertEqual(report.get_specialization_success_failure("GetAttr"),
                         {"success": 7, "failure": 3})

    def test_truth_hits_stay_at_recorded_key(self):
        opcodes = self.opcode_stats({
            "opcode[PopJumpIfFalse].execution_count": 10,
            "opcode[PopJumpIfFalse.Int].execution_count": 90,
            "opcode[ToBool].specialization.hit": 90,
        })
        self.assertEqual(opcodes.get_family_execution_counts(), {"PopJumpIfFalse": 100})
        self.assertEqual(opcodes.get_specialization_counts("ToBool")["hit"], 90)
        self.assertEqual(opcodes.get_specialization_counts("PopJumpIfFalse")["hit"], 0,
                         "truth_hits_stay_at_recorded_key")

    def test_guard_entry_remains_specialized(self):
        opcodes = self.opcode_stats({
            "opcode[LoadAttr.Int].execution_count": 1,
            "opcode[LoadAttr.Int].specialization.miss": 1,
            "opcode[LoadAttr].specialization.miss": 1,
        })
        self.assertEqual(opcodes.get_entry_kind_counts(), (0, 1, 0),
                         "guard_entry_remains_specialized")
        self.assertEqual(opcodes.get_total_execution_count(), 1)
        self.assertEqual(opcodes.get_misses_counts()["LoadAttr"], 1)
        self.assertEqual(opcodes.get_misses_counts()["LoadAttr.Int"], 1)

    def test_nofb_and_ir_entries(self):
        opcodes = self.opcode_stats({
            "opcode[LoadAttr].execution_count": 6,
            "opcode[LoadAttr.Int].execution_count": 3,
            "opcode[LoadAttr.NoFb].execution_count": 1,
            "opcode[Jump.Trailing].execution_count": 2,
            "opcode[IR_OPERANDS].pair_count[Jump.Trailing]": 1,
        })
        self.assertEqual(opcodes.get_entry_kind_counts(), (6, 3, 3))
        self.assertEqual(opcodes.get_family_execution_counts(), {"LoadAttr": 10, "Jump": 2})
        self.assertEqual(opcodes.get_pair_counts(), {("IR_OPERANDS", "Jump.Trailing"): 1})
        self.assertTrue(opcodes.is_specializable("BinaryOp"))
        self.assertFalse(opcodes.is_specializable("LoadAttr.NoFb"))

    def test_events_whitelist_and_zero_entry_event_rendering(self):
        data = sample({"family[BranchTruth].specialization.hit": 90,
                       "opcode[ToBool].boxes": 7})
        families = stats.Stats(data).get_opcode_stats("family")
        self.assertEqual(families.get_specialization_counts("BranchTruth"),
                         dict(hit=90, miss=0, deferred=0, deopt=0))
        out = io.StringIO()
        stats.output_markdown(out, stats.specialization_section(), stats.Stats(data))
        self.assertIn("BranchTruth", out.getvalue())
        self.assertIn(">90<", out.getvalue())
        self.assertNotIn("boxes", out.getvalue())
        self.assertNotIn("handler_flags", out.getvalue())
        self.assertEqual(data["opcode[ToBool].boxes"], 7)

    def test_two_file_metadata_is_not_summed(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "a.txt").write_text(
                "Stats process: pid=1 tag=\n"
                + raw_text(sample({"opcode[LoadAttr].execution_count": 10}))
            )
            (root / "b.txt").write_text(raw_text(sample({"opcode[LoadAttr.Int].execution_count": 90})))
            data = stats.load_raw_data(root)
        self.assertEqual(data["__nfiles__"], 2)
        self.assertEqual(stats.handler_contract(data), stats.handler_contract(sample()))
        self.assertEqual(stats.Stats(data).get_opcode_stats("opcode").get_total_execution_count(), 100)

    def test_cohort_mode_conflict(self):
        other = sample()
        other["_handler_entry_dispatch_mode"] = 1
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "a.txt").write_text(raw_text(sample()))
            (root / "b.txt").write_text(raw_text(other))
            with self.assertRaisesRegex(ValueError, "differs within cohort"):
                stats.load_raw_data(root)

    def test_raw_missing_duplicate_and_incomplete_facts(self):
        text = raw_text(sample())
        variants = (text + "Handler entry coverage version : 2\n",
                    text.replace("Handler entry dispatch mode : 0\n", ""),
                    text.replace("opcode[LoadAttr].handler_flags : 704\n", ""))
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "stats"
            for variant in variants:
                with self.subTest(variant=variant):
                    path.write_text(variant)
                    with self.assertRaises(ValueError):
                        stats._load_dump(path, stats.collections.Counter())

    def test_json_roundtrip_and_duplicate_rejection(self):
        data = sample({"opcode[BinaryOp.Add].execution_count": 11})
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "stats.json"
            with path.open("w") as output:
                stats.save_raw_data(data, output)
            self.assertEqual(stats.load_raw_data(path), data)
            text = path.read_text().replace('"handler_id": 300',
                                             '"handler_id": 300, "handler_id": 300')
            path.write_text(text)
            with self.assertRaisesRegex(ValueError, "duplicate"):
                stats.load_raw_data(path)

    def test_json_missing_or_corrupt_metadata(self):
        missing_mode = sample()
        del missing_mode["_handler_entry_dispatch_mode"]
        corrupt = sample()
        corrupt["_handler_metadata"]["BinaryOp.Add"]["handler_id"] = 500
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "stats.json"
            for data in (missing_mode, corrupt):
                with self.subTest(data=data):
                    path.write_text(json.dumps(data))
                    with self.assertRaises(ValueError):
                        stats.load_raw_data(path)

    def test_complete_report_keeps_separate_domains(self):
        data = sample({"opcode[LoadAttr].execution_count": 10,
                       "opcode[LoadAttr.Int].execution_count": 90,
                       "opcode[ToBool].specialization.hit": 90,
                       "opcode[LoadAttr].specialization.success": 2,
                       "opcode[LoadAttr].specialization.failure": 1})
        out = io.StringIO()
        stats.output_markdown(out, stats.LAYOUT, stats.Stats(data))
        report = out.getvalue()
        self.assertIn("Counted handler entries", report)
        self.assertIn("Recorded specialization events", report)
        self.assertIn("Counted entries by target kind", report)
        self.assertNotIn("Miss ratio", report)
        self.assertNotIn("Specialized misses", report)

    def test_invalid_metadata_and_execution_domains(self):
        mutations = (
            lambda data: data.update(_handler_entry_coverage_version=1),
            lambda data: data["_handler_metadata"]["LoadAttr"].update(handler_id=302),
            lambda data: data["_handler_metadata"]["LoadAttr.Int"].update(handler_id=267),
            lambda data: data["_handler_metadata"]["LoadAttr.Int"].update(handler_flags=64 | 128 | 1 | 512),
            lambda data: data.update({"opcode[BinaryOp].execution_count": 1}),
            lambda data: data.update({"opcode[IR_OPERANDS].pair_count[BinaryOp]": 1}),
        )
        for mutate in mutations:
            data = copy.deepcopy(sample())
            mutate(data)
            with self.subTest(data=data), self.assertRaises(ValueError):
                stats.validate_handler_data(data)

    def test_comparison_rejects_mode_difference(self):
        with tempfile.TemporaryDirectory() as directory:
            paths = [Path(directory) / name for name in ("base.json", "head.json")]
            for mode, path in enumerate(paths):
                data = sample()
                data["_handler_entry_dispatch_mode"] = mode
                path.write_text(json.dumps(data))
            with self.assertRaisesRegex(ValueError, "differs across cohorts"):
                stats.output_stats(paths, None)


if __name__ == "__main__":
    unittest.main()
