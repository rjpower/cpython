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
    import tier1_generator


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


class TestGeneratedCases(unittest.TestCase):
    def setUp(self) -> None:
        super().setUp()
        self.maxDiff = None

        self.temp_dir = tempfile.gettempdir()
        self.temp_input_filename = os.path.join(self.temp_dir, "input.txt")
        self.temp_output_filename = os.path.join(self.temp_dir, "output.txt")
        self.temp_metadata_filename = os.path.join(self.temp_dir, "metadata.txt")
        self.temp_pymetadata_filename = os.path.join(self.temp_dir, "pymetadata.txt")

    def tearDown(self) -> None:
        for filename in [
            self.temp_input_filename,
            self.temp_output_filename,
            self.temp_metadata_filename,
            self.temp_pymetadata_filename,
        ]:
            try:
                os.remove(filename)
            except:
                pass
        super().tearDown()

    def run_cases_test(self, input: str, expected: str):
        with open(self.temp_input_filename, "w+") as temp_input:
            temp_input.write(parser.BEGIN_MARKER)
            temp_input.write(input)
            temp_input.write(parser.END_MARKER)
            temp_input.flush()

        with handle_stderr():
            tier1_generator.generate_tier1_from_files(
                [self.temp_input_filename], self.temp_output_filename, False
            )

        with open(self.temp_output_filename) as temp_output:
            lines = temp_output.read()
            _, rest = lines.split(tier1_generator.INSTRUCTION_START_MARKER)
            instructions, labels_with_prelude_and_postlude = rest.split(tier1_generator.INSTRUCTION_END_MARKER)
            _, labels_with_postlude = labels_with_prelude_and_postlude.split(tier1_generator.LABEL_START_MARKER)
            labels, _ = labels_with_postlude.split(tier1_generator.LABEL_END_MARKER)
            actual = instructions.strip() + "\n\n        " + labels.strip()

        self.assertEqual(actual.strip(), expected.strip())

    def test_inst_no_args(self):
        input = """
        inst(OP, (--)) {
            SPAM();
        }
    """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            SPAM();
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_inst_one_pop(self):
        input = """
        inst(OP, (value --)) {
            SPAM(value);
            DEAD(value);
        }
    """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef value;
            value = stack_pointer[-1];
            SPAM(value);
            stack_pointer += -1;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_inst_one_push(self):
        input = """
        inst(OP, (-- res)) {
            res = SPAM();
        }
    """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef res;
            res = SPAM();
            stack_pointer[0] = res;
            stack_pointer += 1;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_inst_one_push_one_pop(self):
        input = """
        inst(OP, (value -- res)) {
            res = SPAM(value);
            DEAD(value);
        }
    """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef value;
            _PyStackRef res;
            value = stack_pointer[-1];
            res = SPAM(value);
            stack_pointer[-1] = res;
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_binary_op(self):
        input = """
        inst(OP, (left, right -- res)) {
            res = SPAM(left, right);
            INPUTS_DEAD();

        }
    """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef left;
            _PyStackRef right;
            _PyStackRef res;
            right = stack_pointer[-1];
            left = stack_pointer[-2];
            res = SPAM(left, right);
            stack_pointer[-2] = res;
            stack_pointer += -1;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_overlap(self):
        input = """
        inst(OP, (left, right -- left, result)) {
            result = SPAM(left, right);
            INPUTS_DEAD();
        }
    """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef left;
            _PyStackRef right;
            _PyStackRef result;
            right = stack_pointer[-1];
            left = stack_pointer[-2];
            result = SPAM(left, right);
            stack_pointer[-1] = result;
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_predictions(self):
        input = """
        inst(OP1, (arg -- res)) {
            DEAD(arg);
            res = Py_None;
        }
        inst(OP3, (arg -- res)) {
            DEAD(arg);
            DEOPT_IF(xxx);
            res = Py_None;
        }
        family(OP1, INLINE_CACHE_ENTRIES_OP1) = { OP3 };
    """
        output = """
        TARGET(OP1) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP1;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP1);
            PREDICTED_OP1:;
            _PyStackRef arg;
            _PyStackRef res;
            arg = stack_pointer[-1];
            res = Py_None;
            stack_pointer[-1] = res;
            DISPATCH();
        }

        TARGET(OP3) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP3;
            (void)(opcode);
            #endif
            _Py_CODEUNIT* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP3);
            static_assert(INLINE_CACHE_ENTRIES_OP1 == 0, "incorrect cache size");
            _PyStackRef arg;
            _PyStackRef res;
            arg = stack_pointer[-1];
            if (xxx) {
                UPDATE_MISS_STATS(OP1);
                assert(_PyOpcode_Deopt[opcode] == (OP1));
                JUMP_TO_PREDICTED(OP1);
            }
            res = Py_None;
            stack_pointer[-1] = res;
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_sync_sp(self):
        input = """
        inst(A, (arg -- res)) {
            DEAD(arg);
            SYNC_SP();
            escaping_call();
            res = Py_None;
        }
        inst(B, (arg -- res)) {
            DEAD(arg);
            res = Py_None;
            SYNC_SP();
            escaping_call();
        }
    """
        output = """
        TARGET(A) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = A;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(A);
            _PyStackRef arg;
            _PyStackRef res;
            arg = stack_pointer[-1];
            stack_pointer += -1;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            _PyFrame_SetStackPointer(frame, stack_pointer);
            _PyFrame_StackPointerValidate(frame);
            escaping_call();
            _PyFrame_StackPointerInvalidate(frame);
            res = Py_None;
            stack_pointer[0] = res;
            stack_pointer += 1;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            DISPATCH();
        }

        TARGET(B) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = B;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(B);
            _PyStackRef arg;
            _PyStackRef res;
            arg = stack_pointer[-1];
            res = Py_None;
            stack_pointer[-1] = res;
            _PyFrame_SetStackPointer(frame, stack_pointer);
            _PyFrame_StackPointerValidate(frame);
            escaping_call();
            _PyFrame_StackPointerInvalidate(frame);
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)


    def test_pep7_condition(self):
        input = """
        inst(OP, (arg1 -- out)) {
            if (arg1)
                out = 0;
            else {
                out = 1;
            }
        }
        """
        output = ""
        with self.assertRaises(SyntaxError):
            self.run_cases_test(input, output)

    def test_error_if_plain(self):
        input = """
        inst(OP, (--)) {
            ERROR_IF(cond);
        }
    """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            if (cond) {
                JUMP_TO_LABEL(error);
            }
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_error_if_plain_with_comment(self):
        input = """
        inst(OP, (--)) {
            ERROR_IF(cond);  // Comment is ok
        }
    """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            if (cond) {
                JUMP_TO_LABEL(error);
            }
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_error_if_pop(self):
        input = """
        inst(OP, (left, right -- res)) {
            SPAM(left, right);
            INPUTS_DEAD();
            ERROR_IF(cond);
            res = 0;
        }
    """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef left;
            _PyStackRef right;
            _PyStackRef res;
            right = stack_pointer[-1];
            left = stack_pointer[-2];
            SPAM(left, right);
            if (cond) {
                JUMP_TO_LABEL(pop_2_error);
            }
            res = 0;
            stack_pointer[-2] = res;
            stack_pointer += -1;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_error_if_pop_with_result(self):
        input = """
        inst(OP, (left, right -- res)) {
            res = SPAM(left, right);
            INPUTS_DEAD();
            ERROR_IF(cond);
        }
    """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef left;
            _PyStackRef right;
            _PyStackRef res;
            right = stack_pointer[-1];
            left = stack_pointer[-2];
            res = SPAM(left, right);
            if (cond) {
                JUMP_TO_LABEL(pop_2_error);
            }
            stack_pointer[-2] = res;
            stack_pointer += -1;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_cache_effect(self):
        # Named cache effects read fields of the feedback entry
        input = """
        inst(OP, (index/1, extra/2, descr/4, value --)) {
            DEAD(value);
        }
    """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            _Py_CODEUNIT* const this_instr = next_instr;
            (void)this_instr;
            _PyFb_OP *fb = _PyFb_Entry(this_instr);
            frame->instr_ptr = next_instr;
            next_instr += 8;
            INSTRUCTION_STATS(OP);
            _PyStackRef value;
            value = stack_pointer[-1];
            uint32_t index = fb->index;
            (void)index;
            uint32_t extra = fb->extra;
            (void)extra;
            PyObject *descr = (PyObject *)fb->descr;
            (void)descr;
            stack_pointer += -1;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_suppress_dispatch(self):
        input = """
        label(somewhere) {
        }

        inst(OP, (--)) {
            goto somewhere;
        }
    """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            JUMP_TO_LABEL(somewhere);
        }

        LABEL(somewhere)
        {
        }
    """
        self.run_cases_test(input, output)

    def test_macro_instruction(self):
        input = """
        inst(OP1, (index/1, left, right -- left, right)) {
            op1(left, right);
        }
        op(OP2, (extra/2, arg2, left, right -- res)) {
            res = op2(arg2, left, right);
            INPUTS_DEAD();
        }
        macro(OP) = OP1 + cache/2 + OP2;
        inst(OP3, (unused/5, arg2, left, right -- res)) {
            res = op3(arg2, left, right);
            INPUTS_DEAD();
        }
        family(OP, INLINE_CACHE_ENTRIES_OP) = { OP3 };
    """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 6;
            INSTRUCTION_STATS(OP);
            PREDICTED_OP:;
            _Py_CODEUNIT* const this_instr = next_instr - 6;
            (void)this_instr;
            _PyFb_OP *fb = _PyFb_Entry(this_instr);
            _PyStackRef left;
            _PyStackRef right;
            _PyStackRef arg2;
            _PyStackRef res;
            // _OP1
            {
                right = stack_pointer[-1];
                left = stack_pointer[-2];
                uint32_t index = fb->index;
                (void)index;
                _PyFrame_SetStackPointer(frame, stack_pointer);
                _PyFrame_StackPointerValidate(frame);
                op1(left, right);
                _PyFrame_StackPointerInvalidate(frame);
            }
            /* Skip 2 cache entries */
            // OP2
            {
                arg2 = stack_pointer[-3];
                uint32_t extra = fb->extra;
                (void)extra;
                assert(stack_pointer == _PyFrame_GetStackPointer(frame));
                _PyFrame_StackPointerValidate(frame);
                res = op2(arg2, left, right);
                _PyFrame_StackPointerInvalidate(frame);
            }
            stack_pointer[-3] = res;
            stack_pointer += -2;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            DISPATCH();
        }

        TARGET(OP1) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP1;
            (void)(opcode);
            #endif
            _Py_CODEUNIT* const this_instr = next_instr;
            (void)this_instr;
            _PyFb_OP1 *fb = _PyFb_Entry(this_instr);
            frame->instr_ptr = next_instr;
            next_instr += 2;
            INSTRUCTION_STATS(OP1);
            _PyStackRef left;
            _PyStackRef right;
            right = stack_pointer[-1];
            left = stack_pointer[-2];
            uint32_t index = fb->index;
            (void)index;
            _PyFrame_SetStackPointer(frame, stack_pointer);
            _PyFrame_StackPointerValidate(frame);
            op1(left, right);
            _PyFrame_StackPointerInvalidate(frame);
            DISPATCH();
        }

        TARGET(OP3) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP3;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 6;
            INSTRUCTION_STATS(OP3);
            static_assert(INLINE_CACHE_ENTRIES_OP == 5, "incorrect cache size");
            _PyStackRef arg2;
            _PyStackRef left;
            _PyStackRef right;
            _PyStackRef res;
            /* Skip 5 cache entries */
            right = stack_pointer[-1];
            left = stack_pointer[-2];
            arg2 = stack_pointer[-3];
            _PyFrame_SetStackPointer(frame, stack_pointer);
            _PyFrame_StackPointerValidate(frame);
            res = op3(arg2, left, right);
            _PyFrame_StackPointerInvalidate(frame);
            stack_pointer[-3] = res;
            stack_pointer += -2;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_unused_caches(self):
        input = """
        inst(OP, (unused/1, unused/2 --)) {
            body;
        }
    """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 4;
            INSTRUCTION_STATS(OP);
            /* Skip 1 cache entry */
            /* Skip 2 cache entries */
            body;
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_pseudo_instruction_no_flags(self):
        input = """
        pseudo(OP, (in -- out1, out2)) = {
            OP1,
        };

        inst(OP1, (--)) {
        }
    """
        output = """
        TARGET(OP1) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP1;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP1);
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_pseudo_instruction_with_flags(self):
        input = """
        pseudo(OP, (in1, in2 --), (HAS_ARG, HAS_JUMP)) = {
            OP1,
        };

        inst(OP1, (--)) {
        }
    """
        output = """
        TARGET(OP1) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP1;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP1);
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_pseudo_instruction_as_sequence(self):
        input = """
        pseudo(OP, (in -- out1, out2)) = [
            OP1, OP2
        ];

        inst(OP1, (--)) {
        }

        inst(OP2, (--)) {
        }
    """
        output = """
        TARGET(OP1) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP1;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP1);
            DISPATCH();
        }

        TARGET(OP2) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP2;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP2);
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)


    def test_array_input(self):
        input = """
        inst(OP, (below, values[oparg*2], above --)) {
            SPAM(values, oparg);
            DEAD(below);
            DEAD(values);
            DEAD(above);
        }
    """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef below;
            _PyStackRef *values;
            _PyStackRef above;
            above = stack_pointer[-1];
            values = &stack_pointer[-1 - oparg*2];
            below = stack_pointer[-2 - oparg*2];
            SPAM(values, oparg);
            stack_pointer += -2 - oparg*2;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_array_output(self):
        input = """
        inst(OP, (unused, unused -- below, values[oparg*3], above)) {
            SPAM(values, oparg);
            below = 0;
            above = 0;
        }
    """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef below;
            _PyStackRef *values;
            _PyStackRef above;
            values = &stack_pointer[-1];
            SPAM(values, oparg);
            below = 0;
            above = 0;
            stack_pointer[-2] = below;
            stack_pointer[-1 + oparg*3] = above;
            stack_pointer += oparg*3;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_array_input_output(self):
        input = """
        inst(OP, (values[oparg] -- values[oparg], above)) {
            SPAM(values, oparg);
            above = 0;
        }
    """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef *values;
            _PyStackRef above;
            values = &stack_pointer[-oparg];
            SPAM(values, oparg);
            above = 0;
            stack_pointer[0] = above;
            stack_pointer += 1;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_array_error_if(self):
        input = """
        inst(OP, (extra, values[oparg] --)) {
            DEAD(extra);
            DEAD(values);
            ERROR_IF(oparg == 0);
        }
    """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef extra;
            _PyStackRef *values;
            values = &stack_pointer[-oparg];
            extra = stack_pointer[-1 - oparg];
            if (oparg == 0) {
                stack_pointer += -1 - oparg;
                ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
                JUMP_TO_LABEL(error);
            }
            stack_pointer += -1 - oparg;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_macro_push_push(self):
        input = """
        op(A, (-- val1)) {
            val1 = SPAM();
        }
        op(B, (-- val2)) {
            val2 = SPAM();
        }
        macro(M) = A + B;
        """
        output = """
        TARGET(M) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = M;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(M);
            _PyStackRef val1;
            _PyStackRef val2;
            // A
            {
                val1 = SPAM();
            }
            // B
            {
                val2 = SPAM();
            }
            stack_pointer[0] = val1;
            stack_pointer[1] = val2;
            stack_pointer += 2;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_override_inst(self):
        input = """
        inst(OP, (--)) {
            spam;
        }
        override inst(OP, (--)) {
            ham;
        }
        """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            ham;
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_override_op(self):
        input = """
        op(OP, (--)) {
            spam;
        }
        macro(M) = OP;
        override op(OP, (--)) {
            ham;
        }
        """
        output = """
        TARGET(M) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = M;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(M);
            ham;
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_annotated_inst(self):
        input = """
        pure inst(OP, (--)) {
            ham;
        }
        """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            ham;
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_annotated_op(self):
        input = """
        pure op(OP, (--)) {
            SPAM();
        }
        macro(M) = OP;
        """
        output = """
        TARGET(M) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = M;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(M);
            SPAM();
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

        input = """
        pure register specializing op(OP, (--)) {
            SPAM();
        }
        macro(M) = OP;
        """
        self.run_cases_test(input, output)

    def test_deopt_and_exit(self):
        input = """
        pure op(OP, (arg1 -- out)) {
            DEOPT_IF(1);
            EXIT_IF(1);
        }
        """
        output = ""
        with self.assertRaises(SyntaxError):
            self.run_cases_test(input, output)

    def test_inline_value_boxed(self):
        input = """
        inst(OP, (value --)) {
            SPAM(PyStackRef_AsPyObjectBorrow(value));
            PyStackRef_CLOSE(value);
        }

        inline_ok inst(OP_INLINE_OK, (value --)) {
            SPAM(PyStackRef_AsPyObjectBorrow(value));
            PyStackRef_CLOSE(value);
        }
        """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef value;
            value = stack_pointer[-1];
            if (Py_UNLIKELY(PyStackRef_IsInlineAt(&stack_pointer[-1]))) {
                value = PyStackRef_Box(value);
                if (PyStackRef_IsInline(value)) {
                    JUMP_TO_LABEL(error);
                }
                BOX_STAT_INC(OP);
                stack_pointer[-1] = value;
            }
            SPAM(PyStackRef_AsPyObjectBorrow(value));
            stack_pointer += -1;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            _PyFrame_SetStackPointer(frame, stack_pointer);
            _PyFrame_StackPointerValidate(frame);
            _PyStackRef_CLOSE_POINTER(value);
            _PyFrame_StackPointerInvalidate(frame);
            DISPATCH();
        }

        TARGET(OP_INLINE_OK) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP_INLINE_OK;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP_INLINE_OK);
            _PyStackRef value;
            value = stack_pointer[-1];
            SPAM(PyStackRef_AsPyObjectBorrow(value));
            stack_pointer += -1;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            _PyFrame_SetStackPointer(frame, stack_pointer);
            _PyFrame_StackPointerValidate(frame);
            PyStackRef_CLOSE(value);
            _PyFrame_StackPointerInvalidate(frame);
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_inline_value_guard(self):
        # The guard exits on an inline value; _USE then needs no box.
        input = """
        inst(OP, (value --)) {
            DEAD(value);
        }

        op(_GUARD, (value -- value)) {
            DEOPT_IF(Py_TYPE(PyStackRef_AsPyObjectBorrow(value)) != &PyLong_Type);
        }

        op(_USE, (value --)) {
            SPAM(PyStackRef_AsPyObjectBorrow(value));
            DEAD(value);
        }

        macro(OP_INT) = _GUARD + _USE;

        family(OP, 0) = { OP_INT };
        """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            PREDICTED_OP:;
            _PyStackRef value;
            value = stack_pointer[-1];
            stack_pointer += -1;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            DISPATCH();
        }

        TARGET(OP_INT) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP_INT;
            (void)(opcode);
            #endif
            _Py_CODEUNIT* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP_INT);
            static_assert(0 == 0, "incorrect cache size");
            _PyStackRef value;
            // _GUARD
            {
                value = stack_pointer[-1];
                if (Py_UNLIKELY(!PyStackRef_IsPointerSpaceAt(&stack_pointer[-1]))) {
                    UPDATE_MISS_STATS(OP);
                    assert(_PyOpcode_Deopt[opcode] == (OP));
                    JUMP_TO_PREDICTED(OP);
                }
                if (Py_TYPE(PyStackRef_AsPyObjectBorrow(value)) != &PyLong_Type) {
                    UPDATE_MISS_STATS(OP);
                    assert(_PyOpcode_Deopt[opcode] == (OP));
                    JUMP_TO_PREDICTED(OP);
                }
            }
            // _USE
            {
                SPAM(PyStackRef_AsPyObjectBorrow(value));
            }
            stack_pointer += -1;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_inline_value_box_not_written_back(self):
        # Nothing after the boxes reads the stack, so they stay in registers;
        # if the second box fails, the first is stored before the error exit.
        input = """
        inst(OP, (left, right -- res)) {
            res = PyStackRef_FromPyObjectSteal(PAIR(PyStackRef_AsPyObjectSteal(left), PyStackRef_AsPyObjectSteal(right)));
        }
        """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef left;
            _PyStackRef right;
            _PyStackRef res;
            right = stack_pointer[-1];
            left = stack_pointer[-2];
            if (Py_UNLIKELY(PyStackRef_IsInlineAt(&stack_pointer[-2]))) {
                left = PyStackRef_Box(left);
                if (PyStackRef_IsInline(left)) {
                    JUMP_TO_LABEL(error);
                }
                BOX_STAT_INC(OP);
            }
            if (Py_UNLIKELY(PyStackRef_IsInlineAt(&stack_pointer[-1]))) {
                right = PyStackRef_Box(right);
                if (PyStackRef_IsInline(right)) {
                    stack_pointer[-2] = left;
                    JUMP_TO_LABEL(error);
                }
                BOX_STAT_INC(OP);
            }
            res = PyStackRef_FromPyObjectSteal(PAIR(_PyStackRef_AsPyObjectStealPointer(left), _PyStackRef_AsPyObjectStealPointer(right)));
            stack_pointer[-2] = res;
            stack_pointer += -1;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_inline_value_asserted_object(self):
        # An input asserted to be an object is neither tested nor boxed.
        input = """
        inst(OP, (iter --)) {
            assert(PyStackRef_IsObject(iter));
            SPAM(PyStackRef_AsPyObjectBorrow(iter));
            PyStackRef_CLOSE(iter);
        }
        """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef iter;
            iter = stack_pointer[-1];
            assert(PyStackRef_IsObject(iter));
            SPAM(PyStackRef_AsPyObjectBorrow(iter));
            stack_pointer += -1;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            _PyFrame_SetStackPointer(frame, stack_pointer);
            _PyFrame_StackPointerValidate(frame);
            _PyStackRef_CLOSE_POINTER(iter);
            _PyFrame_StackPointerInvalidate(frame);
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_known_pointer_assignment(self):
        # A constructor result is a pointer, so _B closes it with the
        # pointer-only variant, but not when only one path assigned it.
        input = """
        op(_A, (value -- value)) {
            if (COND) {
                value = PyStackRef_FromPyObjectNew(SPAM());
            }
        }

        op(_B, (value --)) {
            PyStackRef_CLOSE(value);
        }

        op(_C, (value -- value)) {
            value = PyStackRef_FromPyObjectNew(SPAM());
        }

        macro(OP) = _A + _B;
        macro(OP2) = _C + _B;
        """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef value;
            // _A
            {
                value = stack_pointer[-1];
                if (COND) {
                    value = PyStackRef_FromPyObjectNew(SPAM());
                }
            }
            // _B
            {
                stack_pointer += -1;
                ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
                _PyFrame_SetStackPointer(frame, stack_pointer);
                _PyFrame_StackPointerValidate(frame);
                PyStackRef_CLOSE(value);
                _PyFrame_StackPointerInvalidate(frame);
            }
            DISPATCH();
        }

        TARGET(OP2) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP2;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP2);
            _PyStackRef value;
            // _C
            {
                value = stack_pointer[-1];
                value = PyStackRef_FromPyObjectNew(SPAM());
            }
            // _B
            {
                stack_pointer += -1;
                ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
                _PyFrame_SetStackPointer(frame, stack_pointer);
                _PyFrame_StackPointerValidate(frame);
                _PyStackRef_CLOSE_POINTER(value);
                _PyFrame_StackPointerInvalidate(frame);
            }
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_array_of_one(self):
        input = """
        inst(OP, (arg[1] -- out[1])) {
            out[0] = arg[0];
            DEAD(arg);
        }
        """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef *arg;
            _PyStackRef *out;
            arg = &stack_pointer[-1];
            out = &stack_pointer[-1];
            out[0] = arg[0];
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_unused_cached_value(self):
        input = """
        op(FIRST, (arg1 -- out)) {
            out = arg1;
        }

        op(SECOND, (unused -- unused)) {
        }

        macro(BOTH) = FIRST + SECOND;
        """
        output = """
        """
        with self.assertRaises(SyntaxError):
            self.run_cases_test(input, output)

    def test_unused_named_values(self):
        input = """
        op(OP, (named -- named)) {
        }

        macro(INST) = OP;
        """
        output = """
        TARGET(INST) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = INST;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(INST);
            DISPATCH();
        }

        """
        self.run_cases_test(input, output)

    def test_used_unused_used(self):
        input = """
        op(FIRST, (w -- w)) {
            USE(w);
        }

        op(SECOND, (x -- x)) {
        }

        op(THIRD, (y -- y)) {
            USE(y);
        }

        macro(TEST) = FIRST + SECOND + THIRD;
        """
        output = """
        TARGET(TEST) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = TEST;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(TEST);
            _PyStackRef w;
            _PyStackRef y;
            // FIRST
            {
                w = stack_pointer[-1];
                USE(w);
            }
            // SECOND
            {
            }
            // THIRD
            {
                y = w;
                USE(y);
            }
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_unused_used_used(self):
        input = """
        op(FIRST, (w -- w)) {
        }

        op(SECOND, (x -- x)) {
            USE(x);
        }

        op(THIRD, (y -- y)) {
            USE(y);
        }

        macro(TEST) = FIRST + SECOND + THIRD;
        """
        output = """
        TARGET(TEST) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = TEST;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(TEST);
            _PyStackRef x;
            _PyStackRef y;
            // FIRST
            {
            }
            // SECOND
            {
                x = stack_pointer[-1];
                USE(x);
            }
            // THIRD
            {
                y = x;
                USE(y);
            }
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_flush(self):
        input = """
        op(FIRST, ( -- a, b)) {
            a = 0;
            b = 1;
        }

        op(SECOND, (a, b -- )) {
            USE(a, b);
            INPUTS_DEAD();
        }

        macro(TEST) = FIRST + flush + SECOND;
        """
        output = """
        TARGET(TEST) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = TEST;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(TEST);
            _PyStackRef a;
            _PyStackRef b;
            // FIRST
            {
                a = 0;
                b = 1;
            }
            // flush
            stack_pointer[0] = a;
            stack_pointer[1] = b;
            stack_pointer += 2;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            // SECOND
            {
                USE(a, b);
            }
            stack_pointer += -2;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_pop_on_error_peeks(self):

        input = """
        op(FIRST, (x, y -- a, b)) {
            a = x;
            DEAD(x);
            b = y;
            DEAD(y);
        }

        op(SECOND, (a, b -- a, b)) {
        }

        op(THIRD, (j, k --)) {
            INPUTS_DEAD(); // Mark j and k as used
            ERROR_IF(cond);
        }

        macro(TEST) = FIRST + SECOND + THIRD;
        """
        output = """
        TARGET(TEST) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = TEST;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(TEST);
            _PyStackRef x;
            _PyStackRef y;
            _PyStackRef a;
            _PyStackRef b;
            // FIRST
            {
                y = stack_pointer[-1];
                x = stack_pointer[-2];
                a = x;
                b = y;
            }
            // SECOND
            {
            }
            // THIRD
            {
                if (cond) {
                    JUMP_TO_LABEL(pop_2_error);
                }
            }
            stack_pointer += -2;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_push_then_error(self):

        input = """
        op(FIRST, ( -- a)) {
            a = 1;
        }

        op(SECOND, (a -- a, b)) {
            b = 1;
            ERROR_IF(cond);
        }

        macro(TEST) = FIRST + SECOND;
        """

        output = """
        TARGET(TEST) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = TEST;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(TEST);
            _PyStackRef a;
            _PyStackRef b;
            // FIRST
            {
                a = 1;
            }
            // SECOND
            {
                b = 1;
                if (cond) {
                    stack_pointer[0] = a;
                    stack_pointer[1] = b;
                    stack_pointer += 2;
                    ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
                    JUMP_TO_LABEL(error);
                }
            }
            stack_pointer[0] = a;
            stack_pointer[1] = b;
            stack_pointer += 2;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_error_if_true(self):

        input = """
        inst(OP1, ( --)) {
            ERROR_IF(true);
        }
        inst(OP2, ( --)) {
            ERROR_IF(1);
        }
        """
        output = """
        TARGET(OP1) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP1;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP1);
            JUMP_TO_LABEL(error);
        }

        TARGET(OP2) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP2;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP2);
            JUMP_TO_LABEL(error);
        }
        """
        self.run_cases_test(input, output)

    def test_scalar_array_inconsistency(self):

        input = """
        op(FIRST, ( -- a)) {
            a = 1;
        }

        op(SECOND, (a[1] -- b)) {
            b = 1;
        }

        macro(TEST) = FIRST + SECOND;
        """

        output = """
        """
        with self.assertRaises(SyntaxError):
            self.run_cases_test(input, output)

    def test_array_size_inconsistency(self):

        input = """
        op(FIRST, ( -- a[2])) {
            a[0] = 1;
        }

        op(SECOND, (a[1] -- b)) {
            b = 1;
        }

        macro(TEST) = FIRST + SECOND;
        """

        output = """
        """
        with self.assertRaises(SyntaxError):
            self.run_cases_test(input, output)

    def test_stack_save_reload(self):

        input = """
        inst(BALANCED, ( -- )) {
            SAVE_STACK();
            code();
            RELOAD_STACK();
        }
        """

        output = """
        TARGET(BALANCED) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = BALANCED;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(BALANCED);
            _PyFrame_SetStackPointer(frame, stack_pointer);
            _PyFrame_StackPointerValidate(frame);
            code();
            stack_pointer = _PyFrame_GetStackPointer(frame);
            _PyFrame_StackPointerInvalidate(frame);
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_stack_save_reload_paired(self):

        input = """
        inst(BALANCED, ( -- )) {
            SAVE_STACK();
            RELOAD_STACK();
        }
        """

        output = """
        TARGET(BALANCED) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = BALANCED;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(BALANCED);
            _PyFrame_SetStackPointer(frame, stack_pointer);
            _PyFrame_StackPointerValidate(frame);
            stack_pointer = _PyFrame_GetStackPointer(frame);
            _PyFrame_StackPointerInvalidate(frame);
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_stack_reload_only(self):

        input = """
        inst(BALANCED, ( -- )) {
            RELOAD_STACK();
        }
        """

        output = """
        TARGET(BALANCED) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = BALANCED;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(BALANCED);
            // Explicit stack reload
            stack_pointer = _PyFrame_GetStackPointer(frame);
            _PyFrame_StackAssertInvalid(frame);
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_stack_save_only(self):

        input = """
        inst(BALANCED, ( -- )) {
            SAVE_STACK();
        }
        """

        output = """
        TARGET(BALANCED) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = BALANCED;
            (void)(opcode);
            #endif
            _Py_CODEUNIT* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(BALANCED);
            _PyFrame_SetStackPointer(frame, stack_pointer);
            stack_pointer = _PyFrame_GetStackPointer(frame);
            DISPATCH();
        }
        """
        with self.assertRaises(SyntaxError):
            self.run_cases_test(input, output)

    def test_instruction_size_macro(self):
        input = """
        inst(OP, (--)) {
            frame->return_offset = INSTRUCTION_SIZE;
        }
        """

        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            frame->return_offset = 1u ;
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

        # Two instructions of different sizes referencing the same
        # uop containing the `INSTRUCTION_SIZE` macro is not allowed.
        input = """
        inst(OP, (--)) {
            frame->return_offset = INSTRUCTION_SIZE;
        }
        macro(OP2) = unused/1 + OP;
        """

        output = ""  # No output needed as this should raise an error.
        with self.assertRaisesRegex(SyntaxError, "All instructions containing a uop"):
            self.run_cases_test(input, output)

    def test_escaping_call_next_to_cmacro(self):
        input = """
        inst(OP, (--)) {
            #ifdef Py_GIL_DISABLED
            escaping_call();
            #else
            another_escaping_call();
            #endif
            yet_another_escaping_call();
        }
        """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            #ifdef Py_GIL_DISABLED
            _PyFrame_SetStackPointer(frame, stack_pointer);
            _PyFrame_StackPointerValidate(frame);
            escaping_call();
            _PyFrame_StackPointerInvalidate(frame);
            #else
            _PyFrame_SetStackPointer(frame, stack_pointer);
            _PyFrame_StackPointerValidate(frame);
            another_escaping_call();
            _PyFrame_StackPointerInvalidate(frame);
            #endif
            assert(stack_pointer == _PyFrame_GetStackPointer(frame));
            _PyFrame_StackPointerValidate(frame);
            yet_another_escaping_call();
            _PyFrame_StackPointerInvalidate(frame);
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_pystackref_frompyobject_new_next_to_cmacro(self):
        input = """
        inst(OP, (-- out1, out2)) {
            PyObject *obj = SPAM();
            #ifdef Py_GIL_DISABLED
            out1 = PyStackRef_FromPyObjectNew(obj);
            #else
            out1 = PyStackRef_FromPyObjectNew(obj);
            #endif
            out2 = PyStackRef_FromPyObjectNew(obj);
        }
        """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef out1;
            _PyStackRef out2;
            PyObject *obj = SPAM();
            #ifdef Py_GIL_DISABLED
            out1 = PyStackRef_FromPyObjectNew(obj);
            #else
            out1 = PyStackRef_FromPyObjectNew(obj);
            #endif
            out2 = PyStackRef_FromPyObjectNew(obj);
            stack_pointer[0] = out1;
            stack_pointer[1] = out2;
            stack_pointer += 2;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_no_escaping_calls_in_branching_macros(self):

        input = """
        inst(OP, ( -- )) {
            DEOPT_IF(escaping_call());
        }
        """
        with self.assertRaises(SyntaxError):
            self.run_cases_test(input, "")

        input = """
        inst(OP, ( -- )) {
            EXIT_IF(escaping_call());
        }
        """
        with self.assertRaises(SyntaxError):
            self.run_cases_test(input, "")

        input = """
        inst(OP, ( -- )) {
            ERROR_IF(escaping_call());
        }
        """
        with self.assertRaises(SyntaxError):
            self.run_cases_test(input, "")

    def test_kill_in_wrong_order(self):
        input = """
        inst(OP, (a, b -- c)) {
            c = b;
            PyStackRef_CLOSE(a);
            PyStackRef_CLOSE(b);
        }
        """
        with self.assertRaises(SyntaxError):
            self.run_cases_test(input, "")

    def test_complex_label(self):
        input = """
        label(other_label) {
        }

        label(other_label2) {
        }

        label(my_label) {
            // Comment
            do_thing();
            if (complex) {
                goto other_label;
            }
            goto other_label2;
        }
        """

        output = """
        LABEL(other_label)
        {
        }

        LABEL(other_label2)
        {
        }

        LABEL(my_label)
        {
            _PyFrame_SetStackPointer(frame, stack_pointer);
            _PyFrame_StackPointerValidate(frame);
            do_thing();
            _PyFrame_StackPointerInvalidate(frame);
            if (complex) {
                JUMP_TO_LABEL(other_label);
            }
            JUMP_TO_LABEL(other_label2);
        }
        """
        self.run_cases_test(input, output)

    def test_spilled_label(self):
        input = """
        spilled label(one) {
            RELOAD_STACK();
            goto two;
        }

        label(two) {
            SAVE_STACK();
            goto one;
        }
        """

        output = """
        LABEL(one)
        {
            stack_pointer = _PyFrame_GetStackPointer(frame);
            _PyFrame_StackPointerInvalidate(frame);
            JUMP_TO_LABEL(two);
        }

        LABEL(two)
        {
            _PyFrame_SetStackPointer(frame, stack_pointer);
            _PyFrame_StackPointerValidate(frame);
            JUMP_TO_LABEL(one);
        }
        """
        self.run_cases_test(input, output)


    def test_incorrect_spills(self):
        input1 = """
        spilled label(one) {
            goto two;
        }

        label(two) {
        }
        """

        input2 = """
        spilled label(one) {
        }

        label(two) {
            goto one;
        }
        """
        with self.assertRaisesRegex(SyntaxError, ".*reload.*"):
            self.run_cases_test(input1, "")
        with self.assertRaisesRegex(SyntaxError, ".*spill.*"):
            self.run_cases_test(input2, "")


    def test_multiple_labels(self):
        input = """
        label(my_label_1) {
            // Comment
            do_thing1();
            goto my_label_2;
        }

        label(my_label_2) {
            // Comment
            do_thing2();
            goto my_label_1;
        }
        """

        output = """
        LABEL(my_label_1)
        {
            _PyFrame_SetStackPointer(frame, stack_pointer);
            _PyFrame_StackPointerValidate(frame);
            do_thing1();
            _PyFrame_StackPointerInvalidate(frame);
            JUMP_TO_LABEL(my_label_2);
        }

        LABEL(my_label_2)
        {
            _PyFrame_SetStackPointer(frame, stack_pointer);
            _PyFrame_StackPointerValidate(frame);
            do_thing2();
            _PyFrame_StackPointerInvalidate(frame);
            JUMP_TO_LABEL(my_label_1);
        }
        """
        self.run_cases_test(input, output)

    def test_reassigning_live_inputs(self):
        input = """
        inst(OP, (in -- in)) {
            in = 0;
        }
        """

        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef in;
            in = stack_pointer[-1];
            in = 0;
            stack_pointer[-1] = in;
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_reassigning_dead_inputs(self):
        input = """
        inst(OP, (in -- )) {
            temp = use(in);
            DEAD(in);
            in = temp;
            PyStackRef_CLOSE(in);
        }
        """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef in;
            in = stack_pointer[-1];
            _PyFrame_SetStackPointer(frame, stack_pointer);
            _PyFrame_StackPointerValidate(frame);
            temp = use(in);
            _PyFrame_StackPointerInvalidate(frame);
            in = temp;
            stack_pointer += -1;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            _PyFrame_SetStackPointer(frame, stack_pointer);
            _PyFrame_StackPointerValidate(frame);
            PyStackRef_CLOSE(in);
            _PyFrame_StackPointerInvalidate(frame);
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_escaping_in_loop(self):
        input = """
        inst(TEST_LOOP_SPILL, (a -- a)) {
            int n = (int)oparg;
            for (int i = 0; i < n; i++) {
                escaping_inside_loop(a);
            }
            escaping_after_loop(a);
        }
        """
        output = """
        TARGET(TEST_LOOP_SPILL) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = TEST_LOOP_SPILL;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(TEST_LOOP_SPILL);
            _PyStackRef a;
            a = stack_pointer[-1];
            int n = (int)oparg;
            for (int i = 0; i < n; i++) {
                _PyFrame_SetStackPointer(frame, stack_pointer);
                _PyFrame_StackPointerValidate(frame);
                escaping_inside_loop(a);
                _PyFrame_StackPointerInvalidate(frame);
            }
            _PyFrame_SetStackPointer(frame, stack_pointer);
            _PyFrame_StackPointerValidate(frame);
            escaping_after_loop(a);
            _PyFrame_StackPointerInvalidate(frame);
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)


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

    def run_ir_test(self, input: str, expected: str):
        analysis = self.analyze(input)
        out = io.StringIO()
        ir_generator.generate_ir([self.temp_input_filename], analysis, out, False)
        _, rest = out.getvalue().split(ir_generator.INSTRUCTION_START_MARKER)
        instructions, _ = rest.split(ir_generator.INSTRUCTION_END_MARKER)
        self.assertEqual(instructions.strip(), expected.strip())

    def test_register_style(self):
        # Inputs that are only used as objects are read through their fields
        # and may be borrowed locals.
        input = """
        inst(OP, (left, right -- res)) {
            PyObject *l = PyStackRef_AsPyObjectBorrow(left);
            PyObject *r = PyStackRef_AsPyObjectBorrow(right);
            PyObject *o = SPAM(l, r);
            DECREF_INPUTS();
            ERROR_IF(o == NULL);
            res = PyStackRef_FromPyObjectSteal(o);
        }
    """
        output = """
        TARGET(OP) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            IR_ENTRY(OP);
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            IR_CHECK_SLOT(ir_a, -2);
            IR_CHECK_INPUT(ir_b, -2, 2);
            IR_CHECK_INPUT(ir_c, -1, 2);
            _PyStackRef left;
            _PyStackRef right;
            _PyStackRef res;
            right = IR_SLOT(ir_c);
            left = IR_SLOT(ir_b);
            if (Py_UNLIKELY(PyStackRef_IsInline(left))) {
                left = PyStackRef_Box(left);
                if (PyStackRef_IsInline(left)) {
                    JUMP_TO_LABEL(error);
                }
                BOX_STAT_INC(OP);
                IR_SLOT(ir_b) = left;
            }
            left = IR_BORROW_KNOWN(left, ir_b);
            if (Py_UNLIKELY(PyStackRef_IsInline(right))) {
                right = PyStackRef_Box(right);
                if (PyStackRef_IsInline(right)) {
                    JUMP_TO_LABEL(error);
                }
                BOX_STAT_INC(OP);
                IR_SLOT(ir_c) = right;
            }
            right = IR_BORROW_KNOWN(right, ir_c);
            PyObject *l = PyStackRef_AsPyObjectBorrow(left);
            PyObject *r = PyStackRef_AsPyObjectBorrow(right);
            PyObject *o = SPAM(l, r);
            _PyFrame_SetStackPointer(frame, stack_pointer);
            _PyFrame_StackPointerValidate(frame);
            _PyStackRef tmp = right;
            right = PyStackRef_NULL;
            stack_pointer[-1] = right;
            _PyStackRef_CLOSE_POINTER(tmp);
            _PyFrame_StackPointerInvalidate(frame);
            assert(stack_pointer == _PyFrame_GetStackPointer(frame));
            _PyFrame_StackPointerValidate(frame);
            tmp = left;
            left = PyStackRef_NULL;
            stack_pointer[-2] = left;
            _PyStackRef_CLOSE_POINTER(tmp);
            _PyFrame_StackPointerInvalidate(frame);
            stack_pointer += -2;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            if (o == NULL) {
                JUMP_TO_LABEL(error);
            }
            res = PyStackRef_FromPyObjectSteal(o);
            stack_pointer[0] = res;
            stack_pointer += 1;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            DISPATCH();
        }
    """
        self.run_ir_test(input, output)
        fmt = self.formats(input)["OP"]
        self.assertFalse(fmt.window)
        self.assertEqual(fmt.roles, [ir_generator.ROLE_DST,
                                     f"{ir_generator.ROLE_IN0} + 0",
                                     f"{ir_generator.ROLE_IN0} + 1"])
        self.assertEqual(fmt.dst_offset, -2)
        self.assertEqual(fmt.borrow_mask, 0b11)

    def test_borrow_mask(self):
        input = """
        inst(OP_PASS, (value -- value, res)) {
            res = SPAM(PyStackRef_AsPyObjectBorrow(value));
        }
        inst(OP_OBJ, (value -- res)) {
            res = SPAM(PyStackRef_AsPyObjectBorrow(value));
            PyStackRef_CLOSE(value);
        }
        inst(OP_FAMILY, (value -- res)) {
            res = SPAM(PyStackRef_AsPyObjectBorrow(value));
            PyStackRef_CLOSE(value);
        }
        inst(OP_FAMILY_RAW, (value -- res)) {
            res = SPAM_RAW(value);
            PyStackRef_CLOSE(value);
        }
        family(OP_FAMILY, 0) = { OP_FAMILY_RAW };
    """
        formats = self.formats(input)
        self.assertEqual(formats["OP_OBJ"].borrow_mask, 1)
        # An input that stays on the stack is not borrowed: the stack slot
        # of a borrowed local holds NULL.
        self.assertEqual(formats["OP_PASS"].borrow_mask, 0)
        self.assertEqual(formats["OP_PASS"].dst_offset, 0)
        # A member that consumes the input raw denies the whole family.
        self.assertEqual(formats["OP_FAMILY"].borrow_mask, 0)
        self.assertIs(formats["OP_FAMILY_RAW"], formats["OP_FAMILY"])

    def test_unused_outputs_pass_through(self):
        input = """
        op(_GUARD, (nos, unused -- nos, unused)) {
            EXIT_IF(!PyStackRef_IsObject(nos));
        }
        op(_USE, (left, right -- left, res)) {
            res = SPAM(PyStackRef_AsPyObjectBorrow(right));
            PyStackRef_CLOSE(right);
        }
        macro(OP) = _GUARD + _USE;
    """
        fmt = self.formats(input)["OP"]
        self.assertEqual(fmt.inputs, ["nos", "unused"])
        self.assertEqual(fmt.dst_offset, -1)
        self.assertEqual(fmt.borrow_mask, 0b10)

    def test_window_style(self):
        # An array input selects the window style: A names the deepest
        # input, which the handler binds through stack_pointer.
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
            #endif
            IR_ENTRY(OP);
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            int oparg = (int)ir_b;
            (void)oparg;
            IR_CHECK_SLOT(ir_a, -(1 + oparg));
            _PyStackRef callable;
            _PyStackRef *args;
            _PyStackRef res;
            args = &stack_pointer[-oparg];
            callable = stack_pointer[-1 - oparg];
            res = SPAM(callable, args, oparg);
            stack_pointer[-1 - oparg] = res;
            stack_pointer += -oparg;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            DISPATCH();
        }
    """
        self.run_ir_test(input, output)
        fmt = self.formats(input)["OP"]
        self.assertTrue(fmt.window)
        self.assertEqual(fmt.roles, [ir_generator.ROLE_BASE,
                                     ir_generator.ROLE_IMM,
                                     ir_generator.ROLE_NONE])
        self.assertEqual(fmt.imm, ir_generator.LOC_B)

    def test_window_selection(self):
        input = """
        inst(OP_FOUR, (a, b, c -- res)) {
            res = SPAM(a, b, c);
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
        # Three inputs and a destination need more than three fields
        self.assertTrue(formats["OP_FOUR"].window)
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
        # A family shares one format. With a destination, an input and a
        # feedback entry, the oparg moves into the entry's arg field, and
        # cache effects read the entry.
        input = """
        specializing op(_SPECIALIZE_OP, (counter/1, owner -- owner)) {
            _PyFbHeader *feedback = FB_COUNTER();
            ADVANCE_ADAPTIVE_COUNTER(feedback);
        }
        op(_OP, (owner -- res)) {
            PyObject *o = PyStackRef_AsPyObjectBorrow(owner);
            res = SPAM(o, oparg);
            DECREF_INPUTS();
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
            #endif
            IR_ENTRY(OP);
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            IR_PREDICTED(OP);
            _PyIRWord* const this_instr = next_instr - 1;
            (void)this_instr;
            _PyFb_OP *fb = (_PyFb_OP *)IR_FB(this_instr, ir_c);
            int oparg = (int)((_PyFbHeader *)fb)->arg;
            (void)oparg;
            IR_CHECK_SLOT(ir_a, -1);
            IR_CHECK_INPUT(ir_b, -1, 1);
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
                owner = IR_SLOT(ir_b);
                if (Py_UNLIKELY(PyStackRef_IsInline(owner))) {
                    owner = PyStackRef_Box(owner);
                    if (PyStackRef_IsInline(owner)) {
                        JUMP_TO_LABEL(error);
                    }
                    BOX_STAT_INC(OP);
                    IR_SLOT(ir_b) = owner;
                }
                owner = IR_BORROW_KNOWN(owner, ir_b);
                PyObject *o = PyStackRef_AsPyObjectBorrow(owner);
                res = SPAM(o, oparg);
                _PyFrame_SetStackPointer(frame, stack_pointer);
                _PyFrame_StackPointerValidate(frame);
                _PyStackRef tmp = owner;
                owner = res;
                stack_pointer[-1] = owner;
                _PyStackRef_CLOSE_POINTER(tmp);
                _PyFrame_StackPointerInvalidate(frame);
            }
            DISPATCH();
        }

        TARGET(OP_SPECIALIZED) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = OP_SPECIALIZED;
            (void)(opcode);
            #endif
            IR_ENTRY(OP_SPECIALIZED);
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP_SPECIALIZED);
            _PyFb_OP *fb = (_PyFb_OP *)IR_FB(this_instr, ir_c);
            int oparg = (int)((_PyFbHeader *)fb)->arg;
            (void)oparg;
            IR_CHECK_SLOT(ir_a, -1);
            IR_CHECK_INPUT(ir_b, -1, 1);
            _PyStackRef owner;
            _PyStackRef res;
            // _GUARD
            {
                owner = IR_SLOT(ir_b);
                uint32_t type_version = fb->type_version;
                if (Py_UNLIKELY(!PyStackRef_IsPointerSpace(owner))) {
                    UPDATE_MISS_STATS(OP);
                    assert(_PyOpcode_Deopt[opcode] == (OP));
                    JUMP_TO_PREDICTED(OP);
                }
                owner = IR_BORROW_KNOWN(owner, ir_b);
                if (Py_TYPE(PyStackRef_AsPyObjectBorrow(owner))->tp_version_tag != type_version) {
                    UPDATE_MISS_STATS(OP);
                    assert(_PyOpcode_Deopt[opcode] == (OP));
                    JUMP_TO_PREDICTED(OP);
                }
            }
            // _OP
            {
                PyObject *o = PyStackRef_AsPyObjectBorrow(owner);
                res = SPAM(o, oparg);
                _PyFrame_SetStackPointer(frame, stack_pointer);
                _PyFrame_StackPointerValidate(frame);
                _PyStackRef tmp = owner;
                owner = res;
                stack_pointer[-1] = owner;
                _PyStackRef_CLOSE_POINTER(tmp);
                _PyFrame_StackPointerInvalidate(frame);
            }
            DISPATCH();
        }
    """
        self.run_ir_test(input, output)
        fmt = self.formats(input)["OP_SPECIALIZED"]
        self.assertEqual(fmt.group, "OP")
        self.assertEqual(fmt.fb, ir_generator.LOC_C)
        self.assertEqual(fmt.imm, ir_generator.LOC_FBARG)

    def test_two_words(self):
        # Two inputs, a destination, an oparg and an entry take word 1.
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
            #endif
            IR_ENTRY(OP);
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 2;
            INSTRUCTION_STATS(OP);
            _PyFb_OP *fb = (_PyFb_OP *)IR_FB(this_instr, IR_F(this_instr[1]));
            int oparg = (int)IR_D(this_instr[1]);
            (void)oparg;
            IR_CHECK_SLOT(ir_a, -2);
            IR_CHECK_INPUT(ir_b, -2, 2);
            IR_CHECK_INPUT(ir_c, -1, 2);
            _PyStackRef left;
            _PyStackRef right;
            _PyStackRef res;
            right = stack_pointer[-1];
            left = stack_pointer[-2];
            uint32_t counter = fb->counter;
            (void)counter;
            _PyFbHeader *feedback = ((_PyFbHeader *)fb);
            ADVANCE_ADAPTIVE_COUNTER(feedback);
            res = SPAM(left, right, oparg);
            stack_pointer[-2] = res;
            stack_pointer += -1;
            ASSERT_WITHIN_STACK_BOUNDS(__FILE__, __LINE__);
            DISPATCH();
        }
    """
        self.run_ir_test(input, output)
        fmt = self.formats(input)["OP"]
        self.assertEqual(fmt.nwords, 2)
        self.assertEqual((fmt.imm, fmt.fb), (ir_generator.LOC_D, ir_generator.LOC_F))

    def test_jump(self):
        input = """
        inst(OP, (cond --)) {
            int flag = PyStackRef_IsFalse(cond);
            DEAD(cond);
            JUMPBY(flag ? oparg : 0);
        }
    """
        fmt = self.formats(input)["OP"]
        self.assertTrue(fmt.jumps)
        self.assertEqual(fmt.roles[:2], [f"{ir_generator.ROLE_IN0} + 0",
                                         ir_generator.ROLE_IMM])

    def test_ext(self):
        # IR_EXT has no stack effect, oparg or entry and dispatches to the
        # next word without TARGET bookkeeping of its own: it is transparent.
        input = """
        ir_only inst(IR_EXT, ( -- )) {
            IR_EXTEND_FIELDS();
            PRE_DISPATCH_GOTO();
            DISPATCH_GOTO();
        }
    """
        output = """
        TARGET(IR_EXT) {
            #if _Py_TAIL_CALL_INTERP
            int opcode = IR_EXT;
            (void)(opcode);
            #endif
            IR_ENTRY(IR_EXT);
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(IR_EXT);
            IR_EXTEND_FIELDS();
            PRE_DISPATCH_GOTO();
            DISPATCH_GOTO();
        }
    """
        self.run_ir_test(input, output)
        self.assertTrue(self.formats(input)["IR_EXT"].transparent)

    def test_metadata(self):
        # IR-only handlers follow the opcodes in name order. An absorbed load
        # shares the format of the load and stands for it in co_code.
        input = """
        inst(LOAD, ( -- value)) {
            value = PyStackRef_DUP(GETLOCAL(oparg));
        }
        ir_only inst(IR_EXT, ( -- )) {
            IR_EXTEND_FIELDS();
            PRE_DISPATCH_GOTO();
            DISPATCH_GOTO();
        }
        ir_only inst(IR_ABSORBED_LOAD, ( -- value)) {
            value = PyStackRef_NULL;
        }
    """
        analysis = self.analyze(input)
        out = io.StringIO()
        ir_metadata_generator.generate_ir_metadata(
            [self.temp_input_filename], analysis, out)
        lines = [line.strip() for line in out.getvalue().splitlines()]
        load = ("{ .nwords = 1, .flags = 0, .roles = { _PyIR_ROLE_DST, "
                "_PyIR_ROLE_IMM, _PyIR_ROLE_NONE }, .imm = _PyIR_LOC_B, "
                ".fb = _PyIR_LOC_NONE, .ninputs = 0, .dst = 0, "
                ".borrow_mask = 0x0, .shadow = LOAD }")
        for expected in [
            "#define IR_ABSORBED_LOAD                               256",
            "#define IR_EXT                                         257",
            "#define IR_NUM_HANDLERS                                258",
            f"[LOAD] = {load},",
            f"[IR_ABSORBED_LOAD] = {load},",
            "[IR_EXT] = { .nwords = 1, .flags = _PyIR_TRANSPARENT, "
            ".roles = { _PyIR_ROLE_NONE, _PyIR_ROLE_NONE, _PyIR_ROLE_NONE }, "
            ".imm = _PyIR_LOC_NONE, .fb = _PyIR_LOC_NONE, .ninputs = 0, "
            ".dst = _PyIR_NO_DST, .borrow_mask = 0x0, .shadow = IR_EXT },",
            '[IR_EXT - 256] = "IR_EXT",',
        ]:
            self.assertIn(expected, lines)


if __name__ == "__main__":
    unittest.main()
