#ifndef Py_INTERNAL_CEVAL_H
#define Py_INTERNAL_CEVAL_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "dynamic_annotations.h"  // _Py_ANNOTATE_RWLOCK_CREATE

#include "pycore_borrow.h"        // _PyBorrowScope_Exit()
#include "pycore_code.h"          // _PyCode_GetTLBCFast()
#include "pycore_interp.h"        // PyInterpreterState.eval_frame
#include "pycore_ir_observer.h"   // Scoped callback ownership phase
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_stats.h"         // EVAL_CALL_STAT_INC()
#include "pycore_typedefs.h"      // _PyInterpreterFrame
#include "pycore_value.h"         // _PyValueTemps


/* Forward declarations */
struct _ceval_runtime_state;

/* Install a trace or profile function. func and arg are the extension's
 * (PyEval_SetTrace()): the thread's value function is an adapter that boxes
 * the argument word and calls func. The Value forms install vfunc, which
 * reads the word as it is; ofunc is the object form that the public
 * tstate->c_tracefunc/c_profilefunc show to readers (NULL with vfunc NULL). */
// Export for '_lsprof' shared extension
PyAPI_FUNC(int) _PyEval_SetProfile(PyThreadState *tstate, Py_tracefunc func, PyObject *arg);
extern int _PyEval_SetProfileAllThreads(PyInterpreterState *interp, Py_tracefunc func, PyObject *arg);
extern int _PyEval_SetProfileValue(PyThreadState *tstate, _Py_valuetracefunc vfunc, Py_tracefunc ofunc, PyObject *arg);
extern int _PyEval_SetProfileValueAllThreads(PyInterpreterState *interp, _Py_valuetracefunc vfunc, Py_tracefunc ofunc, PyObject *arg);

extern int _PyEval_SetTrace(PyThreadState *tstate, Py_tracefunc func, PyObject *arg);
extern int _PyEval_SetTraceAllThreads(PyInterpreterState *interp, Py_tracefunc func, PyObject *arg);
extern int _PyEval_SetTraceValue(PyThreadState *tstate, _Py_valuetracefunc vfunc, Py_tracefunc ofunc, PyObject *arg);
extern int _PyEval_SetTraceValueAllThreads(PyInterpreterState *interp, _Py_valuetracefunc vfunc, Py_tracefunc ofunc, PyObject *arg);

extern int _PyEval_SetOpcodeTrace(PyFrameObject *f, bool enable);

/* Semantic exception recovery, independent of source attribution. Four ints
 * are returned in rax:rdx on x86-64 SysV without addressable result storage. */
typedef struct {
    int handled;
    int exc_slot;
    int handler;
    int lasti;
} _PyExceptionHandler;

/* Retain a failed source event's handler recipe across cleanup reentry. */
extern void _PyEval_RecordSourceError(PyThreadState *tstate,
                                    _PyInterpreterFrame *frame,
                                    _PyExceptionHandler handler);

// Helper to look up a builtin object
// Export for 'array' shared extension
PyAPI_FUNC(PyObject*) _PyEval_GetBuiltin(PyObject *);

extern void _PyEval_SetSwitchInterval(unsigned long microseconds);
extern unsigned long _PyEval_GetSwitchInterval(void);

// Export for '_queue' shared extension
PyAPI_FUNC(int) _PyEval_MakePendingCalls(PyThreadState *);

#ifndef Py_DEFAULT_RECURSION_LIMIT
#  define Py_DEFAULT_RECURSION_LIMIT 1000
#endif

extern void _Py_FinishPendingCalls(PyThreadState *tstate);
extern void _PyEval_InitState(PyInterpreterState *);
extern void _PyEval_SignalReceived(void);

// bitwise flags:
#define _Py_PENDING_MAINTHREADONLY 1
#define _Py_PENDING_RAWFREE 2

typedef int _Py_add_pending_call_result;
#define _Py_ADD_PENDING_SUCCESS 0
#define _Py_ADD_PENDING_FULL -1

// Export for '_testinternalcapi' shared extension
PyAPI_FUNC(_Py_add_pending_call_result) _PyEval_AddPendingCall(
    PyInterpreterState *interp,
    _Py_pending_call_func func,
    void *arg,
    int flags);

#ifdef HAVE_FORK
extern PyStatus _PyEval_ReInitThreads(PyThreadState *tstate);
#endif

