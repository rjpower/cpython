/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_value_call.h"    // METH_VALUE

#if defined(HAVE_GETRUSAGE)

PyDoc_STRVAR(resource_getrusage__doc__,
"getrusage($module, who, /)\n"
"--\n"
"\n");

#define RESOURCE_GETRUSAGE_METHODDEF    \
    {"getrusage", _PyCFunction_CAST(resource_getrusage), METH_VALUE|METH_O, resource_getrusage__doc__},

static PyValue
resource_getrusage_impl(PyObject *module, int who);

static PyValue
resource_getrusage(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int who;

    who = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (who == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = resource_getrusage_impl(PyValue_AsKnownObjectBorrow(module), who);

exit:
    return return_value;
}

#endif /* defined(HAVE_GETRUSAGE) */

PyDoc_STRVAR(resource_getrlimit__doc__,
"getrlimit($module, resource, /)\n"
"--\n"
"\n");

#define RESOURCE_GETRLIMIT_METHODDEF    \
    {"getrlimit", _PyCFunction_CAST(resource_getrlimit), METH_VALUE|METH_O, resource_getrlimit__doc__},

static PyValue
resource_getrlimit_impl(PyObject *module, int resource);

static PyValue
resource_getrlimit(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int resource;

    resource = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (resource == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = resource_getrlimit_impl(PyValue_AsKnownObjectBorrow(module), resource);

exit:
    return return_value;
}

PyDoc_STRVAR(resource_setrlimit__doc__,
"setrlimit($module, resource, limits, /)\n"
"--\n"
"\n");

#define RESOURCE_SETRLIMIT_METHODDEF    \
    {"setrlimit", _PyCFunction_CAST(resource_setrlimit), METH_VALUE, resource_setrlimit__doc__},

static PyValue
resource_setrlimit_impl(PyObject *module, int resource, PyValueBorrow limits);

static PyValue
resource_setrlimit(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int resource;
    PyValueBorrow limits;

    if (!_PyArg_CheckPositional("setrlimit", nargs, 2, 2)) {
        goto exit;
    }
    resource = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (resource == -1 && PyErr_Occurred()) {
        goto exit;
    }
    limits = PyValue_Borrow(args[1]);
    return_value = resource_setrlimit_impl(PyValue_AsKnownObjectBorrow(module), resource, limits);

exit:
    return return_value;
}

#if defined(HAVE_PRLIMIT)

PyDoc_STRVAR(resource_prlimit__doc__,
"prlimit($module, pid, resource, limits=None, /)\n"
"--\n"
"\n");

#define RESOURCE_PRLIMIT_METHODDEF    \
    {"prlimit", _PyCFunction_CAST(resource_prlimit), METH_VALUE, resource_prlimit__doc__},

static PyValue
resource_prlimit_impl(PyObject *module, pid_t pid, int resource,
                      PyValueBorrow limits);

static PyValue
resource_prlimit(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    pid_t pid;
    int resource;
    PyValueBorrow limits = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("prlimit", nargs, 2, 3)) {
        goto exit;
    }
    #if !defined(SIZEOF_PID_T) || SIZEOF_PID_T == SIZEOF_INT
    pid = _PyValue_AsInt(PyValue_Borrow(args[0]));
    #elif SIZEOF_PID_T == SIZEOF_LONG
    pid = _PyValue_AsLong(PyValue_Borrow(args[0]));
    #else
    pid = _PyValue_AsLongLong(PyValue_Borrow(args[0]));
    #endif
    if (pid == (pid_t)(-1) && PyErr_Occurred()) {
        goto exit;
    }
    resource = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (resource == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    limits = PyValue_Borrow(args[2]);
skip_optional:
    return_value = resource_prlimit_impl(PyValue_AsKnownObjectBorrow(module), pid, resource, limits);

exit:
    return return_value;
}

#endif /* defined(HAVE_PRLIMIT) */

PyDoc_STRVAR(resource_getpagesize__doc__,
"getpagesize($module, /)\n"
"--\n"
"\n");

#define RESOURCE_GETPAGESIZE_METHODDEF    \
    {"getpagesize", _PyCFunction_CAST(resource_getpagesize), METH_VALUE|METH_NOARGS, resource_getpagesize__doc__},

static int
resource_getpagesize_impl(PyObject *module);

static PyValue
resource_getpagesize(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int _return_value;

    _return_value = resource_getpagesize_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

#ifndef RESOURCE_GETRUSAGE_METHODDEF
    #define RESOURCE_GETRUSAGE_METHODDEF
#endif /* !defined(RESOURCE_GETRUSAGE_METHODDEF) */

#ifndef RESOURCE_PRLIMIT_METHODDEF
    #define RESOURCE_PRLIMIT_METHODDEF
#endif /* !defined(RESOURCE_PRLIMIT_METHODDEF) */
/*[clinic end generated code: output=331bb2a397e9a65f input=a9049054013a1b77]*/
