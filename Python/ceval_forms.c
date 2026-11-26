/* The function forms of the register instruction arms, of the VM routines
 * and of the labels, and the whole-word completions: generated and
 * written for the baseline JIT and for -X ir_forms.  The evaluator
 * (Python/ceval.c) does not call them, and is smaller for it. */

#include "ceval_shared.h"

/* The function forms (tiers87, C2).  Every handler's body is also a function
 * of its own, ir_run_X(tstate, frame, next_instr, ir_word), and every VM
 * routine (Python/ceval_routines.h) and DSL label is a function ir_NAME; each
 * returns a _PyIRTransfer.  The evaluator's stub of an outlined handler calls
 * its form (IR_RUN_OUTLINED()); the function-form executor,
 * ir_execute_forms(), calls them all, as compiled code will.  Here the
 * transfer macros are returns: the next word is returned; a deoptimisation,
 * a frame entry, a VM routine and a handler run on the same word are tail
 * calls; an error or the end of the activation returns no word and is
 * described in the activation, for the caller to resolve. */
#pragma push_macro("DISPATCH")
#undef DISPATCH
#pragma push_macro("DISPATCH_SAME_OPARG")
#undef DISPATCH_SAME_OPARG
#pragma push_macro("DISPATCH_GOTO")
#undef DISPATCH_GOTO
#pragma push_macro("DISPATCH_INLINED")
#undef DISPATCH_INLINED
#pragma push_macro("PROCESS_ERROR_AT")
#undef PROCESS_ERROR_AT
#pragma push_macro("IR_EXIT")
#undef IR_EXIT
#pragma push_macro("JUMP_TO_PREDICTED")
#undef JUMP_TO_PREDICTED
#pragma push_macro("IR_OBSERVE_EDGE")
#undef IR_OBSERVE_EDGE
#pragma push_macro("IR_HOLD_NEXT")
#undef IR_HOLD_NEXT
#pragma push_macro("IR_STEP_DONE")
#undef IR_STEP_DONE
#pragma push_macro("IR_REPORT_EDGE")
#undef IR_REPORT_EDGE
#pragma push_macro("IR_GEN_RETURN")
#undef IR_GEN_RETURN
#pragma push_macro("IR_ROUTINE")
#undef IR_ROUTINE
#pragma push_macro("IR_HOT_ROUTINE")
#undef IR_HOT_ROUTINE
#pragma push_macro("GEN_DISPATCH")
#undef GEN_DISPATCH
#pragma push_macro("GEN_STEP_JUMP")
#undef GEN_STEP_JUMP
#pragma push_macro("GEN_RAISED")
#undef GEN_RAISED
#pragma push_macro("IR_JIT_EVENT")
#undef IR_JIT_EVENT
#pragma push_macro("IR_JIT_EVENT_AT")
#undef IR_JIT_EVENT_AT
#pragma push_macro("IR_JIT_RESUME")
#undef IR_JIT_RESUME
#pragma push_macro("IR_STORE_RETURN")
#undef IR_STORE_RETURN
#pragma push_macro("IR_PIN_DEPARTING")
#undef IR_PIN_DEPARTING
#pragma push_macro("IR_ERROR_SWITCH")
#undef IR_ERROR_SWITCH
#pragma push_macro("IR_COMPLETIONS")
#undef IR_COMPLETIONS
/* The forms are compiled once: a word runs the form of the table that its
 * frame's code asks for, the observer of its family in observed code
 * (ir_observing_forms).  The switch word only takes the frame's entry off
 * the switch list (_PyEval_SwitchLoop()). */
static const _PyIRForm ir_handler_forms[512];
static const _PyIRForm ir_observing_forms[512];
static inline bool
ir_frame_observed(_PyInterpreterFrame *frame)
{
    return frame->owner != FRAME_OWNED_BY_INTERPRETER &&
           _PyCode_IsObserved(_PyFrame_GetCode(frame));
}
#define IR_FORMS(FRAME) (ir_frame_observed(FRAME) ? ir_observing_forms : ir_handler_forms)
#define IR_COMPLETIONS(SITE) \
    ((ir_frame_observed(frame) && \
      (_PyFrame_Observed(frame, (SITE)) & _PyIR_OBSERVE_EVENTS)) \
        ? _PyRop_ObservingCompletions : _PyRop_Completions)
