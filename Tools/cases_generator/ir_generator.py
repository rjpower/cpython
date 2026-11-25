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
Each handler and label is a label in one C function, which dispatches to the
next by a computed goto (Python/ceval_macros.h).  Handlers read the fields they
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
- A specialisation deoptimises by a jump to its family head, which
  runs the word again (JUMP_TO_PREDICTED, Python/ceval_macros.h).
"""

import argparse
import io
from collections.abc import Sequence
import ir_contract
import ir_leaf
import ir_arms
from ir_records import (
    ASSIGNMENTS, CONST, DEOPT, ENTER_FRAME, EXIT_GUARD, FEEDBACK, GUARD_TOKENS, NEXT,
    IMMEDIATE, LOCAL, MAY_RAISE, OPERANDS, POLY_TABLE_READERS, RELEASE_LIKE,
    RUNS_PYTHON, SITE_TOKENS, SLOT, TARGET, TEMP, WINDOW, YIELD, CallShape,
    FeedbackOperand, FeedbackSchema, Guard, Operand, OperationRecord,
    Transfer, UopRecord, assigns_to, changes, check_record, encoding, feedback_schema,
    uop_record_effects, passed_through, record_through,
    site_tokens, uop_transfers, walk_values,
)
# The exits of the generator-boundary variants (boundary_departure())
from ir_records import BRANCH, RAISE, RETURN
# The jumps that a stub binds (check_stub_arm())
from ir_records import JUMP, TRANSFER_TOKENS
from ir_read_effects import (
    error_nonnull_value, null_value_condition, value_assignment, value_declaration,
    value_nonnull_changes, value_transport_expression,
)
from ir_error_effects import (
    HELPERS, HomeEffect, analyze_error_effects, checked_native_step_effects,
)
import re
from dataclasses import dataclass, fields, replace

from analyzer import (
    Analysis,
    IR_HELPER_CALLEES,
    IR_HELPER_EFFECTS,
    IR_MARKER_EFFECTS,
    CALL_INVOKE_MARKERS,
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
from parser import (BlockStmt, ForStmt, IfStmt, MacroIfStmt, Parser, SimpleStmt, Stmt,
                    WhileStmt)
from generators_common import (
    DEFAULT_INPUT,
    NON_ESCAPING_DEALLOCS,
    POINTER_VARIANTS,
    ROOT,
    Emitter,
    TokenIterator,
    emit_to,
    enclosing_callee,
    find_local,
    instruction_macro_args,
    tokens_text,
    type_and_null,
    write_header,
)
from cwriter import CWriter
from lexer import Token, tokenize
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
DEFAULT_STUBS_OUTPUT = ROOT / "Python/generated_ir_stubs.c.h"

ERROR_INGRESSES = {
    "error": "RAISE",
    "exception_unwind": "UNWIND",
    "exit_unwind": "EXIT",
    "exit_unwind_notrace": "EXIT_NOTRACE",
    "throw_error": "THROW",
}

UNKNOWN_OPCODE_HANDLER = """\
_PyErr_Format(tstate, PyExc_SystemError,
              "%U:%d: unknown opcode %d",
              _PyFrame_GetCode(frame)->co_filename,
              PyUnstable_InterpreterFrame_GetLine(frame),
              opcode);
PROCESS_ERROR_AT(RAISE, 0, next_instr);
"""

INSTRUCTION_START_MARKER = "/* BEGIN INSTRUCTIONS */"
INSTRUCTION_END_MARKER = "/* END INSTRUCTIONS */"
LABEL_START_MARKER = "/* BEGIN LABELS */"
LABEL_END_MARKER = "/* END LABELS */"

# The fields of the word, and the C expressions that read them in a handler
# (Include/internal/pycore_ir.h), from ir_word, which every handler receives.
FIELDS = ("A", "B", "C", "D")
FIELD_VARS = {f: f"IR_{f}(ir_word)" for f in FIELDS}
# The arguments that name a field to the result and input macros, which test
# its slot-class flag in the word (Python/ceval_macros.h)
FIELD_ARGS = {f: f"ir_word, {f}" for f in FIELDS}


def is_local(field: str) -> str:
    "The C test that field of ir_word names a local"
    return f"IR_IS_LOCAL_FIELD(ir_word, {field})"


@dataclass(frozen=True)
class ExecutionVariant:
    base: str
    local_fields: tuple[str, ...] = ()
    temp_fields: tuple[str, ...] = ()
    shape_cases: bool = False


# Execution-only refinements: canonical words keep their ordinary handlers.
EXECUTION_VARIANTS: dict[str, ExecutionVariant] = {}


@dataclass(frozen=True)
class GeneratorBoundary:
    base: str
    # Where the variant departs (analyzer.Boundary.at), as its clause states
    at: str
    # The completion its consumer completes (analyzer.Boundary.completion)
    completion: str
    yielded: str = "NONE"
    completed: str = "NONE"
    resume: str = "NONE"
    # The exits that the boundary(...) clause declares for the completion
    # of the variant (completed is not NONE)
    exits: tuple[str, ...] = ()


# The arms of the generator completion routine (gen_complete,
# Python/ceval_routines.h), by the consumer that a completing variant
# fuses, with the exits each arm takes: RETURN returns from the frame in
# turn, the others continue at the source's exit (its branch target).
# Each may raise.  A variant's boundary(...) clause declares these exits;
# the generator checks the clause against the arm (check_boundary_expansion()).
GENERATOR_COMPLETIONS = {
    "STORE_LOCAL": ("GEN_COMPLETE_STORE", ("branch", "raise")),
    "RETURN": ("GEN_COMPLETE_RETURN", ("return", "raise")),
    "DISCARD": ("GEN_COMPLETE_DISCARD", ("branch", "raise")),
    "HOME": ("GEN_COMPLETE_HOME", ("branch", "raise")),
    "STORE_FAST": ("GEN_COMPLETE_STORE_FAST", ("branch", "raise")),
    "POP_TOP": ("GEN_COMPLETE_POP_TOP", ("branch", "raise")),
}

# The completion step that a variant's completion(...) names: the kind of
# its departing uop's IR_COMPLETE_STEP.  A throw's completion continues at
# the word's exit (GEN_THROWING in the completed table).
GENERATOR_COMPLETION_KINDS = {"send": "_PY_IR_SEND", "throw": "_PY_IR_SEND_THROW"}

# The resume routine's arms, by the consumer that a yield's variant fuses
GENERATOR_RESUMES = {"SEND": "GEN_RESUME_SEND", "STORE_LOCAL": "GEN_RESUME_STORE"}


def generator_boundaries(analysis: Analysis) -> dict[str, GeneratorBoundary]:
    """The generator-boundary variants SOURCE_XFER_CONSUMER, one per consumer
    that the boundary(...) clause of the source, or of its family's head,
    declares (Python/bytecodes.c).  A member's own clause replaces its
    family's and may differ from it only in at(...)."""
    result = {}
    for name, inst in all_instructions(analysis).items():
        clause = analysis.boundaries.get(name)
        family = (analysis.boundaries.get(inst.family.name)
                  if inst.family is not None and inst.family.name != name else None)
        if clause is None:
            clause = family
        elif family is not None and (
                (clause.yields, clause.completion, clause.completes, clause.resumes) !=
                (family.yields, family.completion, family.completes, family.resumes)):
            raise analysis_error(f"boundary({name}) may differ from its family's "
                                 f"boundary({family.source}) only in at(...)", clause.where)
        if clause is None:
            continue
        if bool(clause.completes) == bool(clause.resumes):
            raise analysis_error(f"boundary({clause.source}) declares either "
                                 "completes(...) or resumes(...)", clause.where)
        if clause.yields != "NONE" and not clause.completes:
            raise analysis_error(f"boundary({clause.source}): a delegated yield "
                                 "needs completes(...)", clause.where)
        for consumer, exits in clause.completes.items():
            if consumer not in GENERATOR_COMPLETIONS:
                raise analysis_error(f"boundary({clause.source}): {consumer} has no "
                                     "arm in the completion routine", clause.where)
            result[f"{name}_XFER_{consumer}"] = GeneratorBoundary(
                name, clause.at, clause.completion, clause.yields, consumer, exits=exits)
        for consumer in clause.resumes:
            if consumer not in GENERATOR_RESUMES:
                raise analysis_error(f"boundary({clause.source}): {consumer} has no "
                                     "arm in the resume routine", clause.where)
            result[f"{name}_XFER_{consumer}"] = GeneratorBoundary(
                name, clause.at, clause.completion, resume=consumer)
    return result


def boundary_departure(uop: UopRecord, boundary: GeneratorBoundary,
                       where: Token) -> UopRecord:
    """The departing (last) uop of a generator-boundary variant, with the
    exits its boundary(...) clause declares: a delegated yield takes the
    place of its fall-through, and a completion that returns takes the
    place of its branches, which run the consumer.  That every branch is a
    completion is checked against the lowered completions, not assumed
    (check_boundary_expansion()).  A variant that departs at(enter) yields
    and completes in the frame that it enters, and its uop keeps its
    exits."""
    transfers = uop.transfers
    if boundary.at == "enter":
        if not any(t.kind == ENTER_FRAME for t in transfers):
            raise analysis_error(f"{uop.name}: at(enter) needs the departing "
                                 "uop's enter", where)
        return uop
    if boundary.yielded == "DELEGATED":
        if not transfers or transfers[-1] != Transfer(NEXT):
            raise analysis_error(f"{uop.name}: a delegated yield needs the "
                                 "departing uop's fall-through", where)
        transfers = transfers[:-1] + (Transfer(YIELD, None, "delegated"),)
    if boundary.completed != "NONE":
        if not any(t.kind == BRANCH for t in transfers):
            raise analysis_error(f"{uop.name}: a completion needs the departing "
                                 "uop's branch", where)
        if "return" in boundary.exits and "branch" not in boundary.exits:
            transfers = tuple(replace(t, kind=RETURN) if t.kind == BRANCH else t
                              for t in transfers)
    return replace(uop, transfers=transfers)


# The exit of each transfer kind that a variant's expansion may reach
BOUNDARY_EXIT_KINDS = {"yield": YIELD, "return": RETURN, "branch": BRANCH,
                       "raise": RAISE, "enter": ENTER_FRAME}


def check_boundary_expansion(name: str, boundary: GeneratorBoundary,
                             record: OperationRecord, reached: set[str],
                             completions: list[tuple[Token, str]],
                             where: Token) -> None:
    """VT1 and VT3 over the expansion of a generator-boundary variant: the
    exits that its emitted body reaches (reached: IR_GEN_YIELD, the yield
    effect, IR_GEN_RESUME, and IR_GEN_COMPLETE as "complete"), with the
    completion arm of its consumer for "complete", are each an exit of
    its record, and each exit that its boundary(...) clause adds is
    reached.  Lowering grants nothing.  A variant that departs at(enter)
    reaches its enter and nothing that the clause adds.

    completions are the generator completions that the body lowers, each
    IR_COMPLETE_STEP's token and kind.  Each is of the kind that the
    clause's completion(...) names, and the departing uop's completion
    transfers (boundary_departure()) are exactly at these sites: a branch
    of any other kind is refused, not taken for a completion."""
    if boundary.at == "enter":
        if reached != {"enter"}:
            raise analysis_error(f"{name}: departs at(enter), and its expansion "
                                 f"reaches {sorted(reached)}", where)
    exits = set(reached - {"complete"})
    if "complete" in reached:
        if boundary.completed == "NONE":
            raise analysis_error(f"{name}: its expansion completes, and its "
                                 "boundary(...) clause declares no completes(...)", where)
        exits.update(GENERATOR_COMPLETIONS[boundary.completed][1])
        omitted = set(GENERATOR_COMPLETIONS[boundary.completed][1]) - set(boundary.exits)
        if omitted:
            raise analysis_error(f"{name}: its expansion reaches {sorted(omitted)}, "
                                 "which its boundary(...) clause omits", where)
    for tkn, kind in completions:
        if kind != GENERATOR_COMPLETION_KINDS.get(boundary.completion):
            raise analysis_error(f"{name}: completes with {kind}, and its boundary(...) "
                                 f"clause declares completion({boundary.completion})", tkn)
    if boundary.at == "last" and boundary.completed != "NONE":
        sites = {id(tkn) for tkn, _ in completions}
        for t in record.uops[-1].transfers:
            if t.kind in (BRANCH, RETURN) and (t.site is None or id(t.site) not in sites):
                line = t.site.line if t.site is not None else "?"
                raise analysis_error(f"{name}: {record.uops[-1].name}'s {t.kind.lower()} "
                                     f"at line {line} is no generator completion", where)
    kinds = {t.kind for uop in record.uops for t in uop.transfers}
    for exit in sorted(exits):
        if BOUNDARY_EXIT_KINDS[exit] not in kinds:
            raise analysis_error(f"{name}: its expansion reaches `{exit}`, which "
                                 "its boundary(...) clause omits", where)
    declared = set(boundary.exits) if boundary.at == "last" else set()
    if boundary.yielded != "NONE" and boundary.at == "last":
        declared.add("yield")
    unreached = declared - exits
    if unreached:
        raise analysis_error(f"{name}: its boundary(...) clause declares "
                             f"{sorted(unreached)}, which its expansion never reaches", where)


def generator_word_entry(name: str, analysis: Analysis) -> str:
    """Shape identities share a WORD body without an adapter tail hop."""
    boundaries = generator_boundaries(analysis)
    boundary = boundaries.get(name)
    if boundary is None:
        return name
    if boundary.resume != "NONE":
        return boundary.base
    return min(key for key, row in boundaries.items() if row.base == boundary.base)


def generator_boundary_source_closed(boundary: GeneratorBoundary,
                                     analysis: Analysis,
                                     formats: dict[str, "IRFormat"]) -> bool:
    """Only complete audited sources may publish an executable shape."""
    insts = all_instructions(analysis)
    resume = insts.get("RESUME")
    # The yield-from loop's jump is a Jump word (Jump.Direct), whose source
    # the family's rows hold; no handler of the stack op remains to audit.
    if resume is None or normal_transfer_kind(
            resume, formats["RESUME"]) != "_PyIR_TRANSFER_PRESERVE":
        return False
    # A new family member without the same source contract closes no shape;
    # eligibility is never recovered by declining its specialization later.
    send_members = [inst for name, inst in insts.items()
                    if name == "SEND" or inst.family is not None and
                    inst.family.name == "SEND"]
    if not send_members or any(send_source_contract(inst, formats[inst.name]) is None
                               for inst in send_members):
        return False
    inst = insts[boundary.base]
    fmt = formats[boundary.base]
    if boundary.resume != "NONE":
        return (canonical_frame_transfer_input(inst) or abrupt_suspend_contract(inst, fmt)
                or metadata_only_passthroughs(inst) == frozenset({1}))
    # A native send's source, or a completion's: the structure of the
    # source, not its clause, decides (check_boundary_expansion() then
    # holds the clause to what the source reaches)
    if send_source_contract(inst, fmt) is not None:
        return True
    # The existing typed completion marker owns its canonical argument and
    # checks the complete four-input retirement window.
    return (fmt.window and fmt.field_of(ROLE_DST) == "A" and
            fmt.field_of(ROLE_BASE) == "B" and len(external_io(inst)[0]) == 4 and
            sum(token.text == "IR_COMPLETE_STEP" for token in uop_token_list(inst)) == 1)


def redispatch_cohorts(analysis: Analysis, formats: dict[str, "IRFormat"]):
    """Intersect only bodies selectable with the same physical entry."""
    boundaries = generator_boundaries(analysis)
    # A family's baseline(...) arm is compiled code's, never a word's
    # handler: no word redispatches to it
    baselines = {family.baseline for family in analysis.families.values()}
    groups = {}
    for name in handler_instructions(analysis):
        if name in baselines:
            continue
        boundary = boundaries.get(name)
        shape = None if boundary is None else (
            boundary.yielded, boundary.completed, boundary.resume)
        groups.setdefault((formats[name].group, shape), []).append(name)
    return groups.values()


# The call families, whose words are a head followed by operand words
# (Include/internal/pycore_ir.h), by the _PyIRFormat.operands of their
# format.  The handlers are generated as for any family.  Their stack model is
# a window, so that the DSL of bytecodes.c keeps its positions (callable, self
# or NULL, arguments, ...), but position p lives in the home that the operand
# word of p names (IREmitter.window_slot), and the metadata sees the word's
# real shape (operand_metadata()).
OPERAND_CALLS = {
    "CALL": "_PyIR_OPERANDS_CALL",
    "CALL_KW": "_PyIR_OPERANDS_KW",
    "CALL_FUNCTION_EX": "_PyIR_OPERANDS_EX",
}

# The role flags that a word of a call family may carry: IR_F_NO_SELF where
# CALL and CALL_KW have no self operand (the CallRoles pass proves it,
# Python/regcfg_passes.c), and IR_F_NO_KW where CALL_FUNCTION_EX has no kwargs
# operand.  A handler of the family reads them from its word, so
# specialisation and instrumentation keep them by keeping the flags.
CALL_ROLE_FLAGS = {
    "CALL": ("IR_F_NO_SELF",),
    "CALL_KW": ("IR_F_NO_SELF",),
    "CALL_FUNCTION_EX": ("IR_F_NO_KW",),
}


def call_role_flags(name: str, inst: Instruction | None) -> tuple[str, ...]:
    "The role flags that a word of handler name may carry"
    family = inst.family.name if inst is not None and inst.family is not None else name
    return CALL_ROLE_FLAGS.get(family.removeprefix("INSTRUMENTED_"), ())


def execution_variants(analysis: Analysis) -> dict[str, ExecutionVariant]:
    # Synthetic inputs may reuse a specialization name without its family.
    return {name: variant for name, variant in EXECUTION_VARIANTS.items()
            if variant.base in analysis.instructions
            and analysis.instructions[variant.base].family is not None}


def canonical_handler(name: str) -> str:
    name = name.split("_XFER_", 1)[0]
    variant = EXECUTION_VARIANTS.get(name)
    return variant.base if variant is not None else name


def owner_name(field: str) -> str:
    """The owner of the borrowable input read through field: the value that
    IR_DETACH gives the handler, PyValue_NULL for a local"""
    return f"ir_own_{field}"


# The release of a view's owner, which is PyValue_NULL for a local, for each
# release of a view and its form for a value known to be in pointer space:
# the VIEW_CLOSE and POINTER_FORM columns of the rows
VIEW_RELEASES = {
    row.name: (row.view_close, row.pointer_form)
    for row in ir_contract.rows().values() if row.view_close
}
# The DSL calls that take a view: they give its owner, or a count of its
# own for a local, to the caller (IREmitter.take_view()): the rows that
# declare TAKES_VIEW()
VIEW_TAKES = frozenset(row.name for row in ir_contract.rows().values() if row.takes_view)
# The DSL calls that read a view's owner, so that the handler must have
# taken the input (IR_DETACH) before the statement that holds them
OWNER_READS = set(VIEW_RELEASES) | VIEW_TAKES

# The IR forms of the instruction macros (see STACK_INSTRUCTION_MACROS).
IR_INSTRUCTION_MACROS = {
    "FB_COUNTER": "((_PyFbHeader *)fb)",
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
            # NB_SUBSCR is the GetItem family's (register_ops/items.rop), not
            # a handler of BINARY_OP
            if m and m.group(1) not in ("OPARG_LAST", "SUBSCR"):
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
    discards: bool = False          # scalar result may use the IR_NO_DST sink
    unpacks: bool = False           # dst names a run of locals for the
                                    # elements, or IR_NO_DST, see UNPACKS
    # The rest of the instruction contract (pycore_ir.h, _PyIRFormat)
    jump_always: bool = False       # see jumps_always()
    names_target: bool = False      # see names_target_only()
    fb_layout: str | None = None    # the entry type, see FEEDBACK_LAYOUTS
    fb_kind: str | None = None      # its _PyFbKind
    metadata_inputs: frozenset[int] = frozenset()
    operands: str = "_PyIR_OPERANDS_NONE"   # a call: see OPERAND_CALLS

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
        if part.properties.always_exits or any(
                token.text in CALL_INVOKE_MARKERS for token in part.body.tokens()):
            break


def runs_code(inst: Instruction) -> bool:
    """Whether inst may run Python code, or release an object whose
    finalizer may (ir_records.RUNS_PYTHON), or is an instrumented twin,
    which fires a monitoring event and so calls a Python callback"""
    return inst.name.startswith("INSTRUMENTED_") or any(
        uop_record_effects(part) & RUNS_PYTHON for part in inst.parts if isinstance(part, Uop))


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
            if tkn.kind == "IDENTIFIER" and tkn.text == "FB_COUNTER":
                return True
    return False


@dataclass
class Entry:
    "An item on the abstract stack of external_io()"
    external: int                   # index of the external input, or -1
    item: StackItem
    aliased: bool = False           # assigned from an input, `out = in;`


def external_io(inst: Instruction) -> tuple[list[StackItem], list[Entry]]:
    """The inputs that inst takes from the stack, bottom first, and the
    entries it leaves there, which name an input when it passes through:
    the stack ends of the record's value walk (ir_records.walk_values())."""
    walk = walk_values(inst)
    return walk.inputs, [Entry(external, item, aliased)
                         for _, external, item, aliased in walk.outputs]


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
# Its owner is ir_own_<F>, the value that IR_DETACH gives the handler
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
    "IR_DUP_SHAPE_READ",
    "IR_GUARD_INLINE_INT", "IR_GUARD_ELEMENT_TYPE", "IR_GUARD_ELEMENT_INDEX",
    "IR_DUP_CANONICAL_ELEMENT",
    "coro_awaitable_can_transfer",
    "value_subscript", "value_binary_op",
    "PyValue_DUP", "PyValue_XDUP", "PyValue_AsObject",
    "PyValue_AsKnownObjectBorrow", "PyValue_TYPE", "PyValue_ObjectOrNull",
    "PyValue_BoolCheck", "PyValue_FloatOperandValues", "PyValue_InlineFloat",
    "PyValue_InlineInt64", "_PyValue_IntScaled", "PyValue_LongCheckExact",
    # The value dispatchers (pycore_value_ops.h): they read their operands
    # as values and take a count of what they keep
    "_PyValue_Contains", "_PyValue_Invert", "_PyValue_IsTrue", "_PyValue_IsTrueSlow",
    "_PyValue_Negative", "_PyValue_Positive", "_PyValue_RichCompare",
    "_PyValue_Str", "_PyValue_Repr", "_PyValue_ASCII", "_PyValue_Format",
    "_PyValue_SequenceTuple",
    # Attribute and subscript dispatchers (I4, I3)
    "_PyValue_GetAttr", "_PyValue_SetAttr", "_PyValue_GetMethod",
    "_PyGetSet_ValueRead",
    "_PySuper_LookupValue", "value_import_level", "super_call",
    "_PyValue_GetItem", "_PyValue_SetItem", "_PyValue_DelItem",
    "_PyValue_Size", "_PyValue_GetOptionalItem", "_PyValue_Send",
    # Monitoring events (pycore_instruments.h): their arguments are
    # borrows for the call
    "_Py_call_instrumentation_arg", "_Py_call_instrumentation_2args",
    "_Py_call_instrumentation_exc2",
    # Value-form container calls: they only read their key argument
    "PyDict_ContainsValue", "PySet_ContainsValue", "PyDict_DelValue",
    "_PyDict_ContainsValue", "_PySet_Contains",
    "_PyValue_RichCompareObject",
    # The contains function of an exact str, tuple or list, called through
    # the type's value slot (CONTAINS_OP_JUMP_SEQUENCE)
    "contains_slot",
    # Not a reader: PyDict_SetValue keeps counted copies of its key and
    # value (PyValue_XDUP of a borrow), so it neither keeps nor releases the
    # caller's count and a borrowed local may be passed to it; nor is its
    # internal entry _PyDict_SetValue, for the same reason
    "PyDict_SetValue", "_PyDict_SetValue",
    # Python/ceval.c: they read their value arguments, and the handler
    # releases them after
    "_PyEval_BinarySlice", "_PyEval_StoreName", "_PyEval_StoreSlice",
    "_PyEval_MatchClass",
    # Value-form entries of the object-form callees (H3): they box what
    # they need in their own frame
    "_PyEval_CallIntrinsic1", "_PyEval_CallIntrinsic2", "_PyEval_GetAwaitable",
    "_PyEval_CallMethodDescriptorO",
    "_PyEval_CallMethodDescriptorNoArgs",
    "_PyEval_CheckExceptTypeValidValue",
    "_PyEval_CheckExceptStarTypeValidValue", "_PyInterpolation_BuildValue",
    "monitor_stop_iteration", "PyValue_Borrow",
    "_PyEval_FormatKwargsError",
    # Python/specialize.c: they read types and values
    "_Py_Specialize_BinaryOp", "_Py_Specialize_CompareOp",
    "_Py_Specialize_ContainsOp",
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
    # Type tests and accessors
    "Py_TYPE", "PyAnyDict_CheckExact", "PyAnySet_CheckExact",
    "PyCoro_CheckExact", "PyGen_Check",
    "PyList_CheckExact",
    "PyTuple_CheckExact", "PyUnicode_CheckExact",
    # Operations that read their operands and count what they keep
    "PyErr_GivenExceptionMatches",
    # The value-form container calls (they read the container object and
    # the key as a value)
    "_PyDict_ContainsValue",
    "PyUnicode_Concat",

    "value_exception_group_match",
    "value_load_from_dict", "_PySet_Contains", "_PyTemplate_Build",
    "_PyUnicode_Equal",
    # The indirect calls of BINARY_OP_EXTEND, through a table of functions
    # that read their operands
    "action",
    # CONTAINS_OP_JUMP_SEQUENCE: the contains function of the container's type
    "contains_slot",
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
STACK_ONLY = ("CALL", "INSTRUMENTED_CALL", "RETURN_VALUE",
              "INSTRUMENTED_RETURN_VALUE", "LOAD_ATTR", "LOAD_GLOBAL", "INTERPRETER_EXIT",
              "EXIT_INIT_CHECK", "PUSH_NULL", "EXTENDED_ARG", "NOT_TAKEN",
              "LOAD_FAST_LOAD_FAST",
              "LOAD_FAST_BORROW_LOAD_FAST_BORROW", "STORE_FAST_LOAD_FAST",
              "STORE_FAST_STORE_FAST", "TO_BOOL", "UNARY_NOT", "POP_JUMP_IF_FALSE",
              "POP_JUMP_IF_TRUE", "POP_JUMP_IF_NONE", "POP_JUMP_IF_NOT_NONE",
              "JUMP_FORWARD", "JUMP_BACKWARD", "JUMP_BACKWARD_NO_INTERRUPT",
              "INSTRUMENTED_JUMP_FORWARD", "INSTRUMENTED_JUMP_BACKWARD",
              "INSTRUMENTED_POP_JUMP_IF_TRUE", "INSTRUMENTED_POP_JUMP_IF_FALSE",
              "INSTRUMENTED_POP_JUMP_IF_NONE", "INSTRUMENTED_POP_JUMP_IF_NOT_NONE",
              "JUMP_IF_FALSE_OR_POP", "JUMP_IF_TRUE_OR_POP",
              "INSTRUMENTED_JUMP_IF_FALSE_OR_POP", "INSTRUMENTED_JUMP_IF_TRUE_OR_POP",
              "LOAD_FAST", "LOAD_FAST_BORROW", "LOAD_FAST_CHECK", "LOAD_FAST_AND_CLEAR",
              "DELETE_FAST", "LOAD_CONST", "LOAD_SMALL_INT", "COPY", "SWAP",
              "POP_TOP", "MAKE_FUNCTION", "SET_FUNCTION_ATTRIBUTE",
              "CALL_INTRINSIC_1", "CALL_INTRINSIC_2",
              "LOAD_NAME", "LOAD_LOCALS", "LOAD_BUILD_CLASS",
              "LOAD_FROM_DICT_OR_GLOBALS", "STORE_NAME", "STORE_GLOBAL",
              "SETUP_ANNOTATIONS", "IMPORT_NAME", "IMPORT_FROM",
              "BUILD_TUPLE", "BUILD_LIST", "BUILD_SET", "BUILD_MAP",
              "LIST_APPEND", "SET_ADD", "MAP_ADD", "LIST_EXTEND", "SET_UPDATE",
              "DICT_UPDATE", "DICT_MERGE", "STORE_SUBSCR", "DELETE_SUBSCR",
              "BINARY_SLICE", "STORE_SLICE", "BUILD_SLICE",
              "STORE_ATTR", "LOAD_SUPER_ATTR", "INSTRUMENTED_LOAD_SUPER_ATTR",
              "LOAD_SPECIAL")

# Handlers that the register passes make and that no stack bytecode has, but
# whose opcode numbers lie in the opcode space (their instrumented forms are
# tables indexed by opcode).  They are not in opcode.opmap, which lists what
# co_code can hold.
IR_BASE_ONLY = ("JUMP_IF_FALSE_OR_POP", "JUMP_IF_TRUE_OR_POP",
                "INSTRUMENTED_JUMP_IF_FALSE_OR_POP",
                "INSTRUMENTED_JUMP_IF_TRUE_OR_POP", "MATERIALIZE_KWARGS")


def ir_only_handler_names(analysis: Analysis) -> list[str]:
    "The IR-only handlers, in the order that numbers them"
    return (sorted(analysis.ir_only) + binary_op_handlers(analysis)
            + list(execution_variants(analysis))
            + sorted(generator_boundaries(analysis)))


def ir_only_numbers(analysis: Analysis, count: int) -> list[int]:
    "The numbers of the first count IR-only handlers"
    wide = max(0, count - len(analysis.free_ids))
    return [*analysis.free_ids[:count], *range(256, 256 + wide)]


def handler_ids(analysis: Analysis) -> dict[str, int]:
    """Handler index of each handler, by name.  The real opcodes keep their
    opcode.  The IR-only handlers, in a fixed order, take the numbers below
    256 that no opcode holds, then continue from 256: a pseudo-instruction
    has no handler, so its number is free to name one.  BINARY_OP itself has
    none: its operations do, and a STACK_ONLY opcode has none."""
    ids = {name: inst.opcode for name, inst in analysis.instructions.items()
           if name != BINARY_OP_FAMILY and name not in STACK_ONLY}
    names = ir_only_handler_names(analysis)
    for number, name in zip(ir_only_numbers(analysis, len(names)), names):
        ids[name] = number
    return ids


def handler_instructions(analysis: Analysis) -> dict[str, Instruction]:
    "The instruction that each handler runs, by handler name"
    insts = all_instructions(analysis)
    result = {name: insts[name] for name in handler_ids(analysis)
              if name in insts}
    for name in binary_op_handlers(analysis):
        result[name] = insts[BINARY_OP_FAMILY]
    for name, variant in execution_variants(analysis).items():
        if variant.base in result:
            result[name] = result[variant.base]
    for name, boundary in generator_boundaries(analysis).items():
        result[name] = result[boundary.base]
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
    _CALL_KW_NON_PY puts its result where the callable was)."""
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


# What a SEND handler's uops may declare: they run Python and may enter a
# generator's frame, never poll, and leave by these exits
SEND_EFFECTS = frozenset(("reads", "writes", "allocates", "raises", "releases",
                          "runs_python", "switches_frame"))
SEND_EXITS = frozenset(("next", "branch", "jump", "miss", "deopt", "raise", "enter"))


def send_source_contract(inst: Instruction, fmt: IRFormat) -> tuple[Uop, ...] | None:
    """The native-step source footprint shared by normal and abrupt edges:
    the uops of a SEND handler up to the one that sends (the first that is
    no pass-through), each declared.

    The declarations are the contract (ir_contract.py verifies each body
    against its own): the effects and exits of every uop lie within
    SEND_EFFECTS and SEND_EXITS, a guard or specializer before the send
    passes its three inputs through, and the send is three scalar inputs
    and two or three outputs.  Normal transport still requires its final
    flush or child publication; exceptional transport independently
    requires every error/exit receipt.
    """
    if (fmt.group != "SEND" or not fmt.window or fmt.n_inputs != 0
            or fmt.dst_offset != 2 or
            any(not isinstance(part, (Uop, Skip)) for part in inst.parts)):
        return None
    inputs, outputs = external_io(inst)
    if (len(inputs) != 3 or len(outputs) not in (2, 3) or
        any(item.is_array() for item in inputs) or
        any(entry.item.is_array() for entry in outputs)):
        return None
    parts = [part for part in inst.parts if isinstance(part, Uop)]
    for index, part in enumerate(parts):
        contract = ir_contract.declared_contract(part)
        if contract.effects - SEND_EFFECTS or contract.exits - SEND_EXITS:
            return None
        if any(item.is_array() for item in (*part.stack.inputs, *part.stack.outputs)):
            return None
        shape = ([item.name for item in part.stack.inputs],
                 [item.name for item in part.stack.outputs])
        if len(part.stack.inputs) == 3 and shape[0] != shape[1]:
            # The send; its handler ends here, or it ends the handler
            if index != len(parts) - 1 and not part.properties.always_exits:
                return None
            return tuple(parts[:index + 1])
        # A guard or the specializer inspects or redispatches unchanged
        # inputs, and acquires no frame or home-pointer capability
        if shape[0] != shape[1] or contract.effects & {"writes", "switches_frame"}:
            return None
    return None


def metadata_only_passthroughs(inst: Instruction) -> frozenset[int]:
    """An unread unchanged cursor whose dependency stays in the compiler."""
    markers = [token for token in uop_token_list(inst)
               if token.text == "IR_METADATA_PASSTHROUGH"]
    if not markers:
        return frozenset()
    inputs, outputs = external_io(inst)
    parts = [part for part in inst.parts if isinstance(part, Uop)]
    expected = """{
        IR_METADATA_PASSTHROUGH(1);
        PyValue outgoing = TAKE(retval);
        IR_SUSPEND_DELEGATED(outgoing);
    }"""
    shape = lambda items: [(item.name, item.peek, item.size) for item in items]
    incoming = [("receiver", True, ""), ("unused_cursor", True, ""),
                ("retval", False, "")]
    outgoing = [("receiver", True, ""), ("unused_cursor", True, ""),
                ("value", False, "")]
    producer = parts[-1]
    observer = """{
        int err = _Py_call_instrumentation_arg(
            tstate, PY_MONITORING_EVENT_PY_YIELD, frame, this_instr, val);
        if (err) { RELOAD_STACK(); ERROR_NO_POP(); }
        if (frame->instr_ptr != this_instr) {
            RELOAD_STACK(); DISPATCH_REDIRECT();
        }
    }"""
    if (len(markers) != 1 or len(inputs) != 3 or len(outputs) != 3
            or len(parts) != len(inst.parts)
            or shape(producer.stack.inputs) != incoming
            or shape(producer.stack.outputs) != outgoing
            or outputs[0].external != 0 or outputs[1].external != 1
            or any(item.is_array() for item in inputs)
            or len(parts) > 2
            or any(statement_shape(part.body) != statement_shape(
                Parser(observer, "<delegated yield observer contract>").block())
                   for part in parts[:-1])
            or statement_shape(producer.body) != statement_shape(
                Parser(expected, "<delegated transport contract>").block())
            or any(token.text == "unused_cursor" for part in parts[:-1]
                   for token in part.body.tokens())):
        raise analysis_error("metadata passthrough requires the unchanged unread delegated cursor",
                             markers[0])
    return frozenset({1})


def operand_metadata(fmt: IRFormat) -> IRFormat:
    """The shape of a call family's words in the metadata: A is the dst, B
    the callable (an input), C the number of operands and D the feedback
    offset.  The other inputs are operand words that follow the head, which
    no field names, so the format is not a window."""
    assert fmt.operands != "_PyIR_OPERANDS_NONE"
    return replace(
        fmt, window=False,
        roles=[ROLE_DST, f"{ROLE_IN0} + 0", ROLE_IMM, ROLE_FB],
        inputs=["callable"], n_inputs=1, borrow_mask=1)


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
    metadata_inputs = metadata_only_passthroughs(head)
    if any(metadata_only_passthroughs(member) != metadata_inputs for member in members):
        raise analysis_error("metadata passthroughs must match the complete family", head.where)
    # Members that pass their first outputs through write later slots, which
    # follow dst. Members that push a frame and no outputs write none, and
    # members that return write theirs to the caller's frame.
    new_outputs = [
        None if m.properties.returns
        else first_new_output(io[m.name][1], len(io[m.name][0]))
        for m in members
    ]
    dst_offset = min((o for o in new_outputs if o is not None), default=None)
    # These semantic operations have distinct body/yield and completion
    # outcomes. Their completion markers replace JUMPBY, but the executable
    # ABI remains an explicit window and a real forward exit.
    completes_step = group in {"FOR_ITER", "SEND", "CLEANUP_THROW"}
    array_inputs = completes_step or any(m.properties.window for m in members) or any(
        item.is_array() for m in members for item in io[m.name][0])
    jumps = completes_step or any(m.properties.jumps for m in members)
    fused = group in FUSED_JUMPS
    forward = jumps and (group in FORWARD_JUMPS or fused)
    # A fused word's entry holds its operation and the sense of its jump
    has_fb = fused or any(uses_feedback(m) for m in members)
    # CLEANUP_THROW has no operand in co_code, but its word names the exit
    has_imm = any(m.properties.oparg for m in members) or group == "CLEANUP_THROW"
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
    if metadata_inputs:
        # The cursor is an unchanged compiler dependency, not a physical
        # input window. Receiver and outgoing homes remain independent.
        placed = ([ROLE_DST, f"{ROLE_IN0} + 2", ROLE_NONE, f"{ROLE_IN0} + 0"],
                  LOC_NONE, LOC_NONE)
    elif not array_inputs:
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
        # Every operand call has its count in C and its feedback index in D,
        # also a CALL_FUNCTION_EX, which has no oparg of its own
        if group in OPERAND_CALLS and imm_kind is None:
            imm_kind = "narrow"
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
        if (not window and not metadata_inputs and m_dst is not None and m.properties.switches_frame
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
        if n_inputs > len(m_inputs) or any(item.is_array()
                                          for item in m_inputs[-n_inputs:]):
            raise analysis_error("window input fields require trailing scalar inputs",
                                 head.where)
        first = len(m_inputs) - n_inputs
        uses = classify_inputs(head, len(m_inputs))
        passthrough = {e.external for e in m_outputs if e.external >= 0}
        kept = kept_inputs(head, len(m_inputs))
        for k in range(n_inputs):
            if first + k in passthrough or first + k in kept:
                continue
            if uses[first + k] in ("object", "raw"):
                borrow_mask |= 1 << k
    if metadata_inputs:
        # The finite suspension certificate retains scratch-role identities
        # across the monitoring callback. Neither input may borrow a mutable
        # binding, even when its individual C uses would permit a view.
        borrow_mask = 0
    store_dst = bool(metadata_inputs) or stores_to_dst(members, io, dst_offset, window)
    fb_layout = fb_kind = None
    if has_fb:
        if group not in FEEDBACK_LAYOUTS:
            raise analysis_error(
                f"{group} has a feedback entry but no layout in "
                "FEEDBACK_LAYOUTS (ir_generator.py)", head.where)
        fb_layout = FEEDBACK_LAYOUTS[group]
        fb_kind = LAYOUT_KINDS[fb_layout]
    return IRFormat(
        group=group, window=window, roles=roles, inputs=names,
        n_inputs=n_inputs, dst_offset=dst_offset, borrow_mask=borrow_mask,
        imm=imm, fb=fb, jumps=jumps and not forward, forward=forward,
        fused=fused,
        no_fold=no_fold,
        store_dst=store_dst, runs_code=any(runs_code(m) for m in members),
        discards=(store_dst and store_mode(head, io[head.name][1]) in
            (STORE_RESULT, STORE_RETURN)) or group in
            {"SEND", "CLEANUP_THROW", "YIELD_VALUE", "YIELD_DELEGATED", "RETURN_GENERATOR"},
        unpacks=unpacks,
        jump_always=jumps and jumps_always(head),
        names_target=jumps and names_target_only(head),
        fb_layout=fb_layout, fb_kind=fb_kind,
        metadata_inputs=metadata_inputs,
        operands=OPERAND_CALLS.get(group, "_PyIR_OPERANDS_NONE"),
    )


# The entry type of each family's feedback (Include/internal/pycore_feedback.h),
# by the head of the family; the C code names it _PyFb_<head>.  This is the
# only list: the contract's fb_kind and fb_size and the typedefs come from
# it.  A family that has an entry must be listed.  Its kind follows from
# the type.
FB_HEADER = "_PyFbHeader"
FEEDBACK_LAYOUTS = {
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


def committed_markers() -> frozenset[str]:
    """The rows that observe a committed exit: those declared
    EXITS(...(committed)) (ir_contract.Row.committed), whose observer the
    emitter's replacer generates"""
    return frozenset(row.name for row in ir_contract.rows().values() if row.committed)


