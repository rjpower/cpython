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
                effects = ir_records.infer_effects(uop)
                self.assertFalse(effects & {WRITES_HEAP, MAY_RUN_PYTHON}, name)

    def test_effects(self):
        for name, effects in (
                ("_BINARY_OP_ADD_INT", {ALLOCATES, MAY_RAISE}),
                ("_COMPARE_OP_INT", set()),
                ("_LOAD_SMALL_INT", set()),
                ("_GUARD_TYPE_VERSION", {READS_HEAP}),
                # A close whose deallocator cannot run Python only frees
                ("_POP_TOP_INT", {WRITES_HEAP}),
                ("_POP_TOP", {MAY_RELEASE}),
                ("_INIT_CALL_PY_EXACT_ARGS", {ALLOCATES}),
                ("_STORE_ATTR_SLOT", {READS_HEAP, WRITES_HEAP, MAY_RELEASE}),
                ("_LIST_APPEND", set(EFFECTS))):
            with self.subTest(uop=name):
                self.assertEqual(ir_records.infer_effects(uops[name]), effects)

    def test_result_facts(self):
        def results(handler, uop):
            record = records[handler]
            [found] = [u for u in record.uops if u.name == uop]
            return {(f.kind, f.arg) for f in found.results}

        integer = ("EXACT_TYPE", "PyLong_Type")
        for handler, uop, facts in (
                ("BINARY_OP_ADD_INT", "_BINARY_OP_ADD_INT", {integer}),
                ("BINARY_OP_AND_INT", "_BINARY_OP_AND_INT", {integer, ("INLINE_INT", "")}),
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

        # A specific version, and a version without naming it
        self.assertIn("TYPE_VERSION(v0, type_version)",
                      facts("LOAD_ATTR_METHOD_WITH_VALUES", "_GUARD_TYPE_VERSION"))
        self.assertIn("TYPE_VERSION(v0)",
                      facts("LOAD_ATTR_METHOD_POLY", "_GUARD_TYPE_HAS_VERSION"))

    def test_poly_table_readers(self):
        # The poly execution variants and the handlers that call a
        # POLY_TABLE_READERS macro
        self.assertEqual(
            {name for name, record in records.items() if record.reads_poly_table},
            {"LOAD_ATTR_SHAPE_POLY", "LOAD_ATTR_SHAPE_POLY_LOCAL_TEMP",
             "LOAD_ATTR_METHOD_POLY"})
        self.assertIn(READS_HEAP, ir_records.infer_effects(uops["_LOAD_ATTR_METHOD_POLY"]))

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

    def test_guard_order_exceptions_are_exact(self):
        # build_records() raises if a handler's list differs; each entry is
        # a handler that exists
        handlers = {v[0] for v in ir_records.GUARD_ORDER_EXCEPTIONS}
        self.assertLessEqual(handlers, set(records))


class GeneratorTests(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.source = Path(directory.name) / "records.c"

    def generate(self, source):
        "The handlers of source, whose OP has the specialised form OP_INT"
        source += """
        inst(OP, (value -- res)) {
            assert(PyValue_IsObject(value));
            res = SPAM(PyValue_AsKnownObjectBorrow(value));
            PyValue_CLOSE(value);
        }
        family(OP, 0) = { OP_INT };
        """
        self.source.write_text(cases.parser.BEGIN_MARKER + "\n" + source + "\n" +
                               cases.parser.END_MARKER)
        with cases.handle_stderr():
            analysis = cases.analyzer.analyze_files(
                [str(self.source)], cases.SequentialNumbers())
        output = io.StringIO()
        ir_generator.generate_ir([str(self.source)], analysis, output, False)
        return output.getvalue()

    def test_reason_prints_the_same_c(self):
        source = """
        op(_GUARD, (value -- value)) {
            EXIT_IF(!PyValue_IsInlineInt(value)%s);
        }
        op(_USE, (value -- res)) {
            res = value;
            DEAD(value);
        }
        macro(OP_INT) = _GUARD + _USE;
        """
        self.assertEqual(self.generate(source % ""),
                         self.generate(source % ', "a reason"'))

    def test_unknown_guard_is_an_error(self):
        with self.assertRaisesRegex(SyntaxError, "establishes no fact"):
            self.generate("""
            op(_GUARD, (value -- value)) {
                EXIT_IF(SPAM_FLAG(value));
            }
            op(_USE, (value -- res)) {
                res = value;
                DEAD(value);
            }
            macro(OP_INT) = _GUARD + _USE;
            """)

    def test_pure_uop_must_not_write(self):
        with self.assertRaisesRegex(SyntaxError, "annotated pure but may WRITES_HEAP"):
            self.generate("""
            pure inst(OP_INT, (value -- res)) {
                assert(PyValue_IsObject(value));
                PyObject *o = PyValue_AsKnownObjectBorrow(value);
                o->ob_flags = 1;
                res = value;
                DEAD(value);
            }
            """)

    def test_guard_after_change_is_an_error(self):
        with self.assertRaisesRegex(SyntaxError, "guards that follow a change"):
            self.generate("""
            op(_WRITE, (value -- value)) {
                assert(PyValue_IsObject(value));
                PyObject *o = PyValue_AsKnownObjectBorrow(value);
                o->ob_flags = 1;
            }
            op(_GUARD, (value -- value)) {
                EXIT_IF(!PyValue_IsInlineInt(value));
            }
            op(_USE, (value -- res)) {
                res = value;
                DEAD(value);
            }
            macro(OP_INT) = _WRITE + _GUARD + _USE;
            """)


if __name__ == "__main__":
    unittest.main()
