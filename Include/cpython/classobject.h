/* Former class object interface -- now only bound methods are here  */

/* Revealing some structures (not for general use) */

#ifndef Py_LIMITED_API
#ifndef Py_CLASSOBJECT_H
#define Py_CLASSOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

// The layout of PyMethodObject is internal
// (Include/internal/pycore_classobject.h): im_self is a value word.
typedef struct PyMethodObject PyMethodObject;

PyAPI_DATA(PyTypeObject) PyMethod_Type;

#define PyMethod_Check(op) Py_IS_TYPE((op), &PyMethod_Type)

PyAPI_FUNC(PyObject *) PyMethod_New(PyObject *, PyObject *);

PyAPI_FUNC(PyObject *) PyMethod_Function(PyObject *);
/* A new reference to the instance a method is bound to (a box of an inline
   value). */
PyAPI_FUNC(PyObject *) PyMethod_GetSelfRef(PyObject *);

#define _PyMethod_CAST(meth) \
    (assert(PyMethod_Check(meth)), _Py_CAST(PyMethodObject*, meth))

#ifndef Py_BUILD_CORE
/* Direct access, with no type check; inline inside the core
   (Include/internal/pycore_classobject.h). The borrowed PyMethod_GET_SELF
   is gone: use PyMethod_GetSelfRef(). */
PyAPI_FUNC(PyObject *) PyMethod_GET_FUNCTION(PyObject *meth);
#define PyMethod_GET_FUNCTION(meth) PyMethod_GET_FUNCTION(_PyObject_CAST(meth))
#endif

typedef struct {
    PyObject_HEAD
    PyObject *func;
} PyInstanceMethodObject;

PyAPI_DATA(PyTypeObject) PyInstanceMethod_Type;

#define PyInstanceMethod_Check(op) Py_IS_TYPE((op), &PyInstanceMethod_Type)

PyAPI_FUNC(PyObject *) PyInstanceMethod_New(PyObject *);
PyAPI_FUNC(PyObject *) PyInstanceMethod_Function(PyObject *);

#define _PyInstanceMethod_CAST(meth) \
    (assert(PyInstanceMethod_Check(meth)), \
     _Py_CAST(PyInstanceMethodObject*, meth))

/* Static inline function for direct access to these values.
   Type checks are *not* done, so use with care. */
static inline PyObject* PyInstanceMethod_GET_FUNCTION(PyObject *meth) {
    return _PyInstanceMethod_CAST(meth)->func;
}
#define PyInstanceMethod_GET_FUNCTION(meth) PyInstanceMethod_GET_FUNCTION(_PyObject_CAST(meth))

#ifdef __cplusplus
}
#endif
#endif   // !Py_CLASSOBJECT_H
#endif   // !Py_LIMITED_API
