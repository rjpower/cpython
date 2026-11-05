/* List cases that call the value functions of
 * Include/internal/pycore_pyvalue.h, moved from _testcapi/list.c. */

#include "parts.h"
#include "../_testcapi/util.h"    // NULLABLE

#include "pycore_list.h"          // PyList_GET_VALUE()
#include "pycore_value.h"         // PyValue_AsObject()


static PyObject *
list_get_item(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "On", &obj, &i)) {
        return NULL;
    }
    NULLABLE(obj);
    return PyValue_AsObject(PyList_GET_VALUE(obj, i));
}

static PyObject *
list_get_value(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t index;
    if (!PyArg_ParseTuple(args, "On", &obj, &index)) return NULL;
    PyValue value = PyList_GetValue(obj, index);
    if (!PyValue_IsLanguageValue(value)) return NULL;
    PyObject *result = PyValue_AsObject(value);
    PyValue_XCLOSE(value);
    return result;
}

static PyObject *
list_get_value_and_clear(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t index;
    if (!PyArg_ParseTuple(args, "On", &obj, &index)) return NULL;
    PyValue value = PyList_GetValue(obj, index);
    if (!PyValue_IsLanguageValue(value)) return NULL;
    if (PyList_Clear(obj) < 0) {
        PyValue_XCLOSE(value);
        return NULL;
    }
    PyObject *result = PyValue_AsObject(value);
    PyValue_XCLOSE(value);
    return result;
}

static PyObject *
list_value_bits(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t index;
    if (!PyArg_ParseTuple(args, "On", &obj, &index)) return NULL;
    PyValue value = PyList_GetValue(obj, index);
    if (!PyValue_IsLanguageValue(value)) return NULL;
    uint64_t bits = value.bits;
    PyValue_XCLOSE(value);
    return PyLong_FromUnsignedLongLong(bits);
}


static PyMethodDef test_methods[] = {
    {"list_get_item", list_get_item, METH_VARARGS},
    {"list_get_value", list_get_value, METH_VARARGS},
    {"list_get_value_and_clear", list_get_value_and_clear, METH_VARARGS},
    {"list_value_bits", list_value_bits, METH_VARARGS},
    {NULL},
};

int
_PyTestInternalCapi_Init_List(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
