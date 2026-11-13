/* Support for legacy tracing on top of PEP 669 instrumentation
 * Provides callables to forward PEP 669 events to legacy events.
 */

#include "Python.h"
#include "pycore_audit.h"         // _PySys_Audit()
#include "pycore_ceval.h"         // export _PyEval_SetProfile()
#include "pycore_frame.h"         // PyFrameObject members
#include "pycore_interpframe.h"   // _PyFrame_GetCode()
#include "pycore_ir.h"            // _PyCode_InstrHandler()
#include "pycore_ir_source.h"     // _PyIRObservation_Stored()
#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_pyerrors.h"      // PyStopIterationObject
#include "pycore_tstate.h"        // _PyThreadStateImpl
#include "pycore_tuple.h"         // _PyTuple_PackValues()
#include "pycore_value_api.h"     // _PyBox_CALL_ADAPTER()
#include "pycore_value_call.h"    // _PyValue_VectorcallThunk()
#include "pycore_value_ops.h"     // _PyValue_DescrGet()

#include "opcode.h"
#include <stddef.h>


typedef struct _PyLegacyEventHandler {
    PyObject_HEAD
    vectorcallfunc vectorcall;
    _Py_vectorcallvaluefunc value_vectorcall;
    int event;
} _PyLegacyEventHandler;

#define _PyLegacyEventHandler_CAST(op)  ((_PyLegacyEventHandler *)(op))

/* The handlers are value functions: the event arguments are words (the code
 * object, inline ints for offsets and lines) and the result is an owned word.
 *
 * The _Py_valuetracefunc function expects the following arguments:
 *   obj: the trace object (PyObject *)
 *   frame: the current frame (PyFrameObject *)
 *   kind: the kind of event, see PyTrace_XXX #defines (int)
 *   arg: The arg, a borrowed word (PyValue_NULL for none)
 */

static PyValue
call_profile_func(_PyLegacyEventHandler *self, PyValueBorrow arg)
{
    PyThreadState *tstate = _PyThreadState_GET();
    _Py_valuetracefunc profilefunc =
        ((_PyThreadStateImpl *)tstate)->value_profilefunc;
    if (profilefunc == NULL) {
        return PyValue_None;
    }
    PyFrameObject *frame = PyEval_GetFrame();
    if (frame == NULL) {
        PyErr_SetString(PyExc_SystemError,
                        "Missing frame when calling profile function.");
        return PyValue_NULL;
    }
    Py_INCREF(frame);
    int err = profilefunc(tstate->c_profileobj, frame, self->event, arg);
    Py_DECREF(frame);
    if (err) {
        return PyValue_NULL;
    }
    return PyValue_None;
}

static PyValue
sys_profile_start(
    PyThreadState *tstate, PyObject *callable, const PyValue *args,
    size_t nargsf, PyObject *kwnames
) {
    _PyLegacyEventHandler *self = _PyLegacyEventHandler_CAST(callable);
    assert(kwnames == NULL);
    assert(PyVectorcall_NARGS(nargsf) == 2);
    return call_profile_func(self, PyValue_Borrow(PyValue_None));
}

static PyValue
sys_profile_throw(
    PyThreadState *tstate, PyObject *callable, const PyValue *args,
    size_t nargsf, PyObject *kwnames
) {
    _PyLegacyEventHandler *self = _PyLegacyEventHandler_CAST(callable);
    assert(kwnames == NULL);
    assert(PyVectorcall_NARGS(nargsf) == 3);
    return call_profile_func(self, PyValue_Borrow(PyValue_None));
}

static PyValue
sys_profile_return(
    PyThreadState *tstate, PyObject *callable, const PyValue *args,
    size_t nargsf, PyObject *kwnames
) {
    _PyLegacyEventHandler *self = _PyLegacyEventHandler_CAST(callable);
    assert(kwnames == NULL);
    assert(PyVectorcall_NARGS(nargsf) == 3);
    return call_profile_func(self, PyValue_Borrow(args[2]));
}

static PyValue
sys_profile_unwind(
    PyThreadState *tstate, PyObject *callable, const PyValue *args,
    size_t nargsf, PyObject *kwnames
) {
    _PyLegacyEventHandler *self = _PyLegacyEventHandler_CAST(callable);
    assert(kwnames == NULL);
    assert(PyVectorcall_NARGS(nargsf) == 3);
    return call_profile_func(self, PyValue_Borrow(PyValue_NULL));
}

