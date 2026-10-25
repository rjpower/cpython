/* The handlers of the register IR in call mode, for tier 0
 * (boil/design/05-tier0-jit.md).  Python/ceval.c includes this file in the
 * tail-call build with Py_TIER0, after the interpreter's own handlers.
 *
 * generated_ir_cases.c.h is compiled a second time with the handlers named
 * _TIER0_<name>.  Their signature is the tail-call one, so the machine code
 * that Python/tier0.c emits calls them as it would tail-call them.  Where
 * the interpreter's handler dispatches, a handler in call mode stores
 * next_instr and stack_pointer in the thread state (tier0 in
 * _PyThreadStateImpl) and returns the frame it would go on with, which the
 * frame's instr_ptr does not change: INSTRUMENTED_LINE reads the previous
 * instruction there.  The machine code goes on with its next word if the
 * frame is its own and next_instr is that word, and otherwise in the
 * machine code of the frame for next_instr or, failing that, through
 * _PyTier0_Continue().  Jumps to labels and predicted handlers
 * stay within call mode.  The result is NULL when an error leaves the entry
 * frame, as in the interpreter, and _PyTier0_INTERPRET marks a frame whose
 * next word only the interpreter runs.
 *
 * The interpreter's table sends five handlers through the hooks below: the
 * RESUME family and JUMP_BACKWARD heat the block and enter its machine
 * code, and RETURN_VALUE and YIELD_VALUE return to machine code in the
 * caller.
 */

#ifndef Py_TIER0
#  error "this file is the call mode of tier 0 (Py_TIER0)"
#endif
#ifdef Py_STATS
#  error "tier 0 does not pass lastopcode"
#endif

#include "pycore_tier0.h"

#define TIER0_STATE(TSTATE) (((_PyThreadStateImpl *)(TSTATE))->tier0)

#pragma push_macro("TARGET")
#pragma push_macro("LABEL")
#pragma push_macro("JUMP_TO_LABEL")
#pragma push_macro("JUMP_TO_PREDICTED")
#pragma push_macro("DISPATCH_GOTO")

