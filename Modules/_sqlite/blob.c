#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#define Py_VALUE_CORE 1
#include "blob.h"
#include "util.h"
#include "pycore_list.h"          // PyList_GetValue()
#include "pycore_value.h"         // PyValue
#include "pycore_value_ops.h"     // _PyValue_AsSsize_t()
#include "pycore_weakref.h"    // FT_CLEAR_WEAKREFS()

#define clinic_state() (pysqlite_get_state_by_type(Py_TYPE(self)))
#include "clinic/blob.c.h"
#undef clinic_state

#define _pysqlite_Blob_CAST(op) ((pysqlite_Blob *)(op))

/*[clinic input]
value_emission
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=eb7b17088bfd7215]*/

/*[clinic input]
module _sqlite3
class _sqlite3.Blob "pysqlite_Blob *" "clinic_state()->BlobType"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=908d3e16a45f8da7]*/

static void
close_blob(pysqlite_Blob *self)
{
    if (self->blob) {
        sqlite3_blob *blob = self->blob;
        self->blob = NULL;

        Py_BEGIN_ALLOW_THREADS
        sqlite3_blob_close(blob);
        Py_END_ALLOW_THREADS
    }
}

static int
blob_traverse(PyObject *op, visitproc visit, void *arg)
{
    pysqlite_Blob *self = _pysqlite_Blob_CAST(op);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->connection);
    return 0;
}

static int
blob_clear(PyObject *op)
{
    pysqlite_Blob *self = _pysqlite_Blob_CAST(op);
    Py_CLEAR(self->connection);
    return 0;
}

static void
blob_dealloc(PyObject *op)
{
    pysqlite_Blob *self = _pysqlite_Blob_CAST(op);
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);

    close_blob(self);

    FT_CLEAR_WEAKREFS(op, self->in_weakreflist);
    (void)tp->tp_clear(op);
    tp->tp_free(self);
    Py_DECREF(tp);
}

// Return 1 if the blob object is usable, 0 if not.
static int
check_blob(pysqlite_Blob *self)
{
    if (!pysqlite_check_connection(self->connection) ||
        !pysqlite_check_thread(self->connection)) {
        return 0;
    }
    if (self->blob == NULL) {
        pysqlite_state *state = self->connection->state;
        PyErr_SetString(state->ProgrammingError,
                        "Cannot operate on a closed blob.");
        return 0;
    }
    return 1;
}


/*[clinic input]
_sqlite3.Blob.close as blob_close

Close the blob.
[clinic start generated code]*/

static PyValue
blob_close_impl(pysqlite_Blob *self)
/*[clinic end generated code: output=dd13b4a32c5f5879 input=7bc178a402a40bd8]*/
{
    if (!pysqlite_check_connection(self->connection) ||
        !pysqlite_check_thread(self->connection))
    {
        return PyValue_NULL;
    }
    close_blob(self);
    return PyValue_None;
};

void
pysqlite_close_all_blobs(pysqlite_Connection *self)
{
    for (Py_ssize_t i = 0; i < PyList_GET_SIZE(self->blobs); i++) {
        PyValue weakref = PyList_GetValue(self->blobs, i);
        if (PyValue_IsNull(weakref)) {
            PyErr_Clear();
            continue;
        }
        PyObject *blob;
        /* the list holds only weakref objects */
        int alive = PyWeakref_GetRef(PyValue_AsKnownObjectBorrow(weakref), &blob);
        PyValue_CLOSE(weakref);
        if (!alive) {
            continue;
        }
        close_blob((pysqlite_Blob *)blob);
        Py_DECREF(blob);
    }
}

static void
blob_seterror(pysqlite_Blob *self, int rc)
{
    assert(self->connection != NULL);
    assert(rc != SQLITE_OK);
    set_error_from_code(self->connection->state, rc);
    assert(PyErr_Occurred());
}