static PyValue
sys_profile_call_or_return(
    PyThreadState *tstate, PyObject *op, const PyValue *args,
    size_t nargsf, PyObject *kwnames
) {
    _PyLegacyEventHandler *self = _PyLegacyEventHandler_CAST(op);
    assert(kwnames == NULL);
    assert(PyVectorcall_NARGS(nargsf) == 4);
    if (!PyValue_IsObject(PyValue_Borrow(args[2]))) {
        return PyValue_None;
    }
    PyObject *callable = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[2]));
    if (PyCFunction_Check(callable)) {
        return call_profile_func(self, PyValue_Borrow(args[2]));
    }
    if (Py_TYPE(callable) == &PyMethodDescr_Type) {
        PyValueBorrow self_arg = PyValue_Borrow(args[3]);
        /* For backwards compatibility need to
         * convert to builtin method */

        /* If no arg, skip */
        if (PyValue_IsObject(self_arg) &&
            PyValue_AsKnownObjectBorrow(self_arg) == &_PyInstrumentation_MISSING) {
            return PyValue_None;
        }
        PyValue meth = _PyValue_DescrGet(callable, self_arg,
                                         PyValue_TYPE(self_arg));
        if (PyValue_IsNull(meth)) {
            return PyValue_NULL;
        }
        PyValue res = call_profile_func(self, PyValue_Borrow(meth));
        PyValue_CLOSE(meth);
        return res;
    }
    return PyValue_None;
}

static int
set_opcode_trace_world_stopped(PyCodeObject *code, bool enable)
{
    _PyMonitoringEventSet events = 0;
    if (_PyMonitoring_GetLocalEvents(code, PY_MONITORING_SYS_TRACE_ID, &events) < 0) {
        return -1;
    }

    if (enable) {
        if (events & (1 << PY_MONITORING_EVENT_INSTRUCTION)) {
            return 0;
        }
        events |= (1 << PY_MONITORING_EVENT_INSTRUCTION);
    } else {
        if (!(events & (1 << PY_MONITORING_EVENT_INSTRUCTION))) {
            return 0;
        }
        events &= (~(1 << PY_MONITORING_EVENT_INSTRUCTION));
    }
    return _PyMonitoring_SetLocalEvents(code, PY_MONITORING_SYS_TRACE_ID, events);
}

int
_PyEval_SetOpcodeTrace(PyFrameObject *frame, bool enable)
{
    assert(frame != NULL);

    PyCodeObject *code = _PyFrame_GetCode(frame->f_frame);

#ifdef Py_GIL_DISABLED
    // First check if a change is necessary outside of the stop-the-world pause
    _PyMonitoringEventSet events = 0;
    if (_PyMonitoring_GetLocalEvents(code, PY_MONITORING_SYS_TRACE_ID, &events) < 0) {
        return -1;
    }
    int is_enabled = (events & (1 << PY_MONITORING_EVENT_INSTRUCTION)) != 0;
    if (is_enabled == enable) {
        return 0;  // No change needed
    }
#endif

    PyInterpreterState *interp = _PyInterpreterState_GET();
    _PyEval_StopTheWorld(interp);
    int res = set_opcode_trace_world_stopped(code, enable);
    _PyEval_StartTheWorld(interp);
    return res;
}

static PyValue
call_trace_func(_PyLegacyEventHandler *self, PyValueBorrow arg)
{
    PyThreadState *tstate = _PyThreadState_GET();
    _Py_valuetracefunc tracefunc =
        ((_PyThreadStateImpl *)tstate)->value_tracefunc;
    if (tracefunc == NULL) {
        return PyValue_None;
    }
    PyFrameObject *frame = PyEval_GetFrame();
    if (frame == NULL) {
        PyErr_SetString(PyExc_SystemError,
                        "Missing frame when calling trace function.");
        return PyValue_NULL;
    }
    if (frame->f_trace_opcodes) {
        if (_PyEval_SetOpcodeTrace(frame, true) != 0) {
            return PyValue_NULL;
        }
    }

    Py_INCREF(frame);
    int err = tracefunc(tstate->c_traceobj, frame, self->event, arg);
    frame->f_lineno = 0;
    Py_DECREF(frame);
    if (err) {
        return PyValue_NULL;
    }
    return PyValue_None;
}

static PyValue
sys_trace_exception_func(
    PyThreadState *tstate, PyObject *callable, const PyValue *args,
    size_t nargsf, PyObject *kwnames
) {
    _PyLegacyEventHandler *self = _PyLegacyEventHandler_CAST(callable);
    assert(kwnames == NULL);
    assert(PyVectorcall_NARGS(nargsf) == 3);
    PyObject *exc = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[2]));
    assert(PyExceptionInstance_Check(exc));
    PyObject *type = (PyObject *)Py_TYPE(exc);
    PyObject *tb = PyException_GetTraceback(exc);
    if (tb == NULL) {
        tb = Py_NewRef(Py_None);
    }
    PyObject *tuple = _PyTuple_PackValues(3, (PyValueBorrow[]){PyValue_FromObjectBorrow(type), PyValue_FromObjectBorrow(exc), PyValue_FromObjectBorrow(tb)});
    Py_DECREF(tb);
    if (tuple == NULL) {
        return PyValue_NULL;
    }
    PyValue res = call_trace_func(self, PyValue_FromObjectBorrow(tuple));
    Py_DECREF(tuple);
    return res;
}

