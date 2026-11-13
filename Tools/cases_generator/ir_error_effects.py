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
from parser import BlockStmt, IfStmt, SimpleStmt


class Kind(Enum):
    VALUE = auto()
    OBJECT = auto()
    SCALAR = auto()
    CODE = auto()
    TUPLE = auto()
    TSTATE = auto()
    INTERP = auto()
    VOID = auto()
    NULL = auto()


class HomeEffect(Enum):
    UNKNOWN = auto()
    NO_ARENA_WRITE = auto()


@dataclass(frozen=True)
class HelperEffect:
    inputs: tuple[Kind, ...]
    result: Kind
    homes: HomeEffect
    may_fail: bool = False
    callback: bool = False


def native_helper(inputs, result, may_fail=False, callback=False):
    return HelperEffect(inputs, result, HomeEffect.NO_ARENA_WRITE, may_fail, callback)


# These interfaces receive native values, never an arena-home address. A
# callback may change named bindings; its exceptional home footprint is a
# separate emitter obligation. A returned failure is not itself a goto.
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
    # These typed native interfaces are not yet an audited callback/redirect
    # footprint. Source classification must preserve that distinction.
    "check_periodics": HelperEffect((Kind.TSTATE,), Kind.SCALAR, HomeEffect.UNKNOWN, True, True),
    "_Py_Instrument": HelperEffect((Kind.CODE, Kind.INTERP), Kind.SCALAR, HomeEffect.UNKNOWN, True),
}


@dataclass(frozen=True)
class SourceErrorEffects:
    error_sources: frozenset[int]
    unwind_sources: frozenset[int]
    terminal_sources: frozenset[int]
    helpers: frozenset[str]
    homes: HomeEffect
    may_fail: bool
    callback: bool


def _call(tokens: list[Token]) -> list[list[Token]] | None:
    if len(tokens) < 3:
        return None
    iterator = TokenIterator(tokens[1:])
    try:
        arguments, _ = instruction_macro_args(tokens[0], iterator)
    except (SyntaxError, StopIteration):
        return None
    return arguments if iterator.peek() is None else None


