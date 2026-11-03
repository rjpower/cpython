"""Bind short real-interpreter probes to source, compiler and build identities."""
import hashlib
import json
import os
from pathlib import Path
import resource
import shlex
import shutil
import subprocess
import sys
import tempfile


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main(build_name, label, probe_name):
    repo = next(p for p in Path(__file__).resolve().parents if (p / "Python/bytecodes.c").exists())
    build = repo / "build" / build_name
    binary = build / "python"
    directory = Path(__file__).parent.resolve()
    probe = directory / probe_name
    config = next(line.split("=", 1)[1].strip() for line in (build / "Makefile").read_text().splitlines()
                  if line.startswith("CC=") or line.startswith("CC ="))
    compiler = Path(shutil.which(shlex.split(config)[0])).resolve()
    sources = [repo / name for name in subprocess.check_output(
        ["git", "diff", "--name-only"], cwd=repo, text=True).splitlines()]
    paths = sources + [binary, build / "pyconfig.h", compiler, probe, Path(__file__).resolve(),
                        directory / "allocation_probe.c"]
    identities = {str(path): sha(path) for path in paths}
    env = {key: value for key, value in os.environ.items()
           if not key.startswith("PYTHON_GC") and key != "PYTHONOPTIMIZE"}
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    with tempfile.TemporaryDirectory(prefix="pycache-", dir=directory) as cache:
        command = [str(binary), "-X", "pycache_prefix=" + cache, str(probe)]
        result = subprocess.run(command, text=True, capture_output=True, env=env, timeout=120)
    log = directory / (label + "-" + build_name + ".log")
    log.write_text(result.stdout + result.stderr)
    after = {name: sha(Path(name)) for name in identities}
    manifest = {"head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=repo, text=True).strip(),
                "before": identities, "after": after, "compiler": str(compiler),
                "compiler_CC": config, "command": command, "returncode": result.returncode,
                "log_sha256": sha(log), "core_limit": resource.getrlimit(resource.RLIMIT_CORE)}
    (directory / (label + "-" + build_name + ".json")).write_text(json.dumps(manifest, indent=2) + "\n")
    assert identities == after
    print(label, build_name, result.returncode, flush=True)
    assert result.returncode == 0, result.stdout + result.stderr


if __name__ == "__main__":
    assert sys.argv[4:] == ["--", "-m", "test"], sys.argv
    main(*sys.argv[1:4])
