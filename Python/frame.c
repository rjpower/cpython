#define _PY_INTERPRETER

#include "Python.h"
#include "pycore_ceval.h"         // _PyEval_InvalidateSourceErrors()
#include "pycore_frame.h"         // _PyFrame_New_NoTrack()
#include "pycore_interpframe.h"   // _PyFrame_GetCode()
#include "pycore_genobject.h"     // _PyGen_GetGeneratorFromFrame()
#include "pycore_pyerrors.h"      // _PyErr_ClearExcState()
#include "pycore_pystate.h"       // HEAD_LOCK(), thread enumeration
#include "pycore_transition_roots.h" // _PyTransitionRoots
#include "pycore_value.h"      // _Py_VISIT_VALUE()

/* This is an actual dynamic readability provider, not a static plan proof.
 * The caller owns the allocation and supplies its live ownership extent. */
void
_PyFrame_GetPresenceHomeView(_PyInterpreterFrame *frame, int remaining,
                            _PyIRHomeView *view)
{
    int nl = _PyFrame_GetCode(frame)->co_nlocalsplus;
    assert(remaining >= 0 && remaining <= _PyFrame_NumSlots(frame));
    *view = (_PyIRHomeView){
        .nbindings = (uint16_t)nl,
        .scratch_limit = (uint16_t)Py_MAX(remaining - nl, 0),
        .kind = _PyIR_HOME_VIEW_PRESENCE,
        .first_bit = ((uintptr_t)frame->base >> 3) & 63,
        .presence = frame->presence_bits,
    };
}

static int
find_error_home_view(PyThreadState *owner, _PyInterpreterFrame *frame,
                     _PyIRHomeView *view, _PyIRHomeAuthority *authority)
{
    int matches = 0;
    for (_PyTransitionRoots *roots =
             ((_PyThreadStateImpl *)owner)->transition_roots;
         roots != NULL; roots = roots->previous)
    {
        if (roots->kind != _Py_TRANSITION_KIND_ERROR || roots->frame != frame) {
            continue;
        }
        assert(roots->header_mask == 0 && !roots->result_present);
        if (roots->homes == NULL ||
            roots->home_authority != _PyIR_HOME_MUST_READABLE)
        {
            return -1;
        }
        *view = *roots->homes;
        view->scratch_limit = (uint16_t)Py_MIN(view->scratch_limit,
            Py_MAX(roots->remaining - view->nbindings, 0));
        *authority = roots->home_authority;
        matches++;
    }
    return matches;
}

_PyIRHomeAuthority
_PyFrame_GetErrorHomeView(PyThreadState *owner_hint, _PyInterpreterFrame *frame,
                         _PyIRHomeView *view)
{
    if (frame->phase_state != _PyIR_PHASE_ERROR_SCOPE) {
        return _PyIR_HOME_NONE;
    }
    PyThreadState *current = _PyThreadState_GET();
    if (owner_hint != NULL && owner_hint->interp != current->interp) {
        return _PyIR_HOME_NONE;
    }
    _PyIRHomeAuthority authority = _PyIR_HOME_NONE;
    int matches = 0;
    if (owner_hint == NULL || owner_hint == current) {
        matches = find_error_home_view(current, frame, view, &authority);
        if (matches != 0 || owner_hint == current) {
            return matches == 1 ? authority : _PyIR_HOME_NONE;
        }
    }
#ifdef Py_GIL_DISABLED
    /* A suspended-frame read claim does not protect a foreign C stack. */
    if (!current->interp->stoptheworld.world_stopped) {
        return _PyIR_HOME_NONE;
    }
#endif
    _PyThreadStateImpl *impl = (_PyThreadStateImpl *)current;
    int lock_head = impl->head_locked_frame_walk == 0;
    if (lock_head) {
        HEAD_LOCK(&_PyRuntime);
    }
    if (owner_hint != NULL) {
        matches = find_error_home_view(owner_hint, frame, view, &authority);
    }
    else {
        _Py_FOR_EACH_TSTATE_UNLOCKED(current->interp, owner) {
            if (owner != current) {
                int found = find_error_home_view(owner, frame, view, &authority);
                if (found < 0) {
                    matches = -1;
                    break;
                }
                matches += found;
            }
        }
    }
    if (lock_head) {
        HEAD_UNLOCK(&_PyRuntime);
    }
    return matches == 1 ? authority : _PyIR_HOME_NONE;
}

