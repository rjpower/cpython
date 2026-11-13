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

from analyzer import Analysis, Uop, analyze_files
from generators_common import DEFAULT_INPUT, ROOT, write_header
from cwriter import CWriter
from ir_generator import (
    BINARY_OP_FAMILY,
    EXECUTION_VARIANTS,
    canonical_handler,
    call_role_variant,
    call_role_variants,
    FUSED_SHADOWS,
    first_ir_only_handler,
    Extent,
    HandlerContract,
    IRFormat,
    OwnershipEvent,
    binary_op_handlers,
    binary_ops,
    compute_formats,
    handler_contracts,
    handler_ids,
    handler_instructions,
    monitoring_events,
    STACK_ONLY,
    num_handlers,
    uses_feedback,
)
from typing import TextIO

DEFAULT_OUTPUT = ROOT / "Include/internal/pycore_ir_metadata.h"


def shadow_of(name: str, analysis: Analysis) -> str:
    """The instruction that the handler stands for: itself, for the handler
    of a BINARY_OP operation BINARY_OP."""
    if name == "RETURN_VALUE_EMPTY":
        return "RETURN_VALUE"
    if name == "INSTRUMENTED_RETURN_VALUE_EMPTY":
        return "INSTRUMENTED_RETURN_VALUE"
    if name == "YIELD_DELEGATED":
        return "YIELD_VALUE"
    if name == "INSTRUMENTED_YIELD_DELEGATED":
        return "INSTRUMENTED_YIELD_VALUE"
    if name in binary_op_handlers(analysis):
        return BINARY_OP_FAMILY
    # The fused handlers, their specialisations and twins stand for the
    # compare, whose operation they run
    for fused, compare in FUSED_SHADOWS.items():
        if name.startswith(fused) or name.startswith("INSTRUMENTED_" + fused):
            return compare
    return canonical_handler(name)


def handler_facts(analysis: Analysis, formats: dict[str, IRFormat]
                  ) -> tuple[dict[str, int], dict[str, str], dict[str, list[str]]]:
    """The named IR domain, its canonical bases and generated properties."""
    ids = {**analysis.opmap, **handler_ids(analysis)}
    dispatch = handler_ids(analysis)
    insts = handler_instructions(analysis)
    binary = set(binary_op_handlers(analysis))
    bases = {}
    flags = {}
    for name in ids:
        inst = insts.get(name) or analysis.instructions.get(name)
        base = inst.family.name if inst is not None and inst.family else name
        if name.startswith("INSTRUMENTED_") and base == name:
            original = name[len("INSTRUMENTED_"):]
            if original in ids:
                base = original
        call_role = call_role_variant(name)
        if call_role is not None:
            base = call_role[1]
        bases[name] = base
        parts = ["_PyIR_H_METADATA"]
        if name in dispatch:
            parts.append("_PyIR_H_DISPATCH")
            if inst is not None and not inst.properties.no_save_ip:
                parts.append("_PyIR_H_SAVES_IP")
        if inst is not None and inst.family:
            parts.append("_PyIR_H_SPECIALIZED")
        if name.startswith("INSTRUMENTED_"):
            parts.append("_PyIR_H_INSTRUMENTED")
        if name in binary:
            parts.append("_PyIR_H_BINARY_OP")
        if name in analysis.ir_only or name in EXECUTION_VARIANTS or call_role is not None:
            parts.append("_PyIR_H_IR_ONLY")
        if name.endswith("_NOFB"):
            parts.append("_PyIR_H_NOFB")
        if name in analysis.pseudos:
            parts.append("_PyIR_H_PSEUDO")
        if name in dispatch and (inst is None or not inst.family) \
                and not name.startswith("INSTRUMENTED_"):
            parts.append("_PyIR_H_PRISTINE")
            fmt = formats[name]
            if (fmt.fb != "_PyIR_LOC_NONE" and inst is not None
                    and uses_feedback(inst)
                    and any(isinstance(part, Uop)
                            and "specializing" in part.annotations
                            for part in inst.parts)):
                parts.append("_PyIR_H_ADAPTIVE")
        flags[name] = parts
    for name, base in bases.items():
        if base not in ids or bases[base] != base:
            raise ValueError(f"noncanonical handler base: {name} -> {base}")
    if len(set(ids.values())) != len(ids) or max(ids.values()) >= num_handlers(analysis):
        raise ValueError("invalid handler domain")
    return ids, bases, flags


def bits(names: list[str]) -> str:
    return " | ".join(names) if names else "0"


def extent_initializer(e: Extent) -> str:
    k, form = e
    return f"{{ {k}, {form} }}"


