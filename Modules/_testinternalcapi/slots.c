#include "parts.h"
#include "pycore_descrobject.h"
#include "pycore_dict.h"
#include "pycore_modsupport.h"    // _PyArg_NoKeywords()
#include "pycore_sliceobject.h"   // _PySlice_FromValues()
#include "pycore_tuple.h"
#include "pycore_value.h"
#include "pycore_value_api.h"     // _PyValue_FromObjectKnown()

static PyObject *
slot_value_storage(PyObject *module, PyObject *args)
{
    PyObject *obj, *name;
    if (!PyArg_ParseTuple(args, "OU:slot_value_storage", &obj, &name)) {
        return NULL;
    }
    PyObject *descr = PyObject_GetAttr((PyObject *)Py_TYPE(obj), name);
    if (descr == NULL) {
        return NULL;
    }
    if (!Py_IS_TYPE(descr, &PyMemberDescr_Type) ||
        !PyObject_TypeCheck(obj, PyDescr_TYPE(descr))) {
        Py_DECREF(descr);
        PyErr_SetString(PyExc_TypeError, "expected an instance value member");
        return NULL;
    }
    PyMemberDef *member = ((PyMemberDescrObject *)descr)->d_member;
    if (member->type != _Py_T_VALUE_EX && member->type != _Py_T_VALUE) {
        Py_DECREF(descr);
        PyErr_SetString(PyExc_TypeError, "member does not store PyValue");
        return NULL;
    }
    PyValue value = *(PyValue *)_PyMember_GetOffset(obj, member);
    _PyValue_RecheckHome(value);
    assert(PyValue_IsNull(value) || PyValue_IsLanguageValue(value));
    const char *kind = PyValue_IsNull(value) ? "empty" :
        PyValue_IsInlineInt(value) ? "int" :
        PyValue_IsInlineFloat(value) ? "float" : "object";
    PyObject *result = Py_BuildValue("(sK)", kind,
                                   (unsigned long long)value.bits);
    Py_DECREF(descr);
    return result;
}

