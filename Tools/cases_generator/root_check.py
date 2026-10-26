"""Check that no handler holds a value only in a C local across an escaping call.

The collector does not scan the eval loop's own C frame (_PyEvalActivation
in Include/internal/pycore_tstate.h) when a collection is entered from the
eval breaker or from an escaping call: everything a handler holds there must
be in a frame slot, which the mark visits precisely.  The stack items of a
uop are in slots across an escaping call (the IR generator checks that the
ones without a slot of their own are not live there).  This module checks
the other C locals that hold objects: across an escaping call, each one
that is used afterwards must hold a value that something the collector sees
holds too, an "anchored" value: a stack item, a constant of the code, an
attribute of the frame, a static object, or a borrowed reference out of an
anchored value.  A value that the call makes or returns, or that the old
reference counting code took a new reference to, is not anchored.
"""

from dataclasses import dataclass, field
import re

import lexer
from parser import Stmt, SimpleStmt, BlockStmt, IfStmt, WhileStmt, ForStmt, MacroIfStmt


# Types whose pointers, or values, hold objects
OBJECT_TYPE = re.compile(r"^(PyObject|Py\w*Object|_PyStackRef)$")
NOT_OBJECT_TYPES = {"PyDictKeysObject"}

# Calls whose result is anchored when their first argument is: a borrowed
# reference to the argument, part of it, or a view of it
BORROWING = {
    "PyStackRef_AsPyObjectBorrow",
    "PyStackRef_FromPyObject",
    "Py_TYPE",
    "PyStackRef_TYPE",
    "_PyObject_CAST",
    "_PyType_CAST",
    "_PyFunction_CAST",
    "PyList_GET_ITEM",
    "PyTuple_GET_ITEM",
    "_PyList_ITEMS",
    "_PyTuple_ITEMS",
    "PyCell_GET",
    "PyFunction_GET_CODE",
    "PyFunction_GET_GLOBALS",
    "PyFunction_GET_BUILTINS",
    "PyMethod_GET_FUNCTION",
    "PyMethod_GET_SELF",
    "_PyType_GetDict",
    "_PyObject_InlineValues",
    "_PyObject_GetManagedDict",
    "PyExceptionInstance_Class",
}

# Escaping calls that only allocate: a collection there is one that an
# allocation runs at once, which scans the eval loop's frame too
ALLOCATING = {
    "PyType_GenericAlloc",
}

# Calls and macros whose result is anchored whatever their arguments are:
# the code's constants and names, the frame's attributes, static objects
ANCHORED_CALLS = {
    "GETITEM",
    "_PyFrame_GetCode",
    "_PyFrame_GetFunction",
    "_PyFrame_GetGlobals",
    "_PyFrame_GetBuiltins",
    "_Py_ID",
    "_Py_STR",
    "_Py_LATIN1_CHR",
    "_PyLong_GetZero",
    "_PyLong_GetOne",
}

# Releases, which release nothing: an argument of one is not a use
RELEASES = {
    "Py_DECREF", "Py_XDECREF", "Py_CLEAR", "PyStackRef_CLEAR",
    "_Py_DECREF_SPECIALIZED", "DEAD",
}

# Macros that leave the handler
LEAVING = {
    "ERROR_NO_POP", "JUMP_TO_LABEL", "DISPATCH", "DISPATCH_INLINED",
    "DISPATCH_SAME_OPARG", "Py_UNREACHABLE", "GOTO_TIER_TWO", "GOTO_TIER_ONE",
}

# `F(x, v)` assigns v to x
ASSIGNING = {"Py_SETREF", "Py_XSETREF"}

ANCHORED_NAMES = {
    "NULL", "frame", "tstate", "Py_None", "Py_True", "Py_False",
    "Py_NotImplemented", "Py_Ellipsis",
    "PyStackRef_NULL", "PyStackRef_None", "PyStackRef_True",
    "PyStackRef_False", "PyStackRef_ERROR",
    "FRAME_CO_CONSTS", "FRAME_CO_NAMES",
}


