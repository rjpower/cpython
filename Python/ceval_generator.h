/* Generator transfer entries. Included after ir_targets.h so every outgoing
 * native target has its configured calling convention declared. */

/* Private entries carry a value rather than a word. Ordinary dispatch first
 * reloads the word and needs its own local handler index. */
#define GENERATOR_DISPATCH() \
    do { \
        int opcode; \
        DISPATCH(); \
    } while (0)

static inline PyValue
generator_payload(_PyIRWord payload)
{
    return (PyValue){.bits = payload};
}

static inline uint64_t
generator_control_generation(_PyInterpreterFrame *frame)
{
    /* Interpreter entry frames do not initialize frame_obj. */
    if (frame->owner == FRAME_OWNED_BY_INTERPRETER) {
        return 0;
    }
    return frame->frame_obj == NULL ? 0 : frame->frame_obj->f_lineno_generation;
}

/* Ordinary outcomes need one dispatch-table-shaped lookup. A transparent
 * monitor keeps the same word fields but resolves its source only here, on
 * the actual observation path. */
static inline py_tail_call_funcptr
generator_outcome_target(_PyInterpreterFrame *frame, _PyInstr *site,
                         py_tail_call_funcptr *targets)
{
    if (frame->owner == FRAME_OWNED_BY_INTERPRETER) {
        return NULL;
    }
    int handler = IR_HANDLER(*site);
    py_tail_call_funcptr target = targets[handler];
    if (target == NULL && (_PyIR_Formats[handler].flags & _PyIR_TRANSPARENT)) {
        target = targets[_PyIR_RealHandler(frame, site)];
    }
    return target;
}

/* This guard contains no effect. A monitored or redirected transfer publishes
 * its homes before running the ordinary source's callback boundary. */
static inline bool
generator_transfer_is_plain(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
#ifdef Py_GIL_DISABLED
    return false;
#else
    if (frame->pending_redirect || IS_PEP523_HOOKED(tstate) ||
        _PyFrame_GetCode(frame)->_co_monitoring != NULL) {
        return false;
    }
#ifdef Py_DEBUG
    if (frame->lltrace >= 5) {
        return false;
    }
#endif
#ifdef WITH_DTRACE
    if (PyDTrace_FUNCTION_ENTRY_ENABLED() || PyDTrace_FUNCTION_RETURN_ENABLED()) {
        return false;
    }
#endif
    return true;
#endif
}

/* No callback can intervene between detaching this root and its consuming
 * close, or between publication and detachment of the same owner. */
static inline void
generator_detach_result(_PyTransitionRoots *roots)
{
    roots->result = PyValue_NULL;
    roots->result_present = 0;
}

static inline void
generator_publish_redirected(PyThreadState *tstate, _PyInterpreterFrame *frame,
                             _PyTransitionRoots *roots)
{
    if (frame->pending_redirect) {
        _PyFrame_CompleteRedirect(frame);
    }
    PyValue value = roots->result;
    PyValue old = PyValue_NULL;
    if (frame->return_slot != IR_NO_DST) {
        if (frame->return_slot < _PyFrame_GetCode(frame)->co_nlocalsplus) {
            old = frame->base[frame->return_slot];
        }
        frame->base[frame->return_slot] = value;
        _PyValue_CheckHome(value);
        generator_detach_result(roots);
        PyValue_XCLOSE(old);
    }
    else {
        generator_detach_result(roots);
        PyValue_XCLOSE(value);
    }
    frame->phase_state = 0;
}

/* The remaining-window cursor is an actual home address, not an encoded action.
 * Only this effect entry decodes the source outcome. Earlier successful closes
 * have consumed their owners and NULLed their homes. */
