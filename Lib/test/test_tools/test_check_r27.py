"""Tools/build/check_r27.py: folding dumps, classing owners and the gates.

The resolver is replaced by a table, so no build is needed. The real-dump
test runs when R27_STATS_BUILD names a --enable-pystats build.
"""

import json
import os
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from test.test_tools import imports_under_tool, skip_if_missing
from test.libregrtest import worker
from test.libregrtest.result import State, TestResult as LibregrtestResult

skip_if_missing("build")

with imports_under_tool("build"):
    import check_r27

SRC = Path(check_r27.DEFAULT_SRC)


class TableResolver(check_r27.Resolver):
    """units: {build-id: {address: absolute cu path}}; None marks no DWARF."""

    def __init__(self, units, trees=None, paths=None):
        self.units = units
        self.trees = trees or {}
        self.object_paths = paths or {}

    def lookup(self, bid, addr):
        return (self.units.get(bid) or {}).get(addr)

    def in_tree(self, bid):
        if bid in self.trees:
            return self.trees[bid]
        return None if self.units.get(bid) is None else True

    def path(self, bid):
        return self.object_paths.get(bid)


def unit(rel):
    return str(SRC / rel)


def fold(*texts):
    folded = check_r27.Folded()
    for text in texts:
        folded.add_text(text)
    return folded


def classes(folded, resolver, cdb=None):
    summary = check_r27.resolve(folded, resolver, SRC, cdb)
    return summary, {(r["site"], r["via"]): r["class"] for r in summary["rows"]}