def observer_terminal_arguments(tokens: list[Token]) -> tuple[list[str], int]:
    """One literal owner edge, independent of the reported monitoring event."""
    if len(tokens) < 4 or tokens[1].kind != "LPAREN":
        raise analysis_error("malformed terminal observer", tokens[0])
    args: list[list[str]] = [[]]
    depth = 0
    end = -1
    for i, token in enumerate(tokens[2:], 2):
        if token.kind == "RPAREN" and depth == 0:
            end = i
            break
        if token.kind in ("LPAREN", "LBRACKET"):
            depth += 1
        elif token.kind in ("RPAREN", "RBRACKET"):
            depth -= 1
        if token.kind == "COMMA" and depth == 0:
            args.append([])
        else:
            args[-1].append(token.text)
    if end < 0 or len(args) != 4 or any(not arg for arg in args):
        raise analysis_error("terminal observer takes source, destination, event, owner edge", tokens[0])
    if [t.kind for t in tokens[end + 1:]] != ["SEMI"]:
        raise analysis_error("semantic work follows terminal observer", tokens[0])
    if args[2] not in (["PY_MONITORING_EVENT_BRANCH_LEFT"],
                       ["PY_MONITORING_EVENT_BRANCH_RIGHT"],
                       ["PY_MONITORING_EVENT_JUMP"]):
        raise analysis_error("terminal observer requires a literal event", tokens[0])
    edges = {"_PyIR_EDGE_FALLTHROUGH": 0, "_PyIR_EDGE_TAKEN": 1}
    if len(args[3]) != 1 or args[3][0] not in edges:
        raise analysis_error("terminal observer requires a literal owner edge", tokens[0])
    edge = edges[args[3][0]]
    targets = (["JUMP_TARGET", "(", "oparg", ")"],
               ["next_instr", "+", "IR_FUSED_DISTANCE", "(", ")"])
    if (edge == 0 and args[1] != ["next_instr"] or
            edge == 1 and args[1] not in targets):
        raise analysis_error("terminal observer owner edge disagrees with destination", tokens[0])
    return [" ".join(arg) for arg in args], edge


def observer_terminal_paths(inst: Instruction, fmt: IRFormat) -> dict[Token, tuple[int, str]]:
    """Check terminal composition and distinguish a defined NULL result.

    Storage.push_outputs supplies the separate live-input/defined-output proof
    when each marker is emitted. Metadata and emission use these same paths.
    """
    paths: dict[Token, tuple[int, str]] = {}
    markers_declared = committed_markers()
    table = ir_contract.rows()
    if not markers_declared & set(uop_tokens(inst)):
        return paths
    parts = [p for p in inst.parts if not isinstance(p, Skip)]

    def walk(stmt, nulls: set[str], owners: set[str], spent: set[str],
             deferred: set[str], following: bool, uop: Uop):
        if isinstance(stmt, BlockStmt):
            for index, child in enumerate(stmt.body):
                if not walk(child, nulls, owners, spent, deferred,
                            following or index + 1 < len(stmt.body), uop):
                    return False
            return True
        if isinstance(stmt, IfStmt):
            left = (set(nulls), set(owners), set(spent), set(deferred))
            right = (set(nulls), set(owners), set(spent), set(deferred))
            left_live = walk(stmt.body, *left, following, uop)
            right_live = True
            if stmt.else_body is not None:
                right_live = walk(stmt.else_body, *right, following, uop)
            if left_live and right_live:
                if left[1] != right[1] or left[3] != right[3]:
                    raise analysis_error("ambiguous terminal C ownership at join", stmt.if_)
                result = (left[0] & right[0], left[1], left[2] | right[2], left[3])
            elif left_live:
                result = left
            else:
                result = right
            for destination, source in zip((nulls, owners, spent, deferred), result):
                destination.clear()
                destination.update(source)
            return left_live or right_live
        tokens = [t for t in stmt.tokens() if t.kind != "COMMENT"]
        markers = [t for t in tokens if t.text in markers_declared]
        if not isinstance(stmt, SimpleStmt):
            raise analysis_error("terminal observer in unsupported control flow", tokens[0])
        texts = [t.text for t in tokens]
        def mentions(name):
            pieces = name.replace('.', ' . ').split()
            # (a field of another value is not the local of the same name)
            return any(texts[i:i + len(pieces)] == pieces and (i == 0 or texts[i - 1] != ".")
                       for i in range(len(texts)))
        for name in spent:
            if mentions(name):
                raise analysis_error("terminal C owner used after transfer: " + name, tokens[0])
        def transfer(name):
            if name in owners:
                owners.remove(name)
                spent.add(name)

        used_owners = {name for name in owners if mentions(name)}
        if used_owners:
            # A statement that mentions a C owner calls only rows: an owner
            # passed whole to an argument the row consumes is transferred
            # (the helper's, TAKE's and IR_COMPLETE_STEP's declarations), one
            # passed to a borrowed argument stays held; any other use of it
            # in a call proves nothing
            for call, args in ir_contract._calls_in(tokens):
                row = table.get(call.text)
                if row is None:
                    raise analysis_error("unproved terminal C owner transport", tokens[0])
                if call.text == "IR_DEFER_CLOSE":
                    continue        # held until the observer, checked below
                for position, arg in enumerate(args):
                    whole = "".join(arg)
                    inside = any(arg[i:i + len(pieces)] == pieces
                                 for name in used_owners
                                 for pieces in [name.replace(".", " . ").split()]
                                 for i in range(len(arg)))
                    if not inside:
                        continue
                    mode = row.args[position].mode if position < len(row.args) else ""
                    if whole in used_owners and mode == "consumed":
                        transfer(whole)
                    elif not (whole in used_owners and mode == "borrowed"):
                        raise analysis_error("unproved terminal C owner transport", tokens[0])
            calls = {texts[i] for i in range(len(texts) - 1)
                     if tokens[i].kind == "IDENTIFIER" and texts[i + 1] == "("}
            bindings = declared_locals(uop_token_list(inst))
            if any(name in bindings or any(c.name == name for c in uop.caches)
                   for name in calls):
                raise analysis_error("shadowed terminal owner helper", tokens[0])

        if texts[:1] == ["IR_DEFER_CLOSE"]:
            if len(texts) != 5 or texts[2] not in owners:
                raise analysis_error("deferred observer owner is not held", tokens[0])
            transfer(texts[2])
            deferred.add(texts[2])
        output_names = {item.name for item in uop.stack.outputs}
        if len(texts) > 3 and texts[:1] == ["PyValue"] and texts[2] == "=":
            rhs = "".join(texts[3:-1])
            transfer(rhs)
            if rhs not in ("PyValue_NULL", "PyValue_None", "PyValue_True", "PyValue_False"):
                owners.add(texts[1])
        elif (len(texts) > 4 and texts[2] == "=" and texts[3] in table and
                table[texts[3]].result.shape == "CASES"):
            # A struct result: the fields that its row declares owned are C
            # owners, as `step.value` (the helper's declared outcomes)
            helper = texts[3]
            if (helper in declared_locals(uop_token_list(inst)) or
                    any(cache.name == helper for cache in uop.caches)):
                raise analysis_error("unproved terminal status owner", tokens[0])
            for case in table[helper].result.cases:
                for field, spec in case.values:
                    if spec.mode == "owned":
                        owners.add(f"{texts[1]}.{field}")
        elif texts[:2] == ["PyObject", "*"]:
            raise analysis_error("unproved terminal C object owner", tokens[0])
        if len(texts) > 2 and texts[0] in VIEW_RELEASES and texts[1] == "(":
            transfer("".join(texts[2:-2]))
        if len(texts) > 2 and texts[1] == "=" and texts[0] in output_names:
            transfer("".join(texts[2:-1]))
        elif len(texts) > 2 and texts[1] == "=" and texts[0] in owners:
            raise analysis_error("terminal C owner overwritten before transfer", tokens[0])
        elif (len(texts) > 2 and texts[1] == "=" and
              "".join(texts[2:-1]) in owners):
            raise analysis_error("unproved terminal C owner assignment", tokens[0])
        if len(texts) > 2 and texts[1] == "=":
            if texts[2:] == ["PyValue_NULL", ";"]:
                nulls.add(texts[0])
            else:
                nulls.discard(texts[0])
        if not markers:
            exits = texts and texts[0] in {
                "ERROR_NO_POP", "IR_COMPLETE_STEP", "IR_RETURN_TO_CALLER", "DISPATCH",
                "DISPATCH_REDIRECT"}
            if exits and deferred:
                raise analysis_error("deferred owner exits before terminal observer", tokens[0])
            return not exits
        marker = markers[0]
        if owners:
            raise analysis_error("terminal observer has untransferred C owners: " +
                                 ", ".join(sorted(owners)), marker)
        if len(markers) != 1 or texts[0] not in markers_declared or following:
            raise analysis_error("semantic work follows terminal observer", marker)
        if not parts or parts[-1] is not uop:
            raise analysis_error("terminal observer must finish the composed handler", marker)
        _, edge = observer_terminal_arguments(tokens)
        inputs, outputs = external_io(inst)
        if fmt.window:
            if (len(inputs) != 2 or len(outputs) != 3 or
                    any(x.size for x in inputs) or any(x.item.size for x in outputs) or
                    fmt.dst_offset != 2 or not fmt.store_dst):
                raise analysis_error("unsupported terminal observer window", marker)
            kind = "_PyIR_OBSERVER_FOR_BODY"
        elif len(outputs) == 1 and uop.stack.outputs and uop.stack.outputs[-1].name in nulls:
            kind = "_PyIR_OBSERVER_FORMAT_NULL_FIRST"
        else:
            kind = "_PyIR_OBSERVER_FORMAT_VALUE"
        paths[marker] = edge, kind
        return False

    for part in parts:
        if isinstance(part, Uop) and any(t.text in markers_declared for t in part.body.tokens()):
            deferred: set[str] = set()
            live = walk(part.body, set(), set(), set(), deferred, False, part)
            if live and deferred:
                raise analysis_error("deferred owner leaves terminal observer path", inst.where)
    return paths


def observer_outcomes(inst: Instruction, fmt: IRFormat) -> tuple[str, str]:
    result = ["_PyIR_OBSERVER_NONE", "_PyIR_OBSERVER_NONE"]
    for edge, kind in observer_terminal_paths(inst, fmt).values():
        if result[edge] not in ("_PyIR_OBSERVER_NONE", kind):
            raise analysis_error("conflicting terminal observer edge facts", inst.where)
        result[edge] = kind
    tokens = set(uop_tokens(inst))
    if "IR_COMPLETE_STEP" in tokens:
        result[1] = "_PyIR_OBSERVER_COMPLETION"
    return tuple(result)


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
        elif tkn.text in ("JUMPBY", "JUMPTO", "IR_COMPLETE_STEP"):
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
            if tkn.text == "IR_COMPLETE_STEP":
                # Its checked arguments are kind, held value, target. The
                # target is a real jump, not the other outcome's successor.
                arg = arg[4:]
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


