"""Lexing and recursive-descent parsing of `.rop` definition files.

The tokens and source positions are lexer.py's, unchanged.  The wrapper
accepts only the language's tokens, treats C keywords as identifiers (the
language's keywords are contextual), splits `>>` and `>=` where a type
argument list closes, and fails on any byte that no token or trivia
consumed.  The grammar is in the F0 language reference.
"""

import enum
import re
from collections.abc import Callable
from pathlib import Path
from typing import TypeVar

import lexer
from lexer import Token
from rop_catalog import (AccessMode, BuiltinType, DebtKind, Effect, ExcRole, Fact,
                         FrameRole,
                         Handle, MonitoringEvent, PcRole, Region, RowKind, Scalar, Schema, Select,
                         SchemaCtor, Wrap)
from rop_model import (Access, Advance, Arm, Over, Binding, Block, BoolLit, Branch, Call,
                       CallTerm, Cases, CaseDecl, CBinding, Contract, Debt, Decl,
                       Diagnostic, Each, EdgeFact, Edge, EffectDecl, EnumDecl,
                       EnumType, EvidenceItem, Exit, Expr, Extern, FactUse, Family,
                       Goto, Graph, Guard, HandleType, Let, Macro, Name, Nat, Null,
                       Param, PairType, Point, Publish, Result, Returns, RopError,
                       ScalarType, SchemaType, SchemaVar, SourceFile, SpanLit,
                       SrcSpan, Stmt, Target, Term, Type, WrapType)

ID = "ID"
NUM = "NUM"
STR = "STR"
EOF = "EOF"

# lexer.py's token kinds that the language uses, by their spelling here
PUNCT = {
    lexer.LPAREN: "(", lexer.RPAREN: ")", lexer.LBRACKET: "[", lexer.RBRACKET: "]",
    lexer.LBRACE: "{", lexer.RBRACE: "}", lexer.COMMA: ",", lexer.PERIOD: ".",
    lexer.SEMI: ";", lexer.COLON: ":", lexer.ARROW: "->", lexer.EQUALS: "=",
    lexer.LT: "<", lexer.GT: ">",
}
# Operators that close type argument lists, split into one-character tokens
SPLIT = frozenset((lexer.RSHIFT, lexer.GE, lexer.RSHIFTEQUAL))
WORD_KINDS = frozenset((lexer.IDENTIFIER, lexer.ANNOTATION, *lexer.keywords.values()))
DECIMAL = re.compile(r"0|[1-9][0-9]*")
TRIVIA = frozenset(" \t\n")

X = TypeVar("X")
E = TypeVar("E", bound=enum.Enum)


class Tok:
    __slots__ = ("kind", "text", "span")

    def __init__(self, kind: str, text: str, span: SrcSpan) -> None:
        self.kind = kind
        self.text = text
        self.span = span

    def __repr__(self) -> str:
        return f"{self.kind}({self.text!r} {self.span.line}:{self.span.column})"


def _fail(span: SrcSpan, message: str, definition: str = "lexer") -> RopError:
    return RopError(Diagnostic(span, definition, message))


def _point(filename: str, starts: list[int], offset: int) -> tuple[int, int]:
    line = 0
    lo, hi = 0, len(starts)
    while lo < hi:
        mid = (lo + hi) // 2
        if starts[mid] <= offset:
            line, lo = mid, mid + 1
        else:
            hi = mid
    return line + 1, offset - starts[line] + 1


def _at(filename: str, starts: list[int], offset: int, width: int = 1) -> SrcSpan:
    line, col = _point(filename, starts, offset)
    eline, ecol = _point(filename, starts, offset + width)
    return SrcSpan(filename, line, col, eline, ecol)


