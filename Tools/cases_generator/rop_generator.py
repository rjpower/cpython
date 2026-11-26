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
import dataclasses
from dataclasses import dataclass
from pathlib import Path
from typing import TextIO

import ir_arms
import jit_template_generator
import rop_check
import rop_leaf
import rop_verify
from generators_common import root_relative_path, write_header
from rop_cgen import (ESCAPING, Lowering, Unsupported, inline_rows, lower_arm,
                      outlined_name, pending_blockers, row_signature)
from rop_catalog import (FB_LAYOUT, RECIPE_LAYOUT, SELECT_DIMENSIONS, Effect, FieldKind,
                         Handle, Slot, Wrap)
from rop_check import Program, select_dimension
from rop_model import Arm, HandleType, Param, WrapType
from rop_print import type_text
from rop_ssa import SsaError

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
    # The same arms for the forms and stubs, which call the out-of-line copy
    # of a row declared inline; empty when no row is
    outlined: Mapping[str, Lowering] = dataclasses.field(default_factory=dict)
    # The completion arms, which compile to functions only
    completions: Mapping[str, Lowering] = dataclasses.field(default_factory=dict)

    def for_forms(self, name: str) -> Lowering:
        return self.outlined.get(name, self.lowered[name])


def point_numbers(program: Program) -> dict[str, int]:
    """Error checkpoint numbers, from 1, by point name"""
    return {name: i + 1 for i, name in enumerate(sorted(program.points))}


def is_generic(program: Program, arm: Arm) -> bool:
    generic = program.families[arm.family].generic
    return generic is not None and arm.name == f"{arm.family}.{generic}"


def lower_all(program: Program, numbers: Mapping[str, int]) -> Products:
    points = point_numbers(program)
    if len(points) > 255:
        raise GenerateError("more than 255 checkpoints: the entry has 8 bits")
    lowered: dict[str, Lowering] = {}
    left_out: dict[str, str] = {}
    # The family's miss adapter is its generic arm (`generic(Arm)`), when
    # that arm is emitted; one that does not lower leaves its family without
    generic: dict[str, str | None] = {f: None for f in program.families}
    generic_arms: dict[str, str | None] = {f: None for f in program.families}
    ordered = sorted(program.arms.values(), key=lambda a: not is_generic(program, a))
    for arm in ordered:
        try:
            lowered[arm.name] = lower_arm(program, arm, numbers, points, generic)
        except Unsupported as e:
            pending = pending_blockers(program, arm)
            left_out[arm.name] = (f"pending: {', '.join(pending)}" if pending
                                  else str(e))
            continue
        if is_generic(program, arm):
            generic[arm.family] = c_name(arm.name)
            generic_arms[arm.family] = arm.name
    outlined: dict[str, Lowering] = {}
    if inline_rows(program):
        outlined = {name: lower_arm(program, program.arms[name], numbers, points,
                                    generic, outline=True)
                    for name in lowered}
    completions: dict[str, Lowering] = {}
    for arm in program.completions.values():
        try:
            completions[arm.name] = lower_arm(program, arm, numbers, points, generic,
                                              outline=True)
        except Unsupported as e:
            pending = pending_blockers(program, arm)
            left_out[arm.name] = (f"pending: {', '.join(pending)}" if pending
                                  else str(e))
    return Products(program, numbers, points, lowered, left_out,
                    generic_arms, outlined, completions)


# Interpreter cases and function forms

PROLOGUE = """\
int opcode = {handler};
(void)opcode;
_PyIRWord *const this_instr = next_instr;
(void)this_instr;
{save_ip}IR_HOLD_NEXT();
next_instr += {advance};
INSTRUCTION_STATS({handler});"""


def handler_text(arm: Arm, low: Lowering, indent: str) -> list[str]:
    lines = [f"// {arm.name} ({arm.region.name})"]
    advance = str(1 + low.words) if low.payload is None else f"{1 + low.words} + {low.payload}"
    if low.spans:
        # A span instruction's length depends on its counts
        assert low.payload is None, arm.name
        advance = "_PyIR_SpanWordLength(opcode, ir_word)"
    lines += PROLOGUE.format(
        handler=handler(arm.name), advance=advance,
        save_ip="" if arm.trailing else "frame->instr_ptr = next_instr;\n").splitlines()
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


