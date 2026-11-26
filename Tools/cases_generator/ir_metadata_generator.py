"""Generate the metadata of the register IR.

Reads the instruction definitions from Python/bytecodes.c and writes
Include/internal/pycore_ir_metadata.h, which pycore_ir.h includes:

- the handler indices of the IR-only instructions (annotation `ir_only`),
  which follow the 256 opcodes in name order, and IR_NUM_HANDLERS;
- _PyFb_<family>, the type of the feedback entry of each family that has
  one (ir_generator.FEEDBACK_LAYOUTS);
- _PyIR_Formats, the instruction contract of every handler, indexed by
  handler: its format, as ir_generator.compute_formats() computes it for the
  handlers it generates, and its own columns (ir_generator.handler_contracts());
- _PyIR_OnlyNames, the names of the IR-only handlers.

The register instruction arms (Python/register_ops) follow the old handlers
in every per-handler table, then the observed wrappers of the arms of
families that declare events: rop_generator.arm_entries() lists them, and
IR_NUM_HANDLERS counts them.

The tables are defined where NEED_IR_METADATA is defined (Python/ir.c) and
declared elsewhere. Handler indices without an instruction have a zeroed
row, without _PyIR_VALID.
"""

import argparse
from collections.abc import Sequence

import ir_arms
from analyzer import Analysis, Uop, analyze_files, stack_ops_of
from generators_common import DEFAULT_INPUT, ROOT, write_header
from cwriter import CWriter
from ir_generator import (
    EXECUTION_VARIANTS,
    canonical_handler,
    Extent,
    HandlerContract,
    IRFormat,
    OwnershipEvent,
    call_role_flags,
    compute_formats,
    operand_metadata,
    handler_contracts,
    handler_ids,
    handler_instructions,
    ir_only_handler_names,
    monitoring_events,
    stack_only_names,
    num_handlers,
    num_old_handlers,
    uses_feedback,
)
from typing import TextIO

DEFAULT_OUTPUT = ROOT / "Include/internal/pycore_ir_metadata.h"


def shadow_of(name: str, analysis: Analysis) -> str:
    """The instruction that the handler stands for: itself, for the handler
    of a BINARY_OP operation BINARY_OP."""
    # An IR-only member of a real family (LOAD_ATTR_GETSET) stands for the
    # family head, as the specialisations that have an opcode stand for
    # themselves
    inst = handler_instructions(analysis).get(name)
    if name in analysis.ir_only and inst is not None and inst.family:
        return inst.family.name
    return canonical_handler(name)


def stack_numbers(analysis: Analysis) -> dict[str, int]:
    "The stack opcode of each stack op, or CFG number of a translator-only op"
    return {name: inst.opcode for name, inst in stack_ops_of(analysis).items()}


def base_number(name: str, numbers: dict[str, int], ids: dict[str, int]) -> int:
    """The number that stands for a canonical op in _PyIR_HandlerBase: its
    stack opcode, or for a handler that has no stack op (YIELD_DELEGATED) a
    number past every stack opcode and pseudo-instruction"""
    return numbers[name] if name in numbers else 512 + ids[name]


def handler_facts(analysis: Analysis, formats: dict[str, IRFormat]
                  ) -> tuple[dict[str, int], dict[str, str], dict[str, list[str]]]:
    """The named handlers with their ids, the canonical stack op of each
    handler (by name; a stack-only stub can be one) and generated properties."""
    ids = handler_ids(analysis)
    numbers = stack_numbers(analysis)
    dispatch = ids
    insts = handler_instructions(analysis)

    bases = {}
    flags = {}
    for name in [*ids, *(name for name in numbers if name not in ids)]:
        ordinary = name
        inst = insts.get(name) or stack_ops_of(analysis).get(ordinary)
        base = inst.family.name if inst is not None and inst.family else ordinary
        if ordinary.startswith("INSTRUMENTED_") and base == ordinary:
            original = ordinary[len("INSTRUMENTED_"):]
            if original in numbers or original in ids:
                base = original
        bases[name] = base
        if name not in ids:
            continue
        parts = ["_PyIR_H_METADATA"]
        if name in dispatch:
            parts.append("_PyIR_H_DISPATCH")
            if inst is not None and not inst.properties.no_save_ip:
                parts.append("_PyIR_H_SAVES_IP")
        if inst is not None and inst.family:
            parts.append("_PyIR_H_SPECIALIZED")
        if ordinary.startswith("INSTRUMENTED_"):
            parts.append("_PyIR_H_INSTRUMENTED")
        if name in analysis.ir_only or name in EXECUTION_VARIANTS:
            parts.append("_PyIR_H_IR_ONLY")
        if name.endswith("_NOFB"):
            parts.append("_PyIR_H_NOFB")
        # The baseline JIT's forms of RESUME and JUMP_BACKWARD end in its
        # warm-up event, which is nothing in compiled code: a unit compiles
        # them as their base handler (Python/jit.c)
        if inst is not None and any(isinstance(part, Uop) and part.name == "_JIT_EVENT"
                                    for part in inst.parts):
            parts.append("_PyIR_H_JIT_EVENT")
        if name in dispatch and (inst is None or not inst.family) \
                and not ordinary.startswith("INSTRUMENTED_"):
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
        if base not in numbers and base not in ids:
            raise ValueError(f"noncanonical handler base: {name} -> {base}")
    return ids, bases, flags


