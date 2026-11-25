"""The operation records of the IR handlers (Tools/cases_generator/ir_records.py)."""

import io
from pathlib import Path
import re
import tempfile
import unittest
from unittest import mock

from test import test_generated_cases as cases
from test import test_tools

ir_generator = cases.ir_generator
with test_tools.imports_under_tool("cases_generator"):
    import ir_records
    from ir_records import (ALLOCATES, EFFECTS, MAY_RAISE, MAY_RELEASE,
                            MAY_RUN_PYTHON, READS_HEAP, WRITES_HEAP)

BYTECODES = Path(test_tools.basepath) / "Python" / "bytecodes.c"


def setUpModule():
    global analysis, records, uops
    with cases.handle_stderr():
        analysis = cases.analyzer.analyze_files([str(BYTECODES)])
    records = ir_generator.build_records(
        analysis, ir_generator.compute_formats(analysis))
    uops = {part.name: part for inst in ir_generator.handler_instructions(analysis).values()
            for part in inst.parts if isinstance(part, cases.analyzer.Uop)}


class HandlerRecordTests(unittest.TestCase):
    def test_every_handler_has_a_record(self):
        self.assertEqual(set(records), set(ir_generator.handler_instructions(analysis)))

    def test_every_guard_has_facts_or_a_reason(self):
        for record in records.values():
            for uop in record.uops:
                for guard in uop.guards:
                    with self.subTest(handler=record.handler, uop=uop.name,
                                      line=guard.site and guard.site.line):
                        self.assertNotEqual(bool(guard.facts), bool(guard.reason))
                        for fact in guard.facts:
                            self.assertIn(fact.kind, ir_records.FACT_KINDS)

    def test_every_uop_has_closed_effects(self):
        for record in records.values():
            for uop in record.uops:
                self.assertLessEqual(uop.effects, set(EFFECTS))
                if MAY_RUN_PYTHON in uop.effects:
                    self.assertEqual(uop.effects, set(EFFECTS), uop.name)

    def test_every_call_has_known_effects(self):
        # A call that the tables of ir_records do not know may do anything
        for name, uop in uops.items():
            unknown = [t.text for t, e in ir_records.effect_sites(uop) if e is None]
            self.assertEqual(unknown, [], name)

    def test_pure_uops_change_nothing(self):
        for name, uop in uops.items():
            if "pure" in uop.annotations:
                effects = ir_records.uop_record_effects(uop)
                self.assertFalse(effects & {WRITES_HEAP, MAY_RUN_PYTHON}, name)

    def test_exchanged_outputs_are_no_pass_throughs(self):
        # SWAP's outputs keep their homes but take each other's values
        # (moved(top), moved(bottom)): they are no pass-throughs, so the
        # template moves them (an empty jit_template_SWAP was the bug)
        uop = uops["_SWAP"]
        passes = {item.name: ir_records.passes(uop, item) for item in uop.stack.outputs}
        self.assertEqual(passes, {"bottom": False, "unused": True, "top": False})

    def test_effects(self):
        for name, effects in (
                ("_BINARY_OP_ADD_INT", {ALLOCATES, MAY_RAISE}),
                ("_COMPARE_OP_INT", set()),
                ("_LOAD_SMALL_INT", set()),
                ("_GUARD_TYPE_VERSION", {READS_HEAP}),
                # A close whose deallocator cannot run Python only frees
                ("_POP_TOP_INT", {WRITES_HEAP}),
                ("_POP_TOP", {MAY_RELEASE}),
                ("_INIT_CALL_PY_EXACT_ARGS", {ALLOCATES, READS_HEAP, WRITES_HEAP, MAY_RELEASE}),
                # The expansion only sets CALL_EXPANDED: the push reads the
                # method's function and receiver after the last guard
                ("_INIT_CALL_BOUND_METHOD_EXACT_ARGS", set()),
                ("_STORE_ATTR_SLOT", {READS_HEAP, WRITES_HEAP, MAY_RELEASE}),
                ("_LIST_APPEND", set(EFFECTS))):
            with self.subTest(uop=name):
                self.assertEqual(ir_records.uop_record_effects(uops[name]), effects)

    def test_result_facts(self):
        def results(handler, uop):
            record = records[handler]
            [found] = [u for u in record.uops if u.name == uop]
            return {(f.kind, f.arg) for f in found.results}

        integer = ("EXACT_TYPE", "PyLong_Type")
        for handler, uop, facts in (
                ("BINARY_OP_ADD_INT", "_BINARY_OP_ADD_INT", {integer}),
                ("BINARY_OP_AND_INT", "_BINARY_OP_AND_INT", {integer, ("ENCODING", "inline_int")}),
                ("BINARY_OP_ADD_FLOAT", "_BINARY_OP_ADD_FLOAT",
                 {("EXACT_TYPE", "PyFloat_Type")}),
                ("COMPARE_OP_INT", "_COMPARE_OP_INT", {("EXACT_TYPE", "PyBool_Type")}),
                ("CALL_LEN", "_CALL_LEN", {integer}),
                ("CALL_ISINSTANCE", "_CALL_ISINSTANCE", {("EXACT_TYPE", "PyBool_Type")}),
                ("TO_BOOL_STR", "_TO_BOOL_STR", {("EXACT_TYPE", "PyBool_Type")}),
                # A rich comparison may return any object
                ("COMPARE_OP", "_COMPARE_OP", set())):
            with self.subTest(uop=uop):
                self.assertEqual(results(handler, uop), facts)

    def test_type_version_facts(self):
        def facts(handler, uop):
            [found] = [u for u in records[handler].uops if u.name == uop]
            return {str(f) for g in found.guards for f in g.facts}

        # A specific version
        self.assertIn("TYPE_VERSION(v1, type_version)",
                      facts("STORE_ATTR_SHAPE", "_GUARD_TYPE_VERSION"))

    def test_poly_table_readers(self):
        # The poly arms are register instruction arms: no record of an
        # old handler reads a poly table
        self.assertEqual(
            {name for name, record in records.items() if record.reads_poly_table},
            set())

    def test_feedback_operands_are_schema_fields(self):
        for record in records.values():
            for uop in record.uops:
                for operand in uop.feedback:
                    self.assertIsNotNone(record.feedback, record.handler)
                    self.assertIn(operand.field, record.feedback.fields)

    def test_schema_matches_typedefs(self):
        types = ir_records.feedback_types()
        self.assertEqual(types["_PyFbBinary"][0].name, "h.kind")
        for record in records.values():
            if record.feedback is not None:
                self.assertEqual(record.feedback.fields,
                                 tuple(types[record.feedback.type]))

    def test_guards_precede_changes(self):
        # build_records() raises otherwise; a frame switch is at its token,
        # so the generator's guards precede it (contract87 TR)
        self.assertNotHasAttr(ir_records, "GUARD_ORDER_EXCEPTIONS")
        instructions = ir_generator.handler_instructions(analysis)
        for name in ("FOR_ITER_GEN", "SEND_GEN", "CALL_BOUND_METHOD_EXACT_ARGS",
                     "CALL_BOUND_METHOD_GENERAL", "CALL_EX_PY", "CALL_BUILTIN_FAST",
                     "CALL_METHOD_DESCRIPTOR_FAST"):
            with self.subTest(handler=name):
                parts = [p for p in instructions[name].parts
                         if isinstance(p, cases.analyzer.Uop)]
                record = records[name]
                self.assertEqual(
                    ir_records.guard_order(parts[:len(record.uops)], record.uops), [])

    def test_transfers_are_typed(self):
        # Every way out of a uop is a typed transfer; none enters the
        # evaluator (tiers87, C3)
        self.assertNotHasAttr(ir_records, "EVALUATOR")
        for record in records.values():
            for uop in record.uops:
                for transfer in uop.transfers:
                    with self.subTest(handler=record.handler, uop=uop.name):
                        self.assertIn(transfer.kind, ir_records.TRANSFER_KINDS)
                        if transfer.site is not None and transfer.site.kind == "GOTO":
                            self.assertIn(transfer.target, ir_records.ERROR_LABELS)

    def test_jumps_to_the_same_word(self):
        def transfers(handler):
            return {(t.kind, t.target) for u in records[handler].uops for t in u.transfers}
        # A specializer that rewrote the word runs it again
        self.assertIn((ir_records.JUMP, "self"), transfers("TO_BOOL"))
        # The monitor runs the word with the handler that it chose
        self.assertIn((ir_records.JUMP, "self:handler"), transfers("INSTRUMENTED_INSTRUCTION"))

    def test_native_send_yields(self):
        # A send to a native iterator that suspends yields from the outer
        # frame where the handler would fall through; a send to a generator
        # always enters its frame
        for consumer in ("DISCARD", "STORE_LOCAL", "RETURN", "HOME"):
            for handler in ("SEND", "SEND_VIRTUAL", "SEND_ASYNC_GEN"):
                last = records[f"{handler}_XFER_{consumer}"].uops[-1].transfers[-1]
                self.assertEqual((last.kind, last.target), (ir_records.YIELD, "delegated"))
            uops = records[f"SEND_GEN_XFER_{consumer}"].uops
            kinds = [t.kind for u in uops for t in u.transfers]
            self.assertNotIn(ir_records.YIELD, kinds)
            # The last uop enters, and nothing runs after it in this frame
            last = {t.kind for t in uops[-1].transfers}
            self.assertIn(ir_records.ENTER_FRAME, last)
            self.assertNotIn(ir_records.NEXT, last)