def write_layout_checks(out: TextIO) -> None:
    """Each schema and recipe field's C member has the type that
    rop_catalog states: the structs stay hand-written, and the build fails
    when one drifts from the table"""
    out.write("/* The C layout of the feedback schemas and recipes "
              "(rop_catalog.FB_LAYOUT,\n * RECIPE_LAYOUT) */\n")
    for what, layouts in (("schema", FB_LAYOUT), ("recipe", RECIPE_LAYOUT)):
        for schema, layout in layouts.items():
            for name, f in layout.fields.items():
                if f.member is None:
                    continue
                out.write(f"static_assert(_Generic((({layout.struct} *)0)->{f.member}, "
                          f"{f.ctype}: 1, default: 0),\n"
                          f"              \"{what} {schema.value}.{name}: "
                          f"{layout.struct}.{f.member} is not {f.ctype}\");\n")
            for u in layout.unions:
                first = u.members[0]
                for m in u.members[1:]:
                    out.write(f"static_assert(offsetof({layout.struct}, {m}) == "
                              f"offsetof({layout.struct}, {first}),\n"
                              f"              \"{what} {schema.value}: {layout.struct}.{m} "
                              f"does not share {first}'s storage\");\n")


def write_inline_rows(out: TextIO, p: Products) -> None:
    """The out-of-line copy of each row declared `inline`.  The interpreter
    calls the row's own symbol, which is always inlined (the redeclaration
    adds the attribute to the C body, defined once in the runtime); the
    forms and stubs call this copy."""
    out.write("/* Rows declared inline: always inlined into the interpreter, one\n"
              " * out-of-line copy for the forms and stubs */\n")
    for x in inline_rows(p.program):
        assert x.c is not None
        try:
            ret, params = row_signature(x)
        except Unsupported as e:
            raise GenerateError(str(e)) from e
        sep = "" if ret.endswith("*") else " "
        out.write(f"static Py_ALWAYS_INLINE {ret}{sep}{x.c.symbol}({', '.join(params)});\n")
        formals = ", ".join(f"{t}{'' if t.endswith('*') else ' '}p{i}"
                            for i, t in enumerate(params))
        actuals = ", ".join(f"p{i}" for i in range(len(params)))
        keep = "" if ret == "void" else "return "
        out.write(f"static Py_NO_INLINE {ret}{sep}{outlined_name(x)}({formals})\n"
                  f"{{ {keep}{x.c.symbol}({actuals}); }}\n")
    out.write("\n")


def write_forms(out: TextIO, p: Products, sources: list[str]) -> None:
    write_header(__file__, sources, out)
    write_left_out(out, p)
    out.write(MISS_MACRO + "\n")
    write_inline_rows(out, p)
    for name in p.lowered:
        arm, low = p.program.arms[name], p.for_forms(name)
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
#pragma push_macro("IR_BRANCH")
#undef IR_BRANCH
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
#define IR_BRANCH(TARGET) return ir_status | _PyIR_BRANCH
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
#pragma pop_macro("IR_BRANCH")
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
        out.write("\n".join(handler_text(arm, p.for_forms(name), "    ")) + "\n}\n")
    write_completions(out, p)
    out.write("\n" + STUB_EPILOGUE)


def completion_name(arm: str) -> str:
    return f"_PyRop_Complete_{c_name(arm)}"


