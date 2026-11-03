"""Host-only counter-record tests; no perf tool or target interpreter runs."""

import contextlib
import hashlib
import importlib.util
import io
import json
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock


SOURCE = Path(__file__).resolve().parents[1] / "run.py"
SPEC = importlib.util.spec_from_file_location("boil_bench_capture_tests", SOURCE)
runner = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = runner
SPEC.loader.exec_module(runner)


class PerfRecordsTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="boil-perf-record-test-")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.scratch = self.root / "scratch"
        self.scratch.mkdir()

    def context(self, name="records"):
        journal = self.root / f"{name}.jsonl"
        journal.touch(exist_ok=False)
        return runner.Context(Path("/test/python"), self.root / "pycache", self.scratch, journal)

    def records(self, ctx):
        return [json.loads(line) for line in ctx.perf_records.read_text().splitlines()]

    def fake_perf(self, text, returncode=0, stdout="", stderr=""):
        def run(command, **kwargs):
            if text is not None:
                Path(command[command.index("-o") + 1]).write_text(text)
            return subprocess.CompletedProcess(command, returncode, stdout, stderr)
        return run

    def test_pairs_map_original_counts_to_journal_and_keep_skeleton_raw(self):
        ctx = self.context()
        counts = iter([50, 100, 300, 125, 365])

        def perf(command, **kwargs):
            count = next(counts)
            return self.fake_perf(f"{count},,instructions:u,1,100.00\n")(command, **kwargs)

        bench = runner.Bench("sample", "micro", 1, skeleton="empty")
        with mock.patch.object(runner.subprocess, "run", side_effect=perf):
            result = runner.measure_instr(ctx, bench, runs=2, loops=1)

        self.assertEqual(result["values"], [100, 120])
        self.assertEqual(result["value"], 110)
        self.assertEqual(result["raw_pairs"], [
            {"low": 100, "high": 300, "low_invocation": 2, "high_invocation": 3},
            {"low": 125, "high": 365, "low_invocation": 4, "high_invocation": 5},
        ])
        finishes = {row["invocation"]: row for row in self.records(ctx) if row["phase"] == "finish"}
        self.assertEqual(len(self.records(ctx)), 10)
        for pair in result["raw_pairs"]:
            for side in ("low", "high"):
                record = finishes[pair[f"{side}_invocation"]]
                self.assertEqual(record["instructions"], pair[side])
                self.assertTrue(record["perf_text"].startswith(str(pair[side]) + ","))
                self.assertEqual(record["returncode"], 0)
        runner.subtract_skeletons({"sample": result, "empty": {"value": 10}}, [bench])
        self.assertEqual(result["raw"], 110)
        self.assertEqual(result["value"], 100)
        self.assertEqual(len(result["raw_pairs"]), 2)

    def test_nonzero_exit_retains_complete_streams_and_perf_text(self):
        ctx = self.context()
        text = "<not counted>,,instructions:u\n"
        stdout, stderr = "a" * 5000, "b" * 6000
        with mock.patch.object(runner.subprocess, "run",
                               side_effect=self.fake_perf(text, 7, stdout, stderr)):
            with self.assertRaisesRegex(runner.MeasurementError, "exit status 7"):
                runner.perf_instructions(ctx, ["/test/python", "work.py"], "failure")
        start, finish = self.records(ctx)
        self.assertEqual(start["phase"], "start")
        self.assertEqual(finish["phase"], "finish")
        self.assertEqual(start["invocation"], finish["invocation"])
        self.assertEqual(finish["returncode"], 7)
        self.assertEqual(finish["stdout"], stdout)
        self.assertEqual(finish["stderr"], stderr)
        self.assertEqual(finish["perf_text"], text)
        self.assertIsNone(finish["instructions"])

    def test_invalid_or_missing_counter_output_is_retained(self):
        for name, text in (("unsupported", "<not supported>,,instructions:u\n"),
                           ("no-counter", "# no event\n"), ("no-file", None)):
            with self.subTest(name=name):
                ctx = self.context(name)
                with mock.patch.object(runner.subprocess, "run", side_effect=self.fake_perf(text)):
                    with self.assertRaises(runner.MeasurementError):
                        runner.perf_instructions(ctx, ["/test/python"], name)
                finish = self.records(ctx)[-1]
                self.assertEqual(finish["perf_text"], text)
                self.assertIsNone(finish["instructions"])
                self.assertIn("error", finish)

    def test_spawn_failure_cannot_inherit_previous_scratch_result(self):
        ctx = self.context()
        (self.scratch / "perf.txt").write_text("999,,instructions:u\n")
        failure = FileNotFoundError("perf disappeared")
        with mock.patch.object(runner.subprocess, "run", side_effect=failure):
            with self.assertRaises(FileNotFoundError) as caught:
                runner.perf_instructions(ctx, ["/test/python"], "spawn")
        self.assertIs(caught.exception, failure)
        finish = self.records(ctx)[-1]
        self.assertIsNone(finish["returncode"])
        self.assertIsNone(finish["instructions"])
        self.assertIsNone(finish["perf_text"])
        self.assertIn("FileNotFoundError", finish["error"])
        self.assertFalse((self.scratch / "perf.txt").exists())

    def test_journal_survives_scratch_cleanup(self):
        ctx = self.context()
        with mock.patch.object(runner.subprocess, "run", side_effect=self.fake_perf("123,,instructions:u\n")):
            self.assertEqual(runner.perf_instructions(ctx, ["/test/python"], "durable"), 123)
        shutil.rmtree(self.scratch)
        self.assertEqual(self.records(ctx)[-1]["instructions"], 123)

    def test_scalar_count_does_not_require_a_journal(self):
        ctx = runner.Context(Path("/test/python"), self.root / "pycache", self.scratch)
        with mock.patch.object(runner.subprocess, "run", side_effect=self.fake_perf("123,,instructions:u\n")):
            self.assertEqual(runner.perf_instructions(ctx, ["/test/python"], "ordinary"), 123)
        self.assertIsNone(ctx.perf_records)

    def test_journal_requires_instruction_metric_before_build_probe(self):
        journal = self.root / "records.jsonl"
        with (mock.patch.object(runner, "build_info") as probe,
              contextlib.redirect_stderr(io.StringIO())):
            with self.assertRaises(SystemExit) as caught:
                runner.main(["--build", sys.executable, "--only", "richards", "--metric", "time",
                             "--perf-records", str(journal)])
        self.assertEqual(caught.exception.code, 2)
        self.assertFalse(journal.exists())
        probe.assert_not_called()

    def test_existing_or_aliased_journal_is_refused_before_build_probe(self):
        journal = self.root / "records.jsonl"
        journal.write_text("retained\n")
        args = ["--build", sys.executable, "--only", "richards", "--metric", "instr",
                "--perf-records", str(journal)]
        with mock.patch.object(runner, "build_info") as probe:
            with self.assertRaises(FileExistsError):
                runner.main(args)
            self.assertEqual(journal.read_text(), "retained\n")
            journal.unlink()
            with contextlib.redirect_stderr(io.StringIO()):
                with self.assertRaises(SystemExit) as caught:
                    runner.main([*args, "-o", str(journal)])
            self.assertEqual(caught.exception.code, 2)
            self.assertFalse(journal.exists())
            probe.assert_not_called()

    def test_successful_report_binds_original_journal(self):
        journal = self.root / "journal.jsonl"
        report = self.root / "report.json"
        counts = iter([1, 2, 100, 300])

        def perf(command, **kwargs):
            return self.fake_perf(f"{next(counts)},,instructions:u\n")(command, **kwargs)

        with (mock.patch.object(runner, "build_info", return_value={"path": sys.executable, "label": "test"}),
              mock.patch.object(runner, "machine", return_value={"hostname": "test"}),
              mock.patch.object(runner, "git", return_value=""),
              mock.patch.object(runner.os, "getloadavg", return_value=(0, 0, 0)),
              mock.patch.object(runner.shutil, "which", return_value="/test/perf"),
              mock.patch.object(runner.subprocess, "run", side_effect=perf),
              mock.patch.object(runner.tempfile, "TemporaryDirectory",
                                return_value=contextlib.nullcontext(str(self.scratch))),
              contextlib.redirect_stderr(io.StringIO())):
            status = runner.main(["--build", sys.executable, "--only", "richards",
                                  "--metric", "instr", "--loops", "1", "--instr-runs", "1",
                                  "--perf-records", str(journal), "-o", str(report)])
        self.assertEqual(status, 0)
        result = json.loads(report.read_text())
        self.assertEqual(result["runs"]["instr"]["perf_records"], {
            "path": str(journal), "sha256": hashlib.sha256(journal.read_bytes()).hexdigest(),
            "invocations": 4,
        })
        self.assertEqual(result["results"]["instr"]["richards"]["value"], 100)
        self.assertEqual(len(journal.read_text().splitlines()), 8)


if __name__ == "__main__":
    unittest.main()
