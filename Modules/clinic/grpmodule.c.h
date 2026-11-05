/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_value.h"         // _PyValue_AsObjectAt()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(grp_getgrgid__doc__,
"getgrgid($module, /, id)\n"
"--\n"
"\n"
"Return the group database entry for the given numeric group ID.\n"
"\n"
"If id is not valid, raise KeyError.");

#define GRP_GETGRGID_METHODDEF    \
    {"getgrgid", _PyCFunction_CAST(grp_getgrgid), METH_VALUE|METH_KEYWORDS, grp_getgrgid__doc__},

static PyValue
grp_getgrgid_impl(PyObject *module, gid_t gid);

static PyValue
grp_getgrgid(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(id)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"id", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "getgrgid",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    gid_t gid;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    {
        PyObject *_obj = _PyValue_AsObjectAt(PyValue_Borrow(args[_idx[0]]), BOX_SITE_PARSER);
        if (_obj == NULL) {
            goto exit;
        }
        int _ok;

        _ok = _Py_Gid_Converter(_obj, &gid);

        Py_DECREF(_obj);
        if (!_ok) {
            goto exit;
        }
    }
    return_value = grp_getgrgid_impl(PyValue_AsKnownObjectBorrow(module), gid);

exit:
    return return_value;
}

PyDoc_STRVAR(grp_getgrnam__doc__,
"getgrnam($module, /, name)\n"
"--\n"
"\n"
"Return the group database entry for the given group name.\n"
"\n"
"If name is not valid, raise KeyError.");

#define GRP_GETGRNAM_METHODDEF    \
    {"getgrnam", _PyCFunction_CAST(grp_getgrnam), METH_VALUE|METH_KEYWORDS, grp_getgrnam__doc__},

static PyValue
grp_getgrnam_impl(PyObject *module, PyObject *name);

static PyValue
grp_getgrnam(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(name)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "getgrnam",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyObject *name;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        _PyArg_BadArgumentValue("getgrnam", "argument 'name'", "str", PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
        _PyArg_BadArgument("getgrnam", "argument 'name'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
        goto exit;
    }
    name = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]]));
    return_value = grp_getgrnam_impl(PyValue_AsKnownObjectBorrow(module), name);

exit:
    return return_value;
}

PyDoc_STRVAR(grp_getgrall__doc__,
"getgrall($module, /)\n"
"--\n"
"\n"
"Return a list of all available group entries, in arbitrary order.\n"
"\n"
"An entry whose name starts with \'+\' or \'-\' represents an instruction\n"
"to use YP/NIS and may not be accessible via getgrnam or getgrgid.");

#define GRP_GETGRALL_METHODDEF    \
    {"getgrall", _PyCFunction_CAST(grp_getgrall), METH_VALUE|METH_NOARGS, grp_getgrall__doc__},

static PyValue
grp_getgrall_impl(PyObject *module);

static PyValue
grp_getgrall(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = grp_getgrall_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}
/*[clinic end generated code: output=a3d5bce60e59f38a input=a9049054013a1b77]*/
