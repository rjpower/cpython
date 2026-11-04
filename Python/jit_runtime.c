#include "Python.h"
#include "pycore_jit.h"
#include "pycore_jit_ir.h"
#include "pycore_pystate.h"
#include "pycore_function.h"

/* Stack-owned and thread-local. An already claimed request cannot be inherited
 * by recursive evaluator entries or a callback, even for the same code. */
static _Py_thread_local _PyJitTestRequest *test_request;
/* Test-only request: claimed once by the ordinary evaluator activation. The
 * callback cannot inherit it, and is disarmed before any Python invocation. */
typedef struct JitColdRequest {
    struct JitColdRequest *previous;
    PyThreadState *tstate;
    PyCodeObject *code;
    _PyInterpreterFrame *frame;
    PyObject *callback;
    int site, fail, claimed, used;
    unsigned int blocked;
} JitColdRequest;
static _Py_thread_local JitColdRequest *cold_request;
static void *initial_calloc(PyCodeObject *, int);
static int take_bootstrap_cooldown(void);

_PyJitTestRequest *
_PyJit_TestClaim(PyThreadState *tstate, _PyInterpreterFrame *frame, void **cold_token)
{
    if (cold_request != NULL && !cold_request->claimed && cold_request->tstate == tstate &&
        cold_request->code == _PyFrame_GetCode(frame)) {
        cold_request->claimed = 1;
        cold_request->frame = frame;
        *cold_token = cold_request;
    }
    _PyJitTestRequest *request = test_request;
    if (request == NULL || request->claimed || request->tstate != tstate ||
        request->code != _PyFrame_GetCode(frame)) {
        return NULL;
    }
    request->claimed = 1;
    request->claimed_frame = frame;
    return request;
}

PyObject *
_PyJit_TestCall(PyObject *function, PyObject *args, Py_ssize_t limit)
{
#if _Py_TAIL_CALL_INTERP || defined(Py_STATS) || defined(Py_GIL_DISABLED)
    PyErr_SetString(PyExc_NotImplementedError, "semantic-step fixture requires the GIL non-stats default evaluator");
    return NULL;
#endif
    if (!PyFunction_Check(function) || !PyTuple_Check(args) || limit < 1) {
        PyErr_SetString(PyExc_ValueError, "expected function, argument tuple and positive step limit");
        return NULL;
    }
    PyCodeObject *code = (PyCodeObject *)PyFunction_GetCode(function);
    Py_INCREF(code);
    _PyJitTestRequest request = {
        .previous = test_request, .tstate = _PyThreadState_GET(),
        .code = code, .remaining = limit,
    };
    test_request = &request;
    PyObject *result = PyObject_Call(function, args, NULL);
    test_request = request.previous;
    request.claimed_frame = NULL;
    Py_DECREF(code);
    if (result == NULL) {
        return NULL;
    }
    PyObject *actions = PyTuple_New(_PyJIT_ACTION_COUNT);
    PyObject *phases = PyTuple_New(_PyJIT_PHASE_COUNT);
    if (actions == NULL || phases == NULL) {
        Py_XDECREF(actions);
        Py_XDECREF(phases);
        Py_DECREF(result);
        return NULL;
    }
    for (int i = 0; i < _PyJIT_ACTION_COUNT + _PyJIT_PHASE_COUNT; i++) {
        PyObject *tuple = i < _PyJIT_ACTION_COUNT ? actions : phases;
        int index = i < _PyJIT_ACTION_COUNT ? i : i - _PyJIT_ACTION_COUNT;
        Py_ssize_t count = i < _PyJIT_ACTION_COUNT ? request.actions[index] : request.phases[index];
        PyObject *value = PyLong_FromSsize_t(count);
        if (value == NULL) {
            Py_DECREF(actions);
            Py_DECREF(phases);
            Py_DECREF(result);
            return NULL;
        }
        PyTuple_SET_ITEM(tuple, index, value);
    }
    return Py_BuildValue("NNN", result, actions, phases);
}

/* Native artifacts contain scalar metadata and code indices, never hidden
 * strong Python-object dependencies. Live frames own code; activations pin
 * only artifacts, so completed returns retain normal constant lifetimes. */
#include "pycore_ceval.h"
#include "pycore_instruments.h"
#include "pycore_pyerrors.h"
#include "pycore_opcode_utils.h"
#if _PyJIT_BASELINE_SUPPORTED
#  include <sys/mman.h>
#endif

static _PyJitState initial_building = {.building = 1};
/* Charge cold compiler allocations before callbacks and retain every charge
 * through physical release, including unpublished and retired artifacts. */
static Py_ssize_t process_units;
static Py_ssize_t process_bytes;
static int automatic_building;
static int bootstrap_cooldown;
/* Hot-call and backedge admission is off by default. The baseline runs every
 * word through its generic helper and ignores interpreter quickening, so an
 * admitted method runs several times slower than the specialising
 * interpreter. Tests enable it; explicit compilation does not need it. It is
 * process-wide, like the build lock and bootstrap cooldown above. */
static int natural_admission;

static int
reserve_bytes(size_t bytes)
{
    if (bytes > 8 * 1024 * 1024) {
        return 0;
    }
    Py_ssize_t total = _Py_atomic_load_ssize_relaxed(&process_bytes);
    do {
        if (total > 8 * 1024 * 1024 - (Py_ssize_t)bytes) {
            return 0;
        }
    } while (!_Py_atomic_compare_exchange_ssize(&process_bytes, &total, total + (Py_ssize_t)bytes));
    return 1;
}

int _PyJit_ReserveBytes(size_t bytes) { return reserve_bytes(bytes); }
void _PyJit_ReleaseBytes(size_t bytes)
{
    _Py_atomic_add_ssize(&process_bytes, -(Py_ssize_t)bytes);
}

static int
reserve_budget(_PyJitCodeUnit *unit)
{
    Py_ssize_t n = _Py_atomic_load_ssize_relaxed(&process_units);
    do { if (n >= 64) return 0; }
    while (!_Py_atomic_compare_exchange_ssize(&process_units, &n, n + 1));
    unit->budgeted = 1;
    return 1;
}

static void
restore_entry(_PyCodeExec *exec, _PyJitCodeUnit *unit)
{
    if (unit != NULL && unit->entry_word < exec->nwords &&
        IR_HANDLER(exec->words[unit->entry_word]) == RESUME_JIT) {
        _PyIR_SetHandler(exec->words + unit->entry_word, unit->entry_handler);
    }
}
static void release_jit_state(_PyJitState *);
static void free_state_storage(_PyJitState *);
static void restore_jit_error(PyObject *);

