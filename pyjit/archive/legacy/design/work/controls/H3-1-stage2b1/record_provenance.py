"""Record the frozen candidate and retain small review evidence."""

import hashlib
import json
from pathlib import Path
import shutil
import subprocess


def digest(data):
    return hashlib.sha256(data).hexdigest()


repo = next(p for p in Path(__file__).resolve().parents
            if (p / "Python/bytecodes.c").is_file())
out = repo / "build/H3-1-stage2b1"
retained = Path(__file__).resolve().parent
baseline = "d387aca48a9e6253c8a56fb9056668593d8d5ecf"


def original(path):
    return subprocess.check_output(["git", "show", baseline + ":" + path], cwd=repo)


prior = json.loads((repo / "boil/design/work/controls/H3-1-stage2a/final-provenance.json").read_text())
changed = subprocess.check_output(["git", "diff", "--name-only", baseline], cwd=repo,
                                  text=True).splitlines()
sources = set(prior["sources"])
sources.update(path for path in changed if not path.startswith("boil/"))
sources.update(("Python/regcfg_passes.c", "Lib/test/test_peepholer.py",
                "Lib/dis.py", "Lib/opcode.py", "Modules/_opcode.c"))
capi = (repo / "Modules/_testinternalcapi.c").read_text()
selector = "        const char *name = _PyOpcode_OpName[handler];"
old_selector = '''        const char *name = handler < IR_FIRST_ONLY_HANDLER
            ? _PyOpcode_OpName[handler]
            : _PyIR_OnlyNames[handler - IR_FIRST_ONLY_HANDLER];'''
assert capi.count(selector) == 1
assert capi.replace(selector, old_selector).encode() == original("Modules/_testinternalcapi.c")
tests = (repo / "Lib/test/test_generated_cases.py").read_text()
checker = tests.split("class TestRootCheckC", 1)[1]
assert checker.encode() == original("Lib/test/test_generated_cases.py").split(b"class TestRootCheckC", 1)[1]
marked = (repo / "boil/design/work/controls/H3-1-stage2a/root-source-scan.log").read_text().splitlines()[1:-1]
for path in marked + ["Python/specialize.c", "Python/regcfg_passes.c",
                      "Tools/cases_generator/root_check_c.py", "Tools/cases_generator/call_effects.py"]:
    sources.add(path)
    assert (repo / path).read_bytes() == original(path), path
manifest = {
    "tested_base": baseline,
    "tested_tree": "base plus recorded source hashes; commit follows evidence collection",
    "sources": {path: digest((repo / path).read_bytes()) for path in sorted(sources)},
    "builds": {}, "products": {},
    "shared_file_scope": "CAPI differs only by approved get_ir selector; compiler/scalar/GC fixture remainder identical",
    "root_checker_class_sha256": digest(checker.encode()),
    "control_sources": {path.name: digest(path.read_bytes())
                        for path in sorted(retained.glob("*.py"))},
}
for build in ("default", "debug"):
    manifest["builds"][build] = {
        path: digest((repo / path).read_bytes()) for path in prior["builds"][build]}
for path in prior["products"]:
    before = digest(original(path))
    after = digest((repo / path).read_bytes())
    manifest["products"][path] = {"parent": before, "current": after,
                                    "unchanged": before == after}
assert sum(p["unchanged"] for p in manifest["products"].values()) == 3
workers = [json.loads(line) for line in (out / "kill-workers.jsonl").read_text().splitlines()]
assert len(workers) == 9
assert all(row["mode"] == [2, 0] and row["entrypoint"] == "worker.main" for row in workers)
manifest["workers"] = workers
(retained / "final-provenance.json").write_text(json.dumps(manifest, indent=2) + "\n")
logs = ("baseline-name.log", "baseline-nofb-callback.log",
        "baseline-nofb-binaries.sha256", "baseline-nofb-cases.sha256", "semantic-opcode.patch",
        "default-gate0-final.log", "debug-gate0-final.log", "default-targeted-final.log",
        "debug-targeted-final.log", "debug-kill-final.log", "kill-workers.jsonl",
        "root-stats-check.log", "regen-closure.log", "root-source-scan.log", "unbuilt.log",
        "domain-equality.log", "selection-default-final.log", "selection-debug-final.log",
        "metadata-default-final.log")
for name in logs:
    shutil.copyfile(out / name, retained / name)
for name in ("selection-default", "selection-debug", "metadata-default"):
    source = out / name
    target = retained / name
    for path in source.rglob("*"):
        if not path.is_file():
            continue
        if path.name in ("manifest.json", "output.log", "headers.d", "body.patch", "fixture.c", "metadata.patch"):
            dest = target / path.relative_to(source)
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(path, dest)
print("PASS: candidate provenance, shared ownership and three unchanged products; evidence retained")
