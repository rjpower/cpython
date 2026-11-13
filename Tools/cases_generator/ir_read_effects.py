"""Positive NULL-safe uses of an instruction's declared nullable inputs.

The qualifier alone grants no read capability. This AST walk follows value
copies and the typed NULL-safe interfaces; an opaque projection of a possibly
NULL value fails closed. Index/object protocol checks remain independent.
"""

from analyzer import Uop, analysis_error
from generators_common import TokenIterator, instruction_macro_args
from lexer import Token
from parser import BlockStmt, IfStmt, MacroIfStmt, SimpleStmt


ASSIGNMENTS = {"EQUALS", "TIMESEQUAL", "DIVEQUAL", "MODEQUAL", "PLUSEQUAL",
               "MINUSEQUAL", "LSHIFTEQUAL", "RSHIFTEQUAL", "ANDEQUAL",
               "OREQUAL", "XOREQUAL", "PLUSPLUS", "MINUSMINUS"}


# These interfaces inspect tags, move canonical values or explicitly accept
# NULL. They do not grant home-address or arbitrary callback capabilities.
_PROPAGATE = {"TAKE", "OWNER", "PyValue_Borrow", "PyValue_XDUP",
              "PyValue_AsKnownObjectBorrow"}
_READERS = {"PyValue_IsNull", "PyValue_IsNone", "PyValue_IsObject",
            "PyValue_IsInline", "PyValue_IsInlineInt", "PyValue_IsInlineFloat",
            "PyValue_IsTaggedIndex", "_PyValue_CheckHome", "PyValue_XCLOSE", "DEAD",
            "IR_CLOSE_INPUTS"}
_NULLABLE_ARGUMENTS = {"_PyFrame_WriteHome": {2}, "_PyValue_SetAttr": {2},
                       "value_operation_close3": {0, 1, 2},
                       "_Py_Specialize_ForIter": {1}}
# These macros write their scalar argument without spelling an assignment
# at the call site. Ordinary by-value C calls cannot perform that write.
_VALUE_LVALUE_CALLS = {"PyValue_CLEAR", "PyValue_XSETREF"}
_VALUE_END_CALLS = {"PyValue_CLOSE", "PyValue_XCLOSE", "PyValue_CLOSE_SPECIALIZED",
                    "DEAD", "TAKE", "OWNER"}


def strip_value_expression(tokens):
    tokens = [token for token in tokens if token.kind != "COMMENT"]
    while len(tokens) > 1 and tokens[0].text == "(" and tokens[-1].text == ")":
        depth = 0
        for at, token in enumerate(tokens):
            depth += token.text == "("
            depth -= token.text == ")"
            if depth == 0:
                break
        if at != len(tokens) - 1:
            break
        tokens = tokens[1:-1]
    return tokens


def positive_value_expression(tokens, positive):
    """A current non-NULL C value, without assuming anything from its name."""
    tokens = strip_value_expression(tokens)
    if len(tokens) == 1:
        return tokens[0].text in positive | {"PyValue_None", "PyValue_True", "PyValue_False"}
    depth = 0
    question = None
    nested = 0
    for at, token in enumerate(tokens):
        depth += token.text in {"(", "["}
        depth -= token.text in {")", "]"}
        if depth:
            continue
        if token.text == "?":
            if question is None:
                question = at
            else:
                nested += 1
        elif token.text == ":" and question is not None:
            if nested:
                nested -= 1
                continue
            condition = tokens[:question]
            # The condition must not invalidate an arm's input fact.
            if value_nonnull_changes(condition, positive, positive):
                return False
            return (positive_value_expression(tokens[question + 1:at], positive)
                    and positive_value_expression(tokens[at + 1:], positive))
    # OWNER may yield NULL for an input borrowed from a named binding; TAKE
    # acquires a real owner there. NULL-safe transport is a weaker contract.
    positive_calls = (_PROPAGATE - {"OWNER"}) | {"PyValue_DUP"}
    if (len(tokens) >= 3 and tokens[0].text in positive_calls
            and tokens[1].text == "("):
        args, end = instruction_macro_args(tokens[0], TokenIterator(tokens[1:]))
        return (end is tokens[-1] and len(args) == 1 and
                positive_value_expression(args[0], positive))
    return False


