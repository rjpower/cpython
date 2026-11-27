/* The out-of-line steps of the interpreter, which return the frame and next
 * word to run (a _PyIRTransfer) or no word for an exit that the activation
 * describes, and the completion arms of gen_return, which return a status.
 * The evaluator (Python/ceval.c) calls the steps and dispatches the result. */

#include "ceval_shared.h"

#undef DISPATCH
#undef PROCESS_ERROR_AT
#define DISPATCH() return (_PyIRTransfer){frame, next_instr}
#define PROCESS_ERROR_AT(ingress, checkpoint, origin) \
    do { \
        _PyEvalActivation *ir_exit = IR_ACTIVATION(); \
        ir_exit->exit_kind = _PyIR_EXIT_RAISE; \
        ir_exit->exit_next = (origin); \
        ir_exit->exit_entry = \
            _PyIR_ERROR_ENTRY(_PyIR_ERROR_INGRESS_##ingress, checkpoint); \
        return (_PyIRTransfer){frame, NULL}; \
    } while (0)

/* The event of an observed branch, jump or loop step, after the effect
 * (rule 12).  It is out of the evaluator, where the observers only call
 * it.  Its result is the word to run (the callbacks can redirect
 * it) or no word for an error. */
Py_NO_INLINE _PyIRTransfer
_PyEval_ir_event_edge(PyThreadState *tstate, _PyInterpreterFrame *frame,
              _PyInstr *this_instr, _PyInstr *source, _PyInstr *next,
              int event, int edge)
{
    frame->return_pc = next;
    _PyIRObserverResult result = _PyIR_ObserveCommittedBranch(
        tstate, frame, this_instr, source, next, event, edge);
    PyObject *exception = result.status < 0 ? PyErr_GetRaisedException() : NULL;
    assert(result.status >= 0 || exception != NULL);
    frame->instr_ptr = result.next;
    if (result.status < 0) {
        PyErr_SetRaisedException(exception);
        PROCESS_ERROR_AT(RAISE, 0, result.next);
    }
    return (_PyIRTransfer){frame, result.next};
}

#include "ceval_generator.h"


#undef DISPATCH
#undef PROCESS_ERROR_AT

/* The completion arms (Python/register_ops), which gen_return runs in the
 * consumer's frame: each returns the status that the routine reads, 0 for a
 * word that completed and goes on at the next one, _PyIR_BRANCH at its
 * target, _PyIR_REDIRECT when the frame's control moved while it ran (the
 * word's successor is frame->return_pc), or _PyIR_RAISED. */
#undef IR_REDIRECT
#undef IR_BRANCH
#undef ROP_TAIL_CLOSE
#undef ROP_TAIL_PARK_FIRST
#undef ROP_TAIL_PARK
#undef ROP_TAIL_BRANCH
#define DISPATCH() return 0
#define PROCESS_ERROR_AT(ingress, checkpoint, origin) \
    do { \
        _PyEvalActivation *ir_exit = IR_ACTIVATION(); \
        ir_exit->exit_kind = _PyIR_EXIT_RAISE; \
        ir_exit->exit_next = (origin); \
        ir_exit->exit_entry = \
            _PyIR_ERROR_ENTRY(_PyIR_ERROR_INGRESS_##ingress, checkpoint); \
        return _PyIR_RAISED; \
    } while (0)
#define IR_BRANCH(TARGET) return _PyIR_BRANCH
#define IR_REDIRECT() do { \
        if (frame->pending_redirect) { \
            _PyFrame_CompleteRedirect(frame); \
        } \
        return _PyIR_REDIRECT; \
    } while (0)
/* A tail close happens where it stands; the parking blocks are never entered */
#define ROP_TAIL_CLOSE(V, COLD) \
    do { \
        PyValue_XCLOSE(V); \
        if (0) { \
            goto COLD; \
        } \
    } while (0)
#define ROP_TAIL_PARK_FIRST(V) ((void)(V))
#define ROP_TAIL_PARK(V) ((void)(V))
#define ROP_TAIL_BRANCH(T) IR_BRANCH(T)

#include "generated_rop_completions.c.h"