#define IR_ERROR_SWITCH() ((void)0)
#define DISPATCH() return (_PyIRTransfer){frame, next_instr}
#define DISPATCH_SAME_OPARG() return (_PyIRTransfer){frame, next_instr}
#define DISPATCH_GOTO() \
    do { \
        Py_MUSTTAIL return IR_FORMS(frame)[opcode](tstate, frame, next_instr, ir_word); \
    } while (0)
#define DISPATCH_INLINED(NEW_FRAME) \
    do { \
        assert(!IS_PEP523_HOOKED(tstate)); \
        assert((NEW_FRAME)->previous == frame || \
               (NEW_FRAME)->previous->previous == frame); \
        frame = tstate->current_frame = (NEW_FRAME); \
        CALL_STAT_INC(inlined_py_calls); \
        Py_MUSTTAIL return ir_start_frame(tstate, frame, next_instr); \
    } while (0)
#define PROCESS_ERROR_AT(ingress, checkpoint, origin) \
    do { \
        _PyEvalActivation *ir_exit = IR_ACTIVATION(); \
        ir_exit->exit_kind = _PyIR_EXIT_RAISE; \
        ir_exit->exit_next = (origin); \
        ir_exit->exit_entry = \
            _PyIR_ERROR_ENTRY(_PyIR_ERROR_INGRESS_##ingress, checkpoint); \
        return (_PyIRTransfer){frame, NULL}; \
    } while (0)
#define IR_EXIT(V) \
    do { \
        _PyEvalActivation *ir_exit = IR_ACTIVATION(); \
        ir_exit->exit_kind = _PyIR_EXIT_VALUE; \
        ir_exit->exit_value = (V); \
        return (_PyIRTransfer){frame, NULL}; \
    } while (0)
#define JUMP_TO_PREDICTED(name) \
    do { \
        Py_MUSTTAIL return ir_run_##name(tstate, frame, this_instr, ir_word); \
    } while (0)
#define IR_HOLD_NEXT() ((void)0)
#define IR_STEP_DONE(STEP) return (STEP)
#define IR_REPORT_EDGE(TAKEN) (IR_ACTIVATION()->edge = (TAKEN))
#define IR_OBSERVE_EDGE(SOURCE, NEXT, EVENT, EDGE) \
    return _PyEval_ir_event_edge(tstate, frame, this_instr, (SOURCE), (NEXT), (EVENT), (EDGE))
#define IR_GEN_RETURN(V, DYING, EMPTY) \
    do { \
        Py_MUSTTAIL return ir_gen_return(tstate, frame, next_instr, (V), \
                                         (DYING), (EMPTY)); \
    } while (0)
#define IR_ROUTINE(NAME, ...) \
    static Py_NO_INLINE _PyIRTransfer \
    ir_##NAME(PyThreadState *tstate, _PyInterpreterFrame *frame, \
              _PyInstr *next_instr, __VA_ARGS__)
#define IR_HOT_ROUTINE IR_ROUTINE
#define GEN_DISPATCH() return (_PyIRTransfer){frame, next_instr}
#define GEN_STEP_JUMP(NAME, F, IP, V) \
    do { \
        Py_MUSTTAIL return _PyEval_ir_##NAME(tstate, (F), (IP), _PyValue_ToTransferWord(V)); \
    } while (0)
/* The error that a completion arm reported in the activation, for the
 * caller to resolve */
#define GEN_RAISED() return (_PyIRTransfer){frame, NULL}
/* Compiled code runs the forms: it is in its unit already, and leaves it
 * at every frame switch for the driver, which finds the next frame's unit
 * itself.  So a return store tests only a pending redirect, not the
 * frame's _PyFrame_JIT_RESUME flag. */
#define IR_JIT_EVENT() ((void)0)
#define IR_JIT_EVENT_AT(NEXT) ((void)(NEXT))
#define IR_JIT_RESUME() ((void)0)
/* The pin's release status is not needed: every pinned arm runs Python, so
 * the form's continuation tests RETIRED when the word stays in its frame */
#define IR_PIN_DEPARTING() ((void)_PyJit_PinDeparting(tstate, frame))
#define IR_STORE_RETURN(V) \
    do { \
        assert(_PyValue_IsHome(V)); \
        if (frame->pending_redirect) { \
            _PyFrame_SetReturnValueRare(frame, (V)); \
            next_instr = frame->return_pc; \
        } \
        else { \
            _PyFrame_PublishReturnSlot(frame, (V)); \
        } \
    } while (0)

