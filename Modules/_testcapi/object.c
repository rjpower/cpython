#include "parts.h"
#include "util.h"

static PyObject *
call_pyobject_print(PyObject *self, PyObject * args)
{
    PyObject *object;
    PyObject *filename;
    PyObject *print_raw;
    FILE *fp;
    int flags = 0;

    if (!PyArg_UnpackTuple(args, "call_pyobject_print", 3, 3,
                           &object, &filename, &print_raw)) {
        return NULL;
    }

    fp = Py_fopen(filename, "w+");
    if (fp == NULL) {
        return NULL;
    }

    if (Py_IsTrue(print_raw)) {
        flags = Py_PRINT_RAW;
    }

    if (PyObject_Print(object, fp, flags) < 0) {
        fclose(fp);
        return NULL;
    }

    fclose(fp);

    Py_RETURN_NONE;
}

static PyValue
pyobject_print_null(PyObject *self, PyValue filename_value)
{
    PyObject *filename = PyValue_Object(filename_value);
    if (filename == NULL) {
        return PyValue_NULL;
    }
    FILE *fp;

    fp = Py_fopen(filename, "w+");
    if (fp == NULL) {
        return PyValue_NULL;
    }

    if (PyObject_Print(NULL, fp, 0) < 0) {
        fclose(fp);
        return PyValue_NULL;
    }

    fclose(fp);

    return PyValue_FromObject(Py_None);
}

static PyValue
pyobject_print_os_error(PyObject *self, PyValue filename_value)
{
    PyObject *filename = PyValue_Object(filename_value);
    if (filename == NULL) {
        return PyValue_NULL;
    }
    PyObject *test_string;
    FILE *fp;

    test_string = PyUnicode_FromString("Spam spam spam");
    if (test_string == NULL) {
        return PyValue_NULL;
    }

    // open file in read mode to induce OSError
    fp = Py_fopen(filename, "r");
    if (fp == NULL) {
        Py_DECREF(test_string);
        return PyValue_NULL;
    }

    if (PyObject_Print(test_string, fp, 0) < 0) {
        fclose(fp);
        Py_DECREF(test_string);
        return PyValue_NULL;
    }

    fclose(fp);
    Py_DECREF(test_string);

    return PyValue_FromObject(Py_None);
}

static PyValue
pyobject_clear_weakrefs_no_callbacks(PyObject *self, PyValue obj_value)
{
    PyObject *obj = PyValue_Object(obj_value);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    PyUnstable_Object_ClearWeakRefsNoCallbacks(obj);
    return PyValue_FromObject(Py_None);
}

static PyValue
pyobject_enable_deferred_refcount(PyObject *self, PyValue obj_value)
{
    PyObject *obj = PyValue_Object(obj_value);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    int result = PyUnstable_Object_EnableDeferredRefcount(obj);
    return PyValue_FromResult(PyLong_FromLong(result));
}

static PyValue
pyobject_is_unique_temporary(PyObject *self, PyValue obj_value)
{
    PyObject *obj = PyValue_Object(obj_value);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    int result = PyUnstable_Object_IsUniqueReferencedTemporary(obj);
    return PyValue_FromResult(PyLong_FromLong(result));
}

static PyObject *
pyobject_is_unique_temporary_new_object(PyObject *self, PyObject *unused)
{
    PyObject *obj = PyList_New(0);
    int result = PyUnstable_Object_IsUniqueReferencedTemporary(obj);
    Py_DECREF(obj);
    return PyLong_FromLong(result);
}

static PyValue
clear_managed_dict(PyObject *self, PyValue obj_value)
{
    PyObject *obj = PyValue_Object(obj_value);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    PyObject_ClearManagedDict(obj);
    return PyValue_FromObject(Py_None);
}


static PyValue
is_uniquely_referenced(PyObject *self, PyValue op_value)
{
    PyObject *op = PyValue_Object(op_value);
    if (op == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromResult(
        PyBool_FromLong(PyUnstable_Object_IsUniquelyReferenced(op)));
}


static PyObject *
pyobject_dump(PyObject *self, PyObject *args)
{
    PyObject *op;
    int release_gil = 0;

    if (!PyArg_ParseTuple(args, "O|i", &op, &release_gil)) {
        return NULL;
    }
    NULLABLE(op);

    if (release_gil) {
        Py_BEGIN_ALLOW_THREADS
        PyObject_Dump(op);
        Py_END_ALLOW_THREADS

    }
    else {
        PyObject_Dump(op);
    }
    Py_RETURN_NONE;
}

static PyObject *
pysentinel_new(PyObject *self, PyObject *args)
{
    const char *name;
    const char *module_name = NULL;
    const char *repr = NULL;
    if (!PyArg_ParseTuple(args, "s|ss", &name, &module_name, &repr)) {
        return NULL;
    }
    return PySentinel_New(name, module_name, repr);
}

static PyValue
pysentinel_check(PyObject *self, PyValue obj_value)
{
    PyObject *obj = PyValue_Object(obj_value);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromResult(PyBool_FromLong(PySentinel_Check(obj)));
}

static PyValue
pysentinel_checkexact(PyObject *self, PyValue obj_value)
{
    PyObject *obj = PyValue_Object(obj_value);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromResult(PyBool_FromLong(PySentinel_CheckExact(obj)));
}


static PyMethodDef test_methods[] = {
    {"call_pyobject_print", call_pyobject_print, METH_VARARGS},
    {"pyobject_print_null", _PyCFunction_CAST(pyobject_print_null), METH_O},
    {"pyobject_print_os_error", _PyCFunction_CAST(pyobject_print_os_error), METH_O},
    {"pyobject_clear_weakrefs_no_callbacks", _PyCFunction_CAST(pyobject_clear_weakrefs_no_callbacks), METH_O},
    {"pyobject_enable_deferred_refcount", _PyCFunction_CAST(pyobject_enable_deferred_refcount), METH_O},
    {"pyobject_is_unique_temporary", _PyCFunction_CAST(pyobject_is_unique_temporary), METH_O},
    {"pyobject_is_unique_temporary_new_object", pyobject_is_unique_temporary_new_object, METH_NOARGS},
    {"clear_managed_dict", _PyCFunction_CAST(clear_managed_dict), METH_O, NULL},
    {"is_uniquely_referenced", _PyCFunction_CAST(is_uniquely_referenced), METH_O},
    {"pyobject_dump", pyobject_dump, METH_VARARGS},
    {"pysentinel_new", pysentinel_new, METH_VARARGS},
    {"pysentinel_check", _PyCFunction_CAST(pysentinel_check), METH_O},
    {"pysentinel_checkexact", _PyCFunction_CAST(pysentinel_checkexact), METH_O},
    {NULL},
};

int
_PyTestCapi_Init_Object(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