def write_completions(out: TextIO, p: Products) -> None:
    """Each completion arm as a function with the stubs' status ABI, which
    generator_return calls in the consumer's frame with the child's value,
    and the table from every handler of a family that has one to its
    completion: the observed completion for the instrumented twins.  The
    consumer word's handler is read when the child returns, so a word
    respecialized or instrumented meanwhile completes as it now is."""
    if not p.completions:
        return
    for name, low in p.completions.items():
        arm = p.program.completions[name]
        (param,) = arm.graph.blocks[0].params
        out.write(f"\nstatic Py_NO_INLINE int\n{completion_name(name)}(\n"
                  "    PyThreadState *tstate, _PyInterpreterFrame *frame, _PyIRWord *site,\n"
                  "    PyValue ir_value)\n{\n"
                  "    _PyIRWord ir_word = *site;\n    int ir_status = 0;\n")
        lines = [f"// {name} (completion at {arm.completion})",
                 "int opcode = IR_HANDLER(ir_word);", "(void)opcode;",
                 "_PyIRWord *const this_instr = site;", "(void)this_instr;",
                 *low.decls, f"{param.name} = ir_value;", *low.body]
        out.write("\n".join("    " + line if line else line for line in lines) + "\n}\n")
    out.write("\ntypedef int (*_PyRopCompletion)(PyThreadState *, _PyInterpreterFrame *,\n"
              "                                 _PyIRWord *, PyValue);\n\n"
              "static const _PyRopCompletion _PyRop_Completions[ROP_NUM_HANDLERS] = {\n")
    for handler_name, arm in completion_table(p):
        out.write(f"    [{handler_name} - ROP_FIRST_HANDLER] = {completion_name(arm)},\n")
    out.write("};\n")


def completion_table(p: Products) -> list[tuple[str, str]]:
    """(handler, completion arm) for every handler of a family with an
    emitted completion"""
    prog = p.program
    wrapped = set(wrapped_arms(prog))
    table = []
    for name, arm in prog.arms.items():
        plain = rop_check.completion_of(prog, arm.family)
        if plain is None or plain.name not in p.completions:
            continue
        observed = rop_check.completion_of(prog, arm.family, observes=True)
        if observed is None or observed.name not in p.completions:
            observed = plain
        table.append((handler(name), (observed if arm.observes else plain).name))
        if name in wrapped:
            table.append((handler(name) + "_Observed", observed.name))
    return table


HEAD_FIELDS = {Slot.A: 0, Slot.B: 1, Slot.C: 2, Slot.D: 3}
EFFECT_FLAGS = {
    Effect.runs_python: ("_PyIR_ESCAPES", "_PyIR_RUNS_PYTHON"),
    Effect.polls: ("_PyIR_POLLS",),
    Effect.switches_frame: ("_PyIR_PUSHES_FRAME",),
}
EFFECT_ORDER = ("_PyIR_RAISES", "_PyIR_ESCAPES", "_PyIR_RUNS_PYTHON", "_PyIR_DEOPTS",
                "_PyIR_PUSHES_FRAME", "_PyIR_POLLS", "_PyIR_SPECIALIZES")
LOC_NAMES = {Slot.A: "_PyIR_LOC_A", Slot.B: "_PyIR_LOC_B", Slot.C: "_PyIR_LOC_C",
             Slot.D: "_PyIR_LOC_D"}


# The layout of an argument descriptor (Args) by the stack op that the
# family's words stand for: CALL_KW's last operand is the constant tuple of
# the names, CALL_FUNCTION_EX has no self operand
ARGS_OPERANDS = {"CALL_KW": "_PyIR_OPERANDS_KW",
                 "CALL_FUNCTION_EX": "_PyIR_OPERANDS_EX"}