static PyObject *
slot_clear(PyObject *module, PyObject *obj)
{
    inquiry clear = Py_TYPE(obj)->tp_clear;
    if (clear == NULL) {
        PyErr_SetString(PyExc_TypeError, "object has no clear operation");
        return NULL;
    }
    if (clear(obj) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

typedef struct {
    PyTypeObject *type;
    int seen;
    int valid;
} SlotTracer;

static int
slot_trace_create(PyObject *obj, PyRefTracerEvent event, void *data)
{
    SlotTracer *state = data;
    if (event != PyRefTracer_CREATE || Py_TYPE(obj) != state->type) {
        return 0;
    }
    state->seen++;
    if (state->type->tp_flags & Py_TPFLAGS_INLINE_VALUES) {
        PyDictValues *values = _PyObject_InlineValues(obj);
        PyDictKeysObject *keys = ((PyHeapTypeObject *)state->type)->ht_cached_keys;
        size_t capacity = shared_keys_usable_size(keys);
        int initialized = capacity < 256 && values->capacity == capacity &&
                          values->size == 0 && values->embedded == 1 &&
                          values->valid == 1 &&
                          _PyObject_GetManagedDict(obj) == NULL;
#ifdef Py_SHAPES
        initialized &= values->shape == _Py_SHAPE_ROOT;
#endif
        state->valid &= initialized;
        /* A bad header must not turn this observer into an out-of-bounds read. */
        if (initialized) {
            for (size_t i = 0; i < capacity; i++) {
                state->valid &= PyValue_IsNull(values->values[i]);
            }
        }
    }
    for (PyTypeObject *base = state->type; base != NULL; base = base->tp_base) {
        for (PyMemberDef *mp = base->tp_members;
             mp != NULL && mp->name != NULL; mp++) {
            if (mp->type == _Py_T_VALUE || mp->type == _Py_T_VALUE_EX) {
                PyValue value = *(PyValue *)_PyMember_GetOffset(obj, mp);
                state->valid &= PyValue_IsNull(value);
            }
        }
    }
    return 0;
}

static PyObject *
slot_new_with_tracer(PyObject *module, PyObject *args)
{
    PyObject *type, *constructor_args;
    if (!PyArg_ParseTuple(args, "O!O!:slot_new_with_tracer",
                          &PyType_Type, &type, &PyTuple_Type, &constructor_args)) {
        return NULL;
    }
    void *previous_data;
    PyRefTracer previous = PyRefTracer_GetTracer(&previous_data);
    SlotTracer state = {(PyTypeObject *)type, 0, 1};
    if (PyRefTracer_SetTracer(slot_trace_create, &state) < 0) {
        return NULL;
    }
    PyObject *result = PyObject_CallObject(type, constructor_args);
    int restored = PyRefTracer_SetTracer(previous, previous_data);
    if (restored < 0 || result == NULL) {
        Py_XDECREF(result);
        return NULL;
    }
    if (state.seen != 1 || !state.valid) {
        Py_DECREF(result);
        PyErr_SetString(PyExc_AssertionError,
                        "value slots were not empty at reference creation");
        return NULL;
    }
    return result;
}

static int
append_bits(PyObject *list, PyValue value)
{
    PyObject *bits = PyLong_FromUnsignedLongLong(value.bits);
    if (bits == NULL) {
        return -1;
    }
    int res = PyList_Append(list, bits);
    Py_DECREF(bits);
    return res;
}

/* Allocate an instance of type with PyType_GenericAlloc(), as the tp_alloc
   call of a C constructor does, and return (bits, instance): the bits of
   its tuple items and value members before anything wrote them, which are
   zero-filled. The items of a tuple subclass are then filled with 0 to
   nitems - 1 through PyTuple_SET_ITEM(), which requires empty items. */
static PyObject *
generic_alloc_value_bits(PyObject *module, PyObject *args)
{
    PyTypeObject *type;
    Py_ssize_t nitems;
    if (!PyArg_ParseTuple(args, "O!n:generic_alloc_value_bits",
                          &PyType_Type, &type, &nitems)) {
        return NULL;
    }
    if (nitems < 0 || (nitems != 0 && !PyType_IsSubtype(type, &PyTuple_Type))) {
        PyErr_SetString(PyExc_ValueError, "only a tuple subclass has items");
        return NULL;
    }
    PyObject *bits = PyList_New(0);
    if (bits == NULL) {
        return NULL;
    }
    PyObject *obj = PyType_GenericAlloc(type, nitems);
    if (obj == NULL) {
        goto error;
    }
    for (Py_ssize_t i = 0; i < nitems; i++) {
        if (append_bits(bits, _PyTuple_CAST(obj)->ob_item[i]) < 0) {
            goto error;
        }
    }
    for (PyTypeObject *base = type; base != NULL; base = base->tp_base) {
        for (PyMemberDef *mp = base->tp_members;
             mp != NULL && mp->name != NULL; mp++) {
            if (mp->type == _Py_T_VALUE || mp->type == _Py_T_VALUE_EX) {
                PyValue value = *(PyValue *)_PyMember_GetOffset(obj, mp);
                if (append_bits(bits, value) < 0) {
                    goto error;
                }
            }
        }
    }
    if (PyTuple_Check(obj)) {
        _PyTuple_RESET_HASH_CACHE(obj);
        for (Py_ssize_t i = 0; i < nitems; i++) {
            PyObject *item = PyLong_FromSsize_t(i);
            if (item == NULL) {
                goto error;
            }
            PyTuple_SET_ITEM(obj, i, item);
        }
    }
    PyObject *result = PyTuple_Pack(2, bits, obj);
    Py_DECREF(bits);
    Py_DECREF(obj);
    return result;

error:
    Py_DECREF(bits);
    Py_XDECREF(obj);
    return NULL;
}

/* An extension type that allocates its instances with PyObject_Calloc()
   and leaves its value members zero-filled, which is empty. */
typedef struct {
    PyObject_HEAD
    PyValue value;
    PyValue optional;
} CallocValuesObject;

static PyObject *
calloc_values_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    if (PyTuple_GET_SIZE(args) || (kwargs != NULL && PyDict_GET_SIZE(kwargs))) {
        PyErr_SetString(PyExc_TypeError, "CallocValues takes no arguments");
        return NULL;
    }
    PyObject *self = PyObject_Calloc(1, type->tp_basicsize);
    if (self == NULL) {
        return PyErr_NoMemory();
    }
    return PyObject_Init(self, type);
}

static void
calloc_values_dealloc(PyObject *op)
{
    CallocValuesObject *self = (CallocValuesObject *)op;
    PyValue_CLEAR(self->value);
    PyValue_CLEAR(self->optional);
    PyObject_Free(self);
}

static PyMemberDef calloc_values_members[] = {
    {"value", _Py_T_VALUE_EX, offsetof(CallocValuesObject, value), 0},
    {"optional", _Py_T_VALUE, offsetof(CallocValuesObject, optional), 0},
    {NULL},
};

static PyTypeObject calloc_values_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testinternalcapi.CallocValues",
    .tp_basicsize = sizeof(CallocValuesObject),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = calloc_values_new,
    .tp_dealloc = calloc_values_dealloc,
    .tp_members = calloc_values_members,
};

