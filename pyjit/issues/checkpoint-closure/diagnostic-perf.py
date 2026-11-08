#!/usr/bin/env python3
"""Collect bounded perf counters around warmed benchmark calls.

The coordinator runs this tool after review. It never builds the interpreter.
"""

from __future__ import annotations

import argparse
import collections
import datetime
import importlib.util
import json
import math
import os
from pathlib import Path
import select
import shutil
import signal
import statistics
import subprocess
import sys
import time


ROOT = Path(__file__).resolve().parents[2]
BENCH_PATH = ROOT / "bench" / "run.py"
PROFILE_IDENTITY_DIR = Path(
    "/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/"
    "pyjit-link-wave1/checkpoint-closure/profiles-6a0e4c12c17-diagnostic"
)
CASES = ("deltablue", "logging_silent", "comprehensions")
ROLES = ("upstream", "candidate")
STAT_TARGET_NS = 2_000_000_000
RECORD_TARGET_NS = 5_000_000_000
ACK_TIMEOUT = 10.0
COMMAND_TIMEOUT = 60.0
MIN_PCT_RUNNING = 99.0
MAX_LOAD = 4.0
ACK_RESPONSE_BYTES = (b"ack\n", b"ack\n\x00")
ACK_RESPONSE_TEXT = tuple(value.decode("ascii") for value in ACK_RESPONSE_BYTES)

EVENT_GROUPS = (
    ("cycles-instructions", ("cycles:u", "instructions:u")),
    ("branches", ("branches:u", "branch-misses:u")),
    ("cache", ("cache-references:u", "cache-misses:u")),
)


