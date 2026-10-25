"""Generate the metadata of the register IR.

Reads the instruction definitions from Python/bytecodes.c and writes
Include/internal/pycore_ir_metadata.h, which pycore_ir.h includes:

- the handler indices of the IR-only instructions (annotation `ir_only`),
  which follow the 256 opcodes in name order, then those of the BINARY_OP
  operations and of the NOFB forms, and IR_NUM_HANDLERS;
- _PyIR_Formats, the format of every handler, indexed by handler, as
  ir_generator.compute_formats() computes it for the handlers it generates;
- _PyIR_OnlyNames, the names of the IR-only handlers.

The tables are defined where NEED_IR_METADATA is defined (Python/ir.c) and
declared elsewhere. Handler indices without an instruction have a zeroed
format, without _PyIR_VALID.
"""

import argparse

from analyzer import Analysis, analyze_files
from generators_common import DEFAULT_INPUT, ROOT, write_header
from cwriter import CWriter
from ir_generator import (
    BINARY_OP_FAMILY,
    FIRST_IR_ONLY_HANDLER,
    NOFB_SUFFIX,
    IRFormat,
    binary_op_handlers,
    binary_ops,
    compute_formats,
    handler_ids,
    nofb_bases,
    num_handlers,
)
from typing import TextIO

DEFAULT_OUTPUT = ROOT / "Include/internal/pycore_ir_metadata.h"


def shadow_of(name: str, analysis: Analysis) -> str:
    """The instruction that the handler stands for: itself, for the handler
    of a BINARY_OP operation BINARY_OP, and for a NOFB form its base."""
    if name in binary_op_handlers(analysis):
        return BINARY_OP_FAMILY
    return name.removesuffix(NOFB_SUFFIX)


def format_initializer(fmt: IRFormat, shadow: str, nofb: str) -> str:
    flags = ["_PyIR_VALID"]
    if fmt.window:
        flags.append("_PyIR_WINDOW")
    if fmt.jumps:
        flags.append("_PyIR_JUMPS")
    if fmt.forward:
        flags.append("_PyIR_FORWARD")
    if fmt.no_fold:
        flags.append("_PyIR_NO_FOLD")
    if fmt.transparent:
        flags.append("_PyIR_TRANSPARENT")
    dst = "_PyIR_NO_DST" if fmt.dst_offset is None else str(fmt.dst_offset)
    return (
        f"{{ .flags = {' | '.join(flags)}, "
        f".roles = {{ {', '.join(fmt.roles)} }}, .imm = {fmt.imm}, "
        f".fb = {fmt.fb}, .ninputs = {fmt.n_inputs}, .dst = {dst}, "
        f".borrow_mask = {fmt.borrow_mask:#x}, .shadow = {shadow}, "
        f".nofb = {nofb} }}"
    )


def generate_ir_metadata(
    filenames: list[str], analysis: Analysis, outfile: TextIO
) -> None:
    formats = compute_formats(analysis)
    ids = handler_ids(analysis)
    n = num_handlers(analysis)
    ir_only = sorted((name for name in ids if ids[name] >= FIRST_IR_ONLY_HANDLER),
                     key=lambda name: ids[name])
    write_header(__file__, filenames, outfile)
    out = CWriter(outfile, 0, False)
    with out.header_guard("Py_CORE_IR_METADATA_H"):
        out.emit("#ifndef Py_BUILD_CORE\n")
        out.emit('#  error "this header requires Py_BUILD_CORE define"\n')
        out.emit("#endif\n\n")
        out.emit("/* Handlers of the IR only, after the opcodes */\n")
        for name in ir_only:
            out.emit(f"#define {name:<46} {ids[name]}\n")
        out.emit(f"#define {'IR_NUM_HANDLERS':<46} {n}\n\n")
        ops = binary_op_handlers(analysis)
        if ops:
            out.emit("/* The handler of BINARY_OP operation OP is "
                     "_PyIR_BINARY_OP_FIRST + OP */\n")
            out.emit(f"#define {'_PyIR_BINARY_OP_FIRST':<46} {ops[0]}\n")
            for op, name in zip(binary_ops(), ops):
                out.emit(f"static_assert({name} == _PyIR_BINARY_OP_FIRST + NB_{op}, "
                         f'"{name}");\n')
            out.emit("\n")

        out.emit(f"PyAPI_DATA(const _PyIRFormat) _PyIR_Formats[{n}];\n")
        out.emit("#ifdef NEED_IR_METADATA\n")
        out.emit(f"const _PyIRFormat _PyIR_Formats[{n}] = {{\n")
        # BINARY_OP has a format but no handler: its operations decode as it.
        indices = dict(ids)
        if BINARY_OP_FAMILY in analysis.instructions:
            indices[BINARY_OP_FAMILY] = analysis.instructions[BINARY_OP_FAMILY].opcode
        has_nofb = set(nofb_bases(analysis))
        for name in sorted(indices, key=lambda name: indices[name]):
            nofb = name + NOFB_SUFFIX if name in has_nofb else "0"
            init = format_initializer(formats[name], shadow_of(name, analysis),
                                      nofb)
            out.emit(f"[{name}] = {init},\n")
        out.emit("};\n")
        out.emit("#endif\n\n")

        out.emit(f"PyAPI_DATA(const char *const) _PyIR_OnlyNames[{len(ir_only)}];\n")
        out.emit("#ifdef NEED_IR_METADATA\n")
        out.emit(f"const char *const _PyIR_OnlyNames[{len(ir_only)}] = {{\n")
        for name in ir_only:
            out.emit(f'[{name} - {FIRST_IR_ONLY_HANDLER}] = "{name}",\n')
        out.emit("};\n")
        out.emit("#endif\n")


arg_parser = argparse.ArgumentParser(
    description="Generate the metadata of the register IR.",
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
        generate_ir_metadata(args.input, data, outfile)
