/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_BadArgument()
#include "pycore_tuple.h"         // _PyTuple_FromValues()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()

PyDoc_STRVAR(_tkinter_tkapp_call__doc__,
"call($self, /, *args)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_CALL_METHODDEF    \
    {"call", _PyCFunction_CAST(_tkinter_tkapp_call), METH_FASTCALL, _tkinter_tkapp_call__doc__},

static PyObject *
_tkinter_tkapp_call_impl(TkappObject *self, PyObject *args);

static PyValue
_tkinter_tkapp_call(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *__clinic_args = NULL;
    PyObject *_return_value;

    __clinic_args = _PyTuple_FromValues(args, nargs);
    if (__clinic_args == NULL) {
        goto exit;
    }
    _return_value = _tkinter_tkapp_call_impl((TkappObject *)self, __clinic_args);
    return_value = PyValue_FromResult(_return_value);

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
    {"eval", _PyCFunction_CAST(_tkinter_tkapp_eval), METH_O, _tkinter_tkapp_eval__doc__},

static PyObject *
_tkinter_tkapp_eval_impl(TkappObject *self, const char *script);

static PyValue
_tkinter_tkapp_eval(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    const char *script;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("eval", "argument", "str", _arg);
            goto exit;
        }
        script = _PyUnicode_AsUTF8NoNUL(_arg);
        if (script == NULL) {
            goto exit;
        }
    }
    _return_value = _tkinter_tkapp_eval_impl((TkappObject *)self, script);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_evalfile__doc__,
"evalfile($self, fileName, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_EVALFILE_METHODDEF    \
    {"evalfile", _PyCFunction_CAST(_tkinter_tkapp_evalfile), METH_O, _tkinter_tkapp_evalfile__doc__},

static PyObject *
_tkinter_tkapp_evalfile_impl(TkappObject *self, const char *fileName);

static PyValue
_tkinter_tkapp_evalfile(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    const char *fileName;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("evalfile", "argument", "str", _arg);
            goto exit;
        }
        fileName = _PyUnicode_AsUTF8NoNUL(_arg);
        if (fileName == NULL) {
            goto exit;
        }
    }
    _return_value = _tkinter_tkapp_evalfile_impl((TkappObject *)self, fileName);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_record__doc__,
"record($self, script, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_RECORD_METHODDEF    \
    {"record", _PyCFunction_CAST(_tkinter_tkapp_record), METH_O, _tkinter_tkapp_record__doc__},

static PyObject *
_tkinter_tkapp_record_impl(TkappObject *self, const char *script);

static PyValue
_tkinter_tkapp_record(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    const char *script;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("record", "argument", "str", _arg);
            goto exit;
        }
        script = _PyUnicode_AsUTF8NoNUL(_arg);
        if (script == NULL) {
            goto exit;
        }
    }
    _return_value = _tkinter_tkapp_record_impl((TkappObject *)self, script);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_adderrorinfo__doc__,
"adderrorinfo($self, msg, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_ADDERRORINFO_METHODDEF    \
    {"adderrorinfo", _PyCFunction_CAST(_tkinter_tkapp_adderrorinfo), METH_O, _tkinter_tkapp_adderrorinfo__doc__},

static PyObject *
_tkinter_tkapp_adderrorinfo_impl(TkappObject *self, const char *msg);

static PyValue
_tkinter_tkapp_adderrorinfo(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    const char *msg;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("adderrorinfo", "argument", "str", _arg);
            goto exit;
        }
        msg = _PyUnicode_AsUTF8NoNUL(_arg);
        if (msg == NULL) {
            goto exit;
        }
    }
    _return_value = _tkinter_tkapp_adderrorinfo_impl((TkappObject *)self, msg);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_getint__doc__,
"getint($self, arg, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_GETINT_METHODDEF    \
    {"getint", _PyCFunction_CAST(_tkinter_tkapp_getint), METH_O, _tkinter_tkapp_getint__doc__},

static PyObject *
_tkinter_tkapp_getint_impl(TkappObject *self, PyObject *arg);

