"""Operation records of the register IR handlers.

The record of a handler is the generator's model of it: its uops in order,
each uop's inputs and outputs bound to values and the values to the word's
operands, the ownership of each item, the uop's effects, its guards and the
facts they establish, the ways it leaves the handler, and the feedback
fields it reads.  ir_generator.build_record() builds it before the C is
printed, and the C emitter takes its decisions from it and marks each
element that it prints, so that the record and the C cannot drift
(ir_generator.IREmitter.check_record_consumed()).  The compilers of the
tiers87 plan (pyjit/design/proposals/tiers87.md, C1) read the same records
through build_records().

This module holds the parts of the model that need no word format: the
value walk over an instruction's uops, the transfer table, the facts of
guards, the effects of uops, the checks of a record (check_record()), the
feedback schema and the dump.  Run it as a script to print every handler's
record.
"""

from dataclasses import dataclass
import re

import analyzer

from analyzer import (CALL_INVOKE_MARKERS, IR_MARKER_EFFECTS, Instruction, StackItem,
                      Uop, analysis_error)
from generators_common import ROOT
from lexer import Token
from parser import BlockStmt, ForStmt, IfStmt, MacroIfStmt, SimpleStmt, Stmt, WhileStmt


# Operand kinds: the parts of a word that a handler reads or writes
SLOT = "SLOT"                  # a field naming a frame slot
WINDOW = "WINDOW"              # a slot at an offset from the base field
CALL_OPERAND = "CALL_OPERAND"  # a home that an operand word of a call names
IMMEDIATE = "IMMEDIATE"        # the oparg
TARGET = "TARGET"              # a jump target or forward distance
FEEDBACK = "FEEDBACK"          # the feedback entry

# What a SLOT field may name at run time.  A local or a constant slot is
# read in place, as a view that owns no count; a temporary is owned by the
# handler once it detaches it.
LOCAL, CONST, TEMP = "LOCAL", "CONST", "TEMP"

# The ownership of a uop's inputs and outputs
BORROWED = "BORROWED"   # input: peeked, the uop passes it through
CONSUMED = "CONSUMED"   # input: the uop releases or moves it
THROUGH = "THROUGH"     # output: the input of the same position, unchanged
OWNED = "OWNED"         # output: a new value with a count of its own
ALIAS = "ALIAS"         # output: an input's value, whose count moves to it
INLINE = "INLINE"       # output: an inline value or an immortal, no count


@dataclass(frozen=True)
class Operand:
    kind: str
    field: str | None = None           # "A".."D", "CD", "FBARG", or None
    may_name: frozenset[str] = frozenset()
    position: int | None = None        # WINDOW: offset from base; CALL_OPERAND: index
    detail: str = ""

    def __str__(self) -> str:
        text = self.kind
        if self.field is not None:
            text += f" {self.field}"
        if self.position is not None:
            text += f"[{self.position}]"
        if self.may_name:
            text += " {" + ",".join(n for n in (LOCAL, CONST, TEMP)
                                    if n in self.may_name) + "}"
        if self.detail:
            text += f" {self.detail}"
        return text


@dataclass(frozen=True)
class Value:
    "A value inside a handler: an external input, or a uop's new output"
    id: int
    name: str
    external: int = -1          # index of the external input, bottom first
    defined_by: int = -1        # the index of the uop that defines it
    array: str = ""             # the size of an array value


@dataclass(frozen=True)
class UopInput:
    name: str
    value: int
    ownership: str              # BORROWED or CONSUMED
    array: str = ""
    changed: bool = False       # the uop may write it in place (changes())


@dataclass(frozen=True)
class UopOutput:
    name: str
    value: int
    ownership: str              # THROUGH, OWNED, ALIAS or INLINE
    array: str = ""


@dataclass(frozen=True)
class SchemaField:
    "A field of a feedback entry type, from Include/internal/pycore_feedback.h"
    name: str
    ctype: str
    union: int = -1             # the members of one union share a number

    @property
    def is_pointer(self) -> bool:
        return self.ctype.endswith("*")


@dataclass(frozen=True)
class FeedbackSchema:
    family: str
    type: str                   # the entry type, _PyFbAttr and so on
    fields: tuple[SchemaField, ...]

    def field(self, name: str) -> SchemaField | None:
        for f in self.fields:
            if f.name == name:
                return f
        return None


@dataclass(frozen=True)
class FeedbackOperand:
    "A cache operand of a uop, `type_version/2`, bound to its field"
    name: str
    field: SchemaField


# Transfer kinds: the ways a uop may leave or continue
NEXT = "NEXT"                   # the next uop, or the next word
BRANCH = "BRANCH"               # a conditional jump to the word's target
JUMP = "JUMP"                   # an unconditional jump
DEOPT = "DEOPT"                 # a guard failed: the family's generic form
RAISE = "RAISE"                 # an error exit
ENTER_FRAME = "ENTER_FRAME"     # runs a Python frame that it pushed
RETURN = "RETURN"               # pops the frame
YIELD = "YIELD"                 # suspends a generator
EXIT = "EXIT"                   # leaves the evaluator
REDIRECT = "REDIRECT"           # a monitoring callback moved the frame: the
                                # word it chose, without the rest of the handler

