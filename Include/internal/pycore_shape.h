#ifndef Py_INTERNAL_SHAPE_H
#define Py_INTERNAL_SHAPE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_pyatomic_ft_wrappers.h"  // FT_ATOMIC_LOAD_PTR_ACQUIRE()

/* Shapes (boil/design/09-object-protocol.md, 04-shapes.md).
 *
 * struct _PyShape is declared in Include/object.h, because Py_TYPE() reads
 * its first field.  Every object's header points to its shape; the type is
 * shape->type.  A type owns its shapes:
 *
 * - the root shape, embedded in the type object (tp_root_shape), which is
 *   the only shape of a type whose instances have no attribute slots;
 * - for a type with Py_TPFLAGS_INLINE_VALUES, a table
 *   (PyHeapTypeObject.ht_shapes) of at most _Py_SHAPE_MAX_PER_TYPE shapes,
 *   the root first, and one dictionary-mode shape.
 *
 * An instance of such a type has `capacity` attribute slots, PyValues
 * starting at tp_basicsize; capacity is fixed for the type.  In shape mode
 * slot i holds the shape's field i, or NULL for a hole, and the slots past
 * the shape's nslots are NULL.  A shape is a path of transitions from the
 * root: an ADD edge appends a field as slot nslots, so one insertion order
 * gives one shape; a DELETE edge clears a slot, leaving a hole, unless it
 * deletes the field the last ADD edge appended, which returns to the
 * parent.  Insertion order is slot order without the holes.  In dictionary
 * mode (the type's dictionary-mode shape) slot 0 holds the instance dict or
 * NULL, and the other slots are NULL.  Dictionary mode is terminal.
 *
 * Shapes live as long as their type; feedback entries borrow them.
 */

#define _Py_SHAPE_MAX_PER_TYPE 64
// The most slots a type has: the shared-keys limit it replaces.
#define _Py_SHAPE_MAX_SLOTS 30
// Capacity is min(_Py_SHAPE_MAX_SLOTS, max(_Py_SHAPE_MIN_SLOTS, S + 2)),
// for S the distinct names of __static_attributes__ over the MRO.
#define _Py_SHAPE_MIN_SLOTS 6

typedef enum {
    _Py_SLOT_BOXED = 0,     // any value; the only kind written so far
} _PySlotKind;

typedef enum {
    _Py_SHAPE_ROOT = 0,     // the type's root shape
    _Py_SHAPE_ADD = 1,      // reached from parent by appending edge_name
    _Py_SHAPE_DICT = 2,     // dictionary mode: the attributes are in a dict
    _Py_SHAPE_DELETE = 3,   // reached from parent by clearing edge_slot
} _PyShapeKind;

struct _PyShapeSlot {
    PyObject *name;         // interned exact str; NULL for a hole
    uint8_t kind;           // _PySlotKind
};

typedef struct _PyTypeShapes {
    uint8_t nshapes;                // shapes in use, the root included
    uint8_t capacity;               // the slots of every instance
    uint8_t allocated;              // an instance was allocated: capacity is fixed
    _PyShape dict_shape;            // the type's dictionary-mode shape
    _PyShape *shapes[_Py_SHAPE_MAX_PER_TYPE];  // shapes[0] is the root
} _PyTypeShapes;

/* The index of a dictionary-mode shape for _testinternalcapi. */
#define _Py_SHAPE_INDEX_DICT 0xFF

/* The result of preparing a transition (note 09 section 4.5). */
typedef enum {
    _Py_SHAPE_PREPARE_NOMEMORY = -1,    // MemoryError is set
    _Py_SHAPE_PREPARE_OK = 0,
    _Py_SHAPE_PREPARE_LIMIT = 1,        // capacity or 64 shapes: no exception
} _PyShapePrepare;

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

static inline _PyTypeShapes *
_PyType_Shapes(PyTypeObject *type)
{
    assert(type->tp_flags & Py_TPFLAGS_INLINE_VALUES);
    return ((PyHeapTypeObject *)type)->ht_shapes;
}

/* The bytes of attribute slots in each instance of TYPE. */
static inline size_t
_PyType_SlotsSize(PyTypeObject *type)
{
    if (!(type->tp_flags & Py_TPFLAGS_INLINE_VALUES)) {
        return 0;
    }
    return _PyType_Shapes(type)->capacity * sizeof(PyValue);
}

/* The bytes of attribute slots for a new instance of TYPE.  From the first
 * allocation on, the type's capacity is fixed
 * (_PyType_StaticAttributesChanged()). */
static inline size_t
_PyType_AllocSlotsSize(PyTypeObject *type)
{
    if (!(type->tp_flags & Py_TPFLAGS_INLINE_VALUES)) {
        return 0;
    }
    _PyTypeShapes *shapes = _PyType_Shapes(type);
    shapes->allocated = 1;
    return shapes->capacity * sizeof(PyValue);
}

extern PyObject **_PyObject_ComputedDictPointer(PyObject *);  // pycore_object.h

/* The instance dict of OBJ at DICTOFFSET: a positive offset of the dict
   word (slot 0 of a dictionary-mode object is at tp_basicsize), or a
   negative tp_dictoffset, computed from the end of the object.  Borrowed;
   NULL if there is none. */
static inline PyObject *
_PyObject_DictAt(PyObject *obj, int32_t dictoffset)
{
    assert(dictoffset != 0);
    PyObject **dictptr = dictoffset > 0
        ? (PyObject **)((char *)obj + dictoffset)
        : _PyObject_ComputedDictPointer(obj);
    return FT_ATOMIC_LOAD_PTR_ACQUIRE(*dictptr);
}