def bits(names: list[str]) -> str:
    return " | ".join(names) if names else "0"


def extent_initializer(e: Extent) -> str:
    k, form = e
    return f"{{ {k}, {form} }}"


def format_initializer(fmt: IRFormat, shadow: str,
                       contract: HandlerContract, events: list[str],
                       stack_only: frozenset[str],
                       owner_program: tuple[int, int] = (0, 0),
                       twin: str = "0") -> str:
    if fmt.operands != "_PyIR_OPERANDS_NONE":
        fmt = operand_metadata(fmt)
    # A STACK_ONLY opcode has a format for the CFG but no handler to run
    flags = [] if fmt.group in stack_only else ["_PyIR_VALID"]
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
            (fmt.discards, "_PyIR_DISCARDS"),
            (fmt.unpacks, "_PyIR_UNPACKS"),
            (contract.abrupt_prefix, "_PyIR_ABRUPT_PREFIX"),
            (contract.abrupt_operands, "_PyIR_ABRUPT_OPERANDS"),
            (bool(contract.error_checkpoints or contract.normal_storage), "_PyIR_ERROR_PROGRAM")):
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
        f".word_flags = {bits(list(contract.word_flags))}, "
        f".error_routes = {bits(list(contract.error_routes))}, "
        f".operands = {fmt.operands}, "
        f".observer_outcome = {{ {', '.join(contract.observer_outcome)} }}, "
        f".reads = {extent_initializer(contract.reads)}, "
        f".writes = {extent_initializer(contract.writes)}, "
        f".through = {extent_initializer(contract.through)}, "
        f".shadow = {shadow}, "
        f".twin = {twin}, "
        f".events = {events_init} }}"
    )


def arm_id(analysis: Analysis, arm: ir_arms.ArmEntry) -> int:
    return num_old_handlers(analysis) + arm.index


def arm_base_ids(analysis: Analysis, arms: Sequence[ir_arms.ArmEntry],
                 numbers: dict[str, int], bases: dict[str, str]) -> dict[str, int]:
    """The stack opcode of the canonical stack op that a family names"""
    result = {}
    for arm in arms:
        op = arm.base
        if op not in analysis.opmap or op in analysis.pseudos:
            raise ValueError(f"{arm.dotted}: ops({op}) is not a stack op")
        if bases.get(op, op) != op:
            raise ValueError(f"{arm.dotted}: ops({op}) is not a canonical opcode "
                             f"(its base is {bases[op]})")
        result[op] = numbers[op]
    return result


def twin_by_tools(analysis: Analysis, arm: ir_arms.ArmEntry) -> bool:
    """Whether monitoring selects the arm's observed twin by handler: its
    family declares events, but upstream's pinned table has no
    INSTRUMENTED_ stack opcode of its base to rewrite the opcode to"""
    return bool(arm.events) and f"INSTRUMENTED_{arm.base}" not in analysis.opmap