TRANSFER_KINDS = frozenset({NEXT, BRANCH, JUMP, DEOPT, RAISE, ENTER_FRAME,
                            RETURN, YIELD, EXIT, REDIRECT})

# Guard kinds
EXIT_GUARD, DEOPT_GUARD = "EXIT", "DEOPT"


@dataclass(frozen=True)
class Transfer:
    kind: str
    site: Token | None = None
    target: str = ""


@dataclass(frozen=True)
class Guard:
    kind: str                   # EXIT or DEOPT
    site: Token | None          # None for the implicit object guard
    input: str = ""             # the implicit object guard: its input
    facts: tuple["Fact", ...] = ()
    reason: str = ""            # why a guard with no fact holds


@dataclass(frozen=True)
class UopRecord:
    name: str
    inputs: tuple[UopInput, ...]
    outputs: tuple[UopOutput, ...]
    feedback: tuple[FeedbackOperand, ...]
    uses_oparg: bool
    guards: tuple[Guard, ...]
    transfers: tuple[Transfer, ...]
    effects: frozenset[str] = frozenset()
    results: tuple["Fact", ...] = ()   # what holds of its new outputs
    pure: bool = False          # annotated `pure`: removable if its outputs
                                # are unused (Python/regcfg_passes.c)
    leaf: object = None         # ir_leaf.LeafBody: the body as leaf IR, if
                                # it is inside the leaf subset (ir_leaf.py)


@dataclass(frozen=True)
class CallShape:
    "How a call handler's window is a call (call_run_*() in Python/ceval.c)"
    kind: str                   # CALL_KEYWORDS or CALL_EX
    run: str                    # the call_run_* function
    flags: str                  # its CALL_* flags, a C expression
    argc: str                   # the argument count, a C expression


@dataclass(frozen=True)
class OperationRecord:
    handler: str
    instruction: Instruction
    format: object              # ir_generator.IRFormat: the word layout
    form: str                   # EMITTED, SHARED_BODY, EDGE_TWIN or EVENT_TWIN
    family: str | None
    operands: tuple[Operand, ...]
    values: tuple[Value, ...]
    uops: tuple[UopRecord, ...]
    results: tuple[tuple[int, Operand], ...]
    store_mode: str | None
    layout: object              # ir_generator.SlotLayout, from the operands
    feedback: FeedbackSchema | None
    uses_feedback: bool         # the handler reads its entry
    operation: str | None       # a BINARY_OP operation handler: NB_*
    oparg: str                  # the IMMEDIATE operand's detail, or ""
    call: CallShape | None
    generator_boundary: object  # ir_generator.GeneratorBoundary, if closed
    deopt: str                  # where a failed guard goes, see deopt_target()
    inputs: tuple[StackItem, ...]   # the instruction's stack inputs, bottom first
    # What it leaves on the stack, bottom first (ValueWalk.outputs)
    outputs: tuple[tuple[int, int, StackItem, bool], ...]
    polls: bool                 # it or its call transaction polls the eval breaker
    specializes: bool           # a generic handler whose entry's counter
                                # drives specialisation
    reads_poly_table: bool      # needs the site's learned table of cases:
                                # a poly execution variant or a
                                # POLY_TABLE_READERS macro


def record_through(record: OperationRecord) -> list[StackItem]:
    """The deepest inputs that keep their slot and their value: each is the
    output at its own position, passed through or an array of the same
    name and size left alone, and no uop writes it in place.  The count
    stops at the first input that does not, so that an unchanged input
    above a changed one, as the middle of SWAP, is not counted."""
    changed = {i.value for uop in record.uops for i in uop.inputs if i.changed}
    result = []
    for j, (item, (_, external, out, _)) in enumerate(zip(record.inputs, record.outputs)):
        # External input j is value j (walk_values())
        if j in changed:
            break
        if external == j or (item.is_array() and out.is_array()
                             and item.size == out.size and item.name == out.name):
            result.append(item)
        else:
            break
    return result


