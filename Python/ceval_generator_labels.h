/* The plain generator transfers (rule 13 keeps the others in the steps of
 * Python/ceval_generator.h), as labels of ir_evaluate().  A handler sets
 * ir_gen_value and the other state that a label reads and jumps to it, by
 * the IR_GEN_* macros of Python/ceval_macros.h.  A chain of delegating
 * frames is one transfer: a label walks it in a loop, tests the thread's
 * share of the plain-transfer test once, and tests the frame's share at each
 * level.  The tables that tell the kind of transfer of a word are in
 * Python/ir_targets.h.
 *
 * This file is included in ir_evaluate(), after its handlers. */

/* The labels leave through two shared blocks, so that the evaluator has one
 * dispatch for them: gen_dispatch runs the word at next_instr, and
 * gen_stepped runs what a step returned. */
#define GEN_STEP_JUMP(NAME, F, IP, V) \
    do { \
        ir_gen_transfer = ir_##NAME(tstate, (F), (IP), (V).bits); \
        goto LABEL_gen_stepped; \
    } while (0)
#define GEN_STEP_EFFECT(REMAINING, IP, V) \
    do { \
        ir_gen_transfer = ir_GEN_COMPLETE_EFFECT(tstate, frame, (IP), (V).bits, \
                                                 (REMAINING)); \
        goto LABEL_gen_stepped; \
    } while (0)

HOT_LABEL(gen_dispatch)
{
    int opcode;
    DISPATCH();
}

HOT_LABEL(gen_stepped)
{
    frame = ir_gen_transfer.frame;
    next_instr = ir_gen_transfer.next;
    if (next_instr == NULL) {
        goto LABEL_outlined_exit;
    }
    goto LABEL_gen_dispatch;
}

/* The value passes up from the SEND at next_instr, whose YIELD follows it,
 * through every delegating frame above: each level suspends in its YIELD and
 * hands the value to the SEND that resumed it, until the resumer is not one
 * whose outcome is native, and then the value goes to its return slot. */
HOT_LABEL(gen_yield)
{
    PyValue result = ir_gen_value;
    bool thread_plain = generator_thread_is_plain(tstate);
    for (;;) {
        _PyInstr *yield = next_instr + 1;
        _PyIRWord word = *yield;
        /* The old outgoing input has been consumed by the actual producer. */
        frame->base[IR_B(word)] = PyValue_NULL;
        if (!thread_plain || !generator_frame_is_plain(frame)) {
            frame->base[IR_B(word)] = result;
            _PyValue_CheckTemp(result);
            next_instr = yield;
            goto LABEL_gen_dispatch;
        }
        INSTRUCTION_STATS(YIELD_DELEGATED);
        frame->instr_ptr = yield + 1;
        PyGenObject *gen = _PyGen_GetGeneratorFromFrame(frame);
        tstate->exc_info = gen->gi_exc_state.previous_item;
        gen->gi_exc_state.previous_item = NULL;
        _Py_LeaveRecursiveCallPy(tstate);
        _PyInterpreterFrame *suspended = frame;
        suspended->return_slot = (uint16_t)IR_A(word);
        _PyThreadState_UpdateLastProfiledFrame(tstate, frame, frame->previous);
        frame = tstate->current_frame = frame->previous;
        suspended->previous = NULL;
        ((_PyThreadStateImpl *)tstate)->generator_return_kind = GENERATOR_YIELD;
        FT_ATOMIC_STORE_INT8_RELEASE(gen->gi_frame_state, FRAME_SUSPENDED_YIELD_FROM);
        next_instr = frame->instr_ptr;
        if (!generator_site_kind(frame, next_instr, generator_yielded_targets) ||
            frame->pending_redirect) {
            break;
        }
    }
    next_instr = frame->return_pc;
    if (_PyFrame_SetReturnValue(frame, result)) { next_instr = frame->return_pc; }
    LLTRACE_RESUME_FRAME();
    goto LABEL_gen_dispatch;
}

