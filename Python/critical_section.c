#include "Python.h"

#include "pycore_critical_section.h"
#include "pycore_interp.h"
#include "pycore_lock.h"



void
_PyCriticalSection_BeginSlow(PyThreadState *tstate, PyCriticalSection *c, PyMutex *m)
{
}

void
_PyCriticalSection2_BeginSlow(PyThreadState *tstate, PyCriticalSection2 *c, PyMutex *m1, PyMutex *m2,
                              int is_m1_locked)
{
}


// Release all locks held by critical sections. This is called by
// _PyThreadState_Detach.
void
_PyCriticalSection_SuspendAll(PyThreadState *tstate)
{
}

void
_PyCriticalSection_Resume(PyThreadState *tstate)
{
}

#undef PyCriticalSection_Begin
void
PyCriticalSection_Begin(PyCriticalSection *c, PyObject *op)
{
}

#undef PyCriticalSection_BeginMutex
void
PyCriticalSection_BeginMutex(PyCriticalSection *c, PyMutex *m)
{
}

#undef PyCriticalSection_End
void
PyCriticalSection_End(PyCriticalSection *c)
{
}

#undef PyCriticalSection2_Begin
void
PyCriticalSection2_Begin(PyCriticalSection2 *c, PyObject *a, PyObject *b)
{
}

#undef PyCriticalSection2_BeginMutex
void
PyCriticalSection2_BeginMutex(PyCriticalSection2 *c, PyMutex *m1, PyMutex *m2)
{
}

#undef PyCriticalSection2_End
void
PyCriticalSection2_End(PyCriticalSection2 *c)
{
}
