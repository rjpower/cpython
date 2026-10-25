"""Generate the register IR interpreter.

Reads the instruction definitions from Python/bytecodes.c and writes

- Python/generated_ir_cases.c.h, one handler per instruction, #included in
  ceval.c;
- Python/ir_targets.h, the computed-goto and tail-call tables, indexed by
  handler.

Design note boil/design/06-architecture-v2.md describes the IR, and
Include/internal/pycore_ir.h the word layout. In short, an instruction is one
64-bit word: a 16-bit handler index and four 12-bit fields A, B, C and D, of
which C and D also form a 24-bit field CD. A field names a frame slot, the
feedback entry (its index in the block's feedback vector) or the oparg.
Handlers extract the fields they use from the word in ir_word, which dispatch
loads.

The format of each instruction follows from its stack effect:

- Register style: every input is a scalar, and the slots of dst and the
  inputs, with the feedback index and the oparg, fit in four fields. The
  fields name the first new output (`dst`) and the inputs, bottom first, except
  that a store names the object it stores into first and the value last.
- Window style: the instructions with array inputs, those that do not fit in
  register style, and those with the `window` annotation. The fields name dst,
  if its offset from the deepest input is fixed, and the slot of the deepest
  input (`base`).

Then come the feedback index and the oparg. A jump keeps the absolute word
index of its target in CD, and FOR_ITER and SEND the distance to their exit in
D. Otherwise an oparg that can need more than 12 bits, a constant or name
index, takes CD, and a narrow one the next free field; window style puts it
before the feedback index, as CALL's argc. An instruction with a feedback
entry keeps a wide oparg, or one that does not fit, in the entry's arg field.
BINARY_OP has one handler per operation instead (BINARY_OP_ADD, ...), each
with its operation as a constant, and its specialisations deoptimise to the
handler of the operation that the entry records.

Invariants:

- The handler index of an instruction with an opcode is its opcode. IR-only
  handlers (annotation `ir_only`) follow from 256, then the handlers of the
  BINARY_OP operations in the order of their NB_ constants.
- Members of a family share the format of the family, and INSTRUMENTED_X that
  of X, so specialisation and instrumentation rewrite only the handler field.
  INSTRUMENTED_LINE and INSTRUMENTED_INSTRUCTION take no format: they run
  another handler on the same word.
- The temporary at depth d lives in slot nlocalsplus + d, where the stack
  interpreter keeps it, and handlers keep stack_pointer. A register field
  equals that canonical slot unless it names a local whose LOAD_FAST_BORROW
  the lowering absorbed, which debug builds check at every dispatch. Handlers
  read an input through its field only where the family lets the lowering
  absorb a LOAD_FAST_BORROW, that is where every member converts the input to
  an object first and the format has a dst; there the borrow tag is set
  statically after the object check, if the field is not the canonical slot.
  An absorbed load has no word, so these handlers set stack_pointer from dst
  on entry, and write a borrowed input to its canonical slot only where code
  that reads the stack may run.
- Instruction macros of the DSL (generators_common.STACK_INSTRUCTION_MACROS)
  expand to the IR forms of IR_INSTRUCTION_MACROS.
- A family head marks its PREDICTED_ label with IR_PREDICTED()
  (Python/ceval_macros.h).
"""

import argparse
import re
from dataclasses import dataclass

from analyzer import (
    Analysis,
    CodeSection,
    Instruction,
    Uop,
    Skip,
    Flush,
    StackItem,
    analysis_error,
    analyze_files,
)
from generators_common import (
    DEFAULT_INPUT,
    ROOT,
    Emitter,
    TokenIterator,
    type_and_null,
    write_header,
)
from cwriter import CWriter
from lexer import Token
from stack import (
    Local,
    PointerOffset,
    Stack,
    StackError,
    Storage,
    get_stack_effect,
)
from typing import TextIO

DEFAULT_OUTPUT = ROOT / "Python/generated_ir_cases.c.h"
DEFAULT_TARGETS_OUTPUT = ROOT / "Python/ir_targets.h"

UNKNOWN_OPCODE_HANDLER = """\
_PyErr_Format(tstate, PyExc_SystemError,
              "%U:%d: unknown opcode %d",
              _PyFrame_GetCode(frame)->co_filename,
              PyUnstable_InterpreterFrame_GetLine(frame),
              opcode);
JUMP_TO_LABEL(error);
"""

INSTRUCTION_START_MARKER = "/* BEGIN INSTRUCTIONS */"
INSTRUCTION_END_MARKER = "/* END INSTRUCTIONS */"
LABEL_START_MARKER = "/* BEGIN LABELS */"
LABEL_END_MARKER = "/* END LABELS */"

FIRST_IR_ONLY_HANDLER = 256

# The fields of the word, and the C expressions that extract them from
# ir_word, the word that dispatch loaded (Python/ceval_macros.h).
FIELDS = ("A", "B", "C", "D")
FIELD_VARS = {f: f"IR_{f}(ir_word)" for f in FIELDS}

# The IR forms of the instruction macros (see STACK_INSTRUCTION_MACROS).
IR_INSTRUCTION_MACROS = {
    "FB_COUNTER": "((_PyFbHeader *)fb)",
    "FB_BRANCH": "((_PyFbHeader *)fb)->history",
    "FB_HEADER_OF": "_PyIR_FbHeaderOf(frame, {0}, {1})",
    "INSTR_OPCODE": "IR_HANDLER(*{0p})",
    "INSTR_OPARG": "_PyIR_Oparg(frame, {0})",
    "INSTRUCTION_SIZE_OF": "1u",
    "INSTR_OFFSET_OF": "_PyFrame_InstrOffset(frame, {0})",
    "INSTR_AT_OFFSET": "_PyFrame_InstrAt(frame, {0})",
    "CURRENT_OFFSET": "_PyFrame_InstrOffset(frame, next_instr - 1)",
    "ORIGINAL_HANDLER": "_PyIR_OriginalHandler(frame, this_instr, {0})",
}