static PyObject *
read_single(pysqlite_Blob *self, Py_ssize_t offset)
{
    unsigned char buf = 0;
    int rc;
    Py_BEGIN_ALLOW_THREADS
    rc = sqlite3_blob_read(self->blob, (void *)&buf, 1, (int)offset);
    Py_END_ALLOW_THREADS

    if (rc != SQLITE_OK) {
        blob_seterror(self, rc);
        return NULL;
    }
    return PyLong_FromUnsignedLong((unsigned long)buf);
}

static int
inner_read(pysqlite_Blob *self, char *buf, Py_ssize_t length,
           Py_ssize_t offset)
{
    assert(length <= sqlite3_blob_bytes(self->blob));
    assert(offset < sqlite3_blob_bytes(self->blob));

    int rc;
    Py_BEGIN_ALLOW_THREADS
    rc = sqlite3_blob_read(self->blob, buf, (int)length, (int)offset);
    Py_END_ALLOW_THREADS

    if (rc != SQLITE_OK) {
        blob_seterror(self, rc);
        return -1;
    }
    return 0;
}

static PyObject *
read_multiple(pysqlite_Blob *self, Py_ssize_t length, Py_ssize_t offset)
{
    PyBytesWriter *writer = PyBytesWriter_Create(length);
    if (writer == NULL) {
        return NULL;
    }

    if (inner_read(self, PyBytesWriter_GetData(writer), length, offset) < 0) {
        PyBytesWriter_Discard(writer);
        return NULL;
    }
    return PyBytesWriter_Finish(writer);
}


/*[clinic input]
_sqlite3.Blob.read as blob_read

    length: int = -1
        Read length in bytes.
    /

Read data at the current offset position.

If the end of the blob is reached, the data up to end of file will
be returned.  When length is not specified, or is negative,
Blob.read() will read until the end of the blob.
[clinic start generated code]*/

static PyValue
blob_read_impl(pysqlite_Blob *self, int length)
/*[clinic end generated code: output=202cbab71f35c6a0 input=6b745ad37720e556]*/
{
    if (!check_blob(self)) {
        return PyValue_NULL;
    }

    /* Make sure we never read past "EOB". Also read the rest of the blob if a
     * negative length is specified. */
    int blob_len = sqlite3_blob_bytes(self->blob);
    int max_read_len = blob_len - self->offset;
    if (length < 0 || length > max_read_len) {
        length = max_read_len;
    }

    assert(length >= 0);
    if (length == 0) {
        return _PyValue_FromObjectResult(Py_GetConstant(Py_CONSTANT_EMPTY_BYTES));
    }

    PyObject *buffer = read_multiple(self, length, self->offset);
    if (buffer == NULL) {
        return PyValue_NULL;
    }
    self->offset += length;
    return _PyValue_FromObjectResult(buffer);
};

static int
inner_write(pysqlite_Blob *self, const void *buf, Py_ssize_t len,
            Py_ssize_t offset)
{
    Py_ssize_t blob_len = sqlite3_blob_bytes(self->blob);
    Py_ssize_t remaining_len = blob_len - offset;
    if (len > remaining_len) {
        PyErr_SetString(PyExc_ValueError, "data longer than blob length");
        return -1;
    }

    assert(offset <= blob_len);
    int rc;
    Py_BEGIN_ALLOW_THREADS
    rc = sqlite3_blob_write(self->blob, buf, (int)len, (int)offset);
    Py_END_ALLOW_THREADS

    if (rc != SQLITE_OK) {
        blob_seterror(self, rc);
        return -1;
    }
    return 0;
}


/*[clinic input]
_sqlite3.Blob.write as blob_write

    data: Py_buffer
    /

Write data at the current offset.

This function cannot change the blob length.  Writing beyond the end
of the blob will result in an exception being raised.
[clinic start generated code]*/

static PyValue
blob_write_impl(pysqlite_Blob *self, Py_buffer *data)
/*[clinic end generated code: output=4516ebf6aa6dbf0e input=0d372cb0240a5d49]*/
{
    if (!check_blob(self)) {
        return PyValue_NULL;
    }

    int rc = inner_write(self, data->buf, data->len, self->offset);
    if (rc < 0) {
        return PyValue_NULL;
    }
    self->offset += (int)data->len;
    return PyValue_None;
}


