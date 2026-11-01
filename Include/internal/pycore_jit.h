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
typedef struct {
    PyThreadState *tstate;
    _PyInterpreterFrame *frame;
    /* Always a word in frame; PREPARED points at the unstarted callee.
     * The caller's call word is already published in frame->previous. */
    _PyIRWord *next_instr;
    /* Set only for RAISE: frame->instr_ptr may have separate saved-lasti meaning. */
    _PyIRWord *raising_instr;
    _PyJitPhase phase;
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
extern _PyJitTestRequest *_PyJit_TestClaim(PyThreadState *, _PyInterpreterFrame *);
PyAPI_FUNC(PyObject *) _PyJit_TestCall(PyObject *, PyObject *, Py_ssize_t);

#ifdef __cplusplus
}
#endif
#endif
