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
 * _PyRegAlloc_Fold() then decides which instructions keep no word, and
 * which input fields name a local rather than a temporary.
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


/* Folding (boil/design/06-architecture-v2.md, sections 2.3 and 11).
 *
 * A LOAD_FAST_BORROW has no word when the next word consumes it: that word
 * reads the local through its input field, borrowed, as LOAD_FAST_BORROW
 * would have pushed it.  The value keeps its stack position, so every
 * temporary stays in its canonical slot, and the canonical slot of a folded
 * load holds whatever was left there.  Handlers still keep stack_pointer,
 * which lags behind by the folded loads, so a handler that may read a local
 * this way sets stack_pointer from its fields on entry (the SYNC_ modes of
 * Tools/cases_generator/ir_generator.py), and writes the borrow to the
 * canonical slot, or clears it, before code that reads the stack runs.
 *
 * Only the next word may consume a folded load.  Any other word running
 * in between, or an event or trace function at its start, could see the
 * stale slot under stack_pointer: so a load that is not read by the word
 * after it keeps its word, and so does every load below it.  This makes
 * liveness trivial: no word runs between the load and its consumer, so
 * nothing can store to the local or delete it in between.  Only loads that
 * Python/flowgraph.c made borrowed are folded; the others own their value,
 * because it outlives the block, is stored, or the local is killed while
 * the value is on the stack.  A load keeps its word too if it starts a line
 * that its consumer does not, so that line events stay where they were.
 *
 * A STORE_FAST has no word when the word before it pushes the value alone,
 * on top, and its format has _PyIR_STORE_DST: that word's dst names the
 * local, and its handler moves the result there and releases the old
 * value, as STORE_FAST would (IR_STORE_RESULT).  Nothing runs between the
 * two, so this needs no liveness either.  The value a store takes is never
 * a borrow: Python/flowgraph.c makes no load that a store consumes
 * borrowed, and formats whose result may alias a borrowed input do not
 * have _PyIR_STORE_DST.
 */

/* The instructions that start a basic block: jump targets, exception
 * handlers, and those after a scope exit or an unconditional jump. */
static uint8_t *
block_starts(_PyInstructionSequence *instrs)
{
    int n = instrs->s_used;
    uint8_t *starts = PyMem_Calloc(n + 1, 1);
    if (starts == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    for (int i = 0; i < n; i++) {
        _PyInstruction *instr = &instrs->s_instrs[i];
        int opcode = instr->i_opcode;
        if (OPCODE_HAS_JUMP(opcode) && instr->i_target >= 0
            && instr->i_target < n)
        {
            starts[instr->i_target] = 1;
        }
        int h = instr->i_except_handler_info.h_label;
        if (h >= 0 && h < n) {
            starts[h] = 1;
        }
        if (IS_SCOPE_EXIT_OPCODE(opcode) || IS_UNCONDITIONAL_JUMP_OPCODE(opcode)) {
            starts[i + 1] = 1;
        }
    }
    return starts;
}

/* Fold into instruction i the loads run[0 .. nrun - 1], which push the top
 * nrun values that i finds on the stack, from the top down for as long as
 * i reads each through a field that may name a local. */
static void
fold_loads(_PyInstructionSequence *instrs, const int *depths,
           const int *handlers, _PyRegFold *folds, int i,
           const int *run, int nrun)
{
    const _PyIRFormat *fmt = _PyIR_FormatOf(handlers[i]);
    if ((fmt->flags & _PyIR_WINDOW) || fmt->borrow_mask == 0) {
        return;
    }
    _PyInstruction *consumer = &instrs->s_instrs[i];
    int first_input = depths[i] - fmt->ninputs;
    for (int r = nrun - 1; r >= 0; r--) {
        int k = depths[i] - nrun + r - first_input;
        if (k < 0 || !(fmt->borrow_mask & (1 << k))) {
            return;
        }
        _PyInstruction *load = &instrs->s_instrs[run[r]];
        if (load->i_loc.lineno != consumer->i_loc.lineno) {
            return;
        }
        // Only other folded loads lie between the load and its consumer, so
        // nothing writes the local in between
        assert(run[r] == i - nrun + r);
        for (int j = run[r] + 1; j < i; j++) {
            assert(instrs->s_instrs[j].i_opcode == LOAD_FAST_BORROW);
        }
        folds[run[r]].folded = 1;
        folds[i].in[k] = load->i_oparg;
    }
}

/* Fold STORE_FAST i into the word before it, if that pushes the value
 * alone; see above. */
static void
fold_store(_PyInstructionSequence *instrs, const int *depths,
           const int *handlers, _PyRegFold *folds, int i)
{
    int p = i - 1;
    _PyInstruction *store = &instrs->s_instrs[i];
    _PyInstruction *producer = &instrs->s_instrs[p];
    if (depths[p] < 0 || folds[p].folded || folds[i].in[0] >= 0
        || producer->i_opcode == LOAD_FAST_BORROW
        || producer->i_loc.lineno != store->i_loc.lineno)
    {
        return;
    }
    const _PyIRFormat *fmt = _PyIR_FormatOf(handlers[p]);
    if (!(fmt->flags & _PyIR_STORE_DST)) {
        return;
    }
    int popped = _PyOpcode_num_popped(producer->i_opcode, producer->i_oparg);
    int dst = fmt->flags & _PyIR_WINDOW
        ? depths[p] - popped + fmt->dst : depths[p] + fmt->dst;
    // The depth after the producer is the store's: the value is on top
    if (dst != depths[i] - 1) {
        return;
    }
    // Nothing lies between the producer and the store, so nothing reads or
    // writes the local in between, and no other fold sees the store: loads
    // fold only into the next word, which the store was not.
    assert(folds[p].dst < 0);
    folds[i].folded = 1;
    folds[p].dst = store->i_oparg;
}

int
_PyRegAlloc_Fold(_PyInstructionSequence *instrs, const int *depths,
                 const int *handlers, _PyRegFold *folds)
{
    int n = instrs->s_used;
    for (int i = 0; i < n; i++) {
        folds[i].folded = 0;
        for (int k = 0; k < 4; k++) {
            folds[i].in[k] = -1;
        }
        folds[i].dst = -1;
    }
    if (n == 0) {
        return 0;
    }
    uint8_t *starts = block_starts(instrs);
    int *run = PyMem_Malloc(n * sizeof(int));
    if (starts == NULL || run == NULL) {
        PyMem_Free(starts);
        PyMem_Free(run);
        PyErr_NoMemory();
        return -1;
    }
    // The loads since the last word, which a block start ends: they keep
    // their words, as every load does unless fold_loads() folds it.
    int nrun = 0;
    for (int i = 0; i < n; i++) {
        _PyInstruction *instr = &instrs->s_instrs[i];
        if (starts[i] || depths[i] < 0) {
            nrun = 0;
        }
        if (depths[i] < 0) {
            continue;
        }
        if (instr->i_opcode == LOAD_FAST_BORROW) {
            run[nrun++] = i;
            continue;
        }
        if (nrun > 0) {
            fold_loads(instrs, depths, handlers, folds, i, run, nrun);
            nrun = 0;
        }
    }
    for (int i = 1; i < n; i++) {
        if (instrs->s_instrs[i].i_opcode == STORE_FAST && !starts[i]
            && depths[i] >= 0)
        {
            fold_store(instrs, depths, handlers, folds, i);
        }
    }
    PyMem_Free(starts);
    PyMem_Free(run);
    return 0;
}
