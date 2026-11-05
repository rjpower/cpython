#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif
#define Py_VALUE_CORE 1
#include "Python.h"
#include "pycore_tracemalloc.h"  // _PyTraceMalloc_IsTracing
#include "pycore_value.h"         // PyValue_FromUInt64(), _PyValue_AsObjectAt()
#include "pycore_value_slots.h"// _PyValue_FromObjectResult()

#include "clinic/_tracemalloc.c.h"


/*[clinic input]
value_emission
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=eb7b17088bfd7215]*/

/*[clinic input]
module _tracemalloc
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=708a98302fc46e5f]*/


/*[clinic input]
@permit_long_summary
_tracemalloc.is_tracing

Return True if the tracemalloc module is tracing Python memory allocations.
[clinic start generated code]*/

static PyValue
_tracemalloc_is_tracing_impl(PyObject *module)
/*[clinic end generated code: output=c18273513b85cdb1 input=cac4fc9096babeac]*/
{
    return PyValue_FromBool(_PyTraceMalloc_IsTracing());
}


/*[clinic input]
_tracemalloc.clear_traces

Clear traces of memory blocks allocated by Python.
[clinic start generated code]*/

static PyValue
_tracemalloc_clear_traces_impl(PyObject *module)
/*[clinic end generated code: output=4a95f707e7de9c17 input=0dab5b6c785183a5]*/
{
    _PyTraceMalloc_ClearTraces();
    return PyValue_None;
}


/*[clinic input]
_tracemalloc._get_traces

Get traces of all memory blocks allocated by Python.

Return a list of (size: int, traceback: tuple) tuples.
traceback is a tuple of (filename: str, lineno: int) tuples.

Return an empty list if the tracemalloc module is disabled.
[clinic start generated code]*/

static PyValue
_tracemalloc__get_traces_impl(PyObject *module)
/*[clinic end generated code: output=09dc0dc1bf561c41 input=6c7d2230b24255aa]*/
{
    return _PyValue_FromObjectResult(_PyTraceMalloc_GetTraces());
}



/*[clinic input]
_tracemalloc._get_object_traceback

    obj: object
    /

Get the traceback where the Python object obj was allocated.

Return a tuple of (filename: str, lineno: int) tuples.
Return None if the tracemalloc module is disabled or did not
trace the allocation of the object.
[clinic start generated code]*/

static PyValue
_tracemalloc__get_object_traceback_impl(PyObject *module, PyValueBorrow obj)
/*[clinic end generated code: output=66eb1edc57ca8b26 input=29495f1b21c53212]*/
{
    PyObject *o = _PyValue_AsObjectAt(obj, BOX_SITE_PARSER);
    if (o == NULL) {
        return PyValue_NULL;
    }
    PyObject *traceback = _PyTraceMalloc_GetObjectTraceback(o);
    Py_DECREF(o);
    return _PyValue_FromObjectResult(traceback);
}


/*[clinic input]
_tracemalloc.start

    nframe: int = 1
    /

Start tracing Python memory allocations.

Also set the maximum number of frames stored in the traceback of a
trace to nframe.
[clinic start generated code]*/

static PyValue
_tracemalloc_start_impl(PyObject *module, int nframe)
/*[clinic end generated code: output=f073faf8e678dc8f input=40d849b5b29d1933]*/
{
    if (_PyTraceMalloc_Start(nframe) < 0) {
        return PyValue_NULL;
    }
    return PyValue_None;
}


/*[clinic input]
_tracemalloc.stop

Stop tracing Python memory allocations.

Also clear traces of memory blocks allocated by Python.
[clinic start generated code]*/

static PyValue
_tracemalloc_stop_impl(PyObject *module)
/*[clinic end generated code: output=8c9f640e91baead7 input=7478f075e51dae18]*/
{
    _PyTraceMalloc_Stop();
    return PyValue_None;
}


/*[clinic input]
_tracemalloc.get_traceback_limit

Get the maximum number of frames stored in the traceback of a trace.

By default, a trace of an allocated memory block only stores
the most recent frame: the limit is 1.
[clinic start generated code]*/

static PyValue
_tracemalloc_get_traceback_limit_impl(PyObject *module)
/*[clinic end generated code: output=3e9648d3c681a068 input=da3cd977fc68ae3b]*/
{
    return PyValue_FromInt64(_PyTraceMalloc_GetTracebackLimit());
}

/*[clinic input]
_tracemalloc.get_tracemalloc_memory

Get the memory usage in bytes of the tracemalloc module.

This memory is used internally to trace memory allocations.
[clinic start generated code]*/

static PyValue
_tracemalloc_get_tracemalloc_memory_impl(PyObject *module)
/*[clinic end generated code: output=d933da3bedb28a22 input=5d919c0f4d5132ad]*/
{
    return PyValue_FromUInt64(_PyTraceMalloc_GetMemory());
}


/*[clinic input]
@permit_long_summary
_tracemalloc.get_traced_memory

Get the current size and peak size of memory blocks traced by tracemalloc.

Returns a tuple: (current: int, peak: int).
[clinic start generated code]*/

static PyValue
_tracemalloc_get_traced_memory_impl(PyObject *module)
/*[clinic end generated code: output=b5b2fe8feef5a999 input=b06e7a1a4914fc21]*/
{
    return _PyValue_FromObjectResult(_PyTraceMalloc_GetTracedMemory());
}

/*[clinic input]
@permit_long_summary
_tracemalloc.reset_peak

Set the peak size of memory blocks traced by tracemalloc to the current size.

Do nothing if the tracemalloc module is not tracing memory allocations.

[clinic start generated code]*/

static PyValue
_tracemalloc_reset_peak_impl(PyObject *module)
/*[clinic end generated code: output=cf90a30233dd3ce9 input=4103319210f46286]*/
{
    _PyTraceMalloc_ResetPeak();
    return PyValue_None;
}


static PyMethodDef module_methods[] = {
    _TRACEMALLOC_IS_TRACING_METHODDEF
    _TRACEMALLOC_CLEAR_TRACES_METHODDEF
    _TRACEMALLOC__GET_TRACES_METHODDEF
    _TRACEMALLOC__GET_OBJECT_TRACEBACK_METHODDEF
    _TRACEMALLOC_START_METHODDEF
    _TRACEMALLOC_STOP_METHODDEF
    _TRACEMALLOC_GET_TRACEBACK_LIMIT_METHODDEF
    _TRACEMALLOC_GET_TRACEMALLOC_MEMORY_METHODDEF
    _TRACEMALLOC_GET_TRACED_MEMORY_METHODDEF
    _TRACEMALLOC_RESET_PEAK_METHODDEF
    /* sentinel */
    {NULL, NULL}
};

PyDoc_STRVAR(module_doc,
"Debug module to trace memory blocks allocated by Python.");

static struct PyModuleDef module_def = {
    PyModuleDef_HEAD_INIT,
    "_tracemalloc",
    module_doc,
    0, /* non-negative size to be able to unload the module */
    module_methods,
    NULL,
};

PyMODINIT_FUNC
PyInit__tracemalloc(void)
{
    PyABIInfo_VAR(abi_info);
    if (PyABIInfo_Check(&abi_info, "_tracemalloc") < 0) {
        return NULL;
    }

    PyObject *mod = PyModule_Create(&module_def);
    if (mod == NULL) {
        return NULL;
    }

#ifdef Py_GIL_DISABLED
    PyUnstable_Module_SetGIL(mod, Py_MOD_GIL_NOT_USED);
#endif

    return mod;
}
