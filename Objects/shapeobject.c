/* Shapes: the hidden classes in the object header.
 *
 * See Include/internal/pycore_shape.h and boil/design/09-object-protocol.md.
 * Shapes are not objects.  The root shape is part of its type; the others,
 * of a heap type with inline values, are PyMem blocks owned by the type's
 * table and freed with it.
 */

#include "Python.h"
#include "pycore_dict.h"          // _PyDictKeys_AsSharedKeys()
#include "pycore_shape.h"
#include "pycore_typeobject.h"    // _PyStaticType_GetState()

void
_PyShape_InitRoot(PyTypeObject *type)
{
    _PyShape *root = _Py_TYPE_ROOT_SHAPE(type);
    root->type = type;
    root->flags = type->tp_flags;
    root->type_version = type->tp_version_tag;
    root->shape_kind = _Py_SHAPE_ROOT;
}

int
_PyShape_InitTypeShapes(PyHeapTypeObject *ht)
{
    PyTypeObject *type = &ht->ht_type;
    assert(ht->ht_shapes == NULL);
    _PyTypeShapes *shapes = PyMem_Calloc(1, sizeof(_PyTypeShapes));
    if (shapes == NULL) {
        return -1;
    }
    _PyShape *root = _Py_TYPE_ROOT_SHAPE(type);
    assert(root->type == type && root->children == NULL);
    shapes->nshapes = 1;
    shapes->shapes[0] = root;
    shapes->dict_shape = (_PyShape){
        .type = type,
        .flags = type->tp_flags,
        .type_version = type->tp_version_tag,
        .shape_kind = _Py_SHAPE_DICT,
    };
    ht->ht_shapes = shapes;
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
    _Py_TYPE_ROOT_SHAPE(type)->type_version = version;
    _PyTypeShapes *shapes = type_shapes(type);
    if (shapes != NULL) {
        for (int i = 1; i < shapes->nshapes; i++) {
            shapes->shapes[i]->type_version = version;
        }
        shapes->dict_shape.type_version = version;
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

/* The name of slot SLOT of TYPE: its entry in the type's shared keys. */
static PyObject *
slot_name(PyTypeObject *type, Py_ssize_t slot)
{
    PyDictKeysObject *keys = ((PyHeapTypeObject *)type)->ht_cached_keys;
    assert(keys != NULL && slot < keys->dk_nentries);
    return DK_UNICODE_ENTRIES(keys)[slot].me_key;
}

_PyShape *
_PyShape_Add(_PyShape *from, Py_ssize_t slot)
{
    PyTypeObject *type = from->type;
    _PyTypeShapes *shapes = type_shapes(type);
    assert(shapes != NULL);
    assert(!_PyShape_IsDictMode(from));
    assert(0 <= slot && slot < SHARED_KEYS_MAX_SIZE);
    assert(!_PyShape_HasSlot(from, slot));
    for (_PyShape *child = from->children; child != NULL; child = child->sibling) {
        if (child->edge_slot == slot) {
            return child;
        }
    }
    if (shapes->nshapes == _Py_SHAPE_MAX_PER_TYPE) {
        return NULL;
    }
    uint16_t nslots = (uint16_t)Py_MAX((Py_ssize_t)from->nslots, slot + 1);
    _PyShape *shape = PyMem_Malloc(sizeof(_PyShape)
                                   + nslots * sizeof(_PyShapeSlot));
    if (shape == NULL) {
        return NULL;
    }
    _PyShapeSlot *slots = (_PyShapeSlot *)(shape + 1);
    for (uint16_t i = 0; i < nslots; i++) {
        slots[i] = i < from->nslots ? from->slots[i]
                                    : (_PyShapeSlot){NULL, _Py_SLOT_BOXED};
    }
    PyObject *name = slot_name(type, slot);
    slots[slot] = (_PyShapeSlot){name, _Py_SLOT_BOXED};
    *shape = (_PyShape){
        .type = type,
        .flags = from->flags,
        .type_version = from->type_version,
        .nslots = nslots,
        .shape_kind = _Py_SHAPE_ADD,
        .edge_slot = (uint8_t)slot,
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