// Used by sys.call_tracing()
extern PyObject* _PyEval_CallTracing(PyObject *func, PyObject *args);

// Used by sys.get_asyncgen_hooks()
extern PyObject* _PyEval_GetAsyncGenFirstiter(void);
extern PyObject* _PyEval_GetAsyncGenFinalizer(void);

// Used by sys.set_asyncgen_hooks()
extern int _PyEval_SetAsyncGenFirstiter(PyObject *);
extern int _PyEval_SetAsyncGenFinalizer(PyObject *);

// Used by sys.get_coroutine_origin_tracking_depth()
// and sys.set_coroutine_origin_tracking_depth()
extern int _PyEval_GetCoroutineOriginTrackingDepth(void);
extern int _PyEval_SetCoroutineOriginTrackingDepth(int depth);

extern void _PyEval_Fini(void);


extern PyObject* _PyEval_GetBuiltins(PyThreadState *tstate);

/* Success consumes the window and returns an unpublished child. On failure,
 * preparation may leave it intact; ERROR_NO_POP unwinds its actual NULL homes. */
extern _PyInterpreterFrame *_PyEval_CreateInitFrame(
    PyThreadState *, _PyInterpreterFrame *, PyValue *, int, int, _PyInstr *);

// Trampoline API

typedef struct {
    // Callback to initialize the trampoline state
    void* (*init_state)(void);
    // Callback to register every trampoline being created
    void (*write_state)(void* state, const void *code_addr,
                        size_t code_size, PyCodeObject* code);
    // Callback to free the trampoline state
    int (*free_state)(void* state);
} _PyPerf_Callbacks;

extern int _PyPerfTrampoline_SetCallbacks(_PyPerf_Callbacks *);
extern void _PyPerfTrampoline_GetCallbacks(_PyPerf_Callbacks *);
extern int _PyPerfTrampoline_Init(int activate);
extern int _PyPerfTrampoline_Fini(void);
extern int _PyIsPerfTrampolineActive(void);
extern PyStatus _PyPerfTrampoline_AfterFork_Child(void);
#ifdef PY_HAVE_PERF_TRAMPOLINE
extern _PyPerf_Callbacks _Py_perfmap_callbacks;
extern _PyPerf_Callbacks _Py_perfmap_jit_callbacks;
#endif

static inline PyObject*
_PyEval_EvalFrame(PyThreadState *tstate, _PyInterpreterFrame *frame, int throwflag)
{
    EVAL_CALL_STAT_INC(EVAL_CALL_TOTAL);
    if (tstate->interp->eval_frame == NULL) {
        /* Sets the activation's borrow floor. */
        return _PyEval_EvalFrameDefault(tstate, frame, throwflag);
    }
    /* A PEP 523 hook is native code: give it a borrow scope.  The default
       evaluator it calls sets its own floor above the hook's entries. */
    _PyBorrowMark mark = _PyBorrowScope_Enter(tstate);
    PyObject *result = tstate->interp->eval_frame(tstate, frame, throwflag);
    _PyBorrowScope_Exit(tstate, mark);
    return result;
}

/* Run frame and return its result as a value, PyValue_NULL with an exception
   set on error (Python/ceval.c).  INTERPRETER_EXIT hands the value on as it
   is; a frame run by a PEP 523 hook returns an object, which it steals back.
   _PyEval_EvalFrame() and _PyEval_EvalFrameDefault() are for callers that
   need an object: PyEval_EvalFrame(), the hook itself. */
extern PyValue _PyEval_EvalFrameValue(PyThreadState *tstate,
                                      _PyInterpreterFrame *frame,
                                      int throwflag);

PyAPI_FUNC(PyObject *)
_PyEval_Vector(PyThreadState *tstate,
            PyFunctionObject *func, PyObject *locals,
            PyObject* const* args, size_t argcount,
            PyObject *kwnames);

/* Borrow the input window; the outlined provider owns construction through
 * evaluation, including a custom evaluator's callbacks and frame handoff. */
PyAPI_FUNC(PyValue) _PyEval_VectorValue(PyThreadState *tstate,
                                       PyFunctionObject *func,
                                       const PyValue *args, size_t nargsf,
                                       PyObject *kwnames);

/* Strict custom-evaluator handoffs require a live staged token before reading
 * frame storage. Activation follows current-frame publication; retirement
 * consumes an unentered non-current frame. Both leave an empty outer node. */