/* The generator that frame has just entered, with its debit done, resumes
 * with ir_gen_value.  The word before its resume point tells how it
 * suspended.  If it was a delegation through a SEND that is native, its
 * resumption is the transfer of a send: the value goes down through the
 * RESUME and the loop back to the SEND, and the SEND enters the next
 * generator, and so on.  If it suspended with a native value-taking
 * resume, the value goes to its STORE_FAST. */
HOT_LABEL(gen_resume)
{
    PyValue value = ir_gen_value;
    bool thread_plain = generator_thread_is_plain(tstate);
    for (;;) {
        _PyInstr *resume = frame->instr_ptr;
        if (resume <= frame->ir_words) {
            break;
        }
        uint8_t kind = generator_resume_targets[IR_HANDLER(resume[-1])];
        if (kind != GEN_RESUME_SEND && kind != GEN_RESUME_STORE) {
            break;
        }
        if (!thread_plain || !generator_frame_is_plain(frame)) {
            GEN_STEP_JUMP(GEN_RESUME_EFFECT, frame, resume, value);
        }
        next_instr = resume;
        IR_SKIP_RESUME();
        if (next_instr == resume) {
            GEN_STEP_JUMP(GEN_RESUME_EFFECT, frame, resume, value);
        }
        if (kind == GEN_RESUME_STORE) {
            _PyIRWord store = *next_instr;
            int destination = IR_B(store);
            PyValue old = frame->base[destination];
            if (_PyFrame_ObjectMayReenter(PyValue_IsObject(old)
                    ? PyValue_AsKnownObjectBorrow(old) : NULL)) {
                GEN_STEP_JUMP(GEN_RESUME_EFFECT, frame, next_instr, value);
            }
            INSTRUCTION_STATS(STORE_FAST);
            frame->base[destination] = value;
            _PyValue_CheckHome(value);
            bool closed = _PyValue_TryXCloseNoReentry(old);
            assert(closed);
            (void)closed;
            frame->instr_ptr = next_instr;
            ++next_instr;
            goto LABEL_gen_dispatch;
        }
        _PyIRWord jump = *next_instr;
        assert(IR_HANDLER(jump) == JUMP_BACKWARD_NO_INTERRUPT);
        INSTRUCTION_STATS(JUMP_BACKWARD_NO_INTERRUPT);
        next_instr = frame->ir_words + IR_CD(jump);
        PyValue receiver = frame->base[IR_B(*next_instr)];
        if (generator_resume_targets[IR_HANDLER(*next_instr)] != GEN_SEND_INPUT ||
            !PyValue_IsObject(receiver) || tstate->py_recursion_remaining <= 1) {
            GEN_STEP_JUMP(GEN_SEND_INPUT_HOME, frame, next_instr, value);
        }
        PyGenObject *gen = (PyGenObject *)PyValue_AsKnownObjectBorrow(receiver);
        if ((Py_TYPE(gen) != &PyGen_Type && Py_TYPE(gen) != &PyCoro_Type) ||
            (FT_ATOMIC_LOAD_INT8_RELAXED(gen->gi_frame_state) == FRAME_CREATED &&
             !PyValue_IsNone(value)) || !gen_try_set_executing(gen)) {
            GEN_STEP_JUMP(GEN_SEND_INPUT_HOME, frame, next_instr, value);
        }
        INSTRUCTION_STATS(SEND_GEN);
        STAT_INC(SEND, hit);
        frame->instr_ptr = next_instr;
        frame->return_slot = (uint16_t)(IR_B(*next_instr) + 2);
        frame->return_pc = next_instr + 1;
        _PyInterpreterFrame *child = &gen->gi_iframe;
        child->previous = frame;
        gen->gi_exc_state.previous_item = tstate->exc_info;
        tstate->exc_info = &gen->gi_exc_state;
        frame = tstate->current_frame = child;
        --tstate->py_recursion_remaining;
    }
    /* Not a chain: the generator resumes at its instr_ptr, and the value
     * goes to the slot that the frame that entered it named. */
    _PyFrame_SetReturnValue(frame, value);
    LOAD_IP(0);
    IR_SKIP_RESUME();
    DTRACE_FUNCTION_ENTRY();
    LLTRACE_RESUME_FRAME();
    goto LABEL_gen_dispatch;
}

