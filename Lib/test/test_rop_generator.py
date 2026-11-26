"""Tests of the generated products of the register instruction definitions:
Tools/cases_generator/rop_cgen.py and rop_generator.py."""

import dataclasses
import io
import re
import unittest
from unittest import mock
from pathlib import Path

from test import support, test_tools
from test.support import os_helper

test_tools.skip_if_missing("cases_generator")
with test_tools.imports_under_tool("cases_generator"):
    import handler_table
    import rop_primitives
    import rop_catalog
    import rop_check
    import rop_generator
    import rop_leaf
    import rop_model
    import rop_syntax
    import rop_verify

ROOT = Path(test_tools.basepath)


def products() -> "rop_generator.Products":
    program = rop_check.load(sorted((ROOT / "Python" / "register_ops").glob("*.rop")))
    assert not program.diagnostics
    numbers = rop_generator.number_arms(program)
    return rop_generator.lower_all(program, numbers)


def int51_boundaries(shift: int) -> list[int]:
    top = 1 << 50
    return sorted({v for base in (-top, top - 1, 0, 1, -1, top // 2, -top // 2)
                   for v in (base - 1, base, base + 1)
                   if -top <= v < top})


class AddCheckedScaledTests(unittest.TestCase):
    """The C definition of the row against Python integers"""

    def test_c_against_python(self):
        testcapi = support.import_helper.import_module("_testinternalcapi")
        shift = testcapi.leaf_layout()[0]["Py_INT_SHIFT"]
        values = int51_boundaries(shift)
        self.assertGreater(len(values), 10)
        for x in values:
            for y in values:
                with self.subTest(x=x, y=y):
                    exact = x + y
                    overflowed, out = testcapi.leaf_primitive("_PyValue_AddCheckedScaled", x << shift, y << shift)
                    fits = -(1 << 50) <= exact < (1 << 50)
                    self.assertEqual(overflowed, not fits)
                    self.assertEqual(out, exact << shift if fits else exact)


def hot_sequence(lines):
    """The calls of a template's hot path, by name: up to the cold section,
    without labels, register choices or arguments"""
    names = []
    for line in lines:
        line = line.strip()
        if line.startswith("_PyJitLower_ColdBegin"):
            break
        if m := re.match(r"(?:int c\d+ = )?(_PyJit\w+)\(", line):
            if "NewLabel" not in m[1] and "Bind" not in m[1]:
                names.append(m[1])
        elif line.startswith("if ("):
            names.append(line.rstrip(" {"))
    return names


class AddIntTemplateTests(unittest.TestCase):
    def test_hot_path(self):
        # The old BINARY_OP_ADD_INT template's hot path, with its add and the
        # branch on overflow spelled as the primitive that is both
        prog = products().program
        arm = prog.arms["Add.Int"]
        new = rop_leaf.template_of_arm(prog, arm, "Add_Int")
        self.assertEqual(hot_sequence(new), [
            "_PyJitAsm_MovRM", "_PyJitAsm_MovRM",
            "_PyJitLower_PyValue_BothInlineInt", "_PyJitAsm_Jcc",
            "_PyJitLower__PyValue_IntScaled", "_PyJitLower__PyValue_IntScaled",
            "_PyJitLower__PyValue_AddCheckedScaled",
            "_PyJitLower__PyValue_FromIntScaled",
            "if (!(ir_word & IR_F_LOCAL_B))", "_PyJitAsm_MovMI",
            "if (!(ir_word & IR_F_LOCAL_C))", "_PyJitAsm_MovMI",
            "if ((ir_word & IR_F_LOCAL_A))", "_PyJitAsm_MovRM", "_PyJitAsm_MovMR",
            "if ((ir_word & IR_F_LOCAL_A))", "_PyJitLower_PyValue_XCLOSE"])


SHARED = """\
family F(dst:Dst, x:Src) exits(next, miss) ops(NOP);
point F.Done(current, next_head, clear) keep() spent(x) defs(dst);
extern primitive is_int(v:B) -> Bool
  effects()
  test(inline_int(v))
  c PyValue_IsInlineInt as bool
  evidence(review: "r", contract: "c", lowering: "l", test: "t");
arm F.Copy shared effects(writes, releases) feedback() exits(next, miss)
{
  entry():
    let v:B = use(x);
    guard is_int(v) else miss;
    let z:O = dup(v);
    publish [dst = z] retire [spent(x), old(dst)] at F.Done;
    exit next();
}
"""


# An arm rop_check accepts and rop_verify refuses: the owner it acquires
# leaks and no publish names a point (RV's g01)
UNVERIFIED = """\
family Leak(x:Src) exits(next) ops(NOP);
arm Leak.Owner split effects(reads, writes) feedback() exits(next) {
  entry():
    let o:O = acquire(x);
    exit next();
}
"""


INLINE = SHARED.replace(
    "extern primitive is_int(v:B) -> Bool\n  effects()\n  test(inline_int(v))\n"
    "  c PyValue_IsInlineInt as bool\n",
    "extern helper is_int(v:B) -> Bool\n  effects()\n  test(inline_int(v))\n"
    "  c PyValue_IsInlineInt as bool inline\n")


class InlineRowTests(unittest.TestCase):
    """`c SYMBOL as CONVENTION inline`: the interpreter calls the row's own
    symbol, the forms and stubs one out-of-line copy"""

    def generated(self, text=INLINE):
        import io
        program = rop_check.check([rop_syntax.parse(text, "t.rop")])
        self.assertEqual(program.diagnostics, ())
        p = rop_generator.lower_all(program, rop_generator.number_arms(program))
        out = {}
        for name in ("cases", "forms", "stubs"):
            out[name] = io.StringIO()
            getattr(rop_generator, f"write_{name}")(out[name], p, [])
        return {k: v.getvalue() for k, v in out.items()}

    def test_the_products_call_the_symbol_they_link(self):
        text = self.generated()
        self.assertIn("PyValue_IsInlineInt(", text["cases"])
        self.assertNotIn("PyValue_IsInlineInt_Outlined", text["cases"])
        # the one plain call in the forms is the outlined copy's own
        forms = text["forms"].partition("{ return PyValue_IsInlineInt(p0); }")[2]
        for body in (forms, text["stubs"]):
            self.assertIn("PyValue_IsInlineInt_Outlined(", body)
            self.assertNotRegex(body, r"PyValue_IsInlineInt\(")

    def test_the_forms_define_one_outlined_copy_of_the_row(self):
        forms = self.generated()["forms"]
        self.assertEqual(forms.count("static Py_NO_INLINE bool "
                                     "PyValue_IsInlineInt_Outlined(PyValueBorrow p0)"), 1)
        self.assertIn("static Py_ALWAYS_INLINE bool "
                      "PyValue_IsInlineInt(PyValueBorrow);", forms)

    def test_a_program_without_inline_rows_is_unchanged(self):
        text = self.generated(SHARED)
        self.assertNotIn("_Outlined", "".join(text.values()))

    def test_a_primitive_row_cannot_be_inline(self):
        bad = SHARED.replace("c PyValue_IsInlineInt as bool\n",
                             "c PyValue_IsInlineInt as bool inline\n")
        program = rop_check.check([rop_syntax.parse(bad, "t.rop")])
        self.assertTrue(any("cannot be inline" in d.message
                            for d in program.diagnostics), program.diagnostics)


class FailClosedTests(unittest.TestCase):
    def test_the_generator_writes_nothing_for_an_arm_the_checker_refuses(self):
        import contextlib
        import io
        program = rop_check.check([rop_syntax.parse(UNVERIFIED, "leak.rop")])
        self.assertEqual(program.diagnostics, ())
        with os_helper.temp_dir() as tmp:
            src = Path(tmp) / "leak.rop"
            src.write_text(UNVERIFIED)
            out = Path(tmp) / "cases.c.h"
            err = io.StringIO()
            with contextlib.redirect_stderr(err):
                status = rop_generator.main([str(src), "--cases", str(out)])
            self.assertEqual(status, 1)
            self.assertFalse(out.exists())
            self.assertIn("Leak.Owner", err.getvalue())


class StubTests(unittest.TestCase):
    def test_a_shared_arm_gets_a_stub_with_status_returns(self):
        import io
        program = rop_check.check([rop_syntax.parse(SHARED, "t.rop")])
        self.assertEqual(program.diagnostics, ())
        p = rop_generator.lower_all(program, rop_generator.number_arms(program))
        out = io.StringIO()
        rop_generator.write_stubs(out, p, [])
        text = out.getvalue()
        self.assertEqual(p.left_out, {})
        self.assertIn("_PyIR_Rop_Stub_F_Copy", text)
        self.assertIn("return ir_status", text)
        (entry,) = rop_generator.arm_entries(p)
        self.assertEqual((entry.name, entry.region, entry.stub),
                         ("F_Copy", "shared", "_PyIR_Rop_Stub_F_Copy"))

    def test_publish_takes_inputs_then_reads_old_values_then_writes_then_closes(self):
        # A publish home may be a spent input's home (dst and x name the same
        # slot): the home is emptied first, so the old value read after it is
        # empty and the input is released once, after the output is written
        program = rop_check.check([rop_syntax.parse(SHARED, "t.rop")])
        self.assertEqual(program.diagnostics, ())
        p = rop_generator.lower_all(program, rop_generator.number_arms(program))
        text = "\n".join(p.lowered["F.Copy"].body)
        order = [text.index(mark) for mark in (
            "own_x = _PyFrame_TakeHome(", "old_dst = *", "_PyFrame_WriteHome(frame, home_dst",
            "PyValue_XCLOSE(own_x)", "PyValue_XCLOSE(old_dst)")]
        self.assertEqual(order, sorted(order))
        self.assertEqual(text.count("PyValue_XCLOSE(own_x)"), 1)


    def test_an_owners_binding_stores_then_closes_the_displaced_owners(self):
        # d[] = v exchanges each item with the owner its home displaces;
        # old(d) closes those after every home is written
        program = rop_check.check([rop_syntax.parse(OWNERS, "t.rop")])
        self.assertEqual(program.diagnostics, ())
        p = rop_generator.lower_all(program, rop_generator.number_arms(program))
        self.assertIn("_PyRopOwners v;", p.lowered["F.Split"].decls)
        text = "\n".join(p.lowered["F.Split"].body)
        self.assertIn("v = _PyRop_Split(", text)
        self.assertRegex(text, r"n = \(Py_ssize_t\)IR_\w+\(ir_word\);")
        order = [text.index(mark) for mark in (
            "_PyRop_StoreOwners(&(_PyRopSpan){", "IR_STUB_ESCAPE();",
            "_PyRop_CloseOwners(&v);")]
        self.assertEqual(order, sorted(order))

    def test_unwind_enters_the_unwinder_without_a_raise_event(self):
        program = rop_check.check([rop_syntax.parse(UNWIND, "t.rop")])
        self.assertEqual(program.diagnostics, ())
        p = rop_generator.lower_all(program, rop_generator.number_arms(program))
        self.assertEqual(p.left_out, {})
        text = "\n".join(p.lowered["U.Restore"].body)
        self.assertIn("PROCESS_ERROR_AT(UNWIND, ", text)
        self.assertNotIn("PROCESS_ERROR_AT(RAISE, ", text)

    def test_a_projection_is_its_groups_home_plus_its_index(self):
        from test.test_register_verify import GROUPS
        program = rop_check.check([rop_syntax.parse(GROUPS, "t.rop")])
        self.assertEqual(program.diagnostics, ())
        p = rop_generator.lower_all(program, rop_generator.number_arms(program))
        self.assertEqual(p.left_out, {})
        decls = "\n".join(p.lowered["W.Drop"].decls)
        self.assertIn("home_window_cursor = &IR_SLOT((IR_A(ir_word) + 1));", decls)
        self.assertIn("home_window_value = &IR_SLOT((IR_A(ir_word) + 2));", decls)
        step = "\n".join(p.lowered["G.Step"].decls)
        self.assertIn("home_pair_receiver = &IR_SLOT(IR_B(ir_word));", step)
        # A projection is never a local's home: no flag test guards the take
        text = "\n".join(p.lowered["W.Drop"].body)
        self.assertNotIn("IR_F_LOCAL", text)
        order = [text.index(mark) for mark in (
            "own_window_cursor = _PyFrame_TakeHome(", "own_window_value = _PyFrame_TakeHome(",
            "PyValue_XCLOSE(own_window_value)", "PyValue_XCLOSE(own_window_cursor)")]
        self.assertEqual(order, sorted(order))

    def test_a_completion_is_a_function_every_handler_of_its_family_finds(self):
        import io
        from test.test_register_verify import COMPLETE
        program = rop_check.check([rop_syntax.parse(COMPLETE, "t.rop")])
        self.assertEqual(program.diagnostics, ())
        numbers = rop_generator.number_arms(program)
        self.assertNotIn("S.Complete", numbers)
        p = rop_generator.lower_all(program, numbers)
        self.assertEqual(p.left_out, {})
        self.assertIn("ROP_ENTER(c, (IR_B(ir_word) + 2));",
                      "\n".join(p.lowered["S.Generator"].body))
        out = io.StringIO()
        rop_generator.write_stubs(out, p, [])
        text = out.getvalue()
        self.assertIn("_PyRop_Complete_S_Complete(\n    PyThreadState *tstate, "
                      "_PyInterpreterFrame *frame, _PyIRWord *site,\n    PyValue ir_value)",
                      text)
        self.assertIn("    v = ir_value;\n", text)
        self.assertIn("[ROP_HANDLER_S_Generator - ROP_FIRST_HANDLER] = "
                      "_PyRop_Complete_S_Complete,", text)
        # The completion's status returns are the stubs' (pushed macros)
        self.assertLess(text.index("_PyRop_Complete_S_Complete("),
                        text.index('#pragma pop_macro("IR_BRANCH")'))

    def test_a_yield_names_the_home_the_sent_value_resumes_into(self):
        from test.test_register_verify import YIELD
        program = rop_check.check([rop_syntax.parse(YIELD, "t.rop")])
        self.assertEqual(program.diagnostics, ())
        p = rop_generator.lower_all(program, rop_generator.number_arms(program))
        self.assertEqual(p.left_out, {})
        self.assertIn("ROP_YIELD(v, IR_B(ir_word));", "\n".join(p.lowered["Y.Value"].body))


UNWIND = """\
family U(exc:Src) exits(unwind) ops(NOP);
point U.Raised(current, site, pending) keep(exc) spent() defs();
extern helper restore(t:Thread, e:Src) cases(err(x:Err)) effects(runs_python, raises)
  c RESTORE as raised
  evidence(review: "r", contract: "c");
arm U.Restore split effects(runs_python, raises) feedback() exits(unwind)
{
  entry():
    call restore(thread, exc) { err -> Failed(); }
  Failed(e:Err):
    exit unwind(U.Raised, e);
}
"""


OWNERS = """\
family F(dsts:Span<Dst>, x:Src) exits(next) ops(NOP);
point F.Done(current, next_head, clear) keep(x) spent() defs(dsts);
extern helper split(v:B, n:Size) -> Span<O>
  effects(allocates)
  c _PyRop_Split as handle
  evidence(review: "r", contract: "c");
arm F.Split shared effects(allocates, writes, releases on (dsts), runs_python) feedback()
  exits(next)
{
  entry():
    let n:Size = count(dsts);
    let v:Span<O> = split(use(x), n);
    publish [dsts[] = v] retire [old(dsts)] at F.Done;
    exit next();
}
"""


WRAPPED = """\
extern primitive is_int(v:B) -> Bool
  effects()
  test(inline_int(v))
  c PyValue_IsInlineInt as bool
  evidence(review: "r", contract: "c", lowering: "l", test: "t");
family F(dst:Dst, x:Src, fb:Fb<Call>) exits(next, miss) ops(CALL)
  events(CALL, C_RETURN);
family G(dst:Dst, x:Src) exits(next, miss) ops(NOP);
arm F.A shared effects() feedback() exits(next, miss)
{
  entry():
    let v:B = use(x);
    guard is_int(v) else miss;
    exit next();
}
arm G.A shared effects() feedback() exits(next, miss)
{
  entry():
    let v:B = use(x);
    guard is_int(v) else miss;
    exit next();
}
arm F.B shared effects() feedback() exits(next, miss)
{
  entry():
    let v:B = use(x);
    guard is_int(v) else miss;
    exit next();
}
"""


class HomeStateTests(unittest.TestCase):
    """The homes of a family's word and their state on each normal edge,
    from the points its arms publish there (rop_generator.family_homes)"""

    @classmethod
    def setUpClass(cls):
        cls.program = rop_verify.load(
            sorted((ROOT / "Python" / "register_ops").glob("*.rop")))
        assert not cls.program.diagnostics

    def states(self, family):
        h = rop_generator.family_homes(self.program, family)
        return tuple(" ".join(f"{k}={v.value[0]}" for k, v in e.items()) for e in h.edges)

    def test_the_families_with_a_home_group(self):
        # P preserve, V vacate, D define, U unknown (the arms differ)
        expected = {
            "ForIter": ("item=D pair.receiver=P pair.cursor=U",
                        "item=P pair.receiver=V pair.cursor=V"),
            "Send": ("completed=P send.receiver=P send.cursor=U send.value=D",
                     "completed=D send.receiver=V send.cursor=V send.value=V"),
            "CleanupThrow": ("completed=P send.receiver=P send.cursor=P send.value=P exc=P",
                             "completed=D send.receiver=V send.cursor=V send.value=V exc=V"),
            "IterOpen": ("pair.receiver=D pair.cursor=D source=V",
                         "pair.receiver=P pair.cursor=P source=P"),
            "IterClose": ("pair.receiver=V pair.cursor=V", "pair.receiver=P pair.cursor=P"),
        }
        for family, edges in expected.items():
            with self.subTest(family):
                self.assertEqual(self.states(family), edges)

    def test_groups_reads_and_vacant_homes(self):
        expected = {
            "ForIter": (2, False, {"pair.receiver", "pair.cursor"}, {"pair.cursor"}),
            "Send": (3, False, {"send.receiver", "send.cursor", "send.value"},
                     {"send.cursor"}),
            "CleanupThrow": (3, False, {"send.receiver", "send.cursor", "send.value", "exc"},
                             {"send.cursor", "send.value"}),
            "IterOpen": (2, True, {"source"}, set()),
            "IterClose": (2, False, {"pair.receiver", "pair.cursor"}, {"pair.cursor"}),
            # A Maybe<Src> operand may be vacant by its type
            "WithExit": (0, False, {"exit_fn", "self", "exc"}, {"self"}),
        }
        for family, (group, out, read, vacant) in expected.items():
            with self.subTest(family):
                h = rop_generator.family_homes(self.program, family)
                self.assertEqual((h.group, h.out, set(h.read), set(h.vacant)),
                                 (group, out, read, vacant))

    def test_every_family_states_each_home_from_its_points(self):
        for family in self.program.families:
            with self.subTest(family):
                h = rop_generator.family_homes(self.program, family)
                self.assertLessEqual(h.vacant, h.read)
                for edge in h.edges:
                    self.assertEqual(tuple(edge), h.homes)
                    if h.out:
                        group = [n for n in h.homes if "." in n]
                        taken = any(e == rop_verify.EDGE_NEXT
                                    for a in self.program.arms_of(family)
                                    for e, _ in self.program.exit_points[a.name])
                        if taken and edge is h.edges[0]:
                            self.assertEqual({edge[n] for n in group},
                                             {rop_generator.HomeState.DEFINE})

    def test_every_normal_exit_names_a_point_of_its_family(self):
        for name, pairs in self.program.exit_points.items():
            family = rop_verify.arm_of(self.program, name).family
            for edge, point in pairs:
                with self.subTest(name, edge=edge):
                    self.assertIn(edge, (rop_verify.EDGE_NEXT, rop_verify.EDGE_TAKEN))
                    if point is not None:
                        self.assertEqual(self.program.points[point].family, family)


def wrapped_entries():
    program = rop_check.check([rop_syntax.parse(WRAPPED, "t.rop")])
    assert program.diagnostics == (), program.diagnostics
    p = rop_generator.Products(program, rop_generator.number_arms(program), {}, {}, {}, {})
    return rop_generator.arm_entries(p)


class ArmRowTests(unittest.TestCase):
    """The per-handler tables list every arm, and the wrappers of families
    that declare events after the arms"""

    def test_a_family_with_events_gets_a_twin_per_arm_after_the_arms(self):
        entries = wrapped_entries()
        self.assertEqual([e.dotted for e in entries],
                         ["F.A", "G.A", "F.B", "F.A.Observed", "F.B.Observed"])
        fa, ga, fb, fa_twin, fb_twin = entries
        self.assertEqual((fa.twin, ga.twin, fb.twin), (3, None, 4))
        self.assertEqual((fa_twin.observes, fb_twin.observes), (0, 2))
        self.assertEqual(fa.events, ("CALL", "C_RETURN"))
        self.assertEqual(fa_twin.events, fa.events)
        self.assertEqual(ga.events, ())
        self.assertEqual({e.base for e in entries if e.dotted.startswith("F.")}, {"CALL"})
        self.assertFalse(any(e.emitted for e in entries))

    def test_the_handler_ids_of_the_twins_are_defined(self):
        import io
        program = rop_check.check([rop_syntax.parse(WRAPPED, "t.rop")])
        p = rop_generator.Products(program, rop_generator.number_arms(program), {}, {}, {}, {})
        out = io.StringIO()
        rop_generator.write_metadata(out, p, [])
        text = out.getvalue()
        self.assertIn("#define ROP_NUM_HANDLERS 5", text)
        self.assertIn("#define ROP_HANDLER_F_A_Observed (ROP_FIRST_HANDLER + 3)", text)
        self.assertIn("#define ROP_HANDLER_F_B_Observed (ROP_FIRST_HANDLER + 4)", text)

    def test_add_int_row_is_derived_from_its_encoding_and_declarations(self):
        entries = rop_generator.arm_entries(products())
        add_int = next(e for e in entries if e.dotted == "Add.Int")
        fmt = add_int.format
        self.assertEqual(add_int.base, "BINARY_OP")
        self.assertEqual(fmt.roles, ("_PyIR_ROLE_DST", "_PyIR_ROLE_IN0 + 0",
                                     "_PyIR_ROLE_IN0 + 1", "_PyIR_ROLE_FB"))
        self.assertEqual((fmt.fb, fmt.fb_schema, fmt.ninputs, fmt.dst, fmt.writes),
                         ("_PyIR_LOC_D", "Binary", 2, -2, 1))
        self.assertEqual(fmt.effects, ("_PyIR_RAISES", "_PyIR_DEOPTS"))
        self.assertEqual(fmt.operands, "_PyIR_OPERANDS_NONE")
        generic = next(e for e in entries if e.dotted == "Add.Generic")
        self.assertIn("_PyIR_SPECIALIZES", generic.format.effects)
        self.assertIn("_PyIR_ESCAPES", generic.format.effects)

    def test_every_arm_has_a_row_in_every_generated_table(self):
        entries = rop_generator.arm_entries(products())
        header = (ROOT / "Include/internal/pycore_ir_metadata.h").read_text()
        ids = (ROOT / "Include/ir_handler_ids.h").read_text()
        old = int(re.search(r"IR_FIRST_ARM\s+(\d+)", ids).group(1))
        total = int(re.search(r"IR_NUM_HANDLERS\s+(\d+)", ids).group(1))
        self.assertEqual(total, old + len(entries))
        for entry in entries:
            handler = old + entry.index
            for table in ("_PyIR_HandlerFlags", "_PyIR_HandlerBase"):
                with self.subTest(arm=entry.dotted, table=table):
                    section = header[header.index(f"{table}[IR_NUM_HANDLERS] = {{"):]
                    section = section[:section.index("};")]
                    self.assertRegex(section, rf"\[{handler}\] = ")
            self.assertIn(f'[{handler}] = "{entry.dotted}"', header)
            self.assertIn(f"[{handler}] = {{ .flags = _PyIR_VALID", header)

    def test_too_many_handlers_name_the_overflow(self):
        arms = [None] * 512
        with self.assertRaisesRegex(handler_table.ir_arms.HandlerLimitError,
                                    "1 word kinds and 512 arms and wrappers need "
                                    "513 handler ids, past the 512"):
            handler_table.num_handlers(arms)
        self.assertEqual(handler_table.num_handlers(arms[:511]), 512)


ENVIRONMENT = """\
family F(dst:Dst, x:Src) exits(next, miss) ops(NOP);
extern helper probe(t:Thread, f:Frame, s:Site, h:Head) -> Bool
  effects(reads)
  c probe_c as bool
  evidence(review: "r", contract: "c");
arm F.Probe shared effects() feedback() exits(next, miss)
{
  entry():
    guard probe(thread, frame, site, next_head) else miss;
    exit next();
}
"""


class EnvironmentTests(unittest.TestCase):
    def test_table_covers_implicit_names_the_checker_knows(self):
        import rop_catalog
        self.assertLessEqual(set(rop_catalog.ENVIRONMENT_C), set(rop_catalog.ENVIRONMENT))

    def test_each_name_has_the_same_c_expression_in_every_product(self):
        import io
        program = rop_check.check([rop_syntax.parse(ENVIRONMENT, "t.rop")])
        self.assertEqual(program.diagnostics, ())
        p = rop_generator.lower_all(program, rop_generator.number_arms(program))
        self.assertEqual(p.left_out, {})
        for write in (rop_generator.write_cases, rop_generator.write_forms,
                      rop_generator.write_stubs):
            out = io.StringIO()
            write(out, p, [])
            self.assertIn("probe_c(tstate, frame, this_instr, next_instr)", out.getvalue(),
                          write.__name__)


INDEXED = """\
family F(dst:Dst, x:Src, fb:Fb<Attr>) exits(next, miss) ops(LOAD_ATTR);
extern helper classify(v:B, f:Fb<Attr>)
  cases(first(r:Recipe<Attr>), second(), none())
  effects(reads)
  c classify_c as case_index
  evidence(review: "r", contract: "c");
arm F.Sort shared effects() feedback() exits(next, miss)
{
  entry():
    call classify(use(x), fb) {
      first -> A();
      second -> B();
      none -> C();
    }
  A(r:Recipe<Attr>):
    exit next();
  B():
    exit next();
  C():
    exit miss();
}
"""


class IndexedConventionTests(unittest.TestCase):
    def test_case_index_row_switches_on_the_result(self):
        program = rop_check.check([rop_syntax.parse(INDEXED, "t.rop")])
        self.assertEqual(program.diagnostics, ())
        p = rop_generator.lower_all(program, rop_generator.number_arms(program))
        self.assertEqual(p.left_out, {})
        text = "\n".join(p.lowered["F.Sort"].body)
        self.assertIn("classify_c(", text)
        self.assertIn("&out1", text)
        # The case that falls through needs no test
        self.assertIn("== 1", text)
        self.assertIn("== 2", text)


class GeneratedAddIntTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.products = products()

    def test_metadata_lists_every_arm_in_definition_order(self):
        import io
        out = io.StringIO()
        rop_generator.write_metadata(out, self.products, [])
        text = out.getvalue()
        for i, name in enumerate(self.products.program.arms):
            handler = rop_generator.handler(name)
            self.assertIn(f"#define {handler} (ROP_FIRST_HANDLER + {i})", text)

    def test_add_int_is_emitted(self):
        self.assertIn("Add.Int", self.products.lowered)

    def test_arms_left_out_say_what_blocks_them(self):
        for arm, why in self.products.left_out.items():
            with self.subTest(arm):
                self.assertIn("pending", why)

    def test_add_int_shape(self):
        """About 15 statements, one type test, no NULL test on the fitting path"""
        low = self.products.lowered["Add.Int"]
        hot = []
        for line in low.body:
            if line.startswith("rop_Add_Int_Overflow:"):
                break
            hot.append(line.strip())
        statements = [line for line in hot
                      if line.endswith(";") and not line.startswith("goto")]
        self.assertLessEqual(len(statements), 17, hot)
        tests = [line for line in hot if "BothInlineInt" in line]
        self.assertEqual(len(tests), 1)
        self.assertFalse(any("IsNull" in line for line in hot))

    def test_value_api_only(self):
        for low in self.products.lowered.values():
            text = "\n".join(low.decls + low.body)
            self.assertIsNone(re.search(r"\.bits\b|_PyValue_BITS", text), text)

    def test_generated_files_are_current(self):
        for name, writer in (("Python/generated_rop_cases.c.h", rop_generator.write_cases),
                             ("Python/generated_rop_forms.c.h", rop_generator.write_forms)):
            import io
            out = io.StringIO()
            sources = sorted(f"Python/register_ops/{p.name}"
                             for p in (ROOT / "Python" / "register_ops").glob("*.rop"))
            writer(out, self.products, sources)
            self.assertEqual((ROOT / name).read_text(), out.getvalue(), name)


def c_function(text: str, symbol: str) -> tuple[str, str] | None:
    "The parameters and body of a C function definition in text"
    m = re.search(rf"^{re.escape(symbol)}\(([^)]*)\)\s*\{{", text, re.M)
    if m is None:
        return None
    i, depth = m.end(), 1
    while depth:
        depth += {"{": 1, "}": -1}.get(text[i], 0)
        i += 1
    return m.group(1), text[m.end():i]


class TemplateLayoutTests(unittest.TestCase):
    """Templates read feedback entries through rop_catalog.FB_LAYOUT, the
    one statement of each field's C member and type"""

    @classmethod
    def setUpClass(cls):
        cls.products = products()
        cls.made, cls.refused = rop_generator.template_texts(cls.products)

    def test_the_layout_states_every_schema_field(self):
        for schema, layout in rop_catalog.FB_LAYOUT.items():
            with self.subTest(schema):
                self.assertEqual(set(layout.fields), set(rop_catalog.SCHEMA_FIELDS[schema]))
                for f in layout.fields.values():
                    self.assertEqual(f.member is None, not f.ctype)

    def test_every_member_gets_a_static_check(self):
        out = io.StringIO()
        rop_generator.write_layout_checks(out)
        text = out.getvalue()
        for layouts in (rop_catalog.FB_LAYOUT, rop_catalog.RECIPE_LAYOUT):
            for layout in layouts.values():
                for f in layout.fields.values():
                    if f.member is not None:
                        self.assertIn(f"(({layout.struct} *)0)->{f.member}, "
                                      f"{f.ctype}: 1", text)

    def test_union_members_share_their_storage(self):
        out = io.StringIO()
        rop_generator.write_layout_checks(out)
        text = out.getvalue()
        for layout in rop_catalog.FB_LAYOUT.values():
            for u in layout.unions:
                for m in u.members[1:]:
                    self.assertIn(f"offsetof({layout.struct}, {m}) == "
                                  f"offsetof({layout.struct}, {u.members[0]})", text)

    def test_rows_declare_the_entry_fields_their_c_reads(self):
        # Scans each row's C body for reads through its entry pointer: the
        # body only, not the functions it calls
        headers = sorted((ROOT / "Include" / "internal").glob("*.h"))
        sources = sorted((ROOT / "Python").glob("*.c"))
        text = "\n".join(p.read_text() for p in headers + sources)
        program = self.products.program
        checked = 0
        for name, x in program.externs.items():
            fbs = [p for p in x.params if isinstance(p.type, rop_model.SchemaType)
                   and p.type.ctor is rop_catalog.SchemaCtor.Fb]
            if not fbs or x.c is None:
                continue
            found = c_function(text, x.c.symbol)
            if found is None:
                continue
            params, body = found
            pointer = re.search(r"\b_PyFb\w+\s*\*\s*(\w+)", params)
            if pointer is None:
                continue
            layout = rop_catalog.FB_LAYOUT.get(fbs[0].type.schema)
            declared = {a.field for a in x.contract.feedback
                        if a.mode is rop_catalog.AccessMode.read}
            if layout is None or "entry" in declared:
                continue
            # A plain store is a write, not a read
            reads = re.findall(rf"\b{pointer.group(1)}->([\w.]+)\b(?!\s*=[^=])", body)
            for member in reads:
                field = layout.field_of_member(member) or next(
                    (f for f, c in layout.fields.items()
                     if c.member and c.member.startswith(member + ".")), None)
                with self.subTest(row=name, member=member):
                    self.assertIn(f"entry.{field}", declared, f"{x.c.symbol} reads {member}")
            checked += 1
        self.assertGreater(checked, 5)

    def test_a_template_that_reads_its_entry_checks_its_handler(self):
        for name in ("GetAttr.Shape", "GetAttr.Instance"):
            self.assertIn(name, self.made)
        for name, lines in self.made.items():
            text = "\n".join(lines)
            with self.subTest(name):
                self.assertEqual("_PyJitTemplate_CheckHandler(cx);" in text,
                                 "&fb->" in text)

    def test_reading_an_undeclared_field_refuses(self):
        prog = self.products.program
        arm = prog.arms["GetAttr.Shape"]
        feedback = tuple(a for a in arm.contract.feedback if a.field != "entry.shape")
        narrowed = dataclasses.replace(
            arm, contract=dataclasses.replace(arm.contract, feedback=feedback))
        with self.assertRaisesRegex(rop_generator.Unsupported, "entry.shape"):
            rop_leaf.leaf_of_arm(prog, narrowed)

    def test_templates_file_is_current(self):
        out = io.StringIO()
        sources = sorted(f"Python/register_ops/{p.name}"
                         for p in (ROOT / "Python" / "register_ops").glob("*.rop"))
        rop_generator.write_templates(out, self.products, sources)
        self.assertEqual((ROOT / "Python/generated_rop_templates.c.h").read_text(),
                         out.getvalue())


if __name__ == "__main__":
    unittest.main()