PyAPI_FUNC(int) _PyEval_ActivateStagedFrame(PyThreadState *tstate,
                                          _PyInterpreterFrame *frame);
PyAPI_FUNC(int) _PyEval_RetireStagedFrame(PyThreadState *tstate,
                                        _PyInterpreterFrame *frame);

extern int _PyEval_ThreadsInitialized(void);
extern void _PyEval_InitGIL(PyThreadState *tstate, int own_gil);
extern void _PyEval_FiniGIL(PyInterpreterState *interp);

extern void _PyEval_AcquireLock(PyThreadState *tstate);

extern void _PyEval_ReleaseLock(PyInterpreterState *, PyThreadState *,
                                int final_release);

#ifdef Py_GIL_DISABLED
// Returns 0 or 1 if the GIL for the given thread's interpreter is disabled or
// enabled, respectively.
//
// The enabled state of the GIL will not change while one or more threads are
// attached.
static inline int
_PyEval_IsGILEnabled(PyThreadState *tstate)
{
    struct _gil_runtime_state *gil = tstate->interp->ceval.gil;
    return _Py_atomic_load_int_relaxed(&gil->enabled) != 0;
}

// Enable or disable the GIL used by the interpreter that owns tstate, which
// must be the current thread. This may affect other interpreters, if the GIL
// is shared. All three functions will be no-ops (and return 0) if the
// interpreter's `enable_gil' config is not _PyConfig_GIL_DEFAULT.
//
// Every call to _PyEval_EnableGILTransient() must be paired with exactly one
// call to either _PyEval_EnableGILPermanent() or
// _PyEval_DisableGIL(). _PyEval_EnableGILPermanent() and _PyEval_DisableGIL()
// must only be called while the GIL is enabled from a call to
// _PyEval_EnableGILTransient().
//
// _PyEval_EnableGILTransient() returns 1 if it enabled the GIL, or 0 if the
// GIL was already enabled, whether transiently or permanently. The caller will
// hold the GIL upon return.
//
// _PyEval_EnableGILPermanent() returns 1 if it permanently enabled the GIL
// (which must already be enabled), or 0 if it was already permanently
// enabled. Once _PyEval_EnableGILPermanent() has been called once, all
// subsequent calls to any of the three functions will be no-ops.
//
// _PyEval_DisableGIL() returns 1 if it disabled the GIL, or 0 if the GIL was
// kept enabled because of another request, whether transient or permanent.
//
// All three functions must be called by an attached thread (this implies that
// if the GIL is enabled, the current thread must hold it).
extern int _PyEval_EnableGILTransient(PyThreadState *tstate);
extern int _PyEval_EnableGILPermanent(PyThreadState *tstate);
extern int _PyEval_DisableGIL(PyThreadState *state);

#endif

extern void _PyEval_DeactivateOpCache(void);


/* --- _Py_EnterRecursiveCall() ----------------------------------------- */

static inline int _Py_ReachedRecursionLimit(PyThreadState *tstate)  {
    uintptr_t here_addr = _Py_get_machine_stack_pointer();
    _PyThreadStateImpl *_tstate = (_PyThreadStateImpl *)tstate;
    // Possible overflow if stack pointer is beyond the soft limit.
    // _Py_CheckRecursiveCall will check for corner cases and
    // report an error if there is an overflow.
#if _Py_STACK_GROWS_DOWN
    return here_addr < _tstate->c_stack_soft_limit;
#else
    return here_addr > _tstate->c_stack_soft_limit;
#endif
}

// Export for '_json' shared extension, used via _Py_EnterRecursiveCall()
// static inline function.
PyAPI_FUNC(int) _Py_CheckRecursiveCall(
    PyThreadState *tstate,
    const char *where);

PyAPI_FUNC(int) _Py_CheckRecursiveCallPy(
    PyThreadState *tstate);

static inline int _Py_EnterRecursiveCallTstate(PyThreadState *tstate,
                                               const char *where) {
    return (_Py_ReachedRecursionLimit(tstate) && _Py_CheckRecursiveCall(tstate, where));
}

static inline int _Py_EnterRecursiveCall(const char *where) {
    PyThreadState *tstate = _PyThreadState_GET();
    return _Py_EnterRecursiveCallTstate(tstate, where);
}

static inline void _Py_LeaveRecursiveCallTstate(PyThreadState *tstate) {
    (void)tstate;
}