# The uses of GETLOCAL(oparg) that oparg_slot_role() knows.  Any other use
# is an error, so that a write the derivation does not recognise cannot pass
# as a read.  The superinstructions address their slots as GETLOCAL(oparg1)
# and GETLOCAL(oparg2); flowgraph.c creates none for the register IR, and
# their packed opargs keep role IMM.
SLOT_VALUE_READS = {        # f(GETLOCAL(oparg), ...) reads the slot's value
    "PyValue_DUP", "PyValue_IsNull", "PyCell_NewValue",
    "PyValue_AsKnownObjectBorrow", "PyValue_Borrow", "_PyValue_BoundLocal",
    "_PyValue_CellLocal",
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


@dataclass(frozen=True)
class GeneratorEntryRecipe:
    part: Uop
    generator: str
    kind: str


def generator_entry_recipe(inst: Instruction) -> GeneratorEntryRecipe | None:
    """The terminal intrinsic owns this child's yielded-result recipient.

    This describes recipient transport only. The normal/error source contracts
    still decide whether the surrounding operation preserves home authority.
    Conditional entry in a generic SEND is not an unconditional return recipe.
    """
    if any(not isinstance(part, (Uop, Skip)) for part in inst.parts):
        return None
    parts = [part for part in inst.parts if isinstance(part, Uop)]
    entries = [part for part in parts if any(
        token.text == "IR_ENTER_GENERATOR" for token in part.body.tokens())]
    if len(entries) != 1:
        return None
    entry = entries[0]
    statements = entry.body.body
    if not statements or not isinstance(statements[-1], SimpleStmt):
        return None
    tokens = list(statements[-1].tokens())
    if (len(tokens) != 7 or tokens[0].text != "IR_ENTER_GENERATOR"
            or tokens[1].kind != "LPAREN" or tokens[2].kind != "IDENTIFIER"
            or tokens[3].kind != "COMMA" or tokens[5].kind != "RPAREN"
            or tokens[6].kind != "SEMI"
            or tokens[4].text not in {"_PY_IR_FOR_ITER", "_PY_IR_SEND"}
            or sum(token.text == "IR_ENTER_GENERATOR"
                   for token in entry.body.tokens()) != 1):
        return None
    if (set(entry.properties.escaping_calls) != {statements[-1]}
            or any(token.text in {"frame", "slots", "window"}
                   or token.text.startswith(("_PyFrame_", "_PyHomeBitmap_", "IR_"))
                   for statement in statements[:-1] for token in statement.tokens())):
        return None
    kind = tokens[4].text
    count = 2 if kind == "_PY_IR_FOR_ITER" else 3
    if (len(entry.stack.inputs) != count or len(entry.stack.outputs) != 2
            or any(item.is_array() for item in
                   entry.stack.inputs + entry.stack.outputs)
            or any(passed_through(entry, j) != j for j in range(2))):
        return None
    at = parts.index(entry)
    for part in parts[:at]:
        if not part.is_guard():
            return None
        effect = analyze_error_effects(replace(inst, parts=[part]), guard_prefix=True)
        if (effect is None or effect.homes != HomeEffect.NO_ARENA_WRITE
                or effect.callback or effect.changes_origin or effect.may_fail
                or effect.error_sources or effect.unwind_sources):
            return None
    # The intrinsic's frame handoff is terminal: nothing follows it (VT2)
    if parts[at + 1:]:
        return None
    inputs, outputs = external_io(inst)
    if (len(inputs) != count or len(outputs) != 2
            or any(item.is_array() for item in inputs)
            or any(output.external != j for j, output in enumerate(outputs))):
        return None
    return GeneratorEntryRecipe(entry, tokens[2].text, kind)


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
    entry = generator_entry_recipe(inst)
    if entry is not None and entry.kind == "_PY_IR_FOR_ITER":
        # The child's yields go to A through the ordinary return-slot protocol,
        # including local replacement and discard. SEND instead yields to B+2.
        return STORE_RETURN
    # The terminal adapter publishes the native result itself, or the Python
    # child's eventual return. Its unused DSL C output still has a real dst.
    if inst.properties.always_exits and "IR_COMPLETE_STEP" in uop_tokens(inst):
        # Completion owns its explicit recipient; frontend output aliases
        # describe stack conversion, not stores performed after the marker.
        return None
    if uses_generic_call(inst):
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


def unpacks_elements_directly(inst: Instruction, fmt: "IRFormat") -> bool:
    """Whether an unpack writes its elements one at a time to the output
    window (IR_WRITE_OUTPUT), so that each can go straight to its local."""
    tokens = uop_tokens(inst)
    return fmt.unpacks and ("IR_WRITE_OUTPUT" in tokens or "IR_UNPACK_ITEMS" in tokens)


def uop_tokens(inst: Instruction) -> list[str]:
    return [t.text for part in inst.parts if isinstance(part, Uop)
            for t in part.body.tokens()]


def names_return_slot(inst: Instruction) -> bool:
    """Whether the frames that inst pushes return only their result, to the
    slot dst names.  A generator's frame also hands each value it yields to
    that slot, and its return value at the end of a FOR_ITER."""
    tokens = uop_tokens(inst)
    # _PyEval_CreateInitFrame takes the return slot from the call's own word
    return ("IR_SUSPEND_DELEGATED" in tokens or
            "_PyEval_CreateInitFrame" in tokens or
            "IR_DST_SLOT" in tokens and "gi_iframe" not in tokens)


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
    for name, variant in execution_variants(analysis).items():
        if variant.base not in formats:
            continue
        fmt = formats[variant.base]
        local_inputs = {fmt.input_field(k) for k in range(fmt.n_inputs)
                        if fmt.borrow_mask & (1 << k)}
        if ((fmt.window and (variant.local_fields or variant.temp_fields))
                or not set(variant.local_fields) <= local_inputs
                or not set(variant.temp_fields) <= {fmt.field_of(ROLE_DST)}
                or set(variant.local_fields) & set(variant.temp_fields)):
            raise ValueError(f"invalid operand modes for {name}")
        formats[name] = fmt
    for name, boundary in generator_boundaries(analysis).items():
        formats[name] = formats[boundary.base]
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
    local_fields: tuple[str, ...] = ()
    temp_fields: tuple[str, ...] = ()
    store_dst: bool = False         # dst may name a local
    canonical_frame_transfer: bool = False  # proven unchanged canonical slot input
    call_kind: str | None = None
    call_results: frozenset[str] = frozenset()  # names of a call's outputs
    # Window: the slot, above the inputs', whose result goes to a dst that
    # may name a local, and the dst's macro arguments
    move_dst: int | None = None
    move_dst_args: str = ""

    @staticmethod
    def empty() -> "SlotLayout":
        return SlotLayout(False, PointerOffset.zero(), {}, frozenset())


@dataclass(frozen=True)
class OwnershipEvent:
    kind: str
    flags: int = 0
    dst: int = 0
    src: int | str = 0
    extent: int = 0


@dataclass(frozen=True)
class ErrorCheckpoint:
    id: int
    flags: int
    node: int = 0
    actions: tuple[OwnershipEvent, ...] = ()


ELEMENT_GUARDS = frozenset(("IR_GUARD_INLINE_INT", "IR_GUARD_ELEMENT_TYPE",
                            "IR_GUARD_ELEMENT_INDEX"))
ELEMENT_INTRINSICS = ELEMENT_GUARDS | {"IR_DUP_CANONICAL_ELEMENT"}
ELEMENT_NATIVE_SYMBOLS = frozenset((
    "PyValue_IsInlineInt", "PyValue_InlineInt64", "PyValue_IsObject",
    "PyValue_AsKnownObjectBorrow", "PyValue_XDUP", "PyValue_DUP", "PyList_CheckExact",
    "PyTuple_CheckExact", "PyList_GET_SIZE", "PyTuple_GET_SIZE",
    "PyList_GET_VALUE", "PyTuple_GET_VALUE",
))


class ScalarStorageGraph:
    """Persistent source actions shared by the actual IRStack branches.

    Node zero is the incoming scalar state. A node is also a possible native
    value identity, so SAVE can retain a value across a later VACATE without
    consulting the changed home. References are topological, bounded by the
    existing uint16 ownership-event encoding.
    """

    def __init__(self):
        self.nodes: list[OwnershipEvent] = []
        self.interned: dict[OwnershipEvent, int] = {}

    def action(self, kind: str, parent: int, field: int = 0,
               value: int = 0, flags: int = 0) -> int:
        if not 0 <= parent <= len(self.nodes) or not 0 <= value <= len(self.nodes):
            raise StackError("scalar storage action has a forward reference")
        event = OwnershipEvent(kind, flags=flags, dst=parent, src=field, extent=value)
        known = self.interned.get(event)
        if known is not None:
            return known
        if len(self.nodes) == 65535:
            raise StackError("scalar storage action graph is too large")
        self.nodes.append(event)
        node = len(self.nodes)
        self.interned[event] = node
        return node

    def join(self, left: int | None, right: int | None) -> int | None:
        if left is None or right is None:
            return None
        if left == right:
            return left
        return self.action("_PyIR_OWNER_STORAGE_JOIN", left, value=right)


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


def allocates(tokens: list[Token]) -> bool:
    "Whether the expression calls a row that declares `allocates`"
    table = ir_contract.rows()
    return any(tkn.kind == "IDENTIFIER" and i + 1 < len(tokens)
               and tokens[i + 1].kind == "LPAREN" and tkn.text in table
               and "allocates" in table[tkn.text].effects
               for i, tkn in enumerate(tokens))


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
    return None


def null_output_names(inst: Instruction, fmt: IRFormat) -> frozenset[str] | None:
    """Names whose direct assignments may reach the actual scalar DST store.

    Native address/member aliases and shadowed declarations are deliberately
    outside this transport. They must not leave a remembered NULL assignment
    live across a later indirect C write.
    """
    if fmt.window or fmt.field_of(ROLE_DST) != "A":
        return None
    outputs = frozenset(item.item.name for item in external_io(inst)[1]
                        if not item.item.is_array())
    tokens = uop_token_list(inst)
    bindings = declared_locals(tokens)
    if ((outputs | {"PyValue_NULL"}) & bindings.keys() or
            any(kind in {"pointer", "array"} for kind in bindings.values()) or
            any(cache.name in outputs | {"PyValue_NULL"}
                for part in inst.parts if isinstance(part, Uop) for cache in part.caches)):
        return None
    for index, token in enumerate(tokens):
        if (token.text in {"for", "while"} or token.kind.startswith("CMACRO") or
                (token.kind == "AND" and is_unary_and(tokens, index))):
            return None
        if token.text in outputs:
            before = tokens[index - 1].text if index else ""
            after = tokens[index + 1].text if index + 1 < len(tokens) else ""
            if before in {"&", "++", "--", ".", "->"} or after in {
                    "[", ".", "->", "++", "--"}:
                return None
    return outputs


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
        # position, the latest last: IR_DETACH adopts the bits that
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
        # An error edge after the call finished: the result is published first
        self.publish_on_error = False
        # The closes of the inputs that the handler detached, in order: rule
        # 7 runs them after the last store (IREmitter.emit_deferred).  Each
        # path has its own list, and the paths that join agree on it.
        self.deferred: list[str] = []
        # The IR_STORE_DST and IR_MOVE_TO_DST lines emitted for the handler,
        # which close ir_old after them.  Copies share the list.
        self.dst_stores: list[str] = []
        # The result already went to dst (IR_MOVE_RESULT_TO_DST)
        self.moved_dst = False
        # Must-NULL output facts follow the actual structured storage paths.
        # Distinct normal successors keep distinct facts until the final store.
        self.null_paths: dict[int, set[str]] = {0: set()}
        self.null_stores: dict[int, bool] = {}
        self.null_uncovered = False
        # Each emitted storage operation needs its matching action receipt.
        # A logical IO declaration alone cannot establish its physical effect.
        self.home_writes = 0
        self.home_write_receipts = 0
        self.read_fields: dict[int, bool] = {}
        self.unknown_read = False
        self.native_values: dict[str, Local] = {}
        self.home_graph = ScalarStorageGraph()
        self.home_node: int | None = 0
        self.home_values: dict[int, int] = {}
        self.native_owners: set[int] = set()
        self.possible_native_owners: set[int] = set()
        self.expired_values: set[int] = set()
        self.loaded_value: int | None = None
        # A checked guard supplies one offset to a read of the same current
        # receiver. It is metadata, not a value or an owning Storage token.
        self.shape_read_binding: tuple[str, str] | None = None
        self.shape_read_complete = False
        self.element_guards: dict[tuple[str, int], str] = {}
        # A lexical C index, its actual value/anchor identities and scope.
        self.element_indices: dict[str, tuple[int, int, str, int]] = {}

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
        other.deferred = list(self.deferred)
        other.null_paths = {edge: set(names) for edge, names in self.null_paths.items()}
        other.null_stores = dict(self.null_stores)
        other.null_uncovered = self.null_uncovered
        other.home_writes = self.home_writes
        other.home_write_receipts = self.home_write_receipts
        other.read_fields = dict(self.read_fields)
        other.unknown_read = self.unknown_read
        other.native_values = {name: var.copy() for name, var in self.native_values.items()}
        other.home_values = dict(self.home_values)
        other.native_owners = set(self.native_owners)
        other.possible_native_owners = set(self.possible_native_owners)
        other.expired_values = set(self.expired_values)
        other.loaded_value = None
        other.shape_read_binding = self.shape_read_binding
        other.shape_read_complete = self.shape_read_complete
        other.element_guards = dict(self.element_guards)
        other.element_indices = dict(self.element_indices)
        return other

    def home_action(self, kind: str, field: int = 0,
                    value: int = 0, flags: int = 0) -> int | None:
        if self.home_node is None:
            return None
        self.home_node = self.home_graph.action(kind, self.home_node, field, value, flags)
        return self.home_node

    def take_field(self, field: str) -> None:
        self.home_write_receipts += 1
        index = FIELDS.index(field)
        self.home_action("_PyIR_OWNER_STORAGE_VACATE", index, flags=1)
        token = self.home_values.pop(index, None)
        if token is None:
            self.home_node = None
        else:
            self.native_owners.add(token)
            self.possible_native_owners.add(token)

    def vacate_field(self, field: str) -> None:
        self.home_write_receipts += 1
        index = FIELDS.index(field)
        self.home_action("_PyIR_OWNER_STORAGE_VACATE", index)
        self.home_values.pop(index, None)

    def publish_field(self, field: str, var: Local, *, temporary=False) -> None:
        self.home_write_receipts += 1
        index = FIELDS.index(field)
        if var.ir_value is None or var.ir_value in self.expired_values:
            self.home_node = None
            return
        same_value = self.home_values.get(index) == var.ir_value
        if var.ir_value not in self.native_owners and not same_value:
            # A borrowed C copy does not carry a count into a new home.
            self.home_node = None
            return
        flags = int(temporary) | (int(same_value) << 1)
        self.home_action("_PyIR_OWNER_STORAGE_PUBLISH", index, var.ir_value, flags)
        self.home_values[index] = var.ir_value
        # Scratch publication owns the count in the frame. Local/discard
        # modes still owe a conditional final commit, but this is no longer
        # definite permission to copy or close a native C owner.
        self.native_owners.discard(var.ir_value)
        if not temporary:
            if var.ir_value in self.possible_native_owners:
                self.expired_values.add(var.ir_value)
            self.native_owners.discard(var.ir_value)
            self.possible_native_owners.discard(var.ir_value)

    def refine_value(self, var: Local, nonnull: bool) -> None:
        """Refine only the actual current native token on a selected arm."""
        value = var.ir_value
        if value is None or value not in self.native_owners:
            return
        refined = self.home_action("_PyIR_OWNER_STORAGE_GUARD", value=value,
                                   flags=int(nonnull))
        self.native_owners.discard(value)
        self.possible_native_owners.discard(value)
        self.expired_values.add(value)
        # A plain C alias denotes this same value on this arm. It does not
        # acquire an independent owner from the refinement.
        for local in self.variables + list(self.native_values.values()):
            if local.ir_value == value:
                local.ir_value = refined
        var.ir_value = refined
        if nonnull and refined is not None:
            self.native_owners.add(refined)
            self.possible_native_owners.add(refined)

    def retire_value(self, value: int | None) -> None:
        if value is not None:
            if value not in self.native_owners:
                self.home_node = None
            self.native_owners.discard(value)
            self.possible_native_owners.discard(value)
            self.expired_values.add(value)

    def select_null_edge(self, edge: int) -> None:
        facts = set.intersection(*(set(names) for names in self.null_paths.values()))
        self.null_paths = {edge: facts}

    def note_null_statement(self, stmt: SimpleStmt, outputs: frozenset[str]) -> None:
        texts = _statement_text(stmt)
        for name in outputs:
            if name not in texts:
                continue
            # An unknown macro may retain an lvalue without spelling '&'.
            # Only direct assignments participate in this source transport.
            if texts[:2] != [name, "="]:
                self.null_uncovered = True
            for names in self.null_paths.values():
                names.discard(name)
                if texts == [name, "=", "PyValue_NULL", ";"]:
                    names.add(name)
        if texts[:1] == ["JUMPTO"]:
            if texts == ["JUMPTO", "(", "oparg", ")", ";"]:
                self.select_null_edge(1)
            else:
                self.null_uncovered = True
        elif texts[:1] == ["JUMPBY"] and texts != ["JUMPBY", "(", "0", ")", ";"]:
            self.null_uncovered = True
        elif "next_instr" in texts and any(t in texts for t in ("=", "+=", "-=")):
            self.null_uncovered = True

    def code_ran(self, *, current_identity=False) -> None:
        "Code that can rebind a local may have run: each view may dangle"
        self.shape_read_binding = None
        self.home_action("_PyIR_OWNER_STORAGE_CALLBACK", flags=2 if current_identity else 0)
        self.element_guards.clear()
        self.element_indices.clear()
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
        if self.layout.call_kind is not None:
            return self.call_operand(p)
        return f"window[{(p - self.layout.base).to_c()}]"

    def call_operand(self, p: PointerOffset) -> str:
        """Position p of a call's window, which the DSL numbers as the
        callable, the self or NULL, then the arguments (CALL_KW: then the
        names, CALL_FUNCTION_EX: the unused self, the positional tuple, the
        kwargs).  A call has no window: the value lives in the home that
        its word names, the callable in the head and the others in the
        operand words (IR_OPERAND(k)), and it is read, not owned."""
        relative = (p - self.layout.base).as_int()
        if relative == 0:
            return "IR_SLOT(IR_B(ir_word))"
        kind = self.layout.call_kind
        if kind == "CALL_EX":
            if relative in (2, 3):
                return f"IR_OPERAND({relative - 2})"
            raise StackError(f"no operand for position {p} of a call")
        if relative == 1:
            return "IR_OPERAND(0)"
        if relative is None:
            if kind == "CALL_KEYWORDS" and (
                    (p - self.layout.base).to_c() in ("oparg + 2", "2 + oparg")):
                return "IR_CALL_KWNAMES()"
            raise StackError(f"no operand for position {p} ({(p - self.layout.base).to_c()}) of a call")
        return f"IR_OPERAND({relative - 2} + IR_CALL_HAS_SELF(ir_word))"

    def call_input_role(self, p: PointerOffset) -> str | None:
        if self.layout.call_kind is None:
            return None
        relative = (p - self.layout.base).as_int()
        if relative == 1 and self.layout.call_kind != "CALL_EX":
            return "CALL_SELF_PRESENT"
        if relative == 3 and self.layout.call_kind == "CALL_EX":
            return "CALL_KWARGS_PRESENT"
        return None

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
        for names in self.null_paths.values():
            names.discard(var.name)
        lay = self.layout
        if var.used and var.name != "unused":
            p = memory_offset.as_int()
            if not var.is_array() and p in lay.in_fields:
                field = FIELDS.index(lay.in_fields[p])
                self.read_fields[field] = self.read_fields.get(field, True) and var.nullable
                self.loaded_value = self.home_action("_PyIR_OWNER_STORAGE_SAVE", field,
                                                     flags=int(var.nullable))
                if self.loaded_value is not None:
                    self.home_values[field] = self.loaded_value
            else:
                self.unknown_read = True
        if lay.call_kind is not None:
            if var.is_array():
                # The arguments are operands: a handler reads them with
                # IR_OPERAND(k) or has a helper read the words
                return "NULL"
            if (memory_offset - lay.base).as_int() == 1 and lay.call_kind == "CALL_EX":
                return "PyValue_NULL"
        if not var.is_array():
            role = self.call_input_role(memory_offset)
            if role is not None:
                return f"((call_state & {role}) ? {self.window_slot(memory_offset)} : PyValue_NULL)"
        if not var.is_array():
            p = memory_offset.as_int()
            if (p is not None and self.owner.get(p) == "in"
                    and p in lay.borrowable):
                return f"IR_VIEW({self.input_field(p)})"
        return super().load(var, memory_offset)

    def array_output(self, var: StackItem, memory_offset: PointerOffset) -> str:
        address = super().array_output(var, memory_offset)
        if self.layout.store_dst and not self.layout.window:
            # A scalar result can be discarded or folded into a local when
            # its conditional trailing array is empty. No array home exists
            # then, so even forming its address would be invalid.
            return f"(({var.size}) ? {address} : NULL)"
        return address

    def pop(self, var: StackItem, out: CWriter) -> Local:
        before = self.variables[-1] if self.variables else None
        old_name = before.name if before is not None else None
        fresh = before is None or not before.in_local
        self.loaded_value = None
        local = super().pop(var, out)
        if self.loaded_value is not None:
            local.ir_value = self.loaded_value
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
        self.home_writes += 1
        f = self.input_field(p)
        if p not in self.layout.borrowable:
            role = self.call_input_role(PointerOffset(p, (), ()))
            if role is not None:
                out.emit(f"if (call_state & {role}) {{\n")
            out.emit(f"_PyFrame_WriteHome(frame, &IR_SLOT({f}), PyValue_NULL);\n")
            if role is not None:
                out.emit("}\n")
            self.take_field(self.layout.in_fields[p])
            return
        names = self.views.get(p)
        if not names:
            raise StackError(f"no variable holds the view of the input "
                             f"in {f} when the handler takes it")
        field = self.layout.in_fields[p]
        consume = ("PyValue_NULL" if field in self.layout.local_fields else
                   f"IR_DETACH({FIELD_ARGS[field]}, {names[-1]})")
        out.emit(f"{owner_name(field)} = {consume};\n")
        self.take_field(field)

    def store(self, out: CWriter, var: Local, memory_offset: PointerOffset
              ) -> bool:
        """Write var, which the stack holds at memory_offset; whether the
        frame then holds it there"""
        self.home_writes += 1
        out.start_line()
        p = memory_offset.as_int()
        if self.reloaded:
            self.home_node = None
            # A value that the instruction hands to the frame it switched
            # to: the slot that frame named when it called or resumed
            self.refuse_inplace(var, memory_offset)
            if memory_offset.as_int() != 0:
                raise StackError(f"'{var.name}' is written after the frame changed")
            out.emit(f"IR_STORE_RETURN({var.name});\n")
            return True
        lay = self.layout
        if lay.window and lay.call_kind is not None:
            # The values of a call's positions are views of its operands,
            # which stay in their homes: only the result is stored.  It is
            # published with the call's final store, which releases the
            # temporary operands; on an error edge a value that a uop made
            # is closed.
            self.home_node = None
            if self.final:
                expression = f"ir_old = call_publish(frame, this_instr, ir_word, {var.name}, (int)IR_C(ir_word))"
                self.dst_stores.append(expression)
                out.emit(f"{expression};\n")
            elif self.error_exit and var.name in lay.call_results:
                if self.publish_on_error:
                    # The periodic check follows the store, so the result
                    # stays in its home when the check raises
                    out.emit(f"ir_old = call_publish_cold(frame, this_instr, ir_word, {var.name}, (int)IR_C(ir_word));\n")
                    out.emit("PyValue_XCLOSE(ir_old);\n")
                else:
                    out.emit(f"PyValue_XCLOSE({var.name});\n")
            elif not self.error_exit:
                # A spill at a uop boundary: the result stays in its C
                # variable until the final store
                self.home_writes -= 1
                return False
            return True
        if lay.window:
            self.home_node = None
            role = self.call_input_role(memory_offset)
            if role is not None:
                out.emit(f"if (call_state & {role}) {{\n")
            self.own_inplace(out, var)
            slot = self.window_slot(memory_offset)
            if (self.final and role is None and lay.move_dst is not None
                    and slot == f"window[{lay.move_dst}]"):
                # The result goes to the local that dst names, or to its
                # slot, without the slot's store and reload; the slot is
                # above the inputs, so it holds nothing to replace
                expression = (f"IR_MOVE_RESULT_TO_DST({lay.move_dst_args}, "
                              f"{slot}, {var.name})")
                self.dst_stores.append(expression)
                self.moved_dst = True
                out.emit(f"{expression};\n")
                return True
            out.emit(f"IR_SET_TEMP({slot}, {var.name});\n")
            if role is not None:
                out.emit("}\n")
            return True
        if p is not None and var.ir_input == p and self.owner.get(p) == "in":
            # The input itself, back in its slot, changed: a NULL before the
            # handler closes it.  A borrowable input's slot keeps the view's
            # value: a new value there is an output of its own name.
            f = self.input_field(p)
            if p in lay.borrowable:
                raise StackError(f"'{var.name}' would be stored back to its "
                                 f"input's slot, which may name a local")
            out.emit(f"IR_SET_TEMP(IR_SLOT({f}), {var.name});\n")
            self.publish_field(lay.in_fields[p], var)
            return True
        if p is not None and (lay.dst_offset is None or p < lay.dst_offset):
            if p in lay.in_fields and p not in lay.borrowable:
                # An input that the instruction passes through, changed
                self.own_inplace(out, var)
                out.emit(f"IR_SET_TEMP(IR_SLOT({self.input_field(p)}), {var.name});\n")
                self.publish_field(lay.in_fields[p], var)
                return True
            return self.keep(var, p)
        if lay.store_dst and p != lay.dst_offset:
            # dst may name a local, so the slots after it are not ours
            return self.keep(var, p)
        self.own_inplace(out, var)
        at_dst = lay.store_dst and p == lay.dst_offset
        if at_dst and not self.final:
            # Rules 6 and 7: the result is stored once, with the handler's
            # last store, before any close; until then its C variable owns it.
            if self.error_exit:
                # The error path closes what C owns (rule 9)
                out.emit(f"PyValue_XCLOSE({var.name});\n")
                return True
            # No store happens here: the final one counts
            self.home_writes -= 1
            return self.keep(var, p)
        previous = self.owner.get(p) if p is not None else "out"
        if previous == "in":
            self.consume(out, p)
            self.owner[p] = "out"
            previous = None
        if self.final and p == lay.dst_offset:
            self.null_stores = {edge: not self.null_uncovered and var.name in names
                                for edge, names in self.null_paths.items()}
        slot = self.out_slot(memory_offset)
        if at_dst:
            if lay.dst in lay.temp_fields:
                out.emit(f"IR_STORE_TEMP({slot}, {var.name});\n")
            else:
                expression = f"IR_STORE_DST({FIELD_ARGS[lay.dst]}, {var.name})"
                self.dst_stores.append(expression)
                out.emit(f"{expression};\n")
            self.publish_field(lay.dst, var)
            return True
        macro = "IR_SET_TEMP" if previous == "out" else "IR_STORE_TEMP"
        out.emit(f"{macro}({slot}, {var.name});\n")
        if p == lay.dst_offset and lay.dst is not None:
            self.publish_field(lay.dst, var)
        else:
            self.home_node = None
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

    def save_variables(self, out: CWriter,
                       native_result: PointerOffset | None = None) -> None:
        out.start_line()
        var_offset = self.base_offset
        for var in self.variables:
            if (var_offset != native_result and var.in_local and
                    not var.memory_offset and not var.is_array()):
                if self.store(out, var, var_offset):
                    var.memory_offset = var_offset
            var_offset = var_offset.push(var.item)

    def vacate(self, out: CWriter, low: PointerOffset, high: PointerOffset) -> None:
        "Clear the positions from low up to high, which the stack gave up"
        out.start_line()
        lay = self.layout
        count = high - low
        n = count.as_int()
        if n != 0:
            self.home_writes += 1
            if not lay.window and low.as_int() is not None and n is not None:
                # The bounded sweep is itself a checked transport. Each
                # actual consume/store below also supplies its own receipt.
                self.home_write_receipts += 1
            else:
                self.home_node = None
        if lay.window:
            if lay.call_kind is not None:
                # A call's positions are views of its operands, which its
                # result's store releases (or a pushed frame took)
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
                    self.home_writes += 1
                    out.emit(f"{self.out_slot(PointerOffset(p, (), ()))} = PyValue_NULL;\n")
                    if p == lay.dst_offset and lay.dst is not None:
                        self.vacate_field(lay.dst)
                    else:
                        self.home_node = None
            return
        # Outputs whose count is not constant, above the inputs
        if any(p >= (lo if lo is not None else low.numeric) for p in self.owner
               if self.owner[p] == "in"):
            raise StackError(f"cannot clear inputs from {low} to {high}")
        for p in [p for p in self.owner if lo is not None and p >= lo]:
            del self.owner[p]
        out.emit(f"IR_CLEAR_SLOTS(&{self.out_slot(low)}, {count.to_c()});\n")
        self.home_node = None

    def flush(self, out: CWriter,
              native_result: PointerOffset | None = None) -> None:
        if self.physical_sp is None:
            # After a frame switch there is nothing of this frame to write.
            if self.variables:
                raise StackError("flush while the frame is switching")
            return
        self._print(out)
        if may_be_positive(self.physical_sp - self.logical_sp):
            self.vacate(out, self.logical_sp, self.physical_sp)
        self.save_variables(out, native_result)
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
        if self.shape_read_binding != other.shape_read_binding:
            self.shape_read_binding = other.shape_read_binding = None
        self.shape_read_complete = other.shape_read_complete = (
            self.shape_read_complete and other.shape_read_complete)
        if (self.home_writes != self.home_write_receipts or
                other.home_writes != other.home_write_receipts):
            self.home_node = other.home_node = None
        super().merge(other, out)
        for name in self.native_values.keys() | other.native_values.keys():
            if name not in self.native_values or name not in other.native_values:
                self.native_values.pop(name, None)
                other.native_values.pop(name, None)
                continue
            mine, theirs = self.native_values[name], other.native_values[name]
            mine.nonnull = theirs.nonnull = mine.nonnull and theirs.nonnull
            if mine.ir_value != theirs.ir_value:
                mine.ir_value = theirs.ir_value = None
            mine.owns_value = theirs.owns_value = mine.owns_value and theirs.owns_value
        if self.home_graph is not other.home_graph:
            raise StackError("scalar storage branches have different graphs")
        self.home_node = other.home_node = self.home_graph.join(self.home_node, other.home_node)
        self.native_owners &= other.native_owners
        other.native_owners = set(self.native_owners)
        self.possible_native_owners |= other.possible_native_owners
        other.possible_native_owners = set(self.possible_native_owners)
        self.expired_values |= other.expired_values
        other.expired_values = set(self.expired_values)
        self.home_values = {field: value for field, value in self.home_values.items()
                            if other.home_values.get(field) == value}
        other.home_values = dict(self.home_values)
        for attribute in ("element_guards", "element_indices"):
            common = {key: value for key, value in getattr(self, attribute).items()
                      if getattr(other, attribute).get(key) == value}
            setattr(self, attribute, dict(common))
            setattr(other, attribute, common)
        self.home_writes = other.home_writes = max(self.home_writes, other.home_writes)
        self.home_write_receipts = other.home_write_receipts = max(
            self.home_write_receipts, other.home_write_receipts)
        reads = {field: self.read_fields.get(field, True) and other.read_fields.get(field, True)
                 for field in self.read_fields.keys() | other.read_fields.keys()}
        self.read_fields = dict(reads)
        other.read_fields = reads
        self.unknown_read = other.unknown_read = self.unknown_read or other.unknown_read
        paths = {}
        for edge in self.null_paths.keys() | other.null_paths.keys():
            if edge in self.null_paths and edge in other.null_paths:
                paths[edge] = self.null_paths[edge] & other.null_paths[edge]
            else:
                paths[edge] = set((self.null_paths if edge in self.null_paths
                                   else other.null_paths)[edge])
        self.null_uncovered |= other.null_uncovered
        other.null_uncovered = self.null_uncovered
        self.null_paths = {edge: set(names) for edge, names in paths.items()}
        other.null_paths = paths
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
        # A close names a variable that the handler starts as NULL, so the
        # join closes what either path detached
        self.deferred += [close for close in other.deferred
                          if close not in self.deferred]
        other.deferred = list(self.deferred)
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

    def stack_bound_check(self, out: CWriter) -> None:
        pass


class IREmitter(Emitter):
    "Emits handler bodies for the register IR"

    instruction_macros = IR_INSTRUCTION_MACROS
    def __init__(self, out: CWriter, analysis: Analysis,
                 formats: dict[str, IRFormat]):
        super().__init__(out, analysis.labels)
        self.analysis = analysis
        self.formats = formats
        # Global labels use the same emitter without entering emit_handler.
        # They have no certifying source contract or per-handler receipts.
        self.null_outputs: frozenset[str] | None = None
        self.error_source = None
        self.error_uop_name = None
        self.callback_current_identity = False
        self.error_unwind_sources = set()
        self.error_terminal_sources = set()
        self.error_uncovered = True
        self.error_finalized = False
        self.error_local_dst = False
        self.generator_boundary = None
        # The exits that a generator-boundary variant's body reaches, and
        # the completions it lowers (IR_COMPLETE_STEP's token and kind)
        self.boundary_reached: set[str] = set()
        self.boundary_completions: list[tuple[Token, str]] = []
        self.send_source_parts = None
        self.error_redispatch_origin = None
        self.error_checkpoint_sites = {}
        self.error_checkpoint_count = 0
        self.allowed_error_checkpoints: frozenset[int] | None = frozenset()
        self.observer_source = None
        self.observer_errors = set()
        self.observer_terminals = set()
        self.observer_calls = set()
        # Emitting the one body that every generic BINARY_OP operation shares
        self.shared_binary_op = False
        # The uops of the handler being printed whose entry sets the stub's
        # ESCAPED bit (stub_escapes()), or None if it has no stub
        self.stub_escapes: frozenset[str] | None = None
        # The index of the uop before whose body the handler being printed
        # pins its departing frame's code (pin_point()), or None, and whether
        # it has printed the pin
        self.pin_at: int | None = None
        self.pinned = False
        # The record of the handler being printed, and the identities of its
        # elements printed so far (check_record_consumed())
        self.record: OperationRecord | None = None
        self.record_consumed: set[int] = set()
        self.record_sites: set[int] = set()
        self.uop_index = 0
        self.current_uop: UopRecord | None = None
        self._replacers["INSTRUCTION_SIZE"] = self.ir_instruction_size
        self._replacers["DISPATCH_SAME_OPARG"] = self.dispatch_same_oparg
        # DISPATCH_REDIRECT is DISPATCH() at the word the callback chose
        self._replacers["DISPATCH_REDIRECT"] = self.dispatch
        self._replacers["IR_DST"] = self.ir_dst
        self._replacers["IR_DST_SLOT"] = self.ir_dst_slot
        self._replacers["IR_BASE"] = self.ir_base
        self._replacers["IR_COMPLETE_STEP"] = self.complete_step
        self._replacers["IR_PROCESS_ERROR"] = self.process_error
        for marker in committed_markers():
            self._replacers[marker] = self.observe_committed_branch
        self._replacers["IR_DEFER_CLOSE"] = self.defer_observer_input
        self._replacers["IR_RETURN_TO_CALLER"] = self.return_to_caller
        self._replacers["IR_FINISH_GENERATOR_RETURN"] = self.finish_generator_return
        self._replacers["IR_FINISH_GENERATOR_RETURN_EMPTY"] = self.finish_generator_return
        self._replacers["IR_ENTER_GENERATOR"] = self.enter_generator
        self._replacers["IR_TRY_DICT_FIRST_PROBE"] = self.try_dict_first_probe
        self._replacers["IR_RETIRE_PUBLISHED"] = self.retire_published
        self._replacers["IR_RETIRE_PUBLISHED_RESULT"] = self.retire_published
        self._replacers["IR_RETIRE_PUBLISHED_PAIR"] = self.retire_published
        self._replacers["IR_GUARD_SHAPE_READ"] = self.guard_shape_read
        self._replacers["IR_DUP_SHAPE_READ"] = self.shape_read_value
        self._replacers["IR_FINISH_VALUE_OPERATION"] = self.finish_value_operation
        self._replacers["IR_RESTORE_VALUE_INPUTS"] = self.restore_value_inputs
        self._replacers["IR_COMPLETE_VALUE_READER"] = self.complete_value_reader
        self._replacers["IR_ABORT_VALUE_READER"] = self.abort_value_reader
        self._replacers["IR_COMPLETE_ATTR_READER"] = self.complete_attr_reader
        self._replacers["IR_BINARY_READER"] = self.binary_reader
        self._replacers["IR_GENERATOR_MARKER"] = self.generator_marker
        self._replacers["IR_METADATA_PASSTHROUGH"] = self.metadata_passthrough
        self._replacers["IR_SUSPEND_DELEGATED"] = self.suspend_delegated
        self._replacers["IR_WRITE_OUTPUT"] = self.write_output_home
        self._replacers["IR_UNPACK_ITEMS"] = self.unpack_items_home
        self._replacers["IR_TAKE_INPUT"] = self.take_input_home
        self._replacers["IR_CLOSE_INPUTS"] = self.close_input_homes
        self._replacers["IR_BUILD_STRING"] = self.home_helper
        self._replacers["IR_BUILD_MAP"] = self.home_helper
        self._replacers["IR_BUILD_SET"] = self.home_helper
        self._replacers["IR_BUILD_TUPLE"] = self.home_helper
        self._replacers["IR_BUILD_LIST"] = self.home_helper
        for marker in ELEMENT_INTRINSICS:
            self._replacers[marker] = self.element_operation
        self._replacers["IR_PREPARE_CALL_METHOD"] = self.prepare_call
        self._replacers["IR_PREPARE_CALL_TUPLE"] = self.prepare_call
        self._replacers["IR_MONITOR_CALL"] = self.prepare_call
        self._replacers["IR_INVOKE_CALL"] = self.invoke_call
        self._replacers["IR_INVOKE_GUARDED_PYTHON_CALL"] = self.invoke_call
        self._replacers["IR_BEGIN_CALL_EFFECTS"] = self.call_effects
        self._replacers["IR_REFRESH_CALL"] = self.refresh_call
        self._replacers["TAKE"] = self.take_view
        self._replacers["OWNER"] = self.take_view
        # Set while a statement is emitted
        self.native_scopes: list[dict[str, Local | None]] = []
        self.native_condition_depth = 0
        self.statement: SimpleStmt | None = None
        self.element_result = None
        # The input, and its field, that the statement's escaping release
        # killed, if it is a view
        self.killed_inplace: tuple[str, str] | None = None
        # The variable that `PyValue NAME = OWNER(x);` assigned, which only
        # IR_RETIRE_PUBLISHED may read, with no call that runs code before
        self.pending_owner: Token | None = None
        # The ir_close<N> variables that defer_close made: the first pass
        # of generate_handler finds them and the second declares them
        self.close_count = 0
        self.close_names: set[str] = set()
        self.declared_closes: set[str] = set()
        # The outputs that a statement `out = TAKE(in);` assigned in the
        # handler (check_error_outputs())
        self.taken_outputs: set[str] = set()
        # The C type of each variable of the handler, from the first pass
        self.declared_roles: dict[str, set[str]] = {}
        self.stores_outputs = False  # the statement's uop is the last
        self.call_finished = False
        self.call_has_feedback = False
        # The handler's call_run_*() and its constant arguments
        self.call_run = ""
        self.call_argc = ""
        self.call_kind = ""
        self.call_flags = ""
        # The format of the handler being emitted
        self.handler_format: IRFormat | None = None
        self.execution_variant: ExecutionVariant | None = None
        self.result_publication: "ResultPublicationContract | None" = None
        self.ownership_events: list[OwnershipEvent] = []
        self.disjoint_child_publication: Uop | None = None
        self.disjoint_child_drained = False

    def ownership_event(self, kind, *, flags=0, dst=0, src=0, extent=0):
        self.ownership_events.append(OwnershipEvent(
            "_PyIR_OWNER_" + kind, flags, dst, src, extent))

    def save_stack(self, tkn, tkn_iter, uop, storage, inst):
        result = super().save_stack(tkn, tkn_iter, uop, storage, inst)
        if uop is self.disjoint_child_publication:
            stack = storage.stack
            assert isinstance(stack, IRStack)
            if any(stack.owner.get(p) == "in" for p in stack.layout.in_fields):
                raise analysis_error("disjoint child publication retains an input home", tkn)
            self.disjoint_child_drained = True
        return result

    def checked_home_array(self, tkn, uop, storage):
        operation = home_operation_contract(uop).get(id(tkn))
        if operation is None:
            raise analysis_error("missing checked home operation", tkn)
        name, index = operation
        local = find_local(name, storage)
        stack = storage.stack
        if (local is None or not local.is_array() or not isinstance(stack, IRStack)
                or stack.physical_sp is None or stack.reloaded):
            raise analysis_error("home adapter requires active frame storage", tkn)
        # Storage.for_uop alone binds this pointer from the physical layout.
        # The source contract rejects every rebinding or pointer escape.
        return local, index

    def write_output_home(self, tkn, tkn_iter, uop, storage, inst):
        local, index = self.checked_home_array(tkn, uop, storage)
        args, end = self.instruction_macro_args(tkn, tkn_iter)
        self.out.start_line()
        write = (f"_PyFrame_WriteHome(frame, &{local.name}[{index}], "
                 f"{tokens_text(args[2])})")
        if inst is not None and unpacks_elements_directly(inst, self.format_of(inst)):
            # The element goes straight to its local; the local's old value
            # takes the element's slot of the window, for the close after
            # the last store (see unpack_to_locals())
            fmt = self.format_of(inst)
            dst = fmt.field_of(ROLE_DST)
            write = (f"if ({is_local(dst)}) {{ "
                     f"IR_UNPACK_DIRECT(&IR_SLOT({FIELD_VARS[dst]}), "
                     f"&{local.name}[{index}], {index}, {UNPACKS[fmt.group]}, "
                     f"{tokens_text(args[2])}); }} else {{ {write}; }}")
            self.out.emit_at(write, tkn)
            self.out.last_token = end
            return True
        self.out.emit_at(write, tkn)
        self.out.last_token = end
        return True

    def unpack_items_home(self, tkn, tkn_iter, uop, storage, inst):
        """All the elements of an unpack, written by one leaf function: to
        the locals that dst names, whose old values that own an object go to
        the window for the close (see unpack_to_locals()), or else to the
        window's homes."""
        local, _ = self.checked_home_array(tkn, uop, storage)
        args, end = self.instruction_macro_args(tkn, tkn_iter)
        items, count = tokens_text(args[1]), tokens_text(args[2])
        write = f"_PyIR_UnpackItemsToWindow(frame, {local.name}, {items}, {count})"
        fmt = self.format_of(inst)
        if inst is not None and unpacks_elements_directly(inst, fmt):
            dst = fmt.field_of(ROLE_DST)
            write = (f"if ({is_local(dst)}) {{ "
                     f"ir_unpack_owned = _PyIR_UnpackItemsToLocals("
                     f"&IR_SLOT({FIELD_VARS[dst]}), {local.name}, {items}, {count}); }} "
                     f"else {{ {write}; }}")
        else:
            write += ";"
        self.out.start_line()
        self.out.emit_at(write, tkn)
        self.out.last_token = end
        return True

    def home_helper(self, tkn, tkn_iter, uop, storage, inst):
        local, _ = self.checked_home_array(tkn, uop, storage)
        args, end = self.instruction_macro_args(tkn, tkn_iter)
        if tkn.text in {"IR_BUILD_TUPLE", "IR_BUILD_LIST"}:
            if (inst is None or inst.parts != [uop]
                    or not steal_success_home_contract(uop)):
                raise analysis_error("steal-success helper needs an unboxed terminal handoff", tkn)
            helper = IR_HELPER_CALLEES[tkn.text]
            expression = f"{helper}({local.name}, oparg)"
        else:
            helper = {"IR_BUILD_STRING": "_Py_BuildString_ValueSteal",
                      "IR_BUILD_MAP": "_Py_BuildMap_ValueSteal",
                      "IR_BUILD_SET": "_Py_BuildSet_ValueSteal"}[tkn.text]
            expression = f"{helper}(frame, {local.name}, oparg)"
        self.out.emit_at(expression, tkn)
        self.out.last_token = end
        return True

    def take_input_home(self, tkn, tkn_iter, uop, storage, inst):
        local, index = self.checked_home_array(tkn, uop, storage)
        _, end = self.instruction_macro_args(tkn, tkn_iter)
        self.out.emit_at(f"_PyFrame_TakeHome(frame, &{local.name}[{index}])", tkn)
        self.out.last_token = end
        return True

    def close_input_homes(self, tkn, tkn_iter, uop, storage, inst):
        local, _ = self.checked_home_array(tkn, uop, storage)
        _, end = self.instruction_macro_args(tkn, tkn_iter)
        self.out.emit_at(f"_PyFrame_CloseHomeRange(frame, {local.name}, oparg)", tkn)
        self.out.last_token = end
        local.kill()
        return True

    def generator_marker(self, tkn, tkn_iter, uop, storage, inst):
        assert inst is not None
        args = self.macro_args(tkn, tkn_iter, 0)
        if args or not generator_marker_contract(inst, self.format_of(inst)):
            raise analysis_error("IR_GENERATOR_MARKER needs its checked contract", tkn)
        self.ownership_event("GENERATOR_MARKER")
        self.out.start_line()
        self.emit("Py_UNREACHABLE();\n")
        return True

    def metadata_passthrough(self, tkn, tkn_iter, uop, storage, inst):
        args, _ = self.instruction_macro_args(tkn, tkn_iter)
        assert next(tkn_iter).kind == "SEMI"
        if (len(args) != 1 or [token.text for token in args[0]] != ["1"]
                or inst is None or metadata_only_passthroughs(inst) != frozenset({1})):
            raise analysis_error("uncertified metadata-only passthrough", tkn)
        return True

    def suspend_delegated(self, tkn, tkn_iter, uop, storage, inst):
        value, = self.macro_args(tkn, tkn_iter, 1)
        if inst is None or metadata_only_passthroughs(inst) != frozenset({1}):
            raise analysis_error("delegated suspension requires its checked transport", tkn)
        self.ownership_event("YIELD_DELEGATED")
        dst = FIELD_VARS[self.format_of(inst).field_of(ROLE_DST)]
        self.emit("assert(frame->owner == FRAME_OWNED_BY_GENERATOR);\n")
        self.emit("frame->instr_ptr = next_instr;\n")
        self.emit("PyGenObject *ir_yield_gen = _PyGen_GetGeneratorFromFrame(frame);\n")
        self.emit(f"assert(_PyValue_IsHome({value}));\n")
        self.emit("DTRACE_FUNCTION_RETURN();\n")
        self.emit("tstate->exc_info = ir_yield_gen->gi_exc_state.previous_item;\n")
        self.emit("ir_yield_gen->gi_exc_state.previous_item = NULL;\n")
        self.emit("_Py_LeaveRecursiveCallPy(tstate);\n")
        self.emit("_PyInterpreterFrame *ir_yield_frame = frame;\n")
        self.emit(f"ir_yield_frame->return_slot = (uint16_t){dst};\n")
        self.emit("_PyThreadState_UpdateLastProfiledFrame(tstate, frame, frame->previous);\n")
        self.emit("frame = tstate->current_frame = frame->previous;\n")
        self.emit("ir_yield_frame->previous = NULL;\n")
        self.emit("((_PyThreadStateImpl *)tstate)->generator_return_kind = GENERATOR_YIELD;\n")
        self.emit("FT_ATOMIC_STORE_INT8_RELEASE(ir_yield_gen->gi_frame_state, FRAME_SUSPENDED_YIELD_FROM);\n")
        self.abrupt_prefix = True
        self.emit(f"IR_TRY_NATIVE_YIELD({value});\n")
        self.emit("next_instr = frame->return_pc;\n")
        self.emit(f"IR_STORE_RETURN({value});\n")
        self.emit("LLTRACE_RESUME_FRAME();\n")
        self.emit("DISPATCH();\n")
        return False

    def binary_reader(self, tkn, tkn_iter, uop, storage, inst):
        assert inst is not None
        left, right, result = self.macro_args(tkn, tkn_iter, 3)
        if (not binary_reader_contract(inst, self.format_of(inst))
                or [item.name for item in uop.stack.inputs] != [left, right]
                or [item.name for item in uop.stack.outputs] != [result]):
            raise analysis_error("IR_BINARY_READER needs its checked binary IO contract", tkn)
        # Emit the owner transfer and its semantic event together. The native
        # owners remain C locals under the selected native-root policy.
        for index, (name, native) in enumerate((
                (left, "ir_binary_left"), (right, "ir_binary_right"))):
            self.ownership_event("READ", src=index)
            token = replace(tkn, text=name)
            field = self.view_field(token, storage)
            value = name
            if field is not None:
                self.spend_owner(field, tkn, storage)
                self.ensure_consumed(field, storage)
                value = self.taken_view(name, field, storage)
                storage.stack.taken(storage.stack.field_position(field))
            self.emit(f"PyValue {native} = {value};\n")
            self.value_kill(token, storage, False)
            self.ownership_event("TAKE", dst=index, src=index)
        storage.clear_inputs("before binary provider")
        storage.flush(self.out)
        self.emit("PyValue ir_binary_result = _PyValue_BinaryOps[oparg](PyValue_Borrow(ir_binary_left),\n")
        self.emit("PyValue_Borrow(ir_binary_right));\n")
        # The error branch has its own snapshot; it cannot leak into normal
        # successor facts. Restore publishes only after its revalidation.
        error_branch = len(self.ownership_events)
        self.ownership_event("ERROR_BRANCH")
        self.emit("if (PyValue_IsNull(ir_binary_result)) {\n")
        self.emit("_PyValuePair ir_binary_rest = value_operation_restore2(tstate, frame, "
                  f"ir_binary_left, ir_binary_right, {self.input_slot(inst, 0)}, "
                  f"{self.input_slot(inst, 1)});\n")
        self.emit("value_operation_close3(ir_binary_rest.second, ir_binary_rest.first, PyValue_NULL);\n")
        self.emit(f"(void)value_operation_finish(tstate, frame, this_instr, NULL);\n")
        self.emit("PROCESS_ERROR_AT(RAISE, 0, next_instr - 1);\n")
        self.emit("}\n")
        self.emit(f"ir_binary_result = value_reader_complete(tstate, frame, this_instr, "
                  f"NULL, {FIELD_VARS[self.format_of(inst).field_of(ROLE_DST)]}, "
                  "ir_binary_result, ir_binary_right, ir_binary_left);\n")
        self.emit("if (PyValue_IsNull(ir_binary_result)) {\n")
        self.emit("PROCESS_ERROR_AT(RAISE, 0, next_instr - 1);\n")
        self.emit("}\n")
        self.reader_reload()
        self.emit(f"{result} = ir_binary_result;\n")
        self.ownership_event("FRESH")
        self.ownership_event("NORMAL_EXIT")
        self.ownership_events[error_branch] = replace(
            self.ownership_events[error_branch], dst=len(self.ownership_events))
        self.ownership_event("RESTORE", src=0)
        self.ownership_event("RESTORE", src=1)
        self.ownership_event("ERROR_EXIT")
        for output in storage.outputs:
            if output.name == result:
                output.in_local = True
                output.memory_offset = None
                output.ir_inplace = None
        stack = storage.stack
        assert isinstance(stack, IRStack)
        stack.bind(result, None)
        stack.code_ran()
        return True

    def uop_record(self, uop: Uop) -> UopRecord:
        "The record of uop, the next uop of the handler being printed"
        assert self.record is not None
        if (self.uop_index >= len(self.record.uops)
                or self.record.uops[self.uop_index].name != uop.name):
            raise analysis_error(f"{self.record.handler}: {uop.name} is not the "
                                 f"next uop of its record", self.record.instruction.where)
        self.current_uop = self.record.uops[self.uop_index]
        self.uop_index += 1
        return self.current_uop

    def note_site(self, tkn: Token) -> None:
        """Mark a guard or transfer site of the body as printed.  A site that
        the record lacks is a generation error: the record and the C would
        differ"""
        if self.record is None:
            return
        if not (tkn.kind in ("GOTO", "RETURN") or
                tkn.kind == "IDENTIFIER" and tkn.text in SITE_TOKENS):
            return
        if id(tkn) not in self.record_sites:
            raise analysis_error(f"{self.record.handler}: '{tkn.text}' is not in "
                                 "the handler's record", tkn)
        self.record_consumed.add(id(tkn))

    def check_record_consumed(self) -> None:
        """Every element of the record that the body prints was printed: its
        guard and transfer sites, its implicit object guards and its cache
        operands"""
        record = self.record
        assert record is not None
        missing = []
        for uop in record.uops:
            for guard in uop.guards:
                key = id(guard.site) if guard.site is not None else id(guard)
                if key not in self.record_consumed:
                    missing.append(f"{uop.name}: guard "
                                   f"{guard.site.text if guard.site else 'object ' + guard.input}")
            for transfer in uop.transfers:
                if transfer.site is not None and id(transfer.site) not in self.record_consumed:
                    missing.append(f"{uop.name}: {transfer.kind} at {transfer.site.text}")
            for operand in uop.feedback:
                if id(operand) not in self.record_consumed:
                    missing.append(f"{uop.name}: feedback {operand.name}")
        if missing:
            raise analysis_error(f"{record.handler}: the C omits elements of its "
                                 f"record: {'; '.join(missing)}",
                                 record.instruction.where)

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
        self.pending_owner = None
        stack = storage.stack
        assert isinstance(stack, IRStack)
        # Rule 7: the owners close after the handler's last store
        for retired in (first, second):
            if retired is not None:
                self.defer_close(stack, "PyValue_XCLOSE", retired.text)
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

    def guard_shape_read(self, tkn, tkn_iter, uop, storage, inst):
        args, _ = instruction_macro_args(tkn, tkn_iter)
        assert next(tkn_iter).kind == "SEMI"
        # Prefix uops share the handler's C scope. They must not replace the
        # private primitives or locals that implement this checked read.
        reserved = {"shape_read_offset", "_PyObject_ShapeReadDup", "ir_shape_offset",
                    "ir_selected_offset"}
        if inst is not None and (
                reserved.intersection(uop_tokens(inst)) or
                any(token.kind.startswith("CMACRO") and
                    any(name in token.text for name in reserved)
                    for token in uop_token_list(inst)) or
                any(item.name in reserved for part in inst.parts
                    if isinstance(part, Uop)
                    for item in (*part.stack.inputs, *part.stack.outputs, *part.caches))):
            raise analysis_error("shape read reserved native name", tkn)
        text = [[token.text for token in arg] for arg in args]
        if (len(text) != 3 or len(text[0]) != 4 or
                text[0][:2] != ["PyValue_AsKnownObjectBorrow", "("] or
                text[0][-1] != ")" or
                any(len(arg) != 1 for arg in text[1:])):
            raise analysis_error("shape guard requires a current object input and cache fields", tkn)
        name = text[0][2]
        version, shape = text[1][0], text[2][0]
        caches = {cache.name: cache.size for cache in uop.caches}
        var = find_local(name, storage)
        if (inst is None or self.format_of(inst).fb_layout != "_PyFbAttr" or
                (version, shape) != ("type_version", "shape") or
                caches != {"type_version": 2, "shape": 0} or
                len(uop.body.body) != 1 or len(storage.inputs) != 1 or
                storage.inputs[0] is not var or var.ir_inplace is None or
                not var.known_object or not uop.is_guard() or
                self.statement.contents[0] is not tkn):
            raise analysis_error("shape guard requires unchanged typed attribute inputs", tkn)
        self.check_exit_inputs(tkn, storage)
        pointer = f"_PyValue_AsKnownObjectBorrowPointer({name})"
        variant = self.execution_variant
        if variant is not None and variant.shape_cases:
            self.emit(f"(void){version}; (void){shape};\n")
            self.emit(f"int64_t ir_selected_offset = shape_read_offset(frame->ir_words, fb, {pointer});\n")
            self.emit("if (ir_selected_offset < 0) {\n")
            self.emit_jump_to_predicted(inst)
            self.emit("}\n")
            self.emit("ir_shape_offset = (uint32_t)ir_selected_offset;\n")
            source = "ir_shape_offset"
        else:
            # The expansion that the leaf translator reads too (ir_leaf.py)
            for item in ir_leaf.capability_expansion(tkn.text, [pointer, version, shape]):
                if item[0] == "stmt":
                    self.emit(item[1] + "\n")
                else:
                    self.emit(f"if ({item[1]}) {{\n")
                    self.emit_jump_to_predicted(inst)
                    self.emit("}\n")
            source = "fb->offset"
        storage.stack.shape_read_binding = (var.ir_inplace, source)
        storage.stack.shape_read_complete = False
        self.shape_read_guards += 1
        return True

    def shape_read_value(self, tkn, tkn_iter, uop, storage, inst):
        args, _ = instruction_macro_args(tkn, tkn_iter)
        if (len(args) != 2 or any(len(arg) != 1 or arg[0].kind != "IDENTIFIER"
                                 for arg in args)):
            raise analysis_error("shape read requires current receiver and bound offset", tkn)
        name, offset = (arg[0].text for arg in args)
        text = _statement_text(self.statement)
        direct = (len(text) == 9 and text[0] in {var.name for var in storage.outputs} and
                  text[1:] == ["=", "IR_DUP_SHAPE_READ", "(", name,
                               ",", offset, ")", ";"])
        var = find_local(name, storage)
        binding = storage.stack.shape_read_binding
        if (not direct or binding is None or var is None or var.ir_inplace != binding[0] or
                not var.known_object or len(storage.inputs) != 1 or
                storage.inputs[0] is not var or
                [(cache.name, cache.size) for cache in uop.caches] != [(offset, 1)]):
            raise analysis_error("shape read has no current guarded offset", tkn)
        # The read and its anchor are one primitive; source cannot substitute
        # a different pointer after acquiring the metadata capability.
        storage.stack.shape_read_binding = None
        storage.stack.shape_read_complete = True
        self.shape_read_uses += 1
        [(_, expression)] = ir_leaf.capability_expansion(tkn.text, [name, offset])
        self.emit(" " + expression)
        return True

    def element_operation(self, tkn, tkn_iter, uop, storage, inst):
        """Checked guards and anchored reads; none accepts a raw pointer."""
        stack = storage.stack
        assert isinstance(stack, IRStack)
        count = {"IR_GUARD_INLINE_INT": 1, "IR_GUARD_ELEMENT_TYPE": 2,
                 "IR_GUARD_ELEMENT_INDEX": 4, "IR_DUP_CANONICAL_ELEMENT": 2}[tkn.text]
        if tkn.text == "IR_DUP_CANONICAL_ELEMENT":
            args, _ = instruction_macro_args(tkn, tkn_iter)
            if len(args) != count or any(len(arg) != 1 or arg[0].kind != "IDENTIFIER" for arg in args):
                raise analysis_error("element read requires a value and checked index", tkn)
            names = [arg[0].text for arg in args]
            assignment = value_assignment(self.statement.contents)
            if (assignment is None or [token.text for token in assignment[2]] !=
                    [tkn.text, "(", names[0], ",", names[1], ")"]):
                raise analysis_error("element duplicate must directly assign one scalar", tkn)
        else:
            names = self.macro_args(tkn, tkn_iter, count)
            if _statement_text(self.statement) != [tkn.text, "(", *sum(
                    ([name, ","] for name in names[:-1]), []), names[-1], ")", ";"]:
                raise analysis_error("element guard must be a standalone statement", tkn)

        def current(name):
            var = next((var for var in self.current_value_locals(storage) if var.name == name), None)
            if (var is None or not var.in_local or var.ir_value is None or
                    var.ir_value in stack.expired_values):
                raise analysis_error("element capability requires a live scalar value", tkn)
            return var.ir_value

        def miss(condition):
            self.check_exit_inputs(tkn, storage)
            if stack.possible_native_owners:
                raise analysis_error("element guard must precede owning acquisitions", tkn)
            self.out.start_line()
            self.emit(f"if ({condition}) {{\n")
            self.emit_jump_to_predicted(inst)
            self.emit("}\n")

        def expand(args, pointer_test=None):
            "Print the expansion that the leaf translator reads too (ir_leaf.py)"
            for item in ir_leaf.capability_expansion(tkn.text, args, pointer_test):
                if item[0] == "guard":
                    miss(item[1])
                elif item[0] == "stmt":
                    self.emit(item[1] + "\n")
                elif item[0] == "expr":
                    self.emit(item[1])
                else:
                    # The indexed list borrow/acquisition interval currently
                    # relies on the GIL. Keep FT on its existing generic path.
                    self.emit("#ifdef Py_GIL_DISABLED\n")
                    self.emit_jump_to_predicted(inst)
                    self.emit("#endif\n")

        owner = current(names[0])
        if tkn.text == "IR_GUARD_INLINE_INT":
            expand(names)
            stack.element_guards[("INT", owner)] = "INT"
        elif tkn.text == "IR_GUARD_ELEMENT_TYPE":
            # The type, which the GUARD row's exact(1) names (ir_contract.py)
            value, tp = names
            kind = {"PyList_Type": "LIST", "PyTuple_Type": "TUPLE"}.get(tp)
            if kind is None:
                raise analysis_error("element type must be PyList_Type or PyTuple_Type", tkn)
            # A slot that is read in place holds an object or an inline value,
            # never NULL: a local that may be unbound is checked before it is
            # read as an operand.  The test reads the slot's top bits.
            var = find_local(value, storage)
            pointer_test = None
            if var.ir_inplace is not None:
                pointer_test = f"PyValue_IsPointerSpaceAt(&IR_SLOT({FIELD_VARS[var.ir_inplace]}))"
            expand(names, pointer_test)
            stack.element_guards[("TYPE", owner)] = kind
            var.known_object = True
        elif tkn.text == "IR_GUARD_ELEMENT_INDEX":
            container, subscript, index, mode = names
            kind = stack.element_guards.get(("TYPE", owner))
            subvalue = current(subscript)
            if (kind is None or ("INT", subvalue) not in stack.element_guards or
                    mode != ("FROM_END" if kind == "LIST" else "NONNEGATIVE")):
                raise analysis_error("element index lacks matching type/integer guards", tkn)
            if (index in HANDLER_LOCALS or index in HANDLER_POINTERS or
                    index in stack.element_indices or
                    index in {var.name for var in self.current_value_locals(storage)}):
                raise analysis_error("checked element index shadows a live identity", tkn)
            expand(names)
            stack.element_indices[index] = (owner, subvalue, kind, id(self.native_scopes[-1]))
        else:
            container, index = names
            capability = stack.element_indices.get(index)
            kind = stack.element_guards.get(("TYPE", owner))
            if (capability is None or capability[0] != owner or capability[2] != kind or
                    capability[3] != id(self.native_scopes[-1])):
                raise analysis_error("element read lacks its current lexical bounds/anchor", tkn)
            expand([container, index, kind])
            self.element_result = HELPERS[tkn.text]
        return True

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
                  f"NULL, {dst}, {result}, {self_value}, {receiver}, {method});\n")
        self.emit(f"if (PyValue_IsNull({result})) {{\n")
        self.reader_error()
        self.emit("}\n")
        self.reader_reload()
        assert isinstance(storage.stack, IRStack)
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
        # A call is several words: its head is this_instr, not next_instr - 1
        self.emit("PROCESS_ERROR_AT(RAISE, 0, this_instr);\n" if self.call_kind
                  else "PROCESS_ERROR_AT(RAISE, 0, next_instr - 1);\n")

    def finish_stores(self, stack: "IRStack") -> None:
        """Rule 7: after the handler's last store, close the inputs that it
        detached and the old value that its releasing store displaced"""
        self.emit_deferred(stack)
        if stack.dst_stores:
            # The old value of a local dst closes here, which runs
            # finalizers: the route census covers a local destination
            self.error_local_dst = True
            self.out.emit("PyValue_XCLOSE(ir_old);\n")

    def defer_observer_input(self, tkn, tkn_iter, uop, storage, inst):
        owner, = self.macro_args(tkn, tkn_iter, 1)
        stack = storage.stack
        assert isinstance(stack, IRStack)
        # Rule 7: the owner closes after the last store, which precedes the
        # observer terminal
        self.defer_close(stack, "PyValue_XCLOSE", owner)
        self.emit(f"{owner} = PyValue_NULL;\n")
        return True

    def observe_committed_branch(self, tkn, tkn_iter, uop, storage, inst):
        assert inst is not None
        args, _ = self.instruction_macro_args(tkn, tkn_iter)
        if len(args) != 4:
            raise analysis_error("terminal observer takes source, destination, event, owner edge", tkn)
        assert next(tkn_iter).kind == "SEMI"
        source, destination, event, _ = (" ".join(t.text for t in arg) for arg in args)
        contracts = observer_terminal_paths(inst, self.format_of(inst))
        if tkn not in contracts:
            raise analysis_error("uncertified terminal observer", tkn)
        self.check_owner_exit(tkn)
        stack = storage.stack
        assert isinstance(stack, IRStack)
        if stack.dst_stores:
            raise analysis_error("terminal observer has a prior destination commit", tkn)
        # Each exclusive path owns its own final commit, rather than adding
        # mutually exclusive stores to the ordinary shared counter.
        stack.dst_stores = list(stack.dst_stores)
        reserved = {"ir_observer_next", "ir_observer_result",
                    "ir_observer_exception", "ir_t"}
        if reserved & set(uop_tokens(inst)):
            raise analysis_error("terminal observer reserved local name", tkn)
        self.emit("{\n")
        storage.push_outputs()
        fmt = self.format_of(inst)
        mode = store_mode(inst, external_io(inst)[1]) if fmt.store_dst else None
        edge, _ = contracts[tkn]
        stack.select_null_edge(edge)
        self.normal_result_offsets.add(emit_final_outputs(inst, fmt, mode, stack, self.out))
        self.normal_null_receipts.append({edge: stack.null_stores.get(edge, False)})
        self.finish_stores(stack)
        self.emit(f"_PyInstr *ir_observer_next = {destination};\n")
        # This checked terminal path has performed its final owner commit.
        # Only its successful intended normal edge gets the normal recipe;
        # callback errors/redirects retain their separate recovery protocols.
        if normal_vacate_capability(inst, fmt):
            self.normal_observer_edges.add(edge)
        # The event runs in a cold function (rule 12), which returns the word
        # to run or the error that the stub's exit describes.
        self.emit(f"IR_OBSERVE_EDGE({source}, ir_observer_next, {event}, {edge});\n")
        self.emit("}\n")
        return False

    def unpack_to_locals(self, inst, fmt, count):
        """The elements are in the window, in stack order; if dst names a
        run of locals they go there, one element at a time as the STORE_FASTs
        they replace would, so that a __del__ sees the same locals.  A
        release that may run Python is out of line."""
        if (not fmt.unpacks or not fmt.window or fmt.n_inputs != 0 or
                record_through(self.record) or
                fmt.field_of(ROLE_BASE) is None or
                fmt.field_of(ROLE_DST) is None):
            raise analysis_error("unsupported local unpack transport", inst.where)
        dst = FIELD_VARS[fmt.field_of(ROLE_DST)]
        self.emit(f"if ({is_local(fmt.field_of(ROLE_DST))}) {{\n")
        if unpacks_elements_directly(inst, fmt):
            # The elements are in their locals, and the window holds the
            # old values that own an object, if any
            self.emit(f"if (ir_unpack_owned) {{ IR_CLOSE_WINDOW(window, {count}); }}\n")
        else:
            self.emit(f"if (IR_UNPACK_TO_LOCALS(&IR_SLOT({dst}), window, {count})) "
                      f"{{ IR_CLOSE_WINDOW(window, {count}); }}\n")
        self.unpack_local_drained = True
        self.emit("}\n")

    def reader_reload(self):
        self.emit("this_instr = frame->instr_ptr;\n")
        self.emit("ir_word = FT_ATOMIC_LOAD_UINT64_RELAXED(*this_instr);\n")

    def try_dict_first_probe(self, tkn, tkn_iter, uop, storage, inst):
        self.macro_args(tkn, tkn_iter, 0)
        self.emit("{\n")
        self.emit("PyValue ir_leaf_result = _PyDict_TryGetFirstProbeValue(dict_st, sub_st);\n")
        self.emit("if (!PyValue_IsNull(ir_leaf_result)) {\n")
        self.emit("PyValue ir_leaf_dict = IR_DETACH(ir_word, B, dict_st);\n")
        self.emit("PyValue ir_leaf_key = IR_DETACH(ir_word, C, sub_st);\n")
        self.emit("STAT_INC(BINARY_OP, hit);\n")
        self.emit("IR_STORE_DST(ir_word, A, ir_leaf_result);\n")
        self.emit("PyValue_XCLOSE(ir_leaf_key);\n")
        self.emit("PyValue_XCLOSE(ir_leaf_dict);\n")
        self.emit("PyValue_XCLOSE(ir_old);\n")
        self.emit("DISPATCH();\n")
        self.emit("}\n")
        self.emit("}\n")
        return True

    def call_reload(self, inst):
        self.reader_reload()
        if self.call_has_feedback:
            self.emit(f"fb = ({feedback_type(self.format_of(inst))} *)"
                      "IR_FB(call_feedback);\n")

    def prepare_call(self, tkn, tkn_iter, uop, storage, inst):
        """The preparations of a call (the method form, the tuple of a
        CALL_FUNCTION_EX, the CALL event) are steps of call_run_*(), which
        runs them in this order on the call's operands when the call is
        invoked: the marker is where the DSL says that they come."""
        assert inst is not None
        self.macro_args(tkn, tkn_iter, 0)
        return True

    def call_effects(self, tkn, tkn_iter, uop, storage, inst):
        self.macro_args(tkn, tkn_iter, 0)
        self.out.start_line()
        self.emit("call_state |= CALL_EFFECTS;\n")
        return True

    def refresh_call(self, tkn, tkn_iter, uop, storage, inst):
        self.macro_args(tkn, tkn_iter, 0)
        self.out.start_line()
        self.emit("if (call_refresh(tstate, frame, this_instr) < 0) {\n")
        self.reader_error()
        self.emit("}\n")
        self.call_reload(inst)
        return True

    def call_completion(self, guarded=False):
        self.call_normal_drained = True
        # One out-of-line call, which keeps the call's locals in its own
        # frame and returns {status, child} in two registers (R-c).  It reads
        # the call's operands from the words after this_instr, in place, and
        # releases or moves them itself.
        self.emit("assert(frame->instr_ptr == this_instr);\n")
        self.emit("{\n")
        if guarded:
            # The cached binding plan fills the child from the operands; a
            # miss had no effect, and the call runs generically
            bind = ("call_bind_positional(tstate, frame, this_instr, "
                    "IR_SLOT(IR_B(ir_word)), PyValue_NULL, &fb->plan)")
            self.emit(f"_PyCallResult call_result = {bind};\n")
            self.emit("if (call_result.status > 0) {\n")
            self.emit(f"call_result = {self.call_run}(tstate, frame, this_instr, "
                      f"{self.call_flags} | call_state);\n")
            self.emit("}\n")
        else:
            self.emit(f"_PyCallResult call_result = {self.call_run}(tstate, frame, "
                      f"this_instr, {self.call_flags} | call_state);\n")
        self.emit("if (call_result.status < 0) {\n")
        self.reader_error()
        self.emit("}\n")
        self.emit("if (call_result.child != NULL) {\n")
        self.emit("DISPATCH_INLINED(call_result.child);\n")
        self.emit("}\n")
        # Both result and callback-current destination are already published.
        self.emit("DISPATCH();\n")
        self.emit("}\n")

    def invoke_call(self, tkn, tkn_iter, uop, storage, inst):
        self.macro_args(tkn, tkn_iter, 0)
        guarded = tkn.text == "IR_INVOKE_GUARDED_PYTHON_CALL"
        if guarded:
            self.emit("assert(fb->func_version >= FUNC_VERSION_FIRST_VALID);\n")
        self.call_completion(guarded)
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
                  f"NULL, {dst}, {result}, {first}, {second});\n")
        self.emit(f"if (PyValue_IsNull({result})) {{\n")
        self.reader_error()
        self.emit("}\n")
        self.reader_reload()
        return True

    def abort_value_reader(self, tkn, tkn_iter, uop, storage, inst):
        self.macro_args(tkn, tkn_iter, 0)
        # Never flush the undefined result on this edge: its destination
        # may be one of the just-restored input temporary homes.
        self.emit(f"(void)value_operation_finish(tstate, frame, this_instr, NULL);\n")
        self.reader_error()
        return False

    def finish_value_operation(self, tkn, tkn_iter, uop, storage, inst):
        assert inst is not None
        self.macro_args(tkn, tkn_iter, 0)
        storage.flush(self.out)
        self.emit("{\n")
        self.emit(f"int operation_failed = value_operation_finish(tstate, frame, this_instr, NULL);\n")
        self.emit("if (operation_failed < 0) {\n")
        self.emit("PROCESS_ERROR_AT(RAISE, 0, next_instr - 1);\n")
        self.emit("}\n")
        self.emit("this_instr = frame->instr_ptr;\n")
        self.emit("ir_word = FT_ATOMIC_LOAD_UINT64_RELAXED(*this_instr);\n")
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

    def defer_close(self, stack: "IRStack", close: str, owned: str,
                    dealloc: str | None = None) -> None:
        """Rule 7: the handler closes OWNED, an owner that it detached or
        loaded, after its last store (emit_deferred).  An owner that is not
        a handler-wide ir_own_<F> is copied to a variable of its own, since
        a later part may assign the name again."""
        if not owned.startswith("ir_own_"):
            self.close_count += 1
            name = f"ir_close{self.close_count}"
            self.close_names.add(name)
            self.out.start_line()
            self.emit(f"{name} = {owned};\n")
            owned = name
        args = owned if dealloc is None else f"{owned}, {dealloc}"
        # The close tests for NULL: the owner is empty on a path that
        # joins this one without detaching it
        close = close.replace("CLOSE", "XCLOSE").replace("XXCLOSE", "XCLOSE")
        stack.deferred.append(f"{close}({args});\n")

    def emit_deferred(self, stack: "IRStack") -> None:
        "The closes that rule 7 puts after the handler's last store"
        self.out.start_line()
        for close in stack.deferred:
            self.out.emit(close)
        stack.deferred = []

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

    def taken_view(self, name: str, field: str, storage: Storage) -> str:
        """The IR_TAKEN of the view name of the input in field, with a count
        of its own for a local.  A required input is not NULL, and a view that
        a guard has shown to be in pointer space needs fewer tests still."""
        stack = storage.stack
        assert isinstance(stack, IRStack)
        macro = "IR_TAKEN"
        if stack.read_fields.get(FIELDS.index(field)) is False:
            var = find_local(name, storage)
            pointer = (var is not None and var.known_object) or name == self.killed_pointer
            macro = "IR_TAKEN_POINTER" if pointer else "IR_TAKEN_REQUIRED"
        return f"{macro}({name}, {owner_name(field)})"

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
        self.defer_close(storage.stack, release, owner_name(field))
        self.out.emit_at("(void)", tkn)
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
        self.defer_close(storage.stack, release, owner_name(field),
                         dealloc.text)
        self.out.emit_at("(void)", tkn)
        self.out.emit(lparen)
        self.out.emit(owner_name(field))
        return self.value_kill(name, storage,
                               dealloc.text not in NON_ESCAPING_DEALLOCS)

    def value_steal(self, tkn, tkn_iter, uop, storage, inst):
        lparen = next(tkn_iter)
        assert lparen.kind == "LPAREN"
        arg = tkn_iter.peek()
        var = find_local(arg.text, storage) if arg is not None and arg.kind == "IDENTIFIER" else None
        field = self.view_field(arg, storage)
        if field is None:
            return super().value_steal(
                tkn, iter_with_first(lparen, tkn_iter), uop, storage, inst)
        name = next(tkn_iter)
        owned = name.text
        if field is not None:
            # The object, with a count of its own for a local
            self.spend_owner(field, tkn, storage)
            self.ensure_consumed(field, storage)
            owned = self.taken_view(name.text, field, storage)
            storage.stack.taken(storage.stack.field_position(field))
        steal = (POINTER_VARIANTS[tkn.text] if self.is_pointer(name, storage)
                 else tkn.text)
        self.out.emit_at(steal, tkn)
        self.out.emit(lparen)
        self.out.emit(owned)
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
            text = self.taken_view(name.text, field, storage)
            storage.stack.taken(storage.stack.field_position(field))
        else:
            self.ensure_consumed(field, storage)
            text = owner_name(field)
        self.out.emit_at(text, tkn)
        self.out.last_token = rparen
        return self.value_kill(name, storage, False)

    def note_pointer_assignment(self, stmt: SimpleStmt, storage: Storage) -> None:
        """After `a = b;` with b a view, a is a view of the same input"""
        super().note_pointer_assignment(stmt, storage)
        tkns = stmt.contents
        if len(tkns) != 4 or tkns[1].kind != "EQUALS" or tkns[-1].kind != "SEMI":
            return
        source = find_local(tkns[2].text, storage)
        if source is None:
            return
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
        `[T *]h = [(T *)]g;` with x a view and g such a variable, h holds the object of a
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
        elif len(rhs) == 1 and rhs[0] in stack.view_objects:
            source = stack.view_objects[rhs[0]]
        elif not escapes and not allocates(tokens[eq + 1:-1]) and (
                eq >= 2 and (texts[eq - 2] == "*" or is_word(tokens[eq - 2])
                             and not is_value_type(texts[eq - 2]))
                or eq == 1 and find_local(name, storage) is None):
            # A declaration of a pointer, or of a type that may be one, or
            # an assignment to a variable of the body: derived if the
            # expression reads the object of a view.  A call of a row that
            # allocates returns a new object, which holds its own count
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

    def emit_IfStmt(self, stmt, uop, storage, inst):
        storage.stack.shape_read_binding = None
        storage.stack.element_guards.clear()
        storage.stack.element_indices.clear()
        locals = self.current_value_locals(storage)
        changed = value_nonnull_changes(stmt.condition, set(), {var.name for var in locals})
        for var in locals:
            if var.name in changed:
                var.ir_value = None
                var.owns_value = False
                var.nonnull = False
        self.check_stale_views(list(stmt.condition), storage, inst)
        self.check_spent_views(list(stmt.condition), storage)
        for tkn in stmt.condition:
            self.note_site(tkn)
        # A condition may assign/address an output through C syntax. Such
        # effects are not part of the checked NULL assignment transport.
        if any(t.kind in ASSIGNMENTS or t.text in {"&", "++", "--"} or
               (self.null_outputs is not None and t.text in self.null_outputs)
               for t in stmt.condition):
            storage.stack.null_uncovered = True
        return super().emit_IfStmt(stmt, uop, storage, inst)

    def refine_current_value(self, storage, name, nonnull):
        locals = self.current_value_locals(storage)
        target = next((var for var in locals if var.name == name), None)
        if target is None:
            return
        previous = target.ir_value
        storage.stack.refine_value(target, nonnull)
        if target.ir_value != previous:
            for var in locals:
                if var.ir_value == previous:
                    var.ir_value = target.ir_value

    def branch_storage(self, storage, condition, truth):
        name = null_value_condition(condition)
        if name is not None:
            self.refine_current_value(storage, name, not truth)
        return storage

    def emit_WhileStmt(self, stmt, uop, storage, inst):
        storage.stack.shape_read_binding = None
        storage.stack.element_guards.clear()
        storage.stack.element_indices.clear()
        for var in self.current_value_locals(storage):
            var.ir_value = None
            var.owns_value = False
        self.check_stale_views(list(stmt.condition), storage, inst)
        self.check_spent_views(list(stmt.condition), storage)
        storage.stack.home_node = None
        return super().emit_WhileStmt(stmt, uop, storage, inst)

    def emit_ForStmt(self, stmt, uop, storage, inst):
        storage.stack.shape_read_binding = None
        storage.stack.element_guards.clear()
        storage.stack.element_indices.clear()
        for var in self.current_value_locals(storage):
            var.ir_value = None
            var.owns_value = False
        storage.stack.home_node = None
        return super().emit_ForStmt(stmt, uop, storage, inst)

    def emit_BlockStmt(self, stmt, uop, storage, inst, emit_braces=True):
        # A nested declaration has a new C identity. Its proof cannot leak
        # to the outer variable when this lexical scope ends.
        scope = {}
        self.native_scopes.append(scope)
        try:
            reachable, end, storage = super().emit_BlockStmt(
                stmt, uop, storage, inst, emit_braces)
            storage.stack.element_indices = {
                name: value for name, value in storage.stack.element_indices.items()
                if value[3] != id(scope)}
            for name, previous in scope.items():
                if previous is None:
                    storage.stack.native_values.pop(name, None)
                else:
                    storage.stack.native_values[name] = previous
            return reachable, end, storage
        finally:
            self.native_scopes.pop()

    def emit_MacroIfStmt(self, stmt, uop, storage, inst):
        storage.stack.shape_read_binding = None
        storage.stack.element_guards.clear()
        storage.stack.element_indices.clear()
        self.native_condition_depth += 1
        try:
            return super().emit_MacroIfStmt(stmt, uop, storage, inst)
        finally:
            self.native_condition_depth -= 1

    def before_statement(self, stmt: SimpleStmt, storage: Storage,
                         inst: Instruction | None) -> None:
        """Take each input whose owner the statement reads, so that the
        statement can read it; check OWNER's and S7's rules"""
        tokens = [t for t in stmt.contents if t.kind != "COMMENT"]
        if not any(token.text in ELEMENT_INTRINSICS for token in tokens):
            storage.stack.element_guards.clear()
            storage.stack.element_indices.clear()
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

    def current_value_locals(self, storage):
        native = storage.stack.native_values
        return [var for var in storage.stack.variables + storage.inputs + storage.outputs
                if var.name not in native] + list(native.values())

    def note_nonnull_changes(self, tokens, storage, *, positive=True):
        locals = self.current_value_locals(storage)
        names = {var.name for var in locals if not var.is_array()}
        current = {var.name for var in locals
                   if var.in_local and var.nonnull and not var.is_array()}
        changes = value_nonnull_changes(tokens, current if positive else set(), names)
        for var in locals:
            if var.name in changes:
                var.nonnull = positive and changes[var.name] and var.in_local

    def statement_retired_values(self, stmt, sources, storage):
        """Actual typed releases consume a native value, not its C name."""
        retired = set()
        tokens = [token for token in stmt.contents if token.kind != "COMMENT"]
        if [token.text for token in tokens] == ["DECREF_INPUTS", "(", ")", ";"]:
            return {sources[var.name][0] for var in storage.inputs
                    if var.name in sources and sources[var.name][0] is not None}
        releases = {"PyValue_CLOSE", "PyValue_XCLOSE", "PyValue_CLOSE_SPECIALIZED",
                    "PyValue_AsPyObjectSteal", "PyValue_AsKnownObjectSteal"}
        for at, token in enumerate(tokens):
            if token.text not in releases or at + 1 == len(tokens) or tokens[at + 1].text != "(":
                continue
            args, _ = instruction_macro_args(token, TokenIterator(tokens[at + 1:]))
            if args:
                source = value_transport_expression(args[0])
                if source is not None and source[0] == "copy":
                    value, _ = sources.get(source[1], (None, None))
                    if value is not None:
                        retired.add(value)
        return retired

    def apply_value_transport(self, storage, assignment, transport, sources, changes,
                              result=None):
        stack = storage.stack
        assert isinstance(stack, IRStack)
        targets = self.current_value_locals(storage)
        # Unknown assignments/aliases lose value identity even when the
        # non-NULL lattice independently retains a useful weaker fact.
        identities_changed = set(changes) if assignment is None else {assignment[0]}
        if transport is not None and transport[0] == "take":
            identities_changed.add(transport[1])
        for var in targets:
            if var.name in identities_changed:
                var.ir_value = None
                var.owns_value = False
        if assignment is None:
            return
        name = assignment[0]
        value = None
        if transport is not None:
            mode, source = transport
            if mode == "literal":
                flags = int(source != "PyValue_NULL")
                value = stack.home_action("_PyIR_OWNER_STORAGE_LITERAL", flags=flags)
                if value is not None:
                    stack.native_owners.add(value)
                    stack.possible_native_owners.add(value)
            else:
                value, _ = sources.get(source, (None, None))
                if value in stack.expired_values:
                    stack.home_node = None
                    value = None
                if mode == "dup" and value is not None:
                    owned = value in stack.native_owners
                    saved = stack.home_graph.nodes[value - 1].kind == "_PyIR_OWNER_STORAGE_SAVE"
                    if not owned and not (saved and value in stack.home_values.values()):
                        stack.home_node = None
                    value = stack.home_action("_PyIR_OWNER_STORAGE_COPY", value=value,
                                              flags=int(owned))
                    if value is not None:
                        stack.native_owners.add(value)
                        stack.possible_native_owners.add(value)
        elif result is not None and result.result_owned is True:
            flags = int(result.result_canonical) | (int(result.result_nonnull) << 1)
            value = stack.home_action("_PyIR_OWNER_STORAGE_RESULT", flags=flags)
            if value is not None:
                stack.native_owners.add(value)
                stack.possible_native_owners.add(value)
        elif result is not None:
            # An unclassified VALUE result may hide an owning native
            # temporary. Neither its type nor a C name closes that ledger.
            stack.home_node = None
        for var in targets:
            if var.name == name:
                var.ir_value = value

    def emit_SimpleStmt(self, stmt: SimpleStmt, uop: CodeSection,
                        storage: Storage, inst: Instruction | None
                        ) -> tuple[bool, Token | None, Storage]:
        """A statement that kills an input, as DEAD(), a close or a steal,
        gives up its slot"""
        if (self.generator_boundary is not None and
                self.send_source_parts is not None and
                uop is self.send_source_parts[-1] and
                _statement_text(stmt) == ["PyValue_CLOSE", "(", "v", ")", ";"]):
            # The complete source contract proves sent.value is the owned
            # successful provider result and that all other state is homed.
            # A failed Try leaves v intact for the rooted effect entry.
            self.emit("if (!_PyValue_TryXCloseNoReentry(v)) {\n")
            self.emit("assert(_PyValue_IsHome(sent.value));\n")
            self.emit("IR_GEN_JUMP(GEN_YIELD_INPUT_EFFECT, frame, "
                      "this_instr, sent.value);\n")
            self.emit("}\n")
            self.boundary_reached.add("yield")
            self.emit("window[2] = PyValue_NULL;\n")
            self.value_kill(next(token for token in stmt.contents if token.text == "v"),
                            storage, False)
            storage.stack.home_node = None
            return True, None, storage
        if not any(token.text in {"IR_GUARD_SHAPE_READ", "IR_DUP_SHAPE_READ"}
                   for token in stmt.contents):
            storage.stack.shape_read_binding = None
        leaf_marker = any(t.text == "IR_TRY_DICT_FIRST_PROBE" for t in stmt.contents)
        transaction_marker = any(t.text in IR_MARKER_EFFECTS
                                 for t in stmt.contents)
        if inst is not None and uses_generic_call(inst):
            transaction_marker |= any(t.text in {
                "_Py_Specialize_CallKw",
                "_Py_Specialize_CallFunctionEx"} for t in stmt.contents)
        previous = self.cannot_escape
        self.statement = stmt
        self.element_result = None
        # Capture RHS facts before TAKE/DEAD or an escaping release kills
        # its source. Apply them to the surviving identities after emission.
        locals = self.current_value_locals(storage)
        current = {var.name for var in locals
                   if var.in_local and var.nonnull and not var.is_array()}
        declaration = value_declaration(stmt.contents)
        fresh = set()
        if declaration is not None:
            name = declaration
            if self.native_condition_depth and any(var.name == name for var in locals):
                raise analysis_error("conditional native shadow needs a binding proof", stmt.contents[0])
            scope = self.native_scopes[-1]
            previous_native = storage.stack.native_values.get(name)
            scope.setdefault(name, previous_native.copy() if previous_native is not None else None)
            storage.stack.native_values[name] = Local.register(name)
            locals = self.current_value_locals(storage)
            current.discard(name)
            fresh.add(name)
        nonnull_changes = value_nonnull_changes(
            stmt.contents, current, {var.name for var in locals if not var.is_array()}, fresh)
        assignment = value_assignment(stmt.contents)
        transport = value_transport_expression(assignment[2]) if assignment else None
        sources = {var.name: (var.ir_value, var.ir_inplace) for var in locals}
        retired = self.statement_retired_values(stmt, sources, storage)
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
        if reachable and isinstance(uop, Uop):
            # The facts a row's result has from its producer (F7)
            texts = [t.text for t in stmt.contents if t.kind != "COMMENT"]
            for line in ir_contract.assumed_assertions(uop, texts):
                self.out.start_line()
                self.emit(line + "\n")
        stack = storage.stack
        assert isinstance(stack, IRStack)
        if self.element_result is not None and assignment is not None:
            nonnull_changes[assignment[0]] = self.element_result.result_nonnull
        guarded = error_nonnull_value(stmt.contents)
        intrinsic = any(token.text in ELEMENT_INTRINSICS for token in stmt.contents)
        addressed = {token.text for _, token in addressed_locals(
            inst.name if inst is not None else "", stmt.contents,
            {var.name: var.is_array() for var in self.current_value_locals(storage)})}
        for var in self.current_value_locals(storage):
            if var.name in nonnull_changes:
                var.nonnull = nonnull_changes[var.name] and var.in_local
                # An opaque lvalue/address use can replace C bits as well as
                # their non-NULL fact. A prior canonical token is then stale.
                if ((assignment is None or var.name in addressed) and not intrinsic and
                        var.name != guarded and not nonnull_changes[var.name]):
                    var.ir_value = None
                    var.owns_value = False
        for var in self.current_value_locals(storage):
            if var.name == guarded:
                var.nonnull = var.in_local
        result = (dict(self.error_source.value_results).get(id(stmt))
                  if self.error_source is not None else None)
        if self.element_result is not None:
            result = self.element_result
        self.apply_value_transport(storage, assignment, transport, sources,
                                   nonnull_changes, result)
        if guarded is not None and reachable:
            self.refine_current_value(storage, guarded, True)
        if self.observer_source is not None:
            self.observer_call_receipt(stmt)
            if _statement_text(stmt) == ["DISPATCH_GOTO", "(", ")", ";"]:
                self.observer_terminals.add(id(stmt.contents[0]))
        for target in uop.local_stores:
            if target in stmt.contents:
                var = find_local(target.text, storage)
                if var is not None and var.ir_inplace is None and not var.is_array():
                    stack.bind(var.name, None)
        self.note_view_object(stmt, storage, escape is not None)
        if self.null_outputs is not None:
            stack.note_null_statement(stmt, self.null_outputs)
        if not leaf_marker and (escape is not None or transaction_marker):
            stack.code_ran(current_identity=self.callback_current_identity)
        if reachable:
            self.consume_killed(storage)
            for value in retired:
                stack.retire_value(value)
        if (inst is not None and _statement_text(stmt) == [
                "FT_ATOMIC_STORE_INT8_RELEASE", "(", "gen", "->",
                "gi_frame_state", ",", "FRAME_SUSPENDED", "+", "oparg",
                ")", ";"] and abrupt_suspend_contract(inst, self.format_of(inst))):
            # The actual release publishes the suspended state only after
            # its own homes/header commits. This receipt is separate from
            # successful normal vacates and from readable phase recipes.
            self.abrupt_prefix = True
        return reachable, tkn, storage

    def observer_call_receipt(self, stmt: SimpleStmt) -> None:
        """Record the callback statement after its ordinary emission."""
        for token in stmt.contents:
            if token.text in OBSERVER_ENTRY_HELPERS:
                self.observer_calls.add(id(token))

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
            self.defer_close(stack, close, name)
            var.kill()
        storage.clear_inputs("after DECREF_INPUTS")
        stack.code_ran()
        return True

    def goto_error(self, offset: int, storage: Storage) -> str:
        """Clear the slots that the stack gave up, which the stack machine
        left above its stack pointer, and leave the rest to
        exception_unwind."""
        # Releases may already have removed dead inputs from Stack while
        # Storage still names them for the uop. Only the live stack is
        # flushed on this edge; cloning Storage would require those inputs.
        stack = storage.stack.copy()
        assert isinstance(stack, IRStack)
        if self.statement is not None:
            guarded = error_nonnull_value(self.statement.contents)
            value = next((var for var in self.current_value_locals(storage)
                          if var.name == guarded), None)
            if value is not None:
                # Branch facts belong to the copied error state, never to
                # the original output identity used on normal continuation.
                stack.refine_value(value.copy(), False)
        stack.error_exit = True
        stack.publish_on_error = self.error_uop_name == "_CHECK_PERIODIC_AT_END"
        stack.flush(self.out)
        # Rule 9: the owners that C holds close before the unwind
        self.emit_deferred(stack)
        self.operand_error_flush(stack)
        checkpoint = self.record_error_checkpoint(stack)
        return f"PROCESS_ERROR_AT(RAISE, {checkpoint}, this_instr);"

    def record_error_checkpoint(self, stack: IRStack) -> int:
        """Capture the actual flushed stack, independently of normal IO.

        Every supported scalar action is replayed from its actual incoming
        shape. An uncovered action or a native owner without a home keeps
        the checkpoint on the explicit control path.
        """
        source = self.error_source
        fmt = self.handler_format
        if (source is None or source.homes not in {
                HomeEffect.NO_ARENA_WRITE, HomeEffect.PRESERVES_CANONICAL_OR_NULL} or
                source.changes_origin or fmt is None or fmt.window or
                stack.reloaded or stack.home_node is None or
                stack.home_writes != stack.home_write_receipts or
                stack.possible_native_owners or stack.unknown_read or
                any(stack.read_fields.values())):
            return 0
        route = self.abrupt_operand_origin
        if route is None or route not in source.native_closed_errors:
            return 0
        self.error_checkpoint_count += 1
        if self.error_checkpoint_count > 255:
            raise StackError("more than 255 emitted error checkpoints")
        checkpoint = self.error_checkpoint_count
        self.error_checkpoint_sites[checkpoint] = ErrorCheckpoint(
            checkpoint, int(source.homes == HomeEffect.PRESERVES_CANONICAL_OR_NULL),
            stack.home_node,
            tuple(stack.home_graph.nodes[:stack.home_node]))
        if (self.allowed_error_checkpoints is not None and
                checkpoint not in self.allowed_error_checkpoints):
            return 0
        return checkpoint

    def operand_error_flush(self, stack):
        # These are actual error stores from the same IRStack used to emit C.
        # Scalar layouts address only encoded inputs and the recipient; the
        # source gate has excluded independent aliases and manual protocols.
        if not isinstance(stack, IRStack):
            self.abrupt_operand_uncovered = True
        if self.abrupt_operand_origin is not None:
            self.abrupt_operand_sources.add(self.abrupt_operand_origin)
            self.observer_errors.add(self.abrupt_operand_origin)
        else:
            self.abrupt_operand_uncovered = True

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
        previous = self.error_redispatch_origin
        self.error_redispatch_origin = id(tkn)
        try:
            return super().deopt_if(tkn, tkn_iter, uop, storage, inst)
        finally:
            self.error_redispatch_origin = previous

    def exit_if(self, tkn, tkn_iter, uop, storage, inst):
        self.check_exit_inputs(tkn, storage)
        previous = self.error_redispatch_origin
        self.error_redispatch_origin = id(tkn)
        try:
            return super().exit_if(tkn, tkn_iter, uop, storage, inst)
        finally:
            self.error_redispatch_origin = previous

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
        previous = self.abrupt_operand_origin
        self.abrupt_operand_origin = id(tkn)
        self.error_uop_name = uop.name
        try:
            return super().error_if(tkn, tkn_iter, uop, storage, inst)
        finally:
            self.abrupt_operand_origin = previous
            self.error_uop_name = None

    def emit_return(self, tkn: Token, tkn_iter: TokenIterator) -> None:
        "The activation ends with the value: an EXIT, which each binding defines"
        self.out.start_line()
        self.out.emit("IR_EXIT(")
        semicolon = emit_to(self.out, tkn_iter, "SEMI")
        self.out.emit(")")
        self.emit(semicolon)

    def goto_label(self, goto: Token, label: Token, storage: Storage) -> None:
        self.check_owner_exit(goto)
        if label.text in ERROR_INGRESSES:
            label_node = self.labels[label.text]
            if label_node.spilled and not storage.spilled:
                self.emit_save(storage)
            ingress = ERROR_INGRESSES[label.text]
            origin = ("frame->instr_ptr" if ingress in {"EXIT", "EXIT_NOTRACE"}
                      else "next_instr - 1")
            self.out.start_line()
            self.out.emit("".join(storage.stack.deferred))
            self.emit(f"PROCESS_ERROR_AT({ingress}, 0, {origin})")
        else:
            super().goto_label(goto, label, storage)
        if label.text == "error" and self.observer_source is not None:
            # The transparent wrapper has no frame-home transport. This is
            # its actual emitted failure route after the callback/PC refresh.
            self.observer_errors.add(id(goto))
        if label.text == "exception_unwind":
            self.error_unwind_sources.add(id(goto))
        else:
            self.error_uncovered = True

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
        previous = self.abrupt_operand_origin
        self.abrupt_operand_origin = id(tkn)
        try:
            return super().error_no_pop(tkn, tkn_iter, uop, storage, inst)
        finally:
            self.abrupt_operand_origin = previous

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
        self.finish_stores(storage.stack)
        self.error_terminal_sources.add(id(tkn))
        self.observer_terminals.add(id(tkn))
        self.error_finalized = True
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
        self.error_terminal_sources.add(id(tkn))
        self.emit(tkn)
        return False

    def emit_jump_to_predicted(self, inst: Instruction | None) -> None:
        assert inst is not None
        fmt = self.format_of(inst)
        target = self.formats.get(inst.family.name) if inst.family is not None else None
        if (self.send_source_parts is not None and self.error_redispatch_origin is not None
                and inst.family is not None and inst.family.name == fmt.group
                and target is not None and target.group == fmt.group
                and (target.roles, target.window, target.n_inputs,
                     target.dst_offset, target.imm, target.fb, target.borrow_mask) ==
                    (fmt.roles, fmt.window, fmt.n_inputs,
                     fmt.dst_offset, fmt.imm, fmt.fb, fmt.borrow_mask)):
            # The positive adapter proof excludes effects before these guards.
            # check_exit_inputs has checked their actual Storage state. The
            # emitted target is exactly this same canonical protocol family.
            self.error_terminal_sources.add(self.error_redispatch_origin)
        else:
            self.error_uncovered = True
        deopt = self.record.deopt if self.record is not None else ""
        kind, _, head = deopt.partition("(")
        head = head.rstrip(")")
        if kind == "BINARY_OP_BY_ENTRY":
            # The handler of the operation that the entry records
            self.emit(f"UPDATE_MISS_STATS({BINARY_OP_FAMILY});\n")
            self.emit(f"IR_DEOPT_BINARY_OP({field_expr(fmt.fb)});\n")
            return
        if kind == "FUSED_HEAD":
            # The head has no opcode: the miss counts for the compare
            self.emit(f"UPDATE_MISS_STATS({FUSED_SHADOWS[head]});\n")
            self.emit(f"JUMP_TO_PREDICTED({self.jump_prefix}{head});\n")
            return
        if kind == "GENERATOR_SHAPE":
            if not head:
                raise analysis_error("generator deopt has no complete source shape", inst.where)
            self.emit(f"UPDATE_MISS_STATS({inst.family.name});\n")
            self.emit(f"JUMP_TO_PREDICTED({head});\n")
            return
        if kind == "IR_ONLY_HEAD":
            # No opcode of its own: nothing to look up in _PyOpcode_Deopt
            self.emit(f"UPDATE_MISS_STATS({head});\n")
            self.emit(f"JUMP_TO_PREDICTED({self.jump_prefix}{head});\n")
            return
        if kind == "FAMILY_HEAD":
            super().emit_jump_to_predicted(inst)
            return
        raise analysis_error(f"{inst.name} deoptimises but has no family",
                             inst.where)

    def process_error(self, tkn, tkn_iter, uop, storage, inst):
        args, _ = self.instruction_macro_args(tkn, tkn_iter)
        if (inst is not None or len(args) != 2 or
                any(len(arg) != 1 for arg in args) or
                args[0][0].text not in ERROR_INGRESSES.values() or
                args[1][0].text != "0"):
            raise analysis_error("unsupported error label ingress or checkpoint", tkn)
        ingress, checkpoint = (arg[0].text for arg in args)
        end = next(tkn_iter)
        if end.kind != "SEMI":
            raise analysis_error("error ingress must be a complete statement", end)
        origin = ("frame->instr_ptr" if ingress in {"EXIT", "EXIT_NOTRACE"}
                  else "next_instr - 1")
        self.out.start_line()
        self.emit(f"PROCESS_ERROR_AT({ingress}, {checkpoint}, {origin});\n")
        return False

    def complete_step(self, tkn, tkn_iter, uop, storage, inst):
        """A completed loop/send consumes its window, not its body outputs.

        The helper owns both the held result and detached input owners on
        every exit. Do not flush the other outcome's cached window here.
        """
        kind, value, distance = self.macro_args(tkn, tkn_iter, 3)
        assert inst is not None
        # IR_EXIT is the word's exit, for an instruction whose stack form has
        # no operand (CLEANUP_THROW); SEND and FOR_ITER pass their oparg
        if distance == "IR_EXIT":
            distance = "oparg"
        entries = {
            "_PY_IR_FOR_ITER": "_PyEval_CompleteForIter",
            "_PY_IR_SEND": "_PyEval_CompleteSend",
            "_PY_IR_SEND_THROW": "_PyEval_CompleteSendThrow",
        }
        if kind not in entries:
            raise analysis_error("unsupported completion kind", tkn)
        fmt = self.format_of(inst)
        if distance == "oparg":
            self.normal_completion(inst, fmt, kind)
        else:
            self.normal_completion_uncovered = True
        base = FIELD_VARS[fmt.field_of(ROLE_BASE)]
        dst = FIELD_VARS[fmt.field_of(ROLE_DST)]
        self.emit("{\n")
        target = (f"_PyFrame_InstrAt(frame, {distance})" if kind == "_PY_IR_SEND_THROW"
                  else f"next_instr + {distance}")
        if self.generator_boundary is not None and kind in {"_PY_IR_SEND", "_PY_IR_SEND_THROW"}:
            self.emit(f"PyValue ir_completed = {value};\n")
            self.emit("assert(_PyValue_IsHome(ir_completed));\n")
            self.emit("IR_GEN_COMPLETE(ir_completed);\n")
            self.emit("}\n")
            self.boundary_reached.add("complete")
            self.boundary_completions.append((tkn, kind))
            self.abrupt_operand_sources.add(id(tkn))
            self.error_terminal_sources.add(id(tkn))
            self.error_finalized = True
            return False
        recipient = "" if kind == "_PY_IR_FOR_ITER" else f"{dst}, "
        self.emit(f"_PyIRStepResult ir_step_result = {entries[kind]}(tstate, frame, this_instr, {base}, {recipient}{target}, {value});\n")
        self.emit("next_instr = ir_step_result.next;\n")
        self.emit("if (ir_step_result.status < 0) { PROCESS_ERROR_AT(RAISE, 0, next_instr - 1); }\n")
        self.emit("DISPATCH();\n")
        self.emit("}\n")
        # The reached helper owns its result and whole encoded window on
        # every error exit. Its status check and dispatch are terminal here.
        self.abrupt_operand_sources.add(id(tkn))
        self.error_terminal_sources.add(id(tkn))
        self.error_finalized = True
        return False

    def normal_completion(self, inst, fmt, kind):
        inputs, _ = external_io(inst)
        produces_result = kind != "_PY_IR_FOR_ITER"
        count = 4 if kind == "_PY_IR_SEND_THROW" else 3 if produces_result else 2
        if (not fmt.window or len(inputs) != count or
                any(item.is_array() for item in inputs) or
                fmt.field_of(ROLE_BASE) != "B" or fmt.field_of(ROLE_DST) != "A" or
                fmt.dst_offset != (0 if kind == "_PY_IR_SEND_THROW" else 2)):
            raise analysis_error("unsupported normal completion window", inst.where)
        # The actual completion helper drains the complete input window,
        # then SEND publishes A. A resumed child reaches the same helper on
        # return; a body/yielded result does not take this edge. Flag 1 is
        # the encoded _PyIR_COMPLETION_HAS_RESULT capability.
        self.normal_completions.add(OwnershipEvent(
            "_PyIR_OWNER_NORMAL_COMPLETION", flags=int(produces_result),
            src=count, dst=1))

    def finish_generator_return(self, tkn, tkn_iter, uop, storage, inst):
        value, dying = self.macro_args(tkn, tkn_iter, 2)
        if inst is None or not canonical_frame_transfer_input(inst):
            raise analysis_error("generator return needs canonical source transfer", tkn)
        empty = 1 if tkn.text.endswith("_EMPTY") else 0
        self.emit(f"IR_GEN_RETURN({value}, {dying}, {empty});\n")
        return False

    def return_to_caller(self, tkn, tkn_iter, uop, storage, inst):
        """Terminal transfer; the retired child has no fictitious output."""
        value, generator_return = self.macro_args(tkn, tkn_iter, 2)
        self.emit(f"assert(_PyValue_IsHome({value}));\n")
        self.emit(f"if ({generator_return}) {{\n")
        self.emit(f"_PyIRStepResult ir_step_result = _PyEval_CompleteGeneratorReturn(tstate, frame, {value});\n")
        self.emit("next_instr = ir_step_result.next;\n")
        self.emit("if (ir_step_result.status < 0) { PROCESS_ERROR_AT(RAISE, 0, next_instr - 1); }\n")
        self.emit("}\n")
        self.emit("else {\n")
        self.emit("next_instr = frame->return_pc;\n")
        self.emit(f"IR_STORE_RETURN({value});\n")
        self.emit("}\n")
        self.emit("LLTRACE_RESUME_FRAME();\n")
        self.emit("DISPATCH();\n")
        return False

    def enter_generator(self, tkn, tkn_iter, uop, storage, inst):
        """Commit a resumed child's complete state before disposing input.

        In particular an unused sent value may be the last mortal owner;
        neither the caller window nor an unentered child may advertise it
        while its finalizer executes.
        """
        gen, kind = self.macro_args(tkn, tkn_iter, 2)
        assert inst is not None
        entry = generator_entry_recipe(inst)
        if (kind not in {"_PY_IR_FOR_ITER", "_PY_IR_SEND"}
                or entry is not None and
                    (entry.part is not uop or entry.generator != gen or entry.kind != kind)
                or kind == "_PY_IR_FOR_ITER" and entry is None):
            raise analysis_error("unsupported generator-entry recipient", tkn)
        fmt = self.format_of(inst)
        if kind in {"_PY_IR_FOR_ITER", "_PY_IR_SEND"}:
            self.normal_completion(inst, fmt, kind)
        else:
            self.normal_completion_uncovered = True
        if kind == "_PY_IR_SEND" and self.send_source_parts is not None:
            self.send_yielded_published = True
        base = FIELD_VARS[fmt.field_of(ROLE_BASE)]
        dst = FIELD_VARS[fmt.field_of(ROLE_DST)]
        self.emit("{\n")
        self.emit(f"_PyInterpreterFrame *ir_resumed_frame = &{gen}->gi_iframe;\n")
        self.emit("PyValue ir_resume_input = PyValue_None;\n")
        self.emit(f"if ({kind} == _PY_IR_SEND) {{\n")
        self.emit(f"ir_resume_input = IR_SLOT({base} + 2);\n")
        self.emit(f"IR_SLOT({base} + 2) = PyValue_NULL;\n")
        self.emit("}\n")
        self.emit(f"frame->return_slot = (uint16_t)({kind} == _PY_IR_SEND ? {base} + 2 : {dst});\n")
        self.emit("frame->return_pc = next_instr;\n")
        self.emit("ir_resumed_frame->previous = frame;\n")
        self.emit(f"{gen}->gi_exc_state.previous_item = tstate->exc_info;\n")
        self.emit(f"tstate->exc_info = &{gen}->gi_exc_state;\n")
        self.emit("frame = tstate->current_frame = ir_resumed_frame;\n")
        if self.generator_boundary is not None:
            self.emit("if (tstate->py_recursion_remaining <= 1) {\n")
            self.emit("IR_GEN_JUMP(GEN_ENTRY_EFFECT, frame, "
                      "frame->instr_ptr, ir_resume_input);\n")
            self.emit("}\n")
            self.emit("--tstate->py_recursion_remaining;\n")
            self.emit("IR_GEN_RESUME(ir_resume_input);\n")
            self.boundary_reached.add("enter")
        else:
            self.emit("if (_Py_EnterRecursivePy(tstate)) {\n")
            self.emit("PyValue_XCLOSE(ir_resume_input);\n")
            self.emit("PROCESS_ERROR_AT(EXIT_NOTRACE, 0, frame->instr_ptr);\n")
            self.emit("}\n")
            self.emit("_PyFrame_SetResumeValue(frame, ir_resume_input);\n")
            self.emit("LOAD_IP(0);\n")
            self.emit("IR_SKIP_RESUME();\n")
            self.emit("DTRACE_FUNCTION_ENTRY();\n")
            self.emit("LLTRACE_RESUME_FRAME();\n")
            self.emit("DISPATCH();\n")
        self.emit("}\n")
        # The entered heap frame cannot overlap the parent's values. Its
        # failed entry/return retires the child before error re-enters parent.
        self.abrupt_operand_sources.add(id(tkn))
        self.error_terminal_sources.add(id(tkn))
        return False

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

    def consume_object_guard(self, uop: Uop, name: str) -> None:
        "Mark the implicit object guard of input name of uop as printed"
        record = self.current_uop
        assert record is not None and record.name == uop.name
        for guard in record.guards:
            if guard.site is None and guard.input == name:
                self.record_consumed.add(id(guard))
                return
        raise analysis_error(f"{uop.name}: the object guard of '{name}' is not "
                             "in the handler's record", next(iter(uop.body.tokens())))

    def emit_object_inputs(
        self, uop: Uop, storage: Storage, inst: Instruction | None
    ) -> None:
        """The inputs that uop reads as objects (PyObject *).  A guard exits
        if one holds an inline value; it is then known to be an object for
        the rest of the instruction.  A uop that reads it as an object
        without a guard or an assert is refused: no handler boxes."""
        for var in storage.inputs:
            if var.name in uop.asserted_objects:
                var.known_object = True
            if var.ir_inplace is not None:
                self.check_inplace_input(uop, var.name, inst)
        if uop.properties.inline_ok:
            return
        guard = uop.is_guard()
        for var in storage.inputs:
            if var.name not in uop.object_inputs or var.known_object:
                continue
            if uop.clauses and not guard:
                # The contract's walk proved the object fact of the
                # conversion, which a row requires of its argument
                var.known_object = True
                continue
            self.out.start_line()
            # Test the slot rather than the local where the slot holds the
            # value, which is cheaper (see _PyValue_TopBitsAt())
            ref = var.name
            at = ""
            if var.ir_inplace is not None:
                # A view of an input in place: the slot still holds its value
                ref = f"&IR_SLOT({FIELD_VARS[var.ir_inplace]})"
                at = "At"
            elif var.memory_offset is not None:
                ref = f"&{storage.stack.address(var.memory_offset)}"
                at = "At"
            if guard:
                self.consume_object_guard(uop, var.name)
                if inst is not None:
                    self.check_exit_inputs(inst.where, storage,
                                           f"The guard of {uop.name}")
                self.emit(f"if (Py_UNLIKELY(!PyValue_IsPointerSpace{at}({ref}))) {{\n")
                previous = self.error_redispatch_origin
                if self.send_source_parts is not None and uop in self.send_source_parts:
                    self.error_redispatch_origin = (id(uop), var.name)
                try:
                    self.emit_jump_to_predicted(inst)
                finally:
                    self.error_redispatch_origin = previous
                self.emit("}\n")
                var.known_object = True
                continue
            # No handler boxes an inline input (handlers81, H3): an input
            # that a uop reads as an object is asserted to be one
            # (assert(PyValue_IsObject(x))) or tested by a guard, and a
            # callee that may get a number takes a value
            raise analysis_error(
                f"{uop.name} reads '{var.name}' as an object, which may be an "
                "inline value: assert(PyValue_IsObject()) it, or pass the "
                "value to a callee that takes one", next(iter(uop.body.tokens())))


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
    if emitter.stub_escapes is not None and uop.name in emitter.stub_escapes:
        # Nothing in the evaluator and the forms; ESCAPED in a stub
        emitter.emit("IR_STUB_ESCAPE();\n")
    storage = Storage.for_uop(stack, uop, emitter.out)
    record = emitter.uop_record(uop)
    for operand in record.feedback:
        # The local's type follows the entry field's (pycore_feedback.h)
        name, field = operand.name, operand.field
        if field.ctype == "_PyValueGuarded":
            value = f"_PyValueGuarded {name} = fb->{name};"
        elif field.is_pointer:
            value = f"PyObject *{name} = (PyObject *)fb->{name};"
        elif field.ctype == "uint32_t":
            binding = storage.stack.shape_read_binding
            source = (binding[1] if name == "offset" and binding is not None
                      else f"fb->{name}")
            value = f"uint32_t {name} = {source};"
        else:
            raise analysis_error(f"{uop.name}: cache operand '{name}' has type "
                                 f"{field.ctype}, which no uop reads", inst.where)
        emitter.emit(value + "\n")
        if inst.family is None:
            emitter.emit(f"(void){name};\n")
        emitter.record_consumed.add(id(operand))
    emitter.emit_object_inputs(uop, storage, inst)
    # The facts the uop assumes of its inputs, from their producers (F7)
    for line in ir_contract.assumed_assertions(uop, None):
        emitter.emit(line + "\n")
    if emitter.uop_index - 1 == emitter.pin_at:
        # After the arm's last guard: the word is untouched (g3/design.md
        # section 2).  A stub reports the old pin's release, the forms
        # discard it and the evaluator binds it to nothing
        emitter.emit("IR_PIN_DEPARTING();\n")
        emitter.pinned = True
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