def value_transport_expression(tokens):
    """A complete scalar transport, without inventing a native owner.

    The caller resolves the source to its current Local identity. A plain
    copy and a borrow preserve that identity; only the actual TAKE/DUP
    lowering can establish ownership of the resulting C value.
    """
    tokens = strip_value_expression(tokens)
    if len(tokens) == 1:
        name = tokens[0].text
        if name in {"PyValue_NULL", "PyValue_None", "PyValue_True", "PyValue_False"}:
            return "literal", name
        if tokens[0].kind == "IDENTIFIER":
            return "copy", name
    if len(tokens) < 4 or tokens[1].text != "(":
        return None
    modes = {"TAKE": "take", "PyValue_DUP": "dup", "PyValue_XDUP": "dup",
             "PyValue_Borrow": "borrow"}
    mode = modes.get(tokens[0].text)
    if mode is None:
        return None
    args, end = instruction_macro_args(tokens[0], TokenIterator(tokens[1:]))
    if end is not tokens[-1] or len(args) != 1:
        return None
    source = strip_value_expression(args[0])
    if len(source) != 1 or source[0].kind != "IDENTIFIER":
        return None
    return mode, source[0].text


def value_declaration(tokens):
    tokens = [token for token in tokens if token.kind != "COMMENT"]
    if tokens and tokens[-1].text == ";":
        tokens = tokens[:-1]
    if (tokens and tokens[0].text in {"const", "volatile", "register", "static", "auto", "_Atomic"}
            and any(token.text in {"PyValue", "PyValueBorrow"} for token in tokens)):
        raise analysis_error("qualified current-value declaration needs an explicit scope contract", tokens[0])
    if (len(tokens) < 2 or tokens[0].text not in {"PyValue", "PyValueBorrow"}
            or tokens[1].kind != "IDENTIFIER"):
        return None
    if len(tokens) == 2 or len(tokens) > 3 and tokens[2].text == "=":
        return tokens[1].text
    raise analysis_error("current-value proof requires a single scalar declaration", tokens[1])


def value_assignment(tokens):
    """The complete scalar assignment, shared by both provenance users."""
    tokens = [token for token in tokens if token.kind != "COMMENT"]
    if tokens and tokens[-1].text == ";":
        tokens = tokens[:-1]
    equals = [at for at, token in enumerate(tokens) if token.kind in ASSIGNMENTS]
    if len(equals) != 1 or tokens[equals[0]].text != "=":
        return None
    at = equals[0]
    lhs = strip_value_expression(tokens[:at])
    if len(lhs) == 1 and lhs[0].kind == "IDENTIFIER":
        return lhs[0].text, False, tokens[at + 1:]
    if (len(lhs) == 2 and lhs[0].text in {"PyValue", "PyValueBorrow"}
            and lhs[1].kind == "IDENTIFIER"):
        return lhs[1].text, True, tokens[at + 1:]
    return None


def error_nonnull_value(tokens):
    """ERROR_IF's surviving path, for the exact canonical NULL predicate."""
    tokens = [token for token in tokens if token.kind != "COMMENT"]
    if tokens and tokens[-1].text == ";":
        tokens = tokens[:-1]
    if len(tokens) < 3 or tokens[0].text != "ERROR_IF" or tokens[1].text != "(":
        return None
    args, end = instruction_macro_args(tokens[0], TokenIterator(tokens[1:]))
    if len(args) != 1 or end is not tokens[-1]:
        return None
    condition = strip_value_expression(args[0])
    if (len(condition) == 4 and condition[0].text == "PyValue_IsNull"
            and condition[1].text == "(" and condition[2].kind == "IDENTIFIER"
            and condition[3].text == ")"):
        return condition[2].text
    return None


def value_nonnull_changes(tokens, positive, tracked, fresh=frozenset()):
    """Effects on current scalar facts; unknown writes and aliases revoke them.

    Storage applies these changes to Local identities. The nullable-use walk
    uses the same expression proof, so transport cannot acquire a stronger
    meaning merely by crossing a uop boundary.
    """
    tokens = [token for token in tokens if token.kind != "COMMENT"]
    if tokens and tokens[-1].text == ";":
        tokens = tokens[:-1]
    changes = {}
    if tokens and tokens[0].text in {"PyValue", "PyValueBorrow"}:
        for token in tokens[1:]:
            if token.text in tracked:
                changes[token.text] = False
    if any(token.kind == "CMACRO_OTHER" for token in tokens):
        return dict.fromkeys(tracked, False)
    for at, token in enumerate(tokens):
        if (token.text in _VALUE_LVALUE_CALLS | _VALUE_END_CALLS | {"value_operation_close3"}
                and at + 1 < len(tokens) and tokens[at + 1].text == "("):
            args, _ = instruction_macro_args(token, TokenIterator(tokens[at + 1:]))
            affected = args if token.text == "value_operation_close3" else args[:1]
            for arg in affected:
                for target in arg:
                    if target.text in tracked:
                        changes[target.text] = False
        if token.text == "&":
            after = at + 1
            while after < len(tokens) and tokens[after].text == "(":
                after += 1
            if after < len(tokens) and tokens[after].text in tracked:
                changes[tokens[after].text] = False
        if token.text in {"++", "--"}:
            for neighbour in tokens[max(0, at - 1):at + 2]:
                if neighbour.text in tracked:
                    changes[neighbour.text] = False
        if token.kind in ASSIGNMENTS:
            for lhs in tokens[:at]:
                if lhs.text in tracked:
                    changes[lhs.text] = False
    assignment = value_assignment(tokens)
    if assignment is not None:
        name, declaration, rhs = assignment
        if name in tracked:
            # Only a new scoped identity may gain a declaration's fact.
            changes[name] = ((not declaration or name in fresh) and
                             positive_value_expression(rhs, positive))
    return changes


