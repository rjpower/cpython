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
#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_IsTrue()

PyDoc_STRVAR(_io_FileIO__dealloc_warn__doc__,
"_dealloc_warn($self, source, /)\n"
"--\n"
"\n");

#define _IO_FILEIO__DEALLOC_WARN_METHODDEF    \
    {"_dealloc_warn", _PyCFunction_CAST(_io_FileIO__dealloc_warn), METH_VALUE|METH_O, _io_FileIO__dealloc_warn__doc__},

static PyValue
_io_FileIO__dealloc_warn_impl(fileio *self, PyValueBorrow source);

static PyValue
_io_FileIO__dealloc_warn(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow source;

    source = PyValue_Borrow(args[0]);
    return_value = _io_FileIO__dealloc_warn_impl((fileio *)PyValue_AsKnownObjectBorrow(self), source);

    return return_value;
}

PyDoc_STRVAR(_io_FileIO_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Close the file.\n"
"\n"
"A closed file cannot be used for further I/O operations.  close()\n"
"may be called more than once without error.");

#define _IO_FILEIO_CLOSE_METHODDEF    \
    {"close", _PyCFunction_CAST(_io_FileIO_close), METH_VALUE|METH_METHOD|METH_KEYWORDS, _io_FileIO_close__doc__},

static PyValue
_io_FileIO_close_impl(fileio *self, PyTypeObject *cls);

static PyValue
_io_FileIO_close(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "close() takes no arguments");
        goto exit;
    }
    return_value = _io_FileIO_close_impl((fileio *)PyValue_AsKnownObjectBorrow(self), cls);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_FileIO___init____doc__,
"FileIO(file, mode=\'r\', closefd=True, opener=None)\n"
"--\n"
"\n"
"Open a file.\n"
"\n"
"The mode can be \'r\' (default), \'w\', \'x\' or \'a\' for reading,\n"
"writing, exclusive creation or appending.  The file will be created\n"
"if it doesn\'t exist when opened for writing or appending; it will be\n"
"truncated when opened for writing.  A FileExistsError will be raised\n"
"if it already exists when opened for creating.  Opening a file for\n"
"creating implies writing so this mode behaves in a similar way to\n"
"\'w\'.  Add a \'+\' to the mode to allow simultaneous reading and\n"
"writing.\n"
"\n"
"A custom opener can be used by passing a callable as *opener*.\n"
"The underlying file descriptor for the file object is then obtained\n"
"by calling opener with (*name*, *flags*).  *opener* must return\n"
"an open file descriptor (passing os.open as *opener* results in\n"
"functionality similar to passing None).");

static int
_io_FileIO___init___impl(fileio *self, PyValueBorrow nameobj,
                         const char *mode, int closefd, PyValueBorrow opener);

static int
_io_FileIO___init___value(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(file)), _PyValue_STATIC_IMMORTAL(&_Py_ID(mode)), _PyValue_STATIC_IMMORTAL(&_Py_ID(closefd)), _PyValue_STATIC_IMMORTAL(&_Py_ID(opener)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"file", "mode", "closefd", "opener", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "FileIO",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[4];
    PyValueBorrow nameobj;
    const char *mode = "r";
    int closefd = 1;
    PyValueBorrow opener = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, _idx, 4)) {
        goto exit;
    }
    nameobj = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
            _PyArg_BadArgumentValue("FileIO", "argument 'mode'", "str", PyValue_Borrow(args[_idx[1]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])))) {
            _PyArg_BadArgument("FileIO", "argument 'mode'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
            goto exit;
        }
        mode = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
        if (mode == NULL) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        closefd = _PyValue_IsTrue(PyValue_Borrow(args[_idx[2]]));
        if (closefd < 0) {
            goto exit;
        }
    }
    if (_idx[3] >= 0) {
        opener = PyValue_Borrow(args[_idx[3]]);
    }
    return_value = _io_FileIO___init___impl((fileio *)self, nameobj, mode, closefd, opener);

exit:
    return return_value;
}

