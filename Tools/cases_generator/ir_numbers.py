"""The pinned handler numbers, read from ir_numbers.txt.

A handler number is a fact of the .pyc format, so it is chosen by hand and
checked in rather than derived from the sorted names.  Adding a handler needs
a line in the table; removing one leaves a `# removed: NAME NUMBER` line so
that the number is not reused within a format revision. An incompatible
revision must change the cache magic and pinned table together. See
ir_numbers.txt for the layout.

Run as a script, `ir_numbers.py --check-numbers Python/bytecodes.c` proves
that the table and the source agree.
"""

import argparse
import functools
import re
from dataclasses import dataclass, field
from pathlib import Path


TABLE_PATH = Path(__file__).parent / "ir_numbers.txt"

# Lines of the constants section; they are not handlers.
CONSTANTS = ("HAVE_ARGUMENT", "MIN_INSTRUMENTED_OPCODE", "IR_FIRST_ONLY_HANDLER")

RANGE_RE = re.compile(r"#\s*range\s+(\w+)\s+(\d+)\s+(\d+)\s*$")
REMOVED_RE = re.compile(r"#\s*removed:\s*(\w+)\s+(\d+)\s*$")
LINE_RE = re.compile(r"(\w+)\s+(\d+)\s*$")


class NumberError(Exception):
    pass


@dataclass
class NumberTable:
    numbers: dict[str, int] = field(default_factory=dict)
    removed: dict[str, int] = field(default_factory=dict)
    constants: dict[str, int] = field(default_factory=dict)
    # Reserved ranges of the IR-only region, as [low, high), by step name.
    ranges: dict[str, tuple[int, int]] = field(default_factory=dict)

    @property
    def have_arg(self) -> int:
        return self.constants["HAVE_ARGUMENT"]

    @property
    def min_instrumented(self) -> int:
        return self.constants["MIN_INSTRUMENTED_OPCODE"]

    @property
    def first_ir_only(self) -> int:
        return self.constants["IR_FIRST_ONLY_HANDLER"]

    @property
    def num_handlers(self) -> int:
        """One past the highest number, counting the reserved ranges"""
        ends = [n + 1 for n in self.numbers.values()]
        ends += [high for _, high in self.ranges.values()]
        return max(ends)

    def number(self, name: str, low: int, high: int, what: str) -> int:
        "The number of a handler, which must lie in [low, high)"
        return lookup(self, name, low, high, what)

    def used(self) -> set[int]:
        return set(self.numbers.values()) | set(self.removed.values())

    def next_free(self, low: int, high: int) -> int | None:
        used = self.used()
        return next((n for n in range(low, high) if n not in used), None)


def parse_table(text: str) -> NumberTable:
    table = NumberTable()
    for lineno, line in enumerate(text.splitlines(), 1):
        line = line.strip()
        if not line:
            continue
        if line.startswith("#"):
            if m := RANGE_RE.match(line):
                table.ranges[m[1]] = (int(m[2]), int(m[3]))
            elif m := REMOVED_RE.match(line):
                table.removed[m[1]] = int(m[2])
            continue
        m = LINE_RE.match(line)
        if m is None:
            raise NumberError(f"ir_numbers.txt:{lineno}: bad line {line!r}")
        name, number = m[1], int(m[2])
        target = table.constants if name in CONSTANTS else table.numbers
        if name in target:
            raise NumberError(f"ir_numbers.txt:{lineno}: {name} appears twice")
        target[name] = number
    for name in CONSTANTS:
        if name not in table.constants:
            raise NumberError(f"ir_numbers.txt: missing {name}")
    by_number: dict[int, str] = {}
    for name, number in table.numbers.items():
        if number in by_number:
            raise NumberError(
                f"ir_numbers.txt: {name} and {by_number[number]} "
                f"both have number {number}")
        by_number[number] = name
    for name, number in table.removed.items():
        if number in by_number:
            raise NumberError(
                f"ir_numbers.txt: number {number} of removed {name} "
                f"is reused by {by_number[number]}")
    return table


@functools.cache
def load_table() -> NumberTable:
    return parse_table(TABLE_PATH.read_text())


# The IR-only region must stay below the 12-bit handler field of design B3.
IR_LIMIT = 4096


def lookup(table: "NumberTable", name: str, low: int, high: int,
           what: str) -> int:
    """The number of a handler, which must lie in [low, high)"""
    if name not in table.numbers:
        free = table.next_free(low, high)
        hint = (f"the next free number in {low}..{high - 1} is {free}"
                if free is not None else f"no number is free in {low}..{high - 1} "
                f"(removing a handler frees its number)")
        if low == table.first_ir_only:
            # Each step takes its own reserved range, so name them.
            for step, (range_low, range_high) in table.ranges.items():
                step_free = table.next_free(range_low, range_high)
                hint += (f"; reserved range {step} {range_low}..{range_high - 1}: "
                         + (f"next free {step_free}" if step_free is not None
                            else "full"))
        raise NumberError(
            f"{what} {name} has no line in ir_numbers.txt; add `{name} N`: "
            f"{hint}")
    number = table.numbers[name]
    if not low <= number < high:
        raise NumberError(
            f"{what} {name} has number {number} in ir_numbers.txt, outside "
            f"{low}..{high - 1}")
    return number


def check_numbers(filenames: list[str]) -> list[str]:
    """Problems that keep the table from matching the source, as messages"""
    # Imported here: the analyzer imports this module.
    from analyzer import analyze_files
    from ir_generator import handler_ids
    problems: list[str] = []
    try:
        analysis = analyze_files(filenames)
        table = analysis.numbers
        ids = handler_ids(analysis)
    except NumberError as e:
        return [str(e)]
    live = set(analysis.opmap) | set(ids)
    for name, number in table.numbers.items():
        if name not in live:
            problems.append(
                f"ir_numbers.txt has `{name} {number}` but bytecodes.c has no "
                f"such handler; rewrite it as `# removed: {name} {number}`")
    return problems


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check-numbers", action="store_true",
                        help="check that ir_numbers.txt matches the source")
    parser.add_argument("input", nargs="*", help="Instruction definition file(s)")
    args = parser.parse_args()
    from generators_common import DEFAULT_INPUT
    problems = check_numbers(args.input or [DEFAULT_INPUT])
    for problem in problems:
        print(problem)
    return 1 if problems else 0


if __name__ == "__main__":
    # The analyzer imports this module by name, so raise its NumberError.
    from ir_numbers import main as _main
    raise SystemExit(_main())