static uintptr_t
jit_epoch(PyThreadState *tstate)
{
    return _Py_atomic_load_uintptr_relaxed(&tstate->eval_breaker) & ~_PY_EVAL_EVENTS_MASK;
}

static uint64_t
jit_source_stamp(PyCodeObject *code)
{
    uint64_t stamp = UINT64_C(1469598103934665603);
    const unsigned char *bytes = (const unsigned char *)_PyCode_WORDS(code);
    size_t size = (size_t)Py_SIZE(code) * sizeof(_PyIRWord) +
                  (size_t)code->co_nfeedback * sizeof(uint32_t);
    for (size_t i = 0; i < size; i++) {
        stamp = (stamp ^ bytes[i]) * UINT64_C(1099511628211);
    }
    return stamp ^ (uint64_t)code->co_nlocalsplus;
}

uint64_t _PyJit_SourceStamp(PyCodeObject *code) { return jit_source_stamp(code); }

void
_PyJit_FreeUnit(_PyJitCodeUnit *unit)
{
    if (unit == NULL) {
        return;
    }
    assert(unit->refs > 0);
    if (--unit->refs) {
        return;
    }
    size_t charged = unit->charged_bytes;
    int budgeted = unit->budgeted;
    PyObject *saved = PyErr_GetRaisedException();
#if _PyJIT_BASELINE_SUPPORTED
    if (unit->mapping != NULL) {
        munmap(unit->mapping, unit->mapping_size);
    }
#endif
    PyMem_Free(unit->offsets);
    restore_jit_error(saved);
    saved = PyErr_GetRaisedException();
    PyMem_Free(unit->ops);
    restore_jit_error(saved);
    saved = PyErr_GetRaisedException();
    PyMem_Free(unit->op_offsets);
    restore_jit_error(saved);
    saved = PyErr_GetRaisedException();
    _PyJit_FreeGraph(unit->graph);
    restore_jit_error(saved);
    saved = PyErr_GetRaisedException();
    PyMem_Free(unit);
    restore_jit_error(saved);
    if (budgeted) {
        _Py_atomic_add_ssize(&process_units, -1);
    }
    _PyJit_ReleaseBytes(charged);
}

void
_PyJit_DetachExec(_PyCodeExec *exec)
{
    if (exec == NULL || exec->jit == NULL) {
        return;
    }
    if (exec->jit == &initial_building) {
        exec->jit = NULL;
        return;
    }
    _PyJitState *state = exec->jit;
    state->generation++;
    _PyJitCodeUnit *unit = state->unit;
    state->unit = NULL;
    restore_entry(exec, unit);
    if (unit != NULL) {
        unit->detached = 1;
        _PyJit_FreeUnit(unit);
    }
}

void
_PyJit_ClearExec(_PyCodeExec *exec)
{
    _PyJitState *state = exec->jit;
    exec->jit = NULL;
    if (state == NULL || state == &initial_building) {
        return;
    }
    _PyJitCodeUnit *unit = state->unit;
    state->unit = NULL;
    restore_entry(exec, unit);
    if (unit != NULL) {
        unit->detached = 1;
        _PyJit_FreeUnit(unit);
    }
    release_jit_state(state);
}

size_t
_PyJit_ExecSizeOf(_PyCodeExec *exec)
{
    _PyJitState *state = exec->jit;
    if (state == NULL || state == &initial_building) {
        return 0;
    }
    size_t size = sizeof(*state);
    _PyJitCodeUnit *unit = state->unit;
    if (unit != NULL) {
        size += sizeof(*unit) + unit->nops * (sizeof(*unit->ops) + sizeof(*unit->op_offsets));
        size += unit->nwords * sizeof(*unit->offsets);
        if (unit->graph != NULL) size += sizeof(*unit->graph);
        size += unit->mapping_size;
    }
    return size;
}

typedef struct JitBuildRequest {
    struct JitBuildRequest *previous;
    PyCodeObject *code;
    PyThreadState *tstate;
} JitBuildRequest;
static _Py_thread_local JitBuildRequest *building_request;

static void
restore_jit_error(PyObject *saved)
{
    if (saved != NULL) {
        if (PyErr_Occurred()) {
            _PyErr_ChainExceptions1(saved);
        }
        else {
            PyErr_SetRaisedException(saved);
        }
    }
}

static void
free_state_storage(_PyJitState *state)
{
    int budgeted = state != NULL && state->budgeted;
    PyMem_Free(state);
    if (budgeted) {
        _Py_atomic_add_ssize(&process_bytes, -(Py_ssize_t)sizeof(*state));
    }
}

static void
release_jit_state(_PyJitState *state)
{
    assert(state->refs > 0);
    if (--state->refs == 0) {
        PyObject *saved = PyErr_GetRaisedException();
        free_state_storage(state);
        restore_jit_error(saved);
    }
}

