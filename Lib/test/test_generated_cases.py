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
    import ir_metadata_generator
    import parser
    from stack import Local, Stack


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
            analysis = analyzer.analyze_files([self.temp_input_filename])
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
            return analyzer.analyze_files([self.temp_input_filename])

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

    def run_ir_test(self, input: str, expected: str):
        self.assertEqual(self.ir_output(input).strip(), expected.strip())

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
            res = PyValue_FromPyObjectSteal(o);
        }
    """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            _PyInterpreterFrame *frame = tstate->current_frame;
            #endif
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            #if _Py_TAIL_CALL_INTERP
            PyObject *value_boxes[5];
            _PyValueTemps value_owners;
            #endif
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
            right = IR_VIEW(IR_FIELD_C(this_instr));
            left = IR_VIEW(IR_FIELD_B(this_instr));
            if (Py_UNLIKELY(PyValue_IsInline(left))) {
                ir_obj_left = _PyValueTemps_Box(value_temps, left);
                if (ir_obj_left == NULL) {
                    JUMP_TO_LABEL(error);
                }
                BOX_STAT_INC(OP);
            }
            else {
                ir_obj_left = PyValue_AsKnownObjectBorrow(left);
            }
            if (Py_UNLIKELY(PyValue_IsInline(right))) {
                ir_obj_right = _PyValueTemps_Box(value_temps, right);
                if (ir_obj_right == NULL) {
                    JUMP_TO_LABEL(error);
                }
                BOX_STAT_INC(OP);
            }
            else {
                ir_obj_right = PyValue_AsKnownObjectBorrow(right);
            }
            PyObject *l = ir_obj_left;
            PyObject *r = ir_obj_right;
            PyObject *o = spam_call(l, r);
            ir_own_C = IR_CONSUME_INPUT(IR_FIELD_C(this_instr), right);
            PyValue_XCLOSE(ir_own_C);
            ir_own_B = IR_CONSUME_INPUT(IR_FIELD_B(this_instr), left);
            PyValue_XCLOSE(ir_own_B);
            if (o == NULL) {
                JUMP_TO_LABEL(error);
            }
            res = PyValue_FromPyObjectSteal(o);
            IR_STORE_DST(IR_FIELD_A(this_instr), res);
            if (!PyValue_IsNull(ir_displaced) &&
                finish_displaced_local(tstate, frame, next_instr, value_temps, ir_displaced) < 0) {
                next_instr = frame->instr_ptr + 1;
                JUMP_TO_LABEL(error);
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
            res = PyValue_FromPyObjectSteal(SPAM(o));
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
        self.assertIn("value = IR_VIEW(IR_FIELD_A(this_instr));", close)
        self.assertIn("ir_own_A = IR_CONSUME_INPUT(IR_FIELD_A(this_instr), value);\n"
                      "            PyValue_XCLOSE(ir_own_A);", close)
        # No release of a view
        self.assertNotRegex(output, r"\n\s*(_?PyValue_\w*CLOSE\w*)\(value")
        self.assertIn("ir_own_B = IR_CONSUME_INPUT(IR_FIELD_B(this_instr), value);\n"
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
            "ir_own_B = IR_CONSUME_INPUT(IR_FIELD_B(this_instr), copy);")
        self.assertLess(output.index("// _FIRST"), consume)
        self.assertLess(consume, output.index("// _SECOND"))
        self.assertIn("res = IR_TAKE(copy, ir_own_B);", output)
        self.assertIn("PyValueBorrow copy;", output)

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
        self.assertIn("ir_own_A = IR_CONSUME_INPUT(IR_FIELD_A(this_instr), value);\n"
                      "            PyValue held = ir_own_A;", output)
        self.assertIn("retire_published_values(", output)
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
                        output.index("JUMP_TO_LABEL(error);"))

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
            res = PyValue_FromPyObjectSteal(SPAM(PyValue_AsKnownObjectBorrow(a)));
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
            res = PyValue_FromPyObjectSteal(o);
        }
        inst(OP_OBJECT_COPY_RELEASE, (value --)) {
            PyObject *o = PyValue_AsKnownObjectBorrow(value);
            PyObject *p;
            p = o;
            DEAD(value);
            Py_DECREF(p);
        }
        inst(OP_OBJECT_NESTED_STEAL, (value -- res)) {
            res = PyValue_FromPyObjectSteal(PyValue_AsKnownObjectBorrow(value));
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
            res = PyValue_FromPyObjectSteal(_PyValueTemps_Object(value_temps, value));
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
        self.assertIn("ir_own_A = IR_CONSUME_INPUT(IR_FIELD_A(this_instr), owner);\n"
                      "            PyValue_XCLOSE(ir_own_A);", store)
        self.assertIn("ir_own_B = IR_CONSUME_INPUT(IR_FIELD_B(this_instr), value);\n"
                      "            PyValue_XCLOSE(ir_own_B);", store)
        self.assertIn("ir_own_A = IR_CONSUME_INPUT(IR_FIELD_A(this_instr), cond);",
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
        self.assertIn("IR_STORE_DST(IR_FIELD_A(this_instr), res);",
                      self.handler(output, "OP"))
        self.assertIn("IR_STORE_DST(IR_FIELD_A(this_instr), attr);",
                      self.handler(output, "ATTR"))
        self.assertNotIn("IR_STORE_DST", self.handler(output, "ALIAS"))
        self.assertIn("IR_STORE_TEMP(IR_SLOT(IR_FIELD_A(this_instr)), res);",
                      self.handler(output, "ALIAS"))
        self.assertNotIn("IR_STORE_DST", self.handler(output, "PAIR"))
        self.assertIn("assert(!IR_IS_LOCAL(IR_FIELD_A(this_instr)));",
                      self.handler(output, "PAIR"))

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
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            _PyInterpreterFrame *frame = tstate->current_frame;
            _PyValueTemps *const value_temps = NULL;
            (void)value_temps;
            #endif
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            PyValue ir_displaced = PyValue_NULL;
            (void)ir_displaced;
            int oparg = (int)IR_FIELD_C(this_instr);
            (void)oparg;
            PyValue *window = &IR_SLOT(IR_FIELD_B(this_instr));
            (void)window;
            PyValue callable;
            PyValue *args;
            PyValue res;
            args = &window[1];
            callable = window[0];
            res = SPAM(callable, args, oparg);
            IR_CLEAR_SLOTS(&window[1], oparg);
            IR_PUBLISH_TEMP(window[0], res);
            IR_MOVE_TO_DST(IR_FIELD_A(this_instr), window[0]);
            if (!PyValue_IsNull(ir_displaced) &&
                finish_displaced_local(tstate, frame, next_instr, NULL, ir_displaced) < 0) {
                next_instr = frame->instr_ptr + 1;
                JUMP_TO_LABEL(error);
            }
            DISPATCH();
        }
    """
        self.run_ir_test(input, output)
        fmt = self.formats(input)["OP"]
        self.assertTrue(fmt.window)
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
            res = PyValue_FromPyObjectSteal(PyValue_AsKnownObjectSteal(v));
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
        # A family shares one format. A name index, which may not fit in a
        # field, moves into the entry's arg field, and cache effects read the
        # entry.  The sites past the last index share entry 0 and run the
        # NOFB form of the generic handler, which carries the oparg in the
        # word; a specialisation never does.
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
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            _PyInterpreterFrame *frame = tstate->current_frame;
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            IR_PREDICTED(OP);
            _PyIRWord* const this_instr = next_instr - 1;
            (void)this_instr;
            #if _Py_TAIL_CALL_INTERP
            PyObject *value_boxes[6];
            _PyValueTemps value_owners;
            #endif
            value_owners = (_PyValueTemps){value_boxes, 0, 6};
            _PyValueTemps *value_temps = &value_owners;
            PyValue ir_displaced = PyValue_NULL;
            (void)ir_displaced;
            _PyFb_OP *fb = (_PyFb_OP *)IR_FB(IR_FIELD_C(this_instr));
            int oparg = IR_FB_ARG(fb, IR_FIELD_C(this_instr));
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
                owner = IR_VIEW(IR_FIELD_B(this_instr));
                if (Py_UNLIKELY(PyValue_IsInline(owner))) {
                    ir_obj_owner = _PyValueTemps_Box(value_temps, owner);
                    if (ir_obj_owner == NULL) {
                        JUMP_TO_LABEL(error);
                    }
                    BOX_STAT_INC(OP);
                }
                else {
                    ir_obj_owner = PyValue_AsKnownObjectBorrow(owner);
                }
                PyObject *o = ir_obj_owner;
                res = SPAM(o, GETITEM(FRAME_CO_NAMES, oparg));
                ir_own_B = IR_CONSUME_INPUT(IR_FIELD_B(this_instr), owner);
                PyValue_XCLOSE(ir_own_B);
            }
            IR_STORE_DST(IR_FIELD_A(this_instr), res);
            if (!PyValue_IsNull(ir_displaced) &&
                finish_displaced_local(tstate, frame, next_instr, value_temps, ir_displaced) < 0) {
                next_instr = frame->instr_ptr + 1;
                JUMP_TO_LABEL(error);
            }
            DISPATCH();
        }

        TARGET(OP_NOFB) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP_NOFB;
            (void)(opcode);
            _PyInterpreterFrame *frame = tstate->current_frame;
            #endif
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            #if _Py_TAIL_CALL_INTERP
            PyObject *value_boxes[6];
            _PyValueTemps value_owners;
            #endif
            value_owners = (_PyValueTemps){value_boxes, 0, 6};
            _PyValueTemps *value_temps = &value_owners;
            PyValue ir_displaced = PyValue_NULL;
            (void)ir_displaced;
            _PyFb_OP *fb = (_PyFb_OP *)IR_FB(0);
            int oparg = (int)IR_FIELD_CD(this_instr);
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
                owner = IR_VIEW(IR_FIELD_B(this_instr));
                if (Py_UNLIKELY(PyValue_IsInline(owner))) {
                    ir_obj_owner = _PyValueTemps_Box(value_temps, owner);
                    if (ir_obj_owner == NULL) {
                        JUMP_TO_LABEL(error);
                    }
                    BOX_STAT_INC(OP);
                }
                else {
                    ir_obj_owner = PyValue_AsKnownObjectBorrow(owner);
                }
                PyObject *o = ir_obj_owner;
                res = SPAM(o, GETITEM(FRAME_CO_NAMES, oparg));
                ir_own_B = IR_CONSUME_INPUT(IR_FIELD_B(this_instr), owner);
                PyValue_XCLOSE(ir_own_B);
            }
            IR_STORE_DST(IR_FIELD_A(this_instr), res);
            if (!PyValue_IsNull(ir_displaced) &&
                finish_displaced_local(tstate, frame, next_instr, value_temps, ir_displaced) < 0) {
                next_instr = frame->instr_ptr + 1;
                JUMP_TO_LABEL(error);
            }
            DISPATCH();
        }

        TARGET(OP_SPECIALIZED) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP_SPECIALIZED;
            (void)(opcode);
            _PyInterpreterFrame *frame = tstate->current_frame;
            _PyValueTemps *const value_temps = NULL;
            (void)value_temps;
            #endif
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP_SPECIALIZED);
            PyValue ir_displaced = PyValue_NULL;
            (void)ir_displaced;
            _PyFb_OP *fb = (_PyFb_OP *)IR_FB(IR_FIELD_C(this_instr));
            int oparg = (int)((_PyFbHeader *)fb)->arg;
            (void)oparg;
            PyValueBorrow owner;
            PyValue res;
            PyValue ir_own_B = PyValue_NULL;
            (void)ir_own_B;
            // _GUARD
            {
                owner = IR_VIEW(IR_FIELD_B(this_instr));
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
                ir_own_B = IR_CONSUME_INPUT(IR_FIELD_B(this_instr), owner);
                _PyValue_XCLOSE_POINTER(ir_own_B);
            }
            IR_STORE_DST(IR_FIELD_A(this_instr), res);
            if (!PyValue_IsNull(ir_displaced) &&
                finish_displaced_local(tstate, frame, next_instr, NULL, ir_displaced) < 0) {
                next_instr = frame->instr_ptr + 1;
                JUMP_TO_LABEL(error);
            }
            DISPATCH();
        }


    """
        self.run_ir_test(input, output)
        fmt = self.formats(input)["OP_SPECIALIZED"]
        self.assertEqual(fmt.group, "OP")
        self.assertEqual(fmt.fb, ir_generator.LOC_C)
        self.assertEqual(fmt.imm, ir_generator.LOC_FBARG)

    def test_oparg_in_entry(self):
        # Two inputs, a destination and an entry fill the word, so the oparg
        # lives in the entry, or for the NOFB form in the field of the entry.
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
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            _PyInterpreterFrame *frame = tstate->current_frame;
            _PyValueTemps *const value_temps = NULL;
            (void)value_temps;
            #endif
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            PyValue ir_displaced = PyValue_NULL;
            (void)ir_displaced;
            _PyFb_OP *fb = (_PyFb_OP *)IR_FB(IR_FIELD_D(this_instr));
            int oparg = IR_FB_ARG(fb, IR_FIELD_D(this_instr));
            (void)oparg;
            PyValue left;
            PyValue right;
            PyValue res;
            right = IR_SLOT(IR_FIELD_C(this_instr));
            left = IR_SLOT(IR_FIELD_B(this_instr));
            uint32_t counter = fb->counter;
            (void)counter;
            _PyFbHeader *feedback = ((_PyFbHeader *)fb);
            ADVANCE_ADAPTIVE_COUNTER(feedback);
            res = SPAM(left, right, oparg);
            IR_SLOT(IR_FIELD_B(this_instr)) = PyValue_NULL;
            IR_SLOT(IR_FIELD_C(this_instr)) = PyValue_NULL;
            IR_STORE_DST(IR_FIELD_A(this_instr), res);
            if (!PyValue_IsNull(ir_displaced) &&
                finish_displaced_local(tstate, frame, next_instr, NULL, ir_displaced) < 0) {
                next_instr = frame->instr_ptr + 1;
                JUMP_TO_LABEL(error);
            }
            DISPATCH();
        }

        TARGET(OP_NOFB) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP_NOFB;
            (void)(opcode);
            _PyInterpreterFrame *frame = tstate->current_frame;
            _PyValueTemps *const value_temps = NULL;
            (void)value_temps;
            #endif
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            PyValue ir_displaced = PyValue_NULL;
            (void)ir_displaced;
            _PyFb_OP *fb = (_PyFb_OP *)IR_FB(0);
            int oparg = (int)IR_FIELD_D(this_instr);
            (void)oparg;
            PyValue left;
            PyValue right;
            PyValue res;
            right = IR_SLOT(IR_FIELD_C(this_instr));
            left = IR_SLOT(IR_FIELD_B(this_instr));
            uint32_t counter = fb->counter;
            (void)counter;
            _PyFbHeader *feedback = ((_PyFbHeader *)fb);
            ADVANCE_ADAPTIVE_COUNTER(feedback);
            res = SPAM(left, right, oparg);
            IR_SLOT(IR_FIELD_B(this_instr)) = PyValue_NULL;
            IR_SLOT(IR_FIELD_C(this_instr)) = PyValue_NULL;
            IR_STORE_DST(IR_FIELD_A(this_instr), res);
            if (!PyValue_IsNull(ir_displaced) &&
                finish_displaced_local(tstate, frame, next_instr, NULL, ir_displaced) < 0) {
                next_instr = frame->instr_ptr + 1;
                JUMP_TO_LABEL(error);
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
            #if _Py_TAIL_CALL_INTERP
            int opcode = IR_PASS;
            (void)(opcode);
            _PyInterpreterFrame *frame = tstate->current_frame;
            _PyValueTemps *const value_temps = NULL;
            (void)value_temps;
            #endif
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
        # The oparg names the local that LOAD reads (_PyIR_ROLE_LOCAL_IN).
        # Only the twin runs Python itself; the format does, as the twin's.
        load = ("{ .flags = _PyIR_VALID | _PyIR_STORE_DST | _PyIR_RUNS_CODE, "
                ".effects = 0, "
                ".roles = { _PyIR_ROLE_DST, "
                "_PyIR_ROLE_LOCAL_IN, _PyIR_ROLE_NONE, _PyIR_ROLE_NONE }, "
                ".imm = _PyIR_LOC_B, .fb = _PyIR_LOC_NONE, .ninputs = 0, "
                ".dst = 0, .borrow_mask = 0x0, .fb_kind = 0, .fb_size = 0, "
                ".reads = { 0, _PyIR_EXT_FIXED }, "
                ".writes = { 1, _PyIR_EXT_FIXED }, "
                ".through = { 0, _PyIR_EXT_FIXED }, .shadow = LOAD, "
                ".nofb = 0, .twin = INSTRUMENTED_LOAD, .events = 0 }")
        instrumented = (load.replace(".effects = 0", ".effects = _PyIR_RUNS_PYTHON")
                            .replace("shadow = LOAD", "shadow = INSTRUMENTED_LOAD"))
        for expected in [
            "#define IR_BACON                                       256",
            "#define IR_PASS                                        257",
            "#define IR_NUM_HANDLERS                                258",
            f"[LOAD] = {load},",
            f"[INSTRUMENTED_LOAD] = {instrumented},",
            "[IR_PASS] = { .flags = _PyIR_VALID | _PyIR_TRANSPARENT, "
            ".effects = 0, "
            ".roles = { _PyIR_ROLE_NONE, _PyIR_ROLE_NONE, _PyIR_ROLE_NONE, "
            "_PyIR_ROLE_NONE }, .imm = _PyIR_LOC_NONE, .fb = _PyIR_LOC_NONE, "
            ".ninputs = 0, .dst = _PyIR_NO_DST, .borrow_mask = 0x0, "
            ".fb_kind = 0, .fb_size = 0, .reads = { 0, _PyIR_EXT_FIXED }, "
            ".writes = { 0, _PyIR_EXT_FIXED }, "
            ".through = { 0, _PyIR_EXT_FIXED }, .shadow = IR_PASS, "
            ".nofb = 0, .twin = 0, .events = 0 },",
            '[IR_BACON - IR_FIRST_ONLY_HANDLER] = "IR_BACON",',
            '[IR_PASS - IR_FIRST_ONLY_HANDLER] = "IR_PASS",',
        ]:
            self.assertIn(expected, lines)

    def slot_role(self, effect: str, body: str) -> str | None:
        analysis = self.analyze(f"\ninst(OP, ({effect})) {{\n{body}\n}}\n")
        return ir_generator.oparg_slot_role([analysis.instructions["OP"]])

    def test_constructor_results_are_not_known_objects(self):
        # The PyValue_FromPyObject*() constructors make an exact int or
        # float inline, so their results get the generic DUP and an inline
        # test before an object read, not the _POINTER forms
        for ctor in ("Steal", "New", "Borrow"):
            with self.subTest(ctor=ctor):
                output = self.ir_output(f"""
                op(_MAKE, (-- res)) {{
                    res = PyValue_FromPyObject{ctor}(SPAM());
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
             " res = PyValue_FromPyObjectNew(o);", "_PyIR_ROLE_LOCAL_IN"),
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
             " value = PyValue_FromPyObjectSteal(PyCell_GetRef((PyCellObject *)c));",
             "_PyIR_ROLE_CELL"),
            ("-- value", "value = PyValue_FromPyObjectNew(SPAM(oparg));", None),
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
        self.assertIn("INSTRUCTION_STATS(BINARY_OP);", generic)
        handler = code[code.index("TARGET(BINARY_OP_ADD_INT)"):]
        handler = handler[:handler.index("DISPATCH();")]
        self.assertIn("IR_DEOPT_BINARY_OP(IR_FIELD_D(this_instr));", handler)
        self.assertNotIn("JUMP_TO_PREDICTED", handler)

        out = io.StringIO()
        ir_metadata_generator.generate_ir_metadata(
            [self.temp_input_filename], analysis, out)
        lines = [line.strip() for line in out.getvalue().splitlines()]
        self.assertIn("#define _PyIR_BINARY_OP_FIRST" + " " * 26
                      + "BINARY_OP_ADD", lines)
        self.assertIn('static_assert(BINARY_OP_ADD == _PyIR_BINARY_OP_FIRST + '
                      'NB_ADD, "BINARY_OP_ADD");', lines)