TARGET(GEN_COMPLETE_EFFECT)
{
    _PyValueTemps *const value_temps = NULL;
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
    int remaining_count = (int)(slots - (frame->base + base));
    assert(remaining_count >= 0 && remaining_count <= count);
    PyValue remaining[5];
    _PyTransitionPrefix prefix = {remaining, 5, 0, 0};
    _PyTransitionRoots roots = {
        .frame = frame, .kind = _Py_TRANSITION_KIND_EXTERNAL,
        .result = result, .result_present = 1, .prefix = &prefix};
    _PyThreadState_PushTransitionRoots(tstate, &roots);
    slots = _PyFrame_Slots(frame);
    if (frame->pending_redirect || frame->instr_ptr != site) {
        generator_publish_redirected(tstate, frame, &roots);
        _PyThreadState_PopTransitionRoots(tstate, &roots);
        frame = tstate->current_frame;
        slots = _PyFrame_Slots(frame);
        next_instr = frame->instr_ptr;
        GENERATOR_DISPATCH();
    }
    if (remaining_count == count && !throwing &&
        _PyFrame_GetCode(frame)->_co_monitoring != NULL) {
        _PyIRStepResult pre = _PyIR_ObserveStep(
            tstate, frame, site, exit, PyValue_Borrow(result), _PY_IR_SEND, 0);
        if (pre.status < 0 || pre.status == _PY_IR_STEP_REDIRECT) {
            PyObject *error = pre.status < 0 ? PyErr_GetRaisedException() : NULL;
            generator_detach_result(&roots);
            PyValue_XCLOSE(result);
            _PyThreadState_PopTransitionRoots(tstate, &roots);
            if (error != NULL) {
                PyErr_SetRaisedException(error);
                PROCESS_ERROR_AT(RAISE, 0, frame->instr_ptr);
            }
            next_instr = frame->instr_ptr;
            GENERATOR_DISPATCH();
        }
    }
    /* Detach every remaining owner before an overlapping A publication. */
    for (int k = remaining_count; k-- > 0;) {
        remaining[prefix.end++] = frame->base[base + k];
        frame->base[base + k] = PyValue_NULL;
    }
    PyValue displaced = PyValue_NULL;
    if (destination != IR_NO_DST) {
        if (destination < _PyFrame_GetCode(frame)->co_nlocalsplus) {
            displaced = frame->base[destination];
        }
        else {
            assert(PyValue_IsNull(frame->base[destination]));
        }
        frame->base[destination] = result;
        _PyValue_CheckHome(result);
        generator_detach_result(&roots);
    }
    remaining[prefix.end++] = displaced;
    frame->return_pc = exit;
    _PyIRStepResult post = {.next = exit, .status = _PY_IR_STEP_EXIT};
    if (_PyFrame_GetCode(frame)->_co_monitoring != NULL) {
        post = _PyIR_ObserveStep(tstate, frame, site, exit,
            PyValue_Borrow(PyValue_None), throwing ? _PY_IR_SEND_THROW : _PY_IR_SEND, 1);
    }
    if (post.status >= 0) { frame->instr_ptr = post.next; }
    PyObject *error = post.status < 0 ? PyErr_GetRaisedException() : NULL;
    while (prefix.begin < prefix.end) {
        PyValue value = remaining[prefix.begin++];
        PyValue_XCLOSE(value);
    }
    if (roots.result_present) {
        generator_detach_result(&roots);
        PyValue_XCLOSE(result);
    }
    _PyThreadState_PopTransitionRoots(tstate, &roots);
    frame = tstate->current_frame;
    slots = _PyFrame_Slots(frame);
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
            IR_GEN_TAIL(_TAIL_CALL_GEN_COMPLETE_EFFECT, frame, \
                        frame->base + base + (K) + 1, next_instr, result); \
        } \
        frame->base[base + (K)] = PyValue_NULL; \
    } while (0)
