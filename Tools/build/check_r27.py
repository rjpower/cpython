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
process), <rand>.done (its start-time identity), <rand>.zero (an explicit
zero-stats outcome), <rand>.run (a process that has not finished), <rand>.spawn
(structured launch events), objects/<build-id> (copies of objects that no build
product explains). Completed dumps require a matching .done marker; .zero,
orphan, malformed and contradictory identities are accounting failures.

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
from collections import Counter
import json
import os
import re
import shutil
import stat
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
INVALID_OWNER_SAMPLE_LIMIT = 16


def strict_json_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate JSON field: {key}")
        result[key] = value
    return result


def read_regular_text(path):
    if not stat.S_ISREG(path.lstat().st_mode):
        raise ValueError("metadata file is not regular")
    return path.read_text(encoding="utf-8")

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
PROCESS_RE = re.compile(r"^Stats process: pid=(\d+) tag=(.*)$")
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
        self.counted_spawn = []
        self.opaque_spawn = []
        self.dump_identities = {}
        self.folded_dump_stems = set()
        self.identity_records = {}
        self.identity_errors = []
        self.identity_matched_count = 0
        self.identity_required = False
        self.accounting_errors = []
        self.invalid_owner_count = 0
        self.invalid_owner_samples = []
        self.child_receipts = []

    def add_text(self, text, source="<text>", require_child_receipts=False):
        objects = {}  # dump-local index -> build-id
        owners = []
        tag = ""
        resets = 0
        totals, drops = {}, {}
        receipt_errors = []
        for line in text.splitlines():
            if m := OBJECT_RE.match(line):
                bid = m[3].lower()
                objects[int(m[1])] = bid
                self.paths.setdefault(bid, set()).add(m[2])
            elif m := OWNER_RE.match(line):
                owners.append(m)
            elif m := PROCESS_RE.match(line):
                tag = m[2]
            elif line.startswith("Stats child receipt errors:"):
                receipt_errors.append(line.removeprefix("Stats child receipt errors: "))
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
        if require_child_receipts:
            if len(receipt_errors) != 1 or not receipt_errors[0].isdigit():
                self.accounting_errors.append("missing or malformed child receipt error count")
            elif int(receipt_errors[0]):
                self.accounting_errors.append("native child receipt publication failed")
        counts = {}
        for m in owners:
            add(counts, m["kind"], int(m["count"]))
            if (m["kind"] not in KINDS or
                (m["kind"] == "core" and (m["owner"] != "-" or m["at"] in ("-", "?"))) or
                (m["kind"] != "core" and m["owner"] == "-")):
                self.accounting_errors.append("invalid owner attribution")
                self.invalid_owner_count += 1
                if len(self.invalid_owner_samples) < INVALID_OWNER_SAMPLE_LIMIT:
                    self.invalid_owner_samples.append({
                        "dump": source, "tag": tag, "kind": m["kind"],
                        "site": m["site"], "owner": m["owner"],
                        "at": m["at"], "count": int(m["count"]),
                    })
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
        """Fold finished dumps, retaining their process identity by stem."""
        self.identity_required = True
        for path in sorted(Path(directory).glob("*.txt")):
            try:
                text = path.read_text(errors="replace")
                self.add_text(text, path.name, require_child_receipts=True)
            except OSError as exc:
                self.bad_dumps.append(f"{path.name}: {exc}")
                continue
            process_rows = [PROCESS_RE.match(line) for line in text.splitlines()
                            if line.startswith("Stats process:")]
            if path.stem in self.folded_dump_stems:
                message = f"duplicate folded dump stem {path.stem}"
                self.identity_errors.append(message)
                self.accounting_errors.append(message)
            elif len(process_rows) != 1 or process_rows[0] is None:
                self.folded_dump_stems.add(path.stem)
                message = f"dump {path.name} has missing or malformed process identity"
                self.identity_errors.append(message)
                self.accounting_errors.append(message)
            else:
                self.folded_dump_stems.add(path.stem)
                match = process_rows[0]
                pid = int(match[1])
                if pid <= 0:
                    message = f"dump {path.name} has invalid process pid"
                    self.identity_errors.append(message)
                    self.accounting_errors.append(message)
                else:
                    self.dump_identities[path.stem] = {"pid": pid, "tag": match[2]}
            if delete:
                path.unlink(missing_ok=True)

    @staticmethod
    def _read_identity(path):
        """Parse one start-time marker without normalizing escaped argv bytes."""
        fields = {}
        for line in read_regular_text(path).splitlines():
            key, sep, value = line.partition(": ")
            if not sep or key not in {"version", "pid", "ppid", "tag", "argv", "image_dev", "image_ino"} or key in fields:
                raise ValueError("invalid or duplicate marker field")
            fields[key] = value
        if set(fields) != {"version", "pid", "ppid", "tag", "argv", "image_dev", "image_ino"} or fields["version"] != "2":
            raise ValueError("incomplete marker")
        if not fields["pid"].isdigit() or int(fields["pid"]) <= 0:
            raise ValueError("invalid marker pid")
        if not fields["ppid"].isdigit():
            raise ValueError("invalid marker ppid")
        raw_argv = json.loads(fields["argv"])
        if (not isinstance(raw_argv, list) or not raw_argv or
            any(not isinstance(arg, str) for arg in raw_argv)):
            raise ValueError("marker argv is not a nonempty string list")
        argv = [os.fsdecode(arg.encode("latin-1")) for arg in raw_argv]
        if (not fields["image_dev"].isdigit() or
            not fields["image_ino"].isdigit() or
            int(fields["image_ino"]) == 0):
            raise ValueError("invalid marker image identity")
        return {"id": path.stem, "pid": int(fields["pid"]),
                "ppid": int(fields["ppid"]), "tag": fields["tag"],
                "argv": argv, "image_dev": int(fields["image_dev"]),
                "image_ino": int(fields["image_ino"])}

    def read_markers(self, directory):
        """List .run files that no .txt replaced, and the .spawn lines."""
        directory = Path(directory)
        self.leftover_run = []
        self.identity_records = {}
        self.identity_matched_count = 0

        def identity_error(message):
            self.identity_errors.append(message)
            self.accounting_errors.append(message)

        sidecars = {}
        for suffix in ("done", "zero", "run"):
            for path in sorted(directory.glob(f"*.{suffix}")):
                try:
                    record = self._read_identity(path)
                except (OSError, UnicodeError, ValueError) as exc:
                    identity_error(
                        f"malformed {suffix} identity {path.name}: {type(exc).__name__}")
                    continue
                sidecars.setdefault(path.stem, {})[suffix] = record
                self.identity_records[path.name] = record
                if suffix == "run":
                    self.leftover_run.append(record)
                elif suffix == "zero":
                    identity_error(
                        f"zero-stats identity {path.name} has no completed dump")

        for stem, dump in self.dump_identities.items():
            records = sidecars.get(stem, {})
            done = records.get("done")
            if done is None:
                identity_error(f"missing .done identity for dump {stem}.txt")
                continue
            if "run" in records:
                identity_error(f"contradictory .run and .txt identity for {stem}")
            if "zero" in records:
                identity_error(f"contradictory .zero and completed dump for {stem}")
            if (done["pid"] != dump["pid"] or done["tag"] != dump["tag"]):
                identity_error(f".done identity does not match dump {stem}.txt")
            elif "run" not in records and "zero" not in records:
                self.identity_matched_count += 1
        for stem in sidecars:
            if "done" in sidecars[stem] and stem not in self.dump_identities:
                identity_error(f"orphan .done identity {stem}.done")
        roots_path = directory / "root-launch.json"
        roots = []
        try:
            roots = json.loads(read_regular_text(roots_path),
                               object_pairs_hook=strict_json_object) if (
                                   sidecars or self.dump_identities or roots_path.exists()) else []
            if not isinstance(roots, list):
                raise ValueError("root launches must be a list")
            for root in roots:
                if (not isinstance(root, dict) or set(root) !=
                    {"version", "pid", "ppid", "argv", "image_dev", "image_ino"}
                    or type(root["version"]) is not int or root["version"] != 1
                    or type(root["pid"]) is not int
                    or root["pid"] <= 0 or type(root["ppid"]) is not int
                    or root["ppid"] < 0
                    or type(root["image_dev"]) is not int
                    or root["image_dev"] < 0
                    or type(root["image_ino"]) is not int
                    or root["image_ino"] <= 0 or not isinstance(root["argv"], list)
                    or not root["argv"] or any(not isinstance(a, str) for a in root["argv"])):
                    raise ValueError("invalid root launch")
        except (OSError, UnicodeError, ValueError) as exc:
            identity_error(f"missing or malformed root launches: {type(exc).__name__}")
            roots = []

        self.child_receipts = []
        for path in sorted(directory.glob("*.child")):
            try:
                parts = path.name.split(".")
                if (len(parts) != 3 or not re.fullmatch(r"[0-9a-f]{40}", parts[0])
                    or not parts[1].isdigit() or int(parts[1]) <= 0):
                    raise ValueError("invalid receipt filename")
                receipt = json.loads(read_regular_text(path),
                                     object_pairs_hook=strict_json_object)
                if (not isinstance(receipt, dict) or set(receipt) !=
                    {"version", "parent_pid", "child_pid", "parent_stem", "serial",
                     "source", "parent_image_dev", "parent_image_ino",
                     "candidate_count", "candidates"}
                    or type(receipt["version"]) is not int
                    or receipt["version"] != 1
                    or any(type(receipt[k]) is not int or receipt[k] <= 0
                           for k in ("parent_pid", "child_pid", "serial", "parent_image_ino"))
                    or type(receipt["parent_image_dev"]) is not int
                    or receipt["parent_image_dev"] < 0
                    or receipt["parent_stem"] != parts[0]
                    or receipt["serial"] != int(parts[1])
                    or receipt["source"] not in
                        {"spawn", "spawnp", "fork", "fork1", "forkpty", "fork_exec"}
                    or type(receipt["candidate_count"]) is not int
                    or not isinstance(receipt["candidates"], list)
                    or receipt["candidate_count"] != len(receipt["candidates"])
                    or any(not isinstance(c, str) for c in receipt["candidates"])):
                    raise ValueError("invalid receipt fields")
                n = receipt["candidate_count"]
                if (n != 0 if receipt["source"] in {"fork", "fork1", "forkpty"}
                    else n != 1 if receipt["source"] in {"spawn", "spawnp"}
                    else n < 1):
                    raise ValueError("invalid receipt candidate count")
                receipt["candidates"] = [os.fsdecode(c.encode("latin-1"))
                                         for c in receipt["candidates"]]
                self.child_receipts.append(receipt)
            except (OSError, UnicodeError, ValueError, TypeError) as exc:
                identity_error(f"malformed child receipt {path.name}: {type(exc).__name__}")
        for path in directory.glob("*.child.tmp"):
            identity_error(f"unfinished child receipt {path.name}")

        by_pid = {}
        for record in self.identity_records.values():
            by_pid.setdefault(record["pid"], []).append(record)
        admitted = set()
        for root in roots:
            pid = root["pid"]
            if pid in admitted:
                identity_error(f"duplicate root launch for pid {pid}")
                continue
            matches = by_pid.get(pid, [])
            if not matches or not all(
                r["ppid"] == root["ppid"] and r["argv"] == root["argv"]
                and r["image_dev"] == root["image_dev"]
                and r["image_ino"] == root["image_ino"] for r in matches):
                identity_error(f"root launch does not match marker for pid {pid}")
            admitted.add(pid)
        child_pids = set()
        for receipt in self.child_receipts:
            parent = sidecars.get(receipt["parent_stem"], {}).get("done")
            pid = receipt["child_pid"]
            if pid in child_pids or pid in admitted:
                identity_error(f"duplicate child receipt for pid {pid}")
            child_pids.add(pid)
            if (parent is None or parent["pid"] != receipt["parent_pid"]
                or parent["image_dev"] != receipt["parent_image_dev"]
                or parent["image_ino"] != receipt["parent_image_ino"]):
                identity_error(f"unmatched parent interval for child pid {pid}")
            matches = by_pid.get(pid, [])
            if not matches or not all(r["ppid"] == receipt["parent_pid"]
                                      for r in matches):
                identity_error(f"native child receipt lacks matching child marker: {pid}")
        for pid in by_pid.keys() - admitted - child_pids:
            identity_error(f"process pid {pid} has no root launch or native receipt")
        reachable = set(admitted)
        for _ in range(len(self.child_receipts)):
            prior = len(reachable)
            reachable.update(r["child_pid"] for r in self.child_receipts
                             if r["parent_pid"] in reachable)
            if len(reachable) == prior:
                break
        for pid in by_pid.keys() - reachable:
            identity_error(f"process pid {pid} is not reachable from a root launch")
        images = {(r["image_dev"], r["image_ino"]) for r in roots}
        if len(images) != 1 and by_pid:
            identity_error("root launches have no single measured image")
        elif images:
            image = next(iter(images))
            for records in by_pid.values():
                if any((r["image_dev"], r["image_ino"]) != image for r in records):
                    identity_error("child marker has an unmeasured executable image")
        self.spawn = []
        self.counted_spawn = []
        self.opaque_spawn = []
        for path in sorted(directory.glob("*.spawn")):
            try:
                contents = path.read_bytes().decode("utf-8")
            except (OSError, UnicodeError) as exc:
                self.accounting_errors.append(
                    f"unreadable spawn ledger {path.name}: {type(exc).__name__}")
                continue
            if not contents.strip():
                self.accounting_errors.append(f"empty spawn ledger {path.name}")
                continue
            if contents and not contents.endswith("\n"):
                self.accounting_errors.append(
                    f"truncated spawn ledger {path.name}")
            for line in contents.splitlines():
                if not line.strip():
                    continue
                if line.startswith("{"):
                    try:
                        launch = json.loads(line)
                    except ValueError:
                        self.accounting_errors.append(
                            f"malformed spawn JSON {path.name}")
                        continue
                    if not isinstance(launch, dict):
                        self.accounting_errors.append(
                            f"malformed spawn record {path.name}")
                        continue
                    shape = launch.get("shape")
                    classification = launch.get("classification")
                    argv = launch.get("argv")
                    counts = launch.get("counts")
                    if (not isinstance(launch.get("tag"), str) or
                        type(launch.get("pid")) is not int or launch["pid"] <= 0 or
                        not isinstance(launch.get("event"), str) or
                        not isinstance(launch.get("executable"), (str, type(None))) or
                        not isinstance(launch.get("reason"), str) or
                        not isinstance(launch.get("command"), (str, type(None))) or
                        not isinstance(shape, str) or
                        classification not in ("direct", "opaque-shell")):
                        self.accounting_errors.append(
                            f"incomplete spawn record in {path.name}")
                        continue
                    if classification == "opaque-shell":
                        valid_argv = (argv is None if shape == "opaque-shell" else
                                      isinstance(argv, list) and
                                      all(isinstance(arg, str) for arg in argv))
                        valid_command = (shape == "structured" or
                                         isinstance(launch.get("command"), str))
                        if counts is not None or not valid_argv or not valid_command:
                            self.accounting_errors.append(
                                f"incomplete opaque spawn record in {path.name}")
                            continue
                        self.opaque_spawn.append(launch)
                        self.accounting_errors.append(
                            f"opaque shell launch in {path.name}")
                        continue
                    if (not isinstance(argv, list) or not argv or
                        any(not isinstance(arg, str) for arg in argv) or
                        shape != "structured" or classification != "direct" or
                            not isinstance(launch.get("executable"), str) or
                        not launch["executable"] or type(counts) is not bool):
                        self.accounting_errors.append(
                            f"invalid structured spawn record in {path.name}")
                        continue
                    if counts is True:
                        self.counted_spawn.append(launch)
                        continue
                    if counts is not False:
                        self.accounting_errors.append(
                            f"invalid spawn prediction in {path.name}")
                        continue
                    self.spawn.append({
                        "tag": launch.get("tag", ""), "argv": argv,
                        "command": launch.get("command"),
                        "reason": launch.get("reason", ""),
                        "event": launch.get("event", ""),
                        "pid": launch["pid"],
                    })
                    continue
                tag, _, argv = line.partition("\t")
                # Historical TSV records have no reason or argument boundaries.
                # Retain them for diagnosis; the admission gate rejects them.
                self.spawn.append({"tag": tag, "argv": argv})

        for path in sorted(directory.glob("*.audit-error")):
            self.accounting_errors.append(
                f"audit error ledger present: {path.name}")
            try:
                path.read_bytes()
            except OSError as exc:
                self.accounting_errors.append(
                    f"unreadable audit error ledger {path.name}: {type(exc).__name__}")

        self.identity_complete = (
            self.identity_required and not self.identity_errors and
            not self.bad_dumps and self.dumps == len(self.dump_identities) and
            self.identity_matched_count == len(self.dump_identities))

        identity_kinds = ("done", "zero", "run")
        for stem, records in sidecars.items():
            if sum(kind in records for kind in identity_kinds) > 1:
                identity_error(f"contradictory process identity sidecars for {stem}")
                self.identity_complete = False


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
        "spawn": folded.spawn, "counted_spawn": folded.counted_spawn,
        "opaque_spawn": folded.opaque_spawn, "resets": folded.resets,
        "identity_coverage": {
            "complete": getattr(folded, "identity_complete", False),
            "dumps": len(folded.dump_identities),
            "markers": len(folded.identity_records),
            "matched": folded.identity_matched_count,
            "error_count": len(folded.identity_errors),
            "errors": list(folded.identity_errors[:16]),
            "samples": [
                {"stem": stem, **identity}
                for stem, identity in list(sorted(folded.dump_identities.items()))[:16]
            ],
            "marker_samples": [
                {"file": name, **identity}
                for name, identity in list(sorted(folded.identity_records.items()))[:16]
            ],
        },
        "resets_by_tag": folded.resets_by_tag, "drops": folded.drops,
        "box_cached": folded.box_cached, "kind_totals": folded.kind_totals,
        "site_totals": folded.site_totals, "rows": rows, "objects": objects,
        "stray_cus": sorted(strays), "accounting_errors": folded.accounting_errors,
        "invalid_owner_count": folded.invalid_owner_count,
        "invalid_owner_samples": folded.invalid_owner_samples,
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