static PyValue
_tkinter_tkapp_getint(PyObject *self, PyValue arg_)
{
    PyValue return_value = PyValue_NULL;
    PyObject *arg;
    PyObject *_return_value;

    arg = PyValue_Object(arg_);
    if (arg == NULL) {
        goto exit;
    }
    _return_value = _tkinter_tkapp_getint_impl((TkappObject *)self, arg);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_getdouble__doc__,
"getdouble($self, arg, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_GETDOUBLE_METHODDEF    \
    {"getdouble", _PyCFunction_CAST(_tkinter_tkapp_getdouble), METH_O, _tkinter_tkapp_getdouble__doc__},

static PyObject *
_tkinter_tkapp_getdouble_impl(TkappObject *self, PyObject *arg);

static PyValue
_tkinter_tkapp_getdouble(PyObject *self, PyValue arg_)
{
    PyValue return_value = PyValue_NULL;
    PyObject *arg;
    PyObject *_return_value;

    arg = PyValue_Object(arg_);
    if (arg == NULL) {
        goto exit;
    }
    _return_value = _tkinter_tkapp_getdouble_impl((TkappObject *)self, arg);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_getboolean__doc__,
"getboolean($self, arg, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_GETBOOLEAN_METHODDEF    \
    {"getboolean", _PyCFunction_CAST(_tkinter_tkapp_getboolean), METH_O, _tkinter_tkapp_getboolean__doc__},

static PyObject *
_tkinter_tkapp_getboolean_impl(TkappObject *self, PyObject *arg);

static PyValue
_tkinter_tkapp_getboolean(PyObject *self, PyValue arg_)
{
    PyValue return_value = PyValue_NULL;
    PyObject *arg;
    PyObject *_return_value;

    arg = PyValue_Object(arg_);
    if (arg == NULL) {
        goto exit;
    }
    _return_value = _tkinter_tkapp_getboolean_impl((TkappObject *)self, arg);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_exprstring__doc__,
"exprstring($self, s, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_EXPRSTRING_METHODDEF    \
    {"exprstring", _PyCFunction_CAST(_tkinter_tkapp_exprstring), METH_O, _tkinter_tkapp_exprstring__doc__},

static PyObject *
_tkinter_tkapp_exprstring_impl(TkappObject *self, const char *s);

static PyValue
_tkinter_tkapp_exprstring(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    const char *s;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("exprstring", "argument", "str", _arg);
            goto exit;
        }
        s = _PyUnicode_AsUTF8NoNUL(_arg);
        if (s == NULL) {
            goto exit;
        }
    }
    _return_value = _tkinter_tkapp_exprstring_impl((TkappObject *)self, s);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_exprlong__doc__,
"exprlong($self, s, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_EXPRLONG_METHODDEF    \
    {"exprlong", _PyCFunction_CAST(_tkinter_tkapp_exprlong), METH_O, _tkinter_tkapp_exprlong__doc__},

static PyObject *
_tkinter_tkapp_exprlong_impl(TkappObject *self, const char *s);

static PyValue
_tkinter_tkapp_exprlong(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    const char *s;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("exprlong", "argument", "str", _arg);
            goto exit;
        }
        s = _PyUnicode_AsUTF8NoNUL(_arg);
        if (s == NULL) {
            goto exit;
        }
    }
    _return_value = _tkinter_tkapp_exprlong_impl((TkappObject *)self, s);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_exprdouble__doc__,
"exprdouble($self, s, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_EXPRDOUBLE_METHODDEF    \
    {"exprdouble", _PyCFunction_CAST(_tkinter_tkapp_exprdouble), METH_O, _tkinter_tkapp_exprdouble__doc__},

static PyObject *
_tkinter_tkapp_exprdouble_impl(TkappObject *self, const char *s);

static PyValue
_tkinter_tkapp_exprdouble(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    const char *s;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("exprdouble", "argument", "str", _arg);
            goto exit;
        }
        s = _PyUnicode_AsUTF8NoNUL(_arg);
        if (s == NULL) {
            goto exit;
        }
    }
    _return_value = _tkinter_tkapp_exprdouble_impl((TkappObject *)self, s);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_exprboolean__doc__,
"exprboolean($self, s, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_EXPRBOOLEAN_METHODDEF    \
    {"exprboolean", _PyCFunction_CAST(_tkinter_tkapp_exprboolean), METH_O, _tkinter_tkapp_exprboolean__doc__},

static PyObject *
_tkinter_tkapp_exprboolean_impl(TkappObject *self, const char *s);