def arm_format(program: Program, arm: Arm) -> ir_arms.ArmFormat:
    """The fields of the arm's _PyIRFormat row.  Which homes a word reads and
    defines come from the family's derived encoding: a Src operand is an
    input and a Dst operand the destination.  Fields that describe stack
    positions (through, windows, the stack-only flags) stay empty."""
    fam = program.families[arm.family]
    enc = program.encodings[arm.family]
    params = {x.name: x for x in fam.params}
    # A Local operand that one of the family's points defines is the word's
    # written home, as a Dst is (DELETE_FAST publishes null into its local)
    defined = {d for pt in program.points.values() if pt.family == arm.family
               for d in pt.defs}
    roles = ["_PyIR_ROLE_NONE"] * 4
    imm = fb = "_PyIR_LOC_NONE"
    fb_schema = None
    ninputs = writes = 0
    # A loop's pair of homes (the iterator, then its cursor): the family's
    # points say how the word treats it.  A word that defines it writes two
    # homes at the field; one that keeps the receiver steps it in place, as
    # a window of the legacy op; else the word spends both
    reads = None
    through = 0
    pair_writes = 0
    window = False
    send_window = False
    window_name = ""
    has_target = has_args = has_span = False
    target = "_PyIR_LOC_NONE"
    for slot in enc.elements:
        roles[HEAD_FIELDS[slot]] = "_PyIR_ROLE_ELEMENT"
    for pl in enc.placements:
        if pl.kind is FieldKind.flag:
            # A flag of the handler field: no field of the format
            continue
        param = params[pl.operand]
        inner = param.type.arg if isinstance(param.type, WrapType) else param.type
        is_args = isinstance(inner, HandleType) and inner.handle is Handle.Args
        has_args |= is_args
        if pl.kind is FieldKind.vector and is_args:
            # An argument vector's count field is an immediate of the head
            if pl.slots and pl.slots[0] in HEAD_FIELDS:
                roles[HEAD_FIELDS[pl.slots[0]]] = "_PyIR_ROLE_IMM"
                if imm == "_PyIR_LOC_NONE":
                    imm = LOC_NAMES[pl.slots[0]]
            continue
        in_head = bool(pl.slots) and pl.slots[0] in HEAD_FIELDS
        role = "_PyIR_ROLE_IMM"
        if pl.kind is FieldKind.vector:
            role = "_PyIR_ROLE_COUNT"
            has_span = True
        elif pl.kind is FieldKind.home:
            kind, _ = operand_role(param)
            if kind == "SRC":
                role = f"_PyIR_ROLE_IN0 + {ninputs}"
                ninputs += 1
            elif kind == "DST" or (kind == "LOCAL" and pl.operand in defined):
                role = "_PyIR_ROLE_DST"
                writes += 1
            elif kind in ("CELL", "FREE"):
                role = "_PyIR_ROLE_CELL"
            elif kind == "SENDWINDOW":
                # SEND's shape: the window (receiver, cursor, sent value) is
                # read and rewritten in place, the yielded value goes two
                # homes above its base, and the receiver passes through
                role = "_PyIR_ROLE_BASE"
                window = True
                send_window = True
                window_name = pl.operand
            elif kind == "ITERPAIR":
                pair = pl.operand
                if pair in defined:
                    role = "_PyIR_ROLE_DST"
                    pair_writes = 2
                elif any(f"{pair}.receiver" in pt.keep
                         for pt in program.points.values()
                         if pt.family == arm.family):
                    role = "_PyIR_ROLE_BASE"
                    window = True
                else:
                    role = f"_PyIR_ROLE_IN0 + {ninputs}"
                    ninputs += 2
            else:
                role = "_PyIR_ROLE_NONE"
        elif pl.kind is FieldKind.feedback:
            role = "_PyIR_ROLE_FB"
            fb_schema = param.type.schema.value
            if fb_schema in ("Truth", "Unpack", "Super"):
                # Truth, unpack and super sites share the unary entry
                fb_schema = "Unary"
            elif fb_schema == "Item":
                # The item families keep the entry of BINARY_OP and STORE_SUBSCR
                fb_schema = "Binary"
            elif fb_schema == "Iter":
                fb_schema = "Unary"
        elif pl.kind is FieldKind.target:
            has_target = True
            target = "_PyIR_LOC_AB" if len(pl.slots) == 2 else LOC_NAMES[pl.slots[0]]
        if not in_head:
            continue
        first = pl.slots[0]
        roles[HEAD_FIELDS[first]] = role
        if pl.kind is FieldKind.target:
            for slot in pl.slots:
                roles[HEAD_FIELDS[slot]] = role
            # The assembler writes the distance into the target's fields
            continue
        if pl.kind is FieldKind.feedback:
            fb = LOC_NAMES[first]
        elif role == "_PyIR_ROLE_IMM" and imm == "_PyIR_LOC_NONE":
            both = [s.name for s in pl.slots]
            imm = ("_PyIR_LOC_CD" if both == ["C", "D"] else
                   "_PyIR_LOC_BC" if both == ["B", "C"] else
                   "_PyIR_LOC_AB" if both == ["A", "B"] else LOC_NAMES.get(first, imm))
    branch_events = bool({"BRANCH_LEFT", "BRANCH_RIGHT", "JUMP"} & {e.value for e in fam.events})
    exits = set(arm.contract.exits)
    effects = {"_PyIR_RAISES"} if exits & {"raise", "unwind"} else set()
    for e in arm.contract.effect_set:
        effects.update(EFFECT_FLAGS.get(e, ()))
    if "miss" in exits:
        effects.add("_PyIR_DEOPTS")
    if "enter" in exits:
        effects.add("_PyIR_PUSHES_FRAME")
    if "yield" in exits:
        # A yield suspends this frame; unlike a call it leaves no child
        # frame in the scratch tail above its window, so the jump analysis
        # must not treat that tail as overwritten
        effects.discard("_PyIR_PUSHES_FRAME")
    if is_generic(program, arm) and "again" in fam.exits:
        effects.add("_PyIR_SPECIALIZES")
    flags = ["_PyIR_VALID"]
    if window:
        # FOR_ITER's shape: the pair is read and rewritten in place, the
        # item goes two homes above it, and the receiver passes through.  A
        # window that no point keeps (a throw's completion spends it) has no
        # pass-through, and its sources follow the window's homes.
        flags.append("_PyIR_WINDOW")
        kept = send_window and any(
            f"{window_name}.receiver" in pt.keep and pt.defs
            for pt in program.points.values() if pt.family == arm.family)
        reads, through = ((3 + ninputs, 1 if kept else 0) if send_window
                          else (2, 1))
        writes = 3
    if writes:
        # A result that nothing reads goes to the frame's discard slot
        flags += ["_PyIR_STORE_DST", "_PyIR_DISCARDS"]
    if "_PyIR_RUNS_PYTHON" in effects:
        flags.append("_PyIR_RUNS_CODE")
    if has_target:
        flags.append("_PyIR_FORWARD")
        if not exits & {"next", "branch"}:
            flags.append("_PyIR_JUMP_ALWAYS")
        if not set(fam.exits) & {"branch", "jump"}:
            # No exit of the family goes to the target: the word names it for
            # sys.monitoring
            flags.append("_PyIR_NAMES_TARGET")
    return ir_arms.ArmFormat(
        tuple(flags), tuple(e for e in EFFECT_ORDER if e in effects), tuple(roles),
        imm, fb, ninputs,
        2 if window else -ninputs if writes or pair_writes else None,
        (1 << ninputs) - 1,
        fb_schema, (ARGS_OPERANDS.get(fam.ops[0], "_PyIR_OPERANDS_CALL") if has_args else
                    "_PyIR_OPERANDS_SPANS" if has_span else "_PyIR_OPERANDS_NONE"),
        0 if has_args else enc.words, writes + pair_writes, arm_transfer(exits, ninputs, arm.family, writes or pair_writes > 0 or bool(
            {e.name for e in arm.contract.effect_set} & HOME_EFFECTS)), target,
        ("_PyIR_OBSERVER_NONE" if send_window else
         "_PyIR_OBSERVER_FOR_BODY" if window else
         "_PyIR_OBSERVER_FORMAT_VALUE" if branch_events else "_PyIR_OBSERVER_NONE"),
        # A loop step's item is the body's input; its exhaustion completes
        "_PyIR_OBSERVER_COMPLETION" if window else None,
        reads, through)