/*[clinic input]
_sqlite3.Blob.seek as blob_seek

    offset: int
    origin: int = 0
    /

Set the current access position to offset.

The origin argument defaults to os.SEEK_SET (absolute blob
positioning).  Other values for origin are os.SEEK_CUR (seek
relative to the current position) and os.SEEK_END (seek relative to
the blob's end).
[clinic start generated code]*/

static PyValue
blob_seek_impl(pysqlite_Blob *self, int offset, int origin)
/*[clinic end generated code: output=508a7aa4a9c675a7 input=84aea1b6b48607dd]*/
{
    if (!check_blob(self)) {
        return PyValue_NULL;
    }

    int blob_len = sqlite3_blob_bytes(self->blob);
    switch (origin) {
        case SEEK_SET:
            break;
        case SEEK_CUR:
            if (offset > INT_MAX - self->offset) {
                goto overflow;
            }
            offset += self->offset;
            break;
        case SEEK_END:
            if (offset > INT_MAX - blob_len) {
                goto overflow;
            }
            offset += blob_len;
            break;
        default:
            PyErr_SetString(PyExc_ValueError,
                            "'origin' should be os.SEEK_SET, os.SEEK_CUR, or "
                            "os.SEEK_END");
            return PyValue_NULL;
    }

    if (offset < 0 || offset > blob_len) {
        PyErr_SetString(PyExc_ValueError, "offset out of blob range");
        return PyValue_NULL;
    }

    self->offset = offset;
    return PyValue_None;

overflow:
    PyErr_SetString(PyExc_OverflowError, "seek offset results in overflow");
    return PyValue_NULL;
}


/*[clinic input]
_sqlite3.Blob.tell as blob_tell

Return the current access position for the blob.
[clinic start generated code]*/

static PyValue
blob_tell_impl(pysqlite_Blob *self)
/*[clinic end generated code: output=b71af175191fe9d8 input=7e34057aa303612c]*/
{
    if (!check_blob(self)) {
        return PyValue_NULL;
    }
    return _PyValue_FromObjectResult(PyLong_FromLong(self->offset));
}


/*[clinic input]
_sqlite3.Blob.__enter__ as blob_enter

Blob context manager enter.
[clinic start generated code]*/

static PyValue
blob_enter_impl(pysqlite_Blob *self)
/*[clinic end generated code: output=af81346a33c318fd input=fe4842c3c582d5a7]*/
{
    if (!check_blob(self)) {
        return PyValue_NULL;
    }
    return _PyValue_FromObjectResult(Py_NewRef(self));
}


/*[clinic input]
_sqlite3.Blob.__exit__ as blob_exit

    type: object
    val: object
    tb: object
    /

Blob context manager exit.
[clinic start generated code]*/

static PyValue
blob_exit_impl(pysqlite_Blob *self, PyValueBorrow type, PyValueBorrow val,
               PyValueBorrow tb)
/*[clinic end generated code: output=3977951f385c77d3 input=575d9ecea205f35f]*/
{
    if (!check_blob(self)) {
        return PyValue_NULL;
    }
    close_blob(self);
    return PyValue_False;
}

static Py_ssize_t
blob_length(PyObject *op)
{
    pysqlite_Blob *self = _pysqlite_Blob_CAST(op);
    if (!check_blob(self)) {
        return -1;
    }
    return sqlite3_blob_bytes(self->blob);
};

static Py_ssize_t
get_subscript_index(pysqlite_Blob *self, PyObject *item)
{
    Py_ssize_t i = _PyValue_AsSsize_t(PyValue_FromObjectBorrow(item),
                                      PyExc_IndexError);
    if (i == -1 && PyErr_Occurred()) {
        return -1;
    }
    int blob_len = sqlite3_blob_bytes(self->blob);
    if (i < 0) {
        i += blob_len;
    }
    if (i < 0 || i >= blob_len) {
        PyErr_SetString(PyExc_IndexError, "Blob index out of range");
        return -1;
    }
    return i;
}

