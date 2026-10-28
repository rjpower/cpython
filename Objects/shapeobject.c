/* Shapes: the hidden classes in the object header, and the attribute slots
 * they describe.
 *
 * See Include/internal/pycore_shape.h and boil/design/09-object-protocol.md.
 * Shapes are not objects.  The root shape is part of its type; the others,
 * of a heap type with inline values, are PyMem blocks owned by the type's
 * table and freed with it.
 */

#include "Python.h"
#include "pycore_dict.h"          // _PyDict_SetItemValue()
#include "pycore_gc_trace.h"      // _PyGC_LiveCollectHere()
#include "pycore_object.h"        // _PyObject_ComputedDictPointer()
#include "pycore_pymem.h"         // _PyMem_SizeClass()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_shape.h"
#include "pycore_stats.h"         // OBJECT_STAT_INC()
#include "pycore_typeobject.h"    // _PyType_GetDict()
#include "pycore_unicodeobject.h" // _PyUnicode_Equal()
#include "pycore_value.h"         // _PyThreadState_PushValueRoot()

/* Set by _testinternalcapi.set_conversion_collect(): conversion to
   dictionary mode runs a collection just before it publishes the dict, the
   one point where a missing root would lose a value. */
int _PyShape_ConversionCollect = 0;

const _PySlotKindRange _PySlotKind_Ranges[_Py_SLOT_NKINDS] = {
    [_Py_SLOT_BOXED] = {0, UINT64_MAX},
    [_Py_SLOT_I32] = {_PyValue_INT_BASE, UINT32_MAX},
    [_Py_SLOT_F64] = {_PyValue_DOUBLE_OFFSET,
                      _PyValue_INT_BASE - _PyValue_DOUBLE_OFFSET - 1},
};

static uint64_t generalisations;
static uint64_t shapes_reversioned;

void
_PyShape_InitRoot(PyTypeObject *type)
{
    _PyShape *root = _Py_TYPE_ROOT_SHAPE(type);
    root->type = type;
    root->flags = type->tp_flags;
    root->type_version = type->tp_version_tag;
    root->class_version = type->tp_version_tag;
    root->shape_kind = _Py_SHAPE_ROOT;
}

int
_PyShape_InitTypeShapes(PyHeapTypeObject *ht, int capacity)
{
    PyTypeObject *type = &ht->ht_type;
    assert(ht->ht_shapes == NULL);
    assert(0 < capacity && capacity <= _Py_SHAPE_MAX_SLOTS);
    _PyTypeShapes *shapes = PyMem_Calloc(1, sizeof(_PyTypeShapes));
    if (shapes == NULL) {
        return -1;
    }
    _PyShape *root = _Py_TYPE_ROOT_SHAPE(type);
    assert(root->type == type && root->children == NULL);
    shapes->nshapes = 1;
    shapes->capacity = (uint8_t)capacity;
    shapes->shapes[0] = root;
    shapes->dict_shape = (_PyShape){
        .type = type,
        .flags = type->tp_flags,
        .type_version = type->tp_version_tag,
        .class_version = type->tp_version_tag,
        .shape_kind = _Py_SHAPE_DICT,
    };
    ht->ht_shapes = shapes;
    return 0;
}

/* Add the names in the __static_attributes__ tuple of TYPE's dict to SET. */
static int
add_static_attributes(PyTypeObject *type, PyObject *set)
{
    PyObject *dict = _PyType_GetDict(type);
    if (dict == NULL) {
        return 0;
    }
    PyObject *names;
    if (PyDict_GetItemRef(dict, &_Py_ID(__static_attributes__), &names) < 0) {
        return -1;
    }
    if (names == NULL || !PyTuple_Check(names)) {
        return 0;
    }
    for (Py_ssize_t i = 0; i < PyTuple_GET_SIZE(names); i++) {
        PyObject *name = PyTuple_GET_ITEM(names, i);
        if (PyUnicode_CheckExact(name) && PySet_Add(set, name) < 0) {
            return -1;
        }
    }
    return 0;
}

