#define _PY_INTERPRETER

#include "Python.h"
#include "pycore_abstract.h"      // _PyIndex_Check()
#include "pycore_audit.h"         // _PySys_Audit()
#include "pycore_backoff.h"
#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_cell.h"          // PyCell_GetRef()
#include "pycore_ceval.h"         // SPECIAL___ENTER__
#include "pycore_code.h"
#include "pycore_dict.h"
#include "pycore_emscripten_signal.h"  // _Py_CHECK_EMSCRIPTEN_SIGNALS
#include "pycore_floatobject.h"   // _PyFloat_ExactDealloc()
#include "pycore_frame.h"
#include "pycore_function.h"
#include "pycore_genobject.h"     // _PyCoro_GetAwaitableIter()
#include "pycore_gc_trace.h"      // _PyGC_CHECK_NOT_SWEEPING()
#include "pycore_import.h"        // _PyImport_IsDefaultImportFunc()
#include "pycore_instruments.h"
#include "pycore_interpframe.h"   // _PyFrame_Slots()
#include "pycore_interpolation.h" // _PyInterpolation_Build()
#include "pycore_intrinsics.h"
#include "pycore_lazyimportobject.h"
#include "pycore_list.h"          // _PyList_GetItemRef()
#include "pycore_long.h"          // _PyLong_GetZero()
#include "pycore_moduleobject.h"  // PyModuleObject
#include "pycore_object.h"        // _PyObject_GC_TRACK()
#include "pycore_opcode_metadata.h" // EXTRA_CASES
#include "pycore_opcode_utils.h"  // MAKE_FUNCTION_*
#include "pycore_pyatomic_ft_wrappers.h" // FT_ATOMIC_*
#include "pycore_pyerrors.h"      // _PyErr_GetRaisedException()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_range.h"         // _PyRangeIterObject
#include "pycore_setobject.h"     // _PySet_Update()
#include "pycore_sliceobject.h"   // _PySlice_FromValues()
#include "pycore_sysmodule.h"     // _PySys_GetOptionalAttrString()
#include "pycore_template.h"      // _PyTemplate_Build()
#include "pycore_traceback.h"     // _PyTraceBack_FromFrame
#include "pycore_tuple.h"         // _PyTuple_ITEMS()

#include "dictobject.h"
#include "frameobject.h"          // _PyInterpreterFrame_GetLine
#include "opcode.h"
#include "pydtrace.h"
#include "setobject.h"
#include "pycore_value.h"

#include <stdbool.h>              // bool

#if !defined(Py_BUILD_CORE)
#  error "ceval.c must be build with Py_BUILD_CORE define for best performance"
#endif

#if !defined(Py_DEBUG)
// GH-89279: The MSVC compiler does not inline these static inline functions
// in PGO build in _PyEval_EvalFrameDefault(), because this function is over
// the limit of PGO, and that limit cannot be configured.
// Define them as macros to make sure that they are always inlined by the
// preprocessor.

#undef Py_IS_TYPE
#define Py_IS_TYPE(ob, tp) (_PyObject_SHAPE(ob)->type == (tp))

#endif

static void
check_invalid_reentrancy(void)
{
}


#ifdef Py_DEBUG
static void
dump_item(PyValue item)
{
    if (PyValue_IsNull(item)) {
        printf("<NULL>");
        return;
    }
    if (_PyValue_IsMalformed(item)) {
        printf("<INVALID>");
        return;
    }
    if (_PyValue_IsTaggedIndex(item)) {
        printf("%" PRId64, (int64_t)_PyValue_UntagIndex(item));
        return;
    }
    if (PyValue_IsInline(item)) {
        printf("<inline %s>", PyValue_Type(item)->tp_name);
        return;
    }
    PyObject *obj = PyValue_ObjectBorrow(item);
    if (obj == NULL) {
        printf("<nil>");
        return;
    }
    // Don't call __repr__(), it might recurse into the interpreter.
    printf("<%s at %p>", Py_TYPE(obj)->tp_name, (void *)obj);
}

/* The locals, then the temporaries, of which NULL ones are dead */
static void
dump_stack(_PyInterpreterFrame *frame)
{
    PyValue *slots = _PyFrame_Slots(frame);
    int nlocalsplus = _PyFrame_GetCode(frame)->co_nlocalsplus;
    int nslots = _PyFrame_NumSlots(frame);
    PyObject *exc = PyErr_GetRaisedException();
    printf("    locals=[");
    for (int i = 0; i < nlocalsplus; i++) {
        if (i) {
            printf(", ");
        }
        dump_item(slots[i]);
    }
    printf("]\n");
    printf("    stack=[");
    for (int i = nlocalsplus; i < nslots; i++) {
        if (i != nlocalsplus) {
            printf(", ");
        }
        dump_item(slots[i]);
    }
    printf("]\n");
    fflush(stdout);
    PyErr_SetRaisedException(exc);
}

