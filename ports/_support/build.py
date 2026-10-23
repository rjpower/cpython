"""Build one package port into a wheel and record the result.

A result is content-addressed by its recipe hash: the recipe bytes (which pin
the source and patches), an optional build.py, the SDK fields and this
driver's own code. The interpreter binary is deliberately not an input; the
abi_id stands for it, so interpreter changes reuse wheels until the abi_id is
bumped. `build` skips a port whose record is current; `check` installs only
current records.

Sdists build in a fresh tree and a fresh venv holding only the pinned
build_requires, which the host fetches. The target interpreter's pip runs
without isolation or an index, so nothing else is visible to the build.
"""

from __future__ import annotations

import hashlib
import importlib.util
import json
import shutil
from collections.abc import Callable
from dataclasses import asdict, dataclass
from pathlib import Path

from ports._support.common import (
    PORTS,
    REPO,
    STATE,
    PortError,
    Summary,
    fresh_dir,
    fresh_log,
    log,
    relative,
    run,
    sha256_file,
    write_json,
)
from ports._support.recipe import Recipe
from ports._support.sdk import SDK, interpreter_facts, subprocess_env
from ports._support.sources import (
    apply_patches,
    fetch,
    fetch_requirement,
    unpack,
)

# Driver code that shapes wheel bytes; editing it invalidates every result.
_DRIVER = ("_support/build.py", "_support/sdk.py", "_support/sources.py")


@dataclass(frozen=True)
class BuildContext:
    """What a port's build(ctx) receives.

    The builder must leave exactly one wheel in wheel_dir. Run commands with
    ports._support.common.run(argv, ctx.log, env=ctx.env).
    """

    recipe: Recipe
    source: Path  # fresh upstream tree with patches applied
    python: Path  # build venv interpreter with build_requires installed
    wheel_dir: Path  # empty output directory
    env: dict[str, str]  # subprocess environment with the SDK's CC, CFLAGS
    log: Path


@dataclass(frozen=True)
class Result:
    """The record `build` writes and `check` trusts."""

    ref: str
    version: str
    abi_id: str
    recipe_hash: str
    soabi: str
    wheel: str  # repository-relative path
    wheel_sha256: str
    source_sha256: str
    log: str


def fingerprint(recipe: Recipe, sdk: SDK) -> str:
    custom = recipe.directory / "build.py"
    inputs = {
        "recipe": recipe.digest,
        "build.py": sha256_file(custom) if custom.exists() else None,
        "sdk": [sdk.abi_id, sdk.compiler, sdk.cflags],
        "driver": {name: sha256_file(PORTS / name) for name in _DRIVER},
    }
    encoded = json.dumps(inputs, sort_keys=True).encode()
    return hashlib.sha256(encoded).hexdigest()


def log_path(sdk: SDK, recipe: Recipe, verb: str) -> Path:
    """Start an empty log for one verb on one port."""
    name = f"{recipe.name}-{recipe.version}.{verb}.log"
    return fresh_log(STATE / "logs" / sdk.abi_id / name)


def verified_result(recipe: Recipe, sdk: SDK) -> Result:
    """Return recipe's current result, or raise saying why there is none."""
    path = _result_path(sdk, recipe)
    if not path.exists():
        raise PortError(f"{recipe.ref} is not built for {sdk.abi_id}")
    result = Result(**json.loads(path.read_text()))
    if result.recipe_hash != fingerprint(recipe, sdk):
        raise PortError(f"{recipe.ref} is stale: its recipe, build.py, the"
                        " SDK or the driver changed")
    soabi = interpreter_facts(sdk).soabi
    if result.soabi != soabi:
        raise PortError(
            f"{recipe.ref} was built for SOABI {result.soabi}, but"
            f" {sdk.interpreter} is {soabi}; give it its own --abi-id"
        )
    wheel = REPO / result.wheel
    if not wheel.exists() or sha256_file(wheel) != result.wheel_sha256:
        raise PortError(f"{result.wheel} no longer matches its record")
    return result


def build_port(recipe: Recipe, sdk: SDK, force: bool) -> Summary:
    if not force:
        try:
            result = verified_result(recipe, sdk)
        except PortError as reason:
            log.info("%s; building", reason)
        else:
            return {"status": "cached", **asdict(result)}
    output = log_path(sdk, recipe, "build")
    archive = fetch(recipe.source)
    if recipe.kind == "pure-wheel":
        wheel = archive
    else:
        wheel = _build_sdist(recipe, archive, sdk, output)
    return {"status": "built", **asdict(_publish(recipe, sdk, wheel, output))}


def create_venv(sdk: SDK, path: Path, output: Path) -> Path:
    """Create a fresh venv from the target interpreter; return its python.

    ensurepip installs the interpreter's bundled pip, without the network.
    """
    run([sdk.interpreter, "-m", "venv", fresh_dir(path)], output,
        env=subprocess_env())
    return path / "bin" / "python"


def _build_sdist(
    recipe: Recipe, archive: Path, sdk: SDK, output: Path
) -> Path:
    work = fresh_dir(STATE / "work" / sdk.abi_id / recipe.name)
    tree = unpack(archive, work / "src")
    apply_patches(recipe, tree, output)
    python = create_venv(sdk, work / "venv", output)
    env = subprocess_env(CC=sdk.compiler, CFLAGS=sdk.cflags)
    if recipe.build_requires:
        wheels = [fetch_requirement(pin) for pin in recipe.build_requires]
        run([python, "-m", "pip", "install", "--no-index", "--no-deps",
             *wheels], output, env=env)
    ctx = BuildContext(recipe, tree, python, fresh_dir(work / "dist"), env,
                       output)
    log.info("build %s: compile with %s", recipe.ref, sdk.interpreter)
    _builder(recipe)(ctx)
    wheels = list(ctx.wheel_dir.glob("*.whl"))
    if len(wheels) != 1:
        raise PortError(f"{recipe.ref}: the builder left {len(wheels)} wheels"
                        f" in {relative(ctx.wheel_dir)}, not one")
    return wheels[0]


def _builder(recipe: Recipe) -> Callable[[BuildContext], None]:
    """Return build(ctx) from the port's build.py, else pip_wheel."""
    path = recipe.directory / "build.py"
    if not path.exists():
        return pip_wheel
    spec = importlib.util.spec_from_file_location(
        f"ports_builder_{recipe.name}", path
    )
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.build


def pip_wheel(ctx: BuildContext) -> None:
    """The default builder: an sdist's own PEP 517 backend, via pip."""
    run([
        ctx.python, "-m", "pip", "wheel", "-v", "--no-deps",
        "--no-build-isolation", "--no-index", "--wheel-dir", ctx.wheel_dir,
        ctx.source,
    ], ctx.log, cwd=ctx.source.parent, env=ctx.env)


def _publish(recipe: Recipe, sdk: SDK, wheel: Path, output: Path) -> Result:
    target = STATE / "wheels" / sdk.abi_id / wheel.name
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(wheel, target)
    result = Result(
        ref=recipe.ref,
        version=recipe.version,
        abi_id=sdk.abi_id,
        recipe_hash=fingerprint(recipe, sdk),
        soabi=interpreter_facts(sdk).soabi,
        wheel=relative(target),
        wheel_sha256=sha256_file(target),
        source_sha256=recipe.source.sha256,
        log=relative(output),
    )
    write_json(_result_path(sdk, recipe), asdict(result))
    return result


def _result_path(sdk: SDK, recipe: Recipe) -> Path:
    name = f"{recipe.name}-{recipe.version}.json"
    return STATE / "results" / sdk.abi_id / name