# The tokens of a uop body that leave the uop or may: each is a site of a
# transfer that the emitter prints.  A marker of IR_MARKER_EFFECTS that can
# leave must be here (check_transfer_tokens()).
TRANSFER_TOKENS: dict[str, tuple[str, ...]] = {
    "ERROR_IF": (RAISE,),
    "ERROR_NO_POP": (RAISE,),
    "DISPATCH": (NEXT,),
    "DISPATCH_GOTO": (JUMP,),
    "DISPATCH_SAME_OPARG": (JUMP,),
    "DISPATCH_REDIRECT": (REDIRECT,),
    "DISPATCH_INLINED": (ENTER_FRAME,),
    "JUMPBY": (JUMP,),
    "JUMPTO": (JUMP,),
    "IR_BINARY_READER": (RAISE,),
    "IR_OBSERVE_COMMITTED_BRANCH": (BRANCH, RAISE),
    "IR_COMPLETE_STEP": (BRANCH, RAISE),
    "IR_PROCESS_ERROR": (RAISE,),
    "IR_RETURN_TO_CALLER": (RETURN, RAISE),
    "IR_FINISH_GENERATOR_RETURN": (RETURN, RAISE),
    "IR_FINISH_GENERATOR_RETURN_EMPTY": (RETURN, RAISE),
    "IR_ENTER_GENERATOR": (ENTER_FRAME, RAISE),
    "IR_SUSPEND_DELEGATED": (YIELD,),
    "IR_TRY_DICT_FIRST_PROBE": (RAISE,),
    "IR_PREPARE_CALL_METHOD": (RAISE,),
    "IR_PREPARE_CALL_TUPLE": (RAISE,),
    "IR_MONITOR_CALL": (RAISE,),
    "IR_REFRESH_CALL": (RAISE,),
    "IR_INVOKE_CALL": (ENTER_FRAME, RAISE),
    "IR_INVOKE_GUARDED_PYTHON_CALL": (ENTER_FRAME, RAISE),
    "IR_RETIRE_PUBLISHED": (RAISE,),
    "IR_RETIRE_PUBLISHED_RESULT": (RAISE,),
    "IR_RETIRE_PUBLISHED_PAIR": (RAISE,),
    "IR_FINISH_VALUE_OPERATION": (RAISE,),
    "IR_ABORT_VALUE_READER": (RAISE,),
    "IR_COMPLETE_VALUE_READER": (RAISE,),
    "IR_COMPLETE_ATTR_READER": (RAISE,),
}

# The guard sites of a uop body, and the exit each takes when it fails
GUARD_TOKENS = {
    "EXIT_IF": EXIT_GUARD,
    "DEOPT_IF": DEOPT_GUARD,
    "IR_GUARD_INLINE_INT": EXIT_GUARD,
    "IR_GUARD_ELEMENT_TYPE": EXIT_GUARD,
    "IR_GUARD_ELEMENT_INDEX": EXIT_GUARD,
    "IR_GUARD_SHAPE_READ": EXIT_GUARD,
}

# The targets of the jumps that run the same word again: with its handler,
# or with the handler that the uop chose (`opcode`)
JUMP_TARGETS = {"DISPATCH_SAME_OPARG": "self", "DISPATCH_GOTO": "self:handler"}

# The labels of bytecodes.c that a `goto` in a body may name, which raise
ERROR_LABELS = ("error", "exception_unwind", "pop_1_error", "pop_2_error",
                "pop_3_error", "pop_4_error")

SITE_TOKENS = frozenset(TRANSFER_TOKENS) | frozenset(GUARD_TOKENS)


def check_transfer_tokens() -> None:
    "Every marker that can leave a uop has a transfer"
    for marker, effects in IR_MARKER_EFFECTS.items():
        if effects & {"terminal", "error", "frame"} and marker not in TRANSFER_TOKENS:
            raise ValueError(f"marker {marker} leaves the uop but has no transfer "
                             "in ir_records.TRANSFER_TOKENS")


check_transfer_tokens()


def replaced_input(uop: Uop, j: int) -> int | None:
    """The index of the input whose stack entry the declared output j takes
    over with a new value, or None: its replaces(x) (F16's cursor), at x's
    position (analyzer.analyze_stack)"""
    if j >= len(uop.stack.outputs) or not uop.stack.outputs[j].replaces:
        return None
    return j


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
        return replaced_input(uop, j)
    if j < len(inputs) and not any(
            item.is_array() for item in inputs[:j + 1] + outputs[:j + 1]):
        return j
    return None


ASSIGNMENTS = {"EQUALS", "TIMESEQUAL", "DIVEQUAL", "MODEQUAL", "PLUSEQUAL",
               "MINUSEQUAL", "LSHIFTEQUAL", "RSHIFTEQUAL", "ANDEQUAL",
               "OREQUAL", "XOREQUAL", "PLUSPLUS", "MINUSMINUS"}
# What may stand before an & that is a binary operator, not address-of
OPERANDS = {"IDENTIFIER", "NUMBER", "RPAREN", "RBRACKET"}


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


def assigns_to(uop: Uop, name: str) -> bool:
    "Whether the body of uop assigns to the variable name"
    tokens = list(uop.body.tokens())
    return any(
        tkn.text == name and nxt.kind == "EQUALS"
        and prev.kind not in ("ARROW", "PERIOD")
        for prev, tkn, nxt in zip(tokens, tokens[1:], tokens[2:])
    )


def passes(uop: Uop, item: StackItem) -> bool:
    """Whether output item, named as an input, passes that input's value
    through: declared without a mode and not exchanged.  An exchanged output
    (SWAP) keeps its home but takes another input's value"""
    if item.name == "unused":
        return True
    import ir_contract
    output = ir_contract.declared_contract(uop).output(item.name)
    return not output.mode and not output.exchange


def ends_handler(uop: Uop) -> bool:
    """Whether no uop after uop runs: a terminal call transaction owns the
    call's result and its return, so the uops after it do not execute"""
    return any(token.text in CALL_INVOKE_MARKERS for token in uop.body.tokens())


@dataclass
class ValueWalk:
    """The values of an instruction: its external inputs, bottom first, then
    the new outputs of its uops in order.  uops holds each uop that runs,
    with its bound inputs and outputs.  outputs is what the instruction
    leaves on the stack, bottom first: a value; the external input that the
    entry passes on, or -1; the item that defined the entry; and whether
    that item is assigned from an input (`out = in;`)."""
    values: list[Value]
    inputs: list[StackItem]
    uops: list[tuple[Uop, tuple[UopInput, ...], tuple[UopOutput, ...]]]
    outputs: list[tuple[int, int, StackItem, bool]]


