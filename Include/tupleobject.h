/* Tuple object interface */

#ifndef Py_TUPLEOBJECT_H
#define Py_TUPLEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

/*
Another generally useful object type is a tuple of canonical PyValues.
For Python, this is an immutable type.  C code can change the tuple items
(but not their number), and even use tuples as general-purpose arrays of
object references, but in general only brand new tuples should be mutated,
not ones that might already have been exposed to Python code.

*** WARNING *** PyTuple_SetItem does not increment the new item's reference
count, but does decrement the reference count of the item it replaces,
if not nil.  It does *decrement* the reference count if it is *not*
inserted in the tuple. PyTuple_GetItemRef returns an owning object reference;
boxing an inline scalar can fail. The private value ABI has no generic
borrowed-object getter.
*/

PyAPI_DATA(PyTypeObject) PyTuple_Type;
PyAPI_DATA(PyTypeObject) PyTupleIter_Type;

#define PyTuple_Check(op) \
                 PyType_FastSubclass(Py_TYPE(op), Py_TPFLAGS_TUPLE_SUBCLASS)
#define PyTuple_CheckExact(op) Py_IS_TYPE((op), &PyTuple_Type)

PyAPI_FUNC(PyObject *) PyTuple_New(Py_ssize_t size);
PyAPI_FUNC(Py_ssize_t) PyTuple_Size(PyObject *);
Py_VALUE_COMPAT(PyTuple_GetValue)
PyAPI_FUNC(PyObject *) PyTuple_GetItemRef(PyObject *, Py_ssize_t);
Py_VALUE_COMPAT(PyTuple_SetValue)
PyAPI_FUNC(int) PyTuple_SetItem(PyObject *, Py_ssize_t, PyObject *);
PyAPI_FUNC(PyObject *) PyTuple_GetSlice(PyObject *, Py_ssize_t, Py_ssize_t);
Py_VALUE_COMPAT(_PyTuple_PackValues)
PyAPI_FUNC(PyObject *) PyTuple_Pack(Py_ssize_t, ...);

#ifndef Py_LIMITED_API
#  define Py_CPYTHON_TUPLEOBJECT_H
#  include "cpython/tupleobject.h"
#  undef Py_CPYTHON_TUPLEOBJECT_H
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_TUPLEOBJECT_H */
