"""The R27 gate: attribute every box of a stats build to the code it serves.

See pyjit/design/proposals/value-api-g.md, sections 1.4, 1.8 and 3.

    check_r27.py static  --cdb DB [--ownership TABLE] [--gate]
    check_r27.py collect DIR -o S.json [--gate]
    check_r27.py suite   --build DIR [-j N] -o S.json [--gate] [-- regrtest args]
    check_r27.py kernels R.json [--gate]
    check_r27.py report  [--kernels R.json] [--suite S.json]
    check_r27.py --self-test [--build DIR]

Standard library only, plus readelf and addr2line (llvm-dwarfdump is the
fallback for objects without .debug_aranges).

Dump directory layout (written by Python/pystats.c): <rand>.txt (a finished
process), <rand>.run (a process that started and has not finished), <rand>.spawn
(one "<tag>TAB<argv>" line per uncounted launch), objects/<build-id> (copies of
objects that no build product explains). Anything else, including .tmp, is
ignored.

Classification. A CORE box (owner "-") is classed by the box primitive's call
site (the "at" address); every other box by its owner. An address is
IDX+0xOFF; IDX is mapped to a GNU build-id per dump file, so folded entries
are keyed by build-id. The resolver maps (build-id, address) to a compile
unit; see CLASSES and classify_ref for the rules.

Rule for a compile unit under $SRC when no compile database is given: Objects/,
Python/, Parser/ and Include/ are core; other units are core-pending. With
--cdb, the entry whose file is the unit is passed to
check_value_api.unit_kind ('core' or 'adapter' is core; None is core-pending).
Modules/_testinternalcapi* is test and COMPAT_TEST_UNITS is compat-test
whatever the database says. An object without readable DWARF is classed by
its file name against COMPAT_TEST_UNITS and Modules/_testinternalcapi*, else
unresolved (a build product without DWARF cannot be shown to be core).
"""

import argparse
import bisect
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
import tomllib
from pathlib import Path

import check_value_api
from check_value_api import is_compat_test_unit, unit_kind

HERE = Path(__file__).resolve().parent
DEFAULT_SRC = HERE.parents[1]

CLASSES = ("core", "core-pending", "test", "compat-test", "external",
           "foreign", "unresolved")
DEBT = frozenset({"core", "core-pending", "unresolved"})
KINDS = ("core", "thunk", "slot adapter", "call adapter")
SITES = ("unattributed", "number", "compare", "repr", "sequence", "iteration",
         "attribute", "call", "list", "tuple", "dict", "set", "cell/structseq",
         "call adapter", "parser", "interpreter", "field", "frame", "compiler",
         "test")
KERNEL_NAMES = frozenset("""richards go nbody float spectral_norm raytrace
    deltablue chaos nqueens fannkuch generators comprehensions coroutines
    unpack_sequence deepcopy deepcopy_reduce deepcopy_memo json_dumps json_loads
    pickle unpickle regex_compile logging_format logging_silent logging_simple
    xml_etree_generate xml_etree_iterparse xml_etree_parse xml_etree_process""".split())
FOLD_PERIOD = 30  # seconds between folds of finished dumps in `suite`

# The shape `kernels` accepts: R.json is a list of entries (or a dict with
# such a list under "results" or "entries"). An entry is a dict named by one
# of KERNEL_NAME_KEYS whose processes are a dict {process: {counter: number}}
# under one of KERNEL_PROCESS_KEYS (run.py keeps "raw_counters": {"warmup",
# "low", "high"}). Every counter whose name starts with a
# BOX_COUNTER_PREFIXES item must be 0, absolute and startup included.
KERNEL_NAME_KEYS = ("name", "kernel", "bench", "benchmark")
KERNEL_PROCESS_KEYS = ("raw_counters", "processes")
KERNEL_LIST_KEYS = ("results", "entries")
KERNEL_PROCESSES = frozenset({"warmup", "low", "high"})
REQUIRED_BOX_COUNTERS = frozenset(
    [f"Boxing by kind ({kind})" for kind in KINDS] +
    ["Object inline int boxes", "Object inline float boxes", "Object inline array boxes"])
BOX_COUNTER_PREFIXES = (
    "Object inline int boxes", "Object inline float boxes",
    "Object inline array boxes", "Object boxes (", "Object box site[",
    "Object box cached", "Boxing by site (", "Boxing by kind (", "Box owner (")

# ---------------------------------------------------------------------------
# Dumps

OBJECT_RE = re.compile(r"^Box object \[(\d+)\]: (.+) build-id=([0-9a-fA-F]+)\s*$")
OWNER_RE = re.compile(
    r"^Box owner \((?P<kind>[^,()]+), (?P<site>[^()]+)\) "
    r"(?P<owner>-|foreign|\?|\d+\+0x[0-9a-fA-F]+) "
    r"at (?P<at>-|\?|\d+\+0x[0-9a-fA-F]+): (?P<count>\d+)\s*$")
ADDR_RE = re.compile(r"^(\d+)\+0x([0-9a-fA-F]+)$")
PROCESS_RE = re.compile(r"^Stats process: pid=(\d+) tag=(.*?)\s*$")
DROPPED_RE = re.compile(r"^Box owners dropped \(([^()]+)\): (\d+)\s*$")
RESETS_RE = re.compile(r"^Stats resets: (\d+)\s*$")
CACHED_RE = re.compile(r"^Object box cached: (\d+)\s*$")
BY_SITE_RE = re.compile(r"^Boxing by site \(([^()]+)\): (\d+)\s*$")
BY_KIND_RE = re.compile(r"^Boxing by kind \(([^()]+)\): (\d+)\s*$")