def format_initializer(fmt: IRFormat, shadow: str,
                       contract: HandlerContract, events: list[str],
                       owner_program: tuple[int, int] = (0, 0)) -> str:
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
            (fmt.unpacks, "_PyIR_UNPACKS"),
            (contract.abrupt_prefix, "_PyIR_ABRUPT_PREFIX"),
            (contract.abrupt_operands, "_PyIR_ABRUPT_OPERANDS"),
            (bool(contract.error_checkpoints), "_PyIR_ERROR_PROGRAM")):
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
        f".result_phase = {contract.result_phase}, "
        f".output_kind = {contract.output_kind}, "
        f".transfer_kind = {contract.transfer_kind}, "
        f".owner_program = {owner_program[0]}, .owner_events = {owner_program[1]}, "
        f".call_input_holes = {contract.call_input_holes}, "
        f".error_routes = {bits(list(contract.error_routes))}, "
        f".observer_outcome = {{ {', '.join(contract.observer_outcome)} }}, "
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
    programs = {((), ()): (0, 0)}
    ownership = []
    for name in sorted(contracts):
        normal = contracts[name].ownership_events
        checkpoints = contracts[name].error_checkpoints
        key = normal, checkpoints
        if key not in programs:
            actions = max((checkpoint.actions for checkpoint in checkpoints),
                          key=len, default=())
            suffix = list(actions)
            for checkpoint in checkpoints:
                if actions[:len(checkpoint.actions)] != checkpoint.actions:
                    raise ValueError("error checkpoints have different storage graphs")
                suffix.append(OwnershipEvent("_PyIR_OWNER_ERROR_CHECKPOINT",
                    flags=checkpoint.flags, dst=checkpoint.node,
                    src=checkpoint.id))
            program = normal
            if suffix:
                program += (OwnershipEvent("_PyIR_OWNER_ERROR_PROGRAM",
                            src=len(actions), extent=len(suffix)), *suffix)
            if len(program) > 65535 or len(ownership) + len(program) > 0xffffffff:
                raise ValueError("ownership event table is too large")
            programs[key] = len(ownership), len(normal)
            ownership.extend(program)
    insts = handler_instructions(analysis)
    ids = handler_ids(analysis)
    fact_ids, bases, flags = handler_facts(analysis, formats)
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
        observer_entry = all(contracts[name].observer_entry for name in
                             ("INSTRUMENTED_LINE", "INSTRUMENTED_INSTRUCTION"))
        out.emit(f"#define _PyIR_OBSERVER_ENTRY_ABSENCE {int(observer_entry)}\n\n")
        count = max(len(ownership), 1)
        out.emit(f"PyAPI_DATA(const _PyIROwnershipEvent) _PyIR_OwnerPrograms[{count}];\n")
        out.emit("#ifdef NEED_IR_METADATA\n")
        out.emit(f"const _PyIROwnershipEvent _PyIR_OwnerPrograms[{count}] = {{\n")
        for event in ownership:
            out.emit(f"{{ {event.kind}, {event.flags}, {event.dst}, "
                     f"{event.src}, {event.extent} }},\n")
        if not ownership:
            out.emit("{0},\n")
        out.emit("};\n#endif\n\n")
        out.emit("/* Handlers of the IR only, after the opcodes and the "
                 "pseudo-instructions */\n")
        out.emit(f"#define {'IR_FIRST_ONLY_HANDLER':<46} {first}\n")
        for name in ir_only:
            out.emit(f"#define {name:<46} {ids[name]}\n")
        out.emit("\n")
        for kind, bit in (("SPECIALIZED", 1), ("INSTRUMENTED", 2),
                          ("BINARY_OP", 4), ("IR_ONLY", 8), ("NOFB", 16),
                          ("PSEUDO", 32), ("METADATA", 64), ("DISPATCH", 128),
                          ("PRISTINE", 256), ("ADAPTIVE", 512),
                          ("SAVES_IP", 1024)):
            out.emit(f"#define _PyIR_H_{kind} {bit}\n")
        out.emit("\n")
        for ctype, table, values in (
                ("uint16_t", "_PyIR_HandlerFlags", {
                    name: " | ".join(flags[name]) for name in fact_ids}),
                ("uint16_t", "_PyIR_HandlerBase", {
                    name: str(fact_ids[bases[name]]) for name in fact_ids})):
            out.emit(f"PyAPI_DATA(const {ctype}) {table}[IR_NUM_HANDLERS];\n")
            out.emit("#ifdef NEED_IR_METADATA\n")
            out.emit(f"const {ctype} {table}[IR_NUM_HANDLERS] = {{\n")
            for name in sorted(fact_ids, key=fact_ids.get):
                out.emit(f"[{fact_ids[name]}] = {values[name]}, /* {name} */\n")
            out.emit("};\n#endif\n\n")
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

        out.emit("PyAPI_DATA(const uint16_t) _PyIR_CallRoleHandlers[2][256];\n")
        out.emit("#ifdef NEED_IR_METADATA\n")
        out.emit("const uint16_t _PyIR_CallRoleHandlers[2][256] = {\n")
        for row, holes in enumerate((2, 10)):
            out.emit(f"[{row}] = {{\n")
            for name, (base, _, mask) in sorted(call_role_variants(analysis).items()):
                if mask == holes:
                    out.emit(f"[{base}] = {name},\n")
            out.emit("},\n")
        out.emit("};\n#endif\n\n")

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
            # Semantic completion can report events without a separate
            # instrumented handler (SEND). Keep that capability in its row.
            event_handler = twin or name
            events = (monitoring_events(event_handler, insts[event_handler])
                      if event_handler in insts else [])
            init = format_initializer(formats[name], shadow_of(name, analysis),
                                      rows[name], events,
                                      programs[rows[name].ownership_events, rows[name].error_checkpoints])
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
