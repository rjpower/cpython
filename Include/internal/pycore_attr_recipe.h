/* Rows of the attribute families' recipes (Python/register_ops/getattr.rop).

   A recipe is evidence for one site's name on one receiver type: the type
   version and the payload that go with it, read together from the site's
   feedback entry.  Under the GIL nothing runs between the read of the recipe
   and its use inside one arm, so the recipe is the live entry. */

#ifndef Py_INTERNAL_ATTR_RECIPE_H
#define Py_INTERNAL_ATTR_RECIPE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_feedback.h"    // _PyFbAttr
#include "pycore_dict.h"        // _PyObject_Shape()
#include "pycore_ir.h"          // _PyIR_FbBlock()
#include "pycore_object.h"      // _PyObject_ValueSlot()
#include "pycore_rop_metadata.h" // ROP_HANDLER_GetMethod_MethodValues

/* One recipe: the guards of a lookup path with the payload that goes with
   them, copied together from the site's entry or from one of its poly rows.
   A site that is installed or refreshed concurrently cannot show a version
   from one observation and an offset from another. */
typedef struct {
    uint32_t type_version;
    uint32_t shape;             // the owner's shape, for a shape read
    uint32_t offset;            // a slot's byte offset
    uint32_t read_kind;         // _Py_FB_READ_*
    bool values_layout;         // a method row whose receiver has inline values
    _PyValueGuarded class_value;    // the cached method or class value
} _PyAttrRecipe;

/* The site's recipe when its entry holds a live type version */
static inline bool
_PyAttr_AdmittedInstanceSlot(const _PyFbAttr *fb, PyObject *name,
                             _PyAttrRecipe *out)
{
    (void)name;
    if (fb->type_version == 0) {
        return false;
    }
    out->type_version = fb->type_version;
    out->offset = fb->offset;
    out->shape = 0;
    out->read_kind = 0;
    out->values_layout = false;
    return true;
}

/* The site's recipe when it names a method of a type with a live version.
   The entry's read_kind is not used by methods. */
static inline bool
_PyAttr_AdmittedMethod(const _PyFbAttr *fb, PyObject *name, _PyAttrRecipe *out)
{
    (void)name;
    if (fb->type_version == 0) {
        return false;
    }
    out->type_version = fb->type_version;
    out->offset = 0;
    out->shape = 0;
    out->read_kind = 0;
    out->values_layout = true;
    out->class_value = fb->class_value;
    return true;
}

/* The site's recipe for a shape read: the entry holds the type version and
   the shape of the owners it was nominated on */
static inline bool
_PyAttr_AdmittedShape(const _PyFbAttr *fb, PyObject *name, _PyAttrRecipe *out)
{
    (void)name;
#ifdef Py_SHAPES
    if (fb->type_version == 0 || fb->read_kind != _Py_FB_READ_SHAPE) {
        return false;
    }
    out->type_version = fb->type_version;
    out->shape = fb->shape;
    out->offset = fb->offset;
    out->read_kind = _Py_FB_READ_SHAPE;
    out->values_layout = false;
    return true;
#else
    (void)fb; (void)out;
    return false;
#endif
}

/* The row of a poly table that records the type version, or -1.  A row
   holds one observation: version, auxiliary word, index and payload. */
static inline int
_PyAttr_PolyRow(const _PyInterpreterFrame *frame, const _PyFbAttr *fb,
                uint32_t version)
{
    if (version == 0 || fb->h.poly == 0) {
        return -1;
    }
    const _PyFbPoly *poly = _PyFb_PolyOf(_PyIR_FbBlock(frame->ir_words), &fb->h);
    for (int i = 0; i < FB_POLY_SIZE; i++) {
        if (poly->type_version[i] == version) {
            return i;
        }
    }
    return -1;
}

