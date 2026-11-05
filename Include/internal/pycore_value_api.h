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


/* Box counters (section 6.1). A boxing call names the class of its site
   (BOX_SITE_* in Include/cpython/pystats.h) with an At form, such as
   _PyValue_AsObjectAt() in pycore_value.h, and a boundary box states its
   kind and owner with a For form (_PyBox_AT() in pycore_value.h). Builds
   without Py_STATS drop the argument. */

/* The borrow entries of the call adapter (pycore_borrow.h) with a stated
   box argument: the argument parser's owner, or the callee's C entry
   (_PyCall_BoxOwner()). Without one, an entry's boxes are of kind THUNK
   (_PyBorrow_TupleItems) or CALL_ADAPTER (_PyBorrow_ValueVector), owned by
   its caller. */
#ifdef Py_STATS
PyAPI_FUNC(PyObject *const *) _PyBorrow_TupleItemsFor(PyObject *tuple,
                                                      Py_ssize_t n,
                                                      _PyBoxAt at);
PyAPI_FUNC(PyObject **) _PyBorrow_ValueVectorFor(const PyValue *values,
                                                 Py_ssize_t n, _PyBoxAt at);
/* The C entry that a call adapter's boxes serve, for a callable: a
   function's ml_meth, a wrapper's d_wrapped, a type's tp_vectorcall,
   tp_init or tp_new, else its vectorcall function or tp_call. Runs no
   Python and counts nothing. */
PyAPI_FUNC(const void *) _PyCall_BoxOwner(PyObject *callable);
#else
#  define _PyBorrow_TupleItemsFor(TUPLE, N, AT) \
        _PyBorrow_TupleItems((TUPLE), (N))
#  define _PyBorrow_ValueVectorFor(VALUES, N, AT) \
        _PyBorrow_ValueVector((VALUES), (N))
#endif
/* The THUNK forms of a compatibility entry (Py_VALUE_ENTRY), which boxes for
   its caller (pyjit/design/proposals/value-api-g.md, section 1.2): kind
   THUNK, owned by the entry's caller, which the macro captures where it
   expands, in the entry's own body. Only the bodies of marked functions may
   use them (compat-capture in Tools/build/check_value_api.py).
   _PyValue_THUNK_NEW() is a new reference to the object that V holds, or a
   box of it; _PyValue_THUNK_AT() is the box argument itself, which an entry
   passes to a static helper that it shares with core callers.
   _PyValue_THUNK_EXIT() is in pycore_value_slots.h. */
#define _PyValue_THUNK_AT(SITE) \
    _PyBox_AT((SITE), BOX_KIND_THUNK, _Py_BOX_CALLER())
/* An inline function behind a macro, as _PyValue_THUNK_EXIT() is, so that
   builds without stats do not expand it to PyValue_AsObject() in the body
   of a compatibility entry (compat-body). The macro captures the owner. */
static inline PyObject *
_PyValue_THUNK_New(PyValueBorrow value _PyBox_AT_PARAM)
{
    return _PyValue_AsObjectFor(value, at);
}
#define _PyValue_THUNK_NEW(V, SITE) \
    (_PyValue_THUNK_New)(PyValue_Borrow(V) \
                         _PyBox_AT_ARG(_PyValue_THUNK_AT(SITE)))

// The box argument of a call adapter whose callee is CALLABLE.
#define _PyBox_CALL_ADAPTER(CALLABLE) \
    _PyBox_AT(BOX_SITE_CALL_ADAPTER, BOX_KIND_CALL_ADAPTER, \
              _PyCall_BoxOwner(CALLABLE))


/* The marked declaration that Tools/build/check_value_api.py calls from a
   value-native unit to prove that Py_VALUE_COMPAT stops the build. It has
   no definition. */
Py_VALUE_COMPAT(_PyValue_CompatProbeReplacement)
PyAPI_FUNC(void) _PyValue_CompatProbe(void);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_VALUE_API_H */