int
_PyJit_CompileCode(PyCodeObject *code)
{
#if !_PyJIT_BASELINE_SUPPORTED
    PyErr_SetString(PyExc_NotImplementedError, "method baseline requires Linux x86-64 GIL/default evaluator");
    return -1;
#else
    PyThreadState *tstate = _PyThreadState_GET();
    if (PyErr_Occurred()) {
        return -1;
    }
    for (JitBuildRequest *item = building_request; item; item = item->previous) {
        if (item->code == code && item->tstate == tstate) {
            return 0;
        }
    }
    /* This guard includes GetExec/Instrument and cleanup, not only backend
     * allocation. The cold source root has no native-return lifetime effect. */
    Py_INCREF(code);
    JitBuildRequest request = {building_request, code, tstate};
    building_request = &request;
    _PyCodeExec *exec = NULL;
    _PyJitState *state = NULL;
    _PyJitCodeUnit *unit = NULL;
    int installed = 0;
    int attempted = 0;
    exec = _PyCode_GetExec(code);
    if (exec == NULL || PyErr_Occurred() || _Py_Instrument(code, tstate->interp) < 0 || PyErr_Occurred()) {
        goto done;
    }
    exec = code->_co_exec;
    if (exec == NULL || exec->jit == &initial_building ||
        (exec->jit && (exec->jit->building || exec->jit->disabled))) {
        goto done;
    }
    uintptr_t epoch = jit_epoch(tstate);
    uint64_t source = jit_source_stamp(code);
    if (exec->jit == NULL) {
        exec->jit = &initial_building;
        _PyJitState *fresh = initial_calloc(code, 0);
        if ((fresh != NULL && !fresh->budgeted) || PyErr_Occurred() || code->_co_exec != exec ||
            exec->jit != &initial_building || epoch != jit_epoch(tstate) ||
            source != jit_source_stamp(code)) {
            if (code->_co_exec == exec && exec->jit == &initial_building) {
                exec->jit = NULL;
            }
            PyObject *saved = PyErr_GetRaisedException();
            free_state_storage(fresh);
            restore_jit_error(saved);
            goto done;
        }
        exec->jit = fresh;
        if (fresh == NULL) {
            goto done;
        }
        fresh->refs = 1;  /* exec ownership */
    }
    state = exec->jit;
    state->refs++;  /* survives an explicit exec clear during allocator reentry */
    if (state->unit != NULL) {
        installed = state->unit->interpreter_id == PyInterpreterState_GetID(tstate->interp);
        goto done;
    }
    state->building = 1;
    state->last_failure = _PyJIT_FAILURE_NONE;
    attempted = 1;
    PyTime_t compile_start = 0, compile_stop = 0;
    /* Descriptor/frontend plus backend time, excluding earlier nomination,
     * initial state allocation and instrumentation. Whole-process diagnostics
     * account for those costs separately. */
    (void)PyTime_PerfCounterRaw(&compile_start);
    if (state->compile_attempts < UINT32_MAX) {
        state->compile_attempts++;
    }
    uint64_t generation = state->generation;
    unit = _PyJit_BuildBaseline(code, exec, generation, epoch, source);
    (void)PyTime_PerfCounterRaw(&compile_stop);
    if (compile_stop >= compile_start) {
        state->compile_ns += (uint64_t)(compile_stop - compile_start);
    }
    int valid = !PyErr_Occurred() && code->_co_exec == exec && exec->jit == state &&
                state->generation == generation && !state->disabled && epoch == jit_epoch(tstate) &&
                source == jit_source_stamp(code);
    state->last_failure = !valid ? _PyJIT_FAILURE_INVALIDATED :
        unit == NULL ? _PyJIT_FAILURE_BUILD : _PyJIT_FAILURE_NONE;
    if (code->_co_exec == exec && exec->jit == state) {
        state->building = 0;
        int reserved = valid && unit != NULL && reserve_budget(unit);
        if (valid && unit != NULL && !reserved) {
            state->last_failure = _PyJIT_FAILURE_QUOTA;
        }
        if (reserved) {
            int entry = code->_co_firsttraceable;
            if (entry < 0 || (uint32_t)entry >= exec->nwords ||
                _PyCode_InstrHandler(code, entry) != RESUME ||
                (_PyCode_InstrOparg(code, entry) & RESUME_OPARG_LOCATION_MASK) != RESUME_AT_FUNC_START ||
                IR_HANDLER(exec->words[entry]) != RESUME) {
                state->last_failure = _PyJIT_FAILURE_ENTRY;
                goto done;
            }
            unit->entry_word = (uint32_t)entry;
            unit->entry_handler = (uint16_t)_PyCode_GenericHandler(code, entry);
            unit->interpreter_id = PyInterpreterState_GetID(tstate->interp);
            state->unit = unit;
            _PyIR_SetHandler(exec->words + unit->entry_word, RESUME_JIT);
            unit = NULL;
            installed = 1;
        }
    }
    done:
    {
        if (attempted && !installed && state->compile_failures < UINT32_MAX) {
            state->compile_failures++;
        }
        PyObject *saved = PyErr_GetRaisedException();
        _PyJit_FreeUnit(unit);
        if (state != NULL) {
            release_jit_state(state);
        }
        Py_DECREF(code);
        restore_jit_error(saved);
        building_request = request.previous;
    }
    return PyErr_Occurred() ? -1 : installed;
#endif
}

int
_PyJit_ReplaceRecorded(_PyJitContext *ctx, _PyJitRecord *record)
{
    /* Only a completed nonterminal operation calls this cold transaction.
     * Root code during construction, never across a language frame transfer. */
    PyCodeObject *code = ctx->code;
    _PyCodeExec *exec = code->_co_exec;
    if (exec != ctx->expected_exec || exec->jit == NULL || exec->jit->unit != ctx->unit ||
        exec->jit->building || (record && (record->source_stamp != jit_source_stamp(code) ||
                                         record->generation != exec->jit->generation))) return 0;
    _PyJitState *state = exec->jit;
    state->refs++;
    state->building = 1;
    state->record_attempted = 1;
    Py_INCREF(code);
    JitBuildRequest request = {building_request, code, ctx->tstate};
    building_request = &request;
    uint64_t generation = state->generation, source = jit_source_stamp(code);
    uintptr_t epoch = jit_epoch(ctx->tstate);
    PyTime_t compile_start = 0, compile_stop = 0;
    (void)PyTime_PerfCounterRaw(&compile_start);
    if (state->compile_attempts < UINT32_MAX) state->compile_attempts++;
    _PyJitGraph *graph = record ? _PyJit_BuildGraph(record) : NULL;
    _PyJitCodeUnit *fresh = NULL;
    if (!PyErr_Occurred()) fresh = _PyJit_BuildOptimized(code, exec, generation, epoch, source, graph);
    else _PyJit_FreeGraph(graph);
    (void)PyTime_PerfCounterRaw(&compile_stop);
    if (compile_stop >= compile_start) state->compile_ns += (uint64_t)(compile_stop - compile_start);
    int valid = !PyErr_Occurred() && code->_co_exec == exec && exec->jit == state &&
        state->unit == ctx->unit && state->generation == generation &&
        source == jit_source_stamp(code) && epoch == jit_epoch(ctx->tstate) &&

        _PyJit_ObserversAllow(ctx->tstate, ctx->frame);
    int installed = valid && fresh != NULL && reserve_budget(fresh);
    state->last_failure = !valid ? _PyJIT_FAILURE_INVALIDATED :
        fresh == NULL ? _PyJIT_FAILURE_BUILD : !installed ? _PyJIT_FAILURE_QUOTA : _PyJIT_FAILURE_NONE;
    if (!installed && state->compile_failures < UINT32_MAX) state->compile_failures++;
    if (installed) {
        fresh->entry_word = ctx->unit->entry_word;
        fresh->entry_handler = ctx->unit->entry_handler;
        fresh->interpreter_id = ctx->unit->interpreter_id;
        _PyJitCodeUnit *old = state->unit;
        restore_entry(exec, old);
        state->unit = fresh;
        _PyIR_SetHandler(exec->words + fresh->entry_word, RESUME_JIT);
        fresh = NULL;
        old->detached = 1;
        _PyJit_FreeUnit(old); /* active ctx still pins its return address */
    }
    else if (code->_co_exec == exec && exec->jit == state && state->unit == ctx->unit) {
        _PyJit_DetachExec(exec); /* next natural compile is ordinary baseline */
    }
    if (code->_co_exec == exec && exec->jit == state) state->building = 0;
    PyObject *saved = PyErr_GetRaisedException();
    _PyJit_FreeUnit(fresh);
    restore_jit_error(saved);
    saved = PyErr_GetRaisedException();
    release_jit_state(state);
    restore_jit_error(saved);
    saved = PyErr_GetRaisedException();
    Py_DECREF(code);
    restore_jit_error(saved);
    building_request = request.previous;
    return PyErr_Occurred() ? -1 : 0;
}

