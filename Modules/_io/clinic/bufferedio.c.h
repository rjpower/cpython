/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#  include "pycore_value.h"       // _PyValue_STATIC_IMMORTAL()
#endif
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_modsupport.h"    // _PyArg_BadArgument()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_Index()

PyDoc_STRVAR(_io__BufferedIOBase_readinto__doc__,
"readinto($self, buffer, /)\n"
"--\n"
"\n");

#define _IO__BUFFEREDIOBASE_READINTO_METHODDEF    \
    {"readinto", _PyCFunction_CAST(_io__BufferedIOBase_readinto), METH_VALUE|METH_O, _io__BufferedIOBase_readinto__doc__},

static PyValue
_io__BufferedIOBase_readinto_impl(PyObject *self, Py_buffer *buffer);

static PyValue
_io__BufferedIOBase_readinto(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer buffer = {NULL, NULL};

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("readinto", "argument", "read-write bytes-like object", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &buffer, PyBUF_WRITABLE) < 0) {
        _PyArg_BadArgument("readinto", "argument", "read-write bytes-like object", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io__BufferedIOBase_readinto_impl(PyValue_AsKnownObjectBorrow(self), &buffer);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(_io__BufferedIOBase_readinto1__doc__,
"readinto1($self, buffer, /)\n"
"--\n"
"\n");

#define _IO__BUFFEREDIOBASE_READINTO1_METHODDEF    \
    {"readinto1", _PyCFunction_CAST(_io__BufferedIOBase_readinto1), METH_VALUE|METH_O, _io__BufferedIOBase_readinto1__doc__},

static PyValue
_io__BufferedIOBase_readinto1_impl(PyObject *self, Py_buffer *buffer);

static PyValue
_io__BufferedIOBase_readinto1(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer buffer = {NULL, NULL};

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("readinto1", "argument", "read-write bytes-like object", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &buffer, PyBUF_WRITABLE) < 0) {
        _PyArg_BadArgument("readinto1", "argument", "read-write bytes-like object", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io__BufferedIOBase_readinto1_impl(PyValue_AsKnownObjectBorrow(self), &buffer);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(_io__BufferedIOBase_detach__doc__,
"detach($self, /)\n"
"--\n"
"\n"
"Disconnect this buffer from its underlying raw stream and return it.\n"
"\n"
"After the raw stream has been detached, the buffer is in an unusable\n"
"state.");

#define _IO__BUFFEREDIOBASE_DETACH_METHODDEF    \
    {"detach", _PyCFunction_CAST(_io__BufferedIOBase_detach), METH_VALUE|METH_METHOD, _io__BufferedIOBase_detach__doc__},

static PyValue
_io__BufferedIOBase_detach_impl(PyObject *self, PyTypeObject *cls);

static PyValue
_io__BufferedIOBase_detach(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "detach() takes no arguments");
        goto exit;
    }
    return_value = _io__BufferedIOBase_detach_impl(PyValue_AsKnownObjectBorrow(self), cls);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__BufferedIOBase_read__doc__,
"read($self, size=-1, /)\n"
"--\n"
"\n"
"Read and return up to n bytes.\n"
"\n"
"If the size argument is omitted, None, or negative, read and\n"
"return all data until EOF.\n"
"\n"
"If the size argument is positive, and the underlying raw stream is\n"
"not \'interactive\', multiple raw reads may be issued to satisfy\n"
"the byte count (unless EOF is reached first).\n"
"However, for interactive raw streams (as well as sockets and pipes),\n"
"at most one raw read will be issued, and a short result does not\n"
"imply that EOF is imminent.\n"
"\n"
"Return an empty bytes object on EOF.\n"
"\n"
"Return None if the underlying raw stream was open in non-blocking\n"
"mode and no data is available at the moment.");

#define _IO__BUFFEREDIOBASE_READ_METHODDEF    \
    {"read", _PyCFunction_CAST(_io__BufferedIOBase_read), METH_VALUE|METH_METHOD, _io__BufferedIOBase_read__doc__},

static PyValue
_io__BufferedIOBase_read_impl(PyObject *self, PyTypeObject *cls,
                              int Py_UNUSED(size));

static PyValue
_io__BufferedIOBase_read(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int size = -1;

    if (!_PyArg_NoKwnames("read", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("read", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    size = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (size == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _io__BufferedIOBase_read_impl(PyValue_AsKnownObjectBorrow(self), cls, size);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__BufferedIOBase_read1__doc__,
"read1($self, size=-1, /)\n"
"--\n"
"\n"
"Read and return up to size bytes, with at most one read() call to the underlying raw stream.\n"
"\n"
"Return an empty bytes object on EOF.\n"
"A short result does not imply that EOF is imminent.");

#define _IO__BUFFEREDIOBASE_READ1_METHODDEF    \
    {"read1", _PyCFunction_CAST(_io__BufferedIOBase_read1), METH_VALUE|METH_METHOD, _io__BufferedIOBase_read1__doc__},

static PyValue
_io__BufferedIOBase_read1_impl(PyObject *self, PyTypeObject *cls,
                               int Py_UNUSED(size));

static PyValue
_io__BufferedIOBase_read1(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int size = -1;

    if (!_PyArg_NoKwnames("read1", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("read1", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    size = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (size == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _io__BufferedIOBase_read1_impl(PyValue_AsKnownObjectBorrow(self), cls, size);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__BufferedIOBase_write__doc__,
"write($self, b, /)\n"
"--\n"
"\n"
"Write buffer b to the IO stream.\n"
"\n"
"Return the number of bytes written, which is always\n"
"the length of b in bytes.\n"
"\n"
"Raise BlockingIOError if the buffer is full and the\n"
"underlying raw stream cannot accept more data at the moment.");

#define _IO__BUFFEREDIOBASE_WRITE_METHODDEF    \
    {"write", _PyCFunction_CAST(_io__BufferedIOBase_write), METH_VALUE|METH_METHOD, _io__BufferedIOBase_write__doc__},

static PyValue
_io__BufferedIOBase_write_impl(PyObject *self, PyTypeObject *cls,
                               PyValueBorrow Py_UNUSED(b));

static PyValue
_io__BufferedIOBase_write(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow b;

    if (!_PyArg_NoKwnames("write", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("write", nargs, 1, 1)) {
        goto exit;
    }
    b = PyValue_Borrow(args[0]);
    return_value = _io__BufferedIOBase_write_impl(PyValue_AsKnownObjectBorrow(self), cls, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__Buffered___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED___SIZEOF___METHODDEF    \
    {"__sizeof__", _PyCFunction_CAST(_io__Buffered___sizeof__), METH_VALUE|METH_NOARGS, _io__Buffered___sizeof____doc__},

static PyValue
_io__Buffered___sizeof___impl(buffered *self);

static PyValue
_io__Buffered___sizeof__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io__Buffered___sizeof___impl((buffered *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io__Buffered__dealloc_warn__doc__,
"_dealloc_warn($self, source, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED__DEALLOC_WARN_METHODDEF    \
    {"_dealloc_warn", _PyCFunction_CAST(_io__Buffered__dealloc_warn), METH_VALUE|METH_O, _io__Buffered__dealloc_warn__doc__},

static PyValue
_io__Buffered__dealloc_warn_impl(buffered *self, PyValueBorrow source);

static PyValue
_io__Buffered__dealloc_warn(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow source;

    source = PyValue_Borrow(args[0]);
    return_value = _io__Buffered__dealloc_warn_impl((buffered *)PyValue_AsKnownObjectBorrow(self), source);

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_simple_flush__doc__,
"flush($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_SIMPLE_FLUSH_METHODDEF    \
    {"flush", _PyCFunction_CAST(_io__Buffered_simple_flush), METH_VALUE|METH_NOARGS, _io__Buffered_simple_flush__doc__},

static PyValue
_io__Buffered_simple_flush_impl(buffered *self);

static PyValue
_io__Buffered_simple_flush(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io__Buffered_simple_flush_impl((buffered *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
_io__Buffered_closed_get_impl(buffered *self);

static PyObject *
_io__Buffered_closed_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_closed_get_impl((buffered *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_close__doc__,
"close($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_CLOSE_METHODDEF    \
    {"close", _PyCFunction_CAST(_io__Buffered_close), METH_VALUE|METH_NOARGS, _io__Buffered_close__doc__},

static PyValue
_io__Buffered_close_impl(buffered *self);

static PyValue
_io__Buffered_close(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io__Buffered_close_impl((buffered *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_detach__doc__,
"detach($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_DETACH_METHODDEF    \
    {"detach", _PyCFunction_CAST(_io__Buffered_detach), METH_VALUE|METH_NOARGS, _io__Buffered_detach__doc__},

static PyValue
_io__Buffered_detach_impl(buffered *self);

static PyValue
_io__Buffered_detach(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io__Buffered_detach_impl((buffered *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_seekable__doc__,
"seekable($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_SEEKABLE_METHODDEF    \
    {"seekable", _PyCFunction_CAST(_io__Buffered_seekable), METH_VALUE|METH_NOARGS, _io__Buffered_seekable__doc__},

static PyValue
_io__Buffered_seekable_impl(buffered *self);

static PyValue
_io__Buffered_seekable(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io__Buffered_seekable_impl((buffered *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_readable__doc__,
"readable($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_READABLE_METHODDEF    \
    {"readable", _PyCFunction_CAST(_io__Buffered_readable), METH_VALUE|METH_NOARGS, _io__Buffered_readable__doc__},

static PyValue
_io__Buffered_readable_impl(buffered *self);

static PyValue
_io__Buffered_readable(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io__Buffered_readable_impl((buffered *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_writable__doc__,
"writable($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_WRITABLE_METHODDEF    \
    {"writable", _PyCFunction_CAST(_io__Buffered_writable), METH_VALUE|METH_NOARGS, _io__Buffered_writable__doc__},

static PyValue
_io__Buffered_writable_impl(buffered *self);

static PyValue
_io__Buffered_writable(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io__Buffered_writable_impl((buffered *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
_io__Buffered_name_get_impl(buffered *self);

static PyObject *
_io__Buffered_name_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_name_get_impl((buffered *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
_io__Buffered_mode_get_impl(buffered *self);

static PyObject *
_io__Buffered_mode_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_mode_get_impl((buffered *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_fileno__doc__,
"fileno($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_FILENO_METHODDEF    \
    {"fileno", _PyCFunction_CAST(_io__Buffered_fileno), METH_VALUE|METH_NOARGS, _io__Buffered_fileno__doc__},

static PyValue
_io__Buffered_fileno_impl(buffered *self);

static PyValue
_io__Buffered_fileno(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io__Buffered_fileno_impl((buffered *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_isatty__doc__,
"isatty($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_ISATTY_METHODDEF    \
    {"isatty", _PyCFunction_CAST(_io__Buffered_isatty), METH_VALUE|METH_NOARGS, _io__Buffered_isatty__doc__},

static PyValue
_io__Buffered_isatty_impl(buffered *self);

static PyValue
_io__Buffered_isatty(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io__Buffered_isatty_impl((buffered *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_flush__doc__,
"flush($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_FLUSH_METHODDEF    \
    {"flush", _PyCFunction_CAST(_io__Buffered_flush), METH_VALUE|METH_NOARGS, _io__Buffered_flush__doc__},

static PyValue
_io__Buffered_flush_impl(buffered *self);

static PyValue
_io__Buffered_flush(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io__Buffered_flush_impl((buffered *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_peek__doc__,
"peek($self, size=0, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_PEEK_METHODDEF    \
    {"peek", _PyCFunction_CAST(_io__Buffered_peek), METH_VALUE, _io__Buffered_peek__doc__},

static PyValue
_io__Buffered_peek_impl(buffered *self, Py_ssize_t size);

static PyValue
_io__Buffered_peek(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t size = 0;

    if (!_PyArg_NoKwnamesValue("_Buffered.peek()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("peek", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[0]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        size = ival;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io__Buffered_peek_impl((buffered *)PyValue_AsKnownObjectBorrow(self), size);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io__Buffered_read__doc__,
"read($self, size=-1, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_READ_METHODDEF    \
    {"read", _PyCFunction_CAST(_io__Buffered_read), METH_VALUE, _io__Buffered_read__doc__},

static PyValue
_io__Buffered_read_impl(buffered *self, Py_ssize_t n);

static PyValue
_io__Buffered_read(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t n = -1;

    if (!_PyArg_NoKwnamesValue("_Buffered.read()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("read", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (!_Py_convert_optional_to_ssize_t_value(PyValue_Borrow(args[0]), &n)) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io__Buffered_read_impl((buffered *)PyValue_AsKnownObjectBorrow(self), n);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io__Buffered_read1__doc__,
"read1($self, size=-1, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_READ1_METHODDEF    \
    {"read1", _PyCFunction_CAST(_io__Buffered_read1), METH_VALUE, _io__Buffered_read1__doc__},

static PyValue
_io__Buffered_read1_impl(buffered *self, Py_ssize_t n);

static PyValue
_io__Buffered_read1(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t n = -1;

    if (!_PyArg_NoKwnamesValue("_Buffered.read1()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("read1", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[0]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        n = ival;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io__Buffered_read1_impl((buffered *)PyValue_AsKnownObjectBorrow(self), n);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io__Buffered_readinto__doc__,
"readinto($self, buffer, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_READINTO_METHODDEF    \
    {"readinto", _PyCFunction_CAST(_io__Buffered_readinto), METH_VALUE|METH_O, _io__Buffered_readinto__doc__},

static PyValue
_io__Buffered_readinto_impl(buffered *self, Py_buffer *buffer);

static PyValue
_io__Buffered_readinto(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer buffer = {NULL, NULL};

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("readinto", "argument", "read-write bytes-like object", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &buffer, PyBUF_WRITABLE) < 0) {
        _PyArg_BadArgument("readinto", "argument", "read-write bytes-like object", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io__Buffered_readinto_impl((buffered *)PyValue_AsKnownObjectBorrow(self), &buffer);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_readinto1__doc__,
"readinto1($self, buffer, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_READINTO1_METHODDEF    \
    {"readinto1", _PyCFunction_CAST(_io__Buffered_readinto1), METH_VALUE|METH_O, _io__Buffered_readinto1__doc__},

static PyValue
_io__Buffered_readinto1_impl(buffered *self, Py_buffer *buffer);

static PyValue
_io__Buffered_readinto1(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer buffer = {NULL, NULL};

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("readinto1", "argument", "read-write bytes-like object", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &buffer, PyBUF_WRITABLE) < 0) {
        _PyArg_BadArgument("readinto1", "argument", "read-write bytes-like object", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io__Buffered_readinto1_impl((buffered *)PyValue_AsKnownObjectBorrow(self), &buffer);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_readline__doc__,
"readline($self, size=-1, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_READLINE_METHODDEF    \
    {"readline", _PyCFunction_CAST(_io__Buffered_readline), METH_VALUE, _io__Buffered_readline__doc__},

static PyValue
_io__Buffered_readline_impl(buffered *self, Py_ssize_t size);

static PyValue
_io__Buffered_readline(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t size = -1;

    if (!_PyArg_NoKwnamesValue("_Buffered.readline()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("readline", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (!_Py_convert_optional_to_ssize_t_value(PyValue_Borrow(args[0]), &size)) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io__Buffered_readline_impl((buffered *)PyValue_AsKnownObjectBorrow(self), size);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io__Buffered_tell__doc__,
"tell($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_TELL_METHODDEF    \
    {"tell", _PyCFunction_CAST(_io__Buffered_tell), METH_VALUE|METH_NOARGS, _io__Buffered_tell__doc__},

static PyValue
_io__Buffered_tell_impl(buffered *self);

static PyValue
_io__Buffered_tell(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io__Buffered_tell_impl((buffered *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_seek__doc__,
"seek($self, target, whence=0, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_SEEK_METHODDEF    \
    {"seek", _PyCFunction_CAST(_io__Buffered_seek), METH_VALUE, _io__Buffered_seek__doc__},

static PyValue
_io__Buffered_seek_impl(buffered *self, PyValueBorrow targetobj, int whence);

static PyValue
_io__Buffered_seek(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow targetobj;
    int whence = 0;

    if (!_PyArg_NoKwnamesValue("_Buffered.seek()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("seek", nargs, 1, 2)) {
        goto exit;
    }
    targetobj = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    whence = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (whence == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io__Buffered_seek_impl((buffered *)PyValue_AsKnownObjectBorrow(self), targetobj, whence);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io__Buffered_truncate__doc__,
"truncate($self, pos=None, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_TRUNCATE_METHODDEF    \
    {"truncate", _PyCFunction_CAST(_io__Buffered_truncate), METH_VALUE|METH_METHOD, _io__Buffered_truncate__doc__},

static PyValue
_io__Buffered_truncate_impl(buffered *self, PyTypeObject *cls,
                            PyValueBorrow pos);

static PyValue
_io__Buffered_truncate(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow pos = PyValue_Borrow(PyValue_None);

    if (!_PyArg_NoKwnames("truncate", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("truncate", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    pos = PyValue_Borrow(args[0]);
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io__Buffered_truncate_impl((buffered *)PyValue_AsKnownObjectBorrow(self), cls, pos);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BufferedReader___init____doc__,
"BufferedReader(raw, buffer_size=DEFAULT_BUFFER_SIZE)\n"
"--\n"
"\n"
"Create a new buffered reader using the given readable raw IO object.");

static int
_io_BufferedReader___init___impl(buffered *self, PyValueBorrow raw,
                                 Py_ssize_t buffer_size);

static int
_io_BufferedReader___init___value(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(raw)), _PyValue_STATIC_IMMORTAL(&_Py_ID(buffer_size)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"raw", "buffer_size", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "BufferedReader",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow raw;
    Py_ssize_t buffer_size = DEFAULT_BUFFER_SIZE;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    raw = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        {
            Py_ssize_t ival = -1;
            PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[1]]));
            if (!PyValue_IsNull(iobj)) {
                ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
                PyValue_CLOSE(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            buffer_size = ival;
        }
    }
    return_value = _io_BufferedReader___init___impl((buffered *)self, raw, buffer_size);

exit:
    return return_value;
}

static int
_io_BufferedReader___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _io_BufferedReader___init___value(self, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = _io_BufferedReader___init___value(self, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}

PyDoc_STRVAR(_io_BufferedWriter___init____doc__,
"BufferedWriter(raw, buffer_size=DEFAULT_BUFFER_SIZE)\n"
"--\n"
"\n"
"A buffer for a writeable sequential RawIO object.\n"
"\n"
"The constructor creates a BufferedWriter for the given writeable raw\n"
"stream. If the buffer_size is not given, it defaults to\n"
"DEFAULT_BUFFER_SIZE.");

static int
_io_BufferedWriter___init___impl(buffered *self, PyValueBorrow raw,
                                 Py_ssize_t buffer_size);

static int
_io_BufferedWriter___init___value(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(raw)), _PyValue_STATIC_IMMORTAL(&_Py_ID(buffer_size)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"raw", "buffer_size", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "BufferedWriter",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow raw;
    Py_ssize_t buffer_size = DEFAULT_BUFFER_SIZE;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    raw = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        {
            Py_ssize_t ival = -1;
            PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[1]]));
            if (!PyValue_IsNull(iobj)) {
                ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
                PyValue_CLOSE(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            buffer_size = ival;
        }
    }
    return_value = _io_BufferedWriter___init___impl((buffered *)self, raw, buffer_size);

exit:
    return return_value;
}

static int
_io_BufferedWriter___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _io_BufferedWriter___init___value(self, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = _io_BufferedWriter___init___value(self, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}

PyDoc_STRVAR(_io_BufferedWriter_write__doc__,
"write($self, buffer, /)\n"
"--\n"
"\n");

#define _IO_BUFFEREDWRITER_WRITE_METHODDEF    \
    {"write", _PyCFunction_CAST(_io_BufferedWriter_write), METH_VALUE|METH_O, _io_BufferedWriter_write__doc__},

static PyValue
_io_BufferedWriter_write_impl(buffered *self, Py_buffer *buffer);

static PyValue
_io_BufferedWriter_write(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer buffer = {NULL, NULL};

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &buffer, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_BufferedWriter_write_impl((buffered *)PyValue_AsKnownObjectBorrow(self), &buffer);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(_io_BufferedRWPair___init____doc__,
"BufferedRWPair(reader, writer, buffer_size=DEFAULT_BUFFER_SIZE, /)\n"
"--\n"
"\n"
"A buffered reader and writer object together.\n"
"\n"
"A buffered reader object and buffered writer object put together to\n"
"form a sequential IO object that can read and write. This is typically\n"
"used with a socket or two-way pipe.\n"
"\n"
"reader and writer are RawIOBase objects that are readable and\n"
"writeable respectively. If the buffer_size is omitted it defaults to\n"
"DEFAULT_BUFFER_SIZE.");

static int
_io_BufferedRWPair___init___impl(rwpair *self, PyValueBorrow reader,
                                 PyValueBorrow writer,
                                 Py_ssize_t buffer_size);

static int
_io_BufferedRWPair___init___value(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    int return_value = -1;
    PyTypeObject *base_tp = clinic_state()->PyBufferedRWPair_Type;
    PyValueBorrow reader;
    PyValueBorrow writer;
    Py_ssize_t buffer_size = DEFAULT_BUFFER_SIZE;

    if ((Py_IS_TYPE(self, base_tp) ||
         Py_TYPE(self)->tp_new == base_tp->tp_new) &&
        !_PyArg_NoKwnames("BufferedRWPair", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("BufferedRWPair", nargs, 2, 3)) {
        goto exit;
    }
    reader = PyValue_Borrow(args[0]);
    writer = PyValue_Borrow(args[1]);
    if (nargs < 3) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[2]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        buffer_size = ival;
    }
skip_optional:
    return_value = _io_BufferedRWPair___init___impl((rwpair *)self, reader, writer, buffer_size);

exit:
    return return_value;
}

static int
_io_BufferedRWPair___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _io_BufferedRWPair___init___value(self, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = _io_BufferedRWPair___init___value(self, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}

PyDoc_STRVAR(_io_BufferedRWPair_read__doc__,
"read($self, size=-1, /)\n"
"--\n"
"\n");

#define _IO_BUFFEREDRWPAIR_READ_METHODDEF    \
    {"read", _PyCFunction_CAST(_io_BufferedRWPair_read), METH_VALUE, _io_BufferedRWPair_read__doc__},

static PyValue
_io_BufferedRWPair_read_impl(rwpair *self, PyValueBorrow size);

static PyValue
_io_BufferedRWPair_read(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow size = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_NoKwnamesValue("BufferedRWPair.read()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("read", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    size = PyValue_Borrow(args[0]);
skip_optional:
    return_value = _io_BufferedRWPair_read_impl((rwpair *)PyValue_AsKnownObjectBorrow(self), size);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BufferedRWPair_peek__doc__,
"peek($self, size=0, /)\n"
"--\n"
"\n");

#define _IO_BUFFEREDRWPAIR_PEEK_METHODDEF    \
    {"peek", _PyCFunction_CAST(_io_BufferedRWPair_peek), METH_VALUE, _io_BufferedRWPair_peek__doc__},

static PyValue
_io_BufferedRWPair_peek_impl(rwpair *self, PyValueBorrow size);

static PyValue
_io_BufferedRWPair_peek(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow size = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_NoKwnamesValue("BufferedRWPair.peek()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("peek", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    size = PyValue_Borrow(args[0]);
skip_optional:
    return_value = _io_BufferedRWPair_peek_impl((rwpair *)PyValue_AsKnownObjectBorrow(self), size);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BufferedRWPair_read1__doc__,
"read1($self, size=-1, /)\n"
"--\n"
"\n");

#define _IO_BUFFEREDRWPAIR_READ1_METHODDEF    \
    {"read1", _PyCFunction_CAST(_io_BufferedRWPair_read1), METH_VALUE, _io_BufferedRWPair_read1__doc__},

static PyValue
_io_BufferedRWPair_read1_impl(rwpair *self, PyValueBorrow size);

static PyValue
_io_BufferedRWPair_read1(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow size = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_NoKwnamesValue("BufferedRWPair.read1()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("read1", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    size = PyValue_Borrow(args[0]);
skip_optional:
    return_value = _io_BufferedRWPair_read1_impl((rwpair *)PyValue_AsKnownObjectBorrow(self), size);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BufferedRWPair_readinto__doc__,
"readinto($self, buffer, /)\n"
"--\n"
"\n");

#define _IO_BUFFEREDRWPAIR_READINTO_METHODDEF    \
    {"readinto", _PyCFunction_CAST(_io_BufferedRWPair_readinto), METH_VALUE|METH_O, _io_BufferedRWPair_readinto__doc__},

static PyValue
_io_BufferedRWPair_readinto_impl(rwpair *self, PyValueBorrow buffer);

static PyValue
_io_BufferedRWPair_readinto(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow buffer;

    buffer = PyValue_Borrow(args[0]);
    return_value = _io_BufferedRWPair_readinto_impl((rwpair *)PyValue_AsKnownObjectBorrow(self), buffer);

    return return_value;
}

PyDoc_STRVAR(_io_BufferedRWPair_readinto1__doc__,
"readinto1($self, buffer, /)\n"
"--\n"
"\n");

#define _IO_BUFFEREDRWPAIR_READINTO1_METHODDEF    \
    {"readinto1", _PyCFunction_CAST(_io_BufferedRWPair_readinto1), METH_VALUE|METH_O, _io_BufferedRWPair_readinto1__doc__},

static PyValue
_io_BufferedRWPair_readinto1_impl(rwpair *self, PyValueBorrow buffer);

static PyValue
_io_BufferedRWPair_readinto1(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow buffer;

    buffer = PyValue_Borrow(args[0]);
    return_value = _io_BufferedRWPair_readinto1_impl((rwpair *)PyValue_AsKnownObjectBorrow(self), buffer);

    return return_value;
}

PyDoc_STRVAR(_io_BufferedRWPair_write__doc__,
"write($self, buffer, /)\n"
"--\n"
"\n");

#define _IO_BUFFEREDRWPAIR_WRITE_METHODDEF    \
    {"write", _PyCFunction_CAST(_io_BufferedRWPair_write), METH_VALUE|METH_O, _io_BufferedRWPair_write__doc__},

static PyValue
_io_BufferedRWPair_write_impl(rwpair *self, PyValueBorrow buffer);

static PyValue
_io_BufferedRWPair_write(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow buffer;

    buffer = PyValue_Borrow(args[0]);
    return_value = _io_BufferedRWPair_write_impl((rwpair *)PyValue_AsKnownObjectBorrow(self), buffer);

    return return_value;
}

PyDoc_STRVAR(_io_BufferedRWPair_flush__doc__,
"flush($self, /)\n"
"--\n"
"\n");

#define _IO_BUFFEREDRWPAIR_FLUSH_METHODDEF    \
    {"flush", _PyCFunction_CAST(_io_BufferedRWPair_flush), METH_VALUE|METH_NOARGS, _io_BufferedRWPair_flush__doc__},

static PyValue
_io_BufferedRWPair_flush_impl(rwpair *self);

static PyValue
_io_BufferedRWPair_flush(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _io_BufferedRWPair_flush_impl((rwpair *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_io_BufferedRWPair_readable__doc__,
"readable($self, /)\n"
"--\n"
"\n");

#define _IO_BUFFEREDRWPAIR_READABLE_METHODDEF    \
    {"readable", _PyCFunction_CAST(_io_BufferedRWPair_readable), METH_VALUE|METH_NOARGS, _io_BufferedRWPair_readable__doc__},

static PyValue
_io_BufferedRWPair_readable_impl(rwpair *self);

static PyValue
_io_BufferedRWPair_readable(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _io_BufferedRWPair_readable_impl((rwpair *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_io_BufferedRWPair_writable__doc__,
"writable($self, /)\n"
"--\n"
"\n");

#define _IO_BUFFEREDRWPAIR_WRITABLE_METHODDEF    \
    {"writable", _PyCFunction_CAST(_io_BufferedRWPair_writable), METH_VALUE|METH_NOARGS, _io_BufferedRWPair_writable__doc__},

static PyValue
_io_BufferedRWPair_writable_impl(rwpair *self);

static PyValue
_io_BufferedRWPair_writable(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _io_BufferedRWPair_writable_impl((rwpair *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_io_BufferedRWPair_close__doc__,
"close($self, /)\n"
"--\n"
"\n");

#define _IO_BUFFEREDRWPAIR_CLOSE_METHODDEF    \
    {"close", _PyCFunction_CAST(_io_BufferedRWPair_close), METH_VALUE|METH_NOARGS, _io_BufferedRWPair_close__doc__},

static PyValue
_io_BufferedRWPair_close_impl(rwpair *self);

static PyValue
_io_BufferedRWPair_close(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _io_BufferedRWPair_close_impl((rwpair *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_io_BufferedRWPair_isatty__doc__,
"isatty($self, /)\n"
"--\n"
"\n");

#define _IO_BUFFEREDRWPAIR_ISATTY_METHODDEF    \
    {"isatty", _PyCFunction_CAST(_io_BufferedRWPair_isatty), METH_VALUE|METH_NOARGS, _io_BufferedRWPair_isatty__doc__},

static PyValue
_io_BufferedRWPair_isatty_impl(rwpair *self);

static PyValue
_io_BufferedRWPair_isatty(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _io_BufferedRWPair_isatty_impl((rwpair *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

static PyObject *
_io_BufferedRWPair_closed_get_impl(rwpair *self);

static PyObject *
_io_BufferedRWPair_closed_get(PyObject *self, void *Py_UNUSED(context))
{
    return _io_BufferedRWPair_closed_get_impl((rwpair *)self);
}

PyDoc_STRVAR(_io_BufferedRandom___init____doc__,
"BufferedRandom(raw, buffer_size=DEFAULT_BUFFER_SIZE)\n"
"--\n"
"\n"
"A buffered interface to random access streams.\n"
"\n"
"The constructor creates a reader and writer for a seekable stream,\n"
"raw, given in the first argument. If the buffer_size is omitted it\n"
"defaults to DEFAULT_BUFFER_SIZE.");

static int
_io_BufferedRandom___init___impl(buffered *self, PyValueBorrow raw,
                                 Py_ssize_t buffer_size);

static int
_io_BufferedRandom___init___value(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(raw)), _PyValue_STATIC_IMMORTAL(&_Py_ID(buffer_size)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"raw", "buffer_size", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "BufferedRandom",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow raw;
    Py_ssize_t buffer_size = DEFAULT_BUFFER_SIZE;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    raw = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        {
            Py_ssize_t ival = -1;
            PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[1]]));
            if (!PyValue_IsNull(iobj)) {
                ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
                PyValue_CLOSE(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            buffer_size = ival;
        }
    }
    return_value = _io_BufferedRandom___init___impl((buffered *)self, raw, buffer_size);

exit:
    return return_value;
}

static int
_io_BufferedRandom___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _io_BufferedRandom___init___value(self, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = _io_BufferedRandom___init___value(self, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}
#define _IO__BUFFERED_CLOSED_GETSETDEF {"closed", (getter)_io__Buffered_closed_get, (setter)NULL, NULL},

#define _IO__BUFFERED_NAME_GETSETDEF {"name", (getter)_io__Buffered_name_get, (setter)NULL, NULL},

#define _IO__BUFFERED_MODE_GETSETDEF {"mode", (getter)_io__Buffered_mode_get, (setter)NULL, NULL},

#define _IO_BUFFEREDRWPAIR_CLOSED_GETSETDEF {"closed", (getter)_io_BufferedRWPair_closed_get, (setter)NULL, NULL},

/*[clinic end generated code: output=9441d9756ffbf820 input=a9049054013a1b77]*/