/* A static type with value fields of each kind (pyjit/design/proposals/
   value-api-v2a0.md, M4a): i, f and o read as None when absent
   (_Py_T_VALUE), ex raises when absent (_Py_T_VALUE_EX), and ro reads i and
   refuses writes. ValueFields(i, f, o) stores the arguments it is given. */
typedef struct {
    PyObject_HEAD
    PyValue i;
    PyValue f;
    PyValue o;
    PyValue ex;
} ValueFieldsObject;

#define _ValueFields_CAST(op) ((ValueFieldsObject *)(op))

static PyObject *
value_fields_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *i = NULL, *f = NULL, *o = NULL;
    if (!_PyArg_NoKeywords("ValueFields", kwargs) ||
        !PyArg_UnpackTuple(args, "ValueFields", 0, 3, &i, &f, &o)) {
        return NULL;
    }
    PyObject *self = type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }
    // tp_alloc zero-fills, so every field starts empty.
    ValueFieldsObject *fields = _ValueFields_CAST(self);
    PyObject *given[3] = {i, f, o};
    PyValue *homes[3] = {&fields->i, &fields->f, &fields->o};
    for (int k = 0; k < 3; k++) {
        if (given[k] != NULL) {
            *homes[k] = PyValue_FromPyObjectNew(given[k]);
            _PyValue_CheckHome(*homes[k]);
        }
    }
    return self;
}

static int
value_fields_traverse(PyObject *op, visitproc visit, void *arg)
{
    ValueFieldsObject *self = _ValueFields_CAST(op);
    _Py_VISIT_VALUE(self->i);
    _Py_VISIT_VALUE(self->f);
    _Py_VISIT_VALUE(self->o);
    _Py_VISIT_VALUE(self->ex);
    return 0;
}

static int
value_fields_clear(PyObject *op)
{
    ValueFieldsObject *self = _ValueFields_CAST(op);
    PyValue_CLEAR(self->i);
    PyValue_CLEAR(self->f);
    PyValue_CLEAR(self->o);
    PyValue_CLEAR(self->ex);
    return 0;
}

static void
value_fields_dealloc(PyObject *op)
{
    ValueFieldsObject *self = _ValueFields_CAST(op);
    PyObject_GC_UnTrack(op);
    PyValue_XCLOSE(self->i);
    PyValue_XCLOSE(self->f);
    PyValue_XCLOSE(self->o);
    PyValue_XCLOSE(self->ex);
    Py_TYPE(op)->tp_free(op);
}

static char
value_field_kind(PyValueBorrow value)
{
    return PyValue_IsNull(value) ? 'n' :
           PyValue_IsInlineInt(value) ? 'i' :
           PyValue_IsInlineFloat(value) ? 'f' : 'o';
}

/* The storage of i, f, o and ex, one letter each: n (empty), i (inline
   int), f (inline float) or o (object). */
static PyObject *
value_fields_kinds(PyObject *op, PyObject *Py_UNUSED(args))
{
    ValueFieldsObject *self = _ValueFields_CAST(op);
    char kinds[5] = {
        value_field_kind(PyValue_Borrow(self->i)),
        value_field_kind(PyValue_Borrow(self->f)),
        value_field_kind(PyValue_Borrow(self->o)),
        value_field_kind(PyValue_Borrow(self->ex)),
        '\0',
    };
    return PyUnicode_FromString(kinds);
}

/* (type, (i, f, o), (None, {'ex': ex})), built from the fields by value, so
   nothing boxes. An absent i, f or o is passed as None, which it reads as;
   the state is None when ex is absent. */
