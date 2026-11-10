"""Positive producer and borrowed-probe proofs for terminal leaf calls."""

import io
from pathlib import Path
import tempfile
import unittest

from test import test_generated_cases as cases


class LeafCodegenTests(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.source = Path(directory.name) / "leaf.c"
        self.runtime = (Path(__file__).resolve().parents[2] /
                        "Python" / "bytecodes.c").read_text()

    def generate(self, source):
        self.source.write_text(source)
        with cases.handle_stderr():
            analysis = cases.analyzer.analyze_files([str(self.source)])
        output = io.StringIO()
        cases.ir_generator.generate_ir([str(self.source)], analysis, output, False)
        return output.getvalue()

    def test_plain_exact_hit_is_terminal_before_child_initialization(self):
        output = self.generate(self.runtime)
        start = output.index("TARGET(CALL_PY_EXACT_ARGS) {")
        end = output.find("TARGET(", start + 1)
        handler = output[start:end]
        guard = handler.index("tstate->py_recursion_remaining <= 1")
        probe = handler.index("ir_try_complete_leaf_call(")
        terminal = handler.index("DISPATCH();", probe)
        initialize = handler.index("_PyFrame_PushOverlapping(", probe)
        self.assertLess(guard, probe)
        self.assertLess(handler.index("leaf_code->_co_exec->leaf != NULL"), probe)
        self.assertIn("ir_try_complete_leaf_call(tstate, frame, this_instr, fb->func_version)", handler)
        self.assertLess(probe, terminal)
        self.assertLess(terminal, initialize)
        self.assertIn("next_instr = leaf.next;", handler[probe:terminal])
        self.assertNotIn("&leaf", handler)
        self.assertIn("callable = window[0];", handler)
        self.assertIn("self_or_null = window[1];", handler)
        self.assertIn("args = &window[2];", handler)
        self.assertNotIn("IR_MOVE_TO_DST", handler[probe:terminal])
        for name in ("CALL_BOUND_METHOD_EXACT_ARGS", "CALL_PY_GENERAL"):
            start = output.index(f"TARGET({name}) {{")
            end = output.find("TARGET(", start + 1)
            self.assertNotIn("ir_try_complete_leaf_call(", output[start:end])

    def test_missing_mutated_or_reordered_prefix_rejects(self):
        original = ("            _CHECK_STACK_SPACE +\n"
                    "            _CHECK_RECURSION_REMAINING +\n"
                    "            _TRY_LEAF_CALL +")
        replacements = (
            "            _TRY_LEAF_CALL +",
            "            _TRY_LEAF_CALL +\n"
            "            _CHECK_STACK_SPACE +\n"
            "            _CHECK_RECURSION_REMAINING +",
        )
        for replacement in replacements:
            with self.subTest(replacement=replacement):
                self.assertIn(original, self.runtime)
                with self.assertRaisesRegex(SyntaxError, "plain exact prefix"):
                    self.generate(self.runtime.replace(original, replacement))
        with self.assertRaisesRegex(SyntaxError, "prefix"):
            self.generate(self.runtime.replace(
                "DEOPT_IF(tstate->py_recursion_remaining <= 1);",
                "DEOPT_IF(tstate->py_recursion_remaining <= 0);"))

    def test_bound_method_cannot_inherit_marker(self):
        source = self.runtime.replace(
            "        macro(CALL_BOUND_METHOD_EXACT_ARGS) =\n",
            "        macro(CALL_BOUND_METHOD_EXACT_ARGS) =\n"
            "            _TRY_LEAF_CALL +\n")
        with self.assertRaisesRegex(SyntaxError, "plain exact prefix"):
            self.generate(source)

    def test_typed_probe_source_cannot_hide_effect_or_wrong_guard(self):
        for old, new in (
            ("ir_probe_shape_value", "unproved_shape_reader"),
            ("ir_probe_shape_value(owner, type_version, shape, offset)",
             "ir_probe_shape_value(attr, type_version, shape, offset)"),
            ("ir_probe_shape_value(owner, type_version, shape, offset)",
             "ir_probe_shape_value(owner, type_version, shape, 0)"),
            ("owner, type_version, class_version, class_value, true",
             "owner, type_version, class_version, class_value, false"),
            ("            attr = IR_CANONICAL_HOME_DUP(found);",
             "            PyObject_CallNoArgs(NULL);\n"
             "            attr = IR_CANONICAL_HOME_DUP(found);"),
        ):
            with self.subTest(change=new):
                with self.assertRaises((SyntaxError, ValueError)):
                    self.generate(self.runtime.replace(old, new))

    def test_computation_sharing_keeps_ordinary_admission(self):
        output = self.generate(self.runtime)
        for kind in ("INT", "FLOAT"):
            start = output.index(f"TARGET(COMPARE_OP_{kind}) {{")
            end = output.find("TARGET(", start + 1)
            handler = output[start:end]
            self.assertIn(f"ir_compare_inline_{kind.lower()}(oparg, left, right)", handler)
            self.assertNotIn("ir_probe_inline_compare", handler)
        start = output.index("TARGET(COMPARE_OP) {")
        end = output.find("TARGET(", start + 1)
        self.assertIn("_PyValue_RichCompare(", output[start:end])

    def test_full_read_probes_replace_redundant_guard_prefixes(self):
        self.source.write_text(self.runtime)
        analysis = cases.analyzer.analyze_files([str(self.source)])
        for name in ("LOAD_ATTR_SHAPE", "LOAD_ATTR_CLASS",
                     "LOAD_ATTR_CLASS_WITH_METACLASS_CHECK", "LOAD_GLOBAL_BUILTIN"):
            with self.subTest(name=name):
                parts = [p.name for p in analysis.instructions[name].parts
                         if isinstance(p, cases.analyzer.Uop)]
                self.assertNotIn("_GUARD_TYPE_VERSION", parts)
                self.assertNotIn("_GUARD_SHAPE", parts)
                self.assertNotIn("_CHECK_ATTR_CLASS", parts)
                self.assertNotIn("_GUARD_GLOBALS_VERSION", parts)


if __name__ == "__main__":
    unittest.main()
