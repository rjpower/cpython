#include "parts.h"
#include "pycore_descrobject.h"
#include "pycore_dict.h"
#include "pycore_value.h"

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
    assert(PyValue_IsHeapSafe(value));
    assert(PyValue_IsNull(value) || PyValue_IsLanguageValue(value));
    const char *kind = PyValue_IsNull(value) ? "empty" :
        PyValue_IsInlineInt(value) ? "int" :
        PyValue_IsInlineFloat(value) ? "float" : "object";
    PyObject *result = Py_BuildValue("(sK)", kind,
                                   (unsigned long long)value.bits);
    PyObject *saved = PyErr_GetRaisedException();
    Py_DECREF(descr);
    _PyValue_RestoreException(saved);
    if (result != NULL && PyErr_Occurred()) {
        saved = PyErr_GetRaisedException();
        Py_DECREF(result);
        _PyValue_RestoreException(saved);
        return NULL;
    }
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
    if (clear(obj) < 0 || PyErr_Occurred()) {
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
    PyObject *saved = PyErr_GetRaisedException();
    int restored = PyRefTracer_SetTracer(previous, previous_data);
    _PyValue_RestoreException(saved);
    if (restored < 0 || result == NULL || PyErr_Occurred()) {
        saved = PyErr_GetRaisedException();
        Py_XDECREF(result);
        _PyValue_RestoreException(saved);
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

static PyMethodDef methods[] = {
    {"slot_value_storage", slot_value_storage, METH_VARARGS},
    {"slot_clear", slot_clear, METH_O},
    {"slot_new_with_tracer", slot_new_with_tracer, METH_VARARGS},
    {NULL},
};

int
_PyTestInternalCapi_Init_Slots(PyObject *module)
{
    return PyModule_AddFunctions(module, methods);
}
