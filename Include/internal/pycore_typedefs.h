#ifndef Py_INTERNAL_TYPEDEFS_H
#define Py_INTERNAL_TYPEDEFS_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _PyInterpreterFrame _PyInterpreterFrame;
typedef struct pyruntimestate _PyRuntimeState;

/* The trace and profile function of a thread (pyjit/design/proposals/
 * value-api-m3.md, section 1.8): Py_tracefunc with the argument as a word.
 * The argument is a borrow, PyValue_NULL for an absent one; the event handlers
 * call it with the thread's c_traceobj or c_profileobj. */
typedef int (*_Py_valuetracefunc)(PyObject *obj, PyFrameObject *frame,
                                  int what, PyValueBorrow arg);

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_TYPEDEFS_H