int
_PyJit_PrepareActivation(PyThreadState *tstate, _PyInterpreterFrame *frame,
                         _PyIRWord *next_instr, int from_return, _PyJitContext *ctx)
{
#if !_PyJIT_BASELINE_SUPPORTED
    return 0;
#else
    assert(ctx->unit == NULL && ctx->code == NULL);
    if (PyErr_Occurred()) {
        return -1;
    }
    if (frame->owner == FRAME_OWNED_BY_INTERPRETER) {
        return 0;
    }
    PyCodeObject *code = _PyFrame_GetCode(frame);
    _PyCodeExec *exec = code->_co_exec;
    if (exec == NULL || exec->jit == NULL || exec->jit == &initial_building ||
        exec->jit->building || exec->jit->unit == NULL || frame->ir_words != exec->words) {
        return 0;
    }
    _PyJitCodeUnit *unit = exec->jit->unit;
    Py_ssize_t word = next_instr - exec->words;
    if (unit->detached || word < 0 || word >= unit->nwords ||
        tstate->current_frame != frame ||

        unit->interpreter_id != PyInterpreterState_GetID(tstate->interp) ||
        !_PyJit_ObserversAllow(tstate, frame) ||
        unit->epoch != jit_epoch(tstate) || unit->generation != exec->jit->generation) {
        return 0;
    }
    if (unit->recording_exhausted) {
        if (unit->source_stamp != jit_source_stamp(code)) return 0;
        /* The old RETURN touched only pinned metadata. A new live frame now
         * supplies the code owner for this one cold baseline settlement. */
        unit->refs++;
        _PyJitContext temporary = {.tstate = tstate, .frame = frame,
            .next_instr = next_instr, .unit = unit, .code = code, .expected_exec = exec};
        int replaced = _PyJit_ReplaceRecorded(&temporary, NULL);
        int released = _PyJit_ReleaseActivation(&temporary);
        if (replaced < 0 || released < 0) return -1;

        /* Release can reenter too. Never follow the old unit or derive an
         * address from the old raw PC after the cold transaction. */
        if (tstate->current_frame != frame || _PyFrame_GetCode(frame) != code ||
            code->_co_exec != exec || frame->ir_words != exec->words ||
            word >= exec->nwords) {
            PyErr_SetString(PyExc_SystemError, "JIT recorder settlement changed executing words");
            return -1;
        }
        /* RESUME already served its poll. Only synchronize a changed epoch;
         * ordinary fallback must see the current pending word's handler. */
        if (code->_co_instrumentation_version != jit_epoch(tstate) &&
            (_Py_Instrument(code, tstate->interp) < 0 || PyErr_Occurred())) return -1;
        if (tstate->current_frame != frame || _PyFrame_GetCode(frame) != code ||
            code->_co_exec != exec || frame->ir_words != exec->words ||
            word >= exec->nwords) {
            PyErr_SetString(PyExc_SystemError, "JIT recorder synchronization changed executing words");
            return -1;
        }
        if (exec->jit == NULL || exec->jit == &initial_building ||
            exec->jit->building || exec->jit->unit == NULL) return 0;
        unit = exec->jit->unit;
        if (unit->detached || unit->recording || unit->recording_exhausted ||
            unit->graph != NULL || word >= unit->nwords ||
            unit->source_stamp != jit_source_stamp(code) ||
            unit->interpreter_id != PyInterpreterState_GetID(tstate->interp) ||
            unit->epoch != jit_epoch(tstate) || unit->generation != exec->jit->generation ||
            !_PyJit_ObserversAllow(tstate, frame) || unit->offsets[word] < 0 ||
            (size_t)unit->offsets[word] + 4 > unit->code_size) return 0;
        next_instr = exec->words + word;
    }
    unit->refs++;
    *ctx = (_PyJitContext){.tstate = tstate, .frame = frame, .next_instr = next_instr,
        .unit = unit, .code = code, .expected_exec = exec,
        .entry_target = (char *)unit->mapping + unit->offsets[word],
        .from_return = from_return};
    return 1;
#endif
}

int
_PyJit_ReleaseActivation(_PyJitContext *ctx)
{
    _PyJitCodeUnit *unit = ctx->unit;
    /* Clear every borrowed/owned native pointer before any reentrant free.
     * No old exec, state or dying frame is consulted on this path. */
    ctx->unit = NULL;
    ctx->code = NULL;
    ctx->expected_exec = NULL;
    ctx->entry_target = NULL;
    ctx->pending_caller = NULL;
    ctx->pending_call = ctx->pending_return = 0;
    ctx->cold_request = NULL;
    ctx->scratch = NULL;
    ctx->record_event = NULL;
    PyObject *saved = PyErr_GetRaisedException();
    _PyJit_FreeUnit(unit);
    if (saved != NULL) {
        if (PyErr_Occurred()) {
            _PyErr_ChainExceptions1(saved);
        }
        else {
            PyErr_SetRaisedException(saved);
        }
    }
    return PyErr_Occurred() ? -1 : 0;
}

void
_PyJit_NativeEntered(_PyJitContext *ctx)
{
    ctx->unit->entry_seen = 1;
    if (ctx->from_return) {
        ctx->unit->return_resume_seen = 1;
    }
}

static void
disable_active_unit(_PyJitContext *ctx, int unsupported)
{
    _PyCodeExec *exec = ctx->code->_co_exec;
    if (exec != ctx->expected_exec || exec->jit == NULL || exec->jit == &initial_building) {
        return;
    }
    _PyJitState *state = exec->jit;
    if (unsupported) {
        /* Immutable missing semantics cannot become supported by recompiling
         * while this activation is interpreting its remainder. */
        state->disabled = 1;
    }
    if (unsupported || state->unit == ctx->unit) {
        _PyJit_DetachExec(exec);
    }
}

