"""Tools/build/check_r27.py: folding dumps, classing owners and the gates.

The resolver is replaced by a table, so no build is needed. The real-dump
test runs when R27_STATS_BUILD names a --enable-pystats build.
"""

import json
import os
import tempfile
import unittest
from pathlib import Path

from test.test_tools import imports_under_tool, skip_if_missing

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
            (d / "a.txt").write_text("Stats resets: 1\nStats process: pid=1 tag=t\n")
            (d / "a.run").write_text("pid: 1\n")
            (d / "b.tmp").write_text("Stats resets: 9\n")
            folded = check_r27.Folded()
            folded.fold_dir(d, delete=True)
            folded.read_markers(d)
            self.assertEqual((folded.dumps, folded.resets), (1, 1))
            self.assertEqual(folded.leftover_run, [])
            self.assertEqual([p.name for p in d.iterdir()], ["b.tmp"])

    def test_markers(self):
        with tempfile.TemporaryDirectory() as tmp:
            d = Path(tmp)
            (d / "x.run").write_text("pid: 4\nppid: 1\ntag: test_foo\nargv: python -c x\n")
            (d / "y.run").write_text("pid: 5\ntag: done\n")
            (d / "y.txt").write_text("")
            (d / "z.spawn").write_text("test_bar\tpython -m x\n")
            folded = check_r27.Folded()
            folded.read_markers(d)
            self.assertEqual([(r["id"], r["tag"]) for r in folded.leftover_run],
                             [("x", "test_foo")])
            self.assertEqual(folded.spawn,
                             [{"tag": "test_bar", "argv": "python -m x"}])


def clean_summary(**changes):
    summary = {"rows": [], "drops": {"thunk": 0}, "leftover_run": [],
               "spawn": [], "resets_by_tag": {}, "bad_dumps": []}
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
        run = {"id": "r", "tag": "test_crash", "pid": "3"}
        self.assertEqual(len(self.violations(clean_summary(leftover_run=[run]))), 1)
        self.assertEqual(self.violations(clean_summary(leftover_run=[run]),
                                         crashes={"test_crash"}), [])

    def test_uncounted_launch(self):
        spawn = [{"tag": "test_sub", "argv": "x"}]
        self.assertEqual(len(self.violations(clean_summary(spawn=spawn))), 1)
        self.assertEqual(self.violations(clean_summary(spawn=spawn),
                                         uncounted={"test_sub"}), [])

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
    def entry(self, **counters):
        return {"name": "k", "raw_counters": {
            "warmup": {"Boxing by site (list)": 0, "Other": 5}, "low": counters}}

    def test_zero_passes(self):
        data = [self.entry(**{"Object inline int boxes": 0})]
        self.assertEqual(check_r27.kernel_violations(data), [])

    def test_nonzero_in_any_process_fails(self):
        data = {"results": [self.entry(**{"Box owner (thunk, list) 0+0x1 at -": 2})]}
        out = check_r27.kernel_violations(data)
        self.assertEqual(len(out), 1)
        self.assertIn("kernel k, process low", out[0])

    def test_bad_shape_is_an_error(self):
        with self.assertRaises(SystemExit):
            check_r27.kernel_violations({"x": 1})
        with self.assertRaises(SystemExit):
            check_r27.kernel_violations([{"name": "k"}])


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