def walk_values(inst: Instruction) -> ValueWalk:
    """Bind every stack item of inst's uops to a value.  The one walk over
    the uops' stack effects: the format (external inputs and outputs) and
    the record's bindings both come from it."""
    # Values are numbered in order of discovery first, external inputs top
    # first as the walk meets them, and renumbered at the end
    found: list[tuple[str, int, int, str]] = []   # name, external, uop, array
    # What each stack entry holds for the format: the external input it
    # passes on or the item that defined it, and whether that is an alias.
    # A uop that assigns an input it passes through makes a new value in
    # the same entry.
    origins: list[tuple[int, StackItem, bool]] = []
    external: list[StackItem] = []               # top first
    stack: list[tuple[int, int]] = []            # value, origin
    bound: list[tuple[Uop, list, list]] = []
    for index, part in enumerate(p for p in inst.parts if isinstance(p, Uop)):
        popped: list[tuple[int, int]] = []  # top first
        for item in reversed(part.stack.inputs):
            if stack:
                entry = stack.pop()
            else:
                found.append((item.name, len(external), -1, item.size))
                origins.append((len(external), item, False))
                external.append(item)
                entry = (len(found) - 1, len(origins) - 1)
            popped.append(entry)
        popped.reverse()
        inputs = []
        kept: set[int] = set()
        outputs = []
        for j, item in enumerate(part.stack.outputs):
            i = passed_through(part, j)
            if i is not None and passes(part, item):
                kept.add(i)
                stack.append(popped[i])
                outputs.append((item.name, popped[i][0], THROUGH, item.size))
                continue
            if (i is None and item.name == "unused" and j < len(part.stack.inputs)
                    and part.stack.inputs[j].name == "unused"
                    and part.stack.inputs[j].size == item.size):
                # An unused array that the uop leaves where it is: the same
                # values, although the format's walk cannot place it
                kept.add(j)
                origins.append((-1, item, False))
                stack.append((popped[j][0], len(origins) - 1))
                outputs.append((item.name, popped[j][0], THROUGH, item.size))
                continue
            aliased = item.name in part.aliased_outputs
            found.append((item.name, -1, index, item.size))
            if i is not None:
                origin = popped[i][1]
            else:
                origins.append((-1, item, aliased))
                origin = len(origins) - 1
            value = len(found) - 1
            stack.append((value, origin))
            outputs.append((item.name, value, ALIAS if aliased else OWNED, item.size))
        for i, item in enumerate(part.stack.inputs):
            inputs.append((item.name, popped[i][0],
                           BORROWED if i in kept else CONSUMED, item.size,
                           item.name != "unused" and (
                               changes(part, item.name, item.is_array())
                               # a replaced cursor is a new value in its entry
                               or (i < len(part.stack.outputs)
                                   and replaced_input(part, i) is not None))))
        bound.append((part, inputs, outputs))
        if ends_handler(part):
            break
    n = len(external)
    # External inputs first, bottom first, then the rest in order
    order = sorted(range(len(found)), key=lambda v: (
        (0, n - 1 - found[v][1]) if found[v][1] >= 0 else (1, v)))
    renumber = {old: new for new, old in enumerate(order)}
    values = [Value(renumber[old],
                    found[old][0],
                    n - 1 - found[old][1] if found[old][1] >= 0 else -1,
                    found[old][2], found[old][3]) for old in order]
    uops = [(part,
             tuple(UopInput(name, renumber[v], own, size, changed)
                   for name, v, own, size, changed in ins),
             tuple(UopOutput(name, renumber[v], own, size) for name, v, own, size in outs))
            for part, ins, outs in bound]
    outputs = []
    for v, origin in stack:
        ext, item, aliased = origins[origin]
        outputs.append((renumber[v], n - 1 - ext if ext >= 0 else -1, item, aliased))
    return ValueWalk(values, list(reversed(external)), uops, outputs)


def site_tokens(uop: Uop) -> list[Token]:
    "The guard and transfer sites of uop's body, in order"
    return [t for t in uop.body.tokens()
            if t.kind == "GOTO" or t.kind == "RETURN" or
            (t.kind == "IDENTIFIER" and t.text in SITE_TOKENS)]


def goto_label(uop: Uop, goto: Token) -> str:
    tokens = list(uop.body.tokens())
    return tokens[tokens.index(goto) + 1].text


def conditional(stmts: list[Stmt], token: Token, inside: bool = False) -> bool | None:
    """Whether token is inside an if, else or loop body among stmts, or
    None if it is not in them"""
    for stmt in stmts:
        if isinstance(stmt, SimpleStmt):
            if token in stmt.contents:
                return inside
        elif isinstance(stmt, BlockStmt):
            found = conditional(stmt.body, token, inside)
            if found is not None:
                return found
        elif isinstance(stmt, IfStmt):
            if token in stmt.condition:
                return inside
            for body in (stmt.body, stmt.else_body):
                if body is not None:
                    found = conditional([body], token, True)
                    if found is not None:
                        return found
        elif isinstance(stmt, MacroIfStmt):
            for body in (stmt.body, stmt.else_body or []):
                found = conditional(body, token, inside)
                if found is not None:
                    return found
        elif isinstance(stmt, (ForStmt, WhileStmt)):
            if token in stmt.condition:
                return True
            found = conditional([stmt.body], token, True)
            if found is not None:
                return found
    return None


