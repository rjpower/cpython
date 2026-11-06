"""Check the integrated stats producer and reader using real child processes."""

import argparse
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--python", required=True, type=Path)
    parser.add_argument("--reader", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    sys.path.insert(0, str(args.reader.resolve().parent))
    spec = importlib.util.spec_from_file_location("identity_reader", args.reader)
    reader = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = reader
    spec.loader.exec_module(reader)
    cases = {
        "normal": ("pass", 1, 0),
        "zero": ("import sys; sys._stats_off(); sys._stats_clear()", 0, 1),
        "repeated": ("import sys; sys._stats_dump(); sum(range(10))", 2, 0),
        "fork": (
            "import os, sys\npid = os.fork()\n"
            "if pid == 0:\n    sys.exit(0)\n"
            "os.waitpid(pid, 0)\n", 2, 0),
        "off_fork": (
            "import os, sys\nsys._stats_off()\npid = os.fork()\n"
            "if pid == 0:\n    os._exit(0)\n"
            "os.waitpid(pid, 0)\nsys._stats_on()\n", 1, 0),
        "late_on": (
            "import os, sys\npid = os.fork()\n"
            "if pid == 0:\n    os._exit(0)\n"
            "os.waitpid(pid, 0)\nsys._stats_on()\n", 1, 0),
        "spawn": (
            "import os, sys\n"
            "argv = [sys.executable, '-I', '-S', '-B', '-X', 'pystats', "
            "'-X', 'pystats_dir=' + @STATS_DIR@, '-c', 'pass']\n"
            "pid = os.posix_spawn(sys.executable, argv, os.environ)\n"
            "os.waitpid(pid, 0)\n", 2, 0),
        "popen": (
            "import subprocess, sys\n"
            "subprocess.run([sys.executable, '-I', '-S', '-B', '-X', 'pystats', "
            "'-X', 'pystats_dir=' + @STATS_DIR@, '-c', 'pass'], check=True)\n",
            2, 0),
        "popen_fork_exec": (
            "import subprocess, sys\n"
            "subprocess.run([sys.executable, '-I', '-S', '-B', '-X', 'pystats', "
            "'-X', 'pystats_dir=' + @STATS_DIR@, '-c', 'pass'], "
            "check=True, start_new_session=True)\n", 2, 0),
        "failed_spawn": (
            "import os\ntry:\n"
            " os.posix_spawn('/no-such-identity-probe', ['missing'], os.environ)\n"
            "except FileNotFoundError:\n pass\n", 1, 0),
        "external": (
            "import os\npid = os.posix_spawn('/bin/true', ['true'], os.environ)\n"
            "os.waitpid(pid, 0)\n", 1, 0),
        "fork_callback_dump": (
            "import os, sys\nos.register_at_fork(after_in_parent=sys._stats_dump)\n"
            "pid = os.fork()\nif pid == 0:\n sys.exit(0)\n"
            "os.waitpid(pid, 0)\n", 3, 0),
        "argv_bytes": ("pass", 1, 0),
    }
    env = {k: v for k, v in os.environ.items() if not k.startswith("PYTHON")}
    env["PYTHON_PYSTATS_TAG"] = "identity-probe"
    receipts = {}
    for name, (code, expected_dumps, expected_zero) in cases.items():
        directory = args.output / name
        directory.mkdir()
        code = code.replace("@STATS_DIR@", repr(str(directory.resolve())))
        command = [str(args.python.resolve()), "-I", "-S", "-B", "-X", "pystats",
                   "-X", f"pystats_dir={directory.resolve()}", "-c", code]
        if name == "late_on":
            command.remove("pystats")
            command.remove("-X")
        if name == "argv_bytes":
            command.extend(["é", os.fsdecode(b"\xff"), "a\nb", "a b", ""])
        with subprocess.Popen(command, env=env, stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE) as proc:
            stdout, stderr = proc.communicate()
        (directory / "stdout.log").write_bytes(stdout)
        (directory / "stderr.log").write_bytes(stderr)
        if proc.returncode:
            raise RuntimeError(f"{name}: child returned {proc.returncode}")
        image = args.python.stat()
        (directory / "root-launch.json").write_text(json.dumps([{
            "version": 1, "pid": proc.pid, "ppid": os.getpid(),
            "argv": command, "image_dev": image.st_dev,
            "image_ino": image.st_ino,
        }]))
        folded = reader.Folded()
        folded.fold_dir(directory)
        folded.read_markers(directory)
        zeros = list(directory.glob("*.zero"))
        if folded.dumps != expected_dumps or len(zeros) != expected_zero:
            raise AssertionError(f"{name}: unexpected dump/zero counts")
        if expected_zero:
            if folded.identity_complete or not folded.accounting_errors:
                raise AssertionError("zero-stats outcome incorrectly admitted")
        elif name in ("off_fork", "external"):
            if (folded.identity_complete or len(folded.child_receipts) != 1
                or not any("lacks matching child marker" in error
                           for error in folded.accounting_errors)):
                raise AssertionError(f"{name}: missing child was silently admitted")
        elif not folded.identity_complete or folded.accounting_errors:
            raise AssertionError(f"{name}: {folded.accounting_errors}")
        identities = list(folded.identity_records.values())
        parents = [identity for identity in identities if identity["pid"] == proc.pid]
        if not parents or any(identity["ppid"] != os.getpid() for identity in parents):
            raise AssertionError(f"{name}: incorrect launched process identity")
        child_sources = {"fork": "fork", "fork_callback_dump": "fork",
                         "spawn": "spawn", "popen": "spawn",
                         "popen_fork_exec": "fork_exec"}
        if name in child_sources:
            children = [identity for identity in identities if identity["pid"] != proc.pid]
            if len(children) != 1 or children[0]["ppid"] != proc.pid:
                raise AssertionError("fork child identity does not name its parent")
            if (len(folded.child_receipts) != 1 or
                folded.child_receipts[0]["source"] != child_sources[name]):
                raise AssertionError(f"{name}: incorrect native child receipt")
        elif any(identity["pid"] != proc.pid for identity in identities):
            raise AssertionError(f"{name}: unexpected process identity")
        elif name not in ("off_fork", "external") and folded.child_receipts:
            raise AssertionError(f"{name}: unexpected native child receipt")
        if name == "repeated" and len(folded.dump_identities) != 2:
            raise AssertionError("repeated dump overwrote its interval stem")
        if name == "argv_bytes":
            identity = next(iter(folded.identity_records.values()))
            if identity["argv"] != command:
                raise AssertionError("C marker changed argv bytes or boundaries")
        receipts[name] = {
            "command": command, "returncode": proc.returncode,
            "dumps": folded.dumps, "zero_markers": len(zeros),
            "identity_complete": folded.identity_complete,
            "identities": folded.identity_records,
            "errors": folded.accounting_errors,
        }
        (args.output / "receipt.json").write_text(json.dumps(receipts, indent=2))
        print(f"{name}: PASS", flush=True)


if __name__ == "__main__":
    main()
