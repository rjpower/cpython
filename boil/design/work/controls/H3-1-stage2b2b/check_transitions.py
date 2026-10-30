"""Isolated real monitoring membership/OOM controls on one frozen build."""

import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


def child(case):
    import gc
    import resource
    import _testcapi
    import _testinternalcapi
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    state = (sys.flags.optimize, __debug__, _testinternalcapi.gc_verify_mode(),
             _testinternalcapi.gc_get_stress())
    assert state == (0, True, (0, 0), 0), state
    namespace = {}
    exec(compile("def f(x):\n    y = x + 1\n    return y * 2\n", "<membership>", "exec"), namespace)
    f = namespace["f"]
    code = f.__code__
    assert f(3) == 8
    mon = sys.monitoring
    a, b = mon.PROFILER_ID, mon.COVERAGE_ID
    mon.use_tool_id(a, "existing tool")
    mon.use_tool_id(b, "new tool")
    first, second = [], []

    def record_a(current, offset):
        if current is code:
            first.append(offset)

    def record_b(current, offset):
        if current is code:
            second.append(offset)

    event = mon.events.INSTRUCTION
    mon.register_callback(a, event, record_a)
    mon.register_callback(b, event, record_b)
    try:
        if case == "oom":
            gc.collect()
            old = mon.get_local_events(a, code)
            words = _testinternalcapi.get_ir(code, True)
            requested = mon.events.LINE | event
            try:
                _testcapi.call_with_nomemory(1, 0, mon.set_local_events,
                                             a, code, requested)
            except MemoryError:
                current = mon.get_local_events(a, code)
                print(json.dumps({"case": case, "state": state, "old": old,
                                  "requested": requested, "after": current,
                                  "words_unchanged": words == _testinternalcapi.get_ir(code, True)}), flush=True)
                assert current == old, ("local request changed on OOM", old, current)
                mon.set_local_events(a, code, requested)
                f(3)
                assert first, "same-request retry failed to install INSTRUCTION"
            else:
                raise AssertionError("allocation ordinal did not fail")
            return
        initial = event | (mon.events.LINE if case == "other_event" else 0)
        mon.set_local_events(a, code, initial)
        f(3)
        expected = first[:]
        assert expected
        first.clear()
        added = mon.events.LINE if case == "other_event" else event
        mon.register_callback(b, added, record_b)
        mon.set_local_events(b, code, added)
        assert f(3) == 8
        print(json.dumps({"case": case, "state": state, "before": expected,
                          "after": first, "second": second}), flush=True)
        assert first == expected, ("lost existing instruction membership", expected, first)
    finally:
        for tool in (a, b):
            mon.set_local_events(tool, code, 0)
            mon.free_tool_id(tool)


def main(build, label):
    repo = next(p for p in Path(__file__).resolve().parents if (p / "Python/bytecodes.c").exists())
    out = repo / "build/H3-1-stage2b2b" / (label + "-" + build)
    out.mkdir(parents=True, exist_ok=True)
    binary = repo / "build" / build / "python"
    def sha(path):
        return hashlib.sha256(path.read_bytes()).hexdigest()
    paths = ["Python/instrumentation.c", "Objects/codeobject.c",
             "Include/internal/pycore_instruments.h", "Python/legacy_tracing.c"]
    source = {name: sha(repo / name) for name in paths}
    manifest = {"head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=repo, text=True).strip(),
                "binary_sha256": sha(binary), "config_sha256": sha(binary.parent / "pyconfig.h"),
                "script_sha256": sha(Path(__file__)), "sources": source, "cases": {}}
    env = {key: value for key, value in os.environ.items()
           if not key.startswith("PYTHON_GC") and key != "PYTHONOPTIMIZE"}
    for case in ("same_event", "other_event", "oom"):
        with tempfile.TemporaryDirectory(prefix="pycache-", dir=out) as cache:
            command = [str(binary), "-X", "pycache_prefix=" + cache,
                       str(Path(__file__).resolve()), "--child", case]
            result = subprocess.run(command, env=env, capture_output=True, text=True, timeout=120)
        log = out / (case + ".log")
        log.write_text(result.stdout + result.stderr)
        manifest["cases"][case] = {"returncode": result.returncode, "log_sha256": sha(log), "command": command}
        print(build, label, case, result.returncode, flush=True)
    assert source == {name: sha(repo / name) for name in paths}
    assert manifest["binary_sha256"] == sha(binary)
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")


if __name__ == "__main__":
    if sys.argv[1] == "--child":
        child(sys.argv[2])
    else:
        assert sys.argv[3:] == ["--", "-m", "test"]
        main(sys.argv[1], sys.argv[2])
