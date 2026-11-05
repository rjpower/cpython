/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(_tracemalloc_is_tracing__doc__,
"is_tracing($module, /)\n"
"--\n"
"\n"
"Return True if the tracemalloc module is tracing Python memory allocations.");

#define _TRACEMALLOC_IS_TRACING_METHODDEF    \
    {"is_tracing", _PyCFunction_CAST(_tracemalloc_is_tracing), METH_VALUE|METH_NOARGS, _tracemalloc_is_tracing__doc__},

static PyValue
_tracemalloc_is_tracing_impl(PyObject *module);

static PyValue
_tracemalloc_is_tracing(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _tracemalloc_is_tracing_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_tracemalloc_clear_traces__doc__,
"clear_traces($module, /)\n"
"--\n"
"\n"
"Clear traces of memory blocks allocated by Python.");

#define _TRACEMALLOC_CLEAR_TRACES_METHODDEF    \
    {"clear_traces", _PyCFunction_CAST(_tracemalloc_clear_traces), METH_VALUE|METH_NOARGS, _tracemalloc_clear_traces__doc__},

static PyValue
_tracemalloc_clear_traces_impl(PyObject *module);

static PyValue
_tracemalloc_clear_traces(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _tracemalloc_clear_traces_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_tracemalloc__get_traces__doc__,
"_get_traces($module, /)\n"
"--\n"
"\n"
"Get traces of all memory blocks allocated by Python.\n"
"\n"
"Return a list of (size: int, traceback: tuple) tuples.\n"
"traceback is a tuple of (filename: str, lineno: int) tuples.\n"
"\n"
"Return an empty list if the tracemalloc module is disabled.");

#define _TRACEMALLOC__GET_TRACES_METHODDEF    \
    {"_get_traces", _PyCFunction_CAST(_tracemalloc__get_traces), METH_VALUE|METH_NOARGS, _tracemalloc__get_traces__doc__},

static PyValue
_tracemalloc__get_traces_impl(PyObject *module);

static PyValue
_tracemalloc__get_traces(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _tracemalloc__get_traces_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_tracemalloc__get_object_traceback__doc__,
"_get_object_traceback($module, obj, /)\n"
"--\n"
"\n"
"Get the traceback where the Python object obj was allocated.\n"
"\n"
"Return a tuple of (filename: str, lineno: int) tuples.\n"
"Return None if the tracemalloc module is disabled or did not\n"
"trace the allocation of the object.");

#define _TRACEMALLOC__GET_OBJECT_TRACEBACK_METHODDEF    \
    {"_get_object_traceback", _PyCFunction_CAST(_tracemalloc__get_object_traceback), METH_VALUE|METH_O, _tracemalloc__get_object_traceback__doc__},

static PyValue
_tracemalloc__get_object_traceback_impl(PyObject *module, PyValueBorrow obj);

static PyValue
_tracemalloc__get_object_traceback(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow obj;

    obj = PyValue_Borrow(args[0]);
    return_value = _tracemalloc__get_object_traceback_impl(PyValue_AsKnownObjectBorrow(module), obj);

    return return_value;
}

PyDoc_STRVAR(_tracemalloc_start__doc__,
"start($module, nframe=1, /)\n"
"--\n"
"\n"
"Start tracing Python memory allocations.\n"
"\n"
"Also set the maximum number of frames stored in the traceback of a\n"
"trace to nframe.");

#define _TRACEMALLOC_START_METHODDEF    \
    {"start", _PyCFunction_CAST(_tracemalloc_start), METH_VALUE, _tracemalloc_start__doc__},

static PyValue
_tracemalloc_start_impl(PyObject *module, int nframe);

static PyValue
_tracemalloc_start(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int nframe = 1;

    if (!_PyArg_CheckPositional("start", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    nframe = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (nframe == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _tracemalloc_start_impl(PyValue_AsKnownObjectBorrow(module), nframe);

exit:
    return return_value;
}

PyDoc_STRVAR(_tracemalloc_stop__doc__,
"stop($module, /)\n"
"--\n"
"\n"
"Stop tracing Python memory allocations.\n"
"\n"
"Also clear traces of memory blocks allocated by Python.");

#define _TRACEMALLOC_STOP_METHODDEF    \
    {"stop", _PyCFunction_CAST(_tracemalloc_stop), METH_VALUE|METH_NOARGS, _tracemalloc_stop__doc__},

static PyValue
_tracemalloc_stop_impl(PyObject *module);

static PyValue
_tracemalloc_stop(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _tracemalloc_stop_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_tracemalloc_get_traceback_limit__doc__,
"get_traceback_limit($module, /)\n"
"--\n"
"\n"
"Get the maximum number of frames stored in the traceback of a trace.\n"
"\n"
"By default, a trace of an allocated memory block only stores\n"
"the most recent frame: the limit is 1.");

#define _TRACEMALLOC_GET_TRACEBACK_LIMIT_METHODDEF    \
    {"get_traceback_limit", _PyCFunction_CAST(_tracemalloc_get_traceback_limit), METH_VALUE|METH_NOARGS, _tracemalloc_get_traceback_limit__doc__},

static PyValue
_tracemalloc_get_traceback_limit_impl(PyObject *module);

static PyValue
_tracemalloc_get_traceback_limit(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _tracemalloc_get_traceback_limit_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_tracemalloc_get_tracemalloc_memory__doc__,
"get_tracemalloc_memory($module, /)\n"
"--\n"
"\n"
"Get the memory usage in bytes of the tracemalloc module.\n"
"\n"
"This memory is used internally to trace memory allocations.");

#define _TRACEMALLOC_GET_TRACEMALLOC_MEMORY_METHODDEF    \
    {"get_tracemalloc_memory", _PyCFunction_CAST(_tracemalloc_get_tracemalloc_memory), METH_VALUE|METH_NOARGS, _tracemalloc_get_tracemalloc_memory__doc__},

static PyValue
_tracemalloc_get_tracemalloc_memory_impl(PyObject *module);

static PyValue
_tracemalloc_get_tracemalloc_memory(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _tracemalloc_get_tracemalloc_memory_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_tracemalloc_get_traced_memory__doc__,
"get_traced_memory($module, /)\n"
"--\n"
"\n"
"Get the current size and peak size of memory blocks traced by tracemalloc.\n"
"\n"
"Returns a tuple: (current: int, peak: int).");

#define _TRACEMALLOC_GET_TRACED_MEMORY_METHODDEF    \
    {"get_traced_memory", _PyCFunction_CAST(_tracemalloc_get_traced_memory), METH_VALUE|METH_NOARGS, _tracemalloc_get_traced_memory__doc__},

static PyValue
_tracemalloc_get_traced_memory_impl(PyObject *module);

static PyValue
_tracemalloc_get_traced_memory(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _tracemalloc_get_traced_memory_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_tracemalloc_reset_peak__doc__,
"reset_peak($module, /)\n"
"--\n"
"\n"
"Set the peak size of memory blocks traced by tracemalloc to the current size.\n"
"\n"
"Do nothing if the tracemalloc module is not tracing memory allocations.");

#define _TRACEMALLOC_RESET_PEAK_METHODDEF    \
    {"reset_peak", _PyCFunction_CAST(_tracemalloc_reset_peak), METH_VALUE|METH_NOARGS, _tracemalloc_reset_peak__doc__},

static PyValue
_tracemalloc_reset_peak_impl(PyObject *module);

static PyValue
_tracemalloc_reset_peak(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _tracemalloc_reset_peak_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}
/*[clinic end generated code: output=0bf6a487d261a0d3 input=a9049054013a1b77]*/