def closed_generator_boundary(name: str, analysis: Analysis,
                              formats: dict[str, IRFormat]) -> GeneratorBoundary | None:
    "The generator boundary shape of handler name, if its source is closed"
    boundary = generator_boundaries(analysis).get(name)
    if boundary is not None and not generator_boundary_source_closed(
            boundary, analysis, formats):
        return None
    return boundary


def deopt_target(inst: Instruction, boundary: GeneratorBoundary | None,
                 analysis: Analysis) -> str:
    """Where a failed guard of inst goes: the handler of the operation that
    the entry records for BINARY_OP, the generic shape of a generator
    boundary, or the family head; "" if inst has no family"""
    if inst.family is None:
        return ""
    family = inst.family.name
    if family == BINARY_OP_FAMILY:
        return "BINARY_OP_BY_ENTRY"
    if family in FUSED_JUMPS:
        return f"FUSED_HEAD({family})"
    if boundary is not None:
        head = f"{family}_XFER_{boundary.completed}"
        if head not in generator_boundaries(analysis):
            return "GENERATOR_SHAPE()"
        return f"GENERATOR_SHAPE({generator_word_entry(head, analysis)})"
    if inst.name in analysis.ir_only:
        return f"IR_ONLY_HEAD({family})"
    return f"FAMILY_HEAD({family})"


