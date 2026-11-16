"""Round selection and request records for the call-model timing controller."""

import importlib.util
import io
import json
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock


SCRIPT = Path(__file__).resolve().parents[2] / "issues/deltablue-call-model-20261110/measure.py"
spec = importlib.util.spec_from_file_location("call_model_measure", SCRIPT)
measure = importlib.util.module_from_spec(spec)
spec.loader.exec_module(measure)


def arguments(output):
    return ["--control", "unused-control", "--candidate", "unused-candidate",
            "--module-log", "upstream=unused-log", "--module-log", "candidate=unused-log",
            "--output", str(output)]


class RoundSelectionTests(unittest.TestCase):
    def test_accepted_round_modes(self):
        for extra, rounds, confirmation in (
            ([], 6, False),
            (["--rounds", "2"], 2, False),
            (["--rounds", "8"], 8, False),
            (["--single-pair-confirmation", "--rounds", "1"], 1, True),
        ):
            with self.subTest(extra=extra):
                args = measure.parse_args(arguments("unused-output") + extra)
                self.assertEqual(args.rounds, rounds)
                self.assertEqual(args.single_pair_confirmation, confirmation)

    def test_invalid_round_modes_have_no_side_effects(self):
        invalid = [["--rounds", str(rounds)] for rounds in (-2, 0, 1, 3)]
        invalid += [["--single-pair-confirmation"]]
        invalid += [["--single-pair-confirmation", "--rounds", str(rounds)]
                    for rounds in (-1, 0, 2, 3)]
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp) / "output"
            with mock.patch.object(measure, "load") as load, \
                 mock.patch.object(measure.os, "sched_getaffinity") as affinity, \
                 mock.patch.object(measure.Path, "mkdir") as mkdir, \
                 mock.patch.object(measure.subprocess, "Popen") as launch, \
                 mock.patch.object(measure.subprocess, "run") as run:
                for extra in invalid:
                    with self.subTest(extra=extra), \
                         mock.patch.object(measure.sys, "stderr", new_callable=io.StringIO) as stderr:
                        with self.assertRaises(SystemExit) as error:
                            measure.main(arguments(output) + extra)
                        self.assertEqual(error.exception.code, 2)
                        self.assertIn("error:", stderr.getvalue())
                        self.assertFalse(output.exists())
                affinity.assert_not_called()
                mkdir.assert_not_called()
                load.assert_not_called()
                launch.assert_not_called()
                run.assert_not_called()

    def test_request_survives_preflight_rejection(self):
        for extra, request in (
            (["--rounds", "2"], {"mode": "balanced", "requested_rounds": 2}),
            (["--single-pair-confirmation", "--rounds", "1"],
             {"mode": "single-pair-confirmation", "requested_rounds": 1}),
        ):
            with self.subTest(extra=extra), tempfile.TemporaryDirectory() as temp:
                output = Path(temp) / "output"
                checkpoint = SimpleNamespace(
                    EXPECTED_MODULE_COUNTS={}, CONTROL_HEADS={}, __file__=str(SCRIPT),
                    parse_module_logs=mock.Mock(return_value={"upstream": {}, "candidate": {}}),
                    snapshot_all=mock.Mock(return_value={"errors": ["mock preflight rejection"]}),
                )
                bench = SimpleNamespace(BENCHMARKS=[
                    SimpleNamespace(name="logging_silent", group="kernels")])
                with mock.patch.object(measure, "load", side_effect=[checkpoint, bench]), \
                     mock.patch.object(measure.os, "sched_getaffinity", return_value={15}), \
                     mock.patch.object(measure.subprocess, "Popen") as launch, \
                     mock.patch.object(measure.subprocess, "run") as run:
                    with self.assertRaisesRegex(RuntimeError, "preflight rejected"):
                        measure.main(arguments(output) + extra)
                self.assertEqual(json.loads((output / "run-request.json").read_text()), request)
                self.assertEqual(json.loads((output / "commands.json").read_text()), [])
                self.assertEqual(json.loads((output / "status.json").read_text())["phase"], "preflight")
                self.assertFalse((output / "summary.json").exists())
                checkpoint.snapshot_all.assert_called_once()
                launch.assert_not_called()
                run.assert_not_called()


if __name__ == "__main__":
    unittest.main()
