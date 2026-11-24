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
#include "pycore_object.h"      // _PyObject_ValueSlot()

typedef const _PyFbAttr *_PyAttrRecipe;

/* The site's recipe when it names an instance slot of a type with a live
   version; NULL for an entry that holds none. */
static inline _PyAttrRecipe
_PyAttr_AdmittedInstanceSlot(const _PyFbAttr *fb, PyObject *name)
{
    (void)name;
    return fb->type_version != 0 ? fb : NULL;
}

/* The site's recipe when it names a method of a type with a live version */
static inline _PyAttrRecipe
_PyAttr_AdmittedMethod(const _PyFbAttr *fb, PyObject *name)
{
    (void)name;
    return fb->type_version != 0 ? fb : NULL;
}

static inline PyValueBorrow
_PyAttr_BorrowMethod(_PyAttrRecipe r)
{
    return _PyValueGuarded_Load(r->class_value);
}

static inline bool
_PyAttr_InstanceVersion(PyObject *o, _PyAttrRecipe r)
{
    return FT_ATOMIC_LOAD_UINT_RELAXED(Py_TYPE(o)->tp_version_tag) == r->type_version;
}

static inline bool
_PyAttr_ValidInlineValues(PyObject *o)
{
    assert(Py_TYPE(o)->tp_flags & Py_TPFLAGS_INLINE_VALUES);
    return FT_ATOMIC_LOAD_UINT8(_PyObject_InlineValues(o)->valid) != 0;
}

static inline PyValueBorrow
_PyAttr_BorrowValueSlot(PyObject *o, _PyAttrRecipe r)
{
    return PyValue_Borrow(*_PyObject_ValueSlot(o, r->offset));
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
