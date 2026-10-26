#ifndef Py_INTERNAL_PYCAPSULE_H
#define Py_INTERNAL_PYCAPSULE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

// Export for '_socket' shared extension
PyAPI_FUNC(int) _PyCapsule_SetTraverse(PyObject *op, traverseproc traverse_func, inquiry clear_func);

#ifdef Py_GC
/* The collector calls a dead capsule's destructor before the sweep, which
   it may not run in (Python/gc_live.c); capsule_dealloc() does not. */
extern int _PyCapsule_HasDestructor(PyObject *op);
extern void _PyCapsule_Destroy(PyObject *op);
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_PYCAPSULE_H */