def read_accounting_rules(path):
    """Read exact, bounded process admissions. Missing file admits nothing."""
    path = Path(path)
    if not path.is_file():
        return {"runs": {}, "spawns": {}}
    data = json.loads(path.read_text())
    if not isinstance(data, dict) or set(data) != {"runs", "spawns"}:
        raise ValueError("accounting rules need runs and spawns lists")
    rules = {"runs": {}, "spawns": {}}
    for kind, fields in (("runs", ("tag", "argv")),
                         ("spawns", ("tag", "reason", "event", "argv"))):
        if not isinstance(data[kind], list):
            raise ValueError(f"accounting {kind} must be a list")
        for row in data[kind]:
            if not isinstance(row, dict) or set(row) != set(fields) | {"max", "why"}:
                raise ValueError(f"invalid accounting {kind} rule")
            if (type(row["max"]) is not int or row["max"] < 1 or
                not isinstance(row["why"], str) or not row["why"].strip()):
                raise ValueError(f"invalid accounting {kind} bound or reason")
            if any(not isinstance(row[field], str) or not row[field]
                   for field in fields if field != "argv"):
                raise ValueError(f"invalid accounting {kind} identity")
            argv = row["argv"]
            if (not isinstance(argv, list) or not argv or
                any(not isinstance(arg, str) for arg in argv)):
                raise ValueError(f"accounting {kind} argv must be a list")
            key = (*(row[field] for field in fields[:-1]), tuple(argv))
            if key in rules[kind]:
                raise ValueError(f"duplicate accounting {kind} rule")
            rules[kind][key] = row["max"]
    return rules


