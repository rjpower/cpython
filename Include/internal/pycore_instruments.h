#ifndef Py_INTERNAL_INSTRUMENT_H
#define Py_INTERNAL_INSTRUMENT_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_ir_observer.h"   // _PyIRObserverResult
#include "pycore_structs.h"       // _PyInstr
#include "pycore_typedefs.h"      // _PyInterpreterFrame

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t _PyMonitoringEventSet;

/* Tool IDs */

/* These are defined in PEP 669 for convenience to avoid clashes */
#define PY_MONITORING_DEBUGGER_ID 0
#define PY_MONITORING_COVERAGE_ID 1
#define PY_MONITORING_PROFILER_ID 2
#define PY_MONITORING_OPTIMIZER_ID 5

/* Internal IDs used to support sys.setprofile() and sys.settrace() */
#define PY_MONITORING_SYS_PROFILE_ID 6
#define PY_MONITORING_SYS_TRACE_ID 7


PyObject *_PyMonitoring_RegisterCallback(int tool_id, int event_id, PyObject *obj);

int _PyMonitoring_SetEvents(int tool_id, _PyMonitoringEventSet events);
int _PyMonitoring_SetLocalEvents(PyCodeObject *code, int tool_id, _PyMonitoringEventSet events);
int _PyMonitoring_GetLocalEvents(PyCodeObject *code, int tool_id, _PyMonitoringEventSet *events);


// these are exported only for other re-generated interpreters to call
//
// The calls into the monitoring tools are cold: a handler reaches them only
// when a tool has registered for the event.
PyAPI_FUNC(int) _Py_COLD_FUNCTION
_Py_call_instrumentation(PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _PyInstr *instr);

/* Fire the LINE event of the word at instr, which follows prev: 0, or -1
 * with an exception set.  A callback may move the frame (instr_ptr). */
PyAPI_FUNC(int) _Py_COLD_FUNCTION
_Py_call_instrumentation_line(PyThreadState *tstate, _PyInterpreterFrame* frame,
                              _PyInstr *instr, _PyInstr *prev);

/* Fire the INSTRUCTION event of the word at instr: 0, or -1 */
PyAPI_FUNC(int) _Py_COLD_FUNCTION
_Py_call_instrumentation_instruction(
    PyThreadState *tstate, _PyInterpreterFrame* frame, _PyInstr *instr);

/* The jump or branch event and the line events of its edge: status -1 with
 * an exception set, 0 to go on at dest, or 1 when a callback redirected the
 * frame to `next`, frame->instr_ptr (which may be dest or the current word) */
PyAPI_FUNC(_PyIRObserverResult) _Py_COLD_FUNCTION
_Py_call_instrumentation_jump(
    _PyInstr *instr, PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _PyInstr *src, _PyInstr *dest);

/* The arguments are borrowed for the call; the macros take owners and
   borrows alike. An absent argument is PyValue_FromObjectBorrow(
   &_PyInstrumentation_MISSING). */
PyAPI_FUNC(int) _Py_COLD_FUNCTION
(_Py_call_instrumentation_arg)(PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _PyInstr *instr, PyValueBorrow arg);

PyAPI_FUNC(int) _Py_COLD_FUNCTION
(_Py_call_instrumentation_2args)(PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _PyInstr *instr,
    PyValueBorrow arg0, PyValueBorrow arg1);

PyAPI_FUNC(void) _Py_COLD_FUNCTION
(_Py_call_instrumentation_exc2)(PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _PyInstr *instr,
    PyValueBorrow arg0, PyValueBorrow arg1);

#define _Py_call_instrumentation_arg(T, E, F, I, A) \
    (_Py_call_instrumentation_arg)(T, E, F, I, PyValue_Borrow(A))
#define _Py_call_instrumentation_2args(T, E, F, I, A, B) \
    (_Py_call_instrumentation_2args)(T, E, F, I, PyValue_Borrow(A), PyValue_Borrow(B))
