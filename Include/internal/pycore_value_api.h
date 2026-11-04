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
   tags the pointer without loading the type; debug builds check the
   precondition. NULL gives PyValue_NULL. */
static inline PyValue
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
    // The no-count tag, which NULL does not take.
    return (PyValue){ .bits = (uintptr_t)obj | (uintptr_t)(obj != NULL) };
}


#ifdef Py_STATS
/* Box counters (section 6.1). A boxing call names the class of its site
   (BOX_SITE_* in Include/cpython/pystats.h) with an At form below; the
   class holds on the thread for the duration of that call, and
   _PyValue_BoxInlineValue() counts each box under it. Builds without
   Py_STATS drop the class. */

// Set the current thread's site class and return the previous one.
PyAPI_FUNC(int) _PyValue_SwapBoxSite(int site);
// The current thread's site class, or fallback if none is set.
PyAPI_FUNC(int) _PyValue_BoxSiteOr(int fallback);

static inline PyObject *
_PyValue_AsObjectAt(PyValue value, int site)
{
    int previous = _PyValue_SwapBoxSite(site);
    PyObject *object = PyValue_AsObject(value);
    _PyValue_SwapBoxSite(previous);
    return object;
}

static inline PyObject *
_PyValueTemps_ObjectAt(_PyValueTemps *temps, PyValue value, int site)
{
    int previous = _PyValue_SwapBoxSite(site);
    PyObject *object = _PyValueTemps_Object(temps, value);
    _PyValue_SwapBoxSite(previous);
    return object;
}

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
#  define _PyValueTemps_ObjectAt(TEMPS, VALUE, SITE) \
        _PyValueTemps_Object((TEMPS), (VALUE))
#  define _PyBorrow_TupleItemsAt(TUPLE, N, SITE) \
        _PyBorrow_TupleItems((TUPLE), (N))
#endif


/* Entry, borrowed (section 1.3): the canonical value of obj with no count.
   The pointer case is inline: a type load, two compares and the tag. Only
   an exact int or float calls PyValue_FromObjectBorrow(), which makes it
   inline when it fits. NULL gives PyValue_NULL. */
static inline PyValue
_PyValue_FromObjectBorrowInline(PyObject *obj)
{
    if (obj == NULL) {
        return PyValue_NULL;
    }
    if (!_PyValue_IsNumberType(Py_TYPE(obj))) {
        return _PyValue_FromObjectKnown(obj);
    }
    PyValue value = PyValue_FromObjectBorrow(obj);
#ifdef Py_STATS
    if (PyValue_IsInline(value)) {
        _Py_STATS_EXPR(object_stats.entry_unbox[
            _PyValue_BoxSiteOr(BOX_SITE_UNATTRIBUTED)]++);
    }
#endif
    return value;
}


/* The marked declaration that Tools/build/check_value_api.py calls from a
   value-native unit to prove that Py_VALUE_COMPAT stops the build. It has
   no definition. */
Py_VALUE_COMPAT(_PyValue_CompatProbeReplacement)
PyAPI_FUNC(void) _PyValue_CompatProbe(void);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_VALUE_API_H */
