"""Host-only stats-record tests; no target interpreter or counter probe runs."""

import base64
import hashlib
import importlib.util
import io
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

SOURCE = Path(__file__).resolve().parents[1] / "run.py"
SPEC = importlib.util.spec_from_file_location("boil_pystats_records_tests", SOURCE)
runner = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = runner
SPEC.loader.exec_module(runner)


def sample_counts():
    metadata = {
        "Handler entry coverage version": 2,
        "Handler entry dispatch mode": 0,
        "Handler domain count": 302,
        "Handler metadata entries": 300,
    }
    low = metadata | {
        "Object inline int boxes": 2,
        "Object inline float boxes": 0,
        "Object inline array boxes": 0,
        "opcode[COMPARE_OP_INT].execution_count": 10,
        "opcode[LOAD_FAST_BORROW].execution_count": 6,
        "opcode[COMPARE_OP_INT].pair_count[POP_JUMP_IF_FALSE_BOOL]": 6,
        "opcode[BINARY_OP].pair_count[BINARY_OP_ADD_INT]": 2,
        "Object box site[/test/python+0x100]": 2,
        "Object box site[/test/python+0x200]": 4,
        "Object box site[0x0]": 1,
    }
    high = metadata | {
        "Object inline int boxes": 10,
        "Object inline float boxes": 0,
        "Object inline array boxes": 0,
        "opcode[COMPARE_OP_INT].execution_count": 18,
        "opcode[COMPARE_OP_INT].pair_count[POP_JUMP_IF_FALSE_BOOL]": 14,
        "opcode[BINARY_OP].pair_count[BINARY_OP_ADD_INT]": 4,
        "Object box site[/test/python+0x100]": 10,
        "Object box site[0x0]": 3,
    }
    return [metadata | {"warmup-only counter": 7}, low, high]


STEM = "a" * 40


def write_pair(command, raw, *, stem=STEM, pid=12345, ppid=None, tag="", argv=None,
               marker=True):
    directory = Path(next(arg.removeprefix("pystats_dir=") for arg in command
                          if arg.startswith("pystats_dir=")))
    text = f"Stats process: pid={pid} tag={tag}\n".encode() + raw
    dump = directory / f"{stem}.txt"
    dump.write_bytes(text)
    done = directory / f"{stem}.done"
    if marker:
        byte_argv = [os.fsencode(arg).decode("latin-1") for arg in (command if argv is None else argv)]
        done.write_text(f"pid: {pid}\nppid: {os.getpid() if ppid is None else ppid}\n"
                        f"tag: {tag}\nargv: {json.dumps(byte_argv)}\n")
    return dump, done


class PystatsRecordsTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="boil-pystats-record-test-")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.ctx = runner.Context(Path("/test/python"), self.root / "cache", self.root)

    def test_preserves_complete_samples_and_normalizes_union(self):
        for inner in (1, 1000):
            with self.subTest(inner=inner):
                samples = sample_counts()
                bench = runner.Bench("sample", "micro" if inner > 1 else "kernels", 2,
                                     inner=inner)
                with mock.patch.object(runner, "pystats_counts", side_effect=samples) as collect:
                    result = runner.measure_boxes(self.ctx, bench, runs=1, loops=2)
                self.assertEqual(collect.call_args_list, [mock.call(self.ctx, bench, 2),
                    mock.call(self.ctx, bench, 2), mock.call(self.ctx, bench, 6)])
                self.assertEqual(result["raw_counters"],
                                 dict(zip(("warmup", "low", "high"), samples)))
                self.assertEqual(result["handler_entries"], {
                    "opcode[COMPARE_OP_INT].execution_count": 2 / inner,
                    "opcode[LOAD_FAST_BORROW].execution_count": -1.5 / inner,
                })
                self.assertEqual(result["handler_pairs"], {
                    "opcode[BINARY_OP].pair_count[BINARY_OP_ADD_INT]": 0.5 / inner,
                    "opcode[COMPARE_OP_INT].pair_count[POP_JUMP_IF_FALSE_BOOL]": 2 / inner,
                })
                self.assertEqual(result["box_sites"], {
                    "Object box site[/test/python+0x100]": 2 / inner,
                    "Object box site[/test/python+0x200]": -1 / inner,
                    "Object box site[0x0]": 0.5 / inner,
                })
                self.assertEqual(sum(result["handler_entries"].values()),
                                 result["counted_handler_entries"])
                self.assertEqual(result["value"], 2 / inner)
                self.assertEqual(result["by_instruction"], {})
                self.assertEqual(json.loads(json.dumps(result))["raw_counters"],
                                 result["raw_counters"])

    def test_retention_does_not_accept_mismatched_coverage(self):
        cases = [
            ("Handler entry coverage version", 1, "unsupported pystats handler-entry metadata"),
            ("Handler entry dispatch mode", 1, "coverage/mode differs between samples"),
            ("Handler domain count", 303, "coverage/mode differs between samples"),
        ]
        for key, value, message in cases:
            with self.subTest(key=key):
                samples = sample_counts()
                samples[2][key] = value
                with mock.patch.object(runner, "pystats_counts", side_effect=samples):
                    with self.assertRaisesRegex(runner.MeasurementError, message):
                        runner.measure_boxes(self.ctx, runner.Bench("sample", "kernels", 2), 1, 2)

    def test_original_dump_is_journaled_before_duplicate_key_failure(self):
        unrelated = self.root / "unrelated.txt"
        unrelated.write_text("leave this file alone\n")
        journal = self.root / "stats.jsonl"
        journal.touch()
        self.ctx.pystats_records = journal
        raw = b"Object inline int boxes: 1\nObject inline int boxes: 2\n"
        created = []

        def run(command, what):
            created.extend(write_pair(command, raw))

        with mock.patch.object(runner, "run_checked", side_effect=run):
            with self.assertRaisesRegex(runner.MeasurementError, "duplicate pystats key"):
                runner.pystats_counts(self.ctx, runner.Bench("sample", "kernels", 2), 2)
        record = json.loads(journal.read_text())
        self.assertEqual(len(record["artifacts"]), 2)
        for artifact in record["artifacts"]:
            path = Path(artifact["path"])
            self.assertEqual(base64.b64decode(artifact["raw_base64"]), path.read_bytes())
            self.assertEqual(artifact["sha256"], hashlib.sha256(path.read_bytes()).hexdigest())
        self.assertEqual(record["loops"], 2)
        self.assertEqual(self.ctx.pystats_dumps, 1)
        self.assertIn(raw, created[0].read_bytes())
        self.assertTrue(all(path.exists() for path in created))
        self.assertEqual(unrelated.read_text(), "leave this file alone\n")

    def test_only_identical_specializable_metadata_may_repeat(self):
        cases = (
            ("accepted", b"opcode[GET_ITER].specializable: 1\n"
             b"Object inline int boxes: 7\n"
             b"opcode[GET_ITER].specializable: 1\n", None),
            ("conflicting", b"opcode[GET_ITER].specializable: 1\n"
             b"opcode[GET_ITER].specializable: 0\n", "duplicate pystats key"),
            ("nonflag", b"opcode[GET_ITER].specializable: 2\n"
             b"opcode[GET_ITER].specializable: 2\n", "duplicate pystats key"),
            ("opcode_boxes", b"opcode[GET_ITER].boxes: 1\n"
             b"opcode[GET_ITER].boxes: 1\n", "duplicate pystats key"),
        )
        for name, raw, error in cases:
            with self.subTest(name=name):
                def run(command, what):
                    write_pair(command, raw)

                with mock.patch.object(runner, "run_checked", side_effect=run):
                    if error is None:
                        counts = runner.pystats_counts(
                            self.ctx, runner.Bench("sample", "kernels", 2), 2)
                        self.assertEqual(counts, {
                            "opcode[GET_ITER].specializable": 1,
                            "Object inline int boxes": 7,
                        })
                    else:
                        with self.assertRaisesRegex(runner.MeasurementError, error):
                            runner.pystats_counts(
                                self.ctx, runner.Bench("sample", "kernels", 2), 2)

    def test_completed_pair_rejects_wrong_identity_and_extra_files(self):
        cases = ("pid", "ppid", "tag", "argv", "stem", "run", "zero",
                 "orphan", "extra", "empty", "bad_argv")
        for case in cases:
            with self.subTest(case=case):
                def run(command, what):
                    directory = Path(next(arg.removeprefix("pystats_dir=") for arg in command
                                          if arg.startswith("pystats_dir=")))
                    dump, done = write_pair(command, b"Object inline int boxes: 0\n")
                    if case == "pid":
                        done.write_bytes(done.read_bytes().replace(b"pid: 12345", b"pid: 12346", 1))
                    elif case == "ppid":
                        done.write_bytes(done.read_bytes().replace(
                            f"ppid: {os.getpid()}".encode(), b"ppid: 1", 1))
                    elif case == "tag":
                        done.write_bytes(done.read_bytes().replace(b"tag: \n", b"tag: other\n", 1))
                    elif case == "argv":
                        done.write_bytes(done.read_bytes().replace(b"argv: [", b'argv: ["extra",', 1))
                    elif case == "stem":
                        done.rename(directory / ("b" * 40 + ".done"))
                    elif case == "run":
                        done.rename(directory / (STEM + ".run"))
                    elif case == "zero":
                        done.rename(directory / (STEM + ".zero"))
                    elif case == "orphan":
                        done.unlink()
                    elif case == "extra":
                        (directory / ("b" * 40 + ".txt")).write_bytes(dump.read_bytes())
                    elif case == "empty":
                        done.write_bytes(b"")
                    elif case == "bad_argv":
                        done.write_bytes(done.read_bytes().replace(b"argv: [", b'argv: ["\\u0100",', 1))

                with mock.patch.object(runner, "run_checked", side_effect=run):
                    with self.assertRaises(runner.MeasurementError):
                        runner.pystats_counts(self.ctx, runner.Bench("sample", "kernels", 2), 2)

    def test_marker_argv_decodes_filesystem_bytes(self):
        encoded = ["caf\u00c3\u00a9", "raw\u00ff"]
        raw = ("pid: 12345\nppid: 1\ntag: \nargv: " + json.dumps(encoded) + "\n").encode()
        pid, ppid, tag, argv = runner._marker_identity(raw)
        self.assertEqual((pid, ppid, tag), (12345, 1, ""))
        self.assertEqual([os.fsencode(arg) for arg in argv],
                         [b"caf\xc3\xa9", b"raw\xff"])

    def test_nonregular_and_unreadable_artifacts_are_journaled_before_failure(self):
        for case in ("symlink", "unreadable"):
            with self.subTest(case=case):
                journal = self.root / f"{case}.jsonl"
                journal.touch()
                self.ctx.pystats_records = journal
                original_read = Path.read_bytes

                def run(command, what):
                    directory = Path(next(arg.removeprefix("pystats_dir=") for arg in command
                                          if arg.startswith("pystats_dir=")))
                    dump, done = write_pair(command, b"Object inline int boxes: 0\n")
                    if case == "symlink":
                        done.unlink()
                        done.symlink_to(dump)

                def read(path):
                    if case == "unreadable" and path.suffix == ".done":
                        raise PermissionError("test read failure")
                    return original_read(path)

                with mock.patch.object(runner, "run_checked", side_effect=run), \
                     mock.patch.object(Path, "read_bytes", read):
                    with self.assertRaises(runner.MeasurementError):
                        runner.pystats_counts(self.ctx, runner.Bench("sample", "kernels", 2), 2)
                record = json.loads(journal.read_text())
                self.assertEqual(len(record["artifacts"]), 2)
                problem = next(artifact for artifact in record["artifacts"]
                               if artifact["path"].endswith(".done"))
                if case == "symlink":
                    self.assertEqual(problem["kind"], "symlink")
                else:
                    self.assertIn("test read failure", problem["read_error"])

    def test_cli_rejects_conflicts_and_existing_journal_before_target(self):
        for case in ("output", "perf", "metric", "existing"):
            with self.subTest(case=case):
                journal = self.root / (case + ".jsonl")
                arguments = ["--build", "/test/python", "--metric", "boxes",
                             "--pystats-records", str(journal)]
                if case == "output":
                    arguments += ["--output", str(journal)]
                elif case == "perf":
                    arguments[3] = "instr,boxes"
                    arguments += ["--perf-records", str(journal)]
                elif case == "metric":
                    arguments[3] = "time"
                else:
                    journal.write_text("preserve existing evidence\n")
                expected = FileExistsError if case == "existing" else SystemExit
                with mock.patch.object(runner, "resolve_build", return_value=Path("/test/python")), \
                     mock.patch.object(runner, "build_info") as build, \
                     mock.patch.object(runner, "run_checked") as child, \
                     mock.patch.object(sys, "stderr", new_callable=io.StringIO):
                    with self.assertRaises(expected) as error:
                        runner.main(arguments)
                build.assert_not_called()
                child.assert_not_called()
                if case == "existing":
                    self.assertEqual(journal.read_text(), "preserve existing evidence\n")
                else:
                    self.assertEqual(error.exception.code, 2)
                    if journal.exists():
                        # The perf journal is reserved before cross-journal validation.
                        self.assertEqual(journal.read_bytes(), b"")

    def test_unattributable_or_undecodable_dumps_preserve_originals(self):
        for case in ("failed", "ambiguous", "missing", "invalid_utf8"):
            with self.subTest(case=case):
                unrelated = self.root / f"{case}-unrelated.txt"
                unrelated.write_bytes(b"original unrelated evidence\n")
                journal = self.root / (case + ".jsonl")
                journal.touch()
                self.ctx.pystats_records = journal
                created = {}

                def run(command, what):
                    if case != "missing":
                        raw = b"\xff" if case == "invalid_utf8" else b"counter: 1\n"
                        for path in write_pair(command, raw):
                            created[path] = path.read_bytes()
                    if case == "ambiguous":
                        directory = next(iter(created)).parent
                        extra = directory / ("b" * 40 + ".txt")
                        extra.write_bytes(b"counter: 2\n")
                        created[extra] = extra.read_bytes()
                    if case == "failed":
                        raise runner.MeasurementError("actual child failed")

                with mock.patch.object(runner, "run_checked", side_effect=run):
                    with self.assertRaises(runner.MeasurementError):
                        runner.pystats_counts(self.ctx, runner.Bench("sample", "kernels", 2), 2)
                record = json.loads(journal.read_text())
                self.assertEqual(len(record["artifacts"]), len(created))
                self.assertEqual(unrelated.read_bytes(), b"original unrelated evidence\n")
                for path, raw in created.items():
                    self.assertEqual(path.read_bytes(), raw)

    def test_missing_coverage_is_rejected(self):
        for sample_index in (1, 2):
            with self.subTest(sample_index=sample_index):
                samples = sample_counts()
                del samples[sample_index]["Handler entry coverage version"]
                with mock.patch.object(runner, "pystats_counts", side_effect=samples):
                    with self.assertRaisesRegex(runner.MeasurementError, "lacks full handler-entry"):
                        runner.measure_boxes(self.ctx, runner.Bench("sample", "kernels", 2), 1, 2)

    def test_successful_run_binds_exact_journal_and_preserves_unrelated_dump(self):
        unrelated = self.root / "unrelated.txt"
        unrelated.write_bytes(b"leave this preexisting dump alone\n")
        journal = self.root / "retained.jsonl"
        output = self.root / "report.json"
        samples = sample_counts()
        originals = []
        commands = []

        def run(command, what):
            index = len(commands)
            raw = "".join(f"{key}: {value}\n" for key, value in samples[index].items()).encode()
            paths = write_pair(command, raw)
            originals.append([(path, path.read_bytes()) for path in paths])
            commands.append(command)

        with mock.patch.object(runner, "resolve_build", return_value=Path("/test/python")), \
             mock.patch.object(runner, "select", return_value=[runner.Bench("sample", "kernels", 2)]), \
             mock.patch.object(runner, "build_info", return_value={"path": "/test/python", "label": "test"}), \
             mock.patch.object(runner, "check_pystats"), \
             mock.patch.object(runner, "run_checked", side_effect=run), \
             mock.patch.object(runner.os, "getloadavg", return_value=(0, 0, 0)), \
             mock.patch.object(runner, "git", return_value=""), \
             mock.patch.object(runner, "machine", return_value={"hostname": "host"}), \
             mock.patch.object(sys, "stderr", new_callable=io.StringIO):
            result = runner.main(["--build", "/test/python", "--label", "test",
                                  "--metric", "boxes", "--pystats-records", str(journal),
                                  "--output", str(output)])
        self.assertEqual(result, 0)
        records = [json.loads(line) for line in journal.read_text().splitlines()]
        self.assertEqual(len(records), 3)
        for index, (record, originals_for_run) in enumerate(zip(records, originals)):
            self.assertEqual(record["dump"], index)
            self.assertEqual(record["command"], commands[index])
            self.assertEqual(len(record["artifacts"]), 2)
            for artifact, (path, raw) in zip(record["artifacts"], sorted(originals_for_run)):
                self.assertEqual(artifact["path"], str(path))
                self.assertEqual(base64.b64decode(artifact["raw_base64"]), raw)
                self.assertEqual(artifact["sha256"], hashlib.sha256(raw).hexdigest())
                self.assertFalse(path.exists())
        report = json.loads(output.read_text())
        self.assertEqual(report["runs"]["boxes"]["pystats_records"], {
            "path": str(journal.resolve()),
            "sha256": hashlib.sha256(journal.read_bytes()).hexdigest(),
            "dumps": 3,
        })
        self.assertEqual(report["results"]["boxes"]["sample"]["raw_counters"],
                         dict(zip(("warmup", "low", "high"), samples)))
        self.assertEqual(unrelated.read_bytes(), b"leave this preexisting dump alone\n")
