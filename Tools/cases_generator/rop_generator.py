"""Generate the C products of the register instruction definitions (F2).

From the checked `Program` of Python/register_ops/*.rop:

- the interpreter cases (one computed-goto case per arm),
- the function forms, with the same signature and completion contract as
  the old `ir_run_*` forms,
- the dispatch table that adds the arms to the old one,
- the metadata: family and arm numbers, the arm-to-handler table, the
  point tables.

An arm whose graph needs a row or construct that is still pending is left
out of the code and listed, with what blocks it, at the head of each file.
Its handler number is allocated and its slot in the dispatch table stays
unknown, so the assembler cannot select it.
"""

import argparse
import re
import sys
from collections.abc import Mapping
from dataclasses import dataclass
from pathlib import Path
from typing import TextIO

import rop_check
from generators_common import root_relative_path, write_header
from rop_catalog import Slot
from rop_cgen import Lowering, Unsupported, lower_arm
from rop_check import Program
from rop_model import Arm

ROOT = Path(__file__).resolve().parents[2]
ROP_DIR = ROOT / "Python" / "register_ops"
NUMBERS = ROP_DIR / "handlers.txt"
OLD_TARGETS = ROOT / "Python" / "ir_targets.h"

# The handler ids of the arms follow the old handlers; the field holds nine
# bits
FIRST_HANDLER = 418
HANDLER_LIMIT = 512


class GenerateError(Exception):
    pass


def read_numbers(path: Path = NUMBERS) -> dict[str, int]:
    """The pinned handler number of each arm: `Family.Arm NUMBER` lines"""
    numbers: dict[str, int] = {}
    for n, line in enumerate(path.read_text().splitlines(), 1):
        line = line.split("#")[0].strip()
        if not line:
            continue
        name, number = line.split()
        if name in numbers:
            raise GenerateError(f"{path.name}:{n}: {name} appears twice")
        numbers[name] = int(number)
    seen: dict[int, str] = {}
    for name, number in numbers.items():
        if not FIRST_HANDLER <= number < HANDLER_LIMIT:
            raise GenerateError(
                f"{path.name}: {name} has number {number}, outside "
                f"{FIRST_HANDLER}..{HANDLER_LIMIT - 1}")
        if number in seen:
            raise GenerateError(f"{path.name}: {name} and {seen[number]} share {number}")
        seen[number] = name
    return numbers


def check_numbers(program: Program, numbers: Mapping[str, int]) -> None:
    for name in program.arms:
        if name not in numbers:
            raise GenerateError(
                f"handlers.txt has no line for arm {name}; add `{name} N`")
    for name in numbers:
        if name not in program.arms:
            raise GenerateError(f"handlers.txt names {name}, which is no arm")


def c_name(arm: str) -> str:
    return arm.replace(".", "_")


@dataclass(frozen=True)
class Products:
    program: Program
    numbers: Mapping[str, int]
    points: Mapping[str, int]
    lowered: Mapping[str, Lowering]
    left_out: Mapping[str, str]       # arm -> what blocks it


def point_numbers(program: Program) -> dict[str, int]:
    """Error checkpoint numbers, from 1, by point name"""
    return {name: i + 1 for i, name in enumerate(sorted(program.points))}


def lower_all(program: Program, numbers: Mapping[str, int]) -> Products:
    points = point_numbers(program)
    if len(points) > 255:
        raise GenerateError("more than 255 checkpoints: the entry has 8 bits")
    lowered: dict[str, Lowering] = {}
    left_out: dict[str, str] = {}
    # The family's miss adapter is its Generic arm, when that arm is emitted;
    # a Generic arm that does not lower leaves its family without one
    generic: dict[str, str | None] = {f: None for f in program.families}
    ordered = sorted(program.arms.values(), key=lambda a: a.name != f"{a.family}.Generic")
    for arm in ordered:
        try:
            lowered[arm.name] = lower_arm(program, arm, numbers, points, generic)
        except Unsupported as e:
            left_out[arm.name] = str(e)
            continue
        if arm.name.endswith(".Generic"):
            generic[arm.family] = c_name(arm.name)
    return Products(program, numbers, points, lowered, left_out)


# Interpreter cases and function forms