/* The generator frame ir_gen_dying has returned ir_gen_value to frame, which
 * was current when it ran and is current again; the frame is unlinked and its
 * recursion debit is done.  Retire it, with the value held in C, and route
 * the value to the resumer.  A finalizer that runs during the retirement can
 * redirect the resumer; a changed pc or control generation is taken as a
 * redirect. */
HOT_LABEL(gen_return)
{
    _PyInterpreterFrame *dying = ir_gen_dying;
    PyValue value = ir_gen_value;
    _PyInstr *site = frame->instr_ptr;
    uint8_t kind = generator_site_kind(frame, site, generator_completed_targets);
    uint64_t generation = generator_control_generation(frame);
    _PyThreadState_UpdateLastProfiledFrame(tstate, dying, frame);
    PyGenObject *gen = _PyGen_GetGeneratorFromFrame(dying);
    FT_ATOMIC_STORE_INT8_RELEASE(gen->gi_frame_state, FRAME_CLEARED);
    assert(tstate->exc_info == &gen->gi_exc_state);
    tstate->exc_info = gen->gi_exc_state.previous_item;
    gen->gi_exc_state.previous_item = NULL;
    dying->previous = NULL;
    value = ir_gen_empty ? _PyFrame_ClearGeneratorLocals(gen, value)
                         : _PyFrame_ClearGenerator(gen, value);
    ((_PyThreadStateImpl *)tstate)->generator_return_kind = GENERATOR_RETURN;
    assert(frame == tstate->current_frame);
    if (frame->owner != FRAME_OWNED_BY_INTERPRETER &&
        (frame->pending_redirect ||
         (kind != 0 && (frame->instr_ptr != site ||
                        generator_control_generation(frame) != generation)))) {
        GEN_STEP_JUMP(GEN_REDIRECT_EFFECT, frame, frame->instr_ptr, value);
    }
    if (kind == GEN_COMPLETE_FOR_ITER && generator_transfer_is_plain(tstate, frame)) {
        /* The iterator is exhausted: its window is released after the
         * frame has moved on, as complete_step_impl() does. */
        _PyIRWord word = *site;
        PyValue *window = frame->base + IR_B(word);
        PyValue receiver = window[0];
        PyValue cursor = window[1];
        window[0] = window[1] = PyValue_NULL;
        _PyInstr *exit = frame->return_pc + IR_DS(word);
        frame->return_pc = exit;
        frame->instr_ptr = exit;
        PyValue_XCLOSE(cursor);
        PyValue_XCLOSE(receiver);
        PyValue_XCLOSE(value);
        next_instr = frame->instr_ptr;
        LLTRACE_RESUME_FRAME();
        goto LABEL_gen_dispatch;
    }
    if (kind == 0 || kind == GEN_COMPLETE_FOR_ITER) {
        /* A FOR that must be observed, a nonadmitted SEND or an interpreter
         * entry keeps its own contract. */
        _PyIRStepResult completed = _PyEval_CompleteGeneratorReturn(tstate, frame, value);
        next_instr = completed.next;
        if (completed.status < 0) {
            PROCESS_ERROR_AT(RAISE, 0, next_instr - 1);
        }
        LLTRACE_RESUME_FRAME();
        goto LABEL_gen_dispatch;
    }
    ir_gen_value = value;
    next_instr = site;
    goto LABEL_gen_complete;
}

/* The SEND or CLEANUP_THROW at next_instr, whose word has a native completed
 * outcome, completes with ir_gen_value.  It retires the send's
 * inputs, which stay in their homes until now, and publishes the value as
 * the consumer of the word says.
 *
 * Sequential tests matter: one alias's decrement can make the next alias
 * final, so the inputs close one at a time, from the last.  The first input
 * is the receiver (gen_close_input). */
