#!/usr/bin/env python3
"""Build the generator-delegation benchmark report from sealed campaign JSONs."""

import hashlib
import json
import math
from pathlib import Path


EVIDENCE = Path(
    "/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/generator-delegation52"
)
OUTPUT = EVIDENCE / "analysis.json"
ROSTER = (
    "richards go nbody float spectral_norm raytrace deltablue chaos nqueens "
    "fannkuch generators comprehensions coroutines unpack_sequence deepcopy "
    "deepcopy_reduce deepcopy_memo json_dumps json_loads pickle unpickle "
    "regex_compile logging_format logging_silent logging_simple "
    "xml_etree_generate xml_etree_iterparse xml_etree_parse xml_etree_process"
).split()
HOT = ("generators", "coroutines", "deltablue", "logging_silent", "comprehensions")
CAMPAIGNS = {
    "first_upstream": (
        "final52-all29-upstream", 29, 2,
        "58f9ea6cfee47536893ec59780a0f719b4c856ef",
        "3a440b65695615e9fd9912342b79cdd54fced0d2", "upstream",
    ),
    "first_golden51": (
        "confirm52-hot-golden", 5, 6,
        "58f9ea6cfee47536893ec59780a0f719b4c856ef",
        "0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254", "golden",
    ),
    "entry_golden51": (
        "entry52-all29-golden", 29, 2,
        "e85171923c08b0d68ee169119e761d7583bbcb26",
        "0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254", "golden",
    ),
    "entry_hot_golden51": (
        "entry52-confirm-hot-golden", 5, 6,
        "e85171923c08b0d68ee169119e761d7583bbcb26",
        "0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254", "golden",
    ),
    "entry_upstream": (
        "entry52-all29-upstream", 29, 2,
        "e85171923c08b0d68ee169119e761d7583bbcb26",
        "3a440b65695615e9fd9912342b79cdd54fced0d2", "upstream",
    ),
}


def read_json(path):
    return json.loads(path.read_text())


def load_campaign(label, config):
    name, case_count, pair_count, candidate_head, control_head, control = config
    root = EVIDENCE / name
    paths = [
        root / filename
        for filename in (
            "summary.json",
            "status.json",
            "identity-before.json",
            "identity-after.json",
        )
    ]
    missing = [str(path) for path in paths if not path.is_file()]
    if missing:
        raise FileNotFoundError(f"{label} campaign is incomplete; missing: {', '.join(missing)}")
    summary, status, before, after = map(read_json, paths)
    required_status = {"failure", "postflight_error", "stable_identity"}
    if not required_status <= status.keys():
        raise ValueError(f"{label} status is missing required fields: {required_status - status.keys()}")
    if (
        status["failure"] is not None
        or status["postflight_error"] is not None
        or status["stable_identity"] is not True
    ):
        raise ValueError(f"{label} campaign did not pass failure/postflight/stable-identity gates: {status}")
    if before != after:
        raise ValueError(f"{label} identity changed during the campaign")
    if "errors" not in before or before["errors"] != []:
        raise ValueError(f"{label} identity has errors or lacks the errors field")
    if set(summary) != set(ROSTER if case_count == 29 else HOT):
        raise ValueError(f"{label} case roster mismatch")
    for case, values in summary.items():
        ratios = values["paired_ratios"]
        if len(ratios) != pair_count or any(not math.isfinite(x) or x <= 0 for x in ratios):
            raise ValueError(f"{label}/{case} has invalid pair vector")
        expected = {
            "geomean": geomean(ratios),
            "min": min(ratios),
            "max": max(ratios),
        }
        if any(
            key not in values or not math.isclose(values[key], value, rel_tol=1e-12)
            for key, value in expected.items()
        ):
            raise ValueError(f"{label}/{case} summary statistics do not match paired ratios")
    for identity in (before, after):
        builds = identity["builds"]
        if builds["candidate"]["git"]["head"] != candidate_head or builds[control]["git"]["head"] != control_head:
            raise ValueError(f"{label} source identity mismatch")
    return {
        "name": name,
        "path": str(root),
        "summary": summary,
        "status": status,
        "identities": {"before": before, "after": after},
        "inputs": {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in paths},
    }


def geomean(values):
    return math.exp(sum(math.log(value) for value in values) / len(values))


def cell(row):
    return f"{row['geomean']:.6f} ({row['min']:.6f}–{row['max']:.6f})"


def render(campaigns):
    first = campaigns["first_upstream"]["summary"]
    entry_golden = campaigns["entry_golden51"]["summary"]
    entry_upstream = campaigns["entry_upstream"]["summary"]
    first_golden = campaigns["first_golden51"]["summary"]
    entry_hot = campaigns["entry_hot_golden51"]["summary"]
    lines = [
        "# Generator delegation benchmark tables",
        "",
        "Both candidates are rejected; golden51 remains unchanged. GCC 15.2, CPU 15, hash 0, JIT disabled. Ratios are candidate/control elapsed time; lower is faster. Each column is a direct campaign comparison. Never multiply ratios across campaigns.",
        "",
        "## All 29 cases (two paired runs)",
        "",
        "Each cell shows geometric mean and paired minimum–maximum.",
        "",
        "| Benchmark | First / upstream | Entry / golden51 | Entry / upstream |",
        "| --- | ---: | ---: | ---: |",
    ]
    for case in ROSTER:
        lines.append(f"| {case} | {cell(first[case])} | {cell(entry_golden[case])} | {cell(entry_upstream[case])} |")
    lines += [
        f"| **Geometric mean (29)** | **{geomean([first[c]['geomean'] for c in ROSTER]):.6f}** | **{geomean([entry_golden[c]['geomean'] for c in ROSTER]):.6f}** | **{geomean([entry_upstream[c]['geomean'] for c in ROSTER]):.6f}** |",
        "",
        f"Raw vectors and provenance are retained in `{OUTPUT}`.",
        "",
        "## Hot confirmation (six paired runs)",
        "",
        "| Benchmark | First / golden51 | Entry / golden51 |",
        "| --- | ---: | ---: |",
    ]
    for case in HOT:
        first_losses = sum(x > 1 for x in first_golden[case]["paired_ratios"])
        entry_losses = sum(x > 1 for x in entry_hot[case]["paired_ratios"])
        lines.append(
            f"| {case} | {cell(first_golden[case])}; {first_losses}/6 losses | "
            f"{cell(entry_hot[case])}; {entry_losses}/6 losses |"
        )
    lines += ["", f"Campaign paths, identities, paired vectors, and SHA256 input digests are recorded in `{OUTPUT}`.", ""]
    return "\n".join(lines)


def main():
    campaigns = {label: load_campaign(label, config) for label, config in CAMPAIGNS.items()}
    OUTPUT.write_text(json.dumps({"campaigns": campaigns}, indent=2, sort_keys=True) + "\n")
    (Path(__file__).with_name("benchmark-tables.md")).write_text(render(campaigns))


if __name__ == "__main__":
    main()
