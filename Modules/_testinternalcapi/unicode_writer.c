#include "parts.h"
#include "../_testcapi/util.h"
#include "pycore_unicodeobject.h"

// --- PyUnicodeWriter type -------------------------------------------------

typedef struct {
    PyObject_HEAD
    PyUnicodeWriter *writer;
} WriterObject;


static PyObject *
writer_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    WriterObject *self = (WriterObject *)type->tp_alloc(type, 0);
    if (!self) {
        return NULL;
    }
    self->writer = NULL;
    return (PyObject*)self;
}


static int
writer_init(PyObject *self_raw, PyObject *args, PyObject *kwargs)
{
    WriterObject *self = (WriterObject *)self_raw;

    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "n", &size)) {
        return -1;
    }

    if (self->writer) {
        PyUnicodeWriter_Discard(self->writer);
    }

    self->writer = PyUnicodeWriter_Create(size);
    if (self->writer == NULL) {
        return -1;
    }
    return 0;
}


static void
writer_dealloc(PyObject *self_raw)
{
    WriterObject *self = (WriterObject *)self_raw;
    PyTypeObject *tp = Py_TYPE(self);
    if (self->writer) {
        PyUnicodeWriter_Discard(self->writer);
    }
    tp->tp_free(self);
    Py_DECREF(tp);
}


static inline int
writer_check(WriterObject *self)
{
    if (self->writer == NULL) {
        PyErr_SetString(PyExc_ValueError, "operation on finished writer");
        return -1;
    }
    return 0;
}


