"""Generate the register IR interpreter.

Reads the instruction definitions from Python/bytecodes.c and writes

- Python/generated_ir_cases.c.h, one handler per instruction, #included in
  ceval.c;
- Python/ir_targets.h, the prototypes of the handlers and labels and the
  table of handlers, indexed by handler.

Design note pyjit/design/register-ir.md describes the IR, and
Include/internal/pycore_ir.h the word layout. In short, an instruction is one
64-bit word: a 16-bit handler index and four 12-bit fields A, B, C and D, of
which C and D also form a 24-bit field CD. A field names a frame slot, the
feedback entry (its index in the block's feedback vector) or the oparg.
Each handler and label is a C function, which dispatches to the next by a
guaranteed tail call (Python/ceval_macros.h).  Handlers read the fields they
use with IR_A(ir_word) and so on, from ir_word, the word that dispatch loads
and passes on.

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
A wide oparg of an instruction with an entry takes CD if two fields are free
and otherwise the one free field, as D (LOAD_ATTR, STORE_ATTR and
LOAD_SUPER_ATTR; the compiler lowers an access whose oparg does not fit,
Python/codegen.c); only an oparg that has no field left stays in the entry
(COMPARE_OP, CONTAINS_OP and BINARY_OP), so that the sites past the
feedback limit, which share entry 0 and never specialise, need no second
handler.  BINARY_OP has one handler per operation (BINARY_OP_ADD, ...), each
with its operation as a constant, and its specialisations deoptimise to the
handler of the operation that the entry records.

Invariants:

- The handler index of an instruction with an opcode is its opcode. IR-only
  handlers (annotation `ir_only`) follow the pseudo-instructions, then the
  handlers of the
  BINARY_OP operations in the order of their NB_ constants.
- Members of a family share the format of the family, and INSTRUMENTED_X that
  of X, so specialisation and instrumentation rewrite only the handler field.
  INSTRUMENTED_LINE and INSTRUMENTED_INSTRUCTION take no format: they run
  another handler on the same word.
- The temporary at depth d lives in slot nlocalsplus + d, where the stack
  interpreter keeps it. Handlers address every slot through their fields and
  keep no stack_pointer. A register field equals that canonical slot unless
  it names a local whose LOAD_FAST_BORROW the assembler folded into the
  reader, which debug builds check at every dispatch. Handlers read an input
  through its field only where the family lets a fold absorb a
  LOAD_FAST_BORROW, that is where every member converts the input to an
  object first and the format has a dst; there the borrow tag is set
  statically after the object check, if the field is not the canonical slot.
  An absorbed load has no word, so a handler writes a borrowed input to its
  canonical slot only where code that reads the stack may run.
- Instruction macros of the DSL (generators_common.STACK_INSTRUCTION_MACROS)
  expand to the IR forms of IR_INSTRUCTION_MACROS.
- A specialisation deoptimises by a tail call to its family head, which
  runs the word again (JUMP_TO_PREDICTED, Python/ceval_macros.h).
"""

import argparse
import ir_numbers
import re
from dataclasses import dataclass, replace

from analyzer import (
    Analysis,
    CALL_TRANSACTION_MARKERS,
    CodeSection,
    Instruction,
    Uop,
    Part,
    Skip,
    Flush,
    StackItem,
    VALUE_TO_OBJECT,
    analysis_error,
    analyze_files,
)
from parser import (BlockStmt, ForStmt, IfStmt, MacroIfStmt, SimpleStmt, Stmt,
                    WhileStmt)
