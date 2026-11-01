#include "Python.h"
#include "pycore_jit.h"
#include "pycore_pystate.h"

/* Stack-owned and thread-local. An already claimed request cannot be inherited
 * by recursive evaluator entries or a callback, even for the same code. */
static _Py_thread_local _PyJitTestRequest *test_request;

_PyJitTestRequest *
_PyJit_TestClaim(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
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
#if _PyJIT_BASELINE_SUPPORTED
#  include <sys/mman.h>
#endif

static _PyJitState initial_building = {.building = 1};
static void release_jit_state(_PyJitState *);
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
    PyMem_Free(unit);
    restore_jit_error(saved);
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
        size += sizeof(*unit) + unit->nops * (sizeof(*unit->ops) + sizeof(*unit->offsets));
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
release_jit_state(_PyJitState *state)
{
    assert(state->refs > 0);
    if (--state->refs == 0) {
        PyObject *saved = PyErr_GetRaisedException();
        PyMem_Free(state);
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
        _PyJitState *fresh = PyMem_Calloc(1, sizeof(*fresh));
        if (PyErr_Occurred() || code->_co_exec != exec ||
            exec->jit != &initial_building || epoch != jit_epoch(tstate) ||
            source != jit_source_stamp(code)) {
            if (code->_co_exec == exec && exec->jit == &initial_building) {
                exec->jit = NULL;
            }
            PyObject *saved = PyErr_GetRaisedException();
            PyMem_Free(fresh);
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
    uint64_t generation = state->generation;
    unit = _PyJit_BuildBaseline(code, exec, generation, epoch, source);
    int valid = !PyErr_Occurred() && code->_co_exec == exec && exec->jit == state &&
                state->generation == generation && !state->disabled && epoch == jit_epoch(tstate) &&
                source == jit_source_stamp(code);
    if (code->_co_exec == exec && exec->jit == state) {
        state->building = 0;
        if (valid && unit != NULL) {
            unit->interpreter_id = PyInterpreterState_GetID(tstate->interp);
            state->unit = unit;
            unit = NULL;
            installed = 1;
        }
    }
    done:
    {
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
    if (unit->detached || word < 0 || word >= unit->nops ||
        unit->interpreter_id != PyInterpreterState_GetID(tstate->interp) ||
        !_PyJit_ObserversAllow(tstate, frame) ||
        unit->epoch != jit_epoch(tstate) || unit->generation != exec->jit->generation) {
        return 0;
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
    if (unit == NULL) {
        return Py_BuildValue("{s:O,s:O}", "active", Py_False,
            "disabled", state && state != &initial_building && state->disabled ? Py_True : Py_False);
    }
    /* Boxing can invoke an allocator hook which invalidates this unit. The
     * snapshot owns only its scalar artifact, not code or its constants. */
    unit->refs++;
    size_t exits = 0;
    for (uint32_t i = 0; i < unit->nops; i++) {
        exits += unit->ops[i].kind == _PyJIT_LIR_EXIT;
    }
    PyObject *bytes = PyBytes_FromStringAndSize(unit->mapping, (Py_ssize_t)unit->code_size);
    PyObject *offsets = bytes ? PyTuple_New(unit->nops) : NULL;
    if (offsets != NULL) {
        for (uint32_t i = 0; i < unit->nops; i++) {
            PyObject *value = PyLong_FromLong(unit->offsets[i]);
            if (value == NULL) {
                Py_CLEAR(offsets);
                break;
            }
            PyTuple_SET_ITEM(offsets, i, value);
        }
    }
    PyObject *result = offsets ? Py_BuildValue("{s:O,s:O,s:O,s:O,s:n,s:n,s:n,s:K,s:n,s:O,s:O}",
        "active", Py_True, "entry_seen", unit->entry_seen ? Py_True : Py_False,
        "c_call_seen", unit->c_call_seen ? Py_True : Py_False,
        "return_resume_seen", unit->return_resume_seen ? Py_True : Py_False,
        "code_size", (Py_ssize_t)unit->code_size,
        "mapping_size", (Py_ssize_t)unit->mapping_size, "words", (Py_ssize_t)unit->nops,
        "address", (unsigned long long)(uintptr_t)unit->mapping,
        "exits", (Py_ssize_t)exits, "code", bytes, "offsets", offsets) : NULL;
    Py_XDECREF(offsets);
    Py_XDECREF(bytes);
    _PyJit_FreeUnit(unit);
    if (PyErr_Occurred()) {
        Py_XDECREF(result);
        return NULL;
    }
    return result;
}