PyAPI_FUNC(void) _Py_InitializeRecursionLimits(PyThreadState *tstate);

// Export for test_peg_generator
PyAPI_FUNC(int) _Py_ReachedRecursionLimitWithMargin(
    PyThreadState *tstate,
    int margin_count);

static inline void _Py_LeaveRecursiveCall(void)  {
}

extern _PyInterpreterFrame* _PyEval_GetFrame(void);

extern PyObject * _PyEval_GetGlobalsFromRunningMain(PyThreadState *);
extern int _PyEval_EnsureBuiltins(
    PyThreadState *,
    PyObject *,
    PyObject **p_builtins);
extern int _PyEval_EnsureBuiltinsWithModule(
    PyThreadState *,
    PyObject *,
    PyObject **p_builtins);

PyAPI_FUNC(PyObject *)_Py_MakeCoro(PyFunctionObject *func, PyCodeObject *code);

/* Handle signals, pending calls, GIL drop request
   and asynchronous exception */
PyAPI_FUNC(int) _Py_HandlePending(PyThreadState *tstate);

/* Raise exception set by PyThreadState_SetAsyncExc, if any */
PyAPI_FUNC(int) _PyEval_RaiseAsyncExc(PyThreadState *tstate);

extern PyObject * _PyEval_GetFrameLocals(void);


typedef struct _special_method {
    PyObject *name;
    const char *error;
    const char *error_suggestion;  // improved optional suggestion
} _Py_SpecialMethod;

PyAPI_DATA(const _Py_SpecialMethod) _Py_SpecialMethods[];
PyAPI_DATA(const size_t) _Py_FunctionAttributeOffsets[];

PyAPI_FUNC(int) _PyEval_CheckExceptStarTypeValid(PyThreadState *tstate, PyObject* right);
PyAPI_FUNC(int) _PyEval_CheckExceptTypeValid(PyThreadState *tstate, PyObject* right);
PyAPI_FUNC(int) _PyEval_ExceptionGroupMatch(_PyInterpreterFrame *, PyObject* exc_value, PyObject *match_type, PyObject **match, PyObject **rest);
PyAPI_FUNC(void) _PyEval_FormatAwaitableError(PyThreadState *tstate, PyTypeObject *type, int oparg);
PyAPI_FUNC(void) _PyEval_FormatExcCheckArg(PyThreadState *tstate, PyObject *exc, const char *format_str, PyObject *obj);
PyAPI_FUNC(void) _PyEval_FormatExcUnbound(PyThreadState *tstate, PyCodeObject *co, int oparg);
PyAPI_FUNC(void) _PyEval_FormatKwargsError(PyThreadState *tstate, PyObject *func, PyObject *kwargs, PyObject *dupkey);
PyAPI_FUNC(PyObject *) _PyEval_ImportFrom(PyThreadState *, PyObject *, PyObject *);
PyAPI_FUNC(PyValue) _PyEval_ImportFromValue(PyThreadState *, PyValueBorrow, PyObject *);

PyAPI_FUNC(PyObject *) _PyEval_LazyImportName(
    PyThreadState *tstate, PyObject *builtins, PyObject *globals,
    PyObject *locals, PyObject *name, PyObject *fromlist, int level,
    int lazy);
PyAPI_FUNC(PyObject *) _PyEval_LazyImportFrom(
    PyThreadState *tstate, _PyInterpreterFrame *frame, PyObject *v, PyObject *name);
PyAPI_FUNC(PyObject *) _PyEval_ImportName(
    PyThreadState *tstate, PyObject *builtins, PyObject *globals,
    PyObject *locals, PyObject *name, PyObject *fromlist, int level);
PyObject * _PyEval_ImportNameWithImport(
    PyThreadState *tstate, PyObject *import_func, PyObject *globals,
    PyObject *locals, PyObject *name, PyObject *fromlist, int level);
PyAPI_FUNC(PyObject *)_PyEval_MatchClass(PyThreadState *tstate, PyValueBorrow subject, PyObject *type, Py_ssize_t nargs, PyObject *kwargs);
#define _PyEval_MatchClass(T, S, TY, N, K) \
    (_PyEval_MatchClass)(T, PyValue_Borrow(S), TY, N, K)
