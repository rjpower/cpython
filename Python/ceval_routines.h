/* The VM routines (tiers87, C2): the shared paths of the interpreter that a
 * handler's transfer enters, the error routine and the plain generator
 * transfers (rule 13 keeps the others in the steps of
 * Python/ceval_generator.h).  Each routine is one definition with two
 * bindings.  This file is included in ir_evaluate(), where IR_ROUTINE() and
 * IR_HOT_ROUTINE() open a label and the routine's operands,
 * IR_ROUTINE_OPERAND(NAME), are the evaluator's locals of the same names,
 * which the handlers' transfer macros set (Python/ceval_macros.h); and at
 * file scope, where they open the routine's function form, ir_NAME(tstate,
 * frame, next_instr, ir_word), of the one type of every function of the
 * interpreter (a tail call needs caller and callee of one type), whose
 * operands are the fields of the same names of the activation, which the
 * transfer that enters the routine writes.  A routine leaves only through
 * the macros that its binding defines:
 *
 *   GEN_DISPATCH()               run the word at next_instr in frame;
 *   GEN_STEP_JUMP(NAME, F, IP, V)
 *                                run a step of Python/ceval_generator.h and
 *                                what it returns;
 *   GEN_RAISED()                 unwind the error that a completion arm
 *                                reported in the activation;
 *   DISPATCH(), PROCESS_ERROR_AT(), IR_EXIT() and the IR_GEN_* transfers, as
 *                                in a handler.
 *
 * A chain of delegating frames is one transfer: a routine walks it in a loop,
 * tests the thread's share of the plain-transfer test once, and tests the
 * frame's share at each level.  The tables that tell the kind of transfer of
 * a word are in Python/ir_targets.h. */

/* The generator frame gen_dying has returned gen_value to frame, which
 * was current when it ran and is current again; the frame is unlinked and its
 * recursion debit is done.  Retire it, with the value held in C, and give the
 * value to the consumer.  A consumer that the evaluator entered from C takes it
 * as its return value.  The word of a frame that entered the generator is a
 * Send, whose completion arm runs in that frame with the value
 * (IR_COMPLETIONS, from the family's declaration, or its observer's in the
 * observing evaluator): it publishes the
 * completed value and the head to go on at, or raises or redirects.  A
 * finalizer that runs during the retirement can redirect the resumer; a
 * changed pc or control generation is taken as a redirect. */
IR_HOT_ROUTINE(gen_return)
{
    _PyInterpreterFrame *dying = IR_ROUTINE_OPERAND(gen_dying);
    PyValue value = IR_ROUTINE_OPERAND(gen_value);
    int empty = IR_ROUTINE_OPERAND(gen_empty);
    _PyInstr *site = frame->instr_ptr;
    uint64_t generation = generator_control_generation(frame);
    _PyThreadState_UpdateLastProfiledFrame(tstate, dying, frame);
    PyGenObject *gen = _PyGen_GetGeneratorFromFrame(dying);
    FT_ATOMIC_STORE_INT8_RELEASE(gen->gi_frame_state, FRAME_CLEARED);
    assert(tstate->exc_info == &gen->gi_exc_state);
    tstate->exc_info = gen->gi_exc_state.previous_item;
    gen->gi_exc_state.previous_item = NULL;
    dying->previous = NULL;
    _PyFrame_ClearGenerator(gen, empty ? (int)_PyFrame_Exec(dying)->nfixed
                                       : _PyFrame_NumSlots(dying));
    ((_PyThreadStateImpl *)tstate)->generator_return_kind = GENERATOR_RETURN;
    assert(frame == tstate->current_frame);
    if (frame->owner == FRAME_OWNED_BY_INTERPRETER) {
        _PyFrame_SetReturnValue(frame, value);
        next_instr = frame->return_pc;
        LLTRACE_RESUME_FRAME();
        GEN_DISPATCH();
    }
    if (frame->pending_redirect || frame->instr_ptr != site ||
        generator_control_generation(frame) != generation) {
        GEN_STEP_JUMP(GEN_REDIRECT_EFFECT, frame, frame->instr_ptr, value);
    }
    int handler = IR_HANDLER(FT_ATOMIC_LOAD_UINT64_RELAXED(*site));
    assert(handler >= ROP_FIRST_HANDLER &&
           IR_COMPLETIONS(site)[handler - ROP_FIRST_HANDLER] != NULL);
    int status = IR_COMPLETIONS(site)[handler - ROP_FIRST_HANDLER](
        tstate, frame, site, value);
    if (status & _PyIR_RAISED) {
        GEN_RAISED();
    }
    if (status & _PyIR_REDIRECT) {
        next_instr = frame->return_pc;
        LLTRACE_RESUME_FRAME();
        GEN_DISPATCH();
    }
    assert(status & _PyIR_BRANCH);
    next_instr = site + 1 + _PyIR_FormatDistance(_PyIR_FormatOf(handler), *site);
    frame->instr_ptr = frame->return_pc = next_instr;
    LLTRACE_RESUME_FRAME();
    GEN_DISPATCH();
}

/* An error at exit_next unwinds by the exception table, with the ingress
 * and checkpoint of exit_entry, and either runs the handler that it finds or
 * ends the activation. */
IR_ROUTINE(process_error)
{
    _PyIRErrorResult ir_error_result = _PyEval_ProcessError(
        tstate, frame, IR_ROUTINE_OPERAND(exit_next),
        IR_ROUTINE_OPERAND(exit_entry));
    if (ir_error_result.disposition == _PyIR_ERROR_HANDLED) {
        frame = tstate->current_frame;
        next_instr = ir_error_result.next;
        /* A handler of a frame that ran in its unit runs there */
        IR_JIT_RESUME();
        /* A frame that unwound into the evaluator that its code does not
         * ask for goes on in the other one */
        IR_ERROR_SWITCH();
        int opcode;
        DISPATCH();
    }
    IR_EXIT(PyValue_NULL);
}