/* The size of OBJ's fixed part with its attribute slots, for __sizeof__:
   the current type's capacity, which after a __class__ assignment may
   differ from the allocation's. */
static inline size_t
_PyObject_InstanceSize(PyObject *obj)
{
    return _PyObject_SIZE(Py_TYPE(obj)) + _PyType_SlotsSize(Py_TYPE(obj));
}

/* The attribute slots of OBJ, whose type has Py_TPFLAGS_INLINE_VALUES. */
static inline PyValue *
_PyObject_Slots(PyObject *obj)
{
    PyTypeObject *tp = Py_TYPE(obj);
    assert(tp->tp_flags & Py_TPFLAGS_INLINE_VALUES);
    assert((size_t)tp->tp_basicsize % sizeof(PyValue) == 0);
    return (PyValue *)((char *)obj + tp->tp_basicsize);
}

/* The byte offset of slot IX in objects of TYPE. */
static inline uint32_t
_PyType_SlotOffset(PyTypeObject *type, Py_ssize_t ix)
{
    return (uint32_t)(type->tp_basicsize + ix * sizeof(PyValue));
}

/* The instance dict of OBJ in dictionary mode, or NULL. */
static inline PyDictObject *
_PyObject_SlotDict(PyObject *obj)
{
    assert(_PyShape_IsDictMode(_PyObject_SHAPE(obj)));
    PyValue v = _PyObject_Slots(obj)[0];
    return PyValue_IsNull(v) ? NULL : (PyDictObject *)PyValue_ObjectBorrow(v);
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

// The table of a heap type with inline values, with CAPACITY; 0, or -1
// with no exception.
extern int _PyShape_InitTypeShapes(PyHeapTypeObject *type, int capacity);
extern void _PyShape_FreeTypeShapes(PyHeapTypeObject *type);
// The capacity of a heap type being readied, from its MRO.
extern int _PyType_SlotCapacity(PyTypeObject *type);
extern int _PyType_StaticAttributesChanged(PyTypeObject *type);
// The bytes of the table and of the shapes after the root, for __sizeof__.
extern size_t _PyShape_TypeShapesSize(PyHeapTypeObject *type);
extern int _PyShape_TraverseTypeShapes(PyHeapTypeObject *type,
                                       visitproc visit, void *arg);

// Write the type's version or flags into each of its shapes.
extern void _PyShape_SetTypeVersion(PyTypeObject *type, uint32_t version);
extern void _PyShape_SetTypeFlags(PyTypeObject *type, unsigned long flags);

// The dictionary-mode shape of TYPE, or its root if it has no table.
extern _PyShape *_PyType_DictModeShape(PyTypeObject *type);

// The slot of NAME, an exact str, in SHAPE, or -1.  Runs no code.
PyAPI_FUNC(Py_ssize_t) _PyShape_SlotOf(_PyShape *shape, PyObject *name);

// Preparation, the only steps that make shapes: the shape *TO reached from
// FROM by appending NAME (an interned exact str), or by deleting slot IX.
// Exported for the specialiser and _testinternalcapi.
// The shape an existing edge leads to, or NULL; these allocate nothing.
// FindAdd: the (ADD, NAME) edge; FindDelete: deleting the field at IX.
extern _PyShape *_PyShape_FindAdd(_PyShape *from, PyObject *name);
extern _PyShape *_PyShape_FindDelete(_PyShape *from, Py_ssize_t ix);
PyAPI_FUNC(_PyShapePrepare) _PyShape_PrepareAdd(_PyShape *from, PyObject *name,
                                                _PyShape **to);
PyAPI_FUNC(_PyShapePrepare) _PyShape_PrepareDelete(_PyShape *from,
                                                   Py_ssize_t ix,
                                                   _PyShape **to);

// The position of SHAPE in its type's table, _Py_SHAPE_INDEX_DICT for
// dictionary mode; and the number of shapes of TYPE.
PyAPI_FUNC(int) _PyShape_Index(_PyShape *shape);
PyAPI_FUNC(int) _PyType_ShapeCount(PyTypeObject *type);

/* Instance storage of a type with Py_TPFLAGS_INLINE_VALUES
 * (Objects/shapeobject.c). */

// Store VALUE as attribute NAME of OBJ, or delete NAME if VALUE is NULL:
// the generic path of every store outcome but DATA_DESCR, REPLACE and
// NO_STORAGE.  0, or -1 with an exception and OBJ unchanged.
extern int _PyObject_ShapeStoreAttr(PyObject *obj, PyObject *name,
                                    PyValue value);
// Enter dictionary mode, and return the instance dict (borrowed), made if
// there is none; NULL with an exception, OBJ unchanged.
extern PyDictObject *_PyObject_EnterDictMode(PyObject *obj);
// Replace OBJ's attributes by DICT (NULL: none), in dictionary mode.
extern void _PyObject_SetSlotDict(PyObject *obj, PyDictObject *dict);
// 1 if OBJ has no instance attributes, else 0.
extern int _PyObject_ShapeIsEmpty(PyObject *obj);

/* Test hook: when set, conversion to dictionary mode runs a collection
   just before it publishes the dict (_testinternalcapi). */
PyAPI_DATA(int) _PyShape_ConversionCollect;

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_SHAPE_H */
