"""Actual MEM-domain requests and unpublished-allocation cleanup in C setters.

The failmalloc wrapper runs above this recorder. A denied request never reaches
it, so failure ordinals do not identify denied sizes or native source sites.
"""
import ast
import gc
import hashlib
import importlib.util
import json
from pathlib import Path
import shlex
import subprocess
import sys
import sysconfig

import _testinternalcapi as capi
import _testcapi


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def make_function(kind):
    namespace = {}
    if kind == "ordinary":
        exec("def f(x):\n    y = x + 1\n    return y * 2\n", namespace)
    else:
        line = 1 if kind == "backward" else 2**31 - 1
        loc = dict(lineno=line, col_offset=0, end_lineno=line, end_col_offset=1)
        body = ast.Return(value=ast.Constant(value=1, **loc), **loc)
        node = ast.FunctionDef(
            name="f", args=ast.arguments(posonlyargs=[], args=[], kwonlyargs=[],
                                         kw_defaults=[], defaults=[]),
            body=[body], decorator_list=[], lineno=10, col_offset=0,
            end_lineno=max(10, line), end_col_offset=1)
        exec(compile(ast.Module(body=[node], type_ignores=[]), "<allocation-lines>", "exec"), namespace)
    return namespace["f"]


def main():
    repo = next(p for p in Path(__file__).resolve().parents if (p / "Python/instrumentation.c").exists())
    build = Path(sys.executable).resolve().parent
    out = Path(__file__).parent / ("allocation-final-" + build.name)
    out.mkdir(exist_ok=True)
    assert sys.flags.optimize == 0 and __debug__
    assert capi.gc_verify_mode() == (0, 0) and capi.gc_get_stress() == 0
    extension_origin = Path(capi.__file__).resolve()
    assert extension_origin.is_relative_to(build), extension_origin
    source = Path(__file__).with_name("allocation_probe.c")
    native = [repo / name for name in (
        "Python/instrumentation.c", "Python/legacy_tracing.c", "Python/bytecodes.c",
        "Python/generated_ir_cases.c.h", "Objects/codeobject.c", "Modules/_testinternalcapi.c",
        "Include/internal/pycore_instruments.h", "Include/internal/pycore_ir_metadata.h")]
    identities = {str(path): sha(path) for path in native + [Path(sys.executable),
                  build / "pyconfig.h", extension_origin, source, Path(__file__)]}
    extension = out / ("_h3_allocation" + sysconfig.get_config_var("EXT_SUFFIX"))
    command = (shlex.split(sysconfig.get_config_var("LDSHARED"))
               + shlex.split(sysconfig.get_config_var("CFLAGS"))
               + ["-fPIC", "-I" + str(build), "-I" + str(repo / "Include"),
                  "-I" + str(repo / "Include/internal"), "-MMD", "-MF", str(out / "headers.d"),
                  str(source), "-o", str(extension)])
    subprocess.run(command, check=True, timeout=120)
    dependencies = shlex.split((out / "headers.d").read_text().replace("\\\n", " ").split(":", 1)[1])
    headers = {name: sha(Path(name)) for name in dependencies}
    assert str(build / "pyconfig.h") in headers
    assert str(repo / "Include/internal/pycore_instruments.h") in headers
    spec = importlib.util.spec_from_file_location("_h3_allocation", extension)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    manifest = {"identities_before": identities, "consumed_headers": headers,
                "extension_sha256": sha(extension), "compile_command": command,
                "compiler_version": subprocess.check_output(
                    shlex.split(sysconfig.get_config_var("CC")) + ["--version"], text=True),
                "configuration": {key: sysconfig.get_config_var(key) for key in
                                  ("CC", "LDSHARED", "CFLAGS", "EXT_SUFFIX")},
                "cases": [], "denied_requests": "not measured: failmalloc rejects before recorder"}
    mon = sys.monitoring
    a, b = mon.PROFILER_ID, mon.COVERAGE_ID
    mon.use_tool_id(a, "allocation accounting existing")
    mon.use_tool_id(b, "allocation accounting new")
    for kind, transition in (("ordinary", False), ("ordinary", True),
                             ("backward", False), ("high", False)):
        reached_success = False
        for fail in range(-1, 8):
            f = make_function(kind)
            code = f.__code__
            assert f(3) == 8 if kind == "ordinary" else f() == 1
            mon.set_local_events(a, code, mon.events.LINE | mon.events.INSTRUCTION if transition else 0)
            tool = b if transition else a
            args = (tool, code, mon.events.LINE | mon.events.INSTRUCTION)
            before = capi.monitoring_state(code)
            words = capi.get_ir(code, True)
            n = len(code.co_code) // 8
            gc.collect()
            if fail == -1:
                profile = module.profile(mon.set_local_events, args)
            else:
                profile = module.profile(_testcapi.call_with_nomemory,
                                         (fail, 0, mon.set_local_events, *args))
            entries, peak, live, failed, header, untracked_frees = profile
            assert untracked_frees == 0, profile
            if failed:
                assert capi.monitoring_state(code) == before, (kind, transition, fail, profile)
                assert capi.get_ir(code, True) == words
                assert live == 0, ("unpublished allocation leak", profile)
                mon.set_local_events(*args)
            else:
                expected = [n, n, n] if transition else [header + n * 4, n * 2, n * 2]
                assert [entry[0] for entry in entries] == expected, (kind, transition, fail, profile, expected)
                assert peak == live == sum(expected)
                reached_success = fail >= 0
            record = {"kind": kind, "transition": transition, "fail_ordinal": fail, "words": n,
                      "co_lines": list(code.co_lines()), "entries_size_succeeded_live": entries,
                      "peak_requested": peak, "live_requested": live, "failed": failed,
                      "header_size": header, "untracked_frees": untracked_frees}
            manifest["cases"].append(record)
            print(json.dumps(record), flush=True)
            mon.set_local_events(a, code, 0)
            mon.set_local_events(b, code, 0)
            if reached_success:
                break
        assert reached_success, (kind, transition, "failure sweep never reached success")
    mon.free_tool_id(a)
    mon.free_tool_id(b)
    assert identities == {name: sha(Path(name)) for name in identities}
    assert headers == {name: sha(Path(name)) for name in headers}
    manifest["identities_after"] = identities
    manifest["consumed_headers_after"] = headers
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")


if __name__ == "__main__":
    main()