# The declared exits that continue in another frame, and their transfers
SWITCH_TRANSFERS = (("enter", ENTER_FRAME), ("return", RETURN), ("yield", YIELD))


def uop_transfers(uop: Uop, deopt_target: str) -> tuple[Transfer, ...]:
    """The transfers of uop, in body order: one per site, then the frame
    switch that its properties give, and NEXT if it can fall through"""
    result: list[Transfer] = []
    for token in site_tokens(uop):
        if token.text in GUARD_TOKENS:
            result.append(Transfer(DEOPT, token, deopt_target))
        elif token.kind == "GOTO":
            label = goto_label(uop, token)
            if label not in ERROR_LABELS:
                raise analysis_error(f"{uop.name}: `goto {label}` leaves for the "
                                     "evaluator; a uop may goto only an error label",
                                     token)
            result.append(Transfer(RAISE, token, label))
        elif token.kind == "RETURN":
            result.append(Transfer(EXIT, token))
        else:
            for kind in TRANSFER_TOKENS[token.text]:
                target = JUMP_TARGETS.get(token.text, "")
                if kind == JUMP and not target and conditional(uop.body.body, token):
                    kind = BRANCH
                result.append(Transfer(kind, token, target))
    # A uop that declares a frame switch with no site that names it
    # (`frame = ...`) continues in the other frame in place of the next
    # word, by the switch exits it declares (ir_contract.body_exits)
    if (uop.properties.switches_frame
            and not any(t.kind in (ENTER_FRAME, RETURN, YIELD) for t in result)):
        declared = declared_exits(uop)
        result.extend(Transfer(kind) for exit, kind in SWITCH_TRANSFERS
                      if exit in declared)
    elif not uop.properties.always_exits:
        result.append(Transfer(NEXT))
    return tuple(result)


def declared_exits(uop: Uop) -> frozenset[str]:
    "uop's declared exits"
    import ir_contract
    return ir_contract.declared_contract(uop).exits


# Guard facts (tiers87, C5).  A guard fails when its condition holds, and
# establishes the negation for the rest of the handler.  The uop's contract
# states the facts (ir_contract.record_guard_facts); a guard that states
# none declares its reason at the site, as the second argument of EXIT_IF or
# DEOPT_IF.

@dataclass(frozen=True)
class Fact:
    kind: str
    subject: str            # a value (v3), a path from one (type(v3)), or
                            # the word, globals or builtins
    arg: str = ""

    def __str__(self) -> str:
        return f"{self.kind}({self.subject}{', ' + self.arg if self.arg else ''})"


# TYPE_VERSION(v, K): type(v)'s version is K; TYPE_VERSION(v) without an
# argument: type(v) has a version, not a specific one.
FACT_KINDS = frozenset((
    "ENCODING", "IS_NONE", "EXACT_TYPE", "NOT_TYPE", "SUBTYPE", "IDENTITY", "TYPE_VERSION",
    "CLASS_VERSION", "KEYS_VERSION", "SHAPE", "INLINE_VALUES_VALID",
    "FUNC_VERSION", "LENGTH", "IN_BOUNDS", "NON_NEGATIVE", "NON_ZERO",
    "ARGCOUNT", "OPERAND_COUNT", "CONVENTION", "SELF", "METHOD", "TYPE_FLAG", "TYPE_SLOT",
    "VECTORCALL", "UNIQUE", "ASCII", "NOT_SUBTYPE", "KEYS_KIND", "ENTRY_KEY",
    "NO_MANAGED_DICT", "TAGGED_INDEX",
))


def encoding(subject: str, word: str) -> Fact:
    """The representation fact that the vocabulary's word (`inline_int`,
    `object`, `nonnull`...) states: ENCODING with the kinds of word it allows
    (ir_contract's lattice), the one kind of every representation fact"""
    import ir_contract
    return ir_contract.VFact(word).record(subject)


def join_encodings(facts) -> tuple[Fact, ...]:
    """facts with one ENCODING fact per subject, at its first: the kinds of
    word that all of the subject's allow"""
    kinds: dict[str, set[str]] = {}
    for f in facts:
        if f.kind == "ENCODING":
            members = set(f.arg.split("|"))
            kinds[f.subject] = kinds[f.subject] & members if f.subject in kinds else members
    result = []
    for f in facts:
        if f.kind != "ENCODING":
            result.append(f)
        elif f.subject in kinds:
            result.append(Fact("ENCODING", f.subject, "|".join(sorted(kinds.pop(f.subject)))))
    return tuple(result)

def _strip(tokens: list) -> list:
    "tokens without a pair of parentheses around them all"
    while len(tokens) >= 2 and tokens[0] == "(" and _close(tokens, 0) == len(tokens) - 1:
        tokens = tokens[1:-1]
    return tokens