int
_PyType_SlotCapacity(PyTypeObject *type)
{
    /* S is the number of distinct names that the classes on the MRO assign
       to self in their methods, the attributes an instance will likely
       have; two more leave room for others. */
    PyObject *set = PySet_New(NULL);
    if (set == NULL) {
        return -1;
    }
    PyObject *mro = type->tp_mro;
    assert(mro != NULL && PyTuple_Check(mro));
    for (Py_ssize_t i = 0; i < PyTuple_GET_SIZE(mro); i++) {
        PyTypeObject *base = (PyTypeObject *)PyTuple_GET_ITEM(mro, i);
        if (add_static_attributes(base, set) < 0) {
            return -1;
        }
    }
    Py_ssize_t s = PySet_GET_SIZE(set);
    // The block holds the slots its size class has room for: the bytes
    // past B + 8w would be allocated anyway.
    size_t want = s > 0 ? (size_t)Py_MIN(s + 2, _Py_SHAPE_MAX_SLOTS)
                        : _Py_SHAPE_MIN_SLOTS;
    size_t basic = (size_t)type->tp_basicsize;
    size_t room = (_PyMem_SizeClass(basic + 8 * want) - basic) / 8;
    assert(room >= want);
    return (int)Py_MIN(room, (size_t)_Py_SHAPE_MAX_SLOTS);
}

/* TYPE's dict has a new __static_attributes__, or lost it.  Until TYPE has
 * allocated an instance or gained a subclass, whose capacity counts TYPE's
 * names, the capacity follows the names, so that a class decorator that
 * generates __init__ (dataclasses) can declare what it assigns.  After
 * that the capacity is fixed and the assignment has no effect on it. */
int
_PyType_StaticAttributesChanged(PyTypeObject *type)
{
    if (!(type->tp_flags & Py_TPFLAGS_INLINE_VALUES)) {
        return 0;
    }
    _PyTypeShapes *shapes = _PyType_Shapes(type);
    if (shapes->allocated || _PyType_HasSubclasses(type)) {
        return 0;
    }
    // Only an instance adds shapes; an object given TYPE by a __class__
    // assignment is in dictionary mode and uses slot 0 alone.
    assert(shapes->nshapes == 1);
    int capacity = _PyType_SlotCapacity(type);
    if (capacity < 0) {
        return -1;
    }
    shapes->capacity = (uint8_t)capacity;
    return 0;
}

void
_PyShape_FreeTypeShapes(PyHeapTypeObject *ht)
{
    _PyTypeShapes *shapes = ht->ht_shapes;
    if (shapes == NULL) {
        return;
    }
    ht->ht_shapes = NULL;
    // The root is part of the type; the others are blocks of their own,
    // each with its slot table after it.
    for (int i = 1; i < shapes->nshapes; i++) {
        PyMem_Free(shapes->shapes[i]);
    }
    _PyShape *root = _Py_TYPE_ROOT_SHAPE(&ht->ht_type);
    root->children = NULL;
    PyMem_Free(shapes);
}

size_t
_PyShape_TypeShapesSize(PyHeapTypeObject *ht)
{
    _PyTypeShapes *shapes = ht->ht_shapes;
    if (shapes == NULL) {
        return 0;
    }
    size_t size = sizeof(_PyTypeShapes);
    for (int i = 1; i < shapes->nshapes; i++) {
        size += sizeof(_PyShape) + shapes->shapes[i]->nslots * sizeof(_PyShapeSlot);
    }
    return size;
}

int
_PyShape_TraverseTypeShapes(PyHeapTypeObject *ht, visitproc visit, void *arg)
{
    _PyTypeShapes *shapes = ht->ht_shapes;
    if (shapes == NULL) {
        return 0;
    }
    // Every slot name is the edge name of an ADD edge at or above its shape,
    // so visiting the edge names visits every name the table holds.
    for (int i = 1; i < shapes->nshapes; i++) {
        _PyShape *shape = shapes->shapes[i];
        Py_VISIT(shape->edge_name);
    }
    return 0;
}

