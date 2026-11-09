"""Tests for the checkpoint timing controller's load-only retry."""

import importlib.util
import json
from pathlib import Path
import signal
import subprocess
import tempfile
import unittest
from unittest import mock


SCRIPT = Path(__file__).resolve().parents[2] / "issues/checkpoint-closure/measure.py"
spec = importlib.util.spec_from_file_location("checkpoint_measure", SCRIPT)
measure = importlib.util.module_from_spec(spec)
spec.loader.exec_module(measure)
GUARD = "run.py: 1-minute load average 4.04 reached --max-load 4.0; no measurement report written\n"


class TimingRetryTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.output = Path(self.tmp.name)
        self.path = self.output / "report.json"
        self.command = ["runner", "-o", str(self.path)]
        self.conditions = []
        patcher = mock.patch.object(measure.os, "getloadavg", return_value=(2.0, 2.1, 2.2))
        patcher.start()
        self.addCleanup(patcher.stop)

    def invoke(self, label="candidate", deadline=None):
        if deadline is None:
            deadline = measure.time.monotonic() + 60
        return measure.run_timing_invocation(
            self.output, self.command, self.path, label, "go", 0, deadline,
            self.conditions, "diagnostic", "diagnostic")

    def process(self, returncode, stdout="", stderr=""):
        process = mock.Mock(pid=12345, returncode=returncode)
        process.communicate.return_value = (stdout, stderr)
        return process

    def success(self):
        self.path.write_text(json.dumps({"results": {"time": {"go": {"value": 12.5}}}}))
        return self.process(0, stderr="success\n")

    def test_guard_then_accept_preserves_attempts_and_prefix(self):
        with mock.patch.object(measure.subprocess, "Popen") as run:
            def responses(*args, **kwargs):
                if run.call_count == 1:
                    return self.process(1, stderr=GUARD)
                return self.success()
            run.side_effect = responses
            self.assertEqual(self.invoke(), 12.5)
        self.assertEqual(run.call_count, 2)
        journal = json.loads((self.output / "invocations.json").read_text())
        self.assertEqual([a["attempt"] for a in journal["invocations"]], [1, 2])
        self.assertEqual(journal["invocations"][0]["stderr"], GUARD)
        self.assertEqual(journal["invocations"][0]["command"], self.command)
        self.path.unlink()
        with mock.patch.object(measure.subprocess, "Popen", side_effect=lambda *a, **k: self.success()) as run:
            self.assertEqual(self.invoke(label="upstream"), 12.5)
        self.assertEqual(run.call_count, 1)
        self.assertEqual([(r["label"], r["attempt"]) for r in self.conditions],
                         [("candidate", 1), ("candidate", 2), ("upstream", 1)])

    def test_non_guard_failure_is_fatal_and_journaled(self):
        proc = self.process(1, "out", "crash\n" + GUARD)
        with mock.patch.object(measure.subprocess, "Popen", return_value=proc) as run:
            with self.assertRaises(subprocess.CalledProcessError):
                self.invoke()
        self.assertEqual(run.call_count, 1)
        row = json.loads((self.output / "invocations.json").read_text())["invocations"][0]
        self.assertEqual((row["stdout"], row["stderr"]), ("out", "crash\n" + GUARD))

    def test_guard_with_report_is_fatal(self):
        self.path.write_text("{}")
        with mock.patch.object(measure.subprocess, "Popen", return_value=self.process(1, stderr=GUARD)) as run:
            with self.assertRaises(subprocess.CalledProcessError):
                self.invoke()
        self.assertEqual(run.call_count, 1)

    def test_guard_progress_line_and_spoofed_output(self):
        progress = "candidate time kernels/go 12.5 ns per loop\n"
        accepted = subprocess.CompletedProcess(self.command, 1, "", progress + GUARD)
        self.assertTrue(measure.load_guard_rejected(accepted, self.path, "candidate", "go"))
        for stdout, stderr in (("unexpected", GUARD), ("", "traceback\n" + GUARD),
                               ("", "other time kernels/go 12.5 ns per loop\n" + GUARD),
                               ("", GUARD + "extra\n")):
            with self.subTest(stdout=stdout, stderr=stderr):
                proc = subprocess.CompletedProcess(self.command, 1, stdout, stderr)
                self.assertFalse(measure.load_guard_rejected(proc, self.path, "candidate", "go"))

    def test_timeout_kills_group_and_journals_output(self):
        process = self.process(None)
        process.communicate.side_effect = [subprocess.TimeoutExpired(self.command, 1), ("out", "err")]
        with mock.patch.object(measure.subprocess, "Popen", return_value=process) as run, \
             mock.patch.object(measure.os, "killpg") as kill:
            with self.assertRaises(subprocess.TimeoutExpired):
                self.invoke()
        self.assertEqual(run.call_count, 1)
        self.assertEqual(kill.call_args_list, [mock.call(12345, signal.SIGTERM),
                                               mock.call(12345, signal.SIGKILL)])
        row = json.loads((self.output / "invocations.json").read_text())["invocations"][0]
        self.assertEqual((row["classification"], row["stdout"], row["stderr"]),
                         ("timeout", "out", "err"))

    def test_interrupt_during_grace_still_kills_group(self):
        process = self.process(None)
        process.communicate.side_effect = [subprocess.TimeoutExpired(self.command, 1),
                                           KeyboardInterrupt(), ("out", "err")]
        with mock.patch.object(measure.subprocess, "Popen", return_value=process), \
             mock.patch.object(measure.os, "killpg") as kill:
            with self.assertRaises(subprocess.TimeoutExpired):
                self.invoke()
        self.assertEqual(kill.call_args_list, [mock.call(12345, signal.SIGTERM),
                                               mock.call(12345, signal.SIGKILL)])
        row = json.loads((self.output / "invocations.json").read_text())["invocations"][0]
        self.assertEqual(row["cleanup_error"], "KeyboardInterrupt()")

    def test_success_after_deadline_is_not_accepted(self):
        now = [0]

        def late_success(*args, **kwargs):
            process = self.success()
            process.communicate.side_effect = lambda **kw: (now.__setitem__(0, 2) or ("", ""))
            return process

        with mock.patch.object(measure.time, "monotonic", side_effect=lambda: now[0]), \
             mock.patch.object(measure.subprocess, "Popen", side_effect=late_success):
            with self.assertRaisesRegex(RuntimeError, "timing window expired"):
                self.invoke(deadline=1)
        row = json.loads((self.output / "invocations.json").read_text())["invocations"][0]
        self.assertEqual(row["classification"], "deadline_expired")

    def test_deadline_after_rejection_does_not_launch_again(self):
        now = [0]

        def reject(*args, **kwargs):
            now[0] = 2
            return self.process(1, stderr=GUARD)

        with mock.patch.object(measure.time, "monotonic", side_effect=lambda: now[0]), \
             mock.patch.object(measure.subprocess, "Popen", side_effect=reject) as run:
            with self.assertRaisesRegex(RuntimeError, "timing window expired"):
                self.invoke(deadline=1)
        self.assertEqual(run.call_count, 1)
        row = json.loads((self.output / "invocations.json").read_text())["invocations"][0]
        self.assertEqual(row["stderr"], GUARD)


if __name__ == "__main__":
    unittest.main()
