"""Checked source effects for exceptional ingress, independent of normal IO.

This is a small typed subset of the instruction DSL, not a C verifier. Unknown
source has no summary. The emitter must match the returned source identities
to actual flush/terminal receipts before exporting a complete route contract.
"""

from dataclasses import dataclass, field, replace
from enum import Enum, auto

from analyzer import Instruction, Skip, Uop
from generators_common import TokenIterator, instruction_macro_args
from lexer import Token
from parser import BlockStmt, IfStmt, MacroIfStmt, SimpleStmt


class Kind(Enum):
    VALUE = auto()
    OBJECT = auto()
    SCALAR = auto()
    CODE = auto()
    TUPLE = auto()
    TSTATE = auto()
    INTERP = auto()
    FRAME = auto()
    INSTR = auto()
    ATOMIC_ADDRESS = auto()
    ATOMIC_FIELD = auto()
    LINE_INFO = auto()
    FEEDBACK = auto()
    OBSERVED_OPCODE = auto()
    SITE_INDEX = auto()
    VOID = auto()
    NULL = auto()


class HomeEffect(Enum):
    UNKNOWN = auto()
    NO_ARENA_WRITE = auto()
    PRESERVES_SCRATCH_ABSENCE = auto()
    PRESERVES_CANONICAL_OR_NULL = auto()


@dataclass(frozen=True)
class HelperEffect:
    inputs: tuple[Kind, ...]
    result: Kind
    homes: HomeEffect
    may_fail: bool = False
    callback: bool = False
    result_owned: bool | None = None
    steals: tuple[int, ...] = ()
    result_canonical: bool = False
    result_nonnull: bool = False


def native_helper(inputs, result, may_fail=False, callback=False, *,
                  canonical_clear=False, result_owned=None, steals=(),
                  result_canonical=False, result_nonnull=False):
    homes = (HomeEffect.PRESERVES_CANONICAL_OR_NULL if callback and canonical_clear
             else HomeEffect.PRESERVES_SCRATCH_ABSENCE if callback
             else HomeEffect.NO_ARENA_WRITE)
    return HelperEffect(inputs, result, homes, may_fail, callback, result_owned, steals,
                        result_canonical, result_nonnull)


