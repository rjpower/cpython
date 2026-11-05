"""Generate the metadata of the register IR.

Reads the instruction definitions from Python/bytecodes.c and writes
Include/internal/pycore_ir_metadata.h, which pycore_ir.h includes:

- the handler indices of the IR-only instructions (annotation `ir_only`),
  which follow the 256 opcodes in name order, then those of the BINARY_OP
  operations, and IR_NUM_HANDLERS;
- _PyFb_<family>, the type of the feedback entry of each family that has
  one (ir_generator.FEEDBACK_LAYOUTS);
- _PyIR_Formats, the instruction contract of every handler, indexed by
  handler: its format, as ir_generator.compute_formats() computes it for the
  handlers it generates, and its own columns (ir_generator.handler_contracts());
- _PyIR_OnlyNames, the names of the IR-only handlers.

The tables are defined where NEED_IR_METADATA is defined (Python/ir.c) and
declared elsewhere. Handler indices without an instruction have a zeroed
row, without _PyIR_VALID.
"""

import argparse

from analyzer import Analysis, analyze_files
from generators_common import DEFAULT_INPUT, ROOT, write_header
from cwriter import CWriter
from ir_generator import (
    BINARY_OP_FAMILY,
    FUSED_SHADOWS,
    first_ir_only_handler,
    Extent,
    HandlerContract,
    IRFormat,
    binary_op_handlers,
    binary_ops,
    compute_formats,
    handler_contracts,
    handler_ids,
    handler_instructions,
    monitoring_events,
    STACK_ONLY,
    num_handlers,
)
from typing import TextIO

DEFAULT_OUTPUT = ROOT / "Include/internal/pycore_ir_metadata.h"


def shadow_of(name: str, analysis: Analysis) -> str:
    """The instruction that the handler stands for: itself, for the handler
    of a BINARY_OP operation BINARY_OP."""
    if name in binary_op_handlers(analysis):
        return BINARY_OP_FAMILY
    # The fused handlers, their specialisations and twins stand for the
    # compare, whose operation they run
    for fused, compare in FUSED_SHADOWS.items():
        if name.startswith(fused) or name.startswith("INSTRUMENTED_" + fused):
            return compare
    return name


def bits(names: list[str]) -> str:
    return " | ".join(names) if names else "0"


def extent_initializer(e: Extent) -> str:
    k, form = e
    return f"{{ {k}, {form} }}"


def format_initializer(fmt: IRFormat, shadow: str,
                       contract: HandlerContract, events: list[str]) -> str:
    # A STACK_ONLY opcode has a format for the CFG but no handler to run
    flags = [] if fmt.group in STACK_ONLY else ["_PyIR_VALID"]
    for present, flag in (
            (fmt.window, "_PyIR_WINDOW"),
            (fmt.jumps, "_PyIR_JUMPS"),
            (fmt.forward, "_PyIR_FORWARD"),
            (fmt.no_fold, "_PyIR_NO_FOLD"),
            (fmt.transparent, "_PyIR_TRANSPARENT"),
            (fmt.store_dst, "_PyIR_STORE_DST"),
            (fmt.runs_code, "_PyIR_RUNS_CODE"),
            (fmt.jump_always, "_PyIR_JUMP_ALWAYS"),
            (fmt.names_target, "_PyIR_NAMES_TARGET"),
            (fmt.fused, "_PyIR_FUSED"),
            (fmt.discards, "_PyIR_DISCARDS"),
            (fmt.unpacks, "_PyIR_UNPACKS")):
        if present:
            flags.append(flag)
    dst = "_PyIR_NO_DST" if fmt.dst_offset is None else str(fmt.dst_offset)
    if fmt.fb_layout is None:
        fb_kind, fb_size = "0", "0"
    else:
        fb_kind, fb_size = fmt.fb_kind, f"sizeof({fmt.fb_layout})"
    events_init = bits([f"(1u << {e})" for e in events])
    return (
        f"{{ .flags = {bits(flags)}, .effects = {bits(contract.effects)}, "
        f".roles = {{ {', '.join(fmt.roles)} }}, .imm = {fmt.imm}, "
        f".fb = {fmt.fb}, .ninputs = {fmt.n_inputs}, .dst = {dst}, "
        f".borrow_mask = {fmt.borrow_mask:#x}, "
        f".fb_kind = {fb_kind}, .fb_size = {fb_size}, "
        f".reads = {extent_initializer(contract.reads)}, "
        f".writes = {extent_initializer(contract.writes)}, "
        f".through = {extent_initializer(contract.through)}, "
        f".shadow = {shadow}, "
        f".twin = {contract.twin or 0}, .events = {events_init} }}"
    )