static PyObject *
subscript_index(pysqlite_Blob *self, PyObject *item)
{
    Py_ssize_t i = get_subscript_index(self, item);
    if (i < 0) {
        return NULL;
    }
    return read_single(self, i);
}

static int
get_slice_info(pysqlite_Blob *self, PyObject *item, Py_ssize_t *start,
               Py_ssize_t *stop, Py_ssize_t *step, Py_ssize_t *slicelen)
{
    if (PySlice_Unpack(item, start, stop, step) < 0) {
        return -1;
    }
    int len = sqlite3_blob_bytes(self->blob);
    *slicelen = PySlice_AdjustIndices(len, start, stop, *step);
    return 0;
}

static PyObject *
subscript_slice(pysqlite_Blob *self, PyObject *item)
{
    Py_ssize_t start, stop, step, len;
    if (get_slice_info(self, item, &start, &stop, &step, &len) < 0) {
        return NULL;
    }

    if (len == 0) {
        return Py_GetConstant(Py_CONSTANT_EMPTY_BYTES);
    }

    if (step == 1) {
        return read_multiple(self, len, start);
    }

    // Compute the contiguous blob region covering all slice elements, then
    // copy each element using the standard size_t-cursor pattern that handles
    // both positive and negative steps via unsigned arithmetic.
    Py_ssize_t last = start + (len - 1) * step;
    Py_ssize_t read_offset = Py_MIN(start, last);
    Py_ssize_t read_length = Py_ABS(start - last) + 1;

    PyObject *blob = read_multiple(self, read_length, read_offset);
    if (blob == NULL) {
        return NULL;
    }

    PyBytesWriter *writer = PyBytesWriter_Create(len);
    if (writer == NULL) {
        Py_DECREF(blob);
        return NULL;
    }
    char *res_buf = PyBytesWriter_GetData(writer);
    const char *blob_buf = PyBytes_AS_STRING(blob);

    size_t cur;
    Py_ssize_t i;
    for (cur = (size_t)start, i = 0; i < len; cur += (size_t)step, i++) {
        res_buf[i] = blob_buf[(Py_ssize_t)cur - read_offset];
    }
    Py_DECREF(blob);
    return PyBytesWriter_Finish(writer);
}

static PyObject *
blob_subscript(PyObject *op, PyObject *item)
{
    pysqlite_Blob *self = _pysqlite_Blob_CAST(op);
    if (!check_blob(self)) {
        return NULL;
    }

    if (PyIndex_Check(item)) {
        return subscript_index(self, item);
    }
    if (PySlice_Check(item)) {
        return subscript_slice(self, item);
    }

    PyErr_SetString(PyExc_TypeError, "Blob indices must be integers");
    return NULL;
}

static int
ass_subscript_index(pysqlite_Blob *self, PyObject *item, PyObject *value)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "Blob doesn't support item deletion");
        return -1;
    }
    if (!PyLong_Check(value)) {
        PyErr_Format(PyExc_TypeError,
                     "'%s' object cannot be interpreted as an integer",
                     Py_TYPE(value)->tp_name);
        return -1;
    }
    Py_ssize_t i = get_subscript_index(self, item);
    if (i < 0) {
        return -1;
    }

    long val = _PyValue_AsLong(PyValue_FromObjectBorrow(value));
    if (val == -1 && PyErr_Occurred()) {
        PyErr_Clear();
        val = -1;
    }
    if (val < 0 || val > 255) {
        PyErr_SetString(PyExc_ValueError, "byte must be in range(0, 256)");
        return -1;
    }
    // Downcast to avoid endianness problems.
    unsigned char byte = (unsigned char)val;
    return inner_write(self, (const void *)&byte, 1, i);
}

