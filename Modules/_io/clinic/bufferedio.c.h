/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_abstract.h"      // _Py_convert_optional_to_ssize_t()
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_BadArgument()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()

PyDoc_STRVAR(_io__BufferedIOBase_readinto__doc__,
"readinto($self, buffer, /)\n"
"--\n"
"\n");

#define _IO__BUFFEREDIOBASE_READINTO_METHODDEF    \
    {"readinto", _PyCFunction_CAST(_io__BufferedIOBase_readinto), METH_O, _io__BufferedIOBase_readinto__doc__},

static PyObject *
_io__BufferedIOBase_readinto_impl(PyObject *self, Py_buffer *buffer);

static PyValue
_io__BufferedIOBase_readinto(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer buffer = {NULL, NULL};
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyObject_GetBuffer(_arg, &buffer, PyBUF_WRITABLE) < 0) {
            _PyArg_BadArgument("readinto", "argument", "read-write bytes-like object", _arg);
            goto exit;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _io__BufferedIOBase_readinto_impl(self, &buffer);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
    {"readinto1", _PyCFunction_CAST(_io__BufferedIOBase_readinto1), METH_O, _io__BufferedIOBase_readinto1__doc__},

static PyObject *
_io__BufferedIOBase_readinto1_impl(PyObject *self, Py_buffer *buffer);

static PyValue
_io__BufferedIOBase_readinto1(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer buffer = {NULL, NULL};
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyObject_GetBuffer(_arg, &buffer, PyBUF_WRITABLE) < 0) {
            _PyArg_BadArgument("readinto1", "argument", "read-write bytes-like object", _arg);
            goto exit;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _io__BufferedIOBase_readinto1_impl(self, &buffer);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
    {"detach", _PyCFunction_CAST(_io__BufferedIOBase_detach), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _io__BufferedIOBase_detach__doc__},

static PyObject *
_io__BufferedIOBase_detach_impl(PyObject *self, PyTypeObject *cls);

static PyValue
_io__BufferedIOBase_detach(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "detach() takes no arguments");
        return PyValue_NULL;
    }
    return PyValue_FromResult(_io__BufferedIOBase_detach_impl(self, cls));
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
    {"read", _PyCFunction_CAST(_io__BufferedIOBase_read), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _io__BufferedIOBase_read__doc__},

static PyObject *
_io__BufferedIOBase_read_impl(PyObject *self, PyTypeObject *cls,
                              int Py_UNUSED(size));

static PyValue
_io__BufferedIOBase_read(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int size = -1;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("read", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("read", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    size = PyLong_AsIntValue(args[0]);
    if (size == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    _return_value = _io__BufferedIOBase_read_impl(self, cls, size);
    return_value = PyValue_FromResult(_return_value);

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
    {"read1", _PyCFunction_CAST(_io__BufferedIOBase_read1), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _io__BufferedIOBase_read1__doc__},

static PyObject *
_io__BufferedIOBase_read1_impl(PyObject *self, PyTypeObject *cls,
                               int Py_UNUSED(size));

static PyValue
_io__BufferedIOBase_read1(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int size = -1;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("read1", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("read1", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    size = PyLong_AsIntValue(args[0]);
    if (size == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    _return_value = _io__BufferedIOBase_read1_impl(self, cls, size);
    return_value = PyValue_FromResult(_return_value);

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
    {"write", _PyCFunction_CAST(_io__BufferedIOBase_write), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _io__BufferedIOBase_write__doc__},

static PyObject *
_io__BufferedIOBase_write_impl(PyObject *self, PyTypeObject *cls,
                               PyObject *Py_UNUSED(b));

static PyValue
_io__BufferedIOBase_write(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *b;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("write", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("write", nargs, 1, 1)) {
        goto exit;
    }
    b = PyValue_Object(args[0]);
    if (b == NULL) {
        goto exit;
    }
    _return_value = _io__BufferedIOBase_write_impl(self, cls, b);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__Buffered___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED___SIZEOF___METHODDEF    \
    {"__sizeof__", (PyCFunction)_io__Buffered___sizeof__, METH_NOARGS, _io__Buffered___sizeof____doc__},

static PyObject *
_io__Buffered___sizeof___impl(buffered *self);

static PyObject *
_io__Buffered___sizeof__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered___sizeof___impl((buffered *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io__Buffered__dealloc_warn__doc__,
"_dealloc_warn($self, source, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED__DEALLOC_WARN_METHODDEF    \
    {"_dealloc_warn", _PyCFunction_CAST(_io__Buffered__dealloc_warn), METH_O, _io__Buffered__dealloc_warn__doc__},

static PyObject *
_io__Buffered__dealloc_warn_impl(buffered *self, PyObject *source);

static PyValue
_io__Buffered__dealloc_warn(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *source;
    PyObject *_return_value;

    source = PyValue_Object(arg);
    if (source == NULL) {
        goto exit;
    }
    _return_value = _io__Buffered__dealloc_warn_impl((buffered *)self, source);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__Buffered_simple_flush__doc__,
"flush($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_SIMPLE_FLUSH_METHODDEF    \
    {"flush", (PyCFunction)_io__Buffered_simple_flush, METH_NOARGS, _io__Buffered_simple_flush__doc__},

static PyObject *
_io__Buffered_simple_flush_impl(buffered *self);

static PyObject *
_io__Buffered_simple_flush(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_simple_flush_impl((buffered *)self);
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
    {"close", (PyCFunction)_io__Buffered_close, METH_NOARGS, _io__Buffered_close__doc__},

static PyObject *
_io__Buffered_close_impl(buffered *self);

static PyObject *
_io__Buffered_close(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_close_impl((buffered *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_detach__doc__,
"detach($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_DETACH_METHODDEF    \
    {"detach", (PyCFunction)_io__Buffered_detach, METH_NOARGS, _io__Buffered_detach__doc__},

static PyObject *
_io__Buffered_detach_impl(buffered *self);

static PyObject *
_io__Buffered_detach(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_detach_impl((buffered *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_seekable__doc__,
"seekable($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_SEEKABLE_METHODDEF    \
    {"seekable", (PyCFunction)_io__Buffered_seekable, METH_NOARGS, _io__Buffered_seekable__doc__},

static PyObject *
_io__Buffered_seekable_impl(buffered *self);

static PyObject *
_io__Buffered_seekable(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_seekable_impl((buffered *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_readable__doc__,
"readable($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_READABLE_METHODDEF    \
    {"readable", (PyCFunction)_io__Buffered_readable, METH_NOARGS, _io__Buffered_readable__doc__},

static PyObject *
_io__Buffered_readable_impl(buffered *self);

static PyObject *
_io__Buffered_readable(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_readable_impl((buffered *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_writable__doc__,
"writable($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_WRITABLE_METHODDEF    \
    {"writable", (PyCFunction)_io__Buffered_writable, METH_NOARGS, _io__Buffered_writable__doc__},

static PyObject *
_io__Buffered_writable_impl(buffered *self);

static PyObject *
_io__Buffered_writable(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_writable_impl((buffered *)self);
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
    {"fileno", (PyCFunction)_io__Buffered_fileno, METH_NOARGS, _io__Buffered_fileno__doc__},

static PyObject *
_io__Buffered_fileno_impl(buffered *self);

static PyObject *
_io__Buffered_fileno(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_fileno_impl((buffered *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_isatty__doc__,
"isatty($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_ISATTY_METHODDEF    \
    {"isatty", (PyCFunction)_io__Buffered_isatty, METH_NOARGS, _io__Buffered_isatty__doc__},

static PyObject *
_io__Buffered_isatty_impl(buffered *self);

static PyObject *
_io__Buffered_isatty(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_isatty_impl((buffered *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_flush__doc__,
"flush($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_FLUSH_METHODDEF    \
    {"flush", (PyCFunction)_io__Buffered_flush, METH_NOARGS, _io__Buffered_flush__doc__},

static PyObject *
_io__Buffered_flush_impl(buffered *self);

static PyObject *
_io__Buffered_flush(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_flush_impl((buffered *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_peek__doc__,
"peek($self, size=0, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_PEEK_METHODDEF    \
    {"peek", _PyCFunction_CAST(_io__Buffered_peek), METH_FASTCALL, _io__Buffered_peek__doc__},

static PyObject *
_io__Buffered_peek_impl(buffered *self, Py_ssize_t size);

static PyValue
_io__Buffered_peek(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t size = 0;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("peek", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(args[0]);
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        size = ival;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _io__Buffered_peek_impl((buffered *)self, size);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__Buffered_read__doc__,
"read($self, size=-1, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_READ_METHODDEF    \
    {"read", _PyCFunction_CAST(_io__Buffered_read), METH_FASTCALL, _io__Buffered_read__doc__},

static PyObject *
_io__Buffered_read_impl(buffered *self, Py_ssize_t n);

static PyValue
_io__Buffered_read(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t n = -1;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("read", nargs, 0, 1)) {
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
        if (!_Py_convert_optional_to_ssize_t(_arg, &n)) {
            goto exit;
        }
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _io__Buffered_read_impl((buffered *)self, n);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__Buffered_read1__doc__,
"read1($self, size=-1, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_READ1_METHODDEF    \
    {"read1", _PyCFunction_CAST(_io__Buffered_read1), METH_FASTCALL, _io__Buffered_read1__doc__},

static PyObject *
_io__Buffered_read1_impl(buffered *self, Py_ssize_t n);

static PyValue
_io__Buffered_read1(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t n = -1;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("read1", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(args[0]);
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        n = ival;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _io__Buffered_read1_impl((buffered *)self, n);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__Buffered_readinto__doc__,
"readinto($self, buffer, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_READINTO_METHODDEF    \
    {"readinto", _PyCFunction_CAST(_io__Buffered_readinto), METH_O, _io__Buffered_readinto__doc__},

static PyObject *
_io__Buffered_readinto_impl(buffered *self, Py_buffer *buffer);

static PyValue
_io__Buffered_readinto(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer buffer = {NULL, NULL};
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyObject_GetBuffer(_arg, &buffer, PyBUF_WRITABLE) < 0) {
            _PyArg_BadArgument("readinto", "argument", "read-write bytes-like object", _arg);
            goto exit;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _io__Buffered_readinto_impl((buffered *)self, &buffer);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
    {"readinto1", _PyCFunction_CAST(_io__Buffered_readinto1), METH_O, _io__Buffered_readinto1__doc__},

static PyObject *
_io__Buffered_readinto1_impl(buffered *self, Py_buffer *buffer);

static PyValue
_io__Buffered_readinto1(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer buffer = {NULL, NULL};
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyObject_GetBuffer(_arg, &buffer, PyBUF_WRITABLE) < 0) {
            _PyArg_BadArgument("readinto1", "argument", "read-write bytes-like object", _arg);
            goto exit;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _io__Buffered_readinto1_impl((buffered *)self, &buffer);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
    {"readline", _PyCFunction_CAST(_io__Buffered_readline), METH_FASTCALL, _io__Buffered_readline__doc__},

static PyObject *
_io__Buffered_readline_impl(buffered *self, Py_ssize_t size);

static PyValue
_io__Buffered_readline(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t size = -1;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("readline", nargs, 0, 1)) {
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
        if (!_Py_convert_optional_to_ssize_t(_arg, &size)) {
            goto exit;
        }
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _io__Buffered_readline_impl((buffered *)self, size);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__Buffered_tell__doc__,
"tell($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_TELL_METHODDEF    \
    {"tell", (PyCFunction)_io__Buffered_tell, METH_NOARGS, _io__Buffered_tell__doc__},

static PyObject *
_io__Buffered_tell_impl(buffered *self);

static PyObject *
_io__Buffered_tell(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_tell_impl((buffered *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_seek__doc__,
"seek($self, target, whence=0, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_SEEK_METHODDEF    \
    {"seek", _PyCFunction_CAST(_io__Buffered_seek), METH_FASTCALL, _io__Buffered_seek__doc__},

static PyObject *
_io__Buffered_seek_impl(buffered *self, PyObject *targetobj, int whence);

static PyValue
_io__Buffered_seek(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *targetobj;
    int whence = 0;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("seek", nargs, 1, 2)) {
        goto exit;
    }
    targetobj = PyValue_Object(args[0]);
    if (targetobj == NULL) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    whence = PyLong_AsIntValue(args[1]);
    if (whence == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _io__Buffered_seek_impl((buffered *)self, targetobj, whence);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__Buffered_truncate__doc__,
"truncate($self, pos=None, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_TRUNCATE_METHODDEF    \
    {"truncate", _PyCFunction_CAST(_io__Buffered_truncate), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _io__Buffered_truncate__doc__},

static PyObject *
_io__Buffered_truncate_impl(buffered *self, PyTypeObject *cls, PyObject *pos);

static PyValue
_io__Buffered_truncate(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *pos = Py_None;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("truncate", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("truncate", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    pos = PyValue_Object(args[0]);
    if (pos == NULL) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _io__Buffered_truncate_impl((buffered *)self, cls, pos);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BufferedReader___init____doc__,
"BufferedReader(raw, buffer_size=DEFAULT_BUFFER_SIZE)\n"
"--\n"
"\n"
"Create a new buffered reader using the given readable raw IO object.");

static int
_io_BufferedReader___init___impl(buffered *self, PyObject *raw,
                                 Py_ssize_t buffer_size);

static int
_io_BufferedReader___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(raw), &_Py_ID(buffer_size), },
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
    PyValue argsbuf[2];
    const PyValue *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    PyObject *raw;
    Py_ssize_t buffer_size = DEFAULT_BUFFER_SIZE;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_ITEMS(args), nargs, kwargs, NULL, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    raw = PyValue_Object(fastargs[0]);
    if (raw == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(fastargs[1]);
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        buffer_size = ival;
    }
skip_optional_pos:
    return_value = _io_BufferedReader___init___impl((buffered *)self, raw, buffer_size);

exit:
    return return_value;
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
_io_BufferedWriter___init___impl(buffered *self, PyObject *raw,
                                 Py_ssize_t buffer_size);

static int
_io_BufferedWriter___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(raw), &_Py_ID(buffer_size), },
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
    PyValue argsbuf[2];
    const PyValue *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    PyObject *raw;
    Py_ssize_t buffer_size = DEFAULT_BUFFER_SIZE;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_ITEMS(args), nargs, kwargs, NULL, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    raw = PyValue_Object(fastargs[0]);
    if (raw == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(fastargs[1]);
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        buffer_size = ival;
    }
skip_optional_pos:
    return_value = _io_BufferedWriter___init___impl((buffered *)self, raw, buffer_size);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BufferedWriter_write__doc__,
"write($self, buffer, /)\n"
"--\n"
"\n");

#define _IO_BUFFEREDWRITER_WRITE_METHODDEF    \
    {"write", _PyCFunction_CAST(_io_BufferedWriter_write), METH_O, _io_BufferedWriter_write__doc__},

static PyObject *
_io_BufferedWriter_write_impl(buffered *self, Py_buffer *buffer);

static PyValue
_io_BufferedWriter_write(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer buffer = {NULL, NULL};
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyObject_GetBuffer(_arg, &buffer, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _io_BufferedWriter_write_impl((buffered *)self, &buffer);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
_io_BufferedRWPair___init___impl(rwpair *self, PyObject *reader,
                                 PyObject *writer, Py_ssize_t buffer_size);

static int
_io_BufferedRWPair___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    PyTypeObject *base_tp = clinic_state()->PyBufferedRWPair_Type;
    PyObject *reader;
    PyObject *writer;
    Py_ssize_t buffer_size = DEFAULT_BUFFER_SIZE;

    if ((Py_IS_TYPE(self, base_tp) ||
         Py_TYPE(self)->tp_new == base_tp->tp_new) &&
        !_PyArg_NoKeywords("BufferedRWPair", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("BufferedRWPair", PyTuple_GET_SIZE(args), 2, 3)) {
        goto exit;
    }
    reader = PyValue_Object(PyTuple_GET_VALUE(args, 0));
    if (reader == NULL) {
        goto exit;
    }
    writer = PyValue_Object(PyTuple_GET_VALUE(args, 1));
    if (writer == NULL) {
        goto exit;
    }
    if (PyTuple_GET_SIZE(args) < 3) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(PyTuple_GET_VALUE(args, 2));
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
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

PyDoc_STRVAR(_io_BufferedRWPair_read__doc__,
"read($self, size=-1, /)\n"
"--\n"
"\n");

#define _IO_BUFFEREDRWPAIR_READ_METHODDEF    \
    {"read", _PyCFunction_CAST(_io_BufferedRWPair_read), METH_FASTCALL, _io_BufferedRWPair_read__doc__},

static PyObject *
_io_BufferedRWPair_read_impl(rwpair *self, PyObject *size);

static PyValue
_io_BufferedRWPair_read(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *size = NULL;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("read", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    size = PyValue_Object(args[0]);
    if (size == NULL) {
        goto exit;
    }
skip_optional:
    _return_value = _io_BufferedRWPair_read_impl((rwpair *)self, size);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BufferedRWPair_peek__doc__,
"peek($self, size=0, /)\n"
"--\n"
"\n");

#define _IO_BUFFEREDRWPAIR_PEEK_METHODDEF    \
    {"peek", _PyCFunction_CAST(_io_BufferedRWPair_peek), METH_FASTCALL, _io_BufferedRWPair_peek__doc__},

static PyObject *
_io_BufferedRWPair_peek_impl(rwpair *self, PyObject *size);

static PyValue
_io_BufferedRWPair_peek(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *size = NULL;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("peek", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    size = PyValue_Object(args[0]);
    if (size == NULL) {
        goto exit;
    }
skip_optional:
    _return_value = _io_BufferedRWPair_peek_impl((rwpair *)self, size);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BufferedRWPair_read1__doc__,
"read1($self, size=-1, /)\n"
"--\n"
"\n");

#define _IO_BUFFEREDRWPAIR_READ1_METHODDEF    \
    {"read1", _PyCFunction_CAST(_io_BufferedRWPair_read1), METH_FASTCALL, _io_BufferedRWPair_read1__doc__},

static PyObject *
_io_BufferedRWPair_read1_impl(rwpair *self, PyObject *size);

static PyValue
_io_BufferedRWPair_read1(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *size = NULL;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("read1", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    size = PyValue_Object(args[0]);
    if (size == NULL) {
        goto exit;
    }
skip_optional:
    _return_value = _io_BufferedRWPair_read1_impl((rwpair *)self, size);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BufferedRWPair_readinto__doc__,
"readinto($self, buffer, /)\n"
"--\n"
"\n");

#define _IO_BUFFEREDRWPAIR_READINTO_METHODDEF    \
    {"readinto", _PyCFunction_CAST(_io_BufferedRWPair_readinto), METH_O, _io_BufferedRWPair_readinto__doc__},

static PyObject *
_io_BufferedRWPair_readinto_impl(rwpair *self, PyObject *buffer);

static PyValue
_io_BufferedRWPair_readinto(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *buffer;
    PyObject *_return_value;

    buffer = PyValue_Object(arg);
    if (buffer == NULL) {
        goto exit;
    }
    _return_value = _io_BufferedRWPair_readinto_impl((rwpair *)self, buffer);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BufferedRWPair_readinto1__doc__,
"readinto1($self, buffer, /)\n"
"--\n"
"\n");

#define _IO_BUFFEREDRWPAIR_READINTO1_METHODDEF    \
    {"readinto1", _PyCFunction_CAST(_io_BufferedRWPair_readinto1), METH_O, _io_BufferedRWPair_readinto1__doc__},

static PyObject *
_io_BufferedRWPair_readinto1_impl(rwpair *self, PyObject *buffer);

static PyValue
_io_BufferedRWPair_readinto1(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *buffer;
    PyObject *_return_value;

    buffer = PyValue_Object(arg);
    if (buffer == NULL) {
        goto exit;
    }
    _return_value = _io_BufferedRWPair_readinto1_impl((rwpair *)self, buffer);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BufferedRWPair_write__doc__,
"write($self, buffer, /)\n"
"--\n"
"\n");

#define _IO_BUFFEREDRWPAIR_WRITE_METHODDEF    \
    {"write", _PyCFunction_CAST(_io_BufferedRWPair_write), METH_O, _io_BufferedRWPair_write__doc__},

static PyObject *
_io_BufferedRWPair_write_impl(rwpair *self, PyObject *buffer);

static PyValue
_io_BufferedRWPair_write(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *buffer;
    PyObject *_return_value;

    buffer = PyValue_Object(arg);
    if (buffer == NULL) {
        goto exit;
    }
    _return_value = _io_BufferedRWPair_write_impl((rwpair *)self, buffer);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BufferedRWPair_flush__doc__,
"flush($self, /)\n"
"--\n"
"\n");

#define _IO_BUFFEREDRWPAIR_FLUSH_METHODDEF    \
    {"flush", (PyCFunction)_io_BufferedRWPair_flush, METH_NOARGS, _io_BufferedRWPair_flush__doc__},

static PyObject *
_io_BufferedRWPair_flush_impl(rwpair *self);

static PyObject *
_io_BufferedRWPair_flush(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _io_BufferedRWPair_flush_impl((rwpair *)self);
}

PyDoc_STRVAR(_io_BufferedRWPair_readable__doc__,
"readable($self, /)\n"
"--\n"
"\n");

#define _IO_BUFFEREDRWPAIR_READABLE_METHODDEF    \
    {"readable", (PyCFunction)_io_BufferedRWPair_readable, METH_NOARGS, _io_BufferedRWPair_readable__doc__},

static PyObject *
_io_BufferedRWPair_readable_impl(rwpair *self);

static PyObject *
_io_BufferedRWPair_readable(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _io_BufferedRWPair_readable_impl((rwpair *)self);
}

PyDoc_STRVAR(_io_BufferedRWPair_writable__doc__,
"writable($self, /)\n"
"--\n"
"\n");

#define _IO_BUFFEREDRWPAIR_WRITABLE_METHODDEF    \
    {"writable", (PyCFunction)_io_BufferedRWPair_writable, METH_NOARGS, _io_BufferedRWPair_writable__doc__},

static PyObject *
_io_BufferedRWPair_writable_impl(rwpair *self);

static PyObject *
_io_BufferedRWPair_writable(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _io_BufferedRWPair_writable_impl((rwpair *)self);
}

PyDoc_STRVAR(_io_BufferedRWPair_close__doc__,
"close($self, /)\n"
"--\n"
"\n");

#define _IO_BUFFEREDRWPAIR_CLOSE_METHODDEF    \
    {"close", (PyCFunction)_io_BufferedRWPair_close, METH_NOARGS, _io_BufferedRWPair_close__doc__},

static PyObject *
_io_BufferedRWPair_close_impl(rwpair *self);

static PyObject *
_io_BufferedRWPair_close(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _io_BufferedRWPair_close_impl((rwpair *)self);
}

PyDoc_STRVAR(_io_BufferedRWPair_isatty__doc__,
"isatty($self, /)\n"
"--\n"
"\n");

#define _IO_BUFFEREDRWPAIR_ISATTY_METHODDEF    \
    {"isatty", (PyCFunction)_io_BufferedRWPair_isatty, METH_NOARGS, _io_BufferedRWPair_isatty__doc__},

static PyObject *
_io_BufferedRWPair_isatty_impl(rwpair *self);

static PyObject *
_io_BufferedRWPair_isatty(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _io_BufferedRWPair_isatty_impl((rwpair *)self);
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
_io_BufferedRandom___init___impl(buffered *self, PyObject *raw,
                                 Py_ssize_t buffer_size);

static int
_io_BufferedRandom___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(raw), &_Py_ID(buffer_size), },
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
    PyValue argsbuf[2];
    const PyValue *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    PyObject *raw;
    Py_ssize_t buffer_size = DEFAULT_BUFFER_SIZE;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_ITEMS(args), nargs, kwargs, NULL, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    raw = PyValue_Object(fastargs[0]);
    if (raw == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(fastargs[1]);
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        buffer_size = ival;
    }
skip_optional_pos:
    return_value = _io_BufferedRandom___init___impl((buffered *)self, raw, buffer_size);

exit:
    return return_value;
}
#define _IO__BUFFERED_CLOSED_GETSETDEF {"closed", (getter)_io__Buffered_closed_get, (setter)NULL, NULL},

#define _IO__BUFFERED_NAME_GETSETDEF {"name", (getter)_io__Buffered_name_get, (setter)NULL, NULL},

#define _IO__BUFFERED_MODE_GETSETDEF {"mode", (getter)_io__Buffered_mode_get, (setter)NULL, NULL},

#define _IO_BUFFEREDRWPAIR_CLOSED_GETSETDEF {"closed", (getter)_io_BufferedRWPair_closed_get, (setter)NULL, NULL},

/*[clinic end generated code: output=2286847697b605b7 input=a9049054013a1b77]*/
