#include "parts.h"
#include "util.h"


static PyValue
sys_getattr(PyObject *Py_UNUSED(module), PyValue arg)
{
    PyObject *name = PyValue_Object(arg);
    if (name == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(name);
    return PyValue_FromResult(PySys_GetAttr(name));
}

static PyValue
sys_getattrstring(PyObject *Py_UNUSED(module), PyValue value)
{
    PyObject *arg = PyValue_Object(value);
    if (arg == NULL) {
        return PyValue_NULL;
    }
    const char *name;
    Py_ssize_t size;
    if (!PyArg_Parse(arg, "z#", &name, &size)) {
        return PyValue_NULL;
    }
    return PyValue_FromResult(PySys_GetAttrString(name));
}

static PyValue
sys_getoptionalattr(PyObject *Py_UNUSED(module), PyValue arg)
{
    PyObject *name = PyValue_Object(arg);
    if (name == NULL) {
        return PyValue_NULL;
    }
    PyObject *value = UNINITIALIZED_PTR;
    NULLABLE(name);

    switch (PySys_GetOptionalAttr(name, &value)) {
        case -1:
            assert(value == NULL);
            assert(PyErr_Occurred());
            return PyValue_NULL;
        case 0:
            assert(value == NULL);
            return PyValue_FromResult(Py_NewRef(PyExc_AttributeError));
        case 1:
            return PyValue_FromResult(value);
        default:
            Py_FatalError("PySys_GetOptionalAttr() returned invalid code");
    }
}

static PyValue
sys_getoptionalattrstring(PyObject *Py_UNUSED(module), PyValue v)
{
    PyObject *arg = PyValue_Object(v);
    if (arg == NULL) {
        return PyValue_NULL;
    }
    PyObject *value = UNINITIALIZED_PTR;
    const char *name;
    Py_ssize_t size;
    if (!PyArg_Parse(arg, "z#", &name, &size)) {
        return PyValue_NULL;
    }

    switch (PySys_GetOptionalAttrString(name, &value)) {
        case -1:
            assert(value == NULL);
            assert(PyErr_Occurred());
            return PyValue_NULL;
        case 0:
            assert(value == NULL);
            return PyValue_FromResult(Py_NewRef(PyExc_AttributeError));
        case 1:
            return PyValue_FromResult(value);
        default:
            Py_FatalError("PySys_GetOptionalAttrString() returned invalid code");
    }
}

static PyValue
sys_getobject(PyObject *Py_UNUSED(module), PyValue value)
{
    PyObject *arg = PyValue_Object(value);
    if (arg == NULL) {
        return PyValue_NULL;
    }
    const char *name;
    Py_ssize_t size;
    if (!PyArg_Parse(arg, "z#", &name, &size)) {
        return PyValue_NULL;
    }
    PyObject *result = PySys_GetObject(name);
    if (result == NULL) {
        result = PyExc_AttributeError;
    }
    return PyValue_FromResult(Py_NewRef(result));
}

static PyObject *
sys_setobject(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *name;
    Py_ssize_t size;
    PyObject *value;
    if (!PyArg_ParseTuple(args, "z#O", &name, &size, &value)) {
        return NULL;
    }
    NULLABLE(value);
    RETURN_INT(PySys_SetObject(name, value));
}

static PyObject *
sys_getxoptions(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(ignored))
{
    PyObject *result = PySys_GetXOptions();
    return Py_XNewRef(result);
}


static PyMethodDef test_methods[] = {
    {"sys_getattr", _PyCFunction_CAST(sys_getattr), METH_O},
    {"sys_getattrstring", _PyCFunction_CAST(sys_getattrstring), METH_O},
    {"sys_getoptionalattr", _PyCFunction_CAST(sys_getoptionalattr), METH_O},
    {"sys_getoptionalattrstring", _PyCFunction_CAST(sys_getoptionalattrstring), METH_O},
    {"sys_getobject", _PyCFunction_CAST(sys_getobject), METH_O},
    {"sys_setobject", sys_setobject, METH_VARARGS},
    {"sys_getxoptions", sys_getxoptions, METH_NOARGS},
    {NULL},
};

int
_PyTestLimitedCAPI_Init_Sys(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }

    return 0;
}