_PyJitAction
_PyJit_AfterHelper(_PyJitContext *ctx, _PyJitAction action, const _PyJitLirOp *op)
{
    /* RETURN/ENTER can already have cleared or switched the old frame.
     * In particular do not inspect expected_exec before this terminal test. */
    if (action != _PyJIT_CONTINUE && action != _PyJIT_BRANCH) {
        return action;
    }
    if (op->decoded.handler == CALL) {
        ctx->unit->c_call_seen = 1;
    }
    if (PyErr_Occurred()) {
        ctx->raising_instr = op->decoded.instruction;
        ctx->phase = _PyJIT_ERROR_LOG_PENDING;
        return _PyJIT_RAISE;
    }
    PyCodeObject *code = ctx->code;
    _PyCodeExec *exec = code->_co_exec;
    if (ctx->unit->detached || exec != ctx->expected_exec || exec->jit == NULL ||
        exec->jit == &initial_building || exec->jit->unit != ctx->unit ||
        exec->jit->generation != ctx->unit->generation ||
        jit_epoch(ctx->tstate) != ctx->unit->epoch ||
        ctx->tstate->current_frame != ctx->frame ||
        !_PyJit_ObserversAllow(ctx->tstate, ctx->frame)) {
        disable_active_unit(ctx, 0);
        ctx->phase = _PyJIT_DEOPT_AFTER;
        return _PyJIT_DEOPT;
    }
    /* Helpers own semantic PC changes, including retry and marker skipping.
     * Only the descriptor's validated edges may stay in compiled control flow. */
    if (ctx->next_instr != exec->words + op->fallthrough &&
        (op->kind != _PyJIT_LIR_BRANCH || ctx->next_instr != exec->words + op->target)) {
        disable_active_unit(ctx, 0);
        ctx->phase = _PyJIT_DEOPT_AFTER;
        return _PyJIT_DEOPT;
    }
    return action;
}

_PyJitAction
_PyJit_ExitBefore(_PyJitContext *ctx, const _PyJitDecodedOp *op)
{
    disable_active_unit(ctx, 1);
    ctx->next_instr = op->instruction;
    ctx->raising_instr = NULL;
    ctx->phase = _PyJIT_DEOPT_BEFORE;
    return _PyJIT_DEOPT;
}

PyObject *
_PyJit_CodeInfo(PyCodeObject *code)
{
    _PyCodeExec *exec = code->_co_exec;
    _PyJitState *state = exec ? exec->jit : NULL;
    _PyJitCodeUnit *unit = state && state != &initial_building ? state->unit : NULL;
    Py_ssize_t units = _Py_atomic_load_ssize_relaxed(&process_units);
    Py_ssize_t charged = _Py_atomic_load_ssize_relaxed(&process_bytes);
    int bootstrap = _Py_atomic_load_int_relaxed(&bootstrap_cooldown);
    if (unit == NULL) {
        int ordinary = state != NULL && state != &initial_building;
        return Py_BuildValue("{s:O,s:O,s:i,s:i,s:i,s:i,s:i,s:I,s:I,s:K,s:i,s:n,s:n,s:i}", "active", Py_False,
            "disabled", ordinary && state->disabled ? Py_True : Py_False,
            "coverage", ordinary ? state->coverage : 0,
            "loop_heat", ordinary ? state->loop_heat : 0,
            "cooldown", ordinary ? state->cooldown : 0,
            "unsupported_word", ordinary ? state->unsupported_word : -1,
            "unsupported_handler", ordinary ? state->unsupported_handler : -1,
            "compile_attempts", ordinary ? state->compile_attempts : 0,
            "compile_failures", ordinary ? state->compile_failures : 0,
            "compile_ns", (unsigned long long)(ordinary ? state->compile_ns : 0),
            "last_failure", ordinary ? state->last_failure : 0,
            "process_units", units, "process_bytes", charged, "bootstrap_remaining", bootstrap);
    }
    /* Boxing can invoke an allocator hook which invalidates this unit. The
     * snapshot owns only its scalar artifact, not code or its constants. */
    unit->refs++;
    uint32_t attempts = state->compile_attempts, failures = state->compile_failures;
    uint64_t elapsed = state->compile_ns;
    int last_failure = state->last_failure;
    size_t exits = 0;
    for (uint32_t i = 0; i < unit->nops; i++) {
        exits += unit->ops[i].kind == _PyJIT_LIR_EXIT;
    }
    PyObject *bytes = PyBytes_FromStringAndSize(unit->mapping, (Py_ssize_t)unit->code_size);
    PyObject *offsets = bytes ? PyTuple_New(unit->nwords) : NULL;
    if (offsets != NULL) {
        for (uint32_t i = 0; i < unit->nwords; i++) {
            PyObject *value = PyLong_FromLong(unit->offsets[i]);
            if (value == NULL) {
                Py_CLEAR(offsets);
                break;
            }
            PyTuple_SET_ITEM(offsets, i, value);
        }
    }
    PyObject *result = offsets ? Py_BuildValue("{s:O,s:O,s:O,s:O,s:n,s:n,s:n,s:K,s:n,s:O,s:O,s:I,s:I,s:K,s:i,s:n,s:n,s:i}",
        "active", Py_True, "entry_seen", unit->entry_seen ? Py_True : Py_False,
        "c_call_seen", unit->c_call_seen ? Py_True : Py_False,
        "return_resume_seen", unit->return_resume_seen ? Py_True : Py_False,
        "code_size", (Py_ssize_t)unit->code_size,
        "mapping_size", (Py_ssize_t)unit->mapping_size, "words", (Py_ssize_t)unit->nwords,
        "address", (unsigned long long)(uintptr_t)unit->mapping,
        "exits", (Py_ssize_t)exits, "code", bytes, "offsets", offsets,
        "compile_attempts", attempts,
        "compile_failures", failures,
        "compile_ns", (unsigned long long)elapsed,
        "last_failure", last_failure,
        "process_units", units, "process_bytes", charged, "bootstrap_remaining", bootstrap) : NULL;
    if (result != NULL) {
        _PyJitGraph *g = unit->graph;
        PyObject *trace = Py_BuildValue("{s:O,s:O,s:O,s:O,s:O,s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:I,s:I,s:O,s:I}",
            "optimized", g ? Py_True : Py_False,
            "entry_seen", unit->trace_seen ? Py_True : Py_False,
            "return_seen", unit->trace_return_seen ? Py_True : Py_False,
            "scalar_seen", unit->trace_scalar_seen ? Py_True : Py_False,
            "backedge_seen", unit->trace_backedge_seen ? Py_True : Py_False,
            "events", g ? g->event_count : 0, "nodes", g ? g->nnodes : 0,
            "values", g ? g->nvalues : 0, "spills", g ? g->nspills : 0,
            "scratch", g ? g->nscratch : 0, "boxed_imports", g ? g->boxed_imports : 0,
            "python_calls", g ? g->python_calls : 0,
            "header", g ? g->header : unit->record_header,
            "backedge", g ? g->backedge : unit->record_backedge,
            "recording", unit->recording ? Py_True : Py_False,
            "record_steps", unit->record_steps);
        PyObject *words = trace ? PyTuple_New(g ? g->event_count : 0) : NULL;
        if (words != NULL && g != NULL) for (int i = 0; i < g->event_count; i++) {
            PyObject *word = Py_BuildValue("(II)", g->record_words[i], g->record_successors[i]);
            if (word == NULL) { Py_CLEAR(words); break; }
            PyTuple_SET_ITEM(words, i, word);
        }
        if (words == NULL || PyDict_SetItemString(trace, "path", words) < 0 ||
            PyDict_SetItemString(result, "trace", trace) < 0) Py_CLEAR(result);
        Py_XDECREF(words);
        Py_XDECREF(trace);
    }
    Py_XDECREF(offsets);
    Py_XDECREF(bytes);
    _PyJit_FreeUnit(unit);
    if (PyErr_Occurred()) {
        Py_XDECREF(result);
        return NULL;
    }
    return result;
}


