#define _PY_INTERPRETER

#include "Python.h"
#include "pycore_frame.h"         // _PyFrame_New_NoTrack()
#include "pycore_interpframe.h"   // _PyFrame_GetCode()
#include "pycore_genobject.h"     // _PyGen_GetGeneratorFromFrame()
#include "pycore_pyerrors.h"      // _Py_FatalErrorFormat()
#include "pycore_stackref.h"      // _Py_VISIT_STACKREF()


int
_PyFrame_Traverse(_PyInterpreterFrame *frame, visitproc visit, void *arg)
{
    Py_VISIT(frame->frame_obj);
    Py_VISIT(frame->f_locals);
    _Py_VISIT_STACKREF(frame->f_funcobj);
    _Py_VISIT_STACKREF(frame->f_executable);
    return _PyGC_VisitFrameStack(frame, visit, arg);
}

PyFrameObject *
_PyFrame_MakeAndSetFrameObject(_PyInterpreterFrame *frame)
{
    PyObject *exc = PyErr_GetRaisedException();

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
    assert(frame->frame_obj == NULL);
    frame->frame_obj = f;
    return f;
}

static void
take_ownership(PyFrameObject *f, _PyInterpreterFrame *frame)
{
    Py_BEGIN_CRITICAL_SECTION(f);
    assert(frame->owner < FRAME_OWNED_BY_INTERPRETER);
    assert(frame->owner != FRAME_OWNED_BY_FRAME_OBJECT);
    _PyInterpreterFrame *new_frame = (_PyInterpreterFrame *)f->_f_frame_data;
    _PyFrame_Copy(frame, new_frame);
    f->f_frame = new_frame;
    new_frame->owner = FRAME_OWNED_BY_FRAME_OBJECT;
    if (_PyFrame_IsIncomplete(new_frame)) {
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
is_well_formed(_PyStackRef ref, bool in_stack)
{
#ifdef Py_STACKREF_INLINE_VALUES
    if (PyStackRef_IsInlineInt(ref)) {
        return (_PyStackRef_BITS(ref) >> 32) == (Py_INT_BASE >> 32);
    }
    if (PyStackRef_IsInlineFloat(ref)) {
        return !isnan(PyStackRef_InlineFloat(ref));
    }
#endif
    if (PyStackRef_IsTaggedIndex(ref)) {
        return in_stack;
    }
    if (PyStackRef_IsNull(ref)) {
        return true;
    }
    return !PyStackRef_IsMalformed(ref)
           && Py_TYPE(PyStackRef_AsPyObjectBorrow(ref)) != NULL;
}

void
_PyStackRef_CheckFrame(_PyInterpreterFrame *frame)
{
    _PyStackRef *stack_base = _PyFrame_Stackbase(frame);
    _PyStackRef *end = frame->base + _PyFrame_NumSlots(frame);
    for (_PyStackRef *sp = frame->base; sp < end; sp++) {
        if (!is_well_formed(*sp, sp >= stack_base)) {
            _Py_FatalErrorFormat(__func__,
                "malformed stackref 0x%" PRIx64 " in slot %d",
                (uint64_t)_PyStackRef_BITS(*sp), (int)(sp - frame->base));
        }
    }
}
#endif

/* Release the first n slots of frame, from the top, so that a finalizer
 * that looks at the frame finds the slots that it has not reached yet intact
 * and the rest NULL */
static inline void
clear_slots(_PyInterpreterFrame *frame, int n)
{
#ifdef Py_DEBUG
    _PyStackRef_CheckFrame(frame);
#endif
    _PyStackRef *locals = frame->base;
    _PyStackRef *sp = locals + n;
    while (sp > locals) {
        sp--;
        *sp = PyStackRef_NULL;
    }
    Py_CLEAR(frame->f_locals);
}

void
_PyFrame_ClearLocals(_PyInterpreterFrame *frame)
{
    clear_slots(frame, _PyFrame_NumSlots(frame));
}

void
_PyFrame_ClearExceptCode(_PyInterpreterFrame *frame)
{
    /* It is the responsibility of the owning generator/coroutine
     * to have cleared the enclosing generator, if any. */
    assert(frame->owner != FRAME_OWNED_BY_GENERATOR ||
           FT_ATOMIC_LOAD_INT8_RELAXED(_PyGen_GetGeneratorFromFrame(frame)->gi_frame_state) == FRAME_CLEARED);
    // GH-99729: Clearing this frame can expose the stack (via finalizers). It's
    // crucial that this frame has been unlinked, and is no longer visible:
    assert(_PyThreadState_GET()->current_frame != frame);
    PyFrameObject *f = FT_ATOMIC_LOAD_PTR_RELAXED(frame->frame_obj);
    if (f != NULL) {
        FT_ATOMIC_STORE_PTR_RELAXED(frame->frame_obj, NULL);
        // The frame object may outlive the frame, so it takes the frame's
        // contents.
        take_ownership(f, frame);
        Py_DECREF(f);
        return;
    }
    if (frame->owner == FRAME_OWNED_BY_THREAD) {
        // A frame on the frame stack ends with its temporaries NULL: a
        // return leaves them so, and so does the unwinding of an exception.
        _PyFrame_AssertTemporariesClear(frame);
        clear_slots(frame, _PyFrame_GetCode(frame)->co_nlocalsplus);
    }
    else {
        _PyFrame_ClearLocals(frame);
    }
    PyStackRef_CLEAR(frame->f_funcobj);
}

/* Unstable API functions */

PyObject *
PyUnstable_InterpreterFrame_GetCode(struct _PyInterpreterFrame *frame)
{
    return PyStackRef_AsPyObjectBorrow(frame->f_executable);
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
