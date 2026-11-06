"""Run the checkpoint's three interleaved timing rounds after qualification.

Run through uv with --candidate BUILD --output NEW_DIRECTORY and an explicit
--qualification qualified|diagnostic label. Qualified mode verifies frozen
source/build identities before and after timing. Diagnostic results are never
R27 admission evidence. This script does not run the full suite or RSS stage.
"""

import argparse
import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path
import re
import statistics
import subprocess
import sys
import time


BUILD_ROOT = Path(
    "/home/power/code/cpython/.worktrees/python-boil-the-ocean/build")
CONTROL_HEADS = {
    "upstream": "3a440b65695",
    "interim": "eba351fee7d",
}
EXPECTED_MODULE_COUNTS = {"upstream": 116, "interim": 116, "candidate": 117}
PROBE_MODULES = ("json", "encodings", "_decimal", "_ssl", "_sqlite3",
                 "_lzma", "_ctypes")
def run(cmd: list[str], *, env: dict[str, str] | None = None) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, check=False, capture_output=True, text=True, env=env)


def resolve_build(path: Path) -> tuple[Path, Path]:
    build = path.resolve()
    python = build / "python" if build.is_dir() else build
    if not python.is_file() or not os.access(python, os.X_OK):
        raise ValueError(f"not an executable interpreter/build: {python}")
    return python.parent, python.resolve()


def source_tree_for(build_dir: Path) -> tuple[Path, Path]:
    makefile = build_dir / "Makefile"
    if not makefile.is_file():
        raise ValueError(f"missing configured Makefile: {makefile}")
    match = re.search(r"(?m)^srcdir\s*=\s*(.*?)\s*$", makefile.read_text())
    if not match or not match.group(1):
        raise ValueError(f"Makefile has no concrete srcdir: {makefile}")
    srcdir = Path(match.group(1))
    source = (build_dir / srcdir).resolve() if not srcdir.is_absolute() else srcdir.resolve()
    if not (source / ".git").exists() and not (source / "pyproject.toml").exists():
        raise ValueError(f"srcdir does not look like a source checkout: {source}")
    return source, makefile


def git_identity(source: Path) -> dict:
    head = run(["git", "-C", str(source), "rev-parse", "HEAD"])
    if head.returncode:
        raise ValueError(f"cannot read source HEAD at {source}: {head.stderr.strip()}")
    status = run(["git", "-C", str(source), "status", "--porcelain=v1",
                  "--untracked-files=all"])
    if status.returncode:
        raise ValueError(f"cannot read source status at {source}: {status.stderr.strip()}")
    return {"head": head.stdout.strip(), "status": status.stdout}


def elf_build_id(path: Path) -> str | None:
    proc = run(["readelf", "-n", str(path)])
    if proc.returncode:
        return None
    match = re.search(r"Build ID:\s*([0-9a-fA-F]+)", proc.stdout)
    return match.group(1).lower() if match else None


def _probe_source() -> str:
    return r'''import importlib, json, os, sys, sysconfig
modules = {}
for name in ("json", "encodings", "_decimal", "_ssl", "_sqlite3", "_lzma", "_ctypes"):
    try:
        mod = importlib.import_module(name)
        spec = getattr(mod, "__spec__", None)
        filename = getattr(mod, "__file__", None)
        modules[name] = {"file": os.path.abspath(filename) if filename else None,
                         "origin": getattr(spec, "origin", None), "error": None}
    except BaseException as exc:
        modules[name] = {"file": None, "origin": None,
                         "error": f"{type(exc).__name__}: {exc}"}
jit = getattr(sys, "_jit", None)
def jit_method(name):
    fn = getattr(jit, name, None)
    return fn() if callable(fn) else None
method_jit = {"available": False, "natural_admission": None}
try:
    import _testinternalcapi
    fn = getattr(_testinternalcapi, "jit_method_natural", None)
    if callable(fn):
        old = fn(False)
        try:
            method_jit = {"available": True, "natural_admission": bool(old)}
        finally:
            fn(old)
except ImportError:
    pass
config = {key: sysconfig.get_config_var(key) for key in (
    "ABIFLAGS", "CC", "CFLAGS", "CONFIG_ARGS", "OPT", "Py_DEBUG",
    "Py_TRACE_REFS", "SOABI", "WITH_MIMALLOC", "WITH_PYMALLOC")}
print(json.dumps({"version": sys.version, "git": list(getattr(sys, "_git", ())),
                  "debug_flag": sys.flags.debug, "config": config,
                  "jit": {"available": jit_method("is_available"),
                          "enabled": jit_method("is_enabled")},
                  "method_jit": method_jit, "modules": modules}))'''


