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
Handlers read the fields they use with IR_FIELD_A(this_instr) and so on,
from the word in memory or from ir_word, which dispatch loads, depending on
the build (Python/ceval_macros.h).

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
Such a handler has a NOFB form (LOAD_ATTR_NOFB, ...), with the fields placed
as if it had no entry, for the sites past the IR_FIELD_MAX-th, which share
entry 0 and never specialise. BINARY_OP has one handler per operation instead
(BINARY_OP_ADD, ...), each with its operation as a constant, and its
specialisations deoptimise to the handler of the operation that the entry
records.

Invariants:

- The handler index of an instruction with an opcode is its opcode. IR-only
  handlers (annotation `ir_only`) follow the pseudo-instructions, then the
  handlers of the
  BINARY_OP operations in the order of their NB_ constants, then the NOFB
  forms in name order.
- Members of a family share the format of the family, and INSTRUMENTED_X that
  of X, so specialisation and instrumentation rewrite only the handler field.
  INSTRUMENTED_LINE and INSTRUMENTED_INSTRUCTION take no format: they run
  another handler on the same word.
- The temporary at depth d lives in slot nlocalsplus + d, where the stack
  interpreter keeps it, and handlers keep stack_pointer. A register field
  equals that canonical slot unless it names a local whose LOAD_FAST_BORROW
  a fold absorbed, which debug builds check at every dispatch; the assembler
  folds no load yet. Handlers read an input through its field only where the
  family lets a fold absorb a LOAD_FAST_BORROW, that is where every member
  converts the input to
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
from collections.abc import Mapping
from dataclasses import dataclass, replace
from enum import IntFlag
from types import MappingProxyType