# The exits that end the word abnormally: they carry no normal-edge contract
ABNORMAL_EXITS = frozenset({"raise", "unwind", "redirect"})


# Effects of an arm that may change what its homes hold
HOME_EFFECTS = frozenset({"writes", "releases", "allocates"})


def arm_transfer(exits: set[str], ninputs: int, family: str, touches_homes: bool) -> str:
    """The normal-edge ownership contract (_PyIR_TRANSFER_*) that an arm's
    declared exits give.  An arm whose every normal exit leaves the frame
    (return, exit) takes its one input with it: EXIT.  The other contracts
    name homes that exits alone do not, and stay UNKNOWN."""
    normal = exits - ABNORMAL_EXITS
    if normal and normal <= {"return", "exit"} and ninputs == 1:
        return "_PyIR_TRANSFER_EXIT"
    # A word with no operand that touches no home leaves every owner as it was
    if not ninputs and not touches_homes and normal <= {"next", "jump"}:
        return "_PyIR_TRANSFER_PRESERVE"
    # BranchNone tests its value and retires it on both edges
    if family == "BranchNone":
        return "_PyIR_TRANSFER_RETIRE"
    return "_PyIR_TRANSFER_UNKNOWN"


def observer(program: Program, family: str) -> str | None:
    """The family's observed wrapper (the arm that `observes`), if it has
    one: every twin of the family's other arms runs it."""
    arm = rop_check.observer_of(program.arms.values(), family)
    return None if arm is None else arm.name


