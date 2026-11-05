#ifndef _Py_CPYTHON_SLICEOBJECT_H
#  error "this header file must not be included directly"
#endif

/* Slice object interface. The layout of a slice is internal
   (Include/internal/pycore_sliceobject.h). */

PyAPI_FUNC(PyObject *) _PySlice_FromIndices(Py_ssize_t start, Py_ssize_t stop);