# Roles and locations, as in pycore_ir.h.
ROLE_NONE, ROLE_IN0, ROLE_DST, ROLE_BASE, ROLE_FB, ROLE_IMM = (
    "_PyIR_ROLE_NONE", "_PyIR_ROLE_IN0", "_PyIR_ROLE_DST",
    "_PyIR_ROLE_BASE", "_PyIR_ROLE_FB", "_PyIR_ROLE_IMM")
LOC_NONE, LOC_A, LOC_B, LOC_C, LOC_D, LOC_CD, LOC_FBARG = (
    "_PyIR_LOC_NONE", "_PyIR_LOC_A", "_PyIR_LOC_B", "_PyIR_LOC_C",
    "_PyIR_LOC_D", "_PyIR_LOC_CD", "_PyIR_LOC_FBARG")
FIELD_LOCS = {"A": LOC_A, "B": LOC_B, "C": LOC_C, "D": LOC_D}

# The families whose oparg is a forward distance in words, in D, rather than
# an absolute target: the loops, whose bodies jump relative to the next word.
FORWARD_JUMPS = ("FOR_ITER", "SEND")

# Instructions whose oparg can need more than 12 bits although their bodies
# name no constant or name table, see wide_oparg().
WIDE_OPARGS = ("UNPACK_EX",)

# The operations of BINARY_OP, by their NB_ constants in Include/opcode.h.
BINARY_OP_FAMILY = "BINARY_OP"


def binary_ops() -> list[str]:
    "The NB_ operations, in the order of their values"
    ops: dict[int, str] = {}
    with open(ROOT / "Include/opcode.h") as f:
        for line in f:
            m = re.match(r"#define NB_(\w+)\s+(\d+)", line)
            if m and m.group(1) != "OPARG_LAST":
                ops[int(m.group(2))] = m.group(1)
    assert sorted(ops) == list(range(len(ops))), ops
    return [ops[i] for i in range(len(ops))]


def binary_op_handlers(analysis: Analysis) -> list[str]:
    """The generic handlers of BINARY_OP, one per operation, or none if the
    input defines no BINARY_OP"""
    if BINARY_OP_FAMILY not in analysis.instructions:
        return []
    return [f"{BINARY_OP_FAMILY}_{op}" for op in binary_ops()]


@dataclass
class IRFormat:
    "The word layout shared by a family, see the module docstring"
    group: str                      # the family head, or the instruction
    window: bool
    roles: list[str]                # roles of fields A, B, C and D
    inputs: list[str]               # register style: input names, bottom first
    n_inputs: int                   # register style: scalar inputs
    dst_offset: int | None          # slot of dst, from the stack pointer
                                    # before, or for window style from base
    borrow_mask: int                # bit k: input k may name a borrowed local
    imm: str                        # where the oparg lives (LOC_*)
    fb: str                         # where the feedback index lives
    jumps: bool                     # the oparg is an absolute target
    forward: bool = False           # the oparg is a forward distance
    no_fold: bool = False
    transparent: bool = False       # see is_transparent()

    def field_of(self, role: str) -> str | None:
        for name, r in zip(FIELDS, self.roles):
            if r == role:
                return name
        return None

    def input_field(self, k: int) -> str:
        return FIELDS[self.roles.index(f"{ROLE_IN0} + {k}")]

    def canonical_slot(self, k: int) -> str:
        """The C expression of the canonical slot of input k, which follows
        from dst: the slot that the field names unless it names a local."""
        assert self.dst_offset is not None
        dst = FIELD_VARS[self.field_of(ROLE_DST)]
        delta = k - self.n_inputs - self.dst_offset
        return f"{dst} + {delta}" if delta else dst


# An instruction named PREFIX + X, for an instruction X, shares the format of
# X: the instrumented twins.
SHARED_FORMAT_PREFIXES = ("INSTRUMENTED_",)
def declare_variable(var: StackItem, out: CWriter) -> None:
    type, null = type_and_null(var)
    space = " " if type[-1].isalnum() else ""
    out.emit(f"{type}{space}{var.name};\n")


def declare_variables(inst: Instruction, out: CWriter) -> None:
    try:
        stack = get_stack_effect(inst)
    except StackError as ex:
        raise analysis_error(ex.args[0], inst.where) from None
    seen = {"unused"}
    for part in inst.parts:
        if not isinstance(part, Uop):
            continue
        for var in part.stack.inputs:
            if var.used and var.name not in seen:
                seen.add(var.name)
                declare_variable(var, out)
        for var in part.stack.outputs:
            if var.used and var.name not in seen:
                seen.add(var.name)
                declare_variable(var, out)


def stack_read_later(inst: Instruction, uop: Uop) -> bool:
    """Whether the stack slots of uop's inputs may be read after its body
    starts: a deopt or exit re-executes the instruction from the stack, an
    error without pop leaves the stack to exception_unwind, and an escaping
    call exposes the frame."""
    parts = inst.parts[inst.parts.index(uop):]
    return any(
        p.properties.deopts or p.properties.side_exit
        or p.properties.escapes or p.properties.error_without_pop
        for p in parts if isinstance(p, Uop)
    )


def is_transparent(inst: Instruction) -> bool:
    """Whether inst runs another handler on the same fields: it has no stack
    effect, oparg or feedback, and ends in DISPATCH_GOTO()."""
    uops = [part for part in inst.parts if isinstance(part, Uop)]
    if inst.properties.oparg or uses_feedback(inst):
        return False
    if any(uop.stack.inputs or uop.stack.outputs for uop in uops):
        return False
    return any(tkn.text == "DISPATCH_GOTO"
               for uop in uops for tkn in uop.body.tokens())


