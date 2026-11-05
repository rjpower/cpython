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
#include "pycore_long_value.h"    // _PyValue_IntAsSsize_t()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_Index()

PyDoc_STRVAR(_io_BytesIO_closed__doc__,
"True if the file is closed.");

static int
_io_BytesIO_closed_get_impl(bytesio *self);

static PyObject *
_io_BytesIO_closed_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;
    int _return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _io_BytesIO_closed_get_impl((bytesio *)self);
    Py_END_CRITICAL_SECTION();
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_readable__doc__,
"readable($self, /)\n"
"--\n"
"\n"
"Returns True if the IO object can be read.");

#define _IO_BYTESIO_READABLE_METHODDEF    \
    {"readable", _PyCFunction_CAST(_io_BytesIO_readable), METH_VALUE|METH_NOARGS, _io_BytesIO_readable__doc__},

static PyValue
_io_BytesIO_readable_impl(bytesio *self);

static PyValue
_io_BytesIO_readable(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_BytesIO_readable_impl((bytesio *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_writable__doc__,
"writable($self, /)\n"
"--\n"
"\n"
"Returns True if the IO object can be written.");

#define _IO_BYTESIO_WRITABLE_METHODDEF    \
    {"writable", _PyCFunction_CAST(_io_BytesIO_writable), METH_VALUE|METH_NOARGS, _io_BytesIO_writable__doc__},

static PyValue
_io_BytesIO_writable_impl(bytesio *self);

static PyValue
_io_BytesIO_writable(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_BytesIO_writable_impl((bytesio *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_seekable__doc__,
"seekable($self, /)\n"
"--\n"
"\n"
"Returns True if the IO object can be seeked.");

#define _IO_BYTESIO_SEEKABLE_METHODDEF    \
    {"seekable", _PyCFunction_CAST(_io_BytesIO_seekable), METH_VALUE|METH_NOARGS, _io_BytesIO_seekable__doc__},

static PyValue
_io_BytesIO_seekable_impl(bytesio *self);

static PyValue
_io_BytesIO_seekable(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_BytesIO_seekable_impl((bytesio *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_flush__doc__,
"flush($self, /)\n"
"--\n"
"\n"
"Does nothing.");

#define _IO_BYTESIO_FLUSH_METHODDEF    \
    {"flush", _PyCFunction_CAST(_io_BytesIO_flush), METH_VALUE|METH_NOARGS, _io_BytesIO_flush__doc__},

static PyValue
_io_BytesIO_flush_impl(bytesio *self);

static PyValue
_io_BytesIO_flush(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_BytesIO_flush_impl((bytesio *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_getbuffer__doc__,
"getbuffer($self, /)\n"
"--\n"
"\n"
"Get a read-write view over the contents of the BytesIO object.");

#define _IO_BYTESIO_GETBUFFER_METHODDEF    \
    {"getbuffer", _PyCFunction_CAST(_io_BytesIO_getbuffer), METH_VALUE|METH_METHOD|METH_KEYWORDS, _io_BytesIO_getbuffer__doc__},

static PyValue
_io_BytesIO_getbuffer_impl(bytesio *self, PyTypeObject *cls);

static PyValue
_io_BytesIO_getbuffer(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "getbuffer() takes no arguments");
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_BytesIO_getbuffer_impl((bytesio *)PyValue_AsKnownObjectBorrow(self), cls);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_getvalue__doc__,
"getvalue($self, /)\n"
"--\n"
"\n"
"Retrieve the entire contents of the BytesIO object.");

#define _IO_BYTESIO_GETVALUE_METHODDEF    \
    {"getvalue", _PyCFunction_CAST(_io_BytesIO_getvalue), METH_VALUE|METH_NOARGS, _io_BytesIO_getvalue__doc__},

static PyValue
_io_BytesIO_getvalue_impl(bytesio *self);

static PyValue
_io_BytesIO_getvalue(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_BytesIO_getvalue_impl((bytesio *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_isatty__doc__,
"isatty($self, /)\n"
"--\n"
"\n"
"Always returns False.\n"
"\n"
"BytesIO objects are not connected to a TTY-like device.");

#define _IO_BYTESIO_ISATTY_METHODDEF    \
    {"isatty", _PyCFunction_CAST(_io_BytesIO_isatty), METH_VALUE|METH_NOARGS, _io_BytesIO_isatty__doc__},

static PyValue
_io_BytesIO_isatty_impl(bytesio *self);

static PyValue
_io_BytesIO_isatty(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_BytesIO_isatty_impl((bytesio *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_tell__doc__,
"tell($self, /)\n"
"--\n"
"\n"
"Current file position, an integer.");

#define _IO_BYTESIO_TELL_METHODDEF    \
    {"tell", _PyCFunction_CAST(_io_BytesIO_tell), METH_VALUE|METH_NOARGS, _io_BytesIO_tell__doc__},

static PyValue
_io_BytesIO_tell_impl(bytesio *self);

static PyValue
_io_BytesIO_tell(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_BytesIO_tell_impl((bytesio *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_read__doc__,
"read($self, size=-1, /)\n"
"--\n"
"\n"
"Read at most size bytes, returned as a bytes object.\n"
"\n"
"If the size argument is negative, read until EOF is reached.\n"
"Return an empty bytes object at EOF.");

#define _IO_BYTESIO_READ_METHODDEF    \
    {"read", _PyCFunction_CAST(_io_BytesIO_read), METH_VALUE, _io_BytesIO_read__doc__},

static PyValue
_io_BytesIO_read_impl(bytesio *self, Py_ssize_t size);

static PyValue
_io_BytesIO_read(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t size = -1;

    if (!_PyArg_CheckPositional("read", nargs, 0, 1)) {
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
    return_value = _io_BytesIO_read_impl((bytesio *)PyValue_AsKnownObjectBorrow(self), size);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_read1__doc__,
"read1($self, size=-1, /)\n"
"--\n"
"\n"
"Read at most size bytes, returned as a bytes object.\n"
"\n"
"If the size argument is negative or omitted, read until EOF is\n"
"reached.  Return an empty bytes object at EOF.");

#define _IO_BYTESIO_READ1_METHODDEF    \
    {"read1", _PyCFunction_CAST(_io_BytesIO_read1), METH_VALUE, _io_BytesIO_read1__doc__},

static PyValue
_io_BytesIO_read1_impl(bytesio *self, Py_ssize_t size);

static PyValue
_io_BytesIO_read1(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t size = -1;

    if (!_PyArg_CheckPositional("read1", nargs, 0, 1)) {
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
    return_value = _io_BytesIO_read1_impl((bytesio *)PyValue_AsKnownObjectBorrow(self), size);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_peek__doc__,
"peek($self, size=0, /)\n"
"--\n"
"\n"
"Return bytes from the stream without advancing the position.\n"
"\n"
"Return an empty bytes object at EOF.");

#define _IO_BYTESIO_PEEK_METHODDEF    \
    {"peek", _PyCFunction_CAST(_io_BytesIO_peek), METH_VALUE, _io_BytesIO_peek__doc__},

static PyValue
_io_BytesIO_peek_impl(bytesio *self, Py_ssize_t size);

static PyValue
_io_BytesIO_peek(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t size = 0;

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
    return_value = _io_BytesIO_peek_impl((bytesio *)PyValue_AsKnownObjectBorrow(self), size);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_readline__doc__,
"readline($self, size=-1, /)\n"
"--\n"
"\n"
"Next line from the file, as a bytes object.\n"
"\n"
"Retain newline.  A non-negative size argument limits the maximum\n"
"number of bytes to return (an incomplete line may be returned then).\n"
"Return an empty bytes object at EOF.");

#define _IO_BYTESIO_READLINE_METHODDEF    \
    {"readline", _PyCFunction_CAST(_io_BytesIO_readline), METH_VALUE, _io_BytesIO_readline__doc__},

static PyValue
_io_BytesIO_readline_impl(bytesio *self, Py_ssize_t size);

static PyValue
_io_BytesIO_readline(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t size = -1;

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
    return_value = _io_BytesIO_readline_impl((bytesio *)PyValue_AsKnownObjectBorrow(self), size);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_readlines__doc__,
"readlines($self, size=None, /)\n"
"--\n"
"\n"
"List of bytes objects, each a line from the file.\n"
"\n"
"Call readline() repeatedly and return a list of the lines so read.\n"
"The optional size argument, if given, is an approximate bound on the\n"
"total number of bytes in the lines returned.");

#define _IO_BYTESIO_READLINES_METHODDEF    \
    {"readlines", _PyCFunction_CAST(_io_BytesIO_readlines), METH_VALUE, _io_BytesIO_readlines__doc__},

static PyValue
_io_BytesIO_readlines_impl(bytesio *self, PyValueBorrow arg);

static PyValue
_io_BytesIO_readlines(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow arg = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("readlines", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    arg = PyValue_Borrow(args[0]);
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_BytesIO_readlines_impl((bytesio *)PyValue_AsKnownObjectBorrow(self), arg);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_readinto__doc__,
"readinto($self, buffer, /)\n"
"--\n"
"\n"
"Read bytes into buffer.\n"
"\n"
"Returns number of bytes read (0 for EOF), or None if the object\n"
"is set not to block and has no data to read.");

#define _IO_BYTESIO_READINTO_METHODDEF    \
    {"readinto", _PyCFunction_CAST(_io_BytesIO_readinto), METH_VALUE|METH_O, _io_BytesIO_readinto__doc__},

static PyValue
_io_BytesIO_readinto_impl(bytesio *self, Py_buffer *buffer);

static PyValue
_io_BytesIO_readinto(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
    return_value = _io_BytesIO_readinto_impl((bytesio *)PyValue_AsKnownObjectBorrow(self), &buffer);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_truncate__doc__,
"truncate($self, size=None, /)\n"
"--\n"
"\n"
"Truncate the file to at most size bytes.\n"
"\n"
"Size defaults to the current file position, as returned by tell().\n"
"The current file position is unchanged.  Returns the new size.");

#define _IO_BYTESIO_TRUNCATE_METHODDEF    \
    {"truncate", _PyCFunction_CAST(_io_BytesIO_truncate), METH_VALUE, _io_BytesIO_truncate__doc__},

static PyValue
_io_BytesIO_truncate_impl(bytesio *self, PyValueBorrow size);

static PyValue
_io_BytesIO_truncate(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow size = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("truncate", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    size = PyValue_Borrow(args[0]);
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_BytesIO_truncate_impl((bytesio *)PyValue_AsKnownObjectBorrow(self), size);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

static PyObject *
bytesio_iternext_impl(bytesio *self);

static PyObject *
bytesio_iternext(PyObject *self)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytesio_iternext_impl((bytesio *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_seek__doc__,
"seek($self, pos, whence=0, /)\n"
"--\n"
"\n"
"Change stream position.\n"
"\n"
"Seek to byte offset pos relative to position indicated by whence:\n"
"     0  Start of stream (the default).  pos should be >= 0;\n"
"     1  Current position - pos may be negative;\n"
"     2  End of stream - pos usually negative.\n"
"Returns the new absolute position.");

#define _IO_BYTESIO_SEEK_METHODDEF    \
    {"seek", _PyCFunction_CAST(_io_BytesIO_seek), METH_VALUE, _io_BytesIO_seek__doc__},

static PyValue
_io_BytesIO_seek_impl(bytesio *self, Py_ssize_t pos, int whence);

static PyValue
_io_BytesIO_seek(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t pos;
    int whence = 0;

    if (!_PyArg_CheckPositional("seek", nargs, 1, 2)) {
        goto exit;
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
        pos = ival;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    whence = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (whence == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_BytesIO_seek_impl((bytesio *)PyValue_AsKnownObjectBorrow(self), pos, whence);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_write__doc__,
"write($self, b, /)\n"
"--\n"
"\n"
"Write bytes to file.\n"
"\n"
"Return the number of bytes written.");

#define _IO_BYTESIO_WRITE_METHODDEF    \
    {"write", _PyCFunction_CAST(_io_BytesIO_write), METH_VALUE|METH_O, _io_BytesIO_write__doc__},

static PyValue
_io_BytesIO_write_impl(bytesio *self, PyValueBorrow b);

static PyValue
_io_BytesIO_write(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow b;

    b = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_BytesIO_write_impl((bytesio *)PyValue_AsKnownObjectBorrow(self), b);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_writelines__doc__,
"writelines($self, lines, /)\n"
"--\n"
"\n"
"Write lines to the file.\n"
"\n"
"Note that newlines are not added.  lines can be any iterable object\n"
"producing bytes-like objects.  This is equivalent to calling write()\n"
"for each element.");

#define _IO_BYTESIO_WRITELINES_METHODDEF    \
    {"writelines", _PyCFunction_CAST(_io_BytesIO_writelines), METH_VALUE|METH_O, _io_BytesIO_writelines__doc__},

static PyValue
_io_BytesIO_writelines_impl(bytesio *self, PyValueBorrow lines);

static PyValue
_io_BytesIO_writelines(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow lines;

    lines = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_BytesIO_writelines_impl((bytesio *)PyValue_AsKnownObjectBorrow(self), lines);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Disable all I/O operations.");

#define _IO_BYTESIO_CLOSE_METHODDEF    \
    {"close", _PyCFunction_CAST(_io_BytesIO_close), METH_VALUE|METH_NOARGS, _io_BytesIO_close__doc__},

static PyValue
_io_BytesIO_close_impl(bytesio *self);

static PyValue
_io_BytesIO_close(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_BytesIO_close_impl((bytesio *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_BytesIO___getstate____doc__,
"__getstate__($self, /)\n"
"--\n"
"\n");

#define _IO_BYTESIO___GETSTATE___METHODDEF    \
    {"__getstate__", _PyCFunction_CAST(_io_BytesIO___getstate__), METH_VALUE|METH_NOARGS, _io_BytesIO___getstate____doc__},

static PyValue
_io_BytesIO___getstate___impl(bytesio *self);

static PyValue
_io_BytesIO___getstate__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_BytesIO___getstate___impl((bytesio *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_BytesIO___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n");

#define _IO_BYTESIO___SETSTATE___METHODDEF    \
    {"__setstate__", _PyCFunction_CAST(_io_BytesIO___setstate__), METH_VALUE|METH_O, _io_BytesIO___setstate____doc__},

static PyValue
_io_BytesIO___setstate___impl(bytesio *self, PyValueBorrow state);

static PyValue
_io_BytesIO___setstate__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow state;

    state = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_BytesIO___setstate___impl((bytesio *)PyValue_AsKnownObjectBorrow(self), state);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_BytesIO___init____doc__,
"BytesIO(initial_bytes=b\'\')\n"
"--\n"
"\n"
"Buffered I/O implementation using an in-memory bytes buffer.");

static int
_io_BytesIO___init___impl(bytesio *self, PyValueBorrow initvalue);

static int
_io_BytesIO___init___value(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    int return_value = -1;
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(initial_bytes)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"initial_bytes", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "BytesIO",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow initvalue = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        initvalue = PyValue_Borrow(args[_idx[0]]);
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_BytesIO___init___impl((bytesio *)self, initvalue);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

static int
_io_BytesIO___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _io_BytesIO___init___value(self, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = _io_BytesIO___init___value(self, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}

PyDoc_STRVAR(_io_BytesIO___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Size of object in memory, in bytes.");

#define _IO_BYTESIO___SIZEOF___METHODDEF    \
    {"__sizeof__", _PyCFunction_CAST(_io_BytesIO___sizeof__), METH_VALUE|METH_NOARGS, _io_BytesIO___sizeof____doc__},

static PyValue
_io_BytesIO___sizeof___impl(bytesio *self);

static PyValue
_io_BytesIO___sizeof__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_BytesIO___sizeof___impl((bytesio *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}
#define _IO_BYTESIO_CLOSED_GETSETDEF {"closed", (getter)_io_BytesIO_closed_get, (setter)NULL, _io_BytesIO_closed__doc__},

/*[clinic end generated code: output=b7ca754fc7d59bd2 input=a9049054013a1b77]*/