class ClassifyTests(unittest.TestCase):
    def test_each_class(self):
        dump = "\n".join([
            "Box object [0]: /b/python build-id=aa",
            "Box object [1]: /b/_testcapi.so build-id=bb",
            "Box object [2]: /b/_testinternalcapi.so build-id=cc",
            "Box object [3]: /lib/libz.so build-id=ee",
            "Box object [4]: /b/nodwarf.so build-id=ff",
            "Box owner (thunk, number) 0+0x10 at 0+0x1: 1",
            "Box owner (thunk, list) 0+0x20 at 0+0x1: 2",
            "Box owner (thunk, tuple) 1+0x10 at 0+0x1: 3",
            "Box owner (thunk, dict) 2+0x10 at 0+0x1: 4",
            "Box owner (thunk, set) 3+0x10 at 0+0x1: 5",
            "Box owner (call adapter, call adapter) foreign at 0+0x1: 6",
            "Box owner (thunk, repr) ? at 0+0x1: 7",
            "Box owner (core, parser) - at 0+0x30: 8",
            "Box owner (thunk, frame) 4+0x10 at 0+0x1: 9",
            "Box owner (thunk, field) 0+0x99 at 0+0x1: 10",
            ""])
        resolver = TableResolver({
            "aa": {0x10: unit("Objects/abstract.c"),
                   0x20: unit("Modules/_json.c"),
                   0x30: unit("Python/ceval.c")},
            "bb": {0x10: unit("Modules/_testcapimodule.c")},
            "cc": {0x10: unit("Modules/_testinternalcapi.c")},
            "ee": {0x10: "/usr/src/zlib/inflate.c"},
            "ff": None,
        }, trees={"ee": False}, paths={"ff": "/b/_testcapi.cpython-314.so"})
        _, got = classes(fold(dump), resolver)
        self.assertEqual(got, {
            ("number", "owner"): "core",
            ("list", "owner"): "core-pending",
            ("tuple", "owner"): "compat-test",
            ("dict", "owner"): "test",
            ("set", "owner"): "external",
            ("call adapter", "owner"): "foreign",
            ("repr", "owner"): "unresolved",
            ("parser", "site"): "core",
            ("frame", "owner"): "compat-test",  # no DWARF: by file name
            ("field", "owner"): "unresolved",  # in-tree object, no CU
        })

    def test_no_dwarf_unknown_name_is_unresolved(self):
        dump = ("Box object [0]: /b/other.so build-id=ff\n"
                "Box owner (thunk, list) 0+0x10 at 0+0x1: 1\n")
        resolver = TableResolver({"ff": None}, paths={"ff": "/b/other.so"})
        _, got = classes(fold(dump), resolver)
        self.assertEqual(got, {("list", "owner"): "unresolved"})

    def test_stray_cu_in_tree_object(self):
        dump = ("Box object [0]: /b/python build-id=aa\n"
                "Box owner (thunk, list) 0+0x10 at 0+0x1: 1\n")
        resolver = TableResolver({"aa": {0x10: "/elsewhere/x.c"}})
        summary, got = classes(fold(dump), resolver)
        self.assertEqual(got, {("list", "owner"): "unresolved"})
        self.assertEqual(summary["stray_cus"], ["/elsewhere/x.c"])

    def test_core_box_uses_site_not_owner(self):
        dump = ("Box object [0]: /b/python build-id=aa\n"
                "Box owner (core, list) - at 0+0x20: 1\n")
        resolver = TableResolver({"aa": {0x20: unit("Modules/_json.c")}})
        _, got = classes(fold(dump), resolver)
        self.assertEqual(got, {("list", "site"): "core-pending"})

    def test_compile_database(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / "m.c"
            source.write_text("#define Py_BUILD_CORE\n")
            cdb = {os.path.realpath(unit("Modules/m.c")): {
                "directory": tmp, "file": "m.c",
                "arguments": ["cc", "-DPy_BUILD_CORE", "-c", "m.c"]}}
            self.assertEqual(
                check_r27.unit_class(unit("Modules/m.c"), SRC, cdb),
                ("core", "Modules/m.c"))
            self.assertEqual(
                check_r27.unit_class(unit("Modules/other.c"), SRC, cdb),
                ("core-pending", "Modules/other.c"))


class FoldTests(unittest.TestCase):
    def test_index_remapping_and_same_build_id_at_two_paths(self):
        one = ("Stats process: pid=1 tag=a\n"
               "Box object [0]: /b/python build-id=aa\n"
               "Box object [1]: /v/bin/python build-id=aa\n"
               "Box owner (thunk, list) 1+0x10 at 0+0x1: 5\n"
               "Boxing by site (list): 5\n"
               "Object box cached: 2\n")
        two = ("Stats process: pid=2 tag=b\n"
               "Box object [0]: /x/ext.so build-id=bb\n"
               "Box object [1]: /b/python build-id=aa\n"
               "Box owner (thunk, list) 1+0x10 at 1+0x1: 7\n"
               "Box owner (thunk, list) 0+0x10 at 1+0x1: 1\n"
               "Boxing by site (list): 8\n"
               "Object box cached: 3\n")
        folded = fold(one, two)
        self.assertEqual(folded.dumps, 2)
        self.assertEqual(folded.paths["aa"], {"/b/python", "/v/bin/python"})
        # Both dumps name the same build product, whatever the index or path.
        self.assertEqual(folded.owners[("thunk", "list", "aa+0x10", "aa+0x1")], 12)
        self.assertEqual(folded.owners[("thunk", "list", "bb+0x10", "aa+0x1")], 1)
        self.assertEqual(folded.site_totals, {"list": 13})
        self.assertEqual(folded.box_cached, 5)

    def test_object_only_in_objects_directory(self):
        with tempfile.TemporaryDirectory() as tmp:
            objects = Path(tmp, "objects")
            objects.mkdir()
            (objects / "dd").write_bytes(b"not an elf")
            resolver = check_r27.ElfResolver(SRC, dump_dir=tmp,
                                             paths={"dd": {"/gone/ext.so"}})
            self.assertEqual(resolver.path("dd"), str(objects / "dd"))
            self.assertIsNone(resolver.path("ee"))
            # Unreadable DWARF: classed by name, so unresolved here.
            self.assertEqual(
                check_r27.classify_ref("dd+0x1", resolver, SRC),
                ("unresolved", str(objects / "dd")))

    def test_address_in_no_object(self):
        dump = ("Box object [0]: /b/python build-id=aa\n"
                "Box owner (thunk, list) 7+0x10 at 0+0x1: 1\n"
                "Box owner (thunk, tuple) ? at 0+0x1: 1\n")
        folded = fold(dump)
        self.assertEqual({k[2] for k in folded.owners}, {"?"})
        _, got = classes(folded, TableResolver({"aa": {}}))
        self.assertEqual(set(got.values()), {"unresolved"})

    def test_dir_ignores_tmp_and_deletes_when_asked(self):
        with tempfile.TemporaryDirectory() as tmp:
            d = Path(tmp)
            (d / "a.txt").write_text("Stats process: pid=1 tag=t\nStats resets: 1\n")
            (d / "a.done").write_text(
                'pid: 1\nppid: 0\ntag: t\nargv: ["python"]\n')
            (d / "b.tmp").write_text("Stats resets: 9\n")
            folded = check_r27.Folded()
            folded.fold_dir(d, delete=True)
            folded.read_markers(d)
            self.assertEqual((folded.dumps, folded.resets), (1, 1))
            self.assertEqual(folded.leftover_run, [])
            self.assertTrue(folded.identity_complete)
            self.assertEqual({p.name for p in d.iterdir()}, {"a.done", "b.tmp"})

    def test_invalid_owner_diagnostics_survive_fold_deletion(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp, "bad.txt")
            rows = ["Stats process: pid=4 tag=test_probe",
                    "Box owner (core, list) - at 0+0x1: 1",
                    "Box owner (core, list) 0+0x2 at 0+0x3: 1"]
            rows.extend(f"Box owner (thunk, list) - at 0+0x{i:x}: 1"
                        for i in range(18))
            path.write_text("\n".join(rows) + "\n")
            folded = check_r27.Folded()
            folded.fold_dir(tmp, delete=True)
            summary = check_r27.resolve(folded, TableResolver({}), SRC)
            self.assertFalse(path.exists())
            self.assertEqual(summary["invalid_owner_count"], 19)
            self.assertEqual(len(summary["invalid_owner_samples"]), 16)
            self.assertEqual(summary["invalid_owner_samples"][0], {
                "dump": "bad.txt", "tag": "test_probe", "kind": "core",
                "site": "list", "owner": "0+0x2", "at": "0+0x3", "count": 1,
            })
            self.assertEqual(summary["invalid_owner_samples"][1]["kind"],
                             "thunk")
            self.assertEqual(summary["invalid_owner_samples"][1]["owner"],
                             "-")
            self.assertEqual(summary["accounting_errors"].count(
                "invalid owner attribution"), 19)

    def test_markers(self):
        with tempfile.TemporaryDirectory() as tmp:
            d = Path(tmp)
            (d / "x.run").write_text('pid: 4\nppid: 1\ntag: test_foo\nargv: ["python","-c","x"]\n')
            (d / "y.done").write_text('pid: 5\nppid: 1\ntag: done\nargv: ["python"]\n')
            (d / "y.txt").write_text("Stats process: pid=5 tag=done\n")
            (d / "z.spawn").write_text("test_bar\tpython -m x\n")
            folded = check_r27.Folded()
            folded.fold_dir(d)
            folded.read_markers(d)
            self.assertEqual([(r["id"], r["tag"]) for r in folded.leftover_run],
                             [("x", "test_foo")])
            self.assertEqual(folded.spawn,
                             [{"tag": "test_bar", "argv": "python -m x"}])
            self.assertEqual(folded.leftover_run[0]["argv"],
                             ["python", "-c", "x"])
            self.assertTrue(folded.identity_complete)

    def test_process_identity_pairing_and_retention(self):
        with tempfile.TemporaryDirectory() as tmp:
            d = Path(tmp)
            (d / "a.txt").write_text("Stats process: pid=9 tag=probe\n")
            (d / "a.done").write_text(
                'pid: 9\nppid: 2\ntag: probe\n'
                'argv: ["python", "\\u00ff"]\n')
            folded = check_r27.Folded()
            folded.fold_dir(d, delete=True)
            folded.read_markers(d)
            self.assertFalse((d / "a.txt").exists())
            self.assertTrue((d / "a.done").exists())
            self.assertTrue(folded.identity_complete)
            self.assertEqual(folded.dump_identities,
                             {"a": {"pid": 9, "tag": "probe"}})
            self.assertEqual(folded.identity_records["a.done"],
                             {"id": "a", "pid": 9, "ppid": 2,
                              "tag": "probe", "argv": ["python", os.fsdecode(b"\xff")]})
            coverage = check_r27.resolve(
                folded, TableResolver({}), SRC)["identity_coverage"]
            self.assertEqual(coverage["marker_samples"][0]["argv"],
                             ["python", os.fsdecode(b"\xff")])

    def test_process_identity_missing_or_orphan_done_fails_closed(self):
        for files, expected in [
            ({"a.txt": "Stats process: pid=9 tag=probe\n"}, "missing .done"),
            ({"a.done": 'pid: 9\nppid: 2\ntag: probe\nargv: ["python"]\n'},
             "orphan .done"),
        ]:
            with self.subTest(expected=expected), tempfile.TemporaryDirectory() as tmp:
                d = Path(tmp)
                for name, content in files.items():
                    (d / name).write_text(content)
                folded = check_r27.Folded()
                folded.fold_dir(d)
                folded.read_markers(d)
                self.assertFalse(folded.identity_complete)
                self.assertTrue(any(expected in error
                                    for error in folded.accounting_errors))

    def test_repeated_fold_of_same_stem_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            d = Path(tmp)
            (d / "a.txt").write_text("Stats process: pid=9 tag=probe\n")
            (d / "a.done").write_text(
                'pid: 9\nppid: 2\ntag: probe\nargv: ["python"]\n')
            folded = check_r27.Folded()
            folded.fold_dir(d)
            folded.fold_dir(d)
            folded.read_markers(d)
            self.assertFalse(folded.identity_complete)
            self.assertTrue(any("duplicate folded dump stem" in error
                                for error in folded.accounting_errors))

    def test_process_identity_mismatch_zero_and_run_with_dump_fail(self):
        cases = [
            ("mismatch", "a.done",
             'pid: 10\nppid: 2\ntag: other\nargv: ["python"]\n',
             ".done identity does not match"),
            ("zero", "a.zero",
             'pid: 9\nppid: 2\ntag: probe\nargv: ["python"]\n',
             "zero-stats identity"),
            ("run", "a.run",
             'pid: 9\nppid: 2\ntag: probe\nargv: ["python"]\n',
             "contradictory .run"),
        ]
        for name, marker, contents, expected in cases:
            with self.subTest(name=name), tempfile.TemporaryDirectory() as tmp:
                d = Path(tmp)
                (d / "a.txt").write_text("Stats process: pid=9 tag=probe\n")
                (d / marker).write_text(contents)
                if marker != "a.done":
                    (d / "a.done").write_text(
                        'pid: 9\nppid: 2\ntag: probe\nargv: ["python"]\n')
                folded = check_r27.Folded()
                folded.fold_dir(d)
                folded.read_markers(d)
                self.assertFalse(folded.identity_complete)
                self.assertTrue(any(expected in error
                                    for error in folded.accounting_errors))

    def test_process_identity_malformed_and_duplicate_fields_fail(self):
        with tempfile.TemporaryDirectory() as tmp:
            d = Path(tmp)
            (d / "bad.txt").write_text("Stats process: pid=3 tag=t\n")
            (d / "bad.done").write_text(
                'pid: 3\npid: 3\nppid: 1\ntag: t\nargv: ["python"]\n')
            (d / "empty.zero").write_text("")
            folded = check_r27.Folded()
            folded.fold_dir(d)
            folded.read_markers(d)
            self.assertFalse(folded.identity_complete)
            self.assertTrue(any("malformed done identity" in error
                                for error in folded.accounting_errors))
            self.assertTrue(any("malformed zero identity" in error
                                for error in folded.accounting_errors))

    def test_gate_rejects_missing_identity_coverage(self):
        summary = clean_summary()
        del summary["identity_coverage"]
        self.assertTrue(any("identity coverage" in error
                            for error in check_r27.suite_violations(summary)))

    def test_spawn_ledger_empty_and_invalid_utf8_fail_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            d = Path(tmp)
            (d / "empty.spawn").write_bytes(b"")
            (d / "invalid.spawn").write_bytes(bytes([0xff, 0x0a]))
            folded = check_r27.Folded()
            folded.read_markers(d)
            self.assertTrue(any("empty spawn ledger" in error
                                for error in folded.accounting_errors))
            self.assertTrue(any("unreadable spawn ledger" in error
                                for error in folded.accounting_errors))

    def test_marker_argument_boundaries_and_malformed_record(self):
        with tempfile.TemporaryDirectory() as tmp:
            d = Path(tmp)
            (d / "ok.run").write_text(
                'pid: 4\nppid: 1\ntag: test_foo\n'
                'argv: ["python","-c","a\\u000ab","a b"]\n')
            (d / "bad.run").write_text(
                'pid: 5\nppid: 1\ntag: test_foo\nargv: ["python","-c"\n')
            (d / "utf.run").write_text(
                'pid: 6\nppid: 1\ntag: test_foo\n'
                'argv: ["\\u00c3\\u00a9","\\u00ff"]\n')
            (d / "notbytes.run").write_text(
                'pid: 7\nppid: 1\ntag: test_foo\nargv: ["\\u20ac"]\n')
            (d / "new.spawn").write_text(json.dumps({
                "tag": "test_foo", "pid": 4, "event": "subprocess.Popen",
                "shape": "structured", "classification": "direct",
                "executable": "/build/python", "command": None,
                "reason": "-E", "argv": ["python", "-E"],
                "counts": False}) + "\n")
            (d / "bad.spawn").write_text(json.dumps({
                "tag": "test_foo", "pid": 4, "event": "subprocess.Popen",
                "reason": "-E", "argv": ["python", "-E"],
                "counts": "true"}) + "\n")
            folded = check_r27.Folded()
            folded.read_markers(d)
            by_id = {run["id"]: run for run in folded.leftover_run}
            self.assertEqual(by_id["ok"]["argv"],
                             ["python", "-c", "a\nb", "a b"])
            self.assertEqual(by_id["utf"]["argv"],
                             ["é", os.fsdecode(b"\xff")])
            self.assertTrue(any(error.startswith("malformed run identity bad.run:")
                                for error in folded.accounting_errors))
            self.assertTrue(any(error.startswith(
                "malformed run identity notbytes.run:")
                for error in folded.accounting_errors))
            self.assertIn("incomplete spawn record in bad.spawn",
                          folded.accounting_errors)
            self.assertEqual(folded.spawn[0]["argv"], ["python", "-E"])


    def test_spawn_preserves_counted_events(self):
        with tempfile.TemporaryDirectory() as tmp:
            d = Path(tmp)
            launches = [
                {"tag": "counted", "event": "subprocess.Popen",
                 "shape": "structured", "classification": "direct",
                 "executable": "/build/python",
                 "argv": ["python", "-c", "print('x y')"],
                 "counts": True, "reason": "" , "pid": 44},
                {"tag": "uncounted", "event": "subprocess.Popen",
                 "shape": "structured", "classification": "direct",
                 "executable": "/build/python",
                 "argv": ["python", "-E"],
                 "counts": False, "reason": "-E", "pid": 45},
            ]
            (d / "worker.spawn").write_text(
                "".join(json.dumps(item) + "\n" for item in launches))
            folded = check_r27.Folded()
            folded.read_markers(d)
            self.assertEqual(folded.counted_spawn, [launches[0]])
            self.assertEqual(folded.spawn[0]["argv"], launches[1]["argv"])
            self.assertEqual(folded.spawn[0]["pid"], 45)
            self.assertEqual(folded.accounting_errors, [])

    def test_spawn_and_audit_error_ledgers_fail_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            d = Path(tmp)
            (d / "malformed.spawn").write_text("{not-json}\n")
            (d / "opaque.spawn").write_text(json.dumps({
                "tag": "shell", "pid": 7, "event": "subprocess.Popen",
                "executable": "/bin/sh", "reason": "opaque shell command",
                "command": None, "shape": "structured",
                "classification": "opaque-shell", "counts": None,
                "argv": ["sh", "-c", "python -c 'pass'"],
            }) + "\n")
            (d / "truncated.spawn").write_text(json.dumps({
                "tag": "test", "shape": "structured", "counts": False,
                "argv": ["python"],
            }))
            (d / "incomplete-counted.spawn").write_text(json.dumps({
                "tag": "counted", "shape": "structured", "counts": True,
                "argv": ["python"],
            }) + "\n")
            (d / "empty.audit-error").write_text("")
            folded = check_r27.Folded()
            folded.read_markers(d)
            self.assertTrue(any("malformed spawn JSON" in error
                                for error in folded.accounting_errors))
            self.assertTrue(any("audit error ledger present" in error
                                for error in folded.accounting_errors))
            self.assertTrue(any("truncated spawn ledger" in error
                                for error in folded.accounting_errors))
            self.assertTrue(any("incomplete spawn record" in error
                                for error in folded.accounting_errors))
            self.assertEqual(len(folded.opaque_spawn), 1)
            self.assertTrue(any("opaque shell launch" in error
                                for error in folded.accounting_errors))
            summary = {
                "dumps": 1, "accounting_errors": folded.accounting_errors,
                "rows": [], "drops": {}, "leftover_run": [], "spawn": [],
                "resets_by_tag": {}, "bad_dumps": [],
            }
            self.assertTrue(check_r27.suite_violations(summary))

    def test_unreadable_audit_error_ledger_fails_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            d = Path(tmp)
            path = d / "worker.audit-error"
            path.write_text('{"stage":"spawn-write"}\n')
            with patch.object(Path, "read_bytes", side_effect=OSError("denied")):
                folded = check_r27.Folded()
                folded.read_markers(d)
            self.assertTrue(any("unreadable audit error ledger" in error
                                for error in folded.accounting_errors))


class WorkerLaunchTests(unittest.TestCase):
    def test_structured_argv_and_executable_identity(self):
        python = "/build/python"
        launch = worker._launch(
            "subprocess.Popen",
            (python, [b"fake-argv0", b"-c", b'print("a b")'], None, {}))
        self.assertEqual(launch.argv, ["fake-argv0", "-c", 'print("a b")'])
        self.assertEqual(launch.executable, python)
        target, _, reason = worker._classify_launch(launch, [python])
        self.assertEqual(target, launch.argv)
        self.assertIsNone(reason)
        self.assertEqual(worker._predict(
            ["fake-argv0", "-E"], {"PYTHONSTATS": "1"}),
            (False, "no -X pystats and -E or -I ignores PYTHONSTATS"))
        self.assertEqual(worker._predict(
            ["fake-argv0", "-X", "pystats", "-X", "pystats_dir=/tmp"], {}),
            (True, ""))

        payload_only = worker._launch(
            "subprocess.Popen",
            ("/other/python", ["other", "-c", python], None, {}))
        target, _, reason = worker._classify_launch(payload_only, [python])
        self.assertIsNone(target)
        self.assertIsNone(reason)

        env_launch = worker._launch(
            "subprocess.Popen",
            ("/usr/bin/env", ["env", "-i", "PYTHONSTATS=1",
                              "PYTHON_PYSTATS_DIR=/tmp", python, "-X", "pystats"],
             None, {}))
        target, child_env, reason = worker._classify_launch(env_launch, [python])
        self.assertEqual(target, [python, "-X", "pystats"])
        self.assertEqual(child_env["PYTHONSTATS"], "1")
        self.assertIsNone(reason)

        unsupported_env = worker._launch(
            "subprocess.Popen", ("/usr/bin/env", ["env", "-u", "NAME", python], None, {}))
        target, _, reason = worker._classify_launch(unsupported_env, [python])
        self.assertIsNone(target)
        self.assertEqual(reason, "unsupported env option")

        unknown_env = worker._launch(
            "subprocess.Popen", ("/usr/bin/env", ["env", "grep", "needle"], None, {}))
        target, _, reason = worker._classify_launch(unknown_env, [python])
        self.assertIsNone(target)
        self.assertEqual(reason, "ambiguous relative env target")

    def test_shell_command_is_opaque(self):
        launch = worker._launch("os.system", ("python -c 'print(1 2)'",))
        self.assertEqual(launch.shape, "opaque-shell")
        self.assertIsNone(launch.argv)
        self.assertEqual(launch.command, "python -c 'print(1 2)'")
        target, _, reason = worker._classify_launch(launch, ["/build/python"])
        self.assertIsNone(target)
        self.assertEqual(reason, "opaque shell command")

        shell = worker._launch(
            "subprocess.Popen",
            ("/bin/sh", ["sh", "-c", "/build/python -X pystats"], None, {}))
        target, _, reason = worker._classify_launch(shell, ["/build/python"])
        self.assertIsNone(target)
        self.assertEqual(reason, "opaque shell command")
        self.assertEqual(shell.argv[2], "/build/python -X pystats")

        relative = worker._launch(
            "subprocess.Popen", (None, ["python", "-c", "pass"], None, {}))
        target, _, reason = worker._classify_launch(
            relative, ["/build/python"])
        self.assertIsNone(target)
        self.assertEqual(reason, "ambiguous relative executable")
        alias = worker._launch(
            "subprocess.Popen", (None, ["myrunner", "-c", "pass"], None, {}))
        target, _, reason = worker._classify_launch(alias, ["/build/python"])
        self.assertIsNone(target)
        self.assertEqual(reason, "ambiguous relative executable")

    def test_custom_environment_mapping_is_not_iterated_by_audit(self):
        class HostileEnv(dict):
            def items(self):
                raise AssertionError("audit hook must not call env mapping")

        launch = worker._launch(
            "subprocess.Popen",
            ("/build/python", ["python", "-E"], None, HostileEnv()))
        self.assertEqual(launch.shape, "structured")
        self.assertEqual(launch.command, "opaque environment mapping")
        target, _, reason = worker._classify_launch(launch, ["/build/python"])
        self.assertIsNone(target)
        self.assertEqual(reason, "opaque shell command")

    def test_dual_ledger_failure_reaches_result(self):
        with tempfile.TemporaryDirectory() as tmp:
            with patch.object(worker, "_build_products", return_value=["/build/python"]):
                hook, failure_state = worker._make_spawn_hook(tmp)
            event_args = ("/build/python", ["python", "-c", "pass"], None, {})
            with patch.object(worker.os, "open", side_effect=OSError("write failed")):
                hook("subprocess.Popen", event_args)
            self.assertTrue(failure_state["failed"])
            self.assertEqual({error["stage"] for error in failure_state["errors"]},
                             {"spawn-write", "error-write"})
            result = LibregrtestResult("test_worker_capture", state=State.PASSED,
                                       errors=[("test_case", "prior test error")])
            worker._apply_audit_failures(result, failure_state)
            self.assertEqual(result.state, State.FAILED)
            self.assertEqual(result.errors[0], ("test_case", "prior test error"))
            self.assertIn("pystats audit capture failed", result.errors[-1][1])

    def test_short_spawn_write_fails_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            with patch.object(worker.os, "write", return_value=1):
                with self.assertRaises(OSError):
                    worker._write_record(os.path.join(tmp, "worker.spawn"),
                                         {"event": "test"})

    def test_spawn_failure_records_error_ledger(self):
        with tempfile.TemporaryDirectory() as tmp:
            python = "/build/python"
            with patch.object(worker, "_build_products", return_value=[python]):
                hook, failure_state = worker._make_spawn_hook(tmp)
            real_open = os.open

            def open_spawn_fail_audit_ok(path, flags, mode=0o777):
                if str(path).endswith(".spawn"):
                    raise OSError("spawn write failed")
                return real_open(path, flags, mode)

            with patch.object(worker.os, "open", side_effect=open_spawn_fail_audit_ok):
                hook("subprocess.Popen", (python, ["python", "-c", "pass"], None, {}))
            self.assertTrue(failure_state["failed"])
            ledgers = list(Path(tmp).glob("*.audit-error"))
            self.assertEqual(len(ledgers), 1)
            error = json.loads(ledgers[0].read_text())
            self.assertEqual((error["stage"], error["event"]),
                             ("spawn-write", "subprocess.Popen"))

    def test_hook_writes_structured_counted_event(self):
        with tempfile.TemporaryDirectory() as tmp:
            python = "/build/python"
            with patch.object(worker, "_build_products", return_value=[python]):
                hook, failure_state = worker._make_spawn_hook(tmp)
            argv = ["fake-argv0", "-X", "pystats", "-X", f"pystats_dir={tmp}",
                    "-c", "print('a b')"]
            with patch.dict(os.environ, {"PYTHON_PYSTATS_TAG": "test_spawn"}):
                hook("subprocess.Popen", (python, argv, None, {}))
            self.assertFalse(failure_state["failed"])
            record = json.loads((Path(tmp) / "test_spawn.spawn").read_text())
            self.assertEqual(record["argv"], argv)
            self.assertTrue(record["counts"])


def clean_summary(**changes):
    summary = {"dumps": 1, "accounting_errors": [], "rows": [], "drops": {"thunk": 0}, "leftover_run": [],
               "spawn": [], "resets_by_tag": {}, "bad_dumps": [],
               "identity_coverage": {"complete": True}}
    summary.update(changes)
    return summary


class GateTests(unittest.TestCase):
    def violations(self, summary, crashes=(), uncounted=()):
        return check_r27.suite_violations(summary, crashes, uncounted)

    def test_clean(self):
        self.assertEqual(self.violations(clean_summary()), [])

    def test_debt_classes(self):
        for cls, bad in [("core", True), ("core-pending", True),
                         ("unresolved", True), ("test", False),
                         ("compat-test", False), ("external", False),
                         ("foreign", False)]:
            rows = [{"class": cls, "count": 3}]
            with self.subTest(cls):
                self.assertEqual(bool(self.violations(clean_summary(rows=rows))), bad)

    def test_drops(self):
        out = self.violations(clean_summary(drops={"thunk": 2, "core": 0}))
        self.assertEqual(len(out), 1)
        self.assertIn("drops (thunk)", out[0])

    def test_leftover_run(self):
        run = {"id": "r", "tag": "test_crash", "pid": "3", "ppid": "1",
               "argv": ["/build/python", "-c", "x"]}
        summary = clean_summary(leftover_run=[run], build="/build")
        self.assertTrue(self.violations(summary, crashes={"test_crash"}))
        rules = {"runs": {("test_crash", ("{BUILD}/python", "-c", "x")): 1},
                 "spawns": {}}
        self.assertEqual(check_r27.suite_violations(
            summary, {"test_crash"}, (), rules), [])
        run["argv"][-1] = "y"
        self.assertTrue(check_r27.suite_violations(
            summary, {"test_crash"}, (), rules))

    def test_uncounted_launch(self):
        spawn = [{"tag": "test_sub", "argv": ["/build/python", "-E"],
                  "reason": "isolated", "event": "subprocess.Popen", "pid": 4}]
        summary = clean_summary(spawn=spawn, build="/build")
        self.assertTrue(self.violations(summary, uncounted={"test_sub"}))
        rules = {"runs": {}, "spawns": {
            ("test_sub", "isolated", "subprocess.Popen",
             ("{BUILD}/python", "-E")): 1}}
        self.assertEqual(check_r27.suite_violations(
            summary, (), {"test_sub"}, rules), [])
        spawn[0]["argv"].append("-I")
        self.assertTrue(check_r27.suite_violations(
            summary, (), {"test_sub"}, rules))

    def test_accounting_rule_bounds_and_schema(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp, "rules.json")
            path.write_text('{"runs":[],"spawns":[{"tag":"t",'
                            '"reason":"-E","event":"subprocess.Popen",'
                            '"argv":["{BUILD}/python","-E"],"max":1,'
                            '"why":"isolated child"}]}')
            rules = check_r27.read_accounting_rules(path)
            launch = {"tag": "t", "reason": "-E", "event": "subprocess.Popen",
                      "argv": ["/build/python", "-E"], "pid": 5}
            summary = clean_summary(spawn=[launch], build="/build")
            self.assertEqual(check_r27.suite_violations(
                summary, (), {"t"}, rules), [])
            summary["spawn"].append(dict(launch, pid=6))
            self.assertTrue(check_r27.suite_violations(
                summary, (), {"t"}, rules))
            path.write_text('{"runs":[],"spawns":[{"tag":"t",'
                            '"reason":"-E","event":"subprocess.Popen",'
                            '"argv":"ambiguous", "max":1,"why":"bad"}]}')
            with self.assertRaises(ValueError):
                check_r27.read_accounting_rules(path)

    def test_accounting_normalization_does_not_rewrite_code_payload(self):
        summary = {"build": "/build", "src": "/source",
                   "dump_dir": "/attempt"}
        normalize = check_r27.normalize_accounting_argv
        self.assertEqual(normalize(
            ["/build/python", "-X", "pystats_dir=/attempt", "-c",
             "/build/python"], summary),
            ("{BUILD}/python", "-X", "pystats_dir={DUMPS}", "-c",
             "/build/python"))
        self.assertEqual(normalize(
            ["env", "-i", "/build/python", "-c", "/source/Lib/test"],
            summary),
            ("env", "-i", "/build/python", "-c", "/source/Lib/test"))
        self.assertEqual(normalize(
            ["/build/python", "-c", "pystats_dir=/attempt"], summary),
            ("{BUILD}/python", "-c", "pystats_dir=/attempt"))
        self.assertEqual(normalize(
            ["/bin/sh", "-c", "/build/python"], summary),
            ("/bin/sh", "-c", "/build/python"))
        self.assertEqual(normalize(
            ["/build/python", "-X", "dev", "-X",
             "pystats_dir=/attempt", "-c", "/build/python"], summary),
            ("{BUILD}/python", "-X", "dev", "-X",
             "pystats_dir={DUMPS}", "-c", "/build/python"))

    def test_resets(self):
        ok = clean_summary(resets_by_tag={"test_pystats": 4})
        self.assertEqual(self.violations(ok), [])
        bad = clean_summary(resets_by_tag={"test_os": 1})
        self.assertEqual(len(self.violations(bad)), 1)

    def test_allow_lists_skip_comments(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp, "list.txt")
            path.write_text("# header\ntest_a  # why\n\ntest_b\n")
            self.assertEqual(check_r27.read_list(path), {"test_a", "test_b"})
            self.assertEqual(check_r27.read_list(Path(tmp, "none")), set())

    def test_exit_status(self):
        self.assertEqual(check_r27.finish(["x"], True, "t"), 1)
        self.assertEqual(check_r27.finish(["x"], False, "t"), 0)
        self.assertEqual(check_r27.finish([], True, "t"), 0)


class KernelTests(unittest.TestCase):
    def entries(self):
        return [{"name": name, "raw_counters": {
                    process: {key: 0 for key in check_r27.REQUIRED_BOX_COUNTERS}
                    for process in check_r27.KERNEL_PROCESSES}}
                for name in sorted(check_r27.KERNEL_NAMES)]

    def test_zero_passes(self):
        self.assertEqual(check_r27.kernel_violations(self.entries()), [])

    def test_missing_duplicate_and_empty(self):
        for entries in ([], self.entries()[:-1], self.entries() + self.entries()[:1]):
            with self.subTest(entries=len(entries)):
                self.assertTrue(check_r27.kernel_violations(entries))
        for processes in ({}, {"run": {}}, {"run": {"Other": 1}}):
            entries = self.entries()
            entries[0]["raw_counters"] = processes
            self.assertTrue(check_r27.kernel_violations(entries))

    def test_missing_required_process_and_counter(self):
        entries = self.entries()
        del entries[0]["raw_counters"]["warmup"]
        self.assertTrue(check_r27.kernel_violations(entries))
        entries = self.entries()
        del entries[0]["raw_counters"]["low"]["Boxing by kind (thunk)"]
        self.assertTrue(check_r27.kernel_violations(entries))

    def test_nonzero_in_any_process_fails(self):
        entries = self.entries()
        entries[0]["raw_counters"]["warmup"]["Boxing by kind (thunk)"] = 2
        self.assertTrue(check_r27.kernel_violations(entries))

    def test_run_py_nested_results(self):
        entries = self.entries()
        boxes = {e["name"]: {"raw_counters": e["raw_counters"]} for e in entries}
        self.assertEqual(check_r27.kernel_violations({"results": {"boxes": boxes}}), [])
        boxes[entries[0]["name"]]["raw_counters"]["warmup"]["Boxing by kind (core)"] = 1
        self.assertTrue(check_r27.kernel_violations({"results": {"boxes": boxes}}))

    def test_bad_shape_is_an_error(self):
        with self.assertRaises(SystemExit):
            check_r27.kernel_violations({"x": 1})


class CompletenessTests(unittest.TestCase):
    def test_suite_uses_requested_dump_directory(self):
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp, "attempt")
            args = check_r27.build_parser().parse_args([
                "suite", "--build", tmp, "--dumps", str(directory),
                "-o", str(Path(tmp, "summary.json"))])
            with (patch.object(check_r27, "check_build_flags"),
                  patch.object(check_r27, "run_suite", return_value=(None, 0)) as run,
                  patch.object(check_r27, "collect", return_value=clean_summary()),
                  patch.object(check_r27, "suite_gate", return_value=0)):
                self.assertEqual(check_r27.cmd_suite(args), 0)
            self.assertEqual(run.call_args.args[2], directory)

    def test_suite_rejects_stale_dumps(self):
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            (directory / "old.txt").write_text("old stats")
            with self.assertRaises(SystemExit):
                check_r27.run_suite(directory, 1, directory, [])

    def test_failed_run_and_no_dumps_fail(self):
        for changes in ({"dumps": 0}, {"regrtest_returncode": 2},
                        {"accounting_errors": ["missing owner"]}):
            self.assertTrue(check_r27.suite_violations(clean_summary(**changes)))

    def test_narrow_compat_test_exemptions(self):
        paths = ("Modules/_testclinic.c", "Modules/_testinternalcapi.c",
                 "Modules/_testinternalcapi/interpreter.c", "Modules/_xxtestfuzz/fuzzer.c")
        for path in paths:
            with self.subTest(path=path):
                self.assertTrue(check_r27.is_compat_test_unit(path))
                self.assertIn(check_r27.unit_class(unit(path), SRC)[0],
                              ("test", "compat-test"))
        for path in ("Modules/_testclinic_value.c", "Modules/_decimal/_decimal.c"):
            self.assertFalse(check_r27.is_compat_test_unit(path))

    def test_missing_owner_is_detected(self):
        totals = "".join(f"Boxing by kind ({k}): {int(k == 'core')}\n"
                         f"Box owners dropped ({k}): 0\n" for k in check_r27.KINDS)
        summary, _ = classes(fold(totals), TableResolver({}))
        self.assertTrue(check_r27.suite_violations(summary))
        self.assertIn("core total does not match owners and drops", summary["accounting_errors"])

    def test_manual_inventory_marks_and_duplicates(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "Include").mkdir()
            header = root / "Include/a.h"
            header.write_text("Py_VALUE_COMPAT(PyValue_Foo)\n"
                              "PyAPI_FUNC(int) PyFoo(void);\n"
                              "PyAPI_FUNC(int) PyBar(void);\n"
                              "PyAPI_FUNC(int) PyValue_Foo(void);\n"
                              "PyAPI_FUNC(int) PyValue_Bar(void);\n")
            source = root / "a.c"
            source.write_text("#define Py_VALUE_CORE\n")
            inventory = root / "inventory.toml"
            table = ('r27_complete=true\n[[row]]\nobject="PyFoo"\nvalue="PyValue_Foo"\n'
                     '[[manual]]\nobject="PyBar"\nvalue="PyValue_Bar"\n')
            inventory.write_text(table)
            cdb = root / "cdb.json"
            cdb.write_text(json.dumps([{"directory": tmp, "file": str(source),
                                       "arguments": ["cc", "-c", str(source)]}]))
            out, _ = check_r27.static_violations(cdb, root, inventory)
            self.assertEqual(len(out), 1, out)
            self.assertIn("PyBar lacks Py_VALUE_COMPAT", out[0])
            header.write_text(header.read_text().replace(
                "PyAPI_FUNC(int) PyBar", "Py_VALUE_COMPAT(PyValue_Bar)\nPyAPI_FUNC(int) PyBar"))
            inventory.write_text(table + '[[manual]]\nobject="PyFoo"\nvalue="PyValue_Foo"\n')
            out, _ = check_r27.static_violations(cdb, root, inventory)
            self.assertEqual(out, ["duplicate ownership entry: PyFoo"])

    def test_no_caller_manual_aliases_require_exact_marks(self):
        pairs = (
            ("PyIter_NextItem", "_PyValue_IterNext", "item"),
            ("PyMapping_GetOptionalItemString",
             "_PyValue_MappingGetOptionalItemString", "item"),
            ("_PyMapping_GetOptionalItem2", "_PyValue_GetOptionalItem", "item"),
            ("_PyLong_GCD", "_PyLong_ValueGCD", "number"),
        )
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            include = root / "Include"
            include.mkdir()
            header = include / "api.h"
            declarations = {
                "PyIter_NextItem": (
                    "PyAPI_FUNC(int) PyIter_NextItem(PyObject *, PyObject **);"),
                "PyMapping_GetOptionalItemString": (
                    "PyAPI_FUNC(int) PyMapping_GetOptionalItemString("
                    "PyObject *, const char *, PyObject **);"),
                "_PyMapping_GetOptionalItem2": (
                    "PyAPI_FUNC(PyObject*) _PyMapping_GetOptionalItem2("
                    "PyObject *, PyObject *, int *);"),
                "_PyLong_GCD": (
                    "PyAPI_FUNC(PyObject *) _PyLong_GCD("
                    "PyObject *, PyObject *);"),
            }
            providers = (
                "PyAPI_FUNC(_PyStatusValue) _PyValue_IterNext(PyObject *);",
                "PyAPI_FUNC(_PyStatusValue) "
                "_PyValue_MappingGetOptionalItemString(PyValueBorrow, const char *);",
                "PyAPI_FUNC(_PyStatusValue) _PyValue_GetOptionalItem("
                "PyValueBorrow, PyValueBorrow);",
                "PyAPI_FUNC(PyValue) _PyLong_ValueGCD("
                "PyValueBorrow, PyValueBorrow);",
            )

            def write_header(missing_mark=None):
                lines = []
                for obj, value, _ in pairs:
                    if obj != missing_mark:
                        lines.append(f"Py_VALUE_COMPAT({value})")
                    lines.append(declarations[obj])
                lines.extend(providers)
                header.write_text("\n".join(lines) + "\n")

            write_header()
            source_dir = root / "Modules"
            source_dir.mkdir()
            source = source_dir / "probe.c"
            source.write_text("#define Py_VALUE_CORE\n")
            inventory = root / "inventory.toml"
            rows = ["r27_complete=true"]
            for obj, value, family in pairs:
                rows.extend(("[[manual]]", f'object="{obj}"',
                             f'value="{value}"', f'family="{family}"'))
            inventory.write_text("\n".join(rows) + "\n")
            cdb = root / "cdb.json"
            cdb.write_text(json.dumps([{
                "directory": tmp, "file": str(source),
                "arguments": ["cc", "-c", str(source)],
            }]))

            self.assertEqual(check_r27.static_violations(
                cdb, root, inventory)[0], [])
            for obj, value, _ in pairs:
                with self.subTest(missing_mark=obj):
                    write_header(missing_mark=obj)
                    out, _ = check_r27.static_violations(cdb, root, inventory)
                    self.assertEqual(len(out), 1, out)
                    self.assertIn(f"{obj} lacks Py_VALUE_COMPAT({value})",
                                  out[0])

    def test_static_inventory_and_deferral(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "Include").mkdir()
            (root / "Modules").mkdir()
            header = root / "Include/a.h"
            header.write_text("PyAPI_FUNC(int) PyFoo(void);\n"
                              "PyAPI_FUNC(int) PyValue_Foo(void);\n")
            source = root / "Modules/a.c"
            source.write_text("#define Py_BUILD_CORE_MODULE\n#define Py_VALUE_DEFER_MARKS\n")
            inventory = root / "inventory.toml"
            inventory.write_text('r27_complete=true\n[[row]]\nobject="PyFoo"\nvalue="PyValue_Foo"\n')
            cdb = root / "cdb.json"
            cdb.write_text(json.dumps([{"directory": tmp, "file": str(source),
                                       "arguments": ["cc", "-c", str(source)]}]))
            out, _ = check_r27.static_violations(cdb, root, inventory)
            self.assertTrue(any("lacks Py_VALUE_COMPAT" in x for x in out), out)
            self.assertTrue(any("deferred" in x for x in out), out)
            self.assertTrue(any("not value-native" in x for x in out), out)
            header.write_text("Py_VALUE_COMPAT(PyValue_Foo)\n"
                              "PyAPI_FUNC(int) PyFoo(void);\n"
                              "PyAPI_FUNC(int) PyValue_Foo(void);\n")
            source.write_text("#define Py_BUILD_CORE_MODULE\n#define Py_VALUE_CORE\n")
            out, _ = check_r27.static_violations(cdb, root, inventory)
            self.assertEqual(out, [])
            with patch.object(check_r27.check_value_api, "MARKED_SLOT_FAMILIES", frozenset()):
                out, _ = check_r27.static_violations(cdb, root, inventory)
            self.assertEqual(len(out), 1)
            self.assertIn("object-slot calls", out[0])

    def test_inventory_requires_declared_value_provider_and_valid_sections(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "Include").mkdir()
            header = root / "Include/a.h"
            header.write_text("Py_VALUE_COMPAT(PyValue_Foo)\n"
                              "PyAPI_FUNC(int) PyFoo(void);\n"
                              "PyAPI_FUNC(int) PyValue_Foo(void);\n")
            source = root / "a.c"
            source.write_text("#define Py_VALUE_CORE\n")
            inventory = root / "inventory.toml"
            table = ('r27_complete=true\n[[row]]\n'
                     'object="PyFoo"\nvalue="PyValue_Foo"\n')
            inventory.write_text(table)
            cdb = root / "cdb.json"
            cdb.write_text(json.dumps([{"directory": tmp, "file": str(source),
                                       "arguments": ["cc", "-c", str(source)]}]))
            self.assertEqual(check_r27.static_violations(cdb, root, inventory)[0], [])

            header.write_text(header.read_text().replace(
                "PyAPI_FUNC(int) PyValue_Foo(void);\n", ""))
            out, _ = check_r27.static_violations(cdb, root, inventory)
            self.assertEqual(out, ["PyFoo: value provider PyValue_Foo declaration missing"])
            header.write_text(header.read_text() +
                              "/* PyAPI_FUNC(int) PyValue_Foo(void); */\n")
            out, _ = check_r27.static_violations(cdb, root, inventory)
            self.assertEqual(out, ["PyFoo: value provider PyValue_Foo declaration missing"])

            inventory.write_text('r27_complete=true\n[row]\n'
                                 'object="PyFoo"\nvalue="PyValue_Foo"\n')
            out, _ = check_r27.static_violations(cdb, root, inventory)
            self.assertIn("ownership row section must be an array of tables", out)
            inventory.write_text('r27_complete=true\n[manual]\n'
                                 'object="PyFoo"\nvalue="PyValue_Foo"\n')
            out, _ = check_r27.static_violations(cdb, root, inventory)
            self.assertIn("ownership manual section must be an array of tables", out)
            inventory.write_text(table.replace('object="PyFoo"',
                                               'object="PyFoo;bad"'))
            out, _ = check_r27.static_violations(cdb, root, inventory)
            self.assertTrue(any("invalid ownership row" in item for item in out))

    def test_generated_arithmetic_provider_requires_roster_and_declaration(self):
        with tempfile.TemporaryDirectory() as tmp:
            header = Path(tmp, "pycore_value_ops.h")
            text = ("#define _PyValue_BINARY_DISPATCHERS(X) X(Add)\n"
                    "#define _PyValue_DECLARE_BINARY(NAME) \\\n"
                    "    PyAPI_FUNC(PyValue) _PyValue_##NAME(PyValueBorrow a);\n"
                    "_PyValue_BINARY_DISPATCHERS(_PyValue_DECLARE_BINARY)\n"
                    "#define _PyValue_Add(A) (_PyValue_Add)(A)\n")
            header.write_text(text)
            self.assertTrue(check_r27.value_provider_declared(
                "_PyValue_Add", [(header, text)]))
            without_roster = text.replace("X(Add)", "X(Subtract)")
            self.assertFalse(check_r27.value_provider_declared(
                "_PyValue_Add", [(header, without_roster)]))
            without_declaration = text.replace("PyAPI_FUNC(PyValue)", "")
            self.assertFalse(check_r27.value_provider_declared(
                "_PyValue_Add", [(header, without_declaration)]))
            for commented in (
                text.replace("X(Add)", "/* X(Add) */"),
                text.replace("#define _PyValue_DECLARE_BINARY(NAME)",
                             "/* #define _PyValue_DECLARE_BINARY(NAME)")
                    .replace("PyValueBorrow a);", "PyValueBorrow a); */"),
                text.replace("_PyValue_BINARY_DISPATCHERS(_PyValue_DECLARE_BINARY)\n",
                             "/* _PyValue_BINARY_DISPATCHERS(_PyValue_DECLARE_BINARY) */\n"),
            ):
                code = check_r27.check_value_api.strip_c(commented)
                self.assertFalse(check_r27.value_provider_declared(
                    "_PyValue_Add", [(header, code)]))


