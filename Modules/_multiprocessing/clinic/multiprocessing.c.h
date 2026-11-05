/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_long_value.h"    // _PyValue_IntAsVoidPtr()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_multiprocessing_closesocket__doc__,
"closesocket($module, handle, /)\n"
"--\n"
"\n");

#define _MULTIPROCESSING_CLOSESOCKET_METHODDEF    \
    {"closesocket", _PyCFunction_CAST(_multiprocessing_closesocket), METH_VALUE|METH_O, _multiprocessing_closesocket__doc__},

static PyValue
_multiprocessing_closesocket_impl(PyObject *module, HANDLE handle);

static PyValue
_multiprocessing_closesocket(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    HANDLE handle;

    handle = _PyValue_IntAsVoidPtr(PyValue_Borrow(args[0]));
    if (!handle && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _multiprocessing_closesocket_impl(PyValue_AsKnownObjectBorrow(module), handle);

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
    {"recv", _PyCFunction_CAST(_multiprocessing_recv), METH_VALUE, _multiprocessing_recv__doc__},

static PyValue
_multiprocessing_recv_impl(PyObject *module, HANDLE handle, int size);

static PyValue
_multiprocessing_recv(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    HANDLE handle;
    int size;

    if (!_PyArg_CheckPositional("recv", nargs, 2, 2)) {
        goto exit;
    }
    handle = _PyValue_IntAsVoidPtr(PyValue_Borrow(args[0]));
    if (!handle && PyErr_Occurred()) {
        goto exit;
    }
    size = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (size == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _multiprocessing_recv_impl(PyValue_AsKnownObjectBorrow(module), handle, size);

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
    {"send", _PyCFunction_CAST(_multiprocessing_send), METH_VALUE, _multiprocessing_send__doc__},

static PyValue
_multiprocessing_send_impl(PyObject *module, HANDLE handle, Py_buffer *buf);

static PyValue
_multiprocessing_send(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    HANDLE handle;
    Py_buffer buf = {NULL, NULL};

    if (!_PyArg_CheckPositional("send", nargs, 2, 2)) {
        goto exit;
    }
    handle = _PyValue_IntAsVoidPtr(PyValue_Borrow(args[0]));
    if (!handle && PyErr_Occurred()) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[1]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])), &buf, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = _multiprocessing_send_impl(PyValue_AsKnownObjectBorrow(module), handle, &buf);

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
    {"sem_unlink", _PyCFunction_CAST(_multiprocessing_sem_unlink), METH_VALUE|METH_O, _multiprocessing_sem_unlink__doc__},

static PyValue
_multiprocessing_sem_unlink_impl(PyObject *module, const char *name);

static PyValue
_multiprocessing_sem_unlink(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *name;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("sem_unlink", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("sem_unlink", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    name = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (name == NULL) {
        goto exit;
    }
    return_value = _multiprocessing_sem_unlink_impl(PyValue_AsKnownObjectBorrow(module), name);

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
/*[clinic end generated code: output=b9a6f03d4c9e1e7b input=a9049054013a1b77]*/
