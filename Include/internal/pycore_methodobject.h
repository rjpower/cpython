#ifndef Py_INTERNAL_METHODOBJECT_H
#define Py_INTERNAL_METHODOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_value.h"         // PyValue_Borrow()
#include "pycore_value_slots.h"   // _Py_vectorcallvaluefunc

/* Builtin functions and methods (pyjit/design/proposals/value-api-m3.md,
   section 1.2). m_self is a value home: PyValue_NULL for none, else an owner
   of any value, inline ones included. m_value_vectorcall is the value
   vectorcall of a METH_VALUE function, NULL for every other convention;
   when it is set, vectorcall is _PyValue_VectorcallThunk. */
struct PyCFunctionObject {
    PyObject_HEAD
    PyMethodDef *m_ml; /* Description of the C function to call */
    PyValue m_self; /* Passed as 'self' arg to the C func, can be NULL */
    PyObject *m_module; /* The __module__ attribute, can be anything */
    PyObject *m_weakreflist; /* List of weak references */
    vectorcallfunc vectorcall;
    _Py_vectorcallvaluefunc m_value_vectorcall;
};

struct PyCMethodObject {
    PyCFunctionObject func;
    PyTypeObject *mm_class; /* Class that defines this method */
};

/* Direct access, with no type checks. Extensions call the exported
   functions of the same names (Include/cpython/methodobject.h). */
static inline PyCFunction
_PyCFunction_GET_FUNCTION(PyObject *func)
{
    return _PyCFunctionObject_CAST(func)->m_ml->ml_meth;
}
#define PyCFunction_GET_FUNCTION(func) \
    _PyCFunction_GET_FUNCTION(_PyObject_CAST(func))

static inline int
_PyCFunction_GET_FLAGS(PyObject *func)
{
    return _PyCFunctionObject_CAST(func)->m_ml->ml_flags;
}
#define PyCFunction_GET_FLAGS(func) _PyCFunction_GET_FLAGS(_PyObject_CAST(func))

static inline PyTypeObject *
_PyCFunction_GET_CLASS(PyObject *func_obj)
{
    PyCFunctionObject *func = _PyCFunctionObject_CAST(func_obj);
    if (func->m_ml->ml_flags & METH_METHOD) {
        return _PyCMethodObject_CAST(func)->mm_class;
    }
    return NULL;
}
#define PyCFunction_GET_CLASS(func) _PyCFunction_GET_CLASS(_PyObject_CAST(func))

/* The self of func, borrowed: PyValue_NULL for none (no self, or
   METH_STATIC). */
static inline PyValueBorrow
_PyCFunction_GET_SELF_VALUE(PyObject *func_obj)
{
    PyCFunctionObject *func = _PyCFunctionObject_CAST(func_obj);
    if (func->m_ml->ml_flags & METH_STATIC) {
        return PyValue_Borrow(PyValue_NULL);
    }
    return PyValue_Borrow(func->m_self);
}

/* The self of func as a borrowed object, for the interpreter's object-form
   native calls until I5 deletes them: NULL for none; a box of an inline
   self is kept on the borrow stack of the scope that the caller has open
   (kind CALL_ADAPTER). NULL with an exception set if the box fails. */
extern PyObject *_PyCFunction_SelfObject(PyObject *func);

/* A new builtin function or method for ml, bound to self (PyValue_NULL
   for none), which it DUPs: the value form of PyCMethod_New(), which is its
   thunk. cls is the defining class for METH_METHOD, else NULL. */
// Export for '_testinternalcapi' shared extension.
PyAPI_FUNC(PyObject *) _PyCFunction_NewValue(
    PyMethodDef *ml, PyValueBorrow self, PyObject *module, PyTypeObject *cls);
#define _PyCFunction_NewValue(ML, SELF, MODULE, CLS) \
    (_PyCFunction_NewValue)(ML, PyValue_Borrow(SELF), MODULE, CLS)

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_METHODOBJECT_H */
