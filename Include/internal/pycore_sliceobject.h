#ifndef Py_INTERNAL_SLICEOBJECT_H
#define Py_INTERNAL_SLICEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


/* A new slice of the values start, stop and step, any of which may be
   PyValue_NULL for None, or NULL with an exception set. */
PyAPI_FUNC(PyObject *)
_PySlice_FromValues(PyValue start, PyValue stop, PyValue step);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_SLICEOBJECT_H */
