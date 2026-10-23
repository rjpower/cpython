"""Paths, the user-facing error type and subprocess helpers shared by verbs.

Every artifact the driver writes lives under STATE (build/ports, which is
gitignored). Subprocess output goes to a per-port log file, never to stdout,
so stdout stays reserved for the JSON summary.
"""

from __future__ import annotations

import hashlib
import json
import logging
import shlex
import shutil
import subprocess
from collections.abc import Mapping, Sequence
from pathlib import Path

PORTS = Path(__file__).resolve().parents[1]
REPO = PORTS.parent
STATE = REPO / "build" / "ports"

log = logging.getLogger("ports")

# One port's entry in the JSON summary a verb prints to stdout.
Summary = dict[str, object]


class PortError(Exception):
    """An expected failure of one port: a pin mismatch or a failed step."""


def sha256_file(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def relative(path: Path) -> str:
    """Render a path under the repository relative to it, for records."""
    return path.resolve().relative_to(REPO).as_posix()


def write_json(path: Path, data: object) -> None:
    """Write a record atomically, so readers never see a partial one."""
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(json.dumps(data, indent=2) + "\n")
    temporary.replace(path)


def fresh_log(path: Path) -> Path:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("")
    return path


def fresh_dir(path: Path) -> Path:
    """Replace path with an empty directory, so no state leaks between runs."""
    shutil.rmtree(path, ignore_errors=True)
    path.mkdir(parents=True)
    return path


def run(argv: Sequence[str | Path], output: Path, *, cwd: Path | None = None,
        env: Mapping[str, str] | None = None) -> None:
    """Run argv, appending its output to the log file `output`.

    On failure, log the tail of the log and raise PortError naming it.
    """
    command = [str(arg) for arg in argv]
    with output.open("a") as stream:
        stream.write(f"$ {shlex.join(command)}\n")
        stream.flush()
        status = subprocess.run(
            command, check=False, cwd=cwd, env=env, stdin=subprocess.DEVNULL,
            stdout=stream, stderr=subprocess.STDOUT,
        ).returncode
    if status:
        for line in output.read_text(errors="replace").splitlines()[-20:]:
            log.error("  | %s", line)
        shown = shlex.join([Path(command[0]).name, *command[1:3]])
        raise PortError(f"`{shown}...` exited {status};"
                        f" log: {relative(output)}")


def capture(argv: Sequence[str | Path]) -> str:
    """Run a short query command and return its stripped stdout."""
    command = [str(arg) for arg in argv]
    result = subprocess.run(command, check=False, capture_output=True,
                            text=True, stdin=subprocess.DEVNULL)
    if result.returncode:
        raise PortError(
            f"{shlex.join(command)} exited {result.returncode}:"
            f" {result.stderr.strip()}"
        )
    return result.stdout.strip()
