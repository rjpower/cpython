"""root_check's C mode: precise C functions keep their objects anchored.

Note 11 (boil/design/11-precise-roots.md) section 2.1.  A function marked
``Py_GC_PRECISE`` runs in a C frame that the collector never scans, so at
every call that may collect, every object the function still holds must be
anchored somewhere the collector does see: in a frame slot, in a pushed
value span, in a traced field of an anchored object, or in a static.  This
module checks that over a supported subset of C:

- it parses each function's unexpanded source (macros are names, classified
  by the tables below) into statements and expressions, and builds the
  control-flow graph, gotos and cleanup labels included;
- a forward analysis gives each object local the set of sources its value
  may have come from, and the anchors that hold it now;
- a backward analysis gives liveness;
- at each collecting call, every object local or temporary that is live
  there must be anchored on every path reaching it.

Calls are of three kinds: non-collecting (neither allocates an object nor
runs Python), collecting but not reentrant (allocates, runs no Python), and
reentrant (the default).  A borrow through a field anchors across a
non-reentrant call if the field is strong, and across a reentrant call only
if it is immutable.

Usage: root_check_c.py [--function NAME] [--all] FILE...
Checks the functions marked Py_GC_PRECISE in each FILE (or the named one, or
every function with --all) and prints one line per error; exit status 1 if
there are errors.
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable, Iterator

import call_effects
import lexer


# --- tables ------------------------------------------------------------------

# Calls whose result is a borrow out of their first argument: the source is
# field(arg, FIELD).
BORROW_FIELD = {
    "PyTuple_GET_VALUE": "ob_item",
    "_PyTuple_ITEMS": "ob_item",
    "PyList_GET_VALUE": "ob_item",
    "_PyList_ITEMS": "ob_item",
    "PyCell_GET_VALUE": "ob_ref",
    "PyFunction_GET_CODE": "func_code",
    "PyFunction_GET_GLOBALS": "func_globals",
    "PyFunction_GET_BUILTINS": "func_builtins",
    "PyMethod_GET_FUNCTION": "im_func",
    "PyMethod_GET_SELF": "im_self",
    "Py_TYPE": "ob_type",
    "PyValue_Type": "ob_type",
    "_PyFrame_GetCode": "f_executable",
    "_PyFrame_GetFunction": "f_funcobj",
    "_PyFrame_GetGlobals": "f_globals",
    "_PyFrame_GetBuiltins": "f_builtins",
}

# Calls whose result is their first argument, or a view of it.
IDENTITY = {
    "PyValue_ObjectBorrow", "PyValue_FromObject", "_PyObject_CAST",
    "_PyType_CAST", "_PyVarObject_CAST", "_Py_CAST", "_PyFunction_CAST",
    "_PyTuple_CAST", "_PyList_CAST", "Py_NewRef", "Py_XNewRef",
}

# Calls whose result is a static or an interned constant.
CONSTANT_CALLS = {"_Py_ID", "_Py_STR", "_Py_LATIN1_CHR", "_PyLong_GetZero",
                  "_PyLong_GetOne"}

CONSTANT_NAMES = {
    "NULL", "Py_None", "Py_True", "Py_False", "Py_NotImplemented",
    "Py_Ellipsis", "PyValue_NULL", "_PyValue_None", "_PyValue_True",
    "_PyValue_False", "_PyValue_ERROR",
}

# Fields: immutable strong edges anchor across any call; strong traced
# fields across a non-reentrant call only; every other field, weak ones
# such as wr_object included, never anchors.
IMMUTABLE_FIELDS = {
    "ob_item@tuple", "im_func", "im_self", "co_consts", "co_names",
    "co_localsplusnames", "co_filename", "co_name", "co_qualname",
}
STRONG_FIELDS = {
    "ob_item", "ob_type", "f_executable", "f_funcobj", "f_globals",
    "f_builtins", "f_locals", "func_code", "func_globals", "func_builtins",
    "func_name", "func_qualname", "func_defaults", "func_kwdefaults",
    "func_closure", "func_module", "func_dict", "tp_mro", "tp_dict",
    "tp_bases", "tp_base", "ob_ref", "current_exception", "dict_entries",
}

# Functions that store into a value out-parameter, with the field its value
# is borrowed from (in the first argument).  PyDict_Next() and
# _PyDict_Next() are not here: they box inline keys and values into their
# object out-parameters, and a box is fresh.
OUT_PARAMS = {"_PyDict_NextValue": "dict_entries"}

# Calls that fill an array (argument index dst) with the canonical values of
# the elements of an array they are given (argument index src), when the
# arguments at the null indices are NULL.  When src is an array the
# function received, the copies are anchored as the originals are, by the
# received array's creator (10a-2.md, rule 3): an object's canonical value
# is the object, and a box's is an immediate.  _PyArg_UnpackKeywords()
# with no kwargs dict copies the positional and keyword values of the
# vectorcall array into buf (Argument Clinic's argsbuf).
COPIES_RECEIVED = {
    "values_of": (0, 2, ()),
    "_PyArg_UnpackKeywords": (0, 9, (2,)),
}

# Calls that write memory they are given: a slot or field anchor through
# it ends.
MUTATING = {"memset", "memcpy", "memmove", "PyTuple_SET_ITEM",
            "PyList_SET_ITEM", "PyTuple_SET_VALUE", "PyList_SET_VALUE",
            "PyCell_SET", "_PyValue_FromObjectArray"}

# A span is filled only by this call, which stores at the end of the
# prefix and raises its length in one step.
SPAN_APPEND = "_PyValueSpan_Append"
# ... or by this one, which stores an object's own pointer, so that it
# anchors the object (a box included).
SPAN_APPEND_OBJECT = "_PyValueSpan_AppendObject"
# Calls whose result is the canonical value of an object: an immediate for
# a box, so storing it anchors the value and not the object.
CANONICAL = {"PyValue_FromObject"}

# Functions that may return while their thread stays detached: never
# precise, so that the detach unwind (note 11 section 3.3) always stops at
# a frame that persists.
DETACH_HELPERS = {
    "PyEval_SaveThread", "PyEval_ReleaseThread", "_PyThreadState_Detach",
    "detach_thread", "_PyThreadState_Swap", "PyThreadState_Swap",
    "_PyEval_ReleaseLock", "PyEval_ReleaseLock", "tstate_deactivate",
    "_PyThreadState_Attach", "PyEval_RestoreThread", "PyEval_AcquireThread",
    "_PyGC_CaptureCaller",
}

RELEASES = {"Py_DECREF", "Py_XDECREF", "DEAD"}
ASSIGN_MACROS = {"Py_SETREF", "Py_XSETREF"}
CLEAR_MACROS = {"Py_CLEAR"}

SPAN_PUSH = {"_PyThreadState_PushValueRoot", "_PyThreadState_PushValueSpan"}
SPAN_POP = {"_PyThreadState_PopValueRoot"}
# Push and pop the root only if the value is inline, and box it there: the
# pair is a scope, which the root anchors.
SPAN_PUSH_OBJECT = {"_PyValueRoot_PushObject": 1, "_PyValue_ObjectRooted": 2}
SPAN_POP_OBJECT = {"_PyValue_UnrootObject": 2}

# A pushed frame's slots past its arguments hold garbage until
# _PyFrame_Initialize() sets them to NULL, so nothing between the two may
# collect (10a-1b.md item 1).
FRAME_PUSH = {"_PyThreadState_PushFrame", "_PyThreadState_PushFrameUnchecked"}
FRAME_INIT = "_PyFrame_Initialize"

# Statement macros without a semicolon of their own, and their meaning.
DETACH_MACROS = {"Py_BEGIN_ALLOW_THREADS", "Py_BLOCK_THREADS"}
ATTACH_MACROS = {"Py_END_ALLOW_THREADS", "Py_UNBLOCK_THREADS"}
RETURN_MACROS = {"Py_RETURN_NONE", "Py_RETURN_TRUE", "Py_RETURN_FALSE",
                 "Py_RETURN_NOTIMPLEMENTED"}

# Declarations that mark functions and parameters.
PRECISE = "Py_GC_PRECISE"
ARGS_ANCHORED = "Py_GC_ARGS_ANCHORED"
# The first argument position passed on the stack: x86-64 SysV passes six
# integer-class arguments in registers (AArch64 passes eight; counting from
# seven there checks a superset).  Variadic arguments count the same way.
FIRST_STACK_ARG = 7
SLOTS = "Py_GC_SLOTS"

C_KEYWORDS = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if",
    "inline", "int", "long", "register", "restrict", "return", "short",
    "signed", "sizeof", "static", "struct", "switch", "typedef", "union",
    "unsigned", "void", "volatile", "while", "_Bool", "__inline__",
    "_Alignof", "_Static_assert", "__attribute__", "__extension__",
}
TYPE_WORDS = {
    "char", "double", "float", "int", "long", "short", "signed", "unsigned",
    "void", "_Bool", "bool", "size_t", "ssize_t", "Py_ssize_t", "Py_hash_t",
    "uint8_t", "uint16_t", "uint32_t", "uint64_t", "int8_t", "int16_t",
    "int32_t", "int64_t", "uintptr_t", "intptr_t", "PyValue", "PyObject",
    "vectorcallfunc", "ternaryfunc", "binaryfunc", "unaryfunc",
    "PyCFunction", "getter", "setter", "descrgetfunc", "PyThreadState",
    "PyInterpreterState", "PyTime_t",
}
QUALIFIERS = {"const", "volatile", "static", "register", "restrict",
              "extern", "inline", "_Py_ALIGNED_DEF", PRECISE, ARGS_ANCHORED,
              SLOTS, "Py_LOCAL_INLINE", "_Py_HOT_FUNCTION", "Py_NO_INLINE",
              "DONT_SLP_VECTORIZE", "PyAPI_FUNC"}

OBJECT_TYPE = re.compile(r"^(PyObject|Py\w*Object|PyValue)$")
NOT_OBJECT_TYPES = {"PyDictKeysObject"}


def looks_like_type(name: str) -> bool:
    return (name in TYPE_WORDS or name.endswith("_t")
            or name.endswith("func") or bool(OBJECT_TYPE.match(name))
            or name in {"struct", "union", "enum"}
            or re.match(r"^_?Py[A-Z]\w*$", name) is not None
            and name[-1] != "_" and not re.search(r"_[A-Z]", name[3:])
            and "_" not in name.lstrip("_"))


# --- tokens ------------------------------------------------------------------

@dataclass
class Tok:
    text: str
    kind: str      # "id", "num", "str", "op", "pp"
    line: int

    def __repr__(self) -> str:
        return self.text


class CheckError(Exception):
    pass


def prepare(src: str) -> str:
    """Splice continuation lines (in directives, strings and code alike),
    keeping line numbers, and replace character literals, which the shared
    lexer does not escape."""
    out = []
    lines = src.split("\n")
    i = 0
    while i < len(lines):
        joined = lines[i]
        extra = 0
        while joined.endswith("\\") and i + 1 < len(lines):
            i += 1
            extra += 1
            # A directive's continuation is a space; elsewhere, as in a
            # string, the splice joins the lines directly.
            sep = " " if joined.lstrip().startswith("#") else ""
            joined = joined[:-1] + sep + lines[i]
        out.append(joined)
        out.extend([""] * extra)
        i += 1
    return blank_char_literals("\n".join(out))


def blank_char_literals(text: str) -> str:
    """Replace each character literal in code (not in a string or a
    comment) by a 0 padded to its length, and blank characters that are not
    C tokens."""
    out: list[str] = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == '"':
            j = i + 1
            while j < n and text[j] != '"' and text[j] != "\n":
                j += 2 if text[j] == "\\" else 1
            out.append(text[i:j + 1])
            i = j + 1
        elif text.startswith("//", i):
            j = text.find("\n", i)
            j = n if j < 0 else j
            out.append(text[i:j])
            i = j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            out.append(text[i:j])
            i = j
        elif c == "'":
            j = i + 1
            while j < n and text[j] != "'" and text[j] != "\n":
                j += 2 if text[j] == "\\" else 1
            out.append("0" + " " * (j - i))
            i = j + 1
        else:
            # Characters outside C's token set, as in an EM_JS body's
            # JavaScript, are blanked for the shared lexer.
            out.append(" " if c in "`$@" else c)
            i += 1
    return "".join(out)


def tokens_of(src: str, line: int = 1) -> list[Tok]:
    toks: list[Tok] = []
    for t in lexer.tokenize(prepare(src), line=line):
        if t.kind in ("COMMENT", "\n"):
            continue
        if t.kind.startswith("CMACRO"):
            toks.append(Tok(t.text.strip(), "pp", t.line))
        elif t.kind == "NUMBER" or t.kind == "CHARACTER":
            toks.append(Tok(t.text, "num", t.line))
        elif t.kind == "STRING":
            toks.append(Tok(t.text, "str", t.line))
        elif re.match(r"[A-Za-z_]", t.text):
            toks.append(Tok(t.text, "id", t.line))
        else:
            toks.append(Tok(t.text, "op", t.line))
    # Adjacent string literals are one
    merged: list[Tok] = []
    for t in toks:
        if t.kind == "str" and merged and merged[-1].kind == "str":
            continue
        merged.append(t)
    return merged


# --- expressions -------------------------------------------------------------

@dataclass(eq=False)
class Expr:
    line: int


@dataclass(eq=False)
class Name(Expr):
    id: str


@dataclass(eq=False)
class Const(Expr):
    text: str


@dataclass(eq=False)
class Call(Expr):
    func: Expr
    args: list[Expr]

    @property
    def name(self) -> str | None:
        return self.func.id if isinstance(self.func, Name) else None


@dataclass(eq=False)
class Member(Expr):
    base: Expr
    field: str
    arrow: bool


@dataclass(eq=False)
class Index(Expr):
    base: Expr
    index: Expr


@dataclass(eq=False)
class Unary(Expr):
    op: str
    operand: Expr


@dataclass(eq=False)
class Cast(Expr):
    type: list[str]
    operand: Expr


@dataclass(eq=False)
class Binary(Expr):
    op: str
    left: Expr
    right: Expr


@dataclass(eq=False)
class Assign(Expr):
    op: str
    target: Expr
    value: Expr


@dataclass(eq=False)
class Cond(Expr):
    test: Expr
    then: Expr
    other: Expr


@dataclass(eq=False)
class Comma(Expr):
    items: list[Expr]


@dataclass(eq=False)
class InitList(Expr):
    items: list[Expr]


def children(e: Expr) -> list[Expr]:
    if isinstance(e, Call):
        return [e.func, *e.args]
    if isinstance(e, Member):
        return [e.base]
    if isinstance(e, Index):
        return [e.base, e.index]
    if isinstance(e, (Unary, Cast)):
        return [e.operand]
    if isinstance(e, Binary):
        return [e.left, e.right]
    if isinstance(e, Assign):
        return [e.target, e.value]
    if isinstance(e, Cond):
        return [e.test, e.then, e.other]
    if isinstance(e, (Comma, InitList)):
        return list(e.items)
    return []


def walk_expr(e: Expr) -> Iterator[Expr]:
    yield e
    for c in children(e):
        yield from walk_expr(c)


BINARY_PREC = {
    "||": 1, "&&": 2, "|": 3, "^": 4, "&": 5, "==": 6, "!=": 6,
    "<": 7, ">": 7, "<=": 7, ">=": 7, "<<": 8, ">>": 8, "+": 9, "-": 9,
    "*": 10, "/": 10, "%": 10,
}
ASSIGN_OPS = {"=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=",
              ">>="}


class ExprParser:
    def __init__(self, toks: list[Tok], typedefs: set[str]):
        self.toks = toks
        self.pos = 0
        self.typedefs = typedefs

    def peek(self, k: int = 0) -> Tok | None:
        p = self.pos + k
        return self.toks[p] if p < len(self.toks) else None

    def next(self) -> Tok:
        t = self.peek()
        if t is None:
            raise CheckError("unexpected end of expression")
        self.pos += 1
        return t

    def expect(self, text: str) -> Tok:
        t = self.next()
        if t.text != text:
            raise CheckError(f"line {t.line}: expected '{text}', found '{t.text}'")
        return t

    def at(self, text: str) -> bool:
        t = self.peek()
        return t is not None and t.text == text

    def is_type_name(self, name: str) -> bool:
        return name in self.typedefs or looks_like_type(name) or name in (
            "const", "volatile", "unsigned", "signed", "struct", "union",
            "enum", "long", "short", "char", "int", "void", "double", "float")

    def cast_ahead(self) -> int:
        """If a parenthesized type name starts here, the index of its ')'."""
        t = self.peek(1)
        if t is None or t.kind != "id" or not self.is_type_name(t.text):
            return -1
        k = 1
        while True:
            t = self.peek(k)
            if t is None:
                return -1
            if t.text == ")":
                break
            if t.kind != "id" and t.text not in ("*",):
                return -1
            k += 1
        inner = [self.peek(j).text for j in range(1, k)]  # type: ignore[union-attr]
        # (name) alone is a cast only if name is clearly a type
        if len(inner) == 1 and not (inner[0] in self.typedefs
                                     or inner[0] in TYPE_WORDS
                                     or inner[0].endswith("_t")
                                     or inner[0].endswith("func")
                                     or OBJECT_TYPE.match(inner[0])):
            return -1
        after = self.peek(k + 1)
        if after is None or after.text in (")", ";", ",", "]", "}", "?", ":",
                                           "=", "==", "!=", "||", "&&", "/",
                                           "%", "<", ">", "<=", ">=", "|",
                                           "^", ".", "->", "[") \
                or after.text in ASSIGN_OPS:
            return -1
        return k

    def parse(self) -> Expr:
        e = self.comma()
        return e

    def comma(self) -> Expr:
        first = self.assign()
        if not self.at(","):
            return first
        items = [first]
        while self.at(","):
            self.next()
            items.append(self.assign())
        return Comma(first.line, items)

    def assign(self) -> Expr:
        left = self.cond()
        t = self.peek()
        if t is not None and t.text in ASSIGN_OPS:
            self.next()
            right = self.assign()
            return Assign(t.line, t.text, left, right)
        return left

    def cond(self) -> Expr:
        test = self.binary(1)
        if self.at("?"):
            t = self.next()
            then = self.comma()
            self.expect(":")
            other = self.cond()
            return Cond(t.line, test, then, other)
        return test

    def binary(self, prec: int) -> Expr:
        left = self.unary()
        while True:
            t = self.peek()
            if t is None or t.text not in BINARY_PREC or BINARY_PREC[t.text] < prec:
                return left
            self.next()
            right = self.binary(BINARY_PREC[t.text] + 1)
            left = Binary(t.line, t.text, left, right)

    def unary(self) -> Expr:
        t = self.peek()
        if t is None:
            raise CheckError("unexpected end of expression")
        if t.text in ("&", "*", "!", "~", "-", "+", "++", "--"):
            self.next()
            return Unary(t.line, t.text, self.unary())
        if t.text == "sizeof" or t.text == "_Alignof" or t.text == "offsetof":
            self.next()
            if self.at("("):
                self.skip_parens()
            else:
                self.unary()
            return Const(t.line, "sizeof")
        if t.text == "(" :
            k = self.cast_ahead()
            if k > 0:
                type_toks = [self.peek(j).text for j in range(1, k)]  # type: ignore[union-attr]
                self.pos += k + 1
                if self.at("{"):
                    return self.postfix(self.init_list())
                return Cast(t.line, type_toks, self.unary())
        return self.postfix(self.primary())

    def skip_parens(self) -> None:
        depth = 0
        while True:
            t = self.next()
            if t.text == "(":
                depth += 1
            elif t.text == ")":
                depth -= 1
                if depth == 0:
                    return

    def init_list(self) -> Expr:
        t = self.expect("{")
        items: list[Expr] = []
        while not self.at("}"):
            if self.at("."):
                # designated initializer: .name = value
                self.next()
                self.next()
                self.expect("=")
            elif self.at("["):
                self.next()
                self.comma()
                self.expect("]")
                self.expect("=")
            if self.at("{"):
                items.append(self.init_list())
            else:
                items.append(self.assign())
            if self.at(","):
                self.next()
        self.expect("}")
        return InitList(t.line, items)

    def primary(self) -> Expr:
        t = self.next()
        if t.kind == "id":
            return Name(t.line, t.text)
        if t.kind in ("num", "str"):
            return Const(t.line, t.text)
        if t.text == "(":
            if self.at("{"):
                raise CheckError(f"line {t.line}: statement expression")
            e = self.comma()
            self.expect(")")
            return e
        if t.text == "{":
            self.pos -= 1
            return self.init_list()
        raise CheckError(f"line {t.line}: unexpected '{t.text}' in expression")

    def postfix(self, e: Expr) -> Expr:
        while True:
            t = self.peek()
            if t is None:
                return e
            if t.text == "(":
                self.next()
                args: list[Expr] = []
                while not self.at(")"):
                    if self.is_type_arg():
                        while not (self.at(",") or self.at(")")):
                            self.next()
                        args.append(Const(t.line, "type"))
                    else:
                        args.append(self.assign())
                    if self.at(","):
                        self.next()
                self.expect(")")
                e = Call(t.line, e, args)
            elif t.text == "[":
                self.next()
                idx = self.comma()
                self.expect("]")
                e = Index(t.line, e, idx)
            elif t.text in (".", "->"):
                self.next()
                name = self.next()
                e = Member(t.line, e, name.text, t.text == "->")
            elif t.text in ("++", "--"):
                self.next()
                e = Unary(t.line, "post" + t.text, e)
            else:
                return e

    def is_type_arg(self) -> bool:
        """A macro argument that is a type, such as Py_ARRAY_LENGTH's or a
        cast macro's: a type word followed by '*'s up to ',' or ')'."""
        t = self.peek()
        if t is None or t.kind != "id" or not self.is_type_name(t.text):
            return False
        k = 0
        while True:
            u = self.peek(k)
            if u is None:
                return False
            if u.text in (",", ")"):
                break
            if u.kind != "id" and u.text != "*":
                return False
            k += 1
        texts = [self.peek(j).text for j in range(k)]  # type: ignore[union-attr]
        return "*" in texts or len(texts) > 1 and all(
            x in ("const", "struct", "unsigned", "signed", "long", "short",
                  "int", "char") or x == texts[-1] for x in texts)


