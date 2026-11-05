/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(time_time__doc__,
"time($module, /)\n"
"--\n"
"\n"
"Return the current time in seconds since the Epoch.\n"
"\n"
"Fractions of a second may be present if the system clock provides\n"
"them.");

#define TIME_TIME_METHODDEF    \
    {"time", _PyCFunction_CAST(time_time), METH_VALUE|METH_NOARGS, time_time__doc__},

static double
time_time_impl(PyObject *module);

static PyValue
time_time(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    double _return_value;

    _return_value = time_time_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == -1.0) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromDouble(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(time_time_ns__doc__,
"time_ns($module, /)\n"
"--\n"
"\n"
"Return the current time in nanoseconds since the Epoch.");

#define TIME_TIME_NS_METHODDEF    \
    {"time_ns", _PyCFunction_CAST(time_time_ns), METH_VALUE|METH_NOARGS, time_time_ns__doc__},

static PyValue
time_time_ns_impl(PyObject *module);

static PyValue
time_time_ns(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = time_time_ns_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#if defined(HAVE_CLOCK_GETTIME)

PyDoc_STRVAR(time_clock_gettime__doc__,
"clock_gettime($module, clk_id, /)\n"
"--\n"
"\n"
"Return the time of the specified clock clk_id as a float.");

#define TIME_CLOCK_GETTIME_METHODDEF    \
    {"clock_gettime", _PyCFunction_CAST(time_clock_gettime), METH_VALUE|METH_O, time_clock_gettime__doc__},

static PyValue
time_clock_gettime_impl(PyObject *module, clockid_t clk_id);

static PyValue
time_clock_gettime(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    clockid_t clk_id;

    if (!time_clockid_converter(PyValue_Borrow(args[0]), &clk_id)) {
        goto exit;
    }
    return_value = time_clock_gettime_impl(PyValue_AsKnownObjectBorrow(module), clk_id);

exit:
    return return_value;
}

#endif /* defined(HAVE_CLOCK_GETTIME) */

#if defined(HAVE_CLOCK_GETTIME)

PyDoc_STRVAR(time_clock_gettime_ns__doc__,
"clock_gettime_ns($module, clk_id, /)\n"
"--\n"
"\n"
"Return the time of the specified clock clk_id as nanoseconds (int).");

#define TIME_CLOCK_GETTIME_NS_METHODDEF    \
    {"clock_gettime_ns", _PyCFunction_CAST(time_clock_gettime_ns), METH_VALUE|METH_O, time_clock_gettime_ns__doc__},

static PyValue
time_clock_gettime_ns_impl(PyObject *module, clockid_t clk_id);

static PyValue
time_clock_gettime_ns(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    clockid_t clk_id;

    if (!time_clockid_converter(PyValue_Borrow(args[0]), &clk_id)) {
        goto exit;
    }
    return_value = time_clock_gettime_ns_impl(PyValue_AsKnownObjectBorrow(module), clk_id);

exit:
    return return_value;
}

#endif /* defined(HAVE_CLOCK_GETTIME) */

#if defined(HAVE_CLOCK_SETTIME)

PyDoc_STRVAR(time_clock_settime__doc__,
"clock_settime($module, clk_id, time, /)\n"
"--\n"
"\n"
"Set the time of the specified clock clk_id.");

#define TIME_CLOCK_SETTIME_METHODDEF    \
    {"clock_settime", _PyCFunction_CAST(time_clock_settime), METH_VALUE, time_clock_settime__doc__},

static PyValue
time_clock_settime_impl(PyObject *module, int clk_id, PyValueBorrow obj);

static PyValue
time_clock_settime(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int clk_id;
    PyValueBorrow obj;

    if (!_PyArg_CheckPositional("clock_settime", nargs, 2, 2)) {
        goto exit;
    }
    clk_id = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (clk_id == -1 && PyErr_Occurred()) {
        goto exit;
    }
    obj = PyValue_Borrow(args[1]);
    return_value = time_clock_settime_impl(PyValue_AsKnownObjectBorrow(module), clk_id, obj);

exit:
    return return_value;
}

#endif /* defined(HAVE_CLOCK_SETTIME) */

#if defined(HAVE_CLOCK_SETTIME)

PyDoc_STRVAR(time_clock_settime_ns__doc__,
"clock_settime_ns($module, clk_id, time, /)\n"
"--\n"
"\n"
"Set the time of the specified clock clk_id with nanoseconds.");

#define TIME_CLOCK_SETTIME_NS_METHODDEF    \
    {"clock_settime_ns", _PyCFunction_CAST(time_clock_settime_ns), METH_VALUE, time_clock_settime_ns__doc__},

static PyValue
time_clock_settime_ns_impl(PyObject *module, int clk_id, PyValueBorrow obj);

static PyValue
time_clock_settime_ns(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int clk_id;
    PyValueBorrow obj;

    if (!_PyArg_CheckPositional("clock_settime_ns", nargs, 2, 2)) {
        goto exit;
    }
    clk_id = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (clk_id == -1 && PyErr_Occurred()) {
        goto exit;
    }
    obj = PyValue_Borrow(args[1]);
    return_value = time_clock_settime_ns_impl(PyValue_AsKnownObjectBorrow(module), clk_id, obj);

exit:
    return return_value;
}

#endif /* defined(HAVE_CLOCK_SETTIME) */

#if defined(HAVE_CLOCK_GETRES)

PyDoc_STRVAR(time_clock_getres__doc__,
"clock_getres($module, clk_id, /)\n"
"--\n"
"\n"
"Return the resolution (precision) of the specified clock clk_id.");

#define TIME_CLOCK_GETRES_METHODDEF    \
    {"clock_getres", _PyCFunction_CAST(time_clock_getres), METH_VALUE|METH_O, time_clock_getres__doc__},

static double
time_clock_getres_impl(PyObject *module, int clk_id);

static PyValue
time_clock_getres(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int clk_id;
    double _return_value;

    clk_id = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (clk_id == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = time_clock_getres_impl(PyValue_AsKnownObjectBorrow(module), clk_id);
    if ((_return_value == -1.0) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromDouble(_return_value);

exit:
    return return_value;
}

#endif /* defined(HAVE_CLOCK_GETRES) */

#if defined(HAVE_PTHREAD_GETCPUCLOCKID)

PyDoc_STRVAR(time_pthread_getcpuclockid__doc__,
"pthread_getcpuclockid($module, thread_id, /)\n"
"--\n"
"\n"
"Return the clk_id of a thread\'s CPU time clock.");

#define TIME_PTHREAD_GETCPUCLOCKID_METHODDEF    \
    {"pthread_getcpuclockid", _PyCFunction_CAST(time_pthread_getcpuclockid), METH_VALUE|METH_O, time_pthread_getcpuclockid__doc__},

static PyValue
time_pthread_getcpuclockid_impl(PyObject *module, unsigned long thread_id);

static PyValue
time_pthread_getcpuclockid(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    unsigned long thread_id;

        if (PyValue_TYPE(PyValue_Borrow(args[0]))->tp_as_number == NULL ||
            PyValue_TYPE(PyValue_Borrow(args[0]))->tp_as_number->nb_index == NULL)
        {
            _PyArg_BadArgumentValue("pthread_getcpuclockid", "argument", "int", PyValue_Borrow(args[0]));
            goto exit;
        }

    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[0]), &thread_id, sizeof(unsigned long),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(unsigned long)) {
            if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "integer value out of range", 1) < 0)
            {
                goto exit;
            }
        }
    }
    return_value = time_pthread_getcpuclockid_impl(PyValue_AsKnownObjectBorrow(module), thread_id);

