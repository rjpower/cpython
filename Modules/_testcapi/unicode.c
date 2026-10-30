#include <stddef.h>               // ptrdiff_t

#include "parts.h"
#include "util.h"

/* Test PyUnicode_New() */
static PyObject *
unicode_new(PyObject *self, PyObject *args)
{
    Py_ssize_t size;
    unsigned int maxchar;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "nI", &size, &maxchar)) {
        return NULL;
    }

    result = PyUnicode_New(size, (Py_UCS4)maxchar);
    if (!result) {
        return NULL;
    }
    if (size > 0 && maxchar <= 0x10ffff &&
        PyUnicode_Fill(result, 0, size, (Py_UCS4)maxchar) < 0)
    {
        Py_DECREF(result);
        return NULL;
    }
    return result;
}


static PyObject *
unicode_copy(PyObject *unicode)
{
    PyObject *copy;

    if (!unicode) {
        return NULL;
    }
    if (!PyUnicode_Check(unicode)) {
        Py_INCREF(unicode);
        return unicode;
    }

    copy = PyUnicode_New(PyUnicode_GET_LENGTH(unicode),
                         PyUnicode_MAX_CHAR_VALUE(unicode));
    if (!copy) {
        return NULL;
    }
    if (PyUnicode_CopyCharacters(copy, 0, unicode,
                                 0, PyUnicode_GET_LENGTH(unicode)) < 0)
    {
        Py_DECREF(copy);
        return NULL;
    }
    return copy;
}


/* Test PyUnicode_Fill() */
static PyObject *
unicode_fill(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"to", "start", "length", "fill_char",
                             "incref", NULL};
    PyObject *to, *to_copy;
    Py_ssize_t start, length, filled;
    unsigned int fill_char;
    int incref = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "OnnI|p", kwlist,
                                     &to, &start, &length,
                                     &fill_char, &incref))
    {
        return NULL;
    }

    NULLABLE(to);
    if (!(to_copy = unicode_copy(to)) && to) {
        return NULL;
    }

    if (incref) {
        Py_INCREF(to_copy);
    }
    filled = PyUnicode_Fill(to_copy, start, length, (Py_UCS4)fill_char);
    if (incref) {
        Py_DECREF(to_copy);
    }

    if (filled == -1 && PyErr_Occurred()) {
        Py_DECREF(to_copy);
        return NULL;
    }
    return Py_BuildValue("(Nn)", to_copy, filled);
}


/* Test PyUnicode_FromKindAndData() */
static PyObject *
unicode_fromkindanddata(PyObject *self, PyObject *args)
{
    int kind;
    void *buffer;
    Py_ssize_t bsize;
    Py_ssize_t size = -100;

    if (!PyArg_ParseTuple(args, "iz#|n", &kind, &buffer, &bsize, &size)) {
        return NULL;
    }

    if (size == -100) {
        size = bsize;
    }
    if (kind && size % kind) {
        PyErr_SetString(PyExc_AssertionError,
                        "invalid size in unicode_fromkindanddata()");
        return NULL;
    }
    return PyUnicode_FromKindAndData(kind, buffer, kind ? size / kind : 0);
}


// Test PyUnicode_AsUCS4().
// Part of the limited C API, but the test needs PyUnicode_FromKindAndData().
static PyObject *
unicode_asucs4(PyObject *self, PyObject *args)
{
    PyObject *unicode, *result;
    Py_UCS4 *buffer;
    int copy_null;
    Py_ssize_t str_len, buf_len;

    if (!PyArg_ParseTuple(args, "Onp:unicode_asucs4", &unicode, &str_len, &copy_null)) {
        return NULL;
    }

    NULLABLE(unicode);
    buf_len = str_len + 1;
    buffer = PyMem_NEW(Py_UCS4, buf_len);
    if (buffer == NULL) {
        return PyErr_NoMemory();
    }
    memset(buffer, 0, sizeof(Py_UCS4)*buf_len);
    buffer[str_len] = 0xffffU;

    if (!PyUnicode_AsUCS4(unicode, buffer, buf_len, copy_null)) {
        PyMem_Free(buffer);
        return NULL;
    }

    result = PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND, buffer, buf_len);
    PyMem_Free(buffer);
    return result;
}