#define GENERATOR_RETIRE_SEND_INPUTS(THROW) \
    PyValue result = generator_payload(ir_word); \
    _PyIRWord source_word = *next_instr; \
    int base = IR_B(source_word); \
    if (!generator_transfer_is_plain(tstate, frame)) { \
        IR_GEN_TAIL(_TAIL_CALL_GEN_COMPLETE_EFFECT, frame, \
                    frame->base + base + ((THROW) ? 4 : 3), next_instr, result); \
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
    IR_GEN_TAIL(_TAIL_CALL_GEN_RETURN, frame, slots, next_instr, result); \
}
#define GENERATOR_COMPLETE_HOME(NAME, THROW) \
TARGET(NAME) \
{ \
    _PyValueTemps *const value_temps = NULL; \
    GENERATOR_RETIRE_SEND_INPUTS(THROW); \
    int destination = IR_A(source_word); \
    assert(PyValue_IsNull(frame->base[destination])); \
    frame->base[destination] = result; \
    _PyValue_CheckTemp(result); \
    next_instr = GENERATOR_COMPLETED_EXIT(THROW); \
    frame->instr_ptr = frame->return_pc = next_instr; \
    frame->phase_state = 0; \
    GENERATOR_DISPATCH(); \
}
#define GENERATOR_COMPLETE_STORE(NAME, THROW, EXPLICIT) \
TARGET(NAME) \
{ \
    _PyValueTemps *const value_temps = NULL; \
    GENERATOR_RETIRE_SEND_INPUTS(THROW); \
    _PyInstr *consumer = GENERATOR_COMPLETED_EXIT(THROW); \
    int destination = (EXPLICIT) ? IR_B(*consumer) : IR_A(source_word); \
    PyValue old = frame->base[destination]; \
    if (_PyFrame_ObjectMayReenter(PyValue_IsObject(old) \
            ? PyValue_AsKnownObjectBorrow(old) : NULL)) { \
        IR_GEN_TAIL(_TAIL_CALL_GEN_COMPLETE_EFFECT, frame, frame->base + base, next_instr, result); \
    } \
    if (EXPLICIT) { INSTRUCTION_STATS(STORE_FAST); } \
    frame->base[destination] = result; \
    _PyValue_CheckHome(result); \
    bool closed = _PyValue_TryXCloseNoReentry(old); \
    assert(closed); \
    (void)closed; \
    frame->instr_ptr = frame->return_pc = consumer; \
    next_instr = consumer + ((EXPLICIT) ? 1 : 0); \
    frame->phase_state = 0; \
    GENERATOR_DISPATCH(); \
}
#define GENERATOR_COMPLETE_DISCARD(NAME, THROW, EXPLICIT) \
TARGET(NAME) \
{ \
    _PyValueTemps *const value_temps = NULL; \
    GENERATOR_RETIRE_SEND_INPUTS(THROW); \
    if (!_PyValue_TryXCloseNoReentry(result)) { \
        IR_GEN_TAIL(_TAIL_CALL_GEN_COMPLETE_EFFECT, frame, frame->base + base, next_instr, result); \
    } \
    next_instr = GENERATOR_COMPLETED_EXIT(THROW); \
    frame->instr_ptr = frame->return_pc = next_instr; \
    if (EXPLICIT) { INSTRUCTION_STATS(POP_TOP); ++next_instr; } \
    frame->phase_state = 0; \
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
    _PyValueTemps *const value_temps = NULL;
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
    _PyFrame_PublishDelegatedSuspension(suspended);
    FT_ATOMIC_STORE_INT8_RELEASE(gen->gi_frame_state, FRAME_SUSPENDED_YIELD_FROM);
    slots = _PyFrame_Slots(frame);
    py_tail_call_funcptr target = generator_outcome_target(
        frame, frame->instr_ptr, generator_yielded_targets);
    if (target != NULL && !frame->pending_redirect) {
        IR_GEN_TAIL(target, frame, slots, frame->instr_ptr, result);
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
    _PyValueTemps *const value_temps = NULL;
    _PyTransitionRoots roots = {
        .frame = frame, .kind = _Py_TRANSITION_KIND_EXTERNAL,
        .result = generator_payload(ir_word), .result_present = 1};
    _PyThreadState_PushTransitionRoots(tstate, &roots);
    int home = IR_B(*next_instr) + 2;
    PyValue sent = frame->base[home];
    frame->base[home] = roots.result;
    _PyValue_CheckTemp(roots.result);
    generator_detach_result(&roots);
    frame->instr_ptr = next_instr + 1;
    frame->phase_state = 0;
    PyValue_XCLOSE(sent);
    _PyThreadState_PopTransitionRoots(tstate, &roots);
    frame = tstate->current_frame;
    slots = _PyFrame_Slots(frame);
    next_instr = frame->instr_ptr;
    GENERATOR_DISPATCH();
}

/* A private resumed input is not an ordinary handler ABI. Generic providers
 * and effects receive it once in their authoritative input home. */
TARGET(GEN_SEND_INPUT_HOME)
{
    _PyValueTemps *const value_temps = NULL;
    PyValue result = generator_payload(ir_word);
    int home = IR_B(*next_instr) + 2;
    assert(PyValue_IsNull(frame->base[home]));
    frame->base[home] = result;
    _PyValue_CheckTemp(result);
    GENERATOR_DISPATCH();
}

/* The child link is committed, but its recursion debit has not happened.
 * Error construction may allocate; the incoming owner is rooted first. */
TARGET(GEN_ENTRY_EFFECT)
{
    _PyValueTemps *const value_temps = NULL;
    _PyTransitionRoots roots = {
        .frame = frame, .kind = _Py_TRANSITION_KIND_EXTERNAL,
        .result = generator_payload(ir_word), .result_present = 1};
    _PyThreadState_PushTransitionRoots(tstate, &roots);
    if (_Py_EnterRecursivePy(tstate)) {
        PyValue value = roots.result;
        generator_detach_result(&roots);
        PyValue_XCLOSE(value);
        _PyThreadState_PopTransitionRoots(tstate, &roots);
        PROCESS_ERROR_AT(EXIT_NOTRACE, 0, frame->instr_ptr);
    }
    PyValue value = roots.result;
    generator_detach_result(&roots);
    _PyThreadState_PopTransitionRoots(tstate, &roots);
    IR_GEN_TAIL(_TAIL_CALL_GEN_RESUME_EFFECT, frame, slots,
                frame->instr_ptr, value);
}

TARGET(GEN_RESUME_EFFECT)
{
    _PyValueTemps *const value_temps = NULL;
    _PyInstr *source = frame->instr_ptr;
    uint64_t generation = generator_control_generation(frame);
    int pending = frame->pending_redirect;
    _PyTransitionRoots roots = {
        .frame = frame, .kind = _Py_TRANSITION_KIND_EXTERNAL,
        .result = generator_payload(ir_word), .result_present = 1};
    _PyThreadState_PushTransitionRoots(tstate, &roots);
    generator_publish_redirected(tstate, frame, &roots);
    _PyThreadState_PopTransitionRoots(tstate, &roots);
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
        IR_GEN_TAIL(_TAIL_CALL_GEN_RESUME_EFFECT, frame, slots, resume, value);
    }
    IR_SKIP_RESUME();
    if (next_instr == resume) {
        IR_GEN_TAIL(_TAIL_CALL_GEN_RESUME_EFFECT, frame, slots, resume, value);
    }
    _PyIRWord jump = *next_instr;
    assert(IR_HANDLER(jump) == JUMP_BACKWARD_NO_INTERRUPT);
    INSTRUCTION_STATS(JUMP_BACKWARD_NO_INTERRUPT);
    next_instr = frame->ir_words + IR_CD(jump);
    py_tail_call_funcptr target = generator_resume_targets[IR_HANDLER(*next_instr)];
    assert(target != NULL);
    IR_GEN_TAIL(target, frame, slots, next_instr, value);
}

