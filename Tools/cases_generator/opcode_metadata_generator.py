"""Generate opcode metadata.
Reads the stack ops from stack_ops.py.
Writes the metadata to pycore_opcode_metadata.h by default.
"""

import argparse

from generators_common import ROOT, write_header
from cwriter import CWriter
from stack_opcode_pins import MIN_SPECIALIZED_OPCODE
from stack_ops import (HAVE_ARGUMENT, PSEUDOS, STACK_OPS, Pseudo, StackOp)
from typing import TextIO

FLAGS = [
    "ARG",
    "CONST",
    "NAME",
    "JUMP",
    "FREE",
    "LOCAL",
    "EVAL_BREAK",
]


def flag_text(flags: tuple[str, ...]) -> str:
    return " | ".join(f"HAS_{flag}_FLAG" for flag in flags) or "0"


def generate_flag_macros(out: CWriter) -> None:
    for i, flag in enumerate(FLAGS):
        out.emit(f"#define HAS_{flag}_FLAG ({1<<i})\n")
    for i, flag in enumerate(FLAGS):
        out.emit(
            f"#define OPCODE_HAS_{flag}(OP) (_PyOpcode_opcode_metadata[OP].flags & (HAS_{flag}_FLAG))\n"
        )
    out.emit("\n")


def emit_stack_effect_function(
    out: CWriter, direction: str, data: list[tuple[str, str]]
) -> None:
    out.emit(f"extern int _PyOpcode_num_{direction}(int opcode, int oparg);\n")
    out.emit("#ifdef NEED_OPCODE_METADATA\n")
    out.emit(f"int _PyOpcode_num_{direction}(int opcode, int oparg)  {{\n")
    out.emit("switch(opcode) {\n")
    for name, effect in data:
        out.emit(f"case {name}:\n")
        out.emit(f"    return {effect};\n")
    out.emit("default:\n")
    out.emit("    return -1;\n")
    out.emit("}\n")
    out.emit("}\n\n")
    out.emit("#endif\n\n")


def generate_stack_effect_functions(out: CWriter) -> None:
    popped_data = sorted((op.name, op.popped) for op in (*STACK_OPS, *PSEUDOS))
    pushed_data = sorted((op.name, op.pushed) for op in (*STACK_OPS, *PSEUDOS))
    emit_stack_effect_function(out, "popped", popped_data)
    emit_stack_effect_function(out, "pushed", pushed_data)


def generate_is_pseudo(out: CWriter) -> None:
    """Write the IS_PSEUDO_INSTR macro"""
    out.emit("\n\n#define IS_PSEUDO_INSTR(OP)  ( \\\n")
    for pseudo in PSEUDOS:
        out.emit(f"((OP) == {pseudo.name}) || \\\n")
    out.emit("0")
    out.emit(")\n\n")


def has_oparg(inst: StackOp) -> bool:
    """Whether the stack op takes an oparg.  A base op's number says so, as
    dis and the compiler read it, whatever the body of its stub does."""
    if inst.opcode < MIN_SPECIALIZED_OPCODE:
        return inst.opcode >= HAVE_ARGUMENT
    return inst.oparg


def get_format(inst: StackOp) -> str:
    if has_oparg(inst):
        format = "INSTR_FMT_IB"
    else:
        format = "INSTR_FMT_IX"
    if inst.size > 1:
        format += "C"
    format += "0" * (inst.size - 2)
    return format


def generate_instruction_formats(out: CWriter) -> None:
    # Compute the set of all instruction formats.
    formats: set[str] = set()
    for inst in STACK_OPS:
        formats.add(get_format(inst))
    # Generate an enum for it
    out.emit("enum InstructionFormat {\n")
    next_id = 1
    for format in sorted(formats):
        out.emit(f"{format} = {next_id},\n")
        next_id += 1
    out.emit("};\n\n")


def generate_deopt_table(out: CWriter) -> None:
    out.emit("PyAPI_DATA(const uint8_t) _PyOpcode_Deopt[256];\n")
    out.emit("#ifdef NEED_OPCODE_METADATA\n")
    out.emit("const uint8_t _PyOpcode_Deopt[256] = {\n")
    deopts: list[tuple[str, str]] = []
    for inst in STACK_OPS:
        deopt = inst.family or inst.name
        deopts.append((inst.name, deopt))
    defined = {inst.opcode for inst in STACK_OPS}
    for i in range(256):
        if i not in defined:
            deopts.append((f'{i}', f'{i}'))

    assert len(deopts) == 256
    assert len(set(x[0] for x in deopts)) == 256
    for name, deopt in sorted(deopts):
        out.emit(f"[{name}] = {deopt},\n")
    out.emit("};\n\n")
    out.emit("#endif // NEED_OPCODE_METADATA\n\n")


def generate_cache_table(out: CWriter) -> None:
    out.emit("PyAPI_DATA(const uint8_t) _PyOpcode_Caches[256];\n")
    out.emit("#ifdef NEED_OPCODE_METADATA\n")
    out.emit("const uint8_t _PyOpcode_Caches[256] = {\n")
    for inst in STACK_OPS:
        if inst.family and inst.family != inst.name:
            continue
        if inst.name.startswith("INSTRUMENTED"):
            continue
        if inst.size > 1:
            out.emit(f"[{inst.name}] = {inst.size-1},\n")
    out.emit("};\n")
    out.emit("#endif\n\n")


