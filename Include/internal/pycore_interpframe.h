#ifndef Py_INTERNAL_INTERP_FRAME_H
#define Py_INTERNAL_INTERP_FRAME_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_code.h"          // _PyCode_CODE()
#include "pycore_interpframe_structs.h" // _PyInterpreterFrame
#ifdef Py_IR_INTERP
#  include "pycore_ir.h"          // _PyIRBlock
#endif
#include "pycore_pyatomic_ft_wrappers.h" // FT_ATOMIC_LOAD_PTR_ACQUIRE()
#include "pycore_stackref.h"      // PyStackRef_AsPyObjectBorrow()
#include "pycore_stats.h"         // CALL_STAT_INC()

#ifdef __cplusplus
extern "C" {
#endif

#ifdef Py_IR_INTERP
#define _PyInterpreterFrame_LASTI(IF) \
    _PyFrame_InstrOffset((IF), (IF)->instr_ptr)
#else
#define _PyInterpreterFrame_LASTI(IF) \
    ((int)((IF)->instr_ptr - _PyFrame_GetBytecode((IF))))
#endif

static inline PyCodeObject *_PyFrame_GetCode(_PyInterpreterFrame *f) {
    assert(!PyStackRef_IsNull(f->f_executable));
    PyObject *executable = PyStackRef_AsPyObjectBorrow(f->f_executable);
    assert(PyCode_Check(executable));
    return (PyCodeObject *)executable;
}

#ifdef Py_IR_INTERP
/* The IR block that frame->instr_ptr points into.  A frame that has not
 * started yet may still be at the stub of a code object that another frame
 * has lowered since. */
static inline int
_PyFrame_IsAtStub(const _PyInterpreterFrame *f)
{
    return f->instr_ptr == _PyIR_LowerStub.words
           || f->instr_ptr == _PyIR_STUB_RESUMED;
}

static inline _PyIRBlock *
_PyFrame_IRBlock(_PyInterpreterFrame *f)
{
    if (_PyFrame_IsAtStub(f)) {
        return &_PyIR_LowerStub;
    }
    return _PyFrame_GetCode(f)->_co_ir;
}

static inline _PyIRWord *
_PyFrame_IRWords(_PyInterpreterFrame *f)
{
    return _PyFrame_IRBlock(f)->words;
}

/* The bytecode offset of the instruction at instr, which is f_lasti for
 * frame->instr_ptr.  The word before the block, which the unwinding of a
 * frame that has not finished an instruction reads, is at offset -1, as the
 * code unit before the bytecode is. */
static inline int
_PyFrame_InstrOffset(_PyInterpreterFrame *f, const _PyIRWord *instr)
{
    if (instr == _PyIR_STUB_RESUMED) {
        return _PyFrame_GetCode(f)->_co_firsttraceable + 1;
    }
    _PyIRBlock *block = _PyFrame_IRBlock(f);
    if (instr == block->words - 1) {
        return -1;
    }
    return _PyIR_Anchor(block, instr);
}

/* The first word of the instruction that starts at a bytecode offset. */
static inline _PyIRWord *
_PyFrame_InstrAt(_PyInterpreterFrame *f, int offset)
{
    return _PyIR_InstrAt(_PyFrame_IRBlock(f), offset);
}

static inline void
_PyFrame_InitIRInstrPtr(_PyInterpreterFrame *frame, PyCodeObject *code)
{
    _PyIRBlock *block = (_PyIRBlock *)FT_ATOMIC_LOAD_PTR_ACQUIRE(code->_co_ir);
    frame->instr_ptr = block->words;
#ifdef Py_GIL_DISABLED
    frame->tlbc_index = 0;
#endif
}
#endif

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

    if (PyStackRef_IsNull(f->f_executable)) {
        return NULL;
    }
    void *ptr;
    memcpy(&ptr, &f->f_executable, sizeof(f->f_executable));
    if (_PyMem_IsPtrFreed(ptr)) {
        return NULL;
    }
    PyObject *executable = PyStackRef_AsPyObjectBorrow(f->f_executable);
    if (_PyObject_IsFreed(executable)) {
        return NULL;
    }
    if (!PyCode_Check(executable)) {
        return NULL;
    }
    return (PyCodeObject *)executable;
}