int
_PyJit_EnsureFlow(_PyJitContext *ctx)
{
    if (ctx->flow_owner == NULL || *ctx->flow_owner != NULL) {
        return 0;
    }
    if (take_bootstrap_cooldown()) {
        return 0;
    }
    _PyJitFlow *flow = initial_calloc(_PyFrame_GetCode(ctx->frame), 1);
    if (PyErr_Occurred()) {
        _PyJit_FreeFlow(&flow);
        return -1;
    }
    if (flow == NULL) {
        _Py_atomic_store_int(&bootstrap_cooldown, 4096);
        return 0;
    }
    *ctx->flow_owner = flow;
    return 0;
}

void
_PyJit_PushPending(_PyJitContext *ctx)
{
    if (ctx->pending_caller == NULL || ctx->flow_owner == NULL) {
        return;
    }
    _PyJitFlow *flow = *ctx->flow_owner;
    if (flow != NULL && flow->depth < Py_ARRAY_LENGTH(flow->pending)) {
        flow->pending[flow->depth++] = (_PyJitPending){ctx->pending_caller,
            ctx->pending_call, ctx->pending_return, ++flow->generation};
        if (flow->record != NULL) _PyJit_RecordPending(flow, &flow->pending[flow->depth - 1]);
    }
    ctx->pending_caller = NULL;
}

int
_PyJit_PopPending(_PyJitFlow *flow, _PyInterpreterFrame *frame, _PyIRWord *next)
{
    if (flow == NULL || flow->depth == 0) {
        return 0;
    }
    _PyJitPending *record = &flow->pending[flow->depth - 1];
    if (record->caller != frame) {
        return 0;
    }
    flow->depth--;
    /* The linked caller owns code/words. The record owns no borrowed code. */
    int match = next == frame->ir_words + record->return_word &&
                frame->instr_ptr == frame->ir_words + record->call_word;
    if (match && flow->record != NULL) _PyJit_RecordReturn(flow, frame, next, record->token);
    return match;
}

void
_PyJit_RetirePending(_PyJitFlow *flow, _PyInterpreterFrame *frame)
{
    if (flow == NULL) {
        return;
    }
    if (flow->record != NULL) _PyJit_RecordRetire(flow, frame);
    /* A linked Python call unwinds in stack order. No return-chain scan. */
    if (flow->depth && flow->pending[flow->depth - 1].caller == frame) {
        flow->depth--;
    }
}

int
_PyJit_FreeFlow(_PyJitFlow **owner)
{
    _PyJitFlow *flow = *owner;
    *owner = NULL;
    int budgeted = flow != NULL && flow->budgeted;
    PyObject *saved = PyErr_GetRaisedException();
    if (flow != NULL) _PyJit_FreeRecord(flow->record);
    restore_jit_error(saved);
    saved = PyErr_GetRaisedException();
    PyMem_Free(flow);
    restore_jit_error(saved);
    if (budgeted) {
        _Py_atomic_add_ssize(&process_bytes, -(Py_ssize_t)sizeof(*flow));
    }
    return PyErr_Occurred() ? -1 : 0;
}

static int
complete_coverage(PyCodeObject *code, _PyJitState *state)
{
    int entry = code->_co_firsttraceable;
    state->unsupported_word = state->unsupported_handler = -1;
    if (Py_SIZE(code) < 1 || Py_SIZE(code) > 4096 || entry < 0 || entry >= Py_SIZE(code) ||
        _PyCode_InstrHandler(code, entry) != RESUME ||
        (_PyCode_InstrOparg(code, entry) & RESUME_OPARG_LOCATION_MASK) != RESUME_AT_FUNC_START ||
        code->co_flags & (CO_GENERATOR | CO_COROUTINE | CO_ASYNC_GENERATOR)) return 0;
    for (int i = 0; i < Py_SIZE(code); i++) {
        const _PyJitHelperInfo *info = _PyJit_GetHelper(_PyCode_GenericHandler(code, i));
        if (info == NULL || info->helper == NULL) {
            state->unsupported_word = i;
            state->unsupported_handler = _PyCode_GenericHandler(code, i);
            return 0;
        }
    }
    return 1;
}

static _PyJitState *
natural_state(PyCodeObject *code)
{
    _PyCodeExec *exec = code->_co_exec;
    if (exec == NULL || exec->jit == &initial_building) {
        return NULL;
    }
    if (exec->jit != NULL) {
        return exec->jit;
    }
    if (take_bootstrap_cooldown()) {
        return NULL;
    }
    exec->jit = &initial_building;
    _PyJitState *fresh = initial_calloc(code, 0);
    if ((fresh != NULL && !fresh->budgeted) || PyErr_Occurred() || code->_co_exec != exec || exec->jit != &initial_building) {
        if (fresh != NULL && !fresh->budgeted && !PyErr_Occurred())
            _Py_atomic_store_int(&bootstrap_cooldown, 4096);
        if (code->_co_exec == exec && exec->jit == &initial_building) {
            exec->jit = NULL;
        }
        PyObject *saved = PyErr_GetRaisedException();
        free_state_storage(fresh); restore_jit_error(saved); return NULL;
    }
    exec->jit = fresh;
    if (fresh != NULL) {
        fresh->refs = 1;
    }
    else _Py_atomic_store_int(&bootstrap_cooldown, 4096);
    return fresh;
}

