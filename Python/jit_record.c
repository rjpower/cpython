#include "Python.h"
#include "pycore_jit_ir.h"
#include "pycore_long.h"
#include "pycore_pystate.h"
#include "pycore_ceval.h"
#include "pycore_pyerrors.h"

static void
restore_error(PyObject *saved)
{
    if (saved == NULL) return;
    if (PyErr_Occurred()) _PyErr_ChainExceptions1(saved);
    else PyErr_SetRaisedException(saved);
}

int
_PyJit_ReadInt32(PyValue ref, int32_t *value)
{
    if (PyValue_IsInlineInt(ref)) {
        *value = PyValue_InlineInt(ref);
        return 1;
    }
    if (PyValue_IsNull(ref) || PyValue_IsInline(ref)) return 0;
    if (PyValue_IsNull(ref)) return 0;
    if (!PyValue_IsPointerSpace(ref)) return 0;
    PyObject *object = PyValue_AsPyObjectBorrow(ref);
    if (!PyLong_CheckExact(object)) return 0;
    /* Exact PyLong conversion cannot call __index__; overflow is a guard,
     * not an exception or a request to allocate a boxed replacement. */
    int overflow = 0;
    long result = PyLong_AsLongAndOverflow(object, &overflow);
    if (overflow || !_PyValue_IntFitsInline(result)) return 0;
    *value = (int32_t)result;
    return 1;
}

static void
observe(_PyInterpreterFrame *frame, JitHome *homes, int count)
{
    PyValue *slots = _PyFrame_Slots(frame);
    for (int i = 0; i < count; i++) {
        homes[i] = (JitHome){0, JIT_HOME_OPAQUE};
        if (PyValue_IsNull(slots[i])) homes[i].kind = JIT_HOME_NULL;
        else if (_PyJit_ReadInt32(slots[i], &homes[i].value)) {
            homes[i].kind = PyValue_IsInlineInt(slots[i]) ? JIT_HOME_INLINE : JIT_HOME_BOXED;
        }
    }
}

PyObject *
_PyJit_TestRecordIndex(void)
{
    PyValue index = PyValue_TagIndex(17);
    _PyInterpreterFrame frame = {.base = &index};
    JitHome observation;
    int32_t scalar = 0;
    int accepted = _PyJit_ReadInt32(index, &scalar);
    observe(&frame, &observation, 1);
    return PyBool_FromLong(!accepted && observation.kind == JIT_HOME_OPAQUE);
}

void
_PyJit_TraceEntered(_PyJitContext *ctx)
{
    if (!ctx->unit->trace_seen) ctx->unit->trace_seen = 1;
    if (ctx->from_return && !ctx->unit->trace_return_seen) ctx->unit->trace_return_seen = 1;
}

void
_PyJit_FreeRecord(_PyJitRecord *record)
{
    if (record == NULL) return;
    PyObject *saved = PyErr_GetRaisedException();
    PyMem_Free(record);
    restore_error(saved);
    _PyJit_ReleaseBytes(sizeof(*record));
}

static int
record_valid(_PyJitContext *ctx)
{
    return ctx->unit != NULL && !ctx->unit->detached &&
        ctx->tstate->current_frame == ctx->frame &&
        ctx->code->_co_exec == ctx->expected_exec &&
        ctx->expected_exec->jit != NULL &&
        ctx->expected_exec->jit->unit == ctx->unit &&
        ctx->expected_exec->jit->generation == ctx->unit->generation &&
        (_Py_atomic_load_uintptr_relaxed(&ctx->tstate->eval_breaker) & ~_PY_EVAL_EVENTS_MASK) == ctx->unit->epoch &&
        _PyJit_SourceStamp(ctx->code) == ctx->unit->source_stamp &&
        _PyJit_ObserversAllow(ctx->tstate, ctx->frame);
}

_PyJitAction
_PyJit_RecordBefore(_PyJitContext *ctx, const _PyJitLirOp *op)
{
    ctx->record_event = NULL;
    _PyJitFlow *flow = ctx->flow_owner ? *ctx->flow_owner : NULL;
    _PyJitRecord *record = flow ? flow->record : NULL;
    if (record == NULL || record->frame != ctx->frame || record->paused ||
        record->aborted || record->closed) return _PyJIT_CONTINUE;
    if (!record_valid(ctx) || record->count == JIT_RECORD_EVENTS || record->open) {
        record->aborted = 1;
        return _PyJIT_CONTINUE;
    }
    JitRecordEvent *event = &record->events[record->count];
    event->word = op->word;
    event->decoded = op->decoded;
    event->decoded.instruction = NULL;
    event->effect = _PyJit_GetHelper(op->decoded.handler)->effect;
    observe(ctx->frame, event->before, record->nhomes);
    record->open = 1;
    ctx->record_event = record;
    ctx->record_index = (uint16_t)record->count;
    return _PyJIT_CONTINUE;
}

