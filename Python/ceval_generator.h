/* Generator transfer entries: the private labels of the evaluator,
 * ir_evaluate() in Python/ceval.c, which includes this file and the
 * generated handlers inside the function.  Their helpers are above it.
 *
 * Private entries carry a value rather than a word. Ordinary dispatch first
 * reloads the word and needs its own local handler index. */
#define GENERATOR_DISPATCH() \
    do { \
        int opcode; \
        DISPATCH(); \
    } while (0)

/* ir_gen_remaining counts the send inputs still in their homes, from the
 * base up.  Only this effect entry decodes the source outcome. Earlier successful closes
 * have consumed their owners and NULLed their homes. */
TARGET(GEN_COMPLETE_EFFECT)
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
    int remaining_count = ir_gen_remaining;
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

/* Sequential tests matter: one alias's decrement can make the next alias
 * final. Count/kind are constants in each generated native entry. */
#define GENERATOR_CLOSE_INPUT(K) \
    do { \
        PyValue input = frame->base[base + (K)]; \
        if (!_PyValue_TryXCloseNoReentry(input) && \
            !((K) == 0 && PyValue_RefcountOnObject(input) && \
              _PyGen_TryCloseClearedNoReentry(PyValue_AsKnownObjectBorrow(input)))) { \
            IR_GEN_EFFECT((K) + 1, next_instr, result); \
        } \
        frame->base[base + (K)] = PyValue_NULL; \
    } while (0)
#define GENERATOR_RETIRE_SEND_INPUTS(THROW) \
    PyValue result = generator_payload(ir_word); \
    _PyIRWord source_word = *next_instr; \
    int base = IR_B(source_word); \
    if (!generator_transfer_is_plain(tstate, frame)) { \
        IR_GEN_EFFECT((THROW) ? 4 : 3, next_instr, result); \
    } \
    if (THROW) { GENERATOR_CLOSE_INPUT(3); } \
    GENERATOR_CLOSE_INPUT(2); \
    GENERATOR_CLOSE_INPUT(1); \
    GENERATOR_CLOSE_INPUT(0)
#define GENERATOR_COMPLETED_EXIT(THROW) \
    ((THROW) ? frame->ir_words + IR_CD(source_word) : next_instr + 1 + IR_DS(source_word))

#define GENERATOR_COMPLETE_RETURN(NAME, THROW) \
TARGET(NAME) \
{ \
    GENERATOR_RETIRE_SEND_INPUTS(THROW); \
    next_instr = GENERATOR_COMPLETED_EXIT(THROW); \
    IR_GEN_JUMP(GEN_RETURN, frame, next_instr, result); \
}
#define GENERATOR_COMPLETE_HOME(NAME, THROW) \
TARGET(NAME) \
{ \
    GENERATOR_RETIRE_SEND_INPUTS(THROW); \
    int destination = IR_A(source_word); \
    assert(PyValue_IsNull(frame->base[destination])); \
    frame->base[destination] = result; \
    _PyValue_CheckTemp(result); \
    next_instr = GENERATOR_COMPLETED_EXIT(THROW); \
    frame->instr_ptr = frame->return_pc = next_instr; \
    GENERATOR_DISPATCH(); \
}
#define GENERATOR_COMPLETE_STORE(NAME, THROW, EXPLICIT) \
TARGET(NAME) \
{ \
    GENERATOR_RETIRE_SEND_INPUTS(THROW); \
    _PyInstr *consumer = GENERATOR_COMPLETED_EXIT(THROW); \
    int destination = (EXPLICIT) ? IR_B(*consumer) : IR_A(source_word); \
    PyValue old = frame->base[destination]; \
    if (_PyFrame_ObjectMayReenter(PyValue_IsObject(old) \
            ? PyValue_AsKnownObjectBorrow(old) : NULL)) { \
        IR_GEN_EFFECT(0, next_instr, result); \
    } \
    if (EXPLICIT) { INSTRUCTION_STATS(STORE_FAST); } \
    frame->base[destination] = result; \
    _PyValue_CheckHome(result); \
    bool closed = _PyValue_TryXCloseNoReentry(old); \
    assert(closed); \
    (void)closed; \
    frame->instr_ptr = frame->return_pc = consumer; \
    next_instr = consumer + ((EXPLICIT) ? 1 : 0); \
    GENERATOR_DISPATCH(); \
}
#define GENERATOR_COMPLETE_DISCARD(NAME, THROW, EXPLICIT) \
TARGET(NAME) \
{ \
    GENERATOR_RETIRE_SEND_INPUTS(THROW); \
    if (!_PyValue_TryXCloseNoReentry(result)) { \
        IR_GEN_EFFECT(0, next_instr, result); \
    } \
    next_instr = GENERATOR_COMPLETED_EXIT(THROW); \
    frame->instr_ptr = frame->return_pc = next_instr; \
    if (EXPLICIT) { INSTRUCTION_STATS(POP_TOP); ++next_instr; } \
    GENERATOR_DISPATCH(); \
}
GENERATOR_COMPLETE_RETURN(GEN_COMPLETE_RETURN, false)
GENERATOR_COMPLETE_HOME(GEN_COMPLETE_HOME, false)
GENERATOR_COMPLETE_STORE(GEN_COMPLETE_STORE, false, false)
GENERATOR_COMPLETE_DISCARD(GEN_COMPLETE_DISCARD, false, false)
GENERATOR_COMPLETE_STORE(GEN_COMPLETE_STORE_FAST, false, true)
GENERATOR_COMPLETE_DISCARD(GEN_COMPLETE_POP_TOP, false, true)
GENERATOR_COMPLETE_RETURN(GEN_THROW_COMPLETE_RETURN, true)
GENERATOR_COMPLETE_HOME(GEN_THROW_COMPLETE_HOME, true)
GENERATOR_COMPLETE_STORE(GEN_THROW_COMPLETE_STORE, true, false)
GENERATOR_COMPLETE_DISCARD(GEN_THROW_COMPLETE_DISCARD, true, false)
GENERATOR_COMPLETE_STORE(GEN_THROW_COMPLETE_STORE_FAST, true, true)
GENERATOR_COMPLETE_DISCARD(GEN_THROW_COMPLETE_POP_TOP, true, true)
#undef GENERATOR_COMPLETE_RETURN
#undef GENERATOR_COMPLETE_HOME
#undef GENERATOR_COMPLETE_STORE
#undef GENERATOR_COMPLETE_DISCARD
#undef GENERATOR_COMPLETED_EXIT
#undef GENERATOR_RETIRE_SEND_INPUTS
#undef GENERATOR_CLOSE_INPUT