def normalize_accounting_argv(argv, summary):
    """Normalize an exact executable and its explicit stats-dir option."""
    words = list(argv)
    build = summary.get("build")
    products = ({f"{build}/{name}": f"{{BUILD}}/{name}"
                for name in ("python", "Programs/_testembed")}
                if build else {})
    if not words or words[0] not in products:
        return tuple(words)
    words[0] = products[words[0]]
    dumps = summary.get("dump_dir")
    if dumps:
        i = 1
        while i < len(argv):
            arg = argv[i]
            if arg in ("-c", "-m", "--") or not arg.startswith("-"):
                break
            if arg == "-X" and i + 1 < len(argv):
                if argv[i + 1] == f"pystats_dir={dumps}":
                    words[i + 1] = "pystats_dir={DUMPS}"
                i += 2
                continue
            if arg == "-W" and i + 1 < len(argv):
                i += 2
                continue
            if arg == f"-Xpystats_dir={dumps}":
                words[i] = "-Xpystats_dir={DUMPS}"
            i += 1
    return tuple(words)


def suite_violations(summary, crashes=(), uncounted=(), accounting=None):
    accounting = accounting or {"runs": {}, "spawns": {}}
    out = []
    identity_coverage = summary.get("identity_coverage")
    if not isinstance(identity_coverage, dict) or identity_coverage.get("complete") is not True:
        out.append("missing or incomplete process identity coverage")
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
    run_counts = Counter()
    for run in summary["leftover_run"]:
        tag = run.get("tag", "")
        if tag not in crashes:
            out.append(f"leftover .run {run['id']} (tag {tag!r}, "
                       f"pid {run.get('pid', '?')}) not in the crash list")
        if (set(run) != {"id", "pid", "ppid", "tag", "argv"} or
            not str(run.get("pid", "")).isdigit() or
            not str(run.get("ppid", "")).isdigit() or
            not isinstance(run.get("argv"), list) or not run["argv"] or
            any(not isinstance(arg, str) for arg in run["argv"])):
            out.append(f"leftover .run {run['id']} has incomplete metadata")
            continue
        run_counts[(tag, normalize_accounting_argv(run["argv"], summary))] += 1
    for key, count in sorted(run_counts.items()):
        cap = accounting["runs"].get(key)
        if cap is None or count > cap:
            out.append(f"leftover .run shape {key!r}: {count} observed, "
                       f"approved max {cap!r}")
    # A launch without a tag (the pty REPL tests) is listed as <untagged>.
    launched = {s["tag"] or "<untagged>" for s in summary["spawn"]}
    for tag in sorted(launched - set(uncounted)):
        out.append(f"uncounted launch by tag {tag!r} not in the uncounted list")
    spawn_counts = Counter()
    for launch in summary["spawn"]:
        argv = launch.get("argv")
        if (not isinstance(argv, list) or not argv or
            any(not isinstance(arg, str) for arg in argv) or
            not isinstance(launch.get("reason"), str) or
            not launch["reason"] or not isinstance(launch.get("event"), str) or
            not launch["event"] or type(launch.get("pid")) is not int):
            out.append(f"uncounted launch in tag {launch.get('tag', '')!r} "
                       "has incomplete metadata")
            continue
        key = (launch["tag"], launch["reason"], launch["event"],
               normalize_accounting_argv(argv, summary))
        spawn_counts[key] += 1
    for key, count in sorted(spawn_counts.items()):
        cap = accounting["spawns"].get(key)
        if cap is None or count > cap:
            out.append(f"uncounted launch shape {key!r}: {count} observed, "
                       f"approved max {cap!r}")
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