def parse_expr(toks: list[Tok], typedefs: set[str]) -> Expr:
    p = ExprParser(toks, typedefs)
    e = p.parse()
    if p.pos != len(toks):
        t = toks[p.pos]
        raise CheckError(f"line {t.line}: unexpected '{t.text}' after expression")
    return e


# --- statements --------------------------------------------------------------

@dataclass(eq=False)
class Stmt:
    line: int


@dataclass(eq=False)
class Block(Stmt):
    body: list[Stmt]


@dataclass(eq=False)
class Decl(Stmt):
    # (name, kind, initializer): kind is "object", "slots" (a PyValue *),
    # "span" (a _PyValueRoot), "array" (an array of objects) or "other"
    vars: list[tuple[str, str, Expr | None]]


@dataclass(eq=False)
class ExprStmt(Stmt):
    expr: Expr


@dataclass(eq=False)
class If(Stmt):
    test: Expr
    then: Stmt
    other: Stmt | None


@dataclass(eq=False)
class While(Stmt):
    test: Expr
    body: Stmt


@dataclass(eq=False)
class DoWhile(Stmt):
    body: Stmt
    test: Expr


@dataclass(eq=False)
class For(Stmt):
    init: Stmt | None
    test: Expr | None
    step: Expr | None
    body: Stmt


@dataclass(eq=False)
class Switch(Stmt):
    test: Expr
    body: Stmt


@dataclass(eq=False)
class Case(Stmt):
    default: bool


@dataclass(eq=False)
class Label(Stmt):
    name: str


@dataclass(eq=False)
class Goto(Stmt):
    label: str


@dataclass(eq=False)
class Return(Stmt):
    value: Expr | None


@dataclass(eq=False)
class Break(Stmt):
    pass


@dataclass(eq=False)
class Continue(Stmt):
    pass


@dataclass(eq=False)
class MacroIf(Stmt):
    branches: list[list[Stmt]]


@dataclass(eq=False)
class Detach(Stmt):
    attach: bool


def var_kind(type_toks: list[str], stars: int, array: bool) -> str:
    base = [t for t in type_toks if t not in QUALIFIERS]
    if not base:
        return "other"
    name = base[-1]
    if name == "_PyValueRoot":
        return "span" if stars == 0 else "other"
    if name in NOT_OBJECT_TYPES:
        return "pointer" if stars >= 1 else "other"
    if name == "PyValue":
        if stars == 0:
            return "array" if array else "object"
        if stars == 1:
            return "slots" if SLOTS in type_toks else "values"
        return "pointer"
    if OBJECT_TYPE.match(name):
        if stars == 1:
            return "array" if array else "object"
        return "pointer"
    return "pointer" if stars >= 1 else "other"


