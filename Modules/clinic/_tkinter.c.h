/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_modsupport.h"    // _PyArg_BadArgument()
#include "pycore_tuple.h"         // _PyTuple_FromValueArray()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_IsTrue()

PyDoc_STRVAR(_tkinter_tkapp_call__doc__,
"call($self, /, *args)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_CALL_METHODDEF    \
    {"call", _PyCFunction_CAST(_tkinter_tkapp_call), METH_VALUE, _tkinter_tkapp_call__doc__},

static PyValue
_tkinter_tkapp_call_impl(TkappObject *self, PyObject *args);

static PyValue
_tkinter_tkapp_call(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *__clinic_args = NULL;

    __clinic_args = _PyTuple_FromValueArray(args, nargs);
    if (__clinic_args == NULL) {
        goto exit;
    }
    return_value = _tkinter_tkapp_call_impl((TkappObject *)PyValue_AsKnownObjectBorrow(self), __clinic_args);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);

    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_eval__doc__,
"eval($self, script, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_EVAL_METHODDEF    \
    {"eval", _PyCFunction_CAST(_tkinter_tkapp_eval), METH_VALUE|METH_O, _tkinter_tkapp_eval__doc__},

static PyValue
_tkinter_tkapp_eval_impl(TkappObject *self, const char *script);

static PyValue
_tkinter_tkapp_eval(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *script;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("eval", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("eval", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    script = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (script == NULL) {
        goto exit;
    }
    return_value = _tkinter_tkapp_eval_impl((TkappObject *)PyValue_AsKnownObjectBorrow(self), script);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_evalfile__doc__,
"evalfile($self, fileName, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_EVALFILE_METHODDEF    \
    {"evalfile", _PyCFunction_CAST(_tkinter_tkapp_evalfile), METH_VALUE|METH_O, _tkinter_tkapp_evalfile__doc__},

static PyValue
_tkinter_tkapp_evalfile_impl(TkappObject *self, const char *fileName);

static PyValue
_tkinter_tkapp_evalfile(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *fileName;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("evalfile", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("evalfile", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    fileName = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (fileName == NULL) {
        goto exit;
    }
    return_value = _tkinter_tkapp_evalfile_impl((TkappObject *)PyValue_AsKnownObjectBorrow(self), fileName);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_record__doc__,
"record($self, script, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_RECORD_METHODDEF    \
    {"record", _PyCFunction_CAST(_tkinter_tkapp_record), METH_VALUE|METH_O, _tkinter_tkapp_record__doc__},

static PyValue
_tkinter_tkapp_record_impl(TkappObject *self, const char *script);

static PyValue
_tkinter_tkapp_record(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *script;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("record", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("record", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    script = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (script == NULL) {
        goto exit;
    }
    return_value = _tkinter_tkapp_record_impl((TkappObject *)PyValue_AsKnownObjectBorrow(self), script);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_adderrorinfo__doc__,
"adderrorinfo($self, msg, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_ADDERRORINFO_METHODDEF    \
    {"adderrorinfo", _PyCFunction_CAST(_tkinter_tkapp_adderrorinfo), METH_VALUE|METH_O, _tkinter_tkapp_adderrorinfo__doc__},

static PyValue
_tkinter_tkapp_adderrorinfo_impl(TkappObject *self, const char *msg);

static PyValue
_tkinter_tkapp_adderrorinfo(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *msg;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("adderrorinfo", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("adderrorinfo", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    msg = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (msg == NULL) {
        goto exit;
    }
    return_value = _tkinter_tkapp_adderrorinfo_impl((TkappObject *)PyValue_AsKnownObjectBorrow(self), msg);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_getint__doc__,
"getint($self, arg, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_GETINT_METHODDEF    \
    {"getint", _PyCFunction_CAST(_tkinter_tkapp_getint), METH_VALUE|METH_O, _tkinter_tkapp_getint__doc__},

static PyValue
_tkinter_tkapp_getint_impl(TkappObject *self, PyValueBorrow arg);

static PyValue
_tkinter_tkapp_getint(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow arg;

    arg = PyValue_Borrow(args[0]);
    return_value = _tkinter_tkapp_getint_impl((TkappObject *)PyValue_AsKnownObjectBorrow(self), arg);

    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_getdouble__doc__,
"getdouble($self, arg, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_GETDOUBLE_METHODDEF    \
    {"getdouble", _PyCFunction_CAST(_tkinter_tkapp_getdouble), METH_VALUE|METH_O, _tkinter_tkapp_getdouble__doc__},

static PyValue
_tkinter_tkapp_getdouble_impl(TkappObject *self, PyValueBorrow arg);

static PyValue
_tkinter_tkapp_getdouble(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow arg;

    arg = PyValue_Borrow(args[0]);
    return_value = _tkinter_tkapp_getdouble_impl((TkappObject *)PyValue_AsKnownObjectBorrow(self), arg);

    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_getboolean__doc__,
"getboolean($self, arg, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_GETBOOLEAN_METHODDEF    \
    {"getboolean", _PyCFunction_CAST(_tkinter_tkapp_getboolean), METH_VALUE|METH_O, _tkinter_tkapp_getboolean__doc__},

static PyValue
_tkinter_tkapp_getboolean_impl(TkappObject *self, PyValueBorrow arg);

static PyValue
_tkinter_tkapp_getboolean(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow arg;

    arg = PyValue_Borrow(args[0]);
    return_value = _tkinter_tkapp_getboolean_impl((TkappObject *)PyValue_AsKnownObjectBorrow(self), arg);

    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_exprstring__doc__,
"exprstring($self, s, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_EXPRSTRING_METHODDEF    \
    {"exprstring", _PyCFunction_CAST(_tkinter_tkapp_exprstring), METH_VALUE|METH_O, _tkinter_tkapp_exprstring__doc__},

static PyValue
_tkinter_tkapp_exprstring_impl(TkappObject *self, const char *s);

static PyValue
_tkinter_tkapp_exprstring(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *s;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("exprstring", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("exprstring", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    s = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (s == NULL) {
        goto exit;
    }
    return_value = _tkinter_tkapp_exprstring_impl((TkappObject *)PyValue_AsKnownObjectBorrow(self), s);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_exprlong__doc__,
"exprlong($self, s, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_EXPRLONG_METHODDEF    \
    {"exprlong", _PyCFunction_CAST(_tkinter_tkapp_exprlong), METH_VALUE|METH_O, _tkinter_tkapp_exprlong__doc__},

static PyValue
_tkinter_tkapp_exprlong_impl(TkappObject *self, const char *s);

static PyValue
_tkinter_tkapp_exprlong(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *s;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("exprlong", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("exprlong", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    s = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (s == NULL) {
        goto exit;
    }
    return_value = _tkinter_tkapp_exprlong_impl((TkappObject *)PyValue_AsKnownObjectBorrow(self), s);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_exprdouble__doc__,
"exprdouble($self, s, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_EXPRDOUBLE_METHODDEF    \
    {"exprdouble", _PyCFunction_CAST(_tkinter_tkapp_exprdouble), METH_VALUE|METH_O, _tkinter_tkapp_exprdouble__doc__},

static PyValue
_tkinter_tkapp_exprdouble_impl(TkappObject *self, const char *s);

static PyValue
_tkinter_tkapp_exprdouble(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *s;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("exprdouble", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("exprdouble", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    s = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (s == NULL) {
        goto exit;
    }
    return_value = _tkinter_tkapp_exprdouble_impl((TkappObject *)PyValue_AsKnownObjectBorrow(self), s);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_exprboolean__doc__,
"exprboolean($self, s, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_EXPRBOOLEAN_METHODDEF    \
    {"exprboolean", _PyCFunction_CAST(_tkinter_tkapp_exprboolean), METH_VALUE|METH_O, _tkinter_tkapp_exprboolean__doc__},

static PyValue
_tkinter_tkapp_exprboolean_impl(TkappObject *self, const char *s);

static PyValue
_tkinter_tkapp_exprboolean(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *s;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("exprboolean", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("exprboolean", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    s = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (s == NULL) {
        goto exit;
    }
    return_value = _tkinter_tkapp_exprboolean_impl((TkappObject *)PyValue_AsKnownObjectBorrow(self), s);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_splitlist__doc__,
"splitlist($self, arg, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_SPLITLIST_METHODDEF    \
    {"splitlist", _PyCFunction_CAST(_tkinter_tkapp_splitlist), METH_VALUE|METH_O, _tkinter_tkapp_splitlist__doc__},

static PyValue
_tkinter_tkapp_splitlist_impl(TkappObject *self, PyValueBorrow arg);

static PyValue
_tkinter_tkapp_splitlist(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow arg;

    arg = PyValue_Borrow(args[0]);
    return_value = _tkinter_tkapp_splitlist_impl((TkappObject *)PyValue_AsKnownObjectBorrow(self), arg);

    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_createcommand__doc__,
"createcommand($self, name, func, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_CREATECOMMAND_METHODDEF    \
    {"createcommand", _PyCFunction_CAST(_tkinter_tkapp_createcommand), METH_VALUE, _tkinter_tkapp_createcommand__doc__},

static PyValue
_tkinter_tkapp_createcommand_impl(TkappObject *self, const char *name,
                                  PyValueBorrow func);

static PyValue
_tkinter_tkapp_createcommand(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *name;
    PyValueBorrow func;

    if (!_PyArg_CheckPositional("createcommand", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("createcommand", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("createcommand", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    name = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (name == NULL) {
        goto exit;
    }
    func = PyValue_Borrow(args[1]);
    return_value = _tkinter_tkapp_createcommand_impl((TkappObject *)PyValue_AsKnownObjectBorrow(self), name, func);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_deletecommand__doc__,
"deletecommand($self, name, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_DELETECOMMAND_METHODDEF    \
    {"deletecommand", _PyCFunction_CAST(_tkinter_tkapp_deletecommand), METH_VALUE|METH_O, _tkinter_tkapp_deletecommand__doc__},

static PyValue
_tkinter_tkapp_deletecommand_impl(TkappObject *self, const char *name);

static PyValue
_tkinter_tkapp_deletecommand(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *name;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("deletecommand", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("deletecommand", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    name = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (name == NULL) {
        goto exit;
    }
    return_value = _tkinter_tkapp_deletecommand_impl((TkappObject *)PyValue_AsKnownObjectBorrow(self), name);

exit:
    return return_value;
}

#if defined(HAVE_CREATEFILEHANDLER)

PyDoc_STRVAR(_tkinter_tkapp_createfilehandler__doc__,
"createfilehandler($self, file, mask, func, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_CREATEFILEHANDLER_METHODDEF    \
    {"createfilehandler", _PyCFunction_CAST(_tkinter_tkapp_createfilehandler), METH_VALUE, _tkinter_tkapp_createfilehandler__doc__},

static PyValue
_tkinter_tkapp_createfilehandler_impl(TkappObject *self, PyValueBorrow file,
                                      int mask, PyValueBorrow func);

static PyValue
_tkinter_tkapp_createfilehandler(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow file;
    int mask;
    PyValueBorrow func;

    if (!_PyArg_CheckPositional("createfilehandler", nargs, 3, 3)) {
        goto exit;
    }
    file = PyValue_Borrow(args[0]);
    mask = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (mask == -1 && PyErr_Occurred()) {
        goto exit;
    }
    func = PyValue_Borrow(args[2]);
    return_value = _tkinter_tkapp_createfilehandler_impl((TkappObject *)PyValue_AsKnownObjectBorrow(self), file, mask, func);

exit:
    return return_value;
}

#endif /* defined(HAVE_CREATEFILEHANDLER) */

#if defined(HAVE_CREATEFILEHANDLER)

PyDoc_STRVAR(_tkinter_tkapp_deletefilehandler__doc__,
"deletefilehandler($self, file, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_DELETEFILEHANDLER_METHODDEF    \
    {"deletefilehandler", _PyCFunction_CAST(_tkinter_tkapp_deletefilehandler), METH_VALUE|METH_O, _tkinter_tkapp_deletefilehandler__doc__},

static PyValue
_tkinter_tkapp_deletefilehandler_impl(TkappObject *self, PyValueBorrow file);

static PyValue
_tkinter_tkapp_deletefilehandler(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow file;

    file = PyValue_Borrow(args[0]);
    return_value = _tkinter_tkapp_deletefilehandler_impl((TkappObject *)PyValue_AsKnownObjectBorrow(self), file);

    return return_value;
}

#endif /* defined(HAVE_CREATEFILEHANDLER) */

PyDoc_STRVAR(_tkinter_tktimertoken_deletetimerhandler__doc__,
"deletetimerhandler($self, /)\n"
"--\n"
"\n");

#define _TKINTER_TKTIMERTOKEN_DELETETIMERHANDLER_METHODDEF    \
    {"deletetimerhandler", _PyCFunction_CAST(_tkinter_tktimertoken_deletetimerhandler), METH_VALUE|METH_NOARGS, _tkinter_tktimertoken_deletetimerhandler__doc__},

static PyValue
_tkinter_tktimertoken_deletetimerhandler_impl(TkttObject *self);

static PyValue
_tkinter_tktimertoken_deletetimerhandler(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _tkinter_tktimertoken_deletetimerhandler_impl((TkttObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_createtimerhandler__doc__,
"createtimerhandler($self, milliseconds, func, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_CREATETIMERHANDLER_METHODDEF    \
    {"createtimerhandler", _PyCFunction_CAST(_tkinter_tkapp_createtimerhandler), METH_VALUE, _tkinter_tkapp_createtimerhandler__doc__},

static PyValue
_tkinter_tkapp_createtimerhandler_impl(TkappObject *self, int milliseconds,
                                       PyValueBorrow func);

static PyValue
_tkinter_tkapp_createtimerhandler(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int milliseconds;
    PyValueBorrow func;

    if (!_PyArg_CheckPositional("createtimerhandler", nargs, 2, 2)) {
        goto exit;
    }
    milliseconds = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (milliseconds == -1 && PyErr_Occurred()) {
        goto exit;
    }
    func = PyValue_Borrow(args[1]);
    return_value = _tkinter_tkapp_createtimerhandler_impl((TkappObject *)PyValue_AsKnownObjectBorrow(self), milliseconds, func);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_mainloop__doc__,
"mainloop($self, threshold=0, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_MAINLOOP_METHODDEF    \
    {"mainloop", _PyCFunction_CAST(_tkinter_tkapp_mainloop), METH_VALUE, _tkinter_tkapp_mainloop__doc__},

static PyValue
_tkinter_tkapp_mainloop_impl(TkappObject *self, int threshold);

static PyValue
_tkinter_tkapp_mainloop(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int threshold = 0;

    if (!_PyArg_CheckPositional("mainloop", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    threshold = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (threshold == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _tkinter_tkapp_mainloop_impl((TkappObject *)PyValue_AsKnownObjectBorrow(self), threshold);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_dooneevent__doc__,
"dooneevent($self, flags=0, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_DOONEEVENT_METHODDEF    \
    {"dooneevent", _PyCFunction_CAST(_tkinter_tkapp_dooneevent), METH_VALUE, _tkinter_tkapp_dooneevent__doc__},

static PyValue
_tkinter_tkapp_dooneevent_impl(TkappObject *self, int flags);

static PyValue
_tkinter_tkapp_dooneevent(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int flags = 0;

    if (!_PyArg_CheckPositional("dooneevent", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    flags = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _tkinter_tkapp_dooneevent_impl((TkappObject *)PyValue_AsKnownObjectBorrow(self), flags);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_quit__doc__,
"quit($self, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_QUIT_METHODDEF    \
    {"quit", _PyCFunction_CAST(_tkinter_tkapp_quit), METH_VALUE|METH_NOARGS, _tkinter_tkapp_quit__doc__},

static PyValue
_tkinter_tkapp_quit_impl(TkappObject *self);

static PyValue
_tkinter_tkapp_quit(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _tkinter_tkapp_quit_impl((TkappObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_interpaddr__doc__,
"interpaddr($self, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_INTERPADDR_METHODDEF    \
    {"interpaddr", _PyCFunction_CAST(_tkinter_tkapp_interpaddr), METH_VALUE|METH_NOARGS, _tkinter_tkapp_interpaddr__doc__},

static PyValue
_tkinter_tkapp_interpaddr_impl(TkappObject *self);

static PyValue
_tkinter_tkapp_interpaddr(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _tkinter_tkapp_interpaddr_impl((TkappObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_loadtk__doc__,
"loadtk($self, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_LOADTK_METHODDEF    \
    {"loadtk", _PyCFunction_CAST(_tkinter_tkapp_loadtk), METH_VALUE|METH_NOARGS, _tkinter_tkapp_loadtk__doc__},

static PyValue
_tkinter_tkapp_loadtk_impl(TkappObject *self);

static PyValue
_tkinter_tkapp_loadtk(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _tkinter_tkapp_loadtk_impl((TkappObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_wantobjects__doc__,
"wantobjects($self, value=-1, /)\n"
"--\n"
"\n"
"Return the current setting, or set it if the argument is not -1.");

#define _TKINTER_TKAPP_WANTOBJECTS_METHODDEF    \
    {"wantobjects", _PyCFunction_CAST(_tkinter_tkapp_wantobjects), METH_VALUE, _tkinter_tkapp_wantobjects__doc__},

static PyValue
_tkinter_tkapp_wantobjects_impl(TkappObject *self, int value);

static PyValue
_tkinter_tkapp_wantobjects(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int value = -1;

    if (!_PyArg_CheckPositional("wantobjects", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    value = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (value == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _tkinter_tkapp_wantobjects_impl((TkappObject *)PyValue_AsKnownObjectBorrow(self), value);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_settrace__doc__,
"settrace($self, func, /)\n"
"--\n"
"\n"
"Set the tracing function.");

#define _TKINTER_TKAPP_SETTRACE_METHODDEF    \
    {"settrace", _PyCFunction_CAST(_tkinter_tkapp_settrace), METH_VALUE|METH_O, _tkinter_tkapp_settrace__doc__},

static PyValue
_tkinter_tkapp_settrace_impl(TkappObject *self, PyValueBorrow func);

static PyValue
_tkinter_tkapp_settrace(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow func;

    func = PyValue_Borrow(args[0]);
    return_value = _tkinter_tkapp_settrace_impl((TkappObject *)PyValue_AsKnownObjectBorrow(self), func);

    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_gettrace__doc__,
"gettrace($self, /)\n"
"--\n"
"\n"
"Get the tracing function.");

#define _TKINTER_TKAPP_GETTRACE_METHODDEF    \
    {"gettrace", _PyCFunction_CAST(_tkinter_tkapp_gettrace), METH_VALUE|METH_NOARGS, _tkinter_tkapp_gettrace__doc__},

static PyValue
_tkinter_tkapp_gettrace_impl(TkappObject *self);

static PyValue
_tkinter_tkapp_gettrace(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _tkinter_tkapp_gettrace_impl((TkappObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_willdispatch__doc__,
"willdispatch($self, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_WILLDISPATCH_METHODDEF    \
    {"willdispatch", _PyCFunction_CAST(_tkinter_tkapp_willdispatch), METH_VALUE|METH_NOARGS, _tkinter_tkapp_willdispatch__doc__},

static PyValue
_tkinter_tkapp_willdispatch_impl(TkappObject *self);

static PyValue
_tkinter_tkapp_willdispatch(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _tkinter_tkapp_willdispatch_impl((TkappObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_tkinter__flatten__doc__,
"_flatten($module, item, /)\n"
"--\n"
"\n");

#define _TKINTER__FLATTEN_METHODDEF    \
    {"_flatten", _PyCFunction_CAST(_tkinter__flatten), METH_VALUE|METH_O, _tkinter__flatten__doc__},

static PyValue
_tkinter__flatten_impl(PyObject *module, PyValueBorrow item);

static PyValue
_tkinter__flatten(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow item;

    item = PyValue_Borrow(args[0]);
    return_value = _tkinter__flatten_impl(PyValue_AsKnownObjectBorrow(module), item);

    return return_value;
}

PyDoc_STRVAR(_tkinter_create__doc__,
"create($module, screenName=None, baseName=\'\', className=\'Tk\',\n"
"       interactive=False, wantobjects=0, wantTk=True, sync=False,\n"
"       use=None, /)\n"
"--\n"
"\n"
"\n"
"\n"
"  wantTk\n"
"    if false, then Tk_Init() doesn\'t get called\n"
"  sync\n"
"    if true, then pass -sync to wish\n"
"  use\n"
"    if not None, then pass -use to wish");

#define _TKINTER_CREATE_METHODDEF    \
    {"create", _PyCFunction_CAST(_tkinter_create), METH_VALUE, _tkinter_create__doc__},

static PyValue
_tkinter_create_impl(PyObject *module, const char *screenName,
                     const char *baseName, const char *className,
                     int interactive, int wantobjects, int wantTk, int sync,
                     const char *use);

static PyValue
_tkinter_create(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *screenName = NULL;
    const char *baseName = "";
    const char *className = "Tk";
    int interactive = 0;
    int wantobjects = 0;
    int wantTk = 1;
    int sync = 0;
    const char *use = NULL;

    if (!_PyArg_CheckPositional("create", nargs, 0, 8)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("create", "argument 1", "str or None", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])) == Py_None) {
        screenName = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        screenName = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        if (screenName == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("create", "argument 1", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("create", "argument 2", "str", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        _PyArg_BadArgument("create", "argument 2", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    baseName = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
    if (baseName == NULL) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[2]))) {
        _PyArg_BadArgumentValue("create", "argument 3", "str", PyValue_Borrow(args[2]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[2])))) {
        _PyArg_BadArgument("create", "argument 3", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[2])));
        goto exit;
    }
    className = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[2])));
    if (className == NULL) {
        goto exit;
    }
    if (nargs < 4) {
        goto skip_optional;
    }
    interactive = _PyValue_IsTrue(PyValue_Borrow(args[3]));
    if (interactive < 0) {
        goto exit;
    }
    if (nargs < 5) {
        goto skip_optional;
    }
    wantobjects = _PyValue_AsInt(PyValue_Borrow(args[4]));
    if (wantobjects == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 6) {
        goto skip_optional;
    }
    wantTk = _PyValue_IsTrue(PyValue_Borrow(args[5]));
    if (wantTk < 0) {
        goto exit;
    }
    if (nargs < 7) {
        goto skip_optional;
    }
    sync = _PyValue_IsTrue(PyValue_Borrow(args[6]));
    if (sync < 0) {
        goto exit;
    }
    if (nargs < 8) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[7]))) {
        _PyArg_BadArgumentValue("create", "argument 8", "str or None", PyValue_Borrow(args[7]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[7])) == Py_None) {
        use = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[7])))) {
        use = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[7])));
        if (use == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("create", "argument 8", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[7])));
        goto exit;
    }
skip_optional:
    return_value = _tkinter_create_impl(PyValue_AsKnownObjectBorrow(module), screenName, baseName, className, interactive, wantobjects, wantTk, sync, use);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_setbusywaitinterval__doc__,
"setbusywaitinterval($module, new_val, /)\n"
"--\n"
"\n"
"Set the busy-wait interval in milliseconds between successive calls to Tcl_DoOneEvent in a threaded Python interpreter.\n"
"\n"
"It should be set to a divisor of the maximum time between frames in\n"
"an animation.");

#define _TKINTER_SETBUSYWAITINTERVAL_METHODDEF    \
    {"setbusywaitinterval", _PyCFunction_CAST(_tkinter_setbusywaitinterval), METH_VALUE|METH_O, _tkinter_setbusywaitinterval__doc__},

static PyValue
_tkinter_setbusywaitinterval_impl(PyObject *module, int new_val);

static PyValue
_tkinter_setbusywaitinterval(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int new_val;

    new_val = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (new_val == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _tkinter_setbusywaitinterval_impl(PyValue_AsKnownObjectBorrow(module), new_val);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_getbusywaitinterval__doc__,
"getbusywaitinterval($module, /)\n"
"--\n"
"\n"
"Return the current busy-wait interval between successive calls to Tcl_DoOneEvent in a threaded Python interpreter.");

#define _TKINTER_GETBUSYWAITINTERVAL_METHODDEF    \
    {"getbusywaitinterval", _PyCFunction_CAST(_tkinter_getbusywaitinterval), METH_VALUE|METH_NOARGS, _tkinter_getbusywaitinterval__doc__},

static int
_tkinter_getbusywaitinterval_impl(PyObject *module);

static PyValue
_tkinter_getbusywaitinterval(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int _return_value;

    _return_value = _tkinter_getbusywaitinterval_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

#ifndef _TKINTER_TKAPP_CREATEFILEHANDLER_METHODDEF
    #define _TKINTER_TKAPP_CREATEFILEHANDLER_METHODDEF
#endif /* !defined(_TKINTER_TKAPP_CREATEFILEHANDLER_METHODDEF) */

#ifndef _TKINTER_TKAPP_DELETEFILEHANDLER_METHODDEF
    #define _TKINTER_TKAPP_DELETEFILEHANDLER_METHODDEF
#endif /* !defined(_TKINTER_TKAPP_DELETEFILEHANDLER_METHODDEF) */
/*[clinic end generated code: output=caa82fd9bcf3866c input=a9049054013a1b77]*/