# These interfaces receive native values, never an arena-home address. A
# callback may change named bindings or vacate scratch through a debugger
# redirect. It cannot publish a new scratch owner. The stronger canonical_clear
# contract additionally preserves previously initialized scratch: f_locals
# writes only bindings, frame_lineno_set_impl clears scratch with canonical NULL
# and restores saved values only to bindings, and an executing frame cannot be
# cleared or resumed. Nested activations reserve their own frame storage. This
# permits canonical-or-NULL widening, never initialization from absence or an
# old-PC shape after an accepted redirect. Generated operand writes remain a
# separate emitter obligation. A returned failure is not a goto.
HELPERS = {
    "PyValue_DUP": native_helper((Kind.VALUE,), Kind.VALUE),
    "PyValue_XDUP": native_helper((Kind.VALUE,), Kind.VALUE),
    "TAKE": native_helper((Kind.VALUE,), Kind.VALUE),
    "PyValue_IsNull": native_helper((Kind.VALUE,), Kind.SCALAR),
    "PyValue_IsNone": native_helper((Kind.VALUE,), Kind.SCALAR),
    "PyValue_IsInlineInt": native_helper((Kind.VALUE,), Kind.SCALAR),
    "PyValue_IsInlineFloat": native_helper((Kind.VALUE,), Kind.SCALAR),
    "_PyValue_IsTrue": native_helper((Kind.VALUE,), Kind.SCALAR, True, True,
                                     canonical_clear=True),
    "PyValue_FromInlineInt": native_helper((Kind.SCALAR,), Kind.VALUE,
        result_owned=True, result_canonical=True, result_nonnull=True),
    "PyValue_AsKnownObjectBorrow": native_helper((Kind.VALUE,), Kind.OBJECT,
                                                 result_owned=False),
    "PyValue_FromObjectSteal": native_helper((Kind.OBJECT,), Kind.VALUE, steals=(0,),
        result_owned=True, result_canonical=True, result_nonnull=True),
    **{name: native_helper((Kind.VALUE,), Kind.VALUE, True, True,
            canonical_clear=True, result_owned=True)
       for name in ("_PyValue_Negative", "_PyValue_Positive", "_PyValue_Invert")},
    "PyValue_CLOSE": native_helper((Kind.VALUE,), Kind.VOID, callback=True, canonical_clear=True),
    "PyValue_XCLOSE": native_helper((Kind.VALUE,), Kind.VOID, callback=True, canonical_clear=True),
    "DEAD": native_helper((Kind.VALUE,), Kind.VOID),
    # Their emitted writes are confined by IRStack, independently of this
    # source summary. They grant no permission to pass a home to a C helper.
    "DECREF_INPUTS": native_helper((), Kind.VOID, callback=True, canonical_clear=True),
    "INPUTS_DEAD": native_helper((), Kind.VOID),
    "GETLOCAL": native_helper((Kind.SCALAR,), Kind.VALUE),
    "PyTuple_GET_VALUE": native_helper((Kind.TUPLE, Kind.SCALAR), Kind.VALUE),
    "_PyTuple_GET_VALUE": native_helper((Kind.TUPLE, Kind.SCALAR), Kind.VALUE),
    "_PyEval_GetANext": native_helper((Kind.OBJECT,), Kind.OBJECT, True, True,
                                      canonical_clear=True, result_owned=True),
    "check_periodics": native_helper((Kind.TSTATE,), Kind.SCALAR, True, True),
    "_Py_Instrument": native_helper((Kind.CODE, Kind.INTERP), Kind.SCALAR, True),
    "_Py_atomic_load_uintptr_relaxed": native_helper((Kind.ATOMIC_ADDRESS,), Kind.SCALAR),
    "FT_ATOMIC_LOAD_UINTPTR_ACQUIRE": native_helper((Kind.ATOMIC_FIELD,), Kind.SCALAR),
    "_Py_call_instrumentation": native_helper(
        (Kind.TSTATE, Kind.SCALAR, Kind.FRAME, Kind.INSTR), Kind.SCALAR, True, True),
    "monitor_reraise": native_helper(
        (Kind.TSTATE, Kind.FRAME, Kind.INSTR), Kind.VOID, callback=True),
    "PyExceptionInstance_Check": native_helper((Kind.OBJECT,), Kind.SCALAR),
    "PyErr_GivenExceptionMatches": native_helper((Kind.OBJECT, Kind.OBJECT), Kind.SCALAR),
    "Py_NewRef": native_helper((Kind.OBJECT,), Kind.OBJECT, result_owned=True),
    "_PyErr_SetRaisedException": native_helper(
        (Kind.TSTATE, Kind.OBJECT), Kind.VOID, callback=True, steals=(1,)),
    # The macro has its own checked argument/terminal grammar below. Its
    # generated helper also owns the encoded input window and recipient.
    "IR_COMPLETE_STEP": native_helper((), Kind.VOID, True, True),
    "_Py_call_instrumentation_line": native_helper(
        (Kind.TSTATE, Kind.FRAME, Kind.INSTR, Kind.INSTR), Kind.OBSERVED_OPCODE, True, True),
    "_Py_call_instrumentation_instruction": native_helper(
        (Kind.TSTATE, Kind.FRAME, Kind.INSTR), Kind.OBSERVED_OPCODE, True, True),
    "_PyCode_GetOriginalOpcode": native_helper((Kind.LINE_INFO, Kind.SITE_INDEX), Kind.OBSERVED_OPCODE),
    "INSTR_OFFSET_OF": native_helper((Kind.INSTR,), Kind.SITE_INDEX),
    "_PyFb_HasEntry": native_helper((Kind.OBSERVED_OPCODE,), Kind.SCALAR),
    "FB_HEADER_OF": native_helper((Kind.INSTR, Kind.OBSERVED_OPCODE), Kind.FEEDBACK),
    "PAUSE_ADAPTIVE_COUNTER": native_helper((Kind.FEEDBACK,), Kind.VOID),
    "ORIGINAL_HANDLER": native_helper((Kind.OBSERVED_OPCODE,), Kind.SCALAR),
    "PRE_DISPATCH_GOTO": native_helper((), Kind.VOID),
}

