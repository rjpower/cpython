"""Diagnostic-only pyperformance subset and RSS companion runner.

This script prepares reproducible commands and evidence.  Pyperformance runs
are diagnostic and cannot qualify R27.  The ``pyperf`` and ``rss`` stages are
separate invocations; do not overlap them or other builds/benchmarks.
"""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shlex
import signal
import subprocess
import sys
import time


HERE = Path(__file__).resolve().parent
MEASURE_PATH = HERE.parent / "issues/checkpoint-closure/measure.py"
DEFAULT_PYPERF_ROOT = Path("/home/power/.cache/uv/archive-v0/oE61rffWrav46JbH")
DEFAULT_BENCH_ROOT = Path(
    "/home/power/.cache/uv/archive-v0/xitntS2hDvEA0_ce/pyperformance/data-files/benchmarks")
ROLE_NAMES = ("upstream", "interim", "candidate")
PY_PERF_WINDOW_SECONDS = 30 * 60
LOAD_LIMIT = 4.0
PAGE_SIZE = os.sysconf("SC_PAGE_SIZE")

# One script invocation per listed module; SciMark emits five named results.
TASKS = (
    ("hexiom", "bm_hexiom", "hexiom", ()),
    ("pyflate", "bm_pyflate", "pyflate", ()),
    ("scimark", "bm_scimark", "scimark", ()),
    ("regex_v8", "bm_regex_v8", "regex_v8", ()),
    ("mdp", "bm_mdp", "mdp", ()),
    ("async_tree_none", "bm_async_tree", "async_tree", ("none",)),
    ("unpickle_pure_python", "bm_pickle", "pickle",
     ("--pure-python", "unpickle")),
    ("typing_runtime_protocols", "bm_typing_runtime_protocols", "typing", ()),
)
EXPECTED_RESULTS = (
    "hexiom", "pyflate", "scimark_fft", "scimark_lu",
    "scimark_monte_carlo", "scimark_sor", "scimark_sparse_mat_mult",
    "regex_v8", "mdp", "async_tree_none", "unpickle_pure_python",
    "typing_runtime_protocols",
)


def load_measure():
    spec = importlib.util.spec_from_file_location("checkpoint_measure", MEASURE_PATH)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def tree_digest(root: Path) -> dict:
    """Hash Python package or benchmark source files without changing them."""
    if not root.is_dir():
        raise ValueError(f"measurement source directory is missing: {root}")
    files = sorted(path for path in root.rglob("*") if path.is_file())
    if not files:
        raise ValueError(f"measurement source directory is empty: {root}")
    digest = hashlib.sha256()
    entries = []
    for path in files:
        relative = path.relative_to(root).as_posix()
        file_hash = sha256_file(path)
        digest.update(relative.encode("utf-8"))
        digest.update(b"\0")
        digest.update(file_hash.encode("ascii"))
        digest.update(b"\n")
        entries.append({"path": relative, "sha256": file_hash})
    return {"root": str(root.resolve()), "sha256": digest.hexdigest(),
            "files": entries}


def build_paths(args) -> dict[str, Path]:
    return {
        "upstream": args.upstream.resolve(),
        "interim": args.interim.resolve(),
        "candidate": args.candidate.resolve(),
    }


def build_environment(source: Path, pyperf_root: Path) -> dict[str, str]:
    env = {key: value for key, value in os.environ.items()
           if not key.startswith("PYTHON")}
    env["PYTHONHASHSEED"] = "0"
    env["PYTHONPATH"] = os.pathsep.join((str(pyperf_root), str(source / "Lib")))
    return env


