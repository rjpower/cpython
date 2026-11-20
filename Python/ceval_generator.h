/* The generator transfer steps (rule 13), for the transfers that are not
 * plain: a callback, a hook, a monitor or a pending redirect must observe
 * them.  The plain transfers run in the evaluator, in the labels of
 * Python/ceval_generator_labels.h.  A step takes the value that it transfers
 * as the word of its call, and returns the frame and next word to run, or no
 * word for an exit that the activation describes (Python/ceval.c,
 * outlined_exit).  The evaluator calls a step and dispatches its result: no
 * step jumps into the evaluator.  The steps call one another to continue a
 * transfer.
 *
 * This file is included once, with the exits of the outlined handlers. */
#define GENERATOR_DISPATCH() DISPATCH()
#define GEN_STEP(NAME) \
    static Py_NO_INLINE _PyIRTransfer \
    ir_##NAME(PyThreadState *tstate, _PyInterpreterFrame *frame, \
              _PyInstr *next_instr, _PyIRWord ir_word)

GEN_STEP(GEN_ENTRY_EFFECT);
GEN_STEP(GEN_REDIRECT_EFFECT);
GEN_STEP(GEN_RESUME_EFFECT);
GEN_STEP(GEN_SEND_INPUT_HOME);
GEN_STEP(GEN_YIELD_INPUT_EFFECT);
static Py_NO_INLINE _PyIRTransfer
ir_GEN_COMPLETE_EFFECT(PyThreadState *tstate, _PyInterpreterFrame *frame,
                       _PyInstr *next_instr, _PyIRWord ir_word,
                       int remaining_count);

/* The kinds of transfer that the tables of Python/ir_targets.h give a word's
 * handler.  A completed outcome is by its consumer, and GEN_THROWING marks a
 * CLEANUP_THROW's, whose window has a fourth input. */
enum {
    GEN_COMPLETE_DISCARD = 1,
    GEN_COMPLETE_HOME,
    GEN_COMPLETE_POP_TOP,
    GEN_COMPLETE_RETURN,
    GEN_COMPLETE_STORE,
    GEN_COMPLETE_STORE_FAST,
    GEN_THROWING = 8,
};
/* A resume by the word before the resume point: a YIELD whose resumer takes
 * the value in a SEND's window or in a STORE_FAST, or the SEND at the end of
 * its loop, which a generator or another iterator receives. */
enum {
    GEN_RESUME_SEND = 1,
    GEN_RESUME_STORE,
    GEN_SEND_INPUT,
    GEN_SEND_INPUT_HOME,
};

#define IR_GEN_TABLES
#include "ir_targets.h"
#undef IR_GEN_TABLES

/* remaining_count counts the send inputs still in their homes, from the
 * base up.  Only this effect entry decodes the source outcome. Earlier successful closes
 * have consumed their owners and NULLed their homes. */