def interpreter_probe(python: Path, env: dict[str, str]) -> dict:
    proc = run([str(python), "-S", "-c", _probe_source()], env=env)
    if proc.returncode:
        raise ValueError(f"interpreter probe failed for {python}: {proc.stderr.strip()}")
    try:
        return json.loads(proc.stdout)
    except json.JSONDecodeError as exc:
        raise ValueError(f"invalid interpreter probe output for {python}") from exc


def module_log_info(label: str, path: Path) -> dict:
    path = path.resolve()
    content = path.read_bytes()
    text = content.decode("utf-8", errors="replace")
    rows = re.findall(
        r"Checked (\d+) modules \((\d+) built-in, (\d+) shared, "
        r"(\d+) n/a on [^,]+, (\d+) disabled, (\d+) missing, "
        r"(\d+) failed on import\)", text)
    if len(rows) != 1:
        raise ValueError(f"expected exactly one module-check summary in {path}")
    counts = [int(value) for value in rows[0]]
    return {"path": str(path), "sha256": hashlib.sha256(content).hexdigest(),
            "modules": counts[0], "built_in": counts[1], "shared": counts[2],
            "not_available": counts[3], "disabled": counts[4],
            "missing": counts[5], "failed_import": counts[6],
            "expected_modules": EXPECTED_MODULE_COUNTS[label]}


def is_within(path: Path, roots: tuple[Path, ...]) -> bool:
    resolved = path.resolve()
    return any(resolved == root or root in resolved.parents for root in roots)


def capture_build(label: str, build_path: Path, env: dict[str, str],
                  module_log: Path | None) -> dict:
    build_dir, python = resolve_build(build_path)
    source, makefile = source_tree_for(build_dir)
    git = git_identity(source)
    probe = interpreter_probe(python, env)
    ext_roots = (build_dir, source)
    module_records = probe["modules"]
    for name, record in module_records.items():
        origin = record["file"]
        if origin and Path(origin).suffix in (".so", ".pyd"):
            record["build_id"] = elf_build_id(Path(origin))
        else:
            record["build_id"] = None
    return {
        "role": label, "build_dir": str(build_dir), "python": str(python),
        "source": str(source), "git": git,
        "makefile": str(makefile),
        "makefile_sha256": hashlib.sha256(makefile.read_bytes()).hexdigest(),
        "python_build_id": elf_build_id(python),
        "probe": probe,
        "extension_roots": [str(path) for path in ext_roots],
        "module_log": module_log_info(label, module_log) if module_log else None,
    }


def validate_snapshot(snapshot: dict, label: str, qualification: str) -> list[str]:
    errors = []
    git = snapshot["git"]
    probe = snapshot["probe"]
    if qualification == "qualified":
        if git["status"].strip():
            errors.append(f"{label}: source tree is dirty")
        revision = probe["git"][2] if len(probe["git"]) > 2 else None
        if (not isinstance(revision, str)
                or not re.fullmatch(r"[0-9a-fA-F]{7,40}", revision)):
            errors.append(f"{label}: sys._git does not identify a source revision")
        elif not (git["head"].startswith(revision) or revision.startswith(git["head"])):
            errors.append(
                f"{label}: executable revision {revision} does not match "
                f"source HEAD {git['head']}")
        expected = CONTROL_HEADS.get(label)
        if expected and not git["head"].startswith(expected):
            errors.append(f"{label}: expected HEAD {expected}, got {git['head']}")
        if snapshot["python_build_id"] is None:
            errors.append(f"{label}: interpreter ELF has no Build ID")
        if probe["debug_flag"] != 0 or probe["config"].get("Py_DEBUG") != 0:
            errors.append(f"{label}: not a release build")
        if probe["jit"].get("enabled") is not False:
            errors.append(f"{label}: sys._jit is not confirmed disabled")
        if label == "candidate" and probe["method_jit"].get("natural_admission") is True:
            errors.append("candidate: fork method-JIT natural admission is enabled")
        for name, module in probe["modules"].items():
            if module["error"]:
                errors.append(f"{label}: import {name} failed: {module['error']}")
            origin = module["file"]
            if not origin:
                errors.append(f"{label}: import {name} has no module file")
                continue
            if name in ("json", "encodings"):
                if Path(origin).suffix != ".py" or not is_within(
                        Path(origin), (Path(snapshot["source"]) / "Lib",)):
                    errors.append(f"{label}: {name} did not resolve from this source Lib: {origin}")
                continue
            if Path(origin).suffix != ".so":
                errors.append(f"{label}: {name} did not resolve to a shared extension: {origin}")
            if origin and Path(origin).suffix in (".so", ".pyd"):
                if not is_within(Path(origin), tuple(map(Path, snapshot["extension_roots"]))):
                    errors.append(f"{label}: {name} resolved outside build/source: {origin}")
                if module["build_id"] is None:
                    errors.append(f"{label}: {name} ELF has no Build ID")
        module_log = snapshot["module_log"]
        if module_log is None:
            errors.append(f"{label}: no immutable module-check log supplied")
        else:
            expected_count = EXPECTED_MODULE_COUNTS[label]
            if module_log["modules"] != expected_count:
                errors.append(f"{label}: expected module roster {expected_count}, got {module_log['modules']}")
            if module_log["missing"] or module_log["failed_import"]:
                errors.append(f"{label}: module check reports missing/failed imports")
    return errors


