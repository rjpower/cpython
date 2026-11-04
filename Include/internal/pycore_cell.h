#ifndef Py_INTERNAL_CELL_H
#define Py_INTERNAL_CELL_H

#include "pycore_value.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

static inline PyValue
PyCell_GET_VALUE(PyObject *op)
{
    assert(PyCell_Check(op));
    return PyValue_Borrow(_Py_CAST(PyCellObject *, op)->ob_ref);
}
#define PyCell_GET_VALUE(op) PyCell_GET_VALUE(_PyObject_CAST(op))

/* Move/promote value without releasing anything. Detach the moved source
   before closing the returned previous owner. */
static inline PyValue
_PyCell_SwapValue(PyCellObject *cell, PyValue value)
{
    assert(PyValue_IsLanguageValue(value) || PyValue_IsNull(value));
    PyValue previous = cell->ob_ref;
    cell->ob_ref = PyValue_MakeHeapSafe(value);
    _PyValue_CheckHome(cell->ob_ref);
    return previous;
}

/* Consumes value; its source must already be detached from any live home. */
static inline void
_PyCell_SetValue(PyCellObject *cell, PyValue value)
{
    PyValue previous = _PyCell_SwapValue(cell, value);
    PyValue_XCLOSE(previous);
}

/* An independent owner, or NULL for an empty cell without an exception. */
static inline PyValue
_PyCell_GetValue(PyCellObject *cell)
{
    PyValue value = cell->ob_ref;
    assert(PyValue_IsHeapSafe(value));
    return PyValue_IsNull(value) ? value : PyValue_DUP(value);
}

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_CELL_H */