def make_ref(bid, off):
    return f"{bid}+0x{off:x}"


def split_ref(ref):
    """(build-id, address) of a folded address, or None for - ? foreign."""
    bid, sep, off = ref.partition("+0x")
    return (bid, int(off, 16)) if sep else None


class Folded:
    """The sum of many dumps, with addresses keyed by build-id."""

    def __init__(self):
        self.dumps = 0
        self.bad_dumps = []
        self.owners = {}  # (kind, site, owner, at) -> count
        self.paths = {}  # build-id -> set of paths
        self.drops = {}
        self.resets = 0
        self.resets_by_tag = {}
        self.box_cached = 0
        self.kind_totals = {}
        self.site_totals = {}
        self.leftover_run = []
        self.spawn = []
        self.accounting_errors = []

    def add_text(self, text):
        objects = {}  # dump-local index -> build-id
        owners = []
        tag = ""
        resets = 0
        totals, drops = {}, {}
        for line in text.splitlines():
            if m := OBJECT_RE.match(line):
                bid = m[3].lower()
                objects[int(m[1])] = bid
                self.paths.setdefault(bid, set()).add(m[2])
            elif m := OWNER_RE.match(line):
                owners.append(m)
            elif m := PROCESS_RE.match(line):
                tag = m[2]
            elif m := DROPPED_RE.match(line):
                add(self.drops, m[1], int(m[2]))
                add(drops, m[1], int(m[2]))
            elif m := RESETS_RE.match(line):
                resets += int(m[1])
            elif m := CACHED_RE.match(line):
                self.box_cached += int(m[1])
            elif m := BY_SITE_RE.match(line):
                add(self.site_totals, m[1], int(m[2]))
            elif m := BY_KIND_RE.match(line):
                add(self.kind_totals, m[1], int(m[2]))
                add(totals, m[1], int(m[2]))
            elif line.startswith("Box owner ("):
                self.accounting_errors.append("malformed owner row")
        counts = {}
        for m in owners:
            add(counts, m["kind"], int(m["count"]))
            if (m["kind"] not in KINDS or
                (m["kind"] == "core" and (m["owner"] != "-" or m["at"] in ("-", "?"))) or
                (m["kind"] != "core" and m["owner"] == "-")):
                self.accounting_errors.append("invalid owner attribution")
        for kind in KINDS:
            if kind not in totals or kind not in drops:
                self.accounting_errors.append(f"missing {kind} accounting")
            elif totals[kind] != counts.get(kind, 0) + drops[kind]:
                self.accounting_errors.append(f"{kind} total does not match owners and drops")
        for m in owners:
            key = (m["kind"], m["site"],
                   map_address(m["owner"], objects),
                   map_address(m["at"], objects))
            add(self.owners, key, int(m["count"]))
        self.resets += resets
        if resets:
            add(self.resets_by_tag, tag, resets)
        self.dumps += 1

    def fold_dir(self, directory, delete=False):
        """Fold the finished dumps; if delete, remove them and their .run."""
        for path in sorted(Path(directory).glob("*.txt")):
            try:
                self.add_text(path.read_text(errors="replace"))
            except OSError as exc:
                self.bad_dumps.append(f"{path.name}: {exc}")
                continue
            if delete:
                path.unlink(missing_ok=True)
                path.with_suffix(".run").unlink(missing_ok=True)

    def read_markers(self, directory):
        """List .run files that no .txt replaced, and the .spawn lines."""
        directory = Path(directory)
        self.leftover_run = []
        for path in sorted(directory.glob("*.run")):
            if path.with_suffix(".txt").exists():
                continue
            info = {"id": path.stem}
            for line in path.read_text(errors="replace").splitlines():
                key, sep, value = line.partition(": ")
                if sep:
                    info[key] = value
            self.leftover_run.append(info)
        self.spawn = []
        for path in sorted(directory.glob("*.spawn")):
            for line in path.read_text(errors="replace").splitlines():
                if not line.strip():
                    continue
                if line.startswith("{"):
                    # The regrtest worker's audit hook: one JSON object per
                    # launch of a build product. Only the launches that will
                    # not count are listed; a counted child leaves its own
                    # dump.
                    try:
                        launch = json.loads(line)
                    except ValueError:
                        continue
                    if launch.get("counts"):
                        continue
                    self.spawn.append({
                        "tag": launch.get("tag", ""),
                        "argv": " ".join(launch.get("argv", [])),
                        "reason": launch.get("reason", "")})
                    continue
                tag, _, argv = line.partition("\t")
                self.spawn.append({"tag": tag, "argv": argv})


def add(table, key, n):
    table[key] = table.get(key, 0) + n


def map_address(text, objects):
    if text in ("-", "foreign", "?"):
        return text
    m = ADDR_RE.match(text)
    bid = objects.get(int(m[1]))
    return "?" if bid is None else make_ref(bid, int(m[2], 16))


# ---------------------------------------------------------------------------
# Resolvers

class Resolver:
    """What classification needs from the objects. Tests subclass this."""

    def lookup(self, bid, addr):
        """Absolute path of the compile unit containing addr, or None."""
        return None

    def in_tree(self, bid):
        """True if some CU of the object lies under $SRC, False if none
        does, None if the object is missing or has no readable DWARF."""
        return None

    def path(self, bid):
        """A file for the object (for no-DWARF classification), or None."""
        return None

    def function(self, bid, addr):
        return "?"


