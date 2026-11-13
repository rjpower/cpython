import contextlib
import io
import os
from pathlib import Path
import re
import sys
import tempfile
import unittest
from unittest import mock

from test import support
from test import test_tools


def skip_if_different_mount_drives():
    if sys.platform != "win32":
        return
    ROOT = os.path.dirname(os.path.dirname(__file__))
    root_drive = os.path.splitroot(ROOT)[0]
    cwd_drive = os.path.splitroot(os.getcwd())[0]
    if root_drive != cwd_drive:
        # May raise ValueError if ROOT and the current working
        # different have different mount drives (on Windows).
        raise unittest.SkipTest(
            f"the current working directory and the Python source code "
            f"directory have different mount drives "
            f"({cwd_drive} and {root_drive})"
        )


skip_if_different_mount_drives()


test_tools.skip_if_missing("cases_generator")
with test_tools.imports_under_tool("cases_generator"):
    import analyzer
    from analyzer import StackItem
    from cwriter import CWriter
    import ir_generator
    import stack
    import ir_metadata_generator
    import ir_numbers
    import lexer
    import parser
    from stack import Local, Stack


class SequentialNumbers(ir_numbers.NumberTable):
    """Numbers the definitions of a test input in order of request, in place
    of the pinned table, which lists only the handlers of Python/bytecodes.c"""

    def __init__(self):
        super().__init__(constants={
            "HAVE_ARGUMENT": 40, "MIN_INSTRUMENTED_OPCODE": 233,
            "IR_FIRST_ONLY_HANDLER": 0})
        self.pseudos = {}

    @property
    def first_ir_only(self):
        # The IR-only handlers follow the pseudo-instructions
        return 1 + max(self.pseudos.values(), default=255)

    def number(self, name, low, high, what):
        if name not in self.numbers:
            self.numbers[name] = self.next_free(low, 4096)
        if what == "pseudo-instruction":
            self.pseudos[name] = self.numbers[name]
        return self.numbers[name]


def handle_stderr():
    if support.verbose > 1:
        return contextlib.nullcontext()
    else:
        return support.captured_stderr()


def parse_src(src):
    p = parser.Parser(src, "test.c")
    nodes = []
    while node := p.definition():
        nodes.append(node)
    return nodes


class TestEffects(unittest.TestCase):
    def test_effect_sizes(self):
        stack = Stack()
        inputs = [
            x := StackItem("x", "1"),
            y := StackItem("y", "oparg"),
            z := StackItem("z", "oparg*2"),
        ]
        outputs = [
            StackItem("x", "1"),
            StackItem("b", "oparg*4"),
            StackItem("c", "1"),
        ]
        null = CWriter.null()
        stack.pop(z, null)
        stack.pop(y, null)
        stack.pop(x, null)
        for out in outputs:
            stack.push(Local.undefined(out))
        self.assertEqual(stack.base_offset.to_c(), "-1 - oparg - oparg*2")
        self.assertEqual(stack.physical_sp.to_c(), "0")
        self.assertEqual(stack.logical_sp.to_c(), "1 - oparg - oparg*2 + oparg*4")


class TestAnalysisErrors(unittest.TestCase):
    """Definitions that the generator rejects"""

    def setUp(self) -> None:
        super().setUp()
        fd, self.temp_input_filename = tempfile.mkstemp(suffix=".c")
        os.close(fd)

    def tearDown(self) -> None:
        os.remove(self.temp_input_filename)
        super().tearDown()

    def generate(self, input: str):
        with open(self.temp_input_filename, "w") as temp_input:
            temp_input.write(parser.BEGIN_MARKER)
            temp_input.write(input)
            temp_input.write(parser.END_MARKER)
        with handle_stderr():
            analysis = analyzer.analyze_files(
                [self.temp_input_filename], SequentialNumbers())
            ir_generator.generate_ir([self.temp_input_filename], analysis,
                                     io.StringIO(), False)

    def check_error(self, input: str, regex: str = ""):
        with self.assertRaisesRegex(SyntaxError, regex):
            self.generate(input)

    def test_pep7_condition(self):
        self.check_error("""
        inst(OP, (arg1 -- out)) {
            if (arg1)
                out = 0;
            else {
                out = 1;
            }
        }
        """)

    def test_deopt_and_exit(self):
        self.check_error("""
        pure op(OP, (arg1 -- out)) {
            DEOPT_IF(1);
            EXIT_IF(1);
        }
        """)

    def test_unused_cached_value(self):
        self.check_error("""
        op(FIRST, (arg1 -- out)) {
            out = arg1;
        }

        op(SECOND, (unused -- unused)) {
        }

        macro(BOTH) = FIRST + SECOND;
        """)

    def test_scalar_array_inconsistency(self):
        self.check_error("""
        op(FIRST, ( -- a)) {
            a = 1;
        }

        op(SECOND, (a[1] -- b)) {
            b = 1;
        }

        macro(TEST) = FIRST + SECOND;
        """)

    def test_array_size_inconsistency(self):
        self.check_error("""
        op(FIRST, ( -- a[2])) {
            a[0] = 1;
        }

        op(SECOND, (a[1] -- b)) {
            b = 1;
        }

        macro(TEST) = FIRST + SECOND;
        """)

    def test_stack_save_only(self):
        self.check_error("""
        inst(BALANCED, ( -- )) {
            SAVE_STACK();
        }
        """)

    def test_instruction_size_macro(self):
        # Two instructions of different sizes referencing the same
        # uop containing the `INSTRUCTION_SIZE` macro is not allowed.
        self.check_error("""
        inst(OP, (--)) {
            frame->return_offset = INSTRUCTION_SIZE;
        }
        macro(OP2) = unused/1 + OP;
        """, "All instructions containing a uop")

    def test_no_escaping_calls_in_branching_macros(self):
        for macro in "DEOPT_IF", "EXIT_IF", "ERROR_IF":
            with self.subTest(macro=macro):
                self.check_error(f"""
        inst(OP, ( -- )) {{
            {macro}(escaping_call());
        }}
        """)

    def test_kill_in_wrong_order(self):
        self.check_error("""
        inst(OP, (a, b -- c)) {
            c = b;
            PyValue_CLOSE(a);
            PyValue_CLOSE(b);
        }
        """)

    def test_incorrect_spills(self):
        self.check_error("""
        spilled label(one) {
            goto two;
        }

        label(two) {
        }
        """, ".*reload.*")
        self.check_error("""
        spilled label(one) {
        }

        label(two) {
            goto one;
        }
        """, ".*spill.*")