static PyValue
sys_trace_start(
    PyThreadState *tstate, PyObject *callable, const PyValue *args,
    size_t nargsf, PyObject *kwnames
) {
    _PyLegacyEventHandler *self = _PyLegacyEventHandler_CAST(callable);
    assert(kwnames == NULL);
    assert(PyVectorcall_NARGS(nargsf) == 2);
    return call_trace_func(self, PyValue_Borrow(PyValue_None));
}

static PyValue
sys_trace_throw(
    PyThreadState *tstate, PyObject *callable, const PyValue *args,
    size_t nargsf, PyObject *kwnames
) {
    _PyLegacyEventHandler *self = _PyLegacyEventHandler_CAST(callable);
    assert(kwnames == NULL);
    assert(PyVectorcall_NARGS(nargsf) == 3);
    return call_trace_func(self, PyValue_Borrow(PyValue_None));
}

static PyValue
sys_trace_unwind(
    PyThreadState *tstate, PyObject *callable, const PyValue *args,
    size_t nargsf, PyObject *kwnames
) {
    _PyLegacyEventHandler *self = _PyLegacyEventHandler_CAST(callable);
    assert(kwnames == NULL);
    assert(PyVectorcall_NARGS(nargsf) == 3);
    return call_trace_func(self, PyValue_Borrow(PyValue_NULL));
}

static PyValue
sys_trace_return(
    PyThreadState *tstate, PyObject *callable, const PyValue *args,
    size_t nargsf, PyObject *kwnames
) {
    _PyLegacyEventHandler *self = _PyLegacyEventHandler_CAST(callable);
    assert(!PyErr_Occurred());
    assert(kwnames == NULL);
    assert(PyVectorcall_NARGS(nargsf) == 3);
    assert(PyCode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]))));
    return call_trace_func(self, PyValue_Borrow(args[2]));
}

static PyValue
sys_trace_yield(
    PyThreadState *tstate, PyObject *callable, const PyValue *args,
    size_t nargsf, PyObject *kwnames
) {
    _PyLegacyEventHandler *self = _PyLegacyEventHandler_CAST(callable);
    assert(kwnames == NULL);
    assert(PyVectorcall_NARGS(nargsf) == 3);
    return call_trace_func(self, PyValue_Borrow(args[2]));
}

static PyValue
sys_trace_instruction_func(
    PyThreadState *tstate, PyObject *callable, const PyValue *args,
    size_t nargsf, PyObject *kwnames
) {
    _PyLegacyEventHandler *self = _PyLegacyEventHandler_CAST(callable);
    assert(kwnames == NULL);
    assert(PyVectorcall_NARGS(nargsf) == 2);
    PyFrameObject *frame = PyEval_GetFrame();
    if (frame == NULL) {
        PyErr_SetString(PyExc_SystemError,
                        "Missing frame when calling trace function.");
        return PyValue_NULL;
    }
    _Py_valuetracefunc tracefunc =
        ((_PyThreadStateImpl *)tstate)->value_tracefunc;
    if (!tracefunc || !frame->f_trace_opcodes) {
        if (_PyEval_SetOpcodeTrace(frame, false) != 0) {
            return PyValue_NULL;
        }
        return PyValue_None;
    }
    Py_INCREF(frame);
    int err = tracefunc(tstate->c_traceobj, frame, self->event, PyValue_Borrow(PyValue_None));
    frame->f_lineno = 0;
    Py_DECREF(frame);
    if (err) {
        return PyValue_NULL;
    }
    return PyValue_None;
}

static PyValue
trace_line(
    PyThreadState *tstate, _PyLegacyEventHandler *self,
    PyFrameObject *frame, int line
) {
    if (!frame->f_trace_lines) {
        return PyValue_None;
    }
    if (line < 0) {
        return PyValue_None;
    }
    Py_INCREF(frame);
    frame->f_lineno = line;
    int err = ((_PyThreadStateImpl *)tstate)->value_tracefunc(
        tstate->c_traceobj, frame, self->event, PyValue_Borrow(PyValue_None));
    frame->f_lineno = 0;
    Py_DECREF(frame);
    if (err) {
        return PyValue_NULL;
    }
    return PyValue_None;
}

/* Read an offset or line argument. The interpreter passes inline ints, but
 * PyMonitoring_Fire*Event callers can pass any int object. */
static int
offset_from_word(PyValueBorrow word, int *out)
{
    if (PyValue_IsInlineInt(word)) {
        *out = (int)PyValue_InlineInt64(word);
        return 0;
    }
    int value = _PyValue_AsInt(word);
    if (value == -1 && PyErr_Occurred()) {
        return -1;
    }
    *out = value;
    return 0;
}

