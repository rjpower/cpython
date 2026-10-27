/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_BadArgument()
#include "pycore_tuple.h"         // _PyTuple_FromValues()

PyDoc_STRVAR(getpath_abspath__doc__,
"abspath($module, path, /)\n"
"--\n"
"\n"
"Return the absolute path.");

#define GETPATH_ABSPATH_METHODDEF    \
    {"abspath", _PyCFunction_CAST(getpath_abspath), METH_O, getpath_abspath__doc__},

static PyObject *
getpath_abspath_impl(PyObject *module, const wchar_t *path);

static PyValue
getpath_abspath(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    const wchar_t *path = NULL;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("abspath", "argument", "str", _arg);
            goto exit;
        }
        path = PyUnicode_AsWideCharString(_arg, NULL);
        if (path == NULL) {
            goto exit;
        }
    }
    _return_value = getpath_abspath_impl(module, path);
    return_value = PyValue_FromResult(_return_value);

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
    {"basename", _PyCFunction_CAST(getpath_basename), METH_O, getpath_basename__doc__},

static PyObject *
getpath_basename_impl(PyObject *module, PyObject *path);

static PyValue
getpath_basename(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *path;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("basename", "argument", "str", _arg);
            goto exit;
        }
        path = _arg;
    }
    _return_value = getpath_basename_impl(module, path);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(getpath_dirname__doc__,
"dirname($module, path, /)\n"
"--\n"
"\n"
"Return the directory component of the path.");

#define GETPATH_DIRNAME_METHODDEF    \
    {"dirname", _PyCFunction_CAST(getpath_dirname), METH_O, getpath_dirname__doc__},

static PyObject *
getpath_dirname_impl(PyObject *module, PyObject *path);

static PyValue
getpath_dirname(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *path;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("dirname", "argument", "str", _arg);
            goto exit;
        }
        path = _arg;
    }
    _return_value = getpath_dirname_impl(module, path);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(getpath_isabs__doc__,
"isabs($module, path, /)\n"
"--\n"
"\n"
"Return True if the path is absolute.");

#define GETPATH_ISABS_METHODDEF    \
    {"isabs", _PyCFunction_CAST(getpath_isabs), METH_O, getpath_isabs__doc__},

static int
getpath_isabs_impl(PyObject *module, const wchar_t *path);

static PyValue
getpath_isabs(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    const wchar_t *path = NULL;
    int _return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("isabs", "argument", "str", _arg);
            goto exit;
        }
        path = PyUnicode_AsWideCharString(_arg, NULL);
        if (path == NULL) {
            goto exit;
        }
    }
    _return_value = getpath_isabs_impl(module, path);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromResult(PyBool_FromLong((long)_return_value));

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
    {"hassuffix", _PyCFunction_CAST(getpath_hassuffix), METH_FASTCALL, getpath_hassuffix__doc__},

static int
getpath_hassuffix_impl(PyObject *module, PyObject *pathobj,
                       PyObject *suffixobj);

static PyValue
getpath_hassuffix(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *pathobj;
    PyObject *suffixobj;
    int _return_value;

    if (!_PyArg_CheckPositional("hassuffix", nargs, 2, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("hassuffix", "argument 1", "str", _arg);
            goto exit;
        }
        pathobj = _arg;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("hassuffix", "argument 2", "str", _arg);
            goto exit;
        }
        suffixobj = _arg;
    }
    _return_value = getpath_hassuffix_impl(module, pathobj, suffixobj);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromResult(PyBool_FromLong((long)_return_value));

exit:
    return return_value;
}

PyDoc_STRVAR(getpath_isdir__doc__,
"isdir($module, path, /)\n"
"--\n"
"\n"
"Return True if the path is a directory.");

#define GETPATH_ISDIR_METHODDEF    \
    {"isdir", _PyCFunction_CAST(getpath_isdir), METH_O, getpath_isdir__doc__},

static int
getpath_isdir_impl(PyObject *module, const wchar_t *path);

static PyValue
getpath_isdir(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    const wchar_t *path = NULL;
    int _return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("isdir", "argument", "str", _arg);
            goto exit;
        }
        path = PyUnicode_AsWideCharString(_arg, NULL);
        if (path == NULL) {
            goto exit;
        }
    }
    _return_value = getpath_isdir_impl(module, path);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromResult(PyBool_FromLong((long)_return_value));

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
    {"isfile", _PyCFunction_CAST(getpath_isfile), METH_O, getpath_isfile__doc__},

static int
getpath_isfile_impl(PyObject *module, PyObject *pathobj);

static PyValue
getpath_isfile(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *pathobj;
    int _return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("isfile", "argument", "str", _arg);
            goto exit;
        }
        pathobj = _arg;
    }
    _return_value = getpath_isfile_impl(module, pathobj);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromResult(PyBool_FromLong((long)_return_value));

exit:
    return return_value;
}

PyDoc_STRVAR(getpath_isxfile__doc__,
"isxfile($module, path, /)\n"
"--\n"
"\n"
"Return True if the path is an executable file.");

#define GETPATH_ISXFILE_METHODDEF    \
    {"isxfile", _PyCFunction_CAST(getpath_isxfile), METH_O, getpath_isxfile__doc__},

static int
getpath_isxfile_impl(PyObject *module, PyObject *pathobj);

static PyValue
getpath_isxfile(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *pathobj;
    int _return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("isxfile", "argument", "str", _arg);
            goto exit;
        }
        pathobj = _arg;
    }
    _return_value = getpath_isxfile_impl(module, pathobj);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromResult(PyBool_FromLong((long)_return_value));

exit:
    return return_value;
}

PyDoc_STRVAR(getpath_joinpath__doc__,
"joinpath($module, /, *args)\n"
"--\n"
"\n"
"Join the path components.");

#define GETPATH_JOINPATH_METHODDEF    \
    {"joinpath", _PyCFunction_CAST(getpath_joinpath), METH_FASTCALL, getpath_joinpath__doc__},

static PyObject *
getpath_joinpath_impl(PyObject *module, PyObject *args);

static PyValue
getpath_joinpath(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *__clinic_args = NULL;
    PyObject *_return_value;

    __clinic_args = _PyTuple_FromValues(args, nargs);
    if (__clinic_args == NULL) {
        goto exit;
    }
    _return_value = getpath_joinpath_impl(module, __clinic_args);
    return_value = PyValue_FromResult(_return_value);

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
    {"readlines", _PyCFunction_CAST(getpath_readlines), METH_O, getpath_readlines__doc__},

static PyObject *
getpath_readlines_impl(PyObject *module, PyObject *pathobj);

static PyValue
getpath_readlines(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *pathobj;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("readlines", "argument", "str", _arg);
            goto exit;
        }
        pathobj = _arg;
    }
    _return_value = getpath_readlines_impl(module, pathobj);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(getpath_realpath__doc__,
"realpath($module, path, /)\n"
"--\n"
"\n"
"Resolve a symlinked file.");

#define GETPATH_REALPATH_METHODDEF    \
    {"realpath", _PyCFunction_CAST(getpath_realpath), METH_O, getpath_realpath__doc__},

static PyObject *
getpath_realpath_impl(PyObject *module, PyObject *pathobj);

static PyValue
getpath_realpath(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *pathobj;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("realpath", "argument", "str", _arg);
            goto exit;
        }
        pathobj = _arg;
    }
    _return_value = getpath_realpath_impl(module, pathobj);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=a34aa6f84370f3b5 input=a9049054013a1b77]*/
