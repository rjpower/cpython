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


/* ObjIndex: only an object nb_index */

static PyObject *
objindex_index(PyObject *Py_UNUSED(self))
{
    if (log_call("ObjIndex.index") < 0) {
        return NULL;
    }
    return PyLong_FromLong(7);
}

static PyNumberMethods objindex_as_number = {
    .nb_index = objindex_index,
};

static PyTypeObject ObjIndex_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testcapi.ObjIndex",
    .tp_basicsize = sizeof(PyObject),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_as_number = &objindex_as_number,
};


/* ObjNotImpl: object slots that return NotImplemented */

static PyObject *
objnotimpl_add(PyObject *Py_UNUSED(a), PyObject *Py_UNUSED(b))
{
    if (log_call("ObjNotImpl.add") < 0) {
        return NULL;
    }
    Py_RETURN_NOTIMPLEMENTED;
}

static PyObject *
objnotimpl_power(PyObject *Py_UNUSED(a), PyObject *Py_UNUSED(b),
                 PyObject *Py_UNUSED(c))
{
    if (log_call("ObjNotImpl.power") < 0) {
        return NULL;
    }
    Py_RETURN_NOTIMPLEMENTED;
}

static PyObject *
objnotimpl_richcompare(PyObject *Py_UNUSED(a), PyObject *Py_UNUSED(b),
                       int Py_UNUSED(op))
{
    if (log_call("ObjNotImpl.richcompare") < 0) {
        return NULL;
    }
    Py_RETURN_NOTIMPLEMENTED;
}

static PyNumberMethods objnotimpl_as_number = {
    .nb_add = objnotimpl_add,
    .nb_power = objnotimpl_power,
};

static PyTypeObject ObjNotImpl_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testcapi.ObjNotImpl",
    .tp_basicsize = sizeof(PyObject),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_as_number = &objnotimpl_as_number,
    .tp_richcompare = objnotimpl_richcompare,
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

/* M2: attributes */

/* ObjGetAttr: object tp_getattro and tp_setattro over a per-instance dict,
 * and an object-form getset g that stores an object. */

typedef struct {
    PyObject_HEAD
    PyObject *dict;
    PyObject *g;
} ObjGetAttrObject;

static PyObject *
objgetattr_new(PyTypeObject *type, PyObject *Py_UNUSED(args),
               PyObject *Py_UNUSED(kwds))
{
    ObjGetAttrObject *self = (ObjGetAttrObject *)type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }
    self->dict = PyDict_New();
    if (self->dict == NULL) {
        Py_DECREF(self);
        return NULL;
    }
    return (PyObject *)self;
}

static void
objgetattr_dealloc(PyObject *op)
{
    ObjGetAttrObject *self = (ObjGetAttrObject *)op;
    Py_XDECREF(self->dict);
    Py_XDECREF(self->g);
    Py_TYPE(op)->tp_free(op);
}

static PyObject *
objgetattr_getattro(PyObject *op, PyObject *name)
{
    ObjGetAttrObject *self = (ObjGetAttrObject *)op;
    PyObject *value;
    if (PyDict_GetItemRef(self->dict, name, &value) != 0) {
        return value;
    }
    /* Methods and getsets of the type. */
    return PyObject_GenericGetAttr(op, name);
}

static int
objgetattr_setattro(PyObject *op, PyObject *name, PyObject *value)
{
    ObjGetAttrObject *self = (ObjGetAttrObject *)op;
    /* The getset g is stored by its descriptor, not by the dict. */
    if (PyUnicode_Check(name) && PyUnicode_EqualToUTF8(name, "g")) {
        return PyObject_GenericSetAttr(op, name, value);
    }
    if (value == NULL) {
        int res = PyDict_Pop(self->dict, name, NULL);
        if (res == 0) {
            PyErr_SetObject(PyExc_AttributeError, name);
        }
        return res > 0 ? 0 : -1;
    }
    return PyDict_SetItem(self->dict, name, value);
}

static PyObject *
objgetattr_get_g(PyObject *op, void *Py_UNUSED(closure))
{
    ObjGetAttrObject *self = (ObjGetAttrObject *)op;
    if (self->g == NULL) {
        PyErr_SetString(PyExc_AttributeError, "g is not set");
        return NULL;
    }
    return Py_NewRef(self->g);
}

static int
objgetattr_set_g(PyObject *op, PyObject *value, void *Py_UNUSED(closure))
{
    ObjGetAttrObject *self = (ObjGetAttrObject *)op;
    if (value == NULL) {
        PyErr_SetString(PyExc_AttributeError, "cannot delete g");
        return -1;
    }
    Py_XSETREF(self->g, Py_NewRef(value));
    return 0;
}

static PyGetSetDef objgetattr_getset[] = {
    {"g", objgetattr_get_g, objgetattr_set_g, NULL, NULL},
    {NULL},
};

static PyTypeObject ObjGetAttr_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testcapi.ObjGetAttr",
    .tp_basicsize = sizeof(ObjGetAttrObject),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = objgetattr_new,
    .tp_dealloc = objgetattr_dealloc,
    .tp_getattro = objgetattr_getattro,
    .tp_setattro = objgetattr_setattro,
    .tp_getset = objgetattr_getset,
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
        &ObjIndex_Type,
        &ObjNotImpl_Type,
        &IntSubNothing_Type,
        &IntSubRepr_Type,
        &IntSubAdd_Type,
        &ObjGetAttr_Type,  /* M2 */
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
