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
#include "pycore_modsupport.h"    // _PyArg_NoKwnamesValue()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_Index()

PyDoc_STRVAR(_io_StringIO_getvalue__doc__,
"getvalue($self, /)\n"
"--\n"
"\n"
"Retrieve the entire contents of the object.");

#define _IO_STRINGIO_GETVALUE_METHODDEF    \
    {"getvalue", _PyCFunction_CAST(_io_StringIO_getvalue), METH_VALUE|METH_NOARGS, _io_StringIO_getvalue__doc__},

static PyValue
_io_StringIO_getvalue_impl(stringio *self);

static PyValue
_io_StringIO_getvalue(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_StringIO_getvalue_impl((stringio *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_StringIO_tell__doc__,
"tell($self, /)\n"
"--\n"
"\n"
"Tell the current file position.");

#define _IO_STRINGIO_TELL_METHODDEF    \
    {"tell", _PyCFunction_CAST(_io_StringIO_tell), METH_VALUE|METH_NOARGS, _io_StringIO_tell__doc__},

static PyValue
_io_StringIO_tell_impl(stringio *self);

static PyValue
_io_StringIO_tell(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_StringIO_tell_impl((stringio *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_StringIO_read__doc__,
"read($self, size=-1, /)\n"
"--\n"
"\n"
"Read at most size characters, returned as a string.\n"
"\n"
"If the argument is negative or omitted, read until EOF\n"
"is reached. Return an empty string at EOF.");

#define _IO_STRINGIO_READ_METHODDEF    \
    {"read", _PyCFunction_CAST(_io_StringIO_read), METH_VALUE, _io_StringIO_read__doc__},

static PyValue
_io_StringIO_read_impl(stringio *self, Py_ssize_t size);

static PyValue
_io_StringIO_read(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t size = -1;

    if (!_PyArg_NoKwnamesValue("StringIO.read()", kwnames)) {
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
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_StringIO_read_impl((stringio *)PyValue_AsKnownObjectBorrow(self), size);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io_StringIO_readline__doc__,
"readline($self, size=-1, /)\n"
"--\n"
"\n"
"Read until newline or EOF.\n"
"\n"
"Returns an empty string if EOF is hit immediately.");

#define _IO_STRINGIO_READLINE_METHODDEF    \
    {"readline", _PyCFunction_CAST(_io_StringIO_readline), METH_VALUE, _io_StringIO_readline__doc__},

static PyValue
_io_StringIO_readline_impl(stringio *self, Py_ssize_t size);

static PyValue
_io_StringIO_readline(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t size = -1;

    if (!_PyArg_NoKwnamesValue("StringIO.readline()", kwnames)) {
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
    return_value = _io_StringIO_readline_impl((stringio *)PyValue_AsKnownObjectBorrow(self), size);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

static PyObject *
stringio_iternext_impl(stringio *self);

static PyObject *
stringio_iternext(PyObject *self)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = stringio_iternext_impl((stringio *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_StringIO_truncate__doc__,
"truncate($self, pos=None, /)\n"
"--\n"
"\n"
"Truncate size to pos.\n"
"\n"
"The pos argument defaults to the current file position, as\n"
"returned by tell().  The current file position is unchanged.\n"
"Returns the new absolute position.");

#define _IO_STRINGIO_TRUNCATE_METHODDEF    \
    {"truncate", _PyCFunction_CAST(_io_StringIO_truncate), METH_VALUE, _io_StringIO_truncate__doc__},

static PyValue
_io_StringIO_truncate_impl(stringio *self, PyValueBorrow pos);

static PyValue
_io_StringIO_truncate(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow pos = PyValue_Borrow(PyValue_None);

    if (!_PyArg_NoKwnamesValue("StringIO.truncate()", kwnames)) {
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
    return_value = _io_StringIO_truncate_impl((stringio *)PyValue_AsKnownObjectBorrow(self), pos);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io_StringIO_seek__doc__,
"seek($self, pos, whence=0, /)\n"
"--\n"
"\n"
"Change stream position.\n"
"\n"
"Seek to character offset pos relative to position indicated by\n"
"whence:\n"
"    0  Start of stream (the default).  pos should be >= 0;\n"
"    1  Current position - pos must be 0;\n"
"    2  End of stream - pos must be 0.\n"
"Returns the new absolute position.");

#define _IO_STRINGIO_SEEK_METHODDEF    \
    {"seek", _PyCFunction_CAST(_io_StringIO_seek), METH_VALUE, _io_StringIO_seek__doc__},

static PyValue
_io_StringIO_seek_impl(stringio *self, Py_ssize_t pos, int whence);

static PyValue
_io_StringIO_seek(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t pos;
    int whence = 0;

    if (!_PyArg_NoKwnamesValue("StringIO.seek()", kwnames)) {
        goto exit;
    }
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
    return_value = _io_StringIO_seek_impl((stringio *)PyValue_AsKnownObjectBorrow(self), pos, whence);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io_StringIO_write__doc__,
"write($self, s, /)\n"
"--\n"
"\n"
"Write string to file.\n"
"\n"
"Returns the number of characters written, which is always equal to\n"
"the length of the string.");

#define _IO_STRINGIO_WRITE_METHODDEF    \
    {"write", _PyCFunction_CAST(_io_StringIO_write), METH_VALUE|METH_O, _io_StringIO_write__doc__},

static PyValue
_io_StringIO_write_impl(stringio *self, PyValueBorrow obj);

static PyValue
_io_StringIO_write(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow obj;

    obj = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_StringIO_write_impl((stringio *)PyValue_AsKnownObjectBorrow(self), obj);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_StringIO_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Close the IO object.\n"
"\n"
"Attempting any further operation after the object is closed\n"
"will raise a ValueError.\n"
"\n"
"This method has no effect if the file is already closed.");

#define _IO_STRINGIO_CLOSE_METHODDEF    \
    {"close", _PyCFunction_CAST(_io_StringIO_close), METH_VALUE|METH_NOARGS, _io_StringIO_close__doc__},

static PyValue
_io_StringIO_close_impl(stringio *self);

static PyValue
_io_StringIO_close(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_StringIO_close_impl((stringio *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_StringIO___init____doc__,
"StringIO(initial_value=\'\', newline=\'\\n\')\n"
"--\n"
"\n"
"Text I/O implementation using an in-memory buffer.\n"
"\n"
"The initial_value argument sets the value of object.  The newline\n"
"argument is like the one of TextIOWrapper\'s constructor.");

static int
_io_StringIO___init___impl(stringio *self, PyValueBorrow value,
                           PyValueBorrow newline_obj);

static int
_io_StringIO___init___value(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(initial_value)), _PyValue_STATIC_IMMORTAL(&_Py_ID(newline)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"initial_value", "newline", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "StringIO",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow value = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow newline_obj = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        value = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        newline_obj = PyValue_Borrow(args[_idx[1]]);
    }
    return_value = _io_StringIO___init___impl((stringio *)self, value, newline_obj);

exit:
    return return_value;
}

static int
_io_StringIO___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _io_StringIO___init___value(self, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = _io_StringIO___init___value(self, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}

PyDoc_STRVAR(_io_StringIO_readable__doc__,
"readable($self, /)\n"
"--\n"
"\n"
"Returns True if the IO object can be read.");

#define _IO_STRINGIO_READABLE_METHODDEF    \
    {"readable", _PyCFunction_CAST(_io_StringIO_readable), METH_VALUE|METH_NOARGS, _io_StringIO_readable__doc__},

static PyValue
_io_StringIO_readable_impl(stringio *self);

static PyValue
_io_StringIO_readable(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_StringIO_readable_impl((stringio *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_StringIO_writable__doc__,
"writable($self, /)\n"
"--\n"
"\n"
"Returns True if the IO object can be written.");

#define _IO_STRINGIO_WRITABLE_METHODDEF    \
    {"writable", _PyCFunction_CAST(_io_StringIO_writable), METH_VALUE|METH_NOARGS, _io_StringIO_writable__doc__},

static PyValue
_io_StringIO_writable_impl(stringio *self);

static PyValue
_io_StringIO_writable(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_StringIO_writable_impl((stringio *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_StringIO_seekable__doc__,
"seekable($self, /)\n"
"--\n"
"\n"
"Returns True if the IO object can be seeked.");

#define _IO_STRINGIO_SEEKABLE_METHODDEF    \
    {"seekable", _PyCFunction_CAST(_io_StringIO_seekable), METH_VALUE|METH_NOARGS, _io_StringIO_seekable__doc__},

static PyValue
_io_StringIO_seekable_impl(stringio *self);

static PyValue
_io_StringIO_seekable(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_StringIO_seekable_impl((stringio *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_StringIO___getstate____doc__,
"__getstate__($self, /)\n"
"--\n"
"\n");

#define _IO_STRINGIO___GETSTATE___METHODDEF    \
    {"__getstate__", _PyCFunction_CAST(_io_StringIO___getstate__), METH_VALUE|METH_NOARGS, _io_StringIO___getstate____doc__},

static PyValue
_io_StringIO___getstate___impl(stringio *self);

static PyValue
_io_StringIO___getstate__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_StringIO___getstate___impl((stringio *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_StringIO___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n");

#define _IO_STRINGIO___SETSTATE___METHODDEF    \
    {"__setstate__", _PyCFunction_CAST(_io_StringIO___setstate__), METH_VALUE|METH_O, _io_StringIO___setstate____doc__},

static PyValue
_io_StringIO___setstate___impl(stringio *self, PyValueBorrow state);

static PyValue
_io_StringIO___setstate__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow state;

    state = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_StringIO___setstate___impl((stringio *)PyValue_AsKnownObjectBorrow(self), state);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
_io_StringIO_closed_get_impl(stringio *self);

static PyObject *
_io_StringIO_closed_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;
    int _return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _io_StringIO_closed_get_impl((stringio *)self);
    Py_END_CRITICAL_SECTION();
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

static int
_io_StringIO_line_buffering_get_impl(stringio *self);

static PyObject *
_io_StringIO_line_buffering_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;
    int _return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _io_StringIO_line_buffering_get_impl((stringio *)self);
    Py_END_CRITICAL_SECTION();
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

static PyObject *
_io_StringIO_newlines_get_impl(stringio *self);

static PyObject *
_io_StringIO_newlines_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_StringIO_newlines_get_impl((stringio *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}
#define _IO_STRINGIO_CLOSED_GETSETDEF {"closed", (getter)_io_StringIO_closed_get, (setter)NULL, NULL},

#define _IO_STRINGIO_LINE_BUFFERING_GETSETDEF {"line_buffering", (getter)_io_StringIO_line_buffering_get, (setter)NULL, NULL},

#define _IO_STRINGIO_NEWLINES_GETSETDEF {"newlines", (getter)_io_StringIO_newlines_get, (setter)NULL, NULL},

/*[clinic end generated code: output=d4aca7644462a4dc input=a9049054013a1b77]*/