static int
automatic_compile(PyThreadState *tstate, _PyInterpreterFrame *frame, PyCodeObject *code)
{
#if !_PyJIT_BASELINE_SUPPORTED
    return 0;
#else
    Py_INCREF(code);  /* cold transaction only, never across a Python callee */
    _PyJitState *state = natural_state(code);
    int result = 0;
    if (state == NULL || PyErr_Occurred()) {
        goto done;
    }
    if (state->coverage == 0) {
        state->coverage = complete_coverage(code, state) ? 1 : -1;
    }
    if (state->coverage < 0 || state->disabled) { state->last_failure = _PyJIT_FAILURE_COVERAGE; result = 2; goto done; }
    if (state->building) {
        goto done;
    }
    if (state->unit != NULL) {
        _PyCodeExec *exec = code->_co_exec;
        if (state->unit->epoch == jit_epoch(tstate) &&
            state->unit->generation == state->generation &&
            state->unit->interpreter_id == PyInterpreterState_GetID(tstate->interp)) {
            result = 1; goto done;
        }
        _PyJit_DetachExec(exec);
        if (PyErr_Occurred() || code->_co_exec != exec || exec->jit != state) {
            goto done;
        }
    }
    if (state->cooldown) { state->cooldown--; goto done; }
    if (!_PyJit_ObserversAllow(tstate, frame)) {
        goto done;
    }
    int expected = 0;
    if (!_Py_atomic_compare_exchange_int(&automatic_building, &expected, 1)) {
        goto done;
    }
    result = _PyJit_CompileCode(code);
    _Py_atomic_store_int(&automatic_building, 0);
    if (result == 0 && code->_co_exec != NULL && code->_co_exec->jit != NULL &&
        code->_co_exec->jit != &initial_building) code->_co_exec->jit->cooldown = 4096;
    done:
    { PyObject *saved = PyErr_GetRaisedException(); Py_DECREF(code); restore_jit_error(saved); }
    return PyErr_Occurred() ? -1 : result;
#endif
}

int
_PyJit_PrepareCall(_PyJitContext *ctx, _PyIRWord *instr, PyValue callable)
{
    PyCodeObject *caller_code = _PyFrame_GetCode(ctx->frame);
    _PyCodeExec *caller_exec = caller_code->_co_exec;
    Py_ssize_t word = instr - ctx->frame->ir_words;
    if (ctx->unit != NULL) {
        if (_PyJit_EnsureFlow(ctx) < 0) {
            return -1;
        }
        if (ctx->tstate->current_frame != ctx->frame || caller_code->_co_exec != caller_exec) {
            return 1;
        }
        ctx->pending_caller = ctx->frame;
        ctx->pending_call = (uint32_t)word;
        ctx->pending_return = (uint32_t)word + 1;
    }
    if (ctx->tstate->current_frame != ctx->frame) {
        return 1;
    }
    if (!_PyJit_ObserversAllow(ctx->tstate, ctx->frame)) {
        if (caller_code->_co_exec == caller_exec && caller_exec != NULL &&
            caller_exec->words == ctx->frame->ir_words && IR_HANDLER(*instr) == CALL_JIT_WARM)
            _PyIR_SetHandler(instr, _PyCode_GenericHandler(caller_code, (int)word));
        if (caller_code->_co_exec != NULL) {
            _PyJit_DetachExec(caller_code->_co_exec);
        }
        if (PyErr_Occurred() || _Py_Instrument(caller_code, ctx->tstate->interp) < 0) {
            return -1;
        }
        return 1;
    }
    if (caller_code->_co_exec != caller_exec || caller_exec == NULL ||
        caller_exec->words != ctx->frame->ir_words || IR_HANDLER(*instr) != CALL_JIT_WARM ||
        PyValue_IsInline(callable)) return 0;
    if (!natural_admission) {
        _PyIR_SetHandler(instr, _PyCode_GenericHandler(caller_code, (int)word));
        return 0;
    }
    int index = _PyCode_InstrFbIndex(caller_code, (int)word);
    if (index <= 0 || index >= caller_code->co_nfeedback) {
        return 0;
    }
    _PyFbCall *fb = (_PyFbCall *)_PyIR_FbEntry(caller_exec->entries, index);
    if (fb->h.kind != FB_CALL) {
        return 0;
    }
    PyObject *function = PyValue_AsKnownObjectBorrow(callable);
    if (PyMethod_Check(function)) {
        function = ((PyMethodObject *)function)->im_func;
    }
    if (!PyFunction_Check(function)) {
        return 0;
    }
    uint32_t version = ((PyFunctionObject *)function)->func_version;
    if (!_PyFunction_IsVersionValid(version)) {
        fb->h.hits = 0;
        _PyIR_SetHandler(instr, _PyCode_GenericHandler(caller_code, (int)word));
        return 0;
    }
    if (version != fb->func_version) { fb->func_version = version; fb->h.hits = 0; }
    if (fb->h.hits < 64) {
        fb->h.hits++;
    }
    if (fb->h.hits < 64) {
        return 0;
    }
    PyCodeObject *target_code = (PyCodeObject *)PyFunction_GET_CODE(function);
    int result = automatic_compile(ctx->tstate, ctx->frame, target_code);
    if (PyErr_Occurred()) {
        return -1;
    }
    if (ctx->tstate->current_frame != ctx->frame || caller_code->_co_exec != caller_exec) {
        return 1;
    }
    if (!_PyJit_ObserversAllow(ctx->tstate, ctx->frame)) {
        if (IR_HANDLER(*instr) == CALL_JIT_WARM)
            _PyIR_SetHandler(instr, _PyCode_GenericHandler(caller_code, (int)word));
        _PyJit_DetachExec(caller_exec);
        if (PyErr_Occurred() || _Py_Instrument(caller_code, ctx->tstate->interp) < 0) {
            return -1;
        }
        return 1;
    }
    if (version != ((PyFunctionObject *)function)->func_version ||
        target_code != (PyCodeObject *)PyFunction_GET_CODE(function)) {
        _PyFbCall *current = (_PyFbCall *)_PyIR_FbEntry(caller_exec->entries, index);
        current->h.hits = 0;
        return 0;
    }
    if (caller_code->_co_exec == caller_exec && ctx->frame->ir_words == caller_exec->words &&
        IR_HANDLER(caller_exec->words[word]) == CALL_JIT_WARM && result != 0) {
        _PyIR_SetHandler(caller_exec->words + word, _PyCode_GenericHandler(caller_code, (int)word));
    }
    return 0;
}

int
_PyJit_WarmBackedge(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
#if !_PyJIT_BASELINE_SUPPORTED
    return 0;
#else
    PyCodeObject *code = _PyFrame_GetCode(frame);
    _PyJitState *state = NULL;
    if (natural_admission) {
        state = natural_state(code);
        if (state == NULL) {
            return PyErr_Occurred() ? -1 : 0;
        }
    }
    if (state == NULL || state->coverage < 0 || state->disabled) {
        _PyCodeExec *exec = code->_co_exec;
        for (uint32_t i = 0; i < exec->nwords; i++) {
            if (IR_HANDLER(exec->words[i]) == JUMP_BACKWARD_JIT_WARM)
                _PyIR_SetHandler(exec->words + i, JUMP_BACKWARD);
        }
        return 0;
    }
    if (state->loop_heat < 256) {
        state->loop_heat++;
    }
    if (state->loop_heat < 256) {
        return 0;
    }
    /* A long-running frame may not execute RESUME again after an observer is
     * removed. Sync the header before testing retained active monitor bits. */
    if (code->_co_instrumentation_version != jit_epoch(tstate)) {
        if (_Py_Instrument(code, tstate->interp) < 0 || PyErr_Occurred()) {
            return -1;
        }
    }
    int result = automatic_compile(tstate, frame, code);
    return result == 2 ? 0 : result;
#endif
}