_PyIRHomeAuthority
_PyFrame_GetHomeView(_PyInterpreterFrame *frame, _PyIRHomeView *view)
{
    if (frame->phase_state == _PyIR_PHASE_ERROR_SCOPE) {
        return _PyFrame_GetErrorHomeView(NULL, frame, view);
    }
    if (frame->phase_state & _PyIR_PHASE_CAPTURED) {
        if (frame->phase_state != _PyIR_PHASE_CAPTURED &&
            frame->phase_state != (_PyIR_PHASE_CAPTURED | _PyIR_PHASE_YIELD_DELEGATED))
        {
            return _PyIR_HOME_NONE;
        }
        PyFrameObject *f;
        if (frame->owner == FRAME_OWNED_BY_FRAME_OBJECT) {
            f = (PyFrameObject *)((char *)frame -
                offsetof(PyFrameObject, _f_frame_data));
        }
        else if (frame->owner == FRAME_OWNED_BY_GENERATOR) {
            f = frame->frame_obj;
        }
        else {
            return _PyIR_HOME_NONE;
        }
        if (f == NULL || f->f_frame != frame ||
            f->f_capture_authority != _PyIR_HOME_MUST_READABLE)
        {
            return _PyIR_HOME_NONE;
        }
        *view = f->f_capture;
        return (_PyIRHomeAuthority)f->f_capture_authority;
    }
    if (frame->phase_state == _PyIR_PHASE_CREATED) {
        if (frame->owner != FRAME_OWNED_BY_GENERATOR ||
            FT_ATOMIC_LOAD_INT8_RELAXED(
                _PyGen_GetGeneratorFromFrame(frame)->gi_frame_state) != FRAME_CREATED ||
            frame->f_executable == NULL)
        {
            return _PyIR_HOME_NONE;
        }
        PyCodeObject *code = frame->f_executable;
        _PyCodeExec *exec = FT_ATOMIC_LOAD_PTR_ACQUIRE(code->_co_exec);
        if (exec == NULL || frame->ir_words != exec->words ||
            exec->semantic_plan == NULL ||
            !(exec->semantic_plan->flags & _PyIR_PLAN_ALL_CREATED_EMPTY))
        {
            return _PyIR_HOME_NONE;
        }
        *view = (_PyIRHomeView){.nbindings = (uint16_t)code->co_nlocalsplus};
        return _PyIR_HOME_MUST_READABLE;
    }
    if (frame->owner == FRAME_OWNED_BY_INTERPRETER) {
        return _PyIR_HOME_NONE;
    }
    int remaining = _PyFrame_NumSlots(frame);
    if (frame->owner == FRAME_OWNED_BY_THREAD) {
        /* Presence alone cannot distinguish a waiting caller from the child
         * that now owns its shared suffix. Foreign generic queries need an
         * explicit protected extent; they cannot guess full-frame ownership. */
        _PyInterpreterFrame *child = NULL;
        _PyInterpreterFrame *active = _PyThreadState_GET()->current_frame;
        while (active != NULL && active != frame) {
            child = active;
            active = active->previous;
        }
        if (active == NULL) {
            return _PyIR_HOME_NONE;
        }
        if (child != NULL && child->owner == FRAME_OWNED_BY_THREAD) {
            uintptr_t base = (uintptr_t)frame->base;
            uintptr_t cut = (uintptr_t)child->base;
            uintptr_t end = base + (size_t)remaining * sizeof(PyValue);
            if (base < cut && cut < end) {
                remaining = (int)((cut - base) / sizeof(PyValue));
                assert(remaining >= _PyFrame_GetCode(frame)->co_nlocalsplus);
            }
        }
    }
    _PyFrame_GetPresenceHomeView(frame, remaining, view);
    return _PyIR_HOME_MUST_READABLE;
}

static Py_NO_INLINE void
copy_created_generator_control(_PyInterpreterFrame *src, _PyInterpreterFrame *dest)
{
    _PyIRHomeView view;
    /* This reached marker still owns the complete current source extent. */
    assert(_PyThreadState_GET()->current_frame == src);
    _PyFrame_GetPresenceHomeView(src, _PyFrame_NumSlots(src), &view);
    src->instr_ptr++;
    _PyFrame_Copy(src, dest, &view, _PyIR_HOME_MUST_READABLE);
}