static PyValue
sys_trace_stop_iteration_func(
    PyThreadState *tstate, PyObject *callable, const PyValue *args,
    size_t nargsf, PyObject *kwnames
) {
    assert(kwnames == NULL);
    assert(PyVectorcall_NARGS(nargsf) == 3);
    PyValueBorrow code_arg = PyValue_Borrow(args[0]);
    PyValueBorrow offset_arg = PyValue_Borrow(args[1]);
    PyValueBorrow exception_arg = PyValue_Borrow(args[2]);
    /* STOP is a semantic completion notification. The legacy API omits
     * implicit valueless FOR exhaustion; actual RAISE events remain intact.
     * Use the immutable event site, not a callback-redirected frame PC. */
    if (PyValue_IsObject(code_arg) && PyValue_IsInlineInt(offset_arg) &&
        PyValue_IsObject(exception_arg))
    {
        PyObject *code_obj = PyValue_AsKnownObjectBorrow(code_arg);
        PyObject *exc = PyValue_AsKnownObjectBorrow(exception_arg);
        int64_t offset = PyValue_InlineInt64(offset_arg);
        if (PyCode_Check(code_obj) && Py_IS_TYPE(exc, (PyTypeObject *)PyExc_StopIteration)) {
            PyCodeObject *code = (PyCodeObject *)code_obj;
            if (offset >= 0 && offset < Py_SIZE(code) &&
                _PyIR_BaseOpcode(_PyCode_InstrHandler(code, (int)offset)) == FOR_ITER)
            {
                PyValue value = ((PyStopIterationObject *)exc)->value;
                if (PyValue_IsNull(value) || PyValue_IsNone(value)) {
                    return PyValue_None;
                }
            }
        }
    }
    return sys_trace_exception_func(tstate, callable, args, nargsf, kwnames);
}

static PyValue
sys_trace_line_func(
    PyThreadState *tstate, PyObject *callable, const PyValue *args,
    size_t nargsf, PyObject *kwnames
) {
    _PyLegacyEventHandler *self = _PyLegacyEventHandler_CAST(callable);
    assert(kwnames == NULL);
    if (((_PyThreadStateImpl *)tstate)->value_tracefunc == NULL) {
        return PyValue_None;
    }
    assert(PyVectorcall_NARGS(nargsf) == 2);
    int line;
    if (offset_from_word(PyValue_Borrow(args[1]), &line) < 0) {
        return PyValue_NULL;
    }
    assert(line >= 0);
    PyFrameObject *frame = PyEval_GetFrame();
    if (frame == NULL) {
        PyErr_SetString(PyExc_SystemError,
                        "Missing frame when calling trace function.");
        return PyValue_NULL;
    }
    assert(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]))
           == (PyObject *)_PyFrame_GetCode(frame->f_frame));
    return trace_line(tstate, self, frame, line);
}

/* sys.settrace generates line events for all backward
 * edges, even if on the same line.
 * Handle that case here */
static PyValue
sys_trace_jump_func(
    PyThreadState *tstate, PyObject *callable, const PyValue *args,
    size_t nargsf, PyObject *kwnames
) {
    _PyLegacyEventHandler *self = _PyLegacyEventHandler_CAST(callable);
    assert(kwnames == NULL);
    if (((_PyThreadStateImpl *)tstate)->value_tracefunc == NULL) {
        return PyValue_None;
    }
    assert(PyVectorcall_NARGS(nargsf) == 3);
    int from, to;
    if (offset_from_word(PyValue_Borrow(args[1]), &from) < 0 ||
        offset_from_word(PyValue_Borrow(args[2]), &to) < 0) {
        return PyValue_NULL;
    }
    assert(from >= 0);
    assert(to >= 0);
    /* Immortal: the borrowed word is a valid result. */
    PyValue disable;
    _PyValue_WindowPut(&disable,
                       PyValue_FromObjectBorrow(&_PyInstrumentation_DISABLE));
    if (to > from) {
        /* Forward jump */
        return disable;
    }
    PyCodeObject *code = (PyCodeObject *)PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    assert(PyCode_Check(code));
    /* We can call _Py_Instrumentation_GetLine because we always set
    * line events for tracing */
    int to_line = _Py_Instrumentation_GetLine(code, code->_co_monitoring->lines, to);
    int from_line = _Py_Instrumentation_GetLine(code, code->_co_monitoring->lines, from);
    if (to_line != from_line) {
        /* Will be handled by target INSTRUMENTED_LINE */
        return disable;
    }
    _PyIRObservationMap source = _PyIRObservation_Stored(code);
    for (uint32_t i = 0; i < source.nrecords; i++) {
        _PyIRObservationEvent edge;
        _PyIRObservation_Event(&source, i, &edge);
        if (edge.from == from && edge.to == to &&
            edge.edge == _PyIR_EDGE_TAKEN && edge.line >= 0 &&
            edge.line != from_line)
        {
            /* The compact backedge crosses a source-only loop header.
             * Its source events and landing LINE supply the iteration;
             * emitting the real target here would precede that header. */
            return disable;
        }
    }
    PyFrameObject *frame = PyEval_GetFrame();
    if (frame == NULL) {
        PyErr_SetString(PyExc_SystemError,
                        "Missing frame when calling trace function.");
        return PyValue_NULL;
    }
    if (!frame->f_trace_lines) {
        return PyValue_None;
    }
    return trace_line(tstate, self, frame, to_line);
}

