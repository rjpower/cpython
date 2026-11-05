#ifndef Py_CPYTHON_METHODOBJECT_H
#  error "this header file must not be included directly"
#endif

// The layouts of PyCFunctionObject and PyCMethodObject are internal
// (Include/internal/pycore_methodobject.h): self is a value word.

typedef struct PyCFunctionObject PyCFunctionObject;
typedef struct PyCMethodObject PyCMethodObject;

#define _PyCFunctionObject_CAST(func) \
    (assert(PyCFunction_Check(func)), \
     _Py_CAST(PyCFunctionObject*, (func)))

#define _PyCMethodObject_CAST(func) \
    (assert(PyCMethod_Check(func)), \
     _Py_CAST(PyCMethodObject*, (func)))

PyAPI_DATA(PyTypeObject) PyCMethod_Type;

#define PyCMethod_CheckExact(op) Py_IS_TYPE((op), &PyCMethod_Type)
#define PyCMethod_Check(op) PyObject_TypeCheck((op), &PyCMethod_Type)


/* A new reference to the self of a builtin function or method (a box of an
   inline value), or NULL with no exception set if it has none (a module
   function's module is its self). */
PyAPI_FUNC(PyObject *) PyCFunction_GetSelfRef(PyObject *func);

#ifndef Py_BUILD_CORE
/* Direct access, with no type checks. Inside the core these are inline
   (Include/internal/pycore_methodobject.h). The borrowed PyCFunction_GET_SELF
   is gone: an inline self has no object to lend; use
   PyCFunction_GetSelfRef(). */
PyAPI_FUNC(PyCFunction) PyCFunction_GET_FUNCTION(PyObject *func);
#define PyCFunction_GET_FUNCTION(func) \
    PyCFunction_GET_FUNCTION(_PyObject_CAST(func))
PyAPI_FUNC(int) PyCFunction_GET_FLAGS(PyObject *func);
#define PyCFunction_GET_FLAGS(func) PyCFunction_GET_FLAGS(_PyObject_CAST(func))
PyAPI_FUNC(PyTypeObject *) PyCFunction_GET_CLASS(PyObject *func);
#define PyCFunction_GET_CLASS(func) PyCFunction_GET_CLASS(_PyObject_CAST(func))
#endif