/* Only the actual RETURN_GENERATOR marker calls this entry. The whole-image
 * flag covers every such site, including public/synthetic image sites. */
Py_NO_INLINE void
_PyFrame_CopyCreatedGenerator(_PyInterpreterFrame *src, _PyInterpreterFrame *dest)
{
    PyCodeObject *code = _PyFrame_GetCode(src);
    _PyCodeExec *exec = _PyFrame_Exec(src);
    if (exec->semantic_plan == NULL ||
        !(exec->semantic_plan->flags & _PyIR_PLAN_ALL_CREATED_EMPTY))
    {
        copy_created_generator_control(src, dest);
        return;
    }
    int nl = code->co_nlocalsplus;
    int nslots = _PyFrame_NumSlotsForCodeObject(code);
    src->instr_ptr++;
    _PyFrame_CopyHeader(src, dest);
    for (int i = 0; i < nl; i++) {
        _PyFrame_MoveHome(src, src->base + i, dest, dest->base + i);
    }
    /* The current NULL control still requires initialized destination gaps. */
    for (int i = nl; i < nslots; i++) {
        dest->base[i] = PyValue_NULL;
    }
    dest->phase_state = _PyIR_PHASE_CREATED;
}

void
_PyFrame_CompleteRedirect(_PyInterpreterFrame *frame)
{
    assert(frame->pending_redirect);
    assert(frame->return_slot == IR_NO_DST);
    assert(frame->phase_state == 0);
    /* The child has retired its overlap. Publish the new state before
     * closing remaining scratch owners, whose finalizers can inspect it. */
    frame->pending_redirect = 0;
    frame->instr_ptr = frame->return_pc;
    _PyEval_InvalidateSourceErrors(_PyThreadState_GET(), frame);
    _PyFrame_ClearTemporaries(frame, _PyFrame_GetCode(frame)->co_nlocalsplus);
}


int
_PyFrame_Traverse(_PyInterpreterFrame *frame, visitproc visit, void *arg)
{
    Py_VISIT(frame->frame_obj);
    Py_VISIT(frame->f_locals);
    Py_VISIT(frame->f_funcobj);
    Py_VISIT(frame->f_executable);
    return _PyGC_VisitFrameStack(frame, visit, arg);
}

PyFrameObject *
_PyFrame_MakeAndSetFrameObject(_PyInterpreterFrame *frame)
{
    PyObject *exc = PyErr_GetRaisedException();
    if (_PyFrame_MaterializeKwargs(frame) < 0) {
        Py_XDECREF(exc);
        return NULL;
    }

    PyFrameObject *f = _PyFrame_New_NoTrack(_PyFrame_GetCode(frame));
    if (f == NULL) {
        Py_XDECREF(exc);
        return NULL;
    }
    PyErr_SetRaisedException(exc);

    // GH-97002: There was a time when a frame object could be created when we
    // are allocating the new frame object f above, so frame->frame_obj would
    // be assigned already. That path does not exist anymore. We won't call any
    // Python code in this function and garbage collection will not run.
    // Notice that _PyFrame_New_NoTrack() can potentially raise a MemoryError,
    // but it won't allocate a traceback until the frame unwinds, so we are safe
    // here.
    assert(frame->owner != FRAME_OWNED_BY_FRAME_OBJECT);
    f->f_frame = frame;
#ifdef Py_GIL_DISABLED
    PyFrameObject *expected = NULL;
    if (!_Py_atomic_compare_exchange_ptr(&frame->frame_obj, &expected, f)) {
        Py_DECREF(f);
        return expected;
    }
#else
    assert(frame->frame_obj == NULL);
    frame->frame_obj = f;
#endif
    return f;
}

int
_PyFrame_MaterializeKwargs(_PyInterpreterFrame *frame)
{
    uint16_t slot = frame->pending_kwargs;
    if (slot == IR_NO_DST) {
        return 0;
    }
    assert(slot < _PyFrame_GetCode(frame)->co_nlocalsplus);
    assert(PyValue_IsNull(frame->base[slot]));
    /* As for frame-object allocation above, GC is scheduled, not run here.
     * Install the owner before a subsequent poll can invoke an observer. */
    PyObject *dict = PyDict_New();
    if (dict == NULL) {
        return -1;
    }
    _PyFrame_WriteHome(frame, frame->base + slot, PyValue_FromObjectSteal(dict));
    frame->pending_kwargs = IR_NO_DST;
    return 0;
}

