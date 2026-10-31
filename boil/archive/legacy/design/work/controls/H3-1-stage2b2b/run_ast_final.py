"""Run the unchanged valid-AST oracle against the fixed line representation."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main(build):
    repo = next(p for p in Path(__file__).resolve().parents if (p / "Python/bytecodes.c").exists())
    binary = repo / "build" / build / "python"
    probe = repo / "boil/design/work/controls/H3-1-stage2b2b/probe_ast.py"
    assert sha(probe) == "7feccf21ac66fb3f2da3a205a5d8e194d7446b0d70c72fa625f06dae8af0b612"
    out = Path(__file__).parent / ("final-ast-" + build)
    out.mkdir(exist_ok=True)
    sources = {name: sha(repo / name) for name in (
        "Python/instrumentation.c", "Objects/codeobject.c", "Python/legacy_tracing.c",
        "Include/internal/pycore_instruments.h", "Modules/_testinternalcapi.c")}
    manifest = {"binary_sha256": sha(binary), "config_sha256": sha(binary.parent / "pyconfig.h"),
                "probe_sha256": sha(probe), "script_sha256": sha(Path(__file__)),
                "sources": sources, "cases": {}}
    env = {k: v for k, v in os.environ.items() if not k.startswith("PYTHON_GC") and k != "PYTHONOPTIMIZE"}
    for case in ("backward_9", "backward_2", "module_zero", "extreme_positive"):
        with tempfile.TemporaryDirectory(prefix="pycache-", dir=out) as cache:
            command = [str(binary), "-X", "pycache_prefix=" + cache, str(probe), "--child", case]
            result = subprocess.run(command, text=True, capture_output=True, env=env, timeout=120)
        log = out / (case + ".log")
        log.write_text(result.stdout + result.stderr)
        manifest["cases"][case] = {"returncode": result.returncode, "command": command,
                                   "log_sha256": sha(log)}
        print(build, case, result.returncode, flush=True)
    assert sha(binary) == manifest["binary_sha256"]
    assert sources == {name: sha(repo / name) for name in sources}
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    assert all(item["returncode"] == 0 for item in manifest["cases"].values())


if __name__ == "__main__":
    assert sys.argv[2:] == ["--", "-m", "test"]
    main(sys.argv[1])