def generate_name_table(out: CWriter) -> None:
    table_size = 256 + len(PSEUDOS)
    out.emit(f"PyAPI_DATA(const char) *_PyOpcode_OpName[{table_size}];\n")
    out.emit("#ifdef NEED_OPCODE_METADATA\n")
    out.emit(f"const char *_PyOpcode_OpName[{table_size}] = {{\n")
    names = [op.name for op in STACK_OPS] + [p.name for p in PSEUDOS]
    for name in sorted(names):
        out.emit(f'[{name}] = "{name}",\n')
    out.emit("};\n")
    out.emit("#endif\n\n")


def generate_metadata_table(out: CWriter) -> None:
    table_size = 256 + len(PSEUDOS)
    out.emit("struct opcode_metadata {\n")
    out.emit("uint8_t valid_entry;\n")
    out.emit("uint8_t instr_format;\n")
    out.emit("uint32_t flags;\n")
    out.emit("};\n\n")
    out.emit(
        f"PyAPI_DATA(const struct opcode_metadata) _PyOpcode_opcode_metadata[{table_size}];\n"
    )
    out.emit("#ifdef NEED_OPCODE_METADATA\n")
    out.emit(
        f"const struct opcode_metadata _PyOpcode_opcode_metadata[{table_size}] = {{\n"
    )
    for inst in sorted(STACK_OPS, key=lambda t: t.name):
        out.emit(
            f"[{inst.name}] = {{ true, {get_format(inst)}, {flag_text(inst.flags)} }},\n"
        )
    for pseudo in sorted(PSEUDOS, key=lambda t: t.name):
        out.emit(f"[{pseudo.name}] = {{ true, -1, {flag_text(pseudo.flags)} }},\n")
    out.emit("};\n")
    out.emit("#endif\n\n")


def generate_pseudo_targets(out: CWriter) -> None:
    table_size = len(PSEUDOS)
    max_targets = max(len(pseudo.targets) for pseudo in PSEUDOS)
    out.emit("struct pseudo_targets {\n")
    out.emit(f"uint8_t as_sequence;\n")
    out.emit(f"uint8_t targets[{max_targets + 1}];\n")
    out.emit("};\n")
    out.emit(
        f"extern const struct pseudo_targets _PyOpcode_PseudoTargets[{table_size}];\n"
    )
    out.emit("#ifdef NEED_OPCODE_METADATA\n")
    out.emit(
        f"const struct pseudo_targets _PyOpcode_PseudoTargets[{table_size}] = {{\n"
    )
    for pseudo in PSEUDOS:
        as_sequence = "1" if pseudo.as_sequence else "0"
        targets = ["0"] * (max_targets + 1)
        for i, target in enumerate(pseudo.targets):
            targets[i] = target
        out.emit(f"[{pseudo.name}-256] = {{ {as_sequence}, {{ {', '.join(targets)} }} }},\n")
    out.emit("};\n\n")
    out.emit("#endif // NEED_OPCODE_METADATA\n")
    out.emit("static inline bool\n")
    out.emit("is_pseudo_target(int pseudo, int target) {\n")
    out.emit(f"if (pseudo < 256 || pseudo >= {256+table_size}) {{\n")
    out.emit(f"return false;\n")
    out.emit("}\n")
    out.emit(
        f"for (int i = 0; _PyOpcode_PseudoTargets[pseudo-256].targets[i]; i++) {{\n"
    )
    out.emit(
        f"if (_PyOpcode_PseudoTargets[pseudo-256].targets[i] == target) return true;\n"
    )
    out.emit("}\n")
    out.emit(f"return false;\n")
    out.emit("}\n\n")


def generate_opcode_metadata(filenames: list[str], outfile: TextIO) -> None:
    write_header(__file__, filenames, outfile)
    out = CWriter(outfile, 0, False)
    with out.header_guard("Py_CORE_OPCODE_METADATA_H"):
        out.emit("#ifndef Py_BUILD_CORE\n")
        out.emit('#  error "this header requires Py_BUILD_CORE define"\n')
        out.emit("#endif\n\n")
        out.emit("#include <stdbool.h>              // bool\n")
        out.emit('#include "opcode_ids.h"\n')
        generate_is_pseudo(out)
        generate_stack_effect_functions(out)
        generate_instruction_formats(out)
        table_size = 256 + len(PSEUDOS)
        out.emit("#define IS_VALID_OPCODE(OP) \\\n")
        out.emit(f"    (((OP) >= 0) && ((OP) < {table_size}) && \\\n")
        out.emit("     (_PyOpcode_opcode_metadata[(OP)].valid_entry))\n\n")
        # co_code holds stack bytecode: the translator-only ops, which
        # have table entries for the CFG, are not among its opcodes
        out.emit("#define IS_STACK_OPCODE(OP) \\\n")
        out.emit("    (IS_VALID_OPCODE(OP)")
        for op in STACK_OPS:
            if op.translator_only:
                out.emit(f" && (OP) != {op.name}")
        out.emit(")\n\n")
        generate_flag_macros(out)
        generate_metadata_table(out)
        generate_name_table(out)
        generate_cache_table(out)
        generate_deopt_table(out)
        generate_pseudo_targets(out)


arg_parser = argparse.ArgumentParser(
    description="Generate the header file with opcode metadata.",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
)


DEFAULT_OUTPUT = ROOT / "Include/internal/pycore_opcode_metadata.h"


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
        generate_opcode_metadata(args.input, outfile)
