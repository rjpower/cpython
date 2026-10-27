#include "parts.h"
#include "util.h"

static PyValue
set_check(PyObject *self, PyValue arg)
{
    PyObject *obj = PyValue_Object(arg);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(obj);
    RETURN_INT_VALUE(PySet_Check(obj));
}

static PyValue
set_checkexact(PyObject *self, PyValue arg)
{
    PyObject *obj = PyValue_Object(arg);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(obj);
    RETURN_INT_VALUE(PySet_CheckExact(obj));
}

static PyValue
frozenset_check(PyObject *self, PyValue arg)
{
    PyObject *obj = PyValue_Object(arg);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(obj);
    RETURN_INT_VALUE(PyFrozenSet_Check(obj));
}

static PyValue
frozenset_checkexact(PyObject *self, PyValue arg)
{
    PyObject *obj = PyValue_Object(arg);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(obj);
    RETURN_INT_VALUE(PyFrozenSet_CheckExact(obj));
}

static PyValue
anyset_check(PyObject *self, PyValue arg)
{
    PyObject *obj = PyValue_Object(arg);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(obj);
    RETURN_INT_VALUE(PyAnySet_Check(obj));
}

static PyValue
anyset_checkexact(PyObject *self, PyValue arg)
{
    PyObject *obj = PyValue_Object(arg);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(obj);
    RETURN_INT_VALUE(PyAnySet_CheckExact(obj));
}

static PyObject *
set_new(PyObject *self, PyObject *args)
{
    PyObject *iterable = NULL;
    if (!PyArg_ParseTuple(args, "|O", &iterable)) {
        return NULL;
    }
    return PySet_New(iterable);
}

static PyObject *
frozenset_new(PyObject *self, PyObject *args)
{
    PyObject *iterable = NULL;
    if (!PyArg_ParseTuple(args, "|O", &iterable)) {
        return NULL;
    }
    return PyFrozenSet_New(iterable);
}

static PyValue
set_size(PyObject *self, PyValue arg)
{
    PyObject *obj = PyValue_Object(arg);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(obj);
    RETURN_SIZE_VALUE(PySet_Size(obj));
}