PROLOGUE = """\
int opcode = {number};
(void)opcode;
_PyIRWord *const this_instr = next_instr;
(void)this_instr;
frame->instr_ptr = next_instr;
IR_HOLD_NEXT();
next_instr += 1;
INSTRUCTION_STATS({number});"""


def handler_text(arm: Arm, number: int, low: Lowering, indent: str) -> list[str]:
    lines = [f"// {arm.name} ({arm.region.name})"]
    lines += PROLOGUE.format(number=number).splitlines()
    lines += low.decls
    lines += low.body
    return [indent + line if line else line for line in lines]


def indented(lines: list[str], by: str) -> list[str]:
    return [by + line for line in lines]


def write_left_out(out: TextIO, p: Products) -> None:
    out.write("//\n// Arms left out, and what blocks each:\n")
    for name, why in sorted(p.left_out.items()):
        out.write(f"//   {name}: {why}\n")
    if not p.left_out:
        out.write("//   none\n")
    out.write("\n")


def write_cases(out: TextIO, p: Products, sources: list[str]) -> None:
    write_header(__file__, sources, out)
    write_left_out(out, p)
    out.write(MISS_MACRO)
    for name, low in p.lowered.items():
        arm = p.program.arms[name]
        out.write(f"\n        TARGET({c_name(name)}) {{\n")
        text = handler_text(arm, p.numbers[name], low, "            ")
        out.write("\n".join(text) + "\n        }\n")


MISS_MACRO = """\
/* A family with no generic arm cannot miss: no word selects its arms yet. */
#ifndef ROP_NO_GENERIC
#define ROP_NO_GENERIC(FAMILY) \\
    Py_FatalError("register instruction family " #FAMILY " has no generic arm")
#endif
"""


def write_forms(out: TextIO, p: Products, sources: list[str]) -> None:
    write_header(__file__, sources, out)
    write_left_out(out, p)
    out.write(MISS_MACRO + "\n")
    for name in p.lowered:
        out.write(f"static _PyIRTransfer ir_run_{c_name(name)}(PyThreadState *tstate, "
                  "_PyInterpreterFrame *frame, _PyInstr *next_instr, "
                  "_PyIRWord ir_word);\n")
    for name, low in p.lowered.items():
        arm = p.program.arms[name]
        out.write(f"\nstatic Py_NO_INLINE _PyIRTransfer\nir_run_{c_name(name)}(\n"
                  "    PyThreadState *tstate, _PyInterpreterFrame *frame, "
                  "_PyInstr *next_instr, _PyIRWord ir_word)\n{\n")
        out.write("\n".join(handler_text(arm, p.numbers[name], low, "    ")) + "\n}\n")
    out.write("\nstatic const _PyIRForm ir_rop_forms[ROP_NUM_HANDLERS] = {\n")
    for name in p.lowered:
        out.write(f"    [{p.numbers[name] - FIRST_HANDLER}] = ir_run_{c_name(name)},\n")
    out.write("};\n")


# The dispatch table: the old table, with the arms' entries

TARGET_ENTRY = re.compile(r"^\s*\[(\d+)\] = &&TARGET_(\w+),$")


def old_dispatch(path: Path = OLD_TARGETS) -> dict[int, str]:
    inside = False
    table: dict[int, str] = {}
    for line in path.read_text().splitlines():
        if line.startswith("static void *const opcode_targets"):
            inside = True
        elif inside and (m := TARGET_ENTRY.match(line)):
            table[int(m[1])] = m[2]
    if len(table) != HANDLER_LIMIT:
        raise GenerateError(f"{path.name}: {len(table)} dispatch entries, not {HANDLER_LIMIT}")
    return table


def write_targets(out: TextIO, p: Products, sources: list[str]) -> None:
    table = old_dispatch()
    for name in p.lowered:
        if table[p.numbers[name]] != "UNKNOWN_OPCODE":
            raise GenerateError(f"handler {p.numbers[name]} of {name} is in use")
        table[p.numbers[name]] = c_name(name)
    write_header(__file__, sources, out)
    out.write("// The dispatch table of Python/ir_targets.h with the register arms.\n")
    out.write("static void *const opcode_targets[512] = {\n")
    for i in range(HANDLER_LIMIT):
        out.write(f"    [{i}] = &&TARGET_{table[i]},\n")
    out.write("};\n")
    out.write('static_assert(Py_ARRAY_LENGTH(opcode_targets) == IR_HANDLER_MASK + 1,\n'
              '              "dispatch covers every id that IR_HANDLER yields");\n')