TARGET(GEN_SEND_YIELD)
{
    PyValue result = generator_payload(ir_word);
    _PyInstr *yield = next_instr + 1;
    _PyIRWord word = *yield;
    /* The old outgoing input has been consumed by the actual producer. */
    frame->base[IR_B(word)] = PyValue_NULL;
    if (!generator_transfer_is_plain(tstate, frame)) {
        frame->base[IR_B(word)] = result;
        _PyValue_CheckTemp(result);
        next_instr = yield;
        GENERATOR_DISPATCH();
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
    void *target = generator_outcome_target(
        frame, frame->instr_ptr, generator_yielded_targets);
    if (target != NULL && !frame->pending_redirect) {
        IR_GEN_JUMP_TO(target, frame, frame->instr_ptr, result);
    }
    next_instr = frame->return_pc;
    if (_PyFrame_SetReturnValue(frame, result)) { next_instr = frame->return_pc; }
    LLTRACE_RESUME_FRAME();
    GENERATOR_DISPATCH();
}

/* Provider success commits the yielded edge before retiring a final sent
 * input. The result becomes the next YIELD's real input, so callbacks can
 * redirect that continuation without any old-home retake afterward. */
TARGET(GEN_YIELD_INPUT_EFFECT)
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
TARGET(GEN_SEND_INPUT_HOME)
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
TARGET(GEN_ENTRY_EFFECT)
{
    PyValue value = generator_payload(ir_word);
    if (_Py_EnterRecursivePy(tstate)) {
        PyValue_XCLOSE(value);
        PROCESS_ERROR_AT(EXIT_NOTRACE, 0, frame->instr_ptr);
    }
    IR_GEN_JUMP(GEN_RESUME_EFFECT, frame,
                frame->instr_ptr, value);
}

TARGET(GEN_RESUME_EFFECT)
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

TARGET(GEN_RESUME_SEND)
{
    PyValue value = generator_payload(ir_word);
    _PyInstr *resume = next_instr;
    if (!generator_transfer_is_plain(tstate, frame)) {
        IR_GEN_JUMP(GEN_RESUME_EFFECT, frame, resume, value);
    }
    IR_SKIP_RESUME();
    if (next_instr == resume) {
        IR_GEN_JUMP(GEN_RESUME_EFFECT, frame, resume, value);
    }
    _PyIRWord jump = *next_instr;
    assert(IR_HANDLER(jump) == JUMP_BACKWARD_NO_INTERRUPT);
    INSTRUCTION_STATS(JUMP_BACKWARD_NO_INTERRUPT);
    next_instr = frame->ir_words + IR_CD(jump);
    void *target = generator_resume_targets[IR_HANDLER(*next_instr)];
    assert(target != NULL);
    IR_GEN_JUMP_TO(target, frame, next_instr, value);
}

TARGET(GEN_RESUME_STORE)
{
    PyValue value = generator_payload(ir_word);
    _PyInstr *resume = next_instr;
    if (!generator_transfer_is_plain(tstate, frame)) {
        IR_GEN_JUMP(GEN_RESUME_EFFECT, frame, resume, value);
    }
    IR_SKIP_RESUME();
    if (next_instr == resume) {
        IR_GEN_JUMP(GEN_RESUME_EFFECT, frame, next_instr, value);
    }
    _PyIRWord store = *next_instr;
    int destination = IR_B(store);
    PyValue old = frame->base[destination];
    if (_PyFrame_ObjectMayReenter(PyValue_IsObject(old)
            ? PyValue_AsKnownObjectBorrow(old) : NULL)) {
        IR_GEN_JUMP(GEN_RESUME_EFFECT, frame, next_instr, value);
    }
    INSTRUCTION_STATS(STORE_FAST);
    frame->base[destination] = value;
    _PyValue_CheckHome(value);
    bool closed = _PyValue_TryXCloseNoReentry(old);
    assert(closed);
    (void)closed;
    frame->instr_ptr = next_instr;
    ++next_instr;
    GENERATOR_DISPATCH();
}

TARGET(GEN_SEND_INPUT)
{
    PyValue value = generator_payload(ir_word);
    _PyIRWord word = *next_instr;
    int base = IR_B(word);
    PyValue receiver = frame->base[base];
    if (!generator_transfer_is_plain(tstate, frame) ||
        !PyValue_IsObject(receiver) || tstate->py_recursion_remaining <= 1) {
        IR_GEN_JUMP(GEN_SEND_INPUT_HOME, frame, next_instr, value);
    }
    PyGenObject *gen = (PyGenObject *)PyValue_AsKnownObjectBorrow(receiver);
    if ((Py_TYPE(gen) != &PyGen_Type && Py_TYPE(gen) != &PyCoro_Type) ||
        (FT_ATOMIC_LOAD_INT8_RELAXED(gen->gi_frame_state) == FRAME_CREATED &&
         !PyValue_IsNone(value)) || !gen_try_set_executing(gen)) {
        IR_GEN_JUMP(GEN_SEND_INPUT_HOME, frame, next_instr, value);
    }
    INSTRUCTION_STATS(SEND_GEN);
    STAT_INC(SEND, hit);
    frame->instr_ptr = next_instr;
    frame->return_slot = (uint16_t)(base + 2);
    frame->return_pc = next_instr + 1;
    _PyInterpreterFrame *child = &gen->gi_iframe;
    child->previous = frame;
    gen->gi_exc_state.previous_item = tstate->exc_info;
    tstate->exc_info = &gen->gi_exc_state;
    frame = tstate->current_frame = child;
    --tstate->py_recursion_remaining;
    next_instr = frame->instr_ptr;
    if (next_instr > frame->ir_words) {
        void *target = generator_resume_targets[IR_HANDLER(next_instr[-1])];
        if (target != NULL) {
            IR_GEN_JUMP_TO(target, frame, next_instr, value);
        }
    }
    IR_GEN_JUMP(GEN_RESUME_EFFECT, frame, next_instr, value);
}

TARGET(GEN_REDIRECT_EFFECT)
{
    generator_publish_redirected(tstate, frame, generator_payload(ir_word));
    frame = tstate->current_frame;
    next_instr = frame->instr_ptr;
    LLTRACE_RESUME_FRAME();
    GENERATOR_DISPATCH();
}

TARGET(GEN_RETURN_PARENT)
{
    PyValue value = generator_payload(ir_word);
    if (frame->owner != FRAME_OWNED_BY_INTERPRETER) {
        if (frame->pending_redirect) {
            IR_GEN_JUMP(GEN_REDIRECT_EFFECT, frame,
                        frame->instr_ptr, value);
        }
        void *target = generator_outcome_target(
            frame, frame->instr_ptr, generator_completed_targets);
        if (target != NULL) {
            IR_GEN_JUMP_TO(target, frame, frame->instr_ptr, value);
        }
    }
    /* Nonadmitted FOR/SEND and interpreter entry retain their own contract. */
    _PyIRStepResult completed = _PyEval_CompleteGeneratorReturn(tstate, frame, value);
    next_instr = completed.next;
    if (completed.status < 0) {
        PROCESS_ERROR_AT(RAISE, 0, next_instr - 1);
    }
    LLTRACE_RESUME_FRAME();
    GENERATOR_DISPATCH();
}

/* Source RETURN has already counted, debited and unlinked the child. Keep
 * generator routing and its legacy fallback out of the shared RETURN body. */
TARGET(GEN_RETURN_ROUTE)
{
    PyValue value = generator_payload(ir_word);
    _PyInterpreterFrame *dying = frame;
    frame = tstate->current_frame;
    if (generator_outcome_target(frame, frame->instr_ptr,
                                 generator_completed_targets) != NULL) {
        IR_GEN_JUMP(GEN_RETURN_RETIRED, dying,
                    frame->instr_ptr, value);
    }
    value = _PyEval_FrameClearAndPopWithResult(tstate, dying, value);
    frame = tstate->current_frame;
    _PyIRStepResult completed = _PyEval_CompleteGeneratorReturn(tstate, frame, value);
    next_instr = completed.next;
    if (completed.status < 0) {
        PROCESS_ERROR_AT(RAISE, 0, next_instr - 1);
    }
    LLTRACE_RESUME_FRAME();
    GENERATOR_DISPATCH();
}

/* The incoming frame has been unlinked and debited by the source RETURN.
 * Retire it as upstream does, with the result held in C, then route the
 * result to the parent. Finalizers run during the retirement can redirect
 * the parent; a changed pc or control generation is taken as a redirect. */
TARGET(GEN_RETURN_RETIRED)
{
    PyValue value = generator_payload(ir_word);
    _PyInterpreterFrame *dying = frame;
    _PyInterpreterFrame *parent = tstate->current_frame;
    _PyInstr *site = next_instr;
    uint64_t generation = generator_control_generation(parent);
    bool placed = generator_outcome_target(parent, site, generator_completed_targets) != NULL;
    _PyThreadState_UpdateLastProfiledFrame(tstate, dying, parent);
    PyGenObject *gen = _PyGen_GetGeneratorFromFrame(dying);
    FT_ATOMIC_STORE_INT8_RELEASE(gen->gi_frame_state, FRAME_CLEARED);
    assert(tstate->exc_info == &gen->gi_exc_state);
    tstate->exc_info = gen->gi_exc_state.previous_item;
    gen->gi_exc_state.previous_item = NULL;
    dying->previous = NULL;
    value = _PyFrame_ClearGenerator(gen, value);
    ((_PyThreadStateImpl *)tstate)->generator_return_kind = GENERATOR_RETURN;
    frame = tstate->current_frame;
    assert(frame == parent);
    if (placed &&
        (frame->pending_redirect || frame->instr_ptr != site ||
         generator_control_generation(frame) != generation)) {
        IR_GEN_JUMP(GEN_REDIRECT_EFFECT, frame,
                    frame->instr_ptr, value);
    }
    IR_GEN_JUMP(GEN_RETURN_PARENT, frame, site, value);
}

/* The already-validated RETURN consumes the payload without an A home. The
 * generator decision is the same source decision as ordinary RETURN. */
TARGET(GEN_RETURN)
{
    PyValue value = generator_payload(ir_word);
    frame->instr_ptr = next_instr;
    if (!generator_transfer_is_plain(tstate, frame)) {
        int home = IR_A(*next_instr);
        assert(PyValue_IsNull(frame->base[home]));
        frame->base[home] = value;
        _PyValue_CheckTemp(value);
        GENERATOR_DISPATCH();
    }
    INSTRUCTION_STATS(IR_HANDLER(*next_instr));
    _Py_LeaveRecursiveCallPy(tstate);
    _PyInterpreterFrame *dying = frame;
    int generator_return = dying->owner == FRAME_OWNED_BY_GENERATOR;
    frame = tstate->current_frame = dying->previous;
    if (generator_return) {
        IR_GEN_JUMP(GEN_RETURN_RETIRED, dying,
                    frame->instr_ptr, value);
    }
    value = _PyEval_FrameClearAndPopGeneralWithResult(tstate, dying, value);
    next_instr = frame->return_pc;
    if (_PyFrame_SetReturnValue(frame, value)) { next_instr = frame->return_pc; }
    LLTRACE_RESUME_FRAME();
    GENERATOR_DISPATCH();
}

#undef GENERATOR_DISPATCH
