"""Generate the list of opcode IDs.
Reads the instruction definitions from bytecodes.c.
Writes the IDs to opcode_ids.h by default.
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
from ir_generator import HandlerDomain, build_handler_domain
from cwriter import CWriter
from typing import TextIO


DEFAULT_OUTPUT = ROOT / "Include/opcode_ids.h"


def generate_opcode_header(
    filenames: list[str], analysis: Analysis, domain: HandlerDomain,
    outfile: TextIO
) -> None:
    domain.require_analysis(analysis)
    write_header(__file__, filenames, outfile)
    out = CWriter(outfile, 0, False)
    with out.header_guard("Py_OPCODE_IDS_H"):
        out.emit("/* Instruction opcodes for compiled code */\n")

        def write_define(name: str, op: int) -> None:
            out.emit(f"#define {name:<38} {op:>3}\n")

        for op, name in sorted([(op, name) for (name, op) in domain.opmap.items()]):
            write_define(name, op)

        out.emit("\n")
        write_define("IR_NUM_HANDLERS", domain.count)
        write_define("HAVE_ARGUMENT", domain.have_arg)
        write_define("MIN_SPECIALIZED_OPCODE", domain.opmap["RESUME"]+1)
        write_define("MIN_INSTRUMENTED_OPCODE", domain.min_instrumented)


arg_parser = argparse.ArgumentParser(
    description="Generate the header file with all opcode IDs.",
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
        generate_opcode_header(args.input, data, domain, outfile)
