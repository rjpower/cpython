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
    analysis_error,
    analyze_files,
)
from parser import SimpleStmt
from generators_common import (
    DEFAULT_INPUT,
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
    return 1 + max((p.opcode for p in analysis.pseudos.values()), default=255)

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
    # What place_fields() placed, from which the NOFB form follows
    slot_roles: tuple[str, ...] = ()
    imm_kind: str | None = None
    store_dst: bool = False         # dst may name a local, see
                                    # stores_to_dst(); the handlers release
                                    # the local's old value
    runs_code: bool = False         # a member may run Python code, see
                                    # runs_code()
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
    "IR_RETIRE_PUBLISHED", "IR_RETIRE_PUBLISHED_PAIR",
    "IR_FINISH_VALUE_OPERATION", "IR_ABORT_VALUE_READER",
    "IR_COMPLETE_VALUE_READER", "IR_COMPLETE_ATTR_READER",
    "_PyFrame_ClearTemporaries", "_PyEval_FrameClearAndPop", "_PyErr_Clear",
    "JIT_RETIRE_PENDING",
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


# A value read through a field that names a local holds the slot's bits:
# it borrows the local in place, owns no count, and nothing in the value
# says so (pyjit/design/pyvalue.md D4; register-ir.md IR-D3).  So the
# generator knows each such value (Local.ir_inplace) and what a handler does
# with it.  It may:
# - release it: the release happens only if the field names a temporary,
#   whose value the handler took (IREmitter.inplace_release());
# - take a count of it: PyValue_DUP, PyValue_NewRef, or PyValue_MakeHeapSafe
#   and PyValue_AsKnownObjectSteal, which take one for a local
#   (IREmitter.inplace_owner());
# - read it, as an argument of a function that only reads its value
#   arguments (INPLACE_READERS, the PyValue_Is* tests), through `.bits`,
#   or in `(void)name`;
# - copy it to a stack input or output of the uop, which then borrows the
#   same local (IREmitter.note_pointer_assignment() follows only those);
# - read its object with PyValue_AsKnownObjectBorrow(), which has no count
#   either: that object, or a variable that it is assigned to, must not be
#   an argument that a function takes over or releases (consumed_argument()).
# Any other use, a store or a function that may keep it, keeps the input:
# the format does not let a local reach its field (compute_format(),
# kept_inputs()), and the generator refuses such a use of a value that
# still may borrow a local (IREmitter.check_inplace_input()).  So a handler
# cannot keep or release a local's reference without a count, through the
# value or through those copies.  An object pointer that reaches another
# variable through an expression (a field, an array, a conditional) is not
# followed: A5b's typed borrow covers that.
INPLACE_RELEASES = {"PyValue_CLOSE", "PyValue_XCLOSE",
                    "PyValue_CLOSE_SPECIALIZED"}
INPLACE_OWNERS = {"PyValue_MakeHeapSafe", "PyValue_AsKnownObjectSteal"}
INPLACE_READERS = {
    "PyValue_DUP", "PyValue_NewRef", "PyValue_AsObject",
    "PyValue_AsKnownObjectBorrow", "PyValue_AsPyObjectNew", "PyValue_TYPE",
    "PyValue_BoolCheck", "PyValue_CompactIntValue", "PyValue_FloatCheckExact",
    "PyValue_FloatOperandValues", "PyValue_FloatValue", "PyValue_InlineFloat",
    "PyValue_InlineInt", "PyValue_LongCheckExact", "_PyValue_Truth",
    # Python/ceval.c: they read their value arguments, and the handler
    # releases them after
    "_PyEval_BinarySlice", "_PyEval_StoreName", "_PyEval_StoreSlice",
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


def assigned_name(tokens: list[Token], start: int, end: int) -> str | None:
    """The variable that `name = (cast) tokens[start..end];` assigns, as a
    statement or a declaration (the cast is optional), or None"""
    if end + 1 >= len(tokens) or tokens[end + 1].text != ";":
        return None
    j = start - 1
    if j >= 0 and tokens[j].text == ")":
        while j >= 0 and tokens[j].text != "(":
            j -= 1
        j -= 1
    if j < 1 or tokens[j].text != "=" or tokens[j - 1].kind != "IDENTIFIER":
        return None
    return tokens[j - 1].text


def inplace_input_use(uop: CodeSection, name: str) -> Token | None:
    """The first use of name, which may borrow a local in place, in the
    body of uop that is not one of those above, or None.  Uses inside
    assert() vanish from release builds and are not checked."""
    tokens = list(uop.body.tokens())
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
            copy = assigned_name(tokens, i, i)
            if copy is not None:
                objects.add(copy)
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
            names.add(tokens[i - 2].text)  # A copy, which borrows too
        elif prev == ")" and nxt == ";" and i >= 3 \
                and tokens[i - 2].text == "void" and tokens[i - 3].text == "(":
            pass
        elif prev in ("(", ",") and nxt in (")", ","):
            callee = enclosing_callee(tokens, i) or ""
            whole = prev == "(" and nxt == ")"
            first = prev == "(" and nxt == ","
            if not ((callee in INPLACE_RELEASES and (whole or first))
                    or (callee == "DEAD" and whole)
                    or (callee in INPLACE_OWNERS and whole)
                    or callee in INPLACE_READERS
                    or callee.startswith("PyValue_Is")):
                return tkn
            if callee in INPLACE_OBJECT_READERS and whole:
                # Its object, which has no count either
                if consumed_argument(tokens, i - 2):
                    return tkn
                holder = assigned_name(tokens, i - 2, i + 1)
                if holder is not None:
                    objects.add(holder)
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
            if i is not None and takes_count(part, item.name):
                i = None  # A new value, which owns its count
            stack.append(popped[i] if i is not None else -1)
    return kept


def takes_count(uop: Uop, name: str) -> bool:
    """Whether the body of uop replaces name by the value with a count that
    an owner call gives (`name = PyValue_MakeHeapSafe(name);`) on every
    path: as a statement of the body itself, not of a branch."""
    tokens = list(uop.body.tokens())
    depth = 0
    for i, tkn in enumerate(tokens):
        if tkn.text == "{":
            depth += 1
        elif tkn.text == "}":
            depth -= 1
        elif (depth == 1 and tkn.text == name
              and i > 0 and tokens[i - 1].text in (";", "{", "}")
              and [t.text for t in tokens[i + 1:i + 6]]
                  in ([ "=", owner, "(", name, ")"] for owner in INPLACE_OWNERS)
              and i + 6 < len(tokens) and tokens[i + 6].text == ";"):
            return True
    return False


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
        if base != inst.name and base in analysis.instructions:
            return group_name(analysis.instructions[base], analysis)
    return inst.name


def all_instructions(analysis: Analysis) -> dict[str, Instruction]:
    return {**analysis.instructions, **analysis.ir_only}


EXEC_ONLY = {"CALL_JIT_WARM": "CALL", "JUMP_BACKWARD_JIT_WARM": "JUMP_BACKWARD", "RESUME_JIT": "RESUME"}

def exec_only(analysis: Analysis) -> dict[str, str]:
    # Small generator fixtures need not define the production entry families.
    return EXEC_ONLY if all(base in analysis.instructions for base in EXEC_ONLY.values()) else {}

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


def handler_ids(analysis: Analysis) -> dict[str, int]:
    """Handler index of each handler, by name.  BINARY_OP itself has none:
    its operations do."""
    ids = {name: inst.opcode for name, inst in analysis.instructions.items()
           if name != BINARY_OP_FAMILY}
    names = (sorted(analysis.ir_only) + binary_op_handlers(analysis)
             + [name + NOFB_SUFFIX for name in nofb_bases(analysis)])
    for i, name in enumerate(names):
        ids[name] = first_ir_only_handler(analysis) + i
    for name in exec_only(analysis):
        ids[name] = max(ids.values()) + 1
    return ids


def handler_instructions(analysis: Analysis) -> dict[str, Instruction]:
    "The instruction that each handler runs, by handler name"
    insts = all_instructions(analysis)
    result = {name: insts[name] for name in handler_ids(analysis)
              if name in insts}
    for name in binary_op_handlers(analysis):
        result[name] = insts[BINARY_OP_FAMILY]
    for name in nofb_bases(analysis):
        result[name + NOFB_SUFFIX] = insts[name]
    for name, base in exec_only(analysis).items():
        result[name] = insts[base]
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
    slot_roles = tuple(slots)
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
        no_fold=no_fold, slot_roles=slot_roles, imm_kind=imm_kind,
        store_dst=store_dst, runs_code=any(runs_code(m) for m in members),
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
    "PyValue_AsKnownObjectBorrow",
}
SLOT_ADDRESS_READS = {      # f(&GETLOCAL(oparg)) reads the slot in place
    "PyValue_BorrowAt",
}
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


def nofb_format(fmt: IRFormat) -> IRFormat:
    "The format of the NOFB form of a handler of format fmt"
    placed = place_fields(list(fmt.slot_roles), False, fmt.imm_kind, fmt.window)
    assert placed is not None, fmt.group
    roles, imm, fb = placed
    # The form runs on entry 0, which is no entry of its own
    return replace(fmt, roles=roles, imm=imm, fb=fb, fb_layout=None,
                   fb_kind=None)


def compute_formats(analysis: Analysis) -> dict[str, IRFormat]:
    "The format of every handler and instruction, by name"
    formats = base_formats(analysis)
    for name in nofb_bases(analysis):
        formats[name + NOFB_SUFFIX] = nofb_format(formats[name])
    for name, base in exec_only(analysis).items():
        formats[name] = replace(formats[base])
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

    def __init__(self, layout: SlotLayout | None = None,
                 raw_inputs: set[int] | None = None) -> None:
        super().__init__()
        self.layout = layout if layout is not None else SlotLayout.empty()
        # The positions of the borrowable inputs that the handler consumes
        # as they are: those read through a local come borrowed, as
        # LOAD_FAST_BORROW would have pushed them.
        self.raw_inputs = set(raw_inputs or ())
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
        # Set by IR_WINDOW_TAKEN(): the handler disposed of its window, so
        # that the slots of the dead inputs stay as it left them.
        self.window_taken = False
        # The selected generic attribute helper published temporary outputs
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
        other.raw_inputs = set(self.raw_inputs)
        other.window_taken = self.window_taken
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

    def load(self, var: StackItem, memory_offset: PointerOffset) -> str:
        lay = self.layout
        if not lay.window and not var.is_array():
            p = memory_offset.as_int()
            if (p is not None and self.owner.get(p) == "in"
                    and p in lay.borrowable and p in self.raw_inputs):
                return f"IR_LOAD_BORROWED({self.input_field(p)})"
        return super().load(var, memory_offset)

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
                    local.ir_borrowed = p in self.raw_inputs
                    local.ir_inplace = lay.in_fields[p]
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
            # The input itself, back in its slot: a box, or a NULL before
            # the handler closes it
            f = self.input_field(p)
            if p in lay.borrowable:
                out.emit(f"IR_STORE_INPUT({f}, {var.name});\n")
            else:
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
        """Before var goes to a slot other than its input's: if it borrows a
        local in place, it takes a count, as LOAD_FAST_BORROW does (no frame
        slot holds a borrow)"""
        if var.ir_inplace is not None:
            out.emit(f"IR_INPLACE_OWN({var.name}, {FIELD_VARS[var.ir_inplace]});\n")
            out.start_line()
            var.ir_inplace = None

    def refuse_inplace(self, var: Local, memory_offset: PointerOffset) -> None:
        "A slot must not get a local's reference without a count"
        if var.ir_inplace is not None:
            raise StackError(f"'{var.name}', which may borrow a local in "
                             f"place, would be stored at position "
                             f"{memory_offset}")

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
    helper = False

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
        self._replacers["IR_RETIRE_PUBLISHED_PAIR"] = self.retire_published
        self._replacers["IR_FINISH_VALUE_OPERATION"] = self.finish_value_operation
        self._replacers["IR_CAPTURE_VALUE_INPUTS"] = self.capture_value_inputs
        self._replacers["IR_RESTORE_VALUE_INPUTS"] = self.restore_value_inputs
        self._replacers["IR_CAPTURE_VALUE_RESULT"] = self.capture_value_result
        self._replacers["IR_COMPLETE_VALUE_READER"] = self.complete_value_reader
        self._replacers["IR_ABORT_VALUE_READER"] = self.abort_value_reader
        self._replacers["IR_RESTORE_ATTR_INPUT"] = self.restore_attr_input
        self._replacers["IR_COMPLETE_ATTR_READER"] = self.complete_attr_reader
        self._replacers["IR_PREPARE_CALL_METHOD"] = self.prepare_call
        self._replacers["IR_PREPARE_CALL_TUPLE"] = self.prepare_call
        self._replacers["IR_MONITOR_CALL"] = self.prepare_call
        self._replacers["IR_INVOKE_CALL"] = self.invoke_call
        self._replacers["IR_BEGIN_CALL_EFFECTS"] = self.call_effects
        self._replacers["IR_REFRESH_CALL"] = self.refresh_call
        self._replacers["PyValue_MakeHeapSafe"] = self.inplace_owner
        # Set while a statement is emitted, for inplace_release()
        self.statement: SimpleStmt | None = None
        # The input, and its field, that the statement's escaping release
        # killed, if it may borrow a local in place
        self.killed_inplace: tuple[str, str] | None = None
        self.stores_outputs = False  # the statement's uop is the last
        self.call_finished = False
        self.call_fallback_used = False
        self.call_fallback = ""
        self.call_has_feedback = False
        # The format of the handler being emitted, which for a NOFB form
        # is not its instruction's
        self.handler_format: IRFormat | None = None
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

    def sync_sp(self, tkn, tkn_iter, uop, storage, inst):
        if inst is None or inst.name not in (
                "LOAD_ATTR_SLOT", "LOAD_ATTR_INSTANCE_VALUE", "LOAD_ATTR_SHAPE",
                "LOAD_ATTR_MODULE", "LOAD_ATTR_WITH_HINT"):
            return super().sync_sp(tkn, tkn_iter, uop, storage, inst)
        next(tkn_iter)
        next(tkn_iter)
        next(tkn_iter)
        storage.clear_inputs("before published receiver retirement")
        storage.flush(self.out)
        # A folded local must keep its computed owning result in C until
        # receiver retirement. Its old destination is still observable.
        return True

    def retire_published(self, tkn, tkn_iter, uop, storage, inst):
        assert inst is not None
        assert next(tkn_iter).kind == "LPAREN"
        first = next(tkn_iter)
        assert first.kind == "IDENTIFIER"
        second = None
        end = next(tkn_iter)
        if end.kind == "COMMA":
            second = next(tkn_iter)
            assert second.kind == "IDENTIFIER"
            end = next(tkn_iter)
        assert end.kind == "RPAREN"
        assert next(tkn_iter).kind == "SEMI"
        storage.flush(self.out)
        for retired in (first, second):
            field = self.inplace_field(retired, storage)
            if field is not None:
                # A local's value is not the handler's to retire
                self.emit(f"if (IR_IS_LOCAL({field})) {{ "
                          f"{retired.text} = PyValue_NULL; }}\n")
        failure_result = "NULL"
        if inst.name in ("LOAD_ATTR_SLOT", "LOAD_ATTR_INSTANCE_VALUE", "LOAD_ATTR_SHAPE",
                         "LOAD_ATTR_MODULE", "LOAD_ATTR_WITH_HINT"):
            fmt = self.format_of(inst)
            dst = FIELD_VARS[fmt.field_of(ROLE_DST)]
            failure_result = f"(IR_IS_LOCAL({dst}) ? &attr : NULL)"
        self.emit("{\n")
        boxes = " || value_temps->count != 0" if self.has_record else ""
        self.emit(f"int retire_needed = _PyValue_MayDealloc({first.text}){boxes};\n")
        self.emit("if (!retire_needed) {\n")
        self.emit(f"PyValue_XCLOSE({first.text}); {first.text} = PyValue_NULL;\n")
        if second is not None:
            self.emit(f"retire_needed = _PyValue_MayDealloc({second.text});\n")
        self.emit("}\n")
        self.emit("if (retire_needed) {\n")
        second_ptr = "NULL" if second is None else "&" + second.text
        self.emit(f"int retire_failed = retire_published_values(tstate, frame, next_instr, {self.temps}, &{first.text}, {second_ptr}, {failure_result});\n")
        self.emit("if (retire_failed < 0) { JUMP_TO_LABEL(error); }\n")
        self.emit("this_instr = frame->instr_ptr;\n")
        if not self.helper:
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
        return True

    def capture_value_inputs(self, tkn, tkn_iter, uop, storage, inst):
        assert inst is not None
        assert next(tkn_iter).kind == "LPAREN"
        state = next(tkn_iter)
        assert state.kind == "IDENTIFIER"
        assert next(tkn_iter).kind == "RPAREN"
        assert next(tkn_iter).kind == "SEMI"
        fmt = self.format_of(inst)
        assert fmt.n_inputs in (1, 2)
        for index in range(fmt.n_inputs):
            field = FIELD_VARS[fmt.input_field(index)]
            self.emit(f"{state.text}.input_slots[{index}] = {field};\n")
        return True

    def restore_attr_input(self, tkn, tkn_iter, uop, storage, inst):
        assert next(tkn_iter).kind == "LPAREN"
        state = next(tkn_iter)
        assert next(tkn_iter).kind == "COMMA"
        owner = next(tkn_iter)
        assert next(tkn_iter).kind == "RPAREN"
        assert next(tkn_iter).kind == "SEMI"
        self.out.start_line()
        self.emit(f"(void)value_operation_restore1(&{state.text}, &{owner.text});\n")
        return True

    def complete_attr_reader(self, tkn, tkn_iter, uop, storage, inst):
        assert next(tkn_iter).kind == "LPAREN"
        names = []
        for index in range(5):
            name = next(tkn_iter)
            assert name.kind == "IDENTIFIER"
            names.append(name.text)
            if index < 4:
                assert next(tkn_iter).kind == "COMMA"
        assert next(tkn_iter).kind == "RPAREN"
        assert next(tkn_iter).kind == "SEMI"
        state, result, self_value, receiver, method = names
        self.out.start_line()
        self.emit(f"if (value_attribute_complete(&{state}, next_instr, {self.temps}, &{result}, &{self_value}, &{receiver}, {method}) < 0) {{\n")
        self.reader_error()
        self.emit("}\n")
        self.reader_reload()
        assert isinstance(storage.stack, IRStack)
        storage.stack.published_temporary_result = True
        return True

    def restore_value_inputs(self, tkn, tkn_iter, uop, storage, inst):
        assert inst is not None
        assert next(tkn_iter).kind == "LPAREN"
        names = []
        for index in range(3):
            name = next(tkn_iter)
            assert name.kind == "IDENTIFIER"
            names.append(name.text)
            if index < 2:
                assert next(tkn_iter).kind == "COMMA"
        assert next(tkn_iter).kind == "RPAREN"
        assert next(tkn_iter).kind == "SEMI"
        self.out.start_line()
        self.emit(f"(void)value_operation_restore2(&{names[0]}, &{names[1]}, &{names[2]});\n")
        # Cleanup and forced finish follow even if restoration rejects the
        # frame; it must not strand any of the still-owning C variables.
        return True

    def capture_value_result(self, tkn, tkn_iter, uop, storage, inst):
        assert inst is not None
        assert next(tkn_iter).kind == "LPAREN"
        state = next(tkn_iter)
        assert state.kind == "IDENTIFIER"
        assert next(tkn_iter).kind == "RPAREN"
        assert next(tkn_iter).kind == "SEMI"
        field = FIELD_VARS[self.format_of(inst).field_of(ROLE_DST)]
        self.emit(f"{state.text}.result_slot = {field};\n")
        return True

    def reader_error(self):
        if self.helper:
            self.emit("ctx->frame = frame; ctx->next_instr = next_instr;\n")
            self.emit("ctx->raising_instr = frame->instr_ptr;\n")
            self.emit("ctx->phase = _PyJIT_ERROR_LOG_PENDING;\n")
            self.emit("return _PyJIT_RAISE;\n")
        else:
            self.emit("JUMP_TO_LABEL(error);\n")

    def emit_store_input(self, var: Local, storage: Storage) -> None:
        """An input that emit_object_inputs boxed, which the slot must hold
        for a later reader.  value_temps owns the box and var borrows it, so
        the C code can still read var as an object.  The slot keeps the
        inline value, the canonical form of the box, so nothing goes back."""
        self.out.start_line()
        self.emit(f"IR_KEEP_BOX({var.name});\n")

    def release_displaced(self):
        """Release the local value that IR_STORE_DST or IR_MOVE_TO_DST
        displaced, with the input boxes, then revalidate: the release can
        run __del__ or a weakref callback, which can run any Python."""
        self.emit("if (!PyValue_IsNull(ir_displaced) &&\n")
        self.emit("    finish_displaced_local(tstate, frame, next_instr, "
                  f"{self.temps}, ir_displaced) < 0) {{\n")
        if not self.helper:
            self.emit("next_instr = frame->instr_ptr + 1;\n")
        self.reader_error()
        self.emit("}\n")

    def reader_reload(self):
        self.emit("this_instr = frame->instr_ptr;\n")
        if not self.helper:
            self.emit("ir_word = FT_ATOMIC_LOAD_UINT64_RELAXED(*this_instr);\n")
        self.emit("slots = _PyFrame_Slots(frame);\n")

    def call_reload(self, inst):
        self.reader_reload()
        self.emit("window = slots + call.base;\n")
        if self.call_has_feedback:
            self.emit(f"fb = ({feedback_type(self.format_of(inst))} *)"
                      "(frame->ir_feedback + call_feedback * FB_STRIDE);\n")

    def prepare_call(self, tkn, tkn_iter, uop, storage, inst):
        assert inst is not None
        assert next(tkn_iter).kind == "LPAREN"
        assert next(tkn_iter).kind == "RPAREN"
        assert next(tkn_iter).kind == "SEMI"
        helper = {
            "IR_PREPARE_CALL_METHOD": "call_transaction_expand_method",
            "IR_PREPARE_CALL_TUPLE": "call_transaction_prepare_tuple",
            "IR_MONITOR_CALL": "call_transaction_monitor",
        }[tkn.text]
        storage.flush(self.out)
        self.out.start_line()
        self.emit(f"if ({helper}(&call, next_instr) < 0) {{\n")
        self.reader_error()
        self.emit("}\n")
        self.call_reload(inst)
        return True

    def call_effects(self, tkn, tkn_iter, uop, storage, inst):
        assert next(tkn_iter).kind == "LPAREN"
        assert next(tkn_iter).kind == "RPAREN"
        assert next(tkn_iter).kind == "SEMI"
        self.out.start_line()
        self.emit("call.effects = 1;\n")
        return True

    def refresh_call(self, tkn, tkn_iter, uop, storage, inst):
        assert next(tkn_iter).kind == "LPAREN"
        assert next(tkn_iter).kind == "RPAREN"
        assert next(tkn_iter).kind == "SEMI"
        self.out.start_line()
        self.emit("if (call_transaction_refresh(&call, next_instr) < 0) {\n")
        self.reader_error()
        self.emit("}\n")
        self.call_reload(inst)
        return True

    def call_completion(self):
        if self.has_record:
            self.emit("assert(value_temps->count == 0);\n")
        self.emit("_PyInterpreterFrame *call_child;\n")
        self.emit("if (call_transaction_invoke(&call, next_instr, &call_child) < 0) {\n")
        self.reader_error()
        self.emit("}\n")
        self.emit("if (call_child != NULL) {\n")
        if self.helper:
            self.emit("ctx->frame = call_child; ctx->next_instr = call_child->instr_ptr;\n")
            self.emit("ctx->phase = _PyJIT_ENTER_PREPARED;\n")
            self.emit("return _PyJIT_ENTER_FRAME;\n")
        else:
            self.emit("DISPATCH_INLINED(call_child);\n")
        self.emit("}\n")
        # Both result and callback-current destination are already published.
        # No stack flush, delayed move from window[0], or helper JIT_FINISH
        # may consume stale C inputs after this point.
        if self.helper:
            self.emit("ctx->frame = frame; ctx->next_instr = next_instr;\n")
            self.emit("ctx->phase = jit_phase; return jit_action;\n")
        else:
            self.emit("slots = _PyFrame_Slots(frame);\n")
            self.emit("DISPATCH();\n")

    def invoke_call(self, tkn, tkn_iter, uop, storage, inst):
        assert next(tkn_iter).kind == "LPAREN"
        assert next(tkn_iter).kind == "RPAREN"
        assert next(tkn_iter).kind == "SEMI"
        self.call_completion()
        self.call_finished = True
        return False

    def complete_value_reader(self, tkn, tkn_iter, uop, storage, inst):
        assert next(tkn_iter).kind == "LPAREN"
        names = []
        for index in range(4):
            name = next(tkn_iter)
            assert name.kind == "IDENTIFIER"
            names.append(name.text)
            if index < 3:
                assert next(tkn_iter).kind == "COMMA"
        assert next(tkn_iter).kind == "RPAREN"
        assert next(tkn_iter).kind == "SEMI"
        state, result, first, second = names
        # The DSL output is deliberately undefined here. Failed providers
        # restored their inputs; no NULL result may overwrite those homes.
        self.out.start_line()
        self.emit(f"if (value_reader_complete(&{state}, next_instr, {self.temps}, &{result}, &{first}, &{second}) < 0) {{\n")
        self.reader_error()
        self.emit("}\n")
        self.reader_reload()
        return True

    def abort_value_reader(self, tkn, tkn_iter, uop, storage, inst):
        assert next(tkn_iter).kind == "LPAREN"
        state = next(tkn_iter)
        assert state.kind == "IDENTIFIER"
        assert next(tkn_iter).kind == "RPAREN"
        assert next(tkn_iter).kind == "SEMI"
        # Never flush the undefined result on this edge: its destination
        # may be one of the just-restored input temporary homes.
        self.emit(f"(void)value_operation_finish(&{state.text}, next_instr, {self.temps});\n")
        self.reader_error()
        return False

    def finish_value_operation(self, tkn, tkn_iter, uop, storage, inst):
        assert inst is not None
        assert next(tkn_iter).kind == "LPAREN"
        state = next(tkn_iter)
        assert state.kind == "IDENTIFIER"
        assert next(tkn_iter).kind == "RPAREN"
        assert next(tkn_iter).kind == "SEMI"
        storage.flush(self.out)
        self.emit("{\n")
        self.emit(f"int operation_failed = value_operation_finish(&{state.text}, next_instr, {self.temps});\n")
        self.emit("if (operation_failed < 0) {\n")
        if self.helper:
            # The current word was refreshed without decoding an old pointer.
            # Avoid JIT_FINISH's subtraction from its pre-operation this_instr.
            self.emit("ctx->frame = frame; ctx->next_instr = next_instr;\n")
            self.emit("ctx->raising_instr = frame->instr_ptr;\n")
            self.emit("ctx->phase = _PyJIT_ERROR_LOG_PENDING;\n")
            self.emit("return _PyJIT_RAISE;\n")
        else:
            self.emit("JUMP_TO_LABEL(error);\n")
        self.emit("}\n")
        self.emit("this_instr = frame->instr_ptr;\n")
        if not self.helper:
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
                f"{where}: {uop.name} uses '{name}', which may borrow a "
                f"local in place, as more than a value it reads, releases "
                f"or takes a count of (inplace_input_use())", use)

    def inplace_field(self, arg: Token | None, storage: Storage) -> str | None:
        "The field whose local arg may borrow in place, if arg is a variable"
        if arg is None or arg.kind != "IDENTIFIER":
            return None
        if self.killed_inplace is not None and self.killed_inplace[0] == arg.text:
            return FIELD_VARS[self.killed_inplace[1]]
        var = find_local(arg.text, storage)
        if var is None or var.ir_inplace is None:
            return None
        return FIELD_VARS[var.ir_inplace]

    def inplace_release(self, tkn: Token, storage: Storage,
                        arg: Token | None) -> None:
        """A release of arg, which may borrow a local in place, is a
        statement of its own: it releases arg only if the field names a
        temporary, whose value the handler took."""
        field = self.inplace_field(arg, storage)
        if field is None:
            return
        stmt = self.statement
        if (stmt is None or stmt.contents[0] is not tkn
                or stmt.contents[-1].kind != "SEMI"):
            raise analysis_error(
                f"{tkn.text}({arg.text}) of a value that may borrow a local "
                "in place must be a statement of its own", tkn)
        self.out.start_line()
        self.emit(f"if (!IR_IS_LOCAL({field})) ")

    def value_pointer_variant(self, tkn, tkn_iter, uop, storage, inst):
        lparen = next(tkn_iter)
        assert lparen.kind == "LPAREN"
        arg = tkn_iter.peek()
        assert arg is not None
        if tkn.text != "PyValue_DUP":
            self.inplace_release(tkn, storage, arg)
        self.emit_function_name(tkn, arg, storage)
        self.out.emit(lparen)
        return True

    def value_close_specialized(self, tkn, tkn_iter, uop, storage, inst):
        lparen = next(tkn_iter)
        assert lparen.kind == "LPAREN"
        self.inplace_release(tkn, storage, tkn_iter.peek())
        return super().value_close_specialized(
            tkn, iter_with_first(lparen, tkn_iter), uop, storage, inst)

    def value_steal(self, tkn, tkn_iter, uop, storage, inst):
        lparen = next(tkn_iter)
        assert lparen.kind == "LPAREN"
        field = self.inplace_field(tkn_iter.peek(), storage)
        if field is None:
            return super().value_steal(
                tkn, iter_with_first(lparen, tkn_iter), uop, storage, inst)
        # The object, with a count of its own for a local
        name = next(tkn_iter)
        self.out.emit_at("IR_INPLACE_OBJECT_STEAL", tkn)
        self.out.emit(lparen)
        self.out.emit(name)
        self.out.emit(f", {field}")
        return self.value_kill(name, storage, False)

    def inplace_owner(self, tkn, tkn_iter, uop, storage, inst):
        "PyValue_MakeHeapSafe(), which takes a count for a local"
        lparen = next(tkn_iter)
        field = self.inplace_field(tkn_iter.peek(), storage)
        if field is None:
            self.out.emit(tkn)
            self.out.emit(lparen)
            return True
        name = next(tkn_iter)
        self.out.emit_at("IR_INPLACE_HEAP_SAFE", tkn)
        self.out.emit(lparen)
        self.out.emit(name)
        self.out.emit(f", {field}")
        return True

    def note_pointer_assignment(self, stmt: SimpleStmt, storage: Storage) -> None:
        """After `a = b;` with b borrowing a local in place, a borrows it
        too."""
        super().note_pointer_assignment(stmt, storage)
        tkns = stmt.contents
        if len(tkns) != 4 or tkns[1].kind != "EQUALS" or tkns[-1].kind != "SEMI":
            return
        source = find_local(tkns[2].text, storage)
        if source is None or source.ir_inplace is None:
            return
        if self.stores_outputs and any(
                var.name == tkns[0].text for var in storage.outputs) and not any(
                var.name == tkns[0].text for var in storage.inputs):
            # An output that the handler stores owns its value: take the
            # count here, on this path, rather than at the store, after
            # paths join.  (An earlier uop's output flows to the next uop,
            # which may release it only if it names a temporary.)
            self.out.start_line()
            self.out.emit(f"IR_INPLACE_OWN({tkns[0].text}, "
                          f"{FIELD_VARS[source.ir_inplace]});\n")
            return
        for var in storage.inputs + storage.outputs:
            if var.name == tkns[0].text:
                var.ir_inplace = source.ir_inplace
                return
        # inplace_input_use() keeps such an input out of borrow_mask
        raise analysis_error(
            f"'{tkns[0].text}', not a stack variable, would borrow a local "
            f"in place through '{tkns[2].text}'", tkns[0])

    def consume_killed(self, storage: Storage) -> None:
        """Clear the slots of the inputs that the handler has taken: a dead
        input's slot holds NULL, on every path"""
        stack = storage.stack
        assert isinstance(stack, IRStack)
        if stack.layout.window:
            return
        for var in storage.inputs:
            p = var.ir_input
            if var.is_dead() and p is not None and stack.owner.get(p) == "in":
                self.out.start_line()
                stack.consume(self.out, p)
                del stack.owner[p]

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
        if transaction_marker:
            # Their handlers explicitly publish/refresh captured homes and
            # terminal state. Automatic post-call spill/reload would instead
            # republish stale C inputs after actual destination publication.
            self.cannot_escape = True
        # An escaping release kills its input before the statement is
        # emitted (Emitter.emit_SimpleStmt); remember whether it borrows
        self.killed_inplace = None
        escape = uop.properties.escaping_calls.get(stmt)
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
        its slot; an output defined before stays in its variable."""
        stack = storage.stack
        assert isinstance(stack, IRStack)
        if stack.layout.window or self.cannot_escape:
            if any(var.ir_inplace is not None for var in storage.inputs):
                raise analysis_error(
                    "DECREF_INPUTS here would release a local's reference", tkn)
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
            if var.known_object:
                close = f"_{close}_POINTER"
            self.out.start_line()
            p = var.ir_input
            if p is not None and stack.owner.get(p) == "in":
                stack.consume(self.out, p)
                del stack.owner[p]
            if var.ir_inplace is not None:
                # Only a temporary's value is the handler's to release
                self.emit(f"if (!IR_IS_LOCAL({FIELD_VARS[var.ir_inplace]})) ")
            self.emit(f"{close}({var.name});\n")
            var.kill()
        storage.clear_inputs("after DECREF_INPUTS")
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

    def error_no_pop(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        "The inputs stay in their slots"
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
        "The handler runs again on the same word, and reads its inputs again"
        assert inst is not None
        if uses_call_transaction(inst):
            self.emit(f"if (call.effects) {{ goto {self.call_fallback}; }}\n")
            self.call_fallback_used = True
        self.emit(tkn)
        return True

    def emit_jump_to_predicted(self, inst: Instruction | None) -> None:
        assert inst is not None
        if uses_call_transaction(inst):
            self.emit(f"if (call.effects) {{ goto {self.call_fallback}; }}\n")
            self.call_fallback_used = True
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
        self, uop: Uop, storage: Storage, inst: Instruction | None, stack_read_later: bool
    ) -> None:
        """Inputs read through a field: check, and box an inline value.  A
        box belongs to the handler, as the box of the temporary that
        LOAD_FAST_BORROW pushed would: a temporary's goes back to its slot,
        and a local keeps its inline value.  An object read through a field
        that names a local is the slot's, borrowed in place (see
        inplace_input_use())."""
        for var in storage.inputs:
            if var.ir_inplace is not None:
                self.check_inplace_input(uop, var.name, inst)
        kept = {item.name for item in uop.stack.outputs}
        for var in storage.inputs:
            if var.ir_field is None or var.known_object or var.ir_borrowed:
                continue
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
                    self.emit(f"if (Py_UNLIKELY(!PyValue_IsPointerSpace({var.name}))) {{\n")
                    self.emit_jump_to_predicted(inst)
                else:
                    self.check_boxed_input(uop, var.name, inst)
                    self.emit(f"if (Py_UNLIKELY(PyValue_IsInline({var.name}))) {{\n")
                    self.emit(f"{var.name} = PyValue_Box({var.name});\n")
                    self.emit(f"if (PyValue_IsInline({var.name})) {{\n")
                    storage.stack.copy().flush(self.out)
                    self.emit(f"{self.jump_prefix}JUMP_TO_LABEL(error);\n")
                    self.emit("}\n")
                    if inst is not None:
                        self.emit(f"BOX_STAT_INC({inst.name});\n")
                    self.emit(f"IR_KEEP_BOX({var.name});\n")
                self.emit("}\n")
            var.known_object = True
            var.ir_borrowed = True
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
    if (emitter.helper and uop.name.startswith("_SPECIALIZE_") and
            any(t.text == "DISPATCH_SAME_OPARG" for t in uop.body.tokens())):
        emitter.out.start_line()
        emitter.emit("#if ENABLE_SPECIALIZATION\n")
        emitter.emit(f"jit_after{uop.name}:;\n")
        emitter.emit("#endif\n")
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


def skipped_part(name: str, part: Part) -> bool:
    """Whether the handler of name leaves part of its instruction out: the
    warm-up form of CALL counts calls for the JIT and does not specialise"""
    return (name == "CALL_JIT_WARM" and isinstance(part, Uop)
            and part.name == "_SPECIALIZE_CALL")


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
    (boxing_handlers, the contract's BOXES) needs one."""
    operation = None
    emitter.has_record = record
    nofb = name == inst.name + NOFB_SUFFIX
    if name != inst.name and not nofb and name not in EXEC_ONLY:
        operation = "NB_" + name.removeprefix(BINARY_OP_FAMILY + "_")
    helper = emitter.helper
    transaction = uses_call_transaction(inst)
    emitter.call_finished = False
    emitter.call_fallback_used = False
    emitter.call_fallback = f"call_transaction_fallback_{name}"
    is_target = inst.is_target and name == inst.name and not helper
    out.emit("\n")
    if helper:
        out.emit(f"static _PyJitAction jit_helper_{name}(_PyJitContext *ctx, const _PyJitDecodedOp *op) {{\n")
        out.emit("PyThreadState *tstate = ctx->tstate;\n")
        out.emit("_PyInterpreterFrame *frame = ctx->frame;\n")
        out.emit("_PyIRWord *next_instr = ctx->next_instr;\n")
        out.emit("PyValue *slots = _PyFrame_Slots(frame);\n")
        out.emit("(void)tstate; (void)slots;\n")
        out.emit(f"int opcode = {name}; (void)opcode;\n")
        out.emit("assert(next_instr == op->instruction);\n")
        action = ("_PyJIT_RETURN" if name == "RETURN_VALUE" else
                  "_PyJIT_BRANCH" if fmt.jumps or fmt.forward else "_PyJIT_CONTINUE")
        phase = "_PyJIT_RETURN_COMPLETED" if name == "RETURN_VALUE" else "_PyJIT_AFTER"
        out.emit(f"const _PyJitAction jit_action = {action};\n")
        out.emit(f"const _PyJitPhase jit_phase = {phase};\n")
        if name == "RAISE_VARARGS":
            out.emit("(void)jit_action; (void)jit_phase;\n")
    else:
        out.emit(f"TARGET({name}) {{\n")
    bridge_bound = value_bridge_bound(name, inst)
    out.emit(f"#if _Py_TAIL_CALL_INTERP\n")
    out.emit(f"int opcode = {name};\n")
    out.emit(f"(void)(opcode);\n")
    out.emit("_PyInterpreterFrame *frame = tstate->current_frame;\n")
    if not record and not helper:
        # A tail-called handler is a function of its own
        out.emit("_PyValueTemps *const value_temps = NULL;\n")
        out.emit("(void)value_temps;\n")
    out.emit(f"#endif\n")
    if inst.properties.needs_prev:
        out.emit(f"_PyIRWord* const prev_instr = frame->instr_ptr;\n")
    if not is_target:
        qualifier = "" if transaction or inst.name == "BINARY_OP" or name in ("CALL", "CALL_JIT_WARM", "CALL_KW", "INSTRUMENTED_CALL", "INSTRUMENTED_CALL_KW", "CALL_NON_PY_GENERAL", "CALL_KW_NON_PY", "CALL_BUILTIN_O", "LOAD_ATTR_SLOT", "STORE_ATTR_SLOT",
                                   "BUILD_MAP", "STORE_SUBSCR", "STORE_SUBSCR_DICT",
                                   "STORE_ATTR_WITH_HINT", "BINARY_OP_SUBSCR_DICT", "LOAD_GLOBAL", "LOAD_GLOBAL_NOFB",
                                   "LOAD_ATTR", "LOAD_ATTR_NOFB", "STORE_ATTR", "STORE_ATTR_NOFB",
                                   "STORE_ATTR_INSTANCE_VALUE", "STORE_ATTR_SHAPE", "STORE_ATTR_SHAPE_ADD",
                                   "LOAD_ATTR_INSTANCE_VALUE", "LOAD_ATTR_SHAPE",
                                   "LOAD_ATTR_MODULE", "LOAD_ATTR_WITH_HINT") else "const "
        out.emit(f"_PyIRWord* {qualifier}this_instr = next_instr;\n")
        out.emit("(void)this_instr;\n")
    if not inst.properties.no_save_ip:
        out.emit(f"frame->instr_ptr = next_instr;\n")
    out.emit(f"next_instr += 1;\n")
    out.emit(f"INSTRUCTION_STATS({inst.name});\n")
    if is_target:
        out.emit(f"IR_PREDICTED({name});\n")
        qualifier = "" if transaction or inst.name == "BINARY_OP" or name in ("CALL", "CALL_JIT_WARM", "CALL_KW", "STORE_SUBSCR", "LOAD_GLOBAL", "LOAD_GLOBAL_NOFB", "LOAD_ATTR", "LOAD_ATTR_NOFB", "STORE_ATTR", "STORE_ATTR_NOFB") else "const "
        out.emit(f"_PyIRWord* {qualifier}this_instr = next_instr - 1;\n")
        out.emit("(void)this_instr;\n")
    if not record:
        # No record: the eval function's value_temps is NULL, and a helper,
        # a function of its own, declares its own
        if helper:
            out.emit("_PyValueTemps *const value_temps = NULL;\n")
            out.emit("(void)value_temps;\n")
    elif helper:
        out.emit(f"PyObject *value_boxes[{bridge_bound}];\n")
        out.emit("_PyValueTemps value_owners = {value_boxes, 0, Py_ARRAY_LENGTH(value_boxes)};\n")
    else:
        # The eval function declares one array for all handlers. A tail-called
        # handler is its own function, so it keeps its own.
        out.emit("#if _Py_TAIL_CALL_INTERP\n")
        out.emit(f"PyObject *value_boxes[{bridge_bound}];\n")
        out.emit("_PyValueTemps value_owners;\n")
        out.emit("#endif\n")
        out.emit(f"value_owners = (_PyValueTemps){{value_boxes, 0, {bridge_bound}}};\n")
    if record:
        out.emit("_PyValueTemps *value_temps = &value_owners;\n")
    if fmt.store_dst:
        # The old value of a local that dst names, released after the
        # result is stored (release_displaced)
        out.emit("PyValue ir_displaced = PyValue_NULL;\n")
        out.emit("(void)ir_displaced;\n")
    use_fb = fmt.fb != LOC_NONE and (
        uses_feedback(inst) or reads_fb_arg(inst, fmt, operation))
    emitter.call_has_feedback = (use_fb and name != "CALL_JIT_WARM"
                                 or nofb and uses_feedback(inst))
    if use_fb and name != "CALL_JIT_WARM":
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
        base = FIELD_VARS[fmt.field_of(ROLE_BASE)]
        dst = FIELD_VARS[fmt.field_of(ROLE_DST)]
        argc = "oparg" if inst.properties.oparg else "0"
        periodic = int(polls_at_end(name, inst))
        out.emit(f"_PyCallTransaction call = call_transaction_begin(tstate, frame, next_instr, "
                 f"{base}, {count}, {dst}, {argc}, {kind}, "
                 f"{int(name.startswith('INSTRUMENTED_'))}, {periodic});\n")
        if emitter.call_has_feedback:
            out.emit(f"uint32_t call_feedback = {field_expr(fmt.fb)};\n")
    elif fmt.window and call_family in ("CALL", "CALL_KW"):
        inputs, _ = external_io(inst)
        count = " + ".join(f"({item.size or '1'})" for item in inputs)
        out.emit(f"_PyValue_RecheckWindow(window, {count});\n")
    if name in ("CALL", "CALL_JIT_WARM", "CALL_KW"):
        out.emit("if (JIT_CALL_NEEDS_PREPARE(this_instr)) {\n")
        out.emit("call.effects = 1;\n")
        out.emit("int jit_prepare = JIT_PREPARE_CALL(this_instr, window[0]);\n")
        out.emit("if (call_transaction_refresh(&call, next_instr) < 0 || jit_prepare < 0) {\n")
        emitter.reader_error()
        out.emit("}\n")
        emitter.call_reload(inst)
        # JIT preparation may allocate/observe. A decline continues this
        # same logical call; retrying the handler would replay those effects.
        out.emit("}\n")
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
    raw_inputs: set[int] = set()
    if fmt.borrow_mask:
        uses = classify_inputs(inst, fmt.n_inputs)
        raw_inputs = {k - fmt.n_inputs for k in range(fmt.n_inputs)
                      if fmt.borrow_mask & (1 << k) and uses[k] == "raw"}
    stack: Stack = IRStack(slot_layout(inst, fmt, mode), raw_inputs)
    insert_braces = len([p for p in inst.parts if isinstance(p, Uop)]) > 1
    reachable = True
    for part in inst.parts:
        if skipped_part(name, part):
            continue
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
            stack.dst_stores.append(f"IR_MOVE_TO_DST({dst}, window[{fmt.dst_offset}])")
            out.emit(f"{stack.dst_stores[-1]};\n")
        if fmt.store_dst:
            emitter.release_displaced()
        if name in ("RETURN_VALUE", "RETURN_GENERATOR") and not helper:
            out.emit("JIT_MAYBE_RESUME(1);\n")
        if name in ("RESUME_JIT", "JUMP_BACKWARD_JIT_WARM") and not helper:
            out.emit("JIT_SITE_ENTRY(this_instr);\n")
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
        emitter.emit("TAIL_CALL_LOAD_FRAME();\n")
        storage = Storage(IRStack(), [], [], 0, False)
        if label.spilled:
            storage.spilled = 1
            storage.stack.physical_sp = None
        emitter.emit_tokens(label, storage, None)
        emitter.emit("}\n\n")