def run_tool(args):
    try:
        proc = subprocess.run(args, capture_output=True, text=True,
                              errors="replace")
    except OSError:
        return None
    return proc.stdout if proc.returncode == 0 else None


def read_build_id(path):
    out = run_tool(["readelf", "-n", str(path)])
    m = re.search(r"Build ID:\s*([0-9a-fA-F]+)", out or "")
    return m[1].lower() if m else None


def dwarf_value(line):
    value = line.split(":", 1)[1] if ":" in line else ""
    value = value.strip()
    return re.sub(r"^\((?:indirect (?:line )?string|strp|string)[^)]*\):\s*",
                  "", value)


def parse_aranges(text):
    ranges = []  # (cu offset, start, end)
    cu = None
    for line in text.splitlines():
        if m := re.match(r"\s*Offset into \.debug_info:\s+(?:0x)?([0-9a-fA-F]+)", line):
            cu = int(m[1], 16)
        elif cu is not None and (
                m := re.match(r"\s+([0-9a-f]{8,16})\s+([0-9a-f]{8,16})\s*$", line)):
            start, length = int(m[1], 16), int(m[2], 16)
            if length:
                ranges.append((cu, start, start + length))
    return ranges


def parse_cu_names(text):
    cus = {}  # offset -> [name, comp_dir]
    cur = None
    for line in text.splitlines():
        if m := re.match(r"\s*Compilation Unit @ offset (?:0x)?([0-9a-fA-F]+):", line):
            cur = cus[int(m[1], 16)] = [None, None]
        elif cur is not None and "DW_AT_name" in line and cur[0] is None:
            cur[0] = dwarf_value(line)
        elif cur is not None and "DW_AT_comp_dir" in line and cur[1] is None:
            cur[1] = dwarf_value(line)
    return cus


def parse_llvm_dwarfdump(text):
    """[(start, end, path)] from `llvm-dwarfdump --debug-info --recurse-depth=0`."""
    out = []
    for block in re.split(r"(?m)^(?=0x[0-9a-f]+: DW_TAG_compile_unit)", text):
        if not block.startswith("0x"):
            continue
        name = re.search(r'DW_AT_name\s+\("([^"]*)"\)', block)
        comp = re.search(r'DW_AT_comp_dir\s+\("([^"]*)"\)', block)
        if not name:
            continue
        path = os.path.normpath(os.path.join(comp[1] if comp else "", name[1]))
        spans = re.findall(r"\[0x([0-9a-f]+), 0x([0-9a-f]+)\)", block)
        if not spans:
            low = re.search(r"DW_AT_low_pc\s+\(0x([0-9a-f]+)\)", block)
            high = re.search(r"DW_AT_high_pc\s+\(0x([0-9a-f]+)\)", block)
            if low and high:
                spans = [(low[1], high[1])]
        out.extend((int(a, 16), int(b, 16), path) for a, b in spans)
    return out


def read_cu_table(path):
    """[(start, end, cu path)] sorted by start, or None without DWARF."""
    aranges = run_tool(["readelf", "--debug-dump=aranges", str(path)])
    table = []
    if aranges and (ranges := parse_aranges(aranges)):
        info = run_tool(["readelf", "--debug-dump=info", "--dwarf-depth=1",
                         str(path)])
        names = parse_cu_names(info or "")
        for cu, start, end in ranges:
            name, comp = names.get(cu, (None, None))
            if name:
                table.append((start, end,
                              os.path.normpath(os.path.join(comp or "", name))))
    else:
        dump = run_tool(["llvm-dwarfdump", "--debug-info", "--recurse-depth=0",
                         str(path)])
        table = parse_llvm_dwarfdump(dump) if dump else []
    return sorted(table) or None


def under(path, root):
    try:
        return Path(os.path.realpath(path)).is_relative_to(root)
    except ValueError:
        return False