def oparg_source(inst: Instruction, fmt: IRFormat, operation: str | None) -> str:
    "Where the handler's oparg comes from: the IMMEDIATE operand's detail"
    # CLEANUP_THROW has no oparg in the stack form; its word keeps the jump
    # distance for IR_EXIT
    if not inst.properties.oparg and "IR_EXIT" not in uop_tokens(inst):
        return ""
    if operation is not None:
        return "OPERATION"
    if fmt.fused:
        return "FUSED_OP"           # the entry's arg holds the operation
    if fmt.imm == LOC_FBARG and is_specialization(inst):
        return "FBARG_ENTRY"
    if fmt.imm == LOC_FBARG:
        return "FBARG"
    if fmt.forward and field_expr(fmt.imm).startswith("IR_D("):
        return "SIGNED_D"           # a distance has a sign
    if fmt.operands != "_PyIR_OPERANDS_NONE":
        return "CALL_ARGC"          # C counts the operands
    return "FIELD"


def call_shape(name: str, inst: Instruction, fmt: IRFormat) -> CallShape | None:
    "How the window of a call handler is a call, or None"
    call_family = inst.family.name if inst.family is not None else inst.name
    call_family = {"INSTRUMENTED_CALL_KW": "CALL_KW"}.get(call_family, call_family)
    if not fmt.window or call_family not in (
            "CALL_KW", "CALL_FUNCTION_EX", "INSTRUMENTED_CALL_FUNCTION_EX"):
        return None
    inputs, _ = external_io(inst)
    count = " + ".join(f"({item.size or '1'})" for item in inputs)
    kind = ("CALL_EX" if call_family in
            ("CALL_FUNCTION_EX", "INSTRUMENTED_CALL_FUNCTION_EX") else
            "CALL_KEYWORDS")
    # call_run_*() derives the operand count from its kind
    fixed = sum(1 for item in inputs if not item.size)
    arrays = [item.size for item in inputs if item.size]
    expected = {"CALL_KEYWORDS": (3, ["oparg"]),
                "CALL_EX": (4, [])}[kind]
    if uses_generic_call(inst) and (fixed, arrays) != expected:
        raise analysis_error(
            f"{name}: a {kind} call with inputs {count}", inst.where)
    flags = []
    if name.startswith("INSTRUMENTED_"):
        flags.append("CALL_INSTRUMENTED")
    if polls_at_end(name, inst):
        flags.append("CALL_PERIODIC")
    return CallShape(kind,
                     {"CALL_KEYWORDS": "call_run_keywords",
                      "CALL_EX": "call_run_ex"}[kind],
                     "(" + (" | ".join(flags) or "0") + ")",
                     "oparg" if inst.properties.oparg else "0")


def word_operands(fmt: IRFormat, oparg: str) -> tuple[Operand, ...]:
    "The operands of a word of format fmt, in field order"
    operands = []
    for field, role in zip(FIELDS, fmt.roles):
        if role == ROLE_DST:
            names = frozenset((LOCAL, TEMP)) if fmt.store_dst else frozenset((TEMP,))
            operands.append(Operand(SLOT, field, names, detail="dst"))
        elif role == ROLE_BASE:
            operands.append(Operand(WINDOW, field, frozenset((TEMP,)), 0, "base"))
        elif role.startswith(ROLE_IN0):
            k = int(role.rsplit("+", 1)[1])
            names = (frozenset((LOCAL, CONST, TEMP)) if fmt.borrow_mask & (1 << k)
                     else frozenset((TEMP,)))
            operands.append(Operand(SLOT, field, names, detail=f"input {k}"))
        elif role == ROLE_FB:
            operands.append(Operand(FEEDBACK, field))
        elif role == ROLE_IMM:
            if fmt.imm == LOC_CD and field == "D":
                continue            # the second half of CD
            where = "CD" if fmt.imm == LOC_CD else field
            if fmt.jumps or fmt.forward:
                operands.append(Operand(TARGET, where,
                                        detail="ABSOLUTE" if fmt.jumps else "FORWARD"))
            else:
                operands.append(Operand(IMMEDIATE, where, detail=oparg))
        elif role != ROLE_NONE:
            # An oparg that names a slot: a local or a cell
            operands.append(Operand(SLOT, field, frozenset((LOCAL,)),
                                    detail=role.removeprefix("_PyIR_ROLE_")))
    if fmt.imm == LOC_FBARG and oparg:
        operands.append(Operand(IMMEDIATE, "FBARG", detail=oparg))
    elif oparg == "OPERATION":
        operands.append(Operand(IMMEDIATE, None, detail=oparg))
    return tuple(operands)


def record_layout(inst: Instruction, fmt: IRFormat, mode: str | None,
                  operands: tuple[Operand, ...]) -> SlotLayout:
    """Where the handler keeps the positions of its stack effect: its input
    operands' fields, by position from the top, and its dst"""
    if fmt.transparent:
        return SlotLayout.empty()
    in_fields = {int(o.detail.split()[1]) - fmt.n_inputs: o.field for o in operands
                 if o.kind == SLOT and o.detail.startswith("input ")
                 and int(o.detail.split()[1]) not in fmt.metadata_inputs}
    borrowable = frozenset(k - fmt.n_inputs for k in range(fmt.n_inputs)
                           if fmt.borrow_mask & (1 << k))
    if fmt.window:
        base = get_stack_effect(inst).base_offset
        # The inputs are window[0:n] when their number is constant
        move = (mode == STORE_RESULT and fmt.operands == "_PyIR_OPERANDS_NONE"
                and not base.positive and not base.negative
                and fmt.dst_offset >= -base.numeric)
        return SlotLayout(True, base, in_fields, borrowable,
                          move_dst=fmt.dst_offset if move else None,
                          move_dst_args=FIELD_ARGS[fmt.field_of(ROLE_DST)] if move else "")
    return SlotLayout(False, PointerOffset.zero(), in_fields, borrowable,
                      dst=fmt.field_of(ROLE_DST), dst_offset=fmt.dst_offset,
                      store_dst=mode == STORE_RESULT)


def uop_feedback(uop: Uop, schema: FeedbackSchema | None,
                 inst: Instruction) -> tuple[FeedbackOperand, ...]:
    "The cache operands of uop, bound to the fields of the family's entry"
    result = []
    for cache in uop.caches:
        if cache.name == "unused":
            continue
        field = schema.field(cache.name) if schema is not None else None
        if field is None:
            raise analysis_error(
                f"{uop.name}: cache operand '{cache.name}' names no field of "
                f"{schema.type if schema is not None else 'a feedback entry'}",
                inst.where)
        # The DSL's units of a cache operand say which local the uop reads
        # it into: four for a pointer or a guarded value, fewer for 32 bits
        wide = field.is_pointer or field.ctype == "_PyValueGuarded"
        if wide != (cache.size == 4):
            raise analysis_error(
                f"{uop.name}: cache operand '{cache.name}' is a {field.ctype}, "
                f"so it must use {'four units' if wide else 'fewer than four units'}",
                inst.where)
        result.append(FeedbackOperand(cache.name, field))
    return tuple(result)


def implicit_object_guards(walk, index: int, uop: Uop, known: set[int]) -> list[Guard]:
    """The guards that the emitter puts before a uop that reads an input as
    an object (IREmitter.emit_object_inputs()): one per such input that is
    not known to be an object yet.  known holds those values, and grows."""
    _, inputs, _ = walk.uops[index]
    result = []
    for item in inputs:
        if item.name in uop.asserted_objects:
            known.add(item.value)
    if uop.properties.inline_ok or not uop.is_guard():
        return result
    for item in inputs:
        if item.name in uop.object_inputs and item.value not in known:
            result.append(Guard(EXIT_GUARD, None, item.name,
                                facts=(encoding(f"v{item.value}", "object"),)))
            known.add(item.value)
    return result


def build_record(name: str, inst: Instruction, fmt: IRFormat,
                 analysis: Analysis, formats: dict[str, IRFormat]) -> OperationRecord:
    "The record of handler name, which runs inst in a word of format fmt"
    boundary = closed_generator_boundary(name, analysis, formats)
    operation = None
    if name != inst.name and name not in EXECUTION_VARIANTS and boundary is None:
        operation = "NB_" + name.removeprefix(BINARY_OP_FAMILY + "_")
    oparg = oparg_source(inst, fmt, operation)
    walk = walk_values(inst)
    outputs = [Entry(external, item, aliased)
               for _, external, item, aliased in walk.outputs]
    mode = store_mode(inst, outputs) if fmt.store_dst else None
    operands = word_operands(fmt, oparg)
    layout = record_layout(inst, fmt, mode, operands)
    call = call_shape(name, inst, fmt)
    variant = EXECUTION_VARIANTS.get(name)
    layout = replace(layout, canonical_frame_transfer=canonical_frame_transfer_input(inst),
                     call_kind=call.kind if call is not None else None,
                     call_results=(frozenset(o.item.name for o in outputs)
                                   if call is not None else frozenset()))
    if variant is not None:
        layout = replace(layout, local_fields=variant.local_fields,
                         temp_fields=variant.temp_fields)
    schema = (feedback_schema(fmt.group, fmt.fb_layout)
              if fmt.fb_layout is not None else None)
    deopt = deopt_target(inst, boundary, analysis)
    uops = []
    known: set[int] = set()
    for index, (part, inputs, outs) in enumerate(walk.uops):
        guards = implicit_object_guards(walk, index, part, known)
        # The implicit guards precede the body and its transfers
        transfers = tuple(Transfer(DEOPT, None, deopt) for _ in guards)
        # The record is what the uop's declarations give, once its body is
        # verified against them (ir_contract.py)
        ir_contract.declared_contract(part)
        ir_contract.verify_uop(part, len(guards))
        for site in site_tokens(part):
            if site.text in GUARD_TOKENS:
                facts, reason = ir_contract.record_guard_facts(part, site, inputs)
                guards.append(Guard(GUARD_TOKENS[site.text], site, facts=facts, reason=reason))
        feedback = uop_feedback(part, schema, inst)
        leaf = ir_leaf.leaf_of(part, {fb.name: fb.field.ctype for fb in feedback})
        uops.append(UopRecord(part.name, inputs, outs, feedback,
                              part.properties.oparg, tuple(guards),
                              transfers + uop_transfers(part, deopt), uop_record_effects(part),
                              ir_contract.record_results(part, outs),
                              "pure" in part.annotations, leaf))
    if boundary is not None:
        uops[-1] = boundary_departure(uops[-1], boundary, inst.where)
    results = []
    for j, (value, external, item, _) in enumerate(walk.outputs):
        if external >= 0 and walk.values[value].external >= 0:
            continue
        if inst.properties.returns:
            results.append((value, Operand(SLOT, None, detail="caller")))
        elif fmt.window:
            results.append((value, Operand(WINDOW, fmt.field_of(ROLE_BASE), position=j)))
        else:
            results.append((value, Operand(SLOT, fmt.field_of(ROLE_DST),
                                           position=j - (fmt.dst_offset or 0)
                                           - len(walk.inputs))))
    if mode == STORE_RETURN:
        results.append((-1, Operand(SLOT, fmt.field_of(ROLE_DST), detail="callee return")))
    return OperationRecord(
        handler=name, instruction=inst, format=fmt, form="EMITTED",
        family=inst.family.name if inst.family is not None else None,
        operands=operands, values=tuple(walk.values), uops=tuple(uops),
        results=tuple(results), store_mode=mode, layout=layout, feedback=schema,
        uses_feedback=fmt.fb != LOC_NONE and (
            uses_feedback(inst) or reads_fb_arg(inst, fmt, operation) or fmt.fused),
        operation=operation, oparg=oparg, call=call,
        generator_boundary=boundary, deopt=deopt,
        inputs=tuple(walk.inputs), outputs=tuple(walk.outputs),
        polls=(any(part.properties.eval_breaker for part, _, _ in walk.uops)
               or polls_at_end(name, inst)),
        specializes=fmt.fb != LOC_NONE and any(
            "specializing" in part.annotations for part, _, _ in walk.uops),
        reads_poly_table=(variant is not None and variant.shape_cases) or any(
            t.text in POLY_TABLE_READERS for part, _, _ in walk.uops
            for t in part.body.tokens()))


def build_records(analysis: Analysis,
                  formats: dict[str, IRFormat]) -> dict[str, OperationRecord]:
    "The record of every handler, by name"
    shared = set(binary_op_handlers(analysis))
    records = {}
    for name, inst in handler_instructions(analysis).items():
        record = build_record(name, inst, formats[name], analysis, formats)
        if name in EDGE_TWINS:
            record = replace(record, form=f"EDGE_TWIN({EDGE_TWINS[name][0]})")
        elif name in EVENT_TWINS:
            record = replace(record, form=f"EVENT_TWIN({EVENT_TWINS[name][1]})")
        elif name in shared:
            record = replace(record, form="SHARED_BODY(binary_op)")
        elif generator_word_entry(name, analysis) != name:
            record = replace(record, form=f"SHARED_BODY({generator_word_entry(name, analysis)})")
        check_record(record, [p for p in inst.parts if isinstance(p, Uop)])
        ir_leaf.check_leaf(record, [p for p in inst.parts if isinstance(p, Uop)])
        fmt = formats[name]
        ir_contract.verify_handler(inst, leaf={u.name: u.leaf is not None
                                               for u in record.uops},
                                   temporaries=fmt.window and not fmt.n_inputs)
        records[name] = record
    ir_contract.check_primitives(ir_leaf.PRIMITIVES, ir_contract.lowerings())
    return records


def polls_at_end(name: str, inst: Instruction) -> bool:
    """Whether the generic call of handler name polls the eval breaker
    when the call returns, as _CHECK_PERIODIC_AT_END does"""
    return any(isinstance(p, Uop) and p.name == "_CHECK_PERIODIC_AT_END"
               for p in inst.parts) or canonical_handler(name) == "CALL_KW_NON_PY"


def uses_generic_call(inst: Instruction) -> bool:
    return any(isinstance(part, Uop) and
               any(token.text in CALL_INVOKE_MARKERS for token in part.body.tokens())
               for part in inst.parts)


def validate_dict_first_probe(inst: Instruction, fmt: IRFormat) -> None:
    parts = [p for p in inst.parts if isinstance(p, Uop)]
    markers = [(p, t) for p in parts for t in p.body.tokens()
               if t.text == "IR_TRY_DICT_FIRST_PROBE"]
    if not markers:
        return
    inputs, outputs = external_io(inst)
    expected = ["_GUARD_NOS_DICT_SUBSCRIPT", "_BINARY_OP_SUBSCR_DICT"]
    # The named guard's mapping-provider semantics are maintained with this
    # rule. Its exact shape and nonescaping effects keep the prefix harmless.
    guard = parts[0]
    producer = parts[-1] if parts else None
    first = (" ".join(t.text for t in producer.body.body[0].tokens())
             if producer and producer.body.body else "")
    def stack_items(items):
        return [(item.name, item.peek, item.size) for item in items]

    checked_shapes = [
        ([("nos", True, ""), ("unused", True, "")],
         [("nos", True, ""), ("unused", True, "")]),
        ([("dict_st", False, ""), ("sub_st", False, "")],
         [("res", False, "")]),
    ]
    if (len(markers) != 1 or [p.name for p in parts] != expected
            or [(stack_items(p.stack.inputs), stack_items(p.stack.outputs))
                for p in parts] != checked_shapes
            or inst.parts[-1] is not producer
            or first != "IR_TRY_DICT_FIRST_PROBE ( ) ;"
            or not guard.is_guard() or guard.properties.escapes
            or guard.properties.eval_breaker or guard.properties.switches_frame
            or guard.properties.error_with_pop or guard.properties.error_without_pop
            or len(inputs) != 2 or any(i.size for i in inputs)
            or len(outputs) != 1 or outputs[0].item.size
            or fmt.window or fmt.n_inputs != 2 or fmt.borrow_mask != 3
            or fmt.input_field(0) != "B" or fmt.input_field(1) != "C"
            or fmt.field_of(ROLE_DST) != "A" or not fmt.store_dst
            or fmt.unpacks):
        raise analysis_error("IR_TRY_DICT_FIRST_PROBE requires the checked "
                             "dictionary reader prefix and scalar format", inst.where)


# What IR_INVOKE_GUARDED_PYTHON_CALL's cached binding plan relies on (its
# row's evidence): the callable is the function of the plan's version, no
# PEP 523 evaluator replaces the call, and the recursion budget holds a
# frame.  The prefix declares each, the first as its pass-through's facts
# and the others as its guards' reasons (IS_PEP523_HOOKED's CONTEXT).
GUARDED_CALL_FACTS = frozenset((ir_contract.VFact("exact", "PyFunction_Type"),
                                ir_contract.VFact("func_version", "func_version")))
GUARDED_CALL_REASONS = frozenset(("a PEP 523 frame evaluator may replace the call",
                                  "the recursion budget is spent"))


def validate_guarded_python_call(inst: Instruction, fmt: IRFormat) -> None:
    """The guarded Python call's handler: its composition and operands, and a
    prefix whose declarations establish what the binding plan relies on"""
    parts = [p for p in inst.parts if isinstance(p, Uop)]
    markers = [(p, t) for p in parts for t in p.body.tokens()
               if t.text == "IR_INVOKE_GUARDED_PYTHON_CALL"]
    if not markers:
        return
    expected = ["_CHECK_PEP_523", "_CHECK_FUNCTION_VERSION_KW",
                "_CHECK_RECURSION_REMAINING", "_PY_FRAME_KW_GUARDED",
                "_SAVE_RETURN_SLOT", "_PUSH_FRAME"]
    if len(markers) != 1 or [p.name for p in parts] != expected:
        raise analysis_error("guarded Python call requires the checked prefix", inst.where)
    prefix = parts[:3]
    invoke = parts[3]
    inputs, outputs = external_io(inst)
    def shape(items):
        return [(item.name, item.peek, item.size) for item in items]
    guard_shapes = [([], []),
                    ([("callable", True, ""), ("unused", True, ""),
                      ("unused", True, "oparg"), ("unused", True, "")],
                     [("callable", True, ""), ("unused", True, ""),
                      ("unused", True, "oparg"), ("unused", True, "")]),
                    ([], [])]
    invoke_shape = ([("callable", False, ""), ("self_or_null", False, ""),
                     ("args", False, "oparg"), ("kwnames", False, "")],
                    [("new_frame", False, "")])
    contracts = [ir_contract.declared_contract(p) for p in prefix]
    facts: set = set()
    reasons = set()
    for part, contract in zip(prefix, contracts):
        facts.update(*(o.facts for o in contract.outputs if o.name == "callable"))
        for site in site_tokens(part):
            if site.text in GUARD_TOKENS:
                reasons.add(ir_contract.record_guard_facts(part, site, ())[1])
    if (inst.name != "CALL_KW_PY" or
            not fmt.window or not fmt.store_dst
            or fmt.field_of(ROLE_BASE) is None or fmt.field_of(ROLE_DST) is None
            or fmt.group != "CALL_KW"
            or not inst.properties.oparg
            or [(i.size) for i in inputs] != ["", "", "oparg", ""]
            or len(outputs) != 1 or outputs[0].item.size
            or [(shape(p.stack.inputs), shape(p.stack.outputs))
                for p in prefix] != guard_shapes
            or (shape(invoke.stack.inputs), shape(invoke.stack.outputs)) != invoke_shape
            or [t.text for t, _ in ir_contract.body_calls(invoke)]
               != ["IR_INVOKE_GUARDED_PYTHON_CALL"]
            # The prefix is declared guards that read, and the callable
            # passes through it unchanged
            or any(not c.effects <= {"reads"} or not c.exits <= {"next", "miss", "deopt"}
                   or any(o.name == "callable" and o.mode for o in c.outputs)
                   for c in contracts)
            or not GUARDED_CALL_FACTS <= ir_contract.implied(facts)
            or not GUARDED_CALL_REASONS <= reasons):
        raise analysis_error("guarded Python call requires the checked prefix", inst.where)


# These events permit observer callbacks and dynamic named-local writes, but
# not a scratch redirect. Normal completion still reaches the emitted final
# vacate; an instrumentation error follows the existing error route instead.
NORMAL_FRAME_OBSERVERS = {
    "_Py_call_instrumentation_2args": {
        "PY_MONITORING_EVENT_CALL", "PY_MONITORING_EVENT_C_RETURN"},
    "_Py_call_instrumentation_exc2": {"PY_MONITORING_EVENT_C_RAISE"},
}


STEAL_SUCCESS_HOME_HELPERS = IR_HELPER_CALLEES.keys()


def steal_success_home_contract(part: Uop) -> bool:
    """A native container owns inputs only on the final flush's success path."""
    inputs, outputs = part.stack.inputs, part.stack.outputs
    statements = part.body.body
    if (len(inputs) != 1 or inputs[0].peek or inputs[0].size != "oparg"
            or len(outputs) != 1 or outputs[0].peek or outputs[0].is_array()
            or len(statements) != 4 or not isinstance(statements[0], SimpleStmt)
            or not isinstance(statements[1], IfStmt)
            or not all(isinstance(stmt, SimpleStmt) for stmt in statements[2:])):
        return False
    call = _statement_text(statements[0])
    if (len(call) != 9 or call[:2] != ["PyObject", "*"]
            or call[3] != "=" or call[4] not in STEAL_SUCCESS_HOME_HELPERS
            or call[5:] != ["(", inputs[0].name, ")", ";"]):
        return False
    result = call[2]
    if result in {inputs[0].name, outputs[0].name, *HANDLER_LOCALS}:
        return False
    failure = statements[1]
    if (failure.else_body is not None
            or [t.text for t in failure.condition if t.kind != "COMMENT"] !=
                ["(", result, "==", "NULL", ")"]
            or not isinstance(failure.body, BlockStmt)
            or len(failure.body.body) != 1
            or not isinstance(failure.body.body[0], SimpleStmt)
            or _statement_text(failure.body.body[0]) != ["ERROR_NO_POP", "(", ")", ";"]):
        return False
    return (_statement_text(statements[2]) == ["INPUTS_DEAD", "(", ")", ";"]
            and _statement_text(statements[3]) ==
                [outputs[0].name, "=", "PyValue_FromObjectSteal", "(", result, ")", ";"])


