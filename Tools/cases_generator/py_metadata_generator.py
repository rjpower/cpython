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
from ir_generator import HandlerDomain, build_handler_domain
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


def generate_specialized_opmap(analysis: Analysis, domain: HandlerDomain,
                               out: CWriter) -> None:
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
        out.emit(f"{name}={domain.opmap[name]},\n")
    out.emit(")\n\n")


def generate_opmap(analysis: Analysis, domain: HandlerDomain,
                   out: CWriter) -> None:
    specialized = get_specialized(analysis)
    out.emit("opmap = frozendict(\n")
    for inst, op in domain.opmap.items():
        if inst not in specialized:
            out.emit(f"{inst}={domain.opmap[inst]},\n")
    out.emit(")\n\n")


def generate_ir_opmap(analysis: Analysis, domain: HandlerDomain,
                      out: CWriter) -> None:
    "The handlers of the register IR that are not opcodes, by name"
    out.emit("_ir_opmap = frozendict(\n")
    ids = domain.handlers
    for name, handler in sorted(ids.items(), key=lambda item: item[1]):
        if handler >= domain.first_ir_only:
            out.emit(f"{name}={handler},\n")
    out.emit(")\n\n")


def generate_py_metadata(
    filenames: list[str], analysis: Analysis, domain: HandlerDomain,
    outfile: TextIO
) -> None:
    domain.require_analysis(analysis)
    write_header(__file__, filenames, outfile, "#")
    out = CWriter(outfile, 0, False)
    generate_specializations(analysis, out)
    generate_specialized_opmap(analysis, domain, out)
    generate_opmap(analysis, domain, out)
    generate_ir_opmap(analysis, domain, out)
    out.emit(f"HAVE_ARGUMENT = {domain.have_arg}\n")
    out.emit(f"MIN_INSTRUMENTED_OPCODE = {domain.min_instrumented}\n")


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
    domain = build_handler_domain(data)
    with open(args.output, "w") as outfile:
        generate_py_metadata(args.input, data, domain, outfile)