static void
take_ownership(PyFrameObject *f, _PyInterpreterFrame *frame,
               _PyTransitionRoots *roots)
{
    Py_BEGIN_CRITICAL_SECTION(f);
    assert(frame->owner < FRAME_OWNED_BY_INTERPRETER);
    assert(frame->owner != FRAME_OWNED_BY_FRAME_OBJECT);
    _PyInterpreterFrame *new_frame = (_PyInterpreterFrame *)f->_f_frame_data;
    _PyIRHomeView view;
    _PyIRHomeAuthority authority = (_PyIRHomeAuthority)roots->home_authority;
    if (authority == _PyIR_HOME_NONE) {
        assert(roots->remaining == 0 && roots->homes == NULL);
        assert(_PyFrame_GetCode(frame)->co_nlocalsplus == 0);
        _PyFrame_GetPresenceHomeView(frame, 0, &view);
        authority = _PyIR_HOME_MUST_READABLE;
    }
    else if (authority == _PyIR_HOME_FRAME_PRESENCE) {
        assert(roots->homes == NULL);
        _PyFrame_GetPresenceHomeView(frame, roots->remaining, &view);
        authority = _PyIR_HOME_MUST_READABLE;
    }
    else {
        assert(authority == _PyIR_HOME_MUST_READABLE);
        assert(roots->homes != NULL);
        view = *roots->homes;
        view.scratch_limit = (uint16_t)Py_MIN(view.scratch_limit,
            Py_MAX(roots->remaining - view.nbindings, 0));
    }
    _PyFrame_Copy(frame, new_frame, &view, authority);
    // _PyFrame_Copy moves the reference to the executable, but the caller
    // still clears frame->f_executable, so give the source frame its own.
    frame->f_executable = (PyCodeObject *)Py_NewRef(new_frame->f_executable);
    roots->remaining = 0;
    roots->home_authority = _PyIR_HOME_NONE;
    roots->homes = NULL;
    roots->header_mask = _Py_TRANSITION_CODE |
        (roots->header_mask & _Py_TRANSITION_EXCEPTION);
    f->f_frame = new_frame;
    new_frame->owner = FRAME_OWNED_BY_FRAME_OBJECT;
    if (view.kind == _PyIR_HOME_VIEW_PRESENCE) {
        view.presence = new_frame->presence_bits;
        view.first_bit = ((uintptr_t)new_frame->base >> 3) & 63;
    }
    f->f_capture = view;
    f->f_capture_authority = (uint8_t)authority;
    assert(new_frame->phase_state == 0 ||
           new_frame->phase_state == _PyIR_PHASE_YIELD_DELEGATED);
    new_frame->phase_state |= _PyIR_PHASE_CAPTURED;
    if (_PyFrame_IsIncomplete(new_frame)) {
        new_frame->phase_state = _PyIR_PHASE_CAPTURED;
        // This may be a newly-created generator or coroutine frame. Since it's
        // dead anyways, just pretend that the first RESUME ran:
        new_frame->instr_ptr = new_frame->ir_words
            + _PyFrame_GetCode(new_frame)->_co_firsttraceable + 1;
    }
    assert(!_PyFrame_IsIncomplete(new_frame));
    assert(f->f_back == NULL);
    _PyInterpreterFrame *prev = _PyFrame_GetFirstComplete(frame->previous);
    if (prev) {
        assert(prev->owner < FRAME_OWNED_BY_INTERPRETER);
        PyObject *exc = PyErr_GetRaisedException();
        /* Link PyFrameObjects.f_back and remove link through _PyInterpreterFrame.previous */
        PyFrameObject *back = _PyFrame_GetFrameObject(prev);
        if (back == NULL) {
            /* Memory error here. */
            assert(PyErr_ExceptionMatches(PyExc_MemoryError));
            /* Nothing we can do about it */
            PyErr_Clear();
        }
        else {
            f->f_back = (PyFrameObject *)Py_NewRef(back);
        }
        PyErr_SetRaisedException(exc);
    }
    if (!_PyObject_GC_IS_TRACKED((PyObject *)f)) {
        _PyObject_GC_TRACK((PyObject *)f);
    }
    Py_END_CRITICAL_SECTION();
}

