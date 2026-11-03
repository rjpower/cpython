"""Command line: python -m ports {build,check,bench} [ref ...].

With no refs, a verb covers every package port under ports/python. Progress
and one line per port go to stderr; stdout carries a single JSON summary.
The exit status is 1 if any port failed and 2 if the run could not start.
"""

from __future__ import annotations

import argparse
import json
import logging
import sys
from collections.abc import Callable
from functools import partial

from ports._support.build import build_port
from ports._support.check import bench_node, check_node
from ports._support.common import PortError, Summary, log
from ports._support.graph import Node, resolve
from ports._support.interpreter import build_interpreter
from ports._support.recipe import all_refs
from ports._support.sdk import SDK, load_sdk
from ports._support.sources import patch_lines


def _run_port(ref: str, action: Callable[[], Summary], **extra: object
              ) -> Summary:
    """Run one port's action and log its status line; a PortError fails
    that port, not the run. `extra` joins both the summary and the line."""
    try:
        summary = {**action(), **extra}
    except PortError as error:
        log.error("%s: %s", ref, error)
        summary = {"ref": ref, "status": "FAIL", "error": str(error), **extra}
    log.info("%s %s %s", summary["status"], ref,
             " ".join(f"{key}={value}" for key, value in extra.items()))
    return summary


def _is_interpreter(ref: str) -> bool:
    return ref.partition(":")[0] == "cpython"


def _build(args: argparse.Namespace, sdk: SDK) -> list[Summary]:
    interpreters = [ref for ref in args.refs if _is_interpreter(ref)]
    packages = [ref for ref in args.refs if not _is_interpreter(ref)]
    force = args.force
    nodes = resolve(packages if args.refs else all_refs())
    return [
        *(_run_port(ref, partial(build_interpreter, ref, sdk))
          for ref in interpreters),
        *(_run_port(node.ref, partial(build_port, node.recipe, sdk, force),
                    version=node.recipe.version) for node in nodes),
    ]


def _selected(refs: list[str]) -> list[Node]:
    """Resolve the requested package ports; their requirements are
    installed alongside them but not tested."""
    if any(_is_interpreter(ref) for ref in refs):
        raise PortError("check and bench take package ports, not cpython")
    requested = refs or all_refs()
    nodes = {node.ref: node for node in resolve(requested)}
    return [nodes[ref] for ref in requested]


def _check(args: argparse.Namespace, sdk: SDK) -> list[Summary]:
    return [
        _run_port(node.ref, partial(check_node, node, sdk),
                  version=node.recipe.version,
                  patch_lines=patch_lines(node.recipe))
        for node in _selected(args.refs)
    ]


def _bench(args: argparse.Namespace, sdk: SDK) -> list[Summary]:
    return [
        _run_port(node.ref, partial(bench_node, node, sdk),
                  version=node.recipe.version)
        for node in _selected(args.refs) if node.recipe.bench
    ]


_VERBS = {"build": _build, "check": _check, "bench": _bench}


def _parser() -> argparse.ArgumentParser:
    common = argparse.ArgumentParser(add_help=False)
    common.add_argument("refs", nargs="*",
                        help="ports such as python/six or cpython:ft")
    common.add_argument("--interpreter",
                        help="target interpreter (default: from sdk.json)")
    common.add_argument("--abi-id", help="ABI key (default: from sdk.json)")
    parser = argparse.ArgumentParser(prog="python -m ports",
                                     description=__doc__.splitlines()[0])
    verbs = parser.add_subparsers(dest="verb", required=True)
    build = verbs.add_parser("build", parents=[common],
                             help="build ports and their requirements")
    build.add_argument("--force", action="store_true",
                       help="rebuild even if the result is current")
    verbs.add_parser("check", parents=[common],
                     help="install into a fresh venv and run tests")
    verbs.add_parser("bench", parents=[common],
                     help="time bench scripts into build/ports/bench.tsv")
    return parser


def main() -> int:
    args = _parser().parse_args()
    logging.basicConfig(level=logging.INFO, format="ports: %(message)s",
                        stream=sys.stderr)
    try:
        sdk = load_sdk(args.interpreter, args.abi_id)
        ports = _VERBS[args.verb](args, sdk)
    except PortError as error:
        log.error("%s", error)
        json.dump({"verb": args.verb, "error": str(error)}, sys.stdout)
        print()
        return 2
    json.dump({"verb": args.verb, "abi_id": sdk.abi_id,
               "interpreter": str(sdk.interpreter), "ports": ports},
              sys.stdout, indent=2)
    print()
    return 1 if any(port["status"] == "FAIL" for port in ports) else 0


if __name__ == "__main__":
    sys.exit(main())
