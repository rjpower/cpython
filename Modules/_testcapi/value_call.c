/* Callables and helpers for the value vectorcall, written as an extension
 * would write them: public headers and object slots only (design:
 * pyjit/design/proposals/value-api-m3.md, section 3). The tests are in
 * Lib/test/test_capi/test_value_call.py. */

#include "parts.h"

#include <stddef.h>                 // offsetof


typedef struct {
    PyObject_HEAD
    vectorcallfunc vectorcall;
} vcobject;

/* (args tuple, kwargs dict) from a vectorcall window */
static PyObject *
echo_vectorcall(PyObject *Py_UNUSED(callable), PyObject *const *args,
                size_t nargsf, PyObject *kwnames)
{
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyObject *tuple = PyTuple_New(nargs);
    if (tuple == NULL) {
        return NULL;
    }
    for (Py_ssize_t i = 0; i < nargs; i++) {
        PyTuple_SET_ITEM(tuple, i, Py_NewRef(args[i]));
    }
    PyObject *kwargs = PyDict_New();
    if (kwargs == NULL) {
        Py_DECREF(tuple);
        return NULL;
    }
    Py_ssize_t nkw = kwnames == NULL ? 0 : PyTuple_GET_SIZE(kwnames);
    for (Py_ssize_t i = 0; i < nkw; i++) {
        PyObject *name = PyTuple_GetItemRef(kwnames, i);
        if (name == NULL) {
            Py_DECREF(tuple);
            Py_DECREF(kwargs);
            return NULL;
        }
        int res = PyDict_SetItem(kwargs, name, args[nargs + i]);
        Py_DECREF(name);
        if (res < 0) {
            Py_DECREF(tuple);
            Py_DECREF(kwargs);
            return NULL;
        }
    }
    PyObject *result = PyTuple_Pack(2, tuple, kwargs);
    Py_DECREF(tuple);
    Py_DECREF(kwargs);
    return result;
}

static PyObject *
null_vectorcall(PyObject *Py_UNUSED(callable), PyObject *const *Py_UNUSED(args),
                size_t Py_UNUSED(nargsf), PyObject *Py_UNUSED(kwnames))
{
    /* Deliberately returns NULL with no exception set. */
    return NULL;
}

static PyObject *
echo_tpcall(PyObject *Py_UNUSED(self), PyObject *args, PyObject *kwargs)
{
    PyObject *kw = kwargs == NULL ? PyDict_New() : PyDict_Copy(kwargs);
    if (kw == NULL) {
        return NULL;
    }
    PyObject *result = PyTuple_Pack(2, args, kw);
    Py_DECREF(kw);
    return result;
}

static PyObject *
null_tpcall(PyObject *Py_UNUSED(self), PyObject *Py_UNUSED(args),
            PyObject *Py_UNUSED(kwargs))
{
    /* Deliberately returns NULL with no exception set. */
    return NULL;
}

/* Instances of the vectorcall types carry their entry point */
static PyObject *
echo_vc_new(PyTypeObject *type, PyObject *Py_UNUSED(args),
            PyObject *Py_UNUSED(kwargs))
{
    vcobject *self = (vcobject *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->vectorcall = echo_vectorcall;
    }
    return (PyObject *)self;
}

static PyObject *
null_vc_new(PyTypeObject *type, PyObject *Py_UNUSED(args),
            PyObject *Py_UNUSED(kwargs))
{
    vcobject *self = (vcobject *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->vectorcall = null_vectorcall;
    }
    return (PyObject *)self;
}

static PyTypeObject ObjVectorcall_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testcapi.ObjVectorcall",
    .tp_basicsize = sizeof(vcobject),
    .tp_vectorcall_offset = offsetof(vcobject, vectorcall),
    .tp_call = PyVectorcall_Call,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_VECTORCALL,
    .tp_new = echo_vc_new,
};

static PyTypeObject NullVectorcall_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testcapi.NullVectorcall",
    .tp_basicsize = sizeof(vcobject),
    .tp_vectorcall_offset = offsetof(vcobject, vectorcall),
    .tp_call = PyVectorcall_Call,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_VECTORCALL,
    .tp_new = null_vc_new,
};

static PyTypeObject ObjTpCall_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testcapi.ObjTpCall",
    .tp_basicsize = sizeof(PyObject),
    .tp_call = echo_tpcall,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
};

static PyTypeObject NullTpCall_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testcapi.NullTpCall",
    .tp_basicsize = sizeof(PyObject),
    .tp_call = null_tpcall,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
};


/* A C metaclass over type with its own tp_call: calling a class it made
   returns ("MetaTpCall", instance). */
