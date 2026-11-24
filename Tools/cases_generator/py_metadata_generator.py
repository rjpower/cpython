"""Generate opcode metadata for Python.
Reads the instruction definitions from bytecodes.c.
Writes the metadata to _opcode_metadata.py by default.
"""

import argparse

from analyzer import (
    Analysis,
    analyze_files,
)
from generators_common import (
    DEFAULT_INPUT,
    ROOT,
    write_header,
)
from cwriter import CWriter
from collections.abc import Sequence

import ir_numbers
from ir_generator import (IR_BASE_ONLY, first_ir_only_handler, handler_ids,
                          num_old_handlers)
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
                      arms: Sequence[ir_numbers.ArmEntry] = ()) -> None:
    """The handlers of the register IR that are not opcodes, by name: the
    old ones, then the register instruction arms by their dotted names"""
    out.emit("_ir_opmap = frozendict(\n")
    ids = handler_ids(analysis)
    for name, handler in sorted(ids.items(), key=lambda item: item[1]):
        if handler >= first_ir_only_handler(analysis) or name in IR_BASE_ONLY:
            out.emit(f"{name}={handler},\n")
    if not arms:
        out.emit(")\n\n")
        return
    first = num_old_handlers(analysis)
    out.emit(") | frozendict({\n")
    for arm in arms:
        out.emit(f'"{arm.dotted}": {first + arm.index},\n')
    out.emit("})\n\n")


def generate_py_metadata(
    filenames: list[str], analysis: Analysis, outfile: TextIO,
    arms: Sequence[ir_numbers.ArmEntry] = ()
) -> None:
    write_header(__file__, filenames, outfile, "#")
    out = CWriter(outfile, 0, False)
    generate_specializations(analysis, out)
    generate_specialized_opmap(analysis, out)
    generate_opmap(analysis, out)
    generate_ir_opmap(analysis, out, arms)
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
