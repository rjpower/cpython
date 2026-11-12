#ifndef Py_INTERNAL_GENOBJECT_H
#define Py_INTERNAL_GENOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_value.h"         // _PyStatusValue
#include "pycore_frame.h"         // FRAME_STATE_FINISHED()
#include "pycore_interpframe_structs.h" // _PyGenObject

#include <stddef.h>               // offsetof()


static inline
PyGenObject *_PyGen_GetGeneratorFromFrame(_PyInterpreterFrame *frame)
{
    assert(frame->owner == FRAME_OWNED_BY_GENERATOR);
    size_t offset_in_gen = offsetof(PyGenObject, gi_iframe);
    return (PyGenObject *)(((char *)frame) - offset_in_gen);
}

// The generator finalizer is a no-op once its frame is cleared.
static inline int
_PyGen_FinalizerIsNoop(PyObject *op)
{
    return Py_TYPE(op)->tp_finalize == PyGen_Type.tp_finalize &&
        FRAME_STATE_FINISHED(((PyGenObject *)op)->gi_frame_state);
}

PyAPI_FUNC(PyObject *)_PyGen_yf(PyGenObject *);
extern int _PyGen_ClearFrame(PyGenObject *self);

/* Exact native positional construction from the explicitly selected code.
 * MISS (0) and ERROR (-1) leave the function and argument homes unchanged;
 * SUCCESS (1) moves and NULLs them, returning one canonical object owner. */
PyAPI_FUNC(_PyStatusValue) _PyGen_TryMakeFromFunction(
    PyThreadState *tstate, PyValue *function, PyCodeObject *code,
    PyValue *args, int supplied);

/* Set StopIteration(value), or a bare StopIteration for PyValue_NULL.
   Return 0, or -1 with another exception set. StopIteration's value field
   is an object, so an inline value is boxed (value-api-m1b-m2.md, B6).
   Export for '_asyncio' shared extension. */
PyAPI_FUNC(int) _PyGen_SetStopIterationValue(PyValueBorrow value);
#define _PyGen_SetStopIterationValue(V) \
    (_PyGen_SetStopIterationValue)(PyValue_Borrow(V))

/* Fetch and clear a pending StopIteration and return its value, None if
   it has none or if no exception is set. With another exception set,
   return PyValue_NULL and leave it.
   Export for '_asyncio' shared extension. */
PyAPI_FUNC(PyValue) _PyGen_FetchStopIterationValue(void);
// Set the exception passed to throw(typ[, val[, tb]]).
// Return 0 on success, -1 on failure.
extern int _PyGen_SetException(PyObject *typ, PyObject *val, PyObject *tb);

PyAPI_FUNC(PyObject *)_PyCoro_GetAwaitableIter(PyObject *o);
PyAPI_FUNC(PyObject *)_PyAsyncGenValueWrapperNew(PyThreadState *state, PyObject *);

// Exported for external JIT support
PyAPI_FUNC(PyObject *) _PyCoro_ComputeOrigin(int origin_depth, _PyInterpreterFrame *current_frame);

extern PyTypeObject _PyCoroWrapper_Type;
extern PyTypeObject _PyAsyncGenWrappedValue_Type;
extern PyTypeObject _PyAsyncGenAThrow_Type;

PyAPI_FUNC(PySendResult) _PyAsyncGenASend_Send(PyObject *iter, PyObject *arg, PyObject **result);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_GENOBJECT_H */
