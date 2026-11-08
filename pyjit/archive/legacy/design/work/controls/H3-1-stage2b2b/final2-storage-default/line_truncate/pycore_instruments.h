#ifndef Py_INTERNAL_INSTRUMENT_H
#define Py_INTERNAL_INSTRUMENT_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

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
PyAPI_FUNC(int)
_Py_call_instrumentation(PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _PyInstr *instr);

PyAPI_FUNC(int)
_Py_call_instrumentation_line(PyThreadState *tstate, _PyInterpreterFrame* frame,
                              _PyInstr *instr, _PyInstr *prev);

PyAPI_FUNC(int)
_Py_call_instrumentation_instruction(
    PyThreadState *tstate, _PyInterpreterFrame* frame, _PyInstr *instr);

PyAPI_FUNC(_PyInstr *)
_Py_call_instrumentation_jump(
    _PyInstr *instr, PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _PyInstr *src, _PyInstr *dest);

PyAPI_FUNC(int)
_Py_call_instrumentation_arg(PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _PyInstr *instr, PyObject *arg);

PyAPI_FUNC(int)
_Py_call_instrumentation_2args(PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _PyInstr *instr, PyObject *arg0, PyObject *arg1);

PyAPI_FUNC(void)
_Py_call_instrumentation_exc2(PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _PyInstr *instr, PyObject *arg0, PyObject *arg1);

PyAPI_DATA(PyObject) _PyInstrumentation_MISSING;
PyAPI_DATA(PyObject) _PyInstrumentation_DISABLE;


/* Total tool ids available */
#define  PY_MONITORING_TOOL_IDS 8
/* Count of all "real" monitoring events (not derived from other events).
 * "Other" events can now be turned on/disabled per code object. */
#define _PY_MONITORING_UNGROUPED_EVENTS 16
/* Count of all  monitoring events */
#define _PY_MONITORING_EVENTS 19

/* Tables of which tools are active for each monitored event. */
typedef struct _Py_LocalMonitors {
    uint8_t tools[_PY_MONITORING_UNGROUPED_EVENTS];
} _Py_LocalMonitors;

typedef struct _Py_GlobalMonitors {
    uint8_t tools[_PY_MONITORING_UNGROUPED_EVENTS];
} _Py_GlobalMonitors;

/* Per-word absolute lines and canonical displaced opcodes.  A zero token
 * means that the word is not eligible for LINE instrumentation. */
typedef struct {
    uint16_t *original_opcodes;
    int lines[1];
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
    /* The version of tools when they instrument the code */
    uintptr_t tool_versions[PY_MONITORING_TOOL_IDS];
    /* Information to support line events */
    _PyCoLineInstrumentationData *lines;
    /* The tools that are to be notified for line events for the matching code unit */
    uint8_t *line_tools;
    /* Information to support instruction events */
    /* The underlying instructions, which can themselves be instrumented */
    uint16_t *per_instruction_opcodes;
    /* The tools that are to be notified for instruction events for the matching code unit */
    uint8_t *per_instruction_tools;
} _PyCoMonitoringData;

PyAPI_FUNC(int)
_Py_Instrumentation_GetLine(_PyCoLineInstrumentationData *line_data, int index);

static inline uint16_t
_PyCode_GetOriginalOpcode(_PyCoLineInstrumentationData *line_data, int index)
{
    return line_data->original_opcodes[index];
}

static inline void
_PyCode_SetOriginalOpcode(_PyCoLineInstrumentationData *line_data, int index,
                          uint16_t opcode)
{
    line_data->original_opcodes[index] = opcode;
}

static inline void
_PyCode_SetMonitoringLine(_PyCoLineInstrumentationData *line_data, int index,
                          int line)
{
    assert(line >= -1);
    line_data->lines[index] = (uint16_t)line;
}

// Exported for external JIT support. Invalid scalar IDs return -1.
PyAPI_FUNC(int) _PyCode_Deinstrument(int opcode);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_INSTRUMENT_H */