def validate_nullable_inputs(part: Uop) -> None:
    declared = {item.name for item in part.stack.inputs if item.nullable}
    # These names are typed interfaces, never local function-pointer values.
    # Reject shadow declarations and references before consulting their call
    # contracts, even when the declaration itself has no nullable operand.
    helpers = _PROPAGATE | _READERS | _NULLABLE_ARGUMENTS.keys() | {
        "IR_TAKE_INPUT", "with_exit_call"} | _VALUE_LVALUE_CALLS | _VALUE_END_CALLS
    body_tokens = list(part.body.tokens())
    for at, token in enumerate(body_tokens):
        if token.kind == "CMACRO_OTHER":
            raise analysis_error("nullable uses cannot redefine typed interfaces", token)
        if token.text not in helpers:
            continue
        if (at + 1 == len(body_tokens) or body_tokens[at + 1].text != "(" or
                (at and body_tokens[at - 1].text in {
                    "*", "&", "PyValue", "PyValueBorrow", "PyObject", "int", "void"})):
            raise analysis_error("nullable helper cannot be shadowed or aliased", token)
    if not declared:
        return
    aliases = {item.name: frozenset({item.name}) if item.nullable else frozenset()
               for item in part.stack.inputs if item.name != "unused"}
    values = {item.name for item in (*part.stack.inputs, *part.stack.outputs)}

    def reject(token: Token) -> None:
        raise analysis_error("nullable input needs a checked NULL-safe use", token)

    strip = strip_value_expression

    def expression(tokens, state, nonnull):
        tokens = strip(tokens)
        if not tokens:
            return frozenset()
        if (any(token.text in {"=", "*=", "/=", "%=", "+=", "-=", "<<=", ">>=",
                               "&=", "|=", "^=", "++", "--"} for token in tokens)
                and any(token.text in state for token in tokens)):
            reject(tokens[0])
        for at, token in enumerate(tokens):
            if token.text not in state:
                continue
            before = tokens[at - 1].text if at else None
            after = tokens[at + 1].text if at + 1 < len(tokens) else None
            if before in {"++", "--"} or after in {"++", "--", ".", "->", "["}:
                reject(token)
        # Passing the address can mutate the value despite a preceding NULL
        # test. Do not turn a read qualifier into a writable alias capability.
        for at, token in enumerate(tokens):
            if token.text != "&":
                continue
            after = at + 1
            while after < len(tokens) and tokens[after].text == "(":
                after += 1
            if after < len(tokens) and tokens[after].text in state:
                reject(token)
        if len(tokens) == 1:
            name = tokens[0].text
            if name == "PyValue_NULL":
                return frozenset({"<NULL>"})
            return state.get(name, frozenset()) if name not in nonnull else frozenset()
        if tokens[0].kind == "IDENTIFIER" and tokens[1].text == "(":
            iterator = TokenIterator(tokens[1:])
            args, end = instruction_macro_args(tokens[0], iterator)
            if end is tokens[-1]:
                origins = [expression(arg, state, nonnull) for arg in args]
                call = tokens[0].text
                if call == "IR_TAKE_INPUT" and len(origins) == 2:
                    return origins[0]
                if call in _PROPAGATE and len(origins) == 1:
                    return origins[0]
                if call in _READERS and len(origins) == 1:
                    return frozenset()
                allowed = _NULLABLE_ARGUMENTS.get(call, set())
                if call == "with_exit_call" and len(args) == 6:
                    optional = strip(args[1])
                    if ([token.text for token in optional[:2]] ==
                            ["PyValue_AsKnownObjectBorrow", "("]
                            and optional[-1].text == ")"):
                        optional = strip(optional[2:-1])
                    if len(optional) == 1 and optional[0].kind == "IDENTIFIER":
                        name = optional[0].text
                        condition = [token.text for token in args[2]]
                        if condition == ["!", "PyValue_IsNull", "(", name, ")"]:
                            allowed = {1}
                if any(origin and at not in allowed for at, origin in enumerate(origins)):
                    reject(tokens[0])
                return frozenset()
        # A NULL test may participate in an ordinary scalar condition. An
        # unguarded value/address/member operand cannot participate in one.
        for at, token in enumerate(tokens):
            if token.text in state and token.text not in nonnull:
                # Recurse over a complete nested call instead of accepting
                # its token merely because a safe helper occurs nearby.
                if at >= 2 and tokens[at - 1].text == "(" and tokens[at - 2].text in _READERS:
                    if at + 1 < len(tokens) and tokens[at + 1].text == ")":
                        continue
                reject(token)
        return frozenset()

    def nonnull_branch(tokens):
        texts = [token.text for token in strip(tokens)]
        if len(texts) == 4 and texts[:2] == ["PyValue_IsNull", "("] and texts[-1] == ")":
            return set(), {texts[2]}
        if len(texts) == 5 and texts[:3] == ["!", "PyValue_IsNull", "("] and texts[-1] == ")":
            return {texts[3]}, set()
        return set(), set()

    def positive_expression(tokens, state, nonnull):
        positive = nonnull | {name for name, origins in state.items() if not origins}
        return positive_value_expression(tokens, positive)

    def walk(stmt, state, nonnull):
        if isinstance(stmt, BlockStmt):
            for child in stmt.body:
                walk(child, state, nonnull)
            return
        if isinstance(stmt, MacroIfStmt):
            if any(name in stmt.condition.text for name in declared):
                reject(stmt.condition)
            left, right = state.copy(), state.copy()
            for child in stmt.body:
                walk(child, left, nonnull.copy())
            for child in stmt.else_body or []:
                walk(child, right, nonnull.copy())
            for name in left.keys() | right.keys():
                state[name] = left.get(name, frozenset()) | right.get(name, frozenset())
            tokens = list(stmt.tokens())
            written = [token.text for at, token in enumerate(tokens[:-1])
                       if tokens[at + 1].text == "="]
            nonnull.difference_update(written)
            return
        if isinstance(stmt, IfStmt):
            expression(stmt.condition, state, nonnull)
            yes, no = nonnull_branch(stmt.condition)
            left, right = state.copy(), state.copy()
            walk(stmt.body, left, nonnull | yes)
            if stmt.else_body is not None:
                walk(stmt.else_body, right, nonnull | no)
            for name in left.keys() | right.keys():
                state[name] = left.get(name, frozenset()) | right.get(name, frozenset())
            tokens = list(stmt.tokens())
            nonnull.difference_update(token.text for at, token in enumerate(tokens[:-1])
                                     if tokens[at + 1].text == "=")
            return
        if not isinstance(stmt, SimpleStmt):
            if any(token.text in state for token in stmt.tokens()):
                reject(next(stmt.tokens()))
            return
        tokens = [token for token in stmt.contents if token.kind != "COMMENT"]
        if tokens and tokens[-1].text == ";":
            tokens = tokens[:-1]
        if [token.text for token in tokens[:3]] == ["(", "void", ")"]:
            expression(tokens[3:], state, nonnull)
            return
        equals = [at for at, token in enumerate(tokens) if token.text == "="]
        if len(equals) == 1 and equals[0] > 0:
            at = equals[0]
            origin = expression(tokens[at + 1:], state, nonnull)
            lhs = tokens[at - 1].text
            prefix = [token.text for token in tokens[:at - 1]]
            if not prefix or prefix in (["PyValue"], ["PyValueBorrow"], ["PyObject", "*"]):
                if prefix and lhs in state:
                    reject(tokens[at - 1])
                # Replacing a nullable value with an opaque result does not
                # establish non-NULL. Only a required input or a literal
                # canonical non-NULL value carries that fact here.
                positive = positive_expression(tokens[at + 1:], state, nonnull)
                if (lhs in state or lhs in values) and not origin and not positive:
                    origin = frozenset({"<nullable replacement>"})
                if origin or lhs in state or lhs in values:
                    state[lhs] = origin
                    nonnull.discard(lhs)
                return
            if origin:
                reject(tokens[0])
            if any(token.text in state for token in tokens[:at]):
                reject(tokens[0])
            return
        expression(tokens, state, nonnull)
        guarded = error_nonnull_value(tokens)
        if guarded in state:
            state[guarded] = frozenset()

    walk(part.body, aliases, set())