static PyObject *
meta_tpcall(PyObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *obj = PyType_Type.tp_call(cls, args, kwargs);
    if (obj == NULL) {
        return NULL;
    }
    return Py_BuildValue("(sN)", "MetaTpCall", obj);
}

static PyTypeObject MetaTpCall_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testcapi.MetaTpCall",
    .tp_call = meta_tpcall,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
};


/* A replacement vectorcall for a Python function */
static vectorcallfunc default_vectorcall = NULL;

static PyObject *
replacement_vectorcall(PyObject *Py_UNUSED(callable), PyObject *const *args,
                       size_t nargsf, PyObject *kwnames)
{
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyObject *tuple = PyTuple_New(nargs);
    if (tuple == NULL) {
        return NULL;
    }
    for (Py_ssize_t i = 0; i < nargs; i++) {
        PyTuple_SET_ITEM(tuple, i, Py_NewRef(args[i]));
    }
    PyObject *result = Py_BuildValue("(sN)", "replaced", tuple);
    return result;
}

static PyObject *
set_vectorcall_replacement(PyObject *Py_UNUSED(module), PyObject *func)
{
    if (!PyFunction_Check(func)) {
        PyErr_SetString(PyExc_TypeError, "expected a Python function");
        return NULL;
    }
    if (default_vectorcall == NULL) {
        default_vectorcall = PyVectorcall_Function(func);
        if (default_vectorcall == NULL) {
            PyErr_SetString(PyExc_TypeError, "function has no vectorcall");
            return NULL;
        }
    }
    PyFunction_SetVectorcall((PyFunctionObject *)func,
                             replacement_vectorcall);
    Py_RETURN_NONE;
}

static PyObject *
restore_vectorcall(PyObject *Py_UNUSED(module), PyObject *func)
{
    if (!PyFunction_Check(func)) {
        PyErr_SetString(PyExc_TypeError, "expected a Python function");
        return NULL;
    }
    if (default_vectorcall == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "set_vectorcall_replacement was not called");
        return NULL;
    }
    PyFunction_SetVectorcall((PyFunctionObject *)func, default_vectorcall);
    Py_RETURN_NONE;
}

/* Call the callable's vectorcall entry point directly. For a core callable
   this is the object thunk. */
static PyObject *
vectorcall_function_call(PyObject *Py_UNUSED(module), PyObject *const *args,
                         Py_ssize_t nargs)
{
    if (nargs < 1) {
        PyErr_SetString(PyExc_TypeError, "expected a callable");
        return NULL;
    }
    PyObject *callable = args[0];
    vectorcallfunc func = PyVectorcall_Function(callable);
    if (func == NULL) {
        PyErr_SetString(PyExc_TypeError, "object has no vectorcall");
        return NULL;
    }
    return func(callable, args + 1, (size_t)(nargs - 1), NULL);
}

static PyObject *
get_self_ref(PyObject *Py_UNUSED(module), PyObject *callable)
{
    PyObject *self;
    if (PyCFunction_Check(callable)) {
        self = PyCFunction_GetSelfRef(callable);
    }
    else if (PyMethod_Check(callable)) {
        self = PyMethod_GetSelfRef(callable);
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "expected a builtin function or a bound method");
        return NULL;
    }
    if (self == NULL && !PyErr_Occurred()) {
        Py_RETURN_NONE;
    }
    return self;
}


static PyMethodDef test_methods[] = {
    {"set_vectorcall_replacement", set_vectorcall_replacement, METH_O},
    {"restore_vectorcall", restore_vectorcall, METH_O},
    {"vectorcall_function_call", _PyCFunction_CAST(vectorcall_function_call),
     METH_FASTCALL},
    {"get_self_ref", get_self_ref, METH_O},
    {NULL},
};

int
_PyTestCapi_Init_ValueCall(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }
    MetaTpCall_Type.tp_base = &PyType_Type;
    if (PyModule_AddType(mod, &MetaTpCall_Type) < 0
        || PyModule_AddType(mod, &ObjVectorcall_Type) < 0
        || PyModule_AddType(mod, &ObjTpCall_Type) < 0
        || PyModule_AddType(mod, &NullVectorcall_Type) < 0
        || PyModule_AddType(mod, &NullTpCall_Type) < 0)
    {
        return -1;
    }
    PyObject *null_vc = PyObject_CallNoArgs((PyObject *)&NullVectorcall_Type);
    if (null_vc == NULL) {
        return -1;
    }
    if (PyModule_Add(mod, "null_result_vectorcall", null_vc) < 0) {
        return -1;
    }
    PyObject *null_tp = PyObject_CallNoArgs((PyObject *)&NullTpCall_Type);
    if (null_tp == NULL) {
        return -1;
    }
    return PyModule_Add(mod, "null_result_tpcall", null_tp);
}