static void
lltrace_instruction(_PyInterpreterFrame *frame,
                    _PyIRWord *next_instr,
                    int opcode)
{
    _PyIRWord word = *next_instr;
    uint32_t a = IR_A(word), b = IR_B(word), c = IR_C(word), d = IR_D(word);
    const char *name = opcode < IR_FIRST_ONLY_HANDLER
        ? _PyOpcode_OpName[opcode]
        : _PyIR_OnlyNames[opcode - IR_FIRST_ONLY_HANDLER];
    assert(name != NULL);
    if (frame->owner >= FRAME_OWNED_BY_INTERPRETER) {
        printf("0: %s\n    ir %s %u %u %u %u\n", name, name, a, b, c, d);
        fflush(stdout);
        return;
    }
    dump_stack(frame);
    _PyFrame_CheckValues(frame);
    /* First the word index and the instruction that the handler stands
     * for, with its oparg; then the handler and its fields, on a line
     * without a colon. */
    int offset = _PyFrame_InstrOffset(frame, next_instr);
    int shown = _PyIR_BaseOpcode(_PyIR_RealHandler(frame, next_instr));
    int oparg = _PyCode_InstrOparg(_PyFrame_GetCode(frame), offset);
    if (OPCODE_HAS_ARG(shown)) {
        printf("%d: %s %d\n", offset, _PyOpcode_OpName[shown], oparg);
    }
    else {
        printf("%d: %s\n", offset, _PyOpcode_OpName[shown]);
    }
    printf("    ir %s %u %u %u %u\n", name, a, b, c, d);
    fflush(stdout);
}

static void
lltrace_resume_frame(_PyInterpreterFrame *frame)
{
    PyObject *fobj = PyValue_ObjectBorrow(frame->f_funcobj);
    if (!(PyValue_Type(frame->f_executable) == &PyCode_Type) ||
        fobj == NULL ||
        !PyFunction_Check(fobj)
    ) {
        printf("\nResuming frame.\n");
        return;
    }
    PyFunctionObject *f = (PyFunctionObject *)fobj;
    PyObject *exc = PyErr_GetRaisedException();
    PyObject *name = Py_XNewRef(FT_ATOMIC_LOAD_PTR_ACQUIRE(f->func_qualname));
    if (name == NULL) {
        name = Py_XNewRef(FT_ATOMIC_LOAD_PTR_ACQUIRE(f->func_name));
    }
    printf("\nResuming frame");
    if (name) {
        printf(" for ");
        if (PyObject_Print(name, stdout, 0) < 0) {
            PyErr_Clear();
        }
        Py_DECREF(name);
    }
    PyObject *module = Py_XNewRef(FT_ATOMIC_LOAD_PTR_ACQUIRE(f->func_module));
    if (module) {
        printf(" in module ");
        if (PyObject_Print(module, stdout, 0) < 0) {
            PyErr_Clear();
        }
        Py_DECREF(module);
    }
    printf("\n");
    fflush(stdout);
    PyErr_SetRaisedException(exc);
}

static int
maybe_lltrace_resume_frame(_PyInterpreterFrame *frame, PyObject *globals)
{
    if (globals == NULL) {
        return 0;
    }
    if (frame->owner >= FRAME_OWNED_BY_INTERPRETER) {
        return 0;
    }
    int r = PyDict_Contains(globals, &_Py_ID(__lltrace__));
    if (r < 0) {
        PyErr_Clear();
        return 0;
    }
    int lltrace = r * 5;  // Levels 1-4 do not trace instructions
    if (!lltrace) {
        // Can also be controlled by environment variable
        char *python_lltrace = Py_GETENV("PYTHON_LLTRACE");
        if (python_lltrace != NULL && *python_lltrace >= '0') {
            lltrace = *python_lltrace - '0';  // TODO: Parse an int and all that
        }
    }
    if (lltrace >= 5) {
        lltrace_resume_frame(frame);
    }
    return lltrace;
}

#endif

static void monitor_reraise(PyThreadState *tstate,
                 _PyInterpreterFrame *frame,
                 _PyInstr *instr);
static int monitor_stop_iteration(PyThreadState *tstate,
                 _PyInterpreterFrame *frame,
                 _PyInstr *instr,
                 PyObject *value);
static void monitor_unwind(PyThreadState *tstate,
                 _PyInterpreterFrame *frame,
                 _PyInstr *instr);
static int monitor_handled(PyThreadState *tstate,
                 _PyInterpreterFrame *frame,
                 _PyInstr *instr, PyObject *exc);
