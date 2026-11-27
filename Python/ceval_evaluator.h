/* The evaluator, a tail-calling interpreter (Python/ceval_tail.h), which
 * Python/ceval_normal.c compiles as the normal evaluator and
 * Python/ceval_observing.c compiles again, with IR_OBSERVING set, as the
 * observing one: the same handlers, labels and routines, each a function,
 * over the dispatch table of each (Python/ir_targets.h).  The observing
 * evaluator tests, before each word, what monitoring observes there: the
 * LINE and INSTRUCTION events, and whether the word runs its family's
 * observer (observe_word); its table sends every Resume word to Resume's
 * observer, which keeps the frame in the right evaluator.  A frame runs in
 * the evaluator that its code's observed mark asks for; it moves between
 * the two at the switch word (IR_H_SWITCH, _PyEval_SwitchLoop()) by a tail
 * call of the other evaluator's entry.
 *
 * The frame's instr_ptr stays the word that ran last, which a LINE event
 * compares its word with. */

#include "ir_targets.h"
#include "ceval_tail.h"

/* Every function of the interpreter, declared before the table and the
 * functions, which tail-call one another (an id without a handler repeats
 * its declaration).  The first declaration fixes the function's place in its
 * section (IR_TAIL_HEAD): the arms in IR_TARGET_LAYOUT's order come first,
 * hot families before the rest. */
#define IR_TAIL_LAYOUT(NAME) IR_TAIL_FN(NAME);
IR_TARGET_LAYOUT(IR_TAIL_LAYOUT)
#undef IR_TAIL_LAYOUT
#define IR_TAIL_DECLARE(ID, NAME) IR_TAIL_FN(NAME);
IR_TARGETS(IR_TAIL_DECLARE)
#undef IR_TAIL_DECLARE
IR_TAIL_FN(start_frame);
IR_TAIL_FN(throw_error);
IR_TAIL_FN(process_error);
IR_TAIL_FN(gen_return);
IR_TAIL_FN(close_dispatch);
#if IR_OBSERVING
IR_TAIL_FN(observe_word);
#endif

static const _PyIRTailFn ir_tail_table[IR_TARGETS_COUNT] = {
#define IR_TAIL_TARGET(ID, NAME) [ID] = IR_TAIL_ID(NAME),
#if IR_OBSERVING
    IR_OBSERVING_TARGETS(IR_TAIL_TARGET)
#else
    IR_TARGETS(IR_TAIL_TARGET)
#endif
#undef IR_TAIL_TARGET
};

/* The bodies read some locals that only other bindings use (an
 * `int opcode` before a dispatch).  The observing table leaves out the arms
 * whose observer takes their ids. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-function"
/* GCC warns of every local whose address an arm passed to a helper, such as
 * a checked sum's out-parameter: the helper has returned before the tail
 * call, and no tail call passes such an address. */
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wmaybe-musttail-local-addr"
#endif
#include "ceval_labels.h"
#include "generated_rop_cases.c.h"
#include "ceval_routines.h"
#pragma GCC diagnostic pop

#if IR_OBSERVING
/* Monitoring observes the word at next_instr (_PyFrame_Observed()): its
 * LINE and INSTRUCTION events run first, then the word, under its
 * family's observer where a tool listens to it (_PyIR_OBSERVE_EVENTS).
 * The word is read again after the events: a callback may have run the
 * code, which specialises the word, or changed what is observed. */
IR_TAIL_FN(observe_word)
{
    int opcode;
    uint8_t observed = _PyFrame_Observed(frame, next_instr);
    if (observed & (_PyIR_OBSERVE_LINE | _PyIR_OBSERVE_INSTRUCTION)) {
        int status = _PyEval_LocationEvents(tstate, frame, next_instr);
        if (status < 0) {
            PROCESS_ERROR_AT(RAISE, 0, frame->instr_ptr);
        }
        if (status > 0) {
            IR_REDIRECT();
        }
        observed = _PyFrame_Observed(frame, next_instr);
    }
    NEXTOPARG();
    if (observed & _PyIR_OBSERVE_EVENTS) {
        opcode = _PyIR_ObservingHandler[opcode];
    }
    PRE_DISPATCH_GOTO();
    DISPATCH_GOTO();
}
#endif

/* The cold exit of an arm's tail closes (ROP_TAIL_CLOSE,
 * Python/ceval_macros.h): give the values that it parked, which have no
 * count, their last counts back before any code runs, close them in close
 * order, then dispatch next_instr.  The values leave the activation first,
 * so that the closes, which run code, see none parked. */