from analyzer import (
    Analysis,
    CodeSection,
    Instruction,
    Family,
    PseudoInstruction,
    Uop,
    Skip,
    Flush,
    StackItem,
    analysis_error,
    analyze_files,
)
from parser import SimpleStmt
from generators_common import (
    DEFAULT_INPUT,
    ROOT,
    Emitter,
    box_kind,
    ReplacementFunctionType,
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

# The fields of the word, and the C expressions that read them in a handler
# (Python/ceval_macros.h), from this_instr, which every handler defines.
FIELDS = ("A", "B", "C", "D")
FIELD_VARS = {f: f"IR_FIELD_{f}(this_instr)" for f in FIELDS}

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
    transparent: bool = False       # see is_transparent()
    # What place_fields() placed, from which the NOFB form follows
    slot_roles: tuple[str, ...] = ()
    imm_kind: str | None = None
    store_dst: bool = False         # dst may name a local, see
                                    # stores_to_dst()
    runs_code: bool = False         # a member may run Python code, see
                                    # runs_code()

    def field_of(self, role: str) -> str | None:
        for name, r in zip(FIELDS, self.roles):
            if r == role:
                return name
        return None

    def input_field(self, k: int) -> str:
        return FIELDS[self.roles.index(f"{ROLE_IN0} + {k}")]

    def borrowed_fields(self) -> list[str]:
        "The C expressions of the fields of the inputs that may be borrowed"
        return [FIELD_VARS[self.input_field(k)] for k in range(self.n_inputs)
                if self.borrow_mask & (1 << k)]


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
                stack.append(Entry(-1, item))
    n = len(external)
    inputs = list(reversed(external))
    outputs = [
        Entry(n - 1 - e.external if e.external >= 0 else -1, e.item)
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


def uop_runs_code(uop: Uop) -> bool:
    """Whether uop may run Python code: an escaping call, which
    call_effects.py defines as the reentrant kind, the eval breaker, or a
    frame switch, which pushes a frame or goes back to one.  A specializer
    (annotation `specializing`) inspects types and rewrites the handler
    and runs none; when it specialises, it dispatches the same word again,
    which reads every field anew."""
    if "specializing" in uop.annotations:
        return False
    p = uop.properties
    return p.escapes or p.eval_breaker or p.switches_frame or p.returns


def runs_code(inst: Instruction) -> bool:
    """Whether inst may run Python code (uop_runs_code()), or is an
    instrumented twin, which fires a monitoring event and so calls a Python
    callback: INSTRUMENTED_JUMP() hides that call from the escape analysis."""
    if inst.name.startswith("INSTRUMENTED_"):
        return True
    return any(uop_runs_code(part) for part in inst.parts
               if isinstance(part, Uop))


def inputs_read_after_code(inst: Instruction, n_inputs: int) -> set[int]:
    """The inputs of inst, bottom first, that the handler first reads after
    a uop that may run Python code (uop_runs_code()).  The handler reads an
    input's field where the first uop that uses it takes it; a uop that
    takes it unused, as a peek that passes it through, reads nothing
    (Stack.pop()).  A local folded into such a field would be read after
    the code ran."""
    late: set[int] = set()
    # Per stack entry: the external index (bottom first) or -1, and
    # whether the handler has read it
    stack: list[tuple[int, bool]] = []
    depth = n_inputs        # external inputs not yet reached
    ran = False
    for part in inst.parts:
        if not isinstance(part, Uop):
            continue
        popped: list[tuple[int, bool]] = []     # top first
        for item in reversed(part.stack.inputs):
            if stack:
                ext, read = stack.pop()
            else:
                depth -= 1
                ext, read = depth, False
            if ext >= 0 and item.used and not read:
                read = True
                if ran:
                    late.add(ext)
            popped.append((ext, read))
        popped.reverse()
        for j in range(len(part.stack.outputs)):
            i = passed_through(part, j)
            stack.append(popped[i] if i is not None else (-1, True))
        ran = ran or uop_runs_code(part)
    return late


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


NOFB_SUFFIX = "_NOFB"


def nofb_bases(analysis: Analysis) -> list[str]:
    """The handlers that have a NOFB form: the generic and instrumented
    handlers that take their oparg from their feedback entry.  A site past
    the IR_FIELD_MAX-th shares entry 0, which holds no oparg, and runs the
    NOFB form, which keeps it in the word.  The handlers of the BINARY_OP
    operations have their operation as a constant and need none."""
    formats = base_formats(analysis)
    return sorted(
        name for name, inst in all_instructions(analysis).items()
        if name != BINARY_OP_FAMILY and inst.properties.oparg
        and not is_specialization(inst)
        and formats[name].imm == LOC_FBARG)


class HandlerKind(IntFlag):
    SPECIALIZED = 1
    INSTRUMENTED = 2
    BINARY_OP = 4
    IR_ONLY = 8
    NOFB = 16
    PSEUDO = 32


def check_handler_count(count: int) -> None:
    if not 0 < count <= 1 << 16:
        raise ValueError(f"handler count {count} does not fit the 16-bit domain")


@dataclass(frozen=True)
class HandlerDomain:
    """One numeric product shared by all generators for a symbolic analysis.

    The initial layout retains the existing pseudo positions and thresholds.
    They move only after every runtime consumer supports the full ID width.
    """
    analysis: Analysis
    opmap: Mapping[str, int]
    handlers: Mapping[str, int]
    bases: Mapping[str, str]
    kinds: Mapping[str, HandlerKind]
    have_arg: int
    min_instrumented: int
    first_ir_only: int
    count: int

    def __post_init__(self) -> None:
        check_handler_count(self.count)
        for field in ("opmap", "handlers", "bases", "kinds"):
            object.__setattr__(self, field,
                               MappingProxyType(dict(getattr(self, field))))
        ids = dict(self.opmap)
        for name, handler in self.handlers.items():
            if name in ids and ids[name] != handler:
                raise ValueError(f"conflicting handler ID for {name}")
            ids[name] = handler
        if len(set(ids.values())) != len(ids):
            raise ValueError("duplicate handler ID")
        if any(not 0 <= handler < self.count for handler in ids.values()):
            raise ValueError("handler ID outside domain")
        if set(self.bases) != set(ids) or set(self.kinds) != set(ids):
            raise ValueError("incomplete handler base/kind metadata")
        for name, base in self.bases.items():
            if base not in ids or self.bases[base] != base:
                raise ValueError(f"invalid handler base for {name}: {base}")

    def require_analysis(self, analysis: Analysis) -> None:
        if analysis is not self.analysis:
            raise ValueError("handler domain belongs to a different analysis")

    def shadow(self, name: str) -> str:
        # The legacy format shadow normalizes IR variants, but leaves
        # specializations to the opcode deopt table. Preserve emitted bytes.
        if self.kinds[name] & (HandlerKind.BINARY_OP | HandlerKind.NOFB):
            return self.bases[name]
        return name


def _assign_opcodes(
    instructions: dict[str, Instruction],
    families: dict[str, Family],
    pseudos: dict[str, PseudoInstruction],
) -> tuple[dict[str, int], int, int]:
    """Assigns opcodes, then returns the opmap,
    have_arg and min_instrumented values"""
    instmap: dict[str, int] = {}

    # 0 is reserved for cache entries. This helps debugging.
    instmap["CACHE"] = 0

    # 17 is reserved as it is the initial value for the specializing counter.
    # This helps catch cases where we attempt to execute a cache.
    instmap["RESERVED"] = 17

    # 128 is RESUME - it is hard coded as such in Tools/build/deepfreeze.py
    instmap["RESUME"] = 128

    # This is an historical oddity.
    instmap["BINARY_OP_INPLACE_ADD_UNICODE"] = 3

    # A reserved slot exists even when a small DSL has no body for it.
    pinned_instrumented = {"INSTRUMENTED_LINE": 255}
    instmap.update(pinned_instrumented)

    instrumented = [name for name in instructions if name.startswith("INSTRUMENTED")]

    specialized: set[str] = set()
    no_arg: list[str] = []
    has_arg: list[str] = []

    for family in families.values():
        specialized.update(inst.name for inst in family.members)

    for inst in instructions.values():
        name = inst.name
        if name in specialized:
            continue
        if name in instrumented:
            continue
        if inst.properties.oparg:
            has_arg.append(name)
        else:
            no_arg.append(name)

    # Specialized ops appear in their own section, and those that do not fit
    # take the numbers below RESUME that no other instruction uses.
    # Instrumented opcodes are at the end of the valid range
    min_internal = instmap["RESUME"] + 1
    min_instrumented = 256 - len(set(instrumented) | pinned_instrumented.keys())

    next_opcode = 1

    def add_instruction(name: str) -> None:
        nonlocal next_opcode
        if name in instmap:
            return  # Pre-defined name
        while next_opcode in instmap.values():
            next_opcode += 1
        instmap[name] = next_opcode
        next_opcode += 1

    for name in sorted(no_arg):
        add_instruction(name)
    for name in sorted(has_arg):
        add_instruction(name)
    # For compatibility
    next_opcode = min_internal
    for name in sorted(specialized):
        if next_opcode == min_instrumented:
            next_opcode = 1
        add_instruction(name)
        assert instmap[name] < min_instrumented
    next_opcode = min_instrumented
    for name in instrumented:
        add_instruction(name)

    for op, name in enumerate(sorted(pseudos), 256):
        instmap[name] = op

    return instmap, len(no_arg), min_instrumented


def build_handler_domain(analysis: Analysis) -> HandlerDomain:
    opmap, have_arg, min_instrumented = _assign_opcodes(
        analysis.instructions, analysis.families, analysis.pseudos)
    first = 1 + max((opmap[name] for name in analysis.pseudos), default=255)
    handlers = {name: opmap[name] for name in analysis.instructions
                if name != BINARY_OP_FAMILY}
    binary = binary_op_handlers(analysis)
    nofb = nofb_bases(analysis)
    derived = (sorted(analysis.ir_only) + binary
               + [name + NOFB_SUFFIX for name in nofb])
    for handler, name in enumerate(derived, first):
        if name in opmap or name in handlers:
            raise ValueError(f"duplicate handler name: {name}")
        handlers[name] = handler
    ids = {**opmap, **handlers}
    bases = {name: name for name in ids}
    kinds = {name: HandlerKind(0) for name in ids}
    for name, inst in analysis.instructions.items():
        if is_specialization(inst):
            assert inst.family is not None
            bases[name] = inst.family.name
            kinds[name] |= HandlerKind.SPECIALIZED
        if name.startswith("INSTRUMENTED"):
            kinds[name] |= HandlerKind.INSTRUMENTED
    for name in analysis.pseudos:
        kinds[name] |= HandlerKind.PSEUDO
    for name in analysis.ir_only:
        kinds[name] |= HandlerKind.IR_ONLY
    if "IR_JUMP" in analysis.ir_only:
        bases["IR_JUMP"] = "JUMP_FORWARD"
    for name in binary:
        bases[name] = BINARY_OP_FAMILY
        kinds[name] |= HandlerKind.BINARY_OP
    for name in nofb:
        variant = name + NOFB_SUFFIX
        bases[variant] = bases[name]
        kinds[variant] = kinds[name] | HandlerKind.NOFB
    return HandlerDomain(analysis, opmap, handlers, bases, kinds,
                         have_arg, min_instrumented, first, first + len(derived))


def handler_instructions(analysis: Analysis,
                         domain: HandlerDomain) -> dict[str, Instruction]:
    "The instruction that each handler runs, by handler name"
    domain.require_analysis(analysis)
    insts = all_instructions(analysis)
    result = {name: insts[name] for name in domain.handlers if name in insts}
    for name in binary_op_handlers(analysis):
        result[name] = insts[BINARY_OP_FAMILY]
    for name in nofb_bases(analysis):
        result[name + NOFB_SUFFIX] = insts[name]
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


def result_offset(inst: Instruction, outputs: list[Entry]) -> int | None:
    """window_dst(), or for a window-style instruction that leaves its result
    in the slot of an input, the offset of that slot: the output passes the
    input through, and a uop reassigns it other than from an input (as
    _CALL_BUILTIN_FAST puts its result where the callable was)."""
    j = window_dst(outputs)
    if j is not None:
        return j
    for j, e in enumerate(outputs):
        if e.item.is_array():
            return None
        name = e.item.name
        if any(assigns_to(part, name) and name not in part.aliased_outputs
               and any(i.name == name for i in part.stack.inputs)
               for part in inst.parts if isinstance(part, Uop)):
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
    # follow dst. Members that push a frame and no outputs write none, and
    # members that return write theirs to the caller's frame.
    new_outputs = [
        None if m.properties.returns
        else first_new_output(io[m.name][1], len(io[m.name][0]))
        for m in members
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
    slot_roles = tuple(slots)
    for m, m_dst in zip(members, new_outputs):
        if (not window and m_dst is not None and m.properties.switches_frame
                and not m.properties.returns):
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
        late: set[int] = set()
        for m in members:
            m_inputs, m_outputs = io[m.name]
            shift = n_inputs - len(m_inputs)
            uses.append(["unused"] * shift + classify_inputs(m, len(m_inputs)))
            passthrough |= {e.external + shift for e in m_outputs if e.external >= 0}
            late |= {k + shift for k in inputs_read_after_code(m, len(m_inputs))}
        for k in range(n_inputs):
            if k in passthrough or k in late:
                continue
            # Every member reads the input, so that its canonical slot
            # holds a value whenever code that reads the stack runs, and
            # reads it before any code runs, so that a folded local is
            # read when its load would have run.
            if all(u[k] in ("object", "raw") for u in uses):
                borrow_mask |= 1 << k
    store_dst = stores_to_dst(members, io, dst_offset, window)
    return IRFormat(
        group=group, window=window, roles=roles, inputs=names,
        n_inputs=n_inputs, dst_offset=dst_offset, borrow_mask=borrow_mask,
        imm=imm, fb=fb, jumps=jumps and not forward, forward=forward,
        slot_roles=slot_roles, imm_kind=imm_kind,
        store_dst=store_dst, runs_code=any(runs_code(m) for m in members),
    )


STORE_RESULT = "store"     # moves the result to the local dst names
STORE_RETURN = "return"    # pushes a frame, which returns the result to the
                           # slot that dst names, a local or a temporary
STORE_LEAVE = "leave"      # leaves to the family head if dst names a local
STORE_NEVER = "never"      # pushes more than the result, so dst never names
                           # a local


def store_mode(inst: Instruction, outputs: list[Entry]) -> str | None:
    """How inst, a member of a family whose dst may name a local, treats
    such a dst; None if it cannot.  The result must be the first new output
    of inst; it may be a copy of an input (`out = in;`).  Members of a
    family share their stack effect for each oparg, and the assembler folds
    a store only where the instruction pushes the result alone, so a member
    that always pushes more never sees a local dst.  The result of a frame
    that a member pushes arrives only when the callee returns, in the slot
    of frame->return_slot, which releases a local's old value
    (_PyFrame_SetReturnValue).  A member that always pushes a frame and
    names dst as its return slot needs nothing more; one that may also
    produce the result itself moves it.  A member whose frames return
    elsewhere leaves to the head."""
    if inst.properties.switches_frame:
        if not names_return_slot(inst):
            return STORE_LEAVE
        if pushes_always(inst):
            return STORE_RETURN
    j = result_offset(inst, outputs)
    if j is None:
        return None
    if all(e.item.is_array() and not e.item.size.isdigit()
           for e in outputs[j + 1:]):
        return STORE_RESULT
    return STORE_NEVER


def uop_tokens(inst: Instruction) -> list[str]:
    return [t.text for part in inst.parts if isinstance(part, Uop)
            for t in part.body.tokens()]


def names_return_slot(inst: Instruction) -> bool:
    """Whether the frames that inst pushes hand dst only what a store of the
    result into a local should see.  A generator's frame that a SEND resumes
    also hands dst each value the generator yields, on its way out of the
    SEND's own frame, which a local must not see.  One that a FOR_ITER
    resumes hands dst only the values it yields, each the loop target's
    next value, and returns its return value to the slot after the
    iterator's window, which END_FOR consumes
    (_PyEval_GeneratorReturnTarget())."""
    tokens = uop_tokens(inst)
    if "IR_DST_SLOT" not in tokens:
        return False
    return ("gi_iframe" not in tokens
            or (inst.family is not None and inst.family.name == "FOR_ITER"))


def pushes_always(inst: Instruction) -> bool:
    """Whether inst ends by pushing a frame, rather than pushing one only on
    some paths with DISPATCH_INLINED"""
    return "DISPATCH_INLINED" not in uop_tokens(inst)


def stores_to_dst(members: list[Instruction],
                  io: dict[str, tuple[list[Entry], list[Entry]]],
                  dst_offset: int | None, window: bool) -> bool:
    """Whether the assembler may fold a STORE_FAST of the result into the
    family's words, so that dst names the local (see store_mode())."""
    if dst_offset is None:
        return False
    head = members[0]
    if head.properties.switches_frame and (
            store_mode(head, io[head.name][1]) != STORE_RESULT):
        return False
    for m in members:
        inputs, outputs = io[m.name]
        mode = store_mode(m, outputs)
        if mode is None:
            return False
        if mode in (STORE_LEAVE, STORE_RETURN):
            continue
        offset = result_offset(m, outputs) if window else first_new_output(
            outputs, len(inputs))
        if offset != dst_offset:
            return False
    return True


def nofb_format(fmt: IRFormat) -> IRFormat:
    "The format of the NOFB form of a handler of format fmt"
    placed = place_fields(list(fmt.slot_roles), False, fmt.imm_kind, fmt.window)
    assert placed is not None, fmt.group
    roles, imm, fb = placed
    return replace(fmt, roles=roles, imm=imm, fb=fb)


def compute_formats(analysis: Analysis) -> dict[str, IRFormat]:
    "The format of every handler and instruction, by name"
    formats = base_formats(analysis)
    for name in nofb_bases(analysis):
        formats[name + NOFB_SUFFIX] = nofb_format(formats[name])
    return formats


def base_formats(analysis: Analysis) -> dict[str, IRFormat]:
    "The formats of compute_formats() but those of the NOFB forms"
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
        return "IR_FIELD_CD(this_instr)"
    return FIELD_VARS[loc.removeprefix("_PyIR_LOC_")]


def is_specialization(inst: Instruction) -> bool:
    "Whether inst is a specialised member of a family, not its head"
    return inst.family is not None


@dataclass
class SlotLayout:
    """Where a handler keeps the positions of the stack effect, the offsets
    from the top of the stack before the instruction that Stack tracks.
    There is no stack pointer.  A window-style handler addresses position p
    as window[p - base], from the slot that its base field names.  A
    register-style handler reads input k through its field and writes its
    outputs at dst and the slots after it."""
    window: bool
    base: PointerOffset             # window: position of the deepest input
    # Register style: the field of the input at each position, the
    # positions whose field may name a local, and dst
    in_fields: dict[int, str]
    borrowable: frozenset[int]
    dst: str | None = None
    dst_offset: int | None = None   # register: position of dst; window:
                                    # its offset from base
    store_dst: bool = False         # dst may name a local

    @staticmethod
    def empty() -> "SlotLayout":
        return SlotLayout(False, PointerOffset.zero(), {}, frozenset())


def may_be_positive(diff: PointerOffset) -> bool:
    "Whether diff can be above zero; symbolic sizes are never negative"
    d = diff.as_int()
    if d is not None:
        return d > 0
    return bool(diff.positive) or diff.numeric > 0


class IRStack(Stack):
    """The frame's slots in place of the stack, the SlotStack of the design
    note.  Every slot holds a value that the frame owns or NULL: a handler
    that consumes a temporary takes its reference and stores NULL in its
    slot, and a slot that a handler writes for a temporary holds NULL.  An
    input field that names a local, a cell or a free variable (a slot below
    co_nlocalsplus) is borrowed: its slot is never cleared or written.

    Positions that the stack gives up, below the old and above the new top,
    are cleared.  In register style the value at a position lives in the
    slot of its input (its home) until the handler writes another value
    there, which goes to the slot counted from dst."""

    def __init__(self, layout: SlotLayout | None = None) -> None:
        super().__init__()
        self.layout = layout if layout is not None else SlotLayout.empty()
        # Register style: where the frame holds the value at a constant
        # position, "in" the slot of its input or "out" the slot from dst.
        # Positions that are not constant are outputs.
        self.owner: dict[int, str] = {}
        if not self.layout.window:
            self.owner = {p: "in" for p in self.layout.in_fields}
        # Set for the flush at the end of the handler, where a dst that
        # names a local is written; and after the frame changed, when no
        # position may be written.
        self.final = False
        self.reloaded = False
        self.error_exit = False
        # The handler's body stores what it hands to the frame it switches
        # to before the switch (`stores_early`).
        self.stores_early = False
        # Set at SYNC_SP(), SAVE_STACK() and RELOAD_STACK(), where the
        # frame's slots must be as the stack is, as at the end.
        self.syncing = False
        # Set by IR_WINDOW_TAKEN(): the handler disposed of its window, so
        # that the slots of the dead inputs stay as it left them.
        self.window_taken = False

    def copy(self) -> "IRStack":
        other = super().copy()
        assert isinstance(other, IRStack)
        other.owner = dict(self.owner)
        other.window_taken = self.window_taken
        other.stores_early = self.stores_early
        return other

    def input_field(self, p: int) -> str:
        "The C expression of the field of the input at position p"
        return FIELD_VARS[self.layout.in_fields[p]]

    def out_slot(self, p: PointerOffset) -> str:
        "The slot of position p counted from dst"
        lay = self.layout
        if lay.dst is None or lay.dst_offset is None:
            raise StackError(f"no slot for stack position {p}")
        k = p - PointerOffset(lay.dst_offset, (), ())
        if k.as_int() is not None and k.as_int() < 0 or k.negative:
            raise StackError(f"no slot for stack position {p}")
        dst = FIELD_VARS[lay.dst]
        return f"IR_SLOT({dst})" if k.as_int() == 0 else f"IR_SLOT({dst} + {k.to_c()})"

    def window_slot(self, p: PointerOffset) -> str:
        return f"window[{(p - self.layout.base).to_c()}]"

    def address(self, memory_offset: PointerOffset) -> str:
        lay = self.layout
        if lay.window:
            return self.window_slot(memory_offset)
        p = memory_offset.as_int()
        if p is not None and self.home_of(p) == "in":
            return f"IR_SLOT({self.input_field(p)})"
        return self.out_slot(memory_offset)

    def home_of(self, p: int) -> str | None:
        "Where the value at constant position p lives"
        lay = self.layout
        owner = self.owner.get(p)
        if owner is not None:
            return owner
        if p in lay.in_fields and (lay.dst_offset is None or p < lay.dst_offset):
            return "in"
        return None

    def pop(self, var: StackItem, out: CWriter) -> Local:
        local = super().pop(var, out)
        lay = self.layout
        if (not lay.window and local.memory_offset is not None and var.used
                and local.ir_input is None and not var.is_array()):
            p = local.memory_offset.as_int()
            if p is not None and self.owner.get(p) == "in":
                local.ir_input = p
                if p in lay.borrowable:
                    local.ir_field = lay.in_fields[p]
        return local

    def consume(self, out: CWriter, p: int) -> None:
        "The frame gives up the input at position p, which may be borrowed"
        f = self.input_field(p)
        if p in self.layout.borrowable:
            out.emit(f"IR_CONSUME_INPUT({f});\n")
        else:
            out.emit(f"IR_SLOT({f}) = PyValue_NULL;\n")

    def store(self, out: CWriter, var: Local, memory_offset: PointerOffset) -> bool:
        """Write var, which the stack holds at memory_offset; whether the
        frame then holds it there"""
        out.start_line()
        if self.reloaded:
            # A value that the instruction hands to the frame it switched
            # to: the slot that frame named when it called or resumed
            if memory_offset.as_int() != 0:
                raise StackError(f"'{var.name}' is written after the frame changed")
            if self.stores_early:
                # The body stored it with IR_RETURN_EARLY()
                out.emit(f"(void){var.name};\n")
            else:
                out.emit(f"IR_STORE_RETURN({var.name});\n")
            return True
        lay = self.layout
        if lay.window:
            out.emit(f"{self.window_slot(memory_offset)} = {var.name};\n")
            return True
        p = memory_offset.as_int()
        if p is not None and var.ir_input == p and self.owner.get(p) == "in":
            # The input itself, back in its slot: a box, or a NULL before
            # the handler closes it
            f = self.input_field(p)
            if p in lay.borrowable:
                out.emit(f"IR_STORE_INPUT({f}, {var.name});\n")
            else:
                out.emit(f"IR_SLOT({f}) = {var.name};\n")
            return True
        if p is not None and (lay.dst_offset is None or p < lay.dst_offset):
            if p in lay.in_fields and p not in lay.borrowable:
                # An input that the instruction passes through, changed
                out.emit(f"IR_SLOT({self.input_field(p)}) = {var.name};\n")
                return True
            return self.keep(var, p)
        if lay.store_dst and p != lay.dst_offset:
            # dst may name a local, so the slots after it are not ours
            return self.keep(var, p)
        previous = self.owner.get(p) if p is not None else "out"
        if previous == "in":
            self.consume(out, p)
            self.owner[p] = "out"
            previous = None
        slot = self.out_slot(memory_offset)
        if lay.store_dst and p == lay.dst_offset:
            if self.final:
                out.emit(f"IR_STORE_DST({FIELD_VARS[lay.dst]}, {var.name});\n")
                return True
            # Before the end, dst gets the result only if it is a
            # temporary; a local gets it at the end.  Across a collecting
            # call the result would be held only in a C local of the eval
            # loop, whose frame is precise and so not scanned.
            if self.collecting:
                raise StackError(
                    f"'{var.name}' is defined before a collecting call, and "
                    "dst may name a local, which takes it only at the end")
            out.emit(f"IR_STORE_TEMP_DST({FIELD_VARS[lay.dst]}, {var.name});\n")
            return False
        if previous == "out":
            out.emit(f"{slot} = {var.name};\n")
        else:
            out.emit(f"IR_STORE_TEMP({slot}, {var.name});\n")
        if p is not None:
            self.owner[p] = "out"
        return True

    def keep(self, var: Local, p: int | None) -> bool:
        """A value at a position without a slot of its own, such as the
        input that a later uop of the handler closes, stays in its C
        variable, which owns it; it needs no slot unless the handler can
        leave with it."""
        if self.final or self.error_exit:
            raise StackError(f"'{var.name}' has no slot at position {p}")
        if self.collecting:
            # The eval loop's C frame is precise: the collector does not
            # scan it, so a value live across the call needs a slot.
            raise StackError(f"'{var.name}' has no slot across a collecting call")
        return False

    def save_variables(self, out: CWriter) -> None:
        out.start_line()
        var_offset = self.base_offset
        for var in self.variables:
            if var.in_local and not var.memory_offset and not var.is_array():
                if self.store(out, var, var_offset):
                    var.memory_offset = var_offset
            var_offset = var_offset.push(var.item)

    def vacate(self, out: CWriter, low: PointerOffset, high: PointerOffset) -> None:
        "Clear the positions from low up to high, which the stack gave up"
        out.start_line()
        lay = self.layout
        count = high - low
        n = count.as_int()
        if lay.window:
            if self.window_taken:
                return
            if n is None:
                out.emit(f"IR_CLEAR_SLOTS(&{self.window_slot(low)}, {count.to_c()});\n")
                return
            for i in range(n):
                out.emit(f"{self.window_slot(low + PointerOffset(i, (), ()))} = PyValue_NULL;\n")
            return
        lo = low.as_int()
        if lo is not None and n is not None:
            for p in range(lo, lo + n):
                owner = self.owner.pop(p, None)
                if owner == "in":
                    self.consume(out, p)
                elif owner == "out":
                    out.emit(f"{self.out_slot(PointerOffset(p, (), ()))} = PyValue_NULL;\n")
            return
        # Outputs whose count is not constant, above the inputs
        if any(p >= (lo if lo is not None else low.numeric) for p in self.owner
               if self.owner[p] == "in"):
            raise StackError(f"cannot clear inputs from {low} to {high}")
        for p in [p for p in self.owner if lo is not None and p >= lo]:
            del self.owner[p]
        out.emit(f"IR_CLEAR_SLOTS(&{self.out_slot(low)}, {count.to_c()});\n")

    def flush(self, out: CWriter) -> None:
        if self.physical_sp is None:
            # After a frame switch there is nothing of this frame to write.
            if self.variables:
                raise StackError("flush while the frame is switching")
            return
        self._print(out)
        above = may_be_positive(self.physical_sp - self.logical_sp)
        # Until the handler leaves, the slots that the stack gave up keep
        # their values: what the handler took from its dead inputs is held
        # there across its collecting calls, since the eval loop's C frame
        # is precise and not scanned.
        leaving = self.final or self.error_exit or self.syncing
        if above and leaving:
            self.vacate(out, self.logical_sp, self.physical_sp)
        self.save_variables(out)
        if leaving or not above:
            self.physical_sp = self.logical_sp
        out.start_line()

    def align(self, other: Stack, out: CWriter) -> None:
        if self.logical_sp != other.logical_sp:
            raise StackError("Cannot align stacks: differing logical top")
        if self.physical_sp is None and other.physical_sp is None:
            return
        if self.physical_sp is None or other.physical_sp is None:
            raise StackError("Cannot align stacks: one side switched frames")
        if self.physical_sp == other.physical_sp:
            return
        if may_be_positive(self.physical_sp - other.physical_sp):
            self.vacate(out, other.physical_sp, self.physical_sp)
        # Otherwise this side has cleared more: the slots it cleared are
        # cleared again later, which does no harm.
        self.physical_sp = other.physical_sp

    def merge(self, other: Stack, out: CWriter) -> None:
        assert isinstance(other, IRStack)
        super().merge(other, out)
        # A side that has not written a position yet writes it later,
        # without the check that the slot is NULL.
        for p in set(self.owner) | set(other.owner):
            mine, theirs = self.owner.get(p), other.owner.get(p)
            if mine == theirs:
                continue
            if "in" in (mine, theirs):
                raise StackError(f"position {p} is an input on one path only")
            self.owner[p] = other.owner[p] = "out"

    # The frame's slots are always current, so an escaping call needs no
    # spill.  Only a frame switch changes `slots`.
    def emit_spill(self, out: CWriter) -> None:
        self.frame_sp = self.physical_sp

    def emit_reload(self, out: CWriter) -> None:
        if self.physical_sp is None:
            if self.frame_sp is None:
                self.frame_sp = PointerOffset.zero()
            self.physical_sp = self.frame_sp

    def emit_fullreload(self, out: CWriter, spilled: bool = True) -> None:
        self._print(out)
        while self.variables:
            top = self.variables.pop()
            if not top.in_memory():
                raise StackError(f"Cannot reload with live variable: {top.name}")
        self.base_offset = PointerOffset.zero()
        self.physical_sp = PointerOffset.zero()
        self.frame_sp = PointerOffset.zero()
        self.logical_sp = PointerOffset.zero()
        self.owner = {}
        self.reloaded = True
        out.emit_str("slots = _PyFrame_Slots(frame);\n")

    def stack_bound_check(self, out: CWriter) -> None:
        pass


class IREmitter(Emitter):
    "Emits handler bodies for the register IR"

    instruction_macros = IR_INSTRUCTION_MACROS

    def __init__(self, out: CWriter, analysis: Analysis,
                 formats: dict[str, IRFormat]):
        super().__init__(out, analysis.labels)
        self.formats = formats
        self._replacers["INSTRUCTION_SIZE"] = self.ir_instruction_size
        self._replacers["DISPATCH_SAME_OPARG"] = self.dispatch_same_oparg
        self._replacers["IR_DST"] = self.ir_dst
        self._replacers["IR_DST_SLOT"] = self.ir_dst_slot
        self._replacers["IR_BASE"] = self.ir_base
        self._replacers["IR_WINDOW_TAKEN"] = self.ir_window_taken
        # The storage at a deopt or exit being emitted
        self.deopt_storage: Storage | None = None
        # The format of the handler being emitted, which for a NOFB form
        # is not its instruction's
        self.handler_format: IRFormat | None = None

    def format_of(self, inst: Instruction) -> IRFormat:
        if self.handler_format is not None:
            return self.handler_format
        return self.formats[inst.name]

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

    def goto_error(self, offset: int, storage: Storage) -> str:
        """Clear the slots that the stack gave up, which the stack machine
        left above its stack pointer, and leave the rest to
        exception_unwind."""
        stack = storage.stack.copy()
        assert isinstance(stack, IRStack)
        stack.error_exit = True
        stack.flush(self.out)
        return f"{self.jump_prefix}JUMP_TO_LABEL(error);"

    def error_no_pop(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        "The inputs stay in their slots; drop what was read from locals"
        self.out.start_line()
        return super().error_no_pop(tkn, tkn_iter, uop, storage, inst)

    def dispatch(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        assert isinstance(storage.stack, IRStack)
        storage.stack.final = True
        return super().dispatch(tkn, tkn_iter, uop, storage, inst)

    def synced(self, replacer: ReplacementFunctionType, tkn: Token,
               tkn_iter: TokenIterator, uop: CodeSection, storage: Storage,
               inst: Instruction | None) -> bool:
        stack = storage.stack
        assert isinstance(stack, IRStack)
        stack.syncing = True
        try:
            return replacer(tkn, tkn_iter, uop, storage, inst)
        finally:
            stack.syncing = False

    def sync_sp(self, tkn: Token, tkn_iter: TokenIterator, uop: CodeSection,
                storage: Storage, inst: Instruction | None) -> bool:
        return self.synced(super().sync_sp, tkn, tkn_iter, uop, storage, inst)

    def save_stack(self, tkn: Token, tkn_iter: TokenIterator, uop: CodeSection,
                   storage: Storage, inst: Instruction | None) -> bool:
        return self.synced(super().save_stack, tkn, tkn_iter, uop, storage, inst)

    def reload_stack(self, tkn: Token, tkn_iter: TokenIterator, uop: CodeSection,
                     storage: Storage, inst: Instruction | None) -> bool:
        return self.synced(super().reload_stack, tkn, tkn_iter, uop, storage, inst)

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

    def ir_dst(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        "The dst field, which may name a local (see store_mode())"
        assert inst is not None
        fmt = self.format_of(inst)
        if not fmt.store_dst:
            raise analysis_error(f"{inst.name}: no dst that names a local", tkn)
        self.emit(FIELD_VARS[fmt.field_of(ROLE_DST)])
        return True

    def ir_dst_slot(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        "The dst field: the slot that receives the result of a frame it pushes"
        assert inst is not None
        fmt = self.format_of(inst)
        field = fmt.field_of(ROLE_DST)
        if field is None:
            raise analysis_error(f"{inst.name} has no dst", tkn)
        self.emit(FIELD_VARS[field])
        return True

    def ir_base(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        "The base field of a window-style instruction"
        assert inst is not None
        fmt = self.format_of(inst)
        field = fmt.field_of(ROLE_BASE)
        if field is None:
            raise analysis_error(f"{inst.name} has no base", tkn)
        self.emit(FIELD_VARS[field])
        return True

    def dispatch_same_oparg(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        "The handler runs again on the same word, and reads its inputs again"
        assert inst is not None
        self.emit(tkn)
        return True

    def emit_jump_to_predicted(self, inst: Instruction | None) -> None:
        assert inst is not None
        fmt = self.format_of(inst)
        binary_op = inst.family is not None and inst.family.name == BINARY_OP_FAMILY
        if binary_op:
            # The handler of the operation that the entry records
            self.emit(f"UPDATE_MISS_STATS({BINARY_OP_FAMILY});\n")
            self.emit(f"IR_DEOPT_BINARY_OP({field_expr(fmt.fb)});\n")
            return
        if inst.family is not None:
            super().emit_jump_to_predicted(inst)
            return
        raise analysis_error(f"{inst.name} deoptimises but has no family",
                             inst.where)

    def ir_window_taken(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        """IR_WINDOW_TAKEN(): a window-style handler has cleared the slots of
        its dead inputs or handed them to the frame it pushes, whose slots
        overlap them; nothing clears them after it."""
        next(tkn_iter)
        next(tkn_iter)
        next(tkn_iter)
        stack = storage.stack
        if not isinstance(stack, IRStack) or not stack.layout.window:
            raise analysis_error("IR_WINDOW_TAKEN() needs a window-style handler", tkn)
        stack.window_taken = True
        return True

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
        self, uop: Uop, storage: Storage, inst: Instruction | None
    ) -> None:
        """Inputs read through a field: check, or box (see
        Emitter.emit_object_inputs())."""
        self.deopt_storage = storage
        try:
            super().emit_object_inputs(uop, storage, inst)
        finally:
            self.deopt_storage = None

    def convert_object_inputs(
        self, uop: Uop, storage: Storage, inst: Instruction | None
    ) -> None:
        for var in storage.inputs:
            if var.ir_field is None or var.known_object:
                continue
            converts = not uop.properties.inline_ok and (
                var.name in uop.object_inputs or var.name in uop.asserted_objects)
            if not converts:
                continue
            if var.name in uop.asserted_objects:
                var.known_object = True
            elif uop.is_guard():
                self.out.start_line()
                self.emit(f"if (Py_UNLIKELY(!_PyValue_IsPointerSpace({var.name}))) {{\n")
                self.emit_jump_to_predicted(inst)
                self.emit("}\n")
                var.known_object = True
            else:
                self.out.start_line()
                self.emit_box(var, uop, storage, inst)
        super().convert_object_inputs(uop, storage, inst)


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
            if cache.name in ("shape", "shape_to"):
                # Shape pointers, borrowed from the owner's type.
                value = f"_PyShape *{cache.name} = fb->{cache.name};"
            elif cache.size == 4 and cache.name.endswith("_value"):
                value = f"PyValue {cache.name} = fb->{cache.name};"
            elif cache.size == 4:
                value = f"PyObject *{cache.name} = (PyObject *)fb->{cache.name};"
            else:
                value = f"uint32_t {cache.name} = fb->{cache.name};"
            emitter.emit(value + "\n")
            if inst.family is None:
                emitter.emit(f"(void){cache.name};\n")
    emitter.emit_object_inputs(uop, storage, inst)
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


def slot_layout(inst: Instruction, fmt: IRFormat, mode: str | None) -> SlotLayout:
    "Where the handler of inst, of format fmt, keeps its stack positions"
    if fmt.transparent:
        return SlotLayout.empty()
    if fmt.window:
        return SlotLayout(True, get_stack_effect(inst).base_offset, {},
                          frozenset())
    in_fields = {k - fmt.n_inputs: fmt.input_field(k)
                 for k in range(fmt.n_inputs)}
    borrowable = frozenset(k - fmt.n_inputs for k in range(fmt.n_inputs)
                           if fmt.borrow_mask & (1 << k))
    return SlotLayout(False, PointerOffset.zero(), in_fields, borrowable,
                      dst=fmt.field_of(ROLE_DST), dst_offset=fmt.dst_offset,
                      store_dst=mode == STORE_RESULT)


def generate_handler(name: str, inst: Instruction, fmt: IRFormat,
                     emitter: IREmitter, out: CWriter) -> None:
    """The handler name, which runs inst: inst itself, or for a handler of a
    BINARY_OP operation, BINARY_OP with that operation."""
    operation = None
    nofb = name == inst.name + NOFB_SUFFIX
    if name != inst.name and not nofb:
        operation = "NB_" + name.removeprefix(BINARY_OP_FAMILY + "_")
    is_target = inst.is_target and name == inst.name
    out.emit("\n")
    out.emit(f"TARGET({name}) {{\n")
    out.emit(f"#if _Py_TAIL_CALL_INTERP\n")
    out.emit(f"int opcode = {name};\n")
    out.emit(f"(void)(opcode);\n")
    out.emit("_PyInterpreterFrame *frame = tstate->current_frame;\n")
    out.emit(f"#endif\n")
    if inst.properties.needs_prev:
        out.emit(f"_PyIRWord* const prev_instr = frame->instr_ptr;\n")
    if not is_target:
        out.emit(f"_PyIRWord* const this_instr = next_instr;\n")
        out.emit("(void)this_instr;\n")
    if not inst.properties.no_save_ip:
        out.emit(f"frame->instr_ptr = next_instr;\n")
    out.emit(f"next_instr += 1;\n")
    out.emit(f"INSTRUCTION_STATS({inst.name});\n")
    if is_target:
        out.emit(f"IR_PREDICTED({name});\n")
        out.emit(f"_PyIRWord* const this_instr = next_instr - 1;\n")
        out.emit("(void)this_instr;\n")
    use_fb = fmt.fb != LOC_NONE and (
        uses_feedback(inst) or reads_fb_arg(inst, fmt, operation))
    if use_fb:
        out.emit(f"{feedback_type(fmt)} *fb = ({feedback_type(fmt)} *)"
                 f"IR_FB({field_expr(fmt.fb)});\n")
    elif nofb and uses_feedback(inst):
        # The shared entry, which never specialises
        out.emit(f"{feedback_type(fmt)} *fb = ({feedback_type(fmt)} *)IR_FB(0);\n")
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
    if fmt.window:
        base = FIELD_VARS[fmt.field_of(ROLE_BASE)]
        out.emit(f"PyValue *window = &IR_SLOT({base});\n")
        out.emit("(void)window;\n")
    if inst.properties.uses_opcode:
        out.emit(f"opcode = {name};\n")
    mode = store_mode(inst, external_io(inst)[1]) if fmt.store_dst else None
    if mode == STORE_LEAVE:
        out.emit(f"if (IR_IS_LOCAL({FIELD_VARS[fmt.field_of(ROLE_DST)]})) {{\n")
        emitter.emit_jump_to_predicted(inst)
        out.emit("}\n")
    elif mode == STORE_NEVER:
        out.emit(f"assert(!IR_IS_LOCAL({FIELD_VARS[fmt.field_of(ROLE_DST)]}));\n")
    declare_variables(inst, out)
    stack: Stack = IRStack(slot_layout(inst, fmt, mode))
    assert isinstance(stack, IRStack)
    stack.stores_early = any(isinstance(p, Uop) and p.properties.stores_early
                             for p in inst.parts)
    insert_braces = len([p for p in inst.parts if isinstance(p, Uop)]) > 1
    reachable = True
    for part in inst.parts:
        try:
            reachable, stack = write_uop(part, emitter, stack, inst, insert_braces)
        except SyntaxError as ex:
            ex.msg = f"{name}: {ex.msg}"
            raise
    out.start_line()
    if reachable:
        assert isinstance(stack, IRStack)
        stack.final = True
        stack.flush(out)
        if mode == STORE_RESULT and fmt.window:
            # The result is in its slot of the window; a local takes it
            dst = FIELD_VARS[fmt.field_of(ROLE_DST)]
            out.emit(f"IR_MOVE_TO_DST({dst}, window[{fmt.dst_offset}]);\n")
        out.emit("DISPATCH();\n")
    out.start_line()
    out.emit("}")
    out.emit("\n")


def generate_ir_cases(analysis: Analysis, domain: HandlerDomain, formats: dict[str, IRFormat],
                      outfile: TextIO, lines: bool) -> None:
    out = CWriter(outfile, 2, lines)
    emitter = IREmitter(out, analysis, formats)
    out.emit("\n")
    for name, inst in sorted(handler_instructions(analysis, domain).items()):
        emitter.handler_format = formats[name]
        generate_handler(name, inst, formats[name], emitter, out)
        emitter.handler_format = None


def generate_ir_labels(analysis: Analysis, emitter: Emitter) -> None:
    emitter.emit("\n")
    for name, label in analysis.labels.items():
        emitter.emit(f"LABEL({name})\n")
        emitter.emit("{\n")
        emitter.emit("TAIL_CALL_LOAD_FRAME();\n")
        storage = Storage(IRStack(), [], [], 0, False)
        if label.spilled:
            storage.spilled = 1
            storage.stack.physical_sp = None
        emitter.emit_tokens(label, storage, None)
        emitter.emit("}\n\n")


def generate_ir(filenames: list[str], analysis: Analysis, domain: HandlerDomain,
                outfile: TextIO, lines: bool) -> None:
    domain.require_analysis(analysis)
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
    generate_ir_cases(analysis, domain, formats, outfile, lines)
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
    return (f"static PyValue Py_GC_PRECISE_PRESERVE_NONE Py_PRESERVE_NONE_CC "
            f"{prefix}{name}(TAIL_CALL_PARAMS)")


def generate_ir_targets(filenames: list[str], analysis: Analysis,
                        domain: HandlerDomain, outfile: TextIO) -> None:
    "The dispatch tables, indexed by handler"
    domain.require_analysis(analysis)
    ids = domain.handlers
    n = domain.count
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
    out.emit("TAIL_CALL_LOAD_FRAME();\n")
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
    domain = build_handler_domain(data)
    with open(outfilename, "w") as outfile:
        generate_ir(filenames, data, domain, outfile, lines)


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
    domain = build_handler_domain(data)
    with open(args.output, "w") as outfile:
        generate_ir(args.input, data, domain, outfile, args.emit_line_directives)
    with open(args.targets, "w") as outfile:
        generate_ir_targets(args.input, data, domain, outfile)
