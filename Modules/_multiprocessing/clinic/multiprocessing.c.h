/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_multiprocessing_closesocket__doc__,
"closesocket($module, handle, /)\n"
"--\n"
"\n");

#define _MULTIPROCESSING_CLOSESOCKET_METHODDEF    \
    {"closesocket", _PyCFunction_CAST(_multiprocessing_closesocket), METH_O, _multiprocessing_closesocket__doc__},

static PyObject *
_multiprocessing_closesocket_impl(PyObject *module, HANDLE handle);

static PyValue
_multiprocessing_closesocket(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    HANDLE handle;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        handle = PyLong_AsVoidPtr(_arg);
        if (!handle && PyErr_Occurred()) {
            goto exit;
        }
    }
    _return_value = _multiprocessing_closesocket_impl(module, handle);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_multiprocessing_recv__doc__,
"recv($module, handle, size, /)\n"
"--\n"
"\n");

#define _MULTIPROCESSING_RECV_METHODDEF    \
    {"recv", _PyCFunction_CAST(_multiprocessing_recv), METH_FASTCALL, _multiprocessing_recv__doc__},

static PyObject *
_multiprocessing_recv_impl(PyObject *module, HANDLE handle, int size);

static PyValue
_multiprocessing_recv(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    HANDLE handle;
    int size;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("recv", nargs, 2, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        handle = PyLong_AsVoidPtr(_arg);
        if (!handle && PyErr_Occurred()) {
            goto exit;
        }
    }
    size = PyLong_AsIntValue(args[1]);
    if (size == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _multiprocessing_recv_impl(module, handle, size);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_multiprocessing_send__doc__,
"send($module, handle, buf, /)\n"
"--\n"
"\n");

#define _MULTIPROCESSING_SEND_METHODDEF    \
    {"send", _PyCFunction_CAST(_multiprocessing_send), METH_FASTCALL, _multiprocessing_send__doc__},

static PyObject *
_multiprocessing_send_impl(PyObject *module, HANDLE handle, Py_buffer *buf);

static PyValue
_multiprocessing_send(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    HANDLE handle;
    Py_buffer buf = {NULL, NULL};
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("send", nargs, 2, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        handle = PyLong_AsVoidPtr(_arg);
        if (!handle && PyErr_Occurred()) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyObject_GetBuffer(_arg, &buf, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    _return_value = _multiprocessing_send_impl(module, handle, &buf);
    return_value = PyValue_FromResult(_return_value);

exit:
    /* Cleanup for buf */
    if (buf.obj) {
       PyBuffer_Release(&buf);
    }

    return return_value;
}

#endif /* defined(MS_WINDOWS) */

PyDoc_STRVAR(_multiprocessing_sem_unlink__doc__,
"sem_unlink($module, name, /)\n"
"--\n"
"\n");

#define _MULTIPROCESSING_SEM_UNLINK_METHODDEF    \
    {"sem_unlink", _PyCFunction_CAST(_multiprocessing_sem_unlink), METH_O, _multiprocessing_sem_unlink__doc__},

static PyObject *
_multiprocessing_sem_unlink_impl(PyObject *module, const char *name);

static PyValue
_multiprocessing_sem_unlink(PyObject *module, PyValue arg)
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
            _PyArg_BadArgument("sem_unlink", "argument", "str", _arg);
            goto exit;
        }
        name = _PyUnicode_AsUTF8NoNUL(_arg);
        if (name == NULL) {
            goto exit;
        }
    }
    _return_value = _multiprocessing_sem_unlink_impl(module, name);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

#ifndef _MULTIPROCESSING_CLOSESOCKET_METHODDEF
    #define _MULTIPROCESSING_CLOSESOCKET_METHODDEF
#endif /* !defined(_MULTIPROCESSING_CLOSESOCKET_METHODDEF) */

#ifndef _MULTIPROCESSING_RECV_METHODDEF
    #define _MULTIPROCESSING_RECV_METHODDEF
#endif /* !defined(_MULTIPROCESSING_RECV_METHODDEF) */

#ifndef _MULTIPROCESSING_SEND_METHODDEF
    #define _MULTIPROCESSING_SEND_METHODDEF
#endif /* !defined(_MULTIPROCESSING_SEND_METHODDEF) */
/*[clinic end generated code: output=cd5ef29335ff5810 input=a9049054013a1b77]*/
