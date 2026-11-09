"""Record real valid-AST line tables before enabling the frozen LINE cache."""

import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def child(case):
    import resource
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    import ast
    import types
    import _testinternalcapi

    if case == "module_zero":
        tree = ast.parse("value = 1\n")
    else:
        first, line = {"backward_9": (10, 1), "backward_2": (10, 8),
                       "extreme_positive": (1, 2**31 - 1)}[case]
        loc = dict(lineno=line, col_offset=0, end_lineno=line, end_col_offset=1)
        statement = ast.Return(value=ast.Constant(value=1, **loc), **loc)
        function = ast.FunctionDef(
            name="f", args=ast.arguments(posonlyargs=[], args=[], kwonlyargs=[],
                                         kw_defaults=[], defaults=[]),
            body=[statement], decorator_list=[], lineno=first, col_offset=0,
            end_lineno=max(first, line), end_col_offset=1)
        tree = ast.Module(body=[function], type_ignores=[])
    print(json.dumps({"phase": "ast", "case": case,
                      "tree": ast.dump(tree, include_attributes=True),
                      "core_limit": resource.getrlimit(resource.RLIMIT_CORE)}), flush=True)
    compiled = compile(tree, "<h3-line-codec>", "exec")
    code = compiled if case == "module_zero" else next(
        value for value in compiled.co_consts if isinstance(value, types.CodeType))
    lines = list(code.co_lines())
    expected = {index: -1 if line is None else line
                for start, end, line in lines for index in range(start, end)}
    actual = {index: _testinternalcapi.code_safe_addr2line(code, index)
              for index in expected}
    assert actual == expected, (actual, expected)
    extension = Path(_testinternalcapi.__file__).resolve()
    assert extension.is_relative_to(Path(sys.executable).resolve().parent), extension
    print(json.dumps({"phase": "compiled", "case": case,
                      "firstlineno": code.co_firstlineno, "co_lines": lines,
                      "safe_lines": actual, "words": _testinternalcapi.get_ir(code),
                      "code_sha256": hashlib.sha256(code.co_code).hexdigest(),
                      "linetable_sha256": hashlib.sha256(code.co_linetable).hexdigest(),
                      "capi_origin": str(extension), "capi_sha256": digest(extension)}), flush=True)
    mon = sys.monitoring
    tool = mon.PROFILER_ID
    mon.use_tool_id(tool, "line codec source audit")
    events = []

    def callback(current, line):
        if current is code:
            events.append(line)

    mon.register_callback(tool, mon.events.LINE, callback)
    try:
        print(json.dumps({"phase": "enable", "case": case}), flush=True)
        mon.set_local_events(tool, code, mon.events.LINE)
        print(json.dumps({"phase": "enabled", "case": case}), flush=True)
        if case == "module_zero":
            namespace = {}
            exec(code, namespace)
            assert namespace["value"] == 1
        else:
            assert types.FunctionType(code, {})() == 1
        print(json.dumps({"phase": "executed", "case": case,
                          "events": events}), flush=True)
    finally:
        mon.set_local_events(tool, code, 0)
        mon.register_callback(tool, mon.events.LINE, None)
        mon.free_tool_id(tool)


def main(build):
    repo = next(p for p in Path(__file__).resolve().parents
                if (p / "Objects/codeobject.c").is_file())
    out = repo / "build/H3-1-stage2b2b" / build
    out.mkdir(parents=True, exist_ok=True)
    binary = repo / "build" / build / "python"
    baseline = "dc02f76c0ccf69cd7195431dc0335a349bdec48f"
    paths = ["Objects/codeobject.c", "Python/instrumentation.c",
             "Include/internal/pycore_instruments.h", "Include/internal/pycore_code.h",
             "Modules/_testinternalcapi.c", "Lib/test/test_code.py"]
    source = {}
    for name in paths:
        data = (repo / name).read_bytes()
        assert data == subprocess.check_output(["git", "show", baseline + ":" + name], cwd=repo), name
        source[name] = hashlib.sha256(data).hexdigest()
    manifest = {"baseline": baseline, "build": build, "sources": source,
                "script_sha256": digest(Path(__file__)), "binary_sha256": digest(binary),
                "config_sha256": digest(binary.parent / "pyconfig.h"), "cases": {}}
    for case in ("backward_9", "backward_2", "module_zero", "extreme_positive"):
        with tempfile.TemporaryDirectory(prefix="pycache-", dir=out) as cache:
            command = [str(binary), "-X", "pycache_prefix=" + cache,
                       str(Path(__file__).resolve()), "--child", case]
            result = subprocess.run(command, capture_output=True, text=True, timeout=120)
        (out / (case + ".log")).write_text(result.stdout + result.stderr)
        manifest["cases"][case] = {"returncode": result.returncode,
                                   "command": command,
                                   "log_sha256": digest(out / (case + ".log"))}
        print(build, case, result.returncode, flush=True)
    assert digest(binary) == manifest["binary_sha256"]
    assert all(digest(repo / name) == value for name, value in source.items())
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")


if __name__ == "__main__":
    if sys.argv[1] == "--child":
        child(sys.argv[2])
    else:
        # Trailing '-m test' is the existing gate_run machine-slot admission marker.
        assert sys.argv[2:] == ["--", "-m", "test"], sys.argv
        main(sys.argv[1])
