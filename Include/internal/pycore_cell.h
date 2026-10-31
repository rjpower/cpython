#ifndef Py_INTERNAL_CELL_H
#define Py_INTERNAL_CELL_H

#include "pycore_critical_section.h"
#include "pycore_object.h"
#include "pycore_structs.h"       // _PyGCValueRange
#include "pycore_value.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

static inline _PyGCValueRange
_PyCell_GCValues(PyCellObject *cell)
{
    _PyGCValueRange values = {&cell->ob_ref, 1};
    return values;
}

// Sets the cell contents to the canonical value v and returns the previous
// contents.
static inline PyValue
_PyCell_SwapValue(PyCellObject *cell, PyValue v)
{
    PyValue old_value;
    Py_BEGIN_CRITICAL_SECTION(cell);
    old_value = cell->ob_ref;
    cell->ob_ref = v;
    Py_END_CRITICAL_SECTION();
    return old_value;
}

// The cell contents, PyValue_NULL when empty.
static inline PyValue
_PyCell_GetValue(PyCellObject *cell)
{
    return PyCell_GET_VALUE((PyObject *)cell);
}

// The cell contents as an object: a new box if inline, NULL when empty, or
// NULL with an exception set if boxing fails.
static inline PyObject *
PyCell_GetRef(PyCellObject *cell)
{
    return PyValue_Object(_PyCell_GetValue(cell));
}

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_CELL_H */