class StmtParser:
    def __init__(self, toks: list[Tok], typedefs: set[str]):
        self.toks = toks
        self.pos = 0
        self.typedefs = typedefs

    def peek(self, k: int = 0) -> Tok | None:
        p = self.pos + k
        return self.toks[p] if p < len(self.toks) else None

    def next(self) -> Tok:
        t = self.peek()
        if t is None:
            raise CheckError("unexpected end of function")
        self.pos += 1
        return t

    def expect(self, text: str) -> Tok:
        t = self.next()
        if t.text != text:
            raise CheckError(f"line {t.line}: expected '{text}', found '{t.text}'")
        return t

    def at(self, text: str) -> bool:
        t = self.peek()
        return t is not None and t.text == text

    def until(self, stop: str) -> list[Tok]:
        """Tokens up to the stop token at depth 0, which is consumed."""
        out: list[Tok] = []
        depth = 0
        while True:
            t = self.next()
            if t.text in ("(", "[", "{"):
                depth += 1
            elif t.text in (")", "]", "}"):
                depth -= 1
            if t.text == stop and depth <= 0 and (stop not in ")]}" or depth < 0):
                return out
            out.append(t)

    def paren(self) -> Expr:
        self.expect("(")
        toks = self.until(")")
        return parse_expr(toks, self.typedefs)

    def block(self) -> Block:
        t = self.expect("{")
        body: list[Stmt] = []
        while not self.at("}"):
            body.extend(self.stmt_or_pp())
        self.expect("}")
        return Block(t.line, body)

    def stmt_or_pp(self) -> list[Stmt]:
        t = self.peek()
        assert t is not None
        if t.kind == "pp":
            return [self.macro_if()] if re.match(r"#\s*if", t.text) else self.skip_pp()
        return [self.stmt()]

    def skip_pp(self) -> list[Stmt]:
        t = self.next()
        if re.match(r"#\s*include", t.text):
            raise CheckError(f"line {t.line}: #include in a function body "
                             "(the eval loop's generated region is not "
                             "checked here)")
        return []

    def macro_if(self) -> MacroIf:
        t = self.next()
        branches: list[list[Stmt]] = [[]]
        while True:
            u = self.peek()
            if u is None:
                raise CheckError(f"line {t.line}: unterminated #if")
            if u.kind == "pp":
                if re.match(r"#\s*(else|elif)", u.text):
                    self.next()
                    branches.append([])
                    continue
                if re.match(r"#\s*endif", u.text):
                    self.next()
                    break
            if self.at("}"):
                raise CheckError(f"line {u.line}: #if that crosses a block")
            branches[-1].extend(self.stmt_or_pp())
        if len(branches) == 1:
            branches.append([])
        return MacroIf(t.line, branches)

    def stmt(self) -> Stmt:
        t = self.peek()
        assert t is not None
        line = t.line
        x = t.text
        if x == "{":
            return self.block()
        if x == ";":
            self.next()
            return Block(line, [])
        if x == "static":
            # A static local is in the data segment, not the frame, and its
            # initializer is a constant (Argument Clinic's _kwtuple and
            # _parser): nothing for the frame's analysis
            self.until(";")
            return Block(line, [])
        if x == "if":
            self.next()
            test = self.paren()
            then = self.stmt()
            other = None
            if self.at("else"):
                self.next()
                other = self.stmt()
            return If(line, test, then, other)
        if x == "while":
            self.next()
            test = self.paren()
            return While(line, test, self.stmt())
        if x == "do":
            self.next()
            body = self.stmt()
            self.expect("while")
            test = self.paren()
            self.expect(";")
            return DoWhile(line, body, test)
        if x == "for":
            self.next()
            self.expect("(")
            init: Stmt | None = None
            if not self.at(";"):
                init = self.simple(self.until(";"), line)
            else:
                self.next()
            test_toks = self.until(";")
            step_toks = self.until(")")
            test = parse_expr(test_toks, self.typedefs) if test_toks else None
            step = parse_expr(step_toks, self.typedefs) if step_toks else None
            return For(line, init, test, step, self.stmt())
        if x == "switch":
            self.next()
            test = self.paren()
            return Switch(line, test, self.stmt())
        if x == "case":
            self.next()
            self.until(":")
            return Case(line, False)
        if x == "default" and self.peek(1) and self.peek(1).text == ":":  # type: ignore[union-attr]
            self.next()
            self.next()
            return Case(line, True)
        if x == "return":
            self.next()
            toks = self.until(";")
            return Return(line, parse_expr(toks, self.typedefs) if toks else None)
        if x == "break":
            self.next()
            self.expect(";")
            return Break(line)
        if x == "continue":
            self.next()
            self.expect(";")
            return Continue(line)
        if x == "goto":
            self.next()
            target = self.next()
            if target.text == "*":
                raise CheckError(f"line {line}: computed goto")
            self.expect(";")
            return Goto(line, target.text)
        if t.kind == "id" and self.peek(1) is not None and self.peek(1).text == ":" \
                and x not in C_KEYWORDS:  # type: ignore[union-attr]
            self.next()
            self.next()
            return Label(line, x)
        if x in ("asm", "__asm__", "__asm"):
            raise CheckError(f"line {line}: inline assembly")
        if x in DETACH_MACROS or x in ATTACH_MACROS:
            self.next()
            if self.at(";"):
                self.next()
            return Detach(line, x in ATTACH_MACROS)
        if x in RETURN_MACROS:
            self.next()
            if self.at(";"):
                self.next()
            return Return(line, Name(line, "Py_None"))
        # Statement macros: a diagnostic pragma, with or without its
        # semicolon, and a loop macro, IDENT(...) {...}, whose test is the
        # macro's call (classified like any call).
        if re.match(r"_Py_COMP_DIAG_\w+$", x):
            self.next()
            if self.at(";"):
                self.next()
            return Block(line, [])
        if t.kind == "id" and x not in C_KEYWORDS and self.at_loop_macro():
            self.next()
            self.expect("(")
            args = self.until(")")
            test = parse_expr([Tok(x, "id", line), Tok("(", "op", line), *args,
                               Tok(")", "op", line)], self.typedefs)
            return While(line, test, self.stmt())
        toks = self.until(";")
        return self.simple(toks, line)

    def at_loop_macro(self) -> bool:
        if not (self.peek(1) and self.peek(1).text == "("):  # type: ignore[union-attr]
            return False
        end = matching(self.toks, self.pos + 1)
        return end + 1 < len(self.toks) and self.toks[end + 1].text == "{"

    def simple(self, toks: list[Tok], line: int) -> Stmt:
        decl = self.declaration(toks, line)
        if decl is not None:
            return decl
        if not toks:
            return Block(line, [])
        return ExprStmt(line, parse_expr(toks, self.typedefs))

    def declaration(self, toks: list[Tok], line: int) -> Decl | None:
        i = 0
        type_toks: list[str] = []
        while i < len(toks) and toks[i].text in QUALIFIERS:
            type_toks.append(toks[i].text)
            i += 1
        if i >= len(toks) or toks[i].kind != "id":
            return None
        if toks[i].text in ("struct", "union", "enum"):
            type_toks.append(toks[i].text)
            i += 1
            if i >= len(toks) or toks[i].kind != "id":
                return None
            type_toks.append(toks[i].text)
            i += 1
        else:
            # one or more type words: `unsigned long`, `Py_ssize_t`
            while i < len(toks) and toks[i].kind == "id" and (
                    toks[i].text in QUALIFIERS or toks[i].text in TYPE_WORDS
                    and i + 1 < len(toks) and toks[i + 1].kind == "id"
                    and toks[i + 1].text in TYPE_WORDS):
                type_toks.append(toks[i].text)
                i += 1
            if i >= len(toks) or toks[i].kind != "id":
                return None
            type_toks.append(toks[i].text)
            i += 1
        while i < len(toks) and toks[i].text in QUALIFIERS:
            type_toks.append(toks[i].text)
            i += 1
        # A declarator must follow: '*'s and a name, or a function pointer
        j = i
        while j < len(toks) and toks[j].text in ("*", "const", "volatile", "restrict"):
            j += 1
        if j >= len(toks):
            return None
        if toks[j].text == "(" and j + 1 < len(toks) and toks[j + 1].text == "*":
            pass  # a function pointer
        elif toks[j].kind != "id" or toks[j].text in C_KEYWORDS:
            return None
        elif j + 1 < len(toks) and toks[j + 1].text not in ("=", ",", "[", ";", ")") \
                and not (toks[j + 1].kind == "id" and toks[j + 1].text in QUALIFIERS):
            return None
        if type_toks[-1] in C_KEYWORDS and type_toks[-1] not in TYPE_WORDS \
                and type_toks[-1] not in ("struct", "union", "enum"):
            return None
        # Split the declarators at depth-0 commas
        decls: list[list[Tok]] = [[]]
        depth = 0
        for t in toks[i:]:
            if t.text in ("(", "[", "{"):
                depth += 1
            elif t.text in (")", "]", "}"):
                depth -= 1
            if t.text == "," and depth == 0:
                decls.append([])
            else:
                decls[-1].append(t)
        out: list[tuple[str, str, Expr | None]] = []
        for d in decls:
            stars = 0
            k = 0
            while k < len(d) and d[k].text in ("*", "const", "volatile", "restrict"):
                stars += d[k].text == "*"
                k += 1
            if k < len(d) and d[k].text == "(":
                # function pointer: (*name)(...)
                name = d[k + 2].text if k + 2 < len(d) else "?"
                init = None
                eq = next((m for m, t in enumerate(d) if t.text == "="), None)
                if eq is not None:
                    init = parse_expr(d[eq + 1:], self.typedefs)
                out.append((name, "other", init))
                continue
            if k >= len(d):
                return None
            name = d[k].text
            k += 1
            array = False
            while k < len(d) and d[k].text == "[":
                array = True
                depth = 0
                while k < len(d):
                    if d[k].text == "[":
                        depth += 1
                    elif d[k].text == "]":
                        depth -= 1
                        if depth == 0:
                            break
                    k += 1
                k += 1
            init = None
            if k < len(d) and d[k].text == "=":
                init = parse_expr(d[k + 1:], self.typedefs)
            elif k < len(d):
                return None
            out.append((name, var_kind(type_toks, stars, array), init))
        self.typedefs.discard(out[0][0]) if out else None
        return Decl(line, out)


# --- functions ---------------------------------------------------------------

@dataclass
class Param:
    name: str
    kind: str


@dataclass
class Function:
    name: str
    line: int
    precise: bool
    args_anchored: bool
    static: bool
    variadic: bool
    params: list[Param]
    body_toks: list[Tok]
    returns_object: bool


def split_top(toks: list[Tok], sep: str = ",") -> list[list[Tok]]:
    out: list[list[Tok]] = [[]]
    depth = 0
    for t in toks:
        if t.text in ("(", "[", "{"):
            depth += 1
        elif t.text in (")", "]", "}"):
            depth -= 1
        if t.text == sep and depth == 0:
            out.append([])
        else:
            out[-1].append(t)
    return out if out != [[]] else []


def parse_params(toks: list[Tok]) -> list[Param]:
    params = []
    for p in split_top(toks):
        texts = [t.text for t in p]
        if not texts or texts == ["void"] or texts == ["..."]:
            continue
        names = [t for t in p if t.kind == "id" and t.text not in QUALIFIERS]
        if "(" in texts:
            # a function pointer parameter
            star = texts.index("(")
            name = texts[star + 2] if star + 2 < len(texts) else "?"
            params.append(Param(name, "other"))
            continue
        if len(names) < 2:
            continue
        name = names[-1].text
        stars = texts.count("*")
        array = "[" in texts
        type_toks = [t for t in texts[:texts.index(name)] if t != "*"]
        kind = var_kind(type_toks, stars + array, False)
        params.append(Param(name, kind))
    return params


def matching(toks: list[Tok], i: int) -> int:
    open_ = toks[i].text
    close = {"(": ")", "[": "]", "{": "}"}[open_]
    depth = 0
    for j in range(i, len(toks)):
        if toks[j].text == open_:
            depth += 1
        elif toks[j].text == close:
            depth -= 1
            if depth == 0:
                return j
    raise CheckError(f"line {toks[i].line}: unbalanced '{open_}'")


