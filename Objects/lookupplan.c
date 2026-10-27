/* Lookup plans: phase 2 of the generic attribute protocol.
 *
 * See Include/internal/pycore_lookup.h and boil/design/09-object-protocol.md
 * section 4.  Everything here is pure: it reads the shape, its type, the
 * type's shared keys and the MRO result's type, and allocates nothing.
 */

#include "Python.h"
#include "pycore_descrobject.h"   // PyMemberDescrObject
#include "pycore_dict.h"          // _PyDictKeys_StringLookupSplit()
#include "pycore_lookup.h"
#include "pycore_object.h"        // _PyObject_InlineValues()

/* The kind of DESCR, the MRO result, for a load (precedence step 1, 3 or
   4), and whether its type is mutable, so that its __get__ and __set__ can
   change without a change to the receiver's type. */
int
_PyLookup_ClassifyLoad(PyValue descr_v, int *mutable)
{
    *mutable = 0;
    if (PyValue_IsNull(descr_v)) {
        return _Py_LOOKUP_ABSENT;
    }
    if (PyValue_IsInline(descr_v)) {
        // An inline int or float in the class is not a descriptor.
        return _Py_LOOKUP_CLASS_VALUE;
    }
    PyTypeObject *t = Py_TYPE(PyValue_ObjectBorrow(descr_v));
    *mutable = (t->tp_flags & Py_TPFLAGS_IMMUTABLETYPE) == 0;
    if (t->tp_descr_get != NULL && t->tp_descr_set != NULL) {
        return t == &PyProperty_Type ? _Py_LOOKUP_PROPERTY
                                     : _Py_LOOKUP_DATA_DESCR;
    }
    if (t->tp_descr_get != NULL) {
        return (t->tp_flags & Py_TPFLAGS_METHOD_DESCRIPTOR)
               ? _Py_LOOKUP_METHOD : _Py_LOOKUP_DESCR;
    }
    return _Py_LOOKUP_CLASS_VALUE;
}

/* The offset of DESCR's slot if it is a __slots__ member that TYPE's
   objects may load (or store) directly, else 0. */
static uint32_t
member_offset(PyTypeObject *type, PyObject *descr, int is_store)
{
    if (!Py_IS_TYPE(descr, &PyMemberDescr_Type)) {
        return 0;
    }
    PyMemberDescrObject *member = (PyMemberDescrObject *)descr;
    PyMemberDef *def = member->d_member;
    if (def->type != _Py_T_VALUE_EX
        || (def->flags & _Py_AFTER_ITEMS)
        || def->offset <= 0
        || def->offset + (Py_ssize_t)sizeof(PyValue) > type->tp_basicsize
        || !PyType_IsSubtype(type, member->d_common.d_type))
    {
        return 0;
    }
    if (is_store ? (def->flags & Py_READONLY) : (def->flags & Py_AUDIT_READ)) {
        return 0;
    }
    return (uint32_t)def->offset;
}

/* The index of NAME in TYPE's shared keys, or -1. */
static Py_ssize_t
shared_index(PyTypeObject *type, PyObject *name)
{
    PyDictKeysObject *keys = ((PyHeapTypeObject *)type)->ht_cached_keys;
    if (keys == NULL) {
        return -1;
    }
    Py_ssize_t ix = _PyDictKeys_StringLookupSplit(keys, name);
    return ix < 0 ? -1 : ix;
}

/* The byte offset of inline value IX in an object of TYPE. */
static uint32_t
slot_offset(PyTypeObject *type, Py_ssize_t ix)
{
    return (uint32_t)(type->tp_basicsize + offsetof(PyDictValues, values)
                      + ix * sizeof(PyValue));
}

static int
has_dict_storage(PyTypeObject *type)
{
    return (type->tp_flags & Py_TPFLAGS_MANAGED_DICT) || type->tp_dictoffset != 0;
}