def _close(tokens: list, i: int) -> int:
    depth = 0
    for j in range(i, len(tokens)):
        if tokens[j] == "(":
            depth += 1
        elif tokens[j] == ")":
            depth -= 1
            if depth == 0:
                return j
    return -1


def _split(tokens: list, op: str) -> list[list]:
    "tokens split at op outside parentheses"
    parts, depth, start = [], 0, 0
    for i, t in enumerate(tokens):
        if t == "(":
            depth += 1
        elif t == ")":
            depth -= 1
        elif t == op and depth == 0:
            parts.append(tokens[start:i])
            start = i + 1
    parts.append(tokens[start:])
    return parts


def _call(tokens: list, *names: str) -> list[list] | None:
    "The arguments, if tokens are exactly a call of one of names"
    tokens = _strip(tokens)
    if (len(tokens) >= 3 and tokens[0] in names and tokens[1] == "("
            and _close(tokens, 1) == len(tokens) - 1):
        inner = tokens[2:-1]
        return _split(inner, ",") if inner else []
    return None


def _uncast(tokens: list) -> list:
    "tokens without leading casts, `(PyObject *)x`"
    tokens = _strip(tokens)
    while tokens and tokens[0] == "(":
        end = _close(tokens, 0)
        if end <= 0 or end == len(tokens) - 1 or not all(
                re.fullmatch(r"\w+|\*", t) for t in tokens[1:end]):
            break
        tokens = _strip(tokens[end + 1:])
    return tokens


def guard_condition(uop: Uop, site: Token) -> tuple[list, str]:
    """The failure condition of a guard site, as token texts, and the reason
    that the site declares, or ""; for EXIT_IF(true) inside `if (c)`, c"""
    tokens = [t for t in uop.body.tokens() if t.kind != "COMMENT"]
    i = tokens.index(site)
    end = _close([t.text for t in tokens], i + 1)
    args = _split([t.text for t in tokens[i + 2:end]], ",")
    cond = args[0]
    reason = ""
    if len(args) == 2 and len(args[1]) == 1 and args[1][0].startswith('"'):
        reason = args[1][0].strip('"')
    if cond == ["true"]:
        enclosing = enclosing_condition(uop.body.body, site)
        if enclosing is not None:
            cond = enclosing
    return cond, reason


def enclosing_condition(stmts: list[Stmt], site: Token, depth: int = 0) -> list | None:
    """The condition of the if whose body holds site, if that if is the only
    one around it (a nested one would make the failure a conjunction)"""
    for stmt in stmts:
        if isinstance(stmt, BlockStmt):
            found = enclosing_condition(stmt.body, site, depth)
            if found is not None:
                return found
        elif isinstance(stmt, IfStmt):
            if stmt.else_body is None and site in list(stmt.body.tokens()) and depth == 0:
                inner = enclosing_condition([stmt.body], site, depth + 1)
                if inner is None and not _nested_if(stmt.body, site):
                    return [t.text for t in stmt.condition if t.kind != "COMMENT"]
                return None
        elif isinstance(stmt, MacroIfStmt):
            for body in (stmt.body, stmt.else_body or []):
                found = enclosing_condition(body, site, depth)
                if found is not None:
                    return found
    return None


def _nested_if(stmt: Stmt, site: Token) -> bool:
    if isinstance(stmt, BlockStmt):
        return any(_nested_if(s, site) for s in stmt.body)
    if isinstance(stmt, (IfStmt, ForStmt, WhileStmt)):
        return site in list(stmt.tokens())
    return False


# Effects (tiers87, C6): a set of flags per uop; a pure uop has none.
# MAY_RUN_PYTHON implies every other flag.  Compiled code publishes
# instr_ptr before a uop with MAY_RAISE, MAY_RELEASE or MAY_RUN_PYTHON.
# Each uop's contract declares them (ir_contract.Contract.record_effects).
READS_HEAP = "READS_HEAP"
WRITES_HEAP = "WRITES_HEAP"         # changes an existing object or the frame;
                                    # reference counts do not count
ALLOCATES = "ALLOCATES"             # makes a new object or frame
MAY_RAISE = "MAY_RAISE"
MAY_RELEASE = "MAY_RELEASE"         # drops a reference whose finalizer may run
MAY_RUN_PYTHON = "MAY_RUN_PYTHON"
EFFECTS = (READS_HEAP, WRITES_HEAP, ALLOCATES, MAY_RAISE, MAY_RELEASE, MAY_RUN_PYTHON)
# A uop with one of these may run Python: its own or a finalizer
RUNS_PYTHON = frozenset((MAY_RELEASE, MAY_RUN_PYTHON))

# The identifiers that look like a release, which the transfer checks of
# ir_generator.py treat as the end of a value
RELEASE_LIKE = re.compile(r"CLOSE|DECREF|CLEAR|SETREF|RETIRE|Close|Clear|"
                          r"Dealloc|_READER|FINISH_VALUE")
# The macros that read the site's poly rows, which only the specializer
# fills (OperationRecord.reads_poly_table)
POLY_TABLE_READERS: frozenset[str] = frozenset()

ASSIGNS = frozenset(("=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>="))


