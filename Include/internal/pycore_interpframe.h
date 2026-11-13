#ifndef Py_INTERNAL_INTERP_FRAME_H
#define Py_INTERNAL_INTERP_FRAME_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_code.h"          // _PyCode_WORDS()
#include "pycore_interpframe_structs.h" // _PyInterpreterFrame
#include "pycore_ir.h"            // _PyCodeExec
#include "pycore_pyatomic_ft_wrappers.h" // FT_ATOMIC_LOAD_PTR_ACQUIRE()
#include "pycore_value.h"      // PyValue_AsKnownObjectBorrow()
#include "pycore_stats.h"         // CALL_STAT_INC()
#include "pycore_tuple.h"         // PyTuple_GET_VALUE()

#ifdef __cplusplus
extern "C" {
#endif

#define _PyInterpreterFrame_LASTI(IF) \
    _PyFrame_InstrOffset((IF), (IF)->instr_ptr)

static inline PyCodeObject *_PyFrame_GetCode(_PyInterpreterFrame *f) {
    assert(f->f_executable != NULL);
    assert(PyCode_Check(f->f_executable));
    return f->f_executable;
}

/* The executing copy of the code of f, which f->instr_ptr points into. */
static inline _PyCodeExec *
_PyFrame_Exec(_PyInterpreterFrame *f)
{
    _PyCodeExec *exec = _PyFrame_GetCode(f)->_co_exec;
    assert(exec != NULL && exec->words == f->ir_words);
    return exec;
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
#ifdef Py_GIL_DISABLED
    frame->tlbc_index = 0;
#endif
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

    if (f->f_executable == NULL) {
        return NULL;
    }
    PyCodeObject *ptr = f->f_executable;
    if (_PyMem_IsPtrFreed(ptr)) {
        return NULL;
    }
    PyObject *executable = (PyObject *)f->f_executable;
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
    assert(f->f_funcobj != NULL);
    assert(PyFunction_Check(f->f_funcobj));
    return f->f_funcobj;
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
 * slot that f named. A temporary destination has no owner; a local's old value,
 * where a call stores its result in a local, is released as STORE_FAST
 * releases it. */
static inline void
_PyFrame_PublishReturnSlot(_PyInterpreterFrame *f, PyValue v)
{
    f->phase_state = 0;
#ifdef Py_DEBUG
    if (f->owner != FRAME_OWNED_BY_INTERPRETER) {
        assert(f->return_slot < _PyFrame_NumSlots(f));
    }
#endif
    /* Caller publication reserves a vacant scratch recipient. Its physical
     * word is not a displaced owner; interpreter entry also receives once. */
    PyValue old = PyValue_NULL;
    if (f->owner != FRAME_OWNED_BY_INTERPRETER &&
        f->return_slot < _PyFrame_GetCode(f)->co_nlocalsplus)
    {
        old = f->base[f->return_slot];
    }
    f->base[f->return_slot] = v;
    _PyValue_CheckHome(v);
    PyValue_XCLOSE(old);
}

void _PyFrame_CompleteRedirect(_PyInterpreterFrame *frame);

static inline int
_PyFrame_SetReturnValue(_PyInterpreterFrame *f, PyValue v)
{
    f->phase_state = 0;
    int redirected = f->pending_redirect;
    if (redirected) {
        _PyFrame_CompleteRedirect(f);
    }
    if (f->return_slot == IR_NO_DST) {
        PyValue_XCLOSE(v);
        return redirected;
    }
    _PyFrame_PublishReturnSlot(f, v);
    return redirected;
}

/* Materialize a compiler-proven virtual empty kwargs binding before a read
 * or before publishing a frame object to observers. */
int _PyFrame_MaterializeKwargs(_PyInterpreterFrame *frame);

/* Release the temporaries of f from slot first on and leave them NULL.
 * The slot rules of register-ir.md §1.5.4 (regcfg.c:_PyRegCfg_Compact)
 * make slot order the stack order: a temporary takes the lowest slot above
 * every slot held at its definition, a canonical temporary takes the slot
 * of its stack position, window outputs stay in their window, and a folded
 * load's reserved slot holds NULL. So clearing from the highest slot down
 * releases the latest value first, as upstream pops its stack: the inputs
 * a raising handler left go before the temporaries beneath them. */
static inline void
_PyFrame_ClearTemporaries(_PyInterpreterFrame *f, int first)
{
    f->phase_state = 0;
    for (int i = _PyFrame_NumSlots(f); i-- > first;) {
        PyValue ref = f->base[i];  // a move: cleared below
        if (!PyValue_IsNull(ref)) {
            f->base[i] = PyValue_NULL;
            PyValue_CLOSE(ref);
        }
    }
}

/* Bindings stay dynamic. Scratch ranges are clipped before touching homes. */
static inline bool
_PyFrame_NextHomeRange(const _PyIRHomeView *view, uint32_t *cursor,
                       int *begin, int *end)
{
    if (*cursor == 0) {
        *cursor = 1;
        if (view->nbindings != 0) {
            *begin = 0;
            *end = view->nbindings;
            return true;
        }
    }
    while (*cursor - 1 < view->nranges) {
        const _PyIRPhaseRange *range = &view->ranges[(*cursor)++ - 1];
        if (range->begin >= view->scratch_limit) {
            return false;
        }
        *begin = view->nbindings + range->begin;
        *end = view->nbindings + Py_MIN(range->end, view->scratch_limit);
        if (*begin < *end) {
            return true;
        }
    }
    return false;
}

/* The caller pins code for the entire view/copy commit. */
PyAPI_FUNC(_PyIRHomeAuthority) _PyFrame_GetHomeView(
    _PyInterpreterFrame *frame, _PyIRHomeView *view);
_PyIRHomeAuthority _PyFrame_GetCopyViewUnderNullControl(
    _PyInterpreterFrame *frame, _PyIRHomeView *view);
void _PyFrame_GetNullControlHomeView(
    _PyInterpreterFrame *frame, int remaining, _PyIRHomeView *view);
PyAPI_FUNC(void) _PyFrame_CopyCreatedGenerator(
    _PyInterpreterFrame *src, _PyInterpreterFrame *dest);

static inline void
_PyFrame_CopyHeader(_PyInterpreterFrame *src, _PyInterpreterFrame *dest)
{
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
#ifdef Py_GIL_DISABLED
    dest->tlbc_index = src->tlbc_index;
#endif
    dest->return_slot = src->return_slot;
    dest->return_pc = src->return_pc;
    dest->pending_kwargs = src->pending_kwargs;
    dest->pending_redirect = src->pending_redirect;
    /* Copy/takeover has a different lifecycle until a supported producer
     * publishes its exact site/phase. */
    dest->phase_state = 0;
    src->phase_state = 0;
    // visited is GC bookkeeping for the current stack walk, not frame state.
    dest->visited = 0;
#ifdef Py_DEBUG
    dest->lltrace = src->lltrace;
#endif
}

static inline void
_PyFrame_Copy(_PyInterpreterFrame *src, _PyInterpreterFrame *dest,
              const _PyIRHomeView *view, _PyIRHomeAuthority authority)
{
    int nslots = _PyFrame_NumSlots(src);
    assert(authority == _PyIR_HOME_MUST_READABLE ||
           authority == _PyIR_HOME_NULL_CONTROL_READABLE);
    assert(view->nbindings == _PyFrame_GetCode(src)->co_nlocalsplus);
    assert(view->nbindings + view->scratch_limit <= nslots);
    _PyFrame_CopyHeader(src, dest);
    uint32_t cursor = 0;
    int filled = 0, begin, end;
    while (_PyFrame_NextHomeRange(view, &cursor, &begin, &end)) {
        assert(begin >= filled && end <= nslots);
        while (filled < begin) {
            dest->base[filled++] = PyValue_NULL;
        }
        while (filled < end) {
            // A move: the slots src leaves behind may be the caller's.
            dest->base[filled] = src->base[filled];
            src->base[filled++] = PyValue_NULL;
        }
    }
    /* NULL shadow still initializes inactive destinations without loading the
     * corresponding source words. M4 removes this fill with full authority. */
    while (filled < nslots) {
        dest->base[filled++] = PyValue_NULL;
    }
}


/* Write the constants of the constant slots of code (CO_FAST_CONST) into the
 * slots of frame: owners of inline scalars and immortals, which own no
 * count.  A mortal constant, as after code.replace(co_consts=...), gets
 * one. */
static inline void
_PyFrame_FillConstSlots(_PyInterpreterFrame *frame, PyCodeObject *code)
{
    const uint32_t *table = _PyCode_CONSTSLOTS(code);
    for (int i = 0; i < code->co_nconstslots; i++) {
        frame->base[table[2 * i]] =
            PyValue_XDUP(PyTuple_GET_VALUE(code->co_consts, table[2 * i + 1]));
    }
}

/* Consumes func and locals. The caller supplies initialized value storage. */
static inline Py_ALWAYS_INLINE void
_PyFrame_Initialize(
    PyThreadState *tstate, _PyInterpreterFrame *frame, PyValue func,
    PyObject *locals, PyCodeObject *code, PyValue *base,
    _PyInterpreterFrame *previous)
{
    frame->previous = previous;
    frame->base = base;
    PyFunctionObject *func_obj = _PyValue_FunctionOwnerSteal(func);
    frame->f_funcobj = func_obj;
    frame->f_executable = (PyCodeObject *)Py_NewRef(code);
    frame->f_builtins = func_obj->func_builtins;
    frame->f_globals = func_obj->func_globals;
    frame->f_locals = locals;
    frame->frame_obj = NULL;
    (void)tstate;
    _PyFrame_InitIRInstrPtr(frame, code);
    frame->return_pc = frame->instr_ptr;
    frame->return_slot = 0;
    frame->pending_kwargs = IR_NO_DST;
    frame->pending_redirect = 0;
    frame->phase_state = 0;
    frame->owner = FRAME_OWNED_BY_THREAD;
    frame->visited = 0;
#ifdef Py_DEBUG
    frame->lltrace = 0;
#endif

    // The constant slots (CO_FAST_CONST): owners of inline scalars and
    // immortals, which own no count.  A mortal constant, as after
    // code.replace(co_consts=...), gets one.
    if (code->co_nconstslots) {
        _PyFrame_FillConstSlots(frame, code);
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
PyAPI_FUNC(void) _PyValue_CheckFrame(_PyInterpreterFrame *frame);
#endif

/* A refcount test only proves a close callback-free while holding the GIL. */
static inline bool
_PyFrame_ObjectMayReenter(PyObject *object)
{
    if (object == NULL) {
        return false;
    }
#ifdef Py_GIL_DISABLED
    return !_Py_IsImmortal(object);
#else
    return Py_REFCNT(object) == 1;
#endif
}

enum {
    _Py_FRAME_CLEAR_DONE = -1,
    _Py_FRAME_CLEAR_ASSOCIATION = -2,
    _Py_FRAME_CLEAR_LOCALS = -3,
    _Py_FRAME_CLEAR_FUNCTION = -4,
};

struct _PyTransitionRoots;
int _PyFrame_TryClearExceptCode(_PyInterpreterFrame *frame);
void _PyFrame_InitClearRoots(_PyInterpreterFrame *frame, int stage, PyValue result,
                            struct _PyTransitionRoots *roots);
void _PyFrame_FinishClearExceptCode(_PyInterpreterFrame *frame, int stage,
                                  struct _PyTransitionRoots *roots);

/* Retire frame and exception owners while preserving a detached result.
 * Code remains owned by the generator. Close/deallocation passes NULL. */
PyAPI_FUNC(PyValue) _PyFrame_ClearGenerator(PyGenObject *gen, PyValue result,
                                          uint8_t reason);

int
_PyFrame_Traverse(_PyInterpreterFrame *frame, visitproc visit, void *arg);

bool
_PyFrame_HasHiddenLocals(_PyInterpreterFrame *frame);

PyObject *
_PyFrame_GetLocals(_PyInterpreterFrame *frame);

/* The data stack is untyped memory in pointer-sized units, which the public
 * PyThreadState and _PyStackChunk declare as PyObject *: frames take slots
 * (words) from its bottom and headers from its top.  These convert between
 * its addresses and slot addresses; they are the only casts between a
 * PyValue * and another pointer type outside the value headers
 * (Tools/build/check_value_api.py, raw-bits). */
static inline PyValue *
_PyDataStack_Slots(PyObject **address)
{
    return (PyValue *)address;
}

static inline PyObject **
_PyDataStack_Address(PyValue *slots)
{
    return (PyObject **)slots;
}

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

PyAPI_FUNC(void) _PyStackChunk_ExtendInitialized(
    _PyStackChunk *chunk, PyObject **value_end);

/* Reserve initialized cells before any header write. Old headers are not
 * values: crossing their invalidated storage fills only the uncertain tail.
 * No owner can be retired or callback invoked inside this commit. */
static inline void
_PyStackChunk_EnsureFrameStorage(_PyStackChunk *chunk, PyObject **header,
                                PyObject **value_end)
{
    assert(&chunk->data[0] <= chunk->initialized);
    assert(&chunk->data[0] <= value_end && value_end < header);
    if (chunk->initialized > header) {
        chunk->initialized = header;
    }
    if (chunk->initialized < value_end) {
        _PyStackChunk_ExtendInitialized(chunk, value_end);
    }
}

/* Push a frame header and nslots slots, the header onto the frame stack and
 * the slots onto the value stack.  Must be guarded by
 * _PyThreadState_HasStackSpace(), for nslots + FRAME_SPECIALS_SIZE. */
static inline _PyInterpreterFrame *
_PyThreadState_PushFrameUnchecked(PyThreadState *tstate, int nslots)
{
    PyObject **header = tstate->datastack_limit - FRAME_SPECIALS_SIZE;
    PyObject **end = tstate->datastack_top + nslots;
    _PyStackChunk_EnsureFrameStorage(tstate->datastack_chunk, header, end);
    _PyInterpreterFrame *frame =
        (_PyInterpreterFrame *)header;
    frame->base = _PyDataStack_Slots(tstate->datastack_top);
    frame->saved_top = frame->base;
    tstate->datastack_limit = (PyObject **)frame;
    tstate->datastack_top = end;
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
#ifndef NDEBUG
    /* The code owner may already be cleared. Check the physical departing
     * suffix, including an overlapping caller's previously NULL tail. */
    for (PyValue *p = frame->base;
         p < _PyDataStack_Slots(tstate->datastack_top); p++)
    {
        assert(PyValue_IsNull(*p));
    }
#endif
    // Only the frame that a chunk was allocated for has its first slot.  The
    // root chunk stays, for the next frame.
    if (frame->base == _PyDataStack_Slots(&chunk->data[0])
        && chunk->previous != NULL)
    {
        _PyThreadState_PopChunk(tstate, frame);
        return;
    }
    tstate->datastack_limit = (PyObject **)frame + FRAME_SPECIALS_SIZE;
    tstate->datastack_top = _PyDataStack_Address(frame->saved_top);
    assert(tstate->datastack_top < tstate->datastack_limit);
}

/* Pushes a frame without checking for space.
 * Must be guarded by _PyThreadState_HasStackSpace()
 * Consumes reference to func. */
static inline _PyInterpreterFrame *
_PyFrame_PushUnchecked(PyThreadState *tstate, PyValue func, _PyInterpreterFrame *previous)
{
    CALL_STAT_INC(frames_pushed);
    PyFunctionObject *func_obj = (PyFunctionObject *)PyValue_AsKnownObjectBorrow(func);
    PyCodeObject *code = (PyCodeObject *)func_obj->func_code;
    _PyInterpreterFrame *new_frame = _PyThreadState_PushFrameUnchecked(
        tstate, _PyFrame_NumSlotsForCodeObject(code));
    _PyFrame_Initialize(tstate, new_frame, func, NULL, code,
                        new_frame->base, previous);
    return new_frame;
}

/* Reserve a call's final homes without constructing its header. A thread
 * caller transfers its argument homes in place; a generator caller uses the
 * current thread-stack top, separate from its heap-owned argument window.
 * Must be guarded by _PyThreadState_HasStackSpace(), for co_framesize. */
static inline Py_ALWAYS_INLINE _PyInterpreterFrame *
_PyFrame_ReserveForCall(PyThreadState *tstate, PyCodeObject *code,
                        PyValue *base, int nargs,
                        _PyInterpreterFrame *previous)
{
    CALL_STAT_INC(frames_pushed);
    int nslots = _PyFrame_NumSlotsForCodeObject(code);
#ifndef NDEBUG
    assert(nargs <= nslots);
    if (previous->owner == FRAME_OWNED_BY_THREAD) {
        assert(base > previous->base &&
               base <= _PyDataStack_Slots(tstate->datastack_top));
        PyValue *previous_end = previous->base +
            _PyFrame_NumSlotsForCodeObject(_PyFrame_GetCode(previous));
        assert(base + nargs <= previous_end);
        Py_ssize_t known = previous_end - base;
        int shared = known < nslots ? (int)known : nslots;
        assert(shared >= nargs);
        for (int i = nargs; i < shared; i++) {
            assert(PyValue_IsNull(base[i]));
        }
    }
    else {
        assert(base == _PyDataStack_Slots(tstate->datastack_top));
    }
#endif
    PyValue *saved_top = _PyDataStack_Slots(tstate->datastack_top);
    PyValue *end = base + nslots;
    PyValue *physical_end = end > saved_top ? end : saved_top;
    _PyInterpreterFrame *frame =
        (_PyInterpreterFrame *)(tstate->datastack_limit - FRAME_SPECIALS_SIZE);
    _PyStackChunk_EnsureFrameStorage(tstate->datastack_chunk,
        (PyObject **)frame, _PyDataStack_Address(physical_end));
    frame->saved_top = saved_top;
    tstate->datastack_limit = (PyObject **)frame;
    if (end > saved_top) {
        tstate->datastack_top = _PyDataStack_Address(end);
    }
    assert(tstate->datastack_top < tstate->datastack_limit);
    return frame;
}

/* Pushes a frame for the function func whose slots start at base, among the
 * slots of previous, the current frame, where its nargs arguments already
 * are: the frame overlaps the window of the call. Argument owners transfer
 * to child homes in the same cells; the parent no longer owns those aliases.
 * The remaining caller homes from base + nargs to its slot end are NULL,
 * and the callee leaves the overlap NULL on retirement. Must be guarded by
 * _PyThreadState_HasStackSpace(), for co_framesize. Consumes func. */
static inline Py_ALWAYS_INLINE _PyInterpreterFrame *
_PyFrame_PushOverlapping(PyThreadState *tstate, PyValue func,
                         PyValue *base, int nargs,
                         _PyInterpreterFrame *previous)
{
    PyFunctionObject *func_obj = (PyFunctionObject *)PyValue_AsKnownObjectBorrow(func);
    PyCodeObject *code = (PyCodeObject *)func_obj->func_code;
    assert(previous->owner == FRAME_OWNED_BY_THREAD);
    _PyInterpreterFrame *frame = _PyFrame_ReserveForCall(
        tstate, code, base, nargs, previous);
    _PyFrame_Initialize(tstate, frame, func, NULL, code, base, previous);
    return frame;
}

static inline void
_PyFrame_InitializeTrampoline(_PyInterpreterFrame *frame, PyCodeObject *code,
                             int return_slot, _PyInterpreterFrame *previous)
{
    frame->previous = previous;
    frame->f_funcobj = NULL;
    frame->f_executable = (PyCodeObject *)Py_NewRef(code);
    frame->f_builtins = NULL;
    frame->f_globals = NULL;
    frame->f_locals = NULL;
    frame->frame_obj = NULL;
    _PyFrame_InitIRInstrPtr(frame, code);
    frame->owner = FRAME_OWNED_BY_THREAD;
    frame->visited = 0;
#ifdef Py_DEBUG
    frame->lltrace = 0;
#endif
    assert(return_slot < _PyFrame_NumSlotsForCodeObject(code));
    frame->return_slot = (uint16_t)return_slot;
    frame->return_pc = frame->instr_ptr;
    frame->pending_kwargs = IR_NO_DST;
    frame->pending_redirect = 0;
    frame->phase_state = 0;
}

/* Pushes a trampoline frame without checking for space.
 * Must be guarded by _PyThreadState_HasStackSpace() */
static inline _PyInterpreterFrame *
_PyFrame_PushTrampolineUnchecked(PyThreadState *tstate, PyCodeObject *code,
                               int return_slot, _PyInterpreterFrame *previous)
{
    CALL_STAT_INC(frames_pushed);
    _PyInterpreterFrame *frame = _PyThreadState_PushFrameUnchecked(
        tstate, _PyFrame_NumSlotsForCodeObject(code));
    _PyFrame_InitializeTrampoline(frame, code, return_slot, previous);
    return frame;
}

PyAPI_FUNC(_PyInterpreterFrame *)
_PyEvalFramePushAndInit(PyThreadState *tstate, PyValue func,
                        PyObject *locals, PyValue *args,
                        size_t argcount, PyObject *kwnames,
                        _PyInterpreterFrame *previous);

PyAPI_FUNC(_PyInterpreterFrame *)
_PyEvalFramePushAndInit_Ex(PyThreadState *tstate, PyValue func,
    PyObject *locals, Py_ssize_t nargs, PyObject *callargs, PyObject *kwargs, _PyInterpreterFrame *previous);

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_INTERP_FRAME_H