def wrapped_arms(program: Program) -> list[str]:
    """The arms that have a twin: those of a family that declares events,
    but for the family's observed wrapper"""
    return [name for name, arm in program.arms.items()
            if program.families[arm.family].events and not arm.trailing
            and name != observer(program, arm.family)]


def arm_entries(p: Products) -> list[ir_arms.ArmEntry]:
    """Every arm, in handler order, for the per-handler tables, then the
    observed wrapper (twin) of each arm of a family that declares events.
    The old generator's forms, targets and stub plans list the emitted ones
    after the old handlers (ir_generator.py).  A twin runs its family's
    observed wrapper (`observes`), whose case and form it names; without one it
    has neither."""
    shared = [n for n in p.lowered if p.program.arms[n].region.name == "shared"]
    entries = []
    wrapped = wrapped_arms(p.program)
    twin_index = {name: len(p.program.arms) + k for k, name in enumerate(wrapped)}
    for name, arm in p.program.arms.items():
        fam = p.program.families[arm.family]
        # A split arm has no stub and no template: compiled code calls its
        # function form, the plan of an unplanned handler
        region = "none" if arm.region.name == "split" else arm.region.name
        emitted = name in p.lowered
        generic = p.generic.get(arm.family)
        entries.append(ir_arms.ArmEntry(
            c_name(name), p.numbers[name], region, declares_escape(arm),
            stub_name(name) if name in shared else None,
            stub_name(generic) if generic and generic in shared else None,
            name, fam.ops[0],
            () if arm.trailing else tuple(e.value for e in fam.events),
            "again" in fam.exits, is_generic(p.program, arm),
            arm_format(p.program, arm), twin_index.get(name), None, emitted,
            arm.trailing))
    for name in wrapped:
        base = entries[p.numbers[name]]
        runs = observer(p.program, p.program.arms[name].family)
        if runs is None:
            entries.append(dataclasses.replace(
                base, name=base.name + "_Observed", index=twin_index[name],
                dotted=base.dotted + ".Observed", stub=None, generic=None,
                twin=None, observes=base.index, emitted=False))
            continue
        wrapper = entries[p.numbers[runs]]
        entries.append(dataclasses.replace(
            base, name=wrapper.name, index=twin_index[name],
            dotted=base.dotted + ".Observed", region=wrapper.region,
            escapes=wrapper.escapes, stub=wrapper.stub, generic=None,
            twin=None, observes=base.index, emitted=wrapper.emitted))
    return entries


def load_arm_entries(paths: list[Path] | None = None) -> list[ir_arms.ArmEntry]:
    program = rop_verify.load(paths or sorted(ROP_DIR.glob("*.rop")))
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
        except (Unsupported, SsaError, jit_template_generator.Refused) as e:
            refused[name] = str(e)
            continue
        made[name] = ["// The leaf graph of " + name + ":"] + [
            "//   " + line for line in leaf.body.dump()] + lines
    return made, refused


def template_arm_names(paths: list[Path] | None = None) -> list[str]:
    """The C names of the arms that have a template, for the one table of
    templates that jit_template_generator.py writes"""
    program = rop_verify.load(paths or sorted(ROP_DIR.glob("*.rop")))
    if program.diagnostics:
        raise GenerateError("\n".join(str(d) for d in program.diagnostics))
    made, _ = template_texts(lower_all(program, number_arms(program)))
    return [c_name(name) for name in made]


