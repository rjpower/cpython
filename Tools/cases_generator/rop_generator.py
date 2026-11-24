"""Generate the C products of the register instruction definitions (F2).

From the checked `Program` of Python/register_ops/*.rop:

- the interpreter cases (one computed-goto case per arm),
- the function forms, with the same signature and completion contract as
  the old `ir_run_*` forms,
- the completion stubs of the shared arms (the old generator, ir_generator.py,
  lists the arms in its dispatch, form and stub-plan tables: arm_entries()),
- the metadata: family and arm numbers, the arm-to-handler table, the
  point tables.

An arm whose graph needs a row or construct that is still pending is left
out of the code and listed, with what blocks it, at the head of each file.
Its handler number is allocated and its slot in the dispatch table stays
unknown, so the assembler cannot select it.
"""

import argparse
import sys
from collections.abc import Mapping
from dataclasses import dataclass
from pathlib import Path
from typing import TextIO

import ir_numbers
import jit_template_generator
import rop_check
import rop_leaf
from generators_common import root_relative_path, write_header
from rop_cgen import ESCAPING, Lowering, Unsupported, lower_arm, pending_blockers
from rop_catalog import SELECT_DIMENSIONS, FieldKind, Handle
from rop_check import Program, select_dimension
from rop_model import Arm, HandleType, Param, WrapType
from rop_print import type_text

ROOT = Path(__file__).resolve().parents[2]
ROP_DIR = ROOT / "Python" / "register_ops"

# The arms' handler ids follow the old handlers (ROP_FIRST_HANDLER is the old
# count in the generated metadata) and are dense, in definition order.  They
# are private: nothing outside the generated files names one.
HANDLER_LIMIT = 512


class GenerateError(Exception):
    pass


def c_name(arm: str) -> str:
    return arm.replace(".", "_")


def number_arms(program: Program) -> dict[str, int]:
    """Each arm's index among the arms, in definition order"""
    return {name: i for i, name in enumerate(program.arms)}


def handler(name: str) -> str:
    """The C name of an arm's handler id"""
    return f"ROP_HANDLER_{c_name(name)}"


@dataclass(frozen=True)
class Products:
    program: Program
    numbers: Mapping[str, int]
    points: Mapping[str, int]
    lowered: Mapping[str, Lowering]
    left_out: Mapping[str, str]       # arm -> what blocks it
    generic: Mapping[str, str | None]  # family -> its generic arm, if emitted


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
    generic_arms: dict[str, str | None] = {f: None for f in program.families}
    ordered = sorted(program.arms.values(), key=lambda a: a.name != f"{a.family}.Generic")
    for arm in ordered:
        try:
            lowered[arm.name] = lower_arm(program, arm, numbers, points, generic)
        except Unsupported as e:
            pending = pending_blockers(program, arm)
            left_out[arm.name] = (f"pending: {', '.join(pending)}" if pending
                                  else str(e))
            continue
        if arm.name.endswith(".Generic"):
            generic[arm.family] = c_name(arm.name)
            generic_arms[arm.family] = arm.name
    return Products(program, numbers, points, lowered, left_out,
                    generic_arms)


# Interpreter cases and function forms

PROLOGUE = """\
int opcode = {handler};
(void)opcode;
_PyIRWord *const this_instr = next_instr;
(void)this_instr;
frame->instr_ptr = next_instr;
IR_HOLD_NEXT();
next_instr += {advance};
INSTRUCTION_STATS({handler});"""


