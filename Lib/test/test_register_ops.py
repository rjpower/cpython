"""Tests of the register instruction definition language's frontend:
Tools/cases_generator/rop_*.py and the definitions in Python/register_ops/."""

import unittest
from pathlib import Path

from test import test_tools

test_tools.skip_if_missing("cases_generator")
with test_tools.imports_under_tool("cases_generator"):
    import ir_contract
    import lexer
    import rop_check
    import rop_header
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
family F(dst:Dst, x:Src) exits(next, miss) ops(NOP);
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

    def test_family_clause(self):
        text = "family F(x:Src) exits(next) ops(BINARY_OP, COMPARE_OP) events(CALL, C_RETURN);"
        (family,) = rop_syntax.parse(text, "t.rop").decls
        self.assertEqual(family.ops, ("BINARY_OP", "COMPARE_OP"))
        self.assertEqual([e.value for e in family.events], ["CALL", "C_RETURN"])
        self.assertEqual(rop_print.print_file(rop_syntax.parse(text, "t.rop")).split(),
                         text.split())

    ERRORS = [
        ("family F(x:Src) exits(next) events(CALLS);", 1, 36,
         "'CALLS' is not a monitoring event (one of: PY_START, PY_RESUME, PY_RETURN, "
         "PY_YIELD, CALL, JUMP, BRANCH, BRANCH_LEFT, BRANCH_RIGHT, STOP_ITERATION, "
         "RAISE, EXCEPTION_HANDLED, PY_UNWIND, PY_THROW, RERAISE, C_RETURN, C_RAISE)"),
        ("family F(x:Src);", 1, 16, "expected 'exits', found ';'"),
        ("family F(x:Int) exits();", 1, 12, "'Int' is not a type of the catalog"),
        ("family F(x:Fb<Bin>) exits();", 1, 15,
         "'Bin' is not a schema (one of: Binary, Attr, Global, Truth, Compare, "
         "Contains, Item, Unpack, Iter, Send, Call, Super)"),
        ("arm F.A fast effects() feedback() exits() { entry(): exit next(); }", 1, 9,
         "'fast' is not a region (one of: inline, shared, split)"),
        ("arm F.A inline effects(eats) feedback() exits() { entry(): exit next(); }",
         1, 24, "'eats' is not an effect (one of: reads, writes, allocates, raises, "
         "releases, runs_python, polls, switches_frame)"),
        ("arm F.A inline effects() feedback() exits() { entry(): x = 1; }", 1, 56,
         "expected a statement or a terminator (goto, branch, call, each, advance, "
         "exit, tail), found 'x'"),
        ("macro m() -> Unit effects() feedback() exits() pending;", 1, 40,
         "a macro's exits are its cases: no 'exits' clause"),
        ("point F.P(current, here, clear) keep() spent() defs() held();", 1, 20,
         "'here' is not a pc role (one of: site, next_head, target, resume_head, "
         "saved_origin)"),
        ("extern helper h() -> Unit effects() c h as value evidence();", 1, 59,
         "expected at least one item"),
        ("arm F.A inline effects() feedback() exits() requires(exact<long>(x)) "
         "{ entry(): exit next(); }", 1, 60,
         "'long' is not a builtin type (one of: int, float, complex, bool, NoneType, "
         "str, bytes, bytearray, list, tuple, dict, set, frozenset, range, "
         "range_iterator, list_iterator, tuple_iterator, slice, function, method, "
         "builtin_function, method_descriptor, classmethod, property, cell, code, "
         "generator, coroutine, async_generator, type, module, super, object)"),
        ("frobnicate", 1, 1, "expected a declaration (enum, family, point, arm, macro, "
         "extern), found 'frobnicate'"),
    ]

    def test_syntax_errors(self):
        for src, line, column, message in self.ERRORS:
            with self.subTest(src=src):
                d = parse_error(src).diagnostic
                self.assertEqual((d.span.line, d.span.column, d.message),
                                 (line, column, message))