void
_PyShape_ResolveLoad(_PyShape *shape, PyObject *name, PyValue descr,
                     int cacheable, _PyLookupPlan *plan)
{
    PyTypeObject *type = shape->type;
    *plan = (_PyLookupPlan){
        .type_version = shape->type_version,
        .value = descr,
    };
    int mutable;
    int kind = _PyLookup_ClassifyLoad(descr, &mutable);
    if (cacheable && !mutable && plan->type_version != 0) {
        plan->flags |= _Py_LOOKUP_CACHEABLE;
    }
    if (kind == _Py_LOOKUP_DATA_DESCR || kind == _Py_LOOKUP_PROPERTY) {
        plan->kind = (uint8_t)kind;
        if (kind == _Py_LOOKUP_DATA_DESCR) {
            plan->offset = member_offset(type, PyValue_ObjectBorrow(descr), 0);
            if (plan->offset != 0) {
                plan->flags |= _Py_LOOKUP_MEMBER;
            }
        }
        return;
    }
    plan->fallback = (uint8_t)kind;
    if (type->tp_flags & Py_TPFLAGS_INLINE_VALUES) {
        if (_PyShape_IsDictMode(shape) || !PyUnicode_CheckExact(name)) {
            plan->kind = _Py_LOOKUP_DICT;
            return;
        }
        // A shape-mode object has valid inline values and no dict, and the
        // shape says which names they hold.
        Py_ssize_t ix = shared_index(type, name);
        if (ix >= 0 && _PyShape_HasSlot(shape, ix)) {
            plan->kind = _Py_LOOKUP_SLOT;
            plan->slot_kind = shape->slots[ix].kind;
            plan->offset = slot_offset(type, ix);
            return;
        }
        plan->kind = (uint8_t)kind;
        return;
    }
    plan->kind = has_dict_storage(type) ? _Py_LOOKUP_DICT : (uint8_t)kind;
}

void
_PyShape_ResolveStore(_PyShape *shape, PyObject *name, PyValue descr,
                      int cacheable, _PyStoreOp op, _PyStorePlan *plan)
{
    PyTypeObject *type = shape->type;
    *plan = (_PyStorePlan){
        .type_version = shape->type_version,
        .value = descr,
    };
    int mutable = 0;
    if (PyValue_IsObject(descr)) {
        PyObject *d = PyValue_ObjectBorrow(descr);
        mutable = (Py_TYPE(d)->tp_flags & Py_TPFLAGS_IMMUTABLETYPE) == 0;
        if (Py_TYPE(d)->tp_descr_set != NULL) {
            plan->outcome = _Py_STORE_DATA_DESCR;
            plan->offset = member_offset(type, d, 1);
            if (plan->offset != 0) {
                plan->flags |= _Py_LOOKUP_MEMBER;
            }
        }
    }
    if (cacheable && !mutable && plan->type_version != 0) {
        plan->flags |= _Py_LOOKUP_CACHEABLE;
    }
    if (plan->outcome == _Py_STORE_DATA_DESCR) {
        return;
    }
    if (type->tp_flags & Py_TPFLAGS_INLINE_VALUES) {
        if (_PyShape_IsDictMode(shape) || !PyUnicode_CheckExact(name)) {
            plan->outcome = _Py_STORE_DICT;
            return;
        }
        Py_ssize_t ix = shared_index(type, name);
        if (ix >= 0 && _PyShape_HasSlot(shape, ix)) {
            plan->offset = slot_offset(type, ix);
            plan->outcome = op == _Py_STORE_SET ? _Py_STORE_REPLACE
                                                : _Py_STORE_REMOVE;
            return;
        }
        if (op == _Py_STORE_DELETE) {
            plan->outcome = _Py_STORE_MISSING;
            return;
        }
        plan->outcome = _Py_STORE_ADD;
        if (ix >= 0) {
            plan->slot = (uint8_t)ix;
            plan->known_slot = 1;
            plan->offset = slot_offset(type, ix);
            for (_PyShape *c = shape->children; c != NULL; c = c->sibling) {
                if (c->edge_slot == ix) {
                    plan->shape_to = c;
                    break;
                }
            }
        }
        return;
    }
    plan->outcome = has_dict_storage(type) ? _Py_STORE_DICT
                                           : _Py_STORE_NO_STORAGE;
}

const char *
_PyLookup_KindName(int kind)
{
    switch (kind) {
        case _Py_LOOKUP_NONE: return "NONE";
        case _Py_LOOKUP_SLOT: return "SLOT";
        case _Py_LOOKUP_DICT: return "DICT";
        case _Py_LOOKUP_DATA_DESCR: return "DATA_DESCR";
        case _Py_LOOKUP_PROPERTY: return "PROPERTY";
        case _Py_LOOKUP_METHOD: return "METHOD";
        case _Py_LOOKUP_DESCR: return "DESCR";
        case _Py_LOOKUP_CLASS_VALUE: return "CLASS_VALUE";
        case _Py_LOOKUP_ABSENT: return "ABSENT";
    }
    return "?";
}

const char *
_PyLookup_OutcomeName(int outcome)
{
    switch (outcome) {
        case _Py_STORE_REPLACE: return "REPLACE";
        case _Py_STORE_ADD: return "ADD";
        case _Py_STORE_REMOVE: return "REMOVE";
        case _Py_STORE_DICT: return "DICT";
        case _Py_STORE_DATA_DESCR: return "DATA_DESCR";
        case _Py_STORE_MISSING: return "MISSING";
        case _Py_STORE_NO_STORAGE: return "NO_STORAGE";
    }
    return "?";
}
