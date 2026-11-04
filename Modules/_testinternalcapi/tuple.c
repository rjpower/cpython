#include "parts.h"

#include "pycore_tuple.h"
#include "pycore_value.h"
#include "pycore_object.h"
#include "pycore_cell.h"


static PyObject *
tuple_from_pair(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *first, *second;
    if (!PyArg_ParseTuple(args, "OO", &first, &second)) {
        return NULL;
    }

    return _PyTuple_FromPair(first, second);
}

static PyObject *
tuple_from_pair_steal(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *first, *second;
    if (!PyArg_ParseTuple(args, "OO", &first, &second)) {
        return NULL;
    }

    return _PyTuple_FromPairSteal(Py_NewRef(first), Py_NewRef(second));
}

static PyObject *
tuple_value_layout(PyObject *Py_UNUSED(module), PyObject *obj)
{
    if (!PyTuple_Check(obj)) {
        PyErr_SetString(PyExc_TypeError, "expected a tuple");
        return NULL;
    }
    Py_ssize_t size = PyTuple_GET_SIZE(obj);
    PyObject *result = PyUnicode_New(size, 127);
    if (result == NULL) {
        return NULL;
    }
    for (Py_ssize_t i = 0; i < size; i++) {
        PyValue value = _PyTuple_CAST(obj)->ob_item[i];
        assert(PyValue_IsHeapSafe(value));
        char kind = PyValue_IsInlineInt(value) ? 'i' :
                    PyValue_IsInlineFloat(value) ? 'f' :
                    PyValue_IsObject(value) ? 'o' : '?';
        PyUnicode_WRITE(PyUnicode_KIND(result), PyUnicode_DATA(result), i, kind);
    }
    return result;
}

static PyObject *
tuple_value_object(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *tuple;
    Py_ssize_t index;
    if (!PyArg_ParseTuple(args, "On", &tuple, &index)) {
        return NULL;
    }
    return PyTuple_GetItemRef(tuple, index);
}

static PyObject *
cell_value_kind(PyObject *Py_UNUSED(module), PyObject *obj)
{
    if (!PyCell_Check(obj)) {
        PyErr_SetString(PyExc_TypeError, "expected a cell");
        return NULL;
    }
    PyValue value = ((PyCellObject *)obj)->ob_ref;
    assert(PyValue_IsHeapSafe(value));
    const char *kind = PyValue_IsNull(value) ? "n" :
                       PyValue_IsInlineInt(value) ? "i" :
                       PyValue_IsInlineFloat(value) ? "f" : "o";
    return PyUnicode_FromString(kind);
}

typedef struct {
    PyMemAllocatorEx saved;
} TupleBoxFailure;

static void *
tuple_fail_malloc(void *ctx, size_t size)
{
    return NULL;
}

static void *
tuple_fail_calloc(void *ctx, size_t count, size_t size)
{
    return NULL;
}

static void *
tuple_fail_realloc(void *ctx, void *ptr, size_t size)
{
    return NULL;
}

static void
tuple_fail_free(void *ctx, void *ptr)
{
    TupleBoxFailure *failure = ctx;
    failure->saved.free(failure->saved.ctx, ptr);
}

static void
tuple_fail_boxing(TupleBoxFailure *failure)
{
    PyMem_GetAllocator(PYMEM_DOMAIN_OBJ, &failure->saved);
    PyMemAllocatorEx alloc = {
        .ctx = failure,
        .malloc = tuple_fail_malloc,
        .calloc = tuple_fail_calloc,
        .realloc = tuple_fail_realloc,
        .free = tuple_fail_free,
    };
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &alloc);
}

static PyObject *
tuple_value_boxing_errors(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    /* Two PyLong digits avoid the one-digit freelist and small-int cache. */
    PyObject *integer = PyLong_FromLong(INT32_MAX);
    if (integer == NULL) {
        return NULL;
    }
    PyObject *tuple = _PyTuple_FromPairSteal(integer, Py_NewRef(Py_None));
    if (tuple == NULL) {
        return NULL;
    }
    PyObject *iter = PyObject_GetIter(tuple);
    if (iter == NULL) {
        Py_DECREF(tuple);
        return NULL;
    }

    TupleBoxFailure failure;
    tuple_fail_boxing(&failure);
    _PyValueIndexPair pair = PyTuple_Type._tp_iteritem(tuple, 0);
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &failure.saved);
    int valid = PyValue_IsInlineInt(pair.value) && pair.index == 1 &&
                PyValue_InlineInt64(pair.value) == INT32_MAX && !PyErr_Occurred();
    PyValue_XCLOSE(pair.value);
    PyErr_Clear();
    if (!valid) {
        PyErr_SetString(PyExc_AssertionError,
                        "virtual tuple iterator boxed an inline value");
        goto error;
    }

    tuple_fail_boxing(&failure);
    PyObject *item = PyIter_Next(iter);
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &failure.saved);
    valid = item == NULL && PyErr_ExceptionMatches(PyExc_MemoryError);
    Py_XDECREF(item);
    PyErr_Clear();
    if (!valid) {
        PyErr_SetString(PyExc_AssertionError,
                        "tuple iterator lost a boxing error");
        goto error;
    }
    /* The failed step consumed the int, as iterators do (listiter_next()). */
    item = PyIter_Next(iter);
    if (item == NULL) {
        goto error;
    }
    Py_DECREF(item);
    if (item != Py_None) {
        PyErr_SetString(PyExc_AssertionError,
                        "failed tuple boxing did not consume its item");
        goto error;
    }
    Py_DECREF(iter);
    Py_DECREF(tuple);
    Py_RETURN_NONE;

