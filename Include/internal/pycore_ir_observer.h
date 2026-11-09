#ifndef Py_INTERNAL_IR_OBSERVER_H
#define Py_INTERNAL_IR_OBSERVER_H
#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_structs.h"
#include "pycore_typedefs.h"

typedef enum {
    _PY_IR_OBSERVER_UNSUPPORTED,
    _PY_IR_OBSERVER_PRE_WORD,
    _PY_IR_OBSERVER_POST_COMMITTED,
    _PY_IR_OBSERVER_SOURCE_POST,
    _PY_IR_OBSERVER_STEP_READY,
    _PY_IR_OBSERVER_YIELD_READY,
} _PyIRObserverPhase;

/* Borrowed coordinates of one synchronous callback. The provider's intent
 * snapshot is retained across all tools receiving the same event. */
typedef struct {
    _PyInterpreterFrame *frame;
    _PyInstr *site;
    _PyInstr *next;
    uint64_t generation;
    int event;
    _PyIRObserverPhase phase;
    int edge;
    uint32_t source_record;
} _PyIRObserverSpec;

typedef struct {
    struct _PyEvalActivation *activation;
    const _PyIRObserverSpec *previous;
} _PyIRObserverScope;

void _PyEval_EnterObserverContext(PyThreadState *, const _PyIRObserverSpec *,
                                  _PyIRObserverScope *);
void _PyEval_LeaveObserverContext(PyThreadState *, _PyIRObserverScope *);
int _PyEval_GetObserverContext(PyThreadState *, _PyInterpreterFrame *, int,
                               _PyIRObserverSpec *);
void _PyEval_InvalidateSourceErrors(PyThreadState *, _PyInterpreterFrame *);

typedef struct {
    int status;  /* -1 error, 0 unchanged, 1 committed redirect */
    _PyInstr *next;
} _PyIRObserverResult;

_PyIRObserverResult _PyIR_ObserveCommittedBranch(
    PyThreadState *, _PyInterpreterFrame *, _PyInstr *site,
    _PyInstr *reported_source, _PyInstr *next, int event, int edge);

#endif