#ifdef Py_DEBUG
// An index is valid only on the evaluation stack, never in a local.
static bool
is_well_formed(PyValue ref, bool in_stack)
{
    if (!PyValue_IsTaggedIndex(ref)) {
        _PyValue_RecheckHome(ref);
    }
    if (PyValue_IsInlineInt(ref)) {
        return true;  // Every word in the int region is an int.
    }
    if (PyValue_IsInlineFloat(ref)) {
        return !isnan(PyValue_InlineFloat(ref));
    }
    if (PyValue_IsTaggedIndex(ref)) {
        return in_stack;
    }
    if (PyValue_IsNull(ref)) {
        return true;
    }
    return !PyValue_IsMalformed(ref)
           && Py_TYPE(PyValue_AsKnownObjectBorrow(ref)) != NULL;
}

static void
check_frame_extent(_PyInterpreterFrame *frame, int extent)
{
    /* The caller supplies an initialized NULL-control extent. A retiring
     * general return can own all homes after the frame is unlinked. */
    assert(extent >= 0 && extent <= _PyFrame_NumSlots(frame));
    PyValue *stack_base = _PyFrame_Stackbase(frame);
    PyValue *end = frame->base + extent;
    for (PyValue *sp = frame->base; sp < end; sp++) {
        if (!is_well_formed(*sp, sp >= stack_base)) {
            _Py_FatalErrorFormat(__func__,
                "malformed value 0x%" PRIx64 " in slot %d",
                (uint64_t)_PyValue_Word(*sp), (int)(sp - frame->base));
        }
        if (!PyValue_IsNull(*sp) != _PyFrame_HomeIsPresent(frame, sp)) {
            _Py_FatalErrorFormat(__func__,
                "present-home bitmap mismatch in slot %d",
                (int)(sp - frame->base));
        }
    }
}

void
_PyValue_CheckFrame(_PyInterpreterFrame *frame)
{
    /* A waiting thread frame can overlap its child's homes. */
    assert(frame->owner != FRAME_OWNED_BY_THREAD ||
           _PyThreadState_GET()->current_frame == frame);
    check_frame_extent(frame, _PyFrame_NumSlots(frame));
}
#endif

/* A descending one-pass clear. Callbacks can repopulate passed local homes;
 * the cursor does not revisit them. Reload the owner bits after every close. */
static inline void
clear_slots(_PyInterpreterFrame *frame, int n)
{
    frame->phase_state = 0;
#ifdef Py_DEBUG
    check_frame_extent(frame, n);
#endif
    int end = n;
    while (end > 0) {
        int owner = _PyFrame_PreviousPresentHome(frame, 0, end);
        end = owner < 0 ? 0 : owner;
        if (owner >= 0) {
            end = owner;
            PyValue ref = _PyFrame_TakeHome(frame, frame->base + owner);
            PyValue_CLOSE(ref);
        }
    }
    Py_CLEAR(frame->f_locals);
}

void
_PyFrame_ClearLocals(_PyInterpreterFrame *frame)
{
    clear_slots(frame, _PyFrame_NumSlots(frame));
}

/* Stop before the first close that can call Python. The interrupted owner
 * stays in its home until the outlined continuation has published its roots. */
static inline int
try_clear_except_code(_PyInterpreterFrame *frame, int remaining)
{
    assert(remaining >= 0 && remaining <= _PyFrame_NumSlots(frame));
    frame->phase_state = 0;
    assert(_PyThreadState_GET()->current_frame != frame);
    assert(frame->owner != FRAME_OWNED_BY_GENERATOR ||
           FT_ATOMIC_LOAD_INT8_RELAXED(
               _PyGen_GetGeneratorFromFrame(frame)->gi_frame_state) == FRAME_CLEARED);
    if (FT_ATOMIC_LOAD_PTR_RELAXED(frame->frame_obj) != NULL) {
        return _Py_FRAME_CLEAR_ASSOCIATION;
    }
#ifdef Py_DEBUG
    check_frame_extent(frame, remaining);
#endif
    while (remaining > 0) {
        int owner = _PyFrame_PreviousPresentHome(frame, 0, remaining);
        if (owner < 0) {
            remaining = 0;
            break;
        }
        PyValue ref = frame->base[owner];
        if (!_PyValue_TryXCloseNoReentry(ref)) {
            return remaining;
        }
        _PyFrame_WriteHome(frame, frame->base + owner, PyValue_NULL);
        remaining = owner;
    }
    if (_PyFrame_ObjectMayReenter(frame->f_locals)) {
        return _Py_FRAME_CLEAR_LOCALS;
    }
    Py_CLEAR(frame->f_locals);
    if (_PyFrame_ObjectMayReenter((PyObject *)frame->f_funcobj)) {
        return _Py_FRAME_CLEAR_FUNCTION;
    }
    Py_CLEAR(frame->f_funcobj);
    return _Py_FRAME_CLEAR_DONE;
}