HOT_LABEL(gen_complete)
{
    PyValue result = ir_gen_value;
    _PyIRWord source_word = *next_instr;
    int kind = generator_site_kind(frame, next_instr, generator_completed_targets);
    assert(kind != 0);
    bool throwing = (kind & GEN_THROWING) != 0;
    int base = IR_B(source_word);
    if (!generator_transfer_is_plain(tstate, frame)) {
        GEN_STEP_EFFECT(throwing ? 4 : 3, next_instr, result);
    }
    for (int k = throwing ? 3 : 2; k >= 0; k--) {
        if (!gen_close_input(frame->base[base + k], k == 0)) {
            GEN_STEP_EFFECT(k + 1, next_instr, result);
        }
        frame->base[base + k] = PyValue_NULL;
    }
    _PyInstr *exit = throwing ? frame->ir_words + IR_CD(source_word)
                              : next_instr + 1 + IR_DS(source_word);
    switch (kind & ~GEN_THROWING) {
    case GEN_COMPLETE_DISCARD:
    case GEN_COMPLETE_POP_TOP: {
        if (!_PyValue_TryXCloseNoReentry(result)) {
            GEN_STEP_EFFECT(0, next_instr, result);
        }
        next_instr = exit;
        frame->instr_ptr = frame->return_pc = next_instr;
        if ((kind & ~GEN_THROWING) == GEN_COMPLETE_POP_TOP) {
            INSTRUCTION_STATS(POP_TOP);
            ++next_instr;
        }
        goto LABEL_gen_dispatch;
    }
    case GEN_COMPLETE_HOME: {
        int destination = IR_A(source_word);
        assert(PyValue_IsNull(frame->base[destination]));
        frame->base[destination] = result;
        _PyValue_CheckTemp(result);
        next_instr = exit;
        frame->instr_ptr = frame->return_pc = next_instr;
        goto LABEL_gen_dispatch;
    }
    case GEN_COMPLETE_STORE:
    case GEN_COMPLETE_STORE_FAST: {
        bool explicit_store = (kind & ~GEN_THROWING) == GEN_COMPLETE_STORE_FAST;
        int destination = explicit_store ? IR_B(*exit) : IR_A(source_word);
        PyValue old = frame->base[destination];
        if (_PyFrame_ObjectMayReenter(PyValue_IsObject(old)
                ? PyValue_AsKnownObjectBorrow(old) : NULL)) {
            GEN_STEP_EFFECT(0, next_instr, result);
        }
        if (explicit_store) { INSTRUCTION_STATS(STORE_FAST); }
        frame->base[destination] = result;
        _PyValue_CheckHome(result);
        bool closed = _PyValue_TryXCloseNoReentry(old);
        assert(closed);
        (void)closed;
        frame->instr_ptr = frame->return_pc = exit;
        next_instr = exit + (explicit_store ? 1 : 0);
        goto LABEL_gen_dispatch;
    }
    default: {
        /* The consumer is the RETURN: this frame returns the value in turn.
         * The transfer was plain a moment ago, and nothing has run since. */
        assert((kind & ~GEN_THROWING) == GEN_COMPLETE_RETURN);
        assert(kind != GEN_COMPLETE_FOR_ITER);
        next_instr = exit;
        frame->instr_ptr = next_instr;
        INSTRUCTION_STATS(IR_HANDLER(*next_instr));
        _Py_LeaveRecursiveCallPy(tstate);
        _PyInterpreterFrame *dying = frame;
        int generator_return = dying->owner == FRAME_OWNED_BY_GENERATOR;
        frame = tstate->current_frame = dying->previous;
        if (generator_return) {
            IR_GEN_RETURN(result, dying, 0);
        }
        result = _PyEval_FrameClearAndPopGeneralWithResult(tstate, dying, result);
        next_instr = frame->return_pc;
        if (_PyFrame_SetReturnValue(frame, result)) { next_instr = frame->return_pc; }
        LLTRACE_RESUME_FRAME();
        goto LABEL_gen_dispatch;
    }
    }
}