static inline _PyTypeShapes *
type_shapes(PyTypeObject *type)
{
    if (!(type->tp_flags & Py_TPFLAGS_HEAPTYPE)) {
        return NULL;
    }
    return ((PyHeapTypeObject *)type)->ht_shapes;
}

void
_PyShape_SetTypeVersion(PyTypeObject *type, uint32_t version)
{
    // The class guards read class_version from the header in place of
    // tp_version_tag, so every shape must hold the type's version.
    assert(type->tp_version_tag == version);
    _PyShape *root = _Py_TYPE_ROOT_SHAPE(type);
    root->type_version = version;
    root->class_version = version;
    _PyTypeShapes *shapes = type_shapes(type);
    if (shapes != NULL) {
        for (int i = 1; i < shapes->nshapes; i++) {
            shapes->shapes[i]->type_version = version;
            shapes->shapes[i]->class_version = version;
        }
        shapes->dict_shape.type_version = version;
        shapes->dict_shape.class_version = version;
    }
}

void
_PyShape_SetTypeFlags(PyTypeObject *type, unsigned long flags)
{
    _Py_TYPE_ROOT_SHAPE(type)->flags = flags;
    _PyTypeShapes *shapes = type_shapes(type);
    if (shapes != NULL) {
        for (int i = 1; i < shapes->nshapes; i++) {
            shapes->shapes[i]->flags = flags;
        }
        shapes->dict_shape.flags = flags;
    }
}

_PyShape *
_PyType_DictModeShape(PyTypeObject *type)
{
    _PyTypeShapes *shapes = type_shapes(type);
    if (shapes == NULL) {
        return _Py_TYPE_ROOT_SHAPE(type);
    }
    return &shapes->dict_shape;
}

int
_PyType_HasShapeAt(PyTypeObject *type, const void *addr)
{
    if (addr == (const void *)_Py_TYPE_ROOT_SHAPE(type)) {
        return 1;
    }
    _PyTypeShapes *shapes = type_shapes(type);
    if (shapes == NULL) {
        return 0;
    }
    if (addr == (const void *)&shapes->dict_shape) {
        return 1;
    }
    for (int i = 1; i < shapes->nshapes; i++) {
        if (addr == (const void *)shapes->shapes[i]) {
            return 1;
        }
    }
    return 0;
}

Py_ssize_t
_PyShape_SlotOf(_PyShape *shape, PyObject *name)
{
    assert(PyUnicode_CheckExact(name));
    for (Py_ssize_t i = 0; i < shape->nslots; i++) {
        if (shape->slots[i].name == name) {
            return i;
        }
    }
    if (PyUnicode_CHECK_INTERNED(name)) {
        // Slot names are interned, so an interned name is found by identity.
        return -1;
    }
    for (Py_ssize_t i = 0; i < shape->nslots; i++) {
        PyObject *s = shape->slots[i].name;
        if (s != NULL && _PyUnicode_Equal(s, name)) {
            return i;
        }
    }
    return -1;
}

static _PyShape *
find_edge(_PyShape *from, int kind, PyObject *name)
{
    for (_PyShape *c = from->children; c != NULL; c = c->sibling) {
        if (c->shape_kind == kind && c->edge_name == name) {
            return c;
        }
    }
    return NULL;
}

/* A new child of FROM by an edge of KIND for NAME at slot IX, with NSLOTS
   slots, entered in the table; an ADD edge's field has FIELD_KIND.  NULL
   with MemoryError set. */
