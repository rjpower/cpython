#ifndef Py_INTERNAL_CELL_H
#define Py_INTERNAL_CELL_H

#include "pycore_value.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

static inline PyValueBorrow
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
    cell->ob_ref = value;
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

/* The value forms of PyCell_New() and PyCell_Set(): they borrow their input
   (PyValue_NULL for an empty cell) and take a count of their own. Each has a
   reader macro, so the core passes owners and borrows alike. */
PyAPI_FUNC(PyObject *) PyCell_NewValue(PyValueBorrow);
#define PyCell_NewValue(V) (PyCell_NewValue)(PyValue_Borrow(V))
PyAPI_FUNC(int) PyCell_SetValue(PyObject *, PyValueBorrow);
#define PyCell_SetValue(OP, V) (PyCell_SetValue)(OP, PyValue_Borrow(V))

/* An independent owner, or NULL for an empty cell without an exception. */
static inline PyValue
_PyCell_GetValue(PyCellObject *cell)
{
    _PyValue_RecheckHome(cell->ob_ref);
    return PyValue_XDUP(cell->ob_ref);
}

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_CELL_H */
