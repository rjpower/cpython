"""Bounded diagnostic, run once per case/build outside performance measurements.

Use the target interpreter with -S -B and PYTHONHASHSEED=0. Scope comprises
module-defined functions/class descriptors and their nested code constants;
logging_silent also includes stdlib logging. This is not an execution census.
"""

import argparse
import dis
import hashlib
import importlib.util
import json
import os
import re
import subprocess
import sys
import sysconfig
import types
from collections import Counter
from pathlib import Path

import _testinternalcapi


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def git(source, *args):
    return subprocess.check_output(
        ["git", "-C", str(source), *args], text=True
    ).strip()


def identity(source, expected):
    source = source.resolve()
    assert not sys._jit.is_enabled(), "JIT must be disabled"
    assert sysconfig.get_config_var("Py_DEBUG") == 0, "release build required"
    assert sys.flags.debug == 0 and sys.flags.hash_randomization == 0, sys.flags
    assert sys.flags.no_site == 1 and sys.flags.dont_write_bytecode == 1, "use -S -B"
    magic_header = source / "Include/internal/pycore_magic_number.h"
    magic = int(re.search(r"^#define PYC_MAGIC_NUMBER (\d+)$",
                          magic_header.read_text(), re.M).group(1))
    assert importlib.util.MAGIC_NUMBER == magic.to_bytes(2, "little") + b"\r\n"
    head = git(source, "rev-parse", "HEAD")
    assert head == expected, (head, expected)
    status = git(source, "status", "--porcelain", "--untracked-files=no")
    assert not status, status
    embedded = getattr(sys, "_git", ())
    hashes = [v for v in embedded if re.fullmatch(r"[0-9a-f]{7,40}", v)]
    assert hashes and all(expected.startswith(v) for v in hashes), embedded
    executable = Path(sys.executable).resolve()
    makefile = (executable.parent / "Makefile").read_text()
    configured_source = Path(re.search(r"^abs_srcdir\s*=\s*(.+)$", makefile, re.M).group(1)).resolve()
    assert configured_source == source, (configured_source, source)
    library = source / "Lib"
    path_libraries = [str(Path(p).resolve()) for p in sys.path if Path(p).name == "Lib"]
    assert path_libraries == [str(library)], path_libraries
    assert Path(dis.__file__).resolve() == library / "dis.py", dis.__file__
    notes = subprocess.check_output(["readelf", "-n", str(executable)], text=True)
    return {
        "head": head, "tracked_status": status, "embedded_git": embedded,
        "executable": str(executable),
        "executable_sha256": hashlib.sha256(executable.read_bytes()).hexdigest(),
        "build_id": re.search(r"Build ID: (\w+)", notes).group(1),
        "version": sys.version, "prefix": sys.prefix,
        "jit_enabled": sys._jit.is_enabled(), "magic_number": importlib.util.MAGIC_NUMBER.hex(),
        "hash_randomization": sys.flags.hash_randomization,
        "no_site": sys.flags.no_site, "dont_write_bytecode": sys.flags.dont_write_bytecode,
        "affinity": sorted(os.sched_getaffinity(0)),
        "config_flags": {name: sysconfig.get_config_var(name) for name in
                         ("Py_DEBUG", "Py_GIL_DISABLED", "CONFIG_ARGS", "PY_CFLAGS",
                          "PY_CFLAGS_NODIST", "PY_LDFLAGS")},
        "configured_source": str(configured_source), "sys_path": sys.path,
        "dis_origin": dis.__file__,
        "build_files_sha256": {
            name: hashlib.sha256((executable.parent / name).read_bytes()).hexdigest()
            for name in ("pyconfig.h", "Makefile", "config.status")
            if (executable.parent / name).is_file()
        },
    }


def scoped_codes(module):
    result = set()
    visited = set()

    def code_tree(code):
        if code in result:
            return
        result.add(code)
        for constant in code.co_consts:
            if isinstance(constant, types.CodeType):
                code_tree(constant)

    def visit(value):
        if id(value) in visited:
            return
        visited.add(id(value))
        if isinstance(value, types.FunctionType):
            if value.__module__ == module.__name__:
                code_tree(value.__code__)
        elif isinstance(value, type) and value.__module__ == module.__name__:
            for member in vars(value).values():
                visit(member)
        elif isinstance(value, (staticmethod, classmethod)):
            visit(value.__func__)
        elif isinstance(value, property):
            for accessor in (value.fget, value.fset, value.fdel):
                if accessor is not None:
                    visit(accessor)

    for value in vars(module).values():
        visit(value)
    return result


