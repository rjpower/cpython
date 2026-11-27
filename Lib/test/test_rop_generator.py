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
    import rop_model
    import rop_print
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


SHARED = """\
family F(dst:Dst, x:Src) exits(next, miss) generic(Any);
point F.Done(current, next_head, clear) keep() spent(x) defs(dst);
extern primitive is_int(v:B) -> Bool
  effects()
  test(inline_int(v))
  c PyValue_IsInlineInt as bool
  evidence(review: "r", contract: "c");
arm F.Copy shared effects(writes, releases) feedback() exits(next, miss)
{
  entry():
    let v:B = use(x);
    guard is_int(v) else miss;
    let z:O = dup(v);
    publish [dst = z] retire [spent(x), old(dst)] at F.Done;
    exit next();
}
arm F.Any shared effects(writes, releases) feedback() exits(next)
{
  entry():
    let v:B = use(x);
    let z:O = dup(v);
    publish [dst = z] retire [spent(x), old(dst)] at F.Done;
    exit next();
}
"""




# SHARED with a guard whose fact leaves the value counted
COUNTED = SHARED.replace("test(inline_int(v))\n  c PyValue_IsInlineInt",
                         "test(object(v))\n  c PyValue_IsObject")


# An arm rop_check accepts and rop_verify refuses: the owner it acquires
# leaks and no publish names a point (RV's g01)
UNVERIFIED = """\
family Leak(x:Src) exits(next);
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


class LayoutOrderTests(unittest.TestCase):
    """`hot` on a family: its arms, less the generic arm, come first in the
    evaluator's layout (IR_TARGET_LAYOUT), the others after"""

    def lowered(self, text):
        program = rop_check.check([rop_syntax.parse(text, "t.rop")])
        self.assertEqual(program.diagnostics, ())
        return program, rop_generator.lower_all(program, rop_generator.number_arms(program))

    def test_hot_family_arms_come_first_and_its_generic_last(self):
        hot = SHARED.replace("generic(Any);", "generic(Any) hot;")
        program, p = self.lowered(hot)
        self.assertTrue(program.families["F"].hot)
        self.assertEqual(rop_generator.layout_order(p), ["F_Copy", "F_Any"])
        self.assertIn(" hot;", "\n".join(rop_print.decl_lines(program.families["F"])))

    def test_without_hot_the_source_order_stays(self):
        program, p = self.lowered(SHARED)
        self.assertFalse(program.families["F"].hot)
        self.assertEqual(rop_generator.layout_order(p), list(map(rop_generator.c_name, p.lowered)))


class InlineRowTests(unittest.TestCase):
    """`c SYMBOL as CONVENTION inline`: the arms call the row's own symbol,
    which the inline declarations always inline"""

    def generated(self, text=INLINE):
        import io
        program = rop_check.check([rop_syntax.parse(text, "t.rop")])
        self.assertEqual(program.diagnostics, ())
        p = rop_generator.lower_all(program, rop_generator.number_arms(program))
        out = {}
        for name in ("cases", "inline"):
            out[name] = io.StringIO()
            getattr(rop_generator, "write_inline_declarations" if name == "inline" else f"write_{name}")(out[name], p, [])
        return {k: v.getvalue() for k, v in out.items()}

    def test_the_arms_call_the_symbol_they_link(self):
        text = self.generated()
        self.assertIn("PyValue_IsInlineInt(", text["cases"])
        self.assertNotIn("_Outlined", text["cases"])
        # every unit that runs the arms includes the declaration
        self.assertIn("static Py_ALWAYS_INLINE bool "
                      "PyValue_IsInlineInt(PyValueBorrow);", text["inline"])

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