int
_PyJit_AutoEligible(PyObject *function)
{
#if !_PyJIT_BASELINE_SUPPORTED
    return 0;
#else
    if (!natural_admission) {
        return 0;
    }
    if (PyMethod_Check(function)) {
        function = ((PyMethodObject *)function)->im_func;
    }
    if (!PyFunction_Check(function)) {
        return 0;
    }
    PyCodeObject *code = (PyCodeObject *)PyFunction_GET_CODE(function);
    if (code->co_flags & (CO_GENERATOR | CO_COROUTINE | CO_ASYNC_GENERATOR)) {
        return 0;
    }
    _PyCodeExec *exec = code->_co_exec;
    if (exec == NULL || exec->jit == NULL) {
        return 1;
    }
    _PyJitState *state = exec->jit;
    return state != &initial_building && state->coverage >= 0 && !state->disabled && state->unit == NULL;
#endif
}


void
_PyJit_SeedBackedges(PyCodeObject *code, _PyCodeExec *exec)
{
#if _PyJIT_BASELINE_SUPPORTED
    if (!natural_admission || exec == NULL ||
        code->co_flags & (CO_GENERATOR | CO_COROUTINE | CO_ASYNC_GENERATOR)) {
        return;
    }
    if (exec->jit != NULL && exec->jit != &initial_building &&
        (exec->jit->coverage < 0 || exec->jit->disabled)) return;
    /* This cold pass changes only uninstrumented, real periodic backedges.
     * It also re-arms them after ordinary monitoring removal. */
    for (uint32_t i = 0; i < exec->nwords; i++) {
        if (IR_HANDLER(exec->words[i]) == JUMP_BACKWARD &&
            _PyCode_InstrHandler(code, (int)i) == JUMP_BACKWARD &&
            _PyCode_InstrTarget(code, (int)i) >= 0 && _PyCode_InstrTarget(code, (int)i) < (int)i)
            _PyIR_SetHandler(exec->words + i, JUMP_BACKWARD_JIT_WARM);
    }
#endif
}

/* Returns the previous setting. Enabling affects execution copies created
 * afterwards; disabling makes already warmed sites fall back on next use. */
int
_PyJit_SetNaturalAdmission(int enabled)
{
    int previous = natural_admission;
    natural_admission = enabled != 0;
    return previous;
}


static int
take_bootstrap_cooldown(void)
{
    int remaining = _Py_atomic_load_int_relaxed(&bootstrap_cooldown);
    while (remaining > 0) {
        if (_Py_atomic_compare_exchange_int(&bootstrap_cooldown, &remaining, remaining - 1)) {
            if (cold_request != NULL && cold_request->used &&
                cold_request->tstate == _PyThreadState_GET())
                cold_request->blocked++;
            return 1;
        }
    }
    return 0;
}

static void *
initial_calloc_impl(PyCodeObject *code, int site)
{
    JitColdRequest *request = cold_request;
    if (request == NULL || request->used) {
        return PyMem_Calloc(1, site == 0 ? sizeof(_PyJitState) : sizeof(_PyJitFlow));
    }
    PyThreadState *tstate = _PyThreadState_GET();
    _PyInterpreterFrame *frame = tstate->current_frame;
    /* State injection is specifically the evaluated CALL boundary, not an
     * earlier loop-state allocation in the same requested driver. */
    int at_call = site == 1 || (frame != NULL && frame->owner != FRAME_OWNED_BY_INTERPRETER &&
        _PyIR_BaseOpcode(IR_HANDLER(*frame->instr_ptr)) == CALL);
    if (request != NULL && request->claimed && !request->used && request->site == site &&
        request->tstate == tstate && request->frame == frame && at_call &&
        request->code == _PyFrame_GetCode(frame)) {
        request->used = 1;
        if (request->callback != Py_None) {
            PyObject *result = PyObject_CallNoArgs(request->callback);
            if (result == NULL) {
                return NULL;
            }
            Py_DECREF(result);
            if (PyErr_Occurred()) {
                return NULL;
            }
        }
        if (request->fail) {
            return NULL;
        }
    }
    (void)code;
    return PyMem_Calloc(1, site == 0 ? sizeof(_PyJitState) : sizeof(_PyJitFlow));
}

static void *
initial_calloc(PyCodeObject *code, int site)
{
    size_t bytes = site == 0 ? sizeof(_PyJitState) : sizeof(_PyJitFlow);
    if (!reserve_bytes(bytes)) return NULL;
    void *result = initial_calloc_impl(code, site);
    if (result == NULL) _PyJit_ReleaseBytes(bytes);
    else if (site == 0) ((_PyJitState *)result)->budgeted = 1;
    else ((_PyJitFlow *)result)->budgeted = 1;
    return result;
}

PyObject *
_PyJit_TestColdCall(PyObject *function, PyObject *args, PyObject *callback, int site, int fail)
{
    if (!PyFunction_Check(function) || !PyTuple_Check(args) ||
        (callback != Py_None && !PyCallable_Check(callback)) || site < 0 || site > 1) {
        PyErr_SetString(PyExc_ValueError, "expected function, tuple, callback, State/Flow site");
        return NULL;
    }
    JitColdRequest request = {.previous=cold_request, .tstate=_PyThreadState_GET(),
        .code=(PyCodeObject *)PyFunction_GET_CODE(function), .callback=callback, .site=site, .fail=fail};
    cold_request = &request;
    PyObject *result = PyObject_Call(function, args, NULL);
    /* Restore before boxing or releasing anything that could reenter. The
     * ordinary call's function/args remain owned by the extension's arguments. */
    cold_request = request.previous;
    request.frame = NULL;
    if (result == NULL) {
        return NULL;
    }
    return Py_BuildValue("NiiI", result, request.used,
        _Py_atomic_load_int_relaxed(&bootstrap_cooldown), request.blocked);
}


void
_PyJit_RetireCold(void *token, _PyInterpreterFrame *frame)
{
    JitColdRequest *request = token;
    if (request->frame == frame) {
        request->frame = NULL;
    }
}
