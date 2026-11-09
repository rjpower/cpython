#!/usr/bin/env python3
"""Measure the registered micro and timed memory sets on three fixed builds.

This is a coverage diagnostic. It does not add an elapsed-time parity result to
the 29-kernel screen. The benchmark registry and measurement code stay in
pyjit/bench/run.py.
"""

import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[2]
RUNNER = ROOT / "bench/run.py"
CHECKPOINT = ROOT / "issues/checkpoint-closure/measure.py"
ROLES = ("upstream", "golden", "candidate")
sys.dont_write_bytecode = True


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def run_sweep(command, env, timeout, output, stem):
    # Perf and benchmark children inherit this owned process group. A timeout
    # must retire the whole sweep before another native job can begin.
    with subprocess.Popen(command, env=env, stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, text=True,
                          start_new_session=True) as child:
        try:
            stdout, stderr = child.communicate(timeout=timeout)
        except BaseException:
            try:
                os.killpg(child.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            stdout, stderr = child.communicate()
            (output / f"{stem}.stdout").write_text(stdout)
            (output / f"{stem}.stderr").write_text(stderr)
            raise
        return subprocess.CompletedProcess(command, child.returncode,
                                           stdout, stderr)


def manifest(builds):
    inputs = [Path(__file__), RUNNER, CHECKPOINT]
    inputs.extend((ROOT / "bench/internal").rglob("*.py"))
    for build in builds.values():
        source, makefile = CHECKPOINT_MODULE.source_tree_for(build)
        inputs.append(makefile)
        inputs.extend((source / "Lib").rglob("*.py"))
        inputs.extend(build.glob("build/lib*/*.so"))
        inputs.extend(build.glob("build/lib*/**/__pycache__/*.pyc"))
        inputs.extend(build.glob("sysconfig*.pyc"))
    return {str(path.absolute()): digest(path) for path in sorted(set(inputs))}


def snapshot(builds, logs, heads, env):
    records = {}
    for role in ROLES:
        label = "upstream" if role == "upstream" else "candidate"
        row = CHECKPOINT_MODULE.snapshot_all(
            {label: builds[role]}, env, {label: logs[role]})
        require(not row["errors"], f"{role} native preflight: {row['errors']}")
        record = row["builds"][label]
        require(record["git"] == {"head": heads[role], "status": ""},
                f"{role} source HEAD or cleanliness changed")
        require(record["python_build_id"], f"{role} executable has no Build ID")
        native = record["probe"]["git"][2]
        require(re.fullmatch(r"[0-9a-f]{7,40}", native) is not None
                and heads[role].startswith(native),
                f"{role} native revision differs from source")
        require(record["module_log"]["modules"] == (116 if role == "upstream" else 117),
                f"{role} module count changed")
        require(record["probe"]["jit"]["enabled"] is False
                and record["probe"]["method_jit"]["natural_admission"] is not True,
                f"{role} JIT was not disabled")
        extensions = list(builds[role].glob("build/lib*/*.so"))
        # Value-runtime build directories retain historical ABI variants.
        # Checked module counts above are distinct from physical DSO paths.
        expected_extensions = 78 if role == "upstream" else 213
        require(len(extensions) == expected_extensions,
                f"{role} physical extension set differs from the retained census")
        records[role] = record
    require(records["golden"]["probe"]["config"] ==
            records["candidate"]["probe"]["config"],
            "golden and candidate build configurations differ")
    return {"builds": records, "inputs": manifest(builds),
            "affinity": sorted(os.sched_getaffinity(0))}


def verify_report(path, role, metric, names, identity, roster):
    report = json.loads(path.read_text())
    require(report["schema"] == "boil-bench/1", "benchmark schema changed")
    require(report["build"]["git"] == identity["probe"]["git"],
            f"{role} benchmark child revision changed")
    require(report["build"]["path"] == identity["python"],
            f"{role} benchmark executable changed")
    require(report["build"]["hash_seed"] == 0, "hash seed changed")
    if metric == "instr":
        journal = report["runs"][metric]["perf_records"]
        require(digest(Path(journal["path"])) == journal["sha256"]
                and journal["invocations"] > 0,
                f"{role} raw perf journal changed")
    rows = report["results"][metric]
    require(set(rows) == set(names), f"{role} {metric} roster incomplete")
    for name, row in rows.items():
        expected_runs = 4 if metric == "instr" else 5
        require(row["runs"] == expected_runs and len(row["values"]) == expected_runs,
                f"{role} {name} sample count changed")
        if metric == "instr":
            require(row["loops"] == [roster[name].loops, 3 * roster[name].loops]
                    and row["inner"] == roster[name].inner,
                    f"{role} {name} instruction loop shape changed")
            require(len(row["raw_pairs"]) == 4,
                    f"{role} {name} differenced pairs missing")
            if roster[name].skeleton:
                require(row["skeleton"] == roster[name].skeleton
                        and "raw" in row, f"{role} {name} skeleton missing")
        else:
            require(row["loops"] == roster[name].loops
                    and row["warmup_calls"] == 1,
                    f"{role} {name} timing loop shape changed")
    return report


def collect(args):
    require(os.sched_getaffinity(0) == {15}, "run the wrapper on CPU 15")
    require(len(set(args.head)) == 3 and
            all(re.fullmatch(r"[0-9a-f]{40}", head) for head in args.head),
            "three distinct full source SHAs are required")
    builds = dict(zip(ROLES, map(Path, args.build)))
    builds = {role: build.resolve() for role, build in builds.items()}
    logs = {}
    for entry in args.module_log:
        role, path = entry.split("=", 1)
        require(role in ROLES and role not in logs,
                f"invalid or duplicate module-log role: {role}")
        label = "upstream" if role == "upstream" else "candidate"
        logs[role] = CHECKPOINT_MODULE.parse_module_logs([f"{label}={path}"])[label]
    require(set(logs) == set(ROLES), "all three module logs are required")
    heads = dict(zip(ROLES, args.head))
    require(len({str(path) for path in builds.values()}) == 3,
            "three distinct build directories are required")
    registry = load("supplemental_bench_registry", RUNNER)
    roster = {bench.name: bench for bench in registry.BENCHMARKS}
    micro = [bench.name for bench in registry.BENCHMARKS if bench.group == "micro"]
    memory = [bench.name for bench in registry.BENCHMARKS
              if bench.group == "memory" and "time" in registry.metrics_of(bench)]
    require(len(micro) == len(set(micro)) == 65 and
            len(memory) == len(set(memory)) == 6 and
            {"startup", "imports"}.isdisjoint(memory),
            "registered supplemental roster changed")
    env = {key: value for key, value in os.environ.items()
           if not key.startswith("PYTHON") and key != "LD_PRELOAD"}
    env.update(PYTHONHASHSEED="0", LD_BIND_NOW="1")
    before = snapshot(builds, logs, heads, env)
    require(before["affinity"] == [15], "CPU affinity changed")
    (args.output / "identity-before.json").write_text(json.dumps(before, indent=2))
    runs = []
    failure = None
    try:
        # One serial sweep per metric. This is a coverage diagnostic, not a
        # balanced elapsed-time comparison.
        for metric, group, names in (("instr", "micro", micro),
                                     ("time", "memory", memory)):
            for role in ROLES:
                stem = f"{role}-{group}-{metric}"
                path = args.output / f"{stem}.json"
                command = [sys.executable, "-B", str(RUNNER),
                           "--build", str(builds[role]), "--label", role,
                           "--set", group, "--metric", metric,
                           "--max-load", "4", "-o", str(path)]
                if metric == "instr":
                    command += ["--perf-records", str(args.output / f"{stem}.jsonl")]
                row = {"role": role, "metric": metric, "group": group,
                       "command": command, "state": "started"}
                runs.append(row)
                (args.output / "runs.json").write_text(json.dumps(runs, indent=2))
                try:
                    proc = run_sweep(command, env,
                                     3600 if metric == "instr" else 1800,
                                     args.output, stem)
                except BaseException as exc:
                    row.update(state="launch-failed",
                               error=f"{type(exc).__name__}: {exc}")
                    (args.output / "runs.json").write_text(json.dumps(runs, indent=2))
                    raise
                (args.output / f"{stem}.stdout").write_text(proc.stdout)
                (args.output / f"{stem}.stderr").write_text(proc.stderr)
                row.update(state="finished", returncode=proc.returncode)
                (args.output / "runs.json").write_text(json.dumps(runs, indent=2))
                require(proc.returncode == 0 and path.is_file(),
                        f"{stem} failed; inspect retained stderr/journal")
                verify_report(path, role, metric, names, before["builds"][role],
                              roster)
    except BaseException as exc:
        failure = f"{type(exc).__name__}: {exc}"
        raise
    finally:
        after = None
        postflight_error = None
        try:
            after = snapshot(builds, logs, heads, env)
            (args.output / "identity-after.json").write_text(json.dumps(after, indent=2))
        except BaseException as exc:
            postflight_error = f"{type(exc).__name__}: {exc}"
        (args.output / "status.json").write_text(json.dumps({
            "failure": failure, "postflight_error": postflight_error,
            "postflight_equal": after == before if after is not None else False,
            "complete": failure is None and postflight_error is None
                        and after == before and len(runs) == 6,
            "runs": len(runs), "micro": micro, "timed_memory": memory,
            "excluded_rss_only": ["startup", "imports"],
        }, indent=2))
        require(postflight_error is None and after == before,
                f"postflight identity or inputs changed: {postflight_error}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", action="append", required=True,
                        help="repeat in upstream, golden, candidate order")
    parser.add_argument("--head", action="append", required=True,
                        help="repeat three full SHAs in build order")
    parser.add_argument("--module-log", action="append", required=True,
                        help="repeat upstream=PATH, golden=PATH, candidate=PATH")
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    require(len(args.build) == len(args.head) == len(args.module_log) == 3,
            "supply three builds, heads and module logs in role order")
    args.output.mkdir(parents=True, exist_ok=False)
    try:
        collect(args)
    except BaseException as exc:
        status = args.output / "status.json"
        if not status.exists():
            status.write_text(json.dumps({"complete": False,
                                          "failure": f"{type(exc).__name__}: {exc}"}, indent=2))
        raise


CHECKPOINT_MODULE = load("supplemental_checkpoint", CHECKPOINT)

if __name__ == "__main__":
    main()