# Metadata

def operand_mask(names: tuple[str, ...], operands: list[str]) -> str:
    return " | ".join(f"(1 << {operands.index(n)})" for n in names) or "0"


def write_metadata(out: TextIO, p: Products, sources: list[str]) -> None:
    prog = p.program
    write_header(__file__, sources, out)
    families = sorted(prog.families)
    out.write("\n#ifndef Py_INTERNAL_ROP_METADATA_H\n#define Py_INTERNAL_ROP_METADATA_H\n")
    out.write(f"\n#define ROP_FIRST_HANDLER {FIRST_HANDLER}\n")
    out.write(f"#define ROP_NUM_HANDLERS {max(p.numbers.values()) - FIRST_HANDLER + 1}\n")
    out.write(f"#define ROP_NUM_FAMILIES {len(families)}\n")
    out.write(f"#define ROP_NUM_ARMS {len(prog.arms)}\n\n")
    out.write("/* Family numbers, in name order */\nenum {\n")
    for i, f in enumerate(families):
        out.write(f"    ROP_FAMILY_{f} = {i},\n")
    out.write("};\n\n/* Handler numbers of the arms */\n")
    for name in sorted(prog.arms, key=lambda n: p.numbers[n]):
        out.write(f"#define ROP_HANDLER_{c_name(name)} {p.numbers[name]}\n")
    out.write("\n/* Error checkpoint numbers of the points (PROCESS_ERROR_AT) */\n")
    for name, k in p.points.items():
        out.write(f"#define ROP_POINT_{c_name(name)} {k}\n")
    out.write("""
typedef struct {
    const char *name;
    uint16_t handler;
    uint8_t family;
    uint8_t emitted;        /* the arm has a case in the interpreter */
} _PyRopArm;

typedef struct {
    const char *name;
    uint8_t family;
    uint8_t checkpoint;
    uint8_t frame, pc, exc;      /* the roles, as indexes of the catalogs */
    /* Operands of the family, by position in its parameter list */
    uint16_t keep, spent, defs, held;
} _PyRopPoint;

""")
    out.write("static const _PyRopArm _PyRop_Arms[ROP_NUM_ARMS] = {\n")
    for name in sorted(prog.arms, key=lambda n: p.numbers[n]):
        arm = prog.arms[name]
        out.write(f'    {{"{name}", {p.numbers[name]}, ROP_FAMILY_{arm.family}, '
                  f'{int(name in p.lowered)}}},\n')
    out.write("};\n\n")
    out.write(f"static const _PyRopPoint _PyRop_Points[{len(p.points)}] = {{\n")
    for name, k in p.points.items():
        pt = prog.points[name]
        operands = [x.name for x in prog.families[pt.family].params]
        out.write(
            f'    {{"{name}", ROP_FAMILY_{pt.family}, {k}, {list(type(pt.frame)).index(pt.frame)}, '
            f'{list(type(pt.pc)).index(pt.pc)}, {list(type(pt.exc)).index(pt.exc)}, '
            f'{operand_mask(pt.keep, operands)}, {operand_mask(pt.spent, operands)}, '
            f'{operand_mask(pt.defs, operands)}, {operand_mask(pt.held, operands)}}},\n')
    out.write("};\n\n#endif\n")


# Entry point

def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--cases", help="the interpreter cases")
    parser.add_argument("--forms", help="the function forms")
    parser.add_argument("--targets", help="the dispatch table")
    parser.add_argument("--metadata", help="the metadata header")
    parser.add_argument("sources", nargs="*", type=Path)
    args = parser.parse_args(argv)
    paths = args.sources or sorted(ROP_DIR.glob("*.rop"))
    program = rop_check.load(paths)
    if program.diagnostics:
        for d in program.diagnostics:
            print(d, file=sys.stderr)
        return 1
    numbers = read_numbers()
    check_numbers(program, numbers)
    products = lower_all(program, numbers)
    sources = [root_relative_path(str(s)) for s in paths]
    for option, writer in (("cases", write_cases), ("forms", write_forms),
                           ("targets", write_targets), ("metadata", write_metadata)):
        target = getattr(args, option)
        if target:
            with open(target, "w") as out:
                writer(out, products, sources)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