exit:
    return return_value;
}

#endif /* defined(HAVE_PTHREAD_GETCPUCLOCKID) */

PyDoc_STRVAR(time_sleep__doc__,
"sleep($module, seconds, /)\n"
"--\n"
"\n"
"Delay execution for a given number of seconds.\n"
"\n"
"The argument may be a floating-point number for subsecond precision.");

#define TIME_SLEEP_METHODDEF    \
    {"sleep", _PyCFunction_CAST(time_sleep), METH_VALUE|METH_O, time_sleep__doc__},

static PyValue
time_sleep_impl(PyObject *module, PyValueBorrow timeout_obj);

static PyValue
time_sleep(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow timeout_obj;

    timeout_obj = PyValue_Borrow(args[0]);
    return_value = time_sleep_impl(PyValue_AsKnownObjectBorrow(module), timeout_obj);

    return return_value;
}

PyDoc_STRVAR(time_gmtime__doc__,
"gmtime($module, seconds=None, /)\n"
"--\n"
"\n"
"Convert seconds since the Epoch to a time tuple expressing UTC.\n"
"\n"
"That is, Greenwich Mean Time.  When \'seconds\' is not passed in, convert\n"
"the current time instead.\n"
"\n"
"If the platform supports the tm_gmtoff and tm_zone, they are available\n"
"as attributes only.");

