"""Resolve port references into a dependency-first order.

`requires` edges name exact ports rather than version ranges, so resolution
is a depth-first walk with cycle detection and no solver. Each port appears
once, after every port it requires.
"""

from __future__ import annotations

from collections.abc import Sequence
from dataclasses import dataclass
from pathlib import Path

from ports._support.common import PORTS, PortError
from ports._support.recipe import Recipe, load_recipe


@dataclass(frozen=True)
class Node:
    recipe: Recipe
    requires: tuple[Node, ...]  # direct requirements, already resolved

    @property
    def ref(self) -> str:
        return self.recipe.ref


def resolve(refs: Sequence[str], root: Path = PORTS) -> list[Node]:
    """Return refs and their transitive requirements, dependencies first."""
    order: dict[str, Node] = {}

    def visit(ref: str, chain: tuple[str, ...]) -> Node:
        if ref in chain:
            raise PortError("dependency cycle: " + " -> ".join((*chain, ref)))
        if ref in order:
            return order[ref]
        recipe = load_recipe(ref, root)
        requires = tuple(visit(dep, (*chain, ref)) for dep in recipe.requires)
        order[ref] = Node(recipe, requires)
        return order[ref]

    for ref in refs:
        visit(ref, ())
    return list(order.values())


def closure(node: Node) -> list[Node]:
    """Return node's transitive requirements and then node itself."""
    order: dict[str, Node] = {}

    def visit(current: Node) -> None:
        if current.ref not in order:
            for dep in current.requires:
                visit(dep)
            order[current.ref] = current

    visit(node)
    return list(order.values())