#ifndef Py_IR_INTERP
/* The bytecode that frame->instr_ptr points into.  The IR interpreter has
 * none: frames point into IR blocks, see _PyFrame_IRBlock(). */
static inline _Py_CODEUNIT *
_PyFrame_GetBytecode(_PyInterpreterFrame *f)
{
#ifdef Py_GIL_DISABLED
    PyCodeObject *co = _PyFrame_GetCode(f);
    _PyCodeArray *tlbc = _PyCode_GetTLBCArray(co);
    assert(f->tlbc_index >= 0 && f->tlbc_index < tlbc->size);
    return (_Py_CODEUNIT *)tlbc->entries[f->tlbc_index];
#else
    return _PyCode_CODE(_PyFrame_GetCode(f));
#endif
}
#endif

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

#ifdef Py_IR_INTERP
    if (f->instr_ptr == _PyIR_LowerStub.words) {
        return 0;
    }
    if (f->instr_ptr == _PyIR_STUB_RESUMED) {
        return (co->_co_firsttraceable + 1) * (int)sizeof(_Py_CODEUNIT);
    }
    _PyIRBlock *block = co->_co_ir;
    if (block == NULL || f->instr_ptr < block->words
        || f->instr_ptr >= block->words + block->nwords) {
        return -1;
    }
    return _PyIR_Anchor(block, f->instr_ptr) * (int)sizeof(_Py_CODEUNIT);
#else
    _Py_CODEUNIT *bytecode;
#ifdef Py_GIL_DISABLED
    _PyCodeArray *tlbc = _PyCode_GetTLBCArray(co);
    assert(f->tlbc_index >= 0 && f->tlbc_index < tlbc->size);
    bytecode = (_Py_CODEUNIT *)tlbc->entries[f->tlbc_index];
#else
    bytecode = _PyCode_CODE(co);
#endif

    return (int)(f->instr_ptr - bytecode) * sizeof(_Py_CODEUNIT);
#endif
}

static inline PyFunctionObject *_PyFrame_GetFunction(_PyInterpreterFrame *f) {
    PyObject *func = PyStackRef_AsPyObjectBorrow(f->f_funcobj);
    assert(PyFunction_Check(func));
    return (PyFunctionObject *)func;
}

static inline _PyStackRef *_PyFrame_Stackbase(_PyInterpreterFrame *f) {
    return (f->localsplus + _PyFrame_GetCode(f)->co_nlocalsplus);
}

static inline _PyStackRef _PyFrame_StackPeek(_PyInterpreterFrame *f, int depth) {
    assert(f->stackpointer > _PyFrame_Stackbase(f));
    assert(!PyStackRef_IsNull(f->stackpointer[-depth]));
    return f->stackpointer[-depth];
}

static inline _PyStackRef _PyFrame_StackPop(_PyInterpreterFrame *f) {
    assert(f->stackpointer > _PyFrame_Stackbase(f));
    f->stackpointer--;
    return *f->stackpointer;
}