static PyObject *
set_contains(PyObject *self, PyObject *args)
{
    PyObject *obj, *item;
    if (!PyArg_ParseTuple(args, "OO", &obj, &item)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(item);
    RETURN_INT(PySet_Contains(obj, item));
}

static PyObject *
set_add(PyObject *self, PyObject *args)
{
    PyObject *obj, *item;
    if (!PyArg_ParseTuple(args, "OO", &obj, &item)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(item);
    RETURN_INT(PySet_Add(obj, item));
}

static PyObject *
set_discard(PyObject *self, PyObject *args)
{
    PyObject *obj, *item;
    if (!PyArg_ParseTuple(args, "OO", &obj, &item)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(item);
    RETURN_INT(PySet_Discard(obj, item));
}

static PyValue
set_pop(PyObject *self, PyValue arg)
{
    PyObject *obj = PyValue_Object(arg);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(obj);
    return PyValue_FromResult(PySet_Pop(obj));
}

static PyValue
set_clear(PyObject *self, PyValue arg)
{
    PyObject *obj = PyValue_Object(arg);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(obj);
    RETURN_INT_VALUE(PySet_Clear(obj));
}

static PyObject *
test_frozenset_add_in_capi(PyObject *self, PyObject *Py_UNUSED(obj))
{
    // Test that `frozenset` can be used with `PySet_Add`,
    // when frozenset is just created in CAPI.
    PyObject *fs = PyFrozenSet_New(NULL);
    if (fs == NULL) {
        return NULL;
    }
    PyObject *num = PyLong_FromLong(1);
    if (num == NULL) {
        goto error;
    }
    if (PySet_Add(fs, num) < 0) {
        goto error;
    }
    int contains = PySet_Contains(fs, num);
    if (contains < 0) {
        goto error;
    }
    else if (contains == 0) {
        goto unexpected;
    }
    Py_DECREF(fs);
    Py_DECREF(num);
    Py_RETURN_NONE;

unexpected:
    PyErr_SetString(PyExc_ValueError, "set does not contain expected value");
error:
    Py_DECREF(fs);
    Py_XDECREF(num);
    return NULL;
}

static PyObject *
raiseTestError(const char* test_name, const char* msg)
{
    PyErr_Format(PyExc_AssertionError, "%s: %s", test_name, msg);
    return NULL;
}

static PyObject *
test_frozenset_add_in_capi_tracking(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    // Test: GC tracking - frozenset with tracked object should be tracked
    PyObject *frozenset = PyFrozenSet_New(NULL);
    if (frozenset == NULL) {
        return NULL;
    }

    PyObject *tracked_obj = PyErr_NewException("_testlimitedcapi.py_set_add", NULL, NULL);
    if (tracked_obj == NULL) {
        goto error;
    }
    if (!PyObject_GC_IsTracked(tracked_obj)) {
        Py_DECREF(frozenset);
        Py_DECREF(tracked_obj);
        return raiseTestError("test_frozenset_add_in_capi_tracking",
                              "test object should be tracked");
    }
    if (PySet_Add(frozenset, tracked_obj) < 0) {
        goto error;
    }
    Py_DECREF(tracked_obj);
    if (!PyObject_GC_IsTracked(frozenset)) {
        Py_DECREF(frozenset);
        return raiseTestError("test_frozenset_add_in_capi_tracking",
                              "frozenset with with GC tracked object should be tracked");
    }
    Py_DECREF(frozenset);
    Py_RETURN_NONE;

error:
    Py_DECREF(frozenset);
    Py_XDECREF(tracked_obj);
    return NULL;
}


static PyObject *
test_set_contains_does_not_convert_unhashable_key(PyObject *self, PyObject *Py_UNUSED(obj))
{
    // See https://docs.python.org/3/c-api/set.html#c.PySet_Contains
    PyObject *outer_set = PySet_New(NULL);

    PyObject *needle = PySet_New(NULL);
    if (needle == NULL) {
        Py_DECREF(outer_set);
        return NULL;
    }

    PyObject *num = PyLong_FromLong(42);
    if (num == NULL) {
        Py_DECREF(outer_set);
        Py_DECREF(needle);
        return NULL;
    }

    if (PySet_Add(needle, num) < 0) {
        Py_DECREF(outer_set);
        Py_DECREF(needle);
        Py_DECREF(num);
        return NULL;
    }

    int result = PySet_Contains(outer_set, needle);

    Py_DECREF(num);
    Py_DECREF(needle);
    Py_DECREF(outer_set);

    if (result < 0) {
        if (PyErr_ExceptionMatches(PyExc_TypeError)) {
            PyErr_Clear();
            Py_RETURN_NONE;
        }
        return NULL;
    }

    PyErr_SetString(PyExc_AssertionError,
                    "PySet_Contains should have raised TypeError for unhashable key");
    return NULL;
}

static PyMethodDef test_methods[] = {
    {"set_check", _PyCFunction_CAST(set_check), METH_O},
    {"set_checkexact", _PyCFunction_CAST(set_checkexact), METH_O},
    {"frozenset_check", _PyCFunction_CAST(frozenset_check), METH_O},
    {"frozenset_checkexact", _PyCFunction_CAST(frozenset_checkexact), METH_O},
    {"anyset_check", _PyCFunction_CAST(anyset_check), METH_O},
    {"anyset_checkexact", _PyCFunction_CAST(anyset_checkexact), METH_O},

    {"set_new", set_new, METH_VARARGS},
    {"frozenset_new", frozenset_new, METH_VARARGS},

    {"set_size", _PyCFunction_CAST(set_size), METH_O},
    {"set_contains", set_contains, METH_VARARGS},
    {"set_add", set_add, METH_VARARGS},
    {"set_discard", set_discard, METH_VARARGS},
    {"set_pop", _PyCFunction_CAST(set_pop), METH_O},
    {"set_clear", _PyCFunction_CAST(set_clear), METH_O},

    {"test_frozenset_add_in_capi", test_frozenset_add_in_capi, METH_NOARGS},
    {"test_frozenset_add_in_capi_tracking", test_frozenset_add_in_capi_tracking, METH_NOARGS},
    {"test_set_contains_does_not_convert_unhashable_key",
     test_set_contains_does_not_convert_unhashable_key, METH_NOARGS},

    {NULL},
};

int
_PyTestLimitedCAPI_Init_Set(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }

    return 0;
}
