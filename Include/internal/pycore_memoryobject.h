#ifndef Py_INTERNAL_MEMORYOBJECT_H
#define Py_INTERNAL_MEMORYOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

extern PyTypeObject _PyManagedBuffer_Type;

PyObject *
_PyMemoryView_FromBufferProc(PyObject *v, int flags,
                             getbufferproc bufferproc);

/* Would releasing a buffer that exporter exported run Python code: does the
 * exporter's class define __release_buffer__, or is it the wrapper of a
 * buffer that __buffer__ returned?  Then the collector releases a dead
 * memoryview or managed buffer of it before the sweep, which may not run
 * code (Python/gc_live.c). */
extern int _PyBuffer_ReleaseRunsCode(PyObject *exporter);

/* Would the dealloc of a memoryview or managed buffer release a buffer
 * whose release runs code? */
extern int _PyMemoryView_DeallocReleasesWithCode(PyObject *op);

/* Whether a memoryview can unregister, or a managed buffer release its master,
 * without discarding outstanding exports.  This inspection runs no code. */
extern int _PyMemoryView_ReleaseReady(PyObject *op);

/* Recheck readiness and perform that release.  Return zero without consuming
 * the acquisition if an outstanding export still prevents it. */
extern int _PyMemoryView_ReleaseForDealloc(PyObject *op);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_MEMORYOBJECT_H */