_PyJitAction
_PyJit_RecordAfter(_PyJitContext *ctx, _PyJitAction action, const _PyJitLirOp *op)
{
    _PyJitFlow *flow = ctx->flow_owner ? *ctx->flow_owner : NULL;
    _PyJitRecord *record = flow ? flow->record : NULL;
    /* Terminal handling must not follow the old ctx.code/exec borrows. */
    if (action != _PyJIT_CONTINUE && action != _PyJIT_BRANCH) {
        if (action != _PyJIT_ENTER_FRAME) ctx->unit->recording_exhausted = 1;
        if (record != NULL && ctx->record_event == record && ctx->record_index == record->count &&
            record->open && !record->paused && !record->aborted) {
            JitRecordEvent *event = &record->events[record->count];
            event->action = (uint8_t)action;
            event->phase = (uint8_t)ctx->phase;
            if (action == _PyJIT_ENTER_FRAME && ctx->phase == _PyJIT_ENTER_PREPARED &&
                ctx->pending_caller == record->frame) {
                record->paused = 1;
                record->pending_word = op->word;
            }
            else record->aborted = 1;
        }
        return action;
    }
    action = _PyJit_AfterHelper(ctx, action, op);
    if (action != _PyJIT_CONTINUE && action != _PyJIT_BRANCH) return action;
    if (!record_valid(ctx)) return action;
    /* A nominated but unvisited loop must not leave recorder calls in an
     * indefinitely running method. This budget belongs only to its temporary
     * recording artifact, not to unrelated baseline operations. */
    if (++ctx->unit->record_steps >= 4096) {
        if (record != NULL && record->frame == ctx->frame) {
            flow->record = NULL;
            ctx->record_event = NULL;
            _PyJit_FreeRecord(record);
            if (PyErr_Occurred()) goto error;
            action = _PyJit_AfterHelper(ctx, action, op);
            if (action != _PyJIT_CONTINUE && action != _PyJIT_BRANCH) return action;
        }
        if (_PyJit_ReplaceRecorded(ctx, NULL) < 0) goto error;
        return _PyJit_AfterHelper(ctx, action, op);
    }
    if (record != NULL && record->aborted) {
        int same_frame = record->frame == ctx->frame;
        flow->record = NULL;
        ctx->record_event = NULL;
        _PyJit_FreeRecord(record);
        record = NULL;
        if (PyErr_Occurred()) goto error;
        if (same_frame && _PyJit_ReplaceRecorded(ctx, NULL) < 0) goto error;
        if (!record_valid(ctx)) return _PyJit_AfterHelper(ctx, action, op);
    }
    if (record != NULL && ctx->record_event == record && ctx->record_index == record->count &&
        record->frame == ctx->frame && record->open && !record->aborted) {
        JitRecordEvent *event = &record->events[record->count++];
        event->successor = (uint32_t)(ctx->next_instr - ctx->frame->ir_words);
        event->action = (uint8_t)action;
        event->phase = (uint8_t)ctx->phase;
        observe(ctx->frame, event->after, record->nhomes);
        record->open = 0;
        if (event->successor == record->header) {
            record->closed = 1;
            if (_PyJit_ReplaceRecorded(ctx, record) < 0) {
                ctx->raising_instr = op->decoded.instruction;
                ctx->phase = _PyJIT_ERROR_LOG_PENDING;
                return _PyJIT_RAISE;
            }
            flow->record = NULL;
            ctx->record_event = NULL;
            _PyJit_FreeRecord(record);
            record = NULL;
            if (PyErr_Occurred()) goto error;
            /* Replacement and record release can both reenter. The old
             * activation must retire before following any exec borrow. */
            return _PyJit_AfterHelper(ctx, action, op);
        }
    }
    if (record == NULL && op->word == ctx->unit->record_backedge &&
        ctx->next_instr == ctx->frame->ir_words + ctx->unit->record_header) {
        /* This is AFTER the real periodic backedge, before the next iteration
         * reads operands. Capacity allocation is never a per-operation event. */
        if (ctx->expected_exec->jit->record_attempted) return action;
        ctx->expected_exec->jit->record_attempted = 1;
        if (_PyJit_EnsureFlow(ctx) < 0) goto error;
        flow = ctx->flow_owner ? *ctx->flow_owner : NULL;
        if (flow == NULL || !record_valid(ctx)) goto abandon;
        if (!_PyJit_ReserveBytes(sizeof(*record))) goto abandon;
        record = PyMem_Calloc(1, sizeof(*record));
        if (record == NULL) {
            _PyJit_ReleaseBytes(sizeof(*record));
            if (PyErr_Occurred()) goto error;
            goto abandon;
        }
        if (PyErr_Occurred() || !record_valid(ctx)) {
            PyObject *saved = PyErr_GetRaisedException();
            _PyJit_FreeRecord(record);
            restore_error(saved);
            if (PyErr_Occurred()) goto error;
            return _PyJit_AfterHelper(ctx, action, op);
        }
        record->frame = ctx->frame;
        record->header = ctx->unit->record_header;
        record->backedge = ctx->unit->record_backedge;
        record->source_stamp = ctx->unit->source_stamp;
        record->generation = ctx->unit->generation;
        record->nhomes = (uint16_t)_PyFrame_NumSlotsForCodeObject(ctx->code);
        record->nlocals = (uint16_t)ctx->code->co_nlocalsplus;
        flow->record = record;
    }
    return _PyJit_AfterHelper(ctx, action, op);
abandon:
    if (_PyJit_ReplaceRecorded(ctx, NULL) < 0) goto error;
    return _PyJit_AfterHelper(ctx, action, op);
error:
    ctx->raising_instr = op->decoded.instruction;
    ctx->phase = _PyJIT_ERROR_LOG_PENDING;
    return _PyJIT_RAISE;
}

