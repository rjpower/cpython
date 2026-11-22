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
EVALUATOR = "EVALUATOR"         # jumps into the evaluator (TR removes these)

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


@dataclass(frozen=True)
class CallShape:
    "How a call handler's window is a call (call_run_*() in Python/ceval.c)"
    kind: str                   # CALL_VECTOR, CALL_KEYWORDS or CALL_EX
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
    "DISPATCH_GOTO": (EVALUATOR,),
    "DISPATCH_SAME_OPARG": (EVALUATOR,),
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
            if i is not None and not (item.name != "unused" and assigns_to(part, item.name)):
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
                           item.name != "unused" and changes(part, item.name, item.is_array())))
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


def suspends_frame(uop: Uop) -> bool:
    "Whether uop marks its generator's frame suspended: a yield"
    tokens = " ".join(t.text for t in uop.body.tokens())
    return bool(re.search(r"\bgi_frame_state\s*=\s*FRAME_(?:CREATED|SUSPENDED)\b", tokens)
                or re.search(r"\bFT_ATOMIC_STORE_INT8_RELEASE\s*\(\s*\w+\s*->\s*"
                             r"gi_frame_state\s*,\s*FRAME_(?:CREATED|SUSPENDED)\b", tokens))


def uop_transfers(uop: Uop, deopt_target: str) -> tuple[Transfer, ...]:
    """The transfers of uop, in body order: one per site, then the frame
    switch that its properties give, and NEXT if it can fall through"""
    result: list[Transfer] = []
    for token in site_tokens(uop):
        if token.text in GUARD_TOKENS:
            result.append(Transfer(DEOPT, token, deopt_target))
        elif token.kind == "GOTO":
            label = goto_label(uop, token)
            result.append(Transfer(RAISE if label in ERROR_LABELS else EVALUATOR,
                                   token, label))
        elif token.kind == "RETURN":
            result.append(Transfer(EXIT, token))
        else:
            for kind in TRANSFER_TOKENS[token.text]:
                if kind == JUMP and conditional(uop.body.body, token):
                    kind = BRANCH
                target = "redispatch" if kind == EVALUATOR else ""
                result.append(Transfer(kind, token, target))
    # A frame switch that no site names continues in the other frame, in
    # place of the next word
    p = uop.properties
    if p.returns and not any(t.kind == RETURN for t in result):
        result.append(Transfer(RETURN))
    elif p.switches_frame and not any(t.kind in (ENTER_FRAME, RETURN, YIELD) for t in result):
        result.append(Transfer(YIELD if suspends_frame(uop) else ENTER_FRAME))
    elif not p.always_exits:
        result.append(Transfer(NEXT))
    return tuple(result)


# Guard facts (tiers87, C5).  A guard fails when its condition holds, and
# establishes the negation for the rest of the handler.  GUARD_PREDICATES
# derives facts from the condition; a condition that tests only the thread
# or the interpreter is in CONTEXT_PREDICATES with its reason; a guard on an
# operand that no fact describes declares its reason at the site, as the
# second argument of EXIT_IF or DEOPT_IF.  Anything else is an error.

@dataclass(frozen=True)
class Fact:
    kind: str
    subject: str            # a value (v3), a path from one (type(v3)), or
                            # the word, globals or builtins
    arg: str = ""

    def __str__(self) -> str:
        return f"{self.kind}({self.subject}{', ' + self.arg if self.arg else ''})"


FACT_KINDS = frozenset((
    "INLINE_INT", "INLINE_FLOAT", "NUMBER", "OBJECT", "NOT_NULL", "IS_NULL",
    "IS_NONE", "EXACT_TYPE", "NOT_TYPE", "SUBTYPE", "IDENTITY", "TYPE_VERSION",
    "CLASS_VERSION", "KEYS_VERSION", "SHAPE", "INLINE_VALUES_VALID",
    "FUNC_VERSION", "LENGTH", "IN_BOUNDS", "NON_NEGATIVE", "NON_ZERO",
    "ARGCOUNT", "OPERAND_COUNT", "CALL_FLAGS", "SELF", "TYPE_FLAG", "TYPE_SLOT",
    "VECTORCALL", "UNIQUE", "ASCII", "NOT_SUBTYPE", "KEYS_KIND", "ENTRY_KEY",
    "NO_MANAGED_DICT",
))

# Predicates of the thread or interpreter, which hold no fact of an operand
CONTEXT_PREDICATES = {
    "IS_PEP523_HOOKED": "a PEP 523 frame evaluator may replace the call",
    "_PyThreadState_HasStackSpace": "the data stack must hold the callee's frame",
    "py_recursion_remaining": "the Python recursion limit",
    "_Py_ReachedRecursionLimit": "the C recursion limit",
    "LOCK_OBJECT": "a free-threaded build's critical section",
    "_Py_IsOwnedByCurrentThread": "a free-threaded build's object ownership",
    "gil_disabled": "the free-threaded build",
}


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


