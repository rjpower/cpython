#!/usr/bin/env python3
"""Validate and summarize the calls/frame/publication throughput campaign.

This reads retained campaign and baseline JSON only. It writes derived JSON
and Markdown into a new directory supplied with --output.
"""

from __future__ import annotations

import argparse
import json
import math
import statistics
import sys
from pathlib import Path
from typing import Any

ROLES = ("upstream", "interim", "candidate")
STAT_ROLES = ("upstream", "candidate")
KERNEL_COUNT = 29
PY_PERF_COUNT = 12
STAT_CASES = ("deltablue", "logging_silent", "comprehensions")
EVENTS = {
    "cycles-instructions": ("cycles:u", "instructions:u"),
    "branches": ("branches:u", "branch-misses:u"),
    "cache": ("cache-references:u", "cache-misses:u"),
}


class InvalidData(ValueError):
    """Input evidence is incomplete, inconsistent or malformed."""


def read_json(path: Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise InvalidData(f"cannot read JSON {path}: {exc}") from exc


def finite_number(value: Any, context: str, *, positive: bool = True,
                  nonnegative: bool = False) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise InvalidData(f"{context}: expected a number")
    value = float(value)
    if (not math.isfinite(value) or (positive and value <= 0)
            or (nonnegative and value < 0)):
        qualifier = "nonnegative" if nonnegative else "positive"
        raise InvalidData(f"{context}: expected a finite {qualifier} number")
    return value


def geometric_mean(values: list[float], context: str) -> float:
    if not values or any(not math.isfinite(x) or x <= 0 for x in values):
        raise InvalidData(f"{context}: geometric mean requires positive finite values")
    return math.exp(statistics.mean(math.log(x) for x in values))


def stages(root: Path) -> dict[str, Path]:
    """Resolve the two retained campaign layouts by their documented names."""
    if root.name == "checkpoint-closure":
        result = {
            "kernels": root / "measure-6a0e4c12c17-diagnostic-v3",
            "pyperf": root / "pyperformance-6a0e4c12c17-diagnostic",
            "perf": root / "perf-stat-6a0e4c12c17",
        }
    else:
        result = {name: root / child for name, child in (
            ("kernels", "kernels"), ("pyperf", "pyperformance"),
            ("perf", "perf-stat"))}
    missing = [str(path) for path in result.values() if not path.is_dir()]
    if missing:
        raise InvalidData(f"missing explicit stage directories: {missing}")
    return result


def unique_file(root: Path, name: str) -> Path:
    path = root / name
    if not path.is_file():
        raise InvalidData(f"missing {path}")
    return path


def identity_from_preflight(root: Path) -> dict[str, dict[str, Any]]:
    before = unique_file(root, "preflight-before.json")
    after = unique_file(root, "preflight-after.json")
    a, b = read_json(before), read_json(after)
    if a.get("errors") or b.get("errors"):
        raise InvalidData(f"preflight has errors under {root}")
    builds_a, builds_b = a.get("builds"), b.get("builds")
    if not isinstance(builds_a, dict) or not isinstance(builds_b, dict):
        raise InvalidData(f"preflight build identities missing under {root}")
    result: dict[str, dict[str, Any]] = {}
    for role in ROLES:
        try:
            head_a = builds_a[role]["git"]["head"]
            head_b = builds_b[role]["git"]["head"]
            status_a = builds_a[role]["git"].get("status", "")
            status_b = builds_b[role]["git"].get("status", "")
        except (KeyError, TypeError) as exc:
            raise InvalidData(f"missing {role} identity under {root}") from exc
        if head_a != head_b or status_a != status_b or status_a:
            raise InvalidData(f"{role} identity changed or was dirty under {root}")
        probe_a = builds_a[role].get("probe", {}).get("git")
        probe_b = builds_b[role].get("probe", {}).get("git")
        if (not isinstance(probe_a, list) or probe_a != probe_b or len(probe_a) != 3
                or not head_a.startswith(probe_a[2])):
            raise InvalidData(f"{role} source/probe identity mismatch under {root}")
        result[role] = {"head": head_a, "probe_git": probe_a}
    return result


def find_kernel_reports(root: Path) -> list[Path]:
    return sorted(root.glob("r[123]-*-*.json"))


def validate_kernel_identity(root: Path, identities: dict[str, dict[str, Any]],
                              medians: dict[str, Any]) -> None:
    paths = find_kernel_reports(root)
    expected = {(r, k, role) for r in range(1, 4)
                for k in medians["candidate"]
                for role in ROLES}
    observed: set[tuple[int, str, str]] = set()
    for path in paths:
        data = read_json(path)
        stem = path.stem
        try:
            round_s, case, role = stem.split("-", 2)
            round_no = int(round_s[1:])
        except (ValueError, IndexError) as exc:
            raise InvalidData(f"bad kernel report filename {path.name}") from exc
        if role not in ROLES:
            raise InvalidData(f"unexpected kernel role in {path.name}")
        cell = (round_no, case, role)
        if cell in observed:
            raise InvalidData(f"duplicate kernel cell {cell}")
        observed.add(cell)
        try:
            build = data["build"]
            build_git = build["git"]
            label = build["label"]
            raw = data["results"]["time"][case]["value"]
        except (KeyError, TypeError, IndexError) as exc:
            raise InvalidData(f"incomplete kernel report {path}") from exc
        if label != role or build_git != identities[role]["probe_git"]:
            raise InvalidData(f"kernel identity mismatch in {path}")
        raw_value = finite_number(raw, str(path))
        if not math.isclose(raw_value, medians[role][case][round_no - 1], rel_tol=1e-12):
            raise InvalidData(f"kernel raw/round-median mismatch in {path}")
    if observed != expected:
        missing, extra = sorted(expected - observed)[:3], sorted(observed - expected)[:3]
        raise InvalidData(f"kernel report matrix mismatch; missing={missing}, extra={extra}")


def kernel_names_from_medians(root: Path) -> set[str]:
    data = read_json(root / "round-medians.json")
    validate_roles(data, ROLES, "kernel medians")
    names: set[str] | None = None
    for role in ROLES:
        if not isinstance(data[role], dict):
            raise InvalidData(f"{role} kernel medians are not a mapping")
        current = set(data[role])
        names = current if names is None else names
        if current != names:
            raise InvalidData("kernel roster differs between roles")
        for case, values in data[role].items():
            if not isinstance(values, list) or len(values) != 3:
                raise InvalidData(f"{role}/{case}: expected three round values")
            for index, value in enumerate(values, 1):
                finite_number(value, f"{role}/{case}/round{index}")
    if names is None or len(names) != KERNEL_COUNT:
        raise InvalidData(f"expected {KERNEL_COUNT} kernels, got {len(names or ())}")
    return names


def validate_roles(data: Any, roles: tuple[str, ...], context: str) -> None:
    if not isinstance(data, dict) or set(data) != set(roles):
        raise InvalidData(f"{context}: expected roles {roles}")


def summarize_kernels(root: Path) -> dict[str, Any]:
    raw = read_json(root / "round-medians.json")
    validate_roles(raw, ROLES, "kernel medians")
    names = kernel_names_from_medians(root)
    identities = identity_from_preflight(root)
    validate_kernel_identity(root, identities, raw)
    case_rows = {}
    for case in sorted(names):
        medians = {role: statistics.median(raw[role][case]) for role in ROLES}
        case_rows[case] = {
            "medians_ns_per_loop": medians,
            "candidate_over_upstream": medians["candidate"] / medians["upstream"],
            "candidate_over_interim": medians["candidate"] / medians["interim"],
            "current_candidate_over_old_candidate": None,
        }
    return {"identities": identities, "round_vectors": raw, "cases": case_rows}


def summarize_pyperf(pyroot: Path, expected_identities: dict[str, dict[str, Any]]) -> dict[str, Any]:
    aggregate = read_json(pyroot / "aggregate.json")
    validate_roles(aggregate, ROLES, "pyperformance aggregate")
    expected_names = set(aggregate["upstream"])
    if len(expected_names) != PY_PERF_COUNT or any(
            set(aggregate[role]) != expected_names for role in ROLES):
        raise InvalidData("pyperformance roster must be the same 12 results for all roles")
    vectors: dict[str, dict[str, list[float]]] = {role: {} for role in ROLES}
    identities: dict[str, set[str]] = {role: set() for role in ROLES}
    file_names = sorted(p.name for p in (pyroot / "pyperf").glob("*.json")
                        if p.name.endswith(".json") and p.name.split("-", 1)[0] in ROLES)
    if len(file_names) != len(ROLES) * 8:
        raise InvalidData(f"expected 8 pyperf module files per role, found {len(file_names)}")
    for role in ROLES:
        seen_names: set[str] = set()
        paths = sorted((pyroot / "pyperf").glob(f"{role}-*.json"))
        if len(paths) != 8:
            raise InvalidData(f"{role}: expected 8 pyperf module files, found {len(paths)}")
        for path in paths:
            doc = read_json(path)
            try:
                bench = doc["benchmarks"]
                common = doc["metadata"]
            except (KeyError, TypeError) as exc:
                raise InvalidData(f"incomplete pyperf report {path}") from exc
            for item in bench:
                try:
                    benchmark_metadata = item.get("metadata", {})
                    metadata = {**common, **benchmark_metadata}
                    name, version = metadata["name"], common["python_version"]
                    values = [v for run in item["runs"]
                              for v in run.get("values", [])]
                except (KeyError, TypeError) as exc:
                    raise InvalidData(f"incomplete benchmark in {path}") from exc
                if name not in expected_names or name in seen_names:
                    raise InvalidData(f"unexpected or duplicate pyperf result {role}/{name}")
                if len(values) != 18:
                    raise InvalidData(f"{role}/{name}: expected 18 timed values, found {len(values)}")
                seen_names.add(name)
                vectors[role][name] = [finite_number(v, f"{role}/{name}") for v in values]
                identities[role].add(version)
                recorded = aggregate[role][name]
                mean = statistics.mean(vectors[role][name])
                if recorded.get("values") != 18 or not math.isclose(
                        finite_number(recorded.get("mean"), f"{role}/{name} mean"),
                        mean, rel_tol=1e-12):
                    raise InvalidData(f"aggregate mean/count disagrees with vectors: {role}/{name}")
        if seen_names != expected_names:
            raise InvalidData(f"{role}: pyperf results do not match 12-result roster")
    if any(len(v) != 1 for v in identities.values()):
        raise InvalidData("pyperformance role uses mixed Python identities")
    for role, versions in identities.items():
        sha = expected_identities[role]["probe_git"][2]
        if any(f"revision {sha}" not in version for version in versions):
            raise InvalidData(f"pyperformance version does not identify {role} build")
    result_means = {role: {name: statistics.mean(vec)
                           for name, vec in vectors[role].items()}
                    for role in ROLES}
    ratios = {}
    for reference in ("upstream", "interim"):
        rows = {name: result_means["candidate"][name] / result_means[reference][name]
                for name in sorted(expected_names)}
        ratios[f"candidate_over_{reference}"] = {
            "per_result": rows, "geomean": geometric_mean(list(rows.values()), reference)}
    return {"identities": {k: sorted(v) for k, v in identities.items()},
            "vectors_seconds": vectors, "means_seconds": result_means,
            "ratios": ratios}


def summarize_perf(perf_root: Path) -> dict[str, Any]:
    cells: dict[tuple[int, str, str, str], dict[str, float]] = {}
    all_reports = [p for p in perf_root.glob("*result.json")]
    for path in all_reports:
        data = read_json(path)
        if data.get("mode") != "stat":
            continue
        if data.get("calibration") or not data.get("valid"):
            raise InvalidData(f"invalid perf-stat result: {path}")
        if data.get("errors"):
            raise InvalidData(f"perf-stat errors in {path}: {data['errors']}")
        role, case, round_no, group = (data.get("role"), data.get("case"),
                                      data.get("round"), data.get("event_group"))
        if role not in STAT_ROLES or case not in STAT_CASES or group not in EVENTS:
            raise InvalidData(f"unexpected perf-stat cell in {path}")
        key = (round_no, role, case, group)
        if key in cells:
            raise InvalidData(f"duplicate perf-stat cell {key}")
        validation = data.get("stat_validation")
        if not isinstance(validation, dict) or not validation.get("valid"):
            raise InvalidData(f"invalid event validation in {path}")
        evs = validation.get("events")
        expected_events = EVENTS[group]
        if not isinstance(evs, list) or {e.get("event") for e in evs} != set(expected_events):
            raise InvalidData(f"event set mismatch in {path}")
        values = {}
        for item in evs:
            if not item.get("valid"):
                raise InvalidData(f"invalid event row in {path}")
            values[item["event"]] = finite_number(
                item.get("count_per_loop"), str(path), positive=False, nonnegative=True)
        cells[key] = values
    expected = {(r, role, case, group) for r in range(1, 4)
                for role in STAT_ROLES for case in STAT_CASES for group in EVENTS}
    if set(cells) != expected:
        raise InvalidData(f"perf-stat matrix mismatch: expected {len(expected)}, got {len(cells)}")
    by_round = {}
    for case in STAT_CASES:
        by_round[case] = {role: {} for role in STAT_ROLES}
        for role in STAT_ROLES:
            event_values = {}
            for group, names in EVENTS.items():
                for event in names:
                    event_values[event] = [cells[(rnd, role, case, group)][event]
                                           for rnd in range(1, 4)]
            medians = {event: statistics.median(values)
                       for event, values in event_values.items()}
            by_round[case][role] = {"per_loop_by_round": event_values,
                                    "median_per_loop": medians,
                                    "cpi_ratio_of_medians":
                                        medians["cycles:u"] / medians["instructions:u"]}
    ratios = {}
    for case in STAT_CASES:
        c = by_round[case]["candidate"]
        ratios[case] = {}
        for reference in STAT_ROLES:
            if reference == "candidate":
                continue
            r = by_round[case][reference]
            event_ratios = {event: c["median_per_loop"][event] / r["median_per_loop"][event]
                            for event in c["median_per_loop"]}
            ratios[case][f"candidate_over_{reference}"] = {
                "events": event_ratios,
                "cpi": c["cpi_ratio_of_medians"] / r["cpi_ratio_of_medians"]}
    return {"valid_invocations": len(cells), "cases": by_round, "ratios": ratios}


def add_sensitivity(current: dict[str, Any], old: dict[str, Any]) -> None:
    for case, row in current["kernels"]["cases"].items():
        row["current_candidate_over_old_candidate"] = (
            row["medians_ns_per_loop"]["candidate"] /
            old["kernels"]["cases"][case]["medians_ns_per_loop"]["candidate"])
    current["kernels"]["current_over_old_geomean"] = geometric_mean(
        [r["current_candidate_over_old_candidate"]
         for r in current["kernels"]["cases"].values()], "current/old kernels")
    current["pyperformance"]["current_over_old_candidate_geomean"] = geometric_mean(
        [current["pyperformance"]["means_seconds"]["candidate"][n] /
         old["pyperformance"]["means_seconds"]["candidate"][n]
         for n in current["pyperformance"]["means_seconds"]["candidate"]],
        "current/old pyperformance")


def analyze(root: Path) -> dict[str, Any]:
    stage = stages(root)
    kernels = summarize_kernels(stage["kernels"])
    return {"kernels": kernels,
            "pyperformance": summarize_pyperf(stage["pyperf"], kernels["identities"]),
            "perf_stat": summarize_perf(stage["perf"])}


def format_report(current: dict[str, Any], old: dict[str, Any]) -> str:
    lines = ["# Throughput results analysis", "",
             "Derived from retained raw JSON; lower elapsed-time ratios favor the candidate.",
             "Current/old candidate ratios are a sensitivity comparison, not causal attribution.", "",
             "## 29 kernels", "", "| Kernel | C/U | C/I | Current C / old C |",
             "|---|---:|---:|---:|"]
    for name, row in sorted(current["kernels"]["cases"].items()):
        lines.append(f"| {name} | {row['candidate_over_upstream']:.6f} | "
                     f"{row['candidate_over_interim']:.6f} | "
                     f"{row['current_candidate_over_old_candidate']:.6f} |")
    lines += [f"| Geomean | {geometric_mean([r['candidate_over_upstream'] for r in current['kernels']['cases'].values()], 'kernel C/U'):.8f} | "
              f"{geometric_mean([r['candidate_over_interim'] for r in current['kernels']['cases'].values()], 'kernel C/I'):.8f} | "
              f"{current['kernels']['current_over_old_geomean']:.8f} |", "",
              "## Pyperformance subset", "",
              "Geometric means use the existing equal-weight ratio of per-result arithmetic means (18 values per result).", "",
              "| Comparison | Geomean elapsed ratio |", "|---|---:|"]
    for key, value in current["pyperformance"]["ratios"].items():
        lines.append(f"| {key} | {value['geomean']:.8f} |")
    lines.append(f"| current candidate / old candidate (sensitivity) | "
                 f"{current['pyperformance']['current_over_old_candidate_geomean']:.8f} |")
    lines += ["", "## Hardware counters", "",
              "Median normalized event counts use three rounds; CPI is the ratio of median cycles to median instructions.", "",
              "| Case | Instr C/U | Instr C/U old6a | CPI C/U | CPI C/U old6a |",
              "|---|---:|---:|---:|---:|"]
    for case in STAT_CASES:
        cu = current["perf_stat"]["ratios"][case]["candidate_over_upstream"]
        ou = old["perf_stat"]["ratios"][case]["candidate_over_upstream"]
        lines.append(f"| {case} | {cu['events']['instructions:u']:.6f} | "
                     f"{ou['events']['instructions:u']:.6f} | {cu['cpi']:.6f} | {ou['cpi']:.6f} |")
    lines += ["", "This report does not assign causality to current/old ratios.", ""]
    return "\n".join(lines)


def validate_known_baseline(old: dict[str, Any], self_compare: dict[str, Any]) -> None:
    expected = {
        "kernel_geomean_c_u": 1.2867708613,
        "subset_geomean_c_u": 1.09643692,
        "instructions_deltablue_c_u": 2.046606,
        "instructions_logging_silent_c_u": 1.903192,
        "instructions_comprehensions_c_u": 1.628162,
    }
    actual = {
        "kernel_geomean_c_u": geometric_mean(
            [r["candidate_over_upstream"] for r in old["kernels"]["cases"].values()], "old kernel"),
        "subset_geomean_c_u": old["pyperformance"]["ratios"]["candidate_over_upstream"]["geomean"],
        **{f"instructions_{case}_c_u": old["perf_stat"]["ratios"][case]["candidate_over_upstream"]["events"]["instructions:u"]
           for case in STAT_CASES},
    }
    for key, value in expected.items():
        if not math.isclose(actual[key], value, rel_tol=2e-6, abs_tol=2e-6):
            raise InvalidData(f"baseline self-check {key}: {actual[key]} != {value}")
    if not math.isclose(self_compare["kernels"]["current_over_old_geomean"], 1.0,
                        rel_tol=1e-12):
        raise InvalidData("baseline self-comparison kernel ratio is not 1")
    if not math.isclose(self_compare["pyperformance"]["current_over_old_candidate_geomean"],
                        1.0, rel_tol=1e-12):
        raise InvalidData("baseline self-comparison pyperformance ratio is not 1")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--campaign", type=Path, help="current campaign root")
    parser.add_argument("--baseline-root", type=Path, required=True,
                        help="checkpoint-closure root containing frozen 6a data")
    parser.add_argument("--output", type=Path, required=True,
                        help="new, empty output directory for derived files")
    parser.add_argument("--self-check", action="store_true",
                        help="analyze old baseline as both current and baseline")
    args = parser.parse_args(argv)
    try:
        if args.self_check:
            old = analyze(args.baseline_root)
            current = analyze(args.baseline_root)
            add_sensitivity(current, old)
            validate_known_baseline(old, current)
        else:
            if args.campaign is None:
                parser.error("--campaign is required unless --self-check is used")
            old = analyze(args.baseline_root)
            current = analyze(args.campaign)
            add_sensitivity(current, old)
        args.output.mkdir(parents=True, exist_ok=False)
        doc = {"current": current, "old_6a": old,
               "limitations": ["current/old ratios are sensitivity checks, not causal attribution",
                               "no score filtering; incomplete or invalid cells reject the report"]}
        (args.output / "analysis.json").write_text(json.dumps(doc, indent=2) + "\n",
                                                    encoding="utf-8")
        (args.output / "analysis.md").write_text(format_report(current, old),
                                                   encoding="utf-8")
        print(f"wrote derived analysis to {args.output}")
    except (InvalidData, OSError) as exc:
        print(f"analysis failed: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
