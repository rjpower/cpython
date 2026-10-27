#include "parts.h"

static PyValue
codec_namereplace_errors(PyObject *Py_UNUSED(module), PyValue arg)
{
    PyObject *exc = PyValue_Object(arg);
    if (exc == NULL) {
        return PyValue_NULL;
    }
    assert(exc != NULL);
    return PyValue_FromResult(PyCodec_NameReplaceErrors(exc));
}


// Test Py_DecodeLocale()
static PyValue
decode_locale(PyObject *Py_UNUSED(module), PyValue v)
{
    PyObject *arg = PyValue_Object(v);
    if (arg == NULL) {
        return PyValue_NULL;
    }
    const char *str;
    Py_ssize_t unused_len;
    // Accept embedded null bytes
    if (PyArg_Parse(arg, "y#", &str, &unused_len) < 0) {
        return PyValue_NULL;
    }

    const size_t size_canary = (size_t)-123;
    size_t size = size_canary;
    wchar_t *wstr = Py_DecodeLocale(str, &size);

    if (wstr == NULL) {
        if (size == (size_t)-1) {
            PyErr_NoMemory();
        }
        else if (size == (size_t)-2) {
            PyErr_SetString(PyExc_RuntimeError, "decode error");
        }
        else {
            PyErr_Format(PyExc_SystemError,
                         "unknown Py_DecodeLocale() return value: %zd",
                         (Py_ssize_t)size);
        }
        return PyValue_NULL;
    }
    assert(wstr != NULL);
    assert(size != size_canary);

    PyObject *result = PyUnicode_FromWideChar(wstr, size);
    PyMem_RawFree(wstr);
    return PyValue_FromResult(result);
}


// Test Py_EncodeLocale()
static PyValue
encode_locale(PyObject *Py_UNUSED(module), PyValue value)
{
    PyObject *arg = PyValue_Object(value);
    if (arg == NULL) {
        return PyValue_NULL;
    }
    PyObject *unicode;
    if (PyArg_Parse(arg, "U", &unicode) < 0) {
        return PyValue_NULL;
    }

    // Accept embedded null characters
    Py_ssize_t unused_wlen;
    wchar_t *wstr = PyUnicode_AsWideCharString(unicode, &unused_wlen);
    if (wstr == NULL) {
        return PyValue_NULL;
    }

    const size_t error_pos_canary = (size_t)-123;
    size_t error_pos = error_pos_canary;
    char *str = Py_EncodeLocale(wstr, &error_pos);
    PyMem_Free(wstr);

    if (str == NULL) {
        if (error_pos == (size_t)-1) {
            return PyValue_FromResult(PyErr_NoMemory());
        }
        else {
            assert(error_pos != error_pos_canary);
            PyErr_Format(PyExc_RuntimeError,
                         "encode error: pos=%zd", error_pos);
            return PyValue_NULL;
        }
    }
    assert(error_pos == error_pos_canary);

    PyObject *result = PyBytes_FromString(str);
    PyMem_Free(str);
    return PyValue_FromResult(result);
}


static PyMethodDef test_methods[] = {
    {"codec_namereplace_errors", _PyCFunction_CAST(codec_namereplace_errors), METH_O},
    {"decode_locale", _PyCFunction_CAST(decode_locale), METH_O},
    {"encode_locale", _PyCFunction_CAST(encode_locale), METH_O},
    {NULL},
};

int
_PyTestLimitedCAPI_Init_Codec(PyObject *module)
{
    return PyModule_AddFunctions(module, test_methods);
}