def parse_module_logs(values: list[str]) -> dict[str, Path]:
    logs = {}
    for value in values:
        if "=" not in value:
            raise ValueError("module logs must be ROLE=PATH")
        role, path = value.split("=", 1)
        if role not in EXPECTED_MODULE_COUNTS or role in logs:
            raise ValueError(f"invalid or duplicate module-log role: {role}")
        logs[role] = Path(path)
    return logs


def snapshot_all(builds: dict[str, Path], env: dict[str, str],
                 logs: dict[str, Path]) -> dict:
    snapshots = {}
    errors = []
    for label, build_path in builds.items():
        try:
            snapshot = capture_build(label, build_path, env, logs.get(label))
            snapshots[label] = snapshot
            errors.extend(validate_snapshot(snapshot, label, "qualified"))
        except (OSError, ValueError) as exc:
            snapshots[label] = {"error": str(exc)}
            errors.append(f"{label}: {exc}")
    return {"builds": snapshots, "errors": errors}


def identity_differences(before: dict, after: dict) -> dict:
    """Return changed build snapshots, retaining both full snapshots elsewhere."""
    differences = {}
    before_builds = before.get("builds", {})
    after_builds = after.get("builds", {})
    for label in sorted(before_builds.keys() | after_builds.keys()):
        old = before_builds.get(label)
        new = after_builds.get(label)
        if old != new:
            differences[label] = {"before": old, "after": new}
    return differences


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidate", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--qualification", required=True,
                        choices=("qualified", "diagnostic"))
    parser.add_argument("--module-log", action="append", default=[],
                        metavar="ROLE=PATH",
                        help="immutable output of the build's module check; repeat for all roles")
    parser.add_argument("--preflight-only", action="store_true",
                        help="write preflight evidence without running timing rounds")
    args = parser.parse_args(argv)
    args.output.mkdir(parents=True, exist_ok=False)
    logs = parse_module_logs(args.module_log)

    registry = Path(__file__).resolve().parents[2] / "bench/run.py"
    spec = importlib.util.spec_from_file_location("checkpoint_bench", registry)
    bench = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = bench
    spec.loader.exec_module(bench)
    kernels = [b.name for b in bench.BENCHMARKS if b.group == "kernels"]
    if len(kernels) != 29 or len(set(kernels)) != 29:
        raise RuntimeError("checkpoint roster is no longer exactly 29 kernels")
    builds = {
        "upstream": BUILD_ROOT / "upstream-release-devlibs-3a440b6",
        "interim": BUILD_ROOT / "interim-release-devlibs-eba351",
        "candidate": args.candidate.resolve(),
    }
    before = snapshot_all(builds, bench.CHILD_ENV, logs)
    before["qualification"] = args.qualification
    before["admission_note"] = (
        "Diagnostic results are not R27 admission evidence."
        if args.qualification == "diagnostic" else
        "Timing does not itself establish R27 admission.")
    (args.output / "preflight-before.json").write_text(json.dumps(before, indent=2))
    if args.qualification == "qualified" and before["errors"]:
        (args.output / "qualification-status.json").write_text(json.dumps({
            "qualification": args.qualification, "eligible": False,
            "phase": "preflight", "errors": before["errors"]}, indent=2))
        raise SystemExit("qualified preflight failed; timing was not started")
    if args.preflight_only:
        (args.output / "qualification-status.json").write_text(json.dumps({
            "qualification": args.qualification,
            "preflight_ok": not before["errors"],
            "eligible": False,
            "phase": "preflight-only", "errors": before["errors"]}, indent=2))
        return 0

    labels = list(builds)
    results = {label: {name: [] for name in kernels} for label in labels}
    conditions = []
    failure = None
    deadline = time.monotonic() + 45 * 60
    try:
        for round_index in range(3):
            for kernel_index, name in enumerate(kernels):
                rotation = (round_index + kernel_index) % len(labels)
                order = labels[rotation:] + labels[:rotation]
                for label in order:
                    if time.monotonic() >= deadline:
                        raise RuntimeError("declared 45-minute timing window expired")
                    path = args.output / f"r{round_index + 1}-{name}-{label}.json"
                    command = [sys.executable, "-B", str(registry), "--build",
                               str(builds[label]), "--label", label, "--set", "kernels",
                               "--only", name, "--metric", "time", "--max-load", "4",
                               "-o", str(path)]
                    conditions.append({"round": round_index + 1, "kernel": name,
                                       "label": label, "load": os.getloadavg(),
                                       "command": command})
                    journal = {
                        "qualification": args.qualification,
                        "admission_note": before["admission_note"],
                        "preflight_before": "preflight-before.json",
                        "invocations": conditions,
                    }
                    (args.output / "invocations.json").write_text(
                        json.dumps(journal, indent=2))
                    subprocess.run(command, check=True)
                    data = json.loads(path.read_text())
                    results[label][name].append(data["results"]["time"][name]["value"])
                    print(f"round {round_index + 1}: {name} {label}", flush=True)
    except BaseException as exc:
        failure = f"{type(exc).__name__}: {exc}"
        raise
    finally:
        after = snapshot_all(builds, bench.CHILD_ENV, logs)
        differences = identity_differences(before, after)
        unchanged = not differences
        status = {
            "qualification": args.qualification,
            "eligible": args.qualification == "qualified" and unchanged
                        and not before["errors"] and not after["errors"] and failure is None,
            "phase": "postflight" if failure is None else "timing-failed-postflight",
            "identity_unchanged": unchanged,
            "identity_differences": differences,
            "timing_error": failure,
            "preflight_errors": before["errors"],
            "postflight_errors": after["errors"],
        }
        (args.output / "preflight-after.json").write_text(json.dumps(after, indent=2))
        (args.output / "qualification-status.json").write_text(json.dumps(status, indent=2))

    (args.output / "round-medians.json").write_text(json.dumps(results, indent=2))
    rows = [
        f"Qualification: {args.qualification}. "
        + ("DIAGNOSTIC ONLY — NOT R27 ADMISSION.\n" if args.qualification == "diagnostic"
           else "Timing does not itself establish R27 admission.\n"),
        "Ratios are baseline elapsed / candidate elapsed; above 1 favors candidate.\n",
        "| Kernel | Candidate ns/loop | Upstream/Candidate | Interim/Candidate | Candidate spread |",
        "| --- | ---: | ---: | ---: | ---: |",
    ]
    ratios = {label: [] for label in ("upstream", "interim")}
    for name in kernels:
        medians = {label: statistics.median(results[label][name]) for label in labels}
        c = medians["candidate"]
        for label in ratios:
            ratios[label].append(medians[label] / c)
        samples = results["candidate"][name]
        spread = 100 * (max(samples) / min(samples) - 1)
        rows.append(f"| {name} | {c:,.0f} | {ratios['upstream'][-1]:.3f} | "
                    f"{ratios['interim'][-1]:.3f} | {spread:.1f}% |")
    rows.append("| Geomean | | " + " | ".join(
        f"{math.exp(statistics.mean(map(math.log, ratios[label]))):.3f}"
        for label in ratios) + " | |")
    if (not unchanged or before["errors"] or after["errors"] or failure is not None):
        rows.insert(1, "**Not qualified:** identity/preflight or postflight failed.\n")
    (args.output / "summary.md").write_text("\n".join(rows) + "\n")
    return 0 if status["eligible"] or args.qualification == "diagnostic" else 1


if __name__ == "__main__":
    raise SystemExit(main())