@dataclass
class Event:
    "A statement, or the condition or header of one, in textual order"
    tokens: list[lexer.Token]
    stmt: SimpleStmt | None
    loops: tuple[int, ...]
    # The last event that control reaches from this one before it leaves
    # the handler, if a branch that holds this one ends by leaving
    limit: int = 1 << 30
    # The branches of if statements that hold this one: (if, 0 or 1)
    branches: tuple[tuple[int, int], ...] = ()
    # Variables that are NULL here, as the conditions of those say
    null_vars: frozenset[str] = frozenset()


@dataclass
class Var:
    name: str
    array: bool
    decl: int
    defs: list[tuple[int, list[lexer.Token]]] = field(default_factory=list)
    uses: list[int] = field(default_factory=list)
    addressed: list[int] = field(default_factory=list)


def leaves(stmt: Stmt) -> bool:
    "Whether stmt ends by leaving the handler: an error, a jump or a return"
    if isinstance(stmt, BlockStmt):
        return bool(stmt.body) and leaves(stmt.body[-1])
    if not isinstance(stmt, SimpleStmt):
        return False
    texts = [t.text for t in stmt.contents if t.kind != "COMMENT"]
    if not texts:
        return False
    if texts[0] in LEAVING or texts[0] in ("return", "goto"):
        return True
    return texts[:4] == ["ERROR_IF", "(", "true", ")"]


def null_tests(cond: list[lexer.Token]) -> tuple[frozenset[str], frozenset[str]]:
    """The variable that each branch of `if (cond)` knows to be NULL:
    `x == NULL` and `!x` for the first, `x != NULL` and `x` for the second"""
    texts = [t.text for t in cond if t.kind != "COMMENT"]
    if texts[:1] == ["("] and texts[-1:] == [")"]:
        texts = texts[1:-1]
    if len(texts) == 3 and texts[1] == "==" and texts[2] == "NULL":
        return frozenset({texts[0]}), frozenset()
    if len(texts) == 2 and texts[0] == "!":
        return frozenset({texts[1]}), frozenset()
    if len(texts) == 3 and texts[1] == "!=" and texts[2] == "NULL":
        return frozenset(), frozenset({texts[0]})
    if len(texts) == 1:
        return frozenset(), frozenset({texts[0]})
    if len(texts) == 4 and texts[0] == "PyStackRef_IsNull" and texts[1] == "(":
        return frozenset({texts[2]}), frozenset()
    return frozenset(), frozenset()


def linearize(body: Stmt) -> list[Event]:
    events: list[Event] = []
    loop_ids = [0]

    if_ids = [0]

    def branch(stmt: Stmt, loops: tuple[int, ...], which: tuple[int, int],
               null: frozenset[str]) -> None:
        start = len(events)
        walk(stmt, loops)
        end = len(events) - 1
        for ev in events[start:]:
            ev.branches = (which,) + ev.branches
            ev.null_vars = ev.null_vars | null
            if leaves(stmt):
                ev.limit = min(ev.limit, end)

    def walk(stmt: Stmt, loops: tuple[int, ...]) -> None:
        if isinstance(stmt, SimpleStmt):
            events.append(Event(stmt.contents, stmt, loops))
        elif isinstance(stmt, BlockStmt):
            for s in stmt.body:
                walk(s, loops)
        elif isinstance(stmt, IfStmt):
            events.append(Event(stmt.condition, None, loops))
            if_ids[0] += 1
            n = if_ids[0]
            null_then, null_else = null_tests(stmt.condition)
            branch(stmt.body, loops, (n, 0), null_then)
            if stmt.else_body is not None:
                branch(stmt.else_body, loops, (n, 1), null_else)
        elif isinstance(stmt, (WhileStmt, ForStmt)):
            loop_ids[0] += 1
            inner = loops + (loop_ids[0],)
            head = stmt.condition if isinstance(stmt, WhileStmt) else stmt.header
            events.append(Event(head, None, inner))
            walk(stmt.body, inner)
        elif isinstance(stmt, MacroIfStmt):
            for s in stmt.body:
                walk(s, loops)
            if stmt.else_body is not None:
                for s in stmt.else_body:
                    walk(s, loops)
        else:
            raise NotImplementedError(type(stmt))

    walk(body, ())
    return events