class ElfResolver(Resolver):
    """Find objects by build-id and read their CU tables with readelf.

    Order: build products of `build`, then <dump_dir>/objects/<build-id>, then
    the dumped path if it still has that build-id. Parsed tables are cached
    per build-id as JSON in cache_dir.
    """

    def __init__(self, src, build=None, dump_dir=None, paths=None,
                 cache_dir=None):
        self.src = Path(os.path.realpath(src))
        self.build = Path(build) if build else None
        self.dump_dir = Path(dump_dir) if dump_dir else None
        self.paths = paths or {}
        self.cache_dir = Path(cache_dir) if cache_dir else None
        self._products = None
        self._located = {}
        self._tables = {}

    def products(self):
        if self._products is None:
            self._products = {}
            for path in self.build_products():
                bid = read_build_id(path)
                if bid:
                    self._products.setdefault(bid, str(path))
        return self._products

    def build_products(self):
        if not self.build:
            return []
        found = [self.build / "python", self.build / "Programs" / "_testembed"]
        libs = []
        marker = self.build / "pybuilddir.txt"
        if marker.is_file():
            libs.append(self.build / marker.read_text().strip())
        libs.extend(self.build.glob("build/lib*"))
        for lib in libs:
            found.extend(sorted(lib.glob("*.so")))
        return [p for p in found if p.is_file()]

    def path(self, bid):
        if bid not in self._located:
            self._located[bid] = self._locate(bid)
        return self._located[bid]

    def _locate(self, bid):
        if bid in self.products():
            return self.products()[bid]
        if self.dump_dir and (self.dump_dir / "objects" / bid).is_file():
            return str(self.dump_dir / "objects" / bid)
        for path in sorted(self.paths.get(bid, ())):
            if os.path.isfile(path) and read_build_id(path) == bid:
                return path
        return None

    def table(self, bid):
        if bid in self._tables:
            return self._tables[bid]
        cache = self.cache_dir / f"{bid}.json" if self.cache_dir else None
        table = None
        if cache and cache.is_file():
            try:
                table = [tuple(row) for row in json.loads(cache.read_text())]
            except (ValueError, OSError):
                table = None
        if table is None:
            path = self.path(bid)
            table = read_cu_table(path) if path else None
            if table is not None and cache:
                cache.parent.mkdir(parents=True, exist_ok=True)
                cache.write_text(json.dumps(table))
        self._tables[bid] = table
        return table

    def lookup(self, bid, addr):
        table = self.table(bid)
        if not table:
            return None
        i = bisect.bisect_right(table, (addr, float("inf"), "")) - 1
        if i >= 0 and table[i][0] <= addr < table[i][1]:
            return table[i][2]
        return None

    def in_tree(self, bid):
        table = self.table(bid)
        if not table:
            return None
        return any(under(cu, self.src) for _, _, cu in table)

    def function(self, bid, addr):
        path = self.path(bid)
        out = run_tool(["addr2line", "-f", "-C", "-e", path, hex(addr)]) if path else None
        return out.splitlines()[0] if out else "?"


# ---------------------------------------------------------------------------
# Classification

def load_cdb(path):
    """{normalized source path: compile database entry}."""
    entries = json.loads(Path(path).read_text())
    return {os.path.realpath(os.path.join(e["directory"], e["file"])): e
            for e in entries}


def unit_class(cu, src, cdb=None):
    """(class, source path relative to src) of a compile unit under src."""
    real = os.path.realpath(cu)
    rel = os.path.relpath(real, src).replace(os.sep, "/")
    if rel.startswith("Modules/_testinternalcapi"):
        return "test", rel
    if is_compat_test_unit(rel):
        return "compat-test", rel
    if cdb is None:
        core = rel.split("/", 1)[0] in ("Objects", "Python", "Parser", "Include")
    else:
        entry = cdb.get(real)
        core = entry is not None and unit_kind(entry) in ("core", "adapter")
    return ("core" if core else "core-pending"), rel


def class_by_name(path):
    """Class of an object without DWARF, from its file name."""
    stem = os.path.basename(path).split(".")[0]
    if stem.startswith("_testinternalcapi"):
        return "test"
    for candidate in (f"Modules/{stem}.c", f"Modules/_ctypes/{stem}.c"):
        if is_compat_test_unit(candidate):
            return "compat-test"
    return "unresolved"


def classify_ref(ref, resolver, src, cdb=None, strays=None):
    """(class, unit) of a folded address."""
    if ref == "foreign":
        return "foreign", "foreign"
    parsed = split_ref(ref)
    if parsed is None:
        return "unresolved", "?"
    bid, addr = parsed
    tree = resolver.in_tree(bid)
    if tree is None:
        path = resolver.path(bid)
        return (class_by_name(path), path) if path else ("unresolved", bid)
    cu = resolver.lookup(bid, addr)
    if not tree:
        return "external", cu or bid
    if cu is None:
        return "unresolved", bid
    if not under(cu, src):
        if strays is not None:
            strays.add(cu)
        return "unresolved", cu
    return unit_class(cu, src, cdb)


def resolve(folded, resolver, src, cdb=None):
    """The JSON-able summary of a Folded."""
    src = Path(os.path.realpath(src))
    strays = set()
    rows = []
    for (kind, site, owner, at), count in sorted(folded.owners.items()):
        ref, via = (at, "site") if owner == "-" else (owner, "owner")
        cls, unit = classify_ref(ref, resolver, src, cdb, strays)
        parsed = split_ref(ref)
        rows.append({"kind": kind, "site": site, "via": via, "class": cls,
                     "unit": unit, "bid": parsed[0] if parsed else None,
                     "addr": parsed[1] if parsed else None, "count": count})
    objects = {bid: {"paths": sorted(paths), "file": resolver.path(bid)}
               for bid, paths in sorted(folded.paths.items())}
    return {
        "version": 1, "src": str(src), "dumps": folded.dumps,
        "bad_dumps": folded.bad_dumps, "leftover_run": folded.leftover_run,
        "spawn": folded.spawn, "resets": folded.resets,
        "resets_by_tag": folded.resets_by_tag, "drops": folded.drops,
        "box_cached": folded.box_cached, "kind_totals": folded.kind_totals,
        "site_totals": folded.site_totals, "rows": rows, "objects": objects,
        "stray_cus": sorted(strays), "accounting_errors": folded.accounting_errors,
    }


def debt_total(summary):
    return sum(r["count"] for r in summary["rows"] if r["class"] in DEBT)


# ---------------------------------------------------------------------------
# Gates

def read_list(path):
    path = Path(path)
    if not path.is_file():
        return set()
    lines = (line.split("#", 1)[0].strip() for line in path.read_text().splitlines())
    return {line for line in lines if line}