/* Poly shape rows: the row of the owner's type, whose shape is the owner's */
static inline bool
_PyAttr_PolyShape(const _PyInterpreterFrame *frame, const _PyFbAttr *fb,
                  PyObject *o, _PyAttrRecipe *out)
{
#ifdef Py_SHAPES
    uint32_t version = FT_ATOMIC_LOAD_UINT_RELAXED(Py_TYPE(o)->tp_version_tag);
    int i = _PyAttr_PolyRow(frame, fb, version);
    if (i < 0) {
        return false;
    }
    const _PyFbPoly *poly = _PyFb_PolyOf(_PyIR_FbBlock(frame->ir_words), &fb->h);
    if (poly->payload[i].read_kind != _Py_FB_READ_SHAPE
        || _PyObject_Shape(o) != poly->aux[i])
    {
        return false;
    }
    out->type_version = version;
    out->shape = poly->aux[i];
    out->offset = poly->index[i];
    out->read_kind = _Py_FB_READ_SHAPE;
    out->values_layout = false;
    return true;
#else
    (void)frame; (void)fb; (void)o; (void)out;
    return false;
#endif
}

/* Poly method rows: the row of the owner's type.  A row observed on a
   receiver with inline values is shadowed by an instance attribute unless
   the receiver's values are still valid (_PyAttr_ValidReceiver). */
static inline bool
_PyAttr_PolyMethod(const _PyInterpreterFrame *frame, const _PyFbAttr *fb,
                   PyObject *o, _PyAttrRecipe *out)
{
    uint32_t version = FT_ATOMIC_LOAD_UINT_RELAXED(Py_TYPE(o)->tp_version_tag);
    int i = _PyAttr_PolyRow(frame, fb, version);
    if (i < 0) {
        return false;
    }
    const _PyFbPoly *poly = _PyFb_PolyOf(_PyIR_FbBlock(frame->ir_words), &fb->h);
    out->type_version = version;
    out->shape = 0;
    out->offset = 0;
    out->read_kind = 0;
    out->values_layout = poly->handler[i] == ROP_HANDLER_GetMethod_MethodValues;
    out->class_value = poly->payload[i].class_value;
    return true;
}

/* A method row that was observed on an inline-values receiver also needs
   this receiver's values to be valid */
static inline bool
_PyAttr_ValidReceiver(PyObject *o, _PyAttrRecipe r)
{
    return !r.values_layout || _PyAttr_ValidInlineValues(o);
}

static inline bool
_PyAttr_ShapeCurrent(PyObject *o, _PyAttrRecipe r)
{
#ifdef Py_SHAPES
    return _PyObject_Shape(o) == r.shape;
#else
    (void)o; (void)r;
    return false;
#endif
}

static inline PyValueBorrow
_PyAttr_BorrowMethod(_PyAttrRecipe r)
{
    return _PyValueGuarded_Load(r.class_value);
}

static inline bool
_PyAttr_InstanceVersion(PyObject *o, _PyAttrRecipe r)
{
    return FT_ATOMIC_LOAD_UINT_RELAXED(Py_TYPE(o)->tp_version_tag) == r.type_version;
}

static inline bool
_PyAttr_ValidInlineValues(PyObject *o)
{
    assert(Py_TYPE(o)->tp_flags & Py_TPFLAGS_INLINE_VALUES);
    return FT_ATOMIC_LOAD_UINT8(_PyObject_InlineValues(o)->valid) != 0;
}

static inline PyValueBorrow
_PyAttr_BorrowValueSlotFilled(PyObject *o, _PyAttrRecipe r)
{
    PyValueBorrow found = PyValue_Borrow(*_PyObject_ValueSlot(o, r.offset));
    assert(!PyValue_IsNull(found));
    return found;
}

static inline PyValueBorrow
_PyAttr_BorrowValueSlot(PyObject *o, _PyAttrRecipe r)
{
    return PyValue_Borrow(*_PyObject_ValueSlot(o, r.offset));
}

static inline bool
_PyValue_BorrowPresent(PyValueBorrow b)
{
    return !PyValue_IsNull(b);
}

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_ATTR_RECIPE_H */