int
_PyFrame_TryClearExceptCode(_PyInterpreterFrame *frame)
{
    int extent = frame->owner == FRAME_OWNED_BY_THREAD
        ? _PyFrame_GetCode(frame)->co_nlocalsplus : _PyFrame_NumSlots(frame);
    return try_clear_except_code(frame, extent);
}

int
_PyFrame_TryClearExceptCodeGeneral(_PyInterpreterFrame *frame)
{
    assert(frame->owner == FRAME_OWNED_BY_THREAD);
    return try_clear_except_code(frame, _PyFrame_NumSlots(frame));
}

void
_PyFrame_InitClearRoots(_PyInterpreterFrame *frame, int stage, int initial_extent,
                        PyValue result, _PyTransitionRoots *roots)
{
    assert(initial_extent >= 0 && initial_extent <= _PyFrame_NumSlots(frame));
    assert(stage <= initial_extent);
    frame->phase_state = 0;
    int remaining = stage > 0 ? stage : 0;
    if (stage == _Py_FRAME_CLEAR_ASSOCIATION) {
        remaining = initial_extent;
    }
#ifdef Py_DEBUG
    /* Association release and private heap deallocation can bypass the
     * callback-free sweep. Only the remaining prefix is still ours. */
    check_frame_extent(frame, remaining);
#endif
    *roots = (_PyTransitionRoots){
        .frame = frame,
        .remaining = remaining,
        .home_authority = remaining ? _PyIR_HOME_FRAME_PRESENCE
                                    : _PyIR_HOME_NONE,
        .header_mask = _Py_TRANSITION_CODE |
            (frame->owner == FRAME_OWNED_BY_GENERATOR
                ? _Py_TRANSITION_EXCEPTION : 0) |
            (frame->f_funcobj != NULL ? _Py_TRANSITION_FUNCTION : 0) |
            (frame->f_locals != NULL ? _Py_TRANSITION_LOCALS : 0) |
            (frame->frame_obj != NULL ? _Py_TRANSITION_FRAME_OBJECT : 0),
        .result_present = !PyValue_IsNull(result),
        .kind = _Py_TRANSITION_KIND_RETIRING,
        .result = result,
    };
}

void
_PyFrame_FinishClearExceptCode(_PyInterpreterFrame *frame, int stage,
                              _PyTransitionRoots *roots)
{
    assert(roots->frame == frame);
    if (stage == _Py_FRAME_CLEAR_ASSOCIATION) {
        PyFrameObject *f = FT_ATOMIC_LOAD_PTR_RELAXED(frame->frame_obj);
        assert(f != NULL);
        f->f_line_next = -1;
        roots->header_mask &= ~_Py_TRANSITION_FRAME_OBJECT;
        FT_ATOMIC_STORE_PTR_RELAXED(frame->frame_obj, NULL);
        if (!_PyObject_IsUniquelyReferenced((PyObject *)f)) {
            take_ownership(f, frame, roots);
            Py_DECREF(f);
            return;
        }
        Py_DECREF(f);
    }
    while (roots->remaining > 0) {
        int owner = _PyFrame_PreviousPresentHome(frame, 0, roots->remaining);
        roots->remaining = owner < 0 ? 0 : owner;
        if (owner >= 0) {
            roots->remaining = owner;
            PyValue ref = _PyFrame_TakeHome(frame, frame->base + owner);
            PyValue_CLOSE(ref);
        }
    }
    roots->home_authority = _PyIR_HOME_NONE;
    roots->homes = NULL;
    roots->header_mask &= ~_Py_TRANSITION_LOCALS;
    Py_CLEAR(frame->f_locals);
    roots->header_mask &= ~_Py_TRANSITION_FUNCTION;
    Py_CLEAR(frame->f_funcobj);
}