TARGET(GEN_RESUME_STORE)
{
    _PyValueTemps *const value_temps = NULL;
    PyValue value = generator_payload(ir_word);
    _PyInstr *resume = next_instr;
    if (!generator_transfer_is_plain(tstate, frame)) {
        IR_GEN_TAIL(_TAIL_CALL_GEN_RESUME_EFFECT, frame, slots, resume, value);
    }
    IR_SKIP_RESUME();
    if (next_instr == resume) {
        IR_GEN_TAIL(_TAIL_CALL_GEN_RESUME_EFFECT, frame, slots, next_instr, value);
    }
    _PyIRWord store = *next_instr;
    int destination = IR_B(store);
    PyValue old = frame->base[destination];
    if (_PyFrame_ObjectMayReenter(PyValue_IsObject(old)
            ? PyValue_AsKnownObjectBorrow(old) : NULL)) {
        IR_GEN_TAIL(_TAIL_CALL_GEN_RESUME_EFFECT, frame, slots, next_instr, value);
    }
    INSTRUCTION_STATS(STORE_FAST);
    frame->base[destination] = value;
    _PyValue_CheckHome(value);
    bool closed = _PyValue_TryXCloseNoReentry(old);
    assert(closed);
    (void)closed;
    frame->phase_state = 0;
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
        IR_GEN_TAIL(_TAIL_CALL_GEN_SEND_INPUT_HOME, frame, slots, next_instr, value);
    }
    PyGenObject *gen = (PyGenObject *)PyValue_AsKnownObjectBorrow(receiver);
    if ((Py_TYPE(gen) != &PyGen_Type && Py_TYPE(gen) != &PyCoro_Type) ||
        (FT_ATOMIC_LOAD_INT8_RELAXED(gen->gi_frame_state) == FRAME_CREATED &&
         !PyValue_IsNone(value)) || !gen_try_set_executing(gen)) {
        IR_GEN_TAIL(_TAIL_CALL_GEN_SEND_INPUT_HOME, frame, slots, next_instr, value);
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
    slots = _PyFrame_Slots(frame);
    --tstate->py_recursion_remaining;
    next_instr = frame->instr_ptr;
    if (next_instr > frame->ir_words) {
        py_tail_call_funcptr target = generator_resume_targets[IR_HANDLER(next_instr[-1])];
        if (target != NULL) {
            IR_GEN_TAIL(target, frame, slots, next_instr, value);
        }
    }
    IR_GEN_TAIL(_TAIL_CALL_GEN_RESUME_EFFECT, frame, slots, next_instr, value);
}