def arm_flags(analysis: Analysis, arm: ir_arms.ArmEntry) -> str:
    parts = ["_PyIR_H_METADATA"]
    if arm.emitted:
        parts.append("_PyIR_H_DISPATCH")
        if not arm.trailing:
            parts.append("_PyIR_H_SAVES_IP")
    if arm.adaptive and not arm.is_generic:
        parts.append("_PyIR_H_SPECIALIZED")
    if arm.observes is not None:
        parts.append("_PyIR_H_INSTRUMENTED")
    if twin_by_tools(analysis, arm):
        parts.append("_PyIR_H_TWIN_BY_TOOLS")
    if arm.trailing:
        parts.append("_PyIR_H_TRAILING")
    parts.append("_PyIR_H_IR_ONLY")
    return " | ".join(parts)


def arm_owner_program(fmt: ir_arms.ArmFormat) -> tuple[OwnershipEvent, ...]:
    """An arm's owner program: the leaf of its derived transfer, as an old
    handler with a checked leaf contract has (none for UNKNOWN)"""
    if fmt.transfer == "_PyIR_TRANSFER_UNKNOWN":
        return ()
    return (OwnershipEvent("_PyIR_OWNER_LEAF", src=fmt.transfer),)


def arm_row(analysis: Analysis, arm: ir_arms.ArmEntry,
            program: tuple[int, int] = (0, 0)) -> str:
    """The _PyIRFormat row of an arm, or of its observed wrapper; program
    is its owner program's (start, length) in the owner table"""
    fmt = arm.format
    assert fmt is not None
    own = arm_id(analysis, arm)
    if arm.observes is not None:
        twin = own
    else:
        twin = 0 if arm.twin is None else num_old_handlers(analysis) + arm.twin
    fb_kind, fb_size = "0", "0"
    if fmt.fb_schema is not None:
        fb_kind = f"FB_{fmt.fb_schema.upper()}"
        fb_size = f"sizeof(_PyFb{fmt.fb_schema})"
    events = bits([f"(1u << PY_MONITORING_EVENT_{e})" for e in arm.events])
    fixed = f".fixed_words = {fmt.fixed_words}, " if fmt.fixed_words else ""
    dst = "_PyIR_NO_DST" if fmt.dst is None else str(fmt.dst)
    # sys.monitoring sees an observed wrapper as the instrumented form of
    # its family's stack op, as it sees an old handler's twin
    shadow = arm.base
    if arm.observes is not None and f"INSTRUMENTED_{arm.base}" in analysis.opmap:
        shadow = f"INSTRUMENTED_{arm.base}"
    # An arm of a call family reads the call-role flags of its word
    word_flags = bits(list(call_role_flags(arm.base, None)))
    return (
        f"{{ .flags = {bits(list(fmt.flags))}, .effects = {bits(list(fmt.effects))}, "
        f".roles = {{ {', '.join(fmt.roles)} }}, .imm = {fmt.imm}, "
        f".fb = {fmt.fb}, .target = {fmt.target}, .ninputs = {fmt.ninputs}, "
        f".dst = {dst}, "
        f".borrow_mask = {fmt.borrow_mask:#x}, .fb_kind = {fb_kind}, "
        f".fb_size = {fb_size}, .result_phase = _PyIR_RESULT_PHASE_NONE, "
        f".output_kind = _PyIR_OUTPUT_REGISTER_DST, "
        f".transfer_kind = {fmt.transfer}, .owner_program = {program[0]}, "
        f".owner_events = {program[1]}, .word_flags = {word_flags}, .error_routes = 0, "
        f".operands = {fmt.operands}, {fixed}"
        f".observer_outcome = {{ {fmt.observer}, {fmt.observer_taken or fmt.observer} }}, "
        f".reads = {{ {fmt.ninputs if fmt.reads is None else fmt.reads}, _PyIR_EXT_FIXED }}, "
        f".writes = {{ {fmt.writes}, _PyIR_EXT_FIXED }}, "
        f".through = {{ {fmt.through}, _PyIR_EXT_FIXED }}, .shadow = {shadow}, "
        f".twin = {twin}, .events = {events} }}")


