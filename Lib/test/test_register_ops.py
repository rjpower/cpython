"""Tests of the register instruction definition language's frontend:
Tools/cases_generator/rop_*.py and the definitions in Python/register_ops/."""

import unittest
from pathlib import Path

from test import test_tools

test_tools.skip_if_missing("cases_generator")
with test_tools.imports_under_tool("cases_generator"):
    import lexer
    import rop_print
    import rop_syntax
    from rop_model import RopError

ROP_DIR = Path(test_tools.basepath) / "Python" / "register_ops"


def rop_files() -> list[Path]:
    return sorted(ROP_DIR.glob("*.rop"))


def lex_error(src: str) -> RopError:
    try:
        rop_syntax.tokenize(src, "t.rop")
    except RopError as e:
        return e
    raise AssertionError(f"{src!r} lexed")


def parse_error(src: str) -> RopError:
    try:
        rop_syntax.parse(src, "t.rop")
    except RopError as e:
        return e
    raise AssertionError(f"{src!r} parsed")


MINIMAL = """\
family F(dst:Dst, x:Src) layout HH exits(next, miss);
arm F.A inline effects() feedback() exits(next, miss)
{
  entry():
    let v:B = use(x);
    guard is_object(v) else miss;
    exit next();
}
"""


class LexTests(unittest.TestCase):
    # Characters that lexer.py skips without a token: the wrapper refuses
    # each, at its position
    UNCONSUMED = [
        ("a\rb", 1, 2), ("a\x0bb", 1, 2), ("a\x0cb", 1, 2), ("a\xa0b", 1, 2),
        ("a b", 1, 2), ("a\n  b\r", 2, 4),
    ]

    def test_lexer_skips_them(self):
        for src, _, _ in self.UNCONSUMED:
            with self.subTest(src=src):
                self.assertEqual([t.text for t in lexer.tokenize(src)], ["a", "b"])

    def test_unconsumed_characters_fail(self):
        for src, line, column in self.UNCONSUMED:
            with self.subTest(src=src):
                e = lex_error(src)
                self.assertEqual((e.diagnostic.span.line, e.diagnostic.span.column),
                                 (line, column))
                self.assertIn("is not consumed by any token", e.diagnostic.message)

    REFUSED = [
        ("a @ b", "Bad token: @"),
        ("#define X 1\n", "'#define X 1\\n' is not a token of the language"),
        ("x + y", "'+' is not a token of the language"),
        ("0x10", "number 0x10 is not an unsigned decimal"),
        ("1.5", "number 1.5 is not an unsigned decimal"),
        ("10u", "number 10u is not an unsigned decimal"),
        ("'c'", "\"'c'\" is not a token of the language"),
        ('"a\\n"', "a string has no escapes"),
    ]

    def test_refused_tokens(self):
        for src, message in self.REFUSED:
            with self.subTest(src=src):
                self.assertEqual(lex_error(src).diagnostic.message, message)

    def test_trivia_and_positions(self):
        toks = rop_syntax.tokenize("/* c\n */ goto // x\n\tFb<Fb<K>>=", "t.rop")
        self.assertEqual([t.text for t in toks],
                         ["goto", "Fb", "<", "Fb", "<", "K", ">", ">", "=", ""])
        self.assertEqual([(t.span.line, t.span.column) for t in toks[:2]], [(2, 5), (3, 2)])
        self.assertEqual((toks[7].span.line, toks[7].span.column), (3, 10))


class ParseTests(unittest.TestCase):
    def test_minimal_arm(self):
        f = rop_syntax.parse(MINIMAL, "t.rop")
        family, arm = f.decls
        self.assertEqual(family.name, "F")
        self.assertEqual(arm.name, "F.A")
        (entry,) = arm.graph.blocks
        self.assertEqual(entry.label, "entry")
        self.assertEqual(len(entry.stmts), 2)

    def test_definitions_parse(self):
        files = rop_files()
        self.assertTrue(files)
        for path in files:
            with self.subTest(path=path.name):
                rop_syntax.parse_file(path)

    def test_printer_round_trips(self):
        for path in rop_files():
            with self.subTest(path=path.name):
                parsed = rop_syntax.parse_file(path)
                text = rop_print.print_file(parsed)
                self.assertEqual(rop_syntax.parse(text, str(path)), parsed)
                self.assertEqual(rop_print.print_file(rop_syntax.parse(text, "x")), text)

    ERRORS = [
        ("family F(x:Src) layout HH;", 1, 26, "expected 'exits', found ';'"),
        ("family F(x:Int) layout HH exits();", 1, 12, "'Int' is not a type of the catalog"),
        ("family F(x:Fb<Bin>) layout H exits();", 1, 15,
         "'Bin' is not a schema (one of: Binary, Attr, Global, Truth, Compare, "
         "Contains, Item, Unpack, Iter, Send, Call, Super)"),
        ("arm F.A fast effects() feedback() exits() { entry(): exit next(); }", 1, 9,
         "'fast' is not a region (one of: inline, shared, split)"),
        ("arm F.A inline effects(eats) feedback() exits() { entry(): exit next(); }",
         1, 24, "'eats' is not an effect (one of: reads, writes, allocates, raises, "
         "releases, runs_python, polls, switches_frame)"),
        ("arm F.A inline effects() feedback() exits() { entry(): x = 1; }", 1, 56,
         "expected a statement or a terminator (goto, branch, call, each, advance, "
         "exit), found 'x'"),
        ("macro m() -> Unit effects() feedback() exits() pending;", 1, 40,
         "a macro's exits are its cases: no 'exits' clause"),
        ("point F.P(current, here, clear) keep() spent() defs() held();", 1, 20,
         "'here' is not a pc role (one of: site, next_head, target, resume_head, "
         "saved_origin)"),
        ("extern helper h() -> Unit effects() c h as value evidence();", 1, 59,
         "expected at least one item"),
        ("frobnicate", 1, 1, "expected a declaration (enum, family, point, arm, macro, "
         "extern), found 'frobnicate'"),
    ]

    def test_syntax_errors(self):
        for src, line, column, message in self.ERRORS:
            with self.subTest(src=src):
                d = parse_error(src).diagnostic
                self.assertEqual((d.span.line, d.span.column, d.message),
                                 (line, column, message))


if __name__ == "__main__":
    unittest.main()