def inspect_code(code):
    before = code._co_feedback
    fbinit = code.co_fbinit
    instructions = list(dis.get_instructions(code, adaptive=False))
    handlers = _testinternalcapi.get_ir(code, True)
    sites = []
    for instruction in instructions:
        if instruction.opname not in {"LOAD_ATTR", "STORE_ATTR", "LOAD_SUPER_ATTR"}:
            continue
        fb = instruction.fb
        entry = dis._feedback_entry(fbinit, fb) if fb else None
        sites.append({
            "offset": instruction.offset, "operation": instruction.opname,
            "feedback_unit": fb,
            "admission": "shared" if fb == 0 else "private" if fb else "none",
            "planned_bytes": entry[2] * 8 if entry else 0,
            "handler": handlers[instruction.offset][0] if before is not None else None,
        })
    after = code._co_feedback
    assert (before is None) == (after is None), code.co_qualname
    assert before == after, code.co_qualname
    words = [int.from_bytes(fbinit[i:i + 4], sys.byteorder)
             for i in range(0, len(fbinit), 4)]
    return {
        "file": code.co_filename, "qualname": code.co_qualname,
        "firstlineno": code.co_firstlineno,
        "planned_entry_bytes": (words[0] & 65535) * 8 if words else 0,
        "allocated_entry_bytes": len(before) if before is not None else 0,
        "executing_feedback_present": before is not None, "attr_sites": sites,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", required=True,
                        choices=["class_attr_load", "logging_silent", "deltablue", "coroutines"])
    parser.add_argument("--runtime-source", required=True, type=Path)
    parser.add_argument("--expected-head", required=True)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    pre = identity(args.runtime_source, args.expected_head)
    bench_dir = Path(__file__).resolve().parents[2] / "bench"
    registry = load("_capacity_registry", bench_dir / "run.py")
    driver = load("_capacity_driver", bench_dir / "internal" / "driver.py")
    bench = next(b for b in registry.BENCHMARKS if b.name == args.case)
    module = driver.load(bench.path)
    if "setup" in vars(module):
        module.setup(bench.variant)
    modules = [module]
    if args.case == "logging_silent":
        import logging
        assert Path(logging.__file__).resolve() == args.runtime_source.resolve() / "Lib/logging/__init__.py"
        modules.append(logging)
    codes = set().union(*(scoped_codes(m) for m in modules))
    ordered = sorted(codes, key=lambda c: (c.co_filename, c.co_firstlineno, c.co_qualname))
    input_paths = {bench.path, bench_dir / "run.py", bench_dir / "internal" / "driver.py",
                   Path(__file__), Path(dis.__file__),
                   args.runtime_source / "Include/internal/pycore_magic_number.h",
                   *(Path(c.co_filename) for c in codes)}
    inputs_pre = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in input_paths}
    cold = [inspect_code(c) for c in ordered]
    module.run(bench.loops)
    warm = [inspect_code(c) for c in ordered]
    assert codes == set().union(*(scoped_codes(m) for m in modules)), "scope changed"
    post = identity(args.runtime_source, args.expected_head)
    assert pre == post, "runtime identity changed"
    inputs_post = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in input_paths}
    assert inputs_pre == inputs_post, "input source changed during census"
    totals = {}
    for stage, rows in (("after_setup", cold), ("after_one_warmup", warm)):
        sites = [s for r in rows for s in r["attr_sites"]]
        totals[stage] = {
            "code_objects": len(rows), "attr_admission": dict(Counter(s["admission"] for s in sites)),
            "attr_handlers_existing_feedback": dict(Counter(s["handler"] for s in sites if s["handler"])),
            "planned_attr_bytes": sum(s["planned_bytes"] for s in sites),
            "planned_entry_bytes": sum(r["planned_entry_bytes"] for r in rows),
            "allocated_entry_bytes": sum(r["allocated_entry_bytes"] for r in rows),
        }
    report = {
        "case": args.case, "loops": bench.loops, "variant": bench.variant,
        "identity_pre": pre, "identity_post": post,
        "inputs_sha256_pre": inputs_pre, "inputs_sha256_post": inputs_post, "totals": totals,
        "after_setup": cold, "after_one_warmup": warm,
        "limits": ["Scope is module-defined descriptors and nested code, excluding module top-level code.",
                   "Logging scope includes dormant code, not all runtime callees or framework code.",
                   "After setup is not a pristine interpreter; setup may already specialize code.",
                   "Allocated entry bytes exclude poly tables, execution words, block headers and allocator overhead.",
                   "Handler presence is a snapshot, not dynamic execution counts; shared sites stay generic.",
                   "Compare only matching source digest/qualname/firstlineno; bytecode differs across layouts."],
    }
    with args.output.open("x") as output:
        json.dump(report, output, indent=2)
        output.write("\n")


if __name__ == "__main__":
    main()