class ArmLoweringTests(unittest.TestCase):
    def test_publish_takes_inputs_then_reads_old_values_then_writes_then_closes(self):
        # A publish home may be a spent input's home (dst and x name the same
        # slot): the home is emptied first, so the old value read after it is
        # empty and the input is released once, after the output is written
        program = rop_check.check([rop_syntax.parse(COUNTED, "t.rop")])
        self.assertEqual(program.diagnostics, ())
        p = rop_generator.lower_all(program, rop_generator.number_arms(program))
        text = "\n".join(p.lowered["F.Copy"].body)
        order = [text.index(mark) for mark in (
            "own_x = _PyFrame_TakeHome(", "old_dst = *", "_PyFrame_WriteHome(frame, home_dst",
            "ROP_TAIL_CLOSE(own_x,", "ROP_TAIL_CLOSE(old_dst,")]
        self.assertEqual(order, sorted(order))
        self.assertEqual(text.count("ROP_TAIL_CLOSE(own_x,"), 1)

    def test_the_closes_before_a_dispatch_leave_by_a_tail_call(self):
        # Each close drops its count inline; close k's cold block parks its
        # value, then the later closes' values in parking mode, and leaves
        # through close_dispatch (lower_tail_closes)
        program = rop_check.check([rop_syntax.parse(COUNTED, "t.rop")])
        self.assertEqual(program.diagnostics, ())
        p = rop_generator.lower_all(program, rop_generator.number_arms(program))
        text = "\n".join(p.lowered["F.Copy"].body)
        self.assertNotIn("PyValue_XCLOSE(", text)
        tail = "rop_F_Copy_Tail0"
        self.assertIn(f"ROP_TAIL_CLOSE(own_x, {tail}_0);\n"
                      f"ROP_TAIL_CLOSE(old_dst, {tail}_1);\n"
                      "DISPATCH();\n", text)
        self.assertIn(f"{tail}_0:\nROP_TAIL_PARK_FIRST(own_x);\ngoto {tail}_Rest1;\n"
                      f"{tail}_1:\nROP_TAIL_PARK_FIRST(old_dst);\ngoto {tail}_Rest2;\n"
                      f"{tail}_Rest1:\nROP_TAIL_PARK(old_dst);\n"
                      f"{tail}_Rest2:\nROP_TAIL_DISPATCH();", text)

    def test_a_guarded_uncounted_input_is_cleared_and_not_closed(self):
        # The guard in the publish's own block proves x an inline int, which
        # owns no count: a temporary's home is cleared, nothing is closed
        program = rop_check.check([rop_syntax.parse(SHARED, "t.rop")])
        self.assertEqual(program.diagnostics, ())
        p = rop_generator.lower_all(program, rop_generator.number_arms(program))
        text = "\n".join(p.lowered["F.Copy"].body)
        self.assertIn("_PyFrame_WriteHome(frame, home_x, PyValue_NULL);", text)
        self.assertNotIn("own_x", text)
        self.assertEqual(text.count("ROP_TAIL_CLOSE("), 1)

    def test_an_owners_binding_publishes_the_items_in_one_call(self):
        # d[] = v writes every item into its home before any displaced value
        # is closed (_PyRop_PublishItems), which discharges old(d); the span
        # carries the layout's constant geometry
        program = rop_check.check([rop_syntax.parse(OWNERS, "t.rop")])
        self.assertEqual(program.diagnostics, ())
        p = rop_generator.lower_all(program, rop_generator.number_arms(program))
        self.assertIn("_PyRopItems v;", p.lowered["F.Split"].decls)
        text = "\n".join(p.lowered["F.Split"].body)
        self.assertIn("v = _PyRop_Split(", text)
        self.assertRegex(text, r"n = \(Py_ssize_t\)IR_\w+\(ir_word\);")
        self.assertRegex(text, r"span\d+ = \(_PyRopSpan\)\{frame, this_instr, 0, "
                               r"IR_\w+\(ir_word\), \d, \d\};")
        order = [text.index(mark) for mark in (
            "IR_STUB_ESCAPE();", "_PyRop_PublishItems(&span")]
        self.assertEqual(order, sorted(order))
        self.assertNotIn("PyValue_XCLOSE", text)

    def test_anchors_of_a_literal_span_are_scalars(self):
        # Binary.Generic anchors [x, y]: a view and an owner per source on
        # the hot path; the aggregate exists only on the exits whose rows
        # consume the whole anchor, and no aggregate is a compound literal
        # taken by address
        text = "\n".join(products().lowered["Binary.Generic"].body)
        self.assertNotIn("_PyRop_AnchorForCall(", text)
        self.assertNotIn("_PyRop_AnchorArg(", text)
        self.assertNotIn("&(_PyRop", text)
        self.assertIn("a_view0 = PyValue_Borrow(*home_x);", text)
        self.assertIn("a_owned1 = (ir_word & IR_F_LOCAL_C) ? PyValue_DUP(a_view1) "
                      ": PyValue_NULL;", text)
        hot = text[:text.index("DISPATCH();")]
        self.assertNotIn("_PyRopAnchors", hot)
        self.assertIn("ROP_TAIL_CLOSE(a_owned0, rop_Binary_Generic_Tail0_0);\n"
                      "ROP_TAIL_CLOSE(a_owned1, rop_Binary_Generic_Tail0_1);", hot)

    def test_a_target_is_decoded_by_two_shifts(self):
        # Jump.Poll's signed 24-bit distance at bit 16: shifted up to the
        # word's top bit, then arithmetically down
        text = "\n".join(products().lowered["Jump.Poll"].body)
        self.assertIn("this_instr + 1 + Py_ARITHMETIC_RIGHT_SHIFT(int64_t, "
                      "(int64_t)(ir_word << 24), 40)", text)

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
            "ROP_TAIL_CLOSE(own_window_value,", "ROP_TAIL_CLOSE(own_window_cursor,")]
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
        rop_generator.write_completions(out, p, [])
        text = out.getvalue()
        self.assertIn("_PyRop_Complete_S_Complete(\n    PyThreadState *tstate, "
                      "_PyInterpreterFrame *frame, _PyIRWord *site,\n    PyValue ir_value)",
                      text)
        self.assertIn("    v = ir_value;\n", text)
        self.assertIn("[ROP_HANDLER_S_Generator - ROP_FIRST_HANDLER] = "
                      "_PyRop_Complete_S_Complete,", text)

    def test_a_yield_names_the_home_the_sent_value_resumes_into(self):
        from test.test_register_verify import YIELD
        program = rop_check.check([rop_syntax.parse(YIELD, "t.rop")])
        self.assertEqual(program.diagnostics, ())
        p = rop_generator.lower_all(program, rop_generator.number_arms(program))
        self.assertEqual(p.left_out, {})
        self.assertIn("ROP_YIELD(v, IR_B(ir_word));", "\n".join(p.lowered["Y.Value"].body))


