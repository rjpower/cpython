#ifndef _Py_CPYTHON_SLICEOBJECT_H
#  error "this header file must not be included directly"
#endif

/* Slice object interface */

/*
A slice object containing start, stop, and step data members (the
names are from range).  After much talk with Guido, it was decided to
let these be any arbitrary python type.  None stands for omitted values.
Like every container, a slice holds values (Include/pyvalue.h): an int
bound is usually an inline int, not an object.
*/
typedef struct {
    PyObject_HEAD
    PyValue start, stop, step;          /* not PyValue_NULL */
} PySliceObject;

PyAPI_FUNC(PyObject *) _PySlice_FromIndices(Py_ssize_t start, Py_ssize_t stop);
PyAPI_FUNC(int) _PySlice_GetLongIndices(PySliceObject *self, PyObject *length,
                                 PyObject **start_ptr, PyObject **stop_ptr,
                                 PyObject **step_ptr);