static _PyShape *
new_shape(_PyTypeShapes *shapes, _PyShape *from, int kind, PyObject *name,
          Py_ssize_t ix, uint16_t nslots, uint8_t field_kind)
{
    _PyShape *shape = PyMem_Malloc(sizeof(_PyShape)
                                   + nslots * sizeof(_PyShapeSlot));
    if (shape == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    _PyShapeSlot *slots = (_PyShapeSlot *)(shape + 1);
    for (uint16_t i = 0; i < nslots; i++) {
        slots[i] = i < from->nslots ? from->slots[i]
                                    : (_PyShapeSlot){NULL, _Py_SLOT_BOXED};
    }
    // Kinds are copied from the parent, so a field has its kind in every
    // shape below its owner.
    slots[ix] = kind == _Py_SHAPE_ADD ? (_PyShapeSlot){name, field_kind}
                                      : (_PyShapeSlot){NULL, _Py_SLOT_BOXED};
    *shape = (_PyShape){
        .type = from->type,
        .flags = from->flags,
        .type_version = from->type_version,
        .class_version = from->class_version,
        .nslots = nslots,
        .shape_kind = (uint8_t)kind,
        .edge_slot = (uint8_t)ix,
        .parent = from,
        .edge_name = name,
        .children = NULL,
        .sibling = from->children,
        .slots = slots,
    };
    from->children = shape;
    shapes->shapes[shapes->nshapes++] = shape;
    return shape;
}

_PyShape *
_PyShape_FindAdd(_PyShape *from, PyObject *name)
{
    assert(PyUnicode_CheckExact(name));
    for (_PyShape *c = from->children; c != NULL; c = c->sibling) {
        if (c->shape_kind == _Py_SHAPE_ADD
            && (c->edge_name == name
                || (!PyUnicode_CHECK_INTERNED(name)
                    && _PyUnicode_Equal(c->edge_name, name))))
        {
            return c;
        }
    }
    return NULL;
}

_PyShape *
_PyShape_FindDelete(_PyShape *from, Py_ssize_t ix)
{
    assert(0 <= ix && ix < from->nslots && from->slots[ix].name != NULL);
    if (from->shape_kind == _Py_SHAPE_ADD && from->edge_slot == ix) {
        return from->parent;
    }
    return find_edge(from, _Py_SHAPE_DELETE, from->slots[ix].name);
}

_PyShapePrepare
_PyShape_PrepareAdd(_PyShape *from, PyObject *name, uint8_t kind,
                    _PyShape **to)
{
    // Edges are compared by identity, so the name must be interned.
    assert(PyUnicode_CheckExact(name) && PyUnicode_CHECK_INTERNED(name));
    assert(!_PyShape_IsDictMode(from));
    assert(_PyShape_SlotOf(from, name) < 0);
    *to = find_edge(from, _Py_SHAPE_ADD, name);
    if (*to != NULL) {
        return _Py_SHAPE_PREPARE_OK;
    }
    _PyTypeShapes *shapes = type_shapes(from->type);
    assert(shapes != NULL);
    if (from->nslots >= shapes->capacity
        || shapes->nshapes == _Py_SHAPE_MAX_PER_TYPE)
    {
        return _Py_SHAPE_PREPARE_LIMIT;
    }
    *to = new_shape(shapes, from, _Py_SHAPE_ADD, name, from->nslots,
                    from->nslots + 1, kind);
    return *to != NULL ? _Py_SHAPE_PREPARE_OK : _Py_SHAPE_PREPARE_NOMEMORY;
}

_PyShapePrepare
_PyShape_PrepareDelete(_PyShape *from, Py_ssize_t ix, _PyShape **to)
{
    assert(!_PyShape_IsDictMode(from));
    assert(0 <= ix && ix < from->nslots && from->slots[ix].name != NULL);
    if (from->shape_kind == _Py_SHAPE_ADD && from->edge_slot == ix) {
        // The field the last edge added: back to the parent.
        *to = from->parent;
        return _Py_SHAPE_PREPARE_OK;
    }
    PyObject *name = from->slots[ix].name;
    *to = find_edge(from, _Py_SHAPE_DELETE, name);
    if (*to != NULL) {
        return _Py_SHAPE_PREPARE_OK;
    }
    _PyTypeShapes *shapes = type_shapes(from->type);
    assert(shapes != NULL);
    if (shapes->nshapes == _Py_SHAPE_MAX_PER_TYPE) {
        return _Py_SHAPE_PREPARE_LIMIT;
    }
    *to = new_shape(shapes, from, _Py_SHAPE_DELETE, name, ix, from->nslots,
                    _Py_SLOT_BOXED);
    return *to != NULL ? _Py_SHAPE_PREPARE_OK : _Py_SHAPE_PREPARE_NOMEMORY;
}

void
_PyShape_Generalise(_PyShape *shape, Py_ssize_t ix)
{
    assert(!_PyShape_IsDictMode(shape));
    assert(0 <= ix && ix < shape->nslots && shape->slots[ix].name != NULL);
    assert(shape->slots[ix].kind != _Py_SLOT_BOXED);
    // The field's owner is the shape its ADD edge leads to.  A DELETE edge
    // also records its slot, so the edge kind is tested.
    _PyShape *owner = shape;
    while (!(owner->shape_kind == _Py_SHAPE_ADD && owner->edge_slot == ix)) {
        owner = owner->parent;
        assert(owner != NULL && ix < owner->nslots);
    }
    // One fresh version for the subtree: a version is compared only with
    // a shape pointer or with the type, and this one is unique to these
    // shapes.  Every shape below the owner extends it, so it has slot IX,
    // the field or a hole after a delete, whose kind is BOXED already.
    uint32_t version = _PyType_NewShapeVersion(shape->type);
    generalisations++;
    _PyShape *s = owner;
    for (;;) {
        s->slots[ix].kind = _Py_SLOT_BOXED;
        FT_ATOMIC_STORE_UINT32_RELAXED(s->type_version, version);
        shapes_reversioned++;
        if (s->children != NULL) {
            s = s->children;
            continue;
        }
        while (s != owner && s->sibling == NULL) {
            s = s->parent;
        }
        if (s == owner) {
            break;
        }
        s = s->sibling;
    }
}

void
_PyShape_GeneraliseCounts(uint64_t *n, uint64_t *reversioned)
{
    *n = generalisations;
    *reversioned = shapes_reversioned;
}

int
_PyShape_Index(_PyShape *shape)
{
    if (_PyShape_IsDictMode(shape)) {
        return _Py_SHAPE_INDEX_DICT;
    }
    _PyTypeShapes *shapes = type_shapes(shape->type);
    if (shapes == NULL) {
        assert(shape == _Py_TYPE_ROOT_SHAPE(shape->type));
        return 0;
    }
    for (int i = 0; i < shapes->nshapes; i++) {
        if (shapes->shapes[i] == shape) {
            return i;
        }
    }
    Py_UNREACHABLE();
}

int
_PyType_ShapeCount(PyTypeObject *type)
{
    _PyTypeShapes *shapes = type_shapes(type);
    return shapes == NULL ? 1 : shapes->nshapes;
}


/* Instance storage of the types with Py_TPFLAGS_INLINE_VALUES.
 *
 * In shape mode slot i is non-NULL exactly when the shape names it.  In
 * dictionary mode slot 0 holds the dict or NULL and the others are NULL.
 * Dictionary mode is terminal, and after a __class__ assignment the object
 * may have fewer slots than its type's capacity, so code in dictionary mode
 * touches slot 0 only.
 */

#ifndef NDEBUG
static int
slots_match_shape(PyObject *obj)
{
    _PyShape *shape = _PyObject_SHAPE(obj);
    if (_PyShape_IsDictMode(shape)) {
        return 1;
    }
    PyValue *slots = _PyObject_Slots(obj);
    int capacity = _PyType_Shapes(Py_TYPE(obj))->capacity;
    for (int i = 0; i < capacity; i++) {
        int named = i < shape->nslots && shape->slots[i].name != NULL;
        if (named == PyValue_IsNull(slots[i])) {
            return 0;
        }
    }
    return 1;
}
#endif

/* A new dict of OBJ's fields in slot order, without the holes; NULL with an
   exception. */
static PyObject *
dict_from_slots(PyObject *obj)
{
    _PyShape *shape = _PyObject_SHAPE(obj);
    assert(!_PyShape_IsDictMode(shape));
    PyValue *slots = _PyObject_Slots(obj);
    // Room for the fields and the store that converts, in a table of str
    // keys, which the attribute handlers take.
    PyObject *dict = _PyDict_NewPresizedUnicode(shape->nslots + 1);
    if (dict == NULL) {
        return NULL;
    }
#ifndef NDEBUG
    // The table is made at most once: here, or at the first insertion when
    // the dict starts with the shared empty table.
    PyDictKeysObject *keys = NULL;
#endif
    for (Py_ssize_t i = 0; i < shape->nslots; i++) {
        PyObject *name = shape->slots[i].name;
        if (name == NULL) {
            continue;
        }
        if (_PyDict_SetItemValue((PyDictObject *)dict,
                                 PyValue_FromObject(name), slots[i]) < 0) {
            return NULL;
        }
#ifndef NDEBUG
        if (keys == NULL) {
            keys = ((PyDictObject *)dict)->ma_keys;
        }
        assert(((PyDictObject *)dict)->ma_keys == keys);
#endif
    }
    assert(keys == NULL || (DK_IS_UNICODE(keys) && keys->dk_usable > 0));
    return dict;
}

/* Make DICT (or NULL) OBJ's instance dict.  From shape mode the fields are
   cleared first; then slot 0 is written, then the header.  Plain stores
   only: no call and no allocation.  Slot 0 may be an I32 or F64 field
   until the header changes; nothing runs in between, and the dictionary-
   mode shape has no kinds, so no field is generalised.  An entry from
   shape mode is counted for REASON once the header has changed. */
static void
publish_dict(PyObject *obj, PyDictObject *dict, _PyDictModeReason reason)
{
    PyValue *slots = _PyObject_Slots(obj);
    _PyShape *shape = _PyObject_SHAPE(obj);
    int entering = !_PyShape_IsDictMode(shape);
    if (entering) {
        for (Py_ssize_t i = 1; i < shape->nslots; i++) {
            slots[i] = PyValue_NULL;
        }
    }
    slots[0] = dict == NULL ? PyValue_NULL
                            : PyValue_FromObject((PyObject *)dict);
    _PyObject_SetShape(obj, _PyType_DictModeShape(Py_TYPE(obj)));
    OBJECT_STAT_INC_COND(dict_mode[reason], entering);
}

/* Convert OBJ, in shape mode, to dictionary mode.  If NAME is not NULL the
   store of VALUE to NAME (a delete if VALUE is NULL) is applied to the
   provisional dict first.  The incoming value and the provisional dict are
   rooted until publication; the fields are held by OBJ until then.  On a
   failure OBJ is unchanged.  0, or -1 with an exception. */
static int
convert_to_dict(PyObject *obj, PyObject *name, PyValue value,
                _PyDictModeReason reason)
{
    assert(!_PyShape_IsDictMode(_PyObject_SHAPE(obj)));
    PyThreadState *tstate = _PyThreadState_GET();
    _PyValueRoot value_root, dict_root;
    _PyThreadState_PushValueRoot(tstate, &value_root);
    value_root.value = value;
    _PyThreadState_PushValueRoot(tstate, &dict_root);
    int res = -1;
    PyObject *dict = dict_from_slots(obj);
    if (dict == NULL) {
        goto done;
    }
    dict_root.value = PyValue_FromObject(dict);
    if (name != NULL) {
        int rc;
        if (PyValue_IsNull(value)) {
            rc = PyDict_DelItem(dict, name);
            if (rc < 0 && PyErr_ExceptionMatches(PyExc_KeyError)) {
                PyErr_Clear();
                PyErr_Format(PyExc_AttributeError,
                             "'%.100s' object has no attribute '%U'",
                             Py_TYPE(obj)->tp_name, name);
                _PyObject_SetAttributeErrorContext(obj, name);
            }
        }
        else {
            rc = _PyDict_SetItemValue((PyDictObject *)dict,
                                      PyValue_FromObject(name), value);
        }
        if (rc < 0) {
            goto done;
        }
    }
    if (_PyShape_ConversionCollect) {
        // The collection that an allocation here could start: it scans C
        // locals and runs no finalizer, so the object cannot change.
        (void)_PyGC_LiveCollectHere(tstate);
    }
    publish_dict(obj, (PyDictObject *)dict, reason);
    res = 0;
  done:
    _PyThreadState_PopValueRoot(tstate, &dict_root);
    _PyThreadState_PopValueRoot(tstate, &value_root);
    return res;
}

static void
set_no_attribute_error(PyObject *obj, PyObject *name)
{
    PyErr_Format(PyExc_AttributeError,
                 "'%.100s' object has no attribute '%U'",
                 Py_TYPE(obj)->tp_name, name);
    _PyObject_SetAttributeErrorContext(obj, name);
}

/* A store to NAME (a delete if VALUE is NULL) in the dict in slot 0, which
   a set creates if there is none. */
static int
store_slot_dict(PyObject *obj, PyObject *name, PyValue value)
{
    PyDictObject *dict = _PyObject_SlotDict(obj);
    if (dict == NULL) {
        if (PyValue_IsNull(value)) {
            set_no_attribute_error(obj, name);
            return -1;
        }
        dict = (PyDictObject *)PyDict_New();
        if (dict == NULL) {
            return -1;
        }
        _PyObject_Slots(obj)[0] = PyValue_FromObject((PyObject *)dict);
    }
    int res;
    Py_BEGIN_CRITICAL_SECTION(dict);
    res = _PyDict_SetItem_LockHeld(dict, name, value);
    Py_END_CRITICAL_SECTION();
    if (res < 0 && PyValue_IsNull(value)
        && PyErr_ExceptionMatches(PyExc_KeyError))
    {
        PyErr_Clear();
        set_no_attribute_error(obj, name);
    }
    return res;
}

int
_PyObject_ShapeStoreAttr(PyObject *obj, PyObject *name, PyValue value)
{
    assert(Py_TYPE(obj)->tp_flags & Py_TPFLAGS_INLINE_VALUES);
    _PyShape *shape = _PyObject_SHAPE(obj);
    if (_PyShape_IsDictMode(shape)) {
        return store_slot_dict(obj, name, value);
    }
    if (!PyUnicode_CheckExact(name)) {
        // Only exact str names take slots; the dict holds the others.
        return convert_to_dict(obj, name, value, _Py_DICT_MODE_STR_NAME);
    }
    PyValue *slots = _PyObject_Slots(obj);
    Py_ssize_t ix = _PyShape_SlotOf(shape, name);
    _PyShape *to;
    _PyShapePrepare prep;
    if (PyValue_IsNull(value)) {
        if (ix < 0) {
            set_no_attribute_error(obj, name);
            return -1;
        }
        prep = _PyShape_PrepareDelete(shape, ix, &to);
        if (prep == _Py_SHAPE_PREPARE_OK) {
            slots[ix] = PyValue_NULL;
            _PyObject_SetShape(obj, to);
            assert(slots_match_shape(obj));
            return 0;
        }
    }
    else {
        if (ix >= 0) {
            if (!_PyValue_FitsKind(value, shape->slots[ix].kind)) {
                _PyShape_Generalise(shape, ix);
            }
            slots[ix] = value;
            return 0;
        }
        // Edges are keyed by the interned name.  Interning that cannot
        // allocate leaves the name as it was.
        PyObject *key = name;
        _PyUnicode_InternMortal(_PyInterpreterState_GET(), &key);
        if (!PyUnicode_CHECK_INTERNED(key)) {
            PyErr_NoMemory();
            return -1;
        }
        prep = _PyShape_PrepareAdd(shape, key, _PyValue_SlotKind(value), &to);
        if (prep == _Py_SHAPE_PREPARE_OK) {
            // An existing edge's field may have another kind.
            if (!_PyValue_FitsKind(value, to->slots[shape->nslots].kind)) {
                _PyShape_Generalise(to, shape->nslots);
            }
            slots[shape->nslots] = value;
            _PyObject_SetShape(obj, to);
            assert(slots_match_shape(obj));
            return 0;
        }
    }
    if (prep == _Py_SHAPE_PREPARE_NOMEMORY) {
        return -1;
    }
    assert(prep == _Py_SHAPE_PREPARE_LIMIT);
    // A REMOVE needs no slot, so only an ADD can be refused for capacity.
    _PyDictModeReason reason = _Py_DICT_MODE_SHAPE_LIMIT;
    if (!PyValue_IsNull(value)
        && shape->nslots >= type_shapes(shape->type)->capacity)
    {
        reason = _Py_DICT_MODE_CAPACITY;
    }
    return convert_to_dict(obj, name, value, reason);
}

PyDictObject *
_PyObject_EnterDictMode(PyObject *obj, _PyDictModeReason reason)
{
    assert(Py_TYPE(obj)->tp_flags & Py_TPFLAGS_INLINE_VALUES);
    if (!_PyShape_IsDictMode(_PyObject_SHAPE(obj))) {
        if (convert_to_dict(obj, NULL, PyValue_NULL, reason) < 0) {
            return NULL;
        }
    }
    PyDictObject *dict = _PyObject_SlotDict(obj);
    if (dict == NULL) {
        dict = (PyDictObject *)PyDict_New();
        if (dict == NULL) {
            return NULL;
        }
        _PyObject_Slots(obj)[0] = PyValue_FromObject((PyObject *)dict);
    }
    return dict;
}

void
_PyObject_SetSlotDict(PyObject *obj, PyDictObject *dict)
{
    assert(Py_TYPE(obj)->tp_flags & Py_TPFLAGS_INLINE_VALUES);
    publish_dict(obj, dict, dict == NULL ? _Py_DICT_MODE_DICT_DELETED
                                         : _Py_DICT_MODE_DICT_ASSIGNED);
}

int
_PyObject_ShapeIsEmpty(PyObject *obj)
{
    _PyShape *shape = _PyObject_SHAPE(obj);
    if (_PyShape_IsDictMode(shape)) {
        PyDictObject *dict = _PyObject_SlotDict(obj);
        return dict == NULL || PyDict_GET_SIZE(dict) == 0;
    }
    for (Py_ssize_t i = 0; i < shape->nslots; i++) {
        if (shape->slots[i].name != NULL) {
            return 0;
        }
    }
    return 1;
}


/* The managed-dict C API keeps its contract: visit or clear the instance
 * attributes that the runtime stores for the object.  That storage is the
 * attribute slots of a Py_TPFLAGS_INLINE_VALUES type (the shape's fields,
 * or the dict in slot 0 in dictionary mode), and for other types the dict
 * word at tp_dictoffset. */

int
PyObject_VisitManagedDict(PyObject *obj, visitproc visit, void *arg)
{
    PyTypeObject *tp = Py_TYPE(obj);
    if (tp->tp_flags & Py_TPFLAGS_INLINE_VALUES) {
        _PyShape *shape = _PyObject_SHAPE(obj);
        PyValue *slots = _PyObject_Slots(obj);
        Py_ssize_t n = _PyShape_IsDictMode(shape) ? 1 : shape->nslots;
        for (Py_ssize_t i = 0; i < n; i++) {
            _Py_VISIT_VALUE(slots[i]);
        }
        return 0;
    }
    if (tp->tp_dictoffset != 0) {
        PyObject **dictptr = _PyObject_ComputedDictPointer(obj);
        if (dictptr != NULL) {
            Py_VISIT(*dictptr);
        }
    }
    return 0;
}

void
PyObject_ClearManagedDict(PyObject *obj)
{
    // A cleared inline-values object is a valid empty instance: from shape
    // mode it returns to its root shape, and in dictionary mode it stays
    // there with slot 0 NULL.
    PyTypeObject *tp = Py_TYPE(obj);
    if (tp->tp_flags & Py_TPFLAGS_INLINE_VALUES) {
        _PyShape *shape = _PyObject_SHAPE(obj);
        PyValue *slots = _PyObject_Slots(obj);
        if (_PyShape_IsDictMode(shape)) {
            slots[0] = PyValue_NULL;
            return;
        }
        for (Py_ssize_t i = 0; i < shape->nslots; i++) {
            slots[i] = PyValue_NULL;
        }
        _PyObject_SetShape(obj, _Py_TYPE_ROOT_SHAPE(tp));
        return;
    }
    if (tp->tp_dictoffset != 0) {
        PyObject **dictptr = _PyObject_ComputedDictPointer(obj);
        if (dictptr != NULL) {
            *dictptr = NULL;
        }
    }
}