# LINE may adopt a debugger target before failing. The setter normalizes that
# target: context0 protected targets retain no scratch; protected-context paths
# retain only matching exception origins. It clears other scratch and restores
# saved bindings only to locals (register_resume_plan/frame_lineno_set_impl).
# Its target PRE absence is therefore distinct from the old-PC snapshot. No
# error phase/view is retained across the redirect. INSTRUCTION rejects jumps.


SCALAR_CONSTANTS = {"_PY_EVAL_EVENTS_MASK", "RESUME_OPARG_LOCATION_MASK",
                    "RESUME_AFTER_YIELD_FROM", "PY_MONITORING_EVENT_PY_START",
                    "PY_MONITORING_EVENT_PY_RESUME", "NUM_COMMON_CONSTANTS"}
SCALAR_KINDS = {Kind.SCALAR, Kind.OBSERVED_OPCODE, Kind.SITE_INDEX}
VALUE_CONSTANTS = {"PyValue_None", "PyValue_True", "PyValue_False"}


def matches_kind(actual, expected):
    return actual == expected or expected == Kind.SCALAR and actual in SCALAR_KINDS


@dataclass
class _Origin:
    reachable: bool = True
    changed: bool = False
    native_pending: set[int] = field(default_factory=set)
    native_bindings: dict[str, int] = field(default_factory=dict)
    native_valid: bool = True
    native_nonnull: set[int] = field(default_factory=set)

    def copy(self):
        return _Origin(self.reachable, self.changed, self.native_pending.copy(),
                       self.native_bindings.copy(), self.native_valid,
                       self.native_nonnull.copy())

    def native_closed(self):
        return self.native_valid and not self.native_pending


@dataclass(frozen=True)
class _Expression:
    kind: Kind | None
    owner: int | None = None
    created: frozenset[int] = frozenset()
    consumed: frozenset[int] = frozenset()
    valid: bool = True
    # The true arm proves this native owner absent when the boolean is true.
    null_guard: tuple[int, bool] | None = None
    value_result: HelperEffect | None = None
    nonnull: bool = False


def _expression(kind, *operands, owner=None, created=(), consumed=(), valid=True,
                null_guard=None, value_result=None, nonnull=False):
    made, taken = set(created), set(consumed)
    for operand in operands:
        valid &= operand.valid and not (taken & operand.consumed)
        made.update(operand.created)
        taken.update(operand.consumed)
    return _Expression(kind, owner, frozenset(made), frozenset(taken), valid,
                       null_guard, value_result, nonnull)


def _apply_native(value, origin):
    available = origin.native_pending | value.created
    origin.native_valid &= value.valid and value.consumed <= available
    origin.native_pending = available - value.consumed
    origin.native_nonnull.difference_update(value.consumed)
    if value.owner is not None and value.nonnull:
        origin.native_nonnull.add(value.owner)


def _null_arm(value, origin, truth):
    if value.null_guard is not None:
        owner, absent = value.null_guard
        if absent == truth:
            origin.native_pending.discard(owner)
            origin.native_nonnull.discard(owner)
        else:
            origin.native_nonnull.add(owner)


@dataclass(frozen=True)
class SourceErrorEffects:
    error_sources: frozenset[int]
    unwind_sources: frozenset[int]
    terminal_sources: frozenset[int | tuple[int, str]]
    helpers: frozenset[str]
    homes: HomeEffect
    may_fail: bool
    callback: bool
    changes_origin: bool
    # These native-object counts are separate from canonical PyValue owners
    # in Storage. A source error/entry identity alone grants no home shape.
    native_closed_errors: frozenset[int] = frozenset()
    native_closed_entries: frozenset[int] = frozenset()
    native_exit_closed: bool = False
    # Actual assignment-statement identity, not a helper/name inference in
    # the emitter. VALUE ownership is accounted by the Storage ledger.
    value_results: tuple[tuple[int, HelperEffect], ...] = ()