PyTypeObject _PyLegacyEventHandler_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "sys.legacy_event_handler",
    sizeof(_PyLegacyEventHandler),
    .tp_vectorcall_offset = offsetof(_PyLegacyEventHandler, vectorcall),
    .tp_value_vectorcall_offset =
        offsetof(_PyLegacyEventHandler, value_vectorcall),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
        Py_TPFLAGS_HAVE_VECTORCALL | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    .tp_call = PyVectorcall_Call,
};

static int
set_callbacks(int tool, _Py_vectorcallvaluefunc vectorcall, int legacy_event, int event1, int event2)
{
    _PyLegacyEventHandler *callback =
        PyObject_NEW(_PyLegacyEventHandler, &_PyLegacyEventHandler_Type);
    if (callback == NULL) {
        return -1;
    }
    callback->vectorcall = _PyValue_VectorcallThunk;
    callback->value_vectorcall = vectorcall;
    callback->event = legacy_event;
    Py_XDECREF(_PyMonitoring_RegisterCallback(tool, event1, (PyObject *)callback));
    if (event2 >= 0) {
        Py_XDECREF(_PyMonitoring_RegisterCallback(tool, event2, (PyObject *)callback));
    }
    Py_DECREF(callback);
    return 0;
}

#ifndef NDEBUG
/* Ensure that tstate is valid: sanity check for PyEval_AcquireThread() and
   PyEval_RestoreThread(). Detect if tstate memory was freed. It can happen
   when a thread continues to run after Python finalization, especially
   daemon threads. */
static int
is_tstate_valid(PyThreadState *tstate)
{
    assert(!_PyMem_IsPtrFreed(tstate));
    assert(!_PyMem_IsPtrFreed(tstate->interp));
    return 1;
}
#endif

static int
setup_profile_callbacks(void *Py_UNUSED(arg))
{
    /* Setup PEP 669 monitoring callbacks and events. */
    if (set_callbacks(PY_MONITORING_SYS_PROFILE_ID,
                      sys_profile_start, PyTrace_CALL,
                      PY_MONITORING_EVENT_PY_START,
                      PY_MONITORING_EVENT_PY_RESUME)) {
        return -1;
    }
    if (set_callbacks(PY_MONITORING_SYS_PROFILE_ID,
                      sys_profile_throw, PyTrace_CALL,
                      PY_MONITORING_EVENT_PY_THROW, -1)) {
        return -1;
    }
    if (set_callbacks(PY_MONITORING_SYS_PROFILE_ID,
                      sys_profile_return, PyTrace_RETURN,
                      PY_MONITORING_EVENT_PY_RETURN,
                      PY_MONITORING_EVENT_PY_YIELD)) {
        return -1;
    }
    if (set_callbacks(PY_MONITORING_SYS_PROFILE_ID,
                      sys_profile_unwind, PyTrace_RETURN,
                      PY_MONITORING_EVENT_PY_UNWIND, -1)) {
        return -1;
    }
    if (set_callbacks(PY_MONITORING_SYS_PROFILE_ID,
                      sys_profile_call_or_return, PyTrace_C_CALL,
                      PY_MONITORING_EVENT_CALL, -1)) {
        return -1;
    }
    if (set_callbacks(PY_MONITORING_SYS_PROFILE_ID,
                      sys_profile_call_or_return, PyTrace_C_RETURN,
                      PY_MONITORING_EVENT_C_RETURN, -1)) {
        return -1;
    }
    if (set_callbacks(PY_MONITORING_SYS_PROFILE_ID,
                      sys_profile_call_or_return, PyTrace_C_EXCEPTION,
                      PY_MONITORING_EVENT_C_RAISE, -1)) {
        return -1;
    }
    return 0;
}

/* The value trace function of an extension's Py_tracefunc: box the argument
 * word and call the function. The function is read from the thread state
 * before the call, because the callee may clear it. */
static int
call_object_tracefunc(Py_tracefunc func, PyObject *obj, PyFrameObject *frame,
                      int what, PyValueBorrow arg)
{
    if (PyValue_IsNull(arg)) {
        return func(obj, frame, what, NULL);
    }
    if (PyValue_IsObject(arg)) {
        return func(obj, frame, what, PyValue_AsKnownObjectBorrow(arg));
    }
    PyObject *boxed = _PyValue_AsObjectFor(
        arg, _PyBox_AT(BOX_SITE_CALL_ADAPTER, BOX_KIND_CALL_ADAPTER,
                       _Py_FUNC_ADDR(func)));
    if (boxed == NULL) {
        return -1;
    }
    int err = func(obj, frame, what, boxed);
    Py_DECREF(boxed);
    return err;
}

