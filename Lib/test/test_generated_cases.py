import contextlib
import io
import os
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
        # Releases do not escape, so the order of kills matters only where
        # the stack is flushed, before an escaping call.
        self.check_error("""
        inst(OP, (a, b -- c)) {
            c = b;
            DEAD(a);
            escaping_call();
            DEAD(b);
        }
        """, "Input 'a' is not live, but 'b' is")

    def test_root_held_only_in_c_local(self):
        # The collector does not scan the eval loop's C frame, so a handler
        # must not hold a value only in a C local across an escaping call.
        self.check_error("""
        inst(OP, (a -- c)) {
            PyObject *o = PyObject_Repr(PyStackRef_AsPyObjectBorrow(a));
            escaping_call();
            c = PyStackRef_FromPyObject(o);
            INPUTS_DEAD();
        }
        """, "'o' is held only in a C local across the escaping call "
             "to 'escaping_call'")
        self.check_error("""
        inst(OP, (a -- c)) {
            _PyStackRef r = PyStackRef_FromPyObject(
                PyObject_Repr(PyStackRef_AsPyObjectBorrow(a)));
            escaping_call();
            c = r;
            INPUTS_DEAD();
        }
        """, "'r' is held only in a C local")

    def test_root_held_in_c_stack_ref(self):
        # A _PyCStackRef is a precise root, and a value that is dead before
        # the escaping call needs no root.
        self.generate("""
        inst(OP, (a -- c)) {
            _PyCStackRef held;
            _PyThreadState_PushCStackRef(tstate, &held);
            held.ref = PyStackRef_FromPyObject(
                PyObject_Repr(PyStackRef_AsPyObjectBorrow(a)));
            escaping_call();
            c = _PyThreadState_PopCStackRefSteal(tstate, &held);
            INPUTS_DEAD();
        }
        inst(OP2, (a -- c)) {
            PyObject *o = PyObject_Repr(PyStackRef_AsPyObjectBorrow(a));
            INPUTS_DEAD();
            ERROR_IF(o == NULL);
            c = PyStackRef_FromPyObject(o);
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
        # Inputs that are only used as objects are read through their
        # fields.  They stay in their slots until the handler leaves, so the
        # slots keep them alive across calls that may collect.
        input = """
        inst(OP, (left, right -- res)) {
            PyObject *l = PyStackRef_AsPyObjectBorrow(left);
            PyObject *r = PyStackRef_AsPyObjectBorrow(right);
            PyObject *o = SPAM(l, r);
            INPUTS_DEAD();
            ERROR_IF(o == NULL);
            res = PyStackRef_FromPyObject(o);
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
            _PyStackRef left;
            _PyStackRef right;
            _PyStackRef res;
            right = IR_SLOT(IR_FIELD_C(this_instr));
            left = IR_SLOT(IR_FIELD_B(this_instr));
            if (Py_UNLIKELY(PyStackRef_IsInline(left))) {
                left = PyStackRef_Box(left);
                if (PyStackRef_IsInline(left)) {
                    JUMP_TO_LABEL(error);
                }
                BOX_STAT_INC(OP);
                IR_KEEP_BOX(left, IR_FIELD_B(this_instr));
            }
            if (Py_UNLIKELY(PyStackRef_IsInline(right))) {
                right = PyStackRef_Box(right);
                if (PyStackRef_IsInline(right)) {
                    JUMP_TO_LABEL(error);
                }
                BOX_STAT_INC(OP);
                IR_KEEP_BOX(right, IR_FIELD_C(this_instr));
            }
            PyObject *l = PyStackRef_AsPyObjectBorrow(left);
            PyObject *r = PyStackRef_AsPyObjectBorrow(right);
            PyObject *o = SPAM(l, r);
            if (o == NULL) {
                IR_CONSUME_INPUT(IR_FIELD_B(this_instr));
                IR_CONSUME_INPUT(IR_FIELD_C(this_instr));
                JUMP_TO_LABEL(error);
            }
            res = PyStackRef_FromPyObject(o);
            IR_CONSUME_INPUT(IR_FIELD_C(this_instr));
            IR_CONSUME_INPUT(IR_FIELD_B(this_instr));
            IR_STORE_DST(IR_FIELD_A(this_instr), res);
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
            res = SPAM(PyStackRef_AsPyObjectBorrow(value));
        }
        inst(OP_OBJ, (value -- res)) {
            res = SPAM(PyStackRef_AsPyObjectBorrow(value));
            DEAD(value);
        }
        inst(OP_FAMILY, (value -- res)) {
            res = SPAM(PyStackRef_AsPyObjectBorrow(value));
            DEAD(value);
        }
        inst(OP_FAMILY_RAW, (value -- res)) {
            res = SPAM_RAW(value);
            DEAD(value);
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
            INPUTS_DEAD();
        }
        inst(BRANCH, (counter/1, cond --)) {
            _PyFbHeader *feedback = FB_COUNTER();
            int flag = PyStackRef_IsFalse(cond);
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
        # The inputs stay in their slots across SPAM, which may collect,
        # and are consumed as the handler leaves.
        self.assertIn("SPAM(owner, value);\n"
                      "            IR_CONSUME_INPUT(IR_FIELD_A(this_instr));\n"
                      "            IR_CONSUME_INPUT(IR_FIELD_B(this_instr));\n"
                      "            DISPATCH();", store)
        self.assertNotIn("CLOSE", store)
        self.assertIn("IR_CONSUME_INPUT(IR_FIELD_A(this_instr));",
                      self.handler(output, "BRANCH"))
        self.assertNotIn("stack_pointer", output)

    def test_store_dst(self):
        # Which results a folded STORE_FAST may send to a local, and how
        # the handler then finds the stack pointer: dst may name a local, so
        # from the top slot in a free field, or by counting.
        input = """
        inst(OP, (left, right -- res)) {
            res = SPAM(PyStackRef_AsPyObjectBorrow(left),
                       PyStackRef_AsPyObjectBorrow(right));
            INPUTS_DEAD();
        }
        inst(ATTR, (owner -- attr, null[oparg & 1])) {
            attr = SPAM(PyStackRef_AsPyObjectBorrow(owner));
            INPUTS_DEAD();
            if (oparg & 1) {
                null[0] = PyStackRef_NULL;
            }
        }
        no_fold inst(ALIAS, (value -- res)) {
            res = SPAM(PyStackRef_AsPyObjectBorrow(value));
            INPUTS_DEAD();
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
            EXIT_IF(!PyStackRef_IsObject(nos));
        }
        op(_USE, (left, right -- left, res)) {
            res = SPAM(PyStackRef_AsPyObjectBorrow(right));
            DEAD(right);
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
            int oparg = (int)IR_FIELD_C(this_instr);
            (void)oparg;
            _PyStackRef *window = &IR_SLOT(IR_FIELD_B(this_instr));
            (void)window;
            _PyStackRef callable;
            _PyStackRef *args;
            _PyStackRef res;
            args = &window[1];
            callable = window[0];
            res = SPAM(callable, args, oparg);
            IR_CLEAR_SLOTS(&window[1], oparg);
            window[0] = res;
            IR_MOVE_TO_DST(IR_FIELD_A(this_instr), window[0]);
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
            res = PyStackRef_NULL;
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
            res = PyStackRef_NULL;
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
            PyObject *o = PyStackRef_AsPyObjectBorrow(owner);
            res = SPAM(o, GETITEM(FRAME_CO_NAMES, oparg));
            INPUTS_DEAD();
        }
        macro(OP) = _SPECIALIZE_OP + _OP;
        op(_GUARD, (type_version/2, owner -- owner)) {
            EXIT_IF(Py_TYPE(PyStackRef_AsPyObjectBorrow(owner))->tp_version_tag != type_version);
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
            _PyFb_OP *fb = (_PyFb_OP *)IR_FB(IR_FIELD_C(this_instr));
            int oparg = IR_FB_ARG(fb, IR_FIELD_C(this_instr));
            (void)oparg;
            _PyStackRef owner;
            _PyStackRef res;
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
                if (Py_UNLIKELY(PyStackRef_IsInline(owner))) {
                    owner = PyStackRef_Box(owner);
                    if (PyStackRef_IsInline(owner)) {
                        JUMP_TO_LABEL(error);
                    }
                    BOX_STAT_INC(OP);
                    IR_KEEP_BOX(owner, IR_FIELD_B(this_instr));
                }
                PyObject *o = PyStackRef_AsPyObjectBorrow(owner);
                res = SPAM(o, GETITEM(FRAME_CO_NAMES, oparg));
            }
            IR_CONSUME_INPUT(IR_FIELD_B(this_instr));
            IR_STORE_DST(IR_FIELD_A(this_instr), res);
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
            _PyFb_OP *fb = (_PyFb_OP *)IR_FB(0);
            int oparg = (int)IR_FIELD_CD(this_instr);
            (void)oparg;
            _PyStackRef owner;
            _PyStackRef res;
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
                if (Py_UNLIKELY(PyStackRef_IsInline(owner))) {
                    owner = PyStackRef_Box(owner);
                    if (PyStackRef_IsInline(owner)) {
                        JUMP_TO_LABEL(error);
                    }
                    BOX_STAT_INC(OP);
                    IR_KEEP_BOX(owner, IR_FIELD_B(this_instr));
                }
                PyObject *o = PyStackRef_AsPyObjectBorrow(owner);
                res = SPAM(o, GETITEM(FRAME_CO_NAMES, oparg));
            }
            IR_CONSUME_INPUT(IR_FIELD_B(this_instr));
            IR_STORE_DST(IR_FIELD_A(this_instr), res);
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
            _PyFb_OP *fb = (_PyFb_OP *)IR_FB(IR_FIELD_C(this_instr));
            int oparg = (int)((_PyFbHeader *)fb)->arg;
            (void)oparg;
            _PyStackRef owner;
            _PyStackRef res;
            // _GUARD
            {
                owner = IR_SLOT(IR_FIELD_B(this_instr));
                uint32_t type_version = fb->type_version;
                if (Py_UNLIKELY(!PyStackRef_IsPointerSpace(owner))) {
                    UPDATE_MISS_STATS(OP);
                    assert(_PyOpcode_Deopt[opcode] == (OP));
                    JUMP_TO_PREDICTED(OP);
                }
                if (Py_TYPE(PyStackRef_AsPyObjectBorrow(owner))->tp_version_tag != type_version) {
                    UPDATE_MISS_STATS(OP);
                    assert(_PyOpcode_Deopt[opcode] == (OP));
                    JUMP_TO_PREDICTED(OP);
                }
            }
            // _OP
            {
                PyObject *o = PyStackRef_AsPyObjectBorrow(owner);
                res = SPAM(o, GETITEM(FRAME_CO_NAMES, oparg));
            }
            IR_CONSUME_INPUT(IR_FIELD_B(this_instr));
            IR_STORE_DST(IR_FIELD_A(this_instr), res);
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
            _PyFb_OP *fb = (_PyFb_OP *)IR_FB(IR_FIELD_D(this_instr));
            int oparg = IR_FB_ARG(fb, IR_FIELD_D(this_instr));
            (void)oparg;
            _PyStackRef left;
            _PyStackRef right;
            _PyStackRef res;
            right = IR_SLOT(IR_FIELD_C(this_instr));
            left = IR_SLOT(IR_FIELD_B(this_instr));
            uint32_t counter = fb->counter;
            (void)counter;
            _PyFbHeader *feedback = ((_PyFbHeader *)fb);
            ADVANCE_ADAPTIVE_COUNTER(feedback);
            res = SPAM(left, right, oparg);
            IR_CONSUME_INPUT(IR_FIELD_C(this_instr));
            IR_CONSUME_INPUT(IR_FIELD_B(this_instr));
            IR_STORE_DST(IR_FIELD_A(this_instr), res);
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
            _PyFb_OP *fb = (_PyFb_OP *)IR_FB(0);
            int oparg = (int)IR_FIELD_D(this_instr);
            (void)oparg;
            _PyStackRef left;
            _PyStackRef right;
            _PyStackRef res;
            right = IR_SLOT(IR_FIELD_C(this_instr));
            left = IR_SLOT(IR_FIELD_B(this_instr));
            uint32_t counter = fb->counter;
            (void)counter;
            _PyFbHeader *feedback = ((_PyFbHeader *)fb);
            ADVANCE_ADAPTIVE_COUNTER(feedback);
            res = SPAM(left, right, oparg);
            IR_CONSUME_INPUT(IR_FIELD_C(this_instr));
            IR_CONSUME_INPUT(IR_FIELD_B(this_instr));
            IR_STORE_DST(IR_FIELD_A(this_instr), res);
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
            int flag = PyStackRef_IsFalse(cond);
            DEAD(cond);
            if (flag) {
                JUMPTO(oparg);
            }
        }
        inst(FOR_ITER, (iter -- iter, next)) {
            next = SPAM(iter);
            if (PyStackRef_IsNull(next)) {
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
            value = GETLOCAL(oparg);
        }
        inst(INSTRUMENTED_LOAD, ( -- value)) {
            value = GETLOCAL(oparg);
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

if __name__ == "__main__":
    unittest.main()
