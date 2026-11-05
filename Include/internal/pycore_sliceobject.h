#ifndef Py_INTERNAL_SLICEOBJECT_H
#define Py_INTERNAL_SLICEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_pyvalue.h"       // PyValue_Borrow()

/* A slice object. start, stop and step are value fields: each owns a
   canonical value of any type, None for an omitted one, and is never
   PyValue_NULL (pyjit/design/proposals/value-api-v2a0.md, step M4b). */
typedef struct {
    PyObject_HEAD
    PyValue start, stop, step;
} PySliceObject;

/* A new slice of start, stop and step, which are borrowed; PyValue_NULL
   means "not given" and stores None. Until register IR v2 step A5b-3 lands,
   an object argument must be the view of an owner or an immortal: the
   tagging borrow of PyValue_FromObjectBorrow() does not give it a count. */
PyAPI_FUNC(PyObject *) _PySlice_FromValues(PyValueBorrow start,
                                           PyValueBorrow stop,
                                           PyValueBorrow step);
#define _PySlice_FromValues(V0, V1, V2) \
    (_PySlice_FromValues)(PyValue_Borrow(V0), PyValue_Borrow(V1), \
                          PyValue_Borrow(V2))

// (start, stop, step), a new tuple of the values of the fields.
extern PyObject *_PySlice_AsTuple(PySliceObject *slice);

PyAPI_FUNC(int) _PySlice_GetLongIndices(PySliceObject *self, PyObject *length,
                                        PyObject **start_ptr,
                                        PyObject **stop_ptr,
                                        PyObject **step_ptr);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_SLICEOBJECT_H */
