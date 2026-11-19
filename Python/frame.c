#define _PY_INTERPRETER

#include "Python.h"
#include "pycore_ceval.h"         // _PyEval_InvalidateSourceErrors()
#include "pycore_frame.h"         // _PyFrame_New_NoTrack()
#include "pycore_interpframe.h"   // _PyFrame_GetCode()
#include "pycore_genobject.h"     // _PyGen_GetGeneratorFromFrame()
#include "pycore_pyerrors.h"      // _PyErr_ClearExcState()
#include "pycore_pystate.h"       // HEAD_LOCK(), thread enumeration
#include "pycore_value.h"      // _Py_VISIT_VALUE()

static int
find_error_home_view(PyThreadState *owner, _PyInterpreterFrame *frame,
                     _PyIRHomeView *view, _PyIRHomeAuthority *authority)
{
    int matches = 0;
    for (_PyErrorScope *scope = ((_PyThreadStateImpl *)owner)->error_scopes;
         scope != NULL; scope = scope->previous)
    {
        if (scope->frame != frame) {
            continue;
        }
        if (scope->homes == NULL ||
            (scope->home_authority != _PyIR_HOME_MUST_READABLE &&
             scope->home_authority != _PyIR_HOME_NULL_CONTROL_READABLE))
        {
            return -1;
        }
        *view = *scope->homes;
        view->scratch_limit = (uint16_t)Py_MIN(view->scratch_limit,
            Py_MAX(scope->remaining - view->nbindings, 0));
        *authority = scope->home_authority;
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
            (f->f_capture_authority != _PyIR_HOME_MUST_READABLE &&
             f->f_capture_authority != _PyIR_HOME_NULL_CONTROL_READABLE))
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
    return _PyIR_GetFrameHomeView(frame, view);
}

void
_PyFrame_GetNullControlHomeView(_PyInterpreterFrame *frame, int remaining,
                               _PyIRHomeView *view)
{
    static const _PyIRPhaseRange all_scratch = {
        .begin = 0, .end = IR_MAX_SLOTS, .flags = _PyIR_PHASE_RANGE_NULLABLE};
    int nl = _PyFrame_GetCode(frame)->co_nlocalsplus;
    assert(remaining >= 0 && remaining <= _PyFrame_NumSlots(frame));
    *view = (_PyIRHomeView){
        .nbindings = (uint16_t)nl,
        .scratch_limit = (uint16_t)Py_MAX(remaining - nl, 0),
        .nranges = remaining > nl,
        .ranges = &all_scratch,
    };
}

/* This development adapter is explicit: candidate/unsupported phases still
 * rely on the initialized NULL control, not on a must-readable static proof. */
_PyIRHomeAuthority
_PyFrame_GetCopyViewUnderNullControl(_PyInterpreterFrame *frame,
                                    _PyIRHomeView *view)
{
    _PyIRHomeAuthority authority = _PyFrame_GetHomeView(frame, view);
    if (authority == _PyIR_HOME_MUST_READABLE ||
        authority == _PyIR_HOME_NULL_CONTROL_READABLE)
    {
        return authority;
    }
    _PyFrame_GetNullControlHomeView(frame, _PyFrame_NumSlots(frame), view);
    return _PyIR_HOME_NULL_CONTROL_READABLE;
}

static Py_NO_INLINE void
copy_created_generator_control(_PyInterpreterFrame *src, _PyInterpreterFrame *dest)
{
    _PyIRHomeView view;
    _PyIRHomeAuthority authority =
        _PyFrame_GetCopyViewUnderNullControl(src, &view);
    src->instr_ptr++;
    _PyFrame_Copy(src, dest, &view, authority);
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
        dest->base[i] = src->base[i];
        src->base[i] = PyValue_NULL;
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
    frame->base[slot] = PyValue_FromObjectSteal(dict);
    frame->pending_kwargs = IR_NO_DST;
    return 0;
}

/* Move a frame that a frame object outlives into that object. Its first
 * extent slots are the possible owners; every slot holds a value or NULL. */