def handler_text(arm: Arm, low: Lowering, indent: str) -> list[str]:
    lines = [f"// {arm.name} ({arm.region.name})"]
    advance = str(1 + low.words) if low.payload is None else f"{1 + low.words} + {low.payload}"
    lines += PROLOGUE.format(handler=handler(arm.name), advance=advance).splitlines()
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
        text = handler_text(arm, low, "            ")
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
    for name, low in p.lowered.items():
        arm = p.program.arms[name]
        out.write(f"\nstatic Py_NO_INLINE _PyIRTransfer\nir_run_{c_name(name)}(\n"
                  "    PyThreadState *tstate, _PyInterpreterFrame *frame, "
                  "_PyInstr *next_instr, _PyIRWord ir_word)\n{\n")
        out.write("\n".join(handler_text(arm, low, "    ")) + "\n}\n")


# Whole-word completion stubs (regions89 G1a) and the plan of each arm

STUB_PREAMBLE = """\
/* The whole-word completion stubs of the shared register arms (g1a/abi.md):
 * the arm's statements with the transfer macros bound to status returns.
 * A release reports ESCAPED exactly when it deallocates. */
#ifndef _PyIR_STUB_RELEASE
#define _PyIR_STUB_RELEASE(CLOSE, V) \\
    do { \\
        PyValue ir_released = (V); \\
        ir_status |= _PyIR_StubDropsLast(ir_released); \\
        (CLOSE)(ir_released); \\
    } while (0)
#endif
#pragma push_macro("DISPATCH")
#undef DISPATCH
#pragma push_macro("DISPATCH_SAME_OPARG")
#undef DISPATCH_SAME_OPARG
#pragma push_macro("JUMP_TO_PREDICTED")
#undef JUMP_TO_PREDICTED
#pragma push_macro("PROCESS_ERROR_AT")
#undef PROCESS_ERROR_AT
#pragma push_macro("IR_STUB_ESCAPE")
#undef IR_STUB_ESCAPE
#pragma push_macro("IR_REDIRECT")
#undef IR_REDIRECT
#pragma push_macro("PyValue_CLOSE")
#undef PyValue_CLOSE
#pragma push_macro("PyValue_XCLOSE")
#undef PyValue_XCLOSE
#define DISPATCH() return ir_status
#define DISPATCH_SAME_OPARG() return ir_status | _PyIR_AGAIN
#define JUMP_TO_PREDICTED(name) do { assert(ir_status == 0); return _PyIR_MISS; } while (0)
#define PROCESS_ERROR_AT(ingress, checkpoint, origin) do { \\
        _PyEvalActivation *ir_exit = IR_ACTIVATION(); \\
        ir_exit->exit_kind = _PyIR_EXIT_RAISE; \\
        ir_exit->exit_next = (origin); \\
        ir_exit->exit_entry = \\
            _PyIR_ERROR_ENTRY(_PyIR_ERROR_INGRESS_##ingress, checkpoint); \\
        return ir_status | _PyIR_RAISED; \\
    } while (0)
#define IR_STUB_ESCAPE() (ir_status |= _PyIR_ESCAPED)
#define IR_REDIRECT() do { \\
        if (frame->pending_redirect) { \\
            _PyFrame_CompleteRedirect(frame); \\
        } \\
        return ir_status | _PyIR_ESCAPED | _PyIR_REDIRECT; \\
    } while (0)
#define PyValue_CLOSE(V) _PyIR_STUB_RELEASE(PyValue_CLOSE, V)
#define PyValue_XCLOSE(V) _PyIR_STUB_RELEASE(PyValue_XCLOSE, V)
"""

STUB_EPILOGUE = """\
#pragma pop_macro("PyValue_XCLOSE")
#pragma pop_macro("PyValue_CLOSE")
#pragma pop_macro("IR_REDIRECT")
#pragma pop_macro("IR_STUB_ESCAPE")
#pragma pop_macro("PROCESS_ERROR_AT")
#pragma pop_macro("JUMP_TO_PREDICTED")
#pragma pop_macro("DISPATCH_SAME_OPARG")
#pragma pop_macro("DISPATCH")
"""


def stub_name(arm: str) -> str:
    return f"_PyIR_Rop_Stub_{c_name(arm)}"


