#ifndef Py_INTERNAL_SHAPE_H
#define Py_INTERNAL_SHAPE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

/* Shapes (boil/design/09-object-protocol.md, 04-shapes.md).
 *
 * struct _PyShape is declared in Include/object.h, because Py_TYPE() reads
 * its first field.  Every object's header points to its shape; the type is
 * shape->type.  A type owns its shapes:
 *
 * - the root shape, embedded in the type object (tp_root_shape), which is
 *   the only shape of a type whose instances have no attribute slots;
 * - for a type with inline values, a table (PyHeapTypeObject.ht_shapes) of
 *   at most _Py_SHAPE_MAX_PER_TYPE shapes, the root first, and one
 *   dictionary-mode shape.
 *
 * A shape of a type with inline values stands for the attributes an
 * instance has and the order in which they were added: it is a path of
 * transitions from the root, each adding one slot.  A slot is the
 * attribute's index in the type's shared keys, so a name has the same slot
 * in every shape of its type, and a shape may have holes.  An instance has
 * a shape on that path while it has valid inline values and no __dict__;
 * otherwise, and past the limit, it has the type's dictionary-mode shape.
 * Shapes live as long as their type; feedback entries borrow them.
 */

#define _Py_SHAPE_MAX_PER_TYPE 64

typedef enum {
    _Py_SLOT_BOXED = 0,     // any value; the only kind written so far
} _PySlotKind;

typedef enum {
    _Py_SHAPE_ROOT = 0,     // the type's root shape
    _Py_SHAPE_ADD = 1,      // reached from parent by adding edge_slot
    _Py_SHAPE_DICT = 2,     // dictionary mode: the attributes are in a dict
} _PyShapeKind;

struct _PyShapeSlot {
    PyObject *name;         // NULL for a hole
    uint8_t kind;           // _PySlotKind
};

typedef struct _PyTypeShapes {
    uint8_t nshapes;                // shapes in use, the root included
    _PyShape dict_shape;            // the type's dictionary-mode shape
    _PyShape *shapes[_Py_SHAPE_MAX_PER_TYPE];  // shapes[0] is the root
} _PyTypeShapes;

/* The index of a dictionary-mode shape for _testinternalcapi. */
#define _Py_SHAPE_INDEX_DICT 0xFF

static inline void
_PyObject_SetShape(PyObject *obj, _PyShape *shape)
{
    assert(shape->type == Py_TYPE(obj));
    obj->ob_header = ((obj->ob_header & ~_Py_HEADER_SHAPE_MASK)
                      | (uintptr_t)shape);
}

static inline int
_PyShape_IsDictMode(_PyShape *shape)
{
    return shape->shape_kind == _Py_SHAPE_DICT;
}

static inline int
_PyShape_HasSlot(_PyShape *shape, Py_ssize_t slot)
{
    return slot < shape->nslots && shape->slots[slot].name != NULL;
}

/* The root shape of a value: an inline int or float has the root shape of
   int or float, a heap object the shape in its header. */
static inline _PyShape *
_PyValue_Shape(PyValue v)
{
    if (PyValue_IsInlineInt(v)) {
        return _Py_TYPE_ROOT_SHAPE(&PyLong_Type);
    }
    if (PyValue_IsInlineFloat(v)) {
        return _Py_TYPE_ROOT_SHAPE(&PyFloat_Type);
    }
    return _PyObject_SHAPE(PyValue_ObjectBorrow(v));
}

// Fill TYPE's root shape from the type: type, flags and version.  Called
// for the static builtin types at runtime start and by PyType_Ready().
extern void _PyShape_InitRoot(PyTypeObject *type);
extern void _PyTypes_InitRootShapes(void);
extern void _PyExc_InitRootShapes(void);

// The table of a heap type with inline values; 0 or -1 with no exception.
extern int _PyShape_InitTypeShapes(PyHeapTypeObject *type);
extern void _PyShape_FreeTypeShapes(PyHeapTypeObject *type);
// The bytes of the table and of the shapes after the root, for __sizeof__.
extern size_t _PyShape_TypeShapesSize(PyHeapTypeObject *type);
extern int _PyShape_TraverseTypeShapes(PyHeapTypeObject *type,
                                       visitproc visit, void *arg);

// Write the type's version or flags into each of its shapes.
extern void _PyShape_SetTypeVersion(PyTypeObject *type, uint32_t version);
extern void _PyShape_SetTypeFlags(PyTypeObject *type, unsigned long flags);

// The dictionary-mode shape of TYPE, or its root if it has no table.
extern _PyShape *_PyType_DictModeShape(PyTypeObject *type);

// The shape reached from FROM by adding SLOT, or NULL past the per-type
// limit or when out of memory.  Never sets an exception.  Exported for the
// specialiser and _testinternalcapi.
PyAPI_FUNC(_PyShape *) _PyShape_Add(_PyShape *from, Py_ssize_t slot);

// The position of SHAPE in its type's table, _Py_SHAPE_INDEX_DICT for
// dictionary mode; and the number of shapes of TYPE.
PyAPI_FUNC(int) _PyShape_Index(_PyShape *shape);
PyAPI_FUNC(int) _PyType_ShapeCount(PyTypeObject *type);

// OBJ has gained slot IX: follow the transition, or enter dictionary mode.
static inline void
_PyObject_ShapeAfterAdd(PyObject *obj, Py_ssize_t ix)
{
    _PyShape *shape = _PyObject_SHAPE(obj);
    if (!_PyShape_IsDictMode(shape)) {
        _PyShape *to = _PyShape_Add(shape, ix);
        _PyObject_SetShape(obj, to ? to : _PyType_DictModeShape(Py_TYPE(obj)));
    }
}

// OBJ's attributes leave the shape's description (a __dict__ exists, or the
// inline values are invalid): it enters dictionary mode.
static inline void
_PyObject_SetDictModeShape(PyObject *obj)
{
    _PyObject_SetShape(obj, _PyType_DictModeShape(Py_TYPE(obj)));
}

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_SHAPE_H */
