#ifndef Py_INTERNAL_JIT_H
#define Py_INTERNAL_JIT_H
#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif
#include "pycore_ir.h"
#include "pycore_interpframe.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Helpers return a scalar C enum; all other results are written through ctx.
 * No aggregate-return ABI is part of the eventual machine-code interface. */
typedef enum {
    _PyJIT_CONTINUE, _PyJIT_BRANCH, _PyJIT_ENTER_FRAME, _PyJIT_RETURN,
    _PyJIT_RAISE, _PyJIT_YIELD, _PyJIT_DEOPT, _PyJIT_ACTION_COUNT
} _PyJitAction;
static_assert(sizeof(_PyJitAction) == sizeof(int), "JIT helper scalar return ABI");
typedef enum {
    _PyJIT_AFTER, _PyJIT_ENTER_PREPARED, _PyJIT_ENTER_STARTED,
    _PyJIT_RETURN_COMPLETED, _PyJIT_ERROR_LOG_PENDING,
    _PyJIT_UNWIND_PENDING, _PyJIT_DEOPT_BEFORE, _PyJIT_DEOPT_AFTER, _PyJIT_PHASE_COUNT
} _PyJitPhase;
typedef struct _PyJitCodeUnit _PyJitCodeUnit;
typedef struct _PyJitState _PyJitState;
typedef struct {
    _PyInterpreterFrame *caller;
    uint32_t call_word, return_word;
    uint64_t token;
} _PyJitPending;
typedef struct {
    unsigned int depth;
    unsigned char budgeted;
    uint64_t generation;
    _PyJitPending pending[64];
} _PyJitFlow;
typedef struct {
    PyThreadState *tstate;
    _PyInterpreterFrame *frame;
    /* Always a word in frame; PREPARED points at the unstarted callee.
     * The caller's call word is already published in frame->previous. */
    _PyIRWord *next_instr;
    /* Set only for RAISE: frame->instr_ptr may have separate saved-lasti meaning. */
    _PyIRWord *raising_instr;
    _PyJitPhase phase;
    /* Unit is pinned until ReleaseActivation. Code/exec are borrowed only
     * while the frame lives: RETURN can destroy them before helper return.
     * Never copy an activation context or read these borrows after transfer. */
    _PyJitCodeUnit *unit;
    PyCodeObject *code;
    _PyCodeExec *expected_exec;
    void *entry_target;
    int from_return;
    _PyJitFlow **flow_owner;
    _PyInterpreterFrame *pending_caller;
    uint32_t pending_call, pending_return;
    void *cold_request;  /* stack-scoped test token, never an owning root */
} _PyJitContext;
/* Owned by the method descriptor or by the semantic-step caller. No object
 * references are embedded; the active frame roots code and feedback. */
typedef struct {
    _PyIRWord *instruction;
    uint32_t fields[4];
    uint32_t wide;
    uint16_t handler;
} _PyJitDecodedOp;
typedef _PyJitAction (*_PyJitHelper)(_PyJitContext *, const _PyJitDecodedOp *);
typedef struct {
    const char *name;
    _PyJitHelper helper;
    const char *tier_down_reason;
} _PyJitHelperInfo;

/* Fixed-home call-aware LIR. Optimizing method/trace builders can use this
 * backend without introducing a second frame or Python-operation executor. */
typedef enum {
    _PyJIT_LIR_HELPER, _PyJIT_LIR_BRANCH, _PyJIT_LIR_EXIT
} _PyJitLirKind;
typedef struct {
    _PyJitDecodedOp decoded;
    _PyJitHelper helper;
    uint32_t word;
    uint32_t fallthrough;
    uint32_t target;
    _PyJitLirKind kind;
} _PyJitLirOp;
typedef _PyJitAction (*_PyJitEntry)(_PyJitContext *);
struct _PyJitCodeUnit {
    size_t refs;
    int detached;
    uint64_t generation;
    uintptr_t epoch;
    uint64_t source_stamp;
    int64_t interpreter_id;
    uint32_t nops;
    _PyJitLirOp *ops;
    int32_t *offsets;
    void *mapping;
    size_t mapping_size;
    size_t code_size;
    _PyJitEntry entry;
    size_t charged_bytes;
    uint32_t entry_word;
    uint16_t entry_handler;
    unsigned char budgeted;
    unsigned char entry_seen;
    unsigned char c_call_seen;
    unsigned char return_resume_seen;
};
typedef enum {
    _PyJIT_FAILURE_NONE, _PyJIT_FAILURE_BUILD, _PyJIT_FAILURE_INVALIDATED,
    _PyJIT_FAILURE_QUOTA, _PyJIT_FAILURE_ENTRY, _PyJIT_FAILURE_COVERAGE
} _PyJitFailure;
struct _PyJitState {
    size_t refs;
    uint64_t generation;
    int building;
    int disabled;
    int coverage;  /* 0 unknown, 1 complete, -1 immutable unsupported */
    int unsupported_word;
    int unsupported_handler;
    _PyJitFailure last_failure;
    uint32_t compile_attempts, compile_failures;
    uint64_t compile_ns;
    unsigned char budgeted;
    uint16_t loop_heat;
    uint16_t cooldown;
    _PyJitCodeUnit *unit;
};