def uses_feedback(inst: Instruction) -> bool:
    "Whether the instruction names its feedback entry"
    for part in inst.parts:
        if not isinstance(part, Uop):
            continue
        if any(cache.name != "unused" for cache in part.caches):
            return True
        for tkn in part.body.tokens():
            if tkn.kind == "IDENTIFIER" and tkn.text in ("FB_COUNTER", "FB_BRANCH"):
                return True
    return False


@dataclass
class Entry:
    "An item on the abstract stack of external_io()"
    external: int                   # index of the external input, or -1
    item: StackItem
    aliased: bool = False           # assigned from an input, `out = in;`


def passed_through(uop: Uop, j: int) -> int | None:
    """The index of the input of uop that its output j passes through.

    A named output passes through the input of the same name. An `unused`
    output is not written, so it keeps what the input at its position held,
    provided that no array lies below it."""
    inputs, outputs = uop.stack.inputs, uop.stack.outputs
    name = outputs[j].name
    if name != "unused":
        for i, item in enumerate(inputs):
            if item.name == name:
                return i
        return None
    if j < len(inputs) and not any(
            item.is_array() for item in inputs[:j + 1] + outputs[:j + 1]):
        return j
    return None


def external_io(inst: Instruction) -> tuple[list[StackItem], list[Entry]]:
    """The inputs that inst takes from the stack, bottom first, and the
    entries it leaves there, which name an input when it passes through."""
    stack: list[Entry] = []
    external: list[StackItem] = []   # top first
    for part in inst.parts:
        if not isinstance(part, Uop):
            continue
        popped: list[Entry] = []    # top first
        for item in reversed(part.stack.inputs):
            if stack:
                entry = stack.pop()
            else:
                entry = Entry(len(external), item)
                external.append(item)
            popped.append(entry)
        popped.reverse()
        for j, item in enumerate(part.stack.outputs):
            i = passed_through(part, j)
            if i is not None:
                stack.append(popped[i])
            else:
                stack.append(Entry(-1, item, item.name in part.aliased_outputs))
    n = len(external)
    inputs = list(reversed(external))
    outputs = [
        Entry(n - 1 - e.external if e.external >= 0 else -1, e.item, e.aliased)
        for e in stack
    ]
    return inputs, outputs


def assigns_to(uop: Uop, name: str) -> bool:
    "Whether the body of uop assigns to the variable name"
    tokens = list(uop.body.tokens())
    return any(
        tkn.text == name and nxt.kind == "EQUALS"
        and prev.kind not in ("ARROW", "PERIOD")
        for prev, tkn, nxt in zip(tokens, tokens[1:], tokens[2:])
    )


def classify_inputs(inst: Instruction, n_inputs: int) -> list[str]:
    """How inst first relies on each input, bottom first.

    "object" if a uop converts the input to an object (or asserts that it is
    one) before any uop consumes or reassigns it, "raw" if a uop consumes or
    reassigns it first, "unused" if no uop reads it. A uop that passes the
    input through unchanged only inspects it, which does not depend on the
    borrow tag, so it does not decide."""
    result = ["unused"] * n_inputs
    settled = [False] * n_inputs
    stack: list[int] = []  # external index (bottom first) or -1
    depth = n_inputs        # external inputs not yet reached
    for part in inst.parts:
        if not isinstance(part, Uop):
            continue
        popped: list[int] = []      # top first
        kept = {item.name for item in part.stack.outputs}
        for item in reversed(part.stack.inputs):
            if stack:
                ext = stack.pop()
            else:
                depth -= 1
                ext = depth
            if ext >= 0 and item.is_array():
                # The array covers an unknown number of external inputs
                for k in range(n_inputs):
                    if not settled[k]:
                        settled[k] = True
                        result[k] = "raw"
            elif ext >= 0 and not settled[ext] and item.used:
                if not part.properties.inline_ok and (
                        item.name in part.object_inputs
                        or item.name in part.asserted_objects):
                    settled[ext] = True
                    result[ext] = "object"
                elif item.name not in kept or assigns_to(part, item.name):
                    settled[ext] = True
                    result[ext] = "raw"
                else:
                    result[ext] = "raw"  # unless a later uop decides
            popped.append(ext)
        popped.reverse()
        for j in range(len(part.stack.outputs)):
            i = passed_through(part, j)
            stack.append(popped[i] if i is not None else -1)
    return result


def group_name(inst: Instruction, analysis: Analysis) -> str:
    "The instruction whose format inst shares"
    if inst.family is not None:
        return inst.family.name
    for prefix in SHARED_FORMAT_PREFIXES:
        base = inst.name.removeprefix(prefix)
        if base != inst.name and base in analysis.instructions:
            return group_name(analysis.instructions[base], analysis)
    return inst.name


def all_instructions(analysis: Analysis) -> dict[str, Instruction]:
    return {**analysis.instructions, **analysis.ir_only}


def handler_ids(analysis: Analysis) -> dict[str, int]:
    """Handler index of each handler, by name.  BINARY_OP itself has none:
    its operations do."""
    ids = {name: inst.opcode for name, inst in analysis.instructions.items()
           if name != BINARY_OP_FAMILY}
    names = sorted(analysis.ir_only) + binary_op_handlers(analysis)
    for i, name in enumerate(names):
        ids[name] = FIRST_IR_ONLY_HANDLER + i
    return ids


def handler_instructions(analysis: Analysis) -> dict[str, Instruction]:
    "The instruction that each handler runs, by handler name"
    insts = all_instructions(analysis)
    result = {name: insts[name] for name in handler_ids(analysis)
              if name in insts}
    for name in binary_op_handlers(analysis):
        result[name] = insts[BINARY_OP_FAMILY]
    return result


def get_popped(inst: Instruction) -> str:
    stack = get_stack_effect(inst)
    return (-stack.base_offset).to_c()


