"""The register handler ids: dense, their own, independent of stack opcodes.

The word kinds that are no instruction come first (WORD_KINDS), then the
register instruction arms and the observed wrappers of the arms.  The ids
must fit the handler field.
"""

import os
from collections.abc import Sequence

import ir_arms

# A word that holds operands, not an instruction: its handler id is inert, so
# a zeroed word and a jump into operand words abort.
WORD_KINDS = ("IR_OPERANDS",)


def id_base() -> int:
    """The id of the first handler: 0, or PYJIT_IR_ID_BASE for a guard build.
    A guard build leaves the ids below the base unused, so an opcode or CFG
    number that is mistaken for a handler id lands on an empty row, and the
    ordinary suites assert on it (see ps/design.md, step 3).  The checked-in
    generated files use 0."""
    return int(os.environ.get("PYJIT_IR_ID_BASE", "0"))


def handler_ids() -> dict[str, int]:
    "Handler id of each word kind, by name"
    base = id_base()
    return {name: base + number for number, name in enumerate(WORD_KINDS)}


def first_arm() -> int:
    "The arms start here: id_base() plus the word kinds"
    return id_base() + len(WORD_KINDS)


def num_handlers(arms: Sequence[ir_arms.ArmEntry]) -> int:
    """Every handler id: the word kinds, the register instruction arms and
    the observed wrappers of the arms, which follow them."""
    total = first_arm() + len(arms)
    if total > ir_arms.IR_LIMIT:
        raise ir_arms.HandlerLimitError(
            f"{first_arm()} word kinds and {len(arms)} arms and "
            f"wrappers need {total} handler ids, past the {ir_arms.IR_LIMIT} "
            "that a handler field holds")
    return total
