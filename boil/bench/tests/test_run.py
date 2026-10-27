"""Smoke test of run.py and compare.py on one kernel at the smallest loop count.

Run with any Python 3.10+: python -m unittest discover -s boil/bench/tests
The interpreter under test is the one running the test; set BOIL_BENCH_PYTHON
to measure another build.
"""

import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

BENCH = Path(__file__).resolve().parents[1]
PYTHON = os.environ.get("BOIL_BENCH_PYTHON", sys.executable)


def perf_counts() -> bool:
    if shutil.which("perf") is None:
        return False
    proc = subprocess.run(["perf", "stat", "-x,", "-e", "instructions:u", "true"],
                          check=False, capture_output=True, text=True)
    return proc.returncode == 0 and "<not" not in proc.stderr


class RunTest(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix="boil-bench-test-"))
        self.addCleanup(shutil.rmtree, self.tmp)

    def run_bench(self, metric: str, *extra: str, name: str = "richards",
                  group: str = "kernels") -> dict:
        out = self.tmp / f"{metric}.json"
        subprocess.run([sys.executable, str(BENCH / "run.py"), "--build", PYTHON,
                        "--label", "test", "--set", group, "--only", name,
                        "--metric", metric, "--loops", "1", *extra, "-o", str(out)],
                       check=True, capture_output=True, text=True)
        return json.loads(out.read_text())

    def check_entry(self, report: dict, metric: str, runs: int,
                    name: str = "richards") -> dict:
        self.assertEqual(report["schema"], "boil-bench/1")
        self.assertEqual(report["build"]["label"], "test")
        self.assertIn("sha", report["commit"])
        entry = report["results"][metric][name]
        self.assertEqual(entry["group"], report["results"][metric][name]["group"])
        self.assertEqual(entry["runs"], runs)
        self.assertEqual(len(entry["values"]), runs)
        self.assertGreater(entry["value"], 0)
        return entry

    def test_time(self):
        report = self.run_bench("time", "--time-runs", "2")
        entry = self.check_entry(report, "time", 2)
        self.assertEqual(entry["loops"], 1)

    @unittest.skipUnless(os.access("/usr/bin/time", os.X_OK), "no GNU time")
    def test_rss_of_a_variant(self):
        # logging_silent is one of three benchmarks in logging.py, chosen by setup().
        report = self.run_bench("rss", "--rss-runs", "1", name="logging_silent")
        self.check_entry(report, "rss", 1, name="logging_silent")

    @unittest.skipUnless(perf_counts(), "perf cannot count instructions:u here")
    def test_instr_and_compare(self):
        report = self.run_bench("instr", "--instr-runs", "1")
        entry = self.check_entry(report, "instr", 1)
        self.assertEqual(entry["loops"], [1, 3])
        path = self.tmp / "instr.json"
        table = subprocess.run([sys.executable, str(BENCH / "compare.py"), str(path), str(path)],
                               check=True, capture_output=True, text=True).stdout
        self.assertIn("| richards |", table)
        self.assertIn("| geomean |  | 1.000 (1/1) |", table)

    @unittest.skipUnless(perf_counts(), "perf cannot count instructions:u here")
    def test_micro_subtracts_its_skeleton(self):
        report = self.run_bench("instr", "--instr-runs", "1", name="attr_load_poly",
                                group="micro")
        table = report["results"]["instr"]
        self.assertEqual(set(table), {"attr_load_poly", "empty_list_loop"})
        entry = self.check_entry(report, "instr", 1, name="attr_load_poly")
        self.assertEqual(entry["inner"], 1000)
        self.assertEqual(entry["skeleton"], "empty_list_loop")
        self.assertEqual(entry["value"], entry["raw"] - table["empty_list_loop"]["value"])


if __name__ == "__main__":
    unittest.main()