def load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise ValueError(f"cannot import benchmark module {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def build_path(value: Path) -> Path:
    path = value.resolve(strict=True)
    if path.is_dir():
        path = (path / "python").resolve(strict=True)
    if not path.is_file() or not os.access(path, os.X_OK):
        raise ValueError(f"not an executable interpreter: {path}")
    return path


def run_capture(argv: list[str], env: dict[str, str], timeout: float = COMMAND_TIMEOUT):
    process = subprocess.Popen(
        argv, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
        stderr=subprocess.PIPE, text=True, env=env, start_new_session=True,
        close_fds=True,
    )
    try:
        stdout, stderr = process.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        try:
            os.killpg(process.pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
        try:
            stdout, stderr = process.communicate(timeout=2)
        except subprocess.TimeoutExpired:
            try:
                os.killpg(process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            stdout, stderr = process.communicate()
        return process.returncode, stdout, stderr, True
    except BaseException:
        try:
            os.killpg(process.pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
        try:
            process.communicate(timeout=2)
        except subprocess.TimeoutExpired:
            try:
                os.killpg(process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            process.communicate()
        raise
    return process.returncode, stdout, stderr, False


def read_build_id(path: Path) -> str:
    proc = subprocess.run(["readelf", "-n", str(path)], capture_output=True,
                          text=True, check=True)
    for line in proc.stdout.splitlines():
        if "Build ID:" in line:
            return line.split("Build ID:", 1)[1].strip().lower()
    raise ValueError(f"missing ELF build ID: {path}")


def saved_build_id(path: Path) -> str:
    for line in path.read_text(encoding="utf-8").splitlines():
        if "Build ID:" in line:
            return line.split("Build ID:", 1)[1].strip().lower()
    raise ValueError(f"missing saved build ID: {path}")


def validate_builds(upstream: Path, candidate: Path, identity_dir: Path) -> dict:
    result = {}
    for role, live in (("upstream", upstream), ("candidate", candidate)):
        retained = identity_dir / f"{role}-python"
        notes = identity_dir / f"{role}-elf-notes.txt"
        live_id = read_build_id(live)
        retained_id = read_build_id(retained)
        expected_id = saved_build_id(notes)
        if live_id != retained_id or live_id != expected_id:
            raise ValueError(
                f"{role} build IDs disagree: live={live_id}, retained={retained_id}, "
                f"saved={expected_id}"
            )
        result[role] = {
            "live_executable": str(live),
            "retained_symbolization_elf": str(retained),
            "build_id": live_id,
        }
    return result


def benchmark_roster():
    bench = load_module("_diagnostic_perf_bench", BENCH_PATH)
    items = {item.name: item for item in bench.BENCHMARKS if item.group == "kernels"}
    if len(items) != 29 or any(name not in items for name in CASES):
        raise ValueError("benchmark kernel roster does not match the reviewed 29-kernel set")
    return items


def medians(path: Path) -> dict[str, dict[str, float]]:
    data = json.loads(path.read_text(encoding="utf-8"))
    result = {}
    for role in ROLES:
        if not isinstance(data.get(role), dict):
            raise ValueError(f"medians file lacks role {role}")
        result[role] = {}
        for case in CASES:
            values = data[role].get(case)
            if not isinstance(values, list) or len(values) < 3:
                raise ValueError(f"medians file lacks three fresh values for {role}/{case}")
            nums = [float(value) for value in values]
            if not all(math.isfinite(value) and value > 0 for value in nums):
                raise ValueError(f"invalid median timing values for {role}/{case}")
            result[role][case] = statistics.median(nums)
    return result


def child_environment() -> dict[str, str]:
    env = {key: value for key, value in os.environ.items()
           if not key.startswith("PYTHON") and key != "LD_PRELOAD"}
    env["PYTHONHASHSEED"] = "0"
    return env


def write_json(path: Path, data) -> None:
    path.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def load_now() -> list[float]:
    return list(os.getloadavg())


def wait_for_quiet_load(deadline: float) -> tuple[list[dict], list[float], bool]:
    observations = []
    while True:
        snapshot = load_now()
        observations.append({"time_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
                             "load_1_5_15": snapshot})
        now = time.monotonic()
        if now >= deadline:
            return observations, snapshot, True
        if snapshot[0] < 3.0:
            return observations, snapshot, False
        time.sleep(min(10.0, max(0.0, deadline - now)))


def fifo_paths(output: Path, stem: str) -> tuple[Path, Path, Path]:
    root = output / "control" / stem
    root.mkdir(parents=True)
    ctl, ack = root / "ctl", root / "ack"
    os.mkfifo(ctl, 0o600)
    os.mkfifo(ack, 0o600)
    return root, ctl, ack


def exchange(fd: int, ack_fd: int, command: bytes) -> tuple[bool, dict]:
    started = time.monotonic_ns()
    written = os.write(fd, command)
    if written != len(command):
        return False, {"command": command.decode().strip(), "error": "short control write",
                       "started_monotonic_ns": started}
    ready, _, _ = select.select([ack_fd], [], [], ACK_TIMEOUT)
    if not ready:
        return False, {"command": command.decode().strip(), "error": "ack timeout",
                       "started_monotonic_ns": started,
                       "timeout_seconds": ACK_TIMEOUT}
    response = os.read(ack_fd, 64)
    finished = time.monotonic_ns()
    ok = response in ACK_RESPONSE_BYTES
    return ok, {"command": command.decode().strip(),
                "response": response.decode("ascii", errors="backslashreplace"),
                "response_hex": response.hex(),
                "started_monotonic_ns": started,
                "ack_monotonic_ns": finished,
                "ack_latency_ns": finished - started,
                "error": None if ok else "unexpected ack response"}


def child_run(bench_path: Path, variant: str, warmup: int, loops: int,
              ctl_path: Path, ack_path: Path, result_path: Path,
              empty: bool = False) -> int:
    record = {
        "bench_path": str(bench_path), "variant": variant,
        "warmup_loops": warmup, "hot_loops": loops,
        "calibration": empty,
        "setup_started_monotonic_ns": time.monotonic_ns(),
        "setup_wall_ns": None, "warmup_wall_ns": None,
        "enable": None, "disable": None,
        "hot_wall_ns": None, "hot_process_cpu_ns": None,
        "child_returncode": 1, "error": None,
    }
    control_fd = ack_fd = None
    error: BaseException | None = None
    try:
        start = time.monotonic_ns()
        module = load_module("_diagnostic_perf_kernel", bench_path)
        setup = getattr(module, "setup", None)
        if setup is not None:
            setup(None if variant == "-" else variant)
        record["setup_wall_ns"] = time.monotonic_ns() - start
        start = time.monotonic_ns()
        module.run(warmup)
        record["warmup_wall_ns"] = time.monotonic_ns() - start
        control_fd = os.open(ctl_path, os.O_WRONLY | os.O_CLOEXEC)
        ack_fd = os.open(ack_path, os.O_RDONLY | os.O_CLOEXEC)
        enabled, record["enable"] = exchange(control_fd, ack_fd, b"enable\n")
        if not enabled:
            raise RuntimeError(f"perf enable failed: {record['enable']}")
        wall_start = time.perf_counter_ns()
        cpu_start = time.process_time_ns()
        try:
            module.run(loops)
        finally:
            record["hot_wall_ns"] = time.perf_counter_ns() - wall_start
            record["hot_process_cpu_ns"] = time.process_time_ns() - cpu_start
        record["child_returncode"] = 0
    except BaseException as exc:
        error = exc
        record["error"] = f"{type(exc).__name__}: {exc}"
    finally:
        if control_fd is not None and ack_fd is not None:
            try:
                disabled, record["disable"] = exchange(control_fd, ack_fd, b"disable\n")
                if not disabled:
                    raise RuntimeError(f"perf disable failed: {record['disable']}")
            except BaseException as exc:
                record["child_returncode"] = 1
                if error is None:
                    error = exc
                    record["error"] = f"{type(exc).__name__}: {exc}"
        if ack_fd is not None:
            try:
                os.close(ack_fd)
            except OSError as exc:
                record["child_returncode"] = 1
                if error is None:
                    error = exc
                    record["error"] = f"{type(exc).__name__}: {exc}"
        if control_fd is not None:
            try:
                os.close(control_fd)
            except OSError as exc:
                record["child_returncode"] = 1
                if error is None:
                    error = exc
                    record["error"] = f"{type(exc).__name__}: {exc}"
        record["finished_monotonic_ns"] = time.monotonic_ns()
        write_json(result_path, record)
    if error is not None:
        raise error
    return 0


def child_entry(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--child", action="store_true")
    parser.add_argument("--bench-path", type=Path, required=True)
    parser.add_argument("--variant", required=True)
    parser.add_argument("--warmup", type=int, required=True)
    parser.add_argument("--loops", type=int, required=True)
    parser.add_argument("--empty", action="store_true")
    parser.add_argument("--ctl", type=Path, required=True)
    parser.add_argument("--ack", type=Path, required=True)
    parser.add_argument("--child-result", type=Path, required=True)
    args = parser.parse_args(argv)
    return child_run(args.bench_path, args.variant, args.warmup, args.loops,
                     args.ctl, args.ack, args.child_result, args.empty)


def parse_stat(path: Path, expected: tuple[str, ...]) -> dict:
    rows = []
    errors = []
    if not path.is_file():
        return {"valid": False, "events": [], "errors": ["missing perf stat output"]}
    for lineno, line in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
        if not line.strip() or not line.lstrip().startswith("{"):
            continue
        try:
            row = json.loads(line)
        except json.JSONDecodeError:
            errors.append(f"line {lineno}: malformed JSON")
            continue
        event = str(row.get("event", ""))
        count_raw = row.get("counter-value")
        runtime_raw = row.get("event-runtime")
        running_raw = row.get("pcnt-running")
        try:
            count = float(count_raw)
            runtime = float(runtime_raw)
            running = float(str(running_raw).rstrip("%"))
            if not all(math.isfinite(value) for value in (count, runtime, running)):
                raise ValueError("non-finite measurement")
        except (TypeError, ValueError):
            errors.append(f"{event or 'unknown event'}: missing or non-numeric count/runtime/running")
            rows.append({"event": event, "counter_value": count_raw,
                         "event_runtime": runtime_raw, "pcnt_running": running_raw,
                         "valid": False})
            continue
        valid = (count >= 0 and runtime > 0 and MIN_PCT_RUNNING <= running <= 100.0
                 and event in expected)
        if count < 0:
            errors.append(f"{event}: negative counter value")
        if runtime <= 0:
            errors.append(f"{event}: zero event runtime")
        if running < MIN_PCT_RUNNING:
            errors.append(f"{event}: scheduled {running:.3f}% (< {MIN_PCT_RUNNING:.1f}%)")
        if running > 100.0:
            errors.append(f"{event}: scheduled {running:.3f}% (> 100%)")
        if event not in expected:
            errors.append(f"unexpected event or scope: {event}")
        rows.append({"event": event,
                     "counter_value": count_raw, "event_runtime": runtime_raw,
                     "pcnt_running": running_raw, "valid": valid})
    found = collections.Counter(row.get("event") for row in rows if row.get("valid"))
    for event in expected:
        if found[event] != 1:
            errors.append(f"{event}: expected one valid event row, found {found[event]}")
    return {"valid": not errors, "events": rows, "errors": errors}


def make_invocation(output: Path, role: str, case: str, round_no: int,
                    event_name: str, events: tuple[str, ...], mode: str,
                    executable: Path, bench, loops: int, env: dict[str, str],
                    median: float, load_before: list[float], load_wait: list[dict],
                    perf_path: str, empty: bool = False) -> dict:
    stem = f"{mode}-r{round_no}-{role}-{case}-{event_name}"
    control_dir, ctl, ack = fifo_paths(output, stem)
    # Anchors keep FIFO opens from deadlocking during perf/child startup. They
    # are close-on-exec and subprocesses use close_fds=True.
    ctl_anchor = os.open(ctl, os.O_RDWR | os.O_NONBLOCK | os.O_CLOEXEC)
    ack_anchor = os.open(ack, os.O_RDWR | os.O_NONBLOCK | os.O_CLOEXEC)
    cache = (output / "pycache" / stem).resolve()
    cache.mkdir(parents=True, exist_ok=False)
    child_result = output / f"{stem}-child.json"
    binary = output / f"{stem}-perf.data"
    rawstat = output / f"{stem}-perf.json"
    stderr_path = output / f"{stem}-stderr.txt"
    stdout_path = output / f"{stem}-stdout.txt"
    returncode_path = output / f"{stem}-returncode.txt"
    command = [perf_path]
    if mode == "stat":
        command += ["stat", "--json-output", "-o", str(rawstat), "-D", "-1",
                    "--control", f"fifo:{ctl},{ack}", "-e", "{" + ",".join(events) + "}"]
    else:
        command += ["record", "--no-buildid-cache", "--buildid-all", "--user-callchains",
                    "-e", "cycles:u", "-F", "499", "--call-graph",
                    "dwarf,8192", "-D", "-1", "--control", f"fifo:{ctl},{ack}",
                    "-o", str(binary)]
    command += ["--", str(executable), "-S", "-B", "-X", f"pycache_prefix={cache}",
                str(Path(__file__).resolve()), "--child", "--bench-path", str(bench.path),
                "--variant", bench.variant or "-", "--warmup", str(bench.loops),
                "--loops", str(loops), "--ctl", str(ctl), "--ack", str(ack),
                "--child-result", str(child_result)]
    if empty:
        command.append("--empty")
    record = {
        "stem": stem, "role": role, "case": case, "round": round_no,
        "mode": mode, "event_group": event_name, "requested_events": list(events),
        "median_ns_per_loop": median, "hot_loops": loops,
        "target_hot_ns": 0 if empty else (
            STAT_TARGET_NS if mode == "stat" else RECORD_TARGET_NS),
        "calibration": empty,
        "command": command, "environment_policy": "bench CHILD_ENV; PYTHONHASHSEED=0; LD_PRELOAD removed",
        "environment_overrides": {"PYTHONHASHSEED": "0"},
        "control_fifos": {"ctl": str(ctl), "ack": str(ack)},
        "control_fifos_removed_after_invocation": False,
        "load_before_wait": load_wait, "load_before": load_before,
        "rawstat": str(rawstat) if mode == "stat" else None,
        "perf_data": str(binary) if mode == "record" else None,
        "child_result": str(child_result), "valid": False, "errors": [],
    }
    write_json(output / f"{stem}-command.json", record)
    returncode = None
    stdout = stderr = ""
    timed_out = False
    try:
        returncode, stdout, stderr, timed_out = run_capture(command, env)
    except Exception as exc:
        record["errors"].append(f"launch: {type(exc).__name__}: {exc}")
    finally:
        os.close(ack_anchor)
        os.close(ctl_anchor)
        for path in (ack, ctl):
            path.unlink(missing_ok=True)
        control_dir.rmdir()
        record["control_fifos_removed_after_invocation"] = True
    stdout_path.write_text(stdout, encoding="utf-8")
    stderr_path.write_text(stderr, encoding="utf-8")
    returncode_path.write_text(f"{returncode}\n" if returncode is not None else "launch-failed\n")
    record.update({"returncode": returncode, "timed_out": timed_out,
                   "load_after": load_now(), "stdout": str(stdout_path),
                   "stderr": str(stderr_path), "returncode_file": str(returncode_path)})
    if timed_out:
        record["errors"].append(f"command exceeded {COMMAND_TIMEOUT}s and process group was killed")
    if returncode != 0:
        record["errors"].append(f"perf command returned {returncode}")
    child_data = None
    if child_result.is_file():
        try:
            child_data = json.loads(child_result.read_text(encoding="utf-8"))
            record["child"] = child_data
        except (OSError, json.JSONDecodeError) as exc:
            record["errors"].append(f"invalid child result: {exc}")
    else:
        record["errors"].append("missing child result")
    if child_data:
        for which in ("enable", "disable"):
            ack_data = child_data.get(which) or {}
            if ack_data.get("response") not in ACK_RESPONSE_TEXT or ack_data.get("error"):
                record["errors"].append(f"{which} ack failed: {ack_data}")
        if child_data.get("child_returncode") != 0 or child_data.get("error"):
            record["errors"].append(f"child failed: {child_data.get('error')}")
        if child_data.get("hot_loops") != loops:
            record["errors"].append("child hot-loop count mismatch")
        if not isinstance(child_data.get("hot_wall_ns"), int) or not isinstance(
                child_data.get("hot_process_cpu_ns"), int):
            record["errors"].append("child did not record hot-region wall and CPU time")
        for key in ("hot_wall_ns", "hot_process_cpu_ns"):
            duration = child_data.get(key)
            if not isinstance(duration, int) or duration <= 0:
                record["errors"].append(f"child {key} is not a positive duration")
    load_after = record["load_after"]
    record["load_guard_rejected"] = load_before[0] >= MAX_LOAD or load_after[0] >= MAX_LOAD
    if record["load_guard_rejected"]:
        record["errors"].append(f"one-minute load reached >= {MAX_LOAD}")
    if mode == "stat":
        parsed = parse_stat(rawstat, events)
        record["stat_validation"] = parsed
        if not parsed["valid"]:
            record["errors"].extend(parsed["errors"])
        if child_data and child_data.get("hot_loops", 0) > 0:
            for item in parsed["events"]:
                if item.get("valid"):
                    item["count_per_loop"] = float(item["counter_value"]) / loops
    else:
        if not binary.is_file() or binary.stat().st_size == 0:
            record["errors"].append("perf record produced no nonempty perf.data")
        else:
            buildid_command = [perf_path, "buildid-list", "-i", str(binary), "--with-hits"]
            buildid_rc, buildid_out, buildid_err, buildid_timeout = run_capture(
                buildid_command, env
            )
            buildid_out_path = output / f"{stem}-buildids.txt"
            buildid_err_path = output / f"{stem}-buildids-stderr.txt"
            buildid_out_path.write_text(buildid_out, encoding="utf-8")
            buildid_err_path.write_text(buildid_err, encoding="utf-8")
            record["buildid_list"] = {
                "command": buildid_command, "returncode": buildid_rc,
                "timed_out": buildid_timeout, "stdout": str(buildid_out_path),
                "stderr": str(buildid_err_path),
            }
            if buildid_rc != 0 or buildid_timeout or not buildid_out.strip():
                record["errors"].append("perf buildid-list failed or returned no hit modules")
    record["valid"] = not record["errors"]
    write_json(output / f"{stem}-result.json", record)
    return record


def collect(args) -> int:
    if args.output.exists():
        raise FileExistsError(f"refusing to overwrite output directory: {args.output}")
    live = {"upstream": build_path(args.upstream), "candidate": build_path(args.candidate)}
    identity = validate_builds(live["upstream"], live["candidate"], args.identity_profiles)
    timing = medians(args.medians)
    roster = benchmark_roster()
    if args.empty and args.mode != "stat":
        raise ValueError("--empty is supported only with --mode stat")
    if shutil.which("perf") is None:
        raise FileNotFoundError("perf not found on PATH")
    perf_path = str(Path(shutil.which("perf")).resolve())
    env = child_environment()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    write_json(output / "identity-before.json", identity)
    shutil.copy2(args.medians, output / "round-medians.json")
    roles = list(ROLES)
    cases = [case for case in CASES if args.case is None or case == args.case]
    if not cases:
        raise ValueError(f"unknown case {args.case}")
    invocations = []
    if args.mode == "stat":
        schedule = []
        for round_index in range(3):
            case_order = cases[round_index % len(cases):] + cases[:round_index % len(cases)]
            for case_index, case in enumerate(case_order):
                role_offset = (round_index + case_index) % len(roles)
                role_order = roles[role_offset:] + roles[:role_offset]
                for role_index, role in enumerate(role_order):
                    groups = list(EVENT_GROUPS)
                    offset = (round_index + case_index + role_index) % len(groups)
                    group_order = groups[offset:] + groups[:offset]
                    for group in group_order:
                        schedule.append((round_index + 1, case, role, group))
    else:
        schedule = []
        for case_index, case in enumerate(cases):
            role_offset = case_index % len(roles)
            role_order = roles[role_offset:] + roles[:role_offset]
            schedule.extend((1, case, role, None) for role in role_order)
    window_deadline = time.monotonic() + 45 * 60
    fatal_error = None
    identity_after = None
    identity_error = None
    expired = False
    try:
        for round_no, case, role, group in schedule:
            median = timing[role][case]
            target_ns = STAT_TARGET_NS if args.mode == "stat" else RECORD_TARGET_NS
            loops = 0 if args.empty else max(1, math.ceil(target_ns / median))
            waits, load_before, wait_expired = wait_for_quiet_load(window_deadline)
            stem_event, events = group if group is not None else ("cycles-record", ("cycles:u",))
            if wait_expired:
                record = {"round": round_no, "role": role, "case": case,
                          "mode": args.mode, "event_group": stem_event,
                          "calibration": args.empty, "valid": False,
                          "errors": ["measurement window expired during load wait"],
                          "load_wait": waits, "load_before": load_before,
                          "hot_loops": loops, "median_ns_per_loop": median}
                invocations.append(record)
                expired = True
                write_json(output / "invocations.json", invocations)
                break
            try:
                record = make_invocation(
                    output, role, case, round_no, stem_event, events, args.mode,
                    live[role], roster[case], loops, env, median, load_before,
                    waits, perf_path, empty=args.empty,
                )
            except Exception as exc:
                record = {"round": round_no, "role": role, "case": case,
                          "mode": args.mode, "event_group": stem_event,
                          "calibration": args.empty, "valid": False,
                          "errors": [f"invocation aborted: {type(exc).__name__}: {exc}"]}
                fatal_error = record["errors"][0]
            invocations.append(record)
            write_json(output / "invocations.json", invocations)
            if fatal_error:
                break
    except BaseException as exc:
        fatal_error = f"collection interrupted: {type(exc).__name__}: {exc}"
        raise
    finally:
        write_json(output / "invocations.json", invocations)
        try:
            identity_after = validate_builds(
                live["upstream"], live["candidate"], args.identity_profiles
            )
            write_json(output / "identity-after.json", identity_after)
        except Exception as exc:
            identity_error = f"postflight identity check failed: {type(exc).__name__}: {exc}"
            write_json(output / "identity-after-error.json", {"error": identity_error})
        identity_changed = identity_after is None or identity != identity_after
        failures = sum(not item.get("valid", False) for item in invocations)
        complete = len(invocations) == len(schedule) and not expired and fatal_error is None
        status = {
            "mode": args.mode, "case_filter": args.case,
            "calibration": args.empty,
            "qualification": "diagnostic", "eligible_for_R27": False,
            "invocations": len(invocations), "scheduled_invocations": len(schedule),
            "failed_invocations": failures, "schedule_complete": complete,
            "identity_unchanged": not identity_changed,
            "identity_error": identity_error, "fatal_error": fatal_error,
            "status": "diagnostic-complete" if (
                complete and failures == 0 and not identity_changed
                and identity_error is None
            ) else "failed",
            "event_running_threshold_percent": MIN_PCT_RUNNING,
            "load_reject_threshold_1m": MAX_LOAD,
            "load_wait_resume_below_1m": 3.0,
        }
        write_json(output / "status.json", status)
    return 0 if status["status"] == "diagnostic-complete" else 1


def main(argv: list[str] | None = None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    if argv and argv[0] == "--child":
        return child_entry(argv)
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--medians", type=Path, required=True)
    parser.add_argument("--upstream", type=Path, required=True)
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--mode", choices=("stat", "record"), required=True)
    parser.add_argument("--identity-profiles", type=Path, default=PROFILE_IDENTITY_DIR)
    parser.add_argument("--case", choices=CASES,
                        help="limit collection to one reviewed kernel for a control run")
    parser.add_argument("--empty", action="store_true",
                        help="collect an empty hot-region calibration (stat mode only)")
    return collect(parser.parse_args(argv))


if __name__ == "__main__":
    raise SystemExit(main())