def declares_escape(arm: Arm) -> bool:
    return bool(arm.contract.effect_set & ESCAPING)


def write_stubs(out: TextIO, p: Products, sources: list[str]) -> None:
    write_header(__file__, sources, out)
    shared = [n for n in p.lowered if p.program.arms[n].region.name == "shared"]
    out.write(f"//\n// {len(shared)} stubs.\n\n")
    out.write(STUB_PREAMBLE)
    for name in shared:
        arm = p.program.arms[name]
        out.write(f"\nstatic Py_NO_INLINE int\n{stub_name(name)}(\n"
                  "    PyThreadState *tstate, _PyInterpreterFrame *frame, _PyIRWord *site)\n{\n"
                  "    _PyInstr *next_instr = site;\n    _PyIRWord ir_word = *site;\n"
                  "    int ir_status = 0;\n\n")
        out.write("\n".join(handler_text(arm, p.lowered[name], "    ")) + "\n}\n")
    out.write("\n" + STUB_EPILOGUE)


def arm_entries(p: Products) -> list[ir_numbers.ArmEntry]:
    """The emitted arms for the old generator's forms, targets and stub-plan
    tables (ir_generator.py), which list them after the old handlers"""
    shared = [n for n in p.lowered if p.program.arms[n].region.name == "shared"]
    entries = []
    for name in p.lowered:
        arm = p.program.arms[name]
        # A split arm has no stub and no template: compiled code calls its
        # function form, the plan of an unplanned handler
        region = "none" if arm.region.name == "split" else arm.region.name
        generic = p.generic.get(arm.family)
        entries.append(ir_numbers.ArmEntry(
            c_name(name), p.numbers[name], region, declares_escape(arm),
            stub_name(name) if name in shared else None,
            stub_name(generic) if generic and generic in shared else None))
    return entries


def load_arm_entries(paths: list[Path] | None = None) -> list[ir_numbers.ArmEntry]:
    program = rop_check.load(paths or sorted(ROP_DIR.glob("*.rop")))
    if program.diagnostics:
        raise GenerateError("\n".join(str(d) for d in program.diagnostics))
    return arm_entries(lower_all(program, number_arms(program)))


# Baseline templates, from the leaf graphs of the inline arms

def template_texts(p: Products) -> tuple[dict[str, list[str]], dict[str, str]]:
    made: dict[str, list[str]] = {}
    refused: dict[str, str] = {}
    for name, arm in p.program.arms.items():
        if arm.region.name != "inline":
            continue
        try:
            leaf = rop_leaf.leaf_of_arm(p.program, arm)
            lines = rop_leaf.template_of_arm(p.program, arm, c_name(name), leaf)
        except (Unsupported, jit_template_generator.Refused) as e:
            refused[name] = str(e)
            continue
        made[name] = ["// The leaf graph of " + name + ":"] + [
            "//   " + line for line in leaf.body.dump()] + lines
    return made, refused


def template_arm_names(paths: list[Path] | None = None) -> list[str]:
    """The C names of the arms that have a template, for the one table of
    templates that jit_template_generator.py writes"""
    program = rop_check.load(paths or sorted(ROP_DIR.glob("*.rop")))
    if program.diagnostics:
        raise GenerateError("\n".join(str(d) for d in program.diagnostics))
    made, _ = template_texts(lower_all(program, number_arms(program)))
    return [c_name(name) for name in made]


def write_templates(out: TextIO, p: Products, sources: list[str]) -> None:
    made, refused = template_texts(p)
    write_header(__file__, sources, out)
    out.write(f"//\n// {len(made)} templates.  Inline arms that keep the call of their form:\n")
    for name, why in sorted(refused.items()):
        out.write(f"//   {name}: {why}\n")
    out.write("\n")
    for lines in made.values():
        out.write("\n".join(lines) + "\n\n")


# Metadata