def first_new_output(outputs: list[Entry], n_inputs: int) -> int | None:
    """The offset from the stack pointer before the instruction of the slot
    of its first new output, if no array lies below it."""
    for j, e in enumerate(outputs):
        if e.item.is_array():
            return None
        if e.external < 0:
            return j - n_inputs
    return None




def window_dst(outputs: list[Entry]) -> int | None:
    """The offset from the deepest input of the slot of the first new
    output, if no array lies below it."""
    for j, e in enumerate(outputs):
        if e.item.is_array():
            return None
        if e.external < 0:
            return j
    return None


def wide_oparg(inst: Instruction) -> bool:
    """Whether the oparg of inst may need more than 12 bits: an index into
    co_consts or co_names, or one of WIDE_OPARGS.  Other opargs are counts,
    slots and flags, which the slot limit and their meaning bound."""
    if inst.name in WIDE_OPARGS:
        return True
    return any(
        tkn.kind == "IDENTIFIER" and tkn.text in ("FRAME_CO_NAMES", "FRAME_CO_CONSTS")
        for part in inst.parts if isinstance(part, Uop)
        for tkn in part.body.tokens())


# The families whose inputs a register-style format names as the object
# stored into, then the rest, then the value: the bottom input goes last.
STORES = ("STORE_ATTR", "STORE_SUBSCR", "STORE_SLICE")


def place_fields(slots: list[str], has_fb: bool, imm_kind: str | None,
                 window: bool) -> tuple[list[str], str, str] | None:
    """The roles of fields A to D and the locations of the oparg and of the
    feedback index, given the slot roles, or None if they do not fit.

    imm_kind is None, "target" (an absolute jump target, in CD), "forward"
    (a distance, in D), "wide" (in CD) or "narrow" (in a 12-bit field)."""
    fields = list(slots)
    imm, fb = LOC_NONE, LOC_NONE

    def add_fb() -> None:
        nonlocal fb
        if has_fb:
            fb = FIELD_LOCS[FIELDS[len(fields)]] if len(fields) < 4 else "full"
            fields.append(ROLE_FB)

    if imm_kind == "target":
        add_fb()
        if len(fields) > 2:
            return None
        fields += [ROLE_NONE] * (2 - len(fields)) + [ROLE_IMM, ROLE_IMM]
        imm = LOC_CD
    elif imm_kind == "forward":
        add_fb()
        if len(fields) > 3:
            return None
        fields += [ROLE_NONE] * (3 - len(fields)) + [ROLE_IMM]
        imm = LOC_D
    elif imm_kind == "wide" and has_fb:
        add_fb()
        imm = LOC_FBARG
    elif imm_kind == "wide":
        if len(fields) > 2:
            return None
        fields += [ROLE_NONE] * (2 - len(fields)) + [ROLE_IMM, ROLE_IMM]
        imm = LOC_CD
    elif imm_kind == "narrow":
        if len(fields) + has_fb + 1 > 4:
            if not has_fb:
                return None
            add_fb()
            imm = LOC_FBARG
        elif window:
            # As CALL's argc: the count before the feedback index
            imm = FIELD_LOCS[FIELDS[len(fields)]]
            fields.append(ROLE_IMM)
            add_fb()
        else:
            add_fb()
            imm = FIELD_LOCS[FIELDS[len(fields)]]
            fields.append(ROLE_IMM)
    else:
        add_fb()
    if len(fields) > 4:
        return None
    fields += [ROLE_NONE] * (4 - len(fields))
    return fields, imm, fb


def compute_format(group: str, members: list[Instruction]) -> IRFormat:
    """The format of a group; members[0] is its head.

    Members may read different numbers of inputs, as INSTRUMENTED_END_FOR
    reads two that END_FOR leaves alone. Their inputs are aligned at the top
    of the stack, and the fields cover the member that reads deepest."""
    head = members[0]
    io = {m.name: external_io(m) for m in members}
    deepest = max(members, key=lambda m: len(io[m.name][0]))
    inputs = io[deepest.name][0]
    n_inputs = len(inputs)
    # Members that pass their first outputs through write later slots, which
    # follow dst. Members that push a frame and no outputs write none.
    new_outputs = [
        first_new_output(io[m.name][1], len(io[m.name][0])) for m in members
    ]
    dst_offset = min((o for o in new_outputs if o is not None), default=None)
    array_inputs = any(m.properties.window for m in members) or any(
        item.is_array() for m in members for item in io[m.name][0])
    jumps = any(m.properties.jumps for m in members)
    forward = jumps and group in FORWARD_JUMPS
    has_fb = any(uses_feedback(m) for m in members)
    has_imm = any(m.properties.oparg for m in members)
    if not has_imm:
        imm_kind = None
    elif forward:
        imm_kind = "forward"
    elif jumps:
        imm_kind = "target"
    elif any(wide_oparg(m) for m in members):
        imm_kind = "wide"
    else:
        imm_kind = "narrow"
    no_fold = any(m.properties.no_fold for m in members) or any(
        e.aliased for m in members for e in io[m.name][1])

    placed = None
    if not array_inputs:
        slots = [ROLE_DST] if dst_offset is not None else []
        order = list(range(n_inputs))
        if group in STORES and dst_offset is None and n_inputs > 1:
            order = order[1:] + order[:1]
        slots += [f"{ROLE_IN0} + {k}" for k in order]
        placed = place_fields(slots, has_fb, imm_kind, False)
    window = placed is None
    if window:
        # The head's result, where its members may write lower slots too,
        # as SEND_ASYNC_GEN does
        dst_offset = window_dst(io[head.name][1])
        slots = ([ROLE_DST] if dst_offset is not None else []) + [ROLE_BASE]
        placed = place_fields(slots, has_fb, imm_kind, True)
        if placed is None:
            raise analysis_error(
                f"the fields of {group} do not fit in a word", head.where)
        n_inputs = 0
    roles, imm, fb = placed
    for m, m_dst in zip(members, new_outputs):
        if not window and m_dst is not None and m.properties.switches_frame:
            # Outputs that follow the switch belong to another frame
            raise analysis_error(
                f"{m.name} switches frames and pushes outputs; "
                "annotate it `window`", m.where)
    names: list[str] = []
    borrow_mask = 0
    if not window:
        names = [item.name for item in inputs]
        uses: list[list[str]] = []
        passthrough: set[int] = set()
        for m in members:
            m_inputs, m_outputs = io[m.name]
            shift = n_inputs - len(m_inputs)
            uses.append(["unused"] * shift + classify_inputs(m, len(m_inputs)))
            passthrough |= {e.external + shift for e in m_outputs if e.external >= 0}
        for k in range(n_inputs):
            if k in passthrough:
                continue
            if all(u[k] in ("object", "unused") for u in uses) and any(
                    u[k] == "object" for u in uses):
                borrow_mask |= 1 << k
        if dst_offset is None:
            # The handler finds the stack pointer through dst, see
            # generate_ir_cases(), so without one it borrows nothing.
            borrow_mask = 0
    return IRFormat(
        group=group, window=window, roles=roles, inputs=names,
        n_inputs=n_inputs, dst_offset=dst_offset, borrow_mask=borrow_mask,
        imm=imm, fb=fb, jumps=jumps and not forward, forward=forward,
        no_fold=no_fold,
    )


