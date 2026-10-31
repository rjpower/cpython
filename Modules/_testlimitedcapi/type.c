// Thin wrappers to PyType functions.
// Do no check PyType_Check() so Python tests can pass arbitrary objects,
// even if it's likely to crash.

#include "parts.h"
#include "util.h"


static PyType_Slot HeapTypeNameType_slots[] = {
    {0},
};

static PyType_Spec HeapTypeNameType_Spec = {
    .name = "_testcapi.HeapTypeNameType",
    .basicsize = sizeof(PyObject),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = HeapTypeNameType_slots,
};


// Test PyType_FromSpec() with a minimum PyType_Spec
static PyObject*
get_heaptype_for_name(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return PyType_FromSpec(&HeapTypeNameType_Spec);
}


// Test PyType_GetName()
static PyValue
get_type_name(PyObject *self, PyValue value)
{
    PyObject *arg = PyValue_Object(value);
    if (arg == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(arg);
    PyTypeObject *type = (PyTypeObject*)arg;

    return PyValue_FromResult(PyType_GetName(type));
}


// Test PyType_GetQualName()
static PyValue
get_type_qualname(PyObject *self, PyValue value)
{
    PyObject *arg = PyValue_Object(value);
    if (arg == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(arg);
    PyTypeObject *type = (PyTypeObject*)arg;

    return PyValue_FromResult(PyType_GetQualName(type));
}


// Test PyType_GetFullyQualifiedName()
static PyValue
get_type_fullyqualname(PyObject *self, PyValue value)
{
    PyObject *arg = PyValue_Object(value);
    if (arg == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(arg);
    PyTypeObject *type = (PyTypeObject*)arg;

    return PyValue_FromResult(PyType_GetFullyQualifiedName(type));
}


// Test PyType_GetModuleName()
static PyValue
get_type_module_name(PyObject *self, PyValue value)
{
    PyObject *arg = PyValue_Object(value);
    if (arg == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(arg);
    PyTypeObject *type = (PyTypeObject*)arg;

    return PyValue_FromResult(PyType_GetModuleName(type));
}


// Test PyType_Modified()
static PyValue
type_modified(PyObject *self, PyValue value)
{
    PyObject *arg = PyValue_Object(value);
    if (arg == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(arg);
    PyTypeObject *type = (PyTypeObject*)arg;

    PyType_Modified(type);
    return PyValue_FromObject(Py_None);
}


// Test PyType_Ready()
static PyValue
type_ready(PyObject *self, PyValue value)
{
    PyObject *arg = PyValue_Object(value);
    if (arg == NULL) {
        return PyValue_NULL;
    }
    assert(!PyErr_Occurred());
    NULLABLE(arg);
    PyTypeObject *type = (PyTypeObject*)arg;

    if (PyType_Ready(type) < 0) {
        assert(PyErr_Occurred());
        return PyValue_NULL;
    }
    assert(!PyErr_Occurred());
    return PyValue_FromObject(Py_None);
}


// Test PyType_Freeze()
static PyValue
type_freeze(PyObject *module, PyValue value)
{
    PyObject *arg = PyValue_Object(value);
    if (arg == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(arg);
    PyTypeObject *type = (PyTypeObject*)arg;

    if (PyType_Freeze(type) < 0) {
        return PyValue_NULL;
    }
    return PyValue_FromObject(Py_None);
}


// Test PyType_ClearCache()
static PyObject *
type_clearcache(PyObject *module, PyObject *Py_UNUSED(arg))
{
    // Since Python 3.16, PyType_ClearCache() is a no-op as the type cache is
    // now implemented per-type. It still returns the current version tag.
    unsigned int version_tag = PyType_ClearCache();
    assert(!PyErr_Occurred());
    return PyLong_FromUnsignedLong(version_tag);
}


// Test PyType_GetFlags()
static PyValue
type_getflags(PyObject *module, PyValue value)
{
    PyObject *arg = PyValue_Object(value);
    if (arg == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(arg);
    PyTypeObject *type = (PyTypeObject*)arg;

    unsigned long flags = PyType_GetFlags(type);
    assert(!PyErr_Occurred());
    return PyValue_FromResult(PyLong_FromUnsignedLong(flags));
}


// Test PyType_IsSubtype()
static PyObject *
type_issubtype(PyObject *module, PyObject *args)
{
    PyTypeObject *type1, *type2;
    if (!PyArg_ParseTuple(args, "O!O!",
                          &PyType_Type, &type1,
                          &PyType_Type, &type2)) {
        return NULL;
    }

    int is_subtype = PyType_IsSubtype(type1, type2);
    return PyLong_FromLong(is_subtype);
}


static PyMethodDef test_methods[] = {
    {"get_heaptype_for_name", get_heaptype_for_name, METH_NOARGS},
    {"get_type_name", _PyCFunction_CAST(get_type_name), METH_O},
    {"get_type_qualname",  _PyCFunction_CAST(get_type_qualname), METH_O},
    {"get_type_fullyqualname", _PyCFunction_CAST(get_type_fullyqualname), METH_O},
    {"get_type_module_name", _PyCFunction_CAST(get_type_module_name), METH_O},
    {"type_ready", _PyCFunction_CAST(type_ready), METH_O},
    {"type_modified", _PyCFunction_CAST(type_modified), METH_O},
    {"type_freeze", _PyCFunction_CAST(type_freeze), METH_O},
    {"type_clearcache", type_clearcache, METH_NOARGS},
    {"type_getflags", _PyCFunction_CAST(type_getflags), METH_O},
    {"type_issubtype", type_issubtype, METH_VARARGS},
    {NULL},
};

int
_PyTestLimitedCAPI_Init_Type(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }

#define ADD_INT(macro) \
    do { \
        if (PyModule_AddIntConstant(m, #macro, macro) < 0) { \
            return -1; \
        } \
    } while (0)

    ADD_INT(Py_TPFLAGS_DEFAULT);

    ADD_INT(Py_TPFLAGS_HAVE_FINALIZE);
    ADD_INT(Py_TPFLAGS_HAVE_GC);
    ADD_INT(Py_TPFLAGS_HAVE_VERSION_TAG);
    ADD_INT(Py_TPFLAGS_HAVE_VECTORCALL);

    ADD_INT(Py_TPFLAGS_DISALLOW_INSTANTIATION );
    ADD_INT(Py_TPFLAGS_IMMUTABLETYPE);
    ADD_INT(Py_TPFLAGS_HEAPTYPE);
    ADD_INT(Py_TPFLAGS_BASETYPE);
    ADD_INT(Py_TPFLAGS_READY);
    ADD_INT(Py_TPFLAGS_READYING);
    ADD_INT(Py_TPFLAGS_METHOD_DESCRIPTOR);
    ADD_INT(Py_TPFLAGS_VALID_VERSION_TAG);
    ADD_INT(Py_TPFLAGS_IS_ABSTRACT);
    ADD_INT(_Py_TPFLAGS_MATCH_SELF);
    ADD_INT(Py_TPFLAGS_ITEMS_AT_END);

    ADD_INT(Py_TPFLAGS_LONG_SUBCLASS);
    ADD_INT(Py_TPFLAGS_LIST_SUBCLASS);
    ADD_INT(Py_TPFLAGS_TUPLE_SUBCLASS);
    ADD_INT(Py_TPFLAGS_BYTES_SUBCLASS);
    ADD_INT(Py_TPFLAGS_UNICODE_SUBCLASS);
    ADD_INT(Py_TPFLAGS_DICT_SUBCLASS);
    ADD_INT(Py_TPFLAGS_BASE_EXC_SUBCLASS);
    ADD_INT(Py_TPFLAGS_TYPE_SUBCLASS);

#undef ADD_INT
    return 0;
}
