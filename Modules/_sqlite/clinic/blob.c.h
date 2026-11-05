/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(blob_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Close the blob.");

#define BLOB_CLOSE_METHODDEF    \
    {"close", _PyCFunction_CAST(blob_close), METH_VALUE|METH_NOARGS, blob_close__doc__},

static PyValue
blob_close_impl(pysqlite_Blob *self);

static PyValue
blob_close(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = blob_close_impl((pysqlite_Blob *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(blob_read__doc__,
"read($self, length=-1, /)\n"
"--\n"
"\n"
"Read data at the current offset position.\n"
"\n"
"  length\n"
"    Read length in bytes.\n"
"\n"
"If the end of the blob is reached, the data up to end of file will\n"
"be returned.  When length is not specified, or is negative,\n"
"Blob.read() will read until the end of the blob.");

#define BLOB_READ_METHODDEF    \
    {"read", _PyCFunction_CAST(blob_read), METH_VALUE, blob_read__doc__},

static PyValue
blob_read_impl(pysqlite_Blob *self, int length);

static PyValue
blob_read(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int length = -1;

    if (!_PyArg_CheckPositional("read", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    length = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (length == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = blob_read_impl((pysqlite_Blob *)PyValue_AsKnownObjectBorrow(self), length);

exit:
    return return_value;
}

PyDoc_STRVAR(blob_write__doc__,
"write($self, data, /)\n"
"--\n"
"\n"
"Write data at the current offset.\n"
"\n"
"This function cannot change the blob length.  Writing beyond the end\n"
"of the blob will result in an exception being raised.");

#define BLOB_WRITE_METHODDEF    \
    {"write", _PyCFunction_CAST(blob_write), METH_VALUE|METH_O, blob_write__doc__},

static PyValue
blob_write_impl(pysqlite_Blob *self, Py_buffer *data);

static PyValue
blob_write(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
    return_value = blob_write_impl((pysqlite_Blob *)PyValue_AsKnownObjectBorrow(self), &data);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(blob_seek__doc__,
"seek($self, offset, origin=0, /)\n"
"--\n"
"\n"
"Set the current access position to offset.\n"
"\n"
"The origin argument defaults to os.SEEK_SET (absolute blob\n"
"positioning).  Other values for origin are os.SEEK_CUR (seek\n"
"relative to the current position) and os.SEEK_END (seek relative to\n"
"the blob\'s end).");

#define BLOB_SEEK_METHODDEF    \
    {"seek", _PyCFunction_CAST(blob_seek), METH_VALUE, blob_seek__doc__},

static PyValue
blob_seek_impl(pysqlite_Blob *self, int offset, int origin);

static PyValue
blob_seek(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int offset;
    int origin = 0;

    if (!_PyArg_CheckPositional("seek", nargs, 1, 2)) {
        goto exit;
    }
    offset = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (offset == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    origin = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (origin == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = blob_seek_impl((pysqlite_Blob *)PyValue_AsKnownObjectBorrow(self), offset, origin);

exit:
    return return_value;
}

PyDoc_STRVAR(blob_tell__doc__,
"tell($self, /)\n"
"--\n"
"\n"
"Return the current access position for the blob.");

#define BLOB_TELL_METHODDEF    \
    {"tell", _PyCFunction_CAST(blob_tell), METH_VALUE|METH_NOARGS, blob_tell__doc__},

static PyValue
blob_tell_impl(pysqlite_Blob *self);

static PyValue
blob_tell(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = blob_tell_impl((pysqlite_Blob *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(blob_enter__doc__,
"__enter__($self, /)\n"
"--\n"
"\n"
"Blob context manager enter.");

#define BLOB_ENTER_METHODDEF    \
    {"__enter__", _PyCFunction_CAST(blob_enter), METH_VALUE|METH_NOARGS, blob_enter__doc__},

static PyValue
blob_enter_impl(pysqlite_Blob *self);

static PyValue
blob_enter(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = blob_enter_impl((pysqlite_Blob *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(blob_exit__doc__,
"__exit__($self, type, val, tb, /)\n"
"--\n"
"\n"
"Blob context manager exit.");

#define BLOB_EXIT_METHODDEF    \
    {"__exit__", _PyCFunction_CAST(blob_exit), METH_VALUE, blob_exit__doc__},

static PyValue
blob_exit_impl(pysqlite_Blob *self, PyValueBorrow type, PyValueBorrow val,
               PyValueBorrow tb);

static PyValue
blob_exit(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow type;
    PyValueBorrow val;
    PyValueBorrow tb;

    if (!_PyArg_CheckPositional("__exit__", nargs, 3, 3)) {
        goto exit;
    }
    type = PyValue_Borrow(args[0]);
    val = PyValue_Borrow(args[1]);
    tb = PyValue_Borrow(args[2]);
    return_value = blob_exit_impl((pysqlite_Blob *)PyValue_AsKnownObjectBorrow(self), type, val, tb);

exit:
    return return_value;
}
/*[clinic end generated code: output=f702265f6dafdb2a input=a9049054013a1b77]*/