UNWIND = """\
family U(exc:Src) exits(unwind);
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
family F(dsts:Span<Dst>, x:Src) exits(next);
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
  evidence(review: "r", contract: "c");
family F(dst:Dst, x:Src, fb:Fb<Call>) exits(next, miss)
  events(CALL, C_RETURN);
family G(dst:Dst, x:Src) exits(next, miss);
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
arm F.Observed shared observes effects() feedback() exits(next)
{
  entry():
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

    def test_the_word_format_lays_the_homes_out_by_field(self):
        # ForIter(item:Dst, pair:IterPair): item is home 0 (A), the receiver
        # home 1 (B), the cursor home 4; two bits per home and edge
        fmt = rop_generator.arm_format(self.program, self.program.arms["ForIter.Generic"])
        P, V, D, U = range(4)
        def at(edge, home, state):
            return state << 2 * (6 * edge + home)
        self.assertEqual(fmt.roles[1], "_PyIR_ROLE_GROUP")
        self.assertEqual((fmt.group, fmt.normal, fmt.vacant), (2, 0b11, 1 << 4))
        self.assertEqual(fmt.edges, at(0, 0, D) | at(0, 1, P) | at(0, 4, U) |
                                    at(1, 0, P) | at(1, 1, V) | at(1, 4, V))
        # IterOpen(pair:Out<IterPair>, source:Src) defines the pair
        fmt = rop_generator.arm_format(self.program, self.program.arms["IterOpen.Generic"])
        self.assertEqual(fmt.roles[0], "_PyIR_ROLE_GROUP")
        self.assertIn("_PyIR_GROUP_OUT", fmt.flags)
        self.assertEqual((fmt.group, fmt.normal, fmt.vacant), (2, 0b01, 0))
        self.assertEqual(fmt.edges, at(0, 0, D) | at(0, 4, D) | at(0, 1, V))

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

    def test_the_arms_of_a_family_with_events_name_its_observer(self):
        entries = wrapped_entries()
        self.assertEqual([e.dotted for e in entries],
                         ["F.A", "G.A", "F.B", "F.Observed"])
        fa, ga, fb, fo = entries
        self.assertEqual((fa.observer, ga.observer, fb.observer, fo.observer),
                         ("F_Observed", None, "F_Observed", None))
        self.assertEqual(fa.events, ("CALL", "C_RETURN"))
        self.assertEqual(fo.events, fa.events)
        self.assertEqual(ga.events, ())
        self.assertFalse(any(e.emitted for e in entries))

    def test_the_handler_ids_are_the_arms(self):
        import io
        program = rop_check.check([rop_syntax.parse(WRAPPED, "t.rop")])
        p = rop_generator.Products(program, rop_generator.number_arms(program), {}, {}, {}, {})
        out = io.StringIO()
        rop_generator.write_metadata(out, p, [])
        text = out.getvalue()
        self.assertIn("#define ROP_NUM_HANDLERS 4", text)
        self.assertIn("#define ROP_HANDLER_F_Observed (ROP_FIRST_HANDLER + 3)", text)

    def test_add_int_row_is_derived_from_its_encoding_and_declarations(self):
        entries = rop_generator.arm_entries(products())
        add_int = next(e for e in entries if e.dotted == "Add.Int")
        fmt = add_int.format
        self.assertEqual(fmt.roles, ("_PyIR_ROLE_DST", "_PyIR_ROLE_SRC + 0",
                                     "_PyIR_ROLE_SRC + 1", "_PyIR_ROLE_FB"))
        self.assertEqual((fmt.fb, fmt.fb_schema), ("_PyIR_LOC_D", "Binary"))
        self.assertEqual(fmt.effects, ("_PyIR_RAISES", "_PyIR_DEOPTS"))
        self.assertEqual(fmt.operands, "_PyIR_OPERANDS_NONE")
        generic = next(e for e in entries if e.dotted == "Add.Generic")
        self.assertIn("_PyIR_SPECIALIZES", generic.format.effects)
        self.assertIn("_PyIR_ESCAPES", generic.format.effects)

    def test_group_operands(self):
        # A home group takes one GROUP field.  ForIter, Send and CleanupThrow
        # read it and have a destination of their own: the translator steps
        # the group as a window, whose receiver passes through where edge 0
        # keeps it (_PyRegCfg_ArmFormat).  IterClose spends its group;
        # IterOpen defines it (GROUP_OUT).  The edges come from the verified
        # points (load_arm_entries).
        entries = {e.dotted: e for e in rop_generator.load_arm_entries()}
        for name, group, keeps, has_dst, out in (
                ("ForIter.Generic", 2, True, True, False),
                ("Send.Generic", 3, True, True, False),
                ("CleanupThrow.Generic", 3, False, True, False),
                ("IterClose.Retire", 2, False, False, False),
                ("IterOpen.Generic", 2, False, True, True)):
            with self.subTest(arm=name):
                fmt = entries[name].format
                self.assertEqual(fmt.group, group)
                self.assertEqual(fmt.roles.count("_PyIR_ROLE_GROUP"), 1)
                at = fmt.roles.index("_PyIR_ROLE_GROUP")
                self.assertEqual("_PyIR_GROUP_OUT" in fmt.flags, out)
                self.assertEqual("_PyIR_ROLE_DST" in fmt.roles, has_dst and not out)
                if not out:
                    kept = fmt.normal & 1 and (fmt.edges >> 2 * at) & 3 == 0
                    self.assertEqual(bool(kept), keeps)

    def test_every_arm_has_a_row_in_every_generated_table(self):
        entries = rop_generator.arm_entries(products())
        header = (ROOT / "Include/internal/pycore_ir_metadata.h").read_text()
        ids = (ROOT / "Include/ir_handler_ids.h").read_text()
        old = int(re.search(r"IR_FIRST_ARM\s+(\d+)", ids).group(1))
        total = int(re.search(r"IR_NUM_HANDLERS\s+(\d+)", ids).group(1))
        self.assertEqual(total, old + len(entries))
        for entry in entries:
            handler = old + entry.index
            for table in ("_PyIR_HandlerFlags",):
                with self.subTest(arm=entry.dotted, table=table):
                    section = header[header.index(f"{table}[IR_NUM_HANDLERS] = {{"):]
                    section = section[:section.index("};")]
                    self.assertRegex(section, rf"\[{handler}\] = ")
            self.assertIn(f'[{handler}] = "{entry.dotted}"', header)
            self.assertIn(f"[{handler}] = {{ .flags = _PyIR_VALID", header)

    def test_too_many_handlers_name_the_overflow(self):
        arms = [None] * 512
        # The limit at the checked-in base 0, also under a guard build
        env = self.enterContext(os_helper.EnvironmentVarGuard())
        env.unset("PYJIT_IR_ID_BASE")
        with self.assertRaisesRegex(handler_table.ir_arms.HandlerLimitError,
                                    "1 word kinds and 512 arms and wrappers need "
                                    "513 handler ids, past the 512"):
            handler_table.num_handlers(arms)
        self.assertEqual(handler_table.num_handlers(arms[:511]), 512)


MEMBERS = """\
enum Op(A, B, C);
family F(x:Src, op:Enum<Op>) exits(next, miss);
extern primitive op_ok(op:Enum<Op>, v:B) -> Bool
  effects()
  c op_ok_c as bool
  evidence(review: "r", contract: "c");
