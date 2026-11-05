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
#include "pycore_long_value.h"    // _PyValue_AsLong()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_Index()

PyDoc_STRVAR(mmap_mmap_close__doc__,
"close($self, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_CLOSE_METHODDEF    \
    {"close", _PyCFunction_CAST(mmap_mmap_close), METH_VALUE|METH_NOARGS, mmap_mmap_close__doc__},

static PyValue
mmap_mmap_close_impl(mmap_object *self);

static PyValue
mmap_mmap_close(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = mmap_mmap_close_impl((mmap_object *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(mmap_mmap_read_byte__doc__,
"read_byte($self, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_READ_BYTE_METHODDEF    \
    {"read_byte", _PyCFunction_CAST(mmap_mmap_read_byte), METH_VALUE|METH_NOARGS, mmap_mmap_read_byte__doc__},

static PyValue
mmap_mmap_read_byte_impl(mmap_object *self);

static PyValue
mmap_mmap_read_byte(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = mmap_mmap_read_byte_impl((mmap_object *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(mmap_mmap_readline__doc__,
"readline($self, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_READLINE_METHODDEF    \
    {"readline", _PyCFunction_CAST(mmap_mmap_readline), METH_VALUE|METH_NOARGS, mmap_mmap_readline__doc__},

static PyValue
mmap_mmap_readline_impl(mmap_object *self);

static PyValue
mmap_mmap_readline(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = mmap_mmap_readline_impl((mmap_object *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(mmap_mmap_read__doc__,
"read($self, n=None, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_READ_METHODDEF    \
    {"read", _PyCFunction_CAST(mmap_mmap_read), METH_VALUE, mmap_mmap_read__doc__},

static PyValue
mmap_mmap_read_impl(mmap_object *self, Py_ssize_t num_bytes);

static PyValue
mmap_mmap_read(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t num_bytes = PY_SSIZE_T_MAX;

    if (!_PyArg_CheckPositional("read", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (!_Py_convert_optional_to_ssize_t_value(PyValue_Borrow(args[0]), &num_bytes)) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = mmap_mmap_read_impl((mmap_object *)PyValue_AsKnownObjectBorrow(self), num_bytes);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(mmap_mmap_find__doc__,
"find($self, view, start=None, end=None, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_FIND_METHODDEF    \
    {"find", _PyCFunction_CAST(mmap_mmap_find), METH_VALUE, mmap_mmap_find__doc__},

static PyValue
mmap_mmap_find_impl(mmap_object *self, Py_buffer *view, PyValueBorrow start,
                    PyValueBorrow end);

static PyValue
mmap_mmap_find(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer view = {NULL, NULL};
    PyValueBorrow start = PyValue_Borrow(PyValue_None);
    PyValueBorrow end = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("find", nargs, 1, 3)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &view, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    start = PyValue_Borrow(args[1]);
    if (nargs < 3) {
        goto skip_optional;
    }
    end = PyValue_Borrow(args[2]);
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = mmap_mmap_find_impl((mmap_object *)PyValue_AsKnownObjectBorrow(self), &view, start, end);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for view */
    if (view.obj) {
       PyBuffer_Release(&view);
    }

    return return_value;
}

PyDoc_STRVAR(mmap_mmap_rfind__doc__,
"rfind($self, view, start=None, end=None, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_RFIND_METHODDEF    \
    {"rfind", _PyCFunction_CAST(mmap_mmap_rfind), METH_VALUE, mmap_mmap_rfind__doc__},

static PyValue
mmap_mmap_rfind_impl(mmap_object *self, Py_buffer *view, PyValueBorrow start,
                     PyValueBorrow end);

static PyValue
mmap_mmap_rfind(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer view = {NULL, NULL};
    PyValueBorrow start = PyValue_Borrow(PyValue_None);
    PyValueBorrow end = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("rfind", nargs, 1, 3)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &view, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    start = PyValue_Borrow(args[1]);
    if (nargs < 3) {
        goto skip_optional;
    }
    end = PyValue_Borrow(args[2]);
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = mmap_mmap_rfind_impl((mmap_object *)PyValue_AsKnownObjectBorrow(self), &view, start, end);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for view */
    if (view.obj) {
       PyBuffer_Release(&view);
    }

    return return_value;
}

PyDoc_STRVAR(mmap_mmap_write__doc__,
"write($self, bytes, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_WRITE_METHODDEF    \
    {"write", _PyCFunction_CAST(mmap_mmap_write), METH_VALUE|METH_O, mmap_mmap_write__doc__},

static PyValue
mmap_mmap_write_impl(mmap_object *self, Py_buffer *data);

static PyValue
mmap_mmap_write(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer data = {NULL, NULL};

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = mmap_mmap_write_impl((mmap_object *)PyValue_AsKnownObjectBorrow(self), &data);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(mmap_mmap_write_byte__doc__,
"write_byte($self, byte, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_WRITE_BYTE_METHODDEF    \
    {"write_byte", _PyCFunction_CAST(mmap_mmap_write_byte), METH_VALUE|METH_O, mmap_mmap_write_byte__doc__},

static PyValue
mmap_mmap_write_byte_impl(mmap_object *self, unsigned char value);

static PyValue
mmap_mmap_write_byte(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    unsigned char value;

    {
        long ival = _PyValue_AsLong(PyValue_Borrow(args[0]));
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        else if (ival < 0) {
            PyErr_SetString(PyExc_OverflowError,
                            "unsigned byte integer is less than minimum");
            goto exit;
        }
        else if (ival > UCHAR_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "unsigned byte integer is greater than maximum");
            goto exit;
        }
        else {
            value = (unsigned char) ival;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = mmap_mmap_write_byte_impl((mmap_object *)PyValue_AsKnownObjectBorrow(self), value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(mmap_mmap_size__doc__,
"size($self, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_SIZE_METHODDEF    \
    {"size", _PyCFunction_CAST(mmap_mmap_size), METH_VALUE|METH_NOARGS, mmap_mmap_size__doc__},

static PyValue
mmap_mmap_size_impl(mmap_object *self);

static PyValue
mmap_mmap_size(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = mmap_mmap_size_impl((mmap_object *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if (defined(MS_WINDOWS) || defined(HAVE_MREMAP))

PyDoc_STRVAR(mmap_mmap_resize__doc__,
"resize($self, newsize, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_RESIZE_METHODDEF    \
    {"resize", _PyCFunction_CAST(mmap_mmap_resize), METH_VALUE|METH_O, mmap_mmap_resize__doc__},

static PyValue
mmap_mmap_resize_impl(mmap_object *self, Py_ssize_t new_size);

static PyValue
mmap_mmap_resize(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t new_size;

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
        new_size = ival;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = mmap_mmap_resize_impl((mmap_object *)PyValue_AsKnownObjectBorrow(self), new_size);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#endif /* (defined(MS_WINDOWS) || defined(HAVE_MREMAP)) */

PyDoc_STRVAR(mmap_mmap_tell__doc__,
"tell($self, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_TELL_METHODDEF    \
    {"tell", _PyCFunction_CAST(mmap_mmap_tell), METH_VALUE|METH_NOARGS, mmap_mmap_tell__doc__},

static PyValue
mmap_mmap_tell_impl(mmap_object *self);

static PyValue
mmap_mmap_tell(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = mmap_mmap_tell_impl((mmap_object *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(mmap_mmap_flush__doc__,
"flush($self, offset=0, size=-1, /, *, flags=0)\n"
"--\n"
"\n");

#define MMAP_MMAP_FLUSH_METHODDEF    \
    {"flush", _PyCFunction_CAST(mmap_mmap_flush), METH_VALUE|METH_KEYWORDS, mmap_mmap_flush__doc__},

static PyValue
mmap_mmap_flush_impl(mmap_object *self, Py_ssize_t offset, Py_ssize_t size,
                     int flags);

static PyValue
mmap_mmap_flush(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(flags)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "", "flags", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "flush",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    Py_ssize_t offset = 0;
    Py_ssize_t size = -1;
    int flags = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        {
            Py_ssize_t ival = -1;
            PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[0]]));
            if (!PyValue_IsNull(iobj)) {
                ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
                PyValue_CLOSE(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            offset = ival;
        }
    }
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
            size = ival;
        }
    }
    if (_idx[2] >= 0) {
        flags = _PyValue_AsInt(PyValue_Borrow(args[_idx[2]]));
        if (flags == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = mmap_mmap_flush_impl((mmap_object *)PyValue_AsKnownObjectBorrow(self), offset, size, flags);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(mmap_mmap_seek__doc__,
"seek($self, pos, whence=0, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_SEEK_METHODDEF    \
    {"seek", _PyCFunction_CAST(mmap_mmap_seek), METH_VALUE, mmap_mmap_seek__doc__},

static PyValue
mmap_mmap_seek_impl(mmap_object *self, Py_ssize_t dist, int how);

static PyValue
mmap_mmap_seek(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t dist;
    int how = 0;

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
        dist = ival;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    how = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (how == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = mmap_mmap_seek_impl((mmap_object *)PyValue_AsKnownObjectBorrow(self), dist, how);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(mmap_mmap_set_name__doc__,
"set_name($self, name, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_SET_NAME_METHODDEF    \
    {"set_name", _PyCFunction_CAST(mmap_mmap_set_name), METH_VALUE|METH_O, mmap_mmap_set_name__doc__},

static PyValue
mmap_mmap_set_name_impl(mmap_object *self, const char *name);

static PyValue
mmap_mmap_set_name(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *name;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("set_name", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("set_name", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    name = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (name == NULL) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = mmap_mmap_set_name_impl((mmap_object *)PyValue_AsKnownObjectBorrow(self), name);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(mmap_mmap_seekable__doc__,
"seekable($self, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_SEEKABLE_METHODDEF    \
    {"seekable", _PyCFunction_CAST(mmap_mmap_seekable), METH_VALUE|METH_NOARGS, mmap_mmap_seekable__doc__},

static PyValue
mmap_mmap_seekable_impl(mmap_object *self);

static PyValue
mmap_mmap_seekable(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = mmap_mmap_seekable_impl((mmap_object *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(mmap_mmap_move__doc__,
"move($self, dest, src, count, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_MOVE_METHODDEF    \
    {"move", _PyCFunction_CAST(mmap_mmap_move), METH_VALUE, mmap_mmap_move__doc__},

static PyValue
mmap_mmap_move_impl(mmap_object *self, Py_ssize_t dest, Py_ssize_t src,
                    Py_ssize_t cnt);

static PyValue
mmap_mmap_move(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t dest;
    Py_ssize_t src;
    Py_ssize_t cnt;

    if (!_PyArg_CheckPositional("move", nargs, 3, 3)) {
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
        dest = ival;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[1]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        src = ival;
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
        cnt = ival;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = mmap_mmap_move_impl((mmap_object *)PyValue_AsKnownObjectBorrow(self), dest, src, cnt);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(mmap_mmap___enter____doc__,
"__enter__($self, /)\n"
"--\n"
"\n");

#define MMAP_MMAP___ENTER___METHODDEF    \
    {"__enter__", _PyCFunction_CAST(mmap_mmap___enter__), METH_VALUE|METH_NOARGS, mmap_mmap___enter____doc__},

static PyValue
mmap_mmap___enter___impl(mmap_object *self);

static PyValue
mmap_mmap___enter__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = mmap_mmap___enter___impl((mmap_object *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(mmap_mmap___exit____doc__,
"__exit__($self, exc_type, exc_value, traceback, /)\n"
"--\n"
"\n");

#define MMAP_MMAP___EXIT___METHODDEF    \
    {"__exit__", _PyCFunction_CAST(mmap_mmap___exit__), METH_VALUE, mmap_mmap___exit____doc__},

static PyValue
mmap_mmap___exit___impl(mmap_object *self, PyValueBorrow exc_type,
                        PyValueBorrow exc_value, PyValueBorrow traceback);

static PyValue
mmap_mmap___exit__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow exc_type;
    PyValueBorrow exc_value;
    PyValueBorrow traceback;

    if (!_PyArg_CheckPositional("__exit__", nargs, 3, 3)) {
        goto exit;
    }
    exc_type = PyValue_Borrow(args[0]);
    exc_value = PyValue_Borrow(args[1]);
    traceback = PyValue_Borrow(args[2]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = mmap_mmap___exit___impl((mmap_object *)PyValue_AsKnownObjectBorrow(self), exc_type, exc_value, traceback);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

static PyObject *
mmap__repr__method_impl(PyObject *op);

static PyObject *
mmap__repr__method(PyObject *op)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(op);
    return_value = mmap__repr__method_impl(op);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if defined(MS_WINDOWS)

PyDoc_STRVAR(mmap_mmap___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n");

#define MMAP_MMAP___SIZEOF___METHODDEF    \
    {"__sizeof__", _PyCFunction_CAST(mmap_mmap___sizeof__), METH_VALUE|METH_NOARGS, mmap_mmap___sizeof____doc__},

static PyValue
mmap_mmap___sizeof___impl(mmap_object *self);

static PyValue
mmap_mmap___sizeof__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = mmap_mmap___sizeof___impl((mmap_object *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if (defined(MS_WINDOWS) && defined(Py_DEBUG))

PyDoc_STRVAR(mmap_mmap__protect__doc__,
"_protect($self, flNewProtect, start, length, /)\n"
"--\n"
"\n");

#define MMAP_MMAP__PROTECT_METHODDEF    \
    {"_protect", _PyCFunction_CAST(mmap_mmap__protect), METH_VALUE, mmap_mmap__protect__doc__},

static PyValue
mmap_mmap__protect_impl(mmap_object *self, unsigned int flNewProtect,
                        Py_ssize_t start, Py_ssize_t length);

static PyValue
mmap_mmap__protect(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    unsigned int flNewProtect;
    Py_ssize_t start;
    Py_ssize_t length;

    if (!_PyArg_CheckPositional("_protect", nargs, 3, 3)) {
        goto exit;
    }
    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[0]), &flNewProtect, sizeof(unsigned int),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(unsigned int)) {
            if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "integer value out of range", 1) < 0)
            {
                goto exit;
            }
        }
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[1]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        start = ival;
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
        length = ival;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = mmap_mmap__protect_impl((mmap_object *)PyValue_AsKnownObjectBorrow(self), flNewProtect, start, length);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#endif /* (defined(MS_WINDOWS) && defined(Py_DEBUG)) */

#if defined(HAVE_MADVISE)

PyDoc_STRVAR(mmap_mmap_madvise__doc__,
"madvise($self, option, start=0, length=None, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_MADVISE_METHODDEF    \
    {"madvise", _PyCFunction_CAST(mmap_mmap_madvise), METH_VALUE, mmap_mmap_madvise__doc__},

static PyValue
mmap_mmap_madvise_impl(mmap_object *self, int option, Py_ssize_t start,
                       PyValueBorrow length_obj);

static PyValue
mmap_mmap_madvise(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int option;
    Py_ssize_t start = 0;
    PyValueBorrow length_obj = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("madvise", nargs, 1, 3)) {
        goto exit;
    }
    option = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (option == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[1]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        start = ival;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    length_obj = PyValue_Borrow(args[2]);
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = mmap_mmap_madvise_impl((mmap_object *)PyValue_AsKnownObjectBorrow(self), option, start, length_obj);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#endif /* defined(HAVE_MADVISE) */

static Py_ssize_t
mmap_length_impl(mmap_object *self);

static Py_ssize_t
mmap_length(PyObject *self)
{
    Py_ssize_t return_value = -1;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = mmap_length_impl((mmap_object *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
mmap_item_impl(mmap_object *self, Py_ssize_t i);

static PyObject *
mmap_item(PyObject *self, Py_ssize_t i)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = mmap_item_impl((mmap_object *)self, i);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
mmap_subscript_impl(mmap_object *self, PyObject *item);

static PyObject *
mmap_subscript(PyObject *self, PyObject *item)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = mmap_subscript_impl((mmap_object *)self, item);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
mmap_ass_item_impl(mmap_object *self, Py_ssize_t i, PyObject *v);

static int
mmap_ass_item(PyObject *self, Py_ssize_t i, PyObject *v)
{
    int return_value = -1;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = mmap_ass_item_impl((mmap_object *)self, i, v);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
mmap_ass_subscript_impl(mmap_object *self, PyObject *item, PyObject *value);

static int
mmap_ass_subscript(PyObject *self, PyObject *item, PyObject *value)
{
    int return_value = -1;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = mmap_ass_subscript_impl((mmap_object *)self, item, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#ifndef MMAP_MMAP_RESIZE_METHODDEF
    #define MMAP_MMAP_RESIZE_METHODDEF
#endif /* !defined(MMAP_MMAP_RESIZE_METHODDEF) */

#ifndef MMAP_MMAP___SIZEOF___METHODDEF
    #define MMAP_MMAP___SIZEOF___METHODDEF
#endif /* !defined(MMAP_MMAP___SIZEOF___METHODDEF) */

#ifndef MMAP_MMAP__PROTECT_METHODDEF
    #define MMAP_MMAP__PROTECT_METHODDEF
#endif /* !defined(MMAP_MMAP__PROTECT_METHODDEF) */

#ifndef MMAP_MMAP_MADVISE_METHODDEF
    #define MMAP_MMAP_MADVISE_METHODDEF
#endif /* !defined(MMAP_MMAP_MADVISE_METHODDEF) */
/*[clinic end generated code: output=04f673a6ace985d2 input=a9049054013a1b77]*/