def snapshots(measure, builds: dict[str, Path], pyperf_root: Path,
              logs: dict[str, Path]) -> dict:
    result = {"builds": {}, "errors": []}
    for role in ROLE_NAMES:
        try:
            build_dir, _python = measure.resolve_build(builds[role])
            source, _makefile = measure.source_tree_for(build_dir)
            env = build_environment(source, pyperf_root)
            snapshot = measure.capture_build(role, builds[role], env, logs[role])
            package_proc = measure.run(
                [snapshot["python"], "-B", "-S", "-c",
                 "import json, pyperf; print(json.dumps({'origin': pyperf.__file__, "
                 "'version': pyperf.__version__}))"], env=env)
            if package_proc.returncode:
                raise ValueError(f"{role}: pyperf import probe failed: "
                                 f"{package_proc.stderr.strip()}")
            package_info = json.loads(package_proc.stdout)
            package_origin = Path(package_info["origin"]).resolve()
            if package_origin != (pyperf_root / "pyperf/__init__.py").resolve():
                raise ValueError(f"{role}: pyperf imported from unexpected origin: "
                                 f"{package_origin}")
            if package_info["version"] != "2.10.0":
                raise ValueError(f"{role}: expected pyperf 2.10.0, got "
                                 f"{package_info['version']}")
            snapshot["pyperf"] = {**package_info,
                                  "origin": str(package_origin),
                                  "package_sha256": tree_digest(pyperf_root / "pyperf")["sha256"]}
            result["builds"][role] = snapshot
            result["errors"].extend(
                measure.validate_snapshot(snapshot, role, "qualified"))
        except (OSError, ValueError) as exc:
            result["builds"][role] = {"error": str(exc)}
            result["errors"].append(f"{role}: {exc}")
    return result


def source_manifest(pyperf_root: Path, benchmark_root: Path) -> dict:
    packages = {"pyperf": tree_digest(pyperf_root / "pyperf")}
    benchmarks = {}
    for name, folder, _script, _args in TASKS:
        root = benchmark_root / folder
        benchmarks[name] = tree_digest(root)
    return {"pyperf_version": "2.10.0", "pyperformance_version": "1.14.0",
            "pyperf": packages, "benchmarks": benchmarks}


def write_launcher(path: Path, python: Path, cache_root: Path) -> None:
    """Create an isolated -S launcher with a per-process bytecode cache."""
    path.parent.mkdir(parents=True, exist_ok=True)
    cache = cache_root.resolve()
    script = (
        "#!/bin/sh\n"
        "set -eu\n"
        f"cache={shlex.quote(str(cache))}/$PPID-$$\n"
        'mkdir -p "$cache"\n'
        'trap \'rm -rf "$cache"\' EXIT HUP INT TERM\n'
        f"{shlex.quote(str(python))} -S -X pycache_prefix=\"$cache\" \"$@\"\n"
    )
    path.write_text(script)
    path.chmod(0o755)


def load_guard(loads: tuple[float, float, float] | None = None) -> tuple[float, float, float]:
    if loads is None:
        loads = os.getloadavg()
    if loads[0] >= LOAD_LIMIT:
        raise RuntimeError(f"load guard reached {LOAD_LIMIT}: {loads}")
    return loads


def journal_record(path: Path, record: dict) -> None:
    with path.open("a", encoding="utf-8") as stream:
        stream.write(json.dumps(record, sort_keys=True) + "\n")
        stream.flush()
        os.fsync(stream.fileno())