from generators_common import (
    DEFAULT_INPUT,
    NON_ESCAPING_DEALLOCS,
    POINTER_VARIANTS,
    ROOT,
    Emitter,
    TokenIterator,
    enclosing_callee,
    find_local,
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

def first_ir_only_handler(analysis: Analysis) -> int:
    """The first handler of the IR only.  They follow the pseudo-instructions,
    so that a number names one opcode, pseudo-instruction or handler."""
    return analysis.numbers.first_ir_only

# The fields of the word, and the C expressions that read them in a handler
# (Include/internal/pycore_ir.h), from ir_word, which every handler receives.
FIELDS = ("A", "B", "C", "D")
FIELD_VARS = {f: f"IR_{f}(ir_word)" for f in FIELDS}


def owner_name(field: str) -> str:
    """The owner of the borrowable input read through field: the value that
    IR_CONSUME_INPUT gives the handler, PyValue_NULL for a local"""
    return f"ir_own_{field}"


# The release of a view's owner, which is PyValue_NULL for a local, for each
# release of a view and its form for a value known to be in pointer space
VIEW_RELEASES = {
    "PyValue_CLOSE": ("PyValue_XCLOSE", "_PyValue_XCLOSE_POINTER"),
    "PyValue_XCLOSE": ("PyValue_XCLOSE", "_PyValue_XCLOSE_POINTER"),
    "PyValue_CLOSE_SPECIALIZED": ("PyValue_XCLOSE_SPECIALIZED",
                                  "_PyValue_XCLOSE_SPECIALIZED_POINTER"),
}
# The DSL calls that take a view: they give its owner, or a count of its
# own for a local, to the caller (IREmitter.take_view())
VIEW_TAKES = {"TAKE", "OWNER", "PyValue_AsKnownObjectSteal"}
# The DSL calls that read a view's owner, so that the handler must have
# taken the input (IR_CONSUME_INPUT) before the statement that holds them
OWNER_READS = set(VIEW_RELEASES) | VIEW_TAKES

# The IR forms of the instruction macros (see STACK_INSTRUCTION_MACROS).
IR_INSTRUCTION_MACROS = {
    "FB_COUNTER": "((_PyFbHeader *)fb)",
    "FB_BRANCH": "((_PyFbHeader *)fb)->history",
    "FB_TAKEN": "((_PyFbHeader *)fb)->taken",
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
# The roles of an oparg that names a slot, see oparg_slot_role()
ROLE_LOCAL_IN, ROLE_LOCAL_OUT, ROLE_LOCAL_CLEAR, ROLE_LOCAL_MOVE, ROLE_CELL = (
    "_PyIR_ROLE_LOCAL_IN", "_PyIR_ROLE_LOCAL_OUT", "_PyIR_ROLE_LOCAL_CLEAR",
    "_PyIR_ROLE_LOCAL_MOVE", "_PyIR_ROLE_CELL")
LOC_NONE, LOC_A, LOC_B, LOC_C, LOC_D, LOC_CD, LOC_FBARG = (
    "_PyIR_LOC_NONE", "_PyIR_LOC_A", "_PyIR_LOC_B", "_PyIR_LOC_C",
    "_PyIR_LOC_D", "_PyIR_LOC_CD", "_PyIR_LOC_FBARG")
FIELD_LOCS = {"A": LOC_A, "B": LOC_B, "C": LOC_C, "D": LOC_D}

# The families whose oparg is a forward distance in words, in D, rather than
# an absolute target: the loops, whose bodies jump relative to the next word.
FORWARD_JUMPS = ("FOR_ITER", "SEND")

# The IR-only families that fuse a compare with the conditional jump after
# it: the distance is in D, as for the loops, and the entry's arg holds the
# operation and the sense of the jump.  The handlers stand for the compare.
FUSED_JUMPS = ("COMPARE_OP_JUMP", "CONTAINS_OP_JUMP", "IS_OP_JUMP")
FUSED_SHADOWS = {"COMPARE_OP_JUMP": "COMPARE_OP",
                 "CONTAINS_OP_JUMP": "CONTAINS_OP", "IS_OP_JUMP": "IS_OP"}

# The families whose dst field may be IR_NO_DST, so that the word of a call
# whose result is popped at once (CALL; POP_TOP) needs no POP_TOP.  The
# intrinsics are not calls in this sense and keep their POP_TOP.
DISCARDING = ("CALL", "CALL_KW", "CALL_FUNCTION_EX")

# The unpacks: their dst field names the first of a run of locals that
# receive the elements in order (the STORE_FASTs folded in), or is IR_NO_DST.
# The count of elements as a function of the oparg.
UNPACKS = {
    "UNPACK_SEQUENCE": "oparg",
    "UNPACK_EX": "((oparg & 0xFF) + 1 + (oparg >> 8))",
}

# The appends: a window (the container, and the entries above it) whose
# top NSRC inputs are read through fields of their own, so that a value in a
# local is read in place and one in a temporary needs no particular slot.
APPEND_SOURCES = {"LIST_APPEND": 1, "SET_ADD": 1, "MAP_ADD": 2}

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
    fused: bool = False             # see FUSED_JUMPS
    no_fold: bool = False
    transparent: bool = False       # see is_transparent()
    store_dst: bool = False         # dst may name a local, see
                                    # stores_to_dst(); the handlers release
                                    # the local's old value
    runs_code: bool = False         # a member may run Python code, see
                                    # runs_code()
    discards: bool = False          # dst may be IR_NO_DST, see DISCARDING
    unpacks: bool = False           # dst names a run of locals for the
                                    # elements, or IR_NO_DST, see UNPACKS
    # The rest of the instruction contract (pycore_ir.h, _PyIRFormat)
    jump_always: bool = False       # see jumps_always()
    names_target: bool = False      # see names_target_only()
    fb_layout: str | None = None    # the entry type, see FEEDBACK_LAYOUTS
    fb_kind: str | None = None      # its _PyFbKind

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
def declare_variable(var: StackItem, out: CWriter,
                     roles: dict[str, set[str]]) -> None:
    type, null = type_and_null(var)
    if not var.is_array() and roles.get(var.name) == {"view"}:
        type = "PyValueBorrow"
    space = " " if type[-1].isalnum() else ""
    out.emit(f"{type}{space}{var.name};\n")


def declare_variables(inst: Instruction, out: CWriter,
                      roles: dict[str, set[str]] | None = None) -> None:
    """The handler's variables, one per name of its uops' stack items: a
    view is a PyValueBorrow, any other value a PyValue (S8)"""
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
                declare_variable(var, out, roles or {})
        for var in part.stack.outputs:
            if var.used and var.name not in seen:
                seen.add(var.name)
                declare_variable(var, out, roles or {})
        if any(token.text == "IR_INVOKE_CALL" for token in part.body.tokens()):
            break


def uop_runs_code(uop: Uop) -> bool:
    """Whether uop may run Python code: an escaping call, the eval breaker,
    or a frame switch, which pushes a frame or goes back to one.  A
    specializer (annotation `specializing`) inspects types and rewrites the
    handler and runs none; when it specialises, it dispatches the same word
    again, which reads every field anew."""
    if "specializing" in uop.annotations:
        return False
    p = uop.properties
    return p.escapes or p.eval_breaker or p.switches_frame or p.returns


# The identifiers that look like a release (RELEASE_LIKE), each known to
# release a reference whose __del__ may run Python, or not to.  Any other
# such identifier is an error, so that a new release helper cannot pass as
# one that runs nothing.
RELEASE_LIKE = re.compile(r"CLOSE|DECREF|CLEAR|SETREF|RETIRE|Close|Clear|"
                          r"Dealloc|_READER|FINISH_VALUE")
RELEASES = {
    "PyValue_CLOSE", "PyValue_XCLOSE", "PyValue_CLEAR", "_PyValue_CloseStack",
    "_PyValue_CLOSE_POINTER", "_PyValue_XCLOSE_POINTER", "DECREF_INPUTS",
    "Py_DECREF", "Py_XDECREF", "Py_CLEAR", "Py_SETREF", "Py_XSETREF",
    # The value and attribute transactions release the inputs they retire
    "IR_RETIRE_PUBLISHED", "IR_RETIRE_PUBLISHED_RESULT",
    "IR_RETIRE_PUBLISHED_PAIR",
    "IR_FINISH_VALUE_OPERATION", "IR_ABORT_VALUE_READER",
    "IR_COMPLETE_VALUE_READER", "IR_COMPLETE_ATTR_READER",
    "_PyFrame_ClearTemporaries", "_PyEval_FrameClearAndPop", "_PyErr_Clear",
}
NOT_RELEASES = {
    # A release whose destructor, that of an exact type, runs no Python
    "PyValue_CLOSE_SPECIALIZED", "_Py_DECREF_SPECIALIZED",
    "_PyLong_ExactDealloc", "_PyFloat_ExactDealloc",
    "_PyFloat_OrLongExactDealloc", "_PyUnicode_ExactDealloc",
    "_PyFrame_AssertTemporariesClear",      # a debug check
    "_PyGC_CLEAR_ARRAY",                    # expands to nothing
    "LOAD_FAST_AND_CLEAR",                  # an opcode
}


def releases_object(inst: Instruction) -> bool:
    """Whether inst releases a reference that may be an object's last, whose
    __del__ may run Python, whether or not the escape analysis counts the
    release as escaping (PyValue_CLEAR and the IR_* transactions it does
    not)"""
    found = False
    for part in inst.parts:
        if not isinstance(part, Uop):
            continue
        for tkn in part.body.tokens():
            if tkn.kind != "IDENTIFIER" or not RELEASE_LIKE.search(tkn.text):
                continue
            if tkn.text in RELEASES:
                found = True
            elif tkn.text not in NOT_RELEASES:
                raise analysis_error(
                    f"{inst.name}: {tkn.text} looks like a release, which "
                    "ir_generator.py does not know (RELEASES, NOT_RELEASES)",
                    tkn)
    return found


def runs_code(inst: Instruction) -> bool:
    """Whether inst may run Python code (uop_runs_code()), releases an
    object (releases_object()), or is an instrumented twin, which fires a
    monitoring event and so calls a Python callback: INSTRUMENTED_JUMP()
    hides that call from the escape analysis."""
    if inst.name.startswith("INSTRUMENTED_"):
        return True
    return releases_object(inst) or any(
        uop_runs_code(part) for part in inst.parts if isinstance(part, Uop))


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
            if tkn.kind == "IDENTIFIER" and tkn.text in ("FB_COUNTER", "FB_BRANCH", "FB_TAKEN"):
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
        if any(token.text == "IR_INVOKE_CALL" for token in part.body.tokens()):
            # A terminal transaction owns both immediate result publication
            # and the child's eventual return. Trailing legacy frame/poll
            # uops do not execute and cannot erase that logical output.
            break
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


# A value read through a field that may name a local is a view: a
# PyValueBorrow of the slot's bits, which owns no count and which only C
# locals hold (pyjit/design/proposals/register-ir-v2-a5b.md, section 4).
# Its owner is ir_own_<F>, the value that IR_CONSUME_INPUT gives the handler
# for a temporary and PyValue_NULL for a local.  So the generator knows
# each view (Local.ir_inplace) and what a handler does with it.  It may:
# - release it: the release is of the owner (IREmitter.release_view());
# - take it, with TAKE(x) for a home (a count of its own for a local) or
#   OWNER(x) for a receiver that IR_RETIRE_PUBLISHED releases, or steal its
#   object with PyValue_AsKnownObjectSteal (IREmitter.take_view());
# - read it, as an argument of a function that only reads its value
#   arguments (INPLACE_READERS, the PyValue_Is* tests), through `.bits`,
#   or in `(void)name`;
# - copy it to a stack input or output of the uop, which then views the
#   same slot (IREmitter.note_pointer_assignment() follows only those);
# - read its object with PyValue_AsKnownObjectBorrow(), which has no count
#   either: that object, or a variable that it is assigned or cast to, may
#   be read only as OBJECT_READERS allows (object_use_refused()).
# Any other use keeps the input: the format does not let a local reach its
# field (compute_format(), kept_inputs()), and the generator refuses such a
# use of a view (IREmitter.check_inplace_input()).  The C compiler rejects
# a view that reaches an owner, a home or a function that takes PyValue.
INPLACE_READERS = {
    "PyValue_DUP", "PyValue_XDUP", "PyValue_AsObject",
    "PyValue_AsKnownObjectBorrow", "PyValue_TYPE",
    "PyValue_BoolCheck", "PyValue_FloatOperandValues", "PyValue_InlineFloat",
    "PyValue_InlineInt64", "_PyValue_IntScaled", "PyValue_LongCheckExact",
    # The value dispatchers (pycore_value_ops.h): they read their operands
    # as values and take a count of what they keep
    "_PyValue_Contains", "_PyValue_Invert", "_PyValue_IsTrue",
    "_PyValue_Negative", "_PyValue_Positive", "_PyValue_RichCompare",
    "_PyValue_Str", "_PyValue_Repr", "_PyValue_ASCII", "_PyValue_Format",
    "_PyValue_SequenceTuple",
    # Attribute and subscript dispatchers (I4, I3)
    "_PyValue_GetAttr", "_PyValue_SetAttr", "_PyValue_GetMethod",
    "_PySuper_LookupValue", "value_import_level", "super_call",
    "_PyValue_GetItem", "_PyValue_SetItem", "_PyValue_DelItem",
    "_PyValue_Size", "_PyValue_GetOptionalItem", "_PyValue_Send",
    # Monitoring events (pycore_instruments.h): their arguments are
    # borrows for the call
    "_Py_call_instrumentation_arg", "_Py_call_instrumentation_2args",
    "_Py_call_instrumentation_exc2",
    # Value-form container calls: they only read their key argument
    "PyDict_ContainsValue", "PySet_ContainsValue", "PyDict_DelValue",
    "_PyValue_RichCompareObject",
    # Not a reader: PyDict_SetValue keeps counted copies of its key and
    # value (PyValue_XDUP of a borrow), so it neither keeps nor releases the
    # caller's count and a borrowed local may be passed to it
    "PyDict_SetValue",
    # Boxes an inline input into value_temps, which owns the box; the result
    # is followed as an uncounted object (inplace_input_use)
    "_PyValueTemps_Object",
    # Python/ceval.c: they read their value arguments, and the handler
    # releases them after
    "_PyEval_BinarySlice", "_PyEval_StoreName", "_PyEval_StoreSlice",
    "_PyEval_MatchClass",
    # Python/specialize.c: they read types and values
    "_Py_Specialize_BinaryOp", "_Py_Specialize_CompareOp",
    "_Py_Specialize_ContainsOp", "_Py_Specialize_LoadAttr",
    "_Py_Specialize_PopJumpIf", "_Py_Specialize_StoreAttr",
    "_Py_Specialize_StoreSubscr",
}


INPLACE_OBJECT_READERS = {"PyValue_AsKnownObjectBorrow"}
# Functions that take over or release a PyObject * argument: those named
# "Steal" take every argument, those named "Take" and the SET_ITEM setters
# every argument but the first, their target
OBJECT_RELEASES = {"Py_DECREF", "Py_XDECREF", "Py_CLEAR", "Py_SETREF",
                   "Py_XSETREF", "Py_DecRef", "Py_XDecRef"}
# The functions, macros and statements that may receive the object of a
# view (S3, register-ir-v2-a5b.md 4.7): each reads the object while the
# handler runs and takes a count of its own if it keeps it.  Any other
# callee of such a pointer, a store of it into a field, an array element or
# the result of `?:`, or a return of it keeps the input out of borrow_mask:
# a view's object has no count to keep it alive.  test_generated_cases
# checks that each name receives the object of a view in bytecodes.c
# (OBJECT_READERS_SEEN); that a name only reads is checked by review.
# Uses inside assert() vanish from release builds and are not checked.
OBJECT_READERS = {
    # Statements
    "LOCK_OBJECT", "UNLOCK_OBJECT",
    # Type tests and accessors
    "Py_TYPE", "PyAnyDict_CheckExact", "PyAnySet_CheckExact",
    "PyCoro_CheckExact", "PyGen_Check",
    "PyList_CheckExact", "PyModule_CheckExact", "PySlice_Check",
    "PyTuple_CheckExact", "PyType_Check", "PyUnicode_CheckExact",
    "PyList_GET_SIZE", "PyList_GET_VALUE", "PyTuple_GET_SIZE",
    "PyTuple_GET_VALUE", "PyUnicode_1BYTE_DATA", "PyUnicode_GET_LENGTH",
    "PyUnicode_IS_COMPACT_ASCII", "PyUnicode_READ_CHAR", "_PyList_ITEMS",
    "_PyLong_IsZero", "_PyObject_GetManagedDict", "_PyObject_InlineValues",
    "_PyObject_Shape",
    # They take the index of a value slot, an address inside the object
    # less the start of its values
    "_PyShape_TransitionMissing", "_PyDictValues_AddToInsertionOrder",
    # Operations that read their operands and count what they keep
    "PyErr_GivenExceptionMatches",
    # The value-form container calls (they read the container object and
    # the key as a value)
    "PyDict_ContainsValue", "PySet_ContainsValue",
    "PyFunction_New",
    "PyUnicode_Concat",

    "_PyDict_SetValue_Take2", "_PyEval_CheckExceptStarTypeValid",
    "_PyEval_CheckExceptTypeValid", "value_exception_group_match",
    "_PyEval_GetAwaitable", "_PyList_SliceSubscript",
    "value_load_from_dict", "_PySet_Contains", "_PyTemplate_Build",
    "_PyUnicode_Equal",
    "monitor_stop_iteration",
    # The indirect calls of BINARY_OP_EXTEND and
    # CALL_INTRINSIC_*, through tables of functions that read their
    # operands, and of GET_AITER, through the type's am_aiter slot
    "action", "func", "getter",
}
# The names of OBJECT_READERS that S3 let receive the object of a view,
# collected while this is a set
OBJECT_READERS_SEEN: set[str] | None = None


def call_argument(tokens: list[Token], i: int) -> tuple[str | None, int]:
    """The function whose argument list holds tokens[i] directly, and the
    position of that argument; None if tokens[i] is not in one"""
    depth = 0
    position = 0
    for j in range(i - 1, -1, -1):
        text = tokens[j].text
        if text == ")":
            depth += 1
        elif text == "(":
            if depth == 0:
                if j > 0 and tokens[j - 1].kind == "IDENTIFIER":
                    return tokens[j - 1].text, position
                if j >= 4 and [t.text for t in tokens[j - 4:j]] == \
                        ["(", "*", tokens[j - 2].text, ")"] \
                        and tokens[j - 2].kind == "IDENTIFIER":
                    return tokens[j - 2].text, position  # (*fn)(...)
                return None, 0
            depth -= 1
        elif text == "," and depth == 0:
            position += 1
        elif text in (";", "{", "}") and depth == 0:
            return None, 0
    return None, 0


def consumed_argument(tokens: list[Token], i: int) -> bool:
    "Whether a function takes over or releases its argument at tokens[i]"
    callee, position = call_argument(tokens, i)
    if callee is None:
        return False
    if "Steal" in callee or callee in OBJECT_RELEASES:
        return True
    if "Take" in callee or callee.endswith("_SET_ITEM"):
        return position > 0
    return False


STATEMENT_START = (";", "{", "}")
C_TYPE_WORDS = {"char", "const", "struct", "union", "enum", "unsigned",
                "signed", "int", "long", "short", "void", "double", "float",
                "volatile"}


def is_word(tkn: Token) -> bool:
    "An identifier or a C type keyword, which the lexer gives its own kind"
    return tkn.kind == "IDENTIFIER" or tkn.text in C_TYPE_WORDS


def declared_or_assigned(tokens: list[Token], j: int) -> str | None:
    """The plain local that tokens[..j] names as the target of the `=` at
    tokens[j]: `name =` as a statement, or `T *name =` / `T name =` as a
    declaration; None for anything else (S3 fails closed): a field, an
    array element, `*p`, or the inner target of a chained assignment"""
    if j < 1 or tokens[j].text != "=" or tokens[j - 1].kind != "IDENTIFIER":
        return None
    k = j - 2
    if k < 0 or tokens[k].text in STATEMENT_START:
        return tokens[j - 1].text  # A statement: `name = ...;`
    # A declaration: type words, then pointer stars, from a statement start
    while k >= 0 and tokens[k].text == "*":
        k -= 1
    words = 0
    while k >= 0 and is_word(tokens[k]):
        words += 1
        k -= 1
    if words == 0 or not (k < 0 or tokens[k].text in STATEMENT_START):
        return None
    return tokens[j - 1].text


def assigned_name(tokens: list[Token], start: int, end: int) -> str | None:
    """The plain local that `name = (cast) tokens[start..end];` assigns, as a
    statement or a declaration (the cast is optional), or None
    (declared_or_assigned())"""
    if end + 1 >= len(tokens) or tokens[end + 1].text != ";":
        return None
    j = start - 1
    if j >= 0 and tokens[j].text == ")":
        while j >= 0 and tokens[j].text != "(":
            j -= 1
        j -= 1
    return declared_or_assigned(tokens, j)


def assignment_bounds(tokens: list[Token], start: int,
                      end: int) -> tuple[int, int] | None:
    """(the `=`, the `;`) of a statement `target = <expression>;` in whose
    expression tokens[start..end] stands outside any call, or None"""
    depth = 0
    groups = 0  # Parentheses that enclose tokens[start..end]
    j = start - 1
    while j >= 0:
        text = tokens[j].text
        if text == ")":
            depth += 1
        elif text == "(":
            if depth == 0:
                if j > 0 and tokens[j - 1].kind == "IDENTIFIER" or j >= 4 and \
                        [x.text for x in tokens[j - 4:j]] == \
                        ["(", "*", tokens[j - 2].text, ")"]:
                    return None  # An argument of a call, or of (*fn)(...)
                groups += 1
            else:
                depth -= 1
        elif depth == 0 and (text == "=" or text in STATEMENT_START
                             or text in (",", "?", ":", "return")):
            break
        j -= 1
    if j < 0 or tokens[j].text != "=":
        return None
    k = end + 1
    depth = 0
    while k < len(tokens):
        text = tokens[k].text
        if text == "(":
            depth += 1
        elif text == ")":
            if depth == 0:
                if groups == 0:
                    return None
                groups -= 1
            else:
                depth -= 1
        elif depth == 0 and text in (";", ",", "?", ":", "="):
            break
        k += 1
    if k >= len(tokens) or tokens[k].text != ";":
        return None
    return j, k


def derived_name(tokens: list[Token], start: int, end: int) -> str | None:
    """The plain local that a statement `name = <expression>;` assigns, where
    tokens[start..end], the object of a view, is in the expression outside
    any call and the expression is address arithmetic: an address derived
    from the object, which is followed as the object is.  None if the
    statement is not of that form."""
    bounds = assignment_bounds(tokens, start, end)
    if bounds is None:
        return None
    j, k = bounds
    if tokens[j + 1].text == "*":
        return None  # A load through the address: a field read
    if any(tokens[m].text not in ("+", "-", "*", "(", ")")
           and not is_word(tokens[m]) and tokens[m].kind != "NUMBER"
           for m in range(j + 1, k)):
        return None  # Not address arithmetic: a comparison, for example
    return declared_or_assigned(tokens, j)


# Integer types: a declaration of one from an address inside an object
# (a pointer difference) holds no pointer
INTEGER_TYPES = {"Py_ssize_t", "size_t", "ptrdiff_t", "intptr_t", "uintptr_t",
                 "int", "long"}


def integer_declaration(tokens: list[Token], start: int, end: int) -> bool:
    "Whether tokens[start..end] is in the initializer of an integer local"
    bounds = assignment_bounds(tokens, start, end)
    if bounds is None:
        return False
    j, _ = bounds
    return (j >= 3 and tokens[j - 1].kind == "IDENTIFIER"
            and tokens[j - 2].text in INTEGER_TYPES
            and tokens[j - 3].text in STATEMENT_START)


def dereferenced(tokens: list[Token], start: int, end: int) -> bool:
    """Whether tokens[start..end], an address inside an object, is
    dereferenced, `*(T *)(address [+ offset])` or `*address`: a load or
    store of a field, as `->` is"""
    k = start - 1
    while k >= 0:
        if tokens[k].text == "(":
            k -= 1  # A grouping parenthesis
            continue
        if tokens[k].text == ")":
            # A cast: `(T *)`
            m = k - 1
            while m >= 0 and tokens[m].text != "(":
                if tokens[m].text != "*" and not is_word(tokens[m]):
                    return False
                m -= 1
            k = m - 1
            continue
        break
    if k < 0 or tokens[k].text != "*":
        return False
    before = tokens[k - 1].text if k > 0 else ";"
    return before in STATEMENT_START or before in ("=", "(", ",", "return", "!")


def object_use_allowed(tokens: list[Token], start: int, end: int) -> bool:
    """Whether tokens[start..end], an expression that gives the object of a
    view, uncounted (a variable that holds it, or the call that reads it),
    is used as OBJECT_READERS allows (S3): a field access, a comparison,
    `(void)`, or an argument, or address arithmetic inside an argument, of
    a reader.  A copy (assigned_name()) or a derived address
    (derived_name()) is followed by the caller; anything else is refused."""
    prev = tokens[start - 1].text if start > 0 else ""
    nxt = tokens[end + 1].text if end + 1 < len(tokens) else ""
    if nxt == "->" or prev in ("==", "!=") or nxt in ("==", "!="):
        return True
    if prev == "!" or prev == "(" and nxt == ")" and start >= 2 \
            and tokens[start - 2].text == "if":
        return True
    j = end + 1
    while j < len(tokens) and tokens[j].text == ")":
        j += 1
    if j < len(tokens) and tokens[j].text == "->" and j > end + 1:
        # A field of it through a cast
        return True
    if dereferenced(tokens, start, end) or integer_declaration(tokens, start, end):
        return True
    if prev == ")" and nxt == ";" and start >= 3 \
            and tokens[start - 2].text == "void" \
            and tokens[start - 3].text == "(":
        return True
    callee, _ = call_argument(tokens, start)
    if callee in OBJECT_READERS and OBJECT_READERS_SEEN is not None:
        OBJECT_READERS_SEEN.add(callee)
    return callee in OBJECT_READERS


def object_holder(tokens: list[Token], start: int, end: int) -> str | None:
    "The local that a copy of, or an address derived from, the object gets"
    holder = assigned_name(tokens, start, end)
    if holder is None:
        holder = derived_name(tokens, start, end)
    return holder


def inplace_input_use(uop: CodeSection, name: str) -> Token | None:
    """The first use of name, a view, in the body of uop that is not one of
    those above, or None.  Uses inside assert() vanish from release builds
    and are not checked."""
    tokens = [t for t in uop.body.tokens() if t.kind != "COMMENT"]
    stack_names = ({item.name for item in uop.stack.inputs + uop.stack.outputs}
                   if isinstance(uop, Uop) else set())
    names = {name}
    objects: set[str] = set()  # Variables that hold its object, uncounted
    i = 0
    while i < len(tokens):
        tkn = tokens[i]
        if tkn.text == "assert":
            depth = 0
            for i in range(i + 1, len(tokens)):
                if tokens[i].text == "(":
                    depth += 1
                elif tokens[i].text == ")":
                    depth -= 1
                    if depth == 0:
                        break
            i += 1
            continue
        if (tkn.kind == "IDENTIFIER" and tkn.text in objects
                and not (i > 0 and tokens[i - 1].text in (".", "->"))):
            if consumed_argument(tokens, i):
                return tkn
            copy = object_holder(tokens, i, i)
            if copy is not None:
                objects.add(copy)
            elif not (i + 1 < len(tokens) and tokens[i + 1].text == "="
                      and (i == 0 or tokens[i - 1].text in (";", "{", "}"))) \
                    and not object_use_allowed(tokens, i, i):
                return tkn
            i += 1
            continue
        if (tkn.kind != "IDENTIFIER" or tkn.text not in names
                or (i > 0 and tokens[i - 1].text in (".", "->"))):
            i += 1
            continue
        prev = tokens[i - 1].text if i > 0 else ""
        nxt = tokens[i + 1].text if i + 1 < len(tokens) else ""
        if (prev == "=" and nxt == ";" and i >= 2
                and tokens[i - 2].kind == "IDENTIFIER"
                and (i < 3 or tokens[i - 3].text in (";", "{", "}"))):
            if tokens[i - 2].text not in stack_names:
                # The emitter follows copies to stack variables only
                return tkn
            names.add(tokens[i - 2].text)  # A copy, which views the slot too
        elif prev == ")" and nxt == ";" and i >= 3 \
                and tokens[i - 2].text == "void" and tokens[i - 3].text == "(":
            pass
        elif prev in ("(", ",") and nxt in (")", ","):
            callee = enclosing_callee(tokens, i) or ""
            whole = prev == "(" and nxt == ")"
            first = prev == "(" and nxt == ","
            if not ((callee in VIEW_RELEASES and (whole or first))
                    or (callee == "DEAD" and whole)
                    or (callee in VIEW_TAKES and whole)
                    or callee in INPLACE_READERS
                    or callee.startswith("PyValue_Is")):
                return tkn
            if callee in INPLACE_OBJECT_READERS and whole:
                # Its object, which has no count either
                if consumed_argument(tokens, i - 2):
                    return tkn
                holder = object_holder(tokens, i - 2, i + 1)
                if holder is not None:
                    objects.add(holder)
                elif not object_use_allowed(tokens, i - 2, i + 1):
                    return tkn
            elif callee == "_PyValueTemps_Object" and prev == "," and nxt == ")":
                # `_PyValueTemps_Object(temps, name)`: the object of name,
                # uncounted (or a box that temps owns). Follow it like
                # PyValue_AsKnownObjectBorrow's, so that a release drops
                # the borrow.
                if consumed_argument(tokens, i - 4):
                    return tkn
                holder = object_holder(tokens, i - 4, i + 1)
                if holder is not None:
                    objects.add(holder)
                elif not object_use_allowed(tokens, i - 4, i + 1):
                    return tkn
        elif nxt == "." and i + 3 < len(tokens) and tokens[i + 2].text == "bits" \
                and tokens[i + 3].text not in ASSIGNMENT_OPS:
            pass
        elif not (i + 1 < len(tokens) and tokens[i + 1].text == "="
                  and (i == 0 or tokens[i - 1].text in (";", "{", "}"))):
            # Anything but an assignment of a new value to name
            return tkn
        i += 1
    return None


ASSIGNMENT_OPS = {"=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=",
                  "<<=", ">>=", "++", "--"}


def kept_inputs(inst: Instruction, n_inputs: int) -> set[int]:
    """The inputs of inst, bottom first, that a uop may keep: it uses them,
    or a value that it passes through unchanged, as other than
    inplace_input_use() allows.  A local must not reach their fields."""
    kept: set[int] = set()
    stack: list[int] = []
    depth = n_inputs
    for part in inst.parts:
        if not isinstance(part, Uop):
            continue
        popped: list[int] = []
        for item in reversed(part.stack.inputs):
            if stack:
                ext = stack.pop()
            else:
                depth -= 1
                ext = depth
            if (ext >= 0 and item.used and not item.is_array()
                    and inplace_input_use(part, item.name) is not None):
                kept.add(ext)
            popped.append(ext)
        popped.reverse()
        for j, item in enumerate(part.stack.outputs):
            i = passed_through(part, j)
            stack.append(popped[i] if i is not None else -1)
    return kept


class iter_with_first(TokenIterator):
    "tkn_iter with first put back in front of it"

    def __init__(self, first: Token, rest: TokenIterator):
        self.first: Token | None = first
        self.rest = rest

    def __next__(self) -> Token:
        if self.first is not None:
            first, self.first = self.first, None
            return first
        return next(self.rest)

    def peek(self) -> Token | None:
        if self.first is not None:
            return self.first
        return self.rest.peek()


def group_name(inst: Instruction, analysis: Analysis) -> str:
    "The instruction whose format inst shares"
    if inst.family is not None:
        return inst.family.name
    for prefix in SHARED_FORMAT_PREFIXES:
        base = inst.name.removeprefix(prefix)
        if base != inst.name and base in all_instructions(analysis):
            return group_name(all_instructions(analysis)[base], analysis)
    return inst.name


def all_instructions(analysis: Analysis) -> dict[str, Instruction]:
    return {**analysis.instructions, **analysis.ir_only}


# Opcodes of the compiler's stack model that no word ever has: the register
# CFG omits them (PUSH_NULL writes NULL into a temporary that the NULL
# invariant already guarantees is NULL, register-ir.md G4).  They keep their
# opcode number and their format, which the CFG reads, but have no handler.
STACK_ONLY = ("PUSH_NULL",)


def handler_ids(analysis: Analysis) -> dict[str, int]:
    """Handler index of each handler, by name, from the pinned table.
    BINARY_OP itself has none: its operations do, and a STACK_ONLY opcode
    has none."""
    ids = {name: inst.opcode for name, inst in analysis.instructions.items()
           if name != BINARY_OP_FAMILY and name not in STACK_ONLY}
    names = sorted(analysis.ir_only) + binary_op_handlers(analysis)
    table = analysis.numbers
    for name in names:
        ids[name] = table.number(
            name, table.first_ir_only, ir_numbers.IR_LIMIT, "IR-only handler")
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
    (a distance, in D), "wide" (in CD, or with a feedback index in the one
    free 12-bit field) or "narrow" (in a 12-bit field).  An oparg that fits
    no field stays in the feedback entry (LOC_FBARG)."""
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
        if len(fields) <= 2:
            fields += [ROLE_NONE] * (2 - len(fields)) + [ROLE_IMM, ROLE_IMM]
            imm = LOC_CD
        elif len(fields) < 4:
            # The compiler lowers an oparg that does not fit 12 bits
            imm = FIELD_LOCS[FIELDS[len(fields)]]
            fields.append(ROLE_IMM)
        else:
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
    fused = group in FUSED_JUMPS
    forward = jumps and (group in FORWARD_JUMPS or fused)
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
        n_inputs = APPEND_SOURCES.get(group, 0)
        slots += [f"{ROLE_IN0} + {k}" for k in range(n_inputs)]
        placed = place_fields(slots, has_fb, imm_kind, True)
        if placed is None:
            raise analysis_error(
                f"the fields of {group} do not fit in a word", head.where)
    roles, imm, fb = placed
    unpacks = group in UNPACKS
    if unpacks:
        # The first free field names the run of locals
        roles = list(roles)
        roles[roles.index(ROLE_NONE)] = ROLE_DST
    if imm in FIELD_LOCS.values():
        k = list(FIELD_LOCS.values()).index(imm)
        role = oparg_slot_role(members)
        if role is not None and roles[k] == ROLE_IMM:
            roles = roles[:k] + [role] + roles[k + 1:]
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
        for m in members:
            m_inputs, m_outputs = io[m.name]
            shift = n_inputs - len(m_inputs)
            uses.append(["unused"] * shift + classify_inputs(m, len(m_inputs)))
            passthrough |= {e.external + shift for e in m_outputs if e.external >= 0}
        kept: set[int] = set()
        for m in members:
            shift = n_inputs - len(io[m.name][0])
            kept |= {k + shift for k in kept_inputs(m, len(io[m.name][0]))}
        for k in range(n_inputs):
            if k in passthrough or k in kept:
                continue
            # Every member reads the input, so that its canonical slot
            # holds a value whenever code that reads the stack runs.
            if all(u[k] in ("object", "raw") for u in uses):
                borrow_mask |= 1 << k
    if window and n_inputs:
        # The sources are the top inputs of the window, which the handler
        # may read in place unless it keeps or passes them on
        assert len(members) == 1, group
        m_inputs, m_outputs = io[head.name]
        first = len(m_inputs) - n_inputs
        uses = classify_inputs(head, len(m_inputs))
        passthrough = {e.external for e in m_outputs if e.external >= 0}
        kept = kept_inputs(head, len(m_inputs))
        for k in range(n_inputs):
            if first + k in passthrough or first + k in kept:
                continue
            if uses[first + k] in ("object", "raw"):
                borrow_mask |= 1 << k
    store_dst = stores_to_dst(members, io, dst_offset, window)
    fb_layout = fb_kind = None
    if has_fb:
        if group not in FEEDBACK_LAYOUTS:
            raise analysis_error(
                f"{group} has a feedback entry but no layout in "
                "FEEDBACK_LAYOUTS (ir_generator.py)", head.where)
        fb_layout = FEEDBACK_LAYOUTS[group]
        branch = any(tkn.text == "FB_BRANCH" for m in members
                     for tkn in uop_token_list(m))
        fb_kind = "FB_BRANCH" if branch else LAYOUT_KINDS[fb_layout]
    return IRFormat(
        group=group, window=window, roles=roles, inputs=names,
        n_inputs=n_inputs, dst_offset=dst_offset, borrow_mask=borrow_mask,
        imm=imm, fb=fb, jumps=jumps and not forward, forward=forward,
        fused=fused,
        no_fold=no_fold,
        store_dst=store_dst, runs_code=any(runs_code(m) for m in members),
        discards=store_dst and window and group in DISCARDING,
        unpacks=unpacks,
        jump_always=jumps and jumps_always(head),
        names_target=jumps and names_target_only(head),
        fb_layout=fb_layout, fb_kind=fb_kind,
    )


# The entry type of each family's feedback (Include/internal/pycore_feedback.h),
# by the head of the family; the C code names it _PyFb_<head>.  This is the
# only list: the contract's fb_kind and fb_size and the typedefs come from
# it.  A family that has an entry must be listed.  Its kind follows from
# the type, except that the entries of a family that records branches
# (FB_BRANCH()) are FB_BRANCH.
FB_HEADER = "_PyFbHeader"
FEEDBACK_LAYOUTS = {
    "TO_BOOL": "_PyFbUnary",
    "POP_JUMP_IF_FALSE": "_PyFbUnary",
    "POP_JUMP_IF_TRUE": "_PyFbUnary",
    "POP_JUMP_IF_NONE": FB_HEADER,
    "POP_JUMP_IF_NOT_NONE": FB_HEADER,
    "UNPACK_SEQUENCE": "_PyFbUnary",
    "FOR_ITER": "_PyFbUnary",
    "GET_ITER": "_PyFbUnary",
    "SEND": "_PyFbUnary",
    "BINARY_OP": "_PyFbBinary",
    "COMPARE_OP": "_PyFbBinary",
    "CONTAINS_OP": "_PyFbBinary",
    "STORE_SUBSCR": "_PyFbBinary",
    "LOAD_ATTR": "_PyFbAttr",
    "STORE_ATTR": "_PyFbAttr",
    "LOAD_SUPER_ATTR": "_PyFbAttr",
    "LOAD_GLOBAL": "_PyFbGlobal",
    "CALL": "_PyFbCall",
    "CALL_KW": "_PyFbCall",
    "CALL_FUNCTION_EX": "_PyFbCall",
    "COMPARE_OP_JUMP": "_PyFbBinary",
    "CONTAINS_OP_JUMP": "_PyFbBinary",
    # No specialisation; the entry is as large as that of a branch
    "IS_OP_JUMP": "_PyFbUnary",
}
LAYOUT_KINDS = {
    FB_HEADER: "FB_COUNTER",
    "_PyFbUnary": "FB_UNARY",
    "_PyFbBinary": "FB_BINARY",
    "_PyFbAttr": "FB_ATTR",
    "_PyFbGlobal": "FB_GLOBAL",
    "_PyFbCall": "FB_CALL",
}


def uop_token_list(inst: Instruction) -> list[Token]:
    "The tokens of the bodies of inst's uops, without comments"
    return [t for part in inst.parts if isinstance(part, Uop)
            for t in part.body.tokens() if t.kind != "COMMENT"]


def jump_tokens(uop: Uop) -> list[tuple[int, list[str]]]:
    """The JUMPBY and JUMPTO of uop: the brace depth of each, 1 at the top of
    the body, and the tokens of its argument"""
    result = []
    tokens = [t for t in uop.body.tokens() if t.kind != "COMMENT"]
    # The body's own braces, if it has them, take it to depth 1
    depth = 0 if tokens and tokens[0].kind == "LBRACE" else 1
    for i, tkn in enumerate(tokens):
        if tkn.kind == "LBRACE":
            depth += 1
        elif tkn.kind == "RBRACE":
            depth -= 1
        elif tkn.text in ("JUMPBY", "JUMPTO"):
            arg = []
            nesting = 0
            for t in tokens[i + 1:]:
                if t.kind == "LPAREN":
                    nesting += 1
                    if nesting == 1:
                        continue
                elif t.kind == "RPAREN":
                    nesting -= 1
                    if nesting == 0:
                        break
                arg.append(t.text)
            result.append((depth, arg))
    return result


def jumps_always(inst: Instruction) -> bool:
    """Whether inst jumps unconditionally: a JUMPBY or JUMPTO at the top of
    a uop's body, so that the word has one successor, its target"""
    return any(depth == 1 and arg != ["0"]
               for part in inst.parts if isinstance(part, Uop)
               for depth, arg in jump_tokens(part))


def names_target_only(inst: Instruction) -> bool:
    """Whether inst, whose family jumps, never goes to its target: its only
    jump is JUMPBY(0), which names the word for sys.monitoring, as
    END_ASYNC_FOR names the END_SEND of its loop"""
    jumps = [arg for part in inst.parts if isinstance(part, Uop)
             for _, arg in jump_tokens(part)]
    return bool(jumps) and all(arg == ["0"] for arg in jumps)


# Tokens that assign to what stands before them
ASSIGNMENTS = {"EQUALS", "TIMESEQUAL", "DIVEQUAL", "MODEQUAL", "PLUSEQUAL",
               "MINUSEQUAL", "LSHIFTEQUAL", "RSHIFTEQUAL", "ANDEQUAL",
               "OREQUAL", "XOREQUAL", "PLUSPLUS", "MINUSMINUS"}
# What may stand before an & that is a binary operator, not address-of
OPERANDS = {"IDENTIFIER", "NUMBER", "RPAREN", "RBRACKET"}


# The uses of GETLOCAL(oparg) that oparg_slot_role() knows.  Any other use
# is an error, so that a write the derivation does not recognise cannot pass
# as a read.  The superinstructions address their slots as GETLOCAL(oparg1)
# and GETLOCAL(oparg2); flowgraph.c creates none for the register IR, and
# their packed opargs keep role IMM.
SLOT_VALUE_READS = {        # f(GETLOCAL(oparg), ...) reads the slot's value
    "PyValue_DUP", "PyValue_IsNull", "PyCell_NewValue",
    "PyValue_AsKnownObjectBorrow", "PyValue_Borrow",
}
SLOT_ADDRESS_READS: set[str] = set()  # f(&GETLOCAL(oparg)) reads the slot in place
SLOT_ADDRESS_WRITES = {     # f(&GETLOCAL(oparg), ...) may store to the slot
    "PyValue_BoxInto", "PyValue_Publish",
}


def enclosing_call(tokens: list[Token], k: int) -> Token | None:
    "The function whose argument list holds tokens[k], or None"
    depth = 0
    for j in range(k - 1, -1, -1):
        kind = tokens[j].kind
        if kind == "RPAREN":
            depth += 1
        elif kind == "LPAREN":
            if depth == 0:
                if j > 0 and tokens[j - 1].kind == "IDENTIFIER":
                    return tokens[j - 1]
                return None
            depth -= 1
        elif kind in ("SEMI", "LBRACE", "RBRACE") and depth == 0:
            return None
    return None


def slot_uses(inst: Instruction) -> list[tuple[str, Token, str | None]]:
    """The uses of GETLOCAL(oparg) in inst: ("read", token, the variable it
    is copied to or None), ("store", ...) or ("clear", ...).  Raises
    analysis_error for a use of any other shape."""
    uses: list[tuple[str, Token, str | None]] = []
    for part in inst.parts:
        if not isinstance(part, Uop):
            continue
        tokens = [t for t in part.body.tokens() if t.kind != "COMMENT"]
        for i in range(len(tokens) - 3):
            if [t.text for t in tokens[i:i + 4]] != ["GETLOCAL", "(", "oparg", ")"]:
                continue
            a, b = i, i + 4         # the use is tokens[a:b]
            # Parentheses that only group it, as in (GETLOCAL(oparg)) = v
            while (a > 0 and b < len(tokens) and tokens[a - 1].kind == "LPAREN"
                   and tokens[b].kind == "RPAREN"
                   and (a < 2 or tokens[a - 2].kind != "IDENTIFIER")):
                a -= 1
                b += 1
            prev = tokens[a - 1] if a > 0 else None
            nxt = tokens[b] if b < len(tokens) else None
            if nxt is not None and nxt.kind == "EQUALS":
                rhs = []
                for t in tokens[b + 1:]:
                    if t.kind == "SEMI":
                        break
                    rhs.append(t.text)
                uses.append(("clear" if rhs == ["PyValue_NULL"] else "store",
                             tokens[i], None))
                continue
            if nxt is not None and nxt.kind in ASSIGNMENTS:
                raise analysis_error(
                    f"{inst.name}: compound assignment to GETLOCAL(oparg)",
                    tokens[i])
            if prev is not None and prev.kind == "AND" and (
                    a < 2 or tokens[a - 2].kind not in OPERANDS):
                func = enclosing_call(tokens, a - 1)
                name = func.text if func is not None else None
                if name in SLOT_ADDRESS_READS:
                    uses.append(("read", tokens[i], None))
                    continue
                if name in SLOT_ADDRESS_WRITES:
                    uses.append(("store", tokens[i], None))
                    continue
                raise analysis_error(
                    f"{inst.name}: &GETLOCAL(oparg) passed to {name}, which "
                    "ir_generator.py does not know as a read or a write",
                    tokens[i])
            if (prev is not None and prev.kind == "EQUALS"
                    and nxt is not None and nxt.kind == "SEMI"):
                # x = GETLOCAL(oparg); or a declaration of x
                uses.append(("read", tokens[i], tokens[a - 2].text))
                continue
            if (prev is not None and prev.kind in ("LPAREN", "COMMA")
                    and nxt is not None and nxt.kind in ("RPAREN", "COMMA")):
                func = enclosing_call(tokens, a)
                if func is not None and func.text in SLOT_VALUE_READS:
                    uses.append(("read", tokens[i], None))
                    continue
                name = func.text if func is not None else None
                raise analysis_error(
                    f"{inst.name}: GETLOCAL(oparg) passed to {name}, which "
                    "ir_generator.py does not know as a read", tokens[i])
            raise analysis_error(
                f"{inst.name}: a use of GETLOCAL(oparg) that ir_generator.py "
                "does not recognise", tokens[i])
    return uses


def uses_cells(inst: Instruction) -> bool:
    "Whether inst operates on a cell: it calls PyCell_* or _PyCell_*"
    return any(t.kind == "IDENTIFIER" and t.text.startswith(("PyCell_", "_PyCell_"))
               and t.text != "PyCell_NewValue"
               for t in uop_token_list(inst))


def oparg_slot_role(members: list[Instruction]) -> str | None:
    """The role of an oparg that names a slot below co_nlocalsplus, which the
    handlers address as GETLOCAL(oparg) (slot_uses()), or None for any other
    oparg:

    - LOCAL_OUT if a member stores a value there;
    - LOCAL_MOVE if one clears it after copying its value to an output;
    - LOCAL_CLEAR if one clears it otherwise, releasing the old value;
    - CELL if they only read it and operate on cells (uses_cells());
    - LOCAL_IN if they only read it."""
    stores = clears = moves = reads = cells = False
    for m in members:
        uses = slot_uses(m)
        outputs = {item.name for part in m.parts if isinstance(part, Uop)
                   for item in part.stack.outputs}
        copied_out = any(kind == "read" and target in outputs
                         for kind, _, target in uses)
        for kind, _, _ in uses:
            if kind == "store":
                stores = True
            elif kind == "clear":
                if copied_out:
                    moves = True
                else:
                    clears = True
            else:
                reads = True
                cells |= uses_cells(m)
    if stores:
        return ROLE_LOCAL_OUT
    if clears:
        return ROLE_LOCAL_CLEAR
    if moves:
        return ROLE_LOCAL_MOVE
    if reads:
        return ROLE_CELL if cells else ROLE_LOCAL_IN
    return None


STORE_RESULT = "store"     # moves the result to the local dst names
STORE_RETURN = "return"    # pushes a frame, which returns the result to the
                           # slot that dst names, a local or a temporary
STORE_LEAVE = "leave"      # leaves to the family head if dst names a local
STORE_NEVER = "never"      # pushes more than the result, so dst never names
                           # a local


def store_mode(inst: Instruction, outputs: list[Entry]) -> str | None:
    """How inst, a member of a family whose dst may name a local, treats
    such a dst; None if it cannot.  The result must be the first new output
    of inst, not an alias of an input, which may be a borrow.  Members of a
    family share their stack effect for each oparg, and the assembler folds
    a store only where the instruction pushes the result alone, so a member
    that always pushes more never sees a local dst.  The result of a frame
    that a member pushes arrives only when the callee returns, in the slot
    of frame->return_slot, which releases a local's old value
    (_PyFrame_SetReturnValue).  A member that always pushes a frame and
    names dst as its return slot needs nothing more; one that may also
    produce the result itself moves it.  A member whose frames return
    elsewhere leaves to the head."""
    # The terminal adapter publishes the native result itself, or the Python
    # child's eventual return. Its unused DSL C output still has a real dst.
    if uses_call_transaction(inst):
        return STORE_RESULT
    if inst.properties.switches_frame:
        if not names_return_slot(inst):
            return STORE_LEAVE
        if pushes_always(inst):
            return STORE_RETURN
    if inst.properties.no_fold:
        return None
    j = result_offset(inst, outputs)
    if j is None or outputs[j].aliased:
        return None
    if all(e.item.is_array() and not e.item.size.isdigit()
           for e in outputs[j + 1:]):
        return STORE_RESULT
    return STORE_NEVER


def uop_tokens(inst: Instruction) -> list[str]:
    return [t.text for part in inst.parts if isinstance(part, Uop)
            for t in part.body.tokens()]


def names_return_slot(inst: Instruction) -> bool:
    """Whether the frames that inst pushes return only their result, to the
    slot dst names.  A generator's frame also hands each value it yields to
    that slot, and its return value at the end of a FOR_ITER."""
    tokens = uop_tokens(inst)
    return "IR_DST_SLOT" in tokens and "gi_iframe" not in tokens


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
        # INSTRUMENTED_LINE and INSTRUMENTED_INSTRUCTION call a monitoring
        # callback before they run the word's own handler
        formats[name] = IRFormat(
            group=name, window=False, roles=[ROLE_NONE] * 4,
            inputs=[], n_inputs=0, dst_offset=None, borrow_mask=0,
            imm=LOC_NONE, fb=LOC_NONE, jumps=False,
            transparent=True, runs_code=runs_code(insts[name]))
    return formats


def field_expr(loc: str) -> str:
    "The C expression of the field at loc, in a handler"
    if loc == LOC_CD:
        return "IR_CD(ir_word)"
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


# The types of a declaration that holds no pointer derived from an object:
# numbers, and values (an owner counts its object, and S3 follows views)
VALUE_TYPES = INTEGER_TYPES | {
    "bool", "char", "double", "float", "short", "unsigned", "signed",
    "Py_hash_t", "Py_UCS4", "PyValue", "PyValueBorrow"}


def is_value_type(text: str) -> bool:
    "Whether a declaration of type text holds no pointer"
    return text in VALUE_TYPES or re.fullmatch(r"u?int\d+_t", text) is not None


def designator_start(tokens: list[Token], i: int) -> int:
    """The first token of the postfix expression that ends at tokens[i], the
    designator of a call: `f`, `(*f)`, `Py_TYPE(o)->tp_iternext`"""
    j = i
    while True:
        close = tokens[j].text
        if close in (")", "]"):
            opening = "(" if close == ")" else "["
            depth = 0
            while j > 0:
                if tokens[j].text == close:
                    depth += 1
                elif tokens[j].text == opening:
                    depth -= 1
                    if depth == 0:
                        break
                j -= 1
            if j > 0 and (tokens[j - 1].kind == "IDENTIFIER"
                          or tokens[j - 1].text in (")", "]")):
                j -= 1  # A call or an index of the expression before
                continue
        if j > 1 and tokens[j - 1].text in ("->", "."):
            j -= 2
            continue
        return j


def object_source(tokens: list[Token], views: dict[str, int],
                  view_objects: dict[str, int]) -> int | None:
    """The position of the input whose object the expression reads, uncounted:
    a variable that holds such an object, or the object of a view"""
    texts = [t.text for t in tokens]
    for i, tkn in enumerate(tokens):
        if tkn.kind != "IDENTIFIER" or i > 0 and texts[i - 1] in (".", "->"):
            continue
        if tkn.text in view_objects:
            return view_objects[tkn.text]
        if (tkn.text == "PyValue_AsKnownObjectBorrow" and i + 3 < len(texts)
                and texts[i + 1] == "(" and texts[i + 2] in views
                and texts[i + 3] == ")"):
            return views[texts[i + 2]]
        if (tkn.text == "_PyValueTemps_Object" and i + 5 < len(texts)
                and texts[i + 1] == "(" and texts[i + 3] == ","
                and texts[i + 4] in views and texts[i + 5] == ")"):
            return views[texts[i + 4]]
    return None


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

    def __init__(self, layout: SlotLayout | None = None,
                 roles: dict[str, set[str]] | None = None) -> None:
        super().__init__()
        self.layout = layout if layout is not None else SlotLayout.empty()
        # The C variables that hold the view of each borrowable input, by
        # position, the latest last: IR_CONSUME_INPUT adopts the bits that
        # the handler loaded rather than reloading the slot.
        self.views: dict[int, list[str]] = {}
        # Views that may dangle: they were loaded before a call that can run
        # code, which can rebind a local (S7, IREmitter.check_stale_views).
        self.stale: set[str] = set()
        # The body's variables that hold the object of a view, uncounted,
        # by the view's input, which dangle with it until the body takes
        # the view (IREmitter.note_view_object)
        self.view_objects: dict[str, int] = {}
        # The fields whose input's owner the body has taken or released
        # (TAKE, OWNER, a steal, a release or DECREF_INPUTS), with the
        # token that did: at most once on any path, whichever view of the
        # input names it (IREmitter.spend_owner)
        self.spent: dict[str, Token] = {}
        # The views of those inputs, which the spend killed: no later
        # statement reads them (IREmitter.check_spent_views)
        self.spent_views: set[str] = set()
        # The role of each C variable of the handler, "view" or "owner",
        # which gives its type (S8, generate_handler); shared by copies.
        self.roles: dict[str, set[str]] = roles if roles is not None else {}
        # Register style: where the frame holds the value at a constant
        # position, "in" the slot of its input or "out" the slot from dst.
        # Positions that are not constant are outputs.
        self.owner: dict[int, str] = {}
        self.owner = {p: "in" for p in self.layout.in_fields}
        # Set for the flush at the end of the handler, where a dst that
        # names a local is written; and after the frame changed, when no
        # position may be written.
        self.final = False
        self.reloaded = False
        self.error_exit = False
        # Set by IR_WINDOW_TAKEN(): the handler disposed of its window, so
        # that the slots of the dead inputs stay as it left them.
        self.window_taken = False
        # Attribute completion or retirement published the temporary result
        # before receiver retirement. Only a folded local remains to write.
        self.published_temporary_result = False
        # The IR_STORE_DST and IR_MOVE_TO_DST lines emitted for the handler.
        # Copies share the list: ir_displaced holds one displaced local and
        # finish_displaced_local takes it by value, so a handler may store
        # dst once (generate_handler checks).
        self.dst_stores: list[str] = []

    def copy(self) -> "IRStack":
        other = super().copy()
        assert isinstance(other, IRStack)
        other.owner = dict(self.owner)
        other.views = {p: list(names) for p, names in self.views.items()}
        other.stale = set(self.stale)
        other.view_objects = dict(self.view_objects)
        other.spent = dict(self.spent)
        other.spent_views = set(self.spent_views)
        other.roles = self.roles
        other.window_taken = self.window_taken
        return other

    def code_ran(self) -> None:
        "Code that can rebind a local may have run: each view may dangle"
        for names in self.views.values():
            self.stale.update(names)
        self.stale.update(self.view_objects)

    def taken(self, p: int) -> None:
        "The body took a count of the input at p, which keeps its object"
        for name in [n for n, q in self.view_objects.items() if q == p]:
            del self.view_objects[name]
            self.stale.discard(name)

    def field_position(self, field: str) -> int:
        "The position of the input read through field"
        for p, f in self.layout.in_fields.items():
            if f == field:
                return p
        raise StackError(f"no input is read through field {field}")

    def bind(self, name: str, view_of: int | None) -> None:
        """C variable name is assigned: the view of the input at view_of, or
        an owner if view_of is None"""
        for names in self.views.values():
            if name in names:
                names.remove(name)
        self.stale.discard(name)
        self.spent_views.discard(name)
        if view_of is not None:
            self.views.setdefault(view_of, []).append(name)
        self.roles.setdefault(name, set()).add(
            "view" if view_of is not None else "owner")

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
        p = memory_offset.as_int()
        if p is not None and self.home_of(p) == "in":
            return f"IR_SLOT({self.input_field(p)})"
        if lay.window:
            return self.window_slot(memory_offset)
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

    def load(self, var: StackItem, memory_offset: PointerOffset) -> str:
        lay = self.layout
        if not var.is_array():
            p = memory_offset.as_int()
            if (p is not None and self.owner.get(p) == "in"
                    and p in lay.borrowable):
                return f"IR_VIEW({self.input_field(p)})"
        return super().load(var, memory_offset)

    def pop(self, var: StackItem, out: CWriter) -> Local:
        before = self.variables[-1] if self.variables else None
        old_name = before.name if before is not None else None
        fresh = before is None or not before.in_local
        local = super().pop(var, out)
        lay = self.layout
        if (local.memory_offset is not None and var.used
                and local.ir_input is None and not var.is_array()):
            p = local.memory_offset.as_int()
            if p is not None and self.owner.get(p) == "in":
                local.ir_input = p
                if p in lay.borrowable:
                    local.ir_inplace = lay.in_fields[p]
        if var.used and not var.is_array() and (fresh or old_name != var.name):
            # The pop assigned var: a load, or a copy of another variable
            view_of = None
            if local.ir_inplace is not None:
                view_of = self.field_position(local.ir_inplace)
            stale = old_name in self.stale
            self.bind(var.name, view_of)
            if stale and not fresh:
                self.stale.add(var.name)
            if view_of is not None and not fresh:
                # A copy that a uop may only release, which reads the owner
                out.emit(f"(void){var.name};\n")
        return local

    def consume(self, out: CWriter, p: int) -> None:
        """The frame gives up the input at position p.  A borrowable one's
        owner, ir_own_<F>, adopts the bits of a variable that holds its
        view."""
        f = self.input_field(p)
        if p not in self.layout.borrowable:
            out.emit(f"IR_SLOT({f}) = PyValue_NULL;\n")
            return
        names = self.views.get(p)
        if not names:
            raise StackError(f"no variable holds the view of the input "
                             f"in {f} when the handler takes it")
        out.emit(f"{owner_name(self.layout.in_fields[p])} = "
                 f"IR_CONSUME_INPUT({f}, {names[-1]});\n")

    def store(self, out: CWriter, var: Local, memory_offset: PointerOffset) -> bool:
        """Write var, which the stack holds at memory_offset; whether the
        frame then holds it there"""
        out.start_line()
        p = memory_offset.as_int()
        if self.reloaded:
            # A value that the instruction hands to the frame it switched
            # to: the slot that frame named when it called or resumed
            self.refuse_inplace(var, memory_offset)
            if memory_offset.as_int() != 0:
                raise StackError(f"'{var.name}' is written after the frame changed")
            out.emit(f"IR_STORE_RETURN({var.name});\n")
            return True
        lay = self.layout
        if lay.window:
            self.own_inplace(out, var)
            out.emit(f"IR_PUBLISH_TEMP({self.window_slot(memory_offset)}, {var.name});\n")
            return True
        if p is not None and var.ir_input == p and self.owner.get(p) == "in":
            # The input itself, back in its slot, changed: a NULL before the
            # handler closes it.  A borrowable input's slot keeps the view's
            # value: a new value there is an output of its own name.
            f = self.input_field(p)
            if p in lay.borrowable:
                raise StackError(f"'{var.name}' would be stored back to its "
                                 f"input's slot, which may name a local")
            out.emit(f"IR_PUBLISH_TEMP(IR_SLOT({f}), {var.name});\n")
            return True
        if p is not None and (lay.dst_offset is None or p < lay.dst_offset):
            if p in lay.in_fields and p not in lay.borrowable:
                # An input that the instruction passes through, changed
                self.own_inplace(out, var)
                out.emit(f"IR_PUBLISH_TEMP(IR_SLOT({self.input_field(p)}), {var.name});\n")
                return True
            return self.keep(var, p)
        if lay.store_dst and p != lay.dst_offset:
            # dst may name a local, so the slots after it are not ours
            return self.keep(var, p)
        self.own_inplace(out, var)
        previous = self.owner.get(p) if p is not None else "out"
        if previous == "in":
            self.consume(out, p)
            self.owner[p] = "out"
            previous = None
        slot = self.out_slot(memory_offset)
        if lay.store_dst and p == lay.dst_offset:
            if self.final:
                if self.published_temporary_result:
                    out.emit(f"if (IR_IS_LOCAL({FIELD_VARS[lay.dst]})) {{\n")
                self.dst_stores.append(f"IR_STORE_DST({FIELD_VARS[lay.dst]}, {var.name})")
                out.emit(f"{self.dst_stores[-1]};\n")
                if self.published_temporary_result:
                    out.emit("}\n")
                return True
            # Before the end, dst gets the result only if it is a
            # temporary; a local gets it at the end, releasing its value.
            out.emit(f"IR_STORE_TEMP_DST({FIELD_VARS[lay.dst]}, {var.name});\n")
            return False
        if previous == "out":
            out.emit(f"IR_PUBLISH_TEMP({slot}, {var.name});\n")
        else:
            out.emit(f"IR_STORE_TEMP({slot}, {var.name});\n")
        if p is not None:
            self.owner[p] = "out"
        return True

    def own_inplace(self, out: CWriter, var: Local) -> None:
        """Before var goes to a slot other than its input's: a view cannot
        (no frame slot holds a borrow); the DSL writes TAKE() for a value
        that goes to a home"""
        if var.ir_inplace is not None:
            raise StackError(f"'{var.name}', a view of the input in "
                             f"{FIELD_VARS[var.ir_inplace]}, would be stored "
                             f"to a slot; assign the output TAKE() of it")

    def refuse_inplace(self, var: Local, memory_offset: PointerOffset) -> None:
        "A slot must not get a local's reference without a count"
        if var.ir_inplace is not None:
            raise StackError(f"'{var.name}', a view of the input in "
                             f"{FIELD_VARS[var.ir_inplace]}, would be stored "
                             f"at position {memory_offset}")

    def keep(self, var: Local, p: int | None) -> bool:
        """A value at a position without a slot of its own, such as the
        input that a later uop of the handler closes, stays in its C
        variable, which owns it; it needs no slot unless the handler can
        leave with it."""
        if self.final or self.error_exit:
            raise StackError(f"'{var.name}' has no slot at position {p}")
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
                position = low + PointerOffset(i, (), ())
                p = position.as_int()
                if p is not None and p in lay.in_fields:
                    # A source: its owner takes it, a local stays
                    if self.owner.get(p) == "in":
                        del self.owner[p]
                        self.consume(out, p)
                    continue
                out.emit(f"{self.window_slot(position)} = PyValue_NULL;\n")
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
        if may_be_positive(self.physical_sp - self.logical_sp):
            self.vacate(out, self.logical_sp, self.physical_sp)
        self.save_variables(out)
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
        # A variable views an input after the join if it does on both
        # paths, and may dangle if it may on either
        for p in set(self.views) | set(other.views):
            mine = self.views.get(p, [])
            theirs = other.views.get(p, [])
            both = [name for name in mine if name in theirs]
            self.views[p] = both
            other.views[p] = list(both)
        self.stale |= other.stale
        other.stale = set(self.stale)
        self.view_objects |= other.view_objects
        other.view_objects = dict(self.view_objects)
        # An owner is spent after the join if it is on either path
        self.spent = other.spent | self.spent
        other.spent = dict(self.spent)
        self.spent_views |= other.spent_views
        other.spent_views = set(self.spent_views)
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
        self._replacers["IR_RETIRE_PUBLISHED"] = self.retire_published
        self._replacers["IR_RETIRE_PUBLISHED_RESULT"] = self.retire_published
        self._replacers["IR_RETIRE_PUBLISHED_PAIR"] = self.retire_published
        self._replacers["IR_FINISH_VALUE_OPERATION"] = self.finish_value_operation
        self._replacers["IR_RESTORE_VALUE_INPUTS"] = self.restore_value_inputs
        self._replacers["IR_COMPLETE_VALUE_READER"] = self.complete_value_reader
        self._replacers["IR_ABORT_VALUE_READER"] = self.abort_value_reader
        self._replacers["IR_COMPLETE_ATTR_READER"] = self.complete_attr_reader
        self._replacers["IR_PREPARE_CALL_METHOD"] = self.prepare_call
        self._replacers["IR_PREPARE_CALL_TUPLE"] = self.prepare_call
        self._replacers["IR_MONITOR_CALL"] = self.prepare_call
        self._replacers["IR_INVOKE_CALL"] = self.invoke_call
        self._replacers["IR_BEGIN_CALL_EFFECTS"] = self.call_effects
        self._replacers["IR_REFRESH_CALL"] = self.refresh_call
        self._replacers["TAKE"] = self.take_view
        self._replacers["OWNER"] = self.take_view
        self._replacers["PyValue_AsKnownObjectBorrow"] = self.object_of
        self._replacers["PyValue_AsPyObjectNew"] = self.object_of
        # Set while a statement is emitted
        self.statement: SimpleStmt | None = None
        # The input, and its field, that the statement's escaping release
        # killed, if it is a view
        self.killed_inplace: tuple[str, str] | None = None
        # The variable that `PyValue NAME = OWNER(x);` assigned, which only
        # IR_RETIRE_PUBLISHED may read, with no call that runs code before
        self.pending_owner: Token | None = None
        # The outputs that a statement `out = TAKE(in);` assigned in the
        # handler (check_error_outputs())
        self.taken_outputs: set[str] = set()
        # The object locals of the handler (emit_object_inputs), which the
        # first pass of generate_handler finds and the second declares
        self.boxes: set[str] = set()
        # The variables assigned from one that has an object local, in the
        # uop, with the source; their object is read through the source
        self.box_copies: dict[str, str] = {}
        # The C type of each variable of the handler, from the first pass
        self.declared_roles: dict[str, set[str]] = {}
        self.stores_outputs = False  # the statement's uop is the last
        self.call_finished = False
        self.call_fallback_used = False
        self.call_fallback = ""
        self.call_has_feedback = False
        # The handler's call_transaction_run_*() and its constant arguments
        self.call_run = ""
        self.call_argc = ""
        self.call_kind = ""
        self.call_flags = ""
        # The format of the handler being emitted
        self.handler_format: IRFormat | None = None
        self.result_publication: "ResultPublicationContract | None" = None
        # Whether the handler being emitted has a value record, value_temps:
        # only a handler whose contract has BOXES does (boxing_handlers)
        self.has_record = True

    @property
    def temps(self) -> str:
        "The record argument of a function that finishes the handler's boxes"
        return "value_temps" if self.has_record else "NULL"

    def format_of(self, inst: Instruction) -> IRFormat:
        if self.handler_format is not None:
            return self.handler_format
        return self.formats[inst.name]

    def retire_published(self, tkn, tkn_iter, uop, storage, inst):
        assert inst is not None
        assert next(tkn_iter).kind == "LPAREN"
        first = next(tkn_iter)
        assert first.kind == "IDENTIFIER"
        second = None
        result_name = None
        end = next(tkn_iter)
        if end.kind == "COMMA":
            second = next(tkn_iter)
            assert second.kind == "IDENTIFIER"
            end = next(tkn_iter)
            if tkn.text == "IR_RETIRE_PUBLISHED_RESULT":
                result_name = first.text
                first = second
                second = None
        assert end.kind == "RPAREN"
        assert next(tkn_iter).kind == "SEMI"
        if tkn.text == "IR_RETIRE_PUBLISHED_RESULT":
            contract = self.result_publication
            if (contract is None or contract.kind != "explicit"
                    or contract.result != result_name
                    or contract.owner != first.text
                    or uop is not contract.producer):
                raise analysis_error(
                    f"{tkn.text} has no matching checked result contract", tkn)
        for retired in (first, second):
            if self.view_field(retired, storage) is not None:
                raise analysis_error(
                    f"{tkn.text}({retired.text}) of a view: retire its "
                    f"OWNER()", retired)
        storage.clear_inputs("before published receiver retirement")
        storage.flush(self.out)
        self.pending_owner = None
        failure_result = "PyValue_NULL"
        if tkn.text == "IR_RETIRE_PUBLISHED_RESULT":
            fmt = self.format_of(inst)
            dst = FIELD_VARS[fmt.field_of(ROLE_DST)]
            failure_result = (
                f"(IR_IS_LOCAL({dst}) ? {result_name} : PyValue_NULL)")
            assert isinstance(storage.stack, IRStack)
            storage.stack.published_temporary_result = True
        self.emit("{\n")
        boxes = " || value_temps->count != 0" if self.has_record else ""
        self.emit(f"int retire_needed = _PyValue_MayDealloc({first.text}){boxes};\n")
        self.emit("if (!retire_needed) {\n")
        self.emit(f"PyValue_XCLOSE({first.text}); {first.text} = PyValue_NULL;\n")
        if second is not None:
            self.emit(f"retire_needed = _PyValue_MayDealloc({second.text});\n")
        self.emit("}\n")
        self.emit("if (retire_needed) {\n")
        # The values pass by value and are dead after the call (R-a)
        second_value = "PyValue_NULL" if second is None else second.text
        self.emit(f"int retire_failed = retire_published_values(tstate, frame, next_instr, {self.temps}, {first.text}, {second_value}, {failure_result});\n")
        self.emit("if (retire_failed < 0) { JUMP_TO_LABEL(error); }\n")
        self.emit("this_instr = frame->instr_ptr;\n")
        self.emit("ir_word = FT_ATOMIC_LOAD_UINT64_RELAXED(*this_instr);\n")
        self.emit("slots = _PyFrame_Slots(frame);\n")
        if self.format_of(inst).window:
            base = FIELD_VARS[self.format_of(inst).field_of(ROLE_BASE)]
            self.emit(f"window = &IR_SLOT({base});\n")
        self.emit("}\n")
        if second is not None:
            self.emit("else {\n")
            self.emit(f"PyValue_XCLOSE({second.text}); {second.text} = PyValue_NULL;\n")
            self.emit("}\n")
        self.emit("}\n")
        stack = storage.stack
        assert isinstance(stack, IRStack)
        stack.code_ran()
        return True

    def macro_args(self, tkn, tkn_iter, count: int) -> list[str]:
        "The count identifiers of the statement IR_X(a, b, ...);"
        assert next(tkn_iter).kind == "LPAREN"
        names = []
        for index in range(count):
            name = next(tkn_iter)
            if name.kind != "IDENTIFIER":
                raise analysis_error(
                    f"{tkn.text} takes {count} variables (register-ir-v2-a3.md "
                    f"3.5: no cached operation)", name)
            names.append(name.text)
            if index < count - 1:
                assert next(tkn_iter).kind == "COMMA"
        end = next(tkn_iter)
        if end.kind != "RPAREN":
            raise analysis_error(f"{tkn.text} takes {count} variables", end)
        assert next(tkn_iter).kind == "SEMI"
        return names

    def input_slot(self, inst: Instruction, index: int) -> str:
        """The slot of input index, from the handler's word as read before
        the operation ran: no reload happens between an operation and the
        helper that restores or completes it (R-b)"""
        fmt = self.format_of(inst)
        return FIELD_VARS[fmt.input_field(index)]

    def complete_attr_reader(self, tkn, tkn_iter, uop, storage, inst):
        assert inst is not None
        contract = self.result_publication
        if (contract is None or contract.kind != "generic"
                or contract.producer is not uop):
            raise analysis_error(
                "IR_COMPLETE_ATTR_READER has no checked result contract", tkn)
        result, self_value, receiver, method = self.macro_args(tkn, tkn_iter, 4)
        dst = FIELD_VARS[self.format_of(inst).field_of(ROLE_DST)]
        self.out.start_line()
        self.emit(f"{result} = value_attribute_complete(tstate, frame, this_instr, "
                  f"{self.temps}, {dst}, {result}, {self_value}, {receiver}, {method});\n")
        self.emit(f"if (PyValue_IsNull({result})) {{\n")
        self.reader_error()
        self.emit("}\n")
        self.reader_reload()
        assert isinstance(storage.stack, IRStack)
        storage.stack.published_temporary_result = True
        return True

    def restore_value_inputs(self, tkn, tkn_iter, uop, storage, inst):
        """Each input whose slot is a temporary goes back to it; the
        variables keep the owners not moved"""
        assert inst is not None
        first, second = self.macro_args(tkn, tkn_iter, 2)
        assert self.format_of(inst).n_inputs == 2
        self.out.start_line()
        self.emit("{\n")
        self.emit(f"_PyValuePair ir_rest = value_operation_restore2(tstate, frame, "
                  f"{first}, {second}, {self.input_slot(inst, 0)}, "
                  f"{self.input_slot(inst, 1)});\n")
        self.emit(f"{first} = ir_rest.first;\n")
        self.emit(f"{second} = ir_rest.second;\n")
        self.emit("}\n")
        # Cleanup and forced finish follow even if restoration rejects the
        # frame; it must not strand any of the still-owning C variables.
        return True

    def reader_error(self):
        self.emit("JUMP_TO_LABEL(error);\n")

    def release_displaced(self):
        """Release the local value that IR_STORE_DST or IR_MOVE_TO_DST
        displaced, with the input boxes, then revalidate: the release can
        run __del__ or a weakref callback, which can run any Python."""
        self.emit("if (!PyValue_IsNull(ir_displaced) &&\n")
        self.emit("    finish_displaced_local(tstate, frame, next_instr, "
                  f"{self.temps}, ir_displaced) < 0) {{\n")
        self.emit("next_instr = frame->instr_ptr + 1;\n")
        self.reader_error()
        self.emit("}\n")

    def unpack_to_locals(self, fmt, count):
        """The elements are in the window, in stack order; if dst names a
        run of locals they go there, one element at a time as the STORE_FASTs
        they replace would, so that a __del__ sees the same locals.  A
        release that may run Python is out of line."""
        dst = FIELD_VARS[fmt.field_of(ROLE_DST)]
        self.emit(f"if (IR_IS_LOCAL({dst})) {{\n")
        self.emit(f"int ir_n = {count};\n")
        self.emit(f"int ir_rest = IR_UNPACK_TO_LOCALS(&IR_SLOT({dst}), "
                  "window, ir_n);\n")
        self.emit("if (ir_rest >= 0 &&\n")
        self.emit("    finish_unpack_to_locals(tstate, frame, next_instr, "
                  f"{self.temps}, &IR_SLOT({dst}), window, ir_n, ir_rest) < 0) {{\n")
        self.emit("next_instr = frame->instr_ptr + 1;\n")
        self.reader_error()
        self.emit("}\n")
        self.emit("}\n")

    def reader_reload(self):
        self.emit("this_instr = frame->instr_ptr;\n")
        self.emit("ir_word = FT_ATOMIC_LOAD_UINT64_RELAXED(*this_instr);\n")
        self.emit("slots = _PyFrame_Slots(frame);\n")

    def call_reload(self, inst):
        self.reader_reload()
        self.emit("window = slots + call_base;\n")
        if self.call_has_feedback:
            self.emit(f"fb = ({feedback_type(self.format_of(inst))} *)"
                      "(frame->ir_feedback + ((size_t)call_feedback << 3));\n")

    def prepare_call(self, tkn, tkn_iter, uop, storage, inst):
        """A step of the call before its invocation, which records in the
        register local call_state what it did (register-ir-v2-a3.md 3.5,
        R-c): CALL_EFFECTS sends a later failed guard to the fallback"""
        assert inst is not None
        self.macro_args(tkn, tkn_iter, 0)
        storage.flush(self.out)
        self.out.start_line()
        if tkn.text == "IR_PREPARE_CALL_METHOD":
            self.emit("{\n")
            self.emit("int ir_expanded = call_transaction_expand_method(tstate, frame, this_instr, call_base);\n")
            self.emit("if (ir_expanded < 0) {\n")
            self.reader_error()
            self.emit("}\n")
            self.emit("call_state |= CALL_PREPARED | (ir_expanded ? CALL_EFFECTS : 0);\n")
            self.emit("}\n")
        else:
            if tkn.text == "IR_PREPARE_CALL_TUPLE":
                self.emit("call_state |= CALL_PREPARED | CALL_EFFECTS;\n")
                call = "call_transaction_prepare_tuple(tstate, frame, this_instr, call_base)"
            else:
                assert tkn.text == "IR_MONITOR_CALL"
                self.emit("call_state |= CALL_MONITORED | CALL_EFFECTS;\n")
                call = (f"call_transaction_monitor(tstate, frame, this_instr, call_base, "
                        f"{self.call_argc}, {self.call_kind})")
            self.emit(f"if ({call} < 0) {{\n")
            self.reader_error()
            self.emit("}\n")
        self.call_reload(inst)
        return True

    def call_effects(self, tkn, tkn_iter, uop, storage, inst):
        self.macro_args(tkn, tkn_iter, 0)
        self.out.start_line()
        self.emit("call_state |= CALL_EFFECTS;\n")
        return True

    def refresh_call(self, tkn, tkn_iter, uop, storage, inst):
        self.macro_args(tkn, tkn_iter, 0)
        self.out.start_line()
        self.emit("if (call_transaction_refresh(tstate, frame, this_instr) < 0) {\n")
        self.reader_error()
        self.emit("}\n")
        self.call_reload(inst)
        return True

    def call_completion(self):
        if self.has_record:
            self.emit("assert(value_temps->count == 0);\n")
        # One out-of-line call, which keeps the transaction record in its own
        # frame and returns {status, child} in two registers (R-c).  The run
        # takes its current instruction from frame->instr_ptr: every
        # preparation restores it to this_instr.
        self.emit("assert(frame->instr_ptr == this_instr);\n")
        self.emit("{\n")
        argc = f"{self.call_argc}, " if self.call_kind != "CALL_TRANSACTION_EX" else ""
        self.emit(f"_PyCallResult call_result = {self.call_run}(tstate, frame, "
                  f"call_base, call_destination, {argc}{self.call_flags} | call_state);\n")
        self.emit("if (call_result.status < 0) {\n")
        self.reader_error()
        self.emit("}\n")
        self.emit("if (call_result.child != NULL) {\n")
        self.emit("DISPATCH_INLINED(call_result.child);\n")
        self.emit("}\n")
        # Both result and callback-current destination are already published.
        # No stack flush or delayed move from window[0] may consume stale C
        # inputs after this point.
        self.emit("slots = _PyFrame_Slots(frame);\n")
        self.emit("DISPATCH();\n")
        self.emit("}\n")

    def invoke_call(self, tkn, tkn_iter, uop, storage, inst):
        self.macro_args(tkn, tkn_iter, 0)
        self.call_completion()
        self.call_finished = True
        return False

    def complete_value_reader(self, tkn, tkn_iter, uop, storage, inst):
        assert inst is not None
        result, first, second = self.macro_args(tkn, tkn_iter, 3)
        dst = FIELD_VARS[self.format_of(inst).field_of(ROLE_DST)]
        # The DSL output is deliberately undefined here. Failed providers
        # restored their inputs; no NULL result may overwrite those homes.
        self.out.start_line()
        self.emit(f"{result} = value_reader_complete(tstate, frame, this_instr, "
                  f"{self.temps}, {dst}, {result}, {first}, {second});\n")
        self.emit(f"if (PyValue_IsNull({result})) {{\n")
        self.reader_error()
        self.emit("}\n")
        self.reader_reload()
        return True

    def abort_value_reader(self, tkn, tkn_iter, uop, storage, inst):
        self.macro_args(tkn, tkn_iter, 0)
        # Never flush the undefined result on this edge: its destination
        # may be one of the just-restored input temporary homes.
        self.emit(f"(void)value_operation_finish(tstate, frame, this_instr, {self.temps});\n")
        self.reader_error()
        return False

    def finish_value_operation(self, tkn, tkn_iter, uop, storage, inst):
        assert inst is not None
        self.macro_args(tkn, tkn_iter, 0)
        storage.flush(self.out)
        self.emit("{\n")
        self.emit(f"int operation_failed = value_operation_finish(tstate, frame, this_instr, {self.temps});\n")
        self.emit("if (operation_failed < 0) {\n")
        self.emit("JUMP_TO_LABEL(error);\n")
        self.emit("}\n")
        self.emit("this_instr = frame->instr_ptr;\n")
        self.emit("ir_word = FT_ATOMIC_LOAD_UINT64_RELAXED(*this_instr);\n")
        self.emit("slots = _PyFrame_Slots(frame);\n")
        if self.format_of(inst).window:
            base = FIELD_VARS[self.format_of(inst).field_of(ROLE_BASE)]
            self.emit(f"window = &IR_SLOT({base});\n")
        self.emit("}\n")
        return True

    def check_inplace_input(self, uop: Uop, name: str,
                            inst: Instruction | None) -> None:
        "Refuse a uop that may keep or release a local's reference"
        use = inplace_input_use(uop, name)
        if use is not None:
            where = inst.name if inst is not None else uop.name
            raise analysis_error(
                f"{where}: {uop.name} uses '{name}', which may be a view of "
                f"a local, as more than a value it reads, releases or takes "
                f"a count of (inplace_input_use())", use)

    def view_field(self, arg: Token | None, storage: Storage) -> str | None:
        "The field whose input arg views, if arg is a variable that is a view"
        if arg is None or arg.kind != "IDENTIFIER":
            return None
        if self.killed_inplace is not None and self.killed_inplace[0] == arg.text:
            return self.killed_inplace[1]
        var = find_local(arg.text, storage)
        if var is None:
            return None
        return var.ir_inplace

    def spend_owner(self, field: str, tkn: Token, storage: Storage) -> None:
        """The body takes or releases the owner of the input read through
        field, which it may do once on a path: another view of the same
        input, a copy, does not own a second count"""
        stack = storage.stack
        assert isinstance(stack, IRStack)
        before = stack.spent.get(field)
        if before is not None:
            raise analysis_error(
                f"{tkn.text}: the owner of the input in field {field} was "
                f"taken or released already, by {before.text} at line "
                f"{before.line}; a view or copy of an input owns no count "
                "of its own", tkn)
        stack.spent[field] = tkn
        # The spend kills every view and copy of the input
        p = stack.field_position(field)
        stack.spent_views.update(stack.views.get(p, []))
        for var in storage.inputs:
            if var.ir_inplace == field and not var.is_dead():
                var.kill()

    def check_spent_views(self, tokens: list[Token], storage: Storage) -> None:
        """No statement reads a view, or a copy of one, after the body took
        or released its input's owner: a view has no count of its own"""
        stack = storage.stack
        if not isinstance(stack, IRStack) or not stack.spent_views:
            return
        for i, tkn in enumerate(tokens):
            if (tkn.kind != "IDENTIFIER" or tkn.text not in stack.spent_views
                    or i > 0 and tokens[i - 1].text in (".", "->")):
                continue
            if i >= 2 and tokens[i - 2].text == "DEAD" \
                    and tokens[i - 1].text == "(":
                continue
            raise analysis_error(
                f"view '{tkn.text}' is read after the body took or released "
                "its input's owner, which killed it and every copy of it",
                tkn)

    def ensure_consumed(self, field: str, storage: Storage) -> None:
        """Before code that reads the owner of the input read through field:
        the handler takes the input, so ir_own_<field> holds its owner"""
        stack = storage.stack
        assert isinstance(stack, IRStack)
        p = stack.field_position(field)
        if stack.owner.get(p) == "in":
            self.out.start_line()
            stack.consume(self.out, p)
            del stack.owner[p]

    def is_pointer(self, name: Token, storage: Storage) -> bool:
        var = find_local(name.text, storage)
        return (var is not None and var.known_object) or name.text == self.killed_pointer

    def value_pointer_variant(self, tkn, tkn_iter, uop, storage, inst):
        lparen = next(tkn_iter)
        assert lparen.kind == "LPAREN"
        arg = tkn_iter.peek()
        assert arg is not None
        field = None if tkn.text == "PyValue_DUP" else self.view_field(arg, storage)
        if field is None:
            self.emit_function_name(tkn, arg, storage)
            self.out.emit(lparen)
            return True
        # The release of a view is its owner's
        name = next(tkn_iter)
        self.spend_owner(field, tkn, storage)
        self.ensure_consumed(field, storage)
        release = VIEW_RELEASES[tkn.text][self.is_pointer(name, storage)]
        self.out.emit_at(release, tkn)
        self.out.emit(lparen)
        self.out.emit(owner_name(field))
        return True

    def value_close_specialized(self, tkn, tkn_iter, uop, storage, inst):
        lparen = next(tkn_iter)
        assert lparen.kind == "LPAREN"
        field = self.view_field(tkn_iter.peek(), storage)
        if field is None:
            return super().value_close_specialized(
                tkn, iter_with_first(lparen, tkn_iter), uop, storage, inst)
        name = next(tkn_iter)
        comma = next(tkn_iter)
        dealloc = next(tkn_iter)
        if comma.kind != "COMMA" or dealloc.kind != "IDENTIFIER":
            raise analysis_error("Expected comma and identifier", comma)
        self.spend_owner(field, tkn, storage)
        self.ensure_consumed(field, storage)
        release = VIEW_RELEASES[tkn.text][self.is_pointer(name, storage)]
        self.out.emit_at(release, tkn)
        self.out.emit(lparen)
        self.out.emit(owner_name(field))
        self.out.emit(comma)
        self.out.emit(dealloc)
        return self.value_kill(name, storage,
                               dealloc.text not in NON_ESCAPING_DEALLOCS)

    def value_steal(self, tkn, tkn_iter, uop, storage, inst):
        lparen = next(tkn_iter)
        assert lparen.kind == "LPAREN"
        arg = tkn_iter.peek()
        var = find_local(arg.text, storage) if arg is not None and arg.kind == "IDENTIFIER" else None
        field = self.view_field(arg, storage)
        box = var.ir_box if var is not None else None
        if var is not None and box is None and var.name in self.box_copies:
            raise analysis_error(
                f"{tkn.text}({var.name}): '{var.name}' is a copy of "
                f"'{self.box_copies[var.name]}', which has an object local",
                tkn)
        if field is None and box is None:
            return super().value_steal(
                tkn, iter_with_first(lparen, tkn_iter), uop, storage, inst)
        name = next(tkn_iter)
        owned = name.text
        if field is not None:
            # The object, with a count of its own for a local
            self.spend_owner(field, tkn, storage)
            self.ensure_consumed(field, storage)
            owned = f"IR_TAKE({name.text}, {owner_name(field)})"
            storage.stack.taken(storage.stack.field_position(field))
        steal = (POINTER_VARIANTS[tkn.text] if self.is_pointer(name, storage)
                 else tkn.text)
        if box is None:
            self.out.emit_at(steal, tkn)
            self.out.emit(lparen)
            self.out.emit(owned)
            return self.value_kill(name, storage, False)
        # An inline value owns nothing: the box, which value_temps owns,
        # gives the count
        rparen = next(tkn_iter)
        if rparen.kind != "RPAREN":
            raise analysis_error("Expected ')'", rparen)
        self.out.emit_at(f"(PyValue_IsInline({name.text}) ? Py_NewRef({box}) "
                         f": {steal}({owned}))", tkn)
        self.out.last_token = rparen
        return self.value_kill(name, storage, False)

    def take_view(self, tkn, tkn_iter, uop, storage, inst):
        """TAKE(x): x's value with a count of its own, for a home.  OWNER(x):
        the owner of x's input, PyValue_NULL for a local, which only
        IR_RETIRE_PUBLISHED may read.  Both kill x; for a value that is not
        a view, both are x itself."""
        lparen = next(tkn_iter)
        name = next(tkn_iter)
        rparen = next(tkn_iter)
        if (lparen.kind != "LPAREN" or name.kind != "IDENTIFIER"
                or rparen.kind != "RPAREN"):
            raise analysis_error(f"{tkn.text}() takes a variable", tkn)
        if find_local(name.text, storage) is None:
            raise analysis_error(
                f"{tkn.text}({name.text}): not a stack variable", name)
        field = self.view_field(name, storage)
        if field is not None:
            self.spend_owner(field, tkn, storage)
            texts = [x.text for x in self.statement.contents] \
                if self.statement is not None else []
            if tkn.text == "TAKE" and len(texts) == 7 and texts[1] == "=" \
                    and texts[2] == "TAKE" and any(
                        var.name == texts[0] for var in storage.outputs):
                self.taken_outputs.add(texts[0])
        if field is None:
            text = name.text
        elif tkn.text == "TAKE":
            self.ensure_consumed(field, storage)
            text = f"IR_TAKE({name.text}, {owner_name(field)})"
            storage.stack.taken(storage.stack.field_position(field))
        else:
            self.ensure_consumed(field, storage)
            text = owner_name(field)
        self.out.emit_at(text, tkn)
        self.out.last_token = rparen
        return self.value_kill(name, storage, False)

    def object_of(self, tkn, tkn_iter, uop, storage, inst):
        """The object of a value that has an object local reads it: the
        input's object, or a box that value_temps owns"""
        lparen = next(tkn_iter)
        assert lparen.kind == "LPAREN"
        arg = tkn_iter.peek()
        var = find_local(arg.text, storage) if arg is not None and arg.kind == "IDENTIFIER" else None
        if var is not None and var.ir_box is None and var.name in self.box_copies:
            raise analysis_error(
                f"{tkn.text}({var.name}): '{var.name}' is a copy of "
                f"'{self.box_copies[var.name]}', whose object local holds "
                "its object; read that", tkn)
        if var is None or var.ir_box is None or tkn_iter.peek() is None:
            self.out.emit(tkn)
            self.out.emit(lparen)
            return True
        name = next(tkn_iter)
        rparen = next(tkn_iter)
        if rparen.kind != "RPAREN":
            raise analysis_error(f"{tkn.text}() of '{name.text}' must be its "
                                 "whole argument", rparen)
        text = (var.ir_box if tkn.text == "PyValue_AsKnownObjectBorrow"
                else f"Py_NewRef({var.ir_box})")
        self.out.emit_at(text, tkn)
        self.out.last_token = rparen
        return True

    def note_pointer_assignment(self, stmt: SimpleStmt, storage: Storage) -> None:
        """After `a = b;` with b a view, a is a view of the same input"""
        super().note_pointer_assignment(stmt, storage)
        tkns = stmt.contents
        if len(tkns) != 4 or tkns[1].kind != "EQUALS" or tkns[-1].kind != "SEMI":
            return
        source = find_local(tkns[2].text, storage)
        if source is None:
            return
        if source.ir_box is not None:
            # The copy holds the value, which may be inline; its object
            # is the source's object local, which object_of() reads
            self.box_copies[tkns[0].text] = tkns[2].text
        if source.ir_inplace is None:
            return
        if self.stores_outputs and any(
                var.name == tkns[0].text for var in storage.outputs) and not any(
                var.name == tkns[0].text for var in storage.inputs):
            # An output that the handler stores owns its value
            raise analysis_error(
                f"output '{tkns[0].text}' would be a view of "
                f"'{tkns[2].text}'; assign it TAKE({tkns[2].text})", tkns[0])
        stack = storage.stack
        assert isinstance(stack, IRStack)
        for var in storage.inputs + storage.outputs:
            if var.name == tkns[0].text:
                var.ir_inplace = source.ir_inplace
                stale = tkns[2].text in stack.stale
                stack.bind(var.name, stack.field_position(source.ir_inplace))
                if stale:
                    stack.stale.add(var.name)
                return
        # inplace_input_use() keeps such an input out of borrow_mask
        raise analysis_error(
            f"'{tkns[0].text}', not a stack variable, would be a view of "
            f"'{tkns[2].text}'", tkns[0])

    def consume_killed(self, storage: Storage) -> None:
        """Clear the slots of the inputs that the handler has taken: a dead
        input's slot holds NULL, on every path"""
        stack = storage.stack
        assert isinstance(stack, IRStack)
        if stack.layout.window and not stack.layout.in_fields:
            return
        for var in storage.inputs:
            p = var.ir_input
            if var.is_dead() and p is not None and stack.owner.get(p) == "in":
                self.out.start_line()
                stack.consume(self.out, p)
                del stack.owner[p]

    def note_view_object(self, stmt: SimpleStmt, storage: Storage,
                         escapes: bool) -> None:
        """After `[T *]h = [(T *)]PyValue_AsKnownObjectBorrow(x);` or
        `[T *]h = [(T *)]_PyValueTemps_Object(temps, x);` with x a view, or
        `[T *]h = [(T *)]g;` with g such a variable, h holds the object of a
        view, uncounted, and dangles with it (S7).  So does a pointer that
        `T *h = <expression>;` or `h = <expression>;`, with h not a stack
        variable, derives from the object in a statement that runs no code
        (Py_TYPE(), a field, an address inside it): S7 follows it as the
        object (fail closed)"""
        stack = storage.stack
        assert isinstance(stack, IRStack)
        tokens = [t for t in stmt.contents if t.kind != "COMMENT"]
        texts = [t.text for t in tokens]
        if "=" not in texts or texts[-1] != ";":
            return
        eq = texts.index("=")
        if eq < 1 or tokens[eq - 1].kind != "IDENTIFIER":
            return
        name = texts[eq - 1]
        rhs = texts[eq + 1:-1]
        while len(rhs) >= 3 and rhs[0] == "(" and ")" in rhs \
                and rhs.index(")") + 1 < len(rhs):
            rhs = rhs[rhs.index(")") + 1:]  # A cast, or casts
        views = {n: p for p, names in stack.views.items() for n in names}
        source = None
        if (len(rhs) == 4 and rhs[0] == "PyValue_AsKnownObjectBorrow"
                and rhs[1] == "(" and rhs[2] in views and rhs[3] == ")"):
            source = views[rhs[2]]
        elif (len(rhs) == 6 and rhs[0] == "_PyValueTemps_Object"
                and rhs[1] == "(" and rhs[3] == "," and rhs[4] in views
                and rhs[5] == ")"):
            source = views[rhs[4]]
        elif len(rhs) == 1 and rhs[0] in stack.view_objects:
            source = stack.view_objects[rhs[0]]
        elif not escapes and (
                eq >= 2 and (texts[eq - 2] == "*" or is_word(tokens[eq - 2])
                             and not is_value_type(texts[eq - 2]))
                or eq == 1 and find_local(name, storage) is None):
            # A declaration of a pointer, or of a type that may be one, or
            # an assignment to a variable of the body: derived if the
            # expression reads the object of a view
            source = object_source(tokens[eq + 1:-1], views,
                                   stack.view_objects)
        stale = len(rhs) == 1 and rhs[0] in stack.stale
        stack.view_objects.pop(name, None)
        stack.stale.discard(name)
        if source is not None:
            stack.view_objects[name] = source
            if stale:
                stack.stale.add(name)

    def check_stale_views(self, tokens: list[Token], storage: Storage,
                          inst: Instruction | None) -> None:
        """S7 (register-ir-v2-a5b.md 4.8): a view of a local dangles if a
        call that runs code rebinds the local, so no view loaded before such
        a call is read after it: the body takes it (TAKE()) before the call.
        A release, DEAD() or OWNER() reads the field's owner, which a local
        does not have, and a copy is a stale view itself."""
        stack = storage.stack
        if not isinstance(stack, IRStack) or not stack.stale:
            return
        texts = [t.text for t in tokens]
        for i, tkn in enumerate(tokens):
            if (tkn.kind != "IDENTIFIER" or tkn.text not in stack.stale
                    or i > 0 and tokens[i - 1].text in (".", "->")):
                continue
            if (i + 1 < len(tokens) and tokens[i + 1].kind == "EQUALS"
                    and (i == 0 or tokens[i - 1].text in (";", "{", "}"))):
                continue  # A new value
            prev = tokens[i - 1].text if i > 0 else ""
            nxt = tokens[i + 1].text if i + 1 < len(tokens) else ""
            if prev in ("(", ",") and nxt in (")", ","):
                callee = enclosing_callee(tokens, i) or ""
                if callee in VIEW_RELEASES or callee in ("DEAD", "OWNER"):
                    # They read the field's owner, which a local does not
                    # have, not the view
                    continue
            if prev == ")" and nxt == ";" and i >= 3 \
                    and tokens[i - 2].text == "void":
                continue
            if nxt == ";" and (prev == "=" or prev == ")" and "=" in texts[:i]
                                and texts[texts.index("=") + 1] == "("):
                # A copy, which is stale too (note_pointer_assignment,
                # note_view_object)
                continue
            what = (f"'{tkn.text}', the object of a view,"
                    if tkn.text in stack.view_objects else f"view '{tkn.text}'")
            raise analysis_error(
                f"{what} is read after a call that can run code and rebind "
                "its local (S7); TAKE() the view before the call", tkn)

    def check_unsequenced_views(self, stmt: SimpleStmt, call: Token,
                                storage: Storage) -> None:
        """S7 within a statement: C does not order the operands of an
        expression, so a view, or the object of one, read in the statement
        of a call that can run code, outside the call's designator and
        arguments (which C evaluates before the call), may be read after
        the call"""
        stack = storage.stack
        if not isinstance(stack, IRStack):
            return
        names = {n for ns in stack.views.values() for n in ns}
        names |= set(stack.view_objects)
        if not names:
            return
        tokens = [t for t in stmt.contents if t.kind != "COMMENT"]
        call_at = next((i for i, t in enumerate(tokens) if t is call), None)
        if call_at is None or call_at + 1 >= len(tokens) \
                or tokens[call_at + 1].text != "(":
            return
        start = designator_start(tokens, call_at) - 1
        depth = 0
        end = call_at + 1
        for end in range(call_at + 1, len(tokens)):
            if tokens[end].text == "(":
                depth += 1
            elif tokens[end].text == ")":
                depth -= 1
                if depth == 0:
                    break
        for i, tkn in enumerate(tokens):
            if (start < i < end or tkn.kind != "IDENTIFIER"
                    or tkn.text not in names
                    or i > 0 and tokens[i - 1].text in (".", "->")):
                continue
            if i + 1 < len(tokens) and tokens[i + 1].kind == "EQUALS" \
                    and (i == 0 or tokens[i - 1].text in ("*", "PyObject",
                                                          "PyValue")):
                continue  # The target of the statement
            raise analysis_error(
                f"'{tkn.text}' is read in the statement of {call.text}(), "
                "which can run code, outside its arguments: C may read it "
                "after the call (S7); read it in a statement before", tkn)

    def object_condition(self, condition: list[Token],
                         storage: Storage) -> list[Token]:
        """A condition, which the emitter writes as it is, with the object
        of each value that has an object local read through it, as
        object_of() reads it in a statement"""
        tokens = list(condition)
        out: list[Token] = []
        i = 0
        while i < len(tokens):
            tkn = tokens[i]
            var = None
            if (tkn.text in VALUE_TO_OBJECT and i + 3 < len(tokens)
                    and tokens[i + 1].text == "(" and tokens[i + 3].text == ")"):
                var = find_local(tokens[i + 2].text, storage)
            if var is not None and (var.ir_box is not None
                                    or var.name in self.box_copies):
                if (tkn.text != "PyValue_AsKnownObjectBorrow"
                        or var.ir_box is None):
                    raise analysis_error(
                        f"{tkn.text}({var.name}) in a condition: read the "
                        "object in a statement", tkn)
                out.append(replace(tkn, kind="IDENTIFIER", text=var.ir_box,
                                   end=tokens[i + 3].end))
                i += 4
                continue
            out.append(tkn)
            i += 1
        return out

    def emit_IfStmt(self, stmt, uop, storage, inst):
        self.check_stale_views(list(stmt.condition), storage, inst)
        self.check_spent_views(list(stmt.condition), storage)
        stmt = replace(stmt, condition=self.object_condition(stmt.condition, storage))
        return super().emit_IfStmt(stmt, uop, storage, inst)

    def emit_WhileStmt(self, stmt, uop, storage, inst):
        self.check_stale_views(list(stmt.condition), storage, inst)
        self.check_spent_views(list(stmt.condition), storage)
        stmt = replace(stmt, condition=self.object_condition(stmt.condition, storage))
        return super().emit_WhileStmt(stmt, uop, storage, inst)

    def before_statement(self, stmt: SimpleStmt, storage: Storage,
                         inst: Instruction | None) -> None:
        """Take each input whose owner the statement reads, so that the
        statement can read it; check OWNER's and S7's rules"""
        tokens = [t for t in stmt.contents if t.kind != "COMMENT"]
        self.check_stale_views(tokens, storage, inst)
        self.check_spent_views(tokens, storage)
        texts = [t.text for t in tokens]
        if self.pending_owner is not None and self.pending_owner.text in texts \
                and texts[0] not in ("IR_RETIRE_PUBLISHED",
                                     "IR_RETIRE_PUBLISHED_RESULT",
                                     "IR_RETIRE_PUBLISHED_PAIR"):
            raise analysis_error(
                f"'{self.pending_owner.text}', the owner that OWNER() gave, is "
                "NULL for a local: only a published-retirement marker may "
                "read it; "
                "use TAKE()", tokens[texts.index(self.pending_owner.text)])
        for i, tkn in enumerate(tokens):
            if (tkn.text in OWNER_READS and i + 2 < len(tokens)
                    and texts[i + 1] == "(" and tokens[i + 2].kind == "IDENTIFIER"):
                field = self.view_field(tokens[i + 2], storage)
                if field is not None:
                    self.ensure_consumed(field, storage)
            if tkn.text == "OWNER":
                if not (len(texts) == 8 and texts[0] == "PyValue" and i == 3
                        and texts[2] == "=" and texts[-1] == ";"):
                    raise analysis_error(
                        "OWNER() is the initializer of a PyValue variable, "
                        "`PyValue receiver = OWNER(owner);`", tkn)
                self.pending_owner = tokens[1]

    def emit_SimpleStmt(self, stmt: SimpleStmt, uop: CodeSection,
                        storage: Storage, inst: Instruction | None
                        ) -> tuple[bool, Token | None, Storage]:
        """A statement that kills an input, as DEAD(), a close or a steal,
        gives up its slot"""
        transaction_marker = any(t.text in CALL_TRANSACTION_MARKERS
                                 for t in stmt.contents)
        if inst is not None and uses_call_transaction(inst):
            transaction_marker |= any(t.text in {
                "_Py_Specialize_Call", "_Py_Specialize_CallKw",
                "_Py_Specialize_CallFunctionEx"} for t in stmt.contents)
        previous = self.cannot_escape
        self.statement = stmt
        uops = [part for part in inst.parts if isinstance(part, Uop)] if inst else []
        self.stores_outputs = bool(uops) and uops[-1] is uop
        escape = uop.properties.escaping_calls.get(stmt)
        if escape is not None and self.pending_owner is not None:
            raise analysis_error(
                f"code can run between OWNER() and IR_RETIRE_PUBLISHED, so "
                f"'{self.pending_owner.text}' must own a count; use TAKE()",
                escape.call)
        self.before_statement(stmt, storage, inst)
        if escape is not None:
            self.check_unsequenced_views(stmt, escape.call, storage)
        if transaction_marker:
            # Their handlers explicitly publish/refresh captured homes and
            # terminal state. Automatic post-call spill/reload would instead
            # republish stale C inputs after actual destination publication.
            self.cannot_escape = True
        # An escaping release kills its input before the statement is
        # emitted (Emitter.emit_SimpleStmt); remember whether it is a view
        self.killed_inplace = None
        if escape is not None and escape.kills is not None and not self.cannot_escape:
            killed = find_local(escape.kills.text, storage)
            if killed is not None and killed.ir_inplace is not None:
                self.killed_inplace = (killed.name, killed.ir_inplace)
        try:
            reachable, tkn, storage = super().emit_SimpleStmt(stmt, uop, storage, inst)
        finally:
            self.cannot_escape = previous
            self.statement = None
            self.killed_inplace = None
        stack = storage.stack
        assert isinstance(stack, IRStack)
        for target in uop.local_stores:
            if target in stmt.contents:
                var = find_local(target.text, storage)
                if var is not None and var.ir_inplace is None and not var.is_array():
                    stack.bind(var.name, None)
        self.note_view_object(stmt, storage, escape is not None)
        if escape is not None or transaction_marker:
            stack.code_ran()
        if reachable:
            self.consume_killed(storage)
        return reachable, tkn, storage

    def decref_inputs(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        """In register style, close each input from the top after clearing
        its slot; an output defined before stays in its variable.  A view's
        release is its owner's."""
        stack = storage.stack
        assert isinstance(stack, IRStack)
        if stack.layout.window or self.cannot_escape:
            if any(var.ir_inplace is not None for var in storage.inputs):
                raise analysis_error(
                    "DECREF_INPUTS here would release a view", tkn)
            return super().decref_inputs(tkn, tkn_iter, uop, storage, inst)
        next(tkn_iter)
        next(tkn_iter)
        next(tkn_iter)
        storage.clear_dead_inputs()
        for var in reversed(storage.inputs):
            if var.is_dead():
                continue
            if var.is_array():
                raise analysis_error("register-style array input", tkn)
            close = "PyValue_XCLOSE" if "null" in var.name else "PyValue_CLOSE"
            name = var.name
            if var.ir_inplace is not None:
                close = "PyValue_XCLOSE"
                self.spend_owner(var.ir_inplace, tkn, storage)
                self.ensure_consumed(var.ir_inplace, storage)
                name = owner_name(var.ir_inplace)
            if var.known_object:
                close = f"_{close}_POINTER"
            self.out.start_line()
            p = var.ir_input
            if p is not None and stack.owner.get(p) == "in":
                stack.consume(self.out, p)
                del stack.owner[p]
            self.emit(f"{close}({name});\n")
            var.kill()
        storage.clear_inputs("after DECREF_INPUTS")
        stack.code_ran()
        return True

    def goto_error(self, offset: int, storage: Storage) -> str:
        """Clear the slots that the stack gave up, which the stack machine
        left above its stack pointer, and leave the rest to
        exception_unwind."""
        stack = storage.stack.copy()
        assert isinstance(stack, IRStack)
        stack.error_exit = True
        stack.flush(self.out)
        return f"{self.jump_prefix}JUMP_TO_LABEL(error);"

    def check_exit_inputs(self, tkn: Token, storage: Storage,
                          what: str | None = None) -> None:
        """A DEOPT_IF or EXIT_IF goes to the generic instruction, and
        DISPATCH_SAME_OPARG runs the handler again: either reads the inputs
        from their slots, so none may have been taken or consumed"""
        stack = storage.stack
        if not isinstance(stack, IRStack) or stack.layout.window:
            return
        consumed = [p for p in sorted(stack.layout.borrowable)
                    if stack.owner.get(p) != "in"]
        if consumed or stack.spent:
            field = (stack.layout.in_fields[consumed[0]] if consumed
                     else next(iter(stack.spent)))
            raise analysis_error(
                f"{what or tkn.text} after the handler took the input in field "
                f"{field}: the instruction it goes to would read a cleared "
                "slot, and the taken value would leak; test before the "
                "TAKE() or release", tkn)

    def check_owner_exit(self, tkn: Token) -> None:
        """No error exit between OWNER() and IR_RETIRE_PUBLISHED: the owner
        that OWNER() gave holds a temporary's count, which the exit would
        leak"""
        if self.pending_owner is not None:
            raise analysis_error(
                f"{tkn.text} between OWNER() and IR_RETIRE_PUBLISHED would "
                f"leak '{self.pending_owner.text}'; exit before the OWNER() "
                "or after the retire", tkn)

    def deopt_if(self, tkn, tkn_iter, uop, storage, inst):
        self.check_exit_inputs(tkn, storage)
        return super().deopt_if(tkn, tkn_iter, uop, storage, inst)

    def exit_if(self, tkn, tkn_iter, uop, storage, inst):
        self.check_exit_inputs(tkn, storage)
        return super().exit_if(tkn, tkn_iter, uop, storage, inst)

    def check_error_outputs(self, tkn: Token, storage: Storage) -> None:
        """An error exit stores no output, so no output may hold a TAKE()
        result then: the exit would leak its count"""
        for var in storage.outputs:
            if var.name in self.taken_outputs and var.in_local:
                raise analysis_error(
                    f"{tkn.text} while output '{var.name}' holds a TAKE() "
                    "result, whose count the error exit would leak; exit "
                    "before the TAKE()", tkn)

    def error_if(self, tkn, tkn_iter, uop, storage, inst):
        self.check_owner_exit(tkn)
        self.check_error_outputs(tkn, storage)
        return super().error_if(tkn, tkn_iter, uop, storage, inst)

    def goto_label(self, goto: Token, label: Token, storage: Storage) -> None:
        self.check_owner_exit(goto)
        super().goto_label(goto, label, storage)

    def error_no_pop(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        "The inputs stay in their slots"
        self.check_owner_exit(tkn)
        self.check_error_outputs(tkn, storage)
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
        storage.stack.flush(self.out)
        if inst is not None and self.format_of(inst).store_dst:
            self.release_displaced()
        self.emit(tkn)
        return False

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
        """The handler runs again on the same word, and reads its inputs
        again; the path ends here"""
        assert inst is not None
        self.check_exit_inputs(tkn, storage)
        if uses_call_transaction(inst):
            self.emit(f"if (call_state & CALL_EFFECTS) {{ goto {self.call_fallback}; }}\n")
            self.call_fallback_used = True
        self.emit(tkn)
        return False

    def emit_jump_to_predicted(self, inst: Instruction | None) -> None:
        assert inst is not None
        if uses_call_transaction(inst):
            self.emit(f"if (call_state & CALL_EFFECTS) {{ goto {self.call_fallback}; }}\n")
            self.call_fallback_used = True
        fmt = self.format_of(inst)
        binary_op = inst.family is not None and inst.family.name == BINARY_OP_FAMILY
        if binary_op:
            # The handler of the operation that the entry records
            self.emit(f"UPDATE_MISS_STATS({BINARY_OP_FAMILY});\n")
            self.emit(f"IR_DEOPT_BINARY_OP({field_expr(fmt.fb)});\n")
            return
        if inst.family is not None and inst.family.name in FUSED_JUMPS:
            # The head has no opcode: the miss counts for the compare
            self.emit(f"UPDATE_MISS_STATS({FUSED_SHADOWS[inst.family.name]});\n")
            self.emit(f"JUMP_TO_PREDICTED({self.jump_prefix}{inst.family.name});\n")
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
        its dead inputs, handed them to a pushed frame, or taken explicit
        responsibility for draining private argument homes before any exit.
        The producer must not auto-vacate those homes."""
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
        """The inputs that uop reads as objects (PyObject *).  A guard exits
        if one holds an inline value; it is then known to be an object for
        the rest of the instruction.  Any other uop reads it through an
        object local, ir_obj_<name>: the input's object, or a box of an
        inline value that value_temps owns, made out of line.  The input
        keeps its canonical value (register-ir-v2-a5b.md, 4.5)."""
        for var in storage.inputs:
            if var.name in uop.asserted_objects:
                var.known_object = True
            if var.ir_inplace is not None:
                self.check_inplace_input(uop, var.name, inst)
        if uop.properties.inline_ok:
            return
        guard = uop.is_guard()
        for var in storage.inputs:
            if (var.name not in uop.object_inputs or var.known_object
                    or var.ir_box is not None):
                continue
            self.out.start_line()
            # Test the slot rather than the local where the slot holds the
            # value, which is cheaper (see _PyValue_TopBitsAt())
            ref = var.name
            at = ""
            if var.memory_offset is not None and var.ir_inplace is None:
                ref = f"&{storage.stack.address(var.memory_offset)}"
                at = "At"
            if guard:
                if inst is not None:
                    self.check_exit_inputs(inst.where, storage,
                                           f"The guard of {uop.name}")
                self.emit(f"if (Py_UNLIKELY(!PyValue_IsPointerSpace{at}({ref}))) {{\n")
                self.emit_jump_to_predicted(inst)
                self.emit("}\n")
                var.known_object = True
                continue
            box = f"ir_obj_{var.name}"
            self.boxes.add(box)
            self.emit(f"if (Py_UNLIKELY(PyValue_IsInline{at}({ref}))) {{\n")
            self.emit(f"{box} = _PyValueTemps_Box(value_temps, {var.name});\n")
            self.emit(f"if ({box} == NULL) {{\n")
            # Every input is still in its slot: exception_unwind has them
            storage.stack.copy().flush(self.out)
            self.emit(f"{self.jump_prefix}JUMP_TO_LABEL(error);\n")
            self.emit("}\n")
            if inst is not None:
                self.emit(f"BOX_STAT_INC({inst.name});\n")
            self.emit("}\n")
            self.emit("else {\n")
            self.emit(f"{box} = PyValue_AsKnownObjectBorrow({var.name});\n")
            self.emit("}\n")
            var.ir_box = box


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
    emitter.box_copies = {}
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
    emitter.emit_object_inputs(uop, storage, inst)
    reachable, storage = emitter.emit_tokens(uop, storage, inst, False)
    if emitter.pending_owner is not None:
        raise analysis_error(
            f"OWNER() gave '{emitter.pending_owner.text}', which no "
            f"IR_RETIRE_PUBLISHED marker of {uop.name} retires",
            emitter.pending_owner)
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
        # The top inputs of a window with sources are read through fields
        in_fields = {k - fmt.n_inputs: fmt.input_field(k)
                     for k in range(fmt.n_inputs)}
        borrowable = frozenset(k - fmt.n_inputs for k in range(fmt.n_inputs)
                               if fmt.borrow_mask & (1 << k))
        return SlotLayout(True, get_stack_effect(inst).base_offset, in_fields,
                          borrowable)
    in_fields = {k - fmt.n_inputs: fmt.input_field(k)
                 for k in range(fmt.n_inputs)}
    borrowable = frozenset(k - fmt.n_inputs for k in range(fmt.n_inputs)
                           if fmt.borrow_mask & (1 << k))
    return SlotLayout(False, PointerOffset.zero(), in_fields, borrowable,
                      dst=fmt.field_of(ROLE_DST), dst_offset=fmt.dst_offset,
                      store_dst=mode == STORE_RESULT)


def polls_at_end(name: str, inst: Instruction) -> bool:
    """Whether the call transaction of handler name polls the eval breaker
    when the call returns, as _CHECK_PERIODIC_AT_END does"""
    return any(isinstance(p, Uop) and p.name == "_CHECK_PERIODIC_AT_END"
               for p in inst.parts) or name == "CALL_KW_NON_PY"


def uses_call_transaction(inst: Instruction) -> bool:
    return any(isinstance(part, Uop) and
               any(token.text == "IR_INVOKE_CALL" for token in part.body.tokens())
               for part in inst.parts)


def value_bridge_bound(name: str, inst: Instruction) -> int:
    """The most input boxes that handler name can hold in value_temps.

    Publication frees a result's box at once (the release contract), so
    only inputs that a handler boxes for an object API stay until the
    handler finishes.  An array input can box several values (BUILD_SLICE
    boxes three, RAISE_VARARGS two), so the bound keeps a slot per output
    as well as per input, and two more.  Window loops use their own object
    arrays."""
    return 2 + sum(
        len(part.stack.inputs) + len(part.stack.outputs)
        for part in inst.parts if isinstance(part, Uop)
    )


def generate_handler(name: str, inst: Instruction, fmt: IRFormat,
                     emitter: IREmitter, out: CWriter, record: bool) -> None:
    """The handler name, which runs inst: inst itself, or for a handler of a
    BINARY_OP operation, BINARY_OP with that operation.

    With record, the handler sets up a value record, value_temps, for the
    boxes it makes of inline inputs.  Without it, value_temps is NULL, so
    the dispatch macros finish no bridges: only a handler that boxes
    (boxing_handlers, the contract's BOXES) needs one.

    The handler is generated twice.  The first pass, to no output, finds
    the role of each C variable, a view or an owner, and the object
    locals; the second declares them, a view as a PyValueBorrow (S8).  A
    name with both roles is refused: the DSL gives the owner another name,
    as `taken = TAKE(value)` does."""
    emitter.result_publication = result_publication_contract(inst, fmt)
    saved = emitter.out
    roles: dict[str, set[str]] = {}
    emitter.boxes = set()
    emitter.declared_roles = {}
    emitter.out = CWriter.null()
    try:
        emit_handler(name, inst, fmt, emitter, emitter.out, record, roles)
    finally:
        emitter.out = saved
    for var, kinds in sorted(roles.items()):
        if len(kinds) > 1:
            raise analysis_error(
                f"{name}: '{var}' is a view (PyValueBorrow) and an owner "
                f"(PyValue) in one handler; give one of them another name",
                inst.where)
    emitter.declared_roles = roles
    emit_handler(name, inst, fmt, emitter, out, record, {})


def emit_handler(name: str, inst: Instruction, fmt: IRFormat,
                 emitter: IREmitter, out: CWriter, record: bool,
                 roles: dict[str, set[str]]) -> None:
    "One pass of generate_handler, which records the roles it finds"
    emitter.pending_owner = None
    emitter.taken_outputs = set()
    operation = None
    emitter.has_record = record
    if name != inst.name:
        operation = "NB_" + name.removeprefix(BINARY_OP_FAMILY + "_")
    transaction = uses_call_transaction(inst)
    emitter.call_finished = False
    emitter.call_fallback_used = False
    emitter.call_fallback = f"call_transaction_fallback_{name}"
    is_target = inst.is_target and name == inst.name
    out.emit("\n")
    out.emit(f"TARGET({name}) {{\n")
    bridge_bound = value_bridge_bound(name, inst)
    out.emit(f"int opcode = {name};\n")
    out.emit(f"(void)(opcode);\n")
    if not record:
        # No value record: the dispatch macros finish no bridges
        out.emit("_PyValueTemps *const value_temps = NULL;\n")
        out.emit("(void)value_temps;\n")
    if inst.properties.needs_prev:
        out.emit(f"_PyIRWord* const prev_instr = frame->instr_ptr;\n")
    if not is_target:
        qualifier = "" if transaction or inst.name == "BINARY_OP" or name in ("CALL", "CALL_KW", "INSTRUMENTED_CALL", "INSTRUMENTED_CALL_KW", "CALL_NON_PY_GENERAL", "CALL_KW_NON_PY", "CALL_BUILTIN_O", "LOAD_ATTR_SLOT", "STORE_ATTR_SLOT",
                                   "BUILD_MAP", "STORE_SUBSCR", "STORE_SUBSCR_DICT",
                                   "STORE_ATTR_WITH_HINT", "BINARY_OP_SUBSCR_DICT", "LOAD_GLOBAL",
                                   "LOAD_ATTR", "STORE_ATTR",
                                   "STORE_ATTR_INSTANCE_VALUE", "STORE_ATTR_SHAPE", "STORE_ATTR_SHAPE_ADD",
                                   "LOAD_ATTR_INSTANCE_VALUE", "LOAD_ATTR_SHAPE",
                                   "LOAD_ATTR_MODULE", "LOAD_ATTR_WITH_HINT") else "const "
        out.emit(f"_PyIRWord* {qualifier}this_instr = next_instr;\n")
        out.emit("(void)this_instr;\n")
    if not inst.properties.no_save_ip:
        out.emit(f"frame->instr_ptr = next_instr;\n")
    out.emit(f"next_instr += 1;\n")
    out.emit(f"INSTRUCTION_STATS({name});\n")
    if is_target:
        qualifier = "" if transaction or inst.name == "BINARY_OP" or name in ("CALL", "CALL_KW", "STORE_SUBSCR", "LOAD_GLOBAL", "LOAD_ATTR", "STORE_ATTR") else "const "
        out.emit(f"_PyIRWord* {qualifier}this_instr = next_instr - 1;\n")
        out.emit("(void)this_instr;\n")
    if record:
        # The handler's value record, the box area (R27 deletes it)
        out.emit(f"PyObject *value_boxes[{bridge_bound}];\n")
        out.emit("_PyValueTemps value_owners;\n")
        out.emit(f"value_owners = (_PyValueTemps){{value_boxes, 0, {bridge_bound}}};\n")
        out.emit("_PyValueTemps *value_temps = &value_owners;\n")
    if fmt.store_dst:
        # The old value of a local that dst names, released after the
        # result is stored (release_displaced)
        out.emit("PyValue ir_displaced = PyValue_NULL;\n")
        out.emit("(void)ir_displaced;\n")
    use_fb = fmt.fb != LOC_NONE and (
        uses_feedback(inst) or reads_fb_arg(inst, fmt, operation))
    emitter.call_has_feedback = use_fb
    if use_fb:
        out.emit(f"{feedback_type(fmt)} *fb = ({feedback_type(fmt)} *)"
                 f"IR_FB({field_expr(fmt.fb)});\n")
    if inst.properties.oparg:
        if operation is not None:
            out.emit(f"int oparg = {operation};\n")
        elif fmt.fused:
            # D is the distance; the arg of the entry holds the operation
            out.emit("int oparg = (int)(((_PyFbHeader *)fb)->arg & IR_FUSED_OP_MASK);\n")
        elif fmt.imm == LOC_FBARG and is_specialization(inst):
            out.emit("int oparg = (int)((_PyFbHeader *)fb)->arg;\n")
        elif fmt.imm == LOC_FBARG:
            out.emit(f"int oparg = IR_FB_ARG(fb, {field_expr(fmt.fb)});\n")
        elif fmt.forward and field_expr(fmt.imm).startswith("IR_D("):
            # The distance has a sign, as _PyCode_InstrOparg() decodes it
            out.emit("int oparg = IR_DS(ir_word);\n")
        else:
            out.emit(f"int oparg = (int){field_expr(fmt.imm)};\n")
        out.emit("(void)oparg;\n")
    if fmt.window:
        base = FIELD_VARS[fmt.field_of(ROLE_BASE)]
        out.emit(f"PyValue *window = &IR_SLOT({base});\n")
        out.emit("(void)window;\n")
    call_family = inst.family.name if inst.family is not None else inst.name
    if call_family == "INSTRUMENTED_CALL":
        call_family = "CALL"
    elif call_family == "INSTRUMENTED_CALL_KW":
        call_family = "CALL_KW"
    if transaction:
        assert fmt.window
        inputs, _ = external_io(inst)
        count = " + ".join(f"({item.size or '1'})" for item in inputs)
        kind = ("CALL_TRANSACTION_EX" if call_family in
                ("CALL_FUNCTION_EX", "INSTRUMENTED_CALL_FUNCTION_EX") else
                "CALL_TRANSACTION_KEYWORDS" if call_family == "CALL_KW" else
                "CALL_TRANSACTION_VECTOR")
        # call_transaction_run_*() derives the window's size from its kind
        fixed = sum(1 for item in inputs if not item.size)
        arrays = [item.size for item in inputs if item.size]
        expected = {"CALL_TRANSACTION_VECTOR": (2, ["oparg"]),
                    "CALL_TRANSACTION_KEYWORDS": (3, ["oparg"]),
                    "CALL_TRANSACTION_EX": (4, [])}[kind]
        if (fixed, arrays) != expected:
            raise analysis_error(
                f"{name}: a {kind} call with inputs {count}", inst.where)
        base = FIELD_VARS[fmt.field_of(ROLE_BASE)]
        dst = FIELD_VARS[fmt.field_of(ROLE_DST)]
        emitter.call_argc = "oparg" if inst.properties.oparg else "0"
        emitter.call_kind = kind
        emitter.call_run = {"CALL_TRANSACTION_VECTOR": "call_transaction_run_vector",
                            "CALL_TRANSACTION_KEYWORDS": "call_transaction_run_keywords",
                            "CALL_TRANSACTION_EX": "call_transaction_run_ex"}[kind]
        flags = []
        if name.startswith("INSTRUMENTED_"):
            flags.append("CALL_INSTRUMENTED")
        if polls_at_end(name, inst):
            flags.append("CALL_PERIODIC")
        emitter.call_flags = "(" + (" | ".join(flags) or "0") + ")"
        # The transaction's state is in registers: its numeric homes, read
        # once, and what the handler did (register-ir-v2-a3.md 3.5, R-b)
        out.emit(f"int call_base = {base};\n")
        out.emit(f"int call_destination = {dst};\n")
        out.emit("int call_state = 0;\n")
        out.emit(f"call_transaction_begin(frame, call_base, {count}, call_destination);\n")
        if emitter.call_has_feedback:
            out.emit(f"uint32_t call_feedback = {field_expr(fmt.fb)};\n")
    elif fmt.window and call_family in ("CALL", "CALL_KW"):
        inputs, _ = external_io(inst)
        count = " + ".join(f"({item.size or '1'})" for item in inputs)
        out.emit(f"_PyValue_RecheckWindow(window, {count});\n")
    if inst.properties.uses_opcode:
        out.emit(f"opcode = {name};\n")
    mode = store_mode(inst, external_io(inst)[1]) if fmt.store_dst else None
    if mode == STORE_LEAVE:
        out.emit(f"if (IR_IS_LOCAL({FIELD_VARS[fmt.field_of(ROLE_DST)]})) {{\n")
        emitter.emit_jump_to_predicted(inst)
        out.emit("}\n")
    elif mode == STORE_NEVER:
        out.emit(f"assert(!IR_IS_LOCAL({FIELD_VARS[fmt.field_of(ROLE_DST)]}));\n")
    declare_variables(inst, out, emitter.declared_roles)
    for k in range(fmt.n_inputs):
        if fmt.borrow_mask & (1 << k):
            own = owner_name(fmt.input_field(k))
            out.emit(f"PyValue {own} = PyValue_NULL;\n")
            out.emit(f"(void){own};\n")
    for box in sorted(emitter.boxes):
        out.emit(f"PyObject *{box} = NULL;\n")
    stack: Stack = IRStack(slot_layout(inst, fmt, mode), roles)
    insert_braces = len([p for p in inst.parts if isinstance(p, Uop)]) > 1
    reachable = True
    for part in inst.parts:
        try:
            reachable, stack = write_uop(part, emitter, stack, inst, insert_braces)
        except SyntaxError as ex:
            ex.msg = f"{name}: {ex.msg}"
            raise
        if emitter.call_finished:
            break
    out.start_line()
    if reachable:
        assert isinstance(stack, IRStack)
        stack.final = True
        stack.flush(out)
        if mode == STORE_RESULT and fmt.window:
            # The result is in its slot of the window; a local takes it
            dst = FIELD_VARS[fmt.field_of(ROLE_DST)]
            macro = "IR_MOVE_TO_DST_OR_DISCARD" if fmt.discards else "IR_MOVE_TO_DST"
            stack.dst_stores.append(f"{macro}({dst}, window[{fmt.dst_offset}])")
            out.emit(f"{stack.dst_stores[-1]};\n")
        if fmt.store_dst:
            emitter.release_displaced()
        if fmt.unpacks:
            emitter.unpack_to_locals(fmt, UNPACKS[fmt.group])
        out.emit("DISPATCH();\n")
    if transaction and emitter.call_fallback_used:
        out.emit(f"{emitter.call_fallback}:;\n")
        emitter.call_completion()
    out.start_line()
    out.emit("}")
    out.emit("\n")
    assert isinstance(stack, IRStack)
    if len(stack.dst_stores) > 1:
        raise SyntaxError(f"{name}: {len(stack.dst_stores)} dst stores "
                          f"({'; '.join(stack.dst_stores)}), but ir_displaced "
                          "holds one displaced local")


def generate_ir_cases(analysis: Analysis, formats: dict[str, IRFormat],
                      boxing: set[str], outfile: TextIO, lines: bool) -> None:
    out = CWriter(outfile, 2, lines)
    emitter = IREmitter(out, analysis, formats)
    out.emit("\n")
    for name, inst in sorted(handler_instructions(analysis).items()):
        emitter.handler_format = formats[name]
        generate_handler(name, inst, formats[name], emitter, out,
                         name in boxing)
        emitter.handler_format = None


def generate_ir_labels(analysis: Analysis, emitter: Emitter) -> None:
    emitter.emit("\n")
    for name, label in analysis.labels.items():
        emitter.emit(f"LABEL({name})\n")
        emitter.emit("{\n")
        emitter.emit("_PyValueTemps *const value_temps = NULL;\n")
        storage = Storage(IRStack(), [], [], 0, False)
        if label.spilled:
            storage.spilled = 1
            storage.stack.physical_sp = None
        emitter.emit_tokens(label, storage, None)
        emitter.emit("}\n\n")


# The columns of the instruction contract (pycore_ir.h, _PyIRFormat) that
# describe a handler rather than its format.

EXT_FIXED, EXT_OPARG, EXT_OPARG2, EXT_BIT0, EXT_BYTES = (
    "_PyIR_EXT_FIXED", "_PyIR_EXT_OPARG", "_PyIR_EXT_OPARG2",
    "_PyIR_EXT_BIT0", "_PyIR_EXT_BYTES")

# The array sizes of the DSL, without spaces: the extent form each adds and
# its constant.  UNPACK_EX's two halves add up to EXT_BYTES.
ARRAY_SIZES = {
    "oparg": (EXT_OPARG, 0),
    "oparg-1": (EXT_OPARG, -1),
    "oparg-2": (EXT_OPARG, -2),
    "oparg*2": (EXT_OPARG2, 0),
    "oparg&1": (EXT_BIT0, 0),
    "oparg&0xFF": ("low byte", 0),
    "oparg>>8": ("high byte", 0),
}

Extent = tuple[int, str]


def extent(items: list[StackItem], inst: Instruction) -> Extent:
    "The slots that items cover, as (k, form): k plus the form's oparg part"
    k = 0
    forms = []
    for item in items:
        size = item.size.replace(" ", "")
        if not item.is_array():
            k += 1
        elif size.isdigit():
            k += int(size)
        elif size in ARRAY_SIZES:
            form, c = ARRAY_SIZES[size]
            k += c
            forms.append(form)
        else:
            raise analysis_error(
                f"no extent form for the array size {item.size} of "
                f"{item.name}", inst.where)
    if not forms:
        return k, EXT_FIXED
    if sorted(forms) == ["high byte", "low byte"]:
        return k, EXT_BYTES
    if len(forms) == 1 and forms[0] in (EXT_OPARG, EXT_OPARG2, EXT_BIT0):
        return k, forms[0]
    raise analysis_error(f"no extent form for {forms}", inst.where)


def changes(uop: Uop, name: str, array: bool = False) -> bool:
    """Whether the body of uop may change the variable name: it assigns to
    the variable or to an element of it, increments or decrements it, or
    takes its address, through which a callee may write it.  An array also
    changes where the body uses its name other than to index it, since the
    name is then a pointer to the elements."""
    tokens = [t for t in uop.body.tokens() if t.kind != "COMMENT"]
    for i, tkn in enumerate(tokens):
        if tkn.text != name:
            continue
        prev = tokens[i - 1] if i > 0 else None
        if prev is not None and prev.kind in ("ARROW", "PERIOD"):
            continue    # a member of another variable
        if prev is not None and prev.kind in ("PLUSPLUS", "MINUSMINUS"):
            return True
        if prev is not None and prev.kind == "AND" and (
                i < 2 or tokens[i - 2].kind not in OPERANDS):
            return True
        j = i + 1
        if array and (j >= len(tokens) or tokens[j].kind != "LBRACKET"):
            return True
        while j < len(tokens) and tokens[j].kind == "LBRACKET":
            depth = 0
            for j in range(j, len(tokens)):
                depth += {"LBRACKET": 1, "RBRACKET": -1}.get(tokens[j].kind, 0)
                if depth == 0:
                    break
            j += 1
        if j < len(tokens) and tokens[j].kind in ASSIGNMENTS:
            return True
    return False


def changed_inputs(inst: Instruction) -> set[int]:
    """The inputs of inst, bottom first as external_io() numbers them, that a
    uop may change (changes()) while they are on the stack.  It walks the
    stack as external_io() does."""
    stack: list[int] = []       # external index, top first, or -1
    n = 0
    changed = set()
    for part in inst.parts:
        if not isinstance(part, Uop):
            continue
        popped: list[int] = []  # top first
        for item in reversed(part.stack.inputs):
            if stack:
                ext = stack.pop()
            else:
                ext = n
                n += 1
            if (ext >= 0 and item.name != "unused"
                    and changes(part, item.name, item.is_array())):
                changed.add(ext)
            popped.append(ext)
        popped.reverse()
        for j in range(len(part.stack.outputs)):
            i = passed_through(part, j)
            stack.append(popped[i] if i is not None else -1)
        if any(token.text == "IR_INVOKE_CALL" for token in part.body.tokens()):
            break
    return {n - 1 - ext for ext in changed}


def through_items(inst: Instruction, inputs: list[StackItem],
                  outputs: list[Entry]) -> list[StackItem]:
    """The deepest inputs that keep their slot and their value: each is the
    output at its own position, as an input that the output passes through
    or an array that an array of the same name and size there leaves alone,
    and no uop changes it (changed_inputs()).  The count stops at the first
    input that does not, so that an unchanged input above a changed one, as
    the middle of SWAP, is not counted."""
    changed = changed_inputs(inst)
    result = []
    for j, (item, out) in enumerate(zip(inputs, outputs)):
        if j in changed:
            break
        if out.external == j:
            result.append(item)
        elif (item.is_array() and out.item.is_array()
              and item.size == out.item.size and item.name == out.item.name):
            result.append(item)
        else:
            break
    return result


# Calls whose name says which sys.monitoring events they fire
MONITORING_HELPERS = {
    "IR_MONITOR_CALL": ("CALL",),
    "monitor_stop_iteration": ("STOP_ITERATION",),
    "monitor_reraise": ("RERAISE",),
    "_Py_call_instrumentation_line": ("LINE",),
    "_Py_call_instrumentation_instruction": ("INSTRUCTION",),
}
# An instrumented call transaction (call_transaction_monitor() and
# call_transaction_invoke() in Python/ceval.c)
TRANSACTION_EVENTS = ("CALL", "C_RETURN", "C_RAISE")
EVENT_PREFIX = "PY_MONITORING_EVENT_"


def monitoring_events(name: str, inst: Instruction) -> list[str]:
    "The sys.monitoring events that handler name fires"
    events: set[str] = set()
    for tkn in uop_token_list(inst):
        if tkn.text.startswith(EVENT_PREFIX):
            events.add(tkn.text)
        events.update(EVENT_PREFIX + e for e in MONITORING_HELPERS.get(tkn.text, ()))
    if name.startswith("INSTRUMENTED_") and uses_call_transaction(inst):
        events.update(EVENT_PREFIX + e for e in TRANSACTION_EVENTS)
    return sorted(events)


def twin_of(name: str, fmt: IRFormat, ids: dict[str, int]) -> str | None:
    """The handler that instrumentation runs on a word of handler name: the
    twin of its family; a twin is its own"""
    if name.startswith("INSTRUMENTED_"):
        return name
    twin = "INSTRUMENTED_" + fmt.group
    return twin if twin in ids else None


# Calls that construct an object: conservatively, ALLOCATES
CONSTRUCTOR = re.compile(r"New(?!Ref)|Alloc|^PyValue_From|_PyFrame_Push|_Py_Make")

# What a handler's generated code may do with its value record, value_temps
# (the classifier of the register IR v2 packet,
# pyjit/design/proposals/register-ir-v2-packet.md, A1).  A handler boxes
# unless every use is known not to: the declaration, a read of the count,
# or an argument to a function that only finishes what is there: it
# releases the boxes and adds none, so a handler that boxes nothing passes
# it NULL.  The macros that box without naming value_temps are listed;
# test_ir_contract checks the list against Python/ceval_macros.h.
NON_BOXING_CALLS = {"finish_displaced_local", "finish_unpack_to_locals", "retire_published_values",
                    "value_attribute_complete", "value_operation_finish",
                    "value_reader_complete"}


def enclosing_call_name(code: str, end: int) -> str | None:
    "The function whose argument list holds code[end], or None"
    depth = 0
    for j in range(end - 1, -1, -1):
        c = code[j]
        if c == ")":
            depth += 1
        elif c == "(":
            if depth == 0:
                m = re.search(r"(\w+)\s*$", code[:j])
                return m.group(1) if m else None
            depth -= 1
        elif c in ";{}" and depth == 0:
            return None
    return None


def boxes_into_record(code: str) -> bool:
    "Whether the generated code of a handler may box into value_temps"
    for m in re.finditer(r"\bvalue_temps\b", code):
        before, after = code[:m.start()], code[m.end():]
        if (re.match(r"\s*=\s*&value_owners\s*;", after)
                and before.rstrip().endswith("_PyValueTemps *")):
            continue    # the declaration
        if (re.match(r"->count\b(?!\s*(?:\+\+|--|[-+*/|&^]?=(?!=)))", after)
                and not re.search(r"(?:\+\+|--)\s*$", before)):
            continue    # reads the count
        if enclosing_call_name(code, m.start()) in NON_BOXING_CALLS:
            continue
        return True
    return False


def boxing_handlers(analysis: Analysis, formats: dict[str, IRFormat]) -> set[str]:
    "The handlers whose generated code may box into the value record"
    import io
    result = set()
    for name, inst in sorted(handler_instructions(analysis).items()):
        if formats[name].transparent:
            continue
        buffer = io.StringIO()
        out = CWriter(buffer, 0, False)
        emitter = IREmitter(out, analysis, formats)
        emitter.handler_format = formats[name]
        # Classify the handler as emitted with a record
        generate_handler(name, inst, formats[name], emitter, out, True)
        if boxes_into_record(buffer.getvalue()):
            result.add(name)
    return result


@dataclass
class HandlerContract:
    "The columns of the contract that are the handler's own"
    effects: list[str]
    reads: Extent
    writes: Extent
    through: Extent
    twin: str | None
    result_phase: str


RESULT_PHASE_NONE = "_PyIR_RESULT_PHASE_NONE"
RESULT_PHASE_DST_TEMP_CANONICAL_BEFORE_RETIRE = (
    "_PyIR_RESULT_PHASE_DST_TEMP_CANONICAL_BEFORE_RETIRE")


@dataclass(frozen=True)
class ResultPublicationContract:
    "A checked source declaration of the first output's publication phase"
    phase: str
    result: str
    owner: str | None
    producer: Uop
    kind: str


def _statement_text(stmt: SimpleStmt) -> list[str]:
    return [token.text for token in stmt.contents if token.kind != "COMMENT"]


def _publication_result(inst: Instruction, fmt: IRFormat,
                        result: str, producer: Uop) -> None:
    "Check a declared result against the handler's scalar first output."
    inputs, outputs = external_io(inst)
    first_output = first_new_output(outputs, len(inputs))
    producer_output = next((i for i, item in enumerate(producer.stack.outputs)
                            if item.name == result), None)
    if (not fmt.store_dst or fmt.window or first_output is None
            or fmt.dst_offset != first_output or outputs[0].external >= 0
            or fmt.field_of(ROLE_DST) is None
            or not outputs or outputs[0].item.is_array()
            or outputs[0].item.name != result
            or outputs[0].aliased or result in producer.aliased_outputs
            or producer_output != 0):
        raise analysis_error(
            f"{inst.name}: published result {result} is not the scalar "
            "ROLE_DST output", inst.where)


def _sole_top_level_assignment(uop: Uop, name: str) -> SimpleStmt | None:
    "Return a variable's one unconditional top-level assignment, if any."
    all_tokens = [token for token in uop.body.tokens()
                  if token.kind != "COMMENT"]
    assignments = [i for i, token in enumerate(all_tokens[:-1])
                   if token.text == name
                   and all_tokens[i + 1].kind == "EQUALS"]
    top_level = []
    for stmt in uop.body.body:
        if not isinstance(stmt, SimpleStmt):
            continue
        tokens = [token for token in stmt.contents if token.kind != "COMMENT"]
        direct = (len(tokens) >= 2 and tokens[0].text == name
                  and tokens[1].kind == "EQUALS")
        declared = (len(tokens) >= 3 and tokens[0].text == "PyValue"
                    and tokens[1].text == name
                    and tokens[2].kind == "EQUALS")
        if direct or declared:
            top_level.append(stmt)
    if len(assignments) != 1 or len(top_level) != 1:
        return None
    tokens = list(uop.body.tokens())
    for index, token in enumerate(tokens):
        if (token.text == name and index > 0
                and tokens[index - 1].kind == "AND"
                and (index < 2 or tokens[index - 2].kind not in OPERANDS)):
            return None
    return top_level[0]


def _owned_result_assignment(uop: Uop, name: str, source: str,
                             before: SimpleStmt) -> bool:
    "Whether name is assigned once from OWNER/TAKE in a top-level statement."
    assignment = _sole_top_level_assignment(uop, name)
    if (assignment is None or uop.body.body.index(assignment)
            >= uop.body.body.index(before)):
        return False
    tokens = [item for item in assignment.contents if item.kind != "COMMENT"]
    texts = [item.text for item in tokens]
    return (len(texts) == 8
            and texts[:5] == ["PyValue", name, "=", source, "("]
            and tokens[5].kind == "IDENTIFIER"
            and texts[6:] == [")", ";"])


def _protected_argument_consumption_tokens(tokens: list[Token],
                                           protected: set[str],
                                           immutable: set[str]) -> bool:
    consuming_callees = OWNER_READS | {"DEAD", "value_operation_close3"}
    for index, token in enumerate(tokens):
        if (token.kind == "AND" and index + 1 < len(tokens)
                and tokens[index + 1].text in protected):
            return True
        if (token.kind == "IDENTIFIER" and token.text in protected
                and ((index > 0 and tokens[index - 1].kind in {
                    "PLUSPLUS", "MINUSMINUS"})
                     or (index + 1 < len(tokens)
                         and tokens[index + 1].kind in {
                             "PLUSPLUS", "MINUSMINUS"}))):
            return True
        if (token.kind != "IDENTIFIER"
                or not (RELEASE_LIKE.search(token.text)
                        or token.text in consuming_callees)):
            continue
        if index + 1 >= len(tokens) or tokens[index + 1].kind != "LPAREN":
            continue
        depth = 0
        for argument in tokens[index + 1:]:
            if argument.kind == "LPAREN":
                depth += 1
            elif argument.kind == "RPAREN":
                depth -= 1
                if depth == 0:
                    break
            elif depth > 0 and argument.kind == "IDENTIFIER" \
                    and argument.text in protected:
                return True
    for index, token in enumerate(tokens):
        if token.kind not in ASSIGNMENTS:
            continue
        start = index - 1
        while start >= 0 and tokens[start].kind not in {
                "COMMA", "SEMI", "LBRACE", "RBRACE"}:
            start -= 1
        lhs = tokens[start + 1:index]
        targets = [item.text for item in lhs
                   if item.kind == "IDENTIFIER" and item.text in protected]
        if not targets:
            continue
        assignable = protected - immutable
        if (len(lhs) == 1 and targets[0] in assignable
                and token.kind == "EQUALS"):
            continue
        if (len(lhs) >= 2 and lhs[-1].text in assignable
                and lhs[-1].kind == "IDENTIFIER"
                and token.kind == "EQUALS" and lhs[-2].text in {
                    "PyValue", "int", "_PyStatusValue"}):
            continue
        return True
    return False


def _protected_argument_consumption(stmt: SimpleStmt,
                                    protected: set[str],
                                    immutable: set[str]) -> bool:
    """Whether a simple statement consumes, mutates, or addresses a value."""
    tokens = [token for token in stmt.contents if token.kind != "COMMENT"]
    return _protected_argument_consumption_tokens(tokens, protected, immutable)


def _is_flat_abort_cleanup(stmt: Stmt, protected: set[str],
                           immutable: set[str]) -> bool:
    """Allow only a flat cleanup branch that terminates in reader abort."""
    if (not isinstance(stmt, IfStmt) or stmt.else_body is not None
            or not isinstance(stmt.body, BlockStmt)):
        return False
    body = stmt.body.body
    if (not body or any(not isinstance(item, SimpleStmt) for item in body)
            or _protected_argument_consumption_tokens(
                stmt.condition, protected, immutable)):
        return False
    if _statement_text(body[-1]) != ["IR_ABORT_VALUE_READER", "(", ")", ";"]:
        return False
    cleanup = body[:-1]
    return bool(cleanup) and all(
        _protected_argument_consumption(item, protected, immutable)
        and any(token.kind == "IDENTIFIER"
                and (RELEASE_LIKE.search(token.text)
                     or token.text == "value_operation_close3"
                     or token.text in VIEW_RELEASES)
                for token in item.contents)
        for item in cleanup)


def _has_protected_consumption(stmt: Stmt, protected: set[str],
                               immutable: set[str]) -> bool:
    if isinstance(stmt, SimpleStmt):
        return _protected_argument_consumption(stmt, protected, immutable)
    if isinstance(stmt, IfStmt):
        return (_protected_argument_consumption_tokens(
                    stmt.condition, protected, immutable)
                or _has_protected_consumption(stmt.body, protected, immutable)
                or (stmt.else_body is not None and _has_protected_consumption(
                    stmt.else_body, protected, immutable)))
    if isinstance(stmt, BlockStmt):
        return any(_has_protected_consumption(item, protected, immutable)
                   for item in stmt.body)
    if isinstance(stmt, (ForStmt, WhileStmt)):
        return any(token.kind == "IDENTIFIER" and token.text in protected
                   for token in stmt.tokens())
    if isinstance(stmt, MacroIfStmt):
        return ((stmt.condition.kind == "IDENTIFIER"
                 and stmt.condition.text in protected)
                or any(_has_protected_consumption(item, protected, immutable)
                    for item in stmt.body)
                or (stmt.else_body is not None and any(
                    _has_protected_consumption(item, protected, immutable)
                    for item in stmt.else_body)))
    return any(token.kind == "IDENTIFIER" and token.text in protected
               for token in stmt.tokens())


def _validate_generic_reader_prefix(producer: Uop, call_stmt: SimpleStmt,
                                    call_args: list[str], token: Token) -> None:
    """Reject known input consumption except an abort-only cleanup branch."""
    protected = set(call_args[:3])
    immutable = {call_args[2]}
    receiver_init = _sole_top_level_assignment(producer, call_args[2])
    if receiver_init is None:
        raise analysis_error(
            "generic completion receiver must have one top-level TAKE()",
            token)
    call_position = producer.body.body.index(call_stmt)
    for stmt in producer.body.body[:call_position]:
        if stmt is receiver_init:
            continue
        if _is_flat_abort_cleanup(stmt, protected, immutable):
            continue
        if _has_protected_consumption(stmt, protected, immutable):
            raise analysis_error(
                "generic completion consumes, mutates, or addresses an input "
                "on a continuing path", token)


def _validate_publication_suffix(inst: Instruction, producer: Uop,
                                 outputs: list[Entry], result: str) -> None:
    "Ensure later uops preserve output zero without effects or alias escapes."
    producer_index = inst.parts.index(producer)
    suffix = [part for part in inst.parts[producer_index + 1:]
              if isinstance(part, Uop)]
    if not outputs or outputs[0].item.is_array():
        raise analysis_error(
            f"{inst.name}: publication result must be a scalar first output",
            inst.where)
    if any(item.is_array() for uop in suffix for item in uop.stack.inputs):
        raise analysis_error(
            f"{inst.name}: publication suffix has an array input effect",
            inst.where)
    if any(item.is_array() for uop in suffix[:-1] for item in uop.stack.outputs):
        raise analysis_error(
            f"{inst.name}: publication suffix follows a dynamic array output",
            inst.where)
    depth = len(outputs) - sum(len(u.stack.outputs) - len(u.stack.inputs)
                               for u in suffix)
    for uop in suffix:
        p = uop.properties
        releases = any(
            token.kind == "IDENTIFIER" and RELEASE_LIKE.search(token.text)
            for token in uop.body.tokens())
        writes_frame = any(
            token.kind == "IDENTIFIER"
            and token.text in {"IR_SLOT", "_PyFrame_Slots", "slots", "window"}
            for token in uop.body.tokens())
        if (p.escapes or p.eval_breaker or p.switches_frame or p.returns
                or releases
                or p.error_with_pop
                or p.error_without_pop or p.deopts or p.side_exit
                or p.jumps or p.always_exits or writes_frame):
            raise analysis_error(
                f"{inst.name}: publication suffix has an effect boundary",
                inst.where)
        if any(token.kind == "IDENTIFIER" and token.text == result
               for token in uop.body.tokens()):
            raise analysis_error(
                f"{inst.name}: publication suffix references result {result}",
                inst.where)
        input_count = len(uop.stack.inputs)
        output_count = len(uop.stack.outputs)
        base = depth - input_count
        if any(base + i == 0 for i in range(input_count)):
            raise analysis_error(
                f"{inst.name}: publication suffix reads the declared result "
                "position", inst.where)
        if any(base + j == 0 for j in range(output_count)):
            raise analysis_error(
                f"{inst.name}: publication suffix overwrites the declared "
                "result position", inst.where)
        depth += output_count - input_count
    if depth != len(outputs):
        raise analysis_error(
            f"{inst.name}: publication suffix changes the declared result "
            "home", inst.where)


def result_publication_contract(
    inst: Instruction, fmt: IRFormat,
) -> ResultPublicationContract | None:
    "Validate the source of the first-output publication postcondition."
    marker = "IR_RETIRE_PUBLISHED_RESULT"
    generic = "IR_COMPLETE_ATTR_READER"
    marker_occurrences = [
        (part, token) for part in inst.parts if isinstance(part, Uop)
        for token in part.body.tokens() if token.text == marker]
    generic_occurrences = [
        (part, token) for part in inst.parts if isinstance(part, Uop)
        for token in part.body.tokens() if token.text == generic]
    if len(marker_occurrences) > 1 or len(generic_occurrences) > 1:
        raise analysis_error(
            f"{inst.name}: duplicate result publication declaration",
            inst.where)
    if marker_occurrences and generic_occurrences:
        raise analysis_error(
            f"{inst.name}: conflicting result publication declarations",
            inst.where)
    if not marker_occurrences and not generic_occurrences:
        return None

    is_explicit = bool(marker_occurrences)
    producer, token = (marker_occurrences or generic_occurrences)[0]
    statements = producer.body.body
    meaningful = [stmt for stmt in statements
                  if not isinstance(stmt, SimpleStmt)
                  or _statement_text(stmt)]
    if is_explicit:
        if not meaningful or not isinstance(meaningful[-1], SimpleStmt):
            raise analysis_error(
                f"{inst.name}: result marker must be a top-level final "
                "statement", token)
        declaration = meaningful[-1]
        texts = _statement_text(declaration)
        if (texts[0:2] != [marker, "("] or len(texts) != 7
                or texts[3] != "," or texts[5:7] != [")", ";"]
                or any(not re.fullmatch(r"[A-Za-z_]\w*", name)
                       for name in (texts[2], texts[4]))):
            raise analysis_error(
                f"{inst.name}: expected {marker}(result, owner);", token)
        result, owner = texts[2], texts[4]
        if not _owned_result_assignment(producer, owner, "OWNER", declaration):
            raise analysis_error(
                f"{inst.name}: result marker owner must be an unmodified "
                "top-level OWNER() result",
                token)
        result_assignment = _sole_top_level_assignment(producer, result)
        if (result_assignment is None
                or statements.index(result_assignment)
                >= statements.index(declaration)):
            raise analysis_error(
                f"{inst.name}: result {result} must have one unconditional "
                "top-level assignment before its marker",
                token)
        if sum(item.kind == "IDENTIFIER" and item.text == result
               for item in result_assignment.contents) != 1:
            raise analysis_error(
                f"{inst.name}: result assignment must bind {result} only once",
                token)
        assignment_index = statements.index(result_assignment)
        marker_index = statements.index(declaration)
        if any(token.kind == "IDENTIFIER" and token.text == result
               for stmt in statements[assignment_index + 1:marker_index]
               for token in stmt.tokens()):
            raise analysis_error(
                f"{inst.name}: result {result} is used before receiver "
                "retirement", token)
        _publication_result(inst, fmt, result, producer)
        outputs = external_io(inst)[1]
        _validate_publication_suffix(inst, producer, outputs, result)
        return ResultPublicationContract(
            RESULT_PHASE_DST_TEMP_CANONICAL_BEFORE_RETIRE,
            result, owner, producer, "explicit")

    # The generic reader completes publication in a trusted helper, then
    # assigns its first result to the handler output.
    call_stmt = None
    call_args: list[str] = []
    for stmt in producer.body.body:
        if not isinstance(stmt, SimpleStmt):
            continue
        items = [item.text for item in stmt.contents
                 if item.kind != "COMMENT"]
        if not items or items[0] != generic:
            continue
        if (len(items) != 11 or items[1] != "(" or items[-2:] != [")", ";"]
                or items[3] != "," or items[5] != "," or items[7] != ","):
            raise analysis_error(
                f"{inst.name}: malformed {generic} arguments", token)
        call_stmt = stmt
        call_args = [items[2], items[4], items[6], items[8]]
        break
    if call_stmt is None:
        raise analysis_error(
            f"{inst.name}: {generic} must be a top-level statement", token)
    call_position = producer.body.body.index(call_stmt)
    following = [stmt for stmt in producer.body.body[call_position + 1:]
                 if not isinstance(stmt, SimpleStmt)
                 or _statement_text(stmt)]
    if not following or not isinstance(following[0], SimpleStmt):
        raise analysis_error(
            f"{inst.name}: generic completion has no output assignment", token)
    result_stmt = following[0]
    assignment = _statement_text(result_stmt)
    if (len(assignment) != 4 or assignment[1] != "="
            or assignment[2] != call_args[0] or assignment[3] != ";"):
        raise analysis_error(
            f"{inst.name}: generic completion does not assign its first result",
            token)
    result = assignment[0]
    if len(following) != 1:
        raise analysis_error(
            f"{inst.name}: generic completion must be followed directly by "
            "its output assignment", token)
    if result in call_args:
        raise analysis_error(
            f"{inst.name}: generic completion output must be distinct from "
            "its inputs", token)
    if _sole_top_level_assignment(producer, result) is not result_stmt:
        raise analysis_error(
            f"{inst.name}: generic output must have one unconditional "
            "top-level assignment", token)
    result_position = producer.body.body.index(result_stmt)
    if any(token.kind == "IDENTIFIER" and token.text == result
           for stmt in producer.body.body[:result_position]
           for token in stmt.tokens()):
        raise analysis_error(
            f"{inst.name}: generic output {result} is used before completion",
            token)
    if (len(set(call_args)) != len(call_args)
            or any(not re.fullmatch(r"[A-Za-z_]\w*", name)
                   for name in call_args)):
        raise analysis_error(
            f"{inst.name}: {generic} arguments must name distinct values",
            token)
    receiver = call_args[2]
    if not _owned_result_assignment(producer, receiver, "TAKE", call_stmt):
        raise analysis_error(
            f"{inst.name}: {generic} receiver must be an unmodified "
            "top-level TAKE() result", token)
    _validate_generic_reader_prefix(producer, call_stmt, call_args, token)
    _publication_result(inst, fmt, result, producer)
    outputs = external_io(inst)[1]
    _validate_publication_suffix(inst, producer, outputs, result)
    return ResultPublicationContract(
        RESULT_PHASE_DST_TEMP_CANONICAL_BEFORE_RETIRE,
        result, None, producer, "generic")


def handler_effects(name: str, inst: Instruction, fmt: IRFormat,
                    boxes: bool) -> list[str]:
    "The _PyIR_* effect bits of handler name, which runs inst"
    p = inst.properties
    tokens = {t.text for t in uop_token_list(inst)}
    suspends = p.switches_frame and bool(tokens & {"FRAME_SUSPENDED", "FRAME_CREATED"})
    pushes_frame = p.switches_frame and not p.returns and not suspends
    specializes = fmt.fb != LOC_NONE and any(
        isinstance(part, Uop) and "specializing" in part.annotations
        for part in inst.parts)
    allocates = (not p.infallible or p.escapes or boxes or pushes_frame
                 or any(CONSTRUCTOR.search(t) for t in tokens))
    effects = []
    for bit, present in (
            # INSTRUMENTED_JUMP() leaves for the error label when the
            # callback raises, which the analysis does not see in a macro
            ("_PyIR_RAISES", not p.infallible
                             or fmt.fused and name.startswith("INSTRUMENTED_")),
            ("_PyIR_ESCAPES", p.escapes),
            ("_PyIR_RUNS_PYTHON", runs_code(inst)),
            ("_PyIR_DEOPTS", p.deopts or p.side_exit),
            ("_PyIR_SUSPENDS", suspends),
            ("_PyIR_PUSHES_FRAME", pushes_frame),
            ("_PyIR_POLLS", p.eval_breaker or polls_at_end(name, inst)),
            ("_PyIR_ALLOCATES", allocates),
            ("_PyIR_PURE", p.pure),
            ("_PyIR_SPECIALIZES", specializes),
            ("_PyIR_BOXES", boxes)):
        if present:
            effects.append(bit)
    return effects


def handler_contracts(analysis: Analysis,
                      formats: dict[str, IRFormat]) -> dict[str, HandlerContract]:
    "The handler's own columns of the contract, by handler name"
    ids = handler_ids(analysis)
    boxing = boxing_handlers(analysis, formats)
    result = {}
    insts = {**handler_instructions(analysis),
             **{name: analysis.instructions[name] for name in STACK_ONLY
                if name in analysis.instructions}}
    for name, inst in insts.items():
        fmt = formats[name]
        inputs, outputs = external_io(inst)
        publication = result_publication_contract(inst, fmt)
        result[name] = HandlerContract(
            effects=handler_effects(name, inst, fmt, name in boxing),
            reads=extent(inputs, inst),
            writes=extent([e.item for e in outputs], inst),
            through=extent(through_items(inst, inputs, outputs), inst),
            twin=twin_of(name, fmt, ids),
            result_phase=(publication.phase if publication is not None
                          else RESULT_PHASE_NONE),
        )
    return result


# No handler takes the address of one of its own locals
# (pyjit/design/proposals/register-ir-v2-a3.md 3.5): helpers take and return
# values, work that needs memory runs out of line, and state that outlives a
# call lives in a frame slot.  The address of a handler local makes the
# handler keep a C frame and a stack canary, and GCC cannot then prove its
# musttail dispatch safe.  The exceptions: calls that always inline, and the
# box area (value_owners), which only the generator emits and which R27
# deletes.
ADDRESS_INLINE_CALLS = {
    "__builtin_add_overflow", "__builtin_sub_overflow",
    "__builtin_mul_overflow", "PyValue_FloatOperandValues",
}
# The handler's parameters and the locals the generator declares around a
# body, so that a body cannot take their address either
HANDLER_LOCALS = {
    "frame", "slots", "tstate", "next_instr", "ir_word", "this_instr",
    "window", "oparg", "fb", "opcode", "value_temps", "ir_displaced",
    "ir_rest", "ir_expanded", "value_owners", "value_boxes", "call_base",
    "call_destination", "call_state", "call_result", "call_feedback",
    "retire_needed", "retire_failed", "operation_failed",
}
HANDLER_POINTERS = {"slots", "window", "fb", "value_temps", "frame", "tstate",
                    "next_instr", "this_instr"}
DECL_KINDS = {"INT", "CHAR", "SHORT", "LONG", "SIGNED", "UNSIGNED", "DOUBLE",
              "FLOAT", "VOID", "CONST", "VOLATILE", "STATIC", "STRUCT",
              "UNION", "ENUM", "REGISTER"}


def declared_locals(tokens: list[Token]) -> dict[str, str]:
    """The local variables that a body declares, each with its form:
    "pointer" (T *x), "array" (T x[N]) or "value".  A declaration is a
    statement of type words, then declarators: `T x = ...;`, `T *x;`,
    `T a, b[2], *c;`"""
    found: dict[str, str] = {}
    n = len(tokens)
    i = 0
    while i < n:
        at_start = i == 0 or tokens[i - 1].kind in ("LBRACE", "RBRACE", "SEMI") or (
            tokens[i - 1].kind == "LPAREN" and i >= 2 and tokens[i - 2].kind == "FOR")
        if not at_start:
            i += 1
            continue
        j = i
        while j < n and (tokens[j].kind in DECL_KINDS or tokens[j].kind == "IDENTIFIER"):
            j += 1
        if j >= n:
            break
        words = j - i
        if words >= 1 and tokens[j].kind == "TIMES":
            m = j            # T *x
        elif words >= 2 and tokens[j].kind in ("EQUALS", "SEMI", "COMMA", "LBRACKET"):
            m = j - 1        # T x
        else:
            i += 1
            continue
        # The declarators, to the ';' at depth 0
        while m < n:
            pointer = False
            while m < n and tokens[m].kind in ("TIMES", "CONST"):
                pointer = pointer or tokens[m].kind == "TIMES"
                m += 1
            if m >= n or tokens[m].kind != "IDENTIFIER":
                break
            name = tokens[m].text
            m += 1
            form = "pointer" if pointer else "value"
            if m < n and tokens[m].kind == "LBRACKET":
                form = "array"
            found.setdefault(name, form)
            depth = 0
            while m < n:
                kind = tokens[m].kind
                if kind in ("LPAREN", "LBRACKET", "LBRACE"):
                    depth += 1
                elif kind in ("RPAREN", "RBRACKET", "RBRACE"):
                    depth -= 1
                elif depth == 0 and kind in ("COMMA", "SEMI"):
                    break
                m += 1
            if m >= n or tokens[m].kind == "SEMI":
                break
            m += 1           # the comma
        i = m + 1
    return found


def address_callee(tokens: list[Token], index: int) -> str | None:
    "The function whose argument list holds tokens[index], or None"
    depth = 0
    for j in range(index - 1, 0, -1):
        kind = tokens[j].kind
        if kind == "RPAREN":
            depth += 1
        elif kind == "LPAREN":
            if depth == 0:
                if tokens[j - 1].kind == "IDENTIFIER":
                    return tokens[j - 1].text
                depth = 0
                continue
            depth -= 1
    return None


# The tokens that end an operand: an `&` after one is a bitwise and.  After
# any other token (an operator, `?`, `!`, `==`, `+=`, `return`, a cast's `)`)
# it takes an address.
OPERAND_END = {"IDENTIFIER", "NUMBER", "STRING", "CHARACTER", "RBRACKET",
               "PLUSPLUS", "MINUSMINUS"}
CAST_WORDS = DECL_KINDS | {"IDENTIFIER", "TIMES"}


def closes_cast(tokens: list[Token], index: int) -> bool:
    "Whether the `)` at tokens[index] closes a cast, `(` type words `*`s `)`"
    j = index - 1
    while j >= 0 and tokens[j].kind in CAST_WORDS:
        j -= 1
    if j < 0 or j == index - 1 or tokens[j].kind != "LPAREN":
        return False
    before = tokens[j - 1].kind if j > 0 else ""
    # f(x) and sizeof(T) are operands, if (x) is a condition
    return before not in OPERAND_END | {"RPAREN", "SIZEOF", "IF", "WHILE",
                                        "SWITCH"}


def is_unary_and(tokens: list[Token], index: int) -> bool:
    if index == 0:
        return True
    before = tokens[index - 1]
    if before.kind in OPERAND_END:
        return False
    if before.kind == "RPAREN":
        return closes_cast(tokens, index - 1)
    return True


def addressed_locals(name: str, tokens: list[Token],
                     stack_items: dict[str, bool]) -> list[tuple[str, Token]]:
    """The sites in a uop or label body that take the address of one of the
    handler's locals: a local the body declares, a stack item that is not an
    array, or a local that the generator declares"""
    declared = declared_locals(tokens)
    errors = []
    # A local array is addressable by definition: it decays to a pointer
    for tkn, after in zip(tokens, tokens[1:]):
        if (after.kind == "LBRACKET" and tkn.kind == "IDENTIFIER"
                and declared.get(tkn.text) == "array"):
            errors.append((f"{name}: line {tkn.line}: {tkn.text}[] "
                           "(a local array)", tkn))
            declared[tkn.text] = "reported"
    n = len(tokens)
    for i, tkn in enumerate(tokens[:-1]):
        if tkn.kind != "AND" or not is_unary_and(tokens, i):
            continue
        j = i + 1
        while j < n and tokens[j].kind == "LPAREN":
            j += 1     # &(x)
        if j >= n or tokens[j].kind != "IDENTIFIER":
            continue
        var = tokens[j]
        after = tokens[j + 1].kind if j + 1 < n else ""
        if var.text in declared:
            form = declared[var.text]
        elif var.text in stack_items:
            form = "pointer" if stack_items[var.text] else "value"
        elif var.text in HANDLER_LOCALS or var.text.startswith("ir_own_"):
            form = "pointer" if var.text in HANDLER_POINTERS else "value"
        else:
            continue   # a global or a static
        if after == "ARROW" or (after == "LBRACKET" and form == "pointer"):
            continue   # an address reached through a pointer
        if address_callee(tokens, i) in ADDRESS_INLINE_CALLS:
            continue
        errors.append((f"{name}: line {tkn.line}: &{var.text}", tkn))
    return errors


def check_no_addressable_locals(analysis: Analysis) -> None:
    "Refuse a uop or label that takes the address of a handler local"
    errors = []
    for uop in analysis.uops.values():
        items = {item.name: item.is_array()
                 for item in uop.stack.inputs + uop.stack.outputs}
        tokens = [t for t in uop.body.tokens() if t.kind != "COMMENT"]
        errors += addressed_locals(uop.name, tokens, items)
    for label in analysis.labels.values():
        tokens = [t for t in label.body.tokens() if t.kind != "COMMENT"]
        errors += addressed_locals(label.name, tokens, {})
    if errors:
        raise analysis_error(
            "the address of a handler local (register-ir-v2-a3.md 3.5: pass "
            "values, move memory work out of line, keep state in slots):\n  "
            + "\n  ".join(sorted({text for text, _ in errors})), errors[0][1])


def generate_ir(filenames: list[str], analysis: Analysis, outfile: TextIO,
                lines: bool) -> None:
    formats = compute_formats(analysis)
    check_no_addressable_locals(analysis)
    boxing = boxing_handlers(analysis, formats)
    write_header(__file__, filenames, outfile)
    outfile.write(f"""
            {INSTRUCTION_START_MARKER}
""")
    generate_ir_cases(analysis, formats, boxing, outfile, lines)
    outfile.write(f"""
            {INSTRUCTION_END_MARKER}
        {LABEL_START_MARKER}
""")
    out = CWriter(outfile, 2, lines)
    emitter = IREmitter(out, analysis, formats)
    generate_ir_labels(analysis, emitter)
    outfile.write(f"{LABEL_END_MARKER}\n")


def function_proto(name: str, prefix: str = "_TAIL_CALL_") -> str:
    return f"static PyValue Py_PRESERVE_NONE_CC {prefix}{name}(TAIL_CALL_PARAMS)"


def num_handlers(analysis: Analysis) -> int:
    "One past the highest pinned number, with the reserved ranges"
    return analysis.numbers.num_handlers


def generate_ir_targets(filenames: list[str], analysis: Analysis,
                        outfile: TextIO) -> None:
    "The prototypes of the handlers and labels, and the dispatch table"
    ids = handler_ids(analysis)
    n = num_handlers(analysis)
    write_header(__file__, filenames, outfile)
    out = CWriter(outfile, 0, False)
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
    out.emit("_PyValueTemps *const value_temps = NULL;\n")
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
