"""Retain bounded H3 storage evidence without overwriting prior artifacts."""

import ast
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess


def digest(data):
    return hashlib.sha256(data).hexdigest()


repo = next(p for p in Path(__file__).resolve().parents
            if (p / "Python/bytecodes.c").is_file())
out = repo / "build/H3-1-stage2b2b"
retained = repo / "boil/design/work/controls/H3-1-stage2b2b"
freeze = json.loads((out / "combined-sources.json").read_text())
baseline = freeze["baseline"]


def original(name):
    return subprocess.check_output(["git", "show", baseline + ":" + name], cwd=repo)


sources = {name: digest((repo / name).read_bytes()) for name in freeze["sources"]}
assert sources == freeze["sources"]
native_diff = subprocess.check_output(
    ["git", "diff", "--binary", baseline, "--", *sources], cwd=repo)
assert digest(native_diff) == freeze["diff_sha256"]
changed = subprocess.check_output(["git", "diff", "--name-only", baseline],
                                  cwd=repo, text=True).splitlines()
assert {name for name in changed if not name.startswith("boil/")} == set(sources)

# Existing shared fixtures must be byte-identical outside the exact grant.
capi = (repo / "Modules/_testinternalcapi.c").read_text()
start = capi.index("static PyObject *\nmonitoring_storage_roundtrip(")
end = capi.index("/* Set the farthest exit of FOR_ITER", start)
fixture = capi[start:end]
stripped = capi[:start] + capi[end:]
include = '#include "pycore_instruments.h"  // monitoring storage and deinstrumentation\n'
assert stripped.count(include) == 1
stripped = stripped.replace(include, "")
for name in ("monitoring_storage_roundtrip", "monitoring_deinstrument", "monitoring_state"):
    entry = f'    {{"{name}", {name}, METH_VARARGS, NULL}},\n'
    assert stripped.count(entry) == 1
    stripped = stripped.replace(entry, "")
assert stripped.encode() == original("Modules/_testinternalcapi.c")


def function(text, signature):
    start = text.index(signature)
    return text[start:text.index("\n}\n", start) + 3]


safe = function((repo / "Objects/codeobject.c").read_text(), "int\n_PyCode_SafeAddr2Line(")
assert safe == function(original("Objects/codeobject.c").decode(), "int\n_PyCode_SafeAddr2Line(")
testfile = "Lib/test/test_generated_cases.py"


def checker_class(text):
    tree = ast.parse(text)
    node = next(n for n in tree.body if isinstance(n, ast.ClassDef) and n.name == "TestRootCheckC")
    return "\n".join(text.splitlines()[node.lineno - 1:node.end_lineno])


assert checker_class((repo / testfile).read_text()) == checker_class(original(testfile).decode())
prior = json.loads((repo / "boil/design/work/controls/H3-1-stage2b2a/final-provenance.json").read_text())
unchanged = set(prior["sources"]) - set(sources)
for name in unchanged:
    assert (repo / name).read_bytes() == original(name), name
workers = [json.loads(line) for line in (out / "kill-workers.jsonl").read_text().splitlines()]
assert len(workers) == 11
assert {name for row in workers for name in row["tests"]} == {
    "test_generated_cases", "test_dis", "test_ir", "test__opcode", "test_peepholer",
    "test_compile", "test_monitoring", "test_code", "test_marshal", "test_sys_settrace",
    "test_sys_setprofile"}
assert all(row["mode"] == [2, 0] and row["optimize"] == 0 and row["__debug__"]
           and row["stress"] == 0 and row["entrypoint"] == "worker.main" for row in workers)
builds = {}
for build in ("default", "debug"):
    base = repo / "build" / build
    paths = [base / "python", base / "pyconfig.h", base / "Makefile"]
    extensions = list((base / "Modules").glob("_testinternalcapi*.so"))
    assert len(extensions) == 1
    paths += extensions
    builds[build] = {str(path.relative_to(repo)): digest(path.read_bytes()) for path in paths}
    for label in ("allocation-final", "synthetic-final", "no-monitor-final", "global-oom-final",
                  "storage-controls-final2", "eligibility-combined"):
        manifest = json.loads((out / f"{label}-{build}.json").read_text())
        assert manifest["returncode"] == 0
        assert manifest["before"] == manifest["after"]
        for path, expected in manifest["after"].items():
            assert digest(Path(path).read_bytes()) == expected, path
    for name, expected in json.loads((out / f"allocation-final-{build}/manifest.json").read_text())["identities_before"].items():
        assert digest(Path(name).read_bytes()) == expected, name
    for suffix in ("smoke", "targeted"):
        assert "Result: SUCCESS" in (out / f"combined-{suffix}-{build}.log").read_text()
