/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_testinternalcapi_benchmark_locks__doc__,
"benchmark_locks($module, num_threads, work_inside=1, work_outside=0,\n"
"                time_ms=1000, num_acquisitions=1, total_iters=0,\n"
"                num_locks=1, random_locks=False, /)\n"
"--\n"
"\n");

#define _TESTINTERNALCAPI_BENCHMARK_LOCKS_METHODDEF    \
    {"benchmark_locks", _PyCFunction_CAST(_testinternalcapi_benchmark_locks), METH_FASTCALL, _testinternalcapi_benchmark_locks__doc__},

static PyObject *
_testinternalcapi_benchmark_locks_impl(PyObject *module,
                                       Py_ssize_t num_threads,
                                       int work_inside, int work_outside,
                                       int time_ms, int num_acquisitions,
                                       Py_ssize_t total_iters,
                                       Py_ssize_t num_locks,
                                       int random_locks);

static PyValue
_testinternalcapi_benchmark_locks(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t num_threads;
    int work_inside = 1;
    int work_outside = 0;
    int time_ms = 1000;
    int num_acquisitions = 1;
    Py_ssize_t total_iters = 0;
    Py_ssize_t num_locks = 1;
    int random_locks = 0;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("benchmark_locks", nargs, 1, 8)) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(args[0]);
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        num_threads = ival;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    work_inside = PyLong_AsIntValue(args[1]);
    if (work_inside == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    work_outside = PyLong_AsIntValue(args[2]);
    if (work_outside == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 4) {
        goto skip_optional;
    }
    time_ms = PyLong_AsIntValue(args[3]);
    if (time_ms == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 5) {
        goto skip_optional;
    }
    num_acquisitions = PyLong_AsIntValue(args[4]);
    if (num_acquisitions == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 6) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(args[5]);
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        total_iters = ival;
    }
    if (nargs < 7) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(args[6]);
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        num_locks = ival;
    }
    if (nargs < 8) {
        goto skip_optional;
    }
    random_locks = PyObject_IsTrueValue(args[7]);
    if (random_locks < 0) {
        goto exit;
    }
skip_optional:
    _return_value = _testinternalcapi_benchmark_locks_impl(module, num_threads, work_inside, work_outside, time_ms, num_acquisitions, total_iters, num_locks, random_locks);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=c71bb63f31e5ac46 input=a9049054013a1b77]*/