def write_templates(out: TextIO, p: Products, sources: list[str]) -> None:
    made, refused = template_texts(p)
    write_header(__file__, sources, out)
    write_layout_checks(out)
    out.write(f"//\n// {len(made)} templates.  Inline arms that keep the call of their form:\n")
    for name, why in sorted(refused.items()):
        out.write(f"//   {name}: {why}\n")
    out.write("\n")
    for lines in made.values():
        out.write("\n".join(lines) + "\n\n")


# Metadata

HOME_ROLES = {Handle.Src: "SRC", Handle.Dst: "DST", Handle.Local: "LOCAL",
              Handle.Cell: "CELL", Handle.Free: "FREE", Handle.IterPair: "ITERPAIR",
              Handle.SendWindow: "SENDWINDOW"}


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
        for pl in enc.placements:
            if pl.shared:
                # The translator builds the word's immediate with these
                out.write(f"#define ROP_PACK_{f}_{pl.operand}(v) "
                          f"((uint32_t)(v) << {pl.shift})\n")
        out.write(f"static const _PyRopOperand _PyRop_Operands_{f}[] = {{\n")
        for pl in enc.placements:
            role, optional = operand_role(params[pl.operand])
            slots = pl.slots
            offset = slots[0].offset + pl.shift if slots else pl.shift
            flag = (f"IR_F_LOCAL_{slots[0].name}" if pl.kind is FieldKind.home
                    and slots and slots[0].word == 0 else "0")
            out.write(f'    {{"{pl.operand}", ROP_FIELD_{pl.kind.name.upper()}, '
                      f'ROP_ROLE_{role}, {int(optional)}, {pl.bits if slots else 0}, '
                      f'{offset}, {flag}}},  /* {type_text(params[pl.operand].type)} */\n')
        if not enc.placements:
            out.write("    {NULL},\n")
        out.write("};\n")
        out.write(f"static const uint16_t _PyRop_Ops_{f}[] = {{"
                  f"{', '.join(prog.families[f].ops) or '0'}}};\n")
    out.write(f"\nstatic const _PyRopFamily _PyRop_Families[ROP_NUM_FAMILIES] = {{\n")
    for f in families:
        enc = prog.encodings[f]
        out.write(f'    [ROP_FAMILY_{f}] = {{"{f}", {len(enc.placements)}, {enc.words}, '
                  f'_PyRop_Operands_{f}, {len(prog.families[f].ops)}, '
                  f'_PyRop_Ops_{f}}},\n')
    out.write("};\n\n")


def operand_mask(names: tuple[str, ...], operands: list[str]) -> str:
    """The point's operand bits; a projection g.p sets its group's bit"""
    bits = dict.fromkeys(operands.index(n.partition(".")[0]) for n in names)
    return " | ".join(f"(1 << {b})" for b in bits) or "0"


