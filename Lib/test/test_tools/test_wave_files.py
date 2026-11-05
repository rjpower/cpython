"""Tests for Tools/build/wave_files.py, which packs the files of a wave into
batches of similar call-site counts."""

import contextlib
import io
import os
import tempfile
import unittest

from test.test_tools import imports_under_tool, skip_if_missing

skip_if_missing("build")

with imports_under_tool("build"):
    import wave_files


def totals(batches, counts):
    return [sum(counts[f] for f in batch) for batch in batches]


class PackTests(unittest.TestCase):

    def test_equal_files_spread(self):
        counts = {f"f{i}.c": 10 for i in range(8)}
        batches = wave_files.pack(counts, 4)
        self.assertEqual(totals(batches, counts), [20, 20, 20, 20])

    def test_greedy(self):
        counts = {"a": 50, "b": 30, "c": 20, "d": 20, "e": 10, "f": 10}
        batches = wave_files.pack(counts, 2)
        self.assertEqual(sorted(totals(batches, counts)), [70, 70])

    def test_never_split_and_large_file_alone(self):
        counts = {"big.c": 100, "x.c": 5, "y.c": 5, "z.c": 5}
        batches = wave_files.pack(counts, 3)
        placed = [f for batch in batches for f in batch]
        self.assertEqual(sorted(placed), sorted(counts))
        self.assertIn(["big.c"], batches)

    def test_last_files_form_final_batch(self):
        counts = {"a": 10, "b": 10, "c": 10, "d": 10, "tail1": 3, "tail2": 4}
        batches = wave_files.pack(counts, 3, last=["tail1", "tail2"])
        self.assertEqual(len(batches), 3)
        self.assertEqual(sorted(batches[-1]), ["tail1", "tail2"])
        self.assertEqual(totals(batches[:2], counts), [20, 20])

    def test_group_keeps_directory_together(self):
        counts = {"Modules/a.c": 10, "Objects/z.c": 10, "Objects/c.c": 5}
        groups = {f: wave_files.group_key(f, [], "dir") for f in counts}
        grouped = wave_files.pack(counts, 2, groups)
        self.assertIn({"Objects/z.c", "Objects/c.c"},
                      [set(batch) for batch in grouped])
        ungrouped = wave_files.pack(counts, 2)
        self.assertNotIn({"Objects/z.c", "Objects/c.c"},
                         [set(batch) for batch in ungrouped])

    def test_invalid_batch_counts(self):
        with self.assertRaises(SystemExit):
            wave_files.pack({"a": 1}, 0)
        with self.assertRaises(SystemExit):
            wave_files.pack({"a": 1, "b": 2}, 1, last=["b"])

    def test_deterministic(self):
        counts = {f"f{i}.c": i % 4 + 1 for i in range(20)}
        self.assertEqual(wave_files.pack(counts, 3),
                         wave_files.pack(counts, 3))


class GroupKeyTests(unittest.TestCase):

    def test_modes(self):
        sites = [
            {"file": "Modules/_io/a.c", "family": "list"},
            {"file": "Modules/_io/a.c", "family": "dict"},
            {"file": "Modules/_io/a.c", "family": "list"},
        ]
        self.assertEqual(
            wave_files.group_key("Modules/_io/a.c", sites, "dir"),
            "Modules/_io")
        self.assertEqual(
            wave_files.group_key("Modules/_io/a.c", sites, "family"), "list")
        self.assertEqual(
            wave_files.group_key("Modules/_io/a.c", sites, "none"),
            "Modules/_io/a.c")


SITES_TSV = """\
file\tline\tcol\tfunction\tclass\tfamily\tnote
# a comment
Objects/a.c\t1\t2\tf\t\tlist\t
Objects/a.c\t3\t4\tg\t\tlist\tnote

Python/b.c\t5\t6\th\t\ttuple\t
Python/ceval.c\t7\t8\ti\t\tdict\t
Python/generated_cases.c.h\t9\t1\tj\t\tdict\t
"""


class SitesTests(unittest.TestCase):

    def test_read_sites(self):
        sites = wave_files.read_sites(SITES_TSV.splitlines())
        self.assertEqual(len(sites), 5)
        self.assertEqual(sites[0]["file"], "Objects/a.c")
        self.assertEqual(sites[1]["note"], "note")
        self.assertEqual(sites[2]["family"], "tuple")
        self.assertEqual(wave_files.count_by_file(sites), {
            "Objects/a.c": 2,
            "Python/b.c": 1,
            "Python/ceval.c": 1,
            "Python/generated_cases.c.h": 1,
        })

    def run_main(self, *args):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "sites.tsv")
            with open(path, "w", encoding="utf-8") as f:
                f.write(SITES_TSV)
            out = io.StringIO()
            err = io.StringIO()
            with contextlib.redirect_stdout(out), \
                    contextlib.redirect_stderr(err):
                wave_files.main([path, *args])
        return out.getvalue(), err.getvalue()

    def test_main_tsv_and_default_never(self):
        out, err = self.run_main("--batches", "2", "--tsv", "--summary")
        self.assertEqual(out.splitlines(),
                         ["1\tObjects/a.c\t2", "2\tPython/b.c\t1"])
        self.assertEqual(len(err.splitlines()), 2)
        self.assertTrue(err.startswith("batch 1: 2 calls, 1 files: Objects (2)"))

    def test_main_json_reports_excluded(self):
        import json
        out, _ = self.run_main("--batches", "2")
        report = json.loads(out)
        self.assertEqual(report["total"], 3)
        self.assertEqual(report["excluded"], [
            "Python/ceval.c", "Python/generated_cases.c.h"])
        self.assertEqual(report["batches"][0]["index"], 1)

    def test_excluded_defaults_listed(self):
        self.assertIn("Python/ceval.c", wave_files.NEVER_BATCHED)
        self.assertIn("Python/bytecodes.c", wave_files.NEVER_BATCHED)
        self.assertIn("Python/ceval_macros.h", wave_files.NEVER_BATCHED)


if __name__ == "__main__":
    unittest.main()
