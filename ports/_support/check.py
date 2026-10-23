"""Install built wheels into fresh venvs and run each port's scripts.

check and bench never build. They install only wheels whose result record is
current for this abi_id (see build.verified_result), with pip --no-index
--no-deps, so a venv holds exactly the port and its declared requirements.
Scripts run with -I from the venv directory, so neither the recipe directory
nor the caller's environment is importable.
"""

from __future__ import annotations

import statistics
import time
from pathlib import Path

from ports._support.build import create_venv, log_path, verified_result
from ports._support.common import REPO, STATE, Summary, log, relative, run
from ports._support.graph import Node, closure
from ports._support.recipe import Recipe
from ports._support.sdk import (
    SDK,
    interpreter_facts,
    subprocess_env,
    tree_revision,
)

BENCH_RUNS = 5


def check_node(node: Node, sdk: SDK) -> Summary:
    output = log_path(sdk, node.recipe, "check")
    python = _prepare(node, sdk, output)
    for script in node.recipe.tests:
        _run_script(python, node.recipe, script, output)
    return {"ref": node.ref, "status": "PASS", "log": relative(output)}


def bench_node(node: Node, sdk: SDK) -> Summary:
    """Run each bench script BENCH_RUNS times and append the median wall
    time, which includes interpreter startup, to build/ports/bench.tsv."""
    output = log_path(sdk, node.recipe, "bench")
    python = _prepare(node, sdk, output)
    revision = tree_revision(interpreter_facts(sdk).srcdir)
    medians = {}
    for script in node.recipe.bench:
        times = []
        for _ in range(BENCH_RUNS):
            start = time.perf_counter()
            _run_script(python, node.recipe, script, output)
            times.append(time.perf_counter() - start)
        median = medians[script] = statistics.median(times)
        log.info("bench %s %s: median %.3fs", node.ref, script, median)
        _append_bench(
            [sdk.abi_id, node.ref, script, f"{median:.4f}", revision]
        )
    return {"ref": node.ref, "status": "PASS", "median_seconds": medians,
            "revision": revision, "log": relative(output)}


def _prepare(node: Node, sdk: SDK, output: Path) -> Path:
    """Create the port's fresh venv holding it and its requirements."""
    wheels = [
        REPO / verified_result(dep.recipe, sdk).wheel for dep in closure(node)
    ]
    venv = STATE / "venvs" / sdk.abi_id / node.recipe.name
    python = create_venv(sdk, venv, output)
    run([python, "-m", "pip", "install", "--no-index", "--no-deps", *wheels],
        output, env=subprocess_env())
    return python


def _run_script(
    python: Path, recipe: Recipe, script: str, output: Path
) -> None:
    run([python, "-I", recipe.directory / script], output,
        cwd=python.parents[1], env=subprocess_env())


def _append_bench(row: list[str]) -> None:
    path = STATE / "bench.tsv"
    header = "" if path.exists() else (
        "abi_id\tport\tscript\tmedian_seconds\trevision\n"
    )
    with path.open("a") as stream:
        stream.write(header + "\t".join(row) + "\n")