/* The event of an instrumented branch, jump or loop step, after the effect
 * (rule 12).  It is out of the evaluator, where the instrumented handlers
 * only call it.  Its result is the word to run (the callbacks can redirect
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

/* The events that precede the effect of an instrumented twin (rule 12).  Each
 * leaves the state as the twin's handler left it at the event: an error
 * unwinds with the operand as the event took it. */
static Py_NO_INLINE _PyIRTransfer
ir_event_yield(PyThreadState *tstate, _PyInterpreterFrame *frame,
               _PyInstr *next_instr, _PyIRWord ir_word)
{
    _PyInstr *this_instr = next_instr;
    frame->instr_ptr = this_instr;
    PyValue val = IR_SLOT(IR_B(ir_word));
    int err = _Py_call_instrumentation_arg(
        tstate, PY_MONITORING_EVENT_PY_YIELD, frame, this_instr, val);
    if (err) {
        PROCESS_ERROR_AT(RAISE, 0, this_instr);
    }
    /* The callbacks can have jumped. */
    return (_PyIRTransfer){frame, frame->instr_ptr};
}

static Py_NO_INLINE _PyIRTransfer
ir_event_end_for(PyThreadState *tstate, _PyInterpreterFrame *frame,
                 _PyInstr *next_instr, _PyIRWord ir_word)
{
    _PyInstr *this_instr = next_instr;
    PyValueBorrow value = IR_VIEW(IR_C(ir_word));
    PyValue receiver = IR_SLOT(IR_A(ir_word));
    if (PyValue_GenCheck(receiver)) {
        int err = monitor_stop_iteration(tstate, frame, this_instr, value);
        if (err) {
            PROCESS_ERROR_AT(RAISE, 0, this_instr);
        }
    }
    return (_PyIRTransfer){frame, this_instr};
}


#include "ceval_generator.h"

/* The routines and handlers are printed for the evaluator too, whose
 * bindings of the transfer macros read some locals that the function forms'
 * do not (an `int opcode` before a dispatch, a local that only a dispatch
 * keeps live). */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
/* gen_return runs the completion of the word that entered the generator; the
 * table is generated below with the register instruction arms */
#include "ceval_routines.h"

static _PyIRTransfer ir_run_IR_OPERANDS(PyThreadState *, _PyInterpreterFrame *,
                                        _PyInstr *, _PyIRWord);
static _PyIRTransfer ir_run_UNKNOWN_OPCODE(PyThreadState *, _PyInterpreterFrame *,
                                           _PyInstr *, _PyIRWord);
static _PyIRTransfer ir_start_frame(PyThreadState *, _PyInterpreterFrame *,
                                    _PyInstr *);
static _PyIRTransfer ir_throw_error(PyThreadState *, _PyInterpreterFrame *,
                                    _PyInstr *);
static _PyIRTransfer
ir_run_switch_loop(PyThreadState *tstate, _PyInterpreterFrame *frame,
                   _PyInstr *next_instr, _PyIRWord ir_word)
{
    return (_PyIRTransfer){frame, _PyEval_SwitchLoop(tstate, frame)};
}

#define IR_FUNCTION_FORMS
#include "ceval_labels.h"
#undef IR_FUNCTION_FORMS
/* The forms of the register instruction arms (Python/register_ops), the
 * table of the forms, and the whole-word completion stubs with the plan of
 * each arm */
#include "generated_rop_forms.c.h"
#include "generated_rop_stubs.c.h"
#pragma GCC diagnostic pop

#ifdef _PY_JIT_SUPPORTED
const void *const *
_PyEval_HandlerForms(void)
{
    return (const void *const *)ir_handler_forms;
}

/* The raise exit of a baseline JIT template (Python/jit_template.c): its
 * handler's PROCESS_ERROR_AT(RAISE, 0, this_instr), where the template
 * published this_instr as frame->instr_ptr.  Machine code calls it and
 * takes the transfer in rax:rdx. */
static _PyIRTransfer
jit_raise_exit(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    _PyInstr *this_instr = frame->instr_ptr;
    PROCESS_ERROR_AT(RAISE, 0, this_instr);
}

