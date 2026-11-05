#ifndef Py_INTERNAL_CLASSOBJECT_H
#define Py_INTERNAL_CLASSOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_value.h"         // PyValue_Borrow()
#include "pycore_value_slots.h"   // _Py_vectorcallvaluefunc

/* Bound methods (pyjit/design/proposals/value-api-m3.md, sections 1.2 and
   1.6). im_self is a value home that owns the instance, which may be
   inline. value_vectorcall is always method_value_vectorcall, and
   vectorcall its thunk. */
struct PyMethodObject {
    PyObject_HEAD
    PyObject *im_func;   /* The callable object implementing the method */
    PyValue im_self;     /* The instance it is bound to */
    PyObject *im_weakreflist; /* List of weak references */
    vectorcallfunc vectorcall;
    _Py_vectorcallvaluefunc value_vectorcall;
};

/* Direct access, with no type check. Extensions call the exported function
   of the same name (Include/cpython/classobject.h). */
static inline PyObject *
_PyMethod_GET_FUNCTION(PyObject *meth)
{
    return _PyMethod_CAST(meth)->im_func;
}
#define PyMethod_GET_FUNCTION(meth) _PyMethod_GET_FUNCTION(_PyObject_CAST(meth))

/* The instance a method is bound to, borrowed. */
static inline PyValueBorrow
_PyMethod_GET_SELF_VALUE(PyObject *meth)
{
    return PyValue_Borrow(_PyMethod_CAST(meth)->im_self);
}

/* A new bound method of func and self, which it DUPs: the value form of
   PyMethod_New(), which is its thunk. */
extern PyObject *_PyMethod_NewValue(PyObject *func, PyValueBorrow self);
#define _PyMethod_NewValue(FUNC, SELF) \
    (_PyMethod_NewValue)(FUNC, PyValue_Borrow(SELF))

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CLASSOBJECT_H */