def max_bridge_bound(analysis: Analysis, boxing: set[str]) -> int:
    "The capacity of the eval function's box array, which the handlers that box share"
    # An input in which no handler boxes, such as labels alone, still
    # declares a nonempty array.
    return max((value_bridge_bound(name, inst)
                for name, inst in handler_instructions(analysis).items()
                if name in boxing),
               default=1)


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
    twin of its family, of the NOFB form for a NOFB form; a twin is its own"""
    if name.startswith("INSTRUMENTED_"):
        return name
    suffix = NOFB_SUFFIX if name.endswith(NOFB_SUFFIX) else ""
    twin = "INSTRUMENTED_" + fmt.group + suffix
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
NON_BOXING_CALLS = {"finish_displaced_local", "retire_published_values",
                    "value_attribute_complete", "value_operation_finish",
                    "value_reader_complete"}
BOXING_MACROS = {"IR_KEEP_BOX"}


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
    if re.search(r"\b(?:%s)\b" % "|".join(sorted(BOXING_MACROS)), code):
        return True
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


def handler_effects(name: str, inst: Instruction, fmt: IRFormat,
                    boxes: bool) -> list[str]:
    "The _PyIR_* effect bits of handler name, which runs inst"
    p = inst.properties
    tokens = {t.text for t in uop_token_list(inst)}
    suspends = p.switches_frame and bool(tokens & {"FRAME_SUSPENDED", "FRAME_CREATED"})
    pushes_frame = p.switches_frame and not p.returns and not suspends
    specializes = fmt.fb != LOC_NONE and any(
        isinstance(part, Uop) and "specializing" in part.annotations
        and not skipped_part(name, part) for part in inst.parts)
    allocates = (not p.infallible or p.escapes or boxes or pushes_frame
                 or any(CONSTRUCTOR.search(t) for t in tokens))
    effects = []
    for bit, present in (
            ("_PyIR_RAISES", not p.infallible),
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
    for name, inst in handler_instructions(analysis).items():
        fmt = formats[name]
        inputs, outputs = external_io(inst)
        result[name] = HandlerContract(
            effects=handler_effects(name, inst, fmt, name in boxing),
            reads=extent(inputs, inst),
            writes=extent([e.item for e in outputs], inst),
            through=extent(through_items(inst, inputs, outputs), inst),
            twin=twin_of(name, fmt, ids),
        )
    return result


def generate_ir(filenames: list[str], analysis: Analysis, outfile: TextIO,
                lines: bool) -> None:
    formats = compute_formats(analysis)
    boxing = boxing_handlers(analysis, formats)
    write_header(__file__, filenames, outfile)
    outfile.write(f"""
#if !_Py_TAIL_CALL_INTERP
#if !USE_COMPUTED_GOTOS
    dispatch_opcode:
        switch (opcode)
#endif
        {{
            /* One scratch area serves every handler that boxes; each resets
               value_owners. */
            PyObject *value_boxes[{max_bridge_bound(analysis, boxing)}];
            _PyValueTemps value_owners;
#endif /* _Py_TAIL_CALL_INTERP */
            {INSTRUCTION_START_MARKER}
""")
    generate_ir_cases(analysis, formats, boxing, outfile, lines)
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
    return (first_ir_only_handler(analysis) + len(analysis.ir_only)
            + len(binary_op_handlers(analysis)) + len(nofb_bases(analysis)) + len(exec_only(analysis)))


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