def suite_violations(summary, crashes=(), uncounted=()):
    out = []
    if not summary.get("dumps"):
        out.append("no stats dumps")
    if "regrtest_returncode" in summary and summary["regrtest_returncode"] != 0:
        out.append(f"regrtest exited {summary['regrtest_returncode']}")
    out.extend(summary.get("accounting_errors", ["missing accounting evidence"]))
    if (n := debt_total(summary)):
        out.append(f"{n} boxes attributed to core, core-pending or unresolved")
    for kind, n in sorted(summary["drops"].items()):
        if n:
            out.append(f"{n} owner-table drops ({kind})")
    for run in summary["leftover_run"]:
        tag = run.get("tag", "")
        if tag not in crashes:
            out.append(f"leftover .run {run['id']} (tag {tag!r}, "
                       f"pid {run.get('pid', '?')}) not in the crash list")
    # A launch without a tag (the pty REPL tests) is listed as <untagged>.
    launched = {s["tag"] or "<untagged>" for s in summary["spawn"]}
    for tag in sorted(launched - set(uncounted)):
        out.append(f"uncounted launch by tag {tag!r} not in the uncounted list")
    for tag, n in sorted(summary["resets_by_tag"].items()):
        # test_sys clears the stats on purpose (sys._stats_clear()).
        if tag not in ("test_pystats", "test_sys"):
            out.append(f"{n} stats resets in tag {tag!r}")
    for bad in summary["bad_dumps"]:
        out.append(f"unreadable dump {bad}")
    return out


def kernel_entries(data):
    if (isinstance(data, dict) and isinstance(data.get("results"), dict)
            and isinstance(data["results"].get("boxes"), dict)):
        return [{**entry, "name": name}
                for name, entry in data["results"]["boxes"].items()]
    if isinstance(data, dict):
        for key in KERNEL_LIST_KEYS:
            if isinstance(data.get(key), list):
                data = data[key]
                break
    if not isinstance(data, list):
        raise SystemExit("kernels: expected a list of entries (or a dict "
                         f"with one under {KERNEL_LIST_KEYS})")
    return data


def kernel_processes(entry, index):
    """(name, {process: {counter: number}}) of an entry."""
    if not isinstance(entry, dict):
        raise SystemExit(f"kernels: entry {index} is not an object")
    name = next((str(entry[k]) for k in KERNEL_NAME_KEYS if k in entry),
                f"#{index}")
    for key in KERNEL_PROCESS_KEYS:
        if isinstance(entry.get(key), dict):
            return name, entry[key]
    raise SystemExit(f"kernels: entry {name} has none of {KERNEL_PROCESS_KEYS}")


def box_counters(counters):
    return {k: v for k, v in counters.items()
            if k.startswith(BOX_COUNTER_PREFIXES) and isinstance(v, (int, float))}


def kernel_violations(data):
    out = []
    names = set()
    for i, entry in enumerate(kernel_entries(data)):
        name, processes = kernel_processes(entry, i)
        if name in names:
            out.append(f"duplicate kernel {name}")
        names.add(name)
        for missing in sorted(KERNEL_PROCESSES - processes.keys()):
            out.append(f"kernel {name}: missing process {missing}")
        for process, counters in processes.items():
            if not isinstance(counters, dict) or not box_counters(counters):
                out.append(f"kernel {name}, process {process}: no box counters")
                continue
            missing = REQUIRED_BOX_COUNTERS - counters.keys()
            for key in sorted(missing):
                out.append(f"kernel {name}, process {process}: missing {key}")
            for key in REQUIRED_BOX_COUNTERS & counters.keys():
                if not isinstance(counters[key], (int, float)) or isinstance(counters[key], bool):
                    out.append(f"kernel {name}, process {process}: invalid {key}")
            for key, value in sorted(box_counters(counters).items()):
                if value:
                    out.append(f"kernel {name}, process {process}: {key} = {value}")
    for name in sorted(KERNEL_NAMES - names):
        out.append(f"missing kernel {name}")
    for name in sorted(names - KERNEL_NAMES):
        out.append(f"unexpected kernel {name}")
    return out