class Subjects:
    """What the names of a uop body stand for: its inputs are values, and
    a local assigned once from a known derivation of one stands for it"""

    def __init__(self, uop: Uop, inputs: tuple[UopInput, ...]):
        self.names = {i.name: f"v{i.value}" for i in inputs}
        tokens = [t.text for t in uop.body.tokens() if t.kind != "COMMENT"]
        for i, t in enumerate(tokens):
            if t != "=" or i == 0 or tokens[i - 1] in self.names:
                continue
            end = tokens.index(";", i) if ";" in tokens[i:] else len(tokens)
            subject = self.resolve(tokens[i + 1:end])
            if subject is not None:
                self.names.setdefault(tokens[i - 1], subject)

    def resolve(self, tokens: list) -> str | None:
        tokens = _uncast(tokens)
        if len(tokens) == 1:
            return self.names.get(tokens[0])
        for name in ("PyValue_AsKnownObjectBorrow",):
            args = _call(tokens, name)
            if args is not None and len(args) == 1:
                return self.resolve(args[0])
        args = _call(tokens, "Py_TYPE", "PyValue_TYPE")
        if args is not None and len(args) == 1:
            inner = self.resolve(args[0])
            return f"type({inner})" if inner is not None else None
        if _call(tokens, "GLOBALS") == []:
            return "globals"
        if _call(tokens, "BUILTINS") == []:
            return "builtins"
        if len(tokens) >= 3 and tokens[-2] in ("->", "."):
            inner = self.resolve(tokens[:-2])
            return f"{inner}.{tokens[-1]}" if inner is not None else None
        return None

    def of(self, tokens: list) -> str:
        "The subject of an expression, or its text"
        subject = self.resolve(tokens)
        if subject is not None:
            return subject
        text = ""
        for t in _uncast(tokens):
            text += " " + t if text[-1:].isalnum() and t[:1].isalnum() else t
        return text


_TYPE_CHECK = re.compile(r"(Py\w+?)_(CheckExact|Check)")


def predicate_facts(p: list, subjects: Subjects) -> list[Fact] | None:
    "The facts that predicate p, which holds, establishes, or None"
    p = _strip(p)
    simple = {"PyValue_IsInlineInt": "INLINE_INT", "PyValue_IsInlineFloat": "INLINE_FLOAT",
              "PyValue_IsObject": "OBJECT", "PyValue_IsNone": "IS_NONE",
              "_PyObject_IsUniquelyReferenced": "UNIQUE",
              "PyUnicode_IS_COMPACT_ASCII": "ASCII"}
    for name, kind in simple.items():
        args = _call(p, name)
        if args is not None and len(args) == 1:
            return [Fact(kind, subjects.of(args[0]))]
    args = _call(p, "PyValue_FloatOperandValues")
    if args is not None:
        return [Fact("NUMBER", subjects.of(args[0])), Fact("NUMBER", subjects.of(args[1]))]
    args = _call(p, "PyValue_LongCheckExact")
    if args is not None:
        return [Fact("EXACT_TYPE", subjects.of(args[0]), "PyLong_Type")]
    args = _call(p, "PyValue_BoolCheck")
    if args is not None:
        return [Fact("EXACT_TYPE", subjects.of(args[0]), "PyBool_Type")]
    args = _call(p, "Py_IS_TYPE")
    if args is not None:
        return [Fact("EXACT_TYPE", subjects.of(args[0]), " ".join(args[1]).lstrip("& "))]
    if len(p) >= 3 and _TYPE_CHECK.fullmatch(p[0]) and _call(p, p[0]) is not None:
        args = _call(p, p[0])
        m = _TYPE_CHECK.fullmatch(p[0])
        kind = "EXACT_TYPE" if m.group(2) == "CheckExact" else "SUBTYPE"
        return [Fact(kind, subjects.of(args[0]), m.group(1) + "_Type")]
    args = _call(p, "PyType_HasFeature")
    if args is not None:
        return [Fact("TYPE_FLAG", subjects.of(args[0]), " ".join(args[1]))]
    args = _call(p, "_PyType_IteratesByIndex")
    if args is not None:
        return [Fact("TYPE_FLAG", subjects.of(args[0]), "iterates by index")]
    args = _call(p, "FT_ATOMIC_LOAD_UINT8")
    if (args is not None and len(args[0]) > 2 and args[0][-2:] == ["->", "valid"]
            and _call(args[0][:-2], "_PyObject_InlineValues") is not None):
        return [Fact("INLINE_VALUES_VALID",
                     subjects.of(_call(args[0][:-2], "_PyObject_InlineValues")[0]))]
    if p[:1] == ["call_state"] and p[1:] == ["&", "CALL_SELF_PRESENT"]:
        return [Fact("SELF", "word", "present")]
    if len(p) >= 3 and p[-2] == "->" and p[-1].startswith("tp_"):
        return [Fact("TYPE_SLOT", subjects.of(p[:-2]), f"{p[-1]} != NULL")]
    for op, negated in (("!=", "=="), ("==", "!="), ("<=", ">"), ("<", ">="),
                        (">=", "<"), (">", "<=")):
        parts = _split(p, negated)
        if len(parts) == 2:
            return comparison_facts(parts[0], op, parts[1], subjects)
    return None