def home_operation_contract(part: Uop) -> dict[int, tuple[str, int | str]]:
    """Check declared array capabilities without granting a raw pointer."""
    markers = [t for t in part.body.tokens() if t.text in IR_HELPER_EFFECTS]
    if not markers:
        return {}
    inputs = {item.name: item for item in part.stack.inputs}
    outputs = {item.name: item for item in part.stack.outputs}
    contexts: list[tuple[list[Token], tuple[tuple[int, tuple[str, ...]], ...]]] = []
    loops: dict[int, tuple[str, tuple[str, ...], tuple]] = {}

    def collect(stmt, guards=()):
        if isinstance(stmt, SimpleStmt):
            contexts.append((stmt.contents, guards))
        elif isinstance(stmt, BlockStmt):
            for child in stmt.body:
                collect(child, guards)
        elif isinstance(stmt, IfStmt):
            contexts.append((stmt.condition, guards))
            condition = tuple(t.text for t in stmt.condition if t.kind != "COMMENT")
            collect(stmt.body, guards + ((id(stmt), condition),))
            if stmt.else_body is not None:
                collect(stmt.else_body, guards + ((id(stmt), ("else",)),))
        elif isinstance(stmt, ForStmt):
            header = [t.text for t in stmt.header if t.kind != "COMMENT"]
            semicolons = [at for at, text in enumerate(header) if text == ";"]
            if (any(scope in loops for scope, _ in guards) or len(semicolons) != 2 or
                    header[:2] not in (["(", "int"], ["(", "Py_ssize_t"]) or
                    header[-1:] != [")"]):
                raise analysis_error("home loop needs a forward integer induction", stmt.for_)
            first, second = semicolons
            initial, condition, step = header[1:first], header[first + 1:second], header[second + 1:-1]
            if (len(initial) != 4 or initial[2:] != ["=", "0"] or
                    len(condition) != 3 or condition[:2] != [initial[1], "<"] or
                    step not in ([initial[1], "++"], ["++", initial[1]]) or
                    not (condition[2] == "oparg" or condition[2].isdigit())):
                raise analysis_error("home loop needs zero-to-extent unit induction", stmt.for_)
            index, extent = initial[1], tuple(condition[2:])
            if index in {*HANDLER_LOCALS, *IR_HELPER_EFFECTS, *inputs, *outputs}:
                raise analysis_error("home loop index shadows its extent or array", stmt.for_)
            body = list(stmt.body.tokens())
            if any(t.text in {"break", "continue", "goto", "return", "DISPATCH",
                              "DISPATCH_REDIRECT", "DISPATCH_INLINED", "JUMPTO", "JUMPBY", "ERROR_IF",
                              "ERROR_NO_POP", "EXIT_IF", "DEOPT_IF", "Py_UNREACHABLE"}
                   or t.kind.startswith("CMACRO") or
                   (t.kind == "IDENTIFIER" and t.text.isupper() and
                    t.text not in IR_HELPER_EFFECTS) for t in body):
                raise analysis_error("home loop cannot bypass its per-home operation", stmt.for_)
            loops[id(stmt)] = (index, extent, guards)
            contexts.append((stmt.header, guards))
            collect(stmt.body, guards + ((id(stmt), ("for", index, *extent)),))
        else:
            raise analysis_error("home adapter requires checked statement control", markers[0])

    collect(part.body)
    if any(t.kind.startswith("CMACRO") for t in part.body.tokens()):
        raise analysis_error("home adapter cannot cross preprocessor control", markers[0])

    def name(arg):
        return arg[0].text if len(arg) == 1 and arg[0].kind == "IDENTIFIER" else None

    def in_bounds(item, index, guards):
        if isinstance(index, str):
            return any(scope in loops and condition == ("for", index, item.size)
                       for scope, condition in guards)
        if item.size.isdigit():
            return index < int(item.size)
        return (item.size == "oparg & 1" and index == 0 and
                any(condition == ("(", "oparg", "&", "1", ")")
                    for _, condition in guards))

    used: dict[str, StackItem] = {}
    covered: set[int] = set()
    operations: dict[int, tuple[str, int | str]] = {}
    order = {id(token): at for at, token in enumerate(part.body.tokens())}
    writes: list[tuple[str, int | str, tuple[tuple[int, tuple[str, ...]], ...], int]] = []
    loop_operations: dict[int, int] = {}
    taken: set[str] = set()
    for tokens, guards in contexts:
        iterator = TokenIterator(tokens)
        for token in iterator:
            if token.text not in IR_HELPER_EFFECTS:
                continue
            args, end = instruction_macro_args(token, iterator, IR_HELPER_EFFECTS)
            if token.text == "IR_WRITE_OUTPUT":
                statement = [t for t in tokens if t.kind != "COMMENT"]
                if (statement[0] is not token or statement[-1].text != ";" or
                        statement[-2] is not end):
                    raise analysis_error("output home write must be a statement", token)
                array = name(args[0]) if len(args) == 3 else None
                item = outputs.get(array)
                index = (int(args[1][0].text) if len(args) == 3 and
                         len(args[1]) == 1 and args[1][0].text.isdigit() else
                         name(args[1]) if len(args) == 3 else None)
                if item is None or not item.is_array() or index is None or not in_bounds(item, index, guards):
                    raise analysis_error("output home needs a declared array and proved index", token)
                if any(previous == array and (old == index or isinstance(old, str) or
                       isinstance(index, str)) for previous, old, _, _ in writes):
                    raise analysis_error("output home cannot be published twice", token)
                covered.add(id(args[0][0]))
                writes.append((array, index, guards, order[id(end)]))
            elif token.text == "IR_UNPACK_ITEMS":
                statement = [t for t in tokens if t.kind != "COMMENT"]
                if (statement[0] is not token or statement[-1].text != ";" or
                        statement[-2] is not end):
                    raise analysis_error("unpack items must be a statement", token)
                array = name(args[0]) if len(args) == 3 else None
                item = outputs.get(array)
                if (item is None or not item.is_array() or item.size != "oparg" or
                        guards or [t.text for t in args[2]] != ["oparg"] or
                        any(previous == array for previous, _, _, _ in writes)):
                    raise analysis_error(
                        "unpack items needs the whole output array once, unguarded", token)
                covered.add(id(args[0][0]))
                writes.append((array, 0, guards, order[id(end)]))
                index = 0
            elif token.text == "IR_TAKE_INPUT":
                array = name(args[0]) if len(args) == 2 else None
                item = inputs.get(array)
                index = name(args[1]) if len(args) == 2 else None
                if (item is None or item.peek or not item.is_array() or
                        index is None or not in_bounds(item, index, guards) or array in taken):
                    raise analysis_error("input take needs its unique index-matched owning array", token)
                taken.add(array)
                covered.add(id(args[0][0]))
            elif token.text == "IR_CLOSE_INPUTS":
                array = name(args[0]) if len(args) == 1 else None
                item = inputs.get(array)
                statement = [t for t in tokens if t.kind != "COMMENT"]
                if (item is None or item.peek or item.size != "oparg" or
                        len(inputs) != 1 or outputs or guards or
                        len(part.body.body) != 1 or
                        [t.text for t in statement] !=
                            ["IR_CLOSE_INPUTS", "(", array, ")", ";"]):
                    raise analysis_error("home drain needs its sole owning array and unconditional handoff", token)
                index = 0
                covered.add(id(args[0][0]))
            elif token.text in {"IR_BUILD_STRING", "IR_BUILD_MAP", "IR_BUILD_SET",
                                *STEAL_SUCCESS_HOME_HELPERS}:
                array = name(args[0]) if len(args) == 1 else None
                item = inputs.get(array)
                extent = "oparg*2" if token.text == "IR_BUILD_MAP" else "oparg"
                if (item is None or item.peek or not item.is_array() or
                        item.size != extent or len(inputs) != 1):
                    raise analysis_error("builder needs its sole declared owning array", token)
                if token.text in STEAL_SUCCESS_HOME_HELPERS and not steal_success_home_contract(part):
                    raise analysis_error("steal-success helper needs its checked success handoff", token)
                index = 0
                covered.add(id(args[0][0]))
            else:
                array = name(args[1]) if len(args) == 2 else None
                item = inputs.get(array)
                output = outputs.get(array)
                if (item is None or not item.peek or item.size != "2" or
                        output is None or not output.peek or output.size != "2" or
                        len(inputs) != 1 or len(outputs) != 1 or name(args[0]) is None):
                    raise analysis_error("special lookup needs its declared two-home through pair", token)
                index = 0
                covered.add(id(args[1][0]))
            assert array is not None and item is not None
            for scope, condition in guards:
                if scope in loops:
                    counter, extent, outer = loops[scope]
                    if (index != counter or item.size != " ".join(extent) or
                            guards != outer + ((scope, condition),)):
                        raise analysis_error("every home iteration must execute its index-matched operation", token)
                    loop_operations[scope] = loop_operations.get(scope, 0) + 1
            used[array] = item
            operations[id(token)] = (array, index)
    if any(loop_operations.get(scope) != 1 for scope in loops):
        raise analysis_error("home loop must contain one operation on every normal iteration", markers[0])

    # A declared array's generated pointer cannot be copied, rebound, passed
    # elsewhere or used as a raw write target after acquiring a capability.
    # Raw reads are complete by-value arguments of these existing typed
    # readers, never a member lvalue or an address hidden in parentheses.
    readers = {"PyValue_IsNull": (0, 1), "PyValue_TYPE": (0, 1)}
    reads: set[int] = set()
    if "oparg" in declared_locals(list(part.body.tokens())):
        raise analysis_error("home extent cannot be shadowed", markers[0])
    for tokens, guards in contexts:
        for at, token in enumerate(tokens):
            if token.kind == "AND" and is_unary_and(tokens, at):
                raise analysis_error("home adapter cannot expose a raw address", token)
            if token.text not in readers or (at and tokens[at - 1].text in {".", "->"}):
                continue
            args, _ = instruction_macro_args(token, TokenIterator(tokens[at + 1:]))
            position, count = readers[token.text]
            if len(args) != count:
                continue
            arg = args[position]
            if (len(arg) == 4 and arg[0].text in used and arg[1].text == "[" and
                    arg[2].text.isdigit() and arg[3].text == "]"):
                reads.add(id(arg[0]))
    for tokens, guards in contexts:
        protected = {"oparg", *(loops[scope][0] for scope, _ in guards if scope in loops)}
        if protected.intersection(declared_locals(tokens)):
            raise analysis_error("home loop extent or index cannot be shadowed", tokens[0])
        for at, token in enumerate(tokens):
            if token.text in protected:
                before, after = at - 1, at + 1
                while before >= 0 and tokens[before].kind == "LPAREN":
                    before -= 1
                while after < len(tokens) and tokens[after].kind == "RPAREN":
                    after += 1
                if ((before >= 0 and tokens[before].kind in ASSIGNMENTS) or
                        (after < len(tokens) and tokens[after].kind in ASSIGNMENTS)):
                    raise analysis_error("home extent cannot be mutated or exposed", token)
            if token.text not in used or id(token) in covered:
                continue
            if (at >= 2 and tokens[at - 2].text == "DEAD" and
                    tokens[at - 1].text == "(" and at + 1 < len(tokens) and
                    tokens[at + 1].text == ")"):
                continue
            if (id(token) in reads and
                    in_bounds(used[token.text], int(tokens[at + 2].text), guards)):
                index = int(tokens[at + 2].text)
                if token.text in inputs or any(
                        array == token.text and written == index and
                        before < order[id(token)] and all(guard in guards for guard in path)
                        for array, written, path, before in writes):
                    continue
            raise analysis_error("home array escapes its checked adapter", token)
    return operations


def normal_observer_frame_footprint(part: Uop) -> bool:
    """Explain every frame use by an audited observer-call argument."""
    source = canonical_body_tokens(part)
    if not source or any(text.lstrip().startswith("#") for text in source):
        return False
    reserved = {"frame", *NORMAL_FRAME_OBSERVERS}
    if any(item.name in reserved for item in
           (*part.stack.inputs, *part.stack.outputs, *part.caches)):
        return False
    tokens = [t for t in part.body.tokens() if t.kind != "COMMENT"]
    covered: set[int] = set()
    for at, token in enumerate(tokens):
        if token.text not in NORMAL_FRAME_OBSERVERS:
            continue
        if (at + 1 >= len(tokens) or tokens[at + 1].text != "("
                or at > 0 and tokens[at - 1].text in {".", "->"}):
            return False
        args: list[list[int]] = [[]]
        depth = 0
        for end in range(at + 2, len(tokens)):
            text = tokens[end].text
            if text == ")" and depth == 0:
                break
            if text == "," and depth == 0:
                args.append([])
                continue
            if text in {"(", "[", "{"}:
                depth += 1
            elif text in {")", "]", "}"}:
                depth -= 1
            args[-1].append(end)
        else:
            return False
        prefix = [[tokens[i].text for i in arg] for arg in args[:4]]
        if (len(args) != 6 or prefix[0] != ["tstate"]
                or len(prefix[1]) != 1
                or prefix[1][0] not in NORMAL_FRAME_OBSERVERS[token.text]
                or prefix[2:] != [["frame"], ["this_instr"]]):
            return False
        covered.add(args[2][0])
    return all(token.text != "frame" or at in covered
               for at, token in enumerate(tokens))


def without_guard_reason(text: list[str]) -> list[str]:
    """An EXIT_IF or DEOPT_IF statement's tokens without its reason argument."""
    if text[:2] not in (["EXIT_IF", "("], ["DEOPT_IF", "("]) or text[-2:] != [")", ";"]:
        return text
    depth = 0
    for at, token in enumerate(text[1:-2], 1):
        depth += token == "("
        depth -= token == ")"
        if depth == 1 and token == ",":
            return text[:at] + text[-2:]
    return text


def disjoint_child_transport(inst: Instruction, fmt: IRFormat) -> Uop | None:
    """A scalar input moved to an independently reserved returning child."""
    parts = [part for part in inst.parts if isinstance(part, Uop)]
    if (any(not isinstance(part, (Uop, Skip)) for part in inst.parts)
            or len(parts) < 3 or fmt.window or fmt.n_inputs != 1
            or store_mode(inst, external_io(inst)[1]) != STORE_RETURN
            or fmt.field_of(ROLE_DST) is None):
        return None
    producer, recipient, publish = parts[-3:]
    if ([(i.name, i.peek, i.size) for i in producer.stack.inputs] != [("owner", False, "")]
            or [(i.name, i.peek, i.size) for i in producer.stack.outputs] != [("new_frame", False, "")]
            or producer.properties.escapes or producer.properties.eval_breaker
            or producer.properties.switches_frame):
        return None
    saved_recipient = """{
        frame->return_slot = (uint16_t)IR_DST_SLOT;
        frame->return_pc = next_instr;
    }"""
    publication = """{
        assert(!IS_PEP523_HOOKED(tstate));
        _PyInterpreterFrame *temp = PyValue_Unwrap(new_frame);
        DEAD(new_frame);
        SAVE_STACK();
        assert(temp->previous == frame || temp->previous->previous == frame);
        CALL_STAT_INC(inlined_py_calls);
        frame = tstate->current_frame = temp;
        tstate->py_recursion_remaining--;
        RELOAD_STACK();
        LOAD_IP(0);
        IR_SKIP_RESUME();
        DTRACE_FUNCTION_ENTRY();
        LLTRACE_RESUME_FRAME();
    }"""
    if (statement_shape(recipient.body) != statement_shape(
            Parser(saved_recipient, "<child recipient>").block())
            or statement_shape(publish.body) != statement_shape(
                Parser(publication, "<child publication>").block())
            or recipient.stack.inputs or recipient.stack.outputs
            or [(i.name, i.peek, i.size) for i in publish.stack.inputs] != [("new_frame", False, "")]
            or publish.stack.outputs):
        return None
    guard_bodies = ("""{
        PyTypeObject *tp = Py_TYPE(PyValue_AsKnownObjectBorrow(owner));
        assert(type_version != 0);
        EXIT_IF(FT_ATOMIC_LOAD_UINT_RELAXED(tp->tp_version_tag) != type_version);
    }""", "{ DEOPT_IF(IS_PEP523_HOOKED(tstate)); }")
    if any(not guard.is_guard() or
           not any(statement_shape(guard.body) == statement_shape(
                       Parser(body, "<child guard>").block())
                   for body in guard_bodies)
           for guard in parts[:-3]):
        return None
    body = producer.body.body
    if not all(isinstance(stmt, SimpleStmt) for stmt in body):
        return None
    statements = [_statement_text(stmt) for stmt in body]
    constructors = [at for at, text in enumerate(statements)
                    if "_PyFrame_PushUnchecked" in text]
    if len(constructors) != 1:
        return None
    at = constructors[0]
    constructor = statements[at]
    prefix = ["_PyInterpreterFrame", "*", "pushed_frame", "=",
              "_PyFrame_PushUnchecked", "(", "tstate", ",",
              "PyValue_DUP", "(", "PyValue_FromObjectBorrow", "("]
    suffix = [")", ")", ",", "frame", ")", ";"]
    if (len(constructor) != len(prefix) + 1 + len(suffix)
            or constructor[:len(prefix)] != prefix
            or constructor[-len(suffix):] != suffix):
        return None
    tail = statements[at + 1:]
    move = [t.text for t in tokenize("pushed_frame->base[0] = TAKE(owner);")]
    wrap = [t.text for t in tokenize("new_frame = PyValue_Wrap(pushed_frame);")]
    if len(tail) not in (2, 3) or tail[0] != move or tail[-1] != wrap:
        return None
    if len(tail) == 3:
        # An independent immutable name argument; never another parent home.
        extra = tail[1]
        begin = [t.text for t in tokenize(
            "pushed_frame->base[1] = PyValue_DUP(PyValue_FromObjectBorrow(")]
        end = [")", ")", ";"]
        if (len(extra) != len(begin) + 1 + len(end)
                or extra[:len(begin)] != begin or extra[-len(end):] != end):
            return None
    # A guard's reason documents its condition and does not change the match
    before = [without_guard_reason(text) for text in statements[:at]]
    # Parameterize only the captured function value, not arbitrary C effects.
    # Shared guard/publication contracts and these read-only setup statements
    # exclude aliases that could write the parent's otherwise untouched homes.
    type_assert = [t.text for t in tokenize("assert(Py_IS_TYPE(descriptor, &PyFunction_Type));")]
    descriptors = [text[4] for text in before
                   if len(text) == len(type_assert) and text[:4] == type_assert[:4]
                   and text[5:] == type_assert[5:]]
    if len(descriptors) != 1:
        return None
    descriptor = descriptors[0]
    if constructor[len(prefix)] not in {descriptor, "f"}:
        return None
    required = [
        f"assert(Py_IS_TYPE({descriptor}, &PyFunction_Type));",
        f"PyFunctionObject *f = (PyFunctionObject *){descriptor};",
        "EXIT_IF(f->func_version != func_version);",
        "PyCodeObject *code = (PyCodeObject *)f->func_code;",
        "EXIT_IF(!_PyThreadState_HasStackSpace(tstate, code->co_framesize));",
    ]
    optional = ["assert((oparg & 1) == 0);", "assert(func_version != 0);",
                "assert(code->co_argcount == 2);", "STAT_INC(LOAD_ATTR, hit);",
                "PyObject *name = GETITEM(FRAME_CO_NAMES, oparg >> 1);"]
    required_tokens = [[t.text for t in tokenize(text)] for text in required]
    allowed = required_tokens + [[t.text for t in tokenize(text)] for text in optional]
    if (any(text not in before for text in required_tokens)
            or any(text not in allowed for text in before)):
        return None
    if len(tail) == 3 and tail[1][len(begin)] != "name":
        return None
    return publish


def operand_error_sources(inst: Instruction) -> frozenset[int] | None:
    """Only encoded transport can destroy incoming scratch absence.

    Native callbacks may vacate other scratch homes through a redirect. That
    preserves the NO_HOME fact imported by this absence-only capability.
    """
    return operand_errors_from_summary(analyze_error_effects(inst, HANDLER_LOCALS))


def operand_errors_from_summary(source) -> frozenset[int] | None:
    if (source is None or source.homes == HomeEffect.UNKNOWN
            or not source.error_sources):
        return None
    return source.error_sources


def async_exit_transport(inst: Instruction, fmt: IRFormat) -> bool:
    """Only the matched-exception arm reaches a drained normal successor."""
    parts = [part for part in inst.parts if isinstance(part, Uop)]
    if (len(parts) not in (1, 2) or len(parts) != len(inst.parts)
            or not fmt.window or not fmt.names_target or fmt.n_inputs
            or fmt.roles != [ROLE_BASE, ROLE_NONE, ROLE_IMM, ROLE_IMM]
            or fmt.imm != LOC_CD or fmt.store_dst):
        return False
    producer = parts[-1]
    if ([(i.name, i.peek, i.size) for i in producer.stack.inputs] !=
            [("awaitable_st", False, ""), ("exc_st", False, "")]
            or producer.stack.outputs):
        return False
    expected = """{
        (void)oparg;
        assert(PyValue_IsObject(exc_st));
        PyObject *exc = PyValue_AsKnownObjectBorrow(exc_st);
        assert(exc && PyExceptionInstance_Check(exc));
        int matches = PyErr_GivenExceptionMatches(exc, PyExc_StopAsyncIteration);
        if (matches) {
            DECREF_INPUTS();
        }
        else {
            _PyErr_SetRaisedException(tstate, Py_NewRef(exc));
            monitor_reraise(tstate, frame, this_instr);
            goto exception_unwind;
        }
        JUMPBY(0);
    }"""
    if statement_shape(producer.body) != statement_shape(
            Parser(expected, "<async exit transport>").block()):
        return False
    if len(parts) == 1:
        return True
    observer = parts[0]
    expected = """{
        assert(_PyIR_BaseOpcode(_PyIR_RealHandler(frame, JUMP_TARGET(oparg))) == SEND);
        INSTRUMENTED_JUMP(JUMP_TARGET(oparg), this_instr+1, PY_MONITORING_EVENT_BRANCH_RIGHT);
    }"""
    return (not observer.stack.inputs and not observer.stack.outputs
            and statement_shape(observer.body) == statement_shape(
                Parser(expected, "<async exit observer>").block()))


def normal_vacate_capability(inst: Instruction, fmt: IRFormat) -> bool:
    """Conditions shared by actual final commits, never a source-name grant."""
    if async_exit_transport(inst, fmt):
        return True
    parts = [part for part in inst.parts if isinstance(part, Uop)]
    reserved = {"frame", *NORMAL_FRAME_OBSERVERS}
    steals = [part for part in parts if any(
        t.text in STEAL_SUCCESS_HOME_HELPERS for t in part.body.tokens())]
    return (not steals or (len(steals) == 1 and inst.parts == steals
                          and steal_success_home_contract(steals[0]))) and (
            not inst.properties.switches_frame and not fmt.names_target
            and handler_output_kind(inst, fmt) != "_PyIR_OUTPUT_OUTCOME"
            and not any(item.name in reserved for part in parts for item in
                        (*part.stack.inputs, *part.stack.outputs, *part.caches))
            and not any(t.kind.startswith("CMACRO") and
                        (any(name in t.text for name in reserved) or
                         "_PyFrame_" in t.text or "_PyHomeBitmap_" in t.text)
                        for t in uop_token_list(inst))
            and not any(t.text.startswith(("_PyFrame_", "_PyHomeBitmap_"))
                        for t in uop_token_list(inst))
            and not any(t.text in {"slots", "window", "DISPATCH", "DISPATCH_REDIRECT",
                                  "DISPATCH_INLINED", "IR_COMPLETE_STEP"}
                        for t in uop_token_list(inst))
            and all(not any(t.text == "frame" or t.text in NORMAL_FRAME_OBSERVERS
                            for t in part.body.tokens())
                    or normal_observer_frame_footprint(part)
                    for part in inst.parts if isinstance(part, Uop)))


def emit_final_outputs(inst, fmt, mode, stack: IRStack, out: CWriter) -> int | None:
    """The physical final publication shared by ordinary and observed paths."""
    stack.final = True
    stack.flush(out)
    if mode == STORE_RESULT and fmt.window and fmt.operands == "_PyIR_OPERANDS_NONE":
        if stack.moved_dst:
            return fmt.dst_offset
        dst = FIELD_ARGS[fmt.field_of(ROLE_DST)]
        expression = f"IR_MOVE_TO_DST({dst}, window[{fmt.dst_offset}])"
        stack.dst_stores.append(expression)
        out.emit(f"{expression};\n")
        return fmt.dst_offset
    return None


def generate_handler(name: str, inst: Instruction, fmt: IRFormat,
                     emitter: IREmitter, out: CWriter) -> None:
    """The handler name, which runs inst: inst itself, or for a handler of a
    BINARY_OP operation, BINARY_OP with that operation.

    The handler is generated twice.  The first pass, to no output, finds
    the role of each C variable, a view or an owner, and the object
    locals; the second declares them, a view as a PyValueBorrow (S8).  A
    name with both roles is refused: the DSL gives the owner another name,
    as `taken = TAKE(value)` does."""
    emitter.execution_variant = EXECUTION_VARIANTS.get(name)
    boundary = generator_boundaries(emitter.analysis).get(name)
    if boundary is not None:
        if fmt.group == "SEND" and send_source_contract(inst, fmt) is None:
            raise analysis_error("generator boundary needs the complete SEND source contract", inst.where)
        if boundary.resume != "NONE" and not (
                canonical_frame_transfer_input(inst) or abrupt_suspend_contract(inst, fmt)
                or metadata_only_passthroughs(inst) == frozenset({1})):
            raise analysis_error("generator boundary needs checked yield transport", inst.where)
    parts = [part for part in inst.parts if isinstance(part, Uop)]
    if any(token.text in ELEMENT_INTRINSICS
           for part in parts for token in part.body.tokens()):
        # These native identifiers belong to the checked lowering. A source
        # alias or preprocessor definition must not replace its operations.
        for part in parts:
            body = canonical_body_tokens(part)
            names = (item.name for item in
                     (*part.stack.inputs, *part.stack.outputs, *part.caches))
            if (not body or ELEMENT_NATIVE_SYMBOLS.intersection(names) or
                    any(text in ELEMENT_NATIVE_SYMBOLS or text.startswith("#") for text in body)):
                raise analysis_error("element lowering requires unshadowed native operations", inst.where)
    validate_dict_first_probe(inst, fmt)
    validate_guarded_python_call(inst, fmt)
    emitter.result_publication = result_publication_contract(inst, fmt)
    record = build_record(name, inst, fmt, emitter.analysis, emitter.formats)
    emitter.pin_at = pin_point(name, inst, emitter.analysis)
    saved = emitter.out
    roles: dict[str, set[str]] = {}
    emitter.declared_roles = {}
    emitter.close_names = set()
    emitter.declared_closes = set()
    emitter.out = CWriter.null()
    try:
        emit_handler(record, emitter, emitter.out, roles)
    finally:
        emitter.out = saved
    for var, kinds in sorted(roles.items()):
        if len(kinds) > 1:
            raise analysis_error(
                f"{name}: '{var}' is a view (PyValueBorrow) and an owner "
                f"(PyValue) in one handler; give one of them another name",
                inst.where)
    emitter.declared_roles = roles
    emitter.declared_closes = set(emitter.close_names)
    emit_handler(record, emitter, out, {})
    if emitter.pin_at is not None and not emitter.pinned:
        raise analysis_error(f"{name}: departs its frame, and the emitter printed no "
                             "IR_PIN_DEPARTING() before its departing uop", inst.where)
    emitter.pin_at = None
    if record.generator_boundary is not None:
        check_boundary_expansion(name, record.generator_boundary, record,
                                 emitter.boundary_reached,
                                 emitter.boundary_completions, inst.where)
    check_record(record, [p for p in inst.parts if isinstance(p, Uop)])
    ir_leaf.check_leaf(record, [p for p in inst.parts if isinstance(p, Uop)])


def arm_departs(inst: Instruction, boundary: GeneratorBoundary | None) -> bool:
    """Whether the arm departs its frame by a return or a yield: one of its
    uops declares the exit, or its boundary(...) clause does (a delegated
    yield, or a consumer whose completion returns).  A variant that departs
    at(enter) leaves through the frame that it enters, whose arms pin."""
    if any(ir_contract.declared_contract(part).exits & ir_contract.DEPARTING_EXITS
           for part in inst.parts if isinstance(part, Uop)):
        return True
    if boundary is None or boundary.at == "enter":
        return False
    return boundary.yielded != "NONE" or "return" in boundary.exits


def pin_point(name: str, inst: Instruction, analysis: Analysis) -> int | None:
    """The uop of handler `name` before whose body the arm pins the code of
    the frame it departs (IR_PIN_DEPARTING(), g3/design.md section 2): the
    uop after its last guard, the last uop that declares miss or deopt.
    None if no arm that runs this body departs; a body that generator-
    boundary variants share (generator_word_entry()) pins if one of them
    departs.  VT4 keeps every guard before the departing uop."""
    boundaries = generator_boundaries(analysis)
    insts = all_instructions(analysis)
    sharing = {name} | {n for n in boundaries if generator_word_entry(n, analysis) == name}
    if not any(arm_departs(insts[n] if n in insts else inst, boundaries.get(n))
               for n in sharing):
        return None
    parts = [part for part in inst.parts if isinstance(part, Uop)]
    guards = [i for i, part in enumerate(parts)
              if ir_contract.declared_contract(part).exits & ir_contract.GUARD_EXITS]
    at = guards[-1] + 1 if guards else 0
    if at == len(parts):
        raise analysis_error(f"{name}: departs its frame, and its last uop guards: no uop "
                             "after its guards can take the pin", inst.where)
    return at


def canonical_body_tokens(part: Uop) -> list[str]:
    "Read the complete source body, including preprocessor arm delimiters."
    context = part.context
    tokens = context.owner.tokens[context.begin:context.end]
    begin = tokens.index(part.body.open)
    end = tokens.index(part.body.close, begin)
    tokens = tokens[begin:end + 1]
    # Emission reads the mutable AST. Reparse the original token objects so
    # source delimiters cannot certify a body changed after analysis.
    reader = Parser("", filename=context.owner.filename)
    reader.tokens = tokens
    if reader.block() != part.body:
        return []
    return [token.text for token in tokens
            if token.kind != "COMMENT"]



def canonical_frame_transfer_input(inst: Instruction) -> bool:
    """A yield transfers a canonical home owner unchanged.

    Fail closed to ordinary publication if the checked uop chain changes.
    Native/SEND results do not acquire this fact from their handler name.
    """
    if any(not isinstance(part, (Uop, Skip)) for part in inst.parts):
        return False
    parts = [p for p in inst.parts if isinstance(p, Uop)]
    if any(cache.name != "unused" for part in parts for cache in part.caches):
        return False
    if len(parts) == 2:
        prefix, transfer = parts
        shapes = ((("value",), ("taken",)), (("retval",), ("value",)))
        if all((tuple(item.name for item in part.stack.inputs),
                tuple(item.name for item in part.stack.outputs)) == shape
               for part, shape in zip(parts, shapes)):
            expected_yield = """{
                assert(frame->owner != FRAME_OWNED_BY_INTERPRETER);
                frame->instr_ptr++;
                PyGenObject *gen = _PyGen_GetGeneratorFromFrame(frame);
                assert(FRAME_SUSPENDED_YIELD_FROM == FRAME_SUSPENDED + 1);
                assert(oparg == 0 || oparg == 1);
                PyValue temp = retval;
                DEAD(retval);
                SAVE_STACK();
                DTRACE_FUNCTION_RETURN();
                tstate->exc_info = gen->gi_exc_state.previous_item;
                gen->gi_exc_state.previous_item = NULL;
                _Py_LeaveRecursiveCallPy(tstate);
                _PyInterpreterFrame *gen_frame = frame;
                gen_frame->return_slot = (uint16_t)IR_DST_SLOT;
                _PyThreadState_UpdateLastProfiledFrame(tstate, gen_frame, gen_frame->previous);
                frame = tstate->current_frame = frame->previous;
                gen_frame->previous = NULL;
                ((_PyThreadStateImpl *)tstate)->generator_return_kind = GENERATOR_YIELD;
                FT_ATOMIC_STORE_INT8_RELEASE(gen->gi_frame_state, FRAME_SUSPENDED + oparg);
                #ifdef Py_DEBUG
                if (frame->f_executable != NULL) {
                    Py_ssize_t i = INSTR_OFFSET_OF(frame->instr_ptr);
                    assert(i >= 0 && i <= INT_MAX);
                    int opcode = _PyIR_BaseOpcode(
                        _PyCode_InstrHandler(_PyFrame_GetCode(frame), (int)i));
                    assert(opcode == SEND || opcode == FOR_ITER);
                }
                #endif
                RELOAD_STACK();
                next_instr = frame->return_pc;
                value = temp;
                LLTRACE_RESUME_FRAME();
            }"""
            if (not any(item.is_array() for part in parts
                        for item in (*part.stack.inputs, *part.stack.outputs))
                    and canonical_body_tokens(prefix)
                    == [token.text for token in tokenize(
                        "{ taken = TAKE(value); }")]
                    and canonical_body_tokens(transfer)
                    == [token.text for token in tokenize(expected_yield)]):
                return True
    return False


def current_operation_preserves_scratch(inst: Instruction, fmt: IRFormat, source) -> bool:
    """A complete plain value operation whose callbacks cannot redirect it."""
    # The setter requires a matching current observer, or an active CALL
    # ancestor recipe. A source/step/frame-switch adapter supplies neither
    # proof merely by calling one of the same native helpers.
    return bool(
        source is not None and source.callbacks_without_current_observer and
        source.homes in {HomeEffect.NO_ARENA_WRITE, HomeEffect.PRESERVES_CANONICAL_OR_NULL} and
        not source.changes_origin and not fmt.window and
        not inst.properties.switches_frame)