void
_PyJit_RecordPending(_PyJitFlow *flow, const _PyJitPending *pending)
{
    _PyJitRecord *record = flow->record;
    if (record->paused && record->frame == pending->caller &&
        record->pending_word == pending->call_word) record->pending_token = pending->token;
}

void
_PyJit_RecordReturn(_PyJitFlow *flow, _PyInterpreterFrame *frame,
                    _PyIRWord *next, uint64_t token)
{
    _PyJitRecord *record = flow->record;
    if (!record->paused || record->frame != frame || record->pending_token != token ||
        record->aborted) return;
    PyCodeObject *code = _PyFrame_GetCode(frame);
    _PyCodeExec *exec = code->_co_exec;
    if (exec == NULL || exec->jit == NULL || exec->jit->unit == NULL ||
        exec->jit->unit->detached || exec->jit->generation != record->generation ||
        exec->jit->unit->source_stamp != record->source_stamp ||
        exec->jit->unit->epoch != (_Py_atomic_load_uintptr_relaxed(&_PyThreadState_GET()->eval_breaker) & ~_PY_EVAL_EVENTS_MASK) ||
        _PyJit_SourceStamp(code) != record->source_stamp ||
        !_PyJit_ObserversAllow(_PyThreadState_GET(), frame)) {
        record->aborted = 1;
        return;
    }
    JitRecordEvent *event = &record->events[record->count++];
    event->successor = (uint32_t)(next - frame->ir_words);
    event->phase = _PyJIT_AFTER;
    observe(frame, event->after, record->nhomes);
    record->paused = record->open = 0;
}

void
_PyJit_RecordRetire(_PyJitFlow *flow, _PyInterpreterFrame *frame)
{
    if (flow->record != NULL && flow->record->frame == frame) {
        /* frame is still linked here, before clear or handler entry. No
         * semantic owner or old native pointer is kept in the paused record. */
        _PyCodeExec *exec = _PyFrame_GetCode(frame)->_co_exec;
        if (exec != NULL && exec->jit != NULL && exec->jit->unit != NULL &&
            exec->jit->unit->source_stamp == flow->record->source_stamp)
            exec->jit->unit->recording_exhausted = 1;
        flow->record->aborted = 1;
        flow->record->frame = NULL;
    }
}

_PyJitAction
_PyJit_TraceFallback(_PyJitContext *ctx, uint32_t word)
{
    if (word >= ctx->unit->nwords) {
        PyErr_SetString(PyExc_SystemError, "invalid JIT recovery word");
        ctx->phase = _PyJIT_ERROR_LOG_PENDING;
        ctx->raising_instr = ctx->next_instr;
        return _PyJIT_RAISE;
    }
    ctx->entry_target = (char *)ctx->unit->mapping + ctx->unit->op_offsets[word];
    ctx->next_instr = ctx->frame->ir_words + word;
    ctx->phase = _PyJIT_DEOPT_BEFORE;
    return _PyJIT_BRANCH; /* emitted adapter enters the normal baseline body */
}

_PyJitAction
_PyJit_TraceAfter(_PyJitContext *ctx, _PyJitAction action, const JitSsaNode *node)
{
    /* The helper may have destroyed its old code/frame. */
    if (action != _PyJIT_CONTINUE && action != _PyJIT_BRANCH) return action;
    action = _PyJit_AfterHelper(ctx, action, &ctx->unit->ops[node->word]);
    if (action != _PyJIT_CONTINUE && action != _PyJIT_BRANCH) return action;
    uint32_t word = (uint32_t)(ctx->next_instr - ctx->frame->ir_words);
    if (word != node->successor || node->kind == JIT_SSA_POLL) {
        action = _PyJit_TraceFallback(ctx, word);
        ctx->phase = _PyJIT_AFTER;
        return action;
    }
    return _PyJIT_CONTINUE;
}