def generate_ir_metadata(
    filenames: list[str], analysis: Analysis, outfile: TextIO,
    arms: Sequence[ir_arms.ArmEntry] = ()
) -> None:
    formats = compute_formats(analysis)
    contracts = handler_contracts(analysis, formats)
    programs = {((), (), None): (0, 0)}
    ownership = []
    for name in sorted(contracts):
        normal = contracts[name].ownership_events
        checkpoints = contracts[name].error_checkpoints
        final = contracts[name].normal_storage
        key = normal, checkpoints, final
        if key not in programs:
            snapshots = checkpoints + ((final,) if final is not None else ())
            actions = max((checkpoint.actions for checkpoint in snapshots),
                          key=len, default=())
            suffix = list(actions)
            for checkpoint in snapshots:
                if actions[:len(checkpoint.actions)] != checkpoint.actions:
                    raise ValueError("error checkpoints have different storage graphs")
                suffix.append(OwnershipEvent("_PyIR_OWNER_ERROR_CHECKPOINT" if checkpoint.id else
                    "_PyIR_OWNER_STORAGE_NORMAL",
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
    for arm in arms:
        assert arm.format is not None
        key = arm_owner_program(arm.format), (), None
        if key not in programs:
            programs[key] = len(ownership), len(key[0])
            ownership.extend(key[0])
    insts = handler_instructions(analysis)
    ids = handler_ids(analysis)
    fact_ids, bases, flags = handler_facts(analysis, formats)
    n = num_handlers(analysis, arms)
    numbers = stack_numbers(analysis)
    base_ids = arm_base_ids(analysis, arms, numbers, bases)
    ir_only = sorted(ir_only_handler_names(analysis), key=lambda name: ids[name])
    write_header(__file__, filenames, outfile)
    out = CWriter(outfile, 0, False)
    with out.header_guard("Py_CORE_IR_METADATA_H"):
        out.emit("#ifndef Py_BUILD_CORE\n")
        out.emit('#  error "this header requires Py_BUILD_CORE define"\n')
        out.emit("#endif\n\n")
        observer_entry = all(name in contracts and contracts[name].observer_entry
                             for name in ("INSTRUMENTED_LINE", "INSTRUMENTED_INSTRUCTION"))
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
        for kind, bit in (("SPECIALIZED", 1), ("INSTRUMENTED", 2),
                          ("IR_ONLY", 8), ("NOFB", 16),
                          ("METADATA", 64), ("DISPATCH", 128),
                          ("PRISTINE", 256), ("ADAPTIVE", 512),
                          ("SAVES_IP", 1024), ("JIT_EVENT", 2048),
                          ("TWIN_BY_TOOLS", 4096), ("TRAILING", 8192)):
            out.emit(f"#define _PyIR_H_{kind} {bit}\n")
        out.emit("\n")
        for ctype, table, values in (
                ("uint16_t", "_PyIR_HandlerFlags", {
                    name: " | ".join(flags[name]) for name in fact_ids}),
                ("uint16_t", "_PyIR_HandlerBase", {
                    name: str(base_number(bases[name], numbers, fact_ids)) for name in fact_ids})):
            out.emit(f"PyAPI_DATA(const {ctype}) {table}[IR_NUM_HANDLERS];\n")
            out.emit("#ifdef NEED_IR_METADATA\n")
            out.emit(f"const {ctype} {table}[IR_NUM_HANDLERS] = {{\n")
            for name in sorted(fact_ids, key=fact_ids.get):
                out.emit(f"[{fact_ids[name]}] = {values[name]}, /* {name} */\n")
            for arm in arms:
                value = (arm_flags(analysis, arm) if table == "_PyIR_HandlerFlags"
                         else str(base_ids[arm.base]))
                out.emit(f"[{arm_id(analysis, arm)}] = {value}, /* {arm.dotted} */\n")
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
        stack_only = stack_only_names(analysis)
        stack_ops = stack_ops_of(analysis)

        def row(name: str) -> str:
            twin = contracts[name].twin
            # Semantic completion can report events without a separate
            # instrumented handler (SEND). Keep that capability in its row.
            event_handler = twin or name
            events = (monitoring_events(event_handler, insts[event_handler])
                      if event_handler in insts else [])
            shadow = shadow_of(name, analysis)
            return format_initializer(
                formats[name], shadow if shadow in numbers else "0",
                contracts[name], events, stack_only,
                programs[contracts[name].ownership_events,
                         contracts[name].error_checkpoints,
                         contracts[name].normal_storage],
                f"IR_H_{twin}" if twin in ids else "0")

        for name in sorted(ids, key=ids.get):
            out.emit(f"[{ids[name]}] = {row(name)},  /* {name} */\n")
        for arm in arms:
            assert arm.format is not None
            program = programs[arm_owner_program(arm.format), (), None]
            out.emit(f"[{arm_id(analysis, arm)}] = {arm_row(analysis, arm, program)},  "
                     f"/* {arm.dotted} */\n")
        out.emit("};\n")
        out.emit("#endif\n\n")

        # The CFG reads the format of a stack op by its stack number
        # (_PyRegCfg_FormatOf): the ops that have a handler go through
        # _PyIR_OpcodeHandler, the others have a row here
        out.emit("PyAPI_DATA(const _PyIRFormat) _PyIR_StackOnlyFormats[256];\n")
        out.emit("#ifdef NEED_IR_METADATA\n")
        out.emit("const _PyIRFormat _PyIR_StackOnlyFormats[256] = {\n")
        for name in sorted(stack_only, key=lambda name: numbers.get(name, 0)):
            if name in stack_ops and name not in ids:
                out.emit(f"[{numbers[name]}] = {row(name)},  /* {name} */\n")
        out.emit("};\n")
        out.emit("#endif\n\n")

        none = "_PyIR_NO_HANDLER"
        out.emit("/* The stack opcode that a handler is, or _PyIR_NO_OPCODE; the "
                 "handler of a\n * stack opcode, or _PyIR_NO_HANDLER */\n")
        out.emit("#define _PyIR_NO_HANDLER 0xffff\n#define _PyIR_NO_OPCODE 0xffff\n")
        out.emit("PyAPI_DATA(const uint16_t) _PyIR_HandlerOpcode[IR_NUM_HANDLERS];\n")
        out.emit("PyAPI_DATA(const uint16_t) _PyIR_OpcodeHandler[256];\n")
        out.emit("PyAPI_DATA(const uint16_t) _PyIR_OpcodeBase[256];\n")
        out.emit("#ifdef NEED_IR_METADATA\n")
        out.emit("const uint16_t _PyIR_HandlerOpcode[IR_NUM_HANDLERS] = {\n")
        by_id = {number: name for name, number in ids.items()}
        for i in range(n):
            name = by_id.get(i)
            own = (analysis.instructions[name].opcode
                   if name in analysis.instructions else None)
            out.emit(f"[{i}] = {'_PyIR_NO_OPCODE' if own is None else own},\n")
        out.emit("};\n")
        out.emit("const uint16_t _PyIR_OpcodeBase[256] = {\n")
        by_number = {number: name for name, number in numbers.items()
                     if name in analysis.instructions}
        for number in range(256):
            name = by_number.get(number)
            base = ('_PyIR_NO_OPCODE' if name is None
                    else base_number(bases[name], numbers, fact_ids))
            out.emit(f"[{number}] = {base},\n")
        out.emit("};\n")
        out.emit("const uint16_t _PyIR_OpcodeHandler[256] = {\n")
        for number in range(256):
            handler = next((ids[name] for name, inst in analysis.instructions.items()
                            if inst.opcode == number and name in ids), None)
            out.emit(f"[{number}] = {none if handler is None else handler},\n")
        out.emit("};\n")
        out.emit("#endif\n\n")

        out.emit(f"PyAPI_DATA(const char *const) _PyIR_OnlyNames[IR_NUM_HANDLERS];\n")
        out.emit("#ifdef NEED_IR_METADATA\n")
        out.emit("const char *const _PyIR_OnlyNames[IR_NUM_HANDLERS] = {\n")
        for name in ir_only:
            out.emit(f'[{ids[name]}] = "{name}",\n')
        for arm in arms:
            out.emit(f'[{arm_id(analysis, arm)}] = "{arm.dotted}",\n')
        out.emit("};\n")
        out.emit("#endif\n\n")
        out.emit("/* The IR-only handlers and the arms have a name here; the "
                 "opcodes do not */\n")
        out.emit("static inline int\n_PyIR_IsOnlyHandler(int handler)\n{\n")
        out.emit("    return _PyIR_OnlyNames[handler] != NULL;\n}\n\n")
        out.emit("/* Defined in Python/ir.c */\n")
        out.emit("PyAPI_FUNC(const char *) _PyIR_HandlerName(int handler);\n")


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
        import rop_generator
        generate_ir_metadata(args.input, data, outfile,
                             rop_generator.load_arm_entries())
