"""The interpreter as a port: build this checkout out of tree.

ports/cpython/recipe.json maps each variant to configure flags, and variant
<v> builds in build/ports/cpython-<v>. configure reruns only when its flags
or the SDK compiler change. make always runs: its dependency tracking is the
content address for a source tree that changes with every commit. The SDK's
CFLAGS are for extension modules and are not applied here.
"""

from __future__ import annotations

import json
import os
from dataclasses import dataclass

from ports._support.common import (
    PORTS,
    REPO,
    STATE,
    PortError,
    Summary,
    capture,
    fresh_log,
    log,
    relative,
    run,
    write_json,
)
from ports._support.sdk import SDK, subprocess_env, tree_revision


@dataclass(frozen=True)
class InterpreterRecipe:
    variant: str
    configure: tuple[str, ...]


def load_interpreter(ref: str) -> InterpreterRecipe:
    """Select a variant from ports/cpython/recipe.json; "cpython" alone
    selects the default variant."""
    _, _, variant = ref.partition(":")
    document = json.loads((PORTS / "cpython" / "recipe.json").read_text())
    if set(document) != {"name", "default_variant", "variants"}:
        raise PortError("cpython recipe takes name, default_variant, variants")
    variant = variant or document["default_variant"]
    variants = document["variants"]
    if variant not in variants:
        raise PortError(f"{ref}: unknown variant; have {sorted(variants)}")
    flags = variants[variant].get("configure")
    if set(variants[variant]) != {"configure"} or not isinstance(flags, list):
        raise PortError(f"{ref}: a variant is {{\"configure\": [flags]}}")
    return InterpreterRecipe(variant, tuple(flags))


def build_interpreter(ref: str, sdk: SDK) -> Summary:
    recipe = load_interpreter(ref)
    name = f"cpython-{recipe.variant}"
    directory = STATE / name
    directory.mkdir(parents=True, exist_ok=True)
    output = fresh_log(STATE / "logs" / f"{name}.log")
    env = subprocess_env(CC=sdk.compiler)
    configure = [str(REPO / "configure"), *recipe.configure]
    stamp = directory / "ports-configure.json"
    wanted = json.dumps({"configure": configure, "CC": sdk.compiler})
    if not stamp.exists() or stamp.read_text() != wanted:
        log.info("build %s: configure %s", name, " ".join(recipe.configure))
        run(configure, output, cwd=directory, env=env)
        stamp.write_text(wanted)
    log.info("build %s: make", name)
    run(["make", f"-j{os.cpu_count()}"], output, cwd=directory, env=env)
    python = directory / "python"
    version = capture([python, "-I", "-c", "import sys; print(sys.version)"])
    record: Summary = {
        "ref": f"cpython:{recipe.variant}",
        "configure": list(recipe.configure),
        "interpreter": relative(python),
        "version": version,
        "revision": tree_revision(REPO),
        "log": relative(output),
    }
    write_json(STATE / "results" / f"{name}.json", record)
    return {"status": "built", **record}