arm F.Fast shared OVER effects() feedback() exits(next, miss)
{
  entry():
    guard op_ok(op, use(x)) else miss;
    exit next();
}
"""


def members_program(over: str) -> "rop_check.Program":
    return rop_check.check([rop_syntax.parse(MEMBERS.replace("OVER", over), "m.rop")])


def guard_args(arm: "rop_model.Arm") -> tuple:
    (guard,) = arm.graph.blocks[0].stmts
    return guard.cond.args


class MemberArmTests(unittest.TestCase):
    """An arm stated over the members of an enum operand (`over(op: ...)`)"""

    def test_each_member_is_an_arm_with_the_member_a_constant(self):
        program = members_program("over(op: A, B)")
        self.assertEqual(program.diagnostics, ())
        self.assertEqual(list(program.arms), ["F.Fast_A", "F.Fast_B"])
        for member in ("A", "B"):
            arm = program.arms[f"F.Fast_{member}"]
            self.assertEqual(arm.member, member)
            self.assertEqual(guard_args(arm)[0], rop_model.Name(member))
        ms = program.member_sets["F.Fast"]
        self.assertEqual((ms.operand, ms.enum, ms.rest), ("op", "Op", None))
        self.assertIsNone(ms.arm_of("C"))

    def test_the_rest_arm_reads_the_operand(self):
        program = members_program("over(op: A; rest)")
        self.assertEqual(program.diagnostics, ())
        self.assertEqual(list(program.arms), ["F.Fast_A", "F.Fast"])
        self.assertEqual(guard_args(program.arms["F.Fast"])[0], rop_model.Name("op"))
        self.assertEqual(program.member_sets["F.Fast"].arm_of("C"), "F.Fast")

    def test_refusals(self):
        cases = [
            ("over(x: A)", "over(x): not an Enum<E> operand of family F"),
            ("over(op: A, D)", "over(op): D not a member of Op"),
            ("over(op: A, A)", "over(op): a member is listed twice"),
            ("over(op: A, B, C; rest)", "over(op): rest, but every member of Op is listed"),
        ]
        for over, message in cases:
            with self.subTest(over=over):
                program = members_program(over)
                self.assertEqual([d.message for d in program.diagnostics], [message])

    def test_the_clause_prints_as_parsed(self):
        source = MEMBERS.replace("OVER", "over(op: A, B; rest)")
        (arm,) = [d for d in rop_syntax.parse(source, "m.rop").decls
                  if isinstance(d, rop_model.Arm)]
        self.assertIn("over(op: A, B; rest)", "\n".join(rop_print.decl_lines(arm)))

    def test_branch_truth_arms_take_their_mode_as_a_constant(self):
        p = products()
        ms = p.program.member_sets["BranchTruth.Bool"]
        self.assertEqual(ms.arms, {"BRANCH_IF_FALSE": "BranchTruth.Bool_BRANCH_IF_FALSE",
                                   "BRANCH_IF_TRUE": "BranchTruth.Bool_BRANCH_IF_TRUE"})
        self.assertNotIn("BranchTruth.Bool", p.program.arms)
        out = io.StringIO()
        rop_generator.write_cases(out, p, [])
        text = out.getvalue()
        body = text[text.index("TARGET(BranchTruth_Bool_BRANCH_IF_TRUE)"):]
        body = body[:body.index("\n        }\n")]
        self.assertIn("_PyRop_BranchTaken(ROP_BRANCHMODE_BRANCH_IF_TRUE, hit)", body)
        # The mode sits in field C: the arm does not read it
        self.assertNotIn("IR_C(ir_word)", body)
        # The generic arm installs the member arm of the word's mode
        self.assertIn("_PyRop_Members_BranchTruth_Bool[(uint32_t)IR_C(ir_word)]", text)

    def test_the_table_maps_an_unlisted_member_to_the_generic_arm(self):
        out = io.StringIO()
        rop_generator.write_member_tables(out, products().program)
        self.assertIn(
            "static const uint16_t _PyRop_Members_BranchTruth_Bool[4] = {"
            "ROP_ARM(BranchTruth_Bool_BRANCH_IF_FALSE), "
            "ROP_ARM(BranchTruth_Bool_BRANCH_IF_TRUE), ROP_ARM(BranchTruth_Generic), "
            "ROP_ARM(BranchTruth_Generic)};", out.getvalue())


ENVIRONMENT = """\
family F(dst:Dst, x:Src) exits(next, miss) generic(Any);
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
arm F.Any shared effects() feedback() exits(next)
{
  entry():
    exit next();
}
"""


LOCALS = """\
enum Op(A, B, C);
family F(dst:Dst, x:Src, y:Src, op:Enum<Op>) exits(next, miss);
extern primitive op_ok(op:Enum<Op>, v:B) -> Bool
  effects()
  c op_ok_c as bool
  evidence(review: "r", contract: "c");
