"""Generate the translator's rows of the stack ops:
Include/internal/pycore_regcfg_metadata.h.

One _PyStackFormat (Include/internal/pycore_regcfg.h) per stack op of
stack_ops.py, by stack number.  Only Python/regcfg.c includes the header; the
translator derives an arm's row from its word format instead
(_PyRegCfg_ArmFormat).  What a stack op means is in the translator's rules.
"""

import argparse
from typing import TextIO

from cwriter import CWriter
from generators_common import ROOT, write_header
from stack_ops import STACK_OPS, StackFormat

DEFAULT_OUTPUT = ROOT / "Include/internal/pycore_regcfg_metadata.h"

# The flags of the translator's own (_PyStack_*); the others are the word
# format's (_PyIR_*)
STACK_FLAGS = frozenset({"WINDOW", "NO_FOLD", "STORE_DST", "DISCARDS"})


def bits(names: list[str]) -> str:
    return " | ".join(names) if names else "0"


def stack_format_row(fmt: StackFormat) -> str:
    "A _PyStackFormat row"
    roles = ", ".join("_PyIR_ROLE_" + role.replace("+", " + ") for role in fmt.roles)
    dst = "_PyStack_NO_DST" if fmt.dst is None else str(fmt.dst)
    flags = [("_PyStack_" if f in STACK_FLAGS else "_PyIR_") + f for f in fmt.flags]
    k, form = fmt.through
    return (
        f"{{ .flags = {bits(flags)}, "
        f".effects = {bits(['_PyIR_' + e for e in fmt.effects])}, "
        f".roles = {{ {roles} }}, .imm = _PyIR_LOC_{fmt.imm}, "
        f".ninputs = {fmt.ninputs}, .dst = {dst}, "
        f".borrow_mask = {fmt.borrow_mask:#x}, "
        f".operands = _PyIR_OPERANDS_{fmt.operands}, "
        f".through = {{ {k}, _PyStack_EXT_{form} }} }}"
    )


def generate_regcfg_metadata(filenames: list[str], outfile: TextIO) -> None:
    write_header(__file__, filenames, outfile)
    out = CWriter(outfile, 0, False)
    with out.header_guard("Py_CORE_REGCFG_METADATA_H"):
        out.emit("#ifndef Py_BUILD_CORE\n")
        out.emit('#  error "this header requires Py_BUILD_CORE define"\n')
        out.emit("#endif\n\n")
        out.emit('#include "pycore_regcfg.h"         // _PyStackFormat\n\n')
        out.emit("static const _PyStackFormat _PyStack_Formats[256] = {\n")
        for op in sorted(STACK_OPS, key=lambda op: op.opcode):
            assert op.format is not None, op.name
            out.emit(f"[{op.opcode}] = {stack_format_row(op.format)},  /* {op.name} */\n")
        out.emit("};\n")


arg_parser = argparse.ArgumentParser(
    description="Generate the translator's rows of the stack ops.",
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
        generate_regcfg_metadata(args.input, outfile)