static int
trace_adapter(PyObject *obj, PyFrameObject *frame, int what, PyValueBorrow arg)
{
    Py_tracefunc func = _PyThreadState_GET()->c_tracefunc;
    return call_object_tracefunc(func, obj, frame, what, arg);
}

static int
profile_adapter(PyObject *obj, PyFrameObject *frame, int what, PyValueBorrow arg)
{
    Py_tracefunc func = _PyThreadState_GET()->c_profilefunc;
    return call_object_tracefunc(func, obj, frame, what, arg);
}

static PyObject *
swap_profile_func_arg(PyThreadState *tstate, _Py_valuetracefunc vfunc,
                      Py_tracefunc ofunc, PyObject *arg)
{
    assert((vfunc != NULL) == (ofunc != NULL));
    int delta = (ofunc != NULL) - (tstate->c_profilefunc != NULL);
    tstate->c_profilefunc = ofunc;
    ((_PyThreadStateImpl *)tstate)->value_profilefunc = vfunc;
    PyObject *old_profileobj = tstate->c_profileobj;
    tstate->c_profileobj = Py_XNewRef(arg);
    tstate->interp->sys_profiling_threads += delta;
    assert(tstate->interp->sys_profiling_threads >= 0);
    return old_profileobj;
}

static int
set_monitoring_profile_events(PyInterpreterState *interp)
{
    uint32_t events = 0;
    if (interp->sys_profiling_threads) {
        events =
            (1 << PY_MONITORING_EVENT_PY_START) | (1 << PY_MONITORING_EVENT_PY_RESUME) |
            (1 << PY_MONITORING_EVENT_PY_RETURN) | (1 << PY_MONITORING_EVENT_PY_YIELD) |
            (1 << PY_MONITORING_EVENT_CALL) | (1 << PY_MONITORING_EVENT_PY_UNWIND) |
            (1 << PY_MONITORING_EVENT_PY_THROW);
    }
    return _PyMonitoring_SetEvents(PY_MONITORING_SYS_PROFILE_ID, events);
}

int
_PyEval_SetProfileValue(PyThreadState *tstate, _Py_valuetracefunc vfunc,
                          Py_tracefunc ofunc, PyObject *arg)
{
    assert(is_tstate_valid(tstate));
    /* The caller must hold a thread state */
    _Py_AssertHoldsTstate();

    /* Call _PySys_Audit() in the context of the current thread state,
       even if tstate is not the current thread state. */
    PyThreadState *current_tstate = _PyThreadState_GET();
    if (_PySys_Audit(current_tstate, "sys.setprofile", NULL) < 0) {
        return -1;
    }

    PyInterpreterState *interp = tstate->interp;
    if (_PyOnceFlag_CallOnce(&interp->sys_profile_once_flag,
                             setup_profile_callbacks, NULL) < 0) {
        return -1;
    }

    _PyEval_StopTheWorld(interp);
    PyObject *old_profileobj = swap_profile_func_arg(tstate, vfunc, ofunc, arg);
    int ret = set_monitoring_profile_events(interp);
    _PyEval_StartTheWorld(interp);
    Py_XDECREF(old_profileobj);  // needs to be decref'd outside of stop-the-world
    return ret;
}

int
_PyEval_SetProfileValueAllThreads(PyInterpreterState *interp,
                                   _Py_valuetracefunc vfunc, Py_tracefunc ofunc,
                                   PyObject *arg)
{
    PyThreadState *current_tstate = _PyThreadState_GET();
    assert(is_tstate_valid(current_tstate));
    assert(current_tstate->interp == interp);

    if (_PySys_Audit(current_tstate, "sys.setprofile", NULL) < 0) {
        return -1;
    }

    if (_PyOnceFlag_CallOnce(&interp->sys_profile_once_flag,
                             setup_profile_callbacks, NULL) < 0) {
        return -1;
    }

    PyObject *old_profileobjs = NULL;
    _PyEval_StopTheWorld(interp);
    HEAD_LOCK(&_PyRuntime);
    Py_ssize_t num_thread_states = 0;
    _Py_FOR_EACH_TSTATE_UNLOCKED(interp, p) {
        num_thread_states++;
    }
    old_profileobjs = PyTuple_New(num_thread_states);
    if (old_profileobjs == NULL) {
        HEAD_UNLOCK(&_PyRuntime);
        _PyEval_StartTheWorld(interp);
        return -1;
    }
    _Py_FOR_EACH_TSTATE_UNLOCKED(interp, tstate) {
        PyObject *old = swap_profile_func_arg(tstate, vfunc, ofunc, arg);
        PyTuple_SET_VALUE(old_profileobjs, --num_thread_states, PyValue_PrepareOwned(old));
    }
    HEAD_UNLOCK(&_PyRuntime);
    int ret = set_monitoring_profile_events(interp);
    _PyEval_StartTheWorld(interp);
    Py_XDECREF(old_profileobjs);  // needs to be decref'd outside of stop-the-world
    return ret;
}