#if defined(__linux__) && defined(__x86_64__) && !defined(Py_GIL_DISABLED) && \
    !defined(Py_STATS) && !_Py_TAIL_CALL_INTERP
#  define _PyJIT_BASELINE_SUPPORTED 1
#else
#  define _PyJIT_BASELINE_SUPPORTED 0
#endif

extern void _PyJit_SeedBackedges(PyCodeObject *, _PyCodeExec *);
extern int _PyJit_AutoEligible(PyObject *);
extern int _PyJit_PrepareCall(_PyJitContext *, _PyIRWord *, _PyStackRef);
extern int _PyJit_WarmBackedge(PyThreadState *, _PyInterpreterFrame *);
extern void _PyJit_PushPending(_PyJitContext *);
extern int _PyJit_PopPending(_PyJitFlow *, _PyInterpreterFrame *, _PyIRWord *);
extern void _PyJit_RetirePending(_PyJitFlow *, _PyInterpreterFrame *);
extern int _PyJit_FreeFlow(_PyJitFlow **);
extern int _PyJit_ObserversAllow(PyThreadState *, _PyInterpreterFrame *);
extern _PyJitCodeUnit *_PyJit_BuildBaseline(PyCodeObject *, _PyCodeExec *, uint64_t, uintptr_t, uint64_t);
extern int _PyJit_EmitBaseline(_PyJitCodeUnit *);
extern void _PyJit_FreeUnit(_PyJitCodeUnit *);
extern void _PyJit_DetachExec(_PyCodeExec *);
extern void _PyJit_ClearExec(_PyCodeExec *);
extern size_t _PyJit_ExecSizeOf(_PyCodeExec *);
extern int _PyJit_PrepareActivation(PyThreadState *, _PyInterpreterFrame *, _PyIRWord *, int, _PyJitContext *);
extern int _PyJit_ReleaseActivation(_PyJitContext *);
extern void _PyJit_NativeEntered(_PyJitContext *);
extern _PyJitAction _PyJit_AfterHelper(_PyJitContext *, _PyJitAction, const _PyJitLirOp *);
extern _PyJitAction _PyJit_ExitBefore(_PyJitContext *, const _PyJitDecodedOp *);
PyAPI_FUNC(int) _PyJit_CompileCode(PyCodeObject *);
PyAPI_FUNC(PyObject *) _PyJit_CodeInfo(PyCodeObject *);

typedef struct _PyJitTestRequest {
    struct _PyJitTestRequest *previous;
    PyThreadState *tstate;
    PyCodeObject *code;
    _PyInterpreterFrame *claimed_frame;
    Py_ssize_t remaining;
    Py_ssize_t actions[_PyJIT_ACTION_COUNT];
    Py_ssize_t phases[_PyJIT_PHASE_COUNT];
    int claimed;
} _PyJitTestRequest;

extern const _PyJitHelperInfo *_PyJit_GetHelper(unsigned int);
extern _PyJitTestRequest *_PyJit_TestClaim(PyThreadState *, _PyInterpreterFrame *, void **);
extern void _PyJit_RetireCold(void *, _PyInterpreterFrame *);
PyAPI_FUNC(PyObject *) _PyJit_TestColdCall(PyObject *, PyObject *, PyObject *, int, int);
PyAPI_FUNC(PyObject *) _PyJit_TestCall(PyObject *, PyObject *, Py_ssize_t);

#ifdef __cplusplus
}
#endif
#endif
