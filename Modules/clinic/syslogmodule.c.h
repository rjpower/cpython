/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#  include "pycore_value.h"       // _PyValue_STATIC_IMMORTAL()
#endif
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_long_value.h"    // _PyValue_AsLong()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(syslog_openlog__doc__,
"openlog($module, /, ident=<unrepresentable>, logoption=0,\n"
"        facility=LOG_USER)\n"
"--\n"
"\n"
"Set logging options of subsequent syslog() calls.");

#define SYSLOG_OPENLOG_METHODDEF    \
    {"openlog", _PyCFunction_CAST(syslog_openlog), METH_VALUE|METH_KEYWORDS, syslog_openlog__doc__},

static PyValue
syslog_openlog_impl(PyObject *module, PyObject *ident, long logopt,
                    long facility);

static PyValue
syslog_openlog(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(ident)), _PyValue_STATIC_IMMORTAL(&_Py_ID(logoption)), _PyValue_STATIC_IMMORTAL(&_Py_ID(facility)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"ident", "logoption", "facility", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "openlog",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyObject *ident = NULL;
    long logopt = 0;
    long facility = LOG_USER;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
            _PyArg_BadArgumentValue("openlog", "argument 'ident'", "str", PyValue_Borrow(args[_idx[0]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
            _PyArg_BadArgument("openlog", "argument 'ident'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
            goto exit;
        }
        ident = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]]));
    }
    if (_idx[1] >= 0) {
        logopt = _PyValue_AsLong(PyValue_Borrow(args[_idx[1]]));
        if (logopt == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        facility = _PyValue_AsLong(PyValue_Borrow(args[_idx[2]]));
        if (facility == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = syslog_openlog_impl(PyValue_AsKnownObjectBorrow(module), ident, logopt, facility);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(syslog_syslog__doc__,
"syslog([priority=LOG_INFO,] message)\n"
"Send the string message to the system logger.");

#define SYSLOG_SYSLOG_METHODDEF    \
    {"syslog", _PyCFunction_CAST(syslog_syslog), METH_FASTCALL, syslog_syslog__doc__},

static PyObject *
syslog_syslog_impl(PyObject *module, int group_left_1, int priority,
                   const char *message);

static PyObject *
syslog_syslog(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int group_left_1 = 0;
    int priority = LOG_INFO;
    const char *message;

    if (nargs < 1 || nargs > 2) {
        PyErr_SetString(PyExc_TypeError, "syslog.syslog requires 1 to 2 arguments");
        goto exit;
    }
    if (nargs >= 2) {
        priority = _PyValue_AsInt(PyValue_FromObjectBorrow(args[0]));
        if (priority == -1 && PyErr_Occurred()) {
            goto exit;
        }
        group_left_1 = 1;
    }
    if (!PyUnicode_Check(args[nargs - 1])) {
        _PyArg_BadArgument("syslog", "argument", "str", args[nargs - 1]);
        goto exit;
    }
    message = _PyUnicode_AsUTF8NoNUL(args[nargs - 1]);
    if (message == NULL) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = syslog_syslog_impl(module, group_left_1, priority, message);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(syslog_closelog__doc__,
"closelog($module, /)\n"
"--\n"
"\n"
"Reset the syslog module values and call the system library closelog().");

#define SYSLOG_CLOSELOG_METHODDEF    \
    {"closelog", _PyCFunction_CAST(syslog_closelog), METH_VALUE|METH_NOARGS, syslog_closelog__doc__},

static PyValue
syslog_closelog_impl(PyObject *module);

static PyValue
syslog_closelog(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = syslog_closelog_impl(PyValue_AsKnownObjectBorrow(module));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(syslog_setlogmask__doc__,
"setlogmask($module, maskpri, /)\n"
"--\n"
"\n"
"Set the priority mask to maskpri and return the previous mask value.");

#define SYSLOG_SETLOGMASK_METHODDEF    \
    {"setlogmask", _PyCFunction_CAST(syslog_setlogmask), METH_VALUE|METH_O, syslog_setlogmask__doc__},

static long
syslog_setlogmask_impl(PyObject *module, long maskpri);

static PyValue
syslog_setlogmask(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    long maskpri;
    long _return_value;

    maskpri = _PyValue_AsLong(PyValue_Borrow(args[0]));
    if (maskpri == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = syslog_setlogmask_impl(PyValue_AsKnownObjectBorrow(module), maskpri);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(syslog_LOG_MASK__doc__,
"LOG_MASK($module, pri, /)\n"
"--\n"
"\n"
"Calculates the mask for the individual priority pri.");

#define SYSLOG_LOG_MASK_METHODDEF    \
    {"LOG_MASK", _PyCFunction_CAST(syslog_LOG_MASK), METH_VALUE|METH_O, syslog_LOG_MASK__doc__},

static long
syslog_LOG_MASK_impl(PyObject *module, long pri);

static PyValue
syslog_LOG_MASK(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    long pri;
    long _return_value;

    pri = _PyValue_AsLong(PyValue_Borrow(args[0]));
    if (pri == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = syslog_LOG_MASK_impl(PyValue_AsKnownObjectBorrow(module), pri);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(syslog_LOG_UPTO__doc__,
"LOG_UPTO($module, pri, /)\n"
"--\n"
"\n"
"Calculates the mask for all priorities up to and including pri.");

#define SYSLOG_LOG_UPTO_METHODDEF    \
    {"LOG_UPTO", _PyCFunction_CAST(syslog_LOG_UPTO), METH_VALUE|METH_O, syslog_LOG_UPTO__doc__},

static long
syslog_LOG_UPTO_impl(PyObject *module, long pri);

static PyValue
syslog_LOG_UPTO(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    long pri;
    long _return_value;

    pri = _PyValue_AsLong(PyValue_Borrow(args[0]));
    if (pri == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = syslog_LOG_UPTO_impl(PyValue_AsKnownObjectBorrow(module), pri);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=c24ebb2cc001eff5 input=a9049054013a1b77]*/
