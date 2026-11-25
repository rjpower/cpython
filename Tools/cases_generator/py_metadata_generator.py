"""Generate opcode metadata for Python.
Reads the instruction definitions from bytecodes.c.
Writes the metadata to _opcode_metadata.py by default.
"""

import argparse

from analyzer import (
    Analysis,
    Uop,
    analyze_files,
)
from generators_common import (
    DEFAULT_INPUT,
    ROOT,
    write_header,
)
from cwriter import CWriter
from collections.abc import Sequence

import ir_arms
from ir_generator import (IR_BASE_ONLY, STACK_ONLY, all_instructions,
                          handler_ids, ir_only_handler_names, num_old_handlers)
from typing import TextIO


DEFAULT_OUTPUT = ROOT / "Lib/_opcode_metadata.py"


def get_specialized(analysis: Analysis) -> set[str]:
    specialized: set[str] = set()
    for family in analysis.families.values():
        for member in family.members:
            specialized.add(member.name)
    return specialized


def generate_specializations(analysis: Analysis, out: CWriter) -> None:
    out.emit("_specializations = frozendict(\n")
    for family in analysis.families.values():
        out.emit(f'{family.name}=(\n')
        seen = set()
        for member in family.members:
            if member.name in seen:
                continue
            seen.add(member.name)
            out.emit(f'    "{member.name}",\n')
        out.emit("),\n")
    out.emit(")\n\n")


def generate_specialized_opmap(analysis: Analysis, out: CWriter) -> None:
    out.emit("_specialized_opmap = frozendict(\n")
    names = []
    seen = set()
    for family in analysis.families.values():
        for member in family.members:
            if member.name == family.name:
                continue
            if member.name in seen:
                continue
            seen.add(member.name)
            names.append(member.name)
    for name in sorted(names):
        out.emit(f"{name}={analysis.opmap[name]},\n")
    out.emit(")\n\n")


def generate_opmap(analysis: Analysis, out: CWriter) -> None:
    specialized = get_specialized(analysis)
    out.emit("opmap = frozendict(\n")
    for inst, op in analysis.opmap.items():
        if inst not in specialized and inst not in IR_BASE_ONLY:
            out.emit(f"{inst}={analysis.opmap[inst]},\n")
    out.emit(")\n\n")


def generate_ir_opmap(analysis: Analysis, out: CWriter,
                      arms: Sequence[ir_arms.ArmEntry] = ()) -> None:
    """The handlers of the register IR that are not opcodes, by name: the
    old ones, then the register instruction arms by their dotted names"""
    out.emit("_ir_opmap = frozendict(\n")
    ids = handler_ids(analysis) | {
        name: analysis.opmap[name] for name in IR_BASE_ONLY
        if name not in handler_ids(analysis)}
    only = set(ir_only_handler_names(analysis))
    for name, handler in sorted(ids.items(), key=lambda item: item[1]):
        if name in only or name in IR_BASE_ONLY:
            out.emit(f"{name}={handler},\n")
    first = num_old_handlers(analysis)
    if arms:
        out.emit(") | frozendict({\n")
        for arm in arms:
            out.emit(f'"{arm.dotted}": {first + arm.index},\n')
        out.emit("})\n\n")
    else:
        out.emit(")\n\n")
    # Some of these numbers are below 256: a number says nothing about
    # whether the handler is an opcode.
    numbers = sorted([ids[name] for name in only]
                     + [first + arm.index for arm in arms])
    out.emit("_ir_only_handlers = frozenset((\n")
    for number in numbers:
        out.emit(f"{number},\n")
    out.emit("))\n\n")


def is_abort_body(analysis: Analysis, name: str) -> bool:
    inst = all_instructions(analysis).get(name)
    return inst is not None and any(
        isinstance(part, Uop) and tuple(part.clauses.get("exits", ())) == ("abort",)
        for part in inst.parts)


def classify_handlers(analysis: Analysis) -> tuple[list[str], list[str]]:
    """The handlers that bytecodes.c still defines, as (inherited register
    instructions, stack-only stubs).  A stub is a stack op that has no
    handler of its own or whose body
    only aborts: the register instructions its lowering emits run instead.  The arms are not in either list."""
    stubs = {*STACK_ONLY}
    stubs.update(name for name in all_instructions(analysis)
                 if is_abort_body(analysis, name))
    inherited = set(handler_ids(analysis)) | set(IR_BASE_ONLY)
    return sorted(inherited - stubs), sorted(stubs)


def generate_handler_classes(analysis: Analysis, out: CWriter) -> None:
    """The handlers outside the families, which only shrink as the families
    port (Lib/test/test_ir_ceiling.py checks them against a ceiling)"""
    inherited, stubs = classify_handlers(analysis)
    for table, names in (("_ir_inherited_handlers", inherited),
                         ("_ir_stack_stubs", stubs)):
        out.emit(f"{table} = (\n")
        for name in names:
            out.emit(f'    "{name}",\n')
        out.emit(")\n\n")


def generate_py_metadata(
    filenames: list[str], analysis: Analysis, outfile: TextIO,
    arms: Sequence[ir_arms.ArmEntry] = ()
) -> None:
    write_header(__file__, filenames, outfile, "#")
    out = CWriter(outfile, 0, False)
    generate_specializations(analysis, out)
    generate_specialized_opmap(analysis, out)
    generate_opmap(analysis, out)
    generate_ir_opmap(analysis, out, arms)
    generate_handler_classes(analysis, out)
    out.emit(f"HAVE_ARGUMENT = {analysis.have_arg}\n")
    out.emit(f"MIN_INSTRUMENTED_OPCODE = {analysis.min_instrumented}\n")


arg_parser = argparse.ArgumentParser(
    description="Generate the Python file with opcode metadata.",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
)

arg_parser.add_argument(
    "-o", "--output", type=str, help="Generated code", default=DEFAULT_OUTPUT
)

arg_parser.add_argument(
    "input", nargs=argparse.REMAINDER, help="Instruction definition file(s)"
)

if __name__ == "__main__":
    args = arg_parser.parse_args()
    if len(args.input) == 0:
        args.input.append(DEFAULT_INPUT)
    data = analyze_files(args.input)
    with open(args.output, "w") as outfile:
        import rop_generator
        generate_py_metadata(args.input, data, outfile,
                             rop_generator.load_arm_entries())