static PyValue
_tkinter_tkapp_exprboolean(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    const char *s;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("exprboolean", "argument", "str", _arg);
            goto exit;
        }
        s = _PyUnicode_AsUTF8NoNUL(_arg);
        if (s == NULL) {
            goto exit;
        }
    }
    _return_value = _tkinter_tkapp_exprboolean_impl((TkappObject *)self, s);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_splitlist__doc__,
"splitlist($self, arg, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_SPLITLIST_METHODDEF    \
    {"splitlist", _PyCFunction_CAST(_tkinter_tkapp_splitlist), METH_O, _tkinter_tkapp_splitlist__doc__},

static PyObject *
_tkinter_tkapp_splitlist_impl(TkappObject *self, PyObject *arg);

static PyValue
_tkinter_tkapp_splitlist(PyObject *self, PyValue arg_)
{
    PyValue return_value = PyValue_NULL;
    PyObject *arg;
    PyObject *_return_value;

    arg = PyValue_Object(arg_);
    if (arg == NULL) {
        goto exit;
    }
    _return_value = _tkinter_tkapp_splitlist_impl((TkappObject *)self, arg);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_createcommand__doc__,
"createcommand($self, name, func, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_CREATECOMMAND_METHODDEF    \
    {"createcommand", _PyCFunction_CAST(_tkinter_tkapp_createcommand), METH_FASTCALL, _tkinter_tkapp_createcommand__doc__},

static PyObject *
_tkinter_tkapp_createcommand_impl(TkappObject *self, const char *name,
                                  PyObject *func);

static PyValue
_tkinter_tkapp_createcommand(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    const char *name;
    PyObject *func;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("createcommand", nargs, 2, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("createcommand", "argument 1", "str", _arg);
            goto exit;
        }
        name = _PyUnicode_AsUTF8NoNUL(_arg);
        if (name == NULL) {
            goto exit;
        }
    }
    func = PyValue_Object(args[1]);
    if (func == NULL) {
        goto exit;
    }
    _return_value = _tkinter_tkapp_createcommand_impl((TkappObject *)self, name, func);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_deletecommand__doc__,
"deletecommand($self, name, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_DELETECOMMAND_METHODDEF    \
    {"deletecommand", _PyCFunction_CAST(_tkinter_tkapp_deletecommand), METH_O, _tkinter_tkapp_deletecommand__doc__},

static PyObject *
_tkinter_tkapp_deletecommand_impl(TkappObject *self, const char *name);

static PyValue
_tkinter_tkapp_deletecommand(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    const char *name;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("deletecommand", "argument", "str", _arg);
            goto exit;
        }
        name = _PyUnicode_AsUTF8NoNUL(_arg);
        if (name == NULL) {
            goto exit;
        }
    }
    _return_value = _tkinter_tkapp_deletecommand_impl((TkappObject *)self, name);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

#if defined(HAVE_CREATEFILEHANDLER)

