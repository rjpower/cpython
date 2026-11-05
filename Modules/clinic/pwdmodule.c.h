/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_modsupport.h"    // _PyArg_BadArgument()
#include "pycore_value.h"         // _PyValue_AsObjectAt()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(pwd_getpwuid__doc__,
"getpwuid($module, uidobj, /)\n"
"--\n"
"\n"
"Return the password database entry for the given numeric user ID.\n"
"\n"
"See `help(pwd)` for more on password database entries.");

#define PWD_GETPWUID_METHODDEF    \
    {"getpwuid", _PyCFunction_CAST(pwd_getpwuid), METH_VALUE|METH_O, pwd_getpwuid__doc__},

static PyValue
pwd_getpwuid_impl(PyObject *module, uid_t uid);

static PyValue
pwd_getpwuid(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    uid_t uid;

    {
        PyObject *_obj = _PyValue_AsObjectAt(PyValue_Borrow(args[0]), BOX_SITE_PARSER);
        if (_obj == NULL) {
            goto exit;
        }
        int _ok;

        _ok = _Py_Uid_Converter(_obj, &uid);

        Py_DECREF(_obj);
        if (!_ok) {
            goto exit;
        }
    }
    return_value = pwd_getpwuid_impl(PyValue_AsKnownObjectBorrow(module), uid);

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
    {"getpwnam", _PyCFunction_CAST(pwd_getpwnam), METH_VALUE|METH_O, pwd_getpwnam__doc__},

static PyValue
pwd_getpwnam_impl(PyObject *module, PyObject *name);

static PyValue
pwd_getpwnam(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *name;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("getpwnam", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("getpwnam", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    name = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = pwd_getpwnam_impl(PyValue_AsKnownObjectBorrow(module), name);

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
    {"getpwall", _PyCFunction_CAST(pwd_getpwall), METH_VALUE|METH_NOARGS, pwd_getpwall__doc__},

static PyValue
pwd_getpwall_impl(PyObject *module);

static PyValue
pwd_getpwall(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = pwd_getpwall_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* defined(HAVE_GETPWENT) */

#ifndef PWD_GETPWALL_METHODDEF
    #define PWD_GETPWALL_METHODDEF
#endif /* !defined(PWD_GETPWALL_METHODDEF) */
/*[clinic end generated code: output=02eb9bcdbfe72d4f input=a9049054013a1b77]*/
