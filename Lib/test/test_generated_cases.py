import contextlib
from dataclasses import replace
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
    import opcode_id_generator
    import opcode_metadata_generator
    import py_metadata_generator
    import uop_id_generator
    import parser
    import root_check_c
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
                                     ir_generator.build_handler_domain(analysis),
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
            PyObject *o = PyObject_Repr(PyValue_ObjectBorrow(a));
            escaping_call();
            c = PyValue_FromObject(o);
            INPUTS_DEAD();
        }
        """, "'o' is held only in a C local across the collecting call "
             "to 'escaping_call'")
        self.check_error("""
        inst(OP, (a -- c)) {
            PyValue r = PyValue_FromObject(
                PyObject_Repr(PyValue_ObjectBorrow(a)));
            escaping_call();
            c = r;
            INPUTS_DEAD();
        }
        """, "'r' is held only in a C local")

    def test_frame_push_before_initialize(self):
        # A pushed frame's slots hold garbage until it is initialized, so
        # nothing between the two may escape.
        self.check_error("""
        inst(OP, ( -- )) {
            _PyInterpreterFrame *f = _PyThreadState_PushFrameUnchecked(tstate, 4);
            escaping_call();
            _PyFrame_Initialize(tstate, f, PyValue_NULL, NULL, NULL, f->base, 0, frame);
        }
        """, "the escaping call to 'escaping_call' comes between the push "
             "of a frame")
        self.generate("""
        inst(OP, ( -- )) {
            _PyInterpreterFrame *f = _PyThreadState_PushFrameUnchecked(tstate, 4);
            _PyFrame_Initialize(tstate, f, PyValue_NULL, NULL, NULL, f->base, 0, frame);
            escaping_call();
        }
        """)

    def test_root_held_across_allocation(self):
        # An allocation may collect (call_effects.py)
        self.check_error("""
        inst(OP, (a -- c)) {
            PyObject *o = PyObject_Repr(PyValue_ObjectBorrow(a));
            PyObject *n = PyLong_FromLong(1);
            c = PyValue_FromObject(o);
            use(n);
            INPUTS_DEAD();
        }
        """, "'o' is held only in a C local across the collecting call "
             "to 'PyLong_FromLong'")
        # A field of an object, a type's cache included, is no borrow
        self.check_error("""
        inst(OP, (a -- c)) {
            PyHeapTypeObject *cls = (PyHeapTypeObject *)PyValue_ObjectBorrow(a);
            PyObject *init = cls->_spec_cache.init;
            PyObject *o = PyType_GenericAlloc((PyTypeObject *)cls, 0);
            use(o);
            c = PyValue_FromObject(init);
            INPUTS_DEAD();
        }
        """, "'init' is held only in a C local across the collecting call "
             "to 'PyType_GenericAlloc'")

    def test_slot_load_across_allocation(self):
        # A value loaded from a slot is anchored by the slot across an
        # allocation, which runs no Python code, until a slot is written
        self.generate("""
        inst(OP, (--)) {
            PyValue initial = GETLOCAL(oparg);
            PyObject *cell = PyCell_New(NULL);
            ERROR_IF(cell == NULL);
            PyCell_SET_VALUE(cell, initial);
            GETLOCAL(oparg) = PyValue_FromObject(cell);
        }
        """)
        self.check_error("""
        inst(OP, (--)) {
            PyValue initial = GETLOCAL(oparg);
            GETLOCAL(oparg) = PyValue_NULL;
            PyObject *cell = PyCell_New(NULL);
            ERROR_IF(cell == NULL);
            PyCell_SET_VALUE(cell, initial);
            GETLOCAL(oparg) = PyValue_FromObject(cell);
        }
        """, "'initial' is held only in a C local across the collecting "
             "call to 'PyCell_New'")
        # Across a call that may run Python code, the slot may change
        self.check_error("""
        inst(OP, (--)) {
            PyValue initial = GETLOCAL(oparg);
            escaping_call();
            GETLOCAL(oparg) = initial;
        }
        """, "'initial' is held only in a C local across the collecting "
             "call to 'escaping_call'")

    def test_stack_passed_argument(self):
        # The seventh argument is on the stack, where an opaque callee's
        # frame scan does not reach (note 11 question 13)
        self.check_error("""
        inst(OP, (a -- c)) {
            PyObject *o = PyObject_Repr(PyValue_ObjectBorrow(a));
            PyObject *r = seven(1, 2, 3, 4, 5, 6, o);
            c = PyValue_FromObject(r);
            INPUTS_DEAD();
        }
        """, "argument 7 of 'seven' is passed on the stack and is not anchored")
        self.generate("""
        inst(OP, (a -- c)) {
            PyObject *o = PyValue_ObjectBorrow(a);
            PyObject *r = seven(1, 2, 3, 4, 5, 6, o);
            c = PyValue_FromObject(r);
            INPUTS_DEAD();
        }
        """)

    def test_root_held_in_c_stack_ref(self):
        # A _PyValueRoot is a precise root, and a value that is dead before
        # the escaping call needs no root.
        self.generate("""
        inst(OP, (a -- c)) {
            _PyValueRoot held;
            _PyThreadState_PushValueRoot(tstate, &held);
            held.ref = PyValue_FromObject(
                PyObject_Repr(PyValue_ObjectBorrow(a)));
            escaping_call();
            c = _PyThreadState_PopValueRoot(tstate, &held);
            INPUTS_DEAD();
        }
        inst(OP2, (a -- c)) {
            PyObject *o = PyObject_Repr(PyValue_ObjectBorrow(a));
            INPUTS_DEAD();
            ERROR_IF(o == NULL);
            c = PyValue_FromObject(o);
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
        ir_generator.generate_ir([self.temp_input_filename], analysis,
                                 ir_generator.build_handler_domain(analysis), out, False)
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

    def test_derived_handler_semantic_opcode(self):
        analysis = analyzer.analyze_files([ir_generator.DEFAULT_INPUT])
        domain = ir_generator.build_handler_domain(analysis)
        out = io.StringIO()
        ir_generator.generate_ir_cases(
            analysis, domain, ir_generator.compute_formats(analysis), out, False)
        derived = {name: inst for name, inst in
                   ir_generator.handler_instructions(analysis, domain).items()
                   if name != inst.name and inst.properties.uses_opcode}
        self.assertTrue(derived)
        for name, inst in derived.items():
            with self.subTest(handler=name):
                body = self.handler(out.getvalue(), name)
                lines = [line.strip() for line in body.splitlines()]
                # Tail-call dispatch starts with its raw ID. Shared bodies
                # need the instruction ID for semantic opcode comparisons.
                self.assertIn(f"int opcode = {name};", lines)
                self.assertIn(f"opcode = {inst.name};", lines)
                self.assertNotIn(f"opcode = {name};", lines)

    def test_counted_handler_entry_names(self):
        analysis = analyzer.analyze_files([ir_generator.DEFAULT_INPUT])
        domain = ir_generator.build_handler_domain(analysis)
        for tailcall in (False, True):
            out = io.StringIO()
            ir_generator.generate_ir_cases(
                analysis, domain, ir_generator.compute_formats(analysis), out, tailcall)
            for name, inst in ir_generator.handler_instructions(analysis, domain).items():
                with self.subTest(tailcall=tailcall, handler=name):
                    body = self.handler(out.getvalue(), name)
                    counter = f"INSTRUCTION_STATS({name});"
                    self.assertEqual(body.count(counter), 1)
                    if inst.is_target and name == inst.name:
                        self.assertLess(body.index(counter), body.index(f"IR_PREDICTED({name});"))

    def test_register_style(self):
        # Inputs that are only used as objects are read through their
        # fields.  They stay in their slots until the handler leaves, so the
        # slots keep them alive across calls that may collect.  Boxing the
        # second input allocates, so the first box has a root while it does;
        # the root is popped before either exit.
        input = """
        inst(OP, (left, right -- res)) {
            PyObject *l = PyValue_ObjectBorrow(left);
            PyObject *r = PyValue_ObjectBorrow(right);
            PyObject *o = SPAM(l, r);
            INPUTS_DEAD();
            ERROR_IF(o == NULL);
            res = PyValue_FromObject(o);
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
            PyValue left;
            PyValue right;
            PyValue res;
            right = IR_SLOT(IR_FIELD_C(this_instr));
            left = IR_SLOT(IR_FIELD_B(this_instr));
            PyObject *left_object;
            if (Py_UNLIKELY(PyValue_IsInline(left))) {
                left_object = _PyValue_BoxInline(left);
                if (left_object == NULL) {
                    JUMP_TO_LABEL(error);
                }
                BOX_STAT_INC(OP, _PY_BOX_OTHER);
            }
            else {
                left_object = PyValue_ObjectBorrow(left);
            }
            PyObject *right_object;
            if (Py_UNLIKELY(PyValue_IsInline(right))) {
                _PyValueRoot left_root;
                if (Py_UNLIKELY(PyValue_IsInline(left))) {
                    _PyThreadState_PushValueRoot(tstate, &left_root);
                    left_root.object = left_object;
                }
                right_object = _PyValue_BoxInline(right);
                if (Py_UNLIKELY(PyValue_IsInline(left))) {
                    _PyThreadState_PopValueRoot(tstate, &left_root);
                }
                if (right_object == NULL) {
                    JUMP_TO_LABEL(error);
                }
                BOX_STAT_INC(OP, _PY_BOX_OTHER);
            }
            else {
                right_object = PyValue_ObjectBorrow(right);
            }
            PyObject *l = left_object;
            PyObject *r = right_object;
            PyObject *o = SPAM(l, r);
            if (o == NULL) {
                IR_CONSUME_INPUT(IR_FIELD_B(this_instr));
                IR_CONSUME_INPUT(IR_FIELD_C(this_instr));
                JUMP_TO_LABEL(error);
            }
            res = PyValue_FromObject(o);
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
            res = SPAM(PyValue_ObjectBorrow(value));
        }
        inst(OP_OBJ, (value -- res)) {
            res = SPAM(PyValue_ObjectBorrow(value));
            DEAD(value);
        }
        inst(OP_FAMILY, (value -- res)) {
            res = SPAM(PyValue_ObjectBorrow(value));
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
            int flag = _PyValue_IsFalse(cond);
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
            res = SPAM(PyValue_ObjectBorrow(left),
                       PyValue_ObjectBorrow(right));
            INPUTS_DEAD();
        }
        inst(ATTR, (owner -- attr, null[oparg & 1])) {
            attr = SPAM(PyValue_ObjectBorrow(owner));
            INPUTS_DEAD();
            if (oparg & 1) {
                null[0] = PyValue_NULL;
            }
        }
        inst(ALIAS, (value -- res)) {
            res = value;
            DEAD(value);
        }
        inst(PAIR, (-- a, b)) {
            a = SPAM();
            b = SPAM();
        }
    """
        formats = self.formats(input)
        self.assertTrue(formats["OP"].store_dst)
        self.assertTrue(formats["ATTR"].store_dst)
        # A result copied from an input is a value like any other
        self.assertTrue(formats["ALIAS"].store_dst)
        # PAIR never pushes its first output alone
        self.assertTrue(formats["PAIR"].store_dst)
        output = self.ir_output(input)
        self.assertIn("IR_STORE_DST(IR_FIELD_A(this_instr), res);",
                      self.handler(output, "OP"))
        self.assertIn("IR_STORE_DST(IR_FIELD_A(this_instr), attr);",
                      self.handler(output, "ATTR"))
        self.assertIn("IR_STORE_DST(IR_FIELD_A(this_instr), res);",
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
            res = SPAM(PyValue_ObjectBorrow(right));
            DEAD(right);
        }
        macro(OP) = _GUARD + _USE;
    """
        fmt = self.formats(input)["OP"]
        self.assertEqual(fmt.inputs, ["nos", "unused"])
        self.assertEqual(fmt.dst_offset, -1)
        self.assertEqual(fmt.borrow_mask, 0b10)

    def test_box_root_across_allocation(self):
        # A box is held in a C local, which the collector does not scan, so
        # an allocating call in the body has the box's root pushed around it.
        input = """
        inst(OP, (value -- res)) {
            PyObject *o = PyValue_ObjectBorrow(value);
            PyObject *f = PyFloat_FromDouble(1.0);
            if (f == NULL) {
                ERROR_NO_POP();
            }
            res = PyValue_FromObject(SPAM(o, f));
            INPUTS_DEAD();
        }
    """
        handler = self.handler(self.ir_output(input), "OP")
        call = handler.index("PyFloat_FromDouble(1.0)")
        push = handler.index("_PyThreadState_PushValueRoot(tstate, &value_root);")
        pop = handler.index("_PyThreadState_PopValueRoot(tstate, &value_root);")
        self.assertIn("_PyValueRoot value_root;", handler[:push])
        self.assertLess(push, call)
        self.assertLess(call, pop)

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
            PyValue *window = &IR_SLOT(IR_FIELD_B(this_instr));
            (void)window;
            PyValue callable;
            PyValue *args;
            PyValue res;
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

    def test_stores_early(self):
        # A stores_early instruction stores its output in the frame it
        # returns to before it clears its own (note 11 section 4.4), so the
        # output is written only by the body, through IR_RETURN_EARLY().
        input = """
        window inst(OP, (value -- res)) {
            PyValue temp = value;
            INPUTS_DEAD();
            SAVE_STACK();
            frame = SPAM(temp);
            RELOAD_STACK();
            res = temp;
        }
        window stores_early inst(OP_EARLY, (value -- res)) {
            IR_RETURN_EARLY(frame->previous, value);
            PyValue temp = value;
            INPUTS_DEAD();
            SAVE_STACK();
            frame = SPAM(temp);
            RELOAD_STACK();
            res = temp;
        }
    """
        output = self.ir_output(input)
        late = self.handler(output, "OP")
        self.assertIn("IR_STORE_RETURN(res);", late)
        early = self.handler(output, "OP_EARLY")
        self.assertNotIn("IR_STORE_RETURN", early)
        self.assertIn("(void)res;", early)

    def test_stores_early_needs_early_store(self):
        input = """
        window stores_early inst(OP, (value -- res)) {
            PyValue temp = value;
            INPUTS_DEAD();
            SAVE_STACK();
            frame = SPAM(temp);
            RELOAD_STACK();
            res = temp;
        }
    """
        with self.assertRaisesRegex(SyntaxError, "IR_RETURN_EARLY"):
            self.ir_output(input)

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
            PyObject *o = PyValue_ObjectBorrow(owner);
            res = SPAM(o, GETITEM(FRAME_CO_NAMES, oparg));
            INPUTS_DEAD();
        }
        macro(OP) = _SPECIALIZE_OP + _OP;
        op(_GUARD, (type_version/2, owner -- owner)) {
            EXIT_IF(Py_TYPE(PyValue_ObjectBorrow(owner))->tp_version_tag != type_version);
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
                PyObject *owner_object;
                if (Py_UNLIKELY(PyValue_IsInline(owner))) {
                    owner_object = _PyValue_BoxInline(owner);
                    if (owner_object == NULL) {
                        JUMP_TO_LABEL(error);
                    }
                    BOX_STAT_INC(OP, _PY_BOX_OTHER);
                }
                else {
                    owner_object = PyValue_ObjectBorrow(owner);
                }
                PyObject *o = owner_object;
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
            INSTRUCTION_STATS(OP_NOFB);
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
                PyObject *owner_object;
                if (Py_UNLIKELY(PyValue_IsInline(owner))) {
                    owner_object = _PyValue_BoxInline(owner);
                    if (owner_object == NULL) {
                        JUMP_TO_LABEL(error);
                    }
                    BOX_STAT_INC(OP, _PY_BOX_OTHER);
                }
                else {
                    owner_object = PyValue_ObjectBorrow(owner);
                }
                PyObject *o = owner_object;
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
            PyValue owner;
            PyValue res;
            // _GUARD
            {
                owner = IR_SLOT(IR_FIELD_B(this_instr));
                uint32_t type_version = fb->type_version;
                if (Py_UNLIKELY(!_PyValue_IsPointerSpace(owner))) {
                    UPDATE_MISS_STATS(OP);
                    assert(_PyOpcode_Deopt[opcode] == (OP));
                    JUMP_TO_PREDICTED(OP);
                }
                if (Py_TYPE(PyValue_ObjectBorrow(owner))->tp_version_tag != type_version) {
                    UPDATE_MISS_STATS(OP);
                    assert(_PyOpcode_Deopt[opcode] == (OP));
                    JUMP_TO_PREDICTED(OP);
                }
            }
            // _OP
            {
                PyObject *o = PyValue_ObjectBorrow(owner);
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
            INSTRUCTION_STATS(OP_NOFB);
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
            int flag = _PyValue_IsFalse(cond);
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
            PRE_DISPATCH_GOTO();
            DISPATCH_GOTO();
        }
    """
        self.run_ir_test(input, output)
        self.assertTrue(self.formats(input)["IR_PASS"].transparent)

    def test_runs_code(self):
        # A family runs Python code if any member does: an escaping call,
        # the eval breaker, a frame switch, or an instrumented twin, which
        # fires a monitoring event.  Copy propagation does not keep a
        # folded load across such an op (Python/regcfg_passes.c).
        input = """
        inst(OP_QUIET, (value -- res)) {
            res = SPAM(PyValue_ObjectBorrow(value));
            DEAD(value);
        }
        inst(OP_FAMILY, (value -- res)) {
            res = SPAM(PyValue_ObjectBorrow(value));
            DEAD(value);
        }
        inst(OP_FAMILY_CALLS, (value -- res)) {
            res = spam_call(PyValue_ObjectBorrow(value));
            DEAD(value);
        }
        family(OP_FAMILY, 0) = { OP_FAMILY_CALLS };
        inst(OP_PUSH, (value -- res)) {
            res = SPAM(PyValue_ObjectBorrow(value));
            DEAD(value);
        }
        window inst(OP_PUSH_FRAME, (value -- res)) {
            frame = SPAM(value);
            res = PyValue_NULL;
            INPUTS_DEAD();
        }
        family(OP_PUSH, 0) = { OP_PUSH_FRAME };
        op(_CHECK_PERIODIC, ( -- )) {
            SPAM();
        }
        macro(OP_BREAKER) = _CHECK_PERIODIC;
        inst(OP_TWIN, (value -- res)) {
            res = SPAM(PyValue_ObjectBorrow(value));
            DEAD(value);
        }
        inst(INSTRUMENTED_OP_TWIN, (value -- res)) {
            res = SPAM(PyValue_ObjectBorrow(value));
            DEAD(value);
        }
    """
        formats = self.formats(input)
        self.assertFalse(formats["OP_QUIET"].runs_code)
        self.assertTrue(formats["OP_FAMILY"].runs_code)
        self.assertTrue(formats["OP_PUSH"].runs_code)
        self.assertTrue(formats["OP_BREAKER"].runs_code)
        self.assertTrue(formats["OP_TWIN"].runs_code)
        self.assertIs(formats["INSTRUMENTED_OP_TWIN"], formats["OP_TWIN"])
        out = io.StringIO()
        analysis = self.analyze(input)
        ir_metadata_generator.generate_ir_metadata(
            [self.temp_input_filename], analysis,
            ir_generator.build_handler_domain(analysis), out)
        lines = {line.split(" = ", 1)[0]: line
                 for line in map(str.strip, out.getvalue().splitlines())
                 if line.startswith("[")}
        self.assertNotIn("_PyIR_RUNS_CODE", lines["[OP_QUIET]"])
        self.assertIn("_PyIR_RUNS_CODE", lines["[OP_FAMILY_CALLS]"])
        self.assertIn("_PyIR_RUNS_CODE", lines["[OP_TWIN]"])

    def test_borrowed_input_read_before_code(self):
        # A folded local is read where the handler takes the input.  An
        # input that a member takes only after a uop that may run Python
        # code is not borrowable; a specializer runs none.
        input = """
        op(_CALLS, (value -- value)) {
            spam_call(PyValue_ObjectBorrow(value));
        }
        specializing op(_SPECIALIZE, (value -- value)) {
            spam_specialize(PyValue_ObjectBorrow(value));
        }
        op(_TAKE, (left, value -- res)) {
            res = SPAM(PyValue_ObjectBorrow(left), PyValue_ObjectBorrow(value));
            DEAD(left);
            DEAD(value);
        }
        op(_PEEK_CALLS, (value -- value)) {
            spam_call();
        }
        macro(OP_LATE) = _CALLS + _TAKE;
        macro(OP_SPECIALIZED) = _SPECIALIZE + _TAKE;
        macro(OP_PEEK) = _PEEK_CALLS + _TAKE;
    """
        formats = self.formats(input)
        # left is taken after _CALLS ran; value before
        self.assertEqual(formats["OP_LATE"].borrow_mask, 0b10)
        self.assertEqual(formats["OP_SPECIALIZED"].borrow_mask, 0b11)
        # _PEEK_CALLS takes value without using it, so the handler reads
        # value's field in _TAKE, after the call
        self.assertEqual(formats["OP_PEEK"].borrow_mask, 0b00)
        handler = self.handler(self.ir_output(input), "OP_PEEK")
        self.assertLess(handler.index("spam_call()"),
                        handler.index("value = IR_SLOT("))

    def test_runs_code_covers_every_handler(self):
        # Over the real instruction set, each instruction that may run
        # Python code by its own properties marks its family's format, so
        # that a member added later cannot leave the family unmarked.
        with handle_stderr():
            analysis = analyzer.analyze_files(
                [os.path.join(test_tools.basepath, "Python", "bytecodes.c")])
        formats = ir_generator.compute_formats(analysis)
        insts = {**analysis.instructions, **analysis.ir_only}
        checked = 0
        for name, inst in insts.items():
            fmt = formats.get(name)
            if fmt is None or fmt.transparent:
                continue
            uops = [p for p in inst.parts if isinstance(p, analyzer.Uop)
                    and "specializing" not in p.annotations]
            own = name.startswith("INSTRUMENTED_") or any(
                u.properties.escapes or u.properties.eval_breaker
                or u.properties.switches_frame or u.properties.returns
                for u in uops)
            if own:
                checked += 1
                self.assertTrue(fmt.runs_code, name)
        self.assertGreater(checked, 100)
        for name in ("CALL_PY_EXACT_ARGS", "LOAD_ATTR_PROPERTY",
                     "INSTRUMENTED_NOT_TAKEN", "LOAD_GLOBAL", "BINARY_OP"):
            self.assertTrue(formats[name].runs_code, name)
        for name in ("LOAD_CONST", "LOAD_FAST_BORROW", "LOAD_DEREF",
                     "BUILD_TUPLE", "IS_OP", "PUSH_NULL", "STORE_FAST"):
            self.assertFalse(formats[name].runs_code, name)

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
            [self.temp_input_filename], analysis,
            ir_generator.build_handler_domain(analysis), out)
        lines = [line.strip() for line in out.getvalue().splitlines()]
        # The twin fires a monitoring event, so the family runs code
        load = ("{ .flags = _PyIR_VALID | _PyIR_STORE_DST | _PyIR_RUNS_CODE, "
                ".roles = { _PyIR_ROLE_DST, "
                "_PyIR_ROLE_IMM, _PyIR_ROLE_NONE, _PyIR_ROLE_NONE }, "
                ".imm = _PyIR_LOC_B, .fb = _PyIR_LOC_NONE, .ninputs = 0, "
                ".dst = 0, .borrow_mask = 0x0, .nofb = 0 }")
        for expected in [
            "#define IR_BACON                                       256",
            "#define IR_PASS                                        257",
            f"[LOAD] = {load},",
            "[INSTRUMENTED_LOAD] = "
            f"{load.replace('LOAD,', 'INSTRUMENTED_LOAD,')},",
            "[IR_PASS] = { .flags = _PyIR_VALID | _PyIR_TRANSPARENT, "
            ".roles = { _PyIR_ROLE_NONE, _PyIR_ROLE_NONE, _PyIR_ROLE_NONE, "
            "_PyIR_ROLE_NONE }, .imm = _PyIR_LOC_NONE, .fb = _PyIR_LOC_NONE, "
            ".ninputs = 0, .dst = _PyIR_NO_DST, .borrow_mask = 0x0, "
            ".nofb = 0 },",
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
        domain = ir_generator.build_handler_domain(analysis)
        self.assertEqual(domain.opmap["JUMP"], 256)
        self.assertEqual(domain.first_ir_only, 257)
        self.assertEqual(domain.handlers["IR_BACON"], 257)

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
        domain = ir_generator.build_handler_domain(analysis)
        ids = domain.handlers
        self.assertNotIn("BINARY_OP", ids)
        self.assertEqual(ids["IR_BACON"], 256)
        self.assertEqual([ids[name] for name in handlers],
                         list(range(257, 257 + len(handlers))))
        formats = ir_generator.compute_formats(analysis)
        self.assertIs(formats["BINARY_OP_ADD"], formats["BINARY_OP"])
        self.assertEqual(
            domain.bases["BINARY_OP_ADD"],
            "BINARY_OP")

        out = io.StringIO()
        ir_generator.generate_ir([self.temp_input_filename], analysis,
                                 ir_generator.build_handler_domain(analysis), out, False)
        code = out.getvalue()
        self.assertNotIn("TARGET(BINARY_OP)", code)
        generic = code[code.index("TARGET(BINARY_OP_SUBTRACT)"):]
        generic = generic[:generic.index("DISPATCH();")]
        self.assertIn("int oparg = NB_SUBTRACT;", generic)
        self.assertIn("INSTRUCTION_STATS(BINARY_OP_SUBTRACT);", generic)
        handler = code[code.index("TARGET(BINARY_OP_ADD_INT)"):]
        handler = handler[:handler.index("DISPATCH();")]
        self.assertIn("IR_DEOPT_BINARY_OP(IR_FIELD_D(this_instr));", handler)
        self.assertNotIn("JUMP_TO_PREDICTED", handler)

        out = io.StringIO()
        ir_metadata_generator.generate_ir_metadata(
            [self.temp_input_filename], analysis,
            ir_generator.build_handler_domain(analysis), out)
        lines = [line.strip() for line in out.getvalue().splitlines()]
        self.assertIn("#define _PyIR_BINARY_OP_FIRST" + " " * 26
                      + "BINARY_OP_ADD", lines)
        self.assertIn('static_assert(BINARY_OP_ADD == _PyIR_BINARY_OP_FIRST + '
                      'NB_ADD, "BINARY_OP_ADD");', lines)


class TestHandlerDomain(unittest.TestCase):
    def analyze(self):
        return analyzer.analyze_forest(parse_src("""
            inst(NOP, (--)) { }
            inst(POP, (value --)) { DEAD(value); }
        """))

    def test_adaptive_own_feedback(self):
        analysis = analyzer.analyze_files([ir_generator.DEFAULT_INPUT])
        domain = ir_generator.build_handler_domain(analysis)
        adaptive = ir_generator.HandlerKind.ADAPTIVE
        for name in ("LOAD_ATTR", "GET_ITER", "TO_BOOL", "POP_JUMP_IF_FALSE",
                     "CALL_FUNCTION_EX", "BINARY_OP_ADD", "BINARY_OP_SUBSCR"):
            with self.subTest(adaptive=name):
                self.assertTrue(domain.kinds[name] & adaptive,
                                f"missing own-feedback adaptive property: {name}")
        for name in ("BINARY_OP", "BINARY_SLICE", "STORE_SLICE", "IR_JUMP",
                     "CACHE", "JUMP_BACKWARD", "LOAD_ATTR_NOFB",
                     "BINARY_OP_ADD_INT", "INSTRUMENTED_LOAD_SUPER_ATTR"):
            with self.subTest(nonadaptive=name):
                self.assertFalse(domain.kinds[name] & adaptive)
        self.assertEqual(domain.bases["BINARY_OP_ADD"], "BINARY_OP")

    def test_adaptive_metadata_export(self):
        analysis = analyzer.analyze_files([ir_generator.DEFAULT_INPUT])
        domain = ir_generator.build_handler_domain(analysis)
        output = io.StringIO()
        py_metadata_generator.generate_handler_metadata(domain, CWriter(output, 0, False))
        data = {}
        exec(output.getvalue(), data)
        self.assertEqual(data["_HANDLER_ADAPTIVE"], 512)
        for name in ("GET_ITER", "BINARY_OP_ADD", "LOAD_ATTR_NOFB"):
            identity = {**domain.opmap, **domain.handlers}[name]
            self.assertEqual(data["_handler_flags"][identity], domain.kinds[name])
        kinds = dict(domain.kinds)
        kinds["LOAD_ATTR_NOFB"] |= ir_generator.HandlerKind.ADAPTIVE
        with self.assertRaisesRegex(ValueError, "invalid adaptive handler"):
            replace(domain, kinds=kinds)

    def test_monitoring_relations(self):
        analysis = analyzer.analyze_files([ir_generator.DEFAULT_INPUT])
        domain = ir_generator.build_handler_domain(analysis)
        for base, twin in domain.instrumented.items():
            self.assertTrue(domain.kinds[twin] & ir_generator.HandlerKind.INSTRUMENTED)
            if base != twin:
                self.assertEqual(domain.deinstrumented[twin], base)
            self.assertEqual(domain.instrumented[twin], twin)
        self.assertEqual(domain.instrumented["CALL"], "INSTRUMENTED_CALL")
        self.assertEqual(domain.instrumented["INSTRUMENTED_LINE"], "INSTRUMENTED_LINE")
        self.assertNotIn("INSTRUMENTED_LINE", domain.deinstrumented)
        with self.assertRaises(TypeError):
            domain.instrumented["CALL"] = "NOP"
        with self.assertRaisesRegex(ValueError, "instrumentation relation"):
            replace(domain, instrumented={"CALL": "NOP"})
        with self.assertRaisesRegex(ValueError, "deinstrumentation relation"):
            replace(domain, deinstrumented={"INSTRUMENTED_CALL": "NOP"})
        minimal = ir_generator.build_handler_domain(self.analyze())
        self.assertEqual(dict(minimal.instrumented), {})
        self.assertEqual(dict(minimal.deinstrumented), {})

    def test_full_domain_name_table(self):
        analysis = analyzer.analyze_forest(parse_src("""
            inst(NOP, (--)) { }
            ir_only inst(IR_BACON, (--)) { }
        """))
        domain = ir_generator.build_handler_domain(analysis)
        self.assertEqual(domain.handlers["IR_BACON"], 256)
        out = io.StringIO()
        opcode_metadata_generator.generate_name_table(
            analysis, domain, CWriter(out, 0, False))
        self.assertIn('[256] = "IR_BACON",', out.getvalue())
        self.assertIn('_PyOpcode_OpName[IR_NUM_HANDLERS]', out.getvalue())
        self.assertNotIn('"INSTRUMENTED_LINE"', out.getvalue())

    def test_python_handler_metadata(self):
        analysis = analyzer.analyze_files([ir_generator.DEFAULT_INPUT])
        domain = ir_generator.build_handler_domain(analysis)
        out = io.StringIO()
        py_metadata_generator.generate_handler_metadata(domain, CWriter(out, 0, False))
        data = {}
        exec(out.getvalue(), data)
        ids = {**domain.opmap, **domain.handlers}
        self.assertEqual(len(data["_handler_flags"]), domain.count)
        self.assertEqual(len(data["_handler_bases"]), domain.count)
        self.assertEqual(len(data["_handler_operations"]), domain.count)
        for name, handler in ids.items():
            with self.subTest(handler=name):
                self.assertEqual(data["_handler_flags"][handler], domain.kinds[name])
                self.assertEqual(data["_handler_bases"][handler], ids[domain.bases[name]])
                self.assertEqual(data["_handler_operations"][handler], domain.operations.get(name))
        self.assertEqual(domain.operations["BINARY_OP_ADD"], 0)
        self.assertEqual(set(domain.operations.values()), set(range(len(ir_generator.binary_ops()))))
        with self.assertRaises(TypeError):
            domain.operations["BINARY_OP_ADD"] = 1
        with self.assertRaisesRegex(ValueError, "operation metadata"):
            replace(domain, operations={})
        with self.assertRaisesRegex(ValueError, "operation metadata"):
            replace(domain, operations=dict(domain.operations, BINARY_OP_ADD=-1))

    def test_capacity_without_large_tables(self):
        for count in (1, 255, 256, 512, 65535, 65536):
            with self.subTest(count=count):
                ir_generator.check_handler_count(count)
        for count in (0, -1, 65537):
            with self.subTest(count=count):
                with self.assertRaisesRegex(ValueError, "16-bit domain"):
                    ir_generator.check_handler_count(count)
        domain = ir_generator.build_handler_domain(self.analyze())
        with self.assertRaisesRegex(ValueError, "16-bit domain"):
            replace(domain, count=65537)

    def test_absent_instrumented_pin(self):
        # The legacy LINE pin reserves 255 without requiring its body in a
        # minimal DSL. Previously the real instrumented handler became 256,
        # outside the dispatch table or colliding with the first IR handler.
        source = """
            inst(INSTRUMENTED_TWIN, (--)) {
                PRE_DISPATCH_GOTO();
                DISPATCH_GOTO();
            }
        """
        for extra, count in (("", 256),
                             ("ir_only inst(IR_BACON, (--)) { }", 257)):
            with self.subTest(ir_only=bool(extra)):
                analysis = analyzer.analyze_forest(parse_src(source + extra))
                domain = ir_generator.build_handler_domain(analysis)
                self.assertEqual(domain.opmap["INSTRUMENTED_LINE"], 255)
                self.assertNotIn("INSTRUMENTED_LINE", domain.handlers)
                self.assertEqual(domain.handlers["INSTRUMENTED_TWIN"], 254)
                self.assertEqual(domain.count, count)
                if extra:
                    self.assertEqual(domain.handlers["IR_BACON"], 256)
                self.assertEqual(len(set(domain.handlers.values())),
                                 len(domain.handlers))

    def test_duplicate_id_rejected(self):
        domain = ir_generator.build_handler_domain(self.analyze())
        # An ID accidentally reused by the allocator must fail before any
        # consumer can emit a plausible but ambiguous dispatch table.
        opmap = dict(domain.opmap, NOP=domain.opmap["CACHE"])
        handlers = dict(domain.handlers, NOP=domain.opmap["CACHE"])
        with self.assertRaisesRegex(ValueError, "duplicate handler ID"):
            replace(domain, opmap=opmap, handlers=handlers)

    def test_derived_name_collision_rejected(self):
        analysis = analyzer.analyze_forest(parse_src("""
            ir_only inst(RESUME, (--)) { }
        """))
        with self.assertRaises(ValueError):
            ir_generator.build_handler_domain(analysis)

    def test_base_metadata_rejected(self):
        domain = ir_generator.build_handler_domain(self.analyze())
        bases = dict(domain.bases, NOP="MISSING")
        with self.assertRaisesRegex(ValueError, "invalid handler base"):
            replace(domain, bases=bases)

    def test_consumers_reject_another_analysis(self):
        analysis = self.analyze()
        domain = ir_generator.build_handler_domain(self.analyze())
        consumers = (
            (opcode_id_generator.generate_opcode_header, ()),
            (opcode_metadata_generator.generate_opcode_metadata, ()),
            (py_metadata_generator.generate_py_metadata, ()),
            (uop_id_generator.generate_uop_ids, (False,)),
            (ir_metadata_generator.generate_ir_metadata, ()),
            (ir_generator.generate_ir, (False,)),
            (ir_generator.generate_ir_targets, ()),
        )
        for generate, extra in consumers:
            with self.subTest(consumer=generate.__name__):
                out = io.StringIO()
                with self.assertRaisesRegex(ValueError, "different analysis"):
                    generate(["test.c"], analysis, domain, out, *extra)
                self.assertEqual(out.getvalue(), "")

    def test_runtime_state_and_pristine_are_distinct(self):
        analysis = analyzer.analyze_files([ir_generator.DEFAULT_INPUT])
        domain = ir_generator.build_handler_domain(analysis)
        kind, state = ir_generator.HandlerKind, ir_generator.HandlerState
        for name in ("CACHE", "NOP", "LOAD_CONST_INLINE_INT",
                     "LOAD_CONST_INLINE_FLOAT", "LOAD_SMALL_INT_INLINE",
                     "BINARY_OP_ADD", "LOAD_ATTR_NOFB", "IR_JUMP"):
            with self.subTest(pristine=name):
                self.assertTrue(domain.kinds[name] & kind.METADATA)
                self.assertTrue(domain.kinds[name] & kind.DISPATCH)
                self.assertTrue(domain.kinds[name] & kind.PRISTINE)
                self.assertEqual(domain.required_state[name], 0)
        for name, required in (
                ("BINARY_OP_ADD_INT", state.SPECIALIZATION),
                ("INSTRUMENTED_LINE", state.MONITORING),
                ("INSTRUMENTED_LOAD_SUPER_ATTR_NOFB", state.MONITORING),
                ("INTERPRETER_EXIT", state.STATIC_EXEC),
                ("EXIT_INIT_CHECK", state.STATIC_EXEC)):
            with self.subTest(runtime=name):
                self.assertTrue(domain.kinds[name] & kind.DISPATCH)
                self.assertFalse(domain.kinds[name] & kind.PRISTINE)
                self.assertEqual(domain.required_state[name], required)
        self.assertTrue(domain.kinds["LOAD_CONST_INLINE_INT"] & kind.SPECIALIZED)
        self.assertTrue(domain.kinds["BINARY_OP"] & kind.METADATA)
        self.assertFalse(domain.kinds["BINARY_OP"] & kind.DISPATCH)
        self.assertFalse(domain.kinds["BINARY_OP"] & kind.PRISTINE)
        out = io.StringIO()
        opcode_id_generator.generate_opcode_header(["test.c"], analysis, domain, out)
        self.assertRegex(out.getvalue(), rf"#define IR_NUM_HANDLERS\s+{domain.count}\b")

    def test_inconsistent_admission_properties_rejected(self):
        domain = ir_generator.build_handler_domain(self.analyze())
        kind = ir_generator.HandlerKind
        with self.assertRaisesRegex(ValueError, "dispatch handler has no metadata"):
            replace(domain, kinds=dict(domain.kinds, NOP=kind.DISPATCH))
        state = ir_generator.HandlerState.SPECIALIZATION
        with self.assertRaisesRegex(ValueError, "invalid pristine handler"):
            replace(domain, required_state=dict(domain.required_state, NOP=state))

    def test_conflicting_pristine_state_rejected(self):
        for annotations in ("pristine static_exec", "pristine"):
            name = "NOP" if "static_exec" in annotations else "INSTRUMENTED_LINE"
            with self.subTest(annotations=annotations):
                analysis = analyzer.analyze_forest(parse_src(
                    f"{annotations} inst({name}, (--)) {{ }}"))
                with self.assertRaisesRegex(ValueError, "pristine handler requires runtime state"):
                    ir_generator.build_handler_domain(analysis)

    def test_base_does_not_classify_specialization(self):
        analysis = analyzer.analyze_files([ir_generator.DEFAULT_INPUT])
        domain = ir_generator.build_handler_domain(analysis)
        kind = ir_generator.HandlerKind
        for name, base in (("BINARY_OP_ADD", "BINARY_OP"),
                           ("LOAD_ATTR_NOFB", "LOAD_ATTR"),
                           ("INSTRUMENTED_LOAD_SUPER_ATTR_NOFB",
                            "INSTRUMENTED_LOAD_SUPER_ATTR")):
            with self.subTest(handler=name):
                self.assertEqual(domain.bases[name], base)
                self.assertNotEqual(name, base)
                self.assertFalse(domain.kinds[name] & kind.SPECIALIZED)
        self.assertTrue(domain.kinds["BINARY_OP_ADD_INT"] & kind.SPECIALIZED)
        self.assertEqual(domain.bases["BINARY_OP_ADD_INT"], "BINARY_OP")
        self.assertEqual(domain.bases["IR_JUMP"], "JUMP_FORWARD")
        self.assertTrue(domain.kinds["INSTRUMENTED_LOAD_SUPER_ATTR_NOFB"]
                        & kind.INSTRUMENTED)
        self.assertTrue(domain.kinds["INSTRUMENTED_LOAD_SUPER_ATTR_NOFB"]
                        & kind.NOFB)


class TestRootCheckC(unittest.TestCase):
    """root_check's C mode (Tools/cases_generator/root_check_c.py, note 11
    section 2.1).  Each negative case is a precise function that must
    produce the named error; each positive case must check clean.
    collect() is an unknown function, so a reentrant call; use(), use_v()
    and use_p() are non-collecting."""

    PRELUDE = """static inline void use(PyObject *o) { (void)o; }
static inline void use_v(PyValue v) { (void)v; }
static inline void use_p(const char *p) { (void)p; }
"""

    NEGATIVE = {
        "no_span": ("""
Py_GC_PRECISE static void f(void) {
    PyObject *x = PyLong_FromLong(1);
    collect();
    use(x);
}
""", "'x' is held only in a C local across the collecting call to 'collect'"),
        "store_after_pop": ("""
Py_GC_PRECISE static void f(PyThreadState *tstate) {
    _PyValueRoot r;
    _PyThreadState_PushValueRoot(tstate, &r);
    _PyThreadState_PopValueRoot(tstate, &r);
    PyObject *x = PyLong_FromLong(1);
    r.object = x;
    collect();
    use(x);
}
""", "a store into span 'r', which is not pushed"),
        "pop_on_one_path": ("""
Py_GC_PRECISE static int f(PyThreadState *tstate) {
    _PyValueRoot r;
    _PyThreadState_PushValueRoot(tstate, &r);
    if (collect() < 0) {
        goto error;
    }
    _PyThreadState_PopValueRoot(tstate, &r);
    return 0;
error:
    return -1;
}
""", 'a span is pushed on one path and not on another'),
        "pymem_as_slots": ("""
Py_GC_PRECISE static void f(void) {
    PyValue *buf = PyMem_Malloc(4 * sizeof(PyValue));
    PyValue v = buf[0];
    collect();
    use_v(v);
}
""", "'v' is held only in a C local across the collecting call to 'collect' ('buf', which is not a frame slot pointer)"),
        "anchor_overwritten": ("""
Py_GC_PRECISE static void f(PyThreadState *tstate) {
    _PyValueRoot r;
    _PyThreadState_PushValueRoot(tstate, &r);
    PyObject *x = PyLong_FromLong(1);
    r.object = x;
    PyObject *y = PyLong_FromLong(2);
    r.object = y;
    collect();
    use(x);
    use(y);
    _PyThreadState_PopValueRoot(tstate, &r);
}
""", "'x' is held only in a C local across the collecting call to 'collect'"),
        "list_item_reentrant": ("""
Py_GC_PRECISE static void f(PyListObject *list) {
    PyValue v = PyList_GET_VALUE(list, 0);
    collect();
    use_v(v);
}
""", "'v' is held only in a C local across the collecting call to 'collect' (a borrow of mutable field 'ob_item' across a reentrant call)"),
        "weakref_across_alloc": ("""
Py_GC_PRECISE static void f(PyWeakReference *wr) {
    PyObject *o = wr->wr_object;
    PyObject *n = PyLong_FromLong(1);
    use(o);
    use(n);
}
""", "'o' is held only in a C local across the collecting call to 'PyLong_FromLong' (a borrow of untraced or weak field 'wr_object')"),
        "param_after_call": ("""
Py_GC_PRECISE static void f(PyObject *p) {
    collect();
    use(p);
}
""", "'p' is held only in a C local across the collecting call to 'collect' (parameter 'p', which no span covers)"),
        "sibling_temporary": ("""
Py_GC_PRECISE static void f(void) {
    g(PyLong_FromLong(5), collect());
}
""", "is a temporary held across the collecting call to 'collect'"),
        # C does not fix the order of argument evaluation (review 2,
        # 5.4): each pair is rejected in both textual orders, and so is a
        # temporary nested in another argument's call.
        "argument_order_collecting_first": ("""
int collect_int(void);
void consume(int, PyObject *);
Py_GC_PRECISE static void f(void) {
    consume(collect_int(), PyList_New(1));
}
""", "the result of 'PyList_New' (line 8) in argument 2 of 'consume' may "
     "be held across the collecting call to 'collect_int' in argument 1"),
        "argument_order_collecting_second": ("""
int collect_int(void);
void consume2(PyObject *, int);
Py_GC_PRECISE static void f(void) {
    consume2(PyList_New(1), collect_int());
}
""", "is a temporary held across the collecting call to 'collect_int'"),
        "argument_order_nested_first": ("""
int collect_int(void);
static inline int len_of(PyObject *o) { (void)o; return 0; }
Py_GC_PRECISE static void f(void) {
    g(len_of(PyList_New(1)), collect_int());
}
""", "the result of 'PyList_New' (line 8) in argument 1 of 'g' may be held "
     "across the collecting call to 'collect_int' in argument 2"),
        "argument_order_nested_second": ("""
int collect_int(void);
static inline int len_of(PyObject *o) { (void)o; return 0; }
Py_GC_PRECISE static void f(void) {
    g(collect_int(), len_of(PyList_New(1)));
}
""", "the result of 'PyList_New' (line 8) in argument 2 of 'g' may be held "
     "across the collecting call to 'collect_int' in argument 1"),
        # A foreach runs its caller's callback, which may collect (review
        # 2, 5.5): the _Py_hashtable family is listed by function.
        "hashtable_foreach_collects": ("""
static int visit(void *ht, const void *k, const void *v, void *arg) {
    (void)ht; (void)k; (void)v; (void)arg;
    return collect();
}
Py_GC_PRECISE static void f(void *ht) {
    PyObject *x = PyLong_FromLong(1);
    _Py_hashtable_foreach(ht, visit, NULL);
    use(x);
}
""", "'x' is held only in a C local across the collecting call to "
     "'_Py_hashtable_foreach'"),
        "alias_store": ("""
Py_GC_PRECISE static void f(Py_GC_SLOTS PyValue *slots) {
    PyValue *q = slots;
    PyValue x = slots[0];
    q[0] = PyValue_NULL;
    collect();
    use_v(x);
}
""", "'x' is held only in a C local across the collecting call to 'collect' (a slot of 'slots', overwritten)"),
        "field_other_base": ("""
Py_GC_PRECISE static void f(PyFunctionObject *a, PyFunctionObject *b, PyObject *g) {
    PyObject *globals = a->func_globals;
    b->func_globals = g;
    PyObject *n = PyLong_FromLong(1);
    use(globals);
    use(n);
}
""", "'globals' is held only in a C local across the collecting call to 'PyLong_FromLong' ('a->func_globals', overwritten)"),
        "memset_slots": ("""
Py_GC_PRECISE static void f(Py_GC_SLOTS PyValue *slots) {
    PyValue x = slots[0];
    memset(slots, 0, 4 * sizeof(PyValue));
    collect();
    use_v(x);
}
""", "'x' is held only in a C local across the collecting call to 'collect' (a slot of 'slots', overwritten)"),
        "span_index_store": ("""
Py_GC_PRECISE static void f(PyThreadState *tstate, PyValue *buf) {
    _PyValueRoot r;
    _PyThreadState_PushValueSpan(tstate, &r, buf);
    PyValue x = PyLong_FromLong(1);
    r.base[0] = x;
    collect();
    use_v(x);
    _PyThreadState_PopValueRoot(tstate, &r);
}
""", "'x' is held only in a C local across the collecting call to 'collect'"),
        "buffer_owner_dead": ("""
Py_GC_PRECISE static void f(void) {
    PyObject *s = PyUnicode_FromString("abc");
    const char *p = PyUnicode_AsUTF8(s);
    collect();
    use_p(p);
}
""", "'s' is held only in a C local across the collecting call to 'collect'"),
        "dict_next": ("""
Py_GC_PRECISE static void f(PyObject *d) {
    Py_ssize_t pos = 0;
    PyObject *k, *v;
    while (PyDict_Next(d, &pos, &k, &v)) {
        use(k);
    }
}
""", "the address of object local 'k' escapes to 'PyDict_Next'"),
        "cast_uintptr": ("""
Py_GC_PRECISE static void f(PyObject *o) {
    uintptr_t h = (uintptr_t)o;
    collect();
    use((PyObject *)h);
}
""", 'an object is cast to a type that is not an object, which hides it'),
        "args_anchored_address": ("""
Py_GC_PRECISE Py_GC_ARGS_ANCHORED static void callee(PyObject *o) {
    use(o);
}
Py_GC_PRECISE static void *f(void) {
    return (void *)callee;
}
""", "'callee' is Py_GC_ARGS_ANCHORED; its address may not be taken"),
        "detach_helper_precise": ("""
Py_GC_PRECISE void PyEval_SaveThread(void) {
}
""", "'PyEval_SaveThread' may return while its thread is detached and may not be marked precise"),
        "args_anchored_unanchored": ("""
Py_GC_PRECISE Py_GC_ARGS_ANCHORED static void callee(PyValue v) {
    collect();
    use_v(v);
}
Py_GC_PRECISE static void f(void) {
    PyValue x = PyLong_FromLong(1);
    callee(x);
}
""", "an argument of 'callee', which is Py_GC_ARGS_ANCHORED, is not anchored"),
        "args_anchored_from_imprecise": ("""
Py_GC_PRECISE Py_GC_ARGS_ANCHORED static void callee(PyValue v) {
    use_v(v);
}
static void g(Py_GC_SLOTS PyValue *slots) {
    callee(slots[0]);
}
""", "'callee' is Py_GC_ARGS_ANCHORED but called from 'g', which is not precise"),
        "args_anchored_not_static": ("""
Py_GC_PRECISE Py_GC_ARGS_ANCHORED void callee(PyValue v) {
    use_v(v);
}
""", "'callee' is Py_GC_ARGS_ANCHORED but not static"),
        "stack_arg_unanchored": ("""
Py_GC_PRECISE static void f(void) {
    PyValue x = PyLong_FromLong(1);
    collect7(0, 0, 0, 0, 0, 0, x);
}
""", "argument 7 of 'collect7', which is not precise, is passed on the stack and is not anchored"),
        "unsupported_setjmp": ("""
Py_GC_PRECISE static void f(void) {
    jmp_buf b;
    if (setjmp(b)) {
        return;
    }
}
""", "setjmp is outside the C mode's subset"),
        # Object out-parameters (10a-2b.md section 2): the accessor's slot
        # is the one checked form.  get_out() is unknown, so collecting.
        "object_slot_local_address": ("""
Py_GC_PRECISE static void f(PyThreadState *tstate, PyObject *o) {
    PyValue buf[3];
    _PyValueRoot r;
    _PyThreadState_PushValueSpan(tstate, &r, buf);
    _PyValueSpan_AppendObject(&r, o);
    PyObject *x;
    if (get_out(o, &x) > 0) {
        collect();
        use(x);
    }
    _PyThreadState_PopValueRoot(tstate, &r);
}
""", "the address of object local 'x' escapes to 'get_out'"),
        # The address kept in a pointer, as a control that stores the
        # receiver in a C local does: the load through it reads as fresh.
        "object_slot_kept_address": ("""
Py_GC_PRECISE static void f(PyThreadState *tstate, PyObject *o) {
    PyObject *x = NULL;
    PyObject **xp = &x;
    if (get_out(o, xp) > 0) {
        collect();
        use(*xp);
    }
}
""", "the address of object local 'x' is kept, outside a call's argument"),
        "object_slot_after_pop": ("""
Py_GC_PRECISE static void f(PyThreadState *tstate, PyObject *o) {
    PyValue buf[2];
    _PyValueRoot r;
    _PyThreadState_PushValueSpan(tstate, &r, buf);
    _PyValueSpan_AppendObject(&r, o);
    PyObject **xp = _PyValueSpan_ObjectSlot(&r);
    if (get_out(o, xp) <= 0) {
        _PyThreadState_PopValueRoot(tstate, &r);
        return;
    }
    PyObject *x = *xp;
    _PyThreadState_PopValueRoot(tstate, &r);
    collect();
    use(x);
}
""", "'x' is held only in a C local across the collecting call to 'collect' (span 'r', popped)"),
        "object_slot_reused": ("""
Py_GC_PRECISE static void f(PyThreadState *tstate, PyObject *o) {
    PyValue buf[3];
    _PyValueRoot r;
    _PyThreadState_PushValueSpan(tstate, &r, buf);
    _PyValueSpan_AppendObject(&r, o);
    PyObject **xp = _PyValueSpan_ObjectSlot(&r);
    if (get_out(o, xp) > 0) {
        PyObject *x = *xp;
        if (get_out(o, xp) > 0) {
            collect();
            use(x);
        }
    }
    _PyThreadState_PopValueRoot(tstate, &r);
}
""", "object slot 'xp', which 'get_out' may store into again"),
        "object_slot_cast": ("""
Py_GC_PRECISE static void f(PyThreadState *tstate, PyObject *o) {
    PyValue buf[3];
    _PyValueRoot r;
    _PyThreadState_PushValueSpan(tstate, &r, buf);
    _PyValueSpan_AppendObject(&r, o);
    buf[1] = PyValue_NULL;
    if (get_out(o, (PyObject **)&buf[1]) > 0) {
        collect();
    }
    _PyThreadState_PopValueRoot(tstate, &r);
}
""", "a slot of span-backed 'buf' cast to an object out-parameter: use _PyValueSpan_ObjectSlot()"),
        "object_slot_not_pushed": ("""
Py_GC_PRECISE static void f(PyThreadState *tstate, PyObject *o) {
    _PyValueRoot r;
    PyObject **xp = _PyValueSpan_ObjectSlot(&r);
    (void)get_out(o, xp);
}
""", "an object slot of span 'r', which is not pushed"),
    }

    POSITIVE = {
        # One argument makes a temporary and none of the others collects.
        "argument_order_one_collecting": """
static inline int len_of(PyObject *o) { (void)o; return 0; }
Py_GC_PRECISE static void f(void) {
    g(len_of(NULL), PyList_New(1));
    g(PyList_New(1), len_of(NULL));
}
""",
        # These members of the _Py_hashtable family call no callback.
        "hashtable_listed": """
Py_GC_PRECISE static void f(void *ht) {
    PyObject *x = PyLong_FromLong(1);
    size_t n = _Py_hashtable_len(ht) + _Py_hashtable_size(ht);
    use(x);
    (void)n;
}
""",
        "object_slot": """
Py_GC_PRECISE static void f(PyThreadState *tstate, PyObject *o) {
    PyValue buf[3];
    _PyValueRoot r;
    _PyThreadState_PushValueSpan(tstate, &r, buf);
    _PyValueSpan_AppendObject(&r, o);
    PyObject **xp = _PyValueSpan_ObjectSlot(&r);
    if (get_out(o, xp) > 0) {
        PyObject *x = *xp;
        collect();
        use(x);
    }
    _PyThreadState_PopValueRoot(tstate, &r);
}
""",
        # The slot is raw: a box stored there is anchored as itself.
        "object_slot_box": """
Py_GC_PRECISE static void f(PyThreadState *tstate, PyObject *o) {
    PyValue buf[3];
    _PyValueRoot r;
    _PyThreadState_PushValueSpan(tstate, &r, buf);
    _PyValueSpan_AppendObject(&r, o);
    PyObject **xp = _PyValueSpan_ObjectSlot(&r);
    *xp = PyFloat_FromDouble(1.5);
    PyObject *x = *xp;
    collect();
    use(x);
    _PyThreadState_PopValueRoot(tstate, &r);
}
""",
        # An array of objects to read is not an out-parameter.
        "object_array_cast": """
Py_GC_PRECISE static void f(PyThreadState *tstate, PyObject *o) {
    PyValue buf[3];
    _PyValueRoot r;
    _PyThreadState_PushValueSpan(tstate, &r, buf);
    _PyValueSpan_AppendObject(&r, o);
    _PyValueSpan_AppendObject(&r, o);
    collect_arr((PyObject *const *)buf, 2);
    use(o);
    _PyThreadState_PopValueRoot(tstate, &r);
}
""",
        "value_in_span": """
Py_GC_PRECISE static void f(PyThreadState *tstate) {
    _PyValueRoot r;
    _PyThreadState_PushValueRoot(tstate, &r);
    PyObject *x = PyLong_FromLong(1);
    r.object = x;
    collect();
    use(x);
    _PyThreadState_PopValueRoot(tstate, &r);
}
""",
        "slot_load": """
Py_GC_PRECISE static void f(Py_GC_SLOTS PyValue *slots) {
    PyValue x = slots[0];
    collect();
    use_v(x);
}
""",
        "tuple_item": """
Py_GC_PRECISE static void f(PyThreadState *tstate, PyTupleObject *t) {
    _PyValueRoot r;
    _PyThreadState_PushValueRoot(tstate, &r);
    r.object = (PyObject *)t;
    PyValue v = PyTuple_GET_VALUE(t, 0);
    collect();
    use_v(v);
    _PyThreadState_PopValueRoot(tstate, &r);
}
""",
        "strong_field_alloc": """
Py_GC_PRECISE static void f(PyThreadState *tstate, PyFunctionObject *func) {
    _PyValueRoot r;
    _PyThreadState_PushValueRoot(tstate, &r);
    r.object = (PyObject *)func;
    PyObject *globals = func->func_globals;
    PyObject *n = PyLong_FromLong(1);
    use(globals);
    use(n);
    _PyThreadState_PopValueRoot(tstate, &r);
}
""",
        "args_anchored": """
Py_GC_PRECISE Py_GC_ARGS_ANCHORED static void callee(PyValue v) {
    collect();
    use_v(v);
}
Py_GC_PRECISE static void f(Py_GC_SLOTS PyValue *slots) {
    callee(slots[0]);
}
""",
        "param_in_span": """
Py_GC_PRECISE static void f(PyThreadState *tstate, PyObject *p) {
    _PyValueRoot r;
    _PyThreadState_PushValueRoot(tstate, &r);
    r.object = p;
    collect();
    use(p);
    _PyThreadState_PopValueRoot(tstate, &r);
}
""",
        "goto_cleanup": """
Py_GC_PRECISE static int f(PyThreadState *tstate) {
    int res = -1;
    _PyValueRoot r;
    _PyThreadState_PushValueRoot(tstate, &r);
    PyObject *x = PyLong_FromLong(1);
    if (x == NULL) {
        goto done;
    }
    r.object = x;
    if (collect() < 0) {
        goto done;
    }
    use(x);
    res = 0;
done:
    _PyThreadState_PopValueRoot(tstate, &r);
    return res;
}
""",
        "buffer_owner_in_span": """
Py_GC_PRECISE static void f(PyThreadState *tstate) {
    _PyValueRoot r;
    _PyThreadState_PushValueRoot(tstate, &r);
    PyObject *s = PyUnicode_FromString("abc");
    r.object = s;
    const char *p = PyUnicode_AsUTF8(s);
    collect();
    use_p(p);
    _PyThreadState_PopValueRoot(tstate, &r);
}
""",
        "span_append": """
Py_GC_PRECISE static void f(PyThreadState *tstate, PyValue *buf) {
    _PyValueRoot r;
    _PyThreadState_PushValueSpan(tstate, &r, buf);
    PyValue x = PyLong_FromLong(1);
    _PyValueSpan_Append(&r, x);
    collect();
    use_v(x);
    _PyThreadState_PopValueRoot(tstate, &r);
}
""",
        "dict_next_value": """
Py_GC_PRECISE static void f(PyThreadState *tstate, PyObject *d) {
    _PyValueRoot r;
    _PyThreadState_PushValueRoot(tstate, &r);
    r.object = d;
    Py_ssize_t pos = 0;
    PyValue k, v;
    while (_PyDict_NextValue(d, &pos, &k, &v)) {
        PyObject *n = PyLong_FromLong(1);
        use_v(k);
        use_v(v);
        use(n);
    }
    _PyThreadState_PopValueRoot(tstate, &r);
}
""",
        "stack_arg_in_span": """
Py_GC_PRECISE static void f(PyThreadState *tstate) {
    _PyValueRoot r;
    _PyThreadState_PushValueRoot(tstate, &r);
    PyValue x = PyLong_FromLong(1);
    r.value = x;
    collect7(0, 0, 0, 0, 0, x, x);
    _PyThreadState_PopValueRoot(tstate, &r);
}
""",
        "slot_offset": """
Py_GC_PRECISE static void f(Py_GC_SLOTS PyValue *slots) {
    PyValue *args = slots + 1;
    PyValue x = args[0];
    collect();
    use_v(x);
}
""",
        # x is an argument of the call, which anchors its parameters; its
        # old value is dead once the call returns.
        "reassign_from_call": """
Py_GC_PRECISE static void f(void) {
    PyObject *x = PyLong_FromLong(1);
    x = collect_o(x);
    use(x);
}
""",
        "copy_shares_anchor": """
Py_GC_PRECISE static void f(PyThreadState *tstate, PyObject *p) {
    PyValue buf[1];
    _PyValueRoot r;
    _PyThreadState_PushValueSpan(tstate, &r, buf);
    _PyValueSpan_AppendObject(&r, p);
    PyObject *q = p;
    collect();
    use(q);
    _PyThreadState_PopValueRoot(tstate, &r);
}
""",
        # Anchored by a different store on each path
        "anchored_on_each_path": """
Py_GC_PRECISE static void f(PyThreadState *tstate, PyObject *p, int c) {
    PyValue buf[2];
    _PyValueRoot r;
    _PyThreadState_PushValueSpan(tstate, &r, buf);
    _PyValueSpan_AppendObject(&r, p);
    PyObject *q;
    if (c) {
        q = p;
    }
    else {
        q = PyLong_FromLong(1);
        _PyValueSpan_AppendObject(&r, q);
    }
    collect();
    use(q);
    _PyThreadState_PopValueRoot(tstate, &r);
}
""",
        # A copy of a received array holds what the caller's array holds,
        # which the array's creator anchors (10a-2.md, rule 3).
        "copy_of_received_array": """
Py_GC_PRECISE static void f(PyObject *const *args, Py_ssize_t n) {
    PyValue small[4];
    PyValue *values = values_of(args, n, small, Py_ARRAY_LENGTH(small));
    collect();
    use_v(values[0]);
    use_v(small[0]);
}
""",
        # The eval loop's generated handlers are one region, entered by a
        # goto to a handler's label, which falls through or returns
        "generated_region": """
Py_GC_PRECISE static int f(PyThreadState *tstate, int c) {
    if (c) {
        goto early_exit;
    }
    goto start_frame;
#include "generated_ir_cases.c.h"
early_exit:
    collect();
    return -1;
}
""",
        # A value asserted inline holds no object, in release builds too
        "asserted_immediate": """
Py_GC_PRECISE static void f(PyValue v) {
    assert(PyValue_IsInline(v));
    collect();
    use_v(v);
}
""",
        # Argument Clinic's parsers keep their keyword tables in static
        # locals, which hold no object of the frame's.
        "static_locals": """
Py_GC_PRECISE static void f(PyObject *p) {
    static const char * const keywords[] = {"a", NULL};
    static struct {
        int n;
        PyObject *ob_item[1];
    } table = {
        .n = 1,
        .ob_item = { NULL, },
    };
    collect();
    use_p(keywords[0]);
}
""",
        # _PyArg_UnpackKeywords() with no dict returns the received array
        # or a copy of it in buf: either holds what the caller anchors.
        "unpacked_keywords": """
Py_GC_PRECISE static void f(const PyValue *args, Py_ssize_t nargs,
                            PyObject *kwnames, _PyArg_Parser *parser) {
    PyValue buf[1];
    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, parser,
                                 0, 1, 0, 0, buf);
    if (!args) {
        return;
    }
    collect();
    use_v(args[0]);
}
""",
    }

    NEGATIVE_MORE = {
        # Boxing allocates: the second box may collect the first
        "box_collects_earlier_box": ("""
Py_GC_PRECISE static void f(PyValue x, PyValue y) {
    PyObject *a = PyValue_Object(x);
    PyObject *b = PyValue_Object(y);
    use(a);
    use(b);
}
""", "'a' is held only in a C local across the collecting call to 'PyValue_Object'"),
        # A type's cache is not traced: a load from it is no borrow, even
        # across an allocation
        "cache_is_not_a_borrow": ("""
Py_GC_PRECISE static void f(PyHeapTypeObject *cls) {
    PyObject *init = cls->_spec_cache.init;
    PyObject *o = PyType_GenericAlloc((PyTypeObject *)cls, 0);
    use(init);
    use(o);
}
""", "'init' is held only in a C local across the collecting call to 'PyType_GenericAlloc'"),
        # The region collects
        "held_across_generated_region": ("""
Py_GC_PRECISE static int f(void) {
    PyObject *x = PyLong_FromLong(1);
    goto start_frame;
#include "generated_ir_cases.c.h"
    use(x);
    return 0;
}
""", "'x' is held only in a C local across the collecting call to '_PyEval_GeneratedHandlers'"),
        # Only an inline test makes a value an immediate
        "asserted_object": ("""
Py_GC_PRECISE static void f(PyValue v) {
    assert(PyValue_IsObject(v));
    collect();
    use_v(v);
}
""", "'v' is held only in a C local across the collecting call to 'collect'"),
        # With a dict, the copy holds the dict's values, which no caller
        # anchors.
        "unpacked_keywords_dict": ("""
Py_GC_PRECISE static void f(const PyValue *args, Py_ssize_t nargs,
                            PyObject *kwargs, _PyArg_Parser *parser) {
    PyValue buf[1];
    args = _PyArg_UnpackKeywords(args, nargs, kwargs, NULL, parser,
                                 0, 1, 0, 0, buf);
    if (!args) {
        return;
    }
    collect();
    use_v(args[0]);
}
""", "'buf' is held only in a C local across the collecting call to 'collect'"),
        "copy_after_pop": ("""
Py_GC_PRECISE static void f(PyThreadState *tstate, PyObject *p) {
    PyValue buf[1];
    _PyValueRoot r;
    _PyThreadState_PushValueSpan(tstate, &r, buf);
    _PyValueSpan_AppendObject(&r, p);
    PyObject *q = p;
    _PyThreadState_PopValueRoot(tstate, &r);
    collect();
    use(q);
}
""", "'q' is held only in a C local across the collecting call to 'collect'"),
        "anchored_on_one_path": ("""
Py_GC_PRECISE static void f(PyThreadState *tstate, PyObject *p, int c) {
    PyValue buf[1];
    _PyValueRoot r;
    _PyThreadState_PushValueSpan(tstate, &r, buf);
    _PyValueSpan_AppendObject(&r, p);
    PyObject *q;
    if (c) {
        q = p;
    }
    else {
        q = PyLong_FromLong(1);
    }
    collect();
    use(q);
    _PyThreadState_PopValueRoot(tstate, &r);
}
""", "'q' is held only in a C local across the collecting call to 'collect'"),
        "used_after_reassign_from_call": ("""
Py_GC_PRECISE static void f(void) {
    PyObject *x = PyLong_FromLong(1);
    PyObject *y = PyLong_FromLong(2);
    y = collect_o(x);
    use(x);
    use(y);
}
""", "'x' is held only in a C local across the collecting call to 'collect_o'"),
        "copy_of_a_local_array": ("""
Py_GC_PRECISE static void f(PyObject *const *args, Py_ssize_t n) {
    PyValue small[4];
    PyValue *values = copy_values(args, n, small, Py_ARRAY_LENGTH(small));
    collect();
    use_v(values[0]);
    use_v(small[0]);
}
""", "'small' is held only in a C local across the collecting call to 'collect'"),
    }

    # 10a-2.md, rule 4: the slot before a received argument array holds
    # its creator's value, which the slot alone anchors.
    LENT_SLOT = """
Py_GC_PRECISE static PyValue f(PyThreadState *tstate, PyValue arg,
                               const PyValue *args, size_t n) {
    PyValue *shifted = (PyValue *)args - 1;
    PyValue saved[2];
    _PyValueRoot r;
    _PyThreadState_PushValueSpan(tstate, &r, saved);
    _PyValueSpan_Append(&r, shifted[0]);
    _PyValueSpan_Append(&r, arg);
    shifted[0] = arg;
    PyValue res = call_v(shifted, n + 1);
    shifted[0] = saved[0];
    _PyThreadState_PopValueRoot(tstate, &r);
    return res;
}
"""

    def test_lent_slot(self):
        want = "a slot that PY_VECTORCALL_ARGUMENTS_OFFSET lends"
        self.assertEqual(self.check(self.LENT_SLOT), [])
        # Saved through a local that read the slot, and the object form
        through_local = self.LENT_SLOT.replace(
            "_PyValueSpan_Append(&r, shifted[0]);",
            "PyValue tmp = shifted[0];\n    _PyValueSpan_Append(&r, tmp);")
        self.assertEqual(self.check(through_local), [])
        unsaved = self.LENT_SLOT.replace(
            "_PyValueSpan_Append(&r, shifted[0]);", "")
        self.assertTrue(any(want in e for e in self.check(unsaved)))
        restored_after_pop = self.LENT_SLOT.replace("""    shifted[0] = saved[0];
    _PyThreadState_PopValueRoot(tstate, &r);""", """    _PyThreadState_PopValueRoot(tstate, &r);
    shifted[0] = saved[0];""")
        self.assertTrue(any(want in e for e in self.check(restored_after_pop)))
        direct = """
Py_GC_PRECISE static void f(PyValue arg, PyValue *args) {
    args[-1] = arg;
}
"""
        self.assertTrue(any(want in e for e in self.check(direct)))

    PUSH_FRAME = """
static inline void _PyFrame_Initialize(PyThreadState *tstate,
    _PyInterpreterFrame *frame, PyValue func, PyObject *locals,
    PyCodeObject *code, PyValue *base, int n, _PyInterpreterFrame *previous)
{
    frame->base = base;
}
Py_GC_PRECISE static int f(PyThreadState *tstate) {
    _PyInterpreterFrame *frame = _PyThreadState_PushFrame(tstate, 16);
    if (frame == NULL) {
        goto fail;
    }
    _PyFrame_Initialize(tstate, frame, PyValue_NULL, NULL, NULL, frame->base,
                        0, NULL);
    collect();
    return 0;
fail:
    collect();
    return -1;
}
"""

    def test_push_before_initialize(self):
        # Nothing between a frame's push and its initialize may collect; a
        # failed push pushed nothing.
        want = "comes between the push of frame 'frame'"
        self.assertEqual(self.check(self.PUSH_FRAME), [])
        between = self.PUSH_FRAME.replace("""        goto fail;
    }
""", """        goto fail;
    }
    collect();
""")
        self.assertTrue(any(want in e for e in self.check(between)))
        # On one path only
        one_path = self.PUSH_FRAME.replace("""        goto fail;
    }
""", """        goto fail;
    }
    if (tstate->current_frame == NULL) {
        collect();
    }
""")
        self.assertTrue(any(want in e for e in self.check(one_path)))

    def test_negative_more(self):
        for name, (src, want) in self.NEGATIVE_MORE.items():
            with self.subTest(name):
                errors = self.check(src)
                self.assertTrue(any(want in e for e in errors),
                                f"{want!r} not in {errors}")

    def test_alias_macro_is_its_target(self):
        toks = root_check_c.tokens_of(
            "#define Alias_IsCompact _Target_IsCompact\n")
        bodies = root_check_c.macro_bodies(toks)
        bodies["_Target_IsCompact"] = root_check_c.tokens_of("{ return 1; }")
        self.assertEqual(root_check_c.Classifier(bodies).kind("Alias_IsCompact"),
                         "none")
        bodies["_Target_IsCompact"] = root_check_c.tokens_of(
            "{ return collect(); }")
        self.assertEqual(root_check_c.Classifier(bodies).kind("Alias_IsCompact"),
                         "reentrant")

    def test_instr_oparg_decoder_effect(self):
        bodies = {
            "decode": root_check_c.tokens_of(
                "{ return _PyCode_InstrOparg(code, index); }"),
            "unknown": root_check_c.tokens_of(
                "{ return _PyCode_InstrOpargUnknown(code, index); }"),
            "collecting": root_check_c.tokens_of("""{
                int oparg = _PyCode_InstrOparg(code, index);
                collect();
                return oparg;
            }"""),
        }
        classifier = root_check_c.Classifier(bodies)
        self.assertEqual(classifier.kind("decode"), "none")
        self.assertEqual(classifier.kind("unknown"), "reentrant")
        self.assertEqual(classifier.kind("collecting"), "reentrant")

    def check(self, src):
        return [str(e) for e in root_check_c.check_source(
            self.PRELUDE + src,
            include_dirs=root_check_c.default_include_dirs())]

    def test_negative(self):
        for name, (src, want) in self.NEGATIVE.items():
            with self.subTest(name):
                errors = self.check(src)
                self.assertTrue(any(want in e for e in errors),
                                f"{want!r} not in {errors}")

    def test_positive(self):
        for name, src in self.POSITIVE.items():
            with self.subTest(name):
                self.assertEqual(self.check(src), [])

    # A checker defect, for 10a rung 3 (10a-2b.md, hand-offs): an object
    # local declared in one branch is reported as held across a call in
    # the other when the calls' results go to an _PyInterpreterFrame *.
    # Found by item 3's _PyEvalFramePushAndInit_Ex(), which declares its
    # names at function scope instead.
    BLOCK_LOCAL_FRAME_RESULT = """
Py_GC_PRECISE static _PyInterpreterFrame *f(PyThreadState *tstate,
                                            PyObject *o, int c) {
    PyValue buf[2];
    _PyValueRoot r;
    _PyThreadState_PushValueSpan(tstate, &r, buf);
    _PyValueSpan_AppendObject(&r, o);
    _PyInterpreterFrame *res = NULL;
    if (c) {
        res = push(o, NULL);
    }
    else {
        PyObject *n = make(o);
        if (n != NULL) {
            _PyValueSpan_AppendObject(&r, n);
            res = push(o, n);
        }
    }
    _PyThreadState_PopValueRoot(tstate, &r);
    return res;
}
"""

    @unittest.expectedFailure
    def test_block_local_beside_frame_result(self):
        self.assertEqual(self.check(self.BLOCK_LOCAL_FRAME_RESULT), [])

    def test_unmarked_not_checked(self):
        src = self.NEGATIVE["no_span"][0].replace("Py_GC_PRECISE ", "")
        self.assertEqual(self.check(src), [])

    def test_cli(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "unchecked.c")
            with open(path, "w") as f:
                f.write(self.PRELUDE + self.NEGATIVE["no_span"][0])
            out = io.StringIO()
            with contextlib.redirect_stdout(out):
                status = root_check_c.main([path])
            self.assertEqual(status, 1)
            self.assertIn(f"{path}:", out.getvalue())
            self.assertIn("is held only in a C local", out.getvalue())
            with open(path, "w") as f:
                f.write(self.PRELUDE + self.POSITIVE["value_in_span"])
            with contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(root_check_c.main([path]), 0)

    def test_marked_header_definition_is_checked(self):
        # The build's check runs over internal headers too (Makefile.pre.in,
        # Python/gc_precise.stamp); a marked inline helper there is checked
        # as a marked function of a .c file is.
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "pycore_helper.h")
            with open(path, "w") as f:
                f.write("#ifndef HELPER_H\n#define HELPER_H\n" + self.PRELUDE
                        + self.NEGATIVE["no_span"][0].replace(
                            "static void", "static inline void")
                        + "#endif\n")
            with contextlib.redirect_stdout(io.StringIO()) as out:
                self.assertEqual(root_check_c.main([path]), 1)
            self.assertIn("is held only in a C local", out.getvalue())

    def test_canonical_append_does_not_anchor_a_box(self):
        # The canonical value of an exact float box is an immediate: the
        # span holds the value and the box is free to go (10a-2.md, 0d).
        src = """
Py_GC_PRECISE static void f(PyThreadState *tstate, PyObject *x) {
    PyValue buf[1];
    _PyValueRoot r;
    _PyThreadState_PushValueSpan(tstate, &r, buf);
    _PyValueSpan_Append(&r, PyValue_FromObject(x));
    collect();
    use(x);
    _PyThreadState_PopValueRoot(tstate, &r);
}
"""
        errors = self.check(src)
        self.assertTrue(any("'x' is held only in a C local" in e
                            for e in errors), errors)
        self.assertEqual(self.check(src.replace(
            "_PyValueSpan_Append(&r, PyValue_FromObject(x))",
            "_PyValueSpan_AppendObject(&r, x)")), [])

    def test_out_parameter_into_span(self):
        # A callee's result stored through a pointer into a pushed span's
        # buffer is anchored there; through a local's address it escapes
        # (10a-2.md, 0e).
        src = """
Py_GC_PRECISE static void f(PyThreadState *tstate, PyObject *g) {
    PyValue buf[1];
    _PyValueRoot r;
    _PyThreadState_PushValueSpan(tstate, &r, buf);
    _PyValueSpan_Append(&r, PyValue_NULL);
    send(g, &buf[0]);
    collect();
    use_v(buf[0]);
    _PyThreadState_PopValueRoot(tstate, &r);
}
"""
        self.assertEqual(self.check(src), [])
        after_pop = src.replace("""    use_v(buf[0]);
    _PyThreadState_PopValueRoot(tstate, &r);""", """    _PyThreadState_PopValueRoot(tstate, &r);
    collect();
    use_v(buf[0]);""")
        self.assertTrue(any("'buf' is held only in a C local" in e
                            for e in self.check(after_pop)))
        local = """
Py_GC_PRECISE static void f(PyThreadState *tstate, PyObject *g) {
    PyValue res;
    send(g, &res);
    collect();
    use_v(res);
}
"""
        self.assertTrue(any("the address of object local 'res' escapes" in e
                            for e in self.check(local)))

    def test_variadic_cannot_be_precise(self):
        src = """
Py_GC_PRECISE static int f(int n, ...) {
    va_list ap;
    va_start(ap, n);
    va_end(ap);
    return n;
}
"""
        errors = self.check(src)
        self.assertEqual(len(errors), 1, errors)
        self.assertIn("'f' is variadic", errors[0])

    def test_mem_domain_does_not_collect(self):
        # A value held across PyMem_Malloc() needs no anchor; one held
        # across PyObject_Malloc() does.
        src = """
Py_GC_PRECISE static void f(void) {
    PyObject *x = PyLong_FromLong(1);
    void *p = PyMem_Malloc(8);
    use(x);
    PyMem_Free(p);
    use(x);
}
"""
        self.assertEqual(self.check(src), [])
        errors = self.check(src.replace("PyMem_Malloc", "PyObject_Malloc"))
        self.assertTrue(any("'x' is held only in a C local" in e
                            for e in errors), errors)

    def test_unknown_function_fails(self):
        errors = root_check_c.check_source(self.PRELUDE, function="missing")
        self.assertEqual(len(errors), 1)
        self.assertIn("no function 'missing'", str(errors[0]))

    def test_unparsed_precise_function_fails(self):
        # Braces that balance only across #if branches lose the function
        # to the parser; a marked one fails instead of going unchecked.
        src = """
Py_GC_PRECISE static int f(int n) {
#if A
    if (n < 0) {
        n = -n;
#else
    if (n > 0) {
        n = 0;
#endif
    }
    return n;
}
"""
        errors = self.check(src)
        self.assertEqual(len(errors), 1, errors)
        self.assertIn("'f' is Py_GC_PRECISE but outside the C mode's subset",
                      errors[0])

    def test_cast_to_local_typedef(self):
        # A function pointer typedef of the file, and a struct typedef of
        # the headers, are types to the parser.
        src = """
typedef PyObject *(*getter_fn)(PyObject *);
static inline getter_fn get_getter(PyObject *o) { (void)o; return NULL; }
Py_GC_PRECISE static int f(PyObject *o) {
    getter_fn g = (getter_fn)
        get_getter(o);
    PyThreadState *ts = (PyThreadState *)
        get_getter(o);
    (void)g;
    (void)ts;
    return 0;
}
"""
        self.assertEqual(self.check(src), [])
        self.assertIn("getter_fn",
                      root_check_c.typedef_names(root_check_c.tokens_of(src)))

    def test_windows_scalar_cast(self):
        # Windows SDK typedefs are unavailable to a Unix header scan, but
        # the C mode must still check the complete inactive branch.
        src = """
Py_GC_PRECISE static DWORD f(PyTime_t div) {
    DWORD millis = 0;
#if defined(MS_WINDOWS)
    millis = (DWORD) div;
#else
    millis = 0;
#endif
    return millis;
}
"""
        self.assertEqual(self.check(src), [])
        missing_root = src.replace("millis = (DWORD) div;", """
    millis = (DWORD) div;
    PyObject *x = PyLong_FromLong(1);
    collect();
    use(x);
""")
        errors = self.check(missing_root)
        self.assertEqual(len(errors), 1, errors)
        self.assertIn("'x' is held only in a C local across the collecting "
                      "call to 'collect'", errors[0])

    def test_call_through_pointer_collects(self):
        bodies = {
            "direct": root_check_c.tokens_of("{ return fp(x); }"),
            "deref": root_check_c.tokens_of("{ return (*fp)(x); }"),
            "field": root_check_c.tokens_of("{ return o->call(x); }"),
            "cast": root_check_c.tokens_of("{ return (int)(x); }"),
        }
        cls = root_check_c.Classifier(bodies)
        self.assertEqual(cls.kind("direct"), "reentrant")
        self.assertEqual(cls.kind("deref"), "reentrant")
        self.assertEqual(cls.kind("field"), "reentrant")
        self.assertEqual(cls.kind("cast"), "none")

    def test_char_literals_and_splices(self):
        src = root_check_c.prepare('x = \'"\'; s = "it\'s \\\nfine";')
        self.assertEqual(src.count("\n"), 1)
        self.assertIn('"it\'s fine"', src)
        self.assertTrue(src.startswith("x = 0"))


if __name__ == "__main__":
    unittest.main()