IR_TAIL_FN(close_dispatch)
{
    _PyEvalActivation *activation = IR_ACTIVATION();
    int n = activation->tail_closes;
    assert(0 < n && n <= _PyEval_TAIL_CLOSES);
    PyValue parked[_PyEval_TAIL_CLOSES];
    for (int i = 0; i < n; i++) {
        parked[i] = activation->tail_close[i];
        _PyValue_RestoreLast(parked[i]);
    }
    activation->tail_closes = 0;
    for (int i = 0; i < n; i++) {
        PyValue_XCLOSE(parked[i]);
    }
    /* The cold exit of a compiled arm: the frame goes back to its unit */
    IR_JIT_SWITCH();
    int opcode;
    DISPATCH();
}

/* The switch word: go on at the word that it stands for, in this
 * evaluator or the other one (_PyEval_SwitchesLoop()) */
IR_TAIL_FN(switch_loop)
{
    next_instr = _PyEval_SwitchLoop(tstate, frame);
    if (_PyEval_SwitchesLoop(frame, IR_OBSERVING)) {
        Py_MUSTTAIL return IR_OTHER_EVALUATOR(tstate, frame, next_instr, ir_word);
    }
    int opcode;
    DISPATCH();
}

#if !IR_OBSERVING
/* The driver's entries (ir_evaluate(), Python/ceval.c): run frame from its
 * start, or throw into it */
Py_IR_TAIL_CC PyValue
_PyEval_StartFrame(Py_IR_TAIL_PARAMS)
{
    IR_TAIL_GOTO(start_frame);
}

Py_IR_TAIL_CC PyValue
_PyEval_ThrowFrame(Py_IR_TAIL_PARAMS)
{
    IR_TAIL_GOTO(throw_error);
}
#endif

/* The entry of this evaluator, which the other one switched frame to: go on
 * at next_instr */
Py_IR_TAIL_CC PyValue
IR_THIS_EVALUATOR(Py_IR_TAIL_PARAMS)
{
    int opcode;
    DISPATCH();
}

#if !IR_OBSERVING && defined(_PY_JIT_SUPPORTED)
/* The baseline JIT's exits (Python/jit.c, bind_hole()): the functions that
 * compiled code tail-calls when it leaves a word for anything but the next
 * word's or the target's copy.  Each first closes the thread's departure
 * window (_PyJit_Reclaim()): control has left the unit's code, and a tail
 * call leaves no return address into it. */
#define IR_JIT_LEAVE() (((_PyThreadStateImpl *)tstate)->jit_departing = 0)

/* A frame switch, or a jump whose target has no copy: go on at next_instr
 * of frame, in its code's unit if the code has live entries, otherwise in
 * this evaluator */
Py_IR_TAIL_CC PyValue
_PyEval_JitDispatch(Py_IR_TAIL_PARAMS)
{
    IR_JIT_LEAVE();
#ifdef Py_STATS
    tstate->interp->ceval.jit_switches++;
#endif
    const void *const *ir_entries = _PyFrame_JitEntries(frame);
    if (ir_entries != NULL && next_instr != &_PyEval_SwitchWord &&
        !IR_JIT_TRACED()) {
        Py_MUSTTAIL return ((_PyIRTailFn)ir_entries[next_instr - frame->ir_words])(
            tstate, frame, next_instr, ir_word);
    }
    int opcode;
    DISPATCH();
}

/* A redirect, or a word that runs again under the arm that it now names:
 * the word at next_instr, in this evaluator */
Py_IR_TAIL_CC PyValue
_PyEval_JitInterpret(Py_IR_TAIL_PARAMS)
{
    IR_JIT_LEAVE();
    int opcode;
    DISPATCH();
}

/* A guard of the word's arm failed with the word untouched: its family's
 * generic arm runs the word in this evaluator, with ir_word the word */
Py_IR_TAIL_CC PyValue
_PyEval_JitMiss(Py_IR_TAIL_PARAMS)
{
    IR_JIT_LEAVE();
    int generic = _PyJit_GenericOf(IR_HANDLER(ir_word));
    Py_MUSTTAIL return ir_tail_table[generic](tstate, frame, next_instr, ir_word);
}

Py_IR_TAIL_CC PyValue
_PyEval_JitProcessError(Py_IR_TAIL_PARAMS)
{
    IR_JIT_LEAVE();
    IR_TAIL_GOTO(process_error);
}

Py_IR_TAIL_CC PyValue
_PyEval_JitCloseDispatch(Py_IR_TAIL_PARAMS)
{
    IR_JIT_LEAVE();
    IR_TAIL_GOTO(close_dispatch);
}

Py_IR_TAIL_CC PyValue
_PyEval_JitGenReturn(Py_IR_TAIL_PARAMS)
{
    IR_JIT_LEAVE();
    IR_TAIL_GOTO(gen_return);
}

/* The entry of an operand word, which no transfer names */
Py_IR_TAIL_CC PyValue
_PyEval_JitBadEntry(Py_IR_TAIL_PARAMS)
{
    Py_FatalError("baseline JIT: an operand word was entered");
}
#endif