PyAPI_FUNC(PyObject *)_PyEval_MatchKeys(PyThreadState *tstate, PyObject *map, PyObject *keys);
PyAPI_FUNC(void) _PyEval_MonitorRaise(PyThreadState *tstate, _PyInterpreterFrame *frame, _PyInstr *instr);
PyAPI_FUNC(bool) _PyEval_NoToolsForUnwind(PyThreadState *tstate, _PyInterpreterFrame *frame);
/* Consumes v, including on error; initialized arena outputs survive cleanup. */
PyAPI_FUNC(int) _PyEval_UnpackIterableValue(PyThreadState *tstate, PyObject *v, int argcnt, int argcntafter, PyValue *sp);
/* The returned owner remains C-owned until ordinary result publication. */
PyAPI_FUNC(PyValue) _PyEval_FrameClearAndPopWithResult(
    PyThreadState *tstate, _PyInterpreterFrame *frame, PyValue result);
/* Error/close paths carry no detached result. */
PyAPI_FUNC(void) _PyEval_FrameClearAndPop(PyThreadState *tstate,
                                        _PyInterpreterFrame *frame, uint8_t reason);
/* Distinct semantic outcomes share one completion/ownership protocol. */
enum _PyIRStepKind { _PY_IR_FOR_ITER, _PY_IR_SEND, _PY_IR_SEND_THROW };
enum _PyIRStepStatus {
    _PY_IR_STEP_ERROR_PRE = -2,
    _PY_IR_STEP_ERROR_POST = -1,
    _PY_IR_STEP_EXIT = 0,
    _PY_IR_STEP_REDIRECT = 1
};
typedef struct {
    _PyInstr *next;
    enum _PyIRStepStatus status;
} _PyIRStepResult;

/* PRE (0) observes the held completion value while the input window is
 * intact. POST (1) observes the committed semantic outcome. */
PyAPI_FUNC(_PyIRStepResult) _PyIR_ObserveStep(
    PyThreadState *, _PyInterpreterFrame *, _PyInstr *, _PyInstr *,
    PyValueBorrow, enum _PyIRStepKind, int);
PyAPI_FUNC(_PyIRStepResult) _PyEval_CompleteStep(
    PyThreadState *, _PyInterpreterFrame *, _PyInstr *, int, int,
    _PyInstr *, PyValue, enum _PyIRStepKind);
PyAPI_FUNC(_PyIRStepResult) _PyEval_CompleteGeneratorReturn(
    PyThreadState *, _PyInterpreterFrame *, PyValue);

PyAPI_FUNC(PyObject *) _PyEval_GetANext(PyObject *aiter);
PyAPI_FUNC(void) _PyEval_LoadGlobalValue(PyObject *globals, PyObject *builtins,
    PyObject *name, PyValue *writeto);
PyAPI_FUNC(PyObject *) _PyEval_GetAwaitable(PyObject *iterable, int oparg);
PyAPI_FUNC(PyValue) _PyEval_LoadName(PyThreadState *tstate, _PyInterpreterFrame *frame, PyObject *name);
PyAPI_FUNC(int)
_Py_Check_ArgsIterable(PyThreadState *tstate, PyObject *func, PyObject *args);
PyAPI_FUNC(int) _PyEval_StoreName(PyThreadState *tstate, PyValueBorrow v, PyObject *name, PyObject* ns);
#define _PyEval_StoreName(V0, V1, V2, V3) \
    (_PyEval_StoreName)(V0, PyValue_Borrow(V1), V2, V3)
PyAPI_FUNC(PyValue) _PyEval_BinarySlice(PyValueBorrow container, PyValueBorrow start, PyValueBorrow stop);
#define _PyEval_BinarySlice(V0, V1, V2) \
    (_PyEval_BinarySlice)(PyValue_Borrow(V0), PyValue_Borrow(V1), PyValue_Borrow(V2))
PyAPI_FUNC(int) _PyEval_StoreSlice(PyValueBorrow v, PyValueBorrow container, PyValueBorrow start, PyValueBorrow stop);
#define _PyEval_StoreSlice(V0, V1, V2, V3) \
    (_PyEval_StoreSlice)(PyValue_Borrow(V0), PyValue_Borrow(V1), PyValue_Borrow(V2), PyValue_Borrow(V3))
PyAPI_FUNC(int) _PyEval_IsInstanceValue(PyValueBorrow inst, PyValueBorrow cls);
#define _PyEval_IsInstanceValue(V0, V1) \
    (_PyEval_IsInstanceValue)(PyValue_Borrow(V0), PyValue_Borrow(V1))

