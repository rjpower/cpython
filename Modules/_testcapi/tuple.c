#include "parts.h"
#include "util.h"


static PyObject *
tuple_get_size(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    RETURN_SIZE(PyTuple_GET_SIZE(obj));
}

static PyObject *
tuple_copy(PyObject *tuple)
{
    Py_ssize_t size = PyTuple_GET_SIZE(tuple);
    PyObject *newtuple = PyTuple_New(size);
    if (!newtuple) {
        return NULL;
    }
    for (Py_ssize_t n = 0; n < size; n++) {
        // Items may be NULL in an unfinished tuple; copy the raw value.
        PyValue item = PyTuple_GetValue(tuple, n);
        if (PyErr_Occurred() || PyTuple_SetValue(newtuple, n, item) < 0) {
            Py_DECREF(newtuple);
            return NULL;
        }
    }
    return newtuple;
}

static PyObject *
tuple_set_item(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj, *value, *newtuple;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "OnO", &obj, &i, &value)) {
        return NULL;
    }
    NULLABLE(value);
    // PyTuple_SET_ITEM() only fills an empty slot of a new tuple.
    Py_ssize_t size = PyTuple_GET_SIZE(obj);
    newtuple = PyTuple_New(size);
    if (!newtuple) {
        return NULL;
    }
    for (Py_ssize_t n = 0; n < size; n++) {
        if (n == i) {
            continue;
        }
        PyValue item = PyTuple_GetValue(obj, n);
        if (PyErr_Occurred() || PyTuple_SetValue(newtuple, n, item) < 0) {
            Py_DECREF(newtuple);
            return NULL;
        }
    }
    PyTuple_SET_ITEM(newtuple, i, Py_XNewRef(value));
    return newtuple;
}

static PyObject *
_tuple_resize(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *tup;
    Py_ssize_t newsize;
    int new = 1;
    if (!PyArg_ParseTuple(args, "On|p", &tup, &newsize, &new)) {
        return NULL;
    }
    if (new) {
        tup = tuple_copy(tup);
        if (!tup) {
            return NULL;
        }
    }
    else {
        NULLABLE(tup);
        Py_XINCREF(tup);
    }
    int r = _PyTuple_Resize(&tup, newsize);
    if (r == -1) {
        assert(tup == NULL);
        return NULL;
    }
    return tup;
}

static PyObject *
_check_tuple_item_is_NULL(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "On", &obj, &i)) {
        return NULL;
    }
    // An empty slot is the only non-language value a tuple can hold.
    return PyLong_FromLong(!PyValue_IsLanguageValue(PyTuple_GET_VALUE(obj, i)));
}


static PyObject *
tuple_fromarray(PyObject* Py_UNUSED(module), PyObject *args)
{
    PyObject *src;
    Py_ssize_t size = UNINITIALIZED_SIZE;
    if (!PyArg_ParseTuple(args, "O|n", &src, &size)) {
        return NULL;
    }
    if (src != Py_None && !PyTuple_Check(src)) {
        PyErr_SetString(PyExc_TypeError, "expect a tuple");
        return NULL;
    }

    // Items of a tuple may be stored inline, so build an array of objects.
    PyObject **items = NULL;
    if (src != Py_None) {
        if (size == UNINITIALIZED_SIZE) {
            size = PyTuple_GET_SIZE(src);
        }
        if (size > 0) {
            items = PyMem_Calloc(size, sizeof(PyObject *));
            if (items == NULL) {
                return PyErr_NoMemory();
            }
        }
    }
    PyObject *res = NULL;
    for (Py_ssize_t i = 0; items != NULL && i < size; i++) {
        items[i] = PyTuple_GetItemRef(src, i);
        if (items[i] == NULL) {
            goto done;
        }
    }
    res = PyTuple_FromArray(items, size);
done:
    for (Py_ssize_t i = 0; items != NULL && i < size; i++) {
        Py_XDECREF(items[i]);
    }
    PyMem_Free(items);
    return res;
}


static PyMethodDef test_methods[] = {
    {"tuple_get_size", tuple_get_size, METH_O},
    {"tuple_set_item", tuple_set_item, METH_VARARGS},
    {"_tuple_resize", _tuple_resize, METH_VARARGS},
    {"_check_tuple_item_is_NULL", _check_tuple_item_is_NULL, METH_VARARGS},
    {"tuple_fromarray", tuple_fromarray, METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_Tuple(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }

    return 0;
}