def _value(tokens: list) -> list:
    "An expression read through an atomic load, without the load"
    tokens = _uncast(tokens)
    args = _call(tokens, "FT_ATOMIC_LOAD_UINT32_RELAXED", "FT_ATOMIC_LOAD_UINT_RELAXED",
                 "FT_ATOMIC_LOAD_SSIZE_RELAXED", "FT_ATOMIC_LOAD_INT8_RELAXED",
                 "FT_ATOMIC_LOAD_PTR_RELAXED")
    return _uncast(args[0]) if args is not None and len(args) == 1 else tokens


def comparison_facts(lhs: list, op: str, rhs: list,
                     subjects: Subjects) -> list[Fact] | None:
    """The facts of `lhs op' rhs` holding, where op is the failure's
    comparison: `lhs != rhs` failing establishes lhs == rhs"""
    left, right = _value(lhs), _value(rhs)
    text = " ".join(_strip(rhs))
    if op == "!=":
        if left[-2:] == ["->", "tp_version_tag"]:
            owner = subjects.of(left[:-2])
            if owner.startswith("type(") and owner.endswith(")"):
                return [Fact("TYPE_VERSION", owner[5:-1], text)]
            return [Fact("CLASS_VERSION", owner, text)]
        if left[-2:] == ["->", "dk_version"]:
            return [Fact("KEYS_VERSION", subjects.of(left[:-2]), text)]
        if left[-2:] == ["->", "func_version"]:
            return [Fact("FUNC_VERSION", subjects.of(left[:-2]), text)]
        if left[-2:] == ["->", "co_argcount"]:
            return [Fact("ARGCOUNT", subjects.of(left[:-2]), text)]
        if left[-2:] in (["->", "vectorcall"], ["->", "tp_vectorcall"]):
            return [Fact("VECTORCALL", subjects.of(left[:-2]), text)]
        if len(left) > 2 and left[-2] == "->" and re.match(r"(tp|mp|sq|nb)_", left[-1]):
            # A slot of a type, through its slot tables: Py_TYPE(o)->tp_as_mapping->mp_subscript
            owner = left[:-2]
            while len(owner) > 2 and owner[-2] == "->" and owner[-1].startswith("tp_as_"):
                owner = owner[:-2]
            return [Fact("TYPE_SLOT", subjects.of(owner), f"{left[-1]} == {text}")]
        args = _call(left, "_PyObject_Shape")
        if args is not None:
            return [Fact("SHAPE", subjects.of(args[0]), text)]
        args = _call(left, "Py_TYPE", "PyValue_TYPE")
        if args is not None:
            return [Fact("EXACT_TYPE", subjects.of(args[0]), text.lstrip("& "))]
        if left[-2:] == ["->", "dk_kind"]:
            return [Fact("KEYS_KIND", subjects.of(left[:-2]), text)]
        if left[-2:] == ["->", "me_key"]:
            return [Fact("ENTRY_KEY", subjects.of(left[:-2]), text)]
        args = _call(left, "PyTuple_GET_SIZE", "PyList_GET_SIZE")
        if args is not None:
            return [Fact("LENGTH", subjects.of(args[0]), text)]
        if left[:1] == ["total_args"] and len(left) == 1:
            return [Fact("OPERAND_COUNT", "word", text)]
        if any(t in ("ml_flags", "PyCFunction_GET_FLAGS", "flags") for t in left):
            return [Fact("CALL_FLAGS", "callable", f"{' '.join(left)} == {text}")]
        # (x.bits & ~1) != (uintptr_t)K: x is the object K
        inner = _strip(left)
        if len(inner) >= 4 and inner[1:3] == [".", "bits"] and inner[3] == "&":
            return [Fact("IDENTITY", subjects.of(inner[:1]),
                         " ".join(_uncast(rhs)).lstrip("& "))]
        if len(left) == 1 and right[:1] == ["&"] or _uncast(rhs)[:1] == ["&"]:
            return [Fact("IDENTITY", subjects.of(left), " ".join(_uncast(rhs)).lstrip("& "))]
        if right == ["NULL"]:
            return [Fact("IS_NULL", subjects.of(left))]
    if op == "==":
        if right == ["NULL"]:
            return [Fact("NOT_NULL", subjects.of(left))]
        if right in (["0"], ["0.0"]):
            return [Fact("NON_ZERO", subjects.of(left))]
        args = _call(left, "Py_TYPE")
        if args is not None:
            return [Fact("NOT_TYPE", subjects.of(args[0]), text.lstrip("& "))]
        if left[-2:] == ["->", "dk_kind"]:
            return [Fact("KEYS_KIND", subjects.of(left[:-2]), f"not {text}")]
    if op == "<" and right == ["0"]:
        return [Fact("NON_NEGATIVE", subjects.of(left))]
    if op == "<=":
        args = _call(left, "PyUnicode_GET_LENGTH", "Py_ARRAY_LENGTH")
        if args is not None:
            return [Fact("IN_BOUNDS", subjects.of(rhs), subjects.of(args[0]))]
    if op == ">=":
        # left indexes what the right measures: entries of keys, or a length
        right_value = _value(rhs)
        if right_value[-2:] == ["->", "dk_nentries"]:
            right_value = right_value[:-2]
        return [Fact("IN_BOUNDS", subjects.of(left), subjects.of(right_value))]
    return None