def compute_formats(analysis: Analysis) -> dict[str, IRFormat]:
    "The format of every handler and instruction, by name"
    insts = all_instructions(analysis)
    groups: dict[str, list[Instruction]] = {}
    transparent = {name for name, inst in insts.items() if is_transparent(inst)}
    for name in sorted(insts):
        inst = insts[name]
        if name in transparent:
            continue
        group = group_name(inst, analysis)
        groups.setdefault(group, []).append(inst)
    formats: dict[str, IRFormat] = {}
    for group, members in groups.items():
        # The head first: its stack effect defines the family's
        head = insts[group]
        members = [head] + [m for m in members if m is not head]
        fmt = compute_format(group, members)
        for m in members:
            formats[m.name] = fmt
    for name in binary_op_handlers(analysis):
        formats[name] = formats[BINARY_OP_FAMILY]
    for name in sorted(transparent):
        formats[name] = IRFormat(
            group=name, window=False, roles=[ROLE_NONE] * 4,
            inputs=[], n_inputs=0, dst_offset=None, borrow_mask=0,
            imm=LOC_NONE, fb=LOC_NONE, jumps=False,
            transparent=True)
    return formats


def field_expr(loc: str) -> str:
    "The C expression of the field at loc, in a handler"
    if loc == LOC_CD:
        return "IR_CD(ir_word)"
    return FIELD_VARS[loc.removeprefix("_PyIR_LOC_")]


def is_specialization(inst: Instruction) -> bool:
    "Whether inst is a specialised member of a family, not its head"
    return inst.family is not None


class IRStack(Stack):
    """A stack that reads the external inputs at the offsets of field_inputs
    through their fields, the SlotStack of the design note. An offset leaves
    field_inputs when the handler writes the canonical slot, which then holds
    the current value."""

    def __init__(self, field_inputs: dict[int, str] | None = None) -> None:
        super().__init__()
        self.field_inputs = dict(field_inputs or {})

    def copy(self) -> "IRStack":
        other = super().copy()
        assert isinstance(other, IRStack)
        other.field_inputs = dict(self.field_inputs)
        return other

    def field_of(self, var: StackItem, memory_offset: PointerOffset) -> str | None:
        offset = memory_offset.as_int()
        if var.is_array() or offset is None:
            return None
        return self.field_inputs.get(offset)

    def load(self, var: StackItem, memory_offset: PointerOffset) -> str:
        field = self.field_of(var, memory_offset)
        if field is not None:
            return f"IR_SLOT({FIELD_VARS[field]})"
        return super().load(var, memory_offset)

    def pop(self, var: StackItem, out: CWriter) -> Local:
        local = super().pop(var, out)
        if local.memory_offset is not None and var.used and local.ir_field is None:
            local.ir_field = self.field_of(var, local.memory_offset)
        return local

    def flush(self, out: CWriter) -> None:
        # An input still read through its field may name a local, and then
        # its canonical slot holds whatever the frame left there.  Clear it
        # for whoever reads the stack.
        for var in self.variables if self.physical_sp is not None else ():
            if var.ir_field is not None and var.memory_offset is not None:
                offset = (var.memory_offset - self.physical_sp).to_c()
                out.emit(f"IR_CLEAR_IF_BORROWED({FIELD_VARS[var.ir_field]}, "
                         f"stack_pointer[{offset}]);\n")
        super().flush(out)

    def save_variables(self, out: CWriter) -> None:
        var_offset = self.base_offset
        for var in self.variables:
            if var.in_local and not var.memory_offset and not var.is_array():
                offset = var_offset.as_int()
                if offset is not None:
                    self.field_inputs.pop(offset, None)
            var_offset = var_offset.push(var.item)
        super().save_variables(out)

    def merge(self, other: Stack, out: CWriter) -> None:
        super().merge(other, out)
        assert isinstance(other, IRStack)
        for offset in list(self.field_inputs):
            if offset not in other.field_inputs:
                del self.field_inputs[offset]