def tokenize(src: str, filename: str) -> list[Tok]:
    """The language's tokens of `src`.  Raises RopError at the first byte
    that is neither part of a token nor trivia (space, tab, newline,
    comment)."""
    starts = [0] + [i + 1 for i, ch in enumerate(src) if ch == "\n"]
    out: list[Tok] = []
    covered = 0

    def offset(pos: tuple[int, int]) -> int:
        return starts[pos[0] - 1] + pos[1] - 1

    def gap(upto: int) -> None:
        for i in range(covered, upto):
            if src[i] not in TRIVIA:
                raise _fail(_at(filename, starts, i),
                            f"character {src[i]!r} (U+{ord(src[i]):04X}) is not "
                            "consumed by any token")

    try:
        tokens: list[Token] = list(lexer.tokenize(src, filename=filename))
    except SyntaxError as e:
        line, col = e.lineno or 1, e.offset or 1
        raise _fail(SrcSpan(filename, line, col, line, col + 1),
                    str(e.msg)) from None
    for t in tokens:
        begin, end = offset(t.begin), offset(t.end)
        gap(begin)
        covered = end
        span = SrcSpan(filename, t.begin[0], t.begin[1], t.end[0], t.end[1])
        if t.kind == lexer.COMMENT:
            continue
        if t.kind in WORD_KINDS:
            out.append(Tok(ID, t.text, span))
        elif t.kind == lexer.NUMBER:
            if not DECIMAL.fullmatch(t.text):
                raise _fail(span, f"number {t.text} is not an unsigned decimal")
            out.append(Tok(NUM, t.text, span))
        elif t.kind == lexer.STRING:
            if "\\" in t.text:
                raise _fail(span, "a string has no escapes")
            out.append(Tok(STR, t.text[1:-1], span))
        elif t.kind in PUNCT:
            out.append(Tok(PUNCT[t.kind], t.text, span))
        elif t.kind in SPLIT:
            line, col = t.begin
            for i, ch in enumerate(t.text):
                out.append(Tok(ch, ch, SrcSpan(filename, line, col + i, line, col + i + 1)))
        else:
            raise _fail(span, f"{t.text!r} is not a token of the language")
    gap(len(src))
    eof = _at(filename, starts, len(src), 0)
    out.append(Tok(EOF, "", eof))
    return out


