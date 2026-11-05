/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(_sysconfig_config_vars__doc__,
"config_vars($module, /)\n"
"--\n"
"\n"
"Returns a dictionary containing build variables intended to be exposed by sysconfig.");

#define _SYSCONFIG_CONFIG_VARS_METHODDEF    \
    {"config_vars", _PyCFunction_CAST(_sysconfig_config_vars), METH_VALUE|METH_NOARGS, _sysconfig_config_vars__doc__},

static PyValue
_sysconfig_config_vars_impl(PyObject *module);

static PyValue
_sysconfig_config_vars(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _sysconfig_config_vars_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_sysconfig_get_platform__doc__,
"get_platform($module, /)\n"
"--\n"
"\n"
"Return a string that identifies the current platform.");

#define _SYSCONFIG_GET_PLATFORM_METHODDEF    \
    {"get_platform", _PyCFunction_CAST(_sysconfig_get_platform), METH_VALUE|METH_NOARGS, _sysconfig_get_platform__doc__},

static PyValue
_sysconfig_get_platform_impl(PyObject *module);

static PyValue
_sysconfig_get_platform(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _sysconfig_get_platform_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#ifndef _SYSCONFIG_GET_PLATFORM_METHODDEF
    #define _SYSCONFIG_GET_PLATFORM_METHODDEF
#endif /* !defined(_SYSCONFIG_GET_PLATFORM_METHODDEF) */
/*[clinic end generated code: output=8d6c164171fbe249 input=a9049054013a1b77]*/