def static_violations(cdb_path, src, ownership=None):
    """Check compiled unit markings and declarations in the ownership table."""
    src = Path(src).resolve()
    out = []
    entries = json.loads(Path(cdb_path).read_text())
    if not entries:
        out.append("empty compile database")
    for entry in entries:
        path = Path(entry["directory"], entry["file"]).resolve()
        rel = os.path.relpath(path, src).replace(os.sep, "/")
        if is_compat_test_unit(rel):
            continue
        names = check_value_api.defines(check_value_api.entry_arguments(entry))
        try:
            text = path.read_text(errors="replace")
        except OSError:
            out.append(f"{rel}: cannot read compiled source")
            continue
        names |= set(check_value_api.SOURCE_DEFINE.findall(text))
        if (check_value_api.DEFER_LINE.search(text) or "Py_VALUE_DEFER_MARKS" in names):
            out.append(f"{rel}: deferred compatibility marks")
        if (rel.startswith("Modules/") and
            any(n.startswith("Py_BUILD_CORE") for n in names) and
            unit_kind(entry) not in ("core", "adapter")):
            out.append(f"{rel}: has Py_BUILD_CORE_* but is not value-native")
    table = Path(ownership) if ownership else src / "Tools/build/value_ownership.toml"
    try:
        inventory = tomllib.loads(table.read_text())
        rows = inventory.get("row", []) + inventory.get("manual", [])
        if inventory.get("r27_complete") is not True:
            out.append("ownership inventory is not reviewed as complete for R27")
    except (OSError, ValueError, KeyError) as exc:
        out.append(f"ownership inventory unreadable: {exc}")
        return out, []
    if not rows:
        out.append("ownership inventory is empty")
    if not check_value_api.MARKED_SLOT_FAMILIES:
        out.append("direct object-slot calls are not enforced by value API lint")
    headers = [(p, p.read_text(errors="replace")) for p in (src / "Include").rglob("*.h")]
    if not headers:
        out.append("no headers found for ownership inventory")
    seen = set()
    for row in rows:
        if not isinstance(row, dict):
            out.append(f"invalid ownership row: {row!r}")
            continue
        obj, value = row.get("object"), row.get("value")
        if not isinstance(obj, str) or not isinstance(value, str):
            out.append(f"invalid ownership row: {row!r}")
            continue
        if obj in seen:
            out.append(f"duplicate ownership entry: {obj}")
        seen.add(obj)
        pattern = re.compile(
            rf"^([ \t]*)((?:(?:Py_VALUE_ENTRY|Py_VALUE_COMPAT\(\w+\))[ \t]+)?"
            rf"(?:Py_DEPRECATED\([^)]*\)\s*)?(?:PyAPI_FUNC\([^)]*\)|extern\b[^;(]*?)"
            rf"\s*\(?{re.escape(obj)}\)?\s*\()", re.M)
        matches = [(p, text, m) for p, text in headers for m in pattern.finditer(text)]
        if not matches:
            out.append(f"{obj}: inventory declaration missing")
        for path, text, m in matches:
            mark = f"Py_VALUE_COMPAT({value})"
            prev = text[:m.start()].splitlines()[-1:] or [""]
            if mark not in m.group(2) and prev[0].strip() != mark:
                out.append(f"{path.relative_to(src)}: {obj} lacks {mark}")
    return out, []


# ---------------------------------------------------------------------------
# Report

def table(header, rows):
    lines = ["| " + " | ".join(header) + " |",
             "|" + "|".join("---" for _ in header) + "|"]
    lines += ["| " + " | ".join(str(c) for c in row) + " |" for row in rows]
    return "\n".join(lines)


def kernel_totals(data):
    """({site: n}, {kind: n}, {kernel: total boxes}) summed over processes."""
    sites, kinds, per_kernel = {}, {}, {}
    for i, entry in enumerate(kernel_entries(data)):
        name, processes = kernel_processes(entry, i)
        total = 0
        for counters in processes.values():
            for key, value in box_counters(counters).items():
                if key.startswith("Boxing by site ("):
                    add(sites, key[len("Boxing by site ("):-1], value)
                    total += value
                elif key.startswith("Boxing by kind ("):
                    add(kinds, key[len("Boxing by kind ("):-1], value)
        per_kernel[name] = total
    return sites, kinds, per_kernel


def top_debt(summary, limit=20):
    groups = {}
    for r in summary["rows"]:
        if r["class"] in DEBT:
            key = (r["bid"], r["addr"], r["unit"], r["class"], r["kind"])
            add(groups, key, r["count"])
    top = sorted(groups.items(), key=lambda kv: -kv[1])[:limit]
    rows = []
    for (bid, addr, unit, cls, kind), n in top:
        func = "?"
        if bid and addr is not None:
            obj = summary["objects"].get(bid, {}).get("file")
            out = run_tool(["addr2line", "-f", "-C", "-e", obj, hex(addr)]) if obj else None
            func = out.splitlines()[0] if out else "?"
        rows.append((func, unit, cls, kind, n))
    return rows


def make_report(kernels=None, suite=None):
    out = []
    ksites = kkinds = None
    if kernels is not None:
        ksites, kkinds, per_kernel = kernel_totals(kernels)
    cols = (["kernels"] if kernels is not None else []) \
        + (["suite"] if suite else [])
    out.append("## Boxes by class\n")
    out.append(table(["site"] + cols, [
        [s] + ([ksites.get(s, 0)] if kernels is not None else [])
        + ([suite["site_totals"].get(s, 0)] if suite else [])
        for s in SITES]))
    out.append("\n## Boxes by kind\n")
    out.append(table(["kind"] + cols, [
        [k] + ([kkinds.get(k, 0)] if kernels is not None else [])
        + ([suite["kind_totals"].get(k, 0)] if suite else [])
        for k in KINDS]))
    if kernels is not None:
        out.append("\n## Kernel boxes (all processes)\n")
        out.append(table(["kernel", "boxes"], sorted(per_kernel.items())))
    if suite:
        split = {}
        for r in suite["rows"]:
            add(split, (r["kind"], r["class"]), r["count"])
        out.append("\n## Owner split\n")
        out.append(table(["kind"] + list(CLASSES), [
            [k] + [split.get((k, c), 0) for c in CLASSES] for k in KINDS]))
        out.append(f"\n## Debt total\n\n{debt_total(suite)}")
        out.append("\n## Top debt\n")
        out.append(table(["function", "unit", "class", "kind", "count"],
                         top_debt(suite)))
        if suite["stray_cus"]:
            out.append("\nStray compile units (in-tree objects, outside the "
                       "source tree): " + ", ".join(suite["stray_cus"]))
        spawn = {}
        for s in suite["spawn"]:
            add(spawn, s["tag"], 1)
        total = sum(suite["site_totals"].values())
        cached = suite["box_cached"]
        out.append("\n## Accounting\n")
        out.append(table(["item", "value"], [
            ("dumps folded", suite["dumps"]),
            ("leftover .run", len(suite["leftover_run"])),
            ("uncounted launches by tag",
             ", ".join(f"{t}: {n}" for t, n in sorted(spawn.items())) or "none"),
            ("resets", suite["resets"]),
            ("drops", sum(suite["drops"].values())),
            ("box_cached", f"{cached} ({cached / total:.1%} of boxes)"
             if total else cached),
        ]))
    return "\n".join(out) + "\n"