arm F.Fast shared OVERS effects() feedback() exits(next, miss)
{
  entry():
    guard op_ok(op, use(x)) else miss;
    exit next();
}
"""


def locals_program(overs: str) -> "rop_check.Program":
    return rop_check.check([rop_syntax.parse(LOCALS.replace("OVERS", overs), "m.rop")])


class LocalsArmTests(unittest.TestCase):
    """Slot-class members (`over(locals: ...)`): one arm per combination of
    the family's Src and Dst homes, T or L each, read from the word's flags"""

    def test_each_member_is_an_arm_with_its_letters(self):
        program = locals_program("over(locals: TLL, TTL; rest)")
        self.assertEqual(program.diagnostics, ())
        self.assertEqual(list(program.arms), ["F.Fast_TLL", "F.Fast_TTL", "F.Fast"])
        self.assertEqual(program.arms["F.Fast_TLL"].locals, "TLL")
        self.assertIsNone(program.arms["F.Fast"].locals)
        ls = program.locals_sets["F.Fast"]
        self.assertEqual([name for name, _ in ls.operands], ["dst", "x", "y"])
        self.assertEqual(rop_check.fixed_locals(program, program.arms["F.Fast_TTL"]),
                         {"dst": False, "x": False, "y": True})

    def test_the_row_reads_the_operands_flags(self):
        ls = locals_program("over(locals: TLL; rest)").locals_sets["F.Fast"]
        # dst, x and y sit in fields A, B and C: bits 0, 1 and 2 of the
        # nibble; field D's bit is not an operand's
        row = ls.row()
        self.assertEqual({i for i, arm in enumerate(row) if arm == "F.Fast_TLL"}, {6, 14})
        self.assertEqual(set(row) - {"F.Fast_TLL"}, {"F.Fast"})

    def test_a_qualified_member_belongs_to_one_member_arm(self):
        program = locals_program("over(op: A, B) over(locals: A.TLL, TTT; rest)")
        self.assertEqual(program.diagnostics, ())
        self.assertEqual(list(program.arms), ["F.Fast_A_TLL", "F.Fast_A_TTT", "F.Fast_A",
                                              "F.Fast_B_TTT", "F.Fast_B"])
        self.assertEqual(program.locals_sets["F.Fast_B"].arms, {"TTT": "F.Fast_B_TTT"})

    def test_refusals(self):
        cases = [
            ("over(locals: TL; rest)", "over(locals): member TL does not match F's "
             "locals operands (dst, x, y): 3 letters, each T or L"),
            ("over(locals: TLX; rest)", "over(locals): member TLX does not match F's "
             "locals operands (dst, x, y): 3 letters, each T or L"),
            ("over(op: A) over(locals: C.TLL; rest)", "over(locals): C in C.TLL is not "
             "a member of the arm's over(...) (one of A)"),
            ("over(locals: B.TLL; rest)", "over(locals): B in B.TLL is not a member of "
             "the arm's over(...)"),
            ("over(locals: TLL, TLL; rest)", "over(locals): a member is listed twice "
             "for one arm"),
            ("over(locals: TLL)", "over(locals): without rest, every combination of "
             "dst, x, y is a member: an install meets any word"),
            ("over(op: A) over(op: B)", "over(op): an arm states one set over an enum "
             "operand; a second one is over locals"),
        ]
        for overs, message in cases:
            with self.subTest(overs=overs):
                program = locals_program(overs)
                self.assertEqual([d.message for d in program.diagnostics], [message])

    def test_the_clause_prints_as_parsed(self):
        source = LOCALS.replace("OVERS", "over(op: A) over(locals: A.TLL; rest)")
        (arm,) = [d for d in rop_syntax.parse(source, "m.rop").decls
                  if isinstance(d, rop_model.Arm)]
        self.assertIn("over(op: A) over(locals: A.TLL; rest)",
                      "\n".join(rop_print.decl_lines(arm)))

    def test_a_member_arm_tests_no_flag_and_closes_no_vacant_destination(self):
        p = products()
        out = io.StringIO()
        rop_generator.write_cases(out, p, [])
        text = out.getvalue()
        body = text[text.index("TARGET(GetAttr_Shape_TL)"):]
        body = body[:body.index("\n        }\n")]
        # obj is a local: no flag test, no take; dst is a vacant temporary
        self.assertNotIn("if (!(ir_word & IR_F_LOCAL", body)
        self.assertNotIn("old_dst", body)
        self.assertIn("assert(PyValue_IsNull(*home_dst));", body)
        self.assertIn("assert((ir_word & (IR_F_LOCAL_A | IR_F_LOCAL_B)) == "
                      "(IR_F_LOCAL_B));", body)
        rest = text[text.index("TARGET(GetAttr_Shape)"):]
        rest = rest[:rest.index("\n        }\n")]
        self.assertIn("if (!(ir_word & IR_F_LOCAL_B))", rest)
        self.assertIn("old_dst = *home_dst;", rest)

    def test_only_a_member_drops_an_old_value(self):
        # A projection is a temporary but not vacant: an arm without members
        # (ForIter.Generic's pair cursor) loads and closes its old value
        p = products()
        out = io.StringIO()
        rop_generator.write_cases(out, p, [])
        text = out.getvalue()
        body = text[text.index("TARGET(ForIter_Generic)"):]
        body = body[:body.index("\n        }\n")]
        self.assertIn("old_pair_cursor = *home_pair_cursor;", body)
        self.assertNotIn("assert(PyValue_IsNull(", body)

    def test_the_metadata_lists_rows_and_declared_arms(self):
        entries = {e.dotted: e for e in rop_generator.arm_entries(products())}
        self.assertEqual(entries["GetAttr.Shape_TL"].declared, "GetAttr_Shape")
        row = entries["GetAttr.Shape"].locals_row
        self.assertEqual(len(row), 16)
        self.assertEqual(set(row), {"GetAttr_Shape", "GetAttr_Shape_TL"})


class EnvironmentTests(unittest.TestCase):
    def test_table_covers_implicit_names_the_checker_knows(self):
        import rop_catalog
        self.assertLessEqual(set(rop_catalog.ENVIRONMENT_C), set(rop_catalog.ENVIRONMENT))

    def test_each_name_has_its_c_expression_in_the_cases(self):
        import io
        program = rop_check.check([rop_syntax.parse(ENVIRONMENT, "t.rop")])
        self.assertEqual(program.diagnostics, ())
        p = rop_generator.lower_all(program, rop_generator.number_arms(program))
        self.assertEqual(p.left_out, {})
        out = io.StringIO()
        rop_generator.write_cases(out, p, [])
        self.assertIn("probe_c(tstate, frame, this_instr, next_instr)", out.getvalue())


INDEXED = """\
family F(dst:Dst, x:Src, fb:Fb<Attr>) exits(next, miss);
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

    def test_stencil_decls(self):
        out = io.StringIO()
        rop_generator.write_stencil_decls(out, self.products, [])
        rows = [line for line in out.getvalue().splitlines() if line.startswith("IR_JIT_DECL(")]
        self.assertEqual(len(rows), len(self.products.lowered))
        self.assertIn('IR_JIT_DECL(Add_Int_TLL, Add_Generic, "A B C D", "miss next raise")', rows)
        self.assertIn('IR_JIT_DECL(Jump_Poll, -, "A B", "jump raise redirect")', rows)

    def test_generated_files_are_current(self):
        for name, writer in (("Python/generated_rop_cases.c.h", rop_generator.write_cases),
                             ("Python/generated_rop_completions.c.h",
                              rop_generator.write_completions),
                             ("Python/generated_jit_stencil_decls.h",
                              rop_generator.write_stencil_decls)):
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


class LayoutTests(unittest.TestCase):
    """The arms read feedback entries through rop_catalog.FB_LAYOUT, the
    one statement of each field's C member and type"""

    @classmethod
    def setUpClass(cls):
        cls.products = products()

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


if __name__ == "__main__":
    unittest.main()