#define _Py_call_instrumentation_exc2(T, E, F, I, A, B) \
    (_Py_call_instrumentation_exc2)(T, E, F, I, PyValue_Borrow(A), PyValue_Borrow(B))

PyAPI_DATA(PyObject) _PyInstrumentation_MISSING;
PyAPI_DATA(PyObject) _PyInstrumentation_DISABLE;


/* Total tool ids available */
#define  PY_MONITORING_TOOL_IDS 8
/* Count of all "real" monitoring events (not derived from other events).
 * "Other" events can now be turned on/disabled per code object. */
#define _PY_MONITORING_UNGROUPED_EVENTS 16
/* Count of all  monitoring events */
#define _PY_MONITORING_EVENTS 19

/* Private observer phase: never a registered monitoring event or tool index. */
#define _PY_DEBUGGER_ENTRY_EVENT _PY_MONITORING_EVENTS

/* What a source edge record forces at a word while LINE is observed
 * (_PyCoMonitoringData.source_forced): the branch's observer, which reports
 * the edge (1); the LINE event at the landing (2); the INSTRUCTION event,
 * which records an ingress PC that nothing else saves (4). */
enum {
    _PyIR_SOURCE_FORCE_BRANCH = 1,
    _PyIR_SOURCE_FORCE_LINE = 2,
    _PyIR_SOURCE_FORCE_INGRESS = 4,
};

typedef struct _Py_LocalMonitors {
    uint8_t tools[_PY_MONITORING_UNGROUPED_EVENTS];
} _Py_LocalMonitors;

typedef struct _Py_GlobalMonitors {
    uint8_t tools[_PY_MONITORING_UNGROUPED_EVENTS];
} _Py_GlobalMonitors;

/* Ancillary data structure used for instrumentation.
   Line instrumentation creates this with sufficient
   space for one entry per code unit. The total size
   of the data will be `bytes_per_entry * Py_SIZE(code)` */
typedef struct {
    uint8_t bytes_per_entry;
    uint8_t data[1];
} _PyCoLineInstrumentationData;


/* Main data structure used for instrumentation.
 * This is allocated when needed for instrumentation
 */
typedef struct _PyCoMonitoringData {
    /* Monitoring specific to this code object */
    _Py_LocalMonitors local_monitors;
    /* Monitoring that is active on this code object */
    _Py_LocalMonitors active_monitors;
    /* The tools that are to be notified for events for the matching code unit */
    uint8_t *tools;
    /* The tools that are to be notified of BRANCH_LEFT by the conditional
     * jumps and the END_SEND of an `async for`, whose BRANCH_RIGHT or
     * STOP_ITERATION tools are in tools.  NULL until BRANCH_LEFT is first
     * monitored, and then tools is not NULL */
    uint8_t *left_tools;
    /* FOR_ITER's completion observation is independent of both branches. */
    uint8_t *stop_tools;
    /* The version of tools when they instrument the code */
    uintptr_t tool_versions[PY_MONITORING_TOOL_IDS];
    /* Information to support line events */
    _PyCoLineInstrumentationData *lines;
    /* The tools that are to be notified for line events for the matching code unit */
    uint8_t *line_tools;
    uint8_t *source_line_tools;  /* per source-only edge record */
    uint64_t line_epoch;        /* invalidates pending observed LINE landings */
    /* What source edges force at each word (_PyIR_SOURCE_FORCE_*) */
    uint8_t *source_forced;
    /* The tools that are to be notified for instruction events for the matching code unit */
    uint8_t *per_instruction_tools;
    /* The words and source edge records at which a tool listens: the code
     * is observed (_PyCode_OBSERVED_MARK) while there is one */
    uint32_t observed_tools;
} _PyCoMonitoringData;

extern int
_Py_Instrumentation_GetLine(PyCodeObject *code, _PyCoLineInstrumentationData *line_data, int index);

/* The flags in the first byte of a lines[] entry */
#define _PyCo_LINE_STARTS 1          // the word starts a line

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_INSTRUMENT_H */
