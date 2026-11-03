"""Close regeneration, generated identity and unchanged-checker evidence."""

import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile


def digest(data):
    return hashlib.sha256(data).hexdigest()


repo = next(p for p in Path(__file__).resolve().parents
            if (p / "Python/bytecodes.c").is_file())
out = Path(__file__).resolve().parent
freeze = json.loads((out / "combined-sources.json").read_text())
binary = repo / "build/default/python"


def snapshot():
    return {
        "sources": {name: digest((repo / name).read_bytes()) for name in freeze["sources"]},
        "diff_sha256": digest(subprocess.check_output(["git", "diff", "--binary"], cwd=repo)),
        "status": subprocess.check_output(["git", "status", "--porcelain"], cwd=repo, text=True).splitlines(),
        "binary_sha256": digest(binary.read_bytes()),
    }


before = snapshot()
assert before["sources"] == freeze["sources"]
env = {key: value for key, value in os.environ.items()
       if not key.startswith("PYTHON_GC") and key != "PYTHONOPTIMIZE"}
env["PYTHONDONTWRITEBYTECODE"] = "1"
commands = []


def run(name, command):
    result = subprocess.run(command, cwd=repo, env=env, capture_output=True,
                            text=True, timeout=600)
    log = out / name
    log.write_text(result.stdout + result.stderr)
    commands.append({"command": command, "returncode": result.returncode,
                     "log": name, "log_sha256": digest(log.read_bytes())})
    print(name, result.returncode, flush=True)
    assert result.returncode == 0, result.stdout + result.stderr


run("combined-regen.log", ["make", "-C", "build/default", "-s", "regen-cases", "clinic"])
assert snapshot() == before
marked = subprocess.check_output(
    ["rg", "-l", "Py_GC_PRECISE|Py_GC_ARGS_ANCHORED", "--glob", "*.[ch]",
     "Python", "Objects", "Modules"], cwd=repo, text=True).splitlines()
marked.sort()
(out / "root-source-list.json").write_text(json.dumps(marked, indent=2) + "\n")
with tempfile.TemporaryDirectory(prefix="pycache-", dir=out) as cache:
    prefix = [str(binary), "-X", "pycache_prefix=" + cache, "-c",
              "import sys,runpy,_testinternalcapi as c; "
              "assert (sys.flags.optimize,__debug__,c.gc_verify_mode(),c.gc_get_stress()) == (0,True,(0,0),0); "
              "print('actual mode: optimize=0 __debug__=True verifier=(0,0) stress=0',flush=True); "
              "sys.argv=sys.argv[1:]; runpy.run_path(sys.argv[0],run_name='__main__')"]
    run("combined-domain.log", prefix + ["boil/design/work/controls/H3-1-stage2b1/check_domain.py"])
    run("combined-root-source-scan.log", prefix + ["Tools/cases_generator/root_check_c.py", "--summary", *marked])
    run("combined-traverse-audit.log", prefix + ["Tools/boil/traverse_audit.py", "--check"])
after = snapshot()
assert before == after
(out / "codegen-closure.json").write_text(json.dumps({"before": before, "after": after,
    "commands": commands, "marked_sources": marked,
    "driver_sha256": digest(Path(__file__).read_bytes())}, indent=2) + "\n")
print("PASS: regeneration, IDs, checker/traversal; source/status/binary unchanged", flush=True)