class Parser:
    """Recursive descent over `tokenize`'s output; one method per rule."""

    def __init__(self, src: str, filename: str) -> None:
        self.filename = filename
        self.toks = tokenize(src, filename)
        self.pos = 0
        self.where = "file"

    # Token helpers

    @property
    def tok(self) -> Tok:
        return self.toks[self.pos]

    def error(self, message: str, tok: Tok | None = None) -> RopError:
        tok = tok or self.tok
        return _fail(tok.span, message, self.where)

    def peek(self, text: str, ahead: int = 0) -> bool:
        tok = self.toks[min(self.pos + ahead, len(self.toks) - 1)]
        return tok.text == text and tok.kind != STR

    def take(self) -> Tok:
        tok = self.tok
        if tok.kind != EOF:
            self.pos += 1
        return tok

    def accept(self, text: str) -> Tok | None:
        if self.peek(text):
            return self.take()
        return None

    def expect(self, text: str) -> Tok:
        if not self.peek(text):
            found = self.tok.text or "end of file"
            raise self.error(f"expected '{text}', found '{found}'")
        return self.take()

    def ident(self, what: str = "an identifier") -> Tok:
        if self.tok.kind != ID:
            raise self.error(f"expected {what}, found '{self.tok.text or 'end of file'}'")
        return self.take()

    def qname(self) -> Tok:
        first = self.ident()
        parts = [first.text]
        last = first
        while self.peek(".") and self.toks[self.pos + 1].kind == ID:
            self.take()
            last = self.take()
            parts.append(last.text)
        span = SrcSpan(first.span.filename, first.span.line, first.span.column,
                       last.span.end_line, last.span.end_column)
        return Tok(ID, ".".join(parts), span)

    def nat(self) -> int:
        if self.tok.kind != NUM:
            raise self.error(f"expected a number, found '{self.tok.text}'")
        return int(self.take().text)

    def comma_list(self, close: str, item: Callable[[], X]) -> tuple[X, ...]:
        items: list[X] = []
        if not self.peek(close):
            items.append(item())
            while self.accept(","):
                items.append(item())
        self.expect(close)
        return tuple(items)

    def nonempty(self, close: str, item: Callable[[], X]) -> tuple[X, ...]:
        if self.peek(close):
            raise self.error("expected at least one item")
        return self.comma_list(close, item)

    def enum_word(self, catalog: type[E], what: str) -> tuple[E, Tok]:
        tok = self.ident(what)
        try:
            return catalog(tok.text), tok
        except ValueError:
            known = ", ".join(m.value for m in catalog)
            raise self.error(f"'{tok.text}' is not {what} (one of: {known})",
                             tok) from None

    # File

    def file(self) -> SourceFile:
        decls: list[Decl] = []
        while self.tok.kind != EOF:
            decls.append(self.decl())
        return SourceFile(self.filename, tuple(decls))

    def decl(self) -> Decl:
        word = self.tok.text
        self.where = "file"
        if word == "enum":
            return self.enum_decl()
        if word == "family":
            return self.family()
        if word == "point":
            return self.point()
        if word == "arm":
            return self.arm()
        if word == "macro":
            return self.macro()
        if word == "extern":
            return self.extern()
        raise self.error(f"expected a declaration (enum, family, point, arm, macro, "
                         f"extern), found '{word or 'end of file'}'")

    def enum_decl(self) -> EnumDecl:
        start = self.take()
        name = self.ident("an enum name")
        self.where = name.text
        self.expect("(")
        members = self.nonempty(")", lambda: self.ident("an enum member").text)
        self.expect(";")
        return EnumDecl(name.text, members, start.span)

    def family(self) -> Family:
        start = self.take()
        name = self.ident("a family name")
        self.where = name.text
        params = self.params()
        self.expect("exits")
        exits = self.names()
        generic: str | None = None
        if self.accept("generic"):
            self.expect("(")
            generic = self.ident("an arm name").text
            self.expect(")")
        events: tuple[MonitoringEvent, ...] = ()
        if self.accept("events"):
            self.expect("(")
            events = self.comma_list(
                ")", lambda: self.enum_word(MonitoringEvent, "a monitoring event")[0])
        vacant: tuple[str, ...] = ()
        if self.accept("vacant"):
            vacant = self.names()
        self.expect(";")
        return Family(name.text, params, exits, events, start.span, generic, vacant)

    def point(self) -> Point:
        start = self.take()
        name = self.qname()
        self.where = name.text
        self.expect("(")
        frame, _ = self.enum_word(FrameRole, "a frame role")
        self.expect(",")
        pc, _ = self.enum_word(PcRole, "a pc role")
        self.expect(",")
        exc, _ = self.enum_word(ExcRole, "an exception role")
        self.expect(")")
        lists = []
        for word in ("keep", "spent", "defs"):
            self.expect(word)
            lists.append(self.names())
        self.expect(";")
        keep, spent, defs = lists
        return Point(name.text, frame, pc, exc, keep, spent, defs, start.span)

    def arm(self) -> Arm:
        start = self.take()
        name = self.qname()
        self.where = name.text
        completion: str | None = None
        if self.accept("completion"):
            self.expect("(")
            completion = self.qname().text
            self.expect(")")
        region, _ = self.enum_word(Region, "a region")
        select: tuple[Select, ...] = ()
        if self.accept("select"):
            self.expect("(")
            select = self.comma_list(")", lambda: self.enum_word(Select, "a selection")[0])
        over = self.over() if self.peek("over") else None
        entry: str | None = None
        if self.accept("entry"):
            self.expect("(")
            entry = self.ident("a classifier case").text
            self.expect(")")
        observes = self.accept("observes") is not None
        trailing = self.accept("trailing") is not None
        gates = self.accept("gates") is not None
        chain: str | None = None
        if self.accept("chain"):
            self.expect("(")
            chain = self.ident("a chain role").text
            self.expect(")")
        contract = self.contract(exits=True)
        graph = self.graph()
        return Arm(name.text, region, contract, graph, start.span, select, entry, observes,
                   completion, trailing, gates, over, chain=chain)

    def over(self) -> Over:
        """`over(operand: M1, M2[; rest])`"""
        start = self.expect("over")
        self.expect("(")
        operand = self.ident("an enum operand").text
        self.expect(":")
        members = [self.ident("an enum member").text]
        while self.accept(","):
            members.append(self.ident("an enum member").text)
        rest = False
        if self.accept(";"):
            self.expect("rest")
            rest = True
        self.expect(")")
        return Over(operand, tuple(members), rest, start.span)

    def macro(self) -> Macro:
        start = self.take()
        name = self.ident("a macro name")
        self.where = name.text
        schemas = self.schema_params()
        params = self.params(schemas)
        result = self.result(schemas)
        contract = self.contract(exits=False, schemas=schemas)
        if self.accept("pending"):
            self.expect(";")
            graph = None
        else:
            graph = self.graph(schemas)
        return Macro(name.text, schemas, params, result, contract, graph, start.span)

    def extern(self) -> Extern:
        start = self.take()
        kind, _ = self.enum_word(RowKind, "a row kind")
        name = self.ident("a row name")
        self.where = name.text
        schemas = self.schema_params()
        params = self.params(schemas)
        result = self.result(schemas)
        contract = self.contract(exits=None, schemas=schemas)
        test: tuple[FactUse, ...] = ()
        if self.accept("test"):
            self.expect("(")
            test = self.nonempty(")", self.fact)
        binding: CBinding | None = None
        if not self.accept("pending"):
            c = self.expect("c")
            symbol = self.ident("a C symbol")
            self.expect("as")
            convention = self.ident("a C convention")
            inline = self.accept("inline") is not None
            evidence: tuple[EvidenceItem, ...] = ()
            if self.accept("evidence"):
                self.expect("(")
                evidence = self.nonempty(")", self.evidence_item)
            binding = CBinding(symbol.text, convention.text, evidence, c.span, inline)
        self.expect(";")
        return Extern(kind, name.text, schemas, params, result, contract, test,
                      binding, start.span)

    def evidence_item(self) -> EvidenceItem:
        key = self.ident("an evidence key")
        self.expect(":")
        if self.tok.kind != STR:
            raise self.error("expected a string")
        parts = []
        while self.tok.kind == STR:
            parts.append(self.take().text)
        return EvidenceItem(key.text, " ".join(parts), key.span)

    # Signatures

    def schema_params(self) -> tuple[str, ...]:
        if not self.accept("<"):
            return ()
        return self.nonempty(">", lambda: self.ident("a schema parameter").text)

    def params(self, schemas: tuple[str, ...] = ()) -> tuple[Param, ...]:
        self.expect("(")
        return self.comma_list(")", lambda: self.param(schemas))

    def param(self, schemas: tuple[str, ...] = ()) -> Param:
        name = self.ident("a parameter name")
        self.expect(":")
        return Param(name.text, self.type(schemas), name.span)

    def names(self) -> tuple[str, ...]:
        self.expect("(")
        return self.comma_list(")", lambda: self.qname().text)

    def result(self, schemas: tuple[str, ...]) -> Result:
        tok = self.tok
        if self.accept("->"):
            return Returns(self.type(schemas), tok.span)
        if self.accept("cases"):
            self.expect("(")
            return Cases(self.nonempty(")", lambda: self.case_decl(schemas)), tok.span)
        raise self.error("expected '-> type' or 'cases(...)'")

    def case_decl(self, schemas: tuple[str, ...]) -> CaseDecl:
        name = self.ident("a case name")
        return CaseDecl(name.text, self.params(schemas), name.span)

    def contract(self, exits: bool | None, schemas: tuple[str, ...] = ()) -> Contract:
        """`exits` True: an arm's contract, effects, feedback and exits
        required; False: a macro's, no exits (its cases are its exits);
        None: an extern's, feedback optional and no exits (a row's
        outcomes are its cases)."""
        start = self.expect("effects")
        self.expect("(")
        effects = self.comma_list(")", self.effect)
        feedback: tuple[Access, ...] = ()
        if exits is not None or self.peek("feedback"):
            self.expect("feedback")
            self.expect("(")
            feedback = self.comma_list(")", self.access)
        exit_names: tuple[str, ...] = ()
        if exits:
            self.expect("exits")
            exit_names = self.names()
        elif self.peek("exits"):
            what = "a macro's" if exits is False else "a row's"
            raise self.error(f"{what} exits are its cases: no 'exits' clause")
        borrows: tuple[str, ...] = ()
        spends: tuple[str, ...] = ()
        if not exits and self.accept("borrows"):
            borrows = self.names()
        if not exits and self.accept("spends"):
            spends = self.names()
        requires: tuple[FactUse, ...] = ()
        if self.accept("requires"):
            self.expect("(")
            requires = self.comma_list(")", self.fact)
        ensures: tuple[EdgeFact, ...] = ()
        if self.accept("ensures"):
            self.expect("(")
            ensures = self.comma_list(")", self.edge_fact)
        return Contract(effects, feedback, exit_names, requires, ensures, borrows, spends,
                        start.span)

    def effect(self) -> EffectDecl:
        effect, tok = self.enum_word(Effect, "an effect")
        on: tuple[str, ...] = ()
        if self.accept("on"):
            on = self.names()
        return EffectDecl(effect, on, tok.span)

    def access(self) -> Access:
        mode, tok = self.enum_word(AccessMode, "a feedback access")
        self.expect("(")
        field = self.qname()
        self.expect(")")
        return Access(mode, field.text, tok.span)

    def fact(self) -> FactUse:
        fact, tok = self.enum_word(Fact, "a fact")
        type_arg: BuiltinType | None = None
        if self.accept("<"):
            name = self.ident("a builtin type")
            if name.text not in BuiltinType.__members__:
                known = ", ".join(BuiltinType.__members__)
                raise self.error(f"'{name.text}' is not a builtin type (one of: "
                                 f"{known})", name)
            type_arg = BuiltinType[name.text]
            self.expect(">")
        self.expect("(")
        subjects = self.comma_list(")", lambda: self.qname().text)
        return FactUse(fact, subjects, type_arg, tok.span)

    def edge_fact(self) -> EdgeFact:
        edge = self.ident("an exit or case")
        self.expect(".")
        return EdgeFact(edge.text, self.fact(), edge.span)

    # Types

    def type(self, schemas: tuple[str, ...] = ()) -> Type:
        tok = self.ident("a type")
        word = tok.text
        if word in Scalar.__members__:
            return ScalarType(Scalar[word])
        if word in Handle.__members__:
            return HandleType(Handle[word])
        if word in Wrap.__members__:
            self.expect("<")
            arg = self.type(schemas)
            self.expect(">")
            return WrapType(Wrap[word], arg)
        if word == "Pair":
            self.expect("<")
            first = self.type(schemas)
            self.expect(",")
            second = self.type(schemas)
            self.expect(">")
            return PairType(first, second)
        if word in SchemaCtor.__members__:
            self.expect("<")
            schema = self.ident("a schema")
            self.expect(">")
            if schema.text in schemas:
                return SchemaType(SchemaCtor[word], SchemaVar(schema.text))
            if schema.text not in Schema.__members__:
                known = ", ".join(Schema.__members__)
                raise self.error(f"'{schema.text}' is not a schema (one of: {known})",
                                 schema)
            return SchemaType(SchemaCtor[word], Schema[schema.text])
        if word == "Enum":
            self.expect("<")
            name = self.ident("an enum name")
            self.expect(">")
            return EnumType(name.text)
        raise self.error(f"'{word}' is not a type of the catalog", tok)

    # Graph

    def graph(self, schemas: tuple[str, ...] = ()) -> Graph:
        start = self.expect("{")
        blocks = [self.block(schemas)]
        while not self.accept("}"):
            blocks.append(self.block(schemas))
        return Graph(tuple(blocks), start.span)

    def block(self, schemas: tuple[str, ...]) -> Block:
        label = self.ident("a block label")
        params = self.params(schemas)
        self.expect(":")
        stmts: list[Stmt] = []
        while True:
            word = self.tok.text
            if self.tok.kind == ID and word in ("guard", "let", "publish"):
                stmts.append(self.stmt(schemas))
            else:
                break
        term = self.term()
        return Block(label.text, params, tuple(stmts), term, label.span)

    def stmt(self, schemas: tuple[str, ...]) -> Stmt:
        tok = self.take()
        if tok.text == "guard":
            cond = self.expr()
            self.expect("else")
            self.expect("miss")
            self.expect(";")
            return Guard(cond, tok.span)
        if tok.text == "let":
            name = self.ident("a name")
            self.expect(":")
            ty = self.type(schemas)
            self.expect("=")
            value = self.expr()
            self.expect(";")
            return Let(name.text, ty, value, tok.span)
        self.expect("[")
        bindings = self.comma_list("]", self.binding)
        self.expect("retire")
        self.expect("[")
        debts = self.comma_list("]", self.debt)
        self.expect("at")
        point = self.qname()
        self.expect(";")
        return Publish(bindings, debts, point.text, tok.span)

    def binding(self) -> Binding:
        target = self.qname()
        if not self.accept("["):
            self.expect("=")
            return Binding(target.text, self.expr(), target.span)
        self.expect("]")
        self.expect("=")
        # A vector binding's value: `s[]`, `dup(s[])`, or a Span<O> value `v`
        dup = self.peek("dup") and self.peek("(", 1)
        if dup:
            call = self.take()
            self.expect("(")
        source = self.qname()
        value: Expr = Name(source.text, source.span)
        if not dup and not self.peek("["):
            return Binding(target.text, value, target.span, vector=True, owners=True)
        self.expect("[")
        self.expect("]")
        if dup:
            self.expect(")")
            value = Call("dup", (value,), call.span)
        return Binding(target.text, value, target.span, vector=True)

    def debt(self) -> Debt:
        kind, tok = self.enum_word(DebtKind, "a debt")
        self.expect("(")
        subject = self.qname()
        self.expect(")")
        return Debt(kind, subject.text, tok.span)

    def term(self) -> Term:
        tok = self.tok
        word = tok.text if tok.kind == ID else ""
        if word == "goto":
            self.take()
            target = self.target()
            self.expect(";")
            return Goto(target, tok.span)
        if word == "branch":
            self.take()
            cond = self.expr()
            self.expect("->")
            if_true = self.target()
            self.expect(",")
            if_false = self.target()
            self.expect(";")
            return Branch(cond, if_true, if_false, tok.span)
        if word == "call":
            self.take()
            callee = self.qname()
            args = self.args()
            self.expect("{")
            edges = [self.edge()]
            while not self.accept("}"):
                edges.append(self.edge())
            return CallTerm(callee.text, args, tuple(edges), tok.span)
        if word == "each":
            self.take()
            over = self.expr()
            self.expect("limit")
            limit: int | None = None
            if not self.accept("length"):
                limit = self.nat()
            self.expect("body")
            body = self.ident("a block label")
            self.expect("exhaust")
            exhaust = self.target()
            self.expect(";")
            return Each(over, limit, body.text, exhaust, tok.span)
        if word == "advance":
            self.take()
            body = self.ident("a block label")
            self.expect(";")
            return Advance(body.text, tok.span)
        if word == "exit":
            self.take()
            kind = self.ident("an exit")
            args = self.args()
            resume = complete = None
            if self.accept("resume"):
                resume = self.qname().text
                if self.accept("complete"):
                    complete = self.qname().text
            self.expect(";")
            return Exit(kind.text, args, tok.span, resume, complete)
        raise self.error("expected a statement or a terminator (goto, branch, call, "
                         f"each, advance, exit), found '{tok.text or 'end of file'}'")

    def edge(self) -> Edge:
        case = self.ident("a case name")
        self.expect("->")
        target = self.target()
        self.expect(";")
        return Edge(case.text, target, case.span)

    def target(self) -> Target:
        label = self.ident("a block label")
        return Target(label.text, self.args(), label.span)

    def args(self) -> tuple[Expr, ...]:
        self.expect("(")
        return self.comma_list(")", self.expr)

    def expr(self) -> Expr:
        tok = self.tok
        if tok.kind == NUM:
            return Nat(self.nat(), tok.span)
        if self.accept("["):
            return SpanLit(self.comma_list("]", self.expr), tok.span)
        if tok.kind == ID and tok.text in ("true", "false"):
            self.take()
            return BoolLit(tok.text == "true", tok.span)
        if tok.kind == ID and tok.text == "null" and not self.peek("(", 1):
            self.take()
            return Null(tok.span)
        name = self.qname()
        if self.peek("("):
            return Call(name.text, self.args(), name.span)
        return Name(name.text, name.span)


def parse(src: str, filename: str) -> SourceFile:
    return Parser(src, filename).file()


def parse_file(path: Path) -> SourceFile:
    data = path.read_bytes()
    try:
        src = data.decode("utf-8")
    except UnicodeDecodeError as e:
        prefix = data[:e.start]
        line = prefix.count(b"\n") + 1
        col = e.start - (prefix.rfind(b"\n") + 1) + 1
        raise _fail(SrcSpan(str(path), line, col, line, col + 1),
                    f"byte 0x{data[e.start]:02x} is not UTF-8") from None
    return parse(src, str(path))