def find_functions(toks: list[Tok]) -> tuple[list[Function], set[str], dict[str, str]]:
    """The function definitions at file scope; the names declared
    Py_GC_PRECISE (by a definition or a prior declaration); and the return
    kind ("object" or "other") of every function declared or defined."""
    funcs: list[Function] = []
    precise_names: set[str] = set()
    returns: dict[str, str] = {}
    i = 0
    start = 0
    n = len(toks)
    while i < n:
        t = toks[i]
        if t.kind == "pp":
            i += 1
            start = i
            continue
        # extern "C" { ... } around a header, and its stray closing brace
        if t.text == "extern" and i + 2 < n and toks[i + 1].kind == "str" \
                and toks[i + 2].text == "{":
            i += 3
            start = i
            continue
        if t.text == "}":
            i += 1
            start = i
            continue
        if t.text == ";":
            head = toks[start:i]
            note_declaration(head, precise_names, returns)
            i += 1
            start = i
            continue
        if t.text in ("(", "["):
            try:
                i = matching(toks, i) + 1
            except CheckError:
                i += 1
            continue
        if t.text == "{":
            try:
                end = matching(toks, i)
            except CheckError:
                i += 1
                continue
            head = toks[start:i]
            fn = function_of(head, toks[i + 1:end])
            if fn is not None:
                funcs.append(fn)
                returns[fn.name] = "object" if fn.returns_object else "other"
                parts = head_parts(head)
                if parts is not None and writes_values(parts[2]):
                    returns[fn.name + "()"] = "writes"
                if fn.precise:
                    precise_names.add(fn.name)
                i = end + 1
                start = i
            else:
                # a struct, union, enum or initializer: up to its ';'
                i = end + 1
            continue
        i += 1
    for fn in funcs:
        if fn.name in precise_names:
            fn.precise = True
    return funcs, precise_names, returns


def head_parts(head: list[Tok]) -> tuple[str, list[Tok], list[Tok]] | None:
    """The name, the tokens before it and the parameter tokens of a
    function head `... name(params) [attributes]`."""
    if not head:
        return None
    # The parameter list is the first top-level parenthesis that follows
    # an identifier which is not a macro of the head (PyAPI_FUNC(T)).
    k = 0
    while k < len(head):
        if head[k].text == "(":
            close = matching(head, k)
            if k > 0 and head[k - 1].kind == "id" and head[k - 1].text not in (
                    "PyAPI_FUNC", "Py_LOCAL_INLINE", "__attribute__",
                    "_Py_NO_SANITIZE_ADDRESS", "_Py_NO_SANITIZE_THREAD",
                    "_Py_NO_SANITIZE_MEMORY") \
                    and head[k - 1].text not in C_KEYWORDS:
                rest = [x.text for x in head[close + 1:]]
                if rest and rest[0] == "(":
                    return None  # a function pointer declaration
                return head[k - 1].text, head[:k - 1], head[k + 1:close]
            k = close + 1
            continue
        if head[k].text == "=":
            return None
        k += 1
    return None


def returns_object_of(before: list[Tok]) -> bool:
    texts = [t.text for t in before]
    # PyAPI_FUNC(T)
    if "PyAPI_FUNC" in texts:
        a = texts.index("PyAPI_FUNC")
        inner = texts[a + 2:texts.index(")", a)]
        texts = inner
    stars = texts.count("*")
    words = [x for x in texts if x not in QUALIFIERS and x != "*"
             and x not in ("(", ")")]
    if not words:
        return False
    return var_kind([words[-1]], stars, False) == "object"


def writes_values(params: list[Tok]) -> bool:
    """Does a parameter list take a non-const PyValue *, through which the
    function may write the caller's slots?"""
    for p in split_top(params):
        texts = [t.text for t in p]
        if "PyValue" in texts and "*" in texts and "const" not in texts:
            return True
    return False


def note_declaration(head: list[Tok], precise_names: set[str],
                     returns: dict[str, str]) -> None:
    if not head or head[0].text in ("typedef",):
        return
    parts = head_parts(head)
    if parts is None:
        return
    name, before, params = parts
    if any(t.text == PRECISE for t in head):
        precise_names.add(name)
    returns.setdefault(name, "object" if returns_object_of(before) else "other")
    if writes_values(params):
        returns.setdefault(name + "()", "writes")


def function_of(head: list[Tok], body: list[Tok]) -> Function | None:
    if not head or head[-1].text in ("=", ",") or head[0].text in (
            "struct", "union", "enum", "typedef"):
        if head and head[0].text in ("struct", "union", "enum") and any(
                t.text == "(" for t in head):
            pass
        else:
            return None
    if head[-1].text != ")" and not any(t.text == "__attribute__" for t in head):
        return None
    parts = head_parts(head)
    if parts is None:
        return None
    name, before, params = parts
    texts = [t.text for t in head]
    return Function(
        name=name, line=head[0].line if head else 0,
        precise=PRECISE in texts or ARGS_ANCHORED in texts,
        args_anchored=ARGS_ANCHORED in texts,
        static="static" in texts or "Py_LOCAL_INLINE" in texts,
        variadic=any(t.text == "..." for t in params),
        params=parse_params(params), body_toks=body,
        returns_object=returns_object_of(before))


# --- the control-flow graph --------------------------------------------------

@dataclass(eq=False)
class Node:
    """A statement, or the condition or header of one.  exprs are evaluated
    in order; decls are the (name, kind, init) the node declares."""
    line: int
    exprs: list[Expr] = field(default_factory=list)
    decls: list[tuple[str, str, Expr | None]] = field(default_factory=list)
    succ: list["Node"] = field(default_factory=list)
    pred: list["Node"] = field(default_factory=list)
    # facts on the edge to each successor: (var, is_null) or None
    edge_null: dict[int, tuple[str, bool]] = field(default_factory=dict)
    detach: bool = False
    is_return: bool = False
    is_exit: bool = False
    returns: Expr | None = None


class CFG:
    def __init__(self) -> None:
        self.nodes: list[Node] = []
        self.labels: dict[str, Node] = {}
        self.gotos: list[tuple[Node, str]] = []
        self.entry = self.new(0)
        self.exit = self.new(0)
        self.exit.is_exit = True

    def new(self, line: int) -> Node:
        n = Node(line)
        self.nodes.append(n)
        return n

    @staticmethod
    def link(a: Node, b: Node, null: tuple[str, bool] | None = None) -> None:
        a.succ.append(b)
        b.pred.append(a)
        if null is not None:
            a.edge_null[len(a.succ) - 1] = null


def null_test(e: Expr) -> tuple[str | None, bool]:
    """(var, which branch knows var is NULL: True for the then branch)."""
    if isinstance(e, Binary) and e.op in ("==", "!=") and isinstance(e.left, Name) \
            and isinstance(e.right, Name) and e.right.id in ("NULL",):
        return e.left.id, e.op == "=="
    if isinstance(e, Unary) and e.op == "!" and isinstance(e.operand, Name):
        return e.operand.id, True
    if isinstance(e, Name):
        return e.id, False
    if isinstance(e, Call) and e.name == "PyValue_IsNull" and e.args \
            and isinstance(e.args[0], Name):
        return e.args[0].id, True
    return None, False


def build_cfg(body: Block) -> CFG:
    g = CFG()
    loops: list[tuple[Node, Node]] = []      # (continue target, break target)
    switches: list[tuple[Node, list[Node]]] = []

    def seq(stmts: list[Stmt], cur: list[Node]) -> list[Node]:
        for s in stmts:
            cur = one(s, cur)
        return cur

    def one(s: Stmt, cur: list[Node]) -> list[Node]:
        if isinstance(s, Block):
            return seq(s.body, cur)
        if isinstance(s, MacroIf):
            outs: list[Node] = []
            for br in s.branches:
                outs.extend(seq(br, list(cur)))
            return outs
        if isinstance(s, (ExprStmt, Decl, Detach)):
            n = g.new(s.line)
            if isinstance(s, ExprStmt):
                n.exprs = [s.expr]
            elif isinstance(s, Decl):
                n.decls = s.vars
            else:
                n.detach = not s.attach
            for c in cur:
                g.link(c, n)
            return [n]
        if isinstance(s, If):
            n = g.new(s.line)
            n.exprs = [s.test]
            for c in cur:
                g.link(c, n)
            var, then_null = null_test(s.test)
            t_entry = g.new(s.line)
            g.link(n, t_entry, (var, then_null) if var else None)
            outs = one(s.then, [t_entry])
            e_entry = g.new(s.line)
            g.link(n, e_entry, (var, not then_null) if var else None)
            if s.other is not None:
                outs = outs + one(s.other, [e_entry])
            else:
                outs = outs + [e_entry]
            return outs
        if isinstance(s, While):
            head = g.new(s.line)
            head.exprs = [s.test]
            for c in cur:
                g.link(c, head)
            after = g.new(s.line)
            loops.append((head, after))
            body_out = one(s.body, [head])
            loops.pop()
            for c in body_out:
                g.link(c, head)
            g.link(head, after)
            return [after]
        if isinstance(s, DoWhile):
            top = g.new(s.line)
            for c in cur:
                g.link(c, top)
            test = g.new(s.line)
            test.exprs = [s.test]
            after = g.new(s.line)
            loops.append((test, after))
            body_out = one(s.body, [top])
            loops.pop()
            for c in body_out:
                g.link(c, test)
            g.link(test, top)
            g.link(test, after)
            return [after]
        if isinstance(s, For):
            if s.init is not None:
                cur = one(s.init, cur)
            head = g.new(s.line)
            if s.test is not None:
                head.exprs = [s.test]
            for c in cur:
                g.link(c, head)
            step = g.new(s.line)
            if s.step is not None:
                step.exprs = [s.step]
            after = g.new(s.line)
            loops.append((step, after))
            body_out = one(s.body, [head])
            loops.pop()
            for c in body_out:
                g.link(c, step)
            g.link(step, head)
            g.link(head, after)
            return [after]
        if isinstance(s, Switch):
            n = g.new(s.line)
            n.exprs = [s.test]
            for c in cur:
                g.link(c, n)
            after = g.new(s.line)
            cases: list[Node] = []
            switches.append((n, cases))
            loops.append((loops[-1][0] if loops else after, after))
            outs = one(s.body, [])
            loops.pop()
            switches.pop()
            has_default = any(getattr(c, "_default", False) for c in cases)
            if not has_default:
                g.link(n, after)
            for c in outs:
                g.link(c, after)
            return [after]
        if isinstance(s, Case):
            n = g.new(s.line)
            sw, cases = switches[-1]
            g.link(sw, n)
            n._default = s.default  # type: ignore[attr-defined]
            cases.append(n)
            for c in cur:
                g.link(c, n)
            return [n]
        if isinstance(s, Label):
            n = g.labels.setdefault(s.name, g.new(s.line))
            n.line = s.line
            for c in cur:
                g.link(c, n)
            return [n]
        if isinstance(s, Goto):
            n = g.new(s.line)
            for c in cur:
                g.link(c, n)
            g.gotos.append((n, s.label))
            return []
        if isinstance(s, Return):
            n = g.new(s.line)
            n.is_return = True
            n.returns = s.value
            if s.value is not None:
                n.exprs = [s.value]
            for c in cur:
                g.link(c, n)
            g.link(n, g.exit)
            return []
        if isinstance(s, (Break, Continue)) and not loops:
            raise CheckError(f"line {s.line}: break or continue outside a "
                             "loop (from a macro)")
        if isinstance(s, Break):
            for c in cur:
                g.link(c, loops[-1][1])
            return []
        if isinstance(s, Continue):
            # The innermost loop that is not a switch
            for c in cur:
                g.link(c, loops[-1][0])
            return []
        raise CheckError(f"line {s.line}: unsupported statement {type(s).__name__}")

    outs = one(body, [g.entry])
    for c in outs:
        g.link(c, g.exit)
    for n, label in g.gotos:
        if label not in g.labels:
            raise CheckError(f"line {n.line}: goto to unknown label '{label}'")
        g.link(n, g.labels[label])
    return g


# --- call classification -----------------------------------------------------

