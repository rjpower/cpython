#ifndef Py_INTERNAL_INTERP_FRAME_H
#define Py_INTERNAL_INTERP_FRAME_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_code.h"          // _PyCode_WORDS()
#include "pycore_interpframe_structs.h" // _PyInterpreterFrame
#include "pycore_ir.h"            // _PyCodeExec
#include "pycore_pyatomic_ft_wrappers.h" // FT_ATOMIC_LOAD_PTR_ACQUIRE()
#include "pycore_value.h"         // PyValue_ObjectBorrow()
#include "pycore_stats.h"         // CALL_STAT_INC()

#ifdef __cplusplus
extern "C" {
#endif

#define _PyInterpreterFrame_LASTI(IF) \
    _PyFrame_InstrOffset((IF), (IF)->instr_ptr)

static inline PyCodeObject *_PyFrame_GetCode(_PyInterpreterFrame *f) {
    assert(!PyValue_IsNull(f->f_executable));
    PyObject *executable = PyValue_ObjectBorrow(f->f_executable);
    assert(PyCode_Check(executable));
    return (PyCodeObject *)executable;
}

/* The executing copy of the code of f, which f->instr_ptr points into. */
static inline _PyCodeExec *
_PyFrame_Exec(_PyInterpreterFrame *f)
{
    _PyCodeExec *exec = _PyFrame_GetCode(f)->_co_exec;
    assert(exec != NULL && exec->words == f->ir_words);
    return exec;
}

/* The poly table of the feedback entry H of f's code, which has one. */
static inline _PyFbPoly *
_PyFrame_FbPoly(_PyInterpreterFrame *f, _PyFbHeader *h)
{
    _PyFbBlock *block = _PyFrame_Exec(f)->feedback;
    assert(h->poly != 0 && h->poly <= block->npoly);
    return &block->poly[h->poly - 1];
}

/* The row of POLY for SHAPE at the shape's live version, or -1: the rows
 * that LOAD_ATTR_SHAPE_POLY reads.  Versions are never reused, so a row
 * whose shape has died, and whose address a new shape now has, does not
 * match it.  An unused row has no shape. */
static inline int
_PyFb_ShapeRow(const _PyFbPoly *poly, _PyShape *shape)
{
    uint32_t version = FT_ATOMIC_LOAD_UINT32_RELAXED(shape->type_version);
    for (int i = 0; i < FB_POLY_SIZE; i++) {
        if (poly->shape[i] == shape && poly->type_version[i] == version) {
            return i;
        }
    }
    return -1;
}

/* The word index of the instruction at instr, which is f_lasti for
 * frame->instr_ptr.  The word before the first, which the unwinding of a
 * frame that has not finished an instruction reads, is at -1. */
static inline int
_PyFrame_InstrOffset(_PyInterpreterFrame *f, const _PyIRWord *instr)
{
    return (int)(instr - f->ir_words);
}

/* The word of the instruction at a word index. */
static inline _PyIRWord *
_PyFrame_InstrAt(_PyInterpreterFrame *f, int offset)
{
    return f->ir_words + offset;
}

/* Set the bases that the handlers of the frame index, for exec, which
 * frame->instr_ptr points into. */
static inline void
_PyFrame_SetExec(_PyInterpreterFrame *frame, const _PyCodeExec *exec)
{
    frame->ir_words = exec->words;
    frame->ir_feedback = exec->entries;
}

/* Start a frame of code, whose executing copy exists: _PyCode_GetExec()
 * has succeeded for it, which the call paths ensure. */
static inline void
_PyFrame_InitIRInstrPtr(_PyInterpreterFrame *frame, PyCodeObject *code)
{
    _PyCodeExec *exec = (_PyCodeExec *)FT_ATOMIC_LOAD_PTR_ACQUIRE(code->_co_exec);
    assert(exec != NULL);
    frame->instr_ptr = exec->words;
    _PyFrame_SetExec(frame, exec);
}

// Similar to _PyFrame_GetCode(), but return NULL if the frame is invalid or
// freed. Used by dump_frame() in Python/traceback.c. The function uses
// heuristics to detect freed memory, it's not 100% reliable.
static inline PyCodeObject* _Py_NO_SANITIZE_THREAD
_PyFrame_SafeGetCode(_PyInterpreterFrame *f)
{
    // globals and builtins may be NULL on a legit frame, but it's unlikely.
    // It's more likely that it's a sign of an invalid frame.
    if (f->f_globals == NULL || f->f_builtins == NULL) {
        return NULL;
    }

    if (PyValue_IsNull(f->f_executable)) {
        return NULL;
    }
    void *ptr;
    memcpy(&ptr, &f->f_executable, sizeof(f->f_executable));
    if (_PyMem_IsPtrFreed(ptr)) {
        return NULL;
    }
    PyObject *executable = PyValue_ObjectBorrow(f->f_executable);
    if (_PyObject_IsFreed(executable)) {
        return NULL;
    }
    if (!PyCode_Check(executable)) {
        return NULL;
    }
    return (PyCodeObject *)executable;
}


// Similar to PyUnstable_InterpreterFrame_GetLasti(), but return NULL if the
// frame is invalid or freed. Used by dump_frame() in Python/traceback.c. The
// function uses heuristics to detect freed memory, it's not 100% reliable.
static inline int _Py_NO_SANITIZE_THREAD
_PyFrame_SafeGetLasti(struct _PyInterpreterFrame *f)
{
    // Code based on _PyFrame_GetBytecode() but replace _PyFrame_GetCode()
    // with _PyFrame_SafeGetCode().
    PyCodeObject *co = _PyFrame_SafeGetCode(f);
    if (co == NULL) {
        return -1;
    }

    _PyCodeExec *exec = co->_co_exec;
    if (exec == NULL || f->instr_ptr < exec->words
        || f->instr_ptr >= exec->words + exec->nwords) {
        return -1;
    }
    return (int)(f->instr_ptr - exec->words);
}

static inline PyFunctionObject *_PyFrame_GetFunction(_PyInterpreterFrame *f) {
    PyObject *func = PyValue_ObjectBorrow(f->f_funcobj);
    assert(PyFunction_Check(func));
    return (PyFunctionObject *)func;
}

static inline PyValue *_PyFrame_Stackbase(_PyInterpreterFrame *f) {
    return (f->base + _PyFrame_GetCode(f)->co_nlocalsplus);
}

#define FRAME_SPECIALS_SIZE ((int)((sizeof(_PyInterpreterFrame)-1)/sizeof(PyObject *)))

static inline int
_PyFrame_NumSlotsForCodeObject(PyCodeObject *code)
{
    /* This function needs to remain in sync with the calculation of
     * co_framesize in Tools/build/deepfreeze.py */
    assert(code->co_framesize >= FRAME_SPECIALS_SIZE);
    return code->co_framesize - FRAME_SPECIALS_SIZE;
}

/* The slots of f, which the IR's fields index */
static inline PyValue *
_PyFrame_Slots(_PyInterpreterFrame *f)
{
    return f->base;
}

/* co_nlocalsplus + co_stacksize: the number of slots of f */
static inline int
_PyFrame_NumSlots(_PyInterpreterFrame *f)
{
    return _PyFrame_NumSlotsForCodeObject(_PyFrame_GetCode(f));
}

/* Hand v to f, which a call returns to or a send resumes: it goes to the
 * slot that f named.  A temporary's slot holds NULL; a local's old value,
 * where a call stores its result in a local, is overwritten. */
static inline void
_PyFrame_SetReturnValue(_PyInterpreterFrame *f, PyValue v)
{
#ifdef Py_DEBUG
    if (f->owner != FRAME_OWNED_BY_INTERPRETER) {
        assert(f->return_slot < _PyFrame_NumSlots(f));
        assert(f->return_slot < _PyFrame_GetCode(f)->co_nlocalsplus
               || PyValue_IsNull(f->base[f->return_slot]));
    }
#endif
    f->base[f->return_slot] = v;
}

/* Leave the temporaries of f from slot first on NULL */
static inline void
_PyFrame_ClearTemporaries(_PyInterpreterFrame *f, int first)
{
    int nslots = _PyFrame_NumSlots(f);
    for (int i = first; i < nslots; i++) {
        f->base[i] = PyValue_NULL;
    }
}

/* Whether every temporary of f is NULL, as when it returns */
static inline void
_PyFrame_AssertTemporariesClear(_PyInterpreterFrame *f)
{
#ifdef Py_DEBUG
    int nslots = _PyFrame_NumSlots(f);
    for (int i = _PyFrame_GetCode(f)->co_nlocalsplus; i < nslots; i++) {
        assert(PyValue_IsNull(f->base[i]));
    }
#else
    (void)f;
#endif
}

static inline void _PyFrame_Copy(_PyInterpreterFrame *src, _PyInterpreterFrame *dest)
{
    int nslots = _PyFrame_NumSlots(src);
    dest->f_executable = src->f_executable;
    dest->base = dest->owned_slots;
    // Don't leave a dangling pointer to the old frame when creating generators
    // and coroutines:
    dest->previous = NULL;
    dest->f_funcobj = src->f_funcobj;
    dest->f_globals = src->f_globals;
    dest->f_builtins = src->f_builtins;
    dest->f_locals = src->f_locals;
    dest->frame_obj = src->frame_obj;
    dest->instr_ptr = src->instr_ptr;
    dest->ir_words = src->ir_words;
    dest->ir_feedback = src->ir_feedback;
    dest->return_slot = src->return_slot;
    // visited is GC bookkeeping for the current stack walk, not frame state.
    dest->visited = 0;
#ifdef Py_DEBUG
    dest->lltrace = src->lltrace;
#endif
    // A move: the slots that src leaves behind may be the caller's
    for (int i = 0; i < nslots; i++) {
        dest->base[i] = src->base[i];
        src->base[i] = PyValue_NULL;
    }
}


/* Consumes reference to func and locals.
   Does not initialize frame->previous, which happens
   when frame is linked into the frame stack.
 */
static inline void
_PyFrame_Initialize(
    PyThreadState *tstate, _PyInterpreterFrame *frame, PyValue func,
    PyObject *locals, PyCodeObject *code, PyValue *base,
    int null_locals_from, _PyInterpreterFrame *previous)
{
    frame->previous = previous;
    frame->base = base;
    frame->f_funcobj = func;
    frame->f_executable = PyValue_FromObject(code);
    PyFunctionObject *func_obj = (PyFunctionObject *)PyValue_ObjectBorrow(func);
    frame->f_builtins = func_obj->func_builtins;
    frame->f_globals = func_obj->func_globals;
    frame->f_locals = locals;
    frame->frame_obj = NULL;
    (void)tstate;
    _PyFrame_InitIRInstrPtr(frame, code);
    frame->return_slot = 0;
    frame->owner = FRAME_OWNED_BY_THREAD;
    frame->visited = 0;
#ifdef Py_DEBUG
    frame->lltrace = 0;
#endif

    int nslots = _PyFrame_NumSlotsForCodeObject(code);
    for (int i = null_locals_from; i < nslots; i++) {
        frame->base[i] = PyValue_NULL;
    }
}

/* Gets the pointer to the locals array
 * that precedes this frame.
 */
static inline PyValue*
_PyFrame_GetLocalsArray(_PyInterpreterFrame *frame)
{
    return frame->base;
}

/* Determine whether a frame is incomplete.
 * A frame is incomplete if it is part way through
 * creating cell objects or a generator or coroutine.
 *
 * Frames on the frame stack are incomplete until the
 * first RESUME instruction.
 * Frames owned by a generator are always complete.
 *
 * NOTE: We allow racy accesses to the instruction pointer
 * from other threads for sys._current_frames() and similar APIs.
 */
static inline bool _Py_NO_SANITIZE_THREAD
_PyFrame_IsIncomplete(_PyInterpreterFrame *frame)
{
    if (frame->owner >= FRAME_OWNED_BY_INTERPRETER) {
        return true;
    }
    return frame->owner != FRAME_OWNED_BY_GENERATOR &&
           _PyFrame_InstrOffset(frame, frame->instr_ptr) <
               _PyFrame_GetCode(frame)->_co_firsttraceable;
}

static inline _PyInterpreterFrame *
_PyFrame_GetFirstComplete(_PyInterpreterFrame *frame)
{
    while (frame && _PyFrame_IsIncomplete(frame)) {
        frame = frame->previous;
    }
    return frame;
}

static inline _PyInterpreterFrame *
_PyThreadState_GetFrame(PyThreadState *tstate)
{
    return _PyFrame_GetFirstComplete(tstate->current_frame);
}

// Update last_profiled_frame for remote profiler frame caching.
// Only update if we're removing the exact frame that was last profiled.
// This avoids corrupting the cache when transient frames (called and returned
// between profiler samples) update last_profiled_frame to addresses the
// profiler never saw.
// The sequence distinguishes this anchor from a later frame that reuses the
// same _PyInterpreterFrame address.
#define _PyThreadState_UpdateLastProfiledFrame(tstate, frame, previous) \
    do { \
        PyThreadState *tstate_ = (tstate); \
        _PyInterpreterFrame *frame_ = (frame); \
        if (tstate_->last_profiled_frame == frame_) { \
            tstate_->last_profiled_frame = (previous); \
            tstate_->last_profiled_frame_seq++; \
        } \
    } while (0)

/* For use by _PyFrame_GetFrameObject
  Do not call directly. */
PyAPI_FUNC(PyFrameObject *)
_PyFrame_MakeAndSetFrameObject(_PyInterpreterFrame *frame);

/* Gets the PyFrameObject for this frame, lazily
 * creating it if necessary.
 * Returns a borrowed reference */
static inline PyFrameObject *
_PyFrame_GetFrameObject(_PyInterpreterFrame *frame)
{

    assert(!_PyFrame_IsIncomplete(frame));
    PyFrameObject *res = (PyFrameObject*)FT_ATOMIC_LOAD_PTR_ACQUIRE(frame->frame_obj);
    if (res != NULL) {
        return res;
    }
    return _PyFrame_MakeAndSetFrameObject(frame);
}

// Exported for external JIT support
PyAPI_FUNC(void)
_PyFrame_ClearLocals(_PyInterpreterFrame *frame);

#ifdef Py_DEBUG
// Abort if a live slot of the frame holds a malformed value.
// Exported for lltrace in _testinternalcapi, which includes ceval.h.
PyAPI_FUNC(void) _PyFrame_CheckValues(_PyInterpreterFrame *frame);
#endif

/* Clears all references in the frame.
 * If take is non-zero, then the _PyInterpreterFrame frame
 * may be transferred to the frame object it references
 * instead of being cleared. Either way
 * the caller no longer owns the references
 * in the frame.
 * take should  be set to 1 for heap allocated
 * frames like the ones in generators and coroutines.
 *
 * Exported for external JIT support
 */
 PyAPI_FUNC(void)
_PyFrame_ClearExceptCode(_PyInterpreterFrame * frame);

int
_PyFrame_Traverse(_PyInterpreterFrame *frame, visitproc visit, void *arg);

bool
_PyFrame_HasHiddenLocals(_PyInterpreterFrame *frame);

PyObject *
_PyFrame_GetLocals(_PyInterpreterFrame *frame);

static inline bool
_PyThreadState_HasStackSpace(PyThreadState *tstate, int size)
{
    assert(
        (tstate->datastack_top == NULL && tstate->datastack_limit == NULL)
        ||
        (tstate->datastack_top != NULL && tstate->datastack_limit != NULL)
    );
    return tstate->datastack_top != NULL &&
        size < tstate->datastack_limit - tstate->datastack_top;
}

/* Push a frame header and nslots slots, the header onto the frame stack and
 * the slots onto the value stack.  Must be guarded by
 * _PyThreadState_HasStackSpace(), for nslots + FRAME_SPECIALS_SIZE. */
static inline _PyInterpreterFrame *
_PyThreadState_PushFrameUnchecked(PyThreadState *tstate, int nslots)
{
    _PyInterpreterFrame *frame =
        (_PyInterpreterFrame *)(tstate->datastack_limit - FRAME_SPECIALS_SIZE);
    frame->base = (PyValue *)tstate->datastack_top;
    frame->saved_top = frame->base;
    tstate->datastack_limit = (PyObject **)frame;
    tstate->datastack_top += nslots;
    assert(tstate->datastack_top < tstate->datastack_limit);
    return frame;
}

/* Push a frame of size words, a header and size - FRAME_SPECIALS_SIZE
 * slots, on a new chunk if there is no room.
 * Exported for external JIT support */
PyAPI_FUNC(_PyInterpreterFrame *)
_PyThreadState_PushFrame(PyThreadState *tstate, size_t size);

/* Pop frame, the first frame of the current chunk, and the chunk */
PyAPI_FUNC(void) _PyThreadState_PopChunk(PyThreadState *tstate, _PyInterpreterFrame *frame);

/* Pop frame, the last frame pushed, from the frame stack and its slots from
 * the value stack */
static inline void
_PyThreadState_PopFrame(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    _PyStackChunk *chunk = tstate->datastack_chunk;
    assert(chunk);
    assert((PyObject **)frame == tstate->datastack_limit);
    // Only the frame that a chunk was allocated for has its first slot.  The
    // root chunk stays, for the next frame.
    if (frame->base == (PyValue *)&chunk->data[0] && chunk->previous != NULL) {
        _PyThreadState_PopChunk(tstate, frame);
        return;
    }
    tstate->datastack_limit = (PyObject **)frame + FRAME_SPECIALS_SIZE;
    tstate->datastack_top = (PyObject **)frame->saved_top;
    assert(tstate->datastack_top < tstate->datastack_limit);
}

/* Pushes a frame without checking for space.
 * Must be guarded by _PyThreadState_HasStackSpace()
 * Consumes reference to func. */
static inline _PyInterpreterFrame *
_PyFrame_PushUnchecked(PyThreadState *tstate, PyValue func, int null_locals_from, _PyInterpreterFrame * previous)
{
    CALL_STAT_INC(frames_pushed);
    PyFunctionObject *func_obj = (PyFunctionObject *)PyValue_ObjectBorrow(func);
    PyCodeObject *code = (PyCodeObject *)func_obj->func_code;
    _PyInterpreterFrame *new_frame = _PyThreadState_PushFrameUnchecked(
        tstate, _PyFrame_NumSlotsForCodeObject(code));
    _PyFrame_Initialize(tstate, new_frame, func, NULL, code,
                        new_frame->base, null_locals_from, previous);
    return new_frame;
}

/* Pushes a frame for the function func whose slots start at base, among the
 * slots of previous, the current frame, where its nargs arguments already
 * are: the frame overlaps the window of the call.  The slots of previous
 * above base are dead temporaries, which are NULL and which the callee
 * leaves NULL.  Must be guarded by _PyThreadState_HasStackSpace(), for
 * co_framesize.  Consumes the reference to func. */
static inline _PyInterpreterFrame *
_PyFrame_PushOverlapping(PyThreadState *tstate, PyValue func,
                         PyValue *base, int nargs,
                         _PyInterpreterFrame *previous)
{
    CALL_STAT_INC(frames_pushed);
    PyFunctionObject *func_obj = (PyFunctionObject *)PyValue_ObjectBorrow(func);
    PyCodeObject *code = (PyCodeObject *)func_obj->func_code;
    assert(previous->owner == FRAME_OWNED_BY_THREAD);
    assert(base > previous->base &&
           base <= (PyValue *)tstate->datastack_top);
    _PyInterpreterFrame *frame =
        (_PyInterpreterFrame *)(tstate->datastack_limit - FRAME_SPECIALS_SIZE);
    frame->saved_top = (PyValue *)tstate->datastack_top;
    tstate->datastack_limit = (PyObject **)frame;
    PyValue *end = base + _PyFrame_NumSlotsForCodeObject(code);
    if (end > frame->saved_top) {
        tstate->datastack_top = (PyObject **)end;
    }
    assert(tstate->datastack_top < tstate->datastack_limit);
    _PyFrame_Initialize(tstate, frame, func, NULL, code, base, nargs, previous);
    return frame;
}

/* Pushes a trampoline frame without checking for space.
 * Must be guarded by _PyThreadState_HasStackSpace() */
static inline _PyInterpreterFrame *
_PyFrame_PushTrampolineUnchecked(PyThreadState *tstate, PyCodeObject *code, int return_slot, _PyInterpreterFrame * previous)
{
    CALL_STAT_INC(frames_pushed);
    int nslots = _PyFrame_NumSlotsForCodeObject(code);
    _PyInterpreterFrame *frame = _PyThreadState_PushFrameUnchecked(tstate, nslots);
    frame->previous = previous;
    frame->f_funcobj = _PyValue_None;
    frame->f_executable = PyValue_FromObject(code);
#ifdef Py_DEBUG
    frame->f_builtins = NULL;
    frame->f_globals = NULL;
#endif
    frame->f_locals = NULL;
    frame->frame_obj = NULL;
    _PyFrame_InitIRInstrPtr(frame, code);
    frame->owner = FRAME_OWNED_BY_THREAD;
    frame->visited = 0;
#ifdef Py_DEBUG
    frame->lltrace = 0;
#endif
    assert(return_slot < nslots);
    frame->return_slot = (uint16_t)return_slot;
    for (int i = 0; i < nslots; i++) {
        frame->base[i] = PyValue_NULL;
    }
    return frame;
}

PyAPI_FUNC(_PyInterpreterFrame *)
_PyEvalFramePushAndInit(PyThreadState *tstate, PyValue func,
                        PyObject *locals, PyValue const *args,
                        size_t argcount, PyObject *kwnames,
                        _PyInterpreterFrame *previous);

PyAPI_FUNC(_PyInterpreterFrame *)
_PyEvalFramePushAndInit_Ex(PyThreadState *tstate, PyValue func,
    PyObject *locals, Py_ssize_t nargs, PyObject *callargs, PyObject *kwargs, _PyInterpreterFrame *previous);

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_INTERP_FRAME_H