// Test PyUnicode_AsUCS4Copy().
// Part of the limited C API, but the test needs PyUnicode_FromKindAndData().
static PyObject *
unicode_asucs4copy(PyObject *self, PyObject *args)
{
    PyObject *unicode;
    Py_UCS4 *buffer;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "O", &unicode)) {
        return NULL;
    }

    NULLABLE(unicode);
    buffer = PyUnicode_AsUCS4Copy(unicode);
    if (buffer == NULL) {
        return NULL;
    }
    result = PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND,
                                       buffer,
                                       PyUnicode_GET_LENGTH(unicode) + 1);
    PyMem_FREE(buffer);
    return result;
}


/* Test PyUnicode_AsUTF8() */
static PyObject *
unicode_asutf8(PyObject *self, PyObject *args)
{
    PyObject *unicode;
    Py_ssize_t buflen;
    const char *s;

    if (!PyArg_ParseTuple(args, "On", &unicode, &buflen))
        return NULL;

    NULLABLE(unicode);
    s = PyUnicode_AsUTF8(unicode);
    if (s == NULL)
        return NULL;

    return PyBytes_FromStringAndSize(s, buflen);
}


/* Test PyUnicode_CopyCharacters() */
static PyObject *
unicode_copycharacters(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"to", "to_start", "from", "from_start",
                             "howmany", "incref", NULL};
    PyObject *from, *to, *to_copy;
    Py_ssize_t from_start, to_start, how_many, copied;
    int incref = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "UnOnn|p", kwlist,
                                     &to, &to_start, &from, &from_start,
                                     &how_many, &incref)) {
        return NULL;
    }

    NULLABLE(from);
    if (!(to_copy = PyUnicode_New(PyUnicode_GET_LENGTH(to),
                                  PyUnicode_MAX_CHAR_VALUE(to)))) {
        return NULL;
    }
    if (PyUnicode_Fill(to_copy, 0, PyUnicode_GET_LENGTH(to_copy), 0U) < 0) {
        Py_DECREF(to_copy);
        return NULL;
    }

    if (incref) {
        Py_INCREF(to_copy);
    }
    copied = PyUnicode_CopyCharacters(to_copy, to_start, from,
                                      from_start, how_many);
    if (incref) {
        Py_DECREF(to_copy);
    }

    if (copied == -1 && PyErr_Occurred()) {
        Py_DECREF(to_copy);
        return NULL;
    }

    return Py_BuildValue("(Nn)", to_copy, copied);
}