static int
setup_trace_callbacks(void *Py_UNUSED(arg))
{
    /* Setup PEP 669 monitoring callbacks and events. */
    if (set_callbacks(PY_MONITORING_SYS_TRACE_ID,
                      sys_trace_start, PyTrace_CALL,
                      PY_MONITORING_EVENT_PY_START,
                      PY_MONITORING_EVENT_PY_RESUME)) {
        return -1;
    }
    if (set_callbacks(PY_MONITORING_SYS_TRACE_ID,
                      sys_trace_throw, PyTrace_CALL,
                      PY_MONITORING_EVENT_PY_THROW, -1)) {
        return -1;
    }
    if (set_callbacks(PY_MONITORING_SYS_TRACE_ID,
                      sys_trace_return, PyTrace_RETURN,
                      PY_MONITORING_EVENT_PY_RETURN, -1)) {
        return -1;
    }
    if (set_callbacks(PY_MONITORING_SYS_TRACE_ID,
                      sys_trace_yield, PyTrace_RETURN,
                      PY_MONITORING_EVENT_PY_YIELD, -1)) {
        return -1;
    }
    if (set_callbacks(PY_MONITORING_SYS_TRACE_ID,
                      sys_trace_exception_func, PyTrace_EXCEPTION,
                      PY_MONITORING_EVENT_RAISE, -1)) {
        return -1;
    }
    if (set_callbacks(PY_MONITORING_SYS_TRACE_ID,
                      sys_trace_stop_iteration_func, PyTrace_EXCEPTION,
                      PY_MONITORING_EVENT_STOP_ITERATION, -1)) {
        return -1;
    }
    if (set_callbacks(PY_MONITORING_SYS_TRACE_ID,
                      sys_trace_line_func, PyTrace_LINE,
                      PY_MONITORING_EVENT_LINE, -1)) {
        return -1;
    }
    if (set_callbacks(PY_MONITORING_SYS_TRACE_ID,
                      sys_trace_unwind, PyTrace_RETURN,
                      PY_MONITORING_EVENT_PY_UNWIND, -1)) {
        return -1;
    }
    if (set_callbacks(PY_MONITORING_SYS_TRACE_ID,
                      sys_trace_jump_func, PyTrace_LINE,
                      PY_MONITORING_EVENT_JUMP, -1)) {
        return -1;
    }
    if (set_callbacks(PY_MONITORING_SYS_TRACE_ID,
                      sys_trace_instruction_func, PyTrace_OPCODE,
                      PY_MONITORING_EVENT_INSTRUCTION, -1)) {
        return -1;
    }
    return 0;
}

static PyObject *
swap_trace_func_arg(PyThreadState *tstate, _Py_valuetracefunc vfunc,
                    Py_tracefunc ofunc, PyObject *arg)
{
    assert((vfunc != NULL) == (ofunc != NULL));
    int delta = (ofunc != NULL) - (tstate->c_tracefunc != NULL);
    tstate->c_tracefunc = ofunc;
    ((_PyThreadStateImpl *)tstate)->value_tracefunc = vfunc;
    PyObject *old_traceobj = tstate->c_traceobj;
    tstate->c_traceobj = Py_XNewRef(arg);
    tstate->interp->sys_tracing_threads += delta;
    assert(tstate->interp->sys_tracing_threads >= 0);
    return old_traceobj;
}

static int
set_monitoring_trace_events(PyInterpreterState *interp)
{
    uint32_t events = 0;
    if (interp->sys_tracing_threads) {
        events =
            (1 << PY_MONITORING_EVENT_PY_START) | (1 << PY_MONITORING_EVENT_PY_RESUME) |
            (1 << PY_MONITORING_EVENT_PY_RETURN) | (1 << PY_MONITORING_EVENT_PY_YIELD) |
            (1 << PY_MONITORING_EVENT_RAISE) | (1 << PY_MONITORING_EVENT_LINE) |
            (1 << PY_MONITORING_EVENT_JUMP) |
            (1 << PY_MONITORING_EVENT_PY_UNWIND) | (1 << PY_MONITORING_EVENT_PY_THROW) |
            (1 << PY_MONITORING_EVENT_STOP_ITERATION);
    }
    return _PyMonitoring_SetEvents(PY_MONITORING_SYS_TRACE_ID, events);
}

