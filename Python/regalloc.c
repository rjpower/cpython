/* Slot assignment for the register IR (boil/design/06-architecture-v2.md,
 * section 2.3).
 *
 * The assembler (Python/assemble.c) runs this over the instruction sequence
 * that the flowgraph optimiser leaves: stack instructions with jump targets
 * as instruction indices.  The stack position at depth d becomes slot
 * co_nlocalsplus + d, which locals, cells and free variables precede, so
 * every handler keeps its stack-machine body and temporaries agree on their
 * slots at every jump target without a dataflow pass.
 *
 * Nothing is folded yet: LOAD_FAST, STORE_FAST, COPY, SWAP and POP_TOP keep
 * a word each.  _PyRegAlloc_Slots() is the mapping that folding will
 * extend.
 */

#include "Python.h"

#include "pycore_flowgraph.h"
#include "pycore_instruction_sequence.h"
#include "pycore_ir.h"
#include "pycore_opcode_metadata.h"   // _PyOpcode_num_popped()
#include "pycore_opcode_utils.h"      // IS_SCOPE_EXIT_OPCODE()


static int
push_depth(_PyInstructionSequence *instrs, int *depths, int *stack, int *sp,
           int i, int depth)
{
    if (i < 0 || i >= instrs->s_used) {
        PyErr_SetString(PyExc_SystemError, "jump out of the code");
        return -1;
    }
    if (depths[i] < 0) {
        depths[i] = depth;
        stack[(*sp)++] = i;
    }
    else if (depths[i] != depth) {
        PyErr_Format(PyExc_SystemError,
                     "inconsistent stack depth at instruction %d", i);
        return -1;
    }
    return 0;
}

int
_PyRegAlloc_Depths(_PyInstructionSequence *instrs, int *depths)
{
    int n = instrs->s_used;
    for (int i = 0; i < n; i++) {
        depths[i] = -1;
    }
    if (n == 0) {
        return 0;
    }
    int *stack = PyMem_Malloc(n * sizeof(int));
    if (stack == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    int sp = 0;
    int err = push_depth(instrs, depths, stack, &sp, 0, 0);
    // Exception handlers start at the depth that the table records
    for (int i = 0; err == 0 && i < n; i++) {
        _PyExceptHandlerInfo *h = &instrs->s_instrs[i].i_except_handler_info;
        if (h->h_label >= 0 && h->h_startdepth >= 0) {
            err = push_depth(instrs, depths, stack, &sp, h->h_label,
                             h->h_startdepth);
        }
    }
    while (err == 0 && sp > 0) {
        int i = stack[--sp];
        _PyInstruction *instr = &instrs->s_instrs[i];
        int opcode = instr->i_opcode;
        int popped = _PyOpcode_num_popped(opcode, instr->i_oparg);
        int pushed = _PyOpcode_num_pushed(opcode, instr->i_oparg);
        if (popped < 0 || pushed < 0 || depths[i] < popped) {
            PyErr_Format(PyExc_SystemError,
                         "stack underflow at instruction %d", i);
            err = -1;
            break;
        }
        int depth = depths[i] - popped + pushed;
        // END_ASYNC_FOR names its SEND but does not jump there
        if (OPCODE_HAS_JUMP(opcode) && opcode != END_ASYNC_FOR) {
            err = push_depth(instrs, depths, stack, &sp, instr->i_target, depth);
            if (err) {
                break;
            }
        }
        if (!IS_SCOPE_EXIT_OPCODE(opcode) && !IS_UNCONDITIONAL_JUMP_OPCODE(opcode)
            && i + 1 < n)
        {
            err = push_depth(instrs, depths, stack, &sp, i + 1, depth);
        }
    }
    PyMem_Free(stack);
    return err;
}

void
_PyRegAlloc_Slots(const _PyIRFormat *fmt, int nlocalsplus, int depth,
                  int popped, _PyRegSlots *slots)
{
    int top = nlocalsplus + depth;
    int base = top - fmt->ninputs;
    slots->dst = fmt->flags & _PyIR_WINDOW
        ? top - popped + fmt->dst : top + fmt->dst;
    slots->base = top - popped;
    for (int k = 0; k < 4; k++) {
        slots->in[k] = base + k;
    }
}