def emit_handler(record: OperationRecord, emitter: IREmitter, out: CWriter,
                 roles: dict[str, set[str]]) -> None:
    """One pass of generate_handler, which records the roles it finds.  It
    prints the handler from its record, and checks that it printed every
    element of the record that has a site in the body"""
    name, inst, fmt = record.handler, record.instruction, record.format
    emitter.record = record
    emitter.record_consumed = set()
    emitter.record_sites = {id(site.site) for uop in record.uops
                            for site in (*uop.guards, *uop.transfers)
                            if site.site is not None}
    emitter.uop_index = 0
    emitter.pinned = False
    emitter.generator_boundary = record.generator_boundary
    emitter.boundary_reached = set()
    emitter.boundary_completions = []
    emitter.pending_owner = None
    emitter.close_count = 0
    emitter.shape_read_guards = 0
    emitter.shape_read_uses = 0
    emitter.ownership_events = []
    emitter.normal_inputs_cleared = False
    emitter.abrupt_prefix = False
    emitter.send_source_parts = send_source_contract(inst, fmt)
    emitter.error_source = analyze_error_effects(inst, HANDLER_LOCALS)
    if emitter.error_source is None and emitter.send_source_parts is not None:
        emitter.error_source = checked_native_step_effects(emitter.send_source_parts)
    source = emitter.error_source
    emitter.callback_current_identity = current_operation_preserves_scratch(inst, fmt, source)
    emitter.abrupt_operand_expected = operand_errors_from_summary(emitter.error_source)
    emitter.abrupt_operand_sources = set()
    emitter.abrupt_operand_origin = None
    emitter.abrupt_operand_uncovered = False
    emitter.error_unwind_sources = set()
    emitter.error_terminal_sources = set()
    emitter.error_uncovered = False
    emitter.error_finalized = False
    emitter.error_local_dst = False
    emitter.error_redispatch_origin = None
    emitter.error_checkpoint_sites = {}
    emitter.error_checkpoint_count = 0
    emitter.observer_source = observer_entry_source(inst, fmt)
    emitter.observer_errors = set()
    emitter.observer_terminals = set()
    emitter.observer_calls = set()
    emitter.disjoint_child_publication = disjoint_child_transport(inst, fmt)
    emitter.disjoint_child_drained = False
    emitter.normal_result_offsets = set()
    emitter.normal_null_receipts = []
    emitter.null_outputs = null_output_names(inst, fmt)
    emitter.send_yielded_published = False
    emitter.native_window_out = False
    emitter.unpack_local_drained = False
    emitter.normal_observer_edges = set()
    emitter.normal_completions = set()
    emitter.normal_completion_uncovered = False
    emitter.call_normal_drained = False
    emitter.call_kind = ""
    emitter.taken_outputs = set()
    operation = record.operation
    transaction = uses_generic_call(inst)
    emitter.call_finished = False
    is_target = inst.is_target and name == inst.name
    out.emit("\n")
    if emitter.shared_binary_op:
        out.emit("LABEL(binary_op) {\n")
        handler_id = "_PyIR_BINARY_OP_FIRST + binary_op_operation"
    else:
        out.emit(f"TARGET({name}) {{\n")
        handler_id = canonical_handler(name)
    out.emit(f"int opcode = {handler_id};\n")
    out.emit(f"(void)(opcode);\n")
    if inst.properties.needs_prev:
        out.emit(f"_PyIRWord* const prev_instr = frame->instr_ptr;\n")
    if not is_target:
        qualifier = "" if transaction or emitter.result_publication is not None or inst.name == "BINARY_OP" or canonical_handler(name) in ("CALL_KW", "INSTRUMENTED_CALL_KW", "CALL_KW_NON_PY", "LOAD_ATTR_SLOT",
                                   "BUILD_MAP", "STORE_SUBSCR",
                                   "LOAD_GLOBAL",
                                   "LOAD_ATTR", "STORE_ATTR",
                                   "LOAD_ATTR_INSTANCE_VALUE", "LOAD_ATTR_SHAPE",
                                   "LOAD_ATTR_MODULE", "LOAD_ATTR_WITH_HINT") else "const "
        out.emit(f"_PyIRWord* {qualifier}this_instr = next_instr;\n")
        out.emit("(void)this_instr;\n")
    if not inst.properties.no_save_ip:
        out.emit(f"frame->instr_ptr = next_instr;\n")
    out.emit("IR_HOLD_NEXT();\n")
    # A call is a head and its operand words
    out.emit("next_instr += IR_CALL_LENGTH(ir_word);\n" if fmt.operands != "_PyIR_OPERANDS_NONE"
             else "next_instr += 1;\n")
    out.emit(f"INSTRUCTION_STATS({handler_id});\n")
    if is_target:
        qualifier = "" if transaction or emitter.result_publication is not None or inst.name == "BINARY_OP" or canonical_handler(name) in ("CALL", "CALL_KW", "STORE_SUBSCR", "LOAD_GLOBAL", "LOAD_ATTR", "STORE_ATTR") else "const "
        back = ("IR_CALL_LENGTH(ir_word)" if fmt.operands != "_PyIR_OPERANDS_NONE"
                else "1")
        out.emit(f"_PyIRWord* {qualifier}this_instr = next_instr - {back};\n")
        out.emit("(void)this_instr;\n")
    if fmt.store_dst or fmt.window:
        # The old value of the slot that dst names, a local or the discard
        # slot, which the releasing store hands to the handler: it closes it
        # after its last store (IR_STORE_DST)
        out.emit("PyValue ir_old = PyValue_NULL;\n")
        out.emit("(void)ir_old;\n")
    if unpacks_elements_directly(inst, fmt):
        out.emit("bool ir_unpack_owned = false;\n")
    use_fb = record.uses_feedback
    emitter.call_has_feedback = use_fb
    if use_fb:
        out.emit(f"{feedback_type(fmt)} *fb = ({feedback_type(fmt)} *)"
                 f"IR_FB({field_expr(fmt.fb)});\n")
    if record.oparg:
        if record.oparg == "OPERATION":
            out.emit("int oparg = binary_op_operation;\n" if emitter.shared_binary_op
                     else f"int oparg = {operation};\n")
        elif record.oparg == "FUSED_OP":
            # D is the distance; the arg of the entry holds the operation
            out.emit("int oparg = (int)(((_PyFbHeader *)fb)->arg & IR_FUSED_OP_MASK);\n")
        elif record.oparg == "FBARG_ENTRY":
            out.emit("int oparg = (int)((_PyFbHeader *)fb)->arg;\n")
        elif record.oparg == "FBARG":
            out.emit(f"int oparg = IR_FB_ARG(fb, {field_expr(fmt.fb)});\n")
        elif record.oparg == "SIGNED_D":
            # The distance has a sign, as _PyCode_InstrOparg() decodes it
            out.emit("int oparg = IR_DS(ir_word);\n")
        elif record.oparg == "CALL_ARGC":
            # C is the number of operands; the DSL's oparg counts the
            # arguments, among them the keyword values of a CALL_KW
            out.emit("int oparg = (int)IR_C(ir_word)"
                     + (" - IR_CALL_HAS_SELF(ir_word)" if fmt.operands != "_PyIR_OPERANDS_EX" else "")
                     + (" - 1" if fmt.operands == "_PyIR_OPERANDS_KW" else "")
                     + ";\n")
        else:
            out.emit(f"int oparg = (int){field_expr(fmt.imm)};\n")
        out.emit("(void)oparg;\n")
    if fmt.window and fmt.operands == "_PyIR_OPERANDS_NONE":
        base = FIELD_VARS[fmt.field_of(ROLE_BASE)]
        out.emit(f"PyValue *window = &IR_SLOT({base});\n")
        out.emit("(void)window;\n")
    call_window = record.call is not None
    if call_window:
        kind = record.call.kind
        emitter.call_argc = record.call.argc
        emitter.call_kind = kind
        emitter.call_run = record.call.run
        emitter.call_flags = record.call.flags
        # What the call is, from its word: whether it has a self operand (or
        # a kwargs operand), and what the handler did before the call
        # (call_state, in a register).  The operands are the words after the
        # head.
        # The local is the STATE row's (pycore_ir_components.h), initialized
        # as the row declares; CALL_FUNCTION_EX's holds its kwargs operand
        # instead, which no uop's state(...) clause reads
        state = ir_contract.rows()["call_state"]
        init = "IR_CALL_KWARGS_STATE" if kind == "CALL_EX" else state.init
        if kind == "CALL_EX" and any(
                ir_contract.declared_contract(part).state_item(state.name)
                for part in inst.parts if isinstance(part, Uop)):
            raise analysis_error(f"{name}: a uop declares state({state.name}), whose "
                                 "facts are the STATE row's, but CALL_FUNCTION_EX's "
                                 f"{state.name} holds its kwargs operand", inst.where)
        out.emit(f"{state.ret} {state.name} = {init}(ir_word);\n")
        out.emit(f"(void){state.name};\n")
        if emitter.call_has_feedback:
            out.emit(f"uint32_t call_feedback = {field_expr(fmt.fb)};\n")
            out.emit("(void)call_feedback;\n")
    if inst.properties.uses_opcode:
        out.emit(f"opcode = {canonical_handler(name)};\n")
    mode = record.store_mode
    if mode == STORE_LEAVE:
        dst = FIELD_VARS[fmt.field_of(ROLE_DST)]
        out.emit(f"if ({is_local(fmt.field_of(ROLE_DST))}) {{\n")
        emitter.emit_jump_to_predicted(inst)
        out.emit("}\n")
    elif mode == STORE_NEVER:
        dst = FIELD_VARS[fmt.field_of(ROLE_DST)]
        out.emit(f"assert(!{is_local(fmt.field_of(ROLE_DST))});\n")
    declare_variables(inst, out, emitter.declared_roles)
    for name in sorted(emitter.declared_closes):
        out.emit(f"PyValue {name} = PyValue_NULL;\n")
    if emitter.execution_variant is not None and emitter.execution_variant.shape_cases:
        out.emit("uint32_t ir_shape_offset;\n")
    for k in range(fmt.n_inputs):
        if fmt.borrow_mask & (1 << k):
            own = owner_name(fmt.input_field(k))
            out.emit(f"PyValue {own} = PyValue_NULL;\n")
            out.emit(f"(void){own};\n")
    layout = record.layout
    stack: Stack = IRStack(layout, roles)
    insert_braces = len([p for p in inst.parts if isinstance(p, Uop)]) > 1
    reachable = True
    for part in inst.parts:
        try:
            reachable, stack = write_uop(part, emitter, stack, inst, insert_braces)
        except SyntaxError as ex:
            ex.msg = f"{name}: {ex.msg}"
            raise
        except StackError as ex:
            raise StackError(f"{name}, {part.name}: {ex}") from ex
        if not reachable or emitter.call_finished:
            break
    if (emitter.execution_variant is not None and emitter.execution_variant.shape_cases and
            (emitter.shape_read_guards != 1 or emitter.shape_read_uses != 1 or
             not stack.shape_read_complete)):
        raise analysis_error("polymorphic shape execution needs one dominating checked read", inst.where)
    out.start_line()
    if (reachable and emitter.generator_boundary is not None
            and emitter.send_source_parts is not None):
        # The fall-through of a native send is the inner generator's
        # suspension: the variant yields its value (a delegated yield)
        if not fmt.window or fmt.dst_offset != 2:
            raise analysis_error("native yielded outcome requires checked source window", inst.where)
        outputs = external_io(inst)[1]
        result = outputs[-1].item.name
        # The payload replaces only the result home. Cursor and other through
        # outputs still need their source-ordered final publications.
        assert isinstance(stack, IRStack)
        stack.final = True
        stack.flush(out, native_result=stack.layout.base +
                    PointerOffset(fmt.dst_offset, (), ()))
        out.emit(f"assert(_PyValue_IsHome({result}));\n")
        out.emit("window[2] = PyValue_NULL;\n")
        out.emit(f"IR_GEN_YIELD({result});\n")
        emitter.boundary_reached.add("yield")
        stack.home_node = None
        emitter.native_window_out = True
        emitter.ownership_events.append(OwnershipEvent(
            "_PyIR_OWNER_NATIVE_WINDOW_OUT", flags=1, src=3, dst=0, extent=2))
        reachable = False
    if reachable:
        assert isinstance(stack, IRStack)
        emitter.normal_result_offsets.add(emit_final_outputs(inst, fmt, mode, stack, out))
        emitter.normal_null_receipts.append(
            {edge: stack.null_stores.get(edge, False) for edge in stack.null_paths})
        if emitter.send_source_parts is not None:
            emitter.send_yielded_published = True
        if call_window:
            emitter.call_normal_drained = True
        # A structured JUMPTO/JUMPBY selects next_instr; it does not bypass
        # this final flush. Stack alignment/merge has checked both branch arms
        # at their common point, so consumed inputs are empty on either edge.
        # The unchanged through prefix retains its inputs. Other normal
        # exits, outcome protocols and raw frame access need their own recipe.
        emitter.normal_inputs_cleared = (normal_vacate_capability(inst, fmt)
                                         or emitter.disjoint_child_drained)
        if fmt.unpacks:
            emitter.unpack_to_locals(inst, fmt, UNPACKS[fmt.group])
        # Rule 7: the closes follow the last store.  A close may call
        # Python, which preserves canonical cells but widens their non-NULL
        # facts, just like the typed source callbacks.
        emitter.finish_stores(stack)
        if fmt.store_dst:
            stack.home_action("_PyIR_OWNER_STORAGE_CALLBACK",
                              FIELDS.index(fmt.field_of(ROLE_DST)), flags=1)
        source = emitter.error_source
        if (source is not None and source.native_exit_closed and
                source.homes in {HomeEffect.NO_ARENA_WRITE,
                                 HomeEffect.PRESERVES_CANONICAL_OR_NULL} and
                not source.changes_origin and not fmt.window and
                not inst.properties.switches_frame and not inst.properties.returns and
                not stack.reloaded and stack.home_node is not None and
                stack.home_writes == stack.home_write_receipts and
                not stack.possible_native_owners and not stack.unknown_read and
                not any(stack.read_fields.values())):
            emitter.error_checkpoint_sites[0] = ErrorCheckpoint(
                0, 0, stack.home_node, tuple(stack.home_graph.nodes[:stack.home_node]))
        out.emit("DISPATCH();\n")
        emitter.error_finalized = True
    normal_edges = ({1} if fmt.jump_always else {0, 1}
                    if (fmt.jumps or fmt.forward) and not fmt.names_target else {0})
    if (not reachable and emitter.normal_observer_edges == normal_edges
            and normal_vacate_capability(inst, fmt)):
        # Checked terminal wrappers flush before their callbacks and dispatch
        # themselves. Every ordinary successor must have such a commit; a
        # nonbranching handler has only its fallthrough successor to cover.
        emitter.normal_inputs_cleared = True
    out.start_line()
    out.emit("}")
    out.emit("\n")
    assert isinstance(stack, IRStack)
    if not emitter.ownership_events and inst.name == "BINARY_OP":
        # The generic operation reads both inputs, detaches them and writes
        # a fresh result: the program that the verifier proves homes with
        for index in range(2):
            emitter.ownership_events.append(OwnershipEvent("_PyIR_OWNER_READ", src=index))
            emitter.ownership_events.append(OwnershipEvent(
                "_PyIR_OWNER_TAKE", dst=index, src=index))
        emitter.ownership_events.append(OwnershipEvent("_PyIR_OWNER_FRESH"))
        emitter.ownership_events.append(OwnershipEvent("_PyIR_OWNER_NORMAL_EXIT"))
    if not emitter.ownership_events:
        kind = normal_transfer_kind(inst, fmt)
        if kind != "_PyIR_TRANSFER_UNKNOWN":
            # Audited leaf source contracts remain guards while their actual
            # emitter transport is ported. There is one consumer interface;
            # an uncovered body cannot acquire a recipe from its name.
            emitter.ownership_events.append(OwnershipEvent(
                "_PyIR_OWNER_LEAF", src=kind))
        elif emitter.call_normal_drained:
            emitter.ownership_events.append(OwnershipEvent("_PyIR_OWNER_CALL_DRAIN"))
        elif emitter.normal_inputs_cleared and emitter.unpack_local_drained:
            emitter.ownership_events.append(OwnershipEvent(
                "_PyIR_OWNER_UNPACK_LOCAL_DRAIN"))
        elif emitter.normal_inputs_cleared:
            offsets = emitter.normal_result_offsets
            result_offset = next(iter(offsets)) if len(offsets) == 1 else None
            emitter.ownership_events.append(OwnershipEvent(
                "_PyIR_OWNER_NORMAL_CLEAR_INPUTS",
                flags=(int(result_offset is not None) |
                       (2 if emitter.disjoint_child_drained else 0) |
                       sum(4 << edge for edge in normal_edges
                           if emitter.null_outputs is not None and
                           any(edge in receipt for receipt in emitter.normal_null_receipts) and
                           all(receipt.get(edge, True) for receipt in emitter.normal_null_receipts))),
                src=result_offset or 0))
    if not emitter.normal_completion_uncovered:
        completions = emitter.normal_completions
        if emitter.generator_boundary is not None:
            completions = {replace(event, kind="_PyIR_OWNER_NATIVE_WINDOW_OUT",
                                  flags=1, extent=0) for event in completions}
            if emitter.send_yielded_published and not emitter.native_window_out:
                completions.add(OwnershipEvent(
                    "_PyIR_OWNER_NATIVE_WINDOW_OUT", flags=1, src=3, dst=0, extent=2))
        elif emitter.send_yielded_published:
            completions = {replace(event, flags=event.flags | 2, extent=2)
                           for event in completions}
        emitter.ownership_events.extend(sorted(completions,
                                              key=lambda event: (event.dst, event.src)))
    emitter.check_record_consumed()


# What a printed handler or routine body cannot contain (tiers87, C2 and C3):
# a jump into the evaluator, or a local that only the evaluator has.  A body
# leaves only through the transfer macros that both bindings define
# (Python/ceval_macros.h, Python/ceval.c); the operands of the evaluator's
# shared bodies are its locals only in the evaluator's binding of those
# macros.  The shared BINARY_OP body reads its operation, the operand of its
# routine.
EVALUATOR_ONLY = re.compile(
    r"\bgoto\b|\bLABEL_\w+|\bTARGET_\w+|\bopcode_targets\b|\bJUMP_TO_LABEL\b|"
    r"\blastopcode\b|\b(?:process_error_origin|process_error_entry|gen_value|"
    r"gen_dying|gen_empty|gen_transfer|binary_op_operation)\b")


def check_body(name: str, text: str, where: object, routine_operand: str = "") -> None:
    "Refuse a body that jumps into the evaluator or reads its state"
    for match in EVALUATOR_ONLY.finditer(text):
        if match.group(0) != routine_operand:
            raise analysis_error(f"{name}: the body has `{match.group(0)}`, which "
                                 "only the evaluator has (tiers87, C2 and C3)", where)


# Rule 12.  The instrumented twin of an outlined instruction whose event
# follows the effect is a function that runs the base function, then fires
# the event for the edge that the base took.  It is not a copy of the base:
# the DSL bodies of the twins stay as the contract that their metadata is
# derived from.  Value: the base, and whether the base is a conditional jump
# that reports its edge (a loop step has an event only for its next item, and
# its edge is the word that the base returns).
EDGE_TWINS = {
    "INSTRUMENTED_COMPARE_OP_JUMP": ("COMPARE_OP_JUMP", True),
    "INSTRUMENTED_CONTAINS_OP_JUMP": ("CONTAINS_OP_JUMP", True),
    "INSTRUMENTED_FOR_ITER": ("FOR_ITER", False),
}


# Rule 12, twins whose event precedes the effect: the event is a function of
# Python/ceval.c, and the twin goes on to the base's handler with the same
# word.  The event is the twin's first part in the DSL.
EVENT_TWINS = {
    "INSTRUMENTED_YIELD_VALUE": ("ir_event_yield", "YIELD_VALUE"),
    "INSTRUMENTED_YIELD_DELEGATED": ("ir_event_yield", "YIELD_DELEGATED"),
    "INSTRUMENTED_END_FOR": ("ir_event_end_for", "END_FOR"),
}


def edge_twin_function(name: str, base: str, reports_edge: bool) -> str:
    "The function of the twin `name` of the outlined instruction `base`"
    if reports_edge:
        taken = "IR_ACTIVATION()->edge"
        branch = f"""    /* A jump of no distance runs the next word either way. */
    int taken = IR_ACTIVATION()->edge;
    return ir_event_edge(tstate, frame, this_instr, this_instr, base.next,
        taken ? PY_MONITORING_EVENT_BRANCH_RIGHT : PY_MONITORING_EVENT_BRANCH_LEFT,
        taken ? _PyIR_EDGE_TAKEN : _PyIR_EDGE_FALLTHROUGH);
"""
    else:
        # A loop step has an event only for the next item
        branch = """    if (base.next != this_instr + 1) {
        return base;
    }
    return ir_event_edge(tstate, frame, this_instr, this_instr, base.next,
                         PY_MONITORING_EVENT_BRANCH_LEFT, _PyIR_EDGE_FALLTHROUGH);
"""
    return f"""
static Py_NO_INLINE _PyIRTransfer
ir_run_{name}(
    PyThreadState *tstate, _PyInterpreterFrame *frame,
    _PyInstr *next_instr, _PyIRWord ir_word)
{{
    _PyInstr *this_instr = next_instr;
    /* The base would specialise the word and overwrite the instrumentation. */
    PAUSE_ADAPTIVE_COUNTER(_PyIR_FbHeaderOf(frame, this_instr, {base}));
    _PyIRTransfer base = ir_run_{base}(tstate, frame, next_instr, ir_word);
    if (base.next == NULL) {{
        return base;
    }}
{branch}}}
"""


FORM_PARAMETERS = ("PyThreadState *tstate, _PyInterpreterFrame *frame, "
                   "_PyInstr *next_instr")


def form_header(function: str, extra: str = ", _PyIRWord ir_word") -> str:
    return (f"\nstatic Py_NO_INLINE _PyIRTransfer\n{function}(\n"
            f"    {FORM_PARAMETERS}{extra})\n{{")


def function_form(name: str, text: str) -> str:
    """The function form of handler `name`, from the body printed for the
    evaluator.  It takes the evaluator's state that handlers use, and its
    exits are returns and tail calls (the transfer macros, which
    Python/ceval.c defines for it)."""
    marker = f"TARGET({name}) {{"
    assert marker in text
    return text.replace(marker, form_header(f"ir_run_{name}"), 1)


# The operation of the shared BINARY_OP body: the operand of its routine,
# ir_binary_op(), which the generic handlers pass from their number and the
# specialisations' deopts from their entry
BINARY_OP_OPERAND = "binary_op_operation"


def binary_op_routine(text: str) -> str:
    "The function form of the shared BINARY_OP body"
    marker = "LABEL(binary_op) {"
    assert marker in text
    return text.replace(marker, form_header(
        "ir_binary_op", f", _PyIRWord ir_word, int {BINARY_OP_OPERAND}"), 1)


def binary_op_function(name: str, operation: str) -> str:
    "The function form of the generic BINARY_OP handler `name`"
    return (form_header(f"ir_run_{name}") +
            f"\n    Py_MUSTTAIL return ir_binary_op(tstate, frame, next_instr, ir_word, "
            f"{operation});\n}}\n")


def event_twin_function(name: str, event: str, base: str) -> str:
    """The function form of the twin `name` whose event, the function
    `event`, precedes the effect of `base` (IR_RUN_EVENT in the evaluator)"""
    return (form_header(f"ir_run_{name}") + f"""
    _PyIRTransfer ir_t = {event}(tstate, frame, next_instr, ir_word);
    if (ir_t.next != next_instr) {{
        return ir_t;
    }}
    Py_MUSTTAIL return ir_run_{base}(tstate, frame, next_instr, ir_word);
}}
""")


# ---------------------------------------------------------------------------
# Whole-word completion stubs (regions89 G1a; rd/design.md section 5 and
# g1a/abi.md).
#
# A family is planned when its generic arm declares region(shared); each of
# its members that declares a region is planned with it, and the others
# keep their path.  A shared arm becomes a C function of the completion
# shape,
#
#     int _PyIR_Stub_X(PyThreadState *tstate, _PyInterpreterFrame *frame,
#                      _PyIRWord *site);
#
# printed from the same handler text as the evaluator's handler and the
# function form, with the transfer macros bound to status returns
# (STUB_BINDINGS).  An inline arm keeps B2's template; its plan names the
# family's generic stub, which the JIT calls on a miss.  Plans are read
# from declarations only: an arm with a uop that declares no contract is
# refused, never planned from the inferred record.

# The exits a planned arm may declare: the completion ABI has no branch
# result site yet (G1b) and no transfers (G3, G4).  `jump` is the
# specializing uop's redispatch (DISPATCH_SAME_OPARG), status AGAIN.
STUB_EXITS = frozenset(("next", "miss", "deopt", "raise", "jump"))
STUB_JUMPS = frozenset(("DISPATCH_SAME_OPARG",))
STUB_REGIONS = ("inline", "shared")
# The effects after which a path may have reentered Python whatever it
# released: the uop's entry sets ESCAPED
STUB_REENTRY = frozenset(("runs_python", "polls"))


@dataclass(frozen=True)
class StubPlan:
    region: str         # inline or shared
    family: str         # whose generic stub runs the word after a miss
    escapes: bool       # a uop declares runs_python or polls: every
                        # completion that reaches it reports ESCAPED
    arm: str            # the handler whose template or stub the word
                        # compiles: itself, or its family's baseline(...)


def arm_escapes(inst: Instruction) -> bool:
    return any(ir_contract.declared_contract(part).continuing_effects() & STUB_REENTRY
               for part in inst.parts if isinstance(part, Uop))


def stub_function_name(handler: str) -> str:
    return f"_PyIR_Stub_{handler}"


def release_forms() -> tuple[str, ...]:
    """The releases whose stub binding reports ESCAPED only when it drops a
    last count (g1a/abi.md): every PRIMITIVE row that only releases its one
    consumed value, and the form for a value known to be in pointer space
    that the generator prints for it (IREmitter.decref_inputs)"""
    names = []
    for row in ir_contract.rows().values():
        if (row.kind == "PRIMITIVE" and row.effects == {"releases"}
                and row.result.shape == "NONE"
                and [a.mode for a in row.args] == ["consumed"]):
            names += [row.name, f"_{row.name}_POINTER"]
    return tuple(sorted(names))


def check_stub_arm(arm: Instruction, generic: bool) -> None:
    "Refuse an arm that a stub plan cannot serve, naming what it lacks"
    if arm.region not in STUB_REGIONS:
        raise analysis_error(f"{arm.name}: region({arm.region}) has no stub plan "
                             f"before G4 ({', '.join(STUB_REGIONS)})", arm.where)
    for part in arm.parts:
        if not isinstance(part, Uop):
            continue
        contract = ir_contract.declared_contract(part)
        for site in ir_contract.effect_sites(part):
            if site.token.text in ir_contract.rows() or site.effect != "releases":
                continue
            if site.token.text != "DECREF_INPUTS":
                raise analysis_error(f"{arm.name}: {part.name} releases through "
                                     f"`{site.token.text}`, which no row declares",
                                     site.token)
        extra = contract.exits - STUB_EXITS
        if extra:
            raise analysis_error(f"{arm.name}: {part.name} declares the exits "
                                 f"{', '.join(sorted(extra))}, which no stub plan has "
                                 "before G1b", arm.where)
        # A stub binds a jump to its own word, DISPATCH_SAME_OPARG(), to
        # AGAIN (a specializer's redispatch, _MAYBE_INSTRUMENT's rerun);
        # no other jump
        jumps = {t.text for t in part.body.tokens()
                 if JUMP in TRANSFER_TOKENS.get(t.text, ())}
        if "jump" in contract.exits and jumps - STUB_JUMPS:
            raise analysis_error(f"{arm.name}: {part.name} jumps by "
                                 f"{', '.join(sorted(jumps - STUB_JUMPS))}, which a stub "
                                 "cannot bind: its one jump is to its own word "
                                 "(DISPATCH_SAME_OPARG(), AGAIN)", arm.where)
        if generic and contract.exits & {"miss", "deopt"}:
            raise analysis_error(f"{arm.name} is its family's generic arm, but "
                                 f"{part.name} can miss", arm.where)


def stub_plans(analysis: Analysis) -> dict[str, StubPlan]:
    "The plan of each handler that has one, by handler name"
    insts = all_instructions(analysis)
    plans: dict[str, StubPlan] = {}
    handlers = handler_instructions(analysis)
    for family in analysis.families.values():
        head = insts.get(family.name)
        if head is None or head.region is None:
            continue
        if head.region != "shared":
            raise analysis_error(f"{family.name}: a family's generic arm is "
                                 "region(shared)", head.where)
        # The IR-only members too, which the opcode tables leave out
        arms = [head] + [m for m in insts.values()
                         if m.family is family and m is not head and m.region is not None]
        for arm in arms:
            check_stub_arm(arm, arm is head)
        baseline = None
        if family.baseline:
            baseline = insts[family.baseline]
            if baseline.region is None:
                raise analysis_error(f"{family.name}: its baseline({baseline.name}) "
                                     "declares no region", baseline.where)
        for name, inst in handlers.items():
            if not any(inst is arm for arm in arms) or name in EXECUTION_VARIANTS:
                continue
            # The head's words compile to the baseline's arm, and miss to the
            # head's stub
            if inst is head and baseline is not None:
                plans[name] = StubPlan(baseline.region, family.name,
                                       arm_escapes(baseline), baseline.name)
            else:
                plans[name] = StubPlan(inst.region, family.name, arm_escapes(inst), name)
    return plans


def stub_escapes(inst: Instruction) -> frozenset[str]:
    """The uops of a planned arm whose entry sets ESCAPED: those that
    declare runs_python or polls, and those with a release that no release
    form reports (a row other than release_forms(), whose effects declare
    `releases`)"""
    reporting = set(release_forms()) | {"DECREF_INPUTS"}
    names = set()
    for part in inst.parts:
        if not isinstance(part, Uop):
            continue
        contract = ir_contract.declared_contract(part)
        if contract.continuing_effects() & STUB_REENTRY:
            names.add(part.name)
        elif any(site.effect == "releases" and site.token.text not in reporting
                 for site in ir_contract.effect_sites(part)):
            names.add(part.name)
    return frozenset(names)


STUB_PARAMETERS = "PyThreadState *tstate, _PyInterpreterFrame *frame, _PyIRWord *site"


def stub_header(function: str, extra: str = "") -> str:
    return (f"\nstatic Py_NO_INLINE int\n{function}(\n    {STUB_PARAMETERS}{extra})\n{{\n"
            "    _PyInstr *next_instr = site;\n"
            "    _PyIRWord ir_word = *site;\n"
            "    int ir_status = 0;\n")


def stub_function(function: str, text: str, marker: str, extra: str = "") -> str:
    "The stub `function`, from a handler's text printed for the evaluator"
    assert marker in text
    return text.replace(marker, stub_header(function, extra), 1)


BINARY_OP_STUB = stub_function_name("binary_op")


def binary_op_stub(name: str, operation: str) -> str:
    "The stub of the generic BINARY_OP handler `name`"
    return (f"\nstatic Py_NO_INLINE int\n{stub_function_name(name)}({STUB_PARAMETERS})\n{{\n"
            f"    Py_MUSTTAIL return {BINARY_OP_STUB}(tstate, frame, site, {operation});\n}}\n")


def binary_op_family_stub(fb: str) -> str:
    """BINARY_OP's generic stub after a specialised arm's miss: the shared
    body with the word's operation, which its entry keeps (as the forms'
    IR_DEOPT_BINARY_OP)"""
    return (f"\nstatic Py_NO_INLINE int\n{stub_function_name(BINARY_OP_FAMILY)}"
            f"({STUB_PARAMETERS})\n{{\n"
            "    _PyIRWord ir_word = *site;\n"
            f"    Py_MUSTTAIL return {BINARY_OP_STUB}(tstate, frame, site,\n"
            f"        (int)((_PyFbHeader *)IR_FB({fb}))->arg);\n}}\n")


# The transfer macros that a stub binds (g1a/abi.md).  The others that the
# forms bind are left undefined, so that a stub using one does not compile.
STUB_BINDINGS = {
    "DISPATCH()": "return ir_status",
    "DISPATCH_SAME_OPARG()": "return ir_status | _PyIR_AGAIN",
    "JUMP_TO_PREDICTED(name)":
        "do { assert(ir_status == 0); return _PyIR_MISS; } while (0)",
    "IR_DEOPT_BINARY_OP(F)":
        "do { assert(ir_status == 0); return _PyIR_MISS; } while (0)",
    "PROCESS_ERROR_AT(ingress, checkpoint, origin)": (
        "do { \\\n"
        "        _PyEvalActivation *ir_exit = IR_ACTIVATION(); \\\n"
        "        ir_exit->exit_kind = _PyIR_EXIT_RAISE; \\\n"
        "        ir_exit->exit_next = (origin); \\\n"
        "        ir_exit->exit_entry = \\\n"
        "            _PyIR_ERROR_ENTRY(_PyIR_ERROR_INGRESS_##ingress, checkpoint); \\\n"
        "        return ir_status | _PyIR_RAISED; \\\n"
        "    } while (0)"),
    "IR_STUB_ESCAPE()": "(ir_status |= _PyIR_ESCAPED)",
    # The departing frame's unit runs on until a switch leaves it
    "IR_PIN_DEPARTING()": "(ir_status |= _PyJit_PinDeparting(tstate, frame))",
    "IR_CLOSE_WINDOW(W, N)": "_PyIR_STUB_CLOSE_WINDOW((W), (N))",
}
STUB_UNBOUND = ("DISPATCH_GOTO", "DISPATCH_INLINED", "IR_EXIT", "IR_OBSERVE_EDGE",
                "IR_STEP_DONE", "IR_REPORT_EDGE", "IR_GEN_YIELD_AT",
                "IR_GEN_COMPLETE_AT", "IR_GEN_RETURN", "IR_GEN_RESUME",
                "GEN_DISPATCH", "GEN_STEP_JUMP", "GEN_STEP_EFFECT", "IR_STORE_RETURN")


def macro_name(binding: str) -> str:
    return binding.split("(", 1)[0]


def stub_prologue() -> tuple[str, list[str]]:
    releases = release_forms()
    names = ([macro_name(b) for b in STUB_BINDINGS] + list(STUB_UNBOUND)
             + list(releases))
    lines = ["""
/* The whole-word completion stubs (regions89 G1a; g1a/abi.md), printed by
 * Tools/cases_generator/ir_generator.py from the handlers' text with the
 * transfer macros bound to status returns.  Python/ceval.c includes this
 * file after the function forms, whose bindings of the other macros the
 * stubs share. */

/* Whether closing V drops a last count, which runs its destructor: then
 * the path may have reentered Python (ESCAPED) */
static inline int
_PyIR_StubDropsLast(PyValue v)
{
    return PyValue_RefcountOnObject(v) && Py_REFCNT(BITS_TO_PTR(v)) == 1
        ? _PyIR_ESCAPED : 0;
}

static inline int
_PyIR_StubWindowDropsLast(const PyValue *window, int n)
{
    int status = 0;
    for (int i = 0; i < n; i++) {
        status |= _PyIR_StubDropsLast(window[i]);
    }
    return status;
}

/* A release that reports ESCAPED exactly when it deallocates */
#define _PyIR_STUB_RELEASE(CLOSE, V) \\
    do { \\
        PyValue ir_released = (V); \\
        ir_status |= _PyIR_StubDropsLast(ir_released); \\
        (CLOSE)(ir_released); \\
    } while (0)
#define _PyIR_STUB_CLOSE_WINDOW(W, N) \\
    (ir_status |= _PyIR_StubWindowDropsLast((W), (N)), _PyIR_CloseWindow((W), (N)))
"""]
    for name in names:
        lines.append(f'#pragma push_macro("{name}")\n#undef {name}\n')
    for binding, value in STUB_BINDINGS.items():
        lines.append(f"#define {binding} {value}\n")
    for name in releases:
        lines.append(f"#define {name}(V) _PyIR_STUB_RELEASE({name}, V)\n")
    return "".join(lines), names


def generate_ir_stubs(analysis: Analysis, formats: dict[str, IRFormat],
                      stubs: dict[str, str], outfile: TextIO,
                      arms: Sequence[ir_arms.ArmEntry] = ()) -> None:
    """The stubs of the planned shared arms (their texts in stubs, by
    function), the family stubs that a miss calls, and the plan table that
    Python/jit.c reads"""
    plans = stub_plans(analysis)
    ids = handler_ids(analysis)
    write_header(__file__, [], outfile)
    prologue, names = stub_prologue()
    outfile.write(prologue)
    # The shared BINARY_OP body first: the operations' stubs call it
    for function in sorted(stubs, key=lambda f: (f != BINARY_OP_STUB, f)):
        outfile.write(stubs[function])
    families = sorted({plan.family for plan in plans.values()})
    if BINARY_OP_FAMILY in families:
        fmt = formats[binary_op_handlers(analysis)[0]]
        outfile.write(binary_op_family_stub(field_expr(fmt.fb)))
    for name in reversed(names):
        outfile.write(f'#pragma pop_macro("{name}")\n')
    outfile.write("\n/* The plan of each handler that has one: its region, whether "
                  "its\n * completions report ESCAPED, its stub (shared), and its "
                  "family's generic\n * stub, which runs the word after a miss */\n")
    by_arm = arm_ids(analysis, arms)
    outfile.write("\n/* The stubs of the register instruction arms, defined in "
                  "generated_rop_stubs.c.h */\n")
    for arm in by_arm.values():
        for stub in sorted({s for s in (arm.stub, arm.generic) if s}):
            outfile.write(f"static int {stub}(PyThreadState *, "
                          "_PyInterpreterFrame *, _PyIRWord *);\n")
    outfile.write(f"const _PyIRStubPlan _PyIR_StubPlans[{ir_arms.IR_LIMIT}] = {{\n")
    for i, arm in sorted(by_arm.items()):
        stub = f"(const void *){arm.stub}" if arm.stub else "NULL"
        generic = f"(const void *){arm.generic}" if arm.generic else "NULL"
        outfile.write(f"    [{i}] = {{_PyIR_PLAN_{arm.region.upper()}, "
                      f"{int(arm.escapes)}, {i}, {stub}, {generic}}},  // {arm.name}\n")
    for name in sorted(plans, key=lambda n: ids[n]):
        plan = plans[name]
        stub = (f"(const void *){stub_function_name(plan.arm)}"
                if plan.region == "shared" else "NULL")
        outfile.write(f"    [{ids[name]}] = {{_PyIR_PLAN_{plan.region.upper()}, "
                      f"{int(plan.escapes)}, {ids[plan.arm]}, {stub}, "
                      f"(const void *){stub_function_name(plan.family)}}},  "
                      f"// {name}\n")
    outfile.write("};\n")


def generate_ir_cases(analysis: Analysis, formats: dict[str, IRFormat],
                      outfile: TextIO, lines: bool,
                      checkpoints: dict[str, tuple[ErrorCheckpoint, ...]] | None = None,
                      stubs: dict[str, str] | None = None) -> list[str]:
    """Write the handlers to outfile, and return their function forms.  An
    outlined handler's handler is a call of its form.  The stubs of the
    planned shared arms go to stubs, by function."""
    out = CWriter(outfile, 2, lines)
    emitter = IREmitter(out, analysis, formats)
    out.emit("\n")
    functions: list[str] = []
    twin_functions: list[str] = []
    outlined_names: set[str] = set()
    # The generic handlers of BINARY_OP differ only in the operation.  Each
    # sets it and enters the one body, which handles any operation, so that
    # the body is in the evaluator once, not once per operation.
    binary_ops = frozenset(binary_op_handlers(analysis))
    shared_binary_op_done = False
    family_heads = {inst.family.name for inst in all_instructions(analysis).values()
                    if inst.family is not None} | set(analysis.families)
    plans = stub_plans(analysis)
    for name, inst in sorted(handler_instructions(analysis).items()):
        if generator_word_entry(name, analysis) != name:
            continue
        plan = plans.get(name)
        # The shared arms' stubs, and each family's generic (its head's)
        stub = (plan is not None and stubs is not None
                and (plan.region == "shared" or name == plan.family))
        if name in binary_ops:
            operation = "NB_" + name.removeprefix(BINARY_OP_FAMILY + "_")
            out.emit(f"TARGET({name}) {{\n")
            out.emit(f"{BINARY_OP_OPERAND} = {operation};\n")
            out.emit("goto LABEL_binary_op;\n")
            out.emit("}\n")
            functions.append(binary_op_function(name, operation))
            if stub:
                stubs[stub_function_name(name)] = binary_op_stub(name, operation)
            if shared_binary_op_done:
                continue
            shared_binary_op_done = True
            emitter.shared_binary_op = True
        if name in EDGE_TWINS:
            base, taken_event = EDGE_TWINS[name]
            twin_functions.append(edge_twin_function(name, base, taken_event))
            out.emit(f"TARGET({name}) {{\n")
            out.emit(f"IR_RUN_OUTLINED(ir_run_{name});\n")
            out.emit("}\n")
            continue
        if name in EVENT_TWINS:
            event, base = EVENT_TWINS[name]
            twin_functions.append(event_twin_function(name, event, base))
            out.emit(f"TARGET({name}) {{\n")
            out.emit(f"IR_RUN_EVENT({event}, {base});\n")
            out.emit("}\n")
            continue
        emitter.handler_format = formats[name]
        emitter.allowed_error_checkpoints = frozenset(
            checkpoint.id for checkpoint in (checkpoints or {}).get(name, ()))
        emitter.execution_variant = EXECUTION_VARIANTS.get(name)
        emitter.stub_escapes = stub_escapes(inst) if plan is not None else None
        # The generic path of an adaptive family runs only until a
        # specialisation replaces it, or if it cannot
        generic_head = (name in family_heads
                        and any(isinstance(part, Uop) and
                                "specializing" in part.annotations
                                for part in inst.parts))
        outlined = ((inst.outlined or inst.properties.outlined or generic_head)
                    and not emitter.shared_binary_op)
        buffer = io.StringIO()
        emitter.out = CWriter(buffer, 2, lines)
        generate_handler(name, inst, formats[name], emitter, emitter.out)
        emitter.out = out
        text = buffer.getvalue()
        if emitter.shared_binary_op:
            check_body("binary_op", text, inst.where, BINARY_OP_OPERAND)
            functions.append(binary_op_routine(text))
            outfile.write(text)
            if stub:
                stubs[BINARY_OP_STUB] = stub_function(
                    BINARY_OP_STUB, text, "LABEL(binary_op) {",
                    f", int {BINARY_OP_OPERAND}")
        else:
            check_body(name, text, inst.where)
            functions.append(function_form(name, text))
            if stub:
                stubs[stub_function_name(name)] = stub_function(
                    stub_function_name(name), text, f"TARGET({name}) {{")
            if outlined:
                outlined_names.add(name)
                out.emit(f"TARGET({name}) {{\n")
                out.emit(f"IR_RUN_OUTLINED(ir_run_{name});\n")
                out.emit("}\n")
            else:
                outfile.write(text)
        for checkpoint in (checkpoints or {}).get(name, ()):
            if emitter.error_checkpoint_sites.get(checkpoint.id) != checkpoint:
                raise analysis_error("error checkpoint changed during final emission", inst.where)
        emitter.handler_format = None
        emitter.execution_variant = None
        emitter.stub_escapes = None
        emitter.shared_binary_op = False
    handlers = handler_instructions(analysis)
    for twin, (base, _) in EDGE_TWINS.items():
        if twin in handlers and base not in outlined_names:
            raise analysis_error(f"the base {base} of {twin} is not outlined",
                                 handlers[twin].where)
    return functions + twin_functions


# The labels of bytecodes.c that start an activation's work, which the
# function-form executor enters (ir_execute_forms() in Python/ceval.c) and a
# frame entry tail-calls; the other labels are error ingresses, which the
# emitter prints as PROCESS_ERROR_AT.
ENTRY_LABELS = ("start_frame", "throw_error")


def entry_labels(analysis: Analysis) -> list[str]:
    return [name for name in ENTRY_LABELS if name in analysis.labels]


def generate_ir_labels(analysis: Analysis, emitter: Emitter) -> None:
    emitter.emit("\n")
    for name, label in analysis.labels.items():
        emitter.emit(f"LABEL({name})\n")
        emitter.emit("{\n")
        storage = Storage(IRStack(), [], [], 0, False)
        if label.spilled:
            storage.spilled = 1
            storage.stack.physical_sp = None
        emitter.emit_tokens(label, storage, None)
        emitter.emit("}\n\n")
    # The target of every id without a handler
    emitter.emit("TARGET(UNKNOWN_OPCODE)\n")
    emitter.emit("{\n")
    emitter.emit("int opcode = IR_HANDLER(*next_instr);\n")
    emitter.emit(UNKNOWN_OPCODE_HANDLER)
    emitter.emit("}\n\n")


def label_forms(analysis: Analysis, formats: dict[str, IRFormat],
                lines: bool) -> list[str]:
    """The function forms of the entry labels, ir_<label>(tstate, frame,
    next_instr), and of UNKNOWN_OPCODE, printed from the bodies that the
    evaluator has.  A test's input may lack them."""
    result = []
    for name in entry_labels(analysis):
        label = analysis.labels[name]
        buffer = io.StringIO()
        emitter = IREmitter(CWriter(buffer, 0, lines), analysis, formats)
        storage = Storage(IRStack(), [], [], 0, False)
        if label.spilled:
            storage.spilled = 1
            storage.stack.physical_sp = None
        emitter.emit_tokens(label, storage, None)
        text = buffer.getvalue()
        check_body(name, text, label.body.open)
        result.append(form_header(f"ir_{name}", "") + "\n" + text + "}\n")
    result.append(form_header("ir_run_UNKNOWN_OPCODE") +
                  "\nint opcode = IR_HANDLER(*next_instr);\n" +
                  UNKNOWN_OPCODE_HANDLER + "}\n")
    return result