const void *const _PyEval_JitRaiseExit = (const void *)jit_raise_exit;
#endif

/* The LINE or INSTRUCTION event before a word moved the frame (status 1) or
 * raised (-1), as LABEL_observe_word (Python/ceval_evaluator.h) resolves it */
static _PyIRTransfer
ir_location_exit(PyThreadState *tstate, _PyInterpreterFrame *frame, int status)
{
    if (status < 0) {
        PROCESS_ERROR_AT(RAISE, 0, frame->instr_ptr);
    }
    if (frame->pending_redirect) {
        _PyFrame_CompleteRedirect(frame);
    }
    return (_PyIRTransfer){frame, frame->return_pc};
}

/* The function-form executor (-X ir_forms), a test of the forms: runs the
 * activation's frames by calling each word's function form, as compiled
 * code does, in place of ir_evaluate().  A form that returns no word has
 * raised, which the error routine resolves, or has ended the activation.
 * In observed code it first runs what monitoring observes at the word, as
 * the observing evaluator does. */
PyValue
_PyEval_ExecuteForms(PyThreadState *tstate, _PyInterpreterFrame *frame, int throwflag)
{
    _PyEvalActivation *activation = IR_ACTIVATION();
    _PyIRTransfer t = throwflag ? ir_throw_error(tstate, frame, frame->instr_ptr)
                                : ir_start_frame(tstate, frame, frame->instr_ptr);
    for (;;) {
        if (t.next == NULL) {
            if (activation->exit_kind == _PyIR_EXIT_VALUE) {
                return activation->exit_value;
            }
            assert(activation->exit_kind == _PyIR_EXIT_RAISE);
            t = ir_process_error(tstate, t.frame, NULL, activation->exit_next,
                                 activation->exit_entry);
            continue;
        }
        frame = t.frame;
        _PyInstr *next_instr = t.next;
        _PyIRWord ir_word;
        int opcode;
        NEXTOPARG();
        PRE_DISPATCH_GOTO();
        if (opcode == IR_H_SWITCH || !ir_frame_observed(frame)) {
            t = ir_handler_forms[opcode](tstate, frame, next_instr, ir_word);
            continue;
        }
        uint8_t observed = _PyFrame_Observed(frame, next_instr);
        if (observed & (_PyIR_OBSERVE_LINE | _PyIR_OBSERVE_INSTRUCTION)) {
            int status = _PyEval_LocationEvents(tstate, frame, next_instr);
            if (status != 0) {
                t = ir_location_exit(tstate, frame, status);
                continue;
            }
            observed = _PyFrame_Observed(frame, next_instr);
            NEXTOPARG();
        }
        if (observed & _PyIR_OBSERVE_EVENTS) {
            opcode = _PyIR_ObservingHandler[opcode];
        }
        t = ir_observing_forms[opcode](tstate, frame, next_instr, ir_word);
    }
}
#pragma pop_macro("DISPATCH")
#pragma pop_macro("DISPATCH_SAME_OPARG")
#pragma pop_macro("DISPATCH_GOTO")
#pragma pop_macro("DISPATCH_INLINED")
#pragma pop_macro("PROCESS_ERROR_AT")
#pragma pop_macro("IR_EXIT")
#pragma pop_macro("JUMP_TO_PREDICTED")
#pragma pop_macro("IR_OBSERVE_EDGE")
#pragma pop_macro("IR_HOLD_NEXT")
#pragma pop_macro("IR_STEP_DONE")
#pragma pop_macro("IR_REPORT_EDGE")
#pragma pop_macro("IR_GEN_RETURN")
#pragma pop_macro("IR_ROUTINE")
#pragma pop_macro("IR_HOT_ROUTINE")
#pragma pop_macro("GEN_DISPATCH")
#pragma pop_macro("GEN_STEP_JUMP")
#pragma pop_macro("GEN_RAISED")
#pragma pop_macro("IR_JIT_EVENT_AT")
#pragma pop_macro("IR_JIT_EVENT")
#pragma pop_macro("IR_JIT_RESUME")
#pragma pop_macro("IR_STORE_RETURN")
#pragma pop_macro("IR_PIN_DEPARTING")
#pragma pop_macro("IR_ERROR_SWITCH")
#pragma pop_macro("IR_COMPLETIONS")
