"""Positive NULL-safe uses of an instruction's declared nullable inputs.

The qualifier alone grants no read capability. This AST walk follows value
copies and the typed NULL-safe interfaces; an opaque projection of a possibly
NULL value fails closed. Index/object protocol checks remain independent.
"""

import ir_contract
from analyzer import analysis_error
from generators_common import TokenIterator, instruction_macro_args


ASSIGNMENTS = {"EQUALS", "TIMESEQUAL", "DIVEQUAL", "MODEQUAL", "PLUSEQUAL",
               "MINUSEQUAL", "LSHIFTEQUAL", "RSHIFTEQUAL", "ANDEQUAL",
               "OREQUAL", "XOREQUAL", "PLUSPLUS", "MINUSMINUS"}


# What a call does to a value comes from its row (ir_components.h), not from
# its name. The four questions below are the only ones this module asks.


def _row(name):
    return ir_contract.rows().get(name)


def value_mode(name):
    """How a call moves a scalar: take, dup, borrow, or None.

    TAKE moves its argument's value and owner (`moved(0)`), NULL where it is; XDUP
    is NULL exactly where its argument is, and DUP is XDUP once the argument
    is known to be non-NULL; an INTRINSIC view of a borrowed argument is the
    argument itself.
    """
    row = _row(name)
    if row is None or len(row.args) != 1:
        return None
    if (row.kind == "MACRO" and row.takes_view and row.result.moved == 0
            and row.result.null_iff == 0):
        return "take"
    if row.kind == "INTRINSIC" and row.result.shape == "VIEW" and row.args[0].mode == "borrowed":
        return "borrow"
    form = _row(row.nonnull_form) or row
    if (form.result.shape == "VALUE" and form.result.mode == "owned"
            and form.result.null_iff == 0):
        return "dup"
    return None


def ends_value(name):
    """Whether the call's first argument stops being a value of the caller.

    DEAD says so of an input in the declaration language; a row says it of
    a consumed or taken argument.
    """
    if name == "DEAD":
        return True
    row = _row(name)
    return row is not None and bool(row.args) and row.args[0].mode in ("consumed", "taken")


def written_arguments(name):
    """The positions of the arguments that a call assigns, without spelling
    `=`: a macro's `lvalue` (PyValue_CLEAR(x) sets x) and `out` arguments.
    A function takes values and cannot."""
    row = _row(name)
    if row is None:
        return ()
    return tuple(at for at, arg in enumerate(row.args) if arg.mode in ("lvalue", "out"))


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
    # A call that moves a value keeps it non-NULL. OWNER is not one: its row
    # says it may be NULL (the owner of a local's view).
    if (len(tokens) >= 3 and value_mode(tokens[0].text) is not None
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
    mode = value_mode(tokens[0].text)
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
    return null_value_condition(args[0])


def null_value_condition(tokens):
    """An exact NULL predicate on one current scalar C identity."""
    condition = strip_value_expression(tokens)
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
        if (at + 1 < len(tokens) and tokens[at + 1].text == "("
                and (written_arguments(token.text) or ends_value(token.text))):
            args, _ = instruction_macro_args(token, TokenIterator(tokens[at + 1:]))
            affected = [args[k] for k in written_arguments(token.text) if k < len(args)]
            if ends_value(token.text):
                affected += args[:1]
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