class Classifier:
    """Which calls collect, and which reenter, with the bodies of static
    inline functions consulted for names not in the tables."""

    def __init__(self, inline_bodies: dict[str, list[Tok]] | None = None,
                 extra_non_collecting: Iterable[str] = ()):
        self.inline_bodies = inline_bodies or {}
        self.extra = set(extra_non_collecting)
        self.memo: dict[str, str] = {}

    def kind(self, name: str | None) -> str:
        """"none", "alloc" or "reentrant"."""
        if name is None:
            return "reentrant"  # a call through a pointer
        if name in self.memo:
            return self.memo[name]
        listed = call_effects.kind(name)
        if listed is not None:
            k = listed
        elif name in self.extra:
            k = "none"
        elif name in self.inline_bodies:
            self.memo[name] = "reentrant"  # a cycle counts as collecting
            k = self.summarize(self.inline_bodies[name])
        else:
            k = "reentrant"
        self.memo[name] = k
        return k

    def summarize(self, body: list[Tok]) -> str:
        worst = "none"
        for i, t in enumerate(body[:-1]):
            # A call through a pointer: (*fp)(...)
            if t.text == ")" and body[i + 1].text == "(" and i >= 3 and \
                    body[i - 2].text == "*" and body[i - 3].text == "(":
                return "reentrant"
            if t.kind != "id" or body[i + 1].text != "(" or t.text in C_KEYWORDS:
                continue
            if i > 0 and body[i - 1].text in (".", "->"):
                return "reentrant"  # a call through a field
            k = self.kind(t.text)
            if k == "reentrant":
                return k
            if k == "alloc":
                worst = "alloc"
        return worst


# --- the analysis ------------------------------------------------------------

# A source is a tuple: ("const",), ("fresh", what), ("param", name),
# ("slot", ptr), ("field", base, field).

@dataclass
class State:
    sources: dict[str, frozenset]           # object var -> sources
    stored: dict[str, frozenset]            # object var -> anchors holding it
    slots: dict[str, bool]                  # PyValue * var -> slot provenance
    spans: tuple[str, ...]                  # pushed spans, innermost last
    # (pointer, span): a borrowed args[-1] slot whose value is in the span
    saved: frozenset = frozenset()
    loads: dict[str, str] = field(default_factory=dict)  # var -> pointer it read
    # (var, line): a frame pushed into var and not yet initialized
    pushed: frozenset = frozenset()

    def copy(self) -> "State":
        return State(dict(self.sources), dict(self.stored), dict(self.slots),
                     self.spans, self.saved, dict(self.loads), self.pushed)


@dataclass
class Error:
    line: int
    message: str

    def __str__(self) -> str:
        return f"{self.line}: {self.message}"


def merge(a: State | None, b: State) -> tuple[State, list[str]]:
    if a is None:
        return b.copy(), []
    problems = []
    sources = dict(a.sources)
    for k, v in b.sources.items():
        sources[k] = sources.get(k, frozenset()) | v
    stored = {k: a.stored.get(k, frozenset()) & b.stored.get(k, frozenset())
              for k in set(a.stored) | set(b.stored)}
    # A variable anchored by a store on one path only (`kwdict = keywords`
    # with keywords in a span on one, a fresh dict appended to it on the
    # other) is anchored there by that store, which is a source: an anchor
    # and the source of the same name end together.
    for k in stored:
        if stored[k]:
            continue
        merged = frozenset()
        for side in (a, b):
            anchors = side.stored.get(k, frozenset())
            if anchors:
                merged |= {min(anchors, key=lambda x: (x[0] != "span", x))}
            else:
                merged |= side.sources.get(k, frozenset())
        if merged:
            sources[k] = merged
    slots = {k: a.slots.get(k, False) and b.slots.get(k, False)
             for k in set(a.slots) | set(b.slots)}
    if a.spans != b.spans:
        problems.append(f"spans {list(a.spans)} and {list(b.spans)} meet")
    spans = a.spans if len(a.spans) <= len(b.spans) else b.spans
    loads = {k: v for k, v in a.loads.items() if b.loads.get(k) == v}
    return State(sources, stored, slots, spans, a.saved & b.saved,
                 loads, a.pushed | b.pushed), problems