def failure_facts(cond: list, subjects: Subjects) -> list[Fact] | None:
    """The facts that a guard establishes when its condition cond is false,
    or None if the table does not describe it"""
    cond = _strip(cond)
    facts: list[Fact] = []
    for part in _split(cond, "||"):
        part = _strip(part)
        if part[:1] == ["!"] and len(_split(part, "&&")) == 1:
            found = predicate_facts(part[1:], subjects)
        else:
            found = failure_term_facts(part, subjects)
        if found is None:
            return None
        facts += found
    return facts


def failure_term_facts(term: list, subjects: Subjects) -> list[Fact] | None:
    "The facts of a failure term that is not a negation being false"
    conj = _split(term, "&&")
    if len(conj) > 1:
        # x is none of several types: that it is one of them
        found = [failure_term_facts(_strip(c), subjects) if _strip(c)[:1] != ["!"]
                 else predicate_facts(_strip(c)[1:], subjects) for c in conj]
        for kind in ("EXACT_TYPE", "CALL_FLAGS"):
            if (all(f is not None and len(f) == 1 and f[0].kind == kind for f in found)
                    and len({f[0].subject for f in found}) == 1):
                return [Fact(kind, found[0][0].subject,
                             " | ".join(f[0].arg for f in found))]
        return None
    # A positive type test failing: x is not of the type
    if term and _TYPE_CHECK.fullmatch(term[0]) and _call(term, term[0]) is not None:
        m = _TYPE_CHECK.fullmatch(term[0])
        return [Fact("NOT_TYPE" if m.group(2) == "CheckExact" else "NOT_SUBTYPE",
                     subjects.of(_call(term, term[0])[0]), m.group(1) + "_Type")]
    args = _call(term, "PyValue_IsNull")
    if args is not None:
        return [Fact("NOT_NULL", subjects.of(args[0]))]
    args = _call(term, "_PyObject_GetManagedDict")
    if args is not None:
        return [Fact("NO_MANAGED_DICT", subjects.of(args[0]))]
    if term == ["call_state", "&", "CALL_SELF_PRESENT"]:
        return [Fact("SELF", "word", "absent")]
    for op in ("!=", "==", "<=", ">=", "<", ">"):
        parts = _split(term, op)
        if len(parts) == 2:
            return comparison_facts(parts[0], op, parts[1], subjects)
    return None


def context_reason(cond: list) -> str | None:
    "The reason of a condition on the thread or interpreter only"
    for token in cond:
        if token in CONTEXT_PREDICATES:
            return CONTEXT_PREDICATES[token]
    return None


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


# The guard markers and the facts that each establishes of its arguments
MARKER_FACTS = {
    "IR_GUARD_INLINE_INT": lambda a, s: [Fact("INLINE_INT", s.of(a[0]))],
    "IR_GUARD_ELEMENT_TYPE": lambda a, s: [Fact("EXACT_TYPE", s.of(a[0]),
                                                {"LIST": "PyList_Type", "TUPLE": "PyTuple_Type"}
                                                .get(" ".join(a[1]), " ".join(a[1])))],
    "IR_GUARD_ELEMENT_INDEX": lambda a, s: [Fact("IN_BOUNDS", s.of(a[1]), s.of(a[0]))],
    "IR_GUARD_SHAPE_READ": lambda a, s: [Fact("TYPE_VERSION", s.of(a[0]), " ".join(a[1])),
                                         Fact("SHAPE", s.of(a[0]), " ".join(a[2]))],
}


def guard_facts(uop: Uop, site: Token, subjects: Subjects) -> tuple[tuple[Fact, ...], str]:
    "The facts of a guard site and its reason; neither if nothing describes it"
    if site.text in MARKER_FACTS:
        tokens = [t.text for t in uop.body.tokens() if t.kind != "COMMENT"]
        i = [t for t in uop.body.tokens() if t.kind != "COMMENT"].index(site)
        args = _call(tokens[i:_close(tokens, i + 1) + 1], site.text)
        return tuple(MARKER_FACTS[site.text](args, subjects)), ""
    cond, reason = guard_condition(uop, site)
    if reason:
        return (), reason
    context = context_reason(cond)
    if context is not None:
        return (), context
    facts = failure_facts(cond, subjects)
    if facts is None:
        return (), ""       # check_record() refuses it
    assert all(f.kind in FACT_KINDS for f in facts)
    return tuple(facts), ""


# The facts of a uop's results: every assignment to an output makes a
# value of one kind
BOOL_RESULT = re.compile(r"(.* \? )?PyValue_(True|False)( : PyValue_(True|False))?")
INLINE_INT_RESULTS = frozenset(("PyValue_FromInlineInt", "_PyValue_FromIntScaled"))
INT_RESULTS = INLINE_INT_RESULTS | {
    "PyValue_FromInt64", "_PyValue_BoxInt64", "_PyValue_MultiplyInt64",
    "_PyValue_LshiftInt64"}