# A small family exercising every construct; each case below changes one
# piece of it and lists the diagnostics (line, column, message) that result
CHECK_BASE = """\
enum Op(PLUS, MINUS);
family F(dst:Dst, x:Src, y:Src, fb:Fb<Binary>)
  exits(next, miss, again, raise) ops(BINARY_OP);
point F.Done(current, next_head, clear) keep() spent(y, x) defs(dst) held();
point F.Failed(current, site, pending) keep(x, y) spent() defs() held();
extern primitive test2(a:B, b:B) -> Bool effects() test(inline_int(a), inline_int(b)) pending;
extern primitive scale(v:B) -> Scaled51 effects() pending;
extern helper box(v:Scaled51) cases(ok(z:O), err(e:Err)) effects(allocates, raises) pending;
extern helper tick<K>(f:Fb<K>) cases(quiet(), due(t:Ticket<K>)) effects(reads, writes) feedback(read(counter)) pending;
extern helper op(o:Enum<Op>, s:Span<Src>) -> Unit effects(writes) pending;
macro adapt(f:Fb<Binary>) cases(run(), changed(t:Ticket<Binary>)) effects(reads, writes) feedback(read(counter))
{
  entry():
    call tick(f) {
      quiet -> Run();
      due -> Changed();
    }
  Changed(t:Ticket<Binary>):
    exit changed(t);
  Run():
    exit run();
}
arm F.A inline effects(allocates, raises) feedback(read(counter)) exits(next, miss, raise)
{
  entry():
    let l:B = use(x);
    let r:B = use(y);
    guard test2(l, r) else miss;
    let _:Unit = op(PLUS, [x, y]);
    call box(scale(l)) {
      ok -> Done();
      err -> Failed();
    }
  Done(z:O):
    publish [dst = z] retire [spent(y), spent(x), old(dst)] at F.Done;
    exit next();
  Failed(e:Err):
    exit raise(F.Failed, e);
}
arm F.G shared effects(reads, writes) feedback(read(entry.kind)) exits(next, again)
{
  entry():
    call adapt(fb) {
      run -> Loop();
      changed -> Retry();
    }
  Retry(t:Ticket<Binary>):
    exit again(t);
  Loop():
    each [x, y] limit 2 body Body exhaust After();
  Body(s:Src, k:Size):
    let _:Unit = op(MINUS, [s]);
    advance Body;
  After():
    exit next();
}
"""