static Py_NO_INLINE PyValue
clear_generator_slow(PyGenObject *gen, int stage, PyValue result, uint8_t reason,
                      int initial_extent)
{
    PyThreadState *tstate = _PyThreadState_GET();
    _PyInterpreterFrame *frame = &gen->gi_iframe;
#ifndef NDEBUG
    _PyStackChunk *chunk = tstate->datastack_chunk;
    PyObject **top = tstate->datastack_top;
    PyObject **limit = tstate->datastack_limit;
#endif
    _PyTransitionRoots roots;
    _PyFrame_InitClearRoots(frame, stage, initial_extent, result, &roots);
    roots.retire_reason = reason;
    _PyThreadState_PushTransitionRoots(tstate, &roots);
    roots.shadow_status = _PyIR_ShadowRetirement(frame, reason);
    assert(roots.shadow_status != _PyIR_SHADOW_MISMATCH);
    Py_BEGIN_CRITICAL_SECTION(gen);
    if (stage != _Py_FRAME_CLEAR_DONE) {
        _PyFrame_FinishClearExceptCode(frame, stage, &roots);
    }
    Py_END_CRITICAL_SECTION();
    roots.header_mask &= ~_Py_TRANSITION_EXCEPTION;
    _PyErr_ClearExcState(&gen->gi_exc_state);
    /* The generator retains code even after its frame/exception clear. */
    roots.header_mask &= ~_Py_TRANSITION_CODE;
#ifndef NDEBUG
    assert(tstate->datastack_chunk == chunk);
    assert(tstate->datastack_top == top);
    assert(tstate->datastack_limit == limit);
#endif
    result = roots.result;
    roots.result = PyValue_NULL;
    roots.result_present = 0;
    _PyThreadState_PopTransitionRoots(tstate, &roots);
    return result;
}

static inline Py_ALWAYS_INLINE PyValue
clear_generator_at_extent(PyGenObject *gen, PyValue result, uint8_t reason,
                           int initial_extent)
{
    int stage;
    Py_BEGIN_CRITICAL_SECTION(gen);
    stage = try_clear_except_code(&gen->gi_iframe, initial_extent);
    Py_END_CRITICAL_SECTION();
    if (stage != _Py_FRAME_CLEAR_DONE ||
        _PyFrame_ObjectMayReenter(gen->gi_exc_state.exc_value))
    {
        return clear_generator_slow(gen, stage, result, reason, initial_extent);
    }
    _PyErr_ClearExcState(&gen->gi_exc_state);
    return result;
}

PyValue
_PyFrame_ClearGenerator(PyGenObject *gen, PyValue result, uint8_t reason)
{
    return clear_generator_at_extent(gen, result, reason,
                                     _PyFrame_NumSlots(&gen->gi_iframe));
}

PyValue
_PyFrame_ClearGeneratorBindings(PyGenObject *gen, uint8_t reason)
{
    /* The error processor has drained selected scratch. Do not reconstruct
     * an N-sized view from the generator's allocation or its public PC. */
    return clear_generator_at_extent(gen, PyValue_NULL, reason,
        _PyFrame_GetCode(&gen->gi_iframe)->co_nlocalsplus);
}

/* Unstable API functions */

PyObject *
PyUnstable_InterpreterFrame_GetCode(struct _PyInterpreterFrame *frame)
{
    return Py_NewRef(frame->f_executable != NULL
                     ? (PyObject *)frame->f_executable : Py_None);
}

// NOTE: We allow racy accesses to the instruction pointer from other threads
// for sys._current_frames() and similar APIs.
int _Py_NO_SANITIZE_THREAD
PyUnstable_InterpreterFrame_GetLasti(struct _PyInterpreterFrame *frame)
{
    return _PyInterpreterFrame_LASTI(frame);
}

int _Py_NO_SANITIZE_THREAD
PyUnstable_InterpreterFrame_GetLine(_PyInterpreterFrame *frame)
{
    int addr = _PyInterpreterFrame_LASTI(frame);
    return PyCode_Addr2Line(_PyFrame_GetCode(frame), addr);
}
