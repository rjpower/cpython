#ifndef Py_INTERNAL_VALUE_API_H
#define Py_INTERNAL_VALUE_API_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_borrow.h"        // _PyBorrow_TupleItems()
#include "pycore_value.h"         // PyValue_AsObject()

/* Value API primitives of packet V1 (pyjit/design/value-api.md): the entry
   primitives of section 1.3 and the box counters of section 6.1. */


/* Entry, typed (section 1.3): the borrowed value of obj, which must not be
   an exact int or float: a str, type, function, iterator or container. It
   skips the type load of PyValue_FromObjectBorrow(), and tags the pointer
   if obj is immortal; debug builds check the precondition. NULL gives
   PyValue_NULL. */
static inline PyValueBorrow
_PyValue_FromObjectKnown(PyObject *obj)
{
    assert((uintptr_t)obj < Py_PTR_LIMIT);
#ifdef Py_DEBUG
    if (obj != NULL && (PyLong_CheckExact(obj) || PyFloat_CheckExact(obj))) {
        _PyObject_ASSERT_FAILED_MSG(
            obj, "_PyValue_FromObjectKnown() needs an object that cannot "
                 "be an exact int or float");
    }
#endif
    PyValueBorrow value = {(uintptr_t)obj};
    if (obj != NULL && _Py_IsImmortal(obj)) {
        value.bits |= Py_TAG_REFCNT;
    }
    return value;
}


#ifdef Py_STATS
/* Box counters (section 6.1). A boxing call names the class of its site
   (BOX_SITE_* in Include/cpython/pystats.h) with an At form below; the
   class holds on the thread for the duration of that call, and
   _PyValue_BoxInlineValue() counts each box under it. Builds without
   Py_STATS drop the class. */

// Set the current thread's site class and return the previous one.
PyAPI_FUNC(int) _PyValue_SwapBoxSite(int site);
// _PyValue_BoxSiteOr(), the current class, is in pycore_value.h.
// Set the current thread's box kind (BOX_KIND_*) and return the previous.
PyAPI_FUNC(int) _PyValue_SwapBoxKind(int kind);

static inline PyObject *
_PyValue_AsObjectAt(PyValueBorrow value, int site)
{
    int previous = _PyValue_SwapBoxSite(site);
    PyObject *object = PyValue_AsObject(value);
    _PyValue_SwapBoxSite(previous);
    return object;
}
#define _PyValue_AsObjectAt(V0, V1) (_PyValue_AsObjectAt)(PyValue_Borrow(V0), V1)

/* PyValue_AsObject() under a box kind (BOX_KIND_*) and, with the At form, a
   site class: the kind argument of the box primitive. */
static inline PyObject *
_PyValue_AsObjectKind(PyValueBorrow value, int kind)
{
    int previous = _PyValue_SwapBoxKind(kind);
    PyObject *object = PyValue_AsObject(value);
    _PyValue_SwapBoxKind(previous);
    return object;
}
#define _PyValue_AsObjectKind(V0, V1) (_PyValue_AsObjectKind)(PyValue_Borrow(V0), V1)

static inline PyObject *
_PyValue_AsObjectAtKind(PyValueBorrow value, int site, int kind)
{
    int previous = _PyValue_SwapBoxKind(kind);
    PyObject *object = _PyValue_AsObjectAt(value, site);
    _PyValue_SwapBoxKind(previous);
    return object;
}
#define _PyValue_AsObjectAtKind(V0, V1, V2) \
    (_PyValue_AsObjectAtKind)(PyValue_Borrow(V0), V1, V2)

static inline PyObject *
_PyValueTemps_ObjectAt(_PyValueTemps *temps, PyValueBorrow value, int site)
{
    int previous = _PyValue_SwapBoxSite(site);
    PyObject *object = _PyValueTemps_Object(temps, value);
    _PyValue_SwapBoxSite(previous);
    return object;
}
#define _PyValueTemps_ObjectAt(V0, V1, V2) (_PyValueTemps_ObjectAt)(V0, PyValue_Borrow(V1), V2)

static inline PyObject *const *
_PyBorrow_TupleItemsAt(PyObject *tuple, Py_ssize_t n, int site)
{
    int previous = _PyValue_SwapBoxSite(site);
    PyObject *const *items = _PyBorrow_TupleItems(tuple, n);
    _PyValue_SwapBoxSite(previous);
    return items;
}
#else
#  define _PyValue_AsObjectAt(VALUE, SITE) PyValue_AsObject(VALUE)
#  define _PyValue_AsObjectKind(VALUE, KIND) PyValue_AsObject(VALUE)
#  define _PyValue_AsObjectAtKind(VALUE, SITE, KIND) PyValue_AsObject(VALUE)
#  define _PyValueTemps_ObjectAt(TEMPS, VALUE, SITE) \
        _PyValueTemps_Object((TEMPS), (VALUE))
#  define _PyBorrow_TupleItemsAt(TUPLE, N, SITE) \
        _PyBorrow_TupleItems((TUPLE), (N))
#endif


/* The marked declaration that Tools/build/check_value_api.py calls from a
   value-native unit to prove that Py_VALUE_COMPAT stops the build. It has
   no definition. */
Py_VALUE_COMPAT(_PyValue_CompatProbeReplacement)
PyAPI_FUNC(void) _PyValue_CompatProbe(void);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_VALUE_API_H */
