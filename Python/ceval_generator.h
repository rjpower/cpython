/* The generator transfer steps (rule 13), for the transfers that are not
 * plain: a callback, a hook, a monitor or a pending redirect must observe
 * them.  The plain transfers run in the evaluator, in the VM routine
 * gen_return (Python/ceval_routines.h).  A step takes the value that it transfers
 * as the word of its call, and returns the frame and next word to run, or no
 * word for an exit that the activation describes (the error routine,
 * process_error).  The evaluator calls a step and dispatches its result: no
 * step jumps into the evaluator.  The steps call one another to continue a
 * transfer.
 *
 * This file is included once, with the exits of the outlined handlers. */
#define GENERATOR_DISPATCH() DISPATCH()
#define GEN_STEP(NAME) \
    Py_NO_INLINE _PyIRTransfer \
    _PyEval_ir_##NAME(PyThreadState *tstate, _PyInterpreterFrame *frame, \
              _PyInstr *next_instr, _PyIRWord ir_word)

GEN_STEP(GEN_REDIRECT_EFFECT);

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
