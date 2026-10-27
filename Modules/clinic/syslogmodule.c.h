/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()

PyDoc_STRVAR(syslog_openlog__doc__,
"openlog($module, /, ident=<unrepresentable>, logoption=0,\n"
"        facility=LOG_USER)\n"
"--\n"
"\n"
"Set logging options of subsequent syslog() calls.");

#define SYSLOG_OPENLOG_METHODDEF    \
    {"openlog", _PyCFunction_CAST(syslog_openlog), METH_FASTCALL|METH_KEYWORDS, syslog_openlog__doc__},

static PyObject *
syslog_openlog_impl(PyObject *module, PyObject *ident, long logopt,
                    long facility);

static PyValue
syslog_openlog(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(ident), &_Py_ID(logoption), &_Py_ID(facility), },
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
    PyValue argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *ident = NULL;
    long logopt = 0;
    long facility = LOG_USER;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyValue_IsNull(args[0])) {
        {
            PyObject *_arg = PyValue_Object(args[0]);
            if (_arg == NULL) {
                goto exit;
            }
            if (!PyUnicode_Check(_arg)) {
                _PyArg_BadArgument("openlog", "argument 'ident'", "str", _arg);
                goto exit;
            }
            ident = _arg;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!PyValue_IsNull(args[1])) {
        logopt = PyLong_AsLongValue(args[1]);
        if (logopt == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    facility = PyLong_AsLongValue(args[2]);
    if (facility == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    Py_BEGIN_CRITICAL_SECTION(module);
    _return_value = syslog_openlog_impl(module, ident, logopt, facility);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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

static PyValue
syslog_syslog(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    int group_left_1 = 0;
    int priority = LOG_INFO;
    const char *message;
    PyObject *_return_value;

    if (nargs < 1 || nargs > 2) {
        PyErr_SetString(PyExc_TypeError, "syslog.syslog requires 1 to 2 arguments");
        goto exit;
    }
    if (nargs >= 2) {
        priority = PyLong_AsIntValue(args[0]);
        if (priority == -1 && PyErr_Occurred()) {
            goto exit;
        }
        group_left_1 = 1;
    }
    {
        PyObject *_arg = PyValue_Object(args[nargs - 1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("syslog", "argument", "str", _arg);
            goto exit;
        }
        message = _PyUnicode_AsUTF8NoNUL(_arg);
        if (message == NULL) {
            goto exit;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(module);
    _return_value = syslog_syslog_impl(module, group_left_1, priority, message);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(syslog_closelog__doc__,
"closelog($module, /)\n"
"--\n"
"\n"
"Reset the syslog module values and call the system library closelog().");

#define SYSLOG_CLOSELOG_METHODDEF    \
    {"closelog", (PyCFunction)syslog_closelog, METH_NOARGS, syslog_closelog__doc__},

static PyObject *
syslog_closelog_impl(PyObject *module);

static PyObject *
syslog_closelog(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = syslog_closelog_impl(module);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(syslog_setlogmask__doc__,
"setlogmask($module, maskpri, /)\n"
"--\n"
"\n"
"Set the priority mask to maskpri and return the previous mask value.");

#define SYSLOG_SETLOGMASK_METHODDEF    \
    {"setlogmask", _PyCFunction_CAST(syslog_setlogmask), METH_O, syslog_setlogmask__doc__},

static long
syslog_setlogmask_impl(PyObject *module, long maskpri);

static PyValue
syslog_setlogmask(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    long maskpri;
    long _return_value;

    maskpri = PyLong_AsLongValue(arg);
    if (maskpri == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = syslog_setlogmask_impl(module, maskpri);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromLongLong((long long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(syslog_LOG_MASK__doc__,
"LOG_MASK($module, pri, /)\n"
"--\n"
"\n"
"Calculates the mask for the individual priority pri.");

#define SYSLOG_LOG_MASK_METHODDEF    \
    {"LOG_MASK", _PyCFunction_CAST(syslog_LOG_MASK), METH_O, syslog_LOG_MASK__doc__},

static long
syslog_LOG_MASK_impl(PyObject *module, long pri);

static PyValue
syslog_LOG_MASK(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    long pri;
    long _return_value;

    pri = PyLong_AsLongValue(arg);
    if (pri == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = syslog_LOG_MASK_impl(module, pri);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromLongLong((long long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(syslog_LOG_UPTO__doc__,
"LOG_UPTO($module, pri, /)\n"
"--\n"
"\n"
"Calculates the mask for all priorities up to and including pri.");

#define SYSLOG_LOG_UPTO_METHODDEF    \
    {"LOG_UPTO", _PyCFunction_CAST(syslog_LOG_UPTO), METH_O, syslog_LOG_UPTO__doc__},

static long
syslog_LOG_UPTO_impl(PyObject *module, long pri);

static PyValue
syslog_LOG_UPTO(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    long pri;
    long _return_value;

    pri = PyLong_AsLongValue(arg);
    if (pri == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = syslog_LOG_UPTO_impl(module, pri);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromLongLong((long long)_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=c4e6844fc7c811fb input=a9049054013a1b77]*/