def checked_native_step_effects(parts: tuple[Uop, ...]) -> SourceErrorEffects | None:
    """Collect route identities after the caller proves the complete adapter.

    This is not a second source grammar: the caller must supply the parts
    from its existing positive native-step body/IO contract. Entered heap
    children are disjoint, and the step helper owns encoded window/result
    transport; callbacks preserve absence elsewhere. Every listed identity
    still needs an independent reached emitter receipt.
    """
    if not parts or len({id(part) for part in parts}) != len(parts):
        return None
    tokens = [token for part in parts for token in part.body.tokens()]
    errors = frozenset(id(token) for token in tokens if token.text in {
        "ERROR_IF", "ERROR_NO_POP", "IR_COMPLETE_STEP", "IR_ENTER_GENERATOR"})
    terminals = {id(token) for token in tokens if token.text in {
        "IR_COMPLETE_STEP", "IR_ENTER_GENERATOR", "EXIT_IF", "DEOPT_IF",
        "DISPATCH_SAME_OPARG"}}
    # A native object projection in a guard requires an implicit pointer
    # guard, independently of its later source-level type check.
    terminals.update((id(part), name) for part in parts
                     if part.is_guard() and not part.properties.inline_ok
                     for name in part.object_inputs if name not in part.asserted_objects)
    return SourceErrorEffects(errors, frozenset(), frozenset(terminals), frozenset(),
                              HomeEffect.PRESERVES_SCRATCH_ABSENCE,
                              True, True, False)


def _call(tokens: list[Token]) -> list[list[Token]] | None:
    if len(tokens) < 3:
        return None
    iterator = TokenIterator(tokens[1:])
    try:
        arguments, _ = instruction_macro_args(tokens[0], iterator)
    except (SyntaxError, StopIteration):
        return None
    return arguments if iterator.peek() is None else None