CHECK_CASES = [
    ('enum', 'o:Enum<Op>',
     'o:Enum<Opp>', [
         (10, 18, 'Enum<Opp>: no enum Opp is declared'),
         (29, 21, 'op: argument o is Enum<Opp>; PLUS is Enum<Op>'),
         (52, 21, 'op: argument o is Enum<Opp>; MINUS is Enum<Op>'),
     ]),
    ('macro publish', '  Run():\n    exit run();',
     '  Run():\n    publish [] retire [] at F.Done;\n    exit run();', [
         (21, 5, 'a macro does not publish; it returns a case and its arm publishes'),
     ]),
    ('shared leaves frame', 'feedback(read(entry.kind)) exits(next, again)',
     'feedback(read(entry.kind)) exits(next, again, return)', [
         (40, 16, 'exit return is not an exit of family F (next, miss, again, raise)'),
         (40, 16, 'a shared arm completes the word in a stub, which cannot leave the '
                  'frame (return): make it split'),
         (40, 16, 'declared exit return is not reached'),
     ]),
    ('observer', 'arm F.A inline',
     'arm F.Observed inline', [
         (23, 1, "an observed wrapper needs its family's events: F declares no "
                 "events(...)"),
         (23, 23, 'an observed wrapper adapts nothing (miss)'),
     ]),
    ('callee', 'box(scale(l))',
     'box(scael(l))', [
         (30, 14, 'no row, macro or primitive scael'),
     ]),
    ('name', 'r:B = use(y)',
     'r:B = use(yy)', [
         (27, 19, 'unknown name yy'),
     ]),
    ('point', 'at F.Done',
     'at F.Gone', [
         (35, 5, 'no point F.Gone'),
     ]),
    ('pointfam', 'point F.Failed(',
     'point H.Failed(', [
         (5, 1, "point H.Failed does not name a family's point (Family.Point)"),
         (38, 16, 'no point F.Failed'),
     ]),
    ('armfam', 'arm F.A ',
     'arm H.A ', [
         (23, 1, "arm H.A does not name a family's arm (Family.Arm)"),
     ]),
    ('fbfield', 'read(entry.kind)',
     'read(entry.knd)', [
         (40, 48, 'read(entry.knd): the schema has no field knd (fields: descr, kind)'),
     ]),
    ('fieldkind', 'fb:Fb<Binary>)\n',
     'fb:Name)\n', [
         (2, 1, "exit again needs a feedback operand: its ticket is over the family's schema"),
         (23, 52, 'read(counter): no feedback operand'),
         (40, 48, 'read(entry.kind): no feedback operand'),
         (43, 16, 'adapt: argument f is Fb<Binary>; fb is Name'),
     ]),
    ('role', 'defs(dst) held',
     'defs(x) held', [
         (4, 1, 'defs(x): operand x is Src, not a defs role'),
     ]),
    ('arity', 'test(inline_int(a), inline_int(b))',
     'test(inline_int(a, b))', [
         (6, 57, 'fact inline_int takes 1 subject, not 2'),
     ]),
    ('twice', 'let r:B = use(y);',
     'let l:B = use(y);', [
         (27, 5, 'l is assigned twice; first in block entry'),
         (28, 20, 'unknown name r'),
     ]),
    ('before', '    let l:B = use(x);\n    let r:B = use(y);\n    guard test2(l, r) else miss;',
     '    let l:B = use(x);\n    guard test2(l, r) else miss;\n    let r:B = use(y);', [
         (27, 20, 'r is used before its let in block entry'),
     ]),
    ('dominance', '  After():\n    exit next();',
     '  After():\n    let _:Size = k;\n    exit next();', [
         (55, 18, 'k is used in block After, which block Body (its definition) does not dominate'),
     ]),
    ('gotoarity', 'Done(z:O):',
     'Done(z:O, w:B):', [
         (31, 13, 'edge ok -> Done takes 2 arguments, not 1 (payload 1 + 0 given)'),
     ]),
    ('payload', 'Failed(e:Err):',
     'Failed(e:O):', [
         (32, 14, 'edge err -> Failed: payload value 0 is Err; parameter e is O'),
         (38, 26, 'e is O, expected Err'),
     ]),
    ('noedge', '      err -> Failed();\n',
     '', [
         (23, 16, 'declared exit raise is not reached'),
         (30, 5, 'call box at base.rop:30 has no edge for err'),
         (36, 3, 'block Failed is unreachable'),
     ]),
    ('unknowncase', 'err -> Failed',
     'error -> Failed', [
         (30, 5, 'call box at base.rop:30 has no edge for err'),
         (32, 7, 'call box at base.rop:30: box has no case error (cases: ok, err)'),
     ]),
    ('twoedges', '      err -> Failed();\n',
     '      err -> Failed();\n      err -> Failed();\n', [
         (33, 7, 'call box at base.rop:30 has two edges for err'),
     ]),
    ('callone', 'call box(scale(l))',
     'call scale(l)', [
         (30, 5, 'call scale at base.rop:30: scale has one result: use let'),
     ]),
    ('cycle', '  After():\n    exit next();',
     '  After():\n    goto Loop();', [
         (40, 16, 'declared exit next is not reached'),
         (55, 5, 'edge After -> Loop closes a cycle; only each/advance may loop'),
     ]),
    ('advanceout', '  After():\n    exit next();',
     '  After():\n    advance Body;', [
         (40, 16, 'declared exit next is not reached'),
         (55, 5, 'advance Body from block After, which is outside the body of Body'),
     ]),
    ('intobody', '    exit again(t);',
     '    goto Body(x, 0);', [
         (40, 16, 'declared exit again is not reached'),
         (48, 5, 'block Body is the body of the each at base.rop:50; only each and advance enter it'),
     ]),
    ('bodyparams', 'Body(s:Src, k:Size)',
     'Body(s:Src, k:U8)', [
         (51, 3, 'each body Body takes (Src, Size), not (Src, U8)'),
     ]),
    ('recursion', '}\narm F.A',
     '}\nmacro m(f:Fb<Binary>) cases(run()) effects() feedback() { entry(): call m(f) { run -> R(); } R(): exit run(); }\narm F.A', [
         (23, 1, 'macro expansion recurses: m -> m'),
     ]),
    ('undeclaredexit', '    exit next();\n  Failed',
     '    exit jump(next_head);\n  Failed', [
         (23, 16, 'declared exit next is not reached'),
         (36, 5, 'exit jump: jump is not a declared exit of F.A (miss, next, raise)'),
     ]),
    ('unreached', 'exits(next, miss, raise)',
     'exits(next, miss, raise, again)', [
         (23, 16, 'declared exit again is not reached'),
     ]),
    ('noops', ' ops(BINARY_OP)', '', [
         (2, 1, 'a family with arms names the stack ops that lower to it: '
                'ops(NAME, ...)'),
     ]),
    ('twoops', 'ops(BINARY_OP)', 'ops(BINARY_OP, BINARY_OP)', [
         (2, 1, 'a stack op is listed twice'),
     ]),
    ('twoevents', 'ops(BINARY_OP)', 'ops(BINARY_OP) events(CALL, CALL)', [
         (2, 1, 'an event is listed twice'),
     ]),
    ('notfamily', 'exits(next, again)\n',
     'exits(next, again, redirect)\n', [
         (40, 16, 'exit redirect is not an exit of family F (next, miss, again, raise)'),
         (40, 16, 'declared exit redirect is not reached'),
     ]),
    ('nomiss', 'exits(next, miss, raise)',
     'exits(next, raise)', [
         (28, 5, 'guard ... else miss where miss is not a declared exit'),
     ]),
    ('exitpayload', 'exit raise(F.Failed, e)',
     'exit raise(F.Failed, l)', [
         (38, 26, 'l is B, expected Err'),
     ]),
    ('ticket', 'Retry(t:Ticket<Binary>)',
     'Retry(t:Ticket<Attr>)', [
         (45, 18, 'edge changed -> Retry: payload value 0 is Ticket<Binary>; parameter t is Ticket<Attr>'),
         (48, 16, 't is Ticket<Attr>, expected Ticket<Binary>'),
     ]),
    ('action', 'scale(v:B) -> Scaled51 effects()',
     'scale(v:B) -> Scaled51 effects(writes)', [
         (30, 14, 'scale(...) is an action (effects writes): only the outermost expression of a let'),
     ]),
    ('argtype', 'box(scale(l))',
     'box(scale(x))', [
         (30, 20, 'scale: argument v is B; x is Src'),
     ]),
    ('lettype', 'let l:B = use(x)',
     'let l:O = use(x)', [
         (26, 15, 'use(x) is B, expected O'),
         (28, 17, 'test2: argument a is B; l is O'),
         (30, 20, 'scale: argument v is B; l is O'),
     ]),
    ('discard', 'let _:Unit = op(PLUS, [x, y]);',
     'let _:Err = op(PLUS, [x, y]);', [
         (29, 5, '_ discards only Unit or a scalar, not Err'),
         (29, 17, 'op(PLUS, [x, y]) is Unit, expected Err'),
     ]),
    ('casesexpr', 'let _:Unit = op(PLUS, [x, y]);',
     'let _:Unit = box(scale(l));', [
         (29, 18, 'box has cases (ok, err): use a call terminator'),
     ]),
    ('nat', 'op(PLUS, [x, y])',
     'op(1, [x, y])', [
         (29, 21, 'number 1 needs a numeric context'),
     ]),
    ('publishdst', '[dst = z]',
     '[x = z]', [
         (35, 14, 'publish [x = ...]: x is not a destination operand'),
     ]),
    ('spent', 'spent(y), spent(x)',
     'spent(dst), spent(x)', [
         (35, 31, 'spent(dst): dst is not a Src operand'),
     ]),
    ('schema', 'macro adapt(f:Fb<Binary>)',
     'macro adapt(f:Fb<Attr>)', [
         (16, 14, 'edge due -> Changed: payload value 0 is Ticket<Attr>; parameter t is Ticket<Binary>'),
         (43, 16, 'adapt: argument f is Fb<Attr>; fb is Fb<Binary>'),
     ]),
    ('evidence', 'scale(v:B) -> Scaled51 effects() pending',
     'scale(v:B) -> Scaled51 effects() c _PyValue_IntScaled as scalar', [
         (7, 51, 'c _PyValue_IntScaled needs evidence: review, contract, lowering, test'),
     ]),
    ('convention', 'effects(allocates, raises) pending',
     'effects(allocates, raises) c _PyValue_BoxInt64 as scalar evidence(review: "r", contract: "c")', [
         (8, 85, 'convention scalar maps onto a -> T row'),
     ]),
    ('raises', 'scale(v:B) -> Scaled51 effects()',
     'scale(v:B) -> Scaled51 effects(raises)', [
         (7, 1, 'a row with one outcome cannot raise: declare its error as a case'),
         (30, 14, 'scale(...) is an action (effects raises): only the outermost expression of a let'),
     ]),
    ('dup', 'enum Op(PLUS, MINUS);',
     'enum Op(PLUS, MINUS);\nenum Op(TIMES);', [
         (2, 1, 'Op is declared twice; first at base.rop:1'),
     ]),
    ('entry', '  entry():\n    let l',
     '  start():\n    let l', [
         (25, 3, 'the first block is entry()'),
     ]),
    ('macroguard', '  Run():\n    exit run();',
     '  Run():\n    guard test2(use(x), use(x)) else miss;\n    exit run();', [
         (21, 5, 'guard ... else miss in a macro, whose exits are its cases'),
         (21, 21, 'unknown name x'),
         (21, 29, 'unknown name x'),
     ]),
    ('eachover', 'each [x, y] limit',
     'each fb limit', [
         (50, 5, 'each over fb: Fb<Binary> is not a Span or Rows'),
     ]),
    ('effecton', 'effects(allocates, raises) feedback(read(counter)) exits',
     'effects(allocates on (z), raises) feedback(read(counter)) exits', [
         (23, 24, 'allocates on (z): no operand z'),
     ]),
    ('evidencekey', 'scale(v:B) -> Scaled51 effects() pending',
     'scale(v:B) -> Scaled51 effects() c s as scalar evidence(review: "r", contract: "c", lowering: "l", test: "t", why: "w")', [
         (7, 128, 'evidence why is not one of: review, contract, lowering, test'),
     ]),
    ('testbool', 'scale(v:B) -> Scaled51 effects() pending',
     'scale(v:B) -> Scaled51 effects() test(inline_int(v)) pending', [
         (7, 1, 'test(...) is the true edge of a -> Bool row'),
     ]),
    ('action_prim', 'call box(scale(l))',
     'call box(scale(dup(l)))', [
         (30, 20, 'dup(...) is an action: only the outermost expression of a let'),
         (30, 20, 'scale: argument v is B; dup(l) is O'),
     ]),
    ('ambiguous', 'enum Op(PLUS, MINUS);',
     'enum Op(PLUS, MINUS);\nenum Sign(PLUS);', [
         (30, 21, 'PLUS is a member of Op, Sign: qualify it'),
     ]),
    ('nofield', 'fb:Fb<Binary>)\n',
     'fb:Fb<Binary>, o:Obj)\n', [
         (2, 48, 'operand o:Obj has no field kind'),
     ]),
    ('wideimm', 'fb:Fb<Binary>)\n',
     'fb:Fb<Binary>, n:U64)\n', [
         (2, 48, 'operand n:U64 needs 64 bits; an immediate has at most 32'),
     ]),
    ('acrosswords', 'y:Src, fb:Fb<Binary>)',
     'y:Src, n:U16, fb:Fb<Binary>)', [
         (2, 33, 'operand n:U16 needs 16 bits; field D and the next field of its word cannot hold it'),
     ]),
    ('toomany', 'fb:Fb<Binary>)\n',
     'fb:Fb<Binary>, a1:Src, a2:Src, a3:Src, a4:Src, a5:Src, a6:Src, a7:Src, a8:Src, a9:Src)\n', [
         (2, 112, 'operand a9: every field is taken'),
     ]),
    ('unreachable', '  Failed(e:Err):\n    exit raise(F.Failed, e);',
     '  Failed(e:Err):\n    exit raise(F.Failed, e);\n  Spare():\n    exit next();', [
         (39, 3, 'block Spare is unreachable'),
     ]),
]


