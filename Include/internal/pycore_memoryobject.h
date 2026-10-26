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

#ifdef Py_GC
/* Would releasing a buffer that exporter exported run Python code: does the
 * exporter's class define __release_buffer__, or is it the wrapper of a
 * buffer that __buffer__ returned?  Then the collector releases a dead
 * memoryview or managed buffer of it before the sweep, which may not run
 * code (Python/gc_live.c). */
extern int _PyBuffer_ReleaseRunsCode(PyObject *exporter);

/* Would the dealloc of a memoryview or managed buffer release a buffer
 * whose release runs code? */
extern int _PyMemoryView_DeallocReleasesWithCode(PyObject *op);

/* Make that release now: what the dealloc would do, apart from freeing. */
extern void _PyMemoryView_ReleaseForDealloc(PyObject *op);
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_MEMORYOBJECT_H */