FLOAT_RESULTS = frozenset(("PyValue_FromDouble", "PyValue_FromDoubleNotNaN"))


def result_facts(uop: Uop, outputs: tuple[UopOutput, ...]) -> tuple[Fact, ...]:
    "The facts that hold of the new outputs of uop"
    tokens = [t.text for t in uop.body.tokens() if t.kind != "COMMENT"]
    facts = []
    for out in outputs:
        if out.ownership not in (OWNED, INLINE):
            continue
        sources = []
        for i, t in enumerate(tokens):
            if t == "=" and i and tokens[i - 1] == out.name:
                end = tokens.index(";", i)
                sources.append(tokens[i + 1:end])
        if not sources:
            continue
        subject = f"v{out.value}"
        callees = {src[0] if len(src) > 1 and src[1] == "(" else "" for src in sources}
        if all(BOOL_RESULT.fullmatch(" ".join(src)) for src in sources):
            facts.append(Fact("EXACT_TYPE", subject, "PyBool_Type"))
        elif callees <= INLINE_INT_RESULTS:
            facts += [Fact("INLINE_INT", subject), Fact("EXACT_TYPE", subject, "PyLong_Type")]
        elif callees <= INT_RESULTS:
            facts.append(Fact("EXACT_TYPE", subject, "PyLong_Type"))
        elif callees <= FLOAT_RESULTS:
            facts.append(Fact("EXACT_TYPE", subject, "PyFloat_Type"))
    return tuple(facts)


# Effects (tiers87, C6): a set of flags per uop; a pure uop has none.
# MAY_RUN_PYTHON implies every other flag.  Compiled code publishes
# instr_ptr before a uop with MAY_RAISE, MAY_RELEASE or MAY_RUN_PYTHON.
# The flags are inferred from the analysis and the call tables below; no
# uop declares any.
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

# The names of calls that may allocate, after PURE_CALLS
CONSTRUCTOR = re.compile(r"New(?!Ref)|Alloc|^PyValue_From|_PyFrame_Push|_Py_Make")
# Allocating calls that CONSTRUCTOR does not name, with what else they do
ALLOCATORS = {
    **dict.fromkeys(("_PyValue_BoxInt64", "_PyValue_MultiplyInt64",
                     "_PyValue_LshiftInt64", "_PyValue_BoxArray", "call_push_exact",
                     "_PyTuple_FromValueStealOnSuccess",
                     "_PyList_FromValueStealOnSuccess", "PyFloat_FromDouble",
                     "PyLong_FromLong", "PyLong_FromSsize_t"),
                    frozenset((ALLOCATES,))),
    **dict.fromkeys(("PyUnicode_Concat", "_PyUnicode_JoinArray"),
                    frozenset((ALLOCATES, READS_HEAP))),
    "_PyList_AppendTakeRef": frozenset((ALLOCATES, READS_HEAP, WRITES_HEAP)),
}

# The identifiers that look like a release (RELEASE_LIKE), each known to
# release a reference whose __del__ may run Python, or not to.  Any other
# such identifier is an error, so that a new release helper cannot pass as
# one that runs nothing.
RELEASE_LIKE = re.compile(r"CLOSE|DECREF|CLEAR|SETREF|RETIRE|Close|Clear|"
                          r"Dealloc|_READER|FINISH_VALUE")
RELEASES = {
    "IR_DEFER_CLOSE",
    "IR_CLOSE_INPUTS",
    "PyValue_CLOSE", "PyValue_XCLOSE", "PyValue_CLEAR", "_PyValue_CloseStack",
    "_PyValue_CLOSE_POINTER", "_PyValue_XCLOSE_POINTER", "DECREF_INPUTS",
    "Py_DECREF", "Py_XDECREF", "Py_CLEAR", "Py_SETREF", "Py_XSETREF",
    # The value and attribute transactions release the inputs they retire
    "IR_RETIRE_PUBLISHED", "IR_RETIRE_PUBLISHED_RESULT",
    "IR_RETIRE_PUBLISHED_PAIR",
    "_PyCell_SetValue",                     # stores, then releases the old value
    "IR_FINISH_VALUE_OPERATION", "IR_ABORT_VALUE_READER",
    "IR_TRY_DICT_FIRST_PROBE",
    "IR_COMPLETE_VALUE_READER", "IR_COMPLETE_ATTR_READER",
    "IR_BINARY_READER",
    "_PyFrame_ClearTemporaries", "_PyEval_FrameClearAndPop",
    "_PyEval_FrameClearAndPopWithResult", "_PyEval_FrameClearAndPopGeneralWithResult",
    "_PyEval_RetireThreadFrameWithResult",
    "_PyErr_Clear",
}
NOT_RELEASES = {
    # A release whose destructor, that of an exact type, runs no Python
    "PyValue_CLOSE_SPECIALIZED", "_Py_DECREF_SPECIALIZED",
    "_PyLong_ExactDealloc", "_PyFloat_ExactDealloc",
    "_PyFloat_OrLongExactDealloc", "_PyUnicode_ExactDealloc",
    "_PyFrame_AssertTemporariesClear",      # a debug check
    "_PyGC_CLEAR_ARRAY",                    # expands to nothing
    "LOAD_FAST_AND_CLEAR",                  # an opcode
    # Semantic reason constants passed to the actual release boundary.
    "_PyIR_RETIRE_NONE", "_PyIR_RETIRE_NORMAL_RETURN",
    "_PyIR_RETIRE_ERROR_UNWIND", "_PyIR_RETIRE_HEAP_CLEAR",
    "_PyIR_RETIRE_STAGED_FAILURE",
}



