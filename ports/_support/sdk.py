"""The SDK: the interpreter ports are built against, and its ABI key.

ports/sdk.json is the only build configuration. Results, wheels and venvs are
stored under the abi_id, so a wheel built for one abi_id is never installed
under another. Bump abi_id whenever the C ABI changes. Results also record the
interpreter's SOABI, so pointing --interpreter at an incompatible build (such
as a debug build) without a new abi_id fails instead of reusing wheels.
"""

from __future__ import annotations

import functools
import json
import os
import re
from dataclasses import dataclass
from pathlib import Path

from ports._support.common import PORTS, REPO, PortError, capture

_FIELDS = {"interpreter", "abi_id", "compiler", "cflags"}
_PROBE = (
    "import sysconfig as s;"
    " print(s.get_config_var('SOABI'), s.get_config_var('srcdir'),"
    " s.get_path('include'), s.get_path('platinclude'), sep='\\n')"
)


@dataclass(frozen=True)
class SDK:
    interpreter: Path
    abi_id: str
    compiler: str
    cflags: str


@dataclass(frozen=True)
class InterpreterFacts:
    soabi: str
    srcdir: Path
    # Python.h and pyconfig.h; an uninstalled interpreter's venv reports
    # an installed prefix that does not exist, so ask the interpreter itself.
    include: tuple[Path, Path]


def load_sdk(interpreter: str | None, abi_id: str | None) -> SDK:
    """Read sdk.json, applying command-line overrides."""
    document = json.loads((PORTS / "sdk.json").read_text())
    if set(document) != _FIELDS or not all(
        isinstance(value, str) for value in document.values()
    ):
        raise PortError(f"sdk.json must hold the string fields {_FIELDS}")
    abi = abi_id or document["abi_id"]
    # The abi_id names directories, so keep it to one safe path component.
    if not re.fullmatch(r"[a-z0-9][a-z0-9.-]*", abi):
        raise PortError(f"abi_id {abi!r} must match [a-z0-9][a-z0-9.-]*")
    path = (
        Path(interpreter).absolute() if interpreter
        else REPO / document["interpreter"]
    )
    return SDK(path, abi, document["compiler"], document["cflags"])


@functools.cache
def interpreter_facts(sdk: SDK) -> InterpreterFacts:
    if not sdk.interpreter.is_file():
        raise PortError(
            f"interpreter {sdk.interpreter} does not exist; build one with"
            " `python -m ports build cpython:default` and pass"
            " --interpreter build/ports/cpython-default/python"
        )
    output = capture([sdk.interpreter, "-I", "-c", _PROBE])
    soabi, srcdir, include, platinclude = output.splitlines()
    return InterpreterFacts(
        soabi, Path(srcdir), (Path(include), Path(platinclude)),
    )


def tree_revision(tree: Path) -> str:
    """Return the short git sha of tree, suffixed -dirty for local edits.

    An installed interpreter has no source checkout; report it as unknown
    rather than refusing to benchmark it.
    """
    try:
        sha = capture(["git", "-C", tree, "rev-parse", "--short", "HEAD"])
    except PortError:
        return "unknown"
    status = capture(
        ["git", "-C", tree, "status", "--porcelain", "--untracked-files=no"]
    )
    return sha + "-dirty" if status else sha


def subprocess_env(**extra: str) -> dict[str, str]:
    """Environment for build and test commands.

    Host PYTHON* variables are dropped so they cannot leak into the target
    interpreter; pip runs quietly and never prompts.
    """
    env = {
        key: value for key, value in os.environ.items()
        if not key.startswith("PYTHON")
    }
    env.update(PIP_DISABLE_PIP_VERSION_CHECK="1", PIP_NO_INPUT="1", **extra)
    return env
