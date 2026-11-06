"""Native child receipt and marker admission fixtures for check_r27."""

import json
import os
from pathlib import Path
import tempfile
import unittest

from test.test_tools import imports_under_tool, skip_if_missing

skip_if_missing("build")
with imports_under_tool("build"):
    import check_r27

PARENT = "a" * 40
CHILD = "b" * 40


def marker(pid, ppid):
    return (f"version: 2\npid: {pid}\nppid: {ppid}\ntag: test\n"
            f"argv: [\"python\"]\nimage_dev: 1\nimage_ino: 2\n")


def root_launch():
    return [{"version": 1, "pid": 100, "ppid": os.getpid(),
             "argv": ["python"], "image_dev": 1, "image_ino": 2}]


def receipt():
    return {"version": 1, "parent_pid": 100, "child_pid": 101,
            "parent_stem": PARENT, "serial": 1, "source": "fork",
            "parent_image_dev": 1, "parent_image_ino": 2,
            "candidate_count": 0, "candidates": []}


class NativeIdentityTests(unittest.TestCase):
    def make_dir(self, directory, child=False):
        directory = Path(directory)
        (directory / f"{PARENT}.done").write_text(marker(100, os.getpid()))
        (directory / "root-launch.json").write_text(json.dumps(root_launch()))
        folded = check_r27.Folded()
        folded.identity_required = True
        folded.dumps = 1 + child
        folded.dump_identities[PARENT] = {"pid": 100, "tag": "test"}
        if child:
            (directory / f"{CHILD}.done").write_text(marker(101, 100))
            (directory / f"{PARENT}.1.child").write_text(json.dumps(receipt()))
            folded.dump_identities[CHILD] = {"pid": 101, "tag": "test"}
        return folded

    def test_root_and_native_child(self):
        with tempfile.TemporaryDirectory() as tmp:
            folded = self.make_dir(tmp, child=True)
            folded.read_markers(tmp)
            self.assertTrue(folded.identity_complete, folded.identity_errors)
            self.assertEqual(len(folded.child_receipts), 1)

    def test_missing_receipt_and_unenumerated_root_fail(self):
        with tempfile.TemporaryDirectory() as tmp:
            folded = self.make_dir(tmp, child=True)
            (Path(tmp) / f"{PARENT}.1.child").unlink()
            folded.read_markers(tmp)
            self.assertFalse(folded.identity_complete)
            self.assertTrue(any("no root launch or native receipt" in e
                                for e in folded.identity_errors))
        with tempfile.TemporaryDirectory() as tmp:
            folded = self.make_dir(tmp)
            (Path(tmp) / "root-launch.json").unlink()
            folded.read_markers(tmp)
            self.assertFalse(folded.identity_complete)

    def test_bad_receipt_and_v1_marker_fail(self):
        with tempfile.TemporaryDirectory() as tmp:
            folded = self.make_dir(tmp, child=True)
            path = Path(tmp) / f"{PARENT}.1.child"
            bad = receipt()
            bad["parent_pid"] = 999
            path.write_text(json.dumps(bad))
            folded.read_markers(tmp)
            self.assertFalse(folded.identity_complete)
            self.assertTrue(any("unmatched parent interval" in e
                                for e in folded.identity_errors))
        with tempfile.TemporaryDirectory() as tmp:
            folded = self.make_dir(tmp)
            path = Path(tmp) / f"{PARENT}.done"
            path.write_text(path.read_text().replace("version: 2", "version: 1"))
            folded.read_markers(tmp)
            self.assertFalse(folded.identity_complete)

    def test_duplicate_keys_and_nonregular_receipts_fail(self):
        with tempfile.TemporaryDirectory() as tmp:
            folded = self.make_dir(tmp, child=True)
            path = Path(tmp) / f"{PARENT}.1.child"
            path.write_text(path.read_text().replace('"version": 1,',
                                                    '"version": 1, "version": 1,', 1))
            folded.read_markers(tmp)
            self.assertFalse(folded.identity_complete)
            self.assertTrue(any("malformed child receipt" in e
                                for e in folded.identity_errors))
        with tempfile.TemporaryDirectory() as tmp:
            folded = self.make_dir(tmp, child=True)
            path = Path(tmp) / f"{PARENT}.1.child"
            path.unlink()
            path.symlink_to(Path(tmp) / "root-launch.json")
            folded.read_markers(tmp)
            self.assertFalse(folded.identity_complete)
            self.assertTrue(any("malformed child receipt" in e
                                for e in folded.identity_errors))

    def test_unfinished_receipt_and_sticky_failure_fail(self):
        with tempfile.TemporaryDirectory() as tmp:
            folded = self.make_dir(tmp, child=True)
            (Path(tmp) / f"{PARENT}.1.child").rename(
                Path(tmp) / f"{PARENT}.1.child.tmp")
            folded.read_markers(tmp)
            self.assertFalse(folded.identity_complete)
            self.assertTrue(any("unfinished child receipt" in e
                                for e in folded.identity_errors))
        folded = check_r27.Folded()
        folded.add_text("Stats child receipt errors: 1\n", "case.txt",
                        require_child_receipts=True)
        self.assertIn("native child receipt publication failed",
                      folded.accounting_errors)


if __name__ == "__main__":
    unittest.main()