def write_metadata(out: TextIO, p: Products, sources: list[str]) -> None:
    prog = p.program
    write_header(__file__, sources, out)
    families = sorted(prog.families)
    out.write("\n#ifndef Py_INTERNAL_ROP_METADATA_H\n#define Py_INTERNAL_ROP_METADATA_H\n")
    out.write('\n#include "ir_handler_ids.h"     // IR_NUM_OLD_HANDLERS\n#include "pycore_ir.h"          // IR_F_LOCAL_A\n')
    wrapped = wrapped_arms(prog)
    twins = len(wrapped)
    out.write("\n/* The arms' handler ids follow the old handlers, in definition order; the\n"
              " * observed wrappers of the arms of families that declare events follow them */\n")
    out.write("#define ROP_FIRST_HANDLER IR_NUM_OLD_HANDLERS\n")
    out.write(f"#define ROP_NUM_HANDLERS {len(prog.arms) + twins}\n")
    out.write('static_assert(ROP_FIRST_HANDLER + ROP_NUM_HANDLERS <= 512,\n'
              '              "the arms\' handler ids fit the handler field");\n')
    out.write(f"#define ROP_NUM_FAMILIES {len(families)}\n")
    out.write(f"#define ROP_NUM_ARMS {len(prog.arms)}\n")
    out.write("static_assert(ROP_FIRST_HANDLER + ROP_NUM_HANDLERS == IR_NUM_HANDLERS,\n"
              '              "the handler count is the old handlers, the arms and their wrappers");\n\n')
    out.write("/* Family numbers, in name order */\nenum {\n")
    for i, f in enumerate(families):
        out.write(f"    ROP_FAMILY_{f} = {i},\n")
    out.write("};\n\n/* Handler numbers of the arms */\n")
    for name in prog.arms:
        out.write(f"#define {handler(name)} (ROP_FIRST_HANDLER + {p.numbers[name]})\n")
    for k, name in enumerate(wrapped):
        out.write(f"#define {handler(name)}_Observed "
                  f"(ROP_FIRST_HANDLER + {len(prog.arms) + k})\n")
    out.write("\n/* The translator's CFG holds stack opcodes below 256 in its ops, so an op\n"
              " * that names an arm is tagged: ROP_OP_<arm> is ROP_CFG_BASE plus the arm's\n"
              " * handler offset, and ROP_CFG_HANDLER() takes it back to the handler id */\n")
    out.write("#define ROP_CFG_BASE 1024\n")
    out.write("static_assert(ROP_CFG_BASE >= IR_HANDLER_ONLY_BASE(IR_NUM_HANDLERS),\n"
              "              \"CFG arm numbers overlap handler-only bases\");\n")
    out.write("#define ROP_CFG_IS_ARM(op) ((op) >= ROP_CFG_BASE)\n")
    out.write("#define ROP_CFG_HANDLER(op) ((op) - ROP_CFG_BASE + ROP_FIRST_HANDLER)\n")
    for name in prog.arms:
        out.write(f"#define ROP_OP_{c_name(name)} (ROP_CFG_BASE + {p.numbers[name]})\n")
    for k, name in enumerate(wrapped):
        out.write(f"#define ROP_OP_{c_name(name)}_Observed "
                  f"(ROP_CFG_BASE + {len(prog.arms) + k})\n")
    out.write("\n/* ROP_ARM(name) is the arm's handler id; the translator's CFG files\n"
              " * (regcfg*.c, ir_assemble.c) redefine it as the tagged ROP_OP_<name> */\n")
    out.write("#ifndef ROP_ARM\n#define ROP_ARM(name) ROP_HANDLER_##name\n#endif\n")
    out.write("\n/* The arm of a statically selected family that each selection admits\n"
              " * (its `select`): ROP_SELECT_<Family>_<member>... */\n")
    for name, arm in prog.arms.items():
        if arm.select:
            members = "_".join(m.value for m in arm.select)
            out.write(f"#define ROP_SELECT_{arm.family}_{members} ROP_ARM({c_name(name)})\n")
    out.write("\n/* Error checkpoint numbers of the points (PROCESS_ERROR_AT) */\n")
    for name, k in p.points.items():
        out.write(f"#define ROP_POINT_{c_name(name)} {k}\n")
    out.write("""
/* How a family's operands sit in its words (Program.encodings): the field
 * kind, the role of a home, the bit offset in the 64-bit-per-word image of
 * the head and operand words, the width and, for a home in the head, the
 * word flag that says it names a local.  A vector follows in the payload. */
enum { ROP_FIELD_HOME, ROP_FIELD_IMM, ROP_FIELD_NAME, ROP_FIELD_CONST,
       ROP_FIELD_FEEDBACK, ROP_FIELD_TARGET, ROP_FIELD_VECTOR,
       ROP_FIELD_FLAG };
enum { ROP_ROLE_NONE, ROP_ROLE_SRC, ROP_ROLE_DST, ROP_ROLE_LOCAL, ROP_ROLE_CELL,
       ROP_ROLE_FREE, ROP_ROLE_ITERPAIR, ROP_ROLE_SENDWINDOW };

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
    uint8_t nops;            /* the stack ops that lower to the family; the */
    const uint16_t *ops;     /* first is the base opcode of its arms' words */
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
    uint16_t keep, spent, defs;
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
            f'{operand_mask(pt.defs, operands)}}},\n')
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
            out.write(f"    ({tests}) ? ROP_ARM({c_name(a.name)}) : \\\n")
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
    # The products of a definition the checker refuses are never written
    program = rop_verify.load(paths)
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
