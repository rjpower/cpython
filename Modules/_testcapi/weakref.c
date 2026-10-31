#include "parts.h"
#include "util.h"


static PyValue
pyweakref_getref(PyObject *module, PyValue ref_value)
{
    PyObject *ref = PyValue_Object(ref_value);
    if (ref == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(ref);
    PyObject *obj = UNINITIALIZED_PTR;
    int rc = PyWeakref_GetRef(ref, &obj);
    if (rc == -1 && PyErr_Occurred()) {
        assert(obj == NULL);
        return PyValue_NULL;
    }
    if (obj == NULL) {
        return PyValue_FromResult(Py_BuildValue("i", rc));
    }
    else {
        assert(obj != UNINITIALIZED_PTR);
        return PyValue_FromResult(Py_BuildValue("iN", rc, obj));
    }
}

static PyValue
pyweakref_isdead(PyObject *module, PyValue obj_value)
{
    PyObject *obj = PyValue_Object(obj_value);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(obj);
    int rc = PyWeakref_IsDead(obj);
    if (rc == -1 && PyErr_Occurred()) {
        return PyValue_NULL;
    }
    return PyValue_FromResult(PyLong_FromLong(rc));
}


static PyMethodDef test_methods[] = {
    {"pyweakref_getref", _PyCFunction_CAST(pyweakref_getref), METH_O},
    {"pyweakref_isdead", _PyCFunction_CAST(pyweakref_isdead), METH_O},
    {NULL},
};

int
_PyTestCapi_Init_Weakref(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