static void monitor_throw(PyThreadState *tstate,
                 _PyInterpreterFrame *frame,
                 _PyInstr *instr);

static int get_exception_handler(PyCodeObject *, int, int*, int*, int*);

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

typedef struct {
    _PyInterpreterFrame frame;
    PyValue stack[1];
} _PyEntryFrame;

static int
do_monitor_exc(PyThreadState *tstate, _PyInterpreterFrame *frame,
               _PyInstr *instr, int event)
{
    assert(event < _PY_MONITORING_UNGROUPED_EVENTS);
    if (_PyFrame_GetCode(frame)->co_flags & CO_NO_MONITORING_EVENTS) {
        return 0;
    }
    PyObject *exc = PyErr_GetRaisedException();
    assert(exc != NULL);
    int err = _Py_call_instrumentation_arg(tstate, event, frame, instr, exc);
    if (err == 0) {
        PyErr_SetRaisedException(exc);
    }
    else {
        assert(PyErr_Occurred());
        Py_DECREF(exc);
    }
    return err;
}

static inline bool
no_tools_for_global_event(PyThreadState *tstate, int event)
{
    return tstate->interp->monitors.tools[event] == 0;
}

static inline bool
no_tools_for_local_event(PyThreadState *tstate, _PyInterpreterFrame *frame, int event)
{
    assert(event < _PY_MONITORING_UNGROUPED_EVENTS);
    _PyCoMonitoringData *data = _PyFrame_GetCode(frame)->_co_monitoring;
    if (data) {
        return data->active_monitors.tools[event] == 0;
    }
    else {
        return no_tools_for_global_event(tstate, event);
    }
}

static Py_NO_INLINE int
monitor_handled(PyThreadState *tstate,
                _PyInterpreterFrame *frame,
                _PyInstr *instr, PyObject *exc)
{
    if (no_tools_for_local_event(tstate, frame, PY_MONITORING_EVENT_EXCEPTION_HANDLED)) {
        return 0;
    }
    return _Py_call_instrumentation_arg(tstate, PY_MONITORING_EVENT_EXCEPTION_HANDLED, frame, instr, exc);
}

static Py_NO_INLINE void
monitor_throw(PyThreadState *tstate,
              _PyInterpreterFrame *frame,
              _PyInstr *instr)
{
    if (no_tools_for_local_event(tstate, frame, PY_MONITORING_EVENT_PY_THROW)) {
        return;
    }
    do_monitor_exc(tstate, frame, instr, PY_MONITORING_EVENT_PY_THROW);
}

static Py_NO_INLINE void
monitor_reraise(PyThreadState *tstate, _PyInterpreterFrame *frame,
              _PyInstr *instr)
{
    if (no_tools_for_local_event(tstate, frame, PY_MONITORING_EVENT_RERAISE)) {
        return;
    }
    do_monitor_exc(tstate, frame, instr, PY_MONITORING_EVENT_RERAISE);
}

static Py_NO_INLINE int
monitor_stop_iteration(PyThreadState *tstate, _PyInterpreterFrame *frame,
                       _PyInstr *instr, PyObject *value)
{
    if (no_tools_for_local_event(tstate, frame, PY_MONITORING_EVENT_STOP_ITERATION)) {
        return 0;
    }
    assert(!PyErr_Occurred());
    PyErr_SetObject(PyExc_StopIteration, value);
    int res = do_monitor_exc(tstate, frame, instr, PY_MONITORING_EVENT_STOP_ITERATION);
    if (res < 0) {
        return res;
    }
    PyErr_SetRaisedException(NULL);
    return 0;
}

static Py_NO_INLINE void
monitor_unwind(PyThreadState *tstate,
               _PyInterpreterFrame *frame,
               _PyInstr *instr)
{
    if (no_tools_for_local_event(tstate, frame, PY_MONITORING_EVENT_PY_UNWIND)) {
        return;
    }
    do_monitor_exc(tstate, frame, instr, PY_MONITORING_EVENT_PY_UNWIND);
}

static inline unsigned char *
scan_back_to_entry_start(unsigned char *p) {
    for (; (p[0]&128) == 0; p--);
    return p;
}

static inline unsigned char *
skip_to_next_entry(unsigned char *p, unsigned char *end) {
    while (p < end && ((p[0] & 128) == 0)) {
        p++;
    }
    return p;
}


#define MAX_LINEAR_SEARCH 40

