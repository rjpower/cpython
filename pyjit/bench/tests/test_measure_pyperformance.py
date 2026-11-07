"""Prelaunch load waiting for the pyperformance collector."""

import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest import mock

SCRIPT = Path(__file__).resolve().parents[1] / "measure_pyperformance.py"
spec = importlib.util.spec_from_file_location("measure_pyperformance", SCRIPT)
measure = importlib.util.module_from_spec(spec)
spec.loader.exec_module(measure)


class QuietLaunchTests(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        self.journal = self.root / "commands.jsonl"
        self.now = [0.0]
        clock = mock.patch.object(measure.time, "monotonic", side_effect=lambda: self.now[0])
        clock.start()
        self.addCleanup(clock.stop)

    def run_command(self, timeout=30):
        measure.run_command(["native"], {}, self.root, self.root / "run",
                            self.journal, timeout=timeout)

    def records(self):
        return [json.loads(line) for line in self.journal.read_text().splitlines()]

    def test_busy_then_quiet_launches_once_with_remaining_budget(self):
        loads = [(4.5, 4, 3), (2.5, 3, 3), (2.8, 3, 3)]
        process = mock.Mock(returncode=0)
        process.communicate.return_value = ("out", "err")
        def sleep(seconds):
            self.now[0] += seconds
        with mock.patch.object(measure.os, "getloadavg", side_effect=loads), \
             mock.patch.object(measure.time, "sleep", side_effect=sleep), \
             mock.patch.object(measure.subprocess, "Popen", return_value=process) as launch:
            self.run_command()
        self.assertEqual(launch.call_count, 1)
        process.communicate.assert_called_once_with(timeout=20)
        records = self.records()
        self.assertEqual([r["phase"] for r in records],
                         ["load-wait", "load-wait", "load-guard", "start", "finish"])
        self.assertEqual([r["loadavg"] for r in records[:3]], [list(x) for x in loads])

    def test_expiry_during_wait_never_launches(self):
        def sleep(seconds):
            self.now[0] += seconds
        with mock.patch.object(measure.os, "getloadavg", return_value=(5, 4, 3)), \
             mock.patch.object(measure.time, "sleep", side_effect=sleep), \
             mock.patch.object(measure.subprocess, "Popen") as launch:
            with self.assertRaisesRegex(TimeoutError, "window expired"):
                self.run_command(timeout=5)
        launch.assert_not_called()
        self.assertEqual([r["phase"] for r in self.records()],
                         ["load-wait", "load-wait", "load-wait-timeout"])

    def test_final_guard_spike_reenters_wait_without_launch(self):
        loads = [(2, 3, 3), (4.5, 4, 3), (2, 3, 3), (2, 3, 3)]
        process = mock.Mock(returncode=0)
        process.communicate.return_value = ("", "")
        with mock.patch.object(measure.os, "getloadavg", side_effect=loads), \
             mock.patch.object(measure.subprocess, "Popen", return_value=process) as launch:
            self.run_command()
        self.assertEqual(launch.call_count, 1)
        self.assertEqual([r["phase"] for r in self.records()][:4],
                         ["load-wait", "load-guard", "load-wait", "load-guard"])

    def test_native_failure_does_not_retry(self):
        process = mock.Mock(returncode=2)
        process.communicate.return_value = ("out", "error")
        with mock.patch.object(measure.os, "getloadavg", return_value=(2, 2, 2)), \
             mock.patch.object(measure.subprocess, "Popen", return_value=process) as launch:
            with self.assertRaisesRegex(RuntimeError, "command failed"):
                self.run_command()
        self.assertEqual(launch.call_count, 1)


if __name__ == "__main__":
    unittest.main()
