"""Generate the metadata of the register IR.

Reads the instruction definitions from Python/bytecodes.c and writes
Include/internal/pycore_ir_metadata.h, which pycore_ir.h includes:

- the handler indices of the IR-only instructions (annotation `ir_only`),
  which follow the 256 opcodes in name order, then those of the BINARY_OP
  operations and of the NOFB forms (the count is in opcode_ids.h);
- metadata, dispatch, pristine and installation-state properties;
- _PyIR_Formats, the format of every handler, indexed by handler, as
  ir_generator.compute_formats() computes it for the handlers it generates;

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
    HandlerDomain,
    HandlerKind,
    HandlerState,
    build_handler_domain,
    NOFB_SUFFIX,
    IRFormat,
    binary_op_handlers,
    binary_ops,
    compute_formats,
    nofb_bases,
)
from typing import TextIO

DEFAULT_OUTPUT = ROOT / "Include/internal/pycore_ir_metadata.h"


def format_initializer(fmt: IRFormat, nofb: str) -> str:
    flags = ["_PyIR_VALID"]
    if fmt.window:
        flags.append("_PyIR_WINDOW")
    if fmt.jumps:
        flags.append("_PyIR_JUMPS")
    if fmt.forward:
        flags.append("_PyIR_FORWARD")
    if fmt.transparent:
        flags.append("_PyIR_TRANSPARENT")
    if fmt.store_dst:
        flags.append("_PyIR_STORE_DST")
    if fmt.runs_code:
        flags.append("_PyIR_RUNS_CODE")
    dst = "_PyIR_NO_DST" if fmt.dst_offset is None else str(fmt.dst_offset)
    return (
        f"{{ .flags = {' | '.join(flags)}, "
        f".roles = {{ {', '.join(fmt.roles)} }}, .imm = {fmt.imm}, "
        f".fb = {fmt.fb}, .ninputs = {fmt.n_inputs}, .dst = {dst}, "
        f".borrow_mask = {fmt.borrow_mask:#x}, "
        f".nofb = {nofb} }}"
    )


def generate_ir_metadata(
    filenames: list[str], analysis: Analysis, domain: HandlerDomain,
    outfile: TextIO
) -> None:
    domain.require_analysis(analysis)
    formats = compute_formats(analysis)
    ids = domain.handlers
    n = domain.count
    first = domain.first_ir_only
    ir_only = sorted((name for name in ids if ids[name] >= first),
                     key=lambda name: ids[name])
    write_header(__file__, filenames, outfile)
    out = CWriter(outfile, 0, False)
    with out.header_guard("Py_CORE_IR_METADATA_H"):
        out.emit("#ifndef Py_BUILD_CORE\n")
        out.emit('#  error "this header requires Py_BUILD_CORE define"\n')
        out.emit("#endif\n\n")
        out.emit("/* Handlers of the IR only, after the opcodes and the "
                 "pseudo-instructions */\n")
        out.emit(f"#define {'IR_FIRST_ONLY_HANDLER':<46} {first}\n")
        for name in ir_only:
            out.emit(f"#define {name:<46} {ids[name]}\n")
        out.emit("\n")
        for kind in HandlerKind:
            out.emit(f"#define _PyIR_H_{kind.name} {kind.value}\n")
        for state in HandlerState:
            out.emit(f"#define _PyIR_STATE_{state.name} {state.value}\n")
        out.emit("\n")
        indices = {**domain.opmap, **domain.handlers}
        for ctype, table, values, prefix, enum in (
            ("uint16_t", "_PyIR_HandlerFlags", domain.kinds, "_PyIR_H_", HandlerKind),
            ("uint8_t", "_PyIR_RequiredState", domain.required_state,
             "_PyIR_STATE_", HandlerState),
        ):
            out.emit(f"PyAPI_DATA(const {ctype}) {table}[IR_NUM_HANDLERS];\n")
            out.emit("#ifdef NEED_IR_METADATA\n")
            out.emit(f"const {ctype} {table}[IR_NUM_HANDLERS] = {{\n")
            for name, i in sorted(indices.items(), key=lambda pair: pair[1]):
                flags = " | ".join(prefix + flag.name for flag in enum
                                   if values[name] & flag) or "0"
                out.emit(f"[{i}] = {flags}, /* {name} */\n")
            out.emit("};\n#endif\n\n")
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
            indices[BINARY_OP_FAMILY] = domain.opmap[BINARY_OP_FAMILY]
        has_nofb = set(nofb_bases(analysis))
        for name in sorted(indices, key=lambda name: indices[name]):
            nofb = name + NOFB_SUFFIX if name in has_nofb else "0"
            init = format_initializer(formats[name], nofb)
            out.emit(f"[{name}] = {init},\n")
        out.emit("};\n")
        out.emit("#endif\n\n")

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
    domain = build_handler_domain(data)
    with open(args.output, "w") as outfile:
        generate_ir_metadata(args.input, data, domain, outfile)
