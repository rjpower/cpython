"""Generate the metadata of the register IR: Include/internal/pycore_ir_metadata.h.

The register instruction arms (Python/register_ops) follow the word kinds in
every per-handler table, then the observed wrappers of the arms of families
that declare events: rop_generator.arm_entries() lists them, and
IR_NUM_HANDLERS counts them.  The stack ops (stack_ops.py) have a row of the
translator's format in _PyIR_StackOnlyFormats and no handler.

The tables are defined where NEED_IR_METADATA is defined (Python/ir.c) and
declared elsewhere.  Handler ids without an instruction have a zeroed row,
without _PyIR_VALID.
"""

import argparse
from collections.abc import Sequence
from typing import TextIO

import ir_arms
from cwriter import CWriter
from generators_common import ROOT, write_header
from handler_table import first_arm, num_handlers
from stack_ops import PSEUDOS, STACK_OPS, StackOp

DEFAULT_OUTPUT = ROOT / "Include/internal/pycore_ir_metadata.h"
STACK_NAMES = {op.name for op in STACK_OPS} | {p.name for p in PSEUDOS}
STACK_NUMBERS = {op.name: op.opcode for op in STACK_OPS}

# The role flags that a word of a call family may carry: a handler of the
# family reads them from its word, so specialisation and instrumentation keep
# them by keeping the flags.
CALL_ROLE_FLAGS = {
    "CALL": ("IR_F_NO_SELF", "IR_F_UNINTERRUPTED"),
    "CALL_KW": ("IR_F_NO_SELF", "IR_F_KW"),
    "CALL_FUNCTION_EX": ("IR_F_NO_KW", "IR_F_EX"),
}

# The row of the word kind that holds operands: no instruction, inert
OPERANDS_ROW = (
    "{ .flags = _PyIR_VALID, .effects = 0, .roles = { _PyIR_ROLE_NONE, _PyIR_ROLE_NONE, "
    "_PyIR_ROLE_NONE, _PyIR_ROLE_NONE }, .imm = _PyIR_LOC_NONE, .fb = _PyIR_LOC_NONE, "
    ".ninputs = 0, .dst = _PyIR_NO_DST, .borrow_mask = 0x0, .fb_kind = 0, .fb_size = 0, "
    ".result_phase = _PyIR_RESULT_PHASE_NONE, .output_kind = _PyIR_OUTPUT_REGISTER_DST, "
    ".transfer_kind = _PyIR_TRANSFER_UNKNOWN, .owner_program = 0, .owner_events = 0, "
    ".word_flags = 0, .error_routes = 0, .operands = _PyIR_OPERANDS_NONE, "
    ".observer_outcome = { _PyIR_OBSERVER_NONE, _PyIR_OBSERVER_NONE }, "
    ".reads = { 0, _PyIR_EXT_FIXED }, .writes = { 0, _PyIR_EXT_FIXED }, "
    ".through = { 0, _PyIR_EXT_FIXED }, .shadow = _PyIR_NO_OPCODE, "
    ".twin = _PyIR_NO_HANDLER, .events = 0 }")
OPERANDS_FLAGS = "_PyIR_H_METADATA | _PyIR_H_DISPATCH | _PyIR_H_IR_ONLY | _PyIR_H_PRISTINE"


def bits(names: list[str]) -> str:
    return " | ".join(names) if names else "0"


def extent_row(extent: tuple[int, str]) -> str:
    k, form = extent
    return f"{{ {k}, _PyIR_EXT_{form} }}"


