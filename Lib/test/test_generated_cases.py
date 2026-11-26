import contextlib
from dataclasses import replace
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
    import ir_contract
    import ir_generator
    import ir_records
    import stack
    import ir_metadata_generator
    import opcode_metadata_generator
    import lexer
    import parser
    from stack import Local, Stack



def expand_outlined(text: str) -> str:
    """Put the body of each outlined handler where its stub is, so that a test
    reads a handler as one text: the stub of an outlined handler calls the
    function ir_run_NAME, which the generated file defines before, and the
    function is dropped."""
    functions = []
    for match in re.finditer(
            r"\nstatic Py_NO_INLINE _PyIRTransfer\nir_run_(\w+)\(\n[^{]*\n\{",
            text):
        start = match.end() - 1
        ends = [e for e in (
            text.find("\nstatic Py_NO_INLINE _PyIRTransfer\n", start),
            text.find("\n#else", start)) if e >= 0]
        end = min(ends) if ends else len(text)
        functions.append((match.group(1), match.start(), start, end))
    for name, first, start, end in functions:
        body = text[start:end].rstrip()
        stub = re.compile(r"TARGET\(%s\) \{\s*IR_RUN_OUTLINED\(ir_run_%s\);\s*\}"
                          % (name, name))
        text = stub.sub(lambda _: f"TARGET({name}) " + body, text, 1)
    for name, first, start, end in sorted(functions, key=lambda f: -f[1]):
        text = text[:first] + text[end:]
    return text


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
                [self.temp_input_filename])
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


def generator_labels_source():
    "The generator routines that the handlers' IR_GEN_* macros enter"
    return (Path(__file__).resolve().parents[2] /
            "Python" / "ceval_routines.h").read_text()



TEST_ROWS = """
#define TEST_ROWS(PRIMITIVE, HELPER, MACRO) \\
    MACRO(SPAM, ARGS(scalar, scalar, scalar), SCALAR, EFFECTS(reads), EXITS()) \\
    MACRO(SPAM_FLAG, ARGS(), SCALAR, EFFECTS(), EXITS()) \\
    MACRO(SPAM_KEEP, ARGS(scalar), NONE, EFFECTS(), EXITS()) \\
    MACRO(EGGS, ARGS(scalar, scalar), SCALAR, EFFECTS(), EXITS()) \\
    MACRO(HAM, ARGS(scalar, scalar), SCALAR, EFFECTS(), EXITS())
"""