def find_declarations(events: list[Event], stack_names: set[str],
                      held: set[str]) -> dict[str, Var]:
    """The C locals that hold objects.  held gets the pointers into the
    slots and the C stack refs (_PyCStackRef), which the collector visits."""
    found: dict[str, Var] = {}
    for i, ev in enumerate(events):
        tkns = [t for t in ev.tokens if t.kind != "COMMENT"]
        for j, t in enumerate(tkns):
            if (t.kind == "IDENTIFIER" and t.text == "_PyCStackRef"
                    and j + 1 < len(tkns) and tkns[j + 1].kind == "IDENTIFIER"):
                held.add(tkns[j + 1].text)
                continue
            if t.kind != "IDENTIFIER" or not OBJECT_TYPE.match(t.text):
                continue
            if t.text in NOT_OBJECT_TYPES:
                continue
            # A declaration starts the statement, or follows "const" or a
            # "(" of a for header
            if j > 0 and tkns[j - 1].text not in ("const", "(", "{"):
                continue
            k = j + 1
            stars = 0
            while k < len(tkns) and tkns[k].text == "*":
                stars += 1
                k += 1
            if k >= len(tkns) or tkns[k].kind != "IDENTIFIER":
                continue
            name = tkns[k].text
            nxt = tkns[k + 1].text if k + 1 < len(tkns) else ""
            if nxt not in ("=", ";", "[", ","):
                continue
            is_ref = t.text == "_PyStackRef"
            # A pointer to stack refs points into slots; a pointer to object
            # pointers points into an object or an argument array
            if is_ref and stars > 0:
                held.add(name)
                continue
            if stars > 1:
                continue
            if not is_ref and stars == 0:
                continue
            if name in stack_names:
                continue
            found[name] = Var(name, nxt == "[", i)
    return found


def rhs_after(tkns: list[lexer.Token], j: int) -> list[lexer.Token]:
    "The tokens after the '=' at j, up to the end of the expression"
    depth = 0
    out = []
    for t in tkns[j + 1:]:
        if t.kind in ("LPAREN", "LBRACKET", "LBRACE"):
            depth += 1
        elif t.kind in ("RPAREN", "RBRACKET", "RBRACE"):
            if depth == 0:
                break
            depth -= 1
        elif t.kind in ("SEMI",) or (t.kind == "COMMA" and depth == 0):
            break
        out.append(t)
    return out


def collect_uses(events: list[Event], variables: dict[str, Var]) -> None:
    for i, ev in enumerate(events):
        tkns = [t for t in ev.tokens if t.kind != "COMMENT"]
        for j, t in enumerate(tkns):
            if t.kind != "IDENTIFIER" or t.text not in variables:
                continue
            var = variables[t.text]
            prev = tkns[j - 1].text if j > 0 else ""
            if prev in (".", "->"):
                continue
            nxt = tkns[j + 1].text if j + 1 < len(tkns) else ""
            before = tkns[j - 2].text if j > 1 else ""
            if prev == "(" and before in RELEASES:
                continue
            if prev == "(" and before in ASSIGNING and nxt == ",":
                var.defs.append((i, rhs_after(tkns, j + 1)))
                continue
            if prev == "&":
                var.addressed.append(i)
                var.uses.append(i)
            elif nxt == "=":
                var.defs.append((i, rhs_after(tkns, j + 1)))
            elif nxt == "[" and var.array:
                # An element store, `a[i] = x;`, defines the array too
                close = j + 1
                depth = 0
                for k in range(j + 1, len(tkns)):
                    if tkns[k].text == "[":
                        depth += 1
                    elif tkns[k].text == "]":
                        depth -= 1
                        if depth == 0:
                            close = k
                            break
                if close + 1 < len(tkns) and tkns[close + 1].text == "=":
                    var.defs.append((i, rhs_after(tkns, close + 1)))
                else:
                    var.uses.append(i)
            else:
                var.uses.append(i)


