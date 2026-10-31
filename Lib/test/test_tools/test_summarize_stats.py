"""Handler-entry reports use sampled metadata, never the host opcode table."""

import copy
import importlib.util
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

from test.test_tools import import_tool

stats = import_tool("summarize_stats")


def sample(counters=None):
    metadata = {}
    # Deliberately different from the reporting interpreter's opcode IDs.
    entries = (
        ("CACHE", 0, 0, 64),
        ("LOAD_ATTR", 267, 267, 64 | 128 | 512),
        ("LOAD_ATTR_INT", 268, 267, 64 | 128 | 1),
        ("LOAD_ATTR_NOFB", 269, 267, 64 | 128 | 16),
        ("TO_BOOL", 37, 37, 64 | 128 | 512),
        ("POP_JUMP_IF_FALSE", 98, 98, 64 | 128 | 512),
        ("POP_JUMP_IF_FALSE_INT", 147, 98, 64 | 128 | 1),
        ("BINARY_OP", 10, 10, 64),
        ("BINARY_OP_ADD", 300, 10, 64 | 128 | 512),
        ("JUMP_FORWARD", 110, 110, 64 | 128),
        ("IR_JUMP", 301, 110, 64 | 128 | 8),
    )
    for name, identity, base, flags in entries:
        metadata[name] = dict(handler_id=identity, base_id=base, handler_flags=flags)
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
            "opcode[LOAD_ATTR].execution_count": 10,
            "opcode[LOAD_ATTR_INT].execution_count": 90,
            "opcode[LOAD_ATTR].specialization.hit": 90,
        })
        self.assertEqual(opcodes.get_total_execution_count(), 100)
        self.assertEqual(opcodes.get_family_execution_counts(), {"LOAD_ATTR": 100},
                         "family_entries_100_not_190")
        self.assertEqual(opcodes.get_entry_kind_counts(), (10, 90, 0))

    def test_truth_hits_stay_at_recorded_key(self):
        opcodes = self.opcode_stats({
            "opcode[POP_JUMP_IF_FALSE].execution_count": 10,
            "opcode[POP_JUMP_IF_FALSE_INT].execution_count": 90,
            "opcode[TO_BOOL].specialization.hit": 90,
        })
        self.assertEqual(opcodes.get_family_execution_counts(), {"POP_JUMP_IF_FALSE": 100})
        self.assertEqual(opcodes.get_specialization_counts("TO_BOOL")["hit"], 90)
        self.assertEqual(opcodes.get_specialization_counts("POP_JUMP_IF_FALSE")["hit"], 0,
                         "truth_hits_stay_at_recorded_key")

    def test_guard_entry_remains_specialized(self):
        opcodes = self.opcode_stats({
            "opcode[LOAD_ATTR_INT].execution_count": 1,
            "opcode[LOAD_ATTR_INT].specialization.miss": 1,
            "opcode[LOAD_ATTR].specialization.miss": 1,
        })
        self.assertEqual(opcodes.get_entry_kind_counts(), (0, 1, 0),
                         "guard_entry_remains_specialized")
        self.assertEqual(opcodes.get_total_execution_count(), 1)
        self.assertEqual(opcodes.get_misses_counts()["LOAD_ATTR"], 1)
        self.assertEqual(opcodes.get_misses_counts()["LOAD_ATTR_INT"], 1)

    def test_nofb_and_ir_entries(self):
        opcodes = self.opcode_stats({
            "opcode[LOAD_ATTR].execution_count": 6,
            "opcode[LOAD_ATTR_INT].execution_count": 3,
            "opcode[LOAD_ATTR_NOFB].execution_count": 1,
            "opcode[IR_JUMP].execution_count": 2,
            "opcode[CACHE].pair_count[IR_JUMP]": 1,
        })
        self.assertEqual(opcodes.get_entry_kind_counts(), (6, 3, 3))
        self.assertEqual(opcodes.get_family_execution_counts(), {"LOAD_ATTR": 10, "JUMP_FORWARD": 2})
        self.assertEqual(opcodes.get_pair_counts(), {("CACHE", "IR_JUMP"): 1})
        self.assertTrue(opcodes.is_specializable("BINARY_OP"))
        self.assertFalse(opcodes.is_specializable("LOAD_ATTR_NOFB"))

    def test_events_whitelist_and_zero_entry_event_rendering(self):
        data = sample({"opcode[TO_BOOL].specialization.hit": 90,
                       "opcode[TO_BOOL].boxes": 7})
        opcodes = stats.Stats(data).get_opcode_stats("opcode")
        self.assertEqual(opcodes.get_specialization_counts("TO_BOOL"),
                         dict(hit=90, miss=0, deferred=0, deopt=0))
        out = io.StringIO()
        stats.output_markdown(out, stats.specialization_section(), stats.Stats(data))
        self.assertIn("TO_BOOL", out.getvalue())
        self.assertIn(">90<", out.getvalue())
        self.assertNotIn("boxes", out.getvalue())
        self.assertNotIn("handler_flags", out.getvalue())
        self.assertEqual(data["opcode[TO_BOOL].boxes"], 7)

    def test_two_file_metadata_is_not_summed(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "a").write_text(raw_text(sample({"opcode[LOAD_ATTR].execution_count": 10})))
            (root / "b").write_text(raw_text(sample({"opcode[LOAD_ATTR_INT].execution_count": 90})))
            data = stats.load_raw_data(root)
        self.assertEqual(data["__nfiles__"], 2)
        self.assertEqual(stats.handler_contract(data), stats.handler_contract(sample()))
        self.assertEqual(stats.Stats(data).get_opcode_stats("opcode").get_total_execution_count(), 100)

    def test_cohort_mode_conflict(self):
        other = sample()
        other["_handler_entry_dispatch_mode"] = 1
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "a").write_text(raw_text(sample()))
            (root / "b").write_text(raw_text(other))
            with self.assertRaisesRegex(ValueError, "differs within cohort"):
                stats.load_raw_data(root)

    def test_raw_missing_duplicate_and_incomplete_facts(self):
        text = raw_text(sample())
        variants = (text + "Handler entry coverage version : 2\n",
                    text.replace("Handler entry dispatch mode : 0\n", ""),
                    text.replace("opcode[LOAD_ATTR].base_id : 267\n", ""))
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "stats"
            for variant in variants:
                with self.subTest(variant=variant):
                    path.write_text(variant)
                    with self.assertRaises(ValueError):
                        stats._load_raw_file(path)

    def test_json_roundtrip_and_duplicate_rejection(self):
        data = sample({"opcode[BINARY_OP_ADD].execution_count": 11})
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
        corrupt["_handler_metadata"]["BINARY_OP_ADD"]["handler_id"] = 500
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "stats.json"
            for data in (missing_mode, corrupt):
                with self.subTest(data=data):
                    path.write_text(json.dumps(data))
                    with self.assertRaises(ValueError):
                        stats.load_raw_data(path)

    def test_complete_report_keeps_separate_domains(self):
        data = sample({"opcode[LOAD_ATTR].execution_count": 10,
                       "opcode[LOAD_ATTR_INT].execution_count": 90,
                       "opcode[TO_BOOL].specialization.hit": 90,
                       "opcode[LOAD_ATTR].specialization.success": 2,
                       "opcode[LOAD_ATTR].specialization.failure": 1})
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
            lambda data: data["_handler_metadata"]["LOAD_ATTR"].update(base_id=299),
            lambda data: data["_handler_metadata"]["LOAD_ATTR_INT"].update(handler_id=267),
            lambda data: data["_handler_metadata"]["LOAD_ATTR_INT"].update(handler_flags=64 | 128 | 1 | 512),
            lambda data: data.update({"opcode[BINARY_OP].execution_count": 1}),
            lambda data: data.update({"opcode[CACHE].pair_count[BINARY_OP]": 1}),
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


class BenchmarkEntryMetadataTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        path = Path(__file__).resolve().parents[3] / "boil/bench/run.py"
        spec = importlib.util.spec_from_file_location("_bench_stats_reader", path)
        cls.benchmarks = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = cls.benchmarks
        cls.addClassCleanup(sys.modules.pop, spec.name)
        spec.loader.exec_module(cls.benchmarks)

    def counters(self):
        return {**{source: sample()[target] for source, target in stats.HANDLER_FACTS.items()},
                **dict.fromkeys(self.benchmarks.BOX_TOTALS, 2)}

    def test_counted_entries_and_metadata_survive_differencing(self):
        reader = self.benchmarks
        low, high = self.counters(), self.counters()
        low["opcode[IR_JUMP].execution_count"] = 4
        high["opcode[BINARY_OP_ADD].execution_count"] = 20
        bench = reader.Bench("unused", "kernels", 1)
        context = reader.Context(Path("unused-python"), Path("unused-cache"), Path("unused"))
        with patch.object(reader, "pystats_counts", side_effect=[low, low, high]):
            result = reader.measure_boxes(context, bench, 1, 1)
        self.assertEqual(result["counted_handler_entries"], 8)
        self.assertEqual(result["handler_entry_coverage_version"], 2)
        self.assertEqual(result["handler_entry_dispatch_mode"], 0)
        self.assertNotIn("dispatches", result)

    def test_benchmark_rejects_missing_or_different_coverage(self):
        reader = self.benchmarks
        missing = self.counters()
        del missing["Handler entry dispatch mode"]
        with self.assertRaises(reader.MeasurementError):
            reader.handler_entry_metadata(missing)
        low, high = self.counters(), self.counters()
        high["Handler entry dispatch mode"] = 1
        bench = reader.Bench("unused", "kernels", 1)
        context = reader.Context(Path("unused-python"), Path("unused-cache"), Path("unused"))
        with patch.object(reader, "pystats_counts", side_effect=[low, low, high]):
            with self.assertRaisesRegex(reader.MeasurementError, "differs between samples"):
                reader.measure_boxes(context, bench, 1, 1)


if __name__ == "__main__":
    unittest.main()