TARGET(GEN_REDIRECT_EFFECT)
{
    _PyValueTemps *const value_temps = NULL;
    _PyTransitionRoots roots = {
        .frame = frame, .kind = _Py_TRANSITION_KIND_EXTERNAL,
        .result = generator_payload(ir_word), .result_present = 1};
    _PyThreadState_PushTransitionRoots(tstate, &roots);
    generator_publish_redirected(tstate, frame, &roots);
    _PyThreadState_PopTransitionRoots(tstate, &roots);
    frame = tstate->current_frame;
    slots = _PyFrame_Slots(frame);
    next_instr = frame->instr_ptr;
    LLTRACE_RESUME_FRAME();
    GENERATOR_DISPATCH();
}

TARGET(GEN_RETURN_PARENT)
{
    _PyValueTemps *const value_temps = NULL;
    PyValue value = generator_payload(ir_word);
    if (frame->owner != FRAME_OWNED_BY_INTERPRETER) {
        if (frame->pending_redirect) {
            IR_GEN_TAIL(_TAIL_CALL_GEN_REDIRECT_EFFECT, frame, slots,
                        frame->instr_ptr, value);
        }
        py_tail_call_funcptr target = generator_outcome_target(
            frame, frame->instr_ptr, generator_completed_targets);
        if (target != NULL) {
            IR_GEN_TAIL(target, frame, slots, frame->instr_ptr, value);
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
    _PyValueTemps *const value_temps = NULL;
    PyValue value = generator_payload(ir_word);
    _PyInterpreterFrame *dying = frame;
    frame = tstate->current_frame;
    if (generator_outcome_target(frame, frame->instr_ptr,
                                 generator_completed_targets) != NULL) {
        IR_GEN_TAIL(_TAIL_CALL_GEN_RETURN_RETIRED, dying, slots,
                    frame->instr_ptr, value);
    }
    value = _PyEval_FrameClearAndPopWithResult(tstate, dying, value);
    frame = tstate->current_frame;
    slots = _PyFrame_Slots(frame);
    _PyIRStepResult completed = _PyEval_CompleteGeneratorReturn(tstate, frame, value);
    next_instr = completed.next;
    if (completed.status < 0) {
        PROCESS_ERROR_AT(RAISE, 0, next_instr - 1);
    }
    LLTRACE_RESUME_FRAME();
    GENERATOR_DISPATCH();
}

/* The incoming frame has been unlinked and debited by the source RETURN.
 * Until its first effect, unfinished owners remain in the actual child. */
TARGET(GEN_RETURN_RETIRED)
{
    PyValue value = generator_payload(ir_word);
    _PyInterpreterFrame *dying = frame;
    _PyInterpreterFrame *parent = tstate->current_frame;
    _PyThreadState_UpdateLastProfiledFrame(tstate, dying, parent);
    PyGenObject *gen = _PyGen_GetGeneratorFromFrame(dying);
    FT_ATOMIC_STORE_INT8_RELEASE(gen->gi_frame_state, FRAME_CLEARED);
    assert(tstate->exc_info == &gen->gi_exc_state);
    tstate->exc_info = gen->gi_exc_state.previous_item;
    gen->gi_exc_state.previous_item = NULL;
    dying->previous = NULL;
    int stage = _PyFrame_TryClearGenerator(gen);
    if (stage > 0) {
        IR_GEN_TAIL(_TAIL_CALL_GEN_RETIRE_EFFECT_SLOTS, dying,
                    dying->base + stage, next_instr, value);
    }
    if (stage == _Py_FRAME_CLEAR_ASSOCIATION) {
        IR_GEN_TAIL(_TAIL_CALL_GEN_RETIRE_EFFECT_ASSOCIATION, dying,
                    dying->base, next_instr, value);
    }
    if (stage == _Py_FRAME_CLEAR_LOCALS) {
        IR_GEN_TAIL(_TAIL_CALL_GEN_RETIRE_EFFECT_LOCALS, dying,
                    dying->base, next_instr, value);
    }
    if (stage == _Py_FRAME_CLEAR_FUNCTION) {
        IR_GEN_TAIL(_TAIL_CALL_GEN_RETIRE_EFFECT_FUNCTION, dying,
                    dying->base, next_instr, value);
    }
    assert(stage == _Py_FRAME_CLEAR_DONE);
    if (_PyFrame_ObjectMayReenter(gen->gi_exc_state.exc_value)) {
        IR_GEN_TAIL(_TAIL_CALL_GEN_RETIRE_EFFECT_EXCEPTION, dying,
                    dying->base, next_instr, value);
    }
    _PyErr_ClearExcState(&gen->gi_exc_state);
    ((_PyThreadStateImpl *)tstate)->generator_return_kind = GENERATOR_RETURN;
    IR_GEN_TAIL(_TAIL_CALL_GEN_RETURN_PARENT, parent, _PyFrame_Slots(parent),
                next_instr, value);
}

/* Finish the child's remaining suffix, then replace this native frame with
 * the materialized completion suffix. No old outcome target is rearmed. */
#define GENERATOR_RETIRE_EFFECT(STAGE) \
    _PyValueTemps *const value_temps = NULL; \
    _PyInterpreterFrame *dying = frame; \
    _PyInterpreterFrame *parent = tstate->current_frame; \
    _PyInstr *site = next_instr; \
    uint64_t generation = generator_control_generation(parent); \
    bool placed = generator_outcome_target(parent, site, generator_completed_targets) != NULL; \
    int stage = (STAGE); \
    _PyTransitionRoots roots; \
    _PyFrame_InitClearRoots(dying, stage, _PyFrame_NumSlots(dying), \
                           generator_payload(ir_word), &roots); \
    roots.retire_reason = _PyIR_RETIRE_NORMAL_RETURN; \
    _PyThreadState_PushTransitionRoots(tstate, &roots); \
    roots.shadow_status = _PyIR_ObserveShadowRetirement( \
        dying, _PyIR_RETIRE_NORMAL_RETURN); \
    PyGenObject *gen = _PyGen_GetGeneratorFromFrame(dying); \
    Py_BEGIN_CRITICAL_SECTION(gen); \
    if (stage != _Py_FRAME_CLEAR_DONE) { \
        _PyFrame_FinishClearExceptCode(dying, stage, &roots); \
    } \
    Py_END_CRITICAL_SECTION(); \
    roots.header_mask &= ~_Py_TRANSITION_EXCEPTION; \
    _PyErr_ClearExcState(&gen->gi_exc_state); \
    roots.header_mask &= ~_Py_TRANSITION_CODE; \
    PyValue value = roots.result; \
    generator_detach_result(&roots); \
    _PyThreadState_PopTransitionRoots(tstate, &roots); \
    ((_PyThreadStateImpl *)tstate)->generator_return_kind = GENERATOR_RETURN; \
    frame = tstate->current_frame; \
    slots = _PyFrame_Slots(frame); \
    if (placed) { \
        if (frame != parent || frame->pending_redirect || \
            frame->instr_ptr != site || \
            generator_control_generation(frame) != generation) { \
            IR_GEN_TAIL(_TAIL_CALL_GEN_REDIRECT_EFFECT, frame, slots, \
                        frame->instr_ptr, value); \
        } \
        IR_GEN_TAIL(_TAIL_CALL_GEN_COMPLETE_EFFECT, frame, frame->base + IR_B(*site) + 3, site, value); \
    } \
    _PyIRStepResult completed = _PyEval_CompleteGeneratorReturn(tstate, frame, value); \
    next_instr = completed.next; \
    if (completed.status < 0) { PROCESS_ERROR_AT(RAISE, 0, next_instr - 1); } \
    LLTRACE_RESUME_FRAME(); \
    GENERATOR_DISPATCH()

TARGET(GEN_RETIRE_EFFECT_SLOTS)
{
    GENERATOR_RETIRE_EFFECT((int)(slots - frame->base));
}
TARGET(GEN_RETIRE_EFFECT_ASSOCIATION)
{
    GENERATOR_RETIRE_EFFECT(_Py_FRAME_CLEAR_ASSOCIATION);
}
TARGET(GEN_RETIRE_EFFECT_LOCALS)
{
    GENERATOR_RETIRE_EFFECT(_Py_FRAME_CLEAR_LOCALS);
}
TARGET(GEN_RETIRE_EFFECT_FUNCTION)
{
    GENERATOR_RETIRE_EFFECT(_Py_FRAME_CLEAR_FUNCTION);
}
TARGET(GEN_RETIRE_EFFECT_EXCEPTION)
{
    GENERATOR_RETIRE_EFFECT(_Py_FRAME_CLEAR_DONE);
}
#undef GENERATOR_RETIRE_EFFECT

/* The already-validated RETURN consumes the payload without an A home. The
 * generator decision is the same source decision as ordinary RETURN. */
TARGET(GEN_RETURN)
{
    _PyValueTemps *const value_temps = NULL;
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
        IR_GEN_TAIL(_TAIL_CALL_GEN_RETURN_RETIRED, dying, _PyFrame_Slots(dying),
                    frame->instr_ptr, value);
    }
    value = _PyEval_FrameClearAndPopGeneralWithResult(tstate, dying, value);
    slots = _PyFrame_Slots(frame);
    next_instr = frame->return_pc;
    if (_PyFrame_SetReturnValue(frame, value)) { next_instr = frame->return_pc; }
    LLTRACE_RESUME_FRAME();
    GENERATOR_DISPATCH();
}

#undef GENERATOR_DISPATCH