class Checker:

    def __init__(self, events: list[Event], variables: dict[str, Var],
                 stack_names: set[str], held: set[str]):
        self.events = events
        self.variables = variables
        self.stack_names = stack_names
        self.held = held

    def anchored_expr(self, tkns: list[lexer.Token], before: int,
                      seen: frozenset[str]) -> bool:
        tkns = [t for t in tkns if t.kind != "COMMENT"]
        # Strip casts and parentheses
        while tkns:
            if tkns[0].text == "(" and self.matching(tkns, 0) == len(tkns) - 1:
                tkns = tkns[1:-1]
                continue
            if tkns[0].text == "(":
                close = self.matching(tkns, 0)
                inner = tkns[1:close]
                if inner and all(t.kind == "IDENTIFIER" or t.text == "*" or t.text == "const"
                                 for t in inner) and inner[-1].text == "*":
                    tkns = tkns[close + 1:]
                    continue
            break
        if not tkns:
            return False
        if len(tkns) == 1:
            t = tkns[0]
            if t.kind == "NUMBER" or t.text in ANCHORED_NAMES:
                return True
            if t.text in self.stack_names or t.text in self.held:
                return True
            if t.text in self.variables and t.text not in seen:
                return self.anchored_var(self.variables[t.text], before,
                                         seen | {t.text})
            return False
        # &_PyFoo_Type, &_Py_FalseStruct and other statics
        if tkns[0].text == "&" and len(tkns) == 2 and tkns[1].text.startswith("_Py"):
            return True
        # frame->field
        if tkns[0].text == "frame" and tkns[1].text == "->":
            return True
        # The reference that a C stack ref holds
        if (len(tkns) == 3 and tkns[0].text in self.held
                and tkns[1].text in (".", "->") and tkns[2].text == "ref"):
            return True
        # cond ? a : b
        for k, t in enumerate(tkns):
            if t.text == "?":
                depth = 0
                for m in range(k + 1, len(tkns)):
                    if tkns[m].text in ("(", "["):
                        depth += 1
                    elif tkns[m].text in (")", "]"):
                        depth -= 1
                    elif tkns[m].text == ":" and depth == 0:
                        return (self.anchored_expr(tkns[k + 1:m], before, seen)
                                and self.anchored_expr(tkns[m + 1:], before, seen))
        # f(args) and f(args)->field / [i]: anchored by f's kind
        if tkns[0].kind == "IDENTIFIER" and len(tkns) > 1 and tkns[1].text == "(":
            close = self.matching(tkns, 1)
            if (close < len(tkns) - 1 and tkns[close + 1].text == "->"
                    and tkns[0].text in ANCHORED_CALLS):
                # A field of the code object or of the frame's function
                return True
            if close == len(tkns) - 1:
                fname = tkns[0].text
                if fname in ANCHORED_CALLS:
                    return True
                if fname in BORROWING:
                    first = self.first_arg(tkns[2:close])
                    return self.anchored_expr(first, before, seen)
                return False
        # a[i], a stack item array or an anchored array
        if tkns[0].kind == "IDENTIFIER" and tkns[1].text == "[":
            close = self.matching(tkns, 1)
            if close == len(tkns) - 1:
                return self.anchored_expr(tkns[:1], before, seen)
        # *p, a stack ref pointer into the slots
        if tkns[0].text == "*" and len(tkns) == 2:
            return True
        return False

    @staticmethod
    def matching(tkns: list[lexer.Token], i: int) -> int:
        opening = tkns[i].text
        closing = {"(": ")", "[": "]", "{": "}"}[opening]
        depth = 0
        for k in range(i, len(tkns)):
            if tkns[k].text == opening:
                depth += 1
            elif tkns[k].text == closing:
                depth -= 1
                if depth == 0:
                    return k
        return len(tkns)

    @staticmethod
    def first_arg(tkns: list[lexer.Token]) -> list[lexer.Token]:
        depth = 0
        for k, t in enumerate(tkns):
            if t.text in ("(", "["):
                depth += 1
            elif t.text in (")", "]"):
                depth -= 1
            elif t.text == "," and depth == 0:
                return tkns[:k]
        return tkns

    def anchored_var(self, var: Var, before: int, seen: frozenset[str]) -> bool:
        defs = [(i, rhs) for i, rhs in var.defs
                if i < before or self.carried(var, i, before)]
        if not defs:
            # Declared without a value and not yet assigned: nothing live
            return True
        if var.array:
            # An array initializer, `{a, b}`, or element stores
            return all(self.anchored_list(rhs, i, seen) for i, rhs in defs)
        return all(self.anchored_expr(rhs, i, seen) for i, rhs in defs)

    def anchored_list(self, rhs: list[lexer.Token], before: int,
                      seen: frozenset[str]) -> bool:
        if rhs and rhs[0].text == "{":
            items: list[list[lexer.Token]] = [[]]
            depth = 0
            for t in rhs[1:-1]:
                if t.text in ("(", "["):
                    depth += 1
                elif t.text in (")", "]"):
                    depth -= 1
                if t.text == "," and depth == 0:
                    items.append([])
                else:
                    items[-1].append(t)
            return all(self.anchored_expr(item, before, seen) for item in items if item)
        return self.anchored_expr(rhs, before, seen)

    def same_loop(self, i: int, j: int) -> bool:
        a, b = self.events[i].loops, self.events[j].loops
        return bool(set(a) & set(b))

    def exclusive(self, i: int, s: int) -> bool:
        "Whether the events at i and s are on different branches of an if"
        mine = dict(self.events[i].branches)
        return any(n in mine and mine[n] != side
                   for n, side in self.events[s].branches)

    def carried(self, var: Var, i: int, s: int) -> bool:
        """Whether the use or definition at i may see, or be seen by, the
        statement at s in another iteration: they share a loop that var is
        declared outside of"""
        shared = set(self.events[i].loops) & set(self.events[s].loops)
        return bool(shared - set(self.events[var.decl].loops))

    def live_across(self, var: Var, s: int) -> bool:
        if s in var.addressed:
            return True
        if var.array and s in var.uses:
            return True
        if var.decl > s and not self.carried(var, var.decl, s):
            return False
        if not var.defs and not var.addressed:
            return False
        # The statement assigns the variable: its old value is dead
        if any(i == s for i, _ in var.defs):
            return False
        if var.name in self.events[s].null_vars:
            return False
        limit = self.events[s].limit
        later = [i for i in var.uses
                 if ((s < i <= limit) or (i != s and self.carried(var, i, s)))
                 and not self.exclusive(i, s)]
        if not later:
            return False
        # Assigned again before every later use, in straight-line code
        first_use = min(i for i in later if i > s) if any(i > s for i in later) else None
        redefs = [i for i, _ in var.defs
                  if s < i <= limit and not self.carried(var, i, s)]
        if (first_use is not None and redefs and min(redefs) < first_use
                and all(i > s for i in later)):
            return False
        return True

    def check(self, escaping: dict[SimpleStmt, object]) -> list[tuple[lexer.Token, str]]:
        errors = []
        for s, ev in enumerate(self.events):
            if ev.stmt is None or ev.stmt not in escaping:
                continue
            call = escaping[ev.stmt].call  # type: ignore[attr-defined]
            if call.text in ALLOCATING:
                continue
            for var in self.variables.values():
                if not self.live_across(var, s):
                    continue
                if self.anchored_var(var, s, frozenset({var.name})):
                    continue
                errors.append((call, f"'{var.name}' is held only in a C local "
                               f"across the escaping call to '{call.text}'"))
        return errors


def stack_item_names(uop: object) -> set[str]:
    stack = getattr(uop, "stack", None)
    if stack is None:
        return set()
    return {item.name for item in stack.inputs + stack.outputs}


def check_uop(uop: object) -> list[tuple[lexer.Token, str]]:
    body = uop.body  # type: ignore[attr-defined]
    escaping = uop.properties.escaping_calls  # type: ignore[attr-defined]
    if not escaping:
        return []
    events = linearize(body)
    names = stack_item_names(uop)
    held: set[str] = set()
    variables = find_declarations(events, names, held)
    if not variables:
        return []
    collect_uses(events, variables)
    return Checker(events, variables, names, held).check(escaping)