PyDoc_STRVAR(_tkinter_tkapp_createfilehandler__doc__,
"createfilehandler($self, file, mask, func, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_CREATEFILEHANDLER_METHODDEF    \
    {"createfilehandler", _PyCFunction_CAST(_tkinter_tkapp_createfilehandler), METH_FASTCALL, _tkinter_tkapp_createfilehandler__doc__},

static PyObject *
_tkinter_tkapp_createfilehandler_impl(TkappObject *self, PyObject *file,
                                      int mask, PyObject *func);

static PyValue
_tkinter_tkapp_createfilehandler(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *file;
    int mask;
    PyObject *func;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("createfilehandler", nargs, 3, 3)) {
        goto exit;
    }
    file = PyValue_Object(args[0]);
    if (file == NULL) {
        goto exit;
    }
    mask = PyLong_AsIntValue(args[1]);
    if (mask == -1 && PyErr_Occurred()) {
        goto exit;
    }
    func = PyValue_Object(args[2]);
    if (func == NULL) {
        goto exit;
    }
    _return_value = _tkinter_tkapp_createfilehandler_impl((TkappObject *)self, file, mask, func);
    return_value = PyValue_FromResult(_return_value);

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
    {"deletefilehandler", _PyCFunction_CAST(_tkinter_tkapp_deletefilehandler), METH_O, _tkinter_tkapp_deletefilehandler__doc__},

static PyObject *
_tkinter_tkapp_deletefilehandler_impl(TkappObject *self, PyObject *file);

static PyValue
_tkinter_tkapp_deletefilehandler(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *file;
    PyObject *_return_value;

    file = PyValue_Object(arg);
    if (file == NULL) {
        goto exit;
    }
    _return_value = _tkinter_tkapp_deletefilehandler_impl((TkappObject *)self, file);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

#endif /* defined(HAVE_CREATEFILEHANDLER) */

PyDoc_STRVAR(_tkinter_tktimertoken_deletetimerhandler__doc__,
"deletetimerhandler($self, /)\n"
"--\n"
"\n");

#define _TKINTER_TKTIMERTOKEN_DELETETIMERHANDLER_METHODDEF    \
    {"deletetimerhandler", (PyCFunction)_tkinter_tktimertoken_deletetimerhandler, METH_NOARGS, _tkinter_tktimertoken_deletetimerhandler__doc__},

static PyObject *
_tkinter_tktimertoken_deletetimerhandler_impl(TkttObject *self);

static PyObject *
_tkinter_tktimertoken_deletetimerhandler(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _tkinter_tktimertoken_deletetimerhandler_impl((TkttObject *)self);
}

PyDoc_STRVAR(_tkinter_tkapp_createtimerhandler__doc__,
"createtimerhandler($self, milliseconds, func, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_CREATETIMERHANDLER_METHODDEF    \
    {"createtimerhandler", _PyCFunction_CAST(_tkinter_tkapp_createtimerhandler), METH_FASTCALL, _tkinter_tkapp_createtimerhandler__doc__},

static PyObject *
_tkinter_tkapp_createtimerhandler_impl(TkappObject *self, int milliseconds,
                                       PyObject *func);

static PyValue
_tkinter_tkapp_createtimerhandler(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    int milliseconds;
    PyObject *func;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("createtimerhandler", nargs, 2, 2)) {
        goto exit;
    }
    milliseconds = PyLong_AsIntValue(args[0]);
    if (milliseconds == -1 && PyErr_Occurred()) {
        goto exit;
    }
    func = PyValue_Object(args[1]);
    if (func == NULL) {
        goto exit;
    }
    _return_value = _tkinter_tkapp_createtimerhandler_impl((TkappObject *)self, milliseconds, func);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_mainloop__doc__,
"mainloop($self, threshold=0, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_MAINLOOP_METHODDEF    \
    {"mainloop", _PyCFunction_CAST(_tkinter_tkapp_mainloop), METH_FASTCALL, _tkinter_tkapp_mainloop__doc__},

static PyObject *
_tkinter_tkapp_mainloop_impl(TkappObject *self, int threshold);

static PyValue
_tkinter_tkapp_mainloop(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    int threshold = 0;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("mainloop", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    threshold = PyLong_AsIntValue(args[0]);
    if (threshold == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    _return_value = _tkinter_tkapp_mainloop_impl((TkappObject *)self, threshold);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_dooneevent__doc__,
"dooneevent($self, flags=0, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_DOONEEVENT_METHODDEF    \
    {"dooneevent", _PyCFunction_CAST(_tkinter_tkapp_dooneevent), METH_FASTCALL, _tkinter_tkapp_dooneevent__doc__},

static PyObject *
_tkinter_tkapp_dooneevent_impl(TkappObject *self, int flags);

static PyValue
_tkinter_tkapp_dooneevent(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    int flags = 0;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("dooneevent", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    flags = PyLong_AsIntValue(args[0]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    _return_value = _tkinter_tkapp_dooneevent_impl((TkappObject *)self, flags);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_quit__doc__,
"quit($self, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_QUIT_METHODDEF    \
    {"quit", (PyCFunction)_tkinter_tkapp_quit, METH_NOARGS, _tkinter_tkapp_quit__doc__},

static PyObject *
_tkinter_tkapp_quit_impl(TkappObject *self);

static PyObject *
_tkinter_tkapp_quit(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _tkinter_tkapp_quit_impl((TkappObject *)self);
}

PyDoc_STRVAR(_tkinter_tkapp_interpaddr__doc__,
"interpaddr($self, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_INTERPADDR_METHODDEF    \
    {"interpaddr", (PyCFunction)_tkinter_tkapp_interpaddr, METH_NOARGS, _tkinter_tkapp_interpaddr__doc__},

static PyObject *
_tkinter_tkapp_interpaddr_impl(TkappObject *self);

static PyObject *
_tkinter_tkapp_interpaddr(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _tkinter_tkapp_interpaddr_impl((TkappObject *)self);
}

PyDoc_STRVAR(_tkinter_tkapp_loadtk__doc__,
"loadtk($self, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_LOADTK_METHODDEF    \
    {"loadtk", (PyCFunction)_tkinter_tkapp_loadtk, METH_NOARGS, _tkinter_tkapp_loadtk__doc__},

static PyObject *
_tkinter_tkapp_loadtk_impl(TkappObject *self);

static PyObject *
_tkinter_tkapp_loadtk(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _tkinter_tkapp_loadtk_impl((TkappObject *)self);
}

PyDoc_STRVAR(_tkinter_tkapp_wantobjects__doc__,
"wantobjects($self, value=-1, /)\n"
"--\n"
"\n"
"Return the current setting, or set it if the argument is not -1.");

#define _TKINTER_TKAPP_WANTOBJECTS_METHODDEF    \
    {"wantobjects", _PyCFunction_CAST(_tkinter_tkapp_wantobjects), METH_FASTCALL, _tkinter_tkapp_wantobjects__doc__},

static PyObject *
_tkinter_tkapp_wantobjects_impl(TkappObject *self, int value);

static PyValue
_tkinter_tkapp_wantobjects(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    int value = -1;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("wantobjects", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    value = PyLong_AsIntValue(args[0]);
    if (value == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    _return_value = _tkinter_tkapp_wantobjects_impl((TkappObject *)self, value);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_settrace__doc__,
"settrace($self, func, /)\n"
"--\n"
"\n"
"Set the tracing function.");

#define _TKINTER_TKAPP_SETTRACE_METHODDEF    \
    {"settrace", _PyCFunction_CAST(_tkinter_tkapp_settrace), METH_O, _tkinter_tkapp_settrace__doc__},

static PyObject *
_tkinter_tkapp_settrace_impl(TkappObject *self, PyObject *func);

static PyValue
_tkinter_tkapp_settrace(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *func;
    PyObject *_return_value;

    func = PyValue_Object(arg);
    if (func == NULL) {
        goto exit;
    }
    _return_value = _tkinter_tkapp_settrace_impl((TkappObject *)self, func);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_gettrace__doc__,
"gettrace($self, /)\n"
"--\n"
"\n"
"Get the tracing function.");

#define _TKINTER_TKAPP_GETTRACE_METHODDEF    \
    {"gettrace", (PyCFunction)_tkinter_tkapp_gettrace, METH_NOARGS, _tkinter_tkapp_gettrace__doc__},

static PyObject *
_tkinter_tkapp_gettrace_impl(TkappObject *self);

static PyObject *
_tkinter_tkapp_gettrace(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _tkinter_tkapp_gettrace_impl((TkappObject *)self);
}

PyDoc_STRVAR(_tkinter_tkapp_willdispatch__doc__,
"willdispatch($self, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_WILLDISPATCH_METHODDEF    \
    {"willdispatch", (PyCFunction)_tkinter_tkapp_willdispatch, METH_NOARGS, _tkinter_tkapp_willdispatch__doc__},

static PyObject *
_tkinter_tkapp_willdispatch_impl(TkappObject *self);

static PyObject *
_tkinter_tkapp_willdispatch(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _tkinter_tkapp_willdispatch_impl((TkappObject *)self);
}

PyDoc_STRVAR(_tkinter__flatten__doc__,
"_flatten($module, item, /)\n"
"--\n"
"\n");

#define _TKINTER__FLATTEN_METHODDEF    \
    {"_flatten", _PyCFunction_CAST(_tkinter__flatten), METH_O, _tkinter__flatten__doc__},

static PyObject *
_tkinter__flatten_impl(PyObject *module, PyObject *item);

static PyValue
_tkinter__flatten(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *item;
    PyObject *_return_value;

    item = PyValue_Object(arg);
    if (item == NULL) {
        goto exit;
    }
    _return_value = _tkinter__flatten_impl(module, item);
    return_value = PyValue_FromResult(_return_value);

exit:
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
    {"create", _PyCFunction_CAST(_tkinter_create), METH_FASTCALL, _tkinter_create__doc__},

static PyObject *
_tkinter_create_impl(PyObject *module, const char *screenName,
                     const char *baseName, const char *className,
                     int interactive, int wantobjects, int wantTk, int sync,
                     const char *use);

static PyValue
_tkinter_create(PyObject *module, const PyValue *args, Py_ssize_t nargs)
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
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("create", nargs, 0, 8)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (_arg == Py_None) {
            screenName = NULL;
        }
        else if (PyUnicode_Check(_arg)) {
            screenName = _PyUnicode_AsUTF8NoNUL(_arg);
            if (screenName == NULL) {
                goto exit;
            }
        }
        else {
            _PyArg_BadArgument("create", "argument 1", "str or None", _arg);
            goto exit;
        }
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("create", "argument 2", "str", _arg);
            goto exit;
        }
        baseName = _PyUnicode_AsUTF8NoNUL(_arg);
        if (baseName == NULL) {
            goto exit;
        }
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("create", "argument 3", "str", _arg);
            goto exit;
        }
        className = _PyUnicode_AsUTF8NoNUL(_arg);
        if (className == NULL) {
            goto exit;
        }
    }
    if (nargs < 4) {
        goto skip_optional;
    }
    interactive = PyObject_IsTrueValue(args[3]);
    if (interactive < 0) {
        goto exit;
    }
    if (nargs < 5) {
        goto skip_optional;
    }
    wantobjects = PyLong_AsIntValue(args[4]);
    if (wantobjects == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 6) {
        goto skip_optional;
    }
    wantTk = PyObject_IsTrueValue(args[5]);
    if (wantTk < 0) {
        goto exit;
    }
    if (nargs < 7) {
        goto skip_optional;
    }
    sync = PyObject_IsTrueValue(args[6]);
    if (sync < 0) {
        goto exit;
    }
    if (nargs < 8) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[7]);
        if (_arg == NULL) {
            goto exit;
        }
        if (_arg == Py_None) {
            use = NULL;
        }
        else if (PyUnicode_Check(_arg)) {
            use = _PyUnicode_AsUTF8NoNUL(_arg);
            if (use == NULL) {
                goto exit;
            }
        }
        else {
            _PyArg_BadArgument("create", "argument 8", "str or None", _arg);
            goto exit;
        }
    }
skip_optional:
    _return_value = _tkinter_create_impl(module, screenName, baseName, className, interactive, wantobjects, wantTk, sync, use);
    return_value = PyValue_FromResult(_return_value);

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
    {"setbusywaitinterval", _PyCFunction_CAST(_tkinter_setbusywaitinterval), METH_O, _tkinter_setbusywaitinterval__doc__},

static PyObject *
_tkinter_setbusywaitinterval_impl(PyObject *module, int new_val);

static PyValue
_tkinter_setbusywaitinterval(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    int new_val;
    PyObject *_return_value;

    new_val = PyLong_AsIntValue(arg);
    if (new_val == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _tkinter_setbusywaitinterval_impl(module, new_val);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_getbusywaitinterval__doc__,
"getbusywaitinterval($module, /)\n"
"--\n"
"\n"
"Return the current busy-wait interval between successive calls to Tcl_DoOneEvent in a threaded Python interpreter.");

#define _TKINTER_GETBUSYWAITINTERVAL_METHODDEF    \
    {"getbusywaitinterval", (PyCFunction)_tkinter_getbusywaitinterval, METH_NOARGS, _tkinter_getbusywaitinterval__doc__},

static int
_tkinter_getbusywaitinterval_impl(PyObject *module);

static PyObject *
_tkinter_getbusywaitinterval(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = _tkinter_getbusywaitinterval_impl(module);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

#ifndef _TKINTER_TKAPP_CREATEFILEHANDLER_METHODDEF
    #define _TKINTER_TKAPP_CREATEFILEHANDLER_METHODDEF
#endif /* !defined(_TKINTER_TKAPP_CREATEFILEHANDLER_METHODDEF) */

#ifndef _TKINTER_TKAPP_DELETEFILEHANDLER_METHODDEF
    #define _TKINTER_TKAPP_DELETEFILEHANDLER_METHODDEF
#endif /* !defined(_TKINTER_TKAPP_DELETEFILEHANDLER_METHODDEF) */
/*[clinic end generated code: output=3201f7f7d6dbfe65 input=a9049054013a1b77]*/