class FunctionChecker:
    def __init__(self, fn: Function, classifier: Classifier,
                 typedefs: set[str], returns: dict[str, str],
                 precise_names: set[str], args_anchored_names: set[str]):
        self.fn = fn
        self.cls = classifier
        self.typedefs = typedefs
        self.returns = returns
        self.precise_names = precise_names
        self.args_anchored_names = args_anchored_names
        self.errors: list[Error] = []
        self.kinds: dict[str, str] = {}
        self.borrowed: set[str] = set()
        for p in fn.params:
            self.kinds[p.name] = p.kind

    def error(self, line: int, msg: str) -> None:
        e = Error(line, msg)
        if all(str(x) != str(e) for x in self.errors):
            self.errors.append(e)

    # -- preparation

    def run(self) -> list[Error]:
        body = StmtParser(self.fn.body_toks, set(self.typedefs)).block_from_body()
        self.collect_kinds(body)
        self.cfg = build_cfg(body)
        self.owners = self.derived_owners()
        self.check_subset()
        self.liveness()
        self.forward()
        return self.errors

    def collect_kinds(self, s: Stmt) -> None:
        if isinstance(s, Decl):
            for name, kind, init in s.vars:
                self.kinds[name] = kind
                if init is not None and self.is_slot_before(init):
                    self.borrowed.add(name)
        for c in stmt_children(s):
            self.collect_kinds(c)

    def is_slot_before(self, e: Expr) -> bool:
        """Is e `(T *)args - 1`, a pointer to the slot before a received
        array, which PY_VECTORCALL_ARGUMENTS_OFFSET lends the callee?"""
        while isinstance(e, Cast):
            e = e.operand
        return isinstance(e, Binary) and e.op == "-" and \
            isinstance(e.right, Const) and e.right.text == "1" and \
            self.base_name(e.left) in self.received_arrays()

    def borrowed_slot(self, e: Expr) -> str | None:
        """The pointer whose borrowed slot e is: `p[0]` or `*p` for a p
        from is_slot_before(), or `args[-1]`."""
        if isinstance(e, Index) and isinstance(e.base, Name):
            i = e.index
            if e.base.id in self.borrowed and isinstance(i, Const) and i.text == "0":
                return e.base.id
            if e.base.id in self.received_arrays() and isinstance(i, Unary) \
                    and i.op == "-" and isinstance(i.operand, Const) \
                    and i.operand.text == "1":
                return e.base.id
        if isinstance(e, Unary) and e.op == "*" and isinstance(e.operand, Name) \
                and e.operand.id in self.borrowed:
            return e.operand.id
        return None

    def note_load(self, name: str, value: Expr, st: State) -> None:
        while isinstance(value, Cast):
            value = value.operand
        ptr = self.borrowed_slot(value)
        if ptr is not None:
            st.loads[name] = ptr
        else:
            st.loads.pop(name, None)
        st.pushed = frozenset(p for p in st.pushed if p[0] != name)
        if isinstance(value, Call) and value.name in FRAME_PUSH:
            st.pushed = st.pushed | {(name, value.line)}

    def is_obj(self, name: str) -> bool:
        return self.kinds.get(name) in ("object", "array")

    def check_subset(self) -> None:
        if self.fn.name in DETACH_HELPERS and self.fn.precise:
            self.error(self.fn.line, f"'{self.fn.name}' may return while its "
                       "thread is detached and may not be marked precise")
        for n in self.cfg.nodes:
            for e in self.node_exprs(n):
                for x in walk_expr(e):
                    if isinstance(x, Name) and x.id in self.args_anchored_names \
                            and not self.is_callee(x, e):
                        self.error(x.line, f"'{x.id}' is {ARGS_ANCHORED}; its "
                                   "address may not be taken")
                    if isinstance(x, Cast) and self.hides(x):
                        self.error(x.line, "an object is cast to a type that "
                                   "is not an object, which hides it")
            for name, kind, init in n.decls:
                if kind in ("other", "pointer", "values") and init is not None \
                        and self.object_valued(init):
                    self.error(n.line, f"an object is stored in '{name}', which "
                               "is not object-typed")
        for n in self.cfg.nodes:
            for e in self.node_exprs(n):
                for x in walk_expr(e):
                    if isinstance(x, Call) and x.name in ("setjmp", "_setjmp",
                                                         "sigsetjmp"):
                        self.error(x.line, "setjmp is outside the C mode's subset")
                    if isinstance(x, Call):
                        for a in x.args:
                            if isinstance(a, Unary) and a.op == "&" and \
                                    isinstance(a.operand, Name) and \
                                    self.is_obj(a.operand.id) and \
                                    x.name not in OUT_PARAMS:
                                self.error(x.line, f"the address of object local "
                                           f"'{a.operand.id}' escapes to "
                                           f"'{x.name or 'a call'}'")

    @staticmethod
    def is_callee(x: Name, e: Expr) -> bool:
        return any(isinstance(c, Call) and c.func is x for c in walk_expr(e))

    def hides(self, c: Cast) -> bool:
        words = [t for t in c.type if t not in QUALIFIERS]
        if not words:
            return False
        stars = words.count("*")
        base = [w for w in words if w != "*"][-1]
        target_obj = var_kind([base], stars, False) in ("object",)
        if target_obj or (base == "PyValue" and stars == 0):
            return False
        if base in ("uintptr_t", "intptr_t", "size_t", "Py_ssize_t", "long",
                    "int", "char") or (base == "void" and stars >= 1):
            inner = c.operand
            while isinstance(inner, Cast):
                inner = inner.operand
            return isinstance(inner, Name) and self.is_obj(inner.id) \
                and self.hidden_use(c)
        return False

    def hidden_use(self, c: Cast) -> bool:
        """A cast is a hiding only if it is stored, not if it is an
        argument of a call (the callee's) or compared."""
        for n in self.cfg.nodes:
            for e in self.node_exprs(n):
                for x in walk_expr(e):
                    if isinstance(x, Assign) and x.value is c:
                        return True
            for _, _, init in n.decls:
                if init is c:
                    return True
        return False

    def object_valued(self, e: Expr) -> bool:
        while isinstance(e, Cast):
            e = e.operand
        return isinstance(e, Name) and self.is_obj(e.id)

    def node_exprs(self, n: Node) -> list[Expr]:
        out = list(n.exprs)
        for _, _, init in n.decls:
            if init is not None:
                out.append(init)
        return out

    # -- liveness

    def derived_owners(self) -> dict[str, frozenset[str]]:
        """For each non-object pointer local, the object locals its value
        is derived from (PyBytes_AS_STRING(o), &o->f, _PyTuple_ITEMS(o)):
        a use of the pointer is a use of each of them."""
        direct: dict[str, set[str]] = {}

        def note(name: str, value: Expr) -> None:
            if self.kinds.get(name) not in ("pointer", "values"):
                return
            while isinstance(value, Cast):
                value = value.operand
            if isinstance(value, Call) and value.name in COPIES_RECEIVED:
                # It returns its source or its destination array
                src_i, dst_i, _ = COPIES_RECEIVED[value.name]
                value = Comma(value.line, [a for i, a in enumerate(value.args)
                                           if i in (src_i, dst_i)])
            names = {x.id for x in walk_expr(value) if isinstance(x, Name)}
            direct.setdefault(name, set()).update(names)

        for n in self.cfg.nodes:
            for name, _, init in n.decls:
                if init is not None:
                    note(name, init)
            for e in n.exprs:
                for x in walk_expr(e):
                    if isinstance(x, Assign) and isinstance(x.target, Name):
                        note(x.target.id, x.value)
        owners: dict[str, frozenset[str]] = {}
        for p in direct:
            seen: set[str] = set()
            todo = [p]
            while todo:
                q = todo.pop()
                for m in direct.get(q, ()):
                    if m not in seen:
                        seen.add(m)
                        todo.append(m)
            objs = frozenset(m for m in seen if self.is_obj(m))
            if objs:
                owners[p] = objs
        return owners

    def uses_defs(self, n: Node) -> tuple[set[str], set[str]]:
        uses: set[str] = set()
        defs: set[str] = set()

        def visit(e: Expr, target: bool = False) -> None:
            if isinstance(e, Name):
                if self.is_obj(e.id) and not target:
                    uses.add(e.id)
                if not target:
                    uses.update(self.owners.get(e.id, ()))
                return
            if isinstance(e, Assign):
                if isinstance(e.target, Name) and self.is_obj(e.target.id):
                    if e.op != "=":
                        uses.add(e.target.id)
                    visit(e.value)
                    # A plain assignment kills only if not also used
                    defs.add(e.target.id)
                    return
                visit(e.target, False)
                visit(e.value)
                return
            if isinstance(e, Call):
                name = e.name
                if name in RELEASES:
                    return
                if name in CLEAR_MACROS and e.args and isinstance(e.args[0], Name):
                    defs.add(e.args[0].id)
                    return
                if name in ASSIGN_MACROS and len(e.args) == 2 and \
                        isinstance(e.args[0], Name):
                    defs.add(e.args[0].id)
                    visit(e.args[1])
                    return
                if name in OUT_PARAMS:
                    for a in e.args[1:]:
                        if isinstance(a, Unary) and a.op == "&" and \
                                isinstance(a.operand, Name) and self.is_obj(a.operand.id):
                            defs.add(a.operand.id)
                    visit(e.args[0])
                    return
                visit(e.func)
                for a in e.args:
                    visit(a)
                return
            for c in children(e):
                visit(c)

        for e in n.exprs:
            visit(e)
        for name, kind, init in n.decls:
            if init is not None:
                visit(init)
            if kind in ("object", "array"):
                defs.add(name)
        return uses, defs

    def liveness(self) -> None:
        self.live_in: dict[int, set[str]] = {id(n): set() for n in self.cfg.nodes}
        self.live_out: dict[int, set[str]] = {id(n): set() for n in self.cfg.nodes}
        ud = {id(n): self.uses_defs(n) for n in self.cfg.nodes}
        changed = True
        while changed:
            changed = False
            for n in reversed(self.cfg.nodes):
                out: set[str] = set()
                for s in n.succ:
                    out |= self.live_in[id(s)]
                uses, defs = ud[id(n)]
                inn = uses | (out - defs)
                if out != self.live_out[id(n)] or inn != self.live_in[id(n)]:
                    self.live_out[id(n)] = out
                    self.live_in[id(n)] = inn
                    changed = True

    # -- sources and anchors

    def initial(self) -> State:
        st = State({}, {}, {}, ())
        for p in self.fn.params:
            if p.kind in ("object", "array"):
                st.sources[p.name] = frozenset(
                    [("const",)] if self.fn.args_anchored else [("param", p.name)])
                st.stored[p.name] = frozenset()
            elif p.kind == "slots":
                st.slots[p.name] = True
            elif p.kind == "values":
                st.slots[p.name] = False
        return st

    def forward(self) -> None:
        states: dict[int, State] = {}
        entry = self.initial()
        states[id(self.cfg.entry)] = entry
        work = [self.cfg.entry]
        reported_merge: set[int] = set()
        self.checking = False
        # Iterate to a fixed point without reporting, then report once.
        iterations = 0
        while work:
            iterations += 1
            if iterations > 20000:
                raise CheckError("the analysis did not converge")
            n = work.pop()
            st = states[id(n)].copy()
            self.transfer(n, st)
            for k, s in enumerate(n.succ):
                out = st.copy()
                if k in n.edge_null:
                    var, is_null = n.edge_null[k]
                    if is_null and var in out.sources:
                        out.sources[var] = frozenset([("const",)])
                    if is_null:
                        # The push failed: no frame
                        out.pushed = frozenset(
                            p for p in out.pushed if p[0] != var)
                old = states.get(id(s))
                new, problems = merge(old, out)
                if problems and id(s) not in reported_merge and old is not None:
                    reported_merge.add(id(s))
                    self.error(s.line or n.line,
                               "a span is pushed on one path and not on "
                               f"another ({problems[0]})")
                if old is None or new != old:
                    states[id(s)] = new
                    work.append(s)
        self.checking = True
        for n in self.cfg.nodes:
            if id(n) in states:
                st = states[id(n)].copy()
                self.transfer(n, st)
        ex = states.get(id(self.cfg.exit))
        if ex is not None and ex.spans:
            self.error(self.fn.line, f"span '{ex.spans[-1]}' is not popped on "
                       "every path to the return")

    def transfer(self, n: Node, st: State) -> None:
        live_after = self.live_out[id(n)]
        if n.detach:
            self.safepoint(n, None, "reentrant", "Py_BEGIN_ALLOW_THREADS",
                           st, live_after, set(), [])
        for name, kind, init in n.decls:
            if init is not None:
                srcs = self.eval(init, st, n, live_after, [])
                self.note_load(name, init, st)
                if kind in ("object", "array"):
                    self.assign_var(name, srcs, st)
                    self.copy_anchors(name, init, st)
                elif kind in ("slots", "values"):
                    st.slots[name] = self.slot_provenance(init, st)
            elif kind in ("object", "array"):
                st.sources[name] = frozenset([("const",)])
                st.stored[name] = frozenset()
            elif kind in ("slots", "values"):
                st.slots[name] = False
        for e in n.exprs:
            self.eval(e, st, n, live_after, [])

    def slot_provenance(self, e: Expr, st: State) -> bool:
        while isinstance(e, Cast):
            e = e.operand
        if isinstance(e, Name):
            return st.slots.get(e.id, False)
        if isinstance(e, Member) and e.field == "base" and isinstance(e.base, Name) \
                and e.base.id == "frame":
            return True
        if isinstance(e, Call) and e.name == "_PyFrame_GetLocalsArray":
            return True
        if isinstance(e, Binary) and e.op in ("+", "-"):
            return self.slot_provenance(e.left, st)
        if isinstance(e, Unary) and e.op == "&" and isinstance(e.operand, Index):
            return self.slot_provenance(e.operand.base, st)
        return False

    def assign_var(self, name: str, srcs: frozenset, st: State) -> None:
        st.sources[name] = srcs
        st.stored[name] = frozenset()
        # Borrows through the old value of name are no longer anchored
        for k, v in list(st.sources.items()):
            if k != name and any(s[0] == "field" and s[1] == name for s in v):
                st.sources[k] = frozenset(
                    ("fresh", f"a borrow from '{name}', reassigned")
                    if s[0] == "field" and s[1] == name else s for s in v)

    def copy_anchors(self, name: str, value: Expr, st: State) -> None:
        """A copy of a variable, `kwdict = keywords`, holds the same pointer,
        so the stores that anchor the one anchor the other."""
        src = self.base_name(value)
        if src is not None and src != name and self.is_obj(src):
            st.stored[name] = st.stored.get(src, frozenset())

    def invalidate_slots(self, ptr: str | None, st: State, keep: str | None) -> None:
        for k, v in list(st.sources.items()):
            if any(s[0] == "slot" for s in v):
                st.sources[k] = frozenset(
                    ("fresh", f"a slot of '{s[1]}', overwritten")
                    if s[0] == "slot" else s for s in v)
        for k, v in list(st.stored.items()):
            if k != keep:
                st.stored[k] = frozenset(a for a in v if a[0] != "slot")

    def invalidate_field(self, field_name: str, st: State) -> None:
        for k, v in list(st.sources.items()):
            if any(s[0] == "field" and s[2] == field_name for s in v):
                st.sources[k] = frozenset(
                    ("fresh", f"'{s[1]}->{s[2]}', overwritten")
                    if s[0] == "field" and s[2] == field_name else s for s in v)

    def field_strength(self, base: str, fld: str) -> str:
        if fld in IMMUTABLE_FIELDS:
            return "immutable"
        if fld in STRONG_FIELDS:
            return "strong"
        return "weak"

    def anchored(self, var: str, st: State, kind: str,
                 seen: frozenset = frozenset()) -> tuple[bool, str]:
        if st.stored.get(var):
            return True, ""
        srcs = st.sources.get(var, frozenset([("fresh", "unknown")]))
        for s in srcs:
            ok, why = self.source_anchored(s, st, kind, seen | {var})
            if not ok:
                return False, why
        return True, ""

    def source_anchored(self, s: tuple, st: State, kind: str,
                        seen: frozenset) -> tuple[bool, str]:
        if s[0] in ("const", "received"):
            return True, ""
        if s[0] == "fresh":
            return False, s[1]
        if s[0] == "param":
            return False, f"parameter '{s[1]}', which no span covers"
        if s[0] == "span":
            return (True, "") if s[1] in st.spans else (
                False, f"span '{s[1]}', which is not pushed")
        if s[0] == "slot":
            return (True, "") if st.slots.get(s[1], False) else (
                False, f"'{s[1]}', which is not a frame slot pointer")
        if s[0] == "field":
            base, fld = s[1], s[2]
            strength = self.field_strength(base, fld)
            if strength == "weak":
                return False, f"a borrow of untraced or weak field '{fld}'"
            if kind == "reentrant" and strength != "immutable":
                return False, (f"a borrow of mutable field '{fld}' across a "
                               "reentrant call")
            if base in seen or not self.is_obj(base):
                return True, ""
            ok, why = self.anchored(base, st, kind, seen)
            return (True, "") if ok else (False, f"a borrow from '{base}' ({why})")
        return False, "unknown"

    def base_name(self, e: Expr) -> str | None:
        while isinstance(e, Cast):
            e = e.operand
        if isinstance(e, Call) and e.name in IDENTITY and e.args:
            return self.base_name(e.args[0])
        if isinstance(e, Name):
            return e.id
        return None

    def eval(self, e: Expr, st: State, n: Node, live_after: set[str],
             held: list[tuple[str, int]]) -> frozenset:
        """Evaluate e for its effects on st and return the sources of its
        value.  held lists the temporaries that enclosing calls hold while
        e is evaluated: (callee, line)."""
        if isinstance(e, Name):
            if self.is_obj(e.id):
                return st.sources.get(e.id, frozenset([("fresh", "unknown")]))
            return frozenset([("const",)])
        if isinstance(e, Const):
            return frozenset([("const",)])
        if isinstance(e, Cast):
            return self.eval(e.operand, st, n, live_after, held)
        if isinstance(e, Cond):
            self.eval(e.test, st, n, live_after, held)
            a = self.eval(e.then, st, n, live_after, held)
            b = self.eval(e.other, st, n, live_after, held)
            return a | b
        if isinstance(e, Comma):
            r = frozenset([("const",)])
            for x in e.items:
                r = self.eval(x, st, n, live_after, held)
            return r
        if isinstance(e, InitList):
            for x in e.items:
                self.eval(x, st, n, live_after, held)
            return frozenset([("const",)])
        if isinstance(e, Binary):
            self.eval(e.left, st, n, live_after, held)
            self.eval(e.right, st, n, live_after, held)
            return frozenset([("const",)])
        if isinstance(e, Unary):
            r = self.eval(e.operand, st, n, live_after, held)
            if e.op == "*":
                base = self.base_name(e.operand)
                if base is not None and base in st.slots:
                    return frozenset([("slot", base)])
                return frozenset([("fresh", "a load through a pointer")])
            if e.op == "&":
                return frozenset([("const",)])
            return r
        if isinstance(e, Member):
            self.eval(e.base, st, n, live_after, held)
            base = self.base_name(e.base)
            if isinstance(e.base, Name) and self.kinds.get(e.base.id) == "span":
                if e.field in ("value", "object"):
                    return frozenset([("span", e.base.id)])
            if base is not None:
                return frozenset([("field", base, e.field)])
            if isinstance(e.base, Member):
                return frozenset([("fresh", f"a load of '{e.field}' through a field")])
            return frozenset([("fresh", f"a load of '{e.field}'")])
        if isinstance(e, Index):
            self.eval(e.base, st, n, live_after, held)
            self.eval(e.index, st, n, live_after, held)
            base = self.base_name(e.base)
            if base is not None and base in st.slots:
                return frozenset([("slot", base)])
            if isinstance(e.base, Member):
                inner = self.base_name(e.base.base)
                if inner is not None:
                    return frozenset([("field", inner, e.base.field)])
            if base is not None and self.kinds.get(base) == "array":
                return st.sources.get(base, frozenset([("fresh", "an array")]))
            return frozenset([("fresh", "an element of an array")])
        if isinstance(e, Assign):
            return self.eval_assign(e, st, n, live_after, held)
        if isinstance(e, Call):
            return self.eval_call(e, st, n, live_after, held)
        return frozenset([("fresh", "an expression")])

    def eval_assign(self, e: Assign, st: State, n: Node, live_after: set[str],
                    held: list[tuple[str, int]]) -> frozenset:
        srcs = self.eval(e.value, st, n, live_after, held)
        t = e.target
        while isinstance(t, Cast):
            t = t.operand
        if isinstance(t, Name):
            self.note_load(t.id, e.value, st)
            if self.is_obj(t.id) and e.op == "=":
                self.assign_var(t.id, srcs, st)
                self.copy_anchors(t.id, e.value, st)
            elif t.id in st.slots and e.op == "=":
                st.slots[t.id] = self.slot_provenance(e.value, st)
            return srcs
        stored_var = self.base_name(e.value) if e.op == "=" else None
        if stored_var is not None and not self.is_obj(stored_var):
            stored_var = None
        if isinstance(t, Member):
            self.eval(t.base, st, n, live_after, held)
            if isinstance(t.base, Name) and self.kinds.get(t.base.id) == "span":
                if t.base.id not in st.spans:
                    self.error(e.line, f"a store into span '{t.base.id}', "
                               "which is not pushed")
                    return srcs
                # The span holds this value now, and no other
                anchor = ("span", t.base.id)
                for k, v in list(st.stored.items()):
                    st.stored[k] = v - {anchor}
                for k, v in list(st.sources.items()):
                    if anchor in v:
                        st.sources[k] = frozenset(
                            ("fresh", f"span '{t.base.id}', overwritten")
                            if x == anchor else x for x in v)
                if stored_var is not None:
                    st.stored[stored_var] = st.stored.get(stored_var, frozenset()) | {anchor}
                return srcs
            self.invalidate_field(t.field, st)
            return srcs
        if isinstance(t, (Index, Unary)):
            lent = self.borrowed_slot(t)
            if lent is not None and self.checking and not any(
                    (lent, r) in st.saved for r in st.spans):
                # Its creator's anchor of the old value ends with the store
                # (10a-2.md, rule 4).
                self.error(e.line, "a slot that PY_VECTORCALL_ARGUMENTS_OFFSET "
                           f"lends (through '{lent}') is overwritten while "
                           "no pushed span holds its value")
            ptr = t.base if isinstance(t, Index) else t.operand
            if isinstance(t, Index):
                self.eval(t.index, st, n, live_after, held)
            base = self.base_name(ptr)
            if isinstance(ptr, Member) and isinstance(ptr.base, Name) and \
                    self.kinds.get(ptr.base.id) == "span":
                # Not credited: only _PyValueSpan_Append covers the store
                # with the span's prefix.
                if ptr.base.id not in st.spans:
                    self.error(e.line, f"a store into span '{ptr.base.id}', "
                               "which is not pushed")
                return srcs
            if base is not None and self.kinds.get(base) == "array" \
                    and isinstance(t, Index):
                st.sources[base] = st.sources.get(base, frozenset()) | srcs
                return srcs
            if base is not None and base in st.slots:
                is_slot = st.slots[base]
                self.invalidate_slots(base, st, stored_var)
                if is_slot and stored_var is not None:
                    st.stored[stored_var] = st.stored.get(stored_var, frozenset()) | {("slot", base)}
                return srcs
            if isinstance(ptr, Member):
                self.invalidate_field(ptr.field, st)
            return srcs
        return srcs

    def eval_call(self, e: Call, st: State, n: Node, live_after: set[str],
                  held: list[tuple[str, int]]) -> frozenset:
        name = e.name
        # Span scopes
        if name in SPAN_PUSH or name in SPAN_PUSH_OBJECT:
            root = self.span_arg(e, 1 if name in SPAN_PUSH else SPAN_PUSH_OBJECT[name])
            if root is not None:
                st.spans = st.spans + (root,)
            if name == "_PyThreadState_PushValueSpan" and root is not None:
                # The buffer's elements are the span's while it is pushed:
                # what is appended, and what a callee stores through an
                # out-parameter into it (10a-2.md, 0e).  That such a store
                # lands inside the prefix is the code's obligation; the C
                # mode does not see indices.
                for a in e.args[:1]:
                    self.eval(a, st, n, live_after, held)
                buf = self.base_name(e.args[2]) if len(e.args) > 2 else None
                if buf is not None and self.kinds.get(buf) == "array":
                    st.sources[buf] = frozenset([("span", root)])
                return frozenset([("const",)])
            if name in SPAN_PUSH_OBJECT and root is not None:
                # The root keeps the box it returns
                for a in e.args:
                    self.eval(a, st, n, live_after, held)
                return frozenset([("span", root)])
        if name in SPAN_POP or name in SPAN_POP_OBJECT:
            root = self.span_arg(e, 1 if name in SPAN_POP else SPAN_POP_OBJECT[name])
            if root is not None:
                if not st.spans or st.spans[-1] != root:
                    if root in st.spans:
                        self.error(e.line, f"span '{root}' is popped before "
                                   f"the spans pushed after it")
                    else:
                        self.error(e.line, f"span '{root}' is popped but not pushed")
                st.spans = tuple(s for s in st.spans if s != root)
                st.saved = frozenset(x for x in st.saved if x[1] != root)
                for k, v in list(st.stored.items()):
                    st.stored[k] = frozenset(a for a in v if a != ("span", root))
                for k, v in list(st.sources.items()):
                    if ("span", root) in v:
                        st.sources[k] = frozenset(
                            ("fresh", f"span '{root}', popped")
                            if s == ("span", root) else s for s in v)
        if name in CLEAR_MACROS and e.args and isinstance(e.args[0], Name):
            if self.is_obj(e.args[0].id):
                self.assign_var(e.args[0].id, frozenset([("const",)]), st)
            return frozenset([("const",)])
        if name in ASSIGN_MACROS and len(e.args) == 2:
            srcs = self.eval(e.args[1], st, n, live_after, held)
            if isinstance(e.args[0], Name) and self.is_obj(e.args[0].id):
                self.assign_var(e.args[0].id, srcs, st)
            return frozenset([("const",)])
        if name in RELEASES:
            return frozenset([("const",)])
        # Evaluate the arguments, left to right; each completed argument
        # that is a call result is a temporary while the later ones run.
        inner_held = list(held)
        arg_srcs = []
        for a in e.args:
            s = self.eval(a, st, n, live_after, inner_held)
            arg_srcs.append(s)
            if self.is_temporary(a):
                inner_held.append((self.callee_of(a), a.line))
        if not isinstance(e.func, Name):
            self.eval(e.func, st, n, live_after, held)
        if name in (SPAN_APPEND, SPAN_APPEND_OBJECT) and len(e.args) == 2:
            root = self.span_arg(e, 0)
            stored = e.args[1]
            while isinstance(stored, Cast):
                stored = stored.operand
            if name == SPAN_APPEND and isinstance(stored, Call) \
                    and stored.name in CANONICAL:
                var = None  # the canonical value, not the object (10a-2.md, 0d)
            else:
                var = self.base_name(stored)
            if root is not None and root in st.spans:
                lent = self.borrowed_slot(stored)
                if lent is None and isinstance(stored, Name):
                    lent = st.loads.get(stored.id)
                if lent is not None:
                    st.saved = st.saved | {(lent, root)}
            if root is None or root not in st.spans:
                self.error(e.line, f"an append to span '{root}', which is not "
                           "pushed")
            elif var is not None and self.is_obj(var):
                st.stored[var] = st.stored.get(var, frozenset()) | {("span", root)}
            return frozenset([("const",)])
        # A call that writes memory it is given ends the anchors in it:
        # MUTATING, and a call that may write the slots it is passed.
        if name in MUTATING:
            self.invalidate_slots(None, st, None)
            for fld in ("ob_item", "ob_item@tuple", "ob_ref", "dict_entries"):
                self.invalidate_field(fld, st)
        elif self.passes_writable_slots(e, st):
            self.invalidate_slots(None, st, None)
        if name == "_PyValueSpan_Rebase" and len(e.args) == 2:
            root = self.span_arg(e, 0)
            b = self.base_name(e.args[1])
            if root is not None and b is not None and self.kinds.get(b) == "array":
                st.sources[b] = frozenset([("span", root)])
            return frozenset([("const",)])
        if name in COPIES_RECEIVED:
            src_i, dst_i, null_i = COPIES_RECEIVED[name]
            if len(e.args) > max(src_i, dst_i) and all(
                    isinstance(e.args[i], Name) and e.args[i].id == "NULL"
                    for i in null_i):
                src = self.base_name(e.args[src_i])
                dst = self.base_name(e.args[dst_i])
                if dst is not None and self.kinds.get(dst) == "array" \
                        and src in self.received_arrays():
                    st.sources[dst] = frozenset([("received", src)])
        for a in e.args:
            b = self.base_name(a)
            if name in COPIES_RECEIVED and b is not None \
                    and st.sources.get(b) and all(
                        s[0] == "received" for s in st.sources[b]):
                continue
            # A call that cannot collect and is not listed as writing its
            # arguments (Py_ARRAY_LENGTH, a predicate) stores nothing.
            if name is not None and call_effects.kind(name) == "none" \
                    and name not in MUTATING:
                continue
            if b is not None and self.kinds.get(b) == "array" \
                    and not self.span_backed(b, st):
                st.sources[b] = st.sources.get(b, frozenset()) | {
                    ("fresh", f"an element that '{name or 'a call'}' stored")}
        kind = self.cls.kind(name)
        if kind != "none":
            self.safepoint(n, e, kind, name or "a function pointer", st,
                           live_after, self.names_outside(n, e), held)
            if name is not None and name in self.args_anchored_names:
                for a, s in zip(e.args, arg_srcs):
                    self.check_arg_anchored(e, a, s, st)
            elif name is None or name not in self.precise_names:
                # Question 13: an opaque callee's frame scan does not reach
                # the caller's extent, where its stack-passed arguments are.
                for i, (a, s) in enumerate(zip(e.args, arg_srcs)):
                    if i + 1 >= FIRST_STACK_ARG:
                        self.check_arg_anchored(e, a, s, st, i + 1)
        if name == FRAME_INIT and len(e.args) > 1:
            f = self.base_name(e.args[1])
            st.pushed = frozenset(p for p in st.pushed if p[0] != f)
        if name in OUT_PARAMS:
            base = self.base_name(e.args[0]) if e.args else None
            for a in e.args[1:]:
                if isinstance(a, Unary) and a.op == "&" and isinstance(a.operand, Name) \
                        and self.is_obj(a.operand.id):
                    self.assign_var(a.operand.id, frozenset(
                        [("field", base, OUT_PARAMS[name])] if base else
                        [("fresh", f"an out-parameter of '{name}'")]), st)
        if name in CONSTANT_CALLS:
            return frozenset([("const",)])
        if name in IDENTITY and arg_srcs:
            return arg_srcs[0]
        if name in BORROW_FIELD and e.args:
            base = self.base_name(e.args[0])
            fld = BORROW_FIELD[name]
            if fld == "ob_item" and name.startswith(("PyTuple", "_PyTuple")):
                fld = "ob_item@tuple"
            if base is not None:
                return frozenset([("field", base, fld)])
        return frozenset([("fresh", f"the result of '{name or 'a call'}' "
                           f"(line {e.line})")])

    def passes_writable_slots(self, e: Call, st: State) -> bool:
        if self.cls.kind(e.name) == "none":
            return False
        if e.name is not None and e.name in self.returns and \
                self.returns.get(e.name + "()") != "writes":
            return False
        for a in e.args:
            b = self.base_name(a)
            if isinstance(a, Unary) and a.op == "&" and isinstance(a.operand, Index):
                b = self.base_name(a.operand.base)
            if isinstance(a, Binary):
                b = self.base_name(a.left)
            if b is not None and st.slots.get(b, False):
                return True
        return False

    def received_arrays(self) -> set[str]:
        """The parameters that point at an array of the caller's."""
        return {p.name for p in self.fn.params if p.kind in ("pointer", "values")}

    def span_backed(self, buf: str, st: State) -> bool:
        """Is buf the buffer of a pushed span?  A callee's stores into it
        are stores of values into covered slots."""
        srcs = st.sources.get(buf, frozenset())
        return bool(srcs) and all(s[0] == "span" and s[1] in st.spans
                                  for s in srcs)

    def span_arg(self, e: Call, idx: int) -> str | None:
        if len(e.args) > idx:
            a = e.args[idx]
            if isinstance(a, Unary) and a.op == "&" and isinstance(a.operand, Name):
                return a.operand.id
            if isinstance(a, Name):
                return a.id
        return None

    def is_temporary(self, a: Expr) -> bool:
        while isinstance(a, Cast):
            a = a.operand
        if not isinstance(a, Call):
            return False
        name = a.name
        if name in IDENTITY or name in BORROW_FIELD or name in CONSTANT_CALLS:
            return False
        if name is None:
            return True
        r = self.returns.get(name)
        if r is not None:
            return r == "object"
        # Unknown: a predicate or an accessor of a non-object is not one
        return self.cls.kind(name) != "none"

    @staticmethod
    def callee_of(a: Expr) -> str:
        while isinstance(a, Cast):
            a = a.operand
        return a.name if isinstance(a, Call) and a.name else "a call"

    def names_outside(self, n: Node, call: Call) -> set[str]:
        """Object variables the node uses outside call's own subtree, other
        than as the target of an assignment."""
        inside = {id(x) for x in walk_expr(call)}
        out: set[str] = set()
        for x in walk_expr(call):
            if isinstance(x, Name):
                out.update(self.owners.get(x.id, ()))

        def visit(e: Expr, target: bool = False) -> None:
            if id(e) in inside:
                return
            if isinstance(e, Name):
                if not target and self.is_obj(e.id):
                    out.add(e.id)
                if not target:
                    out.update(self.owners.get(e.id, ()))
                return
            if isinstance(e, Assign):
                visit(e.target, isinstance(e.target, Name) and e.op == "=")
                visit(e.value)
                return
            if isinstance(e, Call) and e.name in RELEASES:
                return
            if isinstance(e, Call) and e.name in CLEAR_MACROS:
                return
            for c in children(e):
                visit(c)

        for e in self.node_exprs(n):
            visit(e)
        return out

    def safepoint(self, n: Node, call: Call | None, kind: str, what: str,
                  st: State, live_after: set[str], outside: set[str],
                  held: list[tuple[str, int]]) -> None:
        if not self.checking:
            return
        # The node's assignments kill: `x = f(x)` passes x to f, which
        # anchors its parameters, and x's old value is dead after the call.
        # A use outside the call's subtree is in outside.
        _, defs = self.uses_defs(n)
        live = (live_after - defs) | outside
        line = call.line if call is not None else n.line
        for var in sorted(live):
            if not self.is_obj(var):
                continue
            ok, why = self.anchored(var, st, kind)
            if not ok:
                self.error(line, f"'{var}' is held only in a C local across the "
                           f"collecting call to '{what}' ({why})")
        for callee, tline in held:
            self.error(line, f"the result of '{callee}' (line {tline}) is a "
                       f"temporary held across the collecting call to '{what}'")
        for frame_var, pline in sorted(st.pushed):
            self.error(line, f"the collecting call to '{what}' comes between "
                       f"the push of frame '{frame_var}' (line {pline}) and "
                       f"its {FRAME_INIT}()")

    def check_arg_anchored(self, call: Call, a: Expr, srcs: frozenset,
                           st: State, stack_position: int = 0) -> None:
        if not self.checking:
            return
        base = self.base_name(a)
        if base is not None and self.is_obj(base):
            ok, why = self.anchored(base, st, "reentrant")
        else:
            ok, why = True, ""
            for s in srcs:
                ok, why = self.source_anchored(s, st, "reentrant", frozenset())
                if not ok:
                    break
        if not ok and stack_position:
            self.error(call.line, f"argument {stack_position} of "
                       f"'{call.name or 'a function pointer'}', which is not "
                       f"precise, is passed on the stack and is not anchored "
                       f"({why})")
        elif not ok:
            self.error(call.line, f"an argument of '{call.name}', which is "
                       f"{ARGS_ANCHORED}, is not anchored ({why})")


