"""Run stage-1 fault controls against copied generators, never live sources.

Run from the checkout root with uv and an existing qualified default build.
Only the task-owned build/H3-1 output and scratch directories are written.
"""

import difflib
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile

controls=(
 ('duplicate','test_duplicate_id_rejected',
  '        if len(set(ids.values())) != len(ids):\n            raise ValueError("duplicate handler ID")\n',''),
 ('capacity','test_capacity_without_large_tables',
  '    if not 0 < count <= 1 << 16:', '    if not 0 < count:'),
 ('consumer','test_consumers_reject_another_analysis',
  '        if analysis is not self.analysis:\n            raise ValueError("handler domain belongs to a different analysis")', '        pass'),
 ('base','test_base_metadata_rejected',
  '        for name, base in self.bases.items():\n            if base not in ids or self.bases[base] != base:\n                raise ValueError(f"invalid handler base for {name}: {base}")\n', ''),
 ('kind','test_base_does_not_classify_specialization',
  '        kinds[name] |= HandlerKind.BINARY_OP\n',
  '        kinds[name] |= HandlerKind.BINARY_OP | HandlerKind.SPECIALIZED\n'),
 ('pin','test_absent_instrumented_pin',
  '    min_instrumented = 256 - len(set(instrumented) | pinned_instrumented.keys())',
  '    min_instrumented = 256 - len(instrumented)'),
 ('name','test_derived_name_collision_rejected',
  '        if name in opmap or name in handlers:\n            raise ValueError(f"duplicate handler name: {name}")\n', ''),
)

def hashes(paths):
    return {str(path): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in paths}


def main():
    repo = Path.cwd()
    source = Path("Tools/cases_generator/ir_generator.py")
    original = source.read_text()
    generator_paths = sorted(Path("Tools/cases_generator").glob("*.py"))
    inputs = [Path("Python/bytecodes.c"), Path("Include/opcode.h")]
    products = [Path(line.split("  ", 1)[1]) for line in
                (Path(__file__).parent / "baseline-products.sha256")
                .read_text().splitlines()]
    live_paths = generator_paths + inputs + products + [
        Path("Lib/test/test_generated_cases.py")]
    before = hashes(live_paths)
    out = repo / "build/H3-1/isolated-controls"
    out.mkdir(parents=True, exist_ok=True)
    worker = Path(__file__).resolve().with_name("isolated-worker.py")
    python = repo / "build/default/python"
    results = []
    with tempfile.TemporaryDirectory(prefix="control-copy-", dir=out) as tmp:
        copied_root = Path(tmp)
        for path in generator_paths + inputs:
            target = copied_root / path
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(path, target)
        copied_source = copied_root / source
        cases = [("positive", "TestHandlerDomain", original)]
        for label, test, old, new in controls:
            assert original.count(old) == 1, (label, original.count(old))
            variant = original.replace(old, new)
            if label == "name":
                # Both layers reject this conflict; remove both to test the
                # invariant rather than the location of its diagnostic.
                variant = variant.replace(
                    '            if name in ids and ids[name] != handler:\n'
                    '                raise ValueError(f"conflicting handler ID for {name}")\n',
                    "")
            patch = "".join(difflib.unified_diff(
                original.splitlines(True), variant.splitlines(True),
                fromfile="a/" + str(source), tofile="b/" + str(source)))
            (out / (label + ".patch")).write_text(patch)
            cases.append((label, test, variant))
        for label, test, variant in cases:
            copied_source.write_text(variant)
            digest = hashlib.sha256(variant.encode()).hexdigest()
            worker_command = shlex.join([
                str(python), str(worker), str(copied_root), digest])
            command = [
                "uv", "run", "--no-project", "--", "timeout", "-k", "30",
                "2400", "boil/tools/gate_run.sh",
                str(repo / "build/default/.gate.lock"), str(python),
                str(python), str(worker), str(copied_root), digest,
                "-m", "test", "-v", "-j1", "--python",
                worker_command, "-m", "*" + test + "*",
                "test_generated_cases"]
            env = dict(os.environ, UV_CACHE_DIR="/home/power/.cache/uv",
                       PYTHONPYCACHEPREFIX=str(copied_root / "pyc" / label))
            with (out / (label + ".log")).open("w") as stream:
                completed = subprocess.run(
                    command, env=env, stdout=stream,
                    stderr=subprocess.STDOUT, check=False)
            log = (out / (label + ".log")).read_text()
            assert "isolated driver generator verified" in log, (label, log)
            assert "isolated worker generator verified" in log, (label, log)
            if label == "positive":
                assert completed.returncode == 0, log
                assert "Traceback" not in log, log
            else:
                assert completed.returncode == 2 and "FAILED" in log, (label, log)
            result = {"control": label, "test": test,
                      "exit": completed.returncode, "command": command,
                      "source_sha256": digest}
            if label != "positive":
                result["patch_sha256"] = hashlib.sha256(
                    (out / (label + ".patch")).read_bytes()).hexdigest()
            results.append(result)
            assert hashes(live_paths) == before, "live source/product changed"
            print(label, "verified", completed.returncode, flush=True)
    after = hashes(live_paths)
    assert after == before
    manifest = {
        "revision": subprocess.check_output(
            ["git", "rev-parse", "HEAD"], text=True).strip(),
        "runner_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "worker_sha256": hashlib.sha256(worker.read_bytes()).hexdigest(),
        "live_before": before, "live_after": after,
        "scratch_removed": not copied_root.exists(), "controls": results}
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")


if __name__ == "__main__":
    main()