def check_source(src: str) -> list[tuple[int, str]]:
    program = rop_check.check([rop_syntax.parse(src, "base.rop")])
    return [(d.span.line, d.span.column, d.message) for d in program.diagnostics]


class CheckTests(unittest.TestCase):
    def test_base_is_clean(self):
        self.assertEqual(check_source(CHECK_BASE), [])

    def test_negative_fixtures(self):
        for name, old, new, expected in CHECK_CASES:
            with self.subTest(name):
                self.assertIn(old, CHECK_BASE)
                found = check_source(CHECK_BASE.replace(old, new, 1))
                self.assertEqual(sorted(found), sorted(expected))

    def test_diagnostic_format(self):
        src = CHECK_BASE.replace("      err -> Failed();\n", "", 1)
        program = rop_check.check([rop_syntax.parse(src, "add.rop")])
        self.assertIn("add.rop:30:5: F.A: call box at add.rop:30 has no edge for err",
                      [str(d) for d in program.diagnostics])

    # Targets, typed facts
    MORE_CASES = [
        ("family F(a:Src, b:Src, c:Src, u:FarHead) exits(next);",
         [(1, 31, "operand u:FarHead needs 24 bits; field D and the next field of its "
           "word cannot hold it")]),
        ("family F(t:NearHead, u:FarHead) exits(branch, jump) ops(NOP);\n"
         "arm F.A inline effects() feedback() exits(branch, jump)\n"
         "{ entry(): branch true -> N(), J(); N(): exit branch(t); J(): exit jump(u); }",
         []),
        ("family F(t:NearHead) exits(branch) ops(NOP);\n"
         "arm F.A inline effects() feedback() exits(branch)\n"
         "{ entry(): let h:NearHead = t; exit branch(h); }",
         [(3, 29, "t is Head, expected NearHead"),
          (3, 44, "h is NearHead, expected Head")]),
        ("extern helper h(v:B) -> O effects() ensures(result.exact(result)) pending;",
         [(1, 52, "fact exact names a builtin type: exact<T>(...)")]),
        ("extern helper h(v:B) -> O effects() ensures(result.object<int>(result)) "
         "pending;",
         [(1, 52, "fact object takes no type argument")]),
        ("extern helper h(v:B) -> O effects() ensures(result.exact<frozenset>(result)) "
         "pending;", []),
        # An optional owner may bind an optional destination, not a plain one
        ("family F(d:Dst, e:Maybe<Dst>, x:Src) exits(next) ops(NOP);\n"
         "point F.Done(current, next_head, clear) keep() spent(x) defs(d, e) held();\n"
         "extern helper h(v:B) -> Maybe<O> effects() pending;\n"
         "arm F.A inline effects(writes, releases) feedback() exits(next)\n"
         "{ entry(): let m:Maybe<O> = h(use(x));\n"
         "  publish [e = m] retire [spent(x), old(e)] at F.Done; exit next(); }",
         []),
        ("family F(d:Dst, x:Src) exits(next) ops(NOP);\n"
         "point F.Done(current, next_head, clear) keep() spent(x) defs(d) held();\n"
         "extern helper h(v:B) -> Maybe<O> effects() pending;\n"
         "arm F.A inline effects(writes, releases) feedback() exits(next)\n"
         "{ entry(): let m:Maybe<O> = h(use(x));\n"
         "  publish [d = m] retire [spent(x), old(d)] at F.Done; exit next(); }",
         [(6, 16, "m is Maybe<O>, expected O")]),
        # Parameter modes: borrows names a linear parameter, spends a source
        # one, and spending releases
        ("extern helper h(a:Anchors, s:Span<Src>, e:Err) -> Err effects(releases) "
         "borrows(a) spends(s) pending;", []),
        ("extern helper h(v:B, s:Src) -> Unit effects(releases) borrows(v) spends(v) "
         "pending;",
         [(1, 37, "borrows(v): v is not a linear parameter"),
          (1, 37, "spends(v): v is not a source parameter")]),
        ("macro m(s:Span<Src>) -> Unit effects() feedback() spends(s) pending;",
         [(1, 30, "spends(s): spending a source releases its owner; declare releases")]),
    ]

    def test_targets_and_typed_facts(self):
        for src, expected in self.MORE_CASES:
            with self.subTest(src):
                program = rop_check.check([rop_syntax.parse(src, "t.rop")])
                self.assertEqual([(d.span.line, d.span.column, d.message)
                                  for d in program.diagnostics], expected)

    def test_linear_types(self):
        handles = rop_check.HandleType
        self.assertTrue(rop_check.is_linear(handles(rop_check.Handle.Route)))
        self.assertTrue(rop_check.is_linear(rop_check.SchemaType(
            rop_check.SchemaCtor.Ticket, rop_check.Schema.Binary)))
        self.assertFalse(rop_check.is_linear(rop_check.SchemaType(
            rop_check.SchemaCtor.Candidate, rop_check.Schema.Binary)))

    def test_graph_info(self):
        program = rop_check.check([rop_syntax.parse(CHECK_BASE, "base.rop")])
        info = program.graphs["F.G"]
        self.assertEqual(info.order, ("entry", "Retry", "Loop", "After", "Body"))
        self.assertEqual(info.idom["Body"], "Loop")
        self.assertEqual(info.preds["After"], ("Loop",))
        self.assertEqual(info.exits_used, {"next", "again"})
        self.assertEqual(rop_print.type_text(info.types["k"]), "Size")
        self.assertEqual(rop_print.type_text(info.types["fb"]), "Fb<Binary>")

    ENCODINGS = [
        ("family F(d:Dst, x:Src, y:Src, f:Fb<Binary>) exits(next);", 0,
         [("d", "A"), ("x", "B"), ("y", "C"), ("f", "D")]),
        ("family F(d:Dst, k:Bool, i:U16) exits(next);", 0,
         [("d", "A"), ("k", "B"), ("i", "C+D")]),
        # A span's count takes a field, like an argument descriptor's
        ("family F(d:Dst, s:Span<Src>, a:Src, b:Src, c:Src, n:Name, i:U16) exits(next);", 1,
         [("d", "A"), ("s", "B"), ("a", "C"), ("b", "D"), ("c", "P0"), ("n", "P1"),
          ("i", "P2")]),
        ("family F(t:NearHead, m:Maybe<U8>, a:Src, b:Src, w:U32) exits(next);", 1,
         [("t", "A"), ("m", "B"), ("a", "C"), ("b", "D"), ("w", "P0+P1")]),
        # A near target takes one field, a far one two of the same word
        ("family F(t:NearHead, u:FarHead, s:Src) exits(next);", 0,
         [("t", "A"), ("u", "B+C"), ("s", "D")]),
        ("family F(a:Src, b:Src, c:Src, d:Src, u:FarHead) exits(next);", 1,
         [("a", "A"), ("b", "B"), ("c", "C"), ("d", "D"), ("u", "P0+P1")]),
        # An argument descriptor's count takes a field; its homes follow
        ("family F(d:Dst, fn:Src, a:Args, f:Fb<Call>) exits(next);", 0,
         [("d", "A"), ("fn", "B"), ("a", "C"), ("f", "D")]),
    ]

    def test_derived_encoding(self):
        for src, words, expected in self.ENCODINGS:
            with self.subTest(src):
                program = rop_check.check([rop_syntax.parse(src, "t.rop")])
                self.assertEqual(program.diagnostics, ())
                encoding = program.encodings["F"]
                self.assertEqual(encoding.words, words)
                self.assertEqual([(p.operand, "+".join(s.name for s in p.slots))
                                  for p in encoding.placements], expected)