def run_command(command: list[str], env: dict[str, str], cwd: Path,
                log_prefix: Path, journal: Path, *, timeout: float | None = None) -> None:
    if timeout is None:
        loads = load_guard()
        deadline = None
    else:
        wait_started = time.monotonic()
        deadline = wait_started + timeout
        while True:
            loads = os.getloadavg()
            now = time.monotonic()
            journal_record(journal, {"phase": "load-wait", "time": time.time(),
                                     "elapsed_seconds": now - wait_started,
                                     "loadavg": loads, "command": command})
            if now >= deadline:
                journal_record(journal, {"phase": "load-wait-timeout",
                                         "time": time.time(), "command": command})
                raise TimeoutError("declared 30-minute pyperf window expired")
            if loads[0] < 3.0:
                # Recheck at the existing hard guard immediately before launch.
                loads = os.getloadavg()
                journal_record(journal, {"phase": "load-guard", "time": time.time(),
                                         "elapsed_seconds": time.monotonic() - wait_started,
                                         "loadavg": loads, "command": command})
                try:
                    load_guard(loads)
                except RuntimeError:
                    continue
                break
            time.sleep(min(10, max(0, deadline - time.monotonic())))
    started = time.time()
    journal_record(journal, {"phase": "start", "time": started,
                             "loadavg": loads, "cwd": str(cwd),
                             "command": command,
                             "environment": {"PYTHONPATH": env.get("PYTHONPATH"),
                                             "PYTHONHASHSEED": env.get("PYTHONHASHSEED")}})
    if deadline is not None:
        timeout = deadline - time.monotonic()
        if timeout <= 0:
            journal_record(journal, {"phase": "load-wait-timeout",
                                     "time": time.time(), "command": command})
            raise TimeoutError("declared 30-minute pyperf window expired")
    proc = subprocess.Popen(command, cwd=cwd, env=env, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            start_new_session=True)
    try:
        if deadline is not None:
            timeout = max(0, deadline - time.monotonic())
        stdout, stderr = proc.communicate(timeout=timeout)
    except subprocess.TimeoutExpired as exc:
        try:
            os.killpg(proc.pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
        try:
            stdout, stderr = proc.communicate(timeout=3)
        except subprocess.TimeoutExpired:
            try:
                os.killpg(proc.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            stdout, stderr = proc.communicate()
        stdout = stdout or exc.stdout or ""
        stderr = stderr or exc.stderr or ""
        if isinstance(stdout, bytes):
            stdout = stdout.decode("utf-8", errors="replace")
        if isinstance(stderr, bytes):
            stderr = stderr.decode("utf-8", errors="replace")
        log_prefix.with_suffix(".stdout.log").write_text(stdout)
        log_prefix.with_suffix(".stderr.log").write_text(stderr)
        journal_record(journal, {"phase": "timeout", "time": time.time(),
                                 "duration_seconds": time.time() - started,
                                 "command": command, "timeout_seconds": timeout,
                                 "stdout_log": str(log_prefix.with_suffix(".stdout.log")),
                                 "stderr_log": str(log_prefix.with_suffix(".stderr.log"))})
        raise
    log_prefix.with_suffix(".stdout.log").write_text(stdout)
    log_prefix.with_suffix(".stderr.log").write_text(stderr)
    journal_record(journal, {"phase": "finish", "time": time.time(),
                             "duration_seconds": time.time() - started,
                             "returncode": proc.returncode,
                             "stdout_log": str(log_prefix.with_suffix(".stdout.log")),
                             "stderr_log": str(log_prefix.with_suffix(".stderr.log"))})
    if proc.returncode:
        raise RuntimeError(f"command failed ({proc.returncode}): {command}")


def pyperf_stage(args, measure, builds: dict[str, Path], before: dict,
                 manifest: dict) -> dict:
    output = args.output
    pyperf_root = args.pyperf_root.resolve()
    benchmark_root = args.benchmark_root.resolve()
    out = output / "pyperf"
    out.mkdir()
    journal = out / "commands.jsonl"
    scripts = {name: benchmark_root / folder / "run_benchmark.py"
               for name, folder, _script, _extra in TASKS}
    launchers = {}
    for role in ROLE_NAMES:
        python = Path(before["builds"][role]["python"])
        launcher = output / "launchers" / f"{role}-python"
        write_launcher(launcher, python, output / "pycache" / role)
        launchers[role] = launcher

    # Rotate both benchmark module order and interpreter order to balance drift.
    module_names = [row[0] for row in TASKS]
    deadline = time.monotonic() + PY_PERF_WINDOW_SECONDS
    invocations = 0
    result_names = {role: [] for role in ROLE_NAMES}
    for module_index, (name, _folder, _script, extra_args) in enumerate(TASKS):
        role_offset = module_index % len(ROLE_NAMES)
        role_order = ROLE_NAMES[role_offset:] + ROLE_NAMES[:role_offset]
        for role in role_order:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise TimeoutError("declared 30-minute pyperf window expired")
            snapshot = before["builds"][role]
            source = Path(snapshot["source"])
            env = build_environment(source, pyperf_root)
            launcher = launchers[role]
            result = out / f"{role}-{name}.json"
            script = scripts[name]
            command = [str(launcher), str(script),
                       "--processes", "6", "--values", "3", "--warmups", "1",
                       "--min-time", "0.1", "--inherit-environ", "PYTHONHASHSEED",
                       "--python", str(launcher), "--output", str(result), *extra_args]
            # Preserve the predeclared module sequence in each command record.
            command_meta = {"module_order_index": module_index,
                            "module_order": module_names,
                            "role_order": role_order}
            prefix = out / f"{role}-{name}"
            run_command(command, env, benchmark_root / next(
                folder for task_name, folder, _script, _opts in TASKS
                if task_name == name), prefix, journal,
                timeout=max(0, deadline - time.monotonic()))
            if not result.is_file():
                raise RuntimeError(f"pyperf did not create {result}")
            data = json.loads(result.read_text())
            actual = sorted(
                {**data.get("metadata", {}), **item.get("metadata", {})}["name"]
                for item in data["benchmarks"])
            expected = sorted(("scimark_fft", "scimark_lu", "scimark_monte_carlo",
                               "scimark_sor", "scimark_sparse_mat_mult")
                              if name == "scimark" else
                              ("async_tree_none" if name == "async_tree_none" else name,))
            if actual != expected:
                raise RuntimeError(f"{name} results differ: expected {expected}, got {actual}")
            journal_record(journal, {"phase": "validated-result", "role": role,
                                     "module": name, "result": str(result),
                                     "expected_names": expected, "actual_names": actual,
                                     **command_meta})
            result_names[role].extend(actual)
            invocations += 1
    if invocations != 24:
        raise RuntimeError(f"expected 24 pyperf invocations, got {invocations}")
    for role, names in result_names.items():
        if sorted(names) != sorted(EXPECTED_RESULTS):
            raise RuntimeError(f"{role} result roster differs: {sorted(names)}")
    manifest_path = out / "pyperformance-manifest.json"
    manifest_path.write_text(json.dumps({"qualification": "diagnostic",
                                         "eligible_for_R27": False,
                                         "settings": {"processes": 6, "values": 3,
                                                      "warmups": 1, "min_time": 0.1,
                                                      "window_seconds": 1800,
                                                      "max_load": LOAD_LIMIT},
                                         "expected_results": EXPECTED_RESULTS,
                                         "source_manifest": manifest,
                                         "files": sorted(str(path) for path in out.glob("*.json"))},
                                        indent=2))
    return {"invocations": invocations, "manifest": str(manifest_path)}


def warm_file(path: Path) -> int:
    import mmap

    size = path.stat().st_size
    if not size:
        return 0
    with path.open("rb") as stream:
        with mmap.mmap(stream.fileno(), 0, access=mmap.ACCESS_READ) as mapped:
            checksum = 0
            for offset in range(0, size, PAGE_SIZE):
                checksum ^= mapped[offset]
    return checksum


def rss_stage(args, measure, builds: dict[str, Path], before: dict,
              manifest: dict) -> dict:
    output = args.output
    out = output / "rss"
    out.mkdir()
    journal = out / "commands.jsonl"
    warm_records = {}
    paths_by_role = {}
    for role in ROLE_NAMES:
        snapshot = before["builds"][role]
        build = Path(snapshot["build_dir"])
        paths = {Path(snapshot["python"])}
        paths.update(Path(module["file"]) for module in snapshot["probe"]["modules"].values()
                     if module.get("file") and Path(module["file"]).suffix == ".so")
        libdir = (build / (build / "pybuilddir.txt").read_text().strip()).resolve()
        if not libdir.is_dir():
            raise ValueError(f"configured extension directory is missing: {libdir}")
        suffix = f".{snapshot['probe']['config']['SOABI']}.so"
        extensions = list(libdir.glob(f"*{suffix}"))
        expected = snapshot["module_log"]["shared"]
        if len(extensions) != expected:
            raise ValueError(f"{role}: expected {expected} current-ABI extensions, "
                             f"found {len(extensions)} in {libdir}")
        paths.update(extensions)
        sorted_paths = sorted(path.resolve() for path in paths if path.is_file())
        paths_by_role[role] = sorted_paths
        warm_records[role] = [{"path": str(path), "size": path.stat().st_size,
                               "page_touch_checksum": warm_file(path)}
                              for path in sorted_paths]
    (out / "warm-pages.json").write_text(json.dumps(warm_records, indent=2))
    for role in ROLE_NAMES:
        sorted_paths = paths_by_role[role]
        fincore = subprocess.run(["fincore", "--bytes", *map(str, sorted_paths)],
                                 text=True, capture_output=True, check=False)
        (out / f"{role}-fincore.stdout.log").write_text(fincore.stdout)
        (out / f"{role}-fincore.stderr.log").write_text(fincore.stderr)
        journal_record(journal, {"phase": "fincore", "role": role,
                                 "command": ["fincore", "--bytes", *map(str, sorted_paths)],
                                 "returncode": fincore.returncode,
                                 "stdout": f"{role}-fincore.stdout.log",
                                 "stderr": f"{role}-fincore.stderr.log",
                                 "files": len(sorted_paths)})
        if fincore.returncode:
            raise RuntimeError(f"fincore failed for {role}: {fincore.stderr.strip()}")

    spec = importlib.util.spec_from_file_location("checkpoint_bench", HERE / "run.py")
    bench = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = bench
    spec.loader.exec_module(bench)
    kernels = [item.name for item in bench.BENCHMARKS if item.group == "kernels"]
    chosen = kernels + ["startup", "imports"]
    results = {}
    for role in ROLE_NAMES:
        build = builds[role]
        result = out / f"{role}-rss.json"
        command = [sys.executable, "-B", str(HERE / "run.py"), "--build", str(build),
                   "--label", role, "--set", "kernels,memory", "--only",
                   ",".join(chosen), "--metric", "rss", "--rss-runs", "3",
                   "--max-load", str(LOAD_LIMIT), "-o", str(result)]
        source = Path(before["builds"][role]["source"])
        env = {key: value for key, value in os.environ.items()
               if not key.startswith("PYTHON")}
        env["PYTHONHASHSEED"] = "0"
        prefix = out / f"{role}-rss"
        run_command(command, env, HERE.parent.parent, prefix, journal)
        results[role] = str(result)
    manifest_path = out / "rss-manifest.json"
    manifest_path.write_text(json.dumps({"qualification": "diagnostic",
                                         "eligible_for_R27": False,
                                         "metric": "rss", "rss_runs": 3,
                                         "benchmarks": chosen,
                                         "source_manifest": manifest,
                                         "results": results}, indent=2))
    return {"results": results, "manifest": str(manifest_path)}


def parse_args(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stage", choices=("pyperf", "rss"), required=True)
    parser.add_argument("--qualification", choices=("diagnostic",), required=True,
                        help="subset and RSS records are never R27 qualification evidence")
    parser.add_argument("--upstream", type=Path, required=True)
    parser.add_argument("--interim", type=Path, required=True)
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--module-log", action="append", default=[], metavar="ROLE=PATH",
                        help="immutable module roster log; repeat for all roles")
    parser.add_argument("--output", type=Path, required=True,
                        help="new empty experiment directory")
    parser.add_argument("--pyperf-root", type=Path, default=DEFAULT_PYPERF_ROOT)
    parser.add_argument("--benchmark-root", type=Path, default=DEFAULT_BENCH_ROOT)
    return parser.parse_args(argv)


def main(argv=None) -> int:
    args = parse_args(argv)
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    measure = load_measure()
    logs = measure.parse_module_logs(args.module_log)
    if set(logs) != set(ROLE_NAMES):
        raise SystemExit("supply exactly one --module-log for each role")
    builds = build_paths(args)
    manifest = source_manifest(args.pyperf_root.resolve(), args.benchmark_root.resolve())
    before = snapshots(measure, builds, args.pyperf_root.resolve(), logs)
    before["qualification"] = "diagnostic"
    before["eligible_for_R27"] = False
    (args.output / "preflight-before.json").write_text(json.dumps(before, indent=2))
    if before["errors"]:
        (args.output / "qualification-status.json").write_text(json.dumps({
            "qualification": "diagnostic", "eligible": False,
            "phase": "preflight", "errors": before["errors"]}, indent=2))
        raise SystemExit("identity preflight failed; no benchmark started")
    (args.output / "source-manifest.json").write_text(json.dumps(manifest, indent=2))
    try:
        if args.stage == "pyperf":
            stage_result = pyperf_stage(args, measure, builds, before, manifest)
        else:
            stage_result = rss_stage(args, measure, builds, before, manifest)
        failure = None
    except BaseException as exc:
        stage_result = None
        failure = f"{type(exc).__name__}: {exc}"
        raise
    finally:
        after = snapshots(measure, builds, args.pyperf_root.resolve(), logs)
        after_manifest = source_manifest(args.pyperf_root.resolve(),
                                          args.benchmark_root.resolve())
        (args.output / "source-manifest-after.json").write_text(
            json.dumps(after_manifest, indent=2))
        manifest_unchanged = manifest == after_manifest
        differences = measure.identity_differences(before, after)
        status = {"qualification": "diagnostic", "eligible": False,
                  "eligible_for_R27": False, "stage": args.stage,
                  "identity_unchanged": not differences,
                  "identity_differences": differences,
                  "measurement_sources_unchanged": manifest_unchanged,
                  "preflight_errors": before["errors"], "postflight_errors": after["errors"],
                  "stage_error": failure}
        after["qualification"] = "diagnostic"
        after["eligible_for_R27"] = False
        (args.output / "preflight-after.json").write_text(json.dumps(after, indent=2))
        (args.output / "qualification-status.json").write_text(json.dumps(status, indent=2))
    if differences or after["errors"] or not manifest_unchanged:
        raise SystemExit("postflight identity/source check failed; inspect saved evidence")
    print(json.dumps(stage_result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