error:
    Py_DECREF(iter);
    Py_DECREF(tuple);
    return NULL;
}

static PyObject *
cell_value_boxing_errors(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    PyObject *cell = PyCell_NewValue(PyValue_FromInlineInt(INT32_MAX));
    if (cell == NULL) {
        return NULL;
    }
    TupleBoxFailure failure;
    tuple_fail_boxing(&failure);
    PyObject *boxed = PyCell_Get(cell);
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &failure.saved);
    int valid = boxed == NULL && PyErr_ExceptionMatches(PyExc_MemoryError);
    Py_XDECREF(boxed);
    PyErr_Clear();
    PyValue value = _PyCell_GetValue((PyCellObject *)cell);
    valid = valid && PyValue_IsInlineInt(value) &&
            PyValue_InlineInt64(value) == INT32_MAX;
    PyValue_XCLOSE(value);
    Py_DECREF(cell);
    if (!valid) {
        PyErr_SetString(PyExc_AssertionError,
                        "cell object getter lost OOM or changed its value");
        return NULL;
    }
    Py_RETURN_NONE;
}

typedef struct {
    PyObject_HEAD
    PyValue value;
} ValueMember;

static int
value_member_traverse(PyObject *self, visitproc visit, void *arg)
{
    _Py_VISIT_VALUE(((ValueMember *)self)->value);
    return 0;
}

static int
value_member_clear(PyObject *self)
{
    PyValue_CLEAR(((ValueMember *)self)->value);
    return 0;
}

static void
value_member_dealloc(PyObject *self)
{
    PyObject_GC_UnTrack(self);
    value_member_clear(self);
    PyObject_GC_Del(self);
}

static PyObject *
value_member_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    if (PyTuple_GET_SIZE(args) || (kwargs != NULL && PyDict_Size(kwargs))) {
        PyErr_SetString(PyExc_TypeError, "ValueMember takes no arguments");
        return NULL;
    }
    ValueMember *self = PyObject_GC_New(ValueMember, type);
    if (self == NULL) {
        return NULL;
    }
    self->value = PyValue_NULL;
    _PyObject_GC_TRACK(self);
    return (PyObject *)self;
}

static PyObject *
value_member_kind(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyValue value = ((ValueMember *)self)->value;
    assert(PyValue_IsHeapSafe(value));
    const char *kind = PyValue_IsNull(value) ? "n" :
                       PyValue_IsInlineInt(value) ? "i" :
                       PyValue_IsInlineFloat(value) ? "f" : "o";
    return PyUnicode_FromString(kind);
}

static PyMemberDef value_member_fields[] = {
    {"value", _Py_T_VALUE_EX, offsetof(ValueMember, value), 0},
    {"optional", _Py_T_VALUE, offsetof(ValueMember, value), 0},
    {"readonly", _Py_T_VALUE_EX, offsetof(ValueMember, value), Py_READONLY},
    {NULL},
};

static PyMethodDef value_member_methods[] = {
    {"kind", value_member_kind, METH_NOARGS},
    {NULL},
};

static PyTypeObject value_member_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testinternalcapi.ValueMember",
    .tp_basicsize = sizeof(ValueMember),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_dealloc = value_member_dealloc,
    .tp_traverse = value_member_traverse,
    .tp_clear = value_member_clear,
    .tp_new = value_member_new,
    .tp_members = value_member_fields,
    .tp_methods = value_member_methods,
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_setattro = PyObject_GenericSetAttr,
};

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


static PyMethodDef test_methods[] = {
    {"tuple_from_pair", tuple_from_pair, METH_VARARGS},
    {"tuple_from_pair_steal", tuple_from_pair_steal, METH_VARARGS},
    {"tuple_value_layout", tuple_value_layout, METH_O},
    {"tuple_value_object", tuple_value_object, METH_VARARGS},
    {"cell_value_kind", cell_value_kind, METH_O},
    {"cell_value_boxing_errors", cell_value_boxing_errors, METH_NOARGS},
    {"tuple_value_boxing_errors", tuple_value_boxing_errors, METH_NOARGS},
    {"_check_tuple_item_is_NULL", _check_tuple_item_is_NULL, METH_VARARGS},
    {NULL},
};

int
_PyTestInternalCapi_Init_Tuple(PyObject *m)
{
    if (PyModule_AddType(m, &value_member_type) < 0) {
        return -1;
    }
    return PyModule_AddFunctions(m, test_methods);
}
