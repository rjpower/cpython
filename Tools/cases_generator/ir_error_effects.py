"""Checked source effects for exceptional ingress, independent of normal IO.

This is a small typed subset of the instruction DSL, not a C verifier. Unknown
source has no summary. The emitter must match the returned source identities
to actual flush/terminal receipts before exporting a complete route contract.
"""

from dataclasses import dataclass
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


@dataclass(frozen=True)
class HelperEffect:
    inputs: tuple[Kind, ...]
    result: Kind
    homes: HomeEffect
    may_fail: bool = False
    callback: bool = False


def native_helper(inputs, result, may_fail=False, callback=False):
    homes = (HomeEffect.PRESERVES_SCRATCH_ABSENCE if callback
             else HomeEffect.NO_ARENA_WRITE)
    return HelperEffect(inputs, result, homes, may_fail, callback)


# These interfaces receive native values, never an arena-home address. A
# callback may change named bindings or vacate scratch through a debugger
# redirect. It cannot publish a new scratch owner. Generated operand writes
# remain a separate emitter obligation. A returned failure is not a goto.
HELPERS = {
    "PyValue_DUP": native_helper((Kind.VALUE,), Kind.VALUE),
    "PyValue_XDUP": native_helper((Kind.VALUE,), Kind.VALUE),
    "TAKE": native_helper((Kind.VALUE,), Kind.VALUE),
    "PyValue_IsNull": native_helper((Kind.VALUE,), Kind.SCALAR),
    "PyValue_IsNone": native_helper((Kind.VALUE,), Kind.SCALAR),
    "PyValue_IsInlineInt": native_helper((Kind.VALUE,), Kind.SCALAR),
    "PyValue_IsInlineFloat": native_helper((Kind.VALUE,), Kind.SCALAR),
    "PyValue_FromInlineInt": native_helper((Kind.SCALAR,), Kind.VALUE),
    "PyValue_AsKnownObjectBorrow": native_helper((Kind.VALUE,), Kind.OBJECT),
    "PyValue_FromObjectSteal": native_helper((Kind.OBJECT,), Kind.VALUE),
    "PyValue_CLOSE": native_helper((Kind.VALUE,), Kind.VOID, callback=True),
    "PyValue_XCLOSE": native_helper((Kind.VALUE,), Kind.VOID, callback=True),
    "DEAD": native_helper((Kind.VALUE,), Kind.VOID),
    # Their emitted writes are confined by IRStack, independently of this
    # source summary. They grant no permission to pass a home to a C helper.
    "DECREF_INPUTS": native_helper((), Kind.VOID, callback=True),
    "INPUTS_DEAD": native_helper((), Kind.VOID),
    "GETLOCAL": native_helper((Kind.SCALAR,), Kind.VALUE),
    "PyTuple_GET_VALUE": native_helper((Kind.TUPLE, Kind.SCALAR), Kind.VALUE),
    "_PyTuple_GET_VALUE": native_helper((Kind.TUPLE, Kind.SCALAR), Kind.VALUE),
    "_PyEval_GetANext": native_helper((Kind.OBJECT,), Kind.OBJECT, True, True),
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
    "Py_NewRef": native_helper((Kind.OBJECT,), Kind.OBJECT),
    "_PyErr_SetRaisedException": native_helper(
        (Kind.TSTATE, Kind.OBJECT), Kind.VOID, callback=True),
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


def matches_kind(actual, expected):
    return actual == expected or expected == Kind.SCALAR and actual in SCALAR_KINDS


@dataclass
class _Origin:
    reachable: bool = True
    changed: bool = False


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
    changes_origin = False
    reserved = {"frame", "slots", "window", "tstate", "next_instr", "this_instr",
                "oparg", "PyValue_NULL", "FRAME_CO_CONSTS", "NULL", "true", "false",
                "_PyFrame_GetCode", "ERROR_IF", "ERROR_NO_POP", "JUMPTO", "JUMPBY",
                "JUMP_TARGET",
                "DISPATCH", "DISPATCH_SAME_OPARG", "assert", "RELOAD_STACK",
                "IR_COMPLETE_STEP", "prev_instr", "DISPATCH_GOTO",
                *HELPERS, *SCALAR_CONSTANTS, *reserved_names}

    def expression(tokens, variables):
        if not tokens:
            return None
        texts = [token.text for token in tokens]
        if texts == ["tstate", "->", "interp"]:
            return Kind.INTERP
        if texts == ["tstate", "->", "tracing"]:
            return Kind.SCALAR
        if texts == ["&", "tstate", "->", "eval_breaker"]:
            return Kind.ATOMIC_ADDRESS
        if texts == ["_PyFrame_GetCode", "(", "frame", ")", "->",
                     "_co_instrumentation_version"]:
            return Kind.ATOMIC_FIELD
        if texts == ["frame", "->", "instr_ptr"]:
            return Kind.INSTR
        # These projections use immutable admitted operand geometry. Neither
        # returns a home address nor executes a callback. Bounds remain the
        # universal word validator's obligation, not a source-effect grant.
        if texts == ["tstate", "->", "interp", "->", "common_consts", "[", "oparg", "]"]:
            return Kind.VALUE
        if texts == ["JUMP_TARGET", "(", "oparg", ")"]:
            return Kind.INSTR
        if (observer_wrapper and len(texts) == 5 and
                variables.get(texts[0]) == Kind.CODE and
                texts[1:] == ["->", "_co_monitoring", "->", "lines"]):
            return Kind.LINE_INFO
        if (observer_wrapper and len(texts) >= 3 and texts[-2] == "+" and
                expression(tokens[:-2], variables) == Kind.INSTR and
                expression(tokens[-1:], variables) == Kind.SCALAR):
            return Kind.INSTR
        if (len(texts) == 9 and texts[:5] == ["(", "(", "PyStopIterationObject", "*", ")"]
                and variables.get(texts[5]) == Kind.OBJECT
                and texts[6:] == [")", "->", "value"]):
            return Kind.VALUE
        # Conditions may compare native values, but cannot form an address,
        # dereference a pointer, assign, or hide a C side effect.
        depth = 0
        question = None
        for at, text in enumerate(texts):
            depth += (text == "(") - (text == ")")
            if depth == 0 and text == "?":
                question = at
            elif depth == 0 and text == ":" and question is not None:
                condition = expression(tokens[:question], variables)
                left = expression(tokens[question + 1:at], variables)
                right = expression(tokens[at + 1:], variables)
                return left if condition in SCALAR_KINDS and left == right else None
        depth = 0
        for at, text in enumerate(texts):
            if text == "(":
                depth += 1
            elif text == ")":
                depth -= 1
            elif depth == 0 and text in {"==", "!=", "<", ">", "<=", ">=", "&&", "||"}:
                left = expression(tokens[:at], variables)
                right = expression(tokens[at + 1:], variables)
                comparable = {*SCALAR_KINDS, Kind.OBJECT, Kind.NULL}
                return (Kind.SCALAR if (left in comparable and right in comparable)
                        or left == right == Kind.INSTR else None)
            elif depth == 0 and text in {"&", "|", "^"}:
                return (Kind.SCALAR if expression(tokens[:at], variables) == Kind.SCALAR
                        and expression(tokens[at + 1:], variables) == Kind.SCALAR else None)
        if texts[0] in {"!", "~"}:
            return Kind.SCALAR if expression(tokens[1:], variables) in SCALAR_KINDS else None
        if len(tokens) == 1:
            if texts[0] in {"true", "false", *SCALAR_CONSTANTS} or texts[0].isdigit():
                return Kind.SCALAR
            if texts[0] == "NULL":
                return Kind.NULL
            if texts[0] == "PyValue_NULL":
                return Kind.VALUE
            if texts[0] == "FRAME_CO_CONSTS":
                return Kind.TUPLE
            if texts[0] == "tstate":
                return Kind.TSTATE
            if texts[0] == "frame":
                return Kind.FRAME
            if texts[0] in {"this_instr", "next_instr"} or (
                    observer_wrapper and texts[0] == "prev_instr"):
                return Kind.INSTR
            if texts[0] == "PyExc_StopIteration":
                return Kind.OBJECT
            return variables.get(texts[0])
        if texts[0] == "(" and texts[-1] == ")":
            return expression(tokens[1:-1], variables)
        # Code projection is read-only, and does not grant the caller a frame
        # pointer it could pass to an otherwise native helper.
        if texts == ["_PyFrame_GetCode", "(", "frame", ")"]:
            helpers.add("_PyFrame_GetCode")
            return Kind.CODE
        contract = HELPERS.get(texts[0])
        if contract is None or texts[0] in variables:
            return None
        arguments = _call(tokens)
        if arguments is None or len(arguments) != len(contract.inputs):
            return None
        if any(not matches_kind(expression(argument, variables), kind)
               for argument, kind in zip(arguments, contract.inputs)):
            return None
        observer_arguments = {
            "_Py_call_instrumentation_line": ("tstate", "frame", "this_instr", "prev_instr"),
            "_Py_call_instrumentation_instruction": ("tstate", "frame", "this_instr"),
            "INSTR_OFFSET_OF": ("this_instr",),
        }
        if texts[0] in observer_arguments and tuple(tuple(t.text for t in arg)
                for arg in arguments) != tuple((name,) for name in observer_arguments[texts[0]]):
            return None
        if texts[0] == "FB_HEADER_OF" and [t.text for t in arguments[0]] != ["next_instr"]:
            return None
        helpers.add(texts[0])
        return contract.result

    def statement(stmt, variables, origin):
        nonlocal changes_origin
        if isinstance(stmt, BlockStmt):
            return all(statement(child, variables, origin) for child in stmt.body)
        if isinstance(stmt, IfStmt):
            if expression(stmt.condition, variables) not in SCALAR_KINDS:
                return False
            arms = [_Origin(origin.reachable, origin.changed) for _ in range(2)]
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
            return True
        if isinstance(stmt, MacroIfStmt):
            # Both native configurations must have the same typed locals.
            # No textual macro may introduce hidden statements or aliases.
            if stmt.condition.text.strip() != "#ifdef Py_GIL_DISABLED":
                return False
            branches = [variables.copy(), variables.copy()]
            arms = [_Origin(origin.reachable, origin.changed) for _ in range(2)]
            for body, scope, arm in zip((stmt.body, stmt.else_body or ()), branches, arms):
                if not all(statement(child, scope, arm) for child in body):
                    return False
            variables.update({name: kind for name, kind in branches[0].items()
                              if branches[1].get(name) == kind})
            origin.reachable = any(arm.reachable for arm in arms)
            origin.changed = any(arm.reachable and arm.changed for arm in arms)
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
                origin.reachable = False
                return True
            if texts != ["goto", "exception_unwind"] or origin.changed:
                return False
            unwinds.add(id(tokens[0]))
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
                    or expression(arguments[1], variables) != Kind.VALUE
                    or [t.text for t in arguments[2]] != ["oparg"]):
                return False
            helpers.add("IR_COMPLETE_STEP")
            errors.add(id(tokens[0]))
            terminals.add(id(tokens[0]))
            origin.reachable = False
            return True
        if texts[:2] == ["next_instr", "="]:
            if expression(tokens[2:], variables) != Kind.INSTR:
                return False
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
            if len(arguments) != count or any(expression(arg, variables) not in SCALAR_KINDS
                                              for arg in arguments):
                return False
            if name in {"ERROR_IF", "ERROR_NO_POP"}:
                if origin.changed and not observer_wrapper:
                    return False
                errors.add(id(tokens[0]))
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
                    or texts[declaration + 1] != "="
                    or not matches_kind(expression(tokens[declaration + 2:], variables), kind)):
                return False
            variables[texts[declaration]] = expression(tokens[declaration + 2:], variables)
            return True
        if len(tokens) >= 3 and texts[1] == "=" and texts[0] in variables:
            actual = expression(tokens[2:], variables)
            expected = variables[texts[0]]
            if texts[0] == "oparg" or not (actual == expected or
                    actual in SCALAR_KINDS and expected in SCALAR_KINDS):
                return False
            variables[texts[0]] = actual
            return True
        return expression(tokens, variables) == Kind.VOID

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
        if observer_wrapper:
            variables["opcode"] = Kind.SCALAR
        if not statement(part.body, variables, origin):
            return None
    used = [HELPERS[name] for name in helpers if name in HELPERS]
    homes = (HomeEffect.UNKNOWN if any(h.homes == HomeEffect.UNKNOWN for h in used)
             else HomeEffect.PRESERVES_SCRATCH_ABSENCE
             if any(h.homes == HomeEffect.PRESERVES_SCRATCH_ABSENCE for h in used)
             else HomeEffect.NO_ARENA_WRITE)
    return SourceErrorEffects(frozenset(errors), frozenset(unwinds), frozenset(terminals),
                              frozenset(helpers), homes, any(h.may_fail for h in used),
                              any(h.callback for h in used), changes_origin)
