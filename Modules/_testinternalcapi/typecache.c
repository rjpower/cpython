// Test wrappers for the per-type lookup cache (pycore_typecache.h) and the
// lookup plans it holds (pycore_lookup.h).
//
// Insertion is exercised indirectly through normal attribute access (which
// calls _PyType_Lookup); only Lookup and Invalidate need direct wrappers.

#include "parts.h"

#include "pycore_critical_section.h"
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_value.h"         // PyValue_ObjectBorrow()
#include "pycore_typecache.h"     // _PyTypeCache_Lookup()
#include "pycore_lookup.h"        // _PyShape_LookupPlan()


static int
require_type(PyObject *obj)
{
    if (!PyType_Check(obj)) {
        PyErr_SetString(PyExc_TypeError, "expected a type");
        return -1;
    }
    return 0;
}

static PyObject *
intern_name(PyObject *name)
{
    if (!PyUnicode_CheckExact(name)) {
        PyErr_SetString(PyExc_TypeError, "name must be a str");
        return NULL;
    }
    Py_INCREF(name);
    PyUnicode_InternInPlace(&name);
    return name;
}

// type_cache_lookup(type, name) -> (cache_hit, value_or_None, version_tag)
static PyObject *
type_cache_lookup(PyObject *Py_UNUSED(self), PyObject *args)
{
    PyObject *type_obj, *name;
    if (!PyArg_ParseTuple(args, "OU", &type_obj, &name)) {
        return NULL;
    }
    if (require_type(type_obj) < 0) {
        return NULL;
    }
    name = intern_name(name);
    if (name == NULL) {
        return NULL;
    }
    PyTypeObject *type = (PyTypeObject *)type_obj;
    struct _PyTypeCacheLookupResult r =
        _PyTypeCache_Lookup(type, _Py_TYPE_ROOT_SHAPE(type), name);
    Py_DECREF(name);
    PyObject *value;
    if (PyValue_IsNull(r.plan.value)) {
        value = Py_NewRef(Py_None);
    }
    else {
        value = PyValue_Object(r.plan.value);
        if (value == NULL) {
            return NULL;
        }
    }
    return Py_BuildValue("(iNk)",
                         r.cache_hit, value,
                         (unsigned long)r.version_tag);
}

static PyValue
type_cache_invalidate(PyObject *Py_UNUSED(self), PyValue arg)
{
    PyObject *type_obj = PyValue_Object(arg);
    if (type_obj == NULL) {
        return PyValue_NULL;
    }
    if (require_type(type_obj) < 0) {
        return PyValue_NULL;
    }
    Py_BEGIN_CRITICAL_SECTION_MUTEX(&_PyInterpreterState_GET()->types.mutex);
    _PyTypeCache_Invalidate((PyTypeObject *)type_obj);
    Py_END_CRITICAL_SECTION();
    return PyValue_FromObject(Py_None);
}

static PyObject *
value_or_none(PyValue v)
{
    return PyValue_IsNull(v) ? Py_NewRef(Py_None) : PyValue_Object(v);
}

static int
ready_name(PyObject *obj, PyObject **name)
{
    if (!PyUnicode_Check(*name)) {
        PyErr_SetString(PyExc_TypeError, "name must be a str");
        return -1;
    }
    if (!_PyType_IsReady(Py_TYPE(obj)) && PyType_Ready(Py_TYPE(obj)) < 0) {
        return -1;
    }
    // Not interned here: a non-interned name must stay uncacheable.
    Py_INCREF(*name);
    return 0;
}

// lookup_plan(obj, name) -> (kind, fallback, slot_kind, cacheable, member,
//                            type_version, offset, value_or_None)
static PyObject *
lookup_plan(PyObject *Py_UNUSED(self), PyObject *args)
{
    PyObject *obj, *name;
    if (!PyArg_ParseTuple(args, "OO", &obj, &name) || ready_name(obj, &name) < 0) {
        return NULL;
    }
    _PyLookupPlan plan;
    _PyShape_LookupPlan(_PyObject_SHAPE(obj), name, &plan);
    Py_DECREF(name);
    PyObject *value = value_or_none(plan.value);
    if (value == NULL) {
        return NULL;
    }
    return Py_BuildValue("(ssiiikkN)",
                         _PyLookup_KindName(plan.kind),
                         _PyLookup_KindName(plan.fallback),
                         (int)plan.slot_kind,
                         (plan.flags & _Py_LOOKUP_CACHEABLE) != 0,
                         (plan.flags & _Py_LOOKUP_MEMBER) != 0,
                         (unsigned long)plan.type_version,
                         (unsigned long)plan.offset, value);
}

// lookup_store_plan(obj, name, delete=False) -> (outcome, cacheable, member,
//                                               offset, has_shape_to)
static PyObject *
lookup_store_plan(PyObject *Py_UNUSED(self), PyObject *args)
{
    PyObject *obj, *name;
    int delete = 0;
    if (!PyArg_ParseTuple(args, "OO|p", &obj, &name, &delete)
        || ready_name(obj, &name) < 0)
    {
        return NULL;
    }
    _PyStorePlan plan;
    _PyShape_LookupStorePlan(_PyObject_SHAPE(obj), name,
                             delete ? _Py_STORE_DELETE : _Py_STORE_SET, &plan);
    Py_DECREF(name);
    return Py_BuildValue("(siiki)",
                         _PyLookup_OutcomeName(plan.outcome),
                         (plan.flags & _Py_LOOKUP_CACHEABLE) != 0,
                         (plan.flags & _Py_LOOKUP_MEMBER) != 0,
                         (unsigned long)plan.offset,
                         plan.shape_to != NULL);
}