static PyObject*
writer_write_char(PyObject *self_raw, PyObject *args)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    unsigned int ch;
    if (!PyArg_ParseTuple(args, "I", &ch)) {
        return NULL;
    }

    if (PyUnicodeWriter_WriteChar(self->writer, (Py_UCS4)ch) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject*
writer_write_utf8(PyObject *self_raw, PyObject *args)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    const char *str;
    Py_ssize_t bsize, size;
    if (!PyArg_ParseTuple(args, "z#n", &str, &bsize, &size)) {
        return NULL;
    }

    if (PyUnicodeWriter_WriteUTF8(self->writer, str, size) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject*
writer_write_ascii(PyObject *self_raw, PyObject *args)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    const char *str;
    Py_ssize_t bsize, size;
    if (!PyArg_ParseTuple(args, "z#n", &str, &bsize, &size)) {
        return NULL;
    }

    if (PyUnicodeWriter_WriteASCII(self->writer, str, size) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject*
writer_write_widechar(PyObject *self_raw, PyObject *args)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    const char *s;
    Py_ssize_t bsize;
    Py_ssize_t size = -100;

    if (!PyArg_ParseTuple(args, "z#|n", &s, &bsize, &size)) {
        return NULL;
    }
    if (size == -100) {
        if (bsize % SIZEOF_WCHAR_T) {
            PyErr_SetString(PyExc_AssertionError,
                            "invalid size in writer.write_widechar()");
            return NULL;
        }
        size = bsize / SIZEOF_WCHAR_T;
    }

    int res = PyUnicodeWriter_WriteWideChar(self->writer, (const wchar_t *)s, size);
    if (res < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject*
writer_write_ucs4(PyObject *self_raw, PyObject *args)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    const char *s;
    Py_ssize_t bsize;
    Py_ssize_t size = -100;

    if (!PyArg_ParseTuple(args, "z#|n", &s, &bsize, &size)) {
        return NULL;
    }
    if (size == -100) {
        if (bsize % sizeof(Py_UCS4)) {
            PyErr_SetString(PyExc_AssertionError,
                            "invalid size in writer.write_ucs4()");
            return NULL;
        }
        size = bsize / sizeof(Py_UCS4);
    }

    int res = PyUnicodeWriter_WriteUCS4(self->writer, (const Py_UCS4 *)s, size);
    if (res < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyValue
writer_write_str(PyObject *self_raw, PyValue obj_value)
{
    PyObject *obj = PyValue_Object(obj_value);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return PyValue_NULL;
    }

    NULLABLE(obj);
    if (PyUnicodeWriter_WriteStr(self->writer, obj) < 0) {
        return PyValue_NULL;
    }
    return PyValue_FromObject(Py_None);
}


static PyValue
writer_write_repr(PyObject *self_raw, PyValue obj_value)
{
    PyObject *obj = PyValue_Object(obj_value);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return PyValue_NULL;
    }

    NULLABLE(obj);
    if (PyUnicodeWriter_WriteRepr(self->writer, obj) < 0) {
        return PyValue_NULL;
    }
    return PyValue_FromObject(Py_None);
}


// write_repr(None) calls PyUnicodeWriter_WriteRepr(writer, NULL), so add
// a specialized function calling PyUnicodeWriter_WriteRepr(writer, Py_None).
static PyObject*
writer_write_repr_none(PyObject *self_raw, PyObject *Py_UNUSED(arg))
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    if (PyUnicodeWriter_WriteRepr(self->writer, Py_None) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject*
writer_write_substring(PyObject *self_raw, PyObject *args)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    PyObject *str;
    Py_ssize_t start, end;
    if (!PyArg_ParseTuple(args, "Onn", &str, &start, &end)) {
        return NULL;
    }
    NULLABLE(str);

    if (PyUnicodeWriter_WriteSubstring(self->writer, str, start, end) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject*
writer_write_latin1(PyObject *self_raw, PyObject *args)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    const char *str;
    Py_ssize_t bsize, size;
    if (!PyArg_ParseTuple(args, "z#n", &str, &bsize, &size)) {
        return NULL;
    }

    _PyUnicodeWriter *writer = (_PyUnicodeWriter*)self->writer;
    if (_PyUnicodeWriter_WriteLatin1String(writer, str, size) < 0) {
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject*
writer_decodeutf8stateful(PyObject *self_raw, PyObject *args)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    const char *str;
    Py_ssize_t bsize, len;
    const char *errors;
    int use_consumed = 0;
    if (!PyArg_ParseTuple(args, "z#nz#|p", &str, &bsize, &len, &errors, &bsize, &use_consumed)) {
        return NULL;
    }

    Py_ssize_t consumed = 12345;
    Py_ssize_t *pconsumed = use_consumed ? &consumed : NULL;
    if (PyUnicodeWriter_DecodeUTF8Stateful(self->writer, str, len,
                                           errors, pconsumed) < 0) {
        if (use_consumed) {
            assert(consumed == 0);
        }
        return NULL;
    }

    if (use_consumed) {
        return PyLong_FromSsize_t(consumed);
    }
    Py_RETURN_NONE;
}


static PyObject*
writer_get_pointer(PyObject *self_raw, PyObject *args)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    return PyLong_FromVoidPtr(self->writer);
}


static PyObject*
writer_get_buffer(PyObject *self_raw, PyObject *args)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    _PyUnicodeWriter *writer = (_PyUnicodeWriter*)self->writer;
    PyObject *allocated;
    Py_UCS4 maxchar;
    if (writer->buffer) {
        allocated = PyLong_FromSsize_t(PyUnicode_GET_LENGTH(writer->buffer));
        maxchar = PyUnicode_MAX_CHAR_VALUE(writer->buffer);
    }
    else {
        allocated = Py_None;
        maxchar = writer->min_char;
    }
    return Py_BuildValue("(NkN)",
                         allocated, (unsigned long)maxchar,
                         PyBool_FromLong(writer->readonly));
}


static PyObject*
writer_finish(PyObject *self_raw, PyObject *Py_UNUSED(args))
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    PyObject *str = PyUnicodeWriter_Finish(self->writer);
    self->writer = NULL;
    return str;
}


static PyObject*
writer_finish_with_size(PyObject *self_raw, PyObject *args)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "n", &size)) {
        return NULL;
    }

    _PyUnicodeWriter *writer = (_PyUnicodeWriter*)self->writer;
    PyObject *str = _PyUnicodeWriter_FinishWithSize(writer, size);
    PyUnicodeWriter_Discard(self->writer);
    self->writer = NULL;
    return str;
}