def generate_ir_metadata(
    filenames: list[str], analysis: Analysis, outfile: TextIO
) -> None:
    formats = compute_formats(analysis)
    contracts = handler_contracts(analysis, formats)
    insts = handler_instructions(analysis)
    ids = handler_ids(analysis)
    n = num_handlers(analysis)
    first = first_ir_only_handler(analysis)
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
        out.emit(f"#define {'IR_NUM_HANDLERS':<46} {n}\n\n")
        ranges = analysis.numbers.ranges
        if ranges:
            out.emit("/* Numbers reserved by ir_numbers.txt for the steps that "
                     "add handlers, inclusive; they stay holes until used */\n")
            for step, (low, high) in ranges.items():
                out.emit(f"/* {step}: {low} to {high - 1} */\n")
            out.emit("\n")
        ops = binary_op_handlers(analysis)
        if ops:
            out.emit("/* The handler of BINARY_OP operation OP is "
                     "_PyIR_BINARY_OP_FIRST + OP */\n")
            out.emit(f"#define {'_PyIR_BINARY_OP_FIRST':<46} {ops[0]}\n")
            for op, name in zip(binary_ops(), ops):
                out.emit(f"static_assert({name} == _PyIR_BINARY_OP_FIRST + NB_{op}, "
                         f'"{name}");\n')
            out.emit("\n")

        layouts = sorted({(fmt.group, fmt.fb_layout) for fmt in formats.values()
                          if fmt.fb_layout is not None})
        if layouts:
            out.emit("/* The feedback entry of each family, which its handlers "
                     "name */\n")
            for group, layout in layouts:
                out.emit(f"typedef {layout} _PyFb_{group};\n")
            out.emit("\n")

        out.emit(f"PyAPI_DATA(const _PyIRFormat) _PyIR_Formats[{n}];\n")
        out.emit("#ifdef NEED_IR_METADATA\n")
        out.emit(f"const _PyIRFormat _PyIR_Formats[{n}] = {{\n")
        # BINARY_OP has a format but no handler: its operations decode as
        # it.  Its row is that of its generic handlers.
        indices = dict(ids)
        rows = dict(contracts)
        if BINARY_OP_FAMILY in analysis.instructions:
            indices[BINARY_OP_FAMILY] = analysis.instructions[BINARY_OP_FAMILY].opcode
            rows[BINARY_OP_FAMILY] = contracts[binary_op_handlers(analysis)[0]]
        # So have the STACK_ONLY opcodes, which the CFG reads
        for name in STACK_ONLY:
            if name in analysis.instructions:
                indices[name] = analysis.instructions[name].opcode
        for name in sorted(indices, key=lambda name: indices[name]):
            twin = rows[name].twin
            events = monitoring_events(twin, insts[twin]) if twin else []
            init = format_initializer(formats[name], shadow_of(name, analysis),
                                      rows[name], events)
            out.emit(f"[{name}] = {init},\n")
        out.emit("};\n")
        out.emit("#endif\n\n")

        out.emit(f"PyAPI_DATA(const char *const) _PyIR_OnlyNames[{n - first}];\n")
        out.emit("#ifdef NEED_IR_METADATA\n")
        out.emit(f"const char *const _PyIR_OnlyNames[{n - first}] = {{\n")
        for name in ir_only:
            out.emit(f'[{name} - IR_FIRST_ONLY_HANDLER] = "{name}",\n')
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