// shape(obj) -> (type, index_or_None, ((name, kind), ...)); holes are None
static PyObject *
shape(PyObject *Py_UNUSED(self), PyObject *obj)
{
    _PyShape *sh = _PyObject_SHAPE(obj);
    int index = _PyShape_Index(sh);
    PyObject *slots = PyTuple_New(sh->nslots);
    if (slots == NULL) {
        return NULL;
    }
    for (int i = 0; i < sh->nslots; i++) {
        PyObject *item = sh->slots[i].name == NULL
            ? Py_NewRef(Py_None)
            : Py_BuildValue("(Oi)", sh->slots[i].name, (int)sh->slots[i].kind);
        if (item == NULL) {
            Py_DECREF(slots);
            return NULL;
        }
        PyTuple_SET_ITEM(slots, i, item);
    }
    if (index == _Py_SHAPE_INDEX_DICT) {
        return Py_BuildValue("(OON)", (PyObject *)sh->type, Py_None, slots);
    }
    return Py_BuildValue("(OiN)", (PyObject *)sh->type, index, slots);
}

// type_slot_capacity(type) -> the slots of each instance, or None for a type
// without inline values
static PyObject *
type_slot_capacity(PyObject *Py_UNUSED(self), PyObject *type)
{
    if (require_type(type) < 0) {
        return NULL;
    }
    PyTypeObject *tp = (PyTypeObject *)type;
    if (!(tp->tp_flags & Py_TPFLAGS_INLINE_VALUES)) {
        Py_RETURN_NONE;
    }
    return PyLong_FromLong(_PyType_Shapes(tp)->capacity);
}

// object_slots(obj) -> the values of obj's attribute slots, NULL as None:
// all of them in shape mode, slot 0 alone in dictionary mode, whose other
// slots may lie outside the object after a __class__ assignment
static PyObject *
object_slots(PyObject *Py_UNUSED(self), PyObject *obj)
{
    PyTypeObject *tp = Py_TYPE(obj);
    if (!(tp->tp_flags & Py_TPFLAGS_INLINE_VALUES)) {
        PyErr_SetString(PyExc_TypeError, "no attribute slots");
        return NULL;
    }
    int n = _PyShape_IsDictMode(_PyObject_SHAPE(obj))
            ? 1 : _PyType_Shapes(tp)->capacity;
    PyValue *slots = _PyObject_Slots(obj);
    PyObject *res = PyTuple_New(n);
    if (res == NULL) {
        return NULL;
    }
    for (int i = 0; i < n; i++) {
        PyValue v = PyValue_IsNull(slots[i]) ? PyValue_FromObject(Py_None)
                                             : slots[i];
        PyTuple_SET_VALUE(res, i, v);
    }
    return res;
}

// object_dict_ptr(obj) -> (has_pointer, dict_or_None, error_pending):
// _PyObject_GetDictPtr(), which sets no exception
static PyObject *
object_dict_ptr(PyObject *Py_UNUSED(self), PyObject *obj)
{
    PyObject **dictptr = _PyObject_GetDictPtr(obj);
    int pending = PyErr_Occurred() != NULL;
    PyErr_Clear();
    PyObject *dict = dictptr != NULL && *dictptr != NULL ? *dictptr : Py_None;
    return Py_BuildValue("(OOO)", dictptr != NULL ? Py_True : Py_False,
                         dict, pending ? Py_True : Py_False);
}

// set_conversion_collect(flag) -> the old flag: whether conversion to
// dictionary mode collects just before it publishes the dict
static PyObject *
set_conversion_collect(PyObject *Py_UNUSED(self), PyObject *flag)
{
    int value = PyObject_IsTrue(flag);
    if (value < 0) {
        return NULL;
    }
    int old = _PyShape_ConversionCollect;
    _PyShape_ConversionCollect = value;
    return PyBool_FromLong(old);
}

// type_cache_size(type) -> bytes of the type's cache table
static PyObject *
type_cache_size(PyObject *Py_UNUSED(self), PyObject *type)
{
    if (require_type(type) < 0) {
        return NULL;
    }
    return PyLong_FromSize_t(_PyTypeCache_SizeOf((PyTypeObject *)type));
}


static PyMethodDef test_methods[] = {
    {"type_cache_lookup", type_cache_lookup, METH_VARARGS},
    {"type_cache_invalidate", _PyCFunction_CAST(type_cache_invalidate), METH_O},
    {"lookup_plan", lookup_plan, METH_VARARGS},
    {"lookup_store_plan", lookup_store_plan, METH_VARARGS},
    {"shape", shape, METH_O},
    {"type_cache_size", type_cache_size, METH_O},
    {"type_slot_capacity", type_slot_capacity, METH_O},
    {"object_slots", object_slots, METH_O},
    {"set_conversion_collect", set_conversion_collect, METH_O},
    {"object_dict_ptr", object_dict_ptr, METH_O},
    {NULL},
};

int
_PyTestInternalCapi_Init_TypeCache(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }
    if (PyModule_AddIntMacro(m, _Py_TYPECACHE_MINSIZE) < 0) {
        return -1;
    }
    return 0;
}