static PyMethodDef writer_methods[] = {
    {"write_char", _PyCFunction_CAST(writer_write_char), METH_VARARGS},
    {"write_utf8", _PyCFunction_CAST(writer_write_utf8), METH_VARARGS},
    {"write_ascii", _PyCFunction_CAST(writer_write_ascii), METH_VARARGS},
    {"write_widechar", _PyCFunction_CAST(writer_write_widechar), METH_VARARGS},
    {"write_ucs4", _PyCFunction_CAST(writer_write_ucs4), METH_VARARGS},
    {"write_str", _PyCFunction_CAST(writer_write_str), METH_O},
    {"write_repr", _PyCFunction_CAST(writer_write_repr), METH_O},
    {"write_repr_none", _PyCFunction_CAST(writer_write_repr_none), METH_NOARGS},
    {"write_substring", _PyCFunction_CAST(writer_write_substring), METH_VARARGS},
    {"write_latin1", _PyCFunction_CAST(writer_write_latin1), METH_VARARGS},
    {"decodeutf8stateful", _PyCFunction_CAST(writer_decodeutf8stateful), METH_VARARGS},
    {"get_pointer", _PyCFunction_CAST(writer_get_pointer), METH_VARARGS},
    {"get_buffer", _PyCFunction_CAST(writer_get_buffer), METH_VARARGS},
    {"finish", _PyCFunction_CAST(writer_finish), METH_NOARGS},
    {"finish_with_size", _PyCFunction_CAST(writer_finish_with_size), METH_VARARGS},
    {NULL,              NULL}           /* sentinel */
};

static PyType_Slot Writer_Type_slots[] = {
    {Py_tp_new, writer_new},
    {Py_tp_init, writer_init},
    {Py_tp_dealloc, writer_dealloc},
    {Py_tp_methods, writer_methods},
    {0, 0},  /* sentinel */
};

static PyType_Spec Writer_spec = {
    .name = "_testinternalcapi.PyUnicodeWriter",
    .basicsize = sizeof(WriterObject),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = Writer_Type_slots,
};


static PyObject *
test_unicode_writer_lifecycle(PyObject *Py_UNUSED(self), PyObject *Py_UNUSED(unused))
{
    _PyUnicodeWriter writer;
    _PyUnicodeWriter_Init(&writer);
    assert(writer.root_active && writer.buffer == NULL);
    _PyUnicodeWriter_Dealloc(&writer);
    assert(!writer.root_active && writer.buffer == NULL);
    _PyUnicodeWriter_Dealloc(&writer);

    PyObject *buffer = PyUnicode_New(4, 0x10ffff);
    if (buffer == NULL) {
        return NULL;
    }
    for (Py_ssize_t i = 0; i < 4; i++) {
        PyUnicode_WRITE(PyUnicode_KIND(buffer), PyUnicode_DATA(buffer),
                        i, 'a' + i);
    }
    _PyUnicodeWriter_InitWithBuffer(&writer, buffer);
    assert(writer.root_active && writer.buffer == buffer);
    writer.pos = 4;
    // Finish must keep the wide buffer rooted while allocating its narrower copy.
    PyObject *result = _PyUnicodeWriter_FinishWithSize(&writer, 2);
    assert(!writer.root_active && writer.buffer == NULL);
    _PyUnicodeWriter_Dealloc(&writer);
    if (result == NULL) {
        return NULL;
    }
    assert(PyUnicode_CompareWithASCIIString(result, "ab") == 0);
    Py_DECREF(result);

    _PyUnicodeWriter_Init(&writer);
    if (_PyUnicodeWriter_WriteASCIIString(&writer, "reused", 6) < 0) {
        _PyUnicodeWriter_Dealloc(&writer);
        return NULL;
    }
    result = _PyUnicodeWriter_Finish(&writer);
    assert(!writer.root_active && writer.buffer == NULL);
    if (result == NULL) {
        return NULL;
    }
    assert(PyUnicode_CompareWithASCIIString(result, "reused") == 0);
    Py_DECREF(result);

    _PyUnicodeWriter_Init(&writer);
    result = _PyUnicodeWriter_Finish(&writer);
    assert(!writer.root_active && writer.buffer == NULL);
    assert(result != NULL && PyUnicode_GET_LENGTH(result) == 0);
    Py_DECREF(result);
    _PyUnicodeWriter_Dealloc(&writer);
    Py_RETURN_NONE;
}

static PyMethodDef test_methods[] = {
    {"test_unicode_writer_lifecycle", test_unicode_writer_lifecycle, METH_NOARGS},
    {NULL},
};

int
_PyTestInternalCapi_Init_UnicodeWriter(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }
    PyTypeObject *writer_type = (PyTypeObject *)PyType_FromSpec(&Writer_spec);
    if (writer_type == NULL) {
        return -1;
    }
    if (PyModule_AddType(m, writer_type) < 0) {
        Py_DECREF(writer_type);
        return -1;
    }
    Py_DECREF(writer_type);

    return 0;
}