class ReportTests(unittest.TestCase):
    def test_report_tables(self):
        dump = ("Box object [0]: /b/python build-id=aa\n"
                "Box owner (thunk, list) 0+0x20 at 0+0x1: 4\n"
                "Box owner (thunk, tuple) ? at 0+0x1: 1\n"
                "Boxing by site (list): 4\nBoxing by kind (thunk): 5\n")
        summary = check_r27.resolve(
            fold(dump), TableResolver({"aa": {0x20: unit("Python/ceval.c")}}), SRC)
        summary = json.loads(json.dumps(summary))
        kernels = [{"name": "k", "raw_counters": {"low": {"Boxing by site (list)": 0}}}]
        text = check_r27.make_report(kernels, summary)
        for heading in ("Boxes by class", "Boxes by kind", "Owner split",
                        "Debt total", "Top debt", "Accounting"):
            self.assertIn(heading, text)
        self.assertIn("## Debt total\n\n5", text)


class DwarfParseTests(unittest.TestCase):
    def test_first_unit_at_offset_zero(self):
        # readelf prints the first unit's offset as "0", not "0x0".
        aranges = (
            "Contents of the .debug_aranges section:\n\n"
            "  Length:                   44\n"
            "  Version:                  2\n"
            "  Offset into .debug_info:  0\n"
            "  Pointer Size:             8\n\n"
            "    Address            Length\n"
            "    0000000000004bc0 0000000000006406\n"
            "    0000000000000000 0000000000000000\n")
        self.assertEqual(check_r27.parse_aranges(aranges),
                         [(0, 0x4bc0, 0x4bc0 + 0x6406)])
        info = ("  Compilation Unit @ offset 0:\n"
                " <0><c>: Abbrev Number: 1 (DW_TAG_compile_unit)\n"
                "    <12>   DW_AT_name        : (indirect line string, "
                "offset: 0): ../g/Modules/_struct.c\n"
                "    <16>   DW_AT_comp_dir    : (indirect line string, "
                "offset: 0x21): /b\n")
        self.assertEqual(check_r27.parse_cu_names(info),
                         {0: ["../g/Modules/_struct.c", "/b"]})


@unittest.skipUnless(os.environ.get("R27_STATS_BUILD"),
                     "R27_STATS_BUILD names no stats build")
class RealDumpTests(unittest.TestCase):
    def test_number_add_resolves_to_abstract(self):
        check_r27.stats_self_test(os.environ["R27_STATS_BUILD"], SRC)


if __name__ == "__main__":
    unittest.main()