class TestIRCases(unittest.TestCase):
    """The register IR backend: ir_generator.py and ir_metadata_generator.py"""

    def setUp(self) -> None:
        super().setUp()
        self.maxDiff = None
        fd, self.temp_input_filename = tempfile.mkstemp(suffix=".c")
        os.close(fd)
        # The families of the tests that have feedback entries: a cache
        # operand names a field of the entry type (pycore_feedback.h)
        layouts = mock.patch.dict(ir_generator.FEEDBACK_LAYOUTS, {
            "OP": "_PyFbUnary", "BRANCH": ir_generator.FB_HEADER})
        layouts.start()
        self.addCleanup(layouts.stop)
        # SPAM stands for a function that reads its arguments (S3), and
        # spam_call for one that reads them and can run code
        readers = mock.patch.object(ir_generator, "OBJECT_READERS",
                                    ir_generator.OBJECT_READERS | {"SPAM", "spam_call"})
        readers.start()
        self.addCleanup(readers.stop)
        # The analyzer takes a call for one that can run code unless a row
        # says otherwise: these upper-case stand-ins have rows that declare
        # no effect that reenters
        rows = mock.patch.dict(ir_contract.rows(), ir_contract.parse_rows(TEST_ROWS))
        rows.start()
        self.addCleanup(rows.stop)

    def tearDown(self) -> None:
        os.remove(self.temp_input_filename)
        super().tearDown()

    def add_rows(self, text: str) -> None:
        """Rows for this test's stand-in callees, in TEST_ROWS's form: a
        declared uop calls only rows"""
        rows = mock.patch.dict(ir_contract.rows(), ir_contract.parse_rows(text))
        rows.start()
        self.addCleanup(rows.stop)

    def analyze(self, input: str):
        with open(self.temp_input_filename, "w") as temp_input:
            temp_input.write(parser.BEGIN_MARKER)
            temp_input.write(input)
            temp_input.write(parser.END_MARKER)
        with handle_stderr():
            return analyzer.analyze_files(
                [self.temp_input_filename])

    def formats(self, input: str):
        return ir_generator.compute_formats(self.analyze(input))

    def ir_output(self, input: str) -> str:
        analysis = self.analyze(input)
        out = io.StringIO()
        ir_generator.generate_ir([self.temp_input_filename], analysis, out, False)
        _, rest = expand_outlined(out.getvalue()).split(
            ir_generator.INSTRUCTION_START_MARKER)
        instructions, _ = rest.split(ir_generator.INSTRUCTION_END_MARKER)
        return instructions

    def handler(self, output: str, name: str) -> str:
        "The handler of name in output"
        output = expand_outlined(output)
        start = output.index(f"TARGET({name}) {{")
        end = output.find("TARGET(", start + 1)
        return output[start:end if end >= 0 else len(output)]

    def test_generator_boundary_source_closure(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()

        def checked(text):
            Path(self.temp_input_filename).write_text(text)
            analysis = analyzer.analyze_files([self.temp_input_filename])
            formats = ir_generator.compute_formats(analysis)
            return analysis, formats, ir_generator.generator_boundaries(analysis)

        analysis, formats, rows = checked(source)
        self.assertEqual(len(rows), 38)
        for name, row in rows.items():
            with self.subTest(name=name):
                self.assertTrue(ir_generator.generator_boundary_source_closed(
                    row, analysis, formats))
                instruction = ir_generator.handler_instructions(analysis)[name]
                ordinary = ir_generator.all_instructions(analysis)[row.base]
                self.assertEqual(ir_generator.monitoring_events(name, instruction),
                                 ir_generator.monitoring_events(row.base, ordinary))
        for before, after in (
            # A call that no row declares is refused by the verifier of the
            # uop's declaration; the closure holds the declaration to what a
            # SEND may do
            ("effects(runs_python, switches_frame) exits(next, branch, enter, raise)",
             "effects(runs_python, switches_frame, polls) exits(next, branch, enter, raise)"),
        ):
            with self.subTest(mutation=before):
                self.assertIn(before, source)
                analysis, formats, rows = checked(source.replace(before, after, 1))
                self.assertFalse(any(ir_generator.generator_boundary_source_closed(
                    row, analysis, formats) for row in rows.values()))

    def test_generator_boundary_placement_cohorts_and_entry(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()
        Path(self.temp_input_filename).write_text(source)
        analysis = analyzer.analyze_files([self.temp_input_filename])
        formats = ir_generator.compute_formats(analysis)
        rows = ir_generator.generator_boundaries(analysis)
        for cohort in ir_generator.redispatch_cohorts(analysis, formats):
            shapes = {(rows[name].yielded, rows[name].completed, rows[name].resume)
                      if name in rows else None for name in cohort}
            self.assertEqual(len(shapes), 1)
        for row in rows.values():
            entries = {ir_generator.generator_word_entry(name, analysis)
                       for name, other in rows.items() if other.base == row.base}
            self.assertEqual(len(entries), 1)
        instructions = ir_generator.handler_instructions(analysis)
        output = io.StringIO()
        out = CWriter(output, 0, False)
        emitter = ir_generator.IREmitter(out, analysis, formats)
        name = "SEND_GEN_XFER_DISCARD"
        ir_generator.generate_handler(name, instructions[name], formats[name],
                                      emitter, out)
        body = output.getvalue()
        self.assertIn("IR_GEN_JUMP(GEN_ENTRY_EFFECT", body)
        self.assertNotIn("_Py_EnterRecursivePy(tstate)", body)
        self.assertEqual(body.count("--tstate->py_recursion_remaining;"), 1)
        # The handler enters the resume path, which reads the resumed
        # frame's boundary shape from the resume table
        self.assertIn("IR_GEN_RESUME(ir_resume_input);", body)
        self.assertNotIn("generator_resume_targets[", body)
        self.assertIn("generator_resume_targets[IR_HANDLER(", generator_labels_source())
        self.assertTrue(any(event.kind == "_PyIR_OWNER_NATIVE_WINDOW_OUT"
                            for event in emitter.ownership_events))

    def test_generator_native_result_preserves_virtual_cursor(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()
        Path(self.temp_input_filename).write_text(source)
        analysis = analyzer.analyze_files([self.temp_input_filename])
        formats = ir_generator.compute_formats(analysis)
        instructions = ir_generator.handler_instructions(analysis)
        output = io.StringIO()
        out = CWriter(output, 0, False)
        emitter = ir_generator.IREmitter(out, analysis, formats)
        name = "SEND_VIRTUAL_XFER_DISCARD"
        ir_generator.generate_handler(name, instructions[name], formats[name],
                                      emitter, out)
        body = output.getvalue()
        update = body.index("next_index = PyValue_TagIndex(item.index);")
        publish = body.index("IR_SET_TEMP(window[1], next_index);", update)
        clear = body.index("window[2] = PyValue_NULL;", publish)
        transfer = body.index("IR_GEN_YIELD(next);", clear)
        self.assertLess(update, publish)
        self.assertLess(publish, clear)
        self.assertLess(clear, transfer)
        self.assertNotIn("IR_SET_TEMP(window[2], next);", body)
        self.assertNotIn("IR_MOVE_TO_DST", body)

    def test_generator_provider_result_precedes_sent_input_effect(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()
        Path(self.temp_input_filename).write_text(source)
        analysis = analyzer.analyze_files([self.temp_input_filename])
        formats = ir_generator.compute_formats(analysis)
        instructions = ir_generator.handler_instructions(analysis)
        for name in ("SEND_XFER_DISCARD", "SEND_ASYNC_GEN_XFER_DISCARD"):
            with self.subTest(name=name):
                output = io.StringIO()
                out = CWriter(output, 0, False)
                emitter = ir_generator.IREmitter(out, analysis, formats)
                ir_generator.generate_handler(name, instructions[name], formats[name],
                                              emitter, out)
                body = output.getvalue()
                provider = body.index("_PyValue_Send(")
                attempt = body.index("if (!_PyValue_TryXCloseNoReentry(v))", provider)
                effect = body.index("IR_GEN_JUMP(GEN_YIELD_INPUT_EFFECT", attempt)
                clear = body.index("window[2] = PyValue_NULL;", effect)
                self.assertLess(provider, attempt)
                self.assertLess(attempt, effect)
                self.assertLess(effect, clear)
                self.assertNotIn("PyValue_CLOSE(v);", body)
                self.assertIn("this_instr, sent.value);", body)

    def test_generator_completion_resolves_actual_source(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()
        Path(self.temp_input_filename).write_text(source)
        analysis = analyzer.analyze_files([self.temp_input_filename])
        formats = ir_generator.compute_formats(analysis)
        instructions = ir_generator.handler_instructions(analysis)
        for name in ("SEND_XFER_DISCARD", "SEND_VIRTUAL_XFER_DISCARD",
                     "SEND_ASYNC_GEN_XFER_DISCARD", "CLEANUP_THROW_XFER_DISCARD"):
            with self.subTest(name=name):
                output = io.StringIO()
                out = CWriter(output, 0, False)
                emitter = ir_generator.IREmitter(out, analysis, formats)
                ir_generator.generate_handler(name, instructions[name], formats[name],
                                              emitter, out)
                body = output.getvalue()
                # The handler hands the completion value, a home, to the
                # completion path with its own word as the source
                guard = body.index("assert(_PyValue_IsHome(ir_completed));")
                tail = body.index("IR_GEN_COMPLETE(ir_completed);", guard)
                self.assertLess(guard, tail)
                self.assertNotIn("generator_completed_targets", body)
                self.assertNotIn("generator_site_kind(", body)
        # The path resolves the consumer from the word at the source, which
        # looks through a monitor's wrapper, and never from the handler
        labels = generator_labels_source()
        complete = labels[labels.index("IR_HOT_ROUTINE(gen_complete,"):]
        resolve = complete.index("generator_site_kind(frame, next_instr, "
                                 "generator_completed_targets)")
        guard = complete.index("assert(kind != 0);", resolve)
        self.assertLess(resolve, guard)
        self.assertNotIn("generator_completed_targets[IR_HANDLER(ir_word)]", labels)

    def test_generator_explicit_completion_consumer_targets(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()
        Path(self.temp_input_filename).write_text(source)
        analysis = analyzer.analyze_files([self.temp_input_filename])
        rows = ir_generator.generator_boundaries(analysis)
        ids = ir_generator.handler_ids(analysis)
        output = io.StringIO()
        ir_generator.generate_ir_targets([self.temp_input_filename], analysis, output)
        targets = output.getvalue().split("generator_completed_targets", 1)[1].split("};", 1)[0]
        for name, row in rows.items():
            if row.completed not in {"STORE_FAST", "POP_TOP"}:
                continue
            kind = f"GEN_COMPLETE_{row.completed}"
            if row.base == "CLEANUP_THROW":
                kind = f"GEN_THROWING | {kind}"
            self.assertIn(f"[{ids[name]}] = {kind},", targets)
            self.assertIn(f"case GEN_COMPLETE_{row.completed}:",
                          generator_labels_source())
            self.assertEqual(ir_generator.generator_word_entry(name, analysis),
                             ir_generator.generator_word_entry(
                                 f"{row.base}_XFER_DISCARD", analysis))

    def test_inline_input_read_as_object_needs_an_assert(self):
        # No handler boxes an inline value (handlers81, H3): a uop that reads
        # an input as an object, outside a guard, declares the `object` fact
        # of it (an assert does not establish it)
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            HELPER(spam_call, PyObject *, (PyObject *), \\
                   ARGS(object), VALUE_OR_NULL(owned), EFFECTS(reads, allocates, raises))
        """)
        body = """
        inst(OP, (value: consumed%s -- result: owned)) effects(reads, allocates, raises, releases)
                exits(next, raise) {
            PyObject *object = PyValue_AsKnownObjectBorrow(value);
            PyObject *found = spam_call(object);
            DECREF_INPUTS();
            ERROR_IF(found == NULL);
            result = PyValue_FromObjectSteal(found);
        }
        """
        with self.assertRaisesRegex(SyntaxError, "requires object of it"):
            self.ir_output(body % "")
        output = self.ir_output(body % " object")
        handler = self.handler(output, "OP")
        self.assertNotIn("value_temps", handler)
        self.assertNotIn("_PyValueTemps", handler)
        self.assertNotIn("finish_value_bridges", handler)

    def test_an_assumed_object_input_needs_no_assert(self):
        # A declared `assumed(object) by P` is the evidence: every execution
        # form asserts the assumption, so the body need not repeat it
        body = """
        inst(OP, (value%s -- value)) effects(reads) exits(next) {
            PyObject *object = PyValue_AsKnownObjectBorrow(value);
            PyObject *code = PyFunction_GET_CODE(object);
        }
        """
        output = self.ir_output(body % ": assumed(object) by SPAM")
        self.assertIn("assert(PyValue_IsObject(value));", self.handler(output, "OP"))
        self.assertEqual(self.analyze(body % ": assumed(object) by SPAM")
                         .uops["OP"].asserted_objects, ("value",))
        # Neither an assumption nor an assert: the contract refuses it, as
        # the generator does an undeclared uop
        # (test_inline_input_read_as_object_needs_an_assert)
        with self.assertRaisesRegex(SyntaxError, "requires object of it"):
            self.ir_output(body % "")

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
                                          formats[instruction.name], emitter, out)
            return output.getvalue()

        self.assertIsNotNone(ir_generator.observer_entry_source(
            instruction, formats[instruction.name]))
        expected = emitted()
        with mock.patch.object(ir_generator, "observer_entry_source", return_value=None):
            uncovered = emitted()
        self.assertEqual(expected, uncovered)
        self.assertIn("PROCESS_ERROR_AT(RAISE, 0, next_instr - 1)", expected)

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
        self.assertIn("call_bind_positional(tstate, frame, this_instr, IR_SLOT(IR_B(ir_word)), PyValue_NULL, &fb->plan)", body)
        self.assertIn("call_run_keywords(", body)
        self.assertIn("if (call_result.status < 0)", body)
        for name in ("CALL_KW_BOUND_METHOD", "INSTRUMENTED_CALL_KW"):
            with self.subTest(name=name):
                self.assertNotIn("call_bind_positional(", self.handler(output, name))

        changes = (
            ("_CHECK_PEP_523 +\n            _CHECK_FUNCTION_VERSION_KW +",
             "_CHECK_FUNCTION_VERSION_KW +"),
            ("_CHECK_FUNCTION_VERSION_KW +\n            _CHECK_RECURSION_REMAINING +",
             "_CHECK_RECURSION_REMAINING +"),
            ("_CHECK_RECURSION_REMAINING +\n            _PY_FRAME_KW_GUARDED +",
             "_PY_FRAME_KW_GUARDED +"),
            ("(callable: taken, self_or_null: taken, args[oparg]: taken, kwnames: borrowed\n"
             "                 -- new_frame: owned))",
             "(callable: taken, self_or_null: taken, args[oparg]: taken\n"
             "                 -- new_frame: owned))"),
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
        inst(OP, (value: consumed -- res: moved(value)))
            effects(runs_python) exits(next, branch, raise)
        {
            PyValue held = TAKE(value);
            res = held;
            IR_OBSERVE_COMMITTED_BRANCH(this_instr, JUMP_TARGET(oparg),
                                        PY_MONITORING_EVENT_BRANCH_RIGHT, _PyIR_EDGE_TAKEN);
        }
        """
        output = self.ir_output(source)
        self.assertNotIn("value_boxes[", output)
        self.assertNotIn("value_owners", output)
        changes = (
            ("res = held;", "PyValue_CLOSE(held);", "`res` is not written before IR_OBSERVE_COMMITTED_BRANCH"),
            ("res = held;", "PyValue extra = PyValue_DUP(held); res = held;",
             "the new owner `extra` is still held"),
            ("res = held;", "PyValue alias = held; PyValue_CLOSE(held); res = alias;",
             "which is already consumed or moved"),
            ("res = held;", "if (flag) { PyValue_CLOSE(held); } res = held;",
             "owned on one path and not on another"),
            ("res = held;", "steal(held); res = held;",
             "calls steal, which pycore_ir_components.h does not declare"),
            ("res = held;", "int ir_observer_next = 0; res = held;",
             "reserved local"),
            ("PY_MONITORING_EVENT_BRANCH_RIGHT, _PyIR_EDGE_TAKEN);",
             "PY_MONITORING_EVENT_BRANCH_RIGHT, _PyIR_EDGE_TAKEN); PyValue_CLOSE(res);",
             "follows IR_OBSERVE_COMMITTED_BRANCH, which never returns"),
            ("PY_MONITORING_EVENT_BRANCH_RIGHT, _PyIR_EDGE_TAKEN);",
             "PY_MONITORING_EVENT_BRANCH_RIGHT, _PyIR_EDGE_TAKEN); "
             "IR_OBSERVE_COMMITTED_BRANCH(this_instr, JUMP_TARGET(oparg), "
             "PY_MONITORING_EVENT_BRANCH_RIGHT, _PyIR_EDGE_TAKEN);", "follows IR_OBSERVE_COMMITTED_BRANCH, which never returns"),
        )
        for old, new, message in changes:
            with self.subTest(message=message):
                self.assertIn(old, source)
                with self.assertRaisesRegex(SyntaxError, message):
                    self.ir_output(source.replace(old, new, 1))
        # An owner still held at the observer: `res` is declared owned, not
        # moved from the input, so that the assignment alone is not refused
        with self.assertRaisesRegex(SyntaxError, "the new owner `held` is still held"):
            self.ir_output(source.replace("res: moved(value)", "res: owned")
                           .replace("res = held;", "res = PyValue_None;", 1))
        with self.assertRaisesRegex(SyntaxError, "finish the composed handler"):
            self.ir_output(source.replace("inst(OP,", "op(_OP,") + """
            op(_AFTER, (--)) effects() exits(next) { assert(1); }
            macro(OP) = _OP + _AFTER;
        """)

    def test_terminal_observer_defined_null_edge(self):
        source = """
        inst(OP, (value: consumed -- res: moved(value) nullable))
            effects(runs_python, releases) exits(next, branch, raise)
        {
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
        analysis = analyzer.analyze_files([self.temp_input_filename])
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
        with self.assertRaisesRegex(SyntaxError, "already consumed or moved"):
            self.ir_output(source.replace("IR_DEFER_CLOSE(held);",
                "IR_DEFER_CLOSE(held); IR_DEFER_CLOSE(held);"))
        with self.assertRaisesRegex(SyntaxError, "names no owner"):
            self.ir_output(source.replace("IR_DEFER_CLOSE(held);",
                                          "IR_DEFER_CLOSE(unknown);"))
        for suffix, message in (
            ('held = PyValue_None;', "after transfer"),
            ('PyValue_CLOSE(held);', "already consumed or moved"),
            ('PyValue_IsNull(held);', "after transfer"),
        ):
            with self.subTest(suffix=suffix):
                with self.assertRaisesRegex(SyntaxError, message):
                    self.ir_output(source.replace('IR_DEFER_CLOSE(held);',
                        'IR_DEFER_CLOSE(held); ' + suffix))
        with self.assertRaisesRegex(SyntaxError, 'deferred owner exits'):
            self.ir_output(source.replace('IR_DEFER_CLOSE(held);',
                                          'IR_DEFER_CLOSE(held); ERROR_NO_POP();'))

    def test_terminal_observer_explicit_owner_edge(self):
        jump = """
        inst(OP, (--)) effects(runs_python) exits(next, branch, raise) {
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
        inst(OP, (iter: consumed, index_or_null: closed --))
            effects(runs_python) exits(next, branch, raise)
        {
            PyValue held = TAKE(iter);
            (void)index_or_null;
            DEAD(index_or_null);
            IR_DEFER_CLOSE(held);
            IR_OBSERVE_COMMITTED_BRANCH(prev_instr, next_instr,
                PY_MONITORING_EVENT_BRANCH_RIGHT, _PyIR_EDGE_FALLTHROUGH);
        }
        """
        output = self.ir_output(iterator)
        # The event runs in a cold function, which sets the return pc
        callback = output.index("IR_OBSERVE_EDGE(")
        self.assertIn("IR_OBSERVE_EDGE(prev_instr, ir_observer_next, "
                      "PY_MONITORING_EVENT_BRANCH_RIGHT, 0);", output)
        # The deferred close comes after the last detach and before the callback
        self.assertLess(output.index("held = PyValue_NULL;"),
                        output.index("PyValue_XCLOSE(ir_close1)"))
        self.assertLess(output.index("PyValue_XCLOSE(ir_close1)"), callback)
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

    def test_terminal_observer_owner_transport_reads_the_rows(self):
        # The cursor taken by value is a C owner until the helper that
        # consumes it (its row's ARGS) takes it; a use the rows do not name
        # proves nothing
        source = (Path(__file__).resolve().parents[2] / 'Python' / 'bytecodes.c').read_text()
        start = source.index('inst(INSTRUMENTED_FOR_ITER,')
        helper = 'PyValue_Borrow(iter), cursor);'
        self.assertIn(helper, source[start:])

        def paths(old, new):
            changed = source[:start] + source[start:].replace(old, new, 1)
            Path(self.temp_input_filename).write_text(changed)
            analysis = analyzer.analyze_files([self.temp_input_filename])
            formats = ir_generator.compute_formats(analysis)
            return ir_generator.observer_terminal_paths(
                analysis.instructions['INSTRUMENTED_FOR_ITER'],
                formats['INSTRUMENTED_FOR_ITER'])

        self.assertEqual(len(paths(helper, helper)), 1)
        with self.assertRaisesRegex(SyntaxError, 'unproved terminal C owner transport'):
            paths(helper, 'PyValue_Borrow(iter), PyValue_DUP(cursor));')
        with self.assertRaisesRegex(SyntaxError, 'used after transfer: cursor'):
            paths(helper, 'PyValue_Borrow(iter), cursor); steal(cursor);')

    def test_canonical_frame_transfer_provenance_fails_closed(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()

        def analyze(text):
            Path(self.temp_input_filename).write_text(text)
            with handle_stderr():
                return analyzer.analyze_files([self.temp_input_filename])

        original = analyze(source)
        self.assertTrue(ir_generator.canonical_frame_transfer_input(
            original.instructions["YIELD_VALUE"]))

        changes = (
            ("PyValue temp = retval;",
             "retval = PyValue_DUP(retval); PyValue temp = retval;",
             ("YIELD_VALUE",)),
        )
        for old, new, affected in changes:
            with self.subTest(new=new):
                self.assertIn(old, source)
                analysis = analyze(source.replace(old, new, 1))
                for name in affected:
                    self.assertFalse(ir_generator.canonical_frame_transfer_input(
                        analysis.instructions[name]))

    def test_canonical_yield_sources(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()
        Path(self.temp_input_filename).write_text(source)
        analysis = analyzer.analyze_files([self.temp_input_filename])
        formats = ir_generator.compute_formats(analysis)
        self.assertTrue(ir_generator.canonical_frame_transfer_input(
            analysis.instructions['YIELD_VALUE']))
        self.assertFalse(ir_generator.canonical_frame_transfer_input(
            analysis.instructions['INSTRUMENTED_YIELD_VALUE']))
        for name, store in (("YIELD_VALUE", "IR_STORE_RETURN(value);"),
                            ("INSTRUMENTED_YIELD_VALUE", "IR_STORE_RETURN(value);")):
            output = io.StringIO()
            writer = CWriter(output, 0, False)
            emitter = ir_generator.IREmitter(writer, analysis, formats)
            ir_generator.generate_handler(name, analysis.instructions[name],
                                          formats[name], emitter, writer)
            self.assertIn(store, output.getvalue())

        renamed = source.replace('YIELD_VALUE', 'RENAMED_YIELD')
        Path(self.temp_input_filename).write_text(renamed)
        other = analyzer.analyze_files([self.temp_input_filename])
        self.assertTrue(ir_generator.canonical_frame_transfer_input(
            other.instructions['RENAMED_YIELD']))

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
            ('window op(_YIELD_VALUE, (retval: consumed -- value: moved(retval)))',
             'window op(_YIELD_VALUE, (TAKE/1, retval: consumed -- value: moved(retval)))'),
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
        formats = ir_generator.compute_formats(analysis)

        def emitted(name):
            output = io.StringIO()
            out = CWriter(output, 0, False)
            emitter = ir_generator.IREmitter(out, analysis, formats)
            instruction = ir_generator.all_instructions(analysis)[name]
            ir_generator.generate_handler(name, instruction,
                                          formats[name], emitter, out)
            return output.getvalue()

        entries = {
            "FOR_ITER": "ForIter", "INSTRUMENTED_FOR_ITER": "ForIter",
            "FOR_ITER_LIST": "ForIter", "FOR_ITER_TUPLE": "ForIter",
            "FOR_ITER_RANGE": "ForIter", "FOR_ITER_VIRTUAL": "ForIter",
            "SEND": "Send", "SEND_VIRTUAL": "Send",
            "SEND_ASYNC_GEN": "Send", "CLEANUP_THROW": "SendThrow",
        }
        for name, entry in entries.items():
            with self.subTest(name=name):
                handler = emitted(name)
                self.assertIn("PyValue completed =", handler)
                self.assertIn("_PyIRStepResult ir_step_result =", handler)
                self.assertNotIn("&completed", handler)
                self.assertNotIn("_PyEval_CompleteStep(", handler)
                self.assertNotIn("_PyIRStepResult completed =", handler)
                target = ("_PyFrame_InstrAt(frame, oparg)"
                          if name == "CLEANUP_THROW" else "next_instr + oparg")
                recipient = "" if entry == "ForIter" else "IR_A(ir_word), "
                self.assertIn(
                    f"_PyEval_Complete{entry}(tstate, frame, this_instr, "
                    f"IR_B(ir_word), {recipient}{target}, completed);", handler)
                if name == "CLEANUP_THROW":
                    self.assertNotIn("assert(!IR_IS_LOCAL(dst)", handler)
        for name in ("FOR_ITER_GEN", "SEND_GEN"):
            handler = emitted(name)
            self.assertNotIn("PyValue new_frame;", handler)
            self.assertNotIn("_PyEval_CompleteForIter(", handler)
            self.assertNotIn("_PyEval_CompleteSend(", handler)

        marker = "IR_COMPLETE_STEP(_PY_IR_FOR_ITER, completed, oparg);"
        Path(self.temp_input_filename).write_text(source.replace(
            marker, "IR_COMPLETE_STEP(unknown_kind, completed, oparg);", 1))
        analysis = analyzer.analyze_files([self.temp_input_filename])
        formats = ir_generator.compute_formats(analysis)
        with self.assertRaisesRegex(SyntaxError, "unsupported completion kind"):
            emitted("FOR_ITER")

    def test_exact_coroutine_await_transfers_existing_owner(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()
        Path(self.temp_input_filename).write_text(source)
        analysis = analyzer.analyze_files([self.temp_input_filename])
        formats = ir_generator.compute_formats(analysis)
        instruction = analysis.instructions["GET_AWAITABLE"]
        self.assertEqual(formats["GET_AWAITABLE"].borrow_mask, 1)
        self.assertTrue(formats["GET_AWAITABLE"].store_dst)
        self.assertFalse(formats["GET_AWAITABLE"].no_fold)
        # Declared `iter: owned`: the transferred reference is the output's
        # own, so it aliases no input
        self.assertEqual(instruction.parts[0].aliased_outputs, ())
        predicate = "coro_awaitable_can_transfer"
        # The row declares no reentry, so the call does not escape
        self.assertFalse(ir_contract.rows()[predicate].effects & ir_contract.REENTRY)
        self.assertFalse(analyzer.call_escapes(predicate))
        self.assertNotIn(predicate, analyzer.IR_MARKER_EFFECTS)
        effect = ir_generator.analyze_error_effects(self.analyze(
            "\ninst(OP, (value --)) { if (coro_awaitable_can_transfer(value)) { assert(true); } }\n"
        ).instructions["OP"])
        self.assertIsNotNone(effect)
        self.assertFalse(effect.callback)
        self.assertFalse(effect.may_fail)
        self.assertEqual(effect.homes, ir_generator.HomeEffect.NO_ARENA_WRITE)
        with self.subTest():
            output = io.StringIO()
            out = CWriter(output, 0, False)
            ir_generator.generate_handler("GET_AWAITABLE", instruction,
                formats["GET_AWAITABLE"], ir_generator.IREmitter(out, analysis, formats),
                out)
            handler = output.getvalue()
            start = handler.index("if (coro_awaitable_can_transfer(iterable))")
            end = handler.index("}\n", start)
            arm = handler[start:end]
            self.assertIn("IR_DETACH(ir_word, B, iterable)", arm)
            self.assertIn("iter = IR_TAKEN_REQUIRED(iterable, ir_own_B)", arm)
            self.assertNotIn("_PyEval_GetAwaitable", arm)
            self.assertNotIn("PyValue_FromObjectSteal", arm)
            self.assertEqual(handler.count("_PyEval_GetAwaitable("), 1)
            self.assertIn("IR_STORE_DST(ir_word, A, iter);", handler)
        body = instruction.parts[0].body.body[0]
        self.assertIsInstance(body, ir_generator.IfStmt)
        fallback = [ir_generator._statement_text(stmt) for stmt in body.else_body.body]
        expected = ir_generator.Parser("""{
            PyObject *iter_o = _PyEval_GetAwaitable(iterable, oparg);
            PyValue_CLOSE(iterable);
            ERROR_IF(iter_o == NULL);
            iter = PyValue_FromObjectSteal(iter_o);
        }""", "<unchanged await fallback>").block()
        self.assertEqual(fallback, [ir_generator._statement_text(stmt) for stmt in expected.body])

    def test_alias_output_meets_no_fresh_owner(self):
        # An output declared moved(x) is x itself on every path. A result
        # that takes the input on one arm and holds a new owner on another
        # is owned, so its dst may name a local.
        source = """
        inst(OP, (value: consumed -- result: %s)) effects(runs_python) exits(next, raise) {
            if (coro_awaitable_can_transfer(value)) {
                result = TAKE(value);
            }
            else {
                PyObject *pointer = _PyEval_GetAwaitable(value, 0);
                PyValue_CLOSE(value);
                ERROR_IF(pointer == NULL);
                result = PyValue_FromObjectSteal(pointer);
            }
        }
        """
        def generate(declaration):
            analysis = self.analyze(source % declaration)
            instruction = analysis.instructions["OP"]
            formats = ir_generator.compute_formats(analysis)
            output = io.StringIO()
            out = CWriter(output, 0, False)
            ir_generator.generate_handler("OP", instruction, formats["OP"],
                ir_generator.IREmitter(out, analysis, formats), out)
            return instruction, formats["OP"], output.getvalue()
        with self.assertRaisesRegex(SyntaxError, "declared moved\\(value\\).*no input's value"):
            generate("moved(value)")
        instruction, fmt, handler = generate("owned")
        self.assertEqual(instruction.parts[0].aliased_outputs, ())
        self.assertTrue(fmt.store_dst)
        self.assertFalse(fmt.no_fold)
        self.assertIn("IR_STORE_DST(ir_word, A, result);", handler)

    def test_generator_entry_recipient_uses_actual_terminal(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()
        Path(self.temp_input_filename).write_text(source)
        analysis = analyzer.analyze_files([self.temp_input_filename])
        formats = ir_generator.compute_formats(analysis)
        for name, kind in (("FOR_ITER_GEN", "_PY_IR_FOR_ITER"),
                           ("SEND_GEN", "_PY_IR_SEND")):
            instruction = analysis.instructions[name]
            recipe = ir_generator.generator_entry_recipe(instruction)
            self.assertIsNotNone(recipe)
            self.assertEqual(recipe.kind, kind)
            with self.subTest(name=name):
                output = io.StringIO()
                out = CWriter(output, 0, False)
                emitter = ir_generator.IREmitter(out, analysis, formats)
                ir_generator.generate_handler(name, instruction, formats[name],
                                              emitter, out)
                handler = output.getvalue()
                self.assertNotIn("if (IR_IS_LOCAL_FIELD(ir_word, A)", handler)
                self.assertIn("frame->return_slot =", handler)
                self.assertIn(f"{kind} == _PY_IR_SEND ? IR_B(ir_word) + 2 : IR_A(ir_word)",
                              handler)
                self.assertLess(handler.index("frame->return_slot ="),
                                handler.index("frame = tstate->current_frame = ir_resumed_frame;"))
                self.assertIn("_PyFrame_SetResumeValue(frame, ir_resume_input)", handler)
                self.assertNotIn("_PyValue_Publish", handler)
        instruction = analysis.instructions["FOR_ITER_GEN"]
        recipe = ir_generator.generator_entry_recipe(instruction)
        self.assertEqual(ir_generator.store_mode(
            instruction, ir_generator.external_io(instruction)[1]), ir_generator.STORE_RETURN)
        self.assertIsNone(ir_generator.generator_entry_recipe(analysis.instructions["SEND"]))
        terminal = "IR_ENTER_GENERATOR(gen, _PY_IR_FOR_ITER);"
        changes = (
            (terminal, "IR_ENTER_GENERATOR(gen, _PY_IR_SEND);"),
            (terminal, terminal + " STAT_INC(FOR_ITER, hit);"),
            (terminal, "if (flag) { " + terminal + " }"),
            (terminal, terminal + terminal),
            (terminal, "opaque(); " + terminal),
            (terminal, "frame->return_slot = 0; " + terminal),
        )
        # Use the real parsed body text so token spacing cannot hide a suffix.
        original = source[source.index("op(_FOR_ITER_GEN_FRAME"):
                          source.index("macro(FOR_ITER_GEN)")]
        original = original[original.index("{"):original.rindex("}") + 1]
        for old, new in changes:
            with self.subTest(change=new):
                changed = source.replace(original, original.replace(old, new), 1)
                Path(self.temp_input_filename).write_text(changed)
                mutated = analyzer.analyze_files([self.temp_input_filename])
                self.assertIsNone(ir_generator.generator_entry_recipe(
                    mutated.instructions["FOR_ITER_GEN"]))
        renamed = re.sub(r"\bgen\b", "child", original)
        Path(self.temp_input_filename).write_text(source.replace(original, renamed, 1))
        mutated = analyzer.analyze_files([self.temp_input_filename])
        self.assertEqual(ir_generator.generator_entry_recipe(
            mutated.instructions["FOR_ITER_GEN"]).generator, "child")
        with mock.patch.object(recipe.part.stack.outputs[0], "name", "changed"):
            self.assertIsNone(ir_generator.generator_entry_recipe(instruction))
        guard = next(part for part in instruction.parts
                     if isinstance(part, analyzer.Uop) and part is not recipe.part)
        for prefix in ("frame->return_slot = 0;", "opaque();",
                       "next_instr = this_instr;", "PyValue *alias = slots;"):
            with self.subTest(prefix=prefix):
                parsed = ir_generator.Parser(
                    "{" + prefix + " DEOPT_IF(IS_PEP523_HOOKED(tstate));}",
                    "<entry prefix>").block()
                with mock.patch.object(guard, "body", parsed):
                    self.assertIsNone(ir_generator.generator_entry_recipe(instruction))
        # Guard-only inspection must not grant an ordinary error-route summary.
        guard_instruction = ir_generator.replace(instruction, parts=[guard])
        self.assertIsNone(ir_generator.analyze_error_effects(guard_instruction))

    def test_a_suspension_is_declared(self):
        # The frame switch of a body that names none (`frame = ...`) takes
        # the switch exits its uop declares; the generator state the body
        # writes decides nothing
        def switches(inst):
            return [t.kind for part in inst.parts if isinstance(part, analyzer.Uop)
                    for t in ir_records.uop_transfers(part, "")
                    if t.kind in (ir_records.ENTER_FRAME, ir_records.YIELD)]
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()
        Path(self.temp_input_filename).write_text(source)
        analysis = analyzer.analyze_files([self.temp_input_filename])
        for name in ("YIELD_VALUE", "INSTRUMENTED_YIELD_VALUE"):
            self.assertEqual(switches(analysis.instructions[name]), [ir_records.YIELD])
        declared = "effects(reads, writes, switches_frame) exits(yield)"
        self.assertEqual(source.count(declared), 1)
        source = source.replace(declared, "effects(reads, writes, switches_frame) exits(enter)")
        Path(self.temp_input_filename).write_text(source)
        mutated = analyzer.analyze_files([self.temp_input_filename])
        for name in ("YIELD_VALUE", "INSTRUMENTED_YIELD_VALUE"):
            self.assertEqual(switches(mutated.instructions[name]),
                             [ir_records.ENTER_FRAME])

    def test_the_guarded_call_prefix_is_declared(self):
        # The binding plan relies on the callable's declared facts and on
        # the prefix's declared guard reasons, not on the guards' bodies
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()

        def validate(text):
            Path(self.temp_input_filename).write_text(text)
            with handle_stderr():
                analysis = analyzer.analyze_files([self.temp_input_filename])
            formats = ir_generator.compute_formats(analysis)
            ir_generator.validate_guarded_python_call(
                analysis.instructions["CALL_KW_PY"], formats["CALL_KW_PY"])

        validate(source)
        version = ("-- callable: exact(PyFunction_Type) func_version(func_version), "
                   "unused, unused[oparg], unused))")
        for old, new in [('"the recursion budget is spent"', '"spam"'),
                         (version, version.replace(" func_version(func_version)", "")),
                         ("op(_CHECK_PEP_523, (--))\n            effects(reads)",
                          "op(_CHECK_PEP_523, (--))\n            effects(reads, writes)")]:
            with self.subTest(old):
                self.assertEqual(source.count(old), 1)
                with self.assertRaisesRegex(SyntaxError, "requires the checked prefix"):
                    validate(source.replace(old, new))

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
        for name in ("LOAD_SUPER_ATTR_METHOD",):
            with self.subTest(name=name):
                self.assertEqual(contracts[name].output_kind, "_PyIR_OUTPUT_WINDOW")
                handler = self.handler(output.getvalue(), name)
                self.assertIn("IR_SET_TEMP(window[", handler)
                self.assertNotIn("IR_MOVE_TO_DST", handler)
        for name in ("LOAD_SUPER_ATTR", "INSTRUMENTED_LOAD_SUPER_ATTR"):
            with self.subTest(name=name):
                self.assertEqual(contracts[name].output_kind,
                                 "_PyIR_OUTPUT_WINDOW_DST_FIRST")
                handler = self.handler(output.getvalue(), name)
                self.assertIn("IR_SET_TEMP(window[0], attr);", handler)
                self.assertIn("IR_MOVE_TO_DST(ir_word, A, window[0]);", handler)
                self.assertLess(handler.index("IR_MOVE_TO_DST("),
                                handler.index("PyValue_XCLOSE(ir_old);"))
                self.assertEqual(formats[name].dst_offset, 0)
        # A call reads its inputs from operand words and writes its result to
        # the destination register; it has no window.
        self.assertEqual(contracts["CALL"].output_kind,
                         "_PyIR_OUTPUT_REGISTER_DST")
        self.assertEqual(contracts["FOR_ITER_GEN"].output_kind,
                         "_PyIR_OUTPUT_OUTCOME")
        with mock.patch.object(ir_generator, "store_mode",
                               return_value=ir_generator.STORE_LEAVE):
            with self.assertRaisesRegex(SyntaxError, "unclassified window output"):
                ir_generator.handler_output_kind(
                    analysis.instructions["LOAD_SUPER_ATTR_METHOD"],
                    formats["LOAD_SUPER_ATTR_METHOD"])

    def test_generator_resume_commits_before_input_disposal(self):
        source = (Path(__file__).resolve().parents[2] /
                  "Python" / "bytecodes.c").read_text()
        Path(self.temp_input_filename).write_text(source)
        analysis = analyzer.analyze_files([self.temp_input_filename])
        for name in ("FOR_ITER", "SEND"):
            self.assertTrue(analysis.instructions[name].properties.jumps)
            self.assertTrue(analysis.instructions[name].properties.error_without_pop)
        # The stack instruction falls through; the IR word's jump exit is
        # declared by IR_EXIT
        self.assertFalse(analysis.instructions["CLEANUP_THROW"].properties.jumps)
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
                consumed = handler.index("_PyFrame_SetResumeValue(frame, ir_resume_input);")
                self.assertLess(entered, consumed)
                if name != "FOR_ITER_GEN":
                    detached = handler.index("IR_SLOT(IR_B(ir_word) + 2) = PyValue_NULL;")
                    self.assertLess(detached, entered)
        handler = self.handler(output.getvalue(), "CLEANUP_THROW")
        self.assertIn("_PyFrame_InstrAt(frame, oparg)", handler)
        self.assertNotIn("next_instr + oparg", handler)

    def run_ir_test(self, input: str, expected: str):
        self.assertEqual(self.ir_output(input).strip(), expected.strip())

    def test_register_style(self):
        # Inputs that are only used as objects are read through their fields
        # and may be borrowed locals.  (The result of a call that runs no
        # code would be followed as derived from the objects, S7.)
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            HELPER(spam_call, PyObject *, (PyObject *, PyObject *), \\
                   ARGS(object, object), VALUE_OR_NULL(owned), EFFECTS(reads, allocates, raises))
        """)
        input = """
        inst(OP, (left: consumed object, right: consumed object -- res: owned))
            effects(reads, allocates, raises, releases) exits(next, raise)
        {
            assert(PyValue_IsObject(left));
            assert(PyValue_IsObject(right));
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
            IR_HOLD_NEXT();
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            PyValue ir_old = PyValue_NULL;
            (void)ir_old;
            PyValueBorrow left;
            PyValueBorrow right;
            PyValue res;
            PyValue ir_own_B = PyValue_NULL;
            (void)ir_own_B;
            PyValue ir_own_C = PyValue_NULL;
            (void)ir_own_C;
            right = IR_VIEW(IR_C(ir_word));
            left = IR_VIEW(IR_B(ir_word));
            assert(PyValue_IsObject(left));
            assert(PyValue_IsObject(right));
            PyObject *l = _PyValue_AsKnownObjectBorrowPointer(left);
            PyObject *r = _PyValue_AsKnownObjectBorrowPointer(right);
            PyObject *o = spam_call(l, r);
            ir_own_C = IR_DETACH(ir_word, C, right);
            ir_own_B = IR_DETACH(ir_word, B, left);
            if (o == NULL) {
                _PyValue_XCLOSE_POINTER(ir_own_C);
                _PyValue_XCLOSE_POINTER(ir_own_B);
                PROCESS_ERROR_AT(RAISE, 0, this_instr);
            }
            res = PyValue_FromObjectSteal(o);
            IR_STORE_DST(ir_word, A, res);
            _PyValue_XCLOSE_POINTER(ir_own_C);
            _PyValue_XCLOSE_POINTER(ir_own_B);
            PyValue_XCLOSE(ir_old);
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
        # SPAM reads the object (a view of the input), SPAM_KEEP keeps its
        # argument as the result
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            HELPER(SPAM, PyValue, (PyObject *), \\
                   ARGS(object), VALUE(owned), EFFECTS(reads)) \\
            HELPER(SPAM_RAW, PyValue, (bool), \\
                   ARGS(scalar), VALUE(owned), EFFECTS()) \\
            HELPER(SPAM_KEEP, PyValue, (PyValue), \\
                   ARGS(consumed), VALUE(moved(0)), EFFECTS())
        """)
        input = """
        inst(OP_PASS, (value: object -- value, res: owned))
            effects(reads) exits(next)
        {
            assert(PyValue_IsObject(value));
            res = SPAM(PyValue_AsKnownObjectBorrow(value));
        }
        inst(OP_OBJ, (value: consumed object -- res: owned))
            effects(reads, releases) exits(next)
        {
            assert(PyValue_IsObject(value));
            res = SPAM(PyValue_AsKnownObjectBorrow(value));
            PyValue_CLOSE(value);
        }
        inst(OP_FAMILY, (value: consumed object -- res: owned))
            effects(reads, releases) exits(next)
        {
            assert(PyValue_IsObject(value));
            res = SPAM(PyValue_AsKnownObjectBorrow(value));
            PyValue_CLOSE(value);
        }
        inst(OP_FAMILY_RAW, (value: consumed -- res: owned))
            effects(releases) exits(next)
        {
            res = SPAM_RAW(PyValue_IsNone(value));
            PyValue_CLOSE(value);
        }
        family(OP_FAMILY, 0) = { OP_FAMILY_RAW };
        inst(OP_KEEP, (value: consumed -- res: moved(value)))
            effects() exits(next)
        {
            res = SPAM_KEEP(value);
        }
        inst(OP_STORE, (value: consumed --)) effects() exits(next) {
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
        # SPAM reads the object; SPAM_OWNED consumes a reference and returns
        # another
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            PRIMITIVE(SPAM, bool, (PyObject *), \\
                      ARGS(object), SCALAR, EFFECTS(reads)) \\
            HELPER(SPAM_OWNED, PyObject *, (PyObject *), \\
                   ARGS(consumed object), VALUE_OR_NULL(owned), EFFECTS(allocates, raises))
        """)
        input = """
        inst(OP_CLOSE, (value: consumed object --)) effects(reads, releases) exits(next) {
            assert(PyValue_IsObject(value));
            SPAM(PyValue_AsKnownObjectBorrow(value));
            PyValue_CLOSE(value);
        }
        inst(OP_HELD, (value: consumed -- res: moved(value))) effects() exits(next) {
            res = TAKE(value);
        }
        inst(OP_STEAL, (value: consumed object -- res: owned))
            effects(allocates, raises) exits(next, raise)
        {
            assert(PyValue_IsObject(value));
            PyObject *o = PyValue_AsKnownObjectSteal(value);
            PyObject *n = SPAM_OWNED(o);
            ERROR_IF(n == NULL);
            res = PyValue_FromObjectSteal(n);
        }
        inst(OP_ALIAS, (value: consumed object -- res: moved(value) encoding(null|object)))
            effects(reads, releases) exits(next)
        {
            assert(PyValue_IsObject(value));
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
        # The effect reads the view, then the owner is detached and closed
        self.assertLess(close.index("SPAM("),
                        close.index("ir_own_A = IR_DETACH(ir_word, A, value);"))
        self.assertLess(close.index("ir_own_A = IR_DETACH(ir_word, A, value);"),
                        close.index("_PyValue_XCLOSE_POINTER(ir_own_A);"))
        # No release of a view
        self.assertNotRegex(output, r"\n\s*(_?PyValue_\w*CLOSE\w*)\(value")
        self.assertIn("ir_own_B = IR_DETACH(ir_word, B, value);\n"
                      "            res = IR_TAKEN_REQUIRED(value, ir_own_B);",
                      self.handler(output, "OP_HELD"))
        self.assertIn("_PyValue_AsPyObjectStealPointer(IR_TAKEN_POINTER(value, ir_own_B))",
                      self.handler(output, "OP_STEAL"))
        alias = self.handler(output, "OP_ALIAS")
        self.assertIn("res = IR_TAKEN_POINTER(value, ir_own_B);", alias)
        self.assertEqual(alias.count("IR_DETACH("), 2)
        self.assertNotIn("TAKE(", output.replace("IR_TAKEN(", ""))
        self.assertNotIn("OWNER(", output)

    def test_view_stored_refused(self):
        # A view has no count, so no slot or home may hold it: an output that
        # a handler stores is assigned TAKE() of it
        input = """
        inst(OP, (value: consumed -- res: moved(value))) effects() exits(next) {
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
        op(_FIRST, (value: consumed -- copy: moved(value))) effects() exits(next) {
            copy = value;
            DEAD(value);
        }
        op(_SECOND, (copy: consumed -- res: moved(copy))) effects() exits(next) {
            res = TAKE(copy);
        }
        macro(OP) = _FIRST + _SECOND;
    """
        output = self.handler(self.ir_output(input), "OP")
        self.assertEqual(output.count("IR_DETACH("), 1)
        consume = output.index(
            "ir_own_B = IR_DETACH(ir_word, B, copy);")
        self.assertLess(output.index("// _FIRST"), consume)
        self.assertLess(consume, output.index("// _SECOND"))
        self.assertIn("res = IR_TAKEN_REQUIRED(copy, ir_own_B);", output)
        self.assertIn("PyValueBorrow copy;", output)

    def test_taken_view_form_follows_nullable_and_pointer_facts(self):
        # A taken input tests for NULL only when the instruction declares it
        # nullable, and tests for an inline value only when the instruction
        # does not declare the input an object (assumed(object): an assert
        # in the body is no evidence)
        input = """
        inst(OP_NULLABLE, (value: consumed nullable -- res: moved(value) nullable))
            effects() exits(next)
        {
            res = TAKE(value);
        }
        inst(OP_REQUIRED, (value: consumed -- res: moved(value))) effects() exits(next) {
            res = TAKE(value);
        }
        inst(OP_OBJECT, (value: consumed assumed(object) by SPAM -- res: moved(value)))
            effects() exits(next)
        {
            res = TAKE(value);
        }
    """
        output = self.ir_output(input)
        self.assertIn("res = IR_TAKEN(value, ir_own_B);", self.handler(output, "OP_NULLABLE"))
        self.assertIn("res = IR_TAKEN_REQUIRED(value, ir_own_B);", self.handler(output, "OP_REQUIRED"))
        self.assertIn("res = IR_TAKEN_POINTER(value, ir_own_B);", self.handler(output, "OP_OBJECT"))

    def test_no_addressable_locals(self):
        # The lint: no uop or label body takes the address of a local it
        # declares or of a stack item that is not an array
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            PRIMITIVE(spam_call, int, (void *), ARGS(scalar), SCALAR, EFFECTS())
        """)
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
            # (ir_displaced is a local of a ceval_macros.h macro, not of the
            # handler the generator writes)
            ("&ir_rest", "spam_call(&ir_rest);"),
            ("&call_state", "spam_call(&call_state);"),
            ("&ir_own_B", "spam_call(&ir_own_B);"),
        ]
        for site, body in bad:
            with self.subTest(site=site):
                input = f"""
        inst(OP, (value: closed --)) effects(reads) exits(next) {{
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
        inst(OP, (value: closed --)) effects(reads) exits(next) {{
            {body}
            DEAD(value);
        }}
    """
                self.ir_output(input)

    def test_value_operation_by_value(self):
        # R-a, R-b: the helpers of a value operation take the handler's
        # values, its frame and instruction, and slot numbers from the word;
        # there is no operation record
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            HELPER(spam_call, PyValue, (PyValueBorrow, PyValueBorrow), \\
                   ARGS(borrowed, borrowed), VALUE(owned null_iff(0)), EFFECTS(runs_python)) \\
            MACRO(IR_RESTORE_VALUE_INPUTS, ARGS(lvalue, lvalue), NONE, EFFECTS(), EXITS()) \\
            HELPER(value_operation_close3, void, (PyValue, PyValue, PyValue), \\
                   ARGS(consumed, consumed, consumed), NONE, EFFECTS(releases)) \\
            MACRO(IR_ABORT_VALUE_READER, ARGS(), NONE, EFFECTS(), EXITS(abort)) \\
            MACRO(IR_COMPLETE_VALUE_READER, ARGS(lvalue, consumed, consumed), NONE, \\
                  EFFECTS(runs_python, releases), EXITS())
        """)
        input = """
        inst(OP, (left: consumed, right: consumed -- res: owned))
            effects(runs_python, releases) exits(next, abort)
        {
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
                      "frame, held_left, held_right, IR_B(ir_word), IR_C(ir_word));\n"
                      "                    held_left = ir_rest.first;\n"
                      "                    held_right = ir_rest.second;", output)
        self.assertIn("(void)value_operation_finish(tstate, frame, this_instr, ", output)
        self.assertIn("pending = value_reader_complete(tstate, frame, this_instr, "
                      "NULL, IR_A(ir_word), pending, held_right, held_left);\n"
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
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            MACRO(IR_RETIRE_PUBLISHED, ARGS(consumed), NONE, EFFECTS(), EXITS()) \\
            HELPER(spam_call, int, (), ARGS(), SCALAR, EFFECTS(runs_python))
        """)
        input = """
        inst(OP, (value: consumed --)) effects() exits(next) {
            PyValue held = OWNER(value);
            IR_RETIRE_PUBLISHED(held);
        }
    """
        output = self.handler(self.ir_output(input), "OP")
        self.assertIn("ir_own_A = IR_DETACH(ir_word, A, value);\n"
                      "            PyValue held = ir_own_A;", output)
        # The retired owner is closed, once, after the detach
        self.assertEqual(output.count("PyValue_XCLOSE(ir_close1);"), 1)
        self.assertLess(output.index("IR_DETACH("),
                        output.index("PyValue_XCLOSE(ir_close1);"))
        self.assertNotIn("retire_published_values", output)
        self.assertNotIn("&held", output)
        gap = """
        inst(OP, (value: consumed --)) effects(runs_python) exits(next) {
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
        inst(OP, (value: consumed --)) effects(reads, releases) exits(next) {
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
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            HELPER(spam_call, int, (PyObject *), \\
                   ARGS(scalar), SCALAR, EFFECTS(runs_python, raises)) \\
            HELPER(SPAM, PyValue, (PyObject *), \\
                   ARGS(scalar), VALUE(owned), EFFECTS(reads)) \\
            HELPER(SPAM_PAIR, PyValue, (Py_ssize_t, PyObject *), \\
                   ARGS(scalar, scalar), VALUE(owned), EFFECTS(reads))
        """)
        # SPAM_PAIR reads its arguments, as SPAM does
        readers = mock.patch.object(ir_generator, "OBJECT_READERS",
                                    ir_generator.OBJECT_READERS | {"SPAM_PAIR"})
        readers.start()
        self.addCleanup(readers.stop)
        # The error exit closes the result, which the body has assigned
        # before it tests the call's status
        raised = "if (err) { PyValue_CLOSE(res); ERROR_NO_POP(); }"
        for body in ("int err = spam_call(PyValue_AsKnownObjectBorrow(value));\n"
                     "res = TAKE(value);\n" + raised,
                     "PyObject *o = PyValue_AsKnownObjectBorrow(value);\n"
                     "int err = spam_call(NULL);\n"
                     "res = SPAM(o);\n"
                     "PyValue_CLOSE(value);\n" + raised,
                     # A cast of the object is followed too
                     "PyTupleObject *t = (PyTupleObject *)"
                     "PyValue_AsKnownObjectBorrow(value);\n"
                     "int err = spam_call(NULL);\n"
                     "res = SPAM(t);\n"
                     "PyValue_CLOSE(value);\n" + raised,
                     # So are pointers derived from the object: its type,
                     # and an address inside it
                     "PyTypeObject *tp = Py_TYPE(PyValue_AsKnownObjectBorrow(value));\n"
                     "int err = spam_call(NULL);\n"
                     "res = SPAM(tp->tp_name);\n"
                     "PyValue_CLOSE(value);\n" + raised,
                     "PyObject **t = (PyObject **)"
                     "((char *)PyValue_AsKnownObjectBorrow(value) + 8);\n"
                     "int err = spam_call(NULL);\n"
                     "res = SPAM(*t);\n"
                     "PyValue_CLOSE(value);\n" + raised,
                     # through an assignment, or a type that may be a pointer
                     "PyTypeObject *tp;\n"
                     "tp = Py_TYPE(PyValue_AsKnownObjectBorrow(value));\n"
                     "int err = spam_call(NULL);\n"
                     "res = SPAM(tp->tp_name);\n"
                     "PyValue_CLOSE(value);\n" + raised,
                     "getattrofunc f = "
                     "Py_TYPE(PyValue_AsKnownObjectBorrow(value))->tp_getattro;\n"
                     "int err = spam_call(NULL);\n"
                     "res = SPAM(f);\n"
                     "PyValue_CLOSE(value);\n" + raised,
                     # C does not order the operands of an expression: the
                     # view may be read after the call
                     "res = SPAM_PAIR(spam_call(NULL), PyValue_AsKnownObjectBorrow(value));\n"
                     "PyValue_CLOSE(value);\n"
                     "if (oparg) { PyValue_CLOSE(res); ERROR_NO_POP(); }"):
            with self.subTest(body=body):
                with self.assertRaisesRegex(SyntaxError, r"\(S7\)"):
                    self.ir_output(f"""
                    inst(OP, (value: consumed object -- res: {"moved(value)" if "TAKE" in body else "owned"}))
                    effects(reads, runs_python, raises, releases) exits(next, raise) {{
                        assert(PyValue_IsObject(value));
                        {body}
                    }}
                """)
        # A release after the call reads the owner, and a TAKE before the
        # call keeps the object alive
        output = self.handler(self.ir_output("""
        inst(OP, (value: consumed object -- res: owned))
            effects(reads, runs_python, raises, releases) exits(next, raise)
        {
            assert(PyValue_IsObject(value));
            PyObject *o = PyValue_AsKnownObjectBorrow(value);
            PyValue held = TAKE(value);
            int err = spam_call(o);
            res = SPAM(o);
            PyValue_CLOSE(held);
            if (err) { PyValue_CLOSE(res); ERROR_NO_POP(); }
        }
        inst(OP_RELEASE, (value: consumed object -- res: owned))
            effects(reads, runs_python, raises, releases) exits(next, raise)
        {
            assert(PyValue_IsObject(value));
            int err = spam_call(PyValue_AsKnownObjectBorrow(value));
            PyValue_CLOSE(value);
            ERROR_IF(err);
            res = PyValue_None;
        }
    """), "OP")
        self.assertLess(output.index("PyValue held = IR_TAKEN_POINTER(value, ir_own_B);"),
                        output.index("spam_call("))
        # The designator and the arguments of the call are read before it
        # runs, and a number read from the object does not dangle
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            SLOT(binaryfunc, PyObject *, (PyObject *, PyObject *), \\
                 ARGS(object, object), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \\
            FIELD(PyTypeObject, tp_as_number, PyNumberMethods *, ON(type), \\
                  PATH(number_methods), EFFECTS(reads)) \\
            FIELD(PyNumberMethods, nb_add, binaryfunc, ON(number_methods), \\
                  SLOT(binaryfunc), EFFECTS(reads))
        """)
        input = """
        inst(OP, (value: consumed object -- res: owned))
            effects(reads, runs_python, raises, releases) exits(next)
        {
            assert(PyValue_IsObject(value));
            res = SPAM(spam_call(PyValue_AsKnownObjectBorrow(value)));
            PyValue_CLOSE(value);
        }
        inst(OP_POINTER, (value: consumed object -- res: owned))
            effects(reads, runs_python, raises, releases) exits(next, raise)
        {
            assert(PyValue_IsObject(value));
            PyObject *o = PyValue_AsKnownObjectBorrow(value);
            binaryfunc action = Py_TYPE(o)->tp_as_number->nb_add;
            Py_ssize_t n = Py_TYPE(o)->tp_basicsize;
            PyObject *r = action(o, o);
            PyValue_CLOSE(value);
            ERROR_IF(r == NULL);
            (void)n;
            res = PyValue_FromObjectSteal(r);
        }
    """
        formats = self.formats(input)
        for name in ("OP", "OP_POINTER"):
            with self.subTest(name=name):
                self.assertEqual(formats[name].borrow_mask, 1)
        self.assertIn("res = SPAM(spam_call(_PyValue_AsKnownObjectBorrowPointer(value)));",
                      self.handler(self.ir_output(input), "OP"))

    def test_owner_taken_once(self):
        # A view and its copy read one owner: a take or release of one of
        # them spends it and kills both, so a second would count it twice
        double = """
        op(_COPY, (value -- value, copy: borrowed)) effects() exits(next) {
            copy = value;
        }
        op(_USE, (value: borrowed, copy: consumed -- %s)) effects(releases) exits(next) {
            res = TAKE(copy);
            %s
        }
        macro(OP) = _COPY + _USE;
    """
        # The contract refuses the second spend before the generator's
        # check: the view is declared borrowed, which no release or take
        # of it may spend, and the copy is spent by the first take
        for second, message in (
            ("PyValue_CLOSE(value);", "consumes `value`, declared borrowed"),
            ("PyValue_CLOSE(copy);", "consumes `copy`, which is already consumed or moved"),
            ("res2 = TAKE(value);", "consumes `value`, declared borrowed"),
        ):
            with self.subTest(second=second):
                outputs = ("res: moved(copy), res2: moved(value)" if "res2" in second
                           else "res: moved(copy)")
                with self.assertRaisesRegex(SyntaxError, message):
                    self.ir_output(double % (outputs, second))
        # DECREF_INPUTS releases no input that a take killed (the contract
        # asks for DEAD() of it first, and of the view as well if it is not
        # consumed)
        output = self.handler(self.ir_output(
            double.replace("value: borrowed, copy", "value: consumed, copy")
            % ("res: moved(copy)", "DEAD(copy);\n            DECREF_INPUTS();")), "OP")
        self.assertNotIn("XCLOSE(ir_own_B)", output)
        self.assertEqual(output.count("ir_own_B"),
                         output.count("ir_own_B = IR_DETACH") + 3)
        # Exclusive branches spend it once each
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            HELPER(SPAM, PyValue, (PyObject *), \\
                   ARGS(object), VALUE(owned), EFFECTS(reads))
        """)
        output = self.handler(self.ir_output("""
        inst(OP, (value: consumed object -- res: owned))
            effects(reads, releases) exits(next)
        {
            assert(PyValue_IsObject(value));
            if (SPAM_FLAG()) {
                res = TAKE(value);
            }
            else {
                res = SPAM(PyValue_AsKnownObjectBorrow(value));
                PyValue_CLOSE(value);
            }
        }
    """), "OP")
        self.assertIn("res = IR_TAKEN_POINTER(value, ir_own_B);", output)
        self.assertIn("_PyValue_XCLOSE_POINTER(ir_own_B);", output)
        # The take kills every view and copy of the input: one is not read
        # after it, and none needs DEAD()
        copy = """
        op(_COPY, (value -- value, copy: borrowed)) effects() exits(next) {
            copy = value;
        }
        op(_USE, (value: closed, copy: consumed -- res: moved(copy)))
            effects() exits(next)
        {
            res = TAKE(copy);
            %s
        }
        macro(OP) = _COPY + _USE;
    """
        for read in ("int x = PyValue_IsNone(value);\n(void)x;",
                     "if (PyValue_IsNone(value)) {\nSPAM_FLAG();\n}"):
            with self.subTest(read=read):
                with self.assertRaisesRegex(SyntaxError,
                                            "view 'value' is read after the "
                                            "body took or released"):
                    self.ir_output(copy % read)
        for rest in ("", "DEAD(value);"):
            with self.subTest(rest=rest):
                output = self.handler(self.ir_output(copy % rest), "OP")
                self.assertIn("res = IR_TAKEN_REQUIRED(copy, ir_own_B);", output)

    def test_exit_after_take_refused(self):
        # A deopt or exit goes to an instruction that reads the inputs from
        # their slots, which a take cleared, and DISPATCH_SAME_OPARG runs
        # the handler again
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            HELPER(SPAM, PyValue, (PyObject *), \\
                   ARGS(object), VALUE(owned), EFFECTS(reads)) \\
            PRIMITIVE(SPAM_FLAG, bool, (), \\
                      ARGS(), CONTEXT("a flag the handler tests"), EFFECTS())
        """)
        # The contract refuses it before the generator does: its message is
        # that the deopt follows a take
        with self.assertRaisesRegex(SyntaxError,
                                    "the deopt follows TAKE consuming `value`, "
                                    "so the word cannot restart"):
            self.ir_output("""
            inst(OP, (value: consumed object -- res: owned))
                effects(reads, releases) exits(next)
            {
                assert(PyValue_IsObject(value));
                res = SPAM(PyValue_AsKnownObjectBorrow(value));
                PyValue_CLOSE(value);
            }
            inst(OP_SPECIALIZED, (value: consumed -- res: owned))
                effects() exits(next, deopt)
            {
                res = TAKE(value);
                DEOPT_IF(SPAM_FLAG());
            }
            family(OP, 0) = { OP_SPECIALIZED };
        """)
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            HELPER(SPAM_KEEP, void, (PyValue), \\
                   ARGS(consumed), NONE, EFFECTS(releases))
        """)
        for consume in ("PyValue held = TAKE(value);\nSPAM_KEEP(held);",
                        "taken = value;\nDEAD(value);"):
            with self.subTest(consume=consume):
                with self.assertRaisesRegex(SyntaxError,
                                            "DISPATCH_SAME_OPARG after the "
                                            "handler took"):
                    self.ir_output(f"""
                    op(_SPECIALIZE_OP, (value: consumed -- taken: moved(value)))
                        effects(releases) exits(next, jump)
                    {{
                        if (SPAM_FLAG()) {{
                            {consume}
                            DISPATCH_SAME_OPARG();
                        }}
                        taken = value;
                        DEAD(value);
                    }}
                    op(_OP, (taken: consumed object -- res: owned))
                        effects(reads, releases) exits(next)
                    {{
                        res = SPAM(PyValue_AsKnownObjectBorrow(taken));
                        PyValue_CLOSE(taken);
                    }}
                    macro(OP) = _SPECIALIZE_OP + _OP;
                """)

    def test_error_exits_refused(self):
        # An error exit between OWNER() and its retire leaks the owner, and
        # one after a TAKE() into an output leaks the count: the generator
        # stores no output on an error exit
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            MACRO(IR_RETIRE_PUBLISHED, ARGS(consumed), NONE, EFFECTS(), EXITS())
        """)
        # The contract refuses both leaks before the generator does (the
        # generator's own messages: "ERROR_NO_POP between OWNER() and
        # IR_RETIRE_PUBLISHED would leak 'held'" and "ERROR_IF while output
        # 'res' holds a TAKE() result")
        with self.assertRaisesRegex(SyntaxError,
                                    "on raise, the new owner `held` is still held"):
            self.ir_output("""
            inst(OP, (value: consumed --)) effects(raises) exits(next, raise) {
                PyValue held = OWNER(value);
                if (SPAM_FLAG()) {
                    ERROR_NO_POP();
                }
                IR_RETIRE_PUBLISHED(held);
            }
        """)
        with self.assertRaisesRegex(SyntaxError,
                                    "on raise, the new owner `res` is still held"):
            self.ir_output("""
            inst(OP, (value: consumed -- res: moved(value))) effects(raises) exits(next, raise) {
                res = TAKE(value);
                ERROR_IF(SPAM_FLAG());
            }
        """)
        # A taken local that the error path closes is the DSL's to release
        output = self.handler(self.ir_output("""
        inst(OP, (value: consumed -- res: moved(value)))
            effects(releases, raises) exits(next, raise)
        {
            PyValue held = TAKE(value);
            if (SPAM_FLAG()) {
                PyValue_CLOSE(held);
                ERROR_NO_POP();
            }
            res = held;
        }
    """), "OP")
        self.assertLess(output.index("PyValue held = IR_TAKEN_REQUIRED(value, ir_own_B);"),
                        output.index("PyValue_CLOSE(held);"))
        self.assertLess(output.index("PyValue_CLOSE(held);"),
                        output.index("PROCESS_ERROR_AT(RAISE, 0, this_instr);"))

    def test_owner_and_take_forms_refused(self):
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            MACRO(IR_RETIRE_PUBLISHED, ARGS(consumed), NONE, EFFECTS(), EXITS()) \\
            HELPER(SPAM_VALUE, PyValue, (), ARGS(), VALUE(owned), EFFECTS())
        """)
        cases = {
            "OWNER\\(\\) is the initializer of a PyValue variable": """
                PyValue held;
                held = OWNER(value);
                IR_RETIRE_PUBLISHED(held);
            """,
            # (the generator's: "OWNER() gave 'held', which no
            # IR_RETIRE_PUBLISHED ...")
            "on next, the new owner `held` is still held": """
                PyValue held = OWNER(value);
            """,
            # (the generator's: "TAKE() takes a variable")
            "TAKE moves `SPAM_VALUE \\( \\)`, which holds no obligation": """
                PyValue held = TAKE(SPAM_VALUE());
                PyValue_CLOSE(held);
                PyValue_CLOSE(value);
            """,
            "TAKE\\(x\\): not a stack variable": """
                PyValue x = SPAM_VALUE();
                PyValue held = TAKE(x);
                PyValue_CLOSE(held);
                PyValue_CLOSE(value);
            """,
        }
        for message, body in cases.items():
            with self.subTest(message=message):
                with self.assertRaisesRegex(SyntaxError, message):
                    self.ir_output(f"""
                    inst(OP, (value: consumed --)) effects(releases) exits(next) {{
                        {body}
                    }}
                """)

    def test_view_backstops(self):
        # The emitter's own refusals of a view retired as an owner, or
        # copied to a variable that is not on the stack, are backstops:
        # S3 (inplace_input_use) refuses both first
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            MACRO(IR_RETIRE_PUBLISHED, ARGS(consumed), NONE, EFFECTS(), EXITS()) \\
            PRIMITIVE(SPAM, bool, (PyObject *), \\
                      ARGS(object), SCALAR, EFFECTS(reads))
        """)
        with self.assertRaisesRegex(SyntaxError, "inplace_input_use"):
            self.ir_output("""
            op(_COPY, (value -- value, copy: borrowed)) effects() exits(next) {
                copy = value;
            }
            op(_USE, (value: closed object, copy: consumed --))
                effects(reads) exits(next)
            {
                assert(PyValue_IsObject(value));
                SPAM(PyValue_AsKnownObjectBorrow(value));
                DEAD(value);
                IR_RETIRE_PUBLISHED(copy);
            }
            macro(OP) = _COPY + _USE;
        """)
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            HELPER(SPAM, PyValue, (PyObject *), \\
                   ARGS(object), VALUE(owned), EFFECTS(reads))
        """)
        self.assertEqual(self.formats("""
        inst(OP, (value: consumed object -- res: owned))
            effects(reads, releases) exits(next)
        {
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
        op(_DIRECT, (value: consumed -- res: moved(value)))
            effects() exits(next)
        {
            res = TAKE(value);
        }
        op(_HELD, (value: consumed -- res: moved(value)))
            effects() exits(next)
        {
            PyValue held = TAKE(value);
            res = held;
        }
    """)
        self.assertEqual(analysis.uops["_DIRECT"].aliased_outputs, ("res",))
        self.assertEqual(analysis.uops["_HELD"].aliased_outputs, ("res",))
        # A specialised close of a view releases its owner, which is NULL
        # for a local
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            MACRO(SPAM, ARGS(object), VALUE(owned), EFFECTS(), EXITS())
        """)
        output = self.handler(self.ir_output("""
        inst(OP, (value: consumed object exact(PyLong_Type) -- res: owned))
            effects(writes) exits(next)
        {
            res = SPAM(PyValue_AsKnownObjectBorrow(value));
            PyValue_CLOSE_SPECIALIZED(value, _PyLong_ExactDealloc);
        }
    """), "OP")
        self.assertIn("_PyValue_XCLOSE_SPECIALIZED_POINTER(ir_own_B, _PyLong_ExactDealloc);",
                      output)

    def test_inplace_inputs_fail_closed(self):
        # The generator refuses what it cannot follow: a value that borrows
        # a local in place on one path and not on another.  The contract's
        # verifier refuses the first form before the generator's own check
        # of a cleared input: the taken value is owned on one path and not
        # on another where they join.
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            MACRO(SPAM, ARGS(object), VALUE(owned), EFFECTS(reads), EXITS())
        """)
        input = """
        op(_MAYBE_HELD, (value: consumed -- res: moved(value)))
            effects() exits(next)
        {
            if (SPAM_FLAG()) {
                PyValue held = TAKE(value);
            }
        }
        op(_USE, (value: consumed object -- res: owned))
            effects(reads, releases) exits(next)
        {
            res = SPAM(PyValue_AsKnownObjectBorrow(value));
            PyValue_CLOSE(value);
        }
        macro(OP) = _MAYBE_HELD + _USE;
    """
        with self.assertRaisesRegex(SyntaxError,
                                    "is owned on one path and not on another "
                                    "where they join"):
            self.ir_output(input)
        # A variable is a view or an owner (S8): it has one C type
        roles = """
        op(_HELD, (value: consumed -- res: moved(value)))
            effects() exits(next)
        {
            res = TAKE(value);
        }
        op(_USE, (value: consumed object -- res: owned))
            effects(reads, releases) exits(next)
        {
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
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            MACRO(SPAM, ARGS(object), VALUE(owned), EFFECTS(reads), EXITS()) \\
            MACRO(SPAM_TakeRef, ARGS(object, consumed object), STATUS(NONZERO), \\
                  EFFECTS(reads, writes), EXITS())
        """)
        input = """
        inst(OP_COPY_CLOSE, (value: consumed object -- res: owned))
            effects(reads, releases) exits(next)
        {
            PyValue a;
            a = value;
            DEAD(value);
            PyObject *made = SPAM(PyValue_AsKnownObjectBorrow(a));
            res = PyValue_FromObjectSteal(made);
            PyValue_CLOSE(a);
        }
        inst(OP_COPY_STORE, (value: consumed -- res: moved(value)))
            effects() exits(next)
        {
            PyValue v2 = PyValue_None;
            v2 = value;
            DEAD(value);
            res = v2;
        }
        inst(OP_OBJECT_STEAL, (value: consumed object -- res: moved(value)))
            effects() exits(next)
        {
            PyObject *o = PyValue_AsKnownObjectBorrow(value);
            res = PyValue_FromObjectSteal(o);
            DEAD(value);
        }
        inst(OP_OBJECT_COPY_RELEASE, (value: consumed object --))
            effects(releases) exits(next)
        {
            PyObject *o = PyValue_AsKnownObjectBorrow(value);
            PyObject *p;
            p = o;
            Py_DECREF(p);
            DEAD(value);
        }
        inst(OP_OBJECT_NESTED_STEAL, (value: consumed object -- res: moved(value)))
            effects() exits(next)
        {
            res = PyValue_FromObjectSteal(PyValue_AsKnownObjectBorrow(value));
            DEAD(value);
        }
        inst(OP_OBJECT_TARGET, (value: consumed object, item: consumed object --))
            effects(reads, writes, releases, raises) exits(next, raise)
        {
            PyObject *stolen = PyValue_AsKnownObjectSteal(item);
            int err = SPAM_TakeRef((PyObject *)PyValue_AsKnownObjectBorrow(value),
                                   stolen);
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
        self.assertIn("_PyValue_AsPyObjectStealPointer(IR_TAKEN_POINTER(item, ir_own_B))", output)

    def test_consumed_inputs(self):
        # A handler takes the inputs that it consumes: the slot of a
        # temporary becomes NULL, and a local keeps its value.  No handler
        # has a stack pointer to keep.
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            MACRO(SPAM, ARGS(object, object), NONE, EFFECTS(reads), EXITS())
        """)
        input = """
        inst(STORE, (owner: consumed object, value: consumed object --))
            effects(reads, releases) exits(next)
        {
            SPAM(PyValue_AsKnownObjectBorrow(owner),
                 PyValue_AsKnownObjectBorrow(value));
            DECREF_INPUTS();
        }
        inst(BRANCH, (unused/1, cond: consumed exact(PyBool_Type) --))
            effects() exits(next, branch) feedback(read(counter))
        {
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
        # The inputs are read in place; the effect comes first, then the
        # detach of each dying input, which gives the owner of a temporary
        # and NULL for a local, and the release of what was detached
        effect = store.index("SPAM(")
        detach_owner = store.index("ir_own_A = IR_DETACH(ir_word, A, owner);")
        detach_value = store.index("ir_own_B = IR_DETACH(ir_word, B, value);")
        close_owner = store.index("_PyValue_XCLOSE_POINTER(ir_own_A);")
        close_value = store.index("_PyValue_XCLOSE_POINTER(ir_own_B);")
        self.assertLess(effect, min(detach_owner, detach_value))
        self.assertLess(max(detach_owner, detach_value),
                        min(close_owner, close_value))
        self.assertIn("ir_own_A = IR_DETACH(ir_word, A, cond);",
                      self.handler(output, "BRANCH"))
        self.assertNotIn("stack_pointer", output)

    def test_store_dst(self):
        # Which results a folded STORE_FAST may send to a local, and how
        # the handler then finds the stack pointer: dst may name a local, so
        # from the top slot in a free field, or by counting.
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            MACRO(SPAM, ARGS(object, object), VALUE(owned), EFFECTS(reads), EXITS()) \\
            MACRO(SPAM_ONE, ARGS(object), VALUE(owned), EFFECTS(reads), EXITS()) \\
            MACRO(SPAM_NONE, ARGS(), VALUE(owned), EFFECTS(), EXITS())
        """)
        input = """
        inst(OP, (left: consumed object, right: consumed object -- res: owned))
            effects(reads, releases) exits(next)
        {
            res = SPAM(PyValue_AsKnownObjectBorrow(left),
                       PyValue_AsKnownObjectBorrow(right));
            DECREF_INPUTS();
        }
        inst(ATTR, (owner: consumed object -- attr: owned, null[oparg & 1]: owned nullable))
            effects(reads, releases) exits(next)
        {
            attr = SPAM_ONE(PyValue_AsKnownObjectBorrow(owner));
            DECREF_INPUTS();
            if (oparg & 1) {
                null[0] = PyValue_NULL;
            }
        }
        no_fold inst(ALIAS, (value: consumed object -- res: owned))
            effects(reads, releases) exits(next)
        {
            res = SPAM_ONE(PyValue_AsKnownObjectBorrow(value));
            DECREF_INPUTS();
        }
        inst(PAIR, (-- a: owned, b: owned))
            effects() exits(next)
        {
            a = SPAM_NONE();
            b = SPAM_NONE();
        }
    """
        formats = self.formats(input)
        self.assertTrue(formats["OP"].store_dst)
        self.assertTrue(formats["ATTR"].store_dst)
        self.assertFalse(formats["ALIAS"].store_dst)
        # PAIR never pushes its first output alone
        self.assertTrue(formats["PAIR"].store_dst)
        output = self.ir_output(input)
        self.assertIn("IR_STORE_DST(ir_word, A, res);",
                      self.handler(output, "OP"))
        self.assertIn("IR_STORE_DST(ir_word, A, attr);",
                      self.handler(output, "ATTR"))
        self.assertNotIn("IR_STORE_DST", self.handler(output, "ALIAS"))
        self.assertIn("IR_STORE_TEMP(IR_SLOT(IR_A(ir_word)), res);",
                      self.handler(output, "ALIAS"))
        self.assertNotIn("IR_STORE_DST", self.handler(output, "PAIR"))
        self.assertIn("assert(!IR_IS_LOCAL_FIELD(ir_word, A));",
                      self.handler(output, "PAIR"))

    def test_scalar_conditional_array_has_no_empty_home(self):
        source = """
        inst(ATTR, (owner: consumed -- attr: owned, null[oparg & 1]: owned nullable))
            effects(releases) exits(next)
        {
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
        self.assertIn("IR_STORE_DST(ir_word, A, attr)", handler)

    def test_unused_outputs_pass_through(self):
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            MACRO(SPAM, ARGS(object), VALUE(owned), EFFECTS(reads), EXITS())
        """)
        input = """
        op(_GUARD, (nos, unused -- nos: object, unused))
            effects() exits(next, miss)
        {
            EXIT_IF(!PyValue_IsObject(nos));
        }
        op(_USE, (left, right: consumed object -- left, res: owned))
            effects(reads, releases) exits(next)
        {
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
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            MACRO(SPAM, ARGS(scalar, scalar, scalar), VALUE(owned), EFFECTS(reads), EXITS())
        """)
        input = """
        inst(OP, (callable: closed, args[oparg]: closed -- res: owned))
            effects(reads) exits(next)
        {
            res = SPAM(callable, args, oparg);
            INPUTS_DEAD();
        }
    """
        output = """
        TARGET(OP) {
            int opcode = OP;
            (void)(opcode);
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            IR_HOLD_NEXT();
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            PyValue ir_old = PyValue_NULL;
            (void)ir_old;
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
            IR_SET_TEMP(window[0], res);
            IR_MOVE_TO_DST(ir_word, A, window[0]);
            PyValue_XCLOSE(ir_old);
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
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            MACRO(SPAM, ARGS(scalar), VALUE(owned), EFFECTS(reads), EXITS()) \\
            MACRO(SPAM_THREE, ARGS(scalar, scalar, scalar), VALUE(owned), EFFECTS(reads), EXITS()) \\
            MACRO(SPAM_FOUR, ARGS(scalar, scalar, scalar, scalar), VALUE(owned), \\
                  EFFECTS(reads), EXITS()) \\
            MACRO(SPAM_STATUS, ARGS(scalar, scalar, scalar), NONE, EFFECTS(reads), EXITS())
        """)
        input = """
        inst(OP_FOUR, (a: closed, b: closed, c: closed -- res: owned))
            effects(reads) exits(next)
        {
            res = SPAM_THREE(a, b, c);
            INPUTS_DEAD();
        }
        inst(OP_FIVE, (a: closed, b: closed, c: closed, d: closed -- res: owned))
            effects(reads) exits(next)
        {
            res = SPAM_FOUR(a, b, c, d);
            INPUTS_DEAD();
        }
        inst(OP_THREE, (a: closed, b: closed, c: closed --))
            effects(reads) exits(next)
        {
            SPAM_STATUS(a, b, c);
            INPUTS_DEAD();
        }
        window inst(OP_ANNOTATED, (value: closed -- res: owned))
            effects(reads) exits(next)
        {
            res = SPAM(value);
            INPUTS_DEAD();
        }
        window inst(OP_SWITCH, (value: consumed -- res: owned nullable))
            effects(switches_frame) exits(enter)
        {
            _PyInterpreterFrame *temp = PyValue_Unwrap(value);
            DEAD(value);
            res = PyValue_NULL;
            frame = tstate->current_frame = temp;
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

    def test_asserted_input_has_no_object_local(self):
        # An input that the body converts to an object is declared to be
        # one: the conversions read it directly, with no object local and no
        # box, and the owned input closes as a pointer.
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            MACRO(SPAM, ARGS(object), NONE, EFFECTS(reads), EXITS()) \\
            MACRO(SPAM_OWN, ARGS(consumed object), VALUE(owned), EFFECTS(), EXITS())
        """)
        input = """
        inst(OP, (v: consumed object -- res: moved(v)))
            effects(reads) exits(next)
        {
            PyObject *v_o = PyValue_AsKnownObjectBorrow(v);
            PyValue kept = TAKE(v);
            SPAM(v_o);
            res = kept;
        }
        window inst(OP_OWNED, (v: consumed object -- res: owned))
            effects(reads) exits(next)
        {
            if (PyValue_IsNone(v)) {
                SPAM(PyValue_AsKnownObjectBorrow(v));
            }
            PyObject *stolen = PyValue_AsKnownObjectSteal(v);
            res = SPAM_OWN(stolen);
        }
    """
        output = self.ir_output(input)
        for name in ("OP", "OP_OWNED"):
            handler = self.handler(output, name)
            self.assertNotIn("ir_obj_", handler)
            self.assertNotIn("_PyValueTemps", handler)
        op = self.handler(output, "OP")
        self.assertIn("PyObject *v_o = _PyValue_AsKnownObjectBorrowPointer(v);", op)
        self.assertIn("PyValue kept = IR_TAKEN_POINTER(v, ir_own_B);", op)
        owned = self.handler(output, "OP_OWNED")
        self.assertIn("SPAM(_PyValue_AsKnownObjectBorrowPointer(v));", owned)
        self.assertIn("_PyValue_AsPyObjectStealPointer(v)", owned)
        # A conversion of an input that does not declare `object` is refused
        with self.assertRaisesRegex(SyntaxError, "requires object of it"):
            self.ir_output("""
            inst(OP, (v: consumed --))
                effects(reads, releases) exits(next)
            {
                SPAM(PyValue_AsKnownObjectBorrow(v));
                DECREF_INPUTS();
            }
        """)

    def test_object_readers(self):
        # S3: the object of a view has no count, so it reaches only a field
        # access, a comparison, a copy or an argument of a function on the
        # checked list; any other use keeps the input out of borrow_mask
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            MACRO(SPAM_KEEP, ARGS(object), VALUE(owned), EFFECTS(reads), EXITS()) \\
            MACRO(SPAM_STRUCT, ARGS(), SCALAR, EFFECTS(), EXITS())
        """)
        input = """
        inst(OP_READ, (value: consumed object -- res: owned exact(PyBool_Type)))
            effects(reads, releases) exits(next)
        {
            PyObject *o = PyValue_AsKnownObjectBorrow(value);
            res = Py_TYPE(o)->tp_flags & 1 ? PyValue_True : PyValue_False;
            PyValue_CLOSE(value);
        }
        inst(OP_FIELD, (value: consumed object -- res: owned exact(PyBool_Type)))
            effects(reads, releases) exits(next)
        {
            res = ((PyTupleObject *)PyValue_AsKnownObjectBorrow(value))->ob_item[0] == NULL
                ? PyValue_True : PyValue_False;
            PyValue_CLOSE(value);
        }
        inst(OP_KEEP, (value: consumed object -- res: owned))
            effects(reads, releases) exits(next)
        {
            PyObject *o = PyValue_AsKnownObjectBorrow(value);
            res = SPAM_KEEP(o);
            PyValue_CLOSE(value);
        }
        inst(OP_STORE, (value: consumed object -- ))
            effects(reads, writes, releases) exits(next)
        {
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
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            MACRO(SPAM_KEEP, ARGS(object), VALUE(owned), EFFECTS(reads), EXITS()) \\
            MACRO(SPAM_SLOT, ARGS(), SCALAR, EFFECTS(), EXITS()) \\
            MACRO(SPAM_BASE, ARGS(), SCALAR, EFFECTS(), EXITS()) \\
            MACRO(SPAM, ARGS(scalar, scalar), VALUE(owned), EFFECTS(reads), EXITS())
        """)
        input = """
        inst(OP_DEREF_STORE, (value: consumed object -- ))
            effects(reads, writes, releases) exits(next)
        {
            PyObject **p = SPAM_SLOT();
            *p = PyValue_AsKnownObjectBorrow(value);
            PyValue_CLOSE(value);
        }
        inst(OP_COPY_STORE, (value: consumed object -- ))
            effects(reads, writes, releases) exits(next)
        {
            PyObject *o = PyValue_AsKnownObjectBorrow(value);
            PyObject **p = SPAM_SLOT();
            *p = o;
            PyValue_CLOSE(value);
        }
        inst(OP_CHAINED, (value: consumed object -- res: owned))
            effects(reads, releases) exits(next)
        {
            PyObject *a;
            PyObject *b;
            a = b = PyValue_AsKnownObjectBorrow(value);
            res = SPAM_KEEP(a);
            PyValue_CLOSE(value);
        }
        inst(OP_ARITHMETIC, (value: consumed object -- res: owned))
            effects(reads, releases) exits(next)
        {
            PyObject *o = PyValue_AsKnownObjectBorrow(value);
            PyObject *k = o + 0;
            res = SPAM_KEEP(k);
            PyValue_CLOSE(value);
        }
        inst(OP_FIELD_ADDRESS, (value: consumed object -- res: owned))
            effects(reads, releases) exits(next)
        {
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
            formats = ir_generator.compute_formats(analysis)
            seen = ir_generator.OBJECT_READERS_SEEN
            lowered = set()
        self.assertEqual(seen | lowered | {"SPAM", "spam_call"},
                         ir_generator.OBJECT_READERS)

    def test_frame_switch_needs_window(self):
        # The outputs of an instruction that switches frames land in the
        # other frame, where the fields name no slots.
        input = """
        inst(OP, (value: consumed -- res: owned nullable))
            effects(switches_frame) exits(enter)
        {
            _PyInterpreterFrame *temp = PyValue_Unwrap(value);
            DEAD(value);
            res = PyValue_NULL;
            frame = tstate->current_frame = temp;
        }
    """
        with self.assertRaisesRegex(SyntaxError, "OP switches frames"):
            self.formats(input)

    def test_feedback_mapping(self):
        # A family shares one format. A name index takes the free field of
        # the word, and cache effects read the entry.  The sites that get no
        # entry share entry 0 and run the same handlers.
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            MACRO(SPAM, ARGS(object, scalar), VALUE(owned), EFFECTS(reads), EXITS()) \\
            MACRO(ADVANCE_ADAPTIVE_COUNTER, ARGS(scalar), NONE, EFFECTS(writes), EXITS(), \\
                  FEEDBACK(write(counter)))
        """)
        input = """
        specializing op(_SPECIALIZE_OP, (owner -- owner))
            effects(writes) exits(next) feedback(write(counter))
        {
            _PyFbHeader *feedback = FB_COUNTER();
            ADVANCE_ADAPTIVE_COUNTER(feedback);
        }
        op(_OP, (owner: consumed object -- res: owned))
            effects(reads, releases) exits(next)
        {
            assert(PyValue_IsObject(owner));
            PyObject *o = PyValue_AsKnownObjectBorrow(owner);
            res = SPAM(o, GETITEM(FRAME_CO_NAMES, oparg));
            DECREF_INPUTS();
        }
        macro(OP) = _SPECIALIZE_OP + _OP;
        op(_GUARD, (type_version/2, owner: object -- owner: type_version(type_version)))
            effects(reads) exits(next, miss)
        {
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
            IR_HOLD_NEXT();
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyIRWord* const this_instr = next_instr - 1;
            (void)this_instr;
            PyValue ir_old = PyValue_NULL;
            (void)ir_old;
            _PyFb_OP *fb = (_PyFb_OP *)IR_FB(IR_C(ir_word));
            int oparg = (int)IR_D(ir_word);
            (void)oparg;
            PyValueBorrow owner;
            PyValue res;
            PyValue ir_own_B = PyValue_NULL;
            (void)ir_own_B;
            // _SPECIALIZE_OP
            {
                _PyFbHeader *feedback = ((_PyFbHeader *)fb);
                ADVANCE_ADAPTIVE_COUNTER(feedback);
            }
            // _OP
            {
                owner = IR_VIEW(IR_B(ir_word));
                assert(PyValue_IsObject(owner));
                PyObject *o = _PyValue_AsKnownObjectBorrowPointer(owner);
                res = SPAM(o, GETITEM(FRAME_CO_NAMES, oparg));
                ir_own_B = IR_DETACH(ir_word, B, owner);
            }
            IR_STORE_DST(ir_word, A, res);
            _PyValue_XCLOSE_POINTER(ir_own_B);
            PyValue_XCLOSE(ir_old);
            DISPATCH();
        }

        TARGET(OP_SPECIALIZED) {
            int opcode = OP_SPECIALIZED;
            (void)(opcode);
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            IR_HOLD_NEXT();
            next_instr += 1;
            INSTRUCTION_STATS(OP_SPECIALIZED);
            PyValue ir_old = PyValue_NULL;
            (void)ir_old;
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
                if (Py_UNLIKELY(!PyValue_IsPointerSpaceAt(&IR_SLOT(IR_B(ir_word))))) {
                    UPDATE_MISS_STATS(OP);
                    assert(_PyOpcode_Deopt[opcode] == (OP));
                    JUMP_TO_PREDICTED(OP);
                }
                if (Py_TYPE(_PyValue_AsKnownObjectBorrowPointer(owner))->tp_version_tag != type_version) {
                    UPDATE_MISS_STATS(OP);
                    assert(_PyOpcode_Deopt[opcode] == (OP));
                    JUMP_TO_PREDICTED(OP);
                }
            }
            // _OP
            {
                assert(PyValue_IsObject(owner));
                PyObject *o = _PyValue_AsKnownObjectBorrowPointer(owner);
                res = SPAM(o, GETITEM(FRAME_CO_NAMES, oparg));
                ir_own_B = IR_DETACH(ir_word, B, owner);
            }
            IR_STORE_DST(ir_word, A, res);
            _PyValue_XCLOSE_POINTER(ir_own_B);
            PyValue_XCLOSE(ir_old);
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
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            MACRO(SPAM, ARGS(scalar, scalar, scalar), VALUE(owned), EFFECTS(reads), EXITS()) \\
            MACRO(ADVANCE_ADAPTIVE_COUNTER, ARGS(scalar), NONE, EFFECTS(writes), EXITS(), \\
                  FEEDBACK(write(counter)))
        """)
        input = """
        inst(OP, (unused/1, left: closed, right: closed -- res: owned))
            effects(reads, writes) exits(next) feedback(write(counter))
        {
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
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            IR_HOLD_NEXT();
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            PyValue ir_old = PyValue_NULL;
            (void)ir_old;
            _PyFb_OP *fb = (_PyFb_OP *)IR_FB(IR_D(ir_word));
            int oparg = IR_FB_ARG(fb, IR_D(ir_word));
            (void)oparg;
            PyValue left;
            PyValue right;
            PyValue res;
            right = IR_SLOT(IR_C(ir_word));
            left = IR_SLOT(IR_B(ir_word));
            _PyFbHeader *feedback = ((_PyFbHeader *)fb);
            ADVANCE_ADAPTIVE_COUNTER(feedback);
            res = SPAM(left, right, oparg);
            _PyFrame_WriteHome(frame, &IR_SLOT(IR_B(ir_word)), PyValue_NULL);
            _PyFrame_WriteHome(frame, &IR_SLOT(IR_C(ir_word)), PyValue_NULL);
            IR_STORE_DST(ir_word, A, res);
            PyValue_XCLOSE(ir_old);
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
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            MACRO(SPAM, ARGS(scalar), VALUE_OR_NULL(owned), EFFECTS(reads), EXITS())
        """)
        input = """
        inst(OP, (cond: consumed exact(PyBool_Type) --))
            effects() exits(next, branch)
        {
            int flag = PyValue_IsFalse(cond);
            DEAD(cond);
            if (flag) {
                JUMPTO(oparg);
            }
        }
        inst(FOR_ITER, (iter -- iter, next: owned nullable))
            effects(reads) exits(next, branch)
        {
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
        ir_only inst(IR_PASS, ( -- ))
            effects() exits(jump)
        {
            PRE_DISPATCH_GOTO();
            DISPATCH_GOTO();
        }
    """
        output = """
        TARGET(IR_PASS) {
            int opcode = IR_PASS;
            (void)(opcode);
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            IR_HOLD_NEXT();
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
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            MACRO(SPAM, ARGS(), NONE, EFFECTS(reads), EXITS())
        """)
        input = """
        inst(LOAD, ( -- value: owned))
            effects() exits(next)
        {
            PyValueBorrow local = _PyValue_BoundLocal(GETLOCAL(oparg));
            value = PyValue_DUP(local);
        }
        inst(INSTRUMENTED_LOAD, ( -- value: owned))
            effects(runs_python) exits(next)
        {
            PyValueBorrow local = _PyValue_BoundLocal(GETLOCAL(oparg));
            value = PyValue_DUP(local);
        }
        ir_only inst(IR_BACON, ( -- ))
            effects(reads) exits(next)
        {
            SPAM();
        }
        ir_only inst(IR_PASS, ( -- ))
            effects() exits(jump)
        {
            PRE_DISPATCH_GOTO();
            DISPATCH_GOTO();
        }
    """
        analysis = self.analyze(input)
        out = io.StringIO()
        ir_metadata_generator.generate_ir_metadata(
            [self.temp_input_filename], analysis, out)
        lines = [line.strip() for line in out.getvalue().splitlines()]
        self.assertIn("#define _PyIR_OBSERVER_ENTRY_ABSENCE 0", lines)
        # The shared handler bound is emitted in opcode_ids.h, not metadata.
        # The IR-only handlers fit the free numbers below 256, so the arms
        # start at 256.
        self.assertEqual(ir_generator.num_old_handlers(analysis), 256)
        bacon, ir_pass = analysis.free_ids[:2]
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
                ".owner_program = 0, .owner_events = 1, .word_flags = 0, "
                ".error_routes = _PyIR_ERROR_COMPLETE | _PyIR_ERROR_LOCAL_DST, "
                ".operands = _PyIR_OPERANDS_NONE, "
                ".observer_outcome = { _PyIR_OBSERVER_NONE, _PyIR_OBSERVER_NONE }, "
                ".reads = { 0, _PyIR_EXT_FIXED }, "
                ".writes = { 1, _PyIR_EXT_FIXED }, "
                ".through = { 0, _PyIR_EXT_FIXED }, .shadow = LOAD, "
                ".twin = INSTRUMENTED_LOAD, .events = 0 }")
        instrumented = (load.replace(".effects = 0",
                                     ".effects = _PyIR_RAISES | _PyIR_ESCAPES | _PyIR_RUNS_PYTHON")
                            .replace("shadow = LOAD", "shadow = INSTRUMENTED_LOAD"))
        for expected in [
            f"#define {'IR_BACON':<46} {bacon}",
            f"#define {'IR_PASS':<46} {ir_pass}",
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
            ".owner_program = 0, .owner_events = 0, .word_flags = 0, .error_routes = 0, "
            ".operands = _PyIR_OPERANDS_NONE, "
            ".observer_outcome = { _PyIR_OBSERVER_NONE, _PyIR_OBSERVER_NONE }, "
            ".reads = { 0, _PyIR_EXT_FIXED }, "
            ".writes = { 0, _PyIR_EXT_FIXED }, "
            ".through = { 0, _PyIR_EXT_FIXED }, .shadow = IR_PASS, "
            ".twin = 0, .events = 0 },",
            f'[IR_BACON] = "IR_BACON",',
            f'[IR_PASS] = "IR_PASS",',
        ]:
            self.assertIn(expected, lines)

    def test_normal_transfer_source_proofs(self):
        cases = (
            ("LOAD_COMMON_CONSTANT", "-- value: owned", "effects(reads) exits(next)",
             "assert(oparg < NUM_COMMON_CONSTANTS); PyValueBorrow constant = "
             "_PyValue_CommonConstant(tstate->interp->common_consts[oparg]); "
             "value = PyValue_DUP(constant);",
             "FRESH", "PyValue_DUP", "PyValue_XDUP"),
        )
        for name, effect, clauses, body, kind, old, new in cases:
            with self.subTest(handler=name):
                def kind_for(body):
                    analysis = self.analyze(
                        f"\ninst({name}, ({effect})) {clauses} {{ {body} }}\n")
                    inst = analysis.instructions[name]
                    fmt = ir_generator.compute_formats(analysis)[name]
                    return ir_generator.normal_transfer_kind(inst, fmt)
                self.assertEqual(kind_for(body), "_PyIR_TRANSFER_" + kind)
                self.assertIn(old, body)
                self.assertEqual(kind_for(body.replace(old, new)),
                                 "_PyIR_TRANSFER_UNKNOWN")

    def test_swap_transfer_comes_from_the_exchange(self):
        # The swap is the declared exchange (ir_contract.check_exchange),
        # not the body text: undeclared, the same body proves nothing
        body = "PyValue temp = bottom; bottom = top; top = temp;"
        def kind_for(effect, clauses=""):
            analysis = self.analyze(f"\ninst(SWAP, ({effect})) {clauses} {{ {body} }}\n")
            inst = analysis.instructions["SWAP"]
            fmt = ir_generator.compute_formats(analysis)["SWAP"]
            return ir_generator.normal_transfer_kind(inst, fmt)
        self.assertEqual(kind_for("bottom, unused[oparg-2], top -- "
                                  "bottom: moved(top), unused[oparg-2], top: moved(bottom)",
                                  "effects() exits(next)"),
                         "_PyIR_TRANSFER_SWAP")
        self.assertEqual(kind_for("bottom, unused[oparg-2], top -- bottom, unused[oparg-2], top",
                                  "effects() exits(next)"),
                         "_PyIR_TRANSFER_UNKNOWN")

    def test_transparent_leaf_still_has_a_consumer_recipe(self):
        source = ("\nir_only inst(IR_JUMP, (--)) effects() exits(next, jump)"
                  " { JUMPTO(oparg); }\n")
        analysis = self.analyze(source)
        fmt = ir_generator.compute_formats(analysis)
        contract = ir_generator.handler_contracts(analysis, fmt)["IR_JUMP"]
        self.assertEqual(contract.ownership_events, (
            ir_generator.OwnershipEvent("_PyIR_OWNER_LEAF", src="_PyIR_TRANSFER_PRESERVE"),))

    def test_normal_transfer_requires_matching_io(self):
        # The same body with a different window cannot acquire COPY geometry.
        source = ("\ninst(COPY, (bottom -- bottom, top: owned)) effects() exits(next)"
                  " { top = PyValue_DUP(bottom); }\n")
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
        # The twin's observer ingress must also be covered.  A statement
        # that the verifier of the observer's declaration refuses (an
        # `opaque(frame)` call is no row) never reaches this proof, so the
        # statements are ones that the verifier accepts.  The generator's
        # boundary then refuses the handler whose observer source is not the
        # checked one.
        twin = analysis.instructions["INSTRUMENTED_YIELD_VALUE"]
        observer = twin.parts[0]
        # This observer uop is also used by delegated suspension, whose
        # metadata cursor has a stricter source contract. Mutate only the
        # ordinary twin whose family permission is under test.
        for changed in ("{ gen_frame->return_slot = 0; }",
                        "{ frame->base[0] = PyValue_NULL; }",
                        "{ frame->instr_ptr = this_instr; }"):
            with self.subTest(observer=changed):
                extra = self.analyze("\ninst(OP, (--)) " + changed + "\n")
                body = replace(observer.body, body=observer.body.body +
                               extra.instructions["OP"].parts[0].body.body)
                twin.parts[0] = replace(observer, body=body)
                try:
                    self.assertFalse(ir_generator.abrupt_suspend_contract(
                        twin, formats["INSTRUMENTED_YIELD_VALUE"]))
                    with self.assertRaisesRegex(SyntaxError,
                                                "needs checked yield transport"):
                        ir_generator.handler_contracts(analysis, formats)
                finally:
                    twin.parts[0] = observer

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
        declaration = ("\ninst(OP, (left: consumed, right: consumed --)) "
                       "effects(runs_python) exits(next, raise) ")
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
        expected = {"BUILD_STRING": "IR_BUILD_STRING", "_LOAD_SPECIAL": "IR_LOOKUP_SPECIAL"}
        for name, helper in expected.items():
            part = analysis.uops[name]
            self.assertTrue(ir_generator.home_operation_contract(part))
            self.assertNotIn(helper, analyzer.IR_MARKER_EFFECTS)
            self.assertIn(helper, {escape.call.text
                                  for escape in part.properties.escaping_calls.values()})
        for name in ("_INSERT_NULL",):
            self.assertTrue(ir_generator.home_operation_contract(analysis.uops[name]))

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
        def releases(name):
            return any(effect == ir_records.MAY_RELEASE
                       for part in analysis.instructions[name].parts
                       if isinstance(part, analyzer.Uop)
                       for _, effect in ir_records.effect_sites(part))
        self.assertTrue(releases("POP_TOP"))

    def test_nullable_annotations_are_input_only(self):
        analysis = self.analyze("""
            inst(OP, (required, value:nullable, items[oparg]:nullable --)) {
                opaque(required, value, items);
            }
        """)
        inputs = analysis.instructions["OP"].parts[0].stack.inputs
        self.assertEqual([item.nullable for item in inputs], [False, True, True])
        self.assertEqual(inputs[2].size, "oparg")
        analysis = self.analyze("""
            inst(OP, (value:nullable, items[oparg]:nullable -- value, items[oparg])) {
            }
        """)
        inputs = analysis.instructions["OP"].parts[0].stack.inputs
        self.assertEqual([item.nullable for item in inputs], [True, True])
        self.assertTrue(all(item.peek for item in inputs))
        for source in (
            "inst(OP, (-- value:nullable)) { value = PyValue_NULL; }",
            "inst(OP, (value:unknown --)) { DEAD(value); }",
        ):
            with self.subTest(source=source), self.assertRaises(SyntaxError):
                self.analyze("\n" + source + "\n")


    def test_execution_read_proofs_follow_current_c_values(self):
        from ir_read_effects import error_nonnull_value, value_nonnull_changes

        def changes(source):
            return value_nonnull_changes(lexer.tokenize(source), {"value"},
                                         {"value", "result"})

        self.assertEqual(changes("result = value;"), {"result": True})
        self.assertEqual(changes("result = PyValue_DUP(value);"), {"result": True})
        self.assertEqual(changes("result = OWNER(value);"),
                         {"value": False, "result": False})
        self.assertEqual(changes("result = flag ? PyValue_True : PyValue_False;"),
                         {"result": True})
        self.assertEqual(changes("result = flag ? value : PyValue_NULL;"),
                         {"result": False})
        self.assertFalse(changes("result = (value = PyValue_NULL) ? value : PyValue_True;")["result"])
        self.assertEqual(error_nonnull_value(lexer.tokenize("ERROR_IF(PyValue_IsNull(value));")),
                         "value")
        self.assertIsNone(error_nonnull_value(lexer.tokenize(
            "ERROR_IF(flag && PyValue_IsNull(value));")))
        for source in ("value = opaque();", "value = PyValue_NULL;",
                       "value.bits &= 0;", "value++;", "opaque(&((value)));",
                       "PyValue value;", "PyValue value = PyValue_None;",
                       "PyValue_CLEAR(value);", "Py_XSETREF(value, PyValue_NULL);",
                       "PyValue_CLOSE(value);"):
            with self.subTest(source=source):
                self.assertFalse(changes(source)["value"])

        local = stack.Local.register("value")
        local.nonnull = True
        copied = local.copy()
        copied.kill()
        self.assertTrue(local.nonnull)
        self.assertFalse(copied.nonnull)

        left, right = stack.Stack(), stack.Stack()
        left.variables = [local]
        right.variables = [local.copy()]
        right.variables[0].nonnull = False
        left.merge(right, CWriter(io.StringIO(), 0, False))
        self.assertFalse(left.variables[0].nonnull)


    def test_input_drain_lowers_through_actual_storage(self):
        output = self.ir_output("""
            inst(OP, (items[oparg]: consumed --))
                effects(writes, releases) exits(next)
            {
                IR_CLOSE_INPUTS(items);
            }
        """)
        self.assertIn("_PyFrame_CloseHomeRange(frame, items, oparg)", output)
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
        for name in ("_UNPACK_SEQUENCE_TUPLE", "_UNPACK_SEQUENCE_LIST"):
            self.assertTrue(ir_generator.home_operation_contract(analysis.uops[name]))

    def test_steal_success_handoff_requires_terminal_ownership_sequence(self):
        body = """{
            PyObject *object = IR_BUILD_TUPLE(values);
            if (object == NULL) { ERROR_NO_POP(); }
            INPUTS_DEAD();
            result = PyValue_FromObjectSteal(object);
        }"""
        def check(changed, inputs="values[oparg]: taken", outputs="result: owned"):
            analysis = self.analyze(f"\ninst(OP, ({inputs} -- {outputs})) "
                                    f"effects(allocates, raises) exits(next, raise) {changed}\n")
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
        for inputs, outputs in (("values[oparg*2]: taken", "result: owned"),
                                ("extra: borrowed, values[oparg]: taken", "result: owned"),
                                ("values[oparg]: taken", "result[1]: owned")):
            with self.subTest(inputs=inputs, outputs=outputs), self.assertRaises(SyntaxError):
                check(body, inputs, outputs)
        inst = check(body)
        fmt = ir_generator.compute_formats(self.analyze(
            "\ninst(OP, (values[oparg]: taken -- result: owned)) "
            "effects(allocates, raises) exits(next, raise) " + body + "\n"))["OP"]
        self.assertTrue(ir_generator.normal_vacate_capability(inst, fmt))
        extra = self.analyze("\ninst(EXTRA, (--)) effects() exits(next) { opaque(); }\n")
        inst.parts.append(extra.instructions["EXTRA"].parts[0])
        self.assertFalse(ir_generator.normal_vacate_capability(inst, fmt))

    def test_steal_success_handoff_uses_existing_call_and_final_flush(self):
        for marker, helper in (("IR_BUILD_TUPLE", "_PyTuple_FromValueStealOnSuccess"),
                               ("IR_BUILD_LIST", "_PyList_FromValueStealOnSuccess")):
            source = f"""
                inst(OP, (values[oparg]: taken -- result: owned))
                    effects(allocates, raises) exits(next, raise)
                {{
                    PyObject *object = {marker}(values);
                    if (object == NULL) {{ ERROR_NO_POP(); }}
                    INPUTS_DEAD();
                    result = PyValue_FromObjectSteal(object);
                }}
            """
            analysis = self.analyze(source)
            formats = ir_generator.compute_formats(analysis)
            programs = {}
            ir_generator.analyze_handlers(analysis, formats, programs)
            self.assertEqual(programs["OP"][0].kind, "_PyIR_OWNER_NORMAL_CLEAR_INPUTS")
            output = self.ir_output(source)
            self.assertIn(f"{helper}(values, oparg)", output)
            self.assertNotIn(marker, output)
            self.assertNotIn(f"{helper}(frame", output)
            self.assertNotIn("_PyValueTemps value_owners", output)
            call = output.index(f"{helper}(values, oparg)")
            vacate = output.index("IR_CLEAR_SLOTS", call)
            publish = output.index("IR_SET_TEMP", vacate)
            release = output.index("PyValue_XCLOSE(ir_old);", publish)
            self.assertLess(call, vacate)
            self.assertLess(vacate, publish)
            self.assertLess(publish, release)
            self.assertNotIn("finish_", output[call:release])

    def test_home_operations_lower_through_actual_storage(self):
        output = self.ir_output("""
            window inst(OP, (-- out[2]: owned))
                effects() exits(next)
            {
                IR_WRITE_OUTPUT(out, 0, PyValue_NULL);
                IR_WRITE_OUTPUT(out, 1, PyValue_NULL);
            }
        """)
        self.assertIn("_PyFrame_WriteHome(frame, &out[0], PyValue_NULL)", output)
        self.assertIn("_PyFrame_WriteHome(frame, &out[1], PyValue_NULL)", output)
        self.assertNotIn("IR_WRITE_OUTPUT", output)
        output = self.ir_output("""
            inst(OP, (values[oparg*2]: consumed -- map: owned))
                effects(reads, writes, allocates, raises, releases, runs_python)
                exits(next, raise)
            {
                PyObject *object = IR_BUILD_MAP(values);
                DEAD(values);
                ERROR_IF(object == NULL);
                map = PyValue_FromObjectSteal(object);
            }
        """)
        self.assertIn("_Py_BuildMap_ValueSteal(frame, values, oparg)", output)
        self.assertNotIn("IR_BUILD_MAP", output)

    def test_home_loop_operations_lower_through_actual_storage(self):
        output = self.ir_output("""
            window inst(OP, (-- out[oparg]: inline))
                effects() exits(next)
            {
                for (int i = 0; i < oparg; i++) {
                    IR_WRITE_OUTPUT(out, i, PyValue_NULL);
                }
            }
        """)
        self.assertIn("_PyFrame_WriteHome(frame, &out[i], PyValue_NULL)", output)
        output = self.ir_output("""
            inst(OP, (values[oparg]: consumed inline -- result: inline))
                effects(reads, writes, releases) exits(next)
            {
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

    def test_generator_marker_contract_checks_actual_body(self):
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        analysis = analyzer.analyze_files([str(source)])
        formats = ir_generator.compute_formats(analysis)
        inst = analysis.instructions["RETURN_GENERATOR"]
        self.assertTrue(ir_generator.generator_marker_contract(inst, formats[inst.name]))
        body = inst.parts[0].body
        for statement in ("opaque(frame);", "*slots = PyValue_NULL;",
                          "frame->base[0] = PyValue_NULL;"):
            changed = self.analyze(f"\ninst(OP, (--)) {{ {statement} }}\n")
            body.body.insert(0, changed.instructions["OP"].parts[0].body.body[0])
            try:
                self.assertFalse(ir_generator.generator_marker_contract(inst, formats[inst.name]))
            finally:
                body.body.pop(0)

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
        inst(OP, (value: consumed --)) effects(releases) exits(next, branch) {
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
        # (`opaque(frame)` is no row: the verifier refuses it before the
        # recipe is asked, so the raw operations here are stores.)
        for raw in ("frame->return_slot = 0;", "frame->base[0] = PyValue_NULL;"):
            with self.subTest(raw=raw):
                changed = self.analyze(
                    source.replace("PyValue_XCLOSE(value);", raw + " PyValue_XCLOSE(value);")
                          .replace("effects(releases)", "effects(reads, writes, releases)"))
                fmt = ir_generator.compute_formats(changed)
                self.assertEqual(
                    ir_generator.handler_contracts(changed, fmt)["OP"].ownership_events, ())
        actual = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        real = analyzer.analyze_files([str(actual)])
        formats = ir_generator.compute_formats(real)
        contracts = ir_generator.handler_contracts(real, formats)
        for name in ("IS_OP_JUMP", "INSTRUMENTED_IS_OP_JUMP",
                     "COMPARE_OP_JUMP", "COMPARE_OP_JUMP_INT",
                     "CONTAINS_OP_JUMP",
                     "POP_ITER", "INSTRUMENTED_POP_ITER"):
            with self.subTest(handler=name):
                self.assertEqual(contracts[name].ownership_events, (
                    ir_generator.OwnershipEvent("_PyIR_OWNER_NORMAL_CLEAR_INPUTS"),))

    def test_actual_call_role_families_have_normal_drain_recipes(self):
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        analysis = analyzer.analyze_files([str(source)])
        formats = ir_generator.compute_formats(analysis)
        contracts = ir_generator.handler_contracts(analysis, formats)
        # The handlers of the call families take the role flags in the word
        roles = {name: contract.word_flags for name, contract in contracts.items()
                 if contract.word_flags}
        self.assertEqual(len(roles), 9)
        self.assertEqual(set(roles.values()),
                         {("IR_F_NO_SELF",), ("IR_F_NO_KW",)})
        self.assertEqual({name for name, flags in roles.items()
                          if flags == ("IR_F_NO_KW",)},
                         {"CALL_FUNCTION_EX", "CALL_EX_PY", "CALL_EX_NON_PY_GENERAL",
                          "INSTRUMENTED_CALL_FUNCTION_EX"})
        for name in roles:
            with self.subTest(handler=name):
                self.assertEqual(contracts[name].ownership_events, (
                    ir_generator.OwnershipEvent("_PyIR_OWNER_CALL_DRAIN"),))

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
        # taken edge: V13 refuses a target held in a local
        text = source.read_text()
        old = "IR_COMPLETE_STEP(_PY_IR_FOR_ITER, completed, oparg);"
        self.assertIn(old, text)
        with open(self.temp_input_filename, "w") as stream:
            changed_route = "int changed_target = oparg + 1;\n" + old.replace("oparg", "changed_target")
            stream.write(text.replace(old, changed_route, 1))
        changed = analyzer.analyze_files([self.temp_input_filename])
        changed_formats = ir_generator.compute_formats(changed)
        with self.assertRaisesRegex(SyntaxError,
                r"IR_COMPLETE_STEP's target is `changed_target`, which is no operand"):
            ir_generator.handler_contracts(changed, changed_formats)

    def test_normal_transfer_real_family_coverage(self):
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        analysis = analyzer.analyze_files([str(source)])
        formats = ir_generator.compute_formats(analysis)
        insts = ir_generator.handler_instructions(analysis)
        # The local, constant and shuffle stack ops other than STORE_FAST
        # are stack-only: their families cover them
        expected = {
            "LOAD_COMMON_CONSTANT": "FRESH",
            "STORE_FAST": "LOCAL_STORE",
            "IR_JUMP": "PRESERVE", "RESUME": "PRESERVE",
            "CALL_KW": "UNKNOWN", "SEND": "UNKNOWN",
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
        # float inline, so their results get the generic DUP, not the
        # _POINTER forms, although a later uop asserts an object
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            MACRO(SPAM_OWNED, ARGS(), VALUE(owned), EFFECTS(), EXITS()) \\
            MACRO(SPAM_BORROWED, ARGS(), SCALAR, EFFECTS(), EXITS()) \\
            MACRO(SPAM_USE, ARGS(object), NONE, EFFECTS(reads), EXITS())
        """)
        # The stolen object is an owner that the declared row hands out, so
        # it is bound to a local before the constructor takes it
        for ctor, mode in (("PyObject *o = SPAM_OWNED(); res = PyValue_FromObjectSteal(o);",
                            "owned"),
                           ("res = PyValue_DUP(PyValue_FromObjectBorrow(SPAM_BORROWED()));",
                            "owned"),
                           ("res = PyValue_FromObjectBorrow(SPAM_BORROWED());", "borrowed")):
            with self.subTest(ctor=ctor):
                output = self.ir_output(f"""
                op(_MAKE, (-- res: {mode}))
                    effects(reads) exits(next)
                {{
                    {ctor}
                }}
                op(_COPY, (value -- value, copy: owned))
                    effects() exits(next)
                {{
                    copy = PyValue_DUP(value);
                }}
                op(_USE, (value: consumed object, copy: consumed --))
                    effects(reads, releases) exits(next)
                {{
                    PyObject *o = PyValue_AsKnownObjectBorrow(value);
                    SPAM_USE(o);
                    DECREF_INPUTS();
                }}
                macro(OP) = _MAKE + _COPY + _USE;
                """)
                self.assertIn("copy = PyValue_DUP(value);", output)
                self.assertNotIn("_PyValue_DUP_POINTER", output)
                self.assertNotIn("PyValue_IsInline(value)", output)

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
        specializing op(_SPECIALIZE_SPAM, (owner -- owner))
            effects(writes) exits(next)
            feedback(read(entry), write(entry))
        {
            _PyFbHeader *feedback = FB_COUNTER();
            _PyFb_AdvanceCounter(feedback);
        }
        op(_SPAM, (owner: consumed -- res: moved(owner)))
            effects() exits(next)
        {
            res = TAKE(owner);
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

    def test_ir_only_takes_free_numbers(self):
        # IR-only handlers take the numbers below 256 that no opcode holds;
        # a pseudo-instruction has no handler, so it reserves none.
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
        self.assertNotIn(256, analysis.free_ids)
        self.assertNotIn(analysis.opmap["GO"], analysis.free_ids)
        self.assertEqual(ir_generator.handler_ids(analysis)["IR_BACON"],
                         analysis.free_ids[0])

    def test_ir_only_continue_from_256(self):
        # Past the free numbers, handlers continue from 256 and the arms
        # follow them.
        input = """
        inst(GO, ( -- )) {
            JUMPBY(oparg);
        }
        ir_only inst(IR_BACON, ( -- )) {
            SPAM();
        }
        ir_only inst(IR_PASS, ( -- )) {
            SPAM();
        }
    """
        analysis = self.analyze(input)
        analysis.free_ids = analysis.free_ids[:1]
        ids = ir_generator.handler_ids(analysis)
        self.assertEqual((ids["IR_BACON"], ids["IR_PASS"]),
                         (analysis.free_ids[0], 256))
        self.assertEqual(ir_generator.num_old_handlers(analysis), 257)

    def test_binary_op_handlers(self):
        # BINARY_OP has one generic handler per operation, with the operation
        # as a constant, after the other IR-only handlers.  Its
        # specialisations deoptimise to the handler of the operation that
        # their entry records.
        self.add_rows("""#define T(PRIMITIVE, HELPER, MACRO) \\
            MACRO(SPAM_BINARY, ARGS(borrowed, borrowed, scalar), VALUE_OR_NULL(owned), \\
                  EFFECTS(runs_python, raises), EXITS()) \\
            MACRO(HAM_VALUE, ARGS(borrowed, borrowed), VALUE(owned), EFFECTS(), EXITS())
        """)
        input = """
        family(BINARY_OP, 1) = { BINARY_OP_ADD_INT };
        specializing op(_SPECIALIZE_BINARY_OP, (lhs, rhs -- lhs, rhs))
            effects(writes) exits(next)
            feedback(read(entry), write(entry))
        {
            _PyFbHeader *feedback = FB_COUNTER();
            _PyFb_AdvanceCounter(feedback);
        }
        op(_BINARY_OP, (lhs: consumed, rhs: consumed -- res: owned))
            effects(runs_python) exits(next, raise)
        {
            res = SPAM_BINARY(lhs, rhs, oparg);
            DECREF_INPUTS();
            ERROR_IF(PyValue_IsNull(res));
        }
        macro(BINARY_OP) = _SPECIALIZE_BINARY_OP + _BINARY_OP;
        op(_GUARD_INT, (left, right -- left, right))
            effects() exits(next, deopt)
        {
            DEOPT_IF(!EGGS(left, right), "eggs");
        }
        op(_ADD_INT, (left: consumed, right: consumed -- res: owned))
            effects(releases) exits(next)
        {
            res = HAM_VALUE(left, right);
            DECREF_INPUTS();
        }
        macro(BINARY_OP_ADD_INT) = unused/1 + _GUARD_INT + _ADD_INT;
        ir_only inst(IR_BACON, ( -- ))
            effects() exits(next)
        {
            SPAM_FLAG();
        }
    """
        analysis = self.analyze(input)
        handlers = ir_generator.binary_op_handlers(analysis)
        self.assertEqual(handlers[0], "BINARY_OP_ADD")
        ids = ir_generator.handler_ids(analysis)
        self.assertNotIn("BINARY_OP", ids)
        self.assertEqual([ids["IR_BACON"], *(ids[name] for name in handlers)],
                         analysis.free_ids[:1 + len(handlers)])
        formats = ir_generator.compute_formats(analysis)
        self.assertIs(formats["BINARY_OP_ADD"], formats["BINARY_OP"])
        self.assertEqual(
            ir_metadata_generator.shadow_of("BINARY_OP_ADD", analysis),
            "BINARY_OP")

        out = io.StringIO()
        ir_generator.generate_ir([self.temp_input_filename], analysis, out, False)
        code = out.getvalue()
        self.assertNotIn("TARGET(BINARY_OP)", code)
        # The generic handlers set their operation and enter one body
        generic = code[code.index("TARGET(BINARY_OP_SUBTRACT)"):]
        generic = generic[:generic.index("}")]
        self.assertIn("binary_op_operation = NB_SUBTRACT;", generic)
        self.assertIn("goto LABEL_binary_op;", generic)
        self.assertEqual(code.count("LABEL(binary_op)"), 1)
        shared = code[code.index("LABEL(binary_op)"):]
        shared = shared[:shared.index("DISPATCH();")]
        self.assertIn("int oparg = binary_op_operation;", shared)
        self.assertIn("INSTRUCTION_STATS(_PyIR_BINARY_OP_FIRST + binary_op_operation);",
                      shared)
        handler = code[code.index("TARGET(BINARY_OP_ADD_INT)"):]
        handler = handler[:handler.index("DISPATCH();")]
        self.assertIn("IR_DEOPT_BINARY_OP(IR_D(ir_word));", handler)
        self.assertNotIn("JUMP_TO_PREDICTED", handler)

        out = io.StringIO()
        ir_metadata_generator.generate_ir_metadata(
            [self.temp_input_filename], analysis, out)
        lines = [line.strip() for line in out.getvalue().splitlines()]
        self.assertIn("#define _PyIR_OBSERVER_ENTRY_ABSENCE 0", lines)
        self.assertIn("#define _PyIR_BINARY_OP_FIRST" + " " * 26
                      + "BINARY_OP_ADD", lines)
        self.assertIn('static_assert(BINARY_OP_ADD == _PyIR_BINARY_OP_FIRST + '
                      'NB_ADD, "BINARY_OP_ADD");', lines)

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


class TestShapeReadMetadata(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.temp_input_filename = str(Path(directory.name) / "source.c")

    # The declarations of the two uops, as Python/bytecodes.c has them.  The
    # verifier of a declaration checks a uop's body before the generator's
    # shape-read rules run, so an edit that the rules are to refuse has its
    # declaration changed to what the edited body does (`redeclared`); one
    # that the verifier refuses is checked against the real declaration.
    GUARD_DECLARATION = (
        "                (type_version/2, shape/0, owner: object\n"
        "                 -- owner: shape(shape) type_version(type_version)))\n"
        "            effects(reads) exits(next, miss)\n")
    LOAD_DECLARATION = (
        "leaf op(_LOAD_ATTR_SHAPE, (offset/1, owner: consumed -- attr: owned))\n"
        "            effects(reads, releases) exits(next)\n")

    def redeclared(self, source, old, new):
        self.assertIn(old, source)
        return source.replace(old, new, 1)

    def shape_read_handler(self, source, name):
        "The handler name that source generates, and its effects"
        Path(self.temp_input_filename).write_text(source)
        analysis = analyzer.analyze_files([self.temp_input_filename])
        formats = ir_generator.compute_formats(analysis)
        output = io.StringIO()
        out = CWriter(output, 0, False)
        emitter = ir_generator.IREmitter(out, analysis, formats)
        instruction = ir_generator.handler_instructions(analysis)[name]
        ir_generator.generate_handler(name, instruction, formats[name], emitter, out)
        record = ir_generator.build_record(name, instruction, formats[name], analysis, formats)
        return output.getvalue(), ir_generator.handler_effects(record)

    def test_zero_width_second_cache_has_no_encoded_operand(self):
        source = """
        // BEGIN BYTECODES //
        op(_SECOND_ZERO, (first/2, second/0 --)) {
            (void)first; (void)second;
        }
        op(_BOTH_ENCODED, (first/2, second/1 --)) {
            (void)first; (void)second;
        }
        op(_FIRST_ZERO, (first/0, second/1 --)) {
            (void)first; (void)second;
        }
        macro(SECOND_ZERO) = unused/1 + _SECOND_ZERO;
        macro(BOTH_ENCODED) = unused/1 + _BOTH_ENCODED;
        macro(FIRST_ZERO) = unused/1 + _FIRST_ZERO;
        // END BYTECODES //
        """
        Path(self.temp_input_filename).write_text(source)
        analysis = analyzer.analyze_files([self.temp_input_filename])
        output = io.StringIO()
        opcode_metadata_generator.generate_expansion_table(analysis, CWriter(output, 0, False))
        table = output.getvalue()
        self.assertNotIn("OPERAND1_0", table)
        self.assertIn("[SECOND_ZERO] = { .nuops = 1, .uops = { { _SECOND_ZERO, 2, 1 } } }", table)
        self.assertIn("[BOTH_ENCODED] = { .nuops = 2, .uops = { { _BOTH_ENCODED, 2, 1 }, { _BOTH_ENCODED, OPERAND1_1, 3 } } }", table)
        self.assertIn("[FIRST_ZERO] = { .nuops = 2, .uops = { { _FIRST_ZERO, 0, 1 }, { _FIRST_ZERO, OPERAND1_1, 1 } } }", table)


class TestCallMarkers(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        source = str(ir_generator.DEFAULT_INPUT)
        cls.analysis = analyzer.analyze_files([source])
        output = io.StringIO()
        ir_generator.generate_ir([source], cls.analysis, output, False)
        cls.generated = output.getvalue()
        cls.interpreter = expand_outlined(cls.generated)

    def test_every_handler_has_a_function_form(self):
        # tiers87, C4: the forms section defines ir_run_NAME for every
        # handler of the evaluator, and a table of them by handler number.
        # A form leaves only through the transfer macros.
        forms, inline = self.generated.split("#else  // IR_FUNCTION_FORMS")
        forms = forms[forms.index("#ifdef IR_FUNCTION_FORMS"):]
        handlers = set(re.findall(r"^\s*(?:HOT_)?TARGET\((\w+)\)(?: \{)?$", inline, re.M))
        defined = set(re.findall(r"^ir_run_(\w+)\($", forms, re.M))
        # A floor, not a count: ported families delete old handlers
        self.assertGreater(len(handlers), 150)
        self.assertEqual(handlers, defined)
        table = re.findall(r"^    \[(\d+)\] = ir_run_(\w+),$", forms, re.M)
        self.assertEqual([int(i) for i, _ in table], list(range(len(table))))
        self.assertLessEqual({name for _, name in table}, defined)
        for name in ("start_frame", "throw_error"):
            self.assertIn(f"\nir_{name}(\n", forms)
        self.assertIsNone(re.search(
            r"\bgoto\b|\bLABEL_\w+|\blastopcode\b", forms))

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
        for name in ("_MAYBE_EXPAND_METHOD_KW", "_MONITOR_CALL_KW",
                     "_MAKE_CALLARGS_A_TUPLE", "_DO_CALL_KW", "_PY_FRAME_KW",
                     "_PY_FRAME_KW_GUARDED", "_DO_CALL_FUNCTION_EX"):
            with self.subTest(name=name):
                uop = self.analysis.uops[name]
                self.assertTrue(uop.properties.escapes)
                self.assertTrue(uop.properties.error_without_pop)
                self.assertFalse(uop.properties.error_with_pop)
                self.assertTrue(uop.properties.sync_sp)
                self.assertTrue(uop.properties.needs_this)
                self.assertFalse(uop.is_viable())
        for name in ("_DO_CALL_KW", "_PY_FRAME_KW_GUARDED"):
            invoke = self.analysis.uops[name].properties
            self.assertTrue(invoke.always_exits)
            self.assertTrue(invoke.switches_frame)
        self.assertFalse(self.analysis.uops["_MONITOR_CALL_KW"].properties.always_exits)

    def test_pure_latch_and_undeclared_macro(self):
        # A row decides, never an upper-case name: the latch declares no
        # effect, a macro with no row escapes
        node, = parse_src("op(_OP, (--)) { UNRELATED_MACRO(); }")
        self.assertTrue(analyzer.compute_properties(node).escapes)
        for marker in ("IR_BEGIN_CALL_EFFECTS",):
            node, = parse_src(f"op(_OP, (--)) {{ {marker}(); }}")
            properties = analyzer.compute_properties(node)
            self.assertFalse(properties.escapes)
            self.assertFalse(properties.error_without_pop)
            self.assertFalse(properties.always_exits)
            self.assertFalse(properties.switches_frame)
            self.assertFalse(properties.sync_sp)

    def test_prelude_and_actual_destination_terminal_paths(self):
        for name in ("CALL_KW", "CALL_KW_NON_PY", "CALL_KW_PY",
                     "CALL_FUNCTION_EX", "CALL_EX_NON_PY_GENERAL", "CALL_EX_PY",
                     "INSTRUMENTED_CALL_KW", "INSTRUMENTED_CALL_FUNCTION_EX"):
            with self.subTest(name=name):
                body = self.interpreter.split(f"TARGET({name}) {{", 1)[1].split("TARGET(", 1)[0]
                # The out-of-line run reads the operand words; the handler
                # keeps no record of the call of its own
                invoke = min(body.index(run) for run in (
                    "call_run_", "call_bind_") if run in body)
                for preparation in ("_Py_Specialize_CallKw",):
                    if preparation in body:
                        self.assertLess(body.index(preparation), invoke)
                self.assertNotIn("IR_MOVE_TO_DST", body[invoke:])
                self.assertNotIn("IR_CLEAR_SLOTS", body[invoke:])
                self.assertNotIn("_PyValue_Publish", body[invoke:])
                self.assertNotIn("_Py_VectorCall", body)
                self.assertIn("DISPATCH_INLINED(call_result.child)", body)
                # The call's locals live in the out-of-line run (R-c)
                self.assertNotIn("&call", body)

    def test_handlers_take_no_local_address(self):
        # The generated handlers and labels take the address of none of
        # their locals: the check of the uop bodies, applied to what the
        # generator emits.  No handler keeps a value record or a box area.
        _, rest = self.interpreter.split(ir_generator.INSTRUCTION_START_MARKER)
        sites = generated_address_sites(rest)
        self.assertGreater(len(sites), 150)
        for name, found in sites.items():
            self.assertEqual(found, [], name)

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
                    (IR_IS_LOCAL_FIELD(ir_word, A) ? &attr : NULL));
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
