#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "parts.h"
#include "util.h"
#include "clinic/long.c.h"

/*[clinic input]
module _testcapi
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=6361033e795369fc]*/


/*[clinic input]
_testcapi.call_long_compact_api
    arg: object
    /
[clinic start generated code]*/

static PyObject *
_testcapi_call_long_compact_api_impl(PyObject *module, PyObject *arg)
/*[clinic end generated code: output=a25e5bf0926c09a1 input=87b87396967af14c]*/

{
    assert(PyLong_Check(arg));
    int is_compact = PyUnstable_Long_IsCompact((PyLongObject*)arg);
    Py_ssize_t value = -1;
    if (is_compact) {
        value = PyUnstable_Long_CompactValue((PyLongObject*)arg);
    }
    return Py_BuildValue("in", is_compact, value);
}


static PyObject *
pylong_fromunicodeobject(PyObject *module, PyObject *args)
{
    PyObject *unicode;
    int base;
    if (!PyArg_ParseTuple(args, "Oi", &unicode, &base)) {
        return NULL;
    }

    NULLABLE(unicode);
    return PyLong_FromUnicodeObject(unicode, base);
}


static PyObject *
pylong_asnativebytes(PyObject *module, PyObject *args)
{
    PyObject *v;
    Py_buffer buffer;
    Py_ssize_t n, flags;
    if (!PyArg_ParseTuple(args, "Ow*nn", &v, &buffer, &n, &flags)) {
        return NULL;
    }
    if (buffer.readonly) {
        PyErr_SetString(PyExc_TypeError, "buffer must be writable");
        PyBuffer_Release(&buffer);
        return NULL;
    }
    if (buffer.len < n) {
        PyErr_SetString(PyExc_ValueError, "buffer must be at least 'n' bytes");
        PyBuffer_Release(&buffer);
        return NULL;
    }
    Py_ssize_t res = PyLong_AsNativeBytes(v, buffer.buf, n, (int)flags);
    PyBuffer_Release(&buffer);
    return res >= 0 ? PyLong_FromSsize_t(res) : NULL;
}


static PyObject *
pylong_fromnativebytes(PyObject *module, PyObject *args)
{
    Py_buffer buffer;
    Py_ssize_t n, flags, signed_;
    if (!PyArg_ParseTuple(args, "y*nnn", &buffer, &n, &flags, &signed_)) {
        return NULL;
    }
    if (buffer.len < n) {
        PyErr_SetString(PyExc_ValueError, "buffer must be at least 'n' bytes");
        PyBuffer_Release(&buffer);
        return NULL;
    }
    PyObject *res = signed_
        ? PyLong_FromNativeBytes(buffer.buf, n, (int)flags)
        : PyLong_FromUnsignedNativeBytes(buffer.buf, n, (int)flags);
    PyBuffer_Release(&buffer);
    return res;
}


static PyValue
pylong_getsign(PyObject *module, PyValue arg_value)
{
    PyObject *arg = PyValue_Object(arg_value);
    if (arg == NULL) {
        return PyValue_NULL;
    }
    int sign;
    NULLABLE(arg);
    if (PyLong_GetSign(arg, &sign) == -1) {
        return PyValue_NULL;
    }
    return PyValue_FromResult(PyLong_FromLong(sign));
}


