#ifndef Py_INTERNAL_CRITICAL_SECTION_H
#define Py_INTERNAL_CRITICAL_SECTION_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_lock.h"        // PyMutex_LockFast()
#include "pycore_pystate.h"     // _PyThreadState_GET()
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Tagged pointers to critical sections use the two least significant bits to
// mark if the pointed-to critical section is inactive and whether it is a
// PyCriticalSection2 object.
#define _Py_CRITICAL_SECTION_INACTIVE       0x1
#define _Py_CRITICAL_SECTION_TWO_MUTEXES    0x2
#define _Py_CRITICAL_SECTION_MASK           0x3

// The critical section APIs are no-ops with the GIL.
# define Py_BEGIN_CRITICAL_SECTION_SEQUENCE_FAST(original) {
# define Py_END_CRITICAL_SECTION_SEQUENCE_FAST() }
# define _Py_CRITICAL_SECTION_ASSERT_MUTEX_LOCKED(mutex)
# define _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(op)

// Resumes the top-most critical section.
PyAPI_FUNC(void)
_PyCriticalSection_Resume(PyThreadState *tstate);

// (private) slow path for locking the mutex
PyAPI_FUNC(void)
_PyCriticalSection_BeginSlow(PyThreadState *tstate, PyCriticalSection *c, PyMutex *m);

PyAPI_FUNC(void)
_PyCriticalSection2_BeginSlow(PyThreadState *tstate, PyCriticalSection2 *c, PyMutex *m1, PyMutex *m2,
                             int is_m1_locked);

PyAPI_FUNC(void)
_PyCriticalSection_SuspendAll(PyThreadState *tstate);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CRITICAL_SECTION_H */
