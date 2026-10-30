#ifndef Py_INTERNAL_MEMORYOBJECT_H
#define Py_INTERNAL_MEMORYOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

extern PyTypeObject _PyManagedBuffer_Type;

/* Sealed per-acquisition storage for internal buffer consumers.  The master
 * address is stable; NULL from GetBuffer means that acquisition was released. */
PyAPI_FUNC(PyObject *) _PyManagedBuffer_FromObject(PyObject *base, int flags);
PyAPI_FUNC(const Py_buffer *) _PyManagedBuffer_GetBuffer(
    _PyManagedBufferObject *self);
PyAPI_FUNC(void) _PyManagedBuffer_Release(_PyManagedBufferObject *self);

PyObject *
_PyMemoryView_FromBufferProc(PyObject *v, int flags,
                             getbufferproc bufferproc);

/* An active memoryview registration or managed-buffer master acquisition,
 * regardless of whether the exporter has a release callback. */
extern int _PyMemoryView_HasAcquisition(PyObject *op);

/* Whether a memoryview can unregister, or a managed buffer release its master,
 * without discarding outstanding exports.  This inspection runs no code. */
extern int _PyMemoryView_ReleaseReady(PyObject *op);

/* Recheck readiness and perform that release.  Return one only for actual
 * consumption, zero for an already consumed or still unready acquisition. */
extern int _PyMemoryView_ReleaseForDealloc(PyObject *op);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_MEMORYOBJECT_H */