static PyValue
pylong_ispositive(PyObject *module, PyValue arg_value)
{
    PyObject *arg = PyValue_Object(arg_value);
    if (arg == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(arg);
    RETURN_INT_VALUE(PyLong_IsPositive(arg));
}


static PyValue
pylong_isnegative(PyObject *module, PyValue arg_value)
{
    PyObject *arg = PyValue_Object(arg_value);
    if (arg == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(arg);
    RETURN_INT_VALUE(PyLong_IsNegative(arg));
}


static PyValue
pylong_iszero(PyObject *module, PyValue arg_value)
{
    PyObject *arg = PyValue_Object(arg_value);
    if (arg == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(arg);
    RETURN_INT_VALUE(PyLong_IsZero(arg));
}


static PyValue
pylong_aspid(PyObject *module, PyValue arg_value)
{
    PyObject *arg = PyValue_Object(arg_value);
    if (arg == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(arg);
    pid_t value = PyLong_AsPid(arg);
    if (value == -1 && PyErr_Occurred()) {
        return PyValue_NULL;
    }
    return PyValue_FromResult(PyLong_FromPid(value));
}


static PyObject *
layout_to_dict(const PyLongLayout *layout)
{
    return Py_BuildValue("{sisisisi}",
        "bits_per_digit", (int)layout->bits_per_digit,
        "digit_size", (int)layout->digit_size,
        "digits_order", (int)layout->digits_order,
        "digit_endianness", (int)layout->digit_endianness);
}


static PyValue
pylong_export(PyObject *module, PyValue obj_value)
{
    PyObject *obj = PyValue_Object(obj_value);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    PyLongExport export_long;
    if (PyLong_Export(obj, &export_long) < 0) {
        return PyValue_NULL;
    }

    if (export_long.digits == NULL) {
        assert(export_long.negative == 0);
        assert(export_long.ndigits == 0);
        assert(export_long.digits == NULL);
        PyObject *res = PyLong_FromInt64(export_long.value);
        PyLong_FreeExport(&export_long);
        return PyValue_FromResult(res);
    }

    assert(PyLong_GetNativeLayout()->digit_size == sizeof(digit));
    const digit *export_long_digits = export_long.digits;

    PyObject *digits = PyList_New(0);
    if (digits == NULL) {
        goto error;
    }
    for (Py_ssize_t i = 0; i < export_long.ndigits; i++) {
        PyObject *item = PyLong_FromUnsignedLong(export_long_digits[i]);
        if (item == NULL) {
            goto error;
        }

        if (PyList_Append(digits, item) < 0) {
            Py_DECREF(item);
            goto error;
        }
        Py_DECREF(item);
    }

    assert(export_long.value == 0);
    PyObject *res = Py_BuildValue("(iN)", export_long.negative, digits);

    PyLong_FreeExport(&export_long);
    assert(export_long._reserved == 0);

    return PyValue_FromResult(res);

error:
    Py_XDECREF(digits);
    PyLong_FreeExport(&export_long);
    return PyValue_NULL;
}


static PyObject *
pylongwriter_create(PyObject *module, PyObject *args)
{
    int negative;
    PyObject *list;
    // TODO(vstinner): write test for negative ndigits and digits==NULL
    if (!PyArg_ParseTuple(args, "iO!", &negative, &PyList_Type, &list)) {
        return NULL;
    }
    Py_ssize_t ndigits = PyList_GET_SIZE(list);

    digit *digits = PyMem_Malloc((size_t)ndigits * sizeof(digit));
    if (digits == NULL) {
        return PyErr_NoMemory();
    }

    for (Py_ssize_t i = 0; i < ndigits; i++) {
        PyObject *item = PyList_GET_ITEM(list, i);

        long num = PyLong_AsLong(item);
        if (num == -1 && PyErr_Occurred()) {
            goto error;
        }

        if (num < 0 || num >= (long)PyLong_BASE) {
            PyErr_SetString(PyExc_ValueError, "digit doesn't fit into digit");
            goto error;
        }
        digits[i] = (digit)num;
    }

    void *writer_digits;
    PyLongWriter *writer = PyLongWriter_Create(negative, ndigits,
                                               &writer_digits);
    if (writer == NULL) {
        goto error;
    }
    assert(PyLong_GetNativeLayout()->digit_size == sizeof(digit));
    memcpy(writer_digits, digits, (size_t)ndigits * sizeof(digit));
    PyObject *res = PyLongWriter_Finish(writer);
    PyMem_Free(digits);

    return res;

error:
    PyMem_Free(digits);
    return NULL;
}


static PyObject *
pylongwriter_finish_bug(PyObject *module, PyObject *Py_UNUSED(args))
{
    void *writer_digits;
    PyLongWriter *writer = PyLongWriter_Create(0, 3, &writer_digits);
    if (writer == NULL) {
        return NULL;
    }

    assert(PyLong_GetNativeLayout()->digit_size == sizeof(digit));
    digit *digits = writer_digits;
    digits[0] = 1;
    digits[1] = 1;
    // Oops, digits[2] is left uninitialized on purpose
    // to test PyLongWriter_Finish()
    return PyLongWriter_Finish(writer);
}


static PyObject *
get_pylong_layout(PyObject *module, PyObject *Py_UNUSED(args))
{
    const PyLongLayout *layout = PyLong_GetNativeLayout();
    return layout_to_dict(layout);
}


// Write into an immutable int object to test _PyStaticObjects_CheckAll()
static PyObject *
corrupt_long(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    int value;
    if (!PyArg_ParseTuple(args, "Oi", &obj, &value)) {
        return NULL;
    }

    ((PyLongObject*)obj)->long_value.ob_digit[0] = value;
    Py_RETURN_NONE;
}


static PyMethodDef test_methods[] = {
    _TESTCAPI_CALL_LONG_COMPACT_API_METHODDEF
    {"pylong_fromunicodeobject",    pylong_fromunicodeobject,   METH_VARARGS},
    {"pylong_asnativebytes",        pylong_asnativebytes,       METH_VARARGS},
    {"pylong_fromnativebytes",      pylong_fromnativebytes,     METH_VARARGS},
    {"pylong_getsign",              _PyCFunction_CAST(pylong_getsign), METH_O},
    {"pylong_aspid",                _PyCFunction_CAST(pylong_aspid), METH_O},
    {"pylong_export",               _PyCFunction_CAST(pylong_export), METH_O},
    {"pylongwriter_create",         pylongwriter_create,        METH_VARARGS},
    {"pylongwriter_finish_bug",     pylongwriter_finish_bug,    METH_NOARGS},
    {"get_pylong_layout",           get_pylong_layout,          METH_NOARGS},
    {"pylong_ispositive",           _PyCFunction_CAST(pylong_ispositive), METH_O},
    {"pylong_isnegative",           _PyCFunction_CAST(pylong_isnegative), METH_O},
    {"pylong_iszero",               _PyCFunction_CAST(pylong_iszero), METH_O},
    {"corrupt_long",                corrupt_long,               METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_Long(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }
    return 0;
}
