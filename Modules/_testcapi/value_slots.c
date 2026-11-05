/* Test types for value slots written as an extension would write them:
 * public headers and object slots only (pyjit/design/proposals/
 * value-api-v2a0.md, section 4.8). The tests are in
 * Lib/test/test_value_slots.py. */

#include "parts.h"


static PyObject *call_log = NULL;

static PyObject *
set_value_slots_log(PyObject *Py_UNUSED(module), PyObject *log)
{
    if (log != Py_None && !PyList_Check(log)) {
        PyErr_SetString(PyExc_TypeError, "expected a list or None");
        return NULL;
    }
    PyObject *old = call_log;
    call_log = log == Py_None ? NULL : Py_NewRef(log);
    Py_XDECREF(old);
    Py_RETURN_NONE;
}

static int
log_call(const char *what)
{
    if (call_log == NULL) {
        return 0;
    }
    PyObject *text = PyUnicode_FromString(what);
    if (text == NULL) {
        return -1;
    }
    int res = PyList_Append(call_log, text);
    Py_DECREF(text);
    return res;
}


/* ObjAdd: only object slots */

static PyObject *
objadd_add(PyObject *a, PyObject *b)
{
    if (log_call("ObjAdd.add") < 0) {
        return NULL;
    }
    return PyUnicode_FromFormat("ObjAdd.add(%s, %s)",
                                Py_TYPE(a)->tp_name, Py_TYPE(b)->tp_name);
}

static PyObject *
objadd_richcompare(PyObject *a, PyObject *b, int op)
{
    if (log_call("ObjAdd.richcompare") < 0) {
        return NULL;
    }
    return PyUnicode_FromFormat("ObjAdd.richcompare(%s, %s, %d)",
                                Py_TYPE(a)->tp_name, Py_TYPE(b)->tp_name, op);
}

static PyNumberMethods objadd_as_number = {
    .nb_add = objadd_add,
};

static PyTypeObject ObjAdd_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testcapi.ObjAdd",
    .tp_basicsize = sizeof(PyObject),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_as_number = &objadd_as_number,
    .tp_richcompare = objadd_richcompare,
};


/* Static subtypes of int */

static PyTypeObject IntSubNothing_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testcapi.IntSubNothing",
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_base = &PyLong_Type,
};

static PyObject *
intsubrepr_repr(PyObject *Py_UNUSED(self))
{
    if (log_call("IntSubRepr.repr") < 0) {
        return NULL;
    }
    return PyUnicode_FromString("IntSubRepr");
}

static PyTypeObject IntSubRepr_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testcapi.IntSubRepr",
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_base = &PyLong_Type,
    .tp_repr = intsubrepr_repr,
};

static PyObject *
intsubadd_add(PyObject *Py_UNUSED(a), PyObject *Py_UNUSED(b))
{
    if (log_call("IntSubAdd.add") < 0) {
        return NULL;
    }
    return PyUnicode_FromString("IntSubAdd.add");
}

static PyNumberMethods intsubadd_as_number = {
    .nb_add = intsubadd_add,
};

static PyTypeObject IntSubAdd_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testcapi.IntSubAdd",
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_base = &PyLong_Type,
    .tp_as_number = &intsubadd_as_number,
};


static PyMethodDef test_methods[] = {
    {"set_value_slots_log", set_value_slots_log, METH_O},
    {NULL},
};

int
_PyTestCapi_Init_ValueSlots(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }
    PyTypeObject *types[] = {
        &ObjAdd_Type,
        &IntSubNothing_Type,
        &IntSubRepr_Type,
        &IntSubAdd_Type,
    };
    for (size_t i = 0; i < Py_ARRAY_LENGTH(types); i++) {
        if (PyType_Ready(types[i]) < 0) {
            return -1;
        }
        if (PyModule_AddType(mod, types[i]) < 0) {
            return -1;
        }
    }
    return 0;
}