# ---------------------------------------------------------------------------
# Commands

def collect(directory, src, cdb=None, folded=None, delete=False, cache_dir=None,
            build=None):
    folded = folded or Folded()
    folded.fold_dir(directory, delete=delete)
    folded.read_markers(directory)
    resolver = ElfResolver(src, build=build, dump_dir=directory,
                           paths=folded.paths, cache_dir=cache_dir)
    return resolve(folded, resolver, src, cdb)


def run_suite(build, jobs, directory, regrtest_args):
    """Run regrtest on a stats build, folding finished dumps as they appear."""
    if directory.exists() and any(directory.iterdir()):
        raise SystemExit(f"suite: stats directory is not empty: {directory}")
    directory.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, PYTHONSTATS="1", PYTHON_PYSTATS_DIR=str(directory))
    cmd = [str(build / "python"), "-X", "pystats", "-X",
           f"pystats_dir={directory}", "-m", "test", "-j", str(jobs),
           *regrtest_args]
    folded = Folded()
    proc = subprocess.Popen(cmd, env=env)
    while True:
        try:
            proc.wait(timeout=FOLD_PERIOD)
            break
        except subprocess.TimeoutExpired:
            folded.fold_dir(directory, delete=True)
    return folded, proc.returncode


def default_cache():
    base = os.environ.get("XDG_CACHE_HOME") or os.path.expanduser("~/.cache")
    return Path(base) / "check_r27"


def finish(violations, gate, what):
    for v in violations:
        print(f"{what}: VIOLATION: {v}", file=sys.stderr)
    if not violations:
        print(f"{what}: ok", file=sys.stderr)
    return 1 if gate and violations else 0


def suite_gate(summary, args):
    return finish(suite_violations(summary, read_list(args.crashes),
                                   read_list(args.uncounted)), args.gate, "suite")


def cmd_collect(args):
    cdb = load_cdb(args.cdb) if args.cdb else None
    summary = collect(Path(args.dir), args.src, cdb, cache_dir=args.cache)
    Path(args.output).write_text(json.dumps(summary, indent=1))
    return suite_gate(summary, args)


def check_build_flags(build):
    """Refuse a build whose owners cannot be trusted.

    A sibling call hands an entry's return address to its caller's caller,
    so the owner table is only read from a build compiled with
    -fno-optimize-sibling-calls (configure adds it under --enable-pystats).
    """
    build = Path(build)
    pyconfig = build / "pyconfig.h"
    if not pyconfig.exists() or "define Py_STATS 1" not in pyconfig.read_text():
        sys.exit(f"check_r27: {build} is not a --enable-pystats build")
    makefile = build / "Makefile"
    if (not makefile.exists()
            or "-fno-optimize-sibling-calls" not in makefile.read_text()):
        sys.exit(f"check_r27: {build} was not compiled with "
                 "-fno-optimize-sibling-calls; reconfigure it")


def cmd_suite(args):
    # Absolute: the workers run in other directories.
    build = Path(args.build).resolve()
    check_build_flags(build)
    directory = Path(args.dumps).resolve() if args.dumps else build / "r27-dumps"
    folded, rc = run_suite(build, args.jobs, directory, args.regrtest)
    cdb = load_cdb(args.cdb) if args.cdb else None
    summary = collect(directory, args.src, cdb, folded=folded, delete=True,
                      cache_dir=args.cache, build=build)
    summary["regrtest_returncode"] = rc
    Path(args.output).write_text(json.dumps(summary, indent=1))
    print(f"regrtest exited {rc}", file=sys.stderr)
    return suite_gate(summary, args)


def cmd_kernels(args):
    return finish(kernel_violations(json.loads(Path(args.json).read_text())),
                  args.gate, "kernels")


def cmd_report(args):
    kernels = json.loads(Path(args.kernels).read_text()) if args.kernels else None
    suite = json.loads(Path(args.suite).read_text()) if args.suite else None
    sys.stdout.write(make_report(kernels, suite))
    return 0


def cmd_static(args):
    violations, notes = static_violations(args.cdb, args.src, args.ownership)
    if args.gate:
        lint = subprocess.run([sys.executable, str(HERE / "check_value_api.py"),
                               "--cdb", str(args.cdb)])
        if lint.returncode:
            violations.append(f"value API lint exited {lint.returncode}")
    for note in notes:
        print(f"static: {note}", file=sys.stderr)
    return finish(violations, args.gate, "static")