_PyJitAction
_PyJit_TraceImport(_PyJitContext *ctx, const JitSsaNode *node)
{
    int32_t scalar;
    PyValue slot = _PyFrame_Slots(ctx->frame)[node->home];
    if (!_PyJit_ReadInt32(slot, &scalar) ||
        (node->home >= ctx->unit->graph->nlocals && !PyValue_IsInlineInt(slot))) {
        return _PyJit_TraceFallback(ctx, node->word);
    }
    ctx->scratch[ctx->unit->graph->values[node->value].spill] = scalar;
    return _PyJIT_CONTINUE;
}

_PyJitAction
_PyJit_TracePublish(_PyJitContext *ctx, const JitFrameState *state)
{
    _PyJitGraph *g = ctx->unit->graph;
    PyValue *slots = _PyFrame_Slots(ctx->frame);
    /* Check the whole plan before any write. Original imported homes keep
     * their exact boxed identity; only virtual inline/NULL homes are installed. */
    for (int h = 0; h < g->nhomes; h++) {
        uint16_t id = state->value[h];
        if (id == JIT_NO_VALUE || (id < g->nvalues && g->values[id].import_home == h)) continue;
        if (!PyValue_IsNull(slots[h]) && !PyValue_IsInline(slots[h])) {
            PyErr_SetString(PyExc_SystemError, "JIT recovery attempted to discard an owner");
            ctx->raising_instr = ctx->frame->ir_words + state->word;
            ctx->phase = _PyJIT_ERROR_LOG_PENDING;
            return _PyJIT_RAISE;
        }
    }
    for (int h = 0; h < g->nhomes; h++) {
        uint16_t id = state->value[h];
        if (id == JIT_NO_VALUE || (id < g->nvalues && g->values[id].import_home == h)) continue;
        slots[h] = id == JIT_NULL_VALUE ? PyValue_NULL : PyValue_FromInt32((int32_t)ctx->scratch[g->values[id].spill]);
    }
    ctx->next_instr = ctx->frame->ir_words + state->word;
    ctx->frame->instr_ptr = ctx->next_instr;
    return _PyJIT_CONTINUE;
}

_PyJitAction
_PyJit_TraceCommit(_PyJitContext *ctx, const JitSsaNode *node)
{
    _PyJitGraph *g = ctx->unit->graph;
    _PyJitAction action = _PyJit_TracePublish(ctx, &g->states[node->before]);
    if (action != _PyJIT_CONTINUE) return action;
    PyValue *slots = _PyFrame_Slots(ctx->frame);
    for (int k = 0; k < 2; k++) if (node->clears & (1u << k)) slots[node->input_home[k]] = PyValue_NULL;
    PyValue old = slots[node->home];
    int reentrant = !PyValue_IsNull(old) && !PyValue_IsInline(old);
    slots[node->home] = PyValue_FromInt32((int32_t)ctx->scratch[g->values[node->value].spill]);
    /* Same order as IR_STORE_DST. Release may reenter or invalidate code;
     * the new local is visible and all other live values are canonical. */
    PyValue_XCLOSE(old);
    ctx->next_instr = (_PyIRWord *)((uintptr_t)ctx->frame->ir_words + node->successor * sizeof(_PyIRWord));
    ctx->phase = _PyJIT_AFTER;
    action = _PyJit_AfterHelper(ctx, _PyJIT_CONTINUE, &ctx->unit->ops[node->word]);
    if (action != _PyJIT_CONTINUE) return action;
    if (reentrant) {
        /* Publication preceded the owner release. Reimport the actual
         * canonical facts after any allocator callback; equal numeric facts
         * permit continuation while the homes retain their current identity. */
        const JitFrameState *after = &g->states[node->after];
        slots = _PyFrame_Slots(ctx->frame);
        for (int h = 0; h < g->nhomes; h++) {
            uint16_t id = after->value[h];
            int32_t actual;
            if (id == JIT_NO_VALUE) continue;
            if ((id == JIT_NULL_VALUE && PyValue_IsNull(slots[h])) ||
                (id < g->nvalues &&
                 (g->values[id].import_home == h || PyValue_IsInlineInt(slots[h])) &&
                 _PyJit_ReadInt32(slots[h], &actual) &&
                 actual == (int32_t)ctx->scratch[g->values[id].spill])) continue;
            action = _PyJit_TraceFallback(ctx, node->successor);
            ctx->phase = _PyJIT_AFTER;
            break;
        }
    }
    return action;
}