class TestCallTransactionMarkers(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        source = str(ir_generator.DEFAULT_INPUT)
        cls.analysis = analyzer.analyze_files([source])
        output = io.StringIO()
        ir_generator.generate_ir([source], cls.analysis, output, False)
        cls.interpreter = output.getvalue()

    def test_real_callout_and_transfer_properties(self):
        for name in ("_MAYBE_EXPAND_METHOD", "_MONITOR_CALL",
                     "_MAKE_CALLARGS_A_TUPLE", "_DO_CALL", "_PY_FRAME_GENERAL",
                     "_DO_CALL_KW", "_PY_FRAME_KW", "_DO_CALL_FUNCTION_EX"):
            with self.subTest(name=name):
                uop = self.analysis.uops[name]
                self.assertTrue(uop.properties.escapes)
                self.assertTrue(uop.properties.error_without_pop)
                self.assertFalse(uop.properties.error_with_pop)
                self.assertTrue(uop.properties.sync_sp)
                self.assertTrue(uop.properties.needs_this)
                self.assertFalse(uop.is_viable())
        invoke = self.analysis.uops["_DO_CALL"].properties
        self.assertTrue(invoke.always_exits)
        self.assertTrue(invoke.switches_frame)
        self.assertFalse(self.analysis.uops["_MONITOR_CALL"].properties.always_exits)

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
                invoke = body.index("call_transaction_invoke(")
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
                self.assertIn("DISPATCH_INLINED(call_child)", body)
        labels = re.findall(r"^\s*(call_transaction_fallback_\w+):", self.interpreter, re.M)
        self.assertEqual(len(labels), len(set(labels)))


if __name__ == "__main__":
    unittest.main()