static int
_io_FileIO___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _io_FileIO___init___value(self, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = _io_FileIO___init___value(self, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}

PyDoc_STRVAR(_io_FileIO_fileno__doc__,
"fileno($self, /)\n"
"--\n"
"\n"
"Return the underlying file descriptor (an integer).");

#define _IO_FILEIO_FILENO_METHODDEF    \
    {"fileno", _PyCFunction_CAST(_io_FileIO_fileno), METH_VALUE|METH_NOARGS, _io_FileIO_fileno__doc__},

static PyValue
_io_FileIO_fileno_impl(fileio *self);

static PyValue
_io_FileIO_fileno(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _io_FileIO_fileno_impl((fileio *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_io_FileIO_readable__doc__,
"readable($self, /)\n"
"--\n"
"\n"
"True if file was opened in a read mode.");

#define _IO_FILEIO_READABLE_METHODDEF    \
    {"readable", _PyCFunction_CAST(_io_FileIO_readable), METH_VALUE|METH_NOARGS, _io_FileIO_readable__doc__},

static PyValue
_io_FileIO_readable_impl(fileio *self);

static PyValue
_io_FileIO_readable(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _io_FileIO_readable_impl((fileio *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_io_FileIO_writable__doc__,
"writable($self, /)\n"
"--\n"
"\n"
"True if file was opened in a write mode.");

#define _IO_FILEIO_WRITABLE_METHODDEF    \
    {"writable", _PyCFunction_CAST(_io_FileIO_writable), METH_VALUE|METH_NOARGS, _io_FileIO_writable__doc__},

static PyValue
_io_FileIO_writable_impl(fileio *self);

static PyValue
_io_FileIO_writable(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _io_FileIO_writable_impl((fileio *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_io_FileIO_seekable__doc__,
"seekable($self, /)\n"
"--\n"
"\n"
"True if file supports random-access.");

#define _IO_FILEIO_SEEKABLE_METHODDEF    \
    {"seekable", _PyCFunction_CAST(_io_FileIO_seekable), METH_VALUE|METH_NOARGS, _io_FileIO_seekable__doc__},

static PyValue
_io_FileIO_seekable_impl(fileio *self);

static PyValue
_io_FileIO_seekable(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _io_FileIO_seekable_impl((fileio *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_io_FileIO_readinto__doc__,
"readinto($self, buffer, /)\n"
"--\n"
"\n"
"Same as RawIOBase.readinto().");

#define _IO_FILEIO_READINTO_METHODDEF    \
    {"readinto", _PyCFunction_CAST(_io_FileIO_readinto), METH_VALUE|METH_METHOD|METH_KEYWORDS, _io_FileIO_readinto__doc__},

static PyValue
_io_FileIO_readinto_impl(fileio *self, PyTypeObject *cls, Py_buffer *buffer);

static PyValue
_io_FileIO_readinto(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer buffer = {NULL, NULL};

    if (!_PyArg_NoKwnames("readinto", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("readinto", nargs, 1, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("readinto", "argument 1", "read-write bytes-like object", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &buffer, PyBUF_WRITABLE) < 0) {
        _PyArg_BadArgument("readinto", "argument 1", "read-write bytes-like object", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    return_value = _io_FileIO_readinto_impl((fileio *)PyValue_AsKnownObjectBorrow(self), cls, &buffer);

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(_io_FileIO_readall__doc__,
"readall($self, /)\n"
"--\n"
"\n"
"Read all data from the file, returned as bytes.\n"
"\n"
"Reads until either there is an error or read() returns size 0\n"
"(indicates EOF).  If the file is already at EOF, returns an empty\n"
"bytes object.\n"
"\n"
"In non-blocking mode, returns as much data as could be read before\n"
"EAGAIN.  If no data is available (EAGAIN is returned before bytes\n"
"are read) returns None.");

#define _IO_FILEIO_READALL_METHODDEF    \
    {"readall", _PyCFunction_CAST(_io_FileIO_readall), METH_VALUE|METH_METHOD|METH_KEYWORDS, _io_FileIO_readall__doc__},

static PyValue
_io_FileIO_readall_impl(fileio *self, PyTypeObject *cls);

static PyValue
_io_FileIO_readall(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "readall() takes no arguments");
        goto exit;
    }
    return_value = _io_FileIO_readall_impl((fileio *)PyValue_AsKnownObjectBorrow(self), cls);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_FileIO_read__doc__,
"read($self, size=-1, /)\n"
"--\n"
"\n"
"Read at most size bytes, returned as bytes.\n"
"\n"
"If size is less than 0, read all bytes in the file making multiple\n"
"read calls.  See ``FileIO.readall``.\n"
"\n"
"Attempts to make only one system call, retrying only per PEP 475\n"
"(EINTR).  This means less data may be returned than requested.\n"
"\n"
"In non-blocking mode, returns None if no data is available.  Return\n"
"an empty bytes object at EOF.");

#define _IO_FILEIO_READ_METHODDEF    \
    {"read", _PyCFunction_CAST(_io_FileIO_read), METH_VALUE|METH_METHOD|METH_KEYWORDS, _io_FileIO_read__doc__},

static PyValue
_io_FileIO_read_impl(fileio *self, PyTypeObject *cls, Py_ssize_t size);

static PyValue
_io_FileIO_read(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t size = -1;

    if (!_PyArg_NoKwnames("read", kwnames)) {
        goto exit;
    }
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
    return_value = _io_FileIO_read_impl((fileio *)PyValue_AsKnownObjectBorrow(self), cls, size);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_FileIO_write__doc__,
"write($self, b, /)\n"
"--\n"
"\n"
"Write buffer b to file, return number of bytes written.\n"
"\n"
"Only makes one system call, so not all of the data may be written.\n"
"The number of bytes actually written is returned.  In non-blocking\n"
"mode, returns None if the write would block.");

#define _IO_FILEIO_WRITE_METHODDEF    \
    {"write", _PyCFunction_CAST(_io_FileIO_write), METH_VALUE|METH_METHOD|METH_KEYWORDS, _io_FileIO_write__doc__},

static PyValue
_io_FileIO_write_impl(fileio *self, PyTypeObject *cls, Py_buffer *b);

static PyValue
_io_FileIO_write(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer b = {NULL, NULL};

    if (!_PyArg_NoKwnames("write", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("write", nargs, 1, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &b, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = _io_FileIO_write_impl((fileio *)PyValue_AsKnownObjectBorrow(self), cls, &b);

exit:
    /* Cleanup for b */
    if (b.obj) {
       PyBuffer_Release(&b);
    }

    return return_value;
}

PyDoc_STRVAR(_io_FileIO_seek__doc__,
"seek($self, pos, whence=0, /)\n"
"--\n"
"\n"
"Move to new file position and return the file position.\n"
"\n"
"Argument offset is a byte count.  Optional argument whence defaults\n"
"to SEEK_SET or 0 (offset from start of file, offset should be >= 0);\n"
"other values are SEEK_CUR or 1 (move relative to current position,\n"
"positive or negative), and SEEK_END or 2 (move relative to end of\n"
"file, usually negative, although many platforms allow seeking beyond\n"
"the end of a file).\n"
"\n"
"Note that not all file objects are seekable.");

#define _IO_FILEIO_SEEK_METHODDEF    \
    {"seek", _PyCFunction_CAST(_io_FileIO_seek), METH_VALUE, _io_FileIO_seek__doc__},

static PyValue
_io_FileIO_seek_impl(fileio *self, PyValueBorrow pos, int whence);

static PyValue
_io_FileIO_seek(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow pos;
    int whence = 0;

    if (!_PyArg_CheckPositional("seek", nargs, 1, 2)) {
        goto exit;
    }
    pos = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    whence = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (whence == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _io_FileIO_seek_impl((fileio *)PyValue_AsKnownObjectBorrow(self), pos, whence);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_FileIO_tell__doc__,
"tell($self, /)\n"
"--\n"
"\n"
"Current file position.\n"
"\n"
"Can raise OSError for non seekable files.");

#define _IO_FILEIO_TELL_METHODDEF    \
    {"tell", _PyCFunction_CAST(_io_FileIO_tell), METH_VALUE|METH_NOARGS, _io_FileIO_tell__doc__},

static PyValue
_io_FileIO_tell_impl(fileio *self);

static PyValue
_io_FileIO_tell(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _io_FileIO_tell_impl((fileio *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

#if defined(HAVE_FTRUNCATE)

PyDoc_STRVAR(_io_FileIO_truncate__doc__,
"truncate($self, size=None, /)\n"
"--\n"
"\n"
"Truncate the file to at most size bytes and return the truncated size.\n"
"\n"
"Size defaults to the current file position, as returned by tell().\n"
"The current file position is changed to the value of size.");

#define _IO_FILEIO_TRUNCATE_METHODDEF    \
    {"truncate", _PyCFunction_CAST(_io_FileIO_truncate), METH_VALUE|METH_METHOD|METH_KEYWORDS, _io_FileIO_truncate__doc__},

static PyValue
_io_FileIO_truncate_impl(fileio *self, PyTypeObject *cls,
                         PyValueBorrow posobj);

static PyValue
_io_FileIO_truncate(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow posobj = PyValue_Borrow(PyValue_None);

    if (!_PyArg_NoKwnames("truncate", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("truncate", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    posobj = PyValue_Borrow(args[0]);
skip_optional:
    return_value = _io_FileIO_truncate_impl((fileio *)PyValue_AsKnownObjectBorrow(self), cls, posobj);

exit:
    return return_value;
}

#endif /* defined(HAVE_FTRUNCATE) */

PyDoc_STRVAR(_io_FileIO_isatty__doc__,
"isatty($self, /)\n"
"--\n"
"\n"
"True if the file is connected to a TTY device.");

#define _IO_FILEIO_ISATTY_METHODDEF    \
    {"isatty", _PyCFunction_CAST(_io_FileIO_isatty), METH_VALUE|METH_NOARGS, _io_FileIO_isatty__doc__},

static PyValue
_io_FileIO_isatty_impl(fileio *self);

static PyValue
_io_FileIO_isatty(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _io_FileIO_isatty_impl((fileio *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_io_FileIO__isatty_open_only__doc__,
"_isatty_open_only($self, /)\n"
"--\n"
"\n");

#define _IO_FILEIO__ISATTY_OPEN_ONLY_METHODDEF    \
    {"_isatty_open_only", _PyCFunction_CAST(_io_FileIO__isatty_open_only), METH_VALUE|METH_NOARGS, _io_FileIO__isatty_open_only__doc__},

static PyValue
_io_FileIO__isatty_open_only_impl(fileio *self);

static PyValue
_io_FileIO__isatty_open_only(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _io_FileIO__isatty_open_only_impl((fileio *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_io_FileIO_closed__doc__,
"True if the file is closed.");

static int
_io_FileIO_closed_get_impl(fileio *self);

static PyObject *
_io_FileIO_closed_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = _io_FileIO_closed_get_impl((fileio *)self);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_FileIO_closefd__doc__,
"True if the file descriptor will be closed by close().");

static int
_io_FileIO_closefd_get_impl(fileio *self);

static PyObject *
_io_FileIO_closefd_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = _io_FileIO_closefd_get_impl((fileio *)self);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_FileIO_mode__doc__,
"String giving the file mode.");

static PyObject *
_io_FileIO_mode_get_impl(fileio *self);

static PyObject *
_io_FileIO_mode_get(PyObject *self, void *Py_UNUSED(context))
{
    return _io_FileIO_mode_get_impl((fileio *)self);
}

PyDoc_STRVAR(_io_FileIO__blksize__doc__,
"Stat st_blksize if available.");

static long
_io_FileIO__blksize_get_impl(fileio *self);

static PyObject *
_io_FileIO__blksize_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;
    long _return_value;

    _return_value = _io_FileIO__blksize_get_impl((fileio *)self);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong(_return_value);

exit:
    return return_value;
}

#ifndef _IO_FILEIO_TRUNCATE_METHODDEF
    #define _IO_FILEIO_TRUNCATE_METHODDEF
#endif /* !defined(_IO_FILEIO_TRUNCATE_METHODDEF) */
#define _IO_FILEIO_CLOSED_GETSETDEF {"closed", (getter)_io_FileIO_closed_get, (setter)NULL, _io_FileIO_closed__doc__},

#define _IO_FILEIO_CLOSEFD_GETSETDEF {"closefd", (getter)_io_FileIO_closefd_get, (setter)NULL, _io_FileIO_closefd__doc__},

#define _IO_FILEIO_MODE_GETSETDEF {"mode", (getter)_io_FileIO_mode_get, (setter)NULL, _io_FileIO_mode__doc__},

#define _IO_FILEIO__BLKSIZE_GETSETDEF {"_blksize", (getter)_io_FileIO__blksize_get, (setter)NULL, _io_FileIO__blksize__doc__},

/*[clinic end generated code: output=439c83b2215e44dc input=a9049054013a1b77]*/