# The stand-in of the base handler OP: a function that reads its argument
# and makes a new value
GENERATOR_ROWS = """
#define GENERATOR_ROWS(PRIMITIVE, HELPER, MACRO) \\
    MACRO(SPAM_VALUE, ARGS(scalar), VALUE(owned), EFFECTS(reads), EXITS())
"""


class GeneratorTests(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.source = Path(directory.name) / "records.c"
        # SPAM_FLAG and its kin are stand-ins whose rows declare no reentry
        rows = mock.patch.dict(cases.ir_contract.rows(),
                               cases.ir_contract.parse_rows(cases.TEST_ROWS) |
                               cases.ir_contract.parse_rows(GENERATOR_ROWS))
        rows.start()
        self.addCleanup(rows.stop)

    def generate(self, source):
        "The handlers of source, whose OP has the specialised form OP_INT"
        source += """
        inst(OP, (value: consumed object -- res: owned))
            effects(reads, releases) exits(next)
        {
            res = SPAM_VALUE(PyValue_AsKnownObjectBorrow(value));
            PyValue_CLOSE(value);
        }
        family(OP, 0) = { OP_INT };
        """
        self.source.write_text(cases.parser.BEGIN_MARKER + "\n" + source + "\n" +
                               cases.parser.END_MARKER)
        with cases.handle_stderr():
            analysis = cases.analyzer.analyze_files(
                [str(self.source)])
        output = io.StringIO()
        ir_generator.generate_ir([str(self.source)], analysis, output, False)
        return output.getvalue()

    def test_reason_prints_the_same_c(self):
        source = """
        op(_GUARD, (value -- value: inline_int)) effects() exits(next, miss) {
            EXIT_IF(!PyValue_IsInlineInt(value)%s);
        }
        op(_USE, (value: consumed -- res: moved(value))) effects() exits(next) {
            res = value;
            DEAD(value);
        }
        macro(OP_INT) = _GUARD + _USE;
        """
        self.assertEqual(self.generate(source % ""),
                         self.generate(source % ', "a reason"'))

    def test_unknown_guard_is_an_error(self):
        # The verifier refuses the guard before ir_records.check_record can
        # find that it establishes no fact
        with self.assertRaisesRegex(SyntaxError, "is no combination of TEST rows"):
            self.generate("""
            op(_GUARD, (value -- value)) effects() exits(next, miss) {
                EXIT_IF(SPAM_FLAG(value));
            }
            op(_USE, (value: consumed -- res: moved(value))) effects() exits(next) {
                res = value;
                DEAD(value);
            }
            macro(OP_INT) = _GUARD + _USE;
            """)

    def test_pure_uop_must_not_write(self):
        # A declared uop's effects are its record's, so the contract refuses
        # a pure uop that declares a write before check_record sees one
        with self.assertRaisesRegex(SyntaxError, "annotated pure but declares writes"):
            self.generate("""
            pure inst(OP_INT, (value: consumed object -- res: moved(value)))
                effects(reads, writes) exits(next)
            {
                PyObject *o = PyValue_AsKnownObjectBorrow(value);
                o->ob_flags = 1;
                res = value;
                DEAD(value);
            }
            """)

    def test_guard_after_change_is_an_error(self):
        with self.assertRaisesRegex(SyntaxError, "guards that follow a change"):
            self.generate("""
            op(_WRITE, (value: object -- value)) effects(reads, writes) exits(next) {
                PyObject *o = PyValue_AsKnownObjectBorrow(value);
                o->ob_flags = 1;
            }
            op(_GUARD, (value -- value: inline_int)) effects() exits(next, miss) {
                EXIT_IF(!PyValue_IsInlineInt(value));
            }
            op(_USE, (value: consumed -- res: moved(value))) effects() exits(next) {
                res = value;
                DEAD(value);
            }
            macro(OP_INT) = _WRITE + _GUARD + _USE;
            """)

    def test_goto_into_the_evaluator_is_an_error(self):
        # The walk models a goto to an error label only, so it refuses this
        # one before uop_transfers can name the label
        with self.assertRaisesRegex(SyntaxError, "`goto` is a form the walk does not model"):
            self.generate("""
            inst(OP_INT, (value: consumed -- res: moved(value)))
                effects() exits(next)
            {
                if (SPAM_FLAG()) {
                    goto start_frame;
                }
                res = value;
                DEAD(value);
            }
            """)

    def test_evaluator_local_is_an_error(self):
        with self.assertRaisesRegex(SyntaxError, "only the evaluator has"):
            self.generate("""
            inst(OP_INT, (value: consumed -- res: moved(value)))
                effects() exits(next)
            {
                SPAM_KEEP(lastopcode);
                res = value;
                DEAD(value);
            }
            """)


if __name__ == "__main__":
    unittest.main()