# Non-escaping calls that only compute on their arguments' bits
PURE_CALLS = re.compile(
    r"__builtin_\w+_overflow|Py_ARITHMETIC_RIGHT_SHIFT|assert|Py_UNREACHABLE|"
    r"_?PyValue_(Is\w*|Inline\w*|BothInlineInt|IntScaled|FromIntScaled|FromInlineInt|"
    r"TagIndex|UntagIndex|IncrementTaggedIndexNoOverflow|FloatOperand\w*|NumberAsDouble|"
    r"Borrow|Wrap|Unwrap|True|False|None|AsKnownObjectBorrow|FromObjectBorrow|"
    r"FromObjectSteal|FromNonNumberSteal|AsKnownObjectSteal|AsPyObjectNew|DUP|XDUP|"
    r"DupImmortal|PointerBorrow|CheckHome|RecheckHome|RecheckWindow|CheckTemp|"
    r"FromDoubleNotNaN)|Py_INCREF|Py_NewRef|_Py_NewRef|_Py_ID|_Py_STR|_Py_SINGLETON|"
    r"Py_ARRAY_LENGTH|_PyIR_AdoptTemp|GETLOCAL|Py_FatalError|PyValue_RefcountOnObject|"
    r"_PyFrame_AssertTemporariesClear|_PyManagedDictPointer_IsValues|doesnt_escape|"
    r"(advance|restart)_backoff_counter|_Py_GatherStats_GetIter|native_vector_capture|"
    r"_Py_CHECK_EMSCRIPTEN_SIGNALS_PERIODICALLY")

# Non-escaping calls that read the heap and change nothing
READER_CALLS = re.compile(
    r"Py_TYPE|PyValue_TYPE|Py_SIZE|Py_REFCNT|\w+_GET_SIZE|\w+_GET_ITEM|\w+_GET_VALUE|"
    r"\w+_GET_LENGTH|_Py\w*_ITEMS|\w+Check(Exact)?|Py_IS_TYPE|Py_Is\w+|_Py_Is\w+|"
    r"FT_ATOMIC_LOAD\w*|\w+_GET_\w+|_PyFrame_(Get\w+|IsIncomplete|Slots|InstrOffset)|"
    r"_PyLong_\w+|PyLong_AsLong|PyFloat_AS_DOUBLE|PyUnicode_READ_CHAR|PyUnicode_1BYTE_DATA|"
    r"PyUnicode_IS_COMPACT_ASCII|_PyObject_(InlineValues|Shape|ManagedDictPointer|"
    r"GetManagedDict|IsUniquelyReferenced|GC_IS_\w+|GC_MAY_BE_TRACKED)|_PyCode_\w+|"
    r"_?PyType_(HasFeature|IteratesByIndex|VALUE_SLOT|FastSubclass)|PyType_FastSubclass|"
    r"_PyErr_Occurred|_PyErr_ExceptionMatches|_PyInterpreterState_GET|_PyThreadState_HasStackSpace|"
    r"PyExceptionInstance_Class|PyException_Get\w+|_PyCell_GetValue|_PyValueGuarded_Load|"
    r"_PyFb_\w+|_PyDataStack_Slots|_PyGen_GetGeneratorFromFrame|_PyClassMethod_GetFunc|"
    r"backoff_counter_triggers|_PyEval_GeneratorReturnTarget|_PyUnicode_Equal|"
    r"PyCFunction_GET_\w+|PyFunction_GET_\w+|_Py_ReachedRecursionLimit|PyUnicode_GET_LENGTH|"
    r"_PyShape_\w+|_PyDictValues_ShapeAfterAdd|_PyIR_\w+|IS_PEP523_HOOKED|"
    r"GETITEM|FRAME_CO_\w+|GLOBALS|BUILTINS|LOCALS|DK_\w+|_Py_atomic_load_\w+|"
    r"_PyForIter_StopIsExhaustion|coro_awaitable_can_transfer|PyCell_GetRef|"
    r"_Py_TryIncrefCompare\w*")

# Non-escaping calls that change existing objects or the thread state
WRITER_CALLS = re.compile(
    r"FT_ATOMIC_STORE\w*|_PyDictValues_(AddToInsertionOrder|SetShape)|_PyFunction_SetVersion|"
    r"_Py_LeaveRecursiveCall\w*|gen_prologue|gen_try_set_executing|PyCell_SwapTakeRef|"
    r"PyList_SET_ITEM|_PyFrame_(Initialize|MoveArgs|SetReturnValue)|_PyObject_GC_TRACK|"
    r"PyObject_GC_UnTrack|_Py_atomic_compare_exchange_\w+|_Py_(un)?set_eval_breaker_bit|"
    r"_PyThreadState_PopCValueSteal|_PyStolenTuple_Free")

