#include "parts.h"
#include "util.h"


static PyValue
function_get_code(PyObject *self, PyValue func_value)
{
    PyObject *func = PyValue_Object(func_value);
    if (func == NULL) {
        return PyValue_NULL;
    }
    PyObject *code = PyFunction_GetCode(func);
    if (code != NULL) {
        return PyValue_FromResult(Py_NewRef(code));
    } else {
        return PyValue_NULL;
    }
}


static PyValue
function_get_globals(PyObject *self, PyValue func_value)
{
    PyObject *func = PyValue_Object(func_value);
    if (func == NULL) {
        return PyValue_NULL;
    }
    PyObject *globals = PyFunction_GetGlobals(func);
    if (globals != NULL) {
        return PyValue_FromResult(Py_NewRef(globals));
    } else {
        return PyValue_NULL;
    }
}


static PyValue
function_get_module(PyObject *self, PyValue func_value)
{
    PyObject *func = PyValue_Object(func_value);
    if (func == NULL) {
        return PyValue_NULL;
    }
    PyObject *module = PyFunction_GetModule(func);
    if (module != NULL) {
        return PyValue_FromResult(Py_NewRef(module));
    } else {
        return PyValue_NULL;
    }
}


static PyValue
function_get_module_macro(PyObject *self, PyValue func_value)
{
    PyObject *func = PyValue_Object(func_value);
    if (func == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromResult(Py_XNewRef(PyFunction_GET_MODULE(func)));
}


static PyValue
function_get_defaults(PyObject *self, PyValue func_value)
{
    PyObject *func = PyValue_Object(func_value);
    if (func == NULL) {
        return PyValue_NULL;
    }
    PyObject *defaults = PyFunction_GetDefaults(func);
    if (defaults != NULL) {
        return PyValue_FromResult(Py_NewRef(defaults));
    } else if (PyErr_Occurred()) {
        return PyValue_NULL;
    } else {
        return PyValue_FromObject(Py_None);  // This can happen when `defaults` are set to `None`
    }
}


static PyObject *
function_set_defaults(PyObject *self, PyObject *args)
{
    PyObject *func = NULL, *defaults = NULL;
    if (!PyArg_ParseTuple(args, "OO", &func, &defaults)) {
        return NULL;
    }
    int result = PyFunction_SetDefaults(func, defaults);
    if (result == -1)
        return NULL;
    Py_RETURN_NONE;
}


static PyValue
function_get_kw_defaults(PyObject *self, PyValue func_value)
{
    PyObject *func = PyValue_Object(func_value);
    if (func == NULL) {
        return PyValue_NULL;
    }
    PyObject *defaults = PyFunction_GetKwDefaults(func);
    if (defaults != NULL) {
        return PyValue_FromResult(Py_NewRef(defaults));
    } else if (PyErr_Occurred()) {
        return PyValue_NULL;
    } else {
        return PyValue_FromObject(Py_None);  // This can happen when `kwdefaults` are set to `None`
    }
}


static PyObject *
function_set_kw_defaults(PyObject *self, PyObject *args)
{
    PyObject *func = NULL, *defaults = NULL;
    if (!PyArg_ParseTuple(args, "OO", &func, &defaults)) {
        return NULL;
    }
    int result = PyFunction_SetKwDefaults(func, defaults);
    if (result == -1)
        return NULL;
    Py_RETURN_NONE;
}


static PyValue
function_get_closure(PyObject *self, PyValue func_value)
{
    PyObject *func = PyValue_Object(func_value);
    if (func == NULL) {
        return PyValue_NULL;
    }
    PyObject *closure = PyFunction_GetClosure(func);
    if (closure != NULL) {
        return PyValue_FromResult(Py_NewRef(closure));
    } else if (PyErr_Occurred()) {
        return PyValue_NULL;
    } else {
        return PyValue_FromObject(Py_None);  // This can happen when `closure` is set to `None`
    }
}


static PyObject *
function_set_closure(PyObject *self, PyObject *args)
{
    PyObject *func = NULL, *closure = NULL;
    if (!PyArg_ParseTuple(args, "OO", &func, &closure)) {
        return NULL;
    }
    int result = PyFunction_SetClosure(func, closure);
    if (result == -1) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyValue
function_get_annotations(PyObject *self, PyValue func_value)
{
    PyObject *func = PyValue_Object(func_value);
    if (func == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromResult(Py_XNewRef(PyFunction_GetAnnotations(func)));
}


static PyMethodDef test_methods[] = {
    {"function_get_code", _PyCFunction_CAST(function_get_code), METH_O, NULL},
    {"function_get_globals", _PyCFunction_CAST(function_get_globals), METH_O, NULL},
    {"function_get_module", _PyCFunction_CAST(function_get_module), METH_O, NULL},
    {"function_get_module_macro", _PyCFunction_CAST(function_get_module_macro), METH_O, NULL},
    {"function_get_defaults", _PyCFunction_CAST(function_get_defaults), METH_O, NULL},
    {"function_set_defaults", function_set_defaults, METH_VARARGS, NULL},
    {"function_get_kw_defaults", _PyCFunction_CAST(function_get_kw_defaults), METH_O, NULL},
    {"function_set_kw_defaults", function_set_kw_defaults, METH_VARARGS, NULL},
    {"function_get_closure", _PyCFunction_CAST(function_get_closure), METH_O, NULL},
    {"function_set_closure", function_set_closure, METH_VARARGS, NULL},
    {"function_get_annotations", _PyCFunction_CAST(function_get_annotations), METH_O, NULL},
    {NULL},
};

int
_PyTestCapi_Init_Function(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