static inline void _PyFrame_StackPush(_PyInterpreterFrame *f, _PyStackRef value) {
    *f->stackpointer = value;
    f->stackpointer++;
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

static inline void _PyFrame_Copy(_PyInterpreterFrame *src, _PyInterpreterFrame *dest)
{
    dest->f_executable = PyStackRef_MakeHeapSafe(src->f_executable);
    // Don't leave a dangling pointer to the old frame when creating generators
    // and coroutines:
    dest->previous = NULL;
    dest->f_funcobj = PyStackRef_MakeHeapSafe(src->f_funcobj);
    dest->f_globals = src->f_globals;
    dest->f_builtins = src->f_builtins;
    dest->f_locals = src->f_locals;
    dest->frame_obj = src->frame_obj;
    dest->instr_ptr = src->instr_ptr;
#ifdef Py_GIL_DISABLED
    dest->tlbc_index = src->tlbc_index;
#endif
    assert(src->stackpointer != NULL);
    int stacktop = (int)(src->stackpointer - src->localsplus);
    assert(stacktop >= 0);
    dest->stackpointer = dest->localsplus + stacktop;
    // visited is GC bookkeeping for the current stack walk, not frame state.
    dest->visited = 0;
#ifdef Py_DEBUG
    dest->stackpointer_valid =  src->stackpointer_valid;
    dest->lltrace = src->lltrace;
#endif
    for (int i = 0; i < stacktop; i++) {
        dest->localsplus[i] = PyStackRef_MakeHeapSafe(src->localsplus[i]);
    }
}

#if defined(Py_GIL_DISABLED) && !defined(Py_IR_INTERP)
static inline void
_PyFrame_InitializeTLBC(PyThreadState *tstate, _PyInterpreterFrame *frame,
                        PyCodeObject *code)
{
    _Py_CODEUNIT *tlbc = _PyCode_GetTLBCFast(tstate, code);
    if (tlbc == NULL) {
        // No thread-local bytecode exists for this thread yet; use the main
        // thread's copy, deferring thread-local bytecode creation to the
        // execution of RESUME.
        frame->instr_ptr = _PyCode_CODE(code);
        frame->tlbc_index = 0;
    }
    else {
        frame->instr_ptr = tlbc;
        frame->tlbc_index = ((_PyThreadStateImpl *)tstate)->tlbc_index;
    }
}
#endif

/* Consumes reference to func and locals.
   Does not initialize frame->previous, which happens
   when frame is linked into the frame stack.
 */
static inline void
_PyFrame_Initialize(
    PyThreadState *tstate, _PyInterpreterFrame *frame, _PyStackRef func,
    PyObject *locals, PyCodeObject *code, int null_locals_from, _PyInterpreterFrame *previous)
{
    frame->previous = previous;
    frame->f_funcobj = func;
    frame->f_executable = PyStackRef_FromPyObjectNew(code);
    PyFunctionObject *func_obj = (PyFunctionObject *)PyStackRef_AsPyObjectBorrow(func);
    frame->f_builtins = func_obj->func_builtins;
    frame->f_globals = func_obj->func_globals;
    frame->f_locals = locals;
    frame->stackpointer = frame->localsplus + code->co_nlocalsplus;
    frame->frame_obj = NULL;
#ifdef Py_IR_INTERP
    (void)tstate;
    _PyFrame_InitIRInstrPtr(frame, code);
#elif defined(Py_GIL_DISABLED)
    _PyFrame_InitializeTLBC(tstate, frame, code);
#else
    (void)tstate;
    frame->instr_ptr = _PyCode_CODE(code);
#endif
    frame->return_offset = 0;
    frame->owner = FRAME_OWNED_BY_THREAD;
    frame->visited = 0;
#ifdef Py_DEBUG
    frame->stackpointer_valid = 1;
    frame->lltrace = 0;
#endif

    for (int i = null_locals_from; i < code->co_nlocalsplus; i++) {
        frame->localsplus[i] = PyStackRef_NULL;
    }
}

/* Gets the pointer to the locals array
 * that precedes this frame.
 */
static inline _PyStackRef*
_PyFrame_GetLocalsArray(_PyInterpreterFrame *frame)
{
    return frame->localsplus;
}

// Fetches the stack pointer, and (on debug builds) sets stackpointer to NULL.
// Having stackpointer == NULL makes it easier to catch missing stack pointer
// spills/restores (which could expose invalid values to the GC) using asserts.
static inline _PyStackRef*
_PyFrame_GetStackPointer(_PyInterpreterFrame *frame)
{
    return frame->stackpointer;
}

static inline void
_PyFrame_SetStackPointer(_PyInterpreterFrame *frame, _PyStackRef *stack_pointer)
{
    frame->stackpointer = stack_pointer;
}

static inline void
_PyFrame_StackPointerValidate(_PyInterpreterFrame *frame)
{
#ifdef Py_DEBUG
    assert(frame->stackpointer_valid == 0);
    frame->stackpointer_valid = 1;
#endif
}

static inline void
_PyFrame_StackPointerInvalidate(_PyInterpreterFrame *frame)
{
#ifdef Py_DEBUG
    assert(frame->stackpointer_valid == 1);
    frame->stackpointer_valid = 0;
#endif
}

static inline void
_PyFrame_StackAssertInvalid(_PyInterpreterFrame *frame)
{
#ifdef Py_DEBUG
    assert(frame->stackpointer_valid == 0);
#endif
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
#ifdef Py_IR_INTERP
    return frame->owner != FRAME_OWNED_BY_GENERATOR &&
           _PyFrame_InstrOffset(frame, frame->instr_ptr) <
               _PyFrame_GetCode(frame)->_co_firsttraceable;
#else
    return frame->owner != FRAME_OWNED_BY_GENERATOR &&
           frame->instr_ptr < _PyFrame_GetBytecode(frame) +
                                  _PyFrame_GetCode(frame)->_co_firsttraceable;
#endif
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
// Abort if a live slot of the frame holds a malformed stackref.
// Exported for lltrace in _testinternalcapi, which includes ceval.h.
PyAPI_FUNC(void) _PyStackRef_CheckFrame(_PyInterpreterFrame *frame);
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

// Exported for external JIT support
PyAPI_FUNC(_PyInterpreterFrame *)
_PyThreadState_PushFrame(PyThreadState *tstate, size_t size);

PyAPI_FUNC(void) _PyThreadState_PopFrame(PyThreadState *tstate, _PyInterpreterFrame *frame);

/* Pushes a frame without checking for space.
 * Must be guarded by _PyThreadState_HasStackSpace()
 * Consumes reference to func. */
static inline _PyInterpreterFrame *
_PyFrame_PushUnchecked(PyThreadState *tstate, _PyStackRef func, int null_locals_from, _PyInterpreterFrame * previous)
{
    CALL_STAT_INC(frames_pushed);
    PyFunctionObject *func_obj = (PyFunctionObject *)PyStackRef_AsPyObjectBorrow(func);
    PyCodeObject *code = (PyCodeObject *)func_obj->func_code;
    _PyInterpreterFrame *new_frame = (_PyInterpreterFrame *)tstate->datastack_top;
    tstate->datastack_top += code->co_framesize;
    assert(tstate->datastack_top < tstate->datastack_limit);
    _PyFrame_Initialize(tstate, new_frame, func, NULL, code, null_locals_from,
                        previous);
    return new_frame;
}

/* Pushes a trampoline frame without checking for space.
 * Must be guarded by _PyThreadState_HasStackSpace() */
static inline _PyInterpreterFrame *
_PyFrame_PushTrampolineUnchecked(PyThreadState *tstate, PyCodeObject *code, int stackdepth, _PyInterpreterFrame * previous)
{
    CALL_STAT_INC(frames_pushed);
    _PyInterpreterFrame *frame = (_PyInterpreterFrame *)tstate->datastack_top;
    tstate->datastack_top += code->co_framesize;
    assert(tstate->datastack_top < tstate->datastack_limit);
    frame->previous = previous;
    frame->f_funcobj = PyStackRef_None;
    frame->f_executable = PyStackRef_FromPyObjectNew(code);
#ifdef Py_DEBUG
    frame->f_builtins = NULL;
    frame->f_globals = NULL;
#endif
    frame->f_locals = NULL;
    assert(stackdepth <= code->co_stacksize);
    frame->stackpointer = frame->localsplus + code->co_nlocalsplus + stackdepth;
    frame->frame_obj = NULL;
#ifdef Py_IR_INTERP
    _PyFrame_InitIRInstrPtr(frame, code);
#elif defined(Py_GIL_DISABLED)
    _PyFrame_InitializeTLBC(tstate, frame, code);
#else
    frame->instr_ptr = _PyCode_CODE(code);
#endif
    frame->owner = FRAME_OWNED_BY_THREAD;
    frame->visited = 0;
#ifdef Py_DEBUG
    frame->stackpointer_valid = 1;
    frame->lltrace = 0;
#endif
    frame->return_offset = 0;
    return frame;
}

PyAPI_FUNC(_PyInterpreterFrame *)
_PyEvalFramePushAndInit(PyThreadState *tstate, _PyStackRef func,
                        PyObject *locals, _PyStackRef const *args,
                        size_t argcount, PyObject *kwnames,
                        _PyInterpreterFrame *previous);

PyAPI_FUNC(_PyInterpreterFrame *)
_PyEvalFramePushAndInit_Ex(PyThreadState *tstate, _PyStackRef func,
    PyObject *locals, Py_ssize_t nargs, PyObject *callargs, PyObject *kwargs, _PyInterpreterFrame *previous);

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_INTERP_FRAME_H
