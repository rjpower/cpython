/* List cases that call the value functions of
 * Include/internal/pycore_pyvalue.h, moved from _testcapi/list.c. */

#include "parts.h"
#include "../_testcapi/util.h"    // NULLABLE

#include "pycore_list.h"          // PyList_GET_VALUE(), PyList_AppendValue()
#include "pycore_tuple.h"         // PyTuple_GET_VALUE()
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
    return _PyValue_AsObjectAt(PyList_GET_VALUE(obj, i), BOX_SITE_TEST);
}

static PyObject *
list_get_value(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t index;
    if (!PyArg_ParseTuple(args, "On", &obj, &index)) return NULL;
    PyValue value = PyList_GetValue(obj, index);
    if (!PyValue_IsLanguageValue(value)) return NULL;
    PyObject *result = _PyValue_AsObjectAt(value, BOX_SITE_TEST);
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
    PyObject *result = _PyValue_AsObjectAt(value, BOX_SITE_TEST);
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

/* The W1 setters, with the value read from the argument tuple, so an int or
   float argument arrives inline. */

static PyObject *
list_set_value(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t index;
    PyObject *item;
    if (!PyArg_ParseTuple(args, "OnO", &obj, &index, &item)) return NULL;
    NULLABLE(obj);
    if (PyList_SetValue(obj, index, PyValue_DUP(PyTuple_GET_VALUE(args, 2))) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
list_append_value(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    PyObject *item;
    if (!PyArg_ParseTuple(args, "OO", &obj, &item)) return NULL;
    NULLABLE(obj);
    if (PyList_AppendValue(obj, PyTuple_GET_VALUE(args, 1)) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
list_insert_value(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t index;
    PyObject *item;
    if (!PyArg_ParseTuple(args, "OnO", &obj, &index, &item)) return NULL;
    NULLABLE(obj);
    if (PyList_InsertValue(obj, index, PyTuple_GET_VALUE(args, 2)) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyMethodDef test_methods[] = {
    {"list_get_item", list_get_item, METH_VARARGS},
    {"list_get_value", list_get_value, METH_VARARGS},
    {"list_get_value_and_clear", list_get_value_and_clear, METH_VARARGS},
    {"list_value_bits", list_value_bits, METH_VARARGS},
    {"list_set_value", list_set_value, METH_VARARGS},
    {"list_append_value", list_append_value, METH_VARARGS},
    {"list_insert_value", list_insert_value, METH_VARARGS},
    {NULL},
};

int
_PyTestInternalCapi_Init_List(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