static Py_NO_INLINE _PyIRTransfer
ir_GEN_COMPLETE_EFFECT(PyThreadState *tstate, _PyInterpreterFrame *frame,
                       _PyInstr *next_instr, _PyIRWord ir_word,
                       int remaining_count)
{
    PyValue result = generator_payload(ir_word);
    _PyInstr *site = next_instr;
    _PyIRWord source_word = *site;
    int source_handler = _PyIR_RealHandler(frame, site);
    bool throwing = _PyIR_GeneratorBoundaries[source_handler].ordinary == CLEANUP_THROW;
    int count = throwing ? 4 : 3;
    int base = IR_B(source_word);
    int destination = IR_A(source_word);
    _PyInstr *exit = throwing ? frame->ir_words + IR_CD(source_word)
                             : site + 1 + IR_DS(source_word);
    assert(remaining_count >= 0 && remaining_count <= count);
    if (frame->pending_redirect || frame->instr_ptr != site) {
        generator_publish_redirected(tstate, frame, result);
        frame = tstate->current_frame;
        next_instr = frame->instr_ptr;
        GENERATOR_DISPATCH();
    }
    if (remaining_count == count && !throwing &&
        _PyFrame_GetCode(frame)->_co_monitoring != NULL) {
        _PyIRStepResult pre = _PyIR_ObserveStep(
            tstate, frame, site, exit, PyValue_Borrow(result), _PY_IR_SEND, 0);
        if (pre.status < 0 || pre.status == _PY_IR_STEP_REDIRECT) {
            PyObject *error = pre.status < 0 ? PyErr_GetRaisedException() : NULL;
            PyValue_XCLOSE(result);
            if (error != NULL) {
                PyErr_SetRaisedException(error);
                PROCESS_ERROR_AT(RAISE, 0, frame->instr_ptr);
            }
            next_instr = frame->instr_ptr;
            GENERATOR_DISPATCH();
        }
    }
    PyObject *error = generator_complete_publish(tstate, frame, site, exit,
        base, remaining_count, destination, throwing, result);
    frame = tstate->current_frame;
    if (error != NULL) {
        PyErr_SetRaisedException(error);
        PROCESS_ERROR_AT(RAISE, 0, frame->instr_ptr);
    }
    next_instr = frame->instr_ptr;
    GENERATOR_DISPATCH();
}

/* Provider success commits the yielded edge before retiring a final sent
 * input. The result becomes the next YIELD's real input, so callbacks can
 * redirect that continuation without any old-home retake afterward. */
GEN_STEP(GEN_YIELD_INPUT_EFFECT)
{
    PyValue result = generator_payload(ir_word);
    int home = IR_B(*next_instr) + 2;
    PyValue sent = frame->base[home];
    frame->base[home] = result;
    _PyValue_CheckTemp(result);
    frame->instr_ptr = next_instr + 1;
    PyValue_XCLOSE(sent);
    frame = tstate->current_frame;
    next_instr = frame->instr_ptr;
    GENERATOR_DISPATCH();
}

/* A private resumed input is not an ordinary handler ABI. Generic providers
 * and effects receive it once in their authoritative input home. */
GEN_STEP(GEN_SEND_INPUT_HOME)
{
    PyValue result = generator_payload(ir_word);
    int home = IR_B(*next_instr) + 2;
    assert(PyValue_IsNull(frame->base[home]));
    frame->base[home] = result;
    _PyValue_CheckTemp(result);
    GENERATOR_DISPATCH();
}

/* The child link is committed, but its recursion debit has not happened.
 * The incoming owner stays in C while error construction allocates. */
GEN_STEP(GEN_ENTRY_EFFECT)
{
    PyValue value = generator_payload(ir_word);
    if (_Py_EnterRecursivePy(tstate)) {
        PyValue_XCLOSE(value);
        PROCESS_ERROR_AT(EXIT_NOTRACE, 0, frame->instr_ptr);
    }
    IR_GEN_JUMP(GEN_RESUME_EFFECT, frame,
                frame->instr_ptr, value);
}

GEN_STEP(GEN_RESUME_EFFECT)
{
    _PyInstr *source = frame->instr_ptr;
    uint64_t generation = generator_control_generation(frame);
    int pending = frame->pending_redirect;
    generator_publish_redirected(tstate, frame, generator_payload(ir_word));
    if (pending || frame->instr_ptr != source ||
        generator_control_generation(frame) != generation) {
        next_instr = frame->instr_ptr;
    }
    DTRACE_FUNCTION_ENTRY();
    LLTRACE_RESUME_FRAME();
    GENERATOR_DISPATCH();
}

GEN_STEP(GEN_REDIRECT_EFFECT)
{
    generator_publish_redirected(tstate, frame, generator_payload(ir_word));
    frame = tstate->current_frame;
    next_instr = frame->instr_ptr;
    LLTRACE_RESUME_FRAME();
    GENERATOR_DISPATCH();
}

#undef GENERATOR_DISPATCH
#undef GEN_STEP