def stack_format_row(op: StackOp) -> str:
    """The row of a stack op that has no handler: the translator's format
    (stack_ops.StackFormat) in the layout of _PyIRFormat"""
    fmt = op.format
    assert fmt is not None, op.name
    roles = ", ".join("_PyIR_ROLE_" + role.replace("+", " + ") for role in fmt.roles)
    dst = "_PyIR_NO_DST" if fmt.dst is None else str(fmt.dst)
    return (
        f"{{ .flags = {bits(['_PyIR_' + f for f in fmt.flags])}, "
        f".effects = {bits(['_PyIR_' + e for e in fmt.effects])}, "
        f".roles = {{ {roles} }}, .imm = _PyIR_LOC_{fmt.imm}, "
        f".fb = _PyIR_LOC_NONE, .ninputs = {fmt.ninputs}, .dst = {dst}, "
        f".borrow_mask = {fmt.borrow_mask:#x}, "
        f".fb_kind = 0, .fb_size = 0, "
        f".result_phase = _PyIR_RESULT_PHASE_NONE, "
        f".output_kind = _PyIR_OUTPUT_{fmt.output_kind}, "
        f".transfer_kind = _PyIR_TRANSFER_UNKNOWN, "
        f".owner_program = 0, .owner_events = 0, "
        f".word_flags = 0, .error_routes = 0, "
        f".operands = _PyIR_OPERANDS_{fmt.operands}, "
        f".observer_outcome = {{ {', '.join('_PyIR_OBSERVER_' + o for o in fmt.observer)} }}, "
        f".reads = {extent_row(fmt.reads)}, "
        f".writes = {extent_row(fmt.writes)}, "
        f".through = {extent_row(fmt.through)}, "
        f".shadow = {op.name}, "
        f".twin = _PyIR_NO_HANDLER, "
        f".events = 0 }}"
    )


def arm_id(arm: ir_arms.ArmEntry) -> int:
    return first_arm() + arm.index


def base_opcode(name: str) -> int:
    "The opcode that an instrumented op stands for, or the op's own"
    if name.startswith("INSTRUMENTED_") and name[len("INSTRUMENTED_"):] in STACK_NUMBERS:
        return STACK_NUMBERS[name[len("INSTRUMENTED_"):]]
    return STACK_NUMBERS[name]


def twin_by_tools(arm: ir_arms.ArmEntry) -> bool:
    """Whether monitoring selects the arm's observed twin by handler: its
    family declares events, but upstream's pinned table has no
    INSTRUMENTED_ stack opcode of its base to rewrite the opcode to"""
    return bool(arm.events) and f"INSTRUMENTED_{arm.base}" not in STACK_NAMES


def arm_flags(arm: ir_arms.ArmEntry) -> str:
    parts = ["_PyIR_H_METADATA"]
    if arm.emitted:
        parts.append("_PyIR_H_DISPATCH")
        if not arm.trailing:
            parts.append("_PyIR_H_SAVES_IP")
    if arm.adaptive and not arm.is_generic:
        parts.append("_PyIR_H_SPECIALIZED")
    if arm.observes is not None or arm.displacing:
        parts.append("_PyIR_H_INSTRUMENTED")
    if twin_by_tools(arm):
        parts.append("_PyIR_H_TWIN_BY_TOOLS")
    if arm.trailing:
        parts.append("_PyIR_H_TRAILING")
    parts.append("_PyIR_H_IR_ONLY")
    return " | ".join(parts)


def arm_owner_program(fmt: ir_arms.ArmFormat) -> tuple[tuple, ...]:
    """An arm's owner program: the leaf of its derived transfer, as an old
    handler with a checked leaf contract has (none for UNKNOWN)"""
    if fmt.transfer == "_PyIR_TRANSFER_UNKNOWN":
        return ()
    return (("_PyIR_OWNER_LEAF", 0, 0, fmt.transfer, 0),)


