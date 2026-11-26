"""Generate the translator's rows of the stack ops:
Include/internal/pycore_regcfg_metadata.h.

One _PyStackFormat (Include/internal/pycore_regcfg.h) per stack op of
stack_ops.py, by stack number, and one _PyStack_Lowering: the register family
that the op lowers to, and the handler of that family's displacing arm if it
has one.  The translator (Python/regcfg.c, Python/ir_assemble.c) includes the
header; it derives an arm's row from its word format instead
(_PyRegCfg_ArmFormat).
"""

import argparse
from typing import TextIO

from cwriter import CWriter
from generators_common import ROOT, write_header
import ir_arms
from handler_table import first_arm
from stack_ops import LOWERS_TO, RULE_FAMILIES, STACK_OPS, StackFormat, StackOp

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


def check_families(arms: list[ir_arms.ArmEntry]) -> list[str]:
    """Every family is reached by a stack op or declared a rule family, and
    by one of the two only"""
    families = {arm.dotted.split(".")[0] for arm in arms}
    reached = {op.lowers_to for op in STACK_OPS if op.lowers_to}
    problems = [f"{name}: no stack op lowers to it and it is not a rule family"
                for name in sorted(families - reached - set(RULE_FAMILIES))]
    problems += [f"{name}: a stack op lowers to it and it is also a rule family"
                 for name in sorted(reached & set(RULE_FAMILIES))]
    problems += [f"{name}: is no family of Python/register_ops"
                 for name in sorted((reached | set(RULE_FAMILIES)) - families)]
    stack_names = {op.name for op in STACK_OPS}
    problems += [f"{name}: its stack op {op} is unknown"
                 for name, (ops, _) in RULE_FAMILIES.items()
                 for op in ops if op not in stack_names]
    return problems


def lowering_row(op: StackOp, canonical: set[str], handlers: dict[str, int]) -> str:
    if op.lowers_to is None:
        return "{ _PyStack_NO_FAMILY, _PyIR_NO_HANDLER, 0 }"
    handler = handlers.get(op.lowers_to, "_PyIR_NO_HANDLER")
    return (f"{{ IR_FAMILY_{op.lowers_to}, {handler}, "
            f"{int(op.name in canonical)} }}")


def generate_regcfg_metadata(
    filenames: list[str], outfile: TextIO, arms: list[ir_arms.ArmEntry]
) -> None:
    problems = check_families(arms)
    if problems:
        raise ValueError("\n".join(problems))
    # A displacing arm is the handler of its family's one op: the
    # instrumented op that monitoring puts in a word
    handlers = {arm.dotted.split(".")[0]: first_arm() + arm.index
                for arm in arms if arm.displacing}
    write_header(__file__, filenames, outfile)
    out = CWriter(outfile, 0, False)
    with out.header_guard("Py_CORE_REGCFG_METADATA_H"):
        out.emit("#ifndef Py_BUILD_CORE\n")
        out.emit('#  error "this header requires Py_BUILD_CORE define"\n')
        out.emit("#endif\n\n")
        out.emit('#include "pycore_ir_metadata.h"       // IR_FAMILY_*\n#include "pycore_regcfg.h"         // _PyStackFormat\n\n')
        out.emit("static const _PyStackFormat _PyStack_Formats[256] = {\n")
        for op in sorted(STACK_OPS, key=lambda op: op.opcode):
            assert op.format is not None, op.name
            out.emit(f"[{op.opcode}] = {stack_format_row(op.format)},  /* {op.name} */\n")
        out.emit("};\n\n")
        out.emit("/* The register family that a stack op lowers to, or _PyStack_NO_FAMILY,\n"
                 " * and the handler of the family's displacing arm, or _PyIR_NO_HANDLER */\n")
        out.emit("static const _PyStackLowering _PyStack_Lowering[256] = {\n")
        by_number = {op.opcode: op for op in STACK_OPS}
        # The first op that LOWERS_TO lists for a family stands for it
        canonical = {}
        for name, family in LOWERS_TO.items():
            canonical.setdefault(family, name)
        for number in range(256):
            op = by_number.get(number)
            if op is None:
                out.emit(f"[{number}] = {{ _PyStack_NO_FAMILY, _PyIR_NO_HANDLER, 0 }},\n")
                continue
            out.emit(f"[{number}] = {lowering_row(op, set(canonical.values()), handlers)},"
                     f"  /* {op.name} */\n")
        out.emit("};\n\n")
        out.emit("/* The families that a translator rule picks, for the stack op that it "
                 "reads */\n")
        out.emit("static const _PyStackRuleFamily _PyStack_RuleFamilies[] = {\n")
        numbers = {op.name: op.opcode for op in STACK_OPS}
        for family, (ops, _) in sorted(RULE_FAMILIES.items()):
            for op in ops:
                out.emit(f"{{ IR_FAMILY_{family}, {numbers[op]} }},  /* {op} */\n")
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
        import rop_generator
        generate_regcfg_metadata(args.input, outfile, rop_generator.load_arm_entries())