static Py_NO_INLINE int
get_exception_handler(PyCodeObject *code, int index, int *exc_slot, int *handler, int *lasti)
{
    unsigned char *start = (unsigned char *)PyBytes_AS_STRING(code->co_exceptiontable);
    unsigned char *end = start + PyBytes_GET_SIZE(code->co_exceptiontable);
    /* Invariants:
     * start_table == end_table OR
     * start_table points to a legal entry and end_table points
     * beyond the table or to a legal entry that is after index.
     */
    if (end - start > MAX_LINEAR_SEARCH) {
        int offset;
        parse_varint(start, &offset);
        if (offset > index) {
            return 0;
        }
        do {
            unsigned char * mid = start + ((end-start)>>1);
            mid = scan_back_to_entry_start(mid);
            parse_varint(mid, &offset);
            if (offset > index) {
                end = mid;
            }
            else {
                start = mid;
            }

        } while (end - start > MAX_LINEAR_SEARCH);
    }
    unsigned char *scan = start;
    while (scan < end) {
        int start_offset, size;
        scan = parse_varint(scan, &start_offset);
        if (start_offset > index) {
            break;
        }
        scan = parse_varint(scan, &size);
        if (start_offset + size > index) {
            scan = parse_varint(scan, handler);
            int slot_and_lasti;
            parse_varint(scan, &slot_and_lasti);
            *exc_slot = slot_and_lasti >> 1;
            *lasti = slot_and_lasti & 1;
            return 1;
        }
        scan = skip_to_next_entry(scan, end);
    }
    return 0;
}



/* Logic for the raise statement (too complicated for inlining).
   This *consumes* a reference count to each of its arguments. */
static Py_NO_INLINE int
do_raise(PyThreadState *tstate, PyObject *exc, PyObject *cause)
{
    PyObject *type = NULL, *value = NULL;

    if (exc == NULL) {
        /* Reraise */
        _PyErr_StackItem *exc_info = _PyErr_GetTopmostException(tstate);
        exc = exc_info->exc_value;
        if (Py_IsNone(exc) || exc == NULL) {
            _PyErr_SetString(tstate, PyExc_RuntimeError,
                             "No active exception to reraise");
            return 0;
        }
        Py_INCREF(exc);
        assert(PyExceptionInstance_Check(exc));
        _PyErr_SetRaisedException(tstate, exc);
        return 1;
    }

    /* We support the following forms of raise:
       raise
       raise <instance>
       raise <type> */

    if (PyExceptionClass_Check(exc)) {
        type = exc;
        value = _PyObject_CallNoArgs(exc);
        if (value == NULL)
            goto raise_error;
        if (!PyExceptionInstance_Check(value)) {
            _PyErr_Format(tstate, PyExc_TypeError,
                          "calling %R should have returned an instance of "
                          "BaseException, not %R",
                          type, Py_TYPE(value));
             goto raise_error;
        }
    }
    else if (PyExceptionInstance_Check(exc)) {
        value = exc;
        type = PyExceptionInstance_Class(exc);
        Py_INCREF(type);
    }
    else {
        /* Not something you can raise.  You get an exception
           anyway, just not what you specified :-) */
        Py_DECREF(exc);
        _PyErr_SetString(tstate, PyExc_TypeError,
                         "exceptions must derive from BaseException");
        goto raise_error;
    }

    assert(type != NULL);
    assert(value != NULL);

    if (cause) {
        PyObject *fixed_cause;
        if (PyExceptionClass_Check(cause)) {
            fixed_cause = _PyObject_CallNoArgs(cause);
            if (fixed_cause == NULL)
                goto raise_error;
            if (!PyExceptionInstance_Check(fixed_cause)) {
                _PyErr_Format(tstate, PyExc_TypeError,
                              "calling %R should have returned an instance of "
                              "BaseException, not %R",
                              cause, Py_TYPE(fixed_cause));
                Py_DECREF(fixed_cause);
                goto raise_error;
            }
            Py_DECREF(cause);
        }
        else if (PyExceptionInstance_Check(cause)) {
            fixed_cause = cause;
        }
        else if (Py_IsNone(cause)) {
            Py_DECREF(cause);
            fixed_cause = NULL;
        }
        else {
            _PyErr_SetString(tstate, PyExc_TypeError,
                             "exception causes must derive from "
                             "BaseException");
            goto raise_error;
        }
        PyException_SetCause(value, fixed_cause);
    }

    _PyErr_SetObject(tstate, type, value);
    /* _PyErr_SetObject incref's its arguments */
    Py_DECREF(value);
    Py_DECREF(type);
    return 0;

raise_error:
    Py_XDECREF(value);
    Py_XDECREF(type);
    Py_XDECREF(cause);
    return 0;
}

/* Disable unused label warnings.  They are handy for debugging, even
   if computed gotos aren't used. */

/* TBD - what about other compilers? */
#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wunused-label"
#elif defined(_MSC_VER) /* MS_WINDOWS */
#  pragma warning(push)
#  pragma warning(disable:4102)
#endif