# The macros of the DSL that the analysis does not count as escaping calls;
# frame slots and the call window are operands, not heap
MACRO_EFFECTS = {
    **dict.fromkeys(("IR_ACTIVATION", "IR_BEGIN_CALL_EFFECTS", "IR_CALL_ARG",
                     "IR_CALL_FIRST", "IR_D", "IR_FUSED_DISTANCE", "IR_FUSED_IF_FALSE",
                     "IR_GUARD_INLINE_INT", "IR_HANDLER",
                     "IR_IS_LOCAL", "IR_METADATA_PASSTHROUGH", "IR_REPORT_EDGE",
                     "IR_SKIP_RESUME", "IR_SLOT", "IR_TAKE_INPUT", "IR_WRITE_OUTPUT"),
                    frozenset()),
    **dict.fromkeys(("IR_DUP_CANONICAL_ELEMENT", "IR_DUP_SHAPE_READ",
                     "IR_GUARD_ELEMENT_INDEX", "IR_GUARD_ELEMENT_TYPE",
                     "IR_GUARD_SHAPE_READ", "IR_METHOD_POLY_LOOKUP",
                     "IR_UNPACK_ITEMS"),
                    frozenset((READS_HEAP,))),
    "IR_BUILD_TUPLE": frozenset((ALLOCATES,)),
    "IR_DEFER_CLOSE": frozenset((MAY_RELEASE,)),
    # Marks RETURN_GENERATOR, whose handler never runs: the generator is
    # made when the frame is entered
    "IR_GENERATOR_MARKER": frozenset(),
    # Fires the monitoring event, whose callback is Python
    "INSTRUMENTED_JUMP": frozenset((MAY_RUN_PYTHON,)),
}

# The macros that read the site's poly rows, which only the specializer
# fills (OperationRecord.reads_poly_table)
POLY_TABLE_READERS = frozenset(("IR_METHOD_POLY_LOOKUP",))