def analyze_error_effects(inst: Instruction, reserved_names=(), *,
                          observer_wrapper=False) -> SourceErrorEffects | None:
    """Summarize complete typed source, including an empty error-source set.

    Token identities are meaningful only for this analysis object. No summary
    alone proves that the emitter visited every route or finalized its homes.
    Observer mode checks home confinement across intentional PC adoption; it
    cannot supply an ordinary failing-origin or readable-shape certificate.
    """
    parts = [part for part in inst.parts if isinstance(part, Uop)]
    # Source identities cannot distinguish two emissions of the same uop.
    # Keep that case uncovered until receipts carry a part occurrence too.
    if len({id(part) for part in parts}) != len(parts):
        return None
    errors: set[int] = set()
    unwinds: set[int] = set()
    terminals: set[int] = set()
    helpers: set[str] = set()
    native_closed_errors: set[int] = set()
    native_closed_entries: set[int] = set()
    value_results: dict[int, HelperEffect] = {}
    changes_origin = False
    reserved = {"frame", "slots", "window", "tstate", "next_instr", "this_instr",
                "oparg", "PyValue_NULL", "FRAME_CO_CONSTS", "NULL", "true", "false",
                "_PyFrame_GetCode", "ERROR_IF", "ERROR_NO_POP", "JUMPTO", "JUMPBY",
                "JUMP_TARGET",
                "DISPATCH", "DISPATCH_SAME_OPARG", "assert", "RELOAD_STACK",
                "IR_COMPLETE_STEP", "prev_instr", "DISPATCH_GOTO",
                *HELPERS, *SCALAR_CONSTANTS, *VALUE_CONSTANTS, *reserved_names}

    def expression(tokens, variables, origin):
        if not tokens:
            return _expression(None)
        texts = [token.text for token in tokens]
        if texts == ["tstate", "->", "interp"]:
            return _expression(Kind.INTERP)
        if texts == ["tstate", "->", "tracing"]:
            return _expression(Kind.SCALAR)
        if texts == ["&", "tstate", "->", "eval_breaker"]:
            return _expression(Kind.ATOMIC_ADDRESS)
        if texts == ["_PyFrame_GetCode", "(", "frame", ")", "->",
                     "_co_instrumentation_version"]:
            return _expression(Kind.ATOMIC_FIELD)
        if texts == ["frame", "->", "instr_ptr"]:
            return _expression(Kind.INSTR)
        # These projections use immutable admitted operand geometry. Neither
        # returns a home address nor executes a callback. Bounds remain the
        # universal word validator's obligation, not a source-effect grant.
        if texts == ["tstate", "->", "interp", "->", "common_consts", "[", "oparg", "]"]:
            return _expression(Kind.VALUE)
        if texts == ["JUMP_TARGET", "(", "oparg", ")"]:
            return _expression(Kind.INSTR)
        if (observer_wrapper and len(texts) == 5 and
                variables.get(texts[0]) == Kind.CODE and
                texts[1:] == ["->", "_co_monitoring", "->", "lines"]):
            return _expression(Kind.LINE_INFO)
        if observer_wrapper and len(texts) >= 3 and texts[-2] == "+":
            left = expression(tokens[:-2], variables, origin)
            right = expression(tokens[-1:], variables, origin)
            if left.kind == Kind.INSTR and right.kind == Kind.SCALAR:
                return _expression(Kind.INSTR, left, right)
        if (len(texts) == 9 and texts[:5] == ["(", "(", "PyStopIterationObject", "*", ")"]
                and variables.get(texts[5]) == Kind.OBJECT
                and texts[6:] == [")", "->", "value"]):
            return _expression(Kind.VALUE, expression(tokens[5:6], variables, origin))
        # Conditions may compare native values, but cannot form an address,
        # dereference a pointer, assign, or hide a C side effect.
        depth = 0
        question = None
        for at, text in enumerate(texts):
            depth += (text == "(") - (text == ")")
            if depth == 0 and text == "?":
                question = at
            elif depth == 0 and text == ":" and question is not None:
                condition = expression(tokens[:question], variables, origin)
                left = expression(tokens[question + 1:at], variables, origin)
                right = expression(tokens[at + 1:], variables, origin)
                kind = left.kind if condition.kind in SCALAR_KINDS and left.kind == right.kind else None
                return _expression(kind, condition, left, right,
                    valid=not any(value.created or value.consumed or value.owner is not None
                                  for value in (condition, left, right)))
        depth = 0
        for at, text in enumerate(texts):
            if text == "(":
                depth += 1
            elif text == ")":
                depth -= 1
            elif depth == 0 and text in {"==", "!=", "<", ">", "<=", ">=", "&&", "||"}:
                left = expression(tokens[:at], variables, origin)
                right = expression(tokens[at + 1:], variables, origin)
                comparable = {*SCALAR_KINDS, Kind.OBJECT, Kind.NULL}
                kind = (Kind.SCALAR if (left.kind in comparable and right.kind in comparable)
                        or left.kind == right.kind == Kind.INSTR else None)
                guard = None
                if text in {"==", "!="}:
                    owner = (left.owner if right.kind == Kind.NULL else
                             right.owner if left.kind == Kind.NULL else None)
                    if owner is not None:
                        guard = owner, text == "=="
                return _expression(kind, left, right, null_guard=guard,
                    valid=text not in {"&&", "||"} or not any(
                        value.created or value.consumed for value in (left, right)))
            elif depth == 0 and text in {"&", "|", "^"}:
                left = expression(tokens[:at], variables, origin)
                right = expression(tokens[at + 1:], variables, origin)
                return _expression(Kind.SCALAR if left.kind == right.kind == Kind.SCALAR
                                   else None, left, right)
        if texts[0] in {"!", "~"}:
            value = expression(tokens[1:], variables, origin)
            guard = (value.null_guard[0], not value.null_guard[1]) if (
                texts[0] == "!" and value.null_guard is not None) else None
            return _expression(Kind.SCALAR if value.kind in SCALAR_KINDS else None,
                               value, null_guard=guard)
        if len(tokens) == 1:
            if texts[0] in {"true", "false", *SCALAR_CONSTANTS} or texts[0].isdigit():
                return _expression(Kind.SCALAR)
            if texts[0] == "NULL":
                return _expression(Kind.NULL)
            if texts[0] in {"PyValue_NULL", *VALUE_CONSTANTS}:
                return _expression(Kind.VALUE)
            if texts[0] == "FRAME_CO_CONSTS":
                return _expression(Kind.TUPLE)
            if texts[0] == "tstate":
                return _expression(Kind.TSTATE)
            if texts[0] == "frame":
                return _expression(Kind.FRAME)
            if texts[0] in {"this_instr", "next_instr"} or (
                    observer_wrapper and texts[0] == "prev_instr"):
                return _expression(Kind.INSTR)
            if texts[0] == "PyExc_StopIteration":
                return _expression(Kind.OBJECT)
            owner = origin.native_bindings.get(texts[0])
            return _expression(variables.get(texts[0]), owner=owner,
                               valid=owner is None or owner in origin.native_pending,
                               nonnull=owner in origin.native_nonnull)
        if texts[0] == "(" and texts[-1] == ")":
            return expression(tokens[1:-1], variables, origin)
        # Code projection is read-only, and does not grant the caller a frame
        # pointer it could pass to an otherwise native helper.
        if texts == ["_PyFrame_GetCode", "(", "frame", ")"]:
            helpers.add("_PyFrame_GetCode")
            return _expression(Kind.CODE)
        contract = HELPERS.get(texts[0])
        if contract is None or texts[0] in variables:
            return _expression(None)
        arguments = _call(tokens)
        if arguments is None or len(arguments) != len(contract.inputs):
            return _expression(None)
        values = [expression(argument, variables, origin) for argument in arguments]
        if any(not matches_kind(value.kind, kind)
               for value, kind in zip(values, contract.inputs)):
            return _expression(None)
        observer_arguments = {
            "_Py_call_instrumentation_line": ("tstate", "frame", "this_instr", "prev_instr"),
            "_Py_call_instrumentation_instruction": ("tstate", "frame", "this_instr"),
            "INSTR_OFFSET_OF": ("this_instr",),
        }
        if texts[0] in observer_arguments and tuple(tuple(t.text for t in arg)
                for arg in arguments) != tuple((name,) for name in observer_arguments[texts[0]]):
            return _expression(None)
        if texts[0] == "FB_HEADER_OF" and [t.text for t in arguments[0]] != ["next_instr"]:
            return _expression(None)
        helpers.add(texts[0])
        stolen = [values[index].owner for index in contract.steals]
        valid = all(owner is not None for owner in stolen) and len(stolen) == len(set(stolen))
        valid &= contract.result != Kind.OBJECT or contract.result_owned is not None
        # A direct canonical assignment hands the stolen owner to Storage's
        # PyValue ledger. Hiding that temporary inside another call has no
        # such publication/retirement receipt yet.
        valid &= not any(value.kind == Kind.VALUE and value.consumed for value in values)
        owner = id(tokens[0]) if contract.result == Kind.OBJECT and contract.result_owned else None
        result = contract if contract.result == Kind.VALUE else None
        if result is not None and any(not values[index].nonnull for index in contract.steals):
            # FromObjectSteal's non-NULL assertion is a precondition. An
            # owning pointer may still be NULL before its actual guard.
            result = replace(result, result_owned=None)
        return _expression(contract.result, *values, owner=owner,
                           created=() if owner is None else (owner,),
                           consumed=(token for token in stolen if token is not None),
                           valid=valid, value_result=result,
                           nonnull=contract.result_nonnull)

    def statement(stmt, variables, origin):
        nonlocal changes_origin
        if isinstance(stmt, BlockStmt):
            return all(statement(child, variables, origin) for child in stmt.body)
        if isinstance(stmt, IfStmt):
            condition = expression(stmt.condition, variables, origin)
            if condition.kind not in SCALAR_KINDS:
                return False
            _apply_native(condition, origin)
            arms = [origin.copy() for _ in range(2)]
            for truth, arm in zip((True, False), arms):
                _null_arm(condition, arm, truth)
            branches = [variables.copy(), variables.copy()]
            if not statement(stmt.body, branches[0], arms[0]):
                return False
            if stmt.else_body is not None and not statement(stmt.else_body, branches[1], arms[1]):
                return False
            for name in variables:
                left, right = branches[0].get(name), branches[1].get(name)
                variables[name] = left if left == right else Kind.SCALAR if (
                    left in SCALAR_KINDS and right in SCALAR_KINDS) else variables[name]
            origin.reachable = any(arm.reachable for arm in arms)
            origin.changed = any(arm.reachable and arm.changed for arm in arms)
            merge_native(origin, arms)
            return True
        if isinstance(stmt, MacroIfStmt):
            # Both native configurations must have the same typed locals.
            # No textual macro may introduce hidden statements or aliases.
            if stmt.condition.text.strip() != "#ifdef Py_GIL_DISABLED":
                return False
            branches = [variables.copy(), variables.copy()]
            arms = [origin.copy() for _ in range(2)]
            for body, scope, arm in zip((stmt.body, stmt.else_body or ()), branches, arms):
                if not all(statement(child, scope, arm) for child in body):
                    return False
            variables.update({name: kind for name, kind in branches[0].items()
                              if branches[1].get(name) == kind})
            origin.reachable = any(arm.reachable for arm in arms)
            origin.changed = any(arm.reachable and arm.changed for arm in arms)
            merge_native(origin, arms)
            return True
        if not isinstance(stmt, SimpleStmt):
            return False
        tokens = [token for token in stmt.contents if token.kind != "COMMENT"]
        if not tokens or tokens[-1].text != ";":
            return False
        tokens = tokens[:-1]
        texts = [token.text for token in tokens]
        if texts[:1] == ["goto"]:
            if observer_wrapper and texts == ["goto", "error"]:
                errors.add(id(tokens[0]))
                if origin.native_closed():
                    native_closed_errors.add(id(tokens[0]))
                origin.reachable = False
                return True
            if texts != ["goto", "exception_unwind"] or origin.changed:
                return False
            unwinds.add(id(tokens[0]))
            if origin.native_closed():
                native_closed_errors.add(id(tokens[0]))
            origin.reachable = False
            return True
        if texts[:1] == ["RELOAD_STACK"]:
            arguments = _call(tokens)
            return arguments is not None and not arguments
        if texts[:1] == ["IR_COMPLETE_STEP"]:
            arguments = _call(tokens)
            if (arguments is None or len(arguments) != 3 or origin.changed
                    or [t.text for t in arguments[0]] not in (
                        ["_PY_IR_FOR_ITER"], ["_PY_IR_SEND"], ["_PY_IR_SEND_THROW"])
                    or expression(arguments[1], variables, origin).kind != Kind.VALUE
                    or [t.text for t in arguments[2]] != ["oparg"]):
                return False
            helpers.add("IR_COMPLETE_STEP")
            _apply_native(expression(arguments[1], variables, origin), origin)
            errors.add(id(tokens[0]))
            if origin.native_closed():
                native_closed_errors.add(id(tokens[0]))
            terminals.add(id(tokens[0]))
            origin.reachable = False
            return True
        if texts[:2] == ["next_instr", "="]:
            value = expression(tokens[2:], variables, origin)
            if value.kind != Kind.INSTR:
                return False
            _apply_native(value, origin)
            changes_origin = origin.changed = True
            return True
        if texts[:1] in (["ERROR_IF"], ["ERROR_NO_POP"], ["JUMPTO"], ["JUMPBY"],
                         ["DISPATCH"], ["DISPATCH_SAME_OPARG"], ["assert"],
                         *([["DISPATCH_GOTO"]] if observer_wrapper else [])):
            arguments = _call(tokens)
            if arguments is None:
                return False
            name = texts[0]
            count = 0 if name in {"ERROR_NO_POP", "DISPATCH", "DISPATCH_SAME_OPARG",
                                 "DISPATCH_GOTO"} else 1
            values = [expression(arg, variables, origin) for arg in arguments]
            if len(arguments) != count or any(value.kind not in SCALAR_KINDS for value in values):
                return False
            for value in values:
                _apply_native(value, origin)
            if name in {"ERROR_IF", "ERROR_NO_POP"}:
                if origin.changed and not observer_wrapper:
                    return False
                errors.add(id(tokens[0]))
                error_origin = origin.copy()
                if values:
                    _null_arm(values[0], error_origin, True)
                if error_origin.native_closed():
                    native_closed_errors.add(id(tokens[0]))
                if values:
                    _null_arm(values[0], origin, False)
                if name == "ERROR_NO_POP" or [t.text for t in arguments[0]] == ["true"]:
                    origin.reachable = False
            elif name in {"DISPATCH", "DISPATCH_SAME_OPARG", "DISPATCH_GOTO"}:
                terminals.add(id(tokens[0]))
                origin.reachable = False
            elif name in {"JUMPTO", "JUMPBY"}:
                if name != "JUMPBY" or [t.text for t in arguments[0]] != ["0"]:
                    changes_origin = origin.changed = True
            return True
        declaration = 0
        kind = None
        if texts[:2] == ["PyObject", "*"]:
            declaration, kind = 2, Kind.OBJECT
        elif observer_wrapper and texts[:2] == ["PyCodeObject", "*"]:
            declaration, kind = 2, Kind.CODE
        elif texts[:1] in (["PyValue"], ["int"], ["uintptr_t"]):
            declaration, kind = 1, Kind.VALUE if texts[0] == "PyValue" else Kind.SCALAR
        if declaration:
            if (len(tokens) < declaration + 3 or tokens[declaration].kind != "IDENTIFIER"
                    or texts[declaration] in variables or texts[declaration] in reserved
                    or texts[declaration + 1] != "="):
                return False
            value = expression(tokens[declaration + 2:], variables, origin)
            if not matches_kind(value.kind, kind):
                return False
            assign_native(texts[declaration], value, origin)
            if value.value_result is not None:
                value_results[id(stmt)] = (value.value_result if value.valid else
                    replace(value.value_result, result_owned=None))
            variables[texts[declaration]] = value.kind
            return True
        if len(tokens) >= 3 and texts[1] == "=" and texts[0] in variables:
            value = expression(tokens[2:], variables, origin)
            actual = value.kind
            expected = variables[texts[0]]
            if texts[0] == "oparg" or not (actual == expected or
                    actual in SCALAR_KINDS and expected in SCALAR_KINDS):
                return False
            assign_native(texts[0], value, origin)
            if value.value_result is not None:
                value_results[id(stmt)] = (value.value_result if value.valid else
                    replace(value.value_result, result_owned=None))
            variables[texts[0]] = actual
            return True
        value = expression(tokens, variables, origin)
        if value.kind != Kind.VOID:
            return False
        _apply_native(value, origin)
        return True

    def assign_native(name, value, origin):
        # An object pointer copy is not a new owning reference. Keep the
        # existing source census, but do not certify ambiguous native aliases
        # or overwrite an unretired native owner.
        previous = origin.native_bindings.pop(name, None)
        if previous in origin.native_pending:
            origin.native_valid = False
        if value.owner is not None and value.owner not in value.created:
            origin.native_valid = False
        _apply_native(value, origin)
        if value.owner is not None:
            origin.native_bindings[name] = value.owner

    def merge_native(origin, arms):
        reached = [arm for arm in arms if arm.reachable]
        if not reached:
            return
        origin.native_pending = set().union(*(arm.native_pending for arm in reached))
        origin.native_valid = all(arm.native_valid for arm in reached)
        origin.native_nonnull = set.intersection(*(arm.native_nonnull for arm in reached))
        common = reached[0].native_bindings
        origin.native_bindings = {name: owner for name, owner in common.items()
            if all(arm.native_bindings.get(name) == owner for arm in reached[1:])}
        # A branch-dependent identity may remain live without a common name.
        # It must not later be discharged by an unrelated source-level alias.
        if any(owner not in origin.native_bindings.values() for owner in origin.native_pending):
            origin.native_valid = False

    origin = _Origin()
    for part in inst.parts:
        if isinstance(part, Skip):
            continue
        if (not isinstance(part, Uop) or part.caches
                or any(item.is_array() for item in (*part.stack.inputs, *part.stack.outputs))
                or origin.changed and part.object_inputs):
            return None
        names = [item.name for item in (*part.stack.inputs, *part.stack.outputs)]
        if any(name in reserved for name in names):
            return None
        variables = {name: Kind.VALUE for name in names}
        variables["oparg"] = Kind.SCALAR
        if origin.reachable and origin.native_closed():
            native_closed_entries.add(id(part))
        origin.native_bindings = {}
        if observer_wrapper:
            variables["opcode"] = Kind.SCALAR
        if not statement(part.body, variables, origin):
            return None
    used = [HELPERS[name] for name in helpers if name in HELPERS]
    homes = (HomeEffect.UNKNOWN if any(h.homes == HomeEffect.UNKNOWN for h in used)
             else HomeEffect.PRESERVES_SCRATCH_ABSENCE
             if any(h.homes == HomeEffect.PRESERVES_SCRATCH_ABSENCE for h in used)
             else HomeEffect.PRESERVES_CANONICAL_OR_NULL
             if any(h.homes == HomeEffect.PRESERVES_CANONICAL_OR_NULL for h in used)
             else HomeEffect.NO_ARENA_WRITE)
    return SourceErrorEffects(frozenset(errors), frozenset(unwinds), frozenset(terminals),
                              frozenset(helpers), homes, any(h.may_fail for h in used),
                              any(h.callback for h in used), changes_origin,
                              frozenset(native_closed_errors), frozenset(native_closed_entries),
                              origin.reachable and origin.native_closed(),
                              tuple(value_results.items()))