// Enable opcode tracing for the thread's current frame if needed.
static int
maybe_set_opcode_trace(PyThreadState *tstate)
{
    _PyInterpreterFrame *iframe = tstate->current_frame;
    if (iframe == NULL) {
        return 0;
    }
    PyFrameObject *frame = iframe->frame_obj;
    if (frame == NULL || !frame->f_trace_opcodes) {
        return 0;
    }
    return set_opcode_trace_world_stopped(_PyFrame_GetCode(iframe), true);
}

int
_PyEval_SetTraceValue(PyThreadState *tstate, _Py_valuetracefunc vfunc,
                        Py_tracefunc ofunc, PyObject *arg)
{
    assert(is_tstate_valid(tstate));
    /* The caller must hold a thread state */
    _Py_AssertHoldsTstate();

    /* Call _PySys_Audit() in the context of the current thread state,
       even if tstate is not the current thread state. */
    PyThreadState *current_tstate = _PyThreadState_GET();
    if (_PySys_Audit(current_tstate, "sys.settrace", NULL) < 0) {
        return -1;
    }

    PyInterpreterState *interp = tstate->interp;
    if (_PyOnceFlag_CallOnce(&interp->sys_trace_once_flag,
                             setup_trace_callbacks, NULL) < 0) {
        return -1;
    }

    int err = 0;
    _PyEval_StopTheWorld(interp);
    PyObject *old_traceobj = swap_trace_func_arg(tstate, vfunc, ofunc, arg);
    err = set_monitoring_trace_events(interp);
    if (err != 0) {
        goto done;
    }
    if (interp->sys_tracing_threads) {
        err = maybe_set_opcode_trace(tstate);
    }
done:
    _PyEval_StartTheWorld(interp);
    Py_XDECREF(old_traceobj);  // needs to be decref'd outside stop-the-world
    return err;
}

int
_PyEval_SetTraceValueAllThreads(PyInterpreterState *interp,
                                 _Py_valuetracefunc vfunc, Py_tracefunc ofunc,
                                 PyObject *arg)
{
    PyThreadState *current_tstate = _PyThreadState_GET();
    assert(is_tstate_valid(current_tstate));
    assert(current_tstate->interp == interp);

    if (_PySys_Audit(current_tstate, "sys.settrace", NULL) < 0) {
        return -1;
    }

    if (_PyOnceFlag_CallOnce(&interp->sys_trace_once_flag,
                             setup_trace_callbacks, NULL) < 0) {
        return -1;
    }

    PyObject *old_trace_objs = NULL;
    _PyEval_StopTheWorld(interp);
    HEAD_LOCK(&_PyRuntime);
    Py_ssize_t num_thread_states = 0;
    _Py_FOR_EACH_TSTATE_UNLOCKED(interp, p) {
        num_thread_states++;
    }
    old_trace_objs = PyTuple_New(num_thread_states);
    if (old_trace_objs == NULL) {
        HEAD_UNLOCK(&_PyRuntime);
        _PyEval_StartTheWorld(interp);
        return -1;
    }
    _Py_FOR_EACH_TSTATE_UNLOCKED(interp, tstate) {
        PyObject *old = swap_trace_func_arg(tstate, vfunc, ofunc, arg);
        PyTuple_SET_VALUE(old_trace_objs, --num_thread_states, PyValue_PrepareOwned(old));
    }
    if (interp->sys_tracing_threads) {
        _Py_FOR_EACH_TSTATE_UNLOCKED(interp, tstate) {
            int err = maybe_set_opcode_trace(tstate);
            if (err != 0) {
                HEAD_UNLOCK(&_PyRuntime);
                _PyEval_StartTheWorld(interp);
                Py_XDECREF(old_trace_objs);
                return -1;
            }
        }
    }
    HEAD_UNLOCK(&_PyRuntime);
    int err = set_monitoring_trace_events(interp);
    _PyEval_StartTheWorld(interp);
    Py_XDECREF(old_trace_objs);  // needs to be decref'd outside of stop-the-world
    return err;
}

int
_PyEval_SetProfile(PyThreadState *tstate, Py_tracefunc func, PyObject *arg)
{
    return _PyEval_SetProfileValue(
        tstate, func ? profile_adapter : NULL, func, arg);
}

int
_PyEval_SetProfileAllThreads(PyInterpreterState *interp, Py_tracefunc func, PyObject *arg)
{
    return _PyEval_SetProfileValueAllThreads(
        interp, func ? profile_adapter : NULL, func, arg);
}

int
_PyEval_SetTrace(PyThreadState *tstate, Py_tracefunc func, PyObject *arg)
{
    return _PyEval_SetTraceValue(
        tstate, func ? trace_adapter : NULL, func, arg);
}

int
_PyEval_SetTraceAllThreads(PyInterpreterState *interp, Py_tracefunc func, PyObject *arg)
{
    return _PyEval_SetTraceValueAllThreads(
        interp, func ? trace_adapter : NULL, func, arg);
}