static void
take_ownership(PyFrameObject *f, _PyInterpreterFrame *frame, int extent)
{
    Py_BEGIN_CRITICAL_SECTION(f);
    assert(frame->owner < FRAME_OWNED_BY_INTERPRETER);
    assert(frame->owner != FRAME_OWNED_BY_FRAME_OBJECT);
    _PyInterpreterFrame *new_frame = (_PyInterpreterFrame *)f->_f_frame_data;
    _PyIRHomeView view;
    _PyIRHomeAuthority authority = _PyIR_HOME_NULL_CONTROL_READABLE;
    _PyFrame_GetNullControlHomeView(frame, extent, &view);
    _PyFrame_Copy(frame, new_frame, &view, authority);
    // _PyFrame_Copy moves the reference to the executable, but the caller
    // still clears frame->f_executable, so give the source frame its own.
    frame->f_executable = (PyCodeObject *)Py_NewRef(new_frame->f_executable);
    f->f_frame = new_frame;
    new_frame->owner = FRAME_OWNED_BY_FRAME_OBJECT;
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
    assert(extent >= 0 && extent <= _PyFrame_NumSlots(frame));
    PyValue *stack_base = _PyFrame_Stackbase(frame);
    PyValue *end = frame->base + extent;
    for (PyValue *sp = frame->base; sp < end; sp++) {
        if (!is_well_formed(*sp, sp >= stack_base)) {
            _Py_FatalErrorFormat(__func__,
                "malformed value 0x%" PRIx64 " in slot %d",
                (uint64_t)_PyValue_Word(*sp), (int)(sp - frame->base));
        }
    }
}

void
_PyValue_CheckFrame(_PyInterpreterFrame *frame)
{
    check_frame_extent(frame, _PyFrame_NumSlots(frame));
}

void
_PyValue_CheckFrameExtent(_PyInterpreterFrame *frame, int extent)
{
    check_frame_extent(frame, extent);
}
#endif

/* Release the first n slots of frame, from the top, so that a finalizer
 * that looks at the frame finds the slots that it has not reached yet intact
 * and the rest NULL */
static inline void
clear_slots(_PyInterpreterFrame *frame, int n)
{
    frame->phase_state = 0;
#ifdef Py_DEBUG
    _PyValue_CheckFrameExtent(frame, n);
#endif
    PyValue *locals = frame->base;
    PyValue *sp = locals + n;
    while (sp > locals) {
        sp--;
        // a move: cleared below
        PyValue ref = *sp;
        *sp = PyValue_NULL;
        PyValue_XCLOSE(ref);
    }
    Py_CLEAR(frame->f_locals);
}

void
_PyFrame_ClearLocals(_PyInterpreterFrame *frame)
{
    clear_slots(frame, _PyFrame_NumSlots(frame));
}

void
_PyFrame_ClearExceptCode(_PyInterpreterFrame *frame, int extent)
{
    assert(extent >= 0 && extent <= _PyFrame_NumSlots(frame));
    // GH-99729: Clearing this frame can expose the stack (via finalizers). It's
    // crucial that this frame has been unlinked, and is no longer visible:
    assert(_PyThreadState_GET()->current_frame != frame);
    assert(frame->owner != FRAME_OWNED_BY_GENERATOR ||
           FT_ATOMIC_LOAD_INT8_RELAXED(
               _PyGen_GetGeneratorFromFrame(frame)->gi_frame_state) == FRAME_CLEARED);
    frame->phase_state = 0;
    PyFrameObject *f = FT_ATOMIC_LOAD_PTR_RELAXED(frame->frame_obj);
    if (f != NULL) {
        f->f_line_next = -1;
        FT_ATOMIC_STORE_PTR_RELAXED(frame->frame_obj, NULL);
        if (!_PyObject_IsUniquelyReferenced((PyObject *)f)) {
            take_ownership(f, frame, extent);
            Py_DECREF(f);
            return;
        }
        Py_DECREF(f);
    }
    clear_slots(frame, extent);
    Py_CLEAR(frame->f_funcobj);
}

static inline PyValue
clear_generator_at_extent(PyGenObject *gen, PyValue result, int extent)
{
    Py_BEGIN_CRITICAL_SECTION(gen);
    _PyFrame_ClearExceptCode(&gen->gi_iframe, extent);
    Py_END_CRITICAL_SECTION();
    _PyErr_ClearExcState(&gen->gi_exc_state);
    return result;
}

PyValue
_PyFrame_ClearGenerator(PyGenObject *gen, PyValue result)
{
    return clear_generator_at_extent(gen, result,
                                     _PyFrame_NumSlots(&gen->gi_iframe));
}

void
_PyFrame_ClearGeneratorBindings(PyGenObject *gen)
{
    /* The error processor has drained selected scratch. Do not reconstruct
     * an N-sized view from the generator's allocation or its public PC. */
    (void)clear_generator_at_extent(gen, PyValue_NULL,
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