def arm_row(arm: ir_arms.ArmEntry,
            program: tuple[int, int] = (0, 0)) -> str:
    """The _PyIRFormat row of an arm, or of its observed wrapper; program
    is its owner program's (start, length) in the owner table"""
    fmt = arm.format
    assert fmt is not None
    own = arm_id(arm)
    if arm.observes is not None:
        twin = own
    else:
        twin = ("_PyIR_NO_HANDLER" if arm.twin is None
                else first_arm() + arm.twin)
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
    if arm.observes is not None and f"INSTRUMENTED_{arm.base}" in STACK_NAMES:
        shadow = f"INSTRUMENTED_{arm.base}"
    # An arm of a call family reads the call-role flags of its word
    word_flags = bits(list(CALL_ROLE_FLAGS.get(arm.base, ())))
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
    filenames: list[str], outfile: TextIO, arms: Sequence[ir_arms.ArmEntry]
) -> None:
    programs: dict[tuple, tuple[int, int]] = {}
    ownership: list[tuple] = []
    for arm in arms:
        assert arm.format is not None
        key = arm_owner_program(arm.format)
        if key not in programs:
            programs[key] = len(ownership), len(key)
            ownership.extend(key)
    n = num_handlers(arms)
    for arm in arms:
        if arm.base not in STACK_NUMBERS:
            raise ValueError(f"{arm.dotted}: ops({arm.base}) is not a stack op")
    write_header(__file__, filenames, outfile)
    out = CWriter(outfile, 0, False)
    with out.header_guard("Py_CORE_IR_METADATA_H"):
        out.emit("#ifndef Py_BUILD_CORE\n")
        out.emit('#  error "this header requires Py_BUILD_CORE define"\n')
        out.emit("#endif\n\n")
        out.emit("#define _PyIR_OBSERVER_ENTRY_ABSENCE 0\n\n")
        count = max(len(ownership), 1)
        out.emit(f"PyAPI_DATA(const _PyIROwnershipEvent) _PyIR_OwnerPrograms[{count}];\n")
        out.emit("#ifdef NEED_IR_METADATA\n")
        out.emit(f"const _PyIROwnershipEvent _PyIR_OwnerPrograms[{count}] = {{\n")
        for kind, flags, dst, src, extent in ownership:
            out.emit(f"{{ {kind}, {flags}, {dst}, {src}, {extent} }},\n")
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
        for table in ("_PyIR_HandlerFlags", "_PyIR_HandlerBase"):
            out.emit(f"PyAPI_DATA(const uint16_t) {table}[IR_NUM_HANDLERS];\n")
            out.emit("#ifdef NEED_IR_METADATA\n")
            out.emit(f"const uint16_t {table}[IR_NUM_HANDLERS] = {{\n")
            value = OPERANDS_FLAGS if table == "_PyIR_HandlerFlags" else "IR_HANDLER_ONLY_BASE(IR_H_IR_OPERANDS)"
            out.emit(f"[IR_H_IR_OPERANDS] = {value}, /* IR_OPERANDS */\n")
            for arm in arms:
                value = (arm_flags(arm) if table == "_PyIR_HandlerFlags"
                         else str(STACK_NUMBERS[arm.base]))
                out.emit(f"[{arm_id(arm)}] = {value}, /* {arm.dotted} */\n")
            out.emit("};\n#endif\n\n")

        out.emit(f"PyAPI_DATA(const _PyIRFormat) _PyIR_Formats[{n}];\n")
        out.emit("#ifdef NEED_IR_METADATA\n")
        out.emit(f"const _PyIRFormat _PyIR_Formats[{n}] = {{\n")
        out.emit(f"[IR_H_IR_OPERANDS] = {OPERANDS_ROW},  /* IR_OPERANDS */\n")
        for arm in arms:
            assert arm.format is not None
            program = programs[arm_owner_program(arm.format)]
            out.emit(f"[{arm_id(arm)}] = {arm_row(arm, program)},  /* {arm.dotted} */\n")
        out.emit("};\n")
        out.emit("#endif\n\n")

        # The CFG reads the format of a stack op by its stack number
        # (_PyRegCfg_FormatOf), or by its handler when a displacing arm is
        # one (_PyIR_OpcodeHandler)
        out.emit("PyAPI_DATA(const _PyIRFormat) _PyIR_StackOnlyFormats[256];\n")
        out.emit("#ifdef NEED_IR_METADATA\n")
        out.emit("const _PyIRFormat _PyIR_StackOnlyFormats[256] = {\n")
        for op in sorted(STACK_OPS, key=lambda op: op.opcode):
            out.emit(f"[{op.opcode}] = {stack_format_row(op)},  /* {op.name} */\n")
        out.emit("};\n")
        out.emit("#endif\n\n")

        none = "_PyIR_NO_HANDLER"
        out.emit("/* The stack opcode that a handler is, or _PyIR_NO_OPCODE; the "
                 "handler of a\n * stack opcode, or _PyIR_NO_HANDLER */\n")
        out.emit("PyAPI_DATA(const uint16_t) _PyIR_HandlerOpcode[IR_NUM_HANDLERS];\n")
        out.emit("PyAPI_DATA(const uint16_t) _PyIR_OpcodeHandler[256];\n")
        out.emit("PyAPI_DATA(const uint16_t) _PyIR_OpcodeBase[256];\n")
        out.emit("#ifdef NEED_IR_METADATA\n")
        out.emit("const uint16_t _PyIR_HandlerOpcode[IR_NUM_HANDLERS] = {\n")
        for i in range(n):
            out.emit(f"[{i}] = _PyIR_NO_OPCODE,\n")
        out.emit("};\n")
        out.emit("const uint16_t _PyIR_OpcodeBase[256] = {\n")
        by_number = {op.opcode: op.name for op in STACK_OPS}
        for number in range(256):
            name = by_number.get(number)
            base = "_PyIR_NO_OPCODE" if name is None else base_opcode(name)
            out.emit(f"[{number}] = {base},\n")
        out.emit("};\n")
        out.emit("const uint16_t _PyIR_OpcodeHandler[256] = {\n")
        for number in range(256):
            # A displacing arm is the handler of its family's one op, the
            # instrumented opcode that monitoring puts in a word
            handler = next((arm_id(arm) for arm in arms
                            if arm.displacing and STACK_NUMBERS.get(arm.base) == number),
                           None)
            out.emit(f"[{number}] = {none if handler is None else handler},\n")
        out.emit("};\n")
        out.emit("#endif\n\n")

        # The statistics of specialisation are kept per family (the dotted
        # name's head) and, in opcode_stats, per arm.  IR_OPERANDS has the
        # family of its own name.
        families = sorted({"IR_OPERANDS"} | {arm.dotted.split(".")[0] for arm in arms})
        out.emit("/* The families, in name order: the index of family_stats */\nenum {\n")
        for number, family in enumerate(families):
            out.emit(f"    IR_FAMILY_{family} = {number},\n")
        out.emit(f"    IR_NUM_FAMILIES = {len(families)}\n}};\n")
        out.emit('static_assert(IR_NUM_FAMILIES <= 256, "family_stats is sized 256");\n\n')
        out.emit("PyAPI_DATA(const uint8_t) _PyIR_HandlerFamily[IR_NUM_HANDLERS];\n")
        out.emit("PyAPI_DATA(const char *const) _PyIR_FamilyNames[IR_NUM_FAMILIES];\n")
        out.emit("#ifdef NEED_IR_METADATA\n")
        out.emit("const uint8_t _PyIR_HandlerFamily[IR_NUM_HANDLERS] = {\n")
        out.emit("[IR_H_IR_OPERANDS] = IR_FAMILY_IR_OPERANDS,\n")
        for arm in arms:
            out.emit(f"[{arm_id(arm)}] = IR_FAMILY_{arm.dotted.split('.')[0]}, /* {arm.dotted} */\n")
        out.emit("};\n")
        out.emit("const char *const _PyIR_FamilyNames[IR_NUM_FAMILIES] = {\n")
        for family in families:
            out.emit(f'[IR_FAMILY_{family}] = "{family}",\n')
        out.emit("};\n#endif\n\n")

        out.emit("PyAPI_DATA(const char *const) _PyIR_OnlyNames[IR_NUM_HANDLERS];\n")
        out.emit("#ifdef NEED_IR_METADATA\n")
        out.emit("const char *const _PyIR_OnlyNames[IR_NUM_HANDLERS] = {\n")
        out.emit('[IR_H_IR_OPERANDS] = "IR_OPERANDS",\n')
        for arm in arms:
            out.emit(f'[{arm_id(arm)}] = "{arm.dotted}",\n')
        out.emit("};\n")
        out.emit("#endif\n\n")
        out.emit("/* The word kinds and the arms have a name here; the "
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
    "input", nargs=argparse.REMAINDER, help="Source files, named in the header"
)

if __name__ == "__main__":
    args = arg_parser.parse_args()
    if len(args.input) == 0:
        args.input.append(str(ROOT / "Python/register_ops"))
    with open(args.output, "w") as outfile:
        import rop_generator
        generate_ir_metadata(args.input, outfile,
                             rop_generator.load_arm_entries())
