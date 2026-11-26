"""Generate the list of opcode IDs.
Reads the stack ops from stack_ops.py.
Writes the IDs to opcode_ids.h by default.
"""

import argparse
from collections.abc import Sequence

import ir_arms

from generators_common import ROOT, write_header
from cwriter import CWriter
from handler_table import first_arm, handler_ids, num_handlers
from stack_opcode_pins import (
    HAVE_ARGUMENT, MIN_INSTRUMENTED_OPCODE, MIN_SPECIALIZED_OPCODE,
    PINNED_OPCODES, PINNED_PSEUDOS,
)
from stack_ops import STACK_OPS
import rop_generator
from typing import TextIO


DEFAULT_OUTPUT = ROOT / "Include/opcode_ids.h"
DEFAULT_HANDLER_IDS_OUTPUT = ROOT / "Include/ir_handler_ids.h"


def generate_opcode_header(filenames: list[str], outfile: TextIO) -> None:
    """The stack opcode numbers, from the pinned table alone: a port that
    deletes or stubs an op leaves this file as it was."""
    write_header(__file__, filenames, outfile)
    out = CWriter(outfile, 0, False)
    with out.header_guard("Py_OPCODE_IDS_H"):
        out.emit("/* Instruction opcodes for compiled code */\n")

        def write_define(name: str, op: int) -> None:
            out.emit(f"#define {name:<38} {op:>3}\n")

        pins = PINNED_OPCODES | PINNED_PSEUDOS
        for op, name in sorted((op, name) for (name, op) in pins.items()):
            write_define(name, op)

        out.emit("\n")
        write_define("HAVE_ARGUMENT", HAVE_ARGUMENT)
        write_define("MIN_SPECIALIZED_OPCODE", MIN_SPECIALIZED_OPCODE)
        write_define("MIN_INSTRUMENTED_OPCODE", MIN_INSTRUMENTED_OPCODE)


def generate_handler_ids_header(
    filenames: list[str], outfile: TextIO,
    arms: Sequence[ir_arms.ArmEntry] = ()
) -> None:
    """The register handler ids.  They are dense, computed at each build and
    independent of the stack opcodes: IR_H_<name> for every word kind, then the
    first arm and the counts.  The numbers of the translator-only
    ops follow: they are CFG numbers in the gap that no stack opcode holds,
    and no word has them as a handler."""
    write_header(__file__, filenames, outfile)
    out = CWriter(outfile, 0, False)
    with out.header_guard("Py_IR_HANDLER_IDS_H"):
        out.emit("/* Register handler ids; not stack opcodes */\n")
        for name, number in handler_ids().items():
            out.emit(f"#define IR_H_{name:<36} {number:>3}\n")
        out.emit("\n")
        out.emit(f"#define {'IR_FIRST_ARM':<38} {first_arm():>3}\n")
        out.emit(f"#define {'IR_NUM_HANDLERS':<38} {num_handlers(arms):>3}\n")
        out.emit("\n/* What _PyIR_HandlerBase holds for a handler that is no stack op: "
                 "a number\n * past every stack opcode and pseudo-instruction */\n")
        out.emit("#define IR_HANDLER_ONLY_BASE(handler) (512 + (handler))\n")
        out.emit("\n/* Ops of the translator's CFG that no co_code holds: "
                 "numbers in the gap of the stack opcodes */\n")
        for op in sorted((op for op in STACK_OPS if op.translator_only),
                         key=lambda op: op.opcode):
            out.emit(f"#define {op.name:<38} {op.opcode:>3}\n")


arg_parser = argparse.ArgumentParser(
    description="Generate the header file with all opcode IDs.",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
)

arg_parser.add_argument(
    "-o", "--output", type=str, help="Generated code", default=DEFAULT_OUTPUT
)

arg_parser.add_argument(
    "--handler-ids-output", type=str, default=DEFAULT_HANDLER_IDS_OUTPUT,
    help="Generated register handler ids",
)

arg_parser.add_argument(
    "input", nargs=argparse.REMAINDER, help="Source files, named in the header"
)

if __name__ == "__main__":
    args = arg_parser.parse_args()
    if len(args.input) == 0:
        args.input.append(str(ROOT / "Tools/cases_generator/stack_ops.py"))
    with open(args.output, "w") as outfile:
        generate_opcode_header(args.input, outfile)
    with open(args.handler_ids_output, "w") as outfile:
        generate_handler_ids_header(args.input, outfile,
                                    rop_generator.load_arm_entries())
