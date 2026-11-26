"""Generate the metadata of the register IR: Include/internal/pycore_ir_metadata.h.

The register instruction arms (Python/register_ops) follow the word kinds in
every per-handler table, then the observed wrappers of the arms of families
that declare events: rop_generator.arm_entries() lists them, and
IR_NUM_HANDLERS counts them.  The translator reads a stack op's row
(stack_ops.py) in _PyStack_Formats and an arm's in _PyStack_ArmFormats, both
of type _PyStackFormat (arm_stack_format).

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
from stack_ops import PSEUDOS, STACK_OPS, StackFormat, StackOp

DEFAULT_OUTPUT = ROOT / "Include/internal/pycore_ir_metadata.h"
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
    ".fb_kind = 0, .fb_size = 0, .transfer_kind = _PyIR_TRANSFER_UNKNOWN, "
    ".word_flags = 0, .operands = _PyIR_OPERANDS_NONE, "
    ".twin = _PyIR_NO_HANDLER, .events = 0 }")
OPERANDS_FLAGS = "_PyIR_H_METADATA | _PyIR_H_DISPATCH | _PyIR_H_IR_ONLY | _PyIR_H_PRISTINE"


def bits(names: list[str]) -> str:
    return " | ".join(names) if names else "0"


def extent_row(extent: tuple[int, str]) -> str:
    k, form = extent
    return f"{{ {k}, _PyIR_EXT_{form} }}"


def stack_format_row(fmt: StackFormat) -> str:
    "A _PyStackFormat row"
    roles = ", ".join("_PyIR_ROLE_" + role.replace("+", " + ") for role in fmt.roles)
    dst = "_PyIR_NO_DST" if fmt.dst is None else str(fmt.dst)
    return (
        f"{{ .flags = {bits(['_PyIR_' + f for f in fmt.flags])}, "
        f".effects = {bits(['_PyIR_' + e for e in fmt.effects])}, "
        f".roles = {{ {roles} }}, .imm = _PyIR_LOC_{fmt.imm}, "
        f".ninputs = {fmt.ninputs}, .dst = {dst}, "
        f".borrow_mask = {fmt.borrow_mask:#x}, "
        f".operands = _PyIR_OPERANDS_{fmt.operands}, "
        f".through = {extent_row(fmt.through)} }}"
    )


def _strip(prefix: str, name: str) -> str:
    assert name.startswith(prefix), name
    return name[len(prefix):]


def arm_stack_format(fmt: ir_arms.ArmFormat) -> StackFormat:
    """The translator's row of an arm, from its word format.  A window (a
    BASE role: ForIter, Send, CleanupThrow) reads its home group from the
    base and its other inputs from IN fields; its result is part 2 of the
    group, and part 0 passes through where the family's edge 0 keeps it.
    Other arms read an input per IN field, or the parts of the home group
    that an IN field names, and write dst above the inputs they pop.  A
    destination of its own may name a local or the discard slot; an out
    group (IterOpen's pair) is a result but not such a destination."""
    window = "_PyIR_ROLE_BASE" in fmt.roles
    ninputs = 0
    has_dst = out_group = False
    for f, role in enumerate(fmt.roles):
        grouped = bool(fmt.group) and f == fmt.group_at
        if role.startswith("_PyIR_ROLE_IN0"):
            ninputs += fmt.group if grouped else 1
        elif role == "_PyIR_ROLE_DST":
            out_group |= grouped
            has_dst |= not grouped
    through = 0
    if window:
        dst: int | None = 2
        kept = (fmt.edges >> (2 * fmt.group_at)) & 3 == 0   # _PyIR_HOME_PRESERVE
        through = 1 if fmt.normal & 1 and kept else 0
    else:
        dst = -ninputs if has_dst or out_group else None
    flags = [_strip("_PyIR_", f) for f in fmt.flags]
    if window:
        flags.append("WINDOW")
    if window or has_dst:
        flags += ["STORE_DST", "DISCARDS"]
    return StackFormat(
        tuple(flags), tuple(_strip("_PyIR_", e) for e in fmt.effects),
        tuple(_strip("_PyIR_ROLE_", r).replace(" ", "") for r in fmt.roles),
        _strip("_PyIR_LOC_", fmt.imm), ninputs, dst, (1 << ninputs) - 1,
        _strip("_PyIR_OPERANDS_", fmt.operands), (through, "FIXED"))


def arm_id(arm: ir_arms.ArmEntry) -> int:
    return first_arm() + arm.index


# The events that a word of an instrumented family can carry in tools[]
# (sys.monitoring's grouped events); C_RETURN and C_RAISE are fired by CALL
# and have no tools of their own
LOCAL_EVENTS = ("PY_START", "PY_RESUME", "PY_RETURN", "PY_YIELD", "CALL", "JUMP",
                "BRANCH_RIGHT", "STOP_ITERATION")


def monitor_events(arm: ir_arms.ArmEntry) -> tuple[str | None, bool, bool]:
    """The (primary event, has BRANCH_LEFT, has a separate STOP_ITERATION
    slot) of an arm, derived from its family's events(...) by one rule:
    BRANCH_LEFT is the left flag; the one local event that remains is the
    primary.  The pair PY_START, PY_RESUME is one primary, whose event the
    word's operand picks.  A STOP_ITERATION beside another primary has its own
    slot (FOR_ITER's completion).  Any other combination is refused."""
    left = "BRANCH_LEFT" in arm.events
    rest = [e for e in arm.events if e in LOCAL_EVENTS]
    stop = False
    if len(rest) == 2 and rest[1] == "STOP_ITERATION" and rest[0] != rest[1]:
        stop = True
        rest = rest[:1]
    elif rest == ["PY_START", "PY_RESUME"]:
        rest = rest[:1]
    if len(rest) > 1:
        raise ValueError(f"{arm.dotted}: events({', '.join(arm.events)}) "
                         "have more than one primary event")
    if rest == [] and (left or stop):
        raise ValueError(f"{arm.dotted}: events({', '.join(arm.events)}) "
                         "have no primary event")
    return (rest[0] if rest else None), left, stop


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
    if arm.trailing:
        parts.append("_PyIR_H_TRAILING")
    _, left, stop = monitor_events(arm)
    if left:
        parts.append("_PyIR_H_BRANCH_LEFT")
    if stop:
        parts.append("_PyIR_H_STOP_SLOT")
    parts.append("_PyIR_H_IR_ONLY")
    return " | ".join(parts)


def arm_row(arm: ir_arms.ArmEntry) -> str:
    "The _PyIRFormat row of an arm, or of its observed wrapper"
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
    # An arm of a call family reads the call-role flags of its word
    word_flags = bits(list(CALL_ROLE_FLAGS.get(arm.base, ())))
    return (
        f"{{ .flags = {bits(list(fmt.flags))}, .effects = {bits(list(fmt.effects))}, "
        f".roles = {{ {', '.join(fmt.roles)} }}, .imm = {fmt.imm}, "
        f".fb = {fmt.fb}, .target = {fmt.target}, .fb_kind = {fb_kind}, "
        f".fb_size = {fb_size}, .transfer_kind = {fmt.transfer}, "
        f".word_flags = {word_flags}, .operands = {fmt.operands}, {fixed}"
        f".twin = {twin}, .events = {events}, .group = {fmt.group}, "
        f".group_at = {fmt.group_at}, .normal = {fmt.normal:#x}, .vacant = {fmt.vacant:#x}, "
        f".committed = {fmt.committed:#x}, .edges = {fmt.edges:#x} }}")


def generate_ir_metadata(
    filenames: list[str], outfile: TextIO, arms: Sequence[ir_arms.ArmEntry]
) -> None:
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
        for kind, bit in (("SPECIALIZED", 1), ("INSTRUMENTED", 2),
                          ("IR_ONLY", 8), ("NOFB", 16),
                          ("METADATA", 64), ("DISPATCH", 128),
                          ("PRISTINE", 256), ("ADAPTIVE", 512),
                          ("SAVES_IP", 1024),
                          ("TRAILING", 8192),
                          ("BRANCH_LEFT", 16384), ("STOP_SLOT", 32768)):
            out.emit(f"#define _PyIR_H_{kind} {bit}\n")
        out.emit("\n")
        out.emit("PyAPI_DATA(const uint16_t) _PyIR_HandlerFlags[IR_NUM_HANDLERS];\n")
        out.emit("#ifdef NEED_IR_METADATA\n")
        out.emit("const uint16_t _PyIR_HandlerFlags[IR_NUM_HANDLERS] = {\n")
        out.emit(f"[IR_H_IR_OPERANDS] = {OPERANDS_FLAGS}, /* IR_OPERANDS */\n")
        for arm in arms:
            out.emit(f"[{arm_id(arm)}] = {arm_flags(arm)}, /* {arm.dotted} */\n")
        out.emit("};\n#endif\n\n")

        out.emit(f"PyAPI_DATA(const _PyIRFormat) _PyIR_Formats[{n}];\n")
        out.emit("#ifdef NEED_IR_METADATA\n")
        out.emit(f"const _PyIRFormat _PyIR_Formats[{n}] = {{\n")
        out.emit(f"[IR_H_IR_OPERANDS] = {OPERANDS_ROW},  /* IR_OPERANDS */\n")
        for arm in arms:
            assert arm.format is not None
            out.emit(f"[{arm_id(arm)}] = {arm_row(arm)},  /* {arm.dotted} */\n")
        out.emit("};\n")
        out.emit("#endif\n\n")

        # The translator reads the row of a stack op by its stack number, and
        # of an arm by its handler (_PyRegCfg_FormatOf)
        out.emit("PyAPI_DATA(const _PyStackFormat) _PyStack_Formats[256];\n")
        out.emit(f"PyAPI_DATA(const _PyStackFormat) _PyStack_ArmFormats[{n}];\n")
        out.emit("#ifdef NEED_IR_METADATA\n")
        out.emit("const _PyStackFormat _PyStack_Formats[256] = {\n")
        for op in sorted(STACK_OPS, key=lambda op: op.opcode):
            assert op.format is not None, op.name
            out.emit(f"[{op.opcode}] = {stack_format_row(op.format)},  /* {op.name} */\n")
        out.emit("};\n")
        out.emit(f"const _PyStackFormat _PyStack_ArmFormats[{n}] = {{\n")
        for arm in arms:
            assert arm.format is not None
            row = stack_format_row(arm_stack_format(arm.format))
            out.emit(f"[{arm_id(arm)}] = {row},  /* {arm.dotted} */\n")
        out.emit("};\n")
        out.emit("#endif\n\n")

        none = "_PyIR_NO_HANDLER"
        out.emit("/* The handler of a stack opcode, or _PyIR_NO_HANDLER */\n")
        out.emit("PyAPI_DATA(const uint16_t) _PyIR_OpcodeHandler[256];\n")
        out.emit("#ifdef NEED_IR_METADATA\n")
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

        # The primary sys.monitoring event of a handler, or -1: the slot of
        # tools[] that its word's tools live in.  PY_START stands for a
        # Resume's pair, whose operand picks the event.
        out.emit("PyAPI_DATA(const int8_t) _PyIR_PrimaryEvent[IR_NUM_HANDLERS];\n")
        out.emit("#ifdef NEED_IR_METADATA\n")
        out.emit("const int8_t _PyIR_PrimaryEvent[IR_NUM_HANDLERS] = {\n")
        primaries = {arm_id(arm): (monitor_events(arm)[0], arm.dotted) for arm in arms}
        for handler in range(n):
            primary, name = primaries.get(handler, (None, "no event"))
            value = -1 if primary is None else "PY_MONITORING_EVENT_" + primary
            out.emit(f"[{handler}] = {value}, /* {name} */\n")
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
