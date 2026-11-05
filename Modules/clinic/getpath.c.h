/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_modsupport.h"    // _PyArg_BadArgument()
#include "pycore_tuple.h"         // _PyTuple_FromValueArray()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(getpath_abspath__doc__,
"abspath($module, path, /)\n"
"--\n"
"\n"
"Return the absolute path.");

#define GETPATH_ABSPATH_METHODDEF    \
    {"abspath", _PyCFunction_CAST(getpath_abspath), METH_VALUE|METH_O, getpath_abspath__doc__},

static PyValue
getpath_abspath_impl(PyObject *module, const wchar_t *path);

static PyValue
getpath_abspath(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const wchar_t *path = NULL;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("abspath", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("abspath", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    path = PyUnicode_AsWideCharString(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), NULL);
    if (path == NULL) {
        goto exit;
    }
    return_value = getpath_abspath_impl(PyValue_AsKnownObjectBorrow(module), path);

exit:
    /* Cleanup for path */
    PyMem_Free((void *)path);

    return return_value;
}

PyDoc_STRVAR(getpath_basename__doc__,
"basename($module, path, /)\n"
"--\n"
"\n"
"Return the final component of the path.");

#define GETPATH_BASENAME_METHODDEF    \
    {"basename", _PyCFunction_CAST(getpath_basename), METH_VALUE|METH_O, getpath_basename__doc__},

static PyValue
getpath_basename_impl(PyObject *module, PyObject *path);

static PyValue
getpath_basename(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *path;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("basename", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("basename", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    path = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = getpath_basename_impl(PyValue_AsKnownObjectBorrow(module), path);

exit:
    return return_value;
}

PyDoc_STRVAR(getpath_dirname__doc__,
"dirname($module, path, /)\n"
"--\n"
"\n"
"Return the directory component of the path.");

#define GETPATH_DIRNAME_METHODDEF    \
    {"dirname", _PyCFunction_CAST(getpath_dirname), METH_VALUE|METH_O, getpath_dirname__doc__},

static PyValue
getpath_dirname_impl(PyObject *module, PyObject *path);

static PyValue
getpath_dirname(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *path;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("dirname", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("dirname", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    path = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = getpath_dirname_impl(PyValue_AsKnownObjectBorrow(module), path);

exit:
    return return_value;
}

PyDoc_STRVAR(getpath_isabs__doc__,
"isabs($module, path, /)\n"
"--\n"
"\n"
"Return True if the path is absolute.");

#define GETPATH_ISABS_METHODDEF    \
    {"isabs", _PyCFunction_CAST(getpath_isabs), METH_VALUE|METH_O, getpath_isabs__doc__},

static int
getpath_isabs_impl(PyObject *module, const wchar_t *path);

static PyValue
getpath_isabs(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const wchar_t *path = NULL;
    int _return_value;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("isabs", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("isabs", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    path = PyUnicode_AsWideCharString(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), NULL);
    if (path == NULL) {
        goto exit;
    }
    _return_value = getpath_isabs_impl(PyValue_AsKnownObjectBorrow(module), path);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    /* Cleanup for path */
    PyMem_Free((void *)path);

    return return_value;
}

PyDoc_STRVAR(getpath_hassuffix__doc__,
"hassuffix($module, path, suffix, /)\n"
"--\n"
"\n"
"Return True if the path ends with the suffix, ignoring the case.");

#define GETPATH_HASSUFFIX_METHODDEF    \
    {"hassuffix", _PyCFunction_CAST(getpath_hassuffix), METH_VALUE, getpath_hassuffix__doc__},

static int
getpath_hassuffix_impl(PyObject *module, PyObject *pathobj,
                       PyObject *suffixobj);

static PyValue
getpath_hassuffix(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *pathobj;
    PyObject *suffixobj;
    int _return_value;

    if (!_PyArg_CheckPositional("hassuffix", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("hassuffix", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("hassuffix", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    pathobj = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("hassuffix", "argument 2", "str", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        _PyArg_BadArgument("hassuffix", "argument 2", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    suffixobj = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1]));
    _return_value = getpath_hassuffix_impl(PyValue_AsKnownObjectBorrow(module), pathobj, suffixobj);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(getpath_isdir__doc__,
"isdir($module, path, /)\n"
"--\n"
"\n"
"Return True if the path is a directory.");

#define GETPATH_ISDIR_METHODDEF    \
    {"isdir", _PyCFunction_CAST(getpath_isdir), METH_VALUE|METH_O, getpath_isdir__doc__},

static int
getpath_isdir_impl(PyObject *module, const wchar_t *path);

static PyValue
getpath_isdir(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const wchar_t *path = NULL;
    int _return_value;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("isdir", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("isdir", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    path = PyUnicode_AsWideCharString(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), NULL);
    if (path == NULL) {
        goto exit;
    }
    _return_value = getpath_isdir_impl(PyValue_AsKnownObjectBorrow(module), path);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    /* Cleanup for path */
    PyMem_Free((void *)path);

    return return_value;
}

PyDoc_STRVAR(getpath_isfile__doc__,
"isfile($module, path, /)\n"
"--\n"
"\n"
"Return True if the path is a regular file.");

#define GETPATH_ISFILE_METHODDEF    \
    {"isfile", _PyCFunction_CAST(getpath_isfile), METH_VALUE|METH_O, getpath_isfile__doc__},

static int
getpath_isfile_impl(PyObject *module, PyObject *pathobj);

static PyValue
getpath_isfile(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *pathobj;
    int _return_value;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("isfile", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("isfile", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    pathobj = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    _return_value = getpath_isfile_impl(PyValue_AsKnownObjectBorrow(module), pathobj);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(getpath_isxfile__doc__,
"isxfile($module, path, /)\n"
"--\n"
"\n"
"Return True if the path is an executable file.");

#define GETPATH_ISXFILE_METHODDEF    \
    {"isxfile", _PyCFunction_CAST(getpath_isxfile), METH_VALUE|METH_O, getpath_isxfile__doc__},

static int
getpath_isxfile_impl(PyObject *module, PyObject *pathobj);

static PyValue
getpath_isxfile(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *pathobj;
    int _return_value;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("isxfile", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("isxfile", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    pathobj = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    _return_value = getpath_isxfile_impl(PyValue_AsKnownObjectBorrow(module), pathobj);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(getpath_joinpath__doc__,
"joinpath($module, /, *args)\n"
"--\n"
"\n"
"Join the path components.");

#define GETPATH_JOINPATH_METHODDEF    \
    {"joinpath", _PyCFunction_CAST(getpath_joinpath), METH_VALUE, getpath_joinpath__doc__},

static PyValue
getpath_joinpath_impl(PyObject *module, PyObject *args);

static PyValue
getpath_joinpath(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *__clinic_args = NULL;

    __clinic_args = _PyTuple_FromValueArray(args, nargs);
    if (__clinic_args == NULL) {
        goto exit;
    }
    return_value = getpath_joinpath_impl(PyValue_AsKnownObjectBorrow(module), __clinic_args);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);

    return return_value;
}

PyDoc_STRVAR(getpath_readlines__doc__,
"readlines($module, path, /)\n"
"--\n"
"\n"
"Return the lines of the file.");

#define GETPATH_READLINES_METHODDEF    \
    {"readlines", _PyCFunction_CAST(getpath_readlines), METH_VALUE|METH_O, getpath_readlines__doc__},

static PyValue
getpath_readlines_impl(PyObject *module, PyObject *pathobj);

static PyValue
getpath_readlines(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *pathobj;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("readlines", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("readlines", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    pathobj = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = getpath_readlines_impl(PyValue_AsKnownObjectBorrow(module), pathobj);

exit:
    return return_value;
}

PyDoc_STRVAR(getpath_realpath__doc__,
"realpath($module, path, /)\n"
"--\n"
"\n"
"Resolve a symlinked file.");

#define GETPATH_REALPATH_METHODDEF    \
    {"realpath", _PyCFunction_CAST(getpath_realpath), METH_VALUE|METH_O, getpath_realpath__doc__},

static PyValue
getpath_realpath_impl(PyObject *module, PyObject *pathobj);

static PyValue
getpath_realpath(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *pathobj;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("realpath", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("realpath", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    pathobj = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = getpath_realpath_impl(PyValue_AsKnownObjectBorrow(module), pathobj);

exit:
    return return_value;
}
/*[clinic end generated code: output=0689196256e73c39 input=a9049054013a1b77]*/