class IREmitter(Emitter):
    "Emits handler bodies for the register IR"

    instruction_macros = IR_INSTRUCTION_MACROS

    def __init__(self, out: CWriter, analysis: Analysis,
                 formats: dict[str, IRFormat]):
        super().__init__(out, analysis.labels)
        self.formats = formats
        self._replacers["INSTRUCTION_SIZE"] = self.ir_instruction_size
        # The storage at a deopt or exit being emitted
        self.deopt_storage: Storage | None = None

    def deopt_if(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        self.deopt_storage = storage
        try:
            return super().deopt_if(tkn, tkn_iter, uop, storage, inst)
        finally:
            self.deopt_storage = None

    exit_if = deopt_if

    def canonical_slot(self, inst: Instruction | None, field: str) -> str:
        "The canonical slot of the input that inst reads through field"
        assert inst is not None
        fmt = self.formats[inst.name]
        for k in range(fmt.n_inputs):
            if fmt.input_field(k) == field:
                return fmt.canonical_slot(k)
        raise analysis_error(f"{inst.name}: no input in field {field}",
                             inst.where)

    def emit_borrow_drops(self, inst: Instruction | None) -> None:
        """Drop the borrows that IR_BORROW_KNOWN made of locals before a
        deopt, where the family head reads the locals again.  Only the
        Py_STACKREF_DEBUG checker tracks them."""
        storage = self.deopt_storage
        if storage is None:
            return
        seen = set()
        for var in storage.inputs + storage.stack.variables + storage.outputs:
            if (var.ir_borrowed and var.in_local and var.ir_field is not None
                    and var.name not in seen):
                seen.add(var.name)
                self.emit(f"IR_DROP_BORROW({var.name}, "
                          f"{FIELD_VARS[var.ir_field]}, "
                          f"{self.canonical_slot(inst, var.ir_field)});\n")

    def expand_instruction_macro(self, tkn: Token, args: list[list[Token]],
                                 inst: Instruction | None) -> str:
        if tkn.text == "INSTRUCTION_SIZE_OF":
            if len(args) != 1 or len(args[0]) != 1:
                raise analysis_error("INSTRUCTION_SIZE_OF takes a family", tkn)
            family = args[0][0].text
            if family not in self.formats:
                raise analysis_error(f"No instruction {family}", tkn)
            return "1u"
        return super().expand_instruction_macro(tkn, args, inst)

    def emit_jump_to_predicted(self, inst: Instruction | None) -> None:
        assert inst is not None
        self.emit_borrow_drops(inst)
        fmt = self.formats[inst.name]
        if inst.family is not None and inst.family.name == BINARY_OP_FAMILY:
            # The handler of the operation that the entry records
            self.emit(f"UPDATE_MISS_STATS({BINARY_OP_FAMILY});\n")
            self.emit(f"IR_DEOPT_BINARY_OP({field_expr(fmt.fb)});\n")
            return
        if inst.family is not None:
            super().emit_jump_to_predicted(inst)
            return
        raise analysis_error(f"{inst.name} deoptimises but has no family",
                             inst.where)

    def ir_instruction_size(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        self.out.emit(" 1u ")
        return True

    def emit_object_inputs(
        self, uop: Uop, storage: Storage, inst: Instruction | None, stack_read_later: bool
    ) -> None:
        """Inputs read through a field: check, box and tag as borrowed when
        the field names a local. A box goes back to the slot the field names,
        which owns it then."""
        self.deopt_storage = storage
        try:
            self.convert_object_inputs(uop, storage, inst, stack_read_later)
        finally:
            self.deopt_storage = None

    def convert_object_inputs(
        self, uop: Uop, storage: Storage, inst: Instruction | None, stack_read_later: bool
    ) -> None:
        kept = {item.name for item in uop.stack.outputs}
        for var in storage.inputs:
            if var.ir_field is None or var.known_object:
                continue
            f = FIELD_VARS[var.ir_field]
            converts = not uop.properties.inline_ok and (
                var.name in uop.object_inputs or var.name in uop.asserted_objects)
            if not converts:
                if var.name in kept and not assigns_to(uop, var.name):
                    continue  # An inspection, see classify_inputs()
                assert inst is not None
                raise analysis_error(
                    f"{inst.name}: input '{var.name}' may name a borrowed "
                    f"local but {uop.name} consumes it as is", inst.where)
            if var.name not in uop.asserted_objects:
                self.out.start_line()
                if uop.is_guard():
                    self.emit(f"if (Py_UNLIKELY(!PyStackRef_IsPointerSpace({var.name}))) {{\n")
                    self.emit_jump_to_predicted(inst)
                else:
                    self.emit(f"if (Py_UNLIKELY(PyStackRef_IsInline({var.name}))) {{\n")
                    self.emit(f"{var.name} = PyStackRef_Box({var.name});\n")
                    self.emit(f"if (PyStackRef_IsInline({var.name})) {{\n")
                    storage.stack.copy().flush(self.out)
                    self.emit(f"{self.jump_prefix}JUMP_TO_LABEL(error);\n")
                    self.emit("}\n")
                    if inst is not None:
                        self.emit(f"BOX_STAT_INC({inst.name});\n")
                    self.emit(f"IR_SLOT({f}) = {var.name};\n")
                self.emit("}\n")
            self.out.start_line()
            self.emit(f"{var.name} = IR_BORROW_KNOWN({var.name}, {f}, "
                      f"{self.canonical_slot(inst, var.ir_field)});\n")
            var.known_object = True
            var.ir_borrowed = True
            # From here the value is safe in any slot, so a flush writes it to
            # its canonical slot for code that reads the stack.
            var.memory_offset = None
        super().emit_object_inputs(uop, storage, inst, stack_read_later)


def write_uop(
    uop: Uop | Skip | Flush,
    emitter: IREmitter,
    stack: Stack,
    inst: Instruction,
    braces: bool,
) -> tuple[bool, Stack]:
    if isinstance(uop, Skip):
        return True, stack
    if isinstance(uop, Flush):
        emitter.emit(f"// flush\n")
        stack.flush(emitter.out)
        return True, stack
    emitter.out.start_line()
    if braces:
        emitter.out.emit(f"// {uop.name}\n")
        emitter.emit("{\n")
    storage = Storage.for_uop(stack, uop, emitter.out)
    for cache in uop.caches:
        if cache.name != "unused":
            if cache.size == 4:
                value = f"PyObject *{cache.name} = (PyObject *)fb->{cache.name};"
            else:
                value = f"uint32_t {cache.name} = fb->{cache.name};"
            emitter.emit(value + "\n")
            if inst.family is None:
                emitter.emit(f"(void){cache.name};\n")
    emitter.emit_object_inputs(uop, storage, inst, stack_read_later(inst, uop))
    reachable, storage = emitter.emit_tokens(uop, storage, inst, False)
    if braces:
        emitter.out.start_line()
        emitter.emit("}\n")
    return reachable, storage.stack


def feedback_type(fmt: IRFormat) -> str:
    "The entry type of a format: that of its group, see pycore_feedback.h"
    return f"_PyFb_{fmt.group}"


def reads_fb_arg(inst: Instruction, fmt: IRFormat, operation: str | None) -> bool:
    "Whether the handler takes its oparg from the arg field of its entry"
    return (inst.properties.oparg and fmt.imm == LOC_FBARG
            and operation is None)


def needs_this(inst: Instruction, fmt: IRFormat, operation: str | None) -> bool:
    if inst.properties.needs_this:
        return True
    if reads_fb_arg(inst, fmt, operation) and not is_specialization(inst):
        # For the shared entry, see IR_FB_ARG
        return True
    for part in inst.parts:
        if isinstance(part, Uop):
            for tkn in part.body.tokens():
                if tkn.kind == "IDENTIFIER" and tkn.text in ("DEOPT_IF", "EXIT_IF"):
                    return True
    return False


def emit_field_checks(inst: Instruction, fmt: IRFormat, out: CWriter) -> None:
    """Debug checks that each slot field names its canonical slot, given as
    an offset from stack_pointer, or for an input that may be borrowed, a
    local.  The members of a window-style family pop different numbers of
    inputs for the same oparg, so a base is checked against the member's
    own."""
    if fmt.transparent:
        return
    popped = get_popped(inst)
    base = f"-{popped}" if popped.isdigit() else f"-({popped})"
    for name, role in zip(FIELDS, fmt.roles):
        f = FIELD_VARS[name]
        if role == ROLE_DST and fmt.window:
            out.emit(f"IR_CHECK_SLOT({f}, {base} + {fmt.dst_offset});\n")
        elif role == ROLE_DST:
            out.emit(f"IR_CHECK_SLOT({f}, {fmt.dst_offset});\n")
        elif role == ROLE_BASE:
            out.emit(f"IR_CHECK_SLOT({f}, {base});\n")
        elif role.startswith(ROLE_IN0):
            k = int(role.split("+")[1])
            if fmt.borrow_mask & (1 << k):
                out.emit(f"IR_CHECK_INPUT({f}, {k - fmt.n_inputs});\n")
            else:
                out.emit(f"IR_CHECK_SLOT({f}, {k - fmt.n_inputs});\n")


def generate_handler(name: str, inst: Instruction, fmt: IRFormat,
                     emitter: IREmitter, out: CWriter) -> None:
    """The handler name, which runs inst: inst itself, or for a handler of a
    BINARY_OP operation, BINARY_OP with that operation."""
    operation = None
    if name != inst.name:
        operation = "NB_" + name.removeprefix(BINARY_OP_FAMILY + "_")
    is_target = inst.is_target and operation is None
    out.emit("\n")
    out.emit(f"TARGET({name}) {{\n")
    out.emit(f"#if _Py_TAIL_CALL_INTERP\n")
    out.emit(f"int opcode = {name};\n")
    out.emit(f"(void)(opcode);\n")
    out.emit(f"#endif\n")
    if fmt.borrow_mask:
        # Absorbed loads have no words, so stack_pointer may lag behind
        dst = FIELD_VARS[fmt.field_of(ROLE_DST)]
        out.emit(f"stack_pointer = IR_STACK_POINTER({dst}, {fmt.dst_offset});\n")
    this = needs_this(inst, fmt, operation)
    if inst.properties.needs_prev:
        out.emit(f"_PyIRWord* const prev_instr = frame->instr_ptr;\n")
    if this and not is_target:
        out.emit(f"_PyIRWord* const this_instr = next_instr;\n")
        out.emit("(void)this_instr;\n")
    if not inst.properties.no_save_ip:
        out.emit(f"frame->instr_ptr = next_instr;\n")
    out.emit(f"next_instr += 1;\n")
    out.emit(f"INSTRUCTION_STATS({inst.name});\n")
    if is_target:
        out.emit(f"IR_PREDICTED({name});\n")
        if this:
            out.emit(f"_PyIRWord* const this_instr = next_instr - 1;\n")
            out.emit("(void)this_instr;\n")
    use_fb = fmt.fb != LOC_NONE and (
        uses_feedback(inst) or reads_fb_arg(inst, fmt, operation))
    if use_fb:
        out.emit(f"{feedback_type(fmt)} *fb = ({feedback_type(fmt)} *)"
                 f"IR_FB({field_expr(fmt.fb)});\n")
    if inst.properties.oparg:
        if operation is not None:
            out.emit(f"int oparg = {operation};\n")
        elif fmt.imm == LOC_FBARG and is_specialization(inst):
            out.emit("int oparg = (int)((_PyFbHeader *)fb)->arg;\n")
        elif fmt.imm == LOC_FBARG:
            out.emit(f"int oparg = IR_FB_ARG(fb, {field_expr(fmt.fb)});\n")
        else:
            out.emit(f"int oparg = (int){field_expr(fmt.imm)};\n")
        out.emit("(void)oparg;\n")
    emit_field_checks(inst, fmt, out)
    if inst.properties.uses_opcode:
        out.emit(f"opcode = {name};\n")
    declare_variables(inst, out)
    field_inputs: dict[int, str] = {}
    for k in range(fmt.n_inputs):
        if fmt.borrow_mask & (1 << k):
            field_inputs[k - fmt.n_inputs] = fmt.input_field(k)
    stack: Stack = IRStack(field_inputs)
    insert_braces = len([p for p in inst.parts if isinstance(p, Uop)]) > 1
    reachable = True
    for part in inst.parts:
        reachable, stack = write_uop(part, emitter, stack, inst, insert_braces)
    out.start_line()
    if reachable:
        stack.flush(out)
        out.emit("DISPATCH();\n")
    out.start_line()
    out.emit("}")
    out.emit("\n")


def generate_ir_cases(analysis: Analysis, formats: dict[str, IRFormat],
                      outfile: TextIO, lines: bool) -> None:
    out = CWriter(outfile, 2, lines)
    emitter = IREmitter(out, analysis, formats)
    out.emit("\n")
    for name, inst in sorted(handler_instructions(analysis).items()):
        generate_handler(name, inst, formats[name], emitter, out)


def generate_ir_labels(analysis: Analysis, emitter: Emitter) -> None:
    emitter.emit("\n")
    for name, label in analysis.labels.items():
        emitter.emit(f"LABEL({name})\n")
        storage = Storage(IRStack(), [], [], 0, False)
        if label.spilled:
            storage.spilled = 1
            storage.stack.physical_sp = None
        emitter.emit_tokens(label, storage, None)
        emitter.emit("\n\n")


def generate_ir(filenames: list[str], analysis: Analysis, outfile: TextIO,
                lines: bool) -> None:
    formats = compute_formats(analysis)
    write_header(__file__, filenames, outfile)
    outfile.write(f"""
#if !_Py_TAIL_CALL_INTERP
#if !USE_COMPUTED_GOTOS
    dispatch_opcode:
        switch (opcode)
#endif
        {{
#endif /* _Py_TAIL_CALL_INTERP */
            {INSTRUCTION_START_MARKER}
""")
    generate_ir_cases(analysis, formats, outfile, lines)
    outfile.write(f"""
            {INSTRUCTION_END_MARKER}
#if !_Py_TAIL_CALL_INTERP
#if USE_COMPUTED_GOTOS
        _unknown_opcode:
#else
        default:
#endif
            /* Tell C compilers not to hold the opcode variable in the loop.
               next_instr points the current instruction without TARGET(). */
            opcode = IR_HANDLER(*next_instr);
            {UNKNOWN_OPCODE_HANDLER}

        }}

        /* This should never be reached. Every opcode should end with DISPATCH()
           or goto error. */
        Py_UNREACHABLE();
#endif /* _Py_TAIL_CALL_INTERP */
        {LABEL_START_MARKER}
""")
    out = CWriter(outfile, 2, lines)
    emitter = IREmitter(out, analysis, formats)
    generate_ir_labels(analysis, emitter)
    outfile.write(f"{LABEL_END_MARKER}\n")


def function_proto(name: str, prefix: str = "_TAIL_CALL_") -> str:
    return f"static PyObject *Py_PRESERVE_NONE_CC {prefix}{name}(TAIL_CALL_PARAMS)"


def num_handlers(analysis: Analysis) -> int:
    return (FIRST_IR_ONLY_HANDLER + len(analysis.ir_only)
            + len(binary_op_handlers(analysis)))


def generate_ir_targets(filenames: list[str], analysis: Analysis,
                        outfile: TextIO) -> None:
    "The dispatch tables, indexed by handler"
    ids = handler_ids(analysis)
    n = num_handlers(analysis)
    write_header(__file__, filenames, outfile)
    out = CWriter(outfile, 0, False)
    targets = ["&&_unknown_opcode,\n"] * n
    for name, i in ids.items():
        targets[i] = f"&&TARGET_{name},\n"
    out.emit("#if !_Py_TAIL_CALL_INTERP\n")
    out.emit(f"static void *opcode_targets[{n}] = {{\n")
    for target in targets:
        out.emit(target)
    out.emit("};\n")
    out.emit("#else /* _Py_TAIL_CALL_INTERP */\n")
    out.emit(f"static py_tail_call_funcptr instruction_funcptr_handler_table[{n}];\n")
    out.emit("\n")
    for name in analysis.labels:
        out.emit(f"{function_proto(name)};\n")
    out.emit("\n")
    for name in sorted(ids):
        out.emit(f"{function_proto(name)};\n")
    out.emit("\n")
    out.emit(function_proto("UNKNOWN_OPCODE"))
    out.emit(" {\n")
    out.emit("int opcode = IR_HANDLER(*next_instr);\n")
    out.emit(UNKNOWN_OPCODE_HANDLER)
    out.emit("}\n")
    out.emit("\n")
    by_index = {i: name for name, i in ids.items()}
    out.emit(f"static py_tail_call_funcptr instruction_funcptr_handler_table[{n}] = {{\n")
    for i in range(n):
        name = by_index.get(i, "UNKNOWN_OPCODE")
        out.emit(f"[{i}] = _TAIL_CALL_{name},\n")
    out.emit("};\n")
    out.emit("#endif /* _Py_TAIL_CALL_INTERP */\n")


def generate_ir_from_files(filenames: list[str], outfilename: str,
                           lines: bool) -> None:
    data = analyze_files(filenames)
    with open(outfilename, "w") as outfile:
        generate_ir(filenames, data, outfile, lines)


arg_parser = argparse.ArgumentParser(
    description="Generate the handlers of the register IR interpreter.",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
)

arg_parser.add_argument(
    "-o", "--output", type=str, help="Generated code", default=DEFAULT_OUTPUT
)

arg_parser.add_argument(
    "-t", "--targets", type=str, help="Generated dispatch tables",
    default=DEFAULT_TARGETS_OUTPUT
)

arg_parser.add_argument(
    "-l", "--emit-line-directives", help="Emit #line directives", action="store_true"
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
        generate_ir(args.input, data, outfile, args.emit_line_directives)
    with open(args.targets, "w") as outfile:
        generate_ir_targets(args.input, data, outfile)