def _lvalue_is_heap(tokens: list, i: int) -> bool:
    "Whether the assignment at tokens[i] stores through a pointer"
    j = i - 1
    depth = 0
    while j >= 0:
        t = tokens[j]
        if t in (")", "]"):
            depth += 1
        elif t in ("(", "["):
            if depth == 0:
                break
            depth -= 1
        elif depth == 0 and t in (";", "{", "}", ",", "=", "?", ":"):
            break
        j -= 1
    target = tokens[j + 1:i]
    return "->" in target or "[" in target or target[:1] == ["*"]


def effect_sites(uop: Uop) -> list[tuple[Token, str | None]]:
    """The tokens of the body of uop that have an effect, in order, with it:
    the sites of its contract's rows (ir_contract.py)"""
    import ir_contract
    return ir_contract.record_effect_sites(uop)


def uop_record_effects(uop: Uop) -> frozenset[str]:
    "The effects of uop's record: its contract's (ir_contract.py)"
    import ir_contract
    return ir_contract.declared_contract(uop).record_effects()


# The effects that a guard must precede: a failed guard re-runs the handler
# from its start in the generic form, so nothing it observes may have
# changed.  A raise changes nothing; an allocation is a value that the exit
# closes.
CHANGES = frozenset((WRITES_HEAP, MAY_RELEASE, MAY_RUN_PYTHON))


def frame_switch_site(tokens: list[Token]) -> int | None:
    """The position in tokens of the first site that switches the frame, as
    analyzer.switches_frame() finds them, or None"""
    for i, tkn in enumerate(tokens):
        if tkn.kind != "IDENTIFIER":
            continue
        if tkn.text == "DISPATCH_INLINED" or "frame" in IR_MARKER_EFFECTS.get(tkn.text, ()):
            return i
        if (tkn.text == "frame" and i + 1 < len(tokens) and tokens[i + 1].kind == "EQUALS"
                and tokens[i - 1].kind in ("SEMI", "LBRACE", "RBRACE", "EQUALS")):
            return i
    return None


def guard_order(parts: list[Uop], uops: tuple[UopRecord, ...]) -> list[tuple[str, str]]:
    """The guards of a handler that follow a change: pairs of the uop of
    the change and the uop of the guard.  parts are the handler's uops,
    uops their records."""
    import ir_contract
    violations = []
    changed = None
    for part, record in zip(parts, uops):
        order = [t for t in part.body.tokens() if t.kind != "COMMENT"]
        position = {id(t): i for i, t in enumerate(order)}
        changes = [position[id(t)] for t, e in effect_sites(part)
                   if e is None or e in CHANGES]
        contract = ir_contract.declared_contract(part)
        if contract.effects & {"switches_frame", "polls"} or "return" in contract.exits:
            # A frame switch is at its token; a return or a poll, which no
            # token says, is assumed at the start
            switch = (frame_switch_site(order)
                      if not ("return" in contract.exits or "polls" in contract.effects)
                      else None)
            changes.append(0 if switch is None else switch)
        first = min(changes, default=None)
        for guard in record.guards:
            at = position[id(guard.site)] if guard.site is not None else -1
            if changed is not None:
                violations.append((changed, part.name))
            elif first is not None and first < at:
                violations.append((part.name, part.name))
        if first is not None and changed is None:
            changed = part.name
    return list(dict.fromkeys(violations))


def check_record(record, parts: list[Uop]) -> None:
    """Refuse what the record of a handler cannot state: a transfer that
    is not typed, a guard with no fact or reason, a `pure` uop that changes
    the heap or runs Python, and a guard that follows a change.  parts are
    the uops of the instruction.

    The generator checks a record after the emitter's own checks, which
    name a fault more precisely."""
    # The record stops at a uop that ends the handler (ends_handler())
    parts = parts[:len(record.uops)]
    assert [p.name for p in parts] == [u.name for u in record.uops]
    for part, uop in zip(parts, record.uops):
        for transfer in uop.transfers:
            if transfer.kind not in TRANSFER_KINDS:
                raise analysis_error(f"{part.name}: the transfer {transfer.kind} is "
                                     "not a typed transfer", transfer.site or part.body.open)
        for guard in uop.guards:
            if not guard.facts and not guard.reason:
                cond, _ = guard_condition(part, guard.site)
                raise analysis_error(
                    f"{part.name}: the guard `{' '.join(cond)}` establishes no fact "
                    "that ir_records knows; add a fact kind or give the reason, "
                    "EXIT_IF(cond, \"reason\")", guard.site)
        changes = uop.effects & {WRITES_HEAP, MAY_RUN_PYTHON}
        if "pure" in part.annotations and changes:
            raise analysis_error(f"{part.name} is annotated pure but may "
                                 f"{' and '.join(sorted(changes))}", part.body.open)
    violations = guard_order(parts, record.uops)
    if violations:
        raise analysis_error(
            f"{record.handler}: the guards that follow a change, as (uop of the "
            f"change, uop of the guard): {violations}; a guard must precede every "
            "change", record.instruction.where)


# The feedback schema, from the typedefs of Include/internal/pycore_feedback.h

FEEDBACK_HEADER = ROOT / "Include/internal/pycore_feedback.h"
_COMMENT = re.compile(r"/\*.*?\*/|//[^\n]*", re.S)