/*
 * Indicate whether a special method of given 'oparg' can use the (improved)
 * alternative error message instead. Only methods loaded by LOAD_SPECIAL
 * support alternative error messages.
 *
 * Symbol is exported for the JIT (see discussion on GH-132218).
 */
PyAPI_FUNC(int)
_PyEval_SpecialMethodCanSuggest(PyObject *self, int oparg);

/* Bits that can be set in PyThreadState.eval_breaker */
#define _PY_GIL_DROP_REQUEST_BIT (1U << 0)
#define _PY_SIGNALS_PENDING_BIT (1U << 1)
#define _PY_CALLS_TO_DO_BIT (1U << 2)
#define _PY_ASYNC_EXCEPTION_BIT (1U << 3)
#define _PY_GC_SCHEDULED_BIT (1U << 4)
#define _PY_EVAL_PLEASE_STOP_BIT (1U << 5)
#define _PY_EVAL_EXPLICIT_MERGE_BIT (1U << 6)
/* Entries on the borrow stack above the eval loop's floor (pycore_borrow.h) */
#define _PY_BORROW_RELEASE_BIT (1U << 7)

/* Reserve a few bits for future use */
#define _PY_EVAL_EVENTS_BITS 8
#define _PY_EVAL_EVENTS_MASK ((1 << _PY_EVAL_EVENTS_BITS)-1)

static inline void
_Py_set_eval_breaker_bit(PyThreadState *tstate, uintptr_t bit)
{
    _Py_atomic_or_uintptr(&tstate->eval_breaker, bit);
}

static inline void
_Py_unset_eval_breaker_bit(PyThreadState *tstate, uintptr_t bit)
{
    _Py_atomic_and_uintptr(&tstate->eval_breaker, ~bit);
}

static inline int
_Py_eval_breaker_bit_is_set(PyThreadState *tstate, uintptr_t bit)
{
    uintptr_t b = _Py_atomic_load_uintptr_relaxed(&tstate->eval_breaker);
    return (b & bit) != 0;
}

// Free-threaded builds use these functions to set or unset a bit on all
// threads in the given interpreter.
void _Py_set_eval_breaker_bit_all(PyInterpreterState *interp, uintptr_t bit);
void _Py_unset_eval_breaker_bit_all(PyInterpreterState *interp, uintptr_t bit);

#ifndef Py_SUPPORTS_REMOTE_DEBUG
    #if defined(__APPLE__)
    #include <TargetConditionals.h>
    #  if !defined(TARGET_OS_OSX)
// Older macOS SDKs do not define TARGET_OS_OSX
    #     define TARGET_OS_OSX 1
    #  endif
    #endif
    #if ((defined(__APPLE__) && TARGET_OS_OSX) || defined(MS_WINDOWS) || (defined(__linux__) && HAVE_PROCESS_VM_READV))
    #    define Py_SUPPORTS_REMOTE_DEBUG 1
    #endif
#endif

#if defined(Py_REMOTE_DEBUG) && defined(Py_SUPPORTS_REMOTE_DEBUG)
extern int _PyRunRemoteDebugger(PyThreadState *tstate);
#endif

/* Special methods used by LOAD_SPECIAL */
#define SPECIAL___ENTER__   0
#define SPECIAL___EXIT__    1
#define SPECIAL___AENTER__  2
#define SPECIAL___AEXIT__   3
#define SPECIAL_MAX   3

PyAPI_DATA(const _PyInstr *) _Py_INTERPRETER_TRAMPOLINE_INSTRUCTIONS_PTR;

/* Helper functions for large uops */

PyAPI_FUNC(PyObject *)
_Py_BuildString_ValueSteal(
    struct _PyInterpreterFrame *frame,
    PyValue *arguments,
    int total_args);

PyAPI_FUNC(PyObject *)
_Py_BuildMap_ValueSteal(
    struct _PyInterpreterFrame *frame,
    PyValue *arguments,
    int half_args);


// Like PyMapping_GetOptionalItem, but returns the PyObject* instead of taking
// it as an out parameter. This helps MSVC's escape analysis when used with
// tail calling.
Py_VALUE_COMPAT(_PyValue_GetOptionalItem)
PyAPI_FUNC(PyObject*) _PyMapping_GetOptionalItem2(PyObject* obj, PyObject* key, int* err);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CEVAL_H */