# A `*` after one of these dereferences a pointer
DEREF_AFTER = frozenset(("(", "=", ",", ";", "{", "return"))
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
    None for a call that the tables do not know"""
    p = uop.properties
    tokens = [t for t in uop.body.tokens() if t.kind != "COMMENT"]
    texts = [t.text for t in tokens]
    escaping = {e.call.text for e in p.escaping_calls.values()}
    sites = []
    i = 0
    while i < len(texts):
        tkn, text = tokens[i], texts[i]
        call = tkn.kind == "IDENTIFIER" and i + 1 < len(texts) and texts[i + 1] == "("
        if call and text == "assert":
            i = _close(texts, i + 1) + 1
            continue
        i += 1
        if text in ASSIGNS and _lvalue_is_heap(texts, i - 1):
            sites.append((tkn, WRITES_HEAP))
        elif text in ("++", "--") and "->" in texts[max(i - 4, 0):i + 3]:
            sites.append((tkn, WRITES_HEAP))
        elif text == "->" or text == "*" and i > 1 and texts[i - 2] in DEREF_AFTER:
            sites.append((tkn, READS_HEAP))
        if not call:
            continue
        if text in TRANSFER_TOKENS and TRANSFER_TOKENS[text] == RAISE:
            sites.append((tkn, MAY_RAISE))
        if text in IR_MARKER_EFFECTS and "escapes" in IR_MARKER_EFFECTS[text]:
            sites.append((tkn, MAY_RUN_PYTHON))
        elif text in RELEASES:
            # The last reference's finalizer may run Python, whether or not
            # the analysis counts the release as escaping
            sites.append((tkn, MAY_RELEASE))
            if text == "_PyCell_SetValue":
                sites.append((tkn, WRITES_HEAP))
        elif text in NOT_RELEASES:
            # A release whose deallocator runs no Python frees memory
            sites.append((tkn, WRITES_HEAP))
        elif p.escapes and text in escaping:
            sites.append((tkn, MAY_RUN_PYTHON))
        # A call that escapes only on the way to an error exit is a raise
        elif PURE_CALLS.fullmatch(text) or text in escaping:
            continue
        elif text in ALLOCATORS:
            sites += [(tkn, e) for e in ALLOCATORS[text]]
        elif CONSTRUCTOR.search(text):
            sites.append((tkn, ALLOCATES))
        elif text in MACRO_EFFECTS:
            sites += [(tkn, e) for e in MACRO_EFFECTS[text]]
        elif WRITER_CALLS.fullmatch(text):
            sites += [(tkn, READS_HEAP), (tkn, WRITES_HEAP)]
        elif READER_CALLS.fullmatch(text):
            sites.append((tkn, READS_HEAP))
        elif not text.isupper() or RELEASE_LIKE.search(text):
            # Not a statement macro (STAT_INC, TAKE, ...): it may do anything
            sites.append((tkn, None))
    return sites


def uop_effects(uop: Uop) -> frozenset[str]:
    "Effects of uop that no one token has: it may switch frames or poll"
    p = uop.properties
    effects = set()
    if not p.infallible:
        effects.add(MAY_RAISE)
    if p.switches_frame or p.returns or p.eval_breaker:
        effects.add(MAY_RUN_PYTHON)
    return frozenset(effects)


def infer_effects(uop: Uop) -> frozenset[str]:
    "The effects that uop may have"
    effects = set(uop_effects(uop)) | {
        e if e is not None else MAY_RUN_PYTHON for _, e in effect_sites(uop)}
    if MAY_RUN_PYTHON in effects:
        effects.update(EFFECTS)
    return frozenset(effects)


# The effects that a guard must precede: a failed guard re-runs the handler
# from its start in the generic form, so nothing it observes may have
# changed.  A raise changes nothing; an allocation is a value that the exit
# closes.
CHANGES = frozenset((WRITES_HEAP, MAY_RELEASE, MAY_RUN_PYTHON))


# The handlers whose guards follow a change, as (handler, uop of the change,
# uop of the guard).  Contract87 TR reorders them; a new one is an error.
GUARD_ORDER_EXCEPTIONS = frozenset((
    # The method expansion writes the call window before the callee checks
    *(("CALL_BOUND_METHOD_EXACT_ARGS", "_INIT_CALL_BOUND_METHOD_EXACT_ARGS", guard)
      for guard in ("_CHECK_FUNCTION_VERSION", "_CHECK_FUNCTION_EXACT_ARGS",
                    "_CHECK_STACK_SPACE", "_CHECK_RECURSION_REMAINING")),
    ("CALL_BOUND_METHOD_GENERAL", "_EXPAND_METHOD", "_CHECK_RECURSION_REMAINING"),
    # The argument tuple is made before the callable is checked
    ("CALL_EX_NON_PY_GENERAL", "_MAKE_CALLARGS_A_TUPLE", "_CHECK_IS_NOT_PY_CALLABLE_EX"),
    ("CALL_EX_PY", "_MAKE_CALLARGS_A_TUPLE", "_CHECK_IS_PY_CALLABLE_EX"),
    # The native call declines a callee before it has an effect
    *((name, uop, uop) for name, uop in (
        ("CALL_BUILTIN_FAST", "_CALL_BUILTIN_FAST"),
        ("CALL_BUILTIN_FAST_WITH_KEYWORDS", "_CALL_BUILTIN_FAST_WITH_KEYWORDS"),
        ("CALL_METHOD_DESCRIPTOR_FAST", "_CALL_METHOD_DESCRIPTOR_FAST"),
        ("CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS",
         "_CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS"))),
    # gen_try_set_executing() claims the generator, then the frame checks
    ("FOR_ITER_GEN", "_FOR_ITER_GEN_FRAME", "_FOR_ITER_GEN_FRAME"),
    *((name, "_SEND_GEN_FRAME", "_SEND_GEN_FRAME") for name in (
        "SEND_GEN", "SEND_GEN_XFER_DISCARD", "SEND_GEN_XFER_HOME", "SEND_GEN_XFER_POP_TOP",
        "SEND_GEN_XFER_RETURN", "SEND_GEN_XFER_STORE_FAST", "SEND_GEN_XFER_STORE_LOCAL")),
))


def guard_order(parts: list[Uop], uops: tuple[UopRecord, ...]) -> list[tuple[str, str]]:
    """The guards of a handler that follow a change: pairs of the uop of
    the change and the uop of the guard.  parts are the handler's uops,
    uops their records."""
    violations = []
    changed = None
    for part, record in zip(parts, uops):
        order = [t for t in part.body.tokens() if t.kind != "COMMENT"]
        position = {id(t): i for i, t in enumerate(order)}
        changes = [position[id(t)] for t, e in effect_sites(part)
                   if e is None or e in CHANGES]
        if uop_effects(part) & CHANGES:
            changes.append(0)   # no token says where; assume the start
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
    """Refuse what the record of a handler cannot state: a guard with no
    fact or reason, a `pure` uop that changes the heap or runs Python, and a
    guard that follows a change but is not in GUARD_ORDER_EXCEPTIONS.
    parts are the uops of the instruction.

    The generator checks a record after the emitter's own checks, which
    name a fault more precisely."""
    # The record stops at a uop that ends the handler (ends_handler())
    parts = parts[:len(record.uops)]
    assert [p.name for p in parts] == [u.name for u in record.uops]
    for part, uop in zip(parts, record.uops):
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
    name = record.handler
    violations = {(name, *v) for v in guard_order(parts, record.uops)}
    expected = {v for v in GUARD_ORDER_EXCEPTIONS if v[0] == name}
    if violations != expected:
        raise analysis_error(
            f"{name}: the guards that follow a change differ from "
            f"ir_records.GUARD_ORDER_EXCEPTIONS: new {sorted(violations - expected)}, "
            f"gone {sorted(expected - violations)}", record.instruction.where)


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