def synthetic_self_test():
    """Fold synthetic dumps through a fake resolver; check each class."""
    bids = {"aa": "Objects/abstract.c", "bb": "Modules/_testcapimodule.c",
            "cc": "Modules/_testinternalcapi.c", "dd": "Modules/_json.c"}

    class Fake(Resolver):
        def lookup(self, bid, addr):
            if bid == "ee":
                return "/usr/lib/foo.c"
            return str(DEFAULT_SRC / bids[bid]) if bid in bids else None

        def in_tree(self, bid):
            if bid == "ee":
                return False
            return True if bid in bids else None

    with tempfile.TemporaryDirectory() as tmp:
        text = "\n".join([
            "Stats process: pid=1 tag=x",
            "Box object [0]: /x/python build-id=aa",
            "Box object [1]: /x/t.so build-id=bb",
            "Box object [2]: /x/i.so build-id=cc",
            "Box object [3]: /x/e.so build-id=ee",
            "Box owner (thunk, call) 0+0x10 at 0+0x1: 1",
            "Box owner (thunk, call) 1+0x10 at 0+0x1: 2",
            "Box owner (thunk, call) 2+0x10 at 0+0x1: 3",
            "Box owner (thunk, call) 3+0x10 at 0+0x1: 4",
            "Box owner (call adapter, call) foreign at 0+0x1: 5",
            "Box owner (thunk, call) ? at 0+0x1: 6",
            "Box owner (core, list) - at 0+0x20: 7",
            "Box owner (thunk, tuple) 9+0x10 at 0+0x1: 8", ""])
        (Path(tmp) / "a.txt").write_text(text)
        folded = Folded()
        folded.fold_dir(tmp)
        summary = resolve(folded, Fake(), DEFAULT_SRC)
    got = [r["class"] for r in summary["rows"]]
    want = {"core": 2, "compat-test": 1, "test": 1, "external": 1,
            "foreign": 1, "unresolved": 2}
    counts = {c: got.count(c) for c in set(got)}
    assert counts == want, (counts, want)
    print("self-test: synthetic classes ok")


def stats_self_test(build, src):
    build = Path(build)
    probe = subprocess.run([str(build / "python"), "-c",
                            "import sys; print('_stats_on' in dir(sys))"],
                           capture_output=True, text=True)
    if probe.stdout.strip() != "True":
        print("self-test: build has no stats; real-dump part skipped")
        return
    tmp = tempfile.mkdtemp(prefix="r27-")
    try:
        env = dict(os.environ, PYTHONSTATS="1", PYTHON_PYSTATS_DIR=tmp)
        subprocess.run([str(build / "python"), "-X", "pystats", "-c",
                        "import _testcapi; _testcapi.number_add(1000, 2000)"],
                       env=env, check=True)
        summary = collect(Path(tmp), src, build=build)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    units = {r["unit"] for r in summary["rows"]}
    assert "Objects/abstract.c" in units, sorted(units)
    print("self-test: real dump resolves Objects/abstract.c")


def build_parser():
    p = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    p.add_argument("--self-test", action="store_true")
    p.add_argument("--build", dest="self_build",
                   help="stats build for the real-dump part of --self-test")
    sub = p.add_subparsers(dest="command")

    def common(sp, **kw):
        sp.add_argument("--gate", action="store_true",
                        help="exit 1 on a violation")
        if kw.get("src", True):
            sp.add_argument("--src", default=str(DEFAULT_SRC))
        return sp

    def suite_options(sp):
        sp.add_argument("--cdb", help="compile_commands.json for unit_kind")
        sp.add_argument("--cache", default=default_cache(),
                        help="directory of per-build-id CU tables")
        sp.add_argument("--crashes", default=HERE / "r27_crashes.txt")
        sp.add_argument("--uncounted", default=HERE / "r27_uncounted.txt")
        sp.add_argument("-o", "--output", required=True)

    sp = common(sub.add_parser("collect", help="fold and resolve a dump dir"))
    sp.add_argument("dir")
    suite_options(sp)
    sp.set_defaults(func=cmd_collect)

    sp = common(sub.add_parser("suite", help="run regrtest on a stats build"))
    sp.add_argument("--build", required=True)
    sp.add_argument("--dumps", help="fresh stats directory for this attempt")
    sp.add_argument("-j", "--jobs", type=int, default=os.cpu_count() or 1)
    suite_options(sp)
    sp.add_argument("regrtest", nargs="*", help="regrtest arguments after --")
    sp.set_defaults(func=cmd_suite)

    sp = common(sub.add_parser("kernels", help="check kernel box counters"),
                src=False)
    sp.add_argument("json")
    sp.set_defaults(func=cmd_kernels)

    sp = sub.add_parser("report", help="Markdown tables of section 3.2")
    sp.add_argument("--kernels")
    sp.add_argument("--suite")
    sp.set_defaults(func=cmd_report)

    sp = common(sub.add_parser("static", help="part 1 of the check"))
    sp.add_argument("--cdb", required=True)
    sp.add_argument("--ownership")
    sp.set_defaults(func=cmd_static)
    return p


def main(argv=None):
    argv = sys.argv[1:] if argv is None else argv
    # Arguments after a bare -- belong to regrtest.
    extra = []
    if "--" in argv:
        i = argv.index("--")
        argv, extra = argv[:i], argv[i + 1:]
    args = build_parser().parse_args(argv)
    if args.self_test:
        synthetic_self_test()
        if args.self_build:
            stats_self_test(args.self_build, DEFAULT_SRC)
        return 0
    if not args.command:
        build_parser().error("a subcommand or --self-test is required")
    if args.command == "suite":
        args.regrtest = [*args.regrtest, *extra]
    elif extra:
        build_parser().error("unexpected arguments after --")
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