def stmt_children(s: Stmt) -> list[Stmt]:
    if isinstance(s, Block):
        return s.body
    if isinstance(s, If):
        return [s.then] + ([s.other] if s.other else [])
    if isinstance(s, (While, Switch)):
        return [s.body]
    if isinstance(s, DoWhile):
        return [s.body]
    if isinstance(s, For):
        return ([s.init] if s.init else []) + [s.body]
    if isinstance(s, MacroIf):
        return [x for br in s.branches for x in br]
    return []


def _block_from_body(self: StmtParser) -> Block:
    body: list[Stmt] = []
    line = self.toks[0].line if self.toks else 0
    while self.peek() is not None:
        body.extend(self.stmt_or_pp())
    return Block(line, body)


StmtParser.block_from_body = _block_from_body  # type: ignore[attr-defined]


# --- the static inline functions of the headers -----------------------------

_INLINE_CACHE: dict[str, tuple[dict[str, list[Tok]], dict[str, str],
                               dict[str, Function]]] = {}
_TYPEDEF_CACHE: dict[str, set[str]] = {}


def typedef_names(toks: list[Tok]) -> set[str]:
    """The names that the typedefs among toks declare: the identifier in
    `(*NAME)` of a function pointer type, otherwise the last identifier
    before the closing `;`, outside braces and brackets."""
    names: set[str] = set()
    i = 0
    while i < len(toks):
        if toks[i].text != "typedef":
            i += 1
            continue
        depth = 0
        last = None
        pointer_name = None
        j = i + 1
        while j < len(toks):
            t = toks[j].text
            if t in ("{", "["):
                depth += 1
            elif t in ("}", "]"):
                depth -= 1
            elif t == ";" and depth == 0:
                break
            elif depth == 0 and toks[j].kind == "id":
                last = t
                if (pointer_name is None and j >= 2
                        and toks[j - 1].text == "*"
                        and toks[j - 2].text == "("):
                    pointer_name = t
            j += 1
        name = pointer_name or last
        if name is not None:
            names.add(name)
        i = j + 1
    return names


