"""Retain identities and small artifacts for the immutable lookup step."""

import hashlib
import json
from pathlib import Path
import shutil
import subprocess


def digest(data):
    return hashlib.sha256(data).hexdigest()


repo = next(p for p in Path(__file__).resolve().parents
            if (p / "Objects/codeobject.c").is_file())
retained = Path(__file__).resolve().parent
out = repo / "build/H3-1-stage2b2a"
baseline = "617bea5be1b1b6b0d368e9ee8c4ca51eb18f7d5d"
native_parent = "c5cf6ba6d2888ebbad085e990cf47e156a079780"


def original(path):
    return subprocess.check_output(["git", "show", baseline + ":" + path], cwd=repo)


prior = json.loads((repo / "boil/design/work/controls/H3-1-stage2b1/final-provenance.json").read_text())
owned = {"Objects/codeobject.c", "Include/internal/pycore_code.h",
         "Modules/_testinternalcapi.c", "Lib/test/test_code.py"}
changed = subprocess.check_output(["git", "diff", "--name-only", baseline],
                                  cwd=repo, text=True).splitlines()
assert {path for path in changed if not path.startswith("boil/")} == owned
sources = set(prior["sources"]) | owned | {"Python/instrumentation.c"}
for path in sources - owned:
    assert (repo / path).read_bytes() == original(path), path
capi = (repo / "Modules/_testinternalcapi.c").read_text()
start = capi.index("static PyObject *\ncode_safe_addr2line(")
end = capi.index("/* Set the farthest exit of FOR_ITER", start)
fixture = capi[start:end]
entry = '    {"code_safe_addr2line", code_safe_addr2line, METH_VARARGS, NULL},\n'
assert capi.count(entry) == 1
assert (capi[:start] + capi[end:]).replace(entry, "").encode() == original("Modules/_testinternalcapi.c")
manifest = {
    "tested_base": baseline, "native_parent": native_parent,
    "tested_tree": "base plus recorded source hashes; evidence commit follows",
    "sources": {path: digest((repo / path).read_bytes()) for path in sorted(sources)},
    "capi_fixture_sha256": digest(fixture.encode()),
    "shared_scope": "exact additive safe-lookup function and method entry; all prior fixtures identical",
    "builds": {}, "products": {},
    "control_sources": {path.name: digest(path.read_bytes()) for path in retained.glob("*.py")},
}
for build, paths in prior["builds"].items():
    manifest["builds"][build] = {path: digest((repo / path).read_bytes()) for path in paths}
    checks = json.loads((out / ("line-controls-" + build) / "manifest.json").read_text())
    assert checks["baseline"] == baseline
    for path, expected in checks["inputs"].items():
        assert digest(Path(path).read_bytes()) == expected, path
    assert checks["old_codeobject_sha256"] == digest(original("Objects/codeobject.c"))
first = json.loads((repo / "boil/design/work/controls/H3-1-stage1/final-provenance.json").read_text())
for path in first["products"]:
    before = digest(original(path))
    after = digest((repo / path).read_bytes())
    assert before == after, path
    manifest["products"][path] = {"parent": before, "current": after, "unchanged": True}
workers = [json.loads(line) for line in (out / "kill-workers.jsonl").read_text().splitlines()]
assert len(workers) == 11
assert all(row["mode"] == [2, 0] and row["entrypoint"] == "worker.main" for row in workers)
debug_capi = next(path for path in manifest["builds"]["debug"] if "_testinternalcapi" in path)
assert all(Path(row["capi_origin"]) == (repo / debug_capi).resolve()
           and row["capi_sha256"] == manifest["builds"]["debug"][debug_capi]
           for row in workers)
manifest["workers"] = workers
assert (out / "pre-regen.sha256").read_bytes() == (out / "post-regen.sha256").read_bytes()
(retained / "final-provenance.json").write_text(json.dumps(manifest, indent=2) + "\n")
logs = ("default-gate0.log", "debug-gate0.log", "default-new-tests.log",
        "default-gate0-final.log", "debug-gate0-final.log",
        "default-targeted-final.log", "debug-targeted-final.log", "debug-kill-final.log",
        "kill-workers.jsonl", "root-stats-check.log", "default-controls.log", "debug-controls.log",
        "pre-eof-rebuild.sha256", "eof-rebuild-identity.log", "default-eof-rebuild.log",
        "debug-eof-rebuild.log", "pre-regen.sha256", "post-regen.sha256", "regen.log",
        "unbuilt.log", "safe-lookup-fixture-proposal.patch")
for name in logs:
    if "-final.log" in name:
        assert "Result: SUCCESS" in (out / name).read_text(), name
    shutil.copyfile(out / name, retained / name)
for build in ("default", "debug"):
    for path in (out / ("line-controls-" + build)).rglob("*"):
        if not path.is_file() or path.suffix == ".so":
            continue
        target = retained / path.relative_to(out)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(path, target)
print("PASS: exact scope, final control/build identities and seven unchanged products")