class DefinitionTests(unittest.TestCase):
    """The checked-in definitions."""

    @classmethod
    def setUpClass(cls):
        cls.program = rop_check.load(rop_files())

    def test_definitions_check(self):
        self.assertEqual([str(d) for d in self.program.diagnostics], [])

    def test_pilot(self):
        p = self.program
        self.assertEqual(sorted(a.name for a in p.arms_of("Add")),
                         ["Add.Extended", "Add.Float", "Add.Generic", "Add.Int", "Add.Text"])
        self.assertIn("GetAttr.Instance", p.arms)
        self.assertIsNotNone(p.macros["adapt_before_use_add"].graph)
        self.assertIsNotNone(p.macros["binary_at_origin"].graph)

    def test_pending_rows(self):
        pending = set(self.program.pending)
        for name in ("getattr_at_origin", "getmethod_at_origin"):
            self.assertNotIn(name, pending)
            self.assertIsNotNone(self.program.macros[name].graph)
        for x in self.program.externs.values():
            if x.c is not None:
                self.assertNotIn(x.name, pending)

    def test_header_agreement(self):
        rows = rop_header.header_rows()
        self.assertEqual(rop_header.disagreements(self.program.externs, rows), [])
        box = self.program.externs["box_i64"]
        changed = {box.c.symbol: ir_contract.Row(
            "HELPER", box.c.symbol, effects=frozenset({"allocates"}),
            exits=frozenset({"miss"}))}
        found = [d.message for d in rop_header.disagreements(self.program.externs, changed)]
        self.assertEqual(found, [
            "c _PyValue_BoxInt64: effects (allocates, raises) disagree with header "
            "row HELPER(_PyValue_BoxInt64)'s (allocates)",
            "c _PyValue_BoxInt64: exits () disagree with header row "
            "HELPER(_PyValue_BoxInt64)'s (miss)",
        ])

    # Appendix A of the addendum, in its order
    APPENDIX_A = """
        Nop Const LocalRead LocalWrite SaveLocal LocalDelete Move Add Truth Jump
        BranchTruth BranchNone Binary Unary Compare Identity Contains TestBranch
        GetAttr GetMethod AttrWrite SuperLookup SpecialLookup
        GetItem SetItem DelItem SliceRead SliceWrite BuildSlice Build AddElement
        UpdateContainer MergeKeywords Unpack Cell NamespaceRead NamespaceWrite
        GlobalRead SetupAnnotations Import BuildText Convert Format Length Pattern
        Function FunctionSet Intrinsic IntrinsicPair
        Call InitResult Return CallKw CallEx MaterializeKwargs IterOpen ForIter
        AsyncIter Awaitable Send Yield Raise Reraise ExceptState ExceptMatch
        CleanupThrow EndAsyncFor WithExit Resume Poll
    """.split()

    def test_appendix_a_families(self):
        self.assertEqual(len(self.APPENDIX_A), 68)
        self.assertEqual(sorted(self.program.families), sorted(self.APPENDIX_A))
        for fam in self.program.families.values():
            with self.subTest(fam.name):
                encoding = self.program.encodings[fam.name]
                self.assertEqual(len(encoding.placements), len(fam.params))
                self.assertTrue(fam.exits)

    def test_no_stack_notions(self):
        words = ("pop", "push", "stack", "unused", "window", "uop", "replaces",
                 "exchange")
        for path in rop_files():
            text = path.read_text().lower()
            for word in words:
                with self.subTest(path=path.name, word=word):
                    self.assertNotRegex(text, rf"\b{word}\b")


if __name__ == "__main__":
    unittest.main()
