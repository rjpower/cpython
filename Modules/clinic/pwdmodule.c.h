/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_BadArgument()

PyDoc_STRVAR(pwd_getpwuid__doc__,
"getpwuid($module, uidobj, /)\n"
"--\n"
"\n"
"Return the password database entry for the given numeric user ID.\n"
"\n"
"See `help(pwd)` for more on password database entries.");

#define PWD_GETPWUID_METHODDEF    \
    {"getpwuid", _PyCFunction_CAST(pwd_getpwuid), METH_O, pwd_getpwuid__doc__},

static PyObject *
pwd_getpwuid_impl(PyObject *module, uid_t uid);

static PyValue
pwd_getpwuid(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    uid_t uid;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_Py_Uid_Converter(_arg, &uid)) {
            goto exit;
        }
    }
    _return_value = pwd_getpwuid_impl(module, uid);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(pwd_getpwnam__doc__,
"getpwnam($module, name, /)\n"
"--\n"
"\n"
"Return the password database entry for the given user name.\n"
"\n"
"See `help(pwd)` for more on password database entries.");

#define PWD_GETPWNAM_METHODDEF    \
    {"getpwnam", _PyCFunction_CAST(pwd_getpwnam), METH_O, pwd_getpwnam__doc__},

static PyObject *
pwd_getpwnam_impl(PyObject *module, PyObject *name);

static PyValue
pwd_getpwnam(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *name;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("getpwnam", "argument", "str", _arg);
            goto exit;
        }
        name = _arg;
    }
    _return_value = pwd_getpwnam_impl(module, name);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

#if defined(HAVE_GETPWENT)

PyDoc_STRVAR(pwd_getpwall__doc__,
"getpwall($module, /)\n"
"--\n"
"\n"
"Return a list of all available password database entries, in arbitrary order.\n"
"\n"
"See help(pwd) for more on password database entries.");

#define PWD_GETPWALL_METHODDEF    \
    {"getpwall", (PyCFunction)pwd_getpwall, METH_NOARGS, pwd_getpwall__doc__},

static PyObject *
pwd_getpwall_impl(PyObject *module);

static PyObject *
pwd_getpwall(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return pwd_getpwall_impl(module);
}

#endif /* defined(HAVE_GETPWENT) */

#ifndef PWD_GETPWALL_METHODDEF
    #define PWD_GETPWALL_METHODDEF
#endif /* !defined(PWD_GETPWALL_METHODDEF) */
/*[clinic end generated code: output=9660e1167414a811 input=a9049054013a1b77]*/