#define TIME_GMTIME_METHODDEF    \
    {"gmtime", _PyCFunction_CAST(time_gmtime), METH_VALUE, time_gmtime__doc__},

static PyValue
time_gmtime_impl(PyObject *module, PyValueBorrow ot);

static PyValue
time_gmtime(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow ot = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("gmtime", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    ot = PyValue_Borrow(args[0]);
skip_optional:
    return_value = time_gmtime_impl(PyValue_AsKnownObjectBorrow(module), ot);

exit:
    return return_value;
}

PyDoc_STRVAR(time_localtime__doc__,
"localtime($module, seconds=None, /)\n"
"--\n"
"\n"
"Convert seconds since the Epoch to a time tuple expressing local time.\n"
"\n"
"When \'seconds\' is not passed in, convert the current time instead.");

#define TIME_LOCALTIME_METHODDEF    \
    {"localtime", _PyCFunction_CAST(time_localtime), METH_VALUE, time_localtime__doc__},

static PyValue
time_localtime_impl(PyObject *module, PyValueBorrow ot);

static PyValue
time_localtime(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow ot = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("localtime", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    ot = PyValue_Borrow(args[0]);
skip_optional:
    return_value = time_localtime_impl(PyValue_AsKnownObjectBorrow(module), ot);

exit:
    return return_value;
}

PyDoc_STRVAR(time_asctime__doc__,
"asctime($module, time_tuple=<unrepresentable>, /)\n"
"--\n"
"\n"
"Convert a time tuple to a string, e.g. \'Sat Jun 06 16:26:11 1998\'.\n"
"\n"
"When the time tuple is not present, current time as returned by\n"
"localtime() is used.");

#define TIME_ASCTIME_METHODDEF    \
    {"asctime", _PyCFunction_CAST(time_asctime), METH_VALUE, time_asctime__doc__},

static PyValue
time_asctime_impl(PyObject *module, PyValueBorrow tup);

static PyValue
time_asctime(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow tup = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_CheckPositional("asctime", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    tup = PyValue_Borrow(args[0]);
skip_optional:
    return_value = time_asctime_impl(PyValue_AsKnownObjectBorrow(module), tup);

exit:
    return return_value;
}

PyDoc_STRVAR(time_ctime__doc__,
"ctime($module, seconds=None, /)\n"
"--\n"
"\n"
"Convert a time in seconds since the Epoch to a string in local time.\n"
"\n"
"This is equivalent to asctime(localtime(seconds)).  When \'seconds\' is\n"
"not passed in, convert the current time instead.");

#define TIME_CTIME_METHODDEF    \
    {"ctime", _PyCFunction_CAST(time_ctime), METH_VALUE, time_ctime__doc__},

static PyValue
time_ctime_impl(PyObject *module, PyValueBorrow ot);

static PyValue
time_ctime(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow ot = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("ctime", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    ot = PyValue_Borrow(args[0]);
skip_optional:
    return_value = time_ctime_impl(PyValue_AsKnownObjectBorrow(module), ot);

exit:
    return return_value;
}

#if defined(HAVE_MKTIME)

PyDoc_STRVAR(time_mktime__doc__,
"mktime($module, time_tuple, /)\n"
"--\n"
"\n"
"Convert a time tuple in local time to seconds since the Epoch.\n"
"\n"
"Note that mktime(gmtime(0)) will not generally return zero for most\n"
"time zones; instead the returned value will either be equal to that of\n"
"the timezone or altzone attributes on the time module.");

#define TIME_MKTIME_METHODDEF    \
    {"mktime", _PyCFunction_CAST(time_mktime), METH_VALUE|METH_O, time_mktime__doc__},

static PyValue
time_mktime_impl(PyObject *module, PyValueBorrow tm_tuple);

static PyValue
time_mktime(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow tm_tuple;

    tm_tuple = PyValue_Borrow(args[0]);
    return_value = time_mktime_impl(PyValue_AsKnownObjectBorrow(module), tm_tuple);

    return return_value;
}

#endif /* defined(HAVE_MKTIME) */

#if defined(HAVE_WORKING_TZSET)

PyDoc_STRVAR(time_tzset__doc__,
"tzset($module, /)\n"
"--\n"
"\n"
"Initialize, or reinitialize, the local timezone.\n"
"\n"
"The local timezone is set to the value stored in os.environ[\'TZ\'].  The\n"
"TZ environment variable should be specified in standard Unix timezone\n"
"format as documented in the tzset man page (eg. \'US/Eastern\',\n"
"\'Europe/Amsterdam\').  Unknown timezones will silently fall back to UTC.\n"
"If the TZ environment variable is not set, the local timezone is set to\n"
"the systems best guess of wallclock time.  Changing the TZ environment\n"
"variable without calling tzset *may* change the local timezone used by\n"
"methods such as localtime, but this behaviour should not be relied on.");

#define TIME_TZSET_METHODDEF    \
    {"tzset", _PyCFunction_CAST(time_tzset), METH_VALUE|METH_NOARGS, time_tzset__doc__},

static PyValue
time_tzset_impl(PyObject *module);

static PyValue
time_tzset(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = time_tzset_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* defined(HAVE_WORKING_TZSET) */

PyDoc_STRVAR(time_monotonic__doc__,
"monotonic($module, /)\n"
"--\n"
"\n"
"Monotonic clock, cannot go backward.");

#define TIME_MONOTONIC_METHODDEF    \
    {"monotonic", _PyCFunction_CAST(time_monotonic), METH_VALUE|METH_NOARGS, time_monotonic__doc__},

static double
time_monotonic_impl(PyObject *module);

static PyValue
time_monotonic(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    double _return_value;

    _return_value = time_monotonic_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == -1.0) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromDouble(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(time_monotonic_ns__doc__,
"monotonic_ns($module, /)\n"
"--\n"
"\n"
"Monotonic clock, cannot go backward, as nanoseconds.");

#define TIME_MONOTONIC_NS_METHODDEF    \
    {"monotonic_ns", _PyCFunction_CAST(time_monotonic_ns), METH_VALUE|METH_NOARGS, time_monotonic_ns__doc__},

static PyValue
time_monotonic_ns_impl(PyObject *module);

static PyValue
time_monotonic_ns(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = time_monotonic_ns_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(time_perf_counter__doc__,
"perf_counter($module, /)\n"
"--\n"
"\n"
"Performance counter for benchmarking.");

#define TIME_PERF_COUNTER_METHODDEF    \
    {"perf_counter", _PyCFunction_CAST(time_perf_counter), METH_VALUE|METH_NOARGS, time_perf_counter__doc__},

static double
time_perf_counter_impl(PyObject *module);

static PyValue
time_perf_counter(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    double _return_value;

    _return_value = time_perf_counter_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == -1.0) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromDouble(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(time_perf_counter_ns__doc__,
"perf_counter_ns($module, /)\n"
"--\n"
"\n"
"Performance counter for benchmarking as nanoseconds.");

#define TIME_PERF_COUNTER_NS_METHODDEF    \
    {"perf_counter_ns", _PyCFunction_CAST(time_perf_counter_ns), METH_VALUE|METH_NOARGS, time_perf_counter_ns__doc__},

static PyValue
time_perf_counter_ns_impl(PyObject *module);

static PyValue
time_perf_counter_ns(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = time_perf_counter_ns_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(time_process_time__doc__,
"process_time($module, /)\n"
"--\n"
"\n"
"Process time for profiling.\n"
"\n"
"That is the sum of the kernel and user-space CPU time.");

#define TIME_PROCESS_TIME_METHODDEF    \
    {"process_time", _PyCFunction_CAST(time_process_time), METH_VALUE|METH_NOARGS, time_process_time__doc__},

static double
time_process_time_impl(PyObject *module);

static PyValue
time_process_time(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    double _return_value;

    _return_value = time_process_time_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == -1.0) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromDouble(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(time_process_time_ns__doc__,
"process_time_ns($module, /)\n"
"--\n"
"\n"
"Process time for profiling as nanoseconds.\n"
"\n"
"That is the sum of the kernel and user-space CPU time.");

#define TIME_PROCESS_TIME_NS_METHODDEF    \
    {"process_time_ns", _PyCFunction_CAST(time_process_time_ns), METH_VALUE|METH_NOARGS, time_process_time_ns__doc__},

static PyValue
time_process_time_ns_impl(PyObject *module);

static PyValue
time_process_time_ns(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = time_process_time_ns_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#if defined(HAVE_THREAD_TIME)

PyDoc_STRVAR(time_thread_time__doc__,
"thread_time($module, /)\n"
"--\n"
"\n"
"Thread time for profiling.\n"
"\n"
"That is the sum of the kernel and user-space CPU time.");

#define TIME_THREAD_TIME_METHODDEF    \
    {"thread_time", _PyCFunction_CAST(time_thread_time), METH_VALUE|METH_NOARGS, time_thread_time__doc__},

static double
time_thread_time_impl(PyObject *module);

static PyValue
time_thread_time(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    double _return_value;

    _return_value = time_thread_time_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == -1.0) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromDouble(_return_value);

exit:
    return return_value;
}

#endif /* defined(HAVE_THREAD_TIME) */

#if defined(HAVE_THREAD_TIME)

PyDoc_STRVAR(time_thread_time_ns__doc__,
"thread_time_ns($module, /)\n"
"--\n"
"\n"
"Thread time for profiling as nanoseconds.\n"
"\n"
"That is the sum of the kernel and user-space CPU time.");

#define TIME_THREAD_TIME_NS_METHODDEF    \
    {"thread_time_ns", _PyCFunction_CAST(time_thread_time_ns), METH_VALUE|METH_NOARGS, time_thread_time_ns__doc__},

static PyValue
time_thread_time_ns_impl(PyObject *module);

static PyValue
time_thread_time_ns(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = time_thread_time_ns_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* defined(HAVE_THREAD_TIME) */

PyDoc_STRVAR(time_get_clock_info__doc__,
"get_clock_info($module, name, /)\n"
"--\n"
"\n"
"Get information of the specified clock.");

#define TIME_GET_CLOCK_INFO_METHODDEF    \
    {"get_clock_info", _PyCFunction_CAST(time_get_clock_info), METH_VALUE|METH_O, time_get_clock_info__doc__},

static PyValue
time_get_clock_info_impl(PyObject *module, const char *name);

static PyValue
time_get_clock_info(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *name;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("get_clock_info", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("get_clock_info", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    name = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (name == NULL) {
        goto exit;
    }
    return_value = time_get_clock_info_impl(PyValue_AsKnownObjectBorrow(module), name);

exit:
    return return_value;
}

#ifndef TIME_CLOCK_GETTIME_METHODDEF
    #define TIME_CLOCK_GETTIME_METHODDEF
#endif /* !defined(TIME_CLOCK_GETTIME_METHODDEF) */

#ifndef TIME_CLOCK_GETTIME_NS_METHODDEF
    #define TIME_CLOCK_GETTIME_NS_METHODDEF
#endif /* !defined(TIME_CLOCK_GETTIME_NS_METHODDEF) */

#ifndef TIME_CLOCK_SETTIME_METHODDEF
    #define TIME_CLOCK_SETTIME_METHODDEF
#endif /* !defined(TIME_CLOCK_SETTIME_METHODDEF) */

#ifndef TIME_CLOCK_SETTIME_NS_METHODDEF
    #define TIME_CLOCK_SETTIME_NS_METHODDEF
#endif /* !defined(TIME_CLOCK_SETTIME_NS_METHODDEF) */

#ifndef TIME_CLOCK_GETRES_METHODDEF
    #define TIME_CLOCK_GETRES_METHODDEF
#endif /* !defined(TIME_CLOCK_GETRES_METHODDEF) */

#ifndef TIME_PTHREAD_GETCPUCLOCKID_METHODDEF
    #define TIME_PTHREAD_GETCPUCLOCKID_METHODDEF
#endif /* !defined(TIME_PTHREAD_GETCPUCLOCKID_METHODDEF) */

#ifndef TIME_MKTIME_METHODDEF
    #define TIME_MKTIME_METHODDEF
#endif /* !defined(TIME_MKTIME_METHODDEF) */

#ifndef TIME_TZSET_METHODDEF
    #define TIME_TZSET_METHODDEF
#endif /* !defined(TIME_TZSET_METHODDEF) */

#ifndef TIME_THREAD_TIME_METHODDEF
    #define TIME_THREAD_TIME_METHODDEF
#endif /* !defined(TIME_THREAD_TIME_METHODDEF) */

#ifndef TIME_THREAD_TIME_NS_METHODDEF
    #define TIME_THREAD_TIME_NS_METHODDEF
#endif /* !defined(TIME_THREAD_TIME_NS_METHODDEF) */
/*[clinic end generated code: output=2b4dbf211111624a input=a9049054013a1b77]*/