def value_provider_declared(name, headers):
    """Whether Include declares a callable value provider for an inventory row."""
    escaped = re.escape(name)
    direct = re.compile(
        rf"(?m)^[ \t]*(?:PyAPI_FUNC\([^)]+\)|Py_LOCAL_INLINE\([^)]+\)|"
        rf"static[ \t]+inline\b[^;{{}}]*?|extern\b[^;{{}}]*?)"
        rf"\s+\(?{escaped}\)?\s*\(")
    if any(direct.search(text) for _, text in headers):
        return True

    if name.startswith("_PyValue_InPlace"):
        family = "INPLACE"
        suffix = name[len("_PyValue_InPlace"):]
        constructor = "_PyValue_InPlace##NAME"
    elif name.startswith("_PyValue_"):
        family = "BINARY"
        suffix = name[len("_PyValue_"):]
        constructor = "_PyValue_##NAME"
    else:
        return False
    if re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", suffix) is None:
        return False
    header = next((text for path, text in headers
                   if path.name == "pycore_value_ops.h"), "")
    roster_name = f"_PyValue_{family}_DISPATCHERS"
    declaration_name = f"_PyValue_DECLARE_{family}"

    def macro_body(name):
        lines = header.splitlines()
        for i, line in enumerate(lines):
            if line.startswith(f"#define {name}("):
                body = [line]
                while body[-1].rstrip().endswith("\\") and i + 1 < len(lines):
                    i += 1
                    body.append(lines[i])
                return "\n".join(body)
        return ""

    roster = macro_body(roster_name)
    declaration = macro_body(declaration_name)
    return (f"X({suffix})" in roster
            and "PyAPI_FUNC(PyValue)" in declaration
            and constructor in declaration
            and f"{roster_name}({declaration_name})" in header)


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
        rows = []
        for section in ("row", "manual"):
            part = inventory.get(section, [])
            if not isinstance(part, list):
                out.append(f"ownership {section} section must be an array of tables")
            else:
                rows.extend(part)
        if inventory.get("r27_complete") is not True:
            out.append("ownership inventory is not reviewed as complete for R27")
    except (OSError, ValueError, KeyError) as exc:
        out.append(f"ownership inventory unreadable: {exc}")
        return out, []
    if not rows:
        out.append("ownership inventory is empty")
    if not check_value_api.MARKED_SLOT_FAMILIES:
        out.append("direct object-slot calls are not enforced by value API lint")
    headers = [(p, check_value_api.strip_c(p.read_text(errors="replace")))
               for p in (src / "Include").rglob("*.h")]
    if not headers:
        out.append("no headers found for ownership inventory")
    seen = set()
    for row in rows:
        if not isinstance(row, dict):
            out.append(f"invalid ownership row: {row!r}")
            continue
        obj, value = row.get("object"), row.get("value")
        if (not isinstance(obj, str) or
            re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", obj) is None or
            not isinstance(value, str) or
            re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", value) is None):
            out.append(f"invalid ownership row: {row!r}")
            continue
        if obj in seen:
            out.append(f"duplicate ownership entry: {obj}")
        seen.add(obj)
        if not value_provider_declared(value, headers):
            out.append(f"{obj}: value provider {value} declaration missing")
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
    summary = resolve(folded, resolver, src, cdb)
    summary["dump_dir"] = str(Path(directory).resolve())
    if build is not None:
        summary["build"] = str(Path(build).resolve())
    return summary


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
    image = (build / "python").stat()
    (directory / "root-launch.json").write_text(json.dumps([{
        "version": 1, "pid": proc.pid, "ppid": os.getpid(), "argv": cmd,
        "image_dev": image.st_dev, "image_ino": image.st_ino,
    }]))
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
                                   read_list(args.uncounted),
                                   read_accounting_rules(args.accounting)),
                  args.gate, "suite")


def cmd_collect(args):
    cdb = load_cdb(args.cdb) if args.cdb else None
    summary = collect(Path(args.dir), args.src, cdb, cache_dir=args.cache,
                      build=args.build)
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
        sp.add_argument("--accounting", default=HERE / "r27_accounting.json",
                        help="exact bounded process-accounting admissions")
        sp.add_argument("-o", "--output", required=True)

    sp = common(sub.add_parser("collect", help="fold and resolve a dump dir"))
    sp.add_argument("dir")
    sp.add_argument("--build", help="stats build for marker path normalization")
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
