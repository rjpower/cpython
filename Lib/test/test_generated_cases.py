import contextlib
import io
import os
from pathlib import Path
import re
import sys
import tempfile
import unittest

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
    import jit_helpers_generator
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
        # and may be borrowed locals.
        input = """
        inst(OP, (left, right -- res)) {
            PyObject *l = PyValue_AsPyObjectBorrow(left);
            PyObject *r = PyValue_AsPyObjectBorrow(right);
            PyObject *o = SPAM(l, r);
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
            PyObject *value_boxes[5];
            _PyValueTemps value_owners = {value_boxes, 0, Py_ARRAY_LENGTH(value_boxes), NULL};
            _PyValueTemps *value_temps = &value_owners;
            PyValue left;
            PyValue right;
            PyValue res;
            right = IR_SLOT(IR_FIELD_C(this_instr));
            left = IR_SLOT(IR_FIELD_B(this_instr));
            if (Py_UNLIKELY(PyValue_IsInline(left))) {
                left = PyValue_Box(left);
                if (PyValue_IsInline(left)) {
                    JUMP_TO_LABEL(error);
                }
                BOX_STAT_INC(OP);
                IR_KEEP_BOX(left, IR_FIELD_B(this_instr));
            }
            else {
                left = IR_BORROW_KNOWN(left, IR_FIELD_B(this_instr));
            }
            if (Py_UNLIKELY(PyValue_IsInline(right))) {
                right = PyValue_Box(right);
                if (PyValue_IsInline(right)) {
                    IR_DROP_BORROW(left, IR_FIELD_B(this_instr));
                    JUMP_TO_LABEL(error);
                }
                BOX_STAT_INC(OP);
                IR_KEEP_BOX(right, IR_FIELD_C(this_instr));
            }
            else {
                right = IR_BORROW_KNOWN(right, IR_FIELD_C(this_instr));
            }
            PyObject *l = PyValue_AsPyObjectBorrow(left);
            PyObject *r = PyValue_AsPyObjectBorrow(right);
            PyObject *o = SPAM(l, r);
            IR_CONSUME_INPUT(IR_FIELD_C(this_instr));
            _PyValue_CLOSE_POINTER(right);
            IR_CONSUME_INPUT(IR_FIELD_B(this_instr));
            _PyValue_CLOSE_POINTER(left);
            if (o == NULL) {
                JUMP_TO_LABEL(error);
            }
            res = PyValue_FromPyObjectSteal(o);
            IR_STORE_DST(IR_FIELD_A(this_instr), res);
            _PyValueTemps_CommitLocal(value_temps);
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
            res = SPAM(PyValue_AsPyObjectBorrow(value));
        }
        inst(OP_OBJ, (value -- res)) {
            res = SPAM(PyValue_AsPyObjectBorrow(value));
            PyValue_CLOSE(value);
        }
        inst(OP_FAMILY, (value -- res)) {
            res = SPAM(PyValue_AsPyObjectBorrow(value));
            PyValue_CLOSE(value);
        }
        inst(OP_FAMILY_RAW, (value -- res)) {
            res = SPAM_RAW(value);
            PyValue_CLOSE(value);
        }
        family(OP_FAMILY, 0) = { OP_FAMILY_RAW };
    """
        formats = self.formats(input)
        self.assertEqual(formats["OP_OBJ"].borrow_mask, 1)
        # An input that stays on the stack is not borrowed: the stack slot
        # of a borrowed local holds NULL.
        self.assertEqual(formats["OP_PASS"].borrow_mask, 0)
        self.assertEqual(formats["OP_PASS"].dst_offset, 0)
        # A member that consumes the input raw reads it borrowed, as
        # LOAD_FAST_BORROW would push it.
        self.assertEqual(formats["OP_FAMILY"].borrow_mask, 1)
        self.assertIs(formats["OP_FAMILY_RAW"], formats["OP_FAMILY"])

    def test_consumed_inputs(self):
        # A handler takes the inputs that it consumes: the slot of a
        # temporary becomes NULL, and a local keeps its value.  No handler
        # has a stack pointer to keep.
        input = """
        inst(STORE, (owner, value --)) {
            SPAM(owner, value);
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
        self.assertIn("IR_CONSUME_INPUT(IR_FIELD_A(this_instr));\n"
                      "            PyValue_CLOSE(owner);", store)
        self.assertIn("IR_CONSUME_INPUT(IR_FIELD_B(this_instr));\n"
                      "            PyValue_CLOSE(value);", store)
        self.assertIn("IR_CONSUME_INPUT(IR_FIELD_A(this_instr));",
                      self.handler(output, "BRANCH"))
        self.assertNotIn("stack_pointer", output)

    def test_store_dst(self):
        # Which results a folded STORE_FAST may send to a local, and how
        # the handler then finds the stack pointer: dst may name a local, so
        # from the top slot in a free field, or by counting.
        input = """
        inst(OP, (left, right -- res)) {
            res = SPAM(PyValue_AsPyObjectBorrow(left),
                       PyValue_AsPyObjectBorrow(right));
            DECREF_INPUTS();
        }
        inst(ATTR, (owner -- attr, null[oparg & 1])) {
            attr = SPAM(PyValue_AsPyObjectBorrow(owner));
            DECREF_INPUTS();
            if (oparg & 1) {
                null[0] = PyValue_NULL;
            }
        }
        no_fold inst(ALIAS, (value -- res)) {
            res = SPAM(PyValue_AsPyObjectBorrow(value));
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
            res = SPAM(PyValue_AsPyObjectBorrow(right));
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
            #endif
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            PyObject *value_boxes[5];
            _PyValueTemps value_owners = {value_boxes, 0, Py_ARRAY_LENGTH(value_boxes), NULL};
            _PyValueTemps *value_temps = &value_owners;
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
            window[0] = _PyValueTemps_Publish(value_temps, &res);
            IR_MOVE_TO_DST(IR_FIELD_A(this_instr), window[0]);
            _PyValueTemps_CommitLocal(value_temps);
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
            PyObject *o = PyValue_AsPyObjectBorrow(owner);
            res = SPAM(o, GETITEM(FRAME_CO_NAMES, oparg));
            DECREF_INPUTS();
        }
        macro(OP) = _SPECIALIZE_OP + _OP;
        op(_GUARD, (type_version/2, owner -- owner)) {
            EXIT_IF(Py_TYPE(PyValue_AsPyObjectBorrow(owner))->tp_version_tag != type_version);
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
            PyObject *value_boxes[6];
            _PyValueTemps value_owners = {value_boxes, 0, Py_ARRAY_LENGTH(value_boxes), NULL};
            _PyValueTemps *value_temps = &value_owners;
            _PyFb_OP *fb = (_PyFb_OP *)IR_FB(IR_FIELD_C(this_instr));
            int oparg = IR_FB_ARG(fb, IR_FIELD_C(this_instr));
            (void)oparg;
            PyValue owner;
            PyValue res;
            // _SPECIALIZE_OP
            {
                uint32_t counter = fb->counter;
                (void)counter;
                _PyFbHeader *feedback = ((_PyFbHeader *)fb);
                ADVANCE_ADAPTIVE_COUNTER(feedback);
            }
            // _OP
            {
                owner = IR_SLOT(IR_FIELD_B(this_instr));
                if (Py_UNLIKELY(PyValue_IsInline(owner))) {
                    owner = PyValue_Box(owner);
                    if (PyValue_IsInline(owner)) {
                        JUMP_TO_LABEL(error);
                    }
                    BOX_STAT_INC(OP);
                    IR_KEEP_BOX(owner, IR_FIELD_B(this_instr));
                }
                else {
                    owner = IR_BORROW_KNOWN(owner, IR_FIELD_B(this_instr));
                }
                PyObject *o = PyValue_AsPyObjectBorrow(owner);
                res = SPAM(o, GETITEM(FRAME_CO_NAMES, oparg));
                IR_CONSUME_INPUT(IR_FIELD_B(this_instr));
                _PyValue_CLOSE_POINTER(owner);
            }
            IR_STORE_DST(IR_FIELD_A(this_instr), res);
            _PyValueTemps_CommitLocal(value_temps);
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
            PyObject *value_boxes[6];
            _PyValueTemps value_owners = {value_boxes, 0, Py_ARRAY_LENGTH(value_boxes), NULL};
            _PyValueTemps *value_temps = &value_owners;
            _PyFb_OP *fb = (_PyFb_OP *)IR_FB(0);
            int oparg = (int)IR_FIELD_CD(this_instr);
            (void)oparg;
            PyValue owner;
            PyValue res;
            // _SPECIALIZE_OP
            {
                uint32_t counter = fb->counter;
                (void)counter;
                _PyFbHeader *feedback = ((_PyFbHeader *)fb);
                ADVANCE_ADAPTIVE_COUNTER(feedback);
            }
            // _OP
            {
                owner = IR_SLOT(IR_FIELD_B(this_instr));
                if (Py_UNLIKELY(PyValue_IsInline(owner))) {
                    owner = PyValue_Box(owner);
                    if (PyValue_IsInline(owner)) {
                        JUMP_TO_LABEL(error);
                    }
                    BOX_STAT_INC(OP);
                    IR_KEEP_BOX(owner, IR_FIELD_B(this_instr));
                }
                else {
                    owner = IR_BORROW_KNOWN(owner, IR_FIELD_B(this_instr));
                }
                PyObject *o = PyValue_AsPyObjectBorrow(owner);
                res = SPAM(o, GETITEM(FRAME_CO_NAMES, oparg));
                IR_CONSUME_INPUT(IR_FIELD_B(this_instr));
                _PyValue_CLOSE_POINTER(owner);
            }
            IR_STORE_DST(IR_FIELD_A(this_instr), res);
            _PyValueTemps_CommitLocal(value_temps);
            DISPATCH();
        }

        TARGET(OP_SPECIALIZED) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP_SPECIALIZED;
            (void)(opcode);
            _PyInterpreterFrame *frame = tstate->current_frame;
            #endif
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP_SPECIALIZED);
            PyObject *value_boxes[6];
            _PyValueTemps value_owners = {value_boxes, 0, Py_ARRAY_LENGTH(value_boxes), NULL};
            _PyValueTemps *value_temps = &value_owners;
            _PyFb_OP *fb = (_PyFb_OP *)IR_FB(IR_FIELD_C(this_instr));
            int oparg = (int)((_PyFbHeader *)fb)->arg;
            (void)oparg;
            PyValue owner;
            PyValue res;
            // _GUARD
            {
                owner = IR_SLOT(IR_FIELD_B(this_instr));
                uint32_t type_version = fb->type_version;
                if (Py_UNLIKELY(!PyValue_IsPointerSpace(owner))) {
                    UPDATE_MISS_STATS(OP);
                    assert(_PyOpcode_Deopt[opcode] == (OP));
                    JUMP_TO_PREDICTED(OP);
                }
                owner = IR_BORROW_KNOWN(owner, IR_FIELD_B(this_instr));
                if (Py_TYPE(PyValue_AsPyObjectBorrow(owner))->tp_version_tag != type_version) {
                    IR_DROP_BORROW(owner, IR_FIELD_B(this_instr));
                    UPDATE_MISS_STATS(OP);
                    assert(_PyOpcode_Deopt[opcode] == (OP));
                    JUMP_TO_PREDICTED(OP);
                }
            }
            // _OP
            {
                PyObject *o = PyValue_AsPyObjectBorrow(owner);
                res = SPAM(o, GETITEM(FRAME_CO_NAMES, oparg));
                IR_CONSUME_INPUT(IR_FIELD_B(this_instr));
                _PyValue_CLOSE_POINTER(owner);
            }
            IR_STORE_DST(IR_FIELD_A(this_instr), res);
            _PyValueTemps_CommitLocal(value_temps);
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
            #endif
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            PyObject *value_boxes[5];
            _PyValueTemps value_owners = {value_boxes, 0, Py_ARRAY_LENGTH(value_boxes), NULL};
            _PyValueTemps *value_temps = &value_owners;
            _PyFb_OP *fb = (_PyFb_OP *)IR_FB(IR_FIELD_D(this_instr));
            int oparg = IR_FB_ARG(fb, IR_FIELD_D(this_instr));
            (void)oparg;
            PyValue left;
            PyValue right;
            PyValue res;
            right = IR_LOAD_BORROWED(IR_FIELD_C(this_instr));
            left = IR_LOAD_BORROWED(IR_FIELD_B(this_instr));
            uint32_t counter = fb->counter;
            (void)counter;
            _PyFbHeader *feedback = ((_PyFbHeader *)fb);
            ADVANCE_ADAPTIVE_COUNTER(feedback);
            res = SPAM(left, right, oparg);
            IR_CONSUME_INPUT(IR_FIELD_B(this_instr));
            IR_CONSUME_INPUT(IR_FIELD_C(this_instr));
            IR_STORE_DST(IR_FIELD_A(this_instr), res);
            _PyValueTemps_CommitLocal(value_temps);
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
            PyObject *value_boxes[5];
            _PyValueTemps value_owners = {value_boxes, 0, Py_ARRAY_LENGTH(value_boxes), NULL};
            _PyValueTemps *value_temps = &value_owners;
            _PyFb_OP *fb = (_PyFb_OP *)IR_FB(0);
            int oparg = (int)IR_FIELD_D(this_instr);
            (void)oparg;
            PyValue left;
            PyValue right;
            PyValue res;
            right = IR_LOAD_BORROWED(IR_FIELD_C(this_instr));
            left = IR_LOAD_BORROWED(IR_FIELD_B(this_instr));
            uint32_t counter = fb->counter;
            (void)counter;
            _PyFbHeader *feedback = ((_PyFbHeader *)fb);
            ADVANCE_ADAPTIVE_COUNTER(feedback);
            res = SPAM(left, right, oparg);
            IR_CONSUME_INPUT(IR_FIELD_B(this_instr));
            IR_CONSUME_INPUT(IR_FIELD_C(this_instr));
            IR_STORE_DST(IR_FIELD_A(this_instr), res);
            _PyValueTemps_CommitLocal(value_temps);
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
            #endif
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(IR_PASS);
            PyObject *value_boxes[2];
            _PyValueTemps value_owners = {value_boxes, 0, Py_ARRAY_LENGTH(value_boxes), NULL};
            _PyValueTemps *value_temps = &value_owners;
            PRE_DISPATCH_GOTO();
            DISPATCH_GOTO();
        }
    """
        self.run_ir_test(input, output)
        self.assertTrue(self.formats(input)["IR_PASS"].transparent)

    def test_metadata(self):
        # IR-only handlers follow the opcodes in name order. An instrumented
        # twin shares the format of its instruction.
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
        load = ("{ .flags = _PyIR_VALID | _PyIR_STORE_DST, "
                ".roles = { _PyIR_ROLE_DST, "
                "_PyIR_ROLE_IMM, _PyIR_ROLE_NONE, _PyIR_ROLE_NONE }, "
                ".imm = _PyIR_LOC_B, .fb = _PyIR_LOC_NONE, .ninputs = 0, "
                ".dst = 0, .borrow_mask = 0x0, .shadow = LOAD, .nofb = 0 }")
        for expected in [
            "#define IR_BACON                                       256",
            "#define IR_PASS                                        257",
            "#define IR_NUM_HANDLERS                                258",
            f"[LOAD] = {load},",
            "[INSTRUMENTED_LOAD] = "
            f"{load.replace('LOAD,', 'INSTRUMENTED_LOAD,')},",
            "[IR_PASS] = { .flags = _PyIR_VALID | _PyIR_TRANSPARENT, "
            ".roles = { _PyIR_ROLE_NONE, _PyIR_ROLE_NONE, _PyIR_ROLE_NONE, "
            "_PyIR_ROLE_NONE }, .imm = _PyIR_LOC_NONE, .fb = _PyIR_LOC_NONE, "
            ".ninputs = 0, .dst = _PyIR_NO_DST, .borrow_mask = 0x0, "
            ".shadow = IR_PASS, .nofb = 0 },",
            '[IR_BACON - IR_FIRST_ONLY_HANDLER] = "IR_BACON",',
            '[IR_PASS - IR_FIRST_ONLY_HANDLER] = "IR_PASS",',
        ]:
            self.assertIn(expected, lines)

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

class TestJitHelpers(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        source = Path(__file__).resolve().parents[2] / "Python" / "bytecodes.c"
        output = io.StringIO()
        jit_helpers_generator.generate([str(source)], output)
        cls.output = output.getvalue()
        cls.analysis = analyzer.analyze_files([str(source)])

    def body(self, name):
        start = self.output.index(f"static _PyJitAction jit_helper_{name}(")
        end = self.output.find("static _PyJitAction", start + 1)
        if end < 0:
            end = self.output.index("#undef JIT_FINISH", start)
        return self.output[start:end]

    def test_natural_handlers_append_and_preserve_formats(self):
        root = Path(__file__).resolve().parents[2]
        analysis = analyzer.analyze_files([str(root / "Python/bytecodes.c")])
        ids = ir_generator.handler_ids(analysis)
        formats = ir_generator.compute_formats(analysis)
        self.assertEqual({name: ids[name] for name in ir_generator.EXEC_ONLY},
                         {"CALL_JIT_WARM": 302, "JUMP_BACKWARD_JIT_WARM": 303, "RESUME_JIT": 304})
        for name, base in ir_generator.EXEC_ONLY.items():
            self.assertEqual(formats[name], formats[base])
            self.assertEqual(ir_metadata_generator.shadow_of(name, analysis), base)
        generated = (root / "Python/generated_ir_cases.c.h").read_text()
        warm = generated.split("TARGET(CALL_JIT_WARM)", 1)[1].split("TARGET(", 1)[0]
        self.assertNotIn("_SPECIALIZE_CALL", warm)
        self.assertIn("JIT_PREPARE_CALL", warm)
        resume = generated.split("TARGET(RESUME_JIT)", 1)[1].split("TARGET(", 1)[0]
        self.assertLess(resume.index("check_periodics"), resume.index("JIT_SITE_ENTRY"))

    def test_object_iteration_and_keyword_adapters(self):
        root = Path(__file__).resolve().parents[2]
        generated = (root / "Python/generated_jit_helpers.c.h").read_text()
        required = {"LOAD_ATTR", "LOAD_ATTR_NOFB", "STORE_ATTR", "STORE_ATTR_NOFB",
                    "GET_ITER", "FOR_ITER", "END_FOR", "POP_ITER", "BUILD_LIST",
                    "BUILD_TUPLE", "BUILD_MAP", "UNPACK_SEQUENCE",
                    "STORE_SUBSCR", "CONTAINS_OP", "CALL_KW", "COPY_FREE_VARS",
                    "MAKE_CELL", "LOAD_DEREF", "STORE_DEREF"}
        for name in required:
            self.assertIn("jit_helper_" + name + "(", generated)
        keyword = generated.split("jit_helper_CALL_KW(", 1)[1].split("static _PyJitAction", 1)[0]
        self.assertLess(keyword.index("JIT_PREPARE_CALL"), keyword.index("_DO_CALL_KW"))
        self.assertIn("_PyValue_OwnArray", keyword)

    def test_every_valid_handler_classified(self):
        names = set(ir_generator.compute_formats(self.analysis))
        self.assertIn("BINARY_OP", names)
        rows = re.findall(r'\[\d+\] = \{"(\w+)", ([^,]+), (NULL|"[^"]*"),', self.output)
        self.assertEqual({name for name, _, _ in rows}, names)
        for name, helper, reason in rows:
            with self.subTest(name=name):
                self.assertNotEqual(helper == "NULL", reason == "NULL")
                if helper != "NULL":
                    self.assertIn(f"static _PyJitAction {helper}(", self.output)
        for name in ("LOAD_CONST_INLINE_INT", "LOAD_CONST_INLINE_FLOAT", "LOAD_SMALL_INT_INLINE"):
            self.assertIn(f"jit_helper_{name}(", self.output)
        self.assertNotIn("jit_helper_YIELD_VALUE(", self.output)

    def test_recorded_home_effects_match_ir_layouts(self):
        for name, fmt in ir_generator.compute_formats(self.analysis).items():
            row = next(line for line in self.output.splitlines() if f'{{"{name}",' in line)
            with self.subTest(name=name):
                self.assertIn(".roles = { " + ", ".join(fmt.roles) + " }", row)
                self.assertIn(f".ninputs = {fmt.n_inputs}", row)
                self.assertIn(f".borrow_mask = {fmt.borrow_mask}", row)
                self.assertIn(f".flags = {int(fmt.window) * 2 | int(fmt.store_dst) * 64}", row)

    def test_common_constant_and_raise_window_order(self):
        self.assertIn("PyValue_DUP(tstate->interp->common_consts[oparg])", self.body("LOAD_COMMON_CONSTANT"))
        raised = self.body("RAISE_VARARGS")
        self.assertLess(raised.index("IR_CLEAR_SLOTS"), raised.index("do_raise("))
        self.assertIn("exception_unwind", raised)
        self.assertIn("JUMP_TO_LABEL(error)", raised)
        self.assertIn("#define JIT_PHASE_exception_unwind _PyJIT_UNWIND_PENDING", self.output)
        self.assertIn("_PyJIT_ERROR_LOG_PENDING", self.output)

    def test_dictionary_operation_publication(self):
        root = Path(__file__).resolve().parents[2]
        generated = (root / "Python/generated_ir_cases.c.h").read_text()
        for name, provider in (("STORE_SUBSCR", "value_store_subscript("),
                               ("STORE_SUBSCR_DICT", "_PyDict_SetValue_Take2(")):
            body = generated.split(f"TARGET({name})", 1)[1].split("TARGET(", 1)[0]
            with self.subTest(name=name):
                call = body.index(provider)
                self.assertLess(body.index("value_operation_begin("), call)
                self.assertLess(body.rindex("IR_CONSUME_INPUT", 0, call), call)
                finish = body.index("value_operation_finish(", call)
                self.assertNotIn("IR_FIELD_", body[call:finish])
                failure = body.index("if (operation_failed < 0)", finish)
                self.assertLess(failure, body.index("ir_word =", failure))
                self.assertNotIn("// _POP_TOP", body)
        generic = generated.split("TARGET(STORE_SUBSCR)", 1)[1].split("TARGET(", 1)[0]
        self.assertIn("value_operation_close3(&held_sub, &held_container, &held_value)", generic)
        hint = generated.split("TARGET(STORE_ATTR_WITH_HINT)", 1)[1].split("TARGET(", 1)[0]
        self.assertIn("value_operation_close3(&release_value, &watched_dict, &receiver)", hint)
        capture = hint.index("operation.input_slots[0] =")
        call = hint.index("int err = PyDict_SetValue(")
        self.assertLess(capture, hint.index("IR_CONSUME_INPUT", capture))
        self.assertLess(hint.index("operation.input_slots[1] =", capture), call)
        restore = hint.index("value_operation_restore2(", call)
        failed_close = hint.index("value_operation_close3(&watched_dict, &release_value, &receiver)", restore)
        self.assertLess(restore, failed_close)
        self.assertNotIn("IR_FIELD_", hint[call:failed_close])
        self.assertIn("&old_value, &receiver, NULL", hint)
        self.assertNotIn("// _POP_TOP", hint)
        for name in ("BUILD_MAP", "STORE_SUBSCR"):
            body = self.body(name)
            self.assertIn("value_operation_finish(", body)
            self.assertIn("ctx->raising_instr = frame->instr_ptr", body)
            self.assertIn("return _PyJIT_RAISE", body)
        ceval = (root / "Python/ceval.c").read_text()
        construction = ceval.split("_Py_BuildMap_ValueSteal(", 1)[1].split("\nPyValue\n", 1)[0]
        self.assertLess(construction.index("_PyValue_OwnArray("),
                        construction.index("_PyDict_FromValueItems("))
        self.assertNotIn("VALUES_TO_PYOBJECTS", construction)

    def test_real_call_return_and_error_order(self):
        call = self.body("CALL")
        self.assertIn("_PyEvalFramePushAndInit(", call)
        self.assertIn("frame->return_slot =", call)
        self.assertIn("DISPATCH_INLINED(new_frame)", call)
        self.assertIn("_Py_VectorCallInstrumentation_ValueSteal", call)
        returned = self.body("RETURN_VALUE")
        self.assertLess(returned.index("tstate->current_frame = dying->previous"),
                        returned.index("_PyEval_FrameClearAndPop"))
        self.assertLess(returned.index("IR_STORE_RETURN(res)"), returned.index("DISPATCH();"))
        self.assertIn("_PyJIT_RETURN_COMPLETED", returned)
        binary = self.body("BINARY_OP_ADD")
        self.assertIn("value_binary_operation(oparg, held_left, held_right, value_temps)", binary)
        failed = binary.split("if (PyValue_IsNull(pending))", 1)[1].split("value_reader_complete(", 1)[0]
        self.assertIn("value_operation_restore2(", failed)
        self.assertIn("return _PyJIT_RAISE;", failed)
        self.assertNotIn("IR_STORE_", failed)
        self.assertNotIn("res =", failed)
        self.assertLess(binary.index("value_reader_complete("), binary.index("res = pending;"))

    def test_reader_publication_and_refresh(self):
        root = Path(__file__).resolve().parents[2]
        generated = (root / "Python/generated_ir_cases.c.h").read_text()
        for name in ("BINARY_OP_ADD", "BINARY_OP_SUBSCR", "BINARY_OP_SUBSCR_DICT"):
            body = generated.split(f"TARGET({name})", 1)[1].split("TARGET(", 1)[0]
            with self.subTest(name=name):
                call = body.index("value_binary_operation(")
                self.assertLess(body.index("operation.result_slot ="), call)
                self.assertLess(body.rindex("IR_CONSUME_INPUT", 0, call), call)
                failed = body.split("if (PyValue_IsNull(pending))", 1)[1].split("value_reader_complete(", 1)[0]
                self.assertIn("JUMP_TO_LABEL(error)", failed)
                self.assertNotIn("IR_STORE_", failed)
                self.assertNotIn("res =", failed)
                self.assertLess(body.index("value_reader_complete("), body.index("ir_word ="))
                self.assertNotIn("// _POP_TOP", body)
        for name in ("LOAD_ATTR_INSTANCE_VALUE", "LOAD_ATTR_SHAPE", "LOAD_ATTR_MODULE", "LOAD_ATTR_WITH_HINT"):
            body = generated.split(f"TARGET({name})", 1)[1].split("TARGET(", 1)[0]
            with self.subTest(name=name):
                retire = body.index("retire_published_values(")
                self.assertIn("IR_IS_LOCAL(IR_FIELD_A(this_instr)) ? &attr : NULL", body)
                self.assertLess(body.index("IR_CONSUME_INPUT"), retire)
                self.assertLess(body.index("if (retire_failed < 0)"), body.index("ir_word =", retire))
                self.assertNotIn("// _POP_TOP", body)
        global_body = self.body("LOAD_GLOBAL")
        call = global_body.index("_PyEval_LoadGlobalValue(")
        self.assertLess(global_body.index("value_operation_begin("), call)
        self.assertLess(global_body.index("value_operation_finish(", call), global_body.index("// _PUSH_NULL_CONDITIONAL", call))
        ceval = (root / "Python/ceval.c").read_text()
        complete = ceval.split("value_reader_complete(", 1)[1].split("\nstatic int\n", 1)[0]
        self.assertLess(complete.index("_PyFrame_Slots(frame)[slot] = *result"),
                        complete.index("value_operation_close3(first, second, NULL)", complete.index("*result = _PyValueTemps_Publish")))

    def test_binary_nomination_does_not_escape(self):
        root = Path(__file__).resolve().parents[2]
        source = (root / "Python/specialize.c").read_text()
        wrapper = source.split("_Py_Specialize_BinaryOp(", 1)[1].split("\n}\n", 1)[0]
        nominate = source.split("\nspecialize_binary_op(", 1)[1].split("\n}\n", 1)[0]
        statistics = source.split("binary_op_fail_kind(", 1)[1].split("\n}\n", 1)[0]
        for body in (wrapper, nominate, statistics):
            self.assertNotIn("NewBoxed", body)
            self.assertNotIn("_PyType_Lookup", body)
            self.assertNotIn("Py_DECREF", body)
            self.assertNotIn("PyErr_Clear", body)
        self.assertIn("fb_begin_binary_ex(instr, lhs_type, rhs_type, false)", nominate)
        self.assertIn("lhs != NULL && rhs != NULL", nominate)
        table = source.split("binaryop_extend_descrs[] = {", 1)[1].split("\n};", 1)[0]
        rows = re.findall(r"^\s*\{(NB_\w+),\s*([^,]+),", table, re.M)
        self.assertGreater(len(rows), 0)
        self.assertTrue(all(guard == "NULL" for _, guard in rows))
        save = source.split("fb_save_mono(", 1)[1].split("\n}\n", 1)[0]
        self.assertLess(save.index("if (!may_allocate)"), save.index("_PyMem_NoScanRealloc"))

    def test_attribute_publication_and_failure_phases(self):
        root = Path(__file__).resolve().parents[2]
        generated = (root / "Python/generated_ir_cases.c.h").read_text()
        for name in ("LOAD_ATTR", "LOAD_ATTR_NOFB"):
            body = generated.split(f"TARGET({name})", 1)[1].split("TARGET(", 1)[0]
            with self.subTest(name=name):
                call = body.index("value_load_attribute(")
                self.assertLess(body.index("operation.result_slot ="), call)
                failed = body.split("if (failed < 0)", 1)[1].split("value_attribute_complete(", 1)[0]
                self.assertLess(failed.index("if (!invoked)"), failed.index("value_operation_restore1("))
                self.assertNotIn("IR_STORE_", failed)
                self.assertNotIn("self_or_null", body)
                complete = body.index("value_attribute_complete(")
                self.assertLess(complete, body.index("ir_word =", complete))
                tail = body[body.index("attr = pending;", complete):]
                self.assertIn("if (IR_IS_LOCAL(IR_FIELD_A(this_instr)))", tail)
        store = generated.split("TARGET(STORE_ATTR)", 1)[1].split("TARGET(", 1)[0]
        call = store.index("value_store_attribute(")
        self.assertLess(store.index("IR_CONSUME_INPUT"), call)
        self.assertIn("if (!invoked)", store[call:])
        self.assertIn("value_operation_close3(&receiver, &held_value, NULL)", store)
        for name in ("STORE_ATTR_INSTANCE_VALUE", "STORE_ATTR_SHAPE", "STORE_ATTR_SHAPE_ADD"):
            body = generated.split(f"TARGET({name})", 1)[1].split("TARGET(", 1)[0]
            self.assertNotIn("// _POP_TOP", body)
            self.assertNotIn("_PyShape_Add(", body)
            self.assertIn("retire_published_values(", body)
        source = (root / "Python/specialize.c").read_text()
        for name in ("_Py_Specialize_LoadAttr", "_Py_Specialize_StoreAttr"):
            body = source.split(name + "(", 1)[1].split("\n}\n", 1)[0]
            self.assertLess(body.index("_PyType_TryLookupValueNoAlloc"), body.index("fb_begin_ex(instr, false)"))
        self.assertNotIn("_Py_LoadAttr_ValueSteal", generated)

    def test_native_vector_taken_window_publication(self):
        root = Path(__file__).resolve().parents[2]
        generated = (root / "Python/generated_ir_cases.c.h").read_text()
        for name in ("CALL_NON_PY_GENERAL", "CALL_KW_NON_PY"):
            body = generated.split(f"TARGET({name})", 1)[1].split("TARGET(", 1)[0]
            with self.subTest(name=name):
                self.assertLess(body.index("PyValue_IsObject(callable)"), body.index("PyValue_AsPyObjectBorrow(callable)"))
                self.assertLess(body.index("native_vector_capture("), body.index("native_vector_prepare("))
                self.assertLess(body.index("native_vector_take("), body.index("_Py_VectorCall_ValueSteal("))
                self.assertIn("c = names_owner = PyValue_NULL", body)
                publish = body.index("_PyNativeResult_Publish(")
                retire = body.index("native_vector_retire(", publish)
                self.assertLess(publish, retire)
                self.assertNotIn("IR_CLEAR_SLOTS", body[publish:retire])
                self.assertNotIn("_PyValueTemps_Publish(value_temps, &res)", body[retire:])
                self.assertLess(body.index("operation_failed < 0", retire), body.index("ir_word =", retire))
                self.assertEqual(body.count("check_periodics_at_end(tstate, frame)"), 1)
                poll = body.index("check_periodics_at_end(tstate, frame)")
                self.assertLess(body.index("ir_word =", retire), poll)
                self.assertLess(poll, body.index("ir_word =", poll))
        source = (root / "Python/specialize.c").read_text()
        for name in ("_Py_Specialize_Call", "_Py_Specialize_CallKw"):
            body = source.split(name + "(", 1)[1].split("\n}\n", 1)[0]
            self.assertIn("fb_begin_call_ex(instr, PyValue_TYPE(callable_st), !native_or_inline)", body)
        ex = source.split("_Py_Specialize_CallFunctionEx(", 1)[1].split("\n}\n", 1)[0]
        self.assertIn("fb_begin_call(instr, PyValue_TYPE(func_st))", ex)

    def test_cold_specialization_continues_generic_once(self):
        for name in ("CALL", "BINARY_OP_ADD", "LOAD_GLOBAL", "COMPARE_OP"):
            with self.subTest(name=name):
                body = self.body(name)
                self.assertNotIn("DISPATCH_SAME_OPARG()", body)
                jump = body.index("goto jit_after_SPECIALIZE_")
                label = body.index(":;", jump)
                self.assertLess(body.index("ADVANCE_ADAPTIVE_COUNTER", jump), label)
                self.assertIn("next_instr = this_instr + 1", body)
        self.assertIn("_PyJIT_BRANCH", self.body("JUMP_BACKWARD"))
        self.assertIn("check_periodics(tstate)", self.body("JUMP_BACKWARD"))

    def test_directives_start_physical_lines(self):
        for lineno, line in enumerate(self.output.splitlines(), 1):
            if re.search(r"#(?:if|elif|else|endif|define|undef|pragma)\b", line):
                with self.subTest(lineno=lineno):
                    self.assertRegex(line, r"^\s*#")

    def test_canonical_publication_and_bridge_entry(self):
        root = Path(__file__).resolve().parents[2]
        output = io.StringIO()
        ir_generator.generate_ir([str(root / "Python/bytecodes.c")], self.analysis, output, False)
        generated = output.getvalue()
        for body in re.split(r"\bTARGET\([A-Z_0-9]+\)", generated)[1:]:
            if "IR_PREDICTED(" in body:
                self.assertLess(body.index("IR_PREDICTED("),
                                body.index("_PyValueTemps value_owners"))
        for name in ("CALL", "CALL_KW", "INSTRUMENTED_CALL", "INSTRUMENTED_CALL_KW"):
            body = generated.split(f"TARGET({name})", 1)[1].split("TARGET(", 1)[0]
            self.assertIn("_PyValue_OwnArray(window,", body)
            first_box = body.find("PyValue_Box(")
            if first_box >= 0:
                self.assertLess(body.index("_PyValue_OwnArray("), first_box)
        constant = generated.split("TARGET(LOAD_CONST)", 1)[1].split("TARGET(", 1)[0]
        self.assertIn("PyTuple_GET_VALUE", constant)
        self.assertNotIn("PyValue_Box(", constant)

    def test_extension_declines_inline_before_object_access(self):
        root = Path(__file__).resolve().parents[2]
        output = io.StringIO()
        ir_generator.generate_ir([str(root / "Python/bytecodes.c")], self.analysis, output, False)
        body = output.getvalue().split("TARGET(BINARY_OP_EXTEND)", 1)[1].split("TARGET(", 1)[0]
        guard = body.index("!PyValue_IsObject(left)")
        self.assertLess(guard, body.index("PyValue_AsPyObjectBorrow(left)"))
        self.assertLess(guard, body.index("d->guard"))
        self.assertNotIn("PyValue_Box(left)", body)
        self.assertNotIn("PyValue_Box(right)", body)

    def test_object_specializations_restart_before_creating_bridges(self):
        root = Path(__file__).resolve().parents[2]
        output = io.StringIO()
        ir_generator.generate_ir([str(root / "Python/bytecodes.c")], self.analysis, output, False)
        generated = output.getvalue()
        for name, operand in (("BINARY_OP_SUBSCR_GETITEM", "container"),
                              ("LOAD_ATTR_MODULE", "owner"),
                              ("SEND_GEN", "receiver")):
            with self.subTest(handler=name):
                body = generated.split(f"TARGET({name})", 1)[1].split("TARGET(", 1)[0]
                self.assertIn(f"!PyValue_IsObject({operand})", body)
                self.assertNotIn(f"PyValue_Box({operand})", body)
        ex = generated.split("TARGET(CALL_EX_PY)", 1)[1].split("TARGET(", 1)[0]
        self.assertEqual(ex.count("// _CHECK_IS_PY_CALLABLE_EX"), 2)
        self.assertLess(ex.index("// _CHECK_IS_PY_CALLABLE_EX"),
                        ex.index("_PyValueTemps_Object"))
        self.assertLess(ex.index("// _MAKE_CALLARGS_A_TUPLE"),
                        ex.rindex("// _CHECK_IS_PY_CALLABLE_EX"))
        super_attr = generated.split("TARGET(LOAD_SUPER_ATTR_ATTR)", 1)[1].split("TARGET(", 1)[0]
        self.assertLess(super_attr.rindex("JUMP_TO_PREDICTED"),
                        super_attr.index("_PyValueTemps_Object"))
        store = generated.split("TARGET(STORE_ATTR_WITH_HINT)", 1)[1].split("TARGET(", 1)[0]
        self.assertNotIn("PyValue_Box(value)", store)
        self.assertIn("*value_ptr = PyValue_MakeHeapSafe(value)", store)
        self.assertIn("PyDict_SetValue", store)

    def test_bridge_cleanup_precedes_next_handler_decode(self):
        root = Path(__file__).resolve().parents[2]
        macros = (root / "Python/ceval_macros.h").read_text()
        dispatch = macros.split("#define DISPATCH()", 1)[1].split("#define DISPATCH_SAME_OPARG", 1)[0]
        self.assertLess(dispatch.index("finish_value_bridges"), dispatch.index("NEXTOPARG"))
        self.assertIn("finish_value_bridges", self.output.split("#define JIT_FINISH", 1)[1])

    def test_native_o_publishes_before_cleanup_and_reloads(self):
        root = Path(__file__).resolve().parents[2]
        output = io.StringIO()
        ir_generator.generate_ir([str(root / "Python/bytecodes.c")], self.analysis, output, False)
        body = output.getvalue().split("TARGET(CALL_BUILTIN_O)", 1)[1].split("TARGET(", 1)[0]
        consume = body.index("IR_CLEAR_SLOTS(&window[0], 2 + oparg)")
        publish = body.index("_PyNativeResult_Publish(")
        store = body.index("window[0] = _PyValueTemps_Publish")
        finish = body.index("int failed = native_call_finish(")
        self.assertLess(consume, publish)
        self.assertLess(publish, store)
        self.assertLess(store, finish)
        self.assertNotIn("window[0] =", body[finish:])
        self.assertNotIn("// _POP_TOP", body)
        reload = body.index("this_instr = frame->instr_ptr;", finish)
        failed = body.index("if (failed < 0)", finish)
        failed_jump = body.index("JUMP_TO_LABEL(error)", failed)
        self.assertLess(failed_jump, reload)
        word = body.index("ir_word = FT_ATOMIC_LOAD_UINT64_RELAXED(*this_instr)", reload)
        slots = body.index("slots = _PyFrame_Slots(frame)", word)
        window = body.index("window = &slots[", slots)
        move = body.index("IR_MOVE_TO_DST", window)
        self.assertLess(finish, reload)
        self.assertLess(window, move)
        self.assertNotIn("jit_helper_CALL_BUILTIN_O(", self.output)
        runtime = (root / "Python/ceval.c").read_text()
        cleanup = runtime.split("native_call_finish(", 1)[1].split('#include "generated_jit_helpers.c.h"', 1)[0]
        self.assertLess(cleanup.index("_PyNativeScope_End"), cleanup.index("*argument = PyValue_NULL"))
        self.assertLess(cleanup.index("*argument = PyValue_NULL"), cleanup.index("*callable = PyValue_NULL"))
        self.assertLess(cleanup.index("*callable = PyValue_NULL"), cleanup.index("_Py_Instrument"))

    def test_final_local_retirement_follows_all_publications(self):
        root = Path(__file__).resolve().parents[2]
        output = io.StringIO()
        ir_generator.generate_ir([str(root / "Python/bytecodes.c")], self.analysis, output, False)
        generated = output.getvalue()
        for name in ("BINARY_OP_ADD", "CALL_BUILTIN_O"):
            body = generated.split(f"TARGET({name})", 1)[1].split("TARGET(", 1)[0]
            publication = max(body.rfind("IR_STORE_DST("), body.rfind("IR_MOVE_TO_DST("))
            commit = body.rfind("_PyValueTemps_CommitLocal(value_temps)")
            self.assertGreaterEqual(publication, 0)
            self.assertLess(publication, commit)
            self.assertLess(commit, body.rfind("DISPATCH();"))
        helper = self.body("BINARY_OP_ADD")
        self.assertLess(helper.rfind("IR_STORE_DST("), helper.rfind("_PyValueTemps_CommitLocal("))
        self.assertLess(helper.rfind("_PyValueTemps_CommitLocal("), helper.rfind("DISPATCH();"))
        nop = generated.split("TARGET(NOP)", 1)[1].split("TARGET(", 1)[0]
        self.assertNotIn("_PyValueTemps_CommitLocal", nop)
        for fmt in ir_generator.compute_formats(self.analysis).values():
            self.assertLessEqual(fmt.roles.count(ir_generator.ROLE_DST), 1)
        formats = ir_generator.compute_formats(self.analysis)
        for name in ("RETURN_VALUE", "RETURN_GENERATOR", "RESUME_JIT", "JUMP_BACKWARD_JIT_WARM"):
            self.assertFalse(formats[name].store_dst)
        instructions = ir_generator.handler_instructions(self.analysis)
        bodies = dict(re.findall(r"TARGET\((\w+)\)(.*?)(?=\bTARGET\(|\Z)", generated, re.S))
        for name, body in bodies.items():
            with self.subTest(capacity=name):
                private_bound = 1 + sum(len(part.stack.inputs) + len(part.stack.outputs)
                                        for part in instructions[name].parts
                                        if isinstance(part, ir_generator.Uop))
                capacity = int(re.search(r"PyObject \*value_boxes\[(\d+)\]", body)[1])
                self.assertGreaterEqual(capacity, private_bound + 1 + (2 if name == "STORE_ATTR_SLOT" else 0))
                last_publication = max(body.rfind("IR_STORE_DST("), body.rfind("IR_MOVE_TO_DST("))
                if last_publication >= 0:
                    self.assertGreater(body.rfind("_PyValueTemps_CommitLocal("), last_publication)
        load = generated.split("TARGET(LOAD_ATTR_SLOT)", 1)[1].split("TARGET(", 1)[0]
        retire = load.index("retire_published_values(")
        self.assertLess(load.index("IR_STORE_TEMP_DST("), retire)
        self.assertIn("? &attr : NULL", load)
        self.assertLess(retire, load.rindex("IR_STORE_DST("))
        self.assertLess(load.index("if (retire_failed < 0)"), load.index("this_instr = frame->instr_ptr"))
        store = generated.split("TARGET(STORE_ATTR_SLOT)", 1)[1].split("TARGET(", 1)[0]
        self.assertIn("&old_value, &receiver, NULL", store)
        self.assertTrue(self.analysis.instructions["LOAD_ATTR_SLOT"].properties.error_without_pop)
        self.assertTrue(self.analysis.instructions["STORE_ATTR_SLOT"].properties.error_without_pop)
        self.assertLess(store.index("PyValue_XCLOSE(old_value)"),
                        store.index("retire_needed = _PyValue_MayDealloc(receiver)"))
        self.assertLess(store.index("IR_CONSUME_INPUT(IR_FIELD_A(this_instr))"),
                        store.index("retire_published_values("))

    def test_descriptor_and_no_evaluator_dispatch(self):
        self.assertIn("const _PyJitDecodedOp *op", self.body("CALL"))
        for field in range(4):
            self.assertIn(f"(op->fields[{field}])", self.output)
        self.assertIn("(op->wide)", self.output)
        self.assertNotIn("JUMP_TO_PREDICTED", self.output)
        self.assertNotIn("goto *", self.output)
        self.assertNotRegex(self.output, r"\bTARGET\(")


if __name__ == "__main__":
    unittest.main()