static PyObject *
value_fields_reduce(PyObject *op, PyObject *Py_UNUSED(args))
{
    ValueFieldsObject *self = _ValueFields_CAST(op);
    PyObject *key = NULL, *slots = NULL, *state = NULL, *result = NULL;
    PyObject *fields = PyTuple_New(3);
    if (fields == NULL) {
        goto done;
    }
    PyValue *homes[3] = {&self->i, &self->f, &self->o};
    for (int k = 0; k < 3; k++) {
        PyTuple_SET_VALUE(fields, k, PyValue_IsNull(*homes[k])
                                     ? PyValue_None : PyValue_DUP(*homes[k]));
    }
    if (PyValue_IsNull(self->ex)) {
        state = Py_NewRef(Py_None);
    }
    else {
        key = PyUnicode_FromString("ex");
        slots = key == NULL ? NULL : PyDict_New();
        if (slots == NULL ||
            PyDict_SetValue(slots, _PyValue_FromObjectKnown(key),
                            self->ex) < 0) {
            goto done;
        }
        state = PyTuple_Pack(2, Py_None, slots);
        if (state == NULL) {
            goto done;
        }
    }
    result = PyTuple_Pack(3, (PyObject *)Py_TYPE(self), fields, state);
done:
    Py_XDECREF(fields);
    Py_XDECREF(key);
    Py_XDECREF(slots);
    Py_XDECREF(state);
    return result;
}

static PyMemberDef value_fields_members[] = {
    {"i", _Py_T_VALUE, offsetof(ValueFieldsObject, i), 0},
    {"f", _Py_T_VALUE, offsetof(ValueFieldsObject, f), 0},
    {"o", _Py_T_VALUE, offsetof(ValueFieldsObject, o), 0},
    {"ex", _Py_T_VALUE_EX, offsetof(ValueFieldsObject, ex), 0},
    {"ro", _Py_T_VALUE, offsetof(ValueFieldsObject, i), Py_READONLY},
    {NULL},
};

static PyMethodDef value_fields_methods[] = {
    {"kinds", value_fields_kinds, METH_NOARGS},
    {"__reduce__", value_fields_reduce, METH_NOARGS},
    {NULL},
};

static PyTypeObject value_fields_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testinternalcapi.ValueFields",
    .tp_basicsize = sizeof(ValueFieldsObject),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_new = value_fields_new,
    .tp_dealloc = value_fields_dealloc,
    .tp_traverse = value_fields_traverse,
    .tp_clear = value_fields_clear,
    .tp_members = value_fields_members,
    .tp_methods = value_fields_methods,
};

/* _PySlice_FromValues() of up to three objects, each passed as the view of
   an owned value; a missing argument is passed as PyValue_NULL. */
static PyObject *
slice_from_values(PyObject *module, PyObject *args)
{
    PyObject *objects[3] = {NULL, NULL, NULL};
    if (!PyArg_UnpackTuple(args, "slice_from_values", 0, 3,
                           &objects[0], &objects[1], &objects[2])) {
        return NULL;
    }
    PyValue values[3];
    for (int k = 0; k < 3; k++) {
        values[k] = objects[k] == NULL ? PyValue_NULL
                                       : PyValue_FromPyObjectNew(objects[k]);
    }
    PyObject *slice = _PySlice_FromValues(values[0], values[1], values[2]);
    for (int k = 0; k < 3; k++) {
        PyValue_XCLOSE(values[k]);
    }
    return slice;
}

static PyMethodDef methods[] = {
    {"slice_from_values", slice_from_values, METH_VARARGS},
    {"slot_value_storage", slot_value_storage, METH_VARARGS},
    {"slot_clear", slot_clear, METH_O},
    {"slot_new_with_tracer", slot_new_with_tracer, METH_VARARGS},
    {"generic_alloc_value_bits", generic_alloc_value_bits, METH_VARARGS},
    {NULL},
};

int
_PyTestInternalCapi_Init_Slots(PyObject *module)
{
    if (PyModule_AddType(module, &calloc_values_type) < 0) {
        return -1;
    }
    if (PyModule_AddType(module, &value_fields_type) < 0) {
        return -1;
    }
    return PyModule_AddFunctions(module, methods);
}