#undef TARGET
#define TARGET(op) Py_NO_INLINE PyObject *Py_PRESERVE_NONE_CC _TIER0_##op(TAIL_CALL_PARAMS)
#undef LABEL
#define LABEL(name) TARGET(name)
#undef JUMP_TO_LABEL
#define JUMP_TO_LABEL(name) \
    do { \
        Py_MUSTTAIL return (_TIER0_##name)(TAIL_CALL_ARGS); \
    } while (0)
#undef JUMP_TO_PREDICTED
#define JUMP_TO_PREDICTED(name) \
    do { \
        Py_MUSTTAIL return (_TIER0_##name)(frame, stack_pointer, tstate, \
                                           this_instr, TAIL_CALL_OPERAND_ARGS); \
    } while (0)
/* The handler and fields that DISPATCH() decodes go unused: the machine code
 * reads them itself.  The volatile store keeps the two stores apart.  Where
 * a handler copies both from a new frame, clang would otherwise copy them
 * with one 16-byte load and store, and neither the load from the frame nor
 * the machine code's 8-byte load of stack_pointer could take its value
 * from the store before it: each would wait for the store to complete. */
#undef DISPATCH_GOTO
#define DISPATCH_GOTO() \
    do { \
        (void)opcode; (void)ir_a; (void)ir_b; (void)ir_c; \
        TIER0_STATE(tstate).next_instr = next_instr; \
        *(_PyStackRef *volatile *)&TIER0_STATE(tstate).stack_pointer = \
            stack_pointer; \
        return (PyObject *)frame; \
    } while (0)

#include "generated_ir_cases.c.h"

#pragma pop_macro("DISPATCH_GOTO")
#pragma pop_macro("JUMP_TO_PREDICTED")
#pragma pop_macro("JUMP_TO_LABEL")
#pragma pop_macro("LABEL")
#pragma pop_macro("TARGET")

/* The handler in _PyTier0_CallTable of words that tier 0 leaves to the
 * interpreter: instrumented and transparent handlers, IR_LOWER. */
static PyObject *Py_PRESERVE_NONE_CC
_TIER0_LEAVE(TAIL_CALL_PARAMS)
{
    TIER0_STATE(tstate).next_instr = next_instr;
    TIER0_STATE(tstate).stack_pointer = stack_pointer;
    return (PyObject *)((uintptr_t)frame | _PyTier0_INTERPRET);
}

/* Go on with the result of a handler in call mode, which frame holds: in
 * the frame's machine code if it has some for its next word, else in the
 * interpreter.  The next word and the stack pointer are in the thread
 * state; the other parameters are ignored. */
static PyObject *Py_PRESERVE_NONE_CC
_PyTier0_Continue(TAIL_CALL_PARAMS)
{
    uintptr_t bits = (uintptr_t)frame;
    if (bits == 0) {
        return NULL;
    }
    frame = (_PyInterpreterFrame *)(bits & ~(uintptr_t)_PyTier0_INTERPRET);
    next_instr = TIER0_STATE(tstate).next_instr;
    stack_pointer = TIER0_STATE(tstate).stack_pointer;
    if (!(bits & _PyTier0_INTERPRET)
        && frame->owner != FRAME_OWNED_BY_INTERPRETER)
    {
        _PyIRBlock *block = _PyFrame_GetCode(frame)->_co_ir;
        _PyTier0Code *code = _PyTier0_ValidCode(block);
        // A frame that has not started is at the lowering stub, and one
        // that IR_REMAP has yet to move is in the block that block
        // replaced: both are outside block's words.
        uintptr_t at = (uintptr_t)next_instr - (uintptr_t)block->words;
        if (code != NULL && at < block->nwords * sizeof(_PyIRWord)) {
            int32_t offset = code->offsets[at / sizeof(_PyIRWord)];
            if (offset != 0) {
                Py_MUSTTAIL return ((py_tail_call_funcptr)code->entry)(
                    frame, stack_pointer, tstate, next_instr,
                    (uint32_t)offset, 0, 0);
            }
        }
    }
    _PyIRWord word = *next_instr;
    Py_MUSTTAIL return instruction_funcptr_handler_table[IR_HANDLER(word)](
        frame, stack_pointer, tstate, next_instr,
        IR_A(word), IR_B(word), IR_C(word));
}

const _PyTier0Runtime _PyTier0_Runtime = {
    .call_table = _PyTier0_CallTable,
    .resume = (const void *)_PyTier0_Continue,
};

/* The valid machine code of the frame's block after heating it by heat,
 * compiling the block once it is hot, or NULL. */
static inline _PyTier0Code *
tier0_heat(_PyInterpreterFrame *frame, int heat)
{
    PyCodeObject *co = _PyFrame_GetCode(frame);
    _PyIRBlock *block = co->_co_ir;
    _PyTier0Code *code = _PyTier0_ValidCode(block);
    if (code == NULL) {
        block->tier0_heat += heat;
        if (block->tier0_heat >= _PyTier0_THRESHOLD) {
            code = _PyTier0_Compile(co);
        }
    }
    return code;
}

/* Enter CODE at next_instr if it has code for that word */
#define TIER0_ENTER(CODE) \
    do { \
        _PyIRBlock *block_ = _PyFrame_GetCode(frame)->_co_ir; \
        int32_t offset_ = (CODE)->offsets[next_instr - block_->words]; \
        if (offset_ != 0) { \
            Py_MUSTTAIL return ((py_tail_call_funcptr)(CODE)->entry)( \
                frame, stack_pointer, tstate, next_instr, \
                (uint32_t)offset_, 0, 0); \
        } \
    } while (0)

/* A hook that heats the block by HEAT and enters machine code at the
 * instruction, which the machine code then runs in call mode. */
#define TIER0_HEATING_HOOK(NAME, HEAT) \
    static PyObject *Py_PRESERVE_NONE_CC \
    _TIER0_HOOK_##NAME(TAIL_CALL_PARAMS) \
    { \
        _PyTier0Code *code = tier0_heat(frame, (HEAT)); \
        if (code != NULL) { \
            TIER0_ENTER(code); \
        } \
        Py_MUSTTAIL return _TAIL_CALL_##NAME(TAIL_CALL_ARGS); \
    }

TIER0_HEATING_HOOK(RESUME, _PyTier0_CALL_HEAT)
TIER0_HEATING_HOOK(RESUME_CHECK, _PyTier0_CALL_HEAT)
TIER0_HEATING_HOOK(JUMP_BACKWARD, 1)

/* A hook that runs the instruction in call mode, so that the frame it goes
 * on with, its caller's, continues in machine code if it has some. */
#define TIER0_RETURNING_HOOK(NAME) \
    static PyObject *Py_PRESERVE_NONE_CC \
    _TIER0_HOOK_##NAME(TAIL_CALL_PARAMS) \
    { \
        PyObject *next = _TIER0_##NAME(TAIL_CALL_ARGS); \
        Py_MUSTTAIL return _PyTier0_Continue( \
            (_PyInterpreterFrame *)next, NULL, tstate, NULL, \
            TAIL_CALL_NO_OPERANDS); \
    }

TIER0_RETURNING_HOOK(RETURN_VALUE)
TIER0_RETURNING_HOOK(YIELD_VALUE)