// Test PyUnstable_Unicode_GET_CACHED_HASH()
static PyValue
unicode_GET_CACHED_HASH(PyObject *self, PyValue arg_value)
{
    PyObject *arg = PyValue_Object(arg_value);
    if (arg == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromResult(
        PyLong_FromSsize_t(PyUnstable_Unicode_GET_CACHED_HASH(arg)));
}


// Test the deprecated _Py_Identifier C API:
// - _Py_IDENTIFIER()
// - _Py_static_string()
// - _Py_static_string_init()
// - _PyObject_CallMethodId()
// - _PyObject_GetAttrId()
// - _PyUnicode_FromId()
//
// _testembed also has tests on _PyUnicode_FromId().
static PyObject*
test_py_identifier(PyObject *self, PyObject *Py_UNUSED(args))
{
// Ignore deprecation warnings
_Py_COMP_DIAG_PUSH
_Py_COMP_DIAG_IGNORE_DEPR_DECLS

    _Py_IDENTIFIER(hello);
    PyObject *str = _PyUnicode_FromId(&PyId_hello);  // borrowed ref
    if (str == NULL) {
        return NULL;
    }
    assert(PyUnicode_EqualToUTF8(str, "hello") == 1);

    // Calling twice return the same object
    PyObject *str2 = _PyUnicode_FromId(&PyId_hello);  // borrowed ref
    assert(str2 == str);

    PyObject *number = Py_GetConstant(Py_CONSTANT_ONE);  // immortal
    _Py_static_string(to_bytes_id, "to_bytes");
    PyObject *res = _PyObject_CallMethodId(number, &to_bytes_id, NULL);
    if (res == NULL) {
        return NULL;
    }
    Py_DECREF(res);

    static _Py_Identifier real_id = _Py_static_string_init("real");
    res = _PyObject_GetAttrId(number, &real_id);
    if (res == NULL) {
        return NULL;
    }
    assert(res == number);
    Py_DECREF(res);

    Py_RETURN_NONE;

_Py_COMP_DIAG_POP
}


// Write into an immutable str object to test _PyStaticObjects_CheckAll()
static PyObject *
corrupt_unicode(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj, *override;
    if (!PyArg_ParseTuple(args, "OO", &obj, &override)) {
        return NULL;
    }
    assert(PyUnicode_KIND(obj) == PyUnicode_1BYTE_KIND);
    assert(PyUnicode_KIND(override) == PyUnicode_1BYTE_KIND);

    Py_UCS1 *dst = PyUnicode_1BYTE_DATA(obj);
    Py_UCS1 *src = PyUnicode_1BYTE_DATA(override);
    Py_ssize_t size = PyUnicode_GET_LENGTH(override);
    memcpy(dst, src, size);
    Py_RETURN_NONE;
}


/* Test PyUnicode_READ_CHAR() */
static PyObject *
unicode_read_char(PyObject *self, PyObject *args)
{
    PyObject *unicode;
    Py_ssize_t index;
    if (!PyArg_ParseTuple(args, "On", &unicode, &index)) {
        return NULL;
    }
    NULLABLE(unicode);

    Py_UCS4 result = PyUnicode_READ_CHAR(unicode, index);
    return PyLong_FromUnsignedLong(result);
}


/* Test PyUnicode_READ() macro */
static PyObject *
unicode_read(PyObject *self, PyObject *args)
{
    PyObject *unicode;
    Py_ssize_t index;
    if (!PyArg_ParseTuple(args, "On", &unicode, &index)) {
        return NULL;
    }
    NULLABLE(unicode);

    int kind = PyUnicode_KIND(unicode);
    const void *data = PyUnicode_DATA(unicode);
    Py_UCS4 result = PyUnicode_READ(kind, data, index);
    return PyLong_FromUnsignedLong(result);
}


/* Test PyUnicode_WRITE() macro */
static PyObject *
unicode_write(PyObject *self, PyObject *args)
{
    PyObject *unicode;
    Py_ssize_t index;
    unsigned int character;
    if (!PyArg_ParseTuple(args, "OnI", &unicode, &index, &character)) {
        return NULL;
    }
    NULLABLE(unicode);

    PyObject *copy = unicode_copy(unicode);
    if (copy == NULL) {
        return NULL;
    }

    int kind = PyUnicode_KIND(copy);
    const void *data = PyUnicode_DATA(copy);
    PyUnicode_WRITE(kind, data, index, character);
    // Same return value than _testlimitedcapi unicode_writechar():
    // always use 0 as the function result
    return Py_BuildValue("(Ni)", copy, 0);
}


/* Test PyUnicode_KIND() macro */
static PyValue
unicode_kind(PyObject *self, PyValue unicode_value)
{
    PyObject *unicode = PyValue_Object(unicode_value);
    if (unicode == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(unicode);

    int kind = PyUnicode_KIND(unicode);
    return PyValue_FromResult(PyLong_FromLong(kind));
}


/* Test PyUnicode_MAX_CHAR_VALUE() macro */
static PyValue
unicode_max_char_value(PyObject *self, PyValue unicode_value)
{
    PyObject *unicode = PyValue_Object(unicode_value);
    if (unicode == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(unicode);

    Py_UCS4 maxchar = PyUnicode_MAX_CHAR_VALUE(unicode);
    return PyValue_FromResult(PyLong_FromUnsignedLong(maxchar));
}


/* Test PyUnicode_GET_LENGTH() macro */
static PyValue
unicode_getlength_macro(PyObject *self, PyValue arg_value)
{
    PyObject *arg = PyValue_Object(arg_value);
    if (arg == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(arg);
    return PyValue_FromResult(PyLong_FromSsize_t(PyUnicode_GET_LENGTH(arg)));
}


/* Test PyUnicode_Equal() macro */
static PyObject *
unicode_equal(PyObject *self, PyObject *args)
{
    PyObject *str1, *str2;
    if (!PyArg_ParseTuple(args, "OnI", &str1, &str2)) {
        return NULL;
    }
    NULLABLE(str1);
    NULLABLE(str2);

    RETURN_INT(PyUnicode_Equal(str1, str2));
}


/* Test PyUnicode_CHECK_INTERNED() macro */
static PyValue
unicode_check_interned(PyObject *self, PyValue arg_value)
{
    PyObject *arg = PyValue_Object(arg_value);
    if (arg == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(arg);
    RETURN_UINT_VALUE(PyUnicode_CHECK_INTERNED(arg));
}


/* Test PyUnicode_IS_ASCII() macro */
static PyValue
unicode_is_ascii(PyObject *self, PyValue arg_value)
{
    PyObject *arg = PyValue_Object(arg_value);
    if (arg == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(arg);
    RETURN_UINT_VALUE(PyUnicode_IS_ASCII(arg));
}


/* Test PyUnicode_IS_COMPACT() macro */
static PyValue
unicode_is_compact(PyObject *self, PyValue arg_value)
{
    PyObject *arg = PyValue_Object(arg_value);
    if (arg == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(arg);
    RETURN_UINT_VALUE(PyUnicode_IS_COMPACT(arg));
}


/* Test PyUnicode_IS_COMPACT_ASCII() macro */
static PyValue
unicode_is_compact_ascii(PyObject *self, PyValue arg_value)
{
    PyObject *arg = PyValue_Object(arg_value);
    if (arg == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(arg);
    RETURN_INT_VALUE(PyUnicode_IS_COMPACT_ASCII(arg));
}


static PyObject *
test_unicode_writer(PyObject *Py_UNUSED(self), PyObject *Py_UNUSED(unused))
{
    PyUnicodeWriter *writer = PyUnicodeWriter_Create(0);
    if (writer == NULL) {
        return NULL;
    }
    if (PyUnicodeWriter_WriteUTF8(writer, "opaque API", -1) < 0) {
        PyUnicodeWriter_Discard(writer);
        return NULL;
    }
    PyObject *result = PyUnicodeWriter_Finish(writer);
    if (result == NULL) {
        return NULL;
    }
    assert(PyUnicode_CompareWithASCIIString(result, "opaque API") == 0);
    Py_DECREF(result);

    writer = PyUnicodeWriter_Create(0);
    if (writer == NULL) {
        return NULL;
    }
    PyUnicodeWriter_Discard(writer);
    Py_RETURN_NONE;
}


static PyMethodDef TestMethods[] = {
    {"test_unicode_writer", test_unicode_writer, METH_NOARGS},
    {"unicode_new",              unicode_new,                    METH_VARARGS},
    {"unicode_fill",             _PyCFunction_CAST(unicode_fill), METH_VARARGS | METH_KEYWORDS},
    {"unicode_fromkindanddata",  unicode_fromkindanddata,        METH_VARARGS},
    {"unicode_asucs4",           unicode_asucs4,                 METH_VARARGS},
    {"unicode_asucs4copy",       unicode_asucs4copy,             METH_VARARGS},
    {"unicode_asutf8",           unicode_asutf8,                 METH_VARARGS},
    {"unicode_copycharacters",   _PyCFunction_CAST(unicode_copycharacters), METH_VARARGS | METH_KEYWORDS},
    {"unicode_GET_CACHED_HASH",  _PyCFunction_CAST(unicode_GET_CACHED_HASH), METH_O},
    {"test_py_identifier",       test_py_identifier,             METH_NOARGS},
    {"corrupt_unicode",          corrupt_unicode,                METH_VARARGS},
    {"unicode_read_char",        unicode_read_char,              METH_VARARGS},
    {"unicode_read",             unicode_read,                   METH_VARARGS},
    {"unicode_write",            unicode_write,                  METH_VARARGS},
    {"unicode_kind",             _PyCFunction_CAST(unicode_kind), METH_O},
    {"unicode_max_char_value",   _PyCFunction_CAST(unicode_max_char_value), METH_O},
    {"unicode_getlength_macro",  _PyCFunction_CAST(unicode_getlength_macro), METH_O},
    {"unicode_equal",            unicode_equal,                  METH_VARARGS},
    {"unicode_check_interned",   _PyCFunction_CAST(unicode_check_interned), METH_O},
    {"unicode_is_ascii",         _PyCFunction_CAST(unicode_is_ascii), METH_O},
    {"unicode_is_compact",       _PyCFunction_CAST(unicode_is_compact), METH_O},
    {"unicode_is_compact_ascii", _PyCFunction_CAST(unicode_is_compact_ascii), METH_O},
    {NULL},
};

int
_PyTestCapi_Init_Unicode(PyObject *m) {
    if (PyModule_AddFunctions(m, TestMethods) < 0) {
        return -1;
    }

    return 0;
}