class TestIRCases(unittest.TestCase):
    """The register IR backend: ir_generator.py and ir_metadata_generator.py"""

    def setUp(self) -> None:
        super().setUp()
        self.maxDiff = None
        fd, self.temp_input_filename = tempfile.mkstemp(suffix=".c")
        os.close(fd)
        # The families of the tests that have feedback entries
        layouts = mock.patch.dict(ir_generator.FEEDBACK_LAYOUTS, {
            "OP": ir_generator.FB_HEADER, "BRANCH": ir_generator.FB_HEADER})
        layouts.start()
        self.addCleanup(layouts.stop)
        # SPAM stands for a function that reads its arguments (S3), and
        # spam_call for one that reads them and can run code
        readers = mock.patch.object(ir_generator, "OBJECT_READERS",
                                    ir_generator.OBJECT_READERS | {"SPAM", "spam_call"})
        readers.start()
        self.addCleanup(readers.stop)

    def tearDown(self) -> None:
        os.remove(self.temp_input_filename)
        super().tearDown()

    def analyze(self, input: str):
        with open(self.temp_input_filename, "w") as temp_input:
            temp_input.write(parser.BEGIN_MARKER)
            temp_input.write(input)
            temp_input.write(parser.END_MARKER)
        with handle_stderr():
            return analyzer.analyze_files(
                [self.temp_input_filename], SequentialNumbers())

    def formats(self, input: str):
        return ir_generator.compute_formats(self.analyze(input))

    def ir_output(self, input: str) -> str:
        analysis = self.analyze(input)
        out = io.StringIO()
        ir_generator.generate_ir([self.temp_input_filename], analysis, out, False)
        _, rest = out.getvalue().split(ir_generator.INSTRUCTION_START_MARKER)
        instructions, _ = rest.split(ir_generator.INSTRUCTION_END_MARKER)
        return instructions

    def handler(self, output: str, name: str) -> str:
        "The handler of name in output"
        start = output.index(f"TARGET({name}) {{")
        end = output.find("TARGET(", start + 1)
        return output[start:end if end >= 0 else len(output)]

    def test_error_ingress_keeps_native_bridge_owners(self):
        output = self.ir_output("""
        inst(OP, (value -- result)) {
            PyObject *object = PyValue_AsKnownObjectBorrow(value);
            PyObject *found = spam_call(object);
            DECREF_INPUTS();
            ERROR_IF(found == NULL);
            result = PyValue_FromObjectSteal(found);
        }
        """)
        body = self.handler(output, "OP")
        self.assertIn("_PyValueTemps *value_temps = &value_owners;", body)
        boxing = body.index("_PyValueTemps_Box(value_temps, value)")
        failure = body.index("PROCESS_ERROR_AT(RAISE, 0, this_instr);", boxing)
        self.assertLess(boxing, failure)
        self.assertNotIn("JUMP_TO_LABEL(error)", body)
        self.assertNotIn("finish_value_bridges", body)

    def test_error_labels_use_explicit_literal_ingresses(self):
        source = "\n" + "\n".join(
            f"label({name}) {{ IR_PROCESS_ERROR({ingress}, 0); }}"
            for name, ingress in ir_generator.ERROR_INGRESSES.items()) + "\n"
        analysis = self.analyze(source)
        output = io.StringIO()
        ir_generator.generate_ir([self.temp_input_filename], analysis, output, False)
        text = output.getvalue()
        for name, ingress in ir_generator.ERROR_INGRESSES.items():
            with self.subTest(ingress=ingress):
                origin = ("frame->instr_ptr" if ingress in {"EXIT", "EXIT_NOTRACE"}
                          else "next_instr - 1")
                self.assertIn(f"PROCESS_ERROR_AT({ingress}, 0, {origin});", text)
        with self.assertRaisesRegex(SyntaxError, "unsupported error label ingress"):
            self.ir_output("\nlabel(error) { IR_PROCESS_ERROR(RAISE, 1); }\n")

    def test_observer_proof_does_not_select_runtime_error_origin(self):
        source = (Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c").read_text()
        Path(self.temp_input_filename).write_text(source)
        analysis = analyzer.analyze_files([self.temp_input_filename])
        formats = ir_generator.compute_formats(analysis)
        instruction = analysis.instructions["INSTRUMENTED_LINE"]

        def emitted():
            output = io.StringIO()
            out = CWriter(output, 0, False)
            emitter = ir_generator.IREmitter(out, analysis, formats)
            ir_generator.generate_handler(instruction.name, instruction,
                                          formats[instruction.name], emitter, out, False)
            return output.getvalue()

        self.assertIsNotNone(ir_generator.observer_entry_source(
            instruction, formats[instruction.name]))
        expected = emitted()
        with mock.patch.object(ir_generator, "observer_entry_source", return_value=None):
            uncovered = emitted()
        self.assertEqual(expected, uncovered)
        self.assertIn("PROCESS_ERROR_AT(RAISE, 0, next_instr - 1)", expected)

    def test_dict_first_probe_contract(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()

        def generate(text):
            Path(self.temp_input_filename).write_text(text)
            with handle_stderr():
                analysis = analyzer.analyze_files([self.temp_input_filename])
            out = io.StringIO()
            ir_generator.generate_ir([self.temp_input_filename], analysis, out, False)
            return out.getvalue()

        body = self.handler(generate(source), "BINARY_OP_SUBSCR_DICT")
        provider = body.index("_PyDict_TryGetFirstProbeValue")
        consume = body.index("IR_CONSUME_INPUT", provider)
        publish = body.index("IR_STORE_CANONICAL_TEMP_DST", consume)
        key = body.index("_PyValue_MayDealloc(ir_leaf_key)", publish)
        dictionary = body.index("_PyValue_MayDealloc(ir_leaf_dict)", key)
        dispatch = body.index("DISPATCH();", dictionary)
        slow = body.index("value_subscript(held_dict, held_sub)", dispatch)
        self.assertLess(provider, consume)
        self.assertLess(dispatch, slow)
        self.assertNotIn("value_reader_complete", body[provider:dispatch])
        self.assertLess(body.index("IR_STORE_DST", dictionary), dispatch)

        marker = "IR_TRY_DICT_FIRST_PROBE();"
        guard = "DEOPT_IF(!Py_TYPE(o)->tp_as_mapping);"
        changes = (
            (marker, "STAT_INC(BINARY_OP, hit); " + marker),
            (marker, marker + " " + marker),
            (guard, "spam_call(); " + guard),
            ("(dict_st, sub_st -- res)", "(sub_st, dict_st -- res)"),
            (marker, "if (oparg) { " + marker + " }"),
        )
        for old, new in changes:
            with self.subTest(new=new):
                self.assertIn(old, source)
                with self.assertRaises(SyntaxError):
                    generate(source.replace(old, new, 1))

        boxes_into_record = ir_generator.boxes_into_record
        def box_dict_reader(code):
            return ("_PyDict_TryGetFirstProbeValue" in code or
                    boxes_into_record(code))
        with mock.patch.object(ir_generator, "boxes_into_record", box_dict_reader):
            with self.assertRaisesRegex(SyntaxError, "cannot use a value record"):
                generate(source)

    def test_guarded_python_call_contract(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()

        def generate(text):
            Path(self.temp_input_filename).write_text(text)
            with handle_stderr():
                analysis = analyzer.analyze_files([self.temp_input_filename])
            out = io.StringIO()
            ir_generator.generate_ir([self.temp_input_filename], analysis, out, False)
            return out.getvalue()

        output = generate(source)
        body = self.handler(output, "CALL_PY_GENERAL")
        success, fallback = body.split("call_transaction_fallback_CALL_PY_GENERAL:;")
        self.assertIn("call_bind_known_python(", success)
        self.assertIn("if (call_result.status > 0)", success)
        self.assertIn("call_transaction_run_vector(", success)
        self.assertIn("CALL_PREPARED", success)
        self.assertIn("call_transaction_run_vector(", fallback)
        self.assertNotIn("call_bind_known_python(", fallback)
        other = self.handler(output, "CALL_BOUND_METHOD_GENERAL")
        self.assertIn("call_transaction_run_vector(", other)
        self.assertNotIn("call_bind_known_python(", other)

        changes = (
            ("_CHECK_PEP_523 +\n            _CHECK_FUNCTION_VERSION +\n"
             "            _CHECK_RECURSION_REMAINING +\n"
             "            _PY_FRAME_GENERAL_GUARDED +",
             "_CHECK_FUNCTION_VERSION +\n            _CHECK_RECURSION_REMAINING +\n"
             "            _PY_FRAME_GENERAL_GUARDED +"),
            ("_CHECK_FUNCTION_VERSION +\n            _CHECK_RECURSION_REMAINING +\n"
             "            _PY_FRAME_GENERAL_GUARDED +",
             "_CHECK_RECURSION_REMAINING +\n            _PY_FRAME_GENERAL_GUARDED +"),
            ("DEOPT_IF(IS_PEP523_HOOKED(tstate));",
             "spam_call(); DEOPT_IF(IS_PEP523_HOOKED(tstate));"),
            ("IR_INVOKE_GUARDED_PYTHON_CALL();",
             "IR_INVOKE_GUARDED_PYTHON_CALL(); IR_INVOKE_GUARDED_PYTHON_CALL();"),
            ("_CHECK_METHOD_VERSION +\n            _EXPAND_METHOD +",
             "_CHECK_METHOD_VERSION +\n            _PY_FRAME_GENERAL_GUARDED +\n"
             "            _EXPAND_METHOD +"),
            ("_MONITOR_CALL + _DO_CALL + _CHECK_PERIODIC_AT_END;",
             "_MONITOR_CALL + _PY_FRAME_GENERAL_GUARDED + _CHECK_PERIODIC_AT_END;"),
        )
        for old, new in changes:
            with self.subTest(new=new):
                self.assertIn(old, source)
                with self.assertRaises(SyntaxError):
                    generate(source.replace(old, new, 1))

    def test_guarded_keyword_call_contract(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()

        def generate(text):
            Path(self.temp_input_filename).write_text(text)
            with handle_stderr():
                analysis = analyzer.analyze_files([self.temp_input_filename])
            out = io.StringIO()
            ir_generator.generate_ir([self.temp_input_filename], analysis, out, False)
            return out.getvalue()

        output = generate(source)
        body = self.handler(output, "CALL_KW_PY")
        success, fallback = body.split("call_transaction_fallback_CALL_KW_PY:;")
        self.assertIn("call_bind_known_python(", success)
        self.assertIn("CALL_TRANSACTION_KEYWORDS, fb)", success)
        self.assertIn("if (call_result.status > 0)", success)
        self.assertIn("call_transaction_run_keywords(", success)
        self.assertNotIn("call_transaction_run_vector(", body)
        self.assertIn("CALL_PREPARED", success)
        self.assertIn("call_transaction_run_keywords(", fallback)
        self.assertNotIn("call_bind_known_python(", fallback)
        for name in ("CALL_KW_BOUND_METHOD", "INSTRUMENTED_CALL_KW"):
            with self.subTest(name=name):
                self.assertNotIn("call_bind_known_python(", self.handler(output, name))

        changes = (
            ("_CHECK_PEP_523 +\n            _CHECK_FUNCTION_VERSION_KW +",
             "_CHECK_FUNCTION_VERSION_KW +"),
            ("_CHECK_FUNCTION_VERSION_KW +\n            _CHECK_RECURSION_REMAINING +",
             "_CHECK_RECURSION_REMAINING +"),
            ("_CHECK_RECURSION_REMAINING +\n            _PY_FRAME_KW_GUARDED +",
             "_PY_FRAME_KW_GUARDED +"),
            ("op(_PY_FRAME_KW_GUARDED, (callable, self_or_null, args[oparg], kwnames -- new_frame))",
             "op(_PY_FRAME_KW_GUARDED, (callable, self_or_null, args[oparg] -- new_frame))"),
            ("_CHECK_METHOD_VERSION_KW +\n            _EXPAND_METHOD_KW +",
             "_CHECK_METHOD_VERSION_KW +\n            _PY_FRAME_KW_GUARDED +"),
            ("_MONITOR_CALL_KW +\n            _DO_CALL_KW;",
             "_MONITOR_CALL_KW +\n            _PY_FRAME_KW_GUARDED;"),
        )
        for old, new in changes:
            with self.subTest(new=new):
                self.assertIn(old, source)
                with self.assertRaises(SyntaxError):
                    generate(source.replace(old, new, 1))

    def test_terminal_observer_ownership_and_composition(self):
        source = """
        inst(OP, (value -- res)) {
            PyValue held = TAKE(value);
            res = held;
            IR_OBSERVE_COMMITTED_BRANCH(this_instr, JUMP_TARGET(oparg),
                                        PY_MONITORING_EVENT_BRANCH_RIGHT, _PyIR_EDGE_TAKEN);
        }
        """
        output = self.ir_output(source)
        self.assertNotIn("value_boxes[", output)
        self.assertNotIn("value_owners", output)
        self.assertFalse(ir_generator.boxes_into_record(
            "_PyValueTemps_Close(value_temps);"))
        self.assertTrue(ir_generator.boxes_into_record(
            "unknown_helper(value_temps);"))
        changes = (
            ("res = held;", "PyValue_CLOSE(held);", "not defined"),
            ("PyValue held = TAKE(value);\n            res = held;",
             "res = PyValue_None;", "still live"),
            ("res = held;", "PyValue extra = PyValue_DUP(held); res = held;",
             "untransferred C owners"),
            ("res = held;", "PyValue alias = held; PyValue_CLOSE(held); res = alias;",
             "used after transfer"),
            ("res = held;", "if (flag) { PyValue_CLOSE(held); } res = held;",
             "ambiguous terminal C ownership"),
            ("res = held;", "steal(held); res = held;",
             "unproved terminal C owner transport"),
            ("res = held;", "int ir_observer_next = 0; res = held;",
             "reserved local"),
            ("PY_MONITORING_EVENT_BRANCH_RIGHT, _PyIR_EDGE_TAKEN);",
             "PY_MONITORING_EVENT_BRANCH_RIGHT, _PyIR_EDGE_TAKEN); PyValue_CLOSE(res);",
             "follows terminal observer"),
            ("PY_MONITORING_EVENT_BRANCH_RIGHT, _PyIR_EDGE_TAKEN);",
             "PY_MONITORING_EVENT_BRANCH_RIGHT, _PyIR_EDGE_TAKEN); "
             "IR_OBSERVE_COMMITTED_BRANCH(this_instr, JUMP_TARGET(oparg), "
             "PY_MONITORING_EVENT_BRANCH_RIGHT, _PyIR_EDGE_TAKEN);", "follows terminal observer"),
        )
        for old, new, message in changes:
            with self.subTest(message=message):
                self.assertIn(old, source)
                with self.assertRaisesRegex(SyntaxError, message):
                    self.ir_output(source.replace(old, new, 1))
        with self.assertRaisesRegex(SyntaxError, "finish the composed handler"):
            self.ir_output(source.replace("inst(OP,", "op(_OP,") + """
            op(_AFTER, (--)) { do_work(); }
            macro(OP) = _OP + _AFTER;
        """)

    def test_terminal_observer_defined_null_edge(self):
        source = """
        inst(OP, (value -- res)) {
            PyValue held = TAKE(value);
            if (flag) {
                res = held;
                IR_OBSERVE_COMMITTED_BRANCH(this_instr, JUMP_TARGET(oparg),
                                            PY_MONITORING_EVENT_BRANCH_RIGHT, _PyIR_EDGE_TAKEN);
            }
            else {
                IR_DEFER_CLOSE(held);
                res = PyValue_NULL;
                IR_OBSERVE_COMMITTED_BRANCH(this_instr, next_instr,
                                            PY_MONITORING_EVENT_BRANCH_LEFT, _PyIR_EDGE_FALLTHROUGH);
            }
        }
        """
        Path(self.temp_input_filename).write_text(parser.BEGIN_MARKER + source + parser.END_MARKER)
        analysis = analyzer.analyze_files([self.temp_input_filename], SequentialNumbers())
        formats = ir_generator.compute_formats(analysis)
        # Terminal C emission still has the instruction's normal CFG and
        # logical result. It is not a return/frame-exit analysis marker.
        self.assertFalse(analysis.uops['OP'].properties.always_exits)
        self.assertTrue(analysis.uops['OP'].properties.sync_sp)
        self.assertEqual(len(ir_generator.external_io(analysis.instructions['OP'])[1]), 1)
        self.assertEqual(ir_generator.observer_outcomes(
            analysis.instructions['OP'], formats['OP']),
            ('_PyIR_OBSERVER_FORMAT_NULL_FIRST', '_PyIR_OBSERVER_FORMAT_VALUE'))
        self.ir_output(source)
        with self.assertRaisesRegex(SyntaxError, "after transfer|not held|duplicate deferred"):
            self.ir_output(source.replace("IR_DEFER_CLOSE(held);",
                "IR_DEFER_CLOSE(held); IR_DEFER_CLOSE(held);"))
        with self.assertRaisesRegex(SyntaxError, "not held"):
            self.ir_output(source.replace("IR_DEFER_CLOSE(held);",
                                          "IR_DEFER_CLOSE(unknown);"))
        for suffix in ('held = PyValue_None;', 'PyValue_CLOSE(held);', 'read(held);'):
            with self.subTest(suffix=suffix):
                with self.assertRaisesRegex(SyntaxError, "after transfer"):
                    self.ir_output(source.replace('IR_DEFER_CLOSE(held);',
                        'IR_DEFER_CLOSE(held); ' + suffix))
        with self.assertRaisesRegex(SyntaxError, 'deferred owner exits'):
            self.ir_output(source.replace('IR_DEFER_CLOSE(held);',
                                          'IR_DEFER_CLOSE(held); ERROR_NO_POP();'))

    def test_terminal_observer_explicit_owner_edge(self):
        jump = """
        inst(OP, (--)) {
            IR_OBSERVE_COMMITTED_BRANCH(this_instr, JUMP_TARGET(oparg),
                PY_MONITORING_EVENT_JUMP, _PyIR_EDGE_TAKEN);
        }
        """
        self.ir_output(jump)
        for old, new, message in (
            (", _PyIR_EDGE_TAKEN", "", "takes source, destination, event, owner edge"),
            ("_PyIR_EDGE_TAKEN", "edge", "literal owner edge"),
            ("_PyIR_EDGE_TAKEN", "_PyIR_EDGE_FALLTHROUGH", "disagrees with destination"),
            ("JUMP_TARGET(oparg)", "next_instr", "disagrees with destination"),
            ("PY_MONITORING_EVENT_JUMP", "event", "literal event"),
        ):
            with self.subTest(message=message, new=new):
                with self.assertRaisesRegex(SyntaxError, message):
                    self.ir_output(jump.replace(old, new))
        iterator = """
        inst(OP, (iter, index_or_null --)) {
            PyValue held = TAKE(iter);
            (void)index_or_null;
            DEAD(index_or_null);
            IR_DEFER_CLOSE(held);
            IR_OBSERVE_COMMITTED_BRANCH(prev_instr, next_instr,
                PY_MONITORING_EVENT_BRANCH_RIGHT, _PyIR_EDGE_FALLTHROUGH);
        }
        """
        output = self.ir_output(iterator)
        callback = output.index("_PyIR_ObserveCommittedBranch(")
        self.assertIn("this_instr, prev_instr, ir_observer_next, "
                      "PY_MONITORING_EVENT_BRANCH_RIGHT, 0", output)
        self.assertLess(output.index("frame->return_pc ="), callback)
        self.assertLess(callback, output.index("PyValue_XCLOSE(ir_observer_input_0)"))
        analysis = self.analyze(iterator)
        formats = ir_generator.compute_formats(analysis)
        self.assertEqual(ir_generator.observer_outcomes(
            analysis.instructions['OP'], formats['OP']),
            ('_PyIR_OBSERVER_FORMAT_VALUE', '_PyIR_OBSERVER_NONE'))

    def test_terminal_observer_dotted_owner_mutations(self):
        source = (Path(__file__).resolve().parents[2] / 'Python' / 'bytecodes.c').read_text()
        start = source.index('inst(INSTRUMENTED_FOR_ITER,')
        for argument in ('step.value', '&step.value'):
            with self.subTest(argument=argument):
                changed = source[:start] + source[start:].replace(
                    'PyValue item = step.value;',
                    f'opaque({argument}); PyValue item = step.value;', 1)
                Path(self.temp_input_filename).write_text(changed)
                analysis = analyzer.analyze_files([self.temp_input_filename])
                formats = ir_generator.compute_formats(analysis)
                with self.assertRaisesRegex(SyntaxError, 'unproved terminal C owner transport'):
                    ir_generator.observer_terminal_paths(
                        analysis.instructions['INSTRUMENTED_FOR_ITER'],
                        formats['INSTRUMENTED_FOR_ITER'])

    def test_canonical_numeric_constructor_proof(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()
        Path(self.temp_input_filename).write_text(source)
        with handle_stderr():
            analysis = analyzer.analyze_files([self.temp_input_filename])
        formats = ir_generator.compute_formats(analysis)
        names = ('BINARY_OP_MULTIPLY_FLOAT', 'BINARY_OP_ADD_FLOAT',
                 'BINARY_OP_SUBTRACT_FLOAT', 'BINARY_OP_TRUEDIV_FLOAT',
                 'BINARY_OP_ADD_INT', 'BINARY_OP_SUBTRACT_INT')
        for name in names:
            contract = ir_generator.canonical_result_source(
                analysis.instructions[name], formats[name])
            self.assertIsNotNone(contract, name)
            self.assertEqual(contract.result, 'res')
        output = io.StringIO()
        ir_generator.generate_ir([self.temp_input_filename], analysis, output, False)
        for name in names:
            handler = self.handler(output.getvalue(), name)
            self.assertIn('IR_STORE_CANONICAL_DST(IR_A(ir_word), res);', handler)
            self.assertNotIn('IR_STORE_DST(IR_A(ir_word), res);', handler)
        for name in ('BINARY_OP_MULTIPLY_INT',):
            self.assertIsNone(ir_generator.canonical_result_source(
                analysis.instructions[name], formats[name]))
            self.assertIn('IR_STORE_DST(IR_A(ir_word), res);',
                          self.handler(output.getvalue(), name))

        # Eligibility follows the source, not the opcode/producer names.
        renamed = source.replace('BINARY_OP_MULTIPLY_FLOAT', 'RENAMED_PRODUCT')
        Path(self.temp_input_filename).write_text(renamed)
        with handle_stderr():
            other = analyzer.analyze_files(
                [self.temp_input_filename], SequentialNumbers())
        other_formats = ir_generator.compute_formats(other)
        self.assertIsNotNone(ir_generator.canonical_result_source(
            other.instructions['RENAMED_PRODUCT'], other_formats['RENAMED_PRODUCT']))

    def test_canonical_numeric_constructor_mutations_fail_closed(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()
        changes = (
            ('_BINARY_OP_MULTIPLY_FLOAT, (left, right -- res, l, r)',
             '_BINARY_OP_MULTIPLY_FLOAT, (PyValue_FromDouble/1, left, right -- res, l, r)'),
            ('_BINARY_OP_MULTIPLY_FLOAT, (left, right -- res, l, r)',
             '_BINARY_OP_MULTIPLY_FLOAT, (PyValue_FloatOperandValues/1, left, right -- res, l, r)'),
            ('_BINARY_OP_MULTIPLY_FLOAT + _POP_TOP_NUMBER + _POP_TOP_NUMBER;',
             '_BINARY_OP_MULTIPLY_FLOAT + flush + _POP_TOP_NUMBER + _POP_TOP_NUMBER;'),
            ('res = PyValue_FromDouble(a * b);',
             'res = unknown_result(a * b);'),
            ('res = PyValue_FromDouble(a * b);',
             'if (a) { res = PyValue_FromDouble(a * b); } else { res = unknown_result(); }'),
            ('res = PyValue_FromDouble(a * b);',
             'res = PyValue_FromDouble(a * b); res = unknown_result();'),
            ('res = PyValue_FromDouble(a * b);',
             'res = PyValue_FromDouble(a * b); mutate(&res);'),
            ('res = PyValue_FromDouble(a * b);',
             'res = PyValue_FromDouble(unknown_result());'),
            ('res = PyValue_FromDouble(a * b);',
             'PyValue (*PyValue_FromDouble)(double) = replacement; res = PyValue_FromDouble(a * b);'),
            ('res = PyValue_FromDouble(a * b);',
             '#define PyValue_FromDouble replacement\nres = PyValue_FromDouble(a * b);\n#undef PyValue_FromDouble'),
            ('res = PyValue_FromDouble(a * b);',
             'res = PyValue_FromDouble(a * b);\n#if 1\nspam();\n#endif'),
            ('res = PyValue_FromDouble(a * b);\n            if (PyValue_IsNull(res)) {\n                ERROR_NO_POP();\n            }',
             'res = PyValue_FromDouble(a * b);'),
            ('res = PyValue_FromDouble(a * b);',
             'res = PyValue_FromDouble(a * b); EXIT_IF(a == 0.0);'),
            ('res = PyValue_FromDouble(a * b);',
             'res = PyValue_FromDouble(a * b); PyValue_CLOSE(res);'),
            ('assert(PyValue_IsInlineFloat(value) || PyValue_IsInlineInt(value));',
             'assert(PyValue_IsInlineFloat(value) || PyValue_IsInlineInt(value)); spam();'),
            ('assert(PyValue_IsInlineFloat(value) || PyValue_IsInlineInt(value));',
             'assert(PyValue_FloatCheck(value));'),
            ('_BINARY_OP_MULTIPLY_FLOAT, (left, right -- res, l, r)',
             '_BINARY_OP_MULTIPLY_FLOAT, (left, right -- left, l, r)'),
            ('_BINARY_OP_MULTIPLY_FLOAT + _POP_TOP_NUMBER + _POP_TOP_NUMBER;',
             '_BINARY_OP_MULTIPLY_FLOAT + _POP_TOP_NUMBER + _POP_TOP_NUMBER + _REPLACE_RESULT;\n'
             '        op(_REPLACE_RESULT, (res -- res)) { mutate(&res); }'),
        )
        for old, new in changes:
            with self.subTest(new=new):
                self.assertIn(old, source)
                Path(self.temp_input_filename).write_text(source.replace(old, new, 1))
                with handle_stderr():
                    analysis = analyzer.analyze_files([self.temp_input_filename])
                formats = ir_generator.compute_formats(analysis)
                self.assertIsNone(ir_generator.canonical_result_source(
                    analysis.instructions['BINARY_OP_MULTIPLY_FLOAT'],
                    formats['BINARY_OP_MULTIPLY_FLOAT']))

    def test_canonical_inline_int_mutations_fail_closed(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()
        start = source.index('inline_ok pure op(_BINARY_OP_ADD_INT,')
        end = source.index('inline_ok pure op(_BINARY_OP_SUBTRACT_INT,', start)
        producer = source[start:end]
        box = ('res = _PyValue_BoxInt64(PyValue_InlineInt64(left) + '
               'PyValue_InlineInt64(right));')
        changes = (
            ('__builtin_add_overflow(a, b, &sum)',
             '__builtin_sub_overflow(a, b, &sum)'),
            ('__builtin_add_overflow(a, b, &sum)',
             '!__builtin_add_overflow(a, b, &sum)'),
            (box, box.replace(' + ', ' - ')),
            (box, box.replace('PyValue_InlineInt64(right)', '1')),
            ('_PyValue_FromIntScaled(sum)', '_PyValue_FromIntScaled(a)'),
            ('if (PyValue_IsNull(res)) {\n                ERROR_NO_POP();\n            }', ''),
            (box, box + ' res = PyValue_None;'),
            (box, box + ' opaque(&res);'),
            (box, 'callback(); ' + box),
            (box, 'PyValue (*_PyValue_BoxInt64)(int64_t) = replacement; ' + box),
            (box, '#define _PyValue_BoxInt64 replacement\n' + box +
             '\n#undef _PyValue_BoxInt64'),
            ('l = left;', 'l = res;'),
            ('(left, right -- res, l, r)',
             '(_PyValue_BoxInt64/1, left, right -- res, l, r)'),
        )
        variants = [source[:start] + producer.replace(old, new, 1) + source[end:]
                    for old, new in changes]
        for old, _ in changes:
            self.assertIn(old, producer)
        variants.extend(source.replace(old, new, 1) for old, new in (
            ('EXIT_IF(!PyValue_IsInlineInt(left));',
             'EXIT_IF(!PyValue_LongCheckExact(left));'),
            ('PyValue_CLOSE_SPECIALIZED(value, _PyLong_ExactDealloc);',
             'PyValue_CLOSE_SPECIALIZED(value, _PyLong_ExactDealloc); callback();'),
            ('_BINARY_OP_ADD_INT + _POP_TOP_INT + _POP_TOP_INT;',
             '_BINARY_OP_ADD_INT + flush + _POP_TOP_INT + _POP_TOP_INT;'),
        ))
        for index, changed in enumerate(variants):
            with self.subTest(variant=index):
                Path(self.temp_input_filename).write_text(changed)
                with handle_stderr():
                    analysis = analyzer.analyze_files([self.temp_input_filename])
                formats = ir_generator.compute_formats(analysis)
                self.assertIsNone(ir_generator.canonical_result_source(
                    analysis.instructions['BINARY_OP_ADD_INT'],
                    formats['BINARY_OP_ADD_INT']))

        Path(self.temp_input_filename).write_text(source)
        with handle_stderr():
            analysis = analyzer.analyze_files([self.temp_input_filename])
        formats = ir_generator.compute_formats(analysis)
        instruction = analysis.instructions['BINARY_OP_ADD_INT']
        body = analysis.uops['_BINARY_OP_ADD_INT'].body
        body.body.append(body.body[-1])
        self.assertIsNone(ir_generator.canonical_result_source(
            instruction, formats['BINARY_OP_ADD_INT']))

    def test_canonical_inline_int_renamed_source(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()
        source = source.replace('BINARY_OP_ADD_INT', 'RENAMED_INT_SUM')
        source = source.replace('BINARY_OP_SUBTRACT_INT', 'RENAMED_INT_DIFFERENCE')
        Path(self.temp_input_filename).write_text(source)
        with handle_stderr():
            analysis = analyzer.analyze_files(
                [self.temp_input_filename], SequentialNumbers())
        formats = ir_generator.compute_formats(analysis)
        for name in ('RENAMED_INT_SUM', 'RENAMED_INT_DIFFERENCE'):
            self.assertIsNotNone(ir_generator.canonical_result_source(
                analysis.instructions[name], formats[name]))

    def test_canonical_numeric_direct_int64_boundary(self):
        source = '''
        inst(DIRECT, (-- result)) {
            result = PyValue_FromInt64(42);
            if (PyValue_IsNull(result)) { ERROR_NO_POP(); }
        }
        '''
        for constructor, valid in (('PyValue_FromInt64', True),
                                   ('_PyValue_BoxInt64', False),
                                   ('_PyValue_FromIntScaled', False)):
            with self.subTest(constructor=constructor):
                with handle_stderr():
                    analysis = analyzer.analyze_forest(
                        parse_src(source.replace('PyValue_FromInt64', constructor)),
                        SequentialNumbers())
                formats = ir_generator.compute_formats(analysis)
                contract = ir_generator.canonical_result_source(
                    analysis.instructions['DIRECT'], formats['DIRECT'])
                self.assertEqual(contract is not None, valid)

    def test_canonical_local_duplicate_source(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()
        Path(self.temp_input_filename).write_text(source)
        with handle_stderr():
            analysis = analyzer.analyze_files([self.temp_input_filename])
        formats = ir_generator.compute_formats(analysis)
        out = io.StringIO()
        ir_generator.generate_ir([self.temp_input_filename], analysis, out, False)
        for name in ("LOAD_FAST", "LOAD_FAST_BORROW"):
            with self.subTest(name=name):
                contract = ir_generator.canonical_result_source(
                    analysis.instructions[name], formats[name])
                self.assertIsNotNone(contract)
                self.assertEqual(contract.result, "value")
                handler = self.handler(out.getvalue(), name)
                self.assertIn("IR_STORE_CANONICAL_DST(IR_A(ir_word), value);",
                              handler)

        template = """
        pure inst(RENAMED_LOCAL_DUP, (-- result)) {
            assert(!PyValue_IsNull(GETLOCAL(oparg)));
            result = PyValue_DUP(GETLOCAL(oparg));
        }
        """
        variants = (
            (template, True),
            (template.replace("GETLOCAL(oparg)", "GETLOCAL(oparg + 1)"), False),
            (template.replace("PyValue_DUP(GETLOCAL(oparg))",
                              "GETLOCAL(oparg)"),
             False),
            (template.replace("result = PyValue_DUP(GETLOCAL(oparg));",
                              "result = PyValue_DUP(GETLOCAL(oparg)); callback();"),
             False),
            (template.replace("result = PyValue_DUP(GETLOCAL(oparg));",
                              "result = PyValue_DUP(GETLOCAL(oparg)); result = PyValue_NULL;"),
             False),
            (template.replace("(-- result)", "(GETLOCAL -- result)"), False),
            (template.replace("pure inst(RENAMED_LOCAL_DUP, (-- result))",
                              "pure inst(RENAMED_LOCAL_DUP, (GETLOCAL/1 -- result))"),
             False),
            (template.replace("result = PyValue_DUP(GETLOCAL(oparg));",
                              "#define GETLOCAL(x) replacement(x)\n"
                              "result = PyValue_DUP(GETLOCAL(oparg));\n"
                              "#undef GETLOCAL"),
             False),
        )
        for text, valid in variants:
            with self.subTest(source=text), mock.patch.dict(
                ir_generator.FEEDBACK_LAYOUTS,
                {"RENAMED_LOCAL_DUP": ir_generator.FB_HEADER}
            ):
                with handle_stderr():
                    variant = analyzer.analyze_forest(
                        parse_src(text), SequentialNumbers())
                variant_formats = ir_generator.compute_formats(variant)
                contract = ir_generator.canonical_result_source(
                    variant.instructions["RENAMED_LOCAL_DUP"],
                    variant_formats["RENAMED_LOCAL_DUP"])
                self.assertEqual(contract is not None, valid)

    def test_canonical_frame_transfer_provenance_fails_closed(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()

        def analyze(text):
            Path(self.temp_input_filename).write_text(text)
            with handle_stderr():
                return analyzer.analyze_files([self.temp_input_filename])

        original = analyze(source)
        for name in ("RETURN_VALUE", "INSTRUMENTED_RETURN_VALUE"):
            self.assertTrue(ir_generator.canonical_frame_transfer_input(
                original.instructions[name]))

        changes = (
            ("PyValue temp = retval;",
             "retval = PyValue_DUP(retval); PyValue temp = retval;",
             ("RETURN_VALUE", "INSTRUMENTED_RETURN_VALUE")),
            ("IR_RETURN_TO_CALLER(temp, generator_return);",
             "temp = PyValue_DUP(temp); IR_RETURN_TO_CALLER(temp, generator_return);",
             ("RETURN_VALUE", "INSTRUMENTED_RETURN_VALUE")),
            ("PyValue held = TAKE(val);",
             "val = PyValue_None; PyValue held = TAKE(val);",
             ("INSTRUMENTED_RETURN_VALUE",)),
            ("taken = held;", "taken = PyValue_DUP(held);",
             ("INSTRUMENTED_RETURN_VALUE",)),
            ("taken = held;", "transform(&held); taken = held;",
             ("INSTRUMENTED_RETURN_VALUE",)),
        )
        for old, new, affected in changes:
            with self.subTest(new=new):
                self.assertIn(old, source)
                analysis = analyze(source.replace(old, new, 1))
                for name in affected:
                    self.assertFalse(ir_generator.canonical_frame_transfer_input(
                        analysis.instructions[name]))
                # An unsupported mutation may also be rejected by ownership
                # validation; it must never silently regain canonical stores.
                output = io.StringIO()
                try:
                    ir_generator.generate_ir(
                        [self.temp_input_filename], analysis, output, False)
                except SyntaxError:
                    continue
                for name in affected:
                    handler = self.handler(output.getvalue(), name)
                    self.assertNotIn("assert(_PyValue_IsHome(temp));", handler)
                    self.assertIn("temp = _PyValue_Publish(&temp);", handler)

    def test_canonical_list_child_and_yield_sources(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()

        def analyze(text, numbers=None):
            Path(self.temp_input_filename).write_text(text)
            with handle_stderr():
                return analyzer.analyze_files([self.temp_input_filename], numbers)

        analysis = analyze(source)
        formats = ir_generator.compute_formats(analysis)
        contract = ir_generator.canonical_result_source(
            analysis.instructions['BINARY_OP_SUBSCR_LIST_INT'],
            formats['BINARY_OP_SUBSCR_LIST_INT'])
        self.assertIsNotNone(contract)
        self.assertEqual(contract.result, 'res')
        self.assertTrue(ir_generator.canonical_frame_transfer_input(
            analysis.instructions['YIELD_VALUE']))
        self.assertFalse(ir_generator.canonical_frame_transfer_input(
            analysis.instructions['INSTRUMENTED_YIELD_VALUE']))
        output = io.StringIO()
        ir_generator.generate_ir([self.temp_input_filename], analysis, output, False)
        handler = self.handler(output.getvalue(), 'BINARY_OP_SUBSCR_LIST_INT')
        early = 'IR_STORE_TEMP_DST(IR_A(ir_word), res);'
        close = '_PyValue_XCLOSE_POINTER(ir_own_B);'
        final = 'IR_STORE_CANONICAL_DST(IR_A(ir_word), res);'
        self.assertLess(handler.index(early), handler.index(close))
        self.assertLess(handler.index(close), handler.index(final))
        self.assertNotIn('IR_STORE_CANONICAL_TEMP_DST', handler)
        self.assertIn('IR_STORE_CANONICAL_RETURN(value);',
                      self.handler(output.getvalue(), 'YIELD_VALUE'))
        self.assertIn('IR_STORE_RETURN(value);',
                      self.handler(output.getvalue(), 'INSTRUMENTED_YIELD_VALUE'))

        # Neither proof depends on the instruction or uop names.
        renamed = source.replace('BINARY_OP_SUBSCR_LIST_INT', 'RENAMED_LIST_CHILD')
        renamed = renamed.replace('YIELD_VALUE', 'RENAMED_YIELD')
        other = analyze(renamed, SequentialNumbers())
        other_formats = ir_generator.compute_formats(other)
        self.assertIsNotNone(ir_generator.canonical_result_source(
            other.instructions['RENAMED_LIST_CHILD'],
            other_formats['RENAMED_LIST_CHILD']))
        self.assertTrue(ir_generator.canonical_frame_transfer_input(
            other.instructions['RENAMED_YIELD']))

    def test_canonical_list_child_mutations_fail_closed(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()
        duplicate = 'res = PyValue_XDUP(PyList_GET_VALUE(list, index));'
        changes = (
            (duplicate, 'res = PyList_GET_VALUE(list, index);'),
            (duplicate, duplicate + ' res = PyValue_None;'),
            (duplicate, duplicate + ' opaque(&res);'),
            (duplicate, 'PyValue_CLOSE(list_st); ' + duplicate),
            (duplicate, 'PyValue (*PyList_GET_VALUE)(void) = replacement; ' + duplicate),
            (duplicate, '#define PyList_GET_VALUE replacement\n' + duplicate +
             '\n#undef PyList_GET_VALUE'),
            ('_BINARY_OP_SUBSCR_LIST_INT, (list_st, sub_st -- res, ls, ss)',
             '_BINARY_OP_SUBSCR_LIST_INT, (PyList_GET_VALUE/1, list_st, sub_st -- res, ls, ss)'),
            ('EXIT_IF(index < 0 || index >= PyList_GET_SIZE(list));',
             'EXIT_IF(index < 0 || index > PyList_GET_SIZE(list));'),
            ('index += PyList_GET_SIZE(list);', 'index = 0;'),
            ('ls = list_st;', 'ls = res;'),
            ('PyValue_XCLOSE(value);', 'PyValue_XCLOSE(value); opaque();'),
            ('_BINARY_OP_SUBSCR_LIST_INT + _POP_TOP_INT + POP_TOP;',
             '_BINARY_OP_SUBSCR_LIST_INT + flush + _POP_TOP_INT + POP_TOP;'),
        )
        for old, new in changes:
            with self.subTest(new=new):
                self.assertIn(old, source)
                Path(self.temp_input_filename).write_text(source.replace(old, new, 1))
                with handle_stderr():
                    analysis = analyzer.analyze_files([self.temp_input_filename])
                formats = ir_generator.compute_formats(analysis)
                self.assertIsNone(ir_generator.canonical_result_source(
                    analysis.instructions['BINARY_OP_SUBSCR_LIST_INT'],
                    formats['BINARY_OP_SUBSCR_LIST_INT']))

    def test_canonical_yield_mutations_fail_closed(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()
        start = source.index('window op(_YIELD_VALUE,')
        changes = (
            ('PyValue temp = retval;', 'PyValue temp = PyValue_DUP(retval);'),
            ('PyValue temp = retval;', 'opaque(&retval); PyValue temp = retval;'),
            ('value = temp;', 'value = PyValue_DUP(temp);'),
            ('value = temp;', 'value = temp; opaque(&value);'),
            ('value = temp;', 'PyValue temp2 = PyValue_None; value = temp2;'),
            ('value = temp;', '#define temp replacement\nvalue = temp;\n#undef temp'),
            ('#endif\n            RELOAD_STACK();',
             '#else\n            value = PyValue_None;\n#endif\n            RELOAD_STACK();'),
            ('window op(_YIELD_VALUE, (retval -- value))',
             'window op(_YIELD_VALUE, (TAKE/1, retval -- value))'),
        )
        for old, new in changes:
            with self.subTest(new=new):
                self.assertIn(old, source[start:])
                changed = source[:start] + source[start:].replace(old, new, 1)
                Path(self.temp_input_filename).write_text(changed)
                with handle_stderr():
                    analysis = analyzer.analyze_files([self.temp_input_filename])
                self.assertFalse(ir_generator.canonical_frame_transfer_input(
                    analysis.instructions['YIELD_VALUE']))

        # An AST rewrite must not retain a proof from its stale source span.
        for move_debug_arm in (False, True):
            with self.subTest(ast_debug_arm=move_debug_arm):
                Path(self.temp_input_filename).write_text(source)
                with handle_stderr():
                    analysis = analyzer.analyze_files([self.temp_input_filename])
                instruction = analysis.instructions['YIELD_VALUE']
                body = instruction.parts[-1].body
                if move_debug_arm:
                    debug = next(stmt for stmt in body.body
                                 if isinstance(stmt, ir_generator.MacroIfStmt))
                    debug.else_body, debug.body = debug.body, []
                    debug.else_ = next(ir_generator.tokenize('#else\n'))
                else:
                    body.body.append(body.body[-1])
                self.assertFalse(ir_generator.canonical_frame_transfer_input(
                    instruction))

    def test_completion_result_local_does_not_capture_held_value(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()
        self.assertIn("PyValue completed =", source)
        Path(self.temp_input_filename).write_text(source)
        analysis = analyzer.analyze_files([self.temp_input_filename])
        output = io.StringIO()
        ir_generator.generate_ir(
            [self.temp_input_filename], analysis, output, False)
        for name in ("FOR_ITER", "FOR_ITER_LIST", "SEND", "CLEANUP_THROW"):
            with self.subTest(name=name):
                handler = self.handler(output.getvalue(), name)
                self.assertIn("PyValue completed =", handler)
                self.assertIn("_PyIRStepResult ir_step_result =", handler)
                self.assertNotIn("&completed", handler)
                self.assertIn(", completed, _PY_IR_", handler)
                self.assertNotIn("_PyIRStepResult completed =", handler)
                if name == "CLEANUP_THROW":
                    self.assertNotIn("assert(!IR_IS_LOCAL(dst)", handler)
        for name in ("FOR_ITER_GEN", "SEND_GEN"):
            self.assertNotIn("PyValue new_frame;",
                             self.handler(output.getvalue(), name))

    def test_suspension_effect_requires_a_state_write(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()
        Path(self.temp_input_filename).write_text(source)
        analysis = analyzer.analyze_files([self.temp_input_filename])
        for name in ("SEND", "SEND_GEN"):
            self.assertIn("FRAME_CREATED", ir_generator.uop_tokens(
                analysis.instructions[name]))
            self.assertFalse(ir_generator.writes_suspended_frame_state(
                analysis.instructions[name]))
        for name in ("YIELD_VALUE", "INSTRUMENTED_YIELD_VALUE", "RETURN_GENERATOR"):
            self.assertTrue(ir_generator.writes_suspended_frame_state(
                analysis.instructions[name]))
        source = source.replace(
            "FT_ATOMIC_STORE_INT8_RELEASE(gen->gi_frame_state, FRAME_SUSPENDED + oparg);",
            "assert(gen->gi_frame_state == FRAME_SUSPENDED + oparg);")
        source = source.replace("gen->gi_frame_state = FRAME_CREATED;",
                                "assert(gen->gi_frame_state == FRAME_CREATED);")
        Path(self.temp_input_filename).write_text(source)
        mutated = analyzer.analyze_files([self.temp_input_filename])
        for name in ("YIELD_VALUE", "INSTRUMENTED_YIELD_VALUE", "RETURN_GENERATOR"):
            self.assertFalse(ir_generator.writes_suspended_frame_state(
                mutated.instructions[name]))

    def test_physical_output_mapping_matches_window_finalization(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()
        Path(self.temp_input_filename).write_text(source)
        analysis = analyzer.analyze_files([self.temp_input_filename])
        formats = ir_generator.compute_formats(analysis)
        contracts = ir_generator.handler_contracts(analysis, formats)
        output = io.StringIO()
        ir_generator.generate_ir(
            [self.temp_input_filename], analysis, output, False)
        for name in ("PUSH_EXC_INFO", "LOAD_SUPER_ATTR_METHOD"):
            with self.subTest(name=name):
                self.assertEqual(contracts[name].output_kind, "_PyIR_OUTPUT_WINDOW")
                handler = self.handler(output.getvalue(), name)
                self.assertIn("IR_PUBLISH_TEMP(window[", handler)
                self.assertNotIn("IR_MOVE_TO_DST", handler)
        for name in ("LOAD_SUPER_ATTR", "INSTRUMENTED_LOAD_SUPER_ATTR"):
            with self.subTest(name=name):
                self.assertEqual(contracts[name].output_kind,
                                 "_PyIR_OUTPUT_WINDOW_DST_FIRST")
                handler = self.handler(output.getvalue(), name)
                self.assertIn("IR_PUBLISH_TEMP(window[1]", handler)
                self.assertIn("IR_MOVE_TO_DST_OR_DISCARD", handler)
                self.assertEqual(formats[name].dst_offset, 0)
        self.assertEqual(contracts["CALL"].output_kind,
                         "_PyIR_OUTPUT_WINDOW_DST_FIRST")
        self.assertIn("call_destination = IR_A(ir_word)",
                      self.handler(output.getvalue(), "CALL"))
        self.assertEqual(contracts["FOR_ITER_GEN"].output_kind,
                         "_PyIR_OUTPUT_OUTCOME")
        with mock.patch.object(ir_generator, "store_mode",
                               return_value=ir_generator.STORE_LEAVE):
            with self.assertRaisesRegex(SyntaxError, "unclassified window output"):
                ir_generator.handler_output_kind(
                    analysis.instructions["PUSH_EXC_INFO"], formats["PUSH_EXC_INFO"])

    def test_generator_resume_commits_before_input_disposal(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()
        Path(self.temp_input_filename).write_text(source)
        analysis = analyzer.analyze_files([self.temp_input_filename])
        for name in ("FOR_ITER", "SEND"):
            self.assertTrue(analysis.instructions[name].properties.jumps)
            self.assertTrue(analysis.instructions[name].properties.error_without_pop)
        self.assertTrue(analysis.instructions["CLEANUP_THROW"].properties.jumps)
        throw_format = ir_generator.compute_formats(analysis)["CLEANUP_THROW"]
        self.assertTrue(throw_format.jump_always)
        self.assertFalse(throw_format.names_target)
        self.assertEqual(throw_format.imm, ir_generator.LOC_CD)
        self.assertTrue(analysis.instructions["RETURN_VALUE"].properties.always_exits)
        output = io.StringIO()
        ir_generator.generate_ir(
            [self.temp_input_filename], analysis, output, False)
        for name in ("SEND", "SEND_GEN", "FOR_ITER_GEN"):
            with self.subTest(name=name):
                handler = self.handler(output.getvalue(), name)
                self.assertEqual(handler.count("_Py_EnterRecursivePy(tstate)"), 1)
                self.assertNotIn("py_recursion_remaining--", handler)
                entered = handler.index("frame = tstate->current_frame = ir_resumed_frame;")
                consumed = handler.index("_PyFrame_SetReturnValue(frame, ir_resume_input);")
                self.assertLess(entered, consumed)
                if name != "FOR_ITER_GEN":
                    detached = handler.index("IR_SLOT(IR_B(ir_word) + 2) = PyValue_NULL;")
                    self.assertLess(detached, entered)
        for name in ("RETURN_VALUE", "INSTRUMENTED_RETURN_VALUE",
                     "RETURN_VALUE_EMPTY", "INSTRUMENTED_RETURN_VALUE_EMPTY"):
            with self.subTest(name=name):
                handler = self.handler(output.getvalue(), name)
                helper = ("_PyEval_FrameClearAndPopWithResult" if name.endswith("_EMPTY")
                          else "_PyEval_FrameClearAndPopGeneralWithResult")
                retired = handler.index(f"temp = {helper}(tstate, dying, temp);")
                completed = handler.index("_PyEval_CompleteGeneratorReturn(")
                published = handler.index("_PyFrame_SetReturnValue(frame, temp)")
                self.assertLess(retired, completed)
                self.assertLess(retired, published)
                self.assertNotIn("IR_STORE_RETURN", handler)
        handler = self.handler(output.getvalue(), "CLEANUP_THROW")
        self.assertIn("_PyFrame_InstrAt(frame, oparg)", handler)
        self.assertNotIn("next_instr + oparg", handler)

    def run_ir_test(self, input: str, expected: str):
        self.assertEqual(self.ir_output(input).strip(), expected.strip())

    def test_retirement_flush_reuses_published_temporary(self):
        for name in ("LOAD_ATTR_SLOT", "LOAD_ATTR_INSTANCE_VALUE", "LOAD_ATTR_MODULE",
                     "LOAD_ATTR_WITH_HINT", "LOAD_ATTR_SHAPE"):
            with self.subTest(name=name):
                input = f"""
                op(_ATTR, (owner -- attr)) {{
                    attr = PyValue_DUP(owner);
                    PyValue receiver = OWNER(owner);
                    IR_RETIRE_PUBLISHED_RESULT(attr, receiver);
                }}
                op(_PUSH_NULL_CONDITIONAL, ( -- null[oparg & 1])) {{
                    if (oparg & 1) {{
                        null[0] = PyValue_NULL;
                    }}
                }}
                macro({name}) = _ATTR + _PUSH_NULL_CONDITIONAL;
                """
                analysis = self.analyze(input)
                self.assertTrue(analysis.instructions[name].properties.sync_sp)
                formats = ir_generator.compute_formats(analysis)
                contract = ir_generator.handler_contracts(analysis, formats)[name]
                self.assertEqual(
                    contract.result_phase,
                    ir_generator.RESULT_PHASE_DST_TEMP_CANONICAL_BEFORE_RETIRE)
                out = io.StringIO()
                ir_generator.generate_ir([self.temp_input_filename], analysis, out, False)
                handler = self.handler(out.getvalue(), name)
                store = "IR_STORE_TEMP_DST(IR_A(ir_word), attr);"
                retire = handler.index("retire_published_values(")
                self.assertEqual(handler.count(store), 1)
                self.assertLess(handler.index(store), retire)
                self.assertIn(
                    "((IR_IS_LOCAL(IR_A(ir_word)) || IR_A(ir_word) == IR_NO_DST) ? attr : PyValue_NULL)",
                              handler[retire:])
                self.assertIn("null = ((oparg & 1) ? &IR_SLOT(IR_A(ir_word) + 1) : NULL);", handler[retire:])
                self.assertIn("null[0] = PyValue_NULL;", handler[retire:])
                self.assertIn("if (IR_IS_LOCAL(IR_A(ir_word)) || IR_A(ir_word) == IR_NO_DST) {\n"
                              "                IR_STORE_DST(IR_A(ir_word), attr);",
                              handler[retire:])
                self.assertIn("finish_displaced_local(", handler[retire:])

    def test_canonical_home_publication(self):
        source = """
        op(_ATTR, (owner -- attr)) {
            PyValueBorrow found = PyValue_Borrow(owner);
            attr = IR_CANONICAL_HOME_DUP(found);
            PyValue receiver = OWNER(owner);
            IR_RETIRE_PUBLISHED_RESULT(attr, receiver);
        }
        op(_PUSH_NULL_CONDITIONAL, ( -- null[oparg & 1])) {
            if (oparg & 1) {
                null[0] = PyValue_NULL;
            }
        }
        macro(LOAD_ATTR_SHAPE) = _ATTR + _PUSH_NULL_CONDITIONAL;
        """
        handler = self.handler(self.ir_output(source), "LOAD_ATTR_SHAPE")
        direct = "IR_STORE_CANONICAL_TEMP_DST(IR_A(ir_word), attr);"
        retire = handler.index("retire_published_values(")
        failure = handler.index("if (retire_failed < 0)", retire)
        local = handler.index("IR_STORE_DST(IR_A(ir_word), attr);", failure)
        self.assertEqual(handler.count(direct), 1)
        self.assertLess(handler.index(direct), retire)
        self.assertIn("PROCESS_ERROR_AT(RAISE, 0, next_instr - 1)", handler[failure:local])
        self.assertIn("if (IR_IS_LOCAL(IR_A(ir_word)) || IR_A(ir_word) == IR_NO_DST)", handler[failure:local])
        self.assertIn("null[0] = PyValue_NULL;", handler[retire:])

    def test_canonical_home_declaration_rejections(self):
        template = """
        inst(OP, (owner -- attr)) {{
            PyValueBorrow found = PyValue_Borrow(owner);
            {assignment}
            {extra}
            PyValue receiver = OWNER(owner);
            {marker}
        }}
        """
        cases = (
            ("other = IR_CANONICAL_HOME_DUP(found);", "", "IR_RETIRE_PUBLISHED_RESULT(attr, receiver);"),
            ("attr = IR_CANONICAL_HOME_DUP(found);", "IR_CANONICAL_HOME_DUP(found);", "IR_RETIRE_PUBLISHED_RESULT(attr, receiver);"),
            ("if (oparg) { attr = IR_CANONICAL_HOME_DUP(found); }", "", "IR_RETIRE_PUBLISHED_RESULT(attr, receiver);"),
            ("attr = IR_CANONICAL_HOME_DUP(found);", "", "IR_RETIRE_PUBLISHED(receiver);"),
            ("attr = IR_CANONICAL_HOME_DUP(found);", "attr = PyValue_DUP(owner);", "IR_RETIRE_PUBLISHED_RESULT(attr, receiver);"),
            ("attr = IR_CANONICAL_HOME_DUP(found);", "mutate(&attr);", "IR_RETIRE_PUBLISHED_RESULT(attr, receiver);"),
            ("attr = IR_CANONICAL_HOME_DUP(found);", "escape(attr);", "IR_RETIRE_PUBLISHED_RESULT(attr, receiver);"),
            ("attr = IR_CANONICAL_HOME_DUP(found + 1);", "", "IR_RETIRE_PUBLISHED_RESULT(attr, receiver);"),
        )
        for assignment, extra, marker in cases:
            with self.subTest(assignment=assignment, extra=extra, marker=marker):
                with self.assertRaises(SyntaxError):
                    self.ir_output(template.format(assignment=assignment,
                                                   extra=extra, marker=marker))

    def test_retirement_publication_rejects_wrong_output(self):
        input = """
        inst(LOAD_ATTR_SHAPE, (owner -- other, attr)) {
            other = PyValue_DUP(owner);
            attr = PyValue_DUP(owner);
            PyValue receiver = OWNER(owner);
            IR_RETIRE_PUBLISHED_RESULT(attr, receiver);
        }
        """
        with self.assertRaisesRegex(
                SyntaxError, r"published result attr is not the scalar ROLE_DST"):
            self.ir_output(input)

    def test_retirement_publication_rejects_conditional_marker(self):
        input = """
        inst(LOAD_ATTR_SHAPE, (owner -- attr)) {
            attr = PyValue_DUP(owner);
            PyValue receiver = OWNER(owner);
            if (flag) {
                IR_RETIRE_PUBLISHED_RESULT(attr, receiver);
            }
        }
        """
        with self.assertRaisesRegex(
                SyntaxError, r"result marker must be a top-level final statement"):
            self.ir_output(input)

    def test_retirement_publication_rejects_conditional_result_assignment(self):
        input = """
        inst(LOAD_ATTR_SHAPE, (owner -- attr)) {
            if (flag) {
                attr = PyValue_DUP(owner);
            }
            PyValue receiver = OWNER(owner);
            IR_RETIRE_PUBLISHED_RESULT(attr, receiver);
        }
        """
        with self.assertRaisesRegex(
                SyntaxError, r"one unconditional top-level assignment"):
            self.ir_output(input)

    def test_retirement_publication_rejects_consumed_result(self):
        input = """
        inst(LOAD_ATTR_SHAPE, (owner -- attr)) {
            attr = PyValue_DUP(owner);
            PyValue_CLOSE(attr);
            PyValue receiver = OWNER(owner);
            IR_RETIRE_PUBLISHED_RESULT(attr, receiver);
        }
        """
        with self.assertRaisesRegex(
                SyntaxError, r"result attr is used before receiver retirement"):
            self.ir_output(input)

    def test_retirement_publication_rejects_comma_consumed_result(self):
        input = """
        inst(LOAD_ATTR_SHAPE, (owner -- attr)) {
            attr = PyValue_DUP(owner), PyValue_CLOSE(attr);
            PyValue receiver = OWNER(owner);
            IR_RETIRE_PUBLISHED_RESULT(attr, receiver);
        }
        """
        with self.assertRaisesRegex(
                SyntaxError, r"result assignment must bind attr only once"):
            self.ir_output(input)

    def test_retirement_publication_rejects_conditional_expression_assignment(self):
        input = """
        inst(LOAD_ATTR_SHAPE, (owner -- attr)) {
            flag ? (attr = PyValue_DUP(owner)) : PyValue_DUP(owner);
            PyValue receiver = OWNER(owner);
            IR_RETIRE_PUBLISHED_RESULT(attr, receiver);
        }
        """
        with self.assertRaisesRegex(
                SyntaxError, r"one unconditional top-level assignment"):
            self.ir_output(input)

    def test_retirement_publication_rejects_reassigned_receiver(self):
        input = """
        inst(LOAD_ATTR_SHAPE, (owner -- attr)) {
            attr = PyValue_DUP(owner);
            PyValue receiver = OWNER(owner);
            receiver = PyValue_DUP(owner);
            IR_RETIRE_PUBLISHED_RESULT(attr, receiver);
        }
        """
        with self.assertRaisesRegex(
                SyntaxError, r"unmodified top-level OWNER\(\) result"):
            self.ir_output(input)

    def test_retirement_publication_rejects_duplicate_marker(self):
        input = """
        inst(LOAD_ATTR_SHAPE, (owner -- attr)) {
            attr = PyValue_DUP(owner);
            PyValue receiver = OWNER(owner);
            IR_RETIRE_PUBLISHED_RESULT(attr, receiver);
            IR_RETIRE_PUBLISHED_RESULT(attr, receiver);
        }
        """
        with self.assertRaisesRegex(
                SyntaxError, r"duplicate result publication declaration"):
            self.ir_output(input)

    def test_retirement_publication_rejects_result_address(self):
        input = """
        inst(LOAD_ATTR_SHAPE, (owner -- attr)) {
            attr = PyValue_DUP(owner);
            PyValue *result = &attr;
            PyValue receiver = OWNER(owner);
            IR_RETIRE_PUBLISHED_RESULT(attr, receiver);
        }
        """
        with self.assertRaisesRegex(
                SyntaxError, r"address of a handler local"):
            self.ir_output(input)

    def test_retirement_publication_rejects_suffix_escape(self):
        input = """
        op(_ATTR, (owner -- attr)) {
            attr = PyValue_DUP(owner);
            PyValue receiver = OWNER(owner);
            IR_RETIRE_PUBLISHED_RESULT(attr, receiver);
        }
        op(_ESCAPE, ( -- )) {
            PyObject *result = spam_call();
        }
        macro(LOAD_ATTR_SHAPE) = _ATTR + _ESCAPE;
        """
        with self.assertRaisesRegex(
                SyntaxError, r"publication suffix has an effect boundary"):
            self.ir_output(input)

    def test_retirement_publication_rejects_suffix_read(self):
        input = """
        op(_ATTR, (owner -- attr)) {
            attr = PyValue_DUP(owner);
            PyValue receiver = OWNER(owner);
            IR_RETIRE_PUBLISHED_RESULT(attr, receiver);
        }
        op(_PASS_ATTR, (attr -- attr)) {
            (void)0;
        }
        macro(LOAD_ATTR_SHAPE) = _ATTR + _PASS_ATTR;
        """
        with self.assertRaisesRegex(
                SyntaxError, r"publication suffix reads the declared result"):
            self.ir_output(input)

    def test_retirement_publication_rejects_suffix_alias_read(self):
        input = """
        op(_ATTR, (owner -- attr)) {
            attr = PyValue_DUP(owner);
            PyValue receiver = OWNER(owner);
            IR_RETIRE_PUBLISHED_RESULT(attr, receiver);
        }
        op(_READ, ( -- )) {
            PyValue alias = PyValue_DUP(attr);
        }
        macro(LOAD_ATTR_SHAPE) = _ATTR + _READ;
        """
        with self.assertRaisesRegex(
                SyntaxError, r"publication suffix references result attr"):
            self.ir_output(input)

    def test_generic_attribute_reader_phase(self):
        input = """
        inst(LOAD_ATTR, (owner -- attr)) {
            PyValue receiver = TAKE(owner);
            PyValue pending = PyValue_NULL;
            PyValue pending_self = PyValue_NULL;
            int method = 0;
            IR_COMPLETE_ATTR_READER(pending, pending_self, receiver, method);
            attr = pending;
        }
        """
        analysis = self.analyze(input)
        formats = ir_generator.compute_formats(analysis)
        contract = ir_generator.handler_contracts(analysis, formats)["LOAD_ATTR"]
        self.assertEqual(
            contract.result_phase,
            ir_generator.RESULT_PHASE_DST_TEMP_CANONICAL_BEFORE_RETIRE)

    def test_generic_reader_allows_abort_cleanup(self):
        input = """
        inst(LOAD_ATTR, (owner -- attr)) {
            PyValue receiver = TAKE(owner);
            PyValue pending = PyValue_NULL;
            PyValue pending_self = PyValue_NULL;
            int method = 0;
            if (PyValue_IsNull(pending)) {
                value_operation_close3(receiver, PyValue_NULL, PyValue_NULL);
                IR_ABORT_VALUE_READER();
            }
            IR_COMPLETE_ATTR_READER(pending, pending_self, receiver, method);
            attr = pending;
        }
        """
        analysis = self.analyze(input)
        formats = ir_generator.compute_formats(analysis)
        contract = ir_generator.handler_contracts(analysis, formats)["LOAD_ATTR"]
        self.assertEqual(
            contract.result_phase,
            ir_generator.RESULT_PHASE_DST_TEMP_CANONICAL_BEFORE_RETIRE)

    def test_generic_reader_rejects_consumed_inputs(self):
        prefix = """
        inst(LOAD_ATTR, (owner -- attr)) {
            PyValue receiver = TAKE(owner);
            PyValue pending = PyValue_NULL;
            PyValue pending_self = PyValue_NULL;
            int method = 0;
            %s
            IR_COMPLETE_ATTR_READER(pending, pending_self, receiver, method);
            attr = pending;
        }
        """
        phase_error = r"generic completion consumes, mutates, or addresses"
        cases = (
            ("PyValue_CLOSE(receiver);", "receiver release", phase_error),
            ("PyValue_CLOSE(pending);", "pending release", phase_error),
            ("PyValue_CLOSE(pending_self);", "pending-self release", phase_error),
            ("value_operation_close3(receiver, PyValue_NULL, PyValue_NULL);",
             "reader close", phase_error),
            ("PyValue_AsKnownObjectSteal(receiver);", "view take", phase_error),
            ("if (flag) { PyValue_CLOSE(receiver); }", "conditional release",
             phase_error),
            ("for (; PyValue_IsNull(TAKE(receiver)); ) { }", "loop-header take",
             phase_error),
            ("receiver.bits = 0;", "member mutation", phase_error),
            ("pending += receiver;", "compound mutation", phase_error),
            ("consume(&pending);", "address escape",
             r"address of a handler local"),
        )
        for cleanup, name, diagnostic in cases:
            with self.subTest(name=name):
                with self.assertRaisesRegex(
                        SyntaxError, diagnostic):
                    self.ir_output(prefix % cleanup)

    def test_register_style(self):
        # Inputs that are only used as objects are read through their fields
        # and may be borrowed locals.  (The result of a call that runs no
        # code would be followed as derived from the objects, S7.)
        input = """
        inst(OP, (left, right -- res)) {
            PyObject *l = PyValue_AsKnownObjectBorrow(left);
            PyObject *r = PyValue_AsKnownObjectBorrow(right);
            PyObject *o = spam_call(l, r);
            DECREF_INPUTS();
            ERROR_IF(o == NULL);
            res = PyValue_FromObjectSteal(o);
        }
    """
        output = """
        TARGET(OP) {
            int opcode = OP;
            (void)(opcode);
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            PyObject *value_boxes[5];
            _PyValueTemps value_owners;
            value_owners = (_PyValueTemps){value_boxes, 0, 5};
            _PyValueTemps *value_temps = &value_owners;
            PyValue ir_displaced = PyValue_NULL;
            (void)ir_displaced;
            PyValueBorrow left;
            PyValueBorrow right;
            PyValue res;
            PyValue ir_own_B = PyValue_NULL;
            (void)ir_own_B;
            PyValue ir_own_C = PyValue_NULL;
            (void)ir_own_C;
            PyObject *ir_obj_left = NULL;
            PyObject *ir_obj_right = NULL;
            right = IR_VIEW(IR_C(ir_word));
            left = IR_VIEW(IR_B(ir_word));
            if (Py_UNLIKELY(PyValue_IsInline(left))) {
                ir_obj_left = _PyValueTemps_Box(value_temps, left);
                if (ir_obj_left == NULL) {
                    PROCESS_ERROR_AT(RAISE, 0, this_instr);
                }
                BOX_STAT_INC(OP);
            }
            else {
                ir_obj_left = PyValue_AsKnownObjectBorrow(left);
            }
            if (Py_UNLIKELY(PyValue_IsInline(right))) {
                ir_obj_right = _PyValueTemps_Box(value_temps, right);
                if (ir_obj_right == NULL) {
                    PROCESS_ERROR_AT(RAISE, 0, this_instr);
                }
                BOX_STAT_INC(OP);
            }
            else {
                ir_obj_right = PyValue_AsKnownObjectBorrow(right);
            }
            PyObject *l = ir_obj_left;
            PyObject *r = ir_obj_right;
            PyObject *o = spam_call(l, r);
            ir_own_C = IR_CONSUME_INPUT(IR_C(ir_word), right);
            PyValue_XCLOSE(ir_own_C);
            ir_own_B = IR_CONSUME_INPUT(IR_B(ir_word), left);
            PyValue_XCLOSE(ir_own_B);
            if (o == NULL) {
                PROCESS_ERROR_AT(RAISE, 0, this_instr);
            }
            res = PyValue_FromObjectSteal(o);
            IR_STORE_DST(IR_A(ir_word), res);
            if (!PyValue_IsNull(ir_displaced) &&
                finish_displaced_local(tstate, frame, next_instr, value_temps, ir_displaced) < 0) {
                next_instr = frame->instr_ptr + 1;
                PROCESS_ERROR_AT(RAISE, 0, next_instr - 1);
            }
            DISPATCH();
        }


    """
        self.run_ir_test(input, output)
        fmt = self.formats(input)["OP"]
        self.assertFalse(fmt.window)
        self.assertEqual(fmt.roles, [ir_generator.ROLE_DST,
                                     f"{ir_generator.ROLE_IN0} + 0",
                                     f"{ir_generator.ROLE_IN0} + 1",
                                     ir_generator.ROLE_NONE])
        self.assertEqual(fmt.dst_offset, -2)
        self.assertEqual(fmt.borrow_mask, 0b11)

    def test_borrow_mask(self):
        input = """
        inst(OP_PASS, (value -- value, res)) {
            res = SPAM(PyValue_AsKnownObjectBorrow(value));
        }
        inst(OP_OBJ, (value -- res)) {
            res = SPAM(PyValue_AsKnownObjectBorrow(value));
            PyValue_CLOSE(value);
        }
        inst(OP_FAMILY, (value -- res)) {
            res = SPAM(PyValue_AsKnownObjectBorrow(value));
            PyValue_CLOSE(value);
        }
        inst(OP_FAMILY_RAW, (value -- res)) {
            res = SPAM_RAW(PyValue_IsNone(value));
            PyValue_CLOSE(value);
        }
        family(OP_FAMILY, 0) = { OP_FAMILY_RAW };
        inst(OP_KEEP, (value -- res)) {
            res = SPAM_KEEP(value);
        }
        inst(OP_STORE, (value --)) {
            GETLOCAL(oparg) = value;
            DEAD(value);
        }
    """
        formats = self.formats(input)
        self.assertEqual(formats["OP_OBJ"].borrow_mask, 1)
        # A local read in place holds no count (pyjit/design/pyvalue.md D4),
        # so no local may reach an input that a handler may keep: one passed
        # to a function that is not known to only read it, or stored.
        self.assertEqual(formats["OP_KEEP"].borrow_mask, 0)
        self.assertEqual(formats["OP_STORE"].borrow_mask, 0)
        # An input that stays on the stack is not borrowed: the stack slot
        # of a borrowed local holds NULL.
        self.assertEqual(formats["OP_PASS"].borrow_mask, 0)
        self.assertEqual(formats["OP_PASS"].dst_offset, 0)
        # A member that consumes the input raw reads it borrowed, as
        # LOAD_FAST_BORROW would push it.
        self.assertEqual(formats["OP_FAMILY"].borrow_mask, 1)
        self.assertIs(formats["OP_FAMILY_RAW"], formats["OP_FAMILY"])

    def test_views_and_owners(self):
        # An input that may name a local is read as a view, a PyValueBorrow
        # that holds the slot's bits and no count (register-ir-v2-a5b.md
        # 4.2).  Its owner, ir_own_<F>, adopts the view when the frame gives
        # up the slot: a temporary's value, or NULL for a local.  A release
        # of the view is the owner's; TAKE() gives the value a count of its
        # own, the owner's or a DUP of a local.
        input = """
        inst(OP_CLOSE, (value --)) {
            SPAM(PyValue_AsKnownObjectBorrow(value));
            PyValue_CLOSE(value);
        }
        inst(OP_HELD, (value -- res)) {
            res = TAKE(value);
        }
        inst(OP_STEAL, (value -- res)) {
            PyObject *o = PyValue_AsKnownObjectSteal(value);
            res = PyValue_FromObjectSteal(SPAM(o));
        }
        inst(OP_ALIAS, (value -- res)) {
            if (SPAM(PyValue_AsKnownObjectBorrow(value))) {
                res = TAKE(value);
            }
            else {
                PyValue_CLOSE(value);
                res = PyValue_NULL;
            }
        }
    """
        formats = self.formats(input)
        for name in ("OP_CLOSE", "OP_HELD", "OP_STEAL", "OP_ALIAS"):
            self.assertEqual(formats[name].borrow_mask, 1, name)
        output = self.ir_output(input)
        close = self.handler(output, "OP_CLOSE")
        self.assertIn("PyValueBorrow value;", close)
        self.assertIn("PyValue ir_own_A = PyValue_NULL;", close)
        self.assertIn("value = IR_VIEW(IR_A(ir_word));", close)
        self.assertIn("ir_own_A = IR_CONSUME_INPUT(IR_A(ir_word), value);\n"
                      "            PyValue_XCLOSE(ir_own_A);", close)
        # No release of a view
        self.assertNotRegex(output, r"\n\s*(_?PyValue_\w*CLOSE\w*)\(value")
        self.assertIn("ir_own_B = IR_CONSUME_INPUT(IR_B(ir_word), value);\n"
                      "            res = IR_TAKE(value, ir_own_B);",
                      self.handler(output, "OP_HELD"))
        self.assertIn("PyValue_AsKnownObjectSteal(IR_TAKE(value, ir_own_B))",
                      self.handler(output, "OP_STEAL"))
        alias = self.handler(output, "OP_ALIAS")
        self.assertIn("res = IR_TAKE(value, ir_own_B);", alias)
        self.assertEqual(alias.count("IR_CONSUME_INPUT"), 2)
        self.assertNotIn("TAKE(", output.replace("IR_TAKE(", ""))
        self.assertNotIn("OWNER(", output)

    def test_view_stored_refused(self):
        # A view has no count, so no slot or home may hold it: an output that
        # a handler stores is assigned TAKE() of it
        input = """
        inst(OP, (value -- res)) {
            res = value;
            DEAD(value);
        }
    """
        with self.assertRaisesRegex(SyntaxError,
                                    "output 'res' would be a view of 'value'"):
            self.ir_output(input)

    def test_take_after_the_consume(self):
        # The frame gives up the slot once, where the first of the input's
        # views dies or is taken; a TAKE after that reads the same owner
        input = """
        op(_FIRST, (value -- copy)) {
            copy = value;
            DEAD(value);
        }
        op(_SECOND, (copy -- res)) {
            res = TAKE(copy);
        }
        macro(OP) = _FIRST + _SECOND;
    """
        output = self.handler(self.ir_output(input), "OP")
        self.assertEqual(output.count("IR_CONSUME_INPUT"), 1)
        consume = output.index(
            "ir_own_B = IR_CONSUME_INPUT(IR_B(ir_word), copy);")
        self.assertLess(output.index("// _FIRST"), consume)
        self.assertLess(consume, output.index("// _SECOND"))
        self.assertIn("res = IR_TAKE(copy, ir_own_B);", output)
        self.assertIn("PyValueBorrow copy;", output)

    def test_no_addressable_locals(self):
        # The lint: no uop or label body takes the address of a local it
        # declares or of a stack item that is not an array
        bad = [
            ("&held", "PyValue held = PyValue_NULL; spam_call(&held);"),
            ("&scope", "_PyNativeScope scope; spam_call(&scope.result);"),
            ("&small", "PyValue small[2]; spam_call(&small[0]);"),
            ("&count", "int other, count = 0; spam_call(&count);"),
            ("&o", "PyObject *o = NULL; spam_call(&o);"),
            ("&value", "spam_call(&value);"),
            ("&oparg", "spam_call(&oparg);"),
            # Unary contexts other than an argument or an initializer
            ("&held", "PyValue held = PyValue_NULL; spam_call(oparg ? &held : NULL);"),
            ("&held", "PyValue held = PyValue_NULL; spam_call(!&held);"),
            ("&held", "PyValue held = PyValue_NULL; spam_call((void *)&held);"),
            ("&held", "PyValue held = PyValue_NULL; spam_call(&(held));"),
            ("&held", "PyValue held = PyValue_NULL; int x = spam_call() == &held;"),
            ("&held", "PyValue held = PyValue_NULL; intptr_t x = 0; x += &held;"),
            # A local array decays to its address
            ("small\\[\\] \\(a local array", "PyValue small[2]; spam_call(small);"),
            # The locals that the generator declares around a body
            ("&ir_displaced", "spam_call(&ir_displaced);"),
            ("&call_state", "spam_call(&call_state);"),
            ("&ir_own_B", "spam_call(&ir_own_B);"),
        ]
        for site, body in bad:
            with self.subTest(site=site):
                input = f"""
        inst(OP, (value -- )) {{
            {body}
            DEAD(value);
        }}
    """
                with self.assertRaisesRegex(SyntaxError, f"OP: line \\d+: {site}\\b"):
                    self.ir_output(input)
        good = [
            "PyObject *o = spam_call(); spam_call(&o->ob_type);",
            "PyValue *p = window; spam_call(&p[1]);",
            "spam_call(&window[1], &IR_SLOT(0));",
            "spam_call(&_PyInstrumentation_MISSING);",
            "int a = 1, b = 2, sum; spam_call(__builtin_add_overflow(a, b, &sum));",
            "int a = oparg & 1; spam_call(a);",
            "int a = spam_call(oparg) & oparg; spam_call(a);",
            "int a = (int)sizeof(PyValue) & oparg; spam_call(a);",
        ]
        for body in good:
            with self.subTest(body=body):
                input = f"""
        inst(OP, (value -- )) {{
            {body}
            DEAD(value);
        }}
    """
                self.ir_output(input)

    def test_value_operation_by_value(self):
        # R-a, R-b: the helpers of a value operation take the handler's
        # values, its frame and instruction, and slot numbers from the word;
        # there is no operation record
        input = """
        inst(OP, (left, right -- res)) {
            PyValue held_left = TAKE(left);
            PyValue held_right = TAKE(right);
            INPUTS_DEAD();
            SYNC_SP();
            PyValue pending = spam_call(held_left, held_right);
            if (PyValue_IsNull(pending)) {
                IR_RESTORE_VALUE_INPUTS(held_left, held_right);
                value_operation_close3(held_right, held_left, PyValue_NULL);
                IR_ABORT_VALUE_READER();
            }
            IR_COMPLETE_VALUE_READER(pending, held_right, held_left);
            res = pending;
        }
    """
        output = self.handler(self.ir_output(input), "OP")
        self.assertIn("_PyValuePair ir_rest = value_operation_restore2(tstate, "
                      "frame, held_left, held_right, IR_B(ir_word), IR_C(ir_word), 0);\n"
                      "                    held_left = ir_rest.first;\n"
                      "                    held_right = ir_rest.second;", output)
        self.assertIn("(void)value_operation_finish(tstate, frame, this_instr, ", output)
        self.assertIn("pending = value_reader_complete(tstate, frame, this_instr, "
                      "NULL, IR_A(ir_word), pending, held_right, held_left, 0);\n"
                      "            if (PyValue_IsNull(pending)) {", output)
        self.assertNotIn("operation", output.replace("value_operation", ""))
        # The record form is gone
        old = input.replace("IR_ABORT_VALUE_READER()", "IR_ABORT_VALUE_READER(operation)")
        with self.assertRaisesRegex(SyntaxError, "IR_ABORT_VALUE_READER takes 0 variables"):
            self.ir_output(old)

    def test_owner(self):
        # OWNER() gives the owner itself, NULL for a local, which only
        # IR_RETIRE_PUBLISHED may read, with no call that can run code
        # between them
        input = """
        inst(OP, (value -- )) {
            PyValue held = OWNER(value);
            IR_RETIRE_PUBLISHED(held);
        }
    """
        output = self.handler(self.ir_output(input), "OP")
        self.assertIn("ir_own_A = IR_CONSUME_INPUT(IR_A(ir_word), value);\n"
                      "            PyValue held = ir_own_A;", output)
        self.assertIn("retire_published_values(", output)
        # By value (R-a): the retired owner is dead after the call
        self.assertIn(", held, PyValue_NULL, PyValue_NULL);", output)
        self.assertNotIn("&held", output)
        gap = """
        inst(OP, (value -- )) {
            PyValue held = OWNER(value);
            spam_call();
            IR_RETIRE_PUBLISHED(held);
        }
    """
        with self.assertRaisesRegex(SyntaxError,
                                    "code can run between OWNER\\(\\) and "
                                    "IR_RETIRE_PUBLISHED"):
            self.ir_output(gap)
        stray = """
        inst(OP, (value -- )) {
            PyValue held = OWNER(value);
            SPAM(held);
            IR_RETIRE_PUBLISHED(held);
        }
    """
        with self.assertRaisesRegex(SyntaxError, "held"):
            self.ir_output(stray)

    def test_stale_view_refused(self):
        # S7: a call that can run code can rebind a local and release the
        # value that its view, or the object of the view, reads.  Neither is
        # read after the call unless the body took the view before it.
        for body in ("int err = spam_call(PyValue_AsKnownObjectBorrow(value));\n"
                     "res = TAKE(value);\n"
                     "ERROR_IF(err);",
                     "PyObject *o = PyValue_AsKnownObjectBorrow(value);\n"
                     "int err = spam_call();\n"
                     "res = SPAM(o);\n"
                     "PyValue_CLOSE(value);\n"
                     "ERROR_IF(err);",
                     # A cast of the object, and the object that
                     # _PyValueTemps_Object() gives, are followed too
                     "PyTupleObject *t = (PyTupleObject *)"
                     "PyValue_AsKnownObjectBorrow(value);\n"
                     "int err = spam_call();\n"
                     "res = SPAM(t);\n"
                     "PyValue_CLOSE(value);\n"
                     "ERROR_IF(err);",
                     "PyObject *t = _PyValueTemps_Object(value_temps, value);\n"
                     "int err = spam_call();\n"
                     "res = SPAM(t);\n"
                     "PyValue_CLOSE(value);\n"
                     "ERROR_IF(err);",
                     # So are pointers derived from the object: its type,
                     # and an address inside it
                     "PyTypeObject *tp = Py_TYPE(PyValue_AsKnownObjectBorrow(value));\n"
                     "int err = spam_call();\n"
                     "res = SPAM(tp->tp_name);\n"
                     "PyValue_CLOSE(value);\n"
                     "ERROR_IF(err);",
                     "PyObject **t = (PyObject **)"
                     "((char *)PyValue_AsKnownObjectBorrow(value) + 8);\n"
                     "int err = spam_call();\n"
                     "res = SPAM(*t);\n"
                     "PyValue_CLOSE(value);\n"
                     "ERROR_IF(err);",
                     # through an assignment, or a type that may be a pointer
                     "PyTypeObject *tp;\n"
                     "tp = Py_TYPE(PyValue_AsKnownObjectBorrow(value));\n"
                     "int err = spam_call();\n"
                     "res = SPAM(tp->tp_name);\n"
                     "PyValue_CLOSE(value);\n"
                     "ERROR_IF(err);",
                     "getattrofunc f = "
                     "Py_TYPE(PyValue_AsKnownObjectBorrow(value))->tp_getattro;\n"
                     "int err = spam_call();\n"
                     "res = SPAM(f);\n"
                     "PyValue_CLOSE(value);\n"
                     "ERROR_IF(err);",
                     # C does not order the operands of an expression: the
                     # view may be read after the call
                     "res = SPAM(spam_call(), PyValue_AsKnownObjectBorrow(value));\n"
                     "PyValue_CLOSE(value);"):
            with self.subTest(body=body):
                with self.assertRaisesRegex(SyntaxError, r"\(S7\)"):
                    self.ir_output(f"""
                    inst(OP, (value -- res)) {{
                        {body}
                    }}
                """)
        # A release after the call reads the owner, and a TAKE before the
        # call keeps the object alive
        output = self.handler(self.ir_output("""
        inst(OP, (value -- res)) {
            PyObject *o = PyValue_AsKnownObjectBorrow(value);
            PyValue held = TAKE(value);
            int err = spam_call(o);
            res = SPAM(o);
            PyValue_CLOSE(held);
            ERROR_IF(err);
        }
        inst(OP_RELEASE, (value -- res)) {
            int err = spam_call(PyValue_AsKnownObjectBorrow(value));
            PyValue_CLOSE(value);
            ERROR_IF(err);
            res = PyValue_None;
        }
    """), "OP")
        self.assertLess(output.index("PyValue held = IR_TAKE(value, ir_own_B);"),
                        output.index("spam_call("))
        # The designator and the arguments of the call are read before it
        # runs, and a number read from the object does not dangle
        input = """
        inst(OP, (value -- res)) {
            res = SPAM(spam_call(PyValue_AsKnownObjectBorrow(value)));
            PyValue_CLOSE(value);
        }
        inst(OP_POINTER, (value -- res)) {
            PyObject *o = PyValue_AsKnownObjectBorrow(value);
            binaryfunc func = Py_TYPE(o)->tp_as_number->nb_add;
            Py_ssize_t n = Py_TYPE(o)->tp_basicsize;
            PyObject *r = (*func)(o, o);
            SPAM(n, r);
            PyValue_CLOSE(value);
            res = PyValue_None;
        }
        inst(OP_FIELD, (value -- res)) {
            PyObject *o = PyValue_AsKnownObjectBorrow(value);
            PyObject *r = Py_TYPE(o)->tp_as_number->func(o, o);
            SPAM(r);
            PyValue_CLOSE(value);
            res = PyValue_None;
        }
    """
        formats = self.formats(input)
        for name in ("OP", "OP_POINTER", "OP_FIELD"):
            with self.subTest(name=name):
                self.assertEqual(formats[name].borrow_mask, 1)
        self.assertIn("res = SPAM(spam_call(ir_obj_value));",
                      self.handler(self.ir_output(input), "OP"))

    def test_owner_taken_once(self):
        # A view and its copy read one owner: a take or release of one of
        # them spends it and kills both, so a second would count it twice
        double = """
        op(_COPY, (value -- value, copy)) {
            copy = value;
        }
        op(_USE, (value, copy -- %s)) {
            res = TAKE(copy);
            %s
        }
        macro(OP) = _COPY + _USE;
    """
        for second in ("PyValue_CLOSE(value);", "PyValue_CLOSE(copy);",
                       "res2 = TAKE(value);"):
            with self.subTest(second=second):
                outputs = "res, res2" if "res2" in second else "res"
                with self.assertRaisesRegex(SyntaxError,
                                            "taken or released already|"
                                            "read after the body took or "
                                            "released"):
                    self.ir_output(double % (outputs, second))
        # DECREF_INPUTS releases no input that a take killed
        output = self.handler(self.ir_output(double % ("res", "DECREF_INPUTS();")),
                              "OP")
        self.assertEqual(output.count("ir_own_B"),
                         output.count("ir_own_B = IR_CONSUME_INPUT") + 3)
        # Exclusive branches spend it once each
        output = self.handler(self.ir_output("""
        inst(OP, (value -- res)) {
            if (SPAM_FLAG()) {
                res = TAKE(value);
            }
            else {
                res = SPAM(PyValue_AsKnownObjectBorrow(value));
                PyValue_CLOSE(value);
            }
        }
    """), "OP")
        self.assertIn("res = IR_TAKE(value, ir_own_B);", output)
        self.assertIn("PyValue_XCLOSE(ir_own_B);", output)
        # The take kills every view and copy of the input: one is not read
        # after it, and none needs DEAD()
        copy = """
        op(_COPY, (value -- value, copy)) {
            copy = value;
        }
        op(_USE, (value, copy -- res)) {
            res = TAKE(copy);
            %s
        }
        macro(OP) = _COPY + _USE;
    """
        for read in ("int x = PyValue_IsNone(value);\nSPAM(x);",
                     "if (PyValue_IsNone(value)) {\nSPAM_FLAG();\n}"):
            with self.subTest(read=read):
                with self.assertRaisesRegex(SyntaxError,
                                            "view 'value' is read after the "
                                            "body took or released"):
                    self.ir_output(copy % read)
        for rest in ("", "DEAD(value);"):
            with self.subTest(rest=rest):
                output = self.handler(self.ir_output(copy % rest), "OP")
                self.assertIn("res = IR_TAKE(copy, ir_own_B);", output)

    def test_exit_after_take_refused(self):
        # A deopt or exit goes to an instruction that reads the inputs from
        # their slots, which a take cleared, and DISPATCH_SAME_OPARG runs
        # the handler again
        with self.assertRaisesRegex(SyntaxError,
                                    "DEOPT_IF after the handler took"):
            self.ir_output("""
            inst(OP, (value -- res)) {
                res = SPAM(PyValue_AsKnownObjectBorrow(value));
                PyValue_CLOSE(value);
            }
            inst(OP_SPECIALIZED, (value -- res)) {
                res = TAKE(value);
                DEOPT_IF(SPAM_FLAG());
            }
            family(OP, 0) = { OP_SPECIALIZED };
        """)
        for consume in ("PyValue held = TAKE(value);\nSPAM_KEEP(held);",
                        "DEAD(value);"):
            with self.subTest(consume=consume):
                with self.assertRaisesRegex(SyntaxError,
                                            "DISPATCH_SAME_OPARG after the "
                                            "handler took"):
                    self.ir_output(f"""
                    op(_SPECIALIZE_OP, (value -- value)) {{
                        if (SPAM_FLAG()) {{
                            {consume}
                            DISPATCH_SAME_OPARG();
                        }}
                    }}
                    op(_OP, (value -- res)) {{
                        res = SPAM(PyValue_AsKnownObjectBorrow(value));
                        PyValue_CLOSE(value);
                    }}
                    macro(OP) = _SPECIALIZE_OP + _OP;
                """)

    def test_error_exits_refused(self):
        # An error exit between OWNER() and its retire leaks the owner, and
        # one after a TAKE() into an output leaks the count: the generator
        # stores no output on an error exit
        with self.assertRaisesRegex(SyntaxError,
                                    "ERROR_NO_POP between OWNER\\(\\) and "
                                    "IR_RETIRE_PUBLISHED would leak 'held'"):
            self.ir_output("""
            inst(OP, (value -- )) {
                PyValue held = OWNER(value);
                if (SPAM_FLAG()) {
                    ERROR_NO_POP();
                }
                IR_RETIRE_PUBLISHED(held);
            }
        """)
        with self.assertRaisesRegex(SyntaxError,
                                    "ERROR_IF while output 'res' holds a "
                                    "TAKE\\(\\) result"):
            self.ir_output("""
            inst(OP, (value -- res)) {
                res = TAKE(value);
                ERROR_IF(SPAM_FLAG());
            }
        """)
        # A taken local that the error path closes is the DSL's to release
        output = self.handler(self.ir_output("""
        inst(OP, (value -- res)) {
            PyValue held = TAKE(value);
            if (SPAM_FLAG()) {
                PyValue_CLOSE(held);
                ERROR_NO_POP();
            }
            res = held;
        }
    """), "OP")
        self.assertLess(output.index("PyValue held = IR_TAKE(value, ir_own_B);"),
                        output.index("PyValue_CLOSE(held);"))
        self.assertLess(output.index("PyValue_CLOSE(held);"),
                        output.index("PROCESS_ERROR_AT(RAISE, 0, this_instr);"))

    def test_owner_and_take_forms_refused(self):
        cases = {
            "OWNER\\(\\) is the initializer of a PyValue variable": """
                PyValue held;
                held = OWNER(value);
                IR_RETIRE_PUBLISHED(held);
            """,
            "OWNER\\(\\) gave 'held', which no IR_RETIRE_PUBLISHED": """
                PyValue held = OWNER(value);
            """,
            "TAKE\\(\\) takes a variable": """
                PyValue held = TAKE(SPAM_VALUE());
                PyValue_CLOSE(held);
                DEAD(value);
            """,
            "TAKE\\(x\\): not a stack variable": """
                PyValue x = PyValue_None;
                PyValue held = TAKE(x);
                PyValue_CLOSE(held);
                PyValue_CLOSE(value);
            """,
        }
        for message, body in cases.items():
            with self.subTest(message=message):
                with self.assertRaisesRegex(SyntaxError, message):
                    self.ir_output(f"""
                    inst(OP, (value -- )) {{
                        {body}
                    }}
                """)

    def test_object_local_refusals(self):
        # An input with an object local has one on every path, is read in a
        # condition through it, and is the whole argument of each read
        cases = {
            "'value' has an object local on some paths, but not all": """
            inst(OP, (value -- value)) {
                PyObject *o = PyValue_AsKnownObjectBorrow(value);
                if (SPAM(o)) {
                    PyValue_CLOSE(value);
                    value = PyValue_None;
                }
            }
            """,
            "PyValue_AsKnownObjectSteal\\(value\\) in a condition": """
            inst(OP, (value -- res)) {
                PyObject *o = PyValue_AsKnownObjectBorrow(value);
                if (PyValue_AsKnownObjectSteal(value) == NULL) {
                    SPAM(o);
                }
                res = SPAM(o);
                PyValue_CLOSE(value);
            }
            """,
            "PyValue_AsKnownObjectBorrow\\(\\) of 'value' must be its "
            "whole argument": """
            inst(OP, (value -- res)) {
                PyObject *o = PyValue_AsKnownObjectBorrow(value);
                res = SPAM(o, PyValue_AsKnownObjectBorrow(value, 0));
                PyValue_CLOSE(value);
            }
            """,
        }
        for message, input in cases.items():
            with self.subTest(message=message):
                with self.assertRaisesRegex(SyntaxError, message):
                    self.ir_output(input)

    def test_view_backstops(self):
        # The emitter's own refusals of a view retired as an owner, or
        # copied to a variable that is not on the stack, are backstops:
        # S3 (inplace_input_use) refuses both first
        with self.assertRaisesRegex(SyntaxError, "inplace_input_use"):
            self.ir_output("""
            op(_COPY, (value -- value, copy)) {
                copy = value;
            }
            op(_USE, (value, copy -- )) {
                SPAM(PyValue_AsKnownObjectBorrow(value));
                DEAD(value);
                IR_RETIRE_PUBLISHED(copy);
            }
            macro(OP) = _COPY + _USE;
        """)
        self.assertEqual(self.formats("""
        inst(OP, (value -- res)) {
            PyValue x;
            x = value;
            res = SPAM(PyValue_AsKnownObjectBorrow(x));
            PyValue_CLOSE(value);
        }
    """)["OP"].borrow_mask, 0)

    def test_take_aliases_and_specialized_close(self):
        # `out = TAKE(in);` and `held = TAKE(in); out = held;` give the
        # output the input's value
        analysis = self.analyze("""
        op(_DIRECT, (value -- res)) {
            res = TAKE(value);
        }
        op(_HELD, (value -- res)) {
            PyValue held = TAKE(value);
            res = held;
        }
    """)
        self.assertEqual(analysis.uops["_DIRECT"].aliased_outputs, ("res",))
        self.assertEqual(analysis.uops["_HELD"].aliased_outputs, ("res",))
        # A specialised close of a view releases its owner, which is NULL
        # for a local
        output = self.handler(self.ir_output("""
        inst(OP, (value -- res)) {
            res = SPAM(PyValue_AsKnownObjectBorrow(value));
            PyValue_CLOSE_SPECIALIZED(value, _PyLong_ExactDealloc);
        }
    """), "OP")
        self.assertIn("PyValue_XCLOSE_SPECIALIZED(ir_own_B, _PyLong_ExactDealloc);",
                      output)

    def test_inplace_inputs_fail_closed(self):
        # The generator refuses what it cannot follow: a value that borrows
        # a local in place on one path and not on another
        input = """
        op(_MAYBE_HELD, (value -- value)) {
            if (SPAM_FLAG()) {
                held = TAKE(value);
            }
        }
        op(_USE, (value -- res)) {
            res = SPAM(PyValue_AsKnownObjectBorrow(value));
            PyValue_CLOSE(value);
        }
        macro(OP) = _MAYBE_HELD + _USE;
    """
        with self.assertRaisesRegex(SyntaxError,
                                    "'value' is cleared on some paths"):
            self.ir_output(input)
        # A variable is a view or an owner (S8): it has one C type
        roles = """
        op(_HELD, (value -- value)) {
            value = TAKE(value);
        }
        op(_USE, (value -- res)) {
            res = SPAM(PyValue_AsKnownObjectBorrow(value));
            PyValue_CLOSE(value);
        }
        macro(OP) = _HELD + _USE;
    """
        with self.assertRaisesRegex(SyntaxError, "'value' is a view .* and an owner"):
            self.ir_output(roles)

    def test_inplace_inputs_untracked_copies(self):
        # The emitter follows a value that borrows a local in place only
        # into stack variables, and its object only into assignments.  An
        # input copied elsewhere, or whose object reaches a function that
        # takes it over or releases it, is kept: no local reaches its field.
        input = """
        inst(OP_COPY_CLOSE, (value -- res)) {
            PyValue a;
            a = value;
            DEAD(value);
            res = PyValue_FromObjectSteal(SPAM(PyValue_AsKnownObjectBorrow(a)));
            PyValue_CLOSE(a);
        }
        inst(OP_COPY_STORE, (value -- res)) {
            PyValue v2 = PyValue_None;
            v2 = value;
            DEAD(value);
            res = v2;
        }
        inst(OP_OBJECT_STEAL, (value -- res)) {
            PyObject *o = PyValue_AsKnownObjectBorrow(value);
            DEAD(value);
            res = PyValue_FromObjectSteal(o);
        }
        inst(OP_OBJECT_COPY_RELEASE, (value --)) {
            PyObject *o = PyValue_AsKnownObjectBorrow(value);
            PyObject *p;
            p = o;
            DEAD(value);
            Py_DECREF(p);
        }
        inst(OP_OBJECT_NESTED_STEAL, (value -- res)) {
            res = PyValue_FromObjectSteal(PyValue_AsKnownObjectBorrow(value));
            DEAD(value);
        }
        inst(OP_OBJECT_TARGET, (value, item --)) {
            int err = SPAM_TakeRef((PyObject *)PyValue_AsKnownObjectBorrow(value),
                                   PyValue_AsKnownObjectSteal(item));
            PyValue_CLOSE(value);
            ERROR_IF(err);
        }
    """
        formats = self.formats(input)
        for name in ("OP_COPY_CLOSE", "OP_COPY_STORE", "OP_OBJECT_STEAL",
                     "OP_OBJECT_COPY_RELEASE", "OP_OBJECT_NESTED_STEAL"):
            self.assertEqual(formats[name].borrow_mask, 0, name)
        # The target of a function that takes its other arguments is kept
        # unless the function is on the checked reader list (S3); the stolen
        # argument is taken
        self.assertEqual(formats["OP_OBJECT_TARGET"].borrow_mask, 0b10)
        with mock.patch.object(ir_generator, "OBJECT_READERS",
                               ir_generator.OBJECT_READERS | {"SPAM_TakeRef"}):
            self.assertEqual(self.formats(input)["OP_OBJECT_TARGET"].borrow_mask, 0b11)
        output = self.handler(self.ir_output(input), "OP_OBJECT_TARGET")
        self.assertIn("PyValue_AsKnownObjectSteal(IR_TAKE(item, ir_own_B))", output)

    def test_inplace_inputs_temps_object(self):
        # _PyValueTemps_Object(temps, name) gives the object of name, uncounted
        # like PyValue_AsKnownObjectBorrow's: reading it keeps the borrow,
        # releasing or stealing it drops the borrow.
        input = """
        inst(OP_TEMPS_READ, (value --)) {
            PyObject *o = _PyValueTemps_Object(value_temps, value);
            int err = SPAM(o);
            PyValue_CLOSE(value);
            ERROR_IF(err);
        }
        inst(OP_TEMPS_RELEASE, (value --)) {
            PyObject *o = _PyValueTemps_Object(value_temps, value);
            DEAD(value);
            Py_DECREF(o);
        }
        inst(OP_TEMPS_COPY_RELEASE, (value --)) {
            PyObject *o = _PyValueTemps_Object(value_temps, value);
            PyObject *p;
            p = o;
            DEAD(value);
            Py_DECREF(p);
        }
        inst(OP_TEMPS_STEAL, (value -- res)) {
            res = PyValue_FromObjectSteal(_PyValueTemps_Object(value_temps, value));
            DEAD(value);
        }
    """
        formats = self.formats(input)
        self.assertEqual(formats["OP_TEMPS_READ"].borrow_mask, 1)
        for name in ("OP_TEMPS_RELEASE", "OP_TEMPS_COPY_RELEASE",
                     "OP_TEMPS_STEAL"):
            self.assertEqual(formats[name].borrow_mask, 0, name)

    def test_consumed_inputs(self):
        # A handler takes the inputs that it consumes: the slot of a
        # temporary becomes NULL, and a local keeps its value.  No handler
        # has a stack pointer to keep.
        input = """
        inst(STORE, (owner, value --)) {
            SPAM(PyValue_AsKnownObjectBorrow(owner),
                 PyValue_AsKnownObjectBorrow(value));
            DECREF_INPUTS();
        }
        inst(BRANCH, (counter/1, cond --)) {
            _PyFbHeader *feedback = FB_COUNTER();
            int flag = PyValue_IsFalse(cond);
            DEAD(cond);
            if (flag) {
                JUMPTO(oparg);
            }
        }
    """
        formats = self.formats(input)
        self.assertEqual(formats["STORE"].roles[2:],
                         [ir_generator.ROLE_NONE, ir_generator.ROLE_NONE])
        output = self.ir_output(input)
        store = self.handler(output, "STORE")
        # A view's release is its owner's: the value of a temporary, which
        # the consume adopts, and NULL for a local
        self.assertIn("ir_own_A = IR_CONSUME_INPUT(IR_A(ir_word), owner);\n"
                      "            PyValue_XCLOSE(ir_own_A);", store)
        self.assertIn("ir_own_B = IR_CONSUME_INPUT(IR_B(ir_word), value);\n"
                      "            PyValue_XCLOSE(ir_own_B);", store)
        self.assertIn("ir_own_A = IR_CONSUME_INPUT(IR_A(ir_word), cond);",
                      self.handler(output, "BRANCH"))
        self.assertNotIn("stack_pointer", output)

    def test_store_dst(self):
        # Which results a folded STORE_FAST may send to a local, and how
        # the handler then finds the stack pointer: dst may name a local, so
        # from the top slot in a free field, or by counting.
        input = """
        inst(OP, (left, right -- res)) {
            res = SPAM(PyValue_AsKnownObjectBorrow(left),
                       PyValue_AsKnownObjectBorrow(right));
            DECREF_INPUTS();
        }
        inst(ATTR, (owner -- attr, null[oparg & 1])) {
            attr = SPAM(PyValue_AsKnownObjectBorrow(owner));
            DECREF_INPUTS();
            if (oparg & 1) {
                null[0] = PyValue_NULL;
            }
        }
        no_fold inst(ALIAS, (value -- res)) {
            res = SPAM(PyValue_AsKnownObjectBorrow(value));
            DECREF_INPUTS();
        }
        inst(PAIR, (-- a, b)) {
            a = SPAM();
            b = SPAM();
        }
    """
        formats = self.formats(input)
        self.assertTrue(formats["OP"].store_dst)
        self.assertTrue(formats["ATTR"].store_dst)
        self.assertFalse(formats["ALIAS"].store_dst)
        # PAIR never pushes its first output alone
        self.assertTrue(formats["PAIR"].store_dst)
        output = self.ir_output(input)
        self.assertIn("IR_STORE_DST(IR_A(ir_word), res);",
                      self.handler(output, "OP"))
        self.assertIn("IR_STORE_DST(IR_A(ir_word), attr);",
                      self.handler(output, "ATTR"))
        self.assertNotIn("IR_STORE_DST", self.handler(output, "ALIAS"))
        self.assertIn("IR_STORE_TEMP(IR_SLOT(IR_A(ir_word)), res);",
                      self.handler(output, "ALIAS"))
        self.assertNotIn("IR_STORE_DST", self.handler(output, "PAIR"))
        self.assertIn("assert(!IR_IS_LOCAL(IR_A(ir_word)) && IR_A(ir_word) != IR_NO_DST);",
                      self.handler(output, "PAIR"))

    def test_scalar_conditional_array_has_no_empty_home(self):
        source = """
        inst(ATTR, (owner -- attr, null[oparg & 1])) {
            attr = PyValue_DUP(owner);
            PyValue_CLOSE(owner);
            if (oparg & 1) {
                null[0] = PyValue_NULL;
            }
        }
        """
        fmt = self.formats(source)["ATTR"]
        self.assertTrue(fmt.discards)
        handler = self.handler(self.ir_output(source), "ATTR")
        self.assertIn(
            "null = ((oparg & 1) ? &IR_SLOT(IR_A(ir_word) + 1) : NULL);",
            handler)
        self.assertNotIn("null = &IR_SLOT", handler)
        self.assertIn("if (oparg & 1)", handler)
        self.assertIn("IR_STORE_DST(IR_A(ir_word), attr)", handler)

    def test_unused_outputs_pass_through(self):
        input = """
        op(_GUARD, (nos, unused -- nos, unused)) {
            EXIT_IF(!PyValue_IsObject(nos));
        }
        op(_USE, (left, right -- left, res)) {
            res = SPAM(PyValue_AsKnownObjectBorrow(right));
            PyValue_CLOSE(right);
        }
        macro(OP) = _GUARD + _USE;
    """
        fmt = self.formats(input)["OP"]
        self.assertEqual(fmt.inputs, ["nos", "unused"])
        self.assertEqual(fmt.dst_offset, -1)
        self.assertEqual(fmt.borrow_mask, 0b10)

    def test_window_style(self):
        # An array input selects the window style: A names dst and B the
        # deepest input, from which the handler addresses its window; the
        # count comes before any feedback index.
        input = """
        inst(OP, (callable, args[oparg] -- res)) {
            res = SPAM(callable, args, oparg);
            INPUTS_DEAD();
        }
    """
        output = """
        TARGET(OP) {
            int opcode = OP;
            (void)(opcode);
            _PyValueTemps *const value_temps = NULL;
            (void)value_temps;
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            PyValue ir_displaced = PyValue_NULL;
            (void)ir_displaced;
            int oparg = (int)IR_C(ir_word);
            (void)oparg;
            PyValue *window = &IR_SLOT(IR_B(ir_word));
            (void)window;
            PyValue callable;
            PyValue *args;
            PyValue res;
            args = &window[1];
            callable = window[0];
            res = SPAM(callable, args, oparg);
            IR_CLEAR_SLOTS(&window[1], oparg);
            IR_PUBLISH_TEMP(window[0], res);
            IR_MOVE_TO_DST_OR_DISCARD(IR_A(ir_word), window[0]);
            if (!PyValue_IsNull(ir_displaced) &&
                finish_displaced_local(tstate, frame, next_instr, NULL, ir_displaced) < 0) {
                next_instr = frame->instr_ptr + 1;
                PROCESS_ERROR_AT(RAISE, 0, next_instr - 1);
            }
            DISPATCH();
        }
    """
        self.run_ir_test(input, output)
        fmt = self.formats(input)["OP"]
        self.assertTrue(fmt.window)
        self.assertTrue(fmt.discards)
        self.assertEqual(fmt.roles, [ir_generator.ROLE_DST,
                                     ir_generator.ROLE_BASE,
                                     ir_generator.ROLE_IMM,
                                     ir_generator.ROLE_NONE])
        self.assertEqual(fmt.imm, ir_generator.LOC_C)
        self.assertEqual(fmt.dst_offset, 0)

    def test_window_selection(self):
        input = """
        inst(OP_FOUR, (a, b, c -- res)) {
            res = SPAM(a, b, c);
            INPUTS_DEAD();
        }
        inst(OP_FIVE, (a, b, c, d -- res)) {
            res = SPAM(a, b, c, d);
            INPUTS_DEAD();
        }
        inst(OP_THREE, (a, b, c --)) {
            SPAM(a, b, c);
            INPUTS_DEAD();
        }
        window inst(OP_ANNOTATED, (value -- res)) {
            res = SPAM(value);
            INPUTS_DEAD();
        }
        window inst(OP_SWITCH, (value -- res)) {
            frame = SPAM(value);
            res = PyValue_NULL;
            INPUTS_DEAD();
        }
    """
        formats = self.formats(input)
        # Three inputs and a destination fill the four fields; four do not
        # fit
        self.assertFalse(formats["OP_FOUR"].window)
        self.assertTrue(formats["OP_FIVE"].window)
        self.assertFalse(formats["OP_THREE"].window)
        self.assertEqual(formats["OP_THREE"].n_inputs, 3)
        self.assertTrue(formats["OP_ANNOTATED"].window)
        self.assertTrue(formats["OP_SWITCH"].window)

    def test_boxed_input(self):
        # An input that the body converts to an object gets an object local
        # at entry: its object, or a box of an inline value that value_temps
        # owns.  The input keeps its canonical value, so a use that keeps the
        # value keeps that, and the conversions read the local.
        input = """
        inst(OP, (v -- res)) {
            PyObject *v_o = PyValue_AsKnownObjectBorrow(v);
            PyValue kept = TAKE(v);
            SPAM(v_o);
            res = kept;
        }
        window inst(OP_OWNED, (v -- res)) {
            if (PyValue_IsNone(v)) {
                SPAM(PyValue_AsKnownObjectBorrow(v));
            }
            res = PyValue_FromObjectSteal(PyValue_AsKnownObjectSteal(v));
        }
    """
        output = self.ir_output(input)
        op = self.handler(output, "OP")
        self.assertIn("PyObject *ir_obj_v = NULL;", op)
        self.assertIn("ir_obj_v = _PyValueTemps_Box(value_temps, v);", op)
        self.assertIn("ir_obj_v = PyValue_AsKnownObjectBorrow(v);", op)
        self.assertIn("PyObject *v_o = ir_obj_v;", op)
        self.assertIn("PyValue kept = IR_TAKE(v, ir_own_B);", op)
        self.assertNotIn("PyValue_Box(", output)
        # An owned input: the condition reads the local, and a steal of an
        # inline value gives a count of the box
        owned = self.handler(output, "OP_OWNED")
        self.assertIn("PyValue v;", owned)
        self.assertIn("SPAM(ir_obj_v);", owned)
        self.assertIn("(PyValue_IsInline(v) ? Py_NewRef(ir_obj_v) : "
                      "PyValue_AsKnownObjectSteal(v))", owned)
        # A copy holds the value, which may be inline, so its object is read
        # through the input
        with self.assertRaisesRegex(SyntaxError, "is a copy of 'v'"):
            self.ir_output("""
            window inst(OP, (v -- res)) {
                PyObject *v_o = PyValue_AsKnownObjectBorrow(v);
                res = v;
                DEAD(v);
                SPAM(v_o, PyValue_AsKnownObjectBorrow(res));
            }
        """)

    def test_object_readers(self):
        # S3: the object of a view has no count, so it reaches only a field
        # access, a comparison, a copy or an argument of a function on the
        # checked list; any other use keeps the input out of borrow_mask
        input = """
        inst(OP_READ, (value -- res)) {
            PyObject *o = PyValue_AsKnownObjectBorrow(value);
            res = Py_TYPE(o)->tp_flags & 1 ? PyValue_True : PyValue_False;
            PyValue_CLOSE(value);
        }
        inst(OP_FIELD, (value -- res)) {
            res = ((PyTupleObject *)PyValue_AsKnownObjectBorrow(value))->ob_item[0] == NULL
                ? PyValue_True : PyValue_False;
            PyValue_CLOSE(value);
        }
        inst(OP_KEEP, (value -- res)) {
            PyObject *o = PyValue_AsKnownObjectBorrow(value);
            res = SPAM_KEEP(o);
            PyValue_CLOSE(value);
        }
        inst(OP_STORE, (value -- )) {
            SPAM_STRUCT()->field = PyValue_AsKnownObjectBorrow(value);
            PyValue_CLOSE(value);
        }
    """
        formats = self.formats(input)
        self.assertEqual(formats["OP_READ"].borrow_mask, 1)
        self.assertEqual(formats["OP_FIELD"].borrow_mask, 1)
        self.assertEqual(formats["OP_KEEP"].borrow_mask, 0)
        self.assertEqual(formats["OP_STORE"].borrow_mask, 0)

    def test_object_readers_fail_closed(self):
        # S3 follows a copy only into a plain local, and an address derived
        # from the object as the object; a store through a pointer, a
        # chained assignment and an address passed on are not reads
        input = """
        inst(OP_DEREF_STORE, (value -- )) {
            PyObject **p = SPAM_SLOT();
            *p = PyValue_AsKnownObjectBorrow(value);
            PyValue_CLOSE(value);
        }
        inst(OP_COPY_STORE, (value -- )) {
            PyObject *o = PyValue_AsKnownObjectBorrow(value);
            PyObject **p = SPAM_SLOT();
            *p = o;
            PyValue_CLOSE(value);
        }
        inst(OP_CHAINED, (value -- res)) {
            PyObject *a;
            PyObject *b;
            a = b = PyValue_AsKnownObjectBorrow(value);
            res = SPAM_KEEP(a);
            PyValue_CLOSE(value);
        }
        inst(OP_ARITHMETIC, (value -- res)) {
            PyObject *o = PyValue_AsKnownObjectBorrow(value);
            PyObject *k = o + 0;
            res = SPAM_KEEP(k);
            PyValue_CLOSE(value);
        }
        inst(OP_FIELD_ADDRESS, (value -- res)) {
            char *addr = (char *)PyValue_AsKnownObjectBorrow(value) + 8;
            Py_ssize_t index = addr - (char *)SPAM_BASE();
            res = SPAM(*(PyObject **)addr, index);
            PyValue_CLOSE(value);
        }
    """
        formats = self.formats(input)
        for name in ("OP_DEREF_STORE", "OP_COPY_STORE", "OP_CHAINED",
                     "OP_ARITHMETIC"):
            with self.subTest(name=name):
                self.assertEqual(formats[name].borrow_mask, 0)
        # A field read through an address inside the object, and the
        # difference of two addresses, are reads
        self.assertEqual(formats["OP_FIELD_ADDRESS"].borrow_mask, 1)

    def test_object_readers_list(self):
        # Each name on the reader list receives the object of a view in
        # bytecodes.c, so the list holds no stale entry.  That a name only
        # reads its argument is for review.
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        analysis = analyzer.analyze_files([str(source)])
        with mock.patch.object(ir_generator, "OBJECT_READERS_SEEN", set()):
            ir_generator.compute_formats(analysis)
            seen = ir_generator.OBJECT_READERS_SEEN
        self.assertEqual(seen | {"SPAM", "spam_call"},
                         ir_generator.OBJECT_READERS)

    def test_frame_switch_needs_window(self):
        # The outputs of an instruction that switches frames land in the
        # other frame, where the fields name no slots.
        input = """
        inst(OP, (value -- res)) {
            frame = SPAM(value);
            res = PyValue_NULL;
            INPUTS_DEAD();
        }
    """
        with self.assertRaisesRegex(SyntaxError, "OP switches frames"):
            self.formats(input)

    def test_feedback_mapping(self):
        # A family shares one format. A name index takes the free field of
        # the word, and cache effects read the entry.  The sites that get no
        # entry share entry 0 and run the same handlers.
        input = """
        specializing op(_SPECIALIZE_OP, (counter/1, owner -- owner)) {
            _PyFbHeader *feedback = FB_COUNTER();
            ADVANCE_ADAPTIVE_COUNTER(feedback);
        }
        op(_OP, (owner -- res)) {
            PyObject *o = PyValue_AsKnownObjectBorrow(owner);
            res = SPAM(o, GETITEM(FRAME_CO_NAMES, oparg));
            DECREF_INPUTS();
        }
        macro(OP) = _SPECIALIZE_OP + _OP;
        op(_GUARD, (type_version/2, owner -- owner)) {
            EXIT_IF(Py_TYPE(PyValue_AsKnownObjectBorrow(owner))->tp_version_tag != type_version);
        }
        macro(OP_SPECIALIZED) = unused/1 + _GUARD + _OP;
        family(OP, INLINE_CACHE_ENTRIES_OP) = { OP_SPECIALIZED };
    """
        output = """
        TARGET(OP) {
            int opcode = OP;
            (void)(opcode);
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyIRWord* const this_instr = next_instr - 1;
            (void)this_instr;
            PyObject *value_boxes[6];
            _PyValueTemps value_owners;
            value_owners = (_PyValueTemps){value_boxes, 0, 6};
            _PyValueTemps *value_temps = &value_owners;
            PyValue ir_displaced = PyValue_NULL;
            (void)ir_displaced;
            _PyFb_OP *fb = (_PyFb_OP *)IR_FB(IR_C(ir_word));
            int oparg = (int)IR_D(ir_word);
            (void)oparg;
            PyValueBorrow owner;
            PyValue res;
            PyValue ir_own_B = PyValue_NULL;
            (void)ir_own_B;
            PyObject *ir_obj_owner = NULL;
            // _SPECIALIZE_OP
            {
                uint32_t counter = fb->counter;
                (void)counter;
                _PyFbHeader *feedback = ((_PyFbHeader *)fb);
                ADVANCE_ADAPTIVE_COUNTER(feedback);
            }
            // _OP
            {
                owner = IR_VIEW(IR_B(ir_word));
                if (Py_UNLIKELY(PyValue_IsInline(owner))) {
                    ir_obj_owner = _PyValueTemps_Box(value_temps, owner);
                    if (ir_obj_owner == NULL) {
                        PROCESS_ERROR_AT(RAISE, 0, this_instr);
                    }
                    BOX_STAT_INC(OP);
                }
                else {
                    ir_obj_owner = PyValue_AsKnownObjectBorrow(owner);
                }
                PyObject *o = ir_obj_owner;
                res = SPAM(o, GETITEM(FRAME_CO_NAMES, oparg));
                ir_own_B = IR_CONSUME_INPUT(IR_B(ir_word), owner);
                PyValue_XCLOSE(ir_own_B);
            }
            IR_STORE_DST(IR_A(ir_word), res);
            if (!PyValue_IsNull(ir_displaced) &&
                finish_displaced_local(tstate, frame, next_instr, value_temps, ir_displaced) < 0) {
                next_instr = frame->instr_ptr + 1;
                PROCESS_ERROR_AT(RAISE, 0, next_instr - 1);
            }
            DISPATCH();
        }

        TARGET(OP_SPECIALIZED) {
            int opcode = OP_SPECIALIZED;
            (void)(opcode);
            _PyValueTemps *const value_temps = NULL;
            (void)value_temps;
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP_SPECIALIZED);
            PyValue ir_displaced = PyValue_NULL;
            (void)ir_displaced;
            _PyFb_OP *fb = (_PyFb_OP *)IR_FB(IR_C(ir_word));
            int oparg = (int)IR_D(ir_word);
            (void)oparg;
            PyValueBorrow owner;
            PyValue res;
            PyValue ir_own_B = PyValue_NULL;
            (void)ir_own_B;
            // _GUARD
            {
                owner = IR_VIEW(IR_B(ir_word));
                uint32_t type_version = fb->type_version;
                if (Py_UNLIKELY(!PyValue_IsPointerSpace(owner))) {
                    UPDATE_MISS_STATS(OP);
                    assert(_PyOpcode_Deopt[opcode] == (OP));
                    JUMP_TO_PREDICTED(OP);
                }
                if (Py_TYPE(PyValue_AsKnownObjectBorrow(owner))->tp_version_tag != type_version) {
                    UPDATE_MISS_STATS(OP);
                    assert(_PyOpcode_Deopt[opcode] == (OP));
                    JUMP_TO_PREDICTED(OP);
                }
            }
            // _OP
            {
                PyObject *o = PyValue_AsKnownObjectBorrow(owner);
                res = SPAM(o, GETITEM(FRAME_CO_NAMES, oparg));
                ir_own_B = IR_CONSUME_INPUT(IR_B(ir_word), owner);
                _PyValue_XCLOSE_POINTER(ir_own_B);
            }
            IR_STORE_DST(IR_A(ir_word), res);
            if (!PyValue_IsNull(ir_displaced) &&
                finish_displaced_local(tstate, frame, next_instr, NULL, ir_displaced) < 0) {
                next_instr = frame->instr_ptr + 1;
                PROCESS_ERROR_AT(RAISE, 0, next_instr - 1);
            }
            DISPATCH();
        }


    """
        self.run_ir_test(input, output)
        fmt = self.formats(input)["OP_SPECIALIZED"]
        self.assertEqual(fmt.group, "OP")
        self.assertEqual(fmt.fb, ir_generator.LOC_C)
        self.assertEqual(fmt.imm, ir_generator.LOC_D)

    def test_oparg_in_entry(self):
        # Two inputs, a destination and an entry fill the word, so the oparg
        # lives in the entry.
        input = """
        inst(OP, (counter/1, left, right -- res)) {
            _PyFbHeader *feedback = FB_COUNTER();
            ADVANCE_ADAPTIVE_COUNTER(feedback);
            res = SPAM(left, right, oparg);
            INPUTS_DEAD();
        }
    """
        output = """
        TARGET(OP) {
            int opcode = OP;
            (void)(opcode);
            _PyValueTemps *const value_temps = NULL;
            (void)value_temps;
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            PyValue ir_displaced = PyValue_NULL;
            (void)ir_displaced;
            _PyFb_OP *fb = (_PyFb_OP *)IR_FB(IR_D(ir_word));
            int oparg = IR_FB_ARG(fb, IR_D(ir_word));
            (void)oparg;
            PyValue left;
            PyValue right;
            PyValue res;
            right = IR_SLOT(IR_C(ir_word));
            left = IR_SLOT(IR_B(ir_word));
            uint32_t counter = fb->counter;
            (void)counter;
            _PyFbHeader *feedback = ((_PyFbHeader *)fb);
            ADVANCE_ADAPTIVE_COUNTER(feedback);
            res = SPAM(left, right, oparg);
            IR_SLOT(IR_B(ir_word)) = PyValue_NULL;
            IR_SLOT(IR_C(ir_word)) = PyValue_NULL;
            IR_STORE_DST(IR_A(ir_word), res);
            if (!PyValue_IsNull(ir_displaced) &&
                finish_displaced_local(tstate, frame, next_instr, NULL, ir_displaced) < 0) {
                next_instr = frame->instr_ptr + 1;
                PROCESS_ERROR_AT(RAISE, 0, next_instr - 1);
            }
            DISPATCH();
        }
    """
        self.run_ir_test(input, output)
        fmt = self.formats(input)["OP"]
        self.assertEqual(fmt.roles[3], ir_generator.ROLE_FB)
        self.assertEqual((fmt.imm, fmt.fb),
                         (ir_generator.LOC_FBARG, ir_generator.LOC_D))

    def test_jump(self):
        # A jump keeps the absolute word index of its target in CD, and
        # FOR_ITER the distance to its exit in D.
        input = """
        inst(OP, (cond --)) {
            int flag = PyValue_IsFalse(cond);
            DEAD(cond);
            if (flag) {
                JUMPTO(oparg);
            }
        }
        inst(FOR_ITER, (iter -- iter, next)) {
            next = SPAM(iter);
            if (PyValue_IsNull(next)) {
                JUMPBY(oparg + 1);
                DISPATCH();
            }
        }
    """
        formats = self.formats(input)
        fmt = formats["OP"]
        self.assertTrue(fmt.jumps)
        self.assertEqual(fmt.roles, [f"{ir_generator.ROLE_IN0} + 0",
                                     ir_generator.ROLE_NONE,
                                     ir_generator.ROLE_IMM,
                                     ir_generator.ROLE_IMM])
        self.assertEqual(fmt.imm, ir_generator.LOC_CD)
        fmt = formats["FOR_ITER"]
        self.assertFalse(fmt.jumps)
        self.assertTrue(fmt.forward)
        self.assertEqual(fmt.imm, ir_generator.LOC_D)

    def test_transparent(self):
        # A handler without stack effect, oparg or entry that dispatches
        # again runs another handler on the same word: it is transparent.
        input = """
        ir_only inst(IR_PASS, ( -- )) {
            PRE_DISPATCH_GOTO();
            DISPATCH_GOTO();
        }
    """
        output = """
        TARGET(IR_PASS) {
            int opcode = IR_PASS;
            (void)(opcode);
            _PyValueTemps *const value_temps = NULL;
            (void)value_temps;
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(IR_PASS);
            PRE_DISPATCH_GOTO();
            DISPATCH_GOTO();
        }
    """
        self.run_ir_test(input, output)
        self.assertTrue(self.formats(input)["IR_PASS"].transparent)

    def test_metadata(self):
        # IR-only handlers follow the opcodes in name order. An instrumented
        # twin shares the format of its instruction, and since the twin calls
        # a monitoring callback, that format runs code (runs_code()).
        input = """
        inst(LOAD, ( -- value)) {
            value = PyValue_DUP(GETLOCAL(oparg));
        }
        inst(INSTRUMENTED_LOAD, ( -- value)) {
            value = PyValue_DUP(GETLOCAL(oparg));
        }
        ir_only inst(IR_BACON, ( -- )) {
            SPAM();
        }
        ir_only inst(IR_PASS, ( -- )) {
            PRE_DISPATCH_GOTO();
            DISPATCH_GOTO();
        }
    """
        analysis = self.analyze(input)
        out = io.StringIO()
        ir_metadata_generator.generate_ir_metadata(
            [self.temp_input_filename], analysis, out)
        lines = [line.strip() for line in out.getvalue().splitlines()]
        # The shared handler bound is emitted in opcode_ids.h, not metadata.
        self.assertEqual(ir_generator.num_handlers(analysis), 258)
        # The oparg names the local that LOAD reads (_PyIR_ROLE_LOCAL_IN).
        # Only the twin runs Python itself; the format does, as the twin's.
        load = ("{ .flags = _PyIR_VALID | _PyIR_STORE_DST | _PyIR_RUNS_CODE | _PyIR_DISCARDS, "
                ".effects = 0, "
                ".roles = { _PyIR_ROLE_DST, "
                "_PyIR_ROLE_LOCAL_IN, _PyIR_ROLE_NONE, _PyIR_ROLE_NONE }, "
                ".imm = _PyIR_LOC_B, .fb = _PyIR_LOC_NONE, .ninputs = 0, "
                ".dst = 0, .borrow_mask = 0x0, .fb_kind = 0, .fb_size = 0, "
                ".result_phase = _PyIR_RESULT_PHASE_NONE, "
                ".output_kind = _PyIR_OUTPUT_REGISTER_DST, "
                ".transfer_kind = _PyIR_TRANSFER_UNKNOWN, "
                ".owner_program = 0, .owner_events = 0, .call_input_holes = 0, "
                ".observer_outcome = { _PyIR_OBSERVER_NONE, _PyIR_OBSERVER_NONE }, "
                ".reads = { 0, _PyIR_EXT_FIXED }, "
                ".writes = { 1, _PyIR_EXT_FIXED }, "
                ".through = { 0, _PyIR_EXT_FIXED }, .shadow = LOAD, "
                ".twin = INSTRUMENTED_LOAD, .events = 0 }")
        instrumented = (load.replace(".effects = 0", ".effects = _PyIR_RUNS_PYTHON")
                            .replace("shadow = LOAD", "shadow = INSTRUMENTED_LOAD"))
        for expected in [
            "#define IR_BACON                                       256",
            "#define IR_PASS                                        257",
            f"[LOAD] = {load},",
            f"[INSTRUMENTED_LOAD] = {instrumented},",
            "[IR_PASS] = { .flags = _PyIR_VALID | _PyIR_TRANSPARENT, "
            ".effects = 0, "
            ".roles = { _PyIR_ROLE_NONE, _PyIR_ROLE_NONE, _PyIR_ROLE_NONE, "
            "_PyIR_ROLE_NONE }, .imm = _PyIR_LOC_NONE, .fb = _PyIR_LOC_NONE, "
            ".ninputs = 0, .dst = _PyIR_NO_DST, .borrow_mask = 0x0, "
            ".fb_kind = 0, .fb_size = 0, "
            ".result_phase = _PyIR_RESULT_PHASE_NONE, "
            ".output_kind = _PyIR_OUTPUT_REGISTER_DST, "
            ".transfer_kind = _PyIR_TRANSFER_UNKNOWN, "
            ".owner_program = 0, .owner_events = 0, .call_input_holes = 0, "
            ".observer_outcome = { _PyIR_OBSERVER_NONE, _PyIR_OBSERVER_NONE }, "
            ".reads = { 0, _PyIR_EXT_FIXED }, "
            ".writes = { 0, _PyIR_EXT_FIXED }, "
            ".through = { 0, _PyIR_EXT_FIXED }, .shadow = IR_PASS, "
            ".twin = 0, .events = 0 },",
            '[IR_BACON - IR_FIRST_ONLY_HANDLER] = "IR_BACON",',
            '[IR_PASS - IR_FIRST_ONLY_HANDLER] = "IR_PASS",',
        ]:
            self.assertIn(expected, lines)

    def test_normal_transfer_source_proofs(self):
        cases = (
            ("LOAD_CONST", "-- value",
             "value = PyValue_XDUP(PyTuple_GET_VALUE(FRAME_CO_CONSTS, oparg));",
             "FRESH", "PyValue_XDUP", "PyValue_Borrow"),
            ("LOAD_FAST_AND_CLEAR", "-- value",
             "value = GETLOCAL(oparg); GETLOCAL(oparg) = PyValue_NULL;",
             "LOCAL_MOVE", "GETLOCAL(oparg) = PyValue_NULL;", ""),
            ("POP_TOP", "value --", "PyValue_XCLOSE(value);",
             "RETIRE", "PyValue_XCLOSE", "PyValue_DUP"),
            ("COPY", "bottom, unused[oparg-1] -- bottom, unused[oparg-1], top",
             "top = PyValue_DUP(bottom);", "COPY", "PyValue_DUP(bottom)", "bottom"),
            ("SWAP", "bottom, unused[oparg-2], top -- bottom, unused[oparg-2], top",
             "PyValue temp = bottom; bottom = top; top = temp;",
             "SWAP", "top = temp;", ""),
        )
        for name, effect, body, kind, old, new in cases:
            with self.subTest(handler=name):
                def kind_for(body):
                    analysis = self.analyze(f"\ninst({name}, ({effect})) {{ {body} }}\n")
                    inst = analysis.instructions[name]
                    fmt = ir_generator.compute_formats(analysis)[name]
                    return ir_generator.normal_transfer_kind(inst, fmt)
                self.assertEqual(kind_for(body), "_PyIR_TRANSFER_" + kind)
                self.assertIn(old, body)
                self.assertEqual(kind_for(body.replace(old, new)),
                                 "_PyIR_TRANSFER_UNKNOWN")

    def test_binary_events_follow_actual_owner_commits(self):
        source = "\ninst(OP, (left, right -- result)) { IR_BINARY_READER(left, right, result); }\n"
        analysis = self.analyze(source)
        formats = ir_generator.compute_formats(analysis)
        contract = ir_generator.handler_contracts(analysis, formats)["OP"]
        events = contract.ownership_events
        self.assertEqual([(e.kind.removeprefix("_PyIR_OWNER_"), e.src)
                          for e in events], [
            ("READ", 0), ("TAKE", 0), ("READ", 1), ("TAKE", 1),
            ("PHASE", 5), ("ERROR_BRANCH", 0), ("FRESH", 0),
            ("PHASE", 6), ("NORMAL_EXIT", 0), ("RESTORE", 0),
            ("RESTORE", 1), ("PHASE", 7), ("ERROR_EXIT", 0)])
        self.assertEqual(events[5].dst, 9)
        self.assertEqual(formats["OP"].borrow_mask, 3)
        output = self.handler(self.ir_output(source), "OP")
        take_left = output.index("PyValue ir_binary_left = IR_TAKE")
        take_right = output.index("PyValue ir_binary_right = IR_TAKE")
        provider_phase = output.index("frame->phase_state = _PyIR_PHASE_PUBLISHED | _PyIR_PHASE_BINARY_PROVIDER")
        provider = output.index("_PyValue_BinaryOps[oparg]")
        revalidate = output.index("int ir_binary_phase_valid")
        self.assertLess(take_left, take_right)
        self.assertLess(take_right, provider_phase)
        self.assertLess(provider_phase, provider)
        self.assertLess(provider, revalidate)
        self.assertIn("frame->instr_ptr == this_instr", output)
        self.assertIn("frame->phase_state & ~_PyIR_PHASE_STATUS_MASK", output)
        self.assertIn("ir_binary_phase_valid ? _PyIR_PHASE_PUBLISHED | _PyIR_PHASE_BINARY_RETIRE : 0", output)
        self.assertIn("ir_binary_phase_valid ? _PyIR_PHASE_PUBLISHED | _PyIR_PHASE_BINARY_ERROR_RESTORE : 0", output)
        # Opaque body edits cannot retain this positive recipe merely because
        # the marker, name and IO are still present.
        for body, reason in (
                ("opaque(frame); IR_BINARY_READER(left, right, result);",
                 "view 'left' is read after a call.*\\(S7\\)"),
                ("IR_BINARY_READER(right, left, result);", "checked binary IO contract"),
                ("if (oparg) { IR_BINARY_READER(left, right, result); }",
                 "checked binary IO contract")):
            with self.subTest(body=body):
                with self.assertRaisesRegex(SyntaxError, reason):
                    self.ir_output("\ninst(OP, (left, right -- result)) { " + body + " }\n")

    def test_binary_recipe_requires_complete_real_specializer(self):
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        analysis = analyzer.analyze_files([str(source)])
        inst = analysis.instructions["BINARY_OP"]
        fmt = ir_generator.compute_formats(analysis)["BINARY_OP"]
        self.assertTrue(ir_generator.binary_reader_contract(inst, fmt))
        prefix = [part for part in inst.parts if isinstance(part, analyzer.Uop)][0]
        extra = self.analyze("\ninst(OP, (--)) { *slots = PyValue_NULL; }\n")
        prefix.body.body.append(extra.instructions["OP"].parts[0].body.body[0])
        try:
            self.assertFalse(ir_generator.binary_reader_contract(inst, fmt))
        finally:
            prefix.body.body.pop()

    def test_transparent_leaf_still_has_a_consumer_recipe(self):
        source = "\nir_only inst(IR_JUMP, (--)) { JUMPTO(oparg); }\n"
        analysis = self.analyze(source)
        fmt = ir_generator.compute_formats(analysis)
        contract = ir_generator.handler_contracts(analysis, fmt)["IR_JUMP"]
        self.assertEqual(contract.ownership_events, (
            ir_generator.OwnershipEvent("_PyIR_OWNER_LEAF", src="_PyIR_TRANSFER_PRESERVE"),))

    def test_normal_transfer_requires_matching_io(self):
        # The same body with a different window cannot acquire COPY geometry.
        source = "\ninst(COPY, (bottom -- bottom, top)) { top = PyValue_DUP(bottom); }\n"
        analysis = self.analyze(source)
        inst = analysis.instructions["COPY"]
        fmt = ir_generator.compute_formats(analysis)["COPY"]
        self.assertEqual(ir_generator.normal_transfer_kind(inst, fmt),
                         "_PyIR_TRANSFER_UNKNOWN")

    def test_resume_normal_transfer_checks_helper_footprint(self):
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        analysis = analyzer.analyze_files([str(source)])
        inst = analysis.instructions["RESUME"]
        fmt = ir_generator.compute_formats(analysis)["RESUME"]
        self.assertEqual(ir_generator.normal_transfer_kind(inst, fmt),
                         "_PyIR_TRANSFER_PRESERVE")
        body = [part for part in inst.parts if isinstance(part, analyzer.Uop)][-1].body
        for statement in ("opaque(frame);", "*slots = PyValue_NULL;",
                          "*(slots + offset) = PyValue_NULL;",
                          "frame->base[0] = PyValue_NULL;"):
            with self.subTest(statement=statement):
                changed = self.analyze(f"\ninst(OP, (--)) {{ {statement} }}\n")
                body.body.append(changed.instructions["OP"].parts[0].body.body[0])
                try:
                    self.assertEqual(ir_generator.normal_transfer_kind(inst, fmt),
                                     "_PyIR_TRANSFER_UNKNOWN")
                finally:
                    body.body.pop()

        # Flattening both preprocessor arms loses their boundary: the same
        # statements in a different arm must not satisfy the source proof.
        instrument = [part for part in inst.parts
                      if isinstance(part, analyzer.Uop)][0].body
        branch = instrument.body[0]
        original = branch.body.pop()
        branch.else_body.insert(0, original)
        try:
            self.assertEqual(ir_generator.normal_transfer_kind(inst, fmt),
                             "_PyIR_TRANSFER_UNKNOWN")
        finally:
            branch.body.append(branch.else_body.pop(0))

    def test_abrupt_prefix_requires_complete_suspension_source(self):
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        analysis = analyzer.analyze_files([str(source)])
        formats = ir_generator.compute_formats(analysis)
        names = ("YIELD_VALUE", "INSTRUMENTED_YIELD_VALUE")
        contracts = ir_generator.handler_contracts(analysis, formats)
        for name in names:
            self.assertTrue(contracts[name].abrupt_prefix)
        inst = analysis.instructions["YIELD_VALUE"]
        producer = inst.parts[-1]
        original = producer.body.body
        for changed in (
                "{ opaque(frame); }",
                "{ gen_frame->return_slot = 0; }",
                "{ frame->base[0] = PyValue_NULL; }"):
            extra = self.analyze("\ninst(OP, (--)) " + changed + "\n")
            producer.body.body = original + extra.instructions["OP"].parts[0].body.body
            try:
                self.assertFalse(ir_generator.abrupt_suspend_contract(inst, formats["YIELD_VALUE"]))
            finally:
                producer.body.body = original
        # The twin's observer ingress must also be covered; a missing
        # sibling revokes the canonical family's permission.
        observer = analysis.instructions["INSTRUMENTED_YIELD_VALUE"].parts[0]
        old = observer.body.body
        extra = self.analyze("\ninst(OP, (--)) { opaque(frame); }\n")
        observer.body.body = old + extra.instructions["OP"].parts[0].body.body
        try:
            changed = ir_generator.handler_contracts(analysis, formats)
            for name in names:
                self.assertFalse(changed[name].abrupt_prefix)
        finally:
            observer.body.body = old

    def test_normal_vacate_observer_frame_footprint(self):
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        analysis = analyzer.analyze_files([str(source)])
        formats = ir_generator.compute_formats(analysis)
        for name in ("LOAD_SUPER_ATTR", "INSTRUMENTED_LOAD_SUPER_ATTR",
                     "LOAD_SUPER_ATTR_ATTR", "LOAD_SUPER_ATTR_METHOD"):
            with self.subTest(handler=name):
                self.assertTrue(ir_generator.normal_vacate_capability(
                    analysis.instructions[name], formats[name]))

        # The capability explains the helper/frame use, not an opcode name.
        call = ("_Py_call_instrumentation_2args(tstate, "
                "PY_MONITORING_EVENT_CALL, frame, this_instr, left, right);")
        declaration = "\ninst(OP, (left, right --)) "
        analysis = self.analyze(declaration + "{ " + call + " DECREF_INPUTS(); }\n")
        inst = analysis.instructions["OP"]
        fmt = ir_generator.compute_formats(analysis)["OP"]
        self.assertTrue(ir_generator.normal_vacate_capability(inst, fmt))
        changes = (
            call.replace("tstate,", "other,"),
            call.replace("EVENT_CALL", "EVENT_LINE"),
            call.replace("frame, this_instr", "other_frame, this_instr"),
            call.replace("frame, this_instr", "this_instr, frame"),
            call.replace("this_instr", "next_instr"),
            call.replace("left, right", "frame, right"),
            call.replace("left, right", "left, right, frame"),
            call.replace("_2args", "_unknown"),
            "void *alias = frame; " + call,
            "frame->base[0] = PyValue_NULL; " + call,
            "opaque(frame); " + call,
            "#if ENABLE_FOOTPRINT\n" + call + "\n#endif\n",
        )
        for changed in changes:
            with self.subTest(changed=changed):
                modified = self.analyze(declaration + "{\n" + changed
                                        + "\nDECREF_INPUTS(); }\n")
                self.assertFalse(ir_generator.normal_vacate_capability(
                    modified.instructions["OP"], fmt))
        # An AST-only arm change must also fail against the original source.
        part = inst.parts[0]
        wrapped = self.analyze(declaration + "{\n#if ENABLE_FOOTPRINT\n"
                               + call + "\n#endif\nDECREF_INPUTS(); }\n")
        original = part.body.body
        part.body.body = wrapped.instructions["OP"].parts[0].body.body
        try:
            self.assertFalse(ir_generator.normal_vacate_capability(inst, fmt))
        finally:
            part.body.body = original

    def test_home_helpers_keep_ordinary_escape_publication(self):
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        analysis = analyzer.analyze_files([str(source)])
        expected = {"BUILD_STRING": "IR_BUILD_STRING", "BUILD_MAP": "IR_BUILD_MAP",
                    "_LOAD_SPECIAL": "IR_LOOKUP_SPECIAL"}
        for name, helper in expected.items():
            part = analysis.uops[name]
            self.assertTrue(ir_generator.home_operation_contract(part))
            self.assertNotIn(helper, analyzer.IR_MARKER_EFFECTS)
            self.assertIn(helper, {escape.call.text
                                  for escape in part.properties.escaping_calls.values()})
        for name in ("_LOAD_GLOBAL", "_PUSH_NULL_CONDITIONAL", "_INSERT_NULL"):
            self.assertTrue(ir_generator.home_operation_contract(analysis.uops[name]))
        for name, marker in (("BUILD_TUPLE", "IR_BUILD_TUPLE"),
                             ("BUILD_LIST", "IR_BUILD_LIST")):
            part = analysis.uops[name]
            self.assertTrue(ir_generator.home_operation_contract(part))
            self.assertNotIn(marker, analyzer.IR_MARKER_EFFECTS)
            escapes = {escape.call.text for escape in part.properties.escaping_calls.values()}
            self.assertEqual(marker in escapes,
                             analyzer.IR_HELPER_CALLEES[marker] not in analyzer.NON_ESCAPING_FUNCTIONS)

    def test_input_drain_requires_its_sole_owning_array(self):
        def check(source):
            analysis = self.analyze("\n" + source + "\n")
            return ir_generator.home_operation_contract(analysis.instructions["OP"].parts[0])

        self.assertTrue(check("inst(OP, (items[oparg] --)) { IR_CLOSE_INPUTS(items); }"))
        for source in (
            "inst(OP, (items[oparg*2] --)) { IR_CLOSE_INPUTS(items); }",
            "inst(OP, (items[oparg] -- result)) { IR_CLOSE_INPUTS(items); result = PyValue_None; }",
            "inst(OP, (items[oparg] -- items[oparg])) { IR_CLOSE_INPUTS(items); }",
            "inst(OP, (items[oparg] --)) { if (oparg) { IR_CLOSE_INPUTS(items); } }",
            "inst(OP, (items[oparg] --)) { SPAM(); IR_CLOSE_INPUTS(items); }",
            "inst(OP, (items[oparg] --)) { IR_CLOSE_INPUTS(items); DEAD(items); }",
            "inst(OP, (items[oparg] --)) { PyValue *alias = items; IR_CLOSE_INPUTS(alias); }",
        ):
            with self.subTest(source=source):
                with self.assertRaises(SyntaxError):
                    check(source)
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        analysis = analyzer.analyze_files([str(source)])
        part = analysis.uops["_POP_TOP_OPARG"]
        self.assertTrue(ir_generator.home_operation_contract(part))
        self.assertNotIn("IR_CLOSE_INPUTS", analyzer.IR_MARKER_EFFECTS)
        self.assertIn("IR_CLOSE_INPUTS", {escape.call.text
                                        for escape in part.properties.escaping_calls.values()})
        self.assertTrue(ir_generator.releases_object(
            analysis.instructions["CALL_BUILTIN_FAST_WITH_KEYWORDS"]))

    def test_input_drain_lowers_through_actual_storage(self):
        output = self.ir_output("""
            inst(OP, (items[oparg] --)) {
                IR_CLOSE_INPUTS(items);
            }
        """)
        self.assertIn("_PyFrame_CloseHomeRange(frame, items, oparg)", output)
        self.assertIn("frame->phase_state = 0;", output)
        self.assertNotIn("IR_CLOSE_INPUTS", output)
        self.assertNotIn("_PyValue_CloseStack", output)

    def test_home_arrays_reject_unproved_bounds_and_pointer_escapes(self):
        def check(source):
            analysis = self.analyze("\n" + source + "\n")
            return ir_generator.home_operation_contract(analysis.instructions["OP"].parts[0])
        self.assertTrue(check("""inst(OP, (-- out[2])) {
            IR_WRITE_OUTPUT(out, 1, PyValue_NULL);
            int vacant = PyValue_IsNull(out[1]);
        }"""))
        self.assertTrue(check("""inst(OP, (-- out[oparg & 1])) {
            if (oparg & 1) { IR_WRITE_OUTPUT(out, 0, PyValue_NULL); }
        }"""))
        for source in (
                "inst(OP, (-- out[2])) { IR_WRITE_OUTPUT(out, 2, PyValue_NULL); }",
                "inst(OP, (out[2] --)) { IR_WRITE_OUTPUT(out, 0, PyValue_NULL); }",
                "inst(OP, (-- out[oparg & 1])) { IR_WRITE_OUTPUT(out, 0, PyValue_NULL); }",
                "inst(OP, (-- out[oparg & 1])) { if (!(oparg & 1)) { IR_WRITE_OUTPUT(out, 0, PyValue_NULL); } }",
                "inst(OP, (-- out[oparg & 1])) { oparg = 1; if (oparg & 1) { IR_WRITE_OUTPUT(out, 0, PyValue_NULL); } }",
                "inst(OP, (-- out[2])) { IR_WRITE_OUTPUT(out, 0, PyValue_NULL); opaque(out); }",
                "inst(OP, (-- out[2])) { IR_WRITE_OUTPUT(out, 0, PyValue_NULL); PyValue *alias = out; }",
                "inst(OP, (-- out[2])) { IR_WRITE_OUTPUT(out, 0, PyValue_NULL); opaque(&out[0]); }",
                "inst(OP, (-- out[2])) { IR_WRITE_OUTPUT(out, 0, PyValue_NULL); PyValue *alias = &(out[0]); }",
                "inst(OP, (pair[2] -- pair[2])) { int rc = IR_LOOKUP_SPECIAL(name, pair); PyValue *alias = &((pair[0])); }",
                "inst(OP, (-- out[2])) { IR_WRITE_OUTPUT(out, 0, PyValue_NULL); opaque(&(oparg)); }",
                "inst(OP, (-- out[2])) { IR_WRITE_OUTPUT(out, 0, PyValue_NULL); (oparg) = 1; }",
                "inst(OP, (-- out[2])) { IR_WRITE_OUTPUT(out, 0, PyValue_NULL); int oparg; }",
                "inst(OP, (-- out[2])) { IR_WRITE_OUTPUT(out, 0, PyValue_NULL); out[0] = PyValue_NULL; }",
                "inst(OP, (-- out[2])) { IR_WRITE_OUTPUT(out, 0, PyValue_NULL); (out[0]) = PyValue_NULL; }",
                "inst(OP, (-- out[2])) { IR_WRITE_OUTPUT(out, 0, PyValue_NULL); out[0].bits = 0; }",
                "inst(OP, (-- out[2])) { opaque(out[0]); IR_WRITE_OUTPUT(out, 0, PyValue_NULL); }",
                "inst(OP, (-- out[2])) { if (oparg) { IR_WRITE_OUTPUT(out, 0, PyValue_NULL); } opaque(out[0]); }",
                "inst(OP, (-- out[2])) { if (flag) { IR_WRITE_OUTPUT(out, 0, PyValue_NULL); } flag = 1; if (flag) { int vacant = PyValue_IsNull(out[0]); } }",
                "inst(OP, (-- out[2])) { if (flag) {} else { IR_WRITE_OUTPUT(out, 0, PyValue_NULL); } int vacant = PyValue_IsNull(out[0]); }",
                "inst(OP, (values[2] -- result)) { result = IR_BUILD_MAP(values); }",
                "inst(OP, (pair[3] -- pair[3])) { int rc = IR_LOOKUP_SPECIAL(name, pair); }",
                "inst(OP, (pair[2] -- pair[2])) { int rc = IR_LOOKUP_SPECIAL(frame, name, pair); }",
                "inst(OP, (-- out[2])) {\n#if FEATURE\nIR_WRITE_OUTPUT(out, 0, PyValue_NULL);\n#endif\n}"):
            with self.subTest(source=source), self.assertRaises(SyntaxError):
                check(source)

    def test_home_loops_require_unique_all_path_induction(self):
        def check(body, inputs="", outputs="out[oparg]"):
            analysis = self.analyze(f"\ninst(OP, ({inputs} -- {outputs})) {{ {body} }}\n")
            return ir_generator.home_operation_contract(analysis.instructions["OP"].parts[0])
        loop = "for (int i = 0; i < oparg; i++) { IR_WRITE_OUTPUT(out, i, PyValue_NULL); }"
        self.assertTrue(check(loop))
        self.assertTrue(check(loop.replace("i++", "++i")))
        self.assertTrue(check("""for (Py_ssize_t i = 0; i < oparg; i++) {
            PyValue value = IR_TAKE_INPUT(values, i);
            PyValue_CLOSE(value);
        } DEAD(values);""", inputs="values[oparg]", outputs=""))
        for changed in (
                loop.replace("i = 0", "i = 1"),
                loop.replace("i < oparg", "i <= oparg"),
                loop.replace("i < oparg", "i < oparg + 1"),
                loop.replace("i++", "i += 2"),
                loop.replace(" i", " frame"),
                loop.replace("out, i", "out, 0"),
                loop.replace("IR_WRITE_OUTPUT", "continue; IR_WRITE_OUTPUT"),
                loop.replace("IR_WRITE_OUTPUT", "SKIP(); IR_WRITE_OUTPUT"),
                loop.replace("IR_WRITE_OUTPUT", "if (flag) IR_WRITE_OUTPUT"),
                loop.replace("IR_WRITE_OUTPUT", "int i = 0; IR_WRITE_OUTPUT"),
                loop.replace("IR_WRITE_OUTPUT", "(i) = 0; IR_WRITE_OUTPUT"),
                loop.replace("IR_WRITE_OUTPUT", "opaque(&(i)); IR_WRITE_OUTPUT"),
                loop.replace(" }", " break; }"),
                loop.replace(" }", " IR_WRITE_OUTPUT(out, i, PyValue_NULL); }"),
                loop + loop):
            with self.subTest(changed=changed), self.assertRaises(SyntaxError):
                check(changed)
        with self.assertRaisesRegex(SyntaxError, "published twice"):
            check("IR_WRITE_OUTPUT(out, 0, PyValue_NULL); IR_WRITE_OUTPUT(out, 0, PyValue_NULL);",
                  outputs="out[2]")
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        analysis = analyzer.analyze_files([str(source)])
        for name in ("_UNPACK_SEQUENCE_TUPLE", "_UNPACK_SEQUENCE_LIST", "BUILD_SET"):
            self.assertTrue(ir_generator.home_operation_contract(analysis.uops[name]))

    def test_native_entry_requires_actual_body_and_owning_scalar_io(self):
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        analysis = analyzer.analyze_files([str(source)])
        inst = analysis.instructions["INTERPRETER_EXIT"]
        self.assertTrue(ir_generator.native_entry_input_contract(inst))
        part = inst.parts[0]
        extra = self.analyze("\ninst(OP, (--)) { opaque(frame); }\n")
        part.body.body.append(extra.instructions["OP"].parts[0].body.body[0])
        try:
            with self.assertRaises(SyntaxError):
                ir_generator.native_entry_input_contract(inst)
        finally:
            part.body.body.pop()
        item = part.stack.inputs[0]
        for attribute, changed in (("name", "other"), ("peek", True), ("size", "1")):
            old = getattr(item, attribute)
            setattr(item, attribute, changed)
            try:
                with self.assertRaises(SyntaxError):
                    ir_generator.native_entry_input_contract(inst)
            finally:
                setattr(item, attribute, old)
        old = inst.name
        inst.name = "OTHER_NATIVE_SHAPE"
        try:
            self.assertFalse(ir_generator.native_entry_input_contract(inst))
        finally:
            inst.name = old

    def test_steal_success_handoff_requires_terminal_ownership_sequence(self):
        body = """{
            PyObject *object = IR_BUILD_TUPLE(values);
            if (object == NULL) { ERROR_NO_POP(); }
            INPUTS_DEAD();
            result = PyValue_FromObjectSteal(object);
        }"""
        def check(changed, inputs="values[oparg]", outputs="result"):
            analysis = self.analyze(f"\ninst(OP, ({inputs} -- {outputs})) {changed}\n")
            inst = analysis.instructions["OP"]
            part = inst.parts[0]
            self.assertTrue(ir_generator.home_operation_contract(part))
            return inst

        self.assertIsNotNone(check(body))
        self.assertIsNotNone(check(body.replace("IR_BUILD_TUPLE", "IR_BUILD_LIST")))
        for changed in (
                body.replace("object == NULL", "object != NULL"),
                body.replace("ERROR_NO_POP();", "ERROR_IF(true);"),
                body.replace("INPUTS_DEAD();", ""),
                body.replace("INPUTS_DEAD();", "opaque(); INPUTS_DEAD();"),
                body.replace("INPUTS_DEAD();", "if (flag) { INPUTS_DEAD(); }"),
                body.replace("INPUTS_DEAD();", "INPUTS_DEAD(); opaque();"),
                body.replace("object);", "other);"),
                body.replace("(values)", "(other)"),
                body.replace("*object", "*values"),
                body.replace("if (object", "if (other"),
                body.replace("result =", "if (flag) result =")):
            with self.subTest(changed=changed), self.assertRaises(SyntaxError):
                check(changed)
        for inputs, outputs in (("values[oparg*2]", "result"),
                                ("extra, values[oparg]", "result"),
                                ("values[oparg]", "result[1]")):
            with self.subTest(inputs=inputs, outputs=outputs), self.assertRaises(SyntaxError):
                check(body, inputs, outputs)
        inst = check(body)
        fmt = ir_generator.compute_formats(self.analyze(
            "\ninst(OP, (values[oparg] -- result)) " + body + "\n"))["OP"]
        self.assertTrue(ir_generator.normal_vacate_capability(inst, fmt))
        extra = self.analyze("\ninst(EXTRA, (--)) { opaque(); }\n")
        inst.parts.append(extra.instructions["EXTRA"].parts[0])
        self.assertFalse(ir_generator.normal_vacate_capability(inst, fmt))

    def test_steal_success_handoff_uses_existing_call_and_final_flush(self):
        for marker, helper in (("IR_BUILD_TUPLE", "_PyTuple_FromValueStealOnSuccess"),
                               ("IR_BUILD_LIST", "_PyList_FromValueStealOnSuccess")):
            source = f"""
                inst(OP, (values[oparg] -- result)) {{
                    PyObject *object = {marker}(values);
                    if (object == NULL) {{ ERROR_NO_POP(); }}
                    INPUTS_DEAD();
                    result = PyValue_FromObjectSteal(object);
                }}
            """
            analysis = self.analyze(source)
            formats = ir_generator.compute_formats(analysis)
            programs = {}
            self.assertNotIn("OP", ir_generator.boxing_handlers(analysis, formats, programs))
            self.assertEqual(programs["OP"][0].kind, "_PyIR_OWNER_NORMAL_CLEAR_INPUTS")
            output = self.ir_output(source)
            self.assertIn(f"{helper}(values, oparg)", output)
            self.assertNotIn(marker, output)
            self.assertNotIn(f"{helper}(frame", output)
            self.assertNotIn("_PyValueTemps value_owners", output)
            call = output.index(f"{helper}(values, oparg)")
            vacate = output.index("IR_CLEAR_SLOTS", call)
            publish = output.index("IR_PUBLISH_TEMP", vacate)
            self.assertLess(call, vacate)
            self.assertLess(vacate, publish)
            self.assertNotIn("finish_", output[call:publish])

    def test_home_operations_lower_through_actual_storage(self):
        output = self.ir_output("""
            window inst(OP, (-- out[2])) {
                IR_WRITE_OUTPUT(out, 0, PyValue_NULL);
                IR_WRITE_OUTPUT(out, 1, PyValue_NULL);
            }
        """)
        self.assertIn("_PyFrame_WriteHome(frame, &out[0], PyValue_NULL)", output)
        self.assertIn("_PyFrame_WriteHome(frame, &out[1], PyValue_NULL)", output)
        self.assertNotIn("IR_WRITE_OUTPUT", output)
        output = self.ir_output("""
            inst(OP, (values[oparg*2] -- map)) {
                PyObject *object = IR_BUILD_MAP(values);
                DEAD(values);
                map = PyValue_FromObjectSteal(object);
            }
        """)
        self.assertIn("_Py_BuildMap_ValueSteal(frame, values, oparg)", output)
        self.assertNotIn("IR_BUILD_MAP", output)
        self.assertIn("frame->phase_state = 0;", output)

    def test_home_loop_operations_lower_through_actual_storage(self):
        output = self.ir_output("""
            window inst(OP, (-- out[oparg])) {
                for (int i = 0; i < oparg; i++) {
                    IR_WRITE_OUTPUT(out, i, PyValue_NULL);
                }
            }
        """)
        self.assertIn("_PyFrame_WriteHome(frame, &out[i], PyValue_NULL)", output)
        output = self.ir_output("""
            inst(OP, (values[oparg] -- result)) {
                for (int i = 0; i < oparg; i++) {
                    PyValue value = IR_TAKE_INPUT(values, i);
                    PyValue_CLOSE(value);
                }
                DEAD(values);
                result = PyValue_None;
            }
        """)
        self.assertIn("_PyFrame_TakeHome(frame, &values[i])", output)
        self.assertNotIn("IR_TAKE_INPUT", output)

    def test_disjoint_child_requires_actual_constructor_and_publication(self):
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        analysis = analyzer.analyze_files([str(source)])
        formats = ir_generator.compute_formats(analysis)
        for name in ("LOAD_ATTR_PROPERTY", "LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN"):
            inst = analysis.instructions[name]
            fmt = formats[name]
            with self.subTest(handler=name):
                self.assertIsNotNone(ir_generator.disjoint_child_transport(inst, fmt))
                parts = [p for p in inst.parts if isinstance(p, analyzer.Uop)]
                for index, statement in (
                        (-3, "frame->base[0] = PyValue_NULL;"),
                        (-3, "pushed_frame->base[3] = PyValue_NULL;"),
                        (-2, "frame->return_slot = 0;"),
                        (-1, "frame->base[0] = PyValue_NULL;")):
                    extra = self.analyze("\ninst(OP, (--)) { " + statement + " }\n")
                    body = parts[index].body.body
                    body.append(extra.instructions["OP"].parts[0].body.body[0])
                    try:
                        self.assertIsNone(ir_generator.disjoint_child_transport(inst, fmt))
                    finally:
                        body.pop()
                producer = parts[-3]
                for statement in producer.body.body:
                    if not isinstance(statement, parser.SimpleStmt):
                        continue
                    for index, token in enumerate(statement.contents):
                        if token.text not in ("_PyFrame_PushUnchecked", "TAKE", "SAVE_STACK"):
                            continue
                        original = token
                        statement.contents[index] = token.replaceText("unknown_child_effect")
                        try:
                            self.assertIsNone(ir_generator.disjoint_child_transport(inst, fmt))
                        finally:
                            statement.contents[index] = original
                # No instruction name is part of the constructor capability.
                original = inst.name
                inst.name = "UNNAMED_DISJOINT_CHILD"
                try:
                    self.assertIsNotNone(ir_generator.disjoint_child_transport(inst, fmt))
                finally:
                    inst.name = original

    def test_disjoint_child_receipt_is_emitted_and_family_closed(self):
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        analysis = analyzer.analyze_files([str(source)])
        formats = ir_generator.compute_formats(analysis)
        contracts = ir_generator.handler_contracts(analysis, formats)
        for name in ("LOAD_ATTR_PROPERTY", "LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN"):
            events = contracts[name].ownership_events
            self.assertEqual(len(events), 1)
            self.assertEqual(events[0].kind, "_PyIR_OWNER_NORMAL_CLEAR_INPUTS")
            self.assertTrue(events[0].flags & 2)
            self.assertFalse(events[0].flags & 1)
        for name, fmt in formats.items():
            if fmt.group == "LOAD_ATTR":
                self.assertTrue(contracts[name].ownership_events, name)
        # An unknown constructor removes the whole family's final-clear grant.
        producer = analysis.uops["_LOAD_ATTR_PROPERTY_FRAME"]
        extra = self.analyze("\ninst(OP, (--)) { opaque(frame); }\n")
        producer.body.body.append(extra.instructions["OP"].parts[0].body.body[0])
        try:
            contracts = ir_generator.handler_contracts(analysis, formats)
            for name in ("LOAD_ATTR", "LOAD_ATTR_PROPERTY"):
                self.assertFalse(contracts[name].ownership_events)
        finally:
            producer.body.body.pop()

    def test_created_copy_recipe_checks_actual_transport(self):
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        analysis = analyzer.analyze_files([str(source)])
        formats = ir_generator.compute_formats(analysis)
        inst = analysis.instructions["RETURN_GENERATOR"]
        self.assertTrue(ir_generator.created_copy_contract(inst, formats[inst.name]))
        body = inst.parts[0].body
        for statement in ("opaque(frame);", "*slots = PyValue_NULL;",
                          "frame->base[0] = PyValue_NULL;"):
            changed = self.analyze(f"\ninst(OP, (--)) {{ {statement} }}\n")
            body.body.insert(0, changed.instructions["OP"].parts[0].body.body[0])
            try:
                self.assertFalse(ir_generator.created_copy_contract(inst, formats[inst.name]))
            finally:
                body.body.pop(0)
        for name in ("MAKE_CELL", "COPY_FREE_VARS"):
            with self.subTest(handler=name):
                local = analysis.instructions[name]
                fmt = formats[name]
                self.assertEqual(ir_generator.normal_transfer_kind(local, fmt),
                                 "_PyIR_TRANSFER_PRESERVE")
                changed = self.analyze("\ninst(OP, (--)) { *slots = PyValue_NULL; }\n")
                local.parts[0].body.body.append(changed.instructions["OP"].parts[0].body.body[0])
                try:
                    self.assertEqual(ir_generator.normal_transfer_kind(local, fmt),
                                     "_PyIR_TRANSFER_UNKNOWN")
                finally:
                    local.parts[0].body.body.pop()

    def test_unused_scalar_passthrough_keeps_distinct_array_geometry(self):
        source = """
        op(FIRST, (callable, unused, unused[oparg] -- callable, unused, unused[oparg])) {
            assert(callable);
        }
        op(SECOND, (callable, self, unused[oparg] -- callable, self, unused[oparg])) {
            assert(callable);
        }
        op(THIRD, (callable, unused, unused[oparg] -- callable, unused, unused[oparg])) {
            assert(callable);
        }
        macro(OP) = FIRST + SECOND + THIRD;
        """
        analysis = self.analyze(source)
        effect = stack.get_stack_effect(analysis.instructions["OP"])
        self.assertEqual((-effect.base_offset).to_c(), "2 + oparg")
        self.assertEqual((effect.logical_sp - effect.base_offset).to_c(), "2 + oparg")

    def test_structured_branch_common_vacate_is_an_actual_both_edge_recipe(self):
        source = """
        inst(OP, (value --)) {
            PyValue_XCLOSE(value);
            if (oparg) { JUMPTO(oparg); }
        }
        """
        analysis = self.analyze(source)
        formats = ir_generator.compute_formats(analysis)
        contract = ir_generator.handler_contracts(analysis, formats)["OP"]
        self.assertEqual(contract.ownership_events, (
            ir_generator.OwnershipEvent("_PyIR_OWNER_NORMAL_CLEAR_INPUTS"),))
        # A raw frame operation before the common point has no footprint
        # contract, even when the declared input/output counts stay identical.
        changed = self.analyze(source.replace("PyValue_XCLOSE(value);",
                                             "opaque(frame); PyValue_XCLOSE(value);"))
        fmt = ir_generator.compute_formats(changed)
        self.assertEqual(ir_generator.handler_contracts(changed, fmt)["OP"].ownership_events, ())
        actual = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        real = analyzer.analyze_files([str(actual)])
        formats = ir_generator.compute_formats(real)
        contracts = ir_generator.handler_contracts(real, formats)
        for name in ("IS_OP_JUMP", "INSTRUMENTED_IS_OP_JUMP",
                     "COMPARE_OP_JUMP", "COMPARE_OP_JUMP_INT",
                     "CONTAINS_OP_JUMP", "DICT_MERGE", "LIST_APPEND",
                     "POP_ITER", "INSTRUMENTED_POP_ITER"):
            with self.subTest(handler=name):
                self.assertEqual(contracts[name].ownership_events, (
                    ir_generator.OwnershipEvent("_PyIR_OWNER_NORMAL_CLEAR_INPUTS"),))

    def test_actual_call_role_families_have_normal_drain_recipes(self):
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        analysis = analyzer.analyze_files([str(source)])
        formats = ir_generator.compute_formats(analysis)
        contracts = ir_generator.handler_contracts(analysis, formats)
        variants = ir_generator.call_role_variants(analysis)
        self.assertEqual(len(variants), 35)
        self.assertEqual({head for _, head, _ in variants.values()},
                         set(ir_generator.CALL_ROLE_FAMILIES))
        for name, (original, _, holes) in variants.items():
            with self.subTest(handler=name):
                self.assertEqual(formats[name], formats[original])
                self.assertEqual(contracts[name].call_input_holes, holes)
                self.assertEqual(contracts[name].ownership_events, (
                    ir_generator.OwnershipEvent("_PyIR_OWNER_CALL_DRAIN"),))
        self.assertEqual(contracts["BUILD_TUPLE"].ownership_events, (
            ir_generator.OwnershipEvent("_PyIR_OWNER_NORMAL_CLEAR_INPUTS", flags=1),))
        body = analysis.instructions["BUILD_TUPLE"].parts[0].body
        changed = self.analyze("\ninst(OP, (--)) { opaque(frame); }\n")
        body.body.insert(0, changed.instructions["OP"].parts[0].body.body[0])
        try:
            changed_contract = ir_generator.handler_contracts(analysis, formats)["BUILD_TUPLE"]
            self.assertEqual(changed_contract.ownership_events, ())
        finally:
            body.body.pop(0)

    def test_loop_completion_recipes_follow_actual_emitted_routes(self):
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        analysis = analyzer.analyze_files([str(source)])
        formats = ir_generator.compute_formats(analysis)
        contracts = ir_generator.handler_contracts(analysis, formats)
        names = [name for name, fmt in formats.items() if fmt.group == "FOR_ITER"]
        self.assertGreaterEqual(len(names), 7)
        expected = ir_generator.OwnershipEvent(
            "_PyIR_OWNER_NORMAL_COMPLETION", src=2, dst=1)
        for name in names:
            with self.subTest(handler=name):
                self.assertIn(expected, contracts[name].ownership_events)
        # A completion at another continuation does not describe the declared
        # taken edge. One uncovered sibling removes the family-wide grant.
        text = source.read_text()
        old = "IR_COMPLETE_STEP(_PY_IR_FOR_ITER, completed, oparg);"
        self.assertIn(old, text)
        with open(self.temp_input_filename, "w") as stream:
            changed_route = "int changed_target = oparg + 1;\n" + old.replace("oparg", "changed_target")
            stream.write(text.replace(old, changed_route, 1))
        changed = analyzer.analyze_files([self.temp_input_filename])
        changed_formats = ir_generator.compute_formats(changed)
        changed_contracts = ir_generator.handler_contracts(changed, changed_formats)
        for name in names:
            with self.subTest(uncovered_handler=name):
                self.assertNotIn(expected, changed_contracts[name].ownership_events)

    def test_normal_transfer_real_family_coverage(self):
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        analysis = analyzer.analyze_files([str(source)])
        formats = ir_generator.compute_formats(analysis)
        insts = ir_generator.handler_instructions(analysis)
        expected = {
            "LOAD_CONST": "FRESH", "LOAD_CONST_INLINE_INT": "FRESH",
            "LOAD_CONST_INLINE_FLOAT": "FRESH", "LOAD_COMMON_CONSTANT": "FRESH",
            "LOAD_SMALL_INT": "FRESH", "LOAD_SMALL_INT_INLINE": "FRESH",
            "LOAD_FAST": "LOCAL_DUP", "LOAD_FAST_BORROW": "LOCAL_DUP",
            "LOAD_FAST_CHECK": "LOCAL_DUP", "LOAD_FAST_AND_CLEAR": "LOCAL_MOVE",
            "STORE_FAST": "LOCAL_STORE", "DELETE_FAST": "LOCAL_CLEAR",
            "POP_TOP": "RETIRE", "COPY": "COPY", "SWAP": "SWAP",
            "POP_JUMP_IF_NONE": "RETIRE", "POP_JUMP_IF_NOT_NONE": "RETIRE",
            "IR_JUMP": "PRESERVE", "RESUME": "PRESERVE", "RETURN_VALUE": "EXIT",
            "CALL": "UNKNOWN", "LOAD_ATTR": "UNKNOWN", "SEND": "UNKNOWN",
        }
        for name, kind in expected.items():
            with self.subTest(handler=name):
                self.assertEqual(ir_generator.normal_transfer_kind(insts[name], formats[name]),
                                 "_PyIR_TRANSFER_" + kind)

    def slot_role(self, effect: str, body: str) -> str | None:
        analysis = self.analyze(f"\ninst(OP, ({effect})) {{\n{body}\n}}\n")
        return ir_generator.oparg_slot_role([analysis.instructions["OP"]])

    def test_constructor_results_are_not_known_objects(self):
        # The PyValue_FromObject*() constructors make an exact int or
        # float inline, so their results get the generic DUP and an inline
        # test before an object read, not the _POINTER forms
        for ctor in ("PyValue_FromObjectSteal(SPAM())",
                     "PyValue_DUP(PyValue_FromObjectBorrow(SPAM()))",
                     "PyValue_FromObjectBorrow(SPAM())"):
            with self.subTest(ctor=ctor):
                output = self.ir_output(f"""
                op(_MAKE, (-- res)) {{
                    res = {ctor};
                }}
                op(_COPY, (value -- value, copy)) {{
                    copy = PyValue_DUP(value);
                }}
                op(_USE, (value, copy --)) {{
                    PyObject *o = PyValue_AsKnownObjectBorrow(value);
                    EGGS(o);
                    DECREF_INPUTS();
                }}
                macro(OP) = _MAKE + _COPY + _USE;
                """)
                self.assertIn("copy = PyValue_DUP(value);", output)
                self.assertNotIn("_PyValue_DUP_POINTER", output)
                self.assertLess(output.index("PyValue_IsInline(value)"),
                                output.index("PyValue_AsKnownObjectBorrow(value)"))

    def test_slot_roles(self):
        # The role of an oparg that names a local follows from the uses of
        # GETLOCAL(oparg) in the body
        for effect, body, role in [
            ("-- value", "value = PyValue_DUP(GETLOCAL(oparg));",
             "_PyIR_ROLE_LOCAL_IN"),
            ("-- res", "PyObject *o = PyValue_AsKnownObjectBorrow(GETLOCAL(oparg));"
             " res = PyValue_DUP(PyValue_FromObjectBorrow(o));", "_PyIR_ROLE_LOCAL_IN"),
            ("value --", "PyValue tmp = GETLOCAL(oparg); "
             "GETLOCAL(oparg) = value; PyValue_XCLOSE(tmp);",
             "_PyIR_ROLE_LOCAL_OUT"),
            ("value --", "PyValue tmp = GETLOCAL(oparg); "
             "(GETLOCAL(oparg)) = value; PyValue_XCLOSE(tmp);",
             "_PyIR_ROLE_LOCAL_OUT"),
            ("value --", "PyValue_BoxInto(&GETLOCAL(oparg), value);",
             "_PyIR_ROLE_LOCAL_OUT"),
            ("--", "PyValue tmp = GETLOCAL(oparg); "
             "GETLOCAL(oparg) = PyValue_NULL; PyValue_XCLOSE(tmp);",
             "_PyIR_ROLE_LOCAL_CLEAR"),
            ("-- value", "value = GETLOCAL(oparg); "
             "GETLOCAL(oparg) = PyValue_NULL;", "_PyIR_ROLE_LOCAL_MOVE"),
            ("-- value", "PyObject *c = PyValue_AsKnownObjectBorrow(GETLOCAL(oparg));"
             " value = PyValue_FromObjectSteal(PyCell_GetRef((PyCellObject *)c));",
             "_PyIR_ROLE_CELL"),
            ("-- value", "value = PyValue_DUP(PyValue_FromObjectBorrow(SPAM(oparg)));", None),
        ]:
            with self.subTest(body=body):
                self.assertEqual(self.slot_role(effect, body), role)

    def test_feedback_layout_required(self):
        # A family with an entry must have its type in FEEDBACK_LAYOUTS
        input = """
        specializing op(_SPECIALIZE_SPAM, (counter/1, owner -- owner)) {
            _PyFbHeader *feedback = FB_COUNTER();
            ADVANCE_ADAPTIVE_COUNTER(feedback);
        }
        op(_SPAM, (owner -- res)) {
            res = owner;
        }
        macro(SPAM) = _SPECIALIZE_SPAM + _SPAM;
    """
        with self.assertRaisesRegex(SyntaxError, "SPAM has a feedback entry "
                                    "but no layout in FEEDBACK_LAYOUTS"):
            self.formats(input)

    def test_slot_roles_fail_closed(self):
        # A use of GETLOCAL(oparg) of a shape that the generator does not
        # know is an error that names the instruction and the line, not a
        # read
        for effect, body, message in [
            ("value --", "PyValue_StoreAt(&GETLOCAL(oparg), value);",
             "&GETLOCAL(oparg) passed to PyValue_StoreAt"),
            ("--", "PyValue_CLEAR(GETLOCAL(oparg));",
             "GETLOCAL(oparg) passed to PyValue_CLEAR"),
            ("--", "GETLOCAL(oparg) |= 1;", "compound assignment"),
            ("-- res", "res = SPAM(GETLOCAL(oparg) + 1);",
             "a use of GETLOCAL(oparg)"),
        ]:
            with self.subTest(body=body):
                with self.assertRaisesRegex(SyntaxError,
                                            f"OP: {re.escape(message)}") as cm:
                    self.slot_role(effect, body)
                self.assertIsNotNone(cm.exception.lineno)

    def test_ir_only_after_pseudo(self):
        # IR-only handlers follow the pseudo-instructions, so that no number
        # names both.
        input = """
        inst(GO, ( -- )) {
            JUMPBY(oparg);
        }
        pseudo(JUMP, (--)) = {
            GO,
        };
        ir_only inst(IR_BACON, ( -- )) {
            SPAM();
        }
    """
        analysis = self.analyze(input)
        self.assertEqual(analysis.pseudos["JUMP"].opcode, 256)
        self.assertEqual(ir_generator.first_ir_only_handler(analysis), 257)
        self.assertEqual(ir_generator.handler_ids(analysis)["IR_BACON"], 257)

    def test_binary_op_handlers(self):
        # BINARY_OP has one generic handler per operation, with the operation
        # as a constant, after the other IR-only handlers.  Its
        # specialisations deoptimise to the handler of the operation that
        # their entry records.
        input = """
        family(BINARY_OP, 1) = { BINARY_OP_ADD_INT };
        specializing op(_SPECIALIZE_BINARY_OP, (counter/1, lhs, rhs -- lhs, rhs)) {
            _PyFbHeader *feedback = FB_COUNTER();
            ADVANCE_ADAPTIVE_COUNTER(feedback);
        }
        op(_BINARY_OP, (lhs, rhs -- res)) {
            res = SPAM(lhs, rhs, oparg);
            INPUTS_DEAD();
        }
        macro(BINARY_OP) = _SPECIALIZE_BINARY_OP + _BINARY_OP;
        op(_GUARD_INT, (left, right -- left, right)) {
            DEOPT_IF(!EGGS(left, right));
        }
        op(_ADD_INT, (left, right -- res)) {
            res = HAM(left, right);
            INPUTS_DEAD();
        }
        macro(BINARY_OP_ADD_INT) = unused/1 + _GUARD_INT + _ADD_INT;
        ir_only inst(IR_BACON, ( -- )) {
            SPAM();
        }
    """
        analysis = self.analyze(input)
        handlers = ir_generator.binary_op_handlers(analysis)
        self.assertEqual(handlers[0], "BINARY_OP_ADD")
        self.assertIn("BINARY_OP_SUBSCR", handlers)
        ids = ir_generator.handler_ids(analysis)
        self.assertNotIn("BINARY_OP", ids)
        self.assertEqual(ids["IR_BACON"], 256)
        self.assertEqual([ids[name] for name in handlers],
                         list(range(257, 257 + len(handlers))))
        formats = ir_generator.compute_formats(analysis)
        self.assertIs(formats["BINARY_OP_ADD"], formats["BINARY_OP"])
        self.assertEqual(
            ir_metadata_generator.shadow_of("BINARY_OP_ADD", analysis),
            "BINARY_OP")

        out = io.StringIO()
        ir_generator.generate_ir([self.temp_input_filename], analysis, out, False)
        code = out.getvalue()
        self.assertNotIn("TARGET(BINARY_OP)", code)
        generic = code[code.index("TARGET(BINARY_OP_SUBTRACT)"):]
        generic = generic[:generic.index("DISPATCH();")]
        self.assertIn("int oparg = NB_SUBTRACT;", generic)
        self.assertIn("INSTRUCTION_STATS(BINARY_OP_SUBTRACT);", generic)
        handler = code[code.index("TARGET(BINARY_OP_ADD_INT)"):]
        handler = handler[:handler.index("DISPATCH();")]
        self.assertIn("IR_DEOPT_BINARY_OP(IR_D(ir_word));", handler)
        self.assertNotIn("JUMP_TO_PREDICTED", handler)

        out = io.StringIO()
        ir_metadata_generator.generate_ir_metadata(
            [self.temp_input_filename], analysis, out)
        lines = [line.strip() for line in out.getvalue().splitlines()]
        self.assertIn("#define _PyIR_BINARY_OP_FIRST" + " " * 26
                      + "BINARY_OP_ADD", lines)
        self.assertIn('static_assert(BINARY_OP_ADD == _PyIR_BINARY_OP_FIRST + '
                      'NB_ADD, "BINARY_OP_ADD");', lines)

# The box area's sites, which only the generator emits (R27 deletes them)
BOX_AREA_SITES = ("&value_owners", "value_boxes[] (a local array)")


def generated_address_sites(text):
    "The sites in each generated handler or label that take a local's address"
    parts = re.split(r"\n\s*(?:TARGET|LABEL)\((\w+)\)", text)
    sites = {}
    for name, body in zip(parts[1::2], parts[2::2]):
        tokens = [t for t in lexer.tokenize(body)
                  if t.kind not in ("COMMENT", "NEWLINE", "CMACRO",
                                    "CMACRO_OTHER")]
        sites[name] = [site for site, _ in
                       ir_generator.addressed_locals(name, tokens, {})]
    return sites


class TestCallTransactionMarkers(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        source = str(ir_generator.DEFAULT_INPUT)
        cls.analysis = analyzer.analyze_files([source])
        output = io.StringIO()
        ir_generator.generate_ir([source], cls.analysis, output, False)
        cls.interpreter = output.getvalue()

    def test_send_completion_events_without_instrumented_twin(self):
        output = io.StringIO()
        ir_metadata_generator.generate_ir_metadata(
            [str(ir_generator.DEFAULT_INPUT)], self.analysis, output)
        row = next(line for line in output.getvalue().splitlines()
                   if line.lstrip().startswith("[SEND] = "))
        self.assertIn(".twin = 0", row)
        self.assertIn("(1u << PY_MONITORING_EVENT_BRANCH_LEFT)", row)
        self.assertIn("(1u << PY_MONITORING_EVENT_STOP_ITERATION)", row)

    def test_real_callout_and_transfer_properties(self):
        for name in ("_MAYBE_EXPAND_METHOD", "_MONITOR_CALL",
                     "_MAKE_CALLARGS_A_TUPLE", "_DO_CALL", "_PY_FRAME_GENERAL",
                     "_PY_FRAME_GENERAL_GUARDED", "_DO_CALL_KW", "_PY_FRAME_KW",
                     "_PY_FRAME_KW_GUARDED",
                     "_DO_CALL_FUNCTION_EX"):
            with self.subTest(name=name):
                uop = self.analysis.uops[name]
                self.assertTrue(uop.properties.escapes)
                self.assertTrue(uop.properties.error_without_pop)
                self.assertFalse(uop.properties.error_with_pop)
                self.assertTrue(uop.properties.sync_sp)
                self.assertTrue(uop.properties.needs_this)
                self.assertFalse(uop.is_viable())
        for name in ("_DO_CALL", "_PY_FRAME_GENERAL_GUARDED", "_PY_FRAME_KW_GUARDED"):
            invoke = self.analysis.uops[name].properties
            self.assertTrue(invoke.always_exits)
            self.assertTrue(invoke.switches_frame)
        self.assertFalse(self.analysis.uops["_MONITOR_CALL"].properties.always_exits)

    def test_exact_activation_does_not_escape(self):
        properties = self.analysis.uops["_INIT_CALL_PY_EXACT_ARGS"].properties
        self.assertFalse(properties.escapes)
        self.assertFalse(properties.error_without_pop)
        self.assertFalse(properties.sync_sp)

    def test_pure_latch_and_unrelated_uppercase_macro(self):
        for marker in ("IR_BEGIN_CALL_EFFECTS", "UNRELATED_MACRO"):
            node, = parse_src(f"op(_OP, (--)) {{ {marker}(); }}")
            properties = analyzer.compute_properties(node)
            self.assertFalse(properties.escapes)
            self.assertFalse(properties.error_without_pop)
            self.assertFalse(properties.always_exits)
            self.assertFalse(properties.switches_frame)
            self.assertFalse(properties.sync_sp)

    def test_prelude_and_actual_destination_terminal_paths(self):
        for name in ("CALL", "CALL_NON_PY_GENERAL",
                     "CALL_PY_GENERAL", "CALL_BOUND_METHOD_GENERAL",
                     "CALL_KW", "CALL_KW_NON_PY", "CALL_KW_PY",
                     "CALL_FUNCTION_EX", "CALL_EX_NON_PY_GENERAL", "CALL_EX_PY",
                     "INSTRUMENTED_CALL", "INSTRUMENTED_CALL_KW",
                     "INSTRUMENTED_CALL_FUNCTION_EX"):
            with self.subTest(name=name):
                body = self.interpreter.split(f"TARGET({name}) {{", 1)[1].split("TARGET(", 1)[0]
                capture = body.index("call_transaction_begin(")
                invoke = body.index("call_transaction_run_")
                self.assertLess(capture, invoke)
                for preparation in ("_Py_Specialize_Call",
                                    "call_transaction_expand_method(",
                                    "call_transaction_prepare_tuple(",
                                    "call_transaction_monitor("):
                    if preparation in body:
                        self.assertLess(capture, body.index(preparation))
                self.assertNotIn("IR_MOVE_TO_DST", body[invoke:])
                self.assertNotIn("IR_CLEAR_SLOTS", body[invoke:])
                self.assertNotIn("_PyValue_Publish", body[invoke:])
                self.assertNotIn("_Py_VectorCall", body)
                self.assertIn("DISPATCH_INLINED(call_result.child)", body)
                # The transaction lives in the out-of-line run (R-c)
                self.assertNotIn("_PyCallTransaction", body)
                self.assertNotIn("&call", body)
        labels = re.findall(r"^\s*(call_transaction_fallback_\w+):", self.interpreter, re.M)
        self.assertEqual(len(labels), len(set(labels)))

    def test_exact_method_post_effect_guards_forward_prepared_roles(self):
        for name in ("CALL_BOUND_METHOD_EXACT_ARGS",
                     "CALL_NO_SELF_BOUND_METHOD_EXACT_ARGS"):
            with self.subTest(handler=name):
                body = self.interpreter.split(f"TARGET({name}) {{", 1)[1].split("TARGET(", 1)[0]
                promoted = body.index("call_state |= CALL_SELF_PRESENT | CALL_EFFECTS")
                wrapper_close = body.index("PyValue_CLOSE", promoted)
                suffix = body[wrapper_close:]
                guards = suffix.split("JUMP_TO_PREDICTED(")[:-1]
                self.assertGreaterEqual(len(guards), 4)
                for guard in guards:
                    self.assertIn(f"if (call_state & CALL_EFFECTS) {{ goto call_transaction_fallback_{name}; }}", guard)
                fallback = body.index(f"call_transaction_fallback_{name}:")
                self.assertIn("call_transaction_run_vector(", body[fallback:])
                self.assertIn("| call_state", body[fallback:])

    def test_handlers_take_no_local_address(self):
        # The generated handlers and labels take the address of none of
        # their locals except the box area (register-ir-v2-a3.md 3.5): the
        # check of the uop bodies, applied to what the generator emits.
        # The box area is exactly in the handlers that box, and R27 deletes
        # it, so their number may only fall.
        _, rest = self.interpreter.split(ir_generator.INSTRUCTION_START_MARKER)
        sites = generated_address_sites(rest)
        self.assertGreater(len(sites), 200)
        box_area = set()
        for name, found in sites.items():
            others = [site for site in found if not site.endswith(BOX_AREA_SITES)]
            self.assertEqual(others, [], name)
            if len(others) < len(found):
                box_area.add(name)
        formats = ir_generator.compute_formats(self.analysis)
        self.assertEqual(box_area,
                         ir_generator.boxing_handlers(self.analysis, formats))
        self.assertLessEqual(len(box_area), 97)

    def test_forward_distance_is_signed(self):
        # A forward distance has 12 bits and a sign (register-ir-v2 section
        # 4.3).  _PyCode_InstrOparg, dis and the generated handlers decode
        # the same value, so a handler never reads the field as unsigned.
        _, rest = self.interpreter.split(ir_generator.INSTRUCTION_START_MARKER)
        bodies = dict(re.findall(
            r"TARGET\((\w+)\) \{(.*?)(?=\n        TARGET\()", rest, re.S))
        checked = 0
        for name, body in bodies.items():
            if name.startswith(("FOR_ITER", "SEND", "INSTRUMENTED_FOR_ITER",
                                "INSTRUMENTED_SEND")):
                with self.subTest(handler=name):
                    self.assertNotIn("(int)IR_D(ir_word)", body)
                    if "int oparg" in body:
                        self.assertIn("int oparg = IR_DS(ir_word);", body)
                        checked += 1
        self.assertGreater(checked, 5)

    def test_generated_address_sites(self):
        # The check of the generated output sees an address in a
        # conditional, after a cast, in parentheses and in a comparison
        sites = generated_address_sites("""
            TARGET(OP) {
                PyValue attr = PyValue_NULL;
                PyValue receiver = ir_own_B;
                int retire_failed = retire_published_values(tstate, frame,
                    next_instr, NULL, &receiver, NULL,
                    (IR_IS_LOCAL(IR_A(ir_word)) ? &attr : NULL));
                spam_call((void *)&retire_failed, &(ir_own_B),
                          receiver.bits == &attr, oparg & 1);
                DISPATCH();
            }
        """)
        self.assertEqual([site.split(": ")[-1] for site in sites["OP"]],
                         ["&receiver", "&attr", "&retire_failed",
                          "&ir_own_B", "&attr"])


if __name__ == "__main__":
    unittest.main()
