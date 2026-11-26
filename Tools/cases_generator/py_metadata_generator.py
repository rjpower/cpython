"""Generate opcode metadata for Python.
Reads the stack ops from stack_ops.py.
Writes the metadata to _opcode_metadata.py by default.
"""

import argparse

from generators_common import ROOT, write_header
from cwriter import CWriter
from collections.abc import Sequence

import ir_arms
from handler_table import WORD_KINDS
from stack_ops import HAVE_ARGUMENT, MIN_INSTRUMENTED_OPCODE, PSEUDOS, STACK_OPS
from typing import TextIO


DEFAULT_OUTPUT = ROOT / "Lib/_opcode_metadata.py"


def generate_specializations(out: CWriter) -> None:
    # No stack op is specialised: the families are register instructions
    out.emit("_specializations = frozendict(\n")
    out.emit(")\n\n")


def generate_specialized_opmap(out: CWriter) -> None:
    out.emit("_specialized_opmap = frozendict(\n")
    out.emit(")\n\n")


def generate_opmap(out: CWriter) -> None:
    out.emit("opmap = frozendict(\n")
    for op in (*(op for op in STACK_OPS if not op.translator_only), *PSEUDOS):
        out.emit(f"{op.name}={op.opcode},\n")
    out.emit(")\n\n")


def generate_ir_opmap(out: CWriter,
                      arms: Sequence[ir_arms.ArmEntry] = ()) -> None:
    """The register IR's handlers by name and id: the word kinds, then the register instruction arms by their dotted names.  The
    ids are no opcodes; the translator-only ops are no handlers"""
    out.emit("_ir_handlers = frozendict(\n")
    ids = {name: number for number, name in enumerate(WORD_KINDS)}
    for name, handler in ids.items():
        out.emit(f"{name}={handler},\n")
    first = len(WORD_KINDS)
    if arms:
        out.emit(") | frozendict({\n")
        for arm in arms:
            out.emit(f'"{arm.dotted}": {first + arm.index},\n')
        out.emit("})\n\n")
    else:
        out.emit(")\n\n")
    out.emit("_ir_translator_only = frozendict(\n")
    for op in sorted((op for op in STACK_OPS if op.translator_only),
                     key=lambda op: op.opcode):
        out.emit(f"{op.name}={op.opcode},\n")
    out.emit(")\n\n")
    numbers = sorted([*ids.values(), *(first + arm.index for arm in arms)])
    out.emit("_ir_only_handlers = frozenset((\n")
    for number in numbers:
        out.emit(f"{number},\n")
    out.emit("))\n\n")


def generate_py_metadata(
    filenames: list[str], outfile: TextIO,
    arms: Sequence[ir_arms.ArmEntry] = ()
) -> None:
    write_header(__file__, filenames, outfile, "#")
    out = CWriter(outfile, 0, False)
    generate_specializations(out)
    generate_specialized_opmap(out)
    generate_opmap(out)
    generate_ir_opmap(out, arms)
    out.emit(f"HAVE_ARGUMENT = {HAVE_ARGUMENT}\n")
    out.emit(f"MIN_INSTRUMENTED_OPCODE = {MIN_INSTRUMENTED_OPCODE}\n")


arg_parser = argparse.ArgumentParser(
    description="Generate the Python file with opcode metadata.",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
)

arg_parser.add_argument(
    "-o", "--output", type=str, help="Generated code", default=DEFAULT_OUTPUT
)

arg_parser.add_argument(
    "input", nargs=argparse.REMAINDER, help="Source files, named in the header"
)

if __name__ == "__main__":
    args = arg_parser.parse_args()
    if len(args.input) == 0:
        args.input.append(str(ROOT / "Tools/cases_generator/stack_ops.py"))
    with open(args.output, "w") as outfile:
        import rop_generator
        generate_py_metadata(args.input, outfile,
                             rop_generator.load_arm_entries())