HOME_ROLES = {Handle.Src: "SRC", Handle.Dst: "DST", Handle.Local: "LOCAL",
              Handle.Cell: "CELL", Handle.Free: "FREE", Handle.IterPair: "ITERPAIR"}


def operand_role(param: Param) -> tuple[str, bool]:
    """The role of a home operand and whether it is optional"""
    t, optional = param.type, False
    if isinstance(t, WrapType):
        t, optional = t.arg, True
    if isinstance(t, HandleType):
        return HOME_ROLES.get(t.handle, "NONE"), optional
    return "NONE", optional


def write_encodings(out: TextIO, p: Products) -> None:
    prog = p.program
    families = sorted(prog.families)
    for f in families:
        enc = prog.encodings[f]
        params = {x.name: x for x in prog.families[f].params}
        out.write(f"static const _PyRopOperand _PyRop_Operands_{f}[] = {{\n")
        for pl in enc.placements:
            role, optional = operand_role(params[pl.operand])
            slots = pl.slots
            offset = slots[0].offset if slots else 0
            flag = (f"IR_F_LOCAL_{slots[0].name}" if pl.kind is FieldKind.home
                    and slots and slots[0].word == 0 else "0")
            out.write(f'    {{"{pl.operand}", ROP_FIELD_{pl.kind.name.upper()}, '
                      f'ROP_ROLE_{role}, {int(optional)}, {pl.bits if slots else 0}, '
                      f'{offset}, {flag}}},  /* {type_text(params[pl.operand].type)} */\n')
        if not enc.placements:
            out.write("    {NULL},\n")
        out.write("};\n")
    out.write(f"\nstatic const _PyRopFamily _PyRop_Families[ROP_NUM_FAMILIES] = {{\n")
    for f in families:
        enc = prog.encodings[f]
        out.write(f'    [ROP_FAMILY_{f}] = {{"{f}", {len(enc.placements)}, {enc.words}, '
                  f'_PyRop_Operands_{f}}},\n')
    out.write("};\n\n")


def operand_mask(names: tuple[str, ...], operands: list[str]) -> str:
    return " | ".join(f"(1 << {operands.index(n)})" for n in names) or "0"