def header_typedefs(include_dirs: Iterable[Path]) -> set[str]:
    """The typedef names of every header under include_dirs."""
    key = "|".join(str(d) for d in include_dirs)
    if key in _TYPEDEF_CACHE:
        return _TYPEDEF_CACHE[key]
    names: set[str] = set()
    for d in include_dirs:
        for path in sorted(Path(d).rglob("*.h")):
            if "mimalloc" in path.parts:
                continue
            try:
                names |= typedef_names(tokens_of(path.read_text(errors="replace")))
            except CheckError:
                continue
    _TYPEDEF_CACHE[key] = names
    return names


MACRO_DEF = re.compile(r"#\s*define\s+(\w+)\(([^)]*)\)\s*(.*)$", re.S)
MACRO_ALIAS = re.compile(r"#\s*define\s+(\w+)\s+([A-Za-z_]\w*)\s*$")


def macro_bodies(toks: list[Tok]) -> dict[str, list[Tok]]:
    """The bodies of the function-like macros defined in toks, which the
    classifier summarizes as it does static inline functions.  An alias,
    `#define PyUnstable_Long_IsCompact _PyLong_IsCompact`, has the body
    `_PyLong_IsCompact()`: a call to it is a call to its target."""
    out: dict[str, list[Tok]] = {}
    for t in toks:
        if t.kind != "pp":
            continue
        a = MACRO_ALIAS.match(t.text)
        if a is not None:
            out.setdefault(a.group(1), tokens_of(f"{a.group(2)}()", t.line))
            continue
        m = MACRO_DEF.match(t.text)
        if m is None:
            continue
        try:
            body = tokens_of(m.group(3), t.line)
        except Exception:
            continue
        out.setdefault(m.group(1), body)
    return out


def macro_returns(macros: dict[str, list[Tok]], returns: dict[str, str]) -> None:
    """A macro whose body is one call, `NAME(...)` or `(NAME(...))`,
    returns what NAME returns."""
    for _ in range(3):
        for name, body in macros.items():
            if name in returns:
                continue
            texts = [t.text for t in body]
            while texts[:1] == ["("] and texts[-1:] == [")"]:
                texts = texts[1:-1]
            if len(texts) >= 3 and re.match(r"[A-Za-z_]", texts[0]) and texts[1] == "(":
                if texts[0] in returns:
                    returns[name] = returns[texts[0]]


def header_inline_bodies(include_dirs: Iterable[Path]) -> tuple[dict[str, list[Tok]], dict[str, str]]:
    """The bodies of the static inline functions in the headers, and the
    return kind of every function they declare."""
    bodies, returns, _ = _parse_headers(include_dirs)
    return bodies, returns


def header_inline_functions(include_dirs: Iterable[Path]) -> dict[str, Function]:
    """The static inline functions of the headers."""
    return _parse_headers(include_dirs)[2]


def _parse_headers(include_dirs: Iterable[Path]) -> tuple[
        dict[str, list[Tok]], dict[str, str], dict[str, Function]]:
    include_dirs = list(include_dirs)
    key = "|".join(str(d) for d in include_dirs)
    if key in _INLINE_CACHE:
        return _INLINE_CACHE[key]
    bodies: dict[str, list[Tok]] = {}
    returns: dict[str, str] = {}
    functions: dict[str, Function] = {}
    for d in include_dirs:
        for path in sorted(Path(d).rglob("*.h")):
            if "mimalloc" in path.parts:
                continue
            try:
                toks = tokens_of(path.read_text(errors="replace"))
                funcs, _, rets = find_functions(toks)
            except (CheckError, RecursionError):
                continue
            returns.update(rets)
            for fn in funcs:
                if fn.static:
                    bodies.setdefault(fn.name, fn.body_toks)
                    functions.setdefault(fn.name, fn)
            for name, body in macro_bodies(toks).items():
                bodies.setdefault(name, body)
    macro_returns(bodies, returns)
    _INLINE_CACHE[key] = (bodies, returns, functions)
    return _INLINE_CACHE[key]


# --- entry points ------------------------------------------------------------

def check_function(fn: Function, classifier: Classifier, typedefs: set[str],
                   returns: dict[str, str], precise_names: set[str],
                   args_anchored: set[str]) -> list[Error]:
    """The errors in fn's body, checked as a precise function's."""
    try:
        checker = FunctionChecker(fn, classifier, typedefs, returns,
                                  precise_names, args_anchored)
        return [Error(err.line, f"in '{fn.name}': {err.message}")
                for err in checker.run()]
    except CheckError as exc:
        return [Error(fn.line, f"in '{fn.name}': outside the C mode's "
                      f"subset: {exc}")]
    except RecursionError:
        return [Error(fn.line, f"in '{fn.name}': expression too deep")]


def check_source(src: str, function: str | None = None, check_all: bool = False,
                 include_dirs: Iterable[Path] = (),
                 filename: str = "<source>") -> list[Error]:
    toks = tokens_of(src)
    funcs, precise_names, returns = find_functions(toks)
    bodies, header_returns = header_inline_bodies(include_dirs) if include_dirs else ({}, {})
    bodies = dict(bodies)
    # Every function defined in the file is summarized by its body, which
    # is what runs.
    for fn in funcs:
        bodies[fn.name] = fn.body_toks
    local_macros = macro_bodies(toks)
    bodies.update(local_macros)
    all_returns = dict(header_returns)
    all_returns.update(returns)
    macro_returns(local_macros, all_returns)
    classifier = Classifier(bodies)
    args_anchored = {fn.name for fn in funcs if fn.args_anchored}
    errors: list[Error] = []
    # A cast to a type the parser does not know as one ends the parse
    typedefs = typedef_names(toks)
    if include_dirs:
        typedefs |= header_typedefs(include_dirs)
    if function is not None and all(fn.name != function for fn in funcs):
        errors.append(Error(1, f"no function '{function}' in the file"))
    for fn in funcs:
        if function is not None and fn.name != function:
            continue
        if function is None and not check_all and not fn.precise:
            continue
        if fn.precise and fn.variadic:
            # Its unread arguments are in its own frame's register save
            # area until va_arg() reads them, and a precise frame is not
            # scanned (10a-2.md).
            errors.append(Error(fn.line, f"'{fn.name}' is variadic and "
                                f"cannot be {PRECISE}"))
            continue
        if fn.args_anchored and not fn.static:
            errors.append(Error(fn.line, f"'{fn.name}' is {ARGS_ANCHORED} "
                                "but not static"))
        errors += check_function(fn, classifier, typedefs, all_returns,
                                 precise_names, args_anchored)
    # A Py_GC_ARGS_ANCHORED function is called only from precise ones
    for fn in funcs if function is None else ():
        for i, t in enumerate(fn.body_toks[:-1]):
            if t.text in args_anchored and fn.body_toks[i + 1].text == "(" \
                    and not fn.precise:
                errors.append(Error(t.line, f"'{t.text}' is {ARGS_ANCHORED} "
                                    f"but called from '{fn.name}', which is "
                                    "not precise"))
    return errors


def default_include_dirs() -> list[Path]:
    root = Path(__file__).resolve().parents[2]
    return [root / "Include"]


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("files", nargs="+", type=Path)
    ap.add_argument("--function", help="check only this function")
    ap.add_argument("--all", action="store_true",
                    help="check every function, marked or not")
    ap.add_argument("--summary", action="store_true",
                    help="print per-file counts instead of each error")
    args = ap.parse_args(argv)
    total = 0
    inc = default_include_dirs()
    for path in args.files:
        errors = check_source(path.read_text(), args.function, args.all, inc,
                              str(path))
        total += len(errors)
        if args.summary:
            print(f"{path}: {len(errors)} errors")
        else:
            for e in errors:
                print(f"{path}:{e}")
    return 1 if total else 0


if __name__ == "__main__":
    sys.exit(main())