def analyze_error_effects(inst: Instruction, reserved_names=()) -> SourceErrorEffects | None:
    """Summarize complete typed source, including an empty error-source set.

    Token identities are meaningful only for this analysis object. No summary
    alone proves that the emitter visited every route or finalized its homes.
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
    reserved = {"frame", "slots", "window", "tstate", "next_instr", "this_instr",
                "oparg", "PyValue_NULL", "FRAME_CO_CONSTS", "NULL", "true", "false",
                "_PyFrame_GetCode", "ERROR_IF", "ERROR_NO_POP", "JUMPTO", "JUMPBY",
                "DISPATCH", "DISPATCH_SAME_OPARG", "assert", *HELPERS, *reserved_names}

    def expression(tokens, variables):
        if not tokens:
            return None
        texts = [token.text for token in tokens]
        if texts == ["tstate", "->", "interp"]:
            return Kind.INTERP
        # Conditions may compare native values, but cannot form an address,
        # dereference a pointer, assign, or hide a C side effect.
        depth = 0
        for at, text in enumerate(texts):
            if text == "(":
                depth += 1
            elif text == ")":
                depth -= 1
            elif depth == 0 and text in {"==", "!=", "<", ">", "<=", ">=", "&&", "||"}:
                left = expression(tokens[:at], variables)
                right = expression(tokens[at + 1:], variables)
                comparable = {Kind.SCALAR, Kind.OBJECT, Kind.NULL}
                return Kind.SCALAR if left in comparable and right in comparable else None
        if texts[0] == "!":
            return Kind.SCALAR if expression(tokens[1:], variables) == Kind.SCALAR else None
        if len(tokens) == 1:
            if texts[0] in {"true", "false"} or texts[0].isdigit():
                return Kind.SCALAR
            if texts[0] == "NULL":
                return Kind.NULL
            if texts[0] == "PyValue_NULL":
                return Kind.VALUE
            if texts[0] == "FRAME_CO_CONSTS":
                return Kind.TUPLE
            if texts[0] == "tstate":
                return Kind.TSTATE
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
        if any(expression(argument, variables) != kind
               for argument, kind in zip(arguments, contract.inputs)):
            return None
        helpers.add(texts[0])
        return contract.result

    def statement(stmt, variables):
        if isinstance(stmt, BlockStmt):
            return all(statement(child, variables) for child in stmt.body)
        if isinstance(stmt, IfStmt):
            return (expression(stmt.condition, variables) == Kind.SCALAR
                    and statement(stmt.body, variables.copy())
                    and (stmt.else_body is None or statement(stmt.else_body, variables.copy())))
        if not isinstance(stmt, SimpleStmt):
            return False
        tokens = [token for token in stmt.contents if token.kind != "COMMENT"]
        if not tokens or tokens[-1].text != ";":
            return False
        tokens = tokens[:-1]
        texts = [token.text for token in tokens]
        if texts[:1] == ["goto"]:
            if texts != ["goto", "exception_unwind"]:
                return False
            unwinds.add(id(tokens[0]))
            return True
        if texts[:1] in (["ERROR_IF"], ["ERROR_NO_POP"], ["JUMPTO"], ["JUMPBY"],
                         ["DISPATCH"], ["DISPATCH_SAME_OPARG"], ["assert"]):
            arguments = _call(tokens)
            if arguments is None:
                return False
            name = texts[0]
            count = 0 if name in {"ERROR_NO_POP", "DISPATCH", "DISPATCH_SAME_OPARG"} else 1
            if len(arguments) != count or any(expression(arg, variables) != Kind.SCALAR
                                              for arg in arguments):
                return False
            if name in {"ERROR_IF", "ERROR_NO_POP"}:
                errors.add(id(tokens[0]))
            elif name in {"DISPATCH", "DISPATCH_SAME_OPARG"}:
                terminals.add(id(tokens[0]))
            return True
        declaration = 0
        kind = None
        if texts[:2] == ["PyObject", "*"]:
            declaration, kind = 2, Kind.OBJECT
        elif texts[:1] in (["PyValue"], ["int"]):
            declaration, kind = 1, Kind.VALUE if texts[0] == "PyValue" else Kind.SCALAR
        if declaration:
            if (len(tokens) < declaration + 3 or tokens[declaration].kind != "IDENTIFIER"
                    or texts[declaration] in variables or texts[declaration] in reserved
                    or texts[declaration + 1] != "="
                    or expression(tokens[declaration + 2:], variables) != kind):
                return False
            variables[texts[declaration]] = kind
            return True
        if len(tokens) >= 3 and texts[1] == "=" and texts[0] in variables:
            return (texts[0] != "oparg"
                    and expression(tokens[2:], variables) == variables[texts[0]])
        return expression(tokens, variables) == Kind.VOID

    for part in inst.parts:
        if isinstance(part, Skip):
            continue
        if (not isinstance(part, Uop) or part.caches
                or any(item.is_array() for item in (*part.stack.inputs, *part.stack.outputs))
                or any(token.kind.startswith("CMACRO") for token in part.body.tokens())):
            return None
        names = [item.name for item in (*part.stack.inputs, *part.stack.outputs)]
        if any(name in reserved for name in names):
            return None
        variables = {name: Kind.VALUE for name in names}
        variables["oparg"] = Kind.SCALAR
        if not statement(part.body, variables):
            return None
    used = [HELPERS[name] for name in helpers if name in HELPERS]
    homes = (HomeEffect.NO_ARENA_WRITE if all(h.homes == HomeEffect.NO_ARENA_WRITE for h in used)
             else HomeEffect.UNKNOWN)
    return SourceErrorEffects(frozenset(errors), frozenset(unwinds), frozenset(terminals),
                              frozenset(helpers), homes, any(h.may_fail for h in used),
                              any(h.callback for h in used))