static int
ass_subscript_slice(pysqlite_Blob *self, PyObject *item, PyObject *value)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "Blob doesn't support slice deletion");
        return -1;
    }

    Py_ssize_t start, stop, step, len;
    if (get_slice_info(self, item, &start, &stop, &step, &len) < 0) {
        return -1;
    }

    Py_buffer vbuf;
    if (PyObject_GetBuffer(value, &vbuf, PyBUF_SIMPLE) < 0) {
        return -1;
    }

    if (vbuf.len != len) {
        PyErr_SetString(PyExc_IndexError,
                        "Blob slice assignment is wrong size");
        PyBuffer_Release(&vbuf);
        return -1;
    }

    if (len == 0) {
        PyBuffer_Release(&vbuf);
        return 0;
    }

    int rc = -1;
    if (step == 1) {
        rc = inner_write(self, vbuf.buf, len, start);
    }
    else {
        /* Compute the contiguous blob region covering all slice elements,
           read it, patch each element and write it back.  The object
           returned by read_multiple() cannot be used as the buffer, because
           for a single byte it is an immortal singleton. */
        Py_ssize_t last = start + (len - 1) * step;
        Py_ssize_t write_offset = Py_MIN(start, last);
        Py_ssize_t write_length = Py_ABS(start - last) + 1;
        char *buf = PyMem_Malloc(write_length);
        if (buf == NULL) {
            PyErr_NoMemory();
        }
        else {
            if (inner_read(self, buf, write_length, write_offset) == 0) {
                /* The size_t cursor handles both positive and negative steps
                   via unsigned arithmetic. */
                size_t cur;
                Py_ssize_t i;
                for (cur = (size_t)start, i = 0; i < len;
                     cur += (size_t)step, i++) {
                    buf[(Py_ssize_t)cur - write_offset] =
                        ((char *)vbuf.buf)[i];
                }
                rc = inner_write(self, buf, write_length, write_offset);
            }
            PyMem_Free(buf);
        }
    }
    PyBuffer_Release(&vbuf);
    return rc;
}

static int
blob_ass_subscript(PyObject *op, PyObject *item, PyObject *value)
{
    pysqlite_Blob *self = _pysqlite_Blob_CAST(op);
    if (!check_blob(self)) {
        return -1;
    }

    if (PyIndex_Check(item)) {
        return ass_subscript_index(self, item, value);
    }
    if (PySlice_Check(item)) {
        return ass_subscript_slice(self, item, value);
    }

    PyErr_SetString(PyExc_TypeError, "Blob indices must be integers");
    return -1;
}


static PyMethodDef blob_methods[] = {
    BLOB_CLOSE_METHODDEF
    BLOB_ENTER_METHODDEF
    BLOB_EXIT_METHODDEF
    BLOB_READ_METHODDEF
    BLOB_SEEK_METHODDEF
    BLOB_TELL_METHODDEF
    BLOB_WRITE_METHODDEF
    {NULL, NULL}
};

static struct PyMemberDef blob_members[] = {
    {"__weaklistoffset__", Py_T_PYSSIZET, offsetof(pysqlite_Blob, in_weakreflist), Py_READONLY},
    {NULL},
};

static PyType_Slot blob_slots[] = {
    {Py_tp_dealloc, blob_dealloc},
    {Py_tp_traverse, blob_traverse},
    {Py_tp_clear, blob_clear},
    {Py_tp_methods, blob_methods},
    {Py_tp_members, blob_members},

    // Mapping protocol
    {Py_mp_length, blob_length},
    {Py_mp_subscript, blob_subscript},
    {Py_mp_ass_subscript, blob_ass_subscript},
    {0, NULL},
};

static PyType_Spec blob_spec = {
    .name = MODULE_NAME ".Blob",
    .basicsize = sizeof(pysqlite_Blob),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_DISALLOW_INSTANTIATION),
    .slots = blob_slots,
};

int
pysqlite_blob_setup_types(PyObject *mod)
{
    PyObject *type = PyType_FromModuleAndSpec(mod, &blob_spec, NULL);
    if (type == NULL) {
        return -1;
    }
    pysqlite_state *state = pysqlite_get_state(mod);
    state->BlobType = (PyTypeObject *)type;
    return 0;
}