def arm_ids(analysis: Analysis,
            arms: Sequence[ir_arms.ArmEntry]) -> dict[int, ir_arms.ArmEntry]:
    """The register instruction arms by handler id: they follow the old
    handlers, in definition order (Python/register_ops)"""
    num_handlers(analysis, arms)
    first = num_old_handlers(analysis)
    return {first + arm.index: arm for arm in arms if arm.emitted}


def form_table(analysis: Analysis, arms: Sequence[ir_arms.ArmEntry] = ()) -> str:
    """The function form of every handler number, as opcode_targets has its
    label: ir_handler_forms[IR_HANDLER(word)] runs the word"""
    ids = handler_ids(analysis)
    by_index = {i: name for name, i in ids.items()}
    by_arm = arm_ids(analysis, arms)
    lines = [f"static const _PyIRForm ir_handler_forms[{ir_arms.IR_LIMIT}] = {{\n"]
    for i in range(ir_arms.IR_LIMIT):
        if i in by_arm:
            lines.append(f"    [{i}] = ir_run_{by_arm[i].name},\n")
            continue
        name = generator_word_entry(by_index.get(i, "UNKNOWN_OPCODE"), analysis)
        lines.append(f"    [{i}] = ir_run_{name},\n")
    lines.append("};\n")
    return "".join(lines)


def form_declarations(analysis: Analysis, arms: Sequence[ir_arms.ArmEntry] = ()) -> str:
    "Every form, declared before the forms, which call one another"
    ids = handler_ids(analysis)
    names = sorted({generator_word_entry(name, analysis) for name in ids}
                   | {"UNKNOWN_OPCODE"} | {arm.name for arm in arms if arm.emitted})
    lines = [f"static _PyIRTransfer ir_run_{name}({FORM_PARAMETERS}, _PyIRWord ir_word);\n"
             for name in names]
    lines.append(f"static _PyIRTransfer ir_binary_op({FORM_PARAMETERS}, _PyIRWord ir_word, "
                 f"int {BINARY_OP_OPERAND});\n")
    lines += [f"static _PyIRTransfer ir_{name}({FORM_PARAMETERS});\n"
              for name in entry_labels(analysis)]
    lines.append(f"static const _PyIRForm ir_handler_forms[{ir_arms.IR_LIMIT}];\n")
    return "".join(lines)


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


# Calls whose name says which sys.monitoring events they fire
MONITORING_HELPERS = {
    "IR_MONITOR_CALL": ("CALL",),
    "monitor_stop_iteration": ("STOP_ITERATION",),
    "monitor_reraise": ("RERAISE",),
    "_Py_call_instrumentation_line": ("LINE",),
    "_Py_call_instrumentation_instruction": ("INSTRUCTION",),
}
# An instrumented generic call (call_monitor(), call_monitor_result() and
# call_run_*() in Python/ceval.c)
CALL_EVENTS = ("CALL", "C_RETURN", "C_RAISE")
EVENT_PREFIX = "PY_MONITORING_EVENT_"


def monitoring_events(name: str, inst: Instruction) -> list[str]:
    "The sys.monitoring events that handler name fires"
    events: set[str] = set()
    for tkn in uop_token_list(inst):
        if tkn.text.startswith(EVENT_PREFIX):
            events.add(tkn.text)
        events.update(EVENT_PREFIX + e for e in MONITORING_HELPERS.get(tkn.text, ()))
    if name.startswith("INSTRUMENTED_") and uses_generic_call(inst):
        events.update(EVENT_PREFIX + e for e in CALL_EVENTS)
    # Completion has one semantic site, including when the producer is a
    # specialized native iterator or a Python generator returning to it.
    if any(t.text in {"IR_COMPLETE_STEP", "IR_ENTER_GENERATOR"}
           for t in uop_token_list(inst)):
        group = inst.family.name if inst.family else inst.name.removeprefix("INSTRUMENTED_")
        if group == "FOR_ITER":
            events.update(EVENT_PREFIX + e for e in
                          ("BRANCH_LEFT", "BRANCH_RIGHT", "STOP_ITERATION"))
        elif group == "SEND":
            events.update(EVENT_PREFIX + e for e in
                          ("BRANCH_LEFT", "STOP_ITERATION"))
    return sorted(events)


def twin_of(name: str, fmt: IRFormat, ids: dict[str, int]) -> str | None:
    """The handler that instrumentation runs on a word of handler name: the
    twin of its family; a twin is its own"""
    if name.startswith("INSTRUMENTED_"):
        return name
    twin = "INSTRUMENTED_" + fmt.group
    return twin if twin in ids else None


def analyze_handlers(analysis: Analysis, formats: dict[str, IRFormat],
                    programs: dict[str, tuple[OwnershipEvent, ...]] | None = None,
                    abrupt: dict[str, bool] | None = None,
                    operand_abrupt: dict[str, bool] | None = None,
                    error_routes: dict[str, tuple[str, ...]] | None = None,
                    observer_entries: dict[str, bool] | None = None,
                    checkpoints: dict[str, tuple[ErrorCheckpoint, ...]] | None = None,
                    normal_storage: dict[str, ErrorCheckpoint | None] | None = None
                    ) -> None:
    "Emit every handler once, to no output, and collect what the contract needs"
    import io
    for name, inst in sorted(handler_instructions(analysis).items()):
        if formats[name].transparent:
            kind = normal_transfer_kind(inst, formats[name])
            if programs is not None and kind != "_PyIR_TRANSFER_UNKNOWN":
                programs[name] = (OwnershipEvent("_PyIR_OWNER_LEAF", src=kind),)
            if observer_entries is not None:
                out = CWriter.null()
                emitter = IREmitter(out, analysis, formats)
                emitter.handler_format = formats[name]
                generate_handler(name, inst, formats[name], emitter, out)
                observer_entries[name] = emitted_observer_entry(emitter, inst)
            continue
        buffer = io.StringIO()
        out = CWriter(buffer, 0, False)
        emitter = IREmitter(out, analysis, formats)
        emitter.handler_format = formats[name]
        emitter.allowed_error_checkpoints = None
        generate_handler(name, inst, formats[name], emitter, out)
        if abrupt is not None:
            abrupt[name] = emitter.abrupt_prefix
        if operand_abrupt is not None:
            operand_abrupt[name] = (
                emitter.abrupt_operand_expected is not None and
                emitter.abrupt_operand_sources == emitter.abrupt_operand_expected and
                not emitter.abrupt_operand_uncovered and
                not emitter.error_uncovered and
                emitter.error_source is not None and
                emitter.error_source.unwind_sources == emitter.error_unwind_sources and
                emitter.error_source.terminal_sources == emitter.error_terminal_sources and
                not (formats[name].store_dst and emitter.error_source is not None
                     and emitter.error_source.changes_origin))
        if programs is not None:
            programs[name] = tuple(emitter.ownership_events)
        if error_routes is not None:
            error_routes[name] = emitted_error_routes(emitter, formats[name])
        if checkpoints is not None:
            checkpoints[name] = (tuple(cp for cp in emitter.error_checkpoint_sites.values() if cp.id)
                if emitted_error_routes(emitter, formats[name]) else ())
        if normal_storage is not None:
            normal_storage[name] = (emitter.error_checkpoint_sites.get(0)
                if emitted_error_routes(emitter, formats[name]) else None)
    if programs is not None:
        # A pristine word can redispatch to any member. A successful final
        # vacate in one member does not certify an uncovered sibling.
        for members in redispatch_cohorts(analysis, formats):
            yielded = [programs[name] for name in members
                       if len(programs.get(name, ())) == 1 and
                       programs[name][0].kind == "_PyIR_OWNER_NORMAL_COMPLETION" and
                       programs[name][0].flags & 2]
            if yielded and not all(programs.get(name) == yielded[0] for name in members):
                for name in members:
                    programs[name] = tuple(
                        replace(event, flags=event.flags & ~2, extent=0)
                        if event.kind == "_PyIR_OWNER_NORMAL_COMPLETION" else event
                        for event in programs.get(name, ()))
            null_flags = 12
            for name in members:
                program = programs.get(name, ())
                null_flags &= (program[0].flags if len(program) == 1 and
                               program[0].kind == "_PyIR_OWNER_NORMAL_CLEAR_INPUTS" else 0)
            for name in members:
                program = programs.get(name, ())
                if len(program) == 1 and program[0].kind == "_PyIR_OWNER_NORMAL_CLEAR_INPUTS":
                    programs[name] = (replace(program[0], flags=(program[0].flags & ~12) | null_flags),)
            moved_results = [programs[name] for name in members
                             if len(programs.get(name, ())) == 1
                             and programs[name][0].kind == "_PyIR_OWNER_NORMAL_CLEAR_INPUTS"
                             and programs[name][0].flags & 1]
            if moved_results and not all(programs.get(name) == moved_results[0]
                                         for name in members):
                for name in members:
                    if programs.get(name) in moved_results:
                        event = programs[name][0]
                        programs[name] = (replace(event, flags=event.flags & ~1, src=0),)
            unpack_drain = (OwnershipEvent("_PyIR_OWNER_UNPACK_LOCAL_DRAIN"),)
            if not all(programs.get(name) == unpack_drain for name in members):
                for name in members:
                    if programs.get(name) == unpack_drain:
                        del programs[name]
            common_completions = set.intersection(*(
                {event for event in programs.get(name, ())
                 if event.kind == "_PyIR_OWNER_NORMAL_COMPLETION"}
                for name in members))
            for name in members:
                if name in programs:
                    programs[name] = tuple(event for event in programs[name]
                        if event.kind != "_PyIR_OWNER_NORMAL_COMPLETION" or
                        event in common_completions)
            if all(programs.get(name) for name in members):
                continue
            for name in members:
                program = programs.get(name, ())
                if len(program) == 1 and program[0].kind in (
                        "_PyIR_OWNER_NORMAL_CLEAR_INPUTS", "_PyIR_OWNER_CALL_DRAIN"):
                    del programs[name]
    if checkpoints is not None:
        for members in redispatch_cohorts(analysis, formats):
            common = checkpoints.get(members[0], ())
            if not all(checkpoints.get(name, ()) == common for name in members):
                for name in members:
                    checkpoints[name] = ()


ERROR_ROUTE_FLAGS = ("_PyIR_ERROR_COMPLETE", "_PyIR_ERROR_DIRECT",
                     "_PyIR_ERROR_LOCAL_DST")

OBSERVER_ENTRY_HELPERS = {
    "_Py_call_instrumentation_line", "_Py_call_instrumentation_instruction",
}


def observer_entry_source(inst: Instruction, fmt: IRFormat):
    """A transparent pre-word observer, independent of the body error census."""
    inputs, outputs = external_io(inst)
    if not fmt.transparent or inputs or outputs:
        return None
    source = analyze_error_effects(inst, HANDLER_LOCALS, observer_wrapper=True)
    if (source is None or source.homes != HomeEffect.PRESERVES_SCRATCH_ABSENCE
            or not source.helpers.intersection(OBSERVER_ENTRY_HELPERS)
            or source.unwind_sources):
        return None
    return source


def emitted_observer_entry(emitter: IREmitter, inst: Instruction) -> bool:
    source = emitter.observer_source
    calls = {id(token) for part in inst.parts if isinstance(part, Uop)
             for token in part.body.tokens() if token.text in OBSERVER_ENTRY_HELPERS}
    return bool(source is not None and calls and
                source.error_sources == emitter.observer_errors and
                source.terminal_sources == emitter.observer_terminals and
                calls == emitter.observer_calls)


def emitted_error_routes(emitter: IREmitter, fmt: IRFormat) -> tuple[str, ...]:
    """Match complete typed source to actual flush and terminal receipts."""
    source = emitter.error_source
    if (source is None or source.homes == HomeEffect.UNKNOWN
            or source.changes_origin and fmt.store_dst
            or emitter.error_uncovered or emitter.abrupt_operand_uncovered
            or source.error_sources != emitter.abrupt_operand_sources
            or source.unwind_sources != emitter.error_unwind_sources
            or source.terminal_sources != emitter.error_terminal_sources
            or fmt.store_dst and not emitter.error_local_dst
            or not (emitter.error_finalized or source.error_sources
                    or source.unwind_sources or source.terminal_sources)):
        return ()
    routes = [ERROR_ROUTE_FLAGS[0]]
    if source.error_sources or source.unwind_sources or source.may_fail:
        routes.append(ERROR_ROUTE_FLAGS[1])
    if emitter.error_local_dst:
        routes.append(ERROR_ROUTE_FLAGS[2])
    return tuple(routes)


@dataclass
class HandlerContract:
    "The columns of the contract that are the handler's own"
    effects: list[str]
    reads: Extent
    writes: Extent
    through: Extent
    twin: str | None
    result_phase: str
    output_kind: str
    observer_outcome: tuple[str, str]
    transfer_kind: str
    ownership_events: tuple[OwnershipEvent, ...]
    word_flags: tuple[str, ...]
    abrupt_prefix: bool
    abrupt_operands: bool
    error_routes: tuple[str, ...]
    observer_entry: bool
    error_checkpoints: tuple[ErrorCheckpoint, ...]
    normal_storage: ErrorCheckpoint | None = None


def statement_shape(node):
    """A source contract retaining preprocessor arms and their delimiters."""
    if isinstance(node, Token):
        return node.kind, node.text
    if isinstance(node, list):
        return tuple(statement_shape(item) for item in node
                     if not isinstance(item, Token) or item.kind != "COMMENT")
    if isinstance(node, Stmt):
        return type(node), tuple(statement_shape(getattr(node, field.name))
                                 for field in fields(node))
    assert node is None
    return None


def abrupt_suspend_contract(inst: Instruction, fmt: IRFormat) -> bool:
    """All own-frame writes of this complete suspension stay at A or B.

    The resume API writes the saved recipient, including throw entry; a
    close/throw execution claim alone does not change suspended homes. The
    checked yield observer allows named-local changes but rejects scratch
    redirects. Native bridges remain the existing counted C workspace.
    """
    if (not fmt.window or fmt.field_of(ROLE_DST) != "A"
            or fmt.field_of(ROLE_BASE) != "B" or fmt.dst_offset != 0):
        return False
    parts = [part for part in inst.parts if isinstance(part, Uop)]
    if len(parts) != len(inst.parts):
        return False
    expected = [
        (("value",), ("taken",), "{ taken = TAKE(value); }"),
        (("retval",), ("value",), """{
            assert(frame->owner != FRAME_OWNED_BY_INTERPRETER);
            frame->instr_ptr++;
            PyGenObject *gen = _PyGen_GetGeneratorFromFrame(frame);
            assert(FRAME_SUSPENDED_YIELD_FROM == FRAME_SUSPENDED + 1);
            assert(oparg == 0 || oparg == 1);
            PyValue temp = retval;
            DEAD(retval);
            SAVE_STACK();
            DTRACE_FUNCTION_RETURN();
            tstate->exc_info = gen->gi_exc_state.previous_item;
            gen->gi_exc_state.previous_item = NULL;
            _Py_LeaveRecursiveCallPy(tstate);
            _PyInterpreterFrame *gen_frame = frame;
            gen_frame->return_slot = (uint16_t)IR_DST_SLOT;
            _PyThreadState_UpdateLastProfiledFrame(tstate, gen_frame, gen_frame->previous);
            frame = tstate->current_frame = frame->previous;
            gen_frame->previous = NULL;
            ((_PyThreadStateImpl *)tstate)->generator_return_kind = GENERATOR_YIELD;
            FT_ATOMIC_STORE_INT8_RELEASE(gen->gi_frame_state, FRAME_SUSPENDED + oparg);
            #ifdef Py_DEBUG
            if (frame->f_executable != NULL) {
                Py_ssize_t i = INSTR_OFFSET_OF(frame->instr_ptr);
                assert(i >= 0 && i <= INT_MAX);
                int opcode = _PyIR_BaseOpcode(
                    _PyCode_InstrHandler(_PyFrame_GetCode(frame), (int)i));
                assert(opcode == SEND || opcode == FOR_ITER);
            }
            #endif
            RELOAD_STACK();
            next_instr = frame->return_pc;
            value = temp;
            LLTRACE_RESUME_FRAME();
        }"""),
    ]
    if len(parts) == 3:
        expected.insert(0, (("val",), ("val",), """{
            int err = _Py_call_instrumentation_arg(
                    tstate, PY_MONITORING_EVENT_PY_YIELD,
                    frame, this_instr, val);
            if (err) {
                RELOAD_STACK();
                ERROR_NO_POP();
            }
            if (frame->instr_ptr != this_instr) {
                RELOAD_STACK();
                DISPATCH_REDIRECT();
            }
        }"""))
    if len(parts) != len(expected):
        return False
    for part, (inputs, outputs, body) in zip(parts, expected):
        if (tuple(item.name for item in part.stack.inputs) != inputs
                or tuple(item.name for item in part.stack.outputs) != outputs
                or any(item.is_array() for item in
                       (*part.stack.inputs, *part.stack.outputs))
                or not canonical_body_tokens(part)
                or statement_shape(part.body) != statement_shape(
                    Parser(body, "<abrupt suspension footprint>").block())):
            return False
    return True


def binary_reader_contract(inst: Instruction, fmt: IRFormat) -> bool:
    """The structured transaction owns its body, not arbitrary caller C."""
    if any(not isinstance(part, (Uop, Skip)) for part in inst.parts):
        return False
    parts = [part for part in inst.parts if isinstance(part, Uop)]
    inputs, outputs = external_io(inst)
    if (fmt.window or len(inputs) != 2 or len(outputs) != 1
            or any(item.is_array() for item in inputs)
            or outputs[0].item.is_array() or fmt.field_of(ROLE_DST) is None
            or not parts):
        return False
    producer = parts[-1]
    expected = ("{ IR_BINARY_READER(" + ", ".join(
        [item.name for item in producer.stack.inputs]
        + [item.name for item in producer.stack.outputs]) + "); }")
    if statement_shape(producer.body) != statement_shape(
            Parser(expected, "<binary provider contract>").block()):
        return False
    if len(parts) == 1:
        return True
    # This audited specialization leaf exits by redispatch after its call;
    # there is no opaque callback followed by continuing borrowed inputs.
    specialize = """{
        #if ENABLE_SPECIALIZATION
        _PyFbHeader *feedback = FB_COUNTER();
        if (_PyFb_CounterTriggers(feedback)) {
            next_instr = this_instr;
            _Py_Specialize_BinaryOp(lhs, rhs, next_instr, oparg, LOCALS_ARRAY);
            DISPATCH_SAME_OPARG();
        }
        OPCODE_DEFERRED_INC(BINARY_OP);
        _PyFb_AdvanceCounter(feedback);
        #endif  /* ENABLE_SPECIALIZATION */
        assert(NB_ADD <= oparg);
        assert(oparg <= NB_OPARG_LAST);
    }"""
    return (len(parts) == 2
            and statement_shape(parts[0].body) == statement_shape(
                Parser(specialize, "<binary specialization contract>").block()))


def generator_marker_contract(inst: Instruction, fmt: IRFormat) -> bool:
    """The marker at the start of a generator's code is never executed."""
    inputs, outputs = external_io(inst)
    parts = [part for part in inst.parts if isinstance(part, Uop)]
    if (inst.name != "RETURN_GENERATOR" or not fmt.window or inputs
            or len(outputs) != 1 or len(parts) != 1
            or len(inst.parts) != 1):
        return False
    expected = """{
        IR_GENERATOR_MARKER();
        res = PyValue_NULL;
    }"""
    return statement_shape(parts[0].body) == statement_shape(
        Parser(expected, "<generator marker contract>").block())


def normal_transfer_kind(inst: Instruction, fmt: IRFormat) -> str:
    """Certify the bounded normal-edge subset from its actual owner operations.

    These facts do not describe callback/error phases. Changed or synthetic
    instruction bodies lose coverage instead of acquiring facts from a name.
    """
    unknown = "_PyIR_TRANSFER_UNKNOWN"
    if any(not isinstance(part, (Uop, Skip)) for part in inst.parts):
        return unknown
    parts = [part for part in inst.parts if isinstance(part, Uop)]
    inputs, outputs = external_io(inst)

    def scalar_shape(nin: int, nout: int) -> bool:
        return (not fmt.window and len(inputs) == nin and len(outputs) == nout
                and not any(item.is_array() for item in inputs)
                and not any(entry.item.is_array() for entry in outputs))

    def bodies_match(*bodies: str) -> bool:
        return len(parts) == len(bodies) and all(
            [token.text for token in part.body.tokens() if token.kind != "COMMENT"]
            == [token.text for token in tokenize(body)]
            for part, body in zip(parts, bodies))

    if inst.name in ("RESUME", "RESUME_JIT") and scalar_shape(0, 0):
        # These audited helper contracts may poll/instrument and change
        # control flow; ordinary completion writes no scratch. Require the
        # complete bodies, not a token blacklist that can miss pointer stores.
        # Callback/error phases remain uncovered.  RESUME_JIT ends in the
        # JIT's warm-up event, which moves no value.
        expected_parts = ["_MAYBE_INSTRUMENT", "_CHECK_PERIODIC_IF_NOT_YIELD_FROM"]
        if inst.name == "RESUME_JIT":
            expected_parts.append("_JIT_EVENT")
        if [part.name for part in parts] != expected_parts:
            return unknown
        instrument = """{
            #ifdef Py_GIL_DISABLED
            int check_instrumentation = 1;
            #else
            int check_instrumentation = (tstate->tracing == 0);
            #endif
            if (check_instrumentation) {
                uintptr_t global_version = _Py_atomic_load_uintptr_relaxed(&tstate->eval_breaker) & ~_PY_EVAL_EVENTS_MASK;
                uintptr_t code_version = FT_ATOMIC_LOAD_UINTPTR_ACQUIRE(_PyFrame_GetCode(frame)->_co_instrumentation_version);
                if (code_version != global_version) {
                    int err = _Py_Instrument(_PyFrame_GetCode(frame), tstate->interp);
                    if (err) { ERROR_NO_POP(); }
                    next_instr = this_instr;
                    DISPATCH_SAME_OPARG();
                }
            }
        }"""
        periodic = """{
            if ((oparg & RESUME_OPARG_LOCATION_MASK) < RESUME_AFTER_YIELD_FROM) {
                int err = check_periodics(tstate);
                ERROR_IF(err != 0);
            }
        }"""
        event = "{ IR_JIT_EVENT(); }"
        if (any(part.stack.inputs or part.stack.outputs for part in parts)
                or any(statement_shape(part.body)
                       != statement_shape(Parser(body, "<RESUME contract>").block())
                       for part, body in zip(parts, (instrument, periodic, event)))):
            return unknown
        return "_PyIR_TRANSFER_PRESERVE"

    if inst.name == "MAKE_CELL" and scalar_shape(0, 0):
        body = """{
            PyObject *cell = PyCell_NewValue(GETLOCAL(oparg));
            if (cell == NULL) { ERROR_NO_POP(); }
            PyValue tmp = GETLOCAL(oparg);
            GETLOCAL(oparg) = PyValue_FromObjectSteal(cell);
            PyValue_XCLOSE(tmp);
        }"""
        return "_PyIR_TRANSFER_PRESERVE" if bodies_match(body) else unknown
    if inst.name == "COPY_FREE_VARS" and scalar_shape(0, 0):
        body = """{
            PyCodeObject *co = _PyFrame_GetCode(frame);
            PyFunctionObject *func = _PyFrame_GetFunction(frame);
            PyObject *closure = func->func_closure;
            assert(oparg == co->co_nfreevars);
            int offset = co->co_nlocalsplus - oparg;
            for (int i = 0; i < oparg; ++i) {
                frame->base[offset + i] = PyValue_XDUP(PyTuple_GET_VALUE(closure, i));
            }
        }"""
        return "_PyIR_TRANSFER_PRESERVE" if bodies_match(body) else unknown

    scalar = {
        "LOAD_COMMON_CONSTANT": ("{ assert(oparg < NUM_COMMON_CONSTANTS); PyValueBorrow constant = "
                                 "_PyValue_CommonConstant(tstate->interp->common_consts[oparg]); "
                                 "value = PyValue_DUP(constant); }"),
    }
    if inst.name in scalar:
        if (scalar_shape(0, 1) and fmt.field_of(ROLE_DST) is not None
                and bodies_match(scalar[inst.name])):
            return "_PyIR_TRANSFER_FRESH"
        return unknown
    close = "{ PyValue_XCLOSE(value); }"
    if inst.name == "STORE_FAST":
        store = """{
            PyValue tmp = GETLOCAL(oparg);
            GETLOCAL(oparg) = value;
            _PyValue_CheckHome(value);
            DEAD(value);
            trash = tmp;
        }"""
        return "_PyIR_TRANSFER_LOCAL_STORE" if scalar_shape(1, 0) and bodies_match(store, close) else unknown
    contracts = [ir_contract.declared_contract(part) for part in parts]
    exchanged = {o.name for c in contracts for o in c.outputs if o.exchange}
    if exchanged:
        # A declared exchange (ir_contract.check_exchange, verify_exchange)
        # of the first and last items of an oparg window: the swap
        items = parts[0].stack.inputs if len(parts) == 1 else []
        return "_PyIR_TRANSFER_SWAP" if (fmt.window and len(items) == 3
            and exchanged == {items[0].name, items[-1].name}
            and items[1].name == "unused"
            and extent(inputs, inst) == (0, "_PyIR_EXT_OPARG")
            and extent([entry.item for entry in outputs], inst) == (0, "_PyIR_EXT_OPARG")
            ) else unknown
    if inst.name == "IR_JUMP":
        return "_PyIR_TRANSFER_PRESERVE" if scalar_shape(0, 0) and bodies_match("{ JUMPTO(oparg); }") else unknown
    if (inst.name in {"RETURN_VALUE", "INSTRUMENTED_RETURN_VALUE",
                      "RETURN_VALUE_EMPTY", "INSTRUMENTED_RETURN_VALUE_EMPTY"} and
            canonical_frame_transfer_input(inst)):
        return "_PyIR_TRANSFER_EXIT"
    return unknown


def handler_output_kind(inst: Instruction, fmt: IRFormat) -> str:
    """Physical output locations, using the emitter's store mode.

    Window flush always writes the frontend slots. STORE_RESULT then moves
    only the first result to dst; a variable trailing output stays in place.
    Frame-return forms use the same explicit recipient without that flush.
    """
    if not fmt.window or fmt.operands != "_PyIR_OPERANDS_NONE":
        return "_PyIR_OUTPUT_REGISTER_DST"
    if set(uop_tokens(inst)) & {"IR_COMPLETE_STEP", "IR_ENTER_GENERATOR"}:
        return "_PyIR_OUTPUT_OUTCOME"
    mode = store_mode(inst, external_io(inst)[1]) if fmt.store_dst else None
    if mode == STORE_LEAVE:
        raise analysis_error("unclassified window output adapter", inst.where)
    if mode in (STORE_RESULT, STORE_RETURN):
        if fmt.dst_offset is None:
            raise analysis_error("window result has no physical offset", inst.where)
        return "_PyIR_OUTPUT_WINDOW_DST_FIRST"
    return "_PyIR_OUTPUT_WINDOW"


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
    canonical_source: bool = False


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
    "No handler publishes its result before its inputs close (rule 7)."
    return None


def handler_effects(record: OperationRecord) -> list[str]:
    "The _PyIR_* effect bits of a handler: a projection of its record"
    effects = set().union(*(uop.effects for uop in record.uops))
    transfers = {t.kind for uop in record.uops for t in uop.transfers}
    bits = []
    for bit, present in (
            ("_PyIR_RAISES", MAY_RAISE in effects),
            ("_PyIR_ESCAPES", bool(effects & RUNS_PYTHON)),
            # An instrumented twin fires a monitoring event, whose callback
            # is Python
            ("_PyIR_RUNS_PYTHON", bool(effects & RUNS_PYTHON)
                                  or record.handler.startswith("INSTRUMENTED_")),
            ("_PyIR_DEOPTS", DEOPT in transfers),
            ("_PyIR_SUSPENDS", YIELD in transfers),
            ("_PyIR_PUSHES_FRAME", ENTER_FRAME in transfers),
            ("_PyIR_POLLS", record.polls),
            ("_PyIR_PURE", bool(record.uops) and all(uop.pure for uop in record.uops)),
            ("_PyIR_SPECIALIZES", record.specializes),
            ("_PyIR_LEARNED_ONLY", record.reads_poly_table)):
        if present:
            bits.append(bit)
    return bits


def handler_contracts(analysis: Analysis,
                      formats: dict[str, IRFormat]) -> dict[str, HandlerContract]:
    "The handler's own columns of the contract, by handler name"
    ids = handler_ids(analysis)
    programs = {}
    abrupt = {}
    operand_abrupt = {}
    error_routes = {}
    observer_entries = {}
    checkpoints = {}
    normal_storage = {}
    analyze_handlers(analysis, formats, programs, abrupt, operand_abrupt,
                     error_routes, observer_entries, checkpoints, normal_storage)
    for members in redispatch_cohorts(analysis, formats):
        # Stack-only formats have no independently emitted family members.
        # They cannot acquire a storage certificate from an empty intersection.
        if not members:
            continue
        if not all(abrupt.get(name, False) for name in members):
            for name in members:
                abrupt[name] = False
        if not all(operand_abrupt.get(name, False) for name in members):
            for name in members:
                operand_abrupt[name] = False
        # A word may run any specialization or its instrumented twin. Every
        # member must have a complete route census; possible routes union.
        if all(error_routes.get(name) for name in members):
            combined = set().union(*(error_routes[name] for name in members))
            common = tuple(flag for flag in ERROR_ROUTE_FLAGS if flag in combined)
        else:
            common = ()
        for name in members:
            error_routes[name] = common
        # A pristine word may execute any sibling. The first normal action
        # cohort requires identical reached transport on every such route.
        first = normal_storage.get(members[0])
        if not all(normal_storage.get(name) == first for name in members):
            for name in members:
                normal_storage[name] = None
    result = {}
    insts = {**handler_instructions(analysis),
             **{name: analysis.instructions[name] for name in STACK_ONLY
                if name in analysis.instructions}}
    # A family's baseline(...) arm runs in compiled code only, never on a
    # word, so instrumentation never replaces it: it has no twin
    baselines = {family.baseline for family in analysis.families.values()}
    for name, inst in insts.items():
        fmt = formats[name]
        record = build_record(name, inst, fmt, analysis, formats)
        publication = result_publication_contract(inst, fmt)
        result[name] = HandlerContract(
            effects=handler_effects(record),
            reads=extent(list(record.inputs), inst),
            writes=extent([item for _, _, item, _ in record.outputs], inst),
            through=extent(record_through(record), inst),
            twin=None if name in baselines else twin_of(name, fmt, ids),
            result_phase=(publication.phase if publication is not None
                          else RESULT_PHASE_NONE),
            output_kind=handler_output_kind(inst, fmt),
            observer_outcome=observer_outcomes(inst, fmt),
            transfer_kind=normal_transfer_kind(inst, fmt),
            ownership_events=programs.get(name, ()),
            # A stack-only op has no handler, so no word admits flags
            word_flags=() if name in STACK_ONLY else call_role_flags(name, inst),
            abrupt_prefix=abrupt.get(name, False),
            abrupt_operands=operand_abrupt.get(name, False),
            error_routes=error_routes.get(name, ()),
            observer_entry=observer_entries.get(name, False),
            error_checkpoints=checkpoints.get(name, ()),
            normal_storage=normal_storage.get(name),
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
    "window", "oparg", "fb", "opcode", "ir_old",
    "ir_rest", "ir_expanded", "call_base",
    "call_destination", "call_state", "call_result", "call_feedback",
    "retire_needed", "retire_failed", "operation_failed",
    "ir_binary_left", "ir_binary_right", "ir_binary_result",
    "ir_binary_rest",
    "ir_shape_offset",
}
HANDLER_POINTERS = {"slots", "window", "fb", "frame", "tstate",
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
                lines: bool, stubs_outfile: TextIO | None = None,
                arms: Sequence[ir_arms.ArmEntry] = ()) -> None:
    formats = compute_formats(analysis)
    check_no_addressable_locals(analysis)
    checkpoints = {}
    analyze_handlers(analysis, formats, checkpoints=checkpoints)
    write_header(__file__, filenames, outfile)
    cases = io.StringIO()
    stubs: dict[str, str] = {}
    functions = generate_ir_cases(analysis, formats, cases, lines, checkpoints, stubs)
    if stubs_outfile is not None:
        generate_ir_stubs(analysis, formats, stubs, stubs_outfile, arms)
    # The file is included twice by Python/ceval.c: at file scope for the
    # function forms of every handler and entry label, and in ir_evaluate()
    # for the evaluator's handlers and labels
    outfile.write("\n#ifdef IR_FUNCTION_FORMS\n")
    outfile.write("\n".join(ir_contract.component_checks()) + "\n\n")
    outfile.write(form_declarations(analysis, arms))
    outfile.writelines(functions)
    outfile.writelines(label_forms(analysis, formats, lines))
    outfile.write(form_table(analysis, arms))
    outfile.write("#else  // IR_FUNCTION_FORMS\n")
    outfile.write(f"""
            {INSTRUCTION_START_MARKER}
""")
    outfile.write(cases.getvalue())
    outfile.write(f"""
            {INSTRUCTION_END_MARKER}
        {LABEL_START_MARKER}
""")
    out = CWriter(outfile, 2, lines)
    emitter = IREmitter(out, analysis, formats)
    generate_ir_labels(analysis, emitter)
    outfile.write(f"{LABEL_END_MARKER}\n")
    outfile.write("#endif  // IR_FUNCTION_FORMS\n")


def num_old_handlers(analysis: Analysis) -> int:
    """The arms start here.  The IR-only handlers fill the free numbers below
    256 and continue from 256, so the arms' range stays contiguous.  If they
    do not fill the free numbers, the rest stay unused."""
    return 256 + max(0, len(ir_only_handler_names(analysis)) - len(analysis.free_ids))


def num_handlers(analysis: Analysis, arms: Sequence[ir_arms.ArmEntry]) -> int:
    """Every handler id: the old handlers, the register instruction arms and
    the observed wrappers of the arms, which follow them.  The ids must fit
    the handler field, IR_LIMIT of them."""
    total = num_old_handlers(analysis) + len(arms)
    if total > ir_arms.IR_LIMIT:
        raise ir_arms.HandlerLimitError(
            f"{num_old_handlers(analysis)} old handlers and {len(arms)} arms and "
            f"wrappers need {total} handler ids, past the {ir_arms.IR_LIMIT} "
            "that a handler field holds")
    return total


def generate_ir_targets(filenames: list[str], analysis: Analysis,
                        outfile: TextIO,
                        arms: Sequence[ir_arms.ArmEntry] = ()) -> None:
    """The tables of the evaluator.  Python/ceval_generator.h includes this
    file at file scope with IR_GEN_TABLES defined, for the generator transfer
    tables, which give the kind of transfer that a word's handler makes (the
    enums of that file).
    ir_evaluate() includes it at its start without, for the dispatch table,
    which is local to it and has the addresses of its labels."""
    ids = handler_ids(analysis)
    formats = compute_formats(analysis)
    n = num_old_handlers(analysis)
    write_header(__file__, filenames, outfile)
    out = CWriter(outfile, 0, False)
    boundaries = generator_boundaries(analysis)
    out.emit("#ifdef IR_GEN_TABLES\n")
    for column in ("Yielded", "Completed", "Resume"):
        out.emit(f"static const uint8_t generator_{column.lower()}_targets[{n}] = {{\n")
        for name, boundary in sorted(boundaries.items()):
            if not generator_boundary_source_closed(boundary, analysis, formats):
                continue
            target = None
            if column == "Yielded" and boundary.yielded == "DELEGATED":
                target = "1"
            elif column == "Completed" and boundary.completed != "NONE":
                target = ("GEN_THROWING | " if boundary.completion == "throw" else ""
                          ) + GENERATOR_COMPLETIONS[boundary.completed][0]
            elif column == "Resume":
                if boundary.resume != "NONE":
                    target = GENERATOR_RESUMES[boundary.resume]
                elif boundary.yielded == "DELEGATED":
                    target = ("GEN_SEND_INPUT" if boundary.base == "SEND_GEN"
                              else "GEN_SEND_INPUT_HOME")
            if target is not None:
                out.emit(f"[{ids[name]}] = {target},\n")
        if column == "Completed":
            # A generator that ends a FOR loop completes it with one outcome
            for name in ("FOR_ITER", "FOR_ITER_GEN"):
                if name in ids:
                    out.emit(f"[{ids[name]}] = GEN_COMPLETE_FOR_ITER,\n")
        out.emit("};\n")
    out.emit("#else\n")
    # Dispatch masks the word with IR_HANDLER_MASK, so the table covers every
    # id the mask yields; ids past n are UNKNOWN_OPCODE.
    by_index = {i: name for name, i in ids.items()}
    out.emit(f"static void *const opcode_targets[{ir_arms.IR_LIMIT}] = {{\n")
    by_arm = arm_ids(analysis, arms)
    for i in range(ir_arms.IR_LIMIT):
        if i in by_arm:
            out.emit(f"[{i}] = &&TARGET_{by_arm[i].name},\n")
            continue
        name = generator_word_entry(by_index.get(i, "UNKNOWN_OPCODE"), analysis)
        out.emit(f"[{i}] = &&TARGET_{name},\n")
    out.emit("};\n")
    out.emit(f"static_assert(Py_ARRAY_LENGTH(opcode_targets) == IR_HANDLER_MASK + 1,\n")
    out.emit('              "dispatch covers every id that IR_HANDLER yields");\n')
    out.emit("#endif\n")


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
    "-s", "--stubs", type=str, help="Generated completion stubs",
    default=DEFAULT_STUBS_OUTPUT
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
    # The register instruction arms follow the old handlers in every table
    import rop_generator
    arms = rop_generator.load_arm_entries()
    with open(args.output, "w") as outfile, open(args.stubs, "w") as stubs_outfile:
        generate_ir(args.input, data, outfile, args.emit_line_directives,
                    stubs_outfile, arms)
    with open(args.targets, "w") as outfile:
        generate_ir_targets(args.input, data, outfile, arms)