def parse_feedback_types(text: str) -> dict[str, tuple[SchemaField, ...]]:
    """The fields of each `typedef struct { ... } NAME;` of text.  A union's
    members get one union number; a field of a struct type is expanded as
    `h.arg`."""
    text = _COMMENT.sub("", text)
    types: dict[str, tuple[SchemaField, ...]] = {}
    for match in re.finditer(r"typedef struct\s*(\w*)\s*\{", text):
        depth, i = 1, match.end()
        while depth:
            depth += {"{": 1, "}": -1}.get(text[i], 0)
            i += 1
        name = re.match(r"\s*(\w+)\s*;", text[i:])
        if name is None:
            continue
        body = text[match.end():i - 1]
        fields: list[SchemaField] = []
        unions = 0
        for member in re.finditer(r"union\s*\{([^}]*)\}\s*;|([^;{}]+);", body):
            if member.group(1) is not None:
                for decl in member.group(1).split(";"):
                    field = _declaration(decl, unions)
                    if field is not None:
                        fields.append(field)
                unions += 1
                continue
            field = _declaration(member.group(2), -1)
            if field is None:
                continue
            if field.ctype in types:
                fields.extend(SchemaField(f"{field.name}.{inner.name}", inner.ctype,
                                          inner.union)
                              for inner in types[field.ctype])
            fields.append(field)
        types[name.group(1)] = tuple(fields)
    return types


def _declaration(decl: str, union: int) -> SchemaField | None:
    decl = " ".join(decl.split())
    m = re.fullmatch(r"(.+?)\s*(\**)\s*(\w+)\s*(\[\w+\])?", decl)
    if m is None:
        return None
    ctype = m.group(1).strip() + (" " + m.group(2) if m.group(2) else "")
    return SchemaField(m.group(3), ctype, union)


_feedback_types: dict[str, tuple[SchemaField, ...]] | None = None


def feedback_types() -> dict[str, tuple[SchemaField, ...]]:
    global _feedback_types
    if _feedback_types is None:
        _feedback_types = parse_feedback_types(FEEDBACK_HEADER.read_text())
    return _feedback_types


def feedback_schema(family: str, entry_type: str) -> FeedbackSchema:
    types = feedback_types()
    if entry_type not in types:
        raise ValueError(f"no typedef {entry_type} for the feedback of {family} "
                         f"in {FEEDBACK_HEADER.name}")
    return FeedbackSchema(family, entry_type, types[entry_type])


# The dump

def format_record(record) -> str:
    "One handler's record as stable text"
    lines = [f"{record.handler}  instruction {record.instruction.name}"
             f"  family {record.family or '-'}  form {record.form}"
             f"  {'window' if record.format.window else 'register'}"
             f"  store {record.store_mode or '-'}"]
    for operand in record.operands:
        lines.append(f"  operand {operand}")
    if record.feedback is not None:
        lines.append(f"  feedback {record.feedback.type}")
    for value in record.values:
        source = (f"input {value.external}" if value.external >= 0
                  else f"uop {value.defined_by}")
        array = f"[{value.array}]" if value.array else ""
        lines.append(f"  v{value.id} {value.name}{array} = {source}")
    for index, uop in enumerate(record.uops):
        ins = ", ".join(f"{i.name}=v{i.value} {i.ownership}" for i in uop.inputs)
        outs = ", ".join(f"{o.name}=v{o.value} {o.ownership}" for o in uop.outputs)
        lines.append(f"  uop {index} {uop.name} ({ins} -- {outs})")
        if uop.effects:
            lines.append(f"    effects {' '.join(e for e in EFFECTS if e in uop.effects)}")
        for fb in uop.feedback:
            lines.append(f"    feedback {fb.name}: {fb.field.ctype}")
        for guard in uop.guards:
            where = guard.site.line if guard.site is not None else f"object {guard.input}"
            facts = " ".join(map(str, guard.facts)) or f"no fact: {guard.reason}"
            lines.append(f"    guard {guard.kind} @{where} {facts}")
        for fact in uop.results:
            lines.append(f"    result {fact}")
        transfers = " ".join(t.kind + (f"({t.target})" if t.target else "")
                             for t in uop.transfers)
        lines.append(f"    transfers {transfers}")
        if uop.leaf is not None:
            lines.extend("    " + line for line in uop.leaf.dump(uop.guards))
    for value, operand in record.results:
        lines.append(f"  result v{value} -> {operand}")
    return "\n".join(lines) + "\n"


def main() -> None:
    import argparse
    import sys
    from analyzer import analyze_files
    from generators_common import DEFAULT_INPUT
    import ir_generator
    parser = argparse.ArgumentParser(description="Print the operation records")
    parser.add_argument("-o", "--output", default=None)
    parser.add_argument("input", nargs="*", default=[DEFAULT_INPUT])
    args = parser.parse_args()
    analysis = analyze_files(args.input)
    records = ir_generator.build_records(analysis, ir_generator.compute_formats(analysis))
    out = open(args.output, "w") if args.output else sys.stdout
    with out:
        for name in sorted(records):
            out.write(format_record(records[name]))


if __name__ == "__main__":
    main()