def write_metadata(out: TextIO, p: Products, sources: list[str]) -> None:
    prog = p.program
    write_header(__file__, sources, out)
    families = sorted(prog.families)
    out.write("\n#ifndef Py_INTERNAL_ROP_METADATA_H\n#define Py_INTERNAL_ROP_METADATA_H\n")
    out.write('\n#include "opcode_ids.h"        // IR_NUM_HANDLERS\n#include "pycore_ir.h"          // IR_F_LOCAL_A\n')
    out.write("\n/* The arms' handler ids follow the old handlers, in definition order */\n")
    out.write("#define ROP_FIRST_HANDLER IR_NUM_HANDLERS\n")
    out.write(f"#define ROP_NUM_HANDLERS {len(prog.arms)}\n")
    out.write('static_assert(ROP_FIRST_HANDLER + ROP_NUM_HANDLERS <= 512,\n'
              '              "the arms\' handler ids fit the handler field");\n')
    out.write(f"#define ROP_NUM_FAMILIES {len(families)}\n")
    out.write(f"#define ROP_NUM_ARMS {len(prog.arms)}\n\n")
    out.write("/* Family numbers, in name order */\nenum {\n")
    for i, f in enumerate(families):
        out.write(f"    ROP_FAMILY_{f} = {i},\n")
    out.write("};\n\n/* Handler numbers of the arms */\n")
    for name in prog.arms:
        out.write(f"#define {handler(name)} (ROP_FIRST_HANDLER + {p.numbers[name]})\n")
    out.write("\n/* Error checkpoint numbers of the points (PROCESS_ERROR_AT) */\n")
    for name, k in p.points.items():
        out.write(f"#define ROP_POINT_{c_name(name)} {k}\n")
    out.write("""
/* How a family's operands sit in its words (Program.encodings): the field
 * kind, the role of a home, the bit offset in the 64-bit-per-word image of
 * the head and operand words, the width and, for a home in the head, the
 * word flag that says it names a local.  A vector follows in the payload. */
enum { ROP_FIELD_HOME, ROP_FIELD_IMM, ROP_FIELD_NAME, ROP_FIELD_CONST,
       ROP_FIELD_FEEDBACK, ROP_FIELD_TARGET, ROP_FIELD_VECTOR };
enum { ROP_ROLE_NONE, ROP_ROLE_SRC, ROP_ROLE_DST, ROP_ROLE_LOCAL, ROP_ROLE_CELL,
       ROP_ROLE_FREE, ROP_ROLE_ITERPAIR };

typedef struct {
    const char *name;
    uint8_t kind, role;
    uint8_t optional;        /* Maybe<...>: a sentinel field value means absent */
    uint8_t bits;
    uint16_t offset;
    uint32_t local_flag;
} _PyRopOperand;

typedef struct {
    const char *name;
    uint8_t noperands;
    uint8_t words;           /* operand words after the head, vectors aside */
    const _PyRopOperand *operands;
} _PyRopFamily;

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
    write_encodings(out, p)
    out.write("static const _PyRopArm _PyRop_Arms[ROP_NUM_ARMS] = {\n")
    for name in prog.arms:
        arm = prog.arms[name]
        out.write(f'    {{"{name}", {handler(name)}, ROP_FAMILY_{arm.family}, '
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
    out.write("};\n\n")
    write_selectors(out, prog)
    out.write("#endif\n")


def write_selectors(out: TextIO, prog: Program) -> None:
    """The static selection of the families whose arms name `select(...)`:
    one member of each catalog dimension, as an enum, and per family a
    constant expression from the members that hold to the arm's handler,
    which a static word array can use"""
    out.write("/* Static selection (rop_catalog.Select): the members of each dimension */\n")
    for dim, members in SELECT_DIMENSIONS.items():
        names = ", ".join(f"ROP_SELECT_{m.value.upper()}" for m in members)
        out.write(f"enum {{ {names} }};  /* {dim} */\n")
    for fam in sorted(prog.families):
        arms = [a for a in prog.arms_of(fam) if a.select]
        if not arms:
            continue
        dims = sorted({select_dimension(m) for a in arms for m in a.select})
        params = ", ".join(dims)
        out.write(f"\n/* The arm of {fam} that holds for ({params}) */\n")
        out.write(f"#define _PyRop_Select{fam}({params}) ( \\\n")
        for a in arms:
            tests = " && ".join(f"({select_dimension(m)}) == ROP_SELECT_{m.value.upper()}"
                                for m in a.select)
            out.write(f"    ({tests}) ? {handler(a.name)} : \\\n")
        out.write("    -1)\n")
    out.write("\n")


# Entry point

def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--cases", help="the interpreter cases")
    parser.add_argument("--forms", help="the function forms")
    parser.add_argument("--stubs", help="the completion stubs")
    parser.add_argument("--templates", help="the baseline templates")
    parser.add_argument("--metadata", help="the metadata header")
    parser.add_argument("sources", nargs="*", type=Path)
    args = parser.parse_args(argv)
    paths = args.sources or sorted(ROP_DIR.glob("*.rop"))
    program = rop_check.load(paths)
    if program.diagnostics:
        for d in program.diagnostics:
            print(d, file=sys.stderr)
        return 1
    numbers = number_arms(program)
    products = lower_all(program, numbers)
    sources = [root_relative_path(str(s)) for s in paths]
    for option, writer in (("cases", write_cases), ("forms", write_forms),
                           ("stubs", write_stubs),
                           ("templates", write_templates),
                           ("metadata", write_metadata)):
        target = getattr(args, option)
        if target:
            with open(target, "w") as out:
                writer(out, products, sources)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