assert "Result: SUCCESS" in (out / "combined-kill-debug.log").read_text()
for row in workers:
    assert Path(row["executable"]).resolve() == (repo / "build/debug/python").resolve()
    assert row["capi_sha256"] == digest(Path(row["capi_origin"]).read_bytes())
closure = json.loads((out / "codegen-closure-v2.json").read_text())
assert closure["before"] == closure["after"]
assert closure["after"]["sources"] == sources
assert len(closure["marked_sources"]) == 13
unbuilt = (out / "combined-unbuilt.log").read_text()
assert len([line for line in unbuilt.splitlines() if line.startswith("ok   ")]) == 19
assert "FAIL" not in unbuilt
stats = re.fullmatch(r"roots: (\d+) processes, (\d+) collections, fallbacks (\d+), unpinned (\d+) \(0\.00 per collection\)\n",
                     (out / "combined-root-stats-check.log").read_text())
assert stats is not None
counts = dict(zip(("processes", "collections", "fallbacks", "unpinned"), map(int, stats.groups())))
assert counts == {"processes": 40, "collections": 839, "fallbacks": 0, "unpinned": 0}
products = {}
for name in prior["products"]:
    before, after = digest(original(name)), digest((repo / name).read_bytes())
    products[name] = {"parent": before, "current": after, "unchanged": before == after}
assert {name for name, row in products.items() if not row["unchanged"]} == {
    "Include/internal/pycore_ir_metadata.h", "Python/generated_ir_cases.c.h"}

files = [
    "allocation_probe.c", "assert-kill-worker.py", "check_transitions.py", "close_codegen.py",
    "close_codegen_v2.py", "record_provenance.py", "combined_continuation.sh", "combined_qualification.sh",
    "eligibility_probe.py", "eligibility_probe_v1.py", "eligibility-proposal.md", "eligibility-proposal.patch",
    "eligibility-proposal-v3.md", "eligibility-proposal-v3.patch", "global_oom.py", "no_monitoring_flag.py",
    "run_allocation_probe.py", "run_allocation_probe_final.py", "run_ast_current.py", "run_ast_final.py",
    "run_frozen_probe.py", "run_storage_controls.py", "run_storage_controls_v2.py", "storage_controls_final.py",
    "storage_controls_final_v2.py", "storage_probe.py", "synthetic_lines.py", "synthetic_lines_final.py",
    "frozen-sources.json", "frozen-production.patch", "refrozen-sources.json", "refrozen-production.patch",
    "combined-sources.json", "combined-production.patch", "bookkeeping-fix.patch", "static-symbol-sizes.log",
    "codegen-closure-v2.json", "root-source-list.json", "kill-workers.jsonl", "roots-combined.jsonl",
]
files += [path.name for path in out.glob("*.log") if path.name != "retain-evidence.log"]
files += [path.name for path in out.glob("*.json") if path.name not in files]
directories = [
    "old-default", "old-debug", "new-default", "new-debug", "new-ast-default", "new-ast-debug",
    "allocation-default", "allocation-debug", "allocation-final-default", "allocation-final-debug",
    "final-ast-default", "final-ast-debug", "final-storage-default", "final2-storage-default", "final2-storage-debug",
]
for name in directories:
    files += [str(path.relative_to(out)) for path in (out / name).rglob("*")
              if path.is_file() and path.suffix not in (".so", ".pyc") and "pycache-" not in str(path)]
for name in sorted(set(files)):
    source, destination = out / name, retained / name
    destination.parent.mkdir(parents=True, exist_ok=True)
    if destination.exists():
        assert destination.read_bytes() == source.read_bytes(), name
    else:
        shutil.copyfile(source, destination)
manifest = {
    "tested_base": baseline, "native_baseline": freeze["native_baseline"],
    "tested_tree": "base plus exact frozen source hashes; evidence commit follows",
    "combined_manifest_sha256": digest((out / "combined-sources.json").read_bytes()),
    "native_diff_sha256": digest(native_diff), "sources": sources,
    "unchanged_source_hashes": {name: digest((repo / name).read_bytes()) for name in sorted(unchanged)},
    "safe_addr2line_body_sha256": digest(safe.encode()),
    "capi_fixture_sha256": digest(fixture.encode()),
    "shared_scope": "one include + three additive VARARGS functions/entries; all prior bytes unchanged",
    "checker_class_sha256": digest(checker_class((repo / testfile).read_text()).encode()),
    "builds": builds, "products": products, "workers": workers,
    "root_stats": counts,
    "artifact_hashes": {name: digest((retained / name).read_bytes()) for name in sorted(set(files))},
}
(retained / "final-provenance.json").write_text(json.dumps(manifest, indent=2) + "\n")
print("PASS: frozen scope, both binaries/controls, workers, shared fixtures and products; evidence retained")
